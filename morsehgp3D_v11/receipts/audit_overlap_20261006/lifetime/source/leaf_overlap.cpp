// Recouvrement de la passe des taches et de l'executeur des feuilles (overlap_leaves, levier L4 du plan GPU du
// 6 octobre 2026 ; build/v11-persist/gpu_optim/PLAN_GPU_FINAL.md, tranche T3).
//
// Un fil dedie consomme les sous-lots dans leur ordre : le sous-lot g couvre les positions [g c, (g+1) c) de l'ordre
// de reclamation des taches ; il attend que toutes ses taches aient fini leur generation, rassemble leurs files
// (copie sequentielle, places fixes), puis lance l'executeur (CUDA ou hote) sur un Pool prive d'un fil. Les
// resultats restent par sous-lot ; finish les concatene dans l'ordre des sous-lots (decalages des feuilles, des
// enregistrements et des populations) et finalise le tout comme un lot unique. Aucune decision ne depend de
// l'ordre d'achevement : la disposition est fixee par (sous-lot, position, feuille), et le catalogue final est trie
// dans l'ordre canonique. Un refus du fil ou de la passe est rendu par finish, sans publication partielle.
#include "catalogue/leaf_queue.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {

namespace {

void add_counts(leaf_device::Counts& into, const leaf_device::Counts& c) noexcept {
  into.dominance_tests += c.dominance_tests; into.prefixes += c.prefixes; into.judged += c.judged;
  into.census_tests += c.census_tests; into.emitted += c.emitted; into.incidences += c.incidences;
  into.q4_candidates += c.q4_candidates; into.q4_levels += c.q4_levels;
  into.region_pair_tests += c.region_pair_tests; into.region_pair_rejects += c.region_pair_rejects;
  into.region_line_tests += c.region_line_tests; into.region_line_rejects += c.region_line_rejects;
  into.region_line_evaluations += c.region_line_evaluations; into.region_line_cache_hits += c.region_line_cache_hits;
  into.region_line_fallbacks += c.region_line_fallbacks;
}

void add_timings(LeafBatchTimings& into, const LeafBatchTimings& t) noexcept {
  into.jobs += t.jobs; into.unresolved += t.unresolved; into.records += t.records; into.population += t.population;
  into.count_ns += t.count_ns; into.scan_ns += t.scan_ns; into.fill_ns += t.fill_ns; into.total_ns += t.total_ns;
  into.device_init_ns += t.device_init_ns; into.upload_ns += t.upload_ns; into.download_ns += t.download_ns;
  into.device_bytes += t.device_bytes; into.prefetch_ns += t.prefetch_ns; into.fill_jobs += t.fill_jobs;
  into.copied_jobs += t.copied_jobs;
  into.device_pool_used_high = std::max(into.device_pool_used_high, t.device_pool_used_high);
  into.device_pool_reserved_high = std::max(into.device_pool_reserved_high, t.device_pool_reserved_high);
}

struct Chunk {
  Buffer<LeafJob> jobs;
  Buffer<u32> sites;
  LeafBatchResult result;
  u64 begin = 0, end = 0;  // positions de l'ordre de reclamation
};

}  // namespace

struct OverlapLane::State {
  const Cloud* cloud = nullptr;
  const CatalogueParams* params = nullptr;
  MemoryBudget* budget = nullptr;
  std::span<const TaskLeafQueue* const> queues;
  std::vector<Chunk> chunks;  // au plus kOverlapChunks, alloue une fois avant le fil
  std::mutex mutex;
  std::condition_variable wake;
  std::vector<u8> done;  // par position, sous mutex
  bool aborted = false;
  Outcome outcome{};
  u64 gather_ns = 0, executor_ns = 0, wait_ns = 0;
  std::thread lane;

  bool chunk_ready(const Chunk& c) const noexcept {
    for (u64 i = c.begin; i < c.end; ++i)
      if (done[i] == 0) return false;
    return true;
  }

  Outcome run_chunk(Chunk& c, sched::Pool& own) noexcept {
    const Stopwatch gathering;
    MHGP11_TRY(gather_leaves(*budget, nullptr, queues.subspan(c.begin, c.end - c.begin), c.jobs, c.sites));
    gather_ns += gathering.nanoseconds();
    const LeafBatchView view = batch_view(*cloud, *params, c.jobs, c.sites);
    const Stopwatch executing;
    if (params->cuda_leaves) MHGP11_TRY(run_leaf_batch_cuda(view, own, *budget, c.result));
    else MHGP11_TRY(run_leaf_batch_host(view, own, *budget, c.result));
    executor_ns += executing.nanoseconds();
    return {};
  }

  void main() noexcept {
    auto made = sched::make_pool({1});
    Outcome result = made.ok() ? Outcome{} : made.outcome();
    for (auto& c : chunks) {
      if (!result.ok()) break;
      {
        const Stopwatch waiting;
        std::unique_lock<std::mutex> lock(mutex);
        wake.wait(lock, [&] { return aborted || chunk_ready(c); });
        wait_ns += waiting.nanoseconds();
        if (aborted) break;
      }
      result = run_chunk(c, *made.value());
    }
    std::lock_guard<std::mutex> lock(mutex);
    outcome = result;
  }
};

OverlapLane::~OverlapLane() {
  if (state_ == nullptr) return;
  abort();
  if (state_->lane.joinable()) state_->lane.join();
  delete state_;
}

Outcome OverlapLane::start(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                           std::span<const TaskLeafQueue* const> queues) noexcept {
  if (state_ != nullptr || (!params.batch_leaves && !params.cuda_leaves)) return fail(Reason::catalogue_invariant);
  try {
    state_ = new State;
    State& s = *state_;
    s.cloud = &cloud;
    s.params = &params;
    s.budget = &budget;
    s.queues = queues;
    s.done.assign(queues.size(), 0);
    const u64 chunks = std::min<u64>(kOverlapChunks, queues.size());
    s.chunks.resize(chunks);
    for (u64 g = 0; g < chunks; ++g) {
      s.chunks[g].begin = queues.size() * g / chunks;
      s.chunks[g].end = queues.size() * (g + 1) / chunks;
    }
    s.lane = std::thread([&s] { s.main(); });
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  } catch (const std::system_error&) {
    return fail(Reason::session_overhead);
  }
  return {};
}

void OverlapLane::task_done(u64 position) noexcept {
  if (state_ == nullptr) return;
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (position < state_->done.size()) state_->done[position] = 1;
  }
  state_->wake.notify_one();
}

void OverlapLane::abort() noexcept {
  if (state_ == nullptr) return;
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->aborted = true;
  }
  state_->wake.notify_one();
}

Outcome OverlapLane::finish(sched::Pool& pool, BatchBlock& out, CatalogueTimings* timings) noexcept {
  if (state_ == nullptr) return fail(Reason::catalogue_invariant);
  State& s = *state_;
  const Stopwatch tail;
  if (s.lane.joinable()) s.lane.join();
  const u64 tail_ns = tail.nanoseconds();
  if (s.aborted) return fail(Reason::catalogue_invariant);
  MHGP11_TRY(s.outcome);
  // Concatenation dans l'ordre des sous-lots : feuilles (debuts decales des sites), statuts, debuts des
  // enregistrements et des populations (decales), emissions, populations, compteurs.
  u64 jobs = 0, sites = 0, records = 0, population = 0;
  for (const auto& c : s.chunks) {
    MHGP11_TRY(checked_add(jobs, c.jobs.size()));
    MHGP11_TRY(checked_add(sites, c.sites.size()));
    MHGP11_TRY(checked_add(records, c.result.records.size()));
    MHGP11_TRY(checked_add(population, c.result.population.size()));
  }
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<LeafJob>(bytes, jobs));
  MHGP11_TRY(add_bytes<u32>(bytes, sites));
  MHGP11_TRY(add_bytes<u8>(bytes, jobs));
  MHGP11_TRY(add_bytes<u64>(bytes, 2 * jobs));
  MHGP11_TRY(add_bytes<LeafRecord>(bytes, records));
  MHGP11_TRY(add_bytes<u8>(bytes, population));
  MHGP11_TRY(s.budget->admit(bytes));
  Buffer<LeafJob> all_jobs;
  Buffer<u32> all_sites;
  LeafBatchResult all;
  MHGP11_TRY(all_jobs.allocate(jobs, *s.budget));
  MHGP11_TRY(all_sites.allocate(sites, *s.budget));
  MHGP11_TRY(all.status.allocate(jobs, *s.budget));
  MHGP11_TRY(all.record_begin.allocate(jobs, *s.budget));
  MHGP11_TRY(all.population_begin.allocate(jobs, *s.budget));
  MHGP11_TRY(all.records.allocate(records, *s.budget));
  MHGP11_TRY(all.population.allocate(population, *s.budget));
  u64 job_at = 0, site_at = 0, record_at = 0, population_at = 0;
  for (auto& c : s.chunks) {
    const u64 n = c.jobs.size();
    if (c.result.status.size() != n || c.result.record_begin.size() != n || c.result.population_begin.size() != n)
      return fail(Reason::catalogue_invariant);
    for (u64 j = 0; j < n; ++j) {
      LeafJob job = c.jobs[j];
      MHGP11_TRY(checked_add(job.begin, site_at));
      all_jobs[job_at + j] = job;
      all.status[job_at + j] = c.result.status[j];
      all.record_begin[job_at + j] = c.result.record_begin[j] + record_at;
      all.population_begin[job_at + j] = c.result.population_begin[j] + population_at;
    }
    std::copy(c.sites.begin(), c.sites.end(), all_sites.begin() + site_at);
    std::copy(c.result.records.begin(), c.result.records.end(), all.records.begin() + record_at);
    std::copy(c.result.population.begin(), c.result.population.end(), all.population.begin() + population_at);
    add_counts(all.counts, c.result.counts);
    add_timings(all.timings, c.result.timings);
    job_at += n;
    site_at += c.sites.size();
    record_at += c.result.records.size();
    population_at += c.result.population.size();
    c = Chunk{};  // memoire du sous-lot rendue avant la finalisation
  }
  const LeafBatchView view = batch_view(*s.cloud, *s.params, all_jobs, all_sites);
  MHGP11_TRY(finalize_leaf_batch(*s.cloud, *s.params, *s.budget, pool, view, all, s.gather_ns, s.executor_ns, out,
                                 timings));
  if (timings != nullptr) {
    timings->batch_overlap_chunks = s.chunks.size();
    timings->batch_overlap_wait_ns = s.wait_ns;
    timings->batch_overlap_tail_ns = tail_ns;
  }
  return {};
}

}  // namespace mhgp11::catalogue_detail
