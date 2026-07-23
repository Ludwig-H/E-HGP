#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
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
inline constexpr std::uint32_t
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version =
        1U;
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_mode =
        "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_top_k_proposal_preparation_proof_basis =
        "batch_caps_and_canonical_keys_before_synchronous_transcript_"
        "revalidation_exact_source_facet_baseline_separate_operational_audit_"
        "transcript_release_before_unseeded_exact_commit_replay_v1";
inline constexpr std::uint32_t
    direct_sparse_facet_descent_batch_sealed_commit_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_mode = "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_sealed_commit_proof_basis =
        "private_move_only_exact_delta_provenance_shared_session_seal_epoch_"
        "full_source_and_successor_cursor_frozen_locator_stamp_single_use_"
        "advance_without_transcript_audit_or_second_geometry_replay_v1";

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

enum class
    ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_preparation_batch_diagnostic_before_proposal_consumption,
  no_preparation_batch_diagnostic_during_proposal_consumption,
  no_preparation_transcript_rejected_before_closure,
  no_preparation_batch_diagnostic_after_exact_proposal_consumption,
  complete_architecture_only_scientific_delta_with_separate_proposal_audit,
};

enum class ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope
    : std::uint8_t {
  unspecified,
  one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only,
};

// The transcript is consumed synchronously and never enters this envelope.
// A complete result owns the unchanged 14D scientific delta and a separate
// scalar operational audit.  commit_prepared() remains proposal-independent:
// after this call returns, callers may destroy the transcript and pass only
// scientific_delta to the historical exact replay.  Preparation success does
// not promise that this unseeded replay fits the same operational budget.
struct ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version};
  std::size_t source_batch_index{};
  exact::ExactLevel closed_batch_squared_level{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  ExactDirectSparseFacetDescentBatchExecutionDecision
      batch_execution_decision{
          ExactDirectSparseFacetDescentBatchExecutionDecision::not_executed};
  ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision
      proposal_consumption_decision{
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision::
              not_validated};
  std::optional<ExactDirectSparseFacetDescentBatchExecutionResult>
      scientific_delta;
  std::optional<ExactDirectSparseFacetDescentBatchExecutionResult>
      batch_diagnostic;
  std::optional<
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
      proposal_consumption_audit;

  bool transcript_consumption_requested{false};
  bool batch_preflight_completed_before_proposal_consumption{false};
  bool transcript_validation_completed_synchronously{false};
  bool exact_proposal_consumption_certified{false};
  bool atomic_transcript_rejection_certified{false};
  bool no_closure_constructed_on_transcript_rejection{false};
  bool scientific_delta_separated_from_proposal_audit{false};
  bool no_transcript_record_candidate_partition_or_shell_persisted{false};
  bool no_proposal_payload_or_audit_retained_by_session{false};
  bool preparation_left_session_cursor_unchanged{false};
  bool standard_commit_replays_unseeded_exact_path{false};
  bool operational_audit_has_no_scientific_authority{false};
  bool unseeded_commit_readiness_claimed{false};
  bool scientific_commit_performed{false};
  bool locator_state_mutated{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool gpu_execution_qualified{false};
  bool sub_second_latency_claimed{false};
  bool ten_million_point_capacity_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision decision{
      ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
          not_prepared};
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope scope{
      ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
          unspecified};

  [[nodiscard]] bool complete_architecture_preparation() const noexcept;
  [[nodiscard]] bool certified_atomic_transcript_rejection() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult&,
      const ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult&) =
      default;
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

enum class ExactDirectSparseFacetDescentBatchSealedCommitDecision
    : std::uint8_t {
  not_committed,
  no_commit_invalid_moved_or_consumed_ticket,
  no_commit_foreign_session,
  no_commit_stale_epoch_or_cursor,
  no_commit_locator_snapshot_changed,
  no_commit_audit_capacity_exhausted,
  complete_architecture_only_sealed_exact_delta_cursor_advance,
};

enum class ExactDirectSparseFacetDescentBatchSealedCommitScope
    : std::uint8_t {
  unspecified,
  one_in_process_exact_preparation_provenance_cursor_advance_before_hierarchy_commit_only,
};

struct ExactDirectSparseFacetDescentBatchSealedCommitVerification {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_batch_sealed_commit_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_batch_sealed_commit_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_batch_sealed_commit_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_descent_batch_sealed_commit_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_batch_sealed_commit_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_batch_sealed_commit_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_batch_sealed_commit_schema_version};
  std::size_t source_batch_index{};
  std::size_t successor_batch_index{};
  bool ticket_was_valid_and_unconsumed{false};
  bool shared_session_seal_matches{false};
  bool source_epoch_matches{false};
  bool full_source_cursor_matches{false};
  bool locator_snapshot_matches{false};
  bool exact_scientific_delta_provenance_minted_before_commit{false};
  bool prevalidated_successor_cursor_used{false};
  bool independent_geometry_replay_performed{false};
  bool closure_budget_consumed_during_commit{false};
  bool transcript_present_during_commit{false};
  bool operational_audit_read_for_commit_authority{false};
  bool locator_or_hierarchy_state_mutated{false};
  bool forbidden_global_structure_materialized{false};
  bool scale_or_public_status_claimed{false};
  bool scientific_delta_moved_to_commit_result{false};
  bool ticket_consumed{false};
  bool session_advanced{false};
  bool result_certified{false};
  ExactDirectSparseFacetDescentBatchSealedCommitDecision decision{
      ExactDirectSparseFacetDescentBatchSealedCommitDecision::not_committed};
  ExactDirectSparseFacetDescentBatchSealedCommitScope scope{
      ExactDirectSparseFacetDescentBatchSealedCommitScope::unspecified};

  [[nodiscard]] bool certified_cursor_advance() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchSealedCommitVerification&,
      const ExactDirectSparseFacetDescentBatchSealedCommitVerification&) =
      default;
};

struct ExactDirectSparseFacetDescentBatchSealedCommitResult {
  ExactDirectSparseFacetDescentBatchSealedCommitVerification verification{};
  std::optional<ExactDirectSparseFacetDescentBatchExecutionResult>
      scientific_delta;
  std::optional<
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
      operational_audit;

  [[nodiscard]] bool certified_cursor_advance() const noexcept {
    return verification.certified_cursor_advance() &&
           scientific_delta.has_value();
  }
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
  std::size_t proposal_prepare_attempt_count{};
  std::size_t proposal_consumption_attempt_count{};
  std::size_t proposal_transcript_rejection_count{};
  // Counts exact proposal-consuming closure entry calls, including an empty
  // validation call.  transient_closure_build_count above counts only shared
  // nonempty scientific closures represented by 14D batch deltas.
  std::size_t proposal_exact_closure_call_count{};
  std::size_t retained_proposal_record_count{};
  std::size_t sealed_ticket_prepare_attempt_count{};
  std::size_t sealed_ticket_issued_count{};
  std::size_t sealed_ticket_commit_attempt_count{};
  std::size_t sealed_ticket_accepted_commit_count{};
  std::size_t sealed_ticket_rejected_commit_count{};
  std::size_t sealed_ticket_exact_replay_avoided_count{};
  bool source_plan_owned_by_session{false};
  bool full_source_plan_replayed_per_batch{false};
  bool closure_graph_retained_between_batches{false};
  bool proposal_payload_or_audit_retained_between_calls{false};
  bool sealed_ticket_or_delta_retained_by_session{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchExecutionSessionAudit&,
      const ExactDirectSparseFacetDescentBatchExecutionSessionAudit&) =
      default;
};

// In-memory execution authority for one complete 14C plan.  Construction
// rebuilds and compares that plan exactly once, then owns the fresh copy; the
// observed plan need not outlive construction.  Every other external source
// must remain alive and immutable for the session.  The locator may change
// only between calls and must be externally frozen during every preparation
// and commit call, including the sealed-ticket APIs.  The session is not
// internally synchronized: callers serialize all of its methods.
class ExactDirectSparseFacetDescentAnchoredBatchExecutor {
 public:
  class PreparedTopKProposalBatch {
   public:
    PreparedTopKProposalBatch(const PreparedTopKProposalBatch&) = delete;
    PreparedTopKProposalBatch& operator=(
        const PreparedTopKProposalBatch&) = delete;
    PreparedTopKProposalBatch(PreparedTopKProposalBatch&& other) noexcept;
    PreparedTopKProposalBatch& operator=(
        PreparedTopKProposalBatch&& other) noexcept;
    ~PreparedTopKProposalBatch() = default;

    [[nodiscard]]
    const ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult&
    preparation() const noexcept {
      return preparation_;
    }
    [[nodiscard]]
    const ExactDirectSparseFacetDescentBatchExecutionResult*
    scientific_delta() const noexcept {
      return preparation_.scientific_delta
                     .has_value()
                 ? &*preparation_.scientific_delta
                 : nullptr;
    }
    [[nodiscard]] bool prepared() const noexcept {
      return valid_ && exact_scientific_delta_provenance_minted_;
    }
    [[nodiscard]] bool consumed() const noexcept {
      return consumed_;
    }
    // The audit is caller-owned operational evidence.  Removing, mutating or
    // destroying it cannot affect the already minted private commit
    // capability.
    [[nodiscard]] std::optional<
        ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
    take_operational_audit() noexcept;

   private:
    PreparedTopKProposalBatch(
        std::shared_ptr<const std::byte> session_seal,
        std::size_t source_epoch,
        std::size_t source_batch_index,
        std::size_t source_chunk_index,
        std::size_t source_lane_index,
        std::size_t source_family_index,
        std::size_t source_arm_seed_index,
        std::size_t successor_batch_index,
        std::size_t successor_chunk_index,
        std::size_t successor_lane_index,
        std::size_t successor_family_index,
        std::size_t successor_arm_seed_index,
        ExactDirectSparsePositiveFacetLocatorSnapshotStamp
            locator_snapshot_stamp,
        ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
            preparation) noexcept;
    explicit PreparedTopKProposalBatch(
        ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
            preparation) noexcept
        : preparation_(std::move(preparation)) {}
    PreparedTopKProposalBatch() noexcept = default;

    std::shared_ptr<const std::byte> session_seal_;
    std::size_t source_epoch_{};
    std::size_t source_batch_index_{};
    std::size_t source_chunk_index_{};
    std::size_t source_lane_index_{};
    std::size_t source_family_index_{};
    std::size_t source_arm_seed_index_{};
    std::size_t successor_batch_index_{};
    std::size_t successor_chunk_index_{};
    std::size_t successor_lane_index_{};
    std::size_t successor_family_index_{};
    std::size_t successor_arm_seed_index_{};
    ExactDirectSparsePositiveFacetLocatorSnapshotStamp
        locator_snapshot_stamp_{};
    ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
        preparation_{};
    bool exact_scientific_delta_provenance_minted_{false};
    bool valid_{false};
    bool consumed_{false};

    friend class ExactDirectSparseFacetDescentAnchoredBatchExecutor;
  };

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

  // The transcript and every external authority must remain immutable for
  // this synchronous call.  Success returns a scientific delta that may be
  // passed to the historical unseeded commit after the transcript has been
  // destroyed.  That independent replay can still exhaust a tighter budget:
  // this preparation does not certify unseeded commit readiness.  Rejection
  // returns no delta and never advances the cursor.
  [[nodiscard]]
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
  prepare_next_with_top_k_proposal_transcript(
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectSparseFacetDescentBatchExecutionBudget&
          execution_budget,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
      const ExactDirectSparseFacetTopKProposalTranscriptResult& transcript);

  // Mints a single-use capability directly from one complete exact 14G
  // preparation.  The ticket owns the compact scientific delta; neither the
  // executor nor the ticket stores transcript records.  Its separate audit
  // may be removed and destroyed before commit.
  [[nodiscard]] PreparedTopKProposalBatch
  prepare_next_sealed_with_top_k_proposal_transcript(
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectSparseFacetDescentBatchExecutionBudget&
          execution_budget,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
      const ExactDirectSparseFacetTopKProposalTranscriptResult& transcript);

  // Consumes only the private capability.  A current ticket advances to its
  // prevalidated successor cursor without top-k, 10.5b, 10.5c, transcript,
  // proposal-audit authority, closure budget or independent geometry replay.
  [[nodiscard]] ExactDirectSparseFacetDescentBatchSealedCommitResult
  commit_prepared_ticket(PreparedTopKProposalBatch&& prepared) noexcept;

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
  std::shared_ptr<const std::byte> session_seal_;
  std::size_t source_epoch_{};
  std::size_t next_source_batch_index_{};
  std::size_t next_source_chunk_index_{};
  std::size_t next_source_lane_index_{};
  std::size_t next_source_family_index_{};
  std::size_t next_source_arm_seed_index_{};
  ExactDirectSparseFacetDescentBatchExecutionSessionAudit audit_{};
};

}  // namespace morsehgp3d::hierarchy
