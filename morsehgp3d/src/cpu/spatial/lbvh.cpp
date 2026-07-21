#include "morsehgp3d/spatial/lbvh.hpp"

#include "morsehgp3d/spatial/point_cloud_aabb.hpp"

#include "exact_query.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

constexpr std::uint64_t morton_grid_size =
    std::uint64_t{1} << MortonLbvhIndex::morton_bits_per_axis;
constexpr std::uint64_t maximum_morton_coordinate = morton_grid_size - 1U;

[[nodiscard]] exact::ExactRational point_coordinate(
    const CanonicalPointCloud& cloud,
    PointId point_id,
    std::size_t axis) {
  return cloud.point(point_id).exact().coordinate(axis);
}

[[nodiscard]] PointId lower_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const exact::ExactRational left_coordinate =
      point_coordinate(cloud, left, axis);
  const exact::ExactRational right_coordinate =
      point_coordinate(cloud, right, axis);
  if (right_coordinate < left_coordinate ||
      (right_coordinate == left_coordinate && right < left)) {
    return right;
  }
  return left;
}

[[nodiscard]] PointId upper_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const exact::ExactRational left_coordinate =
      point_coordinate(cloud, left, axis);
  const exact::ExactRational right_coordinate =
      point_coordinate(cloud, right, axis);
  if (right_coordinate > left_coordinate ||
      (right_coordinate == left_coordinate && right < left)) {
    return right;
  }
  return left;
}

[[nodiscard]] std::uint64_t quantized_coordinate(
    const exact::ExactRational& coordinate,
    const exact::ExactRational& lower,
    const exact::ExactRational& upper) {
  if (lower == upper) {
    return 0U;
  }
  if (coordinate < lower || coordinate > upper) {
    throw std::logic_error("a Morton coordinate lies outside the global AABB");
  }
  const exact::ExactRational ratio =
      (coordinate - lower) / (upper - lower);
  const exact::BigInt scaled_numerator =
      ratio.numerator() * morton_grid_size;
  const exact::BigInt bin = scaled_numerator / ratio.denominator();
  if (bin < 0 || bin > morton_grid_size) {
    throw std::logic_error("an exact Morton bin lies outside its grid");
  }
  if (bin == morton_grid_size) {
    return maximum_morton_coordinate;
  }
  return bin.convert_to<std::uint64_t>();
}

[[nodiscard]] std::uint64_t interleaved_morton_code(
    const std::array<std::uint64_t, 3>& coordinates) {
  std::uint64_t code = 0U;
  for (std::size_t bit = 0U;
       bit < MortonLbvhIndex::morton_bits_per_axis;
       ++bit) {
    const std::size_t output_bit = 3U * bit;
    code |= ((coordinates[0] >> bit) & 1U) << (output_bit + 2U);
    code |= ((coordinates[1] >> bit) & 1U) << (output_bit + 1U);
    code |= ((coordinates[2] >> bit) & 1U) << output_bit;
  }
  return code;
}

[[nodiscard]] bool morton_leaf_less(
    const MortonLeafRecord& left,
    const MortonLeafRecord& right) {
  if (left.morton_code != right.morton_code) {
    return left.morton_code < right.morton_code;
  }
  return left.point_id < right.point_id;
}

struct NodeQueueEntry {
  exact::ExactLevel lower_bound;
  std::size_t node_index;
};

struct NodeQueueCompare {
  LbvhTraversalOrder order;

  [[nodiscard]] bool operator()(
      const NodeQueueEntry& left,
      const NodeQueueEntry& right) const {
    if (left.lower_bound != right.lower_bound) {
      if (order == LbvhTraversalOrder::near_first) {
        return left.lower_bound > right.lower_bound;
      }
      return left.lower_bound < right.lower_bound;
    }
    if (order == LbvhTraversalOrder::near_first) {
      return left.node_index > right.node_index;
    }
    return left.node_index < right.node_index;
  }
};

struct WorstNeighborFirst {
  [[nodiscard]] bool operator()(
      const ExactNeighbor& left,
      const ExactNeighbor& right) const {
    return detail::exact_neighbor_less(left, right);
  }
};

enum class ClosedBallClass : unsigned char {
  unclassified,
  interior,
  shell,
  exterior,
};

void require_valid_traversal_order(LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case LbvhTraversalOrder::near_first:
    case LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

}  // namespace

MortonLbvhIndex MortonLbvhIndex::build(const CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument("a Morton LBVH requires a nonempty point cloud");
  }
  constexpr std::size_t maximum_size =
      std::numeric_limits<std::size_t>::max();
  if (point_count > maximum_size / 2U + 1U) {
    throw std::length_error("the Morton LBVH node count overflows size_t");
  }

  MortonLbvhIndex index;
  index.cloud_identity_ = cloud.identity_;
  index.point_count_ = point_count;
  index.leaves_.reserve(point_count);
  index.leaf_position_by_point_id_.assign(point_count, invalid_node_index);

  const ExactPointCloudAabb3 exact_point_bounds =
      build_exact_point_cloud_aabb(cloud);
  const std::array<PointId, 3>& lower_point_ids =
      exact_point_bounds.lower_witness_point_ids;
  const std::array<PointId, 3>& upper_point_ids =
      exact_point_bounds.upper_witness_point_ids;

  std::array<exact::ExactRational, 3> lower_coordinates{};
  std::array<exact::ExactRational, 3> upper_coordinates{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    lower_coordinates[axis] =
        point_coordinate(cloud, lower_point_ids[axis], axis);
    upper_coordinates[axis] =
        point_coordinate(cloud, upper_point_ids[axis], axis);
  }

  for (std::size_t point_index = 0U; point_index < point_count; ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    std::array<std::uint64_t, 3> quantized{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      quantized[axis] = quantized_coordinate(
          point_coordinate(cloud, point_id, axis),
          lower_coordinates[axis],
          upper_coordinates[axis]);
    }
    index.leaves_.push_back(
        MortonLeafRecord{interleaved_morton_code(quantized), point_id});
  }
  std::sort(index.leaves_.begin(), index.leaves_.end(), morton_leaf_less);

  std::size_t collision_group_count = 0U;
  std::size_t maximum_collision_size = 0U;
  std::size_t group_begin = 0U;
  while (group_begin < point_count) {
    std::size_t group_end = group_begin + 1U;
    while (group_end < point_count &&
           index.leaves_[group_end].morton_code ==
               index.leaves_[group_begin].morton_code) {
      ++group_end;
    }
    const std::size_t group_size = group_end - group_begin;
    if (group_size > 1U) {
      ++collision_group_count;
      maximum_collision_size = std::max(maximum_collision_size, group_size);
    }
    group_begin = group_end;
  }

  for (std::size_t position = 0U; position < point_count; ++position) {
    const PointId point_id = index.leaves_[position].point_id;
    const std::size_t point_index = static_cast<std::size_t>(point_id);
    if (point_index >= point_count ||
        index.leaf_position_by_point_id_[point_index] != invalid_node_index) {
      throw std::logic_error("the Morton order repeats or loses a PointId");
    }
    index.leaf_position_by_point_id_[point_index] = position;
  }

  const std::size_t expected_node_count = point_count * 2U - 1U;
  index.nodes_.reserve(expected_node_count);
  index.root_index_ = index.build_range(cloud, 0U, point_count, 0U);
  index.build_counters_.point_count = point_count;
  index.build_counters_.node_count = index.nodes_.size();
  index.build_counters_.morton_collision_group_count = collision_group_count;
  index.build_counters_.maximum_morton_collision_size = maximum_collision_size;

  const Node& root = index.nodes_[index.root_index_];
  if (root.lower_point_ids != exact_point_bounds.lower_witness_point_ids ||
      root.upper_point_ids != exact_point_bounds.upper_witness_point_ids) {
    throw std::logic_error(
        "the Morton LBVH root disagrees with the exact point-cloud extrema");
  }
  index.root_aabb_ = exact_point_bounds.bounds;
  index.validate_structure(cloud);
  index.structure_complete_ = true;
  return index;
}

std::size_t MortonLbvhIndex::find_split(
    std::size_t begin,
    std::size_t end) const {
  if (begin + 1U >= end || end > leaves_.size()) {
    throw std::logic_error("an LBVH split requires at least two leaves");
  }
  const std::uint64_t first_code = leaves_[begin].morton_code;
  const std::uint64_t last_code = leaves_[end - 1U].morton_code;
  if (first_code == last_code) {
    return begin + (end - begin) / 2U;
  }
  const std::uint64_t difference = first_code ^ last_code;
  const unsigned int highest_bit =
      static_cast<unsigned int>(std::bit_width(difference) - 1);
  const std::uint64_t mask = std::uint64_t{1} << highest_bit;
  std::size_t low = begin + 1U;
  std::size_t high = end;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2U;
    if ((leaves_[middle].morton_code & mask) == 0U) {
      low = middle + 1U;
    } else {
      high = middle;
    }
  }
  if (low <= begin || low >= end) {
    throw std::logic_error("a Morton radix split did not divide its range");
  }
  return low;
}

std::size_t MortonLbvhIndex::build_range(
    const CanonicalPointCloud& cloud,
    std::size_t begin,
    std::size_t end,
    std::size_t depth) {
  if (begin >= end || end > leaves_.size()) {
    throw std::logic_error("an LBVH node has an invalid leaf range");
  }
  build_counters_.maximum_depth =
      std::max(build_counters_.maximum_depth, depth);
  if (end - begin == 1U) {
    Node leaf;
    leaf.leaf_begin = begin;
    leaf.leaf_end = end;
    leaf.lower_point_ids.fill(leaves_[begin].point_id);
    leaf.upper_point_ids.fill(leaves_[begin].point_id);
    nodes_.push_back(leaf);
    return nodes_.size() - 1U;
  }

  const std::size_t split = find_split(begin, end);
  const std::size_t left_child =
      build_range(cloud, begin, split, depth + 1U);
  const std::size_t right_child =
      build_range(cloud, split, end, depth + 1U);
  Node node;
  node.left_child = left_child;
  node.right_child = right_child;
  node.leaf_begin = begin;
  node.leaf_end = end;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    node.lower_point_ids[axis] = lower_witness(
        cloud,
        nodes_[left_child].lower_point_ids[axis],
        nodes_[right_child].lower_point_ids[axis],
        axis);
    node.upper_point_ids[axis] = upper_witness(
        cloud,
        nodes_[left_child].upper_point_ids[axis],
        nodes_[right_child].upper_point_ids[axis],
        axis);
  }
  nodes_.push_back(node);
  return nodes_.size() - 1U;
}

void MortonLbvhIndex::validate_structure(
    const CanonicalPointCloud& cloud) const {
  if (cloud_identity_ == nullptr || cloud_identity_ != cloud.identity_ ||
      point_count_ == 0U || leaves_.size() != point_count_ ||
      leaf_position_by_point_id_.size() != point_count_ ||
      root_index_ >= nodes_.size() ||
      nodes_.size() != point_count_ * 2U - 1U ||
      build_counters_.node_count != nodes_.size() ||
      build_counters_.point_count != point_count_) {
    throw std::logic_error("invalid Morton LBVH top-level storage");
  }
  if (!std::is_sorted(leaves_.begin(), leaves_.end(), morton_leaf_less)) {
    throw std::logic_error("Morton LBVH leaves are not canonically sorted");
  }
  std::vector<unsigned char> point_seen(point_count_, 0U);
  for (std::size_t position = 0U; position < leaves_.size(); ++position) {
    const std::size_t point_index =
        static_cast<std::size_t>(leaves_[position].point_id);
    if (point_index >= point_count_ || point_seen[point_index] != 0U ||
        leaf_position_by_point_id_[point_index] != position) {
      throw std::logic_error("Morton LBVH leaves do not form a PointId permutation");
    }
    point_seen[point_index] = 1U;
  }

  std::vector<unsigned char> node_seen(nodes_.size(), 0U);
  std::size_t observed_maximum_depth = 0U;
  const auto validate_node = [&cloud, &node_seen, &observed_maximum_depth, this](
                                 auto&& self,
                                 std::size_t node_index,
                                 std::size_t depth) -> void {
    if (node_index >= nodes_.size() || node_seen[node_index] != 0U) {
      throw std::logic_error("Morton LBVH contains a cycle or repeated child");
    }
    node_seen[node_index] = 1U;
    observed_maximum_depth = std::max(observed_maximum_depth, depth);
    const Node& node = nodes_[node_index];
    if (node.leaf_begin >= node.leaf_end ||
        node.leaf_end > leaves_.size()) {
      throw std::logic_error("Morton LBVH node range is invalid");
    }
    if (node.is_leaf()) {
      if (node.right_child != invalid_node_index ||
          node.leaf_end - node.leaf_begin != 1U) {
        throw std::logic_error("Morton LBVH leaf storage is invalid");
      }
      const PointId point_id = leaves_[node.leaf_begin].point_id;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if (node.lower_point_ids[axis] != point_id ||
            node.upper_point_ids[axis] != point_id) {
          throw std::logic_error("Morton LBVH leaf AABB has a wrong witness");
        }
      }
      return;
    }
    if (node.right_child == invalid_node_index ||
        node.left_child >= node_index || node.right_child >= node_index) {
      throw std::logic_error("Morton LBVH internal children are invalid");
    }
    self(self, node.left_child, depth + 1U);
    self(self, node.right_child, depth + 1U);
    const Node& left = nodes_[node.left_child];
    const Node& right = nodes_[node.right_child];
    if (node.leaf_begin != left.leaf_begin ||
        left.leaf_end != right.leaf_begin ||
        node.leaf_end != right.leaf_end) {
      throw std::logic_error("Morton LBVH child ranges do not partition their parent");
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const PointId expected_lower = lower_witness(
          cloud,
          left.lower_point_ids[axis],
          right.lower_point_ids[axis],
          axis);
      const PointId expected_upper = upper_witness(
          cloud,
          left.upper_point_ids[axis],
          right.upper_point_ids[axis],
          axis);
      if (node.lower_point_ids[axis] != expected_lower ||
          node.upper_point_ids[axis] != expected_upper) {
        throw std::logic_error("Morton LBVH parent AABB is not the exact merge");
      }
    }
  };
  validate_node(validate_node, root_index_, 0U);
  if (nodes_[root_index_].leaf_begin != 0U ||
      nodes_[root_index_].leaf_end != point_count_ ||
      std::find(node_seen.begin(), node_seen.end(), 0U) != node_seen.end() ||
      observed_maximum_depth != build_counters_.maximum_depth) {
    throw std::logic_error("Morton LBVH root does not cover its complete tree");
  }
}

std::size_t MortonLbvhIndex::eligible_count_in_node(
    const Node& node,
    const ExclusionSet& exclusions) const {
  std::size_t eligible_count = node.leaf_end - node.leaf_begin;
  for (const PointId excluded_id : exclusions.ids()) {
    const std::size_t point_index = static_cast<std::size_t>(excluded_id);
    const std::size_t position = leaf_position_by_point_id_[point_index];
    if (position >= node.leaf_begin && position < node.leaf_end) {
      --eligible_count;
    }
  }
  return eligible_count;
}

exact::ExactLevel MortonLbvhIndex::minimum_squared_distance_to_node(
    const CanonicalPointCloud& cloud,
    std::size_t node_index,
    const exact::ExactRational3& query) const {
  if (node_index >= nodes_.size()) {
    throw std::logic_error("an AABB query references an invalid LBVH node");
  }
  const Node& node = nodes_[node_index];
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational query_coordinate = query.coordinate(axis);
    const exact::ExactRational lower =
        point_coordinate(cloud, node.lower_point_ids[axis], axis);
    const exact::ExactRational upper =
        point_coordinate(cloud, node.upper_point_ids[axis], axis);
    exact::ExactRational delta;
    if (query_coordinate < lower) {
      delta = lower - query_coordinate;
    } else if (query_coordinate > upper) {
      delta = query_coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

exact::ExactLevel MortonLbvhIndex::maximum_squared_distance_to_node(
    const CanonicalPointCloud& cloud,
    std::size_t node_index,
    const exact::ExactRational3& query) const {
  if (node_index >= nodes_.size()) {
    throw std::logic_error("an AABB query references an invalid LBVH node");
  }
  const Node& node = nodes_[node_index];
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational query_coordinate = query.coordinate(axis);
    const exact::ExactRational lower_delta =
        query_coordinate -
        point_coordinate(cloud, node.lower_point_ids[axis], axis);
    const exact::ExactRational upper_delta =
        query_coordinate -
        point_coordinate(cloud, node.upper_point_ids[axis], axis);
    const exact::ExactRational lower_square = lower_delta * lower_delta;
    const exact::ExactRational upper_square = upper_delta * upper_delta;
    squared_distance = squared_distance +
                       (lower_square < upper_square ? upper_square : lower_square);
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

MortonLbvhIndex::MortonLbvhIndex(MortonLbvhIndex&& other) noexcept
    : cloud_identity_(std::move(other.cloud_identity_)),
      point_count_(std::exchange(other.point_count_, 0U)),
      leaves_(std::move(other.leaves_)),
      leaf_position_by_point_id_(std::move(other.leaf_position_by_point_id_)),
      nodes_(std::move(other.nodes_)),
      root_index_(std::exchange(other.root_index_, invalid_node_index)),
      build_counters_(std::exchange(
          other.build_counters_, MortonLbvhBuildCounters{})),
      root_aabb_(std::exchange(other.root_aabb_, ExactDyadicAabb3{})),
      structure_complete_(std::exchange(other.structure_complete_, false)) {}

MortonLbvhIndex& MortonLbvhIndex::operator=(
    MortonLbvhIndex&& other) noexcept {
  if (this != &other) {
    structure_complete_ = false;
    cloud_identity_ = std::move(other.cloud_identity_);
    point_count_ = std::exchange(other.point_count_, 0U);
    leaves_ = std::move(other.leaves_);
    leaf_position_by_point_id_ =
        std::move(other.leaf_position_by_point_id_);
    nodes_ = std::move(other.nodes_);
    root_index_ = std::exchange(other.root_index_, invalid_node_index);
    build_counters_ = std::exchange(
        other.build_counters_, MortonLbvhBuildCounters{});
    root_aabb_ = std::exchange(other.root_aabb_, ExactDyadicAabb3{});
    structure_complete_ = std::exchange(other.structure_complete_, false);
  }
  return *this;
}

TopKPartition lbvh_top_k(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Morton LBVH belongs to a different canonical point namespace");
  }
  if (!exclusions.validated_for(cloud)) {
    throw std::invalid_argument(
        "the exclusion set belongs to a different canonical point namespace");
  }
  require_valid_traversal_order(traversal_order);
  const std::size_t eligible_point_count =
      cloud.size() - exclusions.ids().size();
  if (requested_rank == 0U || requested_rank > eligible_point_count) {
    throw std::out_of_range("the requested rank is outside the eligible point set");
  }
  const exact::ExactRational3 canonical_query =
      detail::validated_query(query);

  SpatialQueryCounters counters;
  counters.method = SpatialQueryMethod::morton_lbvh;
  counters.excluded_point_count = exclusions.ids().size();
  std::vector<ExactNeighbor> evaluated_neighbors;
  evaluated_neighbors.reserve(std::min(eligible_point_count, std::size_t{256}));
  std::priority_queue<
      ExactNeighbor,
      std::vector<ExactNeighbor>,
      WorstNeighborFirst>
      best_neighbors;
  std::priority_queue<
      NodeQueueEntry,
      std::vector<NodeQueueEntry>,
      NodeQueueCompare>
      nodes_to_visit{NodeQueueCompare{traversal_order}};

  exact::ExactLevel root_bound = index.minimum_squared_distance_to_node(
      cloud, index.root_index_, canonical_query);
  ++counters.exact_aabb_bound_evaluation_count;
  nodes_to_visit.push(
      NodeQueueEntry{std::move(root_bound), index.root_index_});
  while (!nodes_to_visit.empty()) {
    NodeQueueEntry entry = nodes_to_visit.top();
    nodes_to_visit.pop();
    ++counters.node_visit_count;
    if (best_neighbors.size() == requested_rank &&
        entry.lower_bound > best_neighbors.top().squared_distance) {
      detail::record_strict_pruning_margin(
          counters,
          entry.lower_bound,
          best_neighbors.top().squared_distance);
      ++counters.pruned_subtree_count;
      counters.pruned_eligible_point_count +=
          index.eligible_count_in_node(
              index.nodes_[entry.node_index], exclusions);
      continue;
    }

    const MortonLbvhIndex::Node& node = index.nodes_[entry.node_index];
    if (node.is_leaf()) {
      const PointId point_id = index.leaves_[node.leaf_begin].point_id;
      if (exclusions.contains(point_id)) {
        continue;
      }
      ExactNeighbor neighbor{
          point_id,
          detail::exact_squared_distance(
              canonical_query, cloud.point(point_id))};
      ++counters.exact_point_distance_evaluation_count;
      evaluated_neighbors.push_back(neighbor);
      if (best_neighbors.size() < requested_rank) {
        best_neighbors.push(std::move(neighbor));
      } else if (detail::exact_neighbor_less(
                     neighbor, best_neighbors.top())) {
        best_neighbors.pop();
        best_neighbors.push(std::move(neighbor));
      }
      continue;
    }

    ++counters.internal_node_expansion_count;
    for (const std::size_t child : {node.left_child, node.right_child}) {
      exact::ExactLevel child_bound =
          index.minimum_squared_distance_to_node(
              cloud, child, canonical_query);
      ++counters.exact_aabb_bound_evaluation_count;
      nodes_to_visit.push(NodeQueueEntry{std::move(child_bound), child});
    }
  }

  return TopKPartition::from_evaluated_neighbors(
      cloud,
      requested_rank,
      std::move(evaluated_neighbors),
      eligible_point_count,
      std::move(counters));
}

TopKPartition lbvh_nearest(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order) {
  return lbvh_top_k(
      index, cloud, query, 1U, exclusions, traversal_order);
}

ClosedBallPartition lbvh_closed_ball(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const exact::ExactLevel& squared_radius,
    LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Morton LBVH belongs to a different canonical point namespace");
  }
  const exact::ExactRational3 canonical_query =
      detail::validated_query(query);
  const exact::ExactLevel canonical_squared_radius =
      detail::validated_squared_radius(squared_radius);
  require_valid_traversal_order(traversal_order);
  SpatialQueryCounters counters;
  counters.method = SpatialQueryMethod::morton_lbvh;
  std::vector<ClosedBallClass> point_classes(
      cloud.size(), ClosedBallClass::unclassified);
  std::vector<PointId> interior_ids;
  std::vector<PointId> shell_ids;
  std::vector<PointId> exterior_ids;
  const auto classify_point = [&point_classes](
                                  PointId point_id,
                                  ClosedBallClass point_class) {
    const std::size_t point_index = static_cast<std::size_t>(point_id);
    if (point_index >= point_classes.size() ||
        point_classes[point_index] != ClosedBallClass::unclassified) {
      throw std::logic_error(
          "an LBVH ball traversal repeated or lost a PointId");
    }
    point_classes[point_index] = point_class;
  };
  const auto classify_node = [&classify_point, &index](
                                 const MortonLbvhIndex::Node& node,
                                 ClosedBallClass point_class) {
    for (std::size_t position = node.leaf_begin;
         position < node.leaf_end;
         ++position) {
      classify_point(index.leaves_[position].point_id, point_class);
    }
  };
  std::priority_queue<
      NodeQueueEntry,
      std::vector<NodeQueueEntry>,
      NodeQueueCompare>
      nodes_to_visit{NodeQueueCompare{traversal_order}};
  exact::ExactLevel root_bound = index.minimum_squared_distance_to_node(
      cloud, index.root_index_, canonical_query);
  ++counters.exact_aabb_bound_evaluation_count;
  nodes_to_visit.push(
      NodeQueueEntry{std::move(root_bound), index.root_index_});
  while (!nodes_to_visit.empty()) {
    NodeQueueEntry entry = nodes_to_visit.top();
    nodes_to_visit.pop();
    const std::size_t node_index = entry.node_index;
    ++counters.node_visit_count;
    const MortonLbvhIndex::Node& node = index.nodes_[node_index];
    if (entry.lower_bound > canonical_squared_radius) {
      detail::record_strict_pruning_margin(
          counters, entry.lower_bound, canonical_squared_radius);
      ++counters.pruned_subtree_count;
      counters.pruned_eligible_point_count +=
          node.leaf_end - node.leaf_begin;
      classify_node(node, ClosedBallClass::exterior);
      continue;
    }
    const exact::ExactLevel maximum_distance =
        index.maximum_squared_distance_to_node(
            cloud, node_index, canonical_query);
    ++counters.exact_aabb_bound_evaluation_count;
    if (maximum_distance < canonical_squared_radius) {
      detail::record_strict_pruning_margin(
          counters, canonical_squared_radius, maximum_distance);
      ++counters.bulk_interior_subtree_count;
      counters.bulk_interior_point_count += node.leaf_end - node.leaf_begin;
      classify_node(node, ClosedBallClass::interior);
      continue;
    }
    if (node.is_leaf()) {
      const PointId point_id = index.leaves_[node.leaf_begin].point_id;
      const exact::ExactLevel distance = detail::exact_squared_distance(
          canonical_query, cloud.point(point_id));
      ++counters.exact_point_distance_evaluation_count;
      if (distance < canonical_squared_radius) {
        classify_point(point_id, ClosedBallClass::interior);
      } else if (distance == canonical_squared_radius) {
        classify_point(point_id, ClosedBallClass::shell);
      } else {
        classify_point(point_id, ClosedBallClass::exterior);
      }
      continue;
    }
    ++counters.internal_node_expansion_count;
    for (const std::size_t child : {node.left_child, node.right_child}) {
      exact::ExactLevel child_bound =
          index.minimum_squared_distance_to_node(
              cloud, child, canonical_query);
      ++counters.exact_aabb_bound_evaluation_count;
      nodes_to_visit.push(NodeQueueEntry{std::move(child_bound), child});
    }
  }
  interior_ids.reserve(counters.bulk_interior_point_count);
  exterior_ids.reserve(counters.pruned_eligible_point_count);
  for (std::size_t point_index = 0U;
       point_index < point_classes.size();
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    switch (point_classes[point_index]) {
      case ClosedBallClass::interior:
        interior_ids.push_back(point_id);
        break;
      case ClosedBallClass::shell:
        shell_ids.push_back(point_id);
        break;
      case ClosedBallClass::exterior:
        exterior_ids.push_back(point_id);
        break;
      case ClosedBallClass::unclassified:
        throw std::logic_error(
            "an LBVH ball traversal did not classify every PointId");
    }
  }
  return ClosedBallPartition{
      cloud,
      canonical_squared_radius,
      std::move(interior_ids),
      std::move(shell_ids),
      std::move(exterior_ids),
      std::move(counters)};
}

}  // namespace morsehgp3d::spatial
