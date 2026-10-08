// Etage G de la tour (CONTRAT_TOUR.md, paragraphes 4, 7 et 8) : sequence preparer, compter, admettre, reserver,
// remplir, resoudre, controler, publier. Toute la memoire de l'etage est admise dans le budget avant d'etre allouee ;
// un refus ne publie rien. Les passes par ordre (index, premieres sondes, resolution, controles) sont dans passes.cpp ;
// ici : controle du catalogue et points exacts (paralleles, T2-c : le controle sequentiel des incidences pesait sur le
// reste du mur a 48 fils), plan des cellules, sorties, espaces des fils, chronos aux frontieres disjointes.
#include <algorithm>
#include <memory>

#include "tower/profile.hpp"
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

constexpr u64 kPrepareGrain = 1 << 16;  // sites, incidences ou boules par tranche de preparation

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

// Points exacts des sites et coherence du catalogue avec l'index (tout SiteIdx des populations et de S* designe un
// site du nuage), par tranches sur le Pool.
struct Prepare {
  const Cloud& cloud;
  const Catalogue& catalogue;
  std::span<num::Point> points;
  static Outcome points_body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Prepare*>(raw);
    const u64 hi = std::min<u64>(s.points.size(), end * kPrepareGrain);
    for (u64 i = begin * kPrepareGrain; i < hi; ++i) {
      auto p = num::Point::make(s.cloud.x()[i], s.cloud.y()[i], s.cloud.z()[i]);
      if (!p.ok()) return p.outcome();
      s.points[i] = p.value();
    }
    return {};
  }
  static Outcome population_body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Prepare*>(raw);
    const auto population = s.catalogue.population();
    const u64 hi = std::min<u64>(population.size(), end * kPrepareGrain);
    for (u64 i = begin * kPrepareGrain; i < hi; ++i)
      if (idx(population[i]) >= s.cloud.sites()) return fail(Reason::tower_invariant);
    return {};
  }
  static Outcome supports_body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Prepare*>(raw);
    const auto balls = s.catalogue.balls_data();
    const u64 hi = std::min<u64>(balls.size(), end * kPrepareGrain);
    for (u64 b = begin * kPrepareGrain; b < hi; ++b)
      for (u32 i = 0; i < balls[b].qmin && i < 4; ++i)
        if (idx(balls[b].support[i]) >= s.cloud.sites()) return fail(Reason::tower_invariant);
    return {};
  }
};

u64 slices(u64 n) noexcept { return (n + kPrepareGrain - 1) / kPrepareGrain; }

Result<Buffer<num::Point>> prepare(const Cloud& cloud, const Catalogue& catalogue, MemoryBudget& budget,
                                   sched::Pool& pool) noexcept {
  Buffer<num::Point> points;
  MHGP12_TRY(points.allocate(cloud.sites(), budget));
  Prepare work{cloud, catalogue, points.span()};
  MHGP12_TRY(pool.parallel_for(slices(catalogue.population().size()), 1, &work, &Prepare::population_body));
  MHGP12_TRY(pool.parallel_for(slices(catalogue.balls()), 1, &work, &Prepare::supports_body));
  MHGP12_TRY(pool.parallel_for(slices(cloud.sites()), 1, &work, &Prepare::points_body));
  return points;
}

// Sorties par ordre : capacite (ordre 1 : les n sites sont les naissances), admission, allocation.
Outcome allocate_outputs(const Domain& d, const CellPlan& plan, Resolution& out, MemoryBudget& budget) noexcept {
  const u32 n = d.index.cloud().sites(), balls = d.catalogue.balls();
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
  return {};
}

Outcome plan_and_fill(const Domain& d, Resolution& out, MemoryBudget& budget, sched::Pool& pool,
                      ResolutionDiagnostics& diag) noexcept {
  const u32 n = d.index.cloud().sites(), balls = d.catalogue.balls();
  CellPlan plan;
  plan.orders = out.orders();
  plan.blocks = (u64{balls} + kBallBlock - 1) / kBallBlock;
  const u64 fields = plan.blocks * kMaxPart * kCountFields, workers = pool.size();
  const Stopwatch counting;
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
  MHGP12_TRY(count_cells(d, plan, words.span(), parent.span(), pool));
  diag.count_ns = counting.nanoseconds();
  const Stopwatch setting;
  MHGP12_TRY(allocate_outputs(d, plan, out, budget));
  diag.setup_ns = setting.nanoseconds();
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

// Memoire des ordres, admise avant tout calcul : espaces de census et compteurs des fils, puis index des naissances,
// garde d'un ordre a l'autre (borne : toutes les naissances du plus grand ordre, fiches de K mots ; un tampon ne grandit
// qu'a la taille du plus grand ordre). La voie G-L7 n'alloue rien d'autre.
u64 orders_bytes(const Resolution& out, u64 workers, u32 sites) noexcept {
  u64 table = 0;
  for (Order k = 2; k <= out.orders(); ++k)
    table = std::max(table, PopulationTable::bytes_for(out.order(k).births(), out.orders()));
  return table + workers * (u64{sites} * sizeof(SiteIdx) + sizeof(OrderCounters) + sizeof(SectionCycles));
}

Result<Resolution> run_stage(const GlobalIndex& index, const Catalogue& catalogue, MemoryBudget& budget,
                             sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  const Order kmax = catalogue.kmax();
  if (kmax < 1 || kmax > kMaxPart) return fail(Reason::kmax_out_of_range);
  const u32 n = index.cloud().sites();
  if (n == 0) return fail(Reason::empty_input);
  const Stopwatch preparing;
  auto points = prepare(index.cloud(), catalogue, budget, pool);
  if (!points.ok()) return points.outcome();
  diag.prepare_ns = preparing.nanoseconds();
  const Domain d{index, catalogue, points.value().span()};
  Resolution out;
  StageAccess::set_orders(out, kmax, static_cast<Order>(std::min<u32>(kmax, n)));
  MHGP12_TRY(plan_and_fill(d, out, budget, pool, diag));
  const u64 workers = pool.size();
  const Stopwatch staffing;
  MHGP12_TRY(budget.admit(orders_bytes(out, workers, n)));
  Workers team;
  MHGP12_TRY(team.counters.allocate(workers, budget));
  if constexpr (kProfile) MHGP12_TRY(team.profiles.allocate(workers, budget));
  for (u32 w = 0; w < workers; ++w) {
    auto made = CensusWorkspace::make(index, budget);
    if (!made.ok()) return made.outcome();
    team.census[w] = std::move(made.value());
  }
  diag.workspace_ns = staffing.nanoseconds();
  OrderBuffers buffers;
  MHGP12_TRY(resolve_orders(d, out, buffers, team, budget, pool, diag));
  diag.threads = workers;
  diag.profiled = kProfile;
  diag.workspace_bytes = workers * u64{n} * sizeof(SiteIdx);
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
