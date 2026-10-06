// Voie lot de la passe unique : ramassage des files, executeur (Pool ou GPU), Level depuis les supports, repli exact.
#include "catalogue/leaf_queue.hpp"
#include "catalogue/adaptive_frontier.hpp"  // add_catalogue_ledger, kAdaptiveTasks
#include "catalogue/frontier.hpp"           // kFrontierTasks

#include <algorithm>
#include <array>
#include <optional>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

// Au plus une file par tache de la passe unique (frontiere adaptative ou fixe).
inline constexpr u64 kMaxGatherQueues = std::max<u64>(kAdaptiveTasks, kFrontierTasks);

CatalogueLedger ledger_of(const leaf_device::Counts& c) noexcept {
  CatalogueLedger l;
  l.dominance_tests = c.dominance_tests; l.prefixes = c.prefixes; l.judged = c.judged;
  l.census_tests = c.census_tests; l.emitted = c.emitted; l.incidences = c.incidences;
  l.q4_candidates = c.q4_candidates; l.q4_levels = c.q4_levels;
  l.region_pair_tests = c.region_pair_tests; l.region_pair_rejects = c.region_pair_rejects;
  l.region_line_tests = c.region_line_tests; l.region_line_rejects = c.region_line_rejects;
  l.region_line_evaluations = c.region_line_evaluations; l.region_line_cache_hits = c.region_line_cache_hits;
  l.region_line_fallbacks = c.region_line_fallbacks;
  return l;
}

// Level et population de chaque feuille du lot : supports et rangs locaux convertis en SiteIdx par la liste des
// sites de la feuille, Level par Sphere::through du support (meme arite que la presentation generatrice, donc
// emission_level), sorties aux places fixees par les prefixes ; en parallele par feuille. Tout rang hors de la
// feuille, toute arite hors de 2..4 et toute population qui ne couvre pas exactement la plage de la feuille sont
// refuses (catalogue_invariant).
struct Materialize {
  const Cloud& cloud;
  const LeafBatchView& view;
  const LeafBatchResult& result;
  std::span<Emission> records;
  std::span<SiteIdx> sites;

  Outcome support_of(const LeafRecord& r, const LeafJob& job, std::array<SiteIdx, 4>& support,
                     std::array<num::Point, 4>& points) const noexcept {
    if (r.qmin < 2 || r.qmin > 4) return fail(Reason::catalogue_invariant);
    const u32* local = view.sites + job.begin;
    for (u32 k = 0; k < 4; ++k) {
      if (k >= r.qmin) {
        if (r.support[k] != kNoLocal) return fail(Reason::catalogue_invariant);
        support[k] = make_id<SiteIdx>(leaf_device::kNoSite);
        continue;
      }
      if (r.support[k] >= job.m || local[r.support[k]] >= cloud.sites()) return fail(Reason::catalogue_invariant);
      support[k] = make_id<SiteIdx>(local[r.support[k]]);
      auto p = point(cloud, support[k]);
      if (!p.ok()) return p.outcome();
      points[k] = p.value();
    }
    return {};
  }

  Outcome leaf(u64 j) const noexcept {
    if (result.status[j] != leaf_device::kOk) return {};
    const bool last = j + 1 == view.count;
    const u64 r1 = last ? result.records.size() : result.record_begin[j + 1];
    const u64 p1 = last ? result.population.size() : result.population_begin[j + 1];
    const LeafJob& job = view.jobs[j];
    const u32* local = view.sites + job.begin;
    u64 at = result.population_begin[j];
    if (result.record_begin[j] > r1 || r1 > result.records.size() || at > p1 || p1 > result.population.size())
      return fail(Reason::catalogue_invariant);
    for (u64 i = result.record_begin[j]; i < r1; ++i) {
      const LeafRecord& r = result.records[i];
      std::array<SiteIdx, 4> support{};
      std::array<num::Point, 4> points{};
      MHGP11_TRY(support_of(r, job, support, points));
      auto made = r.qmin == 2 ? num::Sphere::through(points[0], points[1])
                  : r.qmin == 3 ? num::Sphere::through(points[0], points[1], points[2])
                                : num::Sphere::through(points[0], points[1], points[2], points[3]);
      if (!made.ok()) return made.outcome();
      if (!made.value()) return fail(Reason::catalogue_invariant);
      const u64 n = u64(r.p) + r.m;
      if (n > p1 - at) return fail(Reason::catalogue_invariant);
      records[i] = Emission{CatalogueBall{support, make_id<LevelRank>(0), r.p, r.m, static_cast<u8>(r.qmin)},
                            made.value()->level(), at};
      for (u64 k = 0; k < n; ++k) {
        const u8 rank = result.population[at + k];
        if (rank >= job.m) return fail(Reason::catalogue_invariant);
        sites[at + k] = make_id<SiteIdx>(local[rank]);
      }
      at += n;
    }
    return at == p1 ? Outcome{} : fail(Reason::catalogue_invariant);
  }
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& self = *static_cast<const Materialize*>(context);
    for (u64 j = begin; j < end; ++j) MHGP11_TRY(self.leaf(j));
    return {};
  }
};

// Rassemblement des files : decalages par file (prefixes), puis une copie par file sur le Pool, a places fixes.
struct Gather {
  std::span<const TaskLeafQueue* const> queues;
  std::span<LeafJob> jobs;
  std::span<u32> sites;
  std::array<u64, 2 * kMaxGatherQueues> offsets{};  // debut des feuilles, puis des sites, par file

  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& self = *static_cast<const Gather*>(context);
    for (u64 i = begin; i < end; ++i) {
      const auto* q = self.queues[i];
      const u64 job_at = self.offsets[2 * i], site_at = self.offsets[2 * i + 1];
      MHGP11_TRY(q->copy_to(self.jobs.subspan(job_at, q->jobs()), self.sites.subspan(site_at, q->sites()), site_at));
    }
    return {};
  }
};

Outcome gather(MemoryBudget& budget, sched::Pool& pool, std::span<const TaskLeafQueue* const> queues,
               Buffer<LeafJob>& jobs, Buffer<u32>& sites) noexcept {
  if (queues.size() > kMaxGatherQueues) return fail(Reason::catalogue_invariant);
  Gather gathering{queues, {}, {}};
  u64 job_count = 0, site_count = 0;
  for (u64 i = 0; i < queues.size(); ++i) {
    gathering.offsets[2 * i] = job_count;
    gathering.offsets[2 * i + 1] = site_count;
    MHGP11_TRY(checked_add(job_count, queues[i]->jobs()));
    MHGP11_TRY(checked_add(site_count, queues[i]->sites()));
  }
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<LeafJob>(bytes, job_count));
  MHGP11_TRY(add_bytes<u32>(bytes, site_count));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(jobs.allocate(job_count, budget));
  MHGP11_TRY(sites.allocate(site_count, budget));
  gathering.jobs = jobs.span();
  gathering.sites = sites.span();
  if (!queues.empty()) MHGP11_TRY(pool.parallel_for(queues.size(), 1, &gathering, Gather::body));
  return {};
}

// Feuilles non resolues : leaf.cpp complet (voies checked/Wide ou refus), sans file, dans le bloc de repli.
Outcome fallback(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, const LeafBatchView& view,
                 const LeafBatchResult& result, BatchBlock& out) noexcept {
  if (result.timings.unresolved == 0) return {};
  CatalogueParams plain = params;
  plain.batch_leaves = plain.cuda_leaves = false;
  Workspace scratch;
  MHGP11_TRY(scratch.allocate(leaf_device::kMaxSites, budget, plain.cache_center_lines, plain.pair_graph));
  Collector collector;
  collector.stream = &out.fallback; collector.stream_budget = &budget;
  Run run{cloud, plain, budget, scratch, collector, {}, nullptr};
  std::array<SiteIdx, leaf_device::kMaxSites> ids{};
  for (u64 j = 0; j < view.count; ++j) {
    if (result.status[j] == leaf_device::kOk) continue;
    const LeafJob& job = view.jobs[j];
    for (u32 i = 0; i < job.m; ++i) ids[i] = make_id<SiteIdx>(view.sites[job.begin + i]);
    Box box;
    for (int a = 0; a < 3; ++a) { box.lo[a] = job.lo[a]; box.hi[a] = job.hi[a]; }
    MHGP11_TRY(enumerate_leaf(run, std::span<const SiteIdx>(ids.data(), job.m), box));
  }
  if (collector.balls != out.fallback.balls() || collector.incidences != out.fallback.incidences())
    return fail(Reason::catalogue_invariant);
  return add_catalogue_ledger(out.ledger, run.ledger);
}

}  // namespace

Outcome process_leaf_batch(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool,
                           std::span<const TaskLeafQueue* const> queues, BatchBlock& out,
                           CatalogueTimings* timings) noexcept {
  std::optional<Stopwatch> stage;
  stage.emplace();
  Buffer<LeafJob> jobs;
  Buffer<u32> sites;
  MHGP11_TRY(gather(budget, pool, queues, jobs, sites));
  const u64 gather_ns = stage->nanoseconds();
  LeafBatchView view;
  view.x = cloud.x().data(); view.y = cloud.y().data(); view.z = cloud.z().data(); view.cloud_sites = cloud.sites();
  view.jobs = jobs.data(); view.count = jobs.size(); view.sites = sites.data(); view.site_count = sites.size();
  view.kmax = params.kmax; view.cache = params.cache_center_lines; view.coop = params.coop_leaves;
  LeafBatchResult result;
  stage.emplace();
  if (params.cuda_leaves) MHGP11_TRY(run_leaf_batch_cuda(view, pool, budget, result));
  else MHGP11_TRY(run_leaf_batch_host(view, pool, budget, result));
  const u64 executor_ns = stage->nanoseconds();
  // Level et population du lot, puis repli des non resolues.
  stage.emplace();
  const u64 records = result.records.size(), population = result.population.size();
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<Emission>(bytes, records));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, population));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(out.records.allocate(records, budget));
  MHGP11_TRY(out.population.allocate(population, budget));
  if (result.record_begin.size() != view.count || result.population_begin.size() != view.count)
    return fail(Reason::catalogue_invariant);
  Materialize materialize{cloud, view, result, out.records.span(), out.population.span()};
  if (view.count != 0) MHGP11_TRY(pool.parallel_for(view.count, 256, &materialize, Materialize::body));
  const u64 levels_ns = stage->nanoseconds();
  out.ledger = ledger_of(result.counts);
  stage.emplace();
  MHGP11_TRY(fallback(cloud, params, budget, view, result, out));
  const u64 fallback_ns = stage->nanoseconds();
  if (timings != nullptr) {
    const auto& t = result.timings;
    timings->batch_jobs = t.jobs; timings->batch_unresolved = t.unresolved;
    timings->batch_records = t.records; timings->batch_population = t.population;
    timings->batch_gather_ns = gather_ns; timings->batch_count_ns = t.count_ns; timings->batch_scan_ns = t.scan_ns;
    timings->batch_fill_ns = t.fill_ns; timings->batch_executor_ns = executor_ns;
    timings->batch_device_init_ns = t.device_init_ns; timings->batch_upload_ns = t.upload_ns;
    timings->batch_download_ns = t.download_ns; timings->batch_device_bytes = t.device_bytes;
    timings->batch_levels_ns = levels_ns; timings->batch_fallback_ns = fallback_ns;
    timings->batch_prefetch_ns = t.prefetch_ns; timings->batch_fill_jobs = t.fill_jobs;
    timings->batch_copied_jobs = t.copied_jobs;
    timings->batch_device_pool_used_high = t.device_pool_used_high;
    timings->batch_device_pool_reserved_high = t.device_pool_reserved_high;
  }
  return {};
}

}  // namespace mhgp11::catalogue_detail

namespace mhgp11 {
void prefetch_device_context() noexcept { catalogue_detail::prefetch_cuda_context(); }
}  // namespace mhgp11
