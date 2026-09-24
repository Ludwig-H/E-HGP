// FULL horizontal forests and vertical birth maps from supplied COMPLETE EXACT
// ball censuses. No claim of WSPD completeness, archive authority, or speed.
#pragma once

#include <algorithm>
#include <array>
#include <cfenv>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>
#include <system_error>
#include <thread>
#include <numeric>
#include <unordered_map>

#include "anchor_meb.hpp"
#include "full_coverage_certificate.hpp"
#include "local_plateau.hpp"
#include "ball_data.hpp"
#include "../pipeline/census.hpp"
#include "../parallel/pool.hpp"
#include <optional>

namespace mhgp9::tower {

inline constexpr const char* kFullBallAuthority =
    "full_tower_relative_to_supplied_complete_exact_ball_censuses";
inline constexpr const char* kFullBallValidationAccounting =
    "declared_regular_support_plus_extra_anchor_meb_v1";
inline constexpr const char* kFullBallLotAccounting =
    "direct_unitary_lots_and_grouped_dsu_slots_v1";
inline constexpr const char* kFullBallLowerAccounting =
    "monotone_lower_edge_activation_ranked_union_find_v1";
inline constexpr const char* kFullBallResolverCacheAccounting =
    "exact_direct_mapped_seeded_facet_tokens_16n_per_order_v1";
inline constexpr const char* kFullBallStaticResolverAccounting =
    "static_exact_sort_unique_complete_population_seeds_after_exchange_v2";
inline constexpr const char* kFullBallResidenceAccounting =
    "release_dead_construction_before_population_copy_v1";
enum class FullBallStatus { kCompleteRelative, kInvalidInput, kResourceExhausted, kInvariantViolated };
struct FullBallStats {
  u64 records = 0, extra_records = 0, anchor_blocks = 0, regular_blocks = 0, extra_blocks = 0;
  u64 representatives = 0, anchor_hits = 0, key_lookups = 0;
  u64 intruder_queries = 0, intruder_nodes = 0, intruder_power_tests = 0, interior_ranges = 0;
  u64 same_radius_steps = 0, descending_steps = 0, max_chain_steps = 0;
  u64 births = 0, merges = 0, contributions = 0, inert_blocks = 0;
  u64 declared_support_checks = 0, singleton_lots = 0, grouped_lots = 0, lot_dsu_slots = 0;
  u64 lower_edges_indexed = 0, lower_nodes_activated = 0, lower_edges_activated = 0;
  u64 lower_queries = 0, lower_find_steps = 0, lower_path_writes = 0;
  u64 resolver_cache_queries = 0, resolver_cache_hits = 0, resolver_cache_stores = 0;
  u64 resolver_cache_evictions = 0, resolver_cache_seed_stores = 0;
  // Allocated capacity, retained for accounting after release (not live bytes).
  u64 resolver_cache_slots = 0, resolver_cache_reset_slots = 0, resolver_cache_bytes = 0;
  u64 resolver_cache_released_slots = 0;
  std::array<u64, kFacetMaxK + 1> static_requests{}, static_unique{}, static_seeded{};
  // Actual post-exchange lookup attempts, exact hits and returned terminals.
  // Initial seeds stay in static_seeded; shortcut terminals pay no anchor hit.
  std::array<u64, kFacetMaxK + 1> static_post_seed_queries{}, static_post_seed_hits{}, static_post_seed_terminals{};
  u64 static_workers_created = 0, static_peak_request_bytes = 0, static_peak_target_bytes = 0;
  u64 static_lanes_used = 0, static_peak_retained_bytes = 0;
  u64 static_peak_seed_bytes = 0, static_peak_group_bytes = 0, static_peak_worker_bytes = 0;
  u64 static_batch_calls = 0, static_peak_batch_bytes = 0;
  u64 parallel_orders = 0;  // orders built by the concurrent static path
  u64 overlapped_orders = 0;  // of which with phase A overlapping phase 0 (overlap_static)
  u64 presorted_catalogues = 0;  // key order certified by one scan, no sort
  // v9 E4 (pipelined tail): orders whose population IDs, rows and vertical
  // images ran inside the overlapped window, and the contributions naming a
  // ball whose FIRST contributing order is lower (an extended shell that
  // contributes to several orders), named after the join.
  u64 pipelined_orders = 0, population_deferred_refs = 0;
  // False after a backend failure whose paid work could not be recovered.
  bool static_batch_work_known = true;
  AnchorMebWork validation_work, resolve_work;
};

// Optional, synchronous geometry-only seam. The external owner is bound once
// to the SAME immutable index and BallData storage, not merely equal sizes.
// BallIds always index balls in its original order. by_key is a borrowed lookup
// permutation owned by this build; no view may outlive the callback/run.
struct FullBallGeometryView {
  const CloudIndex& index;
  std::span<const BallData> balls;
  std::span<const u32> by_key;
};
using FullBallFacetKey = std::array<i32, kFacetMaxK>;
struct FullBallBatchSeed { FullBallFacetKey key; u32 ball; };
struct FullBallBatchRequest {
  FullBallFacetKey key;
  u32 consumer;  // before = geometry.balls[consumer].level, exactly
  u64 ordinal;  // earliest original occurrence; stable, not a sorted index
};
struct FullBallBatchView {
  unsigned k;
  std::span<const FullBallBatchSeed> seeds;  // whole I union U, sorted exact keys
  std::span<const FullBallBatchRequest> requests;  // sorted unique, initial seeds removed
};
struct FullBallBatchTarget { u32 ball; u64 ordinal; };
struct FullBallBatchResult {
  FullBallStatus status = FullBallStatus::kInvariantViolated;
  const char* reason = "full_ball_batch_uninitialized";  // static lifetime
  bool work_known = false;
  FullBallStats work;  // only geometric counters are merged, including Q/H/T
  u64 retained_bytes = 0;  // backend-owned sampled capacities, not RSS/VRAM peak
  // Transactional: empty on error; publish only after every admitted worker is
  // joined and every slot checked. Same order and ordinals as requests.
  std::vector<FullBallBatchTarget> targets;
};
struct FullBallBatchResolver {
  void* owner = nullptr;
  void (*resolve)(void*, const FullBallGeometryView&, const FullBallBatchView&,
                  FullBallBatchResult&) = nullptr;
};
// Witness of phase 0 for gates, never read by the product: per order K and
// per request ordinal, its static target and the ordinal of the FIRST
// request of its class (minimum ordinal of the requests of equal facet key).
// Firsts are recorded when the classes are known, targets at the end of the
// order's phase 0. path[K]: 0 while order K has no classes, 1 when they came
// from the sorted witness, 2 from the hashed grouping. tag_rejects and probes
// are schedule-dependent diagnostics of the hashed grouping (tag matches
// refused by the exact key compare, slots probed beyond the home slot),
// never work counters; seed_tag_rejects: the same refusals in the lookups of
// its seed index.
struct FullBallStaticTrace {
  std::array<std::vector<u32>, kFacetMaxK + 1> targets;
  std::array<std::vector<u64>, kFacetMaxK + 1> firsts;
  std::array<u64, kFacetMaxK + 1> path{}, tag_rejects{}, probes{}, seed_tag_rejects{};
};
// Named switches of the static tower path (review before R21: three sibling
// work series each added a positional bool right after overlap_static; a
// merge keeping one of them would compile with another lever on). An
// aggregate: a bare bool never converts to it.
struct FullBallTowerOptions {
  // Phase A of each order starts as soon as its phase 0 is done (phase 0 by
  // decreasing K): same objects and statuses.
  bool overlap_static = false;
  // v9 E4 (only with overlap_static): the population IDs (static offsets),
  // rows and vertical images of each order run on its runner inside the
  // overlapped window; false keeps them after the join (assign_populations,
  // the witness). Same objects, IDs and statuses.
  bool pipelined_tail = true;
  // Phase 0 groups its requests by exact hash classes; false keeps the
  // sorted witness (sort of the 56-byte requests). Same classes, first
  // requests, targets, counters and objects.
  bool hash_grouping = true;
  // Gates only: the static targets and class firsts of every order (never
  // read by the product).
  FullBallStaticTrace* trace = nullptr;
};
struct FullBallOrder {
  FullCoverageCertificate forest;
  // Image at the node's closed creation level. Queries normalize in the lower
  // history at their OWN cut. All entries are absent at K1.
  std::vector<FullNodeId> lower_nodes;
};
// Wall time of the phases in milliseconds (steady clock), measured, never
// compared: validation of the catalogue; static path: phase 0 (static
// targets, per K), phase A (lots, wall of the parallel step and per order on
// its own thread), phase B (population IDs), phase C (vertical images, same);
// sequential path: each order whole (order_by_k); both: bank and encoding
// (wall and per order). Arrays are indexed by K (entry 0 unused).
// v9 E4, pipelined tail (overlapped static path): phases B and C of order K
// run inside the window (B on a helper of its runner, C on the runner), so
// lots_ms is the window up to the end of the LAST phase A minus phase 0,
// populations_ms the exposed part of the window after it until the last
// population step ends, images_ms the rest of the window (the three are
// additive). images_by_k stays each order's image time INSIDE the images
// phase (the part of its step after the last population step; its whole
// step on the other paths); images_own_by_k and populations_by_k are each
// order's own steps (overlapped, never summed; zero on the other paths).
struct FullBallTimes {
  double validate_ms = 0, static_ms = 0, lots_ms = 0, populations_ms = 0, images_ms = 0, bank_ms = 0,
         encode_ms = 0;
  std::array<double, 11> static_by_k{}, lots_by_k{}, images_by_k{}, encode_by_k{}, order_by_k{};
  std::array<double, 11> populations_by_k{}, images_own_by_k{};
  // v9 E0 (plan of the tower judge): sub-timers, each inside its phase.
  // Validation: input and identity, key sort or presorted scan, key index,
  // pass 1, pass 2, level sort, level runs, programs and population slots.
  std::array<double, 8> validate_parts{};
  // Phase 0 of order K: collection and concatenation, the sorts (sorted
  // witness: requests and seeds; hashed grouping: no sort, the seed index),
  // the classes (witness: group starts; hashed: the exact hash classes in
  // smallest-site order), the resolution (targets of every class; hashed:
  // and their gather by request).
  std::array<double, 11> static_collect_by_k{}, static_sort_by_k{}, static_groups_by_k{}, static_resolve_by_k{};
};
struct FullBallTowerResult {
  FullBallStatus status = FullBallStatus::kInvalidInput;
  const char* reason = "full_ball_uninitialized";
  FullBallStats stats;
  FullBallTimes times;
  std::vector<FullBallOrder> orders;  // index K-1, no partial orders on failure
};

namespace full_ball_detail {
using BallId = u32;
constexpr u64 absent = kFullCoverageAbsent;
struct Failure { FullBallStatus status; const char* reason; };
using PhaseClock = std::chrono::steady_clock;
inline double ms_since(PhaseClock::time_point start) {
  return std::chrono::duration<double, std::milli>(PhaseClock::now() - start).count();
}
inline void require(bool ok, const char* reason, FullBallStatus status = FullBallStatus::kInvariantViolated) {
  if (!ok) throw Failure{status, reason};
}
#if defined(MHGP9_TESTING)
// Test failpoints, never compiled in a product target: bit K-1 makes order K
// fail at the end of its lots (A), of its population IDs (B, v9 E4) or of its
// vertical images (C). The sequential path checks the three at the end of
// order K, in this order.
inline std::atomic<u32> failpoint_lots{0}, failpoint_images{0}, failpoint_static{0}, failpoint_populations{0};
inline void failpoint(const std::atomic<u32>& mask, bool lots, unsigned k) {
  static constexpr const char* kLots[] = {"failpoint_lots_k1", "failpoint_lots_k2", "failpoint_lots_k3",
      "failpoint_lots_k4", "failpoint_lots_k5", "failpoint_lots_k6", "failpoint_lots_k7", "failpoint_lots_k8",
      "failpoint_lots_k9", "failpoint_lots_k10"};
  static constexpr const char* kImages[] = {"failpoint_images_k1", "failpoint_images_k2", "failpoint_images_k3",
      "failpoint_images_k4", "failpoint_images_k5", "failpoint_images_k6", "failpoint_images_k7",
      "failpoint_images_k8", "failpoint_images_k9", "failpoint_images_k10"};
  if (k >= 1 && k <= 10 && ((mask.load() >> (k - 1)) & 1U)) throw Failure{FullBallStatus::kInvariantViolated,
                                                                          lots ? kLots[k - 1] : kImages[k - 1]};
}
inline void failpoint_after_lots(unsigned k) { failpoint(failpoint_lots, true, k); }
inline void failpoint_after_images(unsigned k) { failpoint(failpoint_images, false, k); }
inline void failpoint_after_populations(unsigned k) {
  static constexpr const char* kPopulations[] = {"failpoint_populations_k1", "failpoint_populations_k2",
      "failpoint_populations_k3", "failpoint_populations_k4", "failpoint_populations_k5", "failpoint_populations_k6",
      "failpoint_populations_k7", "failpoint_populations_k8", "failpoint_populations_k9",
      "failpoint_populations_k10"};
  if (k >= 1 && k <= 10 && ((failpoint_populations.load() >> (k - 1)) & 1U))
    throw Failure{FullBallStatus::kInvariantViolated, kPopulations[k - 1]};
}
// Phase 0 (static targets) of order K, static path only.
inline void failpoint_after_static(unsigned k) {
  static constexpr const char* kStatic[] = {"failpoint_static_k1", "failpoint_static_k2", "failpoint_static_k3",
      "failpoint_static_k4", "failpoint_static_k5", "failpoint_static_k6", "failpoint_static_k7",
      "failpoint_static_k8", "failpoint_static_k9", "failpoint_static_k10"};
  if (k >= 1 && k <= 10 && ((failpoint_static.load() >> (k - 1)) & 1U))
    throw Failure{FullBallStatus::kInvariantViolated, kStatic[k - 1]};
}
// Exceptions other than Failure on the overlapped static path (v9 E4 fix):
// bit K-1 of failpoint_static_alloc makes phase 0 of order K throw
// std::bad_alloc (the static request arena failing), bit K-1 of
// failpoint_launch makes the launch of the runner of order K throw
// std::system_error. A nonzero failpoint_runner_pause_ms delays every runner
// between its phase A and its next write, so that a caller unwinding on such
// an exception has left its scope first; runner_pauses counts the delays.
inline std::atomic<u32> failpoint_static_alloc{0}, failpoint_launch{0}, failpoint_runner_pause_ms{0};
inline std::atomic<u64> runner_pauses{0};
inline void failpoint_static_allocation(unsigned k) {
  if (k >= 1 && k <= 10 && ((failpoint_static_alloc.load() >> (k - 1)) & 1U)) throw std::bad_alloc();
}
inline void failpoint_runner_launch(unsigned k) {
  if (k >= 1 && k <= 10 && ((failpoint_launch.load() >> (k - 1)) & 1U))
    throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
}
inline void failpoint_runner_pause() {
  if (const u32 ms = failpoint_runner_pause_ms.load()) {
    runner_pauses.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}
#else
inline void failpoint_after_lots(unsigned) {}
inline void failpoint_after_images(unsigned) {}
inline void failpoint_after_populations(unsigned) {}
inline void failpoint_after_static(unsigned) {}
inline void failpoint_static_allocation(unsigned) {}
inline void failpoint_runner_launch(unsigned) {}
inline void failpoint_runner_pause() {}
#endif
inline void add(u64& count, u64 amount = 1) {
  require(amount <= std::numeric_limits<u64>::max() - count, "full_ball_counter_overflow",
          FullBallStatus::kResourceExhausted);
  count += amount;
}
struct History {
  std::vector<ExactLevel> levels;
  std::vector<u64> next;
  u64 root_at(u64 token, const ExactLevel& cut, bool closed) const {
    require(token < levels.size() && full_coverage_detail::admitted(levels[token], cut, closed),
            "full_ball_anchor_not_born");
    while (next[token] != absent && full_coverage_detail::admitted(levels[next[token]], cut, closed))
      token = next[token];
    return token;
  }
};
// Level keys of a lower history: exact levels (sequential path, witness) or
// exact plateau ranks (static path, v9 E1: rank = level_run + 1 of the lot's
// catalogue level, 0 for the zero level of K1). level_run is an exact order
// isomorphism of the catalogue's levels (equal levels share a run) and the
// zero level lies below every catalogue level, so every closed or open cut
// answers the same on ranks as on exact levels (U320 compare replaced by u32).
namespace history_keys {
inline int compare(const ExactLevel& a, const ExactLevel& b) { return compare_exact_level(a, b); }
inline int compare(u32 a, u32 b) { return (a > b) - (a < b); }
inline bool valid(const ExactLevel& level) { return level.den > 0; }
inline bool valid(u32) { return true; }
template <class Key> inline bool admitted(const Key& level, const Key& cut, bool closed) {
  const int cmp = compare(level, cut);
  return cmp < 0 || (closed && cmp == 0);
}
}  // namespace history_keys

// Working view for chronologically increasing LOWER closed cuts. Component
// labels are separate from DSU representatives, so union by rank is permitted.
// The immutable source history still answers arbitrary historical export cuts.
template <class Key>
class MonotoneHistoryOf {
 public:
  MonotoneHistoryOf(std::span<const Key> levels, std::span<const u64> next_of, FullBallStats& stats)
      : levels(levels), next_of(next_of), st(stats),
      heads(levels.size(), absent), links(levels.size(), absent),
      parent(levels.size()), owner(levels.size()), rank(levels.size(), 0) {
    const size_t n = levels.size();
    require(next_of.size() == n, "full_ball_lower_history_shape");
    std::iota(parent.begin(), parent.end(), u64{0});
    std::iota(owner.begin(), owner.end(), u64{0});
    for (size_t node = 0; node < n; ++node) {
      require(history_keys::valid(levels[node]) && (!node ||
          history_keys::compare(levels[node - 1], levels[node]) <= 0),
          "full_ball_lower_history_chronology");
      const u64 next = next_of[node];
      if (next == absent) continue;
      require(next > node && next < n && history_keys::compare(levels[node], levels[next]) < 0,
          "full_ball_lower_history_edge");
      add(st.lower_edges_indexed);
      links[node] = heads[next]; heads[next] = node;
    }
  }

  u64 root_at(u64 token, const Key& cut, bool closed) {
    add(st.lower_queries);
    require(history_keys::valid(cut), "full_ball_lower_cut_domain");
    if (has_cut) {
      const int cmp = history_keys::compare(cut, previous_cut);
      require(cmp > 0 || (cmp == 0 && (closed || !previous_closed)), "full_ball_lower_cut_not_monotone");
    }
    has_cut = true; previous_cut = cut; previous_closed = closed;
#if defined(MHGP9_TOWER_MUTANT_RUNS_OPEN_CUT)
    if constexpr (std::is_same_v<Key, u32>) closed = false;  // mutant: rank cuts read as open
#endif
    while (cursor < levels.size() && history_keys::admitted(levels[cursor], cut, closed)) {
      add(st.lower_nodes_activated);
      u64 representative = cursor;
      for (u64 child = heads[cursor]; child != absent; child = links[child]) {
        add(st.lower_edges_activated);
        const u64 a = find(representative), b = find(child);
        require(a != b, "full_ball_lower_duplicate_component");
        representative = unite(a, b);
      }
      owner[representative] = cursor;
      ++cursor;
    }
    require(token < cursor, "full_ball_lower_anchor_not_born");
    return owner[find(token)];
  }

 private:
  std::span<const Key> levels;
  std::span<const u64> next_of;
  FullBallStats& st;
  std::vector<u64> heads, links, parent, owner;
  std::vector<u8> rank;
  size_t cursor = 0;
  Key previous_cut{};
  bool has_cut = false, previous_closed = false;

  u64 find(u64 token) {
    u64 root = token;
    while (parent[root] != root) { add(st.lower_find_steps); root = parent[root]; }
    while (parent[token] != token) {
      add(st.lower_find_steps);
      const u64 next = parent[token];
      if (next != root) { add(st.lower_path_writes); parent[token] = root; }
      token = next;
    }
    return root;
  }
  u64 unite(u64 a, u64 b) {
    if (rank[a] < rank[b] || (rank[a] == rank[b] && a > b)) std::swap(a, b);
    add(st.lower_path_writes); parent[b] = a;
    if (rank[a] == rank[b]) ++rank[a];  // rank <= log2(node count) <= 64
    return a;
  }
};
// Exact levels: the sequential path and the witness of the rank cursor.
class MonotoneHistory : public MonotoneHistoryOf<ExactLevel> {
 public:
  MonotoneHistory(const History& history, FullBallStats& stats)
      : MonotoneHistoryOf<ExactLevel>(history.levels, history.next, stats) {}
};
struct Draft {
  std::vector<FullCoverageBatch> batches;  // sequential path
  FullCoverageFlatDraft flat;              // static path (phase A): no allocation per action
  bool flat_form = false;
  std::vector<u64> lower_nodes;
};
struct Block {
  BallId ball;
  std::vector<u64> roots;
  u16 contribution = 0;
  bool interior = false;
};

struct ResolverCacheEntry {
  std::array<i32, kFacetMaxK> sites{};
  u64 token = absent;
};

// Optional exact memo, never an authority or a source of geometric rejection.
// Capacity is the next power of two >= 16*n, without a fixed cloud/work ceiling.
// Per-order reset bounds residence; collisions only evict, never alias keys.
class ResolverCache {
 public:
  using Key = std::array<i32, kFacetMaxK>;
  explicit ResolverCache(FullBallStats& stats) : st(stats) {}
  void configure(size_t n) {
    if (n > std::numeric_limits<size_t>::max() / 16) return;
    const size_t target = n * 16;
    size_t count = 1;
    while (count < target) {
      if (count > std::numeric_limits<size_t>::max() / 2) return;
      count *= 2;
    }
    if (count > entries.max_size()) return;
    try { entries.resize(count); }
    catch (const std::bad_alloc&) { std::vector<ResolverCacheEntry>().swap(entries); return; }
    catch (const std::length_error&) { std::vector<ResolverCacheEntry>().swap(entries); return; }
    st.resolver_cache_slots = entries.size();
    st.resolver_cache_bytes = entries.size() * sizeof(ResolverCacheEntry);
  }
  void reset() {
    add(st.resolver_cache_reset_slots, entries.size());
    for (auto& entry : entries) entry.token = absent;
  }
  void release() {
    add(st.resolver_cache_released_slots, entries.size());
    std::vector<ResolverCacheEntry>().swap(entries);
  }
  // The internal caller supplies a sorted, distinct facet of size <= kFacetMaxK.
  static Key key(std::span<const i32> sites) {
    Key value{};
    std::copy(sites.begin(), sites.end(), value.begin());
    return value;
  }
  u64 lookup(const Key& key) {
    if (entries.empty()) return absent;
    add(st.resolver_cache_queries);
    const auto& entry = entries[slot(key)];
    if (entry.token == absent || entry.sites != key) return absent;
    add(st.resolver_cache_hits);
    return entry.token;
  }
  void store(const Key& key, u64 token, bool seed = false) {
    if (entries.empty()) return;
    auto& entry = entries[slot(key)];
    if (entry.token != absent && entry.sites != key) add(st.resolver_cache_evictions);
    add(st.resolver_cache_stores);
    if (seed) add(st.resolver_cache_seed_stores);
    entry.sites = key; entry.token = token;
  }
 private:
  FullBallStats& st;
  std::vector<ResolverCacheEntry> entries;
  size_t slot(const Key& key) const {
    u64 hash = 0x9e3779b97f4a7c15ull;
    for (i32 site : key)
      hash ^= static_cast<u32>(site) + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    hash ^= hash >> 30; hash *= 0xbf58476d1ce4e5b9ull;
    hash ^= hash >> 27; hash *= 0x94d049bb133111ebull;
    hash ^= hash >> 31;
    return static_cast<size_t>(hash) & (entries.size() - 1);
  }
};

class Builder {
 public:
  Builder(const CloudIndex& index, std::span<const BallData> census, unsigned max_k, FullBallStats& stats,
      int static_threads = 0, FullBallBatchResolver batch = {}, bool meb_proposal = true,
      FullBallTimes* phase_times = nullptr, const FullBallTowerOptions& options = {})
      : ix(index), balls(census), requested(max_k), st(stats), resolver_cache(stats),
        geometry_threads(static_threads), batch_resolver(batch), propose_meb(meb_proposal),
        times(phase_times ? phase_times : &unused_times), overlap_static_lots(options.overlap_static),
        pipelined_tail(options.pipelined_tail), hashed_groups(options.hash_grouping),
        static_trace(options.trace) {}

  std::vector<FullBallOrder> run() {
    const auto validate_start = PhaseClock::now();
    validate_catalogue();
    times->validate_ms = ms_since(validate_start);
    require(geometry_threads >= 0, "full_ball_static_threads", FullBallStatus::kInvalidInput);
    require(!batch_resolver.resolve || geometry_threads > 0,
        "full_ball_batch_requires_static", FullBallStatus::kInvalidInput);
    if (kmax > 1 && !geometry_threads) resolver_cache.configure(ix.upos.size());
    // Static path on several threads: the K orders are built concurrently
    // (run_orders_parallel), with the same objects as this sequential loop.
    if (geometry_threads > 1 && kmax > 1 && !batch_resolver.resolve) return run_orders_parallel();
    std::vector<Draft> drafts;
    History lower_history;
    std::vector<u64> lower_anchors;
    for (unsigned k = 1; k <= kmax; ++k) {
      const auto order_start = PhaseClock::now();
      current_k = k;
      if (k > 1) resolver_cache.reset();
      anchors.assign(balls.size(), absent);
      current = {}; compressed.clear();
      if (geometry_threads && k > 1) prepare_static_order();
      if (k == kmax) release_static_arena();  // the last order's phase 0 is done
      MonotoneHistory lower_cursor(lower_history, st);
      Draft draft;
      if (k == 1) {
        FullCoverageBatch initial;
        for (PointId id : domain) {
          const u64 population = populations.size();
          populations.push_back({{}, {id}});
          initial.actions.push_back({{}, {{population, 1, false}}});
          add(st.contributions);
          new_node(initial.level, {}, draft, lower_cursor, absent);
        }
        draft.batches.push_back(std::move(initial));
      }
      const auto& program = programs[k];
      for (size_t begin = 0; begin < program.size();) {
        size_t end = begin + 1;
        const auto& level = balls[program[begin]].level;
        const u32 run = level_run[program[begin]];
        while (end < program.size() && level_run[program[end]] == run) ++end;
        // No anchor of this lot is installed until EVERY representative resolves.
        // Block slots are reused from lot to lot (no allocation per lot).
        if (lot_blocks.size() < end - begin) lot_blocks.resize(end - begin);
        const std::span<Block> blocks(lot_blocks.data(), end - begin);
        const u64 prior_count = current.levels.size();
        for (size_t j = begin; j < end; ++j) prepare_block(blocks[j - begin], program[j], level, prior_count);
        close_lot(blocks, level, draft, lower_cursor, lower_anchors);
        begin = end;
      }
      if (geometry_threads && k > 1) {
        require(static_cursor == static_targets.size(), "full_ball_static_unconsumed_targets");
        RawVector<BallId>().swap(static_targets);
      }
      u64 live = 0;
      for (u64 next : current.next) if (next == absent) ++live;
      require(live == 1, "full_ball_final_component_count");
      failpoint_after_lots(k);
      failpoint_after_populations(k);
      failpoint_after_images(k);
      drafts.push_back(std::move(draft));
      lower_history = std::move(current);
      lower_anchors = std::move(anchors);
      times->order_by_k[k] = ms_since(order_start);
    }
    release_static_arena();
    // Construction indices and histories are dead after the last order; no
    // published object borrows them. Release before copying the immutable bank.
    lower_history = {};
    decltype(lower_anchors)().swap(lower_anchors);
    return finish(drafts);
  }

  std::vector<FullBallOrder> finish(std::vector<Draft>& drafts) {
    resolver_cache.release();
    decltype(key_slots)().swap(key_slots);  // no lookup after the orders
    decltype(identity)().swap(identity);
    decltype(by_key)().swap(by_key);
    decltype(programs)().swap(programs);
    decltype(extra)().swap(extra);
    decltype(population_ids)().swap(population_ids);
    decltype(first_order)().swap(first_order);
    decltype(anchors)().swap(anchors);
    decltype(compressed)().swap(compressed);
    current = {};
    decltype(stack)().swap(stack);
    // Rows are moved into the bank (validated on the static threads).
    const auto bank_start = PhaseClock::now();
    auto bank = build_full_coverage_populations(domain, std::move(populations), geometry_threads);
    times->bank_ms = ms_since(bank_start);
    require(bank.status == FullCertificateStatus::kOk, "full_ball_population_bank",
        bank.status == FullCertificateStatus::kResourceExhausted ? FullBallStatus::kResourceExhausted
                                                               : FullBallStatus::kInvariantViolated);
    // No further population is created: release construction rows before
    // encoding all forests. The immutable bank is shared across the tower.
    std::vector<FullCoveragePopulation>().swap(populations);
    std::vector<FullBallOrder> result;
    result.reserve(kmax);
    // The K forests are independent encodings of their own drafts over the
    // shared immutable bank: built in parallel on the static path, then
    // validated and published in K order (same objects, same statuses).
    std::vector<FullCoverageBuildResult> forests(kmax);
    const auto encode_start = PhaseClock::now();
    parallel_items(kmax, geometry_threads, [&](size_t i, size_t) {
      const auto start = PhaseClock::now();
      forests[i] = drafts[i].flat_form
          ? build_full_coverage_certificate(static_cast<unsigned>(i + 1), bank.value, drafts[i].flat)
          : build_full_coverage_certificate(static_cast<unsigned>(i + 1), bank.value, drafts[i].batches);
      std::vector<FullCoverageBatch>().swap(drafts[i].batches);  // encoded: release at once
      drafts[i].flat = {};
      times->encode_by_k[i + 1] = ms_since(start);
    });
    times->encode_ms = ms_since(encode_start);
    for (unsigned k = 1; k <= kmax; ++k) {
      auto& forest = forests[k - 1];
      require(forest.status == FullCertificateStatus::kOk, "full_ball_structural_certificate",
          forest.status == FullCertificateStatus::kResourceExhausted ? FullBallStatus::kResourceExhausted
                                                                   : FullBallStatus::kInvariantViolated);
      require(forest.value.nodes().size() == drafts[k - 1].lower_nodes.size(), "full_ball_node_encoding");
      result.push_back({std::move(forest.value), std::move(drafts[k - 1].lower_nodes)});
    }
    return result;
  }

  // ---- Static path, K orders built concurrently (geometry_threads > 1).
  //
  // Phase 0 (sequential over K, parallel inside): the static targets of every
  // order. Phase A (parallel over K): each order's lots, anchors, history and
  // draft, with contributions naming BALLS and without vertical images.
  // Phase B (sequential): population IDs assigned in the order of the
  // sequential construction (domain singletons, then K = 1..Kmax, batches,
  // actions, contributions), hence the same bank. Phase C (parallel over K):
  // vertical images of order K from the completed history and anchors of
  // order K-1, queried in node order (monotone cuts, as sequentially). The
  // drafts, the bank and the forests are those of the sequential loop.
  static constexpr u64 kBallTag = u64{1} << 63;
  struct OrderState {
    unsigned k = 0;
    FullBallStats st;
    std::vector<u32> anchors;          // node per ball of this order (kAbsent32)
    History current;
    std::vector<u64> compressed;
    RawVector<BallId> static_targets;
    size_t static_cursor = 0;
    std::vector<Block> lot_blocks;
    Draft draft;
    std::vector<u32> birth_ball;       // per node: ball of a birth, or kAbsent32
    // v9 E1: per node, the exact plateau rank of its level (level_run + 1 of
    // its lot, 0 for the zero level of K1); the images compare these u32.
    std::vector<u32> runs;
    u32 lot_run = 0;
    // v9 E3 (lean phase A): per program position, prepared in parallel.
    std::vector<u32> lean_count;         // facets of the block (targets consumed)
    std::vector<u16> lean_contribution;
    std::vector<u8> lean_interior;
    std::vector<u32> lean_k1;            // K1: domain index of each facet's site, in visit order
    size_t lean_failed = std::numeric_limits<size_t>::max();  // first block whose count fails
    Failure lean_failure{FullBallStatus::kInvariantViolated, ""};
    // v9 E4 (pipelined tail): positions of the draft's contributions naming a
    // ball whose first contributing order is lower (named after the join).
    std::vector<size_t> deferred;
  };
  // Node IDs of one order in u32: an order with 2^32-1 nodes or more is an
  // explicit resource refusal on this path (never a truncation). At 30 M
  // sites and K10 an order holds about 1.2 G nodes.
  static constexpr u32 kAbsent32 = std::numeric_limits<u32>::max();

  std::vector<FullBallOrder> run_orders_parallel() {
    std::vector<OrderState> orders(kmax);
    for (unsigned k = 1; k <= kmax; ++k) orders[k - 1].k = k;
    if (overlap_static_lots) return run_orders_overlapped(orders);
    for (unsigned k = 2; k <= kmax; ++k) {
      auto& o = orders[k - 1];
      const auto start = PhaseClock::now();
      current_k = k;
      prepare_static_order();  // parallel inside, writes static_targets
      o.static_targets.swap(static_targets);
      times->static_by_k[k] = ms_since(start);
      times->static_ms += times->static_by_k[k];
    }
    release_static_arena();
    // A Failure is reported for the SMALLEST K over both phases, as the
    // sequential loop would: when the lots of order f fail, the images of
    // the orders below f (whose lots all succeeded) are still resolved.
    // Other exceptions propagate as before. The private counters of every
    // order are merged exactly once, on success and on every exit.
    std::vector<std::optional<Failure>> failures(kmax);
    bool merged = false;
    const auto merge_once = [&] {
      if (merged) return;
      merged = true;
      for (auto& o : orders) merge_order_stats(o.st);
      add(st.parallel_orders, kmax);
    };
    try {
      const auto lots_start = PhaseClock::now();
      parallel_items(kmax, geometry_threads, [&](size_t i, size_t) {
        const auto start = PhaseClock::now();
        try { order_lots(orders[i]); } catch (const Failure& f) { failures[i] = f; }
        times->lots_by_k[i + 1] = ms_since(start);
      });
      times->lots_ms = ms_since(lots_start);
      size_t lots_done = 0;
      while (lots_done < kmax && !failures[lots_done]) ++lots_done;
#if defined(MHGP9_FULL_ORDERS_MUTANT_PHASE_PRIORITY)
      if (lots_done != kmax) lots_done = 0;  // mutant: a lot failure masks lower images
#endif
      const auto populations_start = PhaseClock::now();
      if (lots_done == kmax) assign_populations(orders);
      times->populations_ms = ms_since(populations_start);
      const size_t imaged = populations_done(failures, lots_done);
      const auto images_start = PhaseClock::now();
      parallel_items(imaged, geometry_threads, [&](size_t i, size_t) {
        const auto start = PhaseClock::now();
        try { order_images(orders[i], i ? &orders[i - 1] : nullptr); } catch (const Failure& f) { failures[i] = f; }
        times->images_by_k[i + 1] = ms_since(start);
      });
      times->images_ms = ms_since(images_start);
      for (auto& failure : failures) if (failure) throw *failure;
      merge_once();
    } catch (...) {
#if !defined(MHGP9_FULL_ORDERS_MUTANT_DROP_FAILED_STATS)
      merge_once();
#endif
      throw;
    }
    std::vector<Draft> drafts;
    for (auto& o : orders) drafts.push_back(std::move(o.draft));
    decltype(orders)().swap(orders);
    return finish(drafts);
  }

  // Population failpoints of the unpipelined paths (MHGP9_TESTING only): the
  // IDs of every order are assigned at once, so the failpoint of order K
  // (below the first lot failure) is recorded as its failure, before its
  // images, as the sequential loop would. Returns the number of orders whose
  // images run: those below the first lot or population failure.
  size_t populations_done(std::vector<std::optional<Failure>>& failures, size_t lots_done) {
    for (size_t i = 0; i < lots_done; ++i) {
      try { failpoint_after_populations(static_cast<unsigned>(i + 1)); }
      catch (const Failure& f) { failures[i] = f; return i; }
    }
    return lots_done;
  }

  // Overlapped static path (same objects): phase 0 runs by DECREASING K on
  // the geometry threads while one runner per order waits for its own static
  // targets and then runs its phase A; order 1 has none and starts at once.
  // Phase A of order K reads only its OrderState and the immutable
  // catalogue, programs and index; phase 0 writes only the builder's own
  // static members, one order at a time. Failures keep the sequential
  // priority: a phase-0 failure is reported for the SMALLEST failing K (the
  // descending loop keeps the last one), before any lot or image failure;
  // otherwise lots and images as in run_orders_parallel. Counters of every
  // order are merged exactly once on every exit. Times: static_by_k as
  // before; lots_by_k is each order's own phase A (it may overlap phase 0);
  // lots_ms is the launch-to-join window minus phase 0.
  std::vector<FullBallOrder> run_orders_overlapped(std::vector<OrderState>& orders) {
    // v9 E4 (pipelined tail, default): after its phase A, order K starts a
    // helper that assigns the population IDs of the balls whose FIRST
    // contributing order is K (static offset, compute_population_offsets)
    // and builds their rows, while its runner waits for phase A of order K-1
    // and runs its vertical images. Everything computable is computed (B
    // needs A(K), C needs A(K) and A(K-1)); after the join the failure kept is
    // the sequential one: phase 0, other exceptions by K, then by K lots,
    // populations, images. Without it (witness), phases B and C run after
    // the join, as before.
    const bool pipelined = pipelined_tail;
    std::vector<std::optional<Failure>> failures(kmax), population_failures(kmax), image_failures(kmax);
    bool merged = false;
    const auto merge_once = [&] {
      if (merged) return;
      merged = true;
      for (auto& o : orders) merge_order_stats(o.st);
      add(st.parallel_orders, kmax);
      add(st.overlapped_orders, kmax);
      if (pipelined) {
        add(st.pipelined_orders, kmax);
        for (const auto& o : orders) add(st.population_deferred_refs, o.deferred.size());
      }
    };
    try {
      std::mutex mu;
      std::condition_variable wake;
      std::vector<char> ready(kmax, 0);
      // Pipelined tail: phase A state per order (0 pending, 1 done, 2 failed
      // or never run) and the population rows' state (0 pending, 1 sized, 2
      // failed); both only move under `mu`, then `wake` is notified.
      std::vector<char> lots_state(kmax, 0);
      char rows_state = 0;
      bool cancelled = false;
      ready[0] = 1;
      // Other exceptions, one slot per order and step (each written by one
      // thread): phase A (and the sizing on order 1), B (helper), C.
      std::vector<std::exception_ptr> errors(kmax), population_errors(kmax), image_errors(kmax);
#if defined(MHGP9_FULL_ORDERS_MUTANT_TIMERS_AFTER_JOIN)
      // mutant: the runners' timers and `publish` die before the join
      std::vector<std::thread> runners;
      runners.reserve(kmax);
      parallel_detail::JoinThreads joined{runners};
#endif
      // One window from the first runner launch to the last join: phase 0
      // and every order's phase A lie inside it (lots_ms = window - static).
      const auto window_start = PhaseClock::now();
      std::vector<PhaseClock::time_point> lots_end(kmax, window_start), populations_end(kmax, window_start),
          images_begin(kmax, window_start), images_end(kmax, window_start);
      const auto cancel = [&] {
        { std::lock_guard<std::mutex> lock(mu); cancelled = true; }
        wake.notify_all();
      };
      const auto publish = [&](char& state, char value) {
        { std::lock_guard<std::mutex> lock(mu); state = value; }
        wake.notify_all();
      };
#if !defined(MHGP9_FULL_ORDERS_MUTANT_TIMERS_AFTER_JOIN)
      // LIFETIME: every object a runner or its helper touches is declared
      // ABOVE `joined`, so it outlives the join on every exit path (a phase-0
      // or launch exception unwinds this scope while runners still run).
      std::vector<std::thread> runners;
      runners.reserve(kmax);
      parallel_detail::JoinThreads joined{runners};
#endif
      try {
        for (size_t i = 0; i < kmax; ++i) {
          failpoint_runner_launch(static_cast<unsigned>(i + 1));
          runners.emplace_back([&, i] {
            {
              std::unique_lock<std::mutex> lock(mu);
              wake.wait(lock, [&] { return ready[i] != 0 || cancelled; });
              if (!ready[i]) {
                lots_state[i] = 2;
                lock.unlock();
                wake.notify_all();
                return;
              }
            }
            if (pipelined && i == 0) {
              // The rows array is sized once, before any runner writes a row.
              try { prepare_population_rows(); publish(rows_state, 1); }
              catch (...) {
                errors[i] = std::current_exception();
                publish(rows_state, 2);
                publish(lots_state[i], 2);
                return;
              }
            }
            const auto start = PhaseClock::now();
            bool lots_ok = false;
            try { order_lots(orders[i]); lots_ok = true; } catch (const Failure& f) { failures[i] = f; }
            catch (...) { errors[i] = std::current_exception(); }
            times->lots_by_k[i + 1] = ms_since(start);
            failpoint_runner_pause();  // tests only: outlast an unwinding caller
            lots_end[i] = PhaseClock::now();
            if (!pipelined) return;
            publish(lots_state[i], lots_ok ? 1 : 2);
            if (!lots_ok) return;
            // Phase B of K on a helper thread (the rows in parallel inside),
            // phase C of K on this runner once phase A of K-1 is done: they
            // touch disjoint parts of the draft (contributions / parents).
            std::vector<std::thread> helper;
            parallel_detail::JoinThreads helper_joined{helper};
            try {
              helper.emplace_back([&, i] {
                char rows = 0;
                {
                  std::unique_lock<std::mutex> lock(mu);
                  wake.wait(lock, [&] { return rows_state != 0; });
                  rows = rows_state;
                }
                if (rows != 1) return;  // the sizing failed: reported by order 1
                const auto populations_start = PhaseClock::now();
                try { order_populations(orders[i]); } catch (const Failure& f) { population_failures[i] = f; }
                catch (...) { population_errors[i] = std::current_exception(); }
                times->populations_by_k[i + 1] = ms_since(populations_start);
                populations_end[i] = PhaseClock::now();
              });
            } catch (...) { population_errors[i] = std::current_exception(); return; }
            bool lower_done = true;
            if (i > 0) {
              std::unique_lock<std::mutex> lock(mu);
              wake.wait(lock, [&] { return lots_state[i - 1] != 0 || (cancelled && !ready[i - 1]); });
              lower_done = lots_state[i - 1] == 1;
            }
            if (!lower_done) return;  // images of K need the completed phase A of K-1
            images_begin[i] = PhaseClock::now();
            try { order_images(orders[i], i ? &orders[i - 1] : nullptr); } catch (const Failure& f) { image_failures[i] = f; }
            catch (...) { image_errors[i] = std::current_exception(); }
            images_end[i] = PhaseClock::now();
          });
        }
      } catch (...) { cancel(); throw; }
      std::optional<Failure> static_failure;
      try {
        for (unsigned k = kmax; k >= 2; --k) {
          const auto start = PhaseClock::now();
          current_k = k;
          bool built = true;
          try {
            prepare_static_order();  // parallel inside, writes static_targets
            orders[k - 1].static_targets.swap(static_targets);
          } catch (const Failure& f) { static_failure = f; built = false; }
          times->static_by_k[k] = ms_since(start);
          times->static_ms += times->static_by_k[k];
          if (built) {
            { std::lock_guard<std::mutex> lock(mu); ready[k - 1] = 1; }
            wake.notify_all();
          }
        }
        release_static_arena();
      } catch (...) { cancel(); throw; }
      if (static_failure) cancel();
      for (auto& runner : runners) runner.join();
      if (pipelined) {
        // Additive split of the window (FullBallTimes): up to the last phase
        // A, then the exposed populations, then the exposed images.
        const auto join = PhaseClock::now();
        const auto last_lots = *std::max_element(lots_end.begin(), lots_end.end());
        const auto last_populations = std::max(last_lots, *std::max_element(populations_end.begin(), populations_end.end()));
        const auto span_ms = [](PhaseClock::time_point a, PhaseClock::time_point b) {
          return std::chrono::duration<double, std::milli>(b - a).count();
        };
        times->lots_ms = std::max(0.0, span_ms(window_start, last_lots) - times->static_ms);
        times->populations_ms = span_ms(last_lots, last_populations);
        times->images_ms = std::max(0.0, span_ms(last_populations, join));
        for (size_t i = 0; i < kmax; ++i) {
          times->images_own_by_k[i + 1] = span_ms(images_begin[i], images_end[i]);
          times->images_by_k[i + 1] = std::max(0.0, span_ms(std::max(images_begin[i], last_populations), images_end[i]));
        }
      } else {
        times->lots_ms = std::max(0.0, ms_since(window_start) - times->static_ms);
      }
      if (static_failure) throw *static_failure;
      for (size_t i = 0; i < kmax; ++i)
        for (const auto* slot : {&errors, &population_errors, &image_errors})
          if ((*slot)[i]) std::rethrow_exception((*slot)[i]);
      if (pipelined) {
        // Everything computable was computed; the reported failure is the
        // sequential loop's: the smallest K, and in order K its lots, then
        // its populations, then its images.
#if defined(MHGP9_FULL_ORDERS_MUTANT_PHASE_PRIORITY)
        // mutant: a lot failure masks lower images
        if (std::any_of(failures.begin(), failures.end(), [](const auto& f) { return f.has_value(); }))
          for (auto& f : image_failures) f.reset();
#endif
        for (size_t i = 0; i < kmax; ++i) {
          if (failures[i]) throw *failures[i];
          if (population_failures[i]) throw *population_failures[i];
          if (image_failures[i]) throw *image_failures[i];
        }
        name_deferred_populations(orders);
        merge_once();
      } else {
      size_t lots_done = 0;
      while (lots_done < kmax && !failures[lots_done]) ++lots_done;
#if defined(MHGP9_FULL_ORDERS_MUTANT_PHASE_PRIORITY)
      if (lots_done != kmax) lots_done = 0;  // mutant: a lot failure masks lower images
#endif
      const auto populations_start = PhaseClock::now();
      if (lots_done == kmax) assign_populations(orders);
      times->populations_ms = ms_since(populations_start);
      const size_t imaged = populations_done(failures, lots_done);
      const auto images_start = PhaseClock::now();
      parallel_items(imaged, geometry_threads, [&](size_t i, size_t) {
        const auto start = PhaseClock::now();
        try { order_images(orders[i], i ? &orders[i - 1] : nullptr); } catch (const Failure& f) { failures[i] = f; }
        times->images_by_k[i + 1] = ms_since(start);
      });
      times->images_ms = ms_since(images_start);
      for (auto& failure : failures) if (failure) throw *failure;
      merge_once();
      }
    } catch (...) {
#if !defined(MHGP9_FULL_ORDERS_MUTANT_DROP_FAILED_STATS)
      merge_once();
#endif
      throw;
    }
    std::vector<Draft> drafts;
    for (auto& o : orders) drafts.push_back(std::move(o.draft));
    std::vector<OrderState>().swap(orders);
    return finish(drafts);
  }

  void merge_order_stats(const FullBallStats& o) {
    // Only the counters phases A and C write; everything else stays zero.
    for (auto [into, from] : {std::pair{&st.anchor_blocks, o.anchor_blocks}, {&st.regular_blocks, o.regular_blocks},
                              {&st.extra_blocks, o.extra_blocks}, {&st.representatives, o.representatives},
                              {&st.births, o.births}, {&st.merges, o.merges}, {&st.contributions, o.contributions},
                              {&st.inert_blocks, o.inert_blocks}, {&st.singleton_lots, o.singleton_lots},
                              {&st.grouped_lots, o.grouped_lots}, {&st.lot_dsu_slots, o.lot_dsu_slots},
                              {&st.lower_edges_indexed, o.lower_edges_indexed},
                              {&st.lower_nodes_activated, o.lower_nodes_activated},
                              {&st.lower_edges_activated, o.lower_edges_activated},
                              {&st.lower_queries, o.lower_queries}, {&st.lower_find_steps, o.lower_find_steps},
                              {&st.lower_path_writes, o.lower_path_writes}})
      add(*into, from);
  }

  u64 order_root(OrderState& o, u64 token, u64 prior_count) {
    require(token < prior_count, "full_ball_anchor_not_prior");
    u64 r = token;
    while (o.compressed[r] != r) r = o.compressed[r];
    while (o.compressed[token] != token) {
      const auto next = o.compressed[token]; o.compressed[token] = r; token = next;
    }
    require(r < prior_count && o.current.next[r] == absent, "full_ball_root_not_prior");
    return r;
  }

  u64 order_new_node(OrderState& o, const ExactLevel& level, const std::vector<u64>& parents, u32 birth) {
    // v9 E3: the static path keeps no ExactLevel per node (the images read
    // the plateau ranks); the node count is the size of `next`.
    static_cast<void>(level);
    const u64 id = o.current.next.size();
    require(id < kAbsent32, "full_ball_node_representation", FullBallStatus::kResourceExhausted);
    o.current.next.push_back(absent); o.compressed.push_back(id);
    for (u64 parent : parents) { o.current.next[parent] = id; o.compressed[parent] = id; }
    o.birth_ball.push_back(birth);
    o.runs.push_back(o.lot_run);
    if (parents.empty()) add(o.st.births); else add(o.st.merges);
    return id;
  }

  // v9 E3 (lean phase A, plan of the tower judge): the history-free data of
  // every block of the order, in parallel before the sequential loop (the
  // facet count, contribution and interior of count_block_at; for K1 the
  // domain index of each facet's site). A count failure is kept for the
  // FIRST failing block and thrown by the loop when it reaches that block,
  // after the same counters: the reported failure is the sequential one.
  void order_prepare_lean(OrderState& o) {
    const auto& program = programs[o.k];
    const size_t n = program.size();
    o.lean_count.assign(n, 0);
    o.lean_contribution.assign(n, 0);
    o.lean_interior.assign(n, 0);
    o.lean_failed = std::numeric_limits<size_t>::max();
    constexpr size_t block = 4096;
    const size_t chunks = (n + block - 1) / block;
    const size_t none = std::numeric_limits<size_t>::max();
    std::vector<size_t> failed_at(chunks, none);
    std::vector<Failure> failures(chunks, Failure{FullBallStatus::kInvariantViolated, ""});
    parallel_items(chunks, geometry_threads, [&](size_t chunk, size_t) {
      for (size_t j = chunk * block; j < std::min(n, (chunk + 1) * block); ++j) {
        try {
          u16 contribution = 0; bool interior = false;
          o.lean_count[j] = count_block_at(o.k, program[j], contribution, interior);
          o.lean_contribution[j] = contribution;
          o.lean_interior[j] = interior ? 1 : 0;
        } catch (const Failure& failure) {
          failed_at[chunk] = j; failures[chunk] = failure;
          return;
        }
      }
    });
    for (size_t c = 0; c < chunks; ++c)
      if (failed_at[c] != none) { o.lean_failed = failed_at[c]; o.lean_failure = failures[c]; break; }
    if (o.k != 1) return;
    // K1: the facets are single sites; their domain indices in visit order
    // (up to the first failing block, which the loop never passes).
    o.lean_k1.clear();
    for (size_t j = 0; j < std::min(n, o.lean_failed); ++j) {
      u16 contribution = 0; bool interior = false;
      visit_block_at(1, program[j], contribution, interior, [&](std::span<const i32> facet) {
        require(facet.size() == 1, "full_ball_representative_cardinality");
        const PointId point = ix.point_id(facet.front());
        o.lean_k1.push_back(static_cast<u32>(std::lower_bound(domain.begin(), domain.end(), point) - domain.begin()));
      });
    }
  }

  // The loop's appends are bounded by the prepared counts: a node, an action
  // and a batch per block at most (plus the K1 domain), a parent per facet,
  // a contribution per block. Reserved once (no regrowth copies in the loop).
  void reserve_lean(OrderState& o) {
    const size_t blocks = programs[o.k].size();
    size_t facets = 0;
    for (const u32 count : o.lean_count) facets += count;
    const size_t nodes = blocks + (o.k == 1 ? domain.size() : 0);
    o.current.next.reserve(nodes); o.compressed.reserve(nodes);
    o.birth_ball.reserve(nodes); o.runs.reserve(nodes);
    auto& d = o.draft.flat;
    d.level.reserve(nodes + 1); d.batch_begin.reserve(nodes + 2);
    d.parent_begin.reserve(nodes + 1); d.contribution_begin.reserve(nodes + 1);
    d.parent.reserve(facets); d.contribution.reserve(nodes);
  }
  // Prefetch distances of the lean loop (facets ahead): hints only.
  static constexpr size_t kLeanAnchorAhead = 24, kLeanRootAhead = 8, kLeanBlockAhead = 8;
  // One block of the lean loop: the counters, requires and roots of
  // order_block in the same order, without building the facets.
  void order_block_lean(OrderState& o, Block& block, size_t position, BallId id, u64 prior_count,
                        size_t& k1_cursor) {
    block.ball = id; block.roots.clear(); block.contribution = 0; block.interior = false;
    add(o.st.anchor_blocks);
    if (balls[id].n_shell == balls[id].arity) add(o.st.regular_blocks); else add(o.st.extra_blocks);
    if (position == o.lean_failed) throw o.lean_failure;
    const u32 facets = o.lean_count[position];
    for (u32 f = 0; f < facets; ++f) {
      add(o.st.representatives);
      if (o.k == 1) {
        require(k1_cursor < o.lean_k1.size(), "full_ball_representative_cardinality");
        block.roots.push_back(order_root(o, o.lean_k1[k1_cursor++], prior_count));
        continue;
      }
      require(o.static_cursor < o.static_targets.size(), "full_ball_static_target_missing");
      // The targets are known ahead: prefetch the anchor of a later facet and
      // the compressed slot of a nearer one (an anchor of an earlier lot).
      if (o.static_cursor + kLeanAnchorAhead < o.static_targets.size()) {
        const BallId later = o.static_targets[o.static_cursor + kLeanAnchorAhead];
        if (later < o.anchors.size()) __builtin_prefetch(&o.anchors[later]);
      }
      if (o.static_cursor + kLeanRootAhead < o.static_targets.size()) {
        const BallId nearer = o.static_targets[o.static_cursor + kLeanRootAhead];
        if (nearer < o.anchors.size() && o.anchors[nearer] < o.compressed.size())
          __builtin_prefetch(&o.compressed[o.anchors[nearer]]);
      }
      const BallId target = o.static_targets[o.static_cursor++];
      require(target < balls.size() && level_run[target] < level_run[id], "full_ball_static_target_not_strict");
      require(o.anchors[target] != kAbsent32, "full_ball_static_closed_anchor_missing");
      block.roots.push_back(order_root(o, o.anchors[target], prior_count));
    }
    block.contribution = o.lean_contribution[position];
    block.interior = o.lean_interior[position] != 0;
    std::sort(block.roots.begin(), block.roots.end());
    block.roots.erase(std::unique(block.roots.begin(), block.roots.end()), block.roots.end());
  }

  void order_lot(OrderState& o, std::span<Block> blocks, const ExactLevel& level) {
    const auto birth_of = [&](const std::vector<u64>& parents, BallId ball) {
      return parents.empty() ? static_cast<u32>(ball) : kAbsent32;
    };
    if (blocks.size() == 1) {
      // Same action as the grouped path, built only when it is published: an
      // inert block (one parent, no contribution) allocates nothing, and the
      // block keeps its roots buffer for the next lot (no steal, no regrowth).
      add(o.st.singleton_lots);
      const auto& block = blocks.front();
      const bool contributes = block.contribution || block.interior;
      if (contributes) add(o.st.contributions);
      u64 target;
      if (block.roots.size() == 1) target = block.roots.front();
      else {
        if (block.roots.empty()) require(contributes, "full_ball_distinct_births");
        target = order_new_node(o, level, block.roots, birth_of(block.roots, block.ball));
      }
      if (block.roots.size() != 1 || contributes) {
        const FullCoverageRef ref{kBallTag | block.ball, block.contribution, block.interior};
        o.draft.flat.open_batch(level);
        o.draft.flat.add_action(block.roots, std::span<const FullCoverageRef>(&ref, contributes ? 1 : 0));
      } else add(o.st.inert_blocks);
      require(o.anchors[block.ball] == kAbsent32 && target != absent, "full_ball_anchor_duplicate");
      o.anchors[block.ball] = static_cast<u32>(target);
      return;
    }
    add(o.st.grouped_lots);
    add(o.st.lot_dsu_slots, blocks.size());
    std::vector<size_t> dsu(blocks.size()); std::iota(dsu.begin(), dsu.end(), size_t{0});
    const auto find = [&](size_t a) { while (dsu[a] != a) { dsu[a] = dsu[dsu[a]]; a = dsu[a]; } return a; };
    std::vector<std::pair<u64,size_t>> owners;
    for (size_t b = 0; b < blocks.size(); ++b) for (u64 p : blocks[b].roots) owners.emplace_back(p,b);
    std::sort(owners.begin(), owners.end());
    for (size_t j = 1; j < owners.size(); ++j) if (owners[j - 1].first == owners[j].first) {
      const auto a = find(owners[j - 1].second), b = find(owners[j].second);
      dsu[std::max(a,b)] = std::min(a,b);
    }
    std::vector<std::vector<size_t>> groups(blocks.size());
    for (size_t b = 0; b < blocks.size(); ++b) groups[find(b)].push_back(b);
    bool opened = false;
    std::vector<u64> targets(blocks.size(), absent);
    for (const auto& group : groups) {
      if (group.empty()) continue;
      FullCoverageAction action;
      for (size_t b : group) {
        const auto& block = blocks[b];
        action.parents.insert(action.parents.end(), block.roots.begin(), block.roots.end());
        if (block.contribution || block.interior) {
          action.contributions.push_back({kBallTag | block.ball, block.contribution, block.interior});
          add(o.st.contributions);
        }
      }
      std::sort(action.parents.begin(), action.parents.end());
      action.parents.erase(std::unique(action.parents.begin(), action.parents.end()), action.parents.end());
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        if (action.parents.empty())
          require(group.size() == 1 && action.contributions.size() == 1, "full_ball_distinct_births");
        target = order_new_node(o, level, action.parents, birth_of(action.parents, blocks[group.front()].ball));
      }
      for (size_t b : group) targets[b] = target;
      if (action.parents.size() != 1 || !action.contributions.empty()) {
        if (!opened) {
          o.draft.flat.open_batch(level);
          opened = true;
        }
        o.draft.flat.add_action(action.parents, action.contributions);
      } else add(o.st.inert_blocks, group.size());
    }
    for (size_t b = 0; b < blocks.size(); ++b) {
      require(o.anchors[blocks[b].ball] == kAbsent32 && targets[b] != absent, "full_ball_anchor_duplicate");
      o.anchors[blocks[b].ball] = static_cast<u32>(targets[b]);
    }
  }

  void order_lots(OrderState& o) {
    o.anchors.assign(balls.size(), kAbsent32);
    o.draft.flat_form = true;
    if (o.k == 1) {
      const ExactLevel zero{{0, 0, 0}, 1};
#if defined(MHGP9_TOWER_MUTANT_RUNS_ZERO_COLLIDES)
      o.lot_run = 1;  // mutant: the zero level shares the rank of the lowest catalogue level
#else
      o.lot_run = 0;  // the zero level is below every catalogue level
#endif
      o.draft.flat.open_batch(zero);
      for (size_t j = 0; j < domain.size(); ++j) {
        const FullCoverageRef ref{j, 1, false};  // domain singleton j (population j)
        o.draft.flat.add_action({}, std::span<const FullCoverageRef>(&ref, 1));
        add(o.st.contributions);
        order_new_node(o, zero, {}, kAbsent32);
      }
    }
    const auto& program = programs[o.k];
    order_prepare_lean(o);
    reserve_lean(o);
    size_t k1_cursor = 0;
    for (size_t begin = 0; begin < program.size();) {
      size_t end = begin + 1;
      const auto& level = balls[program[begin]].level;
      const u32 run = level_run[program[begin]];
      while (end < program.size() && level_run[program[end]] == run) ++end;
      if (o.lot_blocks.size() < end - begin) o.lot_blocks.resize(end - begin);
      const std::span<Block> blocks(o.lot_blocks.data(), end - begin);
      const u64 prior_count = o.current.next.size();
      require(run < kAbsent32 - 1, "full_ball_level_run_representation", FullBallStatus::kResourceExhausted);
      o.lot_run = run + 1;
      // The anchor written by a later lot's block: prefetched for writing.
      if (begin + kLeanBlockAhead < program.size()) __builtin_prefetch(&o.anchors[program[begin + kLeanBlockAhead]], 1);
      for (size_t j = begin; j < end; ++j)
        order_block_lean(o, blocks[j - begin], j, program[j], prior_count, k1_cursor);
      order_lot(o, blocks, level);
      begin = end;
    }
    require(o.k != 1 || k1_cursor == o.lean_k1.size(), "full_ball_k1_facets_unconsumed");
    decltype(o.lean_count)().swap(o.lean_count);
    decltype(o.lean_contribution)().swap(o.lean_contribution);
    decltype(o.lean_interior)().swap(o.lean_interior);
    decltype(o.lean_k1)().swap(o.lean_k1);
    if (o.k > 1) require(o.static_cursor == o.static_targets.size(), "full_ball_static_unconsumed_targets");
    RawVector<BallId>().swap(o.static_targets);
    decltype(o.lot_blocks)().swap(o.lot_blocks);
    decltype(o.compressed)().swap(o.compressed);
    u64 live = 0;
    for (u64 next : o.current.next) if (next == absent) ++live;
    require(live == 1, "full_ball_final_component_count");
    failpoint_after_lots(o.k);
  }

  void assign_populations(std::vector<OrderState>& orders) {
    // IDs in the order of the sequential construction (domain singletons,
    // then first encounter over K, batches, actions, contributions); the
    // rows themselves are then built in parallel, each at its own ID.
    std::vector<BallId> first_seen;
    u64 next = domain.size();
    // The flat contributions are stored in batch, action, contribution order.
    for (auto& o : orders) {
      require(o.draft.flat_form, "full_ball_draft_form");
      for (auto& ref : o.draft.flat.contribution) {
        if ((ref.population & kBallTag) == 0) continue;
        const auto ball = static_cast<BallId>(ref.population & ~kBallTag);
        if (population_ids[ball] == absent) { population_ids[ball] = next++; first_seen.push_back(ball); }
        ref.population = population_ids[ball];
      }
    }
    populations.resize(next);
    for (size_t j = 0; j < domain.size(); ++j) populations[j] = {{}, {domain[j]}};
    parallel_ranges(first_seen.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
      for (size_t j = begin; j < end; ++j) {
        const BallId id = first_seen[j];
        auto& row = populations[domain.size() + j];
        for (i32 site : balls[id].interior()) row.interior.push_back(ix.point_id(site));
        for (i32 site : balls[id].shell()) row.shell.push_back(ix.point_id(site));
        std::sort(row.interior.begin(), row.interior.end()); std::sort(row.shell.begin(), row.shell.end());
      }
    });
  }

  // ---- v9 E4, pipelined tail: population IDs by static offset.
  //
  // The sequential construction names a ball's population at its FIRST
  // contribution, walking K = 1..Kmax, then batches, actions and
  // contributions (assign_populations, kept as the witness). A ball's block
  // contributes at order K iff count_block_at gives it a non-empty shell
  // contribution or its interior, a function of the ball and K only: a
  // regular ball contributes at K = p+u alone (its facet order p+u-1 does
  // not), an extended shell at the ranks whose table leaves some shell site
  // outside every strict local component (possibly several ranks). Each
  // ball's block appears at most once per order and every contributing block
  // is published by phase A. Hence, with first(b) the smallest contributing
  // order of b and offset(K) = #{b : first(b) < K}, the ID of b is
  // n + offset(first(b)) + the rank of b among the balls of first order
  // first(b), in that order's contribution sequence: the same IDs.
  bool pipelined_populations() const {
    return geometry_threads > 1 && kmax > 1 && !batch_resolver.resolve && overlap_static_lots && pipelined_tail;
  }
  void compute_population_offsets() {
    first_order.resize(balls.size());  // every slot written below
    constexpr size_t block = 16384;
    const size_t chunks = std::max<size_t>(1, (balls.size() + block - 1) / block);
    std::vector<std::array<u64, kFacetMaxK + 1>> counts(chunks);
    parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
      auto& local = counts[c];
      local.fill(0);
      for (size_t j = c * block; j < std::min(balls.size(), (c + 1) * block); ++j) {
        const auto& b = balls[j];
        u8 first = 0;
        if (b.n_shell == b.arity) {
          if (static_cast<unsigned>(b.n_interior) + b.n_shell <= kmax) {
            first = static_cast<u8>(b.n_interior + b.n_shell);
            ++local[first];
          }
        } else {
          const auto& table = extra.at(static_cast<BallId>(j));
          const unsigned lo = b.n_interior + b.arity - 1;
          const unsigned hi = std::min<unsigned>(kmax, b.n_interior + b.n_shell);
          for (unsigned k = lo; k <= hi; ++k) {
            const auto rank = table.rank(k);
            if (!rank.contribution_shell && !rank.contribution_interior) continue;
            if (!first) first = static_cast<u8>(k);
#if defined(MHGP9_TOWER_MUTANT_OFFSET_ON_CONTRIBUTIONS)
            ++local[k];  // mutant: every contribution counted, not the first encounter
#else
            ++local[k];
            break;
#endif
          }
        }
        first_order[j] = first;
      }
    });
    population_offset.fill(0);
    for (unsigned k = 1; k <= kmax; ++k) {
      u64 count = 0;
      for (const auto& local : counts) add(count, local[k]);
      population_offset[k + 1] = population_offset[k];
      add(population_offset[k + 1], count);
    }
    require(population_offset[kmax + 1] <= std::numeric_limits<u64>::max() - domain.size(),
            "full_ball_population_count", FullBallStatus::kResourceExhausted);
  }
  // The rows array (domain singletons first, then n + offset(Kmax+1) balls),
  // sized once on the runner of order 1 before any other runner writes a row.
  void prepare_population_rows() {
    populations.resize(domain.size() + population_offset[kmax + 1]);
    for (size_t j = 0; j < domain.size(); ++j) populations[j] = {{}, {domain[j]}};
  }
  // Phase B of one order, on its runner: IDs of the balls whose first order
  // is K, in contribution order, at n + offset(K); references to older balls
  // are kept for name_deferred_populations; then the new rows, in parallel.
  void order_populations(OrderState& o) {
    require(o.draft.flat_form, "full_ball_draft_form");
    const u64 base = domain.size() + population_offset[o.k];
    u64 next = base;
    std::vector<BallId> fresh;
    auto& refs = o.draft.flat.contribution;
    o.deferred.clear();
    for (size_t c = 0; c < refs.size(); ++c) {
      auto& ref = refs[c];
      if ((ref.population & kBallTag) == 0) continue;  // K1 domain singleton j: population j
      const auto ball = static_cast<BallId>(ref.population & ~kBallTag);
      require(ball < balls.size() && first_order[ball] != 0 && first_order[ball] <= o.k,
              "full_ball_population_first_order");
      if (first_order[ball] < o.k) { o.deferred.push_back(c); continue; }
      require(population_ids[ball] == absent, "full_ball_population_duplicate");
      population_ids[ball] = next;
      ref.population = next++;
      fresh.push_back(ball);
    }
    require(next - base == population_offset[o.k + 1] - population_offset[o.k], "full_ball_population_offset");
    parallel_ranges(fresh.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
      for (size_t j = begin; j < end; ++j) {
        const BallId id = fresh[j];
        auto& row = populations[base + j];
        for (i32 site : balls[id].interior()) row.interior.push_back(ix.point_id(site));
        for (i32 site : balls[id].shell()) row.shell.push_back(ix.point_id(site));
        std::sort(row.interior.begin(), row.interior.end()); std::sort(row.shell.begin(), row.shell.end());
      }
    });
    failpoint_after_populations(o.k);
  }
  // After the join, every order's phase B done: the references to balls of a
  // lower first order receive the ID that order assigned.
  void name_deferred_populations(std::vector<OrderState>& orders) {
#if defined(MHGP9_TOWER_MUTANT_DEFERRED_UNNAMED)
    return;  // mutant: the references to balls of a lower first order keep their ball tag
#endif
    for (auto& o : orders)
      for (const size_t c : o.deferred) {
        auto& ref = o.draft.flat.contribution[c];
        const auto ball = static_cast<BallId>(ref.population & ~kBallTag);
        require(population_ids[ball] != absent, "full_ball_population_deferred_unnamed");
        ref.population = population_ids[ball];
      }
  }

  void order_images(OrderState& o, const OrderState* lower) {
    // Closed cuts compared on exact plateau ranks (E1): same answers as the
    // exact levels (level_run is an order isomorphism), same counters.
    MonotoneHistoryOf<u32> cursor(lower ? std::span<const u32>(lower->runs) : std::span<const u32>(),
                                  lower ? std::span<const u64>(lower->current.next) : std::span<const u64>(), o.st);
    require(o.runs.size() == o.current.next.size(), "full_ball_image_rank_shape");
    o.draft.lower_nodes.reserve(o.current.next.size());
    u64 node = 0;
    require(o.draft.flat_form, "full_ball_draft_form");
    const auto& flat = o.draft.flat;
    for (size_t a = 0; a < flat.actions(); ++a) {
      const auto parents = flat.parents_of(a);
      if (parents.size() == 1) continue;  // continuation: no node
      require(node < o.current.next.size(), "full_ball_image_node_count");
      u64 image = absent;
      if (o.k > 1) {
        const u32 level = o.runs[node];
        if (parents.empty()) {
          const u32 ball = o.birth_ball[node];
          require(ball != kAbsent32 && lower->anchors[ball] != kAbsent32, "full_ball_vertical_birth_anchor");
          image = cursor.root_at(lower->anchors[ball], level, true);
        } else {
          image = cursor.root_at(o.draft.lower_nodes[parents.front()], level, true);
          for (u64 p : parents)
            require(cursor.root_at(o.draft.lower_nodes[p], level, true) == image, "full_ball_vertical_naturality");
        }
      }
      o.draft.lower_nodes.push_back(image);
      ++node;
    }
    require(node == o.current.next.size(), "full_ball_image_node_count");
    failpoint_after_images(o.k);
  }

 private:
  const CloudIndex& ix;
  std::span<const BallData> balls;
  unsigned requested, kmax = 0, current_k = 0;
  FullBallStats& st;
  ResolverCache resolver_cache;
  int geometry_threads = 0;
  FullBallBatchResolver batch_resolver;
  bool propose_meb = true;  // anchor_meb_proposed, else the reference enumeration
  FullBallTimes unused_times;
  FullBallTimes* times;  // phase wall times (the caller's, or unused_times)
  bool overlap_static_lots = false;  // phase A of order K starts once phase 0 of K is done
  bool pipelined_tail = true;  // v9 E4: phases B and C inside the overlapped window
  RawVector<u8> first_order;   // v9 E4: first contributing order per ball, 0 if none <= kmax
  std::array<u64, kFacetMaxK + 2> population_offset{};  // v9 E4: #balls of first order < K
  // Phase-0 classes by exact hash classes (default) or by the sorted witness
  // (sort of the 56-byte requests by key and ordinal, then group starts).
  bool hashed_groups = true;
  FullBallStaticTrace* static_trace = nullptr;  // gates only
  using StaticSeed = FullBallBatchSeed;
  RawVector<BallId> static_targets;  // one target per request, every slot written
  // Arena of the phase-0 requests, reused from order to order (default
  // initialised: every slot is written by the parallel copy), released
  // after the last order's phase 0 (release_static_arena).
  RawVector<FullBallBatchRequest> static_request_arena;
  // Open-addressing table of the hashed grouping, reused from order to order:
  // zeroed by the chunks of the concatenation (every slot once) before use.
  RawVector<u64> group_table;
  // Seed index of the hashed grouping (same words: tag << 32 | seed + 1),
  // zeroed with the group table; the seeds are then never sorted.
  RawVector<u64> seed_table;
  void release_static_arena() {
    RawVector<FullBallBatchRequest>().swap(static_request_arena);
    RawVector<u64>().swap(group_table);
    RawVector<u64>().swap(seed_table);
  }
  size_t static_cursor = 0;
  std::vector<PointId> domain;
  std::vector<std::pair<PointId,i32>> identity;
  std::vector<BallId> by_key;
  // Exact open-addressing index of the (pairwise distinct) catalogue keys:
  // slot = BallId + 1, 0 empty, linear probing from the key's hash. A lookup
  // compares whole keys: a collision costs a probe, never a wrong ball.
  // Built in parallel by compare-and-swap; the slot layout may depend on the
  // schedule, the answer of every lookup does not.
  RawVector<u32> key_slots;  // zeroed in parallel by build_key_index (E2)
  u64 key_mask = 0;
  static u64 key_hash(const BallKey& key) {
    u64 h = 0x9e3779b97f4a7c15ull;
    const auto mix = [&](i128 value) {
      for (const u64 word : {static_cast<u64>(static_cast<u128>(value)), static_cast<u64>(static_cast<u128>(value) >> 64)}) {
        h ^= word + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h ^= h >> 31; h *= 0xbf58476d1ce4e5b9ull; h ^= h >> 29;
      }
    };
    mix(key.a); mix(key.b[0]); mix(key.b[1]); mix(key.b[2]); mix(key.c);
    return h;
  }
  void build_key_index() {
    u64 capacity = 16;
    while (capacity < 2 * static_cast<u64>(balls.size())) capacity *= 2;
    key_slots.resize(capacity);  // v9 E2: zeroed in parallel, not by a serial fill
    parallel_ranges(capacity, geometry_threads, [&](size_t begin, size_t end, size_t) {
      std::fill(key_slots.begin() + static_cast<std::ptrdiff_t>(begin),
                key_slots.begin() + static_cast<std::ptrdiff_t>(end), u32{0});
    });
    key_mask = capacity - 1;
    parallel_ranges(balls.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
      for (size_t id = begin; id < end; ++id) {
#if defined(MHGP9_KEY_INDEX_MUTANT_DROP_FIRST)
        if (id == 0) continue;  // mutant: one catalogue key never indexed
#endif
        u64 at = key_hash(balls[id].key) & key_mask;
        for (;;) {
          std::atomic_ref<u32> slot(key_slots[at]);
          u32 expected = 0;
          if (slot.load(std::memory_order_relaxed) == 0 &&
              slot.compare_exchange_strong(expected, static_cast<u32>(id + 1), std::memory_order_relaxed))
            break;
          at = (at + 1) & key_mask;
        }
      }
    });
  }
  // The BallId of `key` in the catalogue, or kAbsent32.
  u32 find_key(const BallKey& key) const {
    for (u64 at = key_hash(key) & key_mask;; at = (at + 1) & key_mask) {
      const u32 slot = key_slots[at];
      if (slot == 0) return std::numeric_limits<u32>::max();
      if (balls[slot - 1].key == key) return slot - 1;
    }
  }
  std::vector<std::vector<BallId>> programs;
  RawVector<u32> level_run;  // exact level run of each ball (validate_catalogue; every slot written)
  std::unordered_map<BallId, local_plateau::ShellTable> extra;
  std::vector<FullCoveragePopulation> populations;
  RawVector<u64> population_ids;  // filled with `absent` in parallel (validate_catalogue)
  std::vector<u64> anchors, compressed;
  History current;
  std::vector<NodeRef> stack;
  std::vector<Block> lot_blocks;  // reused slots of the current lot

  AnchorMebResult meb(std::span<const i32> sites, AnchorMebWork& work) const {
    std::array<P3, kFacetMaxK> positions{};
    require(!sites.empty() && sites.size() <= positions.size(), "full_ball_meb_cardinality");
    for (size_t j = 0; j < sites.size(); ++j) positions[j] = ix.upos[sites[j]];
    // Same result as the reference enumeration (anchor_meb.hpp), less work.
    const std::span<const P3> points(positions.data(), sites.size());
    auto result = propose_meb ? anchor_meb_proposed(points, work) : anchor_meb(points, work);
    require(result.status == AnchorMebStatus::kOk, "full_ball_meb_failure",
        result.status == AnchorMebStatus::kCounterOverflow ? FullBallStatus::kResourceExhausted
                                                         : FullBallStatus::kInvariantViolated);
    return result;
  }

  // Checks of one catalogue ball that read only immutable inputs (safe in
  // parallel); `checks` counts the declared-support certifications.
  void check_ball_locally(const BallData& ball, u64& checks) const {
    constexpr auto invalid = FullBallStatus::kInvalidInput;
    require(ball.arity >= 2 && ball.arity <= 4 && ball.n_shell >= ball.arity &&
        ball.n_shell <= kBallShellMax && ball.n_interior <= kBallInteriorMax &&
        ball.level.den > 0, "full_ball_census_shape", invalid);
    std::array<i32, kBallInteriorMax + kBallShellMax> selected{};
    size_t n = 0;
    for (const auto sites : {ball.interior(), ball.shell()}) for (i32 site : sites) {
      require(site >= 0 && static_cast<size_t>(site) < ix.upos.size(), "full_ball_geometry_index", invalid);
      selected[n++] = site;
    }
    std::sort(selected.begin(), selected.begin() + n);
    require(std::adjacent_find(selected.begin(), selected.begin() + n) == selected.begin() + n,
            "full_ball_repeated_census_site", invalid);
    // Bound BEFORE any power/axis arithmetic, including malformed callers.
    // u18 bounds (v9): q3 gives A<2^76, |B|<2^96, |C|<2^116 (auditor A);
    // q4 gives A=det<2^60, |B|<2^81, |C|<2^100; q2 is smaller.
    require(ball.key.a > 0 && ball.key.a < (i128{1} << 76) &&
        uabs128(ball.key.c) < (u128{1} << 116), "full_ball_key_domain", invalid);
    for (i128 b : ball.key.b) require(uabs128(b) < (u128{1} << 96), "full_ball_key_domain", invalid);
    for (i32 site : ball.interior()) require(ball.key.power(ix.upos[site]) < 0, "full_ball_census_power", invalid);
    for (i32 site : ball.shell()) require(ball.key.power(ix.upos[site]) == 0, "full_ball_census_power", invalid);
    if (ball.n_shell == ball.arity) {
      std::array<i32,4> support{};
      std::copy(ball.shell().begin(), ball.shell().end(), support.begin());
      validate_declared_support(ball, support, checks);
    }
  }

  void validate_declared_support(const BallData& ball, const std::array<i32, 4>& support, u64& checks) const {
    // The caller has already validated indices, primitive-domain bounds and
    // every interior/shell power. A positive declared support reproducing the
    // ball certifies it directly, without enumerating any of its sub-supports.
    std::array<P3, 4> positions{};
    for (size_t j = 0; j < ball.arity; ++j) positions[j] = ix.upos[support[j]];
    add(checks);
    anchor_meb_detail::Candidate candidate;
    require(anchor_meb_detail::form(std::span<const P3>(positions.data(), ball.arity),
        {0, 1, 2, 3}, ball.arity, candidate), "full_ball_census_geometry", FullBallStatus::kInvalidInput);
    BallKey key;
    ExactLevel level;
    if (ball.arity == 2) {
      key = q2_ball_key(candidate.a, candidate.b);
      level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
    } else if (ball.arity == 3) {
      key = q3_ball_key(candidate.three);
      level = promote_level(q3_exact_level(candidate.a, candidate.b, positions[2]));
    } else {
      key = ball_key_reduce(q4_ball_form(candidate.four));
      level = q4_level_raw(candidate.four);
    }
    require(key == ball.key && same_exact_level(level, ball.level),
        "full_ball_census_geometry", FullBallStatus::kInvalidInput);
  }

  void validate_catalogue() {
    constexpr auto invalid = FullBallStatus::kInvalidInput;
    auto lap_start = PhaseClock::now();
    const auto lap = [&](size_t part) {  // E0 sub-timers of the validation
      const auto now = PhaseClock::now();
      times->validate_parts[part] = std::chrono::duration<double, std::milli>(now - lap_start).count();
      lap_start = now;
    };
    require(requested > 0 && requested <= kFacetMaxK && ix.valid && !ix.upos.empty() &&
        !ix.has_duplicate_positions() && ix.input_count <= static_cast<u64>(std::numeric_limits<i32>::max()) &&
        balls.size() <= std::numeric_limits<BallId>::max(), "full_ball_input_domain", invalid);
    // As for existing spatial consumers, ix must be an authentic immutable
    // build_cloud_index result. No arbitrary hand-edited tree is certified.
    kmax = static_cast<unsigned>(std::min<u64>(requested, ix.input_count));
    for (size_t j = 0; j < ix.upos.size(); ++j) {
      require(p3_in_profile(ix.upos[j]), "full_ball_u18_domain", invalid);
      identity.emplace_back(ix.point_id(static_cast<i32>(j)), static_cast<i32>(j));
    }
    std::sort(identity.begin(), identity.end());
    for (const auto& row : identity) domain.push_back(row.first);
    require(std::adjacent_find(domain.begin(), domain.end()) == domain.end(), "full_ball_duplicate_id", invalid);
    lap(0);
    by_key.resize(balls.size()); std::iota(by_key.begin(), by_key.end(), BallId{0});
    // A catalogue already in STRICTLY increasing key order (the chain's) is
    // certified by this one scan: the identity is then the unique sorted
    // permutation and keys are distinct. Otherwise keys are sorted and must
    // be pairwise distinct: a strict total order, so the parallel sort is
    // the unique sorted permutation.
    // v9 E2: the scan in parallel chunks; chunk c judges the pairs (j-1, j)
    // for j in its range, so a pair across a chunk edge is judged by the
    // chunk on its right. Same boolean as the serial scan.
    bool presorted = true;
    {
      const size_t n = balls.size();
      const size_t chunks = std::max<size_t>(1, std::min<size_t>(n / 65536 + 1, 256));
      std::vector<unsigned char> ordered(chunks, 1);
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        const size_t begin = std::max<size_t>(1, n * c / chunks), end = n * (c + 1) / chunks;
        for (size_t j = begin; j < end; ++j)
          if (!(balls[j - 1].key < balls[j].key)) { ordered[c] = 0; return; }
      });
      for (const auto flag : ordered) presorted = presorted && flag != 0;
    }
    if (presorted) add(st.presorted_catalogues);
    else {
      parallel_sort(by_key, geometry_threads, [&](BallId a, BallId b) { return balls[a].key < balls[b].key; });
      for (size_t j = 1; j < by_key.size(); ++j)
        require(!(balls[by_key[j]].key == balls[by_key[j - 1]].key), "full_ball_duplicate_key", invalid);
    }
    lap(1);
    build_key_index();  // keys are distinct from here on
    lap(2);
    std::vector<size_t> counts(kmax + 1, 0);
    // Pass 1, parallel on the static path: the per-ball exact checks that
    // touch no shared state (shape, sites, key domain, census powers, and the
    // declared support of regular balls). Pass 2, serial in index order: the
    // plateau tables and witnesses of extended shells, rank windows, counts.
    // The reported failure is the first one in index order, as sequentially.
    const size_t absent_index = std::numeric_limits<size_t>::max();
    size_t first_local_failure = absent_index;
    Failure local_failure{FullBallStatus::kInvariantViolated, ""};
    {
      constexpr size_t block = 2048;
      const size_t blocks = (balls.size() + block - 1) / block;
      std::vector<size_t> failed_at(blocks, absent_index);
      std::vector<Failure> failures(blocks, local_failure);
      std::vector<u64> checks(blocks, 0);
      parallel_items(blocks, geometry_threads, [&](size_t chunk, size_t) {
        for (size_t j = chunk * block; j < std::min(balls.size(), (chunk + 1) * block); ++j) {
          try {
            check_ball_locally(balls[j], checks[chunk]);
          } catch (const Failure& failure) {
            failed_at[chunk] = j; failures[chunk] = failure;
            return;
          }
        }
      });
      for (size_t c = 0; c < blocks; ++c) {
        add(st.declared_support_checks, checks[c]);
        if (failed_at[c] != absent_index && first_local_failure == absent_index) {
          first_local_failure = failed_at[c]; local_failure = failures[c];
        }
      }
      lap(3);
    }
    // Pass 2. Regular balls in parallel (rank window, per-order counts; their
    // q_min is their arity), extended shells serially in index order (plateau
    // table, minimal support, MEB witness). The failure reported is the one
    // of the smallest index, a pass-1 failure first at equal index: exactly
    // the serial loop's.
    {
      constexpr size_t block = 4096;
      const size_t blocks = std::max<size_t>(1, (balls.size() + block - 1) / block);
      std::vector<size_t> rank_failed(blocks, absent_index);
      std::vector<std::vector<size_t>> block_counts(blocks, std::vector<size_t>(kmax + 1, 0));
      std::vector<std::vector<size_t>> block_extended(blocks);
      parallel_items(blocks, geometry_threads, [&](size_t chunk, size_t) {
        auto& local = block_counts[chunk];
        for (size_t j = chunk * block; j < std::min(balls.size(), (chunk + 1) * block); ++j) {
          const auto& ball = balls[j];
          if (ball.n_shell != ball.arity) block_extended[chunk].push_back(j);
          if (ball.n_interior + ball.arity > std::min<u64>(kmax + 1, ix.input_count)) {
            if (rank_failed[chunk] == absent_index) rank_failed[chunk] = j;
            continue;
          }
          const unsigned lo = ball.n_interior + ball.arity - 1;
          const unsigned hi = std::min<unsigned>(kmax, ball.n_interior + ball.n_shell);
          for (unsigned k = lo; k <= hi; ++k) ++local[k];
        }
      });
      size_t first_rank_failure = absent_index;
      for (const auto failed : rank_failed) first_rank_failure = std::min(first_rank_failure, failed);
      const size_t first_other = std::min(first_local_failure, first_rank_failure);
      for (const auto& part : block_extended)
        for (const size_t j : part) {
          if (j > first_other) break;
          if (j == first_local_failure) throw local_failure;
          const auto& ball = balls[j];
          std::array<i32,4> support{};
          local_plateau::LocalCensus local{ball.key, {}, {}};
          for (i32 site : ball.interior()) local.interior.push_back({ix.point_id(site), ix.upos[site]});
          for (i32 site : ball.shell()) local.shell.push_back({ix.point_id(site), ix.upos[site]});
          auto table = local_plateau::ShellTable::prepare(std::move(local));
          const unsigned q = table.q_min();
          require(q == ball.arity, "full_ball_minimum_arity", invalid);
          const auto mask = table.minimal_supports().front();
          unsigned at = 0;
          for (size_t bit = 0; bit < table.census().shell.size(); ++bit)
            if (mask & (u16{1} << bit)) support[at++] = geometry_id(table.census().shell[bit].id);
          require(at == q, "full_ball_minimum_support");
          extra.emplace(static_cast<BallId>(j), std::move(table)); add(st.extra_records);
          const auto witness = meb(std::span<const i32>(support.data(), q), st.validation_work);
          require(witness.support_size == q && witness.key == ball.key &&
              same_exact_level(witness.level, ball.level), "full_ball_census_geometry", invalid);
          require(j != first_rank_failure, "full_ball_outside_rank_window", invalid);
        }
      if (first_local_failure != absent_index && first_local_failure <= first_rank_failure) throw local_failure;
      require(first_rank_failure == absent_index, "full_ball_outside_rank_window", invalid);
      for (const auto& local : block_counts)
        for (unsigned k = 1; k <= kmax; ++k) counts[k] += local[k];
      add(st.records, balls.size());
    }
    lap(4);
    // Same order as a stable sort of by_key under compare_exact_level: the
    // certified double filter decides clear gaps, the exact U320 comparison
    // decides the rest, and equal levels keep their by_key rank.
    auto by_level = by_key;
    RawVector<double> approx;  // certified filter values (every slot written), reused by the level runs
    if (std::fegetround() == FE_TONEAREST) {
      approx.resize(balls.size());
      RawVector<BallId> rank(balls.size());  // v9 E2: every slot written below, in parallel
      parallel_ranges(by_key.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
        for (size_t j = begin; j < end; ++j) {
          rank[by_key[j]] = static_cast<BallId>(j);
          approx[by_key[j]] = level_approximation(balls[by_key[j]].level);
        }
      });
      // Ties end on the by_key rank: a strict total order, parallel sorted.
      parallel_sort(by_level, geometry_threads, [&](BallId a, BallId b) {
        const double x = approx[a], y = approx[b];
        if (x < y * kLevelFilterMargin) return true;
        if (y < x * kLevelFilterMargin) return false;
        const int cmp = compare_exact_level(balls[a].level, balls[b].level);
        return cmp != 0 ? cmp < 0 : rank[a] < rank[b];
      });
    } else {
      std::stable_sort(by_level.begin(), by_level.end(), [&](BallId a, BallId b) {
        return compare_exact_level(balls[a].level, balls[b].level) < 0;
      });
    }
    // Exact level runs: equal exact levels share one run index, increasing
    // along by_level, so that for catalogue balls level(a) < level(b) <=>
    // level_run[a] < level_run[b] and equality <=> equal runs. The certified
    // double filter separates clear gaps; the exact comparison decides the
    // rest. Phase 0 and phase A then compare runs instead of U320 products.
    lap(5);
    level_run.resize(balls.size());  // v9 E2: every slot written by the second pass below
    {
      // Parallel in two passes: run starts and their count per chunk, then
      // chunk offsets and the runs themselves.
      const size_t n = by_level.size();
      const size_t chunks = std::max<size_t>(1, std::min<size_t>(n / 65536 + 1, 256));
      const auto range = [&](size_t c) { return std::pair<size_t, size_t>{n * c / chunks, n * (c + 1) / chunks}; };
      std::vector<unsigned char> starts(n, 0);
      std::vector<u64> offsets(chunks, 0);
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        const auto [begin, end] = range(c);
        u64 count = 0;
        for (size_t j = std::max<size_t>(1, begin); j < end; ++j) {
          const BallId a = by_level[j - 1], b = by_level[j];
          const bool clear = !approx.empty() && (approx[a] < approx[b] * kLevelFilterMargin ||
                                                 approx[b] < approx[a] * kLevelFilterMargin);
          const bool differ = clear || !same_exact_level(balls[a].level, balls[b].level);
          starts[j] = differ ? 1 : 0;
          count += differ ? 1 : 0;
        }
        offsets[c] = count;
      });
      u64 total = 0;
      for (auto& offset : offsets) { const u64 count = offset; offset = total; total += count; }
      require(total < kAbsent32, "full_ball_level_run_representation", FullBallStatus::kResourceExhausted);
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        const auto [begin, end] = range(c);
        u64 run = offsets[c];
        for (size_t j = begin; j < end; ++j) {
          if (j != 0 && starts[j] != 0) ++run;
          level_run[by_level[j]] = static_cast<u32>(run);
        }
      });
    }
    lap(6);
    // Programs in by_level order, filled in parallel at per-chunk offsets
    // (counts, prefix, scatter): the same vectors as the serial append.
    programs.resize(kmax + 1);
    {
      const size_t chunks = std::max<size_t>(1, std::min<size_t>(by_level.size() / 65536 + 1, 256));
      std::vector<std::vector<size_t>> at(chunks, std::vector<size_t>(kmax + 1, 0));
      const auto range = [&](size_t c) {
        return std::pair<size_t, size_t>{by_level.size() * c / chunks, by_level.size() * (c + 1) / chunks};
      };
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        const auto [begin, end] = range(c);
        for (size_t j = begin; j < end; ++j) {
          const auto& ball = balls[by_level[j]];
          const unsigned hi = std::min<unsigned>(kmax, ball.n_interior + ball.n_shell);
          for (unsigned k = ball.n_interior + ball.arity - 1; k <= hi; ++k) ++at[c][k];
        }
      });
      for (unsigned k = 1; k <= kmax; ++k) {
        size_t running = 0;
        for (size_t c = 0; c < chunks; ++c) { const size_t n = at[c][k]; at[c][k] = running; running += n; }
        require(running == counts[k], "full_ball_program_count");
        programs[k].resize(running);
      }
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        const auto [begin, end] = range(c);
        auto& position = at[c];
        for (size_t j = begin; j < end; ++j) {
          const BallId b = by_level[j];
          const auto& ball = balls[b];
          const unsigned hi = std::min<unsigned>(kmax, ball.n_interior + ball.n_shell);
          for (unsigned k = ball.n_interior + ball.arity - 1; k <= hi; ++k) programs[k][position[k]++] = b;
        }
      });
    }
    population_ids.resize(balls.size());
    parallel_ranges(balls.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
      std::fill(population_ids.begin() + static_cast<std::ptrdiff_t>(begin),
                population_ids.begin() + static_cast<std::ptrdiff_t>(end), absent);
    });
    if (pipelined_populations()) compute_population_offsets();  // v9 E4
    lap(7);
  }

  i32 geometry_id(PointId id) const {
    const auto found = std::lower_bound(identity.begin(), identity.end(), std::pair<PointId,i32>{id, -1});
    require(found != identity.end() && found->first == id, "full_ball_unknown_identity");
    return found->second;
  }
  u64 root(u64 token, u64 prior_count) {
    require(token < prior_count, "full_ball_anchor_not_prior");
    u64 r = token;
    while (compressed[r] != r) r = compressed[r];
    while (compressed[token] != token) {
      const auto next = compressed[token]; compressed[token] = r; token = next;
    }
    require(r < prior_count && current.next[r] == absent, "full_ball_root_not_prior");
    return r;
  }
  i32 intruder_work(const BallKey& key, std::span<const i32> selected,
      FullBallStats& work, std::vector<NodeRef>& scratch) const {
    add(work.intruder_queries);
    const auto member = [&](i32 u) { return std::binary_search(selected.begin(), selected.end(), u); };
    const census_detail::AxisBounds bounds(key);
    scratch.clear(); scratch.push_back(ix.root());
    while (!scratch.empty()) {
      const auto node = scratch.back(); scratch.pop_back(); add(work.intruder_nodes);
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        add(work.interior_ranges);
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u)) {
          add(work.intruder_power_tests);
          if (key.power(ix.upos[u]) < 0) return u;
        }
      } else {
        scratch.push_back(ix.nodes[node].right); scratch.push_back(ix.nodes[node].left);
      }
    }
    return -1;
  }
  i32 intruder(const BallKey& key, std::span<const i32> selected) {
    return intruder_work(key, selected, st, stack);
  }

  // Geometry-only: immutable index/catalogue and rank, never anchors or DSU.
  // find_seed(key): the seed of exactly this key or nullptr (sorted witness:
  // binary search of the sorted seeds; hashed grouping: the seed index).
  template <class FindSeed>
  BallId static_terminal(ResolverCache::Key key, const ExactLevel& before,
      FullBallStats& work, std::vector<NodeRef>& scratch, FindSeed&& find_seed) const {
    std::span<i32> sites(key.data(), current_k);
    auto local = meb(sites, work.resolve_work);
    u64 length = 0;
    for (;;) {
      require(compare_exact_level(local.level, before) < 0, "full_ball_static_not_strict");
      add(work.key_lookups);
      const u32 found = find_key(local.key);
      if (found != kAbsent32) {
        const auto& b = balls[found];
        require(same_exact_level(b.level, local.level), "full_ball_static_anchor_level");
        const unsigned lo = b.n_interior + b.arity - 1;
        if (current_k >= lo && current_k <= b.n_interior + b.n_shell) {
          add(work.anchor_hits); work.max_chain_steps = std::max(work.max_chain_steps, length);
          return found;
        }
      }
      const i32 z = intruder_work(local.key, sites, work, scratch);
      require(z >= 0, "full_ball_static_missing_weak_terminal");
      require(local.support_size > 0 && local.support_slots[0] < sites.size(),
              "full_ball_static_missing_support");
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      add(work.static_post_seed_queries[current_k]);
      if (const StaticSeed* seed = find_seed(key)) {
        const auto& b = balls[seed->ball];
        // Whole I union U, in this Builder's index and K. A surviving support
        // or partial shell is NOT a seed: it can preserve the previous radius.
        require(current_k == static_cast<unsigned>(b.n_interior) + b.n_shell,
                "full_ball_post_seed_whole_population");
        require(compare_exact_level(b.level, local.level) < 0 && compare_exact_level(b.level, before) < 0,
                "full_ball_post_seed_not_strict");
        add(work.static_post_seed_hits[current_k]);
        add(work.descending_steps); add(length);
        work.max_chain_steps = std::max(work.max_chain_steps, length);
        add(work.static_post_seed_terminals[current_k]);
        // Exact full-population MEB is this ball; the original next iteration
        // would terminate here. Do not charge its MEB/lookup/anchor hit.
        return seed->ball;
      }
      auto next = meb(sites, work.resolve_work);
      const int cmp = compare_exact_level(next.level, local.level);
      require(cmp <= 0, "full_ball_static_radius_increased");
      if (cmp == 0) {
        require(next.key == local.key && next.selected_shell_count + 1 == local.selected_shell_count,
                "full_ball_static_equal_radius_shell_not_decreased");
        add(work.same_radius_steps);
      } else add(work.descending_steps);
      add(length); local = next;
    }
  }

  void merge_static_work(const FullBallStats& w) {
    for (size_t k = 0; k < st.static_post_seed_queries.size(); ++k) {
      add(st.static_post_seed_queries[k], w.static_post_seed_queries[k]);
      add(st.static_post_seed_hits[k], w.static_post_seed_hits[k]);
      add(st.static_post_seed_terminals[k], w.static_post_seed_terminals[k]);
    }
    for (const auto field : {&FullBallStats::anchor_hits, &FullBallStats::key_lookups,
        &FullBallStats::intruder_queries, &FullBallStats::intruder_nodes,
        &FullBallStats::intruder_power_tests, &FullBallStats::interior_ranges,
        &FullBallStats::same_radius_steps, &FullBallStats::descending_steps}) add(st.*field, w.*field);
    st.max_chain_steps = std::max(st.max_chain_steps, w.max_chain_steps);
    add(st.resolve_work.calls, w.resolve_work.calls);
    add(st.resolve_work.power_tests, w.resolve_work.power_tests);
    add(st.resolve_work.materializations, w.resolve_work.materializations);
    add(st.resolve_work.pair_distances, w.resolve_work.pair_distances);
    for (size_t q = 0; q < st.resolve_work.supports_by_size.size(); ++q)
      add(st.resolve_work.supports_by_size[q], w.resolve_work.supports_by_size[q]);
    add(st.resolve_work.proposals, w.resolve_work.proposals);
    add(st.resolve_work.verified_proposals, w.resolve_work.verified_proposals);
    add(st.resolve_work.boundary_canonicalizations, w.resolve_work.boundary_canonicalizations);
    add(st.resolve_work.proposal_fallbacks, w.resolve_work.proposal_fallbacks);
  }

  void prepare_external_batch(const RawVector<FullBallBatchRequest>& requests,
      const std::vector<size_t>& groups, const RawVector<StaticSeed>& seeds) {
    const BallId missing = std::numeric_limits<BallId>::max();
    std::vector<BallId> unique_targets(groups.size() - 1, missing);
    std::vector<FullBallBatchRequest> pending;
    FullBallBatchResult result;
    bool invoked = false;
    const auto account = [&] {
      // Account paid geometry before fallible capacity arithmetic. A partial
      // global counter merge cannot retain a claim that all work is known.
      if (result.work_known) {
        try { merge_static_work(result.work); }
        catch (...) { st.static_batch_work_known = false; throw; }
      } else if (invoked) st.static_batch_work_known = false;
      u64 bytes = pending.capacity() * sizeof(FullBallBatchRequest);
      add(bytes, unique_targets.capacity() * sizeof(BallId));
      add(bytes, result.targets.capacity() * sizeof(FullBallBatchTarget));
      add(bytes, result.retained_bytes);
      st.static_peak_batch_bytes = std::max(st.static_peak_batch_bytes, bytes);
      const u64 seed_bytes = seeds.capacity() * sizeof(StaticSeed);
      const u64 group_bytes = groups.capacity() * sizeof(size_t);
      st.static_peak_seed_bytes = std::max(st.static_peak_seed_bytes, seed_bytes);
      st.static_peak_group_bytes = std::max(st.static_peak_group_bytes, group_bytes);
      add(bytes, requests.capacity() * sizeof(FullBallBatchRequest));
      add(bytes, static_targets.capacity() * sizeof(BallId));
      add(bytes, seed_bytes); add(bytes, group_bytes);
      st.static_peak_retained_bytes = std::max(st.static_peak_retained_bytes, bytes);
    };
    try {
      for (size_t group = 0; group + 1 < groups.size(); ++group) {
        const auto& first = requests[groups[group]];
        const auto seed = std::lower_bound(seeds.begin(), seeds.end(), first.key,
            [](const StaticSeed& a, const FullBallFacetKey& key) { return a.key < key; });
        if (seed != seeds.end() && seed->key == first.key) {
          require(compare_exact_level(balls[seed->ball].level, balls[first.consumer].level) < 0,
                  "full_ball_static_seed_not_strict");
          unique_targets[group] = seed->ball;
          add(st.static_seeded[current_k]);
        } else pending.push_back(first);
      }
      if (!pending.empty()) {
        add(st.static_batch_calls);
        invoked = true;
        batch_resolver.resolve(batch_resolver.owner, {ix, balls, by_key},
            {current_k, seeds, pending}, result);
      } else result.work_known = true;  // no callback and no geometric work
    } catch (const std::bad_alloc&) { account(); throw; }
      catch (const std::length_error&) { account(); throw; }
      catch (const std::system_error&) { account(); throw; }
      catch (const Failure&) { account(); throw; }
      catch (...) {
        account();
        throw Failure{FullBallStatus::kInvariantViolated, "full_ball_batch_exception"};
      }
    account();  // retain paid work even if validation below rejects publication
    if (!pending.empty()) {
      if (result.status != FullBallStatus::kCompleteRelative) {
        require(result.targets.empty(), "full_ball_batch_partial_publication");
        require(result.status == FullBallStatus::kInvalidInput ||
            result.status == FullBallStatus::kResourceExhausted ||
            result.status == FullBallStatus::kInvariantViolated, "full_ball_batch_status");
        throw Failure{result.status, result.reason ? result.reason : "full_ball_batch_failed"};
      }
      require(result.work_known, "full_ball_batch_success_work_unknown");
      require(result.targets.size() == pending.size(), "full_ball_batch_target_count");
      const auto& w = result.work;
      const u64 q = w.static_post_seed_queries[current_k], h = w.static_post_seed_hits[current_k];
      const u64 t = w.static_post_seed_terminals[current_k];
      u64 steps = w.descending_steps; add(steps, w.same_radius_steps);
      u64 paid = w.anchor_hits; add(paid, w.intruder_queries);
      u64 terminals = w.anchor_hits; add(terminals, t);
      require(q == steps && h == t && h <= std::min<u64>(steps, pending.size()) &&
          w.resolve_work.calls == paid && terminals == pending.size() &&
          w.intruder_queries == steps && w.key_lookups == w.resolve_work.calls,
          "full_ball_batch_work_identity");
      for (size_t k = 0; k < w.static_post_seed_queries.size(); ++k)
        require(k == current_k || (!w.static_post_seed_queries[k] && !w.static_post_seed_hits[k] &&
            !w.static_post_seed_terminals[k]), "full_ball_batch_work_order");
      for (size_t j = 0; j < pending.size(); ++j) {
        const auto& target = result.targets[j];
        require(target.ordinal == pending[j].ordinal, "full_ball_batch_target_ordinal");
        require(target.ball < balls.size(), "full_ball_batch_target_domain");
        const auto& b = balls[target.ball];
        require(current_k >= static_cast<unsigned>(b.n_interior) + b.arity - 1 &&
            current_k <= b.n_interior + b.n_shell, "full_ball_batch_target_rank");
        require(compare_exact_level(b.level, balls[pending[j].consumer].level) < 0,
                "full_ball_batch_target_not_strict");
      }
    }
    size_t next = 0;
    for (size_t group = 0; group + 1 < groups.size(); ++group) {
      if (unique_targets[group] == missing) unique_targets[group] = result.targets[next++].ball;
      const auto& before = balls[requests[groups[group]].consumer].level;
      for (size_t r = groups[group]; r < groups[group + 1]; ++r) {
        require(compare_exact_level(before, balls[requests[r].consumer].level) <= 0,
                "full_ball_static_request_chronology");
        static_targets[requests[r].ordinal] = unique_targets[group];
      }
    }
    require(next == pending.size(), "full_ball_batch_unconsumed_results");
  }

  // Insertion sort of the first n <= kFacetMaxK sites of a facet key.
  static void sort_prefix(ResolverCache::Key& key, size_t n) {
    require(n <= key.size(), "full_ball_facet_cardinality");
    for (size_t i = 1; i < n; ++i)
      for (size_t j = i; j > 0 && key[j] < key[j - 1]; --j) std::swap(key[j], key[j - 1]);
  }

  // ---- Hashed grouping of phase 0 (default; the sorted witness is kept).
  //
  // The resolution needs the partition of the requests into classes of equal
  // facet key, the FIRST request of each class (minimum ordinal: its consumer
  // gives `before`) and one target per class. The 56-byte requests are never
  // moved: one open-addressing table of 8-byte words, (hash tag << 32) |
  // (ordinal + 1), finds the class slot of every request in one parallel
  // pass. A tag match is only a candidate: the class is decided by comparing
  // the WHOLE keys (a collision probes on, never merges two keys). The word
  // keeps the minimum ordinal of its class by compare-and-swap. The seeds
  // are not sorted either: a second table of the same words indexes them
  // (exact compare, duplicates refused on insertion) for the seed lookups of
  // the class and of static_terminal. Each class is then resolved once, in
  // smallest-site order (same static_terminal as the witness, whose inputs
  // are only the key and the first consumer), its word becomes (target <<
  // 32) | first consumer's level run, and a gather pass gives every request
  // its target and checks its chronology. Same classes, same firsts, same
  // targets and same counters (sums and maxima over classes); only the order
  // in which classes are resolved changes, and the table layouts may depend
  // on the schedule, never an answer.
  static constexpr size_t kHashGroupMaxRequests = size_t{1} << 31;  // ordinal + 1 and slots in 32 bits
  static constexpr u64 kLowHalf = 0xffffffffull;
  // Prefetch distances: home slots of the grouping pass; table words, first
  // requests and first consumers of the resolution (three stages).
  static constexpr size_t kGroupAhead = 16, kClassWordAhead = 24, kClassFirstAhead = 12, kClassBallAhead = 4;
  static u64 group_hash(const FullBallFacetKey& key) {
#if defined(MHGP9_TOWER_GROUP_TEST_WEAK_HASH)
    // Test build only: four hash values in all, so that distinct keys share
    // tag and home slot and the exact key compare must split them.
    u64 h = static_cast<u64>(static_cast<u32>(key[0]) & 3U) + 1;
#else
    u64 h = 0x9e3779b97f4a7c15ull;
    for (size_t j = 0; j + 1 < key.size(); j += 2) {
      h ^= static_cast<u64>(static_cast<u32>(key[j])) | (static_cast<u64>(static_cast<u32>(key[j + 1])) << 32);
      h *= 0xbf58476d1ce4e5b9ull; h ^= h >> 31;
    }
#endif
    h ^= h >> 33; h *= 0xff51afd7ed558ccdull; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ull; h ^= h >> 33;
    return h;
  }

  // Seed index: the seeds stay in collection order; an equal key found on
  // insertion is the witness's duplicate refusal.
  void index_seeds(const RawVector<StaticSeed>& seeds, size_t slots) {
    const u64 mask = static_cast<u64>(slots) - 1;
    parallel_ranges(seeds.size(), geometry_threads, [&](size_t begin, size_t end, size_t) {
      for (size_t i = begin; i < end; ++i) {
        const u64 h = group_hash(seeds[i].key);
        const u64 word = (h & ~kLowHalf) | static_cast<u64>(i + 1);
        for (u64 at = h & mask;; at = (at + 1) & mask) {
          std::atomic_ref<u64> slot(seed_table[at]);
          u64 seen = slot.load(std::memory_order_relaxed);
          if (seen == 0 && slot.compare_exchange_strong(seen, word, std::memory_order_relaxed)) break;
          require((seen ^ word) >> 32 || seeds[(seen & kLowHalf) - 1].key != seeds[i].key,
                  "full_ball_static_duplicate_seed");
        }
      }
    });
  }
  // The seed of exactly `key`, or nullptr; `rejects` counts tag matches
  // refused by the exact key compare (diagnostic).
  const StaticSeed* find_indexed_seed(const RawVector<StaticSeed>& seeds, size_t slots,
                                      const FullBallFacetKey& key, u64& rejects) const {
    if (!slots) return nullptr;
    const u64 mask = static_cast<u64>(slots) - 1, h = group_hash(key);
    for (u64 at = h & mask;; at = (at + 1) & mask) {
      const u64 seen = seed_table[at];
      if (seen == 0) return nullptr;
      if ((seen ^ h) >> 32) continue;  // another tag: another key
      const StaticSeed& seed = seeds[(seen & kLowHalf) - 1];
#if defined(MHGP9_TOWER_GROUP_MUTANT_SEED_TRUST_HASH)
      static_cast<void>(rejects);  // mutant: a tag match taken for the seed's key
#else
      if (seed.key != key) { ++rejects; continue; }  // exact: the whole key
#endif
      return &seed;
    }
  }

  template <class Lap>
  void resolve_hashed_order(const RawVector<FullBallBatchRequest>& requests, const RawVector<StaticSeed>& seeds,
                            size_t slots, size_t seed_slots, Lap& lap) {
    const size_t n = requests.size();
    static_cursor = 0;
    // Every slot is written by the grouping pass (the class slot), then
    // replaced by the target in the gather pass.
    static_targets.resize(n);
    poison_unwritten(static_targets, 0);
    st.static_peak_target_bytes = std::max<u64>(st.static_peak_target_bytes,
        static_targets.capacity() * sizeof(BallId));
    const u64 mask = static_cast<u64>(slots) - 1;
    // Locality: classes are resolved by increasing smallest site (buckets of
    // the Morton-ordered site index, at most 2^16), close to the key order of
    // the witness: neighbouring facets share their terminal balls and index
    // nodes (resolution in hash order measured +25 % thread CPU at K5).
    const u64 sites = ix.upos.size();
    const u64 buckets = std::max<u64>(1, std::min<u64>(sites, u64{1} << 16));
    std::vector<u32> bucket_at(buckets + 1, 0);  // counts, then cursors
    const auto bucket_of = [&](const FullBallFacetKey& key) {
      const u64 site = static_cast<u64>(static_cast<u32>(key[0]));  // a site index of the catalogue
      return static_cast<u32>(std::min<u64>(buckets - 1, site * buckets / std::max<u64>(1, sites)));
    };
    // New classes of each lane: (bucket, slot), in the lane's own order.
    std::vector<std::vector<std::array<u32, 2>>> fresh(planned_workers(n, geometry_threads));
    std::vector<std::array<u64, 2>> diagnostics(fresh.size(), std::array<u64, 2>{});
    parallel_ranges(n, geometry_threads, [&](size_t begin, size_t end, size_t lane) {
      auto& mine = fresh[lane];
      u64 rejects = 0, probes = 0;
      // Home slots are hashed kGroupAhead requests ahead and prefetched.
      std::array<u64, kGroupAhead> ahead{};
      for (size_t j = begin; j < std::min(end, begin + kGroupAhead); ++j) {
        ahead[j % kGroupAhead] = group_hash(requests[j].key);
        __builtin_prefetch(&group_table[ahead[j % kGroupAhead] & mask]);
      }
      for (size_t i = begin; i < end; ++i) {
        const auto& key = requests[i].key;
        const u64 h = ahead[i % kGroupAhead];
        if (i + kGroupAhead < end) {
          const u64 later = group_hash(requests[i + kGroupAhead].key);
          ahead[i % kGroupAhead] = later;
          __builtin_prefetch(&group_table[later & mask]);
        }
        const u64 word = (h & ~kLowHalf) | static_cast<u64>(i + 1);
        u64 at = h & mask;
        for (;; at = (at + 1) & mask, ++probes) {
          std::atomic_ref<u64> slot(group_table[at]);
          u64 seen = slot.load(std::memory_order_relaxed);
          if (seen == 0) {
            if (slot.compare_exchange_strong(seen, word, std::memory_order_relaxed)) {
              const u32 bucket = bucket_of(key);  // new class, first request so far
              mine.push_back({bucket, static_cast<u32>(at)});
              std::atomic_ref<u32>(bucket_at[bucket]).fetch_add(1, std::memory_order_relaxed);
              break;
            }
          }
          if ((seen ^ word) >> 32) continue;  // another tag: another key
#if !defined(MHGP9_TOWER_GROUP_MUTANT_TRUST_HASH)
          // Exact: the whole key, never the hash alone.
          if (requests[(seen & kLowHalf) - 1].key != key) { ++rejects; continue; }
#endif
          // Same key: the word keeps the minimum ordinal (only requests of
          // this class ever write this slot again).
#if defined(MHGP9_TOWER_GROUP_MUTANT_LAST_ORDINAL)
          while ((seen & kLowHalf) < i + 1 &&  // mutant: the class keeps its LAST request
                 !slot.compare_exchange_weak(seen, word, std::memory_order_relaxed)) {}
#else
          while ((seen & kLowHalf) > i + 1 &&
                 !slot.compare_exchange_weak(seen, word, std::memory_order_relaxed)) {}
#endif
          break;
        }
        static_targets[i] = static_cast<BallId>(at);  // the class slot until the gather
      }
      diagnostics[lane][0] += rejects; diagnostics[lane][1] += probes;  // several ranges per lane
    });
    u64 fresh_bytes = bucket_at.capacity() * sizeof(u32);
    for (const auto& part : fresh) add(fresh_bytes, part.capacity() * sizeof(part.front()));
    u32 total = 0;  // < 2^31 classes (at most one per request)
    for (auto& at : bucket_at) { const u32 c = at; at = total; total += c; }
    RawVector<u32> classes(total);  // every slot written once by the scatter
    poison_unwritten(classes, 0);
    parallel_items(fresh.size(), geometry_threads, [&](size_t lane, size_t) {
      for (const auto& [bucket, slot] : fresh[lane])
        classes[std::atomic_ref<u32>(bucket_at[bucket]).fetch_add(1, std::memory_order_relaxed)] = slot;
      std::vector<std::array<u32, 2>>().swap(fresh[lane]);
    });
    st.static_unique[current_k] = classes.size();
    if (static_trace) {  // gates only
      auto& firsts = static_trace->firsts[current_k];
      firsts.resize(n);
      for (size_t i = 0; i < n; ++i) firsts[i] = (group_table[static_targets[i]] & kLowHalf) - 1;
      static_trace->path[current_k] = 2;
      static_trace->tag_rejects[current_k] = static_trace->probes[current_k] = 0;
      for (const auto& d : diagnostics) {
        static_trace->tag_rejects[current_k] += d[0];
        static_trace->probes[current_k] += d[1];
      }
    }
    lap(times->static_groups_by_k);
    struct Worker { FullBallStats work; std::vector<NodeRef> scratch; u64 seeded = 0, seed_rejects = 0; };
    std::vector<Worker> workers(planned_workers(classes.size(), geometry_threads));
    const auto account = [&] {
      u64 worker_bytes = workers.capacity() * sizeof(Worker);
      for (const auto& w : workers) add(worker_bytes, w.scratch.capacity() * sizeof(NodeRef));
      u64 seed_bytes = seeds.capacity() * sizeof(StaticSeed);
      add(seed_bytes, seed_table.capacity() * sizeof(u64));
      u64 group_bytes = group_table.capacity() * sizeof(u64);
      add(group_bytes, classes.capacity() * sizeof(u32)); add(group_bytes, fresh_bytes);
      st.static_peak_seed_bytes = std::max(st.static_peak_seed_bytes, seed_bytes);
      st.static_peak_group_bytes = std::max(st.static_peak_group_bytes, group_bytes);
      st.static_peak_worker_bytes = std::max(st.static_peak_worker_bytes, worker_bytes);
      u64 retained = requests.capacity() * sizeof(FullBallBatchRequest);
      add(retained, static_targets.capacity() * sizeof(BallId));
      add(retained, seed_bytes); add(retained, group_bytes); add(retained, worker_bytes);
      // Sampled retained capacities only, not transient reallocations or RSS.
      st.static_peak_retained_bytes = std::max(st.static_peak_retained_bytes, retained);
      for (const auto& w : workers) {
        merge_static_work(w.work);
        add(st.static_seeded[current_k], w.seeded);
        if (static_trace) static_trace->seed_tag_rejects[current_k] += w.seed_rejects;
      }
    };
    if (static_trace) static_trace->seed_tag_rejects[current_k] = 0;
    const u64 seed_mask = static_cast<u64>(seed_slots) - 1;
    const auto job = [&](size_t begin, size_t end, size_t worker) {
      auto& w = workers[worker];
      const auto find_seed = [&](const FullBallFacetKey& key) {
        return find_indexed_seed(seeds, seed_slots, key, w.seed_rejects);
      };
      for (size_t c = begin; c < end; ++c) {
        // Prefetch pipeline over the classes of this range (a word read
        // ahead is still a first ordinal: only this range writes its slots).
        if (c + kClassWordAhead < end) __builtin_prefetch(&group_table[classes[c + kClassWordAhead]]);
        if (c + kClassFirstAhead < end)
          __builtin_prefetch(&requests[(group_table[classes[c + kClassFirstAhead]] & kLowHalf) - 1]);
        if (c + kClassBallAhead < end) {
          const auto& later = requests[(group_table[classes[c + kClassBallAhead]] & kLowHalf) - 1];
          __builtin_prefetch(&balls[later.consumer].level);
          __builtin_prefetch(&level_run[later.consumer]);
          if (seed_slots) __builtin_prefetch(&seed_table[group_hash(later.key) & seed_mask]);
        }
        u64& word = group_table[classes[c]];  // this class's slot, written by this job only
        const auto& first = requests[(word & kLowHalf) - 1];
        const auto& before = balls[first.consumer].level;
        BallId target;
        if (const StaticSeed* seed = find_seed(first.key)) {
          target = seed->ball;
          require(level_run[target] < level_run[first.consumer], "full_ball_static_seed_not_strict");
          add(w.seeded);
        } else target = static_terminal(first.key, before, w.work, w.scratch, find_seed);
        word = (static_cast<u64>(target) << 32) | level_run[first.consumer];
      }
    };
    try {
      const size_t lanes = parallel_ranges(classes.size(), geometry_threads, job);
      add(st.static_lanes_used, lanes);
      if (lanes > 1) add(st.static_workers_created, lanes);  // one lane uses the caller
    }
    catch (...) {
      account();
      throw;
    }
    account();
    // Gather: each request reads its class's target; the chronology of the
    // witness (no request of a class precedes its first request's level).
    parallel_ranges(n, geometry_threads, [&](size_t begin, size_t end, size_t) {
      for (size_t i = begin; i < end; ++i) {
        if (i + kGroupAhead < end) {
          __builtin_prefetch(&group_table[static_targets[i + kGroupAhead]]);
          __builtin_prefetch(&level_run[requests[i + kGroupAhead].consumer]);
        }
        const u64 word = group_table[static_targets[i]];
        require(static_cast<u32>(word & kLowHalf) <= level_run[requests[i].consumer],
                "full_ball_static_request_chronology");
        static_targets[i] = static_cast<BallId>(word >> 32);
      }
    });
    lap(times->static_resolve_by_k);
    trace_static_targets();
  }

  void prepare_static_order() {
    failpoint_after_static(current_k);
    failpoint_static_allocation(current_k);
    auto lap_start = PhaseClock::now();
    const auto lap = [&](std::array<double, 11>& into) {  // E0 sub-timers of phase 0
      const auto now = PhaseClock::now();
      into[current_k] = std::chrono::duration<double, std::milli>(now - lap_start).count();
      lap_start = now;
    };
    using Request = FullBallBatchRequest;
    auto& requests = static_request_arena;
    RawVector<StaticSeed> seeds;
    // Collection by fixed blocks of the program, in parallel on the static
    // path, then concatenated in program order: ordinals are the positions of
    // the sequential collection (same requests, same order, any thread count).
    const auto& program = programs[current_k];
    constexpr size_t block = 4096;
    struct Collected { std::vector<std::pair<ResolverCache::Key, BallId>> requests; std::vector<StaticSeed> seeds; };
    std::vector<Collected> collected((program.size() + block - 1) / block);
    parallel_items(collected.size(), geometry_threads, [&](size_t chunk, size_t) {
      auto& out = collected[chunk];
      for (size_t j = chunk * block; j < std::min(program.size(), (chunk + 1) * block); ++j) {
        const BallId id = program[j];
        u16 contribution = 0; bool interior = false;
        visit_block(id, contribution, interior, [&](std::span<const i32> facet) {
          require(facet.size() == current_k && facet.size() <= kFacetMaxK,
                  "full_ball_static_representative_cardinality");
          auto key = ResolverCache::key(facet);
          sort_prefix(key, facet.size());
          require(std::adjacent_find(key.begin(), key.begin() + facet.size()) == key.begin() + facet.size(),
                  "full_ball_static_representative_cardinality");
          out.requests.emplace_back(key, id);
        });
        const auto& b = balls[id];
        if (current_k == static_cast<unsigned>(b.n_interior) + b.n_shell) {
          ResolverCache::Key key{};
          size_t n = 0;
          for (i32 site : b.interior()) { require(n < key.size(), "full_ball_static_seed_cardinality"); key[n++] = site; }
          for (i32 site : b.shell()) { require(n < key.size(), "full_ball_static_seed_cardinality"); key[n++] = site; }
          sort_prefix(key, n);
          out.seeds.push_back({key, id});
        }
      }
    });
    // Concatenation in program order, in parallel at precomputed offsets: the
    // ordinal of a request is its position in the sequential collection.
    std::vector<size_t> request_at(collected.size() + 1, 0), seed_at(collected.size() + 1, 0);
    for (size_t c = 0; c < collected.size(); ++c) {
      request_at[c + 1] = request_at[c] + collected[c].requests.size();
      seed_at[c + 1] = seed_at[c] + collected[c].seeds.size();
    }
    // Default-initialised slots: the parallel copy writes every one (the
    // ordinals are the positions of the collection). A larger order than the
    // arena's reallocates without copying the stale requests.
    if (requests.capacity() < request_at.back()) { requests.clear(); requests.shrink_to_fit(); }
    requests.resize(request_at.back()); seeds.resize(seed_at.back());
    poison_unwritten(requests, 0); poison_unwritten(seeds, 0);
    // Hashed grouping (default) unless the batch resolver is bound (its view
    // takes sorted unique requests: the sorted witness serves it) or the
    // ordinals and slots of the order do not fit the 32-bit halves of a
    // table word (the witness groups that order, same object).
    const bool hashed = hashed_groups && !batch_resolver.resolve && requests.size() < kHashGroupMaxRequests &&
                        seeds.size() < kHashGroupMaxRequests;
    // Powers of two >= 2 x entries: load factor <= 1/2.
    const auto table = [](RawVector<u64>& words, size_t entries) -> size_t {
      if (!entries) return 0;
      size_t slots = 16;
      while (slots < 2 * entries) slots *= 2;
      if (words.capacity() < slots) { words.clear(); words.shrink_to_fit(); }
      words.resize(slots);
      poison_unwritten(words, 0);  // test builds: a slice left unzeroed breaks the gates
      return slots;
    };
    const size_t slots = hashed ? table(group_table, requests.size()) : 0;
    const size_t seed_slots = hashed ? table(seed_table, seeds.size()) : 0;
    parallel_items(collected.size(), geometry_threads, [&](size_t chunk, size_t) {
      auto& c = collected[chunk];
      for (size_t j = 0; j < c.requests.size(); ++j)
        requests[request_at[chunk] + j] = {c.requests[j].first, c.requests[j].second, request_at[chunk] + j};
      std::copy(c.seeds.begin(), c.seeds.end(), seeds.begin() + static_cast<std::ptrdiff_t>(seed_at[chunk]));
      decltype(c.requests)().swap(c.requests); decltype(c.seeds)().swap(c.seeds);
      // The same chunks zero the group and seed tables, each its own slice
      // (every slot once).
      for (auto [words, size] : {std::pair{&group_table, slots}, std::pair{&seed_table, seed_slots}})
        if (size)
          std::fill(words->begin() + static_cast<std::ptrdiff_t>(size * chunk / collected.size()),
                    words->begin() + static_cast<std::ptrdiff_t>(size * (chunk + 1) / collected.size()), u64{0});
    });
    lap(times->static_collect_by_k);
    st.static_requests[current_k] = requests.size();
    st.static_peak_request_bytes = std::max<u64>(st.static_peak_request_bytes,
        requests.capacity() * sizeof(Request));
    if (!hashed) {
      // (key, ordinal) is a strict total order: the parallel sort is the unique
      // sorted permutation, identical for every thread count.
      const size_t sorters = parallel_sort(requests, geometry_threads, [](const Request& a, const Request& b) {
        if (a.key != b.key) return a.key < b.key;
        return a.ordinal < b.ordinal;  // earliest consumer first in each class
      });
      // A parallel sort holds a second buffer of the same size at its peak.
      if (sorters > 1)
        st.static_peak_request_bytes = std::max<u64>(st.static_peak_request_bytes,
            2 * static_cast<u64>(requests.capacity()) * sizeof(Request));
    }
    if (hashed) index_seeds(seeds, seed_slots);  // no sort: duplicates refused on insertion
    else {
      // Seed keys are pairwise distinct (checked below): a strict total order.
      parallel_sort(seeds, geometry_threads, [](const StaticSeed& a, const StaticSeed& b) { return a.key < b.key; });
      for (size_t j = 1; j < seeds.size(); ++j)
        require(seeds[j - 1].key != seeds[j].key, "full_ball_static_duplicate_seed");
    }
    lap(times->static_sort_by_k);
    if (hashed) {
      resolve_hashed_order(requests, seeds, slots, seed_slots, lap);
      return;
    }
    // Group starts (first request of each key), found by chunks in parallel.
    std::vector<size_t> groups;
    {
      const size_t chunks = std::max<size_t>(1, std::min<size_t>(requests.size() / 65536 + 1, 256));
      std::vector<std::vector<size_t>> starts(chunks);
      parallel_items(chunks, geometry_threads, [&](size_t c, size_t) {
        for (size_t j = requests.size() * c / chunks; j < requests.size() * (c + 1) / chunks; ++j)
          if (!j || requests[j].key != requests[j - 1].key) starts[c].push_back(j);
      });
      size_t total = 0;
      for (const auto& part : starts) total += part.size();
      groups.reserve(total + 1);
      for (const auto& part : starts) groups.insert(groups.end(), part.begin(), part.end());
    }
    st.static_unique[current_k] = groups.size();
    groups.push_back(requests.size());
    if (static_trace) {  // gates only: the first (minimum) ordinal of each request's class
      auto& firsts = static_trace->firsts[current_k];
      firsts.assign(requests.size(), 0);
      for (size_t g = 0; g + 1 < groups.size(); ++g)
        for (size_t r = groups[g]; r < groups[g + 1]; ++r) firsts[requests[r].ordinal] = requests[groups[g]].ordinal;
      static_trace->path[current_k] = 1;
    }
    static_cursor = 0;
    // Every ordinal is written by exactly one group (the groups partition the
    // requests, the ordinals are a permutation): no sentinel fill.
    static_targets.resize(requests.size());
    poison_unwritten(static_targets, 0);
    st.static_peak_target_bytes = std::max<u64>(st.static_peak_target_bytes,
        static_targets.capacity() * sizeof(BallId));
    lap(times->static_groups_by_k);
    if (batch_resolver.resolve) {
      prepare_external_batch(requests, groups, seeds);
      lap(times->static_resolve_by_k);
      trace_static_targets();
      return;
    }
    struct Worker { FullBallStats work; std::vector<NodeRef> scratch; u64 seeded = 0; };
    std::vector<Worker> workers(planned_workers(groups.size() - 1, geometry_threads));
    const auto account = [&] {
      u64 worker_bytes = workers.capacity() * sizeof(Worker);
      for (const auto& w : workers) add(worker_bytes, w.scratch.capacity() * sizeof(NodeRef));
      const u64 seed_bytes = seeds.capacity() * sizeof(StaticSeed);
      const u64 group_bytes = groups.capacity() * sizeof(size_t);
      st.static_peak_seed_bytes = std::max(st.static_peak_seed_bytes, seed_bytes);
      st.static_peak_group_bytes = std::max(st.static_peak_group_bytes, group_bytes);
      st.static_peak_worker_bytes = std::max(st.static_peak_worker_bytes, worker_bytes);
      u64 retained = requests.capacity() * sizeof(Request);
      add(retained, static_targets.capacity() * sizeof(BallId));
      add(retained, seed_bytes); add(retained, group_bytes); add(retained, worker_bytes);
      // Sampled retained capacities only, not transient reallocations or RSS.
      st.static_peak_retained_bytes = std::max(st.static_peak_retained_bytes, retained);
      for (const auto& w : workers) {
        merge_static_work(w.work);
        add(st.static_seeded[current_k], w.seeded);
      }
    };
    const auto job = [&](size_t begin, size_t end, size_t worker) {
      auto& w = workers[worker];
      for (size_t group = begin; group < end; ++group) {
        const auto& first = requests[groups[group]];
        const auto& before = balls[first.consumer].level;
        const auto seed = std::lower_bound(seeds.begin(), seeds.end(), first.key,
            [](const StaticSeed& a, const ResolverCache::Key& key) { return a.key < key; });
        BallId target;
        if (seed != seeds.end() && seed->key == first.key) {
          target = seed->ball;
          require(level_run[target] < level_run[first.consumer], "full_ball_static_seed_not_strict");
          add(w.seeded);
        } else target = static_terminal(first.key, before, w.work, w.scratch, [&](const ResolverCache::Key& key) {
          const auto found = std::lower_bound(seeds.begin(), seeds.end(), key,
              [](const StaticSeed& a, const ResolverCache::Key& value) { return a.key < value; });
          return found != seeds.end() && found->key == key ? &*found : static_cast<const StaticSeed*>(nullptr);
        });
        for (size_t r = groups[group]; r < groups[group + 1]; ++r) {
          const auto& request = requests[r];
          require(level_run[first.consumer] <= level_run[request.consumer], "full_ball_static_request_chronology");
          static_targets[request.ordinal] = target;
        }
      }
    };
    try {
      const size_t lanes = parallel_ranges(groups.size() - 1, geometry_threads, job);
      add(st.static_lanes_used, lanes);
      if (lanes > 1) add(st.static_workers_created, lanes);  // one lane uses the caller
    }
    catch (...) {
      account();
      throw;
    }
    account();
    lap(times->static_resolve_by_k);
    trace_static_targets();
  }
  void trace_static_targets() {
    if (static_trace) static_trace->targets[current_k].assign(static_targets.begin(), static_targets.end());
  }
  u64 resolve_static_target(const ExactLevel& before, u64 prior_count) {
    require(static_cursor < static_targets.size(), "full_ball_static_target_missing");
    const BallId target = static_targets[static_cursor++];
    require(target < balls.size() && compare_exact_level(balls[target].level, before) < 0,
            "full_ball_static_target_not_strict");
    require(anchors[target] != absent, "full_ball_static_closed_anchor_missing");
    return root(anchors[target], prior_count);
  }
  u64 resolve_static(const ExactLevel& before, u64 prior_count) {
    add(st.representatives);
    return resolve_static_target(before, prior_count);
  }
  u64 resolve(std::vector<i32> sites, const ExactLevel& before, u64 prior_count) {
    add(st.representatives);
    std::sort(sites.begin(), sites.end());
    require(sites.size() == current_k && std::adjacent_find(sites.begin(), sites.end()) == sites.end(),
            "full_ball_representative_cardinality");
    if (current_k == 1) {
      const PointId id = ix.point_id(sites.front());
      return root(std::lower_bound(domain.begin(), domain.end(), id) - domain.begin(), prior_count);
    }
    if (geometry_threads) return resolve_static_target(before, prior_count);
    const auto initial_key = ResolverCache::key(sites);
    const u64 cached = resolver_cache.lookup(initial_key);
    if (cached != absent) return root(cached, prior_count);
    auto local = meb(sites, st.resolve_work);
    u64 length = 0;
    for (;;) {
      require(compare_exact_level(local.level, before) < 0, "full_ball_representative_not_strict");
      add(st.key_lookups);
      const u32 found = find_key(local.key);
      if (found != kAbsent32) {
        require(same_exact_level(balls[found].level, local.level), "full_ball_anchor_level");
        if (anchors[found] != absent) {
          add(st.anchor_hits); st.max_chain_steps = std::max(st.max_chain_steps, length);
          const u64 resolved = root(anchors[found], prior_count);
          resolver_cache.store(initial_key, resolved);
          return resolved;  // hit BEFORE spatial query
        }
        const auto& ball = balls[found];
        const unsigned lo = ball.n_interior + ball.arity - 1;
        require(current_k < lo || current_k > ball.n_interior + ball.n_shell,
                "full_ball_missing_closed_anchor");
      }
      const i32 z = intruder(local.key, sites);
      require(z >= 0, "full_ball_missing_weak_terminal");
      require(local.support_size > 0 && local.support_slots[0] < sites.size(), "full_ball_missing_support");
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      auto next = meb(sites, st.resolve_work);
      const int cmp = compare_exact_level(next.level, local.level);
      require(cmp <= 0, "full_ball_radius_increased");
      if (cmp == 0) {
        require(next.key == local.key && next.selected_shell_count + 1 == local.selected_shell_count,
                "full_ball_equal_radius_shell_not_decreased");
        add(st.same_radius_steps);
      } else add(st.descending_steps);
      add(length); local = next;
    }
  }

  void seed_closed_anchor(BallId ball, u64 target) {
    if (geometry_threads) return;  // Static seeds are BallIds, not temporal tokens.
    const auto& b = balls[ball];
    if (current_k < 2 || current_k != static_cast<unsigned>(b.n_interior) + b.n_shell) return;
    // I union U is the unique K-facet of this closed population. Its positive
    // support certifies its MEB. The whole lot has closed before publication,
    // so any later lookup is strictly after this birth and normalizes its token.
    ResolverCache::Key key{};
    size_t n = 0;
    for (i32 site : b.interior()) key[n++] = site;
    for (i32 site : b.shell()) key[n++] = site;
    std::sort(key.begin(), key.begin() + n);
    resolver_cache.store(key, target, true);
  }

  template<class Emit>
  void visit_block(BallId id, u16& contribution, bool& interior, Emit&& emit) const {
    visit_block_at(current_k, id, contribution, interior, std::forward<Emit>(emit));
  }
  // v9 E3: the facet COUNT, contribution and interior of visit_block_at
  // without building the facets (no site copies, no identity lookups): the
  // same requires in the same order, the number of facets it would emit.
  u32 count_block_at(unsigned k, BallId id, u16& contribution, bool& interior) const {
    const auto& b = balls[id];
    if (b.n_shell == b.arity) {
      if (k == b.n_interior + b.n_shell) {
        contribution = static_cast<u16>((1u << b.n_shell) - 1);
        interior = b.n_interior != 0;
        return 0;
      }
      require(k + 1 == b.n_interior + b.n_shell, "full_ball_regular_rank");
#if defined(MHGP9_TOWER_MUTANT_COUNT_SKIPS_FACET)
      return b.n_shell - 1;  // mutant: one facet of the block forgotten
#else
      return b.n_shell;
#endif
    }
    const auto& table = extra.at(id);
    const auto rank = table.rank(k);
    require(rank.present, "full_ball_absent_scheduled_block");
    contribution = rank.contribution_shell; interior = rank.contribution_interior;
    return static_cast<u32>(rank.strict_components.size());
  }
  // Same as visit_block for an explicit order k (parallel orders).
  template<class Emit>
  void visit_block_at(unsigned k, BallId id, u16& contribution, bool& interior, Emit&& emit) const {
    const auto& b = balls[id];
    if (b.n_shell == b.arity) {
      if (k == b.n_interior + b.n_shell) {
        contribution = static_cast<u16>((1u << b.n_shell) - 1);
        interior = b.n_interior != 0;
      } else {
        require(k + 1 == b.n_interior + b.n_shell, "full_ball_regular_rank");
        // Representative facets are built in a fixed buffer (k <=
        // kFacetMaxK sites): no allocation per representative.
        std::array<i32, kFacetMaxK> sites{};
        for (size_t omit = 0; omit < b.n_shell; ++omit) {
          size_t n = 0;
          for (i32 site : b.interior()) { require(n < sites.size(), "full_ball_representative_cardinality"); sites[n++] = site; }
          for (size_t j = 0; j < b.n_shell; ++j) if (j != omit) {
            require(n < sites.size(), "full_ball_representative_cardinality");
            sites[n++] = b.shell_ids[j];
          }
          emit(std::span<const i32>(sites.data(), n));
        }
      }
    } else {
      const auto& table = extra.at(id);
      const auto rank = table.rank(k);
      require(rank.present, "full_ball_absent_scheduled_block");
      contribution = rank.contribution_shell; interior = rank.contribution_interior;
      std::array<i32, kFacetMaxK> sites{};
      for (const auto& component : rank.strict_components) {
        size_t n = 0;
        for (size_t j = 0; j < component.interior_prefix; ++j) {
          require(n < sites.size(), "full_ball_representative_cardinality");
          sites[n++] = geometry_id(table.census().interior[j].id);
        }
        for (size_t j = 0; j < table.census().shell.size(); ++j)
          if (component.representative_shell & (u16{1} << j)) {
            require(n < sites.size(), "full_ball_representative_cardinality");
            sites[n++] = geometry_id(table.census().shell[j].id);
          }
        emit(std::span<const i32>(sites.data(), n));
      }
    }
  }
  void prepare_block(Block& block, BallId id, const ExactLevel& level, u64 prior_count) {
    block.ball = id; block.roots.clear(); block.contribution = 0; block.interior = false;
    add(st.anchor_blocks);
    if (balls[id].n_shell == balls[id].arity) add(st.regular_blocks); else add(st.extra_blocks);
    visit_block(id, block.contribution, block.interior, [&](std::span<const i32> facet) {
      // Static path (K>1): the facet was validated when its request was
      // collected; only its precomputed target is consumed here.
      if (geometry_threads && current_k > 1) block.roots.push_back(resolve_static(level, prior_count));
      else block.roots.push_back(resolve(std::vector<i32>(facet.begin(), facet.end()), level, prior_count));
    });
    std::sort(block.roots.begin(), block.roots.end());
    block.roots.erase(std::unique(block.roots.begin(), block.roots.end()), block.roots.end());
  }
  u64 population(BallId id) {
    if (population_ids[id] != absent) return population_ids[id];
    FullCoveragePopulation row;
    for (i32 site : balls[id].interior()) row.interior.push_back(ix.point_id(site));
    for (i32 site : balls[id].shell()) row.shell.push_back(ix.point_id(site));
    std::sort(row.interior.begin(), row.interior.end()); std::sort(row.shell.begin(), row.shell.end());
    population_ids[id] = populations.size(); populations.push_back(std::move(row));
    return population_ids[id];
  }
  u64 new_node(const ExactLevel& level, const std::vector<u64>& parents, Draft& draft,
      MonotoneHistory& lower, u64 birth_image) {
    u64 image = birth_image;
    if (current_k > 1 && !parents.empty()) {
      image = lower.root_at(draft.lower_nodes[parents.front()], level, true);
      for (u64 p : parents)
        require(lower.root_at(draft.lower_nodes[p], level, true) == image, "full_ball_vertical_naturality");
    }
    const u64 id = current.levels.size();
    require(id != absent, "full_ball_node_representation", FullBallStatus::kResourceExhausted);
    current.levels.push_back(level); current.next.push_back(absent); compressed.push_back(id);
    for (u64 parent : parents) { current.next[parent] = id; compressed[parent] = id; }
    draft.lower_nodes.push_back(image);
    if (parents.empty()) add(st.births); else add(st.merges);
    return id;
  }
  void close_lot(std::span<Block> blocks, const ExactLevel& level, Draft& draft,
      MonotoneHistory& lower, const std::vector<u64>& lower_anchors) {
    if (blocks.size() == 1) {
      add(st.singleton_lots);
      auto& block = blocks.front();
      FullCoverageAction action;
      action.parents.swap(block.roots);  // Already sorted and unique in prepare_block.
      if (block.contribution || block.interior) {
        action.contributions.push_back({population(block.ball), block.contribution, block.interior});
        add(st.contributions);
      }
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        u64 birth_image = absent;
        if (action.parents.empty()) {
          require(action.contributions.size() == 1, "full_ball_distinct_births");
          if (current_k > 1) {
            require(block.ball < lower_anchors.size() && lower_anchors[block.ball] != absent,
                "full_ball_vertical_birth_anchor");
            birth_image = lower.root_at(lower_anchors[block.ball], level, true);
          }
        }
        target = new_node(level, action.parents, draft, lower, birth_image);
      }
      if (action.parents.size() != 1 || !action.contributions.empty()) {
        FullCoverageBatch batch{level, {}};
        batch.actions.push_back(std::move(action));
        draft.batches.push_back(std::move(batch));
      } else add(st.inert_blocks);
      require(anchors[block.ball] == absent && target != absent, "full_ball_anchor_duplicate");
      anchors[block.ball] = target;  // All representatives and the whole lot are closed.
      seed_closed_anchor(block.ball, target);
      return;
    }
    add(st.grouped_lots);
    // Prospectively account the actual DSU initialization; unitary lots never
    // reach this path and create no DSU, owner groups or target array.
    add(st.lot_dsu_slots, blocks.size());
    std::vector<size_t> dsu(blocks.size()); std::iota(dsu.begin(), dsu.end(), size_t{0});
    const auto find = [&](size_t a) { while (dsu[a] != a) { dsu[a] = dsu[dsu[a]]; a = dsu[a]; } return a; };
    std::vector<std::pair<u64,size_t>> owners;
    for (size_t b = 0; b < blocks.size(); ++b) for (u64 p : blocks[b].roots) owners.emplace_back(p,b);
    std::sort(owners.begin(), owners.end());
    for (size_t j = 1; j < owners.size(); ++j) if (owners[j - 1].first == owners[j].first) {
      const auto a = find(owners[j - 1].second), b = find(owners[j].second);
      dsu[std::max(a,b)] = std::min(a,b);
    }
    std::vector<std::vector<size_t>> groups(blocks.size());
    for (size_t b = 0; b < blocks.size(); ++b) groups[find(b)].push_back(b);
    FullCoverageBatch batch{level, {}};
    std::vector<u64> targets(blocks.size(), absent);
    for (const auto& group : groups) {
      if (group.empty()) continue;
      FullCoverageAction action;
      for (size_t b : group) {
        const auto& block = blocks[b];
        action.parents.insert(action.parents.end(), block.roots.begin(), block.roots.end());
        if (block.contribution || block.interior) {
          action.contributions.push_back({population(block.ball), block.contribution, block.interior});
          add(st.contributions);
        }
      }
      std::sort(action.parents.begin(), action.parents.end());
      action.parents.erase(std::unique(action.parents.begin(), action.parents.end()), action.parents.end());
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        u64 birth_image = absent;
        if (action.parents.empty()) {
          require(group.size() == 1 && action.contributions.size() == 1, "full_ball_distinct_births");
          if (current_k > 1) {
            const auto ball = blocks[group.front()].ball;
            require(ball < lower_anchors.size() && lower_anchors[ball] != absent, "full_ball_vertical_birth_anchor");
            birth_image = lower.root_at(lower_anchors[ball], level, true);
          }
        }
        target = new_node(level, action.parents, draft, lower, birth_image);
      }
      for (size_t b : group) targets[b] = target;
      if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));
      else add(st.inert_blocks, group.size());
    }
    if (!batch.actions.empty()) draft.batches.push_back(std::move(batch));
    // Publish anchors only AFTER the complete atomic group has been staged.
    // Failure anywhere invalidates the whole result, not just this lot.
    for (size_t b = 0; b < blocks.size(); ++b) {
      require(anchors[blocks[b].ball] == absent && targets[b] != absent, "full_ball_anchor_duplicate");
      anchors[blocks[b].ball] = targets[b];
      seed_closed_anchor(blocks[b].ball, targets[b]);
    }
  }
};
}  // namespace full_ball_detail

// meb_proposal selects anchor_meb_proposed (default) or the reference
// enumeration for every local MEB: same objects, different work.
// options (FullBallTowerOptions): the static path's named switches.
inline FullBallTowerResult build_full_ball_tower(const CloudIndex& ix, std::span<const BallData> balls,
    unsigned kmax, int static_threads = 0, FullBallBatchResolver batch = {}, bool meb_proposal = true,
    const FullBallTowerOptions& options = {}) {
  FullBallTowerResult result;
  try {
    result.orders = full_ball_detail::Builder(ix, balls, kmax, result.stats, static_threads, batch, meb_proposal,
                                              &result.times, options).run();
    result.status = FullBallStatus::kCompleteRelative; result.reason = kFullBallAuthority;
  } catch (const full_ball_detail::Failure& error) {
    result.status = error.status; result.reason = error.reason;
  } catch (const std::bad_alloc&) {
    result.status = FullBallStatus::kResourceExhausted; result.reason = "full_ball_allocation_failed";
  } catch (const std::length_error&) {
    result.status = FullBallStatus::kResourceExhausted; result.reason = "full_ball_size_overflow";
  } catch (const std::invalid_argument&) {
    result.status = FullBallStatus::kInvalidInput; result.reason = "full_ball_local_census_invalid";
  } catch (const std::system_error&) {
    result.status = FullBallStatus::kResourceExhausted; result.reason = "full_ball_thread_launch_failed";
  }
  if (result.status != FullBallStatus::kCompleteRelative) result.orders.clear();
  return result;
}

inline FullNodeId full_ball_vertical_root_at(const FullBallTowerResult& tower, unsigned k,
    FullNodeId root, const ExactLevel& cut, bool closed) {
  if (tower.status != FullBallStatus::kCompleteRelative || k < 2 || k > tower.orders.size())
    return kFullCoverageAbsent;
  const auto& upper = tower.orders[k - 1];
  if (root >= upper.lower_nodes.size() || full_coverage_root_at(upper.forest, root, cut, closed) != root)
    return kFullCoverageAbsent;
  return full_coverage_root_at(tower.orders[k - 2].forest, upper.lower_nodes[root], cut, closed);
}

}  // namespace mhgp9::tower
