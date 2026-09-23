// FULL horizontal forests and vertical birth maps from supplied COMPLETE EXACT
// ball censuses. No claim of WSPD completeness, archive authority, or speed.
#pragma once

#include <cfenv>
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
  u64 presorted_catalogues = 0;  // key order certified by one scan, no sort
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
struct FullBallOrder {
  FullCoverageCertificate forest;
  // Image at the node's closed creation level. Queries normalize in the lower
  // history at their OWN cut. All entries are absent at K1.
  std::vector<FullNodeId> lower_nodes;
};
struct FullBallTowerResult {
  FullBallStatus status = FullBallStatus::kInvalidInput;
  const char* reason = "full_ball_uninitialized";
  FullBallStats stats;
  std::vector<FullBallOrder> orders;  // index K-1, no partial orders on failure
};

namespace full_ball_detail {
using BallId = u32;
constexpr u64 absent = kFullCoverageAbsent;
struct Failure { FullBallStatus status; const char* reason; };
inline void require(bool ok, const char* reason, FullBallStatus status = FullBallStatus::kInvariantViolated) {
  if (!ok) throw Failure{status, reason};
}
#if defined(MHGP9_TESTING)
// Test failpoints, never compiled in a product target: bit K-1 makes order K
// fail at the end of its lots (A) or of its vertical images (C). The
// sequential path checks both at the end of order K, lots first.
inline std::atomic<u32> failpoint_lots{0}, failpoint_images{0};
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
#else
inline void failpoint_after_lots(unsigned) {}
inline void failpoint_after_images(unsigned) {}
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
// Working view for chronologically increasing LOWER closed cuts. Component
// labels are separate from DSU representatives, so union by rank is permitted.
// The immutable source history still answers arbitrary historical export cuts.
class MonotoneHistory {
 public:
  MonotoneHistory(const History& history, FullBallStats& stats) : source(history), st(stats),
      heads(history.levels.size(), absent), links(history.levels.size(), absent),
      parent(history.levels.size()), owner(history.levels.size()), rank(history.levels.size(), 0) {
    const size_t n = source.levels.size();
    require(source.next.size() == n, "full_ball_lower_history_shape");
    std::iota(parent.begin(), parent.end(), u64{0});
    std::iota(owner.begin(), owner.end(), u64{0});
    for (size_t node = 0; node < n; ++node) {
      require(source.levels[node].den > 0 && (!node ||
          compare_exact_level(source.levels[node - 1], source.levels[node]) <= 0),
          "full_ball_lower_history_chronology");
      const u64 next = source.next[node];
      if (next == absent) continue;
      require(next > node && next < n && compare_exact_level(source.levels[node], source.levels[next]) < 0,
          "full_ball_lower_history_edge");
      add(st.lower_edges_indexed);
      links[node] = heads[next]; heads[next] = node;
    }
  }

  u64 root_at(u64 token, const ExactLevel& cut, bool closed) {
    add(st.lower_queries);
    require(cut.den > 0, "full_ball_lower_cut_domain");
    if (has_cut) {
      const int cmp = compare_exact_level(cut, previous_cut);
      require(cmp > 0 || (cmp == 0 && (closed || !previous_closed)), "full_ball_lower_cut_not_monotone");
    }
    has_cut = true; previous_cut = cut; previous_closed = closed;
    while (cursor < source.levels.size() &&
        full_coverage_detail::admitted(source.levels[cursor], cut, closed)) {
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
  const History& source;
  FullBallStats& st;
  std::vector<u64> heads, links, parent, owner;
  std::vector<u8> rank;
  size_t cursor = 0;
  ExactLevel previous_cut{};
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
struct Draft {
  std::vector<FullCoverageBatch> batches;
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
      int static_threads = 0, FullBallBatchResolver batch = {}, bool meb_proposal = true)
      : ix(index), balls(census), requested(max_k), st(stats), resolver_cache(stats),
        geometry_threads(static_threads), batch_resolver(batch), propose_meb(meb_proposal) {}

  std::vector<FullBallOrder> run() {
    validate_catalogue();
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
      current_k = k;
      if (k > 1) resolver_cache.reset();
      anchors.assign(balls.size(), absent);
      current = {}; compressed.clear();
      if (geometry_threads && k > 1) prepare_static_order();
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
        while (end < program.size() && same_exact_level(level, balls[program[end]].level)) ++end;
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
        std::vector<BallId>().swap(static_targets);
      }
      u64 live = 0;
      for (u64 next : current.next) if (next == absent) ++live;
      require(live == 1, "full_ball_final_component_count");
      failpoint_after_lots(k);
      failpoint_after_images(k);
      drafts.push_back(std::move(draft));
      lower_history = std::move(current);
      lower_anchors = std::move(anchors);
    }
    // Construction indices and histories are dead after the last order; no
    // published object borrows them. Release before copying the immutable bank.
    lower_history = {};
    decltype(lower_anchors)().swap(lower_anchors);
    return finish(drafts);
  }

  std::vector<FullBallOrder> finish(std::vector<Draft>& drafts) {
    resolver_cache.release();
    decltype(identity)().swap(identity);
    decltype(by_key)().swap(by_key);
    decltype(programs)().swap(programs);
    decltype(extra)().swap(extra);
    decltype(population_ids)().swap(population_ids);
    decltype(anchors)().swap(anchors);
    decltype(compressed)().swap(compressed);
    current = {};
    decltype(stack)().swap(stack);
    // Rows are moved into the bank (validated on the static threads).
    auto bank = build_full_coverage_populations(domain, std::move(populations), geometry_threads);
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
    parallel_items(kmax, geometry_threads, [&](size_t i, size_t) {
      forests[i] = build_full_coverage_certificate(static_cast<unsigned>(i + 1), bank.value, drafts[i].batches);
      std::vector<FullCoverageBatch>().swap(drafts[i].batches);  // encoded: release at once
    });
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
    std::vector<BallId> static_targets;
    size_t static_cursor = 0;
    std::vector<Block> lot_blocks;
    Draft draft;
    std::vector<u32> birth_ball;       // per node: ball of a birth, or kAbsent32
  };
  // Node IDs of one order in u32: an order with 2^32-1 nodes or more is an
  // explicit resource refusal on this path (never a truncation). At 30 M
  // sites and K10 an order holds about 1.2 G nodes.
  static constexpr u32 kAbsent32 = std::numeric_limits<u32>::max();

  std::vector<FullBallOrder> run_orders_parallel() {
    std::vector<OrderState> orders(kmax);
    for (unsigned k = 1; k <= kmax; ++k) {
      auto& o = orders[k - 1];
      o.k = k;
      if (k > 1) {
        current_k = k;
        prepare_static_order();  // parallel inside, writes static_targets
        o.static_targets.swap(static_targets);
      }
    }
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
      parallel_items(kmax, geometry_threads, [&](size_t i, size_t) {
        try { order_lots(orders[i]); } catch (const Failure& f) { failures[i] = f; }
      });
      size_t lots_done = 0;
      while (lots_done < kmax && !failures[lots_done]) ++lots_done;
#if defined(MHGP9_FULL_ORDERS_MUTANT_PHASE_PRIORITY)
      if (lots_done != kmax) lots_done = 0;  // mutant: a lot failure masks lower images
#endif
      if (lots_done == kmax) assign_populations(orders);
      parallel_items(lots_done, geometry_threads, [&](size_t i, size_t) {
        try { order_images(orders[i], i ? &orders[i - 1] : nullptr); } catch (const Failure& f) { failures[i] = f; }
      });
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
    const u64 id = o.current.levels.size();
    require(id < kAbsent32, "full_ball_node_representation", FullBallStatus::kResourceExhausted);
    o.current.levels.push_back(level); o.current.next.push_back(absent); o.compressed.push_back(id);
    for (u64 parent : parents) { o.current.next[parent] = id; o.compressed[parent] = id; }
    o.birth_ball.push_back(birth);
    if (parents.empty()) add(o.st.births); else add(o.st.merges);
    return id;
  }

  void order_block(OrderState& o, Block& block, BallId id, const ExactLevel& level, u64 prior_count) {
    block.ball = id; block.roots.clear(); block.contribution = 0; block.interior = false;
    add(o.st.anchor_blocks);
    if (balls[id].n_shell == balls[id].arity) add(o.st.regular_blocks); else add(o.st.extra_blocks);
    u16 contribution = 0; bool interior = false;
    visit_block_at(o.k, id, contribution, interior, [&](std::span<const i32> facet) {
      if (o.k == 1) {
        add(o.st.representatives);
        require(facet.size() == 1, "full_ball_representative_cardinality");
        const PointId point = ix.point_id(facet.front());
        block.roots.push_back(order_root(o, std::lower_bound(domain.begin(), domain.end(), point) - domain.begin(),
                                         prior_count));
        return;
      }
      add(o.st.representatives);
      require(o.static_cursor < o.static_targets.size(), "full_ball_static_target_missing");
      const BallId target = o.static_targets[o.static_cursor++];
      require(target < balls.size() && compare_exact_level(balls[target].level, level) < 0,
              "full_ball_static_target_not_strict");
      require(o.anchors[target] != kAbsent32, "full_ball_static_closed_anchor_missing");
      block.roots.push_back(order_root(o, o.anchors[target], prior_count));
    });
    block.contribution = contribution; block.interior = interior;
    std::sort(block.roots.begin(), block.roots.end());
    block.roots.erase(std::unique(block.roots.begin(), block.roots.end()), block.roots.end());
  }

  void order_lot(OrderState& o, std::span<Block> blocks, const ExactLevel& level) {
    const auto birth_of = [&](const std::vector<u64>& parents, BallId ball) {
      return parents.empty() ? static_cast<u32>(ball) : kAbsent32;
    };
    if (blocks.size() == 1) {
      add(o.st.singleton_lots);
      auto& block = blocks.front();
      FullCoverageAction action;
      action.parents.swap(block.roots);
      if (block.contribution || block.interior) {
        action.contributions.push_back({kBallTag | block.ball, block.contribution, block.interior});
        add(o.st.contributions);
      }
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        if (action.parents.empty()) require(action.contributions.size() == 1, "full_ball_distinct_births");
        target = order_new_node(o, level, action.parents, birth_of(action.parents, block.ball));
      }
      if (action.parents.size() != 1 || !action.contributions.empty()) {
        FullCoverageBatch batch{level, {}};
        batch.actions.push_back(std::move(action));
        o.draft.batches.push_back(std::move(batch));
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
    FullCoverageBatch batch{level, {}};
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
      if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));
      else add(o.st.inert_blocks, group.size());
    }
    if (!batch.actions.empty()) o.draft.batches.push_back(std::move(batch));
    for (size_t b = 0; b < blocks.size(); ++b) {
      require(o.anchors[blocks[b].ball] == kAbsent32 && targets[b] != absent, "full_ball_anchor_duplicate");
      o.anchors[blocks[b].ball] = static_cast<u32>(targets[b]);
    }
  }

  void order_lots(OrderState& o) {
    o.anchors.assign(balls.size(), kAbsent32);
    if (o.k == 1) {
      FullCoverageBatch initial;
      for (size_t j = 0; j < domain.size(); ++j) {
        initial.actions.push_back({{}, {{j, 1, false}}});  // domain singleton j (population j)
        add(o.st.contributions);
        order_new_node(o, initial.level, {}, kAbsent32);
      }
      o.draft.batches.push_back(std::move(initial));
    }
    const auto& program = programs[o.k];
    for (size_t begin = 0; begin < program.size();) {
      size_t end = begin + 1;
      const auto& level = balls[program[begin]].level;
      while (end < program.size() && same_exact_level(level, balls[program[end]].level)) ++end;
      if (o.lot_blocks.size() < end - begin) o.lot_blocks.resize(end - begin);
      const std::span<Block> blocks(o.lot_blocks.data(), end - begin);
      const u64 prior_count = o.current.levels.size();
      for (size_t j = begin; j < end; ++j) order_block(o, blocks[j - begin], program[j], level, prior_count);
      order_lot(o, blocks, level);
      begin = end;
    }
    if (o.k > 1) require(o.static_cursor == o.static_targets.size(), "full_ball_static_unconsumed_targets");
    std::vector<BallId>().swap(o.static_targets);
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
    for (auto& o : orders)
      for (auto& batch : o.draft.batches)
        for (auto& action : batch.actions)
          for (auto& ref : action.contributions) {
            if ((ref.population & kBallTag) == 0) continue;
            const auto ball = static_cast<BallId>(ref.population & ~kBallTag);
            if (population_ids[ball] == absent) { population_ids[ball] = next++; first_seen.push_back(ball); }
            ref.population = population_ids[ball];
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

  void order_images(OrderState& o, const OrderState* lower) {
    static const History empty_history;
    MonotoneHistory cursor(lower ? lower->current : empty_history, o.st);
    o.draft.lower_nodes.reserve(o.current.levels.size());
    u64 node = 0;
    for (const auto& batch : o.draft.batches)
      for (const auto& action : batch.actions) {
        if (action.parents.size() == 1) continue;  // continuation: no node
        require(node < o.current.levels.size(), "full_ball_image_node_count");
        u64 image = absent;
        if (o.k > 1) {
          const auto& level = o.current.levels[node];
          if (action.parents.empty()) {
            const u32 ball = o.birth_ball[node];
            require(ball != kAbsent32 && lower->anchors[ball] != kAbsent32, "full_ball_vertical_birth_anchor");
            image = cursor.root_at(lower->anchors[ball], level, true);
          } else {
            image = cursor.root_at(o.draft.lower_nodes[action.parents.front()], level, true);
            for (u64 p : action.parents)
              require(cursor.root_at(o.draft.lower_nodes[p], level, true) == image, "full_ball_vertical_naturality");
          }
        }
        o.draft.lower_nodes.push_back(image);
        ++node;
      }
    require(node == o.current.levels.size(), "full_ball_image_node_count");
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
  using StaticSeed = FullBallBatchSeed;
  std::vector<BallId> static_targets;
  size_t static_cursor = 0;
  std::vector<PointId> domain;
  std::vector<std::pair<PointId,i32>> identity;
  std::vector<BallId> by_key;
  std::vector<std::vector<BallId>> programs;
  std::unordered_map<BallId, local_plateau::ShellTable> extra;
  std::vector<FullCoveragePopulation> populations;
  std::vector<u64> population_ids, anchors, compressed;
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
    by_key.resize(balls.size()); std::iota(by_key.begin(), by_key.end(), BallId{0});
    // A catalogue already in STRICTLY increasing key order (the chain's) is
    // certified by this one scan: the identity is then the unique sorted
    // permutation and keys are distinct. Otherwise keys are sorted and must
    // be pairwise distinct: a strict total order, so the parallel sort is
    // the unique sorted permutation.
    bool presorted = true;
    for (size_t j = 1; j < balls.size() && presorted; ++j) presorted = balls[j - 1].key < balls[j].key;
    if (presorted) add(st.presorted_catalogues);
    else {
      parallel_sort(by_key, geometry_threads, [&](BallId a, BallId b) { return balls[a].key < balls[b].key; });
      for (size_t j = 1; j < by_key.size(); ++j)
        require(!(balls[by_key[j]].key == balls[by_key[j - 1]].key), "full_ball_duplicate_key", invalid);
    }
    std::vector<u8> qmins(balls.size());
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
    }
    for (size_t j = 0; j < balls.size(); ++j) {
      if (j == first_local_failure) throw local_failure;
      const auto& ball = balls[j];
      std::array<i32,4> support{};
      unsigned q = ball.arity;
      if (ball.n_shell != ball.arity) {
        local_plateau::LocalCensus local{ball.key, {}, {}};
        for (i32 site : ball.interior()) local.interior.push_back({ix.point_id(site), ix.upos[site]});
        for (i32 site : ball.shell()) local.shell.push_back({ix.point_id(site), ix.upos[site]});
        auto table = local_plateau::ShellTable::prepare(std::move(local));
        q = table.q_min();
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
      }
      require(ball.n_interior + q <= std::min<u64>(kmax + 1, ix.input_count),
              "full_ball_outside_rank_window", invalid);
      qmins[j] = static_cast<u8>(q);
      const unsigned lo = ball.n_interior + q - 1;
      const unsigned hi = std::min<unsigned>(kmax, ball.n_interior + ball.n_shell);
      for (unsigned k = lo; k <= hi; ++k) ++counts[k];
      add(st.records);
    }
    // Same order as a stable sort of by_key under compare_exact_level: the
    // certified double filter decides clear gaps, the exact U320 comparison
    // decides the rest, and equal levels keep their by_key rank.
    auto by_level = by_key;
    if (std::fegetround() == FE_TONEAREST) {
      std::vector<double> approx(balls.size());
      std::vector<BallId> rank(balls.size());
      for (size_t j = 0; j < by_key.size(); ++j) {
        rank[by_key[j]] = static_cast<BallId>(j);
        approx[by_key[j]] = level_approximation(balls[by_key[j]].level);
      }
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
    programs.resize(kmax + 1);
    for (unsigned k = 1; k <= kmax; ++k) programs[k].reserve(counts[k]);
    for (BallId b : by_level) {
      const unsigned lo = balls[b].n_interior + qmins[b] - 1;
      const unsigned hi = std::min<unsigned>(kmax, balls[b].n_interior + balls[b].n_shell);
      for (unsigned k = lo; k <= hi; ++k) programs[k].push_back(b);
    }
    population_ids.assign(balls.size(), absent);
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
  BallId static_terminal(ResolverCache::Key key, const ExactLevel& before,
      FullBallStats& work, std::vector<NodeRef>& scratch, std::span<const StaticSeed> seeds) const {
    std::span<i32> sites(key.data(), current_k);
    auto local = meb(sites, work.resolve_work);
    u64 length = 0;
    for (;;) {
      require(compare_exact_level(local.level, before) < 0, "full_ball_static_not_strict");
      add(work.key_lookups);
      const auto found = std::lower_bound(by_key.begin(), by_key.end(), local.key,
          [&](BallId id, const BallKey& value) { return balls[id].key < value; });
      if (found != by_key.end() && balls[*found].key == local.key) {
        const auto& b = balls[*found];
        require(same_exact_level(b.level, local.level), "full_ball_static_anchor_level");
        const unsigned lo = b.n_interior + b.arity - 1;
        if (current_k >= lo && current_k <= b.n_interior + b.n_shell) {
          add(work.anchor_hits); work.max_chain_steps = std::max(work.max_chain_steps, length);
          return *found;
        }
      }
      const i32 z = intruder_work(local.key, sites, work, scratch);
      require(z >= 0, "full_ball_static_missing_weak_terminal");
      require(local.support_size > 0 && local.support_slots[0] < sites.size(),
              "full_ball_static_missing_support");
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      add(work.static_post_seed_queries[current_k]);
      const auto seed = std::lower_bound(seeds.begin(), seeds.end(), key,
          [](const StaticSeed& a, const ResolverCache::Key& value) { return a.key < value; });
      if (seed != seeds.end() && seed->key == key) {
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

  void prepare_external_batch(const std::vector<FullBallBatchRequest>& requests,
      const std::vector<size_t>& groups, const std::vector<StaticSeed>& seeds) {
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

  void prepare_static_order() {
    using Request = FullBallBatchRequest;
    std::vector<Request> requests;
    std::vector<StaticSeed> seeds;
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
    size_t total_requests = 0, total_seeds = 0;
    for (const auto& c : collected) { total_requests += c.requests.size(); total_seeds += c.seeds.size(); }
    requests.reserve(total_requests); seeds.reserve(total_seeds);
    for (auto& c : collected) {
      for (const auto& [key, id] : c.requests) requests.push_back({key, id, requests.size()});
      seeds.insert(seeds.end(), c.seeds.begin(), c.seeds.end());
      decltype(c.requests)().swap(c.requests); decltype(c.seeds)().swap(c.seeds);
    }
    st.static_requests[current_k] = requests.size();
    st.static_peak_request_bytes = std::max<u64>(st.static_peak_request_bytes,
        requests.capacity() * sizeof(Request));
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
    // Seed keys are pairwise distinct (checked below): a strict total order.
    parallel_sort(seeds, geometry_threads, [](const StaticSeed& a, const StaticSeed& b) { return a.key < b.key; });
    for (size_t j = 1; j < seeds.size(); ++j)
      require(seeds[j - 1].key != seeds[j].key, "full_ball_static_duplicate_seed");
    std::vector<size_t> groups;
    for (size_t j = 0; j < requests.size(); ++j)
      if (!j || requests[j].key != requests[j - 1].key) groups.push_back(j);
    st.static_unique[current_k] = groups.size();
    groups.push_back(requests.size());
    static_cursor = 0;
    static_targets.assign(requests.size(), std::numeric_limits<BallId>::max());
    st.static_peak_target_bytes = std::max<u64>(st.static_peak_target_bytes,
        static_targets.capacity() * sizeof(BallId));
    if (batch_resolver.resolve) {
      prepare_external_batch(requests, groups, seeds);
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
          require(compare_exact_level(balls[target].level, before) < 0, "full_ball_static_seed_not_strict");
          add(w.seeded);
        } else target = static_terminal(first.key, before, w.work, w.scratch, seeds);
        for (size_t r = groups[group]; r < groups[group + 1]; ++r) {
          const auto& request = requests[r];
          require(compare_exact_level(before, balls[request.consumer].level) <= 0,
                  "full_ball_static_request_chronology");
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
      const auto found = std::lower_bound(by_key.begin(), by_key.end(), local.key,
          [&](BallId id, const BallKey& key) { return balls[id].key < key; });
      if (found != by_key.end() && balls[*found].key == local.key) {
        require(same_exact_level(balls[*found].level, local.level), "full_ball_anchor_level");
        if (anchors[*found] != absent) {
          add(st.anchor_hits); st.max_chain_steps = std::max(st.max_chain_steps, length);
          const u64 resolved = root(anchors[*found], prior_count);
          resolver_cache.store(initial_key, resolved);
          return resolved;  // hit BEFORE spatial query
        }
        const auto& ball = balls[*found];
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
inline FullBallTowerResult build_full_ball_tower(const CloudIndex& ix, std::span<const BallData> balls,
    unsigned kmax, int static_threads = 0, FullBallBatchResolver batch = {}, bool meb_proposal = true) {
  FullBallTowerResult result;
  try {
    result.orders =
        full_ball_detail::Builder(ix, balls, kmax, result.stats, static_threads, batch, meb_proposal).run();
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
