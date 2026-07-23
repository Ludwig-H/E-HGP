#pragma once

#include "morsehgp3d/hierarchy/facet_miniball.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_facet_descent_step_schema_version = 1U;
inline constexpr std::string_view direct_sparse_facet_descent_step_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_facet_descent_step_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_facet_descent_step_mode =
    "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_step_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_facet_descent_step_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_step_proof_basis =
        "bounded_complete_lbvh_top_k_canonical_successor_fresh_exact_"
        "miniballs_strict_half_open_segment_relative_positive_locator_v1";
inline constexpr std::size_t
    direct_sparse_facet_descent_step_maximum_fresh_miniball_enumeration_count =
        4U;

// The two locator probes are separate because a source miss can finish with a
// strict geometric successor whose lookup has a different operational budget.
// The local miniballs need no n-dependent budget: a facet contains at most ten
// points and one enumeration examines at most 385 supports.  Each certified
// builder performs a fresh replay, so a two-miniball step executes at most
// direct_sparse_facet_descent_step_maximum_fresh_miniball_enumeration_count
// such enumerations.  The only global operation is the explicitly bounded
// LBVH query.
struct ExactDirectSparseFacetDescentStepBudget {
  ExactDirectSparsePositiveFacetProbeBudget source_locator_probe{};
  spatial::ExactLbvhTopKBudget top_k_query{};
  ExactDirectSparsePositiveFacetProbeBudget successor_locator_probe{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentStepBudget&,
      const ExactDirectSparseFacetDescentStepBudget&) = default;
};

enum class ExactDirectSparseFacetDescentStepDisposition : std::uint8_t {
  not_certified,
  relative_positive,
  unresolved,
  budget_exhausted,
  contradiction,
};

enum class ExactDirectSparseFacetDescentStepDecision : std::uint8_t {
  not_certified,
  no_resolution_source_locator_probe_budget_exhausted,
  complete_relative_source_positive_hit,
  complete_unresolved_source_above_closed_batch_level,
  no_resolution_top_k_budget_exhausted,
  contradiction_top_k_cutoff_above_source_miniball,
  complete_unresolved_source_is_canonical_top_k_choice,
  contradiction_successor_miniball_above_top_k_cutoff,
  complete_unresolved_non_strict_canonical_successor,
  contradiction_successor_source_atom_cutoff_mismatch,
  no_resolution_successor_locator_probe_budget_exhausted,
  complete_unresolved_strict_successor_not_bound,
  complete_relative_strict_successor_positive_hit,
};

enum class ExactDirectSparseFacetDescentStepScope : std::uint8_t {
  unspecified,
  single_strict_top_k_step_and_caller_authority_relative_positive_hit_only,
};

struct ExactDirectSparseFacetDescentStepCounters {
  std::size_t source_locator_probe_count{};
  std::size_t source_miniball_build_count{};
  std::size_t source_miniball_reuse_count{};
  std::size_t top_k_query_count{};
  std::size_t canonical_successor_selection_count{};
  std::size_t successor_miniball_build_count{};
  std::size_t successor_miniball_reuse_count{};
  std::size_t successor_source_distance_evaluation_count{};
  std::size_t successor_source_maximum_comparison_count{};
  std::size_t center_displacement_evaluation_count{};
  std::size_t exact_level_relation_count{};
  std::size_t convex_segment_certification_count{};
  std::size_t successor_locator_probe_count{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentStepCounters&,
      const ExactDirectSparseFacetDescentStepCounters&) = default;
};

// This is the only scientific geometric payload retained by a strict branch.
// It is constant-size because both facet keys contain at most ten PointIds.
// For t in (0,1], the squared-distance chord identity bounds every successor
// point strictly below source_facet_squared_level along the center segment.
// Equality of the source facet level with the closed batch level is allowed;
// the separate closed-segment flag records whether the source endpoint is also
// strictly below that batch level.
struct ExactDirectSparseFacetDescentStepWitness {
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFacetKey successor_facet_key{};
  exact::ExactCenter3 source_center{};
  exact::ExactCenter3 successor_center{};
  exact::ExactLevel source_facet_squared_level{};
  exact::ExactLevel top_k_cutoff_squared_level{};
  exact::ExactLevel successor_at_source_squared_level{};
  exact::ExactLevel successor_facet_squared_level{};
  exact::ExactLevel center_squared_displacement{};
  bool successor_is_complete_canonical_top_k_choice{false};
  bool successor_differs_from_source{false};
  bool successor_at_source_equals_top_k_cutoff{false};
  bool top_k_cutoff_at_most_source_level{false};
  bool successor_level_at_most_top_k_cutoff{false};
  bool strict_miniball_level_decrease{false};
  bool exact_squared_distance_chord_identity_applies{false};
  bool source_facet_at_or_below_closed_batch_level{false};
  bool source_open_target_closed_segment_strict_below_source_level{false};
  bool source_open_target_closed_segment_strict_below_closed_batch_level{
      false};
  bool closed_segment_strict_below_closed_batch_level{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentStepWitness&,
      const ExactDirectSparseFacetDescentStepWitness&) = default;
};

// No TopKPartition, equality shell, exterior-point vector, facet catalogue or
// hierarchy object survives the call.  A budget exhaustion exposes only its
// operational audit.  A strict witness may survive an exhausted or missing
// successor lookup, but it never carries an invented component handle.
struct ExactDirectSparseFacetDescentStepResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_step_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_step_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_step_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_facet_descent_step_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_step_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_step_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_step_schema_version};
  ExactDirectSparseFacetDescentStepBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  exact::ExactLevel closed_batch_squared_level{};
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetProbeResult source_locator_probe{};
  std::optional<ExactDirectSparsePositiveFacetProbeResult>
      successor_locator_probe;
  spatial::ExactLbvhTopKAudit top_k_audit{};
  spatial::ExactLbvhTopKStopReason top_k_stop_reason{
      spatial::ExactLbvhTopKStopReason::none};
  std::optional<spatial::SpatialQueryCounters> complete_top_k_query_counters;
  std::optional<ExactDirectSparseFacetDescentStepWitness> strict_step_witness;
  std::optional<ExactDirectSparseComponentHandle> resolved_component_handle;
  std::optional<ExactDirectSparseFacetWitness> resolved_binding_witness;
  ExactDirectSparseFacetDescentStepCounters counters{};
  bool input_shape_certified{false};
  bool source_probe_used_const_pre_call_locator{false};
  bool source_miniball_freshly_certified{false};
  bool source_miniball_reused_from_certified_input{false};
  bool complete_top_k_partition_certified{false};
  bool complete_top_k_shell_consumed_transiently{false};
  bool successor_miniball_freshly_certified{false};
  bool successor_miniball_reused_from_certified_lookup{false};
  bool exact_level_relations_certified{false};
  bool strict_half_open_segment_certified{false};
  bool successor_probe_used_same_const_pre_call_locator{false};
  bool locator_state_mutated{false};
  bool locator_batch_committed{false};
  bool external_binding_authority_replayed{false};
  bool missing_facet_means_isolated{false};
  bool singleton_component_created{false};
  bool global_closed_ball_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool hierarchy_attachment_published{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseFacetDescentStepDisposition disposition{
      ExactDirectSparseFacetDescentStepDisposition::not_certified};
  ExactDirectSparseFacetDescentStepDecision decision{
      ExactDirectSparseFacetDescentStepDecision::not_certified};
  ExactDirectSparseFacetDescentStepScope scope{
      ExactDirectSparseFacetDescentStepScope::unspecified};

  [[nodiscard]] bool certified_relative_positive_resolution() const noexcept;
  [[nodiscard]] bool certified_unresolved_without_isolation() const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;
  [[nodiscard]] bool certified_fail_closed_contradiction() const noexcept;
  [[nodiscard]] bool certified_partial_refinement_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentStepResult&,
      const ExactDirectSparseFacetDescentStepResult&) = default;
};

// The locator's external authority is deliberately outside this verifier.
// The verifier re-executes the bounded geometry and both const probes from the
// supplied cloud, LBVH, facet, level, witness and budget; no observed field
// steers that replay.
struct ExactDirectSparseFacetDescentStepVerification {
  bool trusted_inputs_certified{false};
  bool observed_outcome_well_formed{false};
  bool source_key_freshly_reconstructed{false};
  bool bounded_top_k_freshly_replayed{false};
  bool strict_witness_freshly_replayed{false};
  bool locator_probes_freshly_replayed{false};
  bool no_partial_top_k_partition_persisted{false};
  bool no_locator_mutation_or_batch_commit{false};
  bool no_isolation_singleton_or_attachment_invented{false};
  bool external_binding_authority_replayed{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentStepVerification&,
      const ExactDirectSparseFacetDescentStepVerification&) = default;
};

[[nodiscard]] ExactDirectSparseFacetDescentStepResult
build_exact_direct_sparse_facet_descent_step(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

[[nodiscard]] ExactDirectSparseFacetDescentStepVerification
verify_exact_direct_sparse_facet_descent_step(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetDescentStepResult& observed);

}  // namespace morsehgp3d::hierarchy
