#include "morsehgp3d/hierarchy/gamma.hpp"

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

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t n,
    std::size_t k) {
  if (k > n) {
    return 0U;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    value = value * (n - k + index) / index;
  }
  return value;
}

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
      "a Gamma deletion facet omitted no coface point");
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

void validate_budget(const ExactStrictGammaBudget& budget) {
  if (budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::
              maximum_supported_union_attempt_count) {
    throw std::invalid_argument(
        "an exact strict-Gamma budget exceeds its reference cap");
  }
}

[[nodiscard]] std::vector<FacetLabel> validate_sources(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    std::span<const FacetLabel> canonical_source_facets) {
  if (canonical_source_facets.empty() ||
      canonical_source_facets.size() >
          ExactStrictGammaBudget::
              maximum_supported_source_facet_count) {
    throw std::invalid_argument(
        "strict Gamma requires one to four source facets");
  }
  std::vector<FacetLabel> sources(
      canonical_source_facets.begin(),
      canonical_source_facets.end());
  for (const FacetLabel& source : sources) {
    if (source.size() != order ||
        !std::is_sorted(source.begin(), source.end()) ||
        std::adjacent_find(source.begin(), source.end()) != source.end()) {
      throw std::invalid_argument(
          "a strict-Gamma source must be a canonical order-k facet");
    }
    for (const PointId point_id : source) {
      static_cast<void>(cloud.point(point_id));
    }
  }
  std::vector<FacetLabel> sorted_sources = sources;
  std::sort(sorted_sources.begin(), sorted_sources.end());
  if (std::adjacent_find(
          sorted_sources.begin(), sorted_sources.end()) !=
      sorted_sources.end()) {
    throw std::invalid_argument(
        "strict-Gamma source facets must be distinct");
  }
  return sources;
}

[[nodiscard]] bool budget_covers_preflight(
    const ExactStrictGammaBudget& budget,
    std::size_t required_facet_count,
    std::size_t required_coface_count,
    std::size_t required_union_attempt_count) {
  return budget.maximum_enumerated_facet_count >=
             required_facet_count &&
         budget.maximum_enumerated_coface_count >=
             required_coface_count &&
         budget.maximum_union_attempt_count >=
             required_union_attempt_count;
}

[[nodiscard]] ExactStrictGammaResult compute_exact_strict_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& strict_cut_squared_level,
    std::span<const FacetLabel> canonical_source_facets,
    ExactStrictGammaBudget budget) {
  validate_budget(budget);
  if (cloud.size() < 2U ||
      cloud.size() >
          ExactStrictGammaBudget::maximum_supported_point_count ||
      order == 0U ||
      order > ExactStrictGammaBudget::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "strict Gamma requires 2<=n<=14 and 1<=k<min(n,11)");
  }
  const std::vector<FacetLabel> sources = validate_sources(
      cloud, order, canonical_source_facets);

  ExactStrictGammaResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.strict_cut_squared_level = strict_cut_squared_level;
  result.source_facet_point_ids = sources;
  result.required_facet_count = bounded_binomial(cloud.size(), order);
  result.required_coface_count =
      bounded_binomial(cloud.size(), order + 1U);
  result.required_union_attempt_count =
      order * result.required_coface_count;
  result.counters.preflight_count = 1U;
  result.counters.required_facet_count = result.required_facet_count;
  result.counters.required_coface_count = result.required_coface_count;
  result.counters.required_union_attempt_count =
      result.required_union_attempt_count;
  result.candidate_space_size_certified = true;
  result.scope = ExactStrictGammaScope::
      bounded_exhaustive_strict_gamma_full_pi0_source_components_only;

  if (!budget_covers_preflight(
          budget,
          result.required_facet_count,
          result.required_coface_count,
          result.required_union_attempt_count)) {
    result.decision = ExactStrictGammaDecision::
        no_cut_preflight_budget_insufficient;
    return result;
  }

  std::map<FacetLabel, ExactFacetMiniballResult> facet_miniballs;
  for_each_combination(
      cloud.size(),
      order,
      [&](const FacetLabel& facet) {
        ExactFacetMiniballResult miniball =
            build_exact_facet_miniball(cloud, facet);
        ++result.counters.enumerated_facet_count;
        ++result.counters.facet_miniball_build_count;
        ++result.counters.facet_strict_level_comparison_count;
        if (miniball.squared_radius < strict_cut_squared_level) {
          result.active_facets.push_back(
              ExactStrictGammaFacetWitness{
                  facet, miniball.squared_radius});
          ++result.counters.active_facet_count;
        }
        const auto [position, inserted] = facet_miniballs.emplace(
            facet, std::move(miniball));
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "the exhaustive Gamma facet enumeration repeated a label");
        }
      });
  if (result.counters.enumerated_facet_count !=
      result.required_facet_count) {
    throw std::logic_error(
        "the exhaustive Gamma facet count missed its preflight size");
  }

  std::map<FacetLabel, std::size_t> active_facet_indices;
  for (std::size_t index = 0U;
       index < result.active_facets.size();
       ++index) {
    active_facet_indices.emplace(
        result.active_facets[index].facet_point_ids, index);
  }
  CanonicalDisjointSet disjoint_set(result.active_facets.size());
  result.counters.disjoint_set_value_count =
      result.active_facets.size();

  for_each_combination(
      cloud.size(),
      order + 1U,
      [&](const FacetLabel& coface) {
        ++result.counters.enumerated_coface_count;
        const std::vector<FacetLabel> facets =
            codimension_one_facets(coface);
        exact::ExactLevel coface_level;
        std::optional<ExactStrictGammaElevenPointWitness>
            eleven_point_witness;
        if (coface.size() <=
            ExactFacetMiniballResult::maximum_facet_point_count) {
          const ExactFacetMiniballResult coface_miniball =
              build_exact_facet_miniball(cloud, coface);
          ++result.counters.direct_coface_miniball_build_count;
          coface_level = coface_miniball.squared_radius;
        } else {
          if (coface.size() != 11U || order != 10U) {
            throw std::logic_error(
                "an unsupported coface size passed strict-Gamma preflight");
          }
          ++result.counters.eleven_point_coface_count;
          const ExactFacetMiniballResult* selected = nullptr;
          const FacetLabel* selected_label = nullptr;
          for (const FacetLabel& facet : facets) {
            const auto found = facet_miniballs.find(facet);
            ++result.counters.
                eleven_point_deletion_level_lookup_count;
            if (found == facet_miniballs.end()) {
              throw std::logic_error(
                  "an eleven-point coface lost a deletion facet");
            }
            if (selected == nullptr) {
              selected = &found->second;
              selected_label = &found->first;
            } else {
              ++result.counters.
                  eleven_point_level_maximum_comparison_count;
              if (found->second.squared_radius >
                  selected->squared_radius) {
                selected = &found->second;
                selected_label = &found->first;
              }
            }
          }
          if (selected == nullptr || selected_label == nullptr) {
            throw std::logic_error(
                "an eleven-point coface has no deletion facet");
          }
          coface_level = selected->squared_radius;
          const PointId omitted = omitted_point_id(
              coface, *selected_label);
          const exact::ExactLevel omitted_distance =
              exact_squared_distance(
                  selected->center,
                  cloud.point(omitted).exact());
          ++result.counters.
              eleven_point_omitted_point_distance_evaluation_count;
          const bool covers_omitted =
              omitted_distance <= coface_level;
          if (!covers_omitted) {
            throw std::logic_error(
                "a maximum deletion ball does not cover its omitted point");
          }
          eleven_point_witness.emplace(
              ExactStrictGammaElevenPointWitness{
                  *selected_label,
                  omitted,
                  selected->center,
                  coface_level,
                  omitted_distance,
                  true,
                  true});
        }

        ++result.counters.coface_strict_level_comparison_count;
        if (!(coface_level < strict_cut_squared_level)) {
          return;
        }
        ++result.counters.active_coface_count;
        ExactStrictGammaCofaceWitness witness;
        witness.coface_point_ids = coface;
        witness.squared_level = coface_level;
        witness.facet_point_ids = facets;
        witness.eleven_point_witness =
            std::move(eleven_point_witness);

        std::vector<std::size_t> active_indices;
        active_indices.reserve(facets.size());
        for (const FacetLabel& facet : facets) {
          ++result.counters.coface_facet_lookup_count;
          const auto found = active_facet_indices.find(facet);
          if (found == active_facet_indices.end()) {
            throw std::logic_error(
                "an active Gamma coface references an inactive facet");
          }
          active_indices.push_back(found->second);
        }
        for (std::size_t index = 1U;
             index < active_indices.size();
             ++index) {
          ++result.counters.union_attempt_count;
          if (disjoint_set.unite(
                  active_indices.front(), active_indices[index])) {
            ++result.counters.union_merge_count;
          }
        }
        result.active_cofaces.push_back(std::move(witness));
      });
  if (result.counters.enumerated_coface_count !=
      result.required_coface_count ||
      result.counters.union_attempt_count >
          result.required_union_attempt_count) {
    throw std::logic_error(
        "the exhaustive Gamma coface work violated preflight");
  }

  std::map<std::size_t, std::vector<FacetLabel>> grouped_facets;
  for (std::size_t index = 0U;
       index < result.active_facets.size();
       ++index) {
    grouped_facets[disjoint_set.find(index)].push_back(
        result.active_facets[index].facet_point_ids);
  }
  for (auto& [root, facets] : grouped_facets) {
    static_cast<void>(root);
    std::sort(facets.begin(), facets.end());
    ExactStrictGammaComponentWitness component;
    component.canonical_representative_facet_point_ids =
        facets.front();
    component.facet_point_ids = std::move(facets);
    if (component.facet_point_ids.size() == 1U) {
      ++result.counters.isolated_component_count;
    }
    result.components.push_back(std::move(component));
  }
  std::sort(
      result.components.begin(),
      result.components.end(),
      [](const ExactStrictGammaComponentWitness& left,
         const ExactStrictGammaComponentWitness& right) {
        return left.canonical_representative_facet_point_ids <
               right.canonical_representative_facet_point_ids;
      });
  result.counters.component_count = result.components.size();

  std::map<FacetLabel, std::size_t> component_by_facet;
  for (std::size_t component_index = 0U;
       component_index < result.components.size();
       ++component_index) {
    for (const FacetLabel& facet :
         result.components[component_index].facet_point_ids) {
      component_by_facet.emplace(facet, component_index);
    }
  }

  result.all_sources_active_and_classified = true;
  for (const FacetLabel& source : sources) {
    ++result.counters.source_lookup_count;
    const auto source_miniball = facet_miniballs.find(source);
    if (source_miniball == facet_miniballs.end()) {
      throw std::logic_error(
          "a validated Gamma source is absent from facet enumeration");
    }
    ExactStrictGammaSourceClassification classification;
    classification.source_facet_point_ids = source;
    classification.squared_level =
        source_miniball->second.squared_radius;
    classification.active_strictly_below_cut =
        classification.squared_level < strict_cut_squared_level;
    if (classification.active_strictly_below_cut) {
      const auto component = component_by_facet.find(source);
      if (component == component_by_facet.end()) {
        throw std::logic_error(
            "an active Gamma source is absent from all components");
      }
      classification.component_index = component->second;
      ++result.counters.active_source_count;
    } else {
      result.all_sources_active_and_classified = false;
      ++result.counters.inactive_source_count;
    }
    result.source_classifications.push_back(
        std::move(classification));
  }

  result.strict_open_cut_certified = true;
  result.full_pi0_isolated_facets_included = true;
  result.exhaustive_active_catalog_certified = true;
  result.complete_source_classification_certified = true;
  result.decision = result.all_sources_active_and_classified
                        ? ExactStrictGammaDecision::
                              complete_all_sources_active_and_classified
                        : ExactStrictGammaDecision::
                              complete_with_inactive_sources;
  return result;
}

}  // namespace

ExactStrictGammaVerification
verify_exact_strict_gamma_source_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& strict_cut_squared_level,
    std::span<const std::vector<PointId>> canonical_source_facets,
    ExactStrictGammaBudget budget,
    const ExactStrictGammaResult& result) {
  const ExactStrictGammaResult expected = compute_exact_strict_gamma(
      cloud,
      order,
      strict_cut_squared_level,
      canonical_source_facets,
      budget);
  ExactStrictGammaVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.order == order &&
      result.order == expected.order &&
      result.strict_cut_squared_level == strict_cut_squared_level &&
      result.strict_cut_squared_level ==
          expected.strict_cut_squared_level &&
      result.source_facet_point_ids ==
          expected.source_facet_point_ids;
  verification.preflight_counts_certified =
      result.required_facet_count == expected.required_facet_count &&
      result.required_coface_count == expected.required_coface_count &&
      result.required_union_attempt_count ==
          expected.required_union_attempt_count &&
      result.candidate_space_size_certified ==
          expected.candidate_space_size_certified;
  verification.active_facets_certified =
      result.active_facets == expected.active_facets;
  verification.active_cofaces_certified =
      result.active_cofaces == expected.active_cofaces;
  verification.components_certified =
      result.components == expected.components;
  verification.source_classifications_certified =
      result.source_classifications ==
          expected.source_classifications;
  verification.result_facts_certified =
      result.strict_open_cut_certified ==
          expected.strict_open_cut_certified &&
      result.full_pi0_isolated_facets_included ==
          expected.full_pi0_isolated_facets_included &&
      result.exhaustive_active_catalog_certified ==
          expected.exhaustive_active_catalog_certified &&
      result.complete_source_classification_certified ==
          expected.complete_source_classification_certified &&
      result.all_sources_active_and_classified ==
          expected.all_sources_active_and_classified;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactStrictGammaScope::
          bounded_exhaustive_strict_gamma_full_pi0_source_components_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.exact_strict_gamma_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.preflight_counts_certified &&
      verification.active_facets_certified &&
      verification.active_cofaces_certified &&
      verification.components_certified &&
      verification.source_classifications_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactStrictGammaResult build_exact_strict_gamma_source_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& strict_cut_squared_level,
    std::span<const std::vector<PointId>> canonical_source_facets,
    ExactStrictGammaBudget budget) {
  ExactStrictGammaResult result = compute_exact_strict_gamma(
      cloud,
      order,
      strict_cut_squared_level,
      canonical_source_facets,
      budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          order,
          strict_cut_squared_level,
          canonical_source_facets,
          budget,
          result);
  if (!verification.exact_strict_gamma_decision_certified) {
    throw std::logic_error(
        "the exact strict-Gamma result failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
