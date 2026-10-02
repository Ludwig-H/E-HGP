// Tampons comptes : tout grand tableau est un Buffer<T>, reserve dans un MemoryBudget AVANT l'allocation. Le budget
// est honnete par construction : la somme des tampons vivants ne depasse jamais la limite, y compris sous
// concurrence. Port de src/core/buffer.hpp de la v10 (raccord R2, commit 865f5e6).
//
// Ce qui change par rapport a la v10 :
//   - aucun budget global : le budget par defaut du processus (default_budget) est retire, une Session porte
//     l'unique MemoryBudget et le passe explicitement (docs/ARCHITECTURE.md, paragraphe 1, regle 3) ;
//   - la limite est explicite a la construction (MemoryBudget::kUnlimited pour un budget sans limite) ;
//   - reserver et rendre sont reserves aux tampons : aucun autre code ne peut desequilibrer le compte ;
//   - le compte est partage entre le MemoryBudget et ses tampons : un tampon peut survivre a l'objet MemoryBudget
//     (un resultat rendu par une Session detruite) sans pointeur pendant ;
//   - allocate rend un Outcome (memory_budget), pas un booleen ;
//   - la taille en octets est gardee avant le produit (kMaxCount), l'alignement de T par static_assert ;
//   - l'empoisonnement (construction MHGP11_POISON) vit dans buffer.cpp : aucune unite cliente n'en depend.
//
// Un Buffer n'initialise pas ses elements. Sous MHGP11_POISON chaque octet alloue vaut 0xA5 : une lecture avant
// ecriture donne alors une valeur reconnaissable au lieu d'un zero accidentel.
#pragma once

#include <atomic>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include "core/status.hpp"

namespace mhgp11 {

namespace detail {

// Compte d'un budget. Invariant : used <= limit a tout instant ; peak = plus grande valeur prise par used.
struct BudgetAccount {
  explicit BudgetAccount(u64 limit_bytes) noexcept : limit(limit_bytes) {}
  const u64 limit;
  std::atomic<u64> used{0};
  std::atomic<u64> peak{0};
};

// Reserve `bytes` octets dans le compte, puis les alloue. nullptr si le compte refuse ou si l'allocation echoue ;
// rien ne reste alors reserve. bytes > 0.
[[nodiscard]] void* buffer_acquire(BudgetAccount& account, u64 bytes) noexcept;
// Rend un bloc obtenu par buffer_acquire avec la meme taille, et sa reservation.
void buffer_release(BudgetAccount& account, void* block, u64 bytes) noexcept;

}  // namespace detail

// Budget memoire d'une Session : limite en octets sur la somme des Buffer vivants qui y ont reserve.
// La construction alloue le compte partage et peut lever std::bad_alloc (frontiere : guarded, status.hpp).
class MemoryBudget {
 public:
  static constexpr u64 kUnlimited = std::numeric_limits<u64>::max();

  explicit MemoryBudget(u64 limit_bytes) : account_(std::make_shared<detail::BudgetAccount>(limit_bytes)) {}
  MemoryBudget(const MemoryBudget&) = delete;
  MemoryBudget& operator=(const MemoryBudget&) = delete;

  u64 limit() const noexcept { return account_->limit; }
  u64 used() const noexcept { return account_->used.load(std::memory_order_relaxed); }
  u64 peak() const noexcept { return account_->peak.load(std::memory_order_relaxed); }

 private:
  template <class T>
  friend class Buffer;
  std::shared_ptr<detail::BudgetAccount> account_;
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
  // reserve.
  [[nodiscard]] Outcome allocate(u64 n, MemoryBudget& budget) {
    reset();
    if (n == 0) return {};
    if (n > kMaxCount) return fail(Reason::memory_budget);
    void* block = detail::buffer_acquire(*budget.account_, n * sizeof(T));
    if (block == nullptr) return fail(Reason::memory_budget);
    data_ = static_cast<T*>(block);
    size_ = n;
    account_ = budget.account_;
    return {};
  }
  // Comme allocate, puis tous les octets a zero.
  [[nodiscard]] Outcome allocate_zero(u64 n, MemoryBudget& budget) {
    MHGP11_TRY(allocate(n, budget));
    if (n != 0) std::memset(static_cast<void*>(data_), 0, static_cast<std::size_t>(n * sizeof(T)));
    return {};
  }
  void reset() noexcept {
    if (data_ != nullptr) detail::buffer_release(*account_, static_cast<void*>(data_), size_ * sizeof(T));
    data_ = nullptr;
    size_ = 0;
    account_.reset();
  }
  void swap(Buffer& o) noexcept {
    std::swap(data_, o.data_);
    std::swap(size_, o.size_);
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
  std::shared_ptr<detail::BudgetAccount> account_;
};

// CSR a decalages u32. Bien forme si : off et val sont vides (aucune ligne) ; ou off.size() == rows + 1 avec
// rows <= 2^32 - 1, off[0] == 0, off croissant au sens large, off[rows] == val.size().
template <class T>
struct Csr {
  Buffer<u32> off;
  Buffer<T> val;

  u32 rows() const noexcept { return off.empty() ? 0 : static_cast<u32>(off.size() - 1); }
  // Ligne i : exige un CSR bien forme et i < rows().
  std::span<const T> row(u32 i) const noexcept {
    return {val.data() + off[i], static_cast<std::size_t>(off[i + 1] - off[i])};
  }
  bool well_formed() const noexcept {
    if (off.empty()) return val.empty();
    if (off.size() - 1 > u64{kNone}) return false;
    if (off[0] != 0) return false;
    for (u64 i = 0; i + 1 < off.size(); ++i)
      if (off[i + 1] < off[i]) return false;
    return off[off.size() - 1] == val.size();
  }
};

}  // namespace mhgp11
