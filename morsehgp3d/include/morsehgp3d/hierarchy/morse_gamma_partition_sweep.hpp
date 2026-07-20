#pragma once

#include "morsehgp3d/hierarchy/critical_arm.hpp"
#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// This budget keeps the Morse construction and its exhaustive Gamma audit
// separate.  The catalogue and arm budgets drive the candidate genealogy;
// the history budget is consumed only after that genealogy is complete and
// supplies every exact Gamma activation level to the posterior audit.
struct ExactMorseGammaPartitionSweepBudget {
  static constexpr std::size_t maximum_supported_birth_record_count = 1456U;
  static constexpr std::size_t maximum_supported_saddle_record_count = 1456U;
  static constexpr std::size_t maximum_supported_arm_reference_count = 5824U;
  static constexpr std::size_t maximum_supported_node_count = 2911U;
  static constexpr std::size_t maximum_supported_child_reference_count =
      2910U;
  static constexpr std::size_t maximum_supported_batch_record_count = 1456U;
  static constexpr std::size_t maximum_supported_contraction_group_count =
      1456U;
  static constexpr std::size_t maximum_supported_group_root_reference_count =
      5824U;
  static constexpr std::size_t maximum_supported_batch_reference_count =
      4368U;
  static constexpr std::size_t maximum_supported_checkpoint_count = 6435U;

  ExactCriticalCatalogBudget critical_catalog_budget{};
  ExactFacetDescentChainBudget per_arm_chain_budget{};
  ExactPersistentReducedGammaOrderHistoryBudget gamma_oracle_history_budget{};
  std::size_t maximum_birth_record_count{};
  std::size_t maximum_saddle_record_count{};
  std::size_t maximum_arm_reference_count{};
  std::size_t maximum_node_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_batch_record_count{};
  std::size_t maximum_contraction_group_count{};
  std::size_t maximum_group_root_reference_count{};
  std::size_t maximum_batch_reference_count{};
  std::size_t maximum_checkpoint_count{};

  friend bool operator==(
      const ExactMorseGammaPartitionSweepBudget&,
      const ExactMorseGammaPartitionSweepBudget&) = default;
};

enum class ExactMorseGammaPartitionSweepDecision : std::uint8_t {
  not_certified,
  no_sweep_preflight_budget_insufficient,
  no_sweep_critical_catalog_rejected,
  no_sweep_critical_arm_family_incomplete,
  no_sweep_terminal_birth_not_unique,
  no_sweep_gamma_oracle_rejected,
  no_sweep_morse_gamma_partition_mismatch,
  complete_morse_gamma_partition_sweep,
};

enum class ExactMorseGammaPartitionSweepScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only,
};

enum class ExactMorseGammaNodeKind : std::uint8_t {
  birth,
  multifusion,
};

// These are deterministic local replay identifiers.  They deliberately are
// neither v2 CanonicalId values nor public MergeNode identifiers.
struct ExactMorseGammaNode {
  std::size_t node_id{};
  exact::ExactLevel squared_level;
  ExactMorseGammaNodeKind kind{ExactMorseGammaNodeKind::birth};
  std::vector<std::size_t> child_node_ids;
  std::vector<std::size_t> catalog_event_indices;

  friend bool operator==(
      const ExactMorseGammaNode&,
      const ExactMorseGammaNode&) = default;
};

struct ExactMorseGammaBirthRecord {
  std::size_t birth_record_index{};
  std::size_t catalog_event_index{};
  std::size_t node_id{};
  exact::ExactLevel squared_level;
  std::vector<spatial::PointId> facet_point_ids;

  friend bool operator==(
      const ExactMorseGammaBirthRecord&,
      const ExactMorseGammaBirthRecord&) = default;
};

struct ExactMorseGammaSaddleRecord {
  std::size_t saddle_record_index{};
  std::size_t catalog_event_index{};
  std::size_t batch_index{};
  exact::ExactLevel squared_level;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<std::size_t> terminal_birth_record_indices;
  std::vector<std::size_t> pre_batch_root_node_ids;
  std::size_t contraction_group_index{};

  friend bool operator==(
      const ExactMorseGammaSaddleRecord&,
      const ExactMorseGammaSaddleRecord&) = default;
};

struct ExactMorseGammaContractionGroup {
  std::size_t contraction_group_index{};
  std::size_t batch_index{};
  std::vector<std::size_t> saddle_record_indices;
  std::vector<std::size_t> prior_root_node_ids;
  std::optional<std::size_t> created_node_id;
  std::size_t resulting_root_node_id{};

  friend bool operator==(
      const ExactMorseGammaContractionGroup&,
      const ExactMorseGammaContractionGroup&) = default;
};

struct ExactMorseGammaBatchRecord {
  std::size_t batch_index{};
  std::size_t catalog_h0_batch_index{};
  exact::ExactLevel squared_level;
  std::vector<std::size_t> birth_record_indices;
  std::vector<std::size_t> saddle_record_indices;
  std::vector<std::size_t> contraction_group_indices;
  std::size_t strict_root_count{};
  std::size_t closed_root_count{};
  bool all_saddles_resolved_from_frozen_pre_batch_roots{false};
  bool quotient_components_invariant_under_reversed_saddle_order{false};
  bool mutations_committed_after_complete_group_resolution{false};

  friend bool operator==(
      const ExactMorseGammaBatchRecord&,
      const ExactMorseGammaBatchRecord&) = default;
};

struct ExactMorseGammaOracleCheckpoint {
  std::size_t checkpoint_index{};
  std::size_t activation_level_index{};
  exact::ExactLevel squared_level;
  std::optional<std::size_t> morse_batch_index;
  std::size_t strict_morse_root_count{};
  std::size_t strict_gamma_component_count{};
  std::size_t closed_morse_root_count{};
  std::size_t closed_gamma_component_count{};
  bool strict_birth_projection_is_bijective{false};
  bool closed_birth_projection_is_bijective{false};

  friend bool operator==(
      const ExactMorseGammaOracleCheckpoint&,
      const ExactMorseGammaOracleCheckpoint&) = default;
};

enum class ExactMorseGammaPartitionMismatchStage : std::uint8_t {
  strict_open,
  closed,
};

enum class ExactMorseGammaPartitionMismatchKind : std::uint8_t {
  morse_batch_level_absent_from_gamma_activation_catalog,
  active_birth_facet_absent_from_gamma,
  gamma_component_without_catalog_birth,
  one_morse_root_spans_multiple_gamma_components,
  one_gamma_component_spans_multiple_morse_roots,
};

struct ExactMorseGammaPartitionMismatchWitness {
  exact::ExactLevel squared_level;
  ExactMorseGammaPartitionMismatchStage stage{
      ExactMorseGammaPartitionMismatchStage::strict_open};
  ExactMorseGammaPartitionMismatchKind kind{
      ExactMorseGammaPartitionMismatchKind::active_birth_facet_absent_from_gamma};
  std::optional<std::size_t> birth_record_index;
  std::optional<std::size_t> root_node_id;
  std::optional<std::size_t> gamma_component_index;
  // Canonical labels keep a diagnostic witness meaningful when the local
  // genealogy and Gamma arenas are deliberately withheld atomically.
  std::vector<spatial::PointId> birth_facet_point_ids;
  std::vector<spatial::PointId>
      gamma_component_representative_facet_point_ids;

  friend bool operator==(
      const ExactMorseGammaPartitionMismatchWitness&,
      const ExactMorseGammaPartitionMismatchWitness&) = default;
};

struct ExactMorseGammaPartitionSweepCounters {
  std::size_t preflight_count{};
  std::size_t critical_catalog_build_count{};
  std::size_t critical_arm_family_build_count{};
  std::size_t birth_record_count{};
  std::size_t saddle_record_count{};
  std::size_t arm_reference_count{};
  std::size_t terminal_birth_lookup_count{};
  std::size_t batch_record_count{};
  std::size_t contraction_group_count{};
  std::size_t group_root_reference_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t node_count{};
  std::size_t child_reference_count{};
  std::size_t batch_reference_count{};
  std::size_t reversed_order_group_comparison_count{};
  std::size_t gamma_history_build_count{};
  std::size_t gamma_transition_build_count{};
  std::size_t checkpoint_count{};
  std::size_t strict_birth_component_lookup_count{};
  std::size_t closed_birth_component_lookup_count{};
  std::size_t strict_partition_bijection_count{};
  std::size_t closed_partition_bijection_count{};

  friend bool operator==(
      const ExactMorseGammaPartitionSweepCounters&,
      const ExactMorseGammaPartitionSweepCounters&) = default;
};

struct ExactMorseGammaPartitionSweepResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_catalog_minima_and_strict_arm_terminal_hyperkruskal_"
      "partition_sweep_with_posterior_exhaustive_gamma_oracle_v1";

  ExactMorseGammaPartitionSweepBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t required_birth_record_capacity{};
  std::size_t required_saddle_record_capacity{};
  std::size_t required_arm_reference_capacity{};
  std::size_t required_node_capacity{};
  std::size_t required_child_reference_capacity{};
  std::size_t required_batch_record_capacity{};
  std::size_t required_contraction_group_capacity{};
  std::size_t required_group_root_reference_capacity{};
  std::size_t required_batch_reference_capacity{};
  std::size_t required_checkpoint_capacity{};
  ExactCriticalCatalogDecision critical_catalog_decision{
      ExactCriticalCatalogDecision::not_certified};
  ExactPersistentReducedGammaOrderHistoryDecision gamma_history_decision{
      ExactPersistentReducedGammaOrderHistoryDecision::not_certified};
  std::optional<std::size_t> incomplete_saddle_catalog_event_index;
  std::optional<ExactMorseGammaPartitionMismatchWitness> mismatch_witness;
  std::vector<ExactMorseGammaBirthRecord> birth_records;
  std::vector<ExactMorseGammaSaddleRecord> saddle_records;
  std::vector<ExactMorseGammaNode> nodes;
  std::vector<ExactMorseGammaContractionGroup> contraction_groups;
  std::vector<ExactMorseGammaBatchRecord> batch_records;
  std::vector<ExactMorseGammaOracleCheckpoint> oracle_checkpoints;
  std::vector<std::size_t> final_root_node_ids;
  bool conservative_preflight_bounds_certified{false};
  bool preflight_budget_sufficient{false};
  bool critical_catalog_fresh_and_generic{false};
  bool every_rank_k_birth_has_one_canonical_record{false};
  bool every_rank_k_plus_one_saddle_family_is_complete{false};
  bool every_arm_terminal_maps_to_one_strictly_earlier_birth{false};
  bool all_saddle_targets_resolved_from_frozen_pre_batch_roots{false};
  bool equal_level_saddles_contracted_as_one_hypergraph{false};
  bool contractions_invariant_under_saddle_permutation{false};
  bool local_genealogy_is_canonical_and_acyclic{false};
  bool gamma_oracle_started_only_after_complete_morse_genealogy{false};
  bool gamma_activation_catalog_fresh_and_complete{false};
  bool every_morse_batch_level_is_a_gamma_activation_level{false};
  bool strict_partitions_biject_gamma_at_every_activation_level{false};
  bool closed_partitions_biject_gamma_at_every_activation_level{false};
  bool gamma_objects_never_select_morse_births_targets_or_unions{false};
  bool records_are_internal_falsifier_objects_not_public_forest_or_attachments{
      false};
  bool diagnostic_outcomes_have_no_genealogy_payload{false};
  bool morse_gamma_partition_sweep_certified{false};
  ExactMorseGammaPartitionSweepCounters counters{};
  ExactMorseGammaPartitionSweepDecision decision{
      ExactMorseGammaPartitionSweepDecision::not_certified};
  ExactMorseGammaPartitionSweepScope scope{
      ExactMorseGammaPartitionSweepScope::unspecified};

  friend bool operator==(
      const ExactMorseGammaPartitionSweepResult&,
      const ExactMorseGammaPartitionSweepResult&) = default;
};

struct ExactMorseGammaPartitionSweepVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool subordinate_decisions_certified{false};
  bool diagnostic_witness_certified{false};
  bool birth_records_certified{false};
  bool saddle_records_certified{false};
  bool nodes_certified{false};
  bool contraction_groups_certified{false};
  bool batch_records_certified{false};
  bool oracle_checkpoints_certified{false};
  bool final_roots_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_morse_gamma_partition_sweep_decision_certified{false};

  friend bool operator==(
      const ExactMorseGammaPartitionSweepVerification&,
      const ExactMorseGammaPartitionSweepVerification&) = default;
};

void validate_exact_morse_gamma_partition_sweep_budget_caps(
    const ExactMorseGammaPartitionSweepBudget& budget);

[[nodiscard]] ExactMorseGammaPartitionSweepResult
build_exact_morse_gamma_partition_sweep(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactMorseGammaPartitionSweepBudget budget);

// Rebuilds both the event-only Morse genealogy and the posterior Gamma audit
// from the cloud, order and trusted budgets.  No observed local identifier,
// root, checkpoint, decision or mismatch witness steers the replay.
[[nodiscard]] ExactMorseGammaPartitionSweepVerification
verify_exact_morse_gamma_partition_sweep(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactMorseGammaPartitionSweepBudget budget,
    const ExactMorseGammaPartitionSweepResult& result);

}  // namespace morsehgp3d::hierarchy
