#pragma once

#include "morsehgp3d/exact/plane.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {

// Binary64 input provenance for the bounded Phase 7 reference oracle.  Point
// identity is independent of position so coincident weighted sites remain
// representable and can be classified exactly.
class Binary64WeightedSite3 {
 public:
  [[nodiscard]] static Binary64WeightedSite3 from_binary64(
      PointId point_id,
      double x,
      double y,
      double z,
      double weight) {
    return from_binary64_bits(
        point_id,
        {std::bit_cast<std::uint64_t>(x),
         std::bit_cast<std::uint64_t>(y),
         std::bit_cast<std::uint64_t>(z)},
        std::bit_cast<std::uint64_t>(weight));
  }

  [[nodiscard]] static Binary64WeightedSite3 from_binary64_bits(
      PointId point_id,
      std::array<std::uint64_t, 3> position_bits,
      std::uint64_t weight_bits);

  [[nodiscard]] PointId point_id() const noexcept { return point_id_; }

  [[nodiscard]] const exact::CertifiedPoint3& position() const & noexcept {
    return position_;
  }
  [[nodiscard]] const exact::CertifiedPoint3& position() const && = delete;

  [[nodiscard]] std::uint64_t weight_binary64_bits() const noexcept {
    return weight_binary64_bits_;
  }

  [[nodiscard]] const exact::ExactRational& exact_weight() const & noexcept {
    return exact_weight_;
  }
  [[nodiscard]] const exact::ExactRational& exact_weight() const && = delete;

 private:
  Binary64WeightedSite3(
      PointId point_id,
      exact::CertifiedPoint3 position,
      std::uint64_t weight_binary64_bits,
      exact::ExactRational exact_weight)
      : point_id_(point_id),
        position_(std::move(position)),
        weight_binary64_bits_(weight_binary64_bits),
        exact_weight_(std::move(exact_weight)) {}

  PointId point_id_;
  exact::CertifiedPoint3 position_;
  std::uint64_t weight_binary64_bits_;
  exact::ExactRational exact_weight_;
};

enum class PowerBisectorConstraintKind : std::uint8_t {
  proper_halfspace,
  owner_dominates,
  competitor_dominates,
  coincident_tie,
};

[[nodiscard]] std::string_view to_string(
    PowerBisectorConstraintKind kind);

// The affine form is power(owner)-power(competitor).  The owner's cell keeps
// its non-positive half-space.  A plane exists exactly for a proper bisector.
struct ExactPowerBisectorConstraint {
  PointId owner_id;
  PointId competitor_id;
  PowerBisectorConstraintKind kind;
  exact::ExactAffineForm3 owner_minus_competitor;
  std::optional<exact::ExactPlane3> plane;

  friend bool operator==(
      const ExactPowerBisectorConstraint&,
      const ExactPowerBisectorConstraint&) = default;
};

[[nodiscard]] exact::ExactRational exact_power_distance(
    const Binary64WeightedSite3& site,
    const exact::ExactRational3& query);

[[nodiscard]] exact::ExactAffineForm3 exact_power_difference_affine_form(
    const Binary64WeightedSite3& owner,
    const Binary64WeightedSite3& competitor);

[[nodiscard]] ExactPowerBisectorConstraint
make_exact_power_bisector_constraint(
    const Binary64WeightedSite3& owner,
    const Binary64WeightedSite3& competitor);

enum class PowerCellBoundaryKind : std::uint8_t {
  box_lower_x,
  box_upper_x,
  box_lower_y,
  box_upper_y,
  box_lower_z,
  box_upper_z,
  power_bisector,
};

[[nodiscard]] std::string_view to_string(PowerCellBoundaryKind kind);

// Every boundary is oriented so the retained cell satisfies form <= 0.
struct ExactPowerCellBoundaryPlane {
  PowerCellBoundaryKind kind;
  std::optional<PointId> competitor_id;
  exact::ExactAffineForm3 owner_halfspace_form;
  exact::ExactPlane3 plane;

  friend bool operator==(
      const ExactPowerCellBoundaryPlane&,
      const ExactPowerCellBoundaryPlane&) = default;
};

struct ExactPowerCellVertex {
  exact::ExactRational3 position;
  std::vector<std::size_t> active_boundary_plane_indices;

  friend bool operator==(
      const ExactPowerCellVertex&,
      const ExactPowerCellVertex&) = default;
};

enum class ExactPowerCellReferenceDecision : std::uint8_t {
  complete_nonempty,
  complete_empty,
  insufficient_budget,
};

[[nodiscard]] std::string_view to_string(
    ExactPowerCellReferenceDecision decision);

struct ExactPowerCellReferenceBudget {
  static constexpr std::size_t trusted_maximum_site_count = 8U;
  static constexpr std::size_t trusted_maximum_constraint_count = 13U;
  static constexpr std::size_t trusted_maximum_plane_triple_count = 286U;
  static constexpr std::size_t trusted_maximum_vertex_count = 286U;
  static constexpr std::size_t trusted_maximum_incidence_count = 3718U;

  static_assert(
      trusted_maximum_constraint_count == trusted_maximum_site_count + 5U);
  static_assert(
      trusted_maximum_plane_triple_count ==
      trusted_maximum_constraint_count *
          (trusted_maximum_constraint_count - 1U) *
          (trusted_maximum_constraint_count - 2U) / 6U);
  static_assert(
      trusted_maximum_vertex_count == trusted_maximum_plane_triple_count);
  static_assert(
      trusted_maximum_incidence_count ==
      trusted_maximum_vertex_count * trusted_maximum_constraint_count);

  std::size_t maximum_site_count{trusted_maximum_site_count};
  std::size_t maximum_constraint_count{trusted_maximum_constraint_count};
  std::size_t maximum_plane_triple_count{
      trusted_maximum_plane_triple_count};
  std::size_t maximum_vertex_count{trusted_maximum_vertex_count};
  std::size_t maximum_incidence_count{trusted_maximum_incidence_count};
};

struct ExactPowerCellReferenceRequirements {
  std::size_t site_count{0U};
  std::size_t conservative_constraint_count{0U};
  std::size_t conservative_plane_triple_count{0U};
  std::size_t conservative_vertex_count{0U};
  std::size_t conservative_incidence_count{0U};

  friend bool operator==(
      const ExactPowerCellReferenceRequirements&,
      const ExactPowerCellReferenceRequirements&) = default;
};

struct ExactPowerCellReferenceAudit {
  std::size_t proper_bisector_count{0U};
  std::size_t redundant_constraint_count{0U};
  std::size_t infeasible_constraint_count{0U};
  std::size_t enumerated_plane_triple_count{0U};
  std::size_t unique_feasible_vertex_count{0U};
  std::size_t active_incidence_count{0U};
  bool has_coincident_tie{false};

  friend bool operator==(
      const ExactPowerCellReferenceAudit&,
      const ExactPowerCellReferenceAudit&) = default;
};

struct ExactPowerCellReferenceResult {
  ExactPowerCellReferenceDecision decision{
      ExactPowerCellReferenceDecision::insufficient_budget};
  ExactPowerCellReferenceRequirements requirements;
  ExactPowerCellReferenceAudit audit;
  std::vector<ExactPowerBisectorConstraint> pairwise_constraints;
  std::vector<ExactPowerCellBoundaryPlane> boundary_planes;
  std::vector<ExactPowerCellVertex> vertices;

  friend bool operator==(
      const ExactPowerCellReferenceResult&,
      const ExactPowerCellReferenceResult&) = default;
};

// Independent, bounded host oracle for one weighted cell clipped by an
// explicit dyadic box.  It is intentionally exhaustive and non-scalable: its
// role is to falsify floating proposals, not to publish a diagram.  A complete
// decision covers the closed local polyhedron, including lower-dimensional or
// coincident-tie cases; it is not a general-position certificate.
[[nodiscard]] ExactPowerCellReferenceResult
build_exact_bounded_power_cell_reference(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> competitors,
    const ExactDyadicAabb3& clipping_box,
    ExactPowerCellReferenceBudget budget = {});

enum class ExactPowerCellSubsetClosureDecision : std::uint8_t {
  complete_nonempty,
  complete_empty,
  incomplete,
  insufficient_budget,
};

[[nodiscard]] std::string_view to_string(
    ExactPowerCellSubsetClosureDecision decision);

struct ExactPowerCellSubsetClosureBudget {
  // The maximum is attained with seven complete competitors and four
  // candidate competitors: (7 - 4) * choose(6 + 4, 3) = 360.
  static constexpr std::size_t trusted_maximum_omitted_vertex_test_count =
      360U;

  ExactPowerCellReferenceBudget candidate_cell_budget{};
  std::size_t maximum_omitted_vertex_test_count{
      trusted_maximum_omitted_vertex_test_count};
};

struct ExactPowerCellSubsetClosureRequirements {
  std::size_t complete_site_count{0U};
  std::size_t candidate_competitor_count{0U};
  std::size_t omitted_competitor_count{0U};
  ExactPowerCellReferenceRequirements candidate_cell_requirements;
  std::size_t conservative_omitted_vertex_test_count{0U};

  friend bool operator==(
      const ExactPowerCellSubsetClosureRequirements&,
      const ExactPowerCellSubsetClosureRequirements&) = default;
};

struct ExactPowerCellSubsetClosureAudit {
  std::size_t classified_omitted_constraint_count{0U};
  std::size_t omitted_proper_halfspace_count{0U};
  std::size_t omitted_violating_halfspace_count{0U};
  std::size_t omitted_missing_active_incidence_count{0U};
  std::size_t omitted_owner_dominates_count{0U};
  std::size_t omitted_competitor_dominates_count{0U};
  std::size_t omitted_coincident_tie_count{0U};
  std::size_t exact_omitted_vertex_test_count{0U};

  friend bool operator==(
      const ExactPowerCellSubsetClosureAudit&,
      const ExactPowerCellSubsetClosureAudit&) = default;
};

enum class ExactPowerCellSubsetClosureWitnessKind : std::uint8_t {
  violating_halfspace,
  missing_active_incidence,
  competitor_dominates,
};

[[nodiscard]] std::string_view to_string(
    ExactPowerCellSubsetClosureWitnessKind kind);

struct ExactPowerCellSubsetClosureWitness {
  PointId omitted_competitor_id;
  PowerBisectorConstraintKind constraint_kind;
  ExactPowerCellSubsetClosureWitnessKind witness_kind;
  std::optional<std::size_t> candidate_vertex_index;

  friend bool operator==(
      const ExactPowerCellSubsetClosureWitness&,
      const ExactPowerCellSubsetClosureWitness&) = default;
};

struct ExactPowerCellSubsetClosureResult {
  ExactPowerCellSubsetClosureDecision decision{
      ExactPowerCellSubsetClosureDecision::insufficient_budget};
  ExactPowerCellSubsetClosureRequirements requirements;
  ExactPowerCellSubsetClosureAudit audit;
  std::vector<PointId> canonical_candidate_competitor_ids;
  std::vector<PointId> required_candidate_additions;
  ExactPowerCellReferenceResult candidate_cell;
  std::optional<ExactPowerCellSubsetClosureWitness> first_omitted_witness;

  friend bool operator==(
      const ExactPowerCellSubsetClosureResult&,
      const ExactPowerCellSubsetClosureResult&) = default;
};

// Certifies whether the exact bounded cell H(J), built from a supplied subset
// J of authentic competitor ids, already equals the cell for the complete
// competitor list with all proper active incidences; coincident ties are
// counted separately because they do not define a boundary plane.  Candidate
// ids are canonicalized but must be unique.  An authentic over-seed relative
// to the active faces
// (for example a symmetrized adjacency) is valid.  The first omitted violation
// or active incidence is deterministic in PointId, then prefers a violation
// over a zero incidence for that id, then uses vertex order.  All required
// additions are returned canonically so one subsequent rebuild can apply them
// together.
[[nodiscard]] ExactPowerCellSubsetClosureResult
certify_exact_bounded_power_cell_subset_closure(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> complete_competitors,
    const ExactDyadicAabb3& clipping_box,
    std::span<const PointId> candidate_competitor_ids,
    ExactPowerCellSubsetClosureBudget budget = {});

}  // namespace morsehgp3d::spatial
