// Voie lot de la passe unique : ramassage des files, executeur (Pool ou GPU), Level depuis les supports, repli exact.
#include "catalogue/leaf_queue.hpp"
#include "catalogue/adaptive_frontier.hpp"  // add_catalogue_ledger

#include <optional>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

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

// Level de chaque enregistrement par Sphere::through de son support (meme arite que la presentation generatrice,
// donc exactement emission_level), et population convertie en SiteIdx ; en parallele, sorties a places fixes.
struct Materialize {
  const Cloud& cloud;
  std::span<const LeafRecord> input;
  std::span<const u32> population;
  std::span<Emission> records;
  std::span<SiteIdx> sites;
  u64 record_count = 0;

  Outcome record(u64 i) noexcept {
    const LeafRecord& r = input[i];
    if (r.qmin < 2 || r.qmin > 4) return fail(Reason::catalogue_invariant);
    std::array<SiteIdx, 4> support{};
    std::array<num::Point, 4> points{};
    for (u32 j = 0; j < 4; ++j) support[j] = make_id<SiteIdx>(r.support[j]);
    for (u32 j = 0; j < r.qmin; ++j) {
      if (r.support[j] >= cloud.sites()) return fail(Reason::catalogue_invariant);
      auto p = point(cloud, support[j]);
      if (!p.ok()) return p.outcome();
      points[j] = p.value();
    }
    auto made = r.qmin == 2 ? num::Sphere::through(points[0], points[1])
                : r.qmin == 3 ? num::Sphere::through(points[0], points[1], points[2])
                              : num::Sphere::through(points[0], points[1], points[2], points[3]);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return fail(Reason::catalogue_invariant);
    records[i] = Emission{CatalogueBall{support, make_id<LevelRank>(0), r.p, r.m, static_cast<u8>(r.qmin)},
                          made.value()->level(), r.population_begin};
    return {};
  }
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<Materialize*>(context);
    for (u64 i = begin; i < end; ++i) {
      if (i < self.record_count) {
        MHGP11_TRY(self.record(i));
      } else {
        // Population : blocs de 4096 apres les enregistrements.
        const u64 block = i - self.record_count, first = block * 4096;
        const u64 last = std::min<u64>(first + 4096, self.population.size());
        for (u64 k = first; k < last; ++k) self.sites[k] = make_id<SiteIdx>(self.population[k]);
      }
    }
    return {};
  }
};

Outcome gather(MemoryBudget& budget, std::span<const TaskLeafQueue* const> queues, Buffer<LeafJob>& jobs,
               Buffer<u32>& sites) noexcept {
  u64 job_count = 0, site_count = 0;
  for (const auto* q : queues) {
    MHGP11_TRY(checked_add(job_count, q->jobs()));
    MHGP11_TRY(checked_add(site_count, q->sites()));
  }
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<LeafJob>(bytes, job_count));
  MHGP11_TRY(add_bytes<u32>(bytes, site_count));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(jobs.allocate(job_count, budget));
  MHGP11_TRY(sites.allocate(site_count, budget));
  u64 job_at = 0, site_at = 0;
  for (const auto* q : queues) {
    MHGP11_TRY(q->copy_to(jobs.span().subspan(job_at, q->jobs()), sites.span().subspan(site_at, q->sites()), site_at));
    job_at += q->jobs(); site_at += q->sites();
  }
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
  MHGP11_TRY(gather(budget, queues, jobs, sites));
  const u64 gather_ns = stage->nanoseconds();
  LeafBatchView view;
  view.x = cloud.x().data(); view.y = cloud.y().data(); view.z = cloud.z().data(); view.cloud_sites = cloud.sites();
  view.jobs = jobs.data(); view.count = jobs.size(); view.sites = sites.data(); view.site_count = sites.size();
  view.kmax = params.kmax; view.cache = params.cache_center_lines;
  LeafBatchResult result;
  stage.emplace();
  if (params.cuda_leaves) MHGP11_TRY(run_leaf_batch_cuda(view, budget, result));
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
  Materialize materialize{cloud, result.records.span(), result.population.span(), out.records.span(),
                          out.population.span(), records};
  const u64 blocks = (population + 4095) / 4096;
  if (records + blocks != 0) MHGP11_TRY(pool.parallel_for(records + blocks, 256, &materialize, Materialize::body));
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
    timings->batch_prefetch_ns = t.prefetch_ns;
  }
  return {};
}

}  // namespace mhgp11::catalogue_detail

namespace mhgp11 {
void prefetch_device_context() noexcept { catalogue_detail::prefetch_cuda_context(); }
}  // namespace mhgp11
