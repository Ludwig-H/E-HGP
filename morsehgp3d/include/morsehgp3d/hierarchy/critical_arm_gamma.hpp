#pragma once

#include "morsehgp3d/hierarchy/critical_arm.hpp"
#include "morsehgp3d/hierarchy/gamma.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

// This bridge composes the complete event-local arm family from milestone 6.7
// with the exhaustive strict Gamma cut from milestone 6.8.  Its component
// indices describe the state strictly below one critical level.  They are not
// hierarchy roots, Morse attachments, merge decisions, forest nodes, or a
// public status; equal-level events still require one simultaneous batch.
enum class ExactCriticalArmGammaDecision : std::uint8_t {
  not_certified,
  no_classification_unsupported_critical_source,
  no_classification_incomplete_arm_family,
  no_classification_strict_gamma_preflight_budget_insufficient,
  complete_arm_to_strict_gamma_component_classification,
};

enum class ExactCriticalArmGammaScope : std::uint8_t {
  unspecified,
  bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only,
};

struct ExactCriticalArmGammaTerminalClassClassification {
  std::size_t terminal_label_class_index{};
  std::vector<spatial::PointId> terminal_facet_point_ids;
  std::vector<spatial::PointId> removed_shell_point_ids;
  std::size_t strict_gamma_component_index{};

  friend bool operator==(
      const ExactCriticalArmGammaTerminalClassClassification&,
      const ExactCriticalArmGammaTerminalClassClassification&) = default;
};

struct ExactCriticalArmGammaArmClassification {
  spatial::PointId removed_shell_point_id{};
  std::size_t terminal_label_class_index{};
  std::size_t strict_gamma_component_index{};

  friend bool operator==(
      const ExactCriticalArmGammaArmClassification&,
      const ExactCriticalArmGammaArmClassification&) = default;
};

struct ExactCriticalArmGammaIncidentComponent {
  std::size_t strict_gamma_component_index{};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  std::vector<std::size_t> terminal_label_class_indices;
  std::vector<spatial::PointId> removed_shell_point_ids;

  friend bool operator==(
      const ExactCriticalArmGammaIncidentComponent&,
      const ExactCriticalArmGammaIncidentComponent&) = default;
};

struct ExactCriticalArmGammaCounters {
  std::size_t arm_family_build_count{};
  std::size_t terminal_label_class_count{};
  std::size_t strict_gamma_build_count{};
  std::size_t strict_gamma_source_facet_count{};
  std::size_t terminal_class_component_projection_count{};
  std::size_t arm_component_projection_count{};
  std::size_t incident_component_count{};
  std::size_t same_terminal_label_arm_coalescence_count{};
  std::size_t distinct_terminal_label_component_coalescence_count{};

  friend bool operator==(
      const ExactCriticalArmGammaCounters&,
      const ExactCriticalArmGammaCounters&) = default;
};

struct ExactCriticalArmGammaResult {
  static constexpr const char* proof_basis =
      "exact_complete_critical_arm_family_strict_path_bounded_"
      "exhaustive_open_gamma_component_classification_v1";

  ExactFacetDescentChainBudget requested_per_arm_chain_budget{};
  ExactStrictGammaBudget requested_strict_gamma_budget{};
  std::vector<spatial::PointId> critical_shell_point_ids;
  std::size_t order{};
  exact::ExactLevel critical_squared_level;
  ExactCriticalArmFamilyResult arm_family;
  std::optional<ExactStrictGammaResult> strict_gamma;
  std::vector<ExactCriticalArmGammaTerminalClassClassification>
      terminal_class_classifications;
  std::vector<ExactCriticalArmGammaArmClassification>
      arm_classifications;
  std::vector<ExactCriticalArmGammaIncidentComponent>
      incident_components;
  bool arm_family_fresh_replay_certified{false};
  bool critical_order_and_level_derived_from_shared_source{false};
  bool strict_gamma_cut_fresh_replay_certified{false};
  bool all_terminal_classes_active_and_classified{false};
  bool every_arm_projected_once{false};
  bool terminal_label_partition_refines_gamma_component_partition{false};
  ExactCriticalArmGammaCounters counters{};
  ExactCriticalArmGammaDecision decision{
      ExactCriticalArmGammaDecision::not_certified};
  ExactCriticalArmGammaScope scope{
      ExactCriticalArmGammaScope::unspecified};
};

struct ExactCriticalArmGammaVerification {
  bool requested_per_arm_chain_budget_certified{false};
  bool requested_strict_gamma_budget_certified{false};
  bool input_shell_identity_certified{false};
  bool arm_family_certified{false};
  bool critical_order_and_level_certified{false};
  bool strict_gamma_presence_certified{false};
  bool strict_gamma_certified{false};
  bool terminal_class_classifications_certified{false};
  bool arm_classifications_certified{false};
  bool incident_components_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_arm_gamma_decision_certified{false};
};

// The trusted budgets are independent: the first limits each 6.7 arm chain,
// while the second must cover the all-or-nothing 6.8 Gamma preflight.  The
// critical order and open-cut level are derived from the freshly replayed
// shared source and cannot be supplied by the caller.
[[nodiscard]] ExactCriticalArmGammaResult
build_exact_critical_arm_gamma_component_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    ExactStrictGammaBudget strict_gamma_budget);

// Reconstructs both 6.7 and 6.8 from the external cloud, shell and trusted
// budgets.  No observed order, level, terminal, component, or decision steers
// the replay.
[[nodiscard]] ExactCriticalArmGammaVerification
verify_exact_critical_arm_gamma_component_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    ExactStrictGammaBudget strict_gamma_budget,
    const ExactCriticalArmGammaResult& result);

}  // namespace morsehgp3d::hierarchy
