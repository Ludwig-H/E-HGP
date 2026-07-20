#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;
using FacetSet = std::vector<FacetLabel>;
using ActiveRoot = ExactPersistentReducedGammaActiveRoot;
using ActiveRootMap = std::map<std::size_t, ActiveRoot>;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t point_count,
    std::size_t subset_size) {
  if (subset_size > point_count) {
    return 0U;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    value = checked_multiply(
        value,
        point_count - subset_size + factor,
        "the persistent reduced-Gamma binomial coefficient overflows");
    value /= factor;
  }
  return value;
}

void validate_budget_caps(
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) {
  if (budget.gamma_budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.gamma_budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.gamma_budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count ||
      budget.maximum_activation_level_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_activation_level_count ||
      budget.maximum_total_facet_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_facet_work_count ||
      budget.maximum_total_coface_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_coface_work_count ||
      budget.maximum_total_union_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_union_work_count ||
      budget.maximum_node_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_node_count ||
      budget.maximum_child_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_child_reference_count ||
      budget.maximum_group_root_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_root_reference_count ||
      budget.maximum_group_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_count ||
      budget.maximum_group_newly_active_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_newly_active_facet_count ||
      budget.maximum_group_equal_level_coface_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_equal_level_coface_count ||
      budget.maximum_delta_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_facet_count ||
      budget.maximum_delta_point_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_point_reference_count) {
    throw std::invalid_argument(
        "a persistent reduced-Gamma history budget exceeds its bounded "
        "reference cap");
  }
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) {
  if (cloud.size() <
          ExactPersistentReducedGammaOrderHistory::
              minimum_supported_point_count ||
      cloud.size() >
          ExactPersistentReducedGammaOrderHistory::
              maximum_supported_point_count ||
      order <
          ExactPersistentReducedGammaOrderHistory::minimum_supported_order ||
      order >
          ExactPersistentReducedGammaOrderHistory::maximum_supported_order ||
      order > cloud.size()) {
    throw std::invalid_argument(
        "a persistent reduced-Gamma history requires 2<=n<=14 and "
        "2<=k<=min(n,10)");
  }
  validate_budget_caps(budget);
}

[[nodiscard]] bool budget_covers_preflight(
    const ExactPersistentReducedGammaOrderHistoryBudget& budget,
    const ExactPersistentReducedGammaOrderHistory& history) {
  return budget.gamma_budget.maximum_enumerated_facet_count >=
             history.exhaustive_facet_count &&
         budget.gamma_budget.maximum_enumerated_coface_count >=
             history.exhaustive_coface_count &&
         budget.gamma_budget.maximum_union_attempt_count >=
             history.exhaustive_union_attempt_count &&
         budget.maximum_activation_level_count >=
             history.required_activation_level_capacity &&
         budget.maximum_total_facet_work_count >=
             history.required_total_facet_work_capacity &&
         budget.maximum_total_coface_work_count >=
             history.required_total_coface_work_capacity &&
         budget.maximum_total_union_work_count >=
             history.required_total_union_work_capacity &&
         budget.maximum_node_count >= history.required_node_capacity &&
         budget.maximum_child_reference_count >=
             history.required_child_reference_capacity &&
         budget.maximum_group_root_reference_count >=
             history.required_group_root_reference_capacity &&
         budget.maximum_group_count >= history.required_group_capacity &&
         budget.maximum_group_newly_active_facet_count >=
             history.required_group_newly_active_facet_capacity &&
         budget.maximum_group_equal_level_coface_count >=
             history.required_group_equal_level_coface_capacity &&
         budget.maximum_delta_facet_count >=
             history.required_delta_facet_capacity &&
         budget.maximum_delta_point_reference_count >=
             history.required_delta_point_reference_capacity;
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
    ExactPersistentReducedGammaOrderHistoryCounters& counters) {
  exact::ExactLevel diameter_squared;
  for (std::size_t left = 0U; left < cloud.size(); ++left) {
    for (std::size_t right = left + 1U;
         right < cloud.size();
         ++right) {
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
        "a duplicate-free nontrivial cloud has zero exact diameter");
  }
  return diameter_squared;
}

[[nodiscard]] FacetLabel first_canonical_facet(std::size_t order) {
  FacetLabel facet(order);
  for (std::size_t index = 0U; index < order; ++index) {
    facet[index] = static_cast<PointId>(index);
  }
  return facet;
}

[[nodiscard]] std::vector<PointId> covered_point_ids(
    const FacetSet& facets) {
  std::set<PointId> points;
  for (const FacetLabel& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return {points.begin(), points.end()};
}

void require_canonical_facet_set(
    const FacetSet& facets,
    std::size_t order) {
  if (!std::is_sorted(facets.begin(), facets.end()) ||
      std::adjacent_find(facets.begin(), facets.end()) != facets.end()) {
    throw std::logic_error(
        "a persistent reduced-Gamma root has a noncanonical facet set");
  }
  for (const FacetLabel& facet : facets) {
    if (facet.size() != order ||
        !std::is_sorted(facet.begin(), facet.end()) ||
        std::adjacent_find(facet.begin(), facet.end()) != facet.end()) {
      throw std::logic_error(
          "a persistent reduced-Gamma root has a noncanonical facet");
    }
  }
}

[[nodiscard]] std::map<FacetSet, std::size_t> root_ids_by_facet_set(
    const ActiveRootMap& active_roots,
    std::size_t order) {
  std::map<FacetSet, std::size_t> result;
  for (const auto& [root_id, root] : active_roots) {
    if (root.root_node_id != root_id) {
      throw std::logic_error(
          "an active reduced-Gamma root key disagrees with its node ID");
    }
    require_canonical_facet_set(root.facet_point_ids, order);
    if (root.covered_point_ids != covered_point_ids(root.facet_point_ids) ||
        !result.emplace(root.facet_point_ids, root_id).second) {
      throw std::logic_error(
          "active reduced-Gamma roots do not have unique exact facet sets");
    }
  }
  return result;
}

[[nodiscard]] bool roots_match_nontrivial_components(
    const ActiveRootMap& active_roots,
    const std::vector<ExactStrictGammaComponentWitness>& components,
    std::size_t order) {
  const std::map<FacetSet, std::size_t> roots =
      root_ids_by_facet_set(active_roots, order);
  std::set<FacetSet> nontrivial_components;
  for (const ExactStrictGammaComponentWitness& component : components) {
    require_canonical_facet_set(component.facet_point_ids, order);
    if (component.facet_point_ids.size() > 1U &&
        !nontrivial_components.insert(component.facet_point_ids).second) {
      return false;
    }
  }
  if (roots.size() != nontrivial_components.size()) {
    return false;
  }
  for (const auto& [facets, root_id] : roots) {
    static_cast<void>(root_id);
    if (!nontrivial_components.contains(facets)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::vector<exact::ExactLevel> activation_levels_from_cut(
    const ExactStrictGammaResult& high_cut,
    ExactPersistentReducedGammaOrderHistoryCounters& counters) {
  std::vector<exact::ExactLevel> levels;
  levels.reserve(
      high_cut.active_facets.size() + high_cut.active_cofaces.size());
  for (const ExactStrictGammaFacetWitness& facet :
       high_cut.active_facets) {
    levels.push_back(facet.squared_level);
    ++counters.activation_facet_level_reference_count;
  }
  for (const ExactStrictGammaCofaceWitness& coface :
       high_cut.active_cofaces) {
    levels.push_back(coface.squared_level);
    ++counters.activation_coface_level_reference_count;
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
  counters.activation_level_count = levels.size();
  return levels;
}

[[nodiscard]] std::size_t nontrivial_component_count(
    const std::vector<ExactStrictGammaComponentWitness>& components) {
  return static_cast<std::size_t>(std::count_if(
      components.begin(),
      components.end(),
      [](const ExactStrictGammaComponentWitness& component) {
        return component.facet_point_ids.size() > 1U;
      }));
}

struct PendingMutation {
  std::size_t group_index{};
  ExactReducedGammaBatchGroupKind kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};
  std::vector<std::size_t> prior_root_ids;
  std::optional<std::size_t> resulting_root_id;
  std::optional<std::size_t> created_node_id;
  ActiveRoot resulting_root;
  ExactPersistentReducedGammaHistoryGroupRecord record;
  std::optional<ExactPersistentReducedGammaNode> node;
};

struct PendingBatchAccounting {
  std::size_t deferred_group_count{};
  std::size_t birth_group_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t fully_redundant_group_count{};
  std::size_t group_root_reference_count{};
  std::size_t group_newly_active_facet_count{};
  std::size_t group_equal_level_coface_count{};
  std::size_t coverage_delta_count{};
  std::size_t added_facet_count{};
  std::size_t added_point_reference_count{};
  std::size_t child_reference_count{};
};

[[nodiscard]] FacetSet canonical_union_with_delta(
    const ActiveRootMap& snapshot,
    std::span<const std::size_t> prior_root_ids,
    const ExactReducedGammaCoverageDelta& delta,
    std::vector<PointId>& prior_covered_points) {
  FacetSet prior_facets;
  for (const std::size_t root_id : prior_root_ids) {
    const auto root = snapshot.find(root_id);
    if (root == snapshot.end()) {
      throw std::logic_error(
          "a compact history record references no snapshot root");
    }
    prior_facets.insert(
        prior_facets.end(),
        root->second.facet_point_ids.begin(),
        root->second.facet_point_ids.end());
  }
  std::sort(prior_facets.begin(), prior_facets.end());
  if (std::adjacent_find(prior_facets.begin(), prior_facets.end()) !=
      prior_facets.end()) {
    throw std::logic_error(
        "snapshot roots share a facet before a reduced-Gamma group");
  }
  prior_covered_points = covered_point_ids(prior_facets);

  FacetSet result = prior_facets;
  result.insert(
      result.end(),
      delta.added_facet_point_ids.begin(),
      delta.added_facet_point_ids.end());
  std::sort(result.begin(), result.end());
  if (std::adjacent_find(result.begin(), result.end()) != result.end()) {
    throw std::logic_error(
        "a coverage delta repeats a prior or newly added Gamma facet");
  }
  return result;
}

void certify_reconstructed_group_state(
    const FacetSet& resulting_facets,
    const std::vector<PointId>& prior_points,
    const ExactReducedGammaCoverageDelta& delta,
    const ExactStrictGammaComponentWitness& closed_component) {
  if (resulting_facets != closed_component.facet_point_ids) {
    throw std::logic_error(
        "snapshot roots plus the facet delta do not reconstruct the "
        "closed Gamma component");
  }
  const std::vector<PointId> resulting_points =
      covered_point_ids(resulting_facets);
  std::vector<PointId> expected_added_points;
  std::set_difference(
      resulting_points.begin(),
      resulting_points.end(),
      prior_points.begin(),
      prior_points.end(),
      std::back_inserter(expected_added_points));
  if (delta.added_point_ids != expected_added_points ||
      delta.fully_redundant !=
          (delta.added_facet_point_ids.empty() &&
           delta.added_point_ids.empty())) {
    throw std::logic_error(
        "a compact history record has an inexact point delta");
  }
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistory
compute_exact_persistent_reduced_gamma_order_history(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget) {
  validate_domain(cloud, order, budget);

  ExactPersistentReducedGammaOrderHistory history;
  history.requested_budget = budget;
  history.point_count = cloud.size();
  history.order = order;
  history.scope = ExactPersistentReducedGammaOrderHistoryScope::
      bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only;
  history.counters.preflight_count = 1U;
  history.exhaustive_facet_count = bounded_binomial(cloud.size(), order);
  history.exhaustive_coface_count =
      bounded_binomial(cloud.size(), order + 1U);
  history.exhaustive_union_attempt_count = checked_multiply(
      order,
      history.exhaustive_coface_count,
      "the persistent reduced-Gamma union bound overflows");
  history.candidate_space_size_certified = true;

  // At terminal order there are no (k+1)-cofaces, hence hgp_reduced has no
  // nontrivial component.  This is a complete empty result, not a failed
  // attempt to run the equal-level k<n machinery.
  if (order == cloud.size()) {
    history.preflight_budget_sufficient = true;
    // These are the explicitly named vacuous terminal properties. Normal
    // high-cut and final-single-root facts remain false because neither
    // object exists at k=n.
    history.activation_levels_canonical_and_complete = true;
    history.all_reduced_gamma_batches_fresh_replay_certified = true;
    history.groups_resolved_against_pre_batch_snapshots = true;
    history.mutations_applied_after_complete_batch_resolution = true;
    history.active_roots_match_nontrivial_components_after_every_batch =
        true;
    history.node_ids_dense_and_deterministic = true;
    history.children_precede_parent = true;
    history.each_child_consumed_at_most_once = true;
    history.every_exhaustive_coface_affected_exactly_once = true;
    history.group_kinds_have_exact_persistent_effects = true;
    history.every_non_deferred_group_has_exactly_one_coverage_delta = true;
    history.fully_redundant_groups_preserved = true;
    // The normal k<n coverage identity is added_facets=F. Here F=1 but the
    // reduced terminal history intentionally has zero deltas, so that normal
    // fact remains false rather than being asserted vacuously.
    history.terminal_order_complete_empty = true;
    history.persistent_reduced_gamma_history_certified = true;
    history.decision = ExactPersistentReducedGammaOrderHistoryDecision::
        complete_empty_terminal_order;
    return history;
  }

  history.required_activation_level_capacity = checked_add(
      history.exhaustive_facet_count,
      history.exhaustive_coface_count,
      "the persistent reduced-Gamma activation-level bound overflows");
  const std::size_t exhaustive_run_count = checked_add(
      history.required_activation_level_capacity,
      1U,
      "the persistent reduced-Gamma exhaustive-run count overflows");
  history.required_total_facet_work_capacity = checked_multiply(
      exhaustive_run_count,
      history.exhaustive_facet_count,
      "the persistent reduced-Gamma facet-work bound overflows");
  history.required_total_coface_work_capacity = checked_multiply(
      exhaustive_run_count,
      history.exhaustive_coface_count,
      "the persistent reduced-Gamma coface-work bound overflows");
  history.required_total_union_work_capacity = checked_multiply(
      exhaustive_run_count,
      history.exhaustive_union_attempt_count,
      "the persistent reduced-Gamma union-work bound overflows");
  // Every created node owns at least one equal-level coface token. The
  // merge tree ends in one root, so both child references and all prior-root
  // references stored in compact group records are bounded by C-1.
  history.required_node_capacity = history.exhaustive_coface_count;
  history.required_child_reference_capacity =
      history.exhaustive_coface_count - 1U;
  history.required_group_root_reference_capacity =
      history.exhaustive_coface_count - 1U;
  history.required_group_capacity =
      history.required_activation_level_capacity;
  history.required_group_newly_active_facet_capacity =
      history.exhaustive_facet_count;
  history.required_group_equal_level_coface_capacity =
      history.exhaustive_coface_count;
  history.required_delta_facet_capacity =
      history.exhaustive_facet_count;
  // This counts added_point_ids only.  Point IDs stored inside the F facet
  // labels are represented by the separate facet capacity.
  history.required_delta_point_reference_capacity = checked_multiply(
      order,
      history.exhaustive_facet_count,
      "the persistent reduced-Gamma point-delta bound overflows");

  history.preflight_budget_sufficient =
      budget_covers_preflight(budget, history);
  if (!history.preflight_budget_sufficient) {
    history.decision = ExactPersistentReducedGammaOrderHistoryDecision::
        no_history_preflight_budget_insufficient;
    return history;
  }
  history.geometry_started_after_successful_preflight = true;

  history.exact_diameter_squared =
      exact_squared_diameter(cloud, history.counters);
  history.high_cut_squared_level = exact::ExactLevel{
      history.exact_diameter_squared->rational() *
      exact::ExactRational{exact::BigInt{2}}};
  history.high_cut_equals_twice_exact_squared_diameter =
      history.high_cut_squared_level->rational() ==
      history.exact_diameter_squared->rational() *
          exact::ExactRational{exact::BigInt{2}};
  const std::vector<FacetLabel> sources{
      first_canonical_facet(order)};
  history.high_cut_gamma =
      build_exact_strict_gamma_source_classification(
          cloud,
          order,
          *history.high_cut_squared_level,
          sources,
          budget.gamma_budget);
  ++history.counters.high_cut_gamma_build_count;
  if (history.high_cut_gamma->decision !=
          ExactStrictGammaDecision::
              complete_all_sources_active_and_classified ||
      !history.high_cut_gamma->candidate_space_size_certified ||
      !history.high_cut_gamma->strict_open_cut_certified ||
      !history.high_cut_gamma->full_pi0_isolated_facets_included ||
      !history.high_cut_gamma->exhaustive_active_catalog_certified ||
      !history.high_cut_gamma->complete_source_classification_certified ||
      !history.high_cut_gamma->all_sources_active_and_classified ||
      history.high_cut_gamma->active_facets.size() !=
          history.exhaustive_facet_count ||
      history.high_cut_gamma->active_cofaces.size() !=
          history.exhaustive_coface_count) {
    throw std::logic_error(
        "the exact high cut failed to catalogue every Gamma facet and "
        "coface");
  }
  history.high_cut_catalog_exhaustive = true;

  history.activation_levels = activation_levels_from_cut(
      *history.high_cut_gamma, history.counters);
  if (history.activation_levels.empty() ||
      history.activation_levels.size() >
          history.required_activation_level_capacity ||
      history.activation_levels.size() >
          budget.maximum_activation_level_count) {
    throw std::logic_error(
        "the exact Gamma activation-level catalogue violated preflight");
  }
  history.high_cut_strictly_above_all_activation_levels =
      std::all_of(
          history.activation_levels.begin(),
          history.activation_levels.end(),
          [&](const exact::ExactLevel& level) {
            return level < *history.high_cut_squared_level;
          });
  if (!history.high_cut_strictly_above_all_activation_levels) {
    throw std::logic_error(
        "twice the exact squared diameter did not dominate Gamma levels");
  }
  history.activation_levels_canonical_and_complete =
      std::is_sorted(
          history.activation_levels.begin(),
          history.activation_levels.end()) &&
      std::adjacent_find(
          history.activation_levels.begin(),
          history.activation_levels.end()) ==
          history.activation_levels.end() &&
      history.counters.activation_facet_level_reference_count ==
          history.exhaustive_facet_count &&
      history.counters.activation_coface_level_reference_count ==
          history.exhaustive_coface_count;

  const std::size_t actual_run_count = checked_add(
      history.activation_levels.size(),
      1U,
      "the actual persistent reduced-Gamma run count overflows");
  history.counters.total_facet_work_count = checked_multiply(
      actual_run_count,
      history.exhaustive_facet_count,
      "the actual persistent reduced-Gamma facet work overflows");
  history.counters.total_coface_work_count = checked_multiply(
      actual_run_count,
      history.exhaustive_coface_count,
      "the actual persistent reduced-Gamma coface work overflows");
  history.counters.total_union_work_count = checked_multiply(
      actual_run_count,
      history.exhaustive_union_attempt_count,
      "the actual persistent reduced-Gamma union work overflows");
  if (history.counters.total_facet_work_count >
          history.required_total_facet_work_capacity ||
      history.counters.total_coface_work_count >
          history.required_total_coface_work_capacity ||
      history.counters.total_union_work_count >
          history.required_total_union_work_capacity) {
    throw std::logic_error(
        "the actual persistent reduced-Gamma work violated preflight");
  }

  ActiveRootMap active_roots;
  std::set<std::size_t> consumed_child_node_ids;
  std::set<FacetLabel> globally_added_facets;
  std::set<FacetLabel> globally_newly_active_facets;
  std::set<FacetLabel> globally_affected_cofaces;
  bool all_batches_certified = true;
  bool all_pre_batch_bijections = true;
  bool all_post_batch_bijections = true;
  bool all_groups_frozen = true;
  bool all_mutations_deferred = true;
  std::size_t completed_batch_count = 0U;

  history.batch_metadata.reserve(history.activation_levels.size());
  for (std::size_t level_index = 0U;
       level_index < history.activation_levels.size();
       ++level_index) {
    const exact::ExactLevel& level =
        history.activation_levels[level_index];
    ExactReducedGammaBatchResult batch =
        build_exact_reduced_gamma_batch(
            cloud, order, level, budget.gamma_budget);
    if (batch.decision != ExactReducedGammaBatchDecision::
                              complete_exhaustive_reduced_gamma_batch ||
        !batch.equal_level_batch_semantics_certified ||
        batch.groups.empty()) {
      throw std::logic_error(
          "an activation level produced no certified reduced-Gamma group");
    }
    all_batches_certified =
        all_batches_certified &&
        batch.gamma_transition_fresh_replay_certified;

    ExactPersistentReducedGammaBatchMetadata metadata;
    metadata.batch_index = level_index;
    metadata.activation_level_index = level_index;
    metadata.squared_level = level;
    metadata.first_group_record_index = history.group_records.size();
    metadata.group_record_count = batch.groups.size();
    metadata.active_root_count_before = active_roots.size();
    metadata.strict_nontrivial_component_count =
        nontrivial_component_count(
            batch.gamma_transition.strict_gamma.components);
    metadata.pre_batch_root_bijection_certified =
        roots_match_nontrivial_components(
            active_roots,
            batch.gamma_transition.strict_gamma.components,
            order);
    if (!metadata.pre_batch_root_bijection_certified) {
      throw std::logic_error(
          "persistent roots do not match the strict nontrivial Gamma "
          "components before a batch");
    }
    all_pre_batch_bijections =
        all_pre_batch_bijections &&
        metadata.pre_batch_root_bijection_certified;

    const std::map<FacetSet, std::size_t> snapshot_root_by_facets =
        root_ids_by_facet_set(active_roots, order);
    std::vector<std::optional<std::size_t>>
        root_by_strict_component(
            batch.gamma_transition.strict_gamma.components.size());
    for (const auto& classification :
         batch.strict_component_classifications) {
      if (classification.strict_component_index >=
          root_by_strict_component.size()) {
        throw std::logic_error(
            "a reduced-Gamma classification references no strict "
            "component");
      }
      const FacetSet& facets = batch.gamma_transition.strict_gamma
                                   .components[classification
                                                   .strict_component_index]
                                   .facet_point_ids;
      const auto root = snapshot_root_by_facets.find(facets);
      if (classification.carries_prior_reduced_root) {
        if (root == snapshot_root_by_facets.end()) {
          throw std::logic_error(
              "a nontrivial strict component has no persistent root");
        }
        root_by_strict_component[classification.strict_component_index] =
            root->second;
      } else if (root != snapshot_root_by_facets.end()) {
        throw std::logic_error(
            "an isolated strict facet unexpectedly carries a root");
      }
    }

    std::vector<PendingMutation> pending;
    std::vector<ExactPersistentReducedGammaNode> pending_nodes;
    std::vector<ExactPersistentReducedGammaHistoryGroupRecord>
        pending_records;
    std::set<std::size_t> referenced_snapshot_roots;
    std::set<std::size_t> pending_consumed_child_node_ids;
    std::set<FacetLabel> pending_added_facets;
    std::set<FacetLabel> pending_newly_active_facets;
    std::set<FacetLabel> pending_affected_cofaces;
    PendingBatchAccounting accounting;
    pending.reserve(batch.groups.size());
    pending_records.reserve(batch.groups.size());
    std::size_t next_node_id = history.nodes.size();
    for (std::size_t group_index = 0U;
         group_index < batch.groups.size();
         ++group_index) {
      const ExactReducedGammaBatchGroup& group =
          batch.groups[group_index];
      if (group.closed_component_index >=
          batch.gamma_transition.closed_components.size()) {
        throw std::logic_error(
            "a reduced-Gamma history group references no closed "
            "component");
      }
      const ExactStrictGammaComponentWitness& closed_component =
          batch.gamma_transition.closed_components
              [group.closed_component_index];

      PendingMutation mutation;
      mutation.group_index = group_index;
      mutation.kind = group.kind;
      for (const std::size_t strict_component_index :
           group.prior_reduced_root_strict_component_indices) {
        if (strict_component_index >= root_by_strict_component.size() ||
            !root_by_strict_component[strict_component_index]
                 .has_value()) {
          throw std::logic_error(
              "a reduced batch prior root has no snapshot identity");
        }
        mutation.prior_root_ids.push_back(
            *root_by_strict_component[strict_component_index]);
      }
      std::sort(
          mutation.prior_root_ids.begin(),
          mutation.prior_root_ids.end());
      if (std::adjacent_find(
              mutation.prior_root_ids.begin(),
              mutation.prior_root_ids.end()) !=
          mutation.prior_root_ids.end()) {
        throw std::logic_error(
            "a reduced-Gamma group repeats a prior root");
      }
      for (const std::size_t prior_root_id : mutation.prior_root_ids) {
        if (!referenced_snapshot_roots.insert(prior_root_id).second) {
          throw std::logic_error(
              "one pre-batch root is referenced by multiple groups");
        }
      }

      mutation.record.group_record_index =
          history.group_records.size() + pending_records.size();
      mutation.record.batch_index = level_index;
      mutation.record.batch_group_index = group_index;
      mutation.record.squared_level = level;
      mutation.record.kind = group.kind;
      mutation.record.canonical_representative_facet_point_ids =
          group.canonical_representative_facet_point_ids;
      mutation.record.prior_root_node_ids = mutation.prior_root_ids;
      mutation.record.newly_active_facet_point_ids =
          group.newly_active_facet_point_ids;
      mutation.record.equal_level_coface_point_ids =
          group.equal_level_coface_point_ids;
      mutation.record.coverage_delta = group.coverage_delta;
      mutation.record.resolved_from_pre_batch_snapshot = true;
      accounting.group_root_reference_count = checked_add(
          accounting.group_root_reference_count,
          mutation.prior_root_ids.size(),
          "the compact group root-reference count overflows");
      accounting.group_newly_active_facet_count = checked_add(
          accounting.group_newly_active_facet_count,
          group.newly_active_facet_point_ids.size(),
          "the compact newly-active facet count overflows");
      accounting.group_equal_level_coface_count = checked_add(
          accounting.group_equal_level_coface_count,
          group.equal_level_coface_point_ids.size(),
          "the compact equal-level coface count overflows");
      for (const FacetLabel& facet : group.newly_active_facet_point_ids) {
        if (globally_newly_active_facets.contains(facet) ||
            !pending_newly_active_facets.insert(facet).second) {
          throw std::logic_error(
              "one Gamma facet is newly active in multiple records");
        }
      }
      for (const FacetLabel& coface : group.equal_level_coface_point_ids) {
        if (globally_affected_cofaces.contains(coface) ||
            !pending_affected_cofaces.insert(coface).second) {
          throw std::logic_error(
              "one Gamma coface is affected in multiple records");
        }
      }

      switch (group.kind) {
        case ExactReducedGammaBatchGroupKind::
            deferred_isolated_facet:
          if (!mutation.prior_root_ids.empty() ||
              group.coverage_delta.has_value() ||
              closed_component.facet_point_ids.size() != 1U) {
            throw std::logic_error(
                "a deferred reduced-Gamma group has persistent state");
          }
          ++accounting.deferred_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::birth:
          if (!mutation.prior_root_ids.empty() ||
              !group.coverage_delta.has_value()) {
            throw std::logic_error(
                "a reduced-Gamma birth has prior roots or no delta");
          }
          mutation.created_node_id = next_node_id++;
          mutation.resulting_root_id = mutation.created_node_id;
          ++accounting.birth_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::continuation:
          if (mutation.prior_root_ids.size() != 1U ||
              !group.coverage_delta.has_value()) {
            throw std::logic_error(
                "a reduced-Gamma continuation does not retain one root");
          }
          mutation.resulting_root_id = mutation.prior_root_ids.front();
          ++accounting.continuation_group_count;
          break;
        case ExactReducedGammaBatchGroupKind::multifusion:
          if (mutation.prior_root_ids.size() < 2U ||
              !group.coverage_delta.has_value()) {
            throw std::logic_error(
                "a reduced-Gamma multifusion has fewer than two frozen "
                "children");
          }
          mutation.created_node_id = next_node_id++;
          mutation.resulting_root_id = mutation.created_node_id;
          ++accounting.multifusion_group_count;
          break;
      }

      if (mutation.resulting_root_id.has_value()) {
        if (!group.coverage_delta.has_value()) {
          throw std::logic_error(
              "a non-deferred reduced-Gamma group omitted its delta");
        }
        std::vector<PointId> prior_points;
        const FacetSet reconstructed_facets = canonical_union_with_delta(
            active_roots,
            mutation.prior_root_ids,
            *group.coverage_delta,
            prior_points);
        certify_reconstructed_group_state(
            reconstructed_facets,
            prior_points,
            *group.coverage_delta,
            closed_component);
        mutation.resulting_root.root_node_id =
            *mutation.resulting_root_id;
        mutation.resulting_root.facet_point_ids =
            reconstructed_facets;
        mutation.resulting_root.covered_point_ids =
            covered_point_ids(reconstructed_facets);
        mutation.record.resulting_root_node_id =
            mutation.resulting_root_id;
      }
      if (mutation.created_node_id.has_value()) {
        if (!group.coverage_delta.has_value()) {
          throw std::logic_error(
              "a created reduced-Gamma node has no exact coverage delta");
        }
        mutation.record.created_node_id = mutation.created_node_id;
        ExactPersistentReducedGammaNode node;
        node.node_id = *mutation.created_node_id;
        node.creation_batch_index = level_index;
        node.creation_group_index = group_index;
        node.squared_level = level;
        node.kind = group.kind == ExactReducedGammaBatchGroupKind::birth
                        ? ExactPersistentReducedGammaNodeKind::birth
                        : ExactPersistentReducedGammaNodeKind::multifusion;
        if (group.kind ==
            ExactReducedGammaBatchGroupKind::multifusion) {
          node.child_node_ids = mutation.prior_root_ids;
        }
        node.children_resolved_from_pre_batch_snapshot = true;
        accounting.child_reference_count = checked_add(
            accounting.child_reference_count,
            node.child_node_ids.size(),
            "the merge-tree child-reference count overflows");
        for (const std::size_t child_id : node.child_node_ids) {
          if (child_id >= node.node_id ||
              consumed_child_node_ids.contains(child_id) ||
              !pending_consumed_child_node_ids.insert(child_id).second) {
            throw std::logic_error(
                "a merge-tree child is not older or was already consumed");
          }
        }
        mutation.node.emplace(std::move(node));
      }

      if (group.coverage_delta.has_value()) {
        ++accounting.coverage_delta_count;
        if (group.coverage_delta->fully_redundant) {
          ++accounting.fully_redundant_group_count;
        }
        accounting.added_facet_count = checked_add(
            accounting.added_facet_count,
            group.coverage_delta->added_facet_point_ids.size(),
            "the persistent reduced-Gamma facet delta count overflows");
        accounting.added_point_reference_count = checked_add(
            accounting.added_point_reference_count,
            group.coverage_delta->added_point_ids.size(),
            "the persistent reduced-Gamma point delta count overflows");
        for (const FacetLabel& facet :
             group.coverage_delta->added_facet_point_ids) {
          if (globally_added_facets.contains(facet) ||
              !pending_added_facets.insert(facet).second) {
            throw std::logic_error(
                "one Gamma facet is added to persistent coverage twice");
          }
        }
      }

      pending_records.push_back(mutation.record);
      if (mutation.node.has_value()) {
        pending_nodes.push_back(*mutation.node);
      }
      pending.push_back(std::move(mutation));
    }

    // No active-root mutation occurs until all groups above have been fully
    // resolved against the immutable pre-batch snapshot.
    metadata.all_groups_resolved_before_mutation =
        pending.size() == batch.groups.size() &&
        std::all_of(
            pending.begin(),
            pending.end(),
            [](const PendingMutation& mutation) {
              return mutation.record.resolved_from_pre_batch_snapshot;
            });
    ActiveRootMap post_batch_roots = active_roots;
    for (const PendingMutation& mutation : pending) {
      if (mutation.kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
        continue;
      }
      for (const std::size_t prior_root_id : mutation.prior_root_ids) {
        if (post_batch_roots.erase(prior_root_id) != 1U) {
          throw std::logic_error(
              "a resolved reduced-Gamma root is absent at mutation time");
        }
      }
      if (!mutation.resulting_root_id.has_value() ||
          !post_batch_roots
               .emplace(
                   *mutation.resulting_root_id,
                   mutation.resulting_root)
               .second) {
        throw std::logic_error(
            "a resolved reduced-Gamma root mutation collides");
      }
    }
    metadata.created_node_count = pending_nodes.size();
    if (!pending_nodes.empty()) {
      metadata.first_created_node_id = pending_nodes.front().node_id;
    }
    metadata.closed_nontrivial_component_count =
        nontrivial_component_count(
            batch.gamma_transition.closed_components);
    metadata.post_batch_root_bijection_certified =
        roots_match_nontrivial_components(
            post_batch_roots,
            batch.gamma_transition.closed_components,
            order);
    if (!metadata.post_batch_root_bijection_certified) {
      throw std::logic_error(
          "persistent roots do not match the closed nontrivial Gamma "
          "components after a batch");
    }
    metadata.active_root_count_after = post_batch_roots.size();

    // Atomic commit: persistent roots, compact records, topology, global
    // counters and global uniqueness sets all change only after every group
    // has been resolved and the complete post-batch partition validated.
    active_roots = std::move(post_batch_roots);
    history.nodes.insert(
        history.nodes.end(), pending_nodes.begin(), pending_nodes.end());
    history.group_records.insert(
        history.group_records.end(),
        pending_records.begin(),
        pending_records.end());
    globally_added_facets.insert(
        pending_added_facets.begin(), pending_added_facets.end());
    globally_newly_active_facets.insert(
        pending_newly_active_facets.begin(),
        pending_newly_active_facets.end());
    globally_affected_cofaces.insert(
        pending_affected_cofaces.begin(),
        pending_affected_cofaces.end());
    consumed_child_node_ids.insert(
        pending_consumed_child_node_ids.begin(),
        pending_consumed_child_node_ids.end());
    ++history.counters.reduced_gamma_batch_build_count;
    ++history.counters.pre_batch_root_bijection_check_count;
    ++history.counters.post_batch_root_bijection_check_count;
    history.counters.reduced_gamma_group_count = checked_add(
        history.counters.reduced_gamma_group_count,
        pending.size(),
        "the persistent reduced-Gamma group count overflows");
    history.counters.deferred_group_count = checked_add(
        history.counters.deferred_group_count,
        accounting.deferred_group_count,
        "the deferred group count overflows");
    history.counters.birth_group_count = checked_add(
        history.counters.birth_group_count,
        accounting.birth_group_count,
        "the birth group count overflows");
    history.counters.continuation_group_count = checked_add(
        history.counters.continuation_group_count,
        accounting.continuation_group_count,
        "the continuation group count overflows");
    history.counters.multifusion_group_count = checked_add(
        history.counters.multifusion_group_count,
        accounting.multifusion_group_count,
        "the multifusion group count overflows");
    history.counters.fully_redundant_group_count = checked_add(
        history.counters.fully_redundant_group_count,
        accounting.fully_redundant_group_count,
        "the fully-redundant group count overflows");
    history.counters.group_root_reference_count = checked_add(
        history.counters.group_root_reference_count,
        accounting.group_root_reference_count,
        "the compact root-reference count overflows");
    history.counters.group_newly_active_facet_count = checked_add(
        history.counters.group_newly_active_facet_count,
        accounting.group_newly_active_facet_count,
        "the compact facet-reference count overflows");
    history.counters.group_equal_level_coface_count = checked_add(
        history.counters.group_equal_level_coface_count,
        accounting.group_equal_level_coface_count,
        "the compact coface-reference count overflows");
    history.counters.coverage_delta_count = checked_add(
        history.counters.coverage_delta_count,
        accounting.coverage_delta_count,
        "the coverage-delta count overflows");
    history.counters.added_facet_count = checked_add(
        history.counters.added_facet_count,
        accounting.added_facet_count,
        "the added-facet count overflows");
    history.counters.added_point_reference_count = checked_add(
        history.counters.added_point_reference_count,
        accounting.added_point_reference_count,
        "the added-point count overflows");
    all_groups_frozen =
        all_groups_frozen &&
        metadata.all_groups_resolved_before_mutation;
    all_post_batch_bijections =
        all_post_batch_bijections &&
        metadata.post_batch_root_bijection_certified;
    all_mutations_deferred =
        all_mutations_deferred &&
        metadata.all_groups_resolved_before_mutation;
    history.batch_metadata.push_back(std::move(metadata));
    ++completed_batch_count;
  }

  history.counters.history_group_record_count =
      history.group_records.size();
  history.counters.created_node_count = history.nodes.size();
  history.counters.child_reference_count = 0U;
  bool dense_ids = true;
  bool children_precede = true;
  for (std::size_t node_index = 0U;
       node_index < history.nodes.size();
       ++node_index) {
    const ExactPersistentReducedGammaNode& node =
        history.nodes[node_index];
    dense_ids =
        dense_ids && node.node_id == node_index &&
        node.children_resolved_from_pre_batch_snapshot;
    history.counters.child_reference_count = checked_add(
        history.counters.child_reference_count,
        node.child_node_ids.size(),
        "the persistent reduced-Gamma child count overflows");
    children_precede =
        children_precede &&
        std::all_of(
            node.child_node_ids.begin(),
            node.child_node_ids.end(),
            [&](std::size_t child_id) {
              return child_id < node.node_id;
            });
  }
  history.counters.consumed_child_count =
      consumed_child_node_ids.size();
  history.node_ids_dense_and_deterministic = dense_ids;
  history.children_precede_parent = children_precede;
  history.each_child_consumed_at_most_once =
      history.counters.consumed_child_count ==
      history.counters.child_reference_count;
  history.all_reduced_gamma_batches_fresh_replay_certified =
      all_batches_certified &&
      completed_batch_count ==
          history.activation_levels.size();
  history.groups_resolved_against_pre_batch_snapshots =
      all_pre_batch_bijections && all_groups_frozen;
  history.mutations_applied_after_complete_batch_resolution =
      all_mutations_deferred;
  history.active_roots_match_nontrivial_components_after_every_batch =
      all_pre_batch_bijections && all_post_batch_bijections;

  history.final_active_roots.reserve(active_roots.size());
  for (const auto& [root_id, root] : active_roots) {
    static_cast<void>(root_id);
    history.final_active_roots.push_back(root);
  }
  history.counters.final_active_root_count =
      history.final_active_roots.size();

  FacetSet all_facets;
  all_facets.reserve(history.high_cut_gamma->active_facets.size());
  for (const ExactStrictGammaFacetWitness& facet :
       history.high_cut_gamma->active_facets) {
    all_facets.push_back(facet.facet_point_ids);
  }
  std::sort(all_facets.begin(), all_facets.end());
  const std::set<FacetLabel> all_facet_set{
      all_facets.begin(), all_facets.end()};
  std::set<FacetLabel> all_cofaces;
  for (const ExactStrictGammaCofaceWitness& coface :
       history.high_cut_gamma->active_cofaces) {
    all_cofaces.insert(coface.coface_point_ids);
  }
  const std::vector<PointId> all_points = covered_point_ids(all_facets);
  history.final_single_root_covers_all_facets_and_points =
      history.final_active_roots.size() == 1U &&
      history.final_active_roots.front().facet_point_ids == all_facets &&
      history.final_active_roots.front().covered_point_ids == all_points &&
      all_points.size() == cloud.size();
  history.coverage_deltas_accounted_exactly =
      history.counters.added_facet_count ==
          history.exhaustive_facet_count &&
      globally_added_facets.size() == history.exhaustive_facet_count &&
      std::equal(
          globally_added_facets.begin(),
          globally_added_facets.end(),
          all_facets.begin(),
          all_facets.end()) &&
      history.counters.added_point_reference_count <=
          history.required_delta_point_reference_capacity;
  history.every_exhaustive_coface_affected_exactly_once =
      globally_affected_cofaces == all_cofaces &&
      history.counters.group_equal_level_coface_count ==
          history.exhaustive_coface_count;

  bool exact_group_effects = true;
  std::size_t observed_non_deferred_delta_count = 0U;
  std::size_t observed_fully_redundant_count = 0U;
  for (std::size_t record_index = 0U;
       record_index < history.group_records.size();
       ++record_index) {
    const ExactPersistentReducedGammaHistoryGroupRecord& record =
        history.group_records[record_index];
    const bool has_result = record.resulting_root_node_id.has_value();
    const bool has_created = record.created_node_id.has_value();
    const bool has_delta = record.coverage_delta.has_value();
    exact_group_effects =
        exact_group_effects &&
        record.group_record_index == record_index &&
        record.resolved_from_pre_batch_snapshot;
    switch (record.kind) {
      case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
        exact_group_effects =
            exact_group_effects && record.prior_root_node_ids.empty() &&
            !has_result && !has_created && !has_delta &&
            record.newly_active_facet_point_ids.size() == 1U &&
            record.equal_level_coface_point_ids.empty();
        break;
      case ExactReducedGammaBatchGroupKind::birth:
        {
          const bool node_matches =
              has_created && *record.created_node_id < history.nodes.size() &&
              history.nodes[*record.created_node_id].kind ==
                  ExactPersistentReducedGammaNodeKind::birth &&
              history.nodes[*record.created_node_id]
                  .child_node_ids.empty() &&
              history.nodes[*record.created_node_id].creation_batch_index ==
                  record.batch_index &&
              history.nodes[*record.created_node_id].creation_group_index ==
                  record.batch_group_index &&
              history.nodes[*record.created_node_id].squared_level ==
                  record.squared_level;
        exact_group_effects =
            exact_group_effects && record.prior_root_node_ids.empty() &&
            has_result && has_created && has_delta &&
            !record.equal_level_coface_point_ids.empty() &&
            record.resulting_root_node_id == record.created_node_id &&
            node_matches;
        break;
        }
      case ExactReducedGammaBatchGroupKind::continuation:
        exact_group_effects =
            exact_group_effects &&
            record.prior_root_node_ids.size() == 1U && has_result &&
            !has_created && has_delta &&
            !record.equal_level_coface_point_ids.empty() &&
            *record.resulting_root_node_id ==
                record.prior_root_node_ids.front();
        break;
      case ExactReducedGammaBatchGroupKind::multifusion:
        {
          const bool node_matches =
              has_created && *record.created_node_id < history.nodes.size() &&
              history.nodes[*record.created_node_id].kind ==
                  ExactPersistentReducedGammaNodeKind::multifusion &&
              history.nodes[*record.created_node_id].child_node_ids ==
                  record.prior_root_node_ids &&
              history.nodes[*record.created_node_id].creation_batch_index ==
                  record.batch_index &&
              history.nodes[*record.created_node_id].creation_group_index ==
                  record.batch_group_index &&
              history.nodes[*record.created_node_id].squared_level ==
                  record.squared_level;
        exact_group_effects =
            exact_group_effects &&
            record.prior_root_node_ids.size() >= 2U && has_result &&
            has_created && has_delta &&
            !record.equal_level_coface_point_ids.empty() &&
            record.resulting_root_node_id == record.created_node_id &&
            node_matches;
        break;
        }
    }
    if (record.kind != ExactReducedGammaBatchGroupKind::
                           deferred_isolated_facet) {
      observed_non_deferred_delta_count += has_delta ? 1U : 0U;
    }
    if (has_delta && record.coverage_delta->fully_redundant) {
      ++observed_fully_redundant_count;
    }
  }
  history.group_kinds_have_exact_persistent_effects = exact_group_effects;
  history.every_non_deferred_group_has_exactly_one_coverage_delta =
      observed_non_deferred_delta_count +
              history.counters.deferred_group_count ==
          history.group_records.size() &&
      history.counters.coverage_delta_count ==
          observed_non_deferred_delta_count;
  history.fully_redundant_groups_preserved =
      observed_fully_redundant_count ==
      history.counters.fully_redundant_group_count;

  const std::size_t classified_group_count = checked_add(
      checked_add(
          history.counters.deferred_group_count,
          history.counters.birth_group_count,
          "the persistent reduced-Gamma group count overflows"),
      checked_add(
          history.counters.continuation_group_count,
          history.counters.multifusion_group_count,
          "the persistent reduced-Gamma group count overflows"),
      "the persistent reduced-Gamma group count overflows");
  const std::size_t non_deferred_group_count =
      history.group_records.size() -
      history.counters.deferred_group_count;
  if (classified_group_count != history.group_records.size() ||
      history.group_records.size() !=
          history.counters.reduced_gamma_group_count ||
      history.group_records.size() > history.required_group_capacity ||
      history.nodes.size() > history.required_node_capacity ||
      history.counters.child_reference_count >
          history.required_child_reference_capacity ||
      history.counters.group_root_reference_count >
          history.required_group_root_reference_capacity ||
      history.counters.group_newly_active_facet_count !=
          history.exhaustive_facet_count ||
      globally_newly_active_facets != all_facet_set ||
      history.counters.group_newly_active_facet_count >
          history.required_group_newly_active_facet_capacity ||
      history.counters.group_equal_level_coface_count >
          history.required_group_equal_level_coface_capacity ||
      history.counters.added_facet_count >
          history.required_delta_facet_capacity ||
      history.counters.added_point_reference_count >
          history.required_delta_point_reference_capacity ||
      history.nodes.empty() ||
      history.counters.child_reference_count + 1U !=
          history.nodes.size() ||
      history.counters.group_root_reference_count + 1U !=
          non_deferred_group_count) {
    throw std::logic_error(
        "the persistent reduced-Gamma genealogy violated its exact "
        "capacity or tree accounting");
  }

  history.persistent_reduced_gamma_history_certified =
      history.candidate_space_size_certified &&
      history.preflight_budget_sufficient &&
      history.geometry_started_after_successful_preflight &&
      history.high_cut_equals_twice_exact_squared_diameter &&
      history.high_cut_strictly_above_all_activation_levels &&
      history.high_cut_catalog_exhaustive &&
      history.activation_levels_canonical_and_complete &&
      history.all_reduced_gamma_batches_fresh_replay_certified &&
      history.groups_resolved_against_pre_batch_snapshots &&
      history.mutations_applied_after_complete_batch_resolution &&
      history.active_roots_match_nontrivial_components_after_every_batch &&
      history.node_ids_dense_and_deterministic &&
      history.children_precede_parent &&
      history.each_child_consumed_at_most_once &&
      history.every_exhaustive_coface_affected_exactly_once &&
      history.group_kinds_have_exact_persistent_effects &&
      history.every_non_deferred_group_has_exactly_one_coverage_delta &&
      history.fully_redundant_groups_preserved &&
      history.coverage_deltas_accounted_exactly &&
      history.final_single_root_covers_all_facets_and_points &&
      !history.terminal_order_complete_empty;
  if (!history.persistent_reduced_gamma_history_certified) {
    throw std::logic_error(
        "the all-level reduced-Gamma genealogy failed certification");
  }
  history.decision = ExactPersistentReducedGammaOrderHistoryDecision::
      complete_persistent_reduced_gamma_history;
  return history;
}

[[nodiscard]] bool result_facts_match(
    const ExactPersistentReducedGammaOrderHistory& observed,
    const ExactPersistentReducedGammaOrderHistory& expected) {
  return observed.candidate_space_size_certified ==
             expected.candidate_space_size_certified &&
         observed.preflight_budget_sufficient ==
             expected.preflight_budget_sufficient &&
         observed.geometry_started_after_successful_preflight ==
             expected.geometry_started_after_successful_preflight &&
         observed.high_cut_equals_twice_exact_squared_diameter ==
             expected.high_cut_equals_twice_exact_squared_diameter &&
         observed.high_cut_strictly_above_all_activation_levels ==
             expected.high_cut_strictly_above_all_activation_levels &&
         observed.high_cut_catalog_exhaustive ==
             expected.high_cut_catalog_exhaustive &&
         observed.activation_levels_canonical_and_complete ==
             expected.activation_levels_canonical_and_complete &&
         observed.all_reduced_gamma_batches_fresh_replay_certified ==
             expected.all_reduced_gamma_batches_fresh_replay_certified &&
         observed.groups_resolved_against_pre_batch_snapshots ==
             expected.groups_resolved_against_pre_batch_snapshots &&
         observed.mutations_applied_after_complete_batch_resolution ==
             expected.mutations_applied_after_complete_batch_resolution &&
         observed.active_roots_match_nontrivial_components_after_every_batch ==
             expected.active_roots_match_nontrivial_components_after_every_batch &&
         observed.node_ids_dense_and_deterministic ==
             expected.node_ids_dense_and_deterministic &&
         observed.children_precede_parent ==
             expected.children_precede_parent &&
         observed.each_child_consumed_at_most_once ==
             expected.each_child_consumed_at_most_once &&
         observed.every_exhaustive_coface_affected_exactly_once ==
             expected.every_exhaustive_coface_affected_exactly_once &&
         observed.group_kinds_have_exact_persistent_effects ==
             expected.group_kinds_have_exact_persistent_effects &&
         observed.every_non_deferred_group_has_exactly_one_coverage_delta ==
             expected.every_non_deferred_group_has_exactly_one_coverage_delta &&
         observed.fully_redundant_groups_preserved ==
             expected.fully_redundant_groups_preserved &&
         observed.coverage_deltas_accounted_exactly ==
             expected.coverage_deltas_accounted_exactly &&
         observed.final_single_root_covers_all_facets_and_points ==
             expected.final_single_root_covers_all_facets_and_points &&
         observed.terminal_order_complete_empty ==
             expected.terminal_order_complete_empty &&
         observed.persistent_reduced_gamma_history_certified ==
             expected.persistent_reduced_gamma_history_certified;
}

}  // namespace

ExactPersistentReducedGammaOrderHistoryVerification
verify_exact_persistent_reduced_gamma_order_history(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& history) {
  const ExactPersistentReducedGammaOrderHistory expected =
      compute_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget);
  ExactPersistentReducedGammaOrderHistoryVerification verification;
  verification.requested_budget_certified =
      history.requested_budget == budget &&
      history.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      history.point_count == cloud.size() &&
      history.point_count == expected.point_count &&
      history.order == order && history.order == expected.order;
  verification.derived_preflight_sizes_certified =
      history.exhaustive_facet_count == expected.exhaustive_facet_count &&
      history.exhaustive_coface_count == expected.exhaustive_coface_count &&
      history.exhaustive_union_attempt_count ==
          expected.exhaustive_union_attempt_count &&
      history.required_activation_level_capacity ==
          expected.required_activation_level_capacity &&
      history.required_total_facet_work_capacity ==
          expected.required_total_facet_work_capacity &&
      history.required_total_coface_work_capacity ==
          expected.required_total_coface_work_capacity &&
      history.required_total_union_work_capacity ==
          expected.required_total_union_work_capacity &&
      history.required_node_capacity == expected.required_node_capacity &&
      history.required_child_reference_capacity ==
          expected.required_child_reference_capacity &&
      history.required_group_root_reference_capacity ==
          expected.required_group_root_reference_capacity &&
      history.required_group_capacity == expected.required_group_capacity &&
      history.required_group_newly_active_facet_capacity ==
          expected.required_group_newly_active_facet_capacity &&
      history.required_group_equal_level_coface_capacity ==
          expected.required_group_equal_level_coface_capacity &&
      history.required_delta_facet_capacity ==
          expected.required_delta_facet_capacity &&
      history.required_delta_point_reference_capacity ==
          expected.required_delta_point_reference_capacity;

  verification.high_cut_gamma_certified =
      history.exact_diameter_squared ==
          expected.exact_diameter_squared &&
      history.high_cut_squared_level == expected.high_cut_squared_level &&
      history.high_cut_gamma == expected.high_cut_gamma;
  // Only the freshly reconstructed expected branch may select a subordinate
  // verifier. An observed decision is untrusted and must never steer a
  // potentially invalid order/cut replay. Equality above also gates the
  // replay so a falsified payload is rejected without unnecessary work.
  if (expected.decision ==
          ExactPersistentReducedGammaOrderHistoryDecision::
              complete_persistent_reduced_gamma_history &&
      verification.high_cut_gamma_certified) {
    if (!history.high_cut_squared_level.has_value() ||
        !history.high_cut_gamma.has_value()) {
      verification.high_cut_gamma_certified = false;
    } else {
      const std::vector<FacetLabel> sources{
          first_canonical_facet(order)};
      const ExactStrictGammaVerification high_cut_verification =
          verify_exact_strict_gamma_source_classification(
              cloud,
              order,
              *history.high_cut_squared_level,
              sources,
              budget.gamma_budget,
              *history.high_cut_gamma);
      verification.high_cut_gamma_certified =
          verification.high_cut_gamma_certified &&
          high_cut_verification.exact_strict_gamma_decision_certified;
    }
  }

  verification.activation_levels_certified =
      history.activation_levels == expected.activation_levels;
  // compute_exact_persistent_reduced_gamma_order_history has just rebuilt
  // every 6.13 batch with its own fresh verifier. The batches remain local;
  // only their compact projections below are compared.
  verification.transient_reduced_gamma_batches_replayed_certified =
      (expected.decision !=
           ExactPersistentReducedGammaOrderHistoryDecision::
               complete_persistent_reduced_gamma_history ||
       expected.all_reduced_gamma_batches_fresh_replay_certified) &&
      history.all_reduced_gamma_batches_fresh_replay_certified ==
          expected.all_reduced_gamma_batches_fresh_replay_certified;
  verification.nodes_certified = history.nodes == expected.nodes;
  verification.group_records_certified =
      history.group_records == expected.group_records;
  verification.batch_metadata_certified =
      history.batch_metadata == expected.batch_metadata;
  verification.final_active_roots_certified =
      history.final_active_roots == expected.final_active_roots;
  verification.result_facts_certified =
      result_facts_match(history, expected);
  verification.counters_certified =
      history.counters == expected.counters;
  verification.decision_certified =
      history.decision == expected.decision;
  verification.scope_certified =
      history.scope ==
          ExactPersistentReducedGammaOrderHistoryScope::
              bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only &&
      history.scope == expected.scope;
  verification.fresh_replay_certified = history == expected;
  verification.
      exact_persistent_reduced_gamma_order_history_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.high_cut_gamma_certified &&
      verification.activation_levels_certified &&
      verification.transient_reduced_gamma_batches_replayed_certified &&
      verification.nodes_certified &&
      verification.group_records_certified &&
      verification.batch_metadata_certified &&
      verification.final_active_roots_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactPersistentReducedGammaOrderHistory
build_exact_persistent_reduced_gamma_order_history(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget) {
  ExactPersistentReducedGammaOrderHistory history =
      compute_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget);
  const ExactPersistentReducedGammaOrderHistoryVerification verification =
      verify_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget, history);
  if (!verification.
          exact_persistent_reduced_gamma_order_history_decision_certified) {
    throw std::logic_error(
        "the exact persistent reduced-Gamma history failed fresh replay");
  }
  return history;
}

}  // namespace morsehgp3d::hierarchy
