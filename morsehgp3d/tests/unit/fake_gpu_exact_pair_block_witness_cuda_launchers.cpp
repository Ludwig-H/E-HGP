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
    std::uint64_t task_id) noexcept {
  switch (task_id % 4U) {
    case 0U:
      return ExactPairBlockWitnessCudaProposalState::directed_negative;
    case 1U:
      return ExactPairBlockWitnessCudaProposalState::directed_nonnegative;
    case 2U:
      return ExactPairBlockWitnessCudaProposalState::directed_ambiguous;
    default:
      return ExactPairBlockWitnessCudaProposalState::
          directed_arithmetic_overflow;
  }
}

[[nodiscard]] ExactPairBlockWitnessCudaRecord classify(
    const Phase15ExactPairBlockWitnessCudaTask& task,
    const Phase15ExactPairBlockWitnessCudaRequest& request) {
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
  if (ranges_overlap(
          task.first_leaf_begin,
          task.first_leaf_end,
          task.second_leaf_begin,
          task.second_leaf_end) ||
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

  record.proposal_state = proposal(task.task_id);
  record.fixed_limb_evaluation_performed = true;
  switch (task.task_id % 3U) {
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
  audit.fp64_used_as_proposal_only = true;
  audit.negative_closure_requires_fixed_limb_exact_decision = true;
  audit.host_fake_lifecycle_exercised = true;

  for (const Phase15ExactPairBlockWitnessCudaTask& task : request.tasks) {
    ExactPairBlockWitnessCudaRecord record = classify(task, request);
    audit.submitted_unordered_pair_mass += record.unordered_pair_mass;
    accumulate(record, audit);
    receipt.records.push_back(record);
  }
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

}  // namespace morsehgp3d::gpu::detail
