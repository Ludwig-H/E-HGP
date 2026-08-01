#pragma once

#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_pipeline.hpp"
#include "morsehgp3d/hierarchy/exact_k2_k1_hierarchy.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_cut.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_k2_k1_target_authority_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_mode =
        "bounded_n14_k2_to_k1_observed_label_external_target_authority_"
        "conformance";
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_k2_k1_target_authority_proof_basis =
        "fresh_direct_pipeline_and_vertical_journal_replay_then_fresh_bounded_"
        "exhaustive_gamma2_to_complete_graph_emst_k1_authority_replay_with_"
        "canonical_PointId_coverage_equality_for_every_observed_resolved_"
        "direct_k2_to_k1_label_only_v1";

// The exhaustive Gamma2/K1 inputs are an intentionally bounded falsification
// and conformance oracle.  They are never retained by the product result and
// this budget is accepted only for point_count <= 14.  The nested cut budget
// is trusted independently; no capacity echoed by an observed receipt steers
// a replay.
struct ExactDirectMorseK2K1TargetAuthorityBudget {
  ExactReducedGammaCutBudget closed_gamma_cut_budget{};
  std::size_t maximum_forest_node_scan_count{};
  std::size_t maximum_forest_child_reference_scan_count{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_source_binding_scan_count{};
  std::size_t maximum_label_resolution_scan_count{};
  std::size_t maximum_group_check_scan_count{};
  std::size_t maximum_gamma_cut_build_count{};
  std::size_t maximum_gamma_active_root_scan_count{};
  std::size_t maximum_gamma_facet_scan_count{};
  std::size_t maximum_external_checkpoint_scan_count{};
  std::size_t maximum_direct_target_point_reference_count{};
  std::size_t maximum_external_target_point_reference_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseK2K1TargetAuthorityBudget&,
      const ExactDirectMorseK2K1TargetAuthorityBudget&) = default;
};

struct ExactDirectMorseK2K1TargetAuthorityCounters {
  std::size_t forest_node_scan_count{};
  std::size_t forest_child_reference_scan_count{};
  std::size_t source_batch_scan_count{};
  std::size_t source_binding_scan_count{};
  std::size_t label_resolution_scan_count{};
  std::size_t group_check_scan_count{};
  std::size_t observed_k2_k1_label_count{};
  std::size_t resolved_k2_k1_label_count{};
  std::size_t gamma_cut_build_count{};
  std::size_t gamma_active_root_scan_count{};
  std::size_t gamma_facet_scan_count{};
  std::size_t external_checkpoint_scan_count{};
  std::size_t direct_target_point_reference_count{};
  std::size_t external_target_point_reference_count{};
  std::size_t certified_target_coverage_equality_count{};

  friend bool operator==(
      const ExactDirectMorseK2K1TargetAuthorityCounters&,
      const ExactDirectMorseK2K1TargetAuthorityCounters&) = default;
};

enum class ExactDirectMorseK2K1TargetAuthorityDecision : std::uint8_t {
  not_certified,
  no_authority_capacity_overflow,
  no_authority_budget_exhausted,
  no_authority_allocation_failed,
  no_authority_point_count_or_order_rejected,
  no_authority_direct_pipeline_rejected,
  no_authority_direct_journal_rejected,
  no_authority_external_hierarchy_rejected,
  no_authority_label_partition_rejected,
  no_authority_observed_k2_k1_label_missing,
  no_authority_observed_k2_k1_label_unresolved,
  no_authority_source_binding_rejected,
  no_authority_gamma_cut_rejected,
  no_authority_gamma_component_rejected,
  no_authority_external_checkpoint_rejected,
  no_authority_direct_target_rejected,
  no_authority_target_coverage_mismatch,
  complete_observed_resolved_k2_k1_label_target_authority_replay,
};

enum class ExactDirectMorseK2K1TargetAuthorityScope : std::uint8_t {
  unspecified,
  bounded_n14_all_observed_resolved_direct_k2_k1_labels_against_fresh_gamma2_emst_k1_targets_only,
};

// This receipt intentionally contains no facet, coface, Gamma component,
// direct-node coverage or external-node coverage arena.  The optional indices
// are non-scientific first-failure diagnostics and are empty on success.
struct ExactDirectMorseK2K1TargetAuthorityResult {
  static constexpr std::string_view backend =
      direct_morse_k2_k1_target_authority_backend;
  static constexpr std::string_view profile =
      direct_morse_k2_k1_target_authority_profile;
  static constexpr std::string_view mode =
      direct_morse_k2_k1_target_authority_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_k2_k1_target_authority_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_k2_k1_target_authority_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_k2_k1_target_authority_proof_basis;

  std::uint32_t schema_version{
      direct_morse_k2_k1_target_authority_schema_version};
  ExactDirectMorseK2K1TargetAuthorityBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_order{2U};
  std::size_t target_order{1U};
  contract::CanonicalId direct_canonical_cloud_digest{};
  contract::CanonicalId external_canonical_cloud_digest{};
  contract::CanonicalId external_hierarchy_projection_digest{};
  std::size_t required_logical_output_entry_count{};
  ExactDirectMorseK2K1TargetAuthorityCounters counters{};
  std::optional<std::size_t> rejected_label_resolution_index;
  std::optional<std::size_t> rejected_atomic_group_index;
  std::optional<std::size_t> rejected_source_binding_index;
  bool direct_pipeline_freshly_replayed{false};
  bool direct_vertical_journal_freshly_replayed{false};
  bool external_k2_k1_hierarchy_freshly_replayed{false};
  bool canonical_point_namespace_identity_certified{false};
  bool all_observed_k2_k1_labels_present{false};
  bool all_observed_k2_k1_labels_resolved{false};
  bool every_direct_target_coverage_reconstructed_transiently{false};
  bool every_external_target_coverage_reconstructed_transiently{false};
  bool every_observed_k2_k1_target_coverage_equal{false};
  bool k2_to_k1_observed_label_target_authority_replayed{false};
  bool bidirectional_gamma_group_completeness_replayed{false};
  bool silent_gamma_checkpoint_completeness_replayed{false};
  bool external_target_authority_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool global_m1_claimed{false};
  bool bounded_exhaustive_gamma_oracle_used{false};
  bool gamma_cells_or_global_cofaces_persisted{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  ExactDirectMorseK2K1TargetAuthorityDecision decision{
      ExactDirectMorseK2K1TargetAuthorityDecision::not_certified};
  ExactDirectMorseK2K1TargetAuthorityScope scope{
      ExactDirectMorseK2K1TargetAuthorityScope::unspecified};

  [[nodiscard]] bool certified_observed_label_target_authority()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseK2K1TargetAuthorityResult&,
      const ExactDirectMorseK2K1TargetAuthorityResult&) = default;
};

struct ExactDirectMorseK2K1TargetAuthorityVerification {
  bool trusted_inputs_accepted{false};
  bool direct_pipeline_freshly_replayed{false};
  bool direct_vertical_journal_freshly_replayed{false};
  bool external_k2_k1_hierarchy_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool observed_storage_within_budget{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseK2K1TargetAuthorityVerification&,
      const ExactDirectMorseK2K1TargetAuthorityVerification&) = default;
};

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityResult
build_exact_direct_morse_k2_k1_target_authority(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseForestJournalResult& direct_forest,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget&
        trusted_pipeline_budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult&
        observed_pipeline,
    const ExactDirectMorseVerticalBudget& trusted_vertical_budget,
    const ExactDirectMorseVerticalConfig& trusted_vertical_config,
    const ExactDirectMorseVerticalJournalResult& observed_vertical_journal,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_history_budget,
    const ExactPersistentReducedGammaOrderHistory& external_source_history,
    const K1CompactForest& external_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_external_hierarchy_budget,
    const ExactK2K1HierarchyResult& observed_external_hierarchy,
    const ExactDirectMorseK2K1TargetAuthorityBudget& authority_budget);

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityVerification
verify_exact_direct_morse_k2_k1_target_authority(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseForestJournalResult& direct_forest,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget&
        trusted_pipeline_budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult&
        observed_pipeline,
    const ExactDirectMorseVerticalBudget& trusted_vertical_budget,
    const ExactDirectMorseVerticalConfig& trusted_vertical_config,
    const ExactDirectMorseVerticalJournalResult& observed_vertical_journal,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_history_budget,
    const ExactPersistentReducedGammaOrderHistory& external_source_history,
    const K1CompactForest& external_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_external_hierarchy_budget,
    const ExactK2K1HierarchyResult& observed_external_hierarchy,
    const ExactDirectMorseK2K1TargetAuthorityBudget& trusted_authority_budget,
    const ExactDirectMorseK2K1TargetAuthorityResult& observed);

}  // namespace morsehgp3d::hierarchy
