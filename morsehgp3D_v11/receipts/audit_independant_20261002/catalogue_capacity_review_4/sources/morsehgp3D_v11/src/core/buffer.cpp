// Compte du budget memoire et allocation des tampons (buffer.hpp). Port de src/core/buffer.hpp et buffer.cpp de la
// v10 (raccord R2, commit 865f5e6) : reserve et release de MemoryBudget, corps d'allocation de Buffer.
#include "core/buffer.hpp"

#include <cstring>
#include <new>

namespace mhgp11::detail {

namespace {

// Reserve `bytes` dans le compte si used + bytes <= limit, de facon atomique.
// Bornes : cur <= limit est l'invariant du compte ; quand bytes <= limit, limit - bytes ne deborde pas, et le test
// cur <= limit - bytes equivaut a cur + bytes <= limit sans jamais calculer une somme qui deborderait.
bool budget_reserve(BudgetAccount& account, u64 bytes) noexcept {
  u64 cur = account.used.load(std::memory_order_relaxed);
  for (;;) {
    if (bytes > account.limit || cur > account.limit - bytes) return false;
    if (account.used.compare_exchange_weak(cur, cur + bytes, std::memory_order_relaxed)) break;
  }
  // Pic : maximum des valeurs prises par used. used ne croit qu'ici, donc le maximum des valeurs `reached` de toutes
  // les reservations reussies est le pic exact.
  const u64 reached = cur + bytes;
  u64 peak = account.peak.load(std::memory_order_relaxed);
  while (reached > peak && !account.peak.compare_exchange_weak(peak, reached, std::memory_order_relaxed)) {
  }
  return true;
}

}  // namespace

void* buffer_acquire(BudgetAccount& account, u64 bytes) noexcept {
  if (!budget_reserve(account, bytes)) return nullptr;
  void* block = ::operator new(static_cast<std::size_t>(bytes), std::nothrow);
  if (block == nullptr) {
    account.used.fetch_sub(bytes, std::memory_order_relaxed);
    return nullptr;
  }
#ifdef MHGP11_POISON
  std::memset(block, 0xA5, static_cast<std::size_t>(bytes));
#endif
  return block;
}

void buffer_release(BudgetAccount& account, void* block, u64 bytes) noexcept {
  ::operator delete(block);
  account.used.fetch_sub(bytes, std::memory_order_relaxed);
}

}  // namespace mhgp11::detail
