#pragma once

#include "q2_census.hpp"
#include "wspd/front.hpp"

namespace mhgp8 {

enum class Q2SiblingMode { Disabled, Saturating };
enum class Q2WitnessOrder { GlobalDfs, ComplementFirst };

struct Q2OrderWork {
  u64 structural_splits{};
  u64 deferred_skips{};
  u64 anchor_skips{};
  u64 phase_switches{};
};

struct Q2SiblingWork {
  u64 proposals{};
  u64 cardinality_skips{};
  u64 bound_tests{};
  u64 rejected_tasks{};
  u64 rejected_pairs{};
  u64 rejected_after_credit{};
};

struct WspdQ2CensusResult {
  WspdFrontResult front;
  Q2CensusResult census;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
  u64 input_rectangles{};
  u64 anchor_queries{};  // Sum min(|A|,|B|), not expanded pair count.
  double total_ms{};
};

// Integrated q2-only traversal. Every unordered pair is either safely
// rejected by the front or counted against ALL sites of this exact index.
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
//
// total_ms and census.total_ms are the same enclosing FRONT+CENSUS interval,
// including destruction of private payload buffers. census.count_ms includes
// the front and counting overhead; payload_ms includes collection/callbacks;
// query_index_ms is zero. No per-rectangle clocks are inserted. This is a
// stream of q2 supports, not a canonical ball catalogue or an HGP FULL tower.
[[nodiscard]] WspdQ2CensusResult run_wspd_q2_census(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    const Q2CensusConsumer& consumer,
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs);

}  // namespace mhgp8
