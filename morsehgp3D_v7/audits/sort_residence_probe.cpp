// Audit-only allocation probe. Does not alter the implementation under audit.
// Checks stable output and an explicit heap budget on a fixed local fixture.
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#include <src/parallel/sort.hpp>

namespace probe {
constexpr size_t n = 32768;
constexpr size_t slice_buffer_bytes = n;
std::atomic<bool> enabled{false};
std::atomic<size_t> live{0}, peak{0}, buffer_arrivals{0};
std::atomic<size_t> active_buffers{0}, peak_buffers{0};
struct alignas(std::max_align_t) Header {
  size_t bytes;
  bool counted;
  bool slice_buffer;
};
void update_peak(std::atomic<size_t>& target, size_t value) {
  size_t seen = target.load();
  while (seen < value && !target.compare_exchange_weak(seen, value)) {}
}
void* allocate(size_t bytes) {
  if (bytes > static_cast<size_t>(-1) - sizeof(Header)) throw std::bad_alloc();
  auto* h = static_cast<Header*>(std::malloc(sizeof(Header) + bytes));
  if (h == nullptr) throw std::bad_alloc();
  h->bytes = bytes;
  h->counted = enabled.load();
  h->slice_buffer = h->counted && bytes == slice_buffer_bytes;
  if (h->counted) update_peak(peak, live.fetch_add(bytes) + bytes);
  if (h->slice_buffer) {
    update_peak(peak_buffers, active_buffers.fetch_add(1) + 1);
    buffer_arrivals.fetch_add(1);
  }
  return h + 1;
}
void release(void* ptr) noexcept {
  if (ptr == nullptr) return;
  auto* h = static_cast<Header*>(ptr) - 1;
  if (h->slice_buffer) active_buffers.fetch_sub(1);
  if (h->counted) live.fetch_sub(h->bytes);
  std::free(h);
}
}  // namespace probe

void* operator new(size_t bytes) { return probe::allocate(bytes); }
void* operator new[](size_t bytes) { return probe::allocate(bytes); }
void operator delete(void* ptr) noexcept { probe::release(ptr); }
void operator delete[](void* ptr) noexcept { probe::release(ptr); }
void operator delete(void* ptr, size_t) noexcept { probe::release(ptr); }
void operator delete[](void* ptr, size_t) noexcept { probe::release(ptr); }

struct Record {
  mhgp7::u32 key = 0, position = 0;
  std::array<mhgp7::u64, 8> payload{};
  bool operator==(const Record&) const = default;
};

int main(int argc, char** argv) {
  const bool observe = argc == 2 && std::strcmp(argv[1], "--observe") == 0;
  if (argc != 1 && !observe) return 2;
  std::vector<Record> records(probe::n);
  for (size_t i = 0; i < records.size(); ++i) {
    records[i].key = static_cast<mhgp7::u32>((i * 491u + 7u) % 31u);
    records[i].position = static_cast<mhgp7::u32>(i);
    records[i].payload.fill(i * 13u + 19u);
  }
  auto reference = records;
  const auto less = [](const Record& a, const Record& b) { return a.key < b.key; };
  std::stable_sort(reference.begin(), reference.end(), less);
  probe::enabled.store(true);
  const size_t workers = mhgp7::parallel_stable_sort_vector(&records, less, 2);
  probe::enabled.store(false);
  const bool stable = records == reference;
  // Named local test budget: 8n bytes of index arrays, at most 4096 bytes of
  // heap metadata at two workers. This is neither a general RSS bound nor a
  // claim that the C++ library must implement sorting without allocations.
  const size_t heap_budget = 8 * probe::n + 4096;
  const size_t measured = probe::peak.load();
  const bool bounded = measured <= heap_budget && probe::buffer_arrivals.load() == 0;
  std::printf("n=%zu record_bytes=%zu workers=%zu stable=%d "
              "slice_buffers=%zu simultaneous_buffers=%zu "
              "additional_peak_bytes=%zu heap_budget_bytes=%zu "
              "live_after=%zu bounded=%d\n",
              probe::n, sizeof(Record), workers, stable,
              probe::buffer_arrivals.load(), probe::peak_buffers.load(),
              measured, heap_budget, probe::live.load(), bounded);
  if (!stable || workers != 2 || probe::live.load() != 0) return 3;
  return observe || bounded ? 0 : 4;
}
