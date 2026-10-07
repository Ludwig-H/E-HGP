// Compte du budget memoire et allocation des tampons (buffer.hpp). Port de src/core/buffer.hpp et buffer.cpp de la
// v10 (raccord R2, commit 865f5e6) : reserve et release de MemoryBudget, corps d'allocation de Buffer.
#include "core/buffer.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <new>

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

namespace mhgp12::detail {

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

// Classes du cache : 256 Kio * 2^(k/8), arrondies au-dessus a 4 Kio (k = 8q + r) ; kCacheSteps[r] = round(1024 * 2^(r/8)).
inline constexpr u64 kCacheMinBytes = u64{1} << 18;
inline constexpr u32 kCacheSteps = 8;
inline constexpr u32 kCacheClasses = kCacheSteps * 26;  // jusqu'a 256 Kio * 2^26 = 16 Tio
inline constexpr u32 kCacheSlots = 32;                   // blocs inactifs par classe
inline constexpr std::array<u64, kCacheSteps> kStepNumerator{1024, 1117, 1218, 1328, 1448, 1579, 1722, 1878};

u64 class_capacity(u32 k) noexcept {
  const u64 raw = ((kCacheMinBytes / 1024) * kStepNumerator[k % kCacheSteps]) << (k / kCacheSteps);  // < 2^45
  return (raw + 4095) / 4096 * 4096;
}

// Plus petite classe de capacite >= bytes ; kCacheClasses sous 256 Kio ou au-dela de la derniere classe.
u32 class_of(u64 bytes) noexcept {
  if (bytes < kCacheMinBytes) return kCacheClasses;
  u32 q = 0;
  while (q + 1 < kCacheClasses / kCacheSteps && (kCacheMinBytes << (q + 1)) <= bytes) ++q;
  for (u32 k = q * kCacheSteps; k < kCacheClasses; ++k)
    if (class_capacity(k) >= bytes) return k;
  return kCacheClasses;
}

// Blocs inactifs empoisonnes pour ASan : tout usage apres restitution reste detecte, comme sans cache.
void poison(void* block, u64 bytes) noexcept {
#if defined(__SANITIZE_ADDRESS__)
  ASAN_POISON_MEMORY_REGION(block, static_cast<std::size_t>(bytes));
#else
  static_cast<void>(block); static_cast<void>(bytes);
#endif
}
void unpoison(void* block, u64 bytes) noexcept {
#if defined(__SANITIZE_ADDRESS__)
  ASAN_UNPOISON_MEMORY_REGION(block, static_cast<std::size_t>(bytes));
#else
  static_cast<void>(block); static_cast<void>(bytes);
#endif
}

}  // namespace

struct BlockCache {
  explicit BlockCache(u64 bytes) noexcept : capacity(bytes) {}
  const u64 capacity;
  std::mutex mutex;
  u64 idle = 0, idle_peak = 0, hits = 0, misses = 0, rejected = 0;
  std::array<u32, kCacheClasses> count{};
  std::array<std::array<void*, kCacheSlots>, kCacheClasses> slots{};
};

namespace {

// Bloc d'une classe : repris du cache, sinon neuf a la capacite de la classe (reutilisable plus tard). La partie
// au-dela de bytes reste empoisonnee pour ASan.
void* cache_take(BlockCache& cache, u32 k, u64 bytes) noexcept {
  const u64 capacity = class_capacity(k);
  void* block = nullptr;
  {
    const std::lock_guard<std::mutex> lock(cache.mutex);
    if (cache.count[k] != 0) {
      block = cache.slots[k][--cache.count[k]];
      cache.idle -= capacity;
      ++cache.hits;
    } else {
      ++cache.misses;
    }
  }
  if (block != nullptr) {
    unpoison(block, bytes);
    return block;
  }
  block = ::operator new(static_cast<std::size_t>(capacity), std::nothrow);
  if (block != nullptr) poison(static_cast<char*>(block) + bytes, capacity - bytes);
  return block;
}

// Garde un bloc de la classe k si la classe a une place et si les blocs inactifs restent sous la capacite ; sinon le
// rend au systeme. Le bloc est empoisonne avant d'etre visible des autres fils.
void cache_give(BlockCache& cache, u32 k, void* block) noexcept {
  const u64 capacity = class_capacity(k);
  poison(block, capacity);
  {
    const std::lock_guard<std::mutex> lock(cache.mutex);
    if (cache.count[k] < kCacheSlots && capacity <= cache.capacity - cache.idle) {  // invariant : idle <= capacity
      cache.slots[k][cache.count[k]++] = block;
      cache.idle += capacity;
      cache.idle_peak = std::max(cache.idle_peak, cache.idle);
      return;
    }
    ++cache.rejected;
  }
  unpoison(block, capacity);
  ::operator delete(block);
}

}  // namespace

BudgetAccount::BudgetAccount(u64 limit_bytes, u64 cache_bytes)
    : limit(limit_bytes), cache(cache_bytes == 0 ? nullptr : std::make_unique<BlockCache>(cache_bytes)) {}

BudgetAccount::~BudgetAccount() {
  if (cache == nullptr) return;
  for (u32 k = 0; k < kCacheClasses; ++k)
    for (u32 i = 0; i < cache->count[k]; ++i) {
      unpoison(cache->slots[k][i], class_capacity(k));
      ::operator delete(cache->slots[k][i]);
    }
}

BlockCacheStats cache_stats(const BudgetAccount& account) noexcept {
  if (account.cache == nullptr) return {};
  const std::lock_guard<std::mutex> lock(account.cache->mutex);
  const BlockCache& c = *account.cache;
  return {c.capacity, c.idle, c.idle_peak, c.hits, c.misses, c.rejected};
}

void* buffer_acquire(BudgetAccount& account, u64 bytes) noexcept {
  if (!budget_reserve(account, bytes)) return nullptr;
  const u32 k = account.cache == nullptr ? kCacheClasses : class_of(bytes);
  void* block = k < kCacheClasses ? cache_take(*account.cache, k, bytes)
                                  : ::operator new(static_cast<std::size_t>(bytes), std::nothrow);
  if (block == nullptr) {
    account.used.fetch_sub(bytes, std::memory_order_relaxed);
    return nullptr;
  }
#ifdef MHGP12_POISON
  std::memset(block, 0xA5, static_cast<std::size_t>(bytes));
#endif
  return block;
}

void buffer_release(BudgetAccount& account, void* block, u64 bytes) noexcept {
  const u32 k = account.cache == nullptr ? kCacheClasses : class_of(bytes);
  if (k < kCacheClasses) cache_give(*account.cache, k, block);  // meme classe qu'a l'acquisition (meme taille)
  else ::operator delete(block);
  account.used.fetch_sub(bytes, std::memory_order_relaxed);
}

bool budget_reserve_only(BudgetAccount& account, u64 bytes) noexcept { return budget_reserve(account, bytes); }

void budget_release_only(BudgetAccount& account, u64 bytes) noexcept {
  account.used.fetch_sub(bytes, std::memory_order_relaxed);
}

}  // namespace mhgp12::detail
