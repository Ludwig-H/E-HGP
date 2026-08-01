#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace morsehgp3d::hierarchy {

struct ExactNormalizedHartiganLevelInitializationResult;

inline constexpr std::uint32_t
    normalized_exact_hartigan_level_manifest_schema_version = 3U;
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
        "certified_bounded_memory_normalized_exact_rational_record_chain";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_public_status = "not_claimed";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_record_schema =
        "morsehgp3d.phase15.normalized_exact_hartigan_level_manifest_record.v3";
inline constexpr std::string_view
    normalized_exact_hartigan_level_manifest_representation =
        "canonical_reduced_rational_decimal_v1";

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

enum class ExactNormalizedHartiganBatchDisposition : std::uint8_t {
  materialized_normalized_equal_level_batch,
  omitted_certified_qr1_noop,
};

struct ExactNormalizedHartiganLevelInput {
  std::size_t normalized_batch_index{};
  std::size_t order{};
  std::size_t order_batch_index{};
  exact::ExactLevel squared_level{};
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
};

struct ExactNormalizedHartiganLevelAppendResult {
  std::optional<ExactNormalizedHartiganLevelManifestRecord> record;
  bool session_state_mutated{false};
  bool no_state_mutated_on_failure{false};
  ExactNormalizedHartiganLevelAppendDecision decision{
      ExactNormalizedHartiganLevelAppendDecision::not_certified};

  [[nodiscard]] bool certified_appended_record() const noexcept;
};

enum class ExactNormalizedHartiganLevelSealDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_manifest_record_count_mismatch,
  no_manifest_source_chain_not_closed,
  complete_certified_manifest_seal,
};

struct ExactNormalizedHartiganLevelManifestSeal {
  std::uint32_t schema_version{
      normalized_exact_hartigan_level_manifest_schema_version};
  std::uint64_t authority_id{};
  std::size_t maximum_order{};
  std::size_t manifest_record_count{};
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
  [[nodiscard]] const contract::CanonicalId&
  current_rational_chain_digest() const noexcept;

  [[nodiscard]] ExactNormalizedHartiganLevelAppendResult append(
      const ExactNormalizedHartiganLevelInput& input);
  [[nodiscard]] ExactNormalizedHartiganLevelManifestSeal seal();

 private:
  ExactNormalizedHartiganLevelManifestContract contract_{};
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
  bool initialized_{false};
  bool sealed_{false};

  friend struct ExactNormalizedHartiganLevelInitializationResult;
  friend ExactNormalizedHartiganLevelInitializationResult
  initialize_exact_normalized_hartigan_level_manifest_session(
      const ExactNormalizedHartiganLevelManifestContract&,
      const ExactNormalizedHartiganLevelManifestBudget&);
};

struct ExactNormalizedHartiganLevelInitializationResult {
  std::optional<ExactNormalizedHartiganLevelManifestSession> session;
  std::size_t required_record_count{};
  bool contract_preflight_certified{false};
  bool budget_preflight_certified{false};
  bool no_manifest_record_arena_materialized{false};
  ExactNormalizedHartiganLevelInitializationDecision decision{
      ExactNormalizedHartiganLevelInitializationDecision::not_certified};

  [[nodiscard]] bool certified_initialized_session() const noexcept;
};

[[nodiscard]] ExactNormalizedHartiganLevelInitializationResult
initialize_exact_normalized_hartigan_level_manifest_session(
    const ExactNormalizedHartiganLevelManifestContract& contract,
    const ExactNormalizedHartiganLevelManifestBudget& budget);

[[nodiscard]] bool verify_exact_normalized_hartigan_level_record(
    const ExactNormalizedHartiganLevelManifestRecord& record);

}  // namespace morsehgp3d::hierarchy
