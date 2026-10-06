// Pipeline des ordres concurrents : resolution reguliere par blocs, publication et balayages verticaux recouverts.
// Une seule distribution du Pool : taches [0,L) de resolution (blocs reclames dans l'ordre global des boules),
// [L,L+K) publications (une par ordre, graines lues bloc par bloc apres leur publication), [L+K,L+2K-1) balayages
// suivis (ordre haut h=2..K). Le Pool reclame les taches par indice croissant et seules publications et balayages
// attendent, toujours des taches d'indice inferieur : aucun interblocage, quel que soit W. Memes graines, forets,
// verticales et compteurs que la voie par etages ; un refus de resolution est rendu par sa tache, les attentes
// abandonnent sans resultat. Journal des graines (L2b) : ecrit par la seule tache de publication de son ordre.
#include <algorithm>
#include <ctime>

#include "tower/forest_ancestor_sweep.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/forest_placement.hpp"
#include "tower/population_lookup.hpp"
#include "tower/regular_vertical_seeds.hpp"
#include "sched/sched.hpp"

namespace mhgp11::tower_detail {
namespace {

constexpr u32 kBlockJobs = 256;

// Temps CPU du fil appelant, diagnostic seulement ; 0 si l'horloge manque (aucune decision n'en depend).
u64 thread_cpu_ns() noexcept {
  timespec now{};
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) return 0;
  return u64(now.tv_sec) * 1000000000u + u64(now.tv_nsec);
}

struct Block { u32 order, local; };

struct Pipeline {
  const FullDomain& domain;
  MemoryBudget& budget;
  ForestParallel& parallel;
  RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population;
  std::span<ForestBuilder* const> builders;
  std::span<const std::span<const BallIdx>> jobs;
  std::span<const std::span<NodeIdx>> seeds;
  std::span<const Block> blocks;
  std::span<const JobGate> gates;
  std::span<ForestProgress> progress;
  std::span<std::optional<ClosedAncestorSweep>> sweeps;
  std::span<DescentLedger> lane_work;   // L * K
  std::span<ForestLedger> vertical_work;  // K, case 0 inutilisee
  std::span<u64> finished;                // fin de chaque tache depuis le debut du pipeline, ns
  std::span<u64> started, cpu, waited;    // diagnostic : debut, CPU du fil, attente bloquee de chaque tache
  std::atomic<u64> next{0};
  const PipelinePlacement* placement = nullptr;  // coeurs des taches (forest_placement.hpp), nul sans placement
  u32 lanes = 0, kmax = 0;
  bool timed = false;
  std::optional<Stopwatch> origin{};

  Outcome resolve(u32 task) noexcept {
    CensusWorkspace* scratch = parallel.census_slot(task);
    Outcome total;
    for (;;) {
      const u64 b = next.fetch_add(1, std::memory_order_relaxed);
      if (b >= blocks.size()) break;
      const Block block = blocks[b];
      ForestBuilder& builder = *builders[block.order];
      const auto list = jobs[block.order];
      const u64 first = u64{block.local} * kBlockJobs, last = std::min<u64>(list.size(), first + kBlockJobs);
      Outcome outcome;
      for (u64 j = first; j < last; ++j) {
        std::array<NodeIdx, 4> found{NodeIdx{kNone}, NodeIdx{kNone}, NodeIdx{kNone}, NodeIdx{kNone}};
        Outcome one = parallel.resolve_regular(builder, list[j], found, scratch,
                                               lane_work[u64{task} * kmax + block.order]);
        // Graine basse de la naissance haute de cette boule : memorisee ici, lue par le balayage de l'ordre haut.
        if (one.ok() && vertical_seeds != nullptr) one = vertical_seeds->remember(builder.result, list[j], found[0]);
        if (!one.ok()) { outcome = merge(outcome, one); continue; }
        std::copy(found.begin(), found.end(), seeds[block.order].begin() + 4 * j);
      }
      std::atomic_ref<u8>(gates[block.order].state[block.local])
          .store(outcome.ok() ? u8{1} : u8{2}, std::memory_order_release);
      gates[block.order].epoch->fetch_add(1, std::memory_order_release);
      gates[block.order].epoch->notify_all();
      total = merge(total, outcome);
    }
    return total;
  }

  Outcome publish(u32 order, u32 task) noexcept {
    ForestBuilder& builder = *builders[order];
    builder.extended_scratch = parallel.census_slot(task);
    builder.gate = &gates[order];
    builder.progress = &progress[order];
    builder.wait_ns = timed ? &waited[task] : nullptr;
    Outcome outcome = builder.publish(jobs[order], seeds[order]);
    if (outcome.ok() && !builder.abandoned) outcome = builder.finish();
    const bool complete = outcome.ok() && !builder.abandoned;
    progress[order].finish(static_cast<u32>(builder.result.nodes().size()), complete);
    builder.extended_scratch = nullptr; builder.gate = nullptr; builder.progress = nullptr; builder.wait_ns = nullptr;
    return outcome;
  }

  Outcome follow(u32 upper, u32 task) noexcept {
    return follow_verticals(domain, builders[upper - 1]->result, builders[upper]->result, budget, *sweeps[upper - 1],
                            parallel, vertical_seeds, population, parallel.census_slot(task), progress[upper - 1],
                            progress[upper], vertical_work[upper], timed ? &waited[task] : nullptr);
  }

  Outcome run(u32 task) noexcept {
    const ScopedAffinity placed(placement == nullptr ? nullptr : placement->set_of(task));
    u64 cpu0 = 0;
    if (timed) { started[task] = origin->nanoseconds(); cpu0 = thread_cpu_ns(); }
    Outcome outcome = task < lanes ? resolve(task) : task < lanes + kmax ? publish(task - lanes, task)
                                                                         : follow(task - lanes - kmax + 1, task);
    if (timed) {
      finished[task] = origin->nanoseconds();
      const u64 cpu1 = thread_cpu_ns();
      cpu[task] = cpu1 >= cpu0 ? cpu1 - cpu0 : 0;
    }
    return outcome;
  }

  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<Pipeline*>(context);
    Outcome outcome;
    for (u64 t = begin; t < end; ++t) outcome = merge(outcome, self.run(static_cast<u32>(t)));
    return outcome;
  }
};

// Diagnostics du pipeline (hors decisions) : phases disjointes, queues par ordre, debuts, CPU et attentes des taches.
Outcome publish_timings(const Pipeline& pipeline, std::span<const u64> starts, std::span<const u64> cpus,
                        std::span<const u64> waits, FullTimings& timings) noexcept {
  const u32 lanes = pipeline.lanes, kmax = pipeline.kmax, tasks = lanes + 2 * kmax - 1;
  // Phases disjointes et queues par ordre : resolution jusqu'a la fin de la derniere resolution R, publication
  // de R a la derniere publication P, verticales au-dela ; chaque ordre rend sa queue dans la phase partagee.
  u64 resolved = 0, published = 0, swept = 0;
  for (u32 t = 0; t < tasks; ++t) {
    u64& end = t < lanes ? resolved : t < lanes + kmax ? published : swept;
    end = std::max(end, pipeline.finished[t]);
  }
  const u64 publish_end = std::max(resolved, published), vertical_end = std::max(publish_end, swept);
  timings.pipeline_lanes = lanes;
  timings.regular_phase_ns = resolved;
  timings.publish_phase_ns = publish_end - resolved;
  timings.vertical_phase_ns = vertical_end - publish_end;
  for (u32 t = lanes; t < tasks; ++t) {
    const u64 end = pipeline.finished[t];
    if (t < lanes + kmax) {
      auto& o = timings.orders[t - lanes];
      o.plateaus_ns = end > resolved ? end - resolved : 0;
      o.publish_start_ns = starts[t]; o.publish_end_ns = end; o.publish_cpu_ns = cpus[t]; o.publish_wait_ns = waits[t];
    } else {
      auto& o = timings.orders[t - lanes - kmax + 1];
      o.verticals_ns = end > publish_end ? end - publish_end : 0;
      o.vertical_start_ns = starts[t]; o.vertical_end_ns = end; o.vertical_cpu_ns = cpus[t];
      o.vertical_wait_ns = waits[t];
    }
  }
  u64 last_start = 0, first_finish = ~u64{0}, lane_cpu = 0;
  for (u32 t = 0; t < lanes; ++t) {
    last_start = std::max(last_start, starts[t]);
    first_finish = std::min(first_finish, pipeline.finished[t]);
    MHGP11_TRY(cell_add(lane_cpu, cpus[t]));
  }
  timings.lanes_last_start_ns = last_start;
  timings.lanes_first_finish_ns = first_finish;
  timings.lanes_last_finish_ns = resolved;
  timings.lanes_cpu_ns = lane_cpu;
  return {};
}

}  // namespace

u32 pipeline_lanes(const ForestParallel& parallel, const sched::Pool& pool, u32 kmax, bool memo) noexcept {
  const u32 tasks = parallel.census_slots() != 0 ? parallel.census_slots() : pool.size();
  if (memo || kmax < 2 || pool.size() < 2 * kmax || tasks < 2 * kmax) return 0;
  return tasks - (2 * kmax - 1);
}

Outcome pipeline_orders(const FullDomain& domain, MemoryBudget& budget, ForestParallel& parallel, sched::Pool& pool,
                        RegularVerticalSeeds* vertical_seeds, const PopulationLookup* population,
                        std::span<ForestBuilder* const> builders, std::span<const std::span<const BallIdx>> jobs,
                        std::span<const std::span<NodeIdx>> seeds, u32 lanes, FullTimings* timings) noexcept {
  const u32 kmax = static_cast<u32>(builders.size());
  if (kmax < 2 || kmax > kMaxMebSites || jobs.size() != kmax || seeds.size() != kmax || lanes == 0)
    return fail(Reason::parameter_out_of_range);
  const u32 tasks = lanes + 2 * kmax - 1;
  // Journal des graines (L2b) : au plus un ordre le porte ; seule sa tache de publication (Pipeline::publish) appelle
  // cell et regular_cell, les resolutions n'ecrivent que des graines par ordinal et les balayages ne lisent que les
  // forets. Un second journal violerait le contrat d'un seul ordre journalise.
  u32 logged = 0;
  for (const ForestBuilder* builder : builders) logged += builder->seed_log != nullptr ? 1 : 0;
  if (logged > 1) return fail(Reason::tower_invariant);
  // Blocs de chaque ordre, puis ordre global par premiere boule : les publications avancent ensemble par rang.
  u64 count = 0;
  for (u32 i = 0; i < kmax; ++i) MHGP11_TRY(cell_add(count, (jobs[i].size() + kBlockJobs - 1) / kBlockJobs));
  const u64 sites = domain.index().cloud().sites();
  const u64 owned = parallel.census_slots() >= tasks ? 0 : tasks - parallel.census_slots();
  u64 bytes = 0;
  MHGP11_TRY(cell_add(bytes, count * (sizeof(Block) + 1)));
  MHGP11_TRY(cell_add(bytes, 4 * u64{tasks} * sizeof(u64) + u64{lanes} * kmax * sizeof(DescentLedger)));
  MHGP11_TRY(cell_add(bytes, u64{kmax} * sizeof(ForestLedger) + 4 * sites * owned));
  for (u32 i = 0; i + 1 < kmax; ++i) {
    MHGP11_TRY(cell_add(bytes, 3 * builders[i]->result.node_capacity() * sizeof(u32)));
    MHGP11_TRY(cell_add(bytes, builders[i + 1]->result.node_capacity() * sizeof(NodeIdx)));
  }
  MHGP11_TRY(budget.admit(bytes));
  Buffer<Block> blocks;
  Buffer<u8> states;
  Buffer<u64> times, starts, cpus, waits;
  Buffer<DescentLedger> lane_work;
  Buffer<ForestLedger> vertical_work;
  MHGP11_TRY(blocks.allocate(count, budget));
  MHGP11_TRY(states.allocate(count, budget));
  MHGP11_TRY(times.allocate(tasks, budget));
  MHGP11_TRY(starts.allocate(tasks, budget));
  MHGP11_TRY(cpus.allocate(tasks, budget));
  MHGP11_TRY(waits.allocate(tasks, budget));
  MHGP11_TRY(lane_work.allocate(u64{lanes} * kmax, budget));
  MHGP11_TRY(vertical_work.allocate(kmax, budget));
  std::fill(states.span().begin(), states.span().end(), u8{0});
  for (auto* array : {&times, &starts, &cpus, &waits}) std::fill(array->span().begin(), array->span().end(), u64{0});
  for (auto& w : lane_work.span()) w = DescentLedger{};
  for (auto& w : vertical_work.span()) w = ForestLedger{};
  std::array<JobGate, kMaxMebSites> gates{};
  std::array<std::atomic<u32>, kMaxMebSites> epochs{};
  u64 at = 0;
  for (u32 i = 0; i < kmax; ++i) {
    const u64 local = (jobs[i].size() + kBlockJobs - 1) / kBlockJobs;
    gates[i] = JobGate{states.span().subspan(at, local), kBlockJobs, &epochs[i]};
    for (u64 c = 0; c < local; ++c) blocks[at + c] = Block{i, static_cast<u32>(c)};
    at += local;
  }
  // Tri en place sans tampon (MemoryBudget) : cle totale (premiere boule, ordre, bloc local).
  forest_sort(blocks.span(), [&](const Block& a, const Block& b) noexcept {
    const u32 x = idx(jobs[a.order][u64{a.local} * kBlockJobs]), y = idx(jobs[b.order][u64{b.local} * kBlockJobs]);
    return x != y ? x < y : a.order != b.order ? a.order < b.order : a.local < b.local;
  });
  // Balayages : etats a la capacite de la foret basse ; images basses de la foret haute allouees avant.
  std::array<std::optional<ClosedAncestorSweep>, kMaxMebSites> sweeps;
  for (u32 i = 0; i + 1 < kmax; ++i) {
    auto made = ClosedAncestorSweep::make(builders[i]->result, budget, builders[i]->result.node_capacity());
    if (!made.ok()) return made.outcome();
    sweeps[i].emplace(std::move(made.value()));
    MHGP11_TRY(allocate_verticals(builders[i]->result, builders[i + 1]->result, budget));
  }
  std::array<ForestProgress, kMaxMebSites> progress;
  for (u32 i = 0; i < kmax; ++i) builders[i]->vertical_seeds = nullptr;  // memorisees par la resolution
  Pipeline pipeline{domain, budget, parallel, vertical_seeds, population, builders, jobs, seeds, blocks.span(),
                    std::span(gates).first(kmax), std::span(progress).first(kmax), std::span(sweeps).first(kmax),
                    lane_work.span(), vertical_work.span(), times.span(), starts.span(), cpus.span(), waits.span()};
  pipeline.lanes = lanes; pipeline.kmax = kmax; pipeline.timed = timings != nullptr;
  // Placement (FullParams::place_pipeline) : plan lu une fois par passe ; topologie illisible ou inadaptee : aucun.
  PipelinePlacement plan;
  if (parallel.place_pipeline()) {
    CpuCores cores;
    if (read_cpu_cores(cores)) plan = plan_pipeline(cores, lanes, kmax);
    if (plan.cores != 0) pipeline.placement = &plan;
  }
  if (timings != nullptr) {
    timings->pipeline_placement_cores = plan.cores;
    timings->pipeline_placement_requested = parallel.place_pipeline();
  }
  if (pipeline.timed) pipeline.origin.emplace();
  MHGP11_TRY(pool.parallel_for(tasks, 1, &pipeline, Pipeline::body));
  // Travail somme dans l'ordre fixe (taches, ordres) : memes totaux que la voie par etages.
  for (u32 i = 0; i < kmax; ++i)
    for (u32 t = 0; t < lanes; ++t) MHGP11_TRY(builders[i]->regular_work(lane_work[u64{t} * kmax + i]));
  for (u32 i = 1; i < kmax; ++i) MHGP11_TRY(add_vertical_work(builders[i]->result, vertical_work[i]));
  if (timings != nullptr) MHGP11_TRY(publish_timings(pipeline, starts.span(), cpus.span(), waits.span(), *timings));
  return {};
}

}  // namespace mhgp11::tower_detail
