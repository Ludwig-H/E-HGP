// Verticales naturelles : remontee FERMEE des naissances puis controle de toutes les images des enfants.
#include "tower/forest_internal.hpp"

namespace mhgp11::tower_detail {

std::optional<NodeIdx> OrderForest::birth_node(const BirthSeed& seed) const noexcept {
  if (seed.order() != order_) return std::nullopt;
  if ((order_ == 1 && (!seed.site() || seed.ball())) || (order_ != 1 && (!seed.ball() || seed.site())))
    return std::nullopt;
  const u32 key = order_ == 1 ? idx(*seed.site()) : idx(*seed.ball());
  u64 lo = 0, hi = lookup_.size();
  while (lo < hi) {
    const u64 mid = lo + (hi - lo) / 2;
    if (lookup_[mid].key < key) lo = mid + 1; else hi = mid;
  }
  return lo < lookup_.size() && lookup_[lo].key == key ? std::optional<NodeIdx>{lookup_[lo].node} : std::nullopt;
}

Result<NodeIdx> OrderForest::ancestor_closed(NodeIdx start, LevelRank level, u64& hops) const noexcept {
  if (idx(start) >= count_ || idx(nodes_[idx(start)].rank) > idx(level)) return fail(Reason::parameter_out_of_range);
  for (;;) {
    const NodeIdx parent = nodes_[idx(start)].parent;
    if (parent == NodeIdx{kNone}) return start;
    if (idx(parent) >= count_ || idx(nodes_[idx(parent)].rank) <= idx(nodes_[idx(start)].rank))
      return fail(Reason::tower_invariant);
    if (idx(nodes_[idx(parent)].rank) > idx(level)) return start;
    MHGP11_TRY(cell_add(hops, 1)); start = parent;
  }
}

struct VerticalBuilder {
  const FullDomain& domain;
  const OrderForest& lower;
  OrderForest& upper;
  MemoryBudget& budget;

  Result<NodeIdx> birth(const ForestNode& node) noexcept {
    const BallIdx ball{node.birth_key};
    const auto inner = domain.catalogue().interior(ball), shell = domain.catalogue().shell(ball);
    std::array<SiteIdx, kMaxMebSites> part{};
    const u32 size = upper.order_ - 1;
    u32 i = 0, j = 0;
    for (u32 n = 0; n < size; ++n) {
      if (i == inner.size() && j == shell.size()) return fail(Reason::tower_invariant);
      if (j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]))) part[n] = inner[i++];
      else part[n] = shell[j++];
    }
    auto down = descend(domain, {part.data(), size}, size, budget);
    if (!down.ok()) return down.outcome();
    const auto& level = domain.catalogue().levels()[idx(node.rank)];
    if (num::compare(down.value().initial_level(), level) > 0) return fail(Reason::tower_invariant);
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, 1));
    MHGP11_TRY(add_descent(upper.ledger_.descent, down.value().ledger()));
    const auto seed = lower.birth_node(down.value().seed());
    if (!seed) return fail(Reason::tower_invariant);
    return lower.ancestor_closed(*seed, node.rank, upper.ledger_.ancestor_hops);
  }

  Outcome run() noexcept {
    if (upper.order_ < 2 || lower.order_ + 1 != upper.order_ || !upper.lower_.empty())
      return fail(Reason::tower_invariant);
    // L'espace des verticales suit la capacite de noeuds retenue, mais seule count_ cases sont exposees.
    MHGP11_TRY(budget.admit(upper.nodes_.size() * sizeof(NodeIdx)));
    MHGP11_TRY(upper.lower_.allocate(upper.nodes_.size(), budget));
    for (u32 i = 0; i < upper.count_; ++i) {
      const auto& node = upper.nodes_[i];
      if (i < upper.births_) {
        auto image = birth(node);
        if (!image.ok()) return image.outcome();
        upper.lower_[i] = image.value();
      } else {
        std::optional<NodeIdx> common;
        for (NodeIdx child : upper.children(NodeIdx{i})) {
          if (idx(child) >= i) return fail(Reason::tower_invariant);
          auto image = lower.ancestor_closed(upper.lower_[idx(child)], node.rank, upper.ledger_.ancestor_hops);
          if (!image.ok()) return image.outcome();
          MHGP11_TRY(cell_add(upper.ledger_.vertical_checks, 1));
          if (common && *common != image.value()) return fail(Reason::tower_invariant);
          common = image.value();
        }
        if (!common) return fail(Reason::tower_invariant);
        upper.lower_[i] = *common;
      }
    }
    return {};
  }
};

Outcome forest_verticals(const FullDomain& domain, const OrderForest& lower, OrderForest& upper,
                         MemoryBudget& budget) noexcept {
  return VerticalBuilder{domain, lower, upper, budget}.run();
}

Result<FullTower> build_full(FullDomain&& domain, MemoryBudget& budget) noexcept {
  const Order kmax = domain.catalogue().kmax();
  if (kmax == 0 || kmax > domain.index().cloud().sites()) return fail(Reason::parameter_out_of_range);
  std::array<std::optional<OrderForest>, kMaxMebSites> orders;
  for (u32 k = 1; k <= kmax; ++k) {
    auto made = build_forest(domain, k, budget);
    if (!made.ok()) return made.outcome();
    orders[k - 1].emplace(std::move(made.value()));
    if (k > 1) MHGP11_TRY(forest_verticals(domain, *orders[k - 2], *orders[k - 1], budget));
  }
  return FullTower(std::move(domain), std::move(orders), kmax);
}

}  // namespace mhgp11::tower_detail
