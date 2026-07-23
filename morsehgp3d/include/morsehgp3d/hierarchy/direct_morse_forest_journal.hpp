#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_root_quotient.hpp"
#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_forest_journal_schema_version =
    2U;
inline constexpr std::string_view direct_morse_forest_journal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_journal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_journal_mode =
    "certified";
inline constexpr std::string_view
    direct_morse_forest_journal_refinement_status =
        "conditional_exact_h0";
inline constexpr std::string_view direct_morse_forest_journal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_forest_journal_proof_basis =
    "certified_phase_9_facade_fresh_10_1_10_2_direct_minimum_carriers_"
    "complete_consumption_of_direct_saddle_family_catalog_strict_10_5c_"
    "arm_terminals_"
    "typed_frozen_r_root_or_l_latent_carrier_hyperedges_deduplicated_"
    "before_transitive_full_component_quotient_qr_counts_only_r_"
    "then_atomic_all_carrier_union_and_minimum_binding_commits_v2";

using ExactDirectMorseForestNodeId = std::uint64_t;

struct ExactDirectMorseForestConfig {
  ExactDirectSparsePositiveFacetLocatorConfig locator_config{};
  ExactDirectSparseFacetDescentClosureConfig closure_config{};

  friend bool operator==(
      const ExactDirectMorseForestConfig&,
      const ExactDirectMorseForestConfig&) = default;
};

// Scalar caps cover the complete all-order replay.  The nested budgets are
// applied to the locator globally and to each 10.5c/quotient batch
// independently; aggregate closure caps prevent many individually bounded
// batches from hiding unbounded total work.
struct ExactDirectMorseForestBudget {
  std::size_t maximum_source_role_scan_count{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_source_arm_seed_scan_count{};
  std::size_t maximum_birth_record_count{};
  std::size_t maximum_arm_root_binding_count{};
  std::size_t maximum_saddle_record_count{};
  std::size_t maximum_atomic_group_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_batch_record_count{};
  std::size_t maximum_node_count{};
  std::size_t maximum_final_root_count{};
  std::size_t maximum_batch_distinct_arm_count{};
  std::size_t maximum_logical_output_entry_count{};
  std::size_t maximum_aggregate_closure_node_count{};
  std::size_t maximum_aggregate_closure_step_call_count{};
  ExactDirectSparsePositiveFacetLocatorBudget locator_budget{};
  ExactDirectSparseFacetDescentClosureBudget closure_budget{};
  ExactFrozenRootQuotientBudget quotient_budget{};

  friend bool operator==(
      const ExactDirectMorseForestBudget&,
      const ExactDirectMorseForestBudget&) = default;
};

enum class ExactDirectMorseForestNodeKind : std::uint8_t {
  order_one_birth,
  reduced_birth,
  multifusion,
};

struct ExactDirectMorseForestNode {
  ExactDirectMorseForestNodeId node_id{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  ExactDirectMorseForestNodeKind kind{
      ExactDirectMorseForestNodeKind::order_one_birth};
  std::size_t child_offset{};
  std::size_t child_count{};
  std::optional<std::size_t> birth_record_index;
  std::optional<std::size_t> atomic_group_index;

  friend bool operator==(
      const ExactDirectMorseForestNode&,
      const ExactDirectMorseForestNode&) = default;
};

struct ExactDirectMorseForestBirthRecord {
  std::size_t birth_record_index{};
  std::size_t source_event_projection_index{};
  std::size_t source_journal_batch_index{};
  std::size_t order{};
  ExactDirectSparseFacetKey facet_key{};
  ExactDirectSparseComponentHandle component_handle{};
  // Direct minima are carrier identities at every order.  Only order-one
  // minima are immediate hgp_reduced roots; for k>=2 the carrier remains
  // latent until a q_R=0 saddle group creates a reduced birth.
  std::optional<ExactDirectMorseForestNodeId> order_one_birth_node_id;
  ExactDirectSparseFacetWitness binding_witness{};

  friend bool operator==(
      const ExactDirectMorseForestBirthRecord&,
      const ExactDirectMorseForestBirthRecord&) = default;
};

struct ExactDirectMorseForestArmRootBinding {
  std::size_t binding_index{};
  std::size_t source_arm_seed_index{};
  std::size_t source_family_index{};
  ExactDirectSparseFacetKey strict_arm_key{};
  ExactDirectSparseComponentHandle frozen_carrier_component_handle{};
  // Presence encodes an R vertex; absence encodes an L vertex.  Both kinds
  // enter the frozen batch hypergraph before q_R is evaluated.
  std::optional<ExactDirectMorseForestNodeId>
      prior_reduced_root_node_id;

  friend bool operator==(
      const ExactDirectMorseForestArmRootBinding&,
      const ExactDirectMorseForestArmRootBinding&) = default;
};

struct ExactDirectMorseForestSaddleRecord {
  std::size_t saddle_record_index{};
  std::size_t source_family_index{};
  std::size_t source_event_index{};
  std::size_t source_journal_batch_index{};
  std::size_t arm_binding_offset{};
  std::size_t arm_binding_count{};
  std::size_t distinct_frozen_carrier_count{};
  std::size_t distinct_latent_carrier_count{};
  std::size_t distinct_prior_reduced_root_count{};
  std::size_t atomic_group_index{};

  friend bool operator==(
      const ExactDirectMorseForestSaddleRecord&,
      const ExactDirectMorseForestSaddleRecord&) = default;
};

// Saddle records are deliberately reordered by quotient-group order; their
// source_family_index retains canonical source provenance.  This makes each
// group slice contiguous even for a source pattern such as A, B, A.
//
enum class ExactDirectMorseForestAtomicGroupKind : std::uint8_t {
  reduced_birth,
  continuation,
  multifusion,
};

// The equal-level quotient is formed transitively over all saddle families on
// every frozen full-component carrier, including L carriers with no prior
// reduced root.  q_R counts only the distinct R roots attached to that
// carrier class.  q_R=0 creates one reduced birth without children, q_R=1 is
// a continuation with no created node, and q_R>=2 creates one multifusion
// whose complete frozen child set occupies the child arena.
struct ExactDirectMorseForestAtomicGroup {
  std::size_t atomic_group_index{};
  std::size_t batch_index{};
  std::size_t saddle_record_offset{};
  std::size_t saddle_record_count{};
  std::size_t frozen_carrier_count{};
  std::size_t latent_carrier_count{};
  std::size_t prior_reduced_root_count{};
  std::size_t child_offset{};
  std::size_t child_count{};
  std::optional<ExactDirectMorseForestNodeId> created_node_id;
  ExactDirectMorseForestNodeId resulting_root_node_id{};
  ExactDirectMorseForestAtomicGroupKind kind{
      ExactDirectMorseForestAtomicGroupKind::reduced_birth};

  friend bool operator==(
      const ExactDirectMorseForestAtomicGroup&,
      const ExactDirectMorseForestAtomicGroup&) = default;
};

struct ExactDirectMorseForestBatch {
  std::size_t batch_index{};
  std::size_t source_journal_batch_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  std::size_t birth_record_offset{};
  std::size_t birth_record_count{};
  std::size_t saddle_record_offset{};
  std::size_t saddle_record_count{};
  std::size_t atomic_group_offset{};
  std::size_t atomic_group_count{};
  std::size_t strict_pre_batch_carrier_count{};
  std::size_t strict_pre_batch_reduced_root_count{};
  std::size_t closed_post_batch_carrier_count{};
  std::size_t closed_post_batch_reduced_root_count{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp strict_pre_batch_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp committed_batch_stamp{};
  bool strict_arms_resolved_before_mutation{false};
  bool quotient_resolved_before_mutation{false};
  bool unions_then_births_committed_atomically{false};

  friend bool operator==(
      const ExactDirectMorseForestBatch&,
      const ExactDirectMorseForestBatch&) = default;
};

struct ExactDirectMorseForestFinalRoot {
  std::size_t final_root_index{};
  std::size_t order{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectMorseForestNodeId root_node_id{};

  friend bool operator==(
      const ExactDirectMorseForestFinalRoot&,
      const ExactDirectMorseForestFinalRoot&) = default;
};

struct ExactDirectMorseForestCounters {
  std::size_t birth_record_count{};
  std::size_t latent_higher_order_birth_count{};
  std::size_t order_one_birth_node_count{};
  std::size_t arm_root_binding_count{};
  std::size_t saddle_record_count{};
  std::size_t atomic_group_count{};
  std::size_t reduced_birth_group_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t child_reference_count{};
  std::size_t batch_record_count{};
  std::size_t node_count{};
  std::size_t final_root_count{};
  std::size_t locator_union_count{};
  std::size_t closure_call_count{};
  std::size_t quotient_call_count{};
  std::size_t distinct_strict_arm_count{};
  std::size_t duplicate_strict_arm_reference_count{};
  std::size_t aggregate_closure_node_count{};
  std::size_t aggregate_closure_step_call_count{};
  std::size_t maximum_batch_arm_count{};
  std::size_t maximum_batch_carrier_arity{};
  std::size_t maximum_batch_merge_arity{};

  friend bool operator==(
      const ExactDirectMorseForestCounters&,
      const ExactDirectMorseForestCounters&) = default;
};

enum class ExactDirectMorseForestDecision : std::uint8_t {
  not_certified,
  no_forest_capacity_overflow,
  no_forest_allocation_failed,
  no_forest_budget_exhausted,
  no_forest_source_rejected,
  no_forest_locator_initialization,
  no_forest_source_batch_inconsistent,
  no_forest_strict_arm_closure_budget_exhausted,
  no_forest_strict_arm_unresolved,
  no_forest_strict_arm_contradiction,
  no_forest_zero_carrier_saddle,
  no_forest_frozen_carrier_quotient_rejected,
  no_forest_locator_commit_rejected,
  complete_conditional_exact_direct_morse_forest,
};

enum class ExactDirectMorseForestScope : std::uint8_t {
  unspecified,
  all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only,
};

struct ExactDirectMorseForestJournalResult {
  static constexpr std::string_view backend =
      direct_morse_forest_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_journal_profile;
  static constexpr std::string_view mode = direct_morse_forest_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_morse_forest_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_forest_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_journal_proof_basis;

  std::uint32_t schema_version{direct_morse_forest_journal_schema_version};
  ExactDirectMorseForestBudget requested_budget{};
  ExactDirectMorseForestConfig config{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp final_locator_stamp{};

  std::vector<ExactDirectMorseForestBirthRecord> birth_records;
  std::vector<ExactDirectMorseForestArmRootBinding> arm_root_bindings;
  std::vector<ExactDirectMorseForestSaddleRecord> saddle_records;
  std::vector<ExactDirectMorseForestAtomicGroup> atomic_groups;
  std::vector<ExactDirectMorseForestNodeId> child_node_ids;
  std::vector<ExactDirectMorseForestBatch> batches;
  std::vector<ExactDirectMorseForestFinalRoot> final_roots;
  std::vector<ExactDirectMorseForestNode> nodes;

  std::size_t logical_output_entry_count{};
  ExactDirectMorseForestCounters counters{};
  bool budget_preflight_certified{false};
  bool source_phase9_facade_freshly_replayed{false};
  bool conditional_on_caller_fresh_phase9_facade_replay{true};
  bool source_event_journal_freshly_replayed{false};
  bool source_strict_arm_journal_freshly_replayed{false};
  bool every_birth_key_reconstructed_from_closed_direct_event{false};
  bool deterministic_disjoint_birth_union_and_query_tokens{false};
  bool batches_processed_in_strict_order_level_order{false};
  bool cardinality_isolates_orders_in_shared_locator{false};
  bool current_level_births_hidden_from_arm_descent{false};
  bool higher_order_direct_births_are_latent_carriers{false};
  bool one_10_5c_call_per_nonempty_strict_arm_batch{false};
  bool every_strict_arm_has_positive_terminal{false};
  bool all_catalogued_saddle_families_consumed_once{false};
  bool carrier_to_optional_reduced_root_authority_maintained{false};
  bool every_saddle_has_positive_carrier{false};
  bool typed_root_or_latent_carrier_hyperedges_closed_transitively{false};
  bool q_r_counts_only_distinct_prior_reduced_roots{false};
  bool all_equal_level_saddles_quotiented_before_mutation{false};
  bool saddle_records_grouped_with_source_family_provenance{false};
  bool q_zero_groups_create_one_reduced_birth{false};
  bool q_one_continuations_create_no_node{false};
  bool q_at_least_two_groups_create_one_multifusion{false};
  bool current_batch_birth_nodes_never_same_batch_children{false};
  bool all_group_carriers_attached_to_resulting_root_atomically{false};
  bool locator_commits_unions_before_current_birth_bindings{false};
  bool final_roots_cover_exactly_nonterminal_reduced_orders{false};
  bool no_partial_scientific_payload_published{false};
  bool external_locator_authority_replayed{true};
  bool conditional_on_caller_external_locator_authority_replay{false};
  bool global_morse_obligation_replayed{false};
  bool conditional_on_separate_global_morse_obligation{true};
  bool equal_or_interior_facets_consumed{false};
  bool gateway_10_6_or_later_consumed{false};
  bool closure_graph_persisted{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool conditional_exact_h0_only{true};
  ExactDirectMorseForestDecision decision{
      ExactDirectMorseForestDecision::not_certified};
  ExactDirectMorseForestScope scope{
      ExactDirectMorseForestScope::unspecified};

  [[nodiscard]] bool certified_conditional_exact_h0() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestJournalResult&,
      const ExactDirectMorseForestJournalResult&) = default;
};

struct ExactDirectMorseForestVerification {
  bool trusted_inputs_certified{false};
  bool observed_storage_within_budget{false};
  bool expected_journal_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool source_phase9_facade_freshly_replayed{false};
  bool conditional_on_caller_fresh_phase9_facade_replay{true};
  bool external_locator_authority_replayed{true};
  bool conditional_on_caller_external_locator_authority_replay{false};
  bool global_morse_obligation_replayed{false};
  bool conditional_on_separate_global_morse_obligation{true};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseForestVerification&,
      const ExactDirectMorseForestVerification&) = default;
};

[[nodiscard]] ExactDirectMorseForestJournalResult
build_exact_direct_morse_forest_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

[[nodiscard]] ExactDirectMorseForestVerification
verify_exact_direct_morse_forest_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
