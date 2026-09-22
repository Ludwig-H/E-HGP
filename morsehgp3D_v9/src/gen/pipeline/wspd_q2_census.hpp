#pragma once

#include "q2_census.hpp"
#include "wspd/front.hpp"

namespace mhgp9::gen {

enum class Q2SiblingMode { Disabled, Saturating };
enum class Q2WitnessOrder { GlobalDfs, ComplementFirst };
enum class Q2AnchorMode { Individual, SharedProduct, SharedAnchors };

struct Q2PoolWork {
  u64 selected_rectangles{}, selected_pairs{}, residual_pairs{}, filtered_pairs{};
  u64 factor_sites{}, selection_tests{}, witness_attempts{}, universal_queries{};
  u64 q2_axis_terms{}, pool_selected{}, pool_insertions{}, pool_shifted_entries{};
  u64 prefix_class_visits{}, factor_read_visits{}, grouping_visits{}, bands{};
  u64 selected_anchors{}, pair_roots{}, plan_peak_bytes{}, original_selected_anchors{};
  u64 passthrough_rectangles{}, passthrough_pairs{}, passthrough_anchors{};
  double preparation_ms{}, selected_total_ms{};
  bool operator==(const Q2PoolWork&) const = default;
};

struct Q2JointWork {
  u64 root_products{}, tasks{}, splits_a{}, splits_b{};
  u64 witness_splits{}, bound_tests{}, cursor_advances{}, structural_splits{};
  u64 deferred_skips{}, phase_switches{}, consumed_witness_sites{};
  u64 credit_events{}, credited_pair_mass{}, splits_after_credit{};
  u64 singleton_handoffs{}, handoffs_after_credit{}, handoff_pair_mass{};
  u64 rejected_pairs{}, accepted_pairs{}, max_depth{};
  bool operator==(const Q2JointWork&) const = default;
};

struct Q2OrderWork {
  u64 structural_splits{};
  u64 deferred_skips{};
  u64 anchor_skips{};
  u64 phase_switches{};
  bool operator==(const Q2OrderWork&) const = default;
};

struct Q2SiblingWork {
  u64 proposals{};
  u64 cardinality_skips{};
  u64 bound_tests{};
  u64 rejected_tasks{};
  u64 rejected_pairs{};
  u64 rejected_after_credit{};
  bool operator==(const Q2SiblingWork&) const = default;
};

struct WspdQ2CensusResult {
  WspdFrontResult front;
  Q2CensusResult census;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
  Q2JointWork joint_work;
  Q2PoolWork pool_work;
  u64 input_rectangles{};
  u64 anchor_queries{};  // Sum min(|A|,|B|), not expanded pair count.
  double total_ms{};
};

// Integrated q2-only traversal. Every unordered pair is either safely
// rejected by the front/Pool or counted against ALL sites of this exact index.
// The smaller factor supplies anchors (ties preserve front orientation).
// SharedBlocks uses the other global node directly: no axis plan, factor
// copy, local B tree or cover scan. Pairwise expands the same residual.
// Only accepted supports trigger payload collection; subdivision may reach
// individual rejected pairs. Every interior and shell ID is collected,
// including distinct supports with the same ball key.
//
// No front credits seed census counts. An initial task starts at (0,root);
// its children inherit count and the fixed, unconsumed Z cursor together.
// Index/consumer are borrowed for the whole synchronous call. Exceptions
// propagate without rolling back earlier callbacks. No freely adoptable
// rectangle/continuation handles are accepted by this entry point.
// Saturating (SharedBlocks only) tests the opposite B child after a split.
// It rejects only if that sibling ALONE certifies K strict interiors for
// every query in this child. It never adds a credit or changes the Z cursor,
// even if these sites already belong to the consumed prefix. Failure leaves
// the ordinary census unchanged. One constant-cost proposal per child;
// no witness search, population copy or additional allocation.
// ComplementFirst (SharedBlocks only) fixes a different Z order per root:
// original B's complement without the anchor, then original B. Children
// inherit that SAME original B, phase, cursor and count. Proper ancestors
// of the deferred node and anchor leaf are split structurally before any
// bound/consumption. Only the depth count omits the anchor (always H=0);
// payload collection still includes every shell site. Structural work is
// separate from count_node_visits, which still counts geometric tests.
// SharedProduct (SharedBlocks only) keeps both factors until A is a
// singleton. Its common count and continuation are then handed to the
// existing anchor task without restarting Z. A is never excluded as a
// whole: another anchor can be an interior witness. ComplementFirst
// defers the original B while A is grouped; only singleton handoffs may
// skip their own known-zero anchor. Joint work is separate from anchor
// work. anchor_queries remains the descriptive sum of smaller factors,
// while count_root_starts equals root_products + pool_work.pair_roots
// in SharedProduct mode (the latter term is zero without effective Pool).
// Sibling certificates are tested only at subsequent anchor-task B splits.
// SharedAnchors uses the same joint bounds but splits only A before the
// singleton handoff, preserving the original B for its anchor task. Thus
// it cannot hand off an original anchor more than once per rectangle.
//
// pool_min_factor=0 disables the terminal Pool filter (the default).
// Otherwise a terminal rectangle with max(|A|,|B|)>=pool_min_factor
// prepares ONE local credit plan on the same index. Its at most K disjoint
// pair bands are expanded, without testing the rejected Cartesian product.
// If Pool removes no pair, the original census route is retained; the paid
// plan is still counted in selected_rectangles/selected_pairs,
// original_selected_anchors, F, preparation and passthrough_*.
// selected_anchors counts only anchors expanded by an effective Pool plan.
// Otherwise each survivor starts its full global census at zero; h_a+h_b
// is never preloaded. These reduced rectangles use Pairwise regardless of
// the anchor/order/sibling options, which still govern every other rectangle.
// All existing option compatibility checks remain mandatory before emission.
// No new cloud, factor coordinates, query tree, or global pair array is built.
// pool_work counts the extra O(K*sum factor sizes) preparation separately;
// selected_pairs=residual_pairs+filtered_pairs and
// front.residual_pair_mass[0]=census.candidate_pairs+filtered_pairs.
// anchor_queries/input_descriptors still describe ALL front rectangles.
// The plan and its permutations remain stable for its synchronous callbacks.
// Future distributed jobs must retain that parent plan rather than prepare
// new credits or rescan B per job. No async lifetime is provided here.
//
// total_ms and census.total_ms are the same enclosing FRONT+CENSUS interval,
// including destruction of private payload buffers. census.count_ms includes
// the front and counting overhead; payload_ms includes collection/callbacks;
// query_index_ms is zero. Only Pool-selected rectangles have local clocks:
// selected_total_ms includes preparation, census, payload and plan destruction;
// preparation_ms and payload_ms overlap it and must not be added to it.
// plan_peak_bytes measures retained vector capacities, not peak RSS. This is a
// stream of q2 supports, not a canonical ball catalogue or an HGP FULL tower.
// front_proposals only widens the front's rejection heuristic
// (WspdFrontProposals); the census still starts every count from zero.
[[nodiscard]] WspdQ2CensusResult run_wspd_q2_census(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    const Q2CensusConsumer& consumer,
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs,
    Q2AnchorMode anchor_mode = Q2AnchorMode::Individual,
    std::size_t pool_min_factor = 0, WspdFrontProposals front_proposals = {});

}  // namespace mhgp9::gen
