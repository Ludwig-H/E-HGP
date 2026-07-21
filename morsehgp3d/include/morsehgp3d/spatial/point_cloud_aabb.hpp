#pragma once

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::spatial {

struct ExactPointCloudAabbAudit {
  std::size_t point_count{};
  std::uint64_t exact_coordinate_evaluation_count{};
  std::uint64_t exact_extremum_comparison_count{};

  friend bool operator==(
      const ExactPointCloudAabbAudit&,
      const ExactPointCloudAabbAudit&) = default;
};

struct ExactPointCloudAabb3 {
  ExactDyadicAabb3 bounds{};
  std::array<PointId, 3> lower_witness_point_ids{};
  std::array<PointId, 3> upper_witness_point_ids{};
  ExactPointCloudAabbAudit audit;

  friend bool operator==(
      const ExactPointCloudAabb3&,
      const ExactPointCloudAabb3&) = default;
};

enum class StrictDyadicPaddingDecision : std::uint8_t {
  complete,
  unsupported_finite_binary64_range,
};

[[nodiscard]] std::string_view to_string(StrictDyadicPaddingDecision decision);

struct StrictlyPaddedDyadicAabb3Certificate {
  static constexpr std::string_view schema =
      "morsehgp3d.phase8.strictly_padded_dyadic_aabb.v1";
  static constexpr std::string_view padding_rule =
      "adjacent_finite_binary64_per_face_v1";
  static constexpr std::string_view proof_basis =
      "exact_axis_extrema_strict_padding_convex_interior_v1";
  static constexpr std::string_view scope =
      "canonical_point_cloud_strict_convex_hull_interior_box_only";

  ExactPointCloudAabb3 exact_site_aabb;
  ExactDyadicAabb3 omega{};
  std::array<exact::ExactRational, 3> lower_padding;
  std::array<exact::ExactRational, 3> upper_padding;
  bool all_sites_strictly_inside_certified{false};
  bool convex_hull_strictly_inside_certified{false};

  friend bool operator==(
      const StrictlyPaddedDyadicAabb3Certificate&,
      const StrictlyPaddedDyadicAabb3Certificate&) = default;
};

struct StrictlyPaddedDyadicAabb3Result {
  StrictDyadicPaddingDecision decision{
      StrictDyadicPaddingDecision::unsupported_finite_binary64_range};
  // Bit d reports that axis d has no finite outward neighbour on that side.
  std::uint8_t unavailable_lower_axis_mask{};
  std::uint8_t unavailable_upper_axis_mask{};
  ExactPointCloudAabbAudit audit;
  std::optional<StrictlyPaddedDyadicAabb3Certificate> certificate;

  friend bool operator==(
      const StrictlyPaddedDyadicAabb3Result&,
      const StrictlyPaddedDyadicAabb3Result&) = default;
};

struct StrictlyPaddedDyadicAabb3Verification {
  bool audit_certified{false};
  bool decision_certified{false};
  bool failure_masks_certified{false};
  bool payload_shape_certified{false};
  bool exact_extrema_certified{false};
  bool extremum_witnesses_certified{false};
  bool finite_adjacent_padding_certified{false};
  bool exact_positive_padding_certified{false};
  bool all_sites_strictly_inside_certified{false};
  bool convex_hull_strictly_inside_certified{false};
  bool result_certified{false};
};

// Exact O(n) coordinate scan with the least PointId retained on every tied
// extremum.  This is the unpadded AABB shared by the LBVH and Phase 8.
[[nodiscard]] ExactPointCloudAabb3 build_exact_point_cloud_aabb(
    const CanonicalPointCloud& cloud);

// Construct the six immediately adjacent finite binary64 faces.  If at least
// one outward neighbour would be infinite, the result reports every affected
// axis and publishes no certificate or clipping box.
[[nodiscard]] StrictlyPaddedDyadicAabb3Result
build_strictly_padded_dyadic_aabb(const CanonicalPointCloud& cloud);

// Freshly replay extrema, witnesses, finite neighbours and every strict point
// inequality.  Convexity of the open AABB then certifies conv(X) in int(Omega).
[[nodiscard]] StrictlyPaddedDyadicAabb3Verification
verify_strictly_padded_dyadic_aabb(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& result);

}  // namespace morsehgp3d::spatial
