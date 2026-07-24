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

#include <boost/multiprecision/cpp_int.hpp>

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

[[nodiscard]] std::uint64_t point_coordinate_bits(
    const CanonicalPointCloud& cloud,
    PointId point_id,
    std::size_t axis) {
  return cloud.point(point_id).canonical_input_bits()[axis];
}

[[nodiscard]] std::uint64_t binary64_order_key(
    std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63U;
  return (bits & sign_bit) != 0U ? ~bits : bits ^ sign_bit;
}

[[nodiscard]] PointId lower_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const std::uint64_t left_key = binary64_order_key(
      point_coordinate_bits(cloud, left, axis));
  const std::uint64_t right_key = binary64_order_key(
      point_coordinate_bits(cloud, right, axis));
  if (right_key < left_key ||
      (right_key == left_key && right < left)) {
    return right;
  }
  return left;
}

[[nodiscard]] PointId upper_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const std::uint64_t left_key = binary64_order_key(
      point_coordinate_bits(cloud, left, axis));
  const std::uint64_t right_key = binary64_order_key(
      point_coordinate_bits(cloud, right, axis));
  if (right_key > left_key ||
      (right_key == left_key && right < left)) {
    return right;
  }
  return left;
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

struct ExactMortonFrame {
  ExactPointCloudAabb3 point_bounds;
};

[[nodiscard]] ExactMortonFrame exact_morton_frame(
    const CanonicalPointCloud& cloud) {
  if (cloud.size() == 0U) {
    throw std::invalid_argument(
        "an exact Morton frame requires a nonempty point cloud");
  }
  ExactMortonFrame frame;
  frame.point_bounds.audit.point_count = cloud.size();
  frame.point_bounds.audit.exact_coordinate_evaluation_count =
      static_cast<std::uint64_t>(cloud.size()) * 3U;
  frame.point_bounds.audit.exact_extremum_comparison_count =
      static_cast<std::uint64_t>(cloud.size() - 1U) * 6U;
  const std::array<std::uint64_t, 3> first_bits =
      cloud.point(PointId{0}).canonical_input_bits();
  frame.point_bounds.bounds.lower_binary64_bits = first_bits;
  frame.point_bounds.bounds.upper_binary64_bits = first_bits;
  frame.point_bounds.lower_witness_point_ids.fill(PointId{0});
  frame.point_bounds.upper_witness_point_ids.fill(PointId{0});
  for (std::size_t point_index = 1U;
       point_index < cloud.size();
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    const std::array<std::uint64_t, 3> bits =
        cloud.point(point_id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (binary64_order_key(bits[axis]) <
          binary64_order_key(
              frame.point_bounds.bounds
                  .lower_binary64_bits[axis])) {
        frame.point_bounds.bounds.lower_binary64_bits[axis] =
            bits[axis];
        frame.point_bounds.lower_witness_point_ids[axis] =
            point_id;
      }
      if (binary64_order_key(bits[axis]) >
          binary64_order_key(
              frame.point_bounds.bounds
                  .upper_binary64_bits[axis])) {
        frame.point_bounds.bounds.upper_binary64_bits[axis] =
            bits[axis];
        frame.point_bounds.upper_witness_point_ids[axis] =
            point_id;
      }
    }
  }
  return frame;
}

struct SignedBinary64Dyadic {
  std::int64_t significand{};
  int exponent{};
};

[[nodiscard]] SignedBinary64Dyadic decode_binary64_dyadic(
    std::uint64_t bits) {
  constexpr std::uint64_t fraction_mask =
      (std::uint64_t{1} << 52U) - 1U;
  constexpr std::uint64_t exponent_mask = 0x7ffU;
  const std::uint64_t exponent_bits =
      (bits >> 52U) & exponent_mask;
  const std::uint64_t fraction = bits & fraction_mask;
  std::uint64_t magnitude = fraction;
  int exponent = -1074;
  if (exponent_bits != 0U) {
    if (exponent_bits == exponent_mask) {
      throw std::invalid_argument(
          "a Morton snapshot coordinate is not finite");
    }
    magnitude |= std::uint64_t{1} << 52U;
    exponent = static_cast<int>(exponent_bits) - 1075;
  }
  std::int64_t significand =
      static_cast<std::int64_t>(magnitude);
  if ((bits >> 63U) != 0U) {
    significand = -significand;
  }
  return {significand, exponent};
}

template <typename Integer>
[[nodiscard]] Integer aligned_significand(
    const SignedBinary64Dyadic& value,
    int minimum_exponent) {
  Integer aligned{value.significand};
  if (value.significand != 0) {
    aligned <<= static_cast<unsigned int>(
        value.exponent - minimum_exponent);
  }
  return aligned;
}

template <typename Integer>
[[nodiscard]] bool certify_quantized_inequality(
    const SignedBinary64Dyadic& lower,
    const SignedBinary64Dyadic& coordinate,
    const SignedBinary64Dyadic& upper,
    int minimum_exponent,
    std::uint64_t quantized) {
  const Integer lower_integer =
      aligned_significand<Integer>(lower, minimum_exponent);
  const Integer coordinate_integer =
      aligned_significand<Integer>(coordinate, minimum_exponent);
  const Integer upper_integer =
      aligned_significand<Integer>(upper, minimum_exponent);
  const Integer extent = upper_integer - lower_integer;
  const Integer delta = coordinate_integer - lower_integer;
  const Integer scaled_delta =
      Integer{morton_grid_size} * delta;
  return Integer{quantized} * extent <= scaled_delta &&
         scaled_delta <
             Integer{quantized + 1U} * extent;
}

template <typename Integer>
[[nodiscard]] std::uint64_t quantized_floor(
    const SignedBinary64Dyadic& lower,
    const SignedBinary64Dyadic& coordinate,
    const SignedBinary64Dyadic& upper,
    int minimum_exponent) {
  const Integer lower_integer =
      aligned_significand<Integer>(lower, minimum_exponent);
  const Integer coordinate_integer =
      aligned_significand<Integer>(coordinate, minimum_exponent);
  const Integer upper_integer =
      aligned_significand<Integer>(upper, minimum_exponent);
  const Integer extent = upper_integer - lower_integer;
  const Integer delta = coordinate_integer - lower_integer;
  const Integer quotient =
      (Integer{morton_grid_size} * delta) / extent;
  return quotient.template convert_to<std::uint64_t>();
}

[[nodiscard]] std::uint64_t quantized_coordinate_from_bits(
    std::uint64_t coordinate_bits,
    std::uint64_t lower_bits,
    std::uint64_t upper_bits) {
  if (binary64_order_key(coordinate_bits) <
          binary64_order_key(lower_bits) ||
      binary64_order_key(coordinate_bits) >
          binary64_order_key(upper_bits)) {
    throw std::logic_error(
        "a Morton coordinate lies outside the exact global AABB");
  }
  if (lower_bits == upper_bits) {
    return 0U;
  }
  if (coordinate_bits == upper_bits) {
    return maximum_morton_coordinate;
  }

  const SignedBinary64Dyadic lower =
      decode_binary64_dyadic(lower_bits);
  const SignedBinary64Dyadic coordinate =
      decode_binary64_dyadic(coordinate_bits);
  const SignedBinary64Dyadic upper =
      decode_binary64_dyadic(upper_bits);
  int minimum_exponent = 0;
  int maximum_exponent = 0;
  bool exponent_initialized = false;
  for (const SignedBinary64Dyadic value :
       {lower, coordinate, upper}) {
    if (value.significand == 0) {
      continue;
    }
    if (!exponent_initialized) {
      minimum_exponent = value.exponent;
      maximum_exponent = value.exponent;
      exponent_initialized = true;
    } else {
      minimum_exponent =
          std::min(minimum_exponent, value.exponent);
      maximum_exponent =
          std::max(maximum_exponent, value.exponent);
    }
  }
  if (!exponent_initialized) {
    throw std::logic_error(
        "a nondegenerate Morton interval has no dyadic exponent");
  }
  std::uint64_t quantized = 0U;
  if (maximum_exponent - minimum_exponent <= 50) {
    quantized = quantized_floor<
        boost::multiprecision::int128_t>(
        lower, coordinate, upper, minimum_exponent);
  } else {
    quantized = quantized_floor<exact::BigInt>(
        lower, coordinate, upper, minimum_exponent);
  }
  if (quantized > maximum_morton_coordinate) {
    throw std::logic_error(
        "an exact Morton bin lies outside its grid");
  }
  return quantized;
}

[[nodiscard]] std::uint64_t exact_morton_code(
    const CanonicalPointCloud& cloud,
    PointId point_id,
    const ExactMortonFrame& frame) {
  const std::array<std::uint64_t, 3> coordinate_bits =
      cloud.point(point_id).canonical_input_bits();
  std::array<std::uint64_t, 3> quantized{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    quantized[axis] = quantized_coordinate_from_bits(
        coordinate_bits[axis],
        frame.point_bounds.bounds.lower_binary64_bits[axis],
        frame.point_bounds.bounds.upper_binary64_bits[axis]);
  }
  return interleaved_morton_code(quantized);
}

[[nodiscard]] bool certify_quantized_coordinate(
    std::uint64_t coordinate_bits,
    std::uint64_t lower_bits,
    std::uint64_t upper_bits,
    std::uint64_t quantized) {
  if (quantized > maximum_morton_coordinate ||
      binary64_order_key(coordinate_bits) <
          binary64_order_key(lower_bits) ||
      binary64_order_key(coordinate_bits) >
          binary64_order_key(upper_bits)) {
    return false;
  }
  if (lower_bits == upper_bits) {
    return coordinate_bits == lower_bits && quantized == 0U;
  }
  if (coordinate_bits == upper_bits) {
    return quantized == maximum_morton_coordinate;
  }

  const SignedBinary64Dyadic lower =
      decode_binary64_dyadic(lower_bits);
  const SignedBinary64Dyadic coordinate =
      decode_binary64_dyadic(coordinate_bits);
  const SignedBinary64Dyadic upper =
      decode_binary64_dyadic(upper_bits);
  int minimum_exponent = 0;
  int maximum_exponent = 0;
  bool exponent_initialized = false;
  for (const SignedBinary64Dyadic value :
       {lower, coordinate, upper}) {
    if (value.significand == 0) {
      continue;
    }
    if (!exponent_initialized) {
      minimum_exponent = value.exponent;
      maximum_exponent = value.exponent;
      exponent_initialized = true;
    } else {
      minimum_exponent =
          std::min(minimum_exponent, value.exponent);
      maximum_exponent =
          std::max(maximum_exponent, value.exponent);
    }
  }
  if (!exponent_initialized) {
    return false;
  }
  if (maximum_exponent - minimum_exponent <= 50) {
    return certify_quantized_inequality<
        boost::multiprecision::int128_t>(
        lower,
        coordinate,
        upper,
        minimum_exponent,
        quantized);
  }
  return certify_quantized_inequality<exact::BigInt>(
      lower,
      coordinate,
      upper,
      minimum_exponent,
      quantized);
}

[[nodiscard]] std::array<std::uint64_t, 3>
deinterleaved_morton_coordinates(std::uint64_t code) {
  std::array<std::uint64_t, 3> coordinates{};
  for (std::size_t bit = 0U;
       bit < MortonLbvhIndex::morton_bits_per_axis;
       ++bit) {
    const std::size_t input_bit = 3U * bit;
    coordinates[0] |=
        ((code >> (input_bit + 2U)) & 1U) << bit;
    coordinates[1] |=
        ((code >> (input_bit + 1U)) & 1U) << bit;
    coordinates[2] |=
        ((code >> input_bit) & 1U) << bit;
  }
  return coordinates;
}

[[nodiscard]] bool certify_morton_code(
    const CanonicalPointCloud& cloud,
    PointId point_id,
    std::uint64_t code,
    const ExactMortonFrame& frame) {
  if ((code >> 63U) != 0U) {
    return false;
  }
  const std::array<std::uint64_t, 3> quantized =
      deinterleaved_morton_coordinates(code);
  const std::array<std::uint64_t, 3> coordinate_bits =
      cloud.point(point_id).canonical_input_bits();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (!certify_quantized_coordinate(
            coordinate_bits[axis],
            frame.point_bounds.bounds.lower_binary64_bits[axis],
            frame.point_bounds.bounds.upper_binary64_bits[axis],
            quantized[axis])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::size_t checked_snapshot_index(
    std::uint64_t value,
    const char* role) {
  if constexpr (
      std::numeric_limits<std::uint64_t>::max() >
      std::numeric_limits<std::size_t>::max()) {
    if (value > std::numeric_limits<std::size_t>::max()) {
      throw std::invalid_argument(role);
    }
  }
  return static_cast<std::size_t>(value);
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

  const ExactMortonFrame frame = exact_morton_frame(cloud);
  const ExactPointCloudAabb3& exact_point_bounds =
      frame.point_bounds;

  for (std::size_t point_index = 0U; point_index < point_count; ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    index.leaves_.push_back(
        MortonLeafRecord{
            exact_morton_code(cloud, point_id, frame),
            point_id});
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

MortonLbvhIndex MortonLbvhIndex::import_certified_snapshot(
    const CanonicalPointCloud& cloud,
    const MortonLbvhSnapshot& snapshot) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot requires a nonempty point cloud");
  }
  constexpr std::size_t maximum_size =
      std::numeric_limits<std::size_t>::max();
  if (point_count > maximum_size / 2U + 1U) {
    throw std::length_error(
        "the Morton LBVH snapshot node count overflows size_t");
  }
  const std::size_t expected_node_count = point_count * 2U - 1U;
  const std::uint64_t expected_point_count =
      static_cast<std::uint64_t>(point_count);
  const std::uint64_t expected_root_index =
      static_cast<std::uint64_t>(expected_node_count - 1U);
  if (snapshot.schema_version !=
          morton_lbvh_snapshot_schema_version ||
      snapshot.morton_bits_per_axis !=
          morton_lbvh_snapshot_morton_bits_per_axis ||
      snapshot.point_count != expected_point_count ||
      snapshot.leaves.size() != point_count ||
      snapshot.nodes.size() != expected_node_count ||
      snapshot.root_node_index != expected_root_index) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot has an invalid identity or size");
  }

  const ExactMortonFrame frame = exact_morton_frame(cloud);
  if (snapshot.root_aabb != frame.point_bounds.bounds) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot has a wrong exact root AABB");
  }

  MortonLbvhIndex index;
  index.point_count_ = point_count;
  index.leaves_ = snapshot.leaves;
  index.leaf_position_by_point_id_.assign(
      point_count, invalid_node_index);
  for (std::size_t position = 0U;
       position < point_count;
       ++position) {
    const MortonLeafRecord& leaf = index.leaves_[position];
    const std::size_t point_index =
        checked_snapshot_index(
            leaf.point_id,
            "a Morton LBVH snapshot PointId is not addressable");
    if (point_index >= point_count ||
        index.leaf_position_by_point_id_[point_index] !=
            invalid_node_index) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot is not a PointId permutation");
    }
    if (!certify_morton_code(
            cloud, leaf.point_id, leaf.morton_code, frame)) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot has an uncertified Morton code");
    }
    if (position != 0U &&
        !morton_leaf_less(index.leaves_[position - 1U], leaf)) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot is not ordered by Morton code and PointId");
    }
    index.leaf_position_by_point_id_[point_index] = position;
  }

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
      maximum_collision_size =
          std::max(maximum_collision_size, group_size);
    }
    group_begin = group_end;
  }

  index.nodes_.resize(expected_node_count);
  std::vector<unsigned char> node_seen(expected_node_count, 0U);
  std::size_t next_postorder_index = 0U;
  std::size_t observed_maximum_depth = 0U;
  const auto certify_node =
      [&cloud,
       &snapshot,
       &node_seen,
       &next_postorder_index,
       &observed_maximum_depth,
       &index](
          auto&& self,
          std::size_t node_index,
          std::size_t expected_begin,
          std::size_t expected_end,
          std::size_t depth) -> void {
    if (node_index >= snapshot.nodes.size() ||
        node_seen[node_index] != 0U) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot has a repeated or cyclic node");
    }
    node_seen[node_index] = 1U;
    observed_maximum_depth =
        std::max(observed_maximum_depth, depth);
    const MortonLbvhSnapshotNode& proposed =
        snapshot.nodes[node_index];
    const std::size_t begin = checked_snapshot_index(
        proposed.leaf_begin,
        "a Morton LBVH snapshot range is not addressable");
    const std::size_t end = checked_snapshot_index(
        proposed.leaf_end,
        "a Morton LBVH snapshot range is not addressable");
    if (begin != expected_begin || end != expected_end ||
        begin >= end || end > index.leaves_.size()) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot node has a wrong leaf range");
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (proposed.lower_point_ids[axis] >= index.point_count_ ||
          proposed.upper_point_ids[axis] >= index.point_count_) {
        throw std::invalid_argument(
            "a Morton LBVH snapshot AABB witness is outside the cloud");
      }
    }

    Node& node = index.nodes_[node_index];
    node.lower_point_ids = proposed.lower_point_ids;
    node.upper_point_ids = proposed.upper_point_ids;
    node.leaf_begin = begin;
    node.leaf_end = end;
    if (end - begin == 1U) {
      if (proposed.left_child !=
              morton_lbvh_snapshot_invalid_node_index ||
          proposed.right_child !=
              morton_lbvh_snapshot_invalid_node_index) {
        throw std::invalid_argument(
            "a Morton LBVH snapshot leaf has child indices");
      }
      const PointId point_id = index.leaves_[begin].point_id;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if (proposed.lower_point_ids[axis] != point_id ||
            proposed.upper_point_ids[axis] != point_id) {
          throw std::invalid_argument(
              "a Morton LBVH snapshot leaf has a wrong AABB witness");
        }
      }
    } else {
      if (proposed.left_child ==
              morton_lbvh_snapshot_invalid_node_index ||
          proposed.right_child ==
              morton_lbvh_snapshot_invalid_node_index) {
        throw std::invalid_argument(
            "a Morton LBVH snapshot internal node lacks topology");
      }
      const std::size_t left_child = checked_snapshot_index(
          proposed.left_child,
          "a Morton LBVH snapshot left child is not addressable");
      const std::size_t right_child = checked_snapshot_index(
          proposed.right_child,
          "a Morton LBVH snapshot right child is not addressable");
      const std::size_t exact_split =
          index.find_split(begin, end);
      if (left_child >= node_index ||
          right_child >= node_index) {
        throw std::invalid_argument(
            "a Morton LBVH snapshot has a wrong CPU radix split");
      }
      self(
          self,
          left_child,
          begin,
          exact_split,
          depth + 1U);
      self(
          self,
          right_child,
          exact_split,
          end,
          depth + 1U);
      const Node& left = index.nodes_[left_child];
      const Node& right = index.nodes_[right_child];
      node.left_child = left_child;
      node.right_child = right_child;
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
        if (proposed.lower_point_ids[axis] != expected_lower ||
            proposed.upper_point_ids[axis] != expected_upper) {
          throw std::invalid_argument(
              "a Morton LBVH snapshot has a wrong exact AABB witness");
        }
      }
    }
    if (node_index != next_postorder_index) {
      throw std::invalid_argument(
          "a Morton LBVH snapshot is not in strict postorder");
    }
    ++next_postorder_index;
  };

  const std::size_t root_index =
      checked_snapshot_index(
          snapshot.root_node_index,
          "a Morton LBVH snapshot root is not addressable");
  certify_node(
      certify_node,
      root_index,
      0U,
      point_count,
      0U);
  if (next_postorder_index != expected_node_count ||
      std::find(node_seen.begin(), node_seen.end(), 0U) !=
          node_seen.end()) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot has unreachable nodes");
  }
  const Node& root = index.nodes_[root_index];
  if (root.lower_point_ids !=
          frame.point_bounds.lower_witness_point_ids ||
      root.upper_point_ids !=
          frame.point_bounds.upper_witness_point_ids) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot root has wrong extremum witnesses");
  }

  const MortonLbvhSnapshotCounters expected_counters{
      expected_point_count,
      static_cast<std::uint64_t>(expected_node_count),
      static_cast<std::uint64_t>(observed_maximum_depth),
      static_cast<std::uint64_t>(collision_group_count),
      static_cast<std::uint64_t>(maximum_collision_size)};
  if (snapshot.proposed_counters != expected_counters) {
    throw std::invalid_argument(
        "a Morton LBVH snapshot has wrong proposed counters");
  }

  index.cloud_identity_ = cloud.identity_;
  index.root_index_ = root_index;
  index.build_counters_ = {
      point_count,
      expected_node_count,
      observed_maximum_depth,
      collision_group_count,
      maximum_collision_size};
  index.root_aabb_ = snapshot.root_aabb;
  // The import traversal above is the single exhaustive validator: it
  // certifies every leaf, node, range, split, witness, counter and postorder
  // index before the ready bit is published.  Calling validate_structure
  // here would repeat the complete O(n) traversal.
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
