#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_segment_sink.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_forest_reducer_schema_version =
    2U;
inline constexpr std::string_view direct_morse_forest_reducer_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_reducer_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_reducer_mode =
    "budgeted";
inline constexpr std::string_view
    direct_morse_forest_reducer_deployment_status = "architecture_only";
inline constexpr std::string_view direct_morse_forest_reducer_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_forest_reducer_proof_basis =
    "strict_batch_stream_frozen_r_or_l_carrier_hypergraph_complete_"
    "transitive_quotient_qr_then_atomic_locator_and_scientific_commit_"
    "shared_locator_canonical_parent_authority_implicit_singleton_carrier_"
    "base_and_direct_only_state_suffix_v2";
inline constexpr std::uint32_t
    direct_morse_forest_live_commit_schema_version = 1U;
inline constexpr std::string_view direct_morse_forest_live_commit_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_live_commit_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_live_commit_mode =
    "budgeted";
inline constexpr std::string_view
    direct_morse_forest_live_commit_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_forest_live_commit_public_status = "not_claimed";
inline constexpr std::string_view direct_morse_forest_live_commit_proof_basis =
    "sealed_14h_ticket_and_reducer_share_exact_pre_batch_locator_all_"
    "fallible_reduction_before_noexcept_preflighted_cursor_advance_v1";

// This is the complete scientific projection consumed by one reducer fold.
// source_chunk_index is operational provenance only: it is deliberately
// excluded from every hierarchy identity and result record.
struct ExactDirectMorseForestReducerBatch {
  std::size_t source_batch_index{};
  std::optional<std::size_t> source_chunk_index;
  std::size_t source_family_begin_index{};
  std::size_t source_family_end_index{};
  std::size_t source_arm_seed_begin_index{};
  std::size_t source_arm_seed_end_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      strict_pre_batch_locator_stamp{};
  ExactDirectSparseFacetDescentClosureBudget requested_closure_budget{};
  std::size_t transient_closure_node_count{};
  std::size_t transient_closure_step_call_count{};
  std::size_t shared_closure_build_count{};
  // Synchronous non-owning views.  Their producer must remain alive and
  // immutable until fold() returns; the reducer never retains either span.
  std::span<const ExactDirectSparseFacetDescentBatchResolvedKey> resolved_keys;
  std::span<const ExactDirectSparseFacetDescentBatchArmJoin> arm_joins;
};

// Projects non-owning views of the compact 14D scientific delta.  Transient
// closure nodes, proposal records and execution-session state never enter the
// reducer.  source must remain alive and immutable until fold() returns.
// Throws std::invalid_argument unless source is a complete, non-mutating 14D
// architecture execution.
[[nodiscard]] ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult& source);
ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    ExactDirectSparseFacetDescentBatchExecutionResult&& source) = delete;
ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult&& source) =
    delete;

enum class ExactDirectMorseForestReducerFoldDecision : std::uint8_t {
  not_folded,
  no_reducer_finished,
  no_reducer_batch_out_of_order,
  no_reducer_batch_inconsistent,
  no_reducer_budget_exhausted,
  no_reducer_operational_allocation_failed,
  no_reducer_frozen_carrier_quotient_rejected,
  no_reducer_locator_commit_rejected,
  complete_reducer_batch_commit,
  no_reducer_output_segment_pending,
};

struct ExactDirectMorseForestReducerFoldResult {
  std::uint32_t schema_version{direct_morse_forest_reducer_schema_version};
  std::size_t source_batch_index{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp pre_fold_locator_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp post_fold_locator_stamp{};
  // Phase-15F allocation audit.  The freshly certified canonical singleton
  // batch reports its dense bulk length and bypasses all three per-birth
  // staging arenas.  Ordinary batches keep bulk_count at zero and report the
  // actual temporary arena sizes used by the general reducer path.
  std::size_t canonical_singleton_bulk_count{};
  std::size_t staged_birth_record_count{};
  std::size_t staged_birth_node_count{};
  std::size_t staged_locator_binding_count{};
  std::size_t implicit_singleton_carrier_count{};
  std::size_t materialized_direct_carrier_state_count{};
  std::size_t total_carrier_handle_count{};
  std::size_t maximum_atomic_group_count{};
  std::size_t root_override_slot_capacity{};
  bool locator_parent_authority_reused_by_carrier_state{false};
  bool no_dense_singleton_carrier_state_materialized{false};
  bool complete_batch_staged_before_mutation{false};
  bool full_equal_level_quotient_resolved_before_mutation{false};
  bool locator_committed_before_scientific_state{false};
  bool scientific_state_committed{false};
  bool reducer_state_mutated{false};
  bool source_chunk_index_has_scientific_authority{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestReducerFoldDecision decision{
      ExactDirectMorseForestReducerFoldDecision::not_folded};

  [[nodiscard]] bool certified_committed_batch() const noexcept;
  [[nodiscard]] bool certified_atomic_rejection() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestReducerFoldResult&,
      const ExactDirectMorseForestReducerFoldResult&) = default;
};

// Freshly binds the fold's representation audit to caller-trusted source
// authority and budget values.  certified_* checks the result's internal
// consistency; persistence or cross-boundary consumers must additionally use
// this verifier instead of trusting the result's echoed counts.
[[nodiscard]] bool verify_exact_direct_morse_forest_reducer_fold_layout(
    const ExactDirectMorseForestReducerFoldResult& observed,
    std::size_t trusted_total_carrier_handle_count,
    std::size_t trusted_implicit_singleton_carrier_count,
    std::size_t trusted_maximum_atomic_group_count) noexcept;

enum class ExactDirectMorseForestLiveCommitDecision : std::uint8_t {
  not_committed,
  no_live_commit_invalid_moved_or_consumed_ticket,
  no_live_commit_foreign_session,
  no_live_commit_stale_epoch_or_cursor,
  no_live_commit_distinct_locator_or_snapshot,
  no_live_commit_reducer_cursor_mismatch,
  no_live_commit_executor_audit_capacity_exhausted,
  no_live_commit_reducer_fold_rejected,
  complete_live_reducer_then_cursor_commit,
};

// One live Phase-15D transaction.  The reducer performs every allocation,
// lookup, quotient and budget decision while the 14H cursor is still frozen.
// Once the reducer has atomically committed its locator and scientific state,
// only preflighted scalar assignments, noexcept moves and counter increments
// remain before the executor reaches the same successor batch.
struct ExactDirectMorseForestLiveCommitResult {
  static constexpr std::string_view backend =
      direct_morse_forest_live_commit_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_live_commit_profile;
  static constexpr std::string_view mode =
      direct_morse_forest_live_commit_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_forest_live_commit_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_forest_live_commit_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_live_commit_proof_basis;

  std::uint32_t schema_version{
      direct_morse_forest_live_commit_schema_version};
  std::size_t source_batch_index{};
  std::size_t successor_batch_index{};
  std::size_t pre_commit_executor_batch_index{};
  std::size_t post_commit_executor_batch_index{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      pre_commit_locator_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      post_commit_locator_stamp{};
  bool ticket_was_valid_and_unconsumed{false};
  bool shared_session_seal_matches{false};
  bool source_epoch_and_full_cursor_match{false};
  bool exact_scientific_delta_provenance_minted{false};
  bool reducer_and_executor_share_locator_instance{false};
  bool reducer_and_executor_batch_cursors_match{false};
  bool executor_commit_capacity_preflighted{false};
  bool all_fallible_scientific_work_precedes_irreversible_mutation{false};
  bool reducer_fold_attempted{false};
  bool reducer_committed_before_executor_cursor{false};
  bool no_fallible_operation_after_reducer_commit{false};
  bool executor_cursor_advanced{false};
  bool scientific_delta_moved_to_result{false};
  bool ticket_consumed{false};
  bool executor_cursor_unchanged_on_rejection{false};
  bool locator_unchanged_on_rejection{false};
  bool independent_geometry_replay_performed{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestReducerFoldResult reducer_fold{};
  std::optional<ExactDirectSparseFacetDescentBatchExecutionResult>
      scientific_delta;
  std::optional<
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
      operational_audit;
  ExactDirectMorseForestLiveCommitDecision decision{
      ExactDirectMorseForestLiveCommitDecision::not_committed};

  [[nodiscard]] bool certified_live_commit() const noexcept;
  [[nodiscard]] bool certified_atomic_rejection() const noexcept;
};

// Incremental Phase-15C reduction.  The source authorities must outlive the
// reducer.  Its persistent scientific state is one sparse positive locator,
// carrier attributes that reuse the locator's canonical-minimum parent
// authority and per-order scalar counts.  Resident mode also retains the
// final forest arenas; segmented mode retains at most one committed batch
// until its sink acknowledgement and then reuses that batch's capacities.
// Canonical singleton carrier attributes are implicit; only the direct suffix
// and an output-proportional reduced-root override table are materialized.  It
// retains no input batch deltas, closure graph, cells, cofaces, Gamma
// structure or Delaunay mosaic.
class ExactDirectMorseForestReducer {
 public:
  ExactDirectMorseForestReducer(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first);

  // Selects the bounded segmented-output path before the first fold.  The
  // scientific locator and carrier authorities are identical to the resident
  // path, while only one committed batch segment may remain pending.
  ExactDirectMorseForestReducer(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      const ExactDirectMorseForestSegmentLimits& segment_limits,
      const contract::CanonicalId& initial_chain_digest,
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first);
  ~ExactDirectMorseForestReducer();

  ExactDirectMorseForestReducer(
      const ExactDirectMorseForestReducer&) = delete;
  ExactDirectMorseForestReducer& operator=(
      const ExactDirectMorseForestReducer&) = delete;
  ExactDirectMorseForestReducer(
      ExactDirectMorseForestReducer&&) noexcept;
  ExactDirectMorseForestReducer& operator=(
      ExactDirectMorseForestReducer&&) noexcept;

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold(
      const ExactDirectMorseForestReducerBatch& batch);

  // Consumes a sealed 14H ticket and folds its compact delta without replay.
  // The executor must have been constructed from strict_locator(), and callers
  // must serialize both objects for the entire call.  Every possible reducer
  // rejection occurs before either scientific state or the executor cursor
  // changes.  A successful reducer fold is followed only by a preflighted,
  // allocation-free cursor advance.
  [[nodiscard]] ExactDirectMorseForestLiveCommitResult fold_prepared_ticket(
      ExactDirectSparseFacetDescentAnchoredBatchExecutor& executor,
      ExactDirectSparseFacetDescentAnchoredBatchExecutor::
          PreparedTopKProposalBatch&& prepared);

  // The returned locator is a short-lived strict-state view for the 14D
  // executor.  No fold may overlap such a read, and the reducer must not move
  // while an executor retains this address.
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& strict_locator()
      const noexcept;

  [[nodiscard]] std::size_t next_source_batch_index() const noexcept;
  [[nodiscard]] bool complete() const noexcept;

  [[nodiscard]] bool segmented_output_enabled() const noexcept;
  [[nodiscard]] bool has_pending_output_segment() const noexcept;
  [[nodiscard]] const ExactDirectMorseForestBatchSegment*
  pending_output_segment() const noexcept;
  [[nodiscard]] const ExactDirectMorseForestSegmentCursor& output_cursor()
      const noexcept;
  [[nodiscard]] ExactDirectMorseForestSegmentDrainResult
  drain_pending_output_segment(
      ExactDirectMorseForestSegmentSinkView sink) noexcept;

  // Consumes the reducer after every source batch has committed.  Calling
  // finish early throws std::logic_error.
  [[nodiscard]] ExactDirectMorseForestJournalResult finish();

  // Segmented counterpart of finish().  Every committed batch must first be
  // acknowledged by the sink.  The returned terminal object is O(K).
  [[nodiscard]] ExactDirectMorseForestFinalSeal finish_segmented();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
