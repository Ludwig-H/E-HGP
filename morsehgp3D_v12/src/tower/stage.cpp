// Etage G de la tour (CONTRAT_TOUR.md, paragraphes 4, 7 et 8) : sequence compter, admettre, reserver, remplir,
// resoudre, controler, publier. Toute la memoire de l'etage est admise dans le budget avant d'etre allouee ; un refus
// ne publie rien. Resolution par tranches de cellules sur le Pool : chaque representant a sa case, ecrite par un seul
// fil ; un espace de census et des compteurs par fil, fusionnes dans l'ordre des fils (sommes et maxima : independants
// du decoupage). Controles globaux avant publication : un controle de decroissance par plus petite boule et par succes
// de sonde (contrat du paragraphe 4.1 : avant toute sortie par saut), une longueur de chaine par representant.
#include <algorithm>
#include <memory>

#include "tower/stage.hpp"

namespace mhgp12 {

u32 Resolution::window_target(BallIdx b, Order k) const noexcept {
  const u64 at = window_offsets_[idx(b)], end = window_offsets_[idx(b) + 1];
  const u32 lo = window_lo_[idx(b)];
  if (k < lo || u64{k} - lo >= end - at) return kNoTarget;
  return window_targets_[at + (k - lo)];
}

Outcome check_order_capacity(u64 births, u64 cells, u64 representatives) noexcept {
  if (births > kMaxOrderBirths || cells > kMaxOrderCells || representatives > kMaxOrderRepresentatives)
    return fail(Reason::tower_capacity);
  return {};
}

namespace tower_detail {
namespace {

constexpr u64 kCellGrain = 256;  // cellules par tranche de resolution

// Octets admis pour les sorties d'un ordre.
u64 order_bytes(u64 births, u64 cells, u64 reps) noexcept {
  return births * (sizeof(u32) + sizeof(LevelRank)) + cells * (sizeof(BallIdx) + sizeof(LevelRank) + 1) +
         (cells + 1) * sizeof(u64) + reps * (sizeof(u64) + sizeof(u32));
}

Outcome allocate_order(ResolvedOrder& o, Order k, u64 births, u64 cells, u64 reps, MemoryBudget& budget) noexcept {
  StageAccess::order_id(o) = k;
  MHGP12_TRY(StageAccess::birth_keys(o).allocate(births, budget));
  MHGP12_TRY(StageAccess::birth_ranks(o).allocate(births, budget));
  MHGP12_TRY(StageAccess::cell_balls(o).allocate(cells, budget));
  MHGP12_TRY(StageAccess::cell_ranks(o).allocate(cells, budget));
  MHGP12_TRY(StageAccess::cell_flags(o).allocate(cells, budget));
  MHGP12_TRY(StageAccess::cell_offsets(o).allocate(cells + 1, budget));
  MHGP12_TRY(StageAccess::trace_masks(o).allocate(reps, budget));
  return StageAccess::targets(o).allocate(reps, budget);
}

// Ordre 1 : les naissances sont les sites, et la cible d'une trace (un site) est sa naissance.
struct FirstOrder {
  const Catalogue& catalogue;
  ResolvedOrder& order;
  static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<FirstOrder*>(raw);
    const auto balls = StageAccess::cell_balls(self.order).span();
    const auto offsets = StageAccess::cell_offsets(self.order).span();
    const auto masks = StageAccess::trace_masks(self.order).span();
    auto targets = StageAccess::targets(self.order).span();
    for (u64 c = begin; c < end; ++c) {
      const auto shell = self.catalogue.shell(balls[c]);
      for (u64 r = offsets[c]; r < offsets[c + 1]; ++r) {
        if (masks[r] == 0 || (masks[r] & (masks[r] - 1)) != 0) return fail(Reason::tower_invariant, 1);
        const u32 j = static_cast<u32>(__builtin_ctzll(masks[r]));
        if (j >= shell.size()) return fail(Reason::tower_invariant, 1);
        targets[r] = birth_target(idx(shell[j]));
      }
    }
    return {};
  }
};

struct Workers {
  std::array<std::unique_ptr<CensusWorkspace>, sched::kMaxWorkers> census;
  Buffer<OrderCounters> counters;
};

struct OrderPass {
  const ResolveContext& context;
  ResolvedOrder& order;
  Workers& workers;
  static Outcome body(void* raw, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<OrderPass*>(raw);
    const Catalogue& cat = self.context.domain.catalogue;
    const auto balls = StageAccess::cell_balls(self.order).span();
    const auto ranks = StageAccess::cell_ranks(self.order).span();
    const auto offsets = StageAccess::cell_offsets(self.order).span();
    const auto masks = StageAccess::trace_masks(self.order).span();
    auto targets = StageAccess::targets(self.order).span();
    OrderCounters& counters = self.workers.counters[worker];
    CensusWorkspace& workspace = *self.workers.census[worker];
    const Order k = self.context.order.k;
    for (u64 c = begin; c < end; ++c) {
      const auto inner = cat.interior(balls[c]), shell = cat.shell(balls[c]);
      for (u64 r = offsets[c]; r < offsets[c + 1]; ++r) {
        Part f;  // trace stricte I u A, SiteIdx croissants
        std::size_t i = 0;
        for (u64 rest = masks[r]; i < inner.size() || rest != 0;) {
          const u32 j = rest != 0 ? static_cast<u32>(__builtin_ctzll(rest)) : 0;
          const bool from_inner = rest == 0 || (i < inner.size() && idx(inner[i]) < idx(shell[j]));
          if (f.k == kMaxPart) return fail(Reason::tower_invariant, k);
          f.id[f.k++] = idx(from_inner ? inner[i++] : shell[j]);
          if (!from_inner) rest &= rest - 1;
        }
        if (f.k != k) return fail(Reason::tower_invariant, k);
        auto target = resolve_part(self.context, f, ranks[c], workspace, counters);
        if (!target.ok()) return target.outcome();
        targets[r] = target.value();
      }
    }
    return {};
  }
};

Outcome resolve_orders(const Domain& d, Resolution& out, const std::array<PopulationTable, kMaxPart>& tables,
                       Workers& workers, sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  for (Order k = 1; k <= out.orders(); ++k) {
    const Stopwatch watch;
    ResolvedOrder& order = StageAccess::order(out, k);
    if (k == 1) {
      FirstOrder first{d.catalogue, order};
      MHGP12_TRY(pool.parallel_for(order.cells(), kCellGrain, &first, &FirstOrder::body));
    } else {
      for (u32 w = 0; w < pool.size(); ++w) workers.counters[w] = OrderCounters{};
      const ResolveContext context{d, out, OrderView{k, order.birth_keys(), &tables[k - 1]}};
      OrderPass pass{context, order, workers};
      MHGP12_TRY(pool.parallel_for(order.cells(), kCellGrain, &pass, &OrderPass::body));
      OrderCounters& total = StageAccess::counters(order);
      for (u32 w = 0; w < pool.size(); ++w) add_counters(total, workers.counters[w]);
      // Controles globaux : un controle par plus petite boule et par succes de sonde ; une chaine par representant.
      u64 chains = 0;
      for (const u64 bin : total.chain_histogram) chains += bin;
      if (total.controls != total.smallest_balls() + total.first_probe_hits + total.probe_hits_after_steps ||
          chains != order.representatives() ||
          chains != total.first_probe_hits + total.probe_hits_after_steps + total.cell_stops + total.birth_stops)
        return fail(Reason::tower_invariant, k);
    }
    diag.order_ns[k] = watch.nanoseconds();
  }
  return {};
}

Result<Buffer<num::Point>> prepare_points(const Cloud& cloud, MemoryBudget& budget) noexcept {
  Buffer<num::Point> points;
  MHGP12_TRY(points.allocate(cloud.sites(), budget));
  for (u32 s = 0; s < cloud.sites(); ++s) {
    auto p = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
    if (!p.ok()) return p.outcome();
    points[s] = p.value();
  }
  return points;
}

// Coherence du catalogue avec l'index : tout SiteIdx de S* et des populations designe un site du nuage.
Outcome check_catalogue(const Catalogue& cat, u32 sites) noexcept {
  for (const SiteIdx s : cat.population())
    if (idx(s) >= sites) return fail(Reason::tower_invariant);
  for (const auto& ball : cat.balls_data())
    for (u32 i = 0; i < ball.qmin && i < 4; ++i)
      if (idx(ball.support[i]) >= sites) return fail(Reason::tower_invariant);
  return {};
}

Outcome plan_and_fill(const Domain& d, Resolution& out, MemoryBudget& budget, sched::Pool& pool,
                      ResolutionDiagnostics& diag) noexcept {
  const u32 n = d.index.cloud().sites(), balls = d.catalogue.balls();
  CellPlan plan;
  plan.orders = out.orders();
  plan.blocks = (u64{balls} + kBallBlock - 1) / kBallBlock;
  const u64 fields = plan.blocks * kMaxPart * kCountFields, workers = pool.size();
  MHGP12_TRY(budget.admit((2 * fields + 2 * plan.blocks + workers * kScratchWordsPerWorker) * sizeof(u64) +
                          workers * kMaxCellCombinations * sizeof(u32)));
  MHGP12_TRY(plan.counts.allocate(fields, budget));
  MHGP12_TRY(plan.starts.allocate(fields, budget));
  MHGP12_TRY(plan.windows.allocate(plan.blocks, budget));
  MHGP12_TRY(plan.window_starts.allocate(plan.blocks, budget));
  Buffer<u64> words;
  Buffer<u32> parent;
  MHGP12_TRY(words.allocate(workers * kScratchWordsPerWorker, budget));
  MHGP12_TRY(parent.allocate(workers * kMaxCellCombinations, budget));
  const Stopwatch count;
  MHGP12_TRY(count_cells(d, plan, words.span(), parent.span(), pool));
  diag.count_ns = count.nanoseconds();
  // Capacite par ordre (ordre 1 : les n sites sont les naissances), puis admission de toutes les sorties.
  u64 bytes = (u64{balls} + 1) * sizeof(u64) + plan.total_windows * sizeof(u32) + u64{balls};
  for (Order k = 1; k <= plan.orders; ++k) {
    const auto& t = plan.totals[k - 1];
    const u64 births = k == 1 ? n : t[kBirths];
    if (const Outcome o = check_order_capacity(births, t[kCells], t[kReps]); !o.ok()) return fail(o.reason, k);
    bytes += order_bytes(births, t[kCells], t[kReps]);
  }
  MHGP12_TRY(budget.admit(bytes));
  MHGP12_TRY(StageAccess::window_offsets(out).allocate(u64{balls} + 1, budget));
  MHGP12_TRY(StageAccess::window_targets(out).allocate(plan.total_windows, budget));
  MHGP12_TRY(StageAccess::window_lo(out).allocate(balls, budget));
  for (Order k = 1; k <= plan.orders; ++k) {
    const auto& t = plan.totals[k - 1];
    MHGP12_TRY(allocate_order(StageAccess::order(out, k), k, k == 1 ? n : t[kBirths], t[kCells], t[kReps], budget));
  }
  const Stopwatch fill;
  MHGP12_TRY(fill_cells(d, plan, out, words.span(), parent.span(), pool));
  ResolvedOrder& first = StageAccess::order(out, 1);
  for (u32 s = 0; s < n; ++s) {
    StageAccess::birth_keys(first)[s] = s;
    StageAccess::birth_ranks(first)[s] = make_id<LevelRank>(0);
  }
  for (Order k = 1; k <= plan.orders; ++k) {  // compteurs de l'objet
    const auto& t = plan.totals[k - 1];
    OrderCounters& c = StageAccess::counters(StageAccess::order(out, k));
    c.births = k == 1 ? n : t[kBirths];
    c.cells = t[kCells];
    c.inert_cells = t[kInert];
    c.extended_cells = t[kExtended];
    c.representatives = t[kReps];
  }
  diag.fill_ns = fill.nanoseconds();
  return {};
}

Result<Resolution> run_stage(const GlobalIndex& index, const Catalogue& catalogue, MemoryBudget& budget,
                             sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  const Order kmax = catalogue.kmax();
  if (kmax < 1 || kmax > kMaxPart) return fail(Reason::kmax_out_of_range);
  const u32 n = index.cloud().sites();
  if (n == 0) return fail(Reason::empty_input);
  MHGP12_TRY(check_catalogue(catalogue, n));
  auto points = prepare_points(index.cloud(), budget);
  if (!points.ok()) return points.outcome();
  const Domain d{index, catalogue, points.value().span()};
  Resolution out;
  StageAccess::set_orders(out, kmax, static_cast<Order>(std::min<u32>(kmax, n)));
  MHGP12_TRY(plan_and_fill(d, out, budget, pool, diag));
  // Tables de populations (un ordre par tache, insertion sequentielle : disposition deterministe).
  std::array<PopulationTable, kMaxPart> tables;
  u64 table_bytes = 0;
  for (Order k = 2; k <= out.orders(); ++k)
    table_bytes += PopulationTable::capacity_for(population_entries(catalogue, out.order(k).birth_keys(), k)) * 8;
  const u64 workers = pool.size();
  MHGP12_TRY(budget.admit(table_bytes + workers * (u64{n} * sizeof(SiteIdx) + sizeof(OrderCounters))));
  const Stopwatch tabling;
  struct Tables {
    const Catalogue& catalogue;
    const Resolution& out;
    std::array<PopulationTable, kMaxPart>& tables;
    MemoryBudget& budget;
    static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
      auto& self = *static_cast<Tables*>(raw);
      for (u64 i = begin; i < end; ++i) {
        const Order k = static_cast<Order>(i + 2);
        MHGP12_TRY(self.tables[k - 1].build(self.catalogue, self.out.order(k).birth_keys(), k, self.budget));
      }
      return {};
    }
  } build{catalogue, out, tables, budget};
  if (out.orders() >= 2) MHGP12_TRY(pool.parallel_for(out.orders() - 1u, 1, &build, &Tables::body));
  diag.tables_ns = tabling.nanoseconds();
  Workers team;
  MHGP12_TRY(team.counters.allocate(workers, budget));
  for (u32 w = 0; w < workers; ++w) {
    auto made = CensusWorkspace::make(index, budget);
    if (!made.ok()) return made.outcome();
    team.census[w] = std::move(made.value());
  }
  const Stopwatch resolving;
  MHGP12_TRY(resolve_orders(d, out, tables, team, pool, diag));
  diag.resolve_ns = resolving.nanoseconds();
  diag.threads = workers;
  diag.workspace_bytes = workers * u64{n} * sizeof(SiteIdx);
  diag.table_bytes = table_bytes;
  diag.peak_bytes = budget.peak();
  return out;
}

}  // namespace
}  // namespace tower_detail

Result<Resolution> resolve_tower(const GlobalIndex& index, const Catalogue& catalogue, MemoryBudget& budget,
                                 sched::Pool& pool, ResolutionDiagnostics* diagnostics) noexcept {
  ResolutionDiagnostics diag;
  auto made = guarded([&]() { return tower_detail::run_stage(index, catalogue, budget, pool, diag); });
  if (made.ok() && diagnostics != nullptr) *diagnostics = diag;
  return made;
}

}  // namespace mhgp12
