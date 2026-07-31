#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t exact_q3_gabriel_triangle_oracle_schema_version =
    1U;
inline constexpr std::size_t
    exact_q3_gabriel_triangle_oracle_maximum_point_count = 14U;
inline constexpr std::string_view exact_q3_gabriel_triangle_oracle_backend =
    "reference_cpu";
inline constexpr std::string_view exact_q3_gabriel_triangle_oracle_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_q3_gabriel_triangle_oracle_mode =
    "bounded_exhaustive_exact_q3_gabriel_triangles";
inline constexpr std::string_view
    exact_q3_gabriel_triangle_oracle_deployment_status = "architecture_only";
inline constexpr std::string_view exact_q3_gabriel_triangle_oracle_public_status =
    "not_claimed";
inline constexpr std::string_view exact_q3_gabriel_triangle_oracle_proof_basis =
    "all_canonical_triplets_local_exact_miniball_full_cloud_exact_"
    "classification_no_omitted_strict_interior_v1";

using ExactQ3GabrielTriangleIds = std::array<spatial::PointId, 3U>;
using ExactQ3GabrielFacetIds = std::array<spatial::PointId, 2U>;
using ExactQ3GabrielTriangleFacets =
    std::array<ExactQ3GabrielFacetIds, 3U>;

enum class ExactQ3GabrielMinimalSupportKind : std::uint8_t {
  pair = 2U,
  triangle = 3U,
};

// The four point-id vectors form a disjoint partition of the complete cloud:
// support is the canonical minimal support of the local triplet miniball,
// strict_interior contains every cloud point strictly inside that ball,
// extra_shell is the complete exact shell minus support, and exterior contains
// every remaining point.  In particular, extra_shell can contain a vertex of
// the emitted triplet (a right triangle) and points omitted from the triplet
// (cospherical degeneracy).
struct ExactQ3GabrielTriangleRecord {
  ExactQ3GabrielTriangleIds triangle_point_ids{};
  ExactQ3GabrielTriangleFacets facet_point_ids{};
  ExactQ3GabrielMinimalSupportKind minimal_support_kind{
      ExactQ3GabrielMinimalSupportKind::pair};
  std::vector<spatial::PointId> support_point_ids;
  std::vector<spatial::PointId> strict_interior_point_ids;
  std::vector<spatial::PointId> extra_shell_point_ids;
  std::vector<spatial::PointId> exterior_point_ids;
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};

  friend bool operator==(
      const ExactQ3GabrielTriangleRecord&,
      const ExactQ3GabrielTriangleRecord&) = default;
};

struct ExactQ3GabrielTriangleOracleCounters {
  std::size_t point_count{};
  std::size_t expected_unordered_triangle_count{};
  std::size_t enumerated_unordered_triangle_count{};
  std::size_t local_exact_miniball_build_count{};
  std::size_t full_cloud_point_classification_count{};
  std::size_t strictly_inside_classification_count{};
  std::size_t boundary_classification_count{};
  std::size_t exterior_classification_count{};
  std::size_t minimal_pair_support_triangle_count{};
  std::size_t minimal_triangle_support_triangle_count{};
  std::size_t rejected_non_gabriel_triangle_count{};
  std::size_t pre_dedup_gabriel_triangle_count{};
  std::size_t duplicate_gabriel_triangle_occurrence_count{};
  std::size_t output_gabriel_triangle_count{};
  std::size_t output_support_point_reference_count{};
  std::size_t output_strict_interior_point_reference_count{};
  std::size_t output_extra_shell_point_reference_count{};
  std::size_t output_exterior_point_reference_count{};
  std::size_t output_facet_reference_count{};
  bool complete_triangle_mass_accounting_closed{false};
  bool complete_point_classification_accounting_closed{false};
  bool canonical_sort_and_dedup_certified{false};
  bool bounded_exhaustive_reference_only{true};
  bool future_product_producer_called{false};
  bool delaunay_or_higher_order_mosaic_materialized{false};
  bool gamma2_completeness_claimed{false};
  bool silent_incidence_coverage_certified{false};
  bool hierarchy_reduction_performed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactQ3GabrielTriangleOracleCounters&,
      const ExactQ3GabrielTriangleOracleCounters&) = default;
};

// Exhaustive falsification oracle for q=3 only.  It enumerates every one of
// the C(n,3) canonical triplets for 1 <= n <= 14, builds that triplet's exact
// local miniball, and classifies every point of the cloud against it.  A
// triplet is emitted exactly when no strictly interior point is omitted from
// the triplet.
//
// The output is not Gamma_2: non-Gabriel cofaces can carry silent incidences
// required by Gamma_2 even though they are intentionally absent here.  This
// oracle performs no Delaunay construction, higher-order mosaic construction,
// hierarchy reduction, or call into a future pair/support-three producer.
struct ExactQ3GabrielTriangleOracleResult {
  static constexpr std::string_view backend =
      exact_q3_gabriel_triangle_oracle_backend;
  static constexpr std::string_view profile =
      exact_q3_gabriel_triangle_oracle_profile;
  static constexpr std::string_view mode =
      exact_q3_gabriel_triangle_oracle_mode;
  static constexpr std::string_view deployment_status =
      exact_q3_gabriel_triangle_oracle_deployment_status;
  static constexpr std::string_view public_status =
      exact_q3_gabriel_triangle_oracle_public_status;
  static constexpr std::string_view proof_basis =
      exact_q3_gabriel_triangle_oracle_proof_basis;

  std::uint32_t schema_version{
      exact_q3_gabriel_triangle_oracle_schema_version};
  std::size_t point_count{};
  contract::CanonicalId canonical_cloud_digest{};
  std::vector<ExactQ3GabrielTriangleRecord> triangles;
  ExactQ3GabrielTriangleOracleCounters counters{};
  contract::CanonicalId payload_digest{};

  [[nodiscard]] bool locally_structurally_valid() const;

  friend bool operator==(
      const ExactQ3GabrielTriangleOracleResult&,
      const ExactQ3GabrielTriangleOracleResult&) = default;
};

struct ExactQ3GabrielTriangleOracleVerification {
  bool bounded_input_certified{false};
  bool schema_and_scope_certified{false};
  bool canonical_cloud_binding_certified{false};
  bool complete_triangle_mass_certified{false};
  bool complete_exact_classification_certified{false};
  bool exact_record_payload_certified{false};
  bool canonical_sort_and_dedup_certified{false};
  bool counters_certified{false};
  bool payload_digest_certified{false};
  bool no_product_or_forbidden_global_structure_certified{false};
  bool gamma2_and_hierarchy_not_claimed_certified{false};
  bool fresh_replay_certified{false};

  [[nodiscard]] bool exact_q3_gabriel_triangle_oracle_certified() const
      noexcept {
    return bounded_input_certified && schema_and_scope_certified &&
           canonical_cloud_binding_certified &&
           complete_triangle_mass_certified &&
           complete_exact_classification_certified &&
           exact_record_payload_certified &&
           canonical_sort_and_dedup_certified && counters_certified &&
           payload_digest_certified &&
           no_product_or_forbidden_global_structure_certified &&
           gamma2_and_hierarchy_not_claimed_certified &&
           fresh_replay_certified;
  }
};

[[nodiscard]] ExactQ3GabrielTriangleOracleResult
build_exact_q3_gabriel_triangle_oracle(
    const spatial::CanonicalPointCloud& cloud);

// Rebuilds the complete bounded oracle from the cloud and trusts no observed
// result field.  A mismatch leaves the final certificate false rather than
// repairing, normalizing, or partially accepting the observed payload.
[[nodiscard]] ExactQ3GabrielTriangleOracleVerification
verify_exact_q3_gabriel_triangle_oracle(
    const spatial::CanonicalPointCloud& cloud,
    const ExactQ3GabrielTriangleOracleResult& observed);

}  // namespace morsehgp3d::hierarchy
