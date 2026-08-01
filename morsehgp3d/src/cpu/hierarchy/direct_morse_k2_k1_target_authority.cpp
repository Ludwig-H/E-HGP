#include "morsehgp3d/hierarchy/direct_morse_k2_k1_target_authority.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using AuthorityDecision = ExactDirectMorseK2K1TargetAuthorityDecision;
using AuthorityResult = ExactDirectMorseK2K1TargetAuthorityResult;
using AuthorityScope = ExactDirectMorseK2K1TargetAuthorityScope;

[[nodiscard]] bool zero_id(const contract::CanonicalId& id) noexcept {
  return id == contract::CanonicalId{};
}

void clear_scientific_facts(AuthorityResult& result) noexcept {
  result.direct_canonical_cloud_digest = {};
  result.external_canonical_cloud_digest = {};
  result.external_hierarchy_projection_digest = {};
  result.direct_pipeline_freshly_replayed = false;
  result.direct_vertical_journal_freshly_replayed = false;
  result.external_k2_k1_hierarchy_freshly_replayed = false;
  result.canonical_point_namespace_identity_certified = false;
  result.all_observed_k2_k1_labels_present = false;
  result.all_observed_k2_k1_labels_resolved = false;
  result.every_direct_target_coverage_reconstructed_transiently = false;
  result.every_external_target_coverage_reconstructed_transiently = false;
  result.every_observed_k2_k1_target_coverage_equal = false;
  result.k2_to_k1_observed_label_target_authority_replayed = false;
  result.bounded_exhaustive_gamma_oracle_used = false;
  result.scope = AuthorityScope::unspecified;
}

[[nodiscard]] AuthorityResult fail(
    AuthorityResult result, AuthorityDecision decision) noexcept {
  clear_scientific_facts(result);
  result.no_partial_scientific_payload_published_on_failure = true;
  result.decision = decision;
  return result;
}

[[nodiscard]] bool add_overflows(
    std::size_t left, std::size_t right) noexcept {
  return right > std::numeric_limits<std::size_t>::max() - left;
}

[[nodiscard]] bool add_charge(
    std::size_t& counter, std::size_t amount,
    std::size_t maximum) noexcept {
  if (add_overflows(counter, amount) || counter > maximum ||
      amount > maximum - counter) {
    return false;
  }
  counter += amount;
  return true;
}

[[nodiscard]] bool storage_within(
    const AuthorityResult& result,
    const ExactDirectMorseK2K1TargetAuthorityBudget& budget) noexcept {
  const auto& counters = result.counters;
  return counters.forest_node_scan_count <=
             budget.maximum_forest_node_scan_count &&
         counters.forest_child_reference_scan_count <=
             budget.maximum_forest_child_reference_scan_count &&
         counters.source_batch_scan_count <=
             budget.maximum_source_batch_scan_count &&
         counters.source_binding_scan_count <=
             budget.maximum_source_binding_scan_count &&
         counters.label_resolution_scan_count <=
             budget.maximum_label_resolution_scan_count &&
         counters.group_check_scan_count <=
             budget.maximum_group_check_scan_count &&
         counters.gamma_cut_build_count <=
             budget.maximum_gamma_cut_build_count &&
         counters.gamma_active_root_scan_count <=
             budget.maximum_gamma_active_root_scan_count &&
         counters.gamma_facet_scan_count <=
             budget.maximum_gamma_facet_scan_count &&
         counters.external_checkpoint_scan_count <=
             budget.maximum_external_checkpoint_scan_count &&
         counters.direct_target_point_reference_count <=
             budget.maximum_direct_target_point_reference_count &&
         counters.external_target_point_reference_count <=
             budget.maximum_external_target_point_reference_count &&
         result.required_logical_output_entry_count <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool closed_cut_complete(
    const ExactReducedGammaCut& cut) noexcept {
  using CutDecision = ExactReducedGammaCutDecision;
  return cut.boundary == ExactReducedGammaCutBoundary::closed &&
         cut.journal_relative_cut_replay_certified &&
         (cut.decision ==
              CutDecision::complete_closed_journal_relative_reduced_gamma_cut ||
          cut.decision == CutDecision::complete_empty_prefix);
}

[[nodiscard]] bool direct_pipeline_equal_and_complete(
    const ExactDirectMorseVerticalTargetProposalPipelineResult& expected,
    const ExactDirectMorseVerticalTargetProposalPipelineResult& observed)
    noexcept {
  return expected.certified_multiorder_target_proposals() &&
         expected == observed;
}

[[nodiscard]] bool external_hierarchy_verification_complete(
    const ExactK2K1HierarchyVerification& verification,
    const ExactK2K1HierarchyResult& observed) noexcept {
  return verification.trusted_inputs_accepted &&
         verification.source_history_freshly_replayed &&
         verification.k1_emst_and_compact_forest_freshly_replayed &&
         verification.fresh_replay_certified &&
         verification.exact_k2_k1_hierarchy_decision_certified &&
         observed.well_formed_success_receipt();
}

[[nodiscard]] AuthorityResult compute_authority(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseForestJournalResult& direct_forest,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget&
        trusted_pipeline_budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult&
        observed_pipeline,
    const ExactDirectMorseVerticalBudget& trusted_vertical_budget,
    const ExactDirectMorseVerticalConfig& trusted_vertical_config,
    const ExactDirectMorseVerticalJournalResult& observed_vertical_journal,
    ExactPersistentReducedGammaOrderHistoryBudget
        trusted_source_history_budget,
    const ExactPersistentReducedGammaOrderHistory& external_source_history,
    const K1CompactForest& external_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_external_hierarchy_budget,
    const ExactK2K1HierarchyResult& observed_external_hierarchy,
    const ExactDirectMorseK2K1TargetAuthorityBudget& authority_budget) {
  AuthorityResult result;
  result.requested_budget = authority_budget;
  result.point_count = cloud.size();
  result.scope = AuthorityScope::
      bounded_n14_all_observed_resolved_direct_k2_k1_labels_against_fresh_gamma2_emst_k1_targets_only;
  result.no_partial_scientific_payload_published_on_failure = true;

  if (cloud.size() < 2U || cloud.size() > 14U ||
      direct_forest.point_count != cloud.size() ||
      direct_forest.effective_maximum_order < 2U ||
      external_source_history.point_count != cloud.size() ||
      external_source_history.order != 2U ||
      external_k1_forest.point_count != cloud.size() ||
      observed_external_hierarchy.source_order != 2U ||
      observed_external_hierarchy.target_order != 1U) {
    return fail(
        std::move(result),
        AuthorityDecision::no_authority_point_count_or_order_rejected);
  }

  try {
    const auto fresh_pipeline =
        build_exact_direct_morse_vertical_target_proposal_pipeline(
            direct_forest, index, cloud, trusted_pipeline_budget);
    if (!direct_pipeline_equal_and_complete(
            fresh_pipeline, observed_pipeline)) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_direct_pipeline_rejected);
    }

    const auto vertical_verification =
        verify_exact_direct_morse_vertical_journal(
            direct_forest,
            observed_pipeline.proposals,
            trusted_vertical_budget,
            trusted_vertical_config,
            observed_vertical_journal);
    if (!vertical_verification.result_certified ||
        !vertical_verification.expected_journal_freshly_reconstructed ||
        !vertical_verification.observed_recursively_equal ||
        !observed_vertical_journal
             .certified_conditional_vertical_candidate()) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_direct_journal_rejected);
    }

    const auto external_verification = verify_exact_k2_k1_hierarchy(
        cloud,
        trusted_source_history_budget,
        external_source_history,
        external_k1_forest,
        trusted_external_hierarchy_budget,
        observed_external_hierarchy);
    if (!external_hierarchy_verification_complete(
            external_verification, observed_external_hierarchy)) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_external_hierarchy_rejected);
    }

    const auto direct_digest =
        direct_forest.source_higher_canonical_cloud_digest;
    const auto external_digest =
        observed_external_hierarchy.provenance.canonical_cloud_digest;
    if (zero_id(direct_digest) || zero_id(external_digest) ||
        direct_digest != observed_pipeline.source_forest_canonical_cloud_digest ||
        direct_digest != observed_pipeline.replayed_canonical_cloud_digest) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_direct_pipeline_rejected);
    }

    const ExactDirectMorseVerticalAdjacentFamily* family = nullptr;
    for (const auto& candidate :
         observed_vertical_journal.adjacent_families) {
      if (candidate.source_order == 2U && candidate.target_order == 1U) {
        if (family != nullptr) {
          return fail(
              std::move(result),
              AuthorityDecision::no_authority_label_partition_rejected);
        }
        family = &candidate;
      }
    }
    if (family == nullptr || family->label_resolution_count == 0U ||
        family->label_resolution_offset >
            observed_vertical_journal.label_resolutions.size() ||
        family->label_resolution_count >
            observed_vertical_journal.label_resolutions.size() -
                family->label_resolution_offset ||
        family->group_check_offset >
            observed_vertical_journal.group_checks.size() ||
        family->group_check_count >
            observed_vertical_journal.group_checks.size() -
                family->group_check_offset) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_label_partition_rejected);
    }

    const ExactDirectMorseForestJournalView forest_view{direct_forest};
    result.counters.forest_node_scan_count = forest_view.node_count();
    result.counters.forest_child_reference_scan_count =
        direct_forest.child_node_ids.size();
    result.counters.source_batch_scan_count = direct_forest.batches.size();
    result.counters.source_binding_scan_count =
        direct_forest.arm_root_bindings.size();
    result.counters.label_resolution_scan_count =
        family->label_resolution_count;
    result.counters.group_check_scan_count = family->group_check_count;
    result.counters.observed_k2_k1_label_count =
        family->label_resolution_count;
    if (add_overflows(family->label_resolution_count, 1U)) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_capacity_overflow);
    }
    result.required_logical_output_entry_count =
        family->label_resolution_count + 1U;
    if (!storage_within(result, authority_budget)) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_budget_exhausted);
    }

    std::vector<const ExactDirectMorseVerticalGroupCheck*> group_by_atomic(
        direct_forest.atomic_groups.size(), nullptr);
    for (std::size_t offset = 0U; offset < family->group_check_count;
         ++offset) {
      const auto& check = observed_vertical_journal.group_checks[
          family->group_check_offset + offset];
      if (check.atomic_group_index >= group_by_atomic.size() ||
          group_by_atomic[check.atomic_group_index] != nullptr) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_label_partition_rejected);
      }
      group_by_atomic[check.atomic_group_index] = &check;
    }

    const std::size_t node_count = forest_view.node_count();
    std::vector<std::vector<spatial::PointId>> direct_coverages(node_count);
    std::vector<std::uint8_t> coverage_state(node_count, 0U);
    bool direct_coverage_shape_valid = true;
    std::function<bool(ExactDirectMorseForestNodeId)> materialize_coverage;
    materialize_coverage = [&](ExactDirectMorseForestNodeId node_id) {
      if (node_id >= node_count) {
        return false;
      }
      const auto logical_id = static_cast<std::size_t>(node_id);
      if (coverage_state[logical_id] == 2U) {
        return true;
      }
      if (coverage_state[logical_id] == 1U) {
        return false;
      }
      coverage_state[logical_id] = 1U;
      const auto node = forest_view.node_at(node_id);
      if (node.node_id != node_id || node.order != 1U ||
          node.child_offset > direct_forest.child_node_ids.size() ||
          node.child_count > direct_forest.child_node_ids.size() -
                                 node.child_offset) {
        return false;
      }
      auto& coverage = direct_coverages[logical_id];
      if (node_id < cloud.size()) {
        if (node.child_count != 0U ||
            node.kind != ExactDirectMorseForestNodeKind::order_one_birth) {
          return false;
        }
        coverage.push_back(static_cast<spatial::PointId>(node_id));
      } else {
        if (node.child_count == 0U ||
            node.kind != ExactDirectMorseForestNodeKind::multifusion) {
          return false;
        }
        for (std::size_t child_offset = 0U;
             child_offset < node.child_count; ++child_offset) {
          const auto child = direct_forest.child_node_ids[
              node.child_offset + child_offset];
          if (child >= node_id || !materialize_coverage(child)) {
            return false;
          }
          const auto& child_coverage =
              direct_coverages[static_cast<std::size_t>(child)];
          coverage.insert(
              coverage.end(), child_coverage.begin(), child_coverage.end());
        }
        std::sort(coverage.begin(), coverage.end());
        if (coverage.empty() ||
            std::adjacent_find(coverage.begin(), coverage.end()) !=
                coverage.end()) {
          return false;
        }
      }
      coverage_state[logical_id] = 2U;
      return true;
    };

    for (std::size_t local_label = 0U;
         local_label < family->label_resolution_count; ++local_label) {
      const std::size_t label_index =
          family->label_resolution_offset + local_label;
      const auto& label =
          observed_vertical_journal.label_resolutions[label_index];
      result.rejected_label_resolution_index = label_index;
      result.rejected_atomic_group_index = label.atomic_group_index;
      result.rejected_source_binding_index =
          label.representative_arm_root_binding_index;
      if (label.label_resolution_index != label_index ||
          label.atomic_group_index >= direct_forest.atomic_groups.size() ||
          group_by_atomic[label.atomic_group_index] == nullptr) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_label_partition_rejected);
      }
      if (label.disposition ==
          ExactDirectMorseVerticalLabelDisposition::missing) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_observed_k2_k1_label_missing);
      }
      if (label.disposition != ExactDirectMorseVerticalLabelDisposition::
                                   resolved_closed_target_root ||
          !label.closed_target_root_node_id.has_value()) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_observed_k2_k1_label_unresolved);
      }

      const auto& group =
          direct_forest.atomic_groups[label.atomic_group_index];
      if (group.atomic_group_index != label.atomic_group_index ||
          group.batch_index >= direct_forest.batches.size()) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_label_partition_rejected);
      }
      const auto& batch = direct_forest.batches[group.batch_index];
      const auto& group_check = *group_by_atomic[label.atomic_group_index];
      if (batch.order != 2U ||
          group_check.source_batch_index != batch.batch_index ||
          label_index < group_check.label_resolution_offset ||
          label_index - group_check.label_resolution_offset >=
              group_check.label_resolution_count ||
          label.representative_arm_root_binding_index >=
              direct_forest.arm_root_bindings.size()) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_label_partition_rejected);
      }

      const auto& binding = direct_forest.arm_root_bindings[
          label.representative_arm_root_binding_index];
      if (binding.binding_index !=
              label.representative_arm_root_binding_index ||
          binding.strict_arm_key.point_count != 2U ||
          binding.strict_arm_key.point_ids[0U] >=
              binding.strict_arm_key.point_ids[1U] ||
          binding.strict_arm_key.point_ids[1U] >= cloud.size()) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_source_binding_rejected);
      }
      bool binding_belongs_to_group = false;
      if (group.saddle_record_offset <= direct_forest.saddle_records.size() &&
          group.saddle_record_count <=
              direct_forest.saddle_records.size() - group.saddle_record_offset) {
        for (std::size_t saddle_offset = 0U;
             saddle_offset < group.saddle_record_count; ++saddle_offset) {
          const auto& saddle = direct_forest.saddle_records[
              group.saddle_record_offset + saddle_offset];
          if (saddle.atomic_group_index == label.atomic_group_index &&
              binding.binding_index >= saddle.arm_binding_offset &&
              binding.binding_index - saddle.arm_binding_offset <
                  saddle.arm_binding_count) {
            binding_belongs_to_group = true;
            break;
          }
        }
      }
      if (!binding_belongs_to_group) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_source_binding_rejected);
      }

      if (!add_charge(
              result.counters.gamma_cut_build_count,
              1U,
              authority_budget.maximum_gamma_cut_build_count)) {
        return fail(
            std::move(result), AuthorityDecision::no_authority_budget_exhausted);
      }
      const ExactReducedGammaCut cut = build_exact_reduced_gamma_cut(
          external_source_history,
          batch.squared_level,
          ExactReducedGammaCutBoundary::closed,
          authority_budget.closed_gamma_cut_budget);
      const ExactReducedGammaCutVerification cut_verification =
          verify_exact_reduced_gamma_cut(
              external_source_history,
              batch.squared_level,
              ExactReducedGammaCutBoundary::closed,
              authority_budget.closed_gamma_cut_budget,
              cut);
      if (!closed_cut_complete(cut) ||
          !cut_verification.fresh_journal_replay_certified ||
          !cut_verification
               .exact_journal_relative_reduced_gamma_cut_replay_decision_certified) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_gamma_cut_rejected);
      }

      const std::vector<spatial::PointId> pair{
          binding.strict_arm_key.point_ids[0U],
          binding.strict_arm_key.point_ids[1U]};
      const ExactPersistentReducedGammaActiveRoot* source_component = nullptr;
      std::size_t source_pair_occurrence_count = 0U;
      for (const auto& root : cut.active_roots) {
        if (!add_charge(
                result.counters.gamma_active_root_scan_count,
                1U,
                authority_budget.maximum_gamma_active_root_scan_count)) {
          return fail(
              std::move(result),
              AuthorityDecision::no_authority_budget_exhausted);
        }
        for (const auto& facet : root.facet_point_ids) {
          if (!add_charge(
                  result.counters.gamma_facet_scan_count,
                  1U,
                  authority_budget.maximum_gamma_facet_scan_count)) {
            return fail(
                std::move(result),
                AuthorityDecision::no_authority_budget_exhausted);
          }
          if (facet == pair) {
            ++source_pair_occurrence_count;
            source_component = &root;
          }
        }
      }
      if (source_pair_occurrence_count != 1U || source_component == nullptr) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_gamma_component_rejected);
      }

      const ExactK2K1VerticalCheckpoint* checkpoint = nullptr;
      std::size_t checkpoint_match_count = 0U;
      for (const auto& candidate :
           observed_external_hierarchy.vertical_checkpoints) {
        if (!add_charge(
                result.counters.external_checkpoint_scan_count,
                1U,
                authority_budget.maximum_external_checkpoint_scan_count)) {
          return fail(
              std::move(result),
              AuthorityDecision::no_authority_budget_exhausted);
        }
        if (candidate.squared_level == batch.squared_level &&
            candidate.source_root_node_id == source_component->root_node_id) {
          checkpoint = &candidate;
          ++checkpoint_match_count;
        }
      }
      if (checkpoint_match_count != 1U || checkpoint == nullptr ||
          checkpoint->closed_target_node_id >= external_k1_forest.node_count()) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_external_checkpoint_rejected);
      }

      const auto direct_target = *label.closed_target_root_node_id;
      if (!materialize_coverage(direct_target)) {
        direct_coverage_shape_valid = false;
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_direct_target_rejected);
      }
      const auto& direct_coverage =
          direct_coverages[static_cast<std::size_t>(direct_target)];
      const auto external_target = external_k1_forest.materialize_node(
          checkpoint->closed_target_node_id);
      if (!add_charge(
              result.counters.direct_target_point_reference_count,
              direct_coverage.size(),
              authority_budget.maximum_direct_target_point_reference_count) ||
          !add_charge(
              result.counters.external_target_point_reference_count,
              external_target.point_ids.size(),
              authority_budget.maximum_external_target_point_reference_count)) {
        return fail(
            std::move(result), AuthorityDecision::no_authority_budget_exhausted);
      }
      if (direct_coverage != external_target.point_ids) {
        return fail(
            std::move(result),
            AuthorityDecision::no_authority_target_coverage_mismatch);
      }
      ++result.counters.resolved_k2_k1_label_count;
      ++result.counters.certified_target_coverage_equality_count;
    }

    if (!direct_coverage_shape_valid) {
      return fail(
          std::move(result),
          AuthorityDecision::no_authority_direct_target_rejected);
    }
    result.rejected_label_resolution_index.reset();
    result.rejected_atomic_group_index.reset();
    result.rejected_source_binding_index.reset();
    result.direct_canonical_cloud_digest = direct_digest;
    result.external_canonical_cloud_digest = external_digest;
    result.external_hierarchy_projection_digest =
        observed_external_hierarchy.hierarchy_projection_digest;
    result.direct_pipeline_freshly_replayed = true;
    result.direct_vertical_journal_freshly_replayed = true;
    result.external_k2_k1_hierarchy_freshly_replayed = true;
    result.canonical_point_namespace_identity_certified = true;
    result.all_observed_k2_k1_labels_present = true;
    result.all_observed_k2_k1_labels_resolved = true;
    result.every_direct_target_coverage_reconstructed_transiently = true;
    result.every_external_target_coverage_reconstructed_transiently = true;
    result.every_observed_k2_k1_target_coverage_equal = true;
    result.k2_to_k1_observed_label_target_authority_replayed = true;
    result.bounded_exhaustive_gamma_oracle_used = true;
    result.decision = AuthorityDecision::
        complete_observed_resolved_k2_k1_label_target_authority_replay;
    return result;
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        AuthorityDecision::no_authority_allocation_failed);
  } catch (const std::length_error&) {
    return fail(
        std::move(result),
        AuthorityDecision::no_authority_capacity_overflow);
  } catch (...) {
    return fail(
        std::move(result),
        AuthorityDecision::no_authority_gamma_component_rejected);
  }
}

}  // namespace

bool ExactDirectMorseK2K1TargetAuthorityResult::
    certified_observed_label_target_authority() const noexcept {
  return schema_version == direct_morse_k2_k1_target_authority_schema_version &&
         point_count >= 2U && point_count <= 14U && source_order == 2U &&
         target_order == 1U &&
         direct_pipeline_freshly_replayed &&
         direct_vertical_journal_freshly_replayed &&
         external_k2_k1_hierarchy_freshly_replayed &&
         canonical_point_namespace_identity_certified &&
         all_observed_k2_k1_labels_present &&
         all_observed_k2_k1_labels_resolved &&
         every_direct_target_coverage_reconstructed_transiently &&
         every_external_target_coverage_reconstructed_transiently &&
         every_observed_k2_k1_target_coverage_equal &&
         k2_to_k1_observed_label_target_authority_replayed &&
         !bidirectional_gamma_group_completeness_replayed &&
         !silent_gamma_checkpoint_completeness_replayed &&
         !external_target_authority_replayed &&
         !global_morse_obligation_replayed &&
         !all_naturality_squares_replayed && !vertical_maps_complete &&
         !global_m1_claimed && bounded_exhaustive_gamma_oracle_used &&
         !gamma_cells_or_global_cofaces_persisted &&
         !higher_order_delaunay_materialized && !public_status_claimed &&
         no_partial_scientific_payload_published_on_failure &&
         counters.observed_k2_k1_label_count ==
             counters.resolved_k2_k1_label_count &&
         counters.observed_k2_k1_label_count > 0U &&
         counters.resolved_k2_k1_label_count ==
             counters.certified_target_coverage_equality_count &&
         required_logical_output_entry_count ==
             counters.observed_k2_k1_label_count + 1U &&
         required_logical_output_entry_count != 0U &&
         storage_within(*this, requested_budget) &&
         !rejected_label_resolution_index.has_value() &&
         !rejected_atomic_group_index.has_value() &&
         !rejected_source_binding_index.has_value() &&
         decision == AuthorityDecision::
             complete_observed_resolved_k2_k1_label_target_authority_replay &&
         scope == AuthorityScope::
             bounded_n14_all_observed_resolved_direct_k2_k1_labels_against_fresh_gamma2_emst_k1_targets_only;
}

bool ExactDirectMorseK2K1TargetAuthorityResult::certified_atomic_failure()
    const noexcept {
  return schema_version == direct_morse_k2_k1_target_authority_schema_version &&
         decision != AuthorityDecision::not_certified &&
         decision != AuthorityDecision::
             complete_observed_resolved_k2_k1_label_target_authority_replay &&
         !direct_pipeline_freshly_replayed &&
         !direct_vertical_journal_freshly_replayed &&
         !external_k2_k1_hierarchy_freshly_replayed &&
         !canonical_point_namespace_identity_certified &&
         !all_observed_k2_k1_labels_present &&
         !all_observed_k2_k1_labels_resolved &&
         !every_direct_target_coverage_reconstructed_transiently &&
         !every_external_target_coverage_reconstructed_transiently &&
         !every_observed_k2_k1_target_coverage_equal &&
         !k2_to_k1_observed_label_target_authority_replayed &&
         !bidirectional_gamma_group_completeness_replayed &&
         !silent_gamma_checkpoint_completeness_replayed &&
         !external_target_authority_replayed &&
         !global_morse_obligation_replayed &&
         !all_naturality_squares_replayed && !vertical_maps_complete &&
         !global_m1_claimed && !bounded_exhaustive_gamma_oracle_used &&
         !gamma_cells_or_global_cofaces_persisted &&
         !higher_order_delaunay_materialized && !public_status_claimed &&
         no_partial_scientific_payload_published_on_failure &&
         zero_id(direct_canonical_cloud_digest) &&
         zero_id(external_canonical_cloud_digest) &&
         zero_id(external_hierarchy_projection_digest) &&
         scope == AuthorityScope::unspecified;
}

bool ExactDirectMorseK2K1TargetAuthorityResult::certified_outcome()
    const noexcept {
  return certified_observed_label_target_authority() ||
         certified_atomic_failure();
}

ExactDirectMorseK2K1TargetAuthorityResult
build_exact_direct_morse_k2_k1_target_authority(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseForestJournalResult& direct_forest,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget&
        trusted_pipeline_budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult&
        observed_pipeline,
    const ExactDirectMorseVerticalBudget& trusted_vertical_budget,
    const ExactDirectMorseVerticalConfig& trusted_vertical_config,
    const ExactDirectMorseVerticalJournalResult& observed_vertical_journal,
    ExactPersistentReducedGammaOrderHistoryBudget
        trusted_source_history_budget,
    const ExactPersistentReducedGammaOrderHistory& external_source_history,
    const K1CompactForest& external_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_external_hierarchy_budget,
    const ExactK2K1HierarchyResult& observed_external_hierarchy,
    const ExactDirectMorseK2K1TargetAuthorityBudget& authority_budget) {
  const AuthorityResult result = compute_authority(
      index,
      cloud,
      direct_forest,
      trusted_pipeline_budget,
      observed_pipeline,
      trusted_vertical_budget,
      trusted_vertical_config,
      observed_vertical_journal,
      trusted_source_history_budget,
      external_source_history,
      external_k1_forest,
      trusted_external_hierarchy_budget,
      observed_external_hierarchy,
      authority_budget);
  const auto verification = verify_exact_direct_morse_k2_k1_target_authority(
      index,
      cloud,
      direct_forest,
      trusted_pipeline_budget,
      observed_pipeline,
      trusted_vertical_budget,
      trusted_vertical_config,
      observed_vertical_journal,
      std::move(trusted_source_history_budget),
      external_source_history,
      external_k1_forest,
      trusted_external_hierarchy_budget,
      observed_external_hierarchy,
      authority_budget,
      result);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the bounded direct K2-to-K1 target authority failed its fresh "
        "verifier");
  }
  return result;
}

ExactDirectMorseK2K1TargetAuthorityVerification
verify_exact_direct_morse_k2_k1_target_authority(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseForestJournalResult& direct_forest,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget&
        trusted_pipeline_budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult&
        observed_pipeline,
    const ExactDirectMorseVerticalBudget& trusted_vertical_budget,
    const ExactDirectMorseVerticalConfig& trusted_vertical_config,
    const ExactDirectMorseVerticalJournalResult& observed_vertical_journal,
    ExactPersistentReducedGammaOrderHistoryBudget
        trusted_source_history_budget,
    const ExactPersistentReducedGammaOrderHistory& external_source_history,
    const K1CompactForest& external_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_external_hierarchy_budget,
    const ExactK2K1HierarchyResult& observed_external_hierarchy,
    const ExactDirectMorseK2K1TargetAuthorityBudget& trusted_authority_budget,
    const ExactDirectMorseK2K1TargetAuthorityResult& observed) {
  ExactDirectMorseK2K1TargetAuthorityVerification verification;
  verification.observed_storage_within_budget =
      observed.requested_budget == trusted_authority_budget &&
      (observed.certified_atomic_failure() ||
       storage_within(observed, trusted_authority_budget));
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const AuthorityResult expected = compute_authority(
      index,
      cloud,
      direct_forest,
      trusted_pipeline_budget,
      observed_pipeline,
      trusted_vertical_budget,
      trusted_vertical_config,
      observed_vertical_journal,
      std::move(trusted_source_history_budget),
      external_source_history,
      external_k1_forest,
      trusted_external_hierarchy_budget,
      observed_external_hierarchy,
      trusted_authority_budget);
  verification.direct_pipeline_freshly_replayed =
      expected.direct_pipeline_freshly_replayed;
  verification.direct_vertical_journal_freshly_replayed =
      expected.direct_vertical_journal_freshly_replayed;
  verification.external_k2_k1_hierarchy_freshly_replayed =
      expected.external_k2_k1_hierarchy_freshly_replayed;
  verification.expected_result_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = expected == observed;
  verification.trusted_inputs_accepted =
      expected.certified_observed_label_target_authority();
  verification.result_certified =
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal && observed.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
