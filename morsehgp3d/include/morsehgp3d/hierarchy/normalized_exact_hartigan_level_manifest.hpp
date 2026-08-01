#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace morsehgp3d::hierarchy {

struct ExactNormalizedHartiganLevelInitializationResult;
class ExactDirectMorseNormalizedH0ProductSession;

inline constexpr std::uint32_t
    normalized_exact_hartigan_level_manifest_schema_version = 4U;
inline constexpr std::size_t
    normalized_exact_hartigan_level_manifest_maximum_order = 10U;
inline constexpr std::size_t
    normalized_exact_hartigan_level_manifest_maximum_decimal_digits = 4096U;
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_backend = "reference_cpu";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_profile = "hgp_reduced";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_mode =
        "certified_bounded_memory_transactional_normalized_exact_rational_record_chain_v4";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_public_status = "not_claimed";
inline constexpr bool
    normalized_exact_hartigan_level_manifest_product_complete = false;
inline constexpr bool
    normalized_exact_hartigan_level_manifest_vertical_maps_complete = false;
inline constexpr bool
    normalized_exact_hartigan_level_manifest_global_gamma_star_or_delaunay_materialized =
        false;
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_record_schema =
        "morsehgp3d.phase15.normalized_exact_hartigan_level_manifest_record.v4";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_representation =
        "canonical_reduced_rational_decimal_v1";
inline constexpr std::uint32_t
    normalized_exact_hartigan_level_manifest_checkpoint_schema_version = 1U;
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_checkpoint_schema =
        "morsehgp3d.phase15.normalized_exact_hartigan_level_manifest_checkpoint.v1";

struct ExactNormalizedHartiganLevelManifestBudget {
  std::size_t maximum_record_count{};
  std::size_t maximum_decimal_digits_per_integer{};

  friend bool operator==(
      const ExactNormalizedHartiganLevelManifestBudget&,
      const ExactNormalizedHartiganLevelManifestBudget&) = default;
};

struct ExactNormalizedHartiganLevelManifestContract {
  std::uint64_t authority_id{};
  std::size_t maximum_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      expected_record_count_by_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      expected_omitted_noop_count_by_order{};
  contract::CanonicalId normalized_source_scientific_digest{};
  contract::CanonicalId normalized_batch_initial_chain_digest{};
  contract::CanonicalId normalized_batch_final_chain_digest{};

  friend bool operator==(
      const ExactNormalizedHartiganLevelManifestContract&,
      const ExactNormalizedHartiganLevelManifestContract&) = default;
};

enum class ExactNormalizedHartiganManifestContractMode : std::uint8_t {
  fixed_predeclared_contract,
  streaming_observed_open_contract,
};

enum class ExactNormalizedHartiganBatchDisposition : std::uint8_t {
  materialized_normalized_equal_level_batch,
  omitted_certified_qr1_noop,
};

enum class ExactNormalizedHartiganGroupSummaryEncoding : std::uint8_t {
  not_certified,
  legacy_single_group_scalar_qr_v3,
  complete_batch_qr_partition_v4,
};

struct ExactNormalizedHartiganLevelInput {
  std::size_t normalized_batch_index{};
  std::size_t order{};
  std::size_t order_batch_index{};
  exact::ExactLevel squared_level{};
  // Compatibility scalar for the explicitly selected v3 single-group
  // encoding only.  Complete v4 batch summaries must leave it at zero and
  // populate the four partition fields below.
  std::size_t q_r{};
  std::size_t core_facet_delta_count{};
  std::size_t point_delta_count{};
  std::size_t parent_delta_count{};
  std::size_t node_delta_count{};
  contract::CanonicalId previous_normalized_batch_chain_digest{};
  contract::CanonicalId normalized_batch_chain_digest{};
  ExactNormalizedHartiganBatchDisposition disposition{
      ExactNormalizedHartiganBatchDisposition::
          materialized_normalized_equal_level_batch};
  std::size_t group_count{};
  std::size_t qr0_group_count{};
  std::size_t qr1_group_count{};
  std::size_t qr_greater_equal2_group_count{};
  ExactNormalizedHartiganGroupSummaryEncoding group_summary_encoding{
      ExactNormalizedHartiganGroupSummaryEncoding::not_certified};

  friend bool operator==(
      const ExactNormalizedHartiganLevelInput&,
      const ExactNormalizedHartiganLevelInput&) = default;
};

struct ExactNormalizedHartiganLevelManifestRecord {
  std::uint32_t schema_version{
      normalized_exact_hartigan_level_manifest_schema_version};
  std::size_t record_index{};
  ExactNormalizedHartiganLevelInput input{};
  contract::CanonicalId previous_rational_chain_digest{};
  contract::CanonicalId rational_chain_digest{};
  bool exact_rational_reduced_and_nonnegative{false};
  bool binary64_serialization_used{false};
  bool source_batch_chain_contiguous{false};
  bool omitted_noop_rule_certified{false};
  bool complete_batch_qr_partition_certified{false};
  bool legacy_single_group_scalar_qr_compatibility_used{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool structurally_certified() const noexcept;
  [[nodiscard]] std::string canonical_json() const;

  friend bool operator==(
      const ExactNormalizedHartiganLevelManifestRecord&,
      const ExactNormalizedHartiganLevelManifestRecord&) = default;
};

struct ExactNormalizedHartiganOrderSummary {
  std::size_t order{};
  std::size_t rational_record_count{};
  std::size_t normalized_omitted_noop_qr1_batch_count{};
  std::size_t normalized_group_count{};
  std::size_t normalized_qr0_group_count{};
  std::size_t normalized_qr1_group_count{};
  std::size_t normalized_qr_greater_equal2_group_count{};
  std::size_t legacy_single_group_scalar_qr_record_count{};
  std::optional<exact::ExactLevel> first_level;
  std::optional<exact::ExactLevel> last_level;

  friend bool operator==(
      const ExactNormalizedHartiganOrderSummary&,
      const ExactNormalizedHartiganOrderSummary&) = default;
};

enum class ExactNormalizedHartiganLevelAppendDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_record_budget_exhausted,
  no_record_chronology_rejected,
  no_record_order_rejected,
  no_record_exact_level_rejected,
  no_record_source_chain_rejected,
  no_record_omitted_noop_rule_rejected,
  no_record_allocation_failed,
  complete_certified_record_appended,
  no_outstanding_prepared_append_budget_exhausted,
  no_prepared_append_already_consumed,
  no_foreign_prepared_append_rejected,
  no_stale_prepared_append_rejected,
  complete_certified_record_prepared,
  no_record_group_summary_rejected,
};

struct ExactNormalizedHartiganLevelAppendResult {
  std::optional<ExactNormalizedHartiganLevelManifestRecord> record;
  bool session_state_mutated{false};
  bool no_state_mutated_on_failure{false};
  ExactNormalizedHartiganLevelAppendDecision decision{
      ExactNormalizedHartiganLevelAppendDecision::not_certified};

  [[nodiscard]] bool certified_appended_record() const noexcept;
};

class ExactNormalizedHartiganLevelPreparedAppend {
 public:
  ExactNormalizedHartiganLevelPreparedAppend() noexcept;
  ~ExactNormalizedHartiganLevelPreparedAppend();
  ExactNormalizedHartiganLevelPreparedAppend(
      ExactNormalizedHartiganLevelPreparedAppend&&) noexcept;
  ExactNormalizedHartiganLevelPreparedAppend& operator=(
      ExactNormalizedHartiganLevelPreparedAppend&&) noexcept;
  ExactNormalizedHartiganLevelPreparedAppend(
      const ExactNormalizedHartiganLevelPreparedAppend&) = delete;
  ExactNormalizedHartiganLevelPreparedAppend& operator=(
      const ExactNormalizedHartiganLevelPreparedAppend&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactNormalizedHartiganLevelManifestRecord&
  prepared_record() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactNormalizedHartiganLevelPreparedAppend(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactNormalizedHartiganLevelManifestSession;
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

struct ExactNormalizedHartiganLevelPreparationResult {
  std::optional<ExactNormalizedHartiganLevelPreparedAppend> ticket;
  bool no_scientific_state_mutated{false};
  bool all_allocations_and_digests_completed{false};
  ExactNormalizedHartiganLevelAppendDecision decision{
      ExactNormalizedHartiganLevelAppendDecision::not_certified};

  [[nodiscard]] bool certified_prepared_append() const noexcept;
};

struct ExactNormalizedHartiganLevelManifestCheckpoint {
  std::uint32_t schema_version{
      normalized_exact_hartigan_level_manifest_checkpoint_schema_version};
  std::uint32_t manifest_schema_version{
      normalized_exact_hartigan_level_manifest_schema_version};
  ExactNormalizedHartiganManifestContractMode contract_mode{
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract};
  std::size_t maximum_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      expected_record_count_by_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      expected_omitted_noop_count_by_order{};
  contract::CanonicalId normalized_source_scientific_digest{};
  contract::CanonicalId normalized_batch_initial_chain_digest{};
  contract::CanonicalId normalized_batch_final_chain_digest{};
  std::array<
      ExactNormalizedHartiganOrderSummary,
      normalized_exact_hartigan_level_manifest_maximum_order>
      records_by_order{};
  std::optional<exact::ExactLevel> last_global_level;
  std::size_t last_global_order{};
  contract::CanonicalId current_source_chain_digest{};
  contract::CanonicalId current_rational_chain_digest{};
  std::size_t expected_total_record_count{};
  std::size_t record_count{};
  contract::CanonicalId checkpoint_digest{};
  bool process_local_authority_serialized{false};
  bool resource_budget_serialized{false};
  bool manifest_record_arena_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactNormalizedHartiganLevelManifestCheckpoint&,
      const ExactNormalizedHartiganLevelManifestCheckpoint&) = default;
};

enum class ExactNormalizedHartiganLevelCheckpointDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_outstanding_prepared_append,
  no_checkpoint_shape_rejected,
  no_checkpoint_digest_rejected,
  no_checkpoint_allocation_failed,
  complete_certified_semantic_checkpoint,
};

struct ExactNormalizedHartiganLevelCheckpointExportResult {
  std::optional<ExactNormalizedHartiganLevelManifestCheckpoint> checkpoint;
  ExactNormalizedHartiganLevelCheckpointDecision decision{
      ExactNormalizedHartiganLevelCheckpointDecision::not_certified};

  [[nodiscard]] bool certified_semantic_checkpoint() const noexcept;
};

enum class ExactNormalizedHartiganLevelSealDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_manifest_record_count_mismatch,
  no_manifest_source_chain_not_closed,
  complete_certified_manifest_seal,
  no_outstanding_prepared_append_rejected,
  no_streaming_closure_authority_required,
  no_streaming_closure_attestation_rejected,
};

struct ExactNormalizedHartiganLevelManifestSeal {
  std::uint32_t schema_version{
      normalized_exact_hartigan_level_manifest_schema_version};
  std::uint64_t authority_id{};
  std::size_t maximum_order{};
  std::size_t manifest_record_count{};
  ExactNormalizedHartiganManifestContractMode contract_mode{
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract};
  std::array<
      ExactNormalizedHartiganOrderSummary,
      normalized_exact_hartigan_level_manifest_maximum_order>
      records_by_order{};
  contract::CanonicalId normalized_batch_final_chain_digest{};
  contract::CanonicalId ordered_rational_chain_digest{};
  bool all_normalized_equal_level_batches_covered{false};
  bool one_record_per_batch_including_omitted_noop{false};
  bool binary64_level_count_zero{false};
  bool bounded_memory_summary_only{false};
  bool manifest_file_bytes_materialized{false};
  bool every_record_uses_complete_batch_qr_partition{false};
  bool legacy_single_group_scalar_qr_compatibility_used{false};
  bool streaming_counts_closed_from_move_only_exhaustion_attestation{false};
  bool public_status_claimed{false};
  ExactNormalizedHartiganLevelSealDecision decision{
      ExactNormalizedHartiganLevelSealDecision::not_certified};

  [[nodiscard]] bool certified_seal() const noexcept;
};

enum class ExactNormalizedHartiganLevelInitializationDecision
    : std::uint8_t {
  not_certified,
  no_session_contract_rejected,
  no_session_budget_rejected,
  complete_certified_empty_manifest_session,
  no_session_allocation_failed,
  no_checkpoint_shape_rejected,
  no_checkpoint_digest_rejected,
  complete_restored_certified_manifest_session,
  complete_certified_empty_streaming_manifest_session,
};

class ExactNormalizedHartiganStreamingClosureAttestation {
 public:
  ExactNormalizedHartiganStreamingClosureAttestation() noexcept;
  ~ExactNormalizedHartiganStreamingClosureAttestation();
  ExactNormalizedHartiganStreamingClosureAttestation(
      ExactNormalizedHartiganStreamingClosureAttestation&&) noexcept;
  ExactNormalizedHartiganStreamingClosureAttestation& operator=(
      ExactNormalizedHartiganStreamingClosureAttestation&&) noexcept;
  ExactNormalizedHartiganStreamingClosureAttestation(
      const ExactNormalizedHartiganStreamingClosureAttestation&) = delete;
  ExactNormalizedHartiganStreamingClosureAttestation& operator=(
      const ExactNormalizedHartiganStreamingClosureAttestation&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactNormalizedHartiganStreamingClosureAttestation(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactNormalizedHartiganLevelManifestSession;
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

class ExactNormalizedHartiganLevelManifestSession {
 public:
  ExactNormalizedHartiganLevelManifestSession() noexcept = default;
  ~ExactNormalizedHartiganLevelManifestSession() = default;
  ExactNormalizedHartiganLevelManifestSession(
      ExactNormalizedHartiganLevelManifestSession&&) noexcept = default;
  ExactNormalizedHartiganLevelManifestSession& operator=(
      ExactNormalizedHartiganLevelManifestSession&&) noexcept = default;
  ExactNormalizedHartiganLevelManifestSession(
      const ExactNormalizedHartiganLevelManifestSession&) = delete;
  ExactNormalizedHartiganLevelManifestSession& operator=(
      const ExactNormalizedHartiganLevelManifestSession&) = delete;

  [[nodiscard]] bool certified_ready() const noexcept;
  [[nodiscard]] bool sealed() const noexcept;
  [[nodiscard]] std::size_t record_count() const noexcept;
  [[nodiscard]] ExactNormalizedHartiganManifestContractMode contract_mode()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId&
  current_rational_chain_digest() const noexcept;

  [[nodiscard]] ExactNormalizedHartiganLevelPreparationResult prepare_append(
      const ExactNormalizedHartiganLevelInput& input);
  [[nodiscard]] ExactNormalizedHartiganLevelAppendResult commit(
      ExactNormalizedHartiganLevelPreparedAppend&& ticket) noexcept;
  [[nodiscard]] ExactNormalizedHartiganLevelAppendResult append(
      const ExactNormalizedHartiganLevelInput& input);
  [[nodiscard]] ExactNormalizedHartiganLevelCheckpointExportResult
  make_checkpoint() const noexcept;
  [[nodiscard]] ExactNormalizedHartiganLevelManifestSeal seal();

 private:
  ExactNormalizedHartiganLevelManifestContract contract_{};
  ExactNormalizedHartiganManifestContractMode contract_mode_{
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract};
  ExactNormalizedHartiganLevelManifestBudget budget_{};
  std::array<
      ExactNormalizedHartiganOrderSummary,
      normalized_exact_hartigan_level_manifest_maximum_order>
      summaries_{};
  std::optional<exact::ExactLevel> last_global_level_;
  std::size_t last_global_order_{};
  contract::CanonicalId current_source_chain_digest_{};
  contract::CanonicalId current_rational_chain_digest_{};
  std::size_t expected_total_record_count_{};
  std::size_t record_count_{};
  std::shared_ptr<std::size_t> live_prepared_append_count_;
  bool initialized_{false};
  bool sealed_{false};

  [[nodiscard]] ExactNormalizedHartiganLevelAppendResult
  commit_prepared_append(
      ExactNormalizedHartiganLevelPreparedAppend&& ticket) noexcept;
  // Private streaming seam for the unique future product owner.  Genesis
  // binds no future counts or final chain, so it requires no pre-replay.  The
  // friend may mint the move-only closure attestation only after its genuine
  // resident source is exhausted and the supplied coverage equals every
  // observed manifest counter.  No public boolean can close this mode.
  [[nodiscard]] static ExactNormalizedHartiganLevelInitializationResult
  initialize_streaming_manifest_session(
      std::uint64_t authority_id,
      std::size_t maximum_order,
      const contract::CanonicalId& normalized_source_scientific_digest,
      const contract::CanonicalId&
          normalized_batch_initial_chain_digest,
      const ExactNormalizedHartiganLevelManifestBudget& budget);
  [[nodiscard]] std::optional<
      ExactNormalizedHartiganStreamingClosureAttestation>
  prepare_streaming_closure_from_exhausted_product_source(
      const contract::CanonicalId& covered_source_scientific_digest,
      std::size_t covered_batch_count,
      const std::array<
          std::size_t,
          normalized_exact_hartigan_level_manifest_maximum_order>&
          covered_record_count_by_order,
      const std::array<
          std::size_t,
          normalized_exact_hartigan_level_manifest_maximum_order>&
          covered_omitted_noop_count_by_order,
      const contract::CanonicalId& expected_final_chain_digest);
  [[nodiscard]] ExactNormalizedHartiganLevelManifestSeal seal_streaming(
      ExactNormalizedHartiganStreamingClosureAttestation&& attestation);

  friend struct ExactNormalizedHartiganLevelInitializationResult;
  friend ExactNormalizedHartiganLevelInitializationResult
  initialize_exact_normalized_hartigan_level_manifest_session(
      const ExactNormalizedHartiganLevelManifestContract&,
      const ExactNormalizedHartiganLevelManifestBudget&);
  friend ExactNormalizedHartiganLevelInitializationResult
  restore_exact_normalized_hartigan_level_manifest_session(
      ExactNormalizedHartiganLevelManifestCheckpoint&&,
      std::uint64_t,
      const ExactNormalizedHartiganLevelManifestBudget&);
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

struct ExactNormalizedHartiganLevelInitializationResult {
  std::optional<ExactNormalizedHartiganLevelManifestSession> session;
  std::size_t required_record_count{};
  bool contract_preflight_certified{false};
  bool budget_preflight_certified{false};
  bool no_manifest_record_arena_materialized{false};
  bool checkpoint_freshly_verified{false};
  ExactNormalizedHartiganLevelInitializationDecision decision{
      ExactNormalizedHartiganLevelInitializationDecision::not_certified};

  [[nodiscard]] bool certified_initialized_session() const noexcept;
};

[[nodiscard]] ExactNormalizedHartiganLevelInitializationResult
initialize_exact_normalized_hartigan_level_manifest_session(
    const ExactNormalizedHartiganLevelManifestContract& contract,
    const ExactNormalizedHartiganLevelManifestBudget& budget);

[[nodiscard]] ExactNormalizedHartiganLevelInitializationResult
restore_exact_normalized_hartigan_level_manifest_session(
    ExactNormalizedHartiganLevelManifestCheckpoint&& checkpoint,
    std::uint64_t reminted_authority_id,
    const ExactNormalizedHartiganLevelManifestBudget& budget);

[[nodiscard]] bool verify_exact_normalized_hartigan_level_record(
    const ExactNormalizedHartiganLevelManifestRecord& record);

}  // namespace morsehgp3d::hierarchy
