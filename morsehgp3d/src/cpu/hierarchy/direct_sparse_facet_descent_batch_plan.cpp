#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"

#include <array>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t maximum_structural_lane_count_per_batch = 3U;
[[nodiscard]] constexpr std::size_t binomial_coefficient(
    std::size_t n,
    std::size_t k) noexcept {
  if (k > n) {
    return 0U;
  }
  if (k > n - k) {
    k = n - k;
  }
  std::size_t result = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    result = result * (n - k + index) / index;
  }
  return result;
}

[[nodiscard]] constexpr auto local_miniball_support_candidate_counts()
    noexcept {
  std::array<
      std::size_t,
      ExactFacetMiniballResult::maximum_facet_point_count + 1U>
      counts{};
  for (std::size_t facet_cardinality = 1U;
       facet_cardinality < counts.size();
       ++facet_cardinality) {
    for (std::size_t support_cardinality = 1U;
         support_cardinality <=
         ExactFacetMiniballResult::maximum_support_point_count;
         ++support_cardinality) {
      counts[facet_cardinality] += binomial_coefficient(
          facet_cardinality, support_cardinality);
    }
  }
  return counts;
}

inline constexpr auto local_miniball_support_candidate_count_by_cardinality =
    local_miniball_support_candidate_counts();
static_assert(
    local_miniball_support_candidate_count_by_cardinality.back() ==
    ExactFacetMiniballResult::maximum_enumerated_support_count);

enum class StructuralScanStatus : std::uint8_t {
  complete,
  capacity_overflow,
  inconsistent,
};

struct StructuralScanResult {
  StructuralScanStatus status{StructuralScanStatus::inconsistent};
  std::size_t lane_count{};
  std::size_t matching_family_count{};
  std::size_t matching_arm_seed_count{};
  std::size_t launch_count_upper_bound{};
  std::size_t
      local_miniball_support_examination_count_upper_bound{};

  friend bool operator==(
      const StructuralScanResult&,
      const StructuralScanResult&) = default;
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

[[nodiscard]] std::optional<std::size_t> as_size_t(
    std::uint64_t value) noexcept {
  if constexpr (sizeof(std::size_t) < sizeof(std::uint64_t)) {
    if (value >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
      return std::nullopt;
    }
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::optional<std::size_t> ceil_divide(
    std::size_t numerator,
    std::size_t denominator) noexcept {
  if (denominator == 0U) {
    return std::nullopt;
  }
  const std::size_t quotient = numerator / denominator;
  if (numerator % denominator == 0U) {
    return quotient;
  }
  return checked_add(quotient, 1U);
}

[[nodiscard]] std::optional<std::size_t>
local_miniball_support_examinations_per_work_item(
    std::size_t facet_cardinality) noexcept {
  if (facet_cardinality == 0U ||
      facet_cardinality >=
          local_miniball_support_candidate_count_by_cardinality.size()) {
    return std::nullopt;
  }
  return checked_multiply(
      direct_sparse_facet_descent_step_maximum_fresh_miniball_enumeration_count,
      local_miniball_support_candidate_count_by_cardinality[
          facet_cardinality]);
}

[[nodiscard]] StructuralScanResult scan_structural_classes(
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanResult&
        source_industrial_plan,
    std::size_t maximum_work_item_count_per_launch,
    std::vector<ExactDirectSparseFacetDescentBatchLane>* lanes) {
  StructuralScanResult result;
  if (maximum_work_item_count_per_launch == 0U) {
    return result;
  }

  std::size_t family_cursor = 0U;
  std::size_t arm_seed_cursor = 0U;
  for (std::size_t chunk_index = 0U;
       chunk_index < source_industrial_plan.chunks.size();
       ++chunk_index) {
    const ExactDirectMorseIndustrialChunk& chunk =
        source_industrial_plan.chunks[chunk_index];
    if (chunk.chunk_index != chunk_index ||
        chunk.source_batch_end_index <
            chunk.source_batch_begin_index ||
        chunk.source_batch_end_index >
            source_event_journal.batches.size()) {
      return result;
    }

    for (std::size_t batch_index =
             chunk.source_batch_begin_index;
         batch_index < chunk.source_batch_end_index;
         ++batch_index) {
      const ExactDirectMorseH0Batch& batch =
          source_event_journal.batches[batch_index];
      if (batch.batch_index != batch_index || batch.order < 1U ||
          batch.order > 10U) {
        return result;
      }

      const std::size_t family_begin = family_cursor;
      const std::size_t arm_seed_begin = arm_seed_cursor;
      std::array<std::size_t, 5U> family_counts_by_support{};
      while (family_cursor <
                 source_arm_seed_journal.families.size() &&
             source_arm_seed_journal.families[family_cursor]
                     .journal_batch_index == batch_index) {
        const ExactDirectSaddleArmFamilyRecord& family =
            source_arm_seed_journal.families[family_cursor];
        if (family.family_index != family_cursor ||
            family.order != batch.order ||
            family.critical_squared_level != batch.squared_level ||
            family.arm_seed_count < 2U ||
            family.arm_seed_count > 4U ||
            family.arm_seed_count > batch.order + 1U ||
            family.arm_seed_offset != arm_seed_cursor) {
          return result;
        }
        const auto next_family_class = checked_add(
            family_counts_by_support[family.arm_seed_count], 1U);
        const auto next_arm_seed_cursor = checked_add(
            arm_seed_cursor, family.arm_seed_count);
        if (!next_family_class.has_value() ||
            !next_arm_seed_cursor.has_value()) {
          result.status = StructuralScanStatus::capacity_overflow;
          return result;
        }
        if (*next_arm_seed_cursor >
            source_arm_seed_journal.arm_seeds.size()) {
          return result;
        }
        family_counts_by_support[family.arm_seed_count] =
            *next_family_class;
        arm_seed_cursor = *next_arm_seed_cursor;
        ++family_cursor;
      }

      if (family_cursor - family_begin != batch.saddle_role_count) {
        return result;
      }
      const std::size_t family_end = family_cursor;
      const std::size_t arm_seed_end = arm_seed_cursor;
      for (std::size_t support_cardinality = 2U;
           support_cardinality <= 4U;
           ++support_cardinality) {
        const std::size_t family_count =
            family_counts_by_support[support_cardinality];
        if (family_count == 0U) {
          continue;
        }

        const auto matching_arm_seed_count = checked_multiply(
            family_count, support_cardinality);
        const auto launch_count = matching_arm_seed_count.has_value()
                                      ? ceil_divide(
                                            *matching_arm_seed_count,
                                            maximum_work_item_count_per_launch)
                                      : std::nullopt;
        const auto examinations_per_work_item =
            local_miniball_support_examinations_per_work_item(
                batch.order);
        const auto examination_count =
            matching_arm_seed_count.has_value() &&
                    examinations_per_work_item.has_value()
                ? checked_multiply(
                      *matching_arm_seed_count,
                      *examinations_per_work_item)
                : std::nullopt;
        const auto next_lane_count =
            checked_add(result.lane_count, 1U);
        const auto next_family_count = checked_add(
            result.matching_family_count, family_count);
        const auto next_arm_seed_count =
            matching_arm_seed_count.has_value()
                ? checked_add(
                      result.matching_arm_seed_count,
                      *matching_arm_seed_count)
                : std::nullopt;
        const auto next_launch_count =
            launch_count.has_value()
                ? checked_add(
                      result.launch_count_upper_bound, *launch_count)
                : std::nullopt;
        const auto next_examination_count =
            examination_count.has_value()
                ? checked_add(
                      result
                          .local_miniball_support_examination_count_upper_bound,
                      *examination_count)
                : std::nullopt;
        if (!matching_arm_seed_count.has_value() ||
            !launch_count.has_value() ||
            !examinations_per_work_item.has_value() ||
            !examination_count.has_value() ||
            !next_lane_count.has_value() ||
            !next_family_count.has_value() ||
            !next_arm_seed_count.has_value() ||
            !next_launch_count.has_value() ||
            !next_examination_count.has_value()) {
          result.status = StructuralScanStatus::capacity_overflow;
          return result;
        }

        if (lanes != nullptr) {
          ExactDirectSparseFacetDescentBatchLane lane;
          lane.lane_index = result.lane_count;
          lane.source_chunk_index = chunk_index;
          lane.source_batch_index = batch_index;
          lane.candidate_family_begin_index = family_begin;
          lane.candidate_family_end_index = family_end;
          lane.candidate_arm_seed_begin_index = arm_seed_begin;
          lane.candidate_arm_seed_end_index = arm_seed_end;
          lane.facet_cardinality = batch.order;
          lane.source_support_cardinality = support_cardinality;
          lane.source_interior_cardinality =
              batch.order + 1U - support_cardinality;
          lane.matching_family_count = family_count;
          lane.matching_arm_seed_count = *matching_arm_seed_count;
          lane.initial_seed_work_item_count =
              *matching_arm_seed_count;
          lane.initial_seed_launch_count = *launch_count;
          lane.initial_seed_standalone_step_support_examination_count_upper_bound =
              *examination_count;
          lanes->push_back(lane);
        }

        result.lane_count = *next_lane_count;
        result.matching_family_count = *next_family_count;
        result.matching_arm_seed_count = *next_arm_seed_count;
        result.launch_count_upper_bound = *next_launch_count;
        result.local_miniball_support_examination_count_upper_bound =
            *next_examination_count;
      }
    }
  }

  if (family_cursor != source_arm_seed_journal.families.size() ||
      arm_seed_cursor != source_arm_seed_journal.arm_seeds.size()) {
    return result;
  }
  result.status = StructuralScanStatus::complete;
  return result;
}

[[nodiscard]] bool source_populations_fit_budget(
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanBudget&
        budget) noexcept {
  return source_event_journal.batches.size() <=
             budget.maximum_source_batch_count &&
         source_arm_seed_journal.families.size() <=
             budget.maximum_source_family_count &&
         source_arm_seed_journal.arm_seeds.size() <=
             budget.maximum_source_arm_seed_count;
}

[[nodiscard]] bool observed_storage_fits_budget(
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget,
    const ExactDirectSparseFacetDescentBatchPlanResult&
        observed) noexcept {
  return budget.maximum_initial_seed_work_item_count_per_launch !=
             0U &&
         source_populations_fit_budget(
             source_event_journal, source_arm_seed_journal, budget) &&
         observed.source_industrial_plan.chunks.size() <=
             budget.maximum_source_chunk_count &&
         observed.lanes.size() <= budget.maximum_lane_count &&
         observed.required_lane_count <= budget.maximum_lane_count &&
         observed.required_initial_seed_launch_count <=
             budget.maximum_initial_seed_launch_count &&
         observed
                 .required_initial_seed_standalone_step_support_examination_count <=
             budget
                 .maximum_initial_seed_standalone_step_support_examination_count;
}

[[nodiscard]] bool architecture_payload_shape_is_consistent(
    const ExactDirectSparseFacetDescentBatchPlanResult&
        result) noexcept {
  if (!result.source_industrial_plan.complete_architecture_plan() ||
      result.source_industrial_plan.requested_config !=
          result.requested_industrial_config ||
      result.requested_budget
              .maximum_initial_seed_work_item_count_per_launch ==
          0U ||
      result.source_industrial_plan.chunks.size() >
          result.requested_budget.maximum_source_chunk_count ||
      result.lanes.size() >
          result.requested_budget.maximum_lane_count ||
      result.required_lane_count != result.lanes.size() ||
      result.required_lane_count >
          result.requested_budget.maximum_lane_count ||
      result.required_initial_seed_launch_count >
          result.requested_budget.maximum_initial_seed_launch_count ||
      result
              .required_initial_seed_standalone_step_support_examination_count >
          result.requested_budget
              .maximum_initial_seed_standalone_step_support_examination_count) {
    return false;
  }

  const auto source_batch_count = as_size_t(
      result.source_industrial_plan.source_counters.batch_count);
  const auto source_family_count = as_size_t(
      result.source_industrial_plan.source_counters.saddle_count);
  const auto source_arm_seed_count = as_size_t(
      result.source_industrial_plan.source_counters.arm_count);
  const auto expected_chunk_scan_count = checked_multiply(
      result.source_industrial_plan.chunks.size(), 2U);
  const auto expected_batch_scan_count =
      source_batch_count.has_value()
          ? checked_multiply(*source_batch_count, 2U)
          : std::nullopt;
  const auto expected_family_scan_count =
      source_family_count.has_value()
          ? checked_multiply(*source_family_count, 2U)
          : std::nullopt;
  const auto maximum_lane_count =
      source_batch_count.has_value()
          ? checked_multiply(
                *source_batch_count,
                maximum_structural_lane_count_per_batch)
          : std::nullopt;
  if (!source_batch_count.has_value() ||
      !source_family_count.has_value() ||
      !source_arm_seed_count.has_value() ||
      !expected_chunk_scan_count.has_value() ||
      !expected_batch_scan_count.has_value() ||
      !expected_family_scan_count.has_value() ||
      !maximum_lane_count.has_value() ||
      *source_batch_count >
          result.requested_budget.maximum_source_batch_count ||
      *source_family_count >
          result.requested_budget.maximum_source_family_count ||
      *source_arm_seed_count >
          result.requested_budget.maximum_source_arm_seed_count ||
      result.lanes.size() > *maximum_lane_count ||
      result.counters.source_chunk_scan_count !=
          *expected_chunk_scan_count ||
      result.counters.source_batch_scan_count !=
          *expected_batch_scan_count ||
      result.counters.source_family_scan_count !=
          *expected_family_scan_count ||
      result.counters.source_arm_seed_reference_count !=
          *source_arm_seed_count) {
    return false;
  }

  std::size_t aggregate_family_count = 0U;
  std::size_t aggregate_arm_seed_count = 0U;
  std::size_t aggregate_launch_count = 0U;
  std::size_t aggregate_examination_count = 0U;
  std::size_t expected_family_begin = 0U;
  std::size_t expected_arm_seed_begin = 0U;
  std::size_t lane_cursor = 0U;
  std::optional<std::size_t> previous_source_batch_index;
  while (lane_cursor < result.lanes.size()) {
    const ExactDirectSparseFacetDescentBatchLane& first =
        result.lanes[lane_cursor];
    if ((previous_source_batch_index.has_value() &&
         first.source_batch_index <=
             *previous_source_batch_index) ||
        first.candidate_family_begin_index !=
            expected_family_begin ||
        first.candidate_arm_seed_begin_index !=
            expected_arm_seed_begin ||
        first.candidate_family_end_index <=
            first.candidate_family_begin_index ||
        first.candidate_arm_seed_end_index <=
            first.candidate_arm_seed_begin_index ||
        first.source_chunk_index >=
            result.source_industrial_plan.chunks.size()) {
      return false;
    }
    const ExactDirectMorseIndustrialChunk& source_chunk =
        result.source_industrial_plan
            .chunks[first.source_chunk_index];
    if (first.source_batch_index <
            source_chunk.source_batch_begin_index ||
        first.source_batch_index >=
            source_chunk.source_batch_end_index) {
      return false;
    }

    const std::size_t batch_family_begin =
        first.candidate_family_begin_index;
    const std::size_t batch_family_end =
        first.candidate_family_end_index;
    const std::size_t batch_arm_seed_begin =
        first.candidate_arm_seed_begin_index;
    const std::size_t batch_arm_seed_end =
        first.candidate_arm_seed_end_index;
    const std::size_t source_batch_index =
        first.source_batch_index;
    const std::size_t source_chunk_index =
        first.source_chunk_index;
    const std::size_t facet_cardinality =
        first.facet_cardinality;
    std::size_t previous_support_cardinality = 1U;
    std::size_t batch_family_count = 0U;
    std::size_t batch_arm_seed_count = 0U;

    while (lane_cursor < result.lanes.size() &&
           result.lanes[lane_cursor].source_batch_index ==
               source_batch_index) {
      const ExactDirectSparseFacetDescentBatchLane& lane =
          result.lanes[lane_cursor];
      if (lane.lane_index != lane_cursor ||
          lane.source_chunk_index != source_chunk_index ||
          lane.candidate_family_begin_index !=
              batch_family_begin ||
          lane.candidate_family_end_index != batch_family_end ||
          lane.candidate_arm_seed_begin_index !=
              batch_arm_seed_begin ||
          lane.candidate_arm_seed_end_index != batch_arm_seed_end ||
          lane.facet_cardinality != facet_cardinality ||
          lane.facet_cardinality < 1U ||
          lane.facet_cardinality > 10U ||
          lane.source_support_cardinality < 2U ||
          lane.source_support_cardinality > 4U ||
          lane.source_support_cardinality <=
              previous_support_cardinality ||
          lane.source_support_cardinality >
              lane.facet_cardinality + 1U ||
          lane.source_interior_cardinality +
                  lane.source_support_cardinality !=
              lane.facet_cardinality + 1U ||
          lane.matching_family_count == 0U) {
        return false;
      }

      const auto expected_arm_seed_count = checked_multiply(
          lane.matching_family_count,
          lane.source_support_cardinality);
      const auto expected_launch_count =
          expected_arm_seed_count.has_value()
              ? ceil_divide(
                    *expected_arm_seed_count,
                    result.requested_budget
                        .maximum_initial_seed_work_item_count_per_launch)
              : std::nullopt;
      const auto examinations_per_work_item =
          local_miniball_support_examinations_per_work_item(
              lane.facet_cardinality);
      const auto expected_examination_count =
          expected_arm_seed_count.has_value() &&
                  examinations_per_work_item.has_value()
              ? checked_multiply(
                    *expected_arm_seed_count,
                    *examinations_per_work_item)
              : std::nullopt;
      const auto next_batch_family_count = checked_add(
          batch_family_count, lane.matching_family_count);
      const auto next_batch_arm_seed_count =
          expected_arm_seed_count.has_value()
              ? checked_add(
                    batch_arm_seed_count,
                    *expected_arm_seed_count)
              : std::nullopt;
      const auto next_aggregate_family_count = checked_add(
          aggregate_family_count, lane.matching_family_count);
      const auto next_aggregate_arm_seed_count =
          expected_arm_seed_count.has_value()
              ? checked_add(
                    aggregate_arm_seed_count,
                    *expected_arm_seed_count)
              : std::nullopt;
      const auto next_aggregate_launch_count =
          expected_launch_count.has_value()
              ? checked_add(
                    aggregate_launch_count,
                    *expected_launch_count)
              : std::nullopt;
      const auto next_aggregate_examination_count =
          expected_examination_count.has_value()
              ? checked_add(
                    aggregate_examination_count,
                    *expected_examination_count)
              : std::nullopt;
      if (!expected_arm_seed_count.has_value() ||
          !expected_launch_count.has_value() ||
          !examinations_per_work_item.has_value() ||
          !expected_examination_count.has_value() ||
          !next_batch_family_count.has_value() ||
          !next_batch_arm_seed_count.has_value() ||
          !next_aggregate_family_count.has_value() ||
          !next_aggregate_arm_seed_count.has_value() ||
          !next_aggregate_launch_count.has_value() ||
          !next_aggregate_examination_count.has_value() ||
          lane.matching_arm_seed_count !=
              *expected_arm_seed_count ||
          lane.initial_seed_work_item_count !=
              *expected_arm_seed_count ||
          lane.initial_seed_launch_count !=
              *expected_launch_count ||
          lane
                  .initial_seed_standalone_step_support_examination_count_upper_bound !=
              *expected_examination_count) {
        return false;
      }

      batch_family_count = *next_batch_family_count;
      batch_arm_seed_count = *next_batch_arm_seed_count;
      aggregate_family_count = *next_aggregate_family_count;
      aggregate_arm_seed_count = *next_aggregate_arm_seed_count;
      aggregate_launch_count = *next_aggregate_launch_count;
      aggregate_examination_count =
          *next_aggregate_examination_count;
      previous_support_cardinality =
          lane.source_support_cardinality;
      ++lane_cursor;
    }

    if (batch_family_count !=
            batch_family_end - batch_family_begin ||
        batch_arm_seed_count !=
            batch_arm_seed_end - batch_arm_seed_begin) {
      return false;
    }
    expected_family_begin = batch_family_end;
    expected_arm_seed_begin = batch_arm_seed_end;
    previous_source_batch_index = source_batch_index;
  }

  return expected_family_begin == *source_family_count &&
         expected_arm_seed_begin == *source_arm_seed_count &&
         aggregate_family_count == *source_family_count &&
         aggregate_arm_seed_count == *source_arm_seed_count &&
         aggregate_launch_count ==
             result.required_initial_seed_launch_count &&
         aggregate_examination_count ==
             result
                 .required_initial_seed_standalone_step_support_examination_count &&
         result.counters.lane_count == result.lanes.size() &&
         result.counters.matching_family_count ==
             aggregate_family_count &&
         result.counters.matching_arm_seed_count ==
             aggregate_arm_seed_count &&
         result.counters.initial_seed_launch_count ==
             aggregate_launch_count &&
         result.counters
                 .initial_seed_standalone_step_support_examination_count_upper_bound ==
             aggregate_examination_count;
}

void clear_lanes(
    ExactDirectSparseFacetDescentBatchPlanResult& result) noexcept {
  result.lanes.clear();
  result.counters = {};
  result.structural_class_is_exact = false;
  result.at_most_three_lanes_per_exact_batch = false;
  result.every_source_family_and_arm_seed_selected_exactly_once =
      false;
  result.lane_order_is_canonical = false;
  result.initial_seed_work_item_tiling_is_bounded = false;
  result.stable_single_pass_lane_selection_required = false;
  result.lanes_never_cross_chunk_or_exact_batch = false;
  result.common_frozen_pre_batch_locator_snapshot_required = false;
  result.one_shared_closure_and_memo_required_per_exact_batch = false;
  result.scientific_commit_barrier_preserved = false;
  result.lane_order_is_operational_only = false;
  result.standalone_shape_check_only = false;
  result.fresh_reconstruction_required_before_execution = false;
}

}  // namespace

bool ExactDirectSparseFacetDescentBatchPlanResult::
    complete_architecture_plan() const noexcept {
  return schema_version ==
             direct_sparse_facet_descent_batch_plan_schema_version &&
         source_population_preflight_certified &&
         source_industrial_plan_freshly_built &&
         source_batches_families_and_arms_joined &&
         budget_preflight_completed && budget_preflight_satisfied &&
         structural_class_is_exact &&
         at_most_three_lanes_per_exact_batch &&
         every_source_family_and_arm_seed_selected_exactly_once &&
         lane_order_is_canonical &&
         initial_seed_work_item_tiling_is_bounded &&
         stable_single_pass_lane_selection_required &&
         lanes_never_cross_chunk_or_exact_batch &&
         common_frozen_pre_batch_locator_snapshot_required &&
         one_shared_closure_and_memo_required_per_exact_batch &&
         scientific_commit_barrier_preserved &&
         lane_order_is_operational_only &&
         standalone_shape_check_only &&
         fresh_reconstruction_required_before_execution &&
         !complete_shared_closure_work_bounded &&
         !actual_lbvh_or_rational_difficulty_claimed &&
         !facets_keys_or_durable_arm_permutation_materialized &&
         !hierarchy_reduction_or_descent_executed &&
         !forbidden_global_structure_materialized &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectSparseFacetDescentBatchPlanDecision::
                 complete_architecture_only_descent_batch_plan &&
         scope ==
             ExactDirectSparseFacetDescentBatchPlanScope::
                 exact_batch_local_structural_provenance_lanes_before_one_shared_closure_only &&
         architecture_payload_shape_is_consistent(*this);
}

ExactDirectSparseFacetDescentBatchPlanResult
build_exact_direct_sparse_facet_descent_batch_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget) {
  ExactDirectSparseFacetDescentBatchPlanResult result;
  result.requested_industrial_config = industrial_config;
  result.requested_budget = budget;
  result.scope = ExactDirectSparseFacetDescentBatchPlanScope::
      exact_batch_local_structural_provenance_lanes_before_one_shared_closure_only;
  result.budget_preflight_completed = true;

  if (budget.maximum_initial_seed_work_item_count_per_launch == 0U) {
    result.decision =
        ExactDirectSparseFacetDescentBatchPlanDecision::
            no_plan_invalid_budget;
    return result;
  }
  if (!source_populations_fit_budget(
          source_event_journal, source_arm_seed_journal, budget)) {
    result.decision =
        ExactDirectSparseFacetDescentBatchPlanDecision::
            no_plan_budget_exhausted;
    return result;
  }
  result.source_population_preflight_certified = true;

  try {
    result.source_industrial_plan =
        build_exact_direct_morse_industrial_plan_with_chunk_count_cap(
            cloud,
            source_facade,
            source_event_journal,
            trusted_arm_seed_budget,
            source_arm_seed_journal,
            industrial_config,
            budget.maximum_source_chunk_count);
    if (!result.source_industrial_plan.complete_architecture_plan()) {
      result.decision =
          result.source_industrial_plan.decision ==
                  ExactDirectMorseIndustrialPlanDecision::
                      no_plan_chunk_count_budget_exhausted
              ? ExactDirectSparseFacetDescentBatchPlanDecision::
                    no_plan_budget_exhausted
              : ExactDirectSparseFacetDescentBatchPlanDecision::
                    no_plan_source_industrial_plan_rejected;
      return result;
    }
    result.source_industrial_plan_freshly_built = true;
    if (result.source_industrial_plan.chunks.size() >
        budget.maximum_source_chunk_count) {
      result.decision =
          ExactDirectSparseFacetDescentBatchPlanDecision::
              no_plan_budget_exhausted;
      return result;
    }

    const StructuralScanResult requirements =
        scan_structural_classes(
            source_event_journal,
            source_arm_seed_journal,
            result.source_industrial_plan,
            budget.maximum_initial_seed_work_item_count_per_launch,
            nullptr);
    if (requirements.status ==
        StructuralScanStatus::capacity_overflow) {
      result.decision =
          ExactDirectSparseFacetDescentBatchPlanDecision::
              no_plan_capacity_overflow;
      return result;
    }
    if (requirements.status != StructuralScanStatus::complete) {
      result.decision =
          ExactDirectSparseFacetDescentBatchPlanDecision::
              no_plan_source_join_inconsistent;
      return result;
    }

    result.required_lane_count = requirements.lane_count;
    result.required_initial_seed_launch_count =
        requirements.launch_count_upper_bound;
    result
        .required_initial_seed_standalone_step_support_examination_count =
        requirements
            .local_miniball_support_examination_count_upper_bound;
    if (result.required_lane_count > budget.maximum_lane_count ||
        result.required_initial_seed_launch_count >
            budget.maximum_initial_seed_launch_count ||
        result
                .required_initial_seed_standalone_step_support_examination_count >
            budget
                .maximum_initial_seed_standalone_step_support_examination_count) {
      result.decision =
          ExactDirectSparseFacetDescentBatchPlanDecision::
              no_plan_budget_exhausted;
      return result;
    }
    result.budget_preflight_satisfied = true;

    result.lanes.reserve(result.required_lane_count);
    const StructuralScanResult emitted = scan_structural_classes(
        source_event_journal,
        source_arm_seed_journal,
        result.source_industrial_plan,
        budget.maximum_initial_seed_work_item_count_per_launch,
        &result.lanes);
    if (emitted.status != StructuralScanStatus::complete ||
        emitted != requirements ||
        result.lanes.size() != result.required_lane_count) {
      clear_lanes(result);
      result.decision =
          emitted.status ==
                  StructuralScanStatus::capacity_overflow
              ? ExactDirectSparseFacetDescentBatchPlanDecision::
                    no_plan_capacity_overflow
              : ExactDirectSparseFacetDescentBatchPlanDecision::
                    no_plan_source_join_inconsistent;
      return result;
    }

    const auto chunk_scan_count = checked_multiply(
        result.source_industrial_plan.chunks.size(), 2U);
    const auto batch_scan_count = checked_multiply(
        source_event_journal.batches.size(), 2U);
    const auto family_scan_count = checked_multiply(
        source_arm_seed_journal.families.size(), 2U);
    if (!chunk_scan_count.has_value() ||
        !batch_scan_count.has_value() ||
        !family_scan_count.has_value()) {
      clear_lanes(result);
      result.decision =
          ExactDirectSparseFacetDescentBatchPlanDecision::
              no_plan_capacity_overflow;
      return result;
    }

    result.counters.source_chunk_scan_count = *chunk_scan_count;
    result.counters.source_batch_scan_count = *batch_scan_count;
    result.counters.source_family_scan_count = *family_scan_count;
    result.counters.source_arm_seed_reference_count =
        source_arm_seed_journal.arm_seeds.size();
    result.counters.lane_count = emitted.lane_count;
    result.counters.matching_family_count =
        emitted.matching_family_count;
    result.counters.matching_arm_seed_count =
        emitted.matching_arm_seed_count;
    result.counters.initial_seed_launch_count =
        emitted.launch_count_upper_bound;
    result.counters
        .initial_seed_standalone_step_support_examination_count_upper_bound =
        emitted
            .local_miniball_support_examination_count_upper_bound;

    result.source_batches_families_and_arms_joined = true;
    result.structural_class_is_exact = true;
    result.at_most_three_lanes_per_exact_batch = true;
    result.every_source_family_and_arm_seed_selected_exactly_once =
        true;
    result.lane_order_is_canonical = true;
    result.initial_seed_work_item_tiling_is_bounded = true;
    result.stable_single_pass_lane_selection_required = true;
    result.lanes_never_cross_chunk_or_exact_batch = true;
    result.common_frozen_pre_batch_locator_snapshot_required = true;
    result.one_shared_closure_and_memo_required_per_exact_batch = true;
    result.scientific_commit_barrier_preserved = true;
    result.lane_order_is_operational_only = true;
    result.standalone_shape_check_only = true;
    result.fresh_reconstruction_required_before_execution = true;
    result.decision =
        ExactDirectSparseFacetDescentBatchPlanDecision::
            complete_architecture_only_descent_batch_plan;
    return result;
  } catch (const std::bad_alloc&) {
    clear_lanes(result);
    result.decision =
        ExactDirectSparseFacetDescentBatchPlanDecision::
            no_plan_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    clear_lanes(result);
    result.decision =
        ExactDirectSparseFacetDescentBatchPlanDecision::
            no_plan_capacity_overflow;
    return result;
  }
}

ExactDirectSparseFacetDescentBatchPlanVerification
verify_exact_direct_sparse_facet_descent_batch_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget,
    const ExactDirectSparseFacetDescentBatchPlanResult& observed) {
  ExactDirectSparseFacetDescentBatchPlanVerification verification;
  verification.observed_storage_within_budget =
      observed_storage_fits_budget(
          source_event_journal,
          source_arm_seed_journal,
          budget,
          observed);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }

  const ExactDirectSparseFacetDescentBatchPlanResult expected =
      build_exact_direct_sparse_facet_descent_batch_plan(
          cloud,
          source_facade,
          source_event_journal,
          trusted_arm_seed_budget,
          source_arm_seed_journal,
          industrial_config,
          budget);
  verification.source_industrial_plan_freshly_rebuilt =
      expected.source_industrial_plan_freshly_built &&
      expected.source_industrial_plan.complete_architecture_plan();
  verification.structural_lanes_freshly_rebuilt =
      expected.complete_architecture_plan();
  verification.exact_result_equality_certified =
      verification.structural_lanes_freshly_rebuilt &&
      expected == observed;
  verification
      .no_geometric_difficulty_or_gpu_qualification_claimed =
      !observed.actual_lbvh_or_rational_difficulty_claimed &&
      !observed.gpu_execution_qualified &&
      !observed.sub_second_latency_claimed &&
      !observed.ten_million_point_capacity_claimed &&
      !observed.public_status_claimed;
  verification.no_forbidden_global_structure_materialized =
      !observed.facets_keys_or_durable_arm_permutation_materialized &&
      !observed.hierarchy_reduction_or_descent_executed &&
      !observed.forbidden_global_structure_materialized &&
      !observed.source_industrial_plan
           .facet_coface_cell_gamma_or_delaunay_materialized;
  verification.result_certified =
      verification.observed_storage_within_budget &&
      verification.source_industrial_plan_freshly_rebuilt &&
      verification.structural_lanes_freshly_rebuilt &&
      verification.exact_result_equality_certified &&
      verification
          .no_geometric_difficulty_or_gpu_qualification_claimed &&
      verification.no_forbidden_global_structure_materialized;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
