#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::spatial {

// Exclusions are part of a query, not of the canonical point cloud.  The
// configured bound is recorded so a result can be audited against the run's
// m_star contract instead of only against the compile-time maximum.
class ExclusionSet {
 public:
  static constexpr std::size_t max_size = 9U;

  ExclusionSet(const ExclusionSet&) = default;
  ExclusionSet& operator=(const ExclusionSet& other);
  ExclusionSet(ExclusionSet&& other) noexcept;
  ExclusionSet& operator=(ExclusionSet&& other) noexcept;

  [[nodiscard]] static ExclusionSet from_ids(
      std::span<const PointId> ids,
      const CanonicalPointCloud& cloud,
      std::size_t run_m_star);

  [[nodiscard]] std::size_t point_count() const noexcept {
    return cloud_identity_ != nullptr ? point_count_ : 0U;
  }
  [[nodiscard]] std::size_t run_m_star() const noexcept {
    return cloud_identity_ != nullptr ? run_m_star_ : 0U;
  }
  [[nodiscard]] std::span<const PointId> ids() const & noexcept {
    return cloud_identity_ != nullptr ? std::span<const PointId>{ids_}
                                      : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> ids() const && = delete;
  [[nodiscard]] bool contains(PointId id) const noexcept {
    return cloud_identity_ != nullptr &&
           std::binary_search(ids_.begin(), ids_.end(), id);
  }
  [[nodiscard]] bool validated_for(const CanonicalPointCloud& cloud) const noexcept {
    return cloud_identity_ != nullptr && cloud_identity_ == cloud.identity_;
  }

 private:
  ExclusionSet(
      std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity,
      std::size_t point_count,
      std::size_t run_m_star,
      std::vector<PointId> ids);

  std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity_;
  std::size_t point_count_;
  std::size_t run_m_star_;
  std::vector<PointId> ids_;
};

struct ExactNeighbor {
  PointId point_id;
  exact::ExactLevel squared_distance;
};

enum class SpatialQueryMethod : std::uint8_t {
  brute_force,
  morton_lbvh,
};

// Scientific partitions are method-independent.  These counters expose which
// work was certified instead of pretending that an accelerated query evaluated
// every point distance.
struct SpatialQueryCounters {
  SpatialQueryMethod method{SpatialQueryMethod::brute_force};
  std::size_t node_visit_count{0U};
  std::size_t internal_node_expansion_count{0U};
  std::size_t exact_aabb_bound_evaluation_count{0U};
  std::size_t pruned_subtree_count{0U};
  std::size_t bulk_interior_subtree_count{0U};
  std::size_t pruned_eligible_point_count{0U};
  std::size_t bulk_interior_point_count{0U};
  std::size_t excluded_point_count{0U};
  std::size_t exact_point_distance_evaluation_count{0U};
  std::optional<exact::ExactLevel> minimum_strict_pruning_margin;

  friend bool operator==(
      const SpatialQueryCounters&,
      const SpatialQueryCounters&) = default;
};

class MortonLbvhIndex;
enum class LbvhTraversalOrder : std::uint8_t;
class TopKPartition;
class ClosedBallPartition;

class TopKPartition {
 public:
  TopKPartition(const TopKPartition&) = default;
  TopKPartition& operator=(const TopKPartition& other);
  TopKPartition(TopKPartition&& other) noexcept;
  TopKPartition& operator=(TopKPartition&& other) noexcept;

  [[nodiscard]] std::size_t requested_rank() const noexcept {
    return complete_ ? requested_rank_ : 0U;
  }
  [[nodiscard]] const exact::ExactLevel& cutoff_squared_distance() const & {
    if (!complete_) {
      throw std::logic_error("a moved-from top-k partition has no cutoff");
    }
    return cutoff_squared_distance_;
  }
  [[nodiscard]] const exact::ExactLevel& cutoff_squared_distance() const && = delete;
  [[nodiscard]] std::span<const ExactNeighbor> strict_below() const & noexcept {
    return complete_ ? std::span<const ExactNeighbor>{strict_below_}
                     : std::span<const ExactNeighbor>{};
  }
  [[nodiscard]] std::span<const ExactNeighbor> strict_below() const && = delete;
  [[nodiscard]] std::span<const PointId> cutoff_shell_ids() const & noexcept {
    return complete_ ? std::span<const PointId>{cutoff_shell_ids_}
                     : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> cutoff_shell_ids() const && = delete;
  [[nodiscard]] std::span<const PointId> canonical_choice_ids() const & noexcept {
    return complete_ ? std::span<const PointId>{canonical_choice_ids_}
                     : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> canonical_choice_ids() const && = delete;
  [[nodiscard]] std::size_t eligible_point_count() const noexcept {
    return complete_ ? eligible_point_count_ : 0U;
  }
  [[nodiscard]] std::size_t distance_evaluation_count() const noexcept {
    return complete_ ? counters_.exact_point_distance_evaluation_count : 0U;
  }
  [[nodiscard]] const SpatialQueryCounters& query_counters() const & {
    if (!complete_) {
      throw std::logic_error("a moved-from top-k partition has no counters");
    }
    return counters_;
  }
  [[nodiscard]] const SpatialQueryCounters& query_counters() const && = delete;
  [[nodiscard]] bool validated_for(const CanonicalPointCloud& cloud) const noexcept {
    return complete_ && cloud_identity_ != nullptr &&
           cloud_identity_ == cloud.identity_;
  }
  [[nodiscard]] bool shell_complete() const noexcept { return complete_; }

 private:
  TopKPartition(
      const CanonicalPointCloud& cloud,
      std::size_t requested_rank,
      exact::ExactLevel cutoff_squared_distance,
      std::vector<ExactNeighbor> strict_below,
      std::vector<PointId> cutoff_shell_ids,
      std::vector<PointId> canonical_choice_ids,
      std::size_t eligible_point_count,
      SpatialQueryCounters counters);

  [[nodiscard]] static TopKPartition from_evaluated_neighbors(
      const CanonicalPointCloud& cloud,
      std::size_t requested_rank,
      std::vector<ExactNeighbor> evaluated_neighbors,
      std::size_t eligible_point_count,
      SpatialQueryCounters counters);

  friend TopKPartition brute_force_top_k(
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions);
  friend TopKPartition brute_force_nearest(
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const ExclusionSet& exclusions);
  friend TopKPartition lbvh_top_k(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      LbvhTraversalOrder traversal_order);
  friend TopKPartition lbvh_nearest(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const ExclusionSet& exclusions,
      LbvhTraversalOrder traversal_order);

  bool complete_;
  std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity_;
  std::size_t point_count_;
  std::size_t requested_rank_;
  exact::ExactLevel cutoff_squared_distance_;
  std::vector<ExactNeighbor> strict_below_;
  std::vector<PointId> cutoff_shell_ids_;
  std::vector<PointId> canonical_choice_ids_;
  std::size_t eligible_point_count_;
  SpatialQueryCounters counters_;
};

// This is deliberately a global closed-ball partition.  Applying exclusions
// would produce a filtered query rank, which is not the closed rank of a Morse
// event and therefore must not share this result type.
class ClosedBallPartition {
 public:
  ClosedBallPartition(const ClosedBallPartition&) = default;
  ClosedBallPartition& operator=(const ClosedBallPartition& other);
  ClosedBallPartition(ClosedBallPartition&& other) noexcept;
  ClosedBallPartition& operator=(ClosedBallPartition&& other) noexcept;

  [[nodiscard]] const exact::ExactLevel& squared_radius() const & {
    if (!complete_) {
      throw std::logic_error("a moved-from closed-ball partition has no radius");
    }
    return squared_radius_;
  }
  [[nodiscard]] const exact::ExactLevel& squared_radius() const && = delete;
  [[nodiscard]] std::span<const PointId> interior_ids() const & noexcept {
    return complete_ ? std::span<const PointId>{interior_ids_}
                     : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> interior_ids() const && = delete;
  [[nodiscard]] std::span<const PointId> shell_ids() const & noexcept {
    return complete_ ? std::span<const PointId>{shell_ids_}
                     : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> shell_ids() const && = delete;
  [[nodiscard]] std::span<const PointId> exterior_ids() const & noexcept {
    return complete_ ? std::span<const PointId>{exterior_ids_}
                     : std::span<const PointId>{};
  }
  [[nodiscard]] std::span<const PointId> exterior_ids() const && = delete;
  [[nodiscard]] std::size_t closed_rank() const noexcept {
    return complete_ ? closed_rank_ : 0U;
  }
  [[nodiscard]] std::size_t evaluation_count() const noexcept {
    return complete_ ? evaluation_count_ : 0U;
  }
  [[nodiscard]] std::size_t distance_evaluation_count() const noexcept {
    return complete_ ? counters_.exact_point_distance_evaluation_count : 0U;
  }
  [[nodiscard]] const SpatialQueryCounters& query_counters() const & {
    if (!complete_) {
      throw std::logic_error("a moved-from closed-ball partition has no counters");
    }
    return counters_;
  }
  [[nodiscard]] const SpatialQueryCounters& query_counters() const && = delete;
  [[nodiscard]] bool validated_for(const CanonicalPointCloud& cloud) const noexcept {
    return complete_ && cloud_identity_ != nullptr &&
           cloud_identity_ == cloud.identity_;
  }
  [[nodiscard]] bool partition_complete() const noexcept { return complete_; }

 private:
  ClosedBallPartition(
      const CanonicalPointCloud& cloud,
      exact::ExactLevel squared_radius,
      std::vector<PointId> interior_ids,
      std::vector<PointId> shell_ids,
      std::vector<PointId> exterior_ids,
      SpatialQueryCounters counters);

  friend ClosedBallPartition brute_force_closed_ball(
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius);
  friend ClosedBallPartition lbvh_closed_ball(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius,
      LbvhTraversalOrder traversal_order);

  bool complete_;
  std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity_;
  exact::ExactLevel squared_radius_;
  std::vector<PointId> interior_ids_;
  std::vector<PointId> shell_ids_;
  std::vector<PointId> exterior_ids_;
  std::size_t closed_rank_;
  std::size_t evaluation_count_;
  SpatialQueryCounters counters_;
};

}  // namespace morsehgp3d::spatial
