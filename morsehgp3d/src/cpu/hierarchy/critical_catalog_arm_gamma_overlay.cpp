#include "morsehgp3d/hierarchy/critical_catalog_arm_gamma_overlay.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using OverlayBudget = ExactCriticalCatalogArmGammaOverlayBudget;
using OverlayDecision = ExactCriticalCatalogArmGammaOverlayDecision;
using OverlayResult = ExactCriticalCatalogArmGammaOverlayResult;
using SaddleFamilyRecord =
    ExactCriticalCatalogArmGammaSaddleFamilyRecord;
using BatchRecord = ExactCriticalCatalogArmGammaBatchRecord;
using TargetComponent = ExactCriticalCatalogArmGammaTargetComponent;
using ArmTarget = ExactCriticalCatalogArmGammaArmTarget;

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
        "the critical-catalog arm-Gamma overlay binomial coefficient "
        "overflows");
    value /= factor;
  }
  return value;
}

void validate_critical_catalog_budget_caps(
    const ExactCriticalCatalogBudget& budget) {
  if (budget.maximum_candidate_count >
          ExactCriticalCatalogBudget::maximum_supported_candidate_count ||
      budget.maximum_point_classification_count >
          ExactCriticalCatalogBudget::
              maximum_supported_point_classification_count) {
    throw std::invalid_argument(
        "the nested critical-catalog budget exceeds its bounded cap");
  }
}

void validate_chain_budget_cap(
    const ExactFacetDescentChainBudget& budget) {
  if (budget.maximum_committed_strict_segment_count >
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count) {
    throw std::invalid_argument(
        "the nested critical-arm chain budget exceeds its bounded cap");
  }
}

void validate_reduced_gamma_batch_budget_cap(
    const ExactStrictGammaBudget& budget) {
  if (budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count) {
    throw std::invalid_argument(
        "the nested reduced-Gamma batch budget exceeds its bounded cap");
  }
}

void validate_overlay_budget_caps(const OverlayBudget& budget) {
  validate_critical_catalog_budget_caps(budget.critical_catalog_budget);
  validate_chain_budget_cap(budget.per_arm_chain_budget);
  validate_reduced_gamma_batch_budget_cap(
      budget.reduced_gamma_batch_budget);
  if (budget.maximum_saddle_event_count >
          OverlayBudget::maximum_supported_saddle_event_count ||
      budget.maximum_arm_count >
          OverlayBudget::maximum_supported_arm_count ||
      budget.maximum_saddle_batch_count >
          OverlayBudget::maximum_supported_saddle_batch_count ||
      budget.maximum_target_component_count >
          OverlayBudget::maximum_supported_target_component_count ||
      budget.maximum_target_component_facet_reference_count >
          OverlayBudget::
              maximum_supported_target_component_facet_reference_count ||
      budget.maximum_target_component_point_id_reference_count >
          OverlayBudget::
              maximum_supported_target_component_point_id_reference_count ||
      budget.maximum_committed_chain_segment_count >
          OverlayBudget::
              maximum_supported_committed_chain_segment_count) {
    throw std::invalid_argument(
        "a critical-catalog arm-Gamma overlay capacity exceeds its "
        "bounded cap");
  }
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const OverlayBudget& budget) {
  if (cloud.size() < OverlayResult::minimum_supported_point_count ||
      cloud.size() > OverlayResult::maximum_supported_point_count ||
      order < OverlayResult::minimum_supported_order ||
      order > OverlayResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the critical-catalog arm-Gamma overlay requires "
        "2<=k<n<=14 and k<=10");
  }
  validate_overlay_budget_caps(budget);
}

[[nodiscard]] std::size_t saddle_event_support_bound(
    std::size_t point_count,
    std::size_t order) {
  const std::size_t maximum_support_size =
      std::min(std::size_t{4U}, order + 1U);
  std::size_t count = 0U;
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    count = checked_add(
        count,
        bounded_binomial(point_count, support_size),
        "the critical-catalog saddle-event support bound overflows");
  }
  return count;
}

void derive_overlay_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayResult& result) {
  result.exhaustive_facet_count =
      bounded_binomial(cloud.size(), order);
  result.required_saddle_event_capacity =
      saddle_event_support_bound(cloud.size(), order);
  result.required_arm_capacity = checked_multiply(
      4U,
      result.required_saddle_event_capacity,
      "the critical-catalog arm capacity overflows");
  result.required_saddle_batch_capacity =
      result.required_saddle_event_capacity;
  result.required_target_component_capacity =
      result.required_arm_capacity;
  result.required_target_component_facet_reference_capacity =
      checked_multiply(
          result.required_saddle_event_capacity,
          result.exhaustive_facet_count,
          "the target-component facet-reference capacity overflows");
  const std::size_t target_component_stored_facet_label_capacity =
      checked_add(
          result.required_target_component_facet_reference_capacity,
          result.required_target_component_capacity,
          "the target-component stored facet-label capacity overflows");
  result.required_target_component_point_id_reference_capacity =
      checked_multiply(
          order,
          target_component_stored_facet_label_capacity,
          "the target-component PointId-reference capacity overflows");
  result.required_committed_chain_segment_capacity = checked_multiply(
      result.required_arm_capacity,
      result.requested_budget.per_arm_chain_budget
          .maximum_committed_strict_segment_count,
      "the committed critical-arm chain-segment capacity overflows");

  if (result.required_saddle_event_capacity >
          OverlayBudget::maximum_supported_saddle_event_count ||
      result.required_arm_capacity >
          OverlayBudget::maximum_supported_arm_count ||
      result.required_saddle_batch_capacity >
          OverlayBudget::maximum_supported_saddle_batch_count ||
      result.required_target_component_capacity >
          OverlayBudget::maximum_supported_target_component_count ||
      result.required_target_component_facet_reference_capacity >
          OverlayBudget::
              maximum_supported_target_component_facet_reference_count ||
      result.required_target_component_point_id_reference_capacity >
          OverlayBudget::
              maximum_supported_target_component_point_id_reference_count ||
      result.required_committed_chain_segment_capacity >
          OverlayBudget::
              maximum_supported_committed_chain_segment_count) {
    throw std::logic_error(
        "the derived critical-catalog arm-Gamma preflight exceeds a "
        "certified static cap");
  }
}

[[nodiscard]] bool overlay_budget_covers_preflight(
    const OverlayBudget& budget,
    const OverlayResult& result) {
  return budget.maximum_saddle_event_count >=
             result.required_saddle_event_capacity &&
         budget.maximum_arm_count >= result.required_arm_capacity &&
         budget.maximum_saddle_batch_count >=
             result.required_saddle_batch_capacity &&
         budget.maximum_target_component_count >=
             result.required_target_component_capacity &&
         budget.maximum_target_component_facet_reference_count >=
             result.required_target_component_facet_reference_capacity &&
         budget.maximum_target_component_point_id_reference_count >=
             result.required_target_component_point_id_reference_capacity &&
         budget.maximum_committed_chain_segment_count >=
             result.required_committed_chain_segment_capacity;
}

[[nodiscard]] bool reduced_gamma_budget_covers_preflight(
    std::size_t point_count,
    std::size_t order,
    const ExactStrictGammaBudget& budget) {
  const std::size_t required_facet_count =
      bounded_binomial(point_count, order);
  const std::size_t required_coface_count =
      bounded_binomial(point_count, order + 1U);
  const std::size_t required_union_attempt_count = checked_multiply(
      order,
      required_coface_count,
      "the reduced-Gamma union-attempt preflight overflows");
  return budget.maximum_enumerated_facet_count >=
             required_facet_count &&
         budget.maximum_enumerated_coface_count >=
             required_coface_count &&
         budget.maximum_union_attempt_count >=
             required_union_attempt_count;
}

template <typename Value>
[[nodiscard]] bool span_equals_vector(
    std::span<const Value> left,
    const std::vector<Value>& right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin());
}

struct SaddleReference {
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};
};

[[nodiscard]] std::vector<SaddleReference>
select_requested_order_saddles(
    const ExactCriticalCatalogResult& catalog,
    std::size_t order,
    OverlayResult& result) {
  std::vector<SaddleReference> references;
  for (std::size_t batch_index = 0U;
       batch_index < catalog.h0_batches.size();
       ++batch_index) {
    const ExactCriticalH0Batch& batch = catalog.h0_batches[batch_index];
    ++result.counters.catalog_h0_batch_scan_count;
    if (batch.order != order) {
      continue;
    }
    for (const std::size_t event_index : batch.saddle_event_indices) {
      if (event_index >= catalog.events.size()) {
        throw std::logic_error(
            "a freshly verified catalogue saddle reference is invalid");
      }
      const ExactCriticalEvent& event = catalog.events[event_index];
      if (event.event_index != event_index ||
          !event.saddle_order.has_value() ||
          *event.saddle_order != order ||
          event.squared_level != batch.squared_level) {
        throw std::logic_error(
            "a freshly verified catalogue H0 batch has an incoherent "
            "saddle reference");
      }
      references.push_back(SaddleReference{event_index, batch_index});
    }
  }
  std::sort(
      references.begin(),
      references.end(),
      [](const SaddleReference& left, const SaddleReference& right) {
        return left.catalog_event_index < right.catalog_event_index;
      });
  if (std::adjacent_find(
          references.begin(),
          references.end(),
          [](const SaddleReference& left, const SaddleReference& right) {
            return left.catalog_event_index == right.catalog_event_index;
          }) != references.end()) {
    throw std::logic_error(
        "a requested-order catalogue saddle appears in several H0 "
        "batches");
  }

  std::vector<std::size_t> expected_event_indices;
  for (std::size_t event_index = 0U;
       event_index < catalog.events.size();
       ++event_index) {
    const ExactCriticalEvent& event = catalog.events[event_index];
    if (event.event_index != event_index) {
      throw std::logic_error(
          "a freshly verified catalogue lost canonical event indices");
    }
    if (event.saddle_order.has_value() &&
        *event.saddle_order == order) {
      expected_event_indices.push_back(event_index);
    }
  }
  if (expected_event_indices.size() != references.size()) {
    throw std::logic_error(
        "the catalogue H0 batches do not exhaust requested-order "
        "saddles");
  }
  for (std::size_t index = 0U; index < references.size(); ++index) {
    if (references[index].catalog_event_index !=
        expected_event_indices[index]) {
      throw std::logic_error(
          "the requested-order saddle selection is not exhaustive");
    }
  }
  if (references.size() > result.required_saddle_event_capacity) {
    throw std::logic_error(
        "the actual requested-order saddle count violated preflight");
  }
  result.counters.catalog_saddle_event_reference_count =
      references.size();
  result.requested_order_saddles_exhaustively_selected = true;
  return references;
}

[[nodiscard]] bool catalog_source_matches_family(
    const spatial::CanonicalPointCloud& cloud,
    const ExactCriticalEvent& event,
    std::size_t order,
    const ExactCriticalArmFamilyResult& family) {
  if (event.support_point_ids != event.shell_point_ids ||
      event.support_point_ids.size() < 2U ||
      event.support_point_ids.size() > 4U ||
      event.closed_rank != order + 1U ||
      event.closed_point_ids.size() != event.closed_rank ||
      !event.saddle_order.has_value() ||
      *event.saddle_order != order ||
      family.critical_shell_point_ids != event.shell_point_ids ||
      family.arms.empty() ||
      !family.every_shell_point_enumerated_once ||
      !family.shared_critical_source_replay_certified) {
    return false;
  }
  const ExactCriticalArmInitialSegmentResult& source =
      family.arms.front().descent.initial_segment;
  if (source.critical_shell_point_ids != event.shell_point_ids ||
      source.critical_shell_miniball.facet_point_ids !=
          event.shell_point_ids ||
      source.critical_shell_miniball.support_point_ids !=
          event.support_point_ids ||
      source.critical_shell_miniball.boundary_point_ids !=
          event.shell_point_ids ||
      source.critical_shell_miniball.center != event.center ||
      source.critical_shell_miniball.squared_radius !=
          event.squared_level ||
      !source.global_closed_ball.has_value() ||
      source.closed_rank != event.closed_rank ||
      source.order != order ||
      !source.critical_shell_is_positive_minimal_support ||
      !source.global_shell_matches_critical_shell ||
      !source.closed_rank_and_order_supported ||
      !source.critical_source_certified ||
      source.source_decision !=
          ExactCriticalArmSourceDecision::critical_source_certified) {
    return false;
  }
  const spatial::ClosedBallPartition& partition =
      *source.global_closed_ball;
  return partition.validated_for(cloud) &&
         partition.partition_complete() &&
         partition.squared_radius() == event.squared_level &&
         partition.closed_rank() == event.closed_rank &&
         span_equals_vector(
             partition.interior_ids(), event.interior_point_ids) &&
         span_equals_vector(
             partition.shell_ids(), event.shell_point_ids);
}

void build_and_verify_saddle_families(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const std::vector<SaddleReference>& references,
    OverlayResult& result) {
  if (!result.critical_catalog.has_value()) {
    throw std::logic_error(
        "saddle-family construction requires a certified catalogue");
  }
  const ExactCriticalCatalogResult& catalog = *result.critical_catalog;
  result.saddle_family_records.reserve(references.size());
  bool all_sources_match = true;
  for (const SaddleReference& reference : references) {
    const ExactCriticalEvent& event =
        catalog.events.at(reference.catalog_event_index);
    SaddleFamilyRecord record;
    record.saddle_family_record_index =
        result.saddle_family_records.size();
    record.catalog_event_index = reference.catalog_event_index;
    record.catalog_h0_batch_index =
        reference.catalog_h0_batch_index;
    record.family = build_exact_critical_arm_family_descent(
        cloud,
        event.shell_point_ids,
        result.requested_budget.per_arm_chain_budget);
    ++result.counters.critical_arm_family_build_count;
    const ExactCriticalArmFamilyVerification verification =
        verify_exact_critical_arm_family_descent(
            cloud,
            event.shell_point_ids,
            result.requested_budget.per_arm_chain_budget,
            record.family);
    ++result.counters.critical_arm_family_verification_count;
    if (!verification.exact_critical_arm_family_decision_certified) {
      throw std::logic_error(
          "a catalogue saddle family failed its fresh 6.7 verifier");
    }
    ++result.counters.catalog_source_replay_comparison_count;
    const bool source_matches = catalog_source_matches_family(
        cloud, event, order, record.family);
    all_sources_match = all_sources_match && source_matches;
    if (!source_matches ||
        record.family.decision ==
            ExactCriticalArmFamilyDecision::
                no_family_unsupported_critical_source) {
      throw std::logic_error(
          "a freshly catalogued saddle is incoherent with its 6.7 "
          "critical source");
    }

    result.counters.critical_arm_count = checked_add(
        result.counters.critical_arm_count,
        record.family.arms.size(),
        "the actual catalogue-arm count overflows");
    result.counters.committed_chain_segment_count = checked_add(
        result.counters.committed_chain_segment_count,
        record.family.counters.committed_chain_strict_segment_count,
        "the actual committed chain-segment count overflows");
    if (record.family.decision ==
            ExactCriticalArmFamilyDecision::
                all_arms_complete_at_regular_active_facets &&
        record.family.complete_terminal_label_partition_certified) {
      ++result.counters.complete_critical_arm_family_count;
    } else {
      ++result.counters.incomplete_critical_arm_family_count;
    }
    result.saddle_family_records.push_back(std::move(record));
  }
  if (result.counters.critical_arm_count >
          result.required_arm_capacity ||
      result.counters.committed_chain_segment_count >
          result.required_committed_chain_segment_capacity ||
      result.saddle_family_records.size() >
          result.required_saddle_event_capacity) {
    throw std::logic_error(
        "the actual critical-arm family work violated overlay "
        "preflight");
  }
  result.critical_arm_families_fresh_replay_certified = true;
  result.catalog_sources_match_all_arm_families = all_sources_match;
  result.all_critical_arm_families_complete =
      result.counters.incomplete_critical_arm_family_count == 0U &&
      result.counters.complete_critical_arm_family_count ==
          result.saddle_family_records.size();
}

[[nodiscard]] std::map<std::size_t, std::vector<std::size_t>>
families_by_catalog_batch(const OverlayResult& result) {
  std::map<std::size_t, std::vector<std::size_t>> families_by_batch;
  for (std::size_t family_index = 0U;
       family_index < result.saddle_family_records.size();
       ++family_index) {
    const SaddleFamilyRecord& family =
        result.saddle_family_records[family_index];
    if (family.saddle_family_record_index != family_index) {
      throw std::logic_error(
          "a saddle-family record lost its canonical arena index");
    }
    families_by_batch[family.catalog_h0_batch_index].push_back(
        family_index);
  }
  if (families_by_batch.size() >
      result.required_saddle_batch_capacity) {
    throw std::logic_error(
        "the actual saddle H0-batch count violated preflight");
  }
  return families_by_batch;
}

[[nodiscard]] std::size_t unique_saddle_group_index(
    const ExactReducedGammaBatchResult& batch,
    const ExactCriticalEvent& event,
    OverlayResult& result) {
  ++result.counters.saddle_coface_group_lookup_count;
  std::optional<std::size_t> found_group_index;
  for (std::size_t group_index = 0U;
       group_index < batch.groups.size();
       ++group_index) {
    const ExactReducedGammaBatchGroup& group =
        batch.groups[group_index];
    for (const std::vector<PointId>& coface :
         group.equal_level_coface_point_ids) {
      ++result.counters.saddle_coface_label_scan_count;
      if (coface != event.closed_point_ids) {
        continue;
      }
      if (found_group_index.has_value()) {
        throw std::logic_error(
            "one catalogue saddle coface appears in several reduced-Gamma "
            "groups");
      }
      found_group_index = group_index;
    }
  }
  if (!found_group_index.has_value() ||
      batch.groups[*found_group_index].kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
    throw std::logic_error(
        "a catalogue saddle coface has no unique non-deferred "
        "reduced-Gamma group");
  }
  return *found_group_index;
}

[[nodiscard]] std::map<std::vector<PointId>, std::size_t>
index_strict_component_facets(
    const ExactReducedGammaBatchResult& batch,
    OverlayResult& result) {
  std::map<std::vector<PointId>, std::size_t> component_index_by_facet;
  const std::vector<ExactStrictGammaComponentWitness>& components =
      batch.gamma_transition.strict_gamma.components;
  for (std::size_t component_index = 0U;
       component_index < components.size();
       ++component_index) {
    for (const std::vector<PointId>& facet :
         components[component_index].facet_point_ids) {
      ++result.counters.strict_component_facet_label_scan_count;
      if (!component_index_by_facet.emplace(facet, component_index).second) {
        throw std::logic_error(
            "one critical-arm endpoint facet appears in several "
            "strict-Gamma components");
      }
    }
  }
  return component_index_by_facet;
}

[[nodiscard]] std::size_t unique_strict_component_index(
    const std::map<std::vector<PointId>, std::size_t>&
        component_index_by_facet,
    const std::vector<PointId>& facet_point_ids) {
  const auto found = component_index_by_facet.find(facet_point_ids);
  if (found == component_index_by_facet.end()) {
    throw std::logic_error(
        "a critical-arm endpoint facet is absent from strict full-pi0 "
        "Gamma");
  }
  return found->second;
}

[[nodiscard]] ExactReducedGammaStrictComponentKind
strict_component_kind(
    const ExactReducedGammaBatchResult& batch,
    std::size_t strict_component_index) {
  if (strict_component_index >=
          batch.strict_component_classifications.size() ||
      batch.strict_component_classifications[strict_component_index]
              .strict_component_index != strict_component_index) {
    throw std::logic_error(
        "a strict target component has no reduced classification");
  }
  return batch.strict_component_classifications[strict_component_index]
      .kind;
}

void append_target_component_if_new(
    std::size_t batch_record_index,
    std::size_t strict_component_index,
    const ExactReducedGammaBatchResult& batch,
    BatchRecord& batch_record,
    OverlayResult& result,
    std::map<std::pair<std::size_t, std::size_t>, std::size_t>&
        target_index_by_key,
    std::size_t& target_component_index,
    bool& inserted) {
  const std::pair<std::size_t, std::size_t> key{
      batch_record_index, strict_component_index};
  const auto existing = target_index_by_key.find(key);
  if (existing != target_index_by_key.end()) {
    target_component_index = existing->second;
    inserted = false;
    return;
  }
  if (strict_component_index >=
      batch.gamma_transition.strict_gamma.components.size()) {
    throw std::logic_error(
        "a strict target-component index is invalid");
  }
  const ExactStrictGammaComponentWitness& witness =
      batch.gamma_transition.strict_gamma
          .components[strict_component_index];
  if (witness.facet_point_ids.empty() ||
      witness.canonical_representative_facet_point_ids !=
          witness.facet_point_ids.front() ||
      witness.canonical_representative_facet_point_ids.size() !=
          result.order) {
    throw std::logic_error(
        "a freshly verified strict target component is malformed");
  }

  TargetComponent target;
  target.target_component_index = result.target_components.size();
  target.batch_record_index = batch_record_index;
  target.strict_component_index = strict_component_index;
  target.strict_component = witness;
  target.reduced_component_kind =
      strict_component_kind(batch, strict_component_index);
  result.counters.target_component_point_id_reference_count =
      checked_add(
          result.counters.target_component_point_id_reference_count,
          target.strict_component
              .canonical_representative_facet_point_ids.size(),
          "the actual target-component representative PointId-reference "
          "count overflows");
  for (const std::vector<PointId>& facet :
       target.strict_component.facet_point_ids) {
    if (facet.size() != result.order) {
      throw std::logic_error(
          "a target strict component contains a malformed facet label");
    }
    result.counters.target_component_point_id_reference_count =
        checked_add(
            result.counters.target_component_point_id_reference_count,
            facet.size(),
            "the actual target-component PointId-reference count "
            "overflows");
  }
  result.counters.target_component_facet_reference_count = checked_add(
      result.counters.target_component_facet_reference_count,
      target.strict_component.facet_point_ids.size(),
      "the actual target-component facet-reference count overflows");
  target_component_index = target.target_component_index;
  inserted = true;
  if (!target_index_by_key.emplace(key, target_component_index).second) {
    throw std::logic_error(
        "a target strict component lost key uniqueness");
  }
  batch_record.target_component_indices.push_back(
      target_component_index);
  result.target_components.push_back(std::move(target));
}

void project_one_complete_batch(
    const ExactCriticalCatalogResult& catalog,
    const ExactReducedGammaBatchResult& batch,
    BatchRecord& batch_record,
    OverlayResult& result,
    std::map<std::pair<std::size_t, std::size_t>, std::size_t>&
        target_index_by_key,
    std::set<std::tuple<std::size_t, std::size_t, PointId>>&
        arm_target_keys) {
  const std::map<std::vector<PointId>, std::size_t>
      component_index_by_facet =
          index_strict_component_facets(batch, result);
  for (const std::size_t family_record_index :
       batch_record.saddle_family_record_indices) {
    if (family_record_index >= result.saddle_family_records.size()) {
      throw std::logic_error(
          "a reduced-Gamma batch references an invalid saddle family");
    }
    SaddleFamilyRecord& family_record =
        result.saddle_family_records[family_record_index];
    const ExactCriticalEvent& event =
        catalog.events.at(family_record.catalog_event_index);
    if (family_record.catalog_h0_batch_index !=
            batch_record.catalog_h0_batch_index ||
        event.squared_level != batch_record.squared_level) {
      throw std::logic_error(
          "a saddle family was assigned to an incoherent reduced-Gamma "
          "batch");
    }

    const std::size_t group_index =
        unique_saddle_group_index(batch, event, result);
    const ExactReducedGammaBatchGroup& group =
        batch.groups[group_index];
    family_record.reduced_gamma_batch_record_index =
        batch_record.batch_record_index;
    family_record.reduced_gamma_group_index = group_index;
    family_record.reduced_gamma_group_kind = group.kind;
    family_record.arm_target_indices.reserve(
        family_record.family.arms.size());

    for (const ExactCriticalArmFamilyArmResult& arm :
         family_record.family.arms) {
      if (!arm.active_terminal.has_value() ||
          !arm.terminal_label_class_index.has_value() ||
          arm.descent.decision !=
              ExactCriticalArmDescentDecision::
                  complete_at_regular_active_facet) {
        throw std::logic_error(
            "an allegedly complete saddle family contains an incomplete "
            "arm");
      }
      ++result.counters.arm_initial_component_lookup_count;
      const std::size_t initial_component_index =
          unique_strict_component_index(
              component_index_by_facet,
              arm.descent.initial_segment.arm_facet_point_ids);
      ++result.counters.arm_terminal_component_lookup_count;
      const std::size_t strict_component_index =
          unique_strict_component_index(
              component_index_by_facet,
              arm.active_terminal->facet_point_ids);
      if (initial_component_index != strict_component_index) {
        throw std::logic_error(
            "a critical arm does not remain inside one strict full-pi0 "
            "Gamma component");
      }
      if (std::find(
              group.strict_component_indices.begin(),
              group.strict_component_indices.end(),
              strict_component_index) ==
          group.strict_component_indices.end()) {
        throw std::logic_error(
            "an arm terminal strict component is outside its saddle "
            "Gamma group");
      }

      std::size_t target_component_index = 0U;
      bool inserted = false;
      append_target_component_if_new(
          batch_record.batch_record_index,
          strict_component_index,
          batch,
          batch_record,
          result,
          target_index_by_key,
          target_component_index,
          inserted);
      if (!inserted) {
        ++result.counters.shared_target_arm_count;
      }

      const auto arm_key = std::make_tuple(
          family_record.catalog_event_index,
          result.order,
          arm.removed_shell_point_id);
      if (!arm_target_keys.insert(arm_key).second) {
        throw std::logic_error(
            "one catalogue saddle arm acquired several Gamma targets");
      }
      ArmTarget target;
      target.arm_target_index = result.arm_targets.size();
      target.saddle_family_record_index = family_record_index;
      target.catalog_event_index = family_record.catalog_event_index;
      target.order = result.order;
      target.removed_shell_point_id = arm.removed_shell_point_id;
      target.terminal_label_class_index =
          *arm.terminal_label_class_index;
      target.batch_record_index = batch_record.batch_record_index;
      target.strict_component_index = strict_component_index;
      target.target_component_index = target_component_index;
      family_record.arm_target_indices.push_back(
          target.arm_target_index);
      result.arm_targets.push_back(std::move(target));
    }
  }
}

[[nodiscard]] bool result_facts_match(
    const OverlayResult& observed,
    const OverlayResult& expected) {
  return observed.overlay_candidate_space_size_certified ==
             expected.overlay_candidate_space_size_certified &&
         observed.overlay_preflight_budget_sufficient ==
             expected.overlay_preflight_budget_sufficient &&
         observed.
                 subordinate_geometry_started_only_after_successful_overlay_preflight ==
             expected.
                 subordinate_geometry_started_only_after_successful_overlay_preflight &&
         observed.critical_catalog_fresh_replay_certified ==
             expected.critical_catalog_fresh_replay_certified &&
         observed.no_relevant_extra_shell_degeneracy ==
             expected.no_relevant_extra_shell_degeneracy &&
         observed.requested_order_saddles_exhaustively_selected ==
             expected.requested_order_saddles_exhaustively_selected &&
         observed.critical_arm_families_fresh_replay_certified ==
             expected.critical_arm_families_fresh_replay_certified &&
         observed.catalog_sources_match_all_arm_families ==
             expected.catalog_sources_match_all_arm_families &&
         observed.all_critical_arm_families_complete ==
             expected.all_critical_arm_families_complete &&
         observed.reduced_gamma_batches_fresh_replay_certified ==
             expected.reduced_gamma_batches_fresh_replay_certified &&
         observed.one_reduced_gamma_batch_per_saddle_h0_batch ==
             expected.one_reduced_gamma_batch_per_saddle_h0_batch &&
         observed.every_saddle_coface_in_unique_non_deferred_group ==
             expected.every_saddle_coface_in_unique_non_deferred_group &&
         observed.
                 every_arm_initial_and_terminal_in_same_unique_group_strict_component ==
             expected.
                 every_arm_initial_and_terminal_in_same_unique_group_strict_component &&
         observed.
                 target_components_deduplicated_by_batch_and_strict_component ==
             expected.
                 target_components_deduplicated_by_batch_and_strict_component &&
         observed.target_components_retain_full_pi0_witnesses ==
             expected.target_components_retain_full_pi0_witnesses &&
         observed.reduced_component_kinds_copied_separately ==
             expected.reduced_component_kinds_copied_separately &&
         observed.reduced_group_kinds_inherited_without_morse_inference ==
             expected.reduced_group_kinds_inherited_without_morse_inference &&
         observed.every_catalog_saddle_arm_has_one_target ==
             expected.every_catalog_saddle_arm_has_one_target &&
         observed.diagnostic_outcomes_have_no_gamma_targets ==
             expected.diagnostic_outcomes_have_no_gamma_targets &&
         observed.critical_catalog_arm_gamma_overlay_certified ==
             expected.critical_catalog_arm_gamma_overlay_certified;
}

[[nodiscard]] OverlayResult
compute_exact_critical_catalog_arm_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget) {
  validate_domain(cloud, order, budget);
  OverlayResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = ExactCriticalCatalogArmGammaOverlayScope::
      bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only;
  result.counters.preflight_count = 1U;
  result.diagnostic_outcomes_have_no_gamma_targets = true;
  derive_overlay_preflight(cloud, order, result);
  result.overlay_candidate_space_size_certified = true;
  result.overlay_preflight_budget_sufficient =
      overlay_budget_covers_preflight(budget, result);
  result.
      subordinate_geometry_started_only_after_successful_overlay_preflight =
      true;
  if (!result.overlay_preflight_budget_sufficient) {
    result.decision =
        OverlayDecision::no_overlay_preflight_budget_insufficient;
    return result;
  }

  result.critical_catalog = build_exact_critical_catalog(
      cloud, order, budget.critical_catalog_budget);
  ++result.counters.critical_catalog_build_count;
  const ExactCriticalCatalogVerification catalog_verification =
      verify_exact_critical_catalog(
          cloud,
          order,
          budget.critical_catalog_budget,
          *result.critical_catalog);
  ++result.counters.critical_catalog_verification_count;
  if (!catalog_verification.exact_critical_catalog_decision_certified) {
    throw std::logic_error(
        "the subordinate critical catalogue failed its fresh verifier");
  }
  result.critical_catalog_fresh_replay_certified = true;
  if (result.critical_catalog->decision ==
      ExactCriticalCatalogDecision::
          no_catalog_preflight_budget_insufficient) {
    result.decision =
        OverlayDecision::no_catalog_preflight_budget_insufficient;
    return result;
  }
  if (result.critical_catalog->decision ==
      ExactCriticalCatalogDecision::
          complete_catalog_with_relevant_extra_shell_degeneracy) {
    result.decision =
        OverlayDecision::no_overlay_relevant_extra_shell_degeneracy;
    return result;
  }
  if (result.critical_catalog->decision !=
          ExactCriticalCatalogDecision::
              complete_supported_critical_catalog ||
      !result.critical_catalog->no_relevant_extra_shell_degeneracy ||
      result.critical_catalog->requested_maximum_order != order ||
      result.critical_catalog->effective_maximum_order != order) {
    throw std::logic_error(
        "the fresh critical catalogue has no supported exact-order "
        "decision");
  }
  result.no_relevant_extra_shell_degeneracy = true;

  const std::vector<SaddleReference> saddle_references =
      select_requested_order_saddles(
          *result.critical_catalog, order, result);
  build_and_verify_saddle_families(
      cloud, order, saddle_references, result);
  if (!result.all_critical_arm_families_complete) {
    if (!result.target_components.empty() ||
        !result.arm_targets.empty() ||
        !result.batch_records.empty()) {
      throw std::logic_error(
          "an incomplete arm-family diagnostic retained Gamma payload");
    }
    result.decision = OverlayDecision::
        no_overlay_incomplete_critical_arm_family;
    return result;
  }

  const auto families_by_batch =
      families_by_catalog_batch(result);
  result.batch_records.reserve(families_by_batch.size());
  const bool gamma_budget_sufficient =
      reduced_gamma_budget_covers_preflight(
          cloud.size(), order, budget.reduced_gamma_batch_budget);
  std::map<std::pair<std::size_t, std::size_t>, std::size_t>
      target_index_by_key;
  std::set<std::tuple<std::size_t, std::size_t, PointId>>
      arm_target_keys;

  for (const auto& [catalog_batch_index, family_record_indices] :
       families_by_batch) {
    if (catalog_batch_index >=
        result.critical_catalog->h0_batches.size()) {
      throw std::logic_error(
          "a saddle family references an invalid catalogue H0 batch");
    }
    const ExactCriticalH0Batch& catalog_batch =
        result.critical_catalog->h0_batches[catalog_batch_index];
    if (catalog_batch.order != order ||
        family_record_indices.empty()) {
      throw std::logic_error(
          "a requested-order saddle family has an invalid H0 batch");
    }
    std::vector<std::size_t> expected_batch_event_indices =
        catalog_batch.saddle_event_indices;
    std::sort(
        expected_batch_event_indices.begin(),
        expected_batch_event_indices.end());
    std::vector<std::size_t> actual_batch_event_indices;
    actual_batch_event_indices.reserve(family_record_indices.size());
    for (const std::size_t family_record_index :
         family_record_indices) {
      if (family_record_index >= result.saddle_family_records.size()) {
        throw std::logic_error(
            "an H0 batch references an invalid saddle-family record");
      }
      actual_batch_event_indices.push_back(
          result.saddle_family_records[family_record_index]
              .catalog_event_index);
    }
    std::sort(
        actual_batch_event_indices.begin(),
        actual_batch_event_indices.end());
    if (actual_batch_event_indices != expected_batch_event_indices) {
      throw std::logic_error(
          "a compact saddle batch does not exhaust its catalogue H0 "
          "saddles");
    }

    ExactReducedGammaBatchResult reduced_batch =
        build_exact_reduced_gamma_batch(
            cloud,
            order,
            catalog_batch.squared_level,
            budget.reduced_gamma_batch_budget);
    ++result.counters.reduced_gamma_batch_build_count;
    const ExactReducedGammaBatchVerification batch_verification =
        verify_exact_reduced_gamma_batch(
            cloud,
            order,
            catalog_batch.squared_level,
            budget.reduced_gamma_batch_budget,
            reduced_batch);
    ++result.counters.reduced_gamma_batch_verification_count;
    if (!batch_verification.exact_reduced_gamma_batch_decision_certified) {
      throw std::logic_error(
          "a subordinate reduced-Gamma batch failed its fresh verifier");
    }

    BatchRecord batch_record;
    batch_record.batch_record_index = result.batch_records.size();
    batch_record.catalog_h0_batch_index = catalog_batch_index;
    batch_record.squared_level = catalog_batch.squared_level;
    batch_record.saddle_family_record_indices =
        family_record_indices;
    if (reduced_batch.decision ==
        ExactReducedGammaBatchDecision::
            no_batch_preflight_budget_insufficient) {
      ++result.counters.insufficient_reduced_gamma_batch_count;
      if (gamma_budget_sufficient) {
        throw std::logic_error(
            "a reduced-Gamma batch rejected a sufficient preflight "
            "budget");
      }
    } else if (
        reduced_batch.decision ==
        ExactReducedGammaBatchDecision::
            complete_exhaustive_reduced_gamma_batch) {
      ++result.counters.complete_reduced_gamma_batch_count;
      if (!gamma_budget_sufficient ||
          !reduced_batch.gamma_transition_fresh_replay_certified ||
          !reduced_batch.strict_components_exhaustively_classified ||
          !reduced_batch.transition_groups_exhaustively_classified ||
          !reduced_batch.strict_components_partitioned_within_groups) {
        throw std::logic_error(
            "a reduced-Gamma batch has an incoherent complete decision");
      }
      project_one_complete_batch(
          *result.critical_catalog,
          reduced_batch,
          batch_record,
          result,
          target_index_by_key,
          arm_target_keys);
      result.batch_records.push_back(std::move(batch_record));
    } else {
      throw std::logic_error(
          "a reduced-Gamma batch returned an uncertified decision");
    }
  }
  result.reduced_gamma_batches_fresh_replay_certified = true;
  result.one_reduced_gamma_batch_per_saddle_h0_batch =
      families_by_batch.empty() ||
      (gamma_budget_sufficient &&
       result.batch_records.size() == families_by_batch.size() &&
       result.counters.reduced_gamma_batch_build_count ==
           families_by_batch.size() &&
       result.counters.reduced_gamma_batch_verification_count ==
           families_by_batch.size());

  if (!gamma_budget_sufficient && !families_by_batch.empty()) {
    if (result.counters.insufficient_reduced_gamma_batch_count !=
            families_by_batch.size() ||
        !result.batch_records.empty() ||
        !result.target_components.empty() ||
        !result.arm_targets.empty()) {
      throw std::logic_error(
          "an insufficient reduced-Gamma diagnostic retained target "
          "payload");
    }
    result.decision = OverlayDecision::
        no_overlay_reduced_gamma_batch_preflight_budget_insufficient;
    return result;
  }

  result.counters.target_component_count =
      result.target_components.size();
  result.counters.arm_target_count = result.arm_targets.size();
  if (result.target_components.size() >
          result.required_target_component_capacity ||
      result.counters.target_component_facet_reference_count >
          result.required_target_component_facet_reference_capacity ||
      result.counters.target_component_point_id_reference_count >
          result.required_target_component_point_id_reference_capacity ||
      result.counters.strict_component_facet_label_scan_count >
          result.required_target_component_facet_reference_capacity ||
      result.arm_targets.size() > result.required_arm_capacity ||
      result.counters.complete_reduced_gamma_batch_count !=
          families_by_batch.size() ||
      result.counters.insufficient_reduced_gamma_batch_count != 0U ||
      result.counters.saddle_coface_group_lookup_count !=
          result.saddle_family_records.size() ||
      result.counters.arm_initial_component_lookup_count !=
          result.counters.critical_arm_count ||
      result.counters.arm_terminal_component_lookup_count !=
          result.counters.critical_arm_count ||
      target_index_by_key.size() != result.target_components.size() ||
      arm_target_keys.size() != result.arm_targets.size() ||
      result.counters.shared_target_arm_count +
              result.target_components.size() !=
          result.arm_targets.size()) {
    throw std::logic_error(
        "the compact catalogue-arm Gamma target arenas violated "
        "preflight or deduplication");
  }
  for (const SaddleFamilyRecord& family :
       result.saddle_family_records) {
    if (!family.reduced_gamma_batch_record_index.has_value() ||
        !family.reduced_gamma_group_index.has_value() ||
        !family.reduced_gamma_group_kind.has_value() ||
        family.arm_target_indices.size() != family.family.arms.size()) {
      throw std::logic_error(
          "a complete catalogue saddle family lacks Gamma targets");
    }
  }
  for (std::size_t target_index = 0U;
       target_index < result.target_components.size();
       ++target_index) {
    if (result.target_components[target_index].target_component_index !=
        target_index) {
      throw std::logic_error(
          "the target-component arena lost canonical indices");
    }
  }
  for (std::size_t target_index = 0U;
       target_index < result.arm_targets.size();
       ++target_index) {
    if (result.arm_targets[target_index].arm_target_index !=
        target_index) {
      throw std::logic_error(
          "the arm-target arena lost canonical indices");
    }
  }

  result.every_saddle_coface_in_unique_non_deferred_group = true;
  result.
      every_arm_initial_and_terminal_in_same_unique_group_strict_component =
      true;
  result.target_components_deduplicated_by_batch_and_strict_component =
      true;
  result.target_components_retain_full_pi0_witnesses = true;
  result.reduced_component_kinds_copied_separately = true;
  result.reduced_group_kinds_inherited_without_morse_inference = true;
  result.every_catalog_saddle_arm_has_one_target =
      result.arm_targets.size() == result.counters.critical_arm_count;
  result.critical_catalog_arm_gamma_overlay_certified =
      result.overlay_candidate_space_size_certified &&
      result.overlay_preflight_budget_sufficient &&
      result.
          subordinate_geometry_started_only_after_successful_overlay_preflight &&
      result.critical_catalog_fresh_replay_certified &&
      result.no_relevant_extra_shell_degeneracy &&
      result.requested_order_saddles_exhaustively_selected &&
      result.critical_arm_families_fresh_replay_certified &&
      result.catalog_sources_match_all_arm_families &&
      result.all_critical_arm_families_complete &&
      result.reduced_gamma_batches_fresh_replay_certified &&
      result.one_reduced_gamma_batch_per_saddle_h0_batch &&
      result.every_saddle_coface_in_unique_non_deferred_group &&
      result.
          every_arm_initial_and_terminal_in_same_unique_group_strict_component &&
      result.
          target_components_deduplicated_by_batch_and_strict_component &&
      result.target_components_retain_full_pi0_witnesses &&
      result.reduced_component_kinds_copied_separately &&
      result.reduced_group_kinds_inherited_without_morse_inference &&
      result.every_catalog_saddle_arm_has_one_target &&
      result.diagnostic_outcomes_have_no_gamma_targets;
  if (!result.critical_catalog_arm_gamma_overlay_certified) {
    throw std::logic_error(
        "the exhaustive catalogue saddle-arm Gamma overlay failed "
        "certification");
  }
  result.decision = OverlayDecision::
      complete_exhaustive_catalog_saddle_arm_gamma_overlay;
  return result;
}

[[nodiscard]] bool saddle_family_records_match(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const OverlayBudget& budget,
    const OverlayResult& observed,
    const OverlayResult& expected) {
  if (observed.saddle_family_records.size() !=
          expected.saddle_family_records.size() ||
      !expected.critical_catalog.has_value()) {
    return observed.saddle_family_records.empty() &&
           expected.saddle_family_records.empty();
  }
  const ExactCriticalCatalogResult& catalog =
      *expected.critical_catalog;
  for (std::size_t index = 0U;
       index < expected.saddle_family_records.size();
       ++index) {
    const SaddleFamilyRecord& observed_record =
        observed.saddle_family_records[index];
    const SaddleFamilyRecord& expected_record =
        expected.saddle_family_records[index];
    if (observed_record.saddle_family_record_index !=
            expected_record.saddle_family_record_index ||
        observed_record.catalog_event_index !=
            expected_record.catalog_event_index ||
        observed_record.catalog_h0_batch_index !=
            expected_record.catalog_h0_batch_index ||
        observed_record.reduced_gamma_batch_record_index !=
            expected_record.reduced_gamma_batch_record_index ||
        observed_record.reduced_gamma_group_index !=
            expected_record.reduced_gamma_group_index ||
        observed_record.reduced_gamma_group_kind !=
            expected_record.reduced_gamma_group_kind ||
        observed_record.arm_target_indices !=
            expected_record.arm_target_indices ||
        expected_record.catalog_event_index >= catalog.events.size()) {
      return false;
    }
    const ExactCriticalEvent& event =
        catalog.events[expected_record.catalog_event_index];
    const ExactCriticalArmFamilyVerification family_verification =
        verify_exact_critical_arm_family_descent(
            cloud,
            event.shell_point_ids,
            budget.per_arm_chain_budget,
            observed_record.family);
    if (!family_verification.
            exact_critical_arm_family_decision_certified ||
        !catalog_source_matches_family(
            cloud, event, order, observed_record.family)) {
      return false;
    }
  }
  return true;
}

}  // namespace

ExactCriticalCatalogArmGammaOverlayVerification
verify_exact_critical_catalog_arm_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget,
    const OverlayResult& result) {
  const OverlayResult expected =
      compute_exact_critical_catalog_arm_gamma_overlay(
          cloud, order, budget);
  ExactCriticalCatalogArmGammaOverlayVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.order == order && result.order == expected.order;
  verification.derived_preflight_sizes_certified =
      result.exhaustive_facet_count == expected.exhaustive_facet_count &&
      result.required_saddle_event_capacity ==
          expected.required_saddle_event_capacity &&
      result.required_arm_capacity == expected.required_arm_capacity &&
      result.required_saddle_batch_capacity ==
          expected.required_saddle_batch_capacity &&
      result.required_target_component_capacity ==
          expected.required_target_component_capacity &&
      result.required_target_component_facet_reference_capacity ==
          expected.required_target_component_facet_reference_capacity &&
      result.required_target_component_point_id_reference_capacity ==
          expected.required_target_component_point_id_reference_capacity &&
      result.required_committed_chain_segment_capacity ==
          expected.required_committed_chain_segment_capacity &&
      result.overlay_candidate_space_size_certified ==
          expected.overlay_candidate_space_size_certified;
  verification.critical_catalog_certified =
      result.critical_catalog == expected.critical_catalog;
  verification.saddle_family_records_certified =
      saddle_family_records_match(
          cloud, order, budget, result, expected);
  verification.batch_records_certified =
      result.batch_records == expected.batch_records;
  verification.target_components_certified =
      result.target_components == expected.target_components;
  verification.arm_targets_certified =
      result.arm_targets == expected.arm_targets;
  verification.result_facts_certified =
      result_facts_match(result, expected);
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalCatalogArmGammaOverlayScope::
          bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.critical_catalog_certified &&
      verification.saddle_family_records_certified &&
      verification.batch_records_certified &&
      verification.target_components_certified &&
      verification.arm_targets_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified;
  verification.
      exact_critical_catalog_arm_gamma_overlay_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogArmGammaOverlayResult
build_exact_critical_catalog_arm_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget) {
  OverlayResult result =
      compute_exact_critical_catalog_arm_gamma_overlay(
          cloud, order, budget);
  const ExactCriticalCatalogArmGammaOverlayVerification verification =
      verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, order, budget, result);
  if (!verification.
          exact_critical_catalog_arm_gamma_overlay_decision_certified) {
    throw std::logic_error(
        "the exact critical-catalog arm-Gamma overlay failed its fresh "
        "replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
