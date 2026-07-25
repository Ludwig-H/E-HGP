#include "morsehgp3d/gpu/direct_sparse_facet_top_k_integrated_adapter.hpp"

#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] bool add_count(
    std::size_t& destination,
    std::size_t increment) noexcept {
  if (increment >
      std::numeric_limits<std::size_t>::max() - destination) {
    return false;
  }
  destination += increment;
  return true;
}

}  // namespace

DirectSparseFacetTopKIntegratedAdapter::
    DirectSparseFacetTopKIntegratedAdapter(
        DirectSparseFacetTopKProposalContext& proposal_context,
        const spatial::CanonicalPointCloud& cloud,
        DirectSparseFacetTopKProposalPolicy policy,
        hierarchy::ExactDirectSparseFacetTopKProposalTranscriptBudget
            per_chunk_transcript_budget)
    : proposal_context_(&proposal_context),
      cloud_(&cloud),
      policy_(policy),
      per_chunk_transcript_budget_(
          per_chunk_transcript_budget),
      maximum_query_count_per_chunk_(
          proposal_context.maximum_query_count()) {
  if (maximum_query_count_per_chunk_ == 0U ||
      proposal_context.point_count() != cloud.size() ||
      !proposal_context
           .device_snapshot_adopted_from_morton_lbvh_lease() ||
      per_chunk_transcript_budget_.maximum_proposal_record_count <
          maximum_query_count_per_chunk_) {
    throw std::invalid_argument(
        "a Phase-14O integrated adapter requires one matching nonzero "
        "adopted context and a per-chunk transcript budget covering its "
        "capacity");
  }
}

std::optional<hierarchy::
    ExactDirectSparseFacetDescentBatchRunNextPreparedChunk>
DirectSparseFacetTopKIntegratedAdapter::prepare_inputs(
    const hierarchy::
        ExactDirectSparseFacetDescentBatchRunNextPrepareInputs& inputs) {
  if (!proposal_context_
           ->device_snapshot_adopted_from_morton_lbvh_lease() ||
      !inputs.cpu_batch_preflights_certified ||
      inputs.query_capacity != maximum_query_count_per_chunk_ ||
      inputs.canonical_queries.size() >
          maximum_query_count_per_chunk_ ||
      inputs.query_begin_index >
          inputs.distinct_source_facet_key_count ||
      inputs.canonical_queries.size() >
          inputs.distinct_source_facet_key_count -
              inputs.query_begin_index ||
      inputs.chunk_count == 0U ||
      inputs.chunk_index >= inputs.chunk_count) {
    return std::nullopt;
  }
  if (inputs.chunk_index == 0U) {
    audit_ = {};
  } else if (audit_.prepared_chunk_count != inputs.chunk_index) {
    return std::nullopt;
  }

  std::vector<DirectSparseFacetTopKProposalQuery> queries;
  queries.reserve(inputs.canonical_queries.size());
  for (const auto& source : inputs.canonical_queries) {
    if (!source.exact_facet_miniball_certified) {
      return std::nullopt;
    }
    queries.push_back(
        {source.source_facet_key, source.exact_query_center});
  }

  DirectSparseFacetTopKProposalBatchResult proposal =
      proposal_context_->build(
          *cloud_,
          inputs.metadata,
          queries,
          policy_,
          per_chunk_transcript_budget_);
  if (!proposal.complete_proposal_batch() ||
      !proposal.audit
           .device_snapshot_adopted_from_morton_lbvh_lease ||
      !proposal.audit.adopted_device_snapshot_owner_retained ||
      proposal.audit.snapshot_host_to_device_byte_count != 0U ||
      proposal.audit.source_morton_lbvh_lease_epoch == 0U) {
    return std::nullopt;
  }
  DirectSparseFacetTopKIntegratedAdapterAudit next_audit = audit_;
  const bool counts_fit =
      add_count(
          next_audit.supported_query_count,
          proposal.audit.gpu_supported_center_query_count) &&
      add_count(
          next_audit.snapshot_host_to_device_byte_count,
          proposal.audit.snapshot_host_to_device_byte_count) &&
      add_count(
          next_audit.active_host_to_device_query_byte_count,
          proposal.audit.active_host_to_device_query_byte_count) &&
      add_count(
          next_audit.copied_device_to_host_byte_count,
          proposal.audit.copied_device_to_host_byte_count);
  if (!counts_fit ||
      next_audit.prepared_chunk_count ==
          std::numeric_limits<std::size_t>::max()) {
    return std::nullopt;
  }
  if (next_audit.prepared_chunk_count == 0U) {
    next_audit.host_snapshot_byte_capacity =
        proposal.audit.host_snapshot_byte_capacity;
    next_audit.persistent_device_snapshot_byte_capacity =
        proposal.audit.static_device_snapshot_byte_capacity;
    next_audit.source_morton_lbvh_lease_epoch =
        proposal.audit.source_morton_lbvh_lease_epoch;
    next_audit.first_buffer_epoch = proposal.audit.buffer_epoch;
  } else if (
      next_audit.host_snapshot_byte_capacity !=
          proposal.audit.host_snapshot_byte_capacity ||
      next_audit.persistent_device_snapshot_byte_capacity !=
          proposal.audit.static_device_snapshot_byte_capacity ||
      next_audit.source_morton_lbvh_lease_epoch !=
          proposal.audit.source_morton_lbvh_lease_epoch) {
    return std::nullopt;
  }
  next_audit.last_buffer_epoch = proposal.audit.buffer_epoch;
  next_audit.every_chunk_used_adopted_device_snapshot =
      next_audit.every_chunk_used_adopted_device_snapshot &&
      proposal.audit.device_snapshot_adopted_from_morton_lbvh_lease;
  next_audit.every_chunk_retained_snapshot_owner =
      next_audit.every_chunk_retained_snapshot_owner &&
      proposal.audit.adopted_device_snapshot_owner_retained;
  next_audit.every_chunk_was_proposal_only =
      next_audit.every_chunk_was_proposal_only &&
      proposal.audit.floating_ordering_only &&
      !proposal.audit.exact_distance_or_partition_published &&
      !proposal.audit.scientific_decision_published &&
      !proposal.audit.hierarchy_reduction_or_attachment_published;
  next_audit.forbidden_global_structure_materialized =
      next_audit.forbidden_global_structure_materialized ||
      proposal.audit.forbidden_global_structure_materialized;
  next_audit.public_status_claimed =
      next_audit.public_status_claimed ||
      proposal.audit.public_status_claimed;
  ++next_audit.prepared_chunk_count;
  audit_ = next_audit;

  hierarchy::ExactDirectSparseFacetDescentBatchRunNextPreparedChunk
      prepared;
  prepared.proposal_records =
      std::move(proposal.transcript.proposal_records);
  prepared.gpu_supported_query_count =
      proposal.audit.gpu_supported_center_query_count;
  prepared.active_host_to_device_query_record_count =
      proposal.audit.active_host_to_device_query_record_count;
  prepared.active_host_to_device_query_byte_count =
      proposal.audit.active_host_to_device_query_byte_count;
  prepared.initialized_device_output_record_count =
      proposal.audit.initialized_device_output_record_count;
  prepared.initialized_device_output_byte_count =
      proposal.audit.initialized_device_output_byte_count;
  prepared.copied_device_to_host_record_count =
      proposal.audit.copied_device_to_host_record_count;
  prepared.copied_device_to_host_byte_count =
      proposal.audit.copied_device_to_host_byte_count;
  prepared.gpu_execution_performed =
      proposal.audit.gpu_execution_performed;
  prepared.proposal_only =
      proposal.audit.floating_ordering_only &&
      !proposal.audit.exact_distance_or_partition_published &&
      !proposal.audit.scientific_decision_published &&
      !proposal.audit.hierarchy_reduction_or_attachment_published;
  prepared.exact_or_scientific_decision_published =
      proposal.audit.exact_distance_or_partition_published ||
      proposal.audit.scientific_decision_published ||
      proposal.audit.hierarchy_reduction_or_attachment_published;
  prepared.forbidden_global_structure_materialized =
      proposal.audit.forbidden_global_structure_materialized;
  prepared.public_status_claimed =
      proposal.audit.public_status_claimed;
  return prepared;
}

std::optional<hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptResult>
DirectSparseFacetTopKIntegratedAdapter::seal_inputs(
    const hierarchy::
        ExactDirectSparseFacetDescentBatchRunNextSealInputs& inputs) {
  if (!proposal_context_
           ->device_snapshot_adopted_from_morton_lbvh_lease() ||
      !inputs.every_prepare_inputs_chunk_certified) {
    return std::nullopt;
  }
  DirectSparseFacetTopKIntegratedAdapterAudit next_audit = audit_;
  if (inputs.proposal_chunk_count == 0U) {
    if (inputs.selected_arm_seed_count != 0U ||
        inputs.distinct_source_facet_key_count != 0U ||
        inputs.proposal_query_count != 0U ||
        !inputs.canonical_proposal_records.empty()) {
      return std::nullopt;
    }
    next_audit = {};
  } else if (
      inputs.proposal_chunk_count !=
          next_audit.prepared_chunk_count ||
      inputs.proposal_query_count <
          next_audit.supported_query_count) {
    return std::nullopt;
  }
  hierarchy::ExactDirectSparseFacetTopKProposalTranscriptResult
      transcript =
          hierarchy::
              build_exact_direct_sparse_facet_top_k_proposal_transcript(
                  inputs.metadata,
                  inputs.canonical_proposal_records,
                  inputs.transcript_budget);
  if (!transcript.complete_proposal_transcript()) {
    return std::nullopt;
  }
  next_audit.aggregate_transcript_sealed = true;
  audit_ = next_audit;
  return transcript;
}

hierarchy::ExactDirectSparseFacetDescentBatchPrepareInputsCallback
DirectSparseFacetTopKIntegratedAdapter::prepare_inputs_callback() {
  return [this](const hierarchy::
                    ExactDirectSparseFacetDescentBatchRunNextPrepareInputs&
                        inputs) {
    return prepare_inputs(inputs);
  };
}

hierarchy::ExactDirectSparseFacetDescentBatchSealInputsCallback
DirectSparseFacetTopKIntegratedAdapter::seal_inputs_callback() {
  return [this](const hierarchy::
                    ExactDirectSparseFacetDescentBatchRunNextSealInputs&
                        inputs) {
    return seal_inputs(inputs);
  };
}

}  // namespace morsehgp3d::gpu
