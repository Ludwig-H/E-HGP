#include "pipeline/wspd_q34.hpp"

#include "parallel/joined_workers.hpp"
#include "parallel/work_reduction.hpp"

#include <algorithm>
#include <optional>
#include <array>
#include <functional>
#include <atomic>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <exception>
#include <thread>
#include <mutex>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp9::gen {
namespace {

// The same immutable index is shared by the front, every cover and both
// lanes. These reductions retain every existing backend work field; their
// compile-time size checks prevent silently omitting a newly added counter.
#define MHGP9G_ADD(field) counter_add(a.field, b.field)
#define MHGP9G_MAX(field) a.field = std::max(a.field, b.field)

void merge(WspdQ3AtlasWork& a, const WspdQ3AtlasWork& b) {
  static_assert(sizeof(WspdQ3AtlasWork) == 9 * sizeof(u64));
  MHGP9G_ADD(edges_with_atlas); MHGP9G_ADD(root_lane_skips); MHGP9G_ADD(locations);
  MHGP9G_ADD(outside_domain); MHGP9G_ADD(rejections);
  MHGP9G_ADD(leaf_censuses); MHGP9G_ADD(leaf_point_tests); MHGP9G_ADD(leaf_rejections);
  MHGP9G_ADD(lower_bound_fallbacks);
}

void merge(Q34EdgeCoverWork& a, const Q34EdgeCoverWork& b) {
  static_assert(sizeof(Q34EdgeCoverWork) == 10 * sizeof(u64));
  MHGP9G_ADD(node_visits); MHGP9G_ADD(bound_tests); MHGP9G_ADD(point_tests);
  MHGP9G_ADD(admitted_nodes); MHGP9G_ADD(rejected_nodes); MHGP9G_ADD(split_nodes);
  MHGP9G_ADD(admitted_sites); MHGP9G_ADD(rejected_sites); MHGP9G_ADD(retained_ranges);
  MHGP9G_ADD(merged_ranges);
}

void merge(Q4PositiveDomainWork& a, const Q4PositiveDomainWork& b) {
  static_assert(sizeof(Q4PositiveDomainWork) == 12 * sizeof(u64));
  MHGP9G_ADD(node_visits); MHGP9G_ADD(bound_tests); MHGP9G_ADD(endpoint_box_tests);
  MHGP9G_ADD(endpoint_leaf_tests); MHGP9G_ADD(point_tests); MHGP9G_ADD(admitted_nodes);
  MHGP9G_ADD(rejected_nodes); MHGP9G_ADD(split_nodes); MHGP9G_ADD(excluded_endpoints);
  MHGP9G_ADD(admitted_sites); MHGP9G_ADD(rejected_sites); MHGP9G_ADD(box_merges);
}

void merge(Q4LocalGeometryQueryWork& a, const Q4LocalGeometryQueryWork& b) {
  static_assert(sizeof(Q4LocalGeometryQueryWork) == 2 * sizeof(u64));
  MHGP9G_ADD(disk_tests); MHGP9G_ADD(facet_tests);
}

void merge(Q4LocalGeometryWork& a, const Q4LocalGeometryWork& b) {
  static_assert(sizeof(Q4LocalGeometryWork) == 15 * sizeof(u64) + sizeof(Q4PositiveDomainWork));
  merge(a.domain, b.domain);
  MHGP9G_ADD(preparations); MHGP9G_ADD(cover_node_visits); MHGP9G_ADD(cover_range_advances);
  MHGP9G_ADD(cover_disjoint_nodes); MHGP9G_ADD(cover_splits); MHGP9G_ADD(cover_blocks);
  MHGP9G_ADD(cover_sites); MHGP9G_ADD(cover_excluded_sites); MHGP9G_ADD(cover_node_ids_copied);
  MHGP9G_ADD(projection_points); MHGP9G_ADD(hull_sort_comparisons); MHGP9G_ADD(hull_orientation_tests);
  MHGP9G_ADD(hull_vertices); MHGP9G_ADD(facets);
  MHGP9G_MAX(peak_retained_bytes);
}

void merge(Q4LocalPartitionWork& a, const Q4LocalPartitionWork& b) {
  static_assert(sizeof(Q4LocalPartitionWork) == 20 * sizeof(u64));
  MHGP9G_ADD(root_factories); MHGP9G_ADD(child_factories); MHGP9G_ADD(refine_factories);
  MHGP9G_ADD(input_nodes); MHGP9G_ADD(input_sites); MHGP9G_ADD(inherited_inside_sites);
  MHGP9G_ADD(node_visits); MHGP9G_ADD(block_bound_tests); MHGP9G_ADD(point_tests);
  MHGP9G_ADD(z_splits); MHGP9G_ADD(inside_nodes); MHGP9G_ADD(outside_nodes);
  MHGP9G_ADD(inside_sites); MHGP9G_ADD(outside_sites); MHGP9G_ADD(active_nodes);
  MHGP9G_ADD(active_sites); MHGP9G_ADD(budget_unexamined_nodes); MHGP9G_ADD(budget_ambiguous_nodes);
  MHGP9G_ADD(frontier_ids_copied);
  MHGP9G_MAX(peak_retained_bytes);
}

void merge(Q4LocalAtlasWork& a, const Q4LocalAtlasWork& b) {
  static_assert(sizeof(Q4LocalAtlasWork) == 16 * sizeof(u64) + sizeof(Q4LocalPartitionWork) + sizeof(Q4LocalGeometryQueryWork));
  merge(a.partition, b.partition);
  merge(a.domain, b.domain);
  MHGP9G_ADD(cells_created); MHGP9G_ADD(outside_cells); MHGP9G_ADD(deep_cells);
  MHGP9G_ADD(leaf_cells); MHGP9G_ADD(splits); MHGP9G_ADD(depth_stops);
  MHGP9G_ADD(node_stops); MHGP9G_ADD(small_stops); MHGP9G_ADD(active_sites_sum);
  MHGP9G_ADD(active_blocks_sum); MHGP9G_ADD(terminal_refinements); MHGP9G_ADD(terminal_deep_cells);
  MHGP9G_MAX(max_depth); MHGP9G_MAX(peak_fragment_bytes); MHGP9G_MAX(peak_build_bytes);
  MHGP9G_MAX(retained_bytes);
}

void merge(Q4LocalSweepWork& a, const Q4LocalSweepWork& b) {
  static_assert(sizeof(Q4LocalSweepWork) == 41 * sizeof(u64));
  MHGP9G_ADD(seed_queries); MHGP9G_ADD(seed_owner_tests); MHGP9G_ADD(seed_owner_rejections);
  MHGP9G_ADD(query_visits); MHGP9G_ADD(line_tests); MHGP9G_ADD(line_skips);
  MHGP9G_ADD(leaf_queries); MHGP9G_ADD(reference_points); MHGP9G_ADD(reference_side_tests);
  MHGP9G_ADD(active_blocks); MHGP9G_ADD(active_sites); MHGP9G_ADD(root_locations);
  MHGP9G_ADD(clipped_events); MHGP9G_ADD(clipped_inside); MHGP9G_ADD(kept_events);
  MHGP9G_ADD(constant_inside); MHGP9G_ADD(constant_outside); MHGP9G_ADD(constant_shell_ids);
  MHGP9G_ADD(entries); MHGP9G_ADD(exits); MHGP9G_ADD(sort_comparisons);
  MHGP9G_ADD(shell_sort_comparisons); MHGP9G_ADD(group_comparisons); MHGP9G_ADD(groups);
  MHGP9G_ADD(boundary_skips); MHGP9G_ADD(boundary_skipped_ids); MHGP9G_ADD(depth_rejections);
  MHGP9G_ADD(depth_skipped_ids); MHGP9G_ADD(presentations); MHGP9G_ADD(owner_tests);
  MHGP9G_ADD(owner_rejections); MHGP9G_ADD(positive_tests); MHGP9G_ADD(positive_rejections);
  MHGP9G_ADD(canonical_tests); MHGP9G_ADD(canonical_rejections); MHGP9G_ADD(emitted);
  MHGP9G_ADD(shell_ids); MHGP9G_ADD(groups_without_support); MHGP9G_ADD(unexamined_after_emit);
  MHGP9G_MAX(max_group); MHGP9G_MAX(peak_buffer_bytes);
}

void merge(Q4LocalEdgeWork& a, const Q4LocalEdgeWork& b) {
  static_assert(sizeof(Q4LocalEdgeWork) == 11 * sizeof(u64) + sizeof(Q4LocalAtlasWork) + sizeof(Q4LocalGeometryWork) + sizeof(Q4LocalSweepWork));
  merge(a.atlas, b.atlas);
  merge(a.geometry, b.geometry);
  merge(a.sweep, b.sweep);
  MHGP9G_ADD(node_visits); MHGP9G_ADD(bound_tests); MHGP9G_ADD(point_tests);
  MHGP9G_ADD(rejected_nodes); MHGP9G_ADD(split_nodes); MHGP9G_ADD(rejected_sites);
  MHGP9G_ADD(acute_seeds); MHGP9G_ADD(owner_tests); MHGP9G_ADD(owner_rejections);
  MHGP9G_ADD(seeds);
  MHGP9G_MAX(peak_live_buffer_bytes);
}

void merge(Q4ShallowSetWork& a, const Q4ShallowSetWork& b) {
  static_assert(sizeof(Q4ShallowSetWork) == 25 * sizeof(u64));
  MHGP9G_ADD(preparations); MHGP9G_ADD(input_sites); MHGP9G_ADD(form_tests);
  MHGP9G_ADD(zero_sites); MHGP9G_ADD(positive_sites); MHGP9G_ADD(negative_sites);
  MHGP9G_ADD(lex_comparisons); MHGP9G_ADD(orientation_tests); MHGP9G_ADD(coordinate_groups);
  MHGP9G_ADD(duplicate_ids); MHGP9G_ADD(positive_layers); MHGP9G_ADD(negative_layers);
  MHGP9G_ADD(layer_input_groups); MHGP9G_ADD(layer_input_ids); MHGP9G_ADD(boundary_groups);
  MHGP9G_ADD(degenerate_groups); MHGP9G_ADD(retained_ids); MHGP9G_ADD(discarded_ids);
  MHGP9G_ADD(retained_id_sort_comparisons); MHGP9G_ADD(record_insertions); MHGP9G_ADD(group_insertions);
  MHGP9G_ADD(hull_index_copies); MHGP9G_ADD(compaction_moves);
  MHGP9G_MAX(peak_live_bytes); MHGP9G_MAX(retained_bytes);
}

void merge(Q4FamilyWork& a, const Q4FamilyWork& b) {
  static_assert(sizeof(Q4FamilyWork) == 13 * sizeof(u64));
  MHGP9G_ADD(sites); MHGP9G_ADD(entries); MHGP9G_ADD(exits);
  MHGP9G_ADD(constant_inside); MHGP9G_ADD(constant_on); MHGP9G_ADD(constant_outside);
  MHGP9G_ADD(sort_comparisons); MHGP9G_ADD(group_comparisons); MHGP9G_ADD(groups);
  MHGP9G_ADD(callbacks); MHGP9G_ADD(event_count);
  MHGP9G_MAX(max_group); MHGP9G_MAX(retained_capacity_bytes);
}

void merge(Q4ShallowSweepWork& a, const Q4ShallowSweepWork& b) {
  static_assert(sizeof(Q4ShallowSweepWork) == 19 * sizeof(u64) + sizeof(Q4FamilyWork));
  merge(a.family, b.family);
  MHGP9G_ADD(seed_queries); MHGP9G_ADD(seed_owner_tests); MHGP9G_ADD(seed_owner_rejections);
  MHGP9G_ADD(removed_seed_rejections); MHGP9G_ADD(membership_comparisons); MHGP9G_ADD(depth_rejected_groups);
  MHGP9G_ADD(depth_skipped_ids); MHGP9G_ADD(presentations); MHGP9G_ADD(owner_tests);
  MHGP9G_ADD(owner_rejections); MHGP9G_ADD(positive_tests); MHGP9G_ADD(positive_rejections);
  MHGP9G_ADD(canonical_tests); MHGP9G_ADD(canonical_rejections); MHGP9G_ADD(groups_without_support);
  MHGP9G_ADD(unexamined_after_emit); MHGP9G_ADD(emitted); MHGP9G_ADD(shell_ids);
  MHGP9G_MAX(peak_buffer_bytes);
}

void merge(Q4WindowSelectionWork& a, const Q4WindowSelectionWork& b) {
  static_assert(sizeof(Q4WindowSelectionWork) == 25 * sizeof(u64));
  MHGP9G_ADD(seed_queries); MHGP9G_ADD(entry_heap_insertions); MHGP9G_ADD(exit_heap_insertions);
  MHGP9G_ADD(entry_heap_replacements); MHGP9G_ADD(exit_heap_replacements); MHGP9G_ADD(heap_comparisons);
  MHGP9G_ADD(heap_sort_comparisons); MHGP9G_ADD(constant_rejected_seeds); MHGP9G_ADD(disjoint_rejected_seeds);
  MHGP9G_ADD(fixed_depth_rejected_seeds); MHGP9G_ADD(lower_bounds); MHGP9G_ADD(upper_bounds);
  MHGP9G_ADD(point_windows); MHGP9G_ADD(second_pass_sites); MHGP9G_ADD(window_comparisons);
  MHGP9G_ADD(lower_ids); MHGP9G_ADD(upper_ids); MHGP9G_ADD(inner_ids);
  MHGP9G_ADD(outside_ids); MHGP9G_ADD(fixed_inside_sites); MHGP9G_ADD(rejected_event_ids);
  MHGP9G_MAX(max_inner_ids); MHGP9G_MAX(max_endpoint_ids); MHGP9G_MAX(peak_heap_bytes);
  MHGP9G_MAX(peak_buffer_bytes);
}

void merge(Q4WindowSweepWork& a, const Q4WindowSweepWork& b) {
  static_assert(sizeof(Q4WindowSweepWork) == 0 * sizeof(u64) + sizeof(Q4ShallowSweepWork) + sizeof(Q4WindowSelectionWork));
  merge(a.sweep, b.sweep);
  merge(a.window, b.window);
}

void merge(Q4WindowEdgeWork& a, const Q4WindowEdgeWork& b) {
  static_assert(sizeof(Q4WindowEdgeWork) == 7 * sizeof(u64) + sizeof(Q4LocalGeometryWork) + sizeof(Q4ShallowSetWork) + sizeof(Q4WindowSweepWork));
  merge(a.geometry, b.geometry);
  merge(a.selection, b.selection);
  merge(a.sweep, b.sweep);
  MHGP9G_ADD(seed_candidates); MHGP9G_ADD(acute_tests); MHGP9G_ADD(acute_seeds);
  MHGP9G_ADD(owner_tests); MHGP9G_ADD(owner_rejections); MHGP9G_ADD(seeds);
  MHGP9G_MAX(peak_live_buffer_bytes);
}

void merge(WspdQ3Work& a, const WspdQ3Work& b) {
  static_assert(sizeof(WspdQ3Work) == 23 * sizeof(u64));
  MHGP9G_ADD(edge_queries); MHGP9G_ADD(seed_node_visits); MHGP9G_ADD(seed_bound_tests);
  MHGP9G_ADD(seed_point_tests); MHGP9G_ADD(seed_rejected_nodes); MHGP9G_ADD(seed_split_nodes);
  MHGP9G_ADD(seed_rejected_sites); MHGP9G_ADD(acute_seeds); MHGP9G_ADD(owner_tests);
  MHGP9G_ADD(owner_rejections); MHGP9G_ADD(seeds); MHGP9G_ADD(ball_builds);
  MHGP9G_ADD(census_range_visits); MHGP9G_ADD(census_point_tests); MHGP9G_ADD(census_inside_sites);
  MHGP9G_ADD(census_outside_sites); MHGP9G_ADD(census_shell_sites); MHGP9G_ADD(depth_rejections);
  MHGP9G_ADD(early_unread_sites); MHGP9G_ADD(shell_sort_comparisons); MHGP9G_ADD(shell_ids);
  MHGP9G_ADD(emitted); MHGP9G_MAX(peak_shell_bytes);
}

void merge(Q34WitnessSearchWork& a, const Q34WitnessSearchWork& b) {
  static_assert(sizeof(Q34WitnessSearchWork) == 25 * sizeof(u64));
  MHGP9G_ADD(queries); MHGP9G_ADD(q3_queries); MHGP9G_ADD(q4_queries);
  MHGP9G_ADD(prepared_bounds); MHGP9G_ADD(node_visits); MHGP9G_ADD(h_bound_tests);
  MHGP9G_ADD(xi_bound_tests); MHGP9G_ADD(point_tests); MHGP9G_ADD(h_excluded_nodes);
  MHGP9G_ADD(admitted_nodes); MHGP9G_ADD(fully_admitted_nodes); MHGP9G_ADD(leaf_remainders);
  MHGP9G_ADD(split_nodes); MHGP9G_ADD(q3_lane_tests); MHGP9G_ADD(q4_lane_tests);
  MHGP9G_ADD(q3_admitted_nodes); MHGP9G_ADD(q4_admitted_nodes);
  MHGP9G_ADD(q3_credits); MHGP9G_ADD(q4_credits); MHGP9G_ADD(q3_rejected); MHGP9G_ADD(q4_rejected);
  MHGP9G_ADD(midpoint_box_tests); MHGP9G_ADD(pending_nodes_skipped);
  MHGP9G_MAX(peak_stack); MHGP9G_MAX(stack_storage_bytes);
}

void merge(Q34WitnessBoundsWork& a, const Q34WitnessBoundsWork& b) {
  static_assert(sizeof(Q34WitnessBoundsWork) == 12 * sizeof(u64));
  MHGP9G_ADD(queries); MHGP9G_ADD(pair_preparations); MHGP9G_ADD(general_preparations);
  MHGP9G_ADD(affine_h_tests); MHGP9G_ADD(affine_xi_tests); MHGP9G_ADD(xi_on_nonpositive_minimum);
  MHGP9G_ADD(q3_exclusion_tests); MHGP9G_ADD(q4_exclusion_tests);
  MHGP9G_ADD(q3_excluded_nodes); MHGP9G_ADD(q4_excluded_nodes);
  MHGP9G_ADD(fully_excluded_nodes); MHGP9G_ADD(mixed_terminal_nodes);
}

void merge(WspdQ34WitnessWork& a, const WspdQ34WitnessWork& b) {
  static_assert(sizeof(WspdQ34WitnessWork) == 9 * sizeof(u64) +
      2 * sizeof(Q34WitnessSearchWork) + 2 * sizeof(Q34WitnessBoundsWork));
  MHGP9G_ADD(input_pair_mass); MHGP9G_ADD(rejected_rectangles); MHGP9G_ADD(rectangle_pair_mass);
  MHGP9G_ADD(rectangle_q3_pairs); MHGP9G_ADD(rectangle_q4_pairs);
  MHGP9G_ADD(rejected_pairs); MHGP9G_ADD(pair_q3_pairs); MHGP9G_ADD(pair_q4_pairs);
  MHGP9G_ADD(cache_rejected_pairs);
  merge(a.rectangles, b.rectangles); merge(a.pairs, b.pairs);
  merge(a.rectangles_bounds, b.rectangles_bounds); merge(a.pairs_bounds, b.pairs_bounds);
}

void merge(Q3BallCensusWork& a, const Q3BallCensusWork& b) {
  static_assert(sizeof(Q3BallCensusWork) == 26 * sizeof(u64));
  MHGP9G_ADD(queries); MHGP9G_ADD(preparations); MHGP9G_ADD(vertex_axes);
  MHGP9G_ADD(accepted_queries); MHGP9G_ADD(rejected_queries);
  MHGP9G_ADD(count_node_visits); MHGP9G_ADD(count_bounds_prepared);
  MHGP9G_ADD(count_box_bound_tests); MHGP9G_ADD(count_point_tests);
  MHGP9G_ADD(count_inside_nodes); MHGP9G_ADD(count_inside_sites);
  MHGP9G_ADD(count_nonnegative_nodes); MHGP9G_ADD(count_nonnegative_sites);
  MHGP9G_ADD(count_split_nodes); MHGP9G_ADD(count_prepared_unvisited); MHGP9G_ADD(count_saturations);
  MHGP9G_ADD(shell_node_visits); MHGP9G_ADD(shell_bounds_prepared);
  MHGP9G_ADD(shell_box_bound_tests); MHGP9G_ADD(shell_point_tests);
  MHGP9G_ADD(shell_excluded_nodes); MHGP9G_ADD(shell_split_nodes); MHGP9G_ADD(shell_ids);
  MHGP9G_MAX(peak_count_stack); MHGP9G_MAX(peak_shell_stack); MHGP9G_MAX(stack_storage_bytes);
}

void merge(Q4SeedCellWork& a, const Q4SeedCellWork& b) {
  static_assert(sizeof(Q4SeedCellWork) == 37 * sizeof(u64));
  MHGP9G_ADD(queries); MHGP9G_ADD(live_preparations); MHGP9G_ADD(live_node_visits);
  MHGP9G_ADD(live_child_reads); MHGP9G_ADD(live_leaves); MHGP9G_ADD(whole_atlas_skips);
  MHGP9G_ADD(live_skipped_nodes); MHGP9G_ADD(antichain_node_visits); MHGP9G_ADD(antichain_splits);
  MHGP9G_ADD(blocks); MHGP9G_ADD(block_sites); MHGP9G_ADD(cache_entries_initialized);
  MHGP9G_ADD(cache_hits); MHGP9G_ADD(cache_misses); MHGP9G_ADD(invalid_cache_hits);
  MHGP9G_ADD(family_preparations); MHGP9G_ADD(family_cache_hits); MHGP9G_ADD(form_preparations);
  MHGP9G_ADD(product_visits); MHGP9G_ADD(product_seed_rejections); MHGP9G_ADD(positive_products);
  MHGP9G_ADD(negative_products); MHGP9G_ADD(uncertain_products); MHGP9G_ADD(zero_bound_products);
  MHGP9G_ADD(block_bound_tests); MHGP9G_ADD(singleton_bound_tests); MHGP9G_ADD(spatial_tests_reused);
  MHGP9G_ADD(x_splits); MHGP9G_ADD(cell_splits); MHGP9G_ADD(terminal_pairs);
  MHGP9G_MAX(max_block_sites); MHGP9G_MAX(peak_cache_bytes); MHGP9G_MAX(peak_live_bytes);
  MHGP9G_MAX(peak_product_stack); MHGP9G_MAX(product_stack_bytes);
  MHGP9G_MAX(peak_auxiliary_bytes); MHGP9G_MAX(peak_total_buffer_bytes);
}

void merge(Q34DeadLaneWork& a, const Q34DeadLaneWork& b) {
  static_assert(sizeof(Q34DeadLaneWork) == 12 * sizeof(u64));
  MHGP9G_ADD(loads); MHGP9G_ADD(form_sites); MHGP9G_ADD(cells); MHGP9G_ADD(outside_cells);
  MHGP9G_ADD(deep_cells); MHGP9G_ADD(failed_cells); MHGP9G_ADD(uniform_tests); MHGP9G_ADD(point_tests);
  MHGP9G_ADD(q3_proved); MHGP9G_ADD(q3_open); MHGP9G_ADD(q4_proved); MHGP9G_ADD(q4_open);
}

void merge(Q34WitnessCacheWork& a, const Q34WitnessCacheWork& b) {
  static_assert(sizeof(Q34WitnessCacheWork) == 5 * sizeof(u64));
  MHGP9G_ADD(queries); MHGP9G_ADD(node_tests); MHGP9G_ADD(q3_rejections); MHGP9G_ADD(q4_rejections);
  MHGP9G_ADD(full_rejections);
}

void merge(Q34LanesWork& a, const Q34LanesWork& b) {
  static_assert(sizeof(Q34LanesWork) == 17 * sizeof(u64) + sizeof(Q34EdgeCoverWork));
  MHGP9G_ADD(edges); MHGP9G_ADD(cover_sites); MHGP9G_MAX(max_cover_sites);
  merge(a.cover, b.cover);
  MHGP9G_ADD(seed_tests); MHGP9G_ADD(acute_sites); MHGP9G_ADD(owner_rejections); MHGP9G_ADD(seeds);
  MHGP9G_ADD(q3_edges); MHGP9G_ADD(census_seeds);
  MHGP9G_ADD(census_point_tests); MHGP9G_ADD(census_inside_sites); MHGP9G_ADD(census_shell_sites);
  MHGP9G_ADD(census_outside_sites); MHGP9G_ADD(depth_rejections); MHGP9G_ADD(emitted); MHGP9G_ADD(shell_ids);
  MHGP9G_ADD(pruned_sites);
}

void merge(Q34Lanes4Work& a, const Q34Lanes4Work& b) {
  static_assert(sizeof(Q34Lanes4Work) == 28 * sizeof(u64));
  MHGP9G_ADD(edges); MHGP9G_ADD(seeds); MHGP9G_ADD(certified); MHGP9G_ADD(certified_chunk1);
  MHGP9G_ADD(survivors); MHGP9G_ADD(pass_chunks); MHGP9G_ADD(pass_site_tests); MHGP9G_ADD(buffered_events);
  MHGP9G_MAX(max_buffered); MHGP9G_ADD(live_buckets); MHGP9G_ADD(filter_steps); MHGP9G_ADD(bucket_events);
  MHGP9G_ADD(candidates); MHGP9G_ADD(foreign_candidates); MHGP9G_ADD(groups); MHGP9G_ADD(compare_steps);
  MHGP9G_ADD(depth_rejected_groups); MHGP9G_ADD(positivity_tests); MHGP9G_ADD(groups_without_valid);
  MHGP9G_ADD(emitted); MHGP9G_ADD(emitting_seeds); MHGP9G_ADD(multi_emission_seeds);
  MHGP9G_MAX(max_emissions_per_seed); MHGP9G_ADD(shell_ids); MHGP9G_MAX(max_group);
  MHGP9G_ADD(constant_shell_sites); MHGP9G_ADD(list_steps); MHGP9G_ADD(group_steps);
}

void merge(WspdQ34Work& a, const WspdQ34Work& b) {
  static_assert(sizeof(WspdQ34Work) == 16 * sizeof(u64) + 2 * sizeof(Q34EdgeCoverWork) +
      sizeof(WspdQ3Work) + sizeof(Q4LocalEdgeWork) + sizeof(Q4WindowEdgeWork) + sizeof(WspdQ34WitnessWork) + sizeof(Q3BallCensusWork) + sizeof(Q4SeedCellWork) + sizeof(WspdQ3AtlasWork) + 2 * sizeof(Q34DeadLaneWork) + sizeof(Q34WitnessCacheWork) + sizeof(Q34LanesWork) + sizeof(Q34Lanes4Work));
  MHGP9G_ADD(input_rectangles); MHGP9G_ADD(expanded_pairs); MHGP9G_ADD(q3_edges);
  MHGP9G_ADD(q4_edges); MHGP9G_ADD(both_edges); MHGP9G_ADD(cover_builds);
  MHGP9G_ADD(cover_sites); MHGP9G_MAX(max_cover_sites); MHGP9G_MAX(peak_cover_bytes);
  MHGP9G_ADD(q3_emitted); MHGP9G_ADD(q4_emitted); MHGP9G_ADD(payload_shell_ids);
  MHGP9G_MAX(peak_edge_buffer_bytes);
  merge(a.cover, b.cover); merge(a.q3, b.q3);
  merge(a.local, b.local); merge(a.window, b.window);
  merge(a.witness, b.witness);
  merge(a.q3_blocks, b.q3_blocks);
  merge(a.q4_seed_cells, b.q4_seed_cells);
  merge(a.q3_atlas, b.q3_atlas);
  merge(a.dead, b.dead);
  merge(a.witness_cache, b.witness_cache);
  MHGP9G_ADD(core_builds); MHGP9G_ADD(core_sites); MHGP9G_ADD(core_closed_edges);
  merge(a.core_cover, b.core_cover);
  merge(a.dead_core, b.dead_core);
  merge(a.lanes, b.lanes);
  merge(a.lanes4, b.lanes4);
}

#undef MHGP9G_ADD
#undef MHGP9G_MAX

u64 pair_count(std::size_t n) {
  if (n < 2) return 0;
  const i128 pairs = n % 2 == 0 ? static_cast<i128>(n / 2) * (n - 1)
                                : static_cast<i128>(n) * ((n - 1) / 2);
  if (pairs > std::numeric_limits<u64>::max())
    throw std::overflow_error("mhgp9 gen q34 pair mass exceeds u64");
  return static_cast<u64>(pairs);
}

u64 storage_bytes(std::size_t capacity, std::size_t element_size) {
  if (capacity > std::numeric_limits<u64>::max() / element_size)
    throw std::overflow_error("mhgp9 gen q34 capacity exceeds u64");
  return static_cast<u64>(capacity) * element_size;
}

u64 shell_bytes(std::size_t capacity) {
  return storage_bytes(capacity, sizeof(std::size_t));
}

void validate(Q2CensusIndexPtr index, unsigned k, unsigned s,
              const WspdQ34Options& options, bool consumer_valid) {
  if (!index || !consumer_valid || k == 0 || k > 10 || s == 0)
    throw std::invalid_argument("mhgp9 gen global q34 requires index, callback, K1..10 and positive s");
  if ((options.front_mode != WspdFrontMode::Pure &&
       options.front_mode != WspdFrontMode::MidpointSamples) ||
      (options.requested_lane_mask != 2 && options.requested_lane_mask != 4 &&
       options.requested_lane_mask != 6) ||
      (options.q4_backend != WspdQ4Backend::Local28 &&
       options.q4_backend != WspdQ4Backend::Window30) ||
      (options.witness_mode != WspdQ34WitnessMode::Disabled &&
       options.witness_mode != WspdQ34WitnessMode::Pair &&
       options.witness_mode != WspdQ34WitnessMode::RectanglePair) ||
      (options.q3_census_mode != WspdQ3CensusMode::ScalarCover &&
       options.q3_census_mode != WspdQ3CensusMode::GlobalBoxes) ||
      (options.witness_bounds_mode != Q34WitnessBoundsMode::Legacy &&
       options.witness_bounds_mode != Q34WitnessBoundsMode::Exclusion &&
       options.witness_bounds_mode != Q34WitnessBoundsMode::Affine))
    throw std::invalid_argument("mhgp9 gen unsupported global q34 options");
  // Match the unchanged local28 public contract, even if this call requests
  // no active q4 lane or selects Window30. Inert options cannot hide errors.
  if ((options.local.domain != Q4CenterDomainMode::Disk &&
       options.local.domain != Q4CenterDomainMode::Positive) ||
      options.local.max_depth > Q4LocalCell::max_depth || options.local.node_budget == 0)
    throw std::invalid_argument("mhgp9 gen global q34 requires valid local options");
  if ((options.q4_seed_cells.mode != Q4SeedCellMode::Individual &&
       options.q4_seed_cells.mode != Q4SeedCellMode::LiveOnly &&
       options.q4_seed_cells.mode != Q4SeedCellMode::Joined) ||
      options.q4_seed_cells.block_sites == 0 ||
      (options.q4_backend == WspdQ4Backend::Window30 &&
       options.q4_seed_cells.mode != Q4SeedCellMode::Individual))
    throw std::invalid_argument("mhgp9 gen invalid or incompatible q4 seed-cell options");
  if (options.dead_core && !options.dead_lanes)
    throw std::invalid_argument("mhgp9 gen dead-lane core requires the dead-lane certificate");
  // The traced cache is the exact singleton Affine search only.
  if (options.pair_witness_cache &&
      (options.witness_mode == WspdQ34WitnessMode::Disabled ||
       options.witness_bounds_mode != Q34WitnessBoundsMode::Affine))
    throw std::invalid_argument("mhgp9 gen pair witness cache requires the Affine pair filter");
  // The leaf census reads the atlas that only the consultation builds, and
  // only Local28 has one: refuse an option that would be silently inert.
  if (options.q3_leaf_census &&
      (!options.q3_atlas_consultation || options.q4_backend != WspdQ4Backend::Local28))
    throw std::invalid_argument("mhgp9 gen q3 leaf census requires q3 atlas consultation on Local28");
}

WspdQ34Result empty_result(const Q2CensusIndexPtr& index, unsigned kmax,
                          const WspdQ34Options& options) {
  WspdQ34Result result{};
  result.front.total_unordered_pairs = pair_count(index->cloud().points().size());
  const auto available = static_cast<std::uint8_t>((1U << std::min(kmax, 3U)) - 1);
  result.front.active_lane_mask = options.requested_lane_mask & available;
  return result;
}

void validate_completion(const WspdQ34Result& result, const WspdQ34Options& options) {
  // Whole-call identities, never promoted from a partly emitted traversal.
  const auto& witness = result.work.witness;
  auto q3 = result.work.q3_edges, q4 = result.work.q4_edges;
  auto expanded = result.work.expanded_pairs, covered = result.work.cover_builds;
  counter_add(q3, witness.rectangle_q3_pairs); counter_add(q3, witness.pair_q3_pairs);
  counter_add(q4, witness.rectangle_q4_pairs); counter_add(q4, witness.pair_q4_pairs);
  counter_add(q3, result.work.dead.q3_proved); counter_add(q4, result.work.dead.q4_proved);
  counter_add(q3, result.work.dead_core.q3_proved); counter_add(q4, result.work.dead_core.q4_proved);
  counter_add(covered, result.work.core_closed_edges);
  counter_add(expanded, witness.rectangle_pair_mass);
  counter_add(covered, witness.rejected_pairs);
  if (q3 != result.front.work.residual_pair_mass[1] ||
      q4 != result.front.work.residual_pair_mass[2] ||
      covered != result.work.expanded_pairs || expanded != witness.input_pair_mass ||
      result.work.input_rectangles != result.front.work.emitted_rectangles ||
      result.work.q3_emitted < result.work.lanes.emitted ||
      result.work.q3_emitted - result.work.lanes.emitted != result.work.q3.emitted ||
      result.work.q4_emitted < result.work.lanes4.emitted ||
      result.work.q4_emitted - result.work.lanes4.emitted != (options.q4_backend == WspdQ4Backend::Local28
          ? result.work.local.sweep.emitted : result.work.window.sweep.sweep.emitted))
    throw std::logic_error("mhgp9 gen global q34 completed ledger mismatch");
}

i64 distance_squared(Point3 a, Point3 b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(a[axis]) - b[axis];
    result += delta * delta;
  }
  return result;
}

std::pair<std::size_t, std::size_t> edge_key(std::size_t a, std::size_t b) {
  return {std::min(a, b), std::max(a, b)};
}

// A range of a-ranks of one residual rectangle (its b-range is whole), with
// the lane mask already filtered at the rectangle level.
struct RectangleTask {
  std::size_t a_node{}, b_node{}, a_first{}, a_last{};
  std::uint8_t mask{};
};
// Returns false when the queue refused the task (full): the caller expands
// that range inline, so every pair is still expanded exactly once.
using RectangleSplitter = std::function<bool(const RectangleTask&)>;

class Engine {
 public:
  Engine(Q2CensusIndexPtr index, unsigned k, WspdQ34Options options,
         const Q34SeedConsumer& consumer, RectangleSplitter splitter = {})
      : index_(std::move(index)), k_(k), options_(options), consumer_(consumer),
        sink_([this](const Q34SeedCandidate& candidate) { emit(candidate); }),
        splitter_(std::move(splitter)) {}
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(Engine&&) = delete;

  void rectangle(const WspdRectangle& rectangle) {
    const auto nodes = index_->spatial_nodes();
    const auto a = nodes[rectangle.a_node].range;
    const auto b = nodes[rectangle.b_node].range;
    counter_add(work.input_rectangles);
    const i128 product = static_cast<i128>(a.size()) * b.size();
    if (product > std::numeric_limits<u64>::max())
      throw std::overflow_error("mhgp9 gen q34 rectangle mass exceeds u64");
    const auto mass = static_cast<u64>(product);
    counter_add(work.witness.input_pair_mass, mass);
    auto mask = rectangle.lane_mask;
    if (options_.witness_mode == WspdQ34WitnessMode::RectanglePair) {
      const auto filtered = filter_q34_witnesses(*index_, nodes[rectangle.a_node].box,
          nodes[rectangle.b_node].box, static_cast<std::uint8_t>(k_), mask,
          work.witness.rectangles, options_.witness_bounds_mode, work.witness.rectangles_bounds);
      if ((mask & 2U) != 0 && (filtered & 2U) == 0)
        counter_add(work.witness.rectangle_q3_pairs, mass);
      if ((mask & 4U) != 0 && (filtered & 4U) == 0)
        counter_add(work.witness.rectangle_q4_pairs, mass);
      mask = filtered;
      if (mask == 0) {
        counter_add(work.witness.rejected_rectangles);
        counter_add(work.witness.rectangle_pair_mass, mass);
        return;
      }
    }
    // This is an explicit expansion of the certified residual WSPD products,
    // not an all-pairs fallback. The front's disjoint cover guarantees one
    // visit per unordered residual edge, even when both lanes survive.
    // With a splitter, a heavy rectangle is expanded by disjoint a-rank
    // ranges: the first one here, the others as published tasks; every
    // pair of the rectangle is still expanded exactly once.
    const auto grain = options_.parallel_task_pairs;
    if (splitter_ && grain != 0) {
      const auto rows = mass > grain ? std::max<std::size_t>(1, grain / b.size()) : a.size();
      if (rows < a.size()) counter_add(split_rectangles);
      for (auto start = a.first; start < a.last; start += rows) {
        const auto stop = std::min(start + rows, a.last);
        if (!splitter_(RectangleTask{rectangle.a_node, rectangle.b_node, start, stop, mask}))
          expand(start, stop, b.first, b.last, mask);
      }
      return;
    }
    expand(a.first, a.last, b.first, b.last, mask);
  }

  // Published range of an already filtered rectangle: no rectangle-level
  // counter or witness search is paid again.
  void rectangle_range(const RectangleTask& task) {
    const auto nodes = index_->spatial_nodes();
    const auto b = nodes[task.b_node].range;
    expand(task.a_first, task.a_last, b.first, b.last, task.mask);
  }

  // Batch path: an edge whose witness filter was decided by the batch call
  // (original IDs). Only the core/cover/certificate/q3/q4 work is paid here.
  void surviving_edge(std::size_t a, std::size_t b, std::uint8_t mask) { filtered_edge(a, b, mask); }
  // S3: the lanes a batch certificate call left open. That call already
  // counted the core, the cover and both certificates of this edge; the
  // cover is rebuilt here for the generation only, uncounted.
  void certified_edge(std::size_t a, std::size_t b, std::uint8_t mask) {
    if (mask == 0 || (mask & ~6U) != 0)
      throw std::logic_error("mhgp9 gen global q34 received an inactive certified edge");
    const auto cover = Q34EdgeCover::make(index_, {a, b});
    observe(cover);
    generate_lanes(cover, mask);
  }

  WspdQ34Work work{};
  u64 split_rectangles{};  // Not a geometric counter: orchestration only.

 private:
  void emit(const Q34SeedCandidate& candidate) {
    if (candidate.arity == 3) counter_add(work.q3_emitted);
    else if (candidate.arity == 4) counter_add(work.q4_emitted);
    else throw std::logic_error("mhgp9 gen global q34 backend emitted another arity");
    counter_add(work.payload_shell_ids, static_cast<u64>(candidate.shell_first.size()));
    counter_add(work.payload_shell_ids, static_cast<u64>(candidate.shell_second.size()));
    consumer_(candidate);
  }

  void expand(std::size_t a_first, std::size_t a_last, std::size_t b_first, std::size_t b_last,
              std::uint8_t mask) {
    const auto order = index_->spatial_order();
    for (auto ai = a_first; ai < a_last; ++ai)
      for (auto bi = b_first; bi < b_last; ++bi)
        edge(order[ai], order[bi], mask);
  }

  void observe(const Q34EdgeCoverPtr& cover, u64 q4_peak = 0) {
    const auto covered = static_cast<u64>(cover->retained_bytes());
    const auto q3 = shell_bytes(shell_.capacity());
    work.peak_cover_bytes = std::max(work.peak_cover_bytes, covered);
    work.q3.peak_shell_bytes = std::max(work.q3.peak_shell_bytes, q3);
    u64 live = covered;
    counter_add(live, q3);
    counter_add(live, q4_peak);
    work.peak_edge_buffer_bytes = std::max(work.peak_edge_buffer_bytes, live);
  }

  void edge(std::size_t a, std::size_t b, std::uint8_t mask) {
    if (mask == 0 || (mask & ~6U) != 0)
      throw std::logic_error("mhgp9 gen global q34 received an inactive rectangle");
    counter_add(work.expanded_pairs);
    if (options_.witness_mode != WspdQ34WitnessMode::Disabled) {
      const auto points = index_->cloud().points();
      std::uint8_t filtered = 0;
      if (options_.pair_witness_cache) {
        // Lanes proved by the nodes cached for this same endpoint a skip the
        // search; the remaining lanes get the full (traced) search.
        const std::uint8_t cached = cache_owner_ == a
            ? q34_cached_witness_rejections(*index_, points[a], points[b], static_cast<std::uint8_t>(k_), mask,
                                            cache_nodes_, work.witness_cache)
            : std::uint8_t{0};
        const auto open = static_cast<std::uint8_t>(mask & ~cached);
        if (open == 0) counter_add(work.witness.cache_rejected_pairs);
        else {
          filtered = filter_q34_witnesses(*index_, points[a], points[b], static_cast<std::uint8_t>(k_), open,
                                          work.witness.pairs, work.witness.pairs_bounds, trace_nodes_);
          cache_owner_ = a;
          cache_nodes_.swap(trace_nodes_);
        }
      } else {
        filtered = filter_q34_witnesses(*index_, singleton_box(points[a]),
            singleton_box(points[b]), static_cast<std::uint8_t>(k_), mask, work.witness.pairs,
            options_.witness_bounds_mode, work.witness.pairs_bounds);
      }
      if ((mask & 2U) != 0 && (filtered & 2U) == 0) counter_add(work.witness.pair_q3_pairs);
      if ((mask & 4U) != 0 && (filtered & 4U) == 0) counter_add(work.witness.pair_q4_pairs);
      mask = filtered;
      if (mask == 0) { counter_add(work.witness.rejected_pairs); return; }
    }
    filtered_edge(a, b, mask);
  }

  // The rest of an edge once its witness filter is decided (engine or batch
  // path): core, cover, certificate, then the q3/q4 lanes. `mask` holds the
  // surviving lanes; expanded_pairs and the witness ledger are not touched.
  void filtered_edge(std::size_t a, std::size_t b, std::uint8_t mask) {
    if (mask == 0 || (mask & ~6U) != 0)
      throw std::logic_error("mhgp9 gen global q34 received an inactive filtered edge");
    if (options_.dead_core) {
      // Diametral core first: every credit it gives, the cover gives too.
      // A lane it leaves open is rerun below on the full cover.
      const auto core = Q34EdgeCover::make_diametral(index_, {a, b});
      counter_add(work.core_builds);
      counter_add(work.core_sites, static_cast<u64>(core->site_count()));
      merge(work.core_cover, core->work());
      dead_.load(*core, work.dead_core);
      mask = static_cast<std::uint8_t>(mask & ~dead_.prove(k_, mask, work.dead_core));
      observe(core, static_cast<u64>(dead_.retained_bytes()));
      if (mask == 0) { counter_add(work.core_closed_edges); return; }
    }
    const auto cover = Q34EdgeCover::make(index_, {a, b});
    counter_add(work.cover_builds);
    counter_add(work.cover_sites, static_cast<u64>(cover->site_count()));
    work.max_cover_sites = std::max(work.max_cover_sites, static_cast<u64>(cover->site_count()));
    merge(work.cover, cover->work());
    observe(cover);
    if (options_.dead_lanes) {
      // A proved lane is empty on the exact path too (certificate in
      // lanes/q34_dead_lanes.hpp); an unproved lane runs unchanged.
      dead_.load(*cover, work.dead);
      mask = static_cast<std::uint8_t>(mask & ~dead_.prove(k_, mask, work.dead));
      observe(cover, static_cast<u64>(dead_.retained_bytes()));
      if (mask == 0) return;
    }
    generate_lanes(cover, mask);
  }

  // The q3/q4 generation of the lanes left open on this edge's cover.
  void generate_lanes(const Q34EdgeCoverPtr& cover, std::uint8_t mask) {
    const bool q3 = (mask & 2U) != 0, q4 = (mask & 4U) != 0;
    if (q3) counter_add(work.q3_edges);
    if (q4) counter_add(work.q4_edges);
    if (q3 && q4) counter_add(work.both_edges);
    // Explicit consultation: one atlas built here serves the q3 seed
    // certificates and, unchanged, the q4 sweep of the same edge.
    Q4LocalAtlasPtr atlas;
    if (q3 && q4 && options_.q3_atlas_consultation && k_ >= 3 &&
        options_.q4_backend == WspdQ4Backend::Local28) {
      atlas = Q4LocalAtlas::make(cover, k_, options_.local);
      counter_add(work.q3_atlas.edges_with_atlas);
    }
    if (q3) {
      const auto root = atlas ? atlas->root_certified_inside_count() : std::nullopt;
      if (root && *root >= k_ - 1) counter_add(work.q3_atlas.root_lane_skips);
      else q3_edge(cover, atlas.get());
      observe(cover);
    }
    if (q4) {
      if (options_.q4_backend == WspdQ4Backend::Local28) {
        const auto local = atlas
            ? run_q4_local_edge_candidates(atlas, sink_, options_.q4_seed_cells, work.q4_seed_cells)
            : run_q4_local_edge_candidates(cover, k_, options_.local, sink_,
                  options_.q4_seed_cells, work.q4_seed_cells);
        merge(work.local, local);
        observe(cover, local.peak_live_buffer_bytes);
      } else {
        const auto window = run_q4_window_edge_candidates(cover, k_, sink_);
        merge(work.window, window);
        observe(cover, window.peak_live_buffer_bytes);
      }
    }
  }

  void q3_seed(const Q34EdgeCoverPtr& cover, std::array<std::size_t, 3> ids, const Q4LocalAtlas* atlas) {
    auto& q3 = work.q3;
    const auto points = index_->cloud().points();
    const auto order = index_->spatial_order();
    Q4LocalCellCertificate cell;
    if (atlas) {
      // The certified count is a lower bound of the strict interior of the
      // seed ball (a, b, x): reaching K-1 rejects it exactly, before any
      // ball construction or census. No credit is transferred otherwise.
      counter_add(work.q3_atlas.locations);
      cell = atlas->certified_cell(atlas->geometry()->q3_center(ids[2]));
      if (cell.kind == Q4LocalCellCertificate::Kind::None) counter_add(work.q3_atlas.outside_domain);
      else if (cell.inside_count >= k_ - 1) {
        counter_add(work.q3_atlas.rejections);
        counter_add(q3.depth_rejections);
        return;
      } else if (cell.kind == Q4LocalCellCertificate::Kind::LowerBound) {
        counter_add(work.q3_atlas.lower_bound_fallbacks);
      }
    }
    counter_add(q3.ball_builds);
    const auto ball = ExactBall::make_q3({points[ids[0]], points[ids[1]], points[ids[2]]});
    if (!ball) throw std::logic_error("mhgp9 gen q3 owned acute seed lacks its exact ball");
    shell_.clear();
    std::size_t depth = 0, visited = 0;
    if (options_.q3_leaf_census && cell.kind == Q4LocalCellCertificate::Kind::ExactLeaf) {
      // The positive owned q3 ball lies inside the edge cover (R + |c-m| <=
      // sqrt(3)|ab|/2 < |ab|) and its centre lies in this closed leaf cell:
      // depth = certified count + frontier sites of negative power, and the
      // whole shell is the frontier sites of zero power (a, b, x included).
      counter_add(work.q3_atlas.leaf_censuses);
      depth = cell.inside_count;
      const auto nodes = index_->spatial_nodes();
      for (const auto node_id : cell.fragment->active_nodes()) {
        const auto range = nodes[node_id].range;
        for (auto rank = range.first; rank < range.last; ++rank) {
          const auto id = order[rank];
          counter_add(work.q3_atlas.leaf_point_tests);
          const auto power = ball->power(points[id]);
          if (power < 0) {
            if (++depth >= k_ - 1) {
              counter_add(work.q3_atlas.leaf_rejections);
              counter_add(q3.depth_rejections);
              return;
            }
          } else if (power == 0) shell_.push_back(id);
        }
      }
    } else if (options_.q3_census_mode == WspdQ3CensusMode::GlobalBoxes) {
      const auto census = census_q3_ball(*index_, *ball, k_ - 1, shell_, work.q3_blocks);
      if (!census.accepted) { counter_add(q3.depth_rejections); return; }
      depth = census.depth;
    } else {
    // No q4-family preparation, events, root comparisons or q3->q4 credit.
    // The same closed cover certifies the entire positive owned q3 ball.
    // ExactBall::power has its u16 i128 proof in that immutable primitive.
    for (const auto range : cover->ranges()) {
      counter_add(q3.census_range_visits);
      for (auto rank = range.first; rank < range.last; ++rank) {
        const auto id = order[rank];
        ++visited;
        counter_add(q3.census_point_tests);
        const auto power = ball->power(points[id]);
        if (power < 0) {
          counter_add(q3.census_inside_sites);
          if (++depth >= k_ - 1) {
            counter_add(q3.depth_rejections);
            counter_add(q3.early_unread_sites, static_cast<u64>(cover->site_count() - visited));
            return;
          }
        } else if (power == 0) {
          counter_add(q3.census_shell_sites);
          shell_.push_back(id);
        } else counter_add(q3.census_outside_sites);
      }
    }
    }
    std::sort(shell_.begin(), shell_.end(), [&](std::size_t a, std::size_t b) {
      counter_add(q3.shell_sort_comparisons);
      return a < b;
    });
    std::sort(ids.begin(), ids.end());
    counter_add(q3.shell_ids, static_cast<u64>(shell_.size()));
    counter_add(q3.emitted);
    sink_(Q34SeedCandidate{3, {ids[0], ids[1], ids[2],
        std::numeric_limits<std::size_t>::max()}, *ball, depth, shell_, {}});
  }

  void q3_edge(const Q34EdgeCoverPtr& cover, const Q4LocalAtlas* atlas) {
    auto& q3 = work.q3;
    counter_add(q3.edge_queries);
    const auto nodes = index_->spatial_nodes();
    const auto order = index_->spatial_order();
    const auto points = index_->cloud().points();
    const auto ids = cover->edge_ids();
    const auto a = points[ids[0]], b = points[ids[1]];
    const auto diameter = distance_squared(a, b);
    const auto owner = edge_key(ids[0], ids[1]);
    // Explicit q3-only port of tranche24's geometric seed-access reasoning.
    // The index boxes certify necessary lens and strict diameter-ball tests;
    // no previously accepted support nor precomputed list of faces is used.
    std::size_t cursor = 0;
    while (cursor < nodes.size()) {
      const auto& node = nodes[cursor];
      counter_add(q3.seed_node_visits);
      if (node.range.size() == 1) {
        counter_add(q3.seed_point_tests);
        const auto id = order[node.range.first];
        const auto ax = distance_squared(a, points[id]);
        const auto bx = distance_squared(b, points[id]);
        if (diameter + ax > bx && diameter + bx > ax && ax + bx > diameter) {
          counter_add(q3.acute_seeds);
          counter_add(q3.owner_tests);
          if (ax > diameter || (ax == diameter && edge_key(ids[0], id) < owner))
            counter_add(q3.owner_rejections);
          else {
            counter_add(q3.owner_tests);
            if (bx > diameter || (bx == diameter && edge_key(ids[1], id) < owner))
              counter_add(q3.owner_rejections);
            else {
              counter_add(q3.seeds);
              q3_seed(cover, {ids[0], ids[1], id}, atlas);
            }
          }
        }
        cursor = node.escape;
        continue;
      }
      counter_add(q3.seed_bound_tests);
      i64 min_a = 0, min_b = 0, max_sum = 0;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const i64 al = static_cast<i64>(node.box.low[axis]) - a[axis];
        const i64 ah = static_cast<i64>(node.box.high[axis]) - a[axis];
        const i64 bl = static_cast<i64>(node.box.low[axis]) - b[axis];
        const i64 bh = static_cast<i64>(node.box.high[axis]) - b[axis];
        const i64 na = al > 0 ? al : (ah < 0 ? ah : 0);
        const i64 nb = bl > 0 ? bl : (bh < 0 ? bh : 0);
        min_a += na * na;
        min_b += nb * nb;
        max_sum += std::max(al * al + bl * bl, ah * ah + bh * bh);
      }
      // At 18 bits each sum is <=6*262143^2<2^39. Ownership permits equality
      // in either distance; the third acute angle demands their sum>D.
      if (min_a > diameter || min_b > diameter || max_sum <= diameter) {
        counter_add(q3.seed_rejected_nodes);
        counter_add(q3.seed_rejected_sites, static_cast<u64>(node.range.size()));
        cursor = node.escape;
      } else {
        counter_add(q3.seed_split_nodes);
        cursor = node.left;
      }
    }
  }

  Q2CensusIndexPtr index_;
  unsigned k_;
  WspdQ34Options options_;
  const Q34SeedConsumer& consumer_;
  Q34SeedConsumer sink_;
  RectangleSplitter splitter_;
  std::vector<std::size_t> shell_;
  Q34DeadLaneProver dead_;
  // Witness-node cache of the pair filter, owned by the last searched a.
  std::size_t cache_owner_ = std::numeric_limits<std::size_t>::max();
  std::vector<Q34WitnessNode> cache_nodes_, trace_nodes_;
};

}  // namespace

WspdQ34Result run_wspd_q34_candidates(Q2CensusIndexPtr index, unsigned kmax,
    unsigned separation_s, WspdQ34Options options, const Q34SeedConsumer& consumer) {
  validate(index, kmax, separation_s, options, static_cast<bool>(consumer));
  WspdQ34Result result = empty_result(index, kmax, options);
  if (result.front.active_lane_mask == 0) return result;
  Engine engine(index, kmax, options, consumer);
  result.front = run_wspd_front(*index, kmax, separation_s, options.front_mode,
      [&](const WspdRectangle& rectangle) { engine.rectangle(rectangle); },
      options.requested_lane_mask);
  result.work = engine.work;
  validate_completion(result, options);
  return result;
}

WspdQ34ParallelResult run_wspd_q34_parallel(Q2CensusIndexPtr index, unsigned kmax,
    unsigned separation_s, WspdQ34Options options, std::size_t worker_count,
    const WspdQ34ParallelConsumer& consumer, std::size_t jobs_per_worker) {
  validate(index, kmax, separation_s, options, static_cast<bool>(consumer));
  if (worker_count == 0 || jobs_per_worker == 0)
    throw std::invalid_argument("mhgp9 gen parallel q34 requires positive workers/job granularity");
  if (worker_count > std::numeric_limits<std::size_t>::max() / jobs_per_worker)
    throw std::overflow_error("mhgp9 gen parallel q34 target job count overflow");
  const auto target_jobs = worker_count * jobs_per_worker;
  WspdQ34ParallelResult result{};
  result.pipeline = empty_result(index, kmax, options);
  auto& orchestration = result.parallel;
  orchestration.requested_workers = static_cast<u64>(worker_count);
  orchestration.target_jobs = static_cast<u64>(target_jobs);
  if (result.pipeline.front.active_lane_mask == 0) return result;

  const auto plan = make_wspd_front_jobs(index, kmax, separation_s, options.front_mode,
                                       target_jobs, options.requested_lane_mask, {}, options.jobs_by_mass);
  result.pipeline.front = plan->prefix_result();
  orchestration.prefix_product_visits = result.pipeline.front.work.product_visits;
  orchestration.jobs = static_cast<u64>(plan->job_count());
  orchestration.terminal_jobs = static_cast<u64>(plan->terminal_job_count());
  orchestration.job_storage_bytes = static_cast<u64>(plan->retained_bytes());
  const auto started = std::min(worker_count, plan->job_count());
  orchestration.started_workers = static_cast<u64>(started);
  // Claim order of the front jobs: plan order (with jobs_by_mass, the plan
  // is prepared largest product first and stored by decreasing mass).
  std::vector<std::size_t> job_order(plan->job_count());
  for (std::size_t j = 0; j < job_order.size(); ++j) job_order[j] = j;

  // Copies and state allocations finish before the first emission. Each
  // function object is private; shared references captured by its target
  // remain the caller's synchronization responsibility.
  const std::vector<WspdQ34ParallelConsumer> callbacks(started, consumer);
  orchestration.callback_storage_bytes = storage_bytes(callbacks.capacity(),
                                                       sizeof(WspdQ34ParallelConsumer));
  struct alignas(64) WorkerState {
    WspdFrontWork front{};
    WspdQ34Work work{};
    WspdQ34WorkerWork stats{};
    u64 tasks{}, split_rectangles{};
    WspdQ34WorkerTiming timing{};
  };
  const auto thread_cpu_ns = [] {
    timespec now{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) return u64{0};
    return static_cast<u64>(static_cast<u64>(now.tv_sec) * u64{1000000000} + static_cast<u64>(now.tv_nsec));
  };
  const auto wall_ns = [] {
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
  };
  std::vector<WorkerState> states(started);
  orchestration.worker_state_bytes = storage_bytes(states.capacity(), sizeof(WorkerState));
  // Rectangle-range tasks: one mutex-protected queue shared by the joined
  // team. A worker publishes ranges of a heavy rectangle while it expands the
  // first one; idle workers take pending ranges before the next front job.
  // Termination: no pending task, no unclaimed job and no busy worker.
  struct TaskQueue {
    std::mutex mutex;
    std::condition_variable wake;
    std::vector<RectangleTask> pending;
    std::size_t busy{}, next_job{}, job_count{};
    u64 published{}, consumed{}, task_pairs{}, peak_queue{}, waits{}, refused{};
    bool cancelled{};
  } queue;
  queue.job_count = plan->job_count();
  const auto sharing = started > 1 && options.parallel_task_pairs != 0;
  parallel_detail::run_joined_workers(started,
      [&](std::size_t slot, const std::atomic<bool>& cancel) {
        auto& state = states[slot];
        const auto wall_start = wall_ns(), cpu_start = thread_cpu_ns();
        u64 waited = 0, job_time = 0, longest_job = 0;
        const Q34SeedConsumer output = [&](const Q34SeedCandidate& candidate) {
          callbacks[slot](slot, candidate);
        };
        {
          const RectangleSplitter splitter = !sharing ? RectangleSplitter{} :
              [&](const RectangleTask& task) {
                const std::lock_guard<std::mutex> lock(queue.mutex);
                if (queue.pending.size() >= options.parallel_queue_capacity) {
                  counter_add(queue.refused);
                  return false;
                }
                queue.pending.push_back(task);
                counter_add(queue.published);
                queue.peak_queue = std::max<u64>(queue.peak_queue, queue.pending.size());
                queue.wake.notify_one();
                return true;
              };
          Engine engine(index, kmax, options, output, splitter);
          const WspdRectangleConsumer receiver = [&](const WspdRectangle& rectangle) {
            engine.rectangle(rectangle);
          };
          while (true) {
            std::optional<RectangleTask> task;
            std::optional<std::size_t> job;
            {
              std::unique_lock<std::mutex> lock(queue.mutex);
              while (queue.pending.empty() && queue.next_job == queue.job_count &&
                     queue.busy != 0 && !queue.cancelled) {
                counter_add(queue.waits);
                const auto before = wall_ns();
                queue.wake.wait(lock);
                counter_add(waited, wall_ns() - before);
              }
              if (queue.cancelled || cancel.load(std::memory_order_relaxed)) {
                queue.cancelled = true;
                queue.wake.notify_all();
                break;
              }
              if (!queue.pending.empty()) {
                task = queue.pending.back();
                queue.pending.pop_back();
                counter_add(queue.consumed);
              } else if (queue.next_job < queue.job_count) {
                job = queue.next_job++;
              } else {
                break;  // Nothing pending, nothing unclaimed, nobody busy.
              }
              ++queue.busy;
            }
            try {
              if (task) {
                const auto nodes = index->spatial_nodes();
                const auto pairs = static_cast<u64>(task->a_last - task->a_first) *
                    nodes[task->b_node].range.size();
                engine.rectangle_range(*task);
                counter_add(state.tasks);
                {
                  const std::lock_guard<std::mutex> lock(queue.mutex);
                  counter_add(queue.task_pairs, pairs);
                }
              } else {
                // Coarse job: an edge is never interrupted; run_job resumes an
                // unvisited subtree or replays a counted terminal's callback.
                const auto job_start = wall_ns();
                const auto part = plan->run_job(job_order[*job], receiver);
                const auto job_elapsed = wall_ns() - job_start;
                counter_add(job_time, job_elapsed);
                longest_job = std::max(longest_job, job_elapsed);
                parallel_detail::merge_work(state.front, part.work);
                counter_add(state.stats.jobs);
              }
            } catch (...) {
              const std::lock_guard<std::mutex> lock(queue.mutex);
              --queue.busy;
              queue.cancelled = true;
              queue.wake.notify_all();
              throw;
            }
            {
              const std::lock_guard<std::mutex> lock(queue.mutex);
              --queue.busy;
              if (queue.busy == 0 && queue.pending.empty() && queue.next_job == queue.job_count)
                queue.wake.notify_all();
            }
          }
          state.work = engine.work;
          state.split_rectangles = engine.split_rectangles;
        }  // Private engine buffers are released before this worker returns.
        state.timing = {wall_ns() - wall_start, thread_cpu_ns() - cpu_start, waited, job_time, longest_job};
        state.stats.front_products = state.front.product_visits;
        state.stats.input_rectangles = state.work.input_rectangles;
        state.stats.expanded_pairs = state.work.expanded_pairs;
        state.stats.q3_emitted = state.work.q3_emitted;
        state.stats.q4_emitted = state.work.q4_emitted;
        state.stats.peak_edge_buffer_bytes = state.work.peak_edge_buffer_bytes;
      });
  // All workers are joined before any reduction, including on launch or
  // callback failure. A failed call never publishes its partial counters.
  result.workers.reserve(states.size());
  result.worker_tasks.reserve(states.size());
  for (const auto& state : states) {
    parallel_detail::merge_work(result.pipeline.front.work, state.front);
    merge(result.pipeline.work, state.work);
    counter_add(orchestration.completed_jobs, state.stats.jobs);
    counter_add(orchestration.edge_buffer_bytes_sum, state.stats.peak_edge_buffer_bytes);
    result.workers.push_back(state.stats);
    result.worker_tasks.push_back(state.tasks);
    result.worker_timings.push_back(state.timing);
    counter_add(result.tasks.split_rectangles, state.split_rectangles);
  }
  result.tasks.published = queue.published;
  result.tasks.consumed = queue.consumed;
  result.tasks.task_pairs = queue.task_pairs;
  result.tasks.peak_queue = queue.peak_queue;
  result.tasks.waits = queue.waits;
  result.tasks.refused = queue.refused;
  if (orchestration.completed_jobs != orchestration.jobs)
    throw std::logic_error("mhgp9 gen parallel q34 lost its front job partition");
  if (result.tasks.published != result.tasks.consumed || !queue.pending.empty())
    throw std::logic_error("mhgp9 gen parallel q34 lost a published rectangle range");
  validate_completion(result.pipeline, options);
  return result;
}

namespace {

// Certificate part of Engine::filtered_edge (same order, same counters),
// without the capacity observations: the S3 CPU reference.
std::uint8_t certify_edge_cpu(const Q2CensusIndexPtr& index, Q34DeadLaneProver& prover, unsigned kmax,
                              bool dead_core, std::size_t a, std::size_t b, std::uint8_t mask, WspdQ34Work& work) {
  if (dead_core) {
    const auto core = Q34EdgeCover::make_diametral(index, {a, b});
    counter_add(work.core_builds);
    counter_add(work.core_sites, static_cast<u64>(core->site_count()));
    merge(work.core_cover, core->work());
    prover.load(*core, work.dead_core);
    mask = static_cast<std::uint8_t>(mask & ~prover.prove(kmax, mask, work.dead_core));
    if (mask == 0) {
      counter_add(work.core_closed_edges);
      return 0;
    }
  }
  const auto cover = Q34EdgeCover::make(index, {a, b});
  counter_add(work.cover_builds);
  counter_add(work.cover_sites, static_cast<u64>(cover->site_count()));
  work.max_cover_sites = std::max(work.max_cover_sites, static_cast<u64>(cover->site_count()));
  merge(work.cover, cover->work());
  prover.load(*cover, work.dead);
  return static_cast<std::uint8_t>(mask & ~prover.prove(kmax, mask, work.dead));
}

// Trust boundary of a certificate call: shapes, masks inside the survivors'
// lanes, and the lane/work identities that bind the reported counters to the
// returned masks. A consistent lie on a single decision is left to the
// engine/batch differentials and the independent judges.
void check_certificate_batch(const Q34CertificateBatch& c, std::span<const Q34SurvivingEdge> survivors,
                             bool dead_core) {
  if (c.masks.size() != survivors.size() || c.deferred.size() != survivors.size())
    throw std::logic_error("mhgp9 gen batched q34 certificate call returned a count different from its survivors");
  u64 decided = 0, q3_in = 0, q4_in = 0, q3_out = 0, q4_out = 0;
  for (std::size_t j = 0; j < survivors.size(); ++j) {
    const auto mask = survivors[j].mask, left = c.masks[j];
    if (c.deferred[j] > 1 || (c.deferred[j] == 1 && left != mask) || (left & ~mask) != 0)
      throw std::logic_error("mhgp9 gen batched q34 certificate call returned a widened or malformed mask");
    if (c.deferred[j] == 1) continue;
    ++decided;
    q3_in += (mask >> 1) & 1U;
    q4_in += (mask >> 2) & 1U;
    q3_out += (left >> 1) & 1U;
    q4_out += (left >> 2) & 1U;
  }
  const auto& core = c.dead_core;
  const auto& cover = c.dead;
  bool ok;
  if (dead_core) {
    ok = c.core_builds == decided && core.loads == decided && c.core_sites >= 2 * decided &&
         core.form_sites == c.core_sites - 2 * decided && c.core_cover.admitted_sites == c.core_sites &&
         core.q3_proved + core.q3_open == q3_in && core.q4_proved + core.q4_open == q4_in &&
         c.core_closed_edges <= decided && c.cover_builds == decided - c.core_closed_edges &&
         cover.q3_proved + cover.q3_open == core.q3_open && cover.q4_proved + cover.q4_open == core.q4_open;
  } else {
    ok = c.core_builds == 0 && c.core_sites == 0 && c.core_closed_edges == 0 &&
         c.core_cover == Q34EdgeCoverWork{} && core == Q34DeadLaneWork{} && c.cover_builds == decided &&
         cover.q3_proved + cover.q3_open == q3_in && cover.q4_proved + cover.q4_open == q4_in;
  }
  ok = ok && cover.loads == c.cover_builds && c.cover.admitted_sites == c.cover_sites &&
       c.cover_sites >= 2 * c.cover_builds && cover.form_sites == c.cover_sites - 2 * c.cover_builds &&
       cover.q3_open == q3_out && cover.q4_open == q4_out && c.max_cover_sites <= c.cover_sites &&
       (c.cover_builds == 0) == (c.max_cover_sites == 0);
  if (!ok) throw std::logic_error("mhgp9 gen batched q34 certificate call broke a lane or work identity");
}

}  // namespace

Q34CertificateBatch run_q34_certificate_batch_cpu(const Q2CensusIndexPtr& index, unsigned kmax, bool dead_core,
                                                  std::span<const Q34SurvivingEdge> survivors,
                                                  std::size_t workers) {
  if (!index) throw std::invalid_argument("mhgp9 gen q34 certificate batch requires an immutable index");
  if (workers == 0) throw std::invalid_argument("mhgp9 gen q34 certificate batch requires a positive worker count");
  if (kmax == 0 || kmax > 10) throw std::invalid_argument("mhgp9 gen q34 certificate batch requires K1..10");
  const auto order = index->spatial_order();
  Q34CertificateBatch out;
  out.backend = "cpu";
  out.masks.assign(survivors.size(), 0);
  out.deferred.assign(survivors.size(), 0);
  constexpr std::size_t grain = 256;
  const std::size_t blocks = (survivors.size() + grain - 1) / grain;
  struct alignas(64) Part {
    WspdQ34Work work{};
  };
  std::vector<Part> parts(std::min(workers, std::max<std::size_t>(blocks, 1)));
  std::atomic<std::size_t> next{0};
  parallel_detail::run_joined_workers(parts.size(), [&](std::size_t slot, const std::atomic<bool>& cancel) {
    auto& work = parts[slot].work;
    Q34DeadLaneProver prover;
    for (;;) {
      if (cancel.load(std::memory_order_relaxed)) return;
      const std::size_t block = next.fetch_add(1);
      if (block >= blocks) return;
      for (std::size_t i = block * grain; i < std::min(survivors.size(), (block + 1) * grain); ++i) {
        const auto& edge = survivors[i];
        if (edge.a_rank >= order.size() || edge.b_rank >= order.size() || edge.a_rank == edge.b_rank ||
            edge.mask == 0 || (edge.mask & ~6U) != 0)
          throw std::invalid_argument("mhgp9 gen q34 certificate batch received an edge outside the index/lanes");
        out.masks[i] = certify_edge_cpu(index, prover, kmax, dead_core, order[edge.a_rank], order[edge.b_rank],
                                        edge.mask, work);
      }
    }
  });
  WspdQ34Work total{};
  for (const auto& part : parts) merge(total, part.work);
  out.core_builds = total.core_builds;
  out.core_sites = total.core_sites;
  out.core_closed_edges = total.core_closed_edges;
  out.core_cover = total.core_cover;
  out.dead_core = total.dead_core;
  out.cover_builds = total.cover_builds;
  out.cover_sites = total.cover_sites;
  out.max_cover_sites = total.max_cover_sites;
  out.cover = total.cover;
  out.dead = total.dead;
  return out;
}

Q34CertificateFilter judge_certificate_filter(Q34CertificateFilter inner, std::size_t workers,
                                              Q34CertificateJudgeWork* work) {
  if (!inner || workers == 0)
    throw std::invalid_argument("mhgp9 gen certificate judge requires a call and a positive worker count");
  return [inner = std::move(inner), workers, work](const Q2CensusIndexPtr& index, unsigned kmax, bool dead_core,
                                                   std::span<const Q34SurvivingEdge> survivors) {
    auto answer = inner(index, kmax, dead_core, survivors);
    if (answer.masks.size() != survivors.size() || answer.deferred.size() != survivors.size())
      throw std::logic_error("mhgp9 gen certificate judge: the call returned a count different from its survivors");
    std::vector<Q34SurvivingEdge> decided;
    std::vector<std::size_t> where;
    for (std::size_t i = 0; i < survivors.size(); ++i)
      if (answer.deferred[i] == 0) {
        decided.push_back(survivors[i]);
        where.push_back(i);
      }
    const auto reference = run_q34_certificate_batch_cpu(index, kmax, dead_core, decided, workers);
    for (std::size_t j = 0; j < where.size(); ++j)
      if (answer.masks[where[j]] != reference.masks[j])
        throw std::logic_error("mhgp9 gen certificate judge: a decided mask differs from the CPU reference");
    if (answer.core_builds != reference.core_builds || answer.core_sites != reference.core_sites ||
        answer.core_closed_edges != reference.core_closed_edges || answer.core_cover != reference.core_cover ||
        answer.dead_core != reference.dead_core || answer.cover_builds != reference.cover_builds ||
        answer.cover_sites != reference.cover_sites || answer.max_cover_sites != reference.max_cover_sites ||
        answer.cover != reference.cover || answer.dead != reference.dead)
      throw std::logic_error("mhgp9 gen certificate judge: the decided work differs from the CPU reference");
    if (work != nullptr) {
      counter_add(work->judged, static_cast<u64>(decided.size()));
      counter_add(work->deferred, static_cast<u64>(survivors.size() - decided.size()));
    }
    return answer;
  };
}

namespace {

Q34LaneRecord lane_record(const Q34SeedCandidate& c) {
  Q34LaneRecord r{};
  r.key = c.ball.coefficients();
  for (std::size_t j = 0; j < 4; ++j)
    r.support[j] = j < c.arity ? static_cast<std::uint32_t>(c.support_ids[j]) : std::numeric_limits<std::uint32_t>::max();
  r.edge = std::numeric_limits<std::uint32_t>::max();
  r.depth = static_cast<std::uint32_t>(c.depth);
  r.shell = static_cast<std::uint32_t>(c.shell_first.size() + c.shell_second.size());
  r.arity = static_cast<std::uint8_t>(c.arity);
  for (const auto part : {c.shell_first, c.shell_second})
    for (const auto id : part) {
      const auto h = q34_shell_hash(static_cast<u64>(id));
      r.shell_sum += h;
      r.shell_xor ^= h;
    }
  return r;
}

// Record order of the judge: everything but the edge ordinal.
bool record_less(const Q34LaneRecord& a, const Q34LaneRecord& b) {
  if (a.key != b.key) return a.key < b.key;
  if (a.support != b.support) return a.support < b.support;
  if (a.depth != b.depth) return a.depth < b.depth;
  if (a.shell != b.shell) return a.shell < b.shell;
  if (a.shell_sum != b.shell_sum) return a.shell_sum < b.shell_sum;
  return a.shell_xor < b.shell_xor;
}

bool same_record(const Q34LaneRecord& a, const Q34LaneRecord& b) {
  return a.key == b.key && a.support == b.support && a.depth == b.depth && a.shell == b.shell &&
         a.arity == b.arity && a.shell_sum == b.shell_sum && a.shell_xor == b.shell_xor;
}

}  // namespace

std::vector<Q34LaneRecord> engine_q3_records(const Q2CensusIndexPtr& index, unsigned kmax,
                                             const WspdQ34Options& options, std::size_t a, std::size_t b,
                                             WspdQ3Work* q3) {
  if (!index || kmax < 2 || kmax > 10)
    throw std::invalid_argument("mhgp9 gen engine q3 records require an index and K2..10");
  std::vector<Q34LaneRecord> out;
  const Q34SeedConsumer capture = [&out](const Q34SeedCandidate& c) {
    if (c.arity != 3) throw std::logic_error("mhgp9 gen engine q3 lane emitted another arity");
    out.push_back(lane_record(c));
  };
  Engine engine(index, kmax, options, capture);
  engine.certified_edge(a, b, 2);
  if (q3 != nullptr) *q3 = engine.work.q3;
  return out;
}

std::vector<Q34LaneRecord> engine_q4_records(const Q2CensusIndexPtr& index, unsigned kmax,
                                             const WspdQ34Options& options, std::size_t a, std::size_t b) {
  if (!index || kmax < 3 || kmax > 10)
    throw std::invalid_argument("mhgp9 gen engine q4 records require an index and K3..10");
  std::vector<Q34LaneRecord> out;
  const Q34SeedConsumer capture = [&out](const Q34SeedCandidate& c) {
    if (c.arity != 4) throw std::logic_error("mhgp9 gen engine q4 lane emitted another arity");
    out.push_back(lane_record(c));
  };
  Engine engine(index, kmax, options, capture);
  engine.certified_edge(a, b, 4);
  return out;
}

void check_lanes_batch(const Q34LanesBatch& batch, const Q2CensusIndex& index, unsigned kmax,
                       std::span<const Q34SurvivingEdge> survivors, std::span<const std::uint8_t> asked) {
  const std::size_t n = survivors.size();
  if (asked.size() != n || batch.decided.size() != n || batch.record_begin.size() != n ||
      batch.record_count.size() != n)
    throw std::logic_error("mhgp9 gen batched q34 lanes call returned a count different from its survivors");
  const auto order = index.spatial_order();
  const auto& w = batch.work;
  const auto& w4 = batch.work4;
  u64 decided = 0, decided3 = 0, decided4 = 0, records3 = 0, records4 = 0, shells3 = 0, shells4 = 0;
  constexpr auto none = std::numeric_limits<std::uint32_t>::max();
  for (std::size_t j = 0; j < n; ++j) {
    const auto lanes = batch.decided[j];
    // A call decides all of an edge's asked lanes or none of them.
    if ((asked[j] & ~6U) != 0 || (lanes != 0 && lanes != asked[j]) || (lanes == 0 && batch.record_count[j] != 0) ||
        ((asked[j] & 4U) != 0 && kmax < 3))
      throw std::logic_error("mhgp9 gen batched q34 lanes call decided a lane it was not asked");
    if (lanes == 0) continue;
    ++decided;
    decided3 += (lanes >> 1) & 1U;
    decided4 += (lanes >> 2) & 1U;
    const u64 begin = batch.record_begin[j], count = batch.record_count[j];
    if (begin > batch.records.size() || count > batch.records.size() - begin)
      throw std::logic_error("mhgp9 gen batched q34 lanes call returned a slice outside its records");
    const auto ida = static_cast<std::uint32_t>(order[survivors[j].a_rank]);
    const auto idb = static_cast<std::uint32_t>(order[survivors[j].b_rank]);
    for (u64 r = begin; r < begin + count; ++r) {
      const auto& record = batch.records[r];
      const auto& s = record.support;
      const unsigned arity = record.arity;
      const bool shape = arity == 3 ? (lanes & 2U) != 0 && s[3] == none && s[0] < s[1] && s[1] < s[2] &&
                                          record.depth < kmax - 1 && record.shell >= 3
                                    : arity == 4 && (lanes & 4U) != 0 && s[0] < s[1] && s[1] < s[2] &&
                                          s[2] < s[3] && s[3] != none && record.depth < kmax - 2 &&
                                          record.shell >= 4;
      bool has_a = false, has_b = false;
      for (unsigned k = 0; k < arity && k < 4; ++k) {
        has_a = has_a || s[k] == ida;
        has_b = has_b || s[k] == idb;
      }
      if (record.edge != j || !shape || !has_a || !has_b || record.key[0] <= 0)
        throw std::logic_error("mhgp9 gen batched q34 lanes call returned a malformed or misplaced record");
      // Auditor B (before R21): every support ID lies in the cloud before the
      // chain dereferences it (the supports are sorted: the last is the max).
      if (s[arity - 1] >= order.size())
        throw std::logic_error("mhgp9 gen batched q34 lanes call returned a support outside the cloud");
      if (arity == 3) {
        ++records3;
        counter_add(shells3, static_cast<u64>(record.shell));
      } else {
        ++records4;
        counter_add(shells4, static_cast<u64>(record.shell));
      }
    }
  }
  // Every record lies in the slice of its own edge (edge field), and the
  // slices sum to the records: they partition the records exactly.
  const bool ok = records3 + records4 == batch.records.size() && w.edges == decided && w.q3_edges == decided3 &&
      w.emitted == records3 && w.shell_ids == shells3 && w.census_seeds == w.depth_rejections + w.emitted &&
      w.census_seeds <= w.seeds && w.acute_sites == w.owner_rejections + w.seeds &&
      w.seed_tests == w.cover_sites && w.cover.admitted_sites == w.cover_sites && w.cover_sites >= 2 * w.edges &&
      // L11: the pruned sites are classified cover sites, never tested as seeds.
      w.pruned_sites <= w.seed_tests && w.acute_sites + w.pruned_sites <= w.seed_tests &&
      w.census_point_tests == w.census_inside_sites + w.census_shell_sites + w.census_outside_sites &&
      w.census_shell_sites >= w.shell_ids && w.max_cover_sites <= w.cover_sites &&
      (w.edges == 0) == (w.max_cover_sites == 0) &&
      // S4b: seeds = certified + survivors; groups = rejected + without a valid presentation + emitted.
      w4.edges == decided4 && w4.emitted == records4 && w4.shell_ids == shells4 && w4.seeds <= w.seeds &&
      w4.seeds == w4.certified + w4.survivors && w4.certified_chunk1 <= w4.certified &&
      w4.groups == w4.depth_rejected_groups + w4.groups_without_valid + w4.emitted &&
      w4.multi_emission_seeds <= w4.emitting_seeds && w4.emitting_seeds <= w4.emitted &&
      w4.emitting_seeds <= w4.survivors && w4.max_buffered <= w4.buffered_events &&
      (decided4 != 0 || w4 == Q34Lanes4Work{});
  if (!ok) throw std::logic_error("mhgp9 gen batched q34 lanes call broke a record or work identity");
}

Q34LanesFilter judge_lanes_filter(Q34LanesFilter inner, WspdQ34Options options, std::size_t workers,
                                  Q34LanesJudgeWork* work) {
  if (!inner || workers == 0)
    throw std::invalid_argument("mhgp9 gen lanes judge requires a call and a positive worker count");
  return [inner = std::move(inner), options, workers, work](const Q2CensusIndexPtr& index, unsigned kmax,
                                                            std::span<const Q34SurvivingEdge> survivors,
                                                            std::span<const std::uint8_t> asked) {
    auto answer = inner(index, kmax, survivors, asked);
    check_lanes_batch(answer, *index, kmax, survivors, asked);
    const auto order = index->spatial_order();
    std::vector<std::size_t> decided;
    for (std::size_t j = 0; j < survivors.size(); ++j)
      if (answer.decided[j] != 0) decided.push_back(j);
    struct alignas(64) Part {
      u64 seeds{}, emitted{}, emitted4{};
    };
    std::vector<Part> parts(std::min(workers, std::max<std::size_t>(decided.size(), 1)));
    std::atomic<std::size_t> next{0};
    constexpr std::size_t grain = 64;
    parallel_detail::run_joined_workers(parts.size(), [&](std::size_t slot, const std::atomic<bool>& cancel) {
      std::vector<Q34LaneRecord> mine;
      for (;;) {
        if (cancel.load(std::memory_order_relaxed)) return;
        const std::size_t begin = next.fetch_add(grain);
        if (begin >= decided.size()) return;
        for (std::size_t i = begin; i < std::min(decided.size(), begin + grain); ++i) {
          const auto j = decided[i];
          const auto a = order[survivors[j].a_rank], b = order[survivors[j].b_rank];
          for (const unsigned arity : {3U, 4U}) {
            if ((answer.decided[j] & (arity == 3 ? 2U : 4U)) == 0) continue;
            std::vector<Q34LaneRecord> reference;
            if (arity == 3) {
              WspdQ3Work q3{};
              reference = engine_q3_records(index, kmax, options, a, b, &q3);
              counter_add(parts[slot].seeds, q3.seeds);
              counter_add(parts[slot].emitted, q3.emitted);
            } else {
              reference = engine_q4_records(index, kmax, options, a, b);
              counter_add(parts[slot].emitted4, static_cast<u64>(reference.size()));
            }
            mine.clear();
            for (std::size_t r = answer.record_begin[j]; r < answer.record_begin[j] + answer.record_count[j]; ++r)
              if (answer.records[r].arity == arity) mine.push_back(answer.records[r]);
            std::sort(mine.begin(), mine.end(), record_less);
            std::sort(reference.begin(), reference.end(), record_less);
            if (mine.size() != reference.size() ||
                !std::equal(mine.begin(), mine.end(), reference.begin(), same_record))
              throw std::logic_error("mhgp9 gen lanes judge: a decided edge's balls differ from the engine");
          }
        }
      }
    });
    u64 seeds = 0, emitted = 0, emitted4 = 0;
    for (const auto& part : parts) {
      counter_add(seeds, part.seeds);
      counter_add(emitted, part.emitted);
      counter_add(emitted4, part.emitted4);
    }
    if (seeds != answer.work.census_seeds || emitted != answer.work.emitted || emitted4 != answer.work4.emitted)
      throw std::logic_error("mhgp9 gen lanes judge: the decided seeds or emissions differ from the engine");
    if (work != nullptr) {
      counter_add(work->judged, static_cast<u64>(decided.size()));
      u64 asked_count = 0;
      for (const auto lane : asked) asked_count += lane != 0 ? 1U : 0U;
      counter_add(work->deferred, asked_count - static_cast<u64>(decided.size()));
      counter_add(work->records, static_cast<u64>(answer.records.size()));
    }
    return answer;
  };
}

Q34FilterBatch run_q34_filter_batch_cpu(const Q2CensusIndex& index, unsigned kmax,
                                        std::span<const WspdRectangle> rectangles, std::size_t workers) {
  if (workers == 0) throw std::invalid_argument("mhgp9 gen q34 batch filter requires a positive worker count");
  if (kmax == 0 || kmax > 10) throw std::invalid_argument("mhgp9 gen q34 batch filter requires K1..10");
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  if (order.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::overflow_error("mhgp9 gen q34 batch filter ranks exceed u32");
  Q34FilterBatch out;
  out.backend = "cpu";
  out.rectangle_masks.assign(rectangles.size(), 0);
  // Contiguous blocks, concatenated in block order: rectangle order, and
  // row-major pairs inside each rectangle, whatever the thread count.
  constexpr std::size_t grain = 256;
  const std::size_t blocks = (rectangles.size() + grain - 1) / grain;
  struct Part {
    std::vector<Q34SurvivingEdge> survivors;
    u64 expanded{}, q3{}, q4{}, rectangle_visits{}, pair_visits{};
  };
  std::vector<Part> parts(blocks);
  std::atomic<std::size_t> next{0};
  const auto k = static_cast<std::uint8_t>(kmax);
  parallel_detail::run_joined_workers(std::min(workers, std::max<std::size_t>(blocks, 1)),
      [&](std::size_t, const std::atomic<bool>& cancel) {
        for (;;) {
          if (cancel.load(std::memory_order_relaxed)) return;
          const std::size_t block = next.fetch_add(1);
          if (block >= blocks) return;
          auto& part = parts[block];
          Q34WitnessSearchWork rectangle_work, pair_work;
          Q34WitnessBoundsWork rectangle_bounds, pair_bounds;
          for (std::size_t i = block * grain; i < std::min(rectangles.size(), (block + 1) * grain); ++i) {
            const auto& r = rectangles[i];
            if (r.a_node >= nodes.size() || r.b_node >= nodes.size() || r.lane_mask == 0 || (r.lane_mask & ~6U) != 0)
              throw std::invalid_argument("mhgp9 gen q34 batch filter received a rectangle outside the index/lanes");
            const auto mask = filter_q34_witnesses(index, nodes[r.a_node].box, nodes[r.b_node].box, k, r.lane_mask,
                                                   rectangle_work, Q34WitnessBoundsMode::Affine, rectangle_bounds);
            out.rectangle_masks[i] = mask;
            if (mask == 0) continue;
            const auto a = nodes[r.a_node].range, b = nodes[r.b_node].range;
            counter_add(part.expanded, static_cast<u64>(a.size()) * b.size());
            for (auto ai = a.first; ai < a.last; ++ai)
              for (auto bi = b.first; bi < b.last; ++bi) {
                const auto pair = filter_q34_witnesses(index, singleton_box(points[order[ai]]),
                    singleton_box(points[order[bi]]), k, mask, pair_work, Q34WitnessBoundsMode::Affine,
                    pair_bounds);
                if ((mask & 2U) != 0 && (pair & 2U) == 0) counter_add(part.q3);
                if ((mask & 4U) != 0 && (pair & 4U) == 0) counter_add(part.q4);
                if (pair != 0)
                  part.survivors.push_back({static_cast<std::uint32_t>(ai), static_cast<std::uint32_t>(bi), pair});
              }
          }
          part.rectangle_visits = rectangle_work.node_visits;
          part.pair_visits = pair_work.node_visits;
        }
      });
  std::size_t total = 0;
  for (const auto& part : parts) total += part.survivors.size();
  out.survivors.reserve(total);
  for (auto& part : parts) {
    out.survivors.insert(out.survivors.end(), part.survivors.begin(), part.survivors.end());
    counter_add(out.expanded_pairs, part.expanded);
    counter_add(out.pair_q3_rejected, part.q3);
    counter_add(out.pair_q4_rejected, part.q4);
    counter_add(out.rectangle_visits, part.rectangle_visits);
    counter_add(out.pair_visits, part.pair_visits);
  }
  return out;
}

WspdQ34ParallelResult run_wspd_q34_batched(Q2CensusIndexPtr index, unsigned kmax,
    unsigned separation_s, WspdQ34Options options, std::size_t worker_count,
    const WspdQ34ParallelConsumer& consumer, std::size_t jobs_per_worker,
    const Q34BatchFilter& filter, WspdQ34BatchTiming* timing, const Q34CertificateFilter* certificates,
    const Q34LanesStage* lanes, const std::function<void()>* after_front) {
  validate(index, kmax, separation_s, options, static_cast<bool>(consumer));
  if (!filter) throw std::invalid_argument("mhgp9 gen batched q34 requires a batch filter");
  const bool certify = certificates != nullptr && static_cast<bool>(*certificates);
  if (certify && !options.dead_lanes)
    throw std::invalid_argument("mhgp9 gen batched q34 certificates require the dead-lane certificate");
  const bool staged = lanes != nullptr && static_cast<bool>(lanes->filter);
  if (staged && (!certify || !lanes->sink || (lanes->lanes != 2 && lanes->lanes != 6)))
    throw std::invalid_argument("mhgp9 gen batched q34 lanes require the certificate call, a record sink and lanes 2 or 6");
  if (options.witness_mode != WspdQ34WitnessMode::RectanglePair ||
      options.witness_bounds_mode != Q34WitnessBoundsMode::Affine)
    throw std::invalid_argument("mhgp9 gen batched q34 implements RectanglePair witnesses with Affine bounds only");
  if (worker_count == 0 || jobs_per_worker == 0)
    throw std::invalid_argument("mhgp9 gen batched q34 requires positive workers/job granularity");
  if (worker_count > std::numeric_limits<std::size_t>::max() / jobs_per_worker)
    throw std::overflow_error("mhgp9 gen batched q34 target job count overflow");
  const auto target_jobs = worker_count * jobs_per_worker;
  WspdQ34ParallelResult result{};
  result.pipeline = empty_result(index, kmax, options);
  auto& orchestration = result.parallel;
  orchestration.requested_workers = static_cast<u64>(worker_count);
  orchestration.target_jobs = static_cast<u64>(target_jobs);
  WspdQ34BatchTiming local_timing;
  if (result.pipeline.front.active_lane_mask == 0) {
    if (timing != nullptr) *timing = local_timing;
    return result;
  }
  const auto wall_ns = [] {
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
  };
  const auto thread_cpu_ns = [] {
    timespec now{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) return u64{0};
    return static_cast<u64>(static_cast<u64>(now.tv_sec) * u64{1000000000} + static_cast<u64>(now.tv_nsec));
  };

  // ---- Phase 1: the front jobs only collect their residual rectangles.
  auto phase = wall_ns();
  const auto plan = make_wspd_front_jobs(index, kmax, separation_s, options.front_mode,
                                       target_jobs, options.requested_lane_mask, {}, options.jobs_by_mass);
  result.pipeline.front = plan->prefix_result();
  orchestration.prefix_product_visits = result.pipeline.front.work.product_visits;
  orchestration.jobs = static_cast<u64>(plan->job_count());
  orchestration.terminal_jobs = static_cast<u64>(plan->terminal_job_count());
  orchestration.job_storage_bytes = static_cast<u64>(plan->retained_bytes());
  const auto started = std::min(worker_count, plan->job_count());
  orchestration.started_workers = static_cast<u64>(started);
  const std::vector<WspdQ34ParallelConsumer> callbacks(started, consumer);
  orchestration.callback_storage_bytes = storage_bytes(callbacks.capacity(), sizeof(WspdQ34ParallelConsumer));
  struct alignas(64) WorkerState {
    WspdFrontWork front{};
    WspdQ34Work work{};
    WspdQ34WorkerWork stats{};
    WspdQ34WorkerTiming timing{};
  };
  std::vector<WorkerState> states(started);
  orchestration.worker_state_bytes = storage_bytes(states.capacity(), sizeof(WorkerState));
  std::vector<std::vector<WspdRectangle>> by_job(plan->job_count());
  {
    std::atomic<std::size_t> next{0};
    parallel_detail::run_joined_workers(started, [&](std::size_t slot, const std::atomic<bool>& cancel) {
      auto& state = states[slot];
      const auto wall_start = wall_ns(), cpu_start = thread_cpu_ns();
      for (;;) {
        if (cancel.load(std::memory_order_relaxed)) break;
        const std::size_t job = next.fetch_add(1);
        if (job >= by_job.size()) break;
        auto& sink = by_job[job];
        const auto job_start = wall_ns();
        const auto part = plan->run_job(job, [&](const WspdRectangle& rectangle) { sink.push_back(rectangle); });
        const auto elapsed = wall_ns() - job_start;
        counter_add(state.timing.job_ns, elapsed);
        state.timing.max_job_ns = std::max(state.timing.max_job_ns, elapsed);
        parallel_detail::merge_work(state.front, part.work);
        counter_add(state.stats.jobs);
      }
      counter_add(state.timing.wall_ns, wall_ns() - wall_start);
      counter_add(state.timing.cpu_ns, thread_cpu_ns() - cpu_start);
    });
  }
  std::vector<WspdRectangle> rectangles;
  {
    std::size_t total = 0;
    for (const auto& job : by_job) total += job.size();
    rectangles.reserve(total);
    for (auto& job : by_job) {
      rectangles.insert(rectangles.end(), job.begin(), job.end());
      std::vector<WspdRectangle>().swap(job);
    }
  }
  local_timing.front_ns = wall_ns() - phase;
  local_timing.rectangles = rectangles.size();
  // v24: the caller's work that may run during the device calls (q2).
  if (after_front != nullptr && *after_front) (*after_front)();

  // ---- Phase 2: one batch call decides every rectangle and every pair.
  phase = wall_ns();
  auto batch = filter(*index, kmax, rectangles);
  local_timing.filter_ns = wall_ns() - phase;
  local_timing.backend = batch.backend;
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  if (batch.rectangle_masks.size() != rectangles.size())
    throw std::logic_error("mhgp9 gen batched q34 filter returned a mask count different from its rectangles");
  WspdQ34Work filter_work{};
  auto& witness = filter_work.witness;
  u64 expanded = 0;
  for (std::size_t i = 0; i < rectangles.size(); ++i) {
    const auto& r = rectangles[i];
    const auto mask = batch.rectangle_masks[i];
    if ((mask & ~r.lane_mask) != 0)
      throw std::logic_error("mhgp9 gen batched q34 filter opened a lane its rectangle did not carry");
    const i128 product = static_cast<i128>(nodes[r.a_node].range.size()) * nodes[r.b_node].range.size();
    if (product > std::numeric_limits<u64>::max())
      throw std::overflow_error("mhgp9 gen batched q34 rectangle mass exceeds u64");
    const auto mass = static_cast<u64>(product);
    counter_add(filter_work.input_rectangles);
    counter_add(witness.input_pair_mass, mass);
    if ((r.lane_mask & 2U) != 0 && (mask & 2U) == 0) counter_add(witness.rectangle_q3_pairs, mass);
    if ((r.lane_mask & 4U) != 0 && (mask & 4U) == 0) counter_add(witness.rectangle_q4_pairs, mass);
    if (mask == 0) {
      counter_add(witness.rejected_rectangles);
      counter_add(witness.rectangle_pair_mass, mass);
    } else {
      counter_add(expanded, mass);
    }
  }
  if (batch.expanded_pairs != expanded || batch.survivors.size() > expanded)
    throw std::logic_error("mhgp9 gen batched q34 filter pair count differs from its surviving rectangles");
  // Structure of the survivors (trust boundary of the batch call, auditors
  // A/B/C): one cursor over the surviving rectangles in order. Every
  // survivor lies in exactly the current rectangle's A x B (the WSPD cover is
  // disjoint), in strictly increasing row-major order (no duplicate), with a
  // mask inside its rectangle's surviving lanes; all survivors are consumed.
  // A dropped pair is not visible here: the engine/batch differential and
  // the independent judges cover that.
  {
    std::size_t cursor = 0;
    for (std::size_t i = 0; i < rectangles.size() && cursor < batch.survivors.size(); ++i) {
      const auto mask = batch.rectangle_masks[i];
      if (mask == 0) continue;
      const auto a = nodes[rectangles[i].a_node].range, b = nodes[rectangles[i].b_node].range;
      std::size_t previous = std::numeric_limits<std::size_t>::max();
      while (cursor < batch.survivors.size()) {
        const auto& edge = batch.survivors[cursor];
        if (edge.a_rank < a.first || edge.a_rank >= a.last || edge.b_rank < b.first || edge.b_rank >= b.last) break;
        const std::size_t ordinal = (edge.a_rank - a.first) * b.size() + (edge.b_rank - b.first);
        if ((previous != std::numeric_limits<std::size_t>::max() && ordinal <= previous) || edge.mask == 0 ||
            (edge.mask & ~mask) != 0)
          throw std::logic_error("mhgp9 gen batched q34 filter returned a duplicate, unordered or widened pair");
        previous = ordinal;
        ++cursor;
      }
    }
    if (cursor != batch.survivors.size())
      throw std::logic_error("mhgp9 gen batched q34 filter returned a pair outside its surviving rectangles");
  }
  filter_work.expanded_pairs = expanded;
  witness.rejected_pairs = expanded - batch.survivors.size();
  witness.pair_q3_pairs = batch.pair_q3_rejected;
  witness.pair_q4_pairs = batch.pair_q4_rejected;
  witness.rectangles.queries = static_cast<u64>(rectangles.size());
  witness.rectangles.node_visits = batch.rectangle_visits;
  witness.pairs.queries = expanded;
  witness.pairs.node_visits = batch.pair_visits;
  local_timing.survivors = batch.survivors.size();
  std::vector<WspdRectangle>().swap(rectangles);

  // ---- Phase 2b (S3): one call decides the certificates of the survivors.
  Q34CertificateBatch certified;
  if (certify) {
    phase = wall_ns();
    certified = (*certificates)(index, kmax, options.dead_core, batch.survivors);
    local_timing.certificate_ns = wall_ns() - phase;
    local_timing.certificate_backend = certified.backend;
    check_certificate_batch(certified, batch.survivors, options.dead_core);
    for (const auto flag : certified.deferred) local_timing.deferred += flag;
    WspdQ34Work w{};
    w.core_builds = certified.core_builds;
    w.core_sites = certified.core_sites;
    w.core_closed_edges = certified.core_closed_edges;
    w.core_cover = certified.core_cover;
    w.dead_core = certified.dead_core;
    w.cover_builds = certified.cover_builds;
    w.cover_sites = certified.cover_sites;
    w.max_cover_sites = certified.max_cover_sites;
    w.cover = certified.cover;
    w.dead = certified.dead;
    merge(filter_work, w);
  }

  // ---- Phase 2c (S4a): one call generates the q3 lane of the certified
  // survivors (`asked`), on its own thread when concurrent: the workers run
  // the other lanes meanwhile. The thread is joined on every path.
  const auto& survivors = batch.survivors;
  std::vector<std::uint8_t> asked;
  Q34LanesBatch lanes_batch;
  std::exception_ptr lanes_failure;
  u64 lanes_ns = 0;
  std::thread lanes_thread;
  struct Joiner {
    std::thread& thread;
    ~Joiner() {
      if (thread.joinable()) thread.join();
    }
  } joiner{lanes_thread};
  if (staged) {
    asked.assign(survivors.size(), 0);
    for (std::size_t j = 0; j < survivors.size(); ++j)
      if (certified.deferred[j] == 0) asked[j] = static_cast<std::uint8_t>(certified.masks[j] & lanes->lanes);
    const auto call = [&] {
      const auto start = wall_ns();
      try {
        lanes_batch = lanes->filter(index, kmax, survivors, asked);
      } catch (...) {
        lanes_failure = std::current_exception();
      }
      lanes_ns = wall_ns() - start;
    };
    if (lanes->concurrent) {
      lanes_thread = std::thread(call);
    } else {
      call();
      if (lanes_failure) std::rethrow_exception(lanes_failure);
    }
  }

  // ---- Phase 3: the workers run the rest of every surviving edge (without
  // the asked q3 lanes).
  phase = wall_ns();
  std::atomic<u64> rebuilt{0};
  {
    std::atomic<std::size_t> next{0};
    constexpr std::size_t grain = 64;
    parallel_detail::run_joined_workers(started, [&](std::size_t slot, const std::atomic<bool>& cancel) {
      auto& state = states[slot];
      const auto wall_start = wall_ns(), cpu_start = thread_cpu_ns();
      const Q34SeedConsumer output = [&](const Q34SeedCandidate& candidate) { callbacks[slot](slot, candidate); };
      u64 rebuilt_here = 0;
      {
        Engine engine(index, kmax, options, output);
        for (;;) {
          if (cancel.load(std::memory_order_relaxed)) break;
          const std::size_t begin = next.fetch_add(grain);
          if (begin >= survivors.size()) break;
          for (std::size_t j = begin; j < std::min(survivors.size(), begin + grain); ++j) {
            const auto a = order[survivors[j].a_rank], b = order[survivors[j].b_rank];
            if (!certify || certified.deferred[j] != 0) {
              engine.surviving_edge(a, b, survivors[j].mask);
              continue;
            }
            const auto open = static_cast<std::uint8_t>(certified.masks[j] & ~(staged ? asked[j] : 0U));
            if (open != 0) {
              engine.certified_edge(a, b, open);
              ++rebuilt_here;
            }
          }
        }
        state.work = engine.work;
      }
      rebuilt.fetch_add(rebuilt_here, std::memory_order_relaxed);
      counter_add(state.timing.wall_ns, wall_ns() - wall_start);
      counter_add(state.timing.cpu_ns, thread_cpu_ns() - cpu_start);
    });
  }
  local_timing.edges_ns = wall_ns() - phase;

  // ---- Phase 3b (S4a): join the call, check it, then the workers run the
  // q3 lanes it did not decide and hand its records to the sink.
  if (staged) {
    phase = wall_ns();
    if (lanes_thread.joinable()) lanes_thread.join();
    local_timing.lanes_wait_ns = wall_ns() - phase;
    if (lanes_failure) std::rethrow_exception(lanes_failure);
    check_lanes_batch(lanes_batch, *index, kmax, survivors, asked);
    local_timing.lanes_ns = lanes_ns;
    local_timing.lanes_backend = lanes_batch.backend;
    std::vector<std::size_t> tail;
    WspdQ34Work lanes_work{};
    for (std::size_t j = 0; j < survivors.size(); ++j) {
      if (asked[j] == 0) continue;
      ++local_timing.lanes_asked;
      const bool decided = lanes_batch.decided[j] != 0;
      // An edge with both lanes open is counted in both_edges here unless the
      // engine runs them together (both asked and deferred: the tail runs 6).
      if (certified.masks[j] == 6U && !(asked[j] == 6U && !decided)) counter_add(lanes_work.both_edges);
      if (!decided) {
        tail.push_back(j);
        continue;
      }
      ++local_timing.lanes_decided;
      if ((asked[j] & 2U) != 0) counter_add(lanes_work.q3_edges);
      if ((asked[j] & 4U) != 0) counter_add(lanes_work.q4_edges);
    }
    local_timing.lanes_deferred = tail.size();
    local_timing.lanes_records = lanes_batch.records.size();
    lanes_work.lanes = lanes_batch.work;
    lanes_work.lanes4 = lanes_batch.work4;
    lanes_work.q3_emitted = lanes_batch.work.emitted;
    lanes_work.q4_emitted = lanes_batch.work4.emitted;
    lanes_work.payload_shell_ids = lanes_batch.work.shell_ids;
    counter_add(lanes_work.payload_shell_ids, lanes_batch.work4.shell_ids);
    merge(filter_work, lanes_work);
    phase = wall_ns();
    std::atomic<std::size_t> next_edge{0}, next_chunk{0};
    constexpr std::size_t grain = 16, chunk = 4096;
    const std::size_t chunks = (lanes_batch.records.size() + chunk - 1) / chunk;
    const std::span<const Q34LaneRecord> records(lanes_batch.records);
    parallel_detail::run_joined_workers(started, [&](std::size_t slot, const std::atomic<bool>& cancel) {
      auto& state = states[slot];
      const auto wall_start = wall_ns(), cpu_start = thread_cpu_ns();
      const Q34SeedConsumer output = [&](const Q34SeedCandidate& candidate) { callbacks[slot](slot, candidate); };
      u64 rebuilt_here = 0;
      {
        Engine engine(index, kmax, options, output);
        for (;;) {
          if (cancel.load(std::memory_order_relaxed)) break;
          const std::size_t begin = next_edge.fetch_add(grain);
          if (begin >= tail.size()) break;
          for (std::size_t i = begin; i < std::min(tail.size(), begin + grain); ++i) {
            const auto j = tail[i];
            engine.certified_edge(order[survivors[j].a_rank], order[survivors[j].b_rank], asked[j]);
            ++rebuilt_here;
          }
        }
        merge(state.work, engine.work);
      }
      for (;;) {
        if (cancel.load(std::memory_order_relaxed)) break;
        const std::size_t c = next_chunk.fetch_add(1);
        if (c >= chunks) break;
        lanes->sink(slot, records.subspan(c * chunk, std::min(chunk, records.size() - c * chunk)));
      }
      rebuilt.fetch_add(rebuilt_here, std::memory_order_relaxed);
      counter_add(state.timing.wall_ns, wall_ns() - wall_start);
      counter_add(state.timing.cpu_ns, thread_cpu_ns() - cpu_start);
    });
    local_timing.tail_ns = wall_ns() - phase;
  }
  local_timing.rebuilt_covers = rebuilt.load();
  merge(result.pipeline.work, filter_work);
  result.workers.reserve(states.size());
  result.worker_tasks.assign(states.size(), 0);
  for (auto& state : states) {
    parallel_detail::merge_work(result.pipeline.front.work, state.front);
    merge(result.pipeline.work, state.work);
    counter_add(orchestration.completed_jobs, state.stats.jobs);
    counter_add(orchestration.edge_buffer_bytes_sum, state.work.peak_edge_buffer_bytes);
    state.stats.front_products = state.front.product_visits;
    state.stats.q3_emitted = state.work.q3_emitted;
    state.stats.q4_emitted = state.work.q4_emitted;
    state.stats.peak_edge_buffer_bytes = state.work.peak_edge_buffer_bytes;
    result.workers.push_back(state.stats);
    result.worker_timings.push_back(state.timing);
  }
  if (orchestration.completed_jobs != orchestration.jobs)
    throw std::logic_error("mhgp9 gen batched q34 lost its front job partition");
  validate_completion(result.pipeline, options);
  if (timing != nullptr) *timing = std::move(local_timing);
  return result;
}

}  // namespace mhgp9::gen
