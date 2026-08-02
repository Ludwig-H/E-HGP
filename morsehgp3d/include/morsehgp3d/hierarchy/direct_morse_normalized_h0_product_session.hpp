#pragma once

#include "morsehgp3d/hierarchy/direct_at_least20_stream_view.hpp"
#include "morsehgp3d/hierarchy/direct_k1_normalized_product_stream.hpp"
#include "morsehgp3d/hierarchy/direct_morse_resident_all_orders_vertical_bridge.hpp"
#include "morsehgp3d/hierarchy/normalized_exact_hartigan_level_manifest.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_normalized_h0_product_session_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_normalized_h0_product_session_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_normalized_h0_product_session_profile = "hgp_reduced";
inline constexpr std::string_view direct_morse_normalized_h0_product_session_mode =
    "certified_bounded_normalized_h0_exact_product_order_transaction_v1";
inline constexpr std::string_view
    direct_morse_normalized_h0_product_session_deployment_status =
        "bounded_reference_cpu_in_memory_non_resumable_vertical_session";
inline constexpr std::string_view
    direct_morse_normalized_h0_product_session_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_normalized_h0_product_session_proof_basis =
        "owned_k1_compact_stream_owned_normalized_resident_all_orders_v7_"
        "bridge_exact_level_order_merge_one_outer_move_only_ticket_atomic_"
        "higher_at_least20_projection_structural_empty_batch_transit_without_"
        "false_normalized_record_and_streaming_hartigan_v4_closure_v1";

struct ExactDirectMorseNormalizedH0ProductBudget {
  std::size_t maximum_product_batch_count{};
  std::size_t maximum_outstanding_prepared_batch_count{};
  ExactDirectAtLeast20StreamViewBudget higher_at_least20{};
  ExactNormalizedHartiganLevelManifestBudget hartigan{};

  friend bool operator==(
      const ExactDirectMorseNormalizedH0ProductBudget&,
      const ExactDirectMorseNormalizedH0ProductBudget&) = default;
};

struct ExactDirectMorseNormalizedH0ProductRequirements {
  std::size_t k1_batch_count{};
  std::size_t resident_batch_count{};
  std::size_t product_batch_count{};
  std::size_t maximum_order{};

  friend bool operator==(
      const ExactDirectMorseNormalizedH0ProductRequirements&,
      const ExactDirectMorseNormalizedH0ProductRequirements&) = default;
};

enum class ExactDirectMorseNormalizedH0ProductNextSource : std::uint8_t {
  k1,
  resident,
  complete,
};

struct ExactDirectMorseNormalizedH0ProductStamp {
  std::uint32_t schema_version{
      direct_morse_normalized_h0_product_session_schema_version};
  std::uint64_t session_instance_id{};
  std::size_t product_batch_cursor{};
  std::size_t epoch{};
  std::size_t hartigan_record_count{};
  std::size_t k1_batch_cursor{};
  std::size_t resident_batch_cursor{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId initial_product_chain_digest{};
  contract::CanonicalId product_chain_digest{};
  contract::CanonicalId hartigan_chain_digest{};

  friend bool operator==(
      const ExactDirectMorseNormalizedH0ProductStamp&,
      const ExactDirectMorseNormalizedH0ProductStamp&) = default;
};

enum class ExactDirectMorseNormalizedH0ProductInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_source_session_rejected,
  no_non_genesis_source_rejected,
  no_normalized_v7_source_premise_rejected,
  no_source_identity_mismatch,
  no_product_capacity_overflow,
  no_product_budget_rejected,
  no_higher_view_initialization_rejected,
  no_hartigan_initialization_rejected,
  no_session_instance_id_exhausted,
  no_allocation_failed,
  complete_certified_bounded_product_session,
};

enum class ExactDirectMorseNormalizedH0ProductPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_session_not_ready,
  no_product_complete,
  no_outstanding_ticket_budget_exhausted,
  no_source_preparation_rejected,
  no_source_schedule_mismatch,
  no_higher_projection_rejected,
  no_higher_view_preparation_rejected,
  no_hartigan_preparation_rejected,
  no_allocation_failed,
  complete_prepared_atomic_product_batch,
};

enum class ExactDirectMorseNormalizedH0ProductCommitDecision
    : std::uint8_t {
  not_committed,
  no_session_not_ready,
  no_ticket_not_valid,
  no_foreign_or_stale_ticket_rejected,
  no_source_commit_rejected,
  complete_committed_atomic_product_batch,
};

class ExactDirectMorseNormalizedH0ProductPreparedBatch {
 public:
  ExactDirectMorseNormalizedH0ProductPreparedBatch() noexcept;
  ~ExactDirectMorseNormalizedH0ProductPreparedBatch();
  ExactDirectMorseNormalizedH0ProductPreparedBatch(
      ExactDirectMorseNormalizedH0ProductPreparedBatch&&) noexcept;
  ExactDirectMorseNormalizedH0ProductPreparedBatch& operator=(
      ExactDirectMorseNormalizedH0ProductPreparedBatch&&) noexcept;
  ExactDirectMorseNormalizedH0ProductPreparedBatch(
      const ExactDirectMorseNormalizedH0ProductPreparedBatch&) = delete;
  ExactDirectMorseNormalizedH0ProductPreparedBatch& operator=(
      const ExactDirectMorseNormalizedH0ProductPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductNextSource source()
      const noexcept;
  [[nodiscard]] std::size_t normalized_batch_index() const noexcept;
  [[nodiscard]] std::size_t order() const noexcept;
  [[nodiscard]] const exact::ExactLevel& squared_level() const noexcept;
  [[nodiscard]] bool emits_hartigan_record() const noexcept;
  [[nodiscard]] const ExactNormalizedHartiganLevelManifestRecord*
  prepared_hartigan_record() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseNormalizedH0ProductPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseNormalizedH0ProductPreparedBatch>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseNormalizedH0ProductPreparedBatch>);

struct ExactDirectMorseNormalizedH0ProductPreparationResult {
  std::optional<ExactDirectMorseNormalizedH0ProductPreparedBatch> ticket;
  ExactDirectMorseNormalizedH0ProductNextSource source{
      ExactDirectMorseNormalizedH0ProductNextSource::complete};
  bool no_scientific_state_mutated{false};
  bool all_fallible_work_completed{false};
  bool no_global_facet_coface_or_gamma_catalog_materialized{false};
  bool emits_hartigan_record{false};
  ExactNormalizedHartiganLevelAppendDecision hartigan_decision{
      ExactNormalizedHartiganLevelAppendDecision::not_certified};
  ExactDirectMorseNormalizedH0ProductPreparationDecision decision{
      ExactDirectMorseNormalizedH0ProductPreparationDecision::not_prepared};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectMorseNormalizedH0ProductCommitResult {
  std::optional<ExactDirectK1NormalizedProductCommitResult> k1_commit;
  std::optional<ExactDirectMorseResidentAllOrdersVerticalCommitResult>
      resident_commit;
  std::optional<ExactDirectAtLeast20StreamConsumeResult>
      higher_at_least20_commit;
  std::optional<ExactNormalizedHartiganLevelAppendResult> hartigan_commit;
  ExactDirectMorseNormalizedH0ProductStamp pre_stamp{};
  ExactDirectMorseNormalizedH0ProductStamp post_stamp{};
  ExactDirectMorseNormalizedH0ProductNextSource source{
      ExactDirectMorseNormalizedH0ProductNextSource::complete};
  bool ticket_consumed{false};
  bool state_mutated{false};
  bool no_allocation_after_preparation{false};
  bool product_chain_advanced{false};
  bool emits_hartigan_record{false};
  bool public_status_claimed{false};
  ExactDirectMorseNormalizedH0ProductCommitDecision decision{
      ExactDirectMorseNormalizedH0ProductCommitDecision::not_committed};

  [[nodiscard]] bool certified_atomic_product_commit() const noexcept;
};

// This is deliberately only an in-memory semantic observation.  K1, the
// higher-order cap-20 view and Hartigan each export authenticated checkpoints,
// but the owned all-orders vertical bridge has no restore seam.  Consequently
// no restore entry point accepts this object and restart_supported remains
// false; serializing it cannot create a durable-resume claim.
struct ExactDirectMorseNormalizedH0ProductSemanticCheckpoint {
  std::uint32_t schema_version{
      direct_morse_normalized_h0_product_session_schema_version};
  ExactDirectMorseNormalizedH0ProductStamp stamp{};
  ExactDirectMorseResidentAllOrdersVerticalBridgeStamp vertical_stamp{};
  ExactDirectK1NormalizedProductStreamCheckpoint k1{};
  ExactDirectAtLeast20StreamViewCheckpoint higher_at_least20{};
  ExactNormalizedHartiganLevelManifestCheckpoint hartigan{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      record_count_by_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      omitted_noop_count_by_order{};
  contract::CanonicalId semantic_digest{};
  bool vertical_restore_available{false};
  bool restart_supported{false};
  bool durable_file_codec_claimed{false};
  bool public_status_claimed{false};
};

enum class ExactDirectMorseNormalizedH0ProductCheckpointDecision
    : std::uint8_t {
  not_built,
  no_session_not_ready,
  no_outstanding_prepared_ticket,
  no_component_checkpoint_rejected,
  no_allocation_failed,
  complete_semantic_snapshot_restart_explicitly_unavailable,
};

struct ExactDirectMorseNormalizedH0ProductCheckpointResult {
  std::optional<ExactDirectMorseNormalizedH0ProductSemanticCheckpoint>
      checkpoint;
  ExactDirectMorseNormalizedH0ProductCheckpointDecision decision{
      ExactDirectMorseNormalizedH0ProductCheckpointDecision::not_built};

  [[nodiscard]] bool certified_nonresumable_semantic_snapshot()
      const noexcept;
};

struct ExactDirectMorseNormalizedH0ProductFinalSeal {
  std::uint32_t schema_version{
      direct_morse_normalized_h0_product_session_schema_version};
  ExactDirectMorseNormalizedH0ProductStamp terminal_stamp{};
  ExactDirectK1NormalizedProductFinalSeal k1{};
  ExactDirectMorseResidentAllOrdersVerticalFinalSeal vertical{};
  ExactDirectAtLeast20StreamViewCheckpoint higher_at_least20{};
  ExactNormalizedHartiganLevelManifestSeal hartigan{};
  contract::CanonicalId seal_digest{};
  std::size_t structural_source_transition_count{};
  std::size_t hartigan_record_count{};
  bool normalized_incidence_complete_v7_source_capability{false};
  bool every_product_batch_committed_in_exact_level_order{false};
  bool empty_structural_batches_excluded_from_hartigan_manifest{false};
  bool higher_at_least20_complete{false};
  bool hartigan_streaming_closed{false};
  bool no_global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_morse_obligation_replayed{false};
  bool m1_replayed{false};
  bool public_status_claimed{false};
};

enum class ExactDirectMorseNormalizedH0ProductSealDecision : std::uint8_t {
  not_sealed,
  no_session_not_ready,
  no_product_not_complete,
  no_outstanding_prepared_ticket,
  no_k1_seal_rejected,
  no_vertical_seal_rejected,
  no_higher_view_checkpoint_rejected,
  no_hartigan_closure_rejected,
  complete_certified_bounded_normalized_product_seal,
};

struct ExactDirectMorseNormalizedH0ProductSealResult {
  std::shared_ptr<const ExactDirectMorseNormalizedH0ProductFinalSeal> seal;
  bool final_seal_published{false};
  bool existing_final_seal_returned_without_mutation{false};
  ExactDirectMorseNormalizedH0ProductSealDecision decision{
      ExactDirectMorseNormalizedH0ProductSealDecision::not_sealed};

  [[nodiscard]] bool certified_bounded_product_seal() const noexcept;
};

struct ExactDirectMorseNormalizedH0ProductInitialization;

class ExactDirectMorseNormalizedH0ProductSession {
 public:
  struct Impl;

  static constexpr std::string_view backend =
      direct_morse_normalized_h0_product_session_backend;
  static constexpr std::string_view profile =
      direct_morse_normalized_h0_product_session_profile;
  static constexpr std::string_view mode =
      direct_morse_normalized_h0_product_session_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_normalized_h0_product_session_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_normalized_h0_product_session_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_normalized_h0_product_session_proof_basis;

  ExactDirectMorseNormalizedH0ProductSession() noexcept;
  ~ExactDirectMorseNormalizedH0ProductSession();
  ExactDirectMorseNormalizedH0ProductSession(
      ExactDirectMorseNormalizedH0ProductSession&&) noexcept;
  ExactDirectMorseNormalizedH0ProductSession& operator=(
      ExactDirectMorseNormalizedH0ProductSession&&) noexcept;
  ExactDirectMorseNormalizedH0ProductSession(
      const ExactDirectMorseNormalizedH0ProductSession&) = delete;
  ExactDirectMorseNormalizedH0ProductSession& operator=(
      const ExactDirectMorseNormalizedH0ProductSession&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool sealed() const noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductNextSource next_source()
      const noexcept;
  [[nodiscard]] std::size_t product_batch_count() const noexcept;
  [[nodiscard]] std::size_t product_batch_cursor() const noexcept;
  [[nodiscard]] std::size_t hartigan_record_count() const noexcept;
  [[nodiscard]] std::size_t maximum_order() const noexcept;
  [[nodiscard]] std::size_t outstanding_prepared_batch_count() const noexcept;
  [[nodiscard]] const contract::CanonicalId& scientific_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& product_chain_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& hartigan_chain_digest()
      const noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductStamp current_stamp()
      const noexcept;
  [[nodiscard]] const ExactDirectK1NormalizedProductStreamSession& k1()
      const noexcept;
  [[nodiscard]] const ExactDirectMorseResidentAllOrdersVerticalBridge&
  vertical() const noexcept;
  [[nodiscard]] const ExactDirectAtLeast20StreamViewSession&
  higher_at_least20() const noexcept;
  [[nodiscard]] const ExactNormalizedHartiganLevelManifestSession& hartigan()
      const noexcept;

  [[nodiscard]] ExactDirectMorseNormalizedH0ProductPreparationResult
  prepare_next() noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductCommitResult commit(
      ExactDirectMorseNormalizedH0ProductPreparedBatch&&) noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductCheckpointResult
  make_semantic_checkpoint() const noexcept;
  [[nodiscard]] ExactDirectMorseNormalizedH0ProductSealResult seal() noexcept;

 private:
  [[nodiscard]] static contract::CanonicalId
  resident_scientific_source_digest(
      const ExactDirectSparseUnifiedLevelPlanResult&,
      ExactDirectMorseUnifiedResidentSourceKind);
  [[nodiscard]] static ExactNormalizedHartiganLevelInitializationResult
  initialize_owned_streaming_hartigan(
      std::uint64_t,
      std::size_t,
      const contract::CanonicalId&,
      const contract::CanonicalId&,
      const ExactNormalizedHartiganLevelManifestBudget&);
  explicit ExactDirectMorseNormalizedH0ProductSession(
      std::unique_ptr<Impl>) noexcept;
  std::unique_ptr<Impl> impl_;

  friend struct ExactDirectMorseNormalizedH0ProductInitialization;
  friend ExactDirectMorseNormalizedH0ProductInitialization
  initialize_exact_direct_morse_normalized_h0_product_session(
      ExactDirectMorseResidentAllOrdersVerticalBridge&&,
      ExactDirectK1NormalizedProductStreamSession&&,
      std::uint64_t,
      const ExactDirectMorseNormalizedH0ProductBudget&) noexcept;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseNormalizedH0ProductSession>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseNormalizedH0ProductSession>);

struct ExactDirectMorseNormalizedH0ProductInitialization {
  ExactDirectMorseNormalizedH0ProductRequirements requirements{};
  ExactDirectMorseNormalizedH0ProductBudget requested_budget{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId initial_product_chain_digest{};
  bool source_sessions_consumed{false};
  bool exact_level_order_scheduler_certified{false};
  bool no_global_facet_coface_or_gamma_catalog_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseNormalizedH0ProductInitializationDecision decision{
      ExactDirectMorseNormalizedH0ProductInitializationDecision::
          not_initialized};
  ExactDirectMorseNormalizedH0ProductSession session{};

  [[nodiscard]] bool certified_initialized_session() const noexcept;
};

[[nodiscard]] ExactDirectMorseNormalizedH0ProductInitialization
initialize_exact_direct_morse_normalized_h0_product_session(
    ExactDirectMorseResidentAllOrdersVerticalBridge&& vertical,
    ExactDirectK1NormalizedProductStreamSession&& k1,
    std::uint64_t product_authority_id,
    const ExactDirectMorseNormalizedH0ProductBudget& budget) noexcept;

}  // namespace morsehgp3d::hierarchy
