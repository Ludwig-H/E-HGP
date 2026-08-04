#include "morsehgp3d/hierarchy/direct_morse_event_rank_tower_link_journal.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_forest_rejected,
  source_forest_structure_inconsistent,
  missing_adjacent_saddle_role,
  duplicate_adjacent_saddle_role,
  duplicate_adjacent_birth_role,
  adjacent_order_mismatch,
  exact_level_mismatch,
  arm_terminal_inconsistent,
  atomic_group_inconsistent,
  terminal_maximum_rank_birth_missing_or_nonunique,
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

[[nodiscard]] std::optional<std::uint64_t> birth_replay_token(
    std::size_t birth_record_index) noexcept {
  constexpr std::uint64_t modulus = 3U;
  if (birth_record_index >
      (std::numeric_limits<std::uint64_t>::max() - 1U) / modulus) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(birth_record_index) * modulus + 1U;
}

[[nodiscard]] bool valid_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t order) noexcept {
  if (key.point_count != order || order == 0U || order > point_count ||
      order > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < order; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = order; index < key.point_ids.size(); ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkDecision decision_for(
    BuildFailure failure) noexcept {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_budget_exhausted;
    case BuildFailure::source_forest_rejected:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_source_forest_rejected;
    case BuildFailure::source_forest_structure_inconsistent:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_source_forest_structure_inconsistent;
    case BuildFailure::missing_adjacent_saddle_role:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_missing_adjacent_saddle_role;
    case BuildFailure::duplicate_adjacent_saddle_role:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_duplicate_adjacent_saddle_role;
    case BuildFailure::duplicate_adjacent_birth_role:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_duplicate_adjacent_birth_role;
    case BuildFailure::adjacent_order_mismatch:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_adjacent_order_mismatch;
    case BuildFailure::exact_level_mismatch:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_exact_level_mismatch;
    case BuildFailure::arm_terminal_inconsistent:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_arm_terminal_inconsistent;
    case BuildFailure::atomic_group_inconsistent:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_atomic_group_inconsistent;
    case BuildFailure::terminal_maximum_rank_birth_missing_or_nonunique:
      return ExactDirectMorseEventRankTowerLinkDecision::
          no_link_terminal_maximum_rank_birth_missing_or_nonunique;
  }
  return ExactDirectMorseEventRankTowerLinkDecision::not_certified;
}

void initialize_scope(
    ExactDirectMorseEventRankTowerLinkJournalResult& result) noexcept {
  result.scope = ExactDirectMorseEventRankTowerLinkScope::
      all_present_rank_at_least_two_births_to_same_event_adjacent_lower_order_saddles_only;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkJournalResult fail(
    ExactDirectMorseEventRankTowerLinkJournalResult&& result,
    BuildFailure failure) noexcept {
  result.links.clear();
  result.arm_terminals.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.budget_preflight_certified = false;
  result.source_conditional_forest_certificate_replayed = false;
  result.source_forest_structure_replayed = false;
  result.every_rank_at_least_two_birth_linked_exactly_once = false;
  result.same_source_event_projection_replayed = false;
  result.adjacent_order_role_replayed = false;
  result.exact_level_identity_replayed = false;
  result.every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth =
      false;
  result.strict_pre_batch_roots_replayed = false;
  result.atomic_post_batch_target_replayed = false;
  result.terminal_maximum_rank_link_required = false;
  result.all_present_terminal_maximum_rank_births_included = false;
  result.lower_rank_links_are_composition_only = false;
  result.output_csr_and_linear_in_source_events_and_arms = false;
  result.no_partial_scientific_payload_published = true;
  result.decision = decision_for(failure);
  initialize_scope(result);
  return std::move(result);
}

[[nodiscard]] bool storage_within_budget(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseEventRankTowerLinkBudget& budget,
    std::size_t logical_birth_count,
    std::size_t logical_node_count,
    std::size_t logical_output_upper_bound) noexcept {
  return logical_birth_count <=
             budget.maximum_forest_birth_record_scan_count &&
         forest.saddle_records.size() <=
             budget.maximum_forest_saddle_record_scan_count &&
         forest.arm_root_bindings.size() <=
             budget.maximum_forest_arm_binding_scan_count &&
         forest.atomic_groups.size() <=
             budget.maximum_forest_atomic_group_scan_count &&
         forest.batches.size() <=
             budget.maximum_forest_batch_scan_count &&
         logical_node_count <= budget.maximum_forest_node_scan_count &&
         forest.birth_records.size() <= budget.maximum_link_count &&
         forest.arm_root_bindings.size() <=
             budget.maximum_arm_terminal_reference_count &&
         logical_output_upper_bound <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool result_storage_within_budget(
    const ExactDirectMorseEventRankTowerLinkJournalResult& result,
    const ExactDirectMorseEventRankTowerLinkBudget& budget) noexcept {
  std::size_t expected_logical_count = 0U;
  return result.source_logical_birth_record_count <=
             budget.maximum_forest_birth_record_scan_count &&
         result.source_saddle_record_count <=
             budget.maximum_forest_saddle_record_scan_count &&
         result.source_arm_binding_count <=
             budget.maximum_forest_arm_binding_scan_count &&
         result.source_atomic_group_count <=
             budget.maximum_forest_atomic_group_scan_count &&
         result.source_batch_count <=
             budget.maximum_forest_batch_scan_count &&
         result.counters.forest_node_scan_count <=
             budget.maximum_forest_node_scan_count &&
         result.links.size() <= budget.maximum_link_count &&
         result.arm_terminals.size() <=
             budget.maximum_arm_terminal_reference_count &&
         try_add(
             result.links.size(),
             result.arm_terminals.size(),
             expected_logical_count) &&
         result.logical_output_entry_count == expected_logical_count &&
         result.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool payload_shape(
    const ExactDirectMorseEventRankTowerLinkJournalResult& result) noexcept {
  const bool terminal_required =
      result.point_count >= 2U &&
      result.effective_maximum_order == result.point_count;
  std::size_t maximum_projection_count = 0U;
  if (!try_add(
          result.source_logical_birth_record_count,
          result.source_saddle_record_count,
          maximum_projection_count)) {
    return false;
  }
  if (result.point_count == 0U ||
      result.effective_maximum_order > result.point_count ||
      result.source_forest_schema_version !=
          direct_morse_forest_journal_schema_version ||
      result.source_forest_final_locator_stamp.schema_version !=
          direct_sparse_positive_facet_locator_schema_version ||
      result.source_forest_final_locator_stamp.external_authority_id == 0U ||
      result.source_forest_final_locator_stamp.committed_batch_count !=
          result.source_batch_count ||
      result.source_event_projection_count < result.point_count ||
      result.source_event_projection_count > maximum_projection_count ||
      result.terminal_maximum_rank_link_required != terminal_required ||
      result.source_logical_birth_record_count < result.point_count ||
      result.links.size() !=
          result.source_logical_birth_record_count - result.point_count ||
      result.counters.higher_rank_birth_count != result.links.size() ||
      result.counters.link_count != result.links.size() ||
      result.counters.arm_terminal_reference_count !=
          result.arm_terminals.size() ||
      result.counters.forest_birth_record_scan_count !=
          result.source_logical_birth_record_count ||
      result.counters.forest_saddle_record_scan_count !=
          result.source_saddle_record_count ||
      result.counters.forest_arm_binding_scan_count !=
          result.source_arm_binding_count ||
      result.counters.forest_atomic_group_scan_count !=
          result.source_atomic_group_count ||
      result.counters.forest_batch_scan_count !=
          result.source_batch_count) {
    return false;
  }
  std::size_t expected_arm_offset = 0U;
  std::size_t terminal_count = 0U;
  std::size_t previous_birth_index = 0U;
  bool has_previous_birth = false;
  for (std::size_t link_index = 0U;
       link_index < result.links.size();
       ++link_index) {
    const auto& link = result.links[link_index];
    if (link.link_index != link_index ||
        link.source_birth_record_index !=
            result.point_count + link_index ||
        link.source_birth_record_index >=
            result.source_logical_birth_record_count ||
        link.source_event_projection_index < result.point_count ||
        link.source_event_projection_index >=
            result.source_event_projection_count ||
        link.source_order < 2U ||
        link.source_order > result.effective_maximum_order ||
        link.target_order + 1U != link.source_order ||
        link.saddle_record_index >= result.source_saddle_record_count ||
        link.lower_batch_index >= result.source_batch_count ||
        link.atomic_group_index >= result.source_atomic_group_count ||
        link.target_resulting_root_node_id >=
            result.counters.forest_node_scan_count ||
        link.arm_terminal_offset != expected_arm_offset ||
        link.arm_terminal_count == 0U ||
        link.arm_terminal_offset > result.arm_terminals.size() ||
        link.arm_terminal_count >
            result.arm_terminals.size() - link.arm_terminal_offset ||
        link.lower_strict_pre_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        link.lower_strict_pre_batch_stamp.external_authority_id !=
            result.source_forest_final_locator_stamp.external_authority_id ||
        link.lower_strict_pre_batch_stamp.committed_batch_count !=
            link.lower_batch_index ||
        link.lower_committed_post_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        link.lower_committed_post_batch_stamp.external_authority_id !=
            result.source_forest_final_locator_stamp.external_authority_id ||
        link.lower_batch_index ==
            std::numeric_limits<std::size_t>::max() ||
        link.lower_committed_post_batch_stamp.committed_batch_count !=
            link.lower_batch_index + 1U ||
        link.source_event_arm_identity_digest ==
            contract::CanonicalId{} ||
        (has_previous_birth &&
         previous_birth_index >= link.source_birth_record_index)) {
      return false;
    }
    for (std::size_t local = 0U;
         local < link.arm_terminal_count;
         ++local) {
      const auto& terminal = result.arm_terminals[
          link.arm_terminal_offset + local];
      const auto expected_terminal_token = birth_replay_token(
          terminal.terminal_birth_record_index);
      if (static_cast<std::size_t>(terminal.removed_support_point_id) >=
              result.point_count ||
          !valid_facet_key(
              terminal.terminal_birth_facet_key,
              result.point_count,
              link.target_order) ||
          !expected_terminal_token.has_value() ||
          terminal.terminal_birth_binding_witness.external_authority_id !=
              result.source_forest_final_locator_stamp
                  .external_authority_id ||
          terminal.terminal_birth_binding_witness.replay_token !=
              *expected_terminal_token ||
          !(terminal.terminal_birth_exact_squared_level <
            link.squared_level)) {
        return false;
      }
    }
    if (result.effective_maximum_order == result.point_count &&
        link.source_order == result.point_count) {
      ++terminal_count;
    }
    previous_birth_index = link.source_birth_record_index;
    has_previous_birth = true;
    expected_arm_offset += link.arm_terminal_count;
  }
  if (expected_arm_offset != result.arm_terminals.size() ||
      terminal_count !=
          result.counters.terminal_maximum_rank_birth_count ||
      (terminal_required && terminal_count != 1U) ||
      (!terminal_required && terminal_count != 0U)) {
    return false;
  }
  for (std::size_t reference_index = 0U;
       reference_index < result.arm_terminals.size();
       ++reference_index) {
    const auto& terminal = result.arm_terminals[reference_index];
    if (terminal.arm_terminal_reference_index != reference_index ||
        terminal.source_arm_root_binding_index >=
            result.source_arm_binding_count ||
        terminal.terminal_birth_record_index >=
            result.source_logical_birth_record_count ||
        terminal.frozen_carrier_component_handle >=
            result.source_logical_birth_record_count ||
        (terminal.prior_reduced_root_node_id.has_value() &&
         *terminal.prior_reduced_root_node_id >=
             result.counters.forest_node_scan_count)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<BuildFailure> validate_atomic_groups_and_arms(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseForestJournalView& view,
    std::size_t logical_birth_count,
    std::size_t logical_node_count) {
  for (std::size_t group_index = 0U;
       group_index < forest.atomic_groups.size();
       ++group_index) {
    const auto& group = forest.atomic_groups[group_index];
    if (group.atomic_group_index != group_index ||
        group.batch_index >= forest.batches.size() ||
        group.saddle_record_offset > forest.saddle_records.size() ||
        group.saddle_record_count == 0U ||
        group.saddle_record_count >
            forest.saddle_records.size() - group.saddle_record_offset ||
        group.child_offset > forest.child_node_ids.size() ||
        group.child_count >
            forest.child_node_ids.size() - group.child_offset ||
        group.resulting_root_node_id >= logical_node_count) {
      return BuildFailure::atomic_group_inconsistent;
    }
    const auto& batch = forest.batches[group.batch_index];
    if (batch.batch_index != group.batch_index ||
        batch.source_journal_batch_index != group.batch_index ||
        batch.order == 0U ||
        group_index < batch.atomic_group_offset ||
        group_index - batch.atomic_group_offset >=
            batch.atomic_group_count ||
        batch.strict_pre_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        batch.strict_pre_batch_stamp.external_authority_id !=
            forest.final_locator_stamp.external_authority_id ||
        batch.strict_pre_batch_stamp.committed_batch_count !=
            group.batch_index ||
        batch.committed_batch_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        batch.committed_batch_stamp.external_authority_id !=
            forest.final_locator_stamp.external_authority_id ||
        group.batch_index == std::numeric_limits<std::size_t>::max() ||
        batch.committed_batch_stamp.committed_batch_count !=
            group.batch_index + 1U ||
        !batch.strict_arms_resolved_before_mutation ||
        !batch.quotient_resolved_before_mutation ||
        !batch.unions_then_births_committed_atomically) {
      return BuildFailure::atomic_group_inconsistent;
    }

    std::vector<ExactDirectSparseComponentHandle> carriers;
    std::vector<ExactDirectMorseForestNodeId> prior_roots;
    for (std::size_t local_saddle = 0U;
         local_saddle < group.saddle_record_count;
         ++local_saddle) {
      const std::size_t saddle_index =
          group.saddle_record_offset + local_saddle;
      const auto& saddle = forest.saddle_records[saddle_index];
      std::size_t expected_projection_index = 0U;
      if (!try_add(
              forest.point_count,
              saddle.source_event_index,
              expected_projection_index)) {
        return BuildFailure::source_forest_structure_inconsistent;
      }
      if (saddle.saddle_record_index != saddle_index ||
          saddle.journal_event_projection_index !=
              expected_projection_index ||
          saddle.journal_event_projection_index < forest.point_count ||
          saddle.journal_event_projection_index >=
              forest.source_event_projection_count ||
          saddle.source_event_arm_identity_digest ==
              contract::CanonicalId{} ||
          saddle.atomic_group_index != group_index ||
          saddle.source_journal_batch_index != group.batch_index ||
          saddle_index < batch.saddle_record_offset ||
          saddle_index - batch.saddle_record_offset >=
              batch.saddle_record_count ||
          saddle.arm_binding_offset > forest.arm_root_bindings.size() ||
          saddle.arm_binding_count == 0U ||
          saddle.arm_binding_count >
              forest.arm_root_bindings.size() -
                  saddle.arm_binding_offset) {
        return BuildFailure::atomic_group_inconsistent;
      }
      for (std::size_t local_arm = 0U;
           local_arm < saddle.arm_binding_count;
           ++local_arm) {
        const std::size_t binding_index =
            saddle.arm_binding_offset + local_arm;
        const auto& binding = forest.arm_root_bindings[binding_index];
        if (binding.binding_index != binding_index ||
            binding.source_family_index != saddle.source_family_index ||
            static_cast<std::size_t>(
                binding.removed_support_point_id) >= forest.point_count ||
            binding.frozen_carrier_component_handle >=
                logical_birth_count ||
            binding.terminal_birth_record_index >= logical_birth_count) {
          return BuildFailure::arm_terminal_inconsistent;
        }
        for (std::size_t key_index = 0U;
             key_index < binding.strict_arm_key.point_count;
             ++key_index) {
          if (binding.strict_arm_key.point_ids[key_index] ==
              binding.removed_support_point_id) {
            return BuildFailure::arm_terminal_inconsistent;
          }
        }
        const auto carrier_birth = view.birth_record_at(
            binding.frozen_carrier_component_handle);
        const auto terminal_birth = view.birth_record_at(
            binding.terminal_birth_record_index);
        const auto expected_terminal_token = birth_replay_token(
            binding.terminal_birth_record_index);
        if (carrier_birth.order != batch.order ||
            carrier_birth.source_journal_batch_index >= group.batch_index ||
            carrier_birth.source_journal_batch_index >=
                forest.batches.size() ||
            terminal_birth.order != batch.order ||
            !valid_facet_key(
                binding.strict_arm_key,
                forest.point_count,
                batch.order) ||
            !valid_facet_key(
                binding.terminal_birth_facet_key,
                forest.point_count,
                batch.order) ||
            !expected_terminal_token.has_value() ||
            terminal_birth.source_journal_batch_index >=
                group.batch_index ||
            terminal_birth.source_journal_batch_index >=
                forest.batches.size()) {
          return BuildFailure::arm_terminal_inconsistent;
        }
        const auto& carrier_birth_batch = forest.batches[
            carrier_birth.source_journal_batch_index];
        const auto& terminal_batch = forest.batches[
            terminal_birth.source_journal_batch_index];
        if (carrier_birth_batch.order != batch.order ||
            !(carrier_birth_batch.squared_level < batch.squared_level) ||
            terminal_batch.order != batch.order ||
            !(terminal_batch.squared_level < batch.squared_level) ||
            binding.terminal_birth_facet_key !=
                terminal_birth.facet_key ||
            binding.terminal_birth_binding_witness !=
                terminal_birth.binding_witness ||
            binding.terminal_birth_binding_witness.external_authority_id !=
                forest.final_locator_stamp.external_authority_id ||
            binding.terminal_birth_binding_witness.replay_token !=
                *expected_terminal_token ||
            binding.terminal_birth_exact_squared_level !=
                terminal_batch.squared_level) {
          return BuildFailure::arm_terminal_inconsistent;
        }
        carriers.push_back(binding.frozen_carrier_component_handle);
        if (binding.prior_reduced_root_node_id.has_value()) {
          const auto root_id = *binding.prior_reduced_root_node_id;
          if (root_id >= logical_node_count) {
            return BuildFailure::arm_terminal_inconsistent;
          }
          const auto root = view.node_at(root_id);
          if (root.order != batch.order ||
              !(root.squared_level < batch.squared_level)) {
            return BuildFailure::arm_terminal_inconsistent;
          }
          prior_roots.push_back(root_id);
        }
      }
    }
    std::sort(carriers.begin(), carriers.end());
    carriers.erase(
        std::unique(carriers.begin(), carriers.end()), carriers.end());
    std::sort(prior_roots.begin(), prior_roots.end());
    prior_roots.erase(
        std::unique(prior_roots.begin(), prior_roots.end()),
        prior_roots.end());
    if (group.prior_reduced_root_count >
            group.frozen_carrier_count ||
        carriers.size() != group.frozen_carrier_count ||
        prior_roots.size() != group.prior_reduced_root_count ||
        carriers.size() > batch.strict_pre_batch_carrier_count ||
        prior_roots.size() >
            batch.strict_pre_batch_reduced_root_count ||
        group.latent_carrier_count !=
            group.frozen_carrier_count -
                group.prior_reduced_root_count) {
      return BuildFailure::atomic_group_inconsistent;
    }

    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
        if (!prior_roots.empty() || group.child_count != 0U ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id) {
          return BuildFailure::atomic_group_inconsistent;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        if (prior_roots.size() != 1U || group.child_count != 0U ||
            group.created_node_id.has_value() ||
            group.resulting_root_node_id != prior_roots.front()) {
          return BuildFailure::atomic_group_inconsistent;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion:
        if (prior_roots.size() < 2U ||
            group.child_count != prior_roots.size() ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id ||
            !std::equal(
                prior_roots.begin(),
                prior_roots.end(),
                forest.child_node_ids.begin() +
                    static_cast<std::ptrdiff_t>(group.child_offset))) {
          return BuildFailure::atomic_group_inconsistent;
        }
        break;
      default:
        return BuildFailure::atomic_group_inconsistent;
    }
    const auto target = view.node_at(group.resulting_root_node_id);
    if (target.order != batch.order) {
      return BuildFailure::atomic_group_inconsistent;
    }
    if (group.created_node_id.has_value() &&
        (target.atomic_group_index !=
             std::optional<std::size_t>{group_index} ||
         target.squared_level != batch.squared_level ||
         target.child_offset != group.child_offset ||
         target.child_count != group.child_count)) {
      return BuildFailure::atomic_group_inconsistent;
    }
  }
  return std::nullopt;
}

}  // namespace

bool ExactDirectMorseEventRankTowerLinkJournalResult::
    certified_conditional_event_rank_tower_links() const noexcept {
  return schema_version ==
             direct_morse_event_rank_tower_link_journal_schema_version &&
         source_higher_canonical_cloud_digest != contract::CanonicalId{} &&
         decision == ExactDirectMorseEventRankTowerLinkDecision::
                         complete_certified_conditional_event_rank_tower_links &&
         scope == ExactDirectMorseEventRankTowerLinkScope::
                      all_present_rank_at_least_two_births_to_same_event_adjacent_lower_order_saddles_only &&
         budget_preflight_certified &&
         source_conditional_forest_certificate_replayed &&
         source_forest_structure_replayed &&
         every_rank_at_least_two_birth_linked_exactly_once &&
         same_source_event_projection_replayed &&
         adjacent_order_role_replayed && exact_level_identity_replayed &&
         every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth &&
         strict_pre_batch_roots_replayed &&
         atomic_post_batch_target_replayed &&
         terminal_maximum_rank_link_required ==
             (point_count >= 2U &&
              effective_maximum_order == point_count) &&
         all_present_terminal_maximum_rank_births_included &&
         lower_rank_links_are_composition_only &&
         output_csr_and_linear_in_source_events_and_arms &&
         no_partial_scientific_payload_published &&
         !gamma_cells_or_global_cofaces_materialized &&
         !higher_order_delaunay_materialized &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed && payload_shape(*this) &&
         result_storage_within_budget(*this, requested_budget);
}

bool ExactDirectMorseEventRankTowerLinkJournalResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_event_rank_tower_link_journal_schema_version &&
         decision != ExactDirectMorseEventRankTowerLinkDecision::
                         not_certified &&
         decision != ExactDirectMorseEventRankTowerLinkDecision::
                         complete_certified_conditional_event_rank_tower_links &&
         scope == ExactDirectMorseEventRankTowerLinkScope::
                      all_present_rank_at_least_two_births_to_same_event_adjacent_lower_order_saddles_only &&
         links.empty() && arm_terminals.empty() &&
         logical_output_entry_count == 0U &&
         counters == ExactDirectMorseEventRankTowerLinkCounters{} &&
         no_partial_scientific_payload_published &&
         !gamma_cells_or_global_cofaces_materialized &&
         !higher_order_delaunay_materialized &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

bool ExactDirectMorseEventRankTowerLinkJournalResult::certified_outcome()
    const noexcept {
  return certified_conditional_event_rank_tower_links() ||
         certified_atomic_failure();
}

ExactDirectMorseEventRankTowerLinkJournalResult
build_exact_direct_morse_event_rank_tower_link_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& budget) {
  ExactDirectMorseEventRankTowerLinkJournalResult result;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order =
      source_forest.effective_maximum_order;
  result.source_higher_canonical_cloud_digest =
      source_forest.source_higher_canonical_cloud_digest;
  result.source_forest_schema_version = source_forest.schema_version;
  result.source_forest_final_locator_stamp =
      source_forest.final_locator_stamp;
  result.source_saddle_record_count =
      source_forest.saddle_records.size();
  result.source_arm_binding_count =
      source_forest.arm_root_bindings.size();
  result.source_atomic_group_count =
      source_forest.atomic_groups.size();
  result.source_batch_count = source_forest.batches.size();
  result.source_event_projection_count =
      source_forest.source_event_projection_count;
  initialize_scope(result);

  std::size_t logical_birth_count = 0U;
  std::size_t logical_node_count = 0U;
  std::size_t output_upper_bound = 0U;
  if (!try_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.birth_records.size(),
          logical_birth_count) ||
      !try_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.nodes.size(),
          logical_node_count) ||
      !try_add(
          source_forest.birth_records.size(),
          source_forest.arm_root_bindings.size(),
          output_upper_bound)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.source_logical_birth_record_count = logical_birth_count;
  if (!storage_within_budget(
          source_forest,
          budget,
          logical_birth_count,
          logical_node_count,
          output_upper_bound)) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  result.budget_preflight_certified = true;

  try {
    const ExactDirectMorseForestJournalView view{source_forest};
    if (source_forest.source_event_projection_count <
            source_forest.point_count ||
        source_forest.final_locator_stamp.schema_version !=
            direct_sparse_positive_facet_locator_schema_version ||
        source_forest.final_locator_stamp.external_authority_id == 0U ||
        source_forest.final_locator_stamp.committed_batch_count !=
            source_forest.batches.size()) {
      return fail(
          std::move(result),
          BuildFailure::source_forest_structure_inconsistent);
    }
    if (const auto group_failure = validate_atomic_groups_and_arms(
            source_forest,
            view,
            logical_birth_count,
            logical_node_count);
        group_failure.has_value()) {
      return fail(std::move(result), *group_failure);
    }

    const std::size_t direct_event_projection_count =
        source_forest.source_event_projection_count -
        source_forest.point_count;
    std::size_t maximum_sparse_event_count = 0U;
    if (!try_add(
            source_forest.birth_records.size(),
            source_forest.saddle_records.size(),
            maximum_sparse_event_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (direct_event_projection_count > maximum_sparse_event_count) {
      return fail(
          std::move(result),
          BuildFailure::source_forest_structure_inconsistent);
    }
    constexpr std::size_t missing_record =
        std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> saddle_by_direct_event(
        direct_event_projection_count, missing_record);
    std::vector<bool> birth_seen_by_direct_event(
        direct_event_projection_count, false);
    for (std::size_t saddle_index = 0U;
         saddle_index < source_forest.saddle_records.size();
         ++saddle_index) {
      const auto& saddle = source_forest.saddle_records[saddle_index];
      std::size_t expected_projection_index = 0U;
      if (!try_add(
              source_forest.point_count,
              saddle.source_event_index,
              expected_projection_index)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
      if (saddle.journal_event_projection_index !=
              expected_projection_index ||
          saddle.journal_event_projection_index <
              source_forest.point_count ||
          saddle.journal_event_projection_index >=
              source_forest.source_event_projection_count ||
          saddle.source_event_arm_identity_digest ==
              contract::CanonicalId{}) {
        return fail(
            std::move(result),
            BuildFailure::source_forest_structure_inconsistent);
      }
      const std::size_t direct_event_index =
          saddle.journal_event_projection_index -
          source_forest.point_count;
      if (saddle_by_direct_event[direct_event_index] != missing_record) {
        return fail(
            std::move(result),
            BuildFailure::duplicate_adjacent_saddle_role);
      }
      saddle_by_direct_event[direct_event_index] = saddle_index;
    }

    std::vector<ExactDirectMorseEventRankTowerLink> pending_links;
    std::vector<ExactDirectMorseEventRankTowerArmTerminal>
        pending_arm_terminals;
    pending_links.reserve(source_forest.birth_records.size());
    pending_arm_terminals.reserve(source_forest.arm_root_bindings.size());
    std::size_t terminal_maximum_rank_birth_count = 0U;

    for (const auto& birth : source_forest.birth_records) {
      if (birth.order < 2U ||
          birth.source_journal_batch_index >= source_forest.batches.size()) {
        return fail(
            std::move(result),
            BuildFailure::source_forest_structure_inconsistent);
      }
      const auto& birth_batch = source_forest.batches[
          birth.source_journal_batch_index];
      if (birth_batch.order != birth.order ||
          birth.source_event_projection_index < source_forest.point_count ||
          birth.source_event_projection_index >=
              source_forest.source_event_projection_count) {
        return fail(
            std::move(result),
            BuildFailure::source_forest_structure_inconsistent);
      }
      const std::size_t direct_event_index =
          birth.source_event_projection_index -
          source_forest.point_count;
      if (birth_seen_by_direct_event[direct_event_index]) {
        return fail(
            std::move(result),
            BuildFailure::duplicate_adjacent_birth_role);
      }
      birth_seen_by_direct_event[direct_event_index] = true;
      const std::size_t saddle_record_index =
          saddle_by_direct_event[direct_event_index];
      if (saddle_record_index == missing_record) {
        return fail(
            std::move(result),
            BuildFailure::missing_adjacent_saddle_role);
      }
      const auto& saddle = source_forest.saddle_records[
          saddle_record_index];
      if (saddle.source_journal_batch_index >=
          source_forest.batches.size()) {
        return fail(
            std::move(result),
            BuildFailure::source_forest_structure_inconsistent);
      }
      const auto& lower_batch = source_forest.batches[
          saddle.source_journal_batch_index];
      if (lower_batch.order ==
              std::numeric_limits<std::size_t>::max() ||
          lower_batch.order + 1U != birth.order) {
        return fail(
            std::move(result), BuildFailure::adjacent_order_mismatch);
      }
      if (lower_batch.squared_level != birth_batch.squared_level) {
        return fail(
            std::move(result), BuildFailure::exact_level_mismatch);
      }
      if (saddle.atomic_group_index >=
          source_forest.atomic_groups.size()) {
        return fail(
            std::move(result), BuildFailure::atomic_group_inconsistent);
      }
      const auto& group = source_forest.atomic_groups[
          saddle.atomic_group_index];
      if (group.batch_index != saddle.source_journal_batch_index ||
          saddle_record_index < group.saddle_record_offset ||
          saddle_record_index - group.saddle_record_offset >=
              group.saddle_record_count) {
        return fail(
            std::move(result), BuildFailure::atomic_group_inconsistent);
      }
      const std::size_t arm_terminal_offset =
          pending_arm_terminals.size();
      for (std::size_t local = 0U;
           local < saddle.arm_binding_count;
           ++local) {
        const std::size_t binding_index =
            saddle.arm_binding_offset + local;
        const auto& binding =
            source_forest.arm_root_bindings[binding_index];
        pending_arm_terminals.push_back(
            {pending_arm_terminals.size(),
             binding_index,
             binding.terminal_birth_record_index,
             binding.frozen_carrier_component_handle,
             binding.prior_reduced_root_node_id,
             binding.removed_support_point_id,
             binding.terminal_birth_facet_key,
             binding.terminal_birth_binding_witness,
             binding.terminal_birth_exact_center,
             binding.terminal_birth_exact_squared_level});
      }
      pending_links.push_back(
          {pending_links.size(),
           birth.birth_record_index,
           birth.source_event_projection_index,
           birth.order,
           birth.order - 1U,
           birth_batch.squared_level,
           saddle_record_index,
           saddle.source_family_index,
           saddle.source_journal_batch_index,
           saddle.atomic_group_index,
           group.resulting_root_node_id,
           arm_terminal_offset,
           saddle.arm_binding_count,
           lower_batch.strict_pre_batch_stamp,
           lower_batch.committed_batch_stamp,
           saddle.source_event_arm_identity_digest});
      if (source_forest.effective_maximum_order ==
              source_forest.point_count &&
          birth.order == source_forest.point_count) {
        ++terminal_maximum_rank_birth_count;
      }
    }

    std::size_t logical_output_count = 0U;
    if (!try_add(
            pending_links.size(),
            pending_arm_terminals.size(),
            logical_output_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (pending_links.size() > budget.maximum_link_count ||
        pending_arm_terminals.size() >
            budget.maximum_arm_terminal_reference_count ||
        logical_output_count > budget.maximum_logical_output_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    if (!source_forest.certified_conditional_h0_candidate()) {
      return fail(
          std::move(result), BuildFailure::source_forest_rejected);
    }
    const bool terminal_maximum_rank_link_required =
        source_forest.point_count >= 2U &&
        source_forest.effective_maximum_order ==
            source_forest.point_count;
    if (terminal_maximum_rank_link_required &&
        terminal_maximum_rank_birth_count != 1U) {
      return fail(
          std::move(result),
          BuildFailure::
              terminal_maximum_rank_birth_missing_or_nonunique);
    }

    result.links = std::move(pending_links);
    result.arm_terminals = std::move(pending_arm_terminals);
    result.logical_output_entry_count = logical_output_count;
    result.counters = {
        logical_birth_count,
        source_forest.saddle_records.size(),
        source_forest.arm_root_bindings.size(),
        source_forest.atomic_groups.size(),
        source_forest.batches.size(),
        logical_node_count,
        source_forest.birth_records.size(),
        terminal_maximum_rank_birth_count,
        result.links.size(),
        result.arm_terminals.size()};
    result.source_conditional_forest_certificate_replayed = true;
    result.source_forest_structure_replayed = true;
    result.every_rank_at_least_two_birth_linked_exactly_once = true;
    result.same_source_event_projection_replayed = true;
    result.adjacent_order_role_replayed = true;
    result.exact_level_identity_replayed = true;
    result.every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth =
        true;
    result.strict_pre_batch_roots_replayed = true;
    result.atomic_post_batch_target_replayed = true;
    result.terminal_maximum_rank_link_required =
        terminal_maximum_rank_link_required;
    result.all_present_terminal_maximum_rank_births_included = true;
    result.lower_rank_links_are_composition_only = true;
    result.output_csr_and_linear_in_source_events_and_arms = true;
    result.no_partial_scientific_payload_published = true;
    result.decision = ExactDirectMorseEventRankTowerLinkDecision::
        complete_certified_conditional_event_rank_tower_links;
    return result;
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  } catch (const std::out_of_range&) {
    return fail(
        std::move(result),
        BuildFailure::source_forest_structure_inconsistent);
  } catch (const std::logic_error&) {
    return fail(
        std::move(result),
        BuildFailure::source_forest_structure_inconsistent);
  }
}

ExactDirectMorseEventRankTowerLinkVerification
verify_exact_direct_morse_event_rank_tower_link_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed) {
  ExactDirectMorseEventRankTowerLinkVerification verification;
  verification.source_forest_certified =
      source_forest.certified_conditional_h0_candidate();
  verification.observed_storage_within_budget =
      result_storage_within_budget(observed, trusted_budget);
  if (!verification.source_forest_certified ||
      !verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected =
      build_exact_direct_morse_event_rank_tower_link_journal(
          source_forest, trusted_budget);
  verification.expected_journal_freshly_rebuilt =
      expected.certified_conditional_event_rank_tower_links();
  verification.observed_structure_certified =
      observed.certified_conditional_event_rank_tower_links();
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.expected_journal_freshly_rebuilt &&
      verification.observed_structure_certified &&
      verification.observed_recursively_equal;
  return verification;
}

ExactDirectMorseEventRankTowerLinkFreshForestVerification
verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
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
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed_link) {
  ExactDirectMorseEventRankTowerLinkFreshForestVerification verification;
  const auto forest_verification = verify_exact_direct_morse_forest_journal(
      index,
      cloud,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      forest_config,
      traversal_order,
      observed_forest);
  verification.trusted_forest_inputs_certified =
      forest_verification.trusted_inputs_certified;
  verification.source_forest_storage_within_budget =
      forest_verification.observed_storage_within_budget;
  verification.source_forest_freshly_reconstructed =
      forest_verification.expected_journal_freshly_reconstructed;
  verification.source_forest_recursively_equal =
      forest_verification.observed_recursively_equal;
  verification.conditional_on_caller_fresh_phase9_facade_replay =
      forest_verification.conditional_on_caller_fresh_phase9_facade_replay;
  verification.source_forest_certified =
      forest_verification.result_certified;
  if (!verification.source_forest_certified) {
    return verification;
  }

  const auto link_verification =
      verify_exact_direct_morse_event_rank_tower_link_journal(
          observed_forest, trusted_link_budget, observed_link);
  verification.observed_link_storage_within_budget =
      link_verification.observed_storage_within_budget;
  verification.expected_link_journal_freshly_rebuilt =
      link_verification.expected_journal_freshly_rebuilt;
  verification.observed_link_structure_certified =
      link_verification.observed_structure_certified;
  verification.observed_link_recursively_equal =
      link_verification.observed_recursively_equal;
  verification.result_certified =
      link_verification.result_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
