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

inline constexpr std::uint32_t direct_sparse_successive_incidence_schema_version =
    1U;
inline constexpr std::string_view direct_sparse_successive_incidence_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_successive_incidence_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_successive_incidence_mode =
    "certified";
inline constexpr std::string_view
    direct_sparse_successive_incidence_refinement_status = "partial_refinement";
inline constexpr std::string_view direct_sparse_successive_incidence_public_status =
    "not_claimed";
inline constexpr std::string_view direct_sparse_successive_incidence_proof_basis =
    "exact_exclusive_threshold_one_point_coface_positive_support_reduction_"
    "weighted_source_support_radial_and_pair_aabb_lower_bound_strict_"
    "pruning_all_next_level_cominimizers_v1";

inline constexpr std::size_t
    direct_sparse_successive_incidence_maximum_source_point_count = 10U;
inline constexpr std::size_t
    direct_sparse_successive_incidence_source_support_enumeration_count_per_pass =
        385U;
inline constexpr std::size_t
    direct_sparse_successive_incidence_maximum_source_support_enumeration_count =
        770U;
inline constexpr std::size_t
    direct_sparse_successive_incidence_maximum_outside_coface_support_count =
        176U;
inline constexpr std::size_t
    direct_sparse_successive_incidence_maximum_outside_coface_classification_count =
        1936U;

// Every cap is checked before the corresponding operation.  No partial
// successive-incidence level or prefix of equal minimizers is published on
// exhaustion.  A provisional equality shell may overflow and later be reset
// by a strictly better incumbent; it is terminal only if still active when
// the traversal completes.
struct ExactDirectSparseSuccessiveIncidenceBudget {
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
      const ExactDirectSparseSuccessiveIncidenceBudget&,
      const ExactDirectSparseSuccessiveIncidenceBudget&) = default;
};

enum class ExactDirectSparseSuccessiveIncidenceStopReason : std::uint8_t {
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

enum class ExactDirectSparseSuccessiveIncidenceDecision : std::uint8_t {
  not_certified,
  complete_no_strictly_higher_coface,
  complete_exact_next_incidence,
  no_next_incidence_budget_exhausted,
};

enum class ExactDirectSparseSuccessiveIncidenceScope : std::uint8_t {
  unspecified,
  single_supplied_facet_next_strict_one_point_coface_level_only,
};

struct ExactDirectSparseSuccessiveIncidenceAudit {
  std::size_t eligible_coface_point_count{};
  std::size_t supplied_incumbent_seed_point_count{};
  std::size_t exact_incumbent_seed_evaluation_count{};
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
  std::size_t at_or_below_exclusive_threshold_point_count{};
  std::size_t pruned_node_count{};
  std::size_t pruned_eligible_point_count{};
  std::size_t peak_frontier_entry_count{};
  std::size_t peak_cominimizer_entry_count{};
  std::size_t incumbent_improvement_count{};
  std::size_t equal_incumbent_observation_count{};
  std::size_t provisional_cominimizer_overflow_count{};
  bool traversal_complete{false};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceAudit&,
      const ExactDirectSparseSuccessiveIncidenceAudit&) = default;
};

// A minimizing coface is represented by F plus added_point_id.  Its exact
// positive support has at most four points in R^3.  Unused support slots are
// zero and never interpreted without support_point_count.
struct ExactDirectSparseSuccessiveIncidenceMinimizer {
  spatial::PointId added_point_id{};
  std::array<spatial::PointId, 4U> support_point_ids{};
  std::size_t support_point_count{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  bool added_point_in_source_closed_ball{false};
  bool added_point_in_selected_positive_support{false};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceMinimizer&,
      const ExactDirectSparseSuccessiveIncidenceMinimizer&) = default;
};

struct ExactDirectSparseSuccessiveIncidenceResult {
  static constexpr std::string_view backend =
      direct_sparse_successive_incidence_backend;
  static constexpr std::string_view profile =
      direct_sparse_successive_incidence_profile;
  static constexpr std::string_view mode = direct_sparse_successive_incidence_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_successive_incidence_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_successive_incidence_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_successive_incidence_proof_basis;

  std::uint32_t schema_version{direct_sparse_successive_incidence_schema_version};
  ExactDirectSparseFacetKey source_facet_key{};
  // No threshold asks for the ordinary first incidence.  A present threshold
  // is exclusive: candidates at or below it remain fully accounted for but
  // can neither become the incumbent nor authorize pruning.
  std::optional<exact::ExactLevel> exclusive_lower_squared_level;
  ExactDirectSparseSuccessiveIncidenceBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::optional<ExactFacetMiniballResult> source_facet_miniball;
  std::optional<exact::ExactLevel> next_incidence_squared_level;
  std::vector<ExactDirectSparseSuccessiveIncidenceMinimizer> cominimizers;
  ExactDirectSparseSuccessiveIncidenceAudit audit{};
  ExactDirectSparseSuccessiveIncidenceStopReason stop_reason{
      ExactDirectSparseSuccessiveIncidenceStopReason::none};
  ExactDirectSparseSuccessiveIncidenceDecision decision{
      ExactDirectSparseSuccessiveIncidenceDecision::not_certified};
  ExactDirectSparseSuccessiveIncidenceScope scope{
      ExactDirectSparseSuccessiveIncidenceScope::unspecified};
  bool trusted_authorities_certified{false};
  bool source_facet_miniball_freshly_certified{false};
  bool every_nonexcluded_point_evaluated_or_strictly_pruned{false};
  bool aabb_lower_bounds_exact_and_valid{false};
  bool equality_bounds_always_descended{false};
  bool every_strict_outside_coface_support_contains_added_point{false};
  bool all_cominimizers_retained_atomically{false};
  bool no_partial_next_incidence_payload_published{false};
  bool no_global_facet_or_coface_catalog_materialized{false};
  bool no_gamma_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};

  [[nodiscard]] bool certified_complete_next_incidence() const noexcept;
  [[nodiscard]] bool certified_complete_no_strictly_higher_coface()
      const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceResult&,
      const ExactDirectSparseSuccessiveIncidenceResult&) = default;
};

struct ExactDirectSparseSuccessiveIncidenceVerification {
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
      const ExactDirectSparseSuccessiveIncidenceVerification&,
      const ExactDirectSparseSuccessiveIncidenceVerification&) = default;
};

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceResult
build_exact_direct_sparse_successive_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// Optional seeds are exact incumbent proposals only.  They must be unique,
// sorted, in-range and outside the source facet.  Every seed is evaluated
// exactly before the LBVH traversal, but it grants no pruning authority by
// itself: the same strict AABB proof still evaluates or strictly prunes every
// other eligible point, and equality still descends to retain all global
// co-minimizers.  In particular, callers may supply the union of the ordinary-
// Delaunay neighbor lists of a two-point source facet without making the
// result depend on the completeness of that graph.
[[nodiscard]] ExactDirectSparseSuccessiveIncidenceResult
build_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::span<const spatial::PointId> incumbent_seed_point_ids,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceVerification
verify_exact_direct_sparse_successive_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceResult& observed);

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceVerification
verify_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::span<const spatial::PointId> incumbent_seed_point_ids,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceResult& observed);

}  // namespace morsehgp3d::hierarchy
