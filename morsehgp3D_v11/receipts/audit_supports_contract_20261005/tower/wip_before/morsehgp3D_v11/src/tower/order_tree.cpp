// Ordre K seul : mise en place de la boucle non concurrente de build_full (forest_vertical.cpp) pour le seul ordre K,
// sans verticales, plus le journal des graines ; le rattachement est dans attachment.cpp. build_full n'est pas modifie.
#include "tower/order_tree.hpp"
#include "tower/census_slots.hpp"
#include "tower/forest_internal.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/population_lookup.hpp"
#include "tower/seed_log.hpp"

namespace mhgp11::tower_detail {

Result<SeedLog> SeedLog::make(const Catalogue& catalogue, u32 k, MemoryBudget& budget) noexcept {
  u64 cells = 0, seeds = 0;
  for (const auto& ball : catalogue.balls_data()) {
    if (!in_window(ball, k)) continue;
    if (ball.m == ball.qmin) {
      if (k == u64{ball.p} + ball.qmin) continue;  // naissance reguliere : aucune trace stricte
      MHGP11_TRY(cell_add(cells, 1));
      MHGP11_TRY(cell_add(seeds, ball.qmin));      // jonction reguliere : ses q faces
      continue;
    }
    const u32 t = k - ball.p;  // 1 <= t <= m par la fenetre
    if (t == ball.m) continue;  // P_b est la K-partie : naissance
    auto traces = cell_binomial(ball.m, t);
    if (!traces.ok()) return traces.outcome();
    MHGP11_TRY(cell_add(cells, 1));
    MHGP11_TRY(cell_add(seeds, traces.value()));
  }
  if (cells >= kNone || seeds > Buffer<NodeIdx>::kMaxCount / 2) return fail(Reason::tower_capacity);
  // cells < 2^32 : 12 cells + 8 < 2^36 ; seeds < 2^61 : 4 seeds < 2^63 ; somme sans debordement.
  MHGP11_TRY(budget.admit(cells * sizeof(BallIdx) + (cells + 1) * sizeof(u64) + seeds * sizeof(NodeIdx)));
  SeedLog log;
  MHGP11_TRY(log.balls_.allocate(cells, budget));
  MHGP11_TRY(log.offsets_.allocate(cells + 1, budget));
  MHGP11_TRY(log.seeds_.allocate(seeds, budget));
  log.offsets_[0] = 0;
  return log;
}

namespace {
// Foret d'un ordre et son journal ; contextes de la boucle non concurrente de build_full, rendus au retour (avant le
// balayage du rattachement), constructeur compris.
Result<OrderForest> order_forest(const FullDomain& domain, Order k, MemoryBudget& budget, const FullParams& params,
                                 sched::Pool* pool, OrderTimings* timings, SeedLog& log) noexcept {
  std::optional<PopulationLookup> population;
  if (params.population_lookup) {
    auto made = PopulationLookup::make(domain, budget, pool);
    if (!made.ok()) return made.outcome();
    population.emplace(std::move(made.value()));
  }
  const u32 workspace_count = ForestParallel::census_workspaces(params, pool);
  auto scratch = CensusSlots::make(domain, workspace_count, budget);
  if (!scratch.ok()) return scratch.outcome();
  auto memo = DescentMemo::make(domain, params.memo_capacity, budget, scratch.value().get(0));
  if (!memo.ok()) return memo.outcome();
  DescentMemo* context = params.memo_capacity == 0 && workspace_count == 0 ? nullptr : &memo.value();
  std::optional<ForestParallel> parallel;
  if (params.regular_batch_capacity != 0) {
    auto made = ForestParallel::make(domain, params, budget, *pool, workspace_count == 0 ? nullptr : &scratch.value());
    if (!made.ok()) return made.outcome();
    parallel.emplace(std::move(made.value()));
  }
  auto made = SeedLog::make(domain.catalogue(), k, budget);
  if (!made.ok()) return made.outcome();
  log = std::move(made.value());
  ForestBuilder builder(domain, k, budget, timings, context, parallel ? &*parallel : nullptr,
                        params.dense_birth_lookup);
  builder.population = population ? &*population : nullptr;
  builder.seed_log = &log;
  auto forest = builder.run();
  if (!forest.ok()) return forest.outcome();
  log.close();
  return std::move(forest.value());
}
}  // namespace

Result<OrderTree> build_order(FullDomain&& domain, Order k, MemoryBudget& budget, FullParams params,
                              sched::Pool* pool, OrderTimings* timings, u64* attach_ns) noexcept {
  if (k == 0 || k > domain.catalogue().kmax() || k > domain.index().cloud().sites())
    return fail(Reason::parameter_out_of_range);
  // Ordres concurrents : voie pipeline a un ordre (tranche S11), pas encore livree. Verticales : sans objet.
  if (params.concurrent_orders || params.parallel_verticals || params.reuse_regular_verticals)
    return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(ForestParallel::validate(params, pool));
  OrderTimings draft;
  SeedLog log;
  auto forest = order_forest(domain, k, budget, params, pool, timings == nullptr ? nullptr : &draft, log);
  if (!forest.ok()) return forest.outcome();
  const Stopwatch clock;
  auto attachment = attach_window(domain, forest.value(), log, budget);
  if (!attachment.ok()) return attachment.outcome();
  const u64 attached = clock.nanoseconds();
  log = SeedLog{};  // journal rendu avant le transfert du domaine
  if (timings != nullptr) *timings = draft;
  if (attach_ns != nullptr) *attach_ns = attached;
  return OrderTree(std::move(domain), std::move(forest.value()), std::move(attachment.value()));
}

}  // namespace mhgp11::tower_detail
