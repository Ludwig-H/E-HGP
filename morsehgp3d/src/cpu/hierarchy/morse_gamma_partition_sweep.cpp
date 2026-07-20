#include "morsehgp3d/hierarchy/morse_gamma_partition_sweep.hpp"

#include "morsehgp3d/hierarchy/gamma_transition.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using SweepBudget = ExactMorseGammaPartitionSweepBudget;
using SweepCounters = ExactMorseGammaPartitionSweepCounters;
using SweepDecision = ExactMorseGammaPartitionSweepDecision;
using SweepResult = ExactMorseGammaPartitionSweepResult;
using SweepScope = ExactMorseGammaPartitionSweepScope;
using BirthRecord = ExactMorseGammaBirthRecord;
using SaddleRecord = ExactMorseGammaSaddleRecord;
using Node = ExactMorseGammaNode;
using Group = ExactMorseGammaContractionGroup;
using Batch = ExactMorseGammaBatchRecord;
using Checkpoint = ExactMorseGammaOracleCheckpoint;
using MismatchWitness = ExactMorseGammaPartitionMismatchWitness;
using MismatchKind = ExactMorseGammaPartitionMismatchKind;
using MismatchStage = ExactMorseGammaPartitionMismatchStage;
using GammaComponent = ExactStrictGammaComponentWitness;

static_assert(
    SweepBudget::maximum_supported_arm_reference_count ==
    4U * SweepBudget::maximum_supported_saddle_record_count);
static_assert(
    SweepBudget::maximum_supported_node_count ==
    2U * SweepBudget::maximum_supported_birth_record_count - 1U);
static_assert(
    SweepBudget::maximum_supported_child_reference_count + 1U ==
    SweepBudget::maximum_supported_node_count);
static_assert(
    SweepBudget::maximum_supported_batch_reference_count ==
    3U * SweepBudget::maximum_supported_batch_record_count);

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
        "the Morse--Gamma sweep binomial coefficient overflows");
    value /= factor;
  }
  return value;
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const SweepBudget& budget) {
  if (cloud.size() < SweepResult::minimum_supported_point_count ||
      cloud.size() > SweepResult::maximum_supported_point_count ||
      order < SweepResult::minimum_supported_order ||
      order > SweepResult::maximum_supported_order || order >= cloud.size()) {
    throw std::invalid_argument(
        "the Morse--Gamma partition sweep requires 3<=n<=14, 2<=k<n "
        "and k<=10");
  }
  validate_exact_morse_gamma_partition_sweep_budget_caps(budget);
}

void derive_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    SweepResult& result) {
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the Morse--Gamma critical-event bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the Morse--Gamma critical-arm bound overflows");
  result.exhaustive_facet_count = bounded_binomial(cloud.size(), order);
  result.exhaustive_coface_count =
      bounded_binomial(cloud.size(), order + 1U);

  result.required_birth_record_capacity =
      result.critical_event_support_bound;
  result.required_saddle_record_capacity =
      result.critical_event_support_bound;
  result.required_arm_reference_capacity = result.critical_arm_bound;
  result.required_node_capacity =
      checked_multiply(
          2U,
          result.critical_event_support_bound,
          "the Morse--Gamma node bound overflows") -
      1U;
  result.required_child_reference_capacity =
      result.required_node_capacity - 1U;
  result.required_batch_record_capacity =
      result.critical_event_support_bound;
  result.required_contraction_group_capacity =
      result.critical_event_support_bound;
  result.required_group_root_reference_capacity = result.critical_arm_bound;
  result.required_batch_reference_capacity = checked_multiply(
      3U,
      result.critical_event_support_bound,
      "the Morse--Gamma batch-reference bound overflows");
  result.required_checkpoint_capacity = checked_add(
      result.exhaustive_facet_count,
      result.exhaustive_coface_count,
      "the Morse--Gamma checkpoint bound overflows");

  if (result.required_birth_record_capacity >
          SweepBudget::maximum_supported_birth_record_count ||
      result.required_saddle_record_capacity >
          SweepBudget::maximum_supported_saddle_record_count ||
      result.required_arm_reference_capacity >
          SweepBudget::maximum_supported_arm_reference_count ||
      result.required_node_capacity >
          SweepBudget::maximum_supported_node_count ||
      result.required_child_reference_capacity >
          SweepBudget::maximum_supported_child_reference_count ||
      result.required_batch_record_capacity >
          SweepBudget::maximum_supported_batch_record_count ||
      result.required_contraction_group_capacity >
          SweepBudget::maximum_supported_contraction_group_count ||
      result.required_group_root_reference_capacity >
          SweepBudget::maximum_supported_group_root_reference_count ||
      result.required_batch_reference_capacity >
          SweepBudget::maximum_supported_batch_reference_count ||
      result.required_checkpoint_capacity >
          SweepBudget::maximum_supported_checkpoint_count) {
    throw std::logic_error(
        "the derived Morse--Gamma preflight exceeds its bounded caps");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const SweepBudget& budget,
    const SweepResult& result) {
  return budget.maximum_birth_record_count >=
             result.required_birth_record_capacity &&
         budget.maximum_saddle_record_count >=
             result.required_saddle_record_capacity &&
         budget.maximum_arm_reference_count >=
             result.required_arm_reference_capacity &&
         budget.maximum_node_count >= result.required_node_capacity &&
         budget.maximum_child_reference_count >=
             result.required_child_reference_capacity &&
         budget.maximum_batch_record_count >=
             result.required_batch_record_capacity &&
         budget.maximum_contraction_group_count >=
             result.required_contraction_group_capacity &&
         budget.maximum_group_root_reference_count >=
             result.required_group_root_reference_capacity &&
         budget.maximum_batch_reference_count >=
             result.required_batch_reference_capacity &&
         budget.maximum_checkpoint_count >=
             result.required_checkpoint_capacity;
}

[[nodiscard]] std::size_t unique_value_count(
    const std::vector<std::size_t>& values) {
  std::vector<std::size_t> canonical = values;
  std::sort(canonical.begin(), canonical.end());
  canonical.erase(
      std::unique(canonical.begin(), canonical.end()), canonical.end());
  return canonical.size();
}

[[nodiscard]] bool contains_value(
    const std::vector<std::size_t>& canonical_values,
    std::size_t value) {
  return std::binary_search(
      canonical_values.begin(), canonical_values.end(), value);
}

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t size) : parent_(size), rank_(size, 0U) {
    for (std::size_t index = 0U; index < size; ++index) {
      parent_[index] = index;
    }
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (parent_[value] != value) {
      parent_[value] = find(parent_[value]);
    }
    return parent_[value];
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
  std::vector<std::size_t> rank_;
};

struct GroupShape {
  std::vector<std::size_t> saddle_record_indices;
  std::vector<std::size_t> prior_root_node_ids;

  friend bool operator==(const GroupShape&, const GroupShape&) = default;
};

[[nodiscard]] std::vector<GroupShape> resolve_hypergraph_groups(
    const std::vector<SaddleRecord>& saddles,
    bool reverse_saddle_order) {
  std::vector<std::size_t> root_labels;
  for (const SaddleRecord& saddle : saddles) {
    if (saddle.pre_batch_root_node_ids.empty()) {
      throw std::logic_error(
          "a Morse saddle does not expose a nonempty frozen root edge");
    }
    root_labels.insert(
        root_labels.end(),
        saddle.pre_batch_root_node_ids.begin(),
        saddle.pre_batch_root_node_ids.end());
  }
  std::sort(root_labels.begin(), root_labels.end());
  root_labels.erase(
      std::unique(root_labels.begin(), root_labels.end()),
      root_labels.end());
  DisjointSet root_components(root_labels.size());

  std::vector<std::size_t> traversal(saddles.size());
  for (std::size_t index = 0U; index < saddles.size(); ++index) {
    traversal[index] = index;
  }
  if (reverse_saddle_order) {
    std::reverse(traversal.begin(), traversal.end());
  }
  for (const std::size_t saddle_index : traversal) {
    const std::vector<std::size_t>& roots =
        saddles[saddle_index].pre_batch_root_node_ids;
    const std::size_t first = static_cast<std::size_t>(
        std::lower_bound(root_labels.begin(), root_labels.end(), roots.front()) -
        root_labels.begin());
    for (std::size_t root_index = 1U;
         root_index < roots.size();
         ++root_index) {
      const std::size_t next = static_cast<std::size_t>(
          std::lower_bound(
              root_labels.begin(), root_labels.end(), roots[root_index]) -
          root_labels.begin());
      root_components.unite(first, next);
    }
  }

  std::vector<GroupShape> groups;
  std::vector<std::size_t> group_representatives;
  for (const std::size_t saddle_index : traversal) {
    const SaddleRecord& saddle = saddles[saddle_index];
    const std::size_t first = static_cast<std::size_t>(
        std::lower_bound(
            root_labels.begin(),
            root_labels.end(),
            saddle.pre_batch_root_node_ids.front()) -
        root_labels.begin());
    const std::size_t representative = root_components.find(first);
    const auto group_position = std::find(
        group_representatives.begin(),
        group_representatives.end(),
        representative);
    std::size_t group_index = 0U;
    if (group_position == group_representatives.end()) {
      group_index = groups.size();
      group_representatives.push_back(representative);
      groups.push_back(GroupShape{});
    } else {
      group_index = static_cast<std::size_t>(
          group_position - group_representatives.begin());
    }
    GroupShape& group = groups[group_index];
    group.saddle_record_indices.push_back(saddle.saddle_record_index);
    group.prior_root_node_ids.insert(
        group.prior_root_node_ids.end(),
        saddle.pre_batch_root_node_ids.begin(),
        saddle.pre_batch_root_node_ids.end());
  }
  for (GroupShape& group : groups) {
    std::sort(
        group.saddle_record_indices.begin(),
        group.saddle_record_indices.end());
    std::sort(
        group.prior_root_node_ids.begin(),
        group.prior_root_node_ids.end());
    group.prior_root_node_ids.erase(
        std::unique(
            group.prior_root_node_ids.begin(),
            group.prior_root_node_ids.end()),
        group.prior_root_node_ids.end());
  }
  std::sort(
      groups.begin(),
      groups.end(),
      [](const GroupShape& left, const GroupShape& right) {
        if (left.prior_root_node_ids != right.prior_root_node_ids) {
          return left.prior_root_node_ids < right.prior_root_node_ids;
        }
        return left.saddle_record_indices < right.saddle_record_indices;
      });
  return groups;
}

[[nodiscard]] std::optional<std::size_t> unique_strict_birth_for_facet(
    const std::vector<BirthRecord>& births,
    const std::vector<spatial::PointId>& facet_point_ids,
    const exact::ExactLevel& saddle_level) {
  std::optional<std::size_t> match;
  for (const BirthRecord& birth : births) {
    if (birth.facet_point_ids == facet_point_ids &&
        birth.squared_level < saddle_level) {
      if (match.has_value()) {
        return std::nullopt;
      }
      match = birth.birth_record_index;
    }
  }
  return match;
}

[[nodiscard]] std::size_t count_strict_births_for_facet(
    const std::vector<BirthRecord>& births,
    const std::vector<spatial::PointId>& facet_point_ids,
    const exact::ExactLevel& saddle_level) {
  return static_cast<std::size_t>(std::count_if(
      births.begin(),
      births.end(),
      [&](const BirthRecord& birth) {
        return birth.facet_point_ids == facet_point_ids &&
               birth.squared_level < saddle_level;
      }));
}

[[nodiscard]] std::vector<std::size_t> canonical_roots(
    const std::vector<std::size_t>& root_by_birth) {
  std::vector<std::size_t> roots = root_by_birth;
  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
  return roots;
}

void apply_batch_to_birth_roots(
    const Batch& batch,
    const std::vector<BirthRecord>& births,
    const std::vector<Group>& groups,
    std::vector<std::optional<std::size_t>>& roots) {
  const std::vector<std::optional<std::size_t>> frozen = roots;
  for (const std::size_t birth_index : batch.birth_record_indices) {
    if (birth_index >= births.size() || roots[birth_index].has_value()) {
      throw std::logic_error(
          "a Morse replay batch has an invalid birth activation");
    }
    roots[birth_index] = births[birth_index].node_id;
  }
  for (const std::size_t group_index : batch.contraction_group_indices) {
    if (group_index >= groups.size()) {
      throw std::logic_error(
          "a Morse replay batch has an invalid contraction group");
    }
    const Group& group = groups[group_index];
    for (std::size_t birth_index = 0U;
         birth_index < frozen.size();
         ++birth_index) {
      if (frozen[birth_index].has_value() &&
          contains_value(
              group.prior_root_node_ids, *frozen[birth_index])) {
        roots[birth_index] = group.resulting_root_node_id;
      }
    }
  }
}

[[nodiscard]] bool component_contains_facet(
    const GammaComponent& component,
    const std::vector<spatial::PointId>& facet_point_ids) {
  return std::binary_search(
      component.facet_point_ids.begin(),
      component.facet_point_ids.end(),
      facet_point_ids);
}

struct PartitionAudit {
  std::size_t morse_root_count{};
  std::size_t gamma_component_count{};
  bool bijective{false};
  std::optional<MismatchWitness> mismatch;
};

[[nodiscard]] PartitionAudit audit_birth_partition(
    const std::vector<BirthRecord>& births,
    const std::vector<std::optional<std::size_t>>& root_by_birth,
    const std::vector<GammaComponent>& components,
    const exact::ExactLevel& level,
    MismatchStage stage,
    SweepCounters& counters) {
  PartitionAudit audit;
  audit.gamma_component_count = components.size();
  std::vector<std::size_t> active_roots;
  std::vector<std::optional<std::size_t>> component_root(
      components.size());
  std::vector<std::size_t> component_birth_count(components.size(), 0U);
  std::vector<std::size_t> seen_roots;
  std::vector<std::size_t> seen_root_components;

  for (const BirthRecord& birth : births) {
    const bool active = stage == MismatchStage::strict_open
                            ? birth.squared_level < level
                            : birth.squared_level <= level;
    if (!active) {
      continue;
    }
    if (!root_by_birth[birth.birth_record_index].has_value()) {
      throw std::logic_error(
          "an active Morse birth has no replay root");
    }
    if (stage == MismatchStage::strict_open) {
      ++counters.strict_birth_component_lookup_count;
    } else {
      ++counters.closed_birth_component_lookup_count;
    }
    const std::size_t root = *root_by_birth[birth.birth_record_index];
    active_roots.push_back(root);

    std::optional<std::size_t> component_index;
    for (std::size_t index = 0U; index < components.size(); ++index) {
      if (component_contains_facet(components[index], birth.facet_point_ids)) {
        if (component_index.has_value()) {
          throw std::logic_error(
              "one Gamma facet occurs in multiple exhaustive components");
        }
        component_index = index;
      }
    }
    if (!component_index.has_value()) {
      MismatchWitness witness{
          level,
          stage,
          MismatchKind::active_birth_facet_absent_from_gamma,
          birth.birth_record_index,
          root,
          std::nullopt,
          {},
          {}};
      witness.birth_facet_point_ids = birth.facet_point_ids;
      audit.mismatch = std::move(witness);
      return audit;
    }
    ++component_birth_count[*component_index];

    const auto seen_root =
        std::find(seen_roots.begin(), seen_roots.end(), root);
    if (seen_root == seen_roots.end()) {
      seen_roots.push_back(root);
      seen_root_components.push_back(*component_index);
    } else {
      const std::size_t position =
          static_cast<std::size_t>(seen_root - seen_roots.begin());
      if (seen_root_components[position] != *component_index) {
        MismatchWitness witness{
            level,
            stage,
            MismatchKind::one_morse_root_spans_multiple_gamma_components,
            birth.birth_record_index,
            root,
            *component_index,
            {},
            {}};
        witness.birth_facet_point_ids = birth.facet_point_ids;
        witness.gamma_component_representative_facet_point_ids =
            components[*component_index].
                canonical_representative_facet_point_ids;
        audit.mismatch = std::move(witness);
        return audit;
      }
    }

    if (!component_root[*component_index].has_value()) {
      component_root[*component_index] = root;
    } else if (*component_root[*component_index] != root) {
      MismatchWitness witness{
          level,
          stage,
          MismatchKind::one_gamma_component_spans_multiple_morse_roots,
          birth.birth_record_index,
          root,
          *component_index,
          {},
          {}};
      witness.birth_facet_point_ids = birth.facet_point_ids;
      witness.gamma_component_representative_facet_point_ids =
          components[*component_index].
              canonical_representative_facet_point_ids;
      audit.mismatch = std::move(witness);
      return audit;
    }
  }

  audit.morse_root_count = unique_value_count(active_roots);
  for (std::size_t component_index = 0U;
       component_index < component_birth_count.size();
       ++component_index) {
    if (component_birth_count[component_index] == 0U) {
      MismatchWitness witness{
          level,
          stage,
          MismatchKind::gamma_component_without_catalog_birth,
          std::nullopt,
          std::nullopt,
          component_index,
          {},
          {}};
      witness.gamma_component_representative_facet_point_ids =
          components[component_index].canonical_representative_facet_point_ids;
      audit.mismatch = std::move(witness);
      return audit;
    }
  }
  audit.bijective = audit.morse_root_count == audit.gamma_component_count &&
                    seen_roots.size() == audit.gamma_component_count;
  if (!audit.bijective) {
    throw std::logic_error(
        "the total Morse--Gamma partition maps are not bijective");
  }
  return audit;
}

[[nodiscard]] bool diagnostic_payload_is_empty(const SweepResult& result) {
  return result.birth_records.empty() && result.saddle_records.empty() &&
         result.nodes.empty() && result.contraction_groups.empty() &&
         result.batch_records.empty() && result.oracle_checkpoints.empty() &&
         result.final_root_node_ids.empty();
}

[[nodiscard]] bool result_facts_match(
    const SweepResult& observed,
    const SweepResult& expected) {
  return observed.conservative_preflight_bounds_certified ==
             expected.conservative_preflight_bounds_certified &&
         observed.preflight_budget_sufficient ==
             expected.preflight_budget_sufficient &&
         observed.critical_catalog_fresh_and_generic ==
             expected.critical_catalog_fresh_and_generic &&
         observed.every_rank_k_birth_has_one_canonical_record ==
             expected.every_rank_k_birth_has_one_canonical_record &&
         observed.every_rank_k_plus_one_saddle_family_is_complete ==
             expected.every_rank_k_plus_one_saddle_family_is_complete &&
         observed.every_arm_terminal_maps_to_one_strictly_earlier_birth ==
             expected.every_arm_terminal_maps_to_one_strictly_earlier_birth &&
         observed.all_saddle_targets_resolved_from_frozen_pre_batch_roots ==
             expected.all_saddle_targets_resolved_from_frozen_pre_batch_roots &&
         observed.equal_level_saddles_contracted_as_one_hypergraph ==
             expected.equal_level_saddles_contracted_as_one_hypergraph &&
         observed.contractions_invariant_under_saddle_permutation ==
             expected.contractions_invariant_under_saddle_permutation &&
         observed.local_genealogy_is_canonical_and_acyclic ==
             expected.local_genealogy_is_canonical_and_acyclic &&
         observed.gamma_oracle_started_only_after_complete_morse_genealogy ==
             expected.gamma_oracle_started_only_after_complete_morse_genealogy &&
         observed.gamma_activation_catalog_fresh_and_complete ==
             expected.gamma_activation_catalog_fresh_and_complete &&
         observed.every_morse_batch_level_is_a_gamma_activation_level ==
             expected.every_morse_batch_level_is_a_gamma_activation_level &&
         observed.strict_partitions_biject_gamma_at_every_activation_level ==
             expected.strict_partitions_biject_gamma_at_every_activation_level &&
         observed.closed_partitions_biject_gamma_at_every_activation_level ==
             expected.closed_partitions_biject_gamma_at_every_activation_level &&
         observed.gamma_objects_never_select_morse_births_targets_or_unions ==
             expected.gamma_objects_never_select_morse_births_targets_or_unions &&
         observed.records_are_internal_falsifier_objects_not_public_forest_or_attachments ==
             expected.records_are_internal_falsifier_objects_not_public_forest_or_attachments &&
         observed.diagnostic_outcomes_have_no_genealogy_payload ==
             expected.diagnostic_outcomes_have_no_genealogy_payload &&
         observed.morse_gamma_partition_sweep_certified ==
             expected.morse_gamma_partition_sweep_certified;
}

[[nodiscard]] SweepResult compute_sweep(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    SweepBudget budget) {
  validate_domain(cloud, order, budget);

  SweepResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = SweepScope::
      bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only;
  result.records_are_internal_falsifier_objects_not_public_forest_or_attachments =
      true;
  result.diagnostic_outcomes_have_no_genealogy_payload = true;
  result.counters.preflight_count = 1U;
  derive_preflight(cloud, order, result);
  result.conservative_preflight_bounds_certified = true;
  result.preflight_budget_sufficient = budget_covers_preflight(budget, result);
  if (!result.preflight_budget_sufficient) {
    result.decision =
        SweepDecision::no_sweep_preflight_budget_insufficient;
    return result;
  }

  const ExactCriticalCatalogResult catalog = build_exact_critical_catalog(
      cloud, order, budget.critical_catalog_budget);
  ++result.counters.critical_catalog_build_count;
  result.critical_catalog_decision = catalog.decision;
  if (catalog.decision !=
          ExactCriticalCatalogDecision::complete_supported_critical_catalog ||
      !catalog.no_relevant_extra_shell_degeneracy) {
    result.decision = SweepDecision::no_sweep_critical_catalog_rejected;
    return result;
  }
  result.critical_catalog_fresh_and_generic = true;

  std::vector<BirthRecord> births;
  std::vector<SaddleRecord> saddles;
  std::vector<Node> nodes;
  std::vector<Group> groups;
  std::vector<Batch> batches;
  std::vector<std::size_t> root_by_birth;

  for (std::size_t catalog_batch_index = 0U;
       catalog_batch_index < catalog.h0_batches.size();
       ++catalog_batch_index) {
    const ExactCriticalH0Batch& source_batch =
        catalog.h0_batches[catalog_batch_index];
    if (source_batch.order != order) {
      continue;
    }

    Batch batch;
    batch.batch_index = batches.size();
    batch.catalog_h0_batch_index = catalog_batch_index;
    batch.squared_level = source_batch.squared_level;
    batch.strict_root_count = canonical_roots(root_by_birth).size();

    std::vector<BirthRecord> pending_births;
    std::vector<Node> pending_birth_nodes;
    pending_births.reserve(source_batch.birth_event_indices.size());
    pending_birth_nodes.reserve(source_batch.birth_event_indices.size());
    for (const std::size_t event_index : source_batch.birth_event_indices) {
      if (event_index >= catalog.events.size()) {
        throw std::logic_error(
            "a Morse birth batch references no fresh critical event");
      }
      const ExactCriticalEvent& event = catalog.events[event_index];
      if (event.event_index != event_index ||
          !event.birth_order.has_value() || *event.birth_order != order ||
          event.closed_rank != order ||
          event.closed_point_ids.size() != order ||
          event.squared_level != source_batch.squared_level) {
        throw std::logic_error(
            "a Morse birth disagrees with its fresh rank-k event");
      }
      const auto duplicate = std::find_if(
          births.begin(),
          births.end(),
          [&](const BirthRecord& previous) {
            return previous.facet_point_ids == event.closed_point_ids;
          });
      if (duplicate != births.end() ||
          std::any_of(
              pending_births.begin(),
              pending_births.end(),
              [&](const BirthRecord& previous) {
                return previous.facet_point_ids == event.closed_point_ids;
              })) {
        throw std::logic_error(
            "a rank-k facet has multiple Morse birth events");
      }
      BirthRecord birth;
      birth.birth_record_index = births.size() + pending_births.size();
      birth.catalog_event_index = event_index;
      birth.node_id = nodes.size() + pending_birth_nodes.size();
      birth.squared_level = event.squared_level;
      birth.facet_point_ids = event.closed_point_ids;
      pending_birth_nodes.push_back(Node{
          birth.node_id,
          birth.squared_level,
          ExactMorseGammaNodeKind::birth,
          {},
          {event_index}});
      batch.birth_record_indices.push_back(birth.birth_record_index);
      pending_births.push_back(std::move(birth));
    }

    std::vector<SaddleRecord> pending_saddles;
    pending_saddles.reserve(source_batch.saddle_event_indices.size());
    for (const std::size_t event_index : source_batch.saddle_event_indices) {
      if (event_index >= catalog.events.size()) {
        throw std::logic_error(
            "a Morse saddle batch references no fresh critical event");
      }
      const ExactCriticalEvent& event = catalog.events[event_index];
      if (event.event_index != event_index ||
          !event.saddle_order.has_value() || *event.saddle_order != order ||
          event.closed_rank != order + 1U ||
          event.closed_point_ids.size() != order + 1U ||
          event.squared_level != source_batch.squared_level) {
        throw std::logic_error(
            "a Morse saddle disagrees with its fresh rank-(k+1) event");
      }

      const ExactCriticalArmFamilyResult family =
          build_exact_critical_arm_family_descent(
              cloud,
              event.shell_point_ids,
              budget.per_arm_chain_budget);
      ++result.counters.critical_arm_family_build_count;
      if (family.decision != ExactCriticalArmFamilyDecision::
              all_arms_complete_at_regular_active_facets ||
          !family.complete_terminal_label_partition_certified ||
          family.critical_shell_point_ids != event.shell_point_ids ||
          family.arms.size() != event.shell_point_ids.size()) {
        result.incomplete_saddle_catalog_event_index = event_index;
        result.decision =
            SweepDecision::no_sweep_critical_arm_family_incomplete;
        if (!diagnostic_payload_is_empty(result)) {
          throw std::logic_error(
              "an incomplete arm diagnostic leaked genealogy payload");
        }
        return result;
      }

      SaddleRecord saddle;
      saddle.saddle_record_index = saddles.size() + pending_saddles.size();
      saddle.catalog_event_index = event_index;
      saddle.batch_index = batch.batch_index;
      saddle.squared_level = event.squared_level;
      saddle.shell_point_ids = event.shell_point_ids;
      for (const ExactCriticalArmFamilyArmResult& arm : family.arms) {
        ++result.counters.arm_reference_count;
        ++result.counters.terminal_birth_lookup_count;
        if (!arm.active_terminal.has_value()) {
          result.decision =
              SweepDecision::no_sweep_critical_arm_family_incomplete;
          result.incomplete_saddle_catalog_event_index = event_index;
          return result;
        }
        const std::vector<spatial::PointId>& terminal_facet =
            arm.active_terminal->facet_point_ids;
        const std::optional<std::size_t> birth_index =
            unique_strict_birth_for_facet(
                births, terminal_facet, event.squared_level);
        if (!birth_index.has_value() ||
            count_strict_births_for_facet(
                births, terminal_facet, event.squared_level) != 1U ||
            *birth_index >= root_by_birth.size()) {
          result.decision =
              SweepDecision::no_sweep_terminal_birth_not_unique;
          return result;
        }
        const BirthRecord& terminal_birth = births[*birth_index];
        if (terminal_birth.catalog_event_index >= catalog.events.size() ||
            terminal_birth.squared_level !=
                arm.active_terminal->squared_level ||
            catalog.events[terminal_birth.catalog_event_index].center !=
                arm.active_terminal->center ||
            catalog.events[terminal_birth.catalog_event_index].squared_level !=
                arm.active_terminal->squared_level) {
          result.decision =
              SweepDecision::no_sweep_terminal_birth_not_unique;
          return result;
        }
        saddle.terminal_birth_record_indices.push_back(*birth_index);
        saddle.pre_batch_root_node_ids.push_back(
            root_by_birth[*birth_index]);
      }
      batch.saddle_record_indices.push_back(saddle.saddle_record_index);
      pending_saddles.push_back(std::move(saddle));
    }

    const std::vector<GroupShape> canonical_groups =
        resolve_hypergraph_groups(pending_saddles, false);
    const std::vector<GroupShape> reversed_groups =
        resolve_hypergraph_groups(pending_saddles, true);
    if (canonical_groups != reversed_groups) {
      throw std::logic_error(
          "the frozen Morse saddle hypergraph depends on traversal order");
    }
    result.counters.reversed_order_group_comparison_count = checked_add(
        result.counters.reversed_order_group_comparison_count,
        canonical_groups.size(),
        "the reversed-group comparison count overflows");

    std::vector<Group> pending_groups;
    std::vector<Node> pending_merge_nodes;
    pending_groups.reserve(canonical_groups.size());
    for (const GroupShape& shape : canonical_groups) {
      Group group;
      group.contraction_group_index = groups.size() + pending_groups.size();
      group.batch_index = batch.batch_index;
      group.saddle_record_indices = shape.saddle_record_indices;
      group.prior_root_node_ids = shape.prior_root_node_ids;
      if (group.prior_root_node_ids.size() > 1U) {
        const std::size_t node_id = nodes.size() +
                                    pending_birth_nodes.size() +
                                    pending_merge_nodes.size();
        group.created_node_id = node_id;
        group.resulting_root_node_id = node_id;
        std::vector<std::size_t> event_indices;
        event_indices.reserve(group.saddle_record_indices.size());
        for (const std::size_t saddle_index :
             group.saddle_record_indices) {
          if (saddle_index < saddles.size() ||
              saddle_index >= saddles.size() + pending_saddles.size()) {
            throw std::logic_error(
                "a pending contraction references another Morse batch");
          }
          const std::size_t local_index = saddle_index - saddles.size();
          event_indices.push_back(
              pending_saddles[local_index].catalog_event_index);
        }
        std::sort(event_indices.begin(), event_indices.end());
        pending_merge_nodes.push_back(Node{
            node_id,
            source_batch.squared_level,
            ExactMorseGammaNodeKind::multifusion,
            group.prior_root_node_ids,
            std::move(event_indices)});
        ++result.counters.multifusion_group_count;
        result.counters.child_reference_count = checked_add(
            result.counters.child_reference_count,
            group.prior_root_node_ids.size(),
            "the Morse child-reference count overflows");
      } else {
        group.resulting_root_node_id = group.prior_root_node_ids.front();
        ++result.counters.continuation_group_count;
      }
      for (const std::size_t saddle_index : group.saddle_record_indices) {
        const std::size_t local_index = saddle_index - saddles.size();
        if (local_index >= pending_saddles.size()) {
          throw std::logic_error(
              "a contraction group has an invalid local saddle index");
        }
        pending_saddles[local_index].contraction_group_index =
            group.contraction_group_index;
      }
      batch.contraction_group_indices.push_back(
          group.contraction_group_index);
      result.counters.group_root_reference_count = checked_add(
          result.counters.group_root_reference_count,
          group.prior_root_node_ids.size(),
          "the Morse group-root-reference count overflows");
      pending_groups.push_back(std::move(group));
    }

    // The entire frozen hypergraph has now been resolved.  Only this point
    // mutates the local genealogy and active-root state.
    births.insert(
        births.end(), pending_births.begin(), pending_births.end());
    nodes.insert(
        nodes.end(), pending_birth_nodes.begin(), pending_birth_nodes.end());
    for (const BirthRecord& birth : pending_births) {
      root_by_birth.push_back(birth.node_id);
    }
    const std::vector<std::size_t> frozen_roots = root_by_birth;
    for (const Group& group : pending_groups) {
      for (std::size_t birth_index = 0U;
           birth_index < root_by_birth.size();
           ++birth_index) {
        if (contains_value(
                group.prior_root_node_ids, frozen_roots[birth_index])) {
          root_by_birth[birth_index] = group.resulting_root_node_id;
        }
      }
    }
    saddles.insert(
        saddles.end(), pending_saddles.begin(), pending_saddles.end());
    groups.insert(groups.end(), pending_groups.begin(), pending_groups.end());
    nodes.insert(
        nodes.end(), pending_merge_nodes.begin(), pending_merge_nodes.end());

    batch.closed_root_count = canonical_roots(root_by_birth).size();
    batch.all_saddles_resolved_from_frozen_pre_batch_roots = true;
    batch.quotient_components_invariant_under_reversed_saddle_order = true;
    batch.mutations_committed_after_complete_group_resolution = true;
    result.counters.batch_reference_count = checked_add(
        result.counters.batch_reference_count,
        checked_add(
            batch.birth_record_indices.size(),
            checked_add(
                batch.saddle_record_indices.size(),
                batch.contraction_group_indices.size(),
                "the Morse batch-reference subtotal overflows"),
            "the Morse batch-reference subtotal overflows"),
        "the Morse batch-reference count overflows");
    batches.push_back(std::move(batch));
  }

  result.counters.birth_record_count = births.size();
  result.counters.saddle_record_count = saddles.size();
  result.counters.node_count = nodes.size();
  result.counters.contraction_group_count = groups.size();
  result.counters.batch_record_count = batches.size();
  if (births.size() > budget.maximum_birth_record_count ||
      saddles.size() > budget.maximum_saddle_record_count ||
      result.counters.arm_reference_count >
          budget.maximum_arm_reference_count ||
      nodes.size() > budget.maximum_node_count ||
      result.counters.child_reference_count >
          budget.maximum_child_reference_count ||
      batches.size() > budget.maximum_batch_record_count ||
      groups.size() > budget.maximum_contraction_group_count ||
      result.counters.group_root_reference_count >
          budget.maximum_group_root_reference_count ||
      result.counters.batch_reference_count >
          budget.maximum_batch_reference_count) {
    throw std::logic_error(
        "the completed Morse genealogy exceeds its successful preflight");
  }
  result.every_rank_k_birth_has_one_canonical_record = true;
  result.every_rank_k_plus_one_saddle_family_is_complete = true;
  result.every_arm_terminal_maps_to_one_strictly_earlier_birth = true;
  result.all_saddle_targets_resolved_from_frozen_pre_batch_roots =
      std::all_of(
          batches.begin(),
          batches.end(),
          [](const Batch& batch) {
            return batch.all_saddles_resolved_from_frozen_pre_batch_roots;
          });
  result.equal_level_saddles_contracted_as_one_hypergraph = true;
  result.contractions_invariant_under_saddle_permutation = std::all_of(
      batches.begin(),
      batches.end(),
      [](const Batch& batch) {
        return batch.
            quotient_components_invariant_under_reversed_saddle_order;
      });
  result.local_genealogy_is_canonical_and_acyclic =
      std::all_of(
          nodes.begin(),
          nodes.end(),
          [](const Node& node) {
            return std::all_of(
                node.child_node_ids.begin(),
                node.child_node_ids.end(),
                [&](std::size_t child) { return child < node.node_id; });
          });
  if (!result.all_saddle_targets_resolved_from_frozen_pre_batch_roots ||
      !result.contractions_invariant_under_saddle_permutation ||
      !result.local_genealogy_is_canonical_and_acyclic) {
    throw std::logic_error(
        "the completed local Morse genealogy is not canonical");
  }

  result.gamma_oracle_started_only_after_complete_morse_genealogy = true;
  const ExactPersistentReducedGammaOrderHistory gamma_history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget.gamma_oracle_history_budget);
  ++result.counters.gamma_history_build_count;
  result.gamma_history_decision = gamma_history.decision;
  if (gamma_history.decision != ExactPersistentReducedGammaOrderHistoryDecision::
          complete_persistent_reduced_gamma_history ||
      !gamma_history.persistent_reduced_gamma_history_certified ||
      !gamma_history.activation_levels_canonical_and_complete) {
    result.decision = SweepDecision::no_sweep_gamma_oracle_rejected;
    return result;
  }
  result.gamma_activation_catalog_fresh_and_complete = true;

  for (const Batch& batch : batches) {
    if (!std::binary_search(
            gamma_history.activation_levels.begin(),
            gamma_history.activation_levels.end(),
            batch.squared_level)) {
      result.mismatch_witness = MismatchWitness{
          batch.squared_level,
          MismatchStage::strict_open,
          MismatchKind::
              morse_batch_level_absent_from_gamma_activation_catalog,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          {},
          {}};
      result.decision =
          SweepDecision::no_sweep_morse_gamma_partition_mismatch;
      return result;
    }
  }
  result.every_morse_batch_level_is_a_gamma_activation_level = true;

  std::vector<Checkpoint> checkpoints;
  checkpoints.reserve(gamma_history.activation_levels.size());
  std::vector<std::optional<std::size_t>> audit_roots(births.size());
  std::size_t next_batch_index = 0U;
  for (std::size_t activation_index = 0U;
       activation_index < gamma_history.activation_levels.size();
       ++activation_index) {
    const exact::ExactLevel& level =
        gamma_history.activation_levels[activation_index];
    if (next_batch_index < batches.size() &&
        batches[next_batch_index].squared_level < level) {
      throw std::logic_error(
          "a Morse batch was skipped by the complete Gamma level catalog");
    }
    const ExactGammaTransitionResult transition =
        build_exact_gamma_equal_level_transition(
            cloud,
            order,
            level,
            budget.gamma_oracle_history_budget.gamma_budget);
    ++result.counters.gamma_transition_build_count;
    if (transition.decision != ExactGammaTransitionDecision::
            complete_exhaustive_open_to_closed_transition ||
        !transition.strict_open_cut_fresh_replay_certified ||
        !transition.equal_level_catalog_exhaustive_certified ||
        !transition.closed_cut_exhaustive_certified ||
        !transition.equal_level_batch_applied_simultaneously) {
      result.decision = SweepDecision::no_sweep_gamma_oracle_rejected;
      return result;
    }

    Checkpoint checkpoint;
    checkpoint.checkpoint_index = checkpoints.size();
    checkpoint.activation_level_index = activation_index;
    checkpoint.squared_level = level;
    const PartitionAudit strict_audit = audit_birth_partition(
        births,
        audit_roots,
        transition.strict_gamma.components,
        level,
        MismatchStage::strict_open,
        result.counters);
    checkpoint.strict_morse_root_count = strict_audit.morse_root_count;
    checkpoint.strict_gamma_component_count =
        strict_audit.gamma_component_count;
    checkpoint.strict_birth_projection_is_bijective =
        strict_audit.bijective;
    if (strict_audit.mismatch.has_value()) {
      result.mismatch_witness = strict_audit.mismatch;
      result.decision =
          SweepDecision::no_sweep_morse_gamma_partition_mismatch;
      return result;
    }
    ++result.counters.strict_partition_bijection_count;

    if (next_batch_index < batches.size() &&
        batches[next_batch_index].squared_level == level) {
      checkpoint.morse_batch_index = next_batch_index;
      apply_batch_to_birth_roots(
          batches[next_batch_index], births, groups, audit_roots);
      ++next_batch_index;
    }
    const PartitionAudit closed_audit = audit_birth_partition(
        births,
        audit_roots,
        transition.closed_components,
        level,
        MismatchStage::closed,
        result.counters);
    checkpoint.closed_morse_root_count = closed_audit.morse_root_count;
    checkpoint.closed_gamma_component_count =
        closed_audit.gamma_component_count;
    checkpoint.closed_birth_projection_is_bijective =
        closed_audit.bijective;
    if (closed_audit.mismatch.has_value()) {
      result.mismatch_witness = closed_audit.mismatch;
      result.decision =
          SweepDecision::no_sweep_morse_gamma_partition_mismatch;
      return result;
    }
    ++result.counters.closed_partition_bijection_count;
    checkpoints.push_back(std::move(checkpoint));
  }
  result.counters.checkpoint_count = checkpoints.size();
  if (next_batch_index != batches.size() ||
      checkpoints.size() > budget.maximum_checkpoint_count) {
    throw std::logic_error(
        "the posterior Gamma audit did not consume the complete genealogy");
  }

  std::vector<std::size_t> final_roots;
  final_roots.reserve(audit_roots.size());
  for (const std::optional<std::size_t>& root : audit_roots) {
    if (!root.has_value()) {
      throw std::logic_error(
          "the completed Morse sweep retains an inactive birth");
    }
    final_roots.push_back(*root);
  }
  std::sort(final_roots.begin(), final_roots.end());
  final_roots.erase(
      std::unique(final_roots.begin(), final_roots.end()), final_roots.end());
  if (final_roots != canonical_roots(root_by_birth)) {
    throw std::logic_error(
        "the posterior Morse replay disagrees with the constructed genealogy");
  }

  result.strict_partitions_biject_gamma_at_every_activation_level =
      result.counters.strict_partition_bijection_count ==
      gamma_history.activation_levels.size();
  result.closed_partitions_biject_gamma_at_every_activation_level =
      result.counters.closed_partition_bijection_count ==
      gamma_history.activation_levels.size();
  result.gamma_objects_never_select_morse_births_targets_or_unions = true;
  result.birth_records = std::move(births);
  result.saddle_records = std::move(saddles);
  result.nodes = std::move(nodes);
  result.contraction_groups = std::move(groups);
  result.batch_records = std::move(batches);
  result.oracle_checkpoints = std::move(checkpoints);
  result.final_root_node_ids = std::move(final_roots);
  result.morse_gamma_partition_sweep_certified =
      result.conservative_preflight_bounds_certified &&
      result.preflight_budget_sufficient &&
      result.critical_catalog_fresh_and_generic &&
      result.every_rank_k_birth_has_one_canonical_record &&
      result.every_rank_k_plus_one_saddle_family_is_complete &&
      result.every_arm_terminal_maps_to_one_strictly_earlier_birth &&
      result.all_saddle_targets_resolved_from_frozen_pre_batch_roots &&
      result.equal_level_saddles_contracted_as_one_hypergraph &&
      result.contractions_invariant_under_saddle_permutation &&
      result.local_genealogy_is_canonical_and_acyclic &&
      result.gamma_oracle_started_only_after_complete_morse_genealogy &&
      result.gamma_activation_catalog_fresh_and_complete &&
      result.every_morse_batch_level_is_a_gamma_activation_level &&
      result.strict_partitions_biject_gamma_at_every_activation_level &&
      result.closed_partitions_biject_gamma_at_every_activation_level &&
      result.gamma_objects_never_select_morse_births_targets_or_unions &&
      result.records_are_internal_falsifier_objects_not_public_forest_or_attachments &&
      result.diagnostic_outcomes_have_no_genealogy_payload;
  if (!result.morse_gamma_partition_sweep_certified) {
    throw std::logic_error(
        "the posterior Morse--Gamma sweep failed certification");
  }
  result.decision =
      SweepDecision::complete_morse_gamma_partition_sweep;
  return result;
}

}  // namespace

void validate_exact_morse_gamma_partition_sweep_budget_caps(
    const SweepBudget& budget) {
  const ExactCriticalCatalogBudget& catalog = budget.critical_catalog_budget;
  const ExactFacetDescentChainBudget& chain = budget.per_arm_chain_budget;
  const ExactPersistentReducedGammaOrderHistoryBudget& history =
      budget.gamma_oracle_history_budget;
  const ExactStrictGammaBudget& gamma = history.gamma_budget;
  if (catalog.maximum_candidate_count >
          ExactCriticalCatalogBudget::maximum_supported_candidate_count ||
      catalog.maximum_point_classification_count >
          ExactCriticalCatalogBudget::
              maximum_supported_point_classification_count ||
      chain.maximum_committed_strict_segment_count >
          ExactFacetDescentChainBudget::
              maximum_supported_committed_strict_segment_count ||
      gamma.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      gamma.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      gamma.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count ||
      history.maximum_activation_level_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_activation_level_count ||
      history.maximum_total_facet_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_facet_work_count ||
      history.maximum_total_coface_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_coface_work_count ||
      history.maximum_total_union_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_union_work_count ||
      history.maximum_node_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_node_count ||
      history.maximum_child_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_child_reference_count ||
      history.maximum_group_root_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_root_reference_count ||
      history.maximum_group_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_count ||
      history.maximum_group_newly_active_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_newly_active_facet_count ||
      history.maximum_group_equal_level_coface_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_equal_level_coface_count ||
      history.maximum_delta_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_facet_count ||
      history.maximum_delta_point_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_point_reference_count ||
      budget.maximum_birth_record_count >
          SweepBudget::maximum_supported_birth_record_count ||
      budget.maximum_saddle_record_count >
          SweepBudget::maximum_supported_saddle_record_count ||
      budget.maximum_arm_reference_count >
          SweepBudget::maximum_supported_arm_reference_count ||
      budget.maximum_node_count > SweepBudget::maximum_supported_node_count ||
      budget.maximum_child_reference_count >
          SweepBudget::maximum_supported_child_reference_count ||
      budget.maximum_batch_record_count >
          SweepBudget::maximum_supported_batch_record_count ||
      budget.maximum_contraction_group_count >
          SweepBudget::maximum_supported_contraction_group_count ||
      budget.maximum_group_root_reference_count >
          SweepBudget::maximum_supported_group_root_reference_count ||
      budget.maximum_batch_reference_count >
          SweepBudget::maximum_supported_batch_reference_count ||
      budget.maximum_checkpoint_count >
          SweepBudget::maximum_supported_checkpoint_count) {
    throw std::invalid_argument(
        "a Morse--Gamma partition-sweep capacity exceeds its bounded cap");
  }
}

ExactMorseGammaPartitionSweepVerification
verify_exact_morse_gamma_partition_sweep(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    SweepBudget budget,
    const SweepResult& result) {
  const SweepResult expected = compute_sweep(cloud, order, budget);
  ExactMorseGammaPartitionSweepVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count && result.order == order &&
      result.order == expected.order;
  verification.derived_preflight_sizes_certified =
      result.critical_event_support_bound ==
          expected.critical_event_support_bound &&
      result.critical_arm_bound == expected.critical_arm_bound &&
      result.exhaustive_facet_count == expected.exhaustive_facet_count &&
      result.exhaustive_coface_count == expected.exhaustive_coface_count &&
      result.required_birth_record_capacity ==
          expected.required_birth_record_capacity &&
      result.required_saddle_record_capacity ==
          expected.required_saddle_record_capacity &&
      result.required_arm_reference_capacity ==
          expected.required_arm_reference_capacity &&
      result.required_node_capacity == expected.required_node_capacity &&
      result.required_child_reference_capacity ==
          expected.required_child_reference_capacity &&
      result.required_batch_record_capacity ==
          expected.required_batch_record_capacity &&
      result.required_contraction_group_capacity ==
          expected.required_contraction_group_capacity &&
      result.required_group_root_reference_capacity ==
          expected.required_group_root_reference_capacity &&
      result.required_batch_reference_capacity ==
          expected.required_batch_reference_capacity &&
      result.required_checkpoint_capacity ==
          expected.required_checkpoint_capacity;
  verification.subordinate_decisions_certified =
      result.critical_catalog_decision == expected.critical_catalog_decision &&
      result.gamma_history_decision == expected.gamma_history_decision;
  verification.diagnostic_witness_certified =
      result.incomplete_saddle_catalog_event_index ==
          expected.incomplete_saddle_catalog_event_index &&
      result.mismatch_witness == expected.mismatch_witness &&
      (result.decision == SweepDecision::
               complete_morse_gamma_partition_sweep ||
       diagnostic_payload_is_empty(result));
  verification.birth_records_certified =
      result.birth_records == expected.birth_records;
  verification.saddle_records_certified =
      result.saddle_records == expected.saddle_records;
  verification.nodes_certified = result.nodes == expected.nodes;
  verification.contraction_groups_certified =
      result.contraction_groups == expected.contraction_groups;
  verification.batch_records_certified =
      result.batch_records == expected.batch_records;
  verification.oracle_checkpoints_certified =
      result.oracle_checkpoints == expected.oracle_checkpoints;
  verification.final_roots_certified =
      result.final_root_node_ids == expected.final_root_node_ids;
  verification.result_facts_certified = result_facts_match(result, expected);
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified = result.decision == expected.decision;
  verification.scope_certified =
      result.scope == SweepScope::
          bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.exact_morse_gamma_partition_sweep_decision_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.subordinate_decisions_certified &&
      verification.diagnostic_witness_certified &&
      verification.birth_records_certified &&
      verification.saddle_records_certified && verification.nodes_certified &&
      verification.contraction_groups_certified &&
      verification.batch_records_certified &&
      verification.oracle_checkpoints_certified &&
      verification.final_roots_certified &&
      verification.result_facts_certified && verification.counters_certified &&
      verification.decision_certified && verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

SweepResult build_exact_morse_gamma_partition_sweep(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    SweepBudget budget) {
  SweepResult result = compute_sweep(cloud, order, budget);
  const ExactMorseGammaPartitionSweepVerification verification =
      verify_exact_morse_gamma_partition_sweep(
          cloud, order, budget, result);
  if (!verification.
          exact_morse_gamma_partition_sweep_decision_certified) {
    throw std::logic_error(
        "the exact Morse--Gamma partition sweep failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
