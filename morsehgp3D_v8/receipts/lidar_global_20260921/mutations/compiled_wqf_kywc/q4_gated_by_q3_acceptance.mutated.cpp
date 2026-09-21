#include "pipeline/wspd_q34.hpp"

#include "parallel/joined_workers.hpp"
#include "parallel/work_reduction.hpp"

#include <algorithm>
#include <array>
#include <atomic>
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

void merge(WspdQ34Work& a, const WspdQ34Work& b) {
  static_assert(sizeof(WspdQ34Work) == 13 * sizeof(u64) + sizeof(Q34EdgeCoverWork) +
      sizeof(WspdQ3Work) + sizeof(Q4LocalEdgeWork) + sizeof(Q4WindowEdgeWork));
  MHGP8_ADD(input_rectangles); MHGP8_ADD(expanded_pairs); MHGP8_ADD(q3_edges);
  MHGP8_ADD(q4_edges); MHGP8_ADD(both_edges); MHGP8_ADD(cover_builds);
  MHGP8_ADD(cover_sites); MHGP8_MAX(max_cover_sites); MHGP8_MAX(peak_cover_bytes);
  MHGP8_ADD(q3_emitted); MHGP8_ADD(q4_emitted); MHGP8_ADD(payload_shell_ids);
  MHGP8_MAX(peak_edge_buffer_bytes);
  merge(a.cover, b.cover); merge(a.q3, b.q3);
  merge(a.local, b.local); merge(a.window, b.window);
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
       options.q4_backend != WspdQ4Backend::Window30))
    throw std::invalid_argument("mhgp8 unsupported global q34 options");
  // Match the unchanged local28 public contract, even if this call requests
  // no active q4 lane or selects Window30. Inert options cannot hide errors.
  if ((options.local.domain != Q4CenterDomainMode::Disk &&
       options.local.domain != Q4CenterDomainMode::Positive) ||
      options.local.max_depth > 44 || options.local.node_budget == 0)
    throw std::invalid_argument("mhgp8 global q34 requires valid local options");
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
  if (result.work.q3_edges != result.front.work.residual_pair_mass[1] ||
      result.work.q4_edges != result.front.work.residual_pair_mass[2] ||
      result.work.cover_builds != result.work.expanded_pairs ||
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

class Engine {
 public:
  Engine(Q2CensusIndexPtr index, unsigned k, WspdQ34Options options,
         const Q34SeedConsumer& consumer)
      : index_(std::move(index)), k_(k), options_(options), consumer_(consumer),
        sink_([this](const Q34SeedCandidate& candidate) { emit(candidate); }) {}
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(Engine&&) = delete;

  void rectangle(const WspdRectangle& rectangle) {
    const auto nodes = index_->spatial_nodes();
    const auto order = index_->spatial_order();
    const auto a = nodes[rectangle.a_node].range;
    const auto b = nodes[rectangle.b_node].range;
    counter_add(work.input_rectangles);
    // This is an explicit expansion of the certified residual WSPD products,
    // not an all-pairs fallback. The front's disjoint cover guarantees one
    // visit per unordered residual edge, even when both lanes survive.
    for (auto ai = a.first; ai < a.last; ++ai)
      for (auto bi = b.first; bi < b.last; ++bi)
        edge(order[ai], order[bi], rectangle.lane_mask);
  }

  WspdQ34Work work{};

 private:
  void emit(const Q34SeedCandidate& candidate) {
    if (candidate.arity == 3) counter_add(work.q3_emitted);
    else if (candidate.arity == 4) counter_add(work.q4_emitted);
    else throw std::logic_error("mhgp8 global q34 backend emitted another arity");
    counter_add(work.payload_shell_ids, static_cast<u64>(candidate.shell_first.size()));
    counter_add(work.payload_shell_ids, static_cast<u64>(candidate.shell_second.size()));
    consumer_(candidate);
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
    const bool q3 = (mask & 2U) != 0, q4 = (mask & 4U) != 0;
    if ((!q3 && !q4) || (mask & ~6U) != 0)
      throw std::logic_error("mhgp8 global q34 received an inactive rectangle");
    counter_add(work.expanded_pairs);
    if (q3) counter_add(work.q3_edges);
    if (q4) counter_add(work.q4_edges);
    if (q3 && q4) counter_add(work.both_edges);
    const auto cover = Q34EdgeCover::make(index_, {a, b});
    counter_add(work.cover_builds);
    counter_add(work.cover_sites, static_cast<u64>(cover->site_count()));
    work.max_cover_sites = std::max(work.max_cover_sites, static_cast<u64>(cover->site_count()));
    merge(work.cover, cover->work());
    observe(cover);
    const auto q3_before = work.q3.emitted;
    if (q3) {
      q3_edge(cover);
      observe(cover);
    }
    if (q4 && (!q3 || work.q3.emitted > q3_before)) {
      if (options_.q4_backend == WspdQ4Backend::Local28) {
        const auto local = run_q4_local_edge_candidates(cover, k_, options_.local, sink_);
        merge(work.local, local);
        observe(cover, local.peak_live_buffer_bytes);
      } else {
        const auto window = run_q4_window_edge_candidates(cover, k_, sink_);
        merge(work.window, window);
        observe(cover, window.peak_live_buffer_bytes);
      }
    }
  }

  void q3_seed(const Q34EdgeCoverPtr& cover, std::array<std::size_t, 3> ids) {
    auto& q3 = work.q3;
    const auto points = index_->cloud().points();
    const auto order = index_->spatial_order();
    counter_add(q3.ball_builds);
    const auto ball = ExactBall::make_q3({points[ids[0]], points[ids[1]], points[ids[2]]});
    if (!ball) throw std::logic_error("mhgp8 q3 owned acute seed lacks its exact ball");
    shell_.clear();
    std::size_t depth = 0, visited = 0;
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

  void q3_edge(const Q34EdgeCoverPtr& cover) {
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
              q3_seed(cover, {ids[0], ids[1], id});
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
  };
  std::vector<WorkerState> states(started);
  orchestration.worker_state_bytes = storage_bytes(states.capacity(), sizeof(WorkerState));
  std::atomic<std::size_t> next{0};
  parallel_detail::run_joined_workers(started,
      [&](std::size_t slot, const std::atomic<bool>& cancel) {
        auto& state = states[slot];
        const Q34SeedConsumer output = [&](const Q34SeedCandidate& candidate) {
          callbacks[slot](slot, candidate);
        };
        {
          Engine engine(index, kmax, options, output);
          const WspdRectangleConsumer receiver = [&](const WspdRectangle& rectangle) {
            engine.rectangle(rectangle);
          };
          while (!cancel.load(std::memory_order_relaxed)) {
            auto job = next.load(std::memory_order_relaxed);
            while (job < plan->job_count() &&
                   !next.compare_exchange_weak(job, job + 1, std::memory_order_relaxed)) {}
            if (job == plan->job_count()) break;
            // Coarse: do not interrupt an edge or re-seed its local census.
            // run_job includes either an unvisited subtree or the callback
            // of an already-counted terminal, never its prefix tests again.
            const auto part = plan->run_job(job, receiver);
            parallel_detail::merge_work(state.front, part.work);
            counter_add(state.stats.jobs);
          }
          state.work = engine.work;
        }  // Private engine buffers are released before this worker returns.
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
  for (const auto& state : states) {
    parallel_detail::merge_work(result.pipeline.front.work, state.front);
    merge(result.pipeline.work, state.work);
    counter_add(orchestration.completed_jobs, state.stats.jobs);
    counter_add(orchestration.edge_buffer_bytes_sum, state.stats.peak_edge_buffer_bytes);
    result.workers.push_back(state.stats);
  }
  if (orchestration.completed_jobs != orchestration.jobs)
    throw std::logic_error("mhgp8 parallel q34 lost its front job partition");
  validate_completion(result.pipeline, options);
  return result;
}

}  // namespace mhgp8
