#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <thread>
#include "core/buffer.hpp"

namespace {
std::atomic<void*> delayed{nullptr};
std::atomic<bool> paused{false}, release_delete{false}, started{false}, finished{false};
}
void* operator new(std::size_t n) { if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return std::malloc(n ? n : 1); }
void operator delete(void* p) noexcept {
  if (p && p == delayed.load()) {
    paused.store(true);
    while (!release_delete.load()) std::this_thread::yield();
  }
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }

int main(int argc, char** argv) {
  using namespace mhgp12;
  if (argc != 2) return 2;
  const bool small = std::strcmp(argv[1], "small") == 0;
  if (!small && std::strcmp(argv[1], "cached") != 0) return 2;
  constexpr u64 limit = 1 << 20, first_request = 300 * 1024;
  const u64 second_request = (small ? 200 : 300) * 1024;
  MemoryBudget budget(limit, limit);
  Buffer<u8> old, first, second;
  if (!old.allocate(limit, budget).ok()) return 3;
  delayed.store(old.data()); old.reset();
  const bool admitted = budget.admit(first_request + second_request).ok();
  bool first_ok = false, second_ok = false;
  std::thread a([&] { first_ok = first.allocate(first_request, budget).ok(); });
  while (!paused.load()) std::this_thread::yield();
  std::thread b([&] { started.store(true); second_ok = second.allocate(second_request, budget).ok(); finished.store(true); });
  while (!started.load()) std::this_thread::yield();
  // Le controleur libere delete sans demander cache_stats, qui prend maintenant le mutex retenu.
  const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  while (!finished.load() && std::chrono::steady_clock::now() < until) std::this_thread::yield();
  const bool finished_before_release = finished.load();
  release_delete.store(true);
  a.join(); b.join(); delayed.store(nullptr);
  const auto stats = budget.cache_stats();
  std::cout << "{\"admitted\":" << admitted << ",\"first_ok\":" << first_ok << ",\"second_ok\":" << second_ok
            << ",\"finished_before_release\":" << finished_before_release << ",\"used\":" << budget.used()
            << ",\"held\":" << stats.held << ",\"evicted\":" << stats.evicted << "}\n";
  return 0;
}
