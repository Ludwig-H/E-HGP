#pragma once

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/aabb.hpp"

#include <array>
#include <cstddef>
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

inline constexpr const char* higher_support_product_aabb_proof_basis =
    "exact_rational_interval_gram_cramer_barycentric_and_scaled_"
    "circumsphere_power_over_dyadic_aabb_products_v1";

}  // namespace morsehgp3d::hierarchy
