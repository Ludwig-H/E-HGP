#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_facet_descent_batch_executor_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_mode = "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_executor_proof_basis =
        "one_fresh_14c_session_anchor_canonical_batch_cursor_stable_family_"
        "selection_distinct_full_key_shared_10_5c_closure_compact_arm_join_"
        "and_transient_graph_release_before_delta_publication_v1";

// The closure owns a separate budget.  These caps cover every population
// retained while selecting one exact batch and every record that survives in
// its compact delta.  The key-reference cap bounds each full-key population:
// K*A for stable selection and K*D for the compact output, with D <= A.
struct ExactDirectSparseFacetDescentBatchExecutionBudget {
  std::size_t maximum_selected_lane_count{};
  std::size_t maximum_selected_family_count{};
  std::size_t maximum_selected_arm_seed_count{};
  std::size_t maximum_selected_key_point_reference_count{};
  std::size_t maximum_resolved_key_count{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionBudget&,
      const ExactDirectSparseFacetDescentBatchExecutionBudget&) = default;
};

// One source key is retained exactly once.  The component and witness come
// from its positive 10.5c terminal.  No closure-node index escapes the local
// arena because that arena is destroyed before the delta is returned.
struct ExactDirectSparseFacetDescentBatchResolvedKey {
  std::size_t resolved_key_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseComponentHandle resolved_component_handle{};
  ExactDirectSparseFacetWitness resolved_binding_witness{};
  ExactDirectSparseFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  bool source_projection_and_terminal_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchResolvedKey&,
      const ExactDirectSparseFacetDescentBatchResolvedKey&) = default;
};

// arm_seed_index remains the durable 10.2 identity.  resolved_key_index joins
// it to one full key and one carrier without duplicating K PointIds per arm.
struct ExactDirectSparseFacetDescentBatchArmJoin {
  std::size_t arm_seed_index{};
  std::size_t family_index{};
  std::size_t lane_index{};
  std::size_t resolved_key_index{};
  bool arm_identity_and_full_key_joined{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchArmJoin&,
      const ExactDirectSparseFacetDescentBatchArmJoin&) = default;
};

// This summary is deliberately the only surviving view of 10.5c.  It retains
// counts and decisions, never nodes, edges, seed projections or miniballs.
struct ExactDirectSparseFacetDescentBatchClosureSummary {
  std::size_t transient_node_count{};
  std::size_t transient_edge_count{};
  std::size_t transient_seed_projection_count{};
  std::size_t required_memo_slot_count{};
  ExactDirectSparseFacetDescentClosureCounters counters{};
  ExactDirectSparseFacetDescentClosureDisposition disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseFacetDescentClosureDecision decision{
      ExactDirectSparseFacetDescentClosureDecision::not_certified};
  bool complete_relative_positive{false};
  bool graph_payload_persisted{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchClosureSummary&,
      const ExactDirectSparseFacetDescentBatchClosureSummary&) = default;
};

struct ExactDirectSparseFacetDescentBatchExecutionCounters {
  std::size_t anchored_source_plan_verification_count{};
  std::size_t batch_local_source_plan_rebuild_count{};
  std::size_t selected_lane_reference_count{};
  std::size_t selected_family_scan_count{};
  std::size_t selected_arm_seed_scan_count{};
  std::size_t reconstructed_facet_count{};
  std::size_t distinct_closure_seed_count{};
  std::size_t duplicate_arm_key_reference_count{};
  std::size_t shared_closure_build_count{};
  std::size_t resolved_key_projection_count{};
  std::size_t arm_join_count{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionCounters&,
      const ExactDirectSparseFacetDescentBatchExecutionCounters&) = default;
};

enum class ExactDirectSparseFacetDescentBatchExecutionDecision
    : std::uint8_t {
  not_executed,
  no_execution_source_join_inconsistent,
  no_execution_capacity_overflow,
  no_execution_batch_budget_exhausted,
  no_execution_allocation_failed,
  no_execution_shared_closure_budget_exhausted,
  no_execution_shared_closure_unresolved,
  no_execution_shared_closure_contradiction,
  no_execution_shared_closure_rejected,
  complete_architecture_only_relative_positive_batch_delta,
};

enum class ExactDirectSparseFacetDescentBatchExecutionScope
    : std::uint8_t {
  unspecified,
  one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only,
};

struct ExactDirectSparseFacetDescentBatchExecutionResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_batch_executor_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_batch_executor_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_batch_executor_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_descent_batch_executor_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_batch_executor_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_batch_executor_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_batch_executor_schema_version};
  ExactDirectSparseFacetDescentBatchExecutionBudget requested_budget{};
  ExactDirectSparseFacetDescentClosureBudget requested_closure_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t source_batch_index{};
  std::optional<std::size_t> source_chunk_index;
  std::size_t source_lane_begin_index{};
  std::size_t source_lane_end_index{};
  std::size_t source_family_begin_index{};
  std::size_t source_family_end_index{};
  std::size_t source_arm_seed_begin_index{};
  std::size_t source_arm_seed_end_index{};
  std::size_t source_facet_cardinality{};
  exact::ExactLevel closed_batch_squared_level{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t required_selected_lane_count{};
  std::size_t required_selected_family_count{};
  std::size_t required_selected_arm_seed_count{};
  std::size_t required_selected_key_point_reference_count{};
  std::size_t required_resolved_key_count{};
  ExactDirectSparseFacetDescentBatchClosureSummary closure_summary{};
  std::vector<ExactDirectSparseFacetDescentBatchResolvedKey> resolved_keys;
  std::vector<ExactDirectSparseFacetDescentBatchArmJoin> arm_joins;
  ExactDirectSparseFacetDescentBatchExecutionCounters counters{};

  bool source_plan_verified_once_at_session_open{false};
  bool source_plan_rebuilt_for_this_batch{false};
  bool source_exact_batch_and_chunk_joined{false};
  bool source_lanes_share_one_exact_batch{false};
  bool batch_budget_preflight_completed{false};
  bool batch_budget_preflight_satisfied{false};
  bool selected_families_scanned_once{false};
  bool every_arm_seed_selected_once_in_source_order{false};
  bool facets_reconstructed_on_demand_only{false};
  bool distinct_full_keys_canonicalized{false};
  bool one_shared_closure_and_memo_built_or_empty_batch{false};
  bool common_frozen_locator_snapshot_certified{false};
  bool every_arm_joined_by_identity_and_full_key{false};
  bool shared_closure_complete_relative_positive{false};
  bool transient_closure_released_before_delta_publication{false};
  bool closure_graph_persisted{false};
  bool batch_delta_ready_before_commit{false};
  bool scientific_commit_performed{false};
  bool locator_state_mutated{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool partial_closure_prefix_published_as_delta{false};
  bool facet_coface_cell_gamma_or_delaunay_materialized{false};
  bool gpu_execution_qualified{false};
  bool sub_second_latency_claimed{false};
  bool ten_million_point_capacity_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseFacetDescentBatchExecutionDecision decision{
      ExactDirectSparseFacetDescentBatchExecutionDecision::not_executed};
  ExactDirectSparseFacetDescentBatchExecutionScope scope{
      ExactDirectSparseFacetDescentBatchExecutionScope::unspecified};

  [[nodiscard]] bool complete_architecture_execution() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionResult&,
      const ExactDirectSparseFacetDescentBatchExecutionResult&) = default;
};

struct ExactDirectSparseFacetDescentBatchExecutionVerification {
  bool observed_storage_within_budget{false};
  bool anchored_source_plan_reused_without_full_replay{false};
  bool locator_snapshot_matches_observed_build{false};
  bool exact_batch_execution_freshly_replayed{false};
  bool exact_result_equality_certified{false};
  bool transient_closure_released_before_comparison{false};
  bool no_partial_delta_or_external_mutation{false};
  bool no_forbidden_global_structure_materialized{false};
  bool no_scale_or_public_status_claimed{false};
  bool diagnostic_result_certified{false};
  bool session_advanced{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionVerification&,
      const ExactDirectSparseFacetDescentBatchExecutionVerification&) =
      default;
};

struct ExactDirectSparseFacetDescentBatchExecutionSessionAudit {
  std::size_t source_plan_verification_count{};
  std::size_t prepare_attempt_count{};
  std::size_t fresh_batch_replay_count{};
  std::size_t accepted_batch_count{};
  std::size_t rejected_batch_replay_count{};
  std::size_t transient_closure_build_count{};
  std::size_t maximum_transient_closure_node_count{};
  std::size_t retained_closure_node_count{};
  bool source_plan_owned_by_session{false};
  bool full_source_plan_replayed_per_batch{false};
  bool closure_graph_retained_between_batches{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionSessionAudit&,
      const ExactDirectSparseFacetDescentBatchExecutionSessionAudit&) =
      default;
};

// In-memory execution authority for one complete 14C plan.  Construction
// rebuilds and compares that plan exactly once, then owns the fresh copy; the
// observed plan need not outlive construction.  Every other external source
// must remain alive and immutable for the session.  The locator may change
// only between calls and must be externally frozen during prepare_next() and
// commit_prepared().  The session is not internally synchronized: callers
// serialize all of its methods.
class ExactDirectSparseFacetDescentAnchoredBatchExecutor {
 public:
  ExactDirectSparseFacetDescentAnchoredBatchExecutor(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_event_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
      const ExactDirectMorseIndustrialPlanConfig& industrial_config,
      const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
      const ExactDirectSparseFacetDescentBatchPlanResult& observed_plan,
      const ExactDirectSparsePositiveFacetLocator& locator,
      const ExactDirectSparseFacetDescentClosureConfig& closure_config = {},
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first);

  ExactDirectSparseFacetDescentAnchoredBatchExecutor(
      const ExactDirectSparseFacetDescentAnchoredBatchExecutor&) = delete;
  ExactDirectSparseFacetDescentAnchoredBatchExecutor& operator=(
      const ExactDirectSparseFacetDescentAnchoredBatchExecutor&) = delete;
  ExactDirectSparseFacetDescentAnchoredBatchExecutor(
      ExactDirectSparseFacetDescentAnchoredBatchExecutor&&) = delete;
  ExactDirectSparseFacetDescentAnchoredBatchExecutor& operator=(
      ExactDirectSparseFacetDescentAnchoredBatchExecutor&&) = delete;

  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] std::size_t next_source_batch_index() const noexcept {
    return next_source_batch_index_;
  }
  [[nodiscard]] const ExactDirectSparseFacetDescentBatchPlanResult&
  source_plan() const noexcept {
    return source_plan_;
  }
  [[nodiscard]]
  const ExactDirectSparseFacetDescentBatchExecutionSessionAudit&
  audit() const noexcept {
    return audit_;
  }

  // A failed diagnostic is retryable and never advances the cursor.
  [[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult
  prepare_next(
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectSparseFacetDescentBatchExecutionBudget&
          execution_budget,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget);

  // Replays only the current exact batch.  A complete equal delta advances
  // the execution cursor; it does not claim or perform the later quotient,
  // locator transaction or hierarchy commit.
  [[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionVerification
  commit_prepared(
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectSparseFacetDescentBatchExecutionBudget&
          execution_budget,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
      const ExactDirectSparseFacetDescentBatchExecutionResult& observed);

 private:
  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  const ExactDirectSupportTerminalFacade* source_facade_{};
  const ExactDirectMorseEventJournalResult* source_event_journal_{};
  ExactDirectSaddleArmSeedBudget trusted_arm_seed_budget_{};
  const ExactDirectSaddleArmSeedJournalResult* source_arm_seed_journal_{};
  ExactDirectMorseIndustrialPlanConfig industrial_config_{};
  ExactDirectSparseFacetDescentBatchPlanBudget plan_budget_{};
  ExactDirectSparseFacetDescentBatchPlanResult source_plan_{};
  const ExactDirectSparsePositiveFacetLocator* locator_{};
  ExactDirectSparseFacetDescentClosureConfig closure_config_{};
  spatial::LbvhTraversalOrder traversal_order_{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t next_source_batch_index_{};
  std::size_t next_source_chunk_index_{};
  std::size_t next_source_lane_index_{};
  std::size_t next_source_family_index_{};
  std::size_t next_source_arm_seed_index_{};
  ExactDirectSparseFacetDescentBatchExecutionSessionAudit audit_{};
};

}  // namespace morsehgp3d::hierarchy
