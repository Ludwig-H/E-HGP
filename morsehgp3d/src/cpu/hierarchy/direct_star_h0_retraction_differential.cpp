#include "morsehgp3d/hierarchy/direct_star_h0_retraction_differential.hpp"

#include "morsehgp3d/hierarchy/facet_miniball.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using Facet = std::vector<PointId>;
using Facets = std::vector<Facet>;

struct Coface {
  Facet point_ids;
  Facets facets;
  exact::ExactLevel squared_level;
  bool retained{false};
  bool direct{false};
  bool first_incidence_candidate{false};
};

struct Component {
  Facets facets;
  Facets core_facets;
  std::vector<PointId> covered_point_ids;
};

struct Snapshot {
  std::vector<Component> components;
  bool every_component_meets_core{true};
  std::size_t core_facet_count{};
  std::size_t covered_point_reference_count{};
};

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t size) : parent_(size), rank_(size, 0U) {
    for (std::size_t index = 0U; index < size; ++index) {
      parent_[index] = index;
    }
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[value] != value) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  void unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return;
    }
    if (rank_[left] < rank_[right]) {
      std::swap(left, right);
    }
    parent_[right] = left;
    if (rank_[left] == rank_[right]) {
      ++rank_[left];
    }
  }

 private:
  std::vector<std::size_t> parent_;
  std::vector<std::uint8_t> rank_;
};

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) { text(domain); }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
      if (value > static_cast<std::size_t>(
                      std::numeric_limits<std::uint64_t>::max())) {
        throw std::length_error("a retraction audit size exceeds uint64");
      }
    }
    u64(static_cast<std::uint64_t>(value));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void level(const exact::ExactLevel& value) {
    text(value.numerator_string());
    text(value.denominator_string());
  }

  void point_ids(std::span<const PointId> point_ids) {
    size(point_ids.size());
    for (const PointId point_id : point_ids) {
      u64(point_id);
    }
  }

  void facets(const Facets& facets_value) {
    size(facets_value.size());
    for (const Facet& facet : facets_value) {
      point_ids(facet);
    }
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

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

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool bounded_binomial(
    std::size_t point_count,
    std::size_t subset_size,
    std::size_t& result) noexcept {
  if (subset_size > point_count) {
    result = 0U;
    return true;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    if (!checked_multiply(
            value,
            point_count - subset_size + factor,
            value)) {
      return false;
    }
    value /= factor;
  }
  result = value;
  return true;
}

[[nodiscard]] Facets canonical_facets(Facets facets) {
  for (Facet& facet : facets) {
    std::sort(facet.begin(), facet.end());
    if (std::adjacent_find(facet.begin(), facet.end()) != facet.end()) {
      throw std::invalid_argument("a retraction facet repeats a PointId");
    }
  }
  std::sort(facets.begin(), facets.end());
  facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
  return facets;
}

[[nodiscard]] Facet reconstructed_star_coface(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    std::size_t coface_index) {
  const auto reconstructed =
      reconstruct_exact_direct_sparse_successive_incidence_star_coface(
          star, coface_index);
  return {
      reconstructed.point_ids.begin(),
      reconstructed.point_ids.begin() +
          static_cast<std::ptrdiff_t>(reconstructed.point_count)};
}

[[nodiscard]] bool active_at(
    const exact::ExactLevel& object_level,
    const exact::ExactLevel& cut_level,
    ExactDirectStarH0RetractionCutSide side) {
  if (side == ExactDirectStarH0RetractionCutSide::strict) {
    return object_level < cut_level;
  }
  return object_level <= cut_level;
}

[[nodiscard]] std::vector<PointId> covered_points(const Facets& facets) {
  std::vector<PointId> points;
  for (const Facet& facet : facets) {
    points.insert(points.end(), facet.begin(), facet.end());
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
  return points;
}

[[nodiscard]] bool facet_intersects_component(
    const Facet& facet,
    const Component& component) {
  return std::binary_search(
      component.facets.begin(), component.facets.end(), facet);
}

[[nodiscard]] Snapshot build_snapshot(
    const std::vector<Coface>& cofaces,
    const Facets& core_facets,
    const exact::ExactLevel& cut_level,
    ExactDirectStarH0RetractionCutSide side) {
  std::map<Facet, std::size_t> facet_indices;
  for (const Coface& coface : cofaces) {
    if (!active_at(coface.squared_level, cut_level, side)) {
      continue;
    }
    for (const Facet& facet : coface.facets) {
      facet_indices.try_emplace(facet, facet_indices.size());
    }
  }

  DisjointSet dsu(facet_indices.size());
  for (const Coface& coface : cofaces) {
    if (!active_at(coface.squared_level, cut_level, side) ||
        coface.facets.empty()) {
      continue;
    }
    const std::size_t pivot = facet_indices.at(coface.facets.front());
    for (std::size_t index = 1U; index < coface.facets.size(); ++index) {
      dsu.unite(pivot, facet_indices.at(coface.facets[index]));
    }
  }

  std::map<std::size_t, Facets> grouped_facets;
  for (const auto& [facet, index] : facet_indices) {
    grouped_facets[dsu.find(index)].push_back(facet);
  }

  Snapshot snapshot;
  snapshot.components.reserve(grouped_facets.size());
  for (auto& [unused_root, facets] : grouped_facets) {
    static_cast<void>(unused_root);
    Component component;
    component.facets = canonical_facets(std::move(facets));
    std::set_intersection(
        component.facets.begin(),
        component.facets.end(),
        core_facets.begin(),
        core_facets.end(),
        std::back_inserter(component.core_facets));
    component.covered_point_ids = covered_points(component.facets);
    snapshot.every_component_meets_core =
        snapshot.every_component_meets_core &&
        !component.core_facets.empty();
    snapshot.core_facet_count += component.core_facets.size();
    snapshot.covered_point_reference_count +=
        component.covered_point_ids.size();
    snapshot.components.push_back(std::move(component));
  }
  std::sort(
      snapshot.components.begin(),
      snapshot.components.end(),
      [](const Component& left, const Component& right) {
        if (left.core_facets != right.core_facets) {
          return left.core_facets < right.core_facets;
        }
        return left.covered_point_ids < right.covered_point_ids;
      });
  return snapshot;
}

[[nodiscard]] contract::CanonicalId partition_digest(
    const Snapshot& snapshot,
    std::size_t order,
    const exact::ExactLevel& level,
    ExactDirectStarH0RetractionCutSide side) {
  DigestWriter writer{
      "MorseHGP3D/DirectStarH0RetractionDifferential/partition/v1"};
  writer.size(order);
  writer.level(level);
  writer.u64(side == ExactDirectStarH0RetractionCutSide::closed ? 1U : 0U);
  writer.size(snapshot.components.size());
  for (const Component& component : snapshot.components) {
    writer.facets(component.core_facets);
    writer.point_ids(component.covered_point_ids);
  }
  return writer.finalize();
}

[[nodiscard]] bool snapshots_equal(
    const Snapshot& gamma,
    const Snapshot& retained) {
  if (!gamma.every_component_meets_core ||
      !retained.every_component_meets_core ||
      gamma.components.size() != retained.components.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < gamma.components.size(); ++index) {
    if (gamma.components[index].core_facets !=
            retained.components[index].core_facets ||
        gamma.components[index].covered_point_ids !=
            retained.components[index].covered_point_ids) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectStarH0RetractionAction action_for(
    std::size_t q_r) noexcept {
  if (q_r == 0U) {
    return ExactDirectStarH0RetractionAction::reduced_birth;
  }
  if (q_r == 1U) {
    return ExactDirectStarH0RetractionAction::continuation;
  }
  return ExactDirectStarH0RetractionAction::multifusion;
}

struct Group {
  Facets core_facets;
  Facets added_core_facets;
  std::vector<PointId> added_point_ids;
  std::vector<const Component*> parents;
  std::size_t retained_new_coface_count{};
  std::size_t omitted_new_coface_count{};
  std::size_t first_incidence_candidate_new_coface_count{};
  std::size_t late_star_omitted_new_coface_count{};
};

[[nodiscard]] std::vector<Group> build_groups(
    const std::vector<Coface>& cofaces,
    const Snapshot& before,
    const Snapshot& after,
    const exact::ExactLevel& level) {
  std::vector<Group> groups;
  for (const Component& component : after.components) {
    Group group;
    for (const Coface& coface : cofaces) {
      if (coface.squared_level != level || coface.facets.empty() ||
          !facet_intersects_component(coface.facets.front(), component)) {
        continue;
      }
      if (coface.retained) {
        ++group.retained_new_coface_count;
      } else {
        ++group.omitted_new_coface_count;
      }
      if (coface.first_incidence_candidate) {
        ++group.first_incidence_candidate_new_coface_count;
      } else if (coface.retained) {
        ++group.late_star_omitted_new_coface_count;
      }
    }
    if (group.retained_new_coface_count == 0U &&
        group.omitted_new_coface_count == 0U) {
      continue;
    }
    group.core_facets = component.core_facets;
    Facets prior_core_facets;
    std::vector<PointId> prior_points;
    for (const Component& prior : before.components) {
      const bool contained = std::any_of(
          prior.facets.begin(),
          prior.facets.end(),
          [&component](const Facet& facet) {
            return std::binary_search(
                component.facets.begin(), component.facets.end(), facet);
          });
      if (!contained) {
        continue;
      }
      group.parents.push_back(&prior);
      prior_core_facets.insert(
          prior_core_facets.end(),
          prior.core_facets.begin(),
          prior.core_facets.end());
      prior_points.insert(
          prior_points.end(),
          prior.covered_point_ids.begin(),
          prior.covered_point_ids.end());
    }
    prior_core_facets = canonical_facets(std::move(prior_core_facets));
    std::sort(prior_points.begin(), prior_points.end());
    prior_points.erase(
        std::unique(prior_points.begin(), prior_points.end()),
        prior_points.end());
    std::set_difference(
        component.core_facets.begin(),
        component.core_facets.end(),
        prior_core_facets.begin(),
        prior_core_facets.end(),
        std::back_inserter(group.added_core_facets));
    std::set_difference(
        component.covered_point_ids.begin(),
        component.covered_point_ids.end(),
        prior_points.begin(),
        prior_points.end(),
        std::back_inserter(group.added_point_ids));
    groups.push_back(std::move(group));
  }
  std::sort(groups.begin(), groups.end(), [](const Group& left, const Group& right) {
    return left.core_facets < right.core_facets;
  });
  return groups;
}

[[nodiscard]] contract::CanonicalId parent_partition_digest(
    const std::vector<const Component*>& parents,
    std::size_t order,
    const exact::ExactLevel& level) {
  std::vector<const Component*> canonical = parents;
  std::sort(
      canonical.begin(),
      canonical.end(),
      [](const Component* left, const Component* right) {
        if (left->core_facets != right->core_facets) {
          return left->core_facets < right->core_facets;
        }
        return left->covered_point_ids < right->covered_point_ids;
      });
  DigestWriter writer{
      "MorseHGP3D/DirectStarH0RetractionDifferential/parents/v1"};
  writer.size(order);
  writer.level(level);
  writer.size(canonical.size());
  for (const Component* component : canonical) {
    writer.facets(component->core_facets);
    writer.point_ids(component->covered_point_ids);
  }
  return writer.finalize();
}

[[nodiscard]] bool plan_verification_closes(
    const ExactDirectSparseUnifiedLevelPlanVerification& verification) {
  return verification.observed_storage_within_budget &&
         verification.source_star_freshly_verified &&
         verification.expected_result_freshly_reconstructed &&
         verification.observed_recursively_equal &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
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

[[nodiscard]] exact::ExactLevel exact_squared_diameter(
    const spatial::CanonicalPointCloud& cloud,
    ExactDirectStarH0RetractionDifferentialCounters& counters) {
  exact::ExactLevel diameter_squared;
  for (std::size_t left = 0U; left < cloud.size(); ++left) {
    for (std::size_t right = left + 1U; right < cloud.size(); ++right) {
      const exact::ExactLevel distance = exact_squared_distance(
          cloud.point(static_cast<PointId>(left)).exact(),
          cloud.point(static_cast<PointId>(right)).exact());
      ++counters.diameter_pair_distance_evaluation_count;
      if (distance > diameter_squared) {
        diameter_squared = distance;
      }
    }
  }
  if (diameter_squared == exact::ExactLevel{}) {
    throw std::logic_error(
        "a duplicate-free retraction cloud has zero exact diameter");
  }
  return diameter_squared;
}

[[nodiscard]] bool high_cut_gamma_closes(
    const ExactStrictGammaResult& gamma,
    std::size_t facet_count,
    std::size_t coface_count) {
  return gamma.decision ==
             ExactStrictGammaDecision::
                 complete_all_sources_active_and_classified &&
         gamma.candidate_space_size_certified &&
         gamma.strict_open_cut_certified &&
         gamma.full_pi0_isolated_facets_included &&
         gamma.exhaustive_active_catalog_certified &&
         gamma.complete_source_classification_certified &&
         gamma.all_sources_active_and_classified &&
         gamma.active_facets.size() == facet_count &&
         gamma.active_cofaces.size() == coface_count;
}

[[nodiscard]] bool high_cut_gamma_verification_closes(
    const ExactStrictGammaVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.preflight_counts_certified &&
         verification.active_facets_certified &&
         verification.active_cofaces_certified &&
         verification.components_certified &&
         verification.source_classifications_certified &&
         verification.result_facts_certified &&
         verification.counters_certified && verification.decision_certified &&
         verification.scope_certified && verification.fresh_replay_certified &&
         verification.exact_strict_gamma_decision_certified;
}

[[nodiscard]] bool preflight_covers(
    std::size_t point_count,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget,
    std::size_t& facet_count,
    std::size_t& coface_count,
    std::size_t& partition_observation_limit,
    std::size_t& logical_output_limit) noexcept {
  if (point_count <
          ExactDirectStarH0RetractionDifferentialResult::
              minimum_supported_point_count ||
      point_count >
          ExactDirectStarH0RetractionDifferentialResult::
              maximum_supported_point_count ||
      order < ExactDirectStarH0RetractionDifferentialResult::
                  minimum_supported_order ||
      order > ExactDirectStarH0RetractionDifferentialResult::
                  maximum_supported_order ||
      order >= point_count ||
      !bounded_binomial(point_count, order, facet_count) ||
      !bounded_binomial(point_count, order + 1U, coface_count)) {
    return false;
  }
  std::size_t regularity_label_limit = 0U;
  std::size_t checkpoint_limit = 0U;
  std::size_t first_incidence_reference_limit = 0U;
  if (!checked_add(facet_count, coface_count, regularity_label_limit) ||
      !checked_multiply(2U, coface_count, checkpoint_limit) ||
      !checked_multiply(
          facet_count,
          point_count - order,
          first_incidence_reference_limit) ||
      budget.maximum_checkpoint_count < checkpoint_limit ||
      budget.maximum_batch_group_audit_count < coface_count ||
      budget.maximum_omitted_coface_audit_count < coface_count ||
      budget.maximum_first_incidence_facet_audit_count < facet_count ||
      budget.maximum_first_incidence_cominimizer_reference_count <
          first_incidence_reference_limit ||
      budget.maximum_late_star_coface_audit_count < coface_count ||
      budget.maximum_regularity_label_count < regularity_label_limit ||
      budget.gamma_budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.gamma_budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.gamma_budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count ||
      budget.gamma_budget.maximum_enumerated_facet_count < facet_count ||
      budget.gamma_budget.maximum_enumerated_coface_count < coface_count) {
    return false;
  }
  std::size_t required_union_count = 0U;
  if (!checked_multiply(order, coface_count, required_union_count) ||
      budget.gamma_budget.maximum_union_attempt_count < required_union_count ||
      !checked_multiply(3U, coface_count, partition_observation_limit) ||
      !checked_multiply(
          partition_observation_limit,
          facet_count,
          partition_observation_limit) ||
      budget.maximum_partition_component_observation_count <
          partition_observation_limit) {
    return false;
  }
  std::size_t per_coface = 0U;
  std::size_t checkpoint_output_limit = 0U;
  std::size_t first_incidence_output_limit = 0U;
  std::size_t per_first_incidence_facet = 0U;
  if (!checked_multiply(12U, point_count, per_coface) ||
      !checked_add(per_coface, 32U, per_coface) ||
      !checked_multiply(coface_count, per_coface, logical_output_limit) ||
      !checked_multiply(2U, coface_count, checkpoint_output_limit) ||
      !checked_add(logical_output_limit, checkpoint_output_limit,
                   logical_output_limit) ||
      !checked_multiply(
          point_count - order,
          order + 1U,
          per_first_incidence_facet) ||
      !checked_add(
          per_first_incidence_facet,
          order + 4U,
          per_first_incidence_facet) ||
      !checked_multiply(
          facet_count,
          per_first_incidence_facet,
          first_incidence_output_limit) ||
      !checked_add(
          logical_output_limit,
          first_incidence_output_limit,
          logical_output_limit) ||
      budget.maximum_logical_output_entry_count < logical_output_limit) {
    return false;
  }
  return true;
}

[[nodiscard]] ExactDirectStarH0RetractionDifferentialResult base_result(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget) {
  ExactDirectStarH0RetractionDifferentialResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope =
      ExactDirectStarH0RetractionDifferentialScope::
          bounded_n9_orders2_and3_regular_direct_star_to_exhaustive_gamma_h0_only;
  result.counters.preflight_count = 1U;
  result.bounded_gamma_oracle_persisted = false;
  result.exhaustive_gamma_faceted_payload_reproduced = false;
  result.contract_v2_identity_equivalence_claimed = false;
  result.forest_semantics_exact_claimed = false;
  result.public_status_claimed = false;
  return result;
}

[[nodiscard]] bool label_regular(
    const spatial::CanonicalPointCloud& cloud,
    const Facet& label,
    ExactDirectStarH0RetractionDifferentialCounters& counters) {
  const ExactFacetMiniballResult miniball =
      build_exact_facet_miniball(cloud, label);
  ++counters.regularity_miniball_build_count;
  const ExactFacetMiniballVerification verification =
      verify_exact_facet_miniball(cloud, label, miniball);
  ++counters.regularity_miniball_verification_count;
  if (!verification.local_exact_facet_miniball_certified ||
      miniball.status !=
          ExactFacetMiniballStatus::exact_facet_miniball_certified ||
      miniball.scope != ExactFacetMiniballScope::local_facet_miniball_only ||
      miniball.squared_radius.numerator() <= 0 ||
      miniball.counters.optimal_support_count != 1U ||
      miniball.support_point_ids.size() < 2U ||
      miniball.boundary_point_ids != miniball.support_point_ids ||
      miniball.strictly_inside_point_ids.size() +
              miniball.support_point_ids.size() !=
          label.size()) {
    return false;
  }
  const auto global = spatial::brute_force_closed_ball(
      cloud, miniball.center, miniball.squared_radius);
  ++counters.regularity_closed_ball_query_count;
  return global.partition_complete() && global.validated_for(cloud) &&
         std::vector<PointId>(global.shell_ids().begin(),
                              global.shell_ids().end()) ==
             miniball.support_point_ids;
}

[[nodiscard]] bool coface_is_non_gabriel(
    const spatial::CanonicalPointCloud& cloud,
    const Coface& coface) {
  const ExactFacetMiniballResult miniball =
      build_exact_facet_miniball(cloud, coface.point_ids);
  const auto global = spatial::brute_force_closed_ball(
      cloud, miniball.center, miniball.squared_radius);
  return std::any_of(
      global.interior_ids().begin(),
      global.interior_ids().end(),
      [&coface](PointId point_id) {
        return !std::binary_search(
            coface.point_ids.begin(), coface.point_ids.end(), point_id);
      });
}

[[nodiscard]] std::vector<PointId> coface_added_points_from_strict_roots(
    const Coface& coface,
    const Snapshot& strict,
    std::size_t& prior_root_count,
    std::size_t& newly_attached_facet_count) {
  std::set<std::size_t> roots;
  newly_attached_facet_count = 0U;
  for (const Facet& facet : coface.facets) {
    bool found = false;
    for (std::size_t index = 0U; index < strict.components.size(); ++index) {
      if (facet_intersects_component(facet, strict.components[index])) {
        roots.insert(index);
        found = true;
      }
    }
    if (!found) {
      ++newly_attached_facet_count;
    }
  }
  prior_root_count = roots.size();
  std::vector<PointId> prior_points;
  for (const std::size_t root : roots) {
    prior_points.insert(
        prior_points.end(),
        strict.components[root].covered_point_ids.begin(),
        strict.components[root].covered_point_ids.end());
  }
  std::sort(prior_points.begin(), prior_points.end());
  prior_points.erase(
      std::unique(prior_points.begin(), prior_points.end()),
      prior_points.end());
  std::vector<PointId> added;
  std::set_difference(
      coface.point_ids.begin(),
      coface.point_ids.end(),
      prior_points.begin(),
      prior_points.end(),
      std::back_inserter(added));
  return added;
}

}  // namespace

bool ExactDirectStarH0RetractionDifferentialResult::certified_differential()
    const noexcept {
  return schema_version ==
             direct_star_h0_retraction_differential_schema_version &&
         point_count >= minimum_supported_point_count &&
         point_count <= maximum_supported_point_count &&
         order >= minimum_supported_order && order <= maximum_supported_order &&
         order < point_count && candidate_space_preflight_certified &&
         source_plan_freshly_replayed && gamma_high_cut_freshly_built &&
         gamma_high_cut_freshly_verified &&
         gamma_high_cut_equals_twice_exact_squared_diameter &&
         regularity_gate_certified &&
         direct_catalog_is_global_core_for_order &&
         retained_cofaces_equal_exact_star_of_direct_core &&
         every_direct_core_facet_has_complete_first_incidence_cominimizers &&
         first_incidence_subflow_equals_direct_union_complete_cominimizers &&
         every_late_star_omission_has_strictly_earlier_core_first_incidence &&
         every_late_star_omission_is_qr1_zero_point_continuation &&
         every_omitted_coface_is_qr1_zero_point_continuation &&
         strict_and_closed_core_partitions_equal_at_every_exact_level &&
         covered_point_unions_equal_at_every_exact_level &&
         qr_actions_parents_and_point_deltas_equal_for_core_groups &&
         first_incidence_subflow_strict_and_closed_core_partitions_equal &&
         first_incidence_subflow_covered_point_unions_equal &&
         first_incidence_subflow_qr_actions_parents_and_point_deltas_equal &&
         every_missing_first_incidence_group_is_certified_noop &&
         every_missing_retained_star_group_is_certified_noop &&
         conditional_first_incidence_subflow_h0_retraction_certified &&
         conditional_reduced_h0_forest_semantics_equivalent &&
         conditional_horizontal_h0_retraction_certified &&
         bounded_gamma_oracle_materialized_transiently &&
         !bounded_gamma_oracle_persisted &&
         !exhaustive_gamma_faceted_payload_reproduced &&
         !contract_v2_identity_equivalence_claimed &&
         !contract_v2_batch_identity_equivalence_claimed &&
         !contract_v2_forest_identity_equivalence_claimed &&
         !forest_semantics_exact_claimed &&
         !incidence_complete_reduction_proved &&
         !universal_product_completeness_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectStarH0RetractionDifferentialDecision::
                 complete_bounded_conditional_direct_star_h0_retraction_differential &&
         scope ==
             ExactDirectStarH0RetractionDifferentialScope::
                 bounded_n9_orders2_and3_regular_direct_star_to_exhaustive_gamma_h0_only;
}

ExactDirectStarH0RetractionDifferentialResult
build_exact_direct_star_h0_retraction_differential(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget) {
  auto result = base_result(cloud, order, budget);
  std::size_t facet_count = 0U;
  std::size_t coface_count = 0U;
  std::size_t partition_observation_limit = 0U;
  std::size_t logical_output_limit = 0U;
  if (!preflight_covers(
          cloud.size(),
          order,
          budget,
          facet_count,
          coface_count,
          partition_observation_limit,
          logical_output_limit)) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_preflight_budget_insufficient;
    return result;
  }
  result.candidate_space_preflight_certified = true;

  const auto plan_verification = verify_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      source_incidence_journal,
      source_star_budget,
      source_star_traversal_order,
      source_star,
      source_plan_budget,
      source_plan);
  ++result.counters.source_plan_verification_count;
  if (!source_star.certified_bounded_star() ||
      !source_plan.certified_bounded_plan() ||
      !plan_verification_closes(plan_verification)) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_source_pipeline_not_certified;
    return result;
  }
  result.source_plan_freshly_replayed = true;
  result.canonical_cloud_digest =
      source_plan.source_higher_canonical_cloud_digest;

  std::set<std::size_t> retained_star_indices;
  std::set<std::size_t> direct_star_indices;
  bool plan_order_consistent = true;
  for (const auto& batch : source_plan.batches) {
    if (batch.order != order) {
      continue;
    }
    for (std::size_t local = 0U; local < batch.direct_reference_count;
         ++local) {
      const auto& reference = source_plan.direct_references[
          batch.direct_reference_offset + local];
      if (reference.role != ExactDirectMorseH0Role::saddle) {
        continue;
      }
      if (!reference.source_star_direct_coface_index.has_value()) {
        plan_order_consistent = false;
        continue;
      }
      const std::size_t star_index =
          *reference.source_star_direct_coface_index;
      if (star_index >= source_star.cofaces.size() ||
          source_star.cofaces[star_index].squared_level !=
              batch.squared_level ||
          reconstructed_star_coface(source_star, star_index).size() !=
              order + 1U ||
          !retained_star_indices.insert(star_index).second) {
        plan_order_consistent = false;
      }
      direct_star_indices.insert(star_index);
    }
    for (std::size_t local = 0U; local < batch.residual_reference_count;
         ++local) {
      const auto& reference = source_plan.residual_references[
          batch.residual_reference_offset + local];
      const std::size_t star_index = reference.source_star_coface_index;
      if (star_index >= source_star.cofaces.size() ||
          source_star.cofaces[star_index].squared_level !=
              batch.squared_level ||
          reconstructed_star_coface(source_star, star_index).size() !=
              order + 1U ||
          !retained_star_indices.insert(star_index).second) {
        plan_order_consistent = false;
      }
    }
  }
  std::size_t target_star_count = 0U;
  for (const auto& star_coface : source_star.cofaces) {
    if (reconstructed_star_coface(
            source_star, star_coface.coface_index)
            .size() == order + 1U) {
      ++target_star_count;
    }
  }
  if (!plan_order_consistent || retained_star_indices.size() != target_star_count ||
      direct_star_indices.empty()) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_source_plan_order_inconsistent;
    return result;
  }

  result.exact_diameter_squared =
      exact_squared_diameter(cloud, result.counters);
  result.exhaustive_gamma_high_cut_squared_level = exact::ExactLevel{
      result.exact_diameter_squared.rational() *
      exact::ExactRational{exact::BigInt{2}}};
  result.gamma_high_cut_equals_twice_exact_squared_diameter =
      result.exhaustive_gamma_high_cut_squared_level.rational() ==
      result.exact_diameter_squared.rational() *
          exact::ExactRational{exact::BigInt{2}};
  Facet first_source(order);
  for (std::size_t index = 0U; index < order; ++index) {
    first_source[index] = static_cast<PointId>(index);
  }
  const std::vector<Facet> high_cut_sources{std::move(first_source)};
  const auto gamma_high_cut =
      build_exact_strict_gamma_source_classification(
          cloud,
          order,
          result.exhaustive_gamma_high_cut_squared_level,
          high_cut_sources,
          budget.gamma_budget);
  ++result.counters.high_cut_gamma_build_count;
  const auto gamma_high_cut_verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          order,
          result.exhaustive_gamma_high_cut_squared_level,
          high_cut_sources,
          budget.gamma_budget,
          gamma_high_cut);
  ++result.counters.high_cut_gamma_verification_count;
  if (!result.gamma_high_cut_equals_twice_exact_squared_diameter ||
      !high_cut_gamma_closes(gamma_high_cut, facet_count, coface_count) ||
      !high_cut_gamma_verification_closes(gamma_high_cut_verification)) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_gamma_high_cut_not_certified;
    return result;
  }
  result.gamma_high_cut_freshly_built = true;
  result.gamma_high_cut_freshly_verified = true;
  result.bounded_gamma_oracle_materialized_transiently = true;

  std::vector<Coface> gamma_cofaces;
  gamma_cofaces.reserve(gamma_high_cut.active_cofaces.size());
  std::map<Facet, std::size_t> gamma_index_by_key;
  std::vector<exact::ExactLevel> activation_levels;
  activation_levels.reserve(gamma_high_cut.active_cofaces.size());
  for (const auto& witness : gamma_high_cut.active_cofaces) {
    Coface coface;
    coface.point_ids = witness.coface_point_ids;
    coface.facets = canonical_facets(witness.facet_point_ids);
    coface.squared_level = witness.squared_level;
    gamma_index_by_key.emplace(coface.point_ids, gamma_cofaces.size());
    activation_levels.push_back(coface.squared_level);
    gamma_cofaces.push_back(std::move(coface));
  }
  std::sort(activation_levels.begin(), activation_levels.end());
  activation_levels.erase(
      std::unique(activation_levels.begin(), activation_levels.end()),
      activation_levels.end());
  result.counters.exhaustive_gamma_coface_count = gamma_cofaces.size();
  if (gamma_cofaces.size() != coface_count ||
      gamma_index_by_key.size() != gamma_cofaces.size()) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_source_plan_order_inconsistent;
    return result;
  }

  std::vector<Coface> retained_cofaces;
  retained_cofaces.reserve(retained_star_indices.size());
  for (const std::size_t star_index : retained_star_indices) {
    const Facet key = reconstructed_star_coface(source_star, star_index);
    const auto gamma_found = gamma_index_by_key.find(key);
    if (gamma_found == gamma_index_by_key.end() ||
        gamma_cofaces[gamma_found->second].squared_level !=
            source_star.cofaces[star_index].squared_level) {
      result.decision =
          ExactDirectStarH0RetractionDifferentialDecision::
              falsified_direct_star_of_core;
      return result;
    }
    Coface retained = gamma_cofaces[gamma_found->second];
    retained.retained = true;
    retained.direct = direct_star_indices.contains(star_index);
    gamma_cofaces[gamma_found->second].retained = true;
    gamma_cofaces[gamma_found->second].direct = retained.direct;
    retained_cofaces.push_back(std::move(retained));
  }
  std::sort(
      retained_cofaces.begin(),
      retained_cofaces.end(),
      [](const Coface& left, const Coface& right) {
        return left.point_ids < right.point_ids;
      });
  result.counters.retained_star_coface_count = retained_cofaces.size();

  Facets core_facets;
  for (const Coface& coface : retained_cofaces) {
    if (!coface.direct) {
      continue;
    }
    ++result.counters.direct_core_coface_count;
    core_facets.insert(
        core_facets.end(), coface.facets.begin(), coface.facets.end());
  }
  core_facets = canonical_facets(std::move(core_facets));
  result.counters.direct_core_facet_count = core_facets.size();
  result.direct_catalog_is_global_core_for_order = !core_facets.empty();

  std::set<Facet> expected_star_keys;
  for (const Coface& coface : gamma_cofaces) {
    const bool touches_core = std::any_of(
        coface.facets.begin(), coface.facets.end(), [&core_facets](const Facet& facet) {
          return std::binary_search(core_facets.begin(), core_facets.end(), facet);
        });
    if (touches_core) {
      expected_star_keys.insert(coface.point_ids);
    }
  }
  std::set<Facet> observed_star_keys;
  for (const Coface& coface : retained_cofaces) {
    observed_star_keys.insert(coface.point_ids);
  }
  result.retained_cofaces_equal_exact_star_of_direct_core =
      expected_star_keys == observed_star_keys;
  if (!result.direct_catalog_is_global_core_for_order ||
      !result.retained_cofaces_equal_exact_star_of_direct_core) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_direct_star_of_core;
    return result;
  }

  std::map<Facet, exact::ExactLevel> first_incidence_by_core_facet;
  std::set<Facet> first_incidence_candidate_keys;
  for (const Coface& coface : retained_cofaces) {
    if (coface.direct) {
      first_incidence_candidate_keys.insert(coface.point_ids);
    }
  }
  bool first_incidence_complete = true;
  for (const Facet& core_facet : core_facets) {
    ExactDirectCoreFirstIncidenceAudit audit;
    audit.first_incidence_audit_index = result.first_incidence_audits.size();
    audit.direct_core_facet_point_ids = core_facet;
    bool found_incidence = false;
    for (const Coface& coface : retained_cofaces) {
      if (!std::binary_search(
              coface.facets.begin(), coface.facets.end(), core_facet)) {
        continue;
      }
      if (!found_incidence ||
          coface.squared_level < audit.first_incidence_squared_level) {
        audit.first_incidence_squared_level = coface.squared_level;
        audit.complete_cominimizer_coface_point_ids.clear();
        audit.complete_cominimizer_coface_point_ids.push_back(
            coface.point_ids);
        found_incidence = true;
      } else if (coface.squared_level ==
                 audit.first_incidence_squared_level) {
        audit.complete_cominimizer_coface_point_ids.push_back(
            coface.point_ids);
      }
    }
    std::sort(
        audit.complete_cominimizer_coface_point_ids.begin(),
        audit.complete_cominimizer_coface_point_ids.end());
    const auto duplicate = std::adjacent_find(
        audit.complete_cominimizer_coface_point_ids.begin(),
        audit.complete_cominimizer_coface_point_ids.end());
    audit.exact_star_incidence_nonempty = found_incidence;
    audit.complete_cominimizers_canonical =
        found_incidence &&
        duplicate == audit.complete_cominimizer_coface_point_ids.end();
    if (found_incidence) {
      first_incidence_by_core_facet.emplace(
          core_facet, audit.first_incidence_squared_level);
      first_incidence_candidate_keys.insert(
          audit.complete_cominimizer_coface_point_ids.begin(),
          audit.complete_cominimizer_coface_point_ids.end());
    }
    audit.every_cominimizer_retained_in_subflow =
        found_incidence &&
        std::all_of(
            audit.complete_cominimizer_coface_point_ids.begin(),
            audit.complete_cominimizer_coface_point_ids.end(),
            [&first_incidence_candidate_keys](const Facet& coface) {
              return first_incidence_candidate_keys.contains(coface);
            });
    first_incidence_complete =
        first_incidence_complete && audit.exact_star_incidence_nonempty &&
        audit.complete_cominimizers_canonical &&
        audit.every_cominimizer_retained_in_subflow;
    result.counters.first_incidence_cominimizer_reference_count +=
        audit.complete_cominimizer_coface_point_ids.size();
    result.first_incidence_audits.push_back(std::move(audit));
  }
  result.counters.first_incidence_facet_count =
      result.first_incidence_audits.size();
  result.every_direct_core_facet_has_complete_first_incidence_cominimizers =
      first_incidence_complete &&
      result.first_incidence_audits.size() == core_facets.size() &&
      first_incidence_by_core_facet.size() == core_facets.size();
  if (!result
           .every_direct_core_facet_has_complete_first_incidence_cominimizers ||
      result.first_incidence_audits.size() >
          budget.maximum_first_incidence_facet_audit_count ||
      result.counters.first_incidence_cominimizer_reference_count >
          budget.maximum_first_incidence_cominimizer_reference_count) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_core_first_incidence_completion;
    return result;
  }

  std::vector<Coface> first_incidence_cofaces;
  first_incidence_cofaces.reserve(first_incidence_candidate_keys.size());
  for (Coface& coface : gamma_cofaces) {
    coface.first_incidence_candidate =
        first_incidence_candidate_keys.contains(coface.point_ids);
    if (coface.first_incidence_candidate) {
      first_incidence_cofaces.push_back(coface);
    }
  }
  for (Coface& coface : retained_cofaces) {
    coface.first_incidence_candidate =
        first_incidence_candidate_keys.contains(coface.point_ids);
  }
  std::sort(
      first_incidence_cofaces.begin(),
      first_incidence_cofaces.end(),
      [](const Coface& left, const Coface& right) {
        return left.point_ids < right.point_ids;
      });
  result.counters.first_incidence_subflow_coface_count =
      first_incidence_cofaces.size();
  const bool every_direct_retained = std::all_of(
      retained_cofaces.begin(),
      retained_cofaces.end(),
      [](const Coface& coface) {
        return !coface.direct || coface.first_incidence_candidate;
      });
  const std::set<Facet> observed_first_incidence_keys = [&]() {
    std::set<Facet> keys;
    for (const Coface& coface : first_incidence_cofaces) {
      keys.insert(coface.point_ids);
    }
    return keys;
  }();
  result.first_incidence_subflow_equals_direct_union_complete_cominimizers =
      every_direct_retained &&
      observed_first_incidence_keys == first_incidence_candidate_keys;
  if (!result
           .first_incidence_subflow_equals_direct_union_complete_cominimizers) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_core_first_incidence_completion;
    return result;
  }

  std::set<Facet> regularity_labels;
  for (const Coface& coface : gamma_cofaces) {
    regularity_labels.insert(coface.point_ids);
    regularity_labels.insert(coface.facets.begin(), coface.facets.end());
  }
  if (regularity_labels.size() > budget.maximum_regularity_label_count) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_preflight_budget_insufficient;
    return result;
  }
  bool regular = true;
  for (const Facet& label : regularity_labels) {
    regular = regular && label_regular(cloud, label, result.counters);
  }
  result.regularity_gate_certified = regular;
  if (!regular) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            unsupported_regularity_gate;
    return result;
  }

  bool late_star_strictly_preceded = true;
  bool late_star_silent = true;
  for (const Coface& coface : retained_cofaces) {
    if (coface.first_incidence_candidate) {
      continue;
    }
    ExactDirectLateStarCofaceAudit audit;
    audit.late_star_coface_audit_index =
        result.late_star_coface_audits.size();
    audit.coface_point_ids = coface.point_ids;
    audit.squared_level = coface.squared_level;
    audit.retained_in_exact_direct_star = true;
    audit.non_direct_non_gabriel_certified =
        !coface.direct && coface_is_non_gabriel(cloud, coface);
    audit.omitted_from_first_incidence_subflow = true;
    audit.every_touched_core_first_incidence_strictly_lower = true;
    for (const Facet& facet : coface.facets) {
      const auto first_incidence =
          first_incidence_by_core_facet.find(facet);
      if (first_incidence == first_incidence_by_core_facet.end()) {
        continue;
      }
      audit.touched_direct_core_facet_point_ids.push_back(facet);
      audit.touched_direct_core_first_incidence_squared_levels.push_back(
          first_incidence->second);
      audit.every_touched_core_first_incidence_strictly_lower =
          audit.every_touched_core_first_incidence_strictly_lower &&
          first_incidence->second < coface.squared_level;
    }
    audit.every_touched_core_first_incidence_strictly_lower =
        audit.every_touched_core_first_incidence_strictly_lower &&
        !audit.touched_direct_core_facet_point_ids.empty();
    const Snapshot strict = build_snapshot(
        first_incidence_cofaces,
        core_facets,
        coface.squared_level,
        ExactDirectStarH0RetractionCutSide::strict);
    audit.added_point_ids = coface_added_points_from_strict_roots(
        coface,
        strict,
        audit.strict_prior_root_count,
        audit.newly_attached_facet_count);
    audit.continuation_q_r_one = audit.strict_prior_root_count == 1U;
    audit.point_delta_empty = audit.added_point_ids.empty();
    late_star_strictly_preceded =
        late_star_strictly_preceded &&
        audit.non_direct_non_gabriel_certified &&
        audit.every_touched_core_first_incidence_strictly_lower;
    late_star_silent = late_star_silent && audit.continuation_q_r_one &&
                       audit.point_delta_empty;
    result.late_star_coface_audits.push_back(std::move(audit));
  }
  result.counters.late_star_omitted_coface_count =
      result.late_star_coface_audits.size();
  result.counters.late_star_silent_continuation_count =
      static_cast<std::size_t>(std::count_if(
          result.late_star_coface_audits.begin(),
          result.late_star_coface_audits.end(),
          [](const auto& audit) {
            return audit.continuation_q_r_one && audit.point_delta_empty;
          }));
  result
      .every_late_star_omission_has_strictly_earlier_core_first_incidence =
      late_star_strictly_preceded;
  result.every_late_star_omission_is_qr1_zero_point_continuation =
      late_star_silent;
  if (!late_star_strictly_preceded || !late_star_silent ||
      result.late_star_coface_audits.size() >
          budget.maximum_late_star_coface_audit_count) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_late_star_coface_silence;
    return result;
  }

  bool omissions_silent = true;
  for (const Coface& coface : gamma_cofaces) {
    if (coface.retained) {
      continue;
    }
    ExactDirectStarH0RetractionOmittedCofaceAudit audit;
    audit.omitted_coface_audit_index = result.omitted_coface_audits.size();
    audit.coface_point_ids = coface.point_ids;
    audit.squared_level = coface.squared_level;
    audit.absent_from_direct_star = true;
    audit.non_gabriel_certified = coface_is_non_gabriel(cloud, coface);
    const Snapshot strict = build_snapshot(
        gamma_cofaces,
        core_facets,
        coface.squared_level,
        ExactDirectStarH0RetractionCutSide::strict);
    audit.added_point_ids = coface_added_points_from_strict_roots(
        coface,
        strict,
        audit.strict_prior_root_count,
        audit.newly_attached_facet_count);
    audit.continuation_q_r_one = audit.strict_prior_root_count == 1U;
    audit.point_delta_empty = audit.added_point_ids.empty();
    omissions_silent = omissions_silent && audit.non_gabriel_certified &&
                       audit.continuation_q_r_one && audit.point_delta_empty;
    result.omitted_coface_audits.push_back(std::move(audit));
  }
  result.counters.omitted_gamma_coface_count =
      result.omitted_coface_audits.size();
  result.counters.omitted_silent_continuation_count =
      static_cast<std::size_t>(std::count_if(
          result.omitted_coface_audits.begin(),
          result.omitted_coface_audits.end(),
          [](const auto& audit) {
            return audit.continuation_q_r_one && audit.point_delta_empty;
          }));
  result.every_omitted_coface_is_qr1_zero_point_continuation =
      omissions_silent;
  result.sparse_publication_requires_new_contract_revision =
      !result.omitted_coface_audits.empty() ||
      !result.late_star_coface_audits.empty();
  if (!omissions_silent) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_omitted_coface_silence;
    return result;
  }

  bool all_partitions_equal = true;
  bool all_point_unions_equal = true;
  bool all_group_semantics_equal = true;
  bool all_first_incidence_partitions_equal = true;
  bool all_first_incidence_point_unions_equal = true;
  bool all_first_incidence_group_semantics_equal = true;
  bool every_missing_retained_group_is_noop = true;
  bool every_missing_first_incidence_group_is_noop = true;
  for (const exact::ExactLevel& level : activation_levels) {
    for (const auto side : {
             ExactDirectStarH0RetractionCutSide::strict,
             ExactDirectStarH0RetractionCutSide::closed}) {
      const Snapshot gamma =
          build_snapshot(gamma_cofaces, core_facets, level, side);
      const Snapshot retained =
          build_snapshot(retained_cofaces, core_facets, level, side);
      const Snapshot first_incidence =
          build_snapshot(first_incidence_cofaces, core_facets, level, side);
      ExactDirectStarH0RetractionCheckpointAudit audit;
      audit.checkpoint_audit_index = result.checkpoint_audits.size();
      audit.squared_level = level;
      audit.cut_side = side;
      audit.exhaustive_gamma_component_count = gamma.components.size();
      audit.retained_star_component_count = retained.components.size();
      audit.first_incidence_subflow_component_count =
          first_incidence.components.size();
      audit.exhaustive_gamma_core_facet_count = gamma.core_facet_count;
      audit.retained_star_core_facet_count = retained.core_facet_count;
      audit.first_incidence_subflow_core_facet_count =
          first_incidence.core_facet_count;
      audit.exhaustive_gamma_covered_point_reference_count =
          gamma.covered_point_reference_count;
      audit.retained_star_covered_point_reference_count =
          retained.covered_point_reference_count;
      audit.first_incidence_subflow_covered_point_reference_count =
          first_incidence.covered_point_reference_count;
      audit.exhaustive_gamma_projected_partition_digest =
          partition_digest(gamma, order, level, side);
      audit.retained_star_projected_partition_digest =
          partition_digest(retained, order, level, side);
      audit.first_incidence_subflow_projected_partition_digest =
          partition_digest(first_incidence, order, level, side);
      audit.every_component_meets_direct_core =
          gamma.every_component_meets_core &&
          retained.every_component_meets_core &&
          first_incidence.every_component_meets_core;
      audit.canonical_core_partition_equal =
          gamma.components.size() == retained.components.size() &&
          std::equal(
              gamma.components.begin(),
              gamma.components.end(),
              retained.components.begin(),
              [](const Component& left, const Component& right) {
                return left.core_facets == right.core_facets;
              });
      audit.covered_point_unions_equal = snapshots_equal(gamma, retained);
      audit.first_incidence_subflow_canonical_core_partition_equal =
          gamma.components.size() == first_incidence.components.size() &&
          std::equal(
              gamma.components.begin(),
              gamma.components.end(),
              first_incidence.components.begin(),
              [](const Component& left, const Component& right) {
                return left.core_facets == right.core_facets;
              });
      audit.first_incidence_subflow_covered_point_unions_equal =
          snapshots_equal(gamma, first_incidence);
      if (all_first_incidence_partitions_equal &&
          all_first_incidence_point_unions_equal &&
          (!audit.first_incidence_subflow_canonical_core_partition_equal ||
           !audit.first_incidence_subflow_covered_point_unions_equal)) {
        for (const Component& component : gamma.components) {
          audit.divergent_exhaustive_gamma_component_core_facets.push_back(
              component.core_facets);
          audit
              .divergent_exhaustive_gamma_component_covered_point_ids
              .push_back(component.covered_point_ids);
        }
        for (const Component& component : first_incidence.components) {
          audit
              .divergent_first_incidence_subflow_component_core_facets
              .push_back(component.core_facets);
          audit
              .divergent_first_incidence_subflow_component_covered_point_ids
              .push_back(component.covered_point_ids);
        }
      }
      all_partitions_equal = all_partitions_equal &&
                             audit.every_component_meets_direct_core &&
                             audit.canonical_core_partition_equal;
      all_point_unions_equal =
          all_point_unions_equal && audit.covered_point_unions_equal;
      all_first_incidence_partitions_equal =
          all_first_incidence_partitions_equal &&
          audit.every_component_meets_direct_core &&
          audit.first_incidence_subflow_canonical_core_partition_equal;
      all_first_incidence_point_unions_equal =
          all_first_incidence_point_unions_equal &&
          audit.first_incidence_subflow_covered_point_unions_equal;
      result.counters.partition_component_observation_count +=
          gamma.components.size() + retained.components.size() +
          first_incidence.components.size();
      result.checkpoint_audits.push_back(std::move(audit));
    }

    const Snapshot gamma_before = build_snapshot(
        gamma_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::strict);
    const Snapshot gamma_after = build_snapshot(
        gamma_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::closed);
    const Snapshot retained_before = build_snapshot(
        retained_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::strict);
    const Snapshot retained_after = build_snapshot(
        retained_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::closed);
    const Snapshot first_incidence_before = build_snapshot(
        first_incidence_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::strict);
    const Snapshot first_incidence_after = build_snapshot(
        first_incidence_cofaces,
        core_facets,
        level,
        ExactDirectStarH0RetractionCutSide::closed);
    const std::vector<Group> gamma_groups =
        build_groups(gamma_cofaces, gamma_before, gamma_after, level);
    const std::vector<Group> retained_groups =
        build_groups(retained_cofaces, retained_before, retained_after, level);
    const std::vector<Group> first_incidence_groups = build_groups(
        first_incidence_cofaces,
        first_incidence_before,
        first_incidence_after,
        level);
    std::map<Facets, const Group*> retained_by_core;
    for (const Group& group : retained_groups) {
      retained_by_core.emplace(group.core_facets, &group);
    }
    std::map<Facets, const Group*> first_incidence_by_core;
    for (const Group& group : first_incidence_groups) {
      first_incidence_by_core.emplace(group.core_facets, &group);
    }
    for (const Group& gamma_group : gamma_groups) {
      if (gamma_group.core_facets.empty()) {
        const bool silent_outside_core =
            gamma_group.parents.size() == 1U &&
            gamma_group.added_point_ids.empty() &&
            gamma_group.retained_new_coface_count == 0U &&
            gamma_group.first_incidence_candidate_new_coface_count == 0U;
        all_group_semantics_equal =
            all_group_semantics_equal && silent_outside_core;
        all_first_incidence_group_semantics_equal =
            all_first_incidence_group_semantics_equal &&
            silent_outside_core;
        continue;
      }
      const auto retained_found =
          retained_by_core.find(gamma_group.core_facets);
      const auto first_incidence_found =
          first_incidence_by_core.find(gamma_group.core_facets);
      ExactDirectStarH0RetractionBatchGroupAudit audit;
      audit.batch_group_audit_index = result.batch_group_audits.size();
      audit.squared_level = level;
      audit.exhaustive_gamma_q_r = gamma_group.parents.size();
      audit.exhaustive_gamma_action = action_for(audit.exhaustive_gamma_q_r);
      audit.direct_core_facet_count = gamma_group.core_facets.size();
      audit.added_direct_core_facet_count =
          gamma_group.added_core_facets.size();
      audit.retained_new_coface_count =
          gamma_group.retained_new_coface_count;
      audit.omitted_new_coface_count =
          gamma_group.omitted_new_coface_count;
      audit.first_incidence_subflow_new_coface_count =
          gamma_group.first_incidence_candidate_new_coface_count;
      audit.late_star_omitted_new_coface_count =
          gamma_group.late_star_omitted_new_coface_count;
      audit.exhaustive_gamma_added_point_ids =
          gamma_group.added_point_ids;
      audit.exhaustive_gamma_parent_partition_digest =
          parent_partition_digest(gamma_group.parents, order, level);
      const bool gamma_group_is_semantic_noop =
          audit.exhaustive_gamma_q_r == 1U &&
          audit.exhaustive_gamma_action ==
              ExactDirectStarH0RetractionAction::continuation &&
          gamma_group.added_core_facets.empty() &&
          audit.exhaustive_gamma_added_point_ids.empty();
      audit.retained_star_group_present =
          retained_found != retained_by_core.end();
      if (audit.retained_star_group_present) {
        const Group& retained_group = *retained_found->second;
        audit.retained_star_q_r = retained_group.parents.size();
        audit.retained_star_action = action_for(audit.retained_star_q_r);
        audit.retained_star_added_point_ids = retained_group.added_point_ids;
        audit.retained_star_parent_partition_digest =
            parent_partition_digest(retained_group.parents, order, level);
        audit.q_r_equal =
            audit.exhaustive_gamma_q_r == audit.retained_star_q_r;
        audit.action_equal =
            audit.exhaustive_gamma_action == audit.retained_star_action;
        audit.parent_partition_equal =
            audit.exhaustive_gamma_parent_partition_digest ==
            audit.retained_star_parent_partition_digest;
        audit.direct_core_delta_equal =
            gamma_group.added_core_facets ==
            retained_group.added_core_facets;
        audit.added_point_delta_equal =
            audit.exhaustive_gamma_added_point_ids ==
            audit.retained_star_added_point_ids;
      } else {
        audit.retained_star_group_omitted_as_certified_noop =
            gamma_group_is_semantic_noop &&
            gamma_group.retained_new_coface_count == 0U;
        every_missing_retained_group_is_noop =
            every_missing_retained_group_is_noop &&
            audit.retained_star_group_omitted_as_certified_noop;
        if (audit.retained_star_group_omitted_as_certified_noop) {
          ++result.counters.retained_star_omitted_noop_group_count;
          audit.retained_star_q_r = audit.exhaustive_gamma_q_r;
          audit.retained_star_action = audit.exhaustive_gamma_action;
          audit.retained_star_parent_partition_digest =
              audit.exhaustive_gamma_parent_partition_digest;
          audit.q_r_equal = true;
          audit.action_equal = true;
          audit.parent_partition_equal = true;
          audit.direct_core_delta_equal = true;
          audit.added_point_delta_equal = true;
        }
      }
      audit.first_incidence_subflow_group_present =
          first_incidence_found != first_incidence_by_core.end();
      if (audit.first_incidence_subflow_group_present) {
        const Group& first_incidence_group =
            *first_incidence_found->second;
        audit.first_incidence_subflow_q_r =
            first_incidence_group.parents.size();
        audit.first_incidence_subflow_action =
            action_for(audit.first_incidence_subflow_q_r);
        audit.first_incidence_subflow_added_point_ids =
            first_incidence_group.added_point_ids;
        audit.first_incidence_subflow_parent_partition_digest =
            parent_partition_digest(
                first_incidence_group.parents, order, level);
        audit.first_incidence_subflow_q_r_equal =
            audit.exhaustive_gamma_q_r ==
            audit.first_incidence_subflow_q_r;
        audit.first_incidence_subflow_action_equal =
            audit.exhaustive_gamma_action ==
            audit.first_incidence_subflow_action;
        audit.first_incidence_subflow_parent_partition_equal =
            audit.exhaustive_gamma_parent_partition_digest ==
            audit.first_incidence_subflow_parent_partition_digest;
        audit.first_incidence_subflow_direct_core_delta_equal =
            gamma_group.added_core_facets ==
            first_incidence_group.added_core_facets;
        audit.first_incidence_subflow_added_point_delta_equal =
            audit.exhaustive_gamma_added_point_ids ==
            audit.first_incidence_subflow_added_point_ids;
      } else {
        audit.first_incidence_subflow_group_omitted_as_certified_noop =
            gamma_group_is_semantic_noop &&
            gamma_group.first_incidence_candidate_new_coface_count == 0U;
        every_missing_first_incidence_group_is_noop =
            every_missing_first_incidence_group_is_noop &&
            audit
                .first_incidence_subflow_group_omitted_as_certified_noop;
        if (audit
                .first_incidence_subflow_group_omitted_as_certified_noop) {
          ++result.counters.first_incidence_omitted_noop_group_count;
          audit.first_incidence_subflow_q_r =
              audit.exhaustive_gamma_q_r;
          audit.first_incidence_subflow_action =
              audit.exhaustive_gamma_action;
          audit.first_incidence_subflow_parent_partition_digest =
              audit.exhaustive_gamma_parent_partition_digest;
          audit.first_incidence_subflow_q_r_equal = true;
          audit.first_incidence_subflow_action_equal = true;
          audit.first_incidence_subflow_parent_partition_equal = true;
          audit.first_incidence_subflow_direct_core_delta_equal = true;
          audit.first_incidence_subflow_added_point_delta_equal = true;
        }
      }
      all_group_semantics_equal =
          all_group_semantics_equal &&
          (audit.retained_star_group_present ||
           audit.retained_star_group_omitted_as_certified_noop) &&
          audit.q_r_equal &&
          audit.action_equal && audit.parent_partition_equal &&
          audit.direct_core_delta_equal && audit.added_point_delta_equal;
      all_first_incidence_group_semantics_equal =
          all_first_incidence_group_semantics_equal &&
          (audit.first_incidence_subflow_group_present ||
           audit
               .first_incidence_subflow_group_omitted_as_certified_noop) &&
          audit.first_incidence_subflow_q_r_equal &&
          audit.first_incidence_subflow_action_equal &&
          audit.first_incidence_subflow_parent_partition_equal &&
          audit.first_incidence_subflow_direct_core_delta_equal &&
          audit.first_incidence_subflow_added_point_delta_equal;
      result.batch_group_audits.push_back(std::move(audit));
      if (retained_found != retained_by_core.end()) {
        retained_by_core.erase(retained_found);
      }
      if (first_incidence_found != first_incidence_by_core.end()) {
        first_incidence_by_core.erase(first_incidence_found);
      }
    }
    all_group_semantics_equal =
        all_group_semantics_equal && retained_by_core.empty();
    all_first_incidence_group_semantics_equal =
        all_first_incidence_group_semantics_equal &&
        first_incidence_by_core.empty();
  }

  result.counters.checkpoint_count = result.checkpoint_audits.size();
  result.counters.batch_group_count = result.batch_group_audits.size();
  if (result.checkpoint_audits.size() > budget.maximum_checkpoint_count ||
      result.counters.partition_component_observation_count >
          budget.maximum_partition_component_observation_count ||
      result.batch_group_audits.size() >
          budget.maximum_batch_group_audit_count ||
      result.omitted_coface_audits.size() >
          budget.maximum_omitted_coface_audit_count ||
      result.first_incidence_audits.size() >
          budget.maximum_first_incidence_facet_audit_count ||
      result.counters.first_incidence_cominimizer_reference_count >
          budget.maximum_first_incidence_cominimizer_reference_count ||
      result.late_star_coface_audits.size() >
          budget.maximum_late_star_coface_audit_count) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_preflight_budget_insufficient;
    return result;
  }

  result.strict_and_closed_core_partitions_equal_at_every_exact_level =
      all_partitions_equal;
  result.covered_point_unions_equal_at_every_exact_level =
      all_point_unions_equal;
  result.qr_actions_parents_and_point_deltas_equal_for_core_groups =
      all_group_semantics_equal;
  result.first_incidence_subflow_strict_and_closed_core_partitions_equal =
      all_first_incidence_partitions_equal;
  result.first_incidence_subflow_covered_point_unions_equal =
      all_first_incidence_point_unions_equal;
  result.first_incidence_subflow_qr_actions_parents_and_point_deltas_equal =
      all_first_incidence_group_semantics_equal;
  result.every_missing_retained_star_group_is_certified_noop =
      every_missing_retained_group_is_noop;
  result.every_missing_first_incidence_group_is_certified_noop =
      every_missing_first_incidence_group_is_noop;
  if (!all_partitions_equal || !all_point_unions_equal ||
      !all_first_incidence_partitions_equal ||
      !all_first_incidence_point_unions_equal) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_strict_or_closed_h0_partition;
    return result;
  }
  if (!all_group_semantics_equal ||
      !all_first_incidence_group_semantics_equal ||
      !every_missing_retained_group_is_noop ||
      !every_missing_first_incidence_group_is_noop) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            falsified_batch_qr_action_or_delta;
    return result;
  }

  for (auto& audit : result.omitted_coface_audits) {
    audit.no_present_or_future_h0_effect = true;
  }
  for (auto& audit : result.late_star_coface_audits) {
    audit.no_present_or_future_h0_effect = true;
  }
  std::size_t logical_entries = result.checkpoint_audits.size() +
                                result.batch_group_audits.size() +
                                result.omitted_coface_audits.size() +
                                result.first_incidence_audits.size() +
                                result.late_star_coface_audits.size();
  for (const auto& audit : result.checkpoint_audits) {
    for (const Facets& component :
         audit.divergent_exhaustive_gamma_component_core_facets) {
      for (const Facet& facet : component) {
        logical_entries += facet.size();
      }
    }
    for (const auto& points :
         audit.divergent_exhaustive_gamma_component_covered_point_ids) {
      logical_entries += points.size();
    }
    for (const Facets& component :
         audit.divergent_first_incidence_subflow_component_core_facets) {
      for (const Facet& facet : component) {
        logical_entries += facet.size();
      }
    }
    for (const auto& points :
         audit
             .divergent_first_incidence_subflow_component_covered_point_ids) {
      logical_entries += points.size();
    }
  }
  for (const auto& audit : result.batch_group_audits) {
    logical_entries += audit.exhaustive_gamma_added_point_ids.size() +
                       audit.retained_star_added_point_ids.size() +
                       audit.first_incidence_subflow_added_point_ids.size();
  }
  for (const auto& audit : result.omitted_coface_audits) {
    logical_entries +=
        audit.coface_point_ids.size() + audit.added_point_ids.size();
  }
  for (const auto& audit : result.first_incidence_audits) {
    logical_entries += audit.direct_core_facet_point_ids.size();
    for (const Facet& coface :
         audit.complete_cominimizer_coface_point_ids) {
      logical_entries += coface.size();
    }
  }
  for (const auto& audit : result.late_star_coface_audits) {
    logical_entries += audit.coface_point_ids.size() +
                       audit.added_point_ids.size() +
                       audit.touched_direct_core_first_incidence_squared_levels
                           .size();
    for (const Facet& facet :
         audit.touched_direct_core_facet_point_ids) {
      logical_entries += facet.size();
    }
  }
  result.counters.logical_output_entry_count = logical_entries;
  if (logical_entries > budget.maximum_logical_output_entry_count) {
    result.decision =
        ExactDirectStarH0RetractionDifferentialDecision::
            no_preflight_budget_insufficient;
    return result;
  }

  result.conditional_reduced_h0_forest_semantics_equivalent = true;
  result.conditional_first_incidence_subflow_h0_retraction_certified = true;
  result.conditional_horizontal_h0_retraction_certified = true;
  result.decision =
      ExactDirectStarH0RetractionDifferentialDecision::
          complete_bounded_conditional_direct_star_h0_retraction_differential;
  return result;
}

ExactDirectStarH0RetractionDifferentialVerification
verify_exact_direct_star_h0_retraction_differential(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget,
    const ExactDirectStarH0RetractionDifferentialResult& observed) {
  ExactDirectStarH0RetractionDifferentialVerification verification;
  const auto expected = build_exact_direct_star_h0_retraction_differential(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      source_incidence_journal,
      source_star_budget,
      source_star_traversal_order,
      source_star,
      source_plan_budget,
      source_plan,
      order,
      budget);
  verification.source_pipeline_freshly_replayed =
      expected.source_plan_freshly_replayed;
  verification.gamma_oracle_freshly_replayed =
      expected.gamma_high_cut_freshly_built &&
      expected.gamma_high_cut_freshly_verified;
  verification.expected_result_freshly_rebuilt = true;
  verification.observed_recursively_equal = observed == expected;
  verification.h0_and_v2_identity_claims_separated =
      expected.conditional_horizontal_h0_retraction_certified &&
      expected.conditional_first_incidence_subflow_h0_retraction_certified &&
      expected.conditional_reduced_h0_forest_semantics_equivalent &&
      !expected.exhaustive_gamma_faceted_payload_reproduced &&
      !expected.contract_v2_identity_equivalence_claimed &&
      !expected.contract_v2_batch_identity_equivalence_claimed &&
      !expected.contract_v2_forest_identity_equivalence_claimed &&
      !expected.forest_semantics_exact_claimed &&
      !expected.incidence_complete_reduction_proved &&
      !expected.universal_product_completeness_claimed &&
      !expected.public_status_claimed;
  verification.fresh_replay_certified =
      verification.source_pipeline_freshly_replayed &&
      verification.gamma_oracle_freshly_replayed &&
      verification.expected_result_freshly_rebuilt &&
      verification.observed_recursively_equal &&
      verification.h0_and_v2_identity_claims_separated;
  verification.result_certified =
      verification.fresh_replay_certified && observed.certified_differential();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
