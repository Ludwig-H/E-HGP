#include "pipeline/prepared_cloud.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {

[[nodiscard]] Box3 unite(const Box3& left, const Box3& right) noexcept {
  return {{std::min(left.low.x, right.low.x),
           std::min(left.low.y, right.low.y),
           std::min(left.low.z, right.low.z)},
          {std::max(left.high.x, right.high.x),
           std::max(left.high.y, right.high.y),
           std::max(left.high.z, right.high.z)}};
}

[[nodiscard]] u64 work_size(std::size_t size) {
  if (std::cmp_greater(size, std::numeric_limits<u64>::max())) {
    throw std::overflow_error("mhgp8 cloud size exceeds work counter range");
  }
  return static_cast<u64>(size);
}

[[nodiscard]] std::size_t capacity_bytes(std::size_t capacity,
                                        std::size_t element_bytes) {
  if (capacity > std::numeric_limits<std::size_t>::max() / element_bytes) {
    throw std::overflow_error("mhgp8 cloud capacity byte count overflow");
  }
  return capacity * element_bytes;
}

}  // namespace

CloudPtr prepare_cloud(std::span<const Point3> points) {
  auto result = std::shared_ptr<PreparedCloud>(new PreparedCloud());
  // Certification only sees this copy. Even a const span can refer to
  // storage with mutable aliases owned by its caller.
  result->points_.assign(points.begin(), points.end());
  const auto size = result->points_.size();
  counter_add(result->work_.coordinate_copies, work_size(size));
  if (size == 0) {
    throw std::invalid_argument("mhgp8 cloud requires at least one site");
  }

  {
    std::vector<u64> keys;
    keys.reserve(size);
    for (const auto& point : result->points_) {
      counter_add(result->work_.validation_points);
      // Explicit range refusal: the storage type admits values the proofs do
      // not cover. Never a clamp, a modulo or a perturbation.
      for (std::size_t axis = 0; axis < 3; ++axis) {
        if (point[axis] < 0 || point[axis] > coordinate_limit) {
          throw std::invalid_argument(
              "mhgp8 cloud requires coordinates in [0, 2^18)");
        }
      }
      // Three disjoint coordinate_bits fields (54 bits): distinct sites have
      // distinct keys, so adjacent equality after sorting is exact uniqueness.
      keys.push_back((static_cast<u64>(point.x) << (2 * coordinate_bits)) |
                     (static_cast<u64>(point.y) << coordinate_bits) |
                     static_cast<u64>(point.z));
    }
    std::sort(keys.begin(), keys.end(), [&result](u64 left, u64 right) {
      counter_add(result->work_.uniqueness_comparisons);
      return left < right;
    });
    for (std::size_t index = 1; index < keys.size(); ++index) {
      counter_add(result->work_.uniqueness_adjacent_tests);
      if (keys[index - 1] == keys[index]) {
        throw std::invalid_argument("mhgp8 cloud requires distinct sites");
      }
    }
  }  // Release the uniqueness keys before allocating the range tree.

  if (size > std::numeric_limits<std::size_t>::max() / 2 ||
      size > result->range_tree_.max_size() / 2) {
    throw std::length_error("mhgp8 cloud range tree exceeds addressable storage");
  }
  result->range_tree_.resize(size * 2);
  for (std::size_t index = 0; index < size; ++index) {
    result->range_tree_[size + index] = singleton_box(result->points_[index]);
    counter_add(result->work_.range_tree_leaf_visits);
    counter_add(result->work_.range_tree_nodes);
  }
  for (std::size_t index = size - 1; index != 0; --index) {
    result->range_tree_[index] = unite(result->range_tree_[index * 2],
                                       result->range_tree_[index * 2 + 1]);
    counter_add(result->work_.range_tree_merges);
    counter_add(result->work_.range_tree_nodes);
  }
  static_cast<void>(result->retained_bytes());
  return result;
}

CloudRangeBounds PreparedCloud::bounds(Range range) const {
  if (range.first >= range.last || range.last > points_.size()) {
    throw std::invalid_argument("mhgp8 cloud bounds require a nonempty in-range interval");
  }
  auto first = range.first + points_.size();
  auto last = range.last + points_.size();
  CloudRangeBounds result;
  bool has_box = false;
  const auto consume = [this, &result, &has_box](std::size_t index) {
    counter_add(result.node_visits);
    result.box = has_box ? unite(result.box, range_tree_[index]) : range_tree_[index];
    has_box = true;
  };
  while (first < last) {
    counter_add(result.steps);
    if ((first & 1U) != 0U) {
      consume(first);
      ++first;
    }
    if ((last & 1U) != 0U) {
      --last;
      consume(last);
    }
    first /= 2;
    last /= 2;
  }
  return result;
}

std::size_t PreparedCloud::retained_bytes() const {
  const auto point_bytes = capacity_bytes(points_.capacity(), sizeof(Point3));
  const auto tree_bytes = capacity_bytes(range_tree_.capacity(), sizeof(Box3));
  if (tree_bytes > std::numeric_limits<std::size_t>::max() - point_bytes) {
    throw std::overflow_error("mhgp8 cloud retained byte count overflow");
  }
  return point_bytes + tree_bytes;
}

}  // namespace mhgp8
