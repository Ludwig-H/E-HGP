#pragma once

#include "morsehgp3d/gpu/spatial_bounds.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class SpatialLbvhContextState;
class SpatialLbvhHostState;
}  // namespace detail

// The device owns the immutable node topology, AABBs, traversal stack and
// cover buffers.  A prune remains only a proposal until the host has rebuilt
// the cover and checked the corresponding rational AABB bound exactly.
struct SpatialLbvhAudit {
  static constexpr const char* proposal_semantics =
      "gpu_resident_lbvh_strict_exterior_cover";
  static constexpr const char* decision_semantics =
      "cpu_exact_cover_and_leaf_recertification";

  // These extents describe the immutable logical device image owned by the
  // context. gpu_launch_count says whether a query materialized or touched
  // that image; unsupported-range fallback retains the extents while
  // reporting zero launches and zero cover records.
  std::size_t resident_node_count{0U};
  std::size_t resident_point_count{0U};
  std::size_t gpu_output_capacity{0U};
  std::size_t gpu_output_cover_record_count{0U};
  std::size_t gpu_prune_proposal_count{0U};
  std::size_t gpu_candidate_leaf_count{0U};
  std::size_t gpu_launch_count{0U};
  std::size_t traversed_node_count{0U};
  std::size_t internal_node_expansion_count{0U};
  std::size_t cpu_exact_aabb_bound_evaluation_count{0U};
  std::size_t cpu_exact_prune_recertification_count{0U};
  std::size_t certified_pruned_subtree_count{0U};
  std::size_t certified_pruned_point_count{0U};
  std::size_t candidate_point_count{0U};
  std::size_t cpu_exact_candidate_distance_evaluation_count{0U};
  std::size_t cpu_exact_seed_distance_evaluation_count{0U};
  std::size_t excluded_candidate_point_count{0U};
  std::size_t certified_pruned_eligible_point_count{0U};
  std::size_t unsupported_range_fallback_count{0U};
  std::uint64_t buffer_epoch{0U};
  std::uint64_t proposal_digest_fnv1a{0U};
  std::array<std::uint64_t, 3> query_lower_bits{};
  std::array<std::uint64_t, 3> query_upper_bits{};
  std::array<DirectedEnclosureStatus, 3> query_enclosure{};
  std::uint64_t cutoff_lower_bits{0U};
  std::uint64_t cutoff_upper_bits{0U};
  DirectedEnclosureStatus cutoff_enclosure{DirectedEnclosureStatus::exact};
  std::optional<exact::ExactLevel> minimum_certified_strict_margin;
  std::optional<exact::ExactLevel> top_k_seed_squared_cutoff;
  bool cover_antichain_complete{false};
  bool point_partition_complete{false};
  bool cpu_exact_recertification_complete{false};
  bool exact_partition_complete{false};

  friend bool operator==(const SpatialLbvhAudit&, const SpatialLbvhAudit&) =
      default;
};

struct SpatialLbvhCoverResult {
  // Both vectors are sorted by canonical PointId.  Their disjoint union is X.
  // Every certified_exterior_point_id lies strictly beyond the supplied
  // cutoff; candidate_point_ids require an exact leaf decision.
  std::vector<spatial::PointId> candidate_point_ids;
  std::vector<spatial::PointId> certified_exterior_point_ids;
  SpatialLbvhAudit audit;
};

struct SpatialLbvhClosedBallResult {
  spatial::ClosedBallPartition exact_partition;
  SpatialLbvhAudit audit;
};

struct SpatialLbvhTopKResult {
  spatial::TopKPartition exact_partition;
  SpatialLbvhAudit audit;
};

class SpatialLbvhContext final {
 public:
  SpatialLbvhContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);
  ~SpatialLbvhContext() noexcept;

  SpatialLbvhContext(SpatialLbvhContext&&) noexcept;
  SpatialLbvhContext& operator=(SpatialLbvhContext&&) noexcept;

  SpatialLbvhContext(const SpatialLbvhContext&) = delete;
  SpatialLbvhContext& operator=(const SpatialLbvhContext&) = delete;

  [[nodiscard]] SpatialLbvhCoverResult cover_strict_exterior(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_cutoff);

  [[nodiscard]] SpatialLbvhClosedBallResult closed_ball(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius);

  [[nodiscard]] SpatialLbvhTopKResult top_k(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const spatial::ExclusionSet& exclusions);

  [[nodiscard]] SpatialLbvhTopKResult nearest(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const spatial::ExclusionSet& exclusions);

  [[nodiscard]] std::size_t node_count() const noexcept;
  [[nodiscard]] std::size_t point_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  std::shared_ptr<detail::SpatialLbvhContextState> state_;
  std::unique_ptr<detail::SpatialLbvhHostState> host_;
};

}  // namespace morsehgp3d::gpu
