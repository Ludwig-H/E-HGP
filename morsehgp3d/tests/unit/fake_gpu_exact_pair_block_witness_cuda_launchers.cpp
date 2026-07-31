#include "phase15_exact_pair_block_witness_cuda_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace morsehgp3d::gpu::detail {
namespace {

[[nodiscard]] bool ranges_overlap(
    std::uint32_t first_begin,
    std::uint32_t first_end,
    std::uint32_t second_begin,
    std::uint32_t second_end) noexcept {
  return first_begin < second_end && second_begin < first_end;
}

[[nodiscard]] bool checked_mass(
    std::uint64_t first_count,
    std::uint64_t second_count,
    std::uint64_t expected) noexcept {
  return first_count == 0U ||
      (second_count <= std::numeric_limits<std::uint64_t>::max() /
               first_count &&
       first_count * second_count == expected);
}

[[nodiscard]] ExactPairBlockWitnessCudaProposalState proposal(
    std::uint64_t selector) noexcept {
  switch (selector % 3U) {
    case 0U:
      return ExactPairBlockWitnessCudaProposalState::directed_negative;
    case 1U:
      return ExactPairBlockWitnessCudaProposalState::directed_nonnegative;
    default:
      return ExactPairBlockWitnessCudaProposalState::
          directed_arithmetic_overflow;
  }
}

[[nodiscard]] ExactPairBlockWitnessCudaRecord classify(
    const Phase15ExactPairBlockWitnessCudaTask& task,
    const Phase15ExactPairBlockWitnessCudaRequest& request,
    std::uint64_t synthetic_geometry_selector) {
  ExactPairBlockWitnessCudaRecord record;
  record.task_id = task.task_id;
  record.unordered_pair_mass =
      static_cast<std::size_t>(task.unordered_pair_mass);
  const bool range_shape = task.first_leaf_begin < task.first_leaf_end &&
      task.second_leaf_begin < task.second_leaf_end &&
      task.witness_leaf_begin < task.witness_leaf_end &&
      task.first_leaf_end <= request.traversal->point_count &&
      task.second_leaf_end <= request.traversal->point_count &&
      task.witness_leaf_end <= request.traversal->point_count;
  const std::uint64_t first_count =
      static_cast<std::uint64_t>(task.first_leaf_end) -
      task.first_leaf_begin;
  const std::uint64_t second_count =
      static_cast<std::uint64_t>(task.second_leaf_end) -
      task.second_leaf_begin;
  if (task.host_input_flags != 0U || !range_shape ||
      task.first_node_index >= request.traversal->certified_node_count ||
      task.second_node_index >= request.traversal->certified_node_count ||
      task.witness_node_index >= request.traversal->certified_node_count ||
      !checked_mass(
          first_count, second_count, task.unordered_pair_mass)) {
    record.decision =
        ExactPairBlockWitnessCudaDecision::residual_invalid_authority;
    return record;
  }
  if (task.first_leaf_end > task.second_leaf_begin ||
      ranges_overlap(
          task.first_leaf_begin,
          task.first_leaf_end,
          task.witness_leaf_begin,
          task.witness_leaf_end) ||
      ranges_overlap(
          task.second_leaf_begin,
          task.second_leaf_end,
          task.witness_leaf_begin,
          task.witness_leaf_end)) {
    record.decision =
        ExactPairBlockWitnessCudaDecision::residual_support_overlap;
    return record;
  }
  const std::uint64_t witness_count =
      static_cast<std::uint64_t>(task.witness_leaf_end) -
      task.witness_leaf_begin;
  if (witness_count < request.config.maximum_closed_rank - 1U) {
    record.decision = ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass;
    return record;
  }

  const std::uint64_t selector = synthetic_geometry_selector;
  record.proposal_state = proposal(selector);
  if (record.proposal_state ==
      ExactPairBlockWitnessCudaProposalState::directed_nonnegative) {
    record.decision = ExactPairBlockWitnessCudaDecision::
        inconclusive_nonnegative_q;
    return record;
  }
  record.fixed_limb_evaluation_performed = true;
  switch (selector % 3U) {
    case 0U:
      record.exact_sign = -1;
      record.decision = ExactPairBlockWitnessCudaDecision::certified_closed;
      break;
    case 1U:
      record.exact_sign = 1;
      record.decision = ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q;
      break;
    default:
      record.decision = ExactPairBlockWitnessCudaDecision::
          residual_fixed_limb_overflow;
      break;
  }
  return record;
}

[[nodiscard]] ExactPairBlockWitnessCudaRecord classify(
    const Phase15ExactPairBlockWitnessCudaTask& task,
    const Phase15ExactPairBlockWitnessCudaRequest& request) {
  return classify(task, request, task.task_id);
}

void accumulate(
    const ExactPairBlockWitnessCudaRecord& record,
    ExactPairBlockWitnessCudaAudit& audit) {
  switch (record.proposal_state) {
    case ExactPairBlockWitnessCudaProposalState::not_evaluated:
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_negative:
      ++audit.directed_negative_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_nonnegative:
      ++audit.directed_nonnegative_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_ambiguous:
      ++audit.directed_ambiguous_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::
        directed_arithmetic_overflow:
      ++audit.directed_arithmetic_overflow_count;
      break;
  }
  if (record.fixed_limb_evaluation_performed) {
    if (record.decision ==
        ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow) {
      ++audit.fixed_limb_residual_count;
    } else {
      ++audit.fixed_limb_exact_decision_count;
      if (record.exact_sign < 0) {
        ++audit.exact_negative_count;
      } else if (record.exact_sign == 0) {
        ++audit.exact_zero_count;
      } else {
        ++audit.exact_positive_count;
      }
    }
  }
  if (!record.fixed_limb_evaluation_performed &&
      record.decision ==
          ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q &&
      record.proposal_state ==
          ExactPairBlockWitnessCudaProposalState::directed_nonnegative) {
    ++audit.directed_nonnegative_short_circuit_count;
  }
  switch (record.decision) {
    case ExactPairBlockWitnessCudaDecision::certified_closed:
      ++audit.certified_task_count;
      audit.pruned_unordered_pair_mass += record.unordered_pair_mass;
      return;
    case ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q:
      ++audit.nonnegative_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass:
      ++audit.insufficient_witness_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_invalid_authority:
      ++audit.invalid_authority_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
      ++audit.support_overlap_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow:
      break;
  }
  audit.residual_unordered_pair_mass += record.unordered_pair_mass;
}

[[nodiscard]] std::uint8_t encode_outcome(
    const ExactPairBlockWitnessCudaRecord& record) {
  using Outcome = Phase15ExactPairBlockWitnessCudaOutcome;
  switch (record.decision) {
    case ExactPairBlockWitnessCudaDecision::residual_invalid_authority:
      return static_cast<std::uint8_t>(Outcome::invalid_authority);
    case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
      return static_cast<std::uint8_t>(Outcome::support_overlap);
    case ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass:
      return static_cast<std::uint8_t>(Outcome::insufficient_witness);
    case ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q:
      if (!record.fixed_limb_evaluation_performed &&
          record.proposal_state ==
              ExactPairBlockWitnessCudaProposalState::directed_nonnegative) {
        return static_cast<std::uint8_t>(Outcome::directed_nonnegative);
      }
      break;
    case ExactPairBlockWitnessCudaDecision::certified_closed:
    case ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow:
      break;
  }
  std::uint8_t base = 0U;
  switch (record.proposal_state) {
    case ExactPairBlockWitnessCudaProposalState::directed_negative:
      base = static_cast<std::uint8_t>(
          Outcome::directed_negative_exact_negative);
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_ambiguous:
      base = static_cast<std::uint8_t>(
          Outcome::directed_ambiguous_exact_negative);
      break;
    case ExactPairBlockWitnessCudaProposalState::
        directed_arithmetic_overflow:
      base = static_cast<std::uint8_t>(
          Outcome::directed_overflow_exact_negative);
      break;
    case ExactPairBlockWitnessCudaProposalState::not_evaluated:
    case ExactPairBlockWitnessCudaProposalState::directed_nonnegative:
      throw std::logic_error(
          "the fake repeated witness outcome lost its proposal state");
  }
  if (record.decision ==
      ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow) {
    return static_cast<std::uint8_t>(base + 3U);
  }
  if (record.exact_sign < 0) {
    return base;
  }
  if (record.exact_sign == 0) {
    return static_cast<std::uint8_t>(base + 1U);
  }
  return static_cast<std::uint8_t>(base + 2U);
}

}  // namespace

Phase15ExactPairBlockWitnessCudaReceipt
phase15_launch_exact_pair_block_witness_cuda(
    const Phase15ExactPairBlockWitnessCudaRequest& request) {
  if (request.traversal == nullptr || !request.traversal->ready ||
      !request.traversal->host_fake ||
      !request.traversal->retained_owner ||
      !request.traversal->source_cloud_identity ||
      request.traversal->device_coordinate_bits != nullptr ||
      request.traversal->device_morton_point_ids != nullptr ||
      request.traversal->device_nodes != nullptr ||
      request.traversal->cuda_device != -1 || request.tasks.empty() ||
      request.tasks.size() > request.task_capacity ||
      request.submitted_task_digest !=
          phase15_exact_pair_block_witness_task_digest(request.tasks)) {
    throw std::invalid_argument(
        "the fake exact pair-block witness launcher received an invalid "
        "resident batch request");
  }

  Phase15ExactPairBlockWitnessCudaReceipt receipt;
  receipt.records.reserve(request.tasks.size());
  ExactPairBlockWitnessCudaAudit& audit = receipt.audit;
  audit.point_count = request.traversal->point_count;
  audit.certified_node_count = request.traversal->certified_node_count;
  audit.maximum_closed_rank = request.config.maximum_closed_rank;
  audit.task_capacity_per_point = request.config.task_capacity_per_point;
  audit.task_capacity = request.task_capacity;
  audit.submitted_task_count = request.tasks.size();
  audit.completed_task_count = request.tasks.size();
  audit.compact_task_record_byte_size =
      sizeof(Phase15ExactPairBlockWitnessCudaTask);
  audit.compact_result_record_byte_size =
      sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord);
  audit.host_to_device_task_byte_count =
      request.tasks.size_bytes();
  audit.device_to_host_result_byte_count = request.tasks.size() *
      sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord);
  audit.source_snapshot_epoch = request.traversal->source_snapshot_epoch;
  audit.submitted_task_digest = request.submitted_task_digest;
  audit.cuda_device = -1;
  audit.native_lbvh_authority_consumed = true;
  audit.fixed_linear_task_capacity_validated = true;
  audit.bounded_final_qualification_readback_performed = true;
  audit.compact_batch_abi_validated = true;
  audit.directed_interval_never_closes_or_prunes = true;
  audit.fp64_used_as_proposal_only = true;
  audit.negative_closure_requires_fixed_limb_exact_decision = true;
  audit.host_fake_lifecycle_exercised = true;

  for (const Phase15ExactPairBlockWitnessCudaTask& task : request.tasks) {
    ExactPairBlockWitnessCudaRecord record = classify(task, request);
    audit.submitted_unordered_pair_mass += record.unordered_pair_mass;
    accumulate(record, audit);
    receipt.records.push_back(record);
  }
  audit.proposal_partition_validated =
      audit.directed_negative_count + audit.directed_nonnegative_count +
              audit.directed_ambiguous_count +
              audit.directed_arithmetic_overflow_count +
              audit.invalid_authority_task_count +
              audit.support_overlap_task_count +
              audit.insufficient_witness_task_count ==
          audit.completed_task_count;
  audit.directed_nonnegative_short_circuit_validated =
      audit.directed_nonnegative_short_circuit_count ==
              audit.directed_nonnegative_count &&
      audit.fixed_limb_exact_decision_count +
              audit.fixed_limb_residual_count ==
          audit.directed_negative_count + audit.directed_ambiguous_count +
              audit.directed_arithmetic_overflow_count &&
      audit.fixed_limb_exact_decision_count ==
          audit.exact_negative_count + audit.exact_zero_count +
              audit.exact_positive_count &&
      audit.certified_task_count == audit.exact_negative_count &&
      audit.nonnegative_task_count ==
          audit.directed_nonnegative_short_circuit_count +
              audit.exact_zero_count + audit.exact_positive_count;
  audit.local_submitted_mass_conservation_validated =
      audit.pruned_unordered_pair_mass +
              audit.residual_unordered_pair_mass ==
          audit.submitted_unordered_pair_mass;
  receipt.submitted_task_digest = request.submitted_task_digest;
  receipt.completed_result_digest =
      phase15_exact_pair_block_witness_result_digest(receipt.records);
  audit.completed_result_digest = receipt.completed_result_digest;
  receipt.source_identity_authenticated = true;
  receipt.every_task_classified_once = true;
  return receipt;
}

Phase15ExactPairBlockWitnessCudaRepeatedReceipt
phase15_launch_repeated_exact_pair_block_witness_cuda(
    const Phase15ExactPairBlockWitnessCudaRepeatedRequest& request) {
  if (request.traversal == nullptr || !request.traversal->ready ||
      !request.traversal->host_fake ||
      !request.traversal->retained_owner ||
      !request.traversal->source_cloud_identity ||
      request.traversal->device_coordinate_bits != nullptr ||
      request.traversal->device_morton_point_ids != nullptr ||
      request.traversal->device_nodes != nullptr ||
      request.traversal->cuda_device != -1 || request.pattern.empty() ||
      request.repetition_count == 0U ||
      request.logical_task_count !=
          request.pattern.size() * request.repetition_count ||
      request.logical_task_count > request.task_capacity ||
      request.repeated_task_recipe_digest == 0U) {
    throw std::invalid_argument(
        "the fake repeated exact pair-block witness launcher received an "
        "invalid compact pattern request");
  }
  const Phase15ExactPairBlockWitnessCudaRequest pattern_request{
      request.traversal,
      request.pattern,
      request.config,
      request.task_capacity,
      phase15_exact_pair_block_witness_task_digest(request.pattern)};
  Phase15ExactPairBlockWitnessCudaRepeatedReceipt receipt;
  receipt.outcomes.resize(request.logical_task_count);
  for (std::size_t logical_index = 0U;
       logical_index < request.logical_task_count;
       ++logical_index) {
    Phase15ExactPairBlockWitnessCudaTask task =
        request.pattern[logical_index % request.pattern.size()];
    const std::uint64_t synthetic_geometry_selector = task.task_id;
    task.task_id = request.first_task_id +
        static_cast<std::uint64_t>(logical_index);
    const ExactPairBlockWitnessCudaRecord record = classify(
        task, pattern_request, synthetic_geometry_selector);
    if (record.task_id != task.task_id) {
      throw std::logic_error(
          "the fake repeated witness launcher lost an affine task identity");
    }
    receipt.outcomes[logical_index] =
        encode_outcome(record);
  }
  receipt.host_to_device_pattern_byte_count = request.pattern.size_bytes();
  receipt.device_to_host_outcome_byte_count = receipt.outcomes.size();
  receipt.cuda_device = -1;
  receipt.source_identity_authenticated = true;
  receipt.every_logical_task_classified_once = true;
  receipt.host_fake_lifecycle_exercised = true;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
