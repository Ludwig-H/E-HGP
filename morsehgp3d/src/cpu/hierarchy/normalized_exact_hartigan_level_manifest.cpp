#include "morsehgp3d/hierarchy/normalized_exact_hartigan_level_manifest.hpp"

#include <boost/multiprecision/integer.hpp>

#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-initial/v1";
constexpr std::string_view record_chain_domain =
    "MorseHGP3D/phase15/normalized-exact-hartigan-record/v1";

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

[[nodiscard]] bool omitted_noop_rule(
    const ExactNormalizedHartiganLevelInput& input) noexcept {
  if (input.disposition ==
      ExactNormalizedHartiganBatchDisposition::
          materialized_normalized_equal_level_batch) {
    return true;
  }
  return input.disposition ==
             ExactNormalizedHartiganBatchDisposition::
                 omitted_certified_qr1_noop &&
         input.q_r == 1U && input.core_facet_delta_count == 0U &&
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
  return builder.finalize();
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

}  // namespace

bool ExactNormalizedHartiganLevelManifestRecord::structurally_certified()
    const noexcept {
  return schema_version ==
             normalized_exact_hartigan_level_manifest_schema_version &&
         record_index == input.normalized_batch_index && input.order != 0U &&
         input.order <=
             normalized_exact_hartigan_level_manifest_maximum_order &&
         input.q_r != 0U &&
         exact_rational_reduced_and_nonnegative &&
         !binary64_serialization_used && source_batch_chain_contiguous &&
         omitted_noop_rule_certified && !public_status_claimed &&
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
      ",\"core_facet_delta_count\":" +
      std::to_string(input.core_facet_delta_count) +
      ",\"denominator\":\"" + input.squared_level.denominator_string() +
      "\",\"disposition\":\"" + disposition_text(input.disposition) +
      "\",\"node_delta_count\":" + std::to_string(input.node_delta_count) +
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

bool ExactNormalizedHartiganLevelManifestSeal::certified_seal()
    const noexcept {
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
      decision != ExactNormalizedHartiganLevelSealDecision::
                      complete_certified_manifest_seal) {
    return false;
  }
  std::size_t total = 0U;
  for (std::size_t index = 0U; index < records_by_order.size(); ++index) {
    const auto& summary = records_by_order[index];
    if (summary.order != index + 1U ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.rational_record_count ||
        (summary.rational_record_count == 0U) !=
            (!summary.first_level.has_value() &&
             !summary.last_level.has_value()) ||
        (summary.rational_record_count != 0U &&
         (!summary.first_level.has_value() || !summary.last_level.has_value() ||
          *summary.last_level < *summary.first_level)) ||
        !checked_add(total, summary.rational_record_count, total)) {
      return false;
    }
  }
  return total == manifest_record_count;
}

bool ExactNormalizedHartiganLevelManifestSession::certified_ready()
    const noexcept {
  if (!initialized_ || sealed_ || contract_.authority_id == 0U ||
      contract_.maximum_order == 0U ||
      contract_.maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      expected_total_record_count_ > budget_.maximum_record_count ||
      record_count_ > expected_total_record_count_ ||
      (record_count_ == 0U) != (last_global_order_ == 0U) ||
      last_global_order_ > contract_.maximum_order ||
      id_is_zero(current_source_chain_digest_) ||
      id_is_zero(current_rational_chain_digest_)) {
    return false;
  }
  std::size_t observed = 0U;
  for (std::size_t index = 0U; index < summaries_.size(); ++index) {
    const auto& summary = summaries_[index];
    if (summary.order != index + 1U ||
        summary.rational_record_count >
            contract_.expected_record_count_by_order[index] ||
        summary.normalized_omitted_noop_qr1_batch_count >
            contract_.expected_omitted_noop_count_by_order[index] ||
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

const contract::CanonicalId&
ExactNormalizedHartiganLevelManifestSession::current_rational_chain_digest()
    const noexcept {
  return current_rational_chain_digest_;
}

ExactNormalizedHartiganLevelAppendResult
ExactNormalizedHartiganLevelManifestSession::append(
    const ExactNormalizedHartiganLevelInput& input) {
  static_assert(std::is_nothrow_move_constructible_v<exact::ExactLevel>);
  const auto reject = [](ExactNormalizedHartiganLevelAppendDecision decision) {
    ExactNormalizedHartiganLevelAppendResult rejected{};
    rejected.session_state_mutated = false;
    rejected.no_state_mutated_on_failure = true;
    rejected.decision = decision;
    return rejected;
  };
  if (!certified_ready()) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::no_session_not_ready);
  }
  if (record_count_ >= expected_total_record_count_ ||
      record_count_ >= budget_.maximum_record_count) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_budget_exhausted);
  }
  if (input.normalized_batch_index != record_count_ || input.q_r == 0U ||
      input.order == 0U || input.order > contract_.maximum_order ||
      input.order_batch_index != summaries_[input.order - 1U]
                                     .rational_record_count) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_chronology_rejected);
  }
  const std::size_t order_index = input.order - 1U;
  if (summaries_[order_index].rational_record_count >=
          contract_.expected_record_count_by_order[order_index] ||
      (input.disposition ==
           ExactNormalizedHartiganBatchDisposition::
               omitted_certified_qr1_noop &&
       summaries_[order_index].normalized_omitted_noop_qr1_batch_count >=
           contract_.expected_omitted_noop_count_by_order[order_index])) {
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
  if (!omitted_noop_rule(input)) {
    return reject(
        ExactNormalizedHartiganLevelAppendDecision::
            no_record_omitted_noop_rule_rejected);
  }

  try {
    ExactNormalizedHartiganLevelAppendResult result{};
    ExactNormalizedHartiganLevelManifestRecord record;
    record.record_index = record_count_;
    record.input = input;
    record.previous_rational_chain_digest =
        current_rational_chain_digest_;
    record.exact_rational_reduced_and_nonnegative = true;
    record.binary64_serialization_used = false;
    record.source_batch_chain_contiguous = true;
    record.omitted_noop_rule_certified = true;
    record.public_status_claimed = false;
    record.rational_chain_digest = record_chain_digest(record);
    if (!record.structurally_certified()) {
      return reject(
          ExactNormalizedHartiganLevelAppendDecision::
              no_record_exact_level_rejected);
    }

    // All potentially allocating exact-level copies are complete before any
    // session scalar is changed, preserving failure atomicity.
    exact::ExactLevel next_global_level = input.squared_level;
    exact::ExactLevel next_order_level = input.squared_level;
    std::optional<exact::ExactLevel> first_order_level;
    if (!summaries_[order_index].first_level.has_value()) {
      first_order_level = input.squared_level;
    }
    result.record.emplace(std::move(record));

    last_global_level_.emplace(std::move(next_global_level));
    last_global_order_ = input.order;
    summaries_[order_index].last_level.emplace(std::move(next_order_level));
    if (first_order_level.has_value()) {
      summaries_[order_index].first_level.emplace(
          std::move(*first_order_level));
    }
    ++summaries_[order_index].rational_record_count;
    if (input.disposition ==
        ExactNormalizedHartiganBatchDisposition::
            omitted_certified_qr1_noop) {
      ++summaries_[order_index]
            .normalized_omitted_noop_qr1_batch_count;
    }
    current_source_chain_digest_ = input.normalized_batch_chain_digest;
    current_rational_chain_digest_ = result.record->rational_chain_digest;
    ++record_count_;
    result.session_state_mutated = true;
    result.no_state_mutated_on_failure = false;
    result.decision = ExactNormalizedHartiganLevelAppendDecision::
        complete_certified_record_appended;
    return result;
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

  ExactNormalizedHartiganLevelManifestSeal output;
  output.authority_id = contract_.authority_id;
  output.maximum_order = contract_.maximum_order;
  output.manifest_record_count = record_count_;
  output.records_by_order = std::move(summaries_);
  output.normalized_batch_final_chain_digest = current_source_chain_digest_;
  output.ordered_rational_chain_digest = current_rational_chain_digest_;
  output.all_normalized_equal_level_batches_covered = true;
  output.one_record_per_batch_including_omitted_noop = true;
  output.binary64_level_count_zero = true;
  output.bounded_memory_summary_only = true;
  output.manifest_file_bytes_materialized = false;
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
  return session.has_value() && session->certified_ready() &&
         session->record_count() == 0U && required_record_count <=
             session->budget_.maximum_record_count &&
         contract_preflight_certified && budget_preflight_certified &&
         no_manifest_record_arena_materialized &&
         decision == ExactNormalizedHartiganLevelInitializationDecision::
                         complete_certified_empty_manifest_session;
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
  session.budget_ = budget;
  session.expected_total_record_count_ = total;
  session.current_source_chain_digest_ =
      manifest_contract.normalized_batch_initial_chain_digest;
  session.current_rational_chain_digest_ =
      initial_chain_digest(manifest_contract);
  for (std::size_t index = 0U; index < session.summaries_.size(); ++index) {
    session.summaries_[index].order = index + 1U;
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

bool verify_exact_normalized_hartigan_level_record(
    const ExactNormalizedHartiganLevelManifestRecord& record) {
  return record.structurally_certified() && omitted_noop_rule(record.input) &&
         record.rational_chain_digest == record_chain_digest(record);
}

}  // namespace morsehgp3d::hierarchy
