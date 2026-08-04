#include "morsehgp3d/hierarchy/direct_morse_event_vertical_propagation_journal.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t absent_index =
    std::numeric_limits<std::size_t>::max();

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_forest_rejected,
  source_link_journal_rejected,
  source_structure_inconsistent,
  parent_forest_inconsistent,
  carrier_link_lookup_inconsistent,
  source_group_input_inconsistent,
  missing_source_root_anchor,
  nonconvergent_group_anchors,
  final_root_inconsistent,
  nonterminal_carrier_unconsumed,
  terminal_latent_carrier_inconsistent,
};

struct ParentEdge {
  std::size_t order{};
  exact::ExactLevel squared_level{};
  ExactDirectMorseForestNodeId child_node_id{};
  ExactDirectMorseForestNodeId parent_node_id{};
};

[[nodiscard]] bool try_add(
    std::size_t lhs,
    std::size_t rhs,
    std::size_t& result) noexcept {
  if (rhs > std::numeric_limits<std::size_t>::max() - lhs) {
    return false;
  }
  result = lhs + rhs;
  return true;
}

[[nodiscard]] bool try_double(
    std::size_t value,
    std::size_t& result) noexcept {
  return try_add(value, value, result);
}

[[nodiscard]] ExactDirectMorseEventVerticalPropagationDecision
decision_for(BuildFailure failure) noexcept {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_budget_exhausted;
    case BuildFailure::source_forest_rejected:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_source_forest_rejected;
    case BuildFailure::source_link_journal_rejected:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_source_link_journal_rejected;
    case BuildFailure::source_structure_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_source_structure_inconsistent;
    case BuildFailure::parent_forest_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_parent_forest_inconsistent;
    case BuildFailure::carrier_link_lookup_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_carrier_link_lookup_inconsistent;
    case BuildFailure::source_group_input_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_source_group_input_inconsistent;
    case BuildFailure::missing_source_root_anchor:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_missing_source_root_anchor;
    case BuildFailure::nonconvergent_group_anchors:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_nonconvergent_group_anchors;
    case BuildFailure::final_root_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_final_root_inconsistent;
    case BuildFailure::nonterminal_carrier_unconsumed:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_nonterminal_carrier_unconsumed;
    case BuildFailure::terminal_latent_carrier_inconsistent:
      return ExactDirectMorseEventVerticalPropagationDecision::
          no_propagation_terminal_latent_carrier_inconsistent;
  }
  return ExactDirectMorseEventVerticalPropagationDecision::not_certified;
}

void initialize_scope(
    ExactDirectMorseEventVerticalPropagationJournalResult& result) noexcept {
  result.scope = ExactDirectMorseEventVerticalPropagationScope::
      all_adjacent_event_carriers_order_at_least_two_atomic_source_groups_and_reduced_final_roots_relative_to_the_conditional_direct_forest;
  result.conditional_on_direct_forest_o3_o4 = true;
  result.conditional_on_caller_fresh_upstream_forest_and_link_replay = true;
  result.gamma_totality_replayed = false;
  result.m1_reconstruction_claimed = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
}

[[nodiscard]] ExactDirectMorseEventVerticalPropagationJournalResult fail(
    ExactDirectMorseEventVerticalPropagationJournalResult&& result,
    BuildFailure failure) noexcept {
  result.carrier_anchors.clear();
  result.group_anchors.clear();
  result.group_input_references.clear();
  result.final_root_anchors.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.budget_preflight_certified = false;
  result.source_conditional_forest_compact_contract_checked = false;
  result.source_link_journal_forest_relative_freshly_rebuilt = false;
  result.source_parent_forest_reconstructed = false;
  result.one_anchor_per_adjacent_event_link = false;
  result.every_order_at_least_two_source_group_input_propagated = false;
  result.every_order_at_least_two_source_group_anchor_converged = false;
  result.every_reduced_final_root_advanced_to_lower_final_root = false;
  result.every_nonterminal_higher_order_carrier_reaches_source_group = false;
  result.terminal_maximum_rank_latent_carrier_required = false;
  result.terminal_maximum_rank_latent_carrier_retained = false;
  result.lower_rank_maps_are_composition_only = false;
  result.output_storage_flat_and_linear_in_sparse_forest = false;
  result.no_partial_scientific_payload_published = true;
  result.decision = decision_for(failure);
  initialize_scope(result);
  return std::move(result);
}

[[nodiscard]] bool preflight_within_budget(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult& links,
    const ExactDirectMorseEventVerticalPropagationBudget& budget,
    std::size_t logical_node_count,
    std::size_t group_anchor_upper_bound,
    std::size_t group_input_upper_bound,
    std::size_t final_anchor_upper_bound,
    std::size_t logical_output_upper_bound) noexcept {
  return links.links.size() <= budget.maximum_source_link_scan_count &&
         forest.atomic_groups.size() <=
             budget.maximum_source_group_scan_count &&
         forest.arm_root_bindings.size() <=
             budget.maximum_source_arm_binding_scan_count &&
         logical_node_count <= budget.maximum_source_node_scan_count &&
         forest.child_node_ids.size() <=
             budget.maximum_source_child_reference_scan_count &&
         forest.final_roots.size() <=
             budget.maximum_source_final_root_scan_count &&
         links.links.size() <= budget.maximum_carrier_anchor_count &&
         group_anchor_upper_bound <= budget.maximum_group_anchor_count &&
         group_input_upper_bound <=
             budget.maximum_group_input_reference_count &&
         final_anchor_upper_bound <=
             budget.maximum_final_root_anchor_count &&
         forest.child_node_ids.size() <=
             budget.maximum_parent_activation_count &&
         logical_output_upper_bound <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool result_storage_within_budget(
    const ExactDirectMorseEventVerticalPropagationJournalResult& result,
    const ExactDirectMorseEventVerticalPropagationBudget& budget) noexcept {
  std::size_t logical = 0U;
  return result.source_link_count <=
             budget.maximum_source_link_scan_count &&
         result.source_group_count <=
             budget.maximum_source_group_scan_count &&
         result.source_arm_binding_count <=
             budget.maximum_source_arm_binding_scan_count &&
         result.source_logical_node_count <=
             budget.maximum_source_node_scan_count &&
         result.source_child_reference_count <=
             budget.maximum_source_child_reference_scan_count &&
         result.source_final_root_count <=
             budget.maximum_source_final_root_scan_count &&
         result.carrier_anchors.size() <=
             budget.maximum_carrier_anchor_count &&
         result.group_anchors.size() <=
             budget.maximum_group_anchor_count &&
         result.group_input_references.size() <=
             budget.maximum_group_input_reference_count &&
         result.final_root_anchors.size() <=
             budget.maximum_final_root_anchor_count &&
         result.counters.parent_activation_count <=
             budget.maximum_parent_activation_count &&
         result.counters.parent_find_step_count <=
             budget.maximum_parent_find_step_count &&
         try_add(result.carrier_anchors.size(), result.group_anchors.size(),
                 logical) &&
         try_add(logical, result.group_input_references.size(), logical) &&
         try_add(logical, result.final_root_anchors.size(), logical) &&
         result.logical_output_entry_count == logical &&
         logical <= budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool payload_shape(
    const ExactDirectMorseEventVerticalPropagationJournalResult& result)
    noexcept {
  if (result.point_count == 0U ||
      result.effective_maximum_order > result.point_count ||
      result.source_forest_schema_version == 0U ||
      result.source_link_journal_schema_version == 0U ||
      result.source_forest_final_locator_stamp.schema_version !=
          direct_sparse_positive_facet_locator_schema_version ||
      result.source_forest_final_locator_stamp.external_authority_id == 0U ||
      result.carrier_anchors.size() != result.source_link_count ||
      result.counters.source_link_scan_count != result.source_link_count ||
      result.counters.source_group_scan_count != result.source_group_count ||
      result.counters.source_arm_binding_scan_count !=
          result.source_arm_binding_count ||
      result.counters.source_node_scan_count !=
          result.source_logical_node_count ||
      result.counters.source_child_reference_scan_count !=
          result.source_child_reference_count ||
      result.counters.source_final_root_scan_count !=
          result.source_final_root_count ||
      result.counters.carrier_anchor_count !=
          result.carrier_anchors.size() ||
      result.counters.group_anchor_count != result.group_anchors.size() ||
      result.counters.group_input_reference_count !=
          result.group_input_references.size() ||
      result.counters.final_root_anchor_count !=
          result.final_root_anchors.size()) {
    return false;
  }

  std::size_t latent_count = 0U;
  std::size_t terminal_count = 0U;
  std::size_t previous_birth_record_index = 0U;
  bool has_previous_birth_record = false;
  for (std::size_t index = 0U; index < result.carrier_anchors.size(); ++index) {
    const auto& anchor = result.carrier_anchors[index];
    if (anchor.carrier_anchor_index != index ||
        anchor.source_link_index != index || anchor.source_order < 2U ||
        anchor.source_order > result.effective_maximum_order ||
        anchor.source_birth_record_index < result.point_count ||
        anchor.target_root_node_id >= result.source_logical_node_count ||
        anchor.target_strict_pre_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        anchor.target_committed_post_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        anchor.target_strict_pre_batch_stamp.external_authority_id == 0U ||
        anchor.target_committed_post_batch_stamp.external_authority_id !=
            anchor.target_strict_pre_batch_stamp.external_authority_id ||
        anchor.target_strict_pre_batch_stamp.external_authority_id !=
            result.source_forest_final_locator_stamp.external_authority_id ||
        anchor.target_strict_pre_batch_stamp.committed_batch_count ==
            std::numeric_limits<std::size_t>::max() ||
        anchor.target_committed_post_batch_stamp.committed_batch_count !=
            anchor.target_strict_pre_batch_stamp.committed_batch_count + 1U ||
        anchor.source_event_arm_identity_digest == contract::CanonicalId{} ||
        (has_previous_birth_record &&
         previous_birth_record_index >= anchor.source_birth_record_index) ||
        anchor.remains_latent_without_source_group ==
            anchor.referenced_by_source_atomic_group ||
        (anchor.terminal_maximum_rank_latent_carrier &&
         (anchor.source_order != result.point_count ||
          anchor.referenced_by_source_atomic_group))) {
      return false;
    }
    if (anchor.remains_latent_without_source_group) {
      ++latent_count;
    }
    if (anchor.terminal_maximum_rank_latent_carrier) {
      ++terminal_count;
    }
    previous_birth_record_index = anchor.source_birth_record_index;
    has_previous_birth_record = true;
  }
  if (latent_count !=
          result.counters.unconsumed_latent_carrier_anchor_count ||
      terminal_count !=
          result.counters.terminal_maximum_rank_latent_carrier_anchor_count ||
      result.terminal_maximum_rank_latent_carrier_required !=
          (result.effective_maximum_order == result.point_count &&
           result.point_count >= 2U) ||
      terminal_count !=
          (result.terminal_maximum_rank_latent_carrier_required ? 1U : 0U) ||
      latent_count != terminal_count) {
    return false;
  }

  std::size_t expected_input_offset = 0U;
  std::size_t previous_atomic_group_index = 0U;
  bool has_previous_atomic_group = false;
  for (std::size_t index = 0U; index < result.group_anchors.size(); ++index) {
    const auto& anchor = result.group_anchors[index];
    if (anchor.group_anchor_index != index || anchor.source_order < 2U ||
        anchor.source_order > result.effective_maximum_order ||
        anchor.source_atomic_group_index >= result.source_group_count ||
        (has_previous_atomic_group &&
         previous_atomic_group_index >= anchor.source_atomic_group_index) ||
        anchor.source_resulting_root_node_id >=
            result.source_logical_node_count ||
        anchor.target_resulting_root_node_id >=
            result.source_logical_node_count ||
        anchor.input_reference_offset != expected_input_offset ||
        anchor.input_reference_count == 0U ||
        anchor.input_reference_offset > result.group_input_references.size() ||
        anchor.input_reference_count >
            result.group_input_references.size() -
                anchor.input_reference_offset) {
      return false;
    }
    for (std::size_t local = 0U; local < anchor.input_reference_count;
         ++local) {
      const auto& reference = result.group_input_references[
          anchor.input_reference_offset + local];
      if (reference.group_anchor_index != index ||
          reference.target_root_at_group_level !=
              anchor.target_resulting_root_node_id) {
        return false;
      }
    }
    expected_input_offset += anchor.input_reference_count;
    previous_atomic_group_index = anchor.source_atomic_group_index;
    has_previous_atomic_group = true;
  }
  if (expected_input_offset != result.group_input_references.size()) {
    return false;
  }
  for (std::size_t index = 0U;
       index < result.group_input_references.size(); ++index) {
    const auto& reference = result.group_input_references[index];
    if (reference.group_input_reference_index != index ||
        reference.group_anchor_index >= result.group_anchors.size() ||
        reference.target_root_before_advance >=
            result.source_logical_node_count ||
        reference.target_root_at_group_level >=
            result.source_logical_node_count) {
      return false;
    }
    switch (reference.kind) {
      case ExactDirectMorseEventVerticalGroupInputKind::carrier_birth_anchor:
        if (!reference.source_carrier_anchor_index.has_value() ||
            !reference.source_birth_record_index.has_value() ||
            reference.source_root_node_id.has_value() ||
            *reference.source_carrier_anchor_index >=
                result.carrier_anchors.size() ||
            result.carrier_anchors[*reference.source_carrier_anchor_index]
                    .source_birth_record_index !=
                *reference.source_birth_record_index ||
            result.carrier_anchors[*reference.source_carrier_anchor_index]
                    .target_root_node_id !=
                reference.target_root_before_advance) {
          return false;
        }
        break;
      case ExactDirectMorseEventVerticalGroupInputKind::
          prior_source_root_anchor:
        if (reference.source_carrier_anchor_index.has_value() ||
            reference.source_birth_record_index.has_value() ||
            !reference.source_root_node_id.has_value() ||
            *reference.source_root_node_id >=
                result.source_logical_node_count) {
          return false;
        }
        break;
      default:
        return false;
    }
  }
  std::size_t previous_final_root_index = 0U;
  bool has_previous_final_root = false;
  for (std::size_t index = 0U; index < result.final_root_anchors.size();
       ++index) {
    const auto& anchor = result.final_root_anchors[index];
    if (anchor.final_root_anchor_index != index ||
        anchor.source_final_root_index >= result.source_final_root_count ||
        anchor.source_order < 2U ||
        anchor.source_order > result.effective_maximum_order ||
        (has_previous_final_root &&
         previous_final_root_index >= anchor.source_final_root_index) ||
        anchor.source_root_node_id >= result.source_logical_node_count ||
        anchor.target_final_root_node_id >=
            result.source_logical_node_count) {
      return false;
    }
    previous_final_root_index = anchor.source_final_root_index;
    has_previous_final_root = true;
  }
  return true;
}

}  // namespace

bool ExactDirectMorseEventVerticalPropagationJournalResult::
    certified_conditional_event_vertical_propagation() const noexcept {
  return schema_version ==
             direct_morse_event_vertical_propagation_journal_schema_version &&
         source_higher_canonical_cloud_digest != contract::CanonicalId{} &&
         source_forest_schema_version ==
             direct_morse_forest_journal_schema_version &&
         source_link_journal_schema_version ==
             direct_morse_event_rank_tower_link_journal_schema_version &&
         decision == ExactDirectMorseEventVerticalPropagationDecision::
                         complete_certified_conditional_event_vertical_propagation &&
         scope == ExactDirectMorseEventVerticalPropagationScope::
                      all_adjacent_event_carriers_order_at_least_two_atomic_source_groups_and_reduced_final_roots_relative_to_the_conditional_direct_forest &&
         budget_preflight_certified &&
         source_conditional_forest_compact_contract_checked &&
         source_link_journal_forest_relative_freshly_rebuilt &&
         conditional_on_caller_fresh_upstream_forest_and_link_replay &&
         source_parent_forest_reconstructed &&
         one_anchor_per_adjacent_event_link &&
         every_order_at_least_two_source_group_input_propagated &&
         every_order_at_least_two_source_group_anchor_converged &&
         every_reduced_final_root_advanced_to_lower_final_root &&
         every_nonterminal_higher_order_carrier_reaches_source_group &&
         terminal_maximum_rank_latent_carrier_required ==
             (effective_maximum_order == point_count && point_count >= 2U) &&
         terminal_maximum_rank_latent_carrier_retained &&
         lower_rank_maps_are_composition_only &&
         output_storage_flat_and_linear_in_sparse_forest &&
         conditional_on_direct_forest_o3_o4 && !gamma_totality_replayed &&
         !m1_reconstruction_claimed && no_partial_scientific_payload_published &&
         !gamma_cells_or_global_cofaces_materialized &&
         !higher_order_delaunay_materialized &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed && payload_shape(*this) &&
         result_storage_within_budget(*this, requested_budget);
}

bool ExactDirectMorseEventVerticalPropagationJournalResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_event_vertical_propagation_journal_schema_version &&
         decision != ExactDirectMorseEventVerticalPropagationDecision::
                         not_certified &&
         decision != ExactDirectMorseEventVerticalPropagationDecision::
                         complete_certified_conditional_event_vertical_propagation &&
         scope == ExactDirectMorseEventVerticalPropagationScope::
                      all_adjacent_event_carriers_order_at_least_two_atomic_source_groups_and_reduced_final_roots_relative_to_the_conditional_direct_forest &&
         carrier_anchors.empty() && group_anchors.empty() &&
         group_input_references.empty() && final_root_anchors.empty() &&
         logical_output_entry_count == 0U &&
         counters == ExactDirectMorseEventVerticalPropagationCounters{} &&
         conditional_on_caller_fresh_upstream_forest_and_link_replay &&
         conditional_on_direct_forest_o3_o4 && !gamma_totality_replayed &&
         !m1_reconstruction_claimed && no_partial_scientific_payload_published &&
         !gamma_cells_or_global_cofaces_materialized &&
         !higher_order_delaunay_materialized &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

bool ExactDirectMorseEventVerticalPropagationJournalResult::
    certified_outcome() const noexcept {
  return certified_conditional_event_vertical_propagation() ||
         certified_atomic_failure();
}

ExactDirectMorseEventVerticalPropagationJournalResult
build_exact_direct_morse_event_vertical_propagation_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult&
        source_link_journal,
    const ExactDirectMorseEventRankTowerLinkBudget&
        trusted_source_link_budget,
    const ExactDirectMorseEventVerticalPropagationBudget& budget) {
  ExactDirectMorseEventVerticalPropagationJournalResult result;
  result.requested_budget = budget;
  result.trusted_source_link_budget = trusted_source_link_budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order = source_forest.effective_maximum_order;
  result.source_higher_canonical_cloud_digest =
      source_forest.source_higher_canonical_cloud_digest;
  result.source_forest_schema_version = source_forest.schema_version;
  result.source_link_journal_schema_version = source_link_journal.schema_version;
  result.source_forest_final_locator_stamp =
      source_forest.final_locator_stamp;
  result.source_link_count = source_link_journal.links.size();
  result.source_group_count = source_forest.atomic_groups.size();
  result.source_arm_binding_count = source_forest.arm_root_bindings.size();
  result.source_child_reference_count = source_forest.child_node_ids.size();
  result.source_final_root_count = source_forest.final_roots.size();
  initialize_scope(result);

  std::size_t logical_node_count = 0U;
  std::size_t group_anchor_upper_bound = 0U;
  std::size_t group_input_upper_bound = 0U;
  std::size_t final_anchor_upper_bound = 0U;
  std::size_t logical_output_upper_bound = 0U;
  if (!try_add(source_forest.implicit_order_one_prefix_count,
               source_forest.nodes.size(), logical_node_count) ||
      !try_double(source_forest.arm_root_bindings.size(),
                  group_input_upper_bound)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.source_logical_node_count = logical_node_count;
  group_anchor_upper_bound = source_forest.atomic_groups.size();
  final_anchor_upper_bound = source_forest.final_roots.size();
  if (!try_add(source_link_journal.links.size(), group_anchor_upper_bound,
               logical_output_upper_bound) ||
      !try_add(logical_output_upper_bound, group_input_upper_bound,
               logical_output_upper_bound) ||
      !try_add(logical_output_upper_bound, final_anchor_upper_bound,
               logical_output_upper_bound)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (!preflight_within_budget(
          source_forest, source_link_journal, budget, logical_node_count,
          group_anchor_upper_bound, group_input_upper_bound,
          final_anchor_upper_bound, logical_output_upper_bound)) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  result.budget_preflight_certified = true;

  try {
    if (!source_forest.certified_conditional_h0_candidate()) {
      return fail(std::move(result), BuildFailure::source_forest_rejected);
    }
    const auto source_link_verification =
        verify_exact_direct_morse_event_rank_tower_link_journal(
            source_forest, trusted_source_link_budget,
            source_link_journal);
    if (!source_link_verification.result_certified) {
      return fail(
          std::move(result), BuildFailure::source_link_journal_rejected);
    }
    if (source_link_journal.source_forest_schema_version !=
            source_forest.schema_version ||
        source_link_journal.source_forest_final_locator_stamp !=
            source_forest.final_locator_stamp ||
        source_link_journal.source_higher_canonical_cloud_digest !=
            source_forest.source_higher_canonical_cloud_digest) {
      return fail(
          std::move(result), BuildFailure::source_link_journal_rejected);
    }

    const ExactDirectMorseForestJournalView forest_view{source_forest};
    std::vector<std::size_t> carrier_anchor_by_birth(
        forest_view.birth_record_count(), absent_index);
    result.carrier_anchors.reserve(source_link_journal.links.size());
    for (std::size_t link_index = 0U;
         link_index < source_link_journal.links.size(); ++link_index) {
      const auto& link = source_link_journal.links[link_index];
      if (link.link_index != link_index ||
          link.source_birth_record_index >=
              carrier_anchor_by_birth.size() ||
          link.target_resulting_root_node_id >= logical_node_count ||
          carrier_anchor_by_birth[link.source_birth_record_index] !=
              absent_index) {
        return fail(
            std::move(result),
            BuildFailure::carrier_link_lookup_inconsistent);
      }
      const auto target =
          forest_view.node_at(link.target_resulting_root_node_id);
      if (target.order != link.target_order ||
          target.order + 1U != link.source_order) {
        return fail(
            std::move(result),
            BuildFailure::carrier_link_lookup_inconsistent);
      }
      carrier_anchor_by_birth[link.source_birth_record_index] = link_index;
      ExactDirectMorseEventVerticalCarrierAnchor anchor;
      anchor.carrier_anchor_index = link_index;
      anchor.source_link_index = link_index;
      anchor.source_birth_record_index = link.source_birth_record_index;
      anchor.source_order = link.source_order;
      anchor.squared_level = link.squared_level;
      anchor.target_root_node_id = link.target_resulting_root_node_id;
      anchor.target_strict_pre_batch_stamp =
          link.lower_strict_pre_batch_stamp;
      anchor.target_committed_post_batch_stamp =
          link.lower_committed_post_batch_stamp;
      anchor.source_event_arm_identity_digest =
          link.source_event_arm_identity_digest;
      result.carrier_anchors.push_back(std::move(anchor));
    }

    std::vector<ExactDirectMorseForestNodeId> declared_parent(
        logical_node_count,
        std::numeric_limits<ExactDirectMorseForestNodeId>::max());
    std::vector<ParentEdge> parent_edges;
    parent_edges.reserve(source_forest.child_node_ids.size());
    for (const auto& group : source_forest.atomic_groups) {
      if (group.batch_index >= source_forest.batches.size() ||
          group.resulting_root_node_id >= logical_node_count ||
          group.child_offset > source_forest.child_node_ids.size() ||
          group.child_count >
              source_forest.child_node_ids.size() - group.child_offset) {
        return fail(
            std::move(result), BuildFailure::parent_forest_inconsistent);
      }
      const auto& batch = source_forest.batches[group.batch_index];
      const auto parent = forest_view.node_at(group.resulting_root_node_id);
      if (parent.order != batch.order ||
          (group.created_node_id.has_value() &&
           parent.squared_level != batch.squared_level)) {
        return fail(
            std::move(result), BuildFailure::parent_forest_inconsistent);
      }
      for (std::size_t local = 0U; local < group.child_count; ++local) {
        const auto child_id =
            source_forest.child_node_ids[group.child_offset + local];
        if (child_id >= logical_node_count ||
            child_id == group.resulting_root_node_id ||
            declared_parent[static_cast<std::size_t>(child_id)] !=
                std::numeric_limits<ExactDirectMorseForestNodeId>::max()) {
          return fail(
              std::move(result), BuildFailure::parent_forest_inconsistent);
        }
        const auto child = forest_view.node_at(child_id);
        if (child.order != parent.order ||
            !(child.squared_level < parent.squared_level)) {
          return fail(
              std::move(result), BuildFailure::parent_forest_inconsistent);
        }
        declared_parent[static_cast<std::size_t>(child_id)] =
            group.resulting_root_node_id;
        parent_edges.push_back(
            {parent.order, parent.squared_level, child_id,
             group.resulting_root_node_id});
      }
    }
    std::sort(
        parent_edges.begin(), parent_edges.end(),
        [](const ParentEdge& lhs, const ParentEdge& rhs) {
          if (lhs.order != rhs.order) {
            return lhs.order < rhs.order;
          }
          if (lhs.squared_level != rhs.squared_level) {
            return lhs.squared_level < rhs.squared_level;
          }
          if (lhs.parent_node_id != rhs.parent_node_id) {
            return lhs.parent_node_id < rhs.parent_node_id;
          }
          return lhs.child_node_id < rhs.child_node_id;
        });

    std::vector<ExactDirectMorseForestNodeId> active_parent(
        logical_node_count);
    std::iota(active_parent.begin(), active_parent.end(),
              ExactDirectMorseForestNodeId{0U});
    std::vector<std::optional<ExactDirectMorseForestNodeId>>
        source_root_anchor(logical_node_count);
    std::vector<std::optional<ExactDirectMorseForestNodeId>>
        final_root_by_order(source_forest.effective_maximum_order + 1U);
    std::vector<std::optional<std::size_t>> final_root_index_by_order(
        source_forest.effective_maximum_order + 1U);
    for (const auto& final_root : source_forest.final_roots) {
      if (final_root.final_root_index >= source_forest.final_roots.size() ||
          final_root.order == 0U ||
          final_root.order > source_forest.effective_maximum_order ||
          final_root.root_node_id >= logical_node_count ||
          final_root_by_order[final_root.order].has_value() ||
          final_root_index_by_order[final_root.order].has_value()) {
        return fail(
            std::move(result), BuildFailure::final_root_inconsistent);
      }
      final_root_by_order[final_root.order] = final_root.root_node_id;
      final_root_index_by_order[final_root.order] =
          final_root.final_root_index;
    }

    std::size_t find_step_count = 0U;
    std::size_t compression_write_count = 0U;
    const auto find_active_root = [&active_parent, &find_step_count,
                                   &compression_write_count, &budget,
                                   logical_node_count](
                                      ExactDirectMorseForestNodeId node)
        -> std::optional<ExactDirectMorseForestNodeId> {
      if (node >= logical_node_count) {
        return std::nullopt;
      }
      ExactDirectMorseForestNodeId root = node;
      while (active_parent[static_cast<std::size_t>(root)] != root) {
        if (find_step_count >= budget.maximum_parent_find_step_count) {
          return std::nullopt;
        }
        ++find_step_count;
        root = active_parent[static_cast<std::size_t>(root)];
        if (root >= logical_node_count) {
          return std::nullopt;
        }
      }
      ExactDirectMorseForestNodeId cursor = node;
      while (active_parent[static_cast<std::size_t>(cursor)] != cursor) {
        if (find_step_count >= budget.maximum_parent_find_step_count) {
          return std::nullopt;
        }
        ++find_step_count;
        const auto next = active_parent[static_cast<std::size_t>(cursor)];
        if (active_parent[static_cast<std::size_t>(cursor)] != root) {
          active_parent[static_cast<std::size_t>(cursor)] = root;
          ++compression_write_count;
        }
        cursor = next;
      }
      return root;
    };

    std::size_t edge_cursor = 0U;
    std::size_t parent_activation_count = 0U;
    const auto activate_through =
        [&parent_edges, &edge_cursor, &active_parent,
         &parent_activation_count, &budget](
            std::size_t target_order, const exact::ExactLevel* cutoff)
        -> bool {
      if (edge_cursor < parent_edges.size() &&
          parent_edges[edge_cursor].order < target_order) {
        return false;
      }
      while (edge_cursor < parent_edges.size() &&
             parent_edges[edge_cursor].order == target_order &&
             (cutoff == nullptr ||
              !(*cutoff < parent_edges[edge_cursor].squared_level))) {
        if (parent_activation_count >=
            budget.maximum_parent_activation_count) {
          return false;
        }
        const auto& edge = parent_edges[edge_cursor];
        const std::size_t child =
            static_cast<std::size_t>(edge.child_node_id);
        if (active_parent[child] != edge.child_node_id) {
          return false;
        }
        active_parent[child] = edge.parent_node_id;
        ++parent_activation_count;
        ++edge_cursor;
      }
      return true;
    };

    result.group_anchors.reserve(group_anchor_upper_bound);
    result.group_input_references.reserve(group_input_upper_bound);
    result.final_root_anchors.reserve(final_anchor_upper_bound);
    std::size_t group_cursor = 0U;
    while (group_cursor < source_forest.atomic_groups.size()) {
      const auto& group = source_forest.atomic_groups[group_cursor];
      if (group.batch_index >= source_forest.batches.size()) {
        return fail(
            std::move(result), BuildFailure::source_structure_inconsistent);
      }
      if (source_forest.batches[group.batch_index].order != 1U) {
        break;
      }
      ++group_cursor;
    }

    std::vector<ExactDirectSparseComponentHandle> carriers;
    std::vector<ExactDirectMorseForestNodeId> prior_roots;
    for (std::size_t source_order = 2U;
         source_order <= source_forest.effective_maximum_order;
         ++source_order) {
      const std::size_t target_order = source_order - 1U;
      while (group_cursor < source_forest.atomic_groups.size()) {
        const auto& group = source_forest.atomic_groups[group_cursor];
        if (group.batch_index >= source_forest.batches.size()) {
          return fail(
              std::move(result), BuildFailure::source_structure_inconsistent);
        }
        const auto& batch = source_forest.batches[group.batch_index];
        if (batch.order > source_order) {
          break;
        }
        if (batch.order != source_order ||
            group.atomic_group_index != group_cursor ||
            group.saddle_record_offset >
                source_forest.saddle_records.size() ||
            group.saddle_record_count == 0U ||
            group.saddle_record_count >
                source_forest.saddle_records.size() -
                    group.saddle_record_offset ||
            group.resulting_root_node_id >= logical_node_count) {
          return fail(
              std::move(result),
              BuildFailure::source_group_input_inconsistent);
        }
        if (!activate_through(target_order, &batch.squared_level)) {
          return fail(
              std::move(result), BuildFailure::parent_forest_inconsistent);
        }

        carriers.clear();
        prior_roots.clear();
        for (std::size_t local_saddle = 0U;
             local_saddle < group.saddle_record_count; ++local_saddle) {
          const auto saddle_index =
              group.saddle_record_offset + local_saddle;
          const auto& saddle = source_forest.saddle_records[saddle_index];
          if (saddle.saddle_record_index != saddle_index ||
              saddle.atomic_group_index != group_cursor ||
              saddle.arm_binding_offset >
                  source_forest.arm_root_bindings.size() ||
              saddle.arm_binding_count == 0U ||
              saddle.arm_binding_count >
                  source_forest.arm_root_bindings.size() -
                      saddle.arm_binding_offset) {
            return fail(
                std::move(result),
                BuildFailure::source_group_input_inconsistent);
          }
          for (std::size_t local_arm = 0U;
               local_arm < saddle.arm_binding_count; ++local_arm) {
            const auto& binding = source_forest.arm_root_bindings[
                saddle.arm_binding_offset + local_arm];
            carriers.push_back(binding.frozen_carrier_component_handle);
            if (binding.prior_reduced_root_node_id.has_value()) {
              prior_roots.push_back(*binding.prior_reduced_root_node_id);
            }
          }
        }
        std::sort(carriers.begin(), carriers.end());
        carriers.erase(std::unique(carriers.begin(), carriers.end()),
                       carriers.end());
        std::sort(prior_roots.begin(), prior_roots.end());
        prior_roots.erase(
            std::unique(prior_roots.begin(), prior_roots.end()),
            prior_roots.end());
        if (carriers.size() != group.frozen_carrier_count ||
            prior_roots.size() != group.prior_reduced_root_count ||
            carriers.empty()) {
          return fail(
              std::move(result),
              BuildFailure::source_group_input_inconsistent);
        }
        std::size_t input_count = 0U;
        if (!try_add(carriers.size(), prior_roots.size(), input_count) ||
            result.group_input_references.size() >
                budget.maximum_group_input_reference_count ||
            input_count >
                budget.maximum_group_input_reference_count -
                    result.group_input_references.size()) {
          return fail(
              std::move(result), BuildFailure::budget_exhausted);
        }

        const std::size_t group_anchor_index = result.group_anchors.size();
        const std::size_t input_offset = result.group_input_references.size();
        std::optional<ExactDirectMorseForestNodeId> common_target;
        for (const auto carrier : carriers) {
          if (carrier >= carrier_anchor_by_birth.size()) {
            return fail(
                std::move(result),
                BuildFailure::carrier_link_lookup_inconsistent);
          }
          const auto carrier_birth = forest_view.birth_record_at(carrier);
          if (carrier_birth.order != source_order ||
              carrier_birth.source_journal_batch_index >= group.batch_index ||
              carrier_birth.source_journal_batch_index >=
                  source_forest.batches.size()) {
            return fail(
                std::move(result),
                BuildFailure::source_group_input_inconsistent);
          }
          const auto& carrier_birth_batch = source_forest.batches[
              carrier_birth.source_journal_batch_index];
          if (carrier_birth_batch.order != source_order ||
              !(carrier_birth_batch.squared_level < batch.squared_level)) {
            return fail(
                std::move(result),
                BuildFailure::source_group_input_inconsistent);
          }
          const std::size_t anchor_index = carrier_anchor_by_birth[carrier];
          if (anchor_index == absent_index ||
              anchor_index >= result.carrier_anchors.size()) {
            return fail(
                std::move(result),
                BuildFailure::carrier_link_lookup_inconsistent);
          }
          auto& carrier_anchor = result.carrier_anchors[anchor_index];
          if (carrier_anchor.source_order != source_order) {
            return fail(
                std::move(result),
                BuildFailure::carrier_link_lookup_inconsistent);
          }
          const auto advanced =
              find_active_root(carrier_anchor.target_root_node_id);
          if (!advanced.has_value()) {
            return fail(
                std::move(result), BuildFailure::budget_exhausted);
          }
          const auto advanced_node = forest_view.node_at(*advanced);
          if (advanced_node.order != target_order ||
              (common_target.has_value() && *common_target != *advanced)) {
            return fail(
                std::move(result),
                BuildFailure::nonconvergent_group_anchors);
          }
          common_target = *advanced;
          carrier_anchor.referenced_by_source_atomic_group = true;
          ExactDirectMorseEventVerticalGroupInputReference reference;
          reference.group_input_reference_index =
              result.group_input_references.size();
          reference.group_anchor_index = group_anchor_index;
          reference.kind = ExactDirectMorseEventVerticalGroupInputKind::
              carrier_birth_anchor;
          reference.source_carrier_anchor_index = anchor_index;
          reference.source_birth_record_index = carrier;
          reference.target_root_before_advance =
              carrier_anchor.target_root_node_id;
          reference.target_root_at_group_level = *advanced;
          result.group_input_references.push_back(std::move(reference));
        }
        for (const auto prior_root : prior_roots) {
          if (prior_root >= logical_node_count ||
              !source_root_anchor[static_cast<std::size_t>(prior_root)]
                   .has_value()) {
            return fail(
                std::move(result),
                BuildFailure::missing_source_root_anchor);
          }
          const auto before =
              *source_root_anchor[static_cast<std::size_t>(prior_root)];
          const auto advanced = find_active_root(before);
          if (!advanced.has_value()) {
            return fail(
                std::move(result), BuildFailure::budget_exhausted);
          }
          const auto advanced_node = forest_view.node_at(*advanced);
          if (advanced_node.order != target_order ||
              (common_target.has_value() && *common_target != *advanced)) {
            return fail(
                std::move(result),
                BuildFailure::nonconvergent_group_anchors);
          }
          common_target = *advanced;
          ExactDirectMorseEventVerticalGroupInputReference reference;
          reference.group_input_reference_index =
              result.group_input_references.size();
          reference.group_anchor_index = group_anchor_index;
          reference.kind = ExactDirectMorseEventVerticalGroupInputKind::
              prior_source_root_anchor;
          reference.source_root_node_id = prior_root;
          reference.target_root_before_advance = before;
          reference.target_root_at_group_level = *advanced;
          result.group_input_references.push_back(std::move(reference));
        }
        if (!common_target.has_value()) {
          return fail(
              std::move(result),
              BuildFailure::source_group_input_inconsistent);
        }
        const auto source_result =
            forest_view.node_at(group.resulting_root_node_id);
        if (source_result.order != source_order) {
          return fail(
              std::move(result),
              BuildFailure::source_group_input_inconsistent);
        }
        source_root_anchor[static_cast<std::size_t>(
            group.resulting_root_node_id)] = *common_target;
        ExactDirectMorseEventVerticalGroupAnchor group_anchor;
        group_anchor.group_anchor_index = group_anchor_index;
        group_anchor.source_atomic_group_index = group_cursor;
        group_anchor.source_batch_index = group.batch_index;
        group_anchor.source_order = source_order;
        group_anchor.squared_level = batch.squared_level;
        group_anchor.source_resulting_root_node_id =
            group.resulting_root_node_id;
        group_anchor.input_reference_offset = input_offset;
        group_anchor.input_reference_count = input_count;
        group_anchor.target_resulting_root_node_id = *common_target;
        result.group_anchors.push_back(std::move(group_anchor));
        ++group_cursor;
      }

      if (!activate_through(target_order, nullptr)) {
        return fail(
            std::move(result), BuildFailure::parent_forest_inconsistent);
      }
      if (source_order < final_root_index_by_order.size() &&
          final_root_index_by_order[source_order].has_value()) {
        const auto& final_root = source_forest.final_roots[
            *final_root_index_by_order[source_order]];
        if (final_root.root_node_id >= logical_node_count ||
            !source_root_anchor[
                 static_cast<std::size_t>(final_root.root_node_id)]
                 .has_value() ||
            target_order >= final_root_by_order.size() ||
            !final_root_by_order[target_order].has_value()) {
          return fail(
              std::move(result), BuildFailure::final_root_inconsistent);
        }
        const auto advanced = find_active_root(
            *source_root_anchor[
                static_cast<std::size_t>(final_root.root_node_id)]);
        if (!advanced.has_value()) {
          return fail(
              std::move(result), BuildFailure::budget_exhausted);
        }
        if (*advanced != *final_root_by_order[target_order]) {
          return fail(
              std::move(result), BuildFailure::final_root_inconsistent);
        }
        ExactDirectMorseEventVerticalFinalRootAnchor final_anchor;
        final_anchor.final_root_anchor_index =
            result.final_root_anchors.size();
        final_anchor.source_final_root_index = final_root.final_root_index;
        final_anchor.source_order = source_order;
        final_anchor.source_root_node_id = final_root.root_node_id;
        final_anchor.target_final_root_node_id = *advanced;
        result.final_root_anchors.push_back(std::move(final_anchor));
      }
    }
    if (group_cursor != source_forest.atomic_groups.size()) {
      return fail(
          std::move(result), BuildFailure::source_structure_inconsistent);
    }

    std::size_t unconsumed_count = 0U;
    std::size_t terminal_latent_count = 0U;
    const bool terminal_required =
        source_forest.effective_maximum_order == source_forest.point_count &&
        source_forest.point_count >= 2U;
    for (auto& anchor : result.carrier_anchors) {
      anchor.remains_latent_without_source_group =
          !anchor.referenced_by_source_atomic_group;
      anchor.terminal_maximum_rank_latent_carrier =
          terminal_required && anchor.source_order == source_forest.point_count;
      if (anchor.remains_latent_without_source_group) {
        ++unconsumed_count;
      }
      if (anchor.terminal_maximum_rank_latent_carrier) {
        ++terminal_latent_count;
        if (anchor.referenced_by_source_atomic_group) {
          return fail(
              std::move(result),
              BuildFailure::terminal_latent_carrier_inconsistent);
        }
      }
    }
    if ((terminal_required && terminal_latent_count != 1U) ||
        (!terminal_required && terminal_latent_count != 0U)) {
      return fail(
          std::move(result),
          BuildFailure::terminal_latent_carrier_inconsistent);
    }
    if (unconsumed_count != terminal_latent_count) {
      return fail(
          std::move(result),
          BuildFailure::nonterminal_carrier_unconsumed);
    }

    std::size_t logical_output_count = 0U;
    if (!try_add(result.carrier_anchors.size(), result.group_anchors.size(),
                 logical_output_count) ||
        !try_add(logical_output_count, result.group_input_references.size(),
                 logical_output_count) ||
        !try_add(logical_output_count, result.final_root_anchors.size(),
                 logical_output_count) ||
        logical_output_count > budget.maximum_logical_output_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    result.logical_output_entry_count = logical_output_count;
    result.counters.source_link_scan_count = source_link_journal.links.size();
    result.counters.source_group_scan_count = source_forest.atomic_groups.size();
    result.counters.source_arm_binding_scan_count =
        source_forest.arm_root_bindings.size();
    result.counters.source_node_scan_count = logical_node_count;
    result.counters.source_child_reference_scan_count =
        source_forest.child_node_ids.size();
    result.counters.source_final_root_scan_count =
        source_forest.final_roots.size();
    result.counters.carrier_anchor_count = result.carrier_anchors.size();
    result.counters.unconsumed_latent_carrier_anchor_count = unconsumed_count;
    result.counters.terminal_maximum_rank_latent_carrier_anchor_count =
        terminal_latent_count;
    result.counters.group_anchor_count = result.group_anchors.size();
    result.counters.group_input_reference_count =
        result.group_input_references.size();
    result.counters.final_root_anchor_count = result.final_root_anchors.size();
    result.counters.parent_activation_count = parent_activation_count;
    result.counters.parent_find_step_count = find_step_count;
    result.counters.path_compression_write_count = compression_write_count;
    result.source_conditional_forest_compact_contract_checked = true;
    result.source_link_journal_forest_relative_freshly_rebuilt = true;
    result.source_parent_forest_reconstructed = true;
    result.one_anchor_per_adjacent_event_link = true;
    result.every_order_at_least_two_source_group_input_propagated = true;
    result.every_order_at_least_two_source_group_anchor_converged = true;
    result.every_reduced_final_root_advanced_to_lower_final_root = true;
    result.every_nonterminal_higher_order_carrier_reaches_source_group = true;
    result.terminal_maximum_rank_latent_carrier_required = terminal_required;
    result.terminal_maximum_rank_latent_carrier_retained = true;
    result.lower_rank_maps_are_composition_only = true;
    result.output_storage_flat_and_linear_in_sparse_forest = true;
    result.no_partial_scientific_payload_published = true;
    result.decision = ExactDirectMorseEventVerticalPropagationDecision::
        complete_certified_conditional_event_vertical_propagation;
    if (!result.certified_conditional_event_vertical_propagation()) {
      throw std::logic_error(
          "a complete event vertical propagation failed its compact contract");
    }
    return result;
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  } catch (const std::out_of_range&) {
    return fail(
        std::move(result), BuildFailure::source_structure_inconsistent);
  } catch (const std::logic_error&) {
    return fail(
        std::move(result), BuildFailure::source_structure_inconsistent);
  }
}

ExactDirectMorseEventVerticalPropagationVerification
verify_exact_direct_morse_event_vertical_propagation_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult&
        source_link_journal,
    const ExactDirectMorseEventRankTowerLinkBudget&
        trusted_source_link_budget,
    const ExactDirectMorseEventVerticalPropagationBudget& trusted_budget,
    const ExactDirectMorseEventVerticalPropagationJournalResult& observed) {
  ExactDirectMorseEventVerticalPropagationVerification verification;
  verification.source_forest_compact_contract_checked =
      source_forest.certified_conditional_h0_candidate();
  const auto link_verification =
      verify_exact_direct_morse_event_rank_tower_link_journal(
          source_forest, trusted_source_link_budget, source_link_journal);
  verification.source_link_journal_forest_relative_freshly_verified =
      link_verification.result_certified;
  verification.observed_storage_within_budget =
      result_storage_within_budget(observed, trusted_budget);
  const auto expected =
      build_exact_direct_morse_event_vertical_propagation_journal(
          source_forest, source_link_journal, trusted_source_link_budget,
          trusted_budget);
  verification.expected_journal_freshly_rebuilt =
      expected.certified_conditional_event_vertical_propagation();
  verification.observed_structure_certified =
      observed.certified_conditional_event_vertical_propagation();
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.source_forest_compact_contract_checked &&
      verification.source_link_journal_forest_relative_freshly_verified &&
      verification.observed_storage_within_budget &&
      verification.expected_journal_freshly_rebuilt &&
      verification.observed_structure_certified &&
      verification.observed_recursively_equal;
  return verification;
}

ExactDirectMorseEventVerticalPropagationFreshForestVerification
verify_exact_direct_morse_event_vertical_propagation_journal_from_fresh_forest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& observed_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed_link,
    const ExactDirectMorseEventVerticalPropagationBudget&
        trusted_propagation_budget,
    const ExactDirectMorseEventVerticalPropagationJournalResult&
        observed_propagation) {
  ExactDirectMorseEventVerticalPropagationFreshForestVerification result;
  result.source_link_fresh_forest_verification =
      verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
          index, cloud, source_facade, source_event_journal,
          trusted_seed_budget, source_seed_journal, trusted_forest_budget,
          forest_config, traversal_order, observed_forest,
          trusted_link_budget, observed_link);
  result.propagation_verification =
      verify_exact_direct_morse_event_vertical_propagation_journal(
          observed_forest, observed_link, trusted_link_budget,
          trusted_propagation_budget, observed_propagation);
  result.result_certified =
      result.source_link_fresh_forest_verification.result_certified &&
      result.propagation_verification.result_certified;
  return result;
}

}  // namespace morsehgp3d::hierarchy
