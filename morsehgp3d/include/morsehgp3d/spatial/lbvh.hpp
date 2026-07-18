#pragma once

#include "morsehgp3d/spatial/query.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu {
class SpatialLbvhContext;
}

namespace morsehgp3d::spatial {
class CanonicalPointCloud;
class MortonLbvhIndex;
}

namespace morsehgp3d::hierarchy {
struct K1ExactBoruvkaResult;
struct K1BoruvkaVerification;
[[nodiscard]] K1ExactBoruvkaResult build_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud);
[[nodiscard]] K1BoruvkaVerification verify_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& result);
}

namespace morsehgp3d::spatial {

struct MortonLeafRecord {
  std::uint64_t morton_code;
  PointId point_id;

  friend bool operator==(const MortonLeafRecord&, const MortonLeafRecord&) = default;
};

// Bounds are selected input dyadics, not rounded arithmetic results.  Their
// closed intervals therefore enclose every descendant exactly, with zero
// inward error even at the finite binary64 extrema.
struct ExactDyadicAabb3 {
  std::array<std::uint64_t, 3> lower_binary64_bits;
  std::array<std::uint64_t, 3> upper_binary64_bits;

  friend bool operator==(
      const ExactDyadicAabb3&,
      const ExactDyadicAabb3&) = default;
};

struct MortonLbvhBuildCounters {
  std::size_t point_count{0U};
  std::size_t node_count{0U};
  std::size_t maximum_depth{0U};
  std::size_t morton_collision_group_count{0U};
  std::size_t maximum_morton_collision_size{0U};

  friend bool operator==(
      const MortonLbvhBuildCounters&,
      const MortonLbvhBuildCounters&) = default;
};

enum class LbvhTraversalOrder : std::uint8_t {
  near_first,
  far_first,
};

class MortonLbvhIndex {
 public:
  static constexpr std::size_t morton_bits_per_axis = 21U;

  [[nodiscard]] static MortonLbvhIndex build(
      const CanonicalPointCloud& cloud);

  MortonLbvhIndex(const MortonLbvhIndex&) = delete;
  MortonLbvhIndex& operator=(const MortonLbvhIndex&) = delete;
  MortonLbvhIndex(MortonLbvhIndex&& other) noexcept;
  MortonLbvhIndex& operator=(MortonLbvhIndex&& other) noexcept;

  [[nodiscard]] bool ready() const noexcept {
    return structure_complete_ && cloud_identity_ != nullptr;
  }
  [[nodiscard]] bool validated_for(
      const CanonicalPointCloud& cloud) const noexcept {
    return ready() && cloud_identity_ == cloud.identity_;
  }
  [[nodiscard]] std::span<const MortonLeafRecord> leaves() const & noexcept {
    return ready() ? std::span<const MortonLeafRecord>{leaves_}
                   : std::span<const MortonLeafRecord>{};
  }
  [[nodiscard]] std::span<const MortonLeafRecord> leaves() const && = delete;
  [[nodiscard]] const MortonLbvhBuildCounters& build_counters() const & {
    if (!ready()) {
      throw std::logic_error("a moved-from Morton LBVH has no counters");
    }
    return build_counters_;
  }
  [[nodiscard]] const MortonLbvhBuildCounters& build_counters() const && = delete;
  [[nodiscard]] const ExactDyadicAabb3& root_aabb() const & {
    if (!ready()) {
      throw std::logic_error("a moved-from Morton LBVH has no root AABB");
    }
    return root_aabb_;
  }
  [[nodiscard]] const ExactDyadicAabb3& root_aabb() const && = delete;

 private:
  static constexpr std::size_t invalid_node_index =
      static_cast<std::size_t>(-1);

  struct Node {
    std::array<PointId, 3> lower_point_ids{};
    std::array<PointId, 3> upper_point_ids{};
    std::size_t left_child{invalid_node_index};
    std::size_t right_child{invalid_node_index};
    std::size_t leaf_begin{0U};
    std::size_t leaf_end{0U};

    [[nodiscard]] bool is_leaf() const noexcept {
      return left_child == invalid_node_index;
    }
  };

  MortonLbvhIndex() = default;

  [[nodiscard]] std::size_t build_range(
      const CanonicalPointCloud& cloud,
      std::size_t begin,
      std::size_t end,
      std::size_t depth);
  [[nodiscard]] std::size_t find_split(
      std::size_t begin,
      std::size_t end) const;
  void validate_structure(const CanonicalPointCloud& cloud) const;
  [[nodiscard]] std::size_t eligible_count_in_node(
      const Node& node,
      const ExclusionSet& exclusions) const;
  [[nodiscard]] exact::ExactLevel minimum_squared_distance_to_node(
      const CanonicalPointCloud& cloud,
      std::size_t node_index,
      const exact::ExactRational3& query) const;
  [[nodiscard]] exact::ExactLevel maximum_squared_distance_to_node(
      const CanonicalPointCloud& cloud,
      std::size_t node_index,
      const exact::ExactRational3& query) const;
  std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity_;
  std::size_t point_count_{0U};
  std::vector<MortonLeafRecord> leaves_;
  std::vector<std::size_t> leaf_position_by_point_id_;
  std::vector<Node> nodes_;
  std::size_t root_index_{invalid_node_index};
  MortonLbvhBuildCounters build_counters_;
  ExactDyadicAabb3 root_aabb_{};
  bool structure_complete_{false};

  friend TopKPartition lbvh_top_k(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      LbvhTraversalOrder traversal_order);
  friend ClosedBallPartition lbvh_closed_ball(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius,
      LbvhTraversalOrder traversal_order);
  friend hierarchy::K1ExactBoruvkaResult
  hierarchy::build_exact_lbvh_boruvka(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);
  friend hierarchy::K1BoruvkaVerification
  hierarchy::verify_exact_lbvh_boruvka(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const hierarchy::K1ExactBoruvkaResult& result);
  friend class gpu::SpatialLbvhContext;
};

[[nodiscard]] TopKPartition lbvh_top_k(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

[[nodiscard]] TopKPartition lbvh_nearest(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

[[nodiscard]] ClosedBallPartition lbvh_closed_ball(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const exact::ExactLevel& squared_radius,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

}  // namespace morsehgp3d::spatial
