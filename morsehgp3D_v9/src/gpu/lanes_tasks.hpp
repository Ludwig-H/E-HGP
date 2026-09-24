#pragma once

// MorseHGP3D v9 (24 septembre 2026) — S4b step 2 of the lanes plan: the
// lanes of an edge as (edge, seed range) TASKS, so that a heavy edge no
// longer makes the tail of the call (docs/tour_voies_conception_20260924/,
// judge's plan « Étape 2 — T2 simplifié »). Three portable steps, run by the
// device (WarpGroup, filter_runner.cu) and by the host twin (HostGroup,
// lanes_host.hpp) with the same functions:
// - P, one per edge: validity, cover, scan order and seeds (the prologue of
//   edge_lanes), written at the edge's place in a COVER ARENA; the number of
//   tasks is lanes_task_count(sites, seeds, B), a function of the input;
// - T, one per task: for each seed of the range, the q3 census (if asked),
//   then the q4 seed (if asked); the records of the task go to the group's
//   record slab, then to one reservation of a STAGING arena; the task keeps
//   its record counts per phase and its first failure (phase, kind);
// - C, one per edge: lanes_replay rejoues the sequential precedence of
//   edge_lanes (prologue, the q3 seeds in order, then the q4 seeds in
//   order; a record slab full on the edge's TOTAL defers), then the caller
//   applies the arena rule in edge order (counter always advanced) and
//   gathers the records of a decided edge in (phase, task) order: all its
//   q3 records, then all its q4 records, in seed order — exactly the
//   records, order, statuses and ledger of edge_lanes. The ledger of an
//   edge's tasks is accumulated in the edge's slot (sums and maxima,
//   commutative) and reduced over the decided edges only.
// Why the object is unchanged: every seed of edge_lanes is run once, by the
// same functions, on the same cover in the same scan order; seeds are
// independent (no state crosses a seed except the record slab and the
// counters); the output order and the deferral decisions are rebuilt from
// the per-task counts, never from the completion order. An arena overflow
// (cover, staging) is an explicit capacity refusal of the call.

#include "q4_lanes.hpp"

namespace mhgp9::gpu {

// B, the task budget in seed-chunks: a task holds the consecutive seeds of
// one edge whose bound seeds x ceil(sites/32) (the 32-site packets of their
// lens passes and censuses) stays within B, at least one seed. The default
// bounds the lens packets of a task to about 0.5 ms of a lone warp on G4
// (2 473 cycles per packet, judge's plafond-752 fit, 2.4 GHz); B = 1 is one
// seed per task, B = single_task_budget one task per edge (step 1). No B
// changes a decided output. MHGP9_LANES_TASK_BUDGET (measurement builds
// only) changes the default.
#ifndef MHGP9_LANES_TASK_BUDGET
#define MHGP9_LANES_TASK_BUDGET 512
#endif
inline constexpr u64 default_task_budget = MHGP9_LANES_TASK_BUDGET;
static_assert(default_task_budget >= 1, "a task holds at least one seed-chunk");
inline constexpr u64 single_task_budget = ~0ULL;

MHGP9_HD inline u32 lanes_seeds_per_task(u32 sites, u64 budget) {
  const u64 chunks = (static_cast<u64>(sites) + 31) / 32;
  const u64 per = chunks == 0 ? budget : budget / chunks;
  return per == 0 ? 1U : (per > 0xffffffffULL ? 0xffffffffU : static_cast<u32>(per));
}

// Tasks of an edge: ceil(seeds / per); none without seeds.
MHGP9_HD inline u32 lanes_task_count(u32 sites, u32 seeds, u64 budget) {
  const u32 per = lanes_seeds_per_task(sites, budget);
#if defined(MHGP9_LANES_TASKS_MUTANT_LAST_RANGE_FORGOTTEN)
  return seeds / per;  // mutant: the last partial range is never run
#else
  return seeds / per + (seeds % per != 0 ? 1U : 0U);
#endif
}

// One task of the table. P's scan and the fill give (edge, first, last); T
// writes the rest. fail_phase: 0 none, else the lane bit of the phase where
// the task stopped (2 q3, 4 q4) and fail_kind its CertificateStatus; q3 and
// q4 count the records of the task before that failure. `begin` is the
// first staged record (tasks without failure only).
struct LanesTask {
  u32 edge, first, last;
  u32 q3, q4;
  u8 fail_phase, fail_kind, unused0, unused1;
  u64 begin;
};

// P's answer for one edge: the prologue's status (decided: the tasks run),
// the cover's place in the arena, its sites, seeds and tasks.
struct LanesPlan {
  u64 offset;
  u32 sites, seeds, tasks;
  u8 status, lanes, unused0, unused1;
};

// The work of an edge's tasks, accumulated in the edge's slot: the census
// part of EdgeQ3Work (in u64) and the q4 work.
struct LanesTaskWork {
  u64 census_point_tests, census_inside_sites, census_shell_sites, census_outside_sites;
  u64 depth_rejections, emitted, shell_ids;
  Q4Work w4;
};

// Portable integer atomics (device: atomicAdd/atomicMax; host: GCC
// builtins, relaxed): sums and maxima commute, so the slot does not depend
// on the order of the tasks.
MHGP9_HD inline void atomic_add_u64(u64& to, u64 value) {
  if (value == 0) return;
#if defined(__CUDA_ARCH__)
  atomicAdd(&to, value);
#else
  __atomic_fetch_add(&to, value, __ATOMIC_RELAXED);
#endif
}

MHGP9_HD inline void atomic_max_u64(u64& to, u64 value) {
  if (value == 0) return;
#if defined(__CUDA_ARCH__)
  atomicMax(&to, value);
#else
  u64 seen = __atomic_load_n(&to, __ATOMIC_RELAXED);
  while (seen < value && !__atomic_compare_exchange_n(&to, &seen, value, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
  }
#endif
}

// A task's work into its edge's slot (one caller per task: the leader),
// the census part as soon as the q3 phase ends (fewer live registers in the
// q4 phase), then the q4 part.
MHGP9_HD inline void lanes_add_census_work(LanesTaskWork& slot, const EdgeQ3Work& census) {
  atomic_add_u64(slot.census_point_tests, census.census_point_tests);
  atomic_add_u64(slot.census_inside_sites, census.census_inside_sites);
  atomic_add_u64(slot.census_shell_sites, census.census_shell_sites);
  atomic_add_u64(slot.census_outside_sites, census.census_outside_sites);
  atomic_add_u64(slot.depth_rejections, census.depth_rejections);
  atomic_add_u64(slot.emitted, census.emitted);
  atomic_add_u64(slot.shell_ids, census.shell_ids);
}

MHGP9_HD inline void lanes_add_q4_work(LanesTaskWork& slot, const Q4Work& w) {
  Q4Work& to = slot.w4;
  atomic_add_u64(to.edges, w.edges); atomic_add_u64(to.seeds, w.seeds); atomic_add_u64(to.certified, w.certified);
  atomic_add_u64(to.certified_chunk1, w.certified_chunk1); atomic_add_u64(to.survivors, w.survivors);
  atomic_add_u64(to.pass_chunks, w.pass_chunks); atomic_add_u64(to.pass_site_tests, w.pass_site_tests);
  atomic_add_u64(to.buffered_events, w.buffered_events); atomic_max_u64(to.max_buffered, w.max_buffered);
  atomic_add_u64(to.live_buckets, w.live_buckets); atomic_add_u64(to.filter_steps, w.filter_steps);
  atomic_add_u64(to.bucket_events, w.bucket_events); atomic_add_u64(to.candidates, w.candidates);
  atomic_add_u64(to.foreign_candidates, w.foreign_candidates); atomic_add_u64(to.groups, w.groups);
  atomic_add_u64(to.compare_steps, w.compare_steps); atomic_add_u64(to.depth_rejected_groups, w.depth_rejected_groups);
  atomic_add_u64(to.positivity_tests, w.positivity_tests);
  atomic_add_u64(to.groups_without_valid, w.groups_without_valid); atomic_add_u64(to.emitted, w.emitted);
  atomic_add_u64(to.emitting_seeds, w.emitting_seeds); atomic_add_u64(to.multi_emission_seeds, w.multi_emission_seeds);
  atomic_max_u64(to.max_emissions_per_seed, w.max_emissions_per_seed); atomic_add_u64(to.shell_ids, w.shell_ids);
  atomic_max_u64(to.max_group, w.max_group); atomic_add_u64(to.constant_shell_sites, w.constant_shell_sites);
  atomic_add_u64(to.list_steps, w.list_steps); atomic_add_u64(to.group_steps, w.group_steps);
}

// The declared work of one task in 32-site group steps (published as its
// maximum over the call): its census packets (census point tests / 32,
// rounded up), lens passes, filters, bucket lists and class passes.
MHGP9_HD inline u64 lanes_census_steps(const EdgeQ3Work& census) { return (census.census_point_tests + 31) / 32; }
MHGP9_HD inline u64 lanes_q4_steps(const Q4Work& w) {
  return w.pass_chunks + w.filter_steps + w.list_steps + w.group_steps;
}

// ---- P ---------------------------------------------------------------------

// P, step 1: the validity of the asked lanes (as edge_lanes) and the cover's
// ranges (lanes_cover). `local` is reset. Uniform result.
template <class Group>
MHGP9_HD CertificateStatus lanes_plan_cover(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                            u8 lanes, unsigned kmax, const LanesSlab& slab, u32& range_count,
                                            u32& sites, EdgeQ3Work& local) {
  range_count = sites = 0;
  local = EdgeQ3Work{};
  if (lanes == 0 || (lanes & ~6U) != 0 || kmax < 2 || ((lanes & 4U) != 0 && kmax < 3))
    return CertificateStatus::fault;
  return lanes_cover(group, index, a_rank, b_rank, slab, range_count, sites, local);
}

// P, step 2, once the cover has its place (slab.points / ranks / seeds
// there): scan order, seeds, and the edge's own q3 counters (q3_edges and
// census_seeds, as q3_census). Returns the number of seeds.
template <class Group>
MHGP9_HD u32 lanes_plan_order(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank, u8 lanes,
                              const LanesSlab& slab, u32 range_count, u32 sites, EdgeQ3Work& local) {
  const u32 seeds = lanes_order(group, index, a_rank, b_rank, slab, range_count, sites, local);
  if ((lanes & 2U) != 0) {
    ++local.q3_edges;
    local.census_seeds += seeds;
  }
  return seeds;
}

// ---- T ---------------------------------------------------------------------

// T: the seeds [first, last) of an edge whose cover and seeds are at
// slab.points / ranks / seeds; the records (q3 of the range, then q4 of the
// range, each in seed order) at slab.records[0, q3 + q4). A q3 failure stops
// the task (every later seed of the edge comes after it in sequential
// order); so does a q4 failure (the q3 part of the range is complete). The
// leader adds the task's work to its edge's `slot` and returns its declared
// steps (lanes_census_steps + lanes_q4_steps); `w4` is the group's q4
// scratch (leader lane). Uniform result.
template <class Group>
MHGP9_HD u64 lanes_task(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank, u8 lanes,
                        unsigned kmax, const LanesSlab& slab, u32 sites, u32 first, u32 last, const Q4Slab& q4,
                        LanesTask& task, Q4Work& w4, LanesTaskWork& slot) {
  task.q3 = task.q4 = 0;
  task.fail_phase = task.fail_kind = 0;
  if (group.leader()) w4 = Q4Work{};
  group.sync();
  u32 count = 0;
  u64 steps = 0;
  if ((lanes & 2U) != 0) {
    EdgeQ3Work census{};
    const auto status = q3_census_range(group, index, a_rank, b_rank, kmax, slab, sites, first, last, count, census);
    task.q3 = count;
    steps = lanes_census_steps(census);
    if (group.leader()) lanes_add_census_work(slot, census);
    if (status != CertificateStatus::decided) {
      task.fail_phase = 2;
      task.fail_kind = static_cast<u8>(status);
      group.sync();
      return steps;
    }
  }
  if ((lanes & 4U) != 0) {
    const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
    const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
    const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
    for (u32 i = first; i < last; ++i) {
      const auto status = q4_seed(group, index, a, b, id_a, id_b, slab.seeds[i], slab, sites, kmax, q4, count, w4);
      if (status != CertificateStatus::decided) {
        task.fail_phase = 4;
        task.fail_kind = static_cast<u8>(status);
        break;
      }
    }
    task.q4 = count - task.q3;
    group.sync();  // the leader's q4 work
    if (group.leader()) lanes_add_q4_work(slot, w4);
    steps += lanes_q4_steps(w4);  // every lane reads the leader's work after the sync
  }
  group.sync();
  return steps;
}

// ---- C ---------------------------------------------------------------------

// C: the status of an edge whose prologue was decided, from its tasks in
// table order, as the sequential edge_lanes: the q3 phase (tasks in order),
// then the q4 phase; in each segment the records come before the task's
// failure. The records of the edge overflow the record slab as soon as
// their running total exceeds record_capacity (deferred); otherwise the
// first failure met decides. `records` receives the edge's total (decided).
MHGP9_HD inline CertificateStatus lanes_replay(const LanesTask* tasks, u32 count, u8 lanes, u32 record_capacity,
                                               u32& records) {
  records = 0;
  u64 total = 0;
#if defined(MHGP9_LANES_TASKS_MUTANT_FAULT_FIRST)
  // mutant: a fault anywhere wins over a deferral met before it
  for (u32 t = 0; t < count; ++t)
    if (tasks[t].fail_phase != 0 && tasks[t].fail_kind == static_cast<u8>(CertificateStatus::fault))
      return CertificateStatus::fault;
#endif
  for (u32 phase = 2; phase <= 4; phase += 2) {
    if ((lanes & phase) == 0) continue;
    for (u32 t = 0; t < count; ++t) {
      total += phase == 2 ? tasks[t].q3 : tasks[t].q4;
      if (total > record_capacity) return CertificateStatus::deferred;
      if (tasks[t].fail_phase == phase) return static_cast<CertificateStatus>(tasks[t].fail_kind);
    }
  }
  records = static_cast<u32>(total);
  return CertificateStatus::decided;
}

// The ledger of a decided edge: the prologue's (P) plus its tasks' slot.
MHGP9_HD inline void lanes_edge_work(const EdgeQ3Work& plan, const LanesTaskWork& slot, u8 lanes, EdgeQ3Work& e3,
                                     Q4Work& e4) {
  e3 = plan;
  e3.census_point_tests += slot.census_point_tests;
  e3.census_inside_sites += slot.census_inside_sites;
  e3.census_shell_sites += slot.census_shell_sites;
  e3.census_outside_sites += slot.census_outside_sites;
  e3.depth_rejections += static_cast<u32>(slot.depth_rejections);
  e3.emitted += static_cast<u32>(slot.emitted);
  e3.shell_ids += slot.shell_ids;
  e4 = slot.w4;
  e4.edges = (lanes & 4U) != 0 ? 1 : 0;
}

}  // namespace mhgp9::gpu
