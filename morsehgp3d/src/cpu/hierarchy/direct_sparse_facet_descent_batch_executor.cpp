#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t maximum_lane_count_per_exact_batch = 3U;
constexpr std::size_t maximum_selected_key_point_reference_count =
    direct_sparse_facet_descent_closure_maximum_seed_count *
    direct_sparse_positive_facet_maximum_point_count;
constexpr std::size_t phase14_active_query_record_byte_count = 208U;
constexpr std::size_t phase14_active_output_record_byte_count = 144U;

enum class BuildFailure : std::uint8_t {
  source_join_inconsistent,
  capacity_overflow,
  batch_budget_exhausted,
  allocation_failed,
  shared_closure_budget_exhausted,
  shared_closure_unresolved,
  shared_closure_contradiction,
  shared_closure_rejected,
};

struct BatchCursor {
  std::size_t source_batch_index{};
  std::size_t source_chunk_index{};
  std::size_t source_lane_index{};
  std::size_t source_family_index{};
  std::size_t source_arm_seed_index{};
};

[[nodiscard]] bool same_cursor(
    const BatchCursor& left,
    const BatchCursor& right) noexcept {
  return left.source_batch_index == right.source_batch_index &&
         left.source_chunk_index == right.source_chunk_index &&
         left.source_lane_index == right.source_lane_index &&
         left.source_family_index == right.source_family_index &&
         left.source_arm_seed_index == right.source_arm_seed_index;
}

[[nodiscard]] std::optional<BatchCursor> prevalidated_successor_cursor(
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan,
    const BatchCursor& source_cursor,
    const ExactDirectSparseFacetDescentBatchExecutionResult& delta) noexcept {
  if (source_cursor.source_batch_index >=
          source_event_journal.batches.size() ||
      delta.source_batch_index != source_cursor.source_batch_index ||
      !delta.source_chunk_index.has_value() ||
      *delta.source_chunk_index != source_cursor.source_chunk_index ||
      delta.source_lane_begin_index != source_cursor.source_lane_index ||
      delta.source_family_begin_index != source_cursor.source_family_index ||
      delta.source_arm_seed_begin_index !=
          source_cursor.source_arm_seed_index) {
    return std::nullopt;
  }

  const std::size_t successor_batch_index =
      source_cursor.source_batch_index + 1U;
  std::size_t successor_chunk_index =
      source_cursor.source_chunk_index;
  while (successor_chunk_index <
             source_plan.source_industrial_plan.chunks.size() &&
         successor_batch_index >=
             source_plan.source_industrial_plan
                 .chunks[successor_chunk_index]
                 .source_batch_end_index) {
    ++successor_chunk_index;
  }
  const bool terminal =
      successor_batch_index == source_event_journal.batches.size();
  if ((terminal &&
       (delta.source_lane_end_index != source_plan.lanes.size() ||
        delta.source_family_end_index !=
            source_arm_seed_journal.families.size() ||
        delta.source_arm_seed_end_index !=
            source_arm_seed_journal.arm_seeds.size() ||
        successor_chunk_index !=
            source_plan.source_industrial_plan.chunks.size())) ||
      (!terminal &&
       successor_chunk_index >=
           source_plan.source_industrial_plan.chunks.size())) {
    return std::nullopt;
  }
  return BatchCursor{
      successor_batch_index,
      successor_chunk_index,
      delta.source_lane_end_index,
      delta.source_family_end_index,
      delta.source_arm_seed_end_index};
}

struct SelectedArm {
  std::size_t arm_seed_index{};
  std::size_t family_index{};
  std::size_t lane_index{};
  ExactDirectSparseFacetKey source_facet_key{};
};

struct CompactClosureProjection {
  ExactDirectSparseFacetDescentBatchClosureSummary summary{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::vector<ExactDirectSparseFacetDescentBatchResolvedKey> resolved_keys;
  std::size_t closure_build_count{};
  std::optional<BuildFailure> failure;
};

struct ProposalPreparationEvidence {
  std::optional<
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
      consumption_audit;
  ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision
      consumption_decision{
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision::
              not_validated};
  bool consumption_attempted{false};
  bool validation_completed_before_closure{false};
  bool exact_consumption_certified{false};
  bool atomic_rejection_certified{false};
  bool no_closure_constructed_on_rejection{false};
  bool scientific_closure_separated_from_consumption_audit{false};
  bool no_transcript_or_top_k_payload_persisted_in_closure{false};
};

enum class IntegratedRunHookFailure : std::uint8_t {
  none,
  run_budget_rejected,
  prepare_inputs_rejected,
  seal_inputs_rejected,
};

class NanosecondClockReader {
 public:
  explicit NanosecondClockReader(
      const ExactDirectSparseFacetDescentBatchNanosecondClock& clock)
      : clock_(&clock) {}

  [[nodiscard]] std::uint64_t read() {
    std::uint64_t value = 0U;
    if (*clock_) {
      value = (*clock_)();
    } else {
      const auto elapsed =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now().time_since_epoch());
      if (elapsed.count() < 0) {
        throw std::runtime_error(
            "the steady nanosecond clock returned a negative value");
      }
      value = static_cast<std::uint64_t>(elapsed.count());
    }
    if (last_.has_value() && value < *last_) {
      throw std::invalid_argument(
          "an integrated batch-run clock must be monotonic");
    }
    last_ = value;
    return value;
  }

 private:
  const ExactDirectSparseFacetDescentBatchNanosecondClock* clock_{};
  std::optional<std::uint64_t> last_;
};

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] std::optional<std::uint64_t> checked_add_duration(
    std::uint64_t left,
    std::uint64_t right) noexcept {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] bool known_atomic_transcript_rejection_decision(
    ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision
        decision) noexcept {
  using Decision =
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision;
  switch (decision) {
    case Decision::no_closure_transcript_not_complete:
    case Decision::no_closure_metadata_mismatch:
    case Decision::no_closure_canonical_seed_shape_rejected:
    case Decision::no_closure_record_key_not_in_seed_domain:
    case Decision::no_closure_candidate_point_domain_rejected:
      return true;
    case Decision::not_validated:
    case Decision::complete_exact_closure_with_proposal_consumption_audit:
      return false;
  }
  return false;
}

[[nodiscard]] bool proposal_rejection_audit_has_no_exact_work(
    const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
        audit) noexcept {
  const bool no_stop_reason_count = std::all_of(
      audit.top_k_stop_reason_counts.begin(),
      audit.top_k_stop_reason_counts.end(),
      [](std::size_t count) { return count == 0U; });
  return audit.closure_build_count == 0U &&
         audit.top_k_query_count == 0U &&
         audit.nonempty_proposal_hit_query_count == 0U &&
         audit.missing_initial_record_fallback_query_count == 0U &&
         audit.explicit_empty_record_fallback_query_count == 0U &&
         audit.dynamic_successor_fallback_query_count == 0U &&
         audit.baseline_facet_point_reference_count == 0U &&
         audit.proposal_point_reference_count == 0U &&
         audit.union_point_reference_count == 0U &&
         audit.deduplicated_point_reference_count == 0U &&
         audit.exact_seed_distance_evaluation_count == 0U &&
         audit.exact_point_distance_evaluation_count == 0U &&
         audit.node_visit_count == 0U &&
         audit.internal_node_expansion_count == 0U &&
         audit.exact_aabb_bound_evaluation_count == 0U &&
         audit.pruned_subtree_count == 0U &&
         audit.pruned_eligible_point_count == 0U &&
         audit.traversal_complete_query_count == 0U &&
         audit.complete_query_count == 0U &&
         audit.full_scan_query_count == 0U &&
         audit.strict_pruning_query_count == 0U &&
         audit.exhausted_query_count == 0U &&
         no_stop_reason_count;
}

[[nodiscard]] bool proposal_consumption_audit_consistent_with_delta(
    const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
        audit,
    const ExactDirectSparseFacetDescentBatchExecutionResult& delta) noexcept {
  std::optional<std::size_t> classified_query_count = checked_add(
      audit.nonempty_proposal_hit_query_count,
      audit.missing_initial_record_fallback_query_count);
  if (!classified_query_count.has_value()) {
    return false;
  }
  classified_query_count = checked_add(
      *classified_query_count,
      audit.explicit_empty_record_fallback_query_count);
  if (!classified_query_count.has_value()) {
    return false;
  }
  classified_query_count = checked_add(
      *classified_query_count,
      audit.dynamic_successor_fallback_query_count);
  const std::optional<std::size_t> combined_pool_reference_count =
      checked_add(
          audit.baseline_facet_point_reference_count,
          audit.proposal_point_reference_count);
  const std::optional<std::size_t> complete_classification_count =
      checked_add(
          audit.full_scan_query_count,
          audit.strict_pruning_query_count);
  const std::optional<std::size_t> terminal_query_count =
      checked_add(
          audit.complete_query_count,
          audit.exhausted_query_count);
  const std::optional<std::size_t> expected_baseline_reference_count =
      checked_multiply(
          audit.top_k_query_count,
          delta.source_facet_cardinality);
  const std::optional<std::size_t> maximum_transcript_candidate_count =
      checked_multiply(
          audit.transcript_record_count,
          delta.source_facet_cardinality);
  const auto& top_k_budget =
      delta.requested_closure_budget.step_budget.top_k_query;
  const std::optional<std::size_t> maximum_node_visit_count =
      checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_node_visit_count);
  const std::optional<std::size_t>
      maximum_internal_node_expansion_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_internal_node_expansion_count);
  const std::optional<std::size_t>
      maximum_exact_aabb_bound_evaluation_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_exact_aabb_bound_evaluation_count);
  const std::optional<std::size_t>
      maximum_exact_point_distance_evaluation_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_exact_point_distance_evaluation_count);
  if (!classified_query_count.has_value() ||
      !combined_pool_reference_count.has_value() ||
      !complete_classification_count.has_value() ||
      !terminal_query_count.has_value() ||
      !expected_baseline_reference_count.has_value() ||
      !maximum_transcript_candidate_count.has_value() ||
      !maximum_node_visit_count.has_value() ||
      !maximum_internal_node_expansion_count.has_value() ||
      !maximum_exact_aabb_bound_evaluation_count.has_value() ||
      !maximum_exact_point_distance_evaluation_count.has_value() ||
      audit.deduplicated_point_reference_count >
          *combined_pool_reference_count) {
    return false;
  }

  std::size_t stop_reason_count = 0U;
  for (const std::size_t count : audit.top_k_stop_reason_counts) {
    const std::optional<std::size_t> next =
        checked_add(stop_reason_count, count);
    if (!next.has_value()) {
      return false;
    }
    stop_reason_count = *next;
  }
  const std::size_t no_stop_reason_index = static_cast<std::size_t>(
      spatial::ExactLbvhTopKStopReason::none);
  if (no_stop_reason_index >=
      audit.top_k_stop_reason_counts.size()) {
    return false;
  }

  const std::size_t expected_union_point_reference_count =
      *combined_pool_reference_count -
      audit.deduplicated_point_reference_count;
  return audit.canonical_seed_key_count ==
             delta.required_resolved_key_count &&
         audit.transcript_record_count <=
             audit.canonical_seed_key_count &&
         audit.transcript_candidate_point_reference_count <=
             *maximum_transcript_candidate_count &&
         audit.nonempty_proposal_hit_query_count <=
             audit.transcript_record_count &&
         audit.proposal_point_reference_count <=
             audit.transcript_candidate_point_reference_count &&
         *classified_query_count == audit.top_k_query_count &&
         stop_reason_count == audit.top_k_query_count &&
         *terminal_query_count == audit.top_k_query_count &&
         audit.top_k_stop_reason_counts[no_stop_reason_index] ==
             audit.complete_query_count &&
         *complete_classification_count == audit.complete_query_count &&
         audit.traversal_complete_query_count <=
             audit.top_k_query_count &&
         audit.union_point_reference_count ==
             expected_union_point_reference_count &&
         audit.baseline_facet_point_reference_count ==
             *expected_baseline_reference_count &&
         audit.exact_seed_distance_evaluation_count <=
             audit.union_point_reference_count &&
         audit.exact_point_distance_evaluation_count >=
             audit.exact_seed_distance_evaluation_count &&
         audit.node_visit_count <= *maximum_node_visit_count &&
         audit.internal_node_expansion_count <=
             *maximum_internal_node_expansion_count &&
         audit.exact_aabb_bound_evaluation_count <=
             *maximum_exact_aabb_bound_evaluation_count &&
         audit.exact_point_distance_evaluation_count <=
             *maximum_exact_point_distance_evaluation_count;
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t shared_count =
      std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < shared_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t expected_cardinality) noexcept {
  if (key.point_count != expected_cardinality ||
      key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key_from(
    const ExactDirectSaddleArmFacet& facet,
    std::size_t point_count,
    std::size_t expected_cardinality) {
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  key.point_ids = facet.point_ids;
  if (!valid_key(key, point_count, expected_cardinality)) {
    throw std::logic_error(
        "a selected direct saddle arm did not reconstruct one canonical "
        "order-k key");
  }
  return key;
}

[[nodiscard]] bool prepared_chunk_shape_is_consistent(
    const ExactDirectSparseFacetDescentBatchRunNextPreparedChunk& chunk,
    std::span<
        const ExactDirectSparseFacetDescentBatchPreparedProposalQuery>
        canonical_queries) noexcept {
  const std::optional<std::size_t> expected_h2d_byte_count =
      checked_multiply(
          chunk.gpu_supported_query_count,
          phase14_active_query_record_byte_count);
  const std::optional<std::size_t> expected_output_byte_count =
      checked_multiply(
          chunk.gpu_supported_query_count,
          phase14_active_output_record_byte_count);
  if (!expected_h2d_byte_count.has_value() ||
      !expected_output_byte_count.has_value() ||
      chunk.gpu_supported_query_count > canonical_queries.size() ||
      chunk.proposal_records.size() >
          chunk.gpu_supported_query_count ||
      chunk.active_host_to_device_query_record_count !=
          chunk.gpu_supported_query_count ||
      chunk.initialized_device_output_record_count !=
          chunk.gpu_supported_query_count ||
      chunk.copied_device_to_host_record_count !=
          chunk.gpu_supported_query_count ||
      chunk.active_host_to_device_query_byte_count !=
          *expected_h2d_byte_count ||
      chunk.initialized_device_output_byte_count !=
          *expected_output_byte_count ||
      chunk.copied_device_to_host_byte_count !=
          *expected_output_byte_count ||
      chunk.gpu_execution_performed !=
          (chunk.gpu_supported_query_count != 0U) ||
      !chunk.proposal_only ||
      chunk.exact_or_scientific_decision_published ||
      chunk.forbidden_global_structure_materialized ||
      chunk.public_status_claimed) {
    return false;
  }
  for (std::size_t record_index = 0U;
       record_index < chunk.proposal_records.size();
       ++record_index) {
    const ExactDirectSparseFacetTopKProposalRecord& record =
        chunk.proposal_records[record_index];
    const auto found = std::lower_bound(
        canonical_queries.begin(),
        canonical_queries.end(),
        record.source_facet_key,
        [](const ExactDirectSparseFacetDescentBatchPreparedProposalQuery&
               query,
           const ExactDirectSparseFacetKey& key) {
          return key_less(query.source_facet_key, key);
        });
    if (found == canonical_queries.end() ||
        found->source_facet_key != record.source_facet_key ||
        (record_index != 0U &&
         !key_less(
             chunk.proposal_records[record_index - 1U]
                 .source_facet_key,
             record.source_facet_key))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool sealed_transcript_matches_inputs(
    const ExactDirectSparseFacetTopKProposalTranscriptResult& transcript,
    const ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata,
    const ExactDirectSparseFacetTopKProposalTranscriptBudget& budget,
    std::span<const ExactDirectSparseFacetTopKProposalRecord>
        records) noexcept {
  return transcript.complete_proposal_transcript() &&
         transcript.metadata == metadata &&
         transcript.requested_budget == budget &&
         transcript.input_proposal_record_count == records.size() &&
         transcript.proposal_records.size() == records.size() &&
         std::equal(
             transcript.proposal_records.begin(),
             transcript.proposal_records.end(),
             records.begin(),
             records.end()) &&
         !transcript.exact_top_k_partition_certified &&
         !transcript.scientific_decision_published &&
         !transcript.locator_state_mutated &&
         !transcript.hierarchy_reduction_or_attachment_published &&
         !transcript.forbidden_global_structure_materialized &&
         !transcript.public_status_claimed &&
         transcript.proposal_only;
}

void increment_audit_counter(std::size_t& counter);

struct IntegratedRunHook {
  const ExactDirectSparseFacetDescentBatchIntegratedRunBudget* budget{};
  const ExactDirectSparseFacetDescentBatchPrepareInputsCallback*
      prepare_inputs{};
  const ExactDirectSparseFacetDescentBatchSealInputsCallback*
      seal_inputs{};
  NanosecondClockReader* clock{};
  ExactDirectSparseFacetDescentBatchIntegratedRunAudit* audit{};
  std::uint64_t run_started_ns{};
  std::uint64_t seal_completed_ns{};
  IntegratedRunHookFailure failure{IntegratedRunHookFailure::none};
  std::optional<ExactDirectSparseFacetTopKProposalTranscriptResult>
      sealed_transcript;

  [[nodiscard]] const ExactDirectSparseFacetTopKProposalTranscriptResult*
  prepare_and_seal(
      const ExactDirectSparseFacetDescentBatchExecutionResult& batch,
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
          live_preclosure_locator_stamp,
      std::span<const ExactDirectSparseFacetKey> canonical_keys) {
    const std::uint64_t preflight_completed_ns = clock->read();
    audit->cpu_preflight_duration_ns =
        preflight_completed_ns - run_started_ns;
    audit->selected_arm_seed_count =
        batch.required_selected_arm_seed_count;
    audit->distinct_source_facet_key_count = canonical_keys.size();
    audit->proposal_query_count = canonical_keys.size();
    audit->proposal_query_capacity =
        budget->maximum_query_count_per_chunk;
    audit->cpu_batch_preflights_completed_before_callbacks =
        batch.source_exact_batch_and_chunk_joined &&
        batch.source_lanes_share_one_exact_batch &&
        batch.batch_budget_preflight_completed &&
        batch.batch_budget_preflight_satisfied &&
        batch.selected_families_scanned_once &&
        batch.every_arm_seed_selected_once_in_source_order &&
        batch.facets_reconstructed_on_demand_only &&
        batch.distinct_full_keys_canonicalized &&
        batch.locator_snapshot_stamp ==
            live_preclosure_locator_stamp;
    if (!audit->cpu_batch_preflights_completed_before_callbacks ||
        budget->maximum_query_count_per_chunk == 0U ||
        budget->maximum_chunk_count == 0U) {
      failure = IntegratedRunHookFailure::run_budget_rejected;
      return nullptr;
    }

    const std::optional<std::size_t> maximum_partitioned_query_count =
        checked_multiply(
            budget->maximum_query_count_per_chunk,
            budget->maximum_chunk_count);
    const std::optional<std::size_t> rounded_query_count = checked_add(
        canonical_keys.size(),
        budget->maximum_query_count_per_chunk - 1U);
    if (!maximum_partitioned_query_count.has_value() ||
        canonical_keys.size() >
            *maximum_partitioned_query_count ||
        !rounded_query_count.has_value()) {
      failure = IntegratedRunHookFailure::run_budget_rejected;
      return nullptr;
    }
    const std::size_t chunk_count =
        canonical_keys.empty()
            ? 0U
            : *rounded_query_count /
                  budget->maximum_query_count_per_chunk;
    audit->proposal_chunk_count = chunk_count;
    if (chunk_count > budget->maximum_chunk_count) {
      failure = IntegratedRunHookFailure::run_budget_rejected;
      return nullptr;
    }

    const std::uint64_t center_preparation_started_ns =
        clock->read();
    std::vector<
        ExactDirectSparseFacetDescentBatchPreparedProposalQuery>
        prepared_queries;
    prepared_queries.reserve(canonical_keys.size());
    for (const ExactDirectSparseFacetKey& key : canonical_keys) {
      ExactFacetMiniballResult miniball = build_exact_facet_miniball(
          cloud,
          std::span<const spatial::PointId>{
              key.point_ids.data(), key.point_count});
      const bool matching_facet =
          miniball.facet_point_ids.size() == key.point_count &&
          std::equal(
              miniball.facet_point_ids.begin(),
              miniball.facet_point_ids.end(),
              key.point_ids.begin());
      const std::optional<std::size_t> next_support_count =
          checked_add(
              audit->exact_proposal_enumerated_support_count,
              miniball.counters.enumerated_support_count);
      if (!matching_facet ||
          miniball.status !=
              ExactFacetMiniballStatus::
                  exact_facet_miniball_certified ||
          miniball.scope !=
              ExactFacetMiniballScope::local_facet_miniball_only ||
          !next_support_count.has_value()) {
        failure = IntegratedRunHookFailure::run_budget_rejected;
        return nullptr;
      }
      audit->exact_proposal_enumerated_support_count =
          *next_support_count;
      increment_audit_counter(
          audit->exact_proposal_miniball_build_count);
      prepared_queries.push_back(
          {key,
           std::move(miniball.center),
           std::move(miniball.squared_radius),
           miniball.counters.enumerated_support_count,
           true});
    }
    const std::uint64_t center_preparation_completed_ns =
        clock->read();
    audit->exact_center_preparation_duration_ns =
        center_preparation_completed_ns -
        center_preparation_started_ns;
    audit->exact_proposal_center_count =
        prepared_queries.size();
    audit->exact_centers_prepared_once_for_proposal =
        prepared_queries.size() == canonical_keys.size() &&
        audit->exact_proposal_miniball_build_count ==
            canonical_keys.size();
    audit->exact_center_reused_by_closure = false;

    std::vector<ExactDirectSparseFacetTopKProposalRecord>
        aggregate_records;
    aggregate_records.reserve(std::min(
        canonical_keys.size(),
        budget->transcript_budget
            .maximum_proposal_record_count));
    const ExactDirectSparseFacetTopKProposalTranscriptMetadata metadata{
        batch.source_batch_index,
        batch.closed_batch_squared_level,
        live_preclosure_locator_stamp};
    for (std::size_t chunk_index = 0U;
         chunk_index < chunk_count;
         ++chunk_index) {
      const std::optional<std::size_t> query_begin = checked_multiply(
          chunk_index, budget->maximum_query_count_per_chunk);
      if (!query_begin.has_value() ||
          *query_begin >= canonical_keys.size()) {
        failure = IntegratedRunHookFailure::run_budget_rejected;
        return nullptr;
      }
      const std::size_t query_count = std::min(
          budget->maximum_query_count_per_chunk,
          prepared_queries.size() - *query_begin);
      const auto chunk_queries =
          std::span<
              const ExactDirectSparseFacetDescentBatchPreparedProposalQuery>{
              prepared_queries}
              .subspan(
          *query_begin, query_count);
      const ExactDirectSparseFacetDescentBatchRunNextPrepareInputs
          inputs{
              metadata,
              chunk_queries,
              batch.required_selected_arm_seed_count,
              canonical_keys.size(),
              *query_begin,
              chunk_index,
              chunk_count,
              budget->maximum_query_count_per_chunk,
              true};
      const std::uint64_t callback_started_ns = clock->read();
      std::optional<
          ExactDirectSparseFacetDescentBatchRunNextPreparedChunk>
          prepared = (*prepare_inputs)(inputs);
      const std::uint64_t callback_completed_ns = clock->read();
      const std::optional<std::uint64_t> next_prepare_duration =
          checked_add_duration(
              audit->prepare_inputs_duration_ns,
              callback_completed_ns - callback_started_ns);
      if (!next_prepare_duration.has_value()) {
        failure = IntegratedRunHookFailure::prepare_inputs_rejected;
        return nullptr;
      }
      audit->prepare_inputs_duration_ns = *next_prepare_duration;
      increment_audit_counter(audit->prepare_inputs_callback_count);
      if (!prepared.has_value() ||
          !prepared_chunk_shape_is_consistent(
              *prepared, chunk_queries)) {
        failure = IntegratedRunHookFailure::prepare_inputs_rejected;
        return nullptr;
      }

      const std::optional<std::size_t> next_record_count =
          checked_add(
              aggregate_records.size(),
              prepared->proposal_records.size());
      const std::optional<std::size_t>
          next_supported_query_count = checked_add(
              audit->gpu_supported_query_count,
              prepared->gpu_supported_query_count);
      const std::optional<std::size_t> next_h2d_record_count =
          checked_add(
              audit->active_host_to_device_query_record_count,
              prepared
                  ->active_host_to_device_query_record_count);
      const std::optional<std::size_t> next_h2d_byte_count =
          checked_add(
              audit->active_host_to_device_query_byte_count,
              prepared->active_host_to_device_query_byte_count);
      const std::optional<std::size_t>
          next_initialized_record_count = checked_add(
              audit->initialized_device_output_record_count,
              prepared->initialized_device_output_record_count);
      const std::optional<std::size_t>
          next_initialized_byte_count = checked_add(
              audit->initialized_device_output_byte_count,
              prepared->initialized_device_output_byte_count);
      const std::optional<std::size_t> next_d2h_record_count =
          checked_add(
              audit->copied_device_to_host_record_count,
              prepared->copied_device_to_host_record_count);
      const std::optional<std::size_t> next_d2h_byte_count =
          checked_add(
              audit->copied_device_to_host_byte_count,
              prepared->copied_device_to_host_byte_count);
      if (!next_record_count.has_value() ||
          *next_record_count >
              budget->transcript_budget
                  .maximum_proposal_record_count ||
          !next_supported_query_count.has_value() ||
          !next_h2d_record_count.has_value() ||
          !next_h2d_byte_count.has_value() ||
          !next_initialized_record_count.has_value() ||
          !next_initialized_byte_count.has_value() ||
          !next_d2h_record_count.has_value() ||
          !next_d2h_byte_count.has_value()) {
        failure = IntegratedRunHookFailure::prepare_inputs_rejected;
        return nullptr;
      }
      audit->gpu_supported_query_count =
          *next_supported_query_count;
      audit->active_host_to_device_query_record_count =
          *next_h2d_record_count;
      audit->active_host_to_device_query_byte_count =
          *next_h2d_byte_count;
      audit->initialized_device_output_record_count =
          *next_initialized_record_count;
      audit->initialized_device_output_byte_count =
          *next_initialized_byte_count;
      audit->copied_device_to_host_record_count =
          *next_d2h_record_count;
      audit->copied_device_to_host_byte_count =
          *next_d2h_byte_count;
      aggregate_records.insert(
          aggregate_records.end(),
          std::make_move_iterator(
              prepared->proposal_records.begin()),
          std::make_move_iterator(
              prepared->proposal_records.end()));
    }
    audit->canonical_queries_partitioned_once =
        audit->proposal_query_count ==
            audit->distinct_source_facet_key_count &&
        audit->prepare_inputs_callback_count == chunk_count;
    audit->every_prepare_inputs_chunk_certified =
        audit->canonical_queries_partitioned_once;
    const std::optional<std::size_t> expected_total_h2d_bytes =
        checked_multiply(
            audit->gpu_supported_query_count,
            phase14_active_query_record_byte_count);
    const std::optional<std::size_t> expected_total_output_bytes =
        checked_multiply(
            audit->gpu_supported_query_count,
            phase14_active_output_record_byte_count);
    audit->active_traffic_only_reported =
        expected_total_h2d_bytes.has_value() &&
        expected_total_output_bytes.has_value() &&
        audit->active_host_to_device_query_record_count ==
            audit->gpu_supported_query_count &&
        audit->initialized_device_output_record_count ==
            audit->gpu_supported_query_count &&
        audit->copied_device_to_host_record_count ==
            audit->gpu_supported_query_count &&
        audit->active_host_to_device_query_byte_count ==
            *expected_total_h2d_bytes &&
        audit->initialized_device_output_byte_count ==
            *expected_total_output_bytes &&
        audit->copied_device_to_host_byte_count ==
            *expected_total_output_bytes;
    audit->aggregate_records_canonical_and_bounded =
        aggregate_records.size() <= canonical_keys.size() &&
        aggregate_records.size() <=
            budget->transcript_budget
                .maximum_proposal_record_count &&
        std::is_sorted(
            aggregate_records.begin(),
            aggregate_records.end(),
            [](const ExactDirectSparseFacetTopKProposalRecord& left,
               const ExactDirectSparseFacetTopKProposalRecord& right) {
              return key_less(
                  left.source_facet_key,
                  right.source_facet_key);
            });
    if (!audit->aggregate_records_canonical_and_bounded) {
      failure = IntegratedRunHookFailure::prepare_inputs_rejected;
      return nullptr;
    }

    const ExactDirectSparseFacetDescentBatchRunNextSealInputs
        seal_input{
            metadata,
            std::span<
                const ExactDirectSparseFacetTopKProposalRecord>{
                aggregate_records},
            budget->transcript_budget,
            batch.required_selected_arm_seed_count,
            canonical_keys.size(),
            audit->proposal_query_count,
            chunk_count,
            true};
    const std::uint64_t seal_started_ns = clock->read();
    std::optional<ExactDirectSparseFacetTopKProposalTranscriptResult>
        transcript = (*seal_inputs)(seal_input);
    seal_completed_ns = clock->read();
    audit->seal_inputs_duration_ns =
        seal_completed_ns - seal_started_ns;
    increment_audit_counter(audit->seal_inputs_callback_count);
    if (!transcript.has_value() ||
        !sealed_transcript_matches_inputs(
            *transcript,
            metadata,
            budget->transcript_budget,
            aggregate_records)) {
      failure = IntegratedRunHookFailure::seal_inputs_rejected;
      return nullptr;
    }
    audit->sealed_proposal_record_count =
        transcript->proposal_records.size();
    audit->transcript_sealed_once = true;
    sealed_transcript.emplace(std::move(*transcript));
    return &*sealed_transcript;
  }
};

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

void require_execution_budget_within_confidence(
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) {
  if (budget.maximum_selected_lane_count >
          maximum_lane_count_per_exact_batch ||
      budget.maximum_selected_family_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_selected_arm_seed_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_selected_key_point_reference_count >
          maximum_selected_key_point_reference_count ||
      budget.maximum_resolved_key_count >
          direct_sparse_facet_descent_closure_maximum_seed_count) {
    throw std::invalid_argument(
        "a direct batch-execution budget exceeds its confidence cap");
  }
}

void require_closure_budget_within_confidence(
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  if (budget.maximum_seed_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_node_count >
          direct_sparse_facet_descent_closure_maximum_node_count ||
      budget.maximum_step_call_count >
          direct_sparse_facet_descent_closure_maximum_step_call_count ||
      budget.maximum_memo_slot_count >
          direct_sparse_facet_descent_closure_maximum_memo_slot_count) {
    throw std::invalid_argument(
        "a direct batch-execution closure budget exceeds its confidence cap");
  }
}

void initialize_result_scope(
    ExactDirectSparseFacetDescentBatchExecutionResult& result) {
  result.source_plan_verified_once_at_session_open = true;
  result.source_plan_rebuilt_for_this_batch = false;
  result.counters.anchored_source_plan_verification_count = 1U;
  result.counters.batch_local_source_plan_rebuild_count = 0U;
  result.scientific_commit_performed = false;
  result.locator_state_mutated = false;
  result.hierarchy_reduction_or_attachment_published = false;
  result.partial_closure_prefix_published_as_delta = false;
  result.facet_coface_cell_gamma_or_delaunay_materialized = false;
  result.gpu_execution_qualified = false;
  result.sub_second_latency_claimed = false;
  result.ten_million_point_capacity_claimed = false;
  result.public_status_claimed = false;
  result.closure_graph_persisted = false;
  result.scope =
      ExactDirectSparseFacetDescentBatchExecutionScope::
          one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult fail(
    ExactDirectSparseFacetDescentBatchExecutionResult result,
    BuildFailure failure) {
  result.resolved_keys.clear();
  result.arm_joins.clear();
  result.batch_delta_ready_before_commit = false;
  result.every_arm_joined_by_identity_and_full_key = false;
  result.partial_closure_prefix_published_as_delta = false;
  result.closure_graph_persisted = false;
  result.transient_closure_released_before_delta_publication = true;
  switch (failure) {
    case BuildFailure::source_join_inconsistent:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_source_join_inconsistent;
      break;
    case BuildFailure::capacity_overflow:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_capacity_overflow;
      break;
    case BuildFailure::batch_budget_exhausted:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_batch_budget_exhausted;
      break;
    case BuildFailure::allocation_failed:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_allocation_failed;
      break;
    case BuildFailure::shared_closure_budget_exhausted:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_budget_exhausted;
      break;
    case BuildFailure::shared_closure_unresolved:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_unresolved;
      break;
    case BuildFailure::shared_closure_contradiction:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_contradiction;
      break;
    case BuildFailure::shared_closure_rejected:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_rejected;
      break;
  }
  return result;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchClosureSummary
summarize_closure(
    const ExactDirectSparseFacetDescentClosureResult& closure) {
  ExactDirectSparseFacetDescentBatchClosureSummary summary;
  summary.transient_node_count = closure.nodes.size();
  summary.transient_edge_count = closure.edges.size();
  summary.transient_seed_projection_count =
      closure.seed_projections.size();
  summary.required_memo_slot_count = closure.required_memo_slot_count;
  summary.counters = closure.counters;
  summary.disposition = closure.disposition;
  summary.decision = closure.decision;
  summary.complete_relative_positive =
      closure.certified_complete_relative_positive_closure();
  summary.graph_payload_persisted = false;
  return summary;
}

[[nodiscard]] CompactClosureProjection build_compact_closure_projection(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const std::vector<ExactDirectSparseFacetKey>& distinct_keys,
    std::size_t source_batch_index,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetTopKProposalTranscriptResult*
        proposal_transcript,
    ProposalPreparationEvidence* proposal_evidence) {
  // The closure object is scoped inside this lambda.  Only the compact
  // projection and, on the proposal path, a key-free scalar audit survive its
  // destruction.
  return [&]() {
    CompactClosureProjection projection;
    ExactDirectSparseFacetDescentClosureResult closure;
    if (proposal_transcript == nullptr) {
      closure =
          build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
              index,
              cloud,
              std::span<const ExactDirectSparseFacetKey>{distinct_keys},
              closed_batch_squared_level,
              locator_query_witness,
              locator,
              closure_budget,
              closure_config,
              traversal_order);
      projection.closure_build_count = 1U;
    } else {
      if (proposal_evidence == nullptr) {
        throw std::logic_error(
            "proposal consumption requires a separate preparation audit");
      }
      proposal_evidence->consumption_attempted = true;
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionResult
          wrapped =
              build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys_with_top_k_proposal_transcript(
                  index,
                  cloud,
                  std::span<const ExactDirectSparseFacetKey>{distinct_keys},
                  source_batch_index,
                  closed_batch_squared_level,
                  locator_query_witness,
                  locator,
                  *proposal_transcript,
                  closure_budget,
                  closure_config,
                  traversal_order);
      proposal_evidence->consumption_audit =
          wrapped.consumption_audit;
      proposal_evidence->consumption_decision = wrapped.decision;
      proposal_evidence->validation_completed_before_closure =
          wrapped.validation_completed_before_closure;
      proposal_evidence->exact_consumption_certified =
          wrapped.certified_exact_consumption_outcome();
      proposal_evidence->atomic_rejection_certified =
          wrapped.certified_atomic_rejection();
      proposal_evidence->no_closure_constructed_on_rejection =
          wrapped.no_closure_constructed_on_rejection;
      proposal_evidence
          ->scientific_closure_separated_from_consumption_audit =
          wrapped.scientific_closure_separated_from_consumption_audit;
      proposal_evidence
          ->no_transcript_or_top_k_payload_persisted_in_closure =
          wrapped.no_transcript_or_top_k_payload_persisted_in_closure;
      projection.locator_snapshot_stamp =
          wrapped.consumption_audit.locator_snapshot_stamp;
      projection.closure_build_count =
          wrapped.consumption_audit.closure_build_count;
      if (wrapped.certified_atomic_rejection()) {
        projection.failure = BuildFailure::shared_closure_rejected;
        return projection;
      }
      if (!wrapped.certified_exact_consumption_outcome() ||
          !wrapped.scientific_closure.has_value()) {
        throw std::logic_error(
            "proposal consumption produced no certified exact closure");
      }
      closure = std::move(*wrapped.scientific_closure);
    }
    projection.summary = summarize_closure(closure);
    projection.locator_snapshot_stamp = closure.locator_snapshot_stamp;

    if (!closure.certified_complete_relative_positive_closure()) {
      if (closure.certified_budget_exhaustion()) {
        projection.failure =
            BuildFailure::shared_closure_budget_exhausted;
      } else if (
          closure.certified_complete_with_unresolved_terminals()) {
        projection.failure = BuildFailure::shared_closure_unresolved;
      } else if (closure.certified_fail_closed_contradiction()) {
        projection.failure = BuildFailure::shared_closure_contradiction;
      } else {
        projection.failure = BuildFailure::shared_closure_rejected;
      }
      return projection;
    }
    if (closure.seed_projections.size() != distinct_keys.size()) {
      projection.failure = BuildFailure::shared_closure_contradiction;
      return projection;
    }

    projection.resolved_keys.reserve(distinct_keys.size());
    for (std::size_t key_index = 0U;
         key_index < distinct_keys.size();
         ++key_index) {
      const ExactDirectSparseFacetDescentSeedProjection& seed_projection =
          closure.seed_projections[key_index];
      if (seed_projection.seed_index != key_index ||
          seed_projection.source_facet_key != distinct_keys[key_index] ||
          seed_projection.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          seed_projection.terminal_node_index >= closure.nodes.size()) {
        projection.failure = BuildFailure::shared_closure_contradiction;
        projection.resolved_keys.clear();
        return projection;
      }
      const ExactDirectSparseFacetDescentNode& terminal =
          closure.nodes[seed_projection.terminal_node_index];
      if (!terminal.resolved_component_handle.has_value() ||
          !terminal.resolved_binding_witness.has_value() ||
          terminal.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          terminal.resolved_binding_witness->external_authority_id !=
              locator.config().external_authority_id ||
          terminal.resolved_binding_witness->replay_token == 0U) {
        projection.failure = BuildFailure::shared_closure_contradiction;
        projection.resolved_keys.clear();
        return projection;
      }
      projection.resolved_keys.push_back(
          {key_index,
           distinct_keys[key_index],
           *terminal.resolved_component_handle,
           *terminal.resolved_binding_witness,
           seed_projection.closure_disposition,
           true});
    }
    return projection;
  }();
}

[[nodiscard]] bool observed_storage_within_budget(
    const ExactDirectSparseFacetDescentBatchExecutionResult& observed,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) noexcept {
  if (observed.resolved_keys.size() >
          budget.maximum_resolved_key_count ||
      observed.arm_joins.size() >
          budget.maximum_selected_arm_seed_count) {
    return false;
  }
  std::size_t key_point_reference_count = 0U;
  for (const auto& resolved : observed.resolved_keys) {
    const std::optional<std::size_t> next_key_point_reference_count =
        checked_add(
            key_point_reference_count,
            resolved.source_facet_key.point_count);
    if (resolved.source_facet_key.point_count >
            resolved.source_facet_key.point_ids.size() ||
        !next_key_point_reference_count.has_value() ||
        *next_key_point_reference_count >
            budget.maximum_selected_key_point_reference_count) {
      return false;
    }
    key_point_reference_count = *next_key_point_reference_count;
  }
  return true;
}

[[nodiscard]] bool architecture_execution_payload_shape_is_consistent(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result) noexcept {
  if (!result.source_chunk_index.has_value() ||
      result.source_facet_cardinality == 0U ||
      result.source_facet_cardinality >
          direct_sparse_positive_facet_maximum_point_count ||
      result.source_lane_end_index < result.source_lane_begin_index ||
      result.source_family_end_index < result.source_family_begin_index ||
      result.source_arm_seed_end_index <
          result.source_arm_seed_begin_index ||
      result.source_lane_end_index - result.source_lane_begin_index !=
          result.required_selected_lane_count ||
      result.source_family_end_index - result.source_family_begin_index !=
          result.required_selected_family_count ||
      result.source_arm_seed_end_index -
              result.source_arm_seed_begin_index !=
          result.required_selected_arm_seed_count ||
      result.required_selected_lane_count >
          result.requested_budget.maximum_selected_lane_count ||
      result.required_selected_family_count >
          result.requested_budget.maximum_selected_family_count ||
      result.required_selected_arm_seed_count >
          result.requested_budget.maximum_selected_arm_seed_count ||
      result.required_selected_key_point_reference_count >
          result.requested_budget
              .maximum_selected_key_point_reference_count ||
      result.required_resolved_key_count >
          result.requested_budget.maximum_resolved_key_count ||
      result.required_resolved_key_count >
          result.required_selected_arm_seed_count ||
      result.resolved_keys.size() != result.required_resolved_key_count ||
      result.arm_joins.size() !=
          result.required_selected_arm_seed_count ||
      result.locator_query_witness.external_authority_id == 0U ||
      result.locator_query_witness.replay_token == 0U) {
    return false;
  }

  const std::optional<std::size_t>
      expected_selected_key_point_reference_count = checked_multiply(
          result.required_selected_arm_seed_count,
          result.source_facet_cardinality);
  const std::optional<std::size_t>
      expected_resolved_key_point_reference_count = checked_multiply(
          result.required_resolved_key_count,
          result.source_facet_cardinality);
  const std::optional<std::size_t> projected_arm_reference_count =
      checked_add(
          result.counters.duplicate_arm_key_reference_count,
          result.counters.distinct_closure_seed_count);
  if (!expected_selected_key_point_reference_count.has_value() ||
      !expected_resolved_key_point_reference_count.has_value() ||
      !projected_arm_reference_count.has_value() ||
      result.required_selected_key_point_reference_count !=
          *expected_selected_key_point_reference_count ||
      *expected_resolved_key_point_reference_count >
          result.requested_budget
              .maximum_selected_key_point_reference_count ||
      result.counters.distinct_closure_seed_count !=
          result.required_resolved_key_count ||
      *projected_arm_reference_count !=
          result.required_selected_arm_seed_count) {
    return false;
  }

  for (std::size_t resolved_index = 0U;
       resolved_index < result.resolved_keys.size();
       ++resolved_index) {
    const ExactDirectSparseFacetDescentBatchResolvedKey& resolved =
        result.resolved_keys[resolved_index];
    if (resolved.resolved_key_index != resolved_index ||
        !valid_key(
            resolved.source_facet_key,
            std::numeric_limits<std::size_t>::max(),
            result.source_facet_cardinality) ||
        resolved.resolved_binding_witness.external_authority_id !=
            result.locator_query_witness.external_authority_id ||
        resolved.resolved_binding_witness.replay_token == 0U ||
        resolved.closure_disposition !=
            ExactDirectSparseFacetDescentClosureDisposition::
                relative_positive ||
        !resolved.source_projection_and_terminal_certified ||
        (resolved_index != 0U &&
         !key_less(
             result.resolved_keys[resolved_index - 1U].source_facet_key,
             resolved.source_facet_key))) {
      return false;
    }
  }

  for (std::size_t join_index = 0U;
       join_index < result.arm_joins.size();
       ++join_index) {
    const ExactDirectSparseFacetDescentBatchArmJoin& join =
        result.arm_joins[join_index];
    const std::optional<std::size_t> expected_arm_seed_index =
        checked_add(result.source_arm_seed_begin_index, join_index);
    if (!expected_arm_seed_index.has_value() ||
        join.arm_seed_index != *expected_arm_seed_index ||
        join.family_index < result.source_family_begin_index ||
        join.family_index >= result.source_family_end_index ||
        join.lane_index < result.source_lane_begin_index ||
        join.lane_index >= result.source_lane_end_index ||
        join.resolved_key_index >= result.resolved_keys.size() ||
        !join.arm_identity_and_full_key_joined ||
        (join_index != 0U &&
         join.family_index <
             result.arm_joins[join_index - 1U].family_index)) {
      return false;
    }
  }

  const bool empty_batch =
      result.required_selected_arm_seed_count == 0U;
  if (empty_batch) {
    return result.required_selected_lane_count == 0U &&
           result.required_selected_family_count == 0U &&
           result.required_resolved_key_count == 0U &&
           result.closure_summary.transient_node_count == 0U &&
           result.closure_summary.transient_edge_count == 0U &&
           result.closure_summary.transient_seed_projection_count == 0U &&
           result.closure_summary.counters ==
               ExactDirectSparseFacetDescentClosureCounters{} &&
           result.closure_summary.disposition ==
               ExactDirectSparseFacetDescentClosureDisposition::
                   relative_positive &&
           result.closure_summary.decision ==
               ExactDirectSparseFacetDescentClosureDecision::
                   complete_empty_seed_set;
  }

  const std::optional<std::size_t> expected_locator_snapshot_check_count =
      checked_add(
          result.closure_summary.counters.evaluated_step_source_count,
          2U);
  return result.required_selected_lane_count != 0U &&
         result.required_selected_family_count != 0U &&
         result.required_resolved_key_count != 0U &&
         result.required_resolved_key_count <=
             result.requested_closure_budget.maximum_seed_count &&
         result.closure_summary.transient_node_count >=
             result.required_resolved_key_count &&
         result.closure_summary.transient_node_count <=
             result.requested_closure_budget.maximum_node_count &&
         result.closure_summary.transient_edge_count <
             result.closure_summary.transient_node_count &&
         result.closure_summary.transient_seed_projection_count ==
             result.required_resolved_key_count &&
         result.closure_summary.required_memo_slot_count <=
             result.requested_closure_budget.maximum_memo_slot_count &&
         result.closure_summary.counters.input_seed_reference_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.processed_seed_reference_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.distinct_seed_key_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.duplicate_seed_key_reference_count ==
             0U &&
         result.closure_summary.counters.interned_node_count ==
             result.closure_summary.transient_node_count &&
         result.closure_summary.counters.strict_edge_count ==
             result.closure_summary.transient_edge_count &&
         result.closure_summary.counters.unresolved_terminal_count == 0U &&
         result.closure_summary.counters.budget_terminal_count == 0U &&
         expected_locator_snapshot_check_count.has_value() &&
         result.closure_summary.counters.locator_snapshot_check_count ==
             *expected_locator_snapshot_check_count &&
         result.closure_summary.disposition ==
             ExactDirectSparseFacetDescentClosureDisposition::
                 relative_positive &&
         result.closure_summary.decision ==
             ExactDirectSparseFacetDescentClosureDecision::
                 complete_all_seeds_relative_positive;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult
build_batch_execution(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan,
    const BatchCursor& cursor,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetTopKProposalTranscriptResult*
        proposal_transcript,
    ProposalPreparationEvidence* proposal_evidence,
    IntegratedRunHook* integrated_run_hook) {
  const bool no_proposal_path =
      proposal_transcript == nullptr &&
      proposal_evidence == nullptr &&
      integrated_run_hook == nullptr;
  const bool supplied_transcript_path =
      proposal_transcript != nullptr &&
      proposal_evidence != nullptr &&
      integrated_run_hook == nullptr;
  const bool integrated_run_path =
      proposal_transcript == nullptr &&
      proposal_evidence != nullptr &&
      integrated_run_hook != nullptr;
  if (!no_proposal_path && !supplied_transcript_path &&
      !integrated_run_path) {
    throw std::invalid_argument(
        "a batch execution requires exactly one coherent proposal path");
  }
  if (proposal_evidence != nullptr) {
    *proposal_evidence = ProposalPreparationEvidence{};
  }
  require_execution_budget_within_confidence(execution_budget);
  require_closure_budget_within_confidence(closure_budget);
  require_valid_traversal_order(traversal_order);
  if (locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      locator_query_witness.replay_token == 0U) {
    throw std::invalid_argument(
        "a direct batch execution requires one matching locator witness");
  }

  ExactDirectSparseFacetDescentBatchExecutionResult result;
  result.requested_budget = execution_budget;
  result.requested_closure_budget = closure_budget;
  result.traversal_order = traversal_order;
  result.source_batch_index = cursor.source_batch_index;
  result.locator_query_witness = locator_query_witness;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  initialize_result_scope(result);

  if (cursor.source_batch_index >= source_event_journal.batches.size() ||
      cursor.source_chunk_index >=
          source_plan.source_industrial_plan.chunks.size() ||
      cursor.source_lane_index > source_plan.lanes.size() ||
      cursor.source_family_index > source_arm_seed_journal.families.size() ||
      cursor.source_arm_seed_index >
          source_arm_seed_journal.arm_seeds.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  const ExactDirectMorseH0Batch& source_batch =
      source_event_journal.batches[cursor.source_batch_index];
  const ExactDirectMorseIndustrialChunk& source_chunk =
      source_plan.source_industrial_plan.chunks[cursor.source_chunk_index];
  if (source_batch.batch_index != cursor.source_batch_index ||
      source_batch.order == 0U ||
      source_batch.order >
          direct_sparse_positive_facet_maximum_point_count ||
      source_chunk.chunk_index != cursor.source_chunk_index ||
      cursor.source_batch_index <
          source_chunk.source_batch_begin_index ||
      cursor.source_batch_index >= source_chunk.source_batch_end_index) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  result.source_chunk_index = cursor.source_chunk_index;
  result.source_facet_cardinality = source_batch.order;
  result.closed_batch_squared_level = source_batch.squared_level;
  result.source_lane_begin_index = cursor.source_lane_index;
  result.source_family_begin_index = cursor.source_family_index;
  result.source_arm_seed_begin_index = cursor.source_arm_seed_index;

  std::size_t lane_end = cursor.source_lane_index;
  while (lane_end < source_plan.lanes.size() &&
         source_plan.lanes[lane_end].source_batch_index ==
             cursor.source_batch_index) {
    ++lane_end;
  }
  if (lane_end < source_plan.lanes.size() &&
      source_plan.lanes[lane_end].source_batch_index <
          cursor.source_batch_index) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.source_lane_end_index = lane_end;

  const std::optional<std::size_t> family_end = checked_add(
      cursor.source_family_index, source_batch.saddle_role_count);
  if (!family_end.has_value() ||
      *family_end > source_arm_seed_journal.families.size()) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.source_family_end_index = *family_end;

  std::size_t required_arm_seed_count = 0U;
  std::size_t required_family_count_from_lanes = 0U;
  std::array<std::optional<std::size_t>, 5U> lane_by_support_cardinality{};
  for (std::size_t lane_index = cursor.source_lane_index;
       lane_index < lane_end;
       ++lane_index) {
    const ExactDirectSparseFacetDescentBatchLane& lane =
        source_plan.lanes[lane_index];
    if (lane.lane_index != lane_index ||
        lane.source_chunk_index != cursor.source_chunk_index ||
        lane.source_batch_index != cursor.source_batch_index ||
        lane.candidate_family_begin_index != cursor.source_family_index ||
        lane.candidate_family_end_index != *family_end ||
        lane.candidate_arm_seed_begin_index !=
            cursor.source_arm_seed_index ||
        lane.facet_cardinality != source_batch.order ||
        lane.source_support_cardinality < 2U ||
        lane.source_support_cardinality > 4U ||
        lane.source_support_cardinality > source_batch.order + 1U ||
        lane.source_interior_cardinality !=
            source_batch.order + 1U -
                lane.source_support_cardinality ||
        lane_by_support_cardinality[lane.source_support_cardinality]
            .has_value()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    lane_by_support_cardinality[lane.source_support_cardinality] =
        lane_index;
    const std::optional<std::size_t> next_family_count = checked_add(
        required_family_count_from_lanes, lane.matching_family_count);
    const std::optional<std::size_t> next_arm_count = checked_add(
        required_arm_seed_count, lane.matching_arm_seed_count);
    if (!next_family_count.has_value() || !next_arm_count.has_value()) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    required_family_count_from_lanes = *next_family_count;
    required_arm_seed_count = *next_arm_count;
  }
  if (required_family_count_from_lanes !=
          source_batch.saddle_role_count ||
      (source_batch.saddle_role_count == 0U) !=
          (lane_end == cursor.source_lane_index)) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  const std::optional<std::size_t> arm_seed_end =
      checked_add(cursor.source_arm_seed_index, required_arm_seed_count);
  const std::optional<std::size_t> key_point_reference_count =
      checked_multiply(required_arm_seed_count, source_batch.order);
  if (!arm_seed_end.has_value() ||
      !key_point_reference_count.has_value()) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (*arm_seed_end > source_arm_seed_journal.arm_seeds.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.source_arm_seed_end_index = *arm_seed_end;
  result.required_selected_lane_count =
      lane_end - cursor.source_lane_index;
  result.required_selected_family_count =
      source_batch.saddle_role_count;
  result.required_selected_arm_seed_count = required_arm_seed_count;
  result.required_selected_key_point_reference_count =
      *key_point_reference_count;
  result.batch_budget_preflight_completed = true;

  if (result.required_selected_lane_count >
          execution_budget.maximum_selected_lane_count ||
      result.required_selected_family_count >
          execution_budget.maximum_selected_family_count ||
      result.required_selected_arm_seed_count >
          execution_budget.maximum_selected_arm_seed_count ||
      result.required_selected_key_point_reference_count >
          execution_budget.maximum_selected_key_point_reference_count) {
    return fail(
        std::move(result), BuildFailure::batch_budget_exhausted);
  }
  result.source_exact_batch_and_chunk_joined = true;
  result.source_lanes_share_one_exact_batch = true;
  result.counters.selected_lane_reference_count =
      result.required_selected_lane_count;

  try {
    std::vector<SelectedArm> selected_arms;
    selected_arms.reserve(required_arm_seed_count);
    std::array<std::size_t, 5U> actual_family_counts{};
    std::array<std::size_t, 5U> actual_arm_counts{};
    std::size_t arm_seed_cursor = cursor.source_arm_seed_index;
    for (std::size_t family_index = cursor.source_family_index;
         family_index < *family_end;
         ++family_index) {
      const ExactDirectSaddleArmFamilyRecord& family =
          source_arm_seed_journal.families[family_index];
      if (family.family_index != family_index ||
          family.journal_batch_index != cursor.source_batch_index ||
          family.order != source_batch.order ||
          family.critical_squared_level != source_batch.squared_level ||
          family.arm_seed_count < 2U ||
          family.arm_seed_count > 4U ||
          family.arm_seed_offset != arm_seed_cursor ||
          !lane_by_support_cardinality[family.arm_seed_count].has_value()) {
        return fail(
            std::move(result), BuildFailure::source_join_inconsistent);
      }
      const std::size_t lane_index =
          *lane_by_support_cardinality[family.arm_seed_count];
      ++actual_family_counts[family.arm_seed_count];
      for (std::size_t local = 0U;
           local < family.arm_seed_count;
           ++local) {
        if (arm_seed_cursor >= *arm_seed_end) {
          return fail(
              std::move(result), BuildFailure::source_join_inconsistent);
        }
        const ExactDirectSaddleArmSeedRecord& arm_seed =
            source_arm_seed_journal.arm_seeds[arm_seed_cursor];
        if (arm_seed.arm_seed_index != arm_seed_cursor ||
            arm_seed.family_index != family_index) {
          return fail(
              std::move(result), BuildFailure::source_join_inconsistent);
        }
        const ExactDirectSaddleArmFacet facet =
            reconstruct_exact_direct_saddle_arm_facet(
                source_facade,
                source_arm_seed_journal,
                arm_seed_cursor);
        selected_arms.push_back(
            {arm_seed_cursor,
             family_index,
             lane_index,
             facet_key_from(
                 facet, cloud.size(), source_batch.order)});
        ++actual_arm_counts[family.arm_seed_count];
        ++arm_seed_cursor;
      }
    }
    if (arm_seed_cursor != *arm_seed_end ||
        selected_arms.size() != required_arm_seed_count) {
      return fail(
          std::move(result), BuildFailure::source_join_inconsistent);
    }
    for (std::size_t lane_index = cursor.source_lane_index;
         lane_index < lane_end;
         ++lane_index) {
      const ExactDirectSparseFacetDescentBatchLane& lane =
          source_plan.lanes[lane_index];
      if (actual_family_counts[lane.source_support_cardinality] !=
              lane.matching_family_count ||
          actual_arm_counts[lane.source_support_cardinality] !=
              lane.matching_arm_seed_count ||
          lane.candidate_arm_seed_end_index != *arm_seed_end) {
        return fail(
            std::move(result), BuildFailure::source_join_inconsistent);
      }
    }

    result.counters.selected_family_scan_count =
        result.required_selected_family_count;
    result.counters.selected_arm_seed_scan_count =
        result.required_selected_arm_seed_count;
    result.counters.reconstructed_facet_count =
        result.required_selected_arm_seed_count;
    result.selected_families_scanned_once = true;
    result.every_arm_seed_selected_once_in_source_order = true;
    result.facets_reconstructed_on_demand_only = true;

    std::vector<ExactDirectSparseFacetKey> distinct_keys;
    distinct_keys.reserve(selected_arms.size());
    for (const SelectedArm& arm : selected_arms) {
      distinct_keys.push_back(arm.source_facet_key);
    }
    std::sort(distinct_keys.begin(), distinct_keys.end(), key_less);
    distinct_keys.erase(
        std::unique(distinct_keys.begin(), distinct_keys.end()),
        distinct_keys.end());
    result.required_resolved_key_count = distinct_keys.size();
    result.counters.distinct_closure_seed_count = distinct_keys.size();
    result.counters.duplicate_arm_key_reference_count =
        selected_arms.size() - distinct_keys.size();
    result.distinct_full_keys_canonicalized = true;
    if (distinct_keys.size() >
        execution_budget.maximum_resolved_key_count) {
      return fail(
          std::move(result), BuildFailure::batch_budget_exhausted);
    }
    result.batch_budget_preflight_satisfied = true;

    if (integrated_run_hook != nullptr) {
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp
          live_preclosure_locator_stamp = locator.snapshot_stamp();
      if (live_preclosure_locator_stamp !=
          result.locator_snapshot_stamp) {
        return fail(
            std::move(result), BuildFailure::shared_closure_rejected);
      }
      proposal_transcript =
          integrated_run_hook->prepare_and_seal(
              result,
              cloud,
              live_preclosure_locator_stamp,
              distinct_keys);
      if (proposal_transcript == nullptr) {
        return fail(
            std::move(result), BuildFailure::shared_closure_rejected);
      }
    }

    if (distinct_keys.empty()) {
      if (proposal_transcript != nullptr) {
        CompactClosureProjection closure_projection =
            build_compact_closure_projection(
                index,
                cloud,
                distinct_keys,
                cursor.source_batch_index,
                source_batch.squared_level,
                locator_query_witness,
                locator,
                closure_budget,
                closure_config,
                traversal_order,
                proposal_transcript,
                proposal_evidence);
        result.locator_snapshot_stamp =
            closure_projection.locator_snapshot_stamp;
        result.transient_closure_released_before_delta_publication = true;
        if (closure_projection.failure.has_value()) {
          return fail(
              std::move(result), *closure_projection.failure);
        }
        if (!closure_projection.resolved_keys.empty()) {
          return fail(
              std::move(result),
              BuildFailure::shared_closure_contradiction);
        }
      }
      result.closure_summary.disposition =
          ExactDirectSparseFacetDescentClosureDisposition::
              relative_positive;
      result.closure_summary.decision =
          ExactDirectSparseFacetDescentClosureDecision::
              complete_empty_seed_set;
      result.closure_summary.complete_relative_positive = true;
      result.closure_summary.graph_payload_persisted = false;
      if (proposal_transcript == nullptr) {
        result.locator_snapshot_stamp = locator.snapshot_stamp();
      }
      result.one_shared_closure_and_memo_built_or_empty_batch = true;
      result.common_frozen_locator_snapshot_certified =
          locator.snapshot_stamp() == result.locator_snapshot_stamp;
      result.shared_closure_complete_relative_positive = true;
      result.transient_closure_released_before_delta_publication = true;
      if (!result.common_frozen_locator_snapshot_certified) {
        throw std::runtime_error(
            "the locator changed after empty proposal consumption");
      }
    } else {
      CompactClosureProjection closure_projection =
          build_compact_closure_projection(
              index,
              cloud,
              distinct_keys,
              cursor.source_batch_index,
              source_batch.squared_level,
              locator_query_witness,
              locator,
              closure_budget,
              closure_config,
              traversal_order,
              proposal_transcript,
              proposal_evidence);
      result.closure_summary = closure_projection.summary;
      result.locator_snapshot_stamp =
          closure_projection.locator_snapshot_stamp;
      result.counters.shared_closure_build_count =
          closure_projection.closure_build_count;
      result.transient_closure_released_before_delta_publication = true;
      if (closure_projection.failure.has_value()) {
        return fail(
            std::move(result), *closure_projection.failure);
      }
      result.resolved_keys =
          std::move(closure_projection.resolved_keys);
      std::vector<ExactDirectSparseFacetKey>{}.swap(distinct_keys);
      result.counters.resolved_key_projection_count =
          result.resolved_keys.size();
      result.one_shared_closure_and_memo_built_or_empty_batch = true;
      result.common_frozen_locator_snapshot_certified =
          locator.snapshot_stamp() == result.locator_snapshot_stamp;
      result.shared_closure_complete_relative_positive =
          result.closure_summary.complete_relative_positive;
      if (!result.common_frozen_locator_snapshot_certified) {
        throw std::runtime_error(
            "the locator changed after compact closure projection");
      }
    }

    result.arm_joins.reserve(selected_arms.size());
    for (const SelectedArm& arm : selected_arms) {
      const auto found = std::lower_bound(
          result.resolved_keys.begin(),
          result.resolved_keys.end(),
          arm.source_facet_key,
          [](const ExactDirectSparseFacetDescentBatchResolvedKey& resolved,
             const ExactDirectSparseFacetKey& key) {
            return key_less(resolved.source_facet_key, key);
          });
      if (found == result.resolved_keys.end() ||
          found->source_facet_key != arm.source_facet_key) {
        return fail(
            std::move(result), BuildFailure::shared_closure_contradiction);
      }
      result.arm_joins.push_back(
          {arm.arm_seed_index,
           arm.family_index,
           arm.lane_index,
           found->resolved_key_index,
           true});
    }
    result.counters.arm_join_count = result.arm_joins.size();
    result.every_arm_joined_by_identity_and_full_key = true;
    result.batch_delta_ready_before_commit = true;
    result.decision =
        ExactDirectSparseFacetDescentBatchExecutionDecision::
            complete_architecture_only_relative_positive_batch_delta;
    return result;
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  }
}

void account_transient_work(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result,
    ExactDirectSparseFacetDescentBatchExecutionSessionAudit& audit) {
  const std::optional<std::size_t> next_closure_build_count =
      checked_add(
          audit.transient_closure_build_count,
          result.counters.shared_closure_build_count);
  if (!next_closure_build_count.has_value()) {
    throw std::overflow_error(
        "an anchored batch-execution audit counter overflows size_t");
  }
  audit.transient_closure_build_count = *next_closure_build_count;
  audit.maximum_transient_closure_node_count =
      std::max(
          audit.maximum_transient_closure_node_count,
          result.closure_summary.transient_node_count);
  audit.retained_closure_node_count = 0U;
  audit.closure_graph_retained_between_batches = false;
}

void increment_audit_counter(std::size_t& counter) {
  const std::optional<std::size_t> next = checked_add(counter, 1U);
  if (!next.has_value()) {
    throw std::overflow_error(
        "an anchored batch-execution audit counter overflows size_t");
  }
  counter = *next;
}

[[nodiscard]]
ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
finalize_proposal_preparation(
    const BatchCursor& source_cursor,
    const BatchCursor& live_cursor,
    ExactDirectSparseFacetDescentBatchExecutionResult batch_execution,
    ProposalPreparationEvidence proposal_evidence,
    ExactDirectSparseFacetDescentBatchExecutionSessionAudit& audit) {
  increment_audit_counter(audit.prepare_attempt_count);
  increment_audit_counter(audit.proposal_prepare_attempt_count);
  account_transient_work(batch_execution, audit);

  ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult result;
  result.source_batch_index = source_cursor.source_batch_index;
  result.closed_batch_squared_level =
      batch_execution.closed_batch_squared_level;
  result.locator_snapshot_stamp =
      batch_execution.locator_snapshot_stamp;
  result.batch_execution_decision = batch_execution.decision;
  result.transcript_consumption_requested = true;
  result.batch_preflight_completed_before_proposal_consumption =
      proposal_evidence.consumption_attempted &&
      batch_execution.batch_budget_preflight_completed &&
      batch_execution.batch_budget_preflight_satisfied;
  result.transcript_validation_completed_synchronously =
      proposal_evidence.validation_completed_before_closure;
  result.exact_proposal_consumption_certified =
      proposal_evidence.exact_consumption_certified;
  result.atomic_transcript_rejection_certified =
      proposal_evidence.atomic_rejection_certified;
  result.no_closure_constructed_on_transcript_rejection =
      proposal_evidence.no_closure_constructed_on_rejection;
  result.scientific_delta_separated_from_proposal_audit =
      proposal_evidence
          .scientific_closure_separated_from_consumption_audit;
  result.no_transcript_record_candidate_partition_or_shell_persisted =
      proposal_evidence
          .no_transcript_or_top_k_payload_persisted_in_closure &&
      (!proposal_evidence.consumption_audit.has_value() ||
       (!proposal_evidence.consumption_audit
             ->transcript_payload_persisted &&
        !proposal_evidence.consumption_audit
             ->top_k_partition_or_shell_persisted));
  result.no_proposal_payload_or_audit_retained_by_session = true;
  result.standard_commit_replays_unseeded_exact_path = true;
  result.operational_audit_has_no_scientific_authority = true;
  result.unseeded_commit_readiness_claimed = false;
  result.scientific_commit_performed = false;
  result.locator_state_mutated = false;
  result.hierarchy_reduction_or_attachment_published = false;
  result.gpu_execution_qualified = false;
  result.sub_second_latency_claimed = false;
  result.ten_million_point_capacity_claimed = false;
  result.public_status_claimed = false;
  result.scope =
      ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
          one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;

  if (proposal_evidence.consumption_attempted) {
    increment_audit_counter(audit.proposal_consumption_attempt_count);
    if (proposal_evidence.consumption_audit.has_value()) {
      const std::optional<std::size_t>
          next_proposal_closure_call_count = checked_add(
              audit.proposal_exact_closure_call_count,
              proposal_evidence.consumption_audit
                  ->closure_build_count);
      if (!next_proposal_closure_call_count.has_value()) {
        throw std::overflow_error(
            "an anchored proposal-closure audit counter overflows size_t");
      }
      audit.proposal_exact_closure_call_count =
          *next_proposal_closure_call_count;
    }
    result.proposal_consumption_decision =
        proposal_evidence.consumption_decision;
    result.proposal_consumption_audit =
        std::move(proposal_evidence.consumption_audit);
    if (proposal_evidence.atomic_rejection_certified) {
      increment_audit_counter(audit.proposal_transcript_rejection_count);
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_transcript_rejected_before_closure;
    } else if (proposal_evidence.exact_consumption_certified &&
               batch_execution.complete_architecture_execution()) {
      result.scientific_delta.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              complete_architecture_only_scientific_delta_with_separate_proposal_audit;
    } else if (proposal_evidence.exact_consumption_certified) {
      result.batch_diagnostic.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_batch_diagnostic_after_exact_proposal_consumption;
    } else {
      result.batch_diagnostic.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_batch_diagnostic_during_proposal_consumption;
    }
  } else {
    result.batch_diagnostic.emplace(std::move(batch_execution));
    result.decision =
        ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
            no_preparation_batch_diagnostic_before_proposal_consumption;
  }

  audit.retained_proposal_record_count = 0U;
  audit.proposal_payload_or_audit_retained_between_calls = false;
  result.preparation_left_session_cursor_unchanged =
      same_cursor(source_cursor, live_cursor);
  return result;
}

}  // namespace

bool ExactDirectSparseFacetDescentBatchExecutionResult::
complete_architecture_execution() const noexcept {
  const bool empty_batch =
      required_selected_arm_seed_count == 0U;
  const bool closure_shape =
      one_shared_closure_and_memo_built_or_empty_batch &&
      common_frozen_locator_snapshot_certified &&
      shared_closure_complete_relative_positive &&
      closure_summary.complete_relative_positive &&
      !closure_summary.graph_payload_persisted &&
      counters.shared_closure_build_count == (empty_batch ? 0U : 1U) &&
      closure_summary.transient_seed_projection_count ==
          required_resolved_key_count;
  return schema_version ==
             direct_sparse_facet_descent_batch_executor_schema_version &&
         source_plan_verified_once_at_session_open &&
         !source_plan_rebuilt_for_this_batch &&
         source_exact_batch_and_chunk_joined &&
         source_lanes_share_one_exact_batch &&
         batch_budget_preflight_completed &&
         batch_budget_preflight_satisfied &&
         selected_families_scanned_once &&
         every_arm_seed_selected_once_in_source_order &&
         facets_reconstructed_on_demand_only &&
         distinct_full_keys_canonicalized &&
         closure_shape &&
         every_arm_joined_by_identity_and_full_key &&
         transient_closure_released_before_delta_publication &&
         !closure_graph_persisted &&
         batch_delta_ready_before_commit &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !partial_closure_prefix_published_as_delta &&
         !facet_coface_cell_gamma_or_delaunay_materialized &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         resolved_keys.size() == required_resolved_key_count &&
         arm_joins.size() == required_selected_arm_seed_count &&
         counters.anchored_source_plan_verification_count == 1U &&
         counters.batch_local_source_plan_rebuild_count == 0U &&
         counters.selected_lane_reference_count ==
             required_selected_lane_count &&
         counters.selected_family_scan_count ==
             required_selected_family_count &&
         counters.selected_arm_seed_scan_count ==
             required_selected_arm_seed_count &&
         counters.reconstructed_facet_count ==
             required_selected_arm_seed_count &&
         counters.distinct_closure_seed_count ==
             required_resolved_key_count &&
         counters.resolved_key_projection_count ==
             required_resolved_key_count &&
         counters.arm_join_count == required_selected_arm_seed_count &&
         decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 complete_architecture_only_relative_positive_batch_delta &&
         scope ==
             ExactDirectSparseFacetDescentBatchExecutionScope::
                 one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only &&
         architecture_execution_payload_shape_is_consistent(*this);
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    complete_architecture_preparation() const noexcept {
  if (!scientific_delta.has_value() ||
      batch_diagnostic.has_value() ||
      !proposal_consumption_audit.has_value()) {
    return false;
  }
  const ExactDirectSparseFacetDescentBatchExecutionResult& delta =
      *scientific_delta;
  const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
      proposal_audit = *proposal_consumption_audit;
  return schema_version ==
             direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version &&
         source_batch_index == delta.source_batch_index &&
         source_batch_index == proposal_audit.source_batch_index &&
         closed_batch_squared_level ==
             delta.closed_batch_squared_level &&
         closed_batch_squared_level ==
             proposal_audit.closed_batch_squared_level &&
         locator_snapshot_stamp == delta.locator_snapshot_stamp &&
         locator_snapshot_stamp ==
             proposal_audit.locator_snapshot_stamp &&
         batch_execution_decision == delta.decision &&
         batch_execution_decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 complete_architecture_only_relative_positive_batch_delta &&
         proposal_consumption_decision ==
             ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision::
                 complete_exact_closure_with_proposal_consumption_audit &&
         delta.complete_architecture_execution() &&
         proposal_audit.schema_version ==
             direct_sparse_facet_descent_closure_top_k_proposal_consumption_schema_version &&
         proposal_audit.closed_batch_squared_level ==
             delta.closed_batch_squared_level &&
         proposal_audit.locator_snapshot_stamp ==
             delta.locator_snapshot_stamp &&
         proposal_audit.canonical_seed_key_count ==
             delta.required_resolved_key_count &&
         proposal_audit.closure_build_count == 1U &&
         proposal_audit.top_k_query_count ==
             delta.closure_summary.counters.aggregate_step_counters
                 .top_k_query_count &&
         proposal_consumption_audit_consistent_with_delta(
             proposal_audit, delta) &&
         proposal_audit.transcript_complete_revalidated &&
         proposal_audit
             .metadata_matches_requested_batch_level_and_live_locator &&
         proposal_audit.canonical_seed_keys_revalidated &&
         proposal_audit.record_keys_are_canonical_seed_subset &&
         proposal_audit.candidate_point_domains_revalidated &&
         proposal_audit.locator_snapshot_stable_during_atomic_validation &&
         proposal_audit
             .every_top_k_query_used_exact_source_facet_baseline &&
         proposal_audit.nonempty_records_passed_only_as_proposals &&
         proposal_audit
             .empty_missing_and_dynamic_fallbacks_used_empty_proposal_pool &&
         proposal_audit.exact_pool_and_spatial_work_accounted &&
         !proposal_audit.transcript_payload_persisted &&
         !proposal_audit.top_k_partition_or_shell_persisted &&
         !proposal_audit.scientific_decision_taken_from_proposal &&
         !proposal_audit.locator_state_mutated &&
         !proposal_audit.public_status_claimed &&
         transcript_consumption_requested &&
         batch_preflight_completed_before_proposal_consumption &&
         transcript_validation_completed_synchronously &&
         exact_proposal_consumption_certified &&
         !atomic_transcript_rejection_certified &&
         !no_closure_constructed_on_transcript_rejection &&
         scientific_delta_separated_from_proposal_audit &&
         no_transcript_record_candidate_partition_or_shell_persisted &&
         no_proposal_payload_or_audit_retained_by_session &&
         preparation_left_session_cursor_unchanged &&
         standard_commit_replays_unseeded_exact_path &&
         operational_audit_has_no_scientific_authority &&
         !unseeded_commit_readiness_claimed &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
                 complete_architecture_only_scientific_delta_with_separate_proposal_audit &&
         scope ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
                 one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    certified_atomic_transcript_rejection() const noexcept {
  if (scientific_delta.has_value() ||
      batch_diagnostic.has_value() ||
      !proposal_consumption_audit.has_value()) {
    return false;
  }
  const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
      proposal_audit = *proposal_consumption_audit;
  return schema_version ==
             direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version &&
         source_batch_index == proposal_audit.source_batch_index &&
         closed_batch_squared_level ==
             proposal_audit.closed_batch_squared_level &&
         locator_snapshot_stamp ==
             proposal_audit.locator_snapshot_stamp &&
         batch_execution_decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_shared_closure_rejected &&
         known_atomic_transcript_rejection_decision(
             proposal_consumption_decision) &&
         proposal_audit.schema_version ==
             direct_sparse_facet_descent_closure_top_k_proposal_consumption_schema_version &&
         proposal_rejection_audit_has_no_exact_work(proposal_audit) &&
         !proposal_audit.transcript_payload_persisted &&
         !proposal_audit.top_k_partition_or_shell_persisted &&
         !proposal_audit.scientific_decision_taken_from_proposal &&
         !proposal_audit.locator_state_mutated &&
         !proposal_audit.public_status_claimed &&
         transcript_consumption_requested &&
         batch_preflight_completed_before_proposal_consumption &&
         transcript_validation_completed_synchronously &&
         !exact_proposal_consumption_certified &&
         atomic_transcript_rejection_certified &&
         no_closure_constructed_on_transcript_rejection &&
         !scientific_delta_separated_from_proposal_audit &&
         no_transcript_record_candidate_partition_or_shell_persisted &&
         no_proposal_payload_or_audit_retained_by_session &&
         preparation_left_session_cursor_unchanged &&
         standard_commit_replays_unseeded_exact_path &&
         operational_audit_has_no_scientific_authority &&
         !unseeded_commit_readiness_claimed &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
                 no_preparation_transcript_rejected_before_closure &&
         scope ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
                 one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    certified_outcome() const noexcept {
  return complete_architecture_preparation() ||
         certified_atomic_transcript_rejection();
}

bool ExactDirectSparseFacetDescentBatchSealedCommitVerification::
    certified_cursor_advance() const noexcept {
  return schema_version ==
             direct_sparse_facet_descent_batch_sealed_commit_schema_version &&
         ticket_was_valid_and_unconsumed &&
         shared_session_seal_matches && source_epoch_matches &&
         full_source_cursor_matches && locator_snapshot_matches &&
         exact_scientific_delta_provenance_minted_before_commit &&
         prevalidated_successor_cursor_used &&
         !independent_geometry_replay_performed &&
         !closure_budget_consumed_during_commit &&
         !transcript_present_during_commit &&
         !operational_audit_read_for_commit_authority &&
         !locator_or_hierarchy_state_mutated &&
         !forbidden_global_structure_materialized &&
         !scale_or_public_status_claimed &&
         scientific_delta_moved_to_commit_result && ticket_consumed &&
         session_advanced && result_certified &&
         decision ==
             ExactDirectSparseFacetDescentBatchSealedCommitDecision::
                 complete_architecture_only_sealed_exact_delta_cursor_advance &&
         scope ==
             ExactDirectSparseFacetDescentBatchSealedCommitScope::
                 one_in_process_exact_preparation_provenance_cursor_advance_before_hierarchy_commit_only;
}

bool ExactDirectSparseFacetDescentBatchIntegratedRunResult::
    complete_architecture_run() const noexcept {
  if (!sealed_commit.has_value() ||
      preparation_diagnostic.has_value() ||
      !sealed_commit->certified_cursor_advance() ||
      !sealed_commit->scientific_delta.has_value() ||
      !sealed_commit->operational_audit.has_value() ||
      requested_budget.maximum_query_count_per_chunk == 0U ||
      requested_budget.maximum_chunk_count == 0U) {
    return false;
  }
  const ExactDirectSparseFacetDescentBatchExecutionResult& delta =
      *sealed_commit->scientific_delta;
  const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
      consumption_audit = *sealed_commit->operational_audit;
  const std::optional<std::size_t> maximum_partitioned_queries =
      checked_multiply(
          requested_budget.maximum_query_count_per_chunk,
          requested_budget.maximum_chunk_count);
  const std::optional<std::size_t> rounded_query_count = checked_add(
      audit.proposal_query_count,
      requested_budget.maximum_query_count_per_chunk - 1U);
  const std::optional<std::size_t> expected_h2d_bytes =
      checked_multiply(
          audit.gpu_supported_query_count,
          phase14_active_query_record_byte_count);
  const std::optional<std::size_t> expected_output_bytes =
      checked_multiply(
          audit.gpu_supported_query_count,
          phase14_active_output_record_byte_count);
  if (!maximum_partitioned_queries.has_value() ||
      !rounded_query_count.has_value() ||
      !expected_h2d_bytes.has_value() ||
      !expected_output_bytes.has_value()) {
    return false;
  }
  const std::size_t expected_chunk_count =
      audit.proposal_query_count == 0U
          ? 0U
          : *rounded_query_count /
                requested_budget.maximum_query_count_per_chunk;

  std::optional<std::uint64_t> accounted_duration =
      checked_add_duration(
          audit.cpu_preflight_duration_ns,
          audit.exact_center_preparation_duration_ns);
  if (accounted_duration.has_value()) {
    accounted_duration = checked_add_duration(
        *accounted_duration,
        audit.prepare_inputs_duration_ns);
  }
  if (accounted_duration.has_value()) {
    accounted_duration = checked_add_duration(
        *accounted_duration, audit.seal_inputs_duration_ns);
  }
  if (accounted_duration.has_value()) {
    accounted_duration = checked_add_duration(
        *accounted_duration,
        audit.exact_preparation_duration_ns);
  }
  if (accounted_duration.has_value()) {
    accounted_duration = checked_add_duration(
        *accounted_duration, audit.sealed_commit_duration_ns);
  }

  return schema_version ==
             direct_sparse_facet_descent_batch_integrated_run_schema_version &&
         source_batch_index == delta.source_batch_index &&
         audit.selected_arm_seed_count ==
             delta.required_selected_arm_seed_count &&
         audit.distinct_source_facet_key_count ==
             delta.required_resolved_key_count &&
         audit.proposal_query_count ==
             audit.distinct_source_facet_key_count &&
         audit.proposal_query_capacity ==
             requested_budget.maximum_query_count_per_chunk &&
         audit.proposal_query_count <=
             *maximum_partitioned_queries &&
         audit.proposal_chunk_count == expected_chunk_count &&
         audit.proposal_chunk_count <=
             requested_budget.maximum_chunk_count &&
         audit.sealed_proposal_record_count ==
             consumption_audit.transcript_record_count &&
         audit.sealed_proposal_record_count <=
             audit.distinct_source_facet_key_count &&
         audit.gpu_supported_query_count <=
             audit.proposal_query_count &&
         audit.exact_top_k_query_count ==
             consumption_audit.top_k_query_count &&
         audit.exact_proposal_center_count ==
             audit.distinct_source_facet_key_count &&
         audit.exact_proposal_miniball_build_count ==
             audit.distinct_source_facet_key_count &&
         audit.prepare_inputs_callback_count ==
             audit.proposal_chunk_count &&
         audit.seal_inputs_callback_count == 1U &&
         audit.active_host_to_device_query_record_count ==
             audit.gpu_supported_query_count &&
         audit.initialized_device_output_record_count ==
             audit.gpu_supported_query_count &&
         audit.copied_device_to_host_record_count ==
             audit.gpu_supported_query_count &&
         audit.active_host_to_device_query_byte_count ==
             *expected_h2d_bytes &&
         audit.initialized_device_output_byte_count ==
             *expected_output_bytes &&
         audit.copied_device_to_host_byte_count ==
             *expected_output_bytes &&
         accounted_duration.has_value() &&
         audit.total_run_duration_ns >= *accounted_duration &&
         audit.maximum_live_ticket_count == 1U &&
         audit.live_ticket_count_at_return == 0U &&
         audit.cpu_batch_preflights_completed_before_callbacks &&
         audit.canonical_queries_partitioned_once &&
         audit.every_prepare_inputs_chunk_certified &&
         audit.aggregate_records_canonical_and_bounded &&
         audit.transcript_sealed_once &&
         audit.exact_preparation_consumed_sealed_transcript &&
         audit.exact_centers_prepared_once_for_proposal &&
         !audit.exact_center_reused_by_closure &&
         audit.private_ticket_never_exposed_to_caller &&
         audit.at_most_one_live_ticket &&
         audit.immediate_sealed_commit_attempted &&
         !audit.independent_commit_replay_performed &&
         audit.active_traffic_only_reported &&
         !audit.warm_e2e_measured_or_claimed &&
         !audit.sub_second_latency_claimed &&
         !audit.ten_million_point_capacity_claimed &&
         !audit.forbidden_global_structure_materialized &&
         !audit.public_status_claimed &&
         !cursor_unchanged_on_rejection &&
         no_ticket_live_at_return &&
         decision ==
             ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
                 complete_architecture_only_integrated_proposal_sealed_commit;
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::PreparedTopKProposalBatch::
    PreparedTopKProposalBatch(
        std::shared_ptr<const std::byte> session_seal,
        std::size_t source_epoch,
        std::size_t source_batch_index,
        std::size_t source_chunk_index,
        std::size_t source_lane_index,
        std::size_t source_family_index,
        std::size_t source_arm_seed_index,
        std::size_t successor_batch_index,
        std::size_t successor_chunk_index,
        std::size_t successor_lane_index,
        std::size_t successor_family_index,
        std::size_t successor_arm_seed_index,
        ExactDirectSparsePositiveFacetLocatorSnapshotStamp
            locator_snapshot_stamp,
        ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
            preparation) noexcept
    : session_seal_(std::move(session_seal)),
      source_epoch_(source_epoch),
      source_batch_index_(source_batch_index),
      source_chunk_index_(source_chunk_index),
      source_lane_index_(source_lane_index),
      source_family_index_(source_family_index),
      source_arm_seed_index_(source_arm_seed_index),
      successor_batch_index_(successor_batch_index),
      successor_chunk_index_(successor_chunk_index),
      successor_lane_index_(successor_lane_index),
      successor_family_index_(successor_family_index),
      successor_arm_seed_index_(successor_arm_seed_index),
      locator_snapshot_stamp_(std::move(locator_snapshot_stamp)),
      preparation_(std::move(preparation)),
      exact_scientific_delta_provenance_minted_(true),
      valid_(true) {
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult>);
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::PreparedTopKProposalBatch::
    PreparedTopKProposalBatch(PreparedTopKProposalBatch&& other) noexcept
    : session_seal_(std::move(other.session_seal_)),
      source_epoch_(other.source_epoch_),
      source_batch_index_(other.source_batch_index_),
      source_chunk_index_(other.source_chunk_index_),
      source_lane_index_(other.source_lane_index_),
      source_family_index_(other.source_family_index_),
      source_arm_seed_index_(other.source_arm_seed_index_),
      successor_batch_index_(other.successor_batch_index_),
      successor_chunk_index_(other.successor_chunk_index_),
      successor_lane_index_(other.successor_lane_index_),
      successor_family_index_(other.successor_family_index_),
      successor_arm_seed_index_(other.successor_arm_seed_index_),
      locator_snapshot_stamp_(std::move(other.locator_snapshot_stamp_)),
      preparation_(std::move(other.preparation_)),
      exact_scientific_delta_provenance_minted_(
          std::exchange(
              other.exact_scientific_delta_provenance_minted_,
              false)),
      valid_(std::exchange(other.valid_, false)),
      consumed_(other.consumed_) {
  other.consumed_ = true;
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::PreparedTopKProposalBatch&
ExactDirectSparseFacetDescentAnchoredBatchExecutor::PreparedTopKProposalBatch::
operator=(PreparedTopKProposalBatch&& other) noexcept {
  if (this != &other) {
    session_seal_ = std::move(other.session_seal_);
    source_epoch_ = other.source_epoch_;
    source_batch_index_ = other.source_batch_index_;
    source_chunk_index_ = other.source_chunk_index_;
    source_lane_index_ = other.source_lane_index_;
    source_family_index_ = other.source_family_index_;
    source_arm_seed_index_ = other.source_arm_seed_index_;
    successor_batch_index_ = other.successor_batch_index_;
    successor_chunk_index_ = other.successor_chunk_index_;
    successor_lane_index_ = other.successor_lane_index_;
    successor_family_index_ = other.successor_family_index_;
    successor_arm_seed_index_ = other.successor_arm_seed_index_;
    locator_snapshot_stamp_ =
        std::move(other.locator_snapshot_stamp_);
    preparation_ = std::move(other.preparation_);
    exact_scientific_delta_provenance_minted_ =
        std::exchange(
            other.exact_scientific_delta_provenance_minted_,
            false);
    valid_ = std::exchange(other.valid_, false);
    consumed_ = other.consumed_;
    other.consumed_ = true;
  }
  return *this;
}

std::optional<
    ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
ExactDirectSparseFacetDescentAnchoredBatchExecutor::PreparedTopKProposalBatch::
    take_operational_audit() noexcept {
  return std::exchange(
      preparation_.proposal_consumption_audit,
      std::nullopt);
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    ExactDirectSparseFacetDescentAnchoredBatchExecutor(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        const ExactDirectSupportTerminalFacade& source_facade,
        const ExactDirectMorseEventJournalResult& source_event_journal,
        const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
        const ExactDirectSaddleArmSeedJournalResult&
            source_arm_seed_journal,
        const ExactDirectMorseIndustrialPlanConfig& industrial_config,
        const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
        const ExactDirectSparseFacetDescentBatchPlanResult& observed_plan,
        const ExactDirectSparsePositiveFacetLocator& locator,
        const ExactDirectSparseFacetDescentClosureConfig& closure_config,
        spatial::LbvhTraversalOrder traversal_order)
    : index_(&index),
      cloud_(&cloud),
      source_facade_(&source_facade),
      source_event_journal_(&source_event_journal),
      trusted_arm_seed_budget_(trusted_arm_seed_budget),
      source_arm_seed_journal_(&source_arm_seed_journal),
      industrial_config_(industrial_config),
      plan_budget_(plan_budget),
      locator_(&locator),
      closure_config_(closure_config),
      traversal_order_(traversal_order),
      session_seal_(
          std::make_shared<const std::byte>(std::byte{0U})) {
  require_valid_traversal_order(traversal_order_);
  if (!index_->validated_for(*cloud_)) {
    throw std::invalid_argument(
        "an anchored batch executor requires a matching LBVH authority");
  }
  if (!locator_->certified_positive_locator()) {
    throw std::invalid_argument(
        "an anchored batch executor requires a certified locator");
  }
  source_plan_ = build_exact_direct_sparse_facet_descent_batch_plan(
      *cloud_,
      *source_facade_,
      *source_event_journal_,
      trusted_arm_seed_budget_,
      *source_arm_seed_journal_,
      industrial_config_,
      plan_budget_);
  if (!source_plan_.complete_architecture_plan() ||
      source_plan_ != observed_plan) {
    throw std::invalid_argument(
        "an anchored batch executor requires one freshly matching 14C plan");
  }
  audit_.source_plan_verification_count = 1U;
  audit_.source_plan_owned_by_session = true;
  audit_.full_source_plan_replayed_per_batch = false;
  audit_.closure_graph_retained_between_batches = false;
  audit_.proposal_payload_or_audit_retained_between_calls = false;
  audit_.sealed_ticket_or_delta_retained_by_session = false;
}

bool ExactDirectSparseFacetDescentAnchoredBatchExecutor::complete()
    const noexcept {
  return next_source_batch_index_ ==
         source_event_journal_->batches.size();
}

ExactDirectSparseFacetDescentBatchExecutionResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::prepare_next(
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparseFacetDescentBatchExecutionBudget&
        execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget) {
  if (complete()) {
    throw std::logic_error(
        "a complete anchored batch executor has no next batch");
  }
  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ExactDirectSparseFacetDescentBatchExecutionResult result =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          nullptr,
          nullptr,
          nullptr);
  increment_audit_counter(audit_.prepare_attempt_count);
  account_transient_work(result, audit_);
  return result;
}

ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    prepare_next_with_top_k_proposal_transcript(
        const ExactDirectSparseFacetWitness& locator_query_witness,
        const ExactDirectSparseFacetDescentBatchExecutionBudget&
            execution_budget,
        const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
        const ExactDirectSparseFacetTopKProposalTranscriptResult&
            transcript) {
  if (complete()) {
    throw std::logic_error(
        "a complete anchored batch executor has no next batch");
  }
  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ProposalPreparationEvidence proposal_evidence;
  ExactDirectSparseFacetDescentBatchExecutionResult batch_execution =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          &transcript,
          &proposal_evidence,
          nullptr);
  const BatchCursor live_cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  return finalize_proposal_preparation(
      cursor,
      live_cursor,
      std::move(batch_execution),
      std::move(proposal_evidence),
      audit_);
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    PreparedTopKProposalBatch
ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    prepare_next_sealed_with_top_k_proposal_transcript(
        const ExactDirectSparseFacetWitness& locator_query_witness,
        const ExactDirectSparseFacetDescentBatchExecutionBudget&
            execution_budget,
        const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
        const ExactDirectSparseFacetTopKProposalTranscriptResult&
            transcript) {
  const BatchCursor source_cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
      preparation = prepare_next_with_top_k_proposal_transcript(
          locator_query_witness,
          execution_budget,
          closure_budget,
          transcript);
  increment_audit_counter(audit_.sealed_ticket_prepare_attempt_count);
  audit_.sealed_ticket_or_delta_retained_by_session = false;

  if (!preparation.complete_architecture_preparation() ||
      !preparation.scientific_delta.has_value()) {
    return PreparedTopKProposalBatch{std::move(preparation)};
  }
  const std::optional<BatchCursor> successor_cursor =
      prevalidated_successor_cursor(
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          source_cursor,
          *preparation.scientific_delta);
  if (!successor_cursor.has_value()) {
    return PreparedTopKProposalBatch{std::move(preparation)};
  }

  increment_audit_counter(audit_.sealed_ticket_issued_count);
  return PreparedTopKProposalBatch{
      session_seal_,
      source_epoch_,
      source_cursor.source_batch_index,
      source_cursor.source_chunk_index,
      source_cursor.source_lane_index,
      source_cursor.source_family_index,
      source_cursor.source_arm_seed_index,
      successor_cursor->source_batch_index,
      successor_cursor->source_chunk_index,
      successor_cursor->source_lane_index,
      successor_cursor->source_family_index,
      successor_cursor->source_arm_seed_index,
      preparation.scientific_delta->locator_snapshot_stamp,
      std::move(preparation)};
}

ExactDirectSparseFacetDescentBatchSealedCommitResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    commit_prepared_ticket(
        PreparedTopKProposalBatch&& prepared) noexcept {
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentBatchExecutionResult>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>);

  ExactDirectSparseFacetDescentBatchSealedCommitResult result;
  auto& verification = result.verification;
  verification.source_batch_index = prepared.source_batch_index_;
  verification.successor_batch_index =
      prepared.successor_batch_index_;
  verification.scope =
      ExactDirectSparseFacetDescentBatchSealedCommitScope::
          one_in_process_exact_preparation_provenance_cursor_advance_before_hierarchy_commit_only;
  verification.ticket_was_valid_and_unconsumed =
      prepared.valid_ && !prepared.consumed_;
  verification.shared_session_seal_matches =
      verification.ticket_was_valid_and_unconsumed &&
      prepared.session_seal_ &&
      prepared.session_seal_ == session_seal_;
  verification.source_epoch_matches =
      verification.shared_session_seal_matches &&
      prepared.source_epoch_ == source_epoch_;
  const BatchCursor live_cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  const BatchCursor ticket_source_cursor{
      prepared.source_batch_index_,
      prepared.source_chunk_index_,
      prepared.source_lane_index_,
      prepared.source_family_index_,
      prepared.source_arm_seed_index_};
  verification.full_source_cursor_matches =
      verification.source_epoch_matches &&
      same_cursor(live_cursor, ticket_source_cursor) &&
      !complete();
  verification.locator_snapshot_matches =
      verification.full_source_cursor_matches &&
      locator_->snapshot_stamp() ==
          prepared.locator_snapshot_stamp_;
  verification
      .exact_scientific_delta_provenance_minted_before_commit =
      verification.locator_snapshot_matches &&
      prepared.exact_scientific_delta_provenance_minted_ &&
      prepared.preparation_.scientific_delta.has_value();

  const auto consume_ticket = [&prepared, &verification]() noexcept {
    prepared.session_seal_.reset();
    prepared.exact_scientific_delta_provenance_minted_ = false;
    prepared.valid_ = false;
    prepared.consumed_ = true;
    verification.ticket_consumed = true;
  };
  const auto reject_ticket =
      [this, &verification, &consume_ticket](
          ExactDirectSparseFacetDescentBatchSealedCommitDecision
              decision) noexcept {
        if (audit_.sealed_ticket_rejected_commit_count !=
            std::numeric_limits<std::size_t>::max()) {
          ++audit_.sealed_ticket_rejected_commit_count;
        } else {
          decision =
              ExactDirectSparseFacetDescentBatchSealedCommitDecision::
                  no_commit_audit_capacity_exhausted;
        }
        verification.decision = decision;
        consume_ticket();
      };

  if (audit_.sealed_ticket_commit_attempt_count ==
      std::numeric_limits<std::size_t>::max()) {
    verification.decision =
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_audit_capacity_exhausted;
    consume_ticket();
    return result;
  }
  ++audit_.sealed_ticket_commit_attempt_count;

  if (!verification.ticket_was_valid_and_unconsumed) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_invalid_moved_or_consumed_ticket);
    return result;
  }
  if (!verification.shared_session_seal_matches) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_foreign_session);
    return result;
  }
  if (!verification.source_epoch_matches ||
      !verification.full_source_cursor_matches) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_stale_epoch_or_cursor);
    return result;
  }
  if (!verification.locator_snapshot_matches) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_locator_snapshot_changed);
    return result;
  }
  if (!verification
           .exact_scientific_delta_provenance_minted_before_commit) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_invalid_moved_or_consumed_ticket);
    return result;
  }
  if (audit_.accepted_batch_count ==
          std::numeric_limits<std::size_t>::max() ||
      audit_.sealed_ticket_accepted_commit_count ==
          std::numeric_limits<std::size_t>::max() ||
      audit_.sealed_ticket_exact_replay_avoided_count ==
          std::numeric_limits<std::size_t>::max() ||
      source_epoch_ == std::numeric_limits<std::size_t>::max()) {
    reject_ticket(
        ExactDirectSparseFacetDescentBatchSealedCommitDecision::
            no_commit_audit_capacity_exhausted);
    return result;
  }

  result.scientific_delta.emplace(
      std::move(*prepared.preparation_.scientific_delta));
  prepared.preparation_.scientific_delta.reset();
  if (prepared.preparation_.proposal_consumption_audit.has_value()) {
    result.operational_audit.emplace(
        std::move(
            *prepared.preparation_.proposal_consumption_audit));
    prepared.preparation_.proposal_consumption_audit.reset();
  }

  next_source_batch_index_ = prepared.successor_batch_index_;
  next_source_chunk_index_ = prepared.successor_chunk_index_;
  next_source_lane_index_ = prepared.successor_lane_index_;
  next_source_family_index_ = prepared.successor_family_index_;
  next_source_arm_seed_index_ =
      prepared.successor_arm_seed_index_;
  ++source_epoch_;
  ++audit_.accepted_batch_count;
  ++audit_.sealed_ticket_accepted_commit_count;
  ++audit_.sealed_ticket_exact_replay_avoided_count;
  audit_.sealed_ticket_or_delta_retained_by_session = false;

  verification.prevalidated_successor_cursor_used = true;
  verification.independent_geometry_replay_performed = false;
  verification.closure_budget_consumed_during_commit = false;
  verification.transcript_present_during_commit = false;
  verification.operational_audit_read_for_commit_authority = false;
  verification.locator_or_hierarchy_state_mutated = false;
  verification.forbidden_global_structure_materialized = false;
  verification.scale_or_public_status_claimed = false;
  verification.scientific_delta_moved_to_commit_result = true;
  verification.session_advanced = true;
  verification.result_certified = true;
  verification.decision =
      ExactDirectSparseFacetDescentBatchSealedCommitDecision::
          complete_architecture_only_sealed_exact_delta_cursor_advance;
  consume_ticket();
  return result;
}

ExactDirectSparseFacetDescentBatchIntegratedRunResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::run_next(
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparseFacetDescentBatchExecutionBudget&
        execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentBatchIntegratedRunBudget&
        run_budget,
    const ExactDirectSparseFacetDescentBatchPrepareInputsCallback&
        prepare_inputs,
    const ExactDirectSparseFacetDescentBatchSealInputsCallback&
        seal_inputs,
    const ExactDirectSparseFacetDescentBatchNanosecondClock& clock) {
  if (complete()) {
    throw std::logic_error(
        "a complete anchored batch executor has no next integrated run");
  }
  if (!prepare_inputs || !seal_inputs) {
    throw std::invalid_argument(
        "an integrated batch run requires prepare-inputs and seal-inputs callbacks");
  }

  require_execution_budget_within_confidence(execution_budget);
  require_closure_budget_within_confidence(closure_budget);
  NanosecondClockReader clock_reader{clock};
  const std::uint64_t run_started_ns = clock_reader.read();
  const BatchCursor source_cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};

  ExactDirectSparseFacetDescentBatchIntegratedRunResult result;
  result.source_batch_index = source_cursor.source_batch_index;
  result.requested_budget = run_budget;
  IntegratedRunHook hook{
      &run_budget,
      &prepare_inputs,
      &seal_inputs,
      &clock_reader,
      &result.audit,
      run_started_ns,
      0U,
      IntegratedRunHookFailure::none,
      std::nullopt};
  ProposalPreparationEvidence proposal_evidence;
  ExactDirectSparseFacetDescentBatchExecutionResult batch_execution =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          source_cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          nullptr,
          &proposal_evidence,
          &hook);
  const BatchCursor live_after_preparation{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
      preparation = finalize_proposal_preparation(
          source_cursor,
          live_after_preparation,
          std::move(batch_execution),
          std::move(proposal_evidence),
          audit_);
  const std::uint64_t preparation_completed_ns =
      clock_reader.read();
  if (result.audit.transcript_sealed_once) {
    result.audit.exact_preparation_duration_ns =
        preparation_completed_ns - hook.seal_completed_ns;
  }
  if (preparation.proposal_consumption_audit.has_value()) {
    result.audit.exact_top_k_query_count =
        preparation.proposal_consumption_audit
            ->top_k_query_count;
  }
  result.audit.exact_preparation_consumed_sealed_transcript =
      result.audit.transcript_sealed_once &&
      preparation.transcript_validation_completed_synchronously;
  increment_audit_counter(audit_.sealed_ticket_prepare_attempt_count);
  audit_.sealed_ticket_or_delta_retained_by_session = false;

  const auto finish_rejection =
      [&]() {
        const BatchCursor live_cursor{
            next_source_batch_index_,
            next_source_chunk_index_,
            next_source_lane_index_,
            next_source_family_index_,
            next_source_arm_seed_index_};
        result.cursor_unchanged_on_rejection =
            same_cursor(source_cursor, live_cursor);
        result.audit.live_ticket_count_at_return = 0U;
        result.no_ticket_live_at_return = true;
        const std::uint64_t run_completed_ns =
            clock_reader.read();
        result.audit.total_run_duration_ns =
            run_completed_ns - run_started_ns;
      };

  switch (hook.failure) {
    case IntegratedRunHookFailure::run_budget_rejected:
      result.decision =
          ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
              no_run_cpu_batch_preflight_rejected;
      result.preparation_diagnostic.emplace(
          std::move(preparation));
      finish_rejection();
      return result;
    case IntegratedRunHookFailure::prepare_inputs_rejected:
      result.decision =
          ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
              no_run_prepare_inputs_rejected;
      result.preparation_diagnostic.emplace(
          std::move(preparation));
      finish_rejection();
      return result;
    case IntegratedRunHookFailure::seal_inputs_rejected:
      result.decision =
          ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
              no_run_seal_inputs_rejected;
      result.preparation_diagnostic.emplace(
          std::move(preparation));
      finish_rejection();
      return result;
    case IntegratedRunHookFailure::none:
      break;
  }
  if (!result.audit.transcript_sealed_once) {
    result.decision =
        ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
            no_run_cpu_batch_preflight_rejected;
    result.preparation_diagnostic.emplace(std::move(preparation));
    finish_rejection();
    return result;
  }
  if (!preparation.complete_architecture_preparation() ||
      !preparation.scientific_delta.has_value()) {
    result.decision =
        ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
            no_run_exact_preparation_rejected;
    result.preparation_diagnostic.emplace(std::move(preparation));
    finish_rejection();
    return result;
  }

  const std::optional<BatchCursor> successor_cursor =
      prevalidated_successor_cursor(
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          source_cursor,
          *preparation.scientific_delta);
  if (!successor_cursor.has_value()) {
    result.decision =
        ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
            no_run_ticket_not_issued;
    result.preparation_diagnostic.emplace(std::move(preparation));
    finish_rejection();
    return result;
  }

  increment_audit_counter(audit_.sealed_ticket_issued_count);
  PreparedTopKProposalBatch ticket{
      session_seal_,
      source_epoch_,
      source_cursor.source_batch_index,
      source_cursor.source_chunk_index,
      source_cursor.source_lane_index,
      source_cursor.source_family_index,
      source_cursor.source_arm_seed_index,
      successor_cursor->source_batch_index,
      successor_cursor->source_chunk_index,
      successor_cursor->source_lane_index,
      successor_cursor->source_family_index,
      successor_cursor->source_arm_seed_index,
      preparation.scientific_delta->locator_snapshot_stamp,
      std::move(preparation)};
  result.audit.maximum_live_ticket_count = 1U;
  result.audit.immediate_sealed_commit_attempted = true;
  const std::uint64_t commit_started_ns = clock_reader.read();
  ExactDirectSparseFacetDescentBatchSealedCommitResult commit =
      commit_prepared_ticket(std::move(ticket));
  const std::uint64_t commit_completed_ns = clock_reader.read();
  result.audit.sealed_commit_duration_ns =
      commit_completed_ns - commit_started_ns;
  result.audit.live_ticket_count_at_return = 0U;
  result.audit.independent_commit_replay_performed =
      commit.verification.independent_geometry_replay_performed;
  result.no_ticket_live_at_return = true;
  result.sealed_commit.emplace(std::move(commit));
  const std::uint64_t run_completed_ns = clock_reader.read();
  result.audit.total_run_duration_ns =
      run_completed_ns - run_started_ns;

  if (result.sealed_commit->certified_cursor_advance()) {
    result.cursor_unchanged_on_rejection = false;
    result.decision =
        ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
            complete_architecture_only_integrated_proposal_sealed_commit;
  } else {
    const BatchCursor live_cursor{
        next_source_batch_index_,
        next_source_chunk_index_,
        next_source_lane_index_,
        next_source_family_index_,
        next_source_arm_seed_index_};
    result.cursor_unchanged_on_rejection =
        same_cursor(source_cursor, live_cursor);
    result.decision =
        ExactDirectSparseFacetDescentBatchIntegratedRunDecision::
            no_run_sealed_commit_rejected;
  }
  return result;
}

ExactDirectSparseFacetDescentBatchExecutionVerification
ExactDirectSparseFacetDescentAnchoredBatchExecutor::commit_prepared(
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparseFacetDescentBatchExecutionBudget&
        execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentBatchExecutionResult& observed) {
  ExactDirectSparseFacetDescentBatchExecutionVerification verification;
  require_execution_budget_within_confidence(execution_budget);
  require_closure_budget_within_confidence(closure_budget);
  verification.observed_storage_within_budget =
      observed_storage_within_budget(observed, execution_budget);
  verification.anchored_source_plan_reused_without_full_replay =
      audit_.source_plan_verification_count == 1U &&
      !audit_.full_source_plan_replayed_per_batch &&
      audit_.source_plan_owned_by_session;
  if (!verification.observed_storage_within_budget || complete()) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    return verification;
  }

  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  const ExactDirectSparseFacetDescentBatchExecutionResult expected =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          nullptr,
          nullptr,
          nullptr);
  increment_audit_counter(audit_.fresh_batch_replay_count);
  account_transient_work(expected, audit_);

  verification.locator_snapshot_matches_observed_build =
      observed.locator_snapshot_stamp ==
          expected.locator_snapshot_stamp &&
      locator_->snapshot_stamp() == expected.locator_snapshot_stamp;
  verification.exact_batch_execution_freshly_replayed = true;
  verification.exact_result_equality_certified = observed == expected;
  verification.transient_closure_released_before_comparison =
      expected.transient_closure_released_before_delta_publication &&
      !expected.closure_graph_persisted &&
      audit_.retained_closure_node_count == 0U;
  verification.no_partial_delta_or_external_mutation =
      !expected.partial_closure_prefix_published_as_delta &&
      !expected.locator_state_mutated &&
      !expected.scientific_commit_performed &&
      !expected.hierarchy_reduction_or_attachment_published;
  verification.no_forbidden_global_structure_materialized =
      !expected.facet_coface_cell_gamma_or_delaunay_materialized;
  verification.no_scale_or_public_status_claimed =
      !expected.gpu_execution_qualified &&
      !expected.sub_second_latency_claimed &&
      !expected.ten_million_point_capacity_claimed &&
      !expected.public_status_claimed;
  verification.diagnostic_result_certified =
      verification.anchored_source_plan_reused_without_full_replay &&
      verification.locator_snapshot_matches_observed_build &&
      verification.exact_batch_execution_freshly_replayed &&
      verification.exact_result_equality_certified &&
      verification.transient_closure_released_before_comparison &&
      verification.no_partial_delta_or_external_mutation &&
      verification.no_forbidden_global_structure_materialized &&
      verification.no_scale_or_public_status_claimed;
  verification.result_certified =
      verification.diagnostic_result_certified;

  if (!verification.result_certified ||
      !expected.complete_architecture_execution()) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    return verification;
  }

  const std::optional<BatchCursor> successor_cursor =
      prevalidated_successor_cursor(
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          expected);
  if (!successor_cursor.has_value() ||
      source_epoch_ == std::numeric_limits<std::size_t>::max()) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    verification.result_certified = false;
    return verification;
  }

  increment_audit_counter(audit_.accepted_batch_count);
  next_source_batch_index_ = successor_cursor->source_batch_index;
  next_source_chunk_index_ = successor_cursor->source_chunk_index;
  next_source_lane_index_ = successor_cursor->source_lane_index;
  next_source_family_index_ = successor_cursor->source_family_index;
  next_source_arm_seed_index_ =
      successor_cursor->source_arm_seed_index;
  ++source_epoch_;
  verification.session_advanced = true;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
