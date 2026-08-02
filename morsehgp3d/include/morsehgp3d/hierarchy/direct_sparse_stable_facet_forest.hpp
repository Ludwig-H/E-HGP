#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactDirectSparseStableFacetForestInitialization;

inline constexpr std::uint32_t direct_sparse_stable_facet_forest_schema_version =
    1U;
// The logical schema and its semantic digest remain v1.  Storage v2 adds a
// second sparse open-addressed index whose slots contain row_index + 1 only.
inline constexpr std::uint32_t
    direct_sparse_stable_facet_forest_storage_schema_version = 2U;
inline constexpr std::string_view direct_sparse_stable_facet_forest_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_stable_facet_forest_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_stable_facet_forest_mode =
    "transactional_sparse_stable_handle_full_key_positive_structure_only_v2";
inline constexpr std::string_view
    direct_sparse_stable_facet_forest_deployment_status =
        "architecture_only_reference_cpu_not_massive_qualified";
inline constexpr std::string_view direct_sparse_stable_facet_forest_public_status =
    "not_claimed";

using ExactDirectSparseStableFacetHandle = std::size_t;

// The namespace size is a scalar bound only.  Initialization never allocates
// an array proportional to it; durable state contains observed handles only.
struct ExactDirectSparseStableFacetForestConfig {
  std::size_t stable_facet_token_count{};
  contract::CanonicalId source_identity_digest{};
  // Physical lookup tuning only.  It is deliberately excluded from every
  // logical digest.  Zero is supported to force fingerprint collisions in
  // exact tests; complete-key comparison remains authoritative.
  std::uint64_t positive_key_fingerprint_mask{~std::uint64_t{0U}};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestConfig&,
      const ExactDirectSparseStableFacetForestConfig&) = default;
};

// The first four limits are cumulative durable limits.  The following limits
// bound one preparation and its canonical staging.  Outstanding tickets are
// siblings over one immutable logical epoch; only one can subsequently commit.
struct ExactDirectSparseStableFacetForestBudget {
  std::size_t maximum_observed_handle_count{};
  std::size_t maximum_committed_key_point_count{};
  std::size_t maximum_committed_operation_count{};
  std::size_t maximum_committed_batch_count{};
  std::size_t maximum_batch_insertion_request_count{};
  std::size_t maximum_batch_union_request_count{};
  std::size_t maximum_batch_key_point_count{};
  std::size_t maximum_batch_operation_count{};
  std::size_t maximum_staging_entry_count{};
  std::size_t maximum_outstanding_ticket_count{};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestBudget&,
      const ExactDirectSparseStableFacetForestBudget&) = default;
};

struct ExactDirectSparseStableFacetInsertion {
  ExactDirectSparseStableFacetHandle stable_source_facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};

  friend bool operator==(
      const ExactDirectSparseStableFacetInsertion&,
      const ExactDirectSparseStableFacetInsertion&) = default;
};

struct ExactDirectSparseStableFacetUnion {
  ExactDirectSparseStableFacetHandle left_handle{};
  ExactDirectSparseStableFacetHandle right_handle{};

  friend bool operator==(
      const ExactDirectSparseStableFacetUnion&,
      const ExactDirectSparseStableFacetUnion&) = default;
};

// One durable sparse locator/DSU row.  parent_handle always names another
// observed row.  component_size is positive exactly at roots and zero below a
// root; neither keys nor stable handles are rewritten after insertion.
struct ExactDirectSparseStableFacetForestEntry {
  ExactDirectSparseStableFacetHandle stable_source_facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};
  ExactDirectSparseStableFacetHandle parent_handle{};
  std::size_t component_size{};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestEntry&,
      const ExactDirectSparseStableFacetForestEntry&) = default;
};

struct ExactDirectSparseStableFacetForestStamp {
  std::uint32_t schema_version{
      direct_sparse_stable_facet_forest_schema_version};
  std::uint64_t session_instance_id{};
  std::size_t stable_facet_token_count{};
  std::size_t epoch{};
  std::size_t committed_batch_count{};
  std::size_t observed_handle_count{};
  std::size_t component_count{};
  std::size_t committed_key_point_count{};
  std::size_t committed_operation_count{};
  std::size_t committed_effective_union_count{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId semantic_history_digest{};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestStamp&,
      const ExactDirectSparseStableFacetForestStamp&) = default;
};

enum class ExactDirectSparseStableFacetForestInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_configuration_rejected,
  no_budget_rejected,
  no_allocation_failed,
  no_session_instance_id_exhausted,
  complete_empty_sparse_structure,
};

enum class ExactDirectSparseStableFacetForestPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_forest_rejected,
  no_outstanding_ticket_budget_exhausted,
  no_capacity_overflow,
  no_budget_exhausted,
  no_input_shape_rejected,
  contradiction_stable_handle_key_collision,
  no_union_unknown_handle_rejected,
  no_allocation_failed,
  complete_canonical_staging,
};

enum class ExactDirectSparseStableFacetForestCommitDecision : std::uint8_t {
  not_committed,
  no_forest_rejected,
  no_ticket_rejected,
  no_foreign_ticket_rejected,
  no_stale_or_sibling_ticket_rejected,
  complete_atomic_sparse_commit,
};

enum class ExactDirectSparseStableFacetLookupDisposition : std::uint8_t {
  not_certified,
  handle_out_of_namespace,
  unobserved,
  observed,
};

struct ExactDirectSparseStableFacetLookupResult {
  ExactDirectSparseStableFacetHandle requested_handle{};
  ExactDirectSparseStableFacetHandle root_handle{};
  ExactDirectSparseFacetKey facet_key{};
  std::size_t component_size{};
  bool forest_certified_at_entry{false};
  bool immutable_handle_key_binding_observed{false};
  bool root_resolved_without_mutation{false};
  bool missing_handle_means_source_absent{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetLookupDisposition disposition{
      ExactDirectSparseStableFacetLookupDisposition::not_certified};

  [[nodiscard]] bool certified_observed() const noexcept;
  [[nodiscard]] bool certified_unobserved() const noexcept;

  friend bool operator==(
      const ExactDirectSparseStableFacetLookupResult&,
      const ExactDirectSparseStableFacetLookupResult&) = default;
};

enum class ExactDirectSparseStableFacetPositiveLookupDisposition
    : std::uint8_t {
  not_certified,
  input_key_rejected,
  unobserved,
  observed,
  contradiction,
};

// Allocation-free positive lookup against one immutable forest snapshot.
// The table slot stores only row_index + 1.  A fingerprint selects and filters
// probes, but an observed answer is certified only after comparing the full
// fixed-capacity key stored in the append-only row.
struct ExactDirectSparseStableFacetPositiveLookupReceipt {
  ExactDirectSparseFacetKey requested_key{};
  ExactDirectSparseStableFacetHandle bound_handle{};
  ExactDirectSparseStableFacetHandle root_handle{};
  std::size_t component_size{};
  ExactDirectSparseStableFacetForestStamp forest_stamp{};
  std::size_t slot_visit_count{};
  std::size_t full_key_comparison_count{};
  bool forest_certified_at_entry{false};
  bool requested_key_canonical{false};
  bool lookup_completed_without_mutation{false};
  bool fingerprint_used_only_as_accelerator{false};
  bool complete_key_comparison_authoritative{false};
  bool immutable_key_handle_binding_observed{false};
  bool root_resolved_without_mutation{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetPositiveLookupDisposition disposition{
      ExactDirectSparseStableFacetPositiveLookupDisposition::not_certified};

  friend bool operator==(
      const ExactDirectSparseStableFacetPositiveLookupReceipt&,
      const ExactDirectSparseStableFacetPositiveLookupReceipt&) = default;
};

// The public receipt remains directly readable, while certification is bound
// to an inline private snapshot minted only by the forest.  Copying a genuine
// lookup copies its snapshot; default construction and public field edits
// cannot forge certification.  No heap allocation or digest is involved.
class ExactDirectSparseStableFacetPositiveLookupResult final
    : public ExactDirectSparseStableFacetPositiveLookupReceipt {
 public:
  ExactDirectSparseStableFacetPositiveLookupResult() noexcept = default;
  ~ExactDirectSparseStableFacetPositiveLookupResult() = default;
  ExactDirectSparseStableFacetPositiveLookupResult(
      const ExactDirectSparseStableFacetPositiveLookupResult&) noexcept =
      default;
  ExactDirectSparseStableFacetPositiveLookupResult& operator=(
      const ExactDirectSparseStableFacetPositiveLookupResult&) noexcept =
      default;
  ExactDirectSparseStableFacetPositiveLookupResult(
      ExactDirectSparseStableFacetPositiveLookupResult&&) noexcept = default;
  ExactDirectSparseStableFacetPositiveLookupResult& operator=(
      ExactDirectSparseStableFacetPositiveLookupResult&&) noexcept = default;

  [[nodiscard]] bool certified_observed() const noexcept;
  [[nodiscard]] bool certified_unobserved() const noexcept;
  [[nodiscard]] bool certified_observed_for(
      const ExactDirectSparseStableFacetForestStamp&) const noexcept;
  [[nodiscard]] bool certified_unobserved_for(
      const ExactDirectSparseStableFacetForestStamp&) const noexcept;

  friend bool operator==(
      const ExactDirectSparseStableFacetPositiveLookupResult&,
      const ExactDirectSparseStableFacetPositiveLookupResult&) = default;

 private:
  explicit ExactDirectSparseStableFacetPositiveLookupResult(
      const ExactDirectSparseStableFacetPositiveLookupReceipt&) noexcept;

  ExactDirectSparseStableFacetPositiveLookupReceipt private_receipt_snapshot_{};
  bool minted_{false};

  friend class ExactDirectSparseStableFacetForest;
};

class ExactDirectSparseStableFacetForestPreparedBatch {
 public:
  ExactDirectSparseStableFacetForestPreparedBatch() noexcept;
  ~ExactDirectSparseStableFacetForestPreparedBatch();
  ExactDirectSparseStableFacetForestPreparedBatch(
      ExactDirectSparseStableFacetForestPreparedBatch&&) noexcept;
  ExactDirectSparseStableFacetForestPreparedBatch& operator=(
      ExactDirectSparseStableFacetForestPreparedBatch&&) noexcept;
  ExactDirectSparseStableFacetForestPreparedBatch(
      const ExactDirectSparseStableFacetForestPreparedBatch&) = delete;
  ExactDirectSparseStableFacetForestPreparedBatch& operator=(
      const ExactDirectSparseStableFacetForestPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp& pre_stamp()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& canonical_batch_digest()
      const noexcept;
  [[nodiscard]] std::size_t insertion_request_count() const noexcept;
  [[nodiscard]] std::size_t union_request_count() const noexcept;
  [[nodiscard]] std::size_t new_handle_count() const noexcept;
  [[nodiscard]] std::size_t compatible_repeat_count() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectSparseStableFacetForestPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectSparseStableFacetForest;
  friend class ExactDirectSparseStableFacetForestPreparedPreview;
};

// A preview is bounded independently from the durable forest budget.  The
// shadow-node upper bound is conservative: every new handle, requested
// handle, and union endpoint may contribute one sparse pre-root.  The total
// entry bound covers that shadow arena plus the requested output records.
struct ExactDirectSparseStableFacetForestPreparedPreviewBudget {
  std::size_t maximum_requested_handle_count{};
  std::size_t maximum_shadow_node_count{};
  std::size_t maximum_union_replay_count{};
  std::size_t maximum_total_entry_count{};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestPreparedPreviewBudget&,
      const ExactDirectSparseStableFacetForestPreparedPreviewBudget&) =
      default;
};

struct ExactDirectSparseStableFacetForestPreparedPreviewRecord {
  ExactDirectSparseStableFacetHandle requested_handle{};
  ExactDirectSparseStableFacetHandle post_root_handle{};
  std::size_t post_component_size{};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestPreparedPreviewRecord&,
      const ExactDirectSparseStableFacetForestPreparedPreviewRecord&) =
      default;
};

enum class ExactDirectSparseStableFacetForestPreparedPreviewDecision
    : std::uint8_t {
  not_prepared,
  no_forest_rejected,
  no_ticket_rejected,
  no_foreign_ticket_rejected,
  no_stale_or_sibling_ticket_rejected,
  no_capacity_overflow,
  no_budget_exhausted,
  no_requested_handle_shape_rejected,
  no_requested_handle_unknown_rejected,
  no_allocation_failed,
  complete_sealed_sparse_shadow_preview,
};

// This receipt is readable and copyable for transaction planning and audit.
// It cannot itself authorize anything: only a privately minted preview can
// certify an exact receipt and bind it to one live prepared-ticket identity.
struct ExactDirectSparseStableFacetForestPreparedPreviewReceipt {
  ExactDirectSparseStableFacetForestStamp pre_stamp{};
  contract::CanonicalId canonical_batch_digest{};
  std::size_t requested_handle_count{};
  std::size_t shadow_node_count{};
  std::size_t shadow_node_upper_bound{};
  std::size_t union_request_count{};
  std::size_t expected_effective_union_count{};
  std::size_t total_entry_upper_bound{};
  std::vector<ExactDirectSparseStableFacetForestPreparedPreviewRecord>
      records;
  bool ticket_identity_bound{false};
  bool all_overflow_and_budget_checks_completed_before_allocation{false};
  bool sparse_pre_roots_and_new_handles_only{false};
  bool canonical_union_replay_exact{false};
  bool forest_logical_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestPreparedPreviewDecision decision{
      ExactDirectSparseStableFacetForestPreparedPreviewDecision::
          not_prepared};

  friend bool operator==(
      const ExactDirectSparseStableFacetForestPreparedPreviewReceipt&,
      const ExactDirectSparseStableFacetForestPreparedPreviewReceipt&) =
      default;
};

// Move-only, privately minted capability.  Its immutable receipt and private
// ticket identity make a copied or edited public receipt non-authoritative.
class ExactDirectSparseStableFacetForestPreparedPreview final {
 public:
  ExactDirectSparseStableFacetForestPreparedPreview() noexcept;
  ~ExactDirectSparseStableFacetForestPreparedPreview();
  ExactDirectSparseStableFacetForestPreparedPreview(
      ExactDirectSparseStableFacetForestPreparedPreview&&) noexcept;
  ExactDirectSparseStableFacetForestPreparedPreview& operator=(
      ExactDirectSparseStableFacetForestPreparedPreview&&) noexcept;
  ExactDirectSparseStableFacetForestPreparedPreview(
      const ExactDirectSparseStableFacetForestPreparedPreview&) = delete;
  ExactDirectSparseStableFacetForestPreparedPreview& operator=(
      const ExactDirectSparseStableFacetForestPreparedPreview&) = delete;

  [[nodiscard]] const
  ExactDirectSparseStableFacetForestPreparedPreviewReceipt&
  receipt() const noexcept;
  [[nodiscard]] std::span<
      const ExactDirectSparseStableFacetForestPreparedPreviewRecord>
  records() const noexcept;
  [[nodiscard]] bool certified_sparse_shadow_preview() const noexcept;
  [[nodiscard]] bool certified_for(
      const ExactDirectSparseStableFacetForestPreparedBatch&,
      const ExactDirectSparseStableFacetForestStamp& expected_pre_stamp)
      const noexcept;
  [[nodiscard]] bool certifies_receipt(
      const ExactDirectSparseStableFacetForestPreparedPreviewReceipt&) const
      noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectSparseStableFacetForestPreparedPreview(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectSparseStableFacetForest;
};

struct ExactDirectSparseStableFacetForestPreparedPreviewResult {
  std::optional<ExactDirectSparseStableFacetForestPreparedPreview> preview;
  ExactDirectSparseStableFacetForestStamp pre_stamp{};
  std::size_t requested_handle_count{};
  std::size_t shadow_node_upper_bound{};
  std::size_t union_request_count{};
  std::size_t total_entry_upper_bound{};
  bool all_overflow_and_budget_checks_completed_before_allocation{false};
  bool forest_logical_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestPreparedPreviewDecision decision{
      ExactDirectSparseStableFacetForestPreparedPreviewDecision::
          not_prepared};

  [[nodiscard]] bool certified_prepared_preview() const noexcept;
};

struct ExactDirectSparseStableFacetForestPreparationResult {
  std::optional<ExactDirectSparseStableFacetForestPreparedBatch> ticket;
  ExactDirectSparseStableFacetForestStamp pre_stamp{};
  std::size_t insertion_request_count{};
  std::size_t union_request_count{};
  std::size_t batch_key_point_count{};
  std::size_t staging_entry_count{};
  std::size_t new_handle_count{};
  std::size_t compatible_repeat_count{};
  std::size_t duplicate_union_request_count{};
  bool canonical_order_independent_staging{false};
  bool all_commit_allocations_completed{false};
  bool forest_logical_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestPreparationDecision decision{
      ExactDirectSparseStableFacetForestPreparationDecision::not_prepared};

  [[nodiscard]] bool certified_prepared() const noexcept;
};

struct ExactDirectSparseStableFacetForestCommitResult {
  ExactDirectSparseStableFacetForestStamp pre_stamp{};
  ExactDirectSparseStableFacetForestStamp post_stamp{};
  std::size_t inserted_handle_count{};
  std::size_t compatible_repeat_count{};
  std::size_t union_request_count{};
  std::size_t effective_union_count{};
  bool ticket_consumed{false};
  bool state_mutated{false};
  bool no_allocation_after_preparation{false};
  bool immutable_handle_key_bindings_preserved{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestCommitDecision decision{
      ExactDirectSparseStableFacetForestCommitDecision::not_committed};

  [[nodiscard]] bool certified_commit() const noexcept;
};

enum class ExactDirectSparseStableFacetForestCheckpointDecision
    : std::uint8_t {
  not_available,
  complete_honest_nonrestartable_semantic_checkpoint,
};

// This is deliberately metadata, not a restore image.  Sparse entries, keys
// and DSU parents are absent, so a matching digest cannot restart the forest.
struct ExactDirectSparseStableFacetForestCheckpoint {
  std::uint32_t schema_version{
      direct_sparse_stable_facet_forest_schema_version};
  ExactDirectSparseStableFacetForestStamp stamp{};
  contract::CanonicalId checkpoint_digest{};
  bool process_local_session_identity_only{false};
  bool semantic_metadata_complete{false};
  bool sparse_entries_serialized{false};
  bool immutable_keys_serialized{false};
  bool dsu_state_serialized{false};
  bool checkpoint_restore_supported{false};
  bool restartable{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestCheckpointDecision decision{
      ExactDirectSparseStableFacetForestCheckpointDecision::not_available};

  [[nodiscard]] bool certified_honest_nonrestartable() const noexcept;

  friend bool operator==(
      const ExactDirectSparseStableFacetForestCheckpoint&,
      const ExactDirectSparseStableFacetForestCheckpoint&) = default;
};

class ExactDirectSparseStableFacetForest {
 public:
  static constexpr std::string_view backend =
      direct_sparse_stable_facet_forest_backend;
  static constexpr std::string_view profile =
      direct_sparse_stable_facet_forest_profile;
  static constexpr std::string_view mode =
      direct_sparse_stable_facet_forest_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_stable_facet_forest_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_stable_facet_forest_public_status;

  ExactDirectSparseStableFacetForest() noexcept;
  ~ExactDirectSparseStableFacetForest();
  ExactDirectSparseStableFacetForest(
      ExactDirectSparseStableFacetForest&&) noexcept;
  ExactDirectSparseStableFacetForest& operator=(
      ExactDirectSparseStableFacetForest&&) noexcept;
  ExactDirectSparseStableFacetForest(
      const ExactDirectSparseStableFacetForest&) = delete;
  ExactDirectSparseStableFacetForest& operator=(
      const ExactDirectSparseStableFacetForest&) = delete;

  [[nodiscard]] bool certified_structure_only_forest() const noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetForestStamp current_stamp()
      const noexcept;
  [[nodiscard]] std::size_t outstanding_ticket_count() const noexcept;
  // Rows are append-only in canonical handle order within each committed
  // batch.  Their order is intentionally not globally sorted across batches.
  // A prepare_batch call may reserve durable row capacity and therefore
  // invalidate a previously returned span even when no logical commit follows.
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetForestEntry>
  observed_entries() const noexcept;
  // Physical architecture metric only.  The index is empty at initialization
  // and grows from observed/staged handle requirements, never from the scalar
  // stable_facet_token_count namespace bound.
  [[nodiscard]] std::size_t materialized_handle_index_slot_count()
      const noexcept;
  // Physical architecture metric only.  Like the handle index, this remains
  // empty at initialization and grows from observed rows, never from the
  // scalar source namespace bound.
  [[nodiscard]] std::size_t materialized_positive_key_index_slot_count()
      const noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetLookupResult lookup(
      ExactDirectSparseStableFacetHandle) const noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetPositiveLookupResult
  lookup_positive_full_key(const ExactDirectSparseFacetKey&) const noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetForestPreparationResult
  prepare_batch(
      std::span<const ExactDirectSparseStableFacetInsertion>,
      std::span<const ExactDirectSparseStableFacetUnion>) noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetForestPreparedPreviewResult
  preview_prepared_batch(
      const ExactDirectSparseStableFacetForestPreparedBatch&,
      std::span<const ExactDirectSparseStableFacetHandle> requested_handles,
      const ExactDirectSparseStableFacetForestPreparedPreviewBudget&) const
      noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetForestCommitResult commit(
      ExactDirectSparseStableFacetForestPreparedBatch&&) noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetForestCheckpoint checkpoint()
      const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectSparseStableFacetForest(std::unique_ptr<Impl>) noexcept;
  friend struct ExactDirectSparseStableFacetForestInitialization;
  friend ExactDirectSparseStableFacetForestInitialization
  initialize_exact_direct_sparse_stable_facet_forest(
      const ExactDirectSparseStableFacetForestConfig&,
      const ExactDirectSparseStableFacetForestBudget&) noexcept;
};

struct ExactDirectSparseStableFacetForestInitialization {
  std::optional<ExactDirectSparseStableFacetForest> forest;
  bool sparse_empty_state_initialized{false};
  bool namespace_capacity_not_materialized{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestInitializationDecision decision{
      ExactDirectSparseStableFacetForestInitializationDecision::
          not_initialized};

  [[nodiscard]] bool certified_initialized() const noexcept;
};

[[nodiscard]] ExactDirectSparseStableFacetForestInitialization
initialize_exact_direct_sparse_stable_facet_forest(
    const ExactDirectSparseStableFacetForestConfig&,
    const ExactDirectSparseStableFacetForestBudget&) noexcept;

}  // namespace morsehgp3d::hierarchy
