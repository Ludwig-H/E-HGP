#pragma once

#include "pipeline/wspd_q2_census.hpp"

#include <algorithm>

namespace mhgp8::parallel_detail {

// Explicit member-wise reduction: no layout/padding or aliasing assumptions.
// All additive counters use checked arithmetic; depths/capacities use maxima.

inline void merge_work(WspdFrontDispatchWork& out, const WspdFrontDispatchWork& value) {
  counter_add(out.seeds_started, value.seeds_started);
  counter_add(out.seeds_completed, value.seeds_completed);
  counter_add(out.donations, value.donations);
  counter_add(out.donor_checks, value.donor_checks);
  counter_add(out.offer_attempts, value.offer_attempts);
  counter_add(out.offer_full, value.offer_full);
  counter_add(out.offer_busy, value.offer_busy);
  counter_add(out.offer_no_demand, value.offer_no_demand);
  counter_add(out.stolen_started, value.stolen_started);
  counter_add(out.stolen_completed, value.stolen_completed);
  counter_add(out.waits, value.waits);
  counter_add(out.wakes, value.wakes);
  out.max_queue_size = std::max(out.max_queue_size, value.max_queue_size);
  out.max_local_stack_size = std::max(out.max_local_stack_size, value.max_local_stack_size);
}

inline void merge_work(WspdFrontWork& out, const WspdFrontWork& value) {
  counter_add(out.product_visits, value.product_visits);
  counter_add(out.diagonal_splits, value.diagonal_splits);
  counter_add(out.diagonal_leaves, value.diagonal_leaves);
  counter_add(out.disjoint_splits, value.disjoint_splits);
  counter_add(out.separation_tests, value.separation_tests);
  counter_add(out.witness_searches, value.witness_searches);
  counter_add(out.witness_descent_steps, value.witness_descent_steps);
  counter_add(out.witness_box_distance_tests, value.witness_box_distance_tests);
  counter_add(out.proposed_sites, value.proposed_sites);
  counter_add(out.proposals_in_factors, value.proposals_in_factors);
  counter_add(out.h_bound_tests, value.h_bound_tests);
  counter_add(out.xi_bound_tests, value.xi_bound_tests);
  counter_add(out.witness_lane_credits, value.witness_lane_credits);
  counter_add(out.fully_rejected_products, value.fully_rejected_products);
  counter_add(out.emitted_rectangles, value.emitted_rectangles);
  counter_add(out.emitted_factor_sites, value.emitted_factor_sites);
  out.max_factor_size = std::max(out.max_factor_size, value.max_factor_size);
  counter_add(out.leaf_pair_rectangles, value.leaf_pair_rectangles);
  out.max_stack_size = std::max(out.max_stack_size, value.max_stack_size);
  out.max_product_depth = std::max(out.max_product_depth, value.max_product_depth);
  for (std::size_t i = 0; i < 5; ++i) counter_add(out.size_class_rectangles[i], value.size_class_rectangles[i]);
  for (std::size_t i = 0; i < 5; ++i) counter_add(out.size_class_pair_mass[i], value.size_class_pair_mass[i]);
  for (std::size_t i = 0; i < 3; ++i) counter_add(out.rejected_pair_mass[i], value.rejected_pair_mass[i]);
  for (std::size_t i = 0; i < 3; ++i) counter_add(out.residual_pair_mass[i], value.residual_pair_mass[i]);
  for (std::size_t i = 0; i < 3; ++i) counter_add(out.lane_rectangles[i], value.lane_rectangles[i]);
  counter_add(out.extended_products, value.extended_products);
  counter_add(out.extended_proposals, value.extended_proposals);
  counter_add(out.extended_proposals_in_factors, value.extended_proposals_in_factors);
  counter_add(out.extended_credits, value.extended_credits);
  counter_add(out.extended_rejections, value.extended_rejections);
  // 25 scalars, two arrays of five and three arrays of three: a new field
  // that is not merged above must fail to compile here, not pass by vacuity.
  static_assert(sizeof(WspdFrontWork) == (25 + 2 * 5 + 3 * 3) * sizeof(u64),
                "merge_work(WspdFrontWork) must be updated with the structure");
}

inline void merge_work(Q2CensusWork& out, const Q2CensusWork& value) {
  counter_add(out.query_build_point_visits, value.query_build_point_visits);
  counter_add(out.query_build_nodes, value.query_build_nodes);
  out.query_build_max_depth = std::max(out.query_build_max_depth, value.query_build_max_depth);
  counter_add(out.input_descriptors, value.input_descriptors);
  counter_add(out.query_cover_visits, value.query_cover_visits);
  counter_add(out.query_tasks, value.query_tasks);
  counter_add(out.query_splits, value.query_splits);
  counter_add(out.witness_splits, value.witness_splits);
  counter_add(out.count_root_starts, value.count_root_starts);
  counter_add(out.shared_splits_after_credit, value.shared_splits_after_credit);
  counter_add(out.cursor_advances, value.cursor_advances);
  counter_add(out.cursor_reuses, value.cursor_reuses);
  counter_add(out.count_node_visits, value.count_node_visits);
  counter_add(out.count_bound_tests, value.count_bound_tests);
  counter_add(out.count_point_tests, value.count_point_tests);
  counter_add(out.uniform_credited_pairs, value.uniform_credited_pairs);
  counter_add(out.uniform_rejected_pairs, value.uniform_rejected_pairs);
  counter_add(out.uniform_accepted_pairs, value.uniform_accepted_pairs);
  counter_add(out.consumed_witness_sites, value.consumed_witness_sites);
  counter_add(out.frontier_restarts, value.frontier_restarts);
  counter_add(out.payload_node_visits, value.payload_node_visits);
  counter_add(out.payload_bound_tests, value.payload_bound_tests);
  counter_add(out.payload_point_tests, value.payload_point_tests);
  counter_add(out.payload_interior_sites, value.payload_interior_sites);
  counter_add(out.payload_shell_sites, value.payload_shell_sites);
  counter_add(out.payload_supports, value.payload_supports);
}

inline void merge_work(Q2SiblingWork& out, const Q2SiblingWork& value) {
  counter_add(out.proposals, value.proposals);
  counter_add(out.cardinality_skips, value.cardinality_skips);
  counter_add(out.bound_tests, value.bound_tests);
  counter_add(out.rejected_tasks, value.rejected_tasks);
  counter_add(out.rejected_pairs, value.rejected_pairs);
  counter_add(out.rejected_after_credit, value.rejected_after_credit);
}

inline void merge_work(Q2OrderWork& out, const Q2OrderWork& value) {
  counter_add(out.structural_splits, value.structural_splits);
  counter_add(out.deferred_skips, value.deferred_skips);
  counter_add(out.anchor_skips, value.anchor_skips);
  counter_add(out.phase_switches, value.phase_switches);
}

inline void merge_work(Q2JointWork& out, const Q2JointWork& value) {
  counter_add(out.root_products, value.root_products);
  counter_add(out.tasks, value.tasks);
  counter_add(out.splits_a, value.splits_a);
  counter_add(out.splits_b, value.splits_b);
  counter_add(out.witness_splits, value.witness_splits);
  counter_add(out.bound_tests, value.bound_tests);
  counter_add(out.cursor_advances, value.cursor_advances);
  counter_add(out.structural_splits, value.structural_splits);
  counter_add(out.deferred_skips, value.deferred_skips);
  counter_add(out.phase_switches, value.phase_switches);
  counter_add(out.consumed_witness_sites, value.consumed_witness_sites);
  counter_add(out.credit_events, value.credit_events);
  counter_add(out.credited_pair_mass, value.credited_pair_mass);
  counter_add(out.splits_after_credit, value.splits_after_credit);
  counter_add(out.singleton_handoffs, value.singleton_handoffs);
  counter_add(out.handoffs_after_credit, value.handoffs_after_credit);
  counter_add(out.handoff_pair_mass, value.handoff_pair_mass);
  counter_add(out.rejected_pairs, value.rejected_pairs);
  counter_add(out.accepted_pairs, value.accepted_pairs);
  out.max_depth = std::max(out.max_depth, value.max_depth);
}

inline void merge_work(Q2PoolWork& out, const Q2PoolWork& value) {
  counter_add(out.selected_rectangles, value.selected_rectangles);
  counter_add(out.selected_pairs, value.selected_pairs);
  counter_add(out.residual_pairs, value.residual_pairs);
  counter_add(out.filtered_pairs, value.filtered_pairs);
  counter_add(out.factor_sites, value.factor_sites);
  counter_add(out.selection_tests, value.selection_tests);
  counter_add(out.witness_attempts, value.witness_attempts);
  counter_add(out.universal_queries, value.universal_queries);
  counter_add(out.q2_axis_terms, value.q2_axis_terms);
  counter_add(out.pool_selected, value.pool_selected);
  counter_add(out.pool_insertions, value.pool_insertions);
  counter_add(out.pool_shifted_entries, value.pool_shifted_entries);
  counter_add(out.prefix_class_visits, value.prefix_class_visits);
  counter_add(out.factor_read_visits, value.factor_read_visits);
  counter_add(out.grouping_visits, value.grouping_visits);
  counter_add(out.bands, value.bands);
  counter_add(out.selected_anchors, value.selected_anchors);
  counter_add(out.pair_roots, value.pair_roots);
  out.plan_peak_bytes = std::max(out.plan_peak_bytes, value.plan_peak_bytes);
  counter_add(out.original_selected_anchors, value.original_selected_anchors);
  counter_add(out.passthrough_rectangles, value.passthrough_rectangles);
  counter_add(out.passthrough_pairs, value.passthrough_pairs);
  counter_add(out.passthrough_anchors, value.passthrough_anchors);
  out.preparation_ms += value.preparation_ms;
  out.selected_total_ms += value.selected_total_ms;
}

}  // namespace mhgp8::parallel_detail
