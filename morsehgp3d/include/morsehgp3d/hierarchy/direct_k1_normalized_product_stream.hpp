#pragma once

#include "morsehgp3d/hierarchy/direct_k1_boruvka_closed_cut_session.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

class ExactDirectMorseNormalizedH0ProductSession;

inline constexpr std::uint32_t
    direct_k1_normalized_product_stream_schema_version = 1U;
inline constexpr std::uint32_t
    direct_k1_normalized_product_stream_checkpoint_schema_version = 1U;
inline constexpr std::size_t direct_k1_normalized_product_stream_order = 1U;
inline constexpr std::size_t
    direct_k1_normalized_product_stream_at_least20_threshold = 20U;
inline constexpr std::string_view
    direct_k1_normalized_product_stream_backend = "reference_cpu";
inline constexpr std::string_view
    direct_k1_normalized_product_stream_profile = "hgp_reduced";
inline constexpr std::string_view direct_k1_normalized_product_stream_mode =
    "certified_exact_boruvka_k1_compact_normalized_product_and_at_least20_stream_v1";
inline constexpr std::string_view
    direct_k1_normalized_product_stream_public_status = "not_claimed";
inline constexpr std::string_view
    direct_k1_normalized_product_stream_proof_basis =
        "fresh_exact_lbvh_boruvka_replay_authoritative_k1_forest_digest_"
        "compact_singleton_genesis_equal_level_csr_complete_at_least20_"
        "qvisible_fold_atomic_chain_terminal_seal_semantic_fresh_replay_"
        "restore_v1";

// The bounds below are checked from point_count before the fresh Boruvka
// replay.  The separate K1 authority budget remains the single source of the
// geometric replay and compact-forest authentication limits.
struct ExactDirectK1NormalizedProductStreamBudget {
  std::size_t maximum_point_count{};
  std::size_t maximum_product_batch_count{};
  std::size_t maximum_merge_node_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_history_digest_count{};
  std::size_t maximum_checkpoint_replay_batch_count{};
  std::size_t maximum_outstanding_prepared_batch_count{};
  std::size_t maximum_batch_at_least20_transition_count{};
  std::size_t maximum_batch_at_least20_visible_child_reference_count{};
  std::size_t maximum_active_at_least20_root_count{};
  std::size_t maximum_checkpoint_active_at_least20_root_count{};

  friend bool operator==(
      const ExactDirectK1NormalizedProductStreamBudget&,
      const ExactDirectK1NormalizedProductStreamBudget&) = default;
};

struct ExactDirectK1NormalizedProductStreamRequirements {
  std::size_t point_count{};
  std::size_t product_batch_count_upper_bound{};
  std::size_t merge_node_count_upper_bound{};
  std::size_t child_reference_count_upper_bound{};
  std::size_t history_digest_count_upper_bound{};
  std::size_t checkpoint_replay_batch_count_upper_bound{};
  std::size_t batch_at_least20_transition_count_upper_bound{};
  std::size_t batch_at_least20_visible_child_reference_count_upper_bound{};
  std::size_t active_at_least20_root_count_upper_bound{};
  std::size_t checkpoint_active_at_least20_root_count_upper_bound{};

  friend bool operator==(
      const ExactDirectK1NormalizedProductStreamRequirements&,
      const ExactDirectK1NormalizedProductStreamRequirements&) = default;
};

enum class ExactDirectK1NormalizedProductBatchKind : std::uint8_t {
  implicit_singleton_genesis,
  compact_equal_level_multifusions,
};

enum class ExactDirectK1NormalizedAtLeast20TransitionKind : std::uint8_t {
  threshold_entry,
  continuation,
  multifusion,
};

struct ExactDirectK1NormalizedAtLeast20Transition {
  std::size_t source_group_index{};
  K1NodeId resultant_root_id{};
  std::size_t source_q_r{};
  std::size_t q_visible{};
  std::size_t coverage_size{};
  std::size_t visible_child_reference_offset{};
  std::size_t visible_child_reference_count{};
  ExactDirectK1NormalizedAtLeast20TransitionKind kind{
      ExactDirectK1NormalizedAtLeast20TransitionKind::threshold_entry};
  bool coverage_size_at_least_20{false};

  friend bool operator==(
      const ExactDirectK1NormalizedAtLeast20Transition&,
      const ExactDirectK1NormalizedAtLeast20Transition&) = default;
};

// core_facet_delta_count and point_delta_count count the n singleton facets
// and points introduced at the unique closed level-zero batch.
// parent_delta_count is the number of new child-to-parent CSR references and
// node_delta_count is the number of newly born K1 nodes.  The current
// canonical-cloud/K1 contract rejects duplicate points and zero-length tree
// edges, so the closed zero partition is exactly n singleton components and
// every compact equality batch is strictly positive.  Genesis is represented
// by one range, never by point_count group objects.
struct ExactDirectK1NormalizedProductBatchRecord {
  std::uint32_t schema_version{
      direct_k1_normalized_product_stream_schema_version};
  std::size_t normalized_batch_index{};
  std::size_t order{direct_k1_normalized_product_stream_order};
  std::size_t order_batch_index{};
  exact::ExactLevel squared_level{};
  ExactDirectK1NormalizedProductBatchKind kind{
      ExactDirectK1NormalizedProductBatchKind::implicit_singleton_genesis};
  std::size_t group_count{};
  std::size_t qr0_group_count{};
  std::size_t qr1_group_count{};
  std::size_t qr_greater_equal2_group_count{};
  std::size_t core_facet_delta_count{};
  std::size_t parent_delta_count{};
  std::size_t node_delta_count{};
  std::size_t point_delta_count{};
  std::size_t merge_node_offset{};
  std::size_t merge_node_count{};
  std::size_t child_reference_offset{};
  std::size_t child_reference_count{};
  K1NodeId implicit_singleton_first_root_id{};
  std::size_t implicit_singleton_count{};
  contract::CanonicalId source_forest_digest{};
  contract::CanonicalId previous_scientific_chain_digest{};
  contract::CanonicalId scientific_chain_digest{};
  contract::CanonicalId previous_at_least20_view_digest{};
  contract::CanonicalId at_least20_transition_digest{};
  contract::CanonicalId at_least20_view_digest{};
  std::size_t at_least20_hidden_result_count{};
  std::size_t at_least20_threshold_entry_count{};
  std::size_t at_least20_continuation_count{};
  std::size_t at_least20_multifusion_count{};
  std::size_t at_least20_visible_child_reference_count{};
  std::size_t active_at_least20_root_count_before{};
  std::size_t active_at_least20_root_count_after{};
  bool complete_qr_partition_certified{false};
  bool exact_parent_node_point_deltas_certified{false};
  bool compact_csr_references_certified{false};
  bool closed_zero_level_partition_certified{false};
  bool at_least20_complete_compact_fold_certified{false};
  bool implicit_singleton_genesis_without_group_materialization{false};
  bool gamma_star_or_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectK1NormalizedProductBatchRecord&,
      const ExactDirectK1NormalizedProductBatchRecord&) = default;
};

// Every merge-node child is a prior root.  The spans point directly into the
// O(n) K1CompactForest arenas and are valid only while the owning session is
// alive.  For genesis both spans are empty and the implicit range is used.
struct ExactDirectK1NormalizedProductBatchView {
  ExactDirectK1NormalizedProductBatchKind kind{
      ExactDirectK1NormalizedProductBatchKind::implicit_singleton_genesis};
  std::span<const K1CompactMergeNode> merge_nodes;
  std::span<const K1NodeId> child_root_ids;
  K1NodeId implicit_singleton_first_root_id{};
  std::size_t implicit_singleton_count{};
  bool at_least20_prior_roots_are_csr_children{false};
  bool at_least20_coverage_delta_is_empty_after_genesis{false};
};

enum class ExactDirectK1NormalizedGenesisSliceDecision : std::uint8_t {
  not_certified,
  no_record_not_authentic_genesis,
  no_requested_slice_budget_exhausted,
  no_requested_slice_out_of_range,
  complete_implicit_singleton_slice,
};

// A downstream at_least20 adapter can seed hidden singleton roots in bounded
// chunks.  root id and PointId are both first_index + local_index; no vector
// of n group records is ever required by this seam.
struct ExactDirectK1NormalizedGenesisSingletonSlice {
  std::size_t first_index{};
  std::size_t count{};
  std::size_t maximum_requested_count{};
  bool root_id_equals_point_id{false};
  bool singleton_coverage_is_implicit{false};
  ExactDirectK1NormalizedGenesisSliceDecision decision{
      ExactDirectK1NormalizedGenesisSliceDecision::not_certified};

  [[nodiscard]] bool certified() const noexcept;
  [[nodiscard]] K1NodeId root_id(std::size_t local_index) const;
  [[nodiscard]] spatial::PointId point_id(std::size_t local_index) const;
};

struct ExactDirectK1NormalizedProductStreamStamp {
  std::uint32_t schema_version{
      direct_k1_normalized_product_stream_schema_version};
  std::uint64_t session_instance_id{};
  std::size_t next_batch_cursor{};
  std::size_t epoch{};
  exact::ExactLevel last_committed_squared_level{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_forest_digest{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId committed_scientific_chain_digest{};
  contract::CanonicalId at_least20_view_digest{};
  std::size_t active_at_least20_root_count{};

  friend bool operator==(
      const ExactDirectK1NormalizedProductStreamStamp&,
      const ExactDirectK1NormalizedProductStreamStamp&) = default;
};

enum class ExactDirectK1NormalizedProductStreamInitializationDecision
    : std::uint8_t {
  not_certified,
  no_preflight_capacity_overflow,
  no_preflight_budget_exhausted,
  no_k1_boruvka_authority_rejected,
  no_compact_k1_forest_rejected,
  no_authenticated_forest_shape_mismatch,
  no_allocation_failed,
  no_session_instance_id_exhausted,
  complete_certified_stream,
};

enum class ExactDirectK1NormalizedProductPrepareDecision : std::uint8_t {
  not_prepared,
  no_session_not_ready,
  no_stream_complete,
  no_outstanding_ticket_budget_exhausted,
  no_allocation_failed,
  complete_certified_prepared_batch,
};

enum class ExactDirectK1NormalizedProductCommitDecision : std::uint8_t {
  not_committed,
  no_session_not_ready,
  no_ticket_not_valid,
  no_foreign_ticket_rejected,
  no_stale_sibling_ticket_rejected,
  complete_certified_atomic_batch_commit,
};

class ExactDirectK1NormalizedProductPreparedBatch {
 public:
  ExactDirectK1NormalizedProductPreparedBatch() noexcept;
  ~ExactDirectK1NormalizedProductPreparedBatch();
  ExactDirectK1NormalizedProductPreparedBatch(
      ExactDirectK1NormalizedProductPreparedBatch&&) noexcept;
  ExactDirectK1NormalizedProductPreparedBatch& operator=(
      ExactDirectK1NormalizedProductPreparedBatch&&) noexcept;
  ExactDirectK1NormalizedProductPreparedBatch(
      const ExactDirectK1NormalizedProductPreparedBatch&) = delete;
  ExactDirectK1NormalizedProductPreparedBatch& operator=(
      const ExactDirectK1NormalizedProductPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactDirectK1NormalizedProductBatchRecord&
  prepared_record() const noexcept;
  [[nodiscard]] std::span<const ExactDirectK1NormalizedAtLeast20Transition>
  at_least20_transitions() const noexcept;
  [[nodiscard]] std::span<const K1NodeId>
  at_least20_visible_child_root_ids() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectK1NormalizedProductPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectK1NormalizedProductStreamSession;
};

struct ExactDirectK1NormalizedProductPreparationResult {
  std::optional<ExactDirectK1NormalizedProductPreparedBatch> ticket;
  bool no_scientific_state_mutated{false};
  bool all_allocations_and_digests_completed{false};
  ExactDirectK1NormalizedProductPrepareDecision decision{
      ExactDirectK1NormalizedProductPrepareDecision::not_prepared};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectK1NormalizedProductCommitResult {
  std::optional<ExactDirectK1NormalizedProductBatchRecord> record;
  std::vector<ExactDirectK1NormalizedAtLeast20Transition>
      at_least20_transitions;
  std::vector<K1NodeId> at_least20_visible_child_root_ids;
  ExactDirectK1NormalizedProductStreamStamp pre_stamp{};
  ExactDirectK1NormalizedProductStreamStamp post_stamp{};
  bool ticket_consumed{false};
  bool state_mutated{false};
  bool no_allocation_after_preparation{false};
  bool scientific_chain_advanced{false};
  bool public_status_claimed{false};
  ExactDirectK1NormalizedProductCommitDecision decision{
      ExactDirectK1NormalizedProductCommitDecision::not_committed};

  [[nodiscard]] bool certified_atomic_batch_commit() const noexcept;
};

struct ExactDirectK1NormalizedProductStreamCheckpoint {
  std::uint32_t schema_version{
      direct_k1_normalized_product_stream_checkpoint_schema_version};
  ExactDirectK1NormalizedProductStreamStamp stamp{};
  std::size_t total_batch_count{};
  std::size_t total_merge_node_count{};
  std::size_t total_child_reference_count{};
  std::vector<K1NodeId> active_at_least20_root_ids;
  contract::CanonicalId state_digest{};
  bool durable_file_codec_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectK1NormalizedProductStreamCheckpoint&,
      const ExactDirectK1NormalizedProductStreamCheckpoint&) = default;
};

enum class ExactDirectK1NormalizedProductCheckpointDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_outstanding_prepared_ticket,
  no_checkpoint_budget_exhausted,
  no_allocation_failed,
  complete_semantic_checkpoint,
};

struct ExactDirectK1NormalizedProductCheckpointResult {
  std::optional<ExactDirectK1NormalizedProductStreamCheckpoint> checkpoint;
  ExactDirectK1NormalizedProductCheckpointDecision decision{
      ExactDirectK1NormalizedProductCheckpointDecision::not_certified};

  [[nodiscard]] bool certified_checkpoint() const noexcept;
};

enum class ExactDirectK1NormalizedProductFinalSealDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_stream_not_complete,
  no_outstanding_prepared_ticket,
  no_terminal_at_least20_state_rejected,
  no_allocation_failed,
  complete_certified_terminal_closure,
};

struct ExactDirectK1NormalizedProductFinalSeal {
  std::uint32_t schema_version{
      direct_k1_normalized_product_stream_schema_version};
  ExactDirectK1NormalizedProductStreamStamp terminal_stamp{};
  K1NodeId root_node_id{};
  std::size_t total_batch_count{};
  std::size_t total_merge_node_count{};
  std::size_t total_child_reference_count{};
  contract::CanonicalId terminal_scientific_chain_digest{};
  contract::CanonicalId terminal_at_least20_view_digest{};
  contract::CanonicalId seal_digest{};
  bool source_stream_exhausted{false};
  bool compact_k1_root_closed{false};
  bool at_least20_terminal_state_closed{false};
  bool no_outstanding_prepared_capability{false};
  bool gamma_star_or_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1NormalizedProductFinalSealDecision decision{
      ExactDirectK1NormalizedProductFinalSealDecision::not_certified};

  [[nodiscard]] bool certified_terminal_closure() const noexcept;

  friend bool operator==(
      const ExactDirectK1NormalizedProductFinalSeal&,
      const ExactDirectK1NormalizedProductFinalSeal&) = default;
};

enum class ExactDirectK1NormalizedProductRestoreDecision : std::uint8_t {
  not_certified,
  no_checkpoint_shape_rejected,
  no_checkpoint_replay_budget_exhausted,
  no_fresh_initialization_rejected,
  no_fresh_prefix_replay_rejected,
  no_fresh_checkpoint_export_rejected,
  no_semantic_checkpoint_mismatch,
  no_session_instance_id_not_distinct,
  complete_certified_fresh_semantic_replay_restore,
};

class ExactDirectK1NormalizedProductStreamSession;
struct ExactDirectK1NormalizedProductStreamInitialization;
struct ExactDirectK1NormalizedProductStreamRestore;

class ExactDirectK1NormalizedProductStreamSession {
 public:
  static constexpr std::string_view backend =
      direct_k1_normalized_product_stream_backend;
  static constexpr std::string_view profile =
      direct_k1_normalized_product_stream_profile;
  static constexpr std::string_view mode =
      direct_k1_normalized_product_stream_mode;
  static constexpr std::string_view public_status =
      direct_k1_normalized_product_stream_public_status;
  static constexpr std::string_view proof_basis =
      direct_k1_normalized_product_stream_proof_basis;

  ExactDirectK1NormalizedProductStreamSession() noexcept;
  ~ExactDirectK1NormalizedProductStreamSession();
  ExactDirectK1NormalizedProductStreamSession(
      ExactDirectK1NormalizedProductStreamSession&&) noexcept;
  ExactDirectK1NormalizedProductStreamSession& operator=(
      ExactDirectK1NormalizedProductStreamSession&&) noexcept;
  ExactDirectK1NormalizedProductStreamSession(
      const ExactDirectK1NormalizedProductStreamSession&) = delete;
  ExactDirectK1NormalizedProductStreamSession& operator=(
      const ExactDirectK1NormalizedProductStreamSession&) = delete;

  [[nodiscard]] bool certified_stream() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t total_batch_count() const noexcept;
  [[nodiscard]] std::size_t next_batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] std::size_t outstanding_prepared_batch_count() const noexcept;
  [[nodiscard]] K1NodeId root_node_id() const noexcept;
  [[nodiscard]] const contract::CanonicalId& canonical_cloud_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_forest_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& scientific_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& scientific_chain_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& at_least20_view_digest()
      const noexcept;
  [[nodiscard]] std::size_t active_at_least20_root_count() const noexcept;
  [[nodiscard]] bool at_least20_root_active(K1NodeId root_id) const noexcept;
  [[nodiscard]] ExactDirectK1NormalizedProductStreamStamp current_stamp()
      const;

  [[nodiscard]] ExactDirectK1NormalizedProductPreparationResult prepare_next()
      noexcept;
  [[nodiscard]] ExactDirectK1NormalizedProductCommitResult commit(
      ExactDirectK1NormalizedProductPreparedBatch&& ticket) noexcept;

  [[nodiscard]] std::optional<ExactDirectK1NormalizedProductBatchView>
  batch_view(
      const ExactDirectK1NormalizedProductBatchRecord& record) const noexcept;
  [[nodiscard]] ExactDirectK1NormalizedGenesisSingletonSlice genesis_slice(
      const ExactDirectK1NormalizedProductBatchRecord& genesis_record,
      std::size_t first_index,
      std::size_t count,
      std::size_t maximum_requested_count) const noexcept;
  [[nodiscard]] ExactDirectK1NormalizedProductCheckpointResult
  export_checkpoint() const noexcept;
  [[nodiscard]] bool verify_checkpoint(
      const ExactDirectK1NormalizedProductStreamCheckpoint&) const noexcept;
  [[nodiscard]] ExactDirectK1NormalizedProductFinalSeal seal() const noexcept;
  [[nodiscard]] bool verify_final_seal(
      const ExactDirectK1NormalizedProductFinalSeal&) const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectK1NormalizedProductStreamSession(
      std::unique_ptr<Impl>) noexcept;
  [[nodiscard]] ExactDirectK1NormalizedProductBatchView private_batch_view(
      std::size_t normalized_batch_index) const noexcept;
  [[nodiscard]] const K1CompactForest& private_forest() const noexcept;

  friend struct ExactDirectK1NormalizedProductStreamInitialization;
  friend struct ExactDirectK1NormalizedProductStreamRestore;
  friend ExactDirectK1NormalizedProductStreamInitialization
  initialize_exact_direct_k1_normalized_product_stream(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const K1ExactBoruvkaResult&,
      const ExactDirectK1BoruvkaClosedCutSessionBudget&,
      const ExactDirectK1NormalizedProductStreamBudget&) noexcept;
  friend ExactDirectK1NormalizedProductStreamRestore
  restore_exact_direct_k1_normalized_product_stream(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const K1ExactBoruvkaResult&,
      const ExactDirectK1NormalizedProductStreamCheckpoint&,
      const ExactDirectK1BoruvkaClosedCutSessionBudget&,
      const ExactDirectK1NormalizedProductStreamBudget&) noexcept;
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectK1NormalizedProductStreamSession>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectK1NormalizedProductStreamSession>);

struct ExactDirectK1NormalizedProductStreamInitialization {
  ExactDirectK1NormalizedProductStreamBudget requested_budget{};
  ExactDirectK1NormalizedProductStreamRequirements requirements{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_forest_digest{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId initial_scientific_chain_digest{};
  contract::CanonicalId initial_at_least20_view_digest{};
  std::size_t exact_product_batch_count{};
  std::size_t exact_merge_node_count{};
  std::size_t exact_child_reference_count{};
  std::size_t replayed_boruvka_round_count{};
  std::size_t replayed_boruvka_component_minimum_count{};
  bool allocation_free_structural_preflight_certified{false};
  bool boruvka_authority_freshly_replayed{false};
  bool source_forest_digest_reused_from_k1_authority{false};
  bool compact_k1_forest_certified{false};
  bool implicit_genesis_and_csr_stream_certified{false};
  bool at_least20_complete_compact_view_certified{false};
  bool gamma_star_or_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1NormalizedProductStreamInitializationDecision decision{
      ExactDirectK1NormalizedProductStreamInitializationDecision::
          not_certified};
  ExactDirectK1NormalizedProductStreamSession session{};

  [[nodiscard]] bool certified_initialized_stream() const noexcept;
};

struct ExactDirectK1NormalizedProductStreamRestoreReceipt {
  ExactDirectK1NormalizedProductStreamStamp checkpoint_stamp{};
  ExactDirectK1NormalizedProductStreamStamp restored_stamp{};
  std::size_t replayed_product_batch_count{};
  bool boruvka_authority_freshly_replayed{false};
  bool compact_k1_forest_freshly_rebuilt{false};
  bool product_prefix_freshly_replayed{false};
  bool at_least20_view_freshly_replayed{false};
  bool semantic_checkpoint_equality_certified{false};
  bool session_instance_ids_distinct{false};
  bool durable_file_codec_claimed{false};
  bool gamma_star_or_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1NormalizedProductRestoreDecision decision{
      ExactDirectK1NormalizedProductRestoreDecision::not_certified};

  friend bool operator==(
      const ExactDirectK1NormalizedProductStreamRestoreReceipt&,
      const ExactDirectK1NormalizedProductStreamRestoreReceipt&) = default;
};

struct ExactDirectK1NormalizedProductStreamRestore {
  ExactDirectK1NormalizedProductStreamRestoreReceipt receipt{};
  ExactDirectK1NormalizedProductStreamSession session{};

  [[nodiscard]] bool certified_restored_stream() const noexcept;
};

[[nodiscard]] ExactDirectK1NormalizedProductStreamInitialization
initialize_exact_direct_k1_normalized_product_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& k1_authority_budget,
    const ExactDirectK1NormalizedProductStreamBudget& stream_budget) noexcept;

[[nodiscard]] ExactDirectK1NormalizedProductStreamRestore
restore_exact_direct_k1_normalized_product_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& k1_authority_budget,
    const ExactDirectK1NormalizedProductStreamBudget& stream_budget) noexcept;

}  // namespace morsehgp3d::hierarchy
