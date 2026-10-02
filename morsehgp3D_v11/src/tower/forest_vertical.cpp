// Verticales naturelles : remontee FERMEE des naissances puis controle de toutes les images des enfants.
#include "tower/forest_internal.hpp"
#include "tower/forest_ancestor_sweep.hpp"

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
  ClosedAncestorSweep& sweep;
  DescentMemo* memo;

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
    auto down = resolve_descent(domain, {part.data(), size}, size, budget, memo);
    if (!down.ok()) return down.outcome();
    const auto& level = domain.catalogue().levels()[idx(node.rank)];
    if (num::compare(down.value().initial_level(), level) > 0) return fail(Reason::tower_invariant);
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, 1));
    MHGP11_TRY(add_descent(upper.ledger_.descent, down.value().ledger()));
    const auto seed = lower.birth_node(down.value().seed());
    if (!seed) return fail(Reason::tower_invariant);
    return sweep.query(*seed, upper.ledger_);
  }

  Outcome run() noexcept {
    if (upper.order_ < 2 || lower.order_ + 1 != upper.order_ || !upper.lower_.empty())
      return fail(Reason::tower_invariant);
    // L'espace des verticales suit la capacite de noeuds retenue, mais seule count_ cases sont exposees.
    MHGP11_TRY(budget.admit(upper.nodes_.size() * sizeof(NodeIdx)));
    MHGP11_TRY(upper.lower_.allocate(upper.nodes_.size(), budget));
    u32 birth_cursor = 0, merge = upper.births_;
    while (birth_cursor < upper.births_ || merge < upper.count_) {
      // Les naissances et les fusions sont deux flux deja tries par niveau ; leur melange ne l'est pas.
      const bool take_birth = birth_cursor < upper.births_ && (merge == upper.count_ ||
          idx(upper.nodes_[birth_cursor].rank) <= idx(upper.nodes_[merge].rank));
      const u32 i = take_birth ? birth_cursor++ : merge++;
      const auto& node = upper.nodes_[i];
      MHGP11_TRY(sweep.advance(node.rank, upper.ledger_));
      if (i < upper.births_) {
        auto image = birth(node);
        if (!image.ok()) return image.outcome();
        upper.lower_[i] = image.value();
      } else {
        std::optional<NodeIdx> common;
        for (NodeIdx child : upper.children(NodeIdx{i})) {
          if (idx(child) >= i || idx(upper.nodes_[idx(child)].rank) >= idx(node.rank))
            return fail(Reason::tower_invariant);
          auto image = sweep.query(upper.lower_[idx(child)], upper.ledger_);
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
                         MemoryBudget& budget, DescentMemo* memo) noexcept {
  auto sweep = ClosedAncestorSweep::make(lower, budget);
  if (!sweep.ok()) return sweep.outcome();
  return VerticalBuilder{domain, lower, upper, budget, sweep.value(), memo}.run();
}

Result<FullTower> build_full(FullDomain&& domain, MemoryBudget& budget, FullTimings* timings,
                            FullParams params) noexcept {
  const Order kmax = domain.catalogue().kmax();
  if (kmax == 0 || kmax > domain.index().cloud().sites()) return fail(Reason::parameter_out_of_range);
  FullTimings draft;
  draft.memo_slot_bytes = DescentMemo::slot_bytes();
  std::array<std::optional<OrderForest>, kMaxMebSites> orders;
  {
    auto memo = DescentMemo::make(domain, params.memo_capacity, budget);
    if (!memo.ok()) return memo.outcome();
    DescentMemo* context = params.memo_capacity == 0 ? nullptr : &memo.value();
    draft.memo_capacity = params.memo_capacity;
    draft.memo_reserved_bytes = params.memo_capacity * DescentMemo::slot_bytes();
    for (u32 k = 1; k <= kmax; ++k) {
      auto made = build_forest(domain, k, budget, timings == nullptr ? nullptr : &draft.orders[k - 1], context);
      if (!made.ok()) return made.outcome();
      orders[k - 1].emplace(std::move(made.value()));
      if (k > 1) {
        std::optional<Stopwatch> clock;
        if (timings != nullptr) clock.emplace();
        MHGP11_TRY(forest_verticals(domain, *orders[k - 2], *orders[k - 1], budget, context));
        if (clock) draft.orders[k - 1].verticals_ns = clock->nanoseconds();
      }
    }
  }  // Rend la table et son emprunt AVANT le deplacement du domaine.
  if (timings != nullptr) *timings = draft;
  return FullTower(std::move(domain), std::move(orders), kmax);
}

}  // namespace mhgp11::tower_detail
