#pragma once

#include "morsehgp3d/hierarchy/direct_projectable_contribution_window.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_carrier_origin_capability.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_source_forest_ledger_transaction_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_mode =
        "exact_source_forest_ledger_transaction_v1";
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_deployment_status =
        "architecture_only_in_process_two_step_commit_poison_on_unexpected_"
        "source_rejection_historical_ledger_prepare_no_durable_restart_not_"
        "massive_qualified";
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_source_forest_ledger_transaction_proof_basis =
        "transaction_minted_source_window_direct_birth_and_exact_closure_"
        "equal_facet_authority_strict_below_carrier_subset_private_carrier_"
        "origin_frozen_batch_sparse_projection_preview_root_ledger_then_"
        "source_commit_v1";

// These caps cover only the new composition passes and arenas.  Every
// subordinate operation retains its own independent budget.  In particular,
// this transaction does not turn the historical root-ledger prepare path into
// the future proof-bound/no-index-rebuild path.
struct ExactDirectNormalizedH0SourceForestLedgerTransactionBudget {
  std::size_t maximum_window_facet_scan_count{};
  std::size_t maximum_direct_reference_scan_count{};
  std::size_t maximum_coface_facet_reference_scan_count{};
  std::size_t maximum_equal_facet_resolution_count{};
  std::size_t maximum_merged_resolution_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_group_token_scan_count{};
  std::size_t maximum_parent_root_reference_count{};
  std::size_t maximum_group_point_delta_reference_count{};
  std::size_t maximum_standalone_birth_count{};
  std::size_t maximum_standalone_birth_point_reference_count{};
  std::size_t maximum_preview_requested_handle_count{};
  std::size_t maximum_scratch_entry_count{};
  ExactDirectSparseCarrierOriginCapabilityBudget carrier_origin{};
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget relative_batch{};
  ExactDirectNormalizedH0SparseForestProjectionBudget forest_projection{};
  ExactDirectSparseStableFacetForestPreparedPreviewBudget forest_preview{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&,
      const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&) =
      default;
};

enum class
ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_source_session_rejected,
  no_source_window_rejected,
  complete_transaction_owned_scientific_window,
};

struct ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult;

// This is the only accepted source ticket.  It is minted by calling
// prepare_next() on the exact session passed to the preparation function and
// privately retains that session's address and cursor snapshot.  A caller
// cannot wrap an arbitrary, foreign prepared window in this type.
class ExactDirectNormalizedH0SourceForestLedgerPreparedWindow final {
 public:
  ExactDirectNormalizedH0SourceForestLedgerPreparedWindow() noexcept =
      default;
  ~ExactDirectNormalizedH0SourceForestLedgerPreparedWindow() = default;
  ExactDirectNormalizedH0SourceForestLedgerPreparedWindow(
      ExactDirectNormalizedH0SourceForestLedgerPreparedWindow&&) noexcept =
      default;
  ExactDirectNormalizedH0SourceForestLedgerPreparedWindow& operator=(
      ExactDirectNormalizedH0SourceForestLedgerPreparedWindow&&) noexcept =
      default;
  ExactDirectNormalizedH0SourceForestLedgerPreparedWindow(
      const ExactDirectNormalizedH0SourceForestLedgerPreparedWindow&) =
      delete;
  ExactDirectNormalizedH0SourceForestLedgerPreparedWindow& operator=(
      const ExactDirectNormalizedH0SourceForestLedgerPreparedWindow&) =
      delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
  owned_window() const noexcept;
  [[nodiscard]] std::size_t source_batch_cursor() const noexcept;
  [[nodiscard]] std::size_t source_epoch() const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_chain_digest()
      const noexcept;

 private:
  const ExactDirectNormalizedH0ScientificWindowCapabilitySession*
      source_session_identity_{};
  const void* provider_identity_{};
  std::size_t source_batch_cursor_{};
  std::size_t source_epoch_{};
  contract::CanonicalId source_chain_digest_{};
  contract::CanonicalId manifest_digest_{};
  contract::CanonicalId source_identity_digest_{};
  std::optional<
      ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow>
      scientific_window_;
  bool minted_{false};

  friend struct
      ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult;
  friend ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult
  prepare_exact_direct_normalized_h0_source_forest_ledger_window(
      ExactDirectNormalizedH0ScientificWindowCapabilitySession&) noexcept;
  friend class ExactDirectNormalizedH0SourceForestLedgerTransaction;
};

struct ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult {
  std::optional<ExactDirectNormalizedH0SourceForestLedgerPreparedWindow>
      window;
  std::size_t source_batch_cursor{};
  std::size_t source_epoch{};
  contract::CanonicalId source_chain_digest{};
  bool source_window_prepared_exactly_once{false};
  bool source_cursor_unchanged{false};
  bool caller_supplied_source_ticket_accepted{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision decision{
      ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
          not_prepared};

  [[nodiscard]] bool certified_prepared_window() const noexcept;
};

[[nodiscard]]
ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult
prepare_exact_direct_normalized_h0_source_forest_ledger_window(
    ExactDirectNormalizedH0ScientificWindowCapabilitySession& source_session)
    noexcept;

struct ExactDirectNormalizedH0SourceForestLedgerTransactionCounters {
  std::size_t window_facet_scan_count{};
  std::size_t direct_reference_scan_count{};
  std::size_t coface_facet_reference_scan_count{};
  std::size_t touched_facet_count{};
  std::size_t equal_facet_resolution_count{};
  std::size_t direct_birth_equal_facet_resolution_count{};
  std::size_t closure_exact_equal_facet_resolution_count{};
  std::size_t carrier_resolution_count{};
  std::size_t merged_resolution_count{};
  std::size_t group_count{};
  std::size_t group_token_scan_count{};
  std::size_t parent_root_reference_count{};
  std::size_t group_point_delta_reference_count{};
  std::size_t standalone_birth_count{};
  std::size_t standalone_birth_point_reference_count{};
  std::size_t preview_requested_handle_count{};
  std::size_t source_commit_attempt_count{};
  // Contribution-window ingress path only; zero on the historical overload.
  std::size_t contribution_atom_scan_count{};
  std::size_t contribution_touched_definition_scan_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceForestLedgerTransactionCounters&,
      const ExactDirectNormalizedH0SourceForestLedgerTransactionCounters&) =
      default;
};

enum class ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition
    : std::uint8_t {
  not_certified,
  complete,
  rejected,
  budget_exhausted,
  contradiction,
};

enum class ExactDirectNormalizedH0SourceForestLedgerTransactionDecision
    : std::uint8_t {
  not_certified,
  no_prepared_window_rejected,
  no_foreign_or_stale_source_window_rejected,
  no_forest_or_ledger_rejected,
  no_source_cursor_forest_ledger_alignment_rejected,
  no_carrier_closure_partition_rejected,
  contradiction_equal_facet_authority,
  no_carrier_origin_rejected,
  no_composition_budget_exhausted,
  no_relative_frozen_batch_rejected,
  no_sparse_forest_projection_rejected,
  no_sparse_forest_preview_rejected,
  no_root_ledger_transition_derivation_rejected,
  no_root_ledger_preparation_rejected,
  no_precommit_stale_or_sibling_rejected,
  no_root_ledger_commit_rejected,
  contradiction_source_commit_after_forest_ledger_commit,
  complete_forest_ledger_then_exactly_once_source_commit,
  // Contribution-window ingress path only; appended so the historical
  // numeric values never move.
  no_contribution_window_ingress_rejected,
};

class ExactDirectNormalizedH0SourceForestLedgerTransactionResult final {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_source_forest_ledger_transaction_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_source_forest_ledger_transaction_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_source_forest_ledger_transaction_mode;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_source_forest_ledger_transaction_deployment_status;
  static constexpr std::string_view public_status =
      direct_normalized_h0_source_forest_ledger_transaction_public_status;
  static constexpr std::string_view proof_basis =
      direct_normalized_h0_source_forest_ledger_transaction_proof_basis;

  ExactDirectNormalizedH0SourceForestLedgerTransactionResult() noexcept =
      default;
  ~ExactDirectNormalizedH0SourceForestLedgerTransactionResult() = default;
  ExactDirectNormalizedH0SourceForestLedgerTransactionResult(
      ExactDirectNormalizedH0SourceForestLedgerTransactionResult&&) noexcept =
      default;
  ExactDirectNormalizedH0SourceForestLedgerTransactionResult& operator=(
      ExactDirectNormalizedH0SourceForestLedgerTransactionResult&&) noexcept =
      default;
  ExactDirectNormalizedH0SourceForestLedgerTransactionResult(
      const ExactDirectNormalizedH0SourceForestLedgerTransactionResult&) =
      delete;
  ExactDirectNormalizedH0SourceForestLedgerTransactionResult& operator=(
      const ExactDirectNormalizedH0SourceForestLedgerTransactionResult&) =
      delete;

  [[nodiscard]] std::uint32_t schema_version() const noexcept;
  [[nodiscard]] const
  ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&
  requested_budget() const & noexcept;
  [[nodiscard]] const
  ExactDirectNormalizedH0SourceForestLedgerTransactionCounters&
  counters() const & noexcept;
  [[nodiscard]] std::size_t source_batch_index() const noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp&
  forest_pre_stamp() const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp&
  forest_post_stamp() const & noexcept;
  [[nodiscard]] const ExactDirectSparseRootLedgerStamp& ledger_pre_stamp()
      const & noexcept;
  [[nodiscard]] const ExactDirectSparseRootLedgerStamp& ledger_post_stamp()
      const & noexcept;
  [[nodiscard]] const ExactDirectSparseRootLedgerCommitResult&
  forest_ledger_commit() const & noexcept;
  [[nodiscard]] const ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult&
  source_commit() const & noexcept;
  [[nodiscard]] ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition
  disposition() const noexcept;
  [[nodiscard]] ExactDirectNormalizedH0SourceForestLedgerTransactionDecision
  decision() const noexcept;
  [[nodiscard]] bool
  equal_facet_authority_derived_from_scientific_window_and_exact_closure()
      const noexcept;
  [[nodiscard]] bool carrier_and_equal_partition_exhaustive() const noexcept;
  [[nodiscard]] bool all_fallible_work_completed_before_any_logical_commit()
      const noexcept;
  [[nodiscard]] bool source_commit_after_forest_ledger_commit() const noexcept;
  [[nodiscard]] bool contribution_window_ingress_bound() const noexcept;
  [[nodiscard]] bool partial_commit_requires_rebuild() const noexcept;
  [[nodiscard]] bool durable_atomicity_claimed() const noexcept;
  [[nodiscard]] bool global_forbidden_structure_materialized() const noexcept;
  [[nodiscard]] bool durable_restart_supported() const noexcept;
  [[nodiscard]] bool vertical_maps_complete() const noexcept;
  [[nodiscard]] bool public_status_claimed() const noexcept;
  [[nodiscard]] bool certified_complete_transaction() const noexcept;
  [[nodiscard]] bool certified_no_commit_failure() const noexcept;
  [[nodiscard]] bool certified_poisoned_partial_commit() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

 private:
  std::uint32_t schema_version_{
      direct_normalized_h0_source_forest_ledger_transaction_schema_version};
  ExactDirectNormalizedH0SourceForestLedgerTransactionBudget
      requested_budget_{};
  ExactDirectNormalizedH0SourceForestLedgerTransactionCounters counters_{};
  std::size_t source_batch_index_{};
  std::size_t source_cursor_pre_{};
  std::size_t source_cursor_post_{};
  contract::CanonicalId source_chain_pre_{};
  contract::CanonicalId source_chain_post_{};
  ExactDirectSparseStableFacetForestStamp forest_pre_stamp_{};
  ExactDirectSparseStableFacetForestStamp forest_post_stamp_{};
  ExactDirectSparseRootLedgerStamp ledger_pre_stamp_{};
  ExactDirectSparseRootLedgerStamp ledger_post_stamp_{};
  ExactDirectSparseRootLedgerCommitResult forest_ledger_commit_{};
  ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult
      source_commit_{};
  bool transaction_window_privately_minted_{false};
  bool
      equal_facet_authority_derived_from_scientific_window_and_exact_closure_{
          false};
  bool carrier_and_equal_partition_exhaustive_{false};
  bool carrier_origin_privately_derived_{false};
  bool relative_batch_and_projection_privately_bound_{false};
  bool root_ledger_transition_derived_from_frozen_batch_{false};
  bool all_fallible_work_completed_before_any_logical_commit_{false};
  bool forest_ledger_committed_{false};
  bool source_commit_after_forest_ledger_commit_{false};
  bool contribution_window_ingress_bound_{false};
  bool partial_commit_requires_rebuild_{false};
  bool durable_atomicity_claimed_{false};
  bool global_forbidden_structure_materialized_{false};
  bool durable_restart_supported_{false};
  bool vertical_maps_complete_{false};
  bool source_exactness_claimed_{false};
  bool public_status_claimed_{false};
  ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition
      disposition_{
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              not_certified};
  ExactDirectNormalizedH0SourceForestLedgerTransactionDecision decision_{
      ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
          not_certified};
  bool minted_{false};

  friend class ExactDirectNormalizedH0SourceForestLedgerTransaction;
};

// Sequential in-process transaction.  The caller must freeze the source
// session, forest and ledger writers for the whole call.  The only caller
// payload is the exact descent closure of every touched non-birth facet and
// its stable source bindings; roots, carrier kinds, tokens, coverages, equal
// facets, quotient groups, parents and deltas are all derived from private or
// authenticated inputs.  Equal-level closure seeds are retained only by the
// equal-facet authority; the carrier capability receives the canonical strict
// subset.
//
// The forest+ledger commit precedes the source commit.  This is not a durable
// atomic commit protocol: a process crash between the two writes is outside
// the certificate.  An unexpected source rejection after the first commit is
// returned as a certified poisoned partial state; all three in-memory objects
// must then be discarded and rebuilt before reuse.
class ExactDirectNormalizedH0SourceForestLedgerTransaction final {
 public:
  [[nodiscard]] static
      ExactDirectNormalizedH0SourceForestLedgerTransactionResult execute(
          ExactDirectNormalizedH0ScientificWindowCapabilitySession&
              source_session,
          ExactDirectNormalizedH0SourceForestLedgerPreparedWindow
              prepared_window,
          const ExactDirectSparseStableFacetDescentClosureResult&
              candidate_closure,
          std::span<const ExactDirectSparseCarrierOriginSeedBinding>
              candidate_seed_bindings,
          ExactDirectSparseStableFacetForest& forest,
          ExactDirectSparseRootLedger& ledger,
          const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&
              budget) noexcept;

  // Contribution-window ingress overload.  The certified exhaustive
  // projection of the very same authenticated scientific window becomes the
  // ingress of the touched-facet provenance: its identity digests must bind
  // the transaction's source chain, batch cursor, manifest and source
  // identity, its atom count must equal the batch's coface-deletion
  // reference count, and its strictly ordered deduplicated touched-facet
  // definitions must equal, stable token by stable token, the set the
  // transaction derives internally.  Any divergence rejects fail-closed
  // before any commit; the window stays a projection and never becomes a
  // scientific authority of the pipeline.
  [[nodiscard]] static
      ExactDirectNormalizedH0SourceForestLedgerTransactionResult execute(
          ExactDirectNormalizedH0ScientificWindowCapabilitySession&
              source_session,
          ExactDirectNormalizedH0SourceForestLedgerPreparedWindow
              prepared_window,
          const ExactDirectProjectableContributionWindowResult&
              contribution_ingress,
          const ExactDirectSparseStableFacetDescentClosureResult&
              candidate_closure,
          std::span<const ExactDirectSparseCarrierOriginSeedBinding>
              candidate_seed_bindings,
          ExactDirectSparseStableFacetForest& forest,
          ExactDirectSparseRootLedger& ledger,
          const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&
              budget) noexcept;

 private:
  [[nodiscard]] static
      ExactDirectNormalizedH0SourceForestLedgerTransactionResult
      execute_with_optional_ingress(
          ExactDirectNormalizedH0ScientificWindowCapabilitySession&
              source_session,
          ExactDirectNormalizedH0SourceForestLedgerPreparedWindow
              prepared_window,
          const ExactDirectProjectableContributionWindowResult*
              contribution_ingress,
          const ExactDirectSparseStableFacetDescentClosureResult&
              candidate_closure,
          std::span<const ExactDirectSparseCarrierOriginSeedBinding>
              candidate_seed_bindings,
          ExactDirectSparseStableFacetForest& forest,
          ExactDirectSparseRootLedger& ledger,
          const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&
              budget) noexcept;
};

}  // namespace morsehgp3d::hierarchy
