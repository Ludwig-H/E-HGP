#include "morsehgp3d/hierarchy/reduced_gamma_cut.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;
using FacetSet = std::vector<FacetLabel>;
using ActiveRoot = ExactPersistentReducedGammaActiveRoot;
using ActiveRootMap = std::map<std::size_t, ActiveRoot>;
using RootFacetCountMap = std::map<std::size_t, std::size_t>;

struct PrefixAnalysis {
  std::size_t batch_count{};
  std::size_t group_record_count{};
  std::size_t node_record_count{};
  std::size_t prior_root_reference_count{};
  std::size_t child_reference_count{};
  std::size_t newly_active_facet_count{};
  std::size_t equal_level_coface_count{};
  std::size_t delta_facet_count{};
  std::size_t delta_point_reference_count{};
  std::size_t non_deferred_group_count{};
  std::size_t fully_redundant_group_count{};
  std::size_t peak_active_root_count{};
  std::size_t final_active_root_count{};
  std::size_t output_facet_reference_count{};
  std::size_t output_point_reference_capacity{};
  std::size_t facet_replay_work_count{};
  std::size_t point_id_replay_work_count{};
  std::size_t result_incidence_facet_check_count{};
  std::size_t result_incidence_point_id_work_count{};
  std::optional<std::size_t> sole_active_root_id;
};

struct PendingRootCountMutation {
  std::vector<std::size_t> prior_root_ids;
  std::size_t resulting_root_id{};
  std::size_t resulting_facet_count{};
};

struct PendingRootMutation {
  std::vector<std::size_t> prior_root_ids;
  std::size_t resulting_root_id{};
  ActiveRoot resulting_root;
};

[[nodiscard]] bool try_add(
    std::size_t& value,
    std::size_t increment) noexcept {
  if (increment > std::numeric_limits<std::size_t>::max() - value) {
    return false;
  }
  value += increment;
  return true;
}

[[nodiscard]] bool try_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
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
    value *= point_count - subset_size + factor;
    value /= factor;
  }
  return value;
}

void validate_boundary(ExactReducedGammaCutBoundary boundary) {
  switch (boundary) {
    case ExactReducedGammaCutBoundary::strict_open:
    case ExactReducedGammaCutBoundary::closed:
      return;
  }
  throw std::invalid_argument("an exact reduced-Gamma cut has an invalid boundary");
}

void validate_budget_caps(const ExactReducedGammaCutBudget& budget) {
  if (budget.maximum_batch_count >
          ExactReducedGammaCutBudget::maximum_supported_batch_count ||
      budget.maximum_group_record_count >
          ExactReducedGammaCutBudget::maximum_supported_group_record_count ||
      budget.maximum_node_record_count >
          ExactReducedGammaCutBudget::maximum_supported_node_record_count ||
      budget.maximum_prior_root_reference_count >
          ExactReducedGammaCutBudget::
              maximum_supported_prior_root_reference_count ||
      budget.maximum_child_reference_count >
          ExactReducedGammaCutBudget::maximum_supported_child_reference_count ||
      budget.maximum_newly_active_facet_count >
          ExactReducedGammaCutBudget::
              maximum_supported_newly_active_facet_count ||
      budget.maximum_equal_level_coface_count >
          ExactReducedGammaCutBudget::
              maximum_supported_equal_level_coface_count ||
      budget.maximum_delta_facet_count >
          ExactReducedGammaCutBudget::maximum_supported_delta_facet_count ||
      budget.maximum_delta_point_reference_count >
          ExactReducedGammaCutBudget::
              maximum_supported_delta_point_reference_count ||
      budget.maximum_active_root_count >
          ExactReducedGammaCutBudget::maximum_supported_active_root_count ||
      budget.maximum_output_facet_reference_count >
          ExactReducedGammaCutBudget::
              maximum_supported_output_facet_reference_count ||
      budget.maximum_output_point_reference_count >
          ExactReducedGammaCutBudget::
              maximum_supported_output_point_reference_count ||
      budget.maximum_facet_replay_work_count >
          ExactReducedGammaCutBudget::
              maximum_supported_facet_replay_work_count ||
      budget.maximum_point_id_replay_work_count >
          ExactReducedGammaCutBudget::
              maximum_supported_point_id_replay_work_count ||
      budget.maximum_result_incidence_facet_check_count >
          ExactReducedGammaCutBudget::
              maximum_supported_result_incidence_facet_check_count ||
      budget.maximum_result_incidence_point_id_work_count >
          ExactReducedGammaCutBudget::
              maximum_supported_result_incidence_point_id_work_count) {
    throw std::invalid_argument(
        "an exact reduced-Gamma cut budget exceeds its bounded reference cap");
  }
}

[[nodiscard]] bool canonical_point_ids(
    const std::vector<PointId>& point_ids,
    std::size_t point_count) {
  return std::is_sorted(point_ids.begin(), point_ids.end()) &&
         std::adjacent_find(point_ids.begin(), point_ids.end()) ==
             point_ids.end() &&
         std::all_of(
             point_ids.begin(),
             point_ids.end(),
             [point_count](PointId point_id) {
               return static_cast<std::size_t>(point_id) < point_count;
             });
}

[[nodiscard]] bool canonical_label(
    const FacetLabel& label,
    std::size_t cardinality,
    std::size_t point_count) {
  return label.size() == cardinality &&
         canonical_point_ids(label, point_count);
}

[[nodiscard]] bool canonical_label_set(
    const FacetSet& labels,
    std::size_t cardinality,
    std::size_t point_count) {
  return std::all_of(
             labels.begin(),
             labels.end(),
             [&](const FacetLabel& label) {
               return canonical_label(label, cardinality, point_count);
             }) &&
         std::is_sorted(labels.begin(), labels.end()) &&
         std::adjacent_find(labels.begin(), labels.end()) == labels.end();
}

[[nodiscard]] std::vector<PointId> covered_point_ids(
    const FacetSet& facets) {
  std::set<PointId> points;
  for (const FacetLabel& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return {points.begin(), points.end()};
}

[[nodiscard]] bool source_claims_accepted(
    const ExactPersistentReducedGammaOrderHistory& history) {
  if (history.point_count <
          ExactPersistentReducedGammaOrderHistory::
              minimum_supported_point_count ||
      history.point_count >
          ExactPersistentReducedGammaOrderHistory::
              maximum_supported_point_count ||
      history.order <
          ExactPersistentReducedGammaOrderHistory::minimum_supported_order ||
      history.order >
          ExactPersistentReducedGammaOrderHistory::maximum_supported_order ||
      history.order > history.point_count ||
      history.scope != ExactPersistentReducedGammaOrderHistoryScope::
                           bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only ||
      !history.candidate_space_size_certified ||
      !history.preflight_budget_sufficient ||
      !history.activation_levels_canonical_and_complete ||
      !history.all_reduced_gamma_batches_fresh_replay_certified ||
      !history.groups_resolved_against_pre_batch_snapshots ||
      !history.mutations_applied_after_complete_batch_resolution ||
      !history.active_roots_match_nontrivial_components_after_every_batch ||
      !history.node_ids_dense_and_deterministic ||
      !history.children_precede_parent ||
      !history.each_child_consumed_at_most_once ||
      !history.every_exhaustive_coface_affected_exactly_once ||
      !history.group_kinds_have_exact_persistent_effects ||
      !history.every_non_deferred_group_has_exactly_one_coverage_delta ||
      !history.fully_redundant_groups_preserved ||
      !history.persistent_reduced_gamma_history_certified) {
    return false;
  }

  if (history.order == history.point_count) {
    return history.decision ==
               ExactPersistentReducedGammaOrderHistoryDecision::
                   complete_empty_terminal_order &&
           history.terminal_order_complete_empty &&
           !history.geometry_started_after_successful_preflight &&
           !history.coverage_deltas_accounted_exactly &&
           !history.final_single_root_covers_all_facets_and_points;
  }

  return history.decision ==
             ExactPersistentReducedGammaOrderHistoryDecision::
                 complete_persistent_reduced_gamma_history &&
         history.geometry_started_after_successful_preflight &&
         history.high_cut_equals_twice_exact_squared_diameter &&
         history.high_cut_strictly_above_all_activation_levels &&
         history.high_cut_catalog_exhaustive &&
         history.coverage_deltas_accounted_exactly &&
         history.final_single_root_covers_all_facets_and_points &&
         !history.terminal_order_complete_empty;
}

[[nodiscard]] bool source_terminal_shape_accepted(
    const ExactPersistentReducedGammaOrderHistory& history) {
  return history.exhaustive_facet_count == 1U &&
         history.exhaustive_coface_count == 0U &&
         history.exhaustive_union_attempt_count == 0U &&
         !history.exact_diameter_squared.has_value() &&
         !history.high_cut_squared_level.has_value() &&
         !history.high_cut_gamma.has_value() &&
         history.activation_levels.empty() && history.nodes.empty() &&
         history.group_records.empty() && history.batch_metadata.empty() &&
         history.final_active_roots.empty() &&
         history.counters.activation_level_count == 0U &&
         history.counters.reduced_gamma_batch_build_count == 0U &&
         history.counters.history_group_record_count == 0U &&
         history.counters.created_node_count == 0U &&
         history.counters.final_active_root_count == 0U;
}

[[nodiscard]] bool source_normal_container_shape_accepted(
    const ExactPersistentReducedGammaOrderHistory& history) {
  const std::size_t point_count = history.point_count;
  const std::size_t order = history.order;
  const std::size_t facet_count = bounded_binomial(point_count, order);
  const std::size_t coface_count = bounded_binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_cap = facet_count + coface_count;

  if (history.exhaustive_facet_count != facet_count ||
      history.exhaustive_coface_count != coface_count ||
      history.exhaustive_union_attempt_count != union_count ||
      history.activation_levels.empty() ||
      history.activation_levels.size() > level_cap ||
      history.activation_levels.size() >
          ExactReducedGammaCutBudget::maximum_supported_batch_count ||
      history.batch_metadata.size() != history.activation_levels.size() ||
      history.group_records.size() > level_cap ||
      history.group_records.size() >
          ExactReducedGammaCutBudget::maximum_supported_group_record_count ||
      history.nodes.size() > coface_count ||
      history.nodes.size() >
          ExactReducedGammaCutBudget::maximum_supported_node_record_count ||
      history.final_active_roots.size() != 1U ||
      !history.exact_diameter_squared.has_value() ||
      !history.high_cut_squared_level.has_value() ||
      !history.high_cut_gamma.has_value() ||
      !std::is_sorted(
          history.activation_levels.begin(),
          history.activation_levels.end()) ||
      std::adjacent_find(
          history.activation_levels.begin(),
          history.activation_levels.end()) != history.activation_levels.end()) {
    return false;
  }

  std::size_t expected_group_start = 0U;
  std::size_t expected_node_start = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < history.batch_metadata.size();
       ++batch_index) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[batch_index];
    if (metadata.batch_index != batch_index ||
        metadata.activation_level_index != batch_index ||
        metadata.squared_level != history.activation_levels[batch_index] ||
        metadata.first_group_record_index != expected_group_start ||
        metadata.group_record_count == 0U ||
        metadata.group_record_count >
            history.group_records.size() - expected_group_start ||
        metadata.created_node_count >
            history.nodes.size() - expected_node_start ||
        metadata.active_root_count_before >
            ExactReducedGammaCutBudget::maximum_supported_active_root_count ||
        metadata.active_root_count_after >
            ExactReducedGammaCutBudget::maximum_supported_active_root_count ||
        metadata.strict_nontrivial_component_count !=
            metadata.active_root_count_before ||
        metadata.closed_nontrivial_component_count !=
            metadata.active_root_count_after ||
        !metadata.pre_batch_root_bijection_certified ||
        !metadata.all_groups_resolved_before_mutation ||
        !metadata.post_batch_root_bijection_certified) {
      return false;
    }
    if (metadata.created_node_count == 0U) {
      if (metadata.first_created_node_id.has_value()) {
        return false;
      }
    } else if (!metadata.first_created_node_id.has_value() ||
               *metadata.first_created_node_id != expected_node_start) {
      return false;
    }
    for (std::size_t local_group_index = 1U;
         local_group_index < metadata.group_record_count;
         ++local_group_index) {
      const std::size_t previous_index =
          expected_group_start + local_group_index - 1U;
      const std::size_t current_index =
          expected_group_start + local_group_index;
      if (history.group_records[previous_index]
                  .canonical_representative_facet_point_ids.size() != order ||
          history.group_records[current_index]
                  .canonical_representative_facet_point_ids.size() != order ||
          !(history.group_records[previous_index]
                .canonical_representative_facet_point_ids <
            history.group_records[current_index]
                .canonical_representative_facet_point_ids)) {
        return false;
      }
    }
    expected_group_start += metadata.group_record_count;
    expected_node_start += metadata.created_node_count;
  }
  if (expected_group_start != history.group_records.size() ||
      expected_node_start != history.nodes.size()) {
    return false;
  }

  for (std::size_t node_index = 0U; node_index < history.nodes.size();
       ++node_index) {
    const ExactPersistentReducedGammaNode& node = history.nodes[node_index];
    if (node.node_id != node_index ||
        node.creation_batch_index >= history.batch_metadata.size() ||
        node.creation_group_index >=
            history.batch_metadata[node.creation_batch_index]
                .group_record_count ||
        node.squared_level !=
            history.activation_levels[node.creation_batch_index] ||
        !node.children_resolved_from_pre_batch_snapshot ||
        node.child_node_ids.size() > node_index ||
        !std::is_sorted(node.child_node_ids.begin(), node.child_node_ids.end()) ||
        std::adjacent_find(
            node.child_node_ids.begin(), node.child_node_ids.end()) !=
            node.child_node_ids.end() ||
        std::any_of(
            node.child_node_ids.begin(),
            node.child_node_ids.end(),
            [node_index](std::size_t child_id) {
              return child_id >= node_index;
            }) ||
        (node.kind == ExactPersistentReducedGammaNodeKind::birth &&
         !node.child_node_ids.empty()) ||
        (node.kind == ExactPersistentReducedGammaNodeKind::multifusion &&
         node.child_node_ids.size() < 2U)) {
      return false;
    }
  }

  std::set<FacetLabel> all_newly_active_facets;
  std::set<FacetLabel> all_equal_level_cofaces;
  std::set<FacetLabel> all_delta_facets;
  std::size_t child_reference_count = 0U;
  std::size_t prior_reference_count = 0U;
  std::size_t delta_point_reference_count = 0U;
  std::size_t non_deferred_count = 0U;
  std::size_t fully_redundant_count = 0U;
  std::size_t birth_count = 0U;
  std::size_t continuation_count = 0U;
  std::size_t multifusion_count = 0U;
  std::size_t deferred_count = 0U;
  std::size_t expected_created_node_id = 0U;
  for (const ExactPersistentReducedGammaNode& node : history.nodes) {
    if (!try_add(child_reference_count, node.child_node_ids.size()) ||
        child_reference_count > ExactReducedGammaCutBudget::
                                    maximum_supported_child_reference_count) {
      return false;
    }
  }

  for (std::size_t record_index = 0U;
       record_index < history.group_records.size();
       ++record_index) {
    const ExactPersistentReducedGammaHistoryGroupRecord& record =
        history.group_records[record_index];
    if (record.group_record_index != record_index ||
        record.batch_index >= history.batch_metadata.size()) {
      return false;
    }
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[record.batch_index];
    if (record.batch_group_index >= metadata.group_record_count ||
        metadata.first_group_record_index + record.batch_group_index !=
            record_index ||
        record.squared_level != history.activation_levels[record.batch_index] ||
        !record.resolved_from_pre_batch_snapshot ||
        record.prior_root_node_ids.size() > history.nodes.size() ||
        record.newly_active_facet_point_ids.size() > facet_count ||
        record.equal_level_coface_point_ids.size() > coface_count ||
        !canonical_label(
            record.canonical_representative_facet_point_ids,
            order,
            point_count) ||
        !std::is_sorted(
            record.prior_root_node_ids.begin(),
            record.prior_root_node_ids.end()) ||
        std::adjacent_find(
            record.prior_root_node_ids.begin(),
            record.prior_root_node_ids.end()) !=
            record.prior_root_node_ids.end() ||
        std::any_of(
            record.prior_root_node_ids.begin(),
            record.prior_root_node_ids.end(),
            [&](std::size_t root_id) {
              return root_id >= history.nodes.size();
            }) ||
        !canonical_label_set(
            record.newly_active_facet_point_ids, order, point_count) ||
        !canonical_label_set(
            record.equal_level_coface_point_ids,
            order + 1U,
            point_count)) {
      return false;
    }
    for (const FacetLabel& facet : record.newly_active_facet_point_ids) {
      if (!all_newly_active_facets.insert(facet).second) {
        return false;
      }
    }
    for (const FacetLabel& coface : record.equal_level_coface_point_ids) {
      if (!all_equal_level_cofaces.insert(coface).second) {
        return false;
      }
    }
    if (all_newly_active_facets.size() > facet_count ||
        all_equal_level_cofaces.size() > coface_count) {
      return false;
    }
    if (!try_add(prior_reference_count, record.prior_root_node_ids.size()) ||
        prior_reference_count > ExactReducedGammaCutBudget::
                                    maximum_supported_prior_root_reference_count) {
      return false;
    }

    const bool has_result = record.resulting_root_node_id.has_value();
    const bool has_created = record.created_node_id.has_value();
    const bool has_delta = record.coverage_delta.has_value();
    switch (record.kind) {
      case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
        if (!record.prior_root_node_ids.empty() || has_result || has_created ||
            has_delta || record.newly_active_facet_point_ids.size() != 1U ||
            !record.equal_level_coface_point_ids.empty() ||
            record.canonical_representative_facet_point_ids !=
                record.newly_active_facet_point_ids.front()) {
          return false;
        }
        ++deferred_count;
        break;
      case ExactReducedGammaBatchGroupKind::birth:
        if (!record.prior_root_node_ids.empty() || !has_result ||
            !has_created || !has_delta ||
            record.resulting_root_node_id != record.created_node_id ||
            record.equal_level_coface_point_ids.empty()) {
          return false;
        }
        ++birth_count;
        ++non_deferred_count;
        break;
      case ExactReducedGammaBatchGroupKind::continuation:
        if (record.prior_root_node_ids.size() != 1U || !has_result ||
            has_created || !has_delta ||
            *record.resulting_root_node_id !=
                record.prior_root_node_ids.front() ||
            record.equal_level_coface_point_ids.empty()) {
          return false;
        }
        ++continuation_count;
        ++non_deferred_count;
        break;
      case ExactReducedGammaBatchGroupKind::multifusion:
        if (record.prior_root_node_ids.size() < 2U || !has_result ||
            !has_created || !has_delta ||
            record.resulting_root_node_id != record.created_node_id ||
            record.equal_level_coface_point_ids.empty()) {
          return false;
        }
        ++multifusion_count;
        ++non_deferred_count;
        break;
    }

    if (has_created) {
      if (*record.created_node_id >= history.nodes.size() ||
          *record.created_node_id != expected_created_node_id) {
        return false;
      }
      ++expected_created_node_id;
      const ExactPersistentReducedGammaNode& node =
          history.nodes[*record.created_node_id];
      const ExactPersistentReducedGammaNodeKind expected_kind =
          record.kind == ExactReducedGammaBatchGroupKind::birth
              ? ExactPersistentReducedGammaNodeKind::birth
              : ExactPersistentReducedGammaNodeKind::multifusion;
      if (node.creation_batch_index != record.batch_index ||
          node.creation_group_index != record.batch_group_index ||
          node.squared_level != record.squared_level ||
          node.kind != expected_kind ||
          node.child_node_ids != record.prior_root_node_ids) {
        return false;
      }
    }
    if (has_result && *record.resulting_root_node_id >= history.nodes.size()) {
      return false;
    }
    if (has_delta) {
      const ExactReducedGammaCoverageDelta& delta = *record.coverage_delta;
      if (delta.added_facet_point_ids.size() > facet_count ||
          delta.added_point_ids.size() > point_count ||
          !canonical_label_set(
              delta.added_facet_point_ids, order, point_count) ||
          !canonical_point_ids(delta.added_point_ids, point_count) ||
          delta.fully_redundant !=
              (delta.added_facet_point_ids.empty() &&
               delta.added_point_ids.empty()) ||
          !try_add(
              delta_point_reference_count,
              delta.added_point_ids.size()) ||
          delta_point_reference_count > ExactReducedGammaCutBudget::
                                              maximum_supported_delta_point_reference_count) {
        return false;
      }
      for (const FacetLabel& facet : delta.added_facet_point_ids) {
        if (!all_delta_facets.insert(facet).second) {
          return false;
        }
      }
      if (all_delta_facets.size() > facet_count) {
        return false;
      }
      if (delta.fully_redundant) {
        ++fully_redundant_count;
      }
    }
  }

  const ActiveRoot& final_root = history.final_active_roots.front();
  if (final_root.root_node_id >= history.nodes.size() ||
      final_root.facet_point_ids.size() != facet_count ||
      final_root.covered_point_ids.size() != point_count ||
      !canonical_label_set(final_root.facet_point_ids, order, point_count) ||
      !canonical_point_ids(final_root.covered_point_ids, point_count) ||
      final_root.covered_point_ids != covered_point_ids(final_root.facet_point_ids) ||
      all_newly_active_facets.size() != facet_count ||
      all_equal_level_cofaces.size() != coface_count ||
      all_delta_facets.size() != facet_count ||
      child_reference_count >
          ExactReducedGammaCutBudget::maximum_supported_child_reference_count ||
      prior_reference_count > ExactReducedGammaCutBudget::
                                  maximum_supported_prior_root_reference_count ||
      delta_point_reference_count >
          ExactReducedGammaCutBudget::
              maximum_supported_delta_point_reference_count ||
      history.counters.activation_level_count != history.activation_levels.size() ||
      history.counters.reduced_gamma_batch_build_count !=
          history.batch_metadata.size() ||
      history.counters.history_group_record_count !=
          history.group_records.size() ||
      history.counters.created_node_count != history.nodes.size() ||
      history.counters.child_reference_count != child_reference_count ||
      history.counters.group_root_reference_count != prior_reference_count ||
      history.counters.group_newly_active_facet_count != facet_count ||
      history.counters.group_equal_level_coface_count != coface_count ||
      history.counters.added_facet_count != facet_count ||
      history.counters.added_point_reference_count !=
          delta_point_reference_count ||
      history.counters.deferred_group_count != deferred_count ||
      history.counters.birth_group_count != birth_count ||
      history.counters.continuation_group_count != continuation_count ||
      history.counters.multifusion_group_count != multifusion_count ||
      history.counters.fully_redundant_group_count != fully_redundant_count ||
      history.counters.final_active_root_count != 1U ||
      expected_created_node_id != history.nodes.size() ||
      non_deferred_count !=
          history.group_records.size() - deferred_count) {
    return false;
  }
  return true;
}

[[nodiscard]] bool analyze_prefix_without_root_payloads(
    const ExactPersistentReducedGammaOrderHistory& history,
    std::size_t batch_prefix_count,
    PrefixAnalysis& analysis) {
  if (batch_prefix_count > history.batch_metadata.size()) {
    return false;
  }
  analysis = {};
  analysis.batch_count = batch_prefix_count;
  RootFacetCountMap active_root_facet_counts;
  std::size_t active_node_prefix_count = 0U;

  for (std::size_t batch_index = 0U;
       batch_index < batch_prefix_count;
       ++batch_index) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[batch_index];
    if (metadata.active_root_count_before !=
        active_root_facet_counts.size()) {
      return false;
    }

    std::vector<PendingRootCountMutation> pending;
    pending.reserve(metadata.group_record_count);
    std::set<std::size_t> referenced_snapshot_roots;
    std::set<std::size_t> pending_result_root_ids;
    std::size_t created_in_batch = 0U;
    for (std::size_t local_group_index = 0U;
         local_group_index < metadata.group_record_count;
         ++local_group_index) {
      const std::size_t record_index =
          metadata.first_group_record_index + local_group_index;
      if (record_index >= history.group_records.size()) {
        return false;
      }
      const ExactPersistentReducedGammaHistoryGroupRecord& record =
          history.group_records[record_index];
      if (!try_add(analysis.group_record_count, 1U) ||
          !try_add(
              analysis.prior_root_reference_count,
              record.prior_root_node_ids.size()) ||
          !try_add(
              analysis.newly_active_facet_count,
              record.newly_active_facet_point_ids.size()) ||
          !try_add(
              analysis.equal_level_coface_count,
              record.equal_level_coface_point_ids.size())) {
        return false;
      }
      for (const std::size_t prior_root_id : record.prior_root_node_ids) {
        if (prior_root_id >= active_node_prefix_count ||
            !active_root_facet_counts.contains(prior_root_id) ||
            !referenced_snapshot_roots.insert(prior_root_id).second) {
          return false;
        }
      }

      if (record.kind == ExactReducedGammaBatchGroupKind::
                             deferred_isolated_facet) {
        continue;
      }
      if (!record.resulting_root_node_id.has_value() ||
          !record.coverage_delta.has_value()) {
        return false;
      }
      ++analysis.non_deferred_group_count;
      const ExactReducedGammaCoverageDelta& delta = *record.coverage_delta;
      if (!try_add(
              analysis.delta_facet_count,
              delta.added_facet_point_ids.size()) ||
          !try_add(
              analysis.delta_point_reference_count,
              delta.added_point_ids.size())) {
        return false;
      }
      if (delta.fully_redundant) {
        ++analysis.fully_redundant_group_count;
      }
      std::size_t coface_incidence_check_count = 0U;
      if (!try_multiply(
              history.order + 1U,
              record.equal_level_coface_point_ids.size(),
              coface_incidence_check_count) ||
          !try_add(
              analysis.result_incidence_facet_check_count,
              record.newly_active_facet_point_ids.size()) ||
          !try_add(
              analysis.result_incidence_facet_check_count,
              coface_incidence_check_count)) {
        return false;
      }

      std::size_t resulting_facet_count =
          delta.added_facet_point_ids.size();
      for (const std::size_t prior_root_id :
           record.prior_root_node_ids) {
        if (!try_add(
                resulting_facet_count,
                active_root_facet_counts.at(prior_root_id))) {
          return false;
        }
      }
      if (resulting_facet_count < history.order + 1U ||
          !try_add(
              analysis.facet_replay_work_count,
              resulting_facet_count)) {
        return false;
      }
      std::size_t group_point_work = 0U;
      if (!try_multiply(
              history.order,
              resulting_facet_count,
              group_point_work) ||
          !try_add(
              analysis.point_id_replay_work_count,
              group_point_work)) {
        return false;
      }

      PendingRootCountMutation mutation;
      mutation.prior_root_ids = record.prior_root_node_ids;
      mutation.resulting_root_id = *record.resulting_root_node_id;
      mutation.resulting_facet_count = resulting_facet_count;
      if (!pending_result_root_ids.insert(mutation.resulting_root_id).second) {
        return false;
      }
      if (record.created_node_id.has_value()) {
        ++created_in_batch;
      }
      pending.push_back(std::move(mutation));
    }

    if (created_in_batch != metadata.created_node_count ||
        !try_add(analysis.node_record_count, created_in_batch)) {
      return false;
    }
    active_node_prefix_count = analysis.node_record_count;
    for (const PendingRootCountMutation& mutation : pending) {
      for (const std::size_t prior_root_id : mutation.prior_root_ids) {
        if (active_root_facet_counts.erase(prior_root_id) != 1U) {
          return false;
        }
      }
      if (!active_root_facet_counts
               .emplace(
                   mutation.resulting_root_id,
                   mutation.resulting_facet_count)
               .second) {
        return false;
      }
    }
    if (active_root_facet_counts.size() !=
            metadata.active_root_count_after ||
        std::any_of(
            active_root_facet_counts.begin(),
            active_root_facet_counts.end(),
            [active_node_prefix_count](const auto& entry) {
              return entry.first >= active_node_prefix_count;
            }) ||
        active_root_facet_counts.size() >
            ExactReducedGammaCutBudget::maximum_supported_active_root_count) {
      return false;
    }
    analysis.peak_active_root_count = std::max(
        analysis.peak_active_root_count,
        active_root_facet_counts.size());
  }

  if (batch_prefix_count != 0U) {
    const ExactPersistentReducedGammaBatchMetadata& final_metadata =
        history.batch_metadata[batch_prefix_count - 1U];
    analysis.group_record_count =
        final_metadata.first_group_record_index +
        final_metadata.group_record_count;
    analysis.node_record_count = final_metadata.created_node_count == 0U
                                     ? analysis.node_record_count
                                     : *final_metadata.first_created_node_id +
                                           final_metadata.created_node_count;
  }
  for (std::size_t node_index = 0U;
       node_index < analysis.node_record_count;
       ++node_index) {
    if (node_index >= history.nodes.size() ||
        !try_add(
            analysis.child_reference_count,
            history.nodes[node_index].child_node_ids.size())) {
      return false;
    }
  }

  analysis.final_active_root_count = active_root_facet_counts.size();
  for (const auto& [root_id, facet_count] : active_root_facet_counts) {
    static_cast<void>(root_id);
    if (!try_add(analysis.output_facet_reference_count, facet_count)) {
      return false;
    }
  }
  if (active_root_facet_counts.size() == 1U) {
    analysis.sole_active_root_id = active_root_facet_counts.begin()->first;
  }
  std::size_t point_bound_by_roots = 0U;
  std::size_t point_bound_by_facets = 0U;
  if (!try_multiply(
          history.point_count,
          analysis.final_active_root_count,
          point_bound_by_roots) ||
      !try_multiply(
          history.order,
          analysis.output_facet_reference_count,
          point_bound_by_facets)) {
    return false;
  }
  analysis.output_point_reference_capacity =
      std::min(point_bound_by_roots, point_bound_by_facets);
  if (!try_multiply(
          history.order,
          analysis.result_incidence_facet_check_count,
          analysis.result_incidence_point_id_work_count)) {
    return false;
  }

  if (analysis.output_facet_reference_count !=
          analysis.delta_facet_count ||
      analysis.non_deferred_group_count <
          analysis.final_active_root_count ||
      analysis.node_record_count < analysis.final_active_root_count ||
      analysis.prior_root_reference_count !=
          analysis.non_deferred_group_count -
              analysis.final_active_root_count ||
      analysis.child_reference_count !=
          analysis.node_record_count - analysis.final_active_root_count ||
      analysis.batch_count >
          ExactReducedGammaCutBudget::maximum_supported_batch_count ||
      analysis.group_record_count >
          ExactReducedGammaCutBudget::maximum_supported_group_record_count ||
      analysis.node_record_count >
          ExactReducedGammaCutBudget::maximum_supported_node_record_count ||
      analysis.prior_root_reference_count > ExactReducedGammaCutBudget::
                                                maximum_supported_prior_root_reference_count ||
      analysis.child_reference_count > ExactReducedGammaCutBudget::
                                           maximum_supported_child_reference_count ||
      analysis.newly_active_facet_count > ExactReducedGammaCutBudget::
                                               maximum_supported_newly_active_facet_count ||
      analysis.equal_level_coface_count > ExactReducedGammaCutBudget::
                                              maximum_supported_equal_level_coface_count ||
      analysis.delta_facet_count >
          ExactReducedGammaCutBudget::maximum_supported_delta_facet_count ||
      analysis.delta_point_reference_count > ExactReducedGammaCutBudget::
                                                  maximum_supported_delta_point_reference_count ||
      analysis.output_facet_reference_count > ExactReducedGammaCutBudget::
                                                   maximum_supported_output_facet_reference_count ||
      analysis.output_point_reference_capacity > ExactReducedGammaCutBudget::
                                                      maximum_supported_output_point_reference_count ||
      analysis.facet_replay_work_count > ExactReducedGammaCutBudget::
                                             maximum_supported_facet_replay_work_count ||
      analysis.point_id_replay_work_count > ExactReducedGammaCutBudget::
                                                maximum_supported_point_id_replay_work_count ||
      analysis.result_incidence_facet_check_count >
          ExactReducedGammaCutBudget::
              maximum_supported_result_incidence_facet_check_count ||
      analysis.result_incidence_point_id_work_count >
          ExactReducedGammaCutBudget::
              maximum_supported_result_incidence_point_id_work_count) {
    return false;
  }
  return true;
}

[[nodiscard]] bool budget_covers(
    const ExactReducedGammaCutBudget& budget,
    const PrefixAnalysis& analysis) {
  return budget.maximum_batch_count >= analysis.batch_count &&
         budget.maximum_group_record_count >= analysis.group_record_count &&
         budget.maximum_node_record_count >= analysis.node_record_count &&
         budget.maximum_prior_root_reference_count >=
             analysis.prior_root_reference_count &&
         budget.maximum_child_reference_count >=
             analysis.child_reference_count &&
         budget.maximum_newly_active_facet_count >=
             analysis.newly_active_facet_count &&
         budget.maximum_equal_level_coface_count >=
             analysis.equal_level_coface_count &&
         budget.maximum_delta_facet_count >= analysis.delta_facet_count &&
         budget.maximum_delta_point_reference_count >=
             analysis.delta_point_reference_count &&
         budget.maximum_active_root_count >=
             analysis.peak_active_root_count &&
         budget.maximum_output_facet_reference_count >=
             analysis.output_facet_reference_count &&
         budget.maximum_output_point_reference_count >=
             analysis.output_point_reference_capacity &&
         budget.maximum_facet_replay_work_count >=
             analysis.facet_replay_work_count &&
         budget.maximum_point_id_replay_work_count >=
             analysis.point_id_replay_work_count &&
         budget.maximum_result_incidence_facet_check_count >=
             analysis.result_incidence_facet_check_count &&
         budget.maximum_result_incidence_point_id_work_count >=
             analysis.result_incidence_point_id_work_count;
}

void assign_requirements(
    ExactReducedGammaCut& cut,
    const PrefixAnalysis& analysis) {
  cut.required_batch_capacity = analysis.batch_count;
  cut.required_group_record_capacity = analysis.group_record_count;
  cut.required_node_record_capacity = analysis.node_record_count;
  cut.required_prior_root_reference_capacity =
      analysis.prior_root_reference_count;
  cut.required_child_reference_capacity = analysis.child_reference_count;
  cut.required_newly_active_facet_capacity =
      analysis.newly_active_facet_count;
  cut.required_equal_level_coface_capacity =
      analysis.equal_level_coface_count;
  cut.required_delta_facet_capacity = analysis.delta_facet_count;
  cut.required_delta_point_reference_capacity =
      analysis.delta_point_reference_count;
  cut.required_active_root_capacity = analysis.peak_active_root_count;
  cut.required_output_facet_reference_capacity =
      analysis.output_facet_reference_count;
  cut.required_output_point_reference_capacity =
      analysis.output_point_reference_capacity;
  cut.required_facet_replay_work_capacity =
      analysis.facet_replay_work_count;
  cut.required_point_id_replay_work_capacity =
      analysis.point_id_replay_work_count;
  cut.required_result_incidence_facet_check_capacity =
      analysis.result_incidence_facet_check_count;
  cut.required_result_incidence_point_id_work_capacity =
      analysis.result_incidence_point_id_work_count;
}

[[nodiscard]] bool assign_global_source_audit(
    ExactReducedGammaCut& cut,
    const ExactPersistentReducedGammaOrderHistory& history,
    const PrefixAnalysis& full_analysis) {
  cut.counters.global_structure_activation_level_count =
      history.activation_levels.size();
  cut.counters.global_structure_batch_metadata_count =
      history.batch_metadata.size();
  cut.counters.global_structure_node_record_count = history.nodes.size();
  cut.counters.global_structure_group_record_count =
      history.group_records.size();

  std::size_t three_facet_count = 0U;
  std::size_t label_validation_count = history.group_records.size();
  if (!try_multiply(3U, history.exhaustive_facet_count, three_facet_count) ||
      !try_add(label_validation_count, three_facet_count) ||
      !try_add(label_validation_count, history.exhaustive_coface_count)) {
    return false;
  }
  cut.counters.global_structure_label_validation_count =
      label_validation_count;

  std::size_t representative_point_references = 0U;
  std::size_t facet_point_references = 0U;
  std::size_t coface_point_references = 0U;
  std::size_t point_reference_count = 0U;
  if (!try_multiply(
          history.order,
          history.group_records.size(),
          representative_point_references) ||
      !try_multiply(
          history.order,
          three_facet_count,
          facet_point_references) ||
      !try_multiply(
          history.order + 1U,
          history.exhaustive_coface_count,
          coface_point_references) ||
      !try_add(point_reference_count, representative_point_references) ||
      !try_add(point_reference_count, facet_point_references) ||
      !try_add(point_reference_count, coface_point_references) ||
      !try_add(
          point_reference_count,
          history.counters.added_point_reference_count) ||
      !try_add(point_reference_count, history.point_count)) {
    return false;
  }
  cut.counters.global_structure_point_id_reference_validation_count =
      point_reference_count;
  cut.counters.global_dry_batch_count = full_analysis.batch_count;
  cut.counters.global_dry_group_record_count =
      full_analysis.group_record_count;
  cut.counters.global_dry_node_record_count =
      full_analysis.node_record_count;
  cut.counters.global_dry_prior_root_reference_count =
      full_analysis.prior_root_reference_count;
  cut.counters.global_dry_child_reference_count =
      full_analysis.child_reference_count;
  cut.counters.global_dry_facet_state_work_count =
      full_analysis.facet_replay_work_count;
  cut.global_source_structure_audit_completed_before_prefix_selection = true;
  return true;
}

[[nodiscard]] ExactReducedGammaCut make_base_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget) {
  ExactReducedGammaCut cut;
  cut.requested_budget = budget;
  cut.point_count = history.point_count;
  cut.order = history.order;
  cut.squared_level = squared_level;
  cut.boundary = boundary;
  cut.scope = ExactReducedGammaCutScope::
      bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only;
  cut.counters.source_history_gate_check_count = 1U;
  return cut;
}

[[nodiscard]] ExactReducedGammaCut source_rejected_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget) {
  ExactReducedGammaCut cut =
      make_base_cut(history, squared_level, boundary, budget);
  cut.decision = ExactReducedGammaCutDecision::
      source_history_claims_or_structure_rejected;
  return cut;
}

[[nodiscard]] bool replay_prefix(
    const ExactPersistentReducedGammaOrderHistory& history,
    const PrefixAnalysis& analysis,
    ExactReducedGammaCut& cut) {
  ActiveRootMap active_roots;
  std::set<FacetLabel> all_output_facets;
  std::size_t replayed_node_prefix_count = 0U;

  for (std::size_t batch_index = 0U;
       batch_index < analysis.batch_count;
       ++batch_index) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[batch_index];
    if (active_roots.size() != metadata.active_root_count_before) {
      return false;
    }
    std::vector<PendingRootMutation> pending;
    pending.reserve(metadata.group_record_count);
    std::set<std::size_t> referenced_snapshot_roots;
    std::set<std::size_t> pending_result_ids;

    for (std::size_t local_group_index = 0U;
         local_group_index < metadata.group_record_count;
         ++local_group_index) {
      const std::size_t record_index =
          metadata.first_group_record_index + local_group_index;
      if (record_index >= history.group_records.size()) {
        return false;
      }
      const ExactPersistentReducedGammaHistoryGroupRecord& record =
          history.group_records[record_index];
      ++cut.counters.replayed_group_record_count;
      if (!try_add(
              cut.counters.replayed_prior_root_reference_count,
              record.prior_root_node_ids.size()) ||
          !try_add(
              cut.counters.replayed_newly_active_facet_count,
              record.newly_active_facet_point_ids.size()) ||
          !try_add(
              cut.counters.replayed_equal_level_coface_count,
              record.equal_level_coface_point_ids.size())) {
        return false;
      }
      for (const std::size_t prior_root_id : record.prior_root_node_ids) {
        if (prior_root_id >= replayed_node_prefix_count ||
            !active_roots.contains(prior_root_id) ||
            !referenced_snapshot_roots.insert(prior_root_id).second) {
          return false;
        }
      }
      if (record.kind == ExactReducedGammaBatchGroupKind::
                             deferred_isolated_facet) {
        continue;
      }
      if (!record.resulting_root_node_id.has_value() ||
          !record.coverage_delta.has_value()) {
        return false;
      }

      const ExactReducedGammaCoverageDelta& delta = *record.coverage_delta;
      if (!try_add(
              cut.counters.replayed_delta_facet_count,
              delta.added_facet_point_ids.size()) ||
          !try_add(
              cut.counters.replayed_delta_point_reference_count,
              delta.added_point_ids.size())) {
        return false;
      }
      if (delta.fully_redundant) {
        ++cut.counters.fully_redundant_group_count;
      }

      FacetSet resulting_facets;
      std::size_t resulting_facet_count =
          delta.added_facet_point_ids.size();
      for (const std::size_t prior_root_id : record.prior_root_node_ids) {
        if (!try_add(
                resulting_facet_count,
                active_roots.at(prior_root_id).facet_point_ids.size())) {
          return false;
        }
      }
      resulting_facets.reserve(resulting_facet_count);
      std::set<PointId> prior_points;
      std::set<PointId> resulting_points;
      for (const std::size_t prior_root_id : record.prior_root_node_ids) {
        const ActiveRoot& prior_root = active_roots.at(prior_root_id);
        resulting_facets.insert(
            resulting_facets.end(),
            prior_root.facet_point_ids.begin(),
            prior_root.facet_point_ids.end());
        for (const FacetLabel& facet : prior_root.facet_point_ids) {
          for (const PointId point_id : facet) {
            prior_points.insert(point_id);
            resulting_points.insert(point_id);
          }
        }
      }
      resulting_facets.insert(
          resulting_facets.end(),
          delta.added_facet_point_ids.begin(),
          delta.added_facet_point_ids.end());
      for (const FacetLabel& facet : delta.added_facet_point_ids) {
        resulting_points.insert(facet.begin(), facet.end());
      }
      std::sort(resulting_facets.begin(), resulting_facets.end());
      if (resulting_facets.size() != resulting_facet_count ||
          resulting_facets.empty() ||
          record.canonical_representative_facet_point_ids !=
              resulting_facets.front() ||
          std::adjacent_find(
              resulting_facets.begin(), resulting_facets.end()) !=
              resulting_facets.end()) {
        return false;
      }
      for (const FacetLabel& newly_active_facet :
           record.newly_active_facet_point_ids) {
        if (!std::binary_search(
                resulting_facets.begin(),
                resulting_facets.end(),
                newly_active_facet) ||
            !std::binary_search(
                delta.added_facet_point_ids.begin(),
                delta.added_facet_point_ids.end(),
                newly_active_facet) ||
            !try_add(
                cut.counters.result_incidence_facet_check_count,
                1U) ||
            !try_add(
                cut.counters.result_incidence_point_id_work_count,
                history.order)) {
          return false;
        }
      }
      for (const FacetLabel& coface :
           record.equal_level_coface_point_ids) {
        for (std::size_t omitted_index = 0U;
             omitted_index < coface.size();
             ++omitted_index) {
          FacetLabel facet;
          facet.reserve(history.order);
          for (std::size_t point_index = 0U;
               point_index < coface.size();
               ++point_index) {
            if (point_index != omitted_index) {
              facet.push_back(coface[point_index]);
            }
          }
          if (!std::binary_search(
                  resulting_facets.begin(),
                  resulting_facets.end(),
                  facet) ||
              !try_add(
                  cut.counters.result_incidence_facet_check_count,
                  1U) ||
              !try_add(
                  cut.counters.result_incidence_point_id_work_count,
                  history.order)) {
            return false;
          }
        }
      }
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
        return false;
      }

      PendingRootMutation mutation;
      mutation.prior_root_ids = record.prior_root_node_ids;
      mutation.resulting_root_id = *record.resulting_root_node_id;
      mutation.resulting_root.root_node_id = mutation.resulting_root_id;
      mutation.resulting_root.facet_point_ids =
          std::move(resulting_facets);
      mutation.resulting_root.covered_point_ids.assign(
          resulting_points.begin(), resulting_points.end());
      if (!pending_result_ids.insert(mutation.resulting_root_id).second ||
          !try_add(
              cut.counters.facet_replay_work_count,
              resulting_facet_count)) {
        return false;
      }
      std::size_t point_work = 0U;
      if (!try_multiply(history.order, resulting_facet_count, point_work) ||
          !try_add(cut.counters.point_id_replay_work_count, point_work)) {
        return false;
      }
      pending.push_back(std::move(mutation));
    }

    // The map above remains the immutable pre-batch snapshot until every
    // group has been resolved.  Commit in place avoids cloning every root at
    // every activation level.
    if (!try_add(
            replayed_node_prefix_count,
            metadata.created_node_count)) {
      return false;
    }
    for (PendingRootMutation& mutation : pending) {
      for (const std::size_t prior_root_id : mutation.prior_root_ids) {
        if (active_roots.erase(prior_root_id) != 1U) {
          return false;
        }
      }
      if (!active_roots
               .emplace(
                   mutation.resulting_root_id,
                   std::move(mutation.resulting_root))
               .second) {
        return false;
      }
    }
    if (active_roots.size() != metadata.active_root_count_after) {
      return false;
    }
    if (std::any_of(
            active_roots.begin(),
            active_roots.end(),
            [replayed_node_prefix_count](const auto& entry) {
              return entry.first >= replayed_node_prefix_count;
            })) {
      return false;
    }
    ++cut.counters.replayed_batch_count;
    ++cut.counters.frozen_root_snapshot_count;
    if (!try_add(
            cut.counters.applied_root_mutation_count,
            pending.size())) {
      return false;
    }
    cut.counters.peak_active_root_count = std::max(
        cut.counters.peak_active_root_count,
        active_roots.size());
  }

  cut.counters.replayed_node_record_count = analysis.node_record_count;
  cut.counters.replayed_child_reference_count =
      analysis.child_reference_count;
  for (const auto& [root_id, root] : active_roots) {
    if (root.root_node_id != root_id ||
        !canonical_label_set(
            root.facet_point_ids, history.order, history.point_count) ||
        !canonical_point_ids(root.covered_point_ids, history.point_count)) {
      return false;
    }
    for (const FacetLabel& facet : root.facet_point_ids) {
      if (!all_output_facets.insert(facet).second) {
        return false;
      }
    }
    if (!try_add(
            cut.counters.output_facet_reference_count,
            root.facet_point_ids.size()) ||
        !try_add(
            cut.counters.output_point_reference_count,
            root.covered_point_ids.size())) {
      return false;
    }
  }
  cut.active_roots.reserve(active_roots.size());
  for (auto& [root_id, root] : active_roots) {
    static_cast<void>(root_id);
    cut.active_roots.push_back(std::move(root));
  }
  cut.counters.final_active_root_count = cut.active_roots.size();

  const bool counters_match =
      cut.counters.replayed_batch_count == analysis.batch_count &&
      cut.counters.replayed_group_record_count ==
          analysis.group_record_count &&
      cut.counters.replayed_node_record_count ==
          analysis.node_record_count &&
      cut.counters.replayed_prior_root_reference_count ==
          analysis.prior_root_reference_count &&
      cut.counters.replayed_child_reference_count ==
          analysis.child_reference_count &&
      cut.counters.replayed_newly_active_facet_count ==
          analysis.newly_active_facet_count &&
      cut.counters.replayed_equal_level_coface_count ==
          analysis.equal_level_coface_count &&
      cut.counters.replayed_delta_facet_count ==
          analysis.delta_facet_count &&
      cut.counters.replayed_delta_point_reference_count ==
          analysis.delta_point_reference_count &&
      cut.counters.fully_redundant_group_count ==
          analysis.fully_redundant_group_count &&
      cut.counters.peak_active_root_count ==
          analysis.peak_active_root_count &&
      cut.counters.final_active_root_count ==
          analysis.final_active_root_count &&
      cut.counters.output_facet_reference_count ==
          analysis.output_facet_reference_count &&
      cut.counters.output_point_reference_count <=
          analysis.output_point_reference_capacity &&
      cut.counters.facet_replay_work_count ==
          analysis.facet_replay_work_count &&
      cut.counters.point_id_replay_work_count ==
          analysis.point_id_replay_work_count &&
      cut.counters.result_incidence_facet_check_count ==
          analysis.result_incidence_facet_check_count &&
      cut.counters.result_incidence_point_id_work_count ==
          analysis.result_incidence_point_id_work_count;
  if (!counters_match) {
    return false;
  }

  cut.complete_batches_replayed_from_frozen_snapshots =
      cut.counters.frozen_root_snapshot_count == analysis.batch_count;
  cut.coverage_deltas_applied_exactly = true;
  cut.persistent_root_ids_preserved = true;
  cut.active_roots_canonical_and_disjoint_by_facet =
      all_output_facets.size() ==
      cut.counters.output_facet_reference_count;
  cut.prefix_forest_accounting_certified =
      analysis.prior_root_reference_count ==
          analysis.non_deferred_group_count -
              analysis.final_active_root_count &&
      analysis.child_reference_count ==
          analysis.node_record_count - analysis.final_active_root_count &&
      cut.counters.output_facet_reference_count ==
          analysis.delta_facet_count;
  return cut.complete_batches_replayed_from_frozen_snapshots &&
         cut.coverage_deltas_applied_exactly &&
         cut.persistent_root_ids_preserved &&
         cut.active_roots_canonical_and_disjoint_by_facet &&
         cut.prefix_forest_accounting_certified;
}

void mark_source_assumption(ExactReducedGammaCut& cut) {
  cut.source_history_claims_and_structure_accepted = true;
  cut.source_history_certification_is_external_assumption = true;
  cut.source_history_geometry_not_freshly_certified = true;
  cut.coherent_forged_history_cannot_be_excluded_without_cloud = true;
}

void mark_complete_empty_replay(ExactReducedGammaCut& cut) {
  cut.preflight_budget_sufficient = true;
  cut.complete_batches_replayed_from_frozen_snapshots = true;
  cut.coverage_deltas_applied_exactly = true;
  cut.persistent_root_ids_preserved = true;
  cut.active_roots_canonical_and_disjoint_by_facet = true;
  cut.prefix_forest_accounting_certified = true;
  cut.cursor_matches_replayed_prefix = true;
  cut.journal_relative_cut_replay_certified = true;
}

[[nodiscard]] ExactReducedGammaCut compute_exact_reduced_gamma_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget) {
  validate_boundary(boundary);
  validate_budget_caps(budget);

  if (!source_claims_accepted(history)) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  if (history.order == history.point_count) {
    if (!source_terminal_shape_accepted(history)) {
      return source_rejected_cut(history, squared_level, boundary, budget);
    }
    ExactReducedGammaCut cut =
        make_base_cut(history, squared_level, boundary, budget);
    mark_source_assumption(cut);
    cut.global_source_structure_audit_completed_before_prefix_selection =
        true;
    cut.counters.exact_prefix_search_count = 1U;
    cut.counters.preflight_count = 1U;
    cut.cursor.selected_by_exact_lower_bound =
        boundary == ExactReducedGammaCutBoundary::strict_open;
    cut.cursor.selected_by_exact_upper_bound =
        boundary == ExactReducedGammaCutBoundary::closed;
    cut.prefix_selected_from_exact_threshold_and_boundary = true;
    cut.terminal_order_complete_empty = true;
    mark_complete_empty_replay(cut);
    cut.decision =
        ExactReducedGammaCutDecision::complete_empty_terminal_order;
    return cut;
  }

  if (!source_normal_container_shape_accepted(history)) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  // A scalar-only full-journal pass rejects bad root ranges and forest
  // accounting before exact prefix selection can index any batch record. It
  // does not authenticate geometric levels or Delta semantics against a
  // cloud.
  PrefixAnalysis full_analysis;
  if (!analyze_prefix_without_root_payloads(
          history, history.batch_metadata.size(), full_analysis) ||
      full_analysis.final_active_root_count != 1U ||
      !full_analysis.sole_active_root_id.has_value() ||
      *full_analysis.sole_active_root_id !=
          history.final_active_roots.front().root_node_id ||
      full_analysis.output_facet_reference_count !=
          history.exhaustive_facet_count) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }

  ExactReducedGammaCut cut =
      make_base_cut(history, squared_level, boundary, budget);
  mark_source_assumption(cut);
  if (!assign_global_source_audit(cut, history, full_analysis)) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  cut.counters.exact_prefix_search_count = 1U;
  const auto prefix_end =
      boundary == ExactReducedGammaCutBoundary::strict_open
          ? std::lower_bound(
                history.activation_levels.begin(),
                history.activation_levels.end(),
                squared_level)
          : std::upper_bound(
                history.activation_levels.begin(),
                history.activation_levels.end(),
                squared_level);
  const std::size_t prefix_count = static_cast<std::size_t>(
      std::distance(history.activation_levels.begin(), prefix_end));
  cut.cursor.activation_level_prefix_count = prefix_count;
  cut.cursor.batch_prefix_count = prefix_count;
  cut.cursor.selected_by_exact_lower_bound =
      boundary == ExactReducedGammaCutBoundary::strict_open;
  cut.cursor.selected_by_exact_upper_bound =
      boundary == ExactReducedGammaCutBoundary::closed;
  if (prefix_end != history.activation_levels.end()) {
    cut.cursor.first_excluded_squared_level = *prefix_end;
  }
  cut.prefix_selected_from_exact_threshold_and_boundary = true;

  if (prefix_count == 0U) {
    cut.counters.preflight_count = 1U;
    cut.empty_prefix_complete = true;
    mark_complete_empty_replay(cut);
    cut.decision = ExactReducedGammaCutDecision::complete_empty_prefix;
    return cut;
  }

  PrefixAnalysis prefix_analysis;
  if (!analyze_prefix_without_root_payloads(
          history, prefix_count, prefix_analysis)) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  cut.cursor.group_record_prefix_count =
      prefix_analysis.group_record_count;
  cut.cursor.node_record_prefix_count = prefix_analysis.node_record_count;
  assign_requirements(cut, prefix_analysis);
  cut.counters.preflight_count = 1U;
  cut.preflight_budget_sufficient = budget_covers(budget, prefix_analysis);
  if (!cut.preflight_budget_sufficient) {
    cut.decision = ExactReducedGammaCutDecision::
        no_cut_preflight_budget_insufficient;
    return cut;
  }

  cut.root_replay_started_after_successful_preflight = true;
  if (!replay_prefix(history, prefix_analysis, cut)) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  cut.cursor_matches_replayed_prefix =
      cut.cursor.activation_level_prefix_count ==
          cut.counters.replayed_batch_count &&
      cut.cursor.batch_prefix_count == cut.counters.replayed_batch_count &&
      cut.cursor.group_record_prefix_count ==
          cut.counters.replayed_group_record_count &&
      cut.cursor.node_record_prefix_count ==
          cut.counters.replayed_node_record_count;
  cut.journal_relative_cut_replay_certified =
      cut.source_history_claims_and_structure_accepted &&
      cut.source_history_certification_is_external_assumption &&
      cut.source_history_geometry_not_freshly_certified &&
      cut.coherent_forged_history_cannot_be_excluded_without_cloud &&
      cut.global_source_structure_audit_completed_before_prefix_selection &&
      cut.prefix_selected_from_exact_threshold_and_boundary &&
      cut.preflight_budget_sufficient &&
      cut.root_replay_started_after_successful_preflight &&
      cut.complete_batches_replayed_from_frozen_snapshots &&
      cut.coverage_deltas_applied_exactly &&
      cut.persistent_root_ids_preserved &&
      cut.active_roots_canonical_and_disjoint_by_facet &&
      cut.prefix_forest_accounting_certified &&
      cut.cursor_matches_replayed_prefix &&
      !cut.terminal_order_complete_empty && !cut.empty_prefix_complete;
  if (!cut.journal_relative_cut_replay_certified) {
    return source_rejected_cut(history, squared_level, boundary, budget);
  }
  cut.decision = boundary == ExactReducedGammaCutBoundary::strict_open
                     ? ExactReducedGammaCutDecision::
                           complete_strict_journal_relative_reduced_gamma_cut
                     : ExactReducedGammaCutDecision::
                           complete_closed_journal_relative_reduced_gamma_cut;
  return cut;
}

[[nodiscard]] bool result_facts_match(
    const ExactReducedGammaCut& observed,
    const ExactReducedGammaCut& expected) {
  return observed.source_history_claims_and_structure_accepted ==
             expected.source_history_claims_and_structure_accepted &&
         observed.source_history_certification_is_external_assumption ==
             expected.source_history_certification_is_external_assumption &&
         observed.source_history_geometry_not_freshly_certified ==
             expected.source_history_geometry_not_freshly_certified &&
         observed.coherent_forged_history_cannot_be_excluded_without_cloud ==
             expected.coherent_forged_history_cannot_be_excluded_without_cloud &&
         observed.
                 global_source_structure_audit_completed_before_prefix_selection ==
             expected.
                 global_source_structure_audit_completed_before_prefix_selection &&
         observed.prefix_selected_from_exact_threshold_and_boundary ==
             expected.prefix_selected_from_exact_threshold_and_boundary &&
         observed.preflight_budget_sufficient ==
             expected.preflight_budget_sufficient &&
         observed.root_replay_started_after_successful_preflight ==
             expected.root_replay_started_after_successful_preflight &&
         observed.complete_batches_replayed_from_frozen_snapshots ==
             expected.complete_batches_replayed_from_frozen_snapshots &&
         observed.coverage_deltas_applied_exactly ==
             expected.coverage_deltas_applied_exactly &&
         observed.persistent_root_ids_preserved ==
             expected.persistent_root_ids_preserved &&
         observed.active_roots_canonical_and_disjoint_by_facet ==
             expected.active_roots_canonical_and_disjoint_by_facet &&
         observed.prefix_forest_accounting_certified ==
             expected.prefix_forest_accounting_certified &&
         observed.cursor_matches_replayed_prefix ==
             expected.cursor_matches_replayed_prefix &&
         observed.terminal_order_complete_empty ==
             expected.terminal_order_complete_empty &&
         observed.empty_prefix_complete == expected.empty_prefix_complete &&
         observed.journal_relative_cut_replay_certified ==
             expected.journal_relative_cut_replay_certified;
}

}  // namespace

ExactReducedGammaCutVerification verify_exact_reduced_gamma_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget,
    const ExactReducedGammaCut& cut) {
  // Only trusted external inputs drive this reconstruction. In particular,
  // no observed decision, cursor, level or root chooses the expected prefix.
  const ExactReducedGammaCut expected = compute_exact_reduced_gamma_cut(
      history, squared_level, boundary, budget);
  ExactReducedGammaCutVerification verification;
  verification.requested_budget_certified =
      cut.requested_budget == budget &&
      cut.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      cut.point_count == history.point_count &&
      cut.point_count == expected.point_count && cut.order == history.order &&
      cut.order == expected.order && cut.squared_level == squared_level &&
      cut.squared_level == expected.squared_level && cut.boundary == boundary &&
      cut.boundary == expected.boundary;
  verification.source_history_gate_outcome_certified =
      cut.source_history_claims_and_structure_accepted ==
      expected.source_history_claims_and_structure_accepted;
  verification.derived_preflight_sizes_certified =
      cut.required_batch_capacity == expected.required_batch_capacity &&
      cut.required_group_record_capacity ==
          expected.required_group_record_capacity &&
      cut.required_node_record_capacity ==
          expected.required_node_record_capacity &&
      cut.required_prior_root_reference_capacity ==
          expected.required_prior_root_reference_capacity &&
      cut.required_child_reference_capacity ==
          expected.required_child_reference_capacity &&
      cut.required_newly_active_facet_capacity ==
          expected.required_newly_active_facet_capacity &&
      cut.required_equal_level_coface_capacity ==
          expected.required_equal_level_coface_capacity &&
      cut.required_delta_facet_capacity ==
          expected.required_delta_facet_capacity &&
      cut.required_delta_point_reference_capacity ==
          expected.required_delta_point_reference_capacity &&
      cut.required_active_root_capacity ==
          expected.required_active_root_capacity &&
      cut.required_output_facet_reference_capacity ==
          expected.required_output_facet_reference_capacity &&
      cut.required_output_point_reference_capacity ==
          expected.required_output_point_reference_capacity &&
      cut.required_facet_replay_work_capacity ==
          expected.required_facet_replay_work_capacity &&
      cut.required_point_id_replay_work_capacity ==
          expected.required_point_id_replay_work_capacity &&
      cut.required_result_incidence_facet_check_capacity ==
          expected.required_result_incidence_facet_check_capacity &&
      cut.required_result_incidence_point_id_work_capacity ==
          expected.required_result_incidence_point_id_work_capacity;
  verification.cursor_certified = cut.cursor == expected.cursor;
  verification.active_roots_certified =
      cut.active_roots == expected.active_roots;
  verification.result_facts_certified = result_facts_match(cut, expected);
  verification.counters_certified = cut.counters == expected.counters;
  verification.decision_certified = cut.decision == expected.decision;
  verification.scope_certified =
      cut.scope == ExactReducedGammaCutScope::
                       bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only &&
      cut.scope == expected.scope;
  verification.fresh_journal_replay_certified = cut == expected;
  verification.
      exact_journal_relative_reduced_gamma_cut_replay_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.source_history_gate_outcome_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.cursor_certified && verification.active_roots_certified &&
      verification.result_facts_certified &&
      verification.counters_certified && verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_journal_replay_certified;
  return verification;
}

ExactReducedGammaCut build_exact_reduced_gamma_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget) {
  ExactReducedGammaCut cut = compute_exact_reduced_gamma_cut(
      history, squared_level, boundary, budget);
  const ExactReducedGammaCutVerification verification =
      verify_exact_reduced_gamma_cut(
          history, squared_level, boundary, budget, cut);
  if (!verification.
          exact_journal_relative_reduced_gamma_cut_replay_decision_certified) {
    throw std::logic_error(
        "the exact journal-relative reduced-Gamma cut failed fresh replay");
  }
  return cut;
}

}  // namespace morsehgp3d::hierarchy
