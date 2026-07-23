#include "morsehgp3d/hierarchy/direct_sparse_facet_top_k_proposal_transcript.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using Decision = ExactDirectSparseFacetTopKProposalTranscriptDecision;
using Record = ExactDirectSparseFacetTopKProposalRecord;
using Result = ExactDirectSparseFacetTopKProposalTranscriptResult;
using Scope = ExactDirectSparseFacetTopKProposalTranscriptScope;

[[nodiscard]] bool try_add_size(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply_size(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t point_index = 0U;
       point_index < key.point_count;
       ++point_index) {
    if (point_index != 0U &&
        key.point_ids[point_index - 1U] >= key.point_ids[point_index]) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] bool full_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.begin() +
          static_cast<std::ptrdiff_t>(left.point_count),
      right.point_ids.begin(),
      right.point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.point_count));
}

[[nodiscard]] bool candidates_are_distinct(
    const Record& record) noexcept {
  for (std::size_t right = 0U;
       right < record.candidate_point_count;
       ++right) {
    for (std::size_t left = 0U; left < right; ++left) {
      if (record.candidate_point_ids[left] ==
          record.candidate_point_ids[right]) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool unused_candidate_slots_are_zero(
    const Record& record) noexcept {
  return std::all_of(
      record.candidate_point_ids.begin() +
          static_cast<std::ptrdiff_t>(record.candidate_point_count),
      record.candidate_point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] bool metadata_shape_valid(
    const ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata)
    noexcept {
  return metadata.locator_snapshot_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         metadata.locator_snapshot_stamp.external_authority_id != 0U;
}

void initialize_non_authoritative_scope(Result& result) noexcept {
  result.scope = Scope::
      bounded_structurally_validated_incumbent_proposals_by_full_facet_key_only;
  result.input_validation_atomic = true;
  result.no_partial_proposal_payload_published = true;
  result.candidate_point_domain_validated = false;
  result.candidate_exclusions_validated = false;
  result.exact_top_k_partition_certified = false;
  result.scientific_decision_published = false;
  result.locator_state_mutated = false;
  result.hierarchy_reduction_or_attachment_published = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.proposal_only = true;
}

void clear_payload(Result& result) noexcept {
  result.proposal_records.clear();
  result.published_payload_byte_count = 0U;
  result.published_logical_storage_entry_count = 0U;
  result.payload_published_only_after_full_validation = false;
  result.no_partial_proposal_payload_published = true;
}

[[nodiscard]] Result fail(Result result, Decision decision) {
  clear_payload(result);
  result.decision = decision;
  return result;
}

struct ObservedShape {
  std::size_t key_point_reference_count{};
  std::size_t candidate_point_reference_count{};
  std::size_t payload_byte_count{};
  std::size_t logical_storage_entry_count{};
  std::size_t facet_cardinality{};
};

[[nodiscard]] bool observe_complete_shape(
    std::span<const Record> records,
    ObservedShape& shape) noexcept {
  if (!try_multiply_size(
          records.size(), sizeof(Record), shape.payload_byte_count)) {
    return false;
  }
  for (std::size_t record_index = 0U;
       record_index < records.size();
       ++record_index) {
    const Record& record = records[record_index];
    if (!canonical_key_shape(record.source_facet_key)) {
      return false;
    }
    if (record_index == 0U) {
      shape.facet_cardinality = record.source_facet_key.point_count;
    } else if (
        record.source_facet_key.point_count != shape.facet_cardinality ||
        !full_key_less(
            records[record_index - 1U].source_facet_key,
            record.source_facet_key)) {
      return false;
    }
    if (record.candidate_point_count >
            record.source_facet_key.point_count ||
        record.candidate_point_count >
            record.candidate_point_ids.size() ||
        !candidates_are_distinct(record) ||
        !unused_candidate_slots_are_zero(record) ||
        !try_add_size(
            shape.key_point_reference_count,
            record.source_facet_key.point_count,
            shape.key_point_reference_count) ||
        !try_add_size(
            shape.candidate_point_reference_count,
            record.candidate_point_count,
            shape.candidate_point_reference_count)) {
      return false;
    }
  }
  if (!try_add_size(
          records.size(),
          shape.key_point_reference_count,
          shape.logical_storage_entry_count) ||
      !try_add_size(
          shape.logical_storage_entry_count,
          shape.candidate_point_reference_count,
          shape.logical_storage_entry_count)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool non_authoritative_scope_honest(
    const Result& result) noexcept {
  return result.input_validation_atomic &&
         result.no_partial_proposal_payload_published &&
         !result.candidate_point_domain_validated &&
         !result.candidate_exclusions_validated &&
         !result.exact_top_k_partition_certified &&
         !result.scientific_decision_published &&
         !result.locator_state_mutated &&
         !result.hierarchy_reduction_or_attachment_published &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.proposal_only &&
         result.scope ==
             Scope::
                 bounded_structurally_validated_incumbent_proposals_by_full_facet_key_only;
}

[[nodiscard]] bool complete_flags(const Result& result) noexcept {
  return result.metadata_shape_validated &&
         result.budget_preflight_completed &&
         result.budget_preflight_satisfied &&
         result.every_full_key_validated &&
         result.homogeneous_facet_cardinality_validated &&
         result.records_strictly_sorted_by_full_key &&
         result.full_keys_unique && result.candidate_counts_within_k &&
         result.candidate_point_ids_distinct &&
         result.unused_candidate_slots_zero &&
         result.payload_published_only_after_full_validation &&
         non_authoritative_scope_honest(result);
}

[[nodiscard]] bool complete_storage_shape(
    const Result& result) noexcept {
  if (result.proposal_records.size() >
      result.requested_budget.maximum_proposal_record_count) {
    return false;
  }
  ObservedShape shape;
  if (!observe_complete_shape(result.proposal_records, shape)) {
    return false;
  }
  return result.input_proposal_record_count ==
             result.proposal_records.size() &&
         result.required_facet_key_point_reference_count ==
             shape.key_point_reference_count &&
         result.required_candidate_point_reference_count ==
             shape.candidate_point_reference_count &&
         result.required_payload_byte_count == shape.payload_byte_count &&
         result.required_logical_storage_entry_count ==
             shape.logical_storage_entry_count &&
         result.facet_cardinality == shape.facet_cardinality &&
         result.published_payload_byte_count == shape.payload_byte_count &&
         result.published_logical_storage_entry_count ==
             shape.logical_storage_entry_count &&
         shape.key_point_reference_count <=
             result.requested_budget
                 .maximum_facet_key_point_reference_count &&
         shape.candidate_point_reference_count <=
             result.requested_budget
                 .maximum_candidate_point_reference_count &&
         shape.payload_byte_count <=
             result.requested_budget.maximum_payload_byte_count &&
         shape.logical_storage_entry_count <=
             result.requested_budget.maximum_logical_storage_entry_count;
}

}  // namespace

bool ExactDirectSparseFacetTopKProposalTranscriptResult::
    complete_proposal_transcript() const noexcept {
  const bool empty = proposal_records.empty();
  const bool decision_matches =
      decision ==
          (empty ? Decision::complete_empty_proposal_transcript
                 : Decision::complete_validated_proposal_transcript);
  return schema_version ==
             direct_sparse_facet_top_k_proposal_transcript_schema_version &&
         decision_matches && complete_flags(*this) &&
         complete_storage_shape(*this);
}

bool ExactDirectSparseFacetTopKProposalTranscriptResult::
    certified_atomic_failure() const noexcept {
  const bool failure_decision =
      decision == Decision::no_transcript_metadata_rejected ||
      decision == Decision::no_transcript_input_shape_rejected ||
      decision == Decision::no_transcript_capacity_overflow ||
      decision == Decision::no_transcript_budget_exhausted;
  return schema_version ==
             direct_sparse_facet_top_k_proposal_transcript_schema_version &&
         failure_decision && proposal_records.empty() &&
         published_payload_byte_count == 0U &&
         published_logical_storage_entry_count == 0U &&
         !payload_published_only_after_full_validation &&
         non_authoritative_scope_honest(*this);
}

bool ExactDirectSparseFacetTopKProposalTranscriptResult::
    certified_outcome() const noexcept {
  return complete_proposal_transcript() || certified_atomic_failure();
}

ExactDirectSparseFacetTopKProposalTranscriptResult
build_exact_direct_sparse_facet_top_k_proposal_transcript(
    const ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata,
    std::span<const ExactDirectSparseFacetTopKProposalRecord>
        proposal_records,
    const ExactDirectSparseFacetTopKProposalTranscriptBudget& budget) {
  Result result;
  result.requested_budget = budget;
  result.metadata = metadata;
  result.input_proposal_record_count = proposal_records.size();
  initialize_non_authoritative_scope(result);

  result.metadata_shape_validated = metadata_shape_valid(metadata);
  if (!result.metadata_shape_validated) {
    return fail(result, Decision::no_transcript_metadata_rejected);
  }

  result.budget_preflight_completed = true;
  const std::vector<Record> empty_vector;
  if (proposal_records.size() > empty_vector.max_size()) {
    return fail(result, Decision::no_transcript_capacity_overflow);
  }
  if (proposal_records.size() >
      budget.maximum_proposal_record_count) {
    return fail(result, Decision::no_transcript_budget_exhausted);
  }

  ObservedShape shape;
  if (!observe_complete_shape(proposal_records, shape)) {
    return fail(result, Decision::no_transcript_input_shape_rejected);
  }
  result.required_facet_key_point_reference_count =
      shape.key_point_reference_count;
  result.required_candidate_point_reference_count =
      shape.candidate_point_reference_count;
  result.required_payload_byte_count = shape.payload_byte_count;
  result.required_logical_storage_entry_count =
      shape.logical_storage_entry_count;
  result.facet_cardinality = shape.facet_cardinality;

  if (shape.key_point_reference_count >
          budget.maximum_facet_key_point_reference_count ||
      shape.candidate_point_reference_count >
          budget.maximum_candidate_point_reference_count ||
      shape.payload_byte_count >
          budget.maximum_payload_byte_count ||
      shape.logical_storage_entry_count >
          budget.maximum_logical_storage_entry_count) {
    return fail(result, Decision::no_transcript_budget_exhausted);
  }

  result.budget_preflight_satisfied = true;
  result.every_full_key_validated = true;
  result.homogeneous_facet_cardinality_validated = true;
  result.records_strictly_sorted_by_full_key = true;
  result.full_keys_unique = true;
  result.candidate_counts_within_k = true;
  result.candidate_point_ids_distinct = true;
  result.unused_candidate_slots_zero = true;

  std::vector<Record> validated_records{
      proposal_records.begin(), proposal_records.end()};
  result.proposal_records = std::move(validated_records);
  result.published_payload_byte_count = shape.payload_byte_count;
  result.published_logical_storage_entry_count =
      shape.logical_storage_entry_count;
  result.payload_published_only_after_full_validation = true;
  result.decision =
      result.proposal_records.empty()
          ? Decision::complete_empty_proposal_transcript
          : Decision::complete_validated_proposal_transcript;
  return result;
}

}  // namespace morsehgp3d::hierarchy
