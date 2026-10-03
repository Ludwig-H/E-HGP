// Verticales naturelles : remontee FERMEE des naissances puis controle de toutes les images des enfants.
#include "tower/forest_internal.hpp"
#include "tower/forest_ancestor_sweep.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/forest_vertical_seed.hpp"
#include "tower/population_lookup.hpp"
#include "tower/regular_vertical_seeds.hpp"

namespace mhgp11::tower_detail {

std::optional<NodeIdx> OrderForest::birth_node(const BirthSeed& seed) const noexcept {
  if (seed.order() != order_) return std::nullopt;
  if ((order_ == 1 && (!seed.site() || seed.ball())) || (order_ != 1 && (!seed.ball() || seed.site())))
    return std::nullopt;
  const u32 key = order_ == 1 ? idx(*seed.site()) : idx(*seed.ball());
  if (!dense_.empty()) {
    if (key >= dense_.size() || dense_[key] == NodeIdx{kNone}) return std::nullopt;
    return dense_[key];
  }
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
  const RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population;

  Result<NodeIdx> birth(const ForestNode& node) noexcept {
    if (vertical_seeds != nullptr) {
      auto cached = vertical_seeds->find(lower, node);
      if (!cached.ok()) return cached.outcome();
      if (cached.value()) {
        MHGP11_TRY(cell_add(upper.ledger_.vertical_reuses, 1));
        return sweep.query(*cached.value(), upper.ledger_);
      }
    }
    auto seed = vertical_seed(domain, lower, upper.order_, node, budget, memo, upper.ledger_.descent, nullptr,
                              population);
    if (!seed.ok()) return seed.outcome();
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, 1));
    return sweep.query(seed.value(), upper.ledger_);
  }

  Outcome run() noexcept {
    MHGP11_TRY(prepare());
    return sweep_all();
  }

  // Images basses des naissances : Pool si parallel, sinon calculees pendant le balayage.
  Outcome prepare() noexcept {
    MHGP11_TRY(allocate());
    if (parallel != nullptr) MHGP11_TRY(parallel->verticals(lower, upper, times, vertical_seeds, population));
    return {};
  }

  Outcome allocate() noexcept {
    if (upper.order_ < 2 || lower.order_ + 1 != upper.order_ || !upper.lower_.empty())
      return fail(Reason::tower_invariant);
    // L'espace des verticales suit la capacite de noeuds retenue, mais seule count_ cases sont exposees.
    MHGP11_TRY(budget.admit(upper.nodes_.size() * sizeof(NodeIdx)));
    return upper.lower_.allocate(upper.nodes_.size(), budget);
  }

  // Balayage croissant ferme ; n'ecrit que upper.lower_ et le ledger de upper (ordres disjoints concurrents).
  Outcome sweep_all() noexcept {
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
                         OrderTimings* times, const RegularVerticalSeeds* vertical_seeds,
                         const PopulationLookup* population) noexcept {
  if (parallel != nullptr && !parallel->belongs_to(domain, budget)) return fail(Reason::parameter_out_of_range);
  if (vertical_seeds != nullptr && !vertical_seeds->belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  if (population != nullptr && !population->belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  auto sweep = ClosedAncestorSweep::make(lower, budget);
  if (!sweep.ok()) return sweep.outcome();
  return VerticalBuilder{domain, lower, upper, budget, sweep.value(), memo, parallel, times, vertical_seeds,
                         population}.run();
}

namespace {
struct SweepTasks {
  const FullDomain& domain;
  std::span<OrderForest* const> forests;
  MemoryBudget& budget;
  std::span<std::optional<ClosedAncestorSweep>> sweeps;
  ForestParallel& parallel;
  std::span<OrderTimings> times;
  const RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& t = *static_cast<SweepTasks*>(context);
    for (u64 i = begin; i < end; ++i) {
      // Tache i : ordre haut i+2 ; ne lit que la foret basse immuable i+1, n'ecrit que l'ordre haut.
      std::optional<Stopwatch> clock;
      OrderTimings* own = t.times.empty() ? nullptr : &t.times[i + 1];
      if (own != nullptr) clock.emplace();
      VerticalBuilder builder{t.domain, *t.forests[i], *t.forests[i + 1], t.budget, *t.sweeps[i], nullptr,
                              &t.parallel, own, t.vertical_seeds, t.population};
      MHGP11_TRY(builder.sweep_all());
      if (clock) MHGP11_TRY(cell_add(own->verticals_ns, clock->nanoseconds()));
    }
    return {};
  }
};
}  // namespace

Outcome concurrent_verticals(const FullDomain& domain, std::span<OrderForest* const> forests, MemoryBudget& budget,
                             ForestParallel& parallel, sched::Pool& pool, std::span<OrderTimings> times,
                             const RegularVerticalSeeds* vertical_seeds, const PopulationLookup* population) noexcept {
  if (forests.size() < 2) return {};
  if (!parallel.belongs_to(domain, budget) || forests.size() > kMaxMebSites ||
      (!times.empty() && times.size() != forests.size())) return fail(Reason::parameter_out_of_range);
  std::array<std::optional<ClosedAncestorSweep>, kMaxMebSites> sweeps;
  for (u64 i = 0; i + 1 < forests.size(); ++i) {
    auto made = ClosedAncestorSweep::make(*forests[i], budget);
    if (!made.ok()) return made.outcome();
    sweeps[i].emplace(std::move(made.value()));
    VerticalBuilder builder{domain, *forests[i], *forests[i + 1], budget, *sweeps[i], nullptr, &parallel,
                            times.empty() ? nullptr : &times[i + 1], vertical_seeds, population};
    MHGP11_TRY(builder.allocate());
  }
  // Une seule distribution pour les images de tous les ordres, puis les K-1 balayages concurrents.
  std::optional<Stopwatch> clock;
  if (!times.empty()) clock.emplace();
  MHGP11_TRY(parallel.vertical_images(forests, vertical_seeds, population, times));
  if (clock)
    for (u64 i = 1; i < forests.size(); ++i) times[i].verticals_ns = times[i].vertical_dispatch_ns;
  SweepTasks tasks{domain, forests, budget, sweeps, parallel, times, vertical_seeds, population};
  return pool.parallel_for(forests.size() - 1, 1, &tasks, SweepTasks::body);
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
    std::optional<RegularVerticalSeeds> vertical_seeds;
    draft.reuse_regular_verticals = params.reuse_regular_verticals;
    if (params.reuse_regular_verticals && kmax > 1) {
      auto made = RegularVerticalSeeds::make(domain, budget);
      if (!made.ok()) return made.outcome();
      vertical_seeds.emplace(std::move(made.value()));
      draft.regular_vertical_reserved_bytes = vertical_seeds->reserved_bytes();
    }
    std::optional<PopulationLookup> population;
    draft.population_lookup = params.population_lookup;
    if (params.population_lookup) {
      auto made = PopulationLookup::make(domain, budget, pool);
      if (!made.ok()) return made.outcome();
      population.emplace(std::move(made.value()));
      draft.population_lookup_entries = population->entries();
      draft.population_lookup_reserved_bytes = population->reserved_bytes();
    }
    const u32 workspace_count = ForestParallel::census_workspaces(params, pool);
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
    draft.concurrent_orders = params.concurrent_orders;
    if (params.concurrent_orders) {
      MHGP11_TRY(build_concurrent(domain, kmax, budget, timings == nullptr ? nullptr : &draft, *parallel, *pool,
                                  vertical_seeds ? &*vertical_seeds : nullptr, population ? &*population : nullptr,
                                  params.dense_birth_lookup, orders));
    }
    for (u32 k = 1; k <= kmax && !params.concurrent_orders; ++k) {
      // Parametres bornes par K et contextes construits ici dans le meme domaine ; cache prive a cet appel FULL.
      ForestBuilder builder(domain, k, budget, timings == nullptr ? nullptr : &draft.orders[k - 1], context,
                            parallel ? &*parallel : nullptr, params.dense_birth_lookup,
                            vertical_seeds ? &*vertical_seeds : nullptr);
      builder.population = population ? &*population : nullptr;
      auto made = builder.run();
      if (!made.ok()) return made.outcome();
      orders[k - 1].emplace(std::move(made.value()));
      if (k > 1) {
        std::optional<Stopwatch> clock;
        if (timings != nullptr) clock.emplace();
        MHGP11_TRY(forest_verticals(domain, *orders[k - 2], *orders[k - 1], budget, context,
                                   params.parallel_verticals ? &*parallel : nullptr,
                                   timings == nullptr ? nullptr : &draft.orders[k - 1],
                                   vertical_seeds ? &*vertical_seeds : nullptr, population ? &*population : nullptr));
        if (clock) draft.orders[k - 1].verticals_ns = clock->nanoseconds();
      }
    }
  }  // Rend les memos, le contexte Pool et les workspaces AVANT le deplacement du domaine.
  if (timings != nullptr) *timings = draft;
  return FullTower(std::move(domain), std::move(orders), kmax);
}

}  // namespace mhgp11::tower_detail
