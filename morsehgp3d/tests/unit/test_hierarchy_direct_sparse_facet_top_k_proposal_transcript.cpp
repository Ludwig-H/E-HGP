#include "morsehgp3d/hierarchy/direct_sparse_facet_top_k_proposal_transcript.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

using Budget = ExactDirectSparseFacetTopKProposalTranscriptBudget;
using Decision = ExactDirectSparseFacetTopKProposalTranscriptDecision;
using Metadata = ExactDirectSparseFacetTopKProposalTranscriptMetadata;
using Record = ExactDirectSparseFacetTopKProposalRecord;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::copy(point_ids.begin(), point_ids.end(), result.point_ids.begin());
  return result;
}

[[nodiscard]] Record record(
    std::initializer_list<PointId> source_point_ids,
    std::initializer_list<PointId> candidate_point_ids) {
  Record result;
  result.source_facet_key = key(source_point_ids);
  result.candidate_point_count = candidate_point_ids.size();
  std::copy(
      candidate_point_ids.begin(),
      candidate_point_ids.end(),
      result.candidate_point_ids.begin());
  return result;
}

[[nodiscard]] Metadata metadata() {
  Metadata result;
  result.source_batch_index = 17U;
  result.closed_batch_squared_level = ExactLevel{BigInt{5}, BigInt{2}};
  result.locator_snapshot_stamp.schema_version =
      direct_sparse_positive_facet_locator_schema_version;
  result.locator_snapshot_stamp.external_authority_id = 91U;
  result.locator_snapshot_stamp.committed_batch_count = 3U;
  result.locator_snapshot_stamp.inserted_key_count = 11U;
  result.locator_snapshot_stamp.component_union_count = 4U;
  result.locator_snapshot_stamp.binding_count = 12U;
  return result;
}

[[nodiscard]] Budget exact_budget(std::span<const Record> records) {
  std::size_t key_point_count = 0U;
  std::size_t candidate_point_count = 0U;
  for (const Record& proposal : records) {
    key_point_count += proposal.source_facet_key.point_count;
    candidate_point_count += proposal.candidate_point_count;
  }
  return {
      records.size(),
      key_point_count,
      candidate_point_count,
      records.size() * sizeof(Record),
      records.size() + key_point_count + candidate_point_count,
  };
}

[[nodiscard]] Budget generous_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum};
}

void check_atomic_failure(
    const ExactDirectSparseFacetTopKProposalTranscriptResult& result,
    Decision expected_decision,
    const std::string& message) {
  check(
      result.decision == expected_decision &&
          result.certified_atomic_failure() &&
          result.certified_outcome() &&
          result.proposal_records.empty() &&
          result.published_payload_byte_count == 0U &&
          result.published_logical_storage_entry_count == 0U &&
          result.no_partial_proposal_payload_published &&
          !result.payload_published_only_after_full_validation,
      message);
}

void test_complete_transcript_is_owned_bounded_and_non_authoritative() {
  std::array<Record, 2U> proposals{
      record(
          {1U, 3U, 5U},
          {5U, std::numeric_limits<PointId>::max(), 1U}),
      record({1U, 3U, 6U}, {8U}),
  };
  const Metadata source_metadata = metadata();
  const Budget budget =
      exact_budget(std::span<const Record>{proposals});
  const auto result =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          source_metadata,
          std::span<const Record>{proposals},
          budget);

  check(
      result.complete_proposal_transcript() &&
          result.certified_outcome() &&
          result.decision ==
              Decision::complete_validated_proposal_transcript &&
          result.metadata == source_metadata &&
          result.proposal_records ==
              std::vector<Record>{proposals.begin(), proposals.end()} &&
          result.input_proposal_record_count == 2U &&
          result.required_facet_key_point_reference_count == 6U &&
          result.required_candidate_point_reference_count == 4U &&
          result.required_payload_byte_count == 2U * sizeof(Record) &&
          result.required_logical_storage_entry_count == 12U &&
          result.facet_cardinality == 3U &&
          result.published_payload_byte_count == 2U * sizeof(Record) &&
          result.published_logical_storage_entry_count == 12U,
      "a canonical transcript is owned only after exact cap preflight");
  check(
      result.every_full_key_validated &&
          result.homogeneous_facet_cardinality_validated &&
          result.records_strictly_sorted_by_full_key &&
          result.full_keys_unique && result.candidate_counts_within_k &&
          result.candidate_point_ids_distinct &&
          result.unused_candidate_slots_zero &&
          result.payload_published_only_after_full_validation &&
          result.input_validation_atomic &&
          result.no_partial_proposal_payload_published,
      "the complete transcript records every structural invariant");
  check(
      !result.candidate_point_domain_validated &&
          !result.candidate_exclusions_validated &&
          !result.exact_top_k_partition_certified &&
          !result.scientific_decision_published &&
          !result.locator_state_mutated &&
          !result.hierarchy_reduction_or_attachment_published &&
          !result.forbidden_global_structure_materialized &&
          !result.public_status_claimed && result.proposal_only &&
          ExactDirectSparseFacetTopKProposalTranscriptResult::mode ==
              "proposal_only" &&
          ExactDirectSparseFacetTopKProposalTranscriptResult::
                  deployment_status ==
              "architecture_only",
      "source-key members and out-of-domain candidates remain honest proposals");
}

void test_empty_and_zero_to_k_candidate_populations_are_valid() {
  const std::array<Record, 0U> empty{};
  const Budget empty_budget{};
  const auto empty_result =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          metadata(), std::span<const Record>{empty}, empty_budget);
  check(
      empty_result.complete_proposal_transcript() &&
          empty_result.decision ==
              Decision::complete_empty_proposal_transcript &&
          empty_result.facet_cardinality == 0U &&
          empty_result.proposal_records.empty(),
      "an empty transcript is a complete bounded proposal outcome");

  const std::array<Record, 2U> proposals{
      record({1U, 2U, 3U}, {}),
      record({1U, 2U, 4U}, {9U, 7U, 8U}),
  };
  const auto result =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          metadata(),
          std::span<const Record>{proposals},
          exact_budget(std::span<const Record>{proposals}));
  check(
      result.complete_proposal_transcript() &&
          result.proposal_records.front().candidate_point_count == 0U &&
          result.proposal_records.back().candidate_point_count == 3U &&
          result.proposal_records.back().candidate_point_ids[0U] == 9U &&
          result.proposal_records.back().candidate_point_ids[1U] == 7U,
      "candidate populations cover zero through K without imposing candidate order");
}

void test_every_storage_cap_fails_without_a_partial_payload() {
  const std::array<Record, 2U> proposals{
      record({1U, 3U, 5U}, {8U, 7U}),
      record({1U, 3U, 6U}, {9U}),
  };
  const std::span<const Record> view{proposals};
  const Budget exact = exact_budget(view);

  std::array<Budget, 5U> insufficient{
      exact, exact, exact, exact, exact};
  insufficient[0U].maximum_proposal_record_count -= 1U;
  insufficient[1U].maximum_facet_key_point_reference_count -= 1U;
  insufficient[2U].maximum_candidate_point_reference_count -= 1U;
  insufficient[3U].maximum_payload_byte_count -= 1U;
  insufficient[4U].maximum_logical_storage_entry_count -= 1U;

  for (std::size_t budget_index = 0U;
       budget_index < insufficient.size();
       ++budget_index) {
    const auto result =
        build_exact_direct_sparse_facet_top_k_proposal_transcript(
            metadata(), view, insufficient[budget_index]);
    check_atomic_failure(
        result,
        Decision::no_transcript_budget_exhausted,
        "every transcript cap rejects atomically at index " +
            std::to_string(budget_index));
  }

  Record malformed = record({1U, 3U, 5U}, {7U, 7U});
  Budget zero_scan_budget{};
  const auto bounded_before_shape =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          metadata(),
          std::span<const Record>{&malformed, 1U},
          zero_scan_budget);
  check_atomic_failure(
      bounded_before_shape,
      Decision::no_transcript_budget_exhausted,
      "the record cap bounds validation before scanning malformed excess input");
}

void test_invalid_metadata_is_rejected_atomically() {
  const std::array<Record, 1U> proposals{
      record({1U, 2U}, {3U}),
  };
  Metadata invalid = metadata();
  invalid.locator_snapshot_stamp.external_authority_id = 0U;
  auto result =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          invalid,
          std::span<const Record>{proposals},
          generous_budget());
  check_atomic_failure(
      result,
      Decision::no_transcript_metadata_rejected,
      "a zero locator authority is not valid transcript metadata");

  invalid = metadata();
  invalid.locator_snapshot_stamp.schema_version += 1U;
  result = build_exact_direct_sparse_facet_top_k_proposal_transcript(
      invalid,
      std::span<const Record>{proposals},
      generous_budget());
  check_atomic_failure(
      result,
      Decision::no_transcript_metadata_rejected,
      "a foreign locator-stamp schema is not valid transcript metadata");
}

void test_malformed_records_are_rejected_atomically() {
  std::vector<std::vector<Record>> malformed_cases;

  Record invalid_key = record({1U, 2U}, {4U});
  invalid_key.source_facet_key.point_count = 0U;
  malformed_cases.push_back({invalid_key});

  Record nonzero_key_tail = record({1U, 2U}, {4U});
  nonzero_key_tail.source_facet_key.point_ids.back() = 9U;
  malformed_cases.push_back({nonzero_key_tail});

  malformed_cases.push_back(
      {record({1U, 2U}, {4U}), record({1U, 2U, 3U}, {4U})});
  malformed_cases.push_back(
      {record({1U, 3U}, {4U}), record({1U, 2U}, {4U})});
  malformed_cases.push_back(
      {record({1U, 2U}, {4U}), record({1U, 2U}, {5U})});

  Record too_many_candidates = record({1U, 2U}, {4U, 5U, 6U});
  malformed_cases.push_back({too_many_candidates});

  Record duplicate_candidates = record({1U, 2U}, {4U, 4U});
  malformed_cases.push_back({duplicate_candidates});

  Record nonzero_candidate_tail = record({1U, 2U}, {4U});
  nonzero_candidate_tail.candidate_point_ids.back() = 9U;
  malformed_cases.push_back({nonzero_candidate_tail});

  for (std::size_t case_index = 0U;
       case_index < malformed_cases.size();
       ++case_index) {
    const auto& records = malformed_cases[case_index];
    const auto result =
        build_exact_direct_sparse_facet_top_k_proposal_transcript(
            metadata(),
            std::span<const Record>{records},
            generous_budget());
    check_atomic_failure(
        result,
        Decision::no_transcript_input_shape_rejected,
        "malformed full-key or candidate shape rejects atomically at index " +
            std::to_string(case_index));
  }
}

void test_self_check_detects_payload_or_authority_promotion_mutations() {
  const std::array<Record, 1U> proposals{
      record({1U, 2U}, {4U}),
  };
  const Budget budget =
      exact_budget(std::span<const Record>{proposals});
  const auto source =
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          metadata(), std::span<const Record>{proposals}, budget);
  check(
      source.complete_proposal_transcript(),
      "the mutation fixture starts from a complete transcript");

  auto mutated = source;
  mutated.candidate_point_domain_validated = true;
  check(
      !mutated.certified_outcome(),
      "the transcript rejects an invented candidate-domain validation");

  mutated = source;
  mutated.proposal_records.front().source_facet_key.point_ids[0U] = 2U;
  check(
      !mutated.certified_outcome(),
      "the transcript self-check rejects a noncanonical retained full key");

  mutated = source;
  mutated.published_payload_byte_count -= 1U;
  check(
      !mutated.certified_outcome(),
      "the transcript self-check rejects a forged physical payload count");

  mutated = source;
  mutated.requested_budget.maximum_proposal_record_count = 0U;
  check(
      !mutated.certified_outcome(),
      "the transcript self-check rejects a narrowed retained record cap");
}

}  // namespace

int main() {
  test_complete_transcript_is_owned_bounded_and_non_authoritative();
  test_empty_and_zero_to_k_candidate_populations_are_valid();
  test_every_storage_cap_fails_without_a_partial_payload();
  test_invalid_metadata_is_rejected_atomically();
  test_malformed_records_are_rejected_atomically();
  test_self_check_detects_payload_or_authority_promotion_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse facet top-k proposal transcript test(s) failed\n";
    return 1;
  }
  std::cout
      << "direct sparse facet top-k proposal transcript tests passed\n";
  return 0;
}
