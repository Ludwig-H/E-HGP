// Table directe optionnelle : tuples entiers du MEME domaine immobile, valeurs datees possedees.
#pragma once
#include "tower/descent.hpp"

namespace mhgp11::tower_detail {

class DescentMemo {
 public:
  DescentMemo(const DescentMemo&) = delete;
  DescentMemo& operator=(const DescentMemo&) = delete;
  DescentMemo& operator=(DescentMemo&&) = delete;
  DescentMemo(DescentMemo&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), slots_(std::move(other.slots_)) {}
  // Capacite 0 ou puissance de deux, pas de repli silencieux en cas de refus memoire.
  // Table privee a un seul appelant synchrone, jamais partagee simultanement.
  // Domaine et budget survivent a la table ; ne pas deplacer le domaine avant sa destruction.
  static Result<DescentMemo> make(const FullDomain&, u64 capacity, MemoryBudget&) noexcept;
  u64 capacity() const noexcept { return slots_.size(); }
  static constexpr u64 slot_bytes() noexcept { return sizeof(Slot); }
  bool belongs_to(const FullDomain& domain) const noexcept { return domain_ == &domain; }
  // Meme refus de partie que descend ; le domaine etranger est refuse meme a capacite zero.
  Result<DescentResult> resolve(const FullDomain&, std::span<const SiteIdx>, u32, MemoryBudget&) noexcept;

 private:
  struct Slot {
    std::array<SiteIdx, kMaxMebSites> ids{};
    num::Level initial, terminal;  // Representations exactes locales, pas seulement egalite rationnelle.
    u32 site = kNone, ball = kNone;
    u8 cardinal = 0;  // zero = vide ; pas de digest utilise comme identite.
  };
  explicit DescentMemo(const FullDomain& domain) noexcept : domain_(&domain) {}
  u64 bucket(const CellTrace&) const noexcept;
  const Slot* lookup(const CellTrace&, MemoLedger&) const noexcept;
  Result<DescentResult> publish(const CellTrace&, const num::Level&, const num::Level&,
                               const BirthSeed&, DescentLedger) noexcept;
  const FullDomain* domain_;
  Buffer<Slot> slots_;
};

// nullptr conserve exactement la reference ; aucun tri ou allocation supplementaire en mode desactive.
[[nodiscard]] Result<DescentResult> resolve_descent(const FullDomain&, std::span<const SiteIdx>, u32,
                                                  MemoryBudget&, DescentMemo* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
