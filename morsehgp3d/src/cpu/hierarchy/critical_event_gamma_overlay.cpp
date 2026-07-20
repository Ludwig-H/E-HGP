#include "morsehgp3d/hierarchy/critical_event_gamma_overlay.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;
using IncidenceKey = std::pair<FacetLabel, FacetLabel>;

void validate_gamma_budget(const ExactStrictGammaBudget& budget) {
  if (budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count) {
    throw std::invalid_argument(
        "an exact critical-event overlay Gamma budget exceeds its cap");
  }
}

void validate_overlay_budget(
    const ExactCriticalEventGammaOverlayBudget& budget) {
  if (budget.maximum_event_count >
          ExactCriticalEventGammaOverlayBudget::
              maximum_supported_event_count ||
      budget.maximum_total_arm_count >
          ExactCriticalEventGammaOverlayBudget::
              maximum_supported_total_arm_count) {
    throw std::invalid_argument(
        "an exact critical-event overlay budget exceeds its cap");
  }
}

[[nodiscard]] std::vector<ExactCriticalEventGammaOverlayRequest>
canonicalize_event_requests(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests) {
  if (event_requests.empty() ||
      event_requests.size() >
          ExactCriticalEventGammaOverlayBudget::
              maximum_supported_event_count) {
    throw std::invalid_argument(
        "a critical-event overlay requires one to eight supplied events");
  }

  std::vector<ExactCriticalEventGammaOverlayRequest> canonical(
      event_requests.begin(), event_requests.end());
  for (ExactCriticalEventGammaOverlayRequest& request : canonical) {
    if (request.critical_shell_point_ids.size() <
            ExactCriticalArmInitialSegmentResult::
                minimum_critical_shell_point_count ||
        request.critical_shell_point_ids.size() >
            ExactCriticalArmInitialSegmentResult::
                maximum_critical_shell_point_count ||
        request.per_arm_chain_budget.
                maximum_committed_strict_segment_count >
            ExactFacetDescentChainBudget::
                maximum_supported_committed_strict_segment_count) {
      throw std::invalid_argument(
          "a supplied critical event has an invalid shell or arm budget");
    }
    std::sort(
        request.critical_shell_point_ids.begin(),
        request.critical_shell_point_ids.end());
    if (std::adjacent_find(
            request.critical_shell_point_ids.begin(),
            request.critical_shell_point_ids.end()) !=
        request.critical_shell_point_ids.end()) {
      throw std::invalid_argument(
          "a supplied critical-event shell repeats a point");
    }
    for (const PointId point_id : request.critical_shell_point_ids) {
      static_cast<void>(cloud.point(point_id));
    }
  }
  std::sort(
      canonical.begin(),
      canonical.end(),
      [](const ExactCriticalEventGammaOverlayRequest& left,
         const ExactCriticalEventGammaOverlayRequest& right) {
        return left.critical_shell_point_ids <
               right.critical_shell_point_ids;
      });
  for (std::size_t index = 1U; index < canonical.size(); ++index) {
    if (canonical[index - 1U].critical_shell_point_ids ==
        canonical[index].critical_shell_point_ids) {
      throw std::invalid_argument(
          "supplied critical-event shells must be distinct");
    }
  }
  return canonical;
}

[[nodiscard]] bool strict_gamma_core_matches(
    const ExactStrictGammaResult& left,
    const ExactStrictGammaResult& right) {
  return left.requested_budget == right.requested_budget &&
         left.point_count == right.point_count &&
         left.order == right.order &&
         left.strict_cut_squared_level == right.strict_cut_squared_level &&
         left.required_facet_count == right.required_facet_count &&
         left.required_coface_count == right.required_coface_count &&
         left.required_union_attempt_count ==
             right.required_union_attempt_count &&
         left.active_facets == right.active_facets &&
         left.active_cofaces == right.active_cofaces &&
         left.components == right.components &&
         left.candidate_space_size_certified ==
             right.candidate_space_size_certified &&
         left.strict_open_cut_certified ==
             right.strict_open_cut_certified &&
         left.full_pi0_isolated_facets_included ==
             right.full_pi0_isolated_facets_included &&
         left.exhaustive_active_catalog_certified ==
             right.exhaustive_active_catalog_certified;
}

[[nodiscard]] FacetLabel deletion_facet(
    std::span<const PointId> coface,
    PointId omitted_point_id) {
  FacetLabel facet;
  facet.reserve(coface.size() - 1U);
  for (const PointId point_id : coface) {
    if (point_id != omitted_point_id) {
      facet.push_back(point_id);
    }
  }
  if (facet.size() + 1U != coface.size()) {
    throw std::logic_error(
        "a critical-event deletion omitted no coface point");
  }
  return facet;
}

[[nodiscard]] bool complete_arm_family(
    const ExactCriticalArmGammaResult& event) {
  return event.arm_family.decision ==
             ExactCriticalArmFamilyDecision::
                 all_arms_complete_at_regular_active_facets &&
         event.arm_family.complete_terminal_label_partition_certified;
}

[[nodiscard]] bool group_overlay_partition_is_structurally_consistent(
    const ExactCriticalEventGammaOverlayResult& result) {
  if (!result.gamma_transition.has_value() ||
      result.event_projections.size() !=
          result.event_classifications.size() ||
      result.group_overlays.size() !=
          result.gamma_transition->transition_groups.size()) {
    return false;
  }

  std::vector<std::size_t> event_reference_counts(
      result.event_projections.size(), 0U);
  for (std::size_t event_index = 0U;
       event_index < result.event_projections.size();
       ++event_index) {
    const ExactCriticalEventGammaProjection& projection =
        result.event_projections[event_index];
    if (projection.canonical_event_index != event_index ||
        projection.transition_group_index >=
            result.group_overlays.size()) {
      return false;
    }
  }

  for (std::size_t group_index = 0U;
       group_index < result.group_overlays.size();
       ++group_index) {
    const ExactGammaTransitionGroup& transition_group =
        result.gamma_transition->transition_groups[group_index];
    const ExactCriticalEventGammaGroupOverlay& overlay =
        result.group_overlays[group_index];
    if (overlay.transition_group_index != group_index ||
        overlay.closed_component_index !=
            transition_group.closed_component_index ||
        overlay.canonical_representative_facet_point_ids !=
            transition_group.canonical_representative_facet_point_ids ||
        !std::is_sorted(
            overlay.canonical_event_indices.begin(),
            overlay.canonical_event_indices.end()) ||
        std::adjacent_find(
            overlay.canonical_event_indices.begin(),
            overlay.canonical_event_indices.end()) !=
            overlay.canonical_event_indices.end() ||
        overlay.has_supplied_event_provenance !=
            !overlay.canonical_event_indices.empty()) {
      return false;
    }

    std::set<FacetLabel> supplied_cofaces;
    for (const std::size_t event_index :
         overlay.canonical_event_indices) {
      if (event_index >= result.event_projections.size()) {
        return false;
      }
      const ExactCriticalEventGammaProjection& projection =
          result.event_projections[event_index];
      if (projection.canonical_event_index != event_index ||
          projection.transition_group_index != group_index ||
          projection.closed_component_index !=
              transition_group.closed_component_index ||
          std::find(
              transition_group.equal_level_coface_point_ids.begin(),
              transition_group.equal_level_coface_point_ids.end(),
              projection.critical_coface_point_ids) ==
              transition_group.equal_level_coface_point_ids.end() ||
          !supplied_cofaces
               .insert(projection.critical_coface_point_ids)
               .second) {
        return false;
      }
      ++event_reference_counts[event_index];
    }

    std::vector<FacetLabel> expected_without_provenance;
    for (const FacetLabel& coface :
         transition_group.equal_level_coface_point_ids) {
      if (!supplied_cofaces.contains(coface)) {
        expected_without_provenance.push_back(coface);
      }
    }
    if (overlay.
            equal_level_cofaces_without_supplied_event_provenance !=
        expected_without_provenance) {
      return false;
    }
  }

  return std::all_of(
      event_reference_counts.begin(),
      event_reference_counts.end(),
      [](std::size_t count) { return count == 1U; });
}

[[nodiscard]] ExactCriticalEventGammaOverlayResult
compute_exact_supplied_critical_event_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests,
    ExactCriticalEventGammaOverlayBudget overlay_budget,
    ExactStrictGammaBudget gamma_budget) {
  if (cloud.size() < 2U ||
      cloud.size() >
          ExactStrictGammaBudget::maximum_supported_point_count) {
    throw std::invalid_argument(
        "a critical-event Gamma overlay requires 2<=n<=14");
  }
  validate_overlay_budget(overlay_budget);
  validate_gamma_budget(gamma_budget);
  const std::vector<ExactCriticalEventGammaOverlayRequest> canonical_requests =
      canonicalize_event_requests(cloud, event_requests);

  ExactCriticalEventGammaOverlayResult result;
  result.requested_overlay_budget = overlay_budget;
  result.requested_gamma_budget = gamma_budget;
  result.canonical_event_requests = canonical_requests;
  result.required_event_count = canonical_requests.size();
  for (const ExactCriticalEventGammaOverlayRequest& request :
       canonical_requests) {
    result.required_total_arm_count +=
        request.critical_shell_point_ids.size();
  }
  result.counters.preflight_count = 1U;
  result.counters.required_event_count = result.required_event_count;
  result.counters.required_total_arm_count =
      result.required_total_arm_count;
  result.event_requests_canonical_certified = true;
  result.supplied_event_preflight_size_certified = true;
  result.scope = ExactCriticalEventGammaOverlayScope::
      bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only;

  if (overlay_budget.maximum_event_count < result.required_event_count ||
      overlay_budget.maximum_total_arm_count <
          result.required_total_arm_count) {
    result.decision = ExactCriticalEventGammaOverlayDecision::
        no_overlay_preflight_budget_insufficient;
    return result;
  }

  result.event_classifications.reserve(canonical_requests.size());
  for (const ExactCriticalEventGammaOverlayRequest& request :
       canonical_requests) {
    ExactCriticalArmGammaResult event =
        build_exact_critical_arm_gamma_component_classification(
            cloud,
            request.critical_shell_point_ids,
            request.per_arm_chain_budget,
            gamma_budget);
    ++result.counters.event_classification_build_count;
    if (complete_arm_family(event)) {
      ++result.counters.complete_arm_family_event_count;
    } else {
      ++result.counters.incomplete_arm_family_event_count;
    }
    if (event.decision == ExactCriticalArmGammaDecision::
                              no_classification_strict_gamma_preflight_budget_insufficient) {
      ++result.counters.gamma_preflight_insufficient_event_count;
    }
    if (event.decision == ExactCriticalArmGammaDecision::
                              complete_arm_to_strict_gamma_component_classification) {
      ++result.counters.complete_event_classification_count;
    }
    result.event_classifications.push_back(std::move(event));
  }
  result.all_event_classifications_fresh_replay_certified = true;

  if (result.counters.incomplete_arm_family_event_count != 0U) {
    result.decision = ExactCriticalEventGammaOverlayDecision::
        no_overlay_event_family_not_complete;
    return result;
  }

  const std::size_t candidate_common_order =
      result.event_classifications.front().order;
  const exact::ExactLevel candidate_common_squared_level =
      result.event_classifications.front().critical_squared_level;
  bool common_pair = true;
  for (std::size_t event_index = 1U;
       event_index < result.event_classifications.size();
       ++event_index) {
    ++result.counters.common_order_and_level_comparison_count;
    const ExactCriticalArmGammaResult& event =
        result.event_classifications[event_index];
    if (event.order != candidate_common_order ||
        event.critical_squared_level !=
            candidate_common_squared_level) {
      common_pair = false;
    }
  }
  if (!common_pair) {
    result.decision = ExactCriticalEventGammaOverlayDecision::
        no_overlay_mixed_order_or_exact_level;
    return result;
  }
  result.common_order = candidate_common_order;
  result.common_squared_level = candidate_common_squared_level;
  result.common_order_and_exact_level_derived = true;

  if (result.counters.gamma_preflight_insufficient_event_count != 0U) {
    result.decision = ExactCriticalEventGammaOverlayDecision::
        no_overlay_gamma_preflight_budget_insufficient;
    return result;
  }
  if (result.counters.complete_event_classification_count !=
      result.event_classifications.size()) {
    throw std::logic_error(
        "a complete supplied arm family has an unknown 6.9 decision");
  }

  result.gamma_transition = build_exact_gamma_equal_level_transition(
      cloud,
      result.common_order,
      result.common_squared_level,
      gamma_budget);
  result.counters.gamma_transition_build_count = 1U;
  if (result.gamma_transition->decision !=
      ExactGammaTransitionDecision::
          complete_exhaustive_open_to_closed_transition) {
    throw std::logic_error(
        "successful event cuts did not yield a complete Gamma transition");
  }

  for (const ExactCriticalArmGammaResult& event :
       result.event_classifications) {
    ++result.counters.strict_gamma_core_comparison_count;
    if (!event.strict_gamma.has_value() ||
        !strict_gamma_core_matches(
            *event.strict_gamma,
            result.gamma_transition->strict_gamma)) {
      throw std::logic_error(
          "a supplied event and the transition disagree on the strict Gamma core");
    }
  }
  result.all_strict_gamma_cores_match_transition = true;

  std::map<FacetLabel, std::size_t> strict_component_by_facet;
  for (std::size_t component_index = 0U;
       component_index <
           result.gamma_transition->strict_gamma.components.size();
       ++component_index) {
    for (const FacetLabel& facet :
         result.gamma_transition->strict_gamma
             .components[component_index]
             .facet_point_ids) {
      if (!strict_component_by_facet
               .emplace(facet, component_index)
               .second) {
        throw std::logic_error(
            "a strict Gamma facet belongs to multiple components");
      }
    }
  }

  std::set<FacetLabel> equal_level_facets;
  for (const ExactGammaEqualLevelFacetWitness& facet :
       result.gamma_transition->equal_level_facets) {
    equal_level_facets.insert(facet.facet_point_ids);
  }
  std::map<FacetLabel, std::size_t> equal_level_coface_index;
  for (std::size_t coface_index = 0U;
       coface_index < result.gamma_transition->equal_level_cofaces.size();
       ++coface_index) {
    equal_level_coface_index.emplace(
        result.gamma_transition->equal_level_cofaces[coface_index]
            .coface_point_ids,
        coface_index);
  }
  std::map<IncidenceKey, const ExactGammaTransitionIncidence*>
      incidence_by_coface_and_facet;
  for (const ExactGammaTransitionIncidence& incidence :
       result.gamma_transition->equal_level_incidences) {
    if (!incidence_by_coface_and_facet
             .emplace(
                 IncidenceKey{
                     incidence.coface_point_ids,
                     incidence.facet_point_ids},
                 &incidence)
             .second) {
      throw std::logic_error(
          "a Gamma transition repeated one equality incidence");
    }
  }
  std::map<FacetLabel, std::size_t> transition_group_by_coface;
  for (std::size_t group_index = 0U;
       group_index < result.gamma_transition->transition_groups.size();
       ++group_index) {
    for (const FacetLabel& coface :
         result.gamma_transition->transition_groups[group_index]
             .equal_level_coface_point_ids) {
      if (!transition_group_by_coface.emplace(coface, group_index).second) {
        throw std::logic_error(
            "an equality coface belongs to multiple transition groups");
      }
    }
  }

  std::set<FacetLabel> supplied_critical_cofaces;
  result.event_projections.reserve(result.event_classifications.size());
  for (std::size_t event_index = 0U;
       event_index < result.event_classifications.size();
       ++event_index) {
    const ExactCriticalArmGammaResult& event =
        result.event_classifications[event_index];
    const ExactCriticalArmFamilyResult& family = event.arm_family;
    if (family.arms.empty()) {
      throw std::logic_error("a complete supplied event has no critical arms");
    }
    const ExactCriticalArmInitialSegmentResult& shared_source =
        family.arms.front().descent.initial_segment;
    if (!shared_source.global_closed_ball.has_value()) {
      throw std::logic_error(
          "a complete supplied event has no global critical partition");
    }

    ExactCriticalEventGammaProjection projection;
    projection.canonical_event_index = event_index;
    projection.critical_shell_point_ids =
        family.critical_shell_point_ids;
    projection.interior_point_ids.assign(
        shared_source.global_closed_ball->interior_ids().begin(),
        shared_source.global_closed_ball->interior_ids().end());
    projection.critical_coface_point_ids = projection.interior_point_ids;
    projection.critical_coface_point_ids.insert(
        projection.critical_coface_point_ids.end(),
        projection.critical_shell_point_ids.begin(),
        projection.critical_shell_point_ids.end());
    std::sort(
        projection.critical_coface_point_ids.begin(),
        projection.critical_coface_point_ids.end());
    if (std::adjacent_find(
            projection.critical_coface_point_ids.begin(),
            projection.critical_coface_point_ids.end()) !=
            projection.critical_coface_point_ids.end() ||
        projection.critical_coface_point_ids.size() !=
            result.common_order + 1U ||
        !supplied_critical_cofaces
             .insert(projection.critical_coface_point_ids)
             .second) {
      throw std::logic_error(
          "supplied critical events do not define distinct order-(k+1) cofaces");
    }

    ++result.counters.critical_coface_lookup_count;
    const auto coface = equal_level_coface_index.find(
        projection.critical_coface_point_ids);
    if (coface == equal_level_coface_index.end()) {
      throw std::logic_error(
          "a supplied critical event is absent from the equality coface catalogue");
    }
    projection.equal_level_coface_index = coface->second;
    const auto group_position = transition_group_by_coface.find(
        projection.critical_coface_point_ids);
    if (group_position == transition_group_by_coface.end()) {
      throw std::logic_error(
          "a supplied critical event belongs to no transition group");
    }
    projection.transition_group_index = group_position->second;
    const ExactGammaTransitionGroup& transition_group =
        result.gamma_transition
            ->transition_groups[projection.transition_group_index];
    projection.closed_component_index =
        transition_group.closed_component_index;
    if (transition_group.strict_component_indices.empty()) {
      throw std::logic_error(
          "a complete critical event cannot project to a q=0 group");
    }

    std::map<PointId, const ExactCriticalArmFamilyArmResult*> family_arm_by_id;
    for (const ExactCriticalArmFamilyArmResult& arm : family.arms) {
      family_arm_by_id.emplace(arm.removed_shell_point_id, &arm);
    }
    std::map<PointId, const ExactCriticalArmGammaArmClassification*>
        arm_mapping_by_id;
    for (const ExactCriticalArmGammaArmClassification& arm_mapping :
         event.arm_classifications) {
      arm_mapping_by_id.emplace(
          arm_mapping.removed_shell_point_id,
          &arm_mapping);
    }

    for (const PointId removed_point_id :
         projection.critical_shell_point_ids) {
      const FacetLabel initial_facet = deletion_facet(
          projection.critical_coface_point_ids,
          removed_point_id);
      const auto family_arm = family_arm_by_id.find(removed_point_id);
      const auto arm_mapping = arm_mapping_by_id.find(removed_point_id);
      const auto strict_component = strict_component_by_facet.find(
          initial_facet);
      const auto incidence = incidence_by_coface_and_facet.find(
          IncidenceKey{
              projection.critical_coface_point_ids,
              initial_facet});
      if (family_arm == family_arm_by_id.end() ||
          arm_mapping == arm_mapping_by_id.end() ||
          strict_component == strict_component_by_facet.end() ||
          incidence == incidence_by_coface_and_facet.end() ||
          family_arm->second->descent.initial_segment
                  .arm_facet_point_ids != initial_facet ||
          !family_arm->second->active_terminal.has_value() ||
          !family_arm->second->terminal_label_class_index.has_value() ||
          arm_mapping->second->terminal_label_class_index !=
              *family_arm->second->terminal_label_class_index ||
          arm_mapping->second->strict_gamma_component_index !=
              strict_component->second ||
          !incidence->second->strict_component_index.has_value() ||
          *incidence->second->strict_component_index !=
              strict_component->second ||
          incidence->second->newly_active_at_level ||
          !std::binary_search(
              transition_group.strict_component_indices.begin(),
              transition_group.strict_component_indices.end(),
              strict_component->second) ||
          result.gamma_transition
                  ->strict_component_to_closed_component_index
                  .at(strict_component->second) !=
              projection.closed_component_index) {
        throw std::logic_error(
            "a supplied critical arm does not reconcile with its strict Gamma token");
      }
      projection.arm_incidences.push_back(
          ExactCriticalEventGammaArmIncidence{
              removed_point_id,
              initial_facet,
              family_arm->second->active_terminal->facet_point_ids,
              arm_mapping->second->terminal_label_class_index,
              strict_component->second});
      ++result.counters.deletion_incidence_projection_count;
      ++result.counters.arm_incidence_projection_count;
    }

    for (const PointId removed_point_id : projection.interior_point_ids) {
      const FacetLabel equal_facet = deletion_facet(
          projection.critical_coface_point_ids,
          removed_point_id);
      const auto incidence = incidence_by_coface_and_facet.find(
          IncidenceKey{
              projection.critical_coface_point_ids,
              equal_facet});
      if (!equal_level_facets.contains(equal_facet) ||
          incidence == incidence_by_coface_and_facet.end() ||
          incidence->second->strict_component_index.has_value() ||
          !incidence->second->newly_active_at_level ||
          !std::binary_search(
              transition_group.newly_active_facet_point_ids.begin(),
              transition_group.newly_active_facet_point_ids.end(),
              equal_facet)) {
        throw std::logic_error(
            "an interior deletion does not reconcile with a new equality facet");
      }
      projection.interior_incidences.push_back(
          ExactCriticalEventGammaInteriorIncidence{
              removed_point_id,
              equal_facet});
      ++result.counters.deletion_incidence_projection_count;
      ++result.counters.interior_incidence_projection_count;
    }
    if (projection.arm_incidences.size() +
            projection.interior_incidences.size() !=
        projection.critical_coface_point_ids.size()) {
      throw std::logic_error(
          "a supplied critical coface has an incomplete deletion partition");
    }
    result.event_projections.push_back(std::move(projection));
    ++result.counters.supplied_event_projection_count;
  }

  result.critical_event_cofaces_distinct_and_equal_level =
      supplied_critical_cofaces.size() ==
      result.event_classifications.size();
  result.every_event_deletion_incidence_reconciled =
      result.counters.deletion_incidence_projection_count ==
      result.event_classifications.size() * (result.common_order + 1U) &&
      result.counters.arm_incidence_projection_count ==
          result.required_total_arm_count;
  result.every_supplied_event_projected_once =
      result.counters.supplied_event_projection_count ==
          result.event_classifications.size() &&
      result.event_projections.size() ==
          result.event_classifications.size();

  result.group_overlays.reserve(
      result.gamma_transition->transition_groups.size());
  for (std::size_t group_index = 0U;
       group_index < result.gamma_transition->transition_groups.size();
       ++group_index) {
    const ExactGammaTransitionGroup& group =
        result.gamma_transition->transition_groups[group_index];
    ExactCriticalEventGammaGroupOverlay overlay;
    overlay.transition_group_index = group_index;
    overlay.closed_component_index = group.closed_component_index;
    overlay.canonical_representative_facet_point_ids =
        group.canonical_representative_facet_point_ids;
    for (const ExactCriticalEventGammaProjection& projection :
         result.event_projections) {
      if (projection.transition_group_index == group_index) {
        overlay.canonical_event_indices.push_back(
            projection.canonical_event_index);
        ++result.counters.group_supplied_event_reference_count;
      }
    }
    std::set<FacetLabel> supplied_group_cofaces;
    for (const std::size_t event_index :
         overlay.canonical_event_indices) {
      supplied_group_cofaces.insert(
          result.event_projections[event_index]
              .critical_coface_point_ids);
    }
    for (const FacetLabel& coface :
         group.equal_level_coface_point_ids) {
      if (!supplied_group_cofaces.contains(coface)) {
        overlay.equal_level_cofaces_without_supplied_event_provenance
            .push_back(coface);
      }
    }
    overlay.has_supplied_event_provenance =
        !overlay.canonical_event_indices.empty();
    if (overlay.has_supplied_event_provenance) {
      ++result.counters.group_with_supplied_event_count;
    } else {
      ++result.counters.group_without_supplied_event_count;
    }
    result.group_overlays.push_back(std::move(overlay));
  }
  result.counters.transition_group_overlay_count =
      result.group_overlays.size();
  result.group_overlays_partition_transition_groups =
      group_overlay_partition_is_structurally_consistent(result) &&
      result.counters.group_supplied_event_reference_count ==
          result.event_projections.size() &&
      result.counters.transition_group_overlay_count ==
          result.counters.group_with_supplied_event_count +
              result.counters.group_without_supplied_event_count;
  result.supplied_event_provenance_order_independent = true;

  if (!result.event_requests_canonical_certified ||
      !result.supplied_event_preflight_size_certified ||
      !result.all_event_classifications_fresh_replay_certified ||
      !result.common_order_and_exact_level_derived ||
      !result.all_strict_gamma_cores_match_transition ||
      !result.critical_event_cofaces_distinct_and_equal_level ||
      !result.every_event_deletion_incidence_reconciled ||
      !result.every_supplied_event_projected_once ||
      !result.group_overlays_partition_transition_groups ||
      !result.supplied_event_provenance_order_independent) {
    throw std::logic_error(
        "the supplied critical-event Gamma overlay failed its proof gate");
  }
  result.decision = ExactCriticalEventGammaOverlayDecision::
      complete_supplied_event_provenance_overlay;
  return result;
}

}  // namespace

ExactCriticalEventGammaOverlayVerification
verify_exact_supplied_critical_event_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests,
    ExactCriticalEventGammaOverlayBudget overlay_budget,
    ExactStrictGammaBudget gamma_budget,
    const ExactCriticalEventGammaOverlayResult& result) {
  const ExactCriticalEventGammaOverlayResult expected =
      compute_exact_supplied_critical_event_gamma_overlay(
          cloud,
          event_requests,
          overlay_budget,
          gamma_budget);
  ExactCriticalEventGammaOverlayVerification verification;
  verification.requested_overlay_budget_certified =
      result.requested_overlay_budget == overlay_budget &&
      result.requested_overlay_budget ==
          expected.requested_overlay_budget;
  verification.requested_gamma_budget_certified =
      result.requested_gamma_budget == gamma_budget &&
      result.requested_gamma_budget == expected.requested_gamma_budget;
  verification.canonical_event_requests_certified =
      result.canonical_event_requests ==
      expected.canonical_event_requests;
  verification.preflight_counts_certified =
      result.required_event_count == expected.required_event_count &&
      result.required_total_arm_count ==
          expected.required_total_arm_count;

  verification.event_classifications_certified =
      result.event_classifications.size() ==
          expected.event_classifications.size();
  if (verification.event_classifications_certified) {
    for (std::size_t event_index = 0U;
         event_index < result.event_classifications.size();
         ++event_index) {
      const ExactCriticalEventGammaOverlayRequest& request =
          expected.canonical_event_requests[event_index];
      const ExactCriticalArmGammaVerification event_verification =
          verify_exact_critical_arm_gamma_component_classification(
              cloud,
              request.critical_shell_point_ids,
              request.per_arm_chain_budget,
              gamma_budget,
              result.event_classifications[event_index]);
      if (!event_verification.
               exact_critical_arm_gamma_decision_certified) {
        verification.event_classifications_certified = false;
      }
    }
  }
  verification.common_order_and_level_certified =
      result.common_order == expected.common_order &&
      result.common_squared_level == expected.common_squared_level;
  verification.gamma_transition_presence_certified =
      result.gamma_transition.has_value() ==
      expected.gamma_transition.has_value();
  verification.gamma_transition_certified =
      verification.gamma_transition_presence_certified;
  if (result.gamma_transition.has_value() &&
      expected.gamma_transition.has_value()) {
    const ExactGammaTransitionVerification transition_verification =
        verify_exact_gamma_equal_level_transition(
            cloud,
            expected.common_order,
            expected.common_squared_level,
            gamma_budget,
            *result.gamma_transition);
    verification.gamma_transition_certified =
        transition_verification.
            exact_gamma_transition_decision_certified;
  }
  verification.event_projections_certified =
      result.event_projections == expected.event_projections;
  verification.group_overlays_certified =
      result.group_overlays == expected.group_overlays;
  verification.result_facts_certified =
      result.event_requests_canonical_certified ==
          expected.event_requests_canonical_certified &&
      result.supplied_event_preflight_size_certified ==
          expected.supplied_event_preflight_size_certified &&
      result.all_event_classifications_fresh_replay_certified ==
          expected.all_event_classifications_fresh_replay_certified &&
      result.common_order_and_exact_level_derived ==
          expected.common_order_and_exact_level_derived &&
      result.all_strict_gamma_cores_match_transition ==
          expected.all_strict_gamma_cores_match_transition &&
      result.critical_event_cofaces_distinct_and_equal_level ==
          expected.critical_event_cofaces_distinct_and_equal_level &&
      result.every_event_deletion_incidence_reconciled ==
          expected.every_event_deletion_incidence_reconciled &&
      result.every_supplied_event_projected_once ==
          expected.every_supplied_event_projected_once &&
      result.group_overlays_partition_transition_groups ==
          expected.group_overlays_partition_transition_groups &&
      result.supplied_event_provenance_order_independent ==
          expected.supplied_event_provenance_order_independent;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalEventGammaOverlayScope::
          bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_overlay_budget_certified &&
      verification.requested_gamma_budget_certified &&
      verification.canonical_event_requests_certified &&
      verification.preflight_counts_certified &&
      verification.event_classifications_certified &&
      verification.common_order_and_level_certified &&
      verification.gamma_transition_presence_certified &&
      verification.gamma_transition_certified &&
      verification.event_projections_certified &&
      verification.group_overlays_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified;
  verification.exact_critical_event_gamma_overlay_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalEventGammaOverlayResult
build_exact_supplied_critical_event_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests,
    ExactCriticalEventGammaOverlayBudget overlay_budget,
    ExactStrictGammaBudget gamma_budget) {
  ExactCriticalEventGammaOverlayResult result =
      compute_exact_supplied_critical_event_gamma_overlay(
          cloud,
          event_requests,
          overlay_budget,
          gamma_budget);
  const ExactCriticalEventGammaOverlayVerification verification =
      verify_exact_supplied_critical_event_gamma_overlay(
          cloud,
          event_requests,
          overlay_budget,
          gamma_budget,
          result);
  if (!verification.
           exact_critical_event_gamma_overlay_decision_certified) {
    throw std::logic_error(
        "the supplied critical-event Gamma overlay failed fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
