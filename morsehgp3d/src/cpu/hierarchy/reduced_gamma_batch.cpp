#include "morsehgp3d/hierarchy/reduced_gamma_batch.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;

[[nodiscard]] std::vector<PointId> covered_point_ids(
    const std::vector<FacetLabel>& facets) {
  std::set<PointId> points;
  for (const FacetLabel& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return {points.begin(), points.end()};
}

[[nodiscard]] std::vector<FacetLabel> facet_set_difference(
    const std::vector<FacetLabel>& closed_facets,
    std::vector<FacetLabel> prior_root_facets) {
  std::sort(prior_root_facets.begin(), prior_root_facets.end());
  if (std::adjacent_find(
          prior_root_facets.begin(), prior_root_facets.end()) !=
      prior_root_facets.end()) {
    throw std::logic_error(
        "prior reduced Gamma roots share a strict facet");
  }
  std::vector<FacetLabel> difference;
  std::set_difference(
      closed_facets.begin(),
      closed_facets.end(),
      prior_root_facets.begin(),
      prior_root_facets.end(),
      std::back_inserter(difference));
  if (difference.size() + prior_root_facets.size() !=
      closed_facets.size()) {
    throw std::logic_error(
        "prior reduced Gamma root facets do not form a closed-component "
        "subset");
  }
  return difference;
}

[[nodiscard]] std::vector<PointId> point_set_difference(
    const std::vector<PointId>& closed_points,
    const std::vector<PointId>& prior_root_points) {
  std::vector<PointId> difference;
  std::set_difference(
      closed_points.begin(),
      closed_points.end(),
      prior_root_points.begin(),
      prior_root_points.end(),
      std::back_inserter(difference));
  return difference;
}

[[nodiscard]] ExactReducedGammaBatchGroupKind reduced_group_kind(
    std::size_t prior_root_count) {
  if (prior_root_count == 0U) {
    return ExactReducedGammaBatchGroupKind::birth;
  }
  if (prior_root_count == 1U) {
    return ExactReducedGammaBatchGroupKind::continuation;
  }
  return ExactReducedGammaBatchGroupKind::multifusion;
}

[[nodiscard]] bool classifications_are_structurally_certified(
    const ExactReducedGammaBatchResult& result) {
  const auto& components =
      result.gamma_transition.strict_gamma.components;
  const auto& cofaces =
      result.gamma_transition.strict_gamma.active_cofaces;
  if (result.strict_component_classifications.size() !=
      components.size()) {
    return false;
  }

  std::map<FacetLabel, std::size_t> component_by_facet;
  for (std::size_t component_index = 0U;
       component_index < components.size();
       ++component_index) {
    for (const FacetLabel& facet :
         components[component_index].facet_point_ids) {
      if (!component_by_facet.emplace(facet, component_index).second) {
        return false;
      }
    }
  }

  std::vector<std::vector<std::size_t>> expected_incidences(
      components.size());
  for (std::size_t coface_index = 0U;
       coface_index < cofaces.size();
       ++coface_index) {
    std::optional<std::size_t> component_index;
    for (const FacetLabel& facet :
         cofaces[coface_index].facet_point_ids) {
      const auto found = component_by_facet.find(facet);
      if (found == component_by_facet.end()) {
        return false;
      }
      if (component_index.has_value() &&
          *component_index != found->second) {
        return false;
      }
      component_index = found->second;
    }
    if (!component_index.has_value()) {
      return false;
    }
    expected_incidences[*component_index].push_back(coface_index);
  }

  for (std::size_t component_index = 0U;
       component_index < components.size();
       ++component_index) {
    const auto& component = components[component_index];
    const auto& classification =
        result.strict_component_classifications[component_index];
    const bool incident = !expected_incidences[component_index].empty();
    const bool nontrivial = component.facet_point_ids.size() > 1U;
    if (classification.strict_component_index != component_index ||
        classification.canonical_representative_facet_point_ids !=
            component.canonical_representative_facet_point_ids ||
        classification.facet_count != component.facet_point_ids.size() ||
        classification.incident_strict_coface_indices !=
            expected_incidences[component_index] ||
        classification.incident_to_strict_coface != incident ||
        classification.facet_count_is_nontrivial != nontrivial ||
        !classification.
            incidence_nontriviality_equivalence_certified ||
        incident != nontrivial ||
        classification.carries_prior_reduced_root != nontrivial) {
      return false;
    }
    const ExactReducedGammaStrictComponentKind expected_kind =
        nontrivial
            ? ExactReducedGammaStrictComponentKind::
                  prior_nontrivial_reduced_root
            : ExactReducedGammaStrictComponentKind::
                  omitted_isolated_facet;
    if (classification.kind != expected_kind) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool groups_are_structurally_certified(
    const ExactReducedGammaBatchResult& result) {
  const auto& transition = result.gamma_transition;
  if (result.groups.size() != transition.transition_groups.size()) {
    return false;
  }
  for (std::size_t group_index = 0U;
       group_index < result.groups.size();
       ++group_index) {
    const auto& source = transition.transition_groups[group_index];
    const auto& group = result.groups[group_index];
    if (source.closed_component_index >=
            transition.closed_components.size() ||
        group.transition_group_index != group_index ||
        group.closed_component_index != source.closed_component_index ||
        group.canonical_representative_facet_point_ids !=
            source.canonical_representative_facet_point_ids ||
        group.strict_component_indices !=
            source.strict_component_indices ||
        group.newly_active_facet_point_ids !=
            source.newly_active_facet_point_ids ||
        group.equal_level_coface_point_ids !=
            source.equal_level_coface_point_ids) {
      return false;
    }

    std::vector<std::size_t> partition;
    partition.reserve(
        group.prior_reduced_root_strict_component_indices.size() +
        group.absorbed_isolated_strict_component_indices.size());
    partition.insert(
        partition.end(),
        group.prior_reduced_root_strict_component_indices.begin(),
        group.prior_reduced_root_strict_component_indices.end());
    partition.insert(
        partition.end(),
        group.absorbed_isolated_strict_component_indices.begin(),
        group.absorbed_isolated_strict_component_indices.end());
    std::sort(partition.begin(), partition.end());
    if (partition != source.strict_component_indices) {
      return false;
    }
    for (const std::size_t component_index :
         group.prior_reduced_root_strict_component_indices) {
      if (component_index >=
              result.strict_component_classifications.size() ||
          !result.strict_component_classifications[component_index]
               .carries_prior_reduced_root) {
        return false;
      }
    }
    for (const std::size_t component_index :
         group.absorbed_isolated_strict_component_indices) {
      if (component_index >=
              result.strict_component_classifications.size() ||
          result.strict_component_classifications[component_index]
              .carries_prior_reduced_root) {
        return false;
      }
    }

    if (source.equal_level_coface_point_ids.empty()) {
      if (group.kind != ExactReducedGammaBatchGroupKind::
                            deferred_isolated_facet ||
          group.coverage_delta.has_value() ||
          !source.strict_component_indices.empty() ||
          source.newly_active_facet_point_ids.size() != 1U) {
        return false;
      }
      continue;
    }

    if (!group.coverage_delta.has_value() ||
        group.kind != reduced_group_kind(
                          group.prior_reduced_root_strict_component_indices
                              .size())) {
      return false;
    }
    const auto& closed_component =
        transition.closed_components[source.closed_component_index];
    std::vector<FacetLabel> prior_root_facets;
    for (const std::size_t component_index :
         group.prior_reduced_root_strict_component_indices) {
      if (component_index >=
          transition.strict_gamma.components.size()) {
        return false;
      }
      const auto& facets = transition.strict_gamma
                                .components[component_index]
                                .facet_point_ids;
      prior_root_facets.insert(
          prior_root_facets.end(), facets.begin(), facets.end());
    }
    std::sort(prior_root_facets.begin(), prior_root_facets.end());
    if (std::adjacent_find(
            prior_root_facets.begin(), prior_root_facets.end()) !=
        prior_root_facets.end()) {
      return false;
    }
    std::vector<FacetLabel> expected_added_facets;
    std::set_difference(
        closed_component.facet_point_ids.begin(),
        closed_component.facet_point_ids.end(),
        prior_root_facets.begin(),
        prior_root_facets.end(),
        std::back_inserter(expected_added_facets));
    if (expected_added_facets.size() + prior_root_facets.size() !=
        closed_component.facet_point_ids.size()) {
      return false;
    }
    const std::vector<PointId> closed_points =
        covered_point_ids(closed_component.facet_point_ids);
    const std::vector<PointId> prior_points =
        covered_point_ids(prior_root_facets);
    std::vector<PointId> expected_added_points;
    std::set_difference(
        closed_points.begin(),
        closed_points.end(),
        prior_points.begin(),
        prior_points.end(),
        std::back_inserter(expected_added_points));
    if (group.coverage_delta->added_facet_point_ids !=
            expected_added_facets ||
        group.coverage_delta->added_point_ids != expected_added_points ||
        group.coverage_delta->fully_redundant !=
            (expected_added_facets.empty() &&
             expected_added_points.empty())) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactReducedGammaBatchResult
compute_exact_reduced_gamma_batch(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget) {
  if (cloud.size() < 2U ||
      cloud.size() >
          ExactStrictGammaBudget::maximum_supported_point_count ||
      order < 2U ||
      order > ExactStrictGammaBudget::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "a reduced Gamma batch requires 2<=n<=14 and "
        "2<=k<min(n,11)");
  }

  ExactReducedGammaBatchResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.squared_level = squared_level;
  result.scope = ExactReducedGammaBatchScope::
      bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only;
  result.gamma_transition = build_exact_gamma_equal_level_transition(
      cloud, order, squared_level, budget);
  result.counters.gamma_transition_build_count = 1U;

  if (result.gamma_transition.decision ==
      ExactGammaTransitionDecision::
          no_transition_preflight_budget_insufficient) {
    result.decision = ExactReducedGammaBatchDecision::
        no_batch_preflight_budget_insufficient;
    return result;
  }
  if (result.gamma_transition.decision !=
      ExactGammaTransitionDecision::
          complete_exhaustive_open_to_closed_transition) {
    throw std::logic_error(
        "the reduced Gamma batch received an uncertified transition");
  }
  // The 6.10 builder closes its own fresh replay before returning.  The
  // public verifier below independently replays the embedded transition.
  result.gamma_transition_fresh_replay_certified = true;

  const auto& strict_components =
      result.gamma_transition.strict_gamma.components;
  const auto& strict_cofaces =
      result.gamma_transition.strict_gamma.active_cofaces;
  std::map<FacetLabel, std::size_t> strict_component_by_facet;
  result.strict_component_classifications.reserve(
      strict_components.size());
  for (std::size_t component_index = 0U;
       component_index < strict_components.size();
       ++component_index) {
    const auto& component = strict_components[component_index];
    ExactReducedGammaStrictComponentClassification classification;
    classification.strict_component_index = component_index;
    classification.canonical_representative_facet_point_ids =
        component.canonical_representative_facet_point_ids;
    classification.facet_count = component.facet_point_ids.size();
    result.strict_component_classifications.push_back(
        std::move(classification));
    ++result.counters.strict_component_classification_count;
    for (const FacetLabel& facet : component.facet_point_ids) {
      if (!strict_component_by_facet
               .emplace(facet, component_index)
               .second) {
        throw std::logic_error(
            "strict Gamma components share a facet");
      }
    }
  }

  for (std::size_t coface_index = 0U;
       coface_index < strict_cofaces.size();
       ++coface_index) {
    const auto& coface = strict_cofaces[coface_index];
    std::optional<std::size_t> component_index;
    ++result.counters.strict_coface_incidence_scan_count;
    for (const FacetLabel& facet : coface.facet_point_ids) {
      ++result.counters.strict_coface_facet_lookup_count;
      const auto found = strict_component_by_facet.find(facet);
      if (found == strict_component_by_facet.end()) {
        throw std::logic_error(
            "an active strict Gamma coface references no component");
      }
      if (component_index.has_value() &&
          *component_index != found->second) {
        throw std::logic_error(
            "an active strict Gamma coface spans multiple components");
      }
      component_index = found->second;
    }
    if (!component_index.has_value()) {
      throw std::logic_error(
          "an active strict Gamma coface has no deletion facet");
    }
    result.strict_component_classifications[*component_index]
        .incident_strict_coface_indices.push_back(coface_index);
  }

  for (ExactReducedGammaStrictComponentClassification& classification :
       result.strict_component_classifications) {
    classification.incident_to_strict_coface =
        !classification.incident_strict_coface_indices.empty();
    classification.facet_count_is_nontrivial =
        classification.facet_count > 1U;
    classification.incidence_nontriviality_equivalence_certified =
        classification.incident_to_strict_coface ==
        classification.facet_count_is_nontrivial;
    ++result.counters.strict_component_facet_count_check_count;
    if (!classification.
            incidence_nontriviality_equivalence_certified) {
      throw std::logic_error(
          "Gamma incidence and strict-component nontriviality disagree");
    }
    classification.carries_prior_reduced_root =
        classification.incident_to_strict_coface;
    if (classification.carries_prior_reduced_root) {
      classification.kind = ExactReducedGammaStrictComponentKind::
          prior_nontrivial_reduced_root;
      ++result.counters.prior_reduced_root_count;
    } else {
      classification.kind = ExactReducedGammaStrictComponentKind::
          omitted_isolated_facet;
      ++result.counters.omitted_isolated_strict_component_count;
    }
  }

  result.strict_components_exhaustively_classified =
      result.counters.strict_component_classification_count ==
          strict_components.size() &&
      result.counters.strict_component_facet_count_check_count ==
          strict_components.size();
  result.
      strict_component_incidence_nontriviality_equivalence_certified =
      std::all_of(
          result.strict_component_classifications.begin(),
          result.strict_component_classifications.end(),
          [](const auto& classification) {
            return classification.
                incidence_nontriviality_equivalence_certified;
          });
  result.strict_reduced_roots_exactly_nontrivial_components =
      result.counters.prior_reduced_root_count +
              result.counters.omitted_isolated_strict_component_count ==
          strict_components.size() &&
      result.
          strict_component_incidence_nontriviality_equivalence_certified;

  result.groups.reserve(
      result.gamma_transition.transition_groups.size());
  for (std::size_t group_index = 0U;
       group_index < result.gamma_transition.transition_groups.size();
       ++group_index) {
    const ExactGammaTransitionGroup& transition_group =
        result.gamma_transition.transition_groups[group_index];
    if (transition_group.closed_component_index >=
        result.gamma_transition.closed_components.size()) {
      throw std::logic_error(
          "a Gamma transition group references no closed component");
    }
    ExactReducedGammaBatchGroup group;
    group.transition_group_index = group_index;
    group.closed_component_index =
        transition_group.closed_component_index;
    group.canonical_representative_facet_point_ids =
        transition_group.canonical_representative_facet_point_ids;
    group.strict_component_indices =
        transition_group.strict_component_indices;
    group.newly_active_facet_point_ids =
        transition_group.newly_active_facet_point_ids;
    group.equal_level_coface_point_ids =
        transition_group.equal_level_coface_point_ids;

    for (const std::size_t component_index :
         transition_group.strict_component_indices) {
      if (component_index >=
          result.strict_component_classifications.size()) {
        throw std::logic_error(
            "a Gamma transition group references no strict component");
      }
      ++result.counters.
          transition_group_strict_component_reference_count;
      if (result.strict_component_classifications[component_index]
              .carries_prior_reduced_root) {
        group.prior_reduced_root_strict_component_indices.push_back(
            component_index);
        ++result.counters.prior_reduced_root_reference_count;
      } else {
        group.absorbed_isolated_strict_component_indices.push_back(
            component_index);
        ++result.counters.absorbed_isolated_reference_count;
      }
    }

    if (transition_group.equal_level_coface_point_ids.empty()) {
      if (!transition_group.strict_component_indices.empty() ||
          transition_group.newly_active_facet_point_ids.size() != 1U) {
        throw std::logic_error(
            "a coface-free Gamma group is not one deferred isolated "
            "facet");
      }
      group.kind = ExactReducedGammaBatchGroupKind::
          deferred_isolated_facet;
      ++result.counters.deferred_isolated_facet_group_count;
    } else {
      group.kind = reduced_group_kind(
          group.prior_reduced_root_strict_component_indices.size());
      switch (group.kind) {
        case ExactReducedGammaBatchGroupKind::birth:
          ++result.counters.birth_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::continuation:
          ++result.counters.continuation_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::multifusion:
          ++result.counters.multifusion_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
          throw std::logic_error(
              "a Gamma coface group was classified as a deferred facet");
      }

      const auto& closed_component =
          result.gamma_transition.closed_components
              [transition_group.closed_component_index];
      std::vector<FacetLabel> prior_root_facets;
      for (const std::size_t component_index :
           group.prior_reduced_root_strict_component_indices) {
        const auto& facets = result.gamma_transition.strict_gamma
                                  .components[component_index]
                                  .facet_point_ids;
        prior_root_facets.insert(
            prior_root_facets.end(), facets.begin(), facets.end());
        result.counters.prior_root_facet_scan_count += facets.size();
      }
      result.counters.closed_component_facet_scan_count +=
          closed_component.facet_point_ids.size();
      ExactReducedGammaCoverageDelta delta;
      delta.added_facet_point_ids = facet_set_difference(
          closed_component.facet_point_ids, prior_root_facets);
      const std::vector<PointId> closed_points =
          covered_point_ids(closed_component.facet_point_ids);
      const std::vector<PointId> prior_root_points =
          covered_point_ids(prior_root_facets);
      delta.added_point_ids = point_set_difference(
          closed_points, prior_root_points);
      delta.fully_redundant =
          delta.added_facet_point_ids.empty() &&
          delta.added_point_ids.empty();
      if (delta.fully_redundant) {
        ++result.counters.fully_redundant_coverage_delta_count;
      }
      group.coverage_delta.emplace(std::move(delta));
      ++result.counters.coverage_delta_count;
    }
    result.groups.push_back(std::move(group));
    ++result.counters.transition_group_classification_count;
  }

  result.transition_groups_exhaustively_classified =
      result.groups.size() ==
          result.gamma_transition.transition_groups.size() &&
      result.counters.transition_group_classification_count ==
          result.groups.size();
  result.strict_components_partitioned_within_groups =
      result.counters.
              transition_group_strict_component_reference_count ==
          result.counters.prior_reduced_root_reference_count +
              result.counters.absorbed_isolated_reference_count;
  result.isolated_facets_deferred_without_reduced_root =
      result.counters.deferred_isolated_facet_group_count ==
      static_cast<std::size_t>(std::count_if(
          result.groups.begin(),
          result.groups.end(),
          [](const ExactReducedGammaBatchGroup& group) {
            return group.kind == ExactReducedGammaBatchGroupKind::
                                     deferred_isolated_facet &&
                   !group.coverage_delta.has_value();
          }));
  result.equal_level_coface_groups_use_reduced_root_count =
      result.counters.coverage_delta_count +
          result.counters.deferred_isolated_facet_group_count ==
      result.groups.size();
  result.coverage_deltas_are_exact_set_differences =
      result.counters.coverage_delta_count ==
      static_cast<std::size_t>(std::count_if(
          result.groups.begin(),
          result.groups.end(),
          [](const ExactReducedGammaBatchGroup& group) {
            return group.coverage_delta.has_value();
          }));

  const bool classifications_certified =
      classifications_are_structurally_certified(result);
  const bool groups_certified =
      groups_are_structurally_certified(result);
  result.strict_components_exhaustively_classified =
      result.strict_components_exhaustively_classified &&
      classifications_certified;
  result.strict_components_partitioned_within_groups =
      result.strict_components_partitioned_within_groups &&
      groups_certified;
  result.isolated_facets_deferred_without_reduced_root =
      result.isolated_facets_deferred_without_reduced_root &&
      groups_certified;
  result.equal_level_coface_groups_use_reduced_root_count =
      result.equal_level_coface_groups_use_reduced_root_count &&
      groups_certified;
  result.coverage_deltas_are_exact_set_differences =
      result.coverage_deltas_are_exact_set_differences &&
      groups_certified;
  result.equal_level_batch_semantics_certified =
      result.gamma_transition_fresh_replay_certified &&
      result.strict_components_exhaustively_classified &&
      result.
          strict_component_incidence_nontriviality_equivalence_certified &&
      result.strict_reduced_roots_exactly_nontrivial_components &&
      result.transition_groups_exhaustively_classified &&
      result.strict_components_partitioned_within_groups &&
      result.isolated_facets_deferred_without_reduced_root &&
      result.equal_level_coface_groups_use_reduced_root_count &&
      result.coverage_deltas_are_exact_set_differences;
  if (!result.equal_level_batch_semantics_certified) {
    throw std::logic_error(
        "the exhaustive Gamma reduction failed its batch semantics proof");
  }
  result.decision = ExactReducedGammaBatchDecision::
      complete_exhaustive_reduced_gamma_batch;
  return result;
}

}  // namespace

ExactReducedGammaBatchVerification verify_exact_reduced_gamma_batch(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget,
    const ExactReducedGammaBatchResult& result) {
  const ExactReducedGammaBatchResult expected =
      compute_exact_reduced_gamma_batch(
          cloud, order, squared_level, budget);
  ExactReducedGammaBatchVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.order == order && result.order == expected.order &&
      result.squared_level == squared_level &&
      result.squared_level == expected.squared_level;
  const ExactGammaTransitionVerification transition_verification =
      verify_exact_gamma_equal_level_transition(
          cloud,
          order,
          squared_level,
          budget,
          result.gamma_transition);
  verification.gamma_transition_certified =
      transition_verification.exact_gamma_transition_decision_certified &&
      result.gamma_transition == expected.gamma_transition;
  verification.strict_component_classifications_certified =
      result.strict_component_classifications ==
      expected.strict_component_classifications;
  verification.groups_certified = result.groups == expected.groups;
  verification.result_facts_certified =
      result.gamma_transition_fresh_replay_certified ==
          expected.gamma_transition_fresh_replay_certified &&
      result.strict_components_exhaustively_classified ==
          expected.strict_components_exhaustively_classified &&
      result.
              strict_component_incidence_nontriviality_equivalence_certified ==
          expected.
              strict_component_incidence_nontriviality_equivalence_certified &&
      result.strict_reduced_roots_exactly_nontrivial_components ==
          expected.strict_reduced_roots_exactly_nontrivial_components &&
      result.transition_groups_exhaustively_classified ==
          expected.transition_groups_exhaustively_classified &&
      result.strict_components_partitioned_within_groups ==
          expected.strict_components_partitioned_within_groups &&
      result.isolated_facets_deferred_without_reduced_root ==
          expected.isolated_facets_deferred_without_reduced_root &&
      result.equal_level_coface_groups_use_reduced_root_count ==
          expected.equal_level_coface_groups_use_reduced_root_count &&
      result.coverage_deltas_are_exact_set_differences ==
          expected.coverage_deltas_are_exact_set_differences &&
      result.equal_level_batch_semantics_certified ==
          expected.equal_level_batch_semantics_certified;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactReducedGammaBatchScope::
                          bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.exact_reduced_gamma_batch_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.gamma_transition_certified &&
      verification.strict_component_classifications_certified &&
      verification.groups_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactReducedGammaBatchResult build_exact_reduced_gamma_batch(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget) {
  ExactReducedGammaBatchResult result =
      compute_exact_reduced_gamma_batch(
          cloud, order, squared_level, budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud,
          order,
          squared_level,
          budget,
          result);
  if (!verification.exact_reduced_gamma_batch_decision_certified) {
    throw std::logic_error(
        "the exact reduced Gamma batch failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
