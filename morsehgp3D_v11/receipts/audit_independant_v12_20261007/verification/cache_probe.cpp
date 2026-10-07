// Temoin d'audit externe au produit : taille physique d'un bloc vivant du cache.
#include <cstddef>
#include <iostream>
#include <new>
#include "core/buffer.hpp"

static std::size_t allocated_bytes = 0;
extern "C" void* __real__ZnwmRKSt9nothrow_t(std::size_t, const std::nothrow_t&) noexcept;
extern "C" void* __wrap__ZnwmRKSt9nothrow_t(std::size_t n, const std::nothrow_t& tag) noexcept {
  allocated_bytes = n;
  return __real__ZnwmRKSt9nothrow_t(n, tag);
}

int main() {
  constexpr mhgp11::u64 requested = (mhgp11::u64{1} << 18) + 1;
  mhgp11::MemoryBudget budget(requested, 1);
  mhgp11::Buffer<mhgp11::u8> buffer;
  if (!buffer.allocate(requested, budget).ok()) return 2;
  const auto live = budget.cache_stats();
  const auto used = budget.used();
  const auto physical = allocated_bytes;
  buffer.reset();
  const auto freed = budget.cache_stats();
  std::cout << "{\"requested\":" << requested << ",\"allocated\":" << physical
            << ",\"used\":" << used << ",\"idle\":" << live.idle
            << ",\"cache_limit\":" << live.capacity << ",\"released\":"
            << (budget.released().ok() ? "true" : "false")
            << ",\"idle_after\":" << freed.idle << ",\"rejected_after\":" << freed.rejected << "}\n";
  return physical > used + live.capacity && budget.released().ok() ? 0 : 1;
}
