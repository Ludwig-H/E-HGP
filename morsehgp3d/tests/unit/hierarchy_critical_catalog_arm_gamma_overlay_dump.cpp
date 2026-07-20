#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/hierarchy/critical_catalog_arm_gamma_overlay.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalArmFamilyArmResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaArmTarget;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaSaddleFamilyRecord;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaTargetComponent;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalEvent;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactReducedGammaStrictComponentKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

struct FixedCase {
  std::string_view name;
  std::vector<CertifiedPoint3> input_points;
  std::vector<std::string_view> input_labels;
  std::size_t order{};
};

struct SemanticArmRecord {
  const ExactCriticalEvent* event{};
  PointId removed_shell_point_id{};
  std::vector<PointId> initial_facet_point_ids;
  std::vector<PointId> terminal_facet_point_ids;
  const ExactCriticalCatalogArmGammaTargetComponent* target_component{};
  ExactReducedGammaBatchGroupKind group_kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};
};

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactCriticalCatalogArmGammaOverlayBudget full_budget() {
  ExactCriticalCatalogArmGammaOverlayBudget budget;
  budget.critical_catalog_budget = ExactCriticalCatalogBudget{
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
  budget.per_arm_chain_budget = ExactFacetDescentChainBudget{
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count};
  budget.reduced_gamma_batch_budget = ExactStrictGammaBudget{
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
  budget.maximum_saddle_event_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_saddle_event_count;
  budget.maximum_arm_count = ExactCriticalCatalogArmGammaOverlayBudget::
                                 maximum_supported_arm_count;
  budget.maximum_saddle_batch_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_saddle_batch_count;
  budget.maximum_target_component_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_count;
  budget.maximum_target_component_facet_reference_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_facet_reference_count;
  budget.maximum_target_component_point_id_reference_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_point_id_reference_count;
  budget.maximum_committed_chain_segment_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_committed_chain_segment_count;
  return budget;
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_point_ids(
    std::ostream& output,
    std::span<const PointId> point_ids) {
  output << '[';
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << point_ids[index];
  }
  output << ']';
}

void write_facets(
    std::ostream& output,
    std::span<const std::vector<PointId>> facets) {
  output << '[';
  for (std::size_t index = 0U; index < facets.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_point_ids(output, facets[index]);
  }
  output << ']';
}

void write_canonical_points(
    std::ostream& output,
    const CanonicalPointCloud& cloud,
    std::span<const std::string_view> input_labels) {
  output << '[';
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const PointId point_id = static_cast<PointId>(index);
    const std::size_t source_index = cloud.source_index(point_id);
    const auto input_bits = cloud.point(point_id).canonical_input_bits();
    output << "{\"id\":" << point_id << ",\"input_bits\":[\""
           << morsehgp3d::exact::binary64_hex(input_bits[0]) << "\",\""
           << morsehgp3d::exact::binary64_hex(input_bits[1]) << "\",\""
           << morsehgp3d::exact::binary64_hex(input_bits[2])
           << "\"],\"label\":\"" << input_labels[source_index]
           << "\",\"source_index\":" << source_index << '}';
  }
  output << ']';
}

[[nodiscard]] std::string_view decision_name(
    ExactCriticalCatalogArmGammaOverlayDecision decision) {
  switch (decision) {
    case ExactCriticalCatalogArmGammaOverlayDecision::not_certified:
      return "not_certified";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        no_overlay_preflight_budget_insufficient:
      return "no_overlay_preflight_budget_insufficient";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        no_catalog_preflight_budget_insufficient:
      return "no_catalog_preflight_budget_insufficient";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        no_overlay_relevant_extra_shell_degeneracy:
      return "no_overlay_relevant_extra_shell_degeneracy";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        no_overlay_incomplete_critical_arm_family:
      return "no_overlay_incomplete_critical_arm_family";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        no_overlay_reduced_gamma_batch_preflight_budget_insufficient:
      return "no_overlay_reduced_gamma_batch_preflight_budget_insufficient";
    case ExactCriticalCatalogArmGammaOverlayDecision::
        complete_exhaustive_catalog_saddle_arm_gamma_overlay:
      return "complete_exhaustive_catalog_saddle_arm_gamma_overlay";
  }
  throw std::logic_error(
      "an unknown catalogue-arm overlay decision was dumped");
}

[[nodiscard]] std::string_view reduced_component_kind_name(
    ExactReducedGammaStrictComponentKind kind) {
  switch (kind) {
    case ExactReducedGammaStrictComponentKind::omitted_isolated_facet:
      return "omitted_isolated_facet";
    case ExactReducedGammaStrictComponentKind::prior_nontrivial_reduced_root:
      return "prior_nontrivial_reduced_root";
  }
  throw std::logic_error("an unknown reduced component kind was dumped");
}

[[nodiscard]] std::string_view reduced_group_kind_name(
    ExactReducedGammaBatchGroupKind kind) {
  switch (kind) {
    case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
      return "deferred_isolated_facet";
    case ExactReducedGammaBatchGroupKind::birth:
      return "birth";
    case ExactReducedGammaBatchGroupKind::continuation:
      return "continuation";
    case ExactReducedGammaBatchGroupKind::multifusion:
      return "multifusion";
  }
  throw std::logic_error("an unknown reduced group kind was dumped");
}

[[nodiscard]] std::vector<SemanticArmRecord> semantic_arm_records(
    const ExactCriticalCatalogArmGammaOverlayResult& result) {
  if (!result.critical_catalog.has_value()) {
    if (!result.arm_targets.empty()) {
      throw std::logic_error("arm targets exist without a critical catalogue");
    }
    return {};
  }

  std::vector<SemanticArmRecord> records;
  records.reserve(result.arm_targets.size());
  for (const ExactCriticalCatalogArmGammaArmTarget& arm_target :
       result.arm_targets) {
    if (arm_target.catalog_event_index >=
            result.critical_catalog->events.size() ||
        arm_target.saddle_family_record_index >=
            result.saddle_family_records.size() ||
        arm_target.target_component_index >= result.target_components.size()) {
      throw std::logic_error(
          "an arm target references an absent overlay record");
    }
    const ExactCriticalEvent& event =
        result.critical_catalog->events[arm_target.catalog_event_index];
    const ExactCriticalCatalogArmGammaSaddleFamilyRecord& family_record =
        result.saddle_family_records[arm_target.saddle_family_record_index];
    const ExactCriticalCatalogArmGammaTargetComponent& target_component =
        result.target_components[arm_target.target_component_index];
    if (arm_target.terminal_label_class_index >=
            family_record.family.terminal_label_classes.size() ||
        !family_record.reduced_gamma_group_kind.has_value()) {
      throw std::logic_error(
          "a complete arm target has no terminal or group kind");
    }
    if (family_record.catalog_event_index != arm_target.catalog_event_index ||
        target_component.batch_record_index != arm_target.batch_record_index ||
        target_component.strict_component_index !=
            arm_target.strict_component_index) {
      throw std::logic_error("an arm target has incoherent overlay indices");
    }
    const auto family_arm = std::find_if(
        family_record.family.arms.begin(),
        family_record.family.arms.end(),
        [&arm_target](const ExactCriticalArmFamilyArmResult& arm) {
          return arm.removed_shell_point_id ==
                 arm_target.removed_shell_point_id;
        });
    const std::size_t matching_arm_count = static_cast<std::size_t>(
        std::count_if(
            family_record.family.arms.begin(),
            family_record.family.arms.end(),
            [&arm_target](const ExactCriticalArmFamilyArmResult& arm) {
              return arm.removed_shell_point_id ==
                     arm_target.removed_shell_point_id;
            }));
    if (family_arm == family_record.family.arms.end() ||
        matching_arm_count != 1U ||
        !family_arm->active_terminal.has_value() ||
        !family_arm->terminal_label_class_index.has_value() ||
        *family_arm->terminal_label_class_index !=
            arm_target.terminal_label_class_index ||
        family_arm->descent.initial_segment.removed_shell_point_id !=
            arm_target.removed_shell_point_id ||
        family_arm->descent.initial_segment.order != arm_target.order) {
      throw std::logic_error("an arm target has no unique coherent family arm");
    }
    const auto& terminal_class =
        family_record.family
            .terminal_label_classes[arm_target.terminal_label_class_index]
            .canonical_terminal;
    if (*family_arm->active_terminal != terminal_class) {
      throw std::logic_error(
          "an arm target active terminal disagrees with its label class");
    }
    records.push_back(SemanticArmRecord{
        &event,
        arm_target.removed_shell_point_id,
        family_arm->descent.initial_segment.arm_facet_point_ids,
        family_arm->active_terminal->facet_point_ids,
        &target_component,
        *family_record.reduced_gamma_group_kind});
  }
  std::sort(
      records.begin(),
      records.end(),
      [](const SemanticArmRecord& left, const SemanticArmRecord& right) {
        return std::tie(
                   left.event->squared_level,
                   left.event->closed_point_ids,
                   left.removed_shell_point_id) <
               std::tie(
                   right.event->squared_level,
                   right.event->closed_point_ids,
                   right.removed_shell_point_id);
      });
  return records;
}

void write_case(const FixedCase& fixed_case) {
  if (fixed_case.input_points.size() != fixed_case.input_labels.size()) {
    throw std::logic_error("a fixed overlay case has mismatched point labels");
  }
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(fixed_case.input_points);
  const ExactCriticalCatalogArmGammaOverlayResult result =
      morsehgp3d::hierarchy::
          build_exact_critical_catalog_arm_gamma_overlay(
              cloud, fixed_case.order, full_budget());
  const std::vector<SemanticArmRecord> records = semantic_arm_records(result);

  std::cout << "{\"arm_targets\":[";
  for (std::size_t index = 0U; index < records.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const SemanticArmRecord& record = records[index];
    std::cout << "{\"event_closed_point_ids\":";
    write_point_ids(std::cout, record.event->closed_point_ids);
    std::cout << ",\"event_squared_level\":";
    write_level(std::cout, record.event->squared_level);
    std::cout << ",\"group_kind\":\""
              << reduced_group_kind_name(record.group_kind)
              << "\",\"initial_facet_point_ids\":";
    write_point_ids(std::cout, record.initial_facet_point_ids);
    std::cout << ",\"reduced_component_kind\":\""
              << reduced_component_kind_name(
                     record.target_component->reduced_component_kind)
              << "\",\"removed_shell_point_id\":"
              << record.removed_shell_point_id
              << ",\"target_component_facet_point_ids\":";
    write_facets(
        std::cout,
        record.target_component->strict_component.facet_point_ids);
    std::cout << ",\"terminal_facet_point_ids\":";
    write_point_ids(std::cout, record.terminal_facet_point_ids);
    std::cout << '}';
  }
  std::cout << "],\"canonical_points\":";
  write_canonical_points(std::cout, cloud, fixed_case.input_labels);
  std::cout << ",\"case\":\"" << fixed_case.name
            << "\",\"decision\":\"" << decision_name(result.decision)
            << "\",\"order\":" << result.order << "}\n";
}

[[nodiscard]] std::vector<FixedCase> fixed_cases() {
  std::vector<FixedCase> cases;
  cases.push_back(FixedCase{
      "mirror_simultaneous_q5",
      {point(-2.0, 0.0),
       point(0.0, -3.0),
       point(0.0, 3.0),
       point(2.0, 0.0)},
      {"A", "B", "C", "D"},
      2U});
  cases.push_back(FixedCase{
      "shared_terminal_interior",
      {point(-8.0, 1.0),
       point(-5.0, -7.0),
       point(-3.0, -8.0),
       point(4.0, 8.0),
       point(5.0, -7.0)},
      {"A", "B", "C", "D", "E"},
      3U});
  return cases;
}

}  // namespace

int main() {
  try {
    for (const FixedCase& fixed_case : fixed_cases()) {
      write_case(fixed_case);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "catalogue-arm Gamma overlay dump failed: " << error.what()
              << '\n';
    return 1;
  }
}
