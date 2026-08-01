#include "morsehgp3d/hierarchy/normalized_exact_hartigan_level_manifest.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;

int failures = 0;

static_assert(normalized_exact_hartigan_level_manifest_schema_version == 4U);
static_assert(
    static_cast<std::uint8_t>(
        ExactNormalizedHartiganLevelAppendDecision::
            complete_certified_record_appended) == 9U);
static_assert(
    static_cast<std::uint8_t>(
        ExactNormalizedHartiganLevelSealDecision::
            complete_certified_manifest_seal) == 4U);
static_assert(
    static_cast<std::uint8_t>(
        ExactNormalizedHartiganLevelInitializationDecision::
            complete_certified_empty_manifest_session) == 3U);
static_assert(
    !std::is_copy_constructible_v<
        ExactNormalizedHartiganLevelPreparedAppend>);
static_assert(
    !std::is_copy_assignable_v<
        ExactNormalizedHartiganLevelPreparedAppend>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactNormalizedHartiganLevelPreparedAppend>);
static_assert(
    std::is_nothrow_move_assignable_v<
        ExactNormalizedHartiganLevelPreparedAppend>);
static_assert(
    !std::is_copy_constructible_v<
        ExactNormalizedHartiganStreamingClosureAttestation>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactNormalizedHartiganStreamingClosureAttestation>);
static_assert(
    !normalized_exact_hartigan_level_manifest_product_complete);
static_assert(
    !normalized_exact_hartigan_level_manifest_vertical_maps_complete);
static_assert(
    !normalized_exact_hartigan_level_manifest_global_gamma_star_or_delaunay_materialized);
static_assert(noexcept(
    std::declval<ExactNormalizedHartiganLevelManifestSession&>().commit(
        std::declval<ExactNormalizedHartiganLevelPreparedAppend&&>())));
static_assert(noexcept(
    std::declval<const ExactNormalizedHartiganLevelManifestSession&>()
        .make_checkpoint()));

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
  value.group_count = 1U;
  value.qr0_group_count = q_r == 0U ? 1U : 0U;
  value.qr1_group_count = q_r == 1U ? 1U : 0U;
  value.qr_greater_equal2_group_count = q_r >= 2U ? 1U : 0U;
  value.group_summary_encoding =
      ExactNormalizedHartiganGroupSummaryEncoding::
          complete_batch_qr_partition_v4;
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

  auto unspecified_group_summary =
      input(0U, 1U, 0U, level(1, 3), 0U, id('a'), id('b'));
  unspecified_group_summary.group_count = 0U;
  unspecified_group_summary.qr0_group_count = 0U;
  unspecified_group_summary.group_summary_encoding =
      ExactNormalizedHartiganGroupSummaryEncoding::not_certified;
  const auto rejected_unspecified_group_summary =
      session.append(unspecified_group_summary);
  check(
      !rejected_unspecified_group_summary.certified_appended_record() &&
          rejected_unspecified_group_summary.no_state_mutated_on_failure &&
          rejected_unspecified_group_summary.decision ==
              ExactNormalizedHartiganLevelAppendDecision::
                  no_record_group_summary_rejected &&
          session.record_count() == 0U &&
          session.current_rational_chain_digest() == initial_rational_chain,
      "an unspecified scalar-shaped group summary fails closed atomically");

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

  const auto invalid_qr0_noop = input(
      1U,
      1U,
      1U,
      level(2, 3),
      0U,
      id('b'),
      id('c'),
      ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop);
  const auto rejected_qr0_noop = session.append(invalid_qr0_noop);
  check(
      !rejected_qr0_noop.certified_appended_record() &&
          rejected_qr0_noop.no_state_mutated_on_failure &&
          rejected_qr0_noop.decision ==
              ExactNormalizedHartiganLevelAppendDecision::
                  no_record_omitted_noop_rule_rejected &&
          session.record_count() == 1U,
      "an omitted batch remains exactly one q_R=1 group");

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

void test_v4_batch_group_partition_and_explicit_legacy_separation() {
  auto one_record_contract = contract();
  one_record_contract.expected_record_count_by_order = {};
  one_record_contract.expected_omitted_noop_count_by_order = {};
  one_record_contract.expected_record_count_by_order[0] = 1U;
  one_record_contract.normalized_batch_final_chain_digest = id('b');

  auto v4_initialized =
      initialize_exact_normalized_hartigan_level_manifest_session(
          one_record_contract, {1U, 8U});
  auto v4_session = std::move(*v4_initialized.session);
  auto mixed_batch =
      input(0U, 1U, 0U, level(1, 3), 0U, id('a'), id('b'));
  mixed_batch.group_count = 3U;
  mixed_batch.qr0_group_count = 1U;
  mixed_batch.qr1_group_count = 1U;
  mixed_batch.qr_greater_equal2_group_count = 1U;
  const auto mixed_record = v4_session.append(mixed_batch);
  check(
      mixed_record.certified_appended_record() &&
          mixed_record.record->complete_batch_qr_partition_certified &&
          !mixed_record.record
               ->legacy_single_group_scalar_qr_compatibility_used &&
          mixed_record.record->input.q_r == 0U,
      "v4 materializes a complete mixed batch including a q_R=0 birth");
  const auto v4_seal = v4_session.seal();
  check(
      v4_seal.certified_seal() &&
          v4_seal.every_record_uses_complete_batch_qr_partition &&
          !v4_seal.legacy_single_group_scalar_qr_compatibility_used &&
          v4_seal.records_by_order[0].normalized_group_count == 3U &&
          v4_seal.records_by_order[0].normalized_qr0_group_count == 1U &&
          v4_seal.records_by_order[0].normalized_qr1_group_count == 1U &&
          v4_seal.records_by_order[0]
                  .normalized_qr_greater_equal2_group_count == 1U,
      "v4 seal retains the exact per-batch q_R partition summary");

  auto legacy_initialized =
      initialize_exact_normalized_hartigan_level_manifest_session(
          one_record_contract, {1U, 8U});
  auto legacy_session = std::move(*legacy_initialized.session);
  auto legacy_input =
      input(0U, 1U, 0U, level(1, 3), 2U, id('a'), id('b'));
  legacy_input.q_r = 2U;
  legacy_input.group_count = 0U;
  legacy_input.qr0_group_count = 0U;
  legacy_input.qr1_group_count = 0U;
  legacy_input.qr_greater_equal2_group_count = 0U;
  legacy_input.group_summary_encoding =
      ExactNormalizedHartiganGroupSummaryEncoding::
          legacy_single_group_scalar_qr_v3;
  const auto legacy_record = legacy_session.append(legacy_input);
  const auto legacy_seal = legacy_session.seal();
  check(
      legacy_record.certified_appended_record() &&
          !legacy_record.record->complete_batch_qr_partition_certified &&
          legacy_record.record
              ->legacy_single_group_scalar_qr_compatibility_used &&
          legacy_seal.certified_seal() &&
          !legacy_seal.every_record_uses_complete_batch_qr_partition &&
          legacy_seal.legacy_single_group_scalar_qr_compatibility_used &&
          legacy_seal.records_by_order[0]
                  .legacy_single_group_scalar_qr_record_count == 1U,
      "legacy v3 scalar input is accepted only as an explicitly marked "
      "single-group compatibility record");
}

void test_move_only_prepare_and_allocation_free_commit_seam() {
  const auto source_contract = contract();
  const ExactNormalizedHartiganLevelManifestBudget budget{3U, 8U};
  auto initialized = initialize_exact_normalized_hartigan_level_manifest_session(
      source_contract, budget);
  auto session = std::move(*initialized.session);
  auto foreign_contract = source_contract;
  foreign_contract.authority_id = 42U;
  auto foreign_initialized =
      initialize_exact_normalized_hartigan_level_manifest_session(
          foreign_contract, budget);
  auto foreign_session = std::move(*foreign_initialized.session);
  const CanonicalId initial_rational_chain =
      session.current_rational_chain_digest();
  const auto first_input =
      input(0U, 1U, 0U, level(1, 3), 2U, id('a'), id('b'));

  auto prepared = session.prepare_append(first_input);
  check(
      prepared.certified_prepared_append() &&
          prepared.ticket->prepared_record().record_index == 0U &&
          prepared.ticket->prepared_record().input == first_input &&
          session.record_count() == 0U &&
          session.current_rational_chain_digest() == initial_rational_chain,
      "prepare owns the exact record and digest without scientific mutation");
  const auto rejected_parallel = session.prepare_append(first_input);
  check(
      !rejected_parallel.certified_prepared_append() &&
          rejected_parallel.no_scientific_state_mutated &&
          rejected_parallel.decision ==
              ExactNormalizedHartiganLevelAppendDecision::
                  no_outstanding_prepared_append_budget_exhausted &&
          session.record_count() == 0U,
      "the bounded seam admits exactly one outstanding prepared append");
  const auto blocked_checkpoint = session.make_checkpoint();
  const auto blocked_seal = session.seal();
  check(
      !blocked_checkpoint.certified_semantic_checkpoint() &&
          blocked_checkpoint.decision ==
              ExactNormalizedHartiganLevelCheckpointDecision::
                  no_outstanding_prepared_append &&
          !blocked_seal.certified_seal() &&
          blocked_seal.decision ==
              ExactNormalizedHartiganLevelSealDecision::
                  no_outstanding_prepared_append_rejected,
      "checkpoint and seal reject an outstanding scientific transaction");

  auto foreign_ticket = std::move(*prepared.ticket);
  check(
      foreign_ticket.valid() && prepared.ticket->consumed(),
      "the opaque ticket moves without copying its authority");
  const auto rejected_foreign =
      foreign_session.commit(std::move(foreign_ticket));
  check(
      !rejected_foreign.certified_appended_record() &&
          rejected_foreign.no_state_mutated_on_failure &&
          rejected_foreign.decision ==
              ExactNormalizedHartiganLevelAppendDecision::
                  no_foreign_prepared_append_rejected &&
          foreign_ticket.consumed() && session.record_count() == 0U &&
          foreign_session.record_count() == 0U,
      "a same-science foreign session cannot forge the private ticket");

  auto retry = session.prepare_append(first_input);
  check(
      retry.certified_prepared_append(),
      "consuming a foreign rejection releases the issuing session slot");
  auto ticket = std::move(*retry.ticket);
  const auto committed = session.commit(std::move(ticket));
  check(
      committed.certified_appended_record() && ticket.consumed() &&
          session.record_count() == 1U,
      "commit publishes the fully prepared record atomically");
  const CanonicalId after_first = session.current_rational_chain_digest();
  const auto rejected_reuse = session.commit(std::move(ticket));
  check(
      !rejected_reuse.certified_appended_record() &&
          rejected_reuse.no_state_mutated_on_failure &&
          rejected_reuse.decision ==
              ExactNormalizedHartiganLevelAppendDecision::
                  no_prepared_append_already_consumed &&
          session.record_count() == 1U &&
          session.current_rational_chain_digest() == after_first,
      "a consumed move-only ticket cannot publish twice");

  {
    auto abandoned = session.prepare_append(input(
        1U,
        1U,
        1U,
        level(2, 3),
        1U,
        id('b'),
        id('c'),
        ExactNormalizedHartiganBatchDisposition::
            omitted_certified_qr1_noop));
    check(
        abandoned.certified_prepared_append() &&
            session.record_count() == 1U,
        "a prepared suffix remains unpublished until commit");
  }
  const auto second = session.append(input(
      1U,
      1U,
      1U,
      level(2, 3),
      1U,
      id('b'),
      id('c'),
      ExactNormalizedHartiganBatchDisposition::
          omitted_certified_qr1_noop));
  check(
      second.certified_appended_record() && session.record_count() == 2U,
      "destroying an uncommitted ticket releases capacity without mutation");
}

void test_semantic_checkpoint_fresh_process_restore() {
  const auto source_contract = contract();
  auto initialized = initialize_exact_normalized_hartigan_level_manifest_session(
      source_contract, {3U, 8U});
  auto uninterrupted = std::move(*initialized.session);
  const auto first = uninterrupted.append(
      input(0U, 1U, 0U, level(1, 3), 2U, id('a'), id('b')));
  check(first.certified_appended_record(), "checkpoint source prefix appends");

  const auto exported = uninterrupted.make_checkpoint();
  check(
      exported.certified_semantic_checkpoint() &&
          exported.checkpoint->contract_mode ==
              ExactNormalizedHartiganManifestContractMode::
                  fixed_predeclared_contract &&
          exported.checkpoint->record_count == 1U &&
          !exported.checkpoint->process_local_authority_serialized &&
          !exported.checkpoint->resource_budget_serialized &&
          !exported.checkpoint->manifest_record_arena_materialized &&
          !exported.checkpoint->public_status_claimed,
      "checkpoint contains only bounded scientific summary state");

  auto checkpoint_for_restore = *exported.checkpoint;
  auto restored = restore_exact_normalized_hartigan_level_manifest_session(
      std::move(checkpoint_for_restore), 99U, {30U, 32U});
  check(
      restored.certified_initialized_session() &&
          restored.checkpoint_freshly_verified &&
          restored.session->record_count() == 1U &&
          restored.session->current_rational_chain_digest() ==
              uninterrupted.current_rational_chain_digest(),
      "fresh-process restore remints authority after semantic verification");
  auto resumed = std::move(*restored.session);
  const auto reexported = resumed.make_checkpoint();
  check(
      reexported.certified_semantic_checkpoint() &&
          *reexported.checkpoint == *exported.checkpoint,
      "checkpoint identity is independent of process authority and budget");

  const auto second_input = input(
      1U,
      1U,
      1U,
      level(2, 3),
      1U,
      id('b'),
      id('c'),
      ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop);
  const auto third_input =
      input(2U, 2U, 0U, level(2, 3), 2U, id('c'), id('d'));
  const auto uninterrupted_second = uninterrupted.append(second_input);
  const auto resumed_second = resumed.append(second_input);
  const auto uninterrupted_third = uninterrupted.append(third_input);
  const auto resumed_third = resumed.append(third_input);
  check(
      uninterrupted_second.certified_appended_record() &&
          resumed_second.certified_appended_record() &&
          uninterrupted_third.certified_appended_record() &&
          resumed_third.certified_appended_record() &&
          uninterrupted_second.record == resumed_second.record &&
          uninterrupted_third.record == resumed_third.record,
      "resumed and uninterrupted exact rational record suffixes are identical");
  const auto uninterrupted_seal = uninterrupted.seal();
  const auto resumed_seal = resumed.seal();
  check(
      uninterrupted_seal.certified_seal() && resumed_seal.certified_seal() &&
          uninterrupted_seal.authority_id == 41U &&
          resumed_seal.authority_id == 99U &&
          uninterrupted_seal.records_by_order ==
              resumed_seal.records_by_order &&
          uninterrupted_seal.normalized_batch_final_chain_digest ==
              resumed_seal.normalized_batch_final_chain_digest &&
          uninterrupted_seal.ordered_rational_chain_digest ==
              resumed_seal.ordered_rational_chain_digest,
      "fresh-process continuation preserves all scientific seal fields");

  auto digest_tampered = *exported.checkpoint;
  digest_tampered.current_rational_chain_digest = id('8');
  const auto rejected_digest =
      restore_exact_normalized_hartigan_level_manifest_session(
          std::move(digest_tampered), 100U, {3U, 8U});
  check(
      !rejected_digest.certified_initialized_session() &&
          rejected_digest.decision ==
              ExactNormalizedHartiganLevelInitializationDecision::
                  no_checkpoint_digest_rejected &&
          !rejected_digest.checkpoint_freshly_verified,
      "fresh verification rejects a tampered scientific chain root");

  auto shape_tampered = *exported.checkpoint;
  shape_tampered.record_count = 2U;
  const auto rejected_shape =
      restore_exact_normalized_hartigan_level_manifest_session(
          std::move(shape_tampered), 100U, {3U, 8U});
  check(
      !rejected_shape.certified_initialized_session() &&
          rejected_shape.decision ==
              ExactNormalizedHartiganLevelInitializationDecision::
                  no_checkpoint_shape_rejected,
      "fresh verification rejects an inconsistent checkpoint shape");

  auto insufficient_budget_checkpoint = *exported.checkpoint;
  const auto rejected_budget =
      restore_exact_normalized_hartigan_level_manifest_session(
          std::move(insufficient_budget_checkpoint), 100U, {2U, 8U});
  check(
      !rejected_budget.certified_initialized_session() &&
          rejected_budget.decision ==
              ExactNormalizedHartiganLevelInitializationDecision::
                  no_session_budget_rejected,
      "restore remains bounded by the fresh process resource budget");
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
  test_v4_batch_group_partition_and_explicit_legacy_separation();
  test_move_only_prepare_and_allocation_free_commit_seam();
  test_semantic_checkpoint_fresh_process_restore();
  test_budget_preflight_and_budget_independent_identity();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "normalized exact Hartigan manifest tests passed\n";
  return 0;
}
