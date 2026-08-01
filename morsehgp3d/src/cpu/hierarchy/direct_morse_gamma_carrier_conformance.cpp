#include "morsehgp3d/hierarchy/direct_morse_gamma_carrier_conformance.hpp"

#include "morsehgp3d/hierarchy/gamma_transition.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using Budget = ExactDirectMorseGammaCarrierConformanceBudget;
using CheckpointAudit = ExactDirectMorseGammaCarrierCheckpointAudit;
using Decision = ExactDirectMorseGammaCarrierConformanceDecision;
using GroupAudit = ExactDirectMorseGammaGroupAudit;
using GroupClassification = ExactDirectMorseGammaGroupClassification;
using Result = ExactDirectMorseGammaCarrierConformanceResult;
using Scope = ExactDirectMorseGammaCarrierConformanceScope;

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool add_charge(
    std::size_t& counter,
    std::size_t amount,
    std::size_t maximum) noexcept {
  std::size_t next = 0U;
  if (!checked_add(counter, amount, next) || next > maximum) {
    return false;
  }
  counter = next;
  return true;
}

[[nodiscard]] bool binomial(
    std::size_t n, std::size_t k, std::size_t& output) noexcept {
  if (k > n) {
    output = 0U;
    return true;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    const std::size_t numerator = n - k + index;
    if (value > std::numeric_limits<std::size_t>::max() / numerator) {
      return false;
    }
    value *= numerator;
    value /= index;
  }
  output = value;
  return true;
}

void clear_scientific_payload(Result& result) noexcept {
  result.exhaustive_facet_count = 0U;
  result.exhaustive_coface_count = 0U;
  result.required_direct_role_scan_capacity = 0U;
  result.required_forest_birth_scan_capacity = 0U;
  result.required_forest_saddle_scan_capacity = 0U;
  result.required_forest_atomic_group_scan_capacity = 0U;
  result.required_activation_level_capacity = 0U;
  result.required_carrier_anchor_scan_capacity = 0U;
  result.required_gamma_component_scan_capacity = 0U;
  result.required_gamma_facet_scan_capacity = 0U;
  result.required_group_event_reference_scan_capacity = 0U;
  result.required_arm_binding_scan_capacity = 0U;
  result.required_gamma_incidence_join_capacity = 0U;
  result.required_carrier_cut_entry_lookup_capacity = 0U;
  result.required_group_component_membership_capacity = 0U;
  result.required_gamma_label_comparison_capacity = 0U;
  result.required_logical_scratch_entry_count = 0U;
  result.required_checkpoint_capacity = 0U;
  result.required_group_audit_capacity = 0U;
  result.required_logical_output_entry_count = 0U;
  result.checkpoints.clear();
  result.group_audits.clear();
  result.counters = {};
  result.scalar_preflight_certified = false;
  result.source_forest_freshly_replayed_relative_to_facade = false;
  result.critical_catalog_gamma_overlay_freshly_replayed = false;
  result.direct_h0_catalog_roles_bidirectionally_reconciled = false;
  result.direct_atomic_groups_bidirectionally_reconciled = false;
  result.
      direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components =
      false;
  result.carrier_partition_bijective_at_every_strict_checkpoint = false;
  result.carrier_partition_bijective_at_every_closed_checkpoint = false;
  result.every_gamma_group_classified_once = false;
  result.every_provenance_free_gamma_group_is_silent_continuation = false;
  result.every_silent_continuation_preserves_one_root_and_adds_zero_points =
      false;
  result.bounded_carrier_faithfulness_replayed = false;
  result.bounded_silent_gamma_group_completeness_replayed = false;
  result.bounded_bidirectional_gamma_group_completeness_replayed = false;
  result.bounded_exhaustive_gamma_oracle_used = false;
  result.scope = Scope::unspecified;
}

[[nodiscard]] Result fail(Result result, Decision decision) noexcept {
  clear_scientific_payload(result);
  result.no_partial_scientific_payload_published_on_failure = true;
  result.decision = decision;
  return result;
}

[[nodiscard]] bool known_failure_decision(Decision decision) noexcept {
  switch (decision) {
    case Decision::no_conformance_capacity_overflow:
    case Decision::no_conformance_budget_exhausted:
    case Decision::no_conformance_allocation_failed:
    case Decision::no_conformance_point_count_or_order_rejected:
    case Decision::no_conformance_direct_forest_replay_rejected:
    case Decision::no_conformance_overlay_rejected:
    case Decision::no_conformance_direct_role_overlay_mismatch:
    case Decision::no_conformance_direct_group_overlay_mismatch:
    case Decision::no_conformance_gamma_residual_group_rejected:
    case Decision::no_conformance_gamma_transition_rejected:
    case Decision::no_conformance_carrier_cut_replay_rejected:
    case Decision::no_conformance_carrier_partition_mismatch:
      return true;
    case Decision::not_certified:
    case Decision::
        complete_bounded_single_order_direct_gamma_carrier_group_conformance:
      return false;
  }
  return false;
}

[[nodiscard]] bool gamma_kind_matches_prior_root_count(
    ExactReducedGammaBatchGroupKind kind,
    std::size_t prior_root_count) noexcept {
  switch (kind) {
    case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
    case ExactReducedGammaBatchGroupKind::birth:
      return prior_root_count == 0U;
    case ExactReducedGammaBatchGroupKind::continuation:
      return prior_root_count == 1U;
    case ExactReducedGammaBatchGroupKind::multifusion:
      return prior_root_count >= 2U;
  }
  return false;
}

[[nodiscard]] bool storage_within(
    const Result& result, const Budget& budget) noexcept {
  const auto& counters = result.counters;
  return result.required_direct_role_scan_capacity <=
             budget.maximum_direct_role_scan_count &&
         result.required_forest_birth_scan_capacity <=
             budget.maximum_forest_birth_scan_count &&
         result.required_forest_saddle_scan_capacity <=
             budget.maximum_forest_saddle_scan_count &&
         result.required_forest_atomic_group_scan_capacity <=
             budget.maximum_forest_atomic_group_scan_count &&
         result.required_activation_level_capacity <=
             budget.maximum_activation_level_count &&
         result.required_activation_level_capacity <=
             budget.maximum_gamma_transition_build_count &&
         result.required_checkpoint_capacity <=
             budget.maximum_checkpoint_count &&
         result.required_group_audit_capacity <=
             budget.maximum_group_audit_count &&
         result.required_carrier_anchor_scan_capacity <=
             budget.maximum_carrier_anchor_scan_count &&
         result.required_gamma_component_scan_capacity <=
             budget.maximum_gamma_component_scan_count &&
         result.required_gamma_facet_scan_capacity <=
             budget.maximum_gamma_facet_scan_count &&
         result.required_group_event_reference_scan_capacity <=
             budget.maximum_group_event_reference_scan_count &&
         result.required_arm_binding_scan_capacity <=
             budget.maximum_arm_binding_scan_count &&
         result.required_gamma_incidence_join_capacity <=
             budget.maximum_gamma_incidence_join_count &&
         result.required_carrier_cut_entry_lookup_capacity <=
             budget.maximum_carrier_cut_entry_lookup_count &&
         result.required_group_component_membership_capacity <=
             budget.maximum_group_component_membership_count &&
         result.required_gamma_label_comparison_capacity <=
             budget.maximum_gamma_label_comparison_count &&
         result.required_logical_scratch_entry_count <=
             budget.maximum_logical_scratch_entry_count &&
         result.required_logical_output_entry_count <=
             budget.maximum_logical_output_entry_count &&
         counters.direct_role_scan_count <=
             budget.maximum_direct_role_scan_count &&
         counters.forest_birth_scan_count <=
             budget.maximum_forest_birth_scan_count &&
         counters.forest_saddle_scan_count <=
             budget.maximum_forest_saddle_scan_count &&
         counters.forest_atomic_group_scan_count <=
             budget.maximum_forest_atomic_group_scan_count &&
         counters.activation_level_count <=
             budget.maximum_activation_level_count &&
         counters.gamma_transition_build_count <=
             budget.maximum_gamma_transition_build_count &&
         counters.checkpoint_count <= budget.maximum_checkpoint_count &&
         counters.group_audit_count <= budget.maximum_group_audit_count &&
         counters.carrier_anchor_scan_count <=
             budget.maximum_carrier_anchor_scan_count &&
         counters.gamma_component_scan_count <=
             budget.maximum_gamma_component_scan_count &&
         counters.gamma_facet_scan_count <=
             budget.maximum_gamma_facet_scan_count &&
         counters.group_event_reference_scan_count <=
             budget.maximum_group_event_reference_scan_count &&
         counters.arm_binding_scan_count <=
             budget.maximum_arm_binding_scan_count &&
         counters.gamma_incidence_join_count <=
             budget.maximum_gamma_incidence_join_count &&
         counters.carrier_cut_entry_lookup_count <=
             budget.maximum_carrier_cut_entry_lookup_count &&
         counters.group_component_membership_count <=
             budget.maximum_group_component_membership_count &&
         counters.gamma_label_comparison_count <=
             budget.maximum_gamma_label_comparison_count &&
         counters.maximum_logical_scratch_entry_count <=
             budget.maximum_logical_scratch_entry_count &&
         result.checkpoints.size() <= budget.maximum_checkpoint_count &&
         result.group_audits.size() <= budget.maximum_group_audit_count;
}

[[nodiscard]] bool success_payload_shape(const Result& result) noexcept {
  std::size_t facet_count = 0U;
  std::size_t coface_count = 0U;
  std::size_t activation_bound = 0U;
  std::size_t twice_activation = 0U;
  std::size_t logical_output_bound = 0U;
  std::size_t role_count = 0U;
  std::size_t anchor_bound = 0U;
  std::size_t gamma_lookup_bound = 0U;
  std::size_t group_event_bound = 0U;
  std::size_t arm_plus_activation_bound = 0U;
  std::size_t group_component_membership_bound = 0U;
  std::size_t coface_and_arm_facet_bound = 0U;
  std::size_t gamma_label_comparison_bound = 0U;
  std::size_t triple_birth_count = 0U;
  std::size_t triple_facet_count = 0U;
  std::size_t logical_scratch_bound = 0U;
  if (!binomial(result.point_count, result.order, facet_count) ||
      !binomial(result.point_count, result.order + 1U, coface_count) ||
      !checked_add(facet_count, coface_count, activation_bound) ||
      activation_bound == 0U ||
      !checked_multiply(activation_bound, 2U, twice_activation) ||
      !checked_add(twice_activation, 1U, logical_output_bound) ||
      !checked_add(
          result.counters.direct_birth_role_count,
          result.counters.direct_saddle_role_count,
          role_count) ||
      !checked_multiply(
          twice_activation,
          result.counters.direct_birth_role_count,
          anchor_bound) ||
      !checked_multiply(anchor_bound, facet_count, gamma_lookup_bound) ||
      !checked_add(
          role_count,
          result.counters.direct_saddle_role_count,
          group_event_bound) ||
      !checked_add(
          result.counters.arm_binding_scan_count,
          activation_bound,
          arm_plus_activation_bound) ||
      !checked_multiply(
          arm_plus_activation_bound,
          facet_count,
          group_component_membership_bound) ||
      !checked_add(
          coface_count,
          result.order + 1U,
          coface_and_arm_facet_bound) ||
      !checked_multiply(
          result.counters.arm_binding_scan_count,
          coface_and_arm_facet_bound,
          gamma_label_comparison_bound) ||
      !checked_multiply(
          result.counters.direct_birth_role_count,
          3U,
          triple_birth_count) ||
      !checked_multiply(facet_count, 3U, triple_facet_count) ||
      !checked_add(
          triple_birth_count,
          triple_facet_count,
          logical_scratch_bound) ||
      result.exhaustive_facet_count != facet_count ||
      result.exhaustive_coface_count != coface_count ||
      result.required_direct_role_scan_capacity != role_count ||
      result.required_forest_birth_scan_capacity !=
          result.counters.forest_birth_scan_count ||
      result.required_forest_saddle_scan_capacity <
          result.counters.forest_saddle_scan_count ||
      result.required_forest_atomic_group_scan_capacity !=
          result.counters.forest_atomic_group_scan_count ||
      result.required_activation_level_capacity != activation_bound ||
      result.required_carrier_anchor_scan_capacity != anchor_bound ||
      result.required_gamma_component_scan_capacity != gamma_lookup_bound ||
      result.required_gamma_facet_scan_capacity != gamma_lookup_bound ||
      result.required_group_event_reference_scan_capacity !=
          group_event_bound ||
      result.required_arm_binding_scan_capacity !=
          result.counters.arm_binding_scan_count ||
      result.required_gamma_incidence_join_capacity !=
          result.counters.arm_binding_scan_count ||
      result.required_carrier_cut_entry_lookup_capacity !=
          result.counters.arm_binding_scan_count ||
      result.required_group_component_membership_capacity !=
          group_component_membership_bound ||
      result.required_gamma_label_comparison_capacity !=
          gamma_label_comparison_bound ||
      result.required_logical_scratch_entry_count != logical_scratch_bound ||
      result.required_checkpoint_capacity != activation_bound ||
      result.required_group_audit_capacity != activation_bound ||
      result.required_logical_output_entry_count != logical_output_bound ||
      result.counters.preflight_count != 1U ||
      result.counters.direct_forest_verification_count != 1U ||
      result.counters.overlay_build_count != 1U ||
      result.counters.direct_role_scan_count != role_count ||
      result.counters.forest_saddle_scan_count !=
          result.counters.direct_saddle_role_count ||
      result.counters.group_event_reference_scan_count !=
          result.counters.direct_saddle_role_count ||
      result.counters.gamma_incidence_join_count !=
          result.counters.arm_binding_scan_count ||
      result.counters.carrier_cut_entry_lookup_count !=
          result.counters.arm_binding_scan_count ||
      result.counters.group_component_membership_count >
          group_component_membership_bound ||
      result.counters.gamma_label_comparison_count >
          gamma_label_comparison_bound ||
      result.counters.maximum_logical_scratch_entry_count >
          logical_scratch_bound ||
      result.checkpoints.empty() || result.group_audits.empty()) {
    return false;
  }

  std::size_t birth_group_count = 0U;
  std::size_t saddle_group_count = 0U;
  std::size_t mixed_group_count = 0U;
  std::size_t silent_group_count = 0U;
  std::size_t birth_role_count = 0U;
  std::size_t saddle_role_count = 0U;
  std::size_t arm_binding_count = 0U;
  std::size_t strict_component_count = 0U;
  for (std::size_t index = 0U; index < result.group_audits.size(); ++index) {
    const auto& audit = result.group_audits[index];
    if (!checked_add(
            birth_role_count,
            audit.direct_birth_role_count,
            birth_role_count) ||
        !checked_add(
            saddle_role_count,
            audit.direct_saddle_role_count,
            saddle_role_count) ||
        !checked_add(
            arm_binding_count,
            audit.direct_arm_binding_count,
            arm_binding_count) ||
        !checked_add(
            strict_component_count,
            audit.strict_gamma_component_count,
            strict_component_count)) {
      return false;
    }
    if (!gamma_kind_matches_prior_root_count(
            audit.gamma_kind, audit.gamma_prior_root_count) ||
        audit.coverage_delta_present !=
            (audit.gamma_kind !=
             ExactReducedGammaBatchGroupKind::deferred_isolated_facet) ||
        (!audit.coverage_delta_present &&
         (audit.delta_facet_count != 0U || audit.delta_point_count != 0U ||
          audit.fully_redundant_delta)) ||
        (audit.coverage_delta_present &&
         audit.fully_redundant_delta !=
             (audit.delta_facet_count == 0U &&
              audit.delta_point_count == 0U)) ||
        audit.delta_facet_count > result.exhaustive_facet_count ||
        audit.delta_point_count > result.point_count ||
        (index != 0U &&
         audit.squared_level < result.group_audits[index - 1U].squared_level)) {
      return false;
    }
    const bool has_residual =
        audit.residual_newly_active_facet_count != 0U ||
        audit.residual_equal_level_coface_count != 0U;
    const bool has_direct_saddle = audit.direct_saddle_role_count != 0U;
    if (has_direct_saddle) {
      std::size_t classified_component_count = 0U;
      if (!audit.direct_atomic_group_present ||
          !audit.direct_arm_strict_component_bijection ||
          audit.direct_arm_binding_count == 0U ||
          audit.strict_gamma_component_count == 0U ||
          !checked_add(
              audit.latent_strict_gamma_component_count,
              audit.resolved_strict_gamma_component_count,
              classified_component_count) ||
          classified_component_count !=
              audit.strict_gamma_component_count ||
          audit.resolved_strict_gamma_component_count !=
              audit.gamma_prior_root_count) {
        return false;
      }
    } else if (audit.direct_arm_binding_count != 0U ||
               audit.strict_gamma_component_count != 0U ||
               audit.latent_strict_gamma_component_count != 0U ||
               audit.resolved_strict_gamma_component_count != 0U ||
               audit.direct_arm_strict_component_bijection) {
      return false;
    }
    switch (audit.classification) {
      case GroupClassification::direct_birth_carrier:
        if (audit.direct_birth_role_count == 0U ||
            audit.direct_saddle_role_count != 0U || has_residual ||
            audit.gamma_kind !=
                ExactReducedGammaBatchGroupKind::deferred_isolated_facet ||
            audit.direct_atomic_group_present ||
            audit.direct_prior_root_count.has_value() ||
            audit.coverage_delta_present ||
            audit.silent_one_root_zero_point_continuation) {
          return false;
        }
        ++birth_group_count;
        break;
      case GroupClassification::direct_saddle_group:
        if (audit.direct_birth_role_count != 0U ||
            audit.direct_saddle_role_count == 0U || has_residual ||
            audit.gamma_kind ==
                ExactReducedGammaBatchGroupKind::deferred_isolated_facet ||
            !audit.direct_atomic_group_present ||
            audit.direct_prior_root_count !=
                std::optional<std::size_t>{audit.gamma_prior_root_count} ||
            audit.silent_one_root_zero_point_continuation) {
          return false;
        }
        ++saddle_group_count;
        break;
      case GroupClassification::mixed_direct_and_residual:
        if ((audit.direct_birth_role_count == 0U &&
             audit.direct_saddle_role_count == 0U) ||
            (audit.direct_birth_role_count != 0U &&
             audit.direct_saddle_role_count != 0U) ||
            !has_residual || audit.silent_one_root_zero_point_continuation) {
          return false;
        }
        if (audit.direct_saddle_role_count != 0U) {
          if (!audit.direct_atomic_group_present ||
              audit.direct_prior_root_count !=
                  std::optional<std::size_t>{audit.gamma_prior_root_count}) {
            return false;
          }
        } else if (audit.direct_atomic_group_present ||
                   audit.direct_prior_root_count.has_value()) {
          return false;
        }
        ++mixed_group_count;
        break;
      case GroupClassification::silent_residual_continuation:
        if (audit.direct_birth_role_count != 0U ||
            audit.direct_saddle_role_count != 0U || !has_residual ||
            audit.gamma_kind !=
                ExactReducedGammaBatchGroupKind::continuation ||
            audit.gamma_prior_root_count != 1U ||
            !audit.coverage_delta_present ||
            audit.direct_atomic_group_present ||
            audit.direct_prior_root_count.has_value() ||
            audit.delta_point_count != 0U ||
            !audit.silent_one_root_zero_point_continuation) {
          return false;
        }
        ++silent_group_count;
        break;
      default:
        return false;
    }
  }

  std::size_t silent_checkpoint_count = 0U;
  std::size_t silent_group_count_from_checkpoints = 0U;
  std::size_t gamma_group_count_from_checkpoints = 0U;
  std::size_t group_cursor = 0U;
  for (std::size_t index = 0U; index < result.checkpoints.size(); ++index) {
    const auto& checkpoint = result.checkpoints[index];
    std::size_t expected_group_count = 0U;
    std::size_t expected_silent_group_count = 0U;
    bool expected_direct_batch_present = false;
    while (group_cursor < result.group_audits.size() &&
           result.group_audits[group_cursor].squared_level ==
               checkpoint.squared_level) {
      const auto& group = result.group_audits[group_cursor];
      if (!checked_add(expected_group_count, 1U, expected_group_count)) {
        return false;
      }
      if (group.classification ==
          GroupClassification::silent_residual_continuation) {
        if (!checked_add(
                expected_silent_group_count,
                1U,
                expected_silent_group_count)) {
          return false;
        }
      } else {
        expected_direct_batch_present = true;
      }
      ++group_cursor;
    }
    if (checkpoint.activation_level_index != index ||
        !checkpoint.strict_partition_bijective ||
        !checkpoint.closed_partition_bijective ||
        checkpoint.strict_direct_carrier_count !=
            checkpoint.strict_gamma_component_count ||
        checkpoint.closed_direct_carrier_count !=
            checkpoint.closed_gamma_component_count ||
        checkpoint.strict_birth_anchor_count <
            checkpoint.strict_direct_carrier_count ||
        checkpoint.closed_birth_anchor_count <
            checkpoint.closed_direct_carrier_count ||
        checkpoint.silent_gamma_group_count >
            checkpoint.gamma_group_count ||
        checkpoint.gamma_group_count != expected_group_count ||
        checkpoint.silent_gamma_group_count !=
            expected_silent_group_count ||
        checkpoint.direct_batch_present != expected_direct_batch_present ||
        (index != 0U &&
         !(result.checkpoints[index - 1U].squared_level <
           checkpoint.squared_level)) ||
        !checked_add(
            silent_group_count_from_checkpoints,
            checkpoint.silent_gamma_group_count,
            silent_group_count_from_checkpoints) ||
        !checked_add(
            gamma_group_count_from_checkpoints,
            checkpoint.gamma_group_count,
            gamma_group_count_from_checkpoints)) {
      return false;
    }
    if (checkpoint.silent_gamma_group_count != 0U) {
      ++silent_checkpoint_count;
    }
  }

  std::size_t minimum_group_component_membership_count = 0U;
  if (!checked_add(
          arm_binding_count,
          strict_component_count,
          minimum_group_component_membership_count)) {
    return false;
  }
  return birth_group_count == result.counters.direct_birth_group_count &&
         saddle_group_count == result.counters.direct_saddle_group_count &&
         mixed_group_count ==
             result.counters.mixed_direct_residual_group_count &&
         silent_group_count ==
             result.counters.silent_residual_continuation_group_count &&
         silent_group_count == silent_group_count_from_checkpoints &&
         gamma_group_count_from_checkpoints == result.group_audits.size() &&
         group_cursor == result.group_audits.size() &&
         silent_checkpoint_count == result.counters.silent_checkpoint_count &&
         birth_role_count == result.counters.direct_birth_role_count &&
         saddle_role_count == result.counters.direct_saddle_role_count &&
         arm_binding_count == result.counters.arm_binding_scan_count &&
         result.counters.group_component_membership_count >=
             minimum_group_component_membership_count &&
         result.counters.group_audit_count == result.group_audits.size() &&
         result.counters.group_audit_count ==
             birth_group_count + saddle_group_count + mixed_group_count +
                 silent_group_count;
}

[[nodiscard]] std::vector<spatial::PointId> used_key(
    const ExactDirectSparseFacetKey& key) {
  if (key.point_count > key.point_ids.size()) {
    throw std::logic_error("an invalid direct facet key escaped replay");
  }
  return std::vector<spatial::PointId>{
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count)};
}

[[nodiscard]] std::vector<spatial::PointId> used_support(
    const ExactDirectSupportEvent& event) {
  if (event.support_size > event.support_ids.size()) {
    throw std::logic_error("an invalid direct support escaped replay");
  }
  return std::vector<spatial::PointId>{
      event.support_ids.begin(),
      event.support_ids.begin() +
          static_cast<std::ptrdiff_t>(event.support_size)};
}

[[nodiscard]] bool event_matches(
    const ExactDirectSupportEvent& direct,
    const ExactCriticalEvent& exhaustive) {
  return used_support(direct) == exhaustive.support_point_ids &&
         direct.interior_ids == exhaustive.interior_point_ids &&
         direct.center == exhaustive.center &&
         direct.squared_level == exhaustive.squared_level &&
         direct.closed_rank == exhaustive.closed_rank &&
         direct.birth_order == exhaustive.birth_order &&
         direct.saddle_order == exhaustive.saddle_order;
}

[[nodiscard]] bool role_matches(
    ExactDirectMorseH0Role direct,
    ExactCriticalCatalogReducedGammaEventRole exhaustive) noexcept {
  return (direct == ExactDirectMorseH0Role::birth &&
          exhaustive == ExactCriticalCatalogReducedGammaEventRole::birth) ||
         (direct == ExactDirectMorseH0Role::saddle &&
          exhaustive == ExactCriticalCatalogReducedGammaEventRole::saddle);
}

[[nodiscard]] bool group_kind_matches(
    ExactDirectMorseForestAtomicGroupKind direct,
    ExactReducedGammaBatchGroupKind exhaustive) noexcept {
  switch (direct) {
    case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
      return exhaustive == ExactReducedGammaBatchGroupKind::birth;
    case ExactDirectMorseForestAtomicGroupKind::continuation:
      return exhaustive == ExactReducedGammaBatchGroupKind::continuation;
    case ExactDirectMorseForestAtomicGroupKind::multifusion:
      return exhaustive == ExactReducedGammaBatchGroupKind::multifusion;
  }
  return false;
}

struct CarrierIdentity {
  bool reduced_root{false};
  std::uint64_t value{};

  friend bool operator==(const CarrierIdentity&, const CarrierIdentity&) =
      default;
};

struct PartitionAudit {
  std::size_t direct_carrier_count{};
  std::size_t gamma_component_count{};
  std::size_t birth_anchor_count{};
  std::size_t logical_scratch_entry_count{};
  std::vector<std::optional<std::size_t>> entry_component_indices;
  std::vector<std::optional<CarrierIdentity>> component_identities;
  bool bijective{false};
};

[[nodiscard]] bool component_contains_facet(
    const ExactStrictGammaComponentWitness& component,
    const std::vector<spatial::PointId>& facet) {
  return std::binary_search(
      component.facet_point_ids.begin(), component.facet_point_ids.end(),
      facet);
}

[[nodiscard]] PartitionAudit audit_partition(
    const std::optional<ExactDirectMorseForestCarrierCutIndexResult>& cut,
    const std::vector<ExactStrictGammaComponentWitness>& gamma_components,
    const ExactDirectMorseForestJournalView& forest_view,
    std::size_t order,
    const Budget& budget,
    ExactDirectMorseGammaCarrierConformanceCounters& counters) {
  PartitionAudit audit;
  audit.gamma_component_count = gamma_components.size();
  if (!cut.has_value()) {
    audit.bijective = gamma_components.empty();
    return audit;
  }

  std::size_t three_entry_count = 0U;
  std::size_t two_component_count = 0U;
  std::size_t scratch_entry_count = 0U;
  if (!checked_multiply(
          cut->entries.size(), 3U, three_entry_count) ||
      !checked_multiply(
          gamma_components.size(), 2U, two_component_count) ||
      !checked_add(
          three_entry_count,
          two_component_count,
          scratch_entry_count) ||
      scratch_entry_count > budget.maximum_logical_scratch_entry_count) {
    throw std::length_error("carrier partition scratch budget exhausted");
  }
  counters.maximum_logical_scratch_entry_count = std::max(
      counters.maximum_logical_scratch_entry_count, scratch_entry_count);
  audit.logical_scratch_entry_count = scratch_entry_count;

  std::vector<CarrierIdentity> identities;
  std::vector<std::size_t> identity_components;
  audit.entry_component_indices.resize(cut->entries.size());
  audit.component_identities.resize(gamma_components.size());
  std::vector<std::size_t> component_anchor_counts(
      gamma_components.size(), 0U);

  for (const auto& entry : cut->entries) {
    if (entry.disposition ==
        ExactDirectMorseForestCarrierCutDisposition::inactive_at_closed_cut) {
      continue;
    }
    if (!add_charge(
            counters.carrier_anchor_scan_count,
            1U,
            budget.maximum_carrier_anchor_scan_count)) {
      throw std::length_error("carrier anchor budget exhausted");
    }
    const auto birth = forest_view.birth_record_at(entry.birth_record_index);
    if (birth.birth_record_index != entry.birth_record_index ||
        birth.order != order || birth.facet_key.point_count != order) {
      throw std::logic_error("a carrier anchor is inconsistent with its birth");
    }
    const std::vector<spatial::PointId> facet = used_key(birth.facet_key);
    std::optional<std::size_t> component_index;
    for (std::size_t index = 0U; index < gamma_components.size(); ++index) {
      if (!add_charge(
              counters.gamma_component_scan_count,
              1U,
              budget.maximum_gamma_component_scan_count) ||
          !add_charge(
              counters.gamma_facet_scan_count,
              gamma_components[index].facet_point_ids.size(),
              budget.maximum_gamma_facet_scan_count)) {
        throw std::length_error("Gamma carrier lookup budget exhausted");
      }
      if (component_contains_facet(gamma_components[index], facet)) {
        if (component_index.has_value()) {
          throw std::logic_error(
              "one exhaustive Gamma facet occurs in multiple components");
        }
        component_index = index;
      }
    }
    if (!component_index.has_value()) {
      return audit;
    }
    if (entry.entry_index >= audit.entry_component_indices.size() ||
        audit.entry_component_indices[entry.entry_index].has_value()) {
      return audit;
    }
    audit.entry_component_indices[entry.entry_index] = *component_index;

    CarrierIdentity identity;
    const bool gamma_component_is_isolated =
        gamma_components[*component_index].facet_point_ids.size() == 1U;
    if (entry.disposition ==
        ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root) {
      if (!entry.reduced_root_node_id.has_value() ||
          gamma_component_is_isolated) {
        return audit;
      }
      identity = CarrierIdentity{true, *entry.reduced_root_node_id};
    } else if (
        entry.disposition ==
        ExactDirectMorseForestCarrierCutDisposition::
            active_latent_without_reduced_root) {
      if (entry.reduced_root_node_id.has_value() ||
          !gamma_component_is_isolated) {
        return audit;
      }
      identity = CarrierIdentity{
          false, static_cast<std::uint64_t>(entry.component_handle)};
    } else {
      return audit;
    }

    ++audit.birth_anchor_count;
    ++component_anchor_counts[*component_index];
    const auto found = std::find(identities.begin(), identities.end(), identity);
    if (found == identities.end()) {
      identities.push_back(identity);
      identity_components.push_back(*component_index);
    } else {
      const std::size_t position =
          static_cast<std::size_t>(found - identities.begin());
      if (identity_components[position] != *component_index) {
        return audit;
      }
    }
    if (!audit.component_identities[*component_index].has_value()) {
      audit.component_identities[*component_index] = identity;
    } else if (*audit.component_identities[*component_index] != identity) {
      return audit;
    }
  }

  if (std::any_of(
          component_anchor_counts.begin(),
          component_anchor_counts.end(),
          [](std::size_t count) { return count == 0U; })) {
    return audit;
  }
  audit.direct_carrier_count = identities.size();
  audit.bijective =
      audit.direct_carrier_count == audit.gamma_component_count &&
      identities.size() == audit.component_identities.size();
  return audit;
}

[[nodiscard]] bool transition_complete(
    const ExactGammaTransitionResult& transition) noexcept {
  return transition.decision == ExactGammaTransitionDecision::
                                    complete_exhaustive_open_to_closed_transition &&
         transition.strict_open_cut_fresh_replay_certified &&
         transition.equal_level_catalog_exhaustive_certified &&
         transition.equal_level_incidences_tokenized &&
         transition.closed_cut_exhaustive_certified &&
         transition.strict_partition_refines_closed_partition &&
         transition.equal_level_batch_applied_simultaneously &&
         transition.transition_groups_partition_equal_level_changes;
}

[[nodiscard]] bool audit_direct_atomic_group_arm_carriers(
    const ExactGammaTransitionResult& transition,
    std::size_t history_group_index,
    const ExactPersistentReducedGammaOrderHistory& history,
    const ExactCriticalCatalogReducedGammaOverlayResult& overlay,
    const ExactCriticalCatalogResult& catalog,
    const std::vector<std::optional<std::size_t>>&
        facade_event_to_overlay_projection,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t direct_atomic_group_index,
    const std::optional<ExactDirectMorseForestCarrierCutIndexResult>&
        strict_cut,
    const PartitionAudit& strict_partition,
    const Budget& budget,
    ExactDirectMorseGammaCarrierConformanceCounters& counters,
    GroupAudit& audit) {
  if (history_group_index >= history.group_records.size() ||
      direct_atomic_group_index >= source_forest.atomic_groups.size() ||
      !strict_cut.has_value()) {
    return false;
  }
  const auto& history_group = history.group_records[history_group_index];
  if (history_group.batch_group_index >= transition.transition_groups.size()) {
    return false;
  }
  const auto& transition_group =
      transition.transition_groups[history_group.batch_group_index];
  const auto& direct_group =
      source_forest.atomic_groups[direct_atomic_group_index];
  if (direct_group.saddle_record_offset >
          source_forest.saddle_records.size() ||
      direct_group.saddle_record_count >
          source_forest.saddle_records.size() -
              direct_group.saddle_record_offset ||
      direct_group.saddle_record_count == 0U) {
    return false;
  }

  std::size_t simultaneous_scratch_entry_count = 0U;
  if (!checked_add(
          strict_partition.logical_scratch_entry_count,
          transition_group.strict_component_indices.size(),
          simultaneous_scratch_entry_count) ||
      simultaneous_scratch_entry_count >
          budget.maximum_logical_scratch_entry_count) {
    throw std::length_error("arm--Gamma join scratch budget exhausted");
  }
  counters.maximum_logical_scratch_entry_count = std::max(
      counters.maximum_logical_scratch_entry_count,
      simultaneous_scratch_entry_count);
  std::vector<bool> strict_component_seen(
      transition_group.strict_component_indices.size(), false);

  std::size_t unique_component_count = 0U;
  std::size_t latent_component_count = 0U;
  std::size_t resolved_component_count = 0U;
  std::size_t arm_binding_count = 0U;
  for (std::size_t local_saddle = 0U;
       local_saddle < direct_group.saddle_record_count;
       ++local_saddle) {
    const std::size_t saddle_index =
        direct_group.saddle_record_offset + local_saddle;
    const auto& saddle = source_forest.saddle_records[saddle_index];
    if (saddle.saddle_record_index != saddle_index ||
        saddle.atomic_group_index != direct_atomic_group_index ||
        saddle.source_event_index >=
            facade_event_to_overlay_projection.size() ||
        !facade_event_to_overlay_projection[saddle.source_event_index]
             .has_value() ||
        saddle.source_family_index >= source_seed_journal.families.size() ||
        saddle.arm_binding_offset > source_forest.arm_root_bindings.size() ||
        saddle.arm_binding_count >
            source_forest.arm_root_bindings.size() -
                saddle.arm_binding_offset ||
        saddle.arm_binding_count == 0U) {
      return false;
    }
    const std::size_t projection_index =
        *facade_event_to_overlay_projection[saddle.source_event_index];
    if (projection_index >= overlay.event_projections.size()) {
      return false;
    }
    const auto& projection = overlay.event_projections[projection_index];
    if (projection.projection_index != projection_index ||
        projection.role !=
            ExactCriticalCatalogReducedGammaEventRole::saddle ||
        projection.history_group_record_index != history_group_index ||
        projection.history_label_kind !=
            ExactCriticalCatalogReducedGammaHistoryLabelKind::
                equal_level_coface ||
        projection.history_group_local_label_index >=
            history_group.equal_level_coface_point_ids.size() ||
        projection.history_label_slot_index >=
            overlay.history_label_slots.size() ||
        projection.catalog_event_index >= catalog.events.size()) {
      return false;
    }
    const auto& slot =
        overlay.history_label_slots[projection.history_label_slot_index];
    if (slot.event_projection_index !=
            std::optional<std::size_t>{projection_index} ||
        slot.history_group_record_index != history_group_index ||
        slot.kind != projection.history_label_kind ||
        slot.history_group_local_label_index !=
            projection.history_group_local_label_index) {
      return false;
    }
    const auto& coface = history_group.equal_level_coface_point_ids[
        projection.history_group_local_label_index];
    if (catalog.events[projection.catalog_event_index].closed_point_ids !=
        coface) {
      return false;
    }

    std::optional<std::size_t> coface_index;
    for (std::size_t candidate_index = 0U;
         candidate_index < transition.equal_level_cofaces.size();
         ++candidate_index) {
      if (!add_charge(
              counters.gamma_label_comparison_count,
              1U,
              budget.maximum_gamma_label_comparison_count)) {
        throw std::length_error("Gamma label comparison budget exhausted");
      }
      if (transition.equal_level_cofaces[candidate_index]
              .coface_point_ids == coface) {
        if (coface_index.has_value()) {
          return false;
        }
        coface_index = candidate_index;
      }
    }
    if (!coface_index.has_value()) {
      return false;
    }
    const auto& coface_witness =
        transition.equal_level_cofaces[*coface_index];
    if (coface_witness.facet_point_ids.size() != transition.order + 1U) {
      return false;
    }

    for (std::size_t local_binding = 0U;
         local_binding < saddle.arm_binding_count;
         ++local_binding) {
      const std::size_t binding_index =
          saddle.arm_binding_offset + local_binding;
      const auto& binding = source_forest.arm_root_bindings[binding_index];
      if (!add_charge(
              counters.arm_binding_scan_count,
              1U,
              budget.maximum_arm_binding_scan_count) ||
          !add_charge(
              counters.carrier_cut_entry_lookup_count,
              1U,
              budget.maximum_carrier_cut_entry_lookup_count)) {
        throw std::length_error("direct arm carrier budget exhausted");
      }
      if (binding.binding_index != binding_index ||
          binding.source_family_index != saddle.source_family_index ||
          binding.source_arm_seed_index >=
              source_seed_journal.arm_seeds.size() ||
          source_seed_journal.arm_seeds[binding.source_arm_seed_index]
                  .family_index != saddle.source_family_index ||
          binding.strict_arm_key.point_count != transition.order) {
        return false;
      }
      const std::vector<spatial::PointId> facet =
          used_key(binding.strict_arm_key);
      std::optional<std::size_t> facet_index;
      for (std::size_t candidate_index = 0U;
           candidate_index < coface_witness.facet_point_ids.size();
           ++candidate_index) {
        if (!add_charge(
                counters.gamma_label_comparison_count,
                1U,
                budget.maximum_gamma_label_comparison_count)) {
          throw std::length_error("Gamma label comparison budget exhausted");
        }
        if (coface_witness.facet_point_ids[candidate_index] == facet) {
          if (facet_index.has_value()) {
            return false;
          }
          facet_index = candidate_index;
        }
      }
      if (!facet_index.has_value()) {
        return false;
      }
      std::size_t incidence_offset = 0U;
      std::size_t incidence_index = 0U;
      if (!checked_multiply(
              *coface_index, transition.order + 1U, incidence_offset) ||
          !checked_add(incidence_offset, *facet_index, incidence_index) ||
          incidence_index >= transition.equal_level_incidences.size()) {
        return false;
      }
      const auto& incidence =
          transition.equal_level_incidences[incidence_index];
      if (!add_charge(
              counters.gamma_incidence_join_count,
              1U,
              budget.maximum_gamma_incidence_join_count)) {
        throw std::length_error("arm--Gamma incidence budget exhausted");
      }
      if (incidence.coface_point_ids != coface ||
          incidence.facet_point_ids != facet ||
          incidence.newly_active_at_level ||
          !incidence.strict_component_index.has_value()) {
        return false;
      }
      std::optional<std::size_t> marker_index;
      for (std::size_t candidate_index = 0U;
           candidate_index <
               transition_group.strict_component_indices.size();
           ++candidate_index) {
        if (!add_charge(
                counters.group_component_membership_count,
                1U,
                budget.maximum_group_component_membership_count)) {
          throw std::length_error(
              "group component membership budget exhausted");
        }
        if (transition_group.strict_component_indices[candidate_index] ==
            *incidence.strict_component_index) {
          if (marker_index.has_value()) {
            return false;
          }
          marker_index = candidate_index;
        }
      }
      if (!marker_index.has_value()) {
        return false;
      }

      const auto* entry = strict_cut->find_entry(
          binding.frozen_carrier_component_handle);
      if (entry == nullptr ||
          entry->disposition ==
              ExactDirectMorseForestCarrierCutDisposition::
                  inactive_at_closed_cut ||
          entry->reduced_root_node_id !=
              binding.prior_reduced_root_node_id ||
          entry->entry_index >=
              strict_partition.entry_component_indices.size() ||
          strict_partition.entry_component_indices[entry->entry_index] !=
              incidence.strict_component_index) {
        return false;
      }
      if (!strict_component_seen[*marker_index]) {
        strict_component_seen[*marker_index] = true;
        ++unique_component_count;
        if (entry->disposition ==
            ExactDirectMorseForestCarrierCutDisposition::
                resolved_reduced_root) {
          ++resolved_component_count;
        } else if (
            entry->disposition ==
            ExactDirectMorseForestCarrierCutDisposition::
                active_latent_without_reduced_root) {
          ++latent_component_count;
        } else {
          return false;
        }
      }
      ++arm_binding_count;
    }
  }

  for (const bool seen : strict_component_seen) {
    if (!add_charge(
            counters.group_component_membership_count,
            1U,
            budget.maximum_group_component_membership_count)) {
      throw std::length_error("group component membership budget exhausted");
    }
    if (!seen) {
      return false;
    }
  }
  if (arm_binding_count == 0U ||
      unique_component_count != direct_group.frozen_carrier_count ||
      unique_component_count !=
          transition_group.strict_component_indices.size() ||
      latent_component_count != direct_group.latent_carrier_count ||
      resolved_component_count != direct_group.prior_reduced_root_count ||
      latent_component_count + resolved_component_count !=
          unique_component_count) {
    return false;
  }
  audit.direct_arm_binding_count = arm_binding_count;
  audit.strict_gamma_component_count = unique_component_count;
  audit.latent_strict_gamma_component_count = latent_component_count;
  audit.resolved_strict_gamma_component_count = resolved_component_count;
  audit.direct_arm_strict_component_bijection = true;
  return true;
}

[[nodiscard]] Result compute_conformance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const Budget& budget) {
  Result result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.no_partial_scientific_payload_published_on_failure = true;
  ++result.counters.preflight_count;

  if (cloud.size() < 3U || cloud.size() > 14U || order < 2U ||
      order >= cloud.size() || order > 10U ||
      source_forest.point_count != cloud.size() ||
      source_forest.effective_maximum_order < order) {
    return fail(
        std::move(result),
        Decision::no_conformance_point_count_or_order_rejected);
  }

  std::size_t facet_count = 0U;
  std::size_t coface_count = 0U;
  std::size_t activation_bound = 0U;
  std::size_t twice_activation = 0U;
  std::size_t logical_output_bound = 0U;

  if (!binomial(cloud.size(), order, facet_count) ||
      !binomial(cloud.size(), order + 1U, coface_count) ||
      !checked_add(facet_count, coface_count, activation_bound) ||
      !checked_multiply(activation_bound, 2U, twice_activation) ||
      !checked_add(twice_activation, 1U, logical_output_bound)) {
    return fail(
        std::move(result), Decision::no_conformance_capacity_overflow);
  }
  result.exhaustive_facet_count = facet_count;
  result.exhaustive_coface_count = coface_count;
  result.required_activation_level_capacity = activation_bound;
  result.required_checkpoint_capacity = activation_bound;
  result.required_group_audit_capacity = activation_bound;
  result.required_logical_output_entry_count = logical_output_bound;

  if (!storage_within(result, budget)) {
    return fail(
        std::move(result), Decision::no_conformance_budget_exhausted);
  }
  try {
    const auto forest_verification = verify_exact_direct_morse_forest_journal(
        index,
        cloud,
        source_facade,
        source_event_journal,
        trusted_seed_budget,
        source_seed_journal,
        trusted_forest_budget,
        trusted_forest_config,
        traversal_order,
        source_forest);
    ++result.counters.direct_forest_verification_count;
    if (!forest_verification.result_certified ||
        !forest_verification.expected_journal_freshly_reconstructed ||
        !forest_verification.observed_recursively_equal ||
        !source_forest.certified_conditional_h0_candidate()) {
      return fail(
          std::move(result),
          Decision::no_conformance_direct_forest_replay_rejected);
    }
    result.source_forest_freshly_replayed_relative_to_facade = true;

    const ExactDirectMorseForestJournalView forest_view{source_forest};
    std::size_t target_role_count = 0U;
    std::size_t target_birth_count = 0U;
    std::size_t target_saddle_count = 0U;
    std::size_t target_arm_binding_count = 0U;
    for (const auto& batch : source_event_journal.batches) {
      if (batch.order == order &&
          !checked_add(
              target_role_count,
              batch.role_record_count,
              target_role_count)) {
        return fail(
            std::move(result), Decision::no_conformance_capacity_overflow);
      }
    }
    const std::size_t forest_birth_count = forest_view.birth_record_count();
    for (std::size_t birth_index = 0U;
         birth_index < forest_birth_count; ++birth_index) {
      if (forest_view.birth_record_at(birth_index).order == order &&
          !checked_add(target_birth_count, 1U, target_birth_count)) {
        return fail(
            std::move(result), Decision::no_conformance_capacity_overflow);
      }
    }
    for (const auto& saddle : source_forest.saddle_records) {
      const auto& group = source_forest.atomic_groups[saddle.atomic_group_index];
      if (source_forest.batches[group.batch_index].order == order) {
        if (!checked_add(target_saddle_count, 1U, target_saddle_count) ||
            !checked_add(
                target_arm_binding_count,
                saddle.arm_binding_count,
                target_arm_binding_count)) {
          return fail(
              std::move(result), Decision::no_conformance_capacity_overflow);
        }
      }
    }

    std::size_t anchor_bound = 0U;
    std::size_t gamma_lookup_bound = 0U;
    std::size_t group_event_bound = 0U;
    std::size_t arm_plus_activation_bound = 0U;
    std::size_t group_component_membership_bound = 0U;
    std::size_t coface_and_arm_facet_bound = 0U;
    std::size_t gamma_label_comparison_bound = 0U;
    std::size_t triple_birth_count = 0U;
    std::size_t triple_facet_count = 0U;
    std::size_t logical_scratch_bound = 0U;
    if (!checked_multiply(
            twice_activation, target_birth_count, anchor_bound) ||
        !checked_multiply(
            anchor_bound, facet_count, gamma_lookup_bound) ||
        !checked_add(
            target_role_count, target_saddle_count, group_event_bound) ||
        !checked_add(
            target_arm_binding_count,
            activation_bound,
            arm_plus_activation_bound) ||
        !checked_multiply(
            arm_plus_activation_bound,
            facet_count,
            group_component_membership_bound) ||
        !checked_add(
            coface_count, order + 1U, coface_and_arm_facet_bound) ||
        !checked_multiply(
            target_arm_binding_count,
            coface_and_arm_facet_bound,
            gamma_label_comparison_bound) ||
        !checked_multiply(target_birth_count, 3U, triple_birth_count) ||
        !checked_multiply(facet_count, 3U, triple_facet_count) ||
        !checked_add(
            triple_birth_count,
            triple_facet_count,
            logical_scratch_bound)) {
      return fail(
          std::move(result), Decision::no_conformance_capacity_overflow);
    }
    result.required_direct_role_scan_capacity = target_role_count;
    result.required_forest_birth_scan_capacity = forest_birth_count;
    result.required_forest_saddle_scan_capacity =
        source_forest.saddle_records.size();
    result.required_forest_atomic_group_scan_capacity =
        source_forest.atomic_groups.size();
    result.required_carrier_anchor_scan_capacity = anchor_bound;
    result.required_gamma_component_scan_capacity = gamma_lookup_bound;
    result.required_gamma_facet_scan_capacity = gamma_lookup_bound;
    result.required_group_event_reference_scan_capacity = group_event_bound;
    result.required_arm_binding_scan_capacity = target_arm_binding_count;
    result.required_gamma_incidence_join_capacity = target_arm_binding_count;
    result.required_carrier_cut_entry_lookup_capacity =
        target_arm_binding_count;
    result.required_group_component_membership_capacity =
        group_component_membership_bound;
    result.required_gamma_label_comparison_capacity =
        gamma_label_comparison_bound;
    result.required_logical_scratch_entry_count = logical_scratch_bound;
    if (!storage_within(result, budget)) {
      return fail(
          std::move(result), Decision::no_conformance_budget_exhausted);
    }
    result.scalar_preflight_certified = true;

    const auto overlay = build_exact_critical_catalog_reduced_gamma_overlay(
        cloud, order, budget.overlay_budget);
    ++result.counters.overlay_build_count;
    if (overlay.decision !=
            ExactCriticalCatalogReducedGammaOverlayDecision::
                complete_exhaustive_critical_catalog_reduced_gamma_overlay ||
        !overlay.critical_catalog_reduced_gamma_overlay_certified ||
        !overlay.critical_catalog_fresh_replay_certified ||
        !overlay.reduced_gamma_history_fresh_replay_certified ||
        !overlay.critical_catalog.has_value() ||
        !overlay.reduced_gamma_history.has_value()) {
      return fail(
          std::move(result), Decision::no_conformance_overlay_rejected);
    }
    result.critical_catalog_gamma_overlay_freshly_replayed = true;
    const auto& catalog = *overlay.critical_catalog;
    const auto& history = *overlay.reduced_gamma_history;

    if (history.activation_levels.size() > activation_bound ||
        history.group_records.size() > activation_bound ||
        history.activation_levels.size() > budget.maximum_activation_level_count ||
        history.group_records.size() > budget.maximum_group_audit_count) {
      return fail(
          std::move(result), Decision::no_conformance_budget_exhausted);
    }
    result.counters.activation_level_count = history.activation_levels.size();

    const ExactDirectMorseEventJournalView event_view{source_event_journal};
    std::vector<std::optional<std::size_t>>
        facade_event_to_overlay_projection(source_facade.events.size());
    std::vector<bool> overlay_projection_matched(
        overlay.event_projections.size(), false);

    for (const auto& batch : source_event_journal.batches) {
      if (batch.order != order) {
        continue;
      }
      if (batch.role_record_offset > event_view.role_record_count() ||
          batch.role_record_count >
              event_view.role_record_count() - batch.role_record_offset) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_role_overlay_mismatch);
      }
      for (std::size_t local = 0U; local < batch.role_record_count; ++local) {
        const auto role =
            event_view.role_record_at(batch.role_record_offset + local);
        if (!add_charge(
                result.counters.direct_role_scan_count,
                1U,
                budget.maximum_direct_role_scan_count) ||
            role.batch_index != batch.batch_index) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_role_overlay_mismatch);
        }
        const auto projection =
            event_view.event_projection_at(role.event_projection_index);
        if (projection.source !=
                ExactDirectMorseEventSource::direct_support_terminal_event ||
            projection.source_index >= source_facade.events.size()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_role_overlay_mismatch);
        }
        const auto& direct_event = source_facade.events[projection.source_index];
        std::optional<std::size_t> match;
        for (const auto& candidate : overlay.event_projections) {
          if (!role_matches(role.role, candidate.role) ||
              candidate.catalog_event_index >= catalog.events.size() ||
              overlay_projection_matched[candidate.projection_index]) {
            continue;
          }
          if (event_matches(
                  direct_event, catalog.events[candidate.catalog_event_index])) {
            if (match.has_value()) {
              return fail(
                  std::move(result),
                  Decision::no_conformance_direct_role_overlay_mismatch);
            }
            match = candidate.projection_index;
          }
        }
        if (!match.has_value() || *match >= overlay_projection_matched.size()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_role_overlay_mismatch);
        }
        overlay_projection_matched[*match] = true;
        if (facade_event_to_overlay_projection[projection.source_index]
                .has_value() &&
            *facade_event_to_overlay_projection[projection.source_index] !=
                *match) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_role_overlay_mismatch);
        }
        facade_event_to_overlay_projection[projection.source_index] = *match;
        if (role.role == ExactDirectMorseH0Role::birth) {
          ++result.counters.direct_birth_role_count;
        } else {
          ++result.counters.direct_saddle_role_count;
        }
      }
    }
    if (result.counters.direct_role_scan_count !=
            overlay.event_projections.size() ||
        std::any_of(
            overlay_projection_matched.begin(),
            overlay_projection_matched.end(),
            [](bool matched) { return !matched; })) {
      return fail(
          std::move(result),
          Decision::no_conformance_direct_role_overlay_mismatch);
    }

    std::vector<bool> overlay_birth_has_forest_record(
        overlay.event_projections.size(), false);
    for (std::size_t birth_index = 0U;
         birth_index < forest_view.birth_record_count(); ++birth_index) {
      const auto birth = forest_view.birth_record_at(birth_index);
      ++result.counters.forest_birth_scan_count;
      if (birth.order != order) {
        continue;
      }
      const auto projection =
          event_view.event_projection_at(birth.source_event_projection_index);
      if (projection.source !=
              ExactDirectMorseEventSource::direct_support_terminal_event ||
          projection.source_index >=
              facade_event_to_overlay_projection.size() ||
          !facade_event_to_overlay_projection[projection.source_index]
               .has_value()) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_role_overlay_mismatch);
      }
      const std::size_t overlay_index =
          *facade_event_to_overlay_projection[projection.source_index];
      if (overlay_index >= overlay.event_projections.size() ||
          overlay.event_projections[overlay_index].role !=
              ExactCriticalCatalogReducedGammaEventRole::birth ||
          overlay_birth_has_forest_record[overlay_index]) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_role_overlay_mismatch);
      }
      overlay_birth_has_forest_record[overlay_index] = true;
    }
    for (const auto& projection : overlay.event_projections) {
      if (projection.role ==
              ExactCriticalCatalogReducedGammaEventRole::birth &&
          !overlay_birth_has_forest_record[projection.projection_index]) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_role_overlay_mismatch);
      }
    }
    if (result.counters.forest_birth_scan_count >
        budget.maximum_forest_birth_scan_count) {
      return fail(
          std::move(result), Decision::no_conformance_budget_exhausted);
    }
    result.direct_h0_catalog_roles_bidirectionally_reconciled = true;

    std::vector<std::optional<std::size_t>>
        history_group_to_direct_atomic(history.group_records.size());
    std::vector<bool> overlay_saddle_has_forest_record(
        overlay.event_projections.size(), false);

    for (const auto& group : source_forest.atomic_groups) {
      ++result.counters.forest_atomic_group_scan_count;
      if (group.batch_index >= source_forest.batches.size()) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
      const auto& batch = source_forest.batches[group.batch_index];
      if (batch.order != order) {
        continue;
      }
      if (group.saddle_record_offset > source_forest.saddle_records.size() ||
          group.saddle_record_count >
              source_forest.saddle_records.size() -
                  group.saddle_record_offset ||
          group.saddle_record_count == 0U) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
      std::optional<std::size_t> history_group_index;
      std::vector<std::size_t> mapped_projections;
      mapped_projections.reserve(group.saddle_record_count);
      for (std::size_t local = 0U; local < group.saddle_record_count; ++local) {
        const auto& saddle = source_forest.saddle_records[
            group.saddle_record_offset + local];
        ++result.counters.forest_saddle_scan_count;
        if (saddle.atomic_group_index != group.atomic_group_index ||
            saddle.source_event_index >=
                facade_event_to_overlay_projection.size() ||
            !facade_event_to_overlay_projection[saddle.source_event_index]
                 .has_value()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        const std::size_t projection_index =
            *facade_event_to_overlay_projection[saddle.source_event_index];
        if (projection_index >= overlay.event_projections.size()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        const auto& projection = overlay.event_projections[projection_index];
        if (projection.role !=
                ExactCriticalCatalogReducedGammaEventRole::saddle ||
            projection.history_group_record_index >=
                history.group_records.size()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        if (history_group_index.has_value() &&
            *history_group_index != projection.history_group_record_index) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        history_group_index = projection.history_group_record_index;
        mapped_projections.push_back(projection_index);
        overlay_saddle_has_forest_record[projection_index] = true;
        if (!add_charge(
                result.counters.group_event_reference_scan_count,
                1U,
                budget.maximum_group_event_reference_scan_count)) {
          return fail(
              std::move(result), Decision::no_conformance_budget_exhausted);
        }
      }
      if (!history_group_index.has_value() ||
          history_group_to_direct_atomic[*history_group_index].has_value()) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
      const auto& history_group = history.group_records[*history_group_index];
      const auto& group_overlay = overlay.group_overlays[*history_group_index];
      std::sort(mapped_projections.begin(), mapped_projections.end());
      std::vector<std::size_t> expected_projections =
          group_overlay.saddle_event_projection_indices;
      std::sort(expected_projections.begin(), expected_projections.end());
      if (mapped_projections != expected_projections ||
          !group_kind_matches(group.kind, history_group.kind) ||
          group.prior_reduced_root_count !=
              history_group.prior_root_node_ids.size()) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
      history_group_to_direct_atomic[*history_group_index] =
          group.atomic_group_index;
    }
    if (result.counters.forest_atomic_group_scan_count >
            budget.maximum_forest_atomic_group_scan_count ||
        result.counters.forest_saddle_scan_count >
            budget.maximum_forest_saddle_scan_count) {
      return fail(
          std::move(result), Decision::no_conformance_budget_exhausted);
    }
    for (const auto& projection : overlay.event_projections) {
      if (projection.role ==
              ExactCriticalCatalogReducedGammaEventRole::saddle &&
          !overlay_saddle_has_forest_record[projection.projection_index]) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
    }
    result.direct_atomic_groups_bidirectionally_reconciled = true;

    result.group_audits.reserve(history.group_records.size());
    for (std::size_t group_index = 0U;
         group_index < history.group_records.size(); ++group_index) {
      const auto& history_group = history.group_records[group_index];
      if (group_index >= overlay.group_overlays.size()) {
        return fail(
            std::move(result), Decision::no_conformance_overlay_rejected);
      }
      const auto& group_overlay = overlay.group_overlays[group_index];
      if (group_overlay.history_group_record_index != group_index ||
          group_overlay.first_label_slot_index >
              overlay.history_label_slots.size() ||
          group_overlay.label_slot_count >
              overlay.history_label_slots.size() -
                  group_overlay.first_label_slot_index) {
        return fail(
            std::move(result), Decision::no_conformance_overlay_rejected);
      }
      GroupAudit audit;
      audit.squared_level = history_group.squared_level;
      audit.gamma_kind = history_group.kind;
      audit.direct_birth_role_count =
          group_overlay.birth_event_projection_indices.size();
      audit.direct_saddle_role_count =
          group_overlay.saddle_event_projection_indices.size();
      audit.gamma_prior_root_count =
          history_group.prior_root_node_ids.size();
      for (std::size_t local = 0U; local < group_overlay.label_slot_count;
           ++local) {
        const auto& slot = overlay.history_label_slots[
            group_overlay.first_label_slot_index + local];
        if (!slot.event_projection_index.has_value()) {
          if (slot.kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet) {
            ++audit.residual_newly_active_facet_count;
          } else {
            ++audit.residual_equal_level_coface_count;
          }
        }
      }
      if (history_group.coverage_delta.has_value()) {
        audit.coverage_delta_present = true;
        audit.delta_facet_count =
            history_group.coverage_delta->added_facet_point_ids.size();
        audit.delta_point_count =
            history_group.coverage_delta->added_point_ids.size();
        audit.fully_redundant_delta =
            history_group.coverage_delta->fully_redundant;
      }

      const bool has_direct = audit.direct_birth_role_count != 0U ||
                              audit.direct_saddle_role_count != 0U;
      const bool has_residual =
          audit.residual_newly_active_facet_count != 0U ||
          audit.residual_equal_level_coface_count != 0U;
      if (audit.direct_birth_role_count != 0U &&
          audit.direct_saddle_role_count != 0U) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }
      if (audit.direct_saddle_role_count != 0U) {
        if (!history_group_to_direct_atomic[group_index].has_value()) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        const auto& direct_group = source_forest.atomic_groups[
            *history_group_to_direct_atomic[group_index]];
        audit.direct_atomic_group_present = true;
        audit.direct_prior_root_count = direct_group.prior_reduced_root_count;
      } else if (history_group_to_direct_atomic[group_index].has_value()) {
        return fail(
            std::move(result),
            Decision::no_conformance_direct_group_overlay_mismatch);
      }

      if (!has_direct) {
        const bool silent =
            has_residual &&
            history_group.kind ==
                ExactReducedGammaBatchGroupKind::continuation &&
            history_group.prior_root_node_ids.size() == 1U &&
            history_group.resulting_root_node_id ==
                std::optional<std::size_t>{
                    history_group.prior_root_node_ids.front()} &&
            !history_group.created_node_id.has_value() &&
            history_group.coverage_delta.has_value() &&
            history_group.coverage_delta->added_point_ids.empty();
        if (!silent) {
          return fail(
              std::move(result),
              Decision::no_conformance_gamma_residual_group_rejected);
        }
        audit.classification =
            GroupClassification::silent_residual_continuation;
        audit.silent_one_root_zero_point_continuation = true;
        ++result.counters.silent_residual_continuation_group_count;
      } else if (has_residual) {
        audit.classification = GroupClassification::mixed_direct_and_residual;
        ++result.counters.mixed_direct_residual_group_count;
      } else if (audit.direct_birth_role_count != 0U) {
        if (history_group.kind !=
                ExactReducedGammaBatchGroupKind::deferred_isolated_facet ||
            audit.direct_atomic_group_present) {
          return fail(
              std::move(result),
              Decision::no_conformance_direct_group_overlay_mismatch);
        }
        audit.classification = GroupClassification::direct_birth_carrier;
        ++result.counters.direct_birth_group_count;
      } else {
        audit.classification = GroupClassification::direct_saddle_group;
        ++result.counters.direct_saddle_group_count;
      }
      result.group_audits.push_back(std::move(audit));
    }
    result.counters.group_audit_count = result.group_audits.size();
    if (result.group_audits.size() != history.group_records.size()) {
      return fail(
          std::move(result), Decision::no_conformance_overlay_rejected);
    }
    result.every_gamma_group_classified_once = true;
    result.every_provenance_free_gamma_group_is_silent_continuation = true;
    result.every_silent_continuation_preserves_one_root_and_adds_zero_points =
        true;

    for (const auto& batch : source_forest.batches) {
      if (batch.order == order &&
          !std::binary_search(
              history.activation_levels.begin(),
              history.activation_levels.end(),
              batch.squared_level)) {
        return fail(
            std::move(result),
            Decision::no_conformance_carrier_partition_mismatch);
      }
    }

    std::optional<ExactDirectMorseForestCarrierCutIndexResult> previous_cut;
    result.checkpoints.reserve(history.activation_levels.size());
    for (std::size_t activation_index = 0U;
         activation_index < history.activation_levels.size();
         ++activation_index) {
      const auto& level = history.activation_levels[activation_index];
      const auto transition = build_exact_gamma_equal_level_transition(
          cloud,
          order,
          level,
          budget.overlay_budget.reduced_gamma_history_budget.gamma_budget);
      ++result.counters.gamma_transition_build_count;
      if (!transition_complete(transition)) {
        return fail(
            std::move(result),
            Decision::no_conformance_gamma_transition_rejected);
      }

      if (activation_index >= history.batch_metadata.size()) {
        return fail(
            std::move(result), Decision::no_conformance_overlay_rejected);
      }
      const auto& metadata = history.batch_metadata[activation_index];
      if (metadata.batch_index != activation_index ||
          metadata.activation_level_index != activation_index ||
          metadata.squared_level != level ||
          metadata.first_group_record_index > history.group_records.size() ||
          metadata.group_record_count >
              history.group_records.size() -
                  metadata.first_group_record_index ||
          metadata.group_record_count != transition.transition_groups.size()) {
        return fail(
            std::move(result), Decision::no_conformance_overlay_rejected);
      }

      std::size_t strict_direct_carrier_count = 0U;
      std::size_t strict_gamma_component_count = 0U;
      std::size_t strict_birth_anchor_count = 0U;
      {
        const PartitionAudit strict = audit_partition(
            previous_cut,
            transition.strict_gamma.components,
            forest_view,
            order,
            budget,
            result.counters);
        if (!strict.bijective) {
          return fail(
              std::move(result),
              Decision::no_conformance_carrier_partition_mismatch);
        }
        strict_direct_carrier_count = strict.direct_carrier_count;
        strict_gamma_component_count = strict.gamma_component_count;
        strict_birth_anchor_count = strict.birth_anchor_count;

        for (std::size_t local_group = 0U;
             local_group < metadata.group_record_count;
             ++local_group) {
          const std::size_t history_group_index =
              metadata.first_group_record_index + local_group;
          const auto& history_group =
              history.group_records[history_group_index];
          if (history_group.group_record_index != history_group_index ||
              history_group.batch_index != activation_index ||
              history_group.batch_group_index >=
                  transition.transition_groups.size() ||
              history_group.squared_level != level) {
            return fail(
                std::move(result), Decision::no_conformance_overlay_rejected);
          }
          const auto& transition_group = transition.transition_groups[
              history_group.batch_group_index];
          if (transition_group.canonical_representative_facet_point_ids !=
                  history_group.canonical_representative_facet_point_ids ||
              transition_group.newly_active_facet_point_ids !=
                  history_group.newly_active_facet_point_ids ||
              transition_group.equal_level_coface_point_ids !=
                  history_group.equal_level_coface_point_ids) {
            return fail(
                std::move(result),
                Decision::no_conformance_gamma_transition_rejected);
          }
          if (history_group_index >= result.group_audits.size()) {
            return fail(
                std::move(result), Decision::no_conformance_overlay_rejected);
          }
          if (history_group_to_direct_atomic[history_group_index].has_value() &&
              !audit_direct_atomic_group_arm_carriers(
                  transition,
                  history_group_index,
                  history,
                  overlay,
                  catalog,
                  facade_event_to_overlay_projection,
                  source_seed_journal,
                  source_forest,
                  *history_group_to_direct_atomic[history_group_index],
                  previous_cut,
                  strict,
                  budget,
                  result.counters,
                  result.group_audits[history_group_index])) {
            return fail(
                std::move(result),
                Decision::no_conformance_carrier_partition_mismatch);
          }
        }
      }

      auto current_cut = build_exact_direct_morse_forest_carrier_cut_index(
          source_forest, order, level, budget.carrier_cut_budget);
      ++result.counters.carrier_cut_build_count;
      if (!current_cut.certified_forest_relative_closed_cut_index()) {
        return fail(
            std::move(result),
            Decision::no_conformance_carrier_cut_replay_rejected);
      }
      std::optional<ExactDirectMorseForestCarrierCutIndexResult>
          current_cut_view{std::move(current_cut)};
      const PartitionAudit closed = audit_partition(
          current_cut_view,
          transition.closed_components,
          forest_view,
          order,
          budget,
          result.counters);
      if (!closed.bijective) {
        return fail(
            std::move(result),
            Decision::no_conformance_carrier_partition_mismatch);
      }

      CheckpointAudit checkpoint;
      checkpoint.activation_level_index = activation_index;
      checkpoint.squared_level = level;
      checkpoint.direct_batch_present = std::any_of(
          source_forest.batches.begin(),
          source_forest.batches.end(),
          [&](const ExactDirectMorseForestBatch& batch) {
            return batch.order == order && batch.squared_level == level;
          });
      checkpoint.strict_direct_carrier_count = strict_direct_carrier_count;
      checkpoint.strict_gamma_component_count = strict_gamma_component_count;
      checkpoint.strict_birth_anchor_count = strict_birth_anchor_count;
      checkpoint.closed_direct_carrier_count = closed.direct_carrier_count;
      checkpoint.closed_gamma_component_count = closed.gamma_component_count;
      checkpoint.closed_birth_anchor_count = closed.birth_anchor_count;
      checkpoint.strict_partition_bijective = true;
      checkpoint.closed_partition_bijective = true;
      checkpoint.gamma_group_count = metadata.group_record_count;
      for (std::size_t local_group = 0U;
           local_group < metadata.group_record_count;
           ++local_group) {
        const auto& audit = result.group_audits[
            metadata.first_group_record_index + local_group];
        if (audit.classification ==
            GroupClassification::silent_residual_continuation) {
          ++checkpoint.silent_gamma_group_count;
        }
      }
      if (checkpoint.silent_gamma_group_count != 0U) {
        ++result.counters.silent_checkpoint_count;
      }
      ++result.counters.strict_partition_bijection_count;
      ++result.counters.closed_partition_bijection_count;
      result.checkpoints.push_back(std::move(checkpoint));
      previous_cut = std::move(current_cut_view);
    }
    result.counters.checkpoint_count = result.checkpoints.size();
    if (result.counters.gamma_transition_build_count !=
            history.activation_levels.size() ||
        result.counters.carrier_cut_build_count !=
            history.activation_levels.size() ||
        result.counters.strict_partition_bijection_count !=
            history.activation_levels.size() ||
        result.counters.closed_partition_bijection_count !=
            history.activation_levels.size() ||
        result.counters.arm_binding_scan_count !=
            result.required_arm_binding_scan_capacity ||
        result.counters.gamma_incidence_join_count !=
            result.required_gamma_incidence_join_capacity ||
        result.counters.carrier_cut_entry_lookup_count !=
            result.required_carrier_cut_entry_lookup_capacity ||
        !storage_within(result, budget)) {
      return fail(
          std::move(result), Decision::no_conformance_budget_exhausted);
    }

    result.carrier_partition_bijective_at_every_strict_checkpoint = true;
    result.carrier_partition_bijective_at_every_closed_checkpoint = true;
    result.
        direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components =
        true;
    result.bounded_carrier_faithfulness_replayed = true;
    result.bounded_silent_gamma_group_completeness_replayed = true;
    result.bounded_bidirectional_gamma_group_completeness_replayed = true;
    result.bounded_exhaustive_gamma_oracle_used = true;
    result.scope = Scope::
        bounded_n14_single_order_direct_forest_carriers_and_h0_groups_against_fresh_exhaustive_gamma_only;
    result.decision = Decision::
        complete_bounded_single_order_direct_gamma_carrier_group_conformance;
    if (!result.certified_bounded_conformance()) {
      throw std::logic_error(
          "a complete direct--Gamma conformance receipt failed its contract");
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result), Decision::no_conformance_allocation_failed);
  } catch (const std::length_error&) {
    return fail(
        std::move(result), Decision::no_conformance_budget_exhausted);
  } catch (const std::logic_error&) {
    return fail(
        std::move(result),
        Decision::no_conformance_carrier_partition_mismatch);
  } catch (const std::exception&) {
    return fail(
        std::move(result),
        Decision::no_conformance_carrier_partition_mismatch);
  }
}

}  // namespace

bool ExactDirectMorseGammaCarrierConformanceResult::
    certified_bounded_conformance() const noexcept {
  return schema_version ==
             direct_morse_gamma_carrier_conformance_schema_version &&
         point_count >= 3U && point_count <= 14U && order >= 2U &&
         order < point_count && order <= 10U && scalar_preflight_certified &&
         source_forest_freshly_replayed_relative_to_facade &&
         critical_catalog_gamma_overlay_freshly_replayed &&
         direct_h0_catalog_roles_bidirectionally_reconciled &&
         direct_atomic_groups_bidirectionally_reconciled &&
         direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components &&
         carrier_partition_bijective_at_every_strict_checkpoint &&
         carrier_partition_bijective_at_every_closed_checkpoint &&
         every_gamma_group_classified_once &&
         every_provenance_free_gamma_group_is_silent_continuation &&
         every_silent_continuation_preserves_one_root_and_adds_zero_points &&
         bounded_carrier_faithfulness_replayed &&
         bounded_silent_gamma_group_completeness_replayed &&
         bounded_bidirectional_gamma_group_completeness_replayed &&
         bounded_exhaustive_gamma_oracle_used &&
         !product_sparse_silent_source_complete &&
         !global_morse_obligation_replayed && !global_m1_claimed &&
         !all_naturality_squares_replayed && !vertical_maps_complete &&
         !forest_semantics_exact && !scalable_50k_claimed &&
         !gamma_cells_or_global_cofaces_persisted &&
         !higher_order_delaunay_materialized && !public_status_claimed &&
         no_partial_scientific_payload_published_on_failure &&
         checkpoints.size() == counters.checkpoint_count &&
         group_audits.size() == counters.group_audit_count &&
         counters.activation_level_count == counters.checkpoint_count &&
         counters.gamma_transition_build_count == counters.checkpoint_count &&
         counters.carrier_cut_build_count == counters.checkpoint_count &&
         counters.strict_partition_bijection_count ==
             counters.checkpoint_count &&
         counters.closed_partition_bijection_count ==
             counters.checkpoint_count &&
         success_payload_shape(*this) &&
         storage_within(*this, requested_budget) &&
         decision == Decision::
             complete_bounded_single_order_direct_gamma_carrier_group_conformance &&
         scope == Scope::
             bounded_n14_single_order_direct_forest_carriers_and_h0_groups_against_fresh_exhaustive_gamma_only;
}

bool ExactDirectMorseGammaCarrierConformanceResult::certified_atomic_failure()
    const noexcept {
  return schema_version ==
             direct_morse_gamma_carrier_conformance_schema_version &&
         known_failure_decision(decision) && checkpoints.empty() &&
         group_audits.empty() && exhaustive_facet_count == 0U &&
         exhaustive_coface_count == 0U &&
         required_direct_role_scan_capacity == 0U &&
         required_forest_birth_scan_capacity == 0U &&
         required_forest_saddle_scan_capacity == 0U &&
         required_forest_atomic_group_scan_capacity == 0U &&
         required_activation_level_capacity == 0U &&
         required_carrier_anchor_scan_capacity == 0U &&
         required_gamma_component_scan_capacity == 0U &&
         required_gamma_facet_scan_capacity == 0U &&
         required_group_event_reference_scan_capacity == 0U &&
         required_arm_binding_scan_capacity == 0U &&
         required_gamma_incidence_join_capacity == 0U &&
         required_carrier_cut_entry_lookup_capacity == 0U &&
         required_group_component_membership_capacity == 0U &&
         required_gamma_label_comparison_capacity == 0U &&
         required_logical_scratch_entry_count == 0U &&
         required_checkpoint_capacity == 0U &&
         required_group_audit_capacity == 0U &&
         required_logical_output_entry_count == 0U &&
         counters == ExactDirectMorseGammaCarrierConformanceCounters{} &&
         !scalar_preflight_certified &&
         !source_forest_freshly_replayed_relative_to_facade &&
         !critical_catalog_gamma_overlay_freshly_replayed &&
         !direct_h0_catalog_roles_bidirectionally_reconciled &&
         !direct_atomic_groups_bidirectionally_reconciled &&
         !direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components &&
         !carrier_partition_bijective_at_every_strict_checkpoint &&
         !carrier_partition_bijective_at_every_closed_checkpoint &&
         !every_gamma_group_classified_once &&
         !every_provenance_free_gamma_group_is_silent_continuation &&
         !every_silent_continuation_preserves_one_root_and_adds_zero_points &&
         !bounded_carrier_faithfulness_replayed &&
         !bounded_silent_gamma_group_completeness_replayed &&
         !bounded_bidirectional_gamma_group_completeness_replayed &&
         !bounded_exhaustive_gamma_oracle_used &&
         !product_sparse_silent_source_complete &&
         !global_morse_obligation_replayed && !global_m1_claimed &&
         !all_naturality_squares_replayed && !vertical_maps_complete &&
         !forest_semantics_exact && !scalable_50k_claimed &&
         !gamma_cells_or_global_cofaces_persisted &&
         !higher_order_delaunay_materialized && !public_status_claimed &&
         no_partial_scientific_payload_published_on_failure &&
         scope == Scope::unspecified;
}

bool ExactDirectMorseGammaCarrierConformanceResult::certified_outcome()
    const noexcept {
  return certified_bounded_conformance() || certified_atomic_failure();
}

ExactDirectMorseGammaCarrierConformanceResult
build_exact_direct_morse_gamma_carrier_conformance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseGammaCarrierConformanceBudget& budget) {
  const Result result = compute_conformance(
      index,
      cloud,
      order,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest,
      budget);
  const auto verification = verify_exact_direct_morse_gamma_carrier_conformance(
      index,
      cloud,
      order,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest,
      budget,
      result);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the bounded direct--Gamma carrier conformance failed its fresh "
        "verifier");
  }
  return result;
}

ExactDirectMorseGammaCarrierConformanceVerification
verify_exact_direct_morse_gamma_carrier_conformance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseGammaCarrierConformanceBudget& trusted_budget,
    const ExactDirectMorseGammaCarrierConformanceResult& observed) {
  ExactDirectMorseGammaCarrierConformanceVerification verification;
  verification.observed_storage_within_budget =
      observed.requested_budget == trusted_budget &&
      storage_within(observed, trusted_budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const Result expected = compute_conformance(
      index,
      cloud,
      order,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest,
      trusted_budget);
  verification.source_forest_freshly_replayed =
      expected.source_forest_freshly_replayed_relative_to_facade;
  verification.expected_result_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = expected == observed;
  verification.result_certified =
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal && observed.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
