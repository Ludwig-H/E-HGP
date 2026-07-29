#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"
#include "morsehgp3d/hierarchy/symmetric_grouped_anchored_pair_prune_receipt.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::MortonLbvhIndex;
using spatial::PointId;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(message);
  }
  return left * right;
}

[[nodiscard]] bool sum_equals(
    std::size_t left,
    std::size_t right,
    std::size_t expected) noexcept {
  return right <= std::numeric_limits<std::size_t>::max() - left &&
      left + right == expected;
}

[[nodiscard]] bool symmetric_coverage_equals(
    std::size_t pruned_directed_pair_count,
    std::size_t classified_unordered_pair_count,
    std::size_t self_pair_count,
    std::size_t expected_directed_pair_count) noexcept {
  if (classified_unordered_pair_count >
      std::numeric_limits<std::size_t>::max() / 2U) {
    return false;
  }
  const std::size_t classified_directed_pair_count =
      2U * classified_unordered_pair_count;
  if (classified_directed_pair_count >
          std::numeric_limits<std::size_t>::max() -
              pruned_directed_pair_count ||
      self_pair_count >
          std::numeric_limits<std::size_t>::max() -
              pruned_directed_pair_count - classified_directed_pair_count) {
    return false;
  }
  return pruned_directed_pair_count + classified_directed_pair_count +
      self_pair_count == expected_directed_pair_count;
}

[[nodiscard]] bool triangular_unordered_universe(
    std::size_t point_count,
    std::size_t& unordered_pair_count) noexcept {
  if (point_count == 0U) {
    return false;
  }
  const std::size_t left =
      point_count % 2U == 0U ? point_count / 2U : point_count;
  const std::size_t right = point_count % 2U == 0U
      ? point_count - 1U
      : (point_count - 1U) / 2U;
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  unordered_pair_count = left * right;
  return true;
}

[[nodiscard]] bool three_sum_equals(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t expected) noexcept {
  return second <= std::numeric_limits<std::size_t>::max() - first &&
      third <= std::numeric_limits<std::size_t>::max() - first - second &&
      first + second + third == expected;
}

[[nodiscard]] bool sum_covers(
    std::size_t first,
    std::size_t second,
    std::size_t value) noexcept {
  return value <= first || value - first <= second;
}

[[nodiscard]] bool can_add_within(
    std::size_t current,
    std::size_t increment,
    std::size_t capacity) noexcept {
  return current <= capacity && increment <= capacity - current;
}

[[nodiscard]] bool bounded_multiple(
    std::size_t value,
    std::size_t count,
    std::size_t bound) noexcept {
  return count <= std::numeric_limits<std::size_t>::max() / bound &&
      value <= count * bound;
}

[[nodiscard]] std::size_t remaining_capacity(
    std::size_t current,
    std::size_t capacity,
    const char* message) {
  if (current > capacity) {
    throw std::logic_error(message);
  }
  return capacity - current;
}

[[nodiscard]] ExactSparseAnchoredPairSessionStopReason map_candidate_stop(
    ExactMortonGroupedAnchoredPairCandidateStopReason reason) {
  switch (reason) {
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        schedule_advance_limit:
      return ExactSparseAnchoredPairSessionStopReason::schedule_advance_limit;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        orientation_check_limit:
      return ExactSparseAnchoredPairSessionStopReason::orientation_check_limit;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        grouped_traversal_node_visit_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          grouped_traversal_node_visit_limit;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        grouped_traversal_exact_predicate_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          grouped_traversal_exact_predicate_limit;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::none:
      break;
  }
  throw std::logic_error(
      "a sparse anchored pair candidate stop has no typed reason");
}

[[nodiscard]] ExactSparseAnchoredPairSessionStopReason
candidate_total_capacity_reason(
    ExactMortonGroupedAnchoredPairCandidateStopReason reason) {
  switch (reason) {
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        schedule_advance_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          total_schedule_advance_capacity;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        orientation_check_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          total_orientation_check_capacity;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        grouped_traversal_node_visit_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          total_grouped_traversal_node_visit_capacity;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::
        grouped_traversal_exact_predicate_limit:
      return ExactSparseAnchoredPairSessionStopReason::
          total_grouped_traversal_exact_predicate_capacity;
    case ExactMortonGroupedAnchoredPairCandidateStopReason::none:
      break;
  }
  throw std::logic_error(
      "a sparse anchored pair total candidate stop has no typed reason");
}

[[nodiscard]] std::size_t record_point_id_reference_count(
    const ExactSparseAnchoredPairRecord& record) {
  if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
    return checked_add(
        2U,
        event->interior_ids.size(),
        "the sparse anchored pair event reference count overflows size_t");
  }
  const auto& diagnostic =
      std::get<ExactPairSupportExtraShellDiagnostic>(record);
  return checked_add(
      3U,
      diagnostic.interior_ids.size(),
      "the sparse anchored pair diagnostic reference count overflows size_t");
}

[[nodiscard]] ExactSparseAnchoredPairRecordKind record_kind(
    const ExactSparseAnchoredPairRecord& record) noexcept {
  return std::holds_alternative<ExactPairSupportEvent>(record)
      ? ExactSparseAnchoredPairRecordKind::event
      : ExactSparseAnchoredPairRecordKind::
            relevant_extra_shell_diagnostic;
}

[[nodiscard]] std::array<PointId, 2> record_support_ids(
    const ExactSparseAnchoredPairRecord& record) noexcept {
  if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
    return event->support_ids;
  }
  return std::get<ExactPairSupportExtraShellDiagnostic>(record).support_ids;
}

struct RecordPopulation {
  std::size_t record_count{};
  std::size_t event_count{};
  std::size_t diagnostic_count{};
  std::size_t point_id_reference_count{};
};

[[nodiscard]] bool observe_record_population(
    std::span<const ExactSparseAnchoredPairRecord> records,
    RecordPopulation& population) noexcept {
  population = {};
  population.record_count = records.size();
  for (const ExactSparseAnchoredPairRecord& record : records) {
    std::size_t reference_count = 0U;
    if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      if (population.event_count ==
              std::numeric_limits<std::size_t>::max() ||
          event->interior_ids.size() >
              std::numeric_limits<std::size_t>::max() - 2U) {
        return false;
      }
      ++population.event_count;
      reference_count = 2U + event->interior_ids.size();
    } else {
      const auto& diagnostic =
          std::get<ExactPairSupportExtraShellDiagnostic>(record);
      if (population.diagnostic_count ==
              std::numeric_limits<std::size_t>::max() ||
          diagnostic.interior_ids.size() >
              std::numeric_limits<std::size_t>::max() - 3U) {
        return false;
      }
      ++population.diagnostic_count;
      reference_count = 3U + diagnostic.interior_ids.size();
    }
    if (reference_count >
        std::numeric_limits<std::size_t>::max() -
            population.point_id_reference_count) {
      return false;
    }
    population.point_id_reference_count += reference_count;
  }
  return sum_equals(
      population.event_count,
      population.diagnostic_count,
      population.record_count);
}

[[nodiscard]] bool cumulative_record_partition_holds(
    const ExactSparseAnchoredPairSessionAudit& audit,
    const RecordPopulation& released,
    const RecordPopulation& resident) noexcept {
  return sum_equals(
             released.event_count,
             released.diagnostic_count,
             released.record_count) &&
      sum_equals(
          released.record_count,
          resident.record_count,
          audit.emitted_record_count) &&
      sum_equals(
          released.event_count,
          resident.event_count,
          audit.accepted_event_count) &&
      sum_equals(
          released.diagnostic_count,
          resident.diagnostic_count,
          audit.relevant_extra_shell_diagnostic_count) &&
      sum_equals(
          released.point_id_reference_count,
          resident.point_id_reference_count,
          audit.emitted_point_id_reference_count);
}

[[nodiscard]] bool released_prefix_matches_resident_size(
    const ExactSparseAnchoredPairSessionAudit& audit,
    const RecordPopulation& released,
    std::size_t resident_record_count) noexcept {
  return sum_equals(
             audit.accepted_event_count,
             audit.relevant_extra_shell_diagnostic_count,
             audit.emitted_record_count) &&
      sum_equals(
          released.event_count,
          released.diagnostic_count,
          released.record_count) &&
      released.event_count <= audit.accepted_event_count &&
      released.diagnostic_count <=
          audit.relevant_extra_shell_diagnostic_count &&
      released.point_id_reference_count <=
          audit.emitted_point_id_reference_count &&
      sum_equals(
          released.record_count,
          resident_record_count,
          audit.emitted_record_count);
}

[[nodiscard]] bool raw_terminal_invariants_hold(
    const ExactMortonGroupedAnchoredPairCandidateContext& cursor,
    const ExactSparseAnchoredPairSessionTotalCapacity& capacity,
    const ExactSparseAnchoredPairSessionAudit& audit,
    const RecordPopulation& released,
    std::span<const ExactSparseAnchoredPairRecord> records) noexcept {
  RecordPopulation resident;
  if (!observe_record_population(records, resident) ||
      !cumulative_record_partition_holds(audit, released, resident)) {
    return false;
  }
  const bool triangular =
      cursor.schedule_config().use_triangular_block_pair_schedule;
  const ExactMortonGroupedAnchoredPairScheduleAudit& schedule =
      cursor.schedule_audit();
  const bool physical_candidate_partition_holds = triangular
      ? cursor.audit().candidate_pair_count ==
              cursor.audit().orientation_check_count &&
          cursor.audit().reverse_or_self_orientation_skip_count == 0U
      : sum_equals(
            cursor.audit().candidate_pair_count,
            cursor.audit().reverse_or_self_orientation_skip_count,
            cursor.audit().orientation_check_count);
  const bool directed_coverage_holds = triangular
      ? symmetric_coverage_equals(
            audit.authenticated_pruned_directed_pair_count,
            cursor.audit().candidate_pair_count,
            schedule.triangular_self_pair_count,
            audit.directed_pair_universe_size)
      : sum_equals(
            audit.authenticated_pruned_directed_pair_count,
            cursor.audit().orientation_check_count,
            audit.directed_pair_universe_size);
  if (!cursor.complete() || audit.poisoned || audit.point_count == 0U ||
      audit.directed_pair_universe_size / audit.point_count !=
          audit.point_count ||
      audit.directed_pair_universe_size % audit.point_count != 0U ||
      audit.maximum_live_candidate_count !=
          (audit.admitted_candidate_count == 0U ? 0U : 1U) ||
      audit.authenticated_prune_count !=
          cursor.audit().certified_prune_count ||
      cursor.audit().candidate_pair_count != audit.admitted_candidate_count ||
      audit.candidate_started_count != audit.admitted_candidate_count ||
      audit.admitted_candidate_count != audit.classification_terminal_count ||
      !sum_equals(
          audit.above_rank_count,
          audit.emitted_record_count,
          audit.classification_terminal_count) ||
      !sum_equals(
          audit.accepted_event_count,
          audit.relevant_extra_shell_diagnostic_count,
          audit.emitted_record_count) ||
      audit.candidate_record_ready_count != audit.emitted_record_count ||
      !physical_candidate_partition_holds || !directed_coverage_holds) {
    return false;
  }

  if (!cursor.audit().complete ||
      !cursor.audit().no_dynamic_candidate_or_output_arena_materialized ||
      !schedule.complete || !schedule.morton_anchor_partition_complete ||
      !schedule.no_global_anchor_pair_or_output_arena_materialized ||
      schedule.scheduled_anchor_count != audit.point_count ||
      cursor.audit().schedule_advance_count != schedule.advance_call_count ||
      cursor.audit().grouped_traversal_node_visit_count !=
          schedule.traversal_node_visit_count ||
      cursor.audit().grouped_traversal_exact_predicate_count !=
          schedule.exact_predicate_count ||
      cursor.audit().certified_prune_count !=
          schedule.certified_prune_count ||
      cursor.audit().opened_unresolved_leaf_range_count !=
          schedule.unresolved_leaf_count ||
      cursor.audit().opened_fallback_subtree_range_count !=
          schedule.fallback_subtree_count ||
      !three_sum_equals(
          schedule.common_traversal_node_visit_count,
          schedule.anchor_subgroup_node_visit_count,
          schedule.singleton_node_visit_count,
          schedule.traversal_node_visit_count) ||
      !three_sum_equals(
          schedule.common_exact_predicate_count,
          schedule.anchor_subgroup_exact_predicate_count,
          schedule.singleton_exact_predicate_count,
          schedule.exact_predicate_count) ||
      (cursor.schedule_config()
               .use_floating_witness_order_for_triangular_blocks &&
       !three_sum_equals(
           schedule.fp64_filtered_negative_predicate_count,
           schedule.fp64_filtered_positive_predicate_count,
           schedule.exact_fallback_predicate_count,
           schedule.exact_predicate_count)) ||
      schedule.completed_singleton_fallback_count !=
          schedule.prepared_singleton_fallback_count ||
      !bounded_multiple(
          schedule.proposed_anchor_subgroup_witness_pool_entry_count,
          schedule.prepared_anchor_subgroup_probe_count,
          exact_grouped_anchored_pair_maximum_witness_pool_size) ||
      !bounded_multiple(
          schedule.proposed_singleton_witness_pool_entry_count,
          schedule.prepared_singleton_fallback_count,
          exact_grouped_anchored_pair_maximum_witness_pool_size) ||
      !sum_covers(
          schedule.proposed_anchor_subgroup_witness_pool_entry_count,
          schedule.proposed_singleton_witness_pool_entry_count,
          schedule.query_facing_fallback_witness_pool_entry_count) ||
      schedule.maximum_pending_anchor_subgroup_count >
          exact_grouped_anchored_pair_maximum_anchor_count) {
    return false;
  }

  if (triangular) {
    std::size_t unordered_pair_universe = 0U;
    if (!triangular_unordered_universe(
            audit.point_count, unordered_pair_universe) ||
        !schedule.triangular_partition_complete ||
        !schedule.no_dynamic_dual_tree_or_pair_arena_materialized ||
        schedule.prepared_group_count != schedule.completed_group_count ||
        schedule.query_subtree_split_count !=
            schedule.triangular_consumer_query_split_count ||
        schedule.query_subtree_split_count >
            schedule.prepared_anchor_subgroup_probe_count ||
        !sum_equals(
            schedule.triangular_certified_unordered_pair_count,
            schedule.triangular_opened_unordered_pair_count,
            unordered_pair_universe) ||
        schedule.triangular_certified_cross_block_count +
                schedule.triangular_opened_singleton_cross_block_count >
            schedule.triangular_cross_block_count) {
      return false;
    }
  } else if (
      schedule.prepared_group_count != schedule.completed_group_count ||
      cursor.audit().completed_group_count != schedule.completed_group_count ||
      schedule.anchor_subgroup_split_count >
          schedule.prepared_anchor_subgroup_probe_count ||
      schedule.anchor_subgroup_certified_prune_count >
          schedule.prepared_anchor_subgroup_probe_count ||
      !sum_equals(
          schedule.anchor_subgroup_split_count,
          schedule.anchor_subgroup_certified_prune_count,
          schedule.prepared_anchor_subgroup_probe_count) ||
      !sum_equals(
          schedule.anchor_subgroup_certified_anchor_count,
          schedule.prepared_singleton_fallback_count,
          schedule.delegated_frontier_anchor_count)) {
    return false;
  }

  if (cursor.audit().schedule_advance_count >
          capacity.maximum_schedule_advance_count ||
      cursor.audit().orientation_check_count >
          capacity.maximum_orientation_check_count ||
      cursor.audit().grouped_traversal_node_visit_count >
          capacity.maximum_grouped_traversal_node_visit_count ||
      cursor.audit().grouped_traversal_exact_predicate_count >
          capacity.maximum_grouped_traversal_exact_predicate_count ||
      audit.admitted_candidate_count >
          capacity.maximum_admitted_candidate_count ||
      audit.classification_node_visit_count >
          capacity.maximum_classification_node_visit_count ||
      audit.emitted_record_count > capacity.maximum_output_record_count ||
      audit.emitted_point_id_reference_count >
          capacity.maximum_output_point_id_reference_count) {
    return false;
  }

  return true;
}

static_assert(
    std::is_nothrow_move_constructible_v<ExactPairSupportEvent>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactPairSupportExtraShellDiagnostic>);
static_assert(
    std::is_nothrow_move_constructible_v<ExactSparseAnchoredPairRecord>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactAnchoredPairCandidateClassificationResult>);

}  // namespace

ExactSparseAnchoredPairTerminalAuthority::
    ExactSparseAnchoredPairTerminalAuthority(
        ExactMortonGroupedAnchoredPairCandidateContext&& terminal_cursor,
        ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
        ExactSparseAnchoredPairSessionAudit audit,
        std::vector<ExactSparseAnchoredPairRecord>&& records) noexcept
    : terminal_cursor_(std::move(terminal_cursor)),
      total_capacity_(total_capacity),
      audit_(audit),
      records_(std::move(records)) {}

ExactSparseAnchoredPairTerminalAuthority::
    ExactSparseAnchoredPairTerminalAuthority(
        ExactSparseAnchoredPairTerminalAuthority&& other) noexcept
    : terminal_cursor_(std::move(other.terminal_cursor_)),
      total_capacity_(other.total_capacity_),
      audit_(other.audit_),
      records_(std::move(other.records_)) {
  other.terminal_cursor_.reset();
  other.total_capacity_ = {};
  other.audit_ = {};
  other.records_.clear();
}

bool ExactSparseAnchoredPairTerminalAuthority::bound_to(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
    ExactSparseAnchoredPairSessionTotalCapacity total_capacity) const
    noexcept {
  return terminal_cursor_.has_value() &&
      terminal_cursor_->validated_for(index, cloud) &&
      terminal_cursor_->maximum_closed_rank() == maximum_closed_rank &&
      terminal_cursor_->schedule_config() == schedule_config &&
      total_capacity_ == total_capacity;
}

bool ExactSparseAnchoredPairTerminalAuthority::
    sealed_in_process_terminal_authority() const noexcept {
  return terminal_cursor_.has_value() && terminal_cursor_->complete() &&
      audit_.complete && audit_.every_prune_recertified &&
      audit_.directed_coverage_certified &&
      audit_.orientation_partition_certified &&
      audit_.candidate_classification_partition_certified &&
      audit_.output_partition_certified &&
      audit_.retained_records_certified &&
      audit_.no_forbidden_global_structure_materialized &&
      raw_terminal_invariants_hold(
          *terminal_cursor_, total_capacity_, audit_, {}, records_);
}

std::size_t ExactSparseAnchoredPairTerminalAuthority::maximum_closed_rank()
    const noexcept {
  return terminal_cursor_.has_value()
      ? terminal_cursor_->maximum_closed_rank()
      : 0U;
}

const ExactMortonGroupedAnchoredPairScheduleConfig&
ExactSparseAnchoredPairTerminalAuthority::schedule_config() const & noexcept {
  static const ExactMortonGroupedAnchoredPairScheduleConfig empty_config{};
  return terminal_cursor_.has_value()
      ? terminal_cursor_->schedule_config()
      : empty_config;
}

const ExactMortonGroupedAnchoredPairCandidateAudit&
ExactSparseAnchoredPairTerminalAuthority::candidate_audit() const & noexcept {
  static const ExactMortonGroupedAnchoredPairCandidateAudit empty_audit{};
  return terminal_cursor_.has_value()
      ? terminal_cursor_->audit()
      : empty_audit;
}

const ExactMortonGroupedAnchoredPairScheduleAudit&
ExactSparseAnchoredPairTerminalAuthority::schedule_audit() const & noexcept {
  static const ExactMortonGroupedAnchoredPairScheduleAudit empty_audit{};
  return terminal_cursor_.has_value()
      ? terminal_cursor_->schedule_audit()
      : empty_audit;
}

std::vector<ExactSparseAnchoredPairRecord>
ExactSparseAnchoredPairTerminalAuthority::release_records() && {
  if (!sealed_in_process_terminal_authority()) {
    throw std::logic_error(
        "only a sealed sparse anchored authority can release its records");
  }
  std::vector<ExactSparseAnchoredPairRecord> released =
      std::move(records_);
  records_.clear();
  terminal_cursor_.reset();
  audit_ = {};
  total_capacity_ = {};
  return released;
}

ExactSparseAnchoredPairSession ExactSparseAnchoredPairSession::start(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
    ExactSparseAnchoredPairSessionTotalCapacity total_capacity) {
  const std::size_t directed_pair_universe_size = checked_multiply(
      cloud.size(),
      cloud.size(),
      "the sparse anchored directed-pair universe overflows size_t");
  return ExactSparseAnchoredPairSession(
      index,
      cloud,
      maximum_closed_rank,
      schedule_config,
      total_capacity,
      directed_pair_universe_size,
      PrivateConstructionTag{});
}

ExactSparseAnchoredPairSession::ExactSparseAnchoredPairSession(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
    ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
    std::size_t directed_pair_universe_size,
    PrivateConstructionTag)
    : candidate_cursor_(
          ExactMortonGroupedAnchoredPairCandidateContext::start(
              index,
              cloud,
              maximum_closed_rank,
              schedule_config)),
      total_capacity_(total_capacity) {
  audit_.point_count = cloud.size();
  audit_.directed_pair_universe_size = directed_pair_universe_size;
}

bool ExactSparseAnchoredPairSession::ready() const noexcept {
  return !sealed_ && candidate_cursor_.ready();
}

bool ExactSparseAnchoredPairSession::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  return ready() && candidate_cursor_.validated_for(index, cloud) &&
      (!active_classifier_.has_value() ||
       active_classifier_->validated_for(index, cloud));
}

ExactSparseAnchoredPairRecordSegment
ExactSparseAnchoredPairSession::release_unsealed_record_segment() & {
  if (!ready() || audit_.poisoned) {
    throw std::logic_error(
        "only a ready unpoisoned sparse anchored session can release a record segment");
  }

  const RecordPopulation released{
      released_record_count_,
      released_event_count_,
      released_relevant_extra_shell_diagnostic_count_,
      released_point_id_reference_count_};
  RecordPopulation resident;
  if (!observe_record_population(records_, resident) ||
      !cumulative_record_partition_holds(audit_, released, resident)) {
    poison();
    throw std::logic_error(
        "the sparse anchored resident record suffix lost its cumulative audit");
  }

  ExactSparseAnchoredPairRecordSegment segment;
  segment.first_output_record_index = released_record_count_;
  segment.first_output_point_id_reference_index =
      released_point_id_reference_count_;
  segment.event_count = resident.event_count;
  segment.relevant_extra_shell_diagnostic_count =
      resident.diagnostic_count;
  segment.point_id_reference_count =
      resident.point_id_reference_count;
  segment.records.swap(records_);

  released_record_count_ = audit_.emitted_record_count;
  released_event_count_ = audit_.accepted_event_count;
  released_relevant_extra_shell_diagnostic_count_ =
      audit_.relevant_extra_shell_diagnostic_count;
  released_point_id_reference_count_ =
      audit_.emitted_point_id_reference_count;
  record_segment_released_ = true;
  audit_.retained_records_certified = false;
  return segment;
}

ExactSparseAnchoredPairSessionStep
ExactSparseAnchoredPairSession::make_step(
    ExactSparseAnchoredPairSessionStepKind kind,
    ExactSparseAnchoredPairSessionStopReason stop_reason,
    ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
    ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
    ExactSparseAnchoredPairSessionStepWork work) const {
  ExactSparseAnchoredPairSessionStep step;
  step.kind_ = kind;
  step.stop_reason_ = stop_reason;
  step.requested_budget_ = requested_budget;
  step.effective_budget_ = effective_budget;
  step.work_ = work;
  step.session_complete_after_step_ = complete();
  return step;
}

ExactSparseAnchoredPairSessionStep
ExactSparseAnchoredPairSession::exhaust_total_capacity(
    ExactSparseAnchoredPairSessionStopReason reason,
    ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
    ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
    ExactSparseAnchoredPairSessionStepWork work) {
  if (!total_capacity_exhausted_) {
    audit_.total_capacity_exhaustion_count = checked_add(
        audit_.total_capacity_exhaustion_count,
        1U,
        "the sparse anchored capacity-exhaustion count overflows size_t");
    total_capacity_exhausted_ = true;
    total_capacity_stop_reason_ = reason;
  }
  return make_step(
      ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted,
      total_capacity_stop_reason_,
      requested_budget,
      effective_budget,
      work);
}

ExactSparseAnchoredPairSessionStep
ExactSparseAnchoredPairSession::start_pending_candidate(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
    ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
    ExactSparseAnchoredPairSessionStepWork work) {
  if (!pending_support_ids_.has_value() || active_classifier_.has_value()) {
    poison();
    throw std::logic_error(
        "a sparse anchored candidate start lost its exclusive pending pair");
  }
  const std::array<PointId, 2> support_ids = *pending_support_ids_;
  if (audit_.admitted_candidate_count >=
      total_capacity_.maximum_admitted_candidate_count) {
    ExactSparseAnchoredPairSessionStep step = exhaust_total_capacity(
        ExactSparseAnchoredPairSessionStopReason::
            total_admitted_candidate_capacity,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }

  ExactAnchoredPairCandidateClassificationContext classifier =
      ExactAnchoredPairCandidateClassificationContext::start(
          index, cloud, support_ids, maximum_closed_rank());
  active_classifier_.emplace(std::move(classifier));
  pending_support_ids_.reset();
  audit_.admitted_candidate_count = checked_add(
      audit_.admitted_candidate_count,
      1U,
      "the sparse anchored admitted-candidate count overflows size_t");
  audit_.candidate_started_count = checked_add(
      audit_.candidate_started_count,
      1U,
      "the sparse anchored candidate-start count overflows size_t");
  audit_.maximum_live_candidate_count =
      std::max(audit_.maximum_live_candidate_count, std::size_t{1U});
  active_classifier_node_visit_count_ = 0U;

  ExactSparseAnchoredPairSessionStep step = make_step(
      ExactSparseAnchoredPairSessionStepKind::candidate_started,
      ExactSparseAnchoredPairSessionStopReason::none,
      requested_budget,
      effective_budget,
      work);
  step.support_ids_ = support_ids;
  return step;
}

ExactSparseAnchoredPairSessionStep
ExactSparseAnchoredPairSession::advance_active_classifier(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
    ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
    ExactSparseAnchoredPairSessionStepWork work) {
  if (!active_classifier_.has_value() ||
      active_classifier_->record_ready() ||
      active_classifier_->complete()) {
    poison();
    throw std::logic_error(
        "a sparse anchored active advance reached an invalid classifier state");
  }
  const std::array<PointId, 2> support_ids =
      active_classifier_->support_ids();
  if (audit_.classification_node_visit_count >=
      total_capacity_.maximum_classification_node_visit_count) {
    ExactSparseAnchoredPairSessionStep step = exhaust_total_capacity(
        ExactSparseAnchoredPairSessionStopReason::
            total_classification_node_visit_capacity,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }

  const ExactAnchoredPairCandidateClassificationStep classification_step =
      active_classifier_->advance(
          index, cloud, effective_budget.classifier);
  work.classification_node_visit_count =
      classification_step.work.node_visit_count;
  audit_.classification_advance_count = checked_add(
      audit_.classification_advance_count,
      1U,
      "the sparse anchored classification-advance count overflows size_t");
  audit_.classification_node_visit_count = checked_add(
      audit_.classification_node_visit_count,
      work.classification_node_visit_count,
      "the sparse anchored classification-node count overflows size_t");
  active_classifier_node_visit_count_ = checked_add(
      active_classifier_node_visit_count_,
      work.classification_node_visit_count,
      "the sparse anchored active classification-node count overflows size_t");
  if (audit_.classification_node_visit_count >
          total_capacity_.maximum_classification_node_visit_count ||
      active_classifier_node_visit_count_ !=
          active_classifier_->audit().node_visit_count) {
    poison();
    throw std::logic_error(
        "a sparse anchored classifier exceeded or lost its visit accounting");
  }

  switch (classification_step.kind) {
    case ExactAnchoredPairCandidateClassificationStepKind::budget_exhausted: {
      audit_.classification_budget_exhaustion_count = checked_add(
          audit_.classification_budget_exhaustion_count,
          1U,
          "the sparse anchored classification-budget count overflows size_t");
      if (audit_.classification_node_visit_count ==
          total_capacity_.maximum_classification_node_visit_count) {
        ExactSparseAnchoredPairSessionStep step = exhaust_total_capacity(
            ExactSparseAnchoredPairSessionStopReason::
                total_classification_node_visit_capacity,
            requested_budget,
            effective_budget,
            work);
        step.support_ids_ = support_ids;
        return step;
      }
      audit_.budget_exhaustion_count = checked_add(
          audit_.budget_exhaustion_count,
          1U,
          "the sparse anchored budget-exhaustion count overflows size_t");
      ExactSparseAnchoredPairSessionStep step = make_step(
          ExactSparseAnchoredPairSessionStepKind::budget_exhausted,
          ExactSparseAnchoredPairSessionStopReason::
              classification_node_visit_limit,
          requested_budget,
          effective_budget,
          work);
      step.support_ids_ = support_ids;
      return step;
    }
    case ExactAnchoredPairCandidateClassificationStepKind::above_rank: {
      if (!active_classifier_->above_rank() ||
          !active_classifier_->complete() ||
          !active_classifier_->audit().early_above_rank_certificate) {
        poison();
        throw std::logic_error(
            "a sparse anchored above-rank step lost its terminal context");
      }
      audit_.classification_terminal_count = checked_add(
          audit_.classification_terminal_count,
          1U,
          "the sparse anchored classification-terminal count overflows size_t");
      audit_.above_rank_count = checked_add(
          audit_.above_rank_count,
          1U,
          "the sparse anchored above-rank count overflows size_t");
      active_classifier_.reset();
      active_classifier_node_visit_count_ = 0U;
      ExactSparseAnchoredPairSessionStep step = make_step(
          ExactSparseAnchoredPairSessionStepKind::candidate_above_rank,
          ExactSparseAnchoredPairSessionStopReason::none,
          requested_budget,
          effective_budget,
          work);
      step.support_ids_ = support_ids;
      return step;
    }
    case ExactAnchoredPairCandidateClassificationStepKind::below_rank:
      poison();
      throw std::logic_error(
          "a legacy sparse pair session received an exact-bucket below-rank terminal");
    case ExactAnchoredPairCandidateClassificationStepKind::record_ready: {
      if (!active_classifier_->record_ready()) {
        poison();
        throw std::logic_error(
            "a sparse anchored ready step lost its pending exact record");
      }
      audit_.candidate_record_ready_count = checked_add(
          audit_.candidate_record_ready_count,
          1U,
          "the sparse anchored ready-record count overflows size_t");
      ExactSparseAnchoredPairSessionStep step = make_step(
          ExactSparseAnchoredPairSessionStepKind::candidate_record_ready,
          ExactSparseAnchoredPairSessionStopReason::none,
          requested_budget,
          effective_budget,
          work);
      step.support_ids_ = support_ids;
      return step;
    }
    case ExactAnchoredPairCandidateClassificationStepKind::complete:
      break;
  }
  poison();
  throw std::logic_error(
      "a sparse anchored active classifier completed without session commit");
}

ExactSparseAnchoredPairSessionStep
ExactSparseAnchoredPairSession::commit_ready_record(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
    ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
    ExactSparseAnchoredPairSessionStepWork work) {
  if (!active_classifier_.has_value() ||
      !active_classifier_->record_ready()) {
    poison();
    throw std::logic_error(
        "a sparse anchored record commit requires one ready classifier");
  }
  const std::array<PointId, 2> support_ids =
      active_classifier_->support_ids();
  const std::optional<ExactAnchoredPairPendingRecordRequirements>
      pending_requirements =
          active_classifier_->pending_record_requirements();
  if (!pending_requirements.has_value() ||
      pending_requirements->point_id_reference_count < 2U ||
      pending_requirements->point_id_reference_count >
          maximum_closed_rank() + 1U) {
    poison();
    throw std::logic_error(
        "a sparse anchored ready classifier lost its exact output requirement");
  }
  const std::size_t required_record_references =
      pending_requirements->point_id_reference_count;

  if (audit_.emitted_record_count >=
      total_capacity_.maximum_output_record_count) {
    ExactSparseAnchoredPairSessionStep step = exhaust_total_capacity(
        ExactSparseAnchoredPairSessionStopReason::
            total_output_record_capacity,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }
  if (!can_add_within(
          audit_.emitted_point_id_reference_count,
          required_record_references,
          total_capacity_.maximum_output_point_id_reference_count)) {
    ExactSparseAnchoredPairSessionStep step = exhaust_total_capacity(
        ExactSparseAnchoredPairSessionStopReason::
            total_output_point_id_reference_capacity,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }
  if (requested_budget.maximum_emitted_record_count == 0U) {
    audit_.budget_exhaustion_count = checked_add(
        audit_.budget_exhaustion_count,
        1U,
        "the sparse anchored budget-exhaustion count overflows size_t");
    ExactSparseAnchoredPairSessionStep step = make_step(
        ExactSparseAnchoredPairSessionStepKind::budget_exhausted,
        ExactSparseAnchoredPairSessionStopReason::emitted_record_limit,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }
  if (requested_budget.maximum_emitted_point_id_reference_count <
      required_record_references) {
    audit_.budget_exhaustion_count = checked_add(
        audit_.budget_exhaustion_count,
        1U,
        "the sparse anchored budget-exhaustion count overflows size_t");
    ExactSparseAnchoredPairSessionStep step = make_step(
        ExactSparseAnchoredPairSessionStepKind::budget_exhausted,
        ExactSparseAnchoredPairSessionStopReason::
            emitted_point_id_reference_limit,
        requested_budget,
        effective_budget,
        work);
    step.support_ids_ = support_ids;
    return step;
  }

  const std::size_t required_record_count = checked_add(
      records_.size(),
      1U,
      "the sparse anchored retained-record count overflows size_t");
  const RecordPopulation released{
      released_record_count_,
      released_event_count_,
      released_relevant_extra_shell_diagnostic_count_,
      released_point_id_reference_count_};
  if (!released_prefix_matches_resident_size(
          audit_, released, records_.size()) ||
      required_record_count >
          total_capacity_.maximum_output_record_count) {
    poison();
    throw std::logic_error(
        "the sparse anchored retained-record store lost its bounded audit");
  }
  if (records_.capacity() < required_record_count) {
    std::size_t proposed_capacity = records_.capacity();
    if (proposed_capacity == 0U) {
      proposed_capacity = 1U;
    }
    while (proposed_capacity < required_record_count) {
      const std::size_t increment = proposed_capacity / 2U + 1U;
      if (increment >
          total_capacity_.maximum_output_record_count -
              proposed_capacity) {
        proposed_capacity =
            total_capacity_.maximum_output_record_count;
      } else {
        proposed_capacity += increment;
      }
    }
    records_.reserve(proposed_capacity);
  }

  ExactAnchoredPairCandidateClassificationResult result =
      active_classifier_->take_result(index, cloud);
  if (!result.complete() ||
      (result.event.has_value() ==
       result.relevant_extra_shell_diagnostic.has_value()) ||
      result.support_ids != support_ids ||
      result.maximum_closed_rank != maximum_closed_rank() ||
      !result.audit.complete_partition_certified ||
      !result.audit.center_and_level_constructed ||
      result.audit.node_visit_count != active_classifier_node_visit_count_ ||
      result.audit != active_classifier_->audit()) {
    poison();
    throw std::logic_error(
        "a sparse anchored committed result lost its exact terminal proof");
  }

  ExactSparseAnchoredPairRecord record = result.event.has_value()
      ? ExactSparseAnchoredPairRecord{
            std::in_place_type<ExactPairSupportEvent>,
            std::move(*result.event)}
      : ExactSparseAnchoredPairRecord{
            std::in_place_type<ExactPairSupportExtraShellDiagnostic>,
            std::move(*result.relevant_extra_shell_diagnostic)};
  const std::size_t actual_references =
      record_point_id_reference_count(record);
  const ExactSparseAnchoredPairRecordKind kind = record_kind(record);
  const ExactSparseAnchoredPairRecordKind expected_kind =
      pending_requirements->kind == ExactAnchoredPairPendingRecordKind::event
      ? ExactSparseAnchoredPairRecordKind::event
      : ExactSparseAnchoredPairRecordKind::
            relevant_extra_shell_diagnostic;
  if (actual_references != required_record_references ||
      kind != expected_kind || record_support_ids(record) != support_ids ||
      !can_add_within(
          audit_.emitted_point_id_reference_count,
          actual_references,
          total_capacity_.maximum_output_point_id_reference_count)) {
    poison();
    throw std::logic_error(
        "a sparse anchored exact record exceeded its preflight bound");
  }

  ExactSparseAnchoredPairSessionAudit next_audit = audit_;
  next_audit.classification_terminal_count = checked_add(
      next_audit.classification_terminal_count,
      1U,
      "the sparse anchored classification-terminal count overflows size_t");
  next_audit.emitted_record_count = checked_add(
      next_audit.emitted_record_count,
      1U,
      "the sparse anchored emitted-record count overflows size_t");
  next_audit.emitted_point_id_reference_count = checked_add(
      next_audit.emitted_point_id_reference_count,
      actual_references,
      "the sparse anchored emitted-reference count overflows size_t");
  if (kind == ExactSparseAnchoredPairRecordKind::event) {
    next_audit.accepted_event_count = checked_add(
        next_audit.accepted_event_count,
        1U,
        "the sparse anchored event count overflows size_t");
  } else {
    next_audit.relevant_extra_shell_diagnostic_count = checked_add(
        next_audit.relevant_extra_shell_diagnostic_count,
        1U,
        "the sparse anchored diagnostic count overflows size_t");
  }

  const std::size_t output_record_index = audit_.emitted_record_count;
  records_.push_back(std::move(record));
  audit_ = next_audit;
  active_classifier_.reset();
  active_classifier_node_visit_count_ = 0U;
  work.emitted_record_count = 1U;
  work.emitted_point_id_reference_count = actual_references;

  ExactSparseAnchoredPairSessionStep step = make_step(
      ExactSparseAnchoredPairSessionStepKind::record_committed,
      ExactSparseAnchoredPairSessionStopReason::none,
      requested_budget,
      effective_budget,
      work);
  step.support_ids_ = support_ids;
  step.output_record_index_ = output_record_index;
  step.record_kind_ = kind;
  return step;
}

bool ExactSparseAnchoredPairSession::terminal_invariants_hold() const noexcept {
  const RecordPopulation released{
      released_record_count_,
      released_event_count_,
      released_relevant_extra_shell_diagnostic_count_,
      released_point_id_reference_count_};
  return !pending_support_ids_.has_value() &&
      !active_classifier_.has_value() && !total_capacity_exhausted_ &&
      raw_terminal_invariants_hold(
          candidate_cursor_, total_capacity_, audit_, released, records_);
}

ExactSparseAnchoredPairSessionStep ExactSparseAnchoredPairSession::advance(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactSparseAnchoredPairSessionAdvanceBudget budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a sparse anchored pair session advance requires its authentic cloud and LBVH");
  }
  if (audit_.poisoned) {
    throw std::logic_error(
        "a poisoned sparse anchored pair session cannot advance");
  }
  audit_.advance_call_count = checked_add(
      audit_.advance_call_count,
      1U,
      "the sparse anchored session advance count overflows size_t");

  ExactSparseAnchoredPairSessionAdvanceBudget effective_budget = budget;
  const ExactMortonGroupedAnchoredPairCandidateAudit& candidate_audit =
      candidate_cursor_.audit();
  effective_budget.candidate_cursor.maximum_schedule_advance_count = std::min(
      budget.candidate_cursor.maximum_schedule_advance_count,
      remaining_capacity(
          candidate_audit.schedule_advance_count,
          total_capacity_.maximum_schedule_advance_count,
          "the sparse anchored schedule work exceeded total capacity"));
  effective_budget.candidate_cursor.maximum_orientation_check_count = std::min(
      budget.candidate_cursor.maximum_orientation_check_count,
      remaining_capacity(
          candidate_audit.orientation_check_count,
          total_capacity_.maximum_orientation_check_count,
          "the sparse anchored orientation work exceeded total capacity"));
  effective_budget.candidate_cursor
      .maximum_grouped_traversal_node_visit_count = std::min(
      budget.candidate_cursor.maximum_grouped_traversal_node_visit_count,
      remaining_capacity(
          candidate_audit.grouped_traversal_node_visit_count,
          total_capacity_.maximum_grouped_traversal_node_visit_count,
          "the sparse anchored grouped visits exceeded total capacity"));
  effective_budget.candidate_cursor
      .maximum_grouped_traversal_exact_predicate_count = std::min(
      budget.candidate_cursor.maximum_grouped_traversal_exact_predicate_count,
      remaining_capacity(
          candidate_audit.grouped_traversal_exact_predicate_count,
          total_capacity_.maximum_grouped_traversal_exact_predicate_count,
          "the sparse anchored grouped predicates exceeded total capacity"));
  effective_budget.classifier.maximum_node_visit_count = std::min(
      budget.classifier.maximum_node_visit_count,
      remaining_capacity(
          audit_.classification_node_visit_count,
          total_capacity_.maximum_classification_node_visit_count,
          "the sparse anchored classifier visits exceeded total capacity"));
  effective_budget.maximum_emitted_record_count = std::min(
      budget.maximum_emitted_record_count,
      remaining_capacity(
          audit_.emitted_record_count,
          total_capacity_.maximum_output_record_count,
          "the sparse anchored records exceeded total capacity"));
  effective_budget.maximum_emitted_point_id_reference_count = std::min(
      budget.maximum_emitted_point_id_reference_count,
      remaining_capacity(
          audit_.emitted_point_id_reference_count,
          total_capacity_.maximum_output_point_id_reference_count,
          "the sparse anchored references exceeded total capacity"));
  ExactSparseAnchoredPairSessionStepWork work;

  if (complete()) {
    return make_step(
        ExactSparseAnchoredPairSessionStepKind::complete,
        ExactSparseAnchoredPairSessionStopReason::none,
        budget,
        effective_budget,
        work);
  }
  if (total_capacity_exhausted_) {
    return make_step(
        ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted,
        total_capacity_stop_reason_,
        budget,
        effective_budget,
        work);
  }
  if (active_classifier_.has_value() &&
      active_classifier_->record_ready()) {
    return commit_ready_record(
        index, cloud, budget, effective_budget, work);
  }
  if (pending_support_ids_.has_value()) {
    return start_pending_candidate(
        index, cloud, budget, effective_budget, work);
  }
  if (active_classifier_.has_value()) {
    return advance_active_classifier(
        index, cloud, budget, effective_budget, work);
  }

  if (candidate_cursor_.complete()) {
    if (!terminal_invariants_hold()) {
      poison();
      throw std::logic_error(
          "a sparse anchored terminal cursor failed its coverage identities");
    }
    audit_.directed_coverage_certified = true;
    audit_.orientation_partition_certified = true;
    audit_.candidate_classification_partition_certified = true;
    audit_.output_partition_certified = true;
    audit_.retained_records_certified =
        !record_segment_released_;
    audit_.no_forbidden_global_structure_materialized =
        candidate_cursor_.audit()
            .no_dynamic_candidate_or_output_arena_materialized &&
        candidate_cursor_.schedule_audit()
            .no_global_anchor_pair_or_output_arena_materialized;
    audit_.complete = true;
    return make_step(
        ExactSparseAnchoredPairSessionStepKind::complete,
        ExactSparseAnchoredPairSessionStopReason::none,
        budget,
        effective_budget,
        work);
  }

  ExactMortonGroupedAnchoredPairCandidateStep candidate_step =
      candidate_cursor_.advance(
          index, cloud, effective_budget.candidate_cursor);
  work.candidate_cursor = candidate_step.work();
  switch (candidate_step.kind()) {
    case ExactMortonGroupedAnchoredPairCandidateStepKind::candidate_pair: {
      if (!candidate_step.support_ids().has_value() ||
          (*candidate_step.support_ids())[0] >=
              (*candidate_step.support_ids())[1]) {
        poison();
        throw std::logic_error(
            "a sparse anchored session observed an invalid oriented pair");
      }
      pending_support_ids_ = *candidate_step.support_ids();
      return start_pending_candidate(
          index, cloud, budget, effective_budget, work);
    }
    case ExactMortonGroupedAnchoredPairCandidateStepKind::certified_prune: {
      const ExactMortonGroupedAnchoredPairScheduleStep* schedule_step =
          candidate_step.schedule_step();
      const ExactGroupedAnchoredPairTraversalStep* traversal_step =
          schedule_step == nullptr ? nullptr : schedule_step->traversal_step();
      const ExactGroupedAnchoredPairPruneCertificate* certificate =
          traversal_step == nullptr
          ? nullptr
          : traversal_step->prune_certificate();
      if (schedule_step == nullptr || traversal_step == nullptr ||
          certificate == nullptr ||
          schedule_step->kind() !=
              ExactMortonGroupedAnchoredPairScheduleStepKind::
                  certified_prune ||
          traversal_step->kind() !=
              ExactGroupedAnchoredPairTraversalStepKind::certified_prune ||
          !schedule_step->group_ordinal().has_value() ||
          !schedule_step->anchor_leaf_begin().has_value() ||
          !schedule_step->anchor_leaf_end().has_value() ||
          !traversal_step->lbvh_node_index().has_value() ||
          !traversal_step->leaf_begin().has_value() ||
          !traversal_step->leaf_end().has_value() ||
          *schedule_step->anchor_leaf_begin() >=
              *schedule_step->anchor_leaf_end() ||
          *schedule_step->anchor_leaf_end() > cloud.size() ||
          schedule_step->anchor_point_ids().size() !=
              *schedule_step->anchor_leaf_end() -
                  *schedule_step->anchor_leaf_begin() ||
          *traversal_step->leaf_begin() >=
              *traversal_step->leaf_end() ||
          *traversal_step->leaf_end() > cloud.size() ||
          certificate->leaf_begin() != *traversal_step->leaf_begin() ||
          certificate->leaf_end() != *traversal_step->leaf_end() ||
          !certificate->certifies(
              index,
              cloud,
              *traversal_step->lbvh_node_index(),
              maximum_closed_rank(),
              schedule_step->anchor_point_ids())) {
        poison();
        audit_.every_prune_recertified = false;
        throw std::logic_error(
            "a sparse anchored session rejected a foreign or malformed prune");
      }
      std::size_t prune_mass = 0U;
      if (schedule_config().use_triangular_block_pair_schedule) {
        ExactSymmetricGroupedAnchoredPairPruneReceipt symmetric_receipt =
            recertify_exact_symmetric_grouped_anchored_pair_prune(
                index,
                cloud,
                *certificate,
                *schedule_step->anchor_leaf_begin(),
                *schedule_step->anchor_leaf_end(),
                *traversal_step->lbvh_node_index(),
                maximum_closed_rank());
        if (!symmetric_receipt.validated_for(index, cloud)) {
          poison();
          audit_.every_prune_recertified = false;
          throw std::logic_error(
              "a sparse anchored session rejected a symmetric prune receipt");
        }
        prune_mass = symmetric_receipt.directed_mass();
      } else {
        prune_mass = checked_multiply(
            schedule_step->anchor_point_ids().size(),
            *traversal_step->leaf_end() - *traversal_step->leaf_begin(),
            "the sparse anchored directed prune mass overflows size_t");
      }
      if (!can_add_within(
              audit_.authenticated_pruned_directed_pair_count,
              prune_mass,
              audit_.directed_pair_universe_size)) {
        poison();
        throw std::logic_error(
            "a sparse anchored prune exceeds the directed pair universe");
      }
      audit_.authenticated_prune_count = checked_add(
          audit_.authenticated_prune_count,
          1U,
          "the sparse anchored prune count overflows size_t");
      audit_.authenticated_pruned_directed_pair_count = checked_add(
          audit_.authenticated_pruned_directed_pair_count,
          prune_mass,
          "the sparse anchored prune mass overflows size_t");
      work.authenticated_pruned_directed_pair_count = prune_mass;
      ExactSparseAnchoredPairSessionStep step = make_step(
          ExactSparseAnchoredPairSessionStepKind::
              certified_prune_accounted,
          ExactSparseAnchoredPairSessionStopReason::none,
          budget,
          effective_budget,
          work);
      step.group_ordinal_ = schedule_step->group_ordinal();
      return step;
    }
    case ExactMortonGroupedAnchoredPairCandidateStepKind::group_complete: {
      const ExactMortonGroupedAnchoredPairScheduleStep* schedule_step =
          candidate_step.schedule_step();
      if (schedule_step == nullptr ||
          schedule_step->kind() !=
              ExactMortonGroupedAnchoredPairScheduleStepKind::group_complete ||
          !schedule_step->group_ordinal().has_value() ||
          !schedule_step->anchor_leaf_begin().has_value() ||
          !schedule_step->anchor_leaf_end().has_value() ||
          *schedule_step->anchor_leaf_begin() >=
              *schedule_step->anchor_leaf_end() ||
          *schedule_step->anchor_leaf_end() > cloud.size() ||
          schedule_step->anchor_point_ids().size() !=
              *schedule_step->anchor_leaf_end() -
                  *schedule_step->anchor_leaf_begin()) {
        poison();
        throw std::logic_error(
            "a sparse anchored session rejected a malformed group boundary");
      }
      ExactSparseAnchoredPairSessionStep step = make_step(
          ExactSparseAnchoredPairSessionStepKind::group_complete,
          ExactSparseAnchoredPairSessionStopReason::none,
          budget,
          effective_budget,
          work);
      step.group_ordinal_ = schedule_step->group_ordinal();
      return step;
    }
    case ExactMortonGroupedAnchoredPairCandidateStepKind::budget_exhausted: {
      const ExactMortonGroupedAnchoredPairCandidateStopReason nested_reason =
          candidate_step.stop_reason();
      bool total_exhausted = false;
      switch (nested_reason) {
        case ExactMortonGroupedAnchoredPairCandidateStopReason::
            schedule_advance_limit:
          total_exhausted =
              candidate_cursor_.audit().schedule_advance_count ==
              total_capacity_.maximum_schedule_advance_count;
          break;
        case ExactMortonGroupedAnchoredPairCandidateStopReason::
            orientation_check_limit:
          total_exhausted =
              candidate_cursor_.audit().orientation_check_count ==
              total_capacity_.maximum_orientation_check_count;
          break;
        case ExactMortonGroupedAnchoredPairCandidateStopReason::
            grouped_traversal_node_visit_limit:
          total_exhausted = candidate_cursor_.audit()
                                  .grouped_traversal_node_visit_count ==
              total_capacity_.maximum_grouped_traversal_node_visit_count;
          break;
        case ExactMortonGroupedAnchoredPairCandidateStopReason::
            grouped_traversal_exact_predicate_limit:
          total_exhausted = candidate_cursor_.audit()
                                  .grouped_traversal_exact_predicate_count ==
              total_capacity_
                  .maximum_grouped_traversal_exact_predicate_count;
          break;
        case ExactMortonGroupedAnchoredPairCandidateStopReason::none:
          poison();
          throw std::logic_error(
              "a sparse anchored candidate budget stop lost its reason");
      }
      if (total_exhausted) {
        return exhaust_total_capacity(
            candidate_total_capacity_reason(nested_reason),
            budget,
            effective_budget,
            work);
      }
      audit_.budget_exhaustion_count = checked_add(
          audit_.budget_exhaustion_count,
          1U,
          "the sparse anchored budget-exhaustion count overflows size_t");
      return make_step(
          ExactSparseAnchoredPairSessionStepKind::budget_exhausted,
          map_candidate_stop(nested_reason),
          budget,
          effective_budget,
          work);
    }
    case ExactMortonGroupedAnchoredPairCandidateStepKind::complete:
      if (!terminal_invariants_hold()) {
        poison();
        throw std::logic_error(
            "a sparse anchored completed step failed its coverage identities");
      }
      audit_.directed_coverage_certified = true;
      audit_.orientation_partition_certified = true;
      audit_.candidate_classification_partition_certified = true;
      audit_.output_partition_certified = true;
      audit_.retained_records_certified =
          !record_segment_released_;
      audit_.no_forbidden_global_structure_materialized =
          candidate_cursor_.audit()
              .no_dynamic_candidate_or_output_arena_materialized &&
          candidate_cursor_.schedule_audit()
              .no_global_anchor_pair_or_output_arena_materialized;
      audit_.complete = true;
      return make_step(
          ExactSparseAnchoredPairSessionStepKind::complete,
          ExactSparseAnchoredPairSessionStopReason::none,
          budget,
          effective_budget,
          work);
  }
  poison();
  throw std::logic_error(
      "a sparse anchored session observed an unknown candidate step");
}

ExactSparseAnchoredPairTerminalAuthority
ExactSparseAnchoredPairSession::seal() && {
  if (!ready() || sealed_ || !audit_.complete ||
      !audit_.every_prune_recertified ||
      !audit_.directed_coverage_certified ||
      !audit_.orientation_partition_certified ||
      !audit_.candidate_classification_partition_certified ||
      !audit_.output_partition_certified ||
      !audit_.retained_records_certified ||
      !audit_.no_forbidden_global_structure_materialized ||
      !terminal_invariants_hold()) {
    throw std::logic_error(
        "only one complete sparse anchored pair session can mint an authority");
  }
  sealed_ = true;
  return ExactSparseAnchoredPairTerminalAuthority(
      std::move(candidate_cursor_),
      total_capacity_,
      audit_,
      std::move(records_));
}

}  // namespace morsehgp3d::hierarchy
