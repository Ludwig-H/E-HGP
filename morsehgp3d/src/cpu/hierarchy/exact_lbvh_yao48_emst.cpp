#include "morsehgp3d/hierarchy/exact_lbvh_yao48_emst.hpp"

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/hierarchy/boruvka.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t invalid_node_index = static_cast<std::size_t>(-1);
constexpr std::array<std::array<std::uint8_t, 3>, 6> axis_permutations{{
    {{0U, 1U, 2U}},
    {{0U, 2U, 1U}},
    {{1U, 0U, 2U}},
    {{1U, 2U, 0U}},
    {{2U, 0U, 1U}},
    {{2U, 1U, 0U}},
}};

struct TraversalNode {
  std::array<spatial::PointId, 3> lower_point_ids{};
  std::array<spatial::PointId, 3> upper_point_ids{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
};

struct TraversalSnapshot {
  std::vector<spatial::PointId> leaves;
  std::vector<std::size_t> leaf_position_by_point_id;
  std::vector<TraversalNode> nodes;
  std::size_t root_index{invalid_node_index};
};

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t size)
      : parents_(size), component_count_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (value >= parents_.size()) {
      throw std::out_of_range("a Yao48 LBVH disjoint-set value is out of range");
    }
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t parent = parents_[value];
      parents_[value] = root;
      value = parent;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    const std::size_t root = std::min(left, right);
    parents_[std::max(left, right)] = root;
    --component_count_;
    return true;
  }

  [[nodiscard]] std::size_t component_count() const noexcept {
    return component_count_;
  }

 private:
  std::vector<std::size_t> parents_;
  std::size_t component_count_{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(message);
  }
  return left * right;
}

void checked_accumulate(
    std::size_t& destination,
    std::size_t increment,
    const char* message) {
  destination = checked_add(destination, increment, message);
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t point_index) {
  if (point_index > static_cast<std::size_t>(
                        spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("a Yao48 LBVH point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(point_index);
}

[[nodiscard]] std::size_t checked_point_index(
    spatial::PointId point_id,
    std::size_t point_count) {
  if (point_id >
      static_cast<spatial::PointId>(std::numeric_limits<std::size_t>::max())) {
    throw std::logic_error("a Yao48 LBVH PointId does not fit size_t");
  }
  const std::size_t point_index = static_cast<std::size_t>(point_id);
  if (point_index >= point_count) {
    throw std::logic_error("a Yao48 LBVH PointId is outside the cloud");
  }
  return point_index;
}

[[nodiscard]] exact::ExactLevel merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left_id,
    spatial::PointId right_id) {
  const exact::ExactRational3& left = cloud.point(left_id).exact();
  const exact::ExactRational3& right = cloud.point(right_id).exact();
  const exact::BigInt common_denominator =
      left.denominator() * right.denominator();
  exact::BigInt squared_numerator = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::BigInt delta =
        left.numerator(axis) * right.denominator() -
        right.numerator(axis) * left.denominator();
    squared_numerator += delta * delta;
  }
  return exact::ExactLevel{
      std::move(squared_numerator),
      common_denominator * common_denominator};
}

[[nodiscard]] ExactEmstEdge exact_edge(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left,
    spatial::PointId right) {
  if (left == right) {
    throw std::logic_error("a Yao48 LBVH edge cannot be a loop");
  }
  const spatial::PointId u = std::min(left, right);
  const spatial::PointId v = std::max(left, right);
  exact::ExactLevel squared_length = exact_squared_distance(cloud, u, v);
  if (squared_length == exact::ExactLevel{}) {
    throw std::invalid_argument(
        "exact Yao48 LBVH search requires distinct geometric points");
  }
  exact::ExactLevel level = merge_level(squared_length);
  return ExactEmstEdge{u, v, std::move(squared_length), std::move(level)};
}

[[nodiscard]] bool edge_less(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] bool same_edge_endpoints(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) noexcept {
  return left.u == right.u && left.v == right.v;
}

[[nodiscard]] bool edge_record_valid(
    const ExactEmstEdge& edge,
    std::size_t point_count) {
  return edge.u < edge.v && edge.v < point_count &&
         edge.squared_length > exact::ExactLevel{} &&
         edge.merge_level == merge_level(edge.squared_length);
}

[[nodiscard]] exact::ExactLevel exact_aabb_lower_bound(
    const TraversalNode& node,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId source_id) {
  const exact::ExactRational3& source = cloud.point(source_id).exact();
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational coordinate = source.coordinate(axis);
    const exact::ExactRational lower =
        cloud.point(node.lower_point_ids[axis]).coordinate(axis);
    const exact::ExactRational upper =
        cloud.point(node.upper_point_ids[axis]).coordinate(axis);
    exact::ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

struct NonnegativeInterval {
  exact::ExactRational lower{};
  exact::ExactRational upper{};
  bool nonempty{false};
};

[[nodiscard]] NonnegativeInterval oriented_nonnegative_interval(
    const exact::ExactRational& displacement_lower,
    const exact::ExactRational& displacement_upper,
    bool negative_direction) {
  NonnegativeInterval interval;
  if (negative_direction) {
    interval.lower = -displacement_upper;
    interval.upper = -displacement_lower;
  } else {
    interval.lower = displacement_lower;
    interval.upper = displacement_upper;
  }
  if (interval.upper < exact::ExactRational{}) {
    return interval;
  }
  if (interval.lower < exact::ExactRational{}) {
    interval.lower = exact::ExactRational{};
  }
  interval.nonempty = interval.lower <= interval.upper;
  return interval;
}

[[nodiscard]] bool ordered_intervals_feasible(
    const std::array<NonnegativeInterval, 3>& intervals,
    const std::array<std::uint8_t, 3>& descending_axes) {
  const std::size_t smallest_axis =
      static_cast<std::size_t>(descending_axes[2]);
  const std::size_t middle_axis =
      static_cast<std::size_t>(descending_axes[1]);
  const std::size_t largest_axis =
      static_cast<std::size_t>(descending_axes[0]);
  exact::ExactRational selected = intervals[smallest_axis].lower;
  if (selected > intervals[smallest_axis].upper) {
    return false;
  }
  if (selected < intervals[middle_axis].lower) {
    selected = intervals[middle_axis].lower;
  }
  if (selected > intervals[middle_axis].upper) {
    return false;
  }
  if (selected < intervals[largest_axis].lower) {
    selected = intervals[largest_axis].lower;
  }
  return selected <= intervals[largest_axis].upper;
}

// The half-open leaf chambers are contained in these closed chambers.  The
// interval feasibility test is exact: after orienting each coordinate by its
// sign, it asks whether independent nonnegative intervals admit values
// t[p0] >= t[p1] >= t[p2].
[[nodiscard]] std::uint64_t exact_closed_cone_aabb_mask(
    const TraversalNode& node,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId source_id,
    ExactLbvhYao48EmstCounters& counters) {
  const exact::ExactRational3& source = cloud.point(source_id).exact();
  std::array<exact::ExactRational, 3> displacement_lower{};
  std::array<exact::ExactRational, 3> displacement_upper{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    displacement_lower[axis] =
        cloud.point(node.lower_point_ids[axis]).coordinate(axis) -
        source.coordinate(axis);
    displacement_upper[axis] =
        cloud.point(node.upper_point_ids[axis]).coordinate(axis) -
        source.coordinate(axis);
  }

  std::uint64_t mask = 0U;
  for (std::uint8_t negative_mask = 0U;
       negative_mask < 8U;
       ++negative_mask) {
    std::array<NonnegativeInterval, 3> intervals{};
    bool sign_orthant_possible = true;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const std::uint8_t bit = static_cast<std::uint8_t>(
          std::uint32_t{1U} << static_cast<std::uint32_t>(axis));
      intervals[axis] = oriented_nonnegative_interval(
          displacement_lower[axis],
          displacement_upper[axis],
          (negative_mask & bit) != 0U);
      sign_orthant_possible =
          sign_orthant_possible && intervals[axis].nonempty;
    }
    for (std::size_t permutation_index = 0U;
         permutation_index < axis_permutations.size();
         ++permutation_index) {
      checked_accumulate(
          counters.closed_cone_aabb_intersection_test_count,
          1U,
          "the Yao48 cone-AABB test count overflows size_t");
      if (!sign_orthant_possible ||
          !ordered_intervals_feasible(
              intervals, axis_permutations[permutation_index])) {
        continue;
      }
      const std::size_t cone_index =
          static_cast<std::size_t>(negative_mask) *
              axis_permutations.size() +
          permutation_index;
      mask |= std::uint64_t{1U} << cone_index;
    }
  }
  return mask;
}

[[nodiscard]] spatial::PointId other_endpoint(
    const ExactEmstEdge& edge,
    spatial::PointId source_id) {
  if (edge.u == source_id) {
    return edge.v;
  }
  if (edge.v == source_id) {
    return edge.u;
  }
  throw std::logic_error("a Yao48 incumbent does not contain its source");
}

using IncumbentTable =
    std::array<std::optional<ExactEmstEdge>, exact_yao48_cone_count>;

void update_incumbent(
    IncumbentTable& incumbents,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId source_id,
    spatial::PointId target_id,
    ExactLbvhYao48EmstCounters& counters,
    bool seed) {
  ExactEmstEdge candidate = exact_edge(cloud, source_id, target_id);
  const std::size_t cone_index = classify_exact_yao48_cone(
                                     cloud.point(source_id),
                                     cloud.point(target_id))
                                     .cone_index;
  if (cone_index >= incumbents.size()) {
    throw std::logic_error("a Yao48 LBVH cone index exceeds 47");
  }
  std::optional<ExactEmstEdge>& incumbent = incumbents[cone_index];
  if (!incumbent.has_value() || edge_less(candidate, *incumbent)) {
    incumbent = std::move(candidate);
  }
  if (seed) {
    checked_accumulate(
        counters.seed_distance_evaluation_count,
        1U,
        "the Yao48 Morton seed distance count overflows size_t");
  } else {
    checked_accumulate(
        counters.leaf_distance_evaluation_count,
        1U,
        "the Yao48 traversal leaf distance count overflows size_t");
  }
  checked_accumulate(
      counters.directed_cone_classification_count,
      1U,
      "the Yao48 directed classification count overflows size_t");
}

[[nodiscard]] bool should_strictly_prune(
    std::uint64_t cone_mask,
    const exact::ExactLevel& lower_bound,
    const IncumbentTable& incumbents,
    bool& equality_only_descent) {
  equality_only_descent = false;
  bool all_strictly_better = true;
  bool all_nonstrictly_better = true;
  bool equality_present = false;
  for (std::size_t cone_index = 0U;
       cone_index < incumbents.size();
       ++cone_index) {
    const std::uint64_t bit = std::uint64_t{1U} << cone_index;
    if ((cone_mask & bit) == 0U) {
      continue;
    }
    const std::optional<ExactEmstEdge>& incumbent = incumbents[cone_index];
    if (!incumbent.has_value()) {
      return false;
    }
    if (!(lower_bound > incumbent->squared_length)) {
      all_strictly_better = false;
    }
    if (lower_bound < incumbent->squared_length) {
      all_nonstrictly_better = false;
    }
    if (lower_bound == incumbent->squared_length) {
      equality_present = true;
    }
  }
  equality_only_descent =
      !all_strictly_better && all_nonstrictly_better && equality_present;
  return all_strictly_better;
}

void validate_snapshot(
    const TraversalSnapshot& snapshot,
    std::size_t point_count) {
  if (point_count == 0U || snapshot.leaves.size() != point_count ||
      snapshot.leaf_position_by_point_id.size() != point_count ||
      snapshot.nodes.empty() ||
      snapshot.root_index != snapshot.nodes.size() - 1U) {
    throw std::logic_error("a Yao48 Morton LBVH snapshot is incomplete");
  }
  if (point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U ||
      snapshot.nodes.size() != point_count * 2U - 1U) {
    throw std::logic_error("a Yao48 Morton LBVH snapshot has a wrong node count");
  }
  std::vector<unsigned char> point_seen(point_count, 0U);
  for (std::size_t position = 0U; position < point_count; ++position) {
    const std::size_t point_index =
        checked_point_index(snapshot.leaves[position], point_count);
    if (point_seen[point_index] != 0U ||
        snapshot.leaf_position_by_point_id[point_index] != position) {
      throw std::logic_error(
          "a Yao48 Morton LBVH snapshot is not a PointId permutation");
    }
    point_seen[point_index] = 1U;
  }
  for (std::size_t node_index = 0U;
       node_index < snapshot.nodes.size();
       ++node_index) {
    const TraversalNode& node = snapshot.nodes[node_index];
    if (node.leaf_begin >= node.leaf_end || node.leaf_end > point_count) {
      throw std::logic_error("a Yao48 Morton LBVH node has a wrong leaf range");
    }
    const std::size_t leaf_count = node.leaf_end - node.leaf_begin;
    if (leaf_count > std::numeric_limits<std::size_t>::max() / 2U + 1U ||
        leaf_count * 2U - 1U > node_index + 1U) {
      throw std::logic_error(
          "a Yao48 Morton LBVH is not strict postorder");
    }
  }
  const TraversalNode& root = snapshot.nodes[snapshot.root_index];
  if (root.leaf_begin != 0U || root.leaf_end != point_count) {
    throw std::logic_error("the Yao48 Morton LBVH root does not cover the cloud");
  }
}

[[nodiscard]] std::size_t subtree_node_count(std::size_t leaf_count) {
  if (leaf_count == 0U ||
      leaf_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error("a Yao48 subtree node count overflows size_t");
  }
  return leaf_count * 2U - 1U;
}

void advance_stackless_cursor(
    std::size_t& cursor,
    std::size_t skipped_node_count,
    bool& complete) {
  if (skipped_node_count == 0U || skipped_node_count > cursor + 1U) {
    throw std::logic_error("a Yao48 stackless subtree skip is invalid");
  }
  complete = skipped_node_count == cursor + 1U;
  if (!complete) {
    cursor -= skipped_node_count;
  }
}

[[nodiscard]] std::size_t fixed_output_byte_count(
    const ExactLbvhYao48EmstResult& result) {
  const std::size_t directed_bytes = checked_product(
      result.directed_candidates.size(),
      sizeof(ExactLbvhYao48DirectedCandidate),
      "the Yao48 directed output byte count overflows size_t");
  const std::size_t unique_bytes = checked_product(
      result.unique_candidate_edges.size(),
      sizeof(ExactEmstEdge),
      "the Yao48 unique-edge output byte count overflows size_t");
  const std::size_t emst_bytes = checked_product(
      result.emst_edges.size(),
      sizeof(ExactEmstEdge),
      "the Yao48 EMST output byte count overflows size_t");
  return checked_add(
      checked_add(
          directed_bytes,
          unique_bytes,
          "the Yao48 output byte count overflows size_t"),
      emst_bytes,
      "the Yao48 output byte count overflows size_t");
}

[[nodiscard]] ExactLbvhYao48EmstResult run_search(
    const TraversalSnapshot& snapshot,
    const spatial::CanonicalPointCloud& cloud,
    ExactLbvhYao48EmstConfig config) {
  const std::size_t point_count = cloud.size();
  validate_snapshot(snapshot, point_count);
  const std::size_t node_count = snapshot.nodes.size();
  const std::size_t point_cone_count = checked_product(
      point_count,
      exact_yao48_cone_count,
      "the Yao48 LBVH directed output bound overflows size_t");
  const std::size_t expected_node_coverage = checked_product(
      point_count,
      node_count,
      "the Yao48 LBVH node coverage count overflows size_t");
  const std::size_t expected_leaf_coverage = checked_product(
      point_count,
      point_count,
      "the Yao48 LBVH leaf coverage count overflows size_t");
  const std::size_t effective_seed_radius =
      point_count == 1U
          ? 0U
          : std::min(config.morton_seed_window_radius, point_count - 1U);

  ExactLbvhYao48EmstResult result;
  result.point_count = point_count;
  result.config = config;
  result.counters.point_count = point_count;
  result.counters.lbvh_node_count = node_count;
  result.counters.morton_seed_window_radius = effective_seed_radius;
  result.counters.morton_permutation_validated = true;
  if (point_cone_count > result.directed_candidates.max_size()) {
    throw std::length_error(
        "the Yao48 LBVH directed output exceeds vector capacity");
  }
  result.directed_candidates.reserve(point_cone_count);

  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    const std::size_t source_position =
        snapshot.leaf_position_by_point_id[source_index];
    const std::size_t seed_begin =
        source_position > effective_seed_radius
            ? source_position - effective_seed_radius
            : 0U;
    const std::size_t remaining = point_count - 1U - source_position;
    const std::size_t seed_end =
        source_position + std::min(effective_seed_radius, remaining) + 1U;

    IncumbentTable incumbents{};
    for (std::size_t candidate_position = seed_begin;
         candidate_position < seed_end;
         ++candidate_position) {
      if (candidate_position == source_position) {
        continue;
      }
      update_incumbent(
          incumbents,
          cloud,
          source_id,
          snapshot.leaves[candidate_position],
          result.counters,
          true);
    }

    std::size_t source_node_visit_count = 0U;
    std::size_t source_leaf_distance_count = 0U;
    std::size_t source_node_coverage = 0U;
    std::size_t source_leaf_coverage = 0U;
    std::size_t cursor = snapshot.root_index;
    bool complete = false;
    while (!complete) {
      if (cursor >= node_count || source_node_visit_count >= node_count) {
        throw std::logic_error(
            "a Yao48 stackless traversal escaped its certified LBVH");
      }
      const TraversalNode& node = snapshot.nodes[cursor];
      const std::size_t leaf_count = node.leaf_end - node.leaf_begin;
      const std::size_t subtree_nodes = subtree_node_count(leaf_count);
      ++source_node_visit_count;
      checked_accumulate(
          result.counters.node_visit_count,
          1U,
          "the Yao48 LBVH node visit count overflows size_t");

      const bool seed_covered =
          node.leaf_begin >= seed_begin && node.leaf_end <= seed_end;
      if (seed_covered) {
        checked_accumulate(
            result.counters.seeded_subtree_skip_count,
            1U,
            "the Yao48 seeded subtree skip count overflows size_t");
        checked_accumulate(
            result.counters.seeded_subtree_skipped_leaf_count,
            leaf_count,
            "the Yao48 seeded skipped leaf count overflows size_t");
        source_node_coverage = checked_add(
            source_node_coverage,
            subtree_nodes,
            "a Yao48 source node coverage count overflows size_t");
        source_leaf_coverage = checked_add(
            source_leaf_coverage,
            leaf_count,
            "a Yao48 source leaf coverage count overflows size_t");
        advance_stackless_cursor(cursor, subtree_nodes, complete);
        continue;
      }

      const std::uint64_t cone_mask = exact_closed_cone_aabb_mask(
          node, cloud, source_id, result.counters);
      if (cone_mask == 0U) {
        checked_accumulate(
            result.counters.cone_mask_empty_count,
            1U,
            "the Yao48 empty cone-mask count overflows size_t");
      }

      bool every_relevant_cone_has_incumbent = cone_mask != 0U;
      for (std::size_t cone_index = 0U;
           cone_index < incumbents.size();
           ++cone_index) {
        if ((cone_mask & (std::uint64_t{1U} << cone_index)) != 0U &&
            !incumbents[cone_index].has_value()) {
          every_relevant_cone_has_incumbent = false;
          break;
        }
      }

      bool prune = false;
      if (every_relevant_cone_has_incumbent) {
        const exact::ExactLevel lower_bound =
            exact_aabb_lower_bound(node, cloud, source_id);
        checked_accumulate(
            result.counters.exact_aabb_lower_bound_evaluation_count,
            1U,
            "the Yao48 exact AABB bound count overflows size_t");
        bool equality_only_descent = false;
        prune = should_strictly_prune(
            cone_mask, lower_bound, incumbents, equality_only_descent);
        if (equality_only_descent) {
          checked_accumulate(
              result.counters.equal_bound_descent_count,
              1U,
              "the Yao48 equal-bound descent count overflows size_t");
        }
      }

      if (prune) {
        checked_accumulate(
            result.counters.strict_aabb_prune_count,
            1U,
            "the Yao48 strict AABB prune count overflows size_t");
        checked_accumulate(
            result.counters.strict_pruned_leaf_count,
            leaf_count,
            "the Yao48 strict-pruned leaf count overflows size_t");
        source_node_coverage = checked_add(
            source_node_coverage,
            subtree_nodes,
            "a Yao48 source node coverage count overflows size_t");
        source_leaf_coverage = checked_add(
            source_leaf_coverage,
            leaf_count,
            "a Yao48 source leaf coverage count overflows size_t");
        advance_stackless_cursor(cursor, subtree_nodes, complete);
        continue;
      }

      source_node_coverage = checked_add(
          source_node_coverage,
          1U,
          "a Yao48 source node coverage count overflows size_t");
      if (leaf_count == 1U) {
        source_leaf_coverage = checked_add(
            source_leaf_coverage,
            1U,
            "a Yao48 source leaf coverage count overflows size_t");
        const spatial::PointId target_id =
            snapshot.leaves[node.leaf_begin];
        if (target_id != source_id) {
          update_incumbent(
              incumbents,
              cloud,
              source_id,
              target_id,
              result.counters,
              false);
          ++source_leaf_distance_count;
        }
      }
      advance_stackless_cursor(cursor, 1U, complete);
    }

    if (source_node_coverage != node_count ||
        source_leaf_coverage != point_count) {
      throw std::logic_error(
          "a Yao48 stackless traversal did not cover the complete LBVH");
    }
    checked_accumulate(
        result.counters.certified_node_coverage_count,
        source_node_coverage,
        "the Yao48 certified node coverage overflows size_t");
    checked_accumulate(
        result.counters.certified_leaf_coverage_count,
        source_leaf_coverage,
        "the Yao48 certified leaf coverage overflows size_t");
    result.counters.maximum_node_visit_count_per_source = std::max(
        result.counters.maximum_node_visit_count_per_source,
        source_node_visit_count);
    result.counters.maximum_leaf_distance_evaluation_count_per_source =
        std::max(
            result.counters.maximum_leaf_distance_evaluation_count_per_source,
            source_leaf_distance_count);
    checked_accumulate(
        result.counters.completed_source_count,
        1U,
        "the Yao48 completed source count overflows size_t");

    for (std::size_t cone_index = 0U;
         cone_index < incumbents.size();
         ++cone_index) {
      if (!incumbents[cone_index].has_value()) {
        continue;
      }
      result.directed_candidates.push_back(
          ExactLbvhYao48DirectedCandidate{
              source_id,
              cone_index,
              other_endpoint(*incumbents[cone_index], source_id)});
    }
  }

  if (result.counters.certified_node_coverage_count !=
          expected_node_coverage ||
      result.counters.certified_leaf_coverage_count !=
          expected_leaf_coverage) {
    throw std::logic_error(
        "the aggregate Yao48 LBVH coverage counters did not close");
  }
  result.counters.directed_candidate_count =
      result.directed_candidates.size();
  result.counters.empty_point_cone_count =
      point_cone_count - result.directed_candidates.size();

  std::vector<std::pair<spatial::PointId, spatial::PointId>> endpoints;
  endpoints.reserve(result.directed_candidates.size());
  for (const ExactLbvhYao48DirectedCandidate& candidate :
       result.directed_candidates) {
    endpoints.push_back(std::minmax(
        candidate.source_point_id, candidate.target_point_id));
  }
  std::sort(endpoints.begin(), endpoints.end());
  endpoints.erase(
      std::unique(endpoints.begin(), endpoints.end()), endpoints.end());
  result.unique_candidate_edges.reserve(endpoints.size());
  for (const auto& [u, v] : endpoints) {
    result.unique_candidate_edges.push_back(exact_edge(cloud, u, v));
    checked_accumulate(
        result.counters.unique_candidate_distance_evaluation_count,
        1U,
        "the Yao48 unique candidate distance count overflows size_t");
  }
  std::sort(
      result.unique_candidate_edges.begin(),
      result.unique_candidate_edges.end(),
      edge_less);
  result.counters.unique_candidate_edge_count =
      result.unique_candidate_edges.size();

  CanonicalDisjointSet components{point_count};
  result.emst_edges.reserve(point_count - 1U);
  exact::ExactRational total_squared_weight;
  for (const ExactEmstEdge& edge : result.unique_candidate_edges) {
    if (!components.unite(
            checked_point_index(edge.u, point_count),
            checked_point_index(edge.v, point_count))) {
      continue;
    }
    result.emst_edges.push_back(edge);
    total_squared_weight =
        total_squared_weight + edge.squared_length.rational();
    if (result.emst_edges.size() == point_count - 1U) {
      break;
    }
  }
  result.total_squared_weight =
      exact::ExactLevel{std::move(total_squared_weight)};
  result.total_hgp_weight = merge_level(result.total_squared_weight);
  result.counters.selected_edge_count = result.emst_edges.size();
  result.counters.redundant_candidate_edge_count =
      result.unique_candidate_edges.size() - result.emst_edges.size();
  result.counters.final_component_count = components.component_count();
  result.counters.fixed_output_record_byte_count =
      fixed_output_byte_count(result);
  result.counters.complete_source_coverage =
      result.counters.completed_source_count == point_count &&
      result.counters.certified_node_coverage_count == expected_node_coverage &&
      result.counters.certified_leaf_coverage_count == expected_leaf_coverage;
  result.counters.no_candidate_truncation = true;
  result.counters.higher_order_delaunay_mosaic_materialized = false;
  result.counters.global_pair_matrix_materialized = false;
  result.counters.public_status_claimed = false;

  if (result.emst_edges.size() != point_count - 1U ||
      components.component_count() != 1U ||
      !result.locally_structurally_valid()) {
    throw std::logic_error(
        "the exact Yao48 LBVH search did not close its spanning-tree audit");
  }
  return result;
}

[[nodiscard]] bool directed_candidates_match(
    const ExactLbvhYao48EmstResult& result,
    const ExactYao48EmstResult& reference) {
  if (result.directed_candidates.size() !=
      reference.directed_candidates.size()) {
    return false;
  }
  for (std::size_t index = 0U;
       index < result.directed_candidates.size();
       ++index) {
    const ExactLbvhYao48DirectedCandidate& observed =
        result.directed_candidates[index];
    const ExactYao48DirectedCandidate& expected =
        reference.directed_candidates[index];
    if (observed.source_point_id != expected.source_point_id ||
        observed.cone_index != expected.cone_index ||
        observed.target_point_id !=
            other_endpoint(expected.edge, expected.source_point_id)) {
      return false;
    }
  }
  return true;
}

}  // namespace

class ExactLbvhYao48EmstBuilder {
 public:
  [[nodiscard]] static ExactLbvhYao48EmstResult build(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactLbvhYao48EmstConfig config) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "exact Yao48 search requires a ready LBVH for the same point namespace");
    }
    TraversalSnapshot snapshot;
    snapshot.leaves.reserve(index.leaves_.size());
    for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
      snapshot.leaves.push_back(leaf.point_id);
    }
    snapshot.leaf_position_by_point_id = index.leaf_position_by_point_id_;
    snapshot.nodes.reserve(index.nodes_.size());
    for (const auto& node : index.nodes_) {
      snapshot.nodes.push_back(TraversalNode{
          node.lower_point_ids,
          node.upper_point_ids,
          node.leaf_begin,
          node.leaf_end});
    }
    snapshot.root_index = index.root_index_;
    return run_search(snapshot, cloud, config);
  }
};

bool ExactLbvhYao48EmstResult::locally_structurally_valid() const {
  try {
    if (schema_version != exact_lbvh_yao48_emst_schema_version ||
        point_count == 0U ||
        point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
      return false;
    }
    const std::size_t expected_node_count = point_count * 2U - 1U;
    const std::size_t expected_point_cone_count = checked_product(
        point_count,
        exact_yao48_cone_count,
        "the local Yao48 point-cone count overflows size_t");
    const std::size_t expected_node_coverage = checked_product(
        point_count,
        expected_node_count,
        "the local Yao48 node coverage count overflows size_t");
    const std::size_t expected_leaf_coverage = checked_product(
        point_count,
        point_count,
        "the local Yao48 leaf coverage count overflows size_t");
    const std::size_t effective_seed_radius =
        point_count == 1U
            ? 0U
            : std::min(config.morton_seed_window_radius, point_count - 1U);
    if (counters.seeded_subtree_skip_count > counters.node_visit_count) {
      return false;
    }
    const std::size_t nonseed_node_visit_count =
        counters.node_visit_count - counters.seeded_subtree_skip_count;
    const std::size_t expected_cone_aabb_test_count = checked_product(
        nonseed_node_visit_count,
        exact_yao48_cone_count,
        "the local Yao48 cone-AABB test count overflows size_t");
    if (counters.point_count != point_count ||
        counters.lbvh_node_count != expected_node_count ||
        counters.morton_seed_window_radius != effective_seed_radius ||
        counters.completed_source_count != point_count ||
        counters.node_visit_count < point_count ||
        counters.node_visit_count > expected_node_coverage ||
        counters.maximum_node_visit_count_per_source == 0U ||
        counters.maximum_node_visit_count_per_source > expected_node_count ||
        counters.maximum_leaf_distance_evaluation_count_per_source >=
            point_count ||
        counters.closed_cone_aabb_intersection_test_count !=
            expected_cone_aabb_test_count ||
        counters.cone_mask_empty_count > nonseed_node_visit_count ||
        counters.exact_aabb_lower_bound_evaluation_count >
            nonseed_node_visit_count ||
        counters.strict_aabb_prune_count >
            counters.exact_aabb_lower_bound_evaluation_count ||
        counters.equal_bound_descent_count >
            counters.exact_aabb_lower_bound_evaluation_count ||
        counters.certified_node_coverage_count != expected_node_coverage ||
        counters.certified_leaf_coverage_count != expected_leaf_coverage ||
        counters.directed_cone_classification_count !=
            checked_add(
                counters.seed_distance_evaluation_count,
                counters.leaf_distance_evaluation_count,
                "the local Yao48 distance count overflows size_t") ||
        counters.directed_candidate_count != directed_candidates.size() ||
        directed_candidates.size() > expected_point_cone_count ||
        counters.empty_point_cone_count !=
            expected_point_cone_count - directed_candidates.size() ||
        counters.unique_candidate_edge_count !=
            unique_candidate_edges.size() ||
        counters.unique_candidate_distance_evaluation_count !=
            unique_candidate_edges.size() ||
        unique_candidate_edges.size() > directed_candidates.size() ||
        counters.selected_edge_count != emst_edges.size() ||
        counters.redundant_candidate_edge_count !=
            unique_candidate_edges.size() - emst_edges.size() ||
        emst_edges.size() != point_count - 1U ||
        counters.final_component_count != 1U ||
        counters.fixed_output_record_byte_count !=
            fixed_output_byte_count(*this) ||
        !counters.complete_source_coverage ||
        !counters.no_candidate_truncation ||
        !counters.morton_permutation_validated ||
        counters.higher_order_delaunay_mosaic_materialized ||
        counters.global_pair_matrix_materialized ||
        counters.public_status_claimed) {
      return false;
    }

    for (std::size_t index = 0U;
         index < directed_candidates.size();
         ++index) {
      const ExactLbvhYao48DirectedCandidate& candidate =
          directed_candidates[index];
      if (candidate.source_point_id >= point_count ||
          candidate.target_point_id >= point_count ||
          candidate.source_point_id == candidate.target_point_id ||
          candidate.cone_index >= exact_yao48_cone_count) {
        return false;
      }
      if (index != 0U) {
        const ExactLbvhYao48DirectedCandidate& previous =
            directed_candidates[index - 1U];
        if (previous.source_point_id > candidate.source_point_id ||
            (previous.source_point_id == candidate.source_point_id &&
             previous.cone_index >= candidate.cone_index)) {
          return false;
        }
      }
    }
    if (!std::is_sorted(
            unique_candidate_edges.begin(),
            unique_candidate_edges.end(),
            edge_less) ||
        !std::is_sorted(emst_edges.begin(), emst_edges.end(), edge_less)) {
      return false;
    }
    for (std::size_t index = 0U;
         index < unique_candidate_edges.size();
         ++index) {
      const ExactEmstEdge& edge = unique_candidate_edges[index];
      if (!edge_record_valid(edge, point_count) ||
          (index != 0U &&
           same_edge_endpoints(unique_candidate_edges[index - 1U], edge))) {
        return false;
      }
    }

    CanonicalDisjointSet components{point_count};
    exact::ExactRational squared_total;
    for (const ExactEmstEdge& edge : emst_edges) {
      if (!edge_record_valid(edge, point_count) ||
          !std::binary_search(
              unique_candidate_edges.begin(),
              unique_candidate_edges.end(),
              edge,
              edge_less) ||
          !components.unite(
              checked_point_index(edge.u, point_count),
              checked_point_index(edge.v, point_count))) {
        return false;
      }
      squared_total = squared_total + edge.squared_length.rational();
    }
    const exact::ExactLevel expected_squared_total{
        std::move(squared_total)};
    return components.component_count() == 1U &&
           total_squared_weight == expected_squared_total &&
           total_hgp_weight == merge_level(expected_squared_total);
  } catch (const std::exception&) {
    return false;
  }
}

ExactLbvhYao48EmstResult build_exact_lbvh_yao48_emst(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactLbvhYao48EmstConfig config) {
  return ExactLbvhYao48EmstBuilder::build(index, cloud, config);
}

ExactLbvhYao48EmstVerification verify_exact_lbvh_yao48_emst_bounded(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactLbvhYao48EmstResult& result) {
  ExactLbvhYao48EmstVerification verification;
  verification.index_identity_certified = index.validated_for(cloud);
  verification.input_within_reference_bound =
      cloud.size() != 0U &&
      cloud.size() <= exact_yao48_emst_reference_max_point_count;
  verification.result_structurally_valid =
      result.locally_structurally_valid();
  if (!verification.index_identity_certified ||
      !verification.input_within_reference_bound) {
    return verification;
  }

  const ExactLbvhYao48EmstResult operational_replay =
      build_exact_lbvh_yao48_emst(index, cloud, result.config);
  verification.operational_replay_exact = result == operational_replay;

  const ExactYao48EmstResult reference =
      build_exact_yao48_emst_reference(cloud);
  verification.directed_candidates_match_reference =
      directed_candidates_match(result, reference);
  verification.unique_candidate_edges_match_reference =
      result.unique_candidate_edges == reference.unique_candidate_edges;
  verification.canonical_kruskal_matches_reference =
      result.emst_edges == reference.emst_edges;
  verification.exact_totals_match_reference =
      result.total_squared_weight == reference.total_squared_weight &&
      result.total_hgp_weight == reference.total_hgp_weight;

  const K1ExactBoruvkaResult boruvka =
      build_exact_lbvh_boruvka(index, cloud);
  verification.canonical_emst_matches_exact_lbvh_boruvka =
      result.emst_edges == boruvka.emst_edges &&
      result.total_squared_weight == boruvka.total_squared_weight &&
      result.total_hgp_weight == boruvka.total_hgp_weight;
  verification.full_bounded_geometry_transcript_exact =
      result.point_count == reference.point_count &&
      verification.directed_candidates_match_reference &&
      verification.unique_candidate_edges_match_reference &&
      verification.canonical_kruskal_matches_reference &&
      verification.exact_totals_match_reference;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
