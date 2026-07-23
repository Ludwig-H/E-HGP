#include "morsehgp3d/hierarchy/direct_morse_industrial_plan.hpp"

#include <limits>
#include <new>
#include <optional>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

enum class ScanStatus : std::uint8_t {
  complete,
  overflow,
  inconsistent,
};

struct ScanResult {
  ScanStatus status{ScanStatus::inconsistent};
  ExactDirectMorseIndustrialCounters counters{};
};

[[nodiscard]] std::optional<std::uint64_t> checked_add(
    std::uint64_t left,
    std::uint64_t right) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::uint64_t> checked_multiply(
    std::uint64_t left,
    std::uint64_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::uint64_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] constexpr std::uint64_t as_u64(std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::optional<ExactDirectMorseIndustrialCounters>
add_counters(
    const ExactDirectMorseIndustrialCounters& left,
    const ExactDirectMorseIndustrialCounters& right) {
  ExactDirectMorseIndustrialCounters result;
  const auto batch = checked_add(left.batch_count, right.batch_count);
  const auto birth = checked_add(left.birth_count, right.birth_count);
  const auto saddle = checked_add(left.saddle_count, right.saddle_count);
  const auto arm = checked_add(left.arm_count, right.arm_count);
  const auto key = checked_add(
      left.key_point_reference_count,
      right.key_point_reference_count);
  const auto node = checked_add(
      left.node_count_upper_bound, right.node_count_upper_bound);
  const auto child = checked_add(
      left.child_reference_count_upper_bound,
      right.child_reference_count_upper_bound);
  const auto descent = checked_add(
      left.descent_node_reserve_count,
      right.descent_node_reserve_count);
  if (!batch.has_value() || !birth.has_value() ||
      !saddle.has_value() || !arm.has_value() || !key.has_value() ||
      !node.has_value() || !child.has_value() ||
      !descent.has_value()) {
    return std::nullopt;
  }
  result.batch_count = *batch;
  result.birth_count = *birth;
  result.saddle_count = *saddle;
  result.arm_count = *arm;
  result.key_point_reference_count = *key;
  result.node_count_upper_bound = *node;
  result.child_reference_count_upper_bound = *child;
  result.descent_node_reserve_count = *descent;
  return result;
}

[[nodiscard]] ScanResult scan_one_batch(
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& arm_journal,
    const ExactDirectMorseIndustrialMemoryModel& memory_model,
    std::size_t batch_index,
    std::size_t& family_cursor) {
  ScanResult result;
  if (batch_index >= event_journal.batches.size()) {
    return result;
  }
  const ExactDirectMorseH0Batch& batch =
      event_journal.batches[batch_index];
  if (batch.batch_index != batch_index || batch.order < 1U ||
      batch.order > 10U) {
    return result;
  }

  if (family_cursor < arm_journal.families.size() &&
      arm_journal.families[family_cursor].journal_batch_index <
          batch_index) {
    return result;
  }

  std::uint64_t family_count = 0U;
  std::uint64_t arm_count = 0U;
  while (family_cursor < arm_journal.families.size() &&
         arm_journal.families[family_cursor].journal_batch_index ==
             batch_index) {
    const ExactDirectSaddleArmFamilyRecord& family =
        arm_journal.families[family_cursor];
    if (family.family_index != family_cursor ||
        family.order != batch.order ||
        family.critical_squared_level != batch.squared_level ||
        family.arm_seed_count < 2U || family.arm_seed_count > 4U) {
      return result;
    }
    const auto next_family = checked_add(family_count, 1U);
    const auto next_arm =
        checked_add(arm_count, as_u64(family.arm_seed_count));
    if (!next_family.has_value() || !next_arm.has_value()) {
      result.status = ScanStatus::overflow;
      return result;
    }
    family_count = *next_family;
    arm_count = *next_arm;
    ++family_cursor;
  }

  const std::uint64_t births = as_u64(batch.birth_role_count);
  const std::uint64_t saddles = as_u64(batch.saddle_role_count);
  const auto maximum_arms = checked_multiply(4U, saddles);
  if (family_count != saddles || !maximum_arms.has_value() ||
      arm_count > *maximum_arms) {
    return result;
  }
  const auto keyed_records = checked_add(births, arm_count);
  const auto key_references = keyed_records.has_value()
                                  ? checked_multiply(
                                        as_u64(batch.order),
                                        *keyed_records)
                                  : std::nullopt;
  const auto node_bound = checked_add(births, saddles);
  const auto descent_reserve = checked_multiply(
      arm_count, memory_model.descent_node_reserve_per_arm);
  if (!key_references.has_value() || !node_bound.has_value() ||
      !descent_reserve.has_value()) {
    result.status = ScanStatus::overflow;
    return result;
  }

  result.counters.batch_count = 1U;
  result.counters.birth_count = births;
  result.counters.saddle_count = saddles;
  result.counters.arm_count = arm_count;
  result.counters.key_point_reference_count = *key_references;
  result.counters.node_count_upper_bound = *node_bound;
  result.counters.child_reference_count_upper_bound = arm_count;
  result.counters.descent_node_reserve_count = *descent_reserve;
  result.status = ScanStatus::complete;
  return result;
}

[[nodiscard]] ScanResult scan_all_batches(
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& arm_journal,
    const ExactDirectMorseIndustrialMemoryModel& memory_model) {
  ScanResult result;
  result.status = ScanStatus::complete;
  std::size_t family_cursor = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < event_journal.batches.size();
       ++batch_index) {
    const ScanResult batch = scan_one_batch(
        event_journal,
        arm_journal,
        memory_model,
        batch_index,
        family_cursor);
    if (batch.status != ScanStatus::complete) {
      return batch;
    }
    const auto next = add_counters(result.counters, batch.counters);
    if (!next.has_value()) {
      result.status = ScanStatus::overflow;
      return result;
    }
    result.counters = *next;
  }
  if (family_cursor != arm_journal.families.size() ||
      result.counters.saddle_count !=
          as_u64(arm_journal.families.size()) ||
      result.counters.arm_count !=
          as_u64(arm_journal.arm_seeds.size())) {
    result.status = ScanStatus::inconsistent;
  }
  return result;
}

[[nodiscard]] std::optional<std::uint64_t> estimated_chunk_bytes(
    const ExactDirectMorseIndustrialCounters& counters,
    const ExactDirectMorseIndustrialMemoryModel& model,
    bool streaming) {
  std::uint64_t bytes = model.fixed_chunk_bytes;
  const auto add_term =
      [&bytes](std::uint64_t count,
               std::uint64_t bytes_per_record) -> bool {
    const auto term = checked_multiply(count, bytes_per_record);
    if (!term.has_value()) {
      return false;
    }
    const auto next = checked_add(bytes, *term);
    if (!next.has_value()) {
      return false;
    }
    bytes = *next;
    return true;
  };

  if (streaming &&
      (!add_term(1U, model.checkpoint_boundary_bytes) ||
       !add_term(1U, model.external_run_boundary_bytes))) {
    return std::nullopt;
  }
  if (!add_term(counters.batch_count, model.bytes_per_batch) ||
      !add_term(counters.birth_count, model.bytes_per_birth) ||
      !add_term(counters.saddle_count, model.bytes_per_saddle) ||
      !add_term(counters.arm_count, model.bytes_per_arm) ||
      !add_term(
          counters.key_point_reference_count,
          model.bytes_per_key_point_reference) ||
      !add_term(
          counters.node_count_upper_bound,
          model.bytes_per_node_upper_bound) ||
      !add_term(
          counters.child_reference_count_upper_bound,
          model.bytes_per_child_reference_upper_bound) ||
      !add_term(
          counters.descent_node_reserve_count,
          model.bytes_per_descent_node_reserve)) {
    return std::nullopt;
  }
  return bytes;
}

[[nodiscard]] bool fits_chunk_budget(
    const ExactDirectMorseIndustrialCounters& counters,
    std::uint64_t bytes,
    const ExactDirectMorseIndustrialChunkBudget& budget) {
  return bytes <= budget.maximum_bytes &&
         counters.batch_count <= budget.maximum_batch_count &&
         counters.birth_count <= budget.maximum_birth_count &&
         counters.saddle_count <= budget.maximum_saddle_count &&
         counters.arm_count <= budget.maximum_arm_count &&
         counters.descent_node_reserve_count <=
             budget.maximum_descent_node_count;
}

void clear_chunks(ExactDirectMorseIndustrialPlanResult& result) {
  result.chunks.clear();
  result.total_estimated_byte_count = 0U;
  result.exact_order_level_batches_never_split = false;
  result.chunks_cover_consecutive_batches = false;
  result.resident_plan_has_exactly_one_chunk = false;
  result.streaming_boundaries_after_every_chunk = false;
}

[[nodiscard]] bool append_chunk(
    ExactDirectMorseIndustrialPlanResult& result,
    std::size_t begin,
    std::size_t end,
    const ExactDirectMorseIndustrialCounters& counters,
    std::uint64_t bytes,
    bool streaming) {
  const auto next_total =
      checked_add(result.total_estimated_byte_count, bytes);
  if (!next_total.has_value()) {
    return false;
  }
  ExactDirectMorseIndustrialChunk chunk;
  chunk.chunk_index = result.chunks.size();
  chunk.source_batch_begin_index = begin;
  chunk.source_batch_end_index = end;
  chunk.counters = counters;
  chunk.estimated_byte_count = bytes;
  chunk.checkpoint_boundary_after_chunk = streaming;
  chunk.external_run_boundary_after_chunk = streaming;
  result.chunks.push_back(chunk);
  result.total_estimated_byte_count = *next_total;
  return true;
}

[[nodiscard]] bool counters_have_valid_shape(
    const ExactDirectMorseIndustrialCounters& counters,
    const ExactDirectMorseIndustrialMemoryModel& model) noexcept {
  const auto role_count =
      checked_add(counters.birth_count, counters.saddle_count);
  const auto minimum_arm_count =
      checked_multiply(2U, counters.saddle_count);
  const auto maximum_arm_count =
      checked_multiply(4U, counters.saddle_count);
  const auto keyed_record_count =
      checked_add(counters.birth_count, counters.arm_count);
  const auto maximum_key_reference_count =
      keyed_record_count.has_value()
          ? checked_multiply(10U, *keyed_record_count)
          : std::nullopt;
  const auto node_count =
      checked_add(counters.birth_count, counters.saddle_count);
  const auto descent_node_count = checked_multiply(
      counters.arm_count, model.descent_node_reserve_per_arm);
  return role_count.has_value() && minimum_arm_count.has_value() &&
         maximum_arm_count.has_value() &&
         keyed_record_count.has_value() &&
         maximum_key_reference_count.has_value() &&
         node_count.has_value() && descent_node_count.has_value() &&
         ((counters.batch_count == 0U) == (*role_count == 0U)) &&
         counters.batch_count <= *role_count &&
         counters.arm_count >= *minimum_arm_count &&
         counters.arm_count <= *maximum_arm_count &&
         counters.key_point_reference_count >= *keyed_record_count &&
         counters.key_point_reference_count <=
             *maximum_key_reference_count &&
         counters.node_count_upper_bound == *node_count &&
         counters.child_reference_count_upper_bound ==
             counters.arm_count &&
         counters.descent_node_reserve_count ==
             *descent_node_count;
}

[[nodiscard]] bool architecture_payload_shape_is_consistent(
    const ExactDirectMorseIndustrialPlanResult& result) noexcept {
  const bool selected_resident =
      result.selected_policy ==
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k;
  const bool selected_streaming =
      result.selected_policy ==
      ExactDirectMorseIndustrialPolicy::massive_external_streaming;
  if ((!selected_resident && !selected_streaming) ||
      result.chunks.empty() ||
      !counters_have_valid_shape(
          result.source_counters,
          result.requested_config.memory_model) ||
      result.source_counters.batch_count >
          std::numeric_limits<std::size_t>::max()) {
    return false;
  }

  const auto resident_bytes = estimated_chunk_bytes(
      result.source_counters,
      result.requested_config.memory_model,
      false);
  if (!resident_bytes.has_value()) {
    return false;
  }
  const bool resident_fits =
      result.point_count <=
          direct_morse_interactive_resident_maximum_point_count &&
      fits_chunk_budget(
          result.source_counters,
          *resident_bytes,
          result.requested_config.chunk_budget);
  switch (result.requested_config.policy) {
    case ExactDirectMorseIndustrialPolicy::automatic:
      if (selected_resident != resident_fits) {
        return false;
      }
      break;
    case ExactDirectMorseIndustrialPolicy::interactive_resident_50k:
      if (!selected_resident || !resident_fits) {
        return false;
      }
      break;
    case ExactDirectMorseIndustrialPolicy::massive_external_streaming:
      if (!selected_streaming) {
        return false;
      }
      break;
    case ExactDirectMorseIndustrialPolicy::unspecified:
    default:
      return false;
  }

  if (result.resident_plan_has_exactly_one_chunk !=
          selected_resident ||
      result.streaming_boundaries_after_every_chunk !=
          selected_streaming ||
      (selected_resident && result.chunks.size() != 1U)) {
    return false;
  }

  ExactDirectMorseIndustrialCounters aggregate{};
  std::uint64_t total_bytes = 0U;
  std::size_t expected_batch_begin = 0U;
  for (std::size_t chunk_index = 0U;
       chunk_index < result.chunks.size();
       ++chunk_index) {
    const ExactDirectMorseIndustrialChunk& chunk =
        result.chunks[chunk_index];
    if (chunk.chunk_index != chunk_index ||
        chunk.source_batch_begin_index != expected_batch_begin ||
        chunk.source_batch_end_index <
            chunk.source_batch_begin_index ||
        chunk.counters.batch_count !=
            as_u64(
                chunk.source_batch_end_index -
                chunk.source_batch_begin_index) ||
        !counters_have_valid_shape(
            chunk.counters, result.requested_config.memory_model) ||
        chunk.checkpoint_boundary_after_chunk != selected_streaming ||
        chunk.external_run_boundary_after_chunk != selected_streaming) {
      return false;
    }
    if (result.source_counters.batch_count != 0U &&
        chunk.counters.batch_count == 0U) {
      return false;
    }

    const auto bytes = estimated_chunk_bytes(
        chunk.counters,
        result.requested_config.memory_model,
        selected_streaming);
    const auto next_aggregate = add_counters(aggregate, chunk.counters);
    const auto next_total =
        checked_add(total_bytes, chunk.estimated_byte_count);
    if (!bytes.has_value() ||
        chunk.estimated_byte_count != *bytes ||
        !fits_chunk_budget(
            chunk.counters,
            chunk.estimated_byte_count,
            result.requested_config.chunk_budget) ||
        !next_aggregate.has_value() || !next_total.has_value()) {
      return false;
    }
    aggregate = *next_aggregate;
    total_bytes = *next_total;
    expected_batch_begin = chunk.source_batch_end_index;
  }

  return expected_batch_begin ==
             static_cast<std::size_t>(
                 result.source_counters.batch_count) &&
         aggregate == result.source_counters &&
         total_bytes == result.total_estimated_byte_count;
}

}  // namespace

bool ExactDirectMorseIndustrialPlanResult::complete_architecture_plan()
    const noexcept {
  const bool selected_resident =
      selected_policy == ExactDirectMorseIndustrialPolicy::
                             interactive_resident_50k;
  const bool selected_streaming =
      selected_policy == ExactDirectMorseIndustrialPolicy::
                             massive_external_streaming;
  return schema_version ==
             direct_morse_industrial_plan_schema_version &&
         (selected_resident || selected_streaming) &&
         source_event_journal_freshly_replayed &&
         source_arm_seed_journal_freshly_replayed &&
         source_batches_and_arm_families_joined &&
         birth_saddle_and_arm_counts_exact &&
         at_most_four_arms_per_saddle &&
         key_reference_formula_exact &&
         node_and_child_bounds_certified &&
         caller_descent_node_reserve_applied &&
         overflow_checked_before_publication &&
         exact_order_level_batches_never_split &&
         chunks_cover_consecutive_batches &&
         (!selected_resident ||
          (resident_plan_has_exactly_one_chunk &&
           chunks.size() == 1U)) &&
         (!selected_streaming ||
          streaming_boundaries_after_every_chunk) &&
         atomic_rejection_publishes_no_chunks && architecture_only &&
         !hierarchy_forest_or_descent_materialized &&
         !facet_coface_cell_gamma_or_delaunay_materialized &&
         !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         !rejected_source_batch_index.has_value() &&
         decision == ExactDirectMorseIndustrialPlanDecision::
                         complete_architecture_only_plan &&
         scope == ExactDirectMorseIndustrialPlanScope::
                      certified_10_1_10_2_batch_resource_projection_only &&
         architecture_payload_shape_is_consistent(*this);
}

ExactDirectMorseIndustrialPlanResult
build_exact_direct_morse_industrial_plan_with_chunk_count_cap(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& config,
    std::size_t maximum_output_chunk_count) {
  ExactDirectMorseIndustrialPlanResult result;
  result.requested_config = config;
  result.point_count = cloud.size();
  result.scope = ExactDirectMorseIndustrialPlanScope::
      certified_10_1_10_2_batch_resource_projection_only;
  result.atomic_rejection_publishes_no_chunks = true;

  if (maximum_output_chunk_count == 0U) {
    result.decision = ExactDirectMorseIndustrialPlanDecision::
        no_plan_chunk_count_budget_exhausted;
    return result;
  }

  switch (config.policy) {
    case ExactDirectMorseIndustrialPolicy::automatic:
    case ExactDirectMorseIndustrialPolicy::interactive_resident_50k:
    case ExactDirectMorseIndustrialPolicy::massive_external_streaming:
      break;
    case ExactDirectMorseIndustrialPolicy::unspecified:
    default:
      result.decision =
          ExactDirectMorseIndustrialPlanDecision::no_plan_invalid_policy;
      return result;
  }

  try {
    const ExactDirectMorseEventJournalStreamingVerification
        event_verification =
            verify_exact_direct_morse_event_journal_streaming(
                cloud, source_facade, source_event_journal);
    if (!event_verification.result_certified ||
        !source_event_journal.certified_partial_refinement()) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_source_not_certified;
      return result;
    }
    result.source_event_journal_freshly_replayed = true;

    const ExactDirectSaddleArmSeedStreamingVerification arm_verification =
        verify_exact_direct_saddle_arm_seed_journal_streaming(
            cloud,
            source_facade,
            source_event_journal,
            trusted_arm_seed_budget,
            source_arm_seed_journal);
    if (!arm_verification.result_certified ||
        !source_arm_seed_journal.certified_partial_refinement()) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_source_not_certified;
      return result;
    }
    result.source_arm_seed_journal_freshly_replayed = true;

    const ScanResult source_scan = scan_all_batches(
        source_event_journal,
        source_arm_seed_journal,
        config.memory_model);
    if (source_scan.status == ScanStatus::overflow) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_capacity_overflow;
      return result;
    }
    if (source_scan.status != ScanStatus::complete) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_source_join_inconsistent;
      return result;
    }
    result.source_counters = source_scan.counters;
    result.source_batches_and_arm_families_joined = true;
    result.birth_saddle_and_arm_counts_exact = true;
    result.at_most_four_arms_per_saddle = true;
    result.key_reference_formula_exact = true;
    result.node_and_child_bounds_certified = true;
    result.caller_descent_node_reserve_applied = true;

    const auto resident_bytes = estimated_chunk_bytes(
        result.source_counters, config.memory_model, false);
    if (!resident_bytes.has_value()) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_capacity_overflow;
      return result;
    }
    const bool resident_fits =
        cloud.size() <=
            direct_morse_interactive_resident_maximum_point_count &&
        fits_chunk_budget(
            result.source_counters,
            *resident_bytes,
            config.chunk_budget);

    if (config.policy ==
            ExactDirectMorseIndustrialPolicy::
                interactive_resident_50k &&
        !resident_fits) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_resident_requirements_not_met;
      return result;
    }
    if (config.policy ==
            ExactDirectMorseIndustrialPolicy::
                interactive_resident_50k ||
        (config.policy ==
             ExactDirectMorseIndustrialPolicy::automatic &&
         resident_fits)) {
      result.selected_policy = ExactDirectMorseIndustrialPolicy::
          interactive_resident_50k;
      if (result.chunks.size() >= maximum_output_chunk_count) {
        result.decision = ExactDirectMorseIndustrialPlanDecision::
            no_plan_chunk_count_budget_exhausted;
        clear_chunks(result);
        return result;
      }
      if (!append_chunk(
              result,
              0U,
              source_event_journal.batches.size(),
              result.source_counters,
              *resident_bytes,
              false)) {
        result.decision = ExactDirectMorseIndustrialPlanDecision::
            no_plan_capacity_overflow;
        clear_chunks(result);
        return result;
      }
      result.overflow_checked_before_publication = true;
      result.exact_order_level_batches_never_split = true;
      result.chunks_cover_consecutive_batches = true;
      result.resident_plan_has_exactly_one_chunk = true;
      result.streaming_boundaries_after_every_chunk = false;
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          complete_architecture_only_plan;
      return result;
    }

    result.selected_policy = ExactDirectMorseIndustrialPolicy::
        massive_external_streaming;
    std::size_t family_cursor = 0U;
    std::size_t chunk_begin = 0U;
    ExactDirectMorseIndustrialCounters current{};

    for (std::size_t batch_index = 0U;
         batch_index < source_event_journal.batches.size();
         ++batch_index) {
      const ScanResult batch = scan_one_batch(
          source_event_journal,
          source_arm_seed_journal,
          config.memory_model,
          batch_index,
          family_cursor);
      if (batch.status != ScanStatus::complete) {
        result.decision =
            batch.status == ScanStatus::overflow
                ? ExactDirectMorseIndustrialPlanDecision::
                      no_plan_capacity_overflow
                : ExactDirectMorseIndustrialPlanDecision::
                      no_plan_source_join_inconsistent;
        clear_chunks(result);
        return result;
      }
      const auto candidate = add_counters(current, batch.counters);
      const auto candidate_bytes =
          candidate.has_value()
              ? estimated_chunk_bytes(
                    *candidate, config.memory_model, true)
              : std::nullopt;
      if (candidate.has_value() && candidate_bytes.has_value() &&
          fits_chunk_budget(
              *candidate, *candidate_bytes, config.chunk_budget)) {
        current = *candidate;
        continue;
      }

      if (current.batch_count == 0U) {
        result.rejected_source_batch_index = batch_index;
        result.decision = candidate.has_value() &&
                                  candidate_bytes.has_value()
                              ? ExactDirectMorseIndustrialPlanDecision::
                                    no_plan_atomic_batch_exceeds_chunk_budget
                              : ExactDirectMorseIndustrialPlanDecision::
                                    no_plan_capacity_overflow;
        clear_chunks(result);
        return result;
      }

      const auto current_bytes = estimated_chunk_bytes(
          current, config.memory_model, true);
      if (result.chunks.size() >= maximum_output_chunk_count) {
        result.decision = ExactDirectMorseIndustrialPlanDecision::
            no_plan_chunk_count_budget_exhausted;
        clear_chunks(result);
        return result;
      }
      if (!current_bytes.has_value() ||
          !append_chunk(
              result,
              chunk_begin,
              batch_index,
              current,
              *current_bytes,
              true)) {
        result.decision = ExactDirectMorseIndustrialPlanDecision::
            no_plan_capacity_overflow;
        clear_chunks(result);
        return result;
      }

      current = batch.counters;
      chunk_begin = batch_index;
      const auto single_batch_bytes = estimated_chunk_bytes(
          current, config.memory_model, true);
      if (!single_batch_bytes.has_value() ||
          !fits_chunk_budget(
              current, *single_batch_bytes, config.chunk_budget)) {
        result.rejected_source_batch_index = batch_index;
        result.decision =
            single_batch_bytes.has_value()
                ? ExactDirectMorseIndustrialPlanDecision::
                      no_plan_atomic_batch_exceeds_chunk_budget
                : ExactDirectMorseIndustrialPlanDecision::
                      no_plan_capacity_overflow;
        clear_chunks(result);
        return result;
      }
    }

    if (source_event_journal.batches.empty()) {
      const auto empty_bytes = estimated_chunk_bytes(
          current, config.memory_model, true);
      if (!empty_bytes.has_value() ||
          !fits_chunk_budget(
              current, *empty_bytes, config.chunk_budget)) {
        result.decision = empty_bytes.has_value()
                              ? ExactDirectMorseIndustrialPlanDecision::
                                    no_plan_atomic_batch_exceeds_chunk_budget
                              : ExactDirectMorseIndustrialPlanDecision::
                                    no_plan_capacity_overflow;
        clear_chunks(result);
        return result;
      }
    }

    const auto final_bytes =
        estimated_chunk_bytes(current, config.memory_model, true);
    if (result.chunks.size() >= maximum_output_chunk_count) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_chunk_count_budget_exhausted;
      clear_chunks(result);
      return result;
    }
    if (!final_bytes.has_value() ||
        !append_chunk(
            result,
            chunk_begin,
            source_event_journal.batches.size(),
            current,
            *final_bytes,
            true)) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_capacity_overflow;
      clear_chunks(result);
      return result;
    }
    if (family_cursor != source_arm_seed_journal.families.size()) {
      result.decision = ExactDirectMorseIndustrialPlanDecision::
          no_plan_source_join_inconsistent;
      clear_chunks(result);
      return result;
    }

    result.overflow_checked_before_publication = true;
    result.exact_order_level_batches_never_split = true;
    result.chunks_cover_consecutive_batches = true;
    result.resident_plan_has_exactly_one_chunk = false;
    result.streaming_boundaries_after_every_chunk = true;
    result.decision = ExactDirectMorseIndustrialPlanDecision::
        complete_architecture_only_plan;
    return result;
  } catch (const std::bad_alloc&) {
    clear_chunks(result);
    result.decision = ExactDirectMorseIndustrialPlanDecision::
        no_plan_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    clear_chunks(result);
    result.decision = ExactDirectMorseIndustrialPlanDecision::
        no_plan_capacity_overflow;
    return result;
  }
}

ExactDirectMorseIndustrialPlanResult
build_exact_direct_morse_industrial_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult&
        source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& config) {
  return build_exact_direct_morse_industrial_plan_with_chunk_count_cap(
      cloud,
      source_facade,
      source_event_journal,
      trusted_arm_seed_budget,
      source_arm_seed_journal,
      config,
      std::numeric_limits<std::size_t>::max());
}

}  // namespace morsehgp3d::hierarchy
