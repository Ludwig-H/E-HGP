// Temoin hors produit : eviction lente, deux allocations admises, aucun malloc refuse.
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
#include "core/buffer.hpp"

namespace {
std::atomic<void*> delay_pointer{nullptr};
std::atomic<bool> paused{false}, resume_delete{false};
}

void* operator new(std::size_t bytes) {
  if (void* p = std::malloc(bytes == 0 ? 1 : bytes)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t bytes, const std::nothrow_t&) noexcept {
  return std::malloc(bytes == 0 ? 1 : bytes);
}
void operator delete(void* p) noexcept {
  if (p != nullptr && p == delay_pointer.load()) {
    paused.store(true);
    while (!resume_delete.load()) std::this_thread::yield();
  }
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }

int main() {
  using namespace mhgp12;
  constexpr u64 limit = 1024 * 1024, request = 300 * 1024;
  MemoryBudget budget(limit, limit);
  Buffer<u8> cached;
  if (!cached.allocate(limit, budget).ok()) return 2;
  delay_pointer.store(cached.data());
  cached.reset();
  const bool admitted = budget.admit(2 * request).ok();
  Buffer<u8> first, second;
  bool first_ok = false;
  std::thread worker([&] { first_ok = first.allocate(request, budget).ok(); });
  while (!paused.load()) std::this_thread::yield();
  const auto during = budget.cache_stats();
  const auto during_used = budget.used();
  const bool second_ok_during_eviction = second.allocate(request, budget).ok();
  resume_delete.store(true);
  worker.join();
  delay_pointer.store(nullptr);
  const bool second_ok_after_eviction = second.allocate(request, budget).ok();
  const auto after = budget.cache_stats();
  std::cout << "{\"admitted\":" << admitted << ",\"first_ok\":" << first_ok
            << ",\"second_ok_during_eviction\":" << second_ok_during_eviction
            << ",\"second_ok_after_eviction\":" << second_ok_after_eviction
            << ",\"during_used\":" << during_used << ",\"during_idle\":" << during.idle
            << ",\"during_held\":" << during.held << ",\"final_used\":" << budget.used()
            << ",\"final_held\":" << after.held << ",\"limit\":" << limit << "}\n";
  return admitted && first_ok && !second_ok_during_eviction && second_ok_after_eviction ? 0 : 1;
}
