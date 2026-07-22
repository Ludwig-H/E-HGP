#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"
#include "morsehgp3d/hierarchy/facet_miniball.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_sparse_first_incidence_schema_version =
    1U;
inline constexpr std::string_view direct_sparse_first_incidence_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_first_incidence_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_first_incidence_mode =
    "certified";
inline constexpr std::string_view
    direct_sparse_first_incidence_refinement_status = "partial_refinement";
inline constexpr std::string_view direct_sparse_first_incidence_public_status =
    "not_claimed";
inline constexpr std::string_view direct_sparse_first_incidence_proof_basis =
    "exact_one_point_coface_positive_support_reduction_weighted_source_"
    "support_radial_and_pair_aabb_lower_bound_strict_pruning_all_"
    "cominimizers_v1";

inline constexpr std::size_t
    direct_sparse_first_incidence_maximum_source_point_count = 10U;
inline constexpr std::size_t
    direct_sparse_first_incidence_source_support_enumeration_count_per_pass =
        385U;
inline constexpr std::size_t
    direct_sparse_first_incidence_maximum_source_support_enumeration_count =
        770U;
inline constexpr std::size_t
    direct_sparse_first_incidence_maximum_outside_coface_support_count =
        176U;
inline constexpr std::size_t
    direct_sparse_first_incidence_maximum_outside_coface_classification_count =
        1936U;

// Every cap is checked before the corresponding operation.  No partial
// first-incidence level or prefix of equal minimizers is published on
// exhaustion.  A provisional equality shell may overflow and later be reset
// by a strictly better incumbent; it is terminal only if still active when
// the traversal completes.
struct ExactDirectSparseFirstIncidenceBudget {
  std::size_t maximum_source_support_enumeration_count{};
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_internal_node_expansion_count{};
  std::size_t maximum_exact_aabb_bound_evaluation_count{};
  std::size_t maximum_exact_point_evaluation_count{};
  std::size_t maximum_coface_support_enumeration_count{};
  std::size_t maximum_candidate_point_classification_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_cominimizer_count{};

  friend bool operator==(
      const ExactDirectSparseFirstIncidenceBudget&,
      const ExactDirectSparseFirstIncidenceBudget&) = default;
};

enum class ExactDirectSparseFirstIncidenceStopReason : std::uint8_t {
  none,
  source_support_enumeration_limit,
  node_visit_limit,
  internal_node_expansion_limit,
  exact_aabb_bound_evaluation_limit,
  exact_point_evaluation_limit,
  coface_support_enumeration_limit,
  candidate_point_classification_limit,
  frontier_entry_limit,
  cominimizer_entry_limit,
};

enum class ExactDirectSparseFirstIncidenceDecision : std::uint8_t {
  not_certified,
  complete_no_coface,
  complete_exact_first_incidence,
  no_first_incidence_budget_exhausted,
};

enum class ExactDirectSparseFirstIncidenceScope : std::uint8_t {
  unspecified,
  single_supplied_facet_all_one_point_cofaces_only,
};

struct ExactDirectSparseFirstIncidenceAudit {
  std::size_t eligible_coface_point_count{};
  std::size_t source_support_enumeration_count{};
  std::size_t node_visit_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t exact_aabb_bound_evaluation_count{};
  std::size_t exact_point_evaluation_count{};
  std::size_t excluded_facet_point_count{};
  std::size_t coface_support_enumeration_count{};
  std::size_t candidate_point_classification_count{};
  std::size_t inside_or_boundary_source_ball_point_count{};
  std::size_t outside_source_ball_point_count{};
  std::size_t pruned_node_count{};
  std::size_t pruned_eligible_point_count{};
  std::size_t peak_frontier_entry_count{};
  std::size_t peak_cominimizer_entry_count{};
  std::size_t incumbent_improvement_count{};
  std::size_t equal_incumbent_observation_count{};
  std::size_t provisional_cominimizer_overflow_count{};
  bool traversal_complete{false};

  friend bool operator==(
      const ExactDirectSparseFirstIncidenceAudit&,
      const ExactDirectSparseFirstIncidenceAudit&) = default;
};

// A minimizing coface is represented by F plus added_point_id.  Its exact
// positive support has at most four points in R^3.  Unused support slots are
// zero and never interpreted without support_point_count.
struct ExactDirectSparseFirstIncidenceMinimizer {
  spatial::PointId added_point_id{};
  std::array<spatial::PointId, 4U> support_point_ids{};
  std::size_t support_point_count{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  bool added_point_in_source_closed_ball{false};
  bool added_point_in_selected_positive_support{false};

  friend bool operator==(
      const ExactDirectSparseFirstIncidenceMinimizer&,
      const ExactDirectSparseFirstIncidenceMinimizer&) = default;
};

struct ExactDirectSparseFirstIncidenceResult {
  static constexpr std::string_view backend =
      direct_sparse_first_incidence_backend;
  static constexpr std::string_view profile =
      direct_sparse_first_incidence_profile;
  static constexpr std::string_view mode = direct_sparse_first_incidence_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_first_incidence_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_first_incidence_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_first_incidence_proof_basis;

  std::uint32_t schema_version{direct_sparse_first_incidence_schema_version};
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFirstIncidenceBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::optional<ExactFacetMiniballResult> source_facet_miniball;
  std::optional<exact::ExactLevel> first_incidence_squared_level;
  std::vector<ExactDirectSparseFirstIncidenceMinimizer> cominimizers;
  ExactDirectSparseFirstIncidenceAudit audit{};
  ExactDirectSparseFirstIncidenceStopReason stop_reason{
      ExactDirectSparseFirstIncidenceStopReason::none};
  ExactDirectSparseFirstIncidenceDecision decision{
      ExactDirectSparseFirstIncidenceDecision::not_certified};
  ExactDirectSparseFirstIncidenceScope scope{
      ExactDirectSparseFirstIncidenceScope::unspecified};
  bool trusted_authorities_certified{false};
  bool source_facet_miniball_freshly_certified{false};
  bool every_nonexcluded_point_evaluated_or_strictly_pruned{false};
  bool aabb_lower_bounds_exact_and_valid{false};
  bool equality_bounds_always_descended{false};
  bool every_strict_outside_coface_support_contains_added_point{false};
  bool all_cominimizers_retained_atomically{false};
  bool no_partial_first_incidence_payload_published{false};
  bool no_global_facet_or_coface_catalog_materialized{false};
  bool no_gamma_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};

  [[nodiscard]] bool certified_complete_first_incidence() const noexcept;
  [[nodiscard]] bool certified_complete_no_coface() const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFirstIncidenceResult&,
      const ExactDirectSparseFirstIncidenceResult&) = default;
};

struct ExactDirectSparseFirstIncidenceVerification {
  bool trusted_inputs_certified{false};
  bool observed_storage_within_budget{false};
  bool source_miniball_freshly_replayed{false};
  bool branch_and_bound_freshly_replayed{false};
  bool all_cominimizers_freshly_replayed{false};
  bool counters_and_decision_freshly_replayed{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseFirstIncidenceVerification&,
      const ExactDirectSparseFirstIncidenceVerification&) = default;
};

[[nodiscard]] ExactDirectSparseFirstIncidenceResult
build_exact_direct_sparse_first_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

[[nodiscard]] ExactDirectSparseFirstIncidenceVerification
verify_exact_direct_sparse_first_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFirstIncidenceResult& observed);

}  // namespace morsehgp3d::hierarchy
