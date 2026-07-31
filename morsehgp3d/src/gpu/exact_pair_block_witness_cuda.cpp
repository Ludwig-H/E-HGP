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

[[nodiscard]] ExactPairBlockWitnessCudaRecord decode_repeated_outcome(
    std::uint8_t encoded,
    std::uint64_t task_id,
    std::size_t unordered_pair_mass) {
  using Outcome = detail::Phase15ExactPairBlockWitnessCudaOutcome;
  ExactPairBlockWitnessCudaRecord record;
  record.task_id = task_id;
  record.unordered_pair_mass = unordered_pair_mass;
  const auto outcome = static_cast<Outcome>(encoded);
  switch (outcome) {
    case Outcome::invalid_authority:
      record.decision =
          ExactPairBlockWitnessCudaDecision::residual_invalid_authority;
      return record;
    case Outcome::support_overlap:
      record.decision =
          ExactPairBlockWitnessCudaDecision::residual_support_overlap;
      return record;
    case Outcome::insufficient_witness:
      record.decision = ExactPairBlockWitnessCudaDecision::
          inconclusive_insufficient_witness_mass;
      return record;
    case Outcome::directed_nonnegative:
      record.decision = ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q;
      record.proposal_state =
          ExactPairBlockWitnessCudaProposalState::directed_nonnegative;
      return record;
    case Outcome::directed_negative_exact_negative:
    case Outcome::directed_negative_exact_zero:
    case Outcome::directed_negative_exact_positive:
    case Outcome::directed_negative_fixed_overflow:
      record.proposal_state =
          ExactPairBlockWitnessCudaProposalState::directed_negative;
      break;
    case Outcome::directed_ambiguous_exact_negative:
    case Outcome::directed_ambiguous_exact_zero:
    case Outcome::directed_ambiguous_exact_positive:
    case Outcome::directed_ambiguous_fixed_overflow:
      record.proposal_state =
          ExactPairBlockWitnessCudaProposalState::directed_ambiguous;
      break;
    case Outcome::directed_overflow_exact_negative:
    case Outcome::directed_overflow_exact_zero:
    case Outcome::directed_overflow_exact_positive:
    case Outcome::directed_overflow_fixed_overflow:
      record.proposal_state = ExactPairBlockWitnessCudaProposalState::
          directed_arithmetic_overflow;
      break;
    default:
      throw std::logic_error(
          "the repeated exact pair-block witness kernel returned an invalid "
          "outcome byte");
  }
  record.fixed_limb_evaluation_performed = true;
  switch (outcome) {
    case Outcome::directed_negative_exact_negative:
    case Outcome::directed_ambiguous_exact_negative:
    case Outcome::directed_overflow_exact_negative:
      record.decision = ExactPairBlockWitnessCudaDecision::certified_closed;
      record.exact_sign = -1;
      return record;
    case Outcome::directed_negative_exact_zero:
    case Outcome::directed_ambiguous_exact_zero:
    case Outcome::directed_overflow_exact_zero:
      record.decision = ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q;
      return record;
    case Outcome::directed_negative_exact_positive:
    case Outcome::directed_ambiguous_exact_positive:
    case Outcome::directed_overflow_exact_positive:
      record.decision = ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q;
      record.exact_sign = 1;
      return record;
    case Outcome::directed_negative_fixed_overflow:
    case Outcome::directed_ambiguous_fixed_overflow:
    case Outcome::directed_overflow_fixed_overflow:
      record.decision = ExactPairBlockWitnessCudaDecision::
          residual_fixed_limb_overflow;
      return record;
    default:
      throw std::logic_error(
          "the repeated exact pair-block witness outcome lost its exact "
          "classification");
  }
}

[[nodiscard]] std::uint64_t repeated_task_recipe_digest(
    std::span<const detail::Phase15ExactPairBlockWitnessCudaTask> pattern,
    std::size_t repetition_count,
    std::size_t logical_task_count,
    std::uint64_t first_task_id) noexcept {
  std::uint64_t digest =
      detail::phase15_exact_pair_block_witness_digest_word(
          detail::phase15_exact_pair_block_witness_digest_offset,
          UINT64_C(0x5245504541545031));
  digest = detail::phase15_exact_pair_block_witness_digest_word(
      digest, first_task_id);
  digest = detail::phase15_exact_pair_block_witness_digest_word(
      digest, static_cast<std::uint64_t>(repetition_count));
  digest = detail::phase15_exact_pair_block_witness_digest_word(
      digest, static_cast<std::uint64_t>(logical_task_count));
  digest = detail::phase15_exact_pair_block_witness_digest_word(
      digest,
      detail::phase15_exact_pair_block_witness_task_digest(pattern));
  return digest;
}

[[nodiscard]] std::uint64_t expanded_outcome_digest(
    std::span<const std::uint8_t> outcomes) noexcept {
  std::uint64_t digest =
      detail::phase15_exact_pair_block_witness_digest_word(
          detail::phase15_exact_pair_block_witness_digest_offset,
          UINT64_C(0x5245504f55545031));
  digest = detail::phase15_exact_pair_block_witness_digest_word(
      digest, static_cast<std::uint64_t>(outcomes.size()));
  for (const std::uint8_t outcome : outcomes) {
    digest ^= outcome;
    digest *= detail::phase15_exact_pair_block_witness_digest_prime;
  }
  return digest;
}

[[nodiscard]] std::size_t scaled_count(
    std::size_t pattern_count,
    std::size_t repetition_count,
    const char* message) {
  std::size_t result = 0U;
  if (!checked_product(pattern_count, repetition_count, result)) {
    throw std::overflow_error(message);
  }
  return result;
}

void validate_negative_contract(
    const ExactPairBlockWitnessCudaTask& task,
    const ExactPairBlockWitnessCudaRecord& record,
    std::size_t maximum_closed_rank) {
  const bool exact_evaluation =
      record.fixed_limb_evaluation_performed;
  if (record.proposal_state ==
          ExactPairBlockWitnessCudaProposalState::directed_nonnegative &&
      (record.decision !=
           ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q ||
       exact_evaluation || record.exact_sign != 0)) {
    throw std::logic_error(
        "the exact pair-block witness launcher used a directed "
        "nonnegative interval outside its fail-open shortcut");
  }
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
      if ((exact_evaluation && record.exact_sign != 0 &&
           record.exact_sign != 1) ||
          (!exact_evaluation &&
           (record.exact_sign != 0 ||
            record.proposal_state !=
                ExactPairBlockWitnessCudaProposalState::
                    directed_nonnegative))) {
        throw std::logic_error(
            "the exact pair-block witness launcher mislabeled a fail-open "
            "nonnegative Q decision");
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
  if (!record.fixed_limb_evaluation_performed &&
      record.decision ==
          ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q &&
      record.proposal_state ==
          ExactPairBlockWitnessCudaProposalState::directed_nonnegative) {
    ++audit.directed_nonnegative_short_circuit_count;
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
      !audit.proposal_partition_validated ||
      !audit.directed_nonnegative_short_circuit_validated ||
      !audit.directed_interval_never_closes_or_prunes ||
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
  std::size_t proposed_task_count = 0U;
  std::size_t authority_gate_task_count = 0U;
  std::size_t partitioned_task_count = 0U;
  std::size_t fixed_limb_evaluation_count = 0U;
  std::size_t expected_fixed_limb_evaluation_count = 0U;
  std::size_t exact_sign_count = 0U;
  std::size_t expected_nonnegative_count = 0U;
  std::size_t classified_task_count = 0U;
  const bool proposal_partition =
      checked_sum(
          recomputed.directed_negative_count,
          recomputed.directed_nonnegative_count,
          proposed_task_count) &&
      checked_sum(
          proposed_task_count,
          recomputed.directed_ambiguous_count,
          proposed_task_count) &&
      checked_sum(
          proposed_task_count,
          recomputed.directed_arithmetic_overflow_count,
          proposed_task_count) &&
      checked_sum(
          recomputed.invalid_authority_task_count,
          recomputed.support_overlap_task_count,
          authority_gate_task_count) &&
      checked_sum(
          authority_gate_task_count,
          recomputed.insufficient_witness_task_count,
          authority_gate_task_count) &&
      checked_sum(
          proposed_task_count,
          authority_gate_task_count,
          partitioned_task_count) &&
      partitioned_task_count == tasks.size() &&
      recomputed.directed_nonnegative_short_circuit_count ==
          recomputed.directed_nonnegative_count &&
      recomputed.directed_nonnegative_short_circuit_count <=
          proposed_task_count &&
      checked_sum(
          recomputed.fixed_limb_exact_decision_count,
          recomputed.fixed_limb_residual_count,
          fixed_limb_evaluation_count) &&
      checked_sum(
          recomputed.directed_negative_count,
          recomputed.directed_ambiguous_count,
          expected_fixed_limb_evaluation_count) &&
      checked_sum(
          expected_fixed_limb_evaluation_count,
          recomputed.directed_arithmetic_overflow_count,
          expected_fixed_limb_evaluation_count) &&
      fixed_limb_evaluation_count ==
          expected_fixed_limb_evaluation_count &&
      checked_sum(
          recomputed.exact_negative_count,
          recomputed.exact_zero_count,
          exact_sign_count) &&
      checked_sum(
          exact_sign_count,
          recomputed.exact_positive_count,
          exact_sign_count) &&
      recomputed.fixed_limb_exact_decision_count == exact_sign_count &&
      recomputed.certified_task_count ==
          recomputed.exact_negative_count &&
      checked_sum(
          recomputed.directed_nonnegative_short_circuit_count,
          recomputed.exact_zero_count,
          expected_nonnegative_count) &&
      checked_sum(
          expected_nonnegative_count,
          recomputed.exact_positive_count,
          expected_nonnegative_count) &&
      recomputed.nonnegative_task_count == expected_nonnegative_count &&
      checked_sum(
          recomputed.certified_task_count,
          recomputed.nonnegative_task_count,
          classified_task_count) &&
      checked_sum(
          classified_task_count,
          recomputed.insufficient_witness_task_count,
          classified_task_count) &&
      checked_sum(
          classified_task_count,
          recomputed.invalid_authority_task_count,
          classified_task_count) &&
      checked_sum(
          classified_task_count,
          recomputed.support_overlap_task_count,
          classified_task_count) &&
      checked_sum(
          classified_task_count,
          recomputed.fixed_limb_residual_count,
          classified_task_count) &&
      classified_task_count == tasks.size();
  if (!checked_sum(
          recomputed.pruned_unordered_pair_mass,
          recomputed.residual_unordered_pair_mass,
          conserved_mass) ||
      conserved_mass != submitted_mass || !proposal_partition ||
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
      receipt.audit.directed_nonnegative_short_circuit_count !=
          recomputed.directed_nonnegative_short_circuit_count ||
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

ExactPairBlockWitnessCudaRepeatedResult
qualify_repeated_exact_pair_block_witness_pattern_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::span<const ExactPairBlockWitnessCudaTask> pattern,
    std::size_t repetition_count,
    std::uint64_t first_task_id,
    ExactPairBlockWitnessCudaConfig config) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a repeated exact pair-block witness batch requires a ready "
        "traversal lease");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_witness_cuda_maximum_closed_rank) {
    throw std::out_of_range(
        "a repeated exact pair-block witness maximum closed rank must be in "
        "[2, 6]");
  }
  if (config.task_capacity_per_point == 0U ||
      config.task_capacity_per_point >
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point) {
    throw std::out_of_range(
        "a repeated exact pair-block witness task factor must be in [1, 64]");
  }
  if (pattern.empty() || repetition_count == 0U) {
    throw std::invalid_argument(
        "a repeated exact pair-block witness batch requires a nonempty "
        "pattern and repetition count");
  }

  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  ExactPairBlockWitnessCudaRepeatedResult refused;
  ExactPairBlockWitnessCudaRepeatedAudit& refused_audit = refused.audit;
  refused_audit.point_count = source_audit.point_count;
  refused_audit.certified_node_count = source_audit.certified_node_count;
  refused_audit.maximum_closed_rank = config.maximum_closed_rank;
  refused_audit.task_capacity_per_point = config.task_capacity_per_point;
  refused_audit.pattern_task_count = pattern.size();
  refused_audit.repetition_count = repetition_count;
  refused_audit.first_task_id = first_task_id;
  refused_audit.source_snapshot_epoch = source_audit.source_snapshot_epoch;
  std::size_t logical_task_count = 0U;
  std::size_t task_capacity = 0U;
  if (!checked_product(
          pattern.size(), repetition_count, logical_task_count) ||
      !checked_product(
          source_audit.point_count,
          config.task_capacity_per_point,
          task_capacity)) {
    return refused;
  }
  refused_audit.logical_task_count = logical_task_count;
  refused_audit.task_capacity = task_capacity;
  if (logical_task_count > task_capacity ||
      source_audit.point_count > std::numeric_limits<std::uint32_t>::max() ||
      source_audit.certified_node_count >
          std::numeric_limits<std::uint32_t>::max()) {
    return refused;
  }
  if (logical_task_count - 1U >
      static_cast<std::size_t>(
          std::numeric_limits<std::uint64_t>::max() - first_task_id)) {
    throw std::overflow_error(
        "the repeated exact pair-block witness affine task identities "
        "overflow uint64");
  }
  const std::uint64_t last_task_id = first_task_id +
      static_cast<std::uint64_t>(logical_task_count - 1U);

  std::vector<detail::Phase15ExactPairBlockWitnessCudaTask> compact_pattern;
  compact_pattern.reserve(pattern.size());
  for (const ExactPairBlockWitnessCudaTask& task : pattern) {
    compact_pattern.push_back(compact_task(task));
  }
  const std::uint64_t recipe_digest = repeated_task_recipe_digest(
      compact_pattern, repetition_count, logical_task_count, first_task_id);
  const detail::Phase15ExactPairBlockWitnessCudaAdoptedTraversal traversal =
      detail::Phase15ExactPairBlockWitnessCudaTraversalAccess::consume(
          std::move(traversal_lease));
  const detail::Phase15ExactPairBlockWitnessCudaRepeatedRequest request{
      &traversal,
      compact_pattern,
      config,
      repetition_count,
      logical_task_count,
      task_capacity,
      first_task_id,
      recipe_digest};
  detail::Phase15ExactPairBlockWitnessCudaRepeatedReceipt receipt =
      detail::phase15_launch_repeated_exact_pair_block_witness_cuda(request);
  const std::size_t expected_pattern_bytes = compact_pattern.size() *
      sizeof(detail::Phase15ExactPairBlockWitnessCudaTask);
  if (!receipt.source_identity_authenticated ||
      !receipt.every_logical_task_classified_once ||
      receipt.outcomes.size() != logical_task_count ||
      receipt.host_to_device_pattern_byte_count != expected_pattern_bytes ||
      receipt.device_to_host_outcome_byte_count != logical_task_count ||
      receipt.host_fake_lifecycle_exercised != traversal.host_fake ||
      receipt.cuda_execution_performed == traversal.host_fake ||
      receipt.native_lbvh_nodes_read_on_device == traversal.host_fake ||
      (traversal.host_fake &&
       (receipt.device_arena_byte_count != 0U ||
        receipt.device_arena_allocation_count != 0U ||
        receipt.kernel_launch_count != 0U ||
        receipt.synchronization_count != 0U)) ||
      (!traversal.host_fake &&
       (receipt.device_arena_byte_count <
            expected_pattern_bytes + logical_task_count ||
        receipt.device_arena_allocation_count != 1U ||
        receipt.kernel_launch_count != 1U ||
        receipt.synchronization_count != 1U))) {
    throw std::logic_error(
        "the repeated exact pair-block witness launcher returned an "
        "unauthenticated compact outcome batch");
  }

  ExactPairBlockWitnessCudaAudit pattern_audit;
  std::vector<ExactPairBlockWitnessCudaRecord> pattern_records;
  pattern_records.reserve(pattern.size());
  for (std::size_t selector = 0U; selector < pattern.size(); ++selector) {
    const ExactPairBlockWitnessCudaRecord record = decode_repeated_outcome(
        receipt.outcomes[selector],
        first_task_id + static_cast<std::uint64_t>(selector),
        pattern[selector].support_block.unordered_pair_mass);
    validate_negative_contract(
        pattern[selector], record, config.maximum_closed_rank);
    std::size_t submitted_mass = 0U;
    if (!checked_sum(
            pattern_audit.submitted_unordered_pair_mass,
            record.unordered_pair_mass,
            submitted_mass)) {
      throw std::overflow_error(
          "the repeated exact pair-block witness pattern mass overflows");
    }
    pattern_audit.submitted_unordered_pair_mass = submitted_mass;
    accumulate_record(record, pattern_audit);
    pattern_records.push_back(record);
  }

  bool pattern_repeated_exactly = true;
  for (std::size_t logical_index = 0U;
       logical_index < receipt.outcomes.size();
       ++logical_index) {
    if (receipt.outcomes[logical_index] !=
        receipt.outcomes[logical_index % pattern.size()]) {
      pattern_repeated_exactly = false;
      break;
    }
  }
  if (!pattern_repeated_exactly) {
    throw std::logic_error(
        "the repeated exact pair-block witness kernel changed a pattern "
        "classification between repetitions");
  }

  ExactPairBlockWitnessCudaRepeatedAudit audit;
  audit.point_count = source_audit.point_count;
  audit.certified_node_count = source_audit.certified_node_count;
  audit.maximum_closed_rank = config.maximum_closed_rank;
  audit.task_capacity_per_point = config.task_capacity_per_point;
  audit.pattern_task_count = pattern.size();
  audit.repetition_count = repetition_count;
  audit.logical_task_count = logical_task_count;
  audit.task_capacity = task_capacity;
  audit.directed_negative_count = scaled_count(
      pattern_audit.directed_negative_count,
      repetition_count,
      "the repeated directed-negative count overflows");
  audit.directed_nonnegative_count = scaled_count(
      pattern_audit.directed_nonnegative_count,
      repetition_count,
      "the repeated directed-nonnegative count overflows");
  audit.directed_ambiguous_count = scaled_count(
      pattern_audit.directed_ambiguous_count,
      repetition_count,
      "the repeated directed-ambiguous count overflows");
  audit.directed_arithmetic_overflow_count = scaled_count(
      pattern_audit.directed_arithmetic_overflow_count,
      repetition_count,
      "the repeated directed-overflow count overflows");
  audit.fixed_limb_exact_decision_count = scaled_count(
      pattern_audit.fixed_limb_exact_decision_count,
      repetition_count,
      "the repeated fixed-limb exact count overflows");
  audit.exact_negative_count = scaled_count(
      pattern_audit.exact_negative_count,
      repetition_count,
      "the repeated exact-negative count overflows");
  audit.exact_zero_count = scaled_count(
      pattern_audit.exact_zero_count,
      repetition_count,
      "the repeated exact-zero count overflows");
  audit.exact_positive_count = scaled_count(
      pattern_audit.exact_positive_count,
      repetition_count,
      "the repeated exact-positive count overflows");
  audit.fixed_limb_residual_count = scaled_count(
      pattern_audit.fixed_limb_residual_count,
      repetition_count,
      "the repeated fixed-limb residual count overflows");
  audit.certified_task_count = scaled_count(
      pattern_audit.certified_task_count,
      repetition_count,
      "the repeated certified count overflows");
  audit.nonnegative_task_count = scaled_count(
      pattern_audit.nonnegative_task_count,
      repetition_count,
      "the repeated nonnegative count overflows");
  audit.insufficient_witness_task_count = scaled_count(
      pattern_audit.insufficient_witness_task_count,
      repetition_count,
      "the repeated insufficient count overflows");
  audit.invalid_authority_task_count = scaled_count(
      pattern_audit.invalid_authority_task_count,
      repetition_count,
      "the repeated invalid-authority count overflows");
  audit.support_overlap_task_count = scaled_count(
      pattern_audit.support_overlap_task_count,
      repetition_count,
      "the repeated overlap count overflows");
  audit.submitted_unordered_pair_mass = scaled_count(
      pattern_audit.submitted_unordered_pair_mass,
      repetition_count,
      "the repeated submitted mass overflows");
  audit.pruned_unordered_pair_mass = scaled_count(
      pattern_audit.pruned_unordered_pair_mass,
      repetition_count,
      "the repeated pruned mass overflows");
  audit.residual_unordered_pair_mass = scaled_count(
      pattern_audit.residual_unordered_pair_mass,
      repetition_count,
      "the repeated residual mass overflows");
  audit.host_to_device_pattern_byte_count =
      receipt.host_to_device_pattern_byte_count;
  audit.device_to_host_outcome_byte_count =
      receipt.device_to_host_outcome_byte_count;
  audit.device_arena_byte_count = receipt.device_arena_byte_count;
  audit.device_arena_allocation_count =
      receipt.device_arena_allocation_count;
  audit.kernel_launch_count = receipt.kernel_launch_count;
  audit.synchronization_count = receipt.synchronization_count;
  audit.first_task_id = first_task_id;
  audit.last_task_id = last_task_id;
  audit.source_snapshot_epoch = source_audit.source_snapshot_epoch;
  audit.repeated_task_recipe_digest = recipe_digest;
  audit.expanded_outcome_digest = expanded_outcome_digest(receipt.outcomes);
  audit.pattern_result_digest =
      detail::phase15_exact_pair_block_witness_result_digest(pattern_records);
  audit.kernel_elapsed_nanoseconds = receipt.kernel_elapsed_nanoseconds;
  audit.cuda_device = receipt.cuda_device;
  audit.native_lbvh_authority_consumed = true;
  audit.native_lbvh_nodes_read_on_device =
      receipt.native_lbvh_nodes_read_on_device;
  audit.cuda_execution_performed = receipt.cuda_execution_performed;
  audit.host_fake_lifecycle_exercised =
      receipt.host_fake_lifecycle_exercised;
  audit.affine_task_identity_range_validated = true;
  audit.repeated_pattern_expansion_validated = true;
  audit.every_logical_task_classified_once = true;
  audit.pattern_results_repeated_exactly = true;
  audit.per_logical_task_host_input_materialization_avoided = true;
  audit.compact_outcome_readback_validated = true;
  audit.local_submitted_mass_conservation_validated =
      audit.pruned_unordered_pair_mass <=
          audit.submitted_unordered_pair_mass &&
      audit.residual_unordered_pair_mass ==
          audit.submitted_unordered_pair_mass -
              audit.pruned_unordered_pair_mass;
  audit.directed_interval_never_closes_or_prunes = true;
  audit.negative_closure_requires_fixed_limb_exact_decision = true;
  if (!audit.local_submitted_mass_conservation_validated ||
      audit.certified_task_count + audit.nonnegative_task_count +
              audit.insufficient_witness_task_count +
              audit.invalid_authority_task_count +
              audit.support_overlap_task_count +
              audit.fixed_limb_residual_count !=
          logical_task_count ||
      audit.fixed_limb_exact_decision_count !=
          audit.exact_negative_count + audit.exact_zero_count +
              audit.exact_positive_count ||
      audit.certified_task_count != audit.exact_negative_count ||
      audit.global_pair_coverage_closed || audit.hierarchy_or_tree_claimed ||
      audit.slo_claimed ||
      audit.ordinary_or_higher_order_delaunay_materialized ||
      audit.public_status_claimed) {
    throw std::logic_error(
        "the repeated exact pair-block witness receipt violated its compact "
        "logical-batch contract");
  }

  ExactPairBlockWitnessCudaRepeatedResult result;
  result.status = traversal.host_fake
      ? ExactPairBlockWitnessCudaStatus::non_authoritative_host_fake
      : ExactPairBlockWitnessCudaStatus::qualified_component_batch;
  result.audit = audit;
  result.pattern_records = std::move(pattern_records);
  return result;
}

}  // namespace morsehgp3d::gpu
