#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactDirectSparseRootLedgerInitialization;

inline constexpr std::uint32_t direct_sparse_root_ledger_schema_version = 1U;
inline constexpr std::uint32_t
    direct_sparse_root_ledger_storage_schema_version = 1U;
inline constexpr std::string_view direct_sparse_root_ledger_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_root_ledger_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_root_ledger_mode =
    "prepared_forest_preview_bound_sparse_root_coverage_dag_structure_only_v1";
inline constexpr std::string_view direct_sparse_root_ledger_deployment_status =
    "architecture_only_not_massive_qualified_no_vertical_no_durable_restart";
inline constexpr std::string_view direct_sparse_root_ledger_public_status =
    "not_claimed";

using ExactDirectSparseRootId = ExactFrozenIncidencePriorRootId;
using ExactDirectSparseRootCoverageId = std::size_t;

// The fingerprint masks tune physical collision tests only.  They never enter
// a logical stamp or digest.  first_allocated_root_id defaults to the required
// monotone namespace origin; a larger nonzero value is accepted so the exact
// UINT64_MAX exhaustion boundary can be exercised without a dense prefix.
struct ExactDirectSparseRootLedgerConfig {
  ExactDirectSparseRootId first_allocated_root_id{1U};
  std::uint64_t forest_root_fingerprint_mask{~std::uint64_t{0U}};
  std::uint64_t root_id_fingerprint_mask{~std::uint64_t{0U}};

  friend bool operator==(
      const ExactDirectSparseRootLedgerConfig&,
      const ExactDirectSparseRootLedgerConfig&) = default;
};

// Durable limits are cumulative.  Batch limits cap every externally supplied
// CSR and every canonical staging arena before allocation.  The active tables
// remain sparse in observed forest roots; no forest-handle namespace is ever
// materialized.
struct ExactDirectSparseRootLedgerBudget {
  std::size_t maximum_active_root_count{};
  std::size_t maximum_history_entry_count{};
  std::size_t maximum_coverage_node_count{};
  std::size_t maximum_coverage_parent_reference_count{};
  std::size_t maximum_coverage_point_delta_count{};
  std::size_t maximum_prior_root_parent_reference_count{};
  std::size_t maximum_committed_batch_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_standalone_birth_count{};
  std::size_t maximum_parent_root_reference_count{};
  std::size_t maximum_point_delta_count{};
  std::size_t maximum_preview_record_count{};
  std::size_t maximum_staging_entry_count{};
  std::size_t maximum_outstanding_ticket_count{};

  friend bool operator==(
      const ExactDirectSparseRootLedgerBudget&,
      const ExactDirectSparseRootLedgerBudget&) = default;
};

// A non-deferred group names one representative handle plus a canonical slice
// of active pre-batch forest roots.  The sealed forest preview, not this public
// record, is authoritative for their common post-batch root.  At least one
// carrier (rooted or latent) is required; equal-only provenance is rejected.
struct ExactDirectSparseRootLedgerGroupTransition {
  std::size_t source_group_index{};
  ExactDirectSparseStableFacetHandle representative_handle{};
  std::size_t parent_root_offset{};
  std::size_t parent_root_count{};
  std::size_t point_delta_offset{};
  std::size_t point_delta_count{};
  std::size_t facet_delta_count{};

  friend bool operator==(
      const ExactDirectSparseRootLedgerGroupTransition&,
      const ExactDirectSparseRootLedgerGroupTransition&) = default;
};

// A deferred direct birth has no carrier parent.  It creates one active latent
// coverage binding and intentionally allocates no PriorRootId.
struct ExactDirectSparseRootLedgerStandaloneBirth {
  ExactDirectSparseStableFacetHandle representative_handle{};
  std::size_t point_delta_offset{};
  std::size_t point_delta_count{};
  std::size_t facet_delta_count{1U};

  friend bool operator==(
      const ExactDirectSparseRootLedgerStandaloneBirth&,
      const ExactDirectSparseRootLedgerStandaloneBirth&) = default;
};

enum class ExactDirectSparseRootLedgerTransitionKind : std::uint8_t {
  quotient_group,
  standalone_direct_birth,
};

// History is append-only.  Liveness is represented solely by the sparse
// active indexes, so an old q_R>1 root remains auditable without being mutated.
struct ExactDirectSparseRootLedgerEntry {
  std::size_t history_index{};
  std::size_t committed_batch_index{};
  ExactDirectSparseStableFacetHandle forest_root_handle{};
  std::optional<std::size_t> source_group_index;
  std::optional<ExactDirectSparseRootId> prior_root_id;
  ExactDirectSparseRootCoverageId coverage_id{};
  std::size_t prior_root_parent_offset{};
  std::size_t prior_root_parent_count{};
  std::size_t q_r{};
  std::size_t parent_root_count{};
  std::size_t facet_delta_count{};
  std::size_t point_delta_count{};
  bool coverage_reused{false};
  ExactDirectSparseRootLedgerTransitionKind kind{
      ExactDirectSparseRootLedgerTransitionKind::quotient_group};

  friend bool operator==(
      const ExactDirectSparseRootLedgerEntry&,
      const ExactDirectSparseRootLedgerEntry&) = default;
};

// Coverage nodes form an append-only DAG.  Parent IDs always precede this ID;
// parent and delta slices address segmented flat arenas.  Point deltas are
// strictly increasing but may overlap ancestors; deduplication is deferred to
// explicitly requested bounded materialization.
struct ExactDirectSparseRootCoverageNode {
  ExactDirectSparseRootCoverageId coverage_id{};
  std::size_t parent_offset{};
  std::size_t parent_count{};
  std::size_t point_delta_offset{};
  std::size_t point_delta_count{};

  friend bool operator==(
      const ExactDirectSparseRootCoverageNode&,
      const ExactDirectSparseRootCoverageNode&) = default;
};

struct ExactDirectSparseRootLedgerStamp {
  std::uint32_t schema_version{direct_sparse_root_ledger_schema_version};
  std::uint64_t session_instance_id{};
  std::size_t epoch{};
  std::size_t committed_batch_count{};
  std::size_t active_root_count{};
  std::size_t rooted_active_count{};
  std::size_t history_entry_count{};
  std::size_t coverage_node_count{};
  std::size_t coverage_parent_reference_count{};
  std::size_t coverage_point_delta_count{};
  std::size_t prior_root_parent_reference_count{};
  std::optional<ExactDirectSparseRootId> next_allocatable_root_id;
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId semantic_history_digest{};
  ExactDirectSparseStableFacetForestStamp aligned_forest_stamp{};

  friend bool operator==(
      const ExactDirectSparseRootLedgerStamp&,
      const ExactDirectSparseRootLedgerStamp&) = default;
};

enum class ExactDirectSparseRootLedgerInitializationDecision : std::uint8_t {
  not_initialized,
  no_configuration_rejected,
  no_budget_rejected,
  no_forest_rejected,
  no_allocation_failed,
  no_session_instance_id_exhausted,
  complete_empty_sparse_ledger,
};

enum class ExactDirectSparseRootLedgerPreparationDecision : std::uint8_t {
  not_prepared,
  no_ledger_rejected,
  no_forest_ticket_rejected,
  no_forest_preview_rejected,
  no_outstanding_ticket_budget_exhausted,
  no_capacity_overflow,
  no_budget_exhausted,
  no_input_shape_rejected,
  no_preview_exhaustiveness_rejected,
  contradiction_unknown_or_duplicate_parent_root,
  contradiction_resulting_root_collision,
  contradiction_root_id_namespace_exhausted,
  no_allocation_failed,
  complete_sealed_structure_only_transition,
};

enum class ExactDirectSparseRootLedgerCommitDecision : std::uint8_t {
  not_committed,
  no_ledger_rejected,
  no_ticket_rejected,
  no_foreign_ticket_rejected,
  no_stale_or_sibling_ticket_rejected,
  no_forest_commit_rejected,
  complete_atomic_forest_and_ledger_commit,
};

struct ExactDirectSparseRootLedgerPreparationReceipt {
  ExactDirectSparseRootLedgerStamp pre_stamp{};
  contract::CanonicalId forest_batch_digest{};
  contract::CanonicalId preview_receipt_digest{};
  contract::CanonicalId canonical_transition_digest{};
  std::size_t group_count{};
  std::size_t standalone_birth_count{};
  std::size_t parent_root_reference_count{};
  std::size_t point_delta_count{};
  std::size_t preview_record_count{};
  std::size_t q_r_zero_count{};
  std::size_t q_r_one_count{};
  std::size_t q_r_multiple_count{};
  std::size_t allocated_root_id_count{};
  std::size_t reused_coverage_count{};
  std::size_t staged_coverage_node_count{};
  std::size_t staged_coverage_parent_reference_count{};
  std::size_t staged_coverage_point_delta_count{};
  std::size_t staged_prior_root_parent_reference_count{};
  std::size_t post_active_root_count{};
  bool forest_ticket_identity_bound_by_private_preview{false};
  bool all_previewed_parents_share_declared_post_root{false};
  bool all_fallible_work_completed_before_commit{false};
  bool forest_or_ledger_logical_state_mutated{false};
  bool physical_capacity_may_have_changed{false};
  bool external_transition_structure_only{false};
  bool frozen_scientific_authority_replayed{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool durable_restart_supported{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootLedgerPreparationDecision decision{
      ExactDirectSparseRootLedgerPreparationDecision::not_prepared};

  friend bool operator==(
      const ExactDirectSparseRootLedgerPreparationReceipt&,
      const ExactDirectSparseRootLedgerPreparationReceipt&) = default;
};

class ExactDirectSparseRootLedgerPreparedBatch {
 public:
  ExactDirectSparseRootLedgerPreparedBatch() noexcept;
  ~ExactDirectSparseRootLedgerPreparedBatch();
  ExactDirectSparseRootLedgerPreparedBatch(
      ExactDirectSparseRootLedgerPreparedBatch&&) noexcept;
  ExactDirectSparseRootLedgerPreparedBatch& operator=(
      ExactDirectSparseRootLedgerPreparedBatch&&) noexcept;
  ExactDirectSparseRootLedgerPreparedBatch(
      const ExactDirectSparseRootLedgerPreparedBatch&) = delete;
  ExactDirectSparseRootLedgerPreparedBatch& operator=(
      const ExactDirectSparseRootLedgerPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactDirectSparseRootLedgerStamp& pre_stamp()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& canonical_transition_digest()
      const noexcept;
  [[nodiscard]] bool certifies_receipt(
      const ExactDirectSparseRootLedgerPreparationReceipt&) const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectSparseRootLedgerPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectSparseRootLedger;
};

struct ExactDirectSparseRootLedgerPreparationResult
    : public ExactDirectSparseRootLedgerPreparationReceipt {
  std::optional<ExactDirectSparseRootLedgerPreparedBatch> ticket;

  [[nodiscard]] bool certified_prepared() const noexcept;
};

struct ExactDirectSparseRootLedgerCommitResult {
  ExactDirectSparseRootLedgerStamp pre_stamp{};
  ExactDirectSparseRootLedgerStamp post_stamp{};
  ExactDirectSparseStableFacetForestCommitResult forest_commit{};
  std::size_t appended_history_entry_count{};
  std::size_t appended_coverage_node_count{};
  std::size_t allocated_root_id_count{};
  std::size_t preserved_root_id_count{};
  std::size_t latent_birth_count{};
  bool ticket_consumed{false};
  bool state_mutated{false};
  bool no_allocation_after_preparation{false};
  bool active_handle_and_root_id_bijections_preserved{false};
  bool aligned_to_committed_forest_post_stamp{false};
  bool external_transition_structure_only{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool durable_restart_supported{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootLedgerCommitDecision decision{
      ExactDirectSparseRootLedgerCommitDecision::not_committed};

  [[nodiscard]] bool certified_commit() const noexcept;
};

enum class ExactDirectSparseRootLedgerLookupDisposition : std::uint8_t {
  not_certified,
  unobserved,
  observed_latent,
  observed_rooted,
};

enum class ExactDirectSparseRootLedgerLookupKeyKind : std::uint8_t {
  unspecified,
  forest_root_handle,
  prior_root_id,
};

struct ExactDirectSparseRootLedgerLookupReceipt {
  ExactDirectSparseRootLedgerLookupKeyKind requested_key_kind{
      ExactDirectSparseRootLedgerLookupKeyKind::unspecified};
  ExactDirectSparseStableFacetHandle requested_forest_root_handle{};
  std::optional<ExactDirectSparseRootId> requested_prior_root_id;
  ExactDirectSparseRootLedgerEntry entry{};
  ExactDirectSparseRootLedgerStamp ledger_stamp{};
  bool ledger_certified_at_entry{false};
  bool sparse_active_index_authoritative{false};
  bool lookup_completed_without_mutation{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootLedgerLookupDisposition disposition{
      ExactDirectSparseRootLedgerLookupDisposition::not_certified};

  friend bool operator==(
      const ExactDirectSparseRootLedgerLookupReceipt&,
      const ExactDirectSparseRootLedgerLookupReceipt&) = default;
};

class ExactDirectSparseRootLedgerLookupResult final
    : public ExactDirectSparseRootLedgerLookupReceipt {
 public:
  ExactDirectSparseRootLedgerLookupResult() noexcept = default;
  [[nodiscard]] bool certified_observed() const noexcept;
  [[nodiscard]] bool certified_unobserved() const noexcept;
  [[nodiscard]] bool certified_for(
      const ExactDirectSparseRootLedgerStamp&) const noexcept;

 private:
  explicit ExactDirectSparseRootLedgerLookupResult(
      const ExactDirectSparseRootLedgerLookupReceipt&) noexcept;
  ExactDirectSparseRootLedgerLookupReceipt private_receipt_snapshot_{};
  bool minted_{false};
  friend class ExactDirectSparseRootLedger;
};

struct ExactDirectSparseRootCoverageMaterializationBudget {
  std::size_t maximum_requested_root_count{};
  std::size_t maximum_reachable_coverage_node_count{};
  std::size_t maximum_parent_reference_scan_count{};
  std::size_t maximum_point_delta_scan_count{};
  std::size_t maximum_output_point_count{};
  std::size_t maximum_scratch_entry_count{};

  friend bool operator==(
      const ExactDirectSparseRootCoverageMaterializationBudget&,
      const ExactDirectSparseRootCoverageMaterializationBudget&) = default;
};

struct ExactDirectSparseRootCoverageMaterializationRecord {
  ExactDirectSparseStableFacetHandle forest_root_handle{};
  std::optional<ExactDirectSparseRootId> prior_root_id;
  ExactDirectSparseRootCoverageId coverage_id{};
  std::size_t point_offset{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectSparseRootCoverageMaterializationRecord&,
      const ExactDirectSparseRootCoverageMaterializationRecord&) = default;
};

enum class ExactDirectSparseRootCoverageMaterializationDecision
    : std::uint8_t {
  not_materialized,
  no_ledger_rejected,
  no_capacity_overflow,
  no_budget_exhausted,
  no_requested_root_shape_rejected,
  no_requested_root_unobserved_rejected,
  no_allocation_failed,
  contradiction_coverage_dag,
  complete_requested_roots_only,
};

struct ExactDirectSparseRootCoverageMaterializationResult {
  ExactDirectSparseRootLedgerStamp ledger_stamp{};
  std::vector<ExactDirectSparseRootCoverageMaterializationRecord> records;
  std::vector<spatial::PointId> point_ids;
  std::size_t requested_root_count{};
  std::size_t reachable_coverage_node_count{};
  std::size_t parent_reference_scan_count{};
  std::size_t point_delta_scan_count{};
  std::size_t scratch_entry_count{};
  bool only_requested_coverage_dags_traversed{false};
  bool canonical_sorted_unique_points{false};
  bool ledger_logical_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootCoverageMaterializationDecision decision{
      ExactDirectSparseRootCoverageMaterializationDecision::not_materialized};

  [[nodiscard]] bool certified_requested_only_materialization() const noexcept;
};

enum class ExactDirectSparseRootLedgerCheckpointDecision : std::uint8_t {
  not_available,
  complete_honest_nonrestartable_semantic_checkpoint,
};

struct ExactDirectSparseRootLedgerCheckpoint {
  ExactDirectSparseRootLedgerStamp stamp{};
  contract::CanonicalId checkpoint_digest{};
  bool process_local_session_identity_only{false};
  bool semantic_metadata_complete{false};
  bool active_indexes_serialized{false};
  bool history_entries_serialized{false};
  bool coverage_dag_serialized{false};
  bool restore_supported{false};
  bool restartable{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootLedgerCheckpointDecision decision{
      ExactDirectSparseRootLedgerCheckpointDecision::not_available};

  [[nodiscard]] bool certified_honest_nonrestartable() const noexcept;
};

class ExactDirectSparseRootLedger {
 public:
  static constexpr std::string_view backend = direct_sparse_root_ledger_backend;
  static constexpr std::string_view profile = direct_sparse_root_ledger_profile;
  static constexpr std::string_view mode = direct_sparse_root_ledger_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_root_ledger_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_root_ledger_public_status;

  ExactDirectSparseRootLedger() noexcept;
  ~ExactDirectSparseRootLedger();
  ExactDirectSparseRootLedger(ExactDirectSparseRootLedger&&) noexcept;
  ExactDirectSparseRootLedger& operator=(
      ExactDirectSparseRootLedger&&) noexcept;
  ExactDirectSparseRootLedger(const ExactDirectSparseRootLedger&) = delete;
  ExactDirectSparseRootLedger& operator=(
      const ExactDirectSparseRootLedger&) = delete;

  [[nodiscard]] bool certified_structure_only_ledger() const noexcept;
  [[nodiscard]] ExactDirectSparseRootLedgerStamp current_stamp() const noexcept;
  [[nodiscard]] std::size_t outstanding_ticket_count() const noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseRootLedgerEntry>
  history_entries() const noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseRootCoverageNode>
  coverage_nodes() const noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseRootCoverageId>
  coverage_parent_references() const noexcept;
  [[nodiscard]] std::span<const spatial::PointId> coverage_point_deltas()
      const noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseRootId>
  prior_root_parent_references() const noexcept;
  [[nodiscard]] std::size_t materialized_active_handle_index_slot_count()
      const noexcept;
  [[nodiscard]] std::size_t materialized_active_root_id_index_slot_count()
      const noexcept;
  [[nodiscard]] ExactDirectSparseRootLedgerLookupResult lookup_active_root(
      ExactDirectSparseStableFacetHandle) const noexcept;
  [[nodiscard]] ExactDirectSparseRootLedgerLookupResult lookup_active_root_id(
      ExactDirectSparseRootId) const noexcept;

  [[nodiscard]] ExactDirectSparseRootLedgerPreparationResult prepare_transition(
      const ExactDirectSparseStableFacetForest& forest,
      ExactDirectSparseStableFacetForestPreparedBatch&& forest_ticket,
      ExactDirectSparseStableFacetForestPreparedPreview&& forest_preview,
      std::span<const ExactDirectSparseRootLedgerGroupTransition> groups,
      std::span<const ExactDirectSparseStableFacetHandle> parent_root_handles,
      std::span<const spatial::PointId> group_point_deltas,
      std::span<const ExactDirectSparseRootLedgerStandaloneBirth>
          standalone_births,
      std::span<const spatial::PointId> standalone_birth_point_deltas) noexcept;

  [[nodiscard]] ExactDirectSparseRootLedgerCommitResult commit(
      ExactDirectSparseRootLedgerPreparedBatch&&,
      ExactDirectSparseStableFacetForest&) noexcept;

  [[nodiscard]] ExactDirectSparseRootCoverageMaterializationResult
  materialize_active_coverages(
      std::span<const ExactDirectSparseStableFacetHandle>
          requested_forest_root_handles,
      const ExactDirectSparseRootCoverageMaterializationBudget&) const noexcept;
  [[nodiscard]] ExactDirectSparseRootLedgerCheckpoint checkpoint()
      const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  explicit ExactDirectSparseRootLedger(std::unique_ptr<Impl>) noexcept;

  friend struct ExactDirectSparseRootLedgerInitialization;
  friend ExactDirectSparseRootLedgerInitialization
  initialize_exact_direct_sparse_root_ledger(
      const ExactDirectSparseRootLedgerConfig&,
      const ExactDirectSparseRootLedgerBudget&,
      const ExactDirectSparseStableFacetForest&) noexcept;
};

struct ExactDirectSparseRootLedgerInitialization {
  std::optional<ExactDirectSparseRootLedger> ledger;
  bool sparse_empty_state_initialized{false};
  bool forest_handle_namespace_not_materialized{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseRootLedgerInitializationDecision decision{
      ExactDirectSparseRootLedgerInitializationDecision::not_initialized};

  [[nodiscard]] bool certified_initialized() const noexcept;
};

[[nodiscard]] ExactDirectSparseRootLedgerInitialization
initialize_exact_direct_sparse_root_ledger(
    const ExactDirectSparseRootLedgerConfig&,
    const ExactDirectSparseRootLedgerBudget&,
    const ExactDirectSparseStableFacetForest&) noexcept;

}  // namespace morsehgp3d::hierarchy
