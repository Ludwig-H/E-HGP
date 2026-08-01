#include "morsehgp3d/hierarchy/normalized_exact_hartigan_level_manifest.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CanonicalId id(char digit) {
  return CanonicalId::from_lower_hex(std::string(64U, digit));
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactNormalizedHartiganLevelManifestContract contract() {
  ExactNormalizedHartiganLevelManifestContract value;
  value.authority_id = 41U;
  value.maximum_order = 2U;
  value.expected_record_count_by_order[0] = 2U;
  value.expected_record_count_by_order[1] = 1U;
  value.expected_omitted_noop_count_by_order[0] = 1U;
  value.normalized_source_scientific_digest = id('9');
  value.normalized_batch_initial_chain_digest = id('a');
  value.normalized_batch_final_chain_digest = id('d');
  return value;
}

[[nodiscard]] ExactNormalizedHartiganLevelInput input(
    std::size_t batch,
    std::size_t order,
    std::size_t order_batch,
    ExactLevel squared_level,
    std::size_t q_r,
    const CanonicalId& previous_source,
    const CanonicalId& source,
    ExactNormalizedHartiganBatchDisposition disposition =
        ExactNormalizedHartiganBatchDisposition::
            materialized_normalized_equal_level_batch) {
  ExactNormalizedHartiganLevelInput value;
  value.normalized_batch_index = batch;
  value.order = order;
  value.order_batch_index = order_batch;
  value.squared_level = std::move(squared_level);
  value.q_r = q_r;
  value.previous_normalized_batch_chain_digest = previous_source;
  value.normalized_batch_chain_digest = source;
  value.disposition = disposition;
  if (disposition == ExactNormalizedHartiganBatchDisposition::
                         materialized_normalized_equal_level_batch) {
    value.core_facet_delta_count = 1U;
    value.point_delta_count = 2U;
    value.parent_delta_count = 1U;
    value.node_delta_count = 1U;
  }
  return value;
}

void test_closed_record_chain_and_failure_atomicity() {
  const auto source_contract = contract();
  const ExactNormalizedHartiganLevelManifestBudget budget{3U, 8U};
  auto initialized = initialize_exact_normalized_hartigan_level_manifest_session(
      source_contract, budget);
  check(
      initialized.certified_initialized_session(),
      "a bounded three-record session initializes");
  auto session = std::move(*initialized.session);
  const CanonicalId initial_rational_chain =
      session.current_rational_chain_digest();

  auto zero_qr = input(0U, 1U, 0U, level(1, 3), 0U, id('a'), id('b'));
  const auto rejected_zero_qr = session.append(zero_qr);
  check(
      !rejected_zero_qr.certified_appended_record() &&
          rejected_zero_qr.no_state_mutated_on_failure &&
          session.record_count() == 0U &&
          session.current_rational_chain_digest() == initial_rational_chain,
      "q_R zero is rejected atomically");

  auto first_input = input(0U, 1U, 0U, level(1, 3), 2U, id('a'), id('b'));
  const auto first = session.append(first_input);
  check(first.certified_appended_record(), "first rational record appends");
  check(
      verify_exact_normalized_hartigan_level_record(*first.record),
      "first rational record independently verifies");
  check(
      first.record->canonical_json().find("\"numerator\":\"1\"") !=
          std::string::npos,
      "canonical JSON preserves the exact numerator as decimal text");
  auto tampered = *first.record;
  tampered.input.q_r = 3U;
  check(
      !verify_exact_normalized_hartigan_level_record(tampered),
      "record-chain verification rejects scientific mutation");

  auto stale = input(1U, 1U, 1U, level(2, 3), 1U, id('a'), id('c'),
      ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop);
  const CanonicalId after_first = session.current_rational_chain_digest();
  const auto rejected_stale = session.append(stale);
  check(
      !rejected_stale.certified_appended_record() &&
          rejected_stale.no_state_mutated_on_failure &&
          session.record_count() == 1U &&
          session.current_rational_chain_digest() == after_first,
      "stale normalized-source chain is rejected atomically");

  auto invalid_noop = input(1U, 1U, 1U, level(2, 3), 1U, id('b'), id('c'),
      ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop);
  invalid_noop.point_delta_count = 1U;
  const auto rejected_noop = session.append(invalid_noop);
  check(
      !rejected_noop.certified_appended_record() &&
          rejected_noop.no_state_mutated_on_failure &&
          session.record_count() == 1U,
      "a claimed q_R=1 no-op with a scientific delta is rejected");

  auto second_input = input(1U, 1U, 1U, level(2, 3), 1U, id('b'), id('c'),
      ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop);
  const auto second = session.append(second_input);
  check(
      second.certified_appended_record(),
      "certified q_R=1 zero-delta no-op retains its exact level record");

  auto duplicate_level_order =
      input(2U, 1U, 2U, level(2, 3), 2U, id('c'), id('d'));
  const auto rejected_duplicate_level_order =
      session.append(duplicate_level_order);
  check(
      !rejected_duplicate_level_order.certified_appended_record() &&
          rejected_duplicate_level_order.no_state_mutated_on_failure &&
          session.record_count() == 2U,
      "a duplicate exact (level, order) source key is rejected");

  auto decreasing = input(2U, 2U, 0U, level(1, 2), 2U, id('c'), id('d'));
  const CanonicalId after_second = session.current_rational_chain_digest();
  const auto rejected_decreasing = session.append(decreasing);
  check(
      !rejected_decreasing.certified_appended_record() &&
          rejected_decreasing.no_state_mutated_on_failure &&
          session.record_count() == 2U &&
          session.current_rational_chain_digest() == after_second,
      "decreasing global exact Hartigan chronology is rejected atomically");

  auto third_input = input(2U, 2U, 0U, level(2, 3), 2U, id('c'), id('d'));
  const auto third = session.append(third_input);
  check(third.certified_appended_record(), "third rational record appends");

  const auto seal = session.seal();
  check(seal.certified_seal(), "complete manifest seals");
  check(
      seal.manifest_record_count == 3U &&
          seal.records_by_order[0].rational_record_count == 2U &&
          seal.records_by_order[0]
                  .normalized_omitted_noop_qr1_batch_count == 1U &&
          seal.records_by_order[0].first_level == level(1, 3) &&
          seal.records_by_order[0].last_level == level(2, 3) &&
          seal.records_by_order[1].first_level == level(2, 3) &&
          seal.ordered_rational_chain_digest ==
              third.record->rational_chain_digest,
      "seal retains exact per-order boundaries and ordered chain root");
  const auto rejected_after_seal = session.append(third_input);
  check(
      !rejected_after_seal.certified_appended_record() &&
          rejected_after_seal.no_state_mutated_on_failure,
      "sealed session rejects further records");
}

void test_budget_preflight_and_budget_independent_identity() {
  const auto source_contract = contract();
  const auto insufficient =
      initialize_exact_normalized_hartigan_level_manifest_session(
          source_contract, {2U, 8U});
  check(
      !insufficient.certified_initialized_session() &&
          insufficient.required_record_count == 3U &&
          insufficient.contract_preflight_certified &&
          !insufficient.budget_preflight_certified,
      "record cap is checked before a session is exposed");

  const auto tight = initialize_exact_normalized_hartigan_level_manifest_session(
      source_contract, {3U, 8U});
  auto fresh_process_contract = source_contract;
  fresh_process_contract.authority_id = 99U;
  const auto loose = initialize_exact_normalized_hartigan_level_manifest_session(
      fresh_process_contract, {30U, 32U});
  check(
      tight.certified_initialized_session() &&
          loose.certified_initialized_session() &&
          tight.session->current_rational_chain_digest() ==
              loose.session->current_rational_chain_digest(),
      "scientific manifest identity is independent of resource ceilings and "
      "process-local session authority");

  auto one_record_contract = source_contract;
  one_record_contract.expected_record_count_by_order = {};
  one_record_contract.expected_omitted_noop_count_by_order = {};
  one_record_contract.expected_record_count_by_order[0] = 1U;
  one_record_contract.normalized_batch_final_chain_digest = id('b');
  auto digit_limited =
      initialize_exact_normalized_hartigan_level_manifest_session(
          one_record_contract, {1U, 1U});
  check(
      digit_limited.certified_initialized_session(),
      "decimal digit limit initializes independently of payload values");
  auto digit_session = std::move(*digit_limited.session);
  const auto too_wide = digit_session.append(
      input(0U, 1U, 0U, level(10), 2U, id('a'), id('b')));
  check(
      !too_wide.certified_appended_record() &&
          too_wide.no_state_mutated_on_failure &&
          digit_session.record_count() == 0U,
      "oversized exact decimal payload is rejected before state mutation");
}

}  // namespace

int main() {
  test_closed_record_chain_and_failure_atomicity();
  test_budget_preflight_and_budget_independent_identity();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "normalized exact Hartigan manifest tests passed\n";
  return 0;
}
