// Compte du budget memoire et allocation des tampons (buffer.hpp). Port de src/core/buffer.hpp et buffer.cpp de la
// v10 (raccord R2, commit 865f5e6) : reserve et release de MemoryBudget, corps d'allocation de Buffer.
#include "core/buffer.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <new>

// Empoisonnement ASan des blocs inactifs : GCC definit __SANITIZE_ADDRESS__, Clang (18 compris) ne le definit pas et
// s'interroge par __has_feature (CST-0019 : sous Clang, l'empoisonnement etait muet).
#if defined(__SANITIZE_ADDRESS__)
#define MHGP12_ASAN_POISON 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MHGP12_ASAN_POISON 1
#endif
#endif
#if defined(MHGP12_ASAN_POISON)
#include <sanitizer/asan_interface.h>
#endif

namespace mhgp12::detail {

namespace {

// Reserve `bytes` dans le compte si used + bytes <= limit, de facon atomique (compte sans cache).
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

// Strictement croissante en k (deux pas consecutifs ecartent les valeurs brutes de plus de 4 Kio) : la classe d'une
// capacite est donc la classe elle-meme, ce que buffer_release lit pour reconnaitre un bloc de classe.
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
#if defined(MHGP12_ASAN_POISON)
  ASAN_POISON_MEMORY_REGION(block, static_cast<std::size_t>(bytes));
#else
  static_cast<void>(block); static_cast<void>(bytes);
#endif
}
void unpoison(void* block, u64 bytes) noexcept {
#if defined(MHGP12_ASAN_POISON)
  ASAN_UNPOISON_MEMORY_REGION(block, static_cast<std::size_t>(bytes));
#else
  static_cast<void>(block); static_cast<void>(bytes);
#endif
}

// Ajoute x octets vivants (apres retenue dans held sous cache) et tient le pic.
void count_live(BudgetAccount& account, u64 x) noexcept {
  const u64 reached = account.used.fetch_add(x, std::memory_order_relaxed) + x;
  u64 peak = account.peak.load(std::memory_order_relaxed);
  while (reached > peak && !account.peak.compare_exchange_weak(peak, reached, std::memory_order_relaxed)) {
  }
}

}  // namespace

struct BlockCache {
  explicit BlockCache(u64 bytes) noexcept : capacity(bytes) {}
  const u64 capacity;
  std::mutex mutex;
  u64 idle = 0, idle_peak = 0, hits = 0, misses = 0, rejected = 0, evicted = 0, exact = 0;
  std::array<u32, kCacheClasses> count{};
  std::array<std::array<void*, kCacheSlots>, kCacheClasses> slots{};
};

namespace {

// Transitions sous le verrou du cache (CST-0007, contre-lecture du 7 octobre, course d'eviction) : reprise d'un
// inactif, restitution d'un bloc de classe et eviction changent used, idle et held ensemble, restitution au systeme
// comprise. Aucune de ces transitions n'est donc en cours hors du verrou : un fil qui le tient et trouve le cache vide
// lit dans held tout ce qui reste a rendre. Ordre des verrous : celui du cache, puis ceux de l'allocateur ; aucun
// chemin ne prend le premier sous les seconds (l'allocateur n'appelle jamais ce fichier).

// Bloc inactif de la classe k repris par un tampon vivant (inactif -> vivant, held inchange), sinon nullptr (un echec
// compte un defaut de cache).
void* cache_pop(BudgetAccount& account, u32 k) noexcept {
  BlockCache& cache = *account.cache;
  const std::lock_guard<std::mutex> lock(cache.mutex);
  if (cache.count[k] == 0) {
    ++cache.misses;
    return nullptr;
  }
  cache.idle -= class_capacity(k);
  ++cache.hits;
  count_live(account, class_capacity(k));
  return cache.slots[k][--cache.count[k]];
}

// Restitution d'un bloc vivant de la classe k : garde si la classe a une place et si les inactifs restent sous la
// capacite du cache, sinon rendu au systeme. Le bloc est empoisonne avant d'etre visible des autres fils.
void release_class(BudgetAccount& account, u32 k, void* block) noexcept {
  BlockCache& cache = *account.cache;
  const u64 capacity = class_capacity(k);
  poison(block, capacity);
  const std::lock_guard<std::mutex> lock(cache.mutex);
  account.used.fetch_sub(capacity, std::memory_order_relaxed);
  if (cache.count[k] < kCacheSlots && capacity <= cache.capacity - cache.idle) {  // invariant : idle <= capacity
    cache.slots[k][cache.count[k]++] = block;
    cache.idle += capacity;
    cache.idle_peak = std::max(cache.idle_peak, cache.idle);
    return;
  }
  ++cache.rejected;
  unpoison(block, capacity);
  ::operator delete(block);
  account.held.fetch_sub(capacity, std::memory_order_relaxed);
}

// Verrou du cache tenu : rend au systeme le plus grand bloc inactif et le retire de held ; false si le cache est vide.
bool evict_locked(BudgetAccount& account, BlockCache& cache) noexcept {
  for (u32 k = kCacheClasses; k-- > 0;) {
    if (cache.count[k] == 0) continue;
    const u64 capacity = class_capacity(k);
    void* block = cache.slots[k][--cache.count[k]];
    cache.idle -= capacity;
    ++cache.evicted;
    unpoison(block, capacity);
    ::operator delete(block);
    account.held.fetch_sub(capacity, std::memory_order_relaxed);
    return true;
  }
  return false;
}

// Sous cache : retient x octets physiques dans held (held <= limit). Sans place, rend des blocs inactifs au systeme
// sous le verrou du cache, et ne refuse qu'apres avoir relu held sous ce verrou, cache vide : une eviction ou la
// restitution d'un bloc de classe en cours sur un autre fil ne fait jamais refuser une reservation qui tiendra a sa
// fin. Un bloc exact ou une reservation sans bloc en cours de restitution compte encore dans held, comme sans cache :
// il appartient a l'etage admis ou a ce qui etait reserve avant lui, et l'engagement de MemoryBudget::admit tient.
bool hold(BudgetAccount& account, u64 x) noexcept {
  if (x > account.limit) return false;
  for (;;) {
    u64 cur = account.held.load(std::memory_order_relaxed);
    if (cur <= account.limit - x) {
      if (account.held.compare_exchange_weak(cur, cur + x, std::memory_order_relaxed)) return true;
      continue;
    }
    const std::lock_guard<std::mutex> lock(account.cache->mutex);
    if (evict_locked(account, *account.cache)) continue;
    if (account.held.load(std::memory_order_relaxed) > account.limit - x) return false;
  }
}

// Bloc neuf de `x` octets retenus et comptes vivants ; nullptr (rien ne reste retenu) si l'allocation echoue.
void* fresh_block(BudgetAccount& account, u64 x) noexcept {
  count_live(account, x);
  void* block = ::operator new(static_cast<std::size_t>(x), std::nothrow);
  if (block == nullptr) {
    account.used.fetch_sub(x, std::memory_order_relaxed);
    account.held.fetch_sub(x, std::memory_order_relaxed);
  }
  return block;
}

// Acquisition sous cache : bloc inactif de la classe, sinon bloc neuf de la classe, sinon (la classe ne tient pas meme
// cache vide) bloc a la taille exacte, jamais garde. La partie au-dela de bytes reste empoisonnee pour ASan.
void* cached_acquire(BudgetAccount& account, u64 bytes, u64& reserved) noexcept {
  const u32 k = class_of(bytes);
  if (k < kCacheClasses) {
    const u64 capacity = class_capacity(k);
    if (void* block = cache_pop(account, k); block != nullptr) {
      unpoison(block, bytes);
      reserved = capacity;
      return block;
    }
    if (hold(account, capacity)) {
      void* block = fresh_block(account, capacity);
      if (block != nullptr) {
        poison(static_cast<char*>(block) + bytes, capacity - bytes);
        reserved = capacity;
      }
      return block;
    }
    const std::lock_guard<std::mutex> lock(account.cache->mutex);
    ++account.cache->exact;
  }
  if (!hold(account, bytes)) return nullptr;
  reserved = bytes;
  return fresh_block(account, bytes);
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
  return {c.capacity, c.idle, c.idle_peak, c.hits, c.misses, c.rejected, c.evicted, c.exact,
          account.held.load(std::memory_order_relaxed)};
}

void* buffer_acquire(BudgetAccount& account, u64 bytes, u64& reserved) noexcept {
  reserved = 0;
  void* block = nullptr;
  if (account.cache != nullptr) {
    block = cached_acquire(account, bytes, reserved);
  } else if (budget_reserve(account, bytes)) {
    block = ::operator new(static_cast<std::size_t>(bytes), std::nothrow);
    if (block == nullptr) account.used.fetch_sub(bytes, std::memory_order_relaxed);
    else reserved = bytes;
  }
  if (block == nullptr) return nullptr;
#ifdef MHGP12_POISON
  std::memset(block, 0xA5, static_cast<std::size_t>(bytes));
#endif
  return block;
}

// Ordre des comptes : un bloc n'est retire de used qu'apres avoir quitte le vivant, et de held qu'apres sa
// restitution au systeme ; used + inactifs <= held <= limit a tout instant.
void buffer_release(BudgetAccount& account, void* block, u64 reserved) noexcept {
  if (account.cache == nullptr) {
    ::operator delete(block);
    account.used.fetch_sub(reserved, std::memory_order_relaxed);
    return;
  }
  // Un bloc de classe a pour taille reservee la capacite de sa classe ; un bloc exact n'est jamais garde.
  const u32 k = class_of(reserved);
  if (k < kCacheClasses && class_capacity(k) == reserved) {
    release_class(account, k, block);
    return;
  }
  ::operator delete(block);
  account.used.fetch_sub(reserved, std::memory_order_relaxed);
  account.held.fetch_sub(reserved, std::memory_order_relaxed);
}

bool budget_reserve_only(BudgetAccount& account, u64 bytes) noexcept {
  if (account.cache == nullptr) return budget_reserve(account, bytes);
  if (!hold(account, bytes)) return false;
  count_live(account, bytes);
  return true;
}

void budget_release_only(BudgetAccount& account, u64 bytes) noexcept {
  account.used.fetch_sub(bytes, std::memory_order_relaxed);
  if (account.cache != nullptr) account.held.fetch_sub(bytes, std::memory_order_relaxed);
}

}  // namespace mhgp12::detail
