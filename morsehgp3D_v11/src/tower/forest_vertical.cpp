// Verticales naturelles : remontee FERMEE des naissances puis controle de toutes les images des enfants.
#include "tower/forest_internal.hpp"
#include "tower/forest_ancestor_sweep.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/forest_vertical_seed.hpp"

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
  ForestParallel* parallel;
  OrderTimings* times;

  Result<NodeIdx> birth(const ForestNode& node) noexcept {
    auto seed = vertical_seed(domain, lower, upper.order_, node, budget, memo, upper.ledger_.descent);
    if (!seed.ok()) return seed.outcome();
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, 1));
    return sweep.query(seed.value(), upper.ledger_);
  }

  Outcome run() noexcept {
    if (upper.order_ < 2 || lower.order_ + 1 != upper.order_ || !upper.lower_.empty())
      return fail(Reason::tower_invariant);
    // L'espace des verticales suit la capacite de noeuds retenue, mais seule count_ cases sont exposees.
    MHGP11_TRY(budget.admit(upper.nodes_.size() * sizeof(NodeIdx)));
    MHGP11_TRY(upper.lower_.allocate(upper.nodes_.size(), budget));
    if (parallel != nullptr) MHGP11_TRY(parallel->verticals(lower, upper, times));
    std::optional<Stopwatch> clock;
    if (parallel != nullptr && times != nullptr) clock.emplace();
    u32 birth_cursor = 0, merge = upper.births_;
    while (birth_cursor < upper.births_ || merge < upper.count_) {
      // Les naissances et les fusions sont deux flux deja tries par niveau ; leur melange ne l'est pas.
      const bool take_birth = birth_cursor < upper.births_ && (merge == upper.count_ ||
          idx(upper.nodes_[birth_cursor].rank) <= idx(upper.nodes_[merge].rank));
      const u32 i = take_birth ? birth_cursor++ : merge++;
      const auto& node = upper.nodes_[i];
      MHGP11_TRY(sweep.advance(node.rank, upper.ledger_));
      if (i < upper.births_) {
        auto image = parallel == nullptr ? birth(node) : sweep.query(upper.lower_[i], upper.ledger_);
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
    if (clock) times->vertical_sweep_ns = clock->nanoseconds();
    return {};
  }
};

Outcome forest_verticals(const FullDomain& domain, const OrderForest& lower, OrderForest& upper,
                         MemoryBudget& budget, DescentMemo* memo, ForestParallel* parallel,
                         OrderTimings* times) noexcept {
  if (parallel != nullptr && !parallel->belongs_to(domain, budget)) return fail(Reason::parameter_out_of_range);
  auto sweep = ClosedAncestorSweep::make(lower, budget);
  if (!sweep.ok()) return sweep.outcome();
  return VerticalBuilder{domain, lower, upper, budget, sweep.value(), memo, parallel, times}.run();
}

Result<FullTower> build_full(FullDomain&& domain, MemoryBudget& budget, FullTimings* timings,
                            FullParams params, sched::Pool* pool) noexcept {
  const Order kmax = domain.catalogue().kmax();
  if (kmax == 0 || kmax > domain.index().cloud().sites()) return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(ForestParallel::validate(params, pool));
  FullTimings draft;
  draft.memo_slot_bytes = DescentMemo::slot_bytes();
  std::array<std::optional<OrderForest>, kMaxMebSites> orders;
  {
    const u32 workspace_count = !params.reuse_census_workspace ? 0 : params.regular_batch_capacity == 0 ? 1 :
        std::min({pool->size(), params.descent_lanes, params.regular_batch_capacity});
    auto scratch = CensusSlots::make(domain, workspace_count, budget);
    if (!scratch.ok()) return scratch.outcome();
    draft.census_workspaces = workspace_count;
    draft.census_workspace_reserved_bytes = scratch.value().reserved_bytes();
    auto memo = DescentMemo::make(domain, params.memo_capacity, budget, scratch.value().get(0));
    if (!memo.ok()) return memo.outcome();
    DescentMemo* context = params.memo_capacity == 0 && workspace_count == 0 ? nullptr : &memo.value();
    draft.memo_capacity = params.memo_capacity;
    draft.memo_reserved_bytes = params.memo_capacity * DescentMemo::slot_bytes();
    std::optional<ForestParallel> parallel;
    if (params.regular_batch_capacity != 0) {
      auto made = ForestParallel::make(domain, params, budget, *pool, workspace_count == 0 ? nullptr : &scratch.value());
      if (!made.ok()) return made.outcome();
      parallel.emplace(std::move(made.value()));
      draft.regular_batch_capacity = params.regular_batch_capacity;
      draft.descent_lanes = params.descent_lanes;
      draft.lane_memo_capacity = params.lane_memo_capacity;
      draft.lane_memo_reserved_bytes = parallel->memo_bytes();
      draft.parallel_verticals = params.parallel_verticals;
    }
    for (u32 k = 1; k <= kmax; ++k) {
      auto made = build_forest(domain, k, budget, timings == nullptr ? nullptr : &draft.orders[k - 1], context,
                               parallel ? &*parallel : nullptr);
      if (!made.ok()) return made.outcome();
      orders[k - 1].emplace(std::move(made.value()));
      if (k > 1) {
        std::optional<Stopwatch> clock;
        if (timings != nullptr) clock.emplace();
        MHGP11_TRY(forest_verticals(domain, *orders[k - 2], *orders[k - 1], budget, context,
                                   params.parallel_verticals ? &*parallel : nullptr,
                                   timings == nullptr ? nullptr : &draft.orders[k - 1]));
        if (clock) draft.orders[k - 1].verticals_ns = clock->nanoseconds();
      }
    }
  }  // Rend les memos, le contexte Pool et les workspaces AVANT le deplacement du domaine.
  if (timings != nullptr) *timings = draft;
  return FullTower(std::move(domain), std::move(orders), kmax);
}

}  // namespace mhgp11::tower_detail
