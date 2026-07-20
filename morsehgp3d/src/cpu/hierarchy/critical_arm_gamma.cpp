#include "morsehgp3d/hierarchy/critical_arm_gamma.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;

void validate_strict_gamma_budget(
    const ExactStrictGammaBudget& budget) {
  if (budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::
              maximum_supported_union_attempt_count) {
    throw std::invalid_argument(
        "an exact critical-arm Gamma budget exceeds its reference cap");
  }
}

[[nodiscard]] std::vector<FacetLabel> terminal_class_sources(
    const ExactCriticalArmFamilyResult& family) {
  std::vector<FacetLabel> sources;
  sources.reserve(family.terminal_label_classes.size());
  for (const ExactCriticalArmTerminalLabelClass& terminal_class :
       family.terminal_label_classes) {
    sources.push_back(
        terminal_class.canonical_terminal.facet_point_ids);
  }
  return sources;
}

[[nodiscard]] bool projection_partition_is_consistent(
    const ExactCriticalArmGammaResult& result) {
  if (!result.strict_gamma.has_value() ||
      result.terminal_class_classifications.size() !=
          result.arm_family.terminal_label_classes.size() ||
      result.arm_classifications.size() !=
          result.arm_family.arms.size() ||
      result.arm_family.terminal_label_classes.size() >
          result.arm_family.arms.size()) {
    return false;
  }

  using ClassAndComponent = std::pair<std::size_t, std::size_t>;
  std::map<PointId, ClassAndComponent> provenance_by_removed_point;
  std::map<std::size_t, ExactCriticalArmGammaIncidentComponent>
      expected_incident_by_component;
  for (std::size_t class_index = 0U;
       class_index < result.terminal_class_classifications.size();
       ++class_index) {
    const ExactCriticalArmTerminalLabelClass& terminal_class =
        result.arm_family.terminal_label_classes[class_index];
    const ExactCriticalArmGammaTerminalClassClassification& mapping =
        result.terminal_class_classifications[class_index];
    const ExactStrictGammaSourceClassification& source =
        result.strict_gamma->source_classifications[class_index];
    if (mapping.terminal_label_class_index != class_index ||
        mapping.terminal_facet_point_ids !=
            terminal_class.canonical_terminal.facet_point_ids ||
        mapping.removed_shell_point_ids !=
            terminal_class.removed_shell_point_ids ||
        mapping.removed_shell_point_ids.empty() ||
        !std::is_sorted(
            mapping.removed_shell_point_ids.begin(),
            mapping.removed_shell_point_ids.end()) ||
        std::adjacent_find(
            mapping.removed_shell_point_ids.begin(),
            mapping.removed_shell_point_ids.end()) !=
            mapping.removed_shell_point_ids.end() ||
        source.source_facet_point_ids !=
            mapping.terminal_facet_point_ids ||
        !source.component_index.has_value() ||
        *source.component_index !=
            mapping.strict_gamma_component_index ||
        mapping.strict_gamma_component_index >=
            result.strict_gamma->components.size()) {
      return false;
    }

    auto [position, inserted] =
        expected_incident_by_component.try_emplace(
            mapping.strict_gamma_component_index);
    ExactCriticalArmGammaIncidentComponent& incident = position->second;
    if (inserted) {
      incident.strict_gamma_component_index =
          mapping.strict_gamma_component_index;
      incident.canonical_representative_facet_point_ids =
          result.strict_gamma
              ->components[mapping.strict_gamma_component_index]
              .canonical_representative_facet_point_ids;
    }
    incident.terminal_label_class_indices.push_back(class_index);
    incident.removed_shell_point_ids.insert(
        incident.removed_shell_point_ids.end(),
        mapping.removed_shell_point_ids.begin(),
        mapping.removed_shell_point_ids.end());

    for (PointId removed_point_id :
         mapping.removed_shell_point_ids) {
      if (!provenance_by_removed_point
               .emplace(
                   removed_point_id,
                   ClassAndComponent{
                       class_index,
                       mapping.strict_gamma_component_index})
               .second) {
        return false;
      }
    }
  }
  if (provenance_by_removed_point.size() !=
      result.arm_family.arms.size()) {
    return false;
  }

  for (std::size_t arm_index = 0U;
       arm_index < result.arm_classifications.size();
       ++arm_index) {
    const ExactCriticalArmFamilyArmResult& family_arm =
        result.arm_family.arms[arm_index];
    const ExactCriticalArmGammaArmClassification& mapping =
        result.arm_classifications[arm_index];
    const auto expected = provenance_by_removed_point.find(
        family_arm.removed_shell_point_id);
    if (!family_arm.terminal_label_class_index.has_value() ||
        expected == provenance_by_removed_point.end() ||
        mapping.removed_shell_point_id !=
            family_arm.removed_shell_point_id ||
        mapping.terminal_label_class_index !=
            *family_arm.terminal_label_class_index ||
        mapping.terminal_label_class_index !=
            expected->second.first ||
        mapping.strict_gamma_component_index !=
            expected->second.second) {
      return false;
    }
  }

  std::vector<ExactCriticalArmGammaIncidentComponent>
      expected_incident_components;
  expected_incident_components.reserve(
      expected_incident_by_component.size());
  for (auto& [component_index, incident] :
       expected_incident_by_component) {
    static_cast<void>(component_index);
    std::sort(
        incident.removed_shell_point_ids.begin(),
        incident.removed_shell_point_ids.end());
    expected_incident_components.push_back(std::move(incident));
  }
  return expected_incident_components == result.incident_components;
}

[[nodiscard]] ExactCriticalArmGammaResult
compute_exact_critical_arm_gamma_component_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    ExactStrictGammaBudget strict_gamma_budget) {
  if (cloud.size() < 2U ||
      cloud.size() >
          ExactStrictGammaBudget::maximum_supported_point_count) {
    throw std::invalid_argument(
        "the exact critical-arm Gamma bridge requires 2<=n<=14");
  }
  validate_strict_gamma_budget(strict_gamma_budget);

  ExactCriticalArmGammaResult result;
  result.requested_per_arm_chain_budget = per_arm_chain_budget;
  result.requested_strict_gamma_budget = strict_gamma_budget;
  result.scope = ExactCriticalArmGammaScope::
      bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only;
  result.arm_family = build_exact_critical_arm_family_descent(
      cloud,
      critical_shell_point_ids,
      per_arm_chain_budget);
  result.critical_shell_point_ids =
      result.arm_family.critical_shell_point_ids;
  result.counters.arm_family_build_count = 1U;
  result.counters.terminal_label_class_count =
      result.arm_family.terminal_label_classes.size();
  result.arm_family_fresh_replay_certified = true;

  if (result.arm_family.decision ==
      ExactCriticalArmFamilyDecision::
          no_family_unsupported_critical_source) {
    result.decision = ExactCriticalArmGammaDecision::
        no_classification_unsupported_critical_source;
    return result;
  }
  if (result.arm_family.decision !=
          ExactCriticalArmFamilyDecision::
              all_arms_complete_at_regular_active_facets ||
      !result.arm_family.
          complete_terminal_label_partition_certified) {
    result.decision = ExactCriticalArmGammaDecision::
        no_classification_incomplete_arm_family;
    return result;
  }
  if (result.arm_family.arms.empty() ||
      result.arm_family.terminal_label_classes.empty() ||
      result.arm_family.terminal_label_classes.size() >
          ExactStrictGammaBudget::
              maximum_supported_source_facet_count) {
    throw std::logic_error(
        "a complete critical-arm family has an invalid terminal partition");
  }

  const ExactCriticalArmInitialSegmentResult& shared_source =
      result.arm_family.arms.front().descent.initial_segment;
  result.order = shared_source.order;
  result.critical_squared_level =
      shared_source.critical_shell_miniball.squared_radius;
  if (result.order == 0U || result.order >= cloud.size()) {
    throw std::logic_error(
        "a complete critical-arm family derived an invalid Gamma order");
  }
  result.critical_order_and_level_derived_from_shared_source = true;

  const std::vector<FacetLabel> sources =
      terminal_class_sources(result.arm_family);
  result.strict_gamma =
      build_exact_strict_gamma_source_classification(
          cloud,
          result.order,
          result.critical_squared_level,
          sources,
          strict_gamma_budget);
  result.counters.strict_gamma_build_count = 1U;
  result.counters.strict_gamma_source_facet_count = sources.size();

  if (result.strict_gamma->decision ==
      ExactStrictGammaDecision::
          no_cut_preflight_budget_insufficient) {
    result.decision = ExactCriticalArmGammaDecision::
        no_classification_strict_gamma_preflight_budget_insufficient;
    return result;
  }
  if (result.strict_gamma->decision !=
          ExactStrictGammaDecision::
              complete_all_sources_active_and_classified ||
      !result.strict_gamma->strict_open_cut_certified ||
      !result.strict_gamma->exhaustive_active_catalog_certified ||
      !result.strict_gamma->complete_source_classification_certified ||
      !result.strict_gamma->all_sources_active_and_classified ||
      result.strict_gamma->source_classifications.size() !=
          result.arm_family.terminal_label_classes.size()) {
    throw std::logic_error(
        "a complete strict arm family was not active in its exact pre-event Gamma cut");
  }
  result.strict_gamma_cut_fresh_replay_certified = true;

  std::map<std::size_t, ExactCriticalArmGammaIncidentComponent>
      incident_by_component;
  result.terminal_class_classifications.reserve(
      result.arm_family.terminal_label_classes.size());
  for (std::size_t class_index = 0U;
       class_index < result.arm_family.terminal_label_classes.size();
       ++class_index) {
    const ExactCriticalArmTerminalLabelClass& terminal_class =
        result.arm_family.terminal_label_classes[class_index];
    const ExactStrictGammaSourceClassification& classification =
        result.strict_gamma->source_classifications[class_index];
    if (classification.source_facet_point_ids !=
            terminal_class.canonical_terminal.facet_point_ids ||
        !classification.active_strictly_below_cut ||
        !classification.component_index.has_value() ||
        classification.squared_level !=
            terminal_class.canonical_terminal.squared_level ||
        *classification.component_index >=
            result.strict_gamma->components.size()) {
      throw std::logic_error(
          "a terminal class has an inconsistent strict-Gamma classification");
    }

    ExactCriticalArmGammaTerminalClassClassification terminal_mapping;
    terminal_mapping.terminal_label_class_index = class_index;
    terminal_mapping.terminal_facet_point_ids =
        terminal_class.canonical_terminal.facet_point_ids;
    terminal_mapping.removed_shell_point_ids =
        terminal_class.removed_shell_point_ids;
    terminal_mapping.strict_gamma_component_index =
        *classification.component_index;
    result.terminal_class_classifications.push_back(
        terminal_mapping);
    ++result.counters.terminal_class_component_projection_count;

    auto [position, inserted] = incident_by_component.try_emplace(
        terminal_mapping.strict_gamma_component_index);
    ExactCriticalArmGammaIncidentComponent& incident =
        position->second;
    if (inserted) {
      incident.strict_gamma_component_index =
          terminal_mapping.strict_gamma_component_index;
      incident.canonical_representative_facet_point_ids =
          result.strict_gamma
              ->components[terminal_mapping.strict_gamma_component_index]
              .canonical_representative_facet_point_ids;
    }
    incident.terminal_label_class_indices.push_back(class_index);
    incident.removed_shell_point_ids.insert(
        incident.removed_shell_point_ids.end(),
        terminal_mapping.removed_shell_point_ids.begin(),
        terminal_mapping.removed_shell_point_ids.end());
  }

  result.arm_classifications.reserve(result.arm_family.arms.size());
  for (const ExactCriticalArmFamilyArmResult& arm :
       result.arm_family.arms) {
    if (!arm.terminal_label_class_index.has_value() ||
        *arm.terminal_label_class_index >=
            result.terminal_class_classifications.size()) {
      throw std::logic_error(
          "a complete critical arm has no terminal-class projection");
    }
    const std::size_t class_index =
        *arm.terminal_label_class_index;
    result.arm_classifications.push_back(
        ExactCriticalArmGammaArmClassification{
            arm.removed_shell_point_id,
            class_index,
            result.terminal_class_classifications[class_index]
                .strict_gamma_component_index});
    ++result.counters.arm_component_projection_count;
  }

  result.incident_components.reserve(incident_by_component.size());
  for (auto& [component_index, incident] :
       incident_by_component) {
    static_cast<void>(component_index);
    std::sort(
        incident.removed_shell_point_ids.begin(),
        incident.removed_shell_point_ids.end());
    if (std::adjacent_find(
            incident.removed_shell_point_ids.begin(),
            incident.removed_shell_point_ids.end()) !=
        incident.removed_shell_point_ids.end()) {
      throw std::logic_error(
          "one critical arm was projected into a Gamma component twice");
    }
    result.incident_components.push_back(std::move(incident));
  }
  result.counters.incident_component_count =
      result.incident_components.size();
  if (result.arm_family.terminal_label_classes.size() >
          result.arm_family.arms.size() ||
      result.incident_components.size() >
          result.arm_family.terminal_label_classes.size()) {
    throw std::logic_error(
        "the arm, terminal-class and Gamma-component counts are inconsistent");
  }
  result.counters.same_terminal_label_arm_coalescence_count =
      result.arm_family.arms.size() -
      result.arm_family.terminal_label_classes.size();
  result.counters.
      distinct_terminal_label_component_coalescence_count =
      result.arm_family.terminal_label_classes.size() -
      result.incident_components.size();

  const bool projection_partition_certified =
      projection_partition_is_consistent(result);
  if (!projection_partition_certified) {
    throw std::logic_error(
        "the arm, terminal-class and Gamma-component projections disagree");
  }
  result.all_terminal_classes_active_and_classified =
      projection_partition_certified &&
      result.counters.terminal_class_component_projection_count ==
      result.arm_family.terminal_label_classes.size();
  result.every_arm_projected_once =
      projection_partition_certified &&
      result.counters.arm_component_projection_count ==
      result.arm_family.arms.size();
  result.terminal_label_partition_refines_gamma_component_partition =
      projection_partition_certified;
  result.decision = ExactCriticalArmGammaDecision::
      complete_arm_to_strict_gamma_component_classification;
  return result;
}

}  // namespace

ExactCriticalArmGammaVerification
verify_exact_critical_arm_gamma_component_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    ExactStrictGammaBudget strict_gamma_budget,
    const ExactCriticalArmGammaResult& result) {
  const ExactCriticalArmGammaResult expected =
      compute_exact_critical_arm_gamma_component_classification(
          cloud,
          critical_shell_point_ids,
          per_arm_chain_budget,
          strict_gamma_budget);
  ExactCriticalArmGammaVerification verification;
  verification.requested_per_arm_chain_budget_certified =
      result.requested_per_arm_chain_budget ==
          per_arm_chain_budget &&
      result.requested_per_arm_chain_budget ==
          expected.requested_per_arm_chain_budget;
  verification.requested_strict_gamma_budget_certified =
      result.requested_strict_gamma_budget ==
          strict_gamma_budget &&
      result.requested_strict_gamma_budget ==
          expected.requested_strict_gamma_budget;
  verification.input_shell_identity_certified =
      result.critical_shell_point_ids ==
          expected.critical_shell_point_ids;

  const ExactCriticalArmFamilyVerification family_verification =
      verify_exact_critical_arm_family_descent(
          cloud,
          critical_shell_point_ids,
          per_arm_chain_budget,
          result.arm_family);
  verification.arm_family_certified =
      family_verification.
          exact_critical_arm_family_decision_certified;
  verification.critical_order_and_level_certified =
      result.order == expected.order &&
      result.critical_squared_level ==
          expected.critical_squared_level;
  verification.strict_gamma_presence_certified =
      result.strict_gamma.has_value() ==
      expected.strict_gamma.has_value();
  verification.strict_gamma_certified =
      verification.strict_gamma_presence_certified;
  if (result.strict_gamma.has_value() &&
      expected.strict_gamma.has_value()) {
    const std::vector<FacetLabel> sources =
        terminal_class_sources(expected.arm_family);
    const ExactStrictGammaVerification gamma_verification =
        verify_exact_strict_gamma_source_classification(
            cloud,
            expected.order,
            expected.critical_squared_level,
            sources,
            strict_gamma_budget,
            *result.strict_gamma);
    verification.strict_gamma_certified =
        gamma_verification.exact_strict_gamma_decision_certified;
  }
  verification.terminal_class_classifications_certified =
      result.terminal_class_classifications ==
      expected.terminal_class_classifications;
  verification.arm_classifications_certified =
      result.arm_classifications == expected.arm_classifications;
  verification.incident_components_certified =
      result.incident_components == expected.incident_components;
  verification.result_facts_certified =
      result.arm_family_fresh_replay_certified ==
          expected.arm_family_fresh_replay_certified &&
      result.critical_order_and_level_derived_from_shared_source ==
          expected.
              critical_order_and_level_derived_from_shared_source &&
      result.strict_gamma_cut_fresh_replay_certified ==
          expected.strict_gamma_cut_fresh_replay_certified &&
      result.all_terminal_classes_active_and_classified ==
          expected.all_terminal_classes_active_and_classified &&
      result.every_arm_projected_once ==
          expected.every_arm_projected_once &&
      result.
          terminal_label_partition_refines_gamma_component_partition ==
          expected.
              terminal_label_partition_refines_gamma_component_partition;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalArmGammaScope::
          bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_per_arm_chain_budget_certified &&
      verification.requested_strict_gamma_budget_certified &&
      verification.input_shell_identity_certified &&
      verification.arm_family_certified &&
      verification.critical_order_and_level_certified &&
      verification.strict_gamma_presence_certified &&
      verification.strict_gamma_certified &&
      verification.terminal_class_classifications_certified &&
      verification.arm_classifications_certified &&
      verification.incident_components_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified;
  verification.exact_critical_arm_gamma_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalArmGammaResult
build_exact_critical_arm_gamma_component_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    ExactStrictGammaBudget strict_gamma_budget) {
  ExactCriticalArmGammaResult result =
      compute_exact_critical_arm_gamma_component_classification(
          cloud,
          critical_shell_point_ids,
          per_arm_chain_budget,
          strict_gamma_budget);
  const ExactCriticalArmGammaVerification verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          critical_shell_point_ids,
          per_arm_chain_budget,
          strict_gamma_budget,
          result);
  if (!verification.exact_critical_arm_gamma_decision_certified) {
    throw std::logic_error(
        "the exact critical-arm Gamma classification failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
