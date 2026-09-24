#pragma once

#include "lanes/q4_local.hpp"
#include "lanes/q4_seed_cells.hpp"
#include "lanes/q4_window.hpp"
#include "lanes/q34_witness_search.hpp"
#include "lanes/q3_ball_census.hpp"
#include "lanes/q34_dead_lanes.hpp"
#include "wspd/front.hpp"
#include "../../common/raw_vector.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace mhgp9::gen {

enum class WspdQ4Backend { Local28, Window30 };
enum class WspdQ34WitnessMode { Disabled, Pair, RectanglePair };
enum class WspdQ3CensusMode { ScalarCover, GlobalBoxes };

struct WspdQ34Options {
  WspdFrontMode front_mode{WspdFrontMode::MidpointSamples};
  std::uint8_t requested_lane_mask{6};  // Exactly 2=q3, 4=q4 or 6=both.
  WspdQ4Backend q4_backend{WspdQ4Backend::Local28};
  Q4LocalOptions local{};
  WspdQ34WitnessMode witness_mode{WspdQ34WitnessMode::Disabled};
  WspdQ3CensusMode q3_census_mode{WspdQ3CensusMode::ScalarCover};
  Q34WitnessBoundsMode witness_bounds_mode{Q34WitnessBoundsMode::Legacy};
  Q4SeedCellOptions q4_seed_cells{};
  // Explicit option: when both lanes are active on Local28 (K>=3), build the
  // q4 center atlas BEFORE the q3 lane and reject without census every q3
  // seed whose circumcenter lies in a cell certified with >=K-1 cover sites
  // strictly inside all its balls; a root certificate skips the q3 lane.
  // Exact (the cover holds every site inside such balls; certificates hold
  // on closed cells); the q4 sweep then reuses the same atlas. Off keeps the
  // historical order and every counter bit-identical.
  bool q3_atlas_consultation{false};
  // v9 option (requires q3_atlas_consultation): a q3 seed whose centre lies in
  // an EXACT atlas leaf is censused from that leaf (certified count + frontier
  // sites) instead of the global index. Same depth and shell; off keeps the
  // v8 path and every historical counter.
  bool q3_leaf_census{false};
  // v9 option: after the witness filter and the cover build, an exact
  // dead-lane certificate (lanes/q34_dead_lanes.hpp) removes a lane whose
  // whole center disk is covered by cells with >=T uniform strict interiors
  // (T=K-1 for q3, K-2 for q4). Such a lane emits nothing on the exact path
  // either: same stream. Off keeps the v8 path and every historical counter.
  bool dead_lanes{false};
  // v9 option (requires dead_lanes): the certificate is first attempted on
  // the closed DIAMETRAL ball of the edge (Q34EdgeCover::make_diametral), a
  // subset of the cover; the cover is built, and the certificate rerun on
  // it, only for the lanes this core leaves open. A subset can only remove
  // credits, never add one: same proved lanes are sound, same stream.
  bool dead_core{false};
  // v9 option (Affine bounds only): the pair filter first re-tests, for the
  // same endpoint a, the witness nodes admitted by the previous full search
  // (lanes/q34_witness_search.hpp): lanes they prove rejected skip the
  // search. Same surviving masks, same stream. Off keeps every v8 counter.
  bool pair_witness_cache{false};
  // Parallel entry only: every surviving residual rectangle is published to
  // the team's bounded task queue, by ranges of a-ranks when its pair mass
  // exceeds this grain, so that any idle worker expands it; the discovering
  // worker expands inline only what the full queue refuses. Whole edges are
  // never split. Zero disables sharing (historical Coarse jobs only).
  std::size_t parallel_task_pairs{256};
  // Pending tasks admitted before publishers fall back to inline expansion
  // (bounded memory, never a search or output quota).
  std::size_t parallel_queue_capacity{4096};
  // Parallel entry only: the front job plan splits the pending product of
  // largest pair mass first (make_wspd_front_jobs mass_first) and jobs are
  // claimed by decreasing mass. Scheduling only: same rectangles, same
  // normalized stream and geometric work; the serial preparation, the
  // job/worker split and the witness-cache hits per worker differ.
  bool jobs_by_mass{false};
};

struct WspdQ3AtlasWork {
  u64 edges_with_atlas{}, root_lane_skips{}, locations{}, outside_domain{}, rejections{};
  // v9: q3 census started from the exact atlas leaf containing the centre
  // (certified count + the leaf frontier only) instead of the global index.
  u64 leaf_censuses{}, leaf_point_tests{}, leaf_rejections{}, lower_bound_fallbacks{};
  bool operator==(const WspdQ3AtlasWork&) const = default;
};

struct WspdQ34WitnessWork {
  // Input union mass, then whole-product and individual-pair rejections.
  // Lane masses overlap; they must NEVER be summed as a union mass.
  u64 input_pair_mass{}, rejected_rectangles{}, rectangle_pair_mass{};
  u64 rectangle_q3_pairs{}, rectangle_q4_pairs{};
  u64 rejected_pairs{}, pair_q3_pairs{}, pair_q4_pairs{};
  // v9 witness-node cache: pairs rejected without a search (every open lane
  // by cached nodes); pairs.queries counts only the searches actually run.
  u64 cache_rejected_pairs{};
  Q34WitnessSearchWork rectangles, pairs;
  Q34WitnessBoundsWork rectangles_bounds, pairs_bounds;
  bool operator==(const WspdQ34WitnessWork&) const = default;
};

struct WspdQ3Work {
  u64 edge_queries{};
  u64 seed_node_visits{}, seed_bound_tests{}, seed_point_tests{};
  u64 seed_rejected_nodes{}, seed_split_nodes{}, seed_rejected_sites{};
  u64 acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{}, ball_builds{};
  u64 census_range_visits{}, census_point_tests{};
  u64 census_inside_sites{}, census_outside_sites{}, census_shell_sites{};
  u64 depth_rejections{}, early_unread_sites{};
  u64 shell_sort_comparisons{}, shell_ids{}, emitted{}, peak_shell_bytes{};
  bool operator==(const WspdQ3Work&) const = default;
};

// v9 S4a: declared ledger of the q3 lanes of a batch call (gpu/lanes.hpp,
// gpu::Q3Work field by field), never compared with the engine's q3 ledger:
// the rebuilt cover, the seed scan of the cover sites, and the ScalarCover
// censuses (site-parallel, exact stopping site).
struct Q34LanesWork {
  u64 edges{}, cover_sites{}, max_cover_sites{};
  Q34EdgeCoverWork cover;
  u64 seed_tests{}, acute_sites{}, owner_rejections{}, seeds{};
  u64 q3_edges{}, census_seeds{};  // S4b: edges whose q3 lane ran, and their seeds
  u64 census_point_tests{}, census_inside_sites{}, census_shell_sites{}, census_outside_sites{};
  u64 depth_rejections{}, emitted{}, shell_ids{};
  bool operator==(const Q34LanesWork&) const = default;
};

// v9 S4b: declared ledger of the q4 lanes of a batch call (gpu::Q4Work
// field by field): lens pass, survivor stage, groups and emissions apart.
struct Q34Lanes4Work {
  u64 edges{}, seeds{}, certified{}, certified_chunk1{}, survivors{}, pass_chunks{}, pass_site_tests{};
  u64 buffered_events{}, max_buffered{}, live_buckets{}, filter_steps{}, bucket_events{}, candidates{};
  u64 foreign_candidates{}, groups{}, compare_steps{}, depth_rejected_groups{}, positivity_tests{};
  u64 groups_without_valid{}, emitted{}, emitting_seeds{}, multi_emission_seeds{}, max_emissions_per_seed{};
  u64 shell_ids{}, max_group{}, constant_shell_sites{};
  u64 list_steps{}, group_steps{};  // every chunk pass of the survivor stage
  bool operator==(const Q34Lanes4Work&) const = default;
};

struct WspdQ34Work {
  u64 input_rectangles{}, expanded_pairs{}, q3_edges{}, q4_edges{}, both_edges{};
  u64 cover_builds{}, cover_sites{}, max_cover_sites{}, peak_cover_bytes{};
  u64 q3_emitted{}, q4_emitted{}, payload_shell_ids{};
  // Cover + reused q3 shell + the current q4 edge's own capacity peak.
  // Excludes front stack, shared cloud/index, fixed objects, allocator
  // transients and all storage/work added by the consumer. Not RSS.
  u64 peak_edge_buffer_bytes{};
  Q34EdgeCoverWork cover;
  WspdQ3Work q3;
  // Only the selected backend runs. Work is summed across edges, except
  // capacities, maxima and retained_bytes fields, which combine by MAX.
  Q4LocalEdgeWork local;
  Q4WindowEdgeWork window;
  WspdQ34WitnessWork witness;
  Q3BallCensusWork q3_blocks;
  Q4SeedCellWork q4_seed_cells;
  WspdQ3AtlasWork q3_atlas;
  // v9 dead-lane certificate; a proved lane is counted here, not in
  // q3_edges/q4_edges (lane mass identities include q3_proved/q4_proved).
  Q34DeadLaneWork dead;
  Q34WitnessCacheWork witness_cache;
  // v9 diametral core of the certificate (dead_core): its own cover work and
  // prover work. An edge closed by the core builds no cover:
  // expanded = cover_builds + core_closed_edges + rejected pairs.
  u64 core_builds{}, core_sites{}, core_closed_edges{};
  Q34EdgeCoverWork core_cover;
  Q34DeadLaneWork dead_core;
  // v9 S4a: the q3 lanes decided by a batch call (run_wspd_q34_batched),
  // their own declared ledger; q3_edges, both_edges, q3_emitted and
  // payload_shell_ids include them, the engine's q3 ledger does not.
  Q34LanesWork lanes;
  // v9 S4b: the q4 lanes decided by the same call; q4_edges, q4_emitted and
  // payload_shell_ids include them, the engine's q4 ledgers do not.
  Q34Lanes4Work lanes4;
};

struct WspdQ34Result {
  WspdFrontResult front;
  WspdQ34Work work;
};

// Global q3/q4 CANDIDATE STREAM over this immutable cloud/index, not a
// deduplicated ball catalogue, interior-ID payload, HGP forest or FULL tower.
// The WSPD covers every active lane independently. Each residual rectangle
// is explicitly expanded once; its original-ID edge builds ONE cover shared
// by q3 and q4. No accepted q2 or q3 gate gives access to another lane, and
// no front witness credit seeds a census. The q2-only front proposal options
// are deliberately not exposed here: the qualified multilanes use defaults.
//
// q3-only: exact box-generated acute owner seeds, then a saturating covered
// census at K-1, collecting/sorting the complete shell on acceptance. Its
// buffer is private and reused between seeds AND edges. There is no hidden
// q4 sweep in this q3 branch. q4: unchanged Local28 by default, Window30 only
// when explicitly selected, at depth<K-2 with their canonical support rules.
// Positive/owner certification transfers every published depth/shell to the
// full cloud. Different presentations/edges may still emit the same ball.
//
// K must be in 1..10, s positive, mask2/4/6, valid enums/local options and
// nonnull index/consumer. Validation precedes any output, including when no
// requested lane is available. K1 (or K2 with q4-only) returns an empty stream
// and zero work, with total_unordered_pairs metadata still checked for u64.
// This bound on K is the existing front contract, not a search/output quota.
//
// Sequential callbacks borrow candidate/spans for the callback duration only.
// The index is owned throughout even if the caller resets its handle. Calls
// have independent mutable engines; nested calls require no shared state.
// Allocation/counter/callback exceptions propagate; earlier emissions remain
// with the caller and no successful partial ledger is returned.
//
// Total work includes the front, sum_rect |A||B| explicitly expanded edges,
// cover preparation per edge, seed generation, q3 sum_seed cover reads/shell
// sorts, and the selected q4 backend. cover_sites is their paid Fcover sum,
// not a distinct-site count. No global subquadratic or performance guarantee
// follows. No complete edge/face list, cloud copy or per-edge index is built.
//
// Explicit witness modes first search the SAME index for strict universal
// citron witnesses, independently at K-1/K-2. RectanglePair searches products
// before expansion; both modes search remaining pairs BEFORE building covers.
// Partial counts are discarded, never used to seed another search or census.
// expanded_pairs counts pairs entered after whole-product filtering;
// q3_edges/q4_edges/both_edges and cover_builds count only surviving lanes.
// The separate witness ledger pays every search and partitions input masses.
// GlobalBoxes is a separate explicit q3 option: census on the global index,
// saturating strict interior first, complete shell only on acceptance. No
// witness credit seeds it. q3.census_* and early_unread_sites then stay zero;
// q3_blocks reports the actual work, including precomputed unvisited bounds.
// Exclusion adds a certified lower-Xi exclusion of WITNESS nodes; Affine
// additionally specializes H/Xi when both endpoint boxes are singletons.
// Such exclusions remove only local DFS lanes and supply ZERO credits:
// they never reject an edge/lane without its separate saturation proof.
// Legacy preserves the previous traversal; the new bounds ledgers stay zero.
// No witness-state inheritance between rectangles/pairs is introduced here.
// Explicit Local28-only LiveOnly/Joined seed-cell traversals keep every live
// incidence/contact and use private bounded caches, not accepted-q3 gates.
// Their additional work is separate; Individual keeps the historical path.
// Non-Individual + Window30, zero block_sites and invalid modes are rejected
// before output, including inactive lanes. This does not split an edge across
// workers: only the traversal within its synchronous call changes.
[[nodiscard]] WspdQ34Result run_wspd_q34_candidates(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdQ34Options options, const Q34SeedConsumer& consumer);

using WspdQ34ParallelConsumer =
    std::function<void(std::size_t, const Q34SeedCandidate&)>;

struct WspdQ34WorkerWork {
  u64 jobs{}, front_products{}, input_rectangles{}, expanded_pairs{};
  u64 q3_emitted{}, q4_emitted{}, peak_edge_buffer_bytes{};
  bool operator==(const WspdQ34WorkerWork&) const = default;
};

struct WspdQ34ParallelWork {
  u64 requested_workers{}, started_workers{}, target_jobs{}, jobs{};
  u64 completed_jobs{}, terminal_jobs{}, prefix_product_visits{};
  u64 job_storage_bytes{}, worker_state_bytes{}, callback_storage_bytes{};
  // SUM of private worker peaks, an upper bound, not a simultaneous peak.
  // Fixed engines/front stacks, callback target allocations, shared cloud,
  // allocator/thread metadata and caller-owned payload/RSS are excluded.
  u64 edge_buffer_bytes_sum{};
  bool operator==(const WspdQ34ParallelWork&) const = default;
};

// Rectangle-range task sharing between the joined workers (SUM fields, plus
// maxima). published = ranges handed to the queue; consumed = ranges taken
// from it (by any worker, the publisher included); task_pairs = their pair
// mass; peak_queue = simultaneously pending tasks; waits = idle waits.
struct WspdQ34TaskWork {
  u64 split_rectangles{}, published{}, consumed{}, task_pairs{}, peak_queue{}, waits{}, refused{};
  bool operator==(const WspdQ34TaskWork&) const = default;
};

// Measured, never logical: wall time of the slot's loop, CPU time of its
// thread (CLOCK_THREAD_CPUTIME_ID) and wall time spent waiting on the task
// queue. Nanoseconds; they vary between runs and are never compared.
struct WspdQ34WorkerTiming {
  u64 wall_ns{}, cpu_ns{}, wait_ns{};
  // Wall time inside front jobs (their inline work: front, rectangle filter,
  // unpublished rectangles and edges), summed and longest single job.
  u64 job_ns{}, max_job_ns{};
};

struct WspdQ34ParallelResult {
  WspdQ34Result pipeline;
  WspdQ34ParallelWork parallel;
  std::vector<WspdQ34WorkerWork> workers;
  WspdQ34TaskWork tasks;
  std::vector<u64> worker_tasks;  // Ranges consumed per started slot.
  std::vector<WspdQ34WorkerTiming> worker_timings;
};

// Explicit Coarse front-job entry; the old mono entry/default is unchanged.
// One joined team, one engine/reused shell per started worker, one immutable
// index and job plan. A rectangle/edge/local28/window30 call is never split.
// jobs_per_worker controls initial job granularity, never search/output size.
// W1 follows the same jobs path. started_workers=min(requested_workers,jobs).
// Inactive lanes return metadata only, no job plan or workers; both positive
// worker_count/jobs_per_worker and their product are still validated first.
//
// Each started slot receives its own COPY of the callback before any worker
// starts. Callbacks within one slot are sequential; distinct slots may call
// concurrently and order is unspecified. Captured references/shared objects
// still need caller synchronization. The borrowed-view lifetime is unchanged.
// Index/job ownership extends through every join, even after caller reset.
// Allocation, callback, counter or launch failure cancels acquisition of new
// jobs, lets already running jobs finish, joins all started threads, then
// propagates. Partial emissions remain; no successful partial result exists.
//
// Front prefix + jobs and additive geometric counters equal the mono work.
// Reductions use SUM/MAX exactly as above; shell capacity and its coupled
// edge-buffer maximum may differ with worker assignment and are NOT logical
// work invariants. Orchestration/capacity sums are reported separately.
[[nodiscard]] WspdQ34ParallelResult run_wspd_q34_parallel(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdQ34Options options, std::size_t worker_count,
    const WspdQ34ParallelConsumer& consumer, std::size_t jobs_per_worker = 16);

// ---- v9 S2 (23 septembre 2026) : filtre témoin par lots.
//
// The front runs first and only collects its residual rectangles. One batch
// call then decides every rectangle (Affine bounds, as filter_q34_witnesses)
// and every pair of every surviving rectangle (point overload, NO cache).
// The workers finally run the rest of the edge (core, cover, certificate,
// q3/q4) on the surviving pairs only. Same decisions as the engine path (the
// engine's row cache never changes a final mask), hence the same candidate
// set; its order and the per-worker split differ.

// A surviving pair: spatial ranks of its endpoints (row-major order of its
// rectangle) and its surviving lanes (subset of 6, nonzero).
struct Q34SurvivingEdge {
  std::uint32_t a_rank{}, b_rank{};
  std::uint8_t mask{};
  bool operator==(const Q34SurvivingEdge&) const = default;
};

// Output of one batch call, in rectangle order.
struct Q34FilterBatch {
  std::vector<std::uint8_t> rectangle_masks;   // surviving lanes of each input rectangle
  std::vector<Q34SurvivingEdge> survivors;     // pairs with at least one lane left
  u64 expanded_pairs{};                        // sum of |A||B| over surviving rectangles
  u64 pair_q3_rejected{}, pair_q4_rejected{};  // lanes rejected among the expanded pairs
  u64 rectangle_visits{}, pair_visits{};       // node visits of the two witness DFS families
  std::string backend;                         // "cpu" or the device name
};

// Implementations: run_q34_filter_batch_cpu below (reference), a device one
// supplied by the caller. Must return exactly the decisions of the reference
// or throw; the result is cross-checked for shape before use.
using Q34BatchFilter = std::function<Q34FilterBatch(const Q2CensusIndex& index, unsigned kmax,
                                                    std::span<const WspdRectangle> rectangles)>;

// CPU reference of the batch call, `workers` threads, dynamic blocks.
[[nodiscard]] Q34FilterBatch run_q34_filter_batch_cpu(const Q2CensusIndex& index, unsigned kmax,
                                                      std::span<const WspdRectangle> rectangles,
                                                      std::size_t workers);

// ---- v9 S3 (23 septembre 2026) : certificats de voie morte par lots.
//
// After the batch filter, one call decides the dead-lane certificate of
// every survivor, exactly as Engine::filtered_edge: the diametral core and
// its certificate (dead_core), then the cover and its certificate for the
// lanes it leaves open. The workers then run only the q3/q4 generation of
// the lanes left open (the cover is rebuilt for it, uncounted). A DEFERRED
// survivor (not decided by the call, e.g. a device memory limit) runs the
// whole engine edge on the workers instead: same decisions, same counters.
struct Q34CertificateBatch {
  std::vector<std::uint8_t> masks;     // per survivor: lanes left open (its own mask when deferred)
  std::vector<std::uint8_t> deferred;  // per survivor: 1 when not decided by the call
  // Certificate work of the decided survivors, the WspdQ34Work fields.
  u64 core_builds{}, core_sites{}, core_closed_edges{};
  Q34EdgeCoverWork core_cover;
  Q34DeadLaneWork dead_core;
  u64 cover_builds{}, cover_sites{}, max_cover_sites{};
  Q34EdgeCoverWork cover;
  Q34DeadLaneWork dead;
  std::string backend;  // "cpu" or the device name
};

// Implementations: run_q34_certificate_batch_cpu below (reference), a device
// one supplied by the caller. Must return exactly the reference decisions and
// work for its decided survivors, or throw; the batch path checks shape and
// the lane/work identities before use.
using Q34CertificateFilter = std::function<Q34CertificateBatch(
    const Q2CensusIndexPtr& index, unsigned kmax, bool dead_core, std::span<const Q34SurvivingEdge> survivors)>;

// CPU reference: product core/cover/prover per survivor, `workers` threads.
[[nodiscard]] Q34CertificateBatch run_q34_certificate_batch_cpu(const Q2CensusIndexPtr& index, unsigned kmax,
                                                                bool dead_core,
                                                                std::span<const Q34SurvivingEdge> survivors,
                                                                std::size_t workers);

// Judge of a certificate call (auditor B, before any device qualification):
// every survivor the call DECIDED is recomputed by the CPU reference; each
// decided mask and the summed work of the decided survivors must be equal,
// else std::logic_error (no answer is used). Deferred survivors are left to
// the workers' engine edges. `work` (may be null) receives the counts.
struct Q34CertificateJudgeWork {
  u64 judged{}, deferred{};
};
[[nodiscard]] Q34CertificateFilter judge_certificate_filter(Q34CertificateFilter inner, std::size_t workers,
                                                            Q34CertificateJudgeWork* work);

// ---- v9 S4a (23 septembre 2026) : voie q3 des survivants certifiés, par lots.
//
// After the certificates (S3), one call generates the q3 lane of every
// certified survivor whose q3 lane is left open (`asked`), without atlas:
// the seeds are drawn from the cover and every census is the exact
// ScalarCover census (gpu/lanes.hpp). The workers run the other lanes (q4)
// meanwhile, then the q3 lanes the call did not decide (deferred: a memory
// decision only). The emitted balls come back as records, without shell IDs.

// One emitted q3 ball (gpu::LaneRecord): primitive key {A, Bx, By, Bz, C},
// sorted support IDs (support[3] = UINT32_MAX), strict interior count, shell
// size and fingerprint (wrapping sum and xor of q34_shell_hash of its IDs).
// Trivially default constructible (v9, 24 septembre 2026): a batch's records
// are allocated without zero fill (RawVector) and written field by field;
// every other construction value-initialises (`Q34LaneRecord r{}`).
struct Q34LaneRecord {
  std::array<i128, 5> key;
  std::array<std::uint32_t, 4> support;
  std::uint32_t edge;  // survivor ordinal
  std::uint32_t depth, shell;
  std::uint8_t arity;
  u64 shell_sum, shell_xor;
  bool operator==(const Q34LaneRecord&) const = default;
};
static_assert(std::is_trivially_default_constructible_v<Q34LaneRecord>);

// SplitMix64 finalizer (gpu::mix64): the shell fingerprint of one input ID.
[[nodiscard]] inline u64 q34_shell_hash(u64 x) noexcept {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

struct Q34LanesBatch {
  std::vector<std::uint8_t> decided;        // per survivor: lanes decided by the call (subset of asked)
  std::vector<std::uint32_t> record_begin;  // per survivor: first record of its slice
  std::vector<std::uint32_t> record_count;  // per survivor: its records (0 unless decided)
  RawVector<Q34LaneRecord> records;         // the slices, in any order of the edges (q3 and q4)
  Q34LanesWork work;                        // decided edges only (prologue and q3 lanes)
  Q34Lanes4Work work4;                      // decided edges only (S4b q4 lanes)
  std::string backend;                      // "cpu" or the device name
};

// Implementations are supplied by the caller (host emulation or device of
// gpu/lanes.hpp). Must return exactly the q3 lane of every edge it decides,
// or throw; check_lanes_batch runs before any record is used.
using Q34LanesFilter = std::function<Q34LanesBatch(const Q2CensusIndexPtr& index, unsigned kmax,
    std::span<const Q34SurvivingEdge> survivors, std::span<const std::uint8_t> asked)>;
// Receives checked records, one chunk per call; concurrent calls come from
// distinct worker slots.
using Q34RecordSink = std::function<void(std::size_t slot, std::span<const Q34LaneRecord> records)>;

struct Q34LanesStage {
  Q34LanesFilter filter;
  Q34RecordSink sink;
  bool concurrent = false;   // the call runs on its own thread while the workers run the other lanes
  std::uint8_t lanes = 2;    // lanes asked of the call: 2 (q3, S4a) or 6 (q3 and q4, S4b)
};

// Trust boundary of a lanes call: shapes, decided lanes inside `asked`, an
// exact partition of the records into the decided edges' slices, records
// well formed (arity 3 from a decided q3 lane: sorted support holding the
// edge, depth below K-1, shell >= 3; arity 4 from a decided q4 lane: four
// sorted IDs holding the edge, depth below K-2, shell >= 4; A > 0), and the
// ledger identities binding the work to them.
void check_lanes_batch(const Q34LanesBatch& batch, const Q2CensusIndex& index, unsigned kmax,
                       std::span<const Q34SurvivingEdge> survivors, std::span<const std::uint8_t> asked);

// The engine's own q3 lane of one edge (original IDs), as records: the
// Engine of the batch path with the q3 lane only (no atlas is built for a
// q3-only lane, so the product options give the GlobalBoxes census). `q3`
// (may be null) receives the engine's q3 ledger of this edge.
[[nodiscard]] std::vector<Q34LaneRecord> engine_q3_records(const Q2CensusIndexPtr& index, unsigned kmax,
                                                           const WspdQ34Options& options, std::size_t a,
                                                           std::size_t b, WspdQ3Work* q3 = nullptr);
// S4b: the engine's own q4 lane of one edge (Engine with the q4 lane only:
// the product Local28/LiveOnly lane with its atlas).
[[nodiscard]] std::vector<Q34LaneRecord> engine_q4_records(const Q2CensusIndexPtr& index, unsigned kmax,
                                                           const WspdQ34Options& options, std::size_t a,
                                                           std::size_t b);

// Judge of a lanes call (before any device qualification): every lane the
// call DECIDED is recomputed by engine_q3_records / engine_q4_records; per
// edge and arity, the multisets of (arity, support, key, depth, shell size,
// shell fingerprint) must be equal, and the summed q3 seeds and emissions;
// else std::logic_error.
struct Q34LanesJudgeWork {
  u64 judged{}, deferred{}, records{};
};
[[nodiscard]] Q34LanesFilter judge_lanes_filter(Q34LanesFilter inner, WspdQ34Options options, std::size_t workers,
                                                Q34LanesJudgeWork* work);

// Measured phases of the batch path (nanoseconds, never compared).
struct WspdQ34BatchTiming {
  u64 front_ns{}, filter_ns{}, certificate_ns{}, edges_ns{};
  u64 rectangles{}, survivors{}, deferred{};
  u64 rebuilt_covers{};  // covers rebuilt by the workers for certified edges (uncounted in the ledger)
  std::string backend, certificate_backend;
  // S4a: wall time of the lanes call, of the workers' wait for it after
  // their own lanes, and of the tail (deferred q3 lanes and record sink).
  u64 lanes_ns{}, lanes_wait_ns{}, tail_ns{};
  u64 lanes_asked{}, lanes_decided{}, lanes_deferred{}, lanes_records{};
  std::string lanes_backend;
};

// Requires witness_mode=RectanglePair and witness_bounds_mode=Affine (the
// only filter the batch call implements); the pair cache is not used.
// Ledger identities of validate_completion hold as on the engine path:
// witness.rectangles.queries=input_rectangles, witness.pairs.queries=
// expanded_pairs, cache counters zero. `timing` may be null. A nonnull
// `certificates` (S3, requires dead_lanes) decides the certificates of the
// survivors in one call before the workers. A nonnull `lanes` (S4a,
// requires `certificates`) generates the q3 lane of the certified survivors
// by one call; its records go to lanes->sink, never to `consumer`.
[[nodiscard]] WspdQ34ParallelResult run_wspd_q34_batched(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdQ34Options options, std::size_t worker_count,
    const WspdQ34ParallelConsumer& consumer, std::size_t jobs_per_worker,
    const Q34BatchFilter& filter, WspdQ34BatchTiming* timing,
    const Q34CertificateFilter* certificates = nullptr, const Q34LanesStage* lanes = nullptr);

}  // namespace mhgp9::gen
