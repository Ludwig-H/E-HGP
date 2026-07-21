#pragma once

#include "morsehgp3d/exact/plane.hpp"
#include "morsehgp3d/spatial/aabb.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::spatial {

enum class HPolytopeConstraintDomain : std::uint8_t {
  generic_affine,
  power_owner_competitor,
  canonical_parent_cross_pair,
  restricted_piece_pair,
};

[[nodiscard]] std::string_view to_string(
    HPolytopeConstraintDomain domain);

struct HPolytopeConstraintId {
  HPolytopeConstraintDomain domain;
  std::uint64_t first;
  std::uint64_t second;

  friend auto operator<=>(
      const HPolytopeConstraintId&,
      const HPolytopeConstraintId&) = default;
};

enum class ExactHPolytopeHalfspaceRole : std::uint8_t {
  parent_constraint,
  new_clip,
};

[[nodiscard]] std::string_view to_string(
    ExactHPolytopeHalfspaceRole role);

// The retained closed half-space always satisfies form <= 0.  Identity is
// semantic: distinct ids remain distinct incidences even when their oriented
// affine forms define the same geometric plane.
struct ExactHPolytopeHalfspace3 {
  HPolytopeConstraintId constraint_id;
  ExactHPolytopeHalfspaceRole role;
  exact::ExactAffineForm3 retained_nonpositive_form;

  friend bool operator==(
      const ExactHPolytopeHalfspace3&,
      const ExactHPolytopeHalfspace3&) = default;
};

enum class ExactHPolytopeHalfspaceKind : std::uint8_t {
  proper_halfspace,
  redundant_strict,
  infeasible,
  identically_active,
};

[[nodiscard]] std::string_view to_string(
    ExactHPolytopeHalfspaceKind kind);

struct ExactClassifiedHPolytopeHalfspace3 {
  HPolytopeConstraintId constraint_id;
  ExactHPolytopeHalfspaceRole role;
  ExactHPolytopeHalfspaceKind kind;
  exact::ExactAffineForm3 retained_nonpositive_form;
  std::optional<exact::ExactPlane3> plane;

  friend bool operator==(
      const ExactClassifiedHPolytopeHalfspace3&,
      const ExactClassifiedHPolytopeHalfspace3&) = default;
};

enum class ExactHPolytopeBoundaryKind : std::uint8_t {
  box_lower_x,
  box_upper_x,
  box_lower_y,
  box_upper_y,
  box_lower_z,
  box_upper_z,
  input_halfspace,
};

[[nodiscard]] std::string_view to_string(ExactHPolytopeBoundaryKind kind);

struct ExactHPolytopeBoundaryPlane {
  ExactHPolytopeBoundaryKind kind;
  std::optional<HPolytopeConstraintId> constraint_id;
  exact::ExactAffineForm3 retained_nonpositive_form;
  exact::ExactPlane3 plane;

  friend bool operator==(
      const ExactHPolytopeBoundaryPlane&,
      const ExactHPolytopeBoundaryPlane&) = default;
};

struct ExactHPolytopeVertex {
  exact::ExactRational3 position;
  std::vector<std::size_t> active_boundary_plane_indices;

  friend bool operator==(
      const ExactHPolytopeVertex&,
      const ExactHPolytopeVertex&) = default;
};

enum class ExactBoundedHPolytopeReferenceDecision : std::uint8_t {
  complete_nonempty,
  complete_empty,
  insufficient_budget,
};

[[nodiscard]] std::string_view to_string(
    ExactBoundedHPolytopeReferenceDecision decision);

struct ExactBoundedHPolytopeReferenceBudget {
  // On the n <= 14 reference domain, a restricted piece P(I,u) has at most
  // max_m ((m + 1) * (14 - m) - 1) = 55 semantic half-spaces.  The explicit
  // box adds six artificial boundaries.
  static constexpr std::size_t trusted_maximum_input_halfspace_count = 55U;
  static constexpr std::size_t trusted_maximum_boundary_count = 61U;
  static constexpr std::size_t trusted_maximum_plane_triple_count = 35990U;
  static constexpr std::size_t trusted_maximum_feasibility_test_count =
      2195390U;
  static constexpr std::size_t trusted_maximum_vertex_count = 35990U;
  static constexpr std::size_t trusted_maximum_incidence_test_count =
      2195390U;

  static_assert(
      trusted_maximum_boundary_count ==
      trusted_maximum_input_halfspace_count + 6U);
  static_assert(
      trusted_maximum_plane_triple_count ==
      trusted_maximum_boundary_count *
          (trusted_maximum_boundary_count - 1U) *
          (trusted_maximum_boundary_count - 2U) / 6U);
  static_assert(
      trusted_maximum_feasibility_test_count ==
      trusted_maximum_plane_triple_count * trusted_maximum_boundary_count);
  static_assert(
      trusted_maximum_vertex_count ==
      trusted_maximum_plane_triple_count);
  static_assert(
      trusted_maximum_incidence_test_count ==
      trusted_maximum_vertex_count * trusted_maximum_boundary_count);

  std::size_t maximum_input_halfspace_count{
      trusted_maximum_input_halfspace_count};
  std::size_t maximum_boundary_count{trusted_maximum_boundary_count};
  std::size_t maximum_plane_triple_count{
      trusted_maximum_plane_triple_count};
  std::size_t maximum_feasibility_test_count{
      trusted_maximum_feasibility_test_count};
  std::size_t maximum_vertex_count{trusted_maximum_vertex_count};
  std::size_t maximum_incidence_test_count{
      trusted_maximum_incidence_test_count};
};

struct ExactBoundedHPolytopeReferenceRequirements {
  std::size_t input_halfspace_count{0U};
  std::size_t conservative_boundary_count{0U};
  std::size_t conservative_plane_triple_count{0U};
  std::size_t conservative_feasibility_test_count{0U};
  std::size_t conservative_vertex_count{0U};
  std::size_t conservative_incidence_test_count{0U};

  friend bool operator==(
      const ExactBoundedHPolytopeReferenceRequirements&,
      const ExactBoundedHPolytopeReferenceRequirements&) = default;
};

struct ExactBoundedHPolytopeReferenceAudit {
  std::size_t proper_halfspace_count{0U};
  std::size_t redundant_strict_count{0U};
  std::size_t infeasible_count{0U};
  std::size_t identically_active_count{0U};
  std::size_t enumerated_plane_triple_count{0U};
  std::size_t exact_feasibility_test_count{0U};
  std::size_t unique_feasible_vertex_count{0U};
  std::size_t exact_incidence_test_count{0U};
  std::size_t active_incidence_count{0U};

  friend bool operator==(
      const ExactBoundedHPolytopeReferenceAudit&,
      const ExactBoundedHPolytopeReferenceAudit&) = default;
};

struct ExactBoundedHPolytopeReferenceResult {
  ExactBoundedHPolytopeReferenceDecision decision{
      ExactBoundedHPolytopeReferenceDecision::insufficient_budget};
  ExactBoundedHPolytopeReferenceRequirements requirements;
  ExactBoundedHPolytopeReferenceAudit audit;
  std::vector<ExactClassifiedHPolytopeHalfspace3> classified_halfspaces;
  std::vector<ExactHPolytopeBoundaryPlane> boundary_planes;
  std::vector<ExactHPolytopeVertex> vertices;
  std::optional<std::size_t> affine_dimension;

  friend bool operator==(
      const ExactBoundedHPolytopeReferenceResult&,
      const ExactBoundedHPolytopeReferenceResult&) = default;
};

// Exhaustive, bounded host oracle for an arbitrary intersection of exact
// closed half-spaces inside an explicit nonempty dyadic box.  The box makes
// the intersection compact; the routine enumerates every plane triple and
// therefore covers dimensions zero through three without a general-position
// assumption.  It is a falsifier for the future product primitive, not a
// scalable implementation or a public diagram certificate.
[[nodiscard]] ExactBoundedHPolytopeReferenceResult
build_exact_bounded_h_polytope_reference(
    std::span<const ExactHPolytopeHalfspace3> halfspaces,
    const ExactDyadicAabb3& clipping_box,
    ExactBoundedHPolytopeReferenceBudget budget = {});

}  // namespace morsehgp3d::spatial
