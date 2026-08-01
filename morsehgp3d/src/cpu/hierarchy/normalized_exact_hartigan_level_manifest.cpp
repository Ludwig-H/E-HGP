#include "morsehgp3d/hierarchy/normalized_exact_hartigan_level_manifest.hpp"

#include <boost/multiprecision/integer.hpp>

#include <array>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-initial/v2";
constexpr std::string_view streaming_initial_chain_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-streaming-initial/v1";
constexpr std::string_view record_chain_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-record/v2";
constexpr std::string_view checkpoint_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-checkpoint/v1";
constexpr std::size_t maximum_live_prepared_append_count = 1U;

[[nodiscard]] bool id_is_zero(const contract::CanonicalId& value) noexcept {
  return value == contract::CanonicalId{};
}

[[nodiscard]] bool fits_u64(std::size_t value) noexcept {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    return value <= std::numeric_limits<std::uint64_t>::max();
  }
  return true;
}

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

void append_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  append_u64(builder, value ? 1U : 0U);
}

[[nodiscard]] std::size_t conservative_decimal_digit_bound(
    const exact::BigInt& nonnegative) noexcept {
  if (nonnegative == 0) {
    return 1U;
  }
  const std::size_t bit_count =
      static_cast<std::size_t>(boost::multiprecision::msb(nonnegative)) + 1U;
  // log10(2) < 30103 / 100000.  This upper bound may reject an integer on a
  // single boundary digit, but never allocates a decimal payload above cap.
  constexpr std::size_t numerator = 30103U;
  constexpr std::size_t denominator = 100000U;
  const std::size_t adjusted = bit_count - 1U;
  if (adjusted >
      (std::numeric_limits<std::size_t>::max() - (denominator - 1U)) /
          numerator) {
    return std::numeric_limits<std::size_t>::max();
  }
  return (adjusted * numerator + denominator - 1U) / denominator + 1U;
}

[[nodiscard]] bool level_fits_decimal_budget(
    const exact::ExactLevel& level,
    std::size_t maximum_digits) noexcept {
  return maximum_digits != 0U &&
         conservative_decimal_digit_bound(level.numerator()) <=
             maximum_digits &&
         conservative_decimal_digit_bound(level.denominator()) <=
             maximum_digits;
}

struct NormalizedGroupSummary {
  std::size_t group_count{};
  std::size_t qr0_group_count{};
  std::size_t qr1_group_count{};
  std::size_t qr_greater_equal2_group_count{};
  bool complete_batch_partition{false};
  bool legacy_single_group_scalar{false};
};

[[nodiscard]] std::optional<NormalizedGroupSummary>
normalized_group_summary(
    const ExactNormalizedHartiganLevelInput& input) noexcept {
  if (input.group_summary_encoding ==
      ExactNormalizedHartiganGroupSummaryEncoding::
          legacy_single_group_scalar_qr_v3) {
    if (input.q_r == 0U || input.group_count != 0U ||
        input.qr0_group_count != 0U || input.qr1_group_count != 0U ||
        input.qr_greater_equal2_group_count != 0U) {
      return std::nullopt;
    }
    NormalizedGroupSummary output;
    output.group_count = 1U;
    output.qr1_group_count = input.q_r == 1U ? 1U : 0U;
    output.qr_greater_equal2_group_count = input.q_r >= 2U ? 1U : 0U;
    output.legacy_single_group_scalar = true;
    return output;
  }
  if (input.group_summary_encoding !=
          ExactNormalizedHartiganGroupSummaryEncoding::
              complete_batch_qr_partition_v4 ||
      input.q_r != 0U || input.group_count == 0U ||
      !fits_u64(input.group_count) || !fits_u64(input.qr0_group_count) ||
      !fits_u64(input.qr1_group_count) ||
      !fits_u64(input.qr_greater_equal2_group_count)) {
    return std::nullopt;
  }
  std::size_t partition_total = 0U;
  if (!checked_add(partition_total, input.qr0_group_count, partition_total) ||
      !checked_add(partition_total, input.qr1_group_count, partition_total) ||
      !checked_add(
          partition_total,
          input.qr_greater_equal2_group_count,
          partition_total) ||
      partition_total != input.group_count) {
    return std::nullopt;
  }
  return NormalizedGroupSummary{
      input.group_count,
      input.qr0_group_count,
      input.qr1_group_count,
      input.qr_greater_equal2_group_count,
      true,
      false};
}

[[nodiscard]] bool batch_group_summary_rule(
    const ExactNormalizedHartiganLevelInput& input) noexcept {
  const auto summary = normalized_group_summary(input);
  if (!summary.has_value()) {
    return false;
  }
  if (input.disposition ==
      ExactNormalizedHartiganBatchDisposition::
          materialized_normalized_equal_level_batch) {
    return true;
  }
  return input.disposition ==
             ExactNormalizedHartiganBatchDisposition::
                 omitted_certified_qr1_noop &&
         summary->group_count == 1U && summary->qr0_group_count == 0U &&
         summary->qr1_group_count == 1U &&
         summary->qr_greater_equal2_group_count == 0U &&
         input.core_facet_delta_count == 0U &&
         input.point_delta_count == 0U && input.parent_delta_count == 0U &&
         input.node_delta_count == 0U;
}

[[nodiscard]] contract::CanonicalId initial_chain_digest(
    const ExactNormalizedHartiganLevelManifestContract& manifest_contract) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_chain_domain);
  append_u32(builder, normalized_exact_hartigan_level_manifest_schema_version);
  append_size(builder, manifest_contract.maximum_order);
  for (std::size_t index = 0U;
       index < normalized_exact_hartigan_level_manifest_maximum_order;
       ++index) {
    append_size(
        builder, manifest_contract.expected_record_count_by_order[index]);
    append_size(
        builder,
        manifest_contract.expected_omitted_noop_count_by_order[index]);
  }
  append_id(builder, manifest_contract.normalized_source_scientific_digest);
  append_id(builder, manifest_contract.normalized_batch_initial_chain_digest);
  append_id(builder, manifest_contract.normalized_batch_final_chain_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId streaming_initial_chain_digest(
    std::size_t maximum_order,
    const contract::CanonicalId& normalized_source_scientific_digest,
    const contract::CanonicalId& normalized_batch_initial_chain_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(streaming_initial_chain_domain);
  append_u32(builder, normalized_exact_hartigan_level_manifest_schema_version);
  append_size(builder, maximum_order);
  append_id(builder, normalized_source_scientific_digest);
  append_id(builder, normalized_batch_initial_chain_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId record_chain_digest(
    const ExactNormalizedHartiganLevelManifestRecord& record) {
  contract::CanonicalSha256Builder builder;
  builder.update(record_chain_domain);
  append_u32(builder, record.schema_version);
  append_id(builder, record.previous_rational_chain_digest);
  append_size(builder, record.record_index);
  append_size(builder, record.input.normalized_batch_index);
  append_size(builder, record.input.order);
  append_size(builder, record.input.order_batch_index);
  append_text(builder, record.input.squared_level.numerator_string());
  append_text(builder, record.input.squared_level.denominator_string());
  append_size(builder, record.input.q_r);
  append_size(builder, record.input.core_facet_delta_count);
  append_size(builder, record.input.point_delta_count);
  append_size(builder, record.input.parent_delta_count);
  append_size(builder, record.input.node_delta_count);
  append_id(builder, record.input.previous_normalized_batch_chain_digest);
  append_id(builder, record.input.normalized_batch_chain_digest);
  append_u64(builder, static_cast<std::uint8_t>(record.input.disposition));
  append_size(builder, record.input.group_count);
  append_size(builder, record.input.qr0_group_count);
  append_size(builder, record.input.qr1_group_count);
  append_size(builder, record.input.qr_greater_equal2_group_count);
  append_u64(
      builder,
      static_cast<std::uint8_t>(record.input.group_summary_encoding));
  return builder.finalize();
}

void append_optional_level(
    contract::CanonicalSha256Builder& builder,
    const std::optional<exact::ExactLevel>& level) {
  append_bool(builder, level.has_value());
  if (!level.has_value()) {
    return;
  }
  append_text(builder, level->numerator_string());
  append_text(builder, level->denominator_string());
}

[[nodiscard]] ExactNormalizedHartiganLevelManifestContract
checkpoint_contract(
    const ExactNormalizedHartiganLevelManifestCheckpoint& checkpoint,
    std::uint64_t authority_id) {
  ExactNormalizedHartiganLevelManifestContract output;
  output.authority_id = authority_id;
  output.maximum_order = checkpoint.maximum_order;
  output.expected_record_count_by_order =
      checkpoint.expected_record_count_by_order;
  output.expected_omitted_noop_count_by_order =
      checkpoint.expected_omitted_noop_count_by_order;
  output.normalized_source_scientific_digest =
      checkpoint.normalized_source_scientific_digest;
  output.normalized_batch_initial_chain_digest =
      checkpoint.normalized_batch_initial_chain_digest;
  output.normalized_batch_final_chain_digest =
      checkpoint.normalized_batch_final_chain_digest;
  return output;
}

[[nodiscard]] contract::CanonicalId semantic_checkpoint_digest(
    const ExactNormalizedHartiganLevelManifestCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_domain);
  append_u32(builder, checkpoint.schema_version);
  append_u32(builder, checkpoint.manifest_schema_version);
  append_u64(builder, static_cast<std::uint8_t>(checkpoint.contract_mode));
  append_size(builder, checkpoint.maximum_order);
  for (std::size_t index = 0U;
       index < normalized_exact_hartigan_level_manifest_maximum_order;
       ++index) {
    append_size(builder, checkpoint.expected_record_count_by_order[index]);
    append_size(
        builder,
        checkpoint.expected_omitted_noop_count_by_order[index]);
  }
  append_id(builder, checkpoint.normalized_source_scientific_digest);
  append_id(builder, checkpoint.normalized_batch_initial_chain_digest);
  append_id(builder, checkpoint.normalized_batch_final_chain_digest);
  for (const auto& summary : checkpoint.records_by_order) {
    append_size(builder, summary.order);
    append_size(builder, summary.rational_record_count);
    append_size(
        builder, summary.normalized_omitted_noop_qr1_batch_count);
    append_size(builder, summary.normalized_group_count);
    append_size(builder, summary.normalized_qr0_group_count);
    append_size(builder, summary.normalized_qr1_group_count);
    append_size(
        builder, summary.normalized_qr_greater_equal2_group_count);
    append_size(
        builder, summary.legacy_single_group_scalar_qr_record_count);
    append_optional_level(builder, summary.first_level);
    append_optional_level(builder, summary.last_level);
  }
  append_optional_level(builder, checkpoint.last_global_level);
  append_size(builder, checkpoint.last_global_order);
  append_id(builder, checkpoint.current_source_chain_digest);
  append_id(builder, checkpoint.current_rational_chain_digest);
  append_size(builder, checkpoint.expected_total_record_count);
  append_size(builder, checkpoint.record_count);
  append_bool(builder, checkpoint.process_local_authority_serialized);
  append_bool(builder, checkpoint.resource_budget_serialized);
  append_bool(builder, checkpoint.manifest_record_arena_materialized);
  append_bool(builder, checkpoint.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] bool checkpoint_shape_certified(
    const ExactNormalizedHartiganLevelManifestCheckpoint& checkpoint,
    const ExactNormalizedHartiganLevelManifestBudget& budget) {
  const bool fixed_contract =
      checkpoint.contract_mode ==
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract;
  const bool streaming_contract =
      checkpoint.contract_mode ==
      ExactNormalizedHartiganManifestContractMode::
          streaming_observed_open_contract;
  if (checkpoint.schema_version !=
          normalized_exact_hartigan_level_manifest_checkpoint_schema_version ||
      checkpoint.manifest_schema_version !=
          normalized_exact_hartigan_level_manifest_schema_version ||
      checkpoint.maximum_order == 0U ||
      checkpoint.maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      budget.maximum_decimal_digits_per_integer == 0U ||
      budget.maximum_decimal_digits_per_integer >
          normalized_exact_hartigan_level_manifest_maximum_decimal_digits ||
      id_is_zero(checkpoint.normalized_source_scientific_digest) ||
      id_is_zero(checkpoint.normalized_batch_initial_chain_digest) ||
      (!fixed_contract && !streaming_contract) ||
      (fixed_contract &&
       id_is_zero(checkpoint.normalized_batch_final_chain_digest)) ||
      (streaming_contract &&
       !id_is_zero(checkpoint.normalized_batch_final_chain_digest)) ||
      id_is_zero(checkpoint.current_source_chain_digest) ||
      id_is_zero(checkpoint.current_rational_chain_digest) ||
      id_is_zero(checkpoint.checkpoint_digest) ||
      checkpoint.process_local_authority_serialized ||
      checkpoint.resource_budget_serialized ||
      checkpoint.manifest_record_arena_materialized ||
      checkpoint.public_status_claimed ||
      (fixed_contract &&
       checkpoint.record_count > checkpoint.expected_total_record_count) ||
      (fixed_contract &&
       checkpoint.expected_total_record_count > budget.maximum_record_count) ||
      (streaming_contract &&
       checkpoint.record_count > budget.maximum_record_count) ||
      !fits_u64(checkpoint.expected_total_record_count) ||
      !fits_u64(checkpoint.record_count)) {
    return false;
  }

  std::size_t expected_total = 0U;
  std::size_t observed_total = 0U;
  for (std::size_t index = 0U;
       index < normalized_exact_hartigan_level_manifest_maximum_order;
       ++index) {
    const std::size_t expected =
        checkpoint.expected_record_count_by_order[index];
    const std::size_t expected_omitted =
        checkpoint.expected_omitted_noop_count_by_order[index];
    const auto& summary = checkpoint.records_by_order[index];
    std::size_t group_partition_total = 0U;
    const bool group_partition_certified =
        checked_add(
            group_partition_total,
            summary.normalized_qr0_group_count,
            group_partition_total) &&
        checked_add(
            group_partition_total,
            summary.normalized_qr1_group_count,
            group_partition_total) &&
        checked_add(
            group_partition_total,
            summary.normalized_qr_greater_equal2_group_count,
            group_partition_total) &&
        group_partition_total == summary.normalized_group_count;
    if ((fixed_contract && expected_omitted > expected) ||
        (streaming_contract && (expected != 0U || expected_omitted != 0U)) ||
        (index >= checkpoint.maximum_order &&
         (expected != 0U || expected_omitted != 0U)) ||
        !fits_u64(expected) || !fits_u64(expected_omitted) ||
        !checked_add(expected_total, expected, expected_total) ||
        summary.order != index + 1U ||
        (fixed_contract && summary.rational_record_count > expected) ||
        (fixed_contract &&
         summary.normalized_omitted_noop_qr1_batch_count > expected_omitted) ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.rational_record_count ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.normalized_qr1_group_count ||
        summary.legacy_single_group_scalar_qr_record_count >
            summary.rational_record_count ||
        (streaming_contract &&
         summary.legacy_single_group_scalar_qr_record_count != 0U) ||
        (summary.rational_record_count == 0U) !=
            (summary.normalized_group_count == 0U) ||
        summary.normalized_group_count < summary.rational_record_count ||
        !group_partition_certified ||
        !fits_u64(summary.rational_record_count) ||
        !fits_u64(summary.normalized_omitted_noop_qr1_batch_count) ||
        !fits_u64(summary.normalized_group_count) ||
        !fits_u64(summary.normalized_qr0_group_count) ||
        !fits_u64(summary.normalized_qr1_group_count) ||
        !fits_u64(summary.normalized_qr_greater_equal2_group_count) ||
        !fits_u64(summary.legacy_single_group_scalar_qr_record_count) ||
        !checked_add(
            observed_total,
            summary.rational_record_count,
            observed_total) ||
        (summary.rational_record_count == 0U) !=
            (!summary.first_level.has_value() &&
             !summary.last_level.has_value())) {
      return false;
    }
    if (summary.rational_record_count != 0U &&
        (!summary.first_level.has_value() ||
         !summary.last_level.has_value() ||
         !level_fits_decimal_budget(
             *summary.first_level,
             budget.maximum_decimal_digits_per_integer) ||
         !level_fits_decimal_budget(
             *summary.last_level,
             budget.maximum_decimal_digits_per_integer) ||
         *summary.last_level < *summary.first_level)) {
      return false;
    }
  }
  if (observed_total != checkpoint.record_count ||
      (fixed_contract &&
       (expected_total != checkpoint.expected_total_record_count ||
        (expected_total == 0U) !=
            (checkpoint.normalized_batch_initial_chain_digest ==
             checkpoint.normalized_batch_final_chain_digest))) ||
      (streaming_contract &&
       (expected_total != 0U ||
        checkpoint.expected_total_record_count != 0U))) {
    return false;
  }

  if (checkpoint.record_count == 0U) {
    const contract::CanonicalId expected_initial_rational_chain =
        fixed_contract
        ? initial_chain_digest(checkpoint_contract(checkpoint, 1U))
        : streaming_initial_chain_digest(
              checkpoint.maximum_order,
              checkpoint.normalized_source_scientific_digest,
              checkpoint.normalized_batch_initial_chain_digest);
    return !checkpoint.last_global_level.has_value() &&
           checkpoint.last_global_order == 0U &&
           checkpoint.current_source_chain_digest ==
               checkpoint.normalized_batch_initial_chain_digest &&
           checkpoint.current_rational_chain_digest ==
               expected_initial_rational_chain;
  }
  if (!checkpoint.last_global_level.has_value() ||
      checkpoint.last_global_order == 0U ||
      checkpoint.last_global_order > checkpoint.maximum_order ||
      !level_fits_decimal_budget(
          *checkpoint.last_global_level,
          budget.maximum_decimal_digits_per_integer)) {
    return false;
  }
  const auto& last_order_summary =
      checkpoint.records_by_order[checkpoint.last_global_order - 1U];
  if (!last_order_summary.last_level.has_value() ||
      *last_order_summary.last_level != *checkpoint.last_global_level) {
    return false;
  }
  for (std::size_t index = 0U;
       index < checkpoint.records_by_order.size();
       ++index) {
    const auto& summary = checkpoint.records_by_order[index];
    if (!summary.last_level.has_value()) {
      continue;
    }
    if (*checkpoint.last_global_level < *summary.last_level ||
        (*checkpoint.last_global_level == *summary.last_level &&
         index + 1U > checkpoint.last_global_order)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactNormalizedHartiganLevelAppendResult rejected_append(
    ExactNormalizedHartiganLevelAppendDecision decision) noexcept {
  ExactNormalizedHartiganLevelAppendResult rejected{};
  rejected.session_state_mutated = false;
  rejected.no_state_mutated_on_failure = true;
  rejected.decision = decision;
  return rejected;
}

[[nodiscard]] const char* disposition_text(
    ExactNormalizedHartiganBatchDisposition disposition) noexcept {
  switch (disposition) {
    case ExactNormalizedHartiganBatchDisposition::
        materialized_normalized_equal_level_batch:
      return "materialized_normalized_equal_level_batch";
    case ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop:
      return "omitted_certified_qr1_noop";
  }
  return "invalid";
}

[[nodiscard]] const char* group_summary_encoding_text(
    ExactNormalizedHartiganGroupSummaryEncoding encoding) noexcept {
  switch (encoding) {
    case ExactNormalizedHartiganGroupSummaryEncoding::not_certified:
      return "not_certified";
    case ExactNormalizedHartiganGroupSummaryEncoding::
        legacy_single_group_scalar_qr_v3:
      return "legacy_single_group_scalar_qr_v3";
    case ExactNormalizedHartiganGroupSummaryEncoding::
        complete_batch_qr_partition_v4:
      return "complete_batch_qr_partition_v4";
  }
  return "invalid";
}

}  // namespace

struct ExactNormalizedHartiganLevelPreparedAppend::Impl {
  ~Impl() noexcept {
    if (!owns_live_slot) {
      return;
    }
    if (live_prepared_append_count == nullptr ||
        *live_prepared_append_count == 0U) {
      std::terminate();
    }
    --*live_prepared_append_count;
  }

  std::shared_ptr<std::size_t> live_prepared_append_count;
  ExactNormalizedHartiganLevelManifestRecord record{};
  std::array<
      ExactNormalizedHartiganOrderSummary,
      normalized_exact_hartigan_level_manifest_maximum_order>
      next_summaries{};
  std::optional<exact::ExactLevel> next_last_global_level;
  std::size_t next_last_global_order{};
  contract::CanonicalId source_source_chain_digest{};
  contract::CanonicalId source_rational_chain_digest{};
  contract::CanonicalId next_source_chain_digest{};
  contract::CanonicalId next_rational_chain_digest{};
  std::size_t source_record_count{};
  std::size_t next_record_count{};
  bool owns_live_slot{false};
  bool consumed{false};
};

struct ExactNormalizedHartiganStreamingClosureAttestation::Impl {
  ~Impl() noexcept {
    if (!owns_live_slot) {
      return;
    }
    if (live_scientific_transaction_count == nullptr ||
        *live_scientific_transaction_count == 0U) {
      std::terminate();
    }
    --*live_scientific_transaction_count;
  }

  std::shared_ptr<std::size_t> live_scientific_transaction_count;
  contract::CanonicalId covered_source_scientific_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId rational_chain_digest{};
  contract::CanonicalId expected_final_chain_digest{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      covered_record_count_by_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      covered_omitted_noop_count_by_order{};
  std::size_t covered_batch_count{};
  std::size_t source_record_count{};
  bool owns_live_slot{false};
  bool consumed{false};
};

bool ExactNormalizedHartiganLevelManifestRecord::structurally_certified()
    const noexcept {
  const auto group_summary = normalized_group_summary(input);
  return group_summary.has_value() &&
         schema_version ==
             normalized_exact_hartigan_level_manifest_schema_version &&
         record_index == input.normalized_batch_index && input.order != 0U &&
         input.order <=
             normalized_exact_hartigan_level_manifest_maximum_order &&
         exact_rational_reduced_and_nonnegative &&
         !binary64_serialization_used && source_batch_chain_contiguous &&
         omitted_noop_rule_certified && batch_group_summary_rule(input) &&
         complete_batch_qr_partition_certified ==
             group_summary->complete_batch_partition &&
         legacy_single_group_scalar_qr_compatibility_used ==
             group_summary->legacy_single_group_scalar &&
         (complete_batch_qr_partition_certified !=
          legacy_single_group_scalar_qr_compatibility_used) &&
         !public_status_claimed &&
         !id_is_zero(previous_rational_chain_digest) &&
         !id_is_zero(rational_chain_digest) &&
         !id_is_zero(input.previous_normalized_batch_chain_digest) &&
         !id_is_zero(input.normalized_batch_chain_digest) &&
         input.previous_normalized_batch_chain_digest !=
             input.normalized_batch_chain_digest;
}

std::string ExactNormalizedHartiganLevelManifestRecord::canonical_json()
    const {
  return std::string{"{\"binary64_serialization_used\":"} +
      (binary64_serialization_used ? "true" : "false") +
      ",\"complete_batch_qr_partition_certified\":" +
      (complete_batch_qr_partition_certified ? "true" : "false") +
      ",\"core_facet_delta_count\":" +
      std::to_string(input.core_facet_delta_count) +
      ",\"denominator\":\"" + input.squared_level.denominator_string() +
      "\",\"disposition\":\"" + disposition_text(input.disposition) +
      "\",\"group_count\":" + std::to_string(input.group_count) +
      ",\"group_summary_encoding\":\"" +
      group_summary_encoding_text(input.group_summary_encoding) +
      "\",\"legacy_single_group_scalar_qr_compatibility_used\":" +
      (legacy_single_group_scalar_qr_compatibility_used ? "true" : "false") +
      ",\"node_delta_count\":" + std::to_string(input.node_delta_count) +
      ",\"normalized_batch_chain_digest\":\"" +
      input.normalized_batch_chain_digest.to_lower_hex() +
      "\",\"normalized_batch_index\":" +
      std::to_string(input.normalized_batch_index) +
      ",\"numerator\":\"" + input.squared_level.numerator_string() +
      "\",\"order\":" + std::to_string(input.order) +
      ",\"order_batch_index\":" + std::to_string(input.order_batch_index) +
      ",\"parent_delta_count\":" + std::to_string(input.parent_delta_count) +
      ",\"point_delta_count\":" + std::to_string(input.point_delta_count) +
      ",\"previous_normalized_batch_chain_digest\":\"" +
      input.previous_normalized_batch_chain_digest.to_lower_hex() +
      "\",\"previous_rational_chain_digest\":\"" +
      previous_rational_chain_digest.to_lower_hex() +
      "\",\"q_r\":" + std::to_string(input.q_r) +
      ",\"qr0_group_count\":" + std::to_string(input.qr0_group_count) +
      ",\"qr1_group_count\":" + std::to_string(input.qr1_group_count) +
      ",\"qr_greater_equal2_group_count\":" +
      std::to_string(input.qr_greater_equal2_group_count) +
      ",\"rational_chain_digest\":\"" +
      rational_chain_digest.to_lower_hex() +
      "\",\"record_index\":" + std::to_string(record_index) +
      ",\"schema\":\"" +
      std::string{normalized_exact_hartigan_level_manifest_record_schema} +
      "\"}";
}

bool ExactNormalizedHartiganLevelAppendResult::certified_appended_record()
    const noexcept {
  return record.has_value() && record->structurally_certified() &&
         session_state_mutated && !no_state_mutated_on_failure &&
         decision == ExactNormalizedHartiganLevelAppendDecision::
                         complete_certified_record_appended;
}

ExactNormalizedHartiganLevelPreparedAppend::
    ExactNormalizedHartiganLevelPreparedAppend() noexcept = default;
ExactNormalizedHartiganLevelPreparedAppend::
    ~ExactNormalizedHartiganLevelPreparedAppend() = default;
ExactNormalizedHartiganLevelPreparedAppend::
    ExactNormalizedHartiganLevelPreparedAppend(
        ExactNormalizedHartiganLevelPreparedAppend&&) noexcept = default;
ExactNormalizedHartiganLevelPreparedAppend&
ExactNormalizedHartiganLevelPreparedAppend::operator=(
    ExactNormalizedHartiganLevelPreparedAppend&&) noexcept = default;
ExactNormalizedHartiganLevelPreparedAppend::
    ExactNormalizedHartiganLevelPreparedAppend(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactNormalizedHartiganLevelPreparedAppend::valid() const noexcept {
  return impl_ != nullptr && !impl_->consumed && impl_->owns_live_slot &&
         impl_->live_prepared_append_count != nullptr &&
         *impl_->live_prepared_append_count == 1U &&
         impl_->source_record_count + 1U == impl_->next_record_count &&
         impl_->record.record_index == impl_->source_record_count &&
         impl_->record.rational_chain_digest ==
             impl_->next_rational_chain_digest &&
         impl_->record.input.normalized_batch_chain_digest ==
             impl_->next_source_chain_digest &&
         impl_->record.structurally_certified();
}

bool ExactNormalizedHartiganLevelPreparedAppend::consumed() const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

const ExactNormalizedHartiganLevelManifestRecord&
ExactNormalizedHartiganLevelPreparedAppend::prepared_record()
    const noexcept {
  static const ExactNormalizedHartiganLevelManifestRecord empty{};
  return impl_ == nullptr ? empty : impl_->record;
}

ExactNormalizedHartiganStreamingClosureAttestation::
    ExactNormalizedHartiganStreamingClosureAttestation() noexcept = default;
ExactNormalizedHartiganStreamingClosureAttestation::
    ~ExactNormalizedHartiganStreamingClosureAttestation() = default;
ExactNormalizedHartiganStreamingClosureAttestation::
    ExactNormalizedHartiganStreamingClosureAttestation(
        ExactNormalizedHartiganStreamingClosureAttestation&&) noexcept =
    default;
ExactNormalizedHartiganStreamingClosureAttestation&
ExactNormalizedHartiganStreamingClosureAttestation::operator=(
    ExactNormalizedHartiganStreamingClosureAttestation&&) noexcept = default;
ExactNormalizedHartiganStreamingClosureAttestation::
    ExactNormalizedHartiganStreamingClosureAttestation(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactNormalizedHartiganStreamingClosureAttestation::valid()
    const noexcept {
  if (impl_ == nullptr || impl_->consumed || !impl_->owns_live_slot ||
      impl_->live_scientific_transaction_count == nullptr ||
      *impl_->live_scientific_transaction_count != 1U ||
      id_is_zero(impl_->covered_source_scientific_digest) ||
      id_is_zero(impl_->source_chain_digest) ||
      id_is_zero(impl_->rational_chain_digest) ||
      impl_->expected_final_chain_digest != impl_->source_chain_digest ||
      impl_->covered_batch_count != impl_->source_record_count) {
    return false;
  }
  std::size_t covered_total = 0U;
  for (std::size_t index = 0U;
       index < impl_->covered_record_count_by_order.size();
       ++index) {
    if (impl_->covered_omitted_noop_count_by_order[index] >
            impl_->covered_record_count_by_order[index] ||
        !checked_add(
            covered_total,
            impl_->covered_record_count_by_order[index],
            covered_total)) {
      return false;
    }
  }
  return covered_total == impl_->covered_batch_count;
}

bool ExactNormalizedHartiganStreamingClosureAttestation::consumed()
    const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

bool ExactNormalizedHartiganLevelPreparationResult::
    certified_prepared_append() const noexcept {
  return ticket.has_value() && ticket->valid() &&
         no_scientific_state_mutated &&
         all_allocations_and_digests_completed &&
         decision == ExactNormalizedHartiganLevelAppendDecision::
                         complete_certified_record_prepared;
}

bool ExactNormalizedHartiganLevelCheckpointExportResult::
    certified_semantic_checkpoint() const noexcept {
  try {
    return checkpoint.has_value() &&
           checkpoint->schema_version ==
               normalized_exact_hartigan_level_manifest_checkpoint_schema_version &&
           checkpoint->manifest_schema_version ==
               normalized_exact_hartigan_level_manifest_schema_version &&
           !id_is_zero(checkpoint->checkpoint_digest) &&
           checkpoint->checkpoint_digest ==
               semantic_checkpoint_digest(*checkpoint) &&
           !checkpoint->process_local_authority_serialized &&
           !checkpoint->resource_budget_serialized &&
           !checkpoint->manifest_record_arena_materialized &&
           !checkpoint->public_status_claimed &&
           decision == ExactNormalizedHartiganLevelCheckpointDecision::
                           complete_certified_semantic_checkpoint;
  } catch (...) {
    return false;
  }
}

bool ExactNormalizedHartiganLevelManifestSeal::certified_seal()
    const noexcept {
  const bool fixed_contract =
      contract_mode ==
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract;
  const bool streaming_contract =
      contract_mode ==
      ExactNormalizedHartiganManifestContractMode::
          streaming_observed_open_contract;
  if (schema_version !=
          normalized_exact_hartigan_level_manifest_schema_version ||
      authority_id == 0U || maximum_order == 0U ||
      maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      id_is_zero(normalized_batch_final_chain_digest) ||
      id_is_zero(ordered_rational_chain_digest) ||
      !all_normalized_equal_level_batches_covered ||
      !one_record_per_batch_including_omitted_noop ||
      !binary64_level_count_zero || !bounded_memory_summary_only ||
      manifest_file_bytes_materialized || public_status_claimed ||
      (!fixed_contract && !streaming_contract) ||
      (fixed_contract &&
       streaming_counts_closed_from_move_only_exhaustion_attestation) ||
      (streaming_contract &&
       (!streaming_counts_closed_from_move_only_exhaustion_attestation ||
        !every_record_uses_complete_batch_qr_partition ||
        legacy_single_group_scalar_qr_compatibility_used)) ||
      decision != ExactNormalizedHartiganLevelSealDecision::
                      complete_certified_manifest_seal) {
    return false;
  }
  std::size_t total = 0U;
  std::size_t legacy_record_total = 0U;
  for (std::size_t index = 0U; index < records_by_order.size(); ++index) {
    const auto& summary = records_by_order[index];
    std::size_t group_partition_total = 0U;
    if (summary.order != index + 1U ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.rational_record_count ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.normalized_qr1_group_count ||
        summary.legacy_single_group_scalar_qr_record_count >
            summary.rational_record_count ||
        (streaming_contract &&
         summary.legacy_single_group_scalar_qr_record_count != 0U) ||
        (summary.rational_record_count == 0U) !=
            (summary.normalized_group_count == 0U) ||
        summary.normalized_group_count < summary.rational_record_count ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr0_group_count,
            group_partition_total) ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr1_group_count,
            group_partition_total) ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr_greater_equal2_group_count,
            group_partition_total) ||
        group_partition_total != summary.normalized_group_count ||
        (summary.rational_record_count == 0U) !=
            (!summary.first_level.has_value() &&
             !summary.last_level.has_value()) ||
        (summary.rational_record_count != 0U &&
         (!summary.first_level.has_value() || !summary.last_level.has_value() ||
          *summary.last_level < *summary.first_level)) ||
        !checked_add(total, summary.rational_record_count, total) ||
        !checked_add(
            legacy_record_total,
            summary.legacy_single_group_scalar_qr_record_count,
            legacy_record_total)) {
      return false;
    }
  }
  return total == manifest_record_count &&
         every_record_uses_complete_batch_qr_partition ==
             (legacy_record_total == 0U) &&
         legacy_single_group_scalar_qr_compatibility_used ==
             (legacy_record_total != 0U);
}

bool ExactNormalizedHartiganLevelManifestSession::certified_ready()
    const noexcept {
  const bool fixed_contract =
      contract_mode_ ==
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract;
  const bool streaming_contract =
      contract_mode_ ==
      ExactNormalizedHartiganManifestContractMode::
          streaming_observed_open_contract;
  if (!initialized_ || sealed_ || contract_.authority_id == 0U ||
      live_prepared_append_count_ == nullptr ||
      *live_prepared_append_count_ > maximum_live_prepared_append_count ||
      contract_.maximum_order == 0U ||
      contract_.maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      (!fixed_contract && !streaming_contract) ||
      id_is_zero(contract_.normalized_source_scientific_digest) ||
      id_is_zero(contract_.normalized_batch_initial_chain_digest) ||
      (fixed_contract &&
       id_is_zero(contract_.normalized_batch_final_chain_digest)) ||
      (streaming_contract &&
       !id_is_zero(contract_.normalized_batch_final_chain_digest)) ||
      (fixed_contract &&
       expected_total_record_count_ > budget_.maximum_record_count) ||
      (fixed_contract && record_count_ > expected_total_record_count_) ||
      (streaming_contract && expected_total_record_count_ != 0U) ||
      (streaming_contract && record_count_ > budget_.maximum_record_count) ||
      (record_count_ == 0U) != (last_global_order_ == 0U) ||
      last_global_order_ > contract_.maximum_order ||
      id_is_zero(current_source_chain_digest_) ||
      id_is_zero(current_rational_chain_digest_)) {
    return false;
  }
  std::size_t observed = 0U;
  for (std::size_t index = 0U; index < summaries_.size(); ++index) {
    const auto& summary = summaries_[index];
    std::size_t group_partition_total = 0U;
    if (summary.order != index + 1U ||
        (fixed_contract &&
         summary.rational_record_count >
             contract_.expected_record_count_by_order[index]) ||
        (fixed_contract &&
         summary.normalized_omitted_noop_qr1_batch_count >
             contract_.expected_omitted_noop_count_by_order[index]) ||
        (streaming_contract &&
         (contract_.expected_record_count_by_order[index] != 0U ||
          contract_.expected_omitted_noop_count_by_order[index] != 0U)) ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.normalized_qr1_group_count ||
        summary.legacy_single_group_scalar_qr_record_count >
            summary.rational_record_count ||
        (streaming_contract &&
         summary.legacy_single_group_scalar_qr_record_count != 0U) ||
        (summary.rational_record_count == 0U) !=
            (summary.normalized_group_count == 0U) ||
        summary.normalized_group_count < summary.rational_record_count ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr0_group_count,
            group_partition_total) ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr1_group_count,
            group_partition_total) ||
        !checked_add(
            group_partition_total,
            summary.normalized_qr_greater_equal2_group_count,
            group_partition_total) ||
        group_partition_total != summary.normalized_group_count ||
        !checked_add(observed, summary.rational_record_count, observed)) {
      return false;
    }
  }
  return observed == record_count_;
}

bool ExactNormalizedHartiganLevelManifestSession::sealed() const noexcept {
  return sealed_;
}

std::size_t ExactNormalizedHartiganLevelManifestSession::record_count()
    const noexcept {
  return record_count_;
}

ExactNormalizedHartiganManifestContractMode
ExactNormalizedHartiganLevelManifestSession::contract_mode()
    const noexcept {
  return contract_mode_;
}

const contract::CanonicalId&
ExactNormalizedHartiganLevelManifestSession::current_rational_chain_digest()
    const noexcept {
  return current_rational_chain_digest_;
}

ExactNormalizedHartiganLevelPreparationResult
ExactNormalizedHartiganLevelManifestSession::prepare_append(
    const ExactNormalizedHartiganLevelInput& input) {
  static_assert(std::is_nothrow_move_constructible_v<exact::ExactLevel>);
  static_assert(std::is_nothrow_move_assignable_v<exact::ExactLevel>);
  const auto reject = [](ExactNormalizedHartiganLevelAppendDecision decision) {
    ExactNormalizedHartiganLevelPreparationResult rejected{};
    rejected.no_scientific_state_mutated = true;
    rejected.all_allocations_and_digests_completed = false;
    rejected.decision = decision;
    return rejected;
  };
  if (!certified_ready()) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::no_session_not_ready);
  }
  if (*live_prepared_append_count_ >=
      maximum_live_prepared_append_count) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_outstanding_prepared_append_budget_exhausted);
  }
  if ((contract_mode_ ==
           ExactNormalizedHartiganManifestContractMode::
               fixed_predeclared_contract &&
       record_count_ >= expected_total_record_count_) ||
      record_count_ >= budget_.maximum_record_count) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_budget_exhausted);
  }
  if (input.normalized_batch_index != record_count_ ||
      input.order == 0U || input.order > contract_.maximum_order ||
      input.order_batch_index != summaries_[input.order - 1U]
                                     .rational_record_count) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_chronology_rejected);
  }
  const std::size_t order_index = input.order - 1U;
  if (contract_mode_ ==
          ExactNormalizedHartiganManifestContractMode::
              fixed_predeclared_contract &&
      (summaries_[order_index].rational_record_count >=
           contract_.expected_record_count_by_order[order_index] ||
       (input.disposition ==
           ExactNormalizedHartiganBatchDisposition::
               omitted_certified_qr1_noop &&
       summaries_[order_index].normalized_omitted_noop_qr1_batch_count >=
           contract_.expected_omitted_noop_count_by_order[order_index]))) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::no_record_order_rejected);
  }
  if (!level_fits_decimal_budget(
          input.squared_level, budget_.maximum_decimal_digits_per_integer) ||
      (last_global_level_.has_value() &&
       (input.squared_level < *last_global_level_ ||
        (input.squared_level == *last_global_level_ &&
         input.order <= last_global_order_))) ||
      (summaries_[order_index].last_level.has_value() &&
       input.squared_level < *summaries_[order_index].last_level)) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_exact_level_rejected);
  }
  if (input.previous_normalized_batch_chain_digest !=
          current_source_chain_digest_ ||
      id_is_zero(input.normalized_batch_chain_digest) ||
      input.normalized_batch_chain_digest ==
          input.previous_normalized_batch_chain_digest) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_source_chain_rejected);
  }
  const auto group_summary = normalized_group_summary(input);
  if (!group_summary.has_value()) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_group_summary_rejected);
  }
  if (contract_mode_ ==
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract &&
      !group_summary->complete_batch_partition) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_group_summary_rejected);
  }
  if (!batch_group_summary_rule(input)) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_omitted_noop_rule_rejected);
  }

  const auto& current_order_summary = summaries_[order_index];
  std::size_t next_group_count = 0U;
  std::size_t next_qr0_group_count = 0U;
  std::size_t next_qr1_group_count = 0U;
  std::size_t next_qr_greater_equal2_group_count = 0U;
  std::size_t next_legacy_record_count = 0U;
  if (!checked_add(
          current_order_summary.normalized_group_count,
          group_summary->group_count,
          next_group_count) ||
      !checked_add(
          current_order_summary.normalized_qr0_group_count,
          group_summary->qr0_group_count,
          next_qr0_group_count) ||
      !checked_add(
          current_order_summary.normalized_qr1_group_count,
          group_summary->qr1_group_count,
          next_qr1_group_count) ||
      !checked_add(
          current_order_summary.normalized_qr_greater_equal2_group_count,
          group_summary->qr_greater_equal2_group_count,
          next_qr_greater_equal2_group_count) ||
      !checked_add(
          current_order_summary.legacy_single_group_scalar_qr_record_count,
          group_summary->legacy_single_group_scalar ? 1U : 0U,
          next_legacy_record_count)) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_group_summary_rejected);
  }

  try {
    auto prepared = std::make_unique<
        ExactNormalizedHartiganLevelPreparedAppend::Impl>();
    prepared->live_prepared_append_count = live_prepared_append_count_;
    prepared->source_record_count = record_count_;
    prepared->next_record_count = record_count_ + 1U;
    prepared->source_source_chain_digest = current_source_chain_digest_;
    prepared->source_rational_chain_digest =
        current_rational_chain_digest_;
    prepared->next_source_chain_digest =
        input.normalized_batch_chain_digest;
    prepared->record.record_index = record_count_;
    prepared->record.input = input;
    prepared->record.previous_rational_chain_digest =
        current_rational_chain_digest_;
    prepared->record.exact_rational_reduced_and_nonnegative = true;
    prepared->record.binary64_serialization_used = false;
    prepared->record.source_batch_chain_contiguous = true;
    prepared->record.omitted_noop_rule_certified = true;
    prepared->record.complete_batch_qr_partition_certified =
        group_summary->complete_batch_partition;
    prepared->record.legacy_single_group_scalar_qr_compatibility_used =
        group_summary->legacy_single_group_scalar;
    prepared->record.public_status_claimed = false;
    prepared->record.rational_chain_digest =
        record_chain_digest(prepared->record);
    prepared->next_rational_chain_digest =
        prepared->record.rational_chain_digest;
    if (!prepared->record.structurally_certified()) {
      return reject(
          ExactNormalizedHartiganLevelAppendDecision::
              no_record_exact_level_rejected);
    }

    // Every potentially allocating exact-level copy and the complete digest
    // are owned by the opaque ticket before its one live slot is published.
    prepared->next_summaries = summaries_;
    prepared->next_last_global_level = input.squared_level;
    prepared->next_last_global_order = input.order;
    auto& next_order_summary = prepared->next_summaries[order_index];
    next_order_summary.last_level = input.squared_level;
    if (!next_order_summary.first_level.has_value()) {
      next_order_summary.first_level = input.squared_level;
    }
    ++next_order_summary.rational_record_count;
    next_order_summary.normalized_group_count = next_group_count;
    next_order_summary.normalized_qr0_group_count = next_qr0_group_count;
    next_order_summary.normalized_qr1_group_count = next_qr1_group_count;
    next_order_summary.normalized_qr_greater_equal2_group_count =
        next_qr_greater_equal2_group_count;
    next_order_summary.legacy_single_group_scalar_qr_record_count =
        next_legacy_record_count;
    if (input.disposition ==
        ExactNormalizedHartiganBatchDisposition::
            omitted_certified_qr1_noop) {
      ++next_order_summary.normalized_omitted_noop_qr1_batch_count;
    }

    prepared->owns_live_slot = true;
    ++*live_prepared_append_count_;
    ExactNormalizedHartiganLevelPreparationResult output;
    output.ticket.emplace(ExactNormalizedHartiganLevelPreparedAppend{
        std::move(prepared)});
    output.no_scientific_state_mutated = true;
    output.all_allocations_and_digests_completed = true;
    output.decision = ExactNormalizedHartiganLevelAppendDecision::
        complete_certified_record_prepared;
    if (!output.certified_prepared_append()) {
      return reject(
          ExactNormalizedHartiganLevelAppendDecision::
              no_record_exact_level_rejected);
    }
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_allocation_failed);
  }
}

ExactNormalizedHartiganLevelAppendResult
ExactNormalizedHartiganLevelManifestSession::commit_prepared_append(
    ExactNormalizedHartiganLevelPreparedAppend&& ticket) noexcept {
  using PreparedImpl = ExactNormalizedHartiganLevelPreparedAppend::Impl;
  static_assert(std::is_nothrow_move_constructible_v<
                ExactNormalizedHartiganLevelManifestRecord>);
  static_assert(std::is_nothrow_move_assignable_v<
                std::array<
                    ExactNormalizedHartiganOrderSummary,
                    normalized_exact_hartigan_level_manifest_maximum_order>>);
  static_assert(std::is_nothrow_move_assignable_v<
                std::optional<exact::ExactLevel>>);

  if (ticket.impl_ == nullptr || ticket.impl_->consumed ||
      !ticket.impl_->owns_live_slot) {
    return rejected_append(
        ExactNormalizedHartiganLevelAppendDecision::
            no_prepared_append_already_consumed);
  }
  PreparedImpl& prepared = *ticket.impl_;
  const auto consume_ticket = [&prepared]() noexcept {
    if (prepared.owns_live_slot) {
      if (prepared.live_prepared_append_count == nullptr ||
          *prepared.live_prepared_append_count == 0U) {
        std::terminate();
      }
      --*prepared.live_prepared_append_count;
      prepared.owns_live_slot = false;
    }
    prepared.consumed = true;
  };

  if (!certified_ready()) {
    consume_ticket();
    return rejected_append(
        ExactNormalizedHartiganLevelAppendDecision::no_session_not_ready);
  }
  if (prepared.live_prepared_append_count !=
      live_prepared_append_count_) {
    consume_ticket();
    return rejected_append(
        ExactNormalizedHartiganLevelAppendDecision::
            no_foreign_prepared_append_rejected);
  }
  if (*live_prepared_append_count_ != 1U ||
      prepared.source_record_count != record_count_ ||
      prepared.source_source_chain_digest != current_source_chain_digest_ ||
      prepared.source_rational_chain_digest !=
          current_rational_chain_digest_ ||
      prepared.next_record_count != record_count_ + 1U ||
      prepared.record.record_index != record_count_ ||
      prepared.record.rational_chain_digest !=
          prepared.next_rational_chain_digest ||
      prepared.record.input.normalized_batch_chain_digest !=
          prepared.next_source_chain_digest ||
      !prepared.record.structurally_certified()) {
    consume_ticket();
    return rejected_append(
        ExactNormalizedHartiganLevelAppendDecision::
            no_stale_prepared_append_rejected);
  }

  ExactNormalizedHartiganLevelAppendResult output;
  output.record.emplace(std::move(prepared.record));
  summaries_ = std::move(prepared.next_summaries);
  last_global_level_ = std::move(prepared.next_last_global_level);
  last_global_order_ = prepared.next_last_global_order;
  current_source_chain_digest_ = prepared.next_source_chain_digest;
  current_rational_chain_digest_ = prepared.next_rational_chain_digest;
  record_count_ = prepared.next_record_count;
  consume_ticket();
  output.session_state_mutated = true;
  output.no_state_mutated_on_failure = false;
  output.decision = ExactNormalizedHartiganLevelAppendDecision::
      complete_certified_record_appended;
  return output;
}

ExactNormalizedHartiganLevelAppendResult
ExactNormalizedHartiganLevelManifestSession::commit(
    ExactNormalizedHartiganLevelPreparedAppend&& ticket) noexcept {
  return commit_prepared_append(std::move(ticket));
}

ExactNormalizedHartiganLevelAppendResult
ExactNormalizedHartiganLevelManifestSession::append(
    const ExactNormalizedHartiganLevelInput& input) {
  auto prepared = prepare_append(input);
  if (!prepared.certified_prepared_append()) {
    return rejected_append(prepared.decision);
  }
  return commit_prepared_append(std::move(*prepared.ticket));
}

ExactNormalizedHartiganLevelCheckpointExportResult
ExactNormalizedHartiganLevelManifestSession::make_checkpoint()
    const noexcept {
  ExactNormalizedHartiganLevelCheckpointExportResult output;
  if (!certified_ready()) {
    output.decision = ExactNormalizedHartiganLevelCheckpointDecision::
        no_session_not_ready;
    return output;
  }
  if (*live_prepared_append_count_ != 0U) {
    output.decision = ExactNormalizedHartiganLevelCheckpointDecision::
        no_outstanding_prepared_append;
    return output;
  }
  try {
    ExactNormalizedHartiganLevelManifestCheckpoint checkpoint;
    checkpoint.contract_mode = contract_mode_;
    checkpoint.maximum_order = contract_.maximum_order;
    checkpoint.expected_record_count_by_order =
        contract_.expected_record_count_by_order;
    checkpoint.expected_omitted_noop_count_by_order =
        contract_.expected_omitted_noop_count_by_order;
    checkpoint.normalized_source_scientific_digest =
        contract_.normalized_source_scientific_digest;
    checkpoint.normalized_batch_initial_chain_digest =
        contract_.normalized_batch_initial_chain_digest;
    checkpoint.normalized_batch_final_chain_digest =
        contract_.normalized_batch_final_chain_digest;
    checkpoint.records_by_order = summaries_;
    checkpoint.last_global_level = last_global_level_;
    checkpoint.last_global_order = last_global_order_;
    checkpoint.current_source_chain_digest = current_source_chain_digest_;
    checkpoint.current_rational_chain_digest =
        current_rational_chain_digest_;
    checkpoint.expected_total_record_count = expected_total_record_count_;
    checkpoint.record_count = record_count_;
    checkpoint.process_local_authority_serialized = false;
    checkpoint.resource_budget_serialized = false;
    checkpoint.manifest_record_arena_materialized = false;
    checkpoint.public_status_claimed = false;
    checkpoint.checkpoint_digest = semantic_checkpoint_digest(checkpoint);
    if (!checkpoint_shape_certified(checkpoint, budget_) ||
        checkpoint.checkpoint_digest !=
            semantic_checkpoint_digest(checkpoint)) {
      output.decision = ExactNormalizedHartiganLevelCheckpointDecision::
          no_checkpoint_shape_rejected;
      return output;
    }
    output.checkpoint.emplace(std::move(checkpoint));
    output.decision = ExactNormalizedHartiganLevelCheckpointDecision::
        complete_certified_semantic_checkpoint;
    return output;
  } catch (...) {
    output.decision = ExactNormalizedHartiganLevelCheckpointDecision::
        no_checkpoint_allocation_failed;
    return output;
  }
}

std::optional<ExactNormalizedHartiganStreamingClosureAttestation>
ExactNormalizedHartiganLevelManifestSession::
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
        const contract::CanonicalId& expected_final_chain_digest) {
  if (!certified_ready() ||
      contract_mode_ !=
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract ||
      *live_prepared_append_count_ != 0U ||
      covered_source_scientific_digest !=
          contract_.normalized_source_scientific_digest ||
      covered_batch_count != record_count_ ||
      id_is_zero(expected_final_chain_digest) ||
      expected_final_chain_digest != current_source_chain_digest_) {
    return std::nullopt;
  }
  for (std::size_t index = 0U; index < summaries_.size(); ++index) {
    if (covered_record_count_by_order[index] !=
            summaries_[index].rational_record_count ||
        covered_omitted_noop_count_by_order[index] !=
            summaries_[index].normalized_omitted_noop_qr1_batch_count ||
        summaries_[index].legacy_single_group_scalar_qr_record_count != 0U) {
      return std::nullopt;
    }
  }

  try {
    auto impl = std::make_unique<
        ExactNormalizedHartiganStreamingClosureAttestation::Impl>();
    impl->live_scientific_transaction_count =
        live_prepared_append_count_;
    impl->covered_source_scientific_digest =
        covered_source_scientific_digest;
    impl->source_chain_digest = current_source_chain_digest_;
    impl->rational_chain_digest = current_rational_chain_digest_;
    impl->expected_final_chain_digest = expected_final_chain_digest;
    impl->covered_record_count_by_order = covered_record_count_by_order;
    impl->covered_omitted_noop_count_by_order =
        covered_omitted_noop_count_by_order;
    impl->covered_batch_count = covered_batch_count;
    impl->source_record_count = record_count_;
    impl->owns_live_slot = true;
    ++*live_prepared_append_count_;
    ExactNormalizedHartiganStreamingClosureAttestation attestation{
        std::move(impl)};
    if (!attestation.valid()) {
      return std::nullopt;
    }
    return std::optional<
        ExactNormalizedHartiganStreamingClosureAttestation>{
        std::move(attestation)};
  } catch (...) {
    return std::nullopt;
  }
}

ExactNormalizedHartiganLevelManifestSeal
ExactNormalizedHartiganLevelManifestSession::seal_streaming(
    ExactNormalizedHartiganStreamingClosureAttestation&& attestation) {
  const auto reject = [](ExactNormalizedHartiganLevelSealDecision decision) {
    ExactNormalizedHartiganLevelManifestSeal rejected{};
    rejected.decision = decision;
    return rejected;
  };
  if (attestation.impl_ == nullptr || attestation.impl_->consumed ||
      !attestation.impl_->owns_live_slot) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_streaming_closure_attestation_rejected);
  }
  auto& closure = *attestation.impl_;
  const auto consume_attestation = [&closure]() noexcept {
    if (closure.owns_live_slot) {
      if (closure.live_scientific_transaction_count == nullptr ||
          *closure.live_scientific_transaction_count == 0U) {
        std::terminate();
      }
      --*closure.live_scientific_transaction_count;
      closure.owns_live_slot = false;
    }
    closure.consumed = true;
  };
  if (!certified_ready() ||
      contract_mode_ !=
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract ||
      closure.live_scientific_transaction_count !=
          live_prepared_append_count_ ||
      !attestation.valid() || closure.source_record_count != record_count_ ||
      closure.covered_batch_count != record_count_ ||
      closure.covered_source_scientific_digest !=
          contract_.normalized_source_scientific_digest ||
      closure.source_chain_digest != current_source_chain_digest_ ||
      closure.expected_final_chain_digest != current_source_chain_digest_ ||
      closure.rational_chain_digest != current_rational_chain_digest_) {
    consume_attestation();
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_streaming_closure_attestation_rejected);
  }
  for (std::size_t index = 0U; index < summaries_.size(); ++index) {
    if (closure.covered_record_count_by_order[index] !=
            summaries_[index].rational_record_count ||
        closure.covered_omitted_noop_count_by_order[index] !=
            summaries_[index].normalized_omitted_noop_qr1_batch_count ||
        summaries_[index].legacy_single_group_scalar_qr_record_count != 0U) {
      consume_attestation();
      return reject(
          ExactNormalizedHartiganLevelSealDecision::
              no_streaming_closure_attestation_rejected);
    }
  }

  ExactNormalizedHartiganLevelManifestSeal output;
  output.authority_id = contract_.authority_id;
  output.maximum_order = contract_.maximum_order;
  output.manifest_record_count = record_count_;
  output.contract_mode =
      ExactNormalizedHartiganManifestContractMode::
          streaming_observed_open_contract;
  output.records_by_order = summaries_;
  output.normalized_batch_final_chain_digest = current_source_chain_digest_;
  output.ordered_rational_chain_digest = current_rational_chain_digest_;
  output.all_normalized_equal_level_batches_covered = true;
  output.one_record_per_batch_including_omitted_noop = true;
  output.binary64_level_count_zero = true;
  output.bounded_memory_summary_only = true;
  output.manifest_file_bytes_materialized = false;
  output.every_record_uses_complete_batch_qr_partition = true;
  output.legacy_single_group_scalar_qr_compatibility_used = false;
  output.streaming_counts_closed_from_move_only_exhaustion_attestation = true;
  output.public_status_claimed = false;
  output.decision = ExactNormalizedHartiganLevelSealDecision::
      complete_certified_manifest_seal;
  if (!output.certified_seal()) {
    consume_attestation();
    return {};
  }
  consume_attestation();
  sealed_ = true;
  return output;
}

ExactNormalizedHartiganLevelManifestSeal
ExactNormalizedHartiganLevelManifestSession::seal() {
  const auto reject = [](ExactNormalizedHartiganLevelSealDecision decision) {
    ExactNormalizedHartiganLevelManifestSeal rejected{};
    rejected.decision = decision;
    return rejected;
  };
  if (!certified_ready()) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::no_session_not_ready);
  }
  if (*live_prepared_append_count_ != 0U) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_outstanding_prepared_append_rejected);
  }
  if (contract_mode_ ==
      ExactNormalizedHartiganManifestContractMode::
          streaming_observed_open_contract) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_streaming_closure_authority_required);
  }
  if (record_count_ != expected_total_record_count_) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_manifest_record_count_mismatch);
  }
  for (std::size_t index = 0U; index < summaries_.size(); ++index) {
    if (summaries_[index].rational_record_count !=
            contract_.expected_record_count_by_order[index] ||
        summaries_[index].normalized_omitted_noop_qr1_batch_count !=
            contract_.expected_omitted_noop_count_by_order[index]) {
      return reject(
          ExactNormalizedHartiganLevelSealDecision::
              no_manifest_record_count_mismatch);
    }
  }
  if (current_source_chain_digest_ !=
      contract_.normalized_batch_final_chain_digest) {
    return reject(
        ExactNormalizedHartiganLevelSealDecision::
            no_manifest_source_chain_not_closed);
  }
  std::size_t legacy_record_count = 0U;
  for (const auto& summary : summaries_) {
    if (!checked_add(
            legacy_record_count,
            summary.legacy_single_group_scalar_qr_record_count,
            legacy_record_count)) {
      return reject(
          ExactNormalizedHartiganLevelSealDecision::
              no_manifest_record_count_mismatch);
    }
  }

  ExactNormalizedHartiganLevelManifestSeal output;
  output.authority_id = contract_.authority_id;
  output.maximum_order = contract_.maximum_order;
  output.manifest_record_count = record_count_;
  output.contract_mode =
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract;
  output.records_by_order = std::move(summaries_);
  output.normalized_batch_final_chain_digest = current_source_chain_digest_;
  output.ordered_rational_chain_digest = current_rational_chain_digest_;
  output.all_normalized_equal_level_batches_covered = true;
  output.one_record_per_batch_including_omitted_noop = true;
  output.binary64_level_count_zero = true;
  output.bounded_memory_summary_only = true;
  output.manifest_file_bytes_materialized = false;
  output.every_record_uses_complete_batch_qr_partition =
      legacy_record_count == 0U;
  output.legacy_single_group_scalar_qr_compatibility_used =
      legacy_record_count != 0U;
  output.streaming_counts_closed_from_move_only_exhaustion_attestation = false;
  output.public_status_claimed = false;
  output.decision = ExactNormalizedHartiganLevelSealDecision::
      complete_certified_manifest_seal;
  if (!output.certified_seal()) {
    return {};
  }
  sealed_ = true;
  return output;
}

bool ExactNormalizedHartiganLevelInitializationResult::
    certified_initialized_session() const noexcept {
  const bool empty_initialization =
      decision == ExactNormalizedHartiganLevelInitializationDecision::
                      complete_certified_empty_manifest_session &&
      session.has_value() && session->record_count() == 0U &&
      session->contract_mode() ==
          ExactNormalizedHartiganManifestContractMode::
              fixed_predeclared_contract &&
      !checkpoint_freshly_verified;
  const bool empty_streaming_initialization =
      decision == ExactNormalizedHartiganLevelInitializationDecision::
                      complete_certified_empty_streaming_manifest_session &&
      session.has_value() && session->record_count() == 0U &&
      session->contract_mode() ==
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract &&
      !checkpoint_freshly_verified;
  const bool checkpoint_restoration =
      decision == ExactNormalizedHartiganLevelInitializationDecision::
                      complete_restored_certified_manifest_session &&
      session.has_value() && checkpoint_freshly_verified;
  return session.has_value() && session->certified_ready() &&
         required_record_count <= session->budget_.maximum_record_count &&
         contract_preflight_certified && budget_preflight_certified &&
         no_manifest_record_arena_materialized &&
         (empty_initialization || empty_streaming_initialization ||
          checkpoint_restoration);
}

ExactNormalizedHartiganLevelInitializationResult
initialize_exact_normalized_hartigan_level_manifest_session(
    const ExactNormalizedHartiganLevelManifestContract& manifest_contract,
    const ExactNormalizedHartiganLevelManifestBudget& budget) {
  ExactNormalizedHartiganLevelInitializationResult output;
  if (manifest_contract.authority_id == 0U ||
      manifest_contract.maximum_order == 0U ||
      manifest_contract.maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      id_is_zero(manifest_contract.normalized_source_scientific_digest) ||
      id_is_zero(manifest_contract.normalized_batch_initial_chain_digest) ||
      id_is_zero(manifest_contract.normalized_batch_final_chain_digest)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_contract_rejected;
    return output;
  }
  std::size_t total = 0U;
  for (std::size_t index = 0U;
       index < normalized_exact_hartigan_level_manifest_maximum_order;
       ++index) {
    const std::size_t expected =
        manifest_contract.expected_record_count_by_order[index];
    const std::size_t omitted =
        manifest_contract.expected_omitted_noop_count_by_order[index];
    if (omitted > expected ||
        (index >= manifest_contract.maximum_order &&
         (expected != 0U || omitted != 0U)) ||
        !fits_u64(expected) || !fits_u64(omitted) ||
        !checked_add(total, expected, total)) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_session_contract_rejected;
      return output;
    }
  }
  if ((total == 0U) !=
      (manifest_contract.normalized_batch_initial_chain_digest ==
       manifest_contract.normalized_batch_final_chain_digest)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_contract_rejected;
    return output;
  }
  output.required_record_count = total;
  output.contract_preflight_certified = true;
  if (budget.maximum_decimal_digits_per_integer == 0U ||
      budget.maximum_decimal_digits_per_integer >
          normalized_exact_hartigan_level_manifest_maximum_decimal_digits ||
      total > budget.maximum_record_count || !fits_u64(total)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_budget_rejected;
    return output;
  }
  output.budget_preflight_certified = true;

  ExactNormalizedHartiganLevelManifestSession session;
  session.contract_ = manifest_contract;
  session.contract_mode_ =
      ExactNormalizedHartiganManifestContractMode::
          fixed_predeclared_contract;
  session.budget_ = budget;
  session.expected_total_record_count_ = total;
  session.current_source_chain_digest_ =
      manifest_contract.normalized_batch_initial_chain_digest;
  session.current_rational_chain_digest_ =
      initial_chain_digest(manifest_contract);
  for (std::size_t index = 0U; index < session.summaries_.size(); ++index) {
    session.summaries_[index].order = index + 1U;
  }
  try {
    session.live_prepared_append_count_ =
        std::make_shared<std::size_t>(0U);
  } catch (...) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_allocation_failed;
    return output;
  }
  session.initialized_ = true;
  if (!session.certified_ready()) {
    output = {};
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_contract_rejected;
    return output;
  }
  output.session.emplace(std::move(session));
  output.no_manifest_record_arena_materialized = true;
  output.decision = ExactNormalizedHartiganLevelInitializationDecision::
      complete_certified_empty_manifest_session;
  return output;
}

ExactNormalizedHartiganLevelInitializationResult
ExactNormalizedHartiganLevelManifestSession::
    initialize_streaming_manifest_session(
        std::uint64_t authority_id,
        std::size_t maximum_order,
        const contract::CanonicalId& normalized_source_scientific_digest,
        const contract::CanonicalId&
            normalized_batch_initial_chain_digest,
        const ExactNormalizedHartiganLevelManifestBudget& budget) {
  ExactNormalizedHartiganLevelInitializationResult output;
  if (authority_id == 0U || maximum_order == 0U ||
      maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      id_is_zero(normalized_source_scientific_digest) ||
      id_is_zero(normalized_batch_initial_chain_digest)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_contract_rejected;
    return output;
  }
  output.contract_preflight_certified = true;
  if (budget.maximum_record_count == 0U ||
      budget.maximum_decimal_digits_per_integer == 0U ||
      budget.maximum_decimal_digits_per_integer >
          normalized_exact_hartigan_level_manifest_maximum_decimal_digits ||
      !fits_u64(budget.maximum_record_count)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_budget_rejected;
    return output;
  }
  output.budget_preflight_certified = true;

  try {
    ExactNormalizedHartiganLevelManifestSession session;
    session.contract_.authority_id = authority_id;
    session.contract_.maximum_order = maximum_order;
    session.contract_.normalized_source_scientific_digest =
        normalized_source_scientific_digest;
    session.contract_.normalized_batch_initial_chain_digest =
        normalized_batch_initial_chain_digest;
    session.contract_mode_ =
        ExactNormalizedHartiganManifestContractMode::
            streaming_observed_open_contract;
    session.budget_ = budget;
    session.current_source_chain_digest_ =
        normalized_batch_initial_chain_digest;
    session.current_rational_chain_digest_ = streaming_initial_chain_digest(
        maximum_order,
        normalized_source_scientific_digest,
        normalized_batch_initial_chain_digest);
    for (std::size_t index = 0U; index < session.summaries_.size(); ++index) {
      session.summaries_[index].order = index + 1U;
    }
    session.live_prepared_append_count_ =
        std::make_shared<std::size_t>(0U);
    session.initialized_ = true;
    if (!session.certified_ready()) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_session_contract_rejected;
      return output;
    }
    output.session.emplace(std::move(session));
    output.required_record_count = 0U;
    output.no_manifest_record_arena_materialized = true;
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        complete_certified_empty_streaming_manifest_session;
    return output;
  } catch (...) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_allocation_failed;
    return output;
  }
}

ExactNormalizedHartiganLevelInitializationResult
restore_exact_normalized_hartigan_level_manifest_session(
    ExactNormalizedHartiganLevelManifestCheckpoint&& checkpoint,
    std::uint64_t reminted_authority_id,
    const ExactNormalizedHartiganLevelManifestBudget& budget) {
  ExactNormalizedHartiganLevelInitializationResult output;
  output.required_record_count = checkpoint.expected_total_record_count;
  if (reminted_authority_id == 0U) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_contract_rejected;
    return output;
  }
  if (budget.maximum_decimal_digits_per_integer == 0U ||
      budget.maximum_decimal_digits_per_integer >
          normalized_exact_hartigan_level_manifest_maximum_decimal_digits ||
      (checkpoint.contract_mode ==
           ExactNormalizedHartiganManifestContractMode::
               fixed_predeclared_contract &&
       checkpoint.expected_total_record_count > budget.maximum_record_count) ||
      (checkpoint.contract_mode ==
           ExactNormalizedHartiganManifestContractMode::
               streaming_observed_open_contract &&
       checkpoint.record_count > budget.maximum_record_count)) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_budget_rejected;
    return output;
  }
  output.budget_preflight_certified = true;

  try {
    if (!checkpoint_shape_certified(checkpoint, budget)) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_checkpoint_shape_rejected;
      return output;
    }
    output.contract_preflight_certified = true;
    if (checkpoint.checkpoint_digest !=
        semantic_checkpoint_digest(checkpoint)) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_checkpoint_digest_rejected;
      return output;
    }

    ExactNormalizedHartiganLevelManifestSession session;
    session.contract_ = checkpoint_contract(checkpoint, reminted_authority_id);
    session.contract_mode_ = checkpoint.contract_mode;
    session.budget_ = budget;
    session.summaries_ = checkpoint.records_by_order;
    session.last_global_level_ = checkpoint.last_global_level;
    session.last_global_order_ = checkpoint.last_global_order;
    session.current_source_chain_digest_ =
        checkpoint.current_source_chain_digest;
    session.current_rational_chain_digest_ =
        checkpoint.current_rational_chain_digest;
    session.expected_total_record_count_ =
        checkpoint.expected_total_record_count;
    session.record_count_ = checkpoint.record_count;
    session.live_prepared_append_count_ =
        std::make_shared<std::size_t>(0U);
    session.initialized_ = true;
    if (!session.certified_ready()) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_checkpoint_shape_rejected;
      return output;
    }

    // Re-export from independently reconstructed state.  Equality proves that
    // restore did not silently import a process-local authority or a resource
    // ceiling into the durable scientific identity.
    const auto freshly_reconstructed = session.make_checkpoint();
    if (!freshly_reconstructed.certified_semantic_checkpoint() ||
        *freshly_reconstructed.checkpoint != checkpoint) {
      output.decision = ExactNormalizedHartiganLevelInitializationDecision::
          no_checkpoint_digest_rejected;
      return output;
    }

    output.session.emplace(std::move(session));
    output.no_manifest_record_arena_materialized = true;
    output.checkpoint_freshly_verified = true;
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        complete_restored_certified_manifest_session;
    return output;
  } catch (...) {
    output.decision = ExactNormalizedHartiganLevelInitializationDecision::
        no_session_allocation_failed;
    return output;
  }
}

bool verify_exact_normalized_hartigan_level_record(
    const ExactNormalizedHartiganLevelManifestRecord& record) {
  return record.structurally_certified() &&
         batch_group_summary_rule(record.input) &&
         record.rational_chain_digest == record_chain_digest(record);
}

}  // namespace morsehgp3d::hierarchy
