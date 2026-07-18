#pragma once

#include "morsehgp3d/spatial/brute_force.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class SpatialReferenceContextState;
}  // namespace detail

// The projection is used only to order diagnostic GPU proposals.  In
// particular, rounded, underflowed, and overflow-clamped coordinates never
// become scientific inputs to the exact spatial decision.
enum class QueryCoordinateProjection : std::uint8_t {
  exact,
  rounded,
  underflow,
  overflow_clamped,
};

struct SpatialReferenceAudit {
  static constexpr const char* proposal_semantics =
      "non_certifying_fp64";
  static constexpr const char* decision_semantics =
      "cpu_exact_all_points";

  std::size_t gpu_input_point_count{0U};
  std::size_t gpu_output_record_count{0U};
  std::size_t gpu_unique_point_id_count{0U};
  std::size_t gpu_finite_distance_proposal_count{0U};
  std::size_t gpu_infinite_distance_proposal_count{0U};
  std::size_t gpu_nan_distance_proposal_count{0U};
  std::size_t gpu_launch_count{0U};
  std::size_t cpu_exact_distance_evaluation_count{0U};
  std::uint64_t buffer_epoch{0U};
  std::uint64_t proposal_digest_fnv1a{0U};
  std::array<std::uint64_t, 3> projected_query_bits{};
  std::array<QueryCoordinateProjection, 3> query_projection{};
  bool all_points_enumerated{false};
  bool cpu_exact_recertification_complete{false};

  friend bool operator==(
      const SpatialReferenceAudit&,
      const SpatialReferenceAudit&) = default;
};

struct SpatialReferenceTopKResult {
  spatial::TopKPartition exact_partition;
  SpatialReferenceAudit audit;
};

struct SpatialReferenceClosedBallResult {
  spatial::ClosedBallPartition exact_partition;
  SpatialReferenceAudit audit;
};

// One context is tied to one canonical PointId namespace.  CUDA owns only a
// persistent proposal workspace; callers must still provide the matching cloud
// so the CPU can recertify every admissible point exactly. Rational query
// coordinates outside finite binary64 range are visibly clamped only in the
// non-certifying proposal; the exact CPU decision keeps the original query.
class SpatialReferenceContext final {
 public:
  explicit SpatialReferenceContext(
      const spatial::CanonicalPointCloud& cloud);
  ~SpatialReferenceContext() noexcept;

  SpatialReferenceContext(SpatialReferenceContext&&) noexcept;
  SpatialReferenceContext& operator=(SpatialReferenceContext&&) noexcept;

  SpatialReferenceContext(const SpatialReferenceContext&) = delete;
  SpatialReferenceContext& operator=(const SpatialReferenceContext&) = delete;

  [[nodiscard]] SpatialReferenceTopKResult top_k(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const spatial::ExclusionSet& exclusions);

  [[nodiscard]] SpatialReferenceTopKResult nearest(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const spatial::ExclusionSet& exclusions);

  [[nodiscard]] SpatialReferenceClosedBallResult closed_ball(
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius);

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;
  [[nodiscard]] SpatialReferenceAudit run_proposal(
      const exact::ExactRational3& query);

  std::shared_ptr<detail::SpatialReferenceContextState> state_;
  std::shared_ptr<const void> cloud_identity_;
  std::size_t point_count_{0U};
  std::vector<std::uint64_t> coordinate_bits_;
};

}  // namespace morsehgp3d::gpu
