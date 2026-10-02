// Deux passes par ordinaux de frontiere, buffers exacts, quotas globaux et sorties privees jusqu'au succes.
#include <optional>

#include "catalogue/frontier.hpp"
#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

struct TaskCounts {
  u64 balls = 0, incidences = 0, ball_begin = 0, population_begin = 0;
  u64 count_ns = 0, fill_ns = 0;
  CatalogueLedger ledger;
};

Outcome add_ledger(CatalogueLedger& sum, const CatalogueLedger& part) noexcept {
  constexpr std::array<u64 CatalogueLedger::*, 15> fields{
      &CatalogueLedger::nodes, &CatalogueLedger::leaves, &CatalogueLedger::filter_tests,
      &CatalogueLedger::dominance_tests, &CatalogueLedger::prefixes, &CatalogueLedger::judged,
      &CatalogueLedger::census_tests, &CatalogueLedger::emitted, &CatalogueLedger::incidences,
      &CatalogueLedger::q4_candidates, &CatalogueLedger::q4_levels, &CatalogueLedger::region_pair_tests,
      &CatalogueLedger::region_pair_rejects, &CatalogueLedger::region_line_tests,
      &CatalogueLedger::region_line_rejects};
  for (auto field : fields) MHGP11_TRY(checked_add(sum.*field, part.*field));
  sum.max_leaf = std::max(sum.max_leaf, part.max_leaf);
  sum.max_depth = std::max(sum.max_depth, part.max_depth);
  return {};
}

// Tous les tableaux de metadonnees ont une taille constante ; seuls leurs Buffer dependent de l'entree.
struct ParallelRun {
  const Cloud& cloud;
  const CatalogueParams& params;
  MemoryBudget& budget;
  const Frontier& frontier;
  NodeQuota& quota;
  std::array<Workspace, kFrontierTasks>& workspaces;
  std::array<TaskCounts, kFrontierTasks>& counts;
  u32 workers = 1;
  bool filling = false;
  std::span<Emission> records;
  std::span<SiteIdx> population;
  bool timing = false;

  Outcome task(u32 ordinal, u32 worker) noexcept {
    // Si J<W, chaque ordinal possede un scratch : un worker quelconque peut obtenir cet ordinal.
    const u32 slot = frontier.size() < workers ? ordinal : worker;
    if (ordinal >= frontier.size() || slot >= std::min(workers, frontier.size()))
      return fail(Reason::catalogue_invariant);
    auto& expected = counts[ordinal];
    Collector collector;
    if (filling) {
      collector.filling = true;
      collector.records = records.subspan(expected.ball_begin, expected.balls);
      collector.population = population.subspan(expected.population_begin, expected.incidences);
    }
    Run run{cloud, params, budget, workspaces[slot], collector, {}, &quota};
    std::optional<Stopwatch> task_clock;
    if (timing) task_clock.emplace();
    MHGP11_TRY(frontier.execute_task(ordinal, run));
    if (timing) (filling ? expected.fill_ns : expected.count_ns) = task_clock->nanoseconds();
    if (!filling) {
      expected.balls = collector.balls;
      expected.incidences = collector.incidences;
      expected.ledger = run.ledger;
      return {};
    }
    if (collector.balls != expected.balls || collector.incidences != expected.incidences ||
        run.ledger != expected.ledger)
      return fail(Reason::catalogue_invariant);
    for (auto& record : collector.records)
      MHGP11_TRY(checked_add(record.population_begin, expected.population_begin));
    return {};
  }

  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& run = *static_cast<ParallelRun*>(context);
    if (end > run.frontier.size()) return fail(Reason::catalogue_invariant);
    for (u64 i = begin; i < end; ++i) MHGP11_TRY(run.task(static_cast<u32>(i), worker));
    return {};
  }
};

Outcome workspace_memory_bound(u32 capacity, u32 workers, u64& bytes) noexcept {
  bytes = 0;
  const u64 words = (u64(capacity) + 63) / 64;
  MHGP11_TRY(add_bytes<num::Point>(bytes, u64(capacity) * workers));
  MHGP11_TRY(add_bytes<u64>(bytes, u64(capacity) * words * workers));
  return add_bytes<SiteIdx>(bytes, 2 * u64(capacity) * workers);
}

Outcome prefix_counts(std::span<TaskCounts> counts, const CatalogueParams& params,
                      CatalogueLedger& ledger, u64& balls, u64& population) noexcept {
  balls = population = 0;
  for (auto& task : counts) {
    task.ball_begin = balls;
    task.population_begin = population;
    MHGP11_TRY(checked_add(balls, task.balls));
    MHGP11_TRY(checked_add(population, task.incidences));
    MHGP11_TRY(add_ledger(ledger, task.ledger));
  }
  if (balls >= params.ball_limit) return fail(Reason::index_overflow_u32);
  if (balls > Buffer<Emission>::kMaxCount || population > Buffer<SiteIdx>::kMaxCount)
    return fail(Reason::memory_budget);
  return {};
}

Outcome generate_parallel(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                          sched::Pool& pool, Buffer<Emission>& records, Buffer<SiteIdx>& population,
                          CatalogueLedger& ledger, CatalogueTimings* timings) noexcept {
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  Frontier frontier;
  Workspace unused;
  Collector unused_collector;
  NodeQuota first_quota(params.max_nodes);
  Run prelude{cloud, params, budget, unused, unused_collector, {}, &first_quota};
  u64 bytes = 0;
  MHGP11_TRY(frontier_memory_bound(cloud.sites(), kFrontierDepth, bytes));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(frontier.prepare(prelude));
  if (timings != nullptr) {
    timings->prefix_ns = stage->nanoseconds();
    timings->tasks = frontier.size();
  }
  if (timings != nullptr) stage.emplace();
  const u32 workers = std::min(pool.size(), frontier.size());
  const u32 capacity = std::min(cloud.sites(), params.max_leaf);
  u64 suffix_bytes = 0;
  MHGP11_TRY(frontier.suffix_memory_bound(pool.size(), suffix_bytes));
  MHGP11_TRY(workspace_memory_bound(capacity, workers, bytes));
  MHGP11_TRY(checked_add(bytes, suffix_bytes));
  MHGP11_TRY(budget.admit(bytes));
  std::array<Workspace, kFrontierTasks> workspaces;
  for (u32 i = 0; i < workers; ++i) MHGP11_TRY(workspaces[i].allocate(capacity, budget));
  if (timings != nullptr) timings->allocation_ns = stage->nanoseconds();
  std::array<TaskCounts, kFrontierTasks> counts{};
  ParallelRun count{cloud, params, budget, frontier, first_quota, workspaces, counts, pool.size(), false, {}, {}};
  count.timing = timings != nullptr;
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(pool.parallel_for(frontier.size(), 1, &count, ParallelRun::body));
  if (timings != nullptr) timings->count_ns = stage->nanoseconds();
  ledger = frontier.ledger();
  u64 balls = 0, incidences = 0;
  MHGP11_TRY(prefix_counts(std::span(counts).first(frontier.size()), params, ledger, balls, incidences));
  if (timings != nullptr) stage.emplace();
  u64 replay_bytes = 0;
  MHGP11_TRY(frontier.verify_memory_bound(replay_bytes));
  bytes = std::max(replay_bytes, suffix_bytes);
  MHGP11_TRY(add_bytes<Emission>(bytes, balls));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, incidences));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(records.allocate(balls, budget));
  MHGP11_TRY(population.allocate(incidences, budget));
  if (timings != nullptr) MHGP11_TRY(checked_add(timings->allocation_ns, stage->nanoseconds()));
  NodeQuota second_quota(params.max_nodes);
  Run replay{cloud, params, budget, unused, unused_collector, {}, &second_quota};
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(frontier.verify(replay));
  if (timings != nullptr) timings->replay_ns = stage->nanoseconds();
  ParallelRun fill{cloud, params, budget, frontier, second_quota, workspaces, counts,
                   pool.size(), true, records.span(), population.span()};
  fill.timing = timings != nullptr;
  if (timings != nullptr) stage.emplace();
  MHGP11_TRY(pool.parallel_for(frontier.size(), 1, &fill, ParallelRun::body));
  if (timings != nullptr) {
    timings->fill_ns = stage->nanoseconds();
    for (u32 i = 0; i < frontier.size(); ++i) {
      MHGP11_TRY(checked_add(timings->count_task_sum_ns, counts[i].count_ns));
      MHGP11_TRY(checked_add(timings->fill_task_sum_ns, counts[i].fill_ns));
      timings->count_task_max_ns = std::max(timings->count_task_max_ns, counts[i].count_ns);
      timings->fill_task_max_ns = std::max(timings->fill_task_max_ns, counts[i].fill_ns);
    }
  }
  return {};
  // Tous les scratchs et la frontiere sont rendus avant Assembly::finish ; seules les sorties coexistent.
}

}  // namespace

Result<Catalogue> build_parallel(const Cloud& cloud, const CatalogueParams& params,
                                MemoryBudget& budget, sched::Pool& pool, CatalogueTimings* timings) noexcept {
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  CatalogueLedger ledger;
  MHGP11_TRY(generate_parallel(cloud, params, budget, pool, records, population, ledger, timings));
  return Assembly::finish(records, population, params, ledger, budget, timings);
}

}  // namespace mhgp11::catalogue_detail

namespace mhgp11 {

Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params,
                                 MemoryBudget& budget, sched::Pool& pool, CatalogueTimings* timings) noexcept {
  MHGP11_TRY(check_catalogue_params(params));
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  for (u32 weight : cloud.w())
    if (weight != 1) return fail(Reason::multiplicity_unsupported);
  CatalogueTimings draft;
  auto result = catalogue_detail::build_parallel(cloud, params, budget, pool, timings == nullptr ? nullptr : &draft);
  if (result.ok() && timings != nullptr) *timings = draft;
  return result;
}

}  // namespace mhgp11
