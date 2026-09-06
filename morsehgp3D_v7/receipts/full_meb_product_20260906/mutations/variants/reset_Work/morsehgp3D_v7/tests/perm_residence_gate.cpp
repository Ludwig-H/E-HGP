// Regression de residence revelee par l'audit independant du 2026-09-04.
// Mesure les demandes new/delete, PAS le RSS ni les piles natives. La scene
// conserve une charge utile distincte a cle egale; le temoin historique impose
// la coexistence des deux tampons stable_sort locaux pour prouver la sonde.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <string>
#include <thread>
#include <vector>

#include "../src/parallel/sort.hpp"

namespace allocation_probe {
std::atomic<bool> enabled{false};
std::atomic<size_t> live{0}, peak{0}, calls{0}, index_arrays{0}, other_large{0};
std::atomic<size_t> arrivals{0}, slice_live{0}, slice_peak{0};
std::atomic<bool> deadline_missed{false};
size_t cardinal = 0, fail_at = 0;
bool rendezvous = false;

struct alignas(std::max_align_t) Header {
  size_t bytes;
  bool counted, slice;
};

void update_peak(std::atomic<size_t>& target, size_t value) {
  size_t observed = target.load();
  while (observed < value && !target.compare_exchange_weak(observed, value)) {}
}

void* allocate(size_t bytes) {
  const bool counted = enabled.load();
  if (counted && calls.fetch_add(1) + 1 == fail_at) throw std::bad_alloc();
  if (bytes > std::numeric_limits<size_t>::max() - sizeof(Header)) throw std::bad_alloc();
  auto* header = static_cast<Header*>(std::malloc(sizeof(Header) + std::max<size_t>(1, bytes)));
  if (header == nullptr) throw std::bad_alloc();
  header->bytes = bytes;
  header->counted = counted;
  header->slice = counted && rendezvous && bytes == cardinal;
  if (counted) {
    update_peak(peak, live.fetch_add(bytes) + bytes);
    if (bytes == 4 * cardinal) ++index_arrays;
    else if (bytes >= cardinal) ++other_large;
  }
  if (header->slice) {
    update_peak(slice_peak, slice_live.fetch_add(1) + 1);
    ++arrivals;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (arrivals.load() < 2) {
      if (std::chrono::steady_clock::now() >= deadline) {
        deadline_missed.store(true);
        break;
      }
      std::this_thread::yield();
    }
  }
  return header + 1;
}

void release(void* pointer) noexcept {
  if (pointer == nullptr) return;
  auto* header = static_cast<Header*>(pointer) - 1;
  if (header->slice) --slice_live;
  if (header->counted) live.fetch_sub(header->bytes);
  std::free(header);
}

bool arm(size_t n, bool legacy = false, size_t allocation_failure = 0) {
  if (enabled.load() || live.load() != 0 || slice_live.load() != 0) return false;
  cardinal = n;
  fail_at = allocation_failure;
  rendezvous = legacy;
  peak.store(0);
  calls.store(0);
  index_arrays.store(0);
  other_large.store(0);
  arrivals.store(0);
  slice_peak.store(0);
  deadline_missed.store(false);
  enabled.store(true);
  return true;
}
void disarm() { enabled.store(false); }
}  // namespace allocation_probe

void* operator new(size_t bytes) { return allocation_probe::allocate(bytes); }
void* operator new[](size_t bytes) { return allocation_probe::allocate(bytes); }
void operator delete(void* pointer) noexcept { allocation_probe::release(pointer); }
void operator delete[](void* pointer) noexcept { allocation_probe::release(pointer); }
void operator delete(void* pointer, size_t) noexcept { allocation_probe::release(pointer); }
void operator delete[](void* pointer, size_t) noexcept { allocation_probe::release(pointer); }

namespace {
using namespace mhgp7;
namespace probe = allocation_probe;

struct Record {
  u32 key = 0, position = 0;
  std::array<u64, 8> payload{};
  bool operator==(const Record&) const = default;
};
static_assert(sizeof(Record) == 72 && alignof(Record) <= alignof(std::max_align_t));

bool less(const Record& a, const Record& b) { return a.key < b.key; }

std::vector<Record> records(size_t n) {
  std::vector<Record> result(n);
  for (size_t i = 0; i < n; ++i) {
    result[i].key = static_cast<u32>((i * 491u + 7u) % 31u);
    result[i].position = static_cast<u32>(i);
    result[i].payload.fill(i * 13u + 19u);
  }
  return result;
}

size_t legacy_permutation(std::vector<Record>* input, int threads) {
  std::vector<u32> indices(input->size());
  for (size_t i = 0; i < indices.size(); ++i) indices[i] = static_cast<u32>(i);
  const parallel_sort_detail::IndexLess comparator{input->begin(), less, false};
  const size_t workers = parallel_sort_detail::sort_direct(indices.begin(), indices.end(), comparator, threads);
  parallel_sort_detail::apply_permutation_cycles(input->begin(), &indices);
  return workers;
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  if (argc == 2) {
    const std::string argument = argv[1];
    if (argument != "--inject=perm-apply-scatter" && argument != "--inject=perm-apply-partial" &&
        argument != "--inject=perm-tie-desc") return 2;
    inject = argument.substr(9);
    if (!mutants_enable(inject)) return 2;
  } else if (argc != 1) {
    return 2;
  }
  size_t failures = 0, cases = 0, ties = 0, different = 0, unsorted = 0;
  size_t allocation_failures = 0, comparator_failures = 0;
  size_t parallel_peak = 0, sequential_peak = 0, allocation_count = 0;
  for (size_t n : {size_t{1000}, size_t{32768}, size_t{65536}}) {
    const auto base = records(n);
    auto reference = base;
    std::stable_sort(reference.begin(), reference.end(), less);
    for (size_t i = 1; i < n; ++i) if (reference[i - 1].key == reference[i].key) ++ties;
    for (int threads : {1, 2, 4, 8}) {
      auto input = base;
      if (!probe::arm(n)) return 3;
      const size_t workers = parallel_stable_sort_vector(&input, less, threads);
      probe::disarm();
      const size_t expected_workers = n < kParallelSortMinElems ? 1 : planned_workers(n / kParallelSortMinSlice, threads);
      if (input != reference) ++different;
      if (!std::is_sorted(input.begin(), input.end(), less)) ++unsorted;
      if (workers != expected_workers || probe::live.load() != 0 || parallel_detail::launch_active.load() != 0) ++failures;
      if (inject.empty() && (probe::index_arrays.load() != (workers == 1 ? 1u : 2u) ||
                            probe::other_large.load() != 0 || probe::peak.load() < 4 * n ||
                            probe::peak.load() > (workers == 1 ? 4 : 8) * n + 4096 * workers)) ++failures;
      if (n == 32768 && threads == 1) sequential_peak = probe::peak.load();
      if (n == 32768 && threads == 2) {
        parallel_peak = probe::peak.load();
        allocation_count = probe::calls.load();
      }
      // Generic stable route is kept independent of the index specialization.
      if (inject.empty()) {
        input = base;
        stable_sort_direct_route(input.begin(), input.end(), less, threads);
        if (input != reference) ++failures;
      }
      ++cases;
    }
  }
  if (!inject.empty()) {
    const bool signature = inject == "perm-tie-desc" ? different == cases && unsorted == 0 : different == cases && unsorted > 0;
    std::printf("perm_residence mutant=%s cases=%zu differences=%zu unsorted=%zu failures=%zu\n",
                inject.c_str(), cases, different, unsorted, failures);
    return signature && failures == 0 && ties > 90000 ? 4 : 1;
  }
  if (different != 0 || unsorted != 0 || cases != 12 || ties < 90000 || allocation_count < 2) return 3;

  // Negative control: historical local stable_sort must be observed allocating
  // both extra buffers simultaneously; its objects must nevertheless be equal.
  auto base = records(32768);
  auto reference = base;
  std::stable_sort(reference.begin(), reference.end(), less);
  auto legacy = base;
  if (!probe::arm(base.size(), true)) return 3;
  const size_t legacy_workers = legacy_permutation(&legacy, 2);
  probe::disarm();
  const size_t legacy_peak = probe::peak.load();
  if (legacy != reference || legacy_workers != 2 || probe::arrivals.load() != 2 ||
      probe::slice_peak.load() != 2 || probe::other_large.load() != 2 || probe::deadline_missed.load() ||
      probe::live.load() != 0 || legacy_peak < 10 * base.size() || legacy_peak <= parallel_peak) return 3;

  // Every allocation performed by the corrected two-worker route is refused
  // in turn. Before permutation application the input must remain untouched.
  for (size_t ordinal = 1; ordinal <= allocation_count; ++ordinal) {
    auto input = base;
    bool caught = false;
    if (!probe::arm(base.size(), false, ordinal)) return 3;
    try {
      parallel_stable_sort_vector(&input, less, 2);
    } catch (const std::bad_alloc&) {
      caught = true;
    }
    probe::disarm();
    if (!caught || probe::calls.load() < ordinal || input != base || probe::live.load() != 0 ||
        parallel_detail::launch_active.load() != 0) ++failures;
    ++allocation_failures;
  }

  base = records(65536);
  for (int threads : {1, 2, 4}) {
    for (bool merge_only : {false, true}) {
      if (threads == 1 && merge_only) continue;
      auto input = base;
      const size_t width = base.size() / static_cast<size_t>(threads);
      std::atomic<size_t> thrown{0};
      bool caught = false;
      if (!probe::arm(base.size())) return 3;
      try {
        parallel_stable_sort_vector(&input, [&](const Record& a, const Record& b) {
          if (!merge_only || a.position / width != b.position / width) {
            ++thrown;
            throw 74;
          }
          return less(a, b);
        }, threads);
      } catch (int code) {
        caught = code == 74;
      }
      probe::disarm();
      if (!caught || thrown.load() == 0 || input != base || probe::live.load() != 0 ||
          parallel_detail::launch_active.load() != 0) ++failures;
      ++comparator_failures;
    }
  }
  auto input = base;
  parallel_detail::launch_started.store(0);
  parallel_detail::launch_fail_after = 2;
  bool launch_caught = false;
  if (!probe::arm(base.size())) return 3;
  try {
    parallel_stable_sort_vector(&input, less, 4);
  } catch (const std::system_error& error) {
    launch_caught = error.code() == std::errc::resource_unavailable_try_again;
  }
  probe::disarm();
  parallel_detail::launch_fail_after = static_cast<size_t>(-1);
  if (!launch_caught || parallel_detail::launch_started.load() != 2 || parallel_detail::launch_active.load() != 0 ||
      input != base || probe::live.load() != 0) ++failures;
  const size_t final_workers = parallel_stable_sort_vector(&input, less, 4);
  reference = base;
  std::stable_sort(reference.begin(), reference.end(), less);
  if (final_workers != 4 || input != reference) ++failures;

  std::printf("perm_residence cases=%zu ties=%zu sequential_peak=%zu parallel_peak=%zu legacy_peak=%zu "
              "legacy_simultaneous=2 allocation_failures=%zu comparator_failures=%zu launch_failures=1 failures=%zu\n",
              cases, ties, sequential_peak, parallel_peak, legacy_peak, allocation_failures, comparator_failures, failures);
  return failures == 0 && comparator_failures == 5 ? 0 : 1;
}
