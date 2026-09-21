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
#include <mutex>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp8 {
namespace {

// The same immutable index is shared by the front, every cover and both
// lanes. These reductions retain every existing backend work field; their
// compile-time size checks prevent silently omitting a newly added counter.
#define MHGP8_ADD(field) counter_add(a.field, b.field)
#define MHGP8_MAX(field) a.field = std::max(a.field, b.field)

void merge(WspdQ3AtlasWork& a, const WspdQ3AtlasWork& b) {
  static_assert(sizeof(WspdQ3AtlasWork) == 5 * sizeof(u64));
  MHGP8_ADD(edges_with_atlas); MHGP8_ADD(root_lane_skips); MHGP8_ADD(locations);
  MHGP8_ADD(outside_domain); MHGP8_ADD(rejections);
}

void merge(Q34EdgeCoverWork& a, const Q34EdgeCoverWork& b) {
  static_assert(sizeof(Q34EdgeCoverWork) == 10 * sizeof(u64));
  MHGP8_ADD(node_visits); MHGP8_ADD(bound_tests); MHGP8_ADD(point_tests);
  MHGP8_ADD(admitted_nodes); MHGP8_ADD(rejected_nodes); MHGP8_ADD(split_nodes);
  MHGP8_ADD(admitted_sites); MHGP8_ADD(rejected_sites); MHGP8_ADD(retained_ranges);
  MHGP8_ADD(merged_ranges);
}

void merge(Q4PositiveDomainWork& a, const Q4PositiveDomainWork& b) {
  static_assert(sizeof(Q4PositiveDomainWork) == 12 * sizeof(u64));
  MHGP8_ADD(node_visits); MHGP8_ADD(bound_tests); MHGP8_ADD(endpoint_box_tests);
  MHGP8_ADD(endpoint_leaf_tests); MHGP8_ADD(point_tests); MHGP8_ADD(admitted_nodes);
  MHGP8_ADD(rejected_nodes); MHGP8_ADD(split_nodes); MHGP8_ADD(excluded_endpoints);
  MHGP8_ADD(admitted_sites); MHGP8_ADD(rejected_sites); MHGP8_ADD(box_merges);
}

void merge(Q4LocalGeometryQueryWork& a, const Q4LocalGeometryQueryWork& b) {
  static_assert(sizeof(Q4LocalGeometryQueryWork) == 2 * sizeof(u64));
  MHGP8_ADD(disk_tests); MHGP8_ADD(facet_tests);
}

void merge(Q4LocalGeometryWork& a, const Q4LocalGeometryWork& b) {
  static_assert(sizeof(Q4LocalGeometryWork) == 15 * sizeof(u64) + sizeof(Q4PositiveDomainWork));
  merge(a.domain, b.domain);
  MHGP8_ADD(preparations); MHGP8_ADD(cover_node_visits); MHGP8_ADD(cover_range_advances);
  MHGP8_ADD(cover_disjoint_nodes); MHGP8_ADD(cover_splits); MHGP8_ADD(cover_blocks);
  MHGP8_ADD(cover_sites); MHGP8_ADD(cover_excluded_sites); MHGP8_ADD(cover_node_ids_copied);
  MHGP8_ADD(projection_points); MHGP8_ADD(hull_sort_comparisons); MHGP8_ADD(hull_orientation_tests);
  MHGP8_ADD(hull_vertices); MHGP8_ADD(facets);
  MHGP8_MAX(peak_retained_bytes);
}

void merge(Q4LocalPartitionWork& a, const Q4LocalPartitionWork& b) {
  static_assert(sizeof(Q4LocalPartitionWork) == 20 * sizeof(u64));
  MHGP8_ADD(root_factories); MHGP8_ADD(child_factories); MHGP8_ADD(refine_factories);
  MHGP8_ADD(input_nodes); MHGP8_ADD(input_sites); MHGP8_ADD(inherited_inside_sites);
  MHGP8_ADD(node_visits); MHGP8_ADD(block_bound_tests); MHGP8_ADD(point_tests);
  MHGP8_ADD(z_splits); MHGP8_ADD(inside_nodes); MHGP8_ADD(outside_nodes);
  MHGP8_ADD(inside_sites); MHGP8_ADD(outside_sites); MHGP8_ADD(active_nodes);
  MHGP8_ADD(active_sites); MHGP8_ADD(budget_unexamined_nodes); MHGP8_ADD(budget_ambiguous_nodes);
  MHGP8_ADD(frontier_ids_copied);
  MHGP8_MAX(peak_retained_bytes);
}

void merge(Q4LocalAtlasWork& a, const Q4LocalAtlasWork& b) {
  static_assert(sizeof(Q4LocalAtlasWork) == 16 * sizeof(u64) + sizeof(Q4LocalPartitionWork) + sizeof(Q4LocalGeometryQueryWork));
  merge(a.partition, b.partition);
  merge(a.domain, b.domain);
  MHGP8_ADD(cells_created); MHGP8_ADD(outside_cells); MHGP8_ADD(deep_cells);
  MHGP8_ADD(leaf_cells); MHGP8_ADD(splits); MHGP8_ADD(depth_stops);
  MHGP8_ADD(node_stops); MHGP8_ADD(small_stops); MHGP8_ADD(active_sites_sum);
  MHGP8_ADD(active_blocks_sum); MHGP8_ADD(terminal_refinements); MHGP8_ADD(terminal_deep_cells);
  MHGP8_MAX(max_depth); MHGP8_MAX(peak_fragment_bytes); MHGP8_MAX(peak_build_bytes);
  MHGP8_MAX(retained_bytes);
}

void merge(Q4LocalSweepWork& a, const Q4LocalSweepWork& b) {
  static_assert(sizeof(Q4LocalSweepWork) == 41 * sizeof(u64));
  MHGP8_ADD(seed_queries); MHGP8_ADD(seed_owner_tests); MHGP8_ADD(seed_owner_rejections);
  MHGP8_ADD(query_visits); MHGP8_ADD(line_tests); MHGP8_ADD(line_skips);
  MHGP8_ADD(leaf_queries); MHGP8_ADD(reference_points); MHGP8_ADD(reference_side_tests);
  MHGP8_ADD(active_blocks); MHGP8_ADD(active_sites); MHGP8_ADD(root_locations);
  MHGP8_ADD(clipped_events); MHGP8_ADD(clipped_inside); MHGP8_ADD(kept_events);
  MHGP8_ADD(constant_inside); MHGP8_ADD(constant_outside); MHGP8_ADD(constant_shell_ids);
  MHGP8_ADD(entries); MHGP8_ADD(exits); MHGP8_ADD(sort_comparisons);
  MHGP8_ADD(shell_sort_comparisons); MHGP8_ADD(group_comparisons); MHGP8_ADD(groups);
  MHGP8_ADD(boundary_skips); MHGP8_ADD(boundary_skipped_ids); MHGP8_ADD(depth_rejections);
  MHGP8_ADD(depth_skipped_ids); MHGP8_ADD(presentations); MHGP8_ADD(owner_tests);
  MHGP8_ADD(owner_rejections); MHGP8_ADD(positive_tests); MHGP8_ADD(positive_rejections);
  MHGP8_ADD(canonical_tests); MHGP8_ADD(canonical_rejections); MHGP8_ADD(emitted);
  MHGP8_ADD(shell_ids); MHGP8_ADD(groups_without_support); MHGP8_ADD(unexamined_after_emit);
  MHGP8_MAX(max_group); MHGP8_MAX(peak_buffer_bytes);
}

void merge(Q4LocalEdgeWork& a, const Q4LocalEdgeWork& b) {
  static_assert(sizeof(Q4LocalEdgeWork) == 11 * sizeof(u64) + sizeof(Q4LocalAtlasWork) + sizeof(Q4LocalGeometryWork) + sizeof(Q4LocalSweepWork));
  merge(a.atlas, b.atlas);
  merge(a.geometry, b.geometry);
  merge(a.sweep, b.sweep);
  MHGP8_ADD(node_visits); MHGP8_ADD(bound_tests); MHGP8_ADD(point_tests);
  MHGP8_ADD(rejected_nodes); MHGP8_ADD(split_nodes); MHGP8_ADD(rejected_sites);
  MHGP8_ADD(acute_seeds); MHGP8_ADD(owner_tests); MHGP8_ADD(owner_rejections);
  MHGP8_ADD(seeds);
  MHGP8_MAX(peak_live_buffer_bytes);
}

void merge(Q4ShallowSetWork& a, const Q4ShallowSetWork& b) {
  static_assert(sizeof(Q4ShallowSetWork) == 25 * sizeof(u64));
  MHGP8_ADD(preparations); MHGP8_ADD(input_sites); MHGP8_ADD(form_tests);
  MHGP8_ADD(zero_sites); MHGP8_ADD(positive_sites); MHGP8_ADD(negative_sites);
  MHGP8_ADD(lex_comparisons); MHGP8_ADD(orientation_tests); MHGP8_ADD(coordinate_groups);
  MHGP8_ADD(duplicate_ids); MHGP8_ADD(positive_layers); MHGP8_ADD(negative_layers);
  MHGP8_ADD(layer_input_groups); MHGP8_ADD(layer_input_ids); MHGP8_ADD(boundary_groups);
  MHGP8_ADD(degenerate_groups); MHGP8_ADD(retained_ids); MHGP8_ADD(discarded_ids);
  MHGP8_ADD(retained_id_sort_comparisons); MHGP8_ADD(record_insertions); MHGP8_ADD(group_insertions);
  MHGP8_ADD(hull_index_copies); MHGP8_ADD(compaction_moves);
  MHGP8_MAX(peak_live_bytes); MHGP8_MAX(retained_bytes);
}

void merge(Q4FamilyWork& a, const Q4FamilyWork& b) {
  static_assert(sizeof(Q4FamilyWork) == 13 * sizeof(u64));
  MHGP8_ADD(sites); MHGP8_ADD(entries); MHGP8_ADD(exits);
  MHGP8_ADD(constant_inside); MHGP8_ADD(constant_on); MHGP8_ADD(constant_outside);
  MHGP8_ADD(sort_comparisons); MHGP8_ADD(group_comparisons); MHGP8_ADD(groups);
  MHGP8_ADD(callbacks); MHGP8_ADD(event_count);
  MHGP8_MAX(max_group); MHGP8_MAX(retained_capacity_bytes);
}

void merge(Q4ShallowSweepWork& a, const Q4ShallowSweepWork& b) {
  static_assert(sizeof(Q4ShallowSweepWork) == 19 * sizeof(u64) + sizeof(Q4FamilyWork));
  merge(a.family, b.family);
  MHGP8_ADD(seed_queries); MHGP8_ADD(seed_owner_tests); MHGP8_ADD(seed_owner_rejections);
  MHGP8_ADD(removed_seed_rejections); MHGP8_ADD(membership_comparisons); MHGP8_ADD(depth_rejected_groups);
  MHGP8_ADD(depth_skipped_ids); MHGP8_ADD(presentations); MHGP8_ADD(owner_tests);
  MHGP8_ADD(owner_rejections); MHGP8_ADD(positive_tests); MHGP8_ADD(positive_rejections);
  MHGP8_ADD(canonical_tests); MHGP8_ADD(canonical_rejections); MHGP8_ADD(groups_without_support);
  MHGP8_ADD(unexamined_after_emit); MHGP8_ADD(emitted); MHGP8_ADD(shell_ids);
  MHGP8_MAX(peak_buffer_bytes);
}

void merge(Q4WindowSelectionWork& a, const Q4WindowSelectionWork& b) {
  static_assert(sizeof(Q4WindowSelectionWork) == 25 * sizeof(u64));
  MHGP8_ADD(seed_queries); MHGP8_ADD(entry_heap_insertions); MHGP8_ADD(exit_heap_insertions);
  MHGP8_ADD(entry_heap_replacements); MHGP8_ADD(exit_heap_replacements); MHGP8_ADD(heap_comparisons);
  MHGP8_ADD(heap_sort_comparisons); MHGP8_ADD(constant_rejected_seeds); MHGP8_ADD(disjoint_rejected_seeds);
  MHGP8_ADD(fixed_depth_rejected_seeds); MHGP8_ADD(lower_bounds); MHGP8_ADD(upper_bounds);
  MHGP8_ADD(point_windows); MHGP8_ADD(second_pass_sites); MHGP8_ADD(window_comparisons);
  MHGP8_ADD(lower_ids); MHGP8_ADD(upper_ids); MHGP8_ADD(inner_ids);
  MHGP8_ADD(outside_ids); MHGP8_ADD(fixed_inside_sites); MHGP8_ADD(rejected_event_ids);
  MHGP8_MAX(max_inner_ids); MHGP8_MAX(max_endpoint_ids); MHGP8_MAX(peak_heap_bytes);
  MHGP8_MAX(peak_buffer_bytes);
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
  MHGP8_ADD(seed_candidates); MHGP8_ADD(acute_tests); MHGP8_ADD(acute_seeds);
  MHGP8_ADD(owner_tests); MHGP8_ADD(owner_rejections); MHGP8_ADD(seeds);
  MHGP8_MAX(peak_live_buffer_bytes);
}

void merge(WspdQ3Work& a, const WspdQ3Work& b) {
  static_assert(sizeof(WspdQ3Work) == 23 * sizeof(u64));
  MHGP8_ADD(edge_queries); MHGP8_ADD(seed_node_visits); MHGP8_ADD(seed_bound_tests);
  MHGP8_ADD(seed_point_tests); MHGP8_ADD(seed_rejected_nodes); MHGP8_ADD(seed_split_nodes);
  MHGP8_ADD(seed_rejected_sites); MHGP8_ADD(acute_seeds); MHGP8_ADD(owner_tests);
  MHGP8_ADD(owner_rejections); MHGP8_ADD(seeds); MHGP8_ADD(ball_builds);
  MHGP8_ADD(census_range_visits); MHGP8_ADD(census_point_tests); MHGP8_ADD(census_inside_sites);
  MHGP8_ADD(census_outside_sites); MHGP8_ADD(census_shell_sites); MHGP8_ADD(depth_rejections);
  MHGP8_ADD(early_unread_sites); MHGP8_ADD(shell_sort_comparisons); MHGP8_ADD(shell_ids);
  MHGP8_ADD(emitted); MHGP8_MAX(peak_shell_bytes);
}

void merge(Q34WitnessSearchWork& a, const Q34WitnessSearchWork& b) {
  static_assert(sizeof(Q34WitnessSearchWork) == 25 * sizeof(u64));
  MHGP8_ADD(queries); MHGP8_ADD(q3_queries); MHGP8_ADD(q4_queries);
  MHGP8_ADD(prepared_bounds); MHGP8_ADD(node_visits); MHGP8_ADD(h_bound_tests);
  MHGP8_ADD(xi_bound_tests); MHGP8_ADD(point_tests); MHGP8_ADD(h_excluded_nodes);
  MHGP8_ADD(admitted_nodes); MHGP8_ADD(fully_admitted_nodes); MHGP8_ADD(leaf_remainders);
  MHGP8_ADD(split_nodes); MHGP8_ADD(q3_lane_tests); MHGP8_ADD(q4_lane_tests);
  MHGP8_ADD(q3_admitted_nodes); MHGP8_ADD(q4_admitted_nodes);
  MHGP8_ADD(q3_credits); MHGP8_ADD(q4_credits); MHGP8_ADD(q3_rejected); MHGP8_ADD(q4_rejected);
  MHGP8_ADD(midpoint_box_tests); MHGP8_ADD(pending_nodes_skipped);
  MHGP8_MAX(peak_stack); MHGP8_MAX(stack_storage_bytes);
}

void merge(Q34WitnessBoundsWork& a, const Q34WitnessBoundsWork& b) {
  static_assert(sizeof(Q34WitnessBoundsWork) == 12 * sizeof(u64));
  MHGP8_ADD(queries); MHGP8_ADD(pair_preparations); MHGP8_ADD(general_preparations);
  MHGP8_ADD(affine_h_tests); MHGP8_ADD(affine_xi_tests); MHGP8_ADD(xi_on_nonpositive_minimum);
  MHGP8_ADD(q3_exclusion_tests); MHGP8_ADD(q4_exclusion_tests);
  MHGP8_ADD(q3_excluded_nodes); MHGP8_ADD(q4_excluded_nodes);
  MHGP8_ADD(fully_excluded_nodes); MHGP8_ADD(mixed_terminal_nodes);
}

void merge(WspdQ34WitnessWork& a, const WspdQ34WitnessWork& b) {
  static_assert(sizeof(WspdQ34WitnessWork) == 8 * sizeof(u64) +
      2 * sizeof(Q34WitnessSearchWork) + 2 * sizeof(Q34WitnessBoundsWork));
  MHGP8_ADD(input_pair_mass); MHGP8_ADD(rejected_rectangles); MHGP8_ADD(rectangle_pair_mass);
  MHGP8_ADD(rectangle_q3_pairs); MHGP8_ADD(rectangle_q4_pairs);
  MHGP8_ADD(rejected_pairs); MHGP8_ADD(pair_q3_pairs); MHGP8_ADD(pair_q4_pairs);
  merge(a.rectangles, b.rectangles); merge(a.pairs, b.pairs);
  merge(a.rectangles_bounds, b.rectangles_bounds); merge(a.pairs_bounds, b.pairs_bounds);
}

void merge(Q3BallCensusWork& a, const Q3BallCensusWork& b) {
  static_assert(sizeof(Q3BallCensusWork) == 26 * sizeof(u64));
  MHGP8_ADD(queries); MHGP8_ADD(preparations); MHGP8_ADD(vertex_axes);
  MHGP8_ADD(accepted_queries); MHGP8_ADD(rejected_queries);
  MHGP8_ADD(count_node_visits); MHGP8_ADD(count_bounds_prepared);
  MHGP8_ADD(count_box_bound_tests); MHGP8_ADD(count_point_tests);
  MHGP8_ADD(count_inside_nodes); MHGP8_ADD(count_inside_sites);
  MHGP8_ADD(count_nonnegative_nodes); MHGP8_ADD(count_nonnegative_sites);
  MHGP8_ADD(count_split_nodes); MHGP8_ADD(count_prepared_unvisited); MHGP8_ADD(count_saturations);
  MHGP8_ADD(shell_node_visits); MHGP8_ADD(shell_bounds_prepared);
  MHGP8_ADD(shell_box_bound_tests); MHGP8_ADD(shell_point_tests);
  MHGP8_ADD(shell_excluded_nodes); MHGP8_ADD(shell_split_nodes); MHGP8_ADD(shell_ids);
  MHGP8_MAX(peak_count_stack); MHGP8_MAX(peak_shell_stack); MHGP8_MAX(stack_storage_bytes);
}

void merge(Q4SeedCellWork& a, const Q4SeedCellWork& b) {
  static_assert(sizeof(Q4SeedCellWork) == 37 * sizeof(u64));
  MHGP8_ADD(queries); MHGP8_ADD(live_preparations); MHGP8_ADD(live_node_visits);
  MHGP8_ADD(live_child_reads); MHGP8_ADD(live_leaves); MHGP8_ADD(whole_atlas_skips);
  MHGP8_ADD(live_skipped_nodes); MHGP8_ADD(antichain_node_visits); MHGP8_ADD(antichain_splits);
  MHGP8_ADD(blocks); MHGP8_ADD(block_sites); MHGP8_ADD(cache_entries_initialized);
  MHGP8_ADD(cache_hits); MHGP8_ADD(cache_misses); MHGP8_ADD(invalid_cache_hits);
  MHGP8_ADD(family_preparations); MHGP8_ADD(family_cache_hits); MHGP8_ADD(form_preparations);
  MHGP8_ADD(product_visits); MHGP8_ADD(product_seed_rejections); MHGP8_ADD(positive_products);
  MHGP8_ADD(negative_products); MHGP8_ADD(uncertain_products); MHGP8_ADD(zero_bound_products);
  MHGP8_ADD(block_bound_tests); MHGP8_ADD(singleton_bound_tests); MHGP8_ADD(spatial_tests_reused);
  MHGP8_ADD(x_splits); MHGP8_ADD(cell_splits); MHGP8_ADD(terminal_pairs);
  MHGP8_MAX(max_block_sites); MHGP8_MAX(peak_cache_bytes); MHGP8_MAX(peak_live_bytes);
  MHGP8_MAX(peak_product_stack); MHGP8_MAX(product_stack_bytes);
  MHGP8_MAX(peak_auxiliary_bytes); MHGP8_MAX(peak_total_buffer_bytes);
}

void merge(WspdQ34Work& a, const WspdQ34Work& b) {
  static_assert(sizeof(WspdQ34Work) == 13 * sizeof(u64) + sizeof(Q34EdgeCoverWork) +
      sizeof(WspdQ3Work) + sizeof(Q4LocalEdgeWork) + sizeof(Q4WindowEdgeWork) + sizeof(WspdQ34WitnessWork) + sizeof(Q3BallCensusWork) + sizeof(Q4SeedCellWork) + sizeof(WspdQ3AtlasWork));
  MHGP8_ADD(input_rectangles); MHGP8_ADD(expanded_pairs); MHGP8_ADD(q3_edges);
  MHGP8_ADD(q4_edges); MHGP8_ADD(both_edges); MHGP8_ADD(cover_builds);
  MHGP8_ADD(cover_sites); MHGP8_MAX(max_cover_sites); MHGP8_MAX(peak_cover_bytes);
  MHGP8_ADD(q3_emitted); MHGP8_ADD(q4_emitted); MHGP8_ADD(payload_shell_ids);
  MHGP8_MAX(peak_edge_buffer_bytes);
  merge(a.cover, b.cover); merge(a.q3, b.q3);
  merge(a.local, b.local); merge(a.window, b.window);
  merge(a.witness, b.witness);
  merge(a.q3_blocks, b.q3_blocks);
  merge(a.q4_seed_cells, b.q4_seed_cells);
  merge(a.q3_atlas, b.q3_atlas);
}

#undef MHGP8_ADD
#undef MHGP8_MAX

u64 pair_count(std::size_t n) {
  if (n < 2) return 0;
  const i128 pairs = n % 2 == 0 ? static_cast<i128>(n / 2) * (n - 1)
                                : static_cast<i128>(n) * ((n - 1) / 2);
  if (pairs > std::numeric_limits<u64>::max())
    throw std::overflow_error("mhgp8 q34 pair mass exceeds u64");
  return static_cast<u64>(pairs);
}

u64 storage_bytes(std::size_t capacity, std::size_t element_size) {
  if (capacity > std::numeric_limits<u64>::max() / element_size)
    throw std::overflow_error("mhgp8 q34 capacity exceeds u64");
  return static_cast<u64>(capacity) * element_size;
}

u64 shell_bytes(std::size_t capacity) {
  return storage_bytes(capacity, sizeof(std::size_t));
}

void validate(Q2CensusIndexPtr index, unsigned k, unsigned s,
              const WspdQ34Options& options, bool consumer_valid) {
  if (!index || !consumer_valid || k == 0 || k > 10 || s == 0)
    throw std::invalid_argument("mhgp8 global q34 requires index, callback, K1..10 and positive s");
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
    throw std::invalid_argument("mhgp8 unsupported global q34 options");
  // Match the unchanged local28 public contract, even if this call requests
  // no active q4 lane or selects Window30. Inert options cannot hide errors.
  if ((options.local.domain != Q4CenterDomainMode::Disk &&
       options.local.domain != Q4CenterDomainMode::Positive) ||
      options.local.max_depth > 44 || options.local.node_budget == 0)
    throw std::invalid_argument("mhgp8 global q34 requires valid local options");
  if ((options.q4_seed_cells.mode != Q4SeedCellMode::Individual &&
       options.q4_seed_cells.mode != Q4SeedCellMode::LiveOnly &&
       options.q4_seed_cells.mode != Q4SeedCellMode::Joined) ||
      options.q4_seed_cells.block_sites == 0 ||
      (options.q4_backend == WspdQ4Backend::Window30 &&
       options.q4_seed_cells.mode != Q4SeedCellMode::Individual))
    throw std::invalid_argument("mhgp8 invalid or incompatible q4 seed-cell options");
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
  counter_add(expanded, witness.rectangle_pair_mass);
  counter_add(covered, witness.rejected_pairs);
  if (q3 != result.front.work.residual_pair_mass[1] ||
      q4 != result.front.work.residual_pair_mass[2] ||
      covered != result.work.expanded_pairs || expanded != witness.input_pair_mass ||
      result.work.input_rectangles != result.front.work.emitted_rectangles ||
      result.work.q3_emitted != result.work.q3.emitted ||
      result.work.q4_emitted != (options.q4_backend == WspdQ4Backend::Local28
          ? result.work.local.sweep.emitted : result.work.window.sweep.sweep.emitted))
    throw std::logic_error("mhgp8 global q34 completed ledger mismatch");
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
      throw std::overflow_error("mhgp8 q34 rectangle mass exceeds u64");
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

  WspdQ34Work work{};
  u64 split_rectangles{};  // Not a geometric counter: orchestration only.

 private:
  void emit(const Q34SeedCandidate& candidate) {
    if (candidate.arity == 3) counter_add(work.q3_emitted);
    else if (candidate.arity == 4) counter_add(work.q4_emitted);
    else throw std::logic_error("mhgp8 global q34 backend emitted another arity");
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
      throw std::logic_error("mhgp8 global q34 received an inactive rectangle");
    counter_add(work.expanded_pairs);
    if (options_.witness_mode != WspdQ34WitnessMode::Disabled) {
      const auto points = index_->cloud().points();
      const auto filtered = filter_q34_witnesses(*index_, singleton_box(points[a]),
          singleton_box(points[b]), static_cast<std::uint8_t>(k_), mask, work.witness.pairs,
          options_.witness_bounds_mode, work.witness.pairs_bounds);
      if ((mask & 2U) != 0 && (filtered & 2U) == 0) counter_add(work.witness.pair_q3_pairs);
      if ((mask & 4U) != 0 && (filtered & 4U) == 0) counter_add(work.witness.pair_q4_pairs);
      mask = filtered;
      if (mask == 0) { counter_add(work.witness.rejected_pairs); return; }
    }
    const bool q3 = (mask & 2U) != 0, q4 = (mask & 4U) != 0;
    if (q3) counter_add(work.q3_edges);
    if (q4) counter_add(work.q4_edges);
    if (q3 && q4) counter_add(work.both_edges);
    const auto cover = Q34EdgeCover::make(index_, {a, b});
    counter_add(work.cover_builds);
    counter_add(work.cover_sites, static_cast<u64>(cover->site_count()));
    work.max_cover_sites = std::max(work.max_cover_sites, static_cast<u64>(cover->site_count()));
    merge(work.cover, cover->work());
    observe(cover);
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
    if (atlas) {
      // The certified count is a lower bound of the strict interior of the
      // seed ball (a, b, x): reaching K-1 rejects it exactly, before any
      // ball construction or census. No credit is transferred otherwise.
      counter_add(work.q3_atlas.locations);
      const auto certified = atlas->certified_inside_count(atlas->geometry()->q3_center(ids[2]));
      if (!certified) counter_add(work.q3_atlas.outside_domain);
      else if (*certified >= k_ - 1) {
        counter_add(work.q3_atlas.rejections);
        counter_add(q3.depth_rejections);
        return;
      }
    }
    counter_add(q3.ball_builds);
    const auto ball = ExactBall::make_q3({points[ids[0]], points[ids[1]], points[ids[2]]});
    if (!ball) throw std::logic_error("mhgp8 q3 owned acute seed lacks its exact ball");
    shell_.clear();
    std::size_t depth = 0, visited = 0;
    if (options_.q3_census_mode == WspdQ3CensusMode::GlobalBoxes) {
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
      // Under u16 each sum is <=6*65535^2<2^35. Ownership permits equality
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
    throw std::invalid_argument("mhgp8 parallel q34 requires positive workers/job granularity");
  if (worker_count > std::numeric_limits<std::size_t>::max() / jobs_per_worker)
    throw std::overflow_error("mhgp8 parallel q34 target job count overflow");
  const auto target_jobs = worker_count * jobs_per_worker;
  WspdQ34ParallelResult result{};
  result.pipeline = empty_result(index, kmax, options);
  auto& orchestration = result.parallel;
  orchestration.requested_workers = static_cast<u64>(worker_count);
  orchestration.target_jobs = static_cast<u64>(target_jobs);
  if (result.pipeline.front.active_lane_mask == 0) return result;

  const auto plan = make_wspd_front_jobs(index, kmax, separation_s, options.front_mode,
                                       target_jobs, options.requested_lane_mask);
  result.pipeline.front = plan->prefix_result();
  orchestration.prefix_product_visits = result.pipeline.front.work.product_visits;
  orchestration.jobs = static_cast<u64>(plan->job_count());
  orchestration.terminal_jobs = static_cast<u64>(plan->terminal_job_count());
  orchestration.job_storage_bytes = static_cast<u64>(plan->retained_bytes());
  const auto started = std::min(worker_count, plan->job_count());
  orchestration.started_workers = static_cast<u64>(started);

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
        u64 waited = 0;
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
                const auto part = plan->run_job(*job, receiver);
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
        state.timing = {wall_ns() - wall_start, thread_cpu_ns() - cpu_start, waited};
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
    throw std::logic_error("mhgp8 parallel q34 lost its front job partition");
  if (result.tasks.published != result.tasks.consumed || !queue.pending.empty())
    throw std::logic_error("mhgp8 parallel q34 lost a published rectangle range");
  validate_completion(result.pipeline, options);
  return result;
}

}  // namespace mhgp8
