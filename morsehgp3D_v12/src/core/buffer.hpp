// Tampons comptes : tout grand tableau est un Buffer<T>, reserve dans un MemoryBudget AVANT l'allocation. Le budget
// est honnete par construction : la somme des tampons vivants ne depasse jamais la limite, y compris sous
// concurrence. Port de src/core/buffer.hpp de la v10 (raccord R2, commit 865f5e6).
//
// Ce qui change par rapport a la v10 :
//   - aucun budget global : le budget par defaut du processus (default_budget) est retire, une Session porte
//     l'unique MemoryBudget et le passe explicitement (ARCHITECTURE.md de la v11, paragraphe 1, regle 3) ;
//   - la limite est explicite a la construction (MemoryBudget::kUnlimited pour un budget sans limite) ;
//   - reserver et rendre sont reserves aux tampons : aucun autre code ne peut desequilibrer le compte ;
//   - contrat de duree de vie (ARCHITECTURE.md de la v11, paragraphe 7.1) : la Session possede le budget et survit a
//     tous les resultats qu'elle a servis ; un budget non revenu a zero a sa destruction est une violation
//     d'invariant, que released() rend. Le compte est partage entre le MemoryBudget et ses tampons pour que cette
//     violation reste un refus observable et jamais un comportement indefini : un tampon qui survivrait a l'objet
//     MemoryBudget rend ses octets a un compte encore vivant ;
//   - allocate rend un Outcome (memory_budget), pas un booleen ; allocate_zero, sans emploi dans la v10, est retire ;
//   - MemoryBudget::admit : admission d'un etage par formule avant toute allocation ; restart_peak : pic par etage ;
//   - la taille en octets est gardee avant le produit (kMaxCount), l'alignement de T par static_assert ;
//   - les decalages d'un Csr sont des u64 (paragraphe 7.3 de l'architecture) ;
//   - l'empoisonnement (construction MHGP12_POISON) vit dans buffer.cpp : aucune unite cliente n'en depend.
//
// Un Buffer n'initialise pas ses elements. Sous MHGP12_POISON chaque octet alloue vaut 0xA5 : une lecture avant
// ecriture donne alors une valeur reconnaissable au lieu d'un zero accidentel.
//
// Concurrence : le compte d'un budget est sur entre fils (atomiques) ; un Buffer, lui, appartient a un fil a la fois.
// Portee du compte : les Buffer seulement. L'audit de la v10 mesure que son budget ne voyait que 0,26 % du pic
// resident apres le catalogue et 29 % apres la tour, le reste vivant dans des std::vector. Ce qui rend le compte
// honnete est la regle de l'architecture (paragraphes 1 et 7.1) : tout tableau dont la taille depend de l'entree est
// un Buffer ; ce fichier ne peut pas la faire respecter seul.
#pragma once

#include <atomic>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include "core/status.hpp"

namespace mhgp12 {

namespace detail {

// Cache de blocs d'un compte (buffer.cpp) et ses compteurs, diagnostic seulement. held : octets physiques tenus par
// le compte (blocs vivants a leur taille physique et blocs inactifs) ; evicted : blocs inactifs rendus au systeme pour
// faire place sous la limite ; exact : reservations servies a leur taille exacte, la classe ne tenant pas.
struct BlockCache;
struct BlockCacheStats {
  u64 capacity = 0, idle = 0, idle_peak = 0, hits = 0, misses = 0, rejected = 0, evicted = 0, exact = 0, held = 0;
};

// Compte d'un budget. Invariant : used <= limit a tout instant ; peak = plus grande valeur prise par used, donc le
// maximum des reservations, y compris celle d'une allocation que le systeme refuse ensuite. Ce n'est pas la memoire
// residente du processus. cache : nul sauf capacite de cache non nulle ; ses blocs inactifs sont rendus avec le compte.
// Avec cache (CST-0007, CST-0019) : used porte la taille PHYSIQUE des blocs vivants (classe arrondie), held la somme
// des blocs vivants et inactifs, et l'invariant devient used <= held <= limit ; sans cache, held reste nul.
struct BudgetAccount {
  BudgetAccount(u64 limit_bytes, u64 cache_bytes);
  ~BudgetAccount();
  BudgetAccount(const BudgetAccount&) = delete;
  BudgetAccount& operator=(const BudgetAccount&) = delete;
  const u64 limit;
  std::atomic<u64> used{0};
  std::atomic<u64> peak{0};
  std::atomic<u64> held{0};
  std::unique_ptr<BlockCache> cache;
};
[[nodiscard]] BlockCacheStats cache_stats(const BudgetAccount& account) noexcept;

// Reserve au moins `bytes` octets dans le compte, puis les alloue ; `reserved` recoit les octets reserves (taille
// physique du bloc : `bytes`, ou la capacite de sa classe sous cache). nullptr si le compte refuse ou si l'allocation
// echoue ; rien ne reste alors reserve. bytes > 0.
[[nodiscard]] void* buffer_acquire(BudgetAccount& account, u64 bytes, u64& reserved) noexcept;
// Rend un bloc obtenu par buffer_acquire avec les octets `reserved` qu'il a rendus, et sa reservation.
void buffer_release(BudgetAccount& account, void* block, u64 reserved) noexcept;
// Reservation seule, sans bloc (BudgetReservation) : memes regles que buffer_acquire ; et son retour.
[[nodiscard]] bool budget_reserve_only(BudgetAccount& account, u64 bytes) noexcept;
void budget_release_only(BudgetAccount& account, u64 bytes) noexcept;

}  // namespace detail

// Budget memoire d'une Session : limite en octets sur la somme des Buffer vivants qui y ont reserve.
// La construction alloue le compte partage et peut lever std::bad_alloc (frontiere : guarded, status.hpp).
//
// Cache de blocs (7 octobre 2026, recu retention_tas) : avec cache_bytes > 0, les blocs d'au moins 256 Kio rendus par
// les Buffer de ce budget sont gardes, jusqu'a cache_bytes octets inactifs, et repris par la reservation suivante de
// la meme classe de taille (2^(1/8) par pas). Ils ne sont plus rendus au systeme (munmap, sur un seul fil) puis
// refaits page par page a la passe suivante. Les blocs inactifs sont bornes par cache_bytes et rendus au systeme avec
// le compte. Aucune decision ni aucune sortie n'en depend. Sans cache (defaut), comportement inchange.
// Le cache est compte comme une reserve (CST-0007, CST-0019, 7 octobre) : un bloc vivant de classe compte sa taille
// physique (used, peak), les blocs inactifs comptent sous la meme limite (held = vivants + inactifs <= limit), et une
// reservation qui ne tient pas rend d'abord des blocs inactifs au systeme ; si la classe ne tient pas meme cache vide,
// le bloc est pris a sa taille exacte et ne sera pas garde. admit prend une marge de 1/8 (la classe arrondit au plus
// de 2^(1/8) et d'une page de 4 Kio, soit moins de 10,7 % au-dessus de 256 Kio) : son engagement tient sous cache.
class MemoryBudget {
 public:
  static constexpr u64 kUnlimited = std::numeric_limits<u64>::max();

  explicit MemoryBudget(u64 limit_bytes, u64 cache_bytes = 0)
      : account_(std::make_shared<detail::BudgetAccount>(limit_bytes, cache_bytes)) {}
  MemoryBudget(const MemoryBudget&) = delete;
  MemoryBudget& operator=(const MemoryBudget&) = delete;

  u64 limit() const noexcept { return account_->limit; }
  u64 used() const noexcept { return account_->used.load(std::memory_order_relaxed); }
  u64 peak() const noexcept { return account_->peak.load(std::memory_order_relaxed); }

  // Admission d'un etage par formule, AVANT d'allouer : refus memory_budget si `bytes` octets de plus ne tiennent pas
  // sous la limite. Ce n'est PAS une reservation : rien n'est retenu. Le contrat est celui d'une Session, qui n'a
  // qu'un pilote : le fil qui pilote un etage appelle admit avant ses taches paralleles, avec la somme des tampons
  // que l'etage allouera, et rien d'autre n'alloue dans ce budget pendant l'etage. Alors aucune allocation de
  // l'etage n'est refusee par le budget, quel que soit le nombre de fils ou leur entrelacement, et le refus, s'il a
  // lieu, a lieu ici, avant tout calcul. Si la formule sous-estime, une allocation ulterieure refuse (memory_budget).
  // Borne : used <= limit (invariant du compte), donc limit - used ne deborde pas.
  // Sous cache, chaque tampon peut couter sa classe : la formule recoit une marge de bytes / 8.
  [[nodiscard]] Outcome admit(u64 bytes) const noexcept {
    const u64 room = account_->limit - used();
    if (bytes > room) return fail(Reason::memory_budget);
    if (account_->cache != nullptr && bytes / 8 > room - bytes) return fail(Reason::memory_budget);
    return {};
  }

  // Pic par etage : remet le pic a la quantite en usage et rend l'ancien pic. Le pilote l'appelle entre deux etages,
  // quand aucune tache n'alloue ; le pic lu a la fin de l'etage est alors celui de l'etage, mesure et non estime.
  u64 restart_peak() noexcept { return account_->peak.exchange(used(), std::memory_order_relaxed); }

  // Fin de vie du budget : refus budget_not_released (invariant viole) si des octets restent reserves. La Session
  // l'appelle avant sa destruction ; les portes l'appellent a la fin de chaque operation.
  [[nodiscard]] Outcome released() const noexcept {
    return used() == 0 ? Outcome{} : fail(Reason::budget_not_released);
  }

  // Compteurs du cache de blocs (capacite nulle sans cache) : diagnostic, aucune decision ne les lit.
  [[nodiscard]] detail::BlockCacheStats cache_stats() const noexcept { return detail::cache_stats(*account_); }

 private:
  template <class T>
  friend class Buffer;
  friend class BudgetReservation;
  std::shared_ptr<detail::BudgetAccount> account_;
};

// Octets reserves dans un budget sans allocation hote, pour une memoire que le processus ne tient pas par operator
// new (tableaux du GPU, contrat R7) : comptes avec les Buffer du meme budget, donc dans leurs coexistences et le pic,
// et rendus a la destruction ou par reset. Memes refus que Buffer::allocate (memory_budget, rien de reserve).
class BudgetReservation {
 public:
  BudgetReservation() = default;
  BudgetReservation(const BudgetReservation&) = delete;
  BudgetReservation& operator=(const BudgetReservation&) = delete;
  ~BudgetReservation() { reset(); }

  // Rend d'abord la reservation precedente ; zero octet ne reserve rien.
  [[nodiscard]] Outcome reserve(u64 bytes, MemoryBudget& budget) noexcept {
    reset();
    if (bytes == 0) return {};
    if (!detail::budget_reserve_only(*budget.account_, bytes)) return fail(Reason::memory_budget);
    account_ = budget.account_;
    bytes_ = bytes;
    return {};
  }
  void reset() noexcept {
    if (bytes_ != 0) detail::budget_release_only(*account_, bytes_);
    bytes_ = 0;
    account_.reset();
  }
  u64 bytes() const noexcept { return bytes_; }

 private:
  std::shared_ptr<detail::BudgetAccount> account_;
  u64 bytes_ = 0;
};

template <class T>
class Buffer {
  // Objets a duree de vie implicite : le stockage rendu par operator new les contient sans construction.
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                "Buffer<T> : types triviaux seulement");
  static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                "Buffer<T> : alignement de T au plus celui de operator new");

 public:
  // Plus grand nombre d'elements alloue : n <= kMaxCount entraine n * sizeof(T) <= 2^64 - 1, sans debordement.
  static constexpr u64 kMaxCount = std::numeric_limits<u64>::max() / sizeof(T);

  Buffer() = default;
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& o) noexcept { swap(o); }
  Buffer& operator=(Buffer&& o) noexcept {
    if (this != &o) {
      reset();
      swap(o);
    }
    return *this;
  }
  ~Buffer() { reset(); }

  // Alloue n elements non initialises dans `budget`. Le contenu precedent est d'abord libere. Refus memory_budget si
  // n depasse kMaxCount, si le budget refuse ou si l'allocation echoue ; le tampon est alors vide et rien n'est
  // reserve. Pour remplacer un resultat sans le perdre sur un refus : allouer un tampon temporaire, puis swap.
  [[nodiscard]] Outcome allocate(u64 n, MemoryBudget& budget) {
    reset();
    if (n == 0) return {};
    if (n > kMaxCount) return fail(Reason::memory_budget);
    u64 reserved = 0;
    void* block = detail::buffer_acquire(*budget.account_, n * sizeof(T), reserved);
    if (block == nullptr) return fail(Reason::memory_budget);
    data_ = static_cast<T*>(block);
    size_ = n;
    reserved_ = reserved;
    account_ = budget.account_;
    return {};
  }
  void reset() noexcept {
    if (data_ != nullptr) detail::buffer_release(*account_, static_cast<void*>(data_), reserved_);
    data_ = nullptr;
    size_ = 0;
    reserved_ = 0;
    account_.reset();
  }
  void swap(Buffer& o) noexcept {
    std::swap(data_, o.data_);
    std::swap(size_, o.size_);
    std::swap(reserved_, o.reserved_);
    account_.swap(o.account_);
  }

  u64 size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  T* data() noexcept { return data_; }
  const T* data() const noexcept { return data_; }
  // Acces sans controle de borne : i < size().
  T& operator[](u64 i) noexcept { return data_[i]; }
  const T& operator[](u64 i) const noexcept { return data_[i]; }
  T* begin() noexcept { return data_; }
  T* end() noexcept { return data_ + size_; }
  const T* begin() const noexcept { return data_; }
  const T* end() const noexcept { return data_ + size_; }
  std::span<T> span() noexcept { return {data_, static_cast<std::size_t>(size_)}; }
  std::span<const T> span() const noexcept { return {data_, static_cast<std::size_t>(size_)}; }

 private:
  T* data_ = nullptr;
  u64 size_ = 0;
  u64 reserved_ = 0;  // octets reserves dans le compte (taille physique du bloc)
  std::shared_ptr<detail::BudgetAccount> account_;
};

// CSR a decalages u64 : un decalage de tableau est un u64, jamais tronque (ARCHITECTURE.md de la v11, paragraphe 7.3 ;
// la v10 les rangeait en u32). Bien forme si : off et val sont vides (aucune ligne) ; ou off.size() == rows + 1,
// off[0] == 0, off croissant au sens large, off[rows] == val.size().
template <class T>
struct Csr {
  Buffer<u64> off;
  Buffer<T> val;

  u64 rows() const noexcept { return off.empty() ? 0 : off.size() - 1; }
  // Ligne i : exige un CSR bien forme et i < rows().
  std::span<const T> row(u64 i) const noexcept {
    return {val.data() + off[i], static_cast<std::size_t>(off[i + 1] - off[i])};
  }
  bool well_formed() const noexcept {
    if (off.empty()) return val.empty();
    if (off[0] != 0) return false;
    for (u64 i = 0; i + 1 < off.size(); ++i)
      if (off[i + 1] < off[i]) return false;
    return off[off.size() - 1] == val.size();
  }
};

}  // namespace mhgp12
