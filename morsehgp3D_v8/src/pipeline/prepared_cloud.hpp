#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "core/types.hpp"

namespace mhgp8 {

// Half-open ranges into the original, immutable input order. Not Morton IDs.
struct Range {
  std::size_t first{};
  std::size_t last{};
  [[nodiscard]] std::size_t size() const noexcept { return last - first; }
};

struct CloudWork {
  u64 coordinate_copies{};
  u64 validation_points{};
  // Sorting comparisons, preserving the legacy rectangle convention.
  u64 uniqueness_comparisons{};
  // Adjacent equality checks are paid separately: n-1 on success.
  u64 uniqueness_adjacent_tests{};
  u64 range_tree_leaf_visits{};
  // Useful nodes only; slot zero of the iterative tree is not a node.
  u64 range_tree_nodes{};
  u64 range_tree_merges{};
};

struct CloudRangeBounds {
  Box3 box{};
  // Nodes consumed by this query, not a mutation of shared preparation work.
  u64 node_visits{};
  u64 steps{};  // Loop levels, including levels that consume no node.
};

class PreparedCloud;
using CloudPtr = std::shared_ptr<const PreparedCloud>;

// Copies into private storage before checking uniqueness. A move of the
// caller's vector would not revoke mutable aliases and is never used here.
// Rejects an empty cloud, any coordinate outside [0, coordinate_limit] and
// duplicate sites, always without perturbation.
[[nodiscard]] CloudPtr prepare_cloud(std::span<const Point3> points);

class PreparedCloud final {
 public:
  PreparedCloud(const PreparedCloud&) = delete;
  PreparedCloud& operator=(const PreparedCloud&) = delete;
  PreparedCloud(PreparedCloud&&) = delete;
  PreparedCloud& operator=(PreparedCloud&&) = delete;
  [[nodiscard]] std::span<const Point3> points() const noexcept { return points_; }
  [[nodiscard]] const CloudWork& work() const noexcept { return work_; }

  // Nonempty, half-open ranges in original input order. The iterative tree
  // answers in O(log n) work without copying or rescanning any coordinates.
  [[nodiscard]] CloudRangeBounds bounds(Range range) const;

  // Vector capacities only: excludes this object, shared_ptr metadata and
  // temporary uniqueness keys (released before the tree is constructed).
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  PreparedCloud() = default;
  friend CloudPtr prepare_cloud(std::span<const Point3>);
  std::vector<Point3> points_;
  // 2*n slots, with leaves at [n,2*n), root at 1 and slot zero unused.
  // This tree indexes original order, not a spatial/WSPD permutation.
  std::vector<Box3> range_tree_;
  CloudWork work_{};
};

}  // namespace mhgp8
