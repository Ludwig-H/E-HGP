// Executeur hote du lot de feuilles : les deux passes de l'appareil, feuille par feuille sur le Pool.
#include "catalogue/leaf_batch.hpp"

#include <utility>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

leaf_device::Input input_of(const LeafBatchView& view, const LeafJob& job) noexcept {
  leaf_device::Input in;
  in.x = view.x; in.y = view.y; in.z = view.z;
  in.sites = view.sites + job.begin; in.m = job.m;
  for (int j = 0; j < 3; ++j) { in.lo[j] = job.lo[j]; in.hi[j] = job.hi[j]; }
  in.kmax = view.kmax; in.cache = view.cache;
  return in;
}

void add_counts(leaf_device::Counts& total, const leaf_device::Counts& c) noexcept {
  total.dominance_tests += c.dominance_tests; total.prefixes += c.prefixes; total.judged += c.judged;
  total.census_tests += c.census_tests; total.emitted += c.emitted; total.incidences += c.incidences;
  total.q4_candidates += c.q4_candidates; total.q4_levels += c.q4_levels;
  total.region_pair_tests += c.region_pair_tests; total.region_pair_rejects += c.region_pair_rejects;
  total.region_line_tests += c.region_line_tests; total.region_line_rejects += c.region_line_rejects;
  total.region_line_evaluations += c.region_line_evaluations;
  total.region_line_cache_hits += c.region_line_cache_hits;
  total.region_line_fallbacks += c.region_line_fallbacks;
}

struct HostBatch {
  const LeafBatchView& view;
  std::span<u8> status, stored;
  std::span<u64> balls, incidences;  // comptes au comptage, debuts apres le prefixe
  std::span<LeafRecord> scratch_records, records;
  std::span<u8> scratch_population, population;
  std::span<leaf_device::Counts> per_worker;
  u64 total_records = 0, total_population = 0;
  bool fill = false;

  Outcome one(u64 j, u32 worker) noexcept {
    const auto in = input_of(view, view.jobs[j]);
    leaf_device::Counts c;
    if (!fill) {
      ScratchSink sink{scratch_records.data() + j * kScratchRecords, scratch_population.data() + j * kScratchPopulation,
                       in.sites, in.m};
      status[j] = static_cast<u8>(leaf_device::run_leaf(in, c, sink));
      const bool resolved = status[j] == leaf_device::kOk;
      balls[j] = resolved ? sink.balls : 0;
      incidences[j] = resolved ? sink.incidences : 0;
      stored[j] = resolved && sink.fits;
      if (resolved) add_counts(per_worker[worker], c);
      return {};
    }
    if (status[j] != leaf_device::kOk) return {};
    // Apres le prefixe, balls/incidences portent les debuts ; la fin est le debut suivant (ou le total).
    const u64 record_end = j + 1 < balls.size() ? balls[j + 1] : total_records;
    const u64 population_end = j + 1 < incidences.size() ? incidences[j + 1] : total_population;
    if (record_end == balls[j]) return {};  // aucune emission
    if (stored[j]) {
      copy_scratch(j, record_end - balls[j], population_end - incidences[j], balls[j], incidences[j],
                   scratch_records.data(), scratch_population.data(), records.data(), population.data());
      return {};
    }
    FillSink sink{records.data(), population.data(), balls[j], incidences[j], in.sites, in.m};
    if (leaf_device::run_leaf(in, c, sink) != leaf_device::kOk) return fail(Reason::catalogue_invariant);
    if (sink.record_at != record_end || sink.population_at != population_end) return fail(Reason::catalogue_invariant);
    return {};
  }
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<HostBatch*>(context);
    for (u64 j = begin; j < end; ++j) MHGP11_TRY(self.one(j, worker));
    return {};
  }
};

}  // namespace

Outcome run_leaf_batch_host(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget,
                            LeafBatchResult& result) noexcept {
  const Stopwatch total;
  // Hypothese de la borne des sommes (leaf_device::kCountBound) : au plus kMaxBatchJobs feuilles.
  if (view.count > leaf_device::kMaxBatchJobs) return fail(Reason::catalogue_counter_overflow);
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<u8>(bytes, 2 * view.count));
  MHGP11_TRY(add_bytes<u64>(bytes, 2 * view.count));
  MHGP11_TRY(add_bytes<LeafRecord>(bytes, view.count * kScratchRecords));  // count <= 2^40 (garde ci-dessus)
  MHGP11_TRY(add_bytes<u8>(bytes, view.count * kScratchPopulation));
  MHGP11_TRY(add_bytes<leaf_device::Counts>(bytes, pool.size()));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.status.allocate(view.count, budget));
  Buffer<u8> stored;
  Buffer<u64> balls, incidences;
  Buffer<LeafRecord> scratch_records;
  Buffer<u8> scratch_population;
  Buffer<leaf_device::Counts> per_worker;
  MHGP11_TRY(stored.allocate(view.count, budget));
  MHGP11_TRY(balls.allocate(view.count, budget));
  MHGP11_TRY(incidences.allocate(view.count, budget));
  MHGP11_TRY(scratch_records.allocate(view.count * kScratchRecords, budget));
  MHGP11_TRY(scratch_population.allocate(view.count * kScratchPopulation, budget));
  MHGP11_TRY(per_worker.allocate(pool.size(), budget));
  for (auto& c : per_worker.span()) c = leaf_device::Counts{};
  HostBatch batch{view, result.status.span(), stored.span(), balls.span(), incidences.span(), scratch_records.span(),
                  {}, scratch_population.span(), {}, per_worker.span()};
  const Stopwatch count;
  MHGP11_TRY(pool.parallel_for(view.count, 64, &batch, HostBatch::body));
  result.timings.count_ns = count.nanoseconds();
  // Prefixes exclusifs dans l'ordre du lot : chaque feuille ecrit a une place fixe.
  const Stopwatch scan;
  u64 records = 0, population = 0;
  for (u64 j = 0; j < view.count; ++j) {
    const u64 b = balls[j], i = incidences[j];
    balls[j] = records; incidences[j] = population;
    MHGP11_TRY(checked_add(records, b));
    MHGP11_TRY(checked_add(population, i));
    if (result.status[j] != leaf_device::kOk) ++result.timings.unresolved;
    else if (b != 0 && stored[j] == 0) ++result.timings.fill_jobs;
  }
  result.timings.scan_ns = scan.nanoseconds();
  bytes = 0;
  MHGP11_TRY(add_bytes<LeafRecord>(bytes, records));
  MHGP11_TRY(add_bytes<u8>(bytes, population));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.records.allocate(records, budget));
  MHGP11_TRY(result.population.allocate(population, budget));
  batch.records = result.records.span(); batch.population = result.population.span(); batch.fill = true;
  batch.total_records = records; batch.total_population = population;
  const Stopwatch fill;
  MHGP11_TRY(pool.parallel_for(view.count, 64, &batch, HostBatch::body));
  result.timings.fill_ns = fill.nanoseconds();
  result.counts = leaf_device::Counts{};
  for (const auto& c : per_worker.span()) add_counts(result.counts, c);
  result.record_begin = std::move(balls);  // debuts par feuille apres le prefixe
  result.population_begin = std::move(incidences);
  result.timings.jobs = view.count; result.timings.records = records; result.timings.population = population;
  result.timings.total_ns = total.nanoseconds();
  return {};
}

#if !defined(MHGP11_HAVE_CUDA)
Outcome run_leaf_batch_cuda(const LeafBatchView&, sched::Pool&, MemoryBudget&, LeafBatchResult&) noexcept {
  return fail(Reason::parameter_out_of_range);
}
bool cuda_leaf_batch_available() noexcept { return false; }
void prefetch_cuda_context() noexcept {}
#endif

}  // namespace mhgp11::catalogue_detail
