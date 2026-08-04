#pragma once

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/aabb.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace morsehgp3d::hierarchy {

// Closed exact interval used only for certified support-product decisions.
// Its endpoints are rational consequences of the dyadic AABB endpoints; no
// floating-point rounding participates in these bounds.
struct ExactRationalInterval {
  exact::ExactRational lower;
  exact::ExactRational upper;

  friend bool operator==(
      const ExactRationalInterval&,
      const ExactRationalInterval&) = default;
};

// Division-free Gram/Cramer analysis of every point tuple in a product of
// three or four AABBs.  For an affinely independent support, det(G)>0 and the
// four stored quantities are the numerators of the circumcentre barycentric
// coordinates over the common denominator 2 det(G).  Only the first
// support_size entries are meaningful.
//
// If query_scaled_power is present, it encloses
//
//   det(G) ||x-p0||^2 - sum_j M_j e_j.(x-p0),
//
// for every query x in the supplied query box.  Its sign is the sign of the
// circumsphere power for every affinely independent support, without forming
// a centre or performing a division.
struct ExactHigherSupportProductAabbAnalysis {
  std::size_t support_size{};
  ExactRationalInterval gram_determinant{};
  std::array<ExactRationalInterval, 4> barycentric_numerators{};
  // For triangles only, exact continuous upper bounds of the three vertex
  // dot products.  A nonpositive entry certifies a nonacute angle throughout
  // the complete box product and is generally sharper than natural interval
  // evaluation of the corresponding Cramer numerator.
  std::optional<std::array<exact::ExactRational, 3>>
      triangle_vertex_dot_upper_bounds;
  std::optional<ExactRationalInterval> query_scaled_power;

  [[nodiscard]] bool all_supports_affinely_dependent_certified() const;

  // True only when every tuple represented by the product is either
  // affinely dependent or has at least one nonpositive circumcentre
  // barycentric coordinate.  It is therefore safe to prune the whole product
  // from the well-centred support stream.
  [[nodiscard]] bool no_well_centered_support_certified() const;

  // True only when every tuple represented by the product is affinely
  // independent and has a strictly positive circumcentre barycentric
  // coordinate at every support vertex.  False is deliberately
  // inconclusive: it does not prove that the product contains a
  // non-well-centred tuple.
  [[nodiscard]] bool all_supports_well_centered_certified() const;

  // True only when every query in the query box is strictly inside the
  // circumsphere of every affinely independent support in the product.
  // Degenerate tuples are irrelevant to the well-centred support stream.
  [[nodiscard]] bool query_strictly_inside_every_independent_sphere_certified()
      const;

  friend bool operator==(
      const ExactHigherSupportProductAabbAnalysis&,
      const ExactHigherSupportProductAabbAnalysis&) = default;
};

// support_boxes must contain exactly three or four boxes.  Omitting query_box
// computes only the affine-dependence and well-centring bounds.
[[nodiscard]] ExactHigherSupportProductAabbAnalysis
exact_higher_support_product_aabb_analysis(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    std::optional<spatial::ExactDyadicAabb3> query_box = std::nullopt);

// Operational arithmetic selected by the decision-only wrappers below.  Both
// backends evaluate the same exact interval DAG and therefore have identical
// scientific semantics; this tag is diagnostic only.
enum class ExactHigherSupportProductAabbDecisionBackend : std::uint8_t {
  bounded_dyadic_int1024 = 0U,
  arbitrary_precision_rational = 1U,
};

// Exact, deliberately one-sided decisions for one query cell against a
// product of triangle or tetrahedron supports.  The strict-inside decision
// proves that every point of the query cell is strictly inside every sphere
// induced by an affinely independent support tuple.  The outside decision
// proves that every point is outside or on every such sphere.  Inconclusive
// carries no geometric information and callers must split or otherwise fall
// back.  In particular, a zero power endpoint is never a strict witness.
enum class ExactHigherSupportProductQueryCellDecision : std::uint8_t {
  inconclusive = 0U,
  strictly_inside_every_independent_sphere = 1U,
  outside_or_boundary_every_independent_sphere = 2U,
};

// Total exact geometry classification for one fixed triangle or
// tetrahedron.  The public ordinals are independent from the implementation
// enum used by exact::analyze_circumcenter_support, so neither layer can
// silently become the wire authority of the other.
enum class ExactHigherSupportTerminalGeometryDecision : std::uint8_t {
  affinely_dependent = 0U,
  boundary_reduced = 1U,
  exterior_circumcenter = 2U,
  minimal = 3U,
};

// Decision-only fast path for traversal sites that do not need to persist the
// rich rational interval analysis.  backend, when non-null, reports whether
// the exact fixed-width dyadic kernel fitted or the arbitrary-precision
// rational implementation was used as an integral fallback.
[[nodiscard]] bool
exact_higher_support_product_no_well_centered_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend = nullptr);

[[nodiscard]] bool
exact_higher_support_product_all_well_centered_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend = nullptr);

[[nodiscard]] bool
exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3& query_box,
    ExactHigherSupportProductAabbDecisionBackend* backend = nullptr);

// This is the Morton/LBVH locality primitive used by the higher-support
// stream.  It does not assert support-enumeration locality: support tuples may
// cross arbitrary Morton-cell boundaries.  It only classifies the supplied
// query cell with respect to the complete support-box product.
[[nodiscard]] ExactHigherSupportProductQueryCellDecision
exact_higher_support_product_query_cell_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3& query_box,
    ExactHigherSupportProductAabbDecisionBackend* backend = nullptr);

// support_boxes must contain exactly three or four singleton AABBs.  The
// function is total on that domain: affine dependence is a scientific
// category, not an operational failure.  Non-singleton boxes are rejected
// because a categorical result for a mixed product would not identify one
// support.  Negative barycentric numerators take precedence over zeros, so a
// support with both signs is exterior rather than merely boundary-reduced.
[[nodiscard]] ExactHigherSupportTerminalGeometryDecision
exact_higher_support_terminal_geometry_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend = nullptr);

inline constexpr const char*
    higher_support_product_bounded_decision_proof_basis =
        "common_dyadic_exponent_exact_int1024_interval_gram_cramer_"
        "barycentric_and_scaled_power_with_124_bit_coordinate_envelope_"
        "and_integral_arbitrary_precision_rational_fallback_v1";

inline constexpr const char* higher_support_product_aabb_proof_basis =
    "exact_rational_interval_gram_cramer_barycentric_and_scaled_"
    "circumsphere_power_over_dyadic_aabb_products_v1";

inline constexpr const char*
    higher_support_terminal_geometry_decision_proof_basis =
        "singleton_exact_gram_cramer_determinant_then_negative_before_zero_"
        "circumcenter_barycentric_category_with_int1024_and_rational_"
        "fallback_v1";

}  // namespace morsehgp3d::hierarchy
