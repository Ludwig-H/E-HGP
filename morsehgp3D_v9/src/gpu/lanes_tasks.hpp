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
// - C: the replay of the sequential precedence of edge_lanes (prologue,
//   the q3 seeds in order, then the q4 seeds in order; a record slab full
//   on the edge's TOTAL defers), then the arena rule in edge order (counter
//   always advanced), the records of a decided edge placed in (phase, task)
//   order: all its q3 records, then all its q4 records, in seed order —
//   exactly the records, order, statuses and ledger of edge_lanes. The
//   ledger of an edge's tasks is accumulated in the edge's slot (sums and
//   maxima, commutative) and reduced over the decided edges only. Since the
//   C step v2 (lanes plan step 3) no loop runs over an edge's tasks: T
//   publishes each task's records and its failure (order-free minimum), the
//   replay (lanes_replay_scan) and the places (lanes_task_destinations) read
//   the inclusive scan of the task records in O(1).
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
  u8 fail_phase, fail_kind, layout, unused1;  // layout: 1 when the fused pass placed the records (L15)
  u64 begin;
};

// The slab index of a task's r-th record in staging order (its q3 records,
// then its q4 records): the fused pass (L15) keeps the q3 records at the top
// of the slab, last to first, and the q4 records from the bottom.
MHGP9_HD inline u32 lanes_task_slab_index(const LanesTask& task, u32 record_capacity, u32 r) {
  if (task.layout == 0) return r;
  return r < task.q3 ? record_capacity - 1 - r : r - task.q3;
}

// L15 counters of the call, summed over ALL its tasks (order-free): seeds
// run by the fused pass, the chunks it read (each read once for both
// lanes), those read by the census alone after the lens pass had stopped,
// the chunks the census consumed (what a separate census would have read:
// census_chunks - q3_chunks were absorbed by the lens pass), and the tasks
// rerun unfused because their records did not fit the slab.
struct LanesFusedWork {
  u64 seeds, chunks, q3_chunks, census_chunks, fallbacks;
};
MHGP9_HD inline void add_fused(LanesFusedWork& to, const LanesFusedWork& from) {
  to.seeds += from.seeds;
  to.chunks += from.chunks;
  to.q3_chunks += from.q3_chunks;
  to.census_chunks += from.census_chunks;
  to.fallbacks += from.fallbacks;
}

// P's answer for one edge: the prologue's status (decided: the tasks run),
// the cover's place in the arena, its sites, seeds and tasks.
struct LanesPlan {
  u64 offset;
  u32 sites, seeds, tasks;
  u8 status, lanes, unused0, unused1;
};

// The first failure of an edge's tasks in edge_lanes' sequential order
// (every q3 seed before every q4 seed; the tasks of an edge are consecutive
// seed ranges, consecutive in the table): the q3 phase of the task at table
// index t is at position t, its q4 phase at 2^32 + t. No failure: ~0.
inline constexpr u64 lanes_no_failure = ~0ULL;
MHGP9_HD inline u64 lanes_fail_position(u32 fail_phase, u64 t) {
  return (fail_phase == 4 ? (u64{1} << 32) : u64{0}) | t;
}

// The work of an edge's tasks, accumulated in the edge's slot: the census
// part of EdgeQ3Work (in u64) and the q4 work; `fail_first` is the minimum
// lanes_fail_position of its failed tasks (C step v2).
struct LanesTaskWork {
  u64 census_point_tests, census_inside_sites, census_shell_sites, census_outside_sites;
  u64 depth_rejections, emitted, shell_ids;
  Q4Work w4;
  u64 fail_first = lanes_no_failure;
};

// Records of one task per phase; the C step scans them over the task table
// (inclusive, table order) to replay and place without a loop over tasks.
struct LanesTaskRecords {
  u64 q3, q4;
};
struct LanesRecordsSum {
  MHGP9_HD LanesTaskRecords operator()(const LanesTaskRecords& x, const LanesTaskRecords& y) const {
    return LanesTaskRecords{x.q3 + y.q3, x.q4 + y.q4};
  }
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

MHGP9_HD inline void atomic_min_u64(u64& to, u64 value) {
#if defined(__CUDA_ARCH__)
  atomicMin(&to, value);
#else
  u64 seen = __atomic_load_n(&to, __ATOMIC_RELAXED);
  while (seen > value && !__atomic_compare_exchange_n(&to, &seen, value, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
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
// census_seeds, as q3_census). Returns the number of seeds; `sites` becomes
// the number of sites kept by L11 (the tasks' scans run over them).
template <class Group>
MHGP9_HD u32 lanes_plan_order(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank, u8 lanes,
                              const LanesSlab& slab, u32 range_count, u32& sites, EdgeQ3Work& local) {
  const u32 seeds = lanes_order(group, index, a_rank, b_rank, slab, range_count, sites, local);
  if ((lanes & 2U) != 0) {
    ++local.q3_edges;
    local.census_seeds += seeds;
  }
  return seeds;
}

// ---- T ---------------------------------------------------------------------

// L15 (lanes plan step 3, 24 septembre 2026): the seeds [first, last) of an
// edge with BOTH lanes in one scan per seed. The census of the seed's q3
// ball and the lens pass of its q4 family read the same 32-site chunks in
// the same order; P is computed once per site (the lens vote keeps the sign
// of P = f(0) per lane, LaneSigns). Each scan stops exactly where it would
// alone (the census at its (K-1)-th site of negative power, the pass at
// certification) and the other goes on, so decisions, records and counters
// are those of the separate phases (edge_lanes' q3 census then q4 seeds).
// Records: q3 at the top of the slab (last to first), q4 from the bottom;
// the task keeps its sequential failure semantics (a q3 fault stops the
// task with no q4 record; a q4 failure closes the q4 lane, the census goes
// on to the end of the range). If a record would not fit the slab the
// outcome is `fallback`: nothing is published and the caller reruns the
// task unfused (the separate phases' own slab rule then decides).
enum class LanesFused : u8 { done, q3_failed, fallback };

template <class Group>
MHGP9_HD LanesFused lanes_task_fused(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                     unsigned kmax, const LanesSlab& slab, u32 sites, u32 first, u32 last,
                                     const Q4Slab& q4, LanesTask& task, Q4Work& w4, EdgeQ3Work& census,
                                     LanesFusedWork& fused) {
  const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
  const u32 capacity = slab.record_capacity;
#if defined(MHGP9_LANES_MUTANT_FUSED_Q3_AT_K_MINUS_2)
  const u32 threshold3 = kmax - 2;  // mutant: the fused census stops one interior site early
#else
  const u32 threshold3 = kmax - 1;
#endif
  const u32 threshold4 = kmax - 2;
  u32 c3 = 0, c4 = 0;
  bool q4_open = true;
  u32 seeds = 0, chunks = 0, q3_chunks = 0, census_chunks = 0;  // this task's L15 counters
  for (u32 i = first; i < last; ++i) {
    const u32 seed = slab.seeds[i];
    const std::int32_t* x = slab.points + 3 * static_cast<std::size_t>(seed);
    Q4Pass p;  // p.f.form: the seed's q3 form, shared by both lanes
    if (!q3_form(a, b, x, p.f.form)) {  // a q3 fault comes before every q4 seed
      task.q3 = c3;
      task.q4 = 0;
      task.fail_phase = 2;
      task.fail_kind = static_cast<u8>(CertificateStatus::fault);
      return LanesFused::q3_failed;
    }
    bool pass = q4_open;
    if (pass && !q4_pass_begin(a, b, x, p)) {  // a q4 fault closes the q4 lane
      q4_open = pass = false;
      task.q4 = c4;
      task.fail_phase = 4;
      task.fail_kind = static_cast<u8>(CertificateStatus::fault);
    }
    Q3Census c{0, 0, 0, 0, 0, false};
    LaneSigns signs;
    for (u32 base = 0; base < sites; base += Group::size) {
      const bool lens = pass && !p.certified;
#if defined(MHGP9_LANES_MUTANT_FUSED_JOINT_STOP)
      if (c.rejected) break;  // mutant: the lens pass stops with the census
#endif
      if (c.rejected && !lens) break;
      ++chunks;
      u32 inside = 0, zero = 0;
      if (lens) {
        q4_pass_chunk(group, index, a, slab, sites, threshold4, q4, base, p, signs);
        if (!c.rejected) group.ballot2(base, sites, [&](u32 t) { return signs.get(t - base); }, inside, zero);
      } else {
        ++q3_chunks;
        group.ballot2(base, sites, [&](u32 t) -> u32 {
          const i128 power = q3_power(p.f.form, a, slab.points + 3 * static_cast<std::size_t>(t));
          return (power < 0 ? 1U : 0U) | (power == 0 ? 2U : 0U);
        }, inside, zero);
      }
      if (!c.rejected) {
        ++census_chunks;
        q3_census_chunk(group, index, slab, sites, threshold3, base, inside, zero, c);
      }
    }
    ++seeds;
    q3_census_count(c, census);
    if (c.rejected) {
      ++census.depth_rejections;
    } else {
      if (c3 + c4 == capacity) return LanesFused::fallback;
      if (group.leader())
        q3_record(p.f.form, a, id_a, id_b, index.rank_ids[slab.ranks[seed]], c, slab.records[capacity - 1 - c3]);
      ++c3;
      ++census.emitted;
      census.shell_ids += c.shell;
    }
    if (pass) {
      const LanesSlab below{slab.ranges, slab.points, slab.ranks, slab.seeds, slab.scratch, slab.records,
                            slab.capacity, capacity - c3};
      const auto status = q4_seed_finish(group, index, a, b, id_a, id_b, seed, below, sites, kmax, q4, p, c4, w4);
      if (status != CertificateStatus::decided) {
        if (c4 == capacity - c3) return LanesFused::fallback;  // the slab is full: the unfused rule decides
        q4_open = false;
        task.q4 = c4;
        task.fail_phase = 4;
        task.fail_kind = static_cast<u8>(status);
      }
    }
  }
  task.q3 = c3;
  if (q4_open) task.q4 = c4;
  task.layout = 1;
  group.sync();  // the leader's records
  if (group.leader()) add_fused(fused, LanesFusedWork{seeds, chunks, q3_chunks, census_chunks, 0});
  return LanesFused::done;
}

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
                        LanesTask& task, Q4Work& w4, LanesTaskWork& slot, LanesFusedWork& fused,
                        bool fused_pass = true) {
  task.q3 = task.q4 = 0;
  task.fail_phase = task.fail_kind = task.layout = 0;
  if (group.leader()) w4 = Q4Work{};
  group.sync();
  u32 count = 0;
  u64 steps = 0;
  if (fused_pass && lanes == 6) {  // L15: both lanes in one scan per seed
    EdgeQ3Work census{};
    const LanesFused outcome =
        lanes_task_fused(group, index, a_rank, b_rank, kmax, slab, sites, first, last, q4, task, w4, census, fused);
    if (outcome != LanesFused::fallback) {
      steps = lanes_census_steps(census);
      if (group.leader()) lanes_add_census_work(slot, census);
      if (outcome == LanesFused::done) {
        group.sync();  // the leader's q4 work
        if (group.leader()) lanes_add_q4_work(slot, w4);
        steps += lanes_q4_steps(w4);
      }
      group.sync();
      return steps;
    }
    // The records did not fit: the task again, unfused, from a clean state.
    task.q3 = task.q4 = 0;
    task.fail_phase = task.fail_kind = task.layout = 0;
    if (group.leader()) {
      w4 = Q4Work{};
      ++fused.fallbacks;
    }
    group.sync();
  }
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

// T, end of a task (leader, table index t): its records per phase for the
// C scans, and its failure into its edge's first failure (order-free
// minimum: the result does not depend on which task ends first).
MHGP9_HD inline void lanes_task_publish(const LanesTask& task, u64 t, LanesTaskWork& slot, LanesTaskRecords& records) {
  records.q3 = task.q3;
  records.q4 = task.q4;
  if (task.fail_phase != 0) atomic_min_u64(slot.fail_first, lanes_fail_position(task.fail_phase, t));
}

// ---- C ---------------------------------------------------------------------

// C step v2 (lanes plan step 3, 24 septembre 2026): the replay and the
// placement of an edge read O(1) values instead of looping over its tasks
// (the loops made a heavy edge with thousands of tasks the tail of C on G4:
// 32 ms at 08/000000 K5). `scan` is the inclusive scan, in table order, of
// the tasks' LanesTaskRecords; the edge's tasks are [first, first + count).
//
// lanes_replay_scan equals lanes_replay (the sequential witness below, gate
// mhgp9_gpu_lanes_port): let i_f be the first failure in the sequential
// order (q3 items, then q4 items, each in task order) and prefix(i) the
// records of the items up to i (included). The witness returns deferred at
// the first item whose prefix exceeds record_capacity, if it comes before
// or at i_f, else the kind of i_f (else decided with the total). The prefix
// never decreases, so an item i <= i_f with prefix(i) > capacity exists iff
// prefix(i_f) > capacity: the status is a function of i_f (the minimum of
// lanes_fail_position, the order of edge_lanes) and prefix(i_f) (two scan
// reads). Unasked phases have neither records nor failures.
MHGP9_HD inline CertificateStatus lanes_replay_scan(const LanesTask* tasks, const LanesTaskRecords* scan, u64 first,
                                                    u32 count, u64 fail_first, u32 record_capacity, u32& records) {
  records = 0;
  if (count == 0) return CertificateStatus::decided;  // no seed: no task, no record
  const LanesTaskRecords base = first == 0 ? LanesTaskRecords{0, 0} : scan[first - 1];
  const LanesTaskRecords end = scan[first + count - 1];
  const u64 q3 = end.q3 - base.q3;
  if (fail_first == lanes_no_failure) {
    const u64 total = q3 + (end.q4 - base.q4);
    if (total > record_capacity) return CertificateStatus::deferred;
    records = static_cast<u32>(total);
    return CertificateStatus::decided;
  }
  const u64 t = fail_first & 0xffffffffULL;  // table index of the failing task
  const LanesTaskRecords at = scan[t];
  const u64 prefix = (fail_first >> 32) == 0 ? at.q3 - base.q3 : q3 + (at.q4 - base.q4);
#if defined(MHGP9_LANES_TASKS_MUTANT_FAULT_FIRST)
  // mutant: the failure's kind wins over a record overflow met before it
  if (tasks[t].fail_kind == static_cast<u8>(CertificateStatus::fault)) return CertificateStatus::fault;
#endif
  if (prefix > record_capacity) return CertificateStatus::deferred;
  return static_cast<CertificateStatus>(tasks[t].fail_kind);
}

// An edge's answer after the arena rule: decided by the replay and its
// records within the arena at its exclusive prefix (counter always
// advanced, edge order).
MHGP9_HD inline bool lanes_edge_kept(u8 replay_status, u64 prefix, u64 records, u64 arena_capacity) {
  return replay_status == static_cast<u8>(CertificateStatus::decided) && prefix + records <= arena_capacity;
}

// Where the records of the task at table index t go, for a decided edge
// whose slice begins at `begin`: all the q3 records of the edge in task
// order, then all its q4 records in task order (edge_lanes' order).
MHGP9_HD inline void lanes_task_destinations(const LanesTaskRecords* scan, const LanesTask& task, u64 first,
                                             u32 count, u64 t, u64 begin, u64& to3, u64& to4) {
  const LanesTaskRecords base = first == 0 ? LanesTaskRecords{0, 0} : scan[first - 1];
  const LanesTaskRecords end = scan[first + count - 1];
  const LanesTaskRecords at = scan[t];
#if defined(MHGP9_LANES_TASKS_MUTANT_COMPLETION_ORDER)
  // mutant: each task's records placed whole, in the reverse table order
  // (the order in which the emulation completes them)
  static_cast<void>(base);
  to3 = begin + (end.q3 + end.q4) - (at.q3 + at.q4);
  to4 = to3 + task.q3;
#elif defined(MHGP9_LANES_TASKS_MUTANT_SEGMENTS_SWAPPED)
  to4 = begin + (at.q4 - task.q4 - base.q4);  // mutant: the q4 segment first
  to3 = begin + (end.q4 - base.q4) + (at.q3 - task.q3 - base.q3);
#else
  to3 = begin + (at.q3 - task.q3 - base.q3);
  to4 = begin + (end.q3 - base.q3) + (at.q4 - task.q4 - base.q4);
#endif
}

// The sequential witness of lanes_replay_scan (gate mhgp9_gpu_lanes_port
// compares both on engraved tables and on every real edge): the status of
// an edge whose prologue was decided, from its tasks in table order, as the
// sequential edge_lanes: the q3 phase (tasks in order), then the q4 phase;
// in each segment the records come before the task's failure. The records
// of the edge overflow the record slab as soon as their running total
// exceeds record_capacity (deferred); otherwise the first failure met
// decides. `records` receives the edge's total (decided).
MHGP9_HD inline CertificateStatus lanes_replay(const LanesTask* tasks, u32 count, u8 lanes, u32 record_capacity,
                                               u32& records) {
  records = 0;
  u64 total = 0;
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
