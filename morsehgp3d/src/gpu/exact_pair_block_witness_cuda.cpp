#include "morsehgp3d/gpu/exact_pair_block_witness_cuda.hpp"

#include "phase15_exact_pair_block_witness_cuda_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

Phase15ExactPairBlockWitnessCudaAdoptedTraversal
Phase15ExactPairBlockWitnessCudaTraversalAccess::consume(
    MortonLbvhDeviceTraversalLease&& lease) {
  if (!lease.ready()) {
    throw std::invalid_argument(
        "the exact pair-block witness batch requires a ready traversal "
        "lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit = lease.audit();
  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(lease));

  Phase15ExactPairBlockWitnessCudaAdoptedTraversal adopted;
  adopted.retained_owner = owner;
  adopted.source_cloud_identity = owner->source_cloud_identity_;
  adopted.device_coordinate_bits = owner->device_coordinate_bits_;
  adopted.device_morton_point_ids = owner->device_morton_point_ids_;
  adopted.device_nodes = owner->device_nodes_;
  adopted.point_count = audit.point_count;
  adopted.certified_node_count = audit.certified_node_count;
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.cuda_device = owner->cuda_device_;
  adopted.host_fake = audit.host_fake_lifecycle_exercised;
  adopted.ready = owner->ready();

  const bool fake_shape = adopted.host_fake &&
      !owner->cuda_resident() && adopted.device_coordinate_bits == nullptr &&
      adopted.device_morton_point_ids == nullptr &&
      adopted.device_nodes == nullptr && adopted.cuda_device == -1;
  const bool cuda_shape = !adopted.host_fake && owner->cuda_resident() &&
      adopted.device_coordinate_bits != nullptr &&
      adopted.device_morton_point_ids != nullptr &&
      adopted.device_nodes != nullptr && adopted.cuda_device >= 0;
  if (!adopted.ready || !adopted.retained_owner ||
      !adopted.source_cloud_identity || (!fake_shape && !cuda_shape)) {
    throw std::logic_error(
        "the exact pair-block witness batch failed to adopt native LBVH "
        "device authority");
  }
  return adopted;
}

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] bool checked_product(
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

[[nodiscard]] bool checked_sum(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] std::uint32_t compact_value(
    std::size_t value,
    std::uint32_t& flags) noexcept {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    flags |= detail::phase15_exact_pair_block_witness_host_invalid;
    return 0U;
  }
  return static_cast<std::uint32_t>(value);
}

[[nodiscard]] detail::Phase15ExactPairBlockWitnessCudaTask compact_task(
    const ExactPairBlockWitnessCudaTask& source) noexcept {
  detail::Phase15ExactPairBlockWitnessCudaTask task;
  task.task_id = source.task_id;
  task.unordered_pair_mass =
      static_cast<std::uint64_t>(source.support_block.unordered_pair_mass);
  if (source.support_block.kind != ExactPairBlockAuthorityKind::cross) {
    task.host_input_flags |=
        detail::phase15_exact_pair_block_witness_host_invalid;
  }
  task.first_node_index = compact_value(
      source.support_block.first.node_index, task.host_input_flags);
  task.first_leaf_begin = compact_value(
      source.support_block.first.leaf_begin, task.host_input_flags);
  task.first_leaf_end = compact_value(
      source.support_block.first.leaf_end, task.host_input_flags);
  task.second_node_index = compact_value(
      source.support_block.second.node_index, task.host_input_flags);
  task.second_leaf_begin = compact_value(
      source.support_block.second.leaf_begin, task.host_input_flags);
  task.second_leaf_end = compact_value(
      source.support_block.second.leaf_end, task.host_input_flags);
  task.witness_node_index = compact_value(
      source.witness.node_index, task.host_input_flags);
  task.witness_leaf_begin = compact_value(
      source.witness.leaf_begin, task.host_input_flags);
  task.witness_leaf_end = compact_value(
      source.witness.leaf_end, task.host_input_flags);
  return task;
}

void validate_negative_contract(
    const ExactPairBlockWitnessCudaTask& task,
    const ExactPairBlockWitnessCudaRecord& record,
    std::size_t maximum_closed_rank) {
  const bool exact_evaluation =
      record.fixed_limb_evaluation_performed;
  switch (record.decision) {
    case ExactPairBlockWitnessCudaDecision::certified_closed:
      if (!exact_evaluation || record.exact_sign != -1 ||
          task.witness.leaf_begin >= task.witness.leaf_end ||
          task.witness.leaf_count() < maximum_closed_rank - 1U) {
        throw std::logic_error(
            "the exact pair-block witness launcher closed a block without "
            "an exact negative fixed-limb decision and sufficient witness "
            "mass");
      }
      return;
    case ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q:
      if (!exact_evaluation || record.exact_sign < 0) {
        throw std::logic_error(
            "the exact pair-block witness launcher mislabeled an exact Q "
            "decision");
      }
      return;
    case ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow:
      if (!exact_evaluation || record.exact_sign != 0) {
        throw std::logic_error(
            "the exact pair-block witness launcher lost its fixed-limb "
            "residual");
      }
      return;
    case ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass:
    case ExactPairBlockWitnessCudaDecision::residual_invalid_authority:
    case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
      if (exact_evaluation || record.exact_sign != 0 ||
          record.proposal_state !=
              ExactPairBlockWitnessCudaProposalState::not_evaluated) {
        throw std::logic_error(
            "the exact pair-block witness launcher evaluated Q before its "
            "authority gate");
      }
      return;
  }
  throw std::logic_error(
      "the exact pair-block witness launcher returned an unknown decision");
}

void accumulate_record(
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

  std::size_t next_mass = 0U;
  switch (record.decision) {
    case ExactPairBlockWitnessCudaDecision::certified_closed:
      ++audit.certified_task_count;
      if (!checked_sum(
              audit.pruned_unordered_pair_mass,
              record.unordered_pair_mass,
              next_mass)) {
        throw std::overflow_error(
            "the exact pair-block witness pruned mass overflows size_t");
      }
      audit.pruned_unordered_pair_mass = next_mass;
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
  if (!checked_sum(
          audit.residual_unordered_pair_mass,
          record.unordered_pair_mass,
          next_mass)) {
    throw std::overflow_error(
        "the exact pair-block witness residual mass overflows size_t");
  }
  audit.residual_unordered_pair_mass = next_mass;
}

void validate_fixed_audit_fields(
    const ExactPairBlockWitnessCudaAudit& audit,
    const detail::Phase15ExactPairBlockWitnessCudaAdoptedTraversal& traversal,
    const ExactPairBlockWitnessCudaConfig& config,
    std::size_t capacity,
    std::size_t task_count) {
  if (audit.schema_version != exact_pair_block_witness_cuda_schema_version ||
      audit.point_count != traversal.point_count ||
      audit.certified_node_count != traversal.certified_node_count ||
      audit.maximum_closed_rank != config.maximum_closed_rank ||
      audit.task_capacity_per_point != config.task_capacity_per_point ||
      audit.task_capacity != capacity ||
      audit.submitted_task_count != task_count ||
      audit.completed_task_count != task_count ||
      audit.compact_task_record_byte_size !=
          sizeof(detail::Phase15ExactPairBlockWitnessCudaTask) ||
      audit.compact_result_record_byte_size !=
          sizeof(detail::Phase15ExactPairBlockWitnessCudaDeviceRecord) ||
      audit.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      !audit.native_lbvh_authority_consumed ||
      !audit.fixed_linear_task_capacity_validated ||
      !audit.local_submitted_mass_conservation_validated ||
      !audit.bounded_final_qualification_readback_performed ||
      !audit.compact_batch_abi_validated ||
      !audit.fp64_used_as_proposal_only ||
      !audit.negative_closure_requires_fixed_limb_exact_decision ||
      audit.per_task_allocation_count != 0U ||
      audit.per_task_synchronization_count != 0U ||
      audit.device_arena_allocation_count > 1U ||
      audit.double_buffered_transactional_frontier_claimed ||
      audit.global_pair_coverage_closed ||
      audit.pair_catalog_complete_claimed || audit.hierarchy_or_tree_claimed ||
      audit.slo_claimed || audit.global_pair_matrix_materialized ||
      audit.ordinary_or_higher_order_delaunay_materialized ||
      audit.public_status_claimed) {
    throw std::logic_error(
        "the exact pair-block witness launcher violated its bounded batch "
        "audit contract");
  }
  if (traversal.host_fake) {
    if (!audit.host_fake_lifecycle_exercised ||
        audit.cuda_execution_performed ||
        audit.native_lbvh_nodes_read_on_device ||
        audit.resident_batch_submission_validated ||
        audit.device_arena_allocation_count != 0U || audit.cuda_device != -1) {
      throw std::logic_error(
          "the exact pair-block witness fake claimed CUDA execution");
    }
  } else if (
      audit.host_fake_lifecycle_exercised ||
      !audit.cuda_execution_performed ||
      !audit.native_lbvh_nodes_read_on_device ||
      !audit.resident_batch_submission_validated ||
      audit.device_arena_allocation_count != 1U ||
      audit.cuda_device != traversal.cuda_device ||
      audit.kernel_launch_count != 1U || audit.synchronization_count != 1U) {
    throw std::logic_error(
        "the exact pair-block witness CUDA batch lost resident execution "
        "authority");
  }
}

}  // namespace

ExactPairBlockWitnessCudaResult qualify_exact_pair_block_witnesses_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::span<const ExactPairBlockWitnessCudaTask> tasks,
    ExactPairBlockWitnessCudaConfig config) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "an exact pair-block witness batch requires a ready traversal lease");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_witness_cuda_maximum_closed_rank) {
    throw std::out_of_range(
        "an exact pair-block witness maximum closed rank must be in [2, 6]");
  }
  if (config.task_capacity_per_point == 0U ||
      config.task_capacity_per_point >
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point) {
    throw std::out_of_range(
        "an exact pair-block witness task factor must be in [1, 64]");
  }
  if (tasks.empty()) {
    throw std::invalid_argument(
        "an exact pair-block witness batch cannot be empty");
  }

  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  ExactPairBlockWitnessCudaResult refused;
  refused.audit.point_count = source_audit.point_count;
  refused.audit.certified_node_count = source_audit.certified_node_count;
  refused.audit.maximum_closed_rank = config.maximum_closed_rank;
  refused.audit.task_capacity_per_point = config.task_capacity_per_point;
  refused.audit.compact_task_record_byte_size =
      sizeof(detail::Phase15ExactPairBlockWitnessCudaTask);
  refused.audit.compact_result_record_byte_size =
      sizeof(detail::Phase15ExactPairBlockWitnessCudaDeviceRecord);
  refused.audit.source_snapshot_epoch = source_audit.source_snapshot_epoch;
  refused.audit.fixed_linear_task_capacity_validated = true;
  refused.audit.global_pair_coverage_closed = false;
  std::size_t task_capacity = 0U;
  if (!checked_product(
          source_audit.point_count,
          config.task_capacity_per_point,
          task_capacity)) {
    return refused;
  }
  refused.audit.task_capacity = task_capacity;
  refused.audit.submitted_task_count = tasks.size();
  if (tasks.size() > task_capacity ||
      source_audit.point_count > std::numeric_limits<std::uint32_t>::max() ||
      source_audit.certified_node_count >
          std::numeric_limits<std::uint32_t>::max()) {
    return refused;
  }

  std::vector<detail::Phase15ExactPairBlockWitnessCudaTask> compact_tasks;
  compact_tasks.reserve(tasks.size());
  std::size_t submitted_mass = 0U;
  for (const ExactPairBlockWitnessCudaTask& task : tasks) {
    std::size_t next_mass = 0U;
    if (!checked_sum(
            submitted_mass,
            task.support_block.unordered_pair_mass,
            next_mass)) {
      throw std::overflow_error(
          "the exact pair-block witness submitted mass overflows size_t");
    }
    submitted_mass = next_mass;
    compact_tasks.push_back(compact_task(task));
  }
  const std::uint64_t task_digest =
      detail::phase15_exact_pair_block_witness_task_digest(compact_tasks);

  const detail::Phase15ExactPairBlockWitnessCudaAdoptedTraversal traversal =
      detail::Phase15ExactPairBlockWitnessCudaTraversalAccess::consume(
          std::move(traversal_lease));
  const detail::Phase15ExactPairBlockWitnessCudaRequest request{
      &traversal,
      compact_tasks,
      config,
      task_capacity,
      task_digest};
  detail::Phase15ExactPairBlockWitnessCudaReceipt receipt =
      detail::phase15_launch_exact_pair_block_witness_cuda(request);
  if (!receipt.source_identity_authenticated ||
      !receipt.every_task_classified_once ||
      receipt.forbidden_intermediate_readback_performed ||
      receipt.records.size() != tasks.size() ||
      receipt.submitted_task_digest != task_digest ||
      receipt.completed_result_digest !=
          detail::phase15_exact_pair_block_witness_result_digest(
              receipt.records)) {
    throw std::logic_error(
        "the exact pair-block witness launcher returned an unauthenticated "
        "batch receipt");
  }
  validate_fixed_audit_fields(
      receipt.audit, traversal, config, task_capacity, tasks.size());

  ExactPairBlockWitnessCudaAudit recomputed;
  recomputed.submitted_unordered_pair_mass = submitted_mass;
  for (std::size_t index = 0U; index < tasks.size(); ++index) {
    const ExactPairBlockWitnessCudaRecord& record = receipt.records[index];
    if (record.task_id != tasks[index].task_id ||
        record.unordered_pair_mass !=
            tasks[index].support_block.unordered_pair_mass) {
      throw std::logic_error(
          "the exact pair-block witness launcher permuted or changed its "
          "compact task batch");
    }
    validate_negative_contract(
        tasks[index], record, config.maximum_closed_rank);
    accumulate_record(record, recomputed);
  }
  std::size_t conserved_mass = 0U;
  if (!checked_sum(
          recomputed.pruned_unordered_pair_mass,
          recomputed.residual_unordered_pair_mass,
          conserved_mass) ||
      conserved_mass != submitted_mass ||
      receipt.audit.submitted_unordered_pair_mass != submitted_mass ||
      receipt.audit.pruned_unordered_pair_mass !=
          recomputed.pruned_unordered_pair_mass ||
      receipt.audit.residual_unordered_pair_mass !=
          recomputed.residual_unordered_pair_mass ||
      receipt.audit.terminal_unordered_pair_mass != 0U ||
      receipt.audit.pending_unordered_pair_mass != 0U ||
      receipt.audit.directed_negative_count !=
          recomputed.directed_negative_count ||
      receipt.audit.directed_nonnegative_count !=
          recomputed.directed_nonnegative_count ||
      receipt.audit.directed_ambiguous_count !=
          recomputed.directed_ambiguous_count ||
      receipt.audit.directed_arithmetic_overflow_count !=
          recomputed.directed_arithmetic_overflow_count ||
      receipt.audit.fixed_limb_exact_decision_count !=
          recomputed.fixed_limb_exact_decision_count ||
      receipt.audit.exact_negative_count != recomputed.exact_negative_count ||
      receipt.audit.exact_zero_count != recomputed.exact_zero_count ||
      receipt.audit.exact_positive_count != recomputed.exact_positive_count ||
      receipt.audit.fixed_limb_residual_count !=
          recomputed.fixed_limb_residual_count ||
      receipt.audit.certified_task_count != recomputed.certified_task_count ||
      receipt.audit.nonnegative_task_count !=
          recomputed.nonnegative_task_count ||
      receipt.audit.insufficient_witness_task_count !=
          recomputed.insufficient_witness_task_count ||
      receipt.audit.invalid_authority_task_count !=
          recomputed.invalid_authority_task_count ||
      receipt.audit.support_overlap_task_count !=
          recomputed.support_overlap_task_count ||
      receipt.audit.submitted_task_digest != task_digest ||
      receipt.audit.completed_result_digest !=
          receipt.completed_result_digest) {
    throw std::logic_error(
        "the exact pair-block witness launcher lost task counts or local "
        "submitted mass");
  }

  ExactPairBlockWitnessCudaResult result;
  result.status = traversal.host_fake
      ? ExactPairBlockWitnessCudaStatus::non_authoritative_host_fake
      : ExactPairBlockWitnessCudaStatus::qualified_component_batch;
  result.audit = receipt.audit;
  result.records = std::move(receipt.records);
  return result;
}

}  // namespace morsehgp3d::gpu
