#include "morsehgp3d/gpu/exact_pair_block_to_direct_pair_terminal.hpp"

#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool source_has_no_forbidden_global_structure(
    const ExactPairBlockTransactionalFrontierResidentCudaAudit& audit) {
  return !audit.global_pair_matrix_materialized &&
      !audit.ordinary_or_higher_order_delaunay_materialized &&
      !audit.global_facet_coface_or_incidence_arena_materialized &&
      !audit.hierarchy_or_tree_claimed;
}

}  // namespace

ExactPairBlockToDirectPairTerminalAttempt
ExactPairBlockToDirectPairTerminalBuilder::build(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactPairBlockToDirectPairTerminalBudget budget,
    ExactPairBlockTransactionalFrontierResidentCudaResult source) {
  ExactPairBlockToDirectPairTerminalAttempt attempt;
  attempt.requested_budget = budget;
  hierarchy::ExactDirectPairTerminalAudit& audit = attempt.audit;

  const hierarchy::ExactPairSupportCheckpointManifest manifest =
      hierarchy::make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  audit.point_count = manifest.point_count;
  audit.requested_maximum_order = manifest.requested_maximum_order;
  audit.effective_maximum_order = manifest.effective_maximum_order;
  audit.maximum_closed_rank = std::max<std::size_t>(
      2U, manifest.maximum_relevant_closed_rank);

  const ExactPairBlockTransactionalFrontierResidentCudaAudit& source_audit =
      source.audit();
  audit.unordered_pair_universe_size =
      source_audit.unordered_pair_universe_mass;
  audit.source_prune_receipt_count = source_audit.prune_receipt_count;
  audit.pruned_unordered_pair_mass =
      source_audit.pruned_unordered_pair_mass;
  audit.source_terminal_pair_count = source_audit.terminal_pair_count;
  audit.source_final_cut_digest = source_audit.final_cut_digest;
  audit.qualified_cuda_execution =
      source.status() ==
          ExactPairBlockTransactionalFrontierResidentCudaStatus::
              qualified_cuda_terminal_authority &&
      source.qualified_cuda_terminal_authority();
  audit.host_fake_execution =
      source.status() ==
          ExactPairBlockTransactionalFrontierResidentCudaStatus::
              non_authoritative_host_fake_terminal &&
      source.complete();
  if (audit.qualified_cuda_execution) {
    audit.source_kind = hierarchy::ExactDirectPairTerminalSourceKind::
        qualified_cuda_transactional_pair_cut;
  } else if (audit.host_fake_execution) {
    audit.source_kind = hierarchy::ExactDirectPairTerminalSourceKind::
        reference_host_fake_transactional_pair_cut;
  }

  audit.no_forbidden_global_structure_materialized =
      source_has_no_forbidden_global_structure(source_audit);
  audit.global_pair_matrix_materialized =
      source_audit.global_pair_matrix_materialized;
  audit.ordinary_or_higher_order_delaunay_materialized =
      source_audit.ordinary_or_higher_order_delaunay_materialized;
  audit.global_facet_coface_or_incidence_arena_materialized =
      source_audit.global_facet_coface_or_incidence_arena_materialized;
  audit.hierarchy_reduction_performed = source_audit.hierarchy_or_tree_claimed;
  audit.public_status_claimed = source_audit.public_status_claimed;

  if (!source.complete()) {
    attempt.status =
        ExactPairBlockToDirectPairTerminalStatus::source_not_terminal;
    return attempt;
  }

  audit.source_partition_validated = source.validated_for(index, cloud);
  audit.every_prune_recertified = audit.source_partition_validated;
  audit.unordered_pair_coverage_certified =
      audit.source_partition_validated &&
      source_audit.global_pair_coverage_closed &&
      source_audit.transactional_mass_conservation_validated &&
      source_audit.native_split_partition_validated &&
      source_audit.pairwise_disjoint_support_products_validated &&
      audit.pruned_unordered_pair_mass <= audit.unordered_pair_universe_size &&
      audit.source_terminal_pair_count ==
          audit.unordered_pair_universe_size -
              audit.pruned_unordered_pair_mass;
  if (!audit.source_partition_validated ||
      !audit.unordered_pair_coverage_certified ||
      source_audit.maximum_closed_rank != audit.maximum_closed_rank ||
      (!audit.qualified_cuda_execution && !audit.host_fake_execution) ||
      !audit.no_forbidden_global_structure_materialized ||
      audit.public_status_claimed) {
    attempt.status =
        ExactPairBlockToDirectPairTerminalStatus::source_authority_mismatch;
    return attempt;
  }

  std::vector<hierarchy::ExactDirectPairTerminalRecord> records;
  try {
    records.reserve(std::min(
        audit.source_terminal_pair_count,
        budget.maximum_emitted_record_count));
    for (const ExactPairBlockTransactionalFrontierCudaTerminalPair& terminal :
         source.terminal_pairs()) {
      ++audit.classification_started_count;
      hierarchy::ExactAnchoredPairCandidateClassificationContext context =
          hierarchy::ExactAnchoredPairCandidateClassificationContext::start(
              index,
              cloud,
              terminal.point_ids,
              audit.maximum_closed_rank);
      const std::size_t remaining_node_visits =
          budget.maximum_classification_node_visit_count -
          audit.classification_node_visit_count;
      const hierarchy::ExactAnchoredPairCandidateClassificationStep step =
          context.advance(
              index,
              cloud,
              hierarchy::ExactAnchoredPairCandidateClassificationBudget{
                  remaining_node_visits});
      if (!checked_add(
              audit.classification_node_visit_count,
              step.work.node_visit_count,
              audit.classification_node_visit_count) ||
          audit.classification_node_visit_count >
              budget.maximum_classification_node_visit_count) {
        throw std::logic_error(
            "the exact terminal classifier exceeded its assigned visit cap");
      }

      if (step.kind == hierarchy::
                           ExactAnchoredPairCandidateClassificationStepKind::
                               budget_exhausted) {
        ++audit.classification_budget_exhaustion_count;
        attempt.status = ExactPairBlockToDirectPairTerminalStatus::
            classification_node_visit_capacity_exhausted;
        return attempt;
      }
      if (step.kind == hierarchy::
                           ExactAnchoredPairCandidateClassificationStepKind::
                               above_rank) {
        ++audit.classification_terminal_count;
        ++audit.above_rank_count;
        continue;
      }
      if (step.kind != hierarchy::
                           ExactAnchoredPairCandidateClassificationStepKind::
                               record_ready ||
          !context.record_ready()) {
        throw std::logic_error(
            "a terminal pair classification reached an invalid state");
      }

      const auto requirements = context.pending_record_requirements();
      if (!requirements.has_value()) {
        throw std::logic_error(
            "a ready terminal pair omitted its output requirements");
      }
      if (records.size() >= budget.maximum_emitted_record_count) {
        attempt.status = ExactPairBlockToDirectPairTerminalStatus::
            output_record_capacity_exhausted;
        return attempt;
      }
      if (requirements->point_id_reference_count >
          budget.maximum_emitted_point_id_reference_count -
              audit.emitted_point_id_reference_count) {
        attempt.status = ExactPairBlockToDirectPairTerminalStatus::
            output_point_id_reference_capacity_exhausted;
        return attempt;
      }

      hierarchy::ExactAnchoredPairCandidateClassificationResult result =
          context.take_result(index, cloud);
      if (result.event.has_value() ==
          result.relevant_extra_shell_diagnostic.has_value()) {
        throw std::logic_error(
            "an exact pair terminal produced an invalid record partition");
      }
      if (result.event.has_value()) {
        records.emplace_back(std::move(*result.event));
        ++audit.accepted_event_count;
      } else {
        records.emplace_back(
            std::move(*result.relevant_extra_shell_diagnostic));
        ++audit.relevant_extra_shell_diagnostic_count;
      }
      ++audit.classification_terminal_count;
      ++audit.emitted_record_count;
      audit.emitted_point_id_reference_count +=
          requirements->point_id_reference_count;
    }
  } catch (const std::bad_alloc&) {
    attempt.status =
        ExactPairBlockToDirectPairTerminalStatus::
            operational_allocation_failure;
    return attempt;
  }

  audit.terminal_pair_classification_partition_certified =
      audit.classification_started_count == audit.source_terminal_pair_count &&
      audit.classification_terminal_count == audit.source_terminal_pair_count;
  audit.output_partition_certified =
      audit.above_rank_count <= audit.classification_terminal_count &&
      audit.emitted_record_count ==
          audit.classification_terminal_count - audit.above_rank_count &&
      audit.accepted_event_count <= audit.emitted_record_count &&
      audit.relevant_extra_shell_diagnostic_count ==
          audit.emitted_record_count - audit.accepted_event_count;
  audit.retained_records_certified =
      records.size() == audit.emitted_record_count;
  audit.complete = audit.terminal_pair_classification_partition_certified &&
      audit.output_partition_certified &&
      audit.retained_records_certified;
  if (!audit.terminal_identities_hold()) {
    audit.poisoned = true;
    attempt.status =
        ExactPairBlockToDirectPairTerminalStatus::source_authority_mismatch;
    return attempt;
  }

  attempt.authority =
      std::unique_ptr<hierarchy::ExactDirectPairTerminalAuthority>(
          new hierarchy::ExactDirectPairTerminalAuthority(
              manifest, audit, std::move(records)));
  if (!attempt.authority->sealed_in_process_terminal_authority()) {
    attempt.authority.reset();
    audit.poisoned = true;
    attempt.status =
        ExactPairBlockToDirectPairTerminalStatus::source_authority_mismatch;
    return attempt;
  }
  attempt.status = ExactPairBlockToDirectPairTerminalStatus::complete;
  return attempt;
}

ExactPairBlockToDirectPairTerminalAttempt
build_exact_pair_block_to_direct_pair_terminal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactPairBlockToDirectPairTerminalBudget budget,
    ExactPairBlockTransactionalFrontierResidentCudaResult source) {
  return ExactPairBlockToDirectPairTerminalBuilder::build(
      index,
      cloud,
      requested_maximum_order,
      budget,
      std::move(source));
}

}  // namespace morsehgp3d::gpu
