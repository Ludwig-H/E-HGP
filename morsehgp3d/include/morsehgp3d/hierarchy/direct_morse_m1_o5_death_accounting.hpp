#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_m1_o5_death_accounting_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_m1_o5_death_accounting_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_m1_o5_death_accounting_profile = "hgp_reduced";
inline constexpr std::string_view direct_morse_m1_o5_death_accounting_mode =
    "bounded_n14_conditional_direct_equal_level_m1_o5_death_accounting";
inline constexpr std::string_view
    direct_morse_m1_o5_death_accounting_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_m1_o5_death_accounting_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_m1_o5_death_accounting_proof_basis =
        "fresh_source_relative_event_seed_and_forest_replay_then_equal_level_"
        "carrier_equivalence_generated_first_by_common_prior_root_and_then_"
        "by_every_saddle_hyperedge_including_latent_hyperedges_with_batch_h0_"
        "death_identity_and_local_index_one_multiplicity_bound_only_v1";

// Phase 15 bounded qualification budget.  Every cap is checked before the
// O5 layer allocates a persistent audit or transient quotient arena.  A
// source scan counter charges every complete O5-layer pass over that record
// domain; indexed joins are charged to the pass that owns them.  Exact-level
// comparisons count the comparison expressions executed on the successful
// path.  The independently trusted source budgets remain arguments to the
// builder and verifier; observed source counters never steer their replay.
struct ExactDirectMorseM1O5DeathAccountingBudget {
  std::size_t maximum_event_audit_count{};
  std::size_t maximum_group_audit_count{};
  std::size_t maximum_batch_audit_count{};
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_source_arm_seed_scan_count{};
  std::size_t maximum_source_binding_scan_count{};
  std::size_t maximum_source_saddle_scan_count{};
  std::size_t maximum_source_group_scan_count{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_point_id_reference_scan_count{};
  std::size_t maximum_prior_root_reference_scan_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_logical_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseM1O5DeathAccountingBudget&,
      const ExactDirectMorseM1O5DeathAccountingBudget&) = default;
};

struct ExactDirectMorseM1O5EventAudit {
  std::size_t event_audit_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  contract::CanonicalId source_event_arm_identity_digest{};
  std::array<spatial::PointId, 4U> canonical_support_point_ids{};
  std::size_t support_point_count{};
  std::size_t arm_count{};
  std::size_t delta_one{};

  friend bool operator==(
      const ExactDirectMorseM1O5EventAudit&,
      const ExactDirectMorseM1O5EventAudit&) = default;
};

// The digest binds the sorted scientific arm identities in the quotient
// class.  It contains source event digests, removed PointIds and reconstructed
// F_u PointIds, but no process-local carrier, root or forest node identifier.
struct ExactDirectMorseM1O5GroupAudit {
  std::size_t group_audit_index{};
  std::size_t batch_audit_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  contract::CanonicalId canonical_group_digest{};
  std::size_t saddle_count{};
  std::size_t arm_count{};
  std::size_t distinct_carrier_count{};
  std::size_t prior_reduced_root_count{};
  std::size_t death_count{};
  ExactDirectMorseForestAtomicGroupKind kind{
      ExactDirectMorseForestAtomicGroupKind::reduced_birth};

  friend bool operator==(
      const ExactDirectMorseM1O5GroupAudit&,
      const ExactDirectMorseM1O5GroupAudit&) = default;
};

struct ExactDirectMorseM1O5BatchAudit {
  std::size_t batch_audit_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  std::size_t event_count{};
  std::size_t group_count{};
  std::size_t local_multiplicity_capacity{};
  std::size_t touched_distinct_prior_root_count{};
  std::size_t positive_root_group_count{};
  std::size_t global_death_count{};
  std::size_t sum_group_death_count{};

  friend bool operator==(
      const ExactDirectMorseM1O5BatchAudit&,
      const ExactDirectMorseM1O5BatchAudit&) = default;
};

struct ExactDirectMorseM1O5DeathAccountingCounters {
  std::size_t event_audit_count{};
  std::size_t group_audit_count{};
  std::size_t batch_audit_count{};
  std::size_t source_family_scan_count{};
  std::size_t source_arm_seed_scan_count{};
  std::size_t source_binding_scan_count{};
  std::size_t source_saddle_scan_count{};
  std::size_t source_group_scan_count{};
  std::size_t source_batch_scan_count{};
  std::size_t point_id_reference_scan_count{};
  std::size_t prior_root_reference_scan_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_batch_carrier_count{};
  std::size_t maximum_batch_saddle_count{};
  std::size_t peak_logical_scratch_entry_count{};
  std::size_t total_local_multiplicity_capacity{};
  std::size_t total_touched_distinct_prior_root_count{};
  std::size_t total_positive_root_group_count{};
  std::size_t total_global_death_count{};

  friend bool operator==(
      const ExactDirectMorseM1O5DeathAccountingCounters&,
      const ExactDirectMorseM1O5DeathAccountingCounters&) = default;
};

enum class ExactDirectMorseM1O5DeathAccountingDecision : std::uint8_t {
  not_certified,
  no_accounting_capacity_overflow,
  no_accounting_allocation_failed,
  no_accounting_budget_exhausted,
  no_accounting_point_count_or_order_rejected,
  no_accounting_source_event_journal_rejected,
  no_accounting_source_seed_journal_rejected,
  no_accounting_source_forest_rejected,
  no_accounting_source_join_inconsistent,
  no_accounting_arm_bijection_rejected,
  no_accounting_point_id_or_level_join_rejected,
  no_accounting_carrier_root_conflict,
  no_accounting_quotient_group_mismatch,
  no_accounting_death_identity_mismatch,
  no_accounting_local_multiplicity_bound_failed,
  complete_bounded_conditional_m1_o5_death_accounting,
};

enum class ExactDirectMorseM1O5DeathAccountingScope : std::uint8_t {
  unspecified,
  bounded_n14_fresh_direct_equal_level_hypergraph_local_multiplicity_and_global_h0_death_accounting_only,
};

struct ExactDirectMorseM1O5DeathAccountingResult {
  static constexpr std::string_view backend =
      direct_morse_m1_o5_death_accounting_backend;
  static constexpr std::string_view profile =
      direct_morse_m1_o5_death_accounting_profile;
  static constexpr std::string_view mode =
      direct_morse_m1_o5_death_accounting_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_m1_o5_death_accounting_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_m1_o5_death_accounting_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_m1_o5_death_accounting_proof_basis;

  std::uint32_t schema_version{
      direct_morse_m1_o5_death_accounting_schema_version};
  ExactDirectMorseM1O5DeathAccountingBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  contract::CanonicalId source_canonical_cloud_digest{};
  contract::CanonicalId event_audit_digest{};
  contract::CanonicalId group_audit_digest{};
  contract::CanonicalId batch_audit_digest{};
  std::size_t required_logical_output_entry_count{};
  ExactDirectMorseM1O5DeathAccountingCounters counters{};
  std::vector<ExactDirectMorseM1O5EventAudit> event_audits;
  std::vector<ExactDirectMorseM1O5GroupAudit> group_audits;
  std::vector<ExactDirectMorseM1O5BatchAudit> batch_audits;
  std::optional<std::size_t> rejected_source_family_index;
  std::optional<std::size_t> rejected_source_binding_index;
  std::optional<std::size_t> rejected_atomic_group_index;
  std::optional<std::size_t> rejected_source_batch_index;
  bool budget_preflight_certified{false};
  bool source_event_journal_freshly_replayed_relative_to_facade{false};
  bool source_seed_journal_freshly_replayed_relative_to_facade{false};
  bool source_forest_freshly_replayed_relative_to_facade{false};
  bool conditional_on_caller_fresh_phase9_facade_replay{true};
  bool every_index_one_event_has_complete_shell_arms{false};
  bool every_arm_seed_point_id_and_exact_level_joined{false};
  bool same_prior_root_carriers_unioned_before_saddle_hyperedges{false};
  bool every_saddle_hyperedge_including_latent_closed_transitively{false};
  bool simultaneous_carrier_quotient_reconstructed{false};
  bool quotient_partition_matches_forest_atomic_groups{false};
  bool group_root_counts_kinds_and_children_replayed{false};
  bool group_death_counts_replayed{false};
  bool batch_global_death_identity_replayed{false};
  bool local_index_one_multiplicity_bounds_replayed{false};
  bool m1_o5_combinatorial_accounting_replayed{false};
  bool event_local_death_attribution_serialized{false};
  bool durable_carrier_root_or_node_ids_serialized{false};
  bool source_catalog_complete_for_full_pi0{false};
  bool carrier_faithfulness_complete{false};
  bool silent_gamma_group_completeness{false};
  bool bidirectional_gamma_group_completeness{false};
  bool external_target_authority_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool global_m1_claimed{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool forest_semantics_exact{false};
  bool bounded_exhaustive_gamma_oracle_used{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool scalable_50k_claimed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  ExactDirectMorseM1O5DeathAccountingDecision decision{
      ExactDirectMorseM1O5DeathAccountingDecision::not_certified};
  ExactDirectMorseM1O5DeathAccountingScope scope{
      ExactDirectMorseM1O5DeathAccountingScope::unspecified};

  [[nodiscard]] bool certified_bounded_conditional_m1_o5_accounting()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseM1O5DeathAccountingResult&,
      const ExactDirectMorseM1O5DeathAccountingResult&) = default;
};

struct ExactDirectMorseM1O5DeathAccountingVerification {
  bool observed_storage_within_budget{false};
  bool source_event_journal_freshly_replayed{false};
  bool source_seed_journal_freshly_replayed{false};
  bool source_forest_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool trusted_inputs_accepted{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseM1O5DeathAccountingVerification&,
      const ExactDirectMorseM1O5DeathAccountingVerification&) = default;
};

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingResult
build_exact_direct_morse_m1_o5_death_accounting(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseM1O5DeathAccountingBudget& accounting_budget);

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingVerification
verify_exact_direct_morse_m1_o5_death_accounting(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseM1O5DeathAccountingBudget&
        trusted_accounting_budget,
    const ExactDirectMorseM1O5DeathAccountingResult& observed);

}  // namespace morsehgp3d::hierarchy
