#pragma once

#include "../pipeline/q2_census.hpp"

#include <cstdint>

namespace mhgp8 {

enum class Q34WitnessBoundsMode { Legacy, Exclusion, Affine };

struct Q34WitnessBoundsWork {
  u64 queries{};
  u64 pair_preparations{}, general_preparations{};
  u64 affine_h_tests{}, affine_xi_tests{};
  u64 xi_on_nonpositive_minimum{};
  u64 q3_exclusion_tests{}, q4_exclusion_tests{};
  u64 q3_excluded_nodes{}, q4_excluded_nodes{};
  u64 fully_excluded_nodes{}, mixed_terminal_nodes{};
  // All twelve members combine by SUM, never MAX.
  bool operator==(const Q34WitnessBoundsWork&) const = default;
};

struct Q34WitnessSearchWork {
  u64 queries{};
  u64 q3_queries{};
  u64 q4_queries{};
  u64 prepared_bounds{};
  u64 node_visits{};
  u64 h_bound_tests{};
  u64 xi_bound_tests{};
  u64 point_tests{};  // All visited singleton leaves, including H exclusions.
  u64 h_excluded_nodes{};
  u64 admitted_nodes{};  // At least one lane certified; may also be split.
  u64 fully_admitted_nodes{};  // All locally unresolved lanes certified.
  u64 leaf_remainders{};  // Leaf not H-excluded, with an uncertified lane.
  u64 split_nodes{};
  u64 q3_lane_tests{};
  u64 q4_lane_tests{};
  u64 q3_admitted_nodes{};  // Includes leaf admissions, not just large blocks.
  u64 q4_admitted_nodes{};
  u64 q3_credits{};  // Effective credits, individually saturated at K-1.
  u64 q4_credits{};  // Effective credits, individually saturated at K-2.
  u64 q3_rejected{};
  u64 q4_rejected{};
  u64 midpoint_box_tests{};  // Two child-distance tests for each split.
  u64 pending_nodes_skipped{};  // Popped frames whose lanes saturated elsewhere.
  u64 peak_stack{};  // MAX across calls; all preceding fields are SUM.
  u64 stack_storage_bytes{};  // MAX: actual fixed backing array, not logical fill.
  bool operator==(const Q34WitnessSearchWork&) const = default;
};

// Searches the SAME immutable global index; no cloud copy, factor scan,
// persistent witness frontier, consumer or heap allocation. Return value is
// the surviving subset of lane_mask, after removing unavailable arities:
// K1 has neither lane, K2 only bit2 (q3), K>=3 bits2|4. K must be in [1,10]
// and lane_mask may be 0,2,4,6; any other bit is invalid. Both boxes are
// validated even for an inactive query, before changing work.
//
// Each lane starts with ZERO credits. Strict witnesses satisfy H>0 and
// alpha_q H^2>Xi, with alpha3=3, alpha4=2, and separate thresholds K-1/K-2.
// Whole-node admission certifies ALL sites in that node for ALL pairs in
// the continuous boxes. A/B singleton therefore gives the exact saturated
// citron decision. General boxes use sufficient conservative bounds, not
// a complete oracle for universal-over-discrete-factor witnesses. Hmin>0
// itself excludes a or b: such a point would give H=0 for an endpoint
// choice. This also excludes any witness site lying within either box.
//
// Credits prove only rejection of positive q-supports having this edge as a
// longest support edge. They are NOT initial counts for a later census and
// they do not establish q3 acceptance, q4 acceptance, catalogue or FULL.
// A lane admitted at a node is removed from that node's CHILD mask, even if
// the other lane must refine. Thus disjoint node populations are never
// counted twice in either lane. Equality/tangency never earns a credit.
//
// Work is accumulated with checked u64 additions and MAX for the last two
// members. Index and work are borrowed throughout this synchronous call;
// concurrent calls may share the immutable index, never the same work object
// without synchronization. Counter overflow throws; work can then be partial,
// and no surviving-mask result is returned. Invalid arguments change no work.
// Completed-call identities (also valid after SUM/MAX merging):
// node_visits = h_bound_tests = h_excluded_nodes + fully_admitted_nodes +
// leaf_remainders + split_nodes; midpoint_box_tests = 2*split_nodes.
//
// DFS uses at most49 pending frames because Q2CensusIndex::build splits the
// longest positive integer coordinate extent at its midpoint. Each child
// has at most floor(extent/2) in that coordinate; sixteen such reductions
// per coordinate reach zero. Unique u16 sites force a singleton by depth48.
// The fixed backing is this proven bound, not a search/point/visit quota.
// Work O(visited nodes), with no universal subquadratic bound on all calls.
[[nodiscard]] std::uint8_t filter_q34_witnesses(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work);

// Explicit new path; the six-argument entry above stays STRICTLY Legacy.
// Legacy leaves bounds_work unchanged, including on success. Exclusion uses
// the general Xi lower bound to exclude a node's witnesses separately per
// lane. Affine also prepares fixed-endpoint H/Xi when BOTH boxes are points;
// otherwise it is the same general preparation as Exclusion. No preparation
// is doubled. Invalid mode/K/mask/boxes leave both work objects unchanged.
// Active and inactive valid non-Legacy calls increment bounds_work.queries;
// only active calls prepare bounds and increment exactly one preparation.
//
// After Hmax4<=0 has been handled, alpha*Hmax4^2<=16*Xi_low excludes that
// lane LOCALLY: zero credit and NO change to the global surviving mask.
// Xi is therefore also paid when Hmin4<=0, counted separately. Admission is
// still strict Hmin4>0 && alpha*Hmin4^2>16*Xi_high. Exclusions are tested
// before admissions; old lane_tests count only admissions actually tested.
// All masks, endpoints, threshold saturation, borrowing and exception rules
// above remain unchanged; no inherited witness credit is introduced.
//
// q3/q4_excluded_nodes include partial lane exclusions. fully_excluded_nodes
// means every input lane was excluded by Xi, and mixed_terminal_nodes means
// at least one input lane admitted and another excluded, with none pending.
// old fully_admitted_nodes continues to mean ALL input lanes were admitted;
// old admitted_nodes means at least one admission, even on mixed terminals.
// The new completed-call partition is:
// node_visits = h_excluded_nodes + fully_admitted_nodes + leaf_remainders +
// split_nodes + fully_excluded_nodes + mixed_terminal_nodes.
// Affine counts only actual affine tests; Search25.h_bound_tests/xi_bound_tests
// include either arithmetic path. No additional heap or frontier allocation.
[[nodiscard]] std::uint8_t filter_q34_witnesses(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work,
    Q34WitnessBoundsMode mode, Q34WitnessBoundsWork& bounds_work);

}  // namespace mhgp8
