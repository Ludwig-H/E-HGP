// Une passe geometrique, arene par ordinal et compactage parallele ; refus tardif sans publication partielle.
#include <algorithm>
#include <optional>
#include "catalogue/single_pass.hpp"
#include "catalogue/single_pass_storage.hpp"
#include "catalogue/frontier_dispatch.hpp"
#include "catalogue/leaf_queue.hpp"
#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {

Outcome SinglePassOutput::account(CatalogueExecution& total) const noexcept {
  const u64 a = records_.blocks(), b = population_.blocks();
  const u64 meta = FixedPages<Emission,kEmissionBlock>::metadata_bytes();
  const u64 meta_pop = FixedPages<SiteIdx,kPopulationBlock>::metadata_bytes();
  if (a > Buffer<Emission>::kMaxCount / kEmissionBlock ||
      b > Buffer<SiteIdx>::kMaxCount / kPopulationBlock ||
      a > Buffer<std::byte>::kMaxCount / meta || b > Buffer<std::byte>::kMaxCount / meta_pop)
    return fail(Reason::memory_budget);
  MHGP11_TRY(checked_add(total.arena_blocks, a));
  MHGP11_TRY(checked_add(total.arena_blocks, b));
  MHGP11_TRY(add_bytes<Emission>(total.arena_capacity_bytes, a * kEmissionBlock));
  MHGP11_TRY(add_bytes<SiteIdx>(total.arena_capacity_bytes, b * kPopulationBlock));
  MHGP11_TRY(add_bytes<std::byte>(total.arena_metadata_bytes, a * meta));
  MHGP11_TRY(add_bytes<std::byte>(total.arena_metadata_bytes, b * meta_pop));
  MHGP11_TRY(checked_add(total.compact_records, balls()));
  return checked_add(total.compact_population, incidences());
}

namespace {
struct Output {
  SinglePassOutput data;
  TaskLeafQueue queue;  // voie lot seulement
  CatalogueLedger ledger;
  u64 ball_begin = 0, population_begin = 0, generation_ns = 0, compact_ns = 0;
};

template <class Front>
struct SingleRun {
  const Cloud& cloud;
  const CatalogueParams& params;
  MemoryBudget& budget;
  const Front& frontier;
  NodeQuota& quota;
  std::span<Workspace> workspaces;
  std::span<Output> output;
  u32 workers;
  bool timing;
  std::span<Emission> records;
  std::span<SiteIdx> population;
  std::span<const u32> order;  // reclamation par charge decroissante ; sorties toujours par ordinal

  Outcome generate(u32 ordinal, u32 worker) noexcept {
    const u32 slot = frontier.size() < workers ? ordinal : worker;
    if (ordinal >= frontier.size() || slot >= workspaces.size()) return fail(Reason::catalogue_invariant);
    auto& out = output[ordinal];
    Collector collector;
    collector.stream = &out.data; collector.stream_budget = &budget;
    Run run{cloud, params, budget, workspaces[slot], collector, {}, &quota};
    if (params.batch_leaves || params.cuda_leaves) {
      out.queue.bind(budget);
      run.deferred = &out.queue;
    }
    std::optional<Stopwatch> clock;
    if (timing) clock.emplace();
    MHGP11_TRY(frontier.execute_task(ordinal, run));
    if (workspaces[slot].walk_top != 0) return fail(Reason::catalogue_invariant);  // arene de pile rembobinee
    if (clock) out.generation_ns = clock->nanoseconds();
    if (collector.balls != out.data.balls() || collector.incidences != out.data.incidences())
      return fail(Reason::catalogue_invariant);
    out.ledger = run.ledger;
    return {};
  }
  Outcome compact(u32 ordinal) noexcept {
    if (ordinal >= frontier.size()) return fail(Reason::catalogue_invariant);
    auto& out = output[ordinal];
    if (out.ball_begin > records.size() || out.data.balls() > records.size() - out.ball_begin ||
        out.population_begin > population.size() || out.data.incidences() > population.size() - out.population_begin)
      return fail(Reason::catalogue_invariant);
    std::optional<Stopwatch> clock;
    if (timing) clock.emplace();
    MHGP11_TRY(out.data.compact(records.subspan(out.ball_begin, out.data.balls()),
                               population.subspan(out.population_begin, out.data.incidences()), out.population_begin));
    if (clock) out.compact_ns = clock->nanoseconds();
    return {};
  }
  static Outcome generate_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& run = *static_cast<SingleRun*>(context);
    if (end > run.frontier.size()) return fail(Reason::catalogue_invariant);
    for (u64 i = begin; i < end; ++i) MHGP11_TRY(run.generate(run.order[i], worker));
    return {};
  }
  static Outcome compact_body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& run = *static_cast<SingleRun*>(context);
    if (end > run.frontier.size()) return fail(Reason::catalogue_invariant);
    for (u64 i = begin; i < end; ++i) MHGP11_TRY(run.compact(static_cast<u32>(i)));
    return {};
  }
};

Outcome prefix(std::span<Output> output, const CatalogueParams& params, CatalogueLedger& ledger,
               CatalogueExecution& execution, u64& balls, u64& incidences) noexcept {
  balls = incidences = 0;
  for (auto& out : output) {
    out.ball_begin = balls; out.population_begin = incidences;
    MHGP11_TRY(checked_add(balls, out.data.balls()));
    MHGP11_TRY(checked_add(incidences, out.data.incidences()));
    MHGP11_TRY(add_catalogue_ledger(ledger, out.ledger));
    MHGP11_TRY(out.data.account(execution));
  }
  if (balls >= params.ball_limit) return fail(Reason::index_overflow_u32);
  return {};
}

// Voie lot : apres les taches, toutes les feuilles en file passent par l'executeur ; le bloc rejoint les totaux.
template <u32 Capacity>
Outcome batch_stage(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool,
                    std::span<Output> active, BatchBlock& batch, CatalogueLedger& ledger, CatalogueExecution& execution,
                    u64& balls, u64& incidences, CatalogueTimings* timings) noexcept {
  std::array<const TaskLeafQueue*, Capacity> queues{};
  for (u64 i = 0; i < active.size(); ++i) queues[i] = &active[i].queue;
  MHGP11_TRY(process_leaf_batch(cloud, params, budget, pool, std::span(queues).first(active.size()), batch, timings));
  MHGP11_TRY(add_catalogue_ledger(ledger, batch.ledger));
  MHGP11_TRY(batch.fallback.account(execution));
  for (const u64 count : {u64(batch.records.size()), batch.fallback.balls()}) {
    MHGP11_TRY(checked_add(balls, count));
  }
  MHGP11_TRY(checked_add(execution.compact_records, batch.records.size()));
  MHGP11_TRY(checked_add(execution.compact_population, batch.population.size()));
  MHGP11_TRY(checked_add(incidences, batch.population.size()));
  MHGP11_TRY(checked_add(incidences, batch.fallback.incidences()));
  if (params.ball_limit <= balls) return fail(Reason::index_overflow_u32);  // meme borne exclusive avec le lot
  return {};
}

// Copie du bloc du lot apres les sorties des taches : enregistrements decales, population, puis repli.
// Copie du bloc de lot a sa place, sur le Pool par tranches a places fixes (1,3 M emissions et 6 M incidences sur
// ng00 : la copie sequentielle coutait 30 a 40 ms sur G4).
struct BatchCopy {
  static constexpr u64 kChunk = 16384;
  const BatchBlock& batch;
  std::span<Emission> records;
  std::span<SiteIdx> population;
  u64 ball_at, population_at, record_chunks;

  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& self = *static_cast<const BatchCopy*>(context);
    for (u64 chunk = begin; chunk < end; ++chunk) {
      if (chunk < self.record_chunks) {
        const u64 first = chunk * kChunk, last = std::min<u64>(first + kChunk, self.batch.records.size());
        for (u64 i = first; i < last; ++i) {
          Emission& out = self.records[self.ball_at + i];
          out = self.batch.records[i];
          MHGP11_TRY(checked_add(out.population_begin, self.population_at));
        }
      } else {
        const u64 first = (chunk - self.record_chunks) * kChunk;
        const u64 last = std::min<u64>(first + kChunk, self.batch.population.size());
        std::copy(self.batch.population.data() + first, self.batch.population.data() + last,
                  self.population.data() + self.population_at + first);
      }
    }
    return {};
  }
};

Outcome batch_compact(const BatchBlock& batch, sched::Pool& pool, std::span<Emission> records,
                      std::span<SiteIdx> population, u64 ball_at, u64 population_at) noexcept {
  const u64 n = batch.records.size(), p = batch.population.size();
  const u64 fb = batch.fallback.balls(), fp = batch.fallback.incidences();
  if (ball_at > records.size() || n + fb > records.size() - ball_at || population_at > population.size() ||
      p + fp > population.size() - population_at)
    return fail(Reason::catalogue_invariant);
  const u64 record_chunks = (n + BatchCopy::kChunk - 1) / BatchCopy::kChunk;
  const u64 chunks = record_chunks + (p + BatchCopy::kChunk - 1) / BatchCopy::kChunk;
  BatchCopy copy{batch, records, population, ball_at, population_at, record_chunks};
  if (chunks != 0) MHGP11_TRY(pool.parallel_for(chunks, 1, &copy, BatchCopy::body));
  return batch.fallback.compact(records.subspan(ball_at + n, fb), population.subspan(population_at + p, fp),
                                population_at + p);
}

template <class Front, u32 Capacity>
Outcome generate_single(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool,
                        Buffer<Emission>& records, Buffer<SiteIdx>& population, CatalogueLedger& ledger,
                        CatalogueExecution& execution, CatalogueTimings* timings, CatalogueDiagnostics* diagnostics) noexcept {
  if (params.cuda_leaves) prefetch_cuda_context();  // recouvre l'ouverture du GPU par la frontiere et le parcours
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  Front frontier;
  Workspace unused;
  Collector unused_collector;
  NodeQuota quota(params.max_nodes);
  Run prelude{cloud, params, budget, unused, unused_collector, {}, &quota};
  MHGP11_TRY(prepare_frontier(frontier, prelude, pool));
  if (unused_collector.balls != 0 || unused_collector.incidences != 0) return fail(Reason::catalogue_invariant);
  if (timings != nullptr) { timings->prefix_ns = stage->nanoseconds(); timings->tasks = frontier.size(); stage.emplace(); }
  const u32 workers = std::min(pool.size(), frontier.size()), capacity = std::min(cloud.sites(), params.max_leaf);
  u64 bytes = 0, suffix_bytes = 0;
  MHGP11_TRY(frontier.suffix_memory_bound(pool.size(), suffix_bytes));
  MHGP11_TRY(workspace_memory_bound(capacity, workers, params.cache_center_lines, bytes, params.pair_graph));
  MHGP11_TRY(checked_add(bytes, suffix_bytes));
  if (diagnostics != nullptr) MHGP11_TRY(add_bytes<CatalogueTaskDiagnostic>(bytes, frontier.size()));
  MHGP11_TRY(budget.admit(bytes));  // Scratchs seulement. La sortie inconnue peut refuser plus tard dans un worker.
  MHGP11_TRY(DiagnosticAccess::prepare(diagnostics, frontier, budget, false));
  std::array<Workspace, sched::kMaxWorkers> workspaces;
  for (u32 i = 0; i < workers; ++i) MHGP11_TRY(workspaces[i].allocate(capacity, budget, params.cache_center_lines, params.pair_graph));
  if (timings != nullptr) timings->allocation_ns = stage->nanoseconds();
  std::array<Output, Capacity> outputs;
  auto active = std::span(outputs).first(frontier.size());
  std::array<u32, Capacity> order;
  heaviest_first(frontier, order);
  SingleRun<Front> run{cloud, params, budget, frontier, quota, std::span(workspaces).first(workers), active,
                       pool.size(), timings != nullptr || diagnostics != nullptr, {}, {},
                       std::span(order).first(frontier.size())};
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(pool.parallel_for(frontier.size(), 1, &run, SingleRun<Front>::generate_body));
  if (timings != nullptr) {
    timings->single_pass_ns = stage->nanoseconds();
    for (u32 i = 0; i < workers; ++i) MHGP11_TRY(checked_add(timings->walk_fallbacks, workspaces[i].walk_fallbacks));
  }
  ledger = frontier.ledger();
  u64 balls = 0, incidences = 0;
  MHGP11_TRY(prefix(active, params, ledger, execution, balls, incidences));
  const u64 task_balls = balls, task_incidences = incidences;
  BatchBlock batch;
  const bool batched = params.batch_leaves || params.cuda_leaves;
  if (batched)
    MHGP11_TRY(batch_stage<Capacity>(cloud, params, budget, pool, active, batch, ledger, execution, balls, incidences,
                                     timings));
  if (timings != nullptr) stage.emplace();
  bytes = 0;
  MHGP11_TRY(add_bytes<Emission>(bytes, balls));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, incidences));
  MHGP11_TRY(budget.admit(bytes));  // Toutes les pages/frontiere/scratchs coexistent encore avec ces deux sorties.
  MHGP11_TRY(records.allocate(balls, budget));
  MHGP11_TRY(population.allocate(incidences, budget));
  if (timings != nullptr) MHGP11_TRY(checked_add(timings->allocation_ns, stage->nanoseconds()));
  run.records = records.span(); run.population = population.span();
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(pool.parallel_for(frontier.size(), 1, &run, SingleRun<Front>::compact_body));
  if (batched) MHGP11_TRY(batch_compact(batch, pool, records.span(), population.span(), task_balls, task_incidences));
  if (timings != nullptr) timings->compact_ns = stage->nanoseconds();
  for (u32 i = 0; i < frontier.size(); ++i) {
    const auto& out = active[i];
    if (timings != nullptr) {
      MHGP11_TRY(checked_add(timings->single_task_sum_ns, out.generation_ns));
      MHGP11_TRY(checked_add(timings->compact_task_sum_ns, out.compact_ns));
      timings->single_task_max_ns = std::max(timings->single_task_max_ns, out.generation_ns);
      timings->compact_task_max_ns = std::max(timings->compact_task_max_ns, out.compact_ns);
    }
    DiagnosticAccess::single_result(diagnostics, i, out.ledger, out.generation_ns, out.compact_ns);
  }
  return {};  // Pages et metadonnees sont rendues AVANT le tri/assemblage.
}
}  // namespace

Result<Catalogue> build_single_pass(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                                    sched::Pool& pool, CatalogueTimings* timings, CatalogueDiagnostics* diagnostics) noexcept {
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  CatalogueLedger ledger;
  CatalogueExecution execution;
  execution.geometry_passes = 1;
  if (params.adaptive_frontier) {
    MHGP11_TRY((generate_single<AdaptiveFrontier,kAdaptiveTasks>(cloud, params, budget, pool, records, population,
                                                                ledger, execution, timings, diagnostics)));
  } else {
    MHGP11_TRY((generate_single<Frontier,kFrontierTasks>(cloud, params, budget, pool, records, population,
                                                       ledger, execution, timings, diagnostics)));
  }
  return Assembly::finish(records, population, params, ledger, budget, timings, &pool, &execution);
}

}  // namespace mhgp11::catalogue_detail
