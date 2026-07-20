#include "morsehgp3d/hierarchy/gamma_transition.hpp"

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

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t value_count)
      : parent_(value_count) {
    for (std::size_t value = 0U; value < value_count; ++value) {
      parent_[value] = value;
    }
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (parent_.at(value) != value) {
      parent_[value] = find(parent_[value]);
    }
    return parent_[value];
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) {
    const std::size_t left_root = find(left);
    const std::size_t right_root = find(right);
    if (left_root == right_root) {
      return false;
    }
    if (left_root < right_root) {
      parent_[right_root] = left_root;
    } else {
      parent_[left_root] = right_root;
    }
    return true;
  }

 private:
  std::vector<std::size_t> parent_;
};

template <typename Function>
void for_each_combination(
    std::size_t point_count,
    std::size_t subset_size,
    Function&& function) {
  FacetLabel combination(subset_size);
  for (std::size_t index = 0U; index < subset_size; ++index) {
    combination[index] = static_cast<PointId>(index);
  }
  while (true) {
    function(combination);
    std::size_t pivot = subset_size;
    while (pivot > 0U) {
      --pivot;
      const PointId maximum_value = static_cast<PointId>(
          point_count - subset_size + pivot);
      if (combination[pivot] != maximum_value) {
        break;
      }
    }
    if (pivot == 0U &&
        combination[pivot] ==
            static_cast<PointId>(point_count - subset_size)) {
      return;
    }
    ++combination[pivot];
    for (std::size_t index = pivot + 1U;
         index < subset_size;
         ++index) {
      combination[index] = combination[index - 1U] + PointId{1};
    }
  }
}

[[nodiscard]] std::vector<FacetLabel> codimension_one_facets(
    std::span<const PointId> coface_point_ids) {
  std::vector<FacetLabel> facets;
  facets.reserve(coface_point_ids.size());
  for (std::size_t omitted = 0U;
       omitted < coface_point_ids.size();
       ++omitted) {
    FacetLabel facet;
    facet.reserve(coface_point_ids.size() - 1U);
    for (std::size_t index = 0U;
         index < coface_point_ids.size();
         ++index) {
      if (index != omitted) {
        facet.push_back(coface_point_ids[index]);
      }
    }
    facets.push_back(std::move(facet));
  }
  std::sort(facets.begin(), facets.end());
  return facets;
}

[[nodiscard]] PointId omitted_point_id(
    std::span<const PointId> coface_point_ids,
    std::span<const PointId> facet_point_ids) {
  for (const PointId point_id : coface_point_ids) {
    if (!std::binary_search(
            facet_point_ids.begin(),
            facet_point_ids.end(),
            point_id)) {
      return point_id;
    }
  }
  throw std::logic_error(
      "a Gamma transition deletion facet omitted no coface point");
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational difference =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance = squared_distance + difference * difference;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] FacetLabel internal_source(std::size_t order) {
  FacetLabel source(order);
  for (std::size_t index = 0U; index < order; ++index) {
    source[index] = static_cast<PointId>(index);
  }
  return source;
}

struct ExactCofaceLevel {
  exact::ExactLevel squared_level;
  std::vector<FacetLabel> facet_point_ids;
  std::optional<ExactStrictGammaElevenPointWitness>
      eleven_point_witness;
};

[[nodiscard]] ExactCofaceLevel build_exact_coface_level(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const FacetLabel& coface,
    const std::map<FacetLabel, ExactFacetMiniballResult>&
        facet_miniballs,
    ExactGammaTransitionCounters& counters) {
  ExactCofaceLevel result;
  result.facet_point_ids = codimension_one_facets(coface);
  if (coface.size() <=
      ExactFacetMiniballResult::maximum_facet_point_count) {
    const ExactFacetMiniballResult coface_miniball =
        build_exact_facet_miniball(cloud, coface);
    ++counters.direct_coface_miniball_build_count;
    result.squared_level = coface_miniball.squared_radius;
    return result;
  }

  if (coface.size() != 11U || order != 10U) {
    throw std::logic_error(
        "an unsupported coface size passed Gamma-transition preflight");
  }
  ++counters.eleven_point_coface_count;
  const ExactFacetMiniballResult* selected = nullptr;
  const FacetLabel* selected_label = nullptr;
  for (const FacetLabel& facet : result.facet_point_ids) {
    const auto found = facet_miniballs.find(facet);
    ++counters.eleven_point_deletion_level_lookup_count;
    if (found == facet_miniballs.end()) {
      throw std::logic_error(
          "an eleven-point Gamma-transition coface lost a deletion facet");
    }
    if (selected == nullptr) {
      selected = &found->second;
      selected_label = &found->first;
    } else {
      ++counters.eleven_point_level_maximum_comparison_count;
      if (found->second.squared_radius > selected->squared_radius) {
        selected = &found->second;
        selected_label = &found->first;
      }
    }
  }
  if (selected == nullptr || selected_label == nullptr) {
    throw std::logic_error(
        "an eleven-point Gamma-transition coface has no deletion facet");
  }

  result.squared_level = selected->squared_radius;
  const PointId omitted = omitted_point_id(coface, *selected_label);
  const exact::ExactLevel omitted_distance = exact_squared_distance(
      selected->center,
      cloud.point(omitted).exact());
  ++counters.eleven_point_omitted_point_distance_evaluation_count;
  if (omitted_distance > result.squared_level) {
    throw std::logic_error(
        "a maximum deletion ball does not cover its omitted point");
  }
  result.eleven_point_witness.emplace(
      ExactStrictGammaElevenPointWitness{
          *selected_label,
          omitted,
          selected->center,
          result.squared_level,
          omitted_distance,
          true,
          true});
  return result;
}

[[nodiscard]] ExactStrictGammaCofaceWitness coface_witness(
    const FacetLabel& coface,
    ExactCofaceLevel level) {
  ExactStrictGammaCofaceWitness witness;
  witness.coface_point_ids = coface;
  witness.squared_level = std::move(level.squared_level);
  witness.facet_point_ids = std::move(level.facet_point_ids);
  witness.eleven_point_witness =
      std::move(level.eleven_point_witness);
  return witness;
}

[[nodiscard]] ExactGammaTransitionGroupKind transition_group_kind(
    std::size_t strict_component_count) {
  if (strict_component_count == 0U) {
    return ExactGammaTransitionGroupKind::
        new_closed_component_without_strict_component;
  }
  if (strict_component_count == 1U) {
    return ExactGammaTransitionGroupKind::
        one_strict_component_continuation;
  }
  return ExactGammaTransitionGroupKind::
      multiple_strict_component_coalescence;
}

[[nodiscard]] ExactGammaTransitionResult compute_exact_gamma_transition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget) {
  if (cloud.size() < 2U ||
      cloud.size() >
          ExactStrictGammaBudget::maximum_supported_point_count ||
      order == 0U ||
      order > ExactStrictGammaBudget::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "a Gamma transition requires 2<=n<=14 and 1<=k<min(n,11)");
  }

  ExactGammaTransitionResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.squared_level = squared_level;
  result.scope = ExactGammaTransitionScope::
      bounded_exhaustive_gamma_equal_level_transition_only;

  const std::vector<FacetLabel> sources{internal_source(order)};
  result.strict_gamma =
      build_exact_strict_gamma_source_classification(
          cloud,
          order,
          squared_level,
          sources,
          budget);
  result.counters.strict_gamma_build_count = 1U;
  result.required_facet_count =
      result.strict_gamma.required_facet_count;
  result.required_coface_count =
      result.strict_gamma.required_coface_count;
  result.required_union_attempt_count =
      result.strict_gamma.required_union_attempt_count;
  result.counters.preflight_count = 1U;
  result.counters.required_facet_count = result.required_facet_count;
  result.counters.required_coface_count = result.required_coface_count;
  result.counters.required_union_attempt_count =
      result.required_union_attempt_count;
  result.candidate_space_size_certified =
      result.strict_gamma.candidate_space_size_certified;

  if (result.strict_gamma.decision ==
      ExactStrictGammaDecision::
          no_cut_preflight_budget_insufficient) {
    result.decision = ExactGammaTransitionDecision::
        no_transition_preflight_budget_insufficient;
    return result;
  }
  if (!result.candidate_space_size_certified ||
      !result.strict_gamma.strict_open_cut_certified ||
      !result.strict_gamma.full_pi0_isolated_facets_included ||
      !result.strict_gamma.exhaustive_active_catalog_certified ||
      !result.strict_gamma.complete_source_classification_certified) {
    throw std::logic_error(
        "the embedded strict Gamma cut is not exhaustive");
  }

  std::map<FacetLabel, ExactFacetMiniballResult> facet_miniballs;
  std::vector<ExactStrictGammaFacetWitness> replayed_strict_facets;
  for_each_combination(
      cloud.size(),
      order,
      [&](const FacetLabel& facet) {
        ExactFacetMiniballResult miniball =
            build_exact_facet_miniball(cloud, facet);
        ++result.counters.enumerated_facet_count;
        ++result.counters.facet_miniball_build_count;
        ++result.counters.facet_level_comparison_count;
        if (miniball.squared_radius < squared_level) {
          replayed_strict_facets.push_back(
              ExactStrictGammaFacetWitness{
                  facet, miniball.squared_radius});
          ++result.counters.strict_facet_replay_count;
        } else if (miniball.squared_radius == squared_level) {
          result.equal_level_facets.push_back(
              ExactGammaEqualLevelFacetWitness{
                  facet, miniball.squared_radius});
          ++result.counters.equal_level_facet_count;
        }
        const auto [position, inserted] = facet_miniballs.emplace(
            facet, std::move(miniball));
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "the Gamma-transition facet enumeration repeated a label");
        }
      });
  if (result.counters.enumerated_facet_count !=
          result.required_facet_count ||
      replayed_strict_facets != result.strict_gamma.active_facets) {
    throw std::logic_error(
        "the Gamma-transition facet replay disagrees with the strict cut");
  }

  std::vector<ExactStrictGammaCofaceWitness>
      replayed_strict_cofaces;
  for_each_combination(
      cloud.size(),
      order + 1U,
      [&](const FacetLabel& coface) {
        ++result.counters.enumerated_coface_count;
        ExactCofaceLevel coface_level = build_exact_coface_level(
            cloud,
            order,
            coface,
            facet_miniballs,
            result.counters);
        ++result.counters.coface_level_comparison_count;
        if (coface_level.squared_level < squared_level) {
          replayed_strict_cofaces.push_back(
              coface_witness(coface, std::move(coface_level)));
          ++result.counters.strict_coface_replay_count;
        } else if (coface_level.squared_level == squared_level) {
          result.equal_level_cofaces.push_back(
              coface_witness(coface, std::move(coface_level)));
          ++result.counters.equal_level_coface_count;
        }
      });
  if (result.counters.enumerated_coface_count !=
          result.required_coface_count ||
      replayed_strict_cofaces != result.strict_gamma.active_cofaces) {
    throw std::logic_error(
        "the Gamma-transition coface replay disagrees with the strict cut");
  }
  result.strict_open_cut_fresh_replay_certified = true;
  result.equal_level_catalog_exhaustive_certified = true;

  std::vector<FacetLabel> closed_facets;
  closed_facets.reserve(
      result.strict_gamma.active_facets.size() +
      result.equal_level_facets.size());
  for (const ExactStrictGammaFacetWitness& facet :
       result.strict_gamma.active_facets) {
    closed_facets.push_back(facet.facet_point_ids);
  }
  for (const ExactGammaEqualLevelFacetWitness& facet :
       result.equal_level_facets) {
    closed_facets.push_back(facet.facet_point_ids);
  }
  std::sort(closed_facets.begin(), closed_facets.end());
  if (std::adjacent_find(
          closed_facets.begin(), closed_facets.end()) !=
      closed_facets.end()) {
    throw std::logic_error(
        "the strict and equal Gamma facets overlap");
  }
  result.counters.closed_facet_count = closed_facets.size();
  result.counters.closed_coface_count =
      result.strict_gamma.active_cofaces.size() +
      result.equal_level_cofaces.size();

  std::map<FacetLabel, std::size_t> closed_facet_indices;
  for (std::size_t index = 0U;
       index < closed_facets.size();
       ++index) {
    closed_facet_indices.emplace(closed_facets[index], index);
  }
  CanonicalDisjointSet closed_disjoint_set(closed_facets.size());
  result.counters.closed_disjoint_set_value_count =
      closed_facets.size();

  const auto apply_closed_coface =
      [&](const ExactStrictGammaCofaceWitness& coface) {
        std::vector<std::size_t> indices;
        indices.reserve(coface.facet_point_ids.size());
        for (const FacetLabel& facet : coface.facet_point_ids) {
          ++result.counters.closed_coface_facet_lookup_count;
          const auto found = closed_facet_indices.find(facet);
          if (found == closed_facet_indices.end()) {
            throw std::logic_error(
                "a closed Gamma coface references an inactive facet");
          }
          indices.push_back(found->second);
        }
        for (std::size_t index = 1U;
             index < indices.size();
             ++index) {
          ++result.counters.closed_union_attempt_count;
          if (closed_disjoint_set.unite(
                  indices.front(), indices[index])) {
            ++result.counters.closed_union_merge_count;
          }
        }
      };
  for (const ExactStrictGammaCofaceWitness& coface :
       result.strict_gamma.active_cofaces) {
    apply_closed_coface(coface);
  }
  for (const ExactStrictGammaCofaceWitness& coface :
       result.equal_level_cofaces) {
    apply_closed_coface(coface);
  }
  const std::size_t expected_closed_union_attempt_count =
      order * result.counters.closed_coface_count;
  const std::size_t expected_closed_facet_lookup_count =
      (order + 1U) * result.counters.closed_coface_count;
  if (result.counters.closed_union_attempt_count !=
          expected_closed_union_attempt_count ||
      result.counters.closed_coface_facet_lookup_count !=
          expected_closed_facet_lookup_count ||
      result.counters.closed_union_attempt_count >
          result.required_union_attempt_count) {
    throw std::logic_error(
        "the closed Gamma transition disagrees with its exact work bound");
  }

  std::map<std::size_t, std::vector<FacetLabel>> grouped_facets;
  for (std::size_t index = 0U;
       index < closed_facets.size();
       ++index) {
    grouped_facets[closed_disjoint_set.find(index)].push_back(
        closed_facets[index]);
  }
  for (auto& [root, facets] : grouped_facets) {
    static_cast<void>(root);
    std::sort(facets.begin(), facets.end());
    result.closed_components.push_back(
        ExactStrictGammaComponentWitness{
            facets.front(), std::move(facets)});
  }
  std::sort(
      result.closed_components.begin(),
      result.closed_components.end(),
      [](const ExactStrictGammaComponentWitness& left,
         const ExactStrictGammaComponentWitness& right) {
        return left.canonical_representative_facet_point_ids <
               right.canonical_representative_facet_point_ids;
      });
  result.counters.closed_component_count =
      result.closed_components.size();

  std::map<FacetLabel, std::size_t> closed_component_by_facet;
  for (std::size_t component_index = 0U;
       component_index < result.closed_components.size();
       ++component_index) {
    for (const FacetLabel& facet :
         result.closed_components[component_index].facet_point_ids) {
      if (!closed_component_by_facet
               .emplace(facet, component_index)
               .second) {
        throw std::logic_error(
            "a closed Gamma facet belongs to multiple components");
      }
    }
  }

  std::map<FacetLabel, std::size_t> strict_component_by_facet;
  result.strict_component_to_closed_component_index.reserve(
      result.strict_gamma.components.size());
  for (std::size_t strict_index = 0U;
       strict_index < result.strict_gamma.components.size();
       ++strict_index) {
    const ExactStrictGammaComponentWitness& strict_component =
        result.strict_gamma.components[strict_index];
    const auto representative = closed_component_by_facet.find(
        strict_component.canonical_representative_facet_point_ids);
    if (representative == closed_component_by_facet.end()) {
      throw std::logic_error(
          "a strict Gamma component disappeared from the closed cut");
    }
    for (const FacetLabel& facet : strict_component.facet_point_ids) {
      const auto closed = closed_component_by_facet.find(facet);
      if (closed == closed_component_by_facet.end() ||
          closed->second != representative->second ||
          !strict_component_by_facet
               .emplace(facet, strict_index)
               .second) {
        throw std::logic_error(
            "a strict Gamma component does not refine one closed component");
      }
    }
    result.strict_component_to_closed_component_index.push_back(
        representative->second);
    ++result.counters.strict_component_projection_count;
  }

  std::set<FacetLabel> equal_level_facet_labels;
  for (const ExactGammaEqualLevelFacetWitness& facet :
       result.equal_level_facets) {
    if (!equal_level_facet_labels.insert(facet.facet_point_ids).second) {
      throw std::logic_error(
          "the equal-level Gamma facet catalogue repeated a label");
    }
  }
  for (const ExactStrictGammaCofaceWitness& coface :
       result.equal_level_cofaces) {
    for (const FacetLabel& facet : coface.facet_point_ids) {
      const auto strict = strict_component_by_facet.find(facet);
      const bool newly_active =
          equal_level_facet_labels.contains(facet);
      if ((strict != strict_component_by_facet.end()) ==
          newly_active) {
        throw std::logic_error(
            "an equal-level coface facet has no unique frozen-cut token");
      }
      ExactGammaTransitionIncidence incidence;
      incidence.coface_point_ids = coface.coface_point_ids;
      incidence.facet_point_ids = facet;
      if (strict != strict_component_by_facet.end()) {
        incidence.strict_component_index = strict->second;
      }
      incidence.newly_active_at_level = newly_active;
      result.equal_level_incidences.push_back(std::move(incidence));
      ++result.counters.equal_level_incidence_count;
    }
  }
  result.equal_level_incidences_tokenized =
      result.counters.equal_level_incidence_count ==
      (order + 1U) * result.equal_level_cofaces.size();
  if (!result.equal_level_incidences_tokenized) {
    throw std::logic_error(
        "the equal-level Gamma incidences are not completely tokenized");
  }

  std::map<std::size_t, ExactGammaTransitionGroup>
      group_by_closed_component;
  const auto ensure_group =
      [&](std::size_t closed_component_index)
          -> ExactGammaTransitionGroup& {
        auto [position, inserted] =
            group_by_closed_component.try_emplace(
                closed_component_index);
        if (inserted) {
          position->second.closed_component_index =
              closed_component_index;
          position->second.
              canonical_representative_facet_point_ids =
              result.closed_components[closed_component_index]
                  .canonical_representative_facet_point_ids;
        }
        return position->second;
      };

  for (const ExactGammaEqualLevelFacetWitness& facet :
       result.equal_level_facets) {
    const auto closed = closed_component_by_facet.find(
        facet.facet_point_ids);
    if (closed == closed_component_by_facet.end()) {
      throw std::logic_error(
          "an equal-level facet is absent from the closed cut");
    }
    ensure_group(closed->second)
        .newly_active_facet_point_ids.push_back(
            facet.facet_point_ids);
    ++result.counters.equal_level_facet_projection_count;
  }
  for (const ExactStrictGammaCofaceWitness& coface :
       result.equal_level_cofaces) {
    if (coface.facet_point_ids.empty()) {
      throw std::logic_error(
          "an equal-level coface has no deletion facets");
    }
    const auto first = closed_component_by_facet.find(
        coface.facet_point_ids.front());
    if (first == closed_component_by_facet.end()) {
      throw std::logic_error(
          "an equal-level coface is absent from the closed cut");
    }
    for (const FacetLabel& facet : coface.facet_point_ids) {
      const auto closed = closed_component_by_facet.find(facet);
      if (closed == closed_component_by_facet.end() ||
          closed->second != first->second) {
        throw std::logic_error(
            "an equal-level coface spans multiple closed components");
      }
    }
    ensure_group(first->second)
        .equal_level_coface_point_ids.push_back(
            coface.coface_point_ids);
    ++result.counters.equal_level_coface_projection_count;
  }

  result.transition_groups.reserve(group_by_closed_component.size());
  for (auto& [closed_component_index, group] :
       group_by_closed_component) {
    std::set<std::size_t> strict_indices;
    for (const FacetLabel& facet :
         result.closed_components[closed_component_index]
             .facet_point_ids) {
      const auto strict = strict_component_by_facet.find(facet);
      if (strict != strict_component_by_facet.end()) {
        strict_indices.insert(strict->second);
      }
    }
    group.strict_component_indices.assign(
        strict_indices.begin(), strict_indices.end());
    std::sort(
        group.newly_active_facet_point_ids.begin(),
        group.newly_active_facet_point_ids.end());
    std::sort(
        group.equal_level_coface_point_ids.begin(),
        group.equal_level_coface_point_ids.end());
    group.kind = transition_group_kind(
        group.strict_component_indices.size());
    switch (group.kind) {
      case ExactGammaTransitionGroupKind::
          new_closed_component_without_strict_component:
        ++result.counters.new_component_group_count;
        break;
      case ExactGammaTransitionGroupKind::
          one_strict_component_continuation:
        ++result.counters.continuation_group_count;
        break;
      case ExactGammaTransitionGroupKind::
          multiple_strict_component_coalescence:
        ++result.counters.coalescence_group_count;
        break;
    }
    result.transition_groups.push_back(std::move(group));
  }
  result.counters.transition_group_count =
      result.transition_groups.size();

  result.closed_cut_exhaustive_certified = true;
  result.strict_partition_refines_closed_partition =
      result.counters.strict_component_projection_count ==
      result.strict_gamma.components.size();
  result.equal_level_batch_applied_simultaneously = true;
  result.transition_groups_partition_equal_level_changes =
      result.counters.equal_level_facet_projection_count ==
          result.equal_level_facets.size() &&
      result.counters.equal_level_coface_projection_count ==
          result.equal_level_cofaces.size() &&
      result.counters.transition_group_count ==
          result.counters.new_component_group_count +
              result.counters.continuation_group_count +
              result.counters.coalescence_group_count;
  if (!result.candidate_space_size_certified ||
      !result.strict_open_cut_fresh_replay_certified ||
      !result.equal_level_catalog_exhaustive_certified ||
      !result.equal_level_incidences_tokenized ||
      !result.closed_cut_exhaustive_certified ||
      !result.strict_partition_refines_closed_partition ||
      !result.equal_level_batch_applied_simultaneously ||
      !result.transition_groups_partition_equal_level_changes) {
    throw std::logic_error(
        "the Gamma transition failed its strict-to-closed partition proof");
  }
  result.decision = ExactGammaTransitionDecision::
      complete_exhaustive_open_to_closed_transition;
  return result;
}

}  // namespace

ExactGammaTransitionVerification
verify_exact_gamma_equal_level_transition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget,
    const ExactGammaTransitionResult& result) {
  const ExactGammaTransitionResult expected =
      compute_exact_gamma_transition(
          cloud, order, squared_level, budget);
  ExactGammaTransitionVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.order == order &&
      result.order == expected.order &&
      result.squared_level == squared_level &&
      result.squared_level == expected.squared_level;
  verification.preflight_counts_certified =
      result.required_facet_count == expected.required_facet_count &&
      result.required_coface_count == expected.required_coface_count &&
      result.required_union_attempt_count ==
          expected.required_union_attempt_count &&
      result.candidate_space_size_certified ==
          expected.candidate_space_size_certified;

  const std::vector<FacetLabel> sources{internal_source(order)};
  const ExactStrictGammaVerification strict_verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          order,
          squared_level,
          sources,
          budget,
          result.strict_gamma);
  verification.strict_gamma_certified =
      strict_verification.exact_strict_gamma_decision_certified;
  verification.equal_level_facets_certified =
      result.equal_level_facets == expected.equal_level_facets;
  verification.equal_level_cofaces_certified =
      result.equal_level_cofaces == expected.equal_level_cofaces;
  verification.equal_level_incidences_certified =
      result.equal_level_incidences ==
      expected.equal_level_incidences;
  verification.closed_components_certified =
      result.closed_components == expected.closed_components;
  verification.strict_component_projection_certified =
      result.strict_component_to_closed_component_index ==
      expected.strict_component_to_closed_component_index;
  verification.transition_groups_certified =
      result.transition_groups == expected.transition_groups;
  verification.result_facts_certified =
      result.strict_open_cut_fresh_replay_certified ==
          expected.strict_open_cut_fresh_replay_certified &&
      result.equal_level_catalog_exhaustive_certified ==
          expected.equal_level_catalog_exhaustive_certified &&
      result.equal_level_incidences_tokenized ==
          expected.equal_level_incidences_tokenized &&
      result.closed_cut_exhaustive_certified ==
          expected.closed_cut_exhaustive_certified &&
      result.strict_partition_refines_closed_partition ==
          expected.strict_partition_refines_closed_partition &&
      result.equal_level_batch_applied_simultaneously ==
          expected.equal_level_batch_applied_simultaneously &&
      result.transition_groups_partition_equal_level_changes ==
          expected.transition_groups_partition_equal_level_changes;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactGammaTransitionScope::
          bounded_exhaustive_gamma_equal_level_transition_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.exact_gamma_transition_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.preflight_counts_certified &&
      verification.strict_gamma_certified &&
      verification.equal_level_facets_certified &&
      verification.equal_level_cofaces_certified &&
      verification.equal_level_incidences_certified &&
      verification.closed_components_certified &&
      verification.strict_component_projection_certified &&
      verification.transition_groups_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactGammaTransitionResult build_exact_gamma_equal_level_transition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget) {
  ExactGammaTransitionResult result = compute_exact_gamma_transition(
      cloud, order, squared_level, budget);
  const ExactGammaTransitionVerification verification =
      verify_exact_gamma_equal_level_transition(
          cloud,
          order,
          squared_level,
          budget,
          result);
  if (!verification.exact_gamma_transition_decision_certified) {
    throw std::logic_error(
        "the exact Gamma transition failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
