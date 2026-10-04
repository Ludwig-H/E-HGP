// Certificat de provenance d'une graine basse : cellule reguliere d'ordre h-1, lecture a la naissance h.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::tower_detail {

class RegularVerticalSeeds {
 public:
  RegularVerticalSeeds(const RegularVerticalSeeds&) = delete;
  RegularVerticalSeeds& operator=(const RegularVerticalSeeds&) = delete;
  RegularVerticalSeeds& operator=(RegularVerticalSeeds&&) = delete;
  RegularVerticalSeeds(RegularVerticalSeeds&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), seeds_(std::move(other.seeds_)) {}
  static Result<RegularVerticalSeeds> make(const FullDomain& domain, MemoryBudget& budget) noexcept {
    RegularVerticalSeeds result(domain);
    const u64 count = domain.catalogue().balls();
    MHGP11_TRY(budget.admit(count * sizeof(NodeIdx)));
    MHGP11_TRY(result.seeds_.allocate(count, budget));
    for (auto& seed : result.seeds_.span()) seed = NodeIdx{kNone};
    return result;
  }
  bool belongs_to(const FullDomain& domain) const noexcept { return domain_ == &domain; }
  u64 reserved_bytes() const noexcept { return seeds_.size() * sizeof(NodeIdx); }

  Outcome remember(const OrderForest& lower, BallIdx ball, NodeIdx seed) noexcept {
    if (domain_ == nullptr || idx(ball) >= seeds_.size() || lower.order() == 0)
      return fail(Reason::tower_invariant);
    const auto& data = domain_->catalogue().balls_data()[idx(ball)];
    if (data.m != data.qmin) return {};  // Lemme limite aux cellules regulieres.
    const u64 upper_order = u64{data.p} + data.qmin;
    if (upper_order != u64{lower.order()} + 1) return fail(Reason::tower_invariant);
    if (upper_order > domain_->catalogue().kmax()) return {};  // Pas de verticale K+1 demandee.
    if (idx(seed) >= lower.births() || idx(lower.birth_nodes()[idx(seed)].rank) >= idx(data.rank) ||
        seeds_[idx(ball)] != NodeIdx{kNone}) return fail(Reason::tower_invariant);
    seeds_[idx(ball)] = seed;  // Naissance basse, JAMAIS une racine ou un top de DSU.
    return {};
  }

  Result<std::optional<NodeIdx>> find(const OrderForest& lower, const ForestNode& upper_birth) const noexcept {
    if (domain_ == nullptr || upper_birth.birth_key >= seeds_.size() || lower.order() == 0 ||
        u64{lower.order()} + 1 > domain_->catalogue().kmax()) return fail(Reason::tower_invariant);
    const auto& data = domain_->catalogue().balls_data()[upper_birth.birth_key];
    if (data.rank != upper_birth.rank) return fail(Reason::tower_invariant);
    if (data.m != data.qmin) return std::optional<NodeIdx>{};
    if (u64{data.p} + data.qmin != u64{lower.order()} + 1) return fail(Reason::tower_invariant);
    const NodeIdx seed = seeds_[upper_birth.birth_key];
    if (idx(seed) >= lower.births() || idx(lower.birth_nodes()[idx(seed)].rank) >= idx(data.rank))
      return fail(Reason::tower_invariant);  // Une entree eligible absente est un defaut, pas un repli silencieux.
    return std::optional<NodeIdx>{seed};  // L'appelant doit encore remonter a la coupe FERMEE data.rank.
  }

 private:
  explicit RegularVerticalSeeds(const FullDomain& domain) noexcept : domain_(&domain) {}
  const FullDomain* domain_;
  Buffer<NodeIdx> seeds_;
};

}  // namespace mhgp11::tower_detail
