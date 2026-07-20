#pragma once

#include "morsehgp3d/hierarchy/critical_arm_gamma.hpp"
#include "morsehgp3d/hierarchy/gamma_transition.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

// This overlay reconciles only the caller-supplied, independently complete
// 6.9 events with one exhaustive 6.10 Gamma transition.  It does not certify
// that the supplied list is the complete critical catalogue at (order, level),
// and it creates no persistent root, public birth, merge, or forest node.
struct ExactCriticalEventGammaOverlayRequest {
  std::vector<spatial::PointId> critical_shell_point_ids;
  ExactFacetDescentChainBudget per_arm_chain_budget{};

  friend bool operator==(
      const ExactCriticalEventGammaOverlayRequest&,
      const ExactCriticalEventGammaOverlayRequest&) = default;
};

struct ExactCriticalEventGammaOverlayBudget {
  static constexpr std::size_t maximum_supported_event_count = 8U;
  static constexpr std::size_t maximum_supported_total_arm_count = 32U;

  std::size_t maximum_event_count{};
  std::size_t maximum_total_arm_count{};

  friend bool operator==(
      const ExactCriticalEventGammaOverlayBudget&,
      const ExactCriticalEventGammaOverlayBudget&) = default;
};

enum class ExactCriticalEventGammaOverlayDecision : std::uint8_t {
  not_certified,
  no_overlay_preflight_budget_insufficient,
  no_overlay_event_family_not_complete,
  no_overlay_mixed_order_or_exact_level,
  no_overlay_gamma_preflight_budget_insufficient,
  complete_supplied_event_provenance_overlay,
};

enum class ExactCriticalEventGammaOverlayScope : std::uint8_t {
  unspecified,
  bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only,
};

struct ExactCriticalEventGammaArmIncidence {
  spatial::PointId removed_shell_point_id{};
  std::vector<spatial::PointId> initial_arm_facet_point_ids;
  std::vector<spatial::PointId> terminal_facet_point_ids;
  std::size_t terminal_label_class_index{};
  std::size_t strict_gamma_component_index{};

  friend bool operator==(
      const ExactCriticalEventGammaArmIncidence&,
      const ExactCriticalEventGammaArmIncidence&) = default;
};

struct ExactCriticalEventGammaInteriorIncidence {
  spatial::PointId removed_interior_point_id{};
  std::vector<spatial::PointId> equal_level_facet_point_ids;

  friend bool operator==(
      const ExactCriticalEventGammaInteriorIncidence&,
      const ExactCriticalEventGammaInteriorIncidence&) = default;
};

struct ExactCriticalEventGammaProjection {
  // This is an index into canonical_event_requests, not a caller position.
  std::size_t canonical_event_index{};
  std::vector<spatial::PointId> critical_shell_point_ids;
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> critical_coface_point_ids;
  std::size_t equal_level_coface_index{};
  std::vector<ExactCriticalEventGammaArmIncidence> arm_incidences;
  std::vector<ExactCriticalEventGammaInteriorIncidence>
      interior_incidences;
  std::size_t transition_group_index{};
  std::size_t closed_component_index{};

  friend bool operator==(
      const ExactCriticalEventGammaProjection&,
      const ExactCriticalEventGammaProjection&) = default;
};

struct ExactCriticalEventGammaGroupOverlay {
  std::size_t transition_group_index{};
  std::size_t closed_component_index{};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  // These are indices into canonical_event_requests.  Provenance is
  // existential: a nonempty list does not claim to cover every coface below.
  std::vector<std::size_t> canonical_event_indices;
  // Exhaustive relative to the 6.10 equality-coface catalogue only; no
  // missing coface is classified as silent or noncritical.
  std::vector<std::vector<spatial::PointId>>
      equal_level_cofaces_without_supplied_event_provenance;
  bool has_supplied_event_provenance{false};

  friend bool operator==(
      const ExactCriticalEventGammaGroupOverlay&,
      const ExactCriticalEventGammaGroupOverlay&) = default;
};

struct ExactCriticalEventGammaOverlayCounters {
  std::size_t preflight_count{};
  std::size_t required_event_count{};
  std::size_t required_total_arm_count{};
  std::size_t event_classification_build_count{};
  std::size_t complete_arm_family_event_count{};
  std::size_t incomplete_arm_family_event_count{};
  std::size_t gamma_preflight_insufficient_event_count{};
  std::size_t complete_event_classification_count{};
  std::size_t common_order_and_level_comparison_count{};
  std::size_t gamma_transition_build_count{};
  std::size_t strict_gamma_core_comparison_count{};
  std::size_t critical_coface_lookup_count{};
  std::size_t deletion_incidence_projection_count{};
  std::size_t arm_incidence_projection_count{};
  std::size_t interior_incidence_projection_count{};
  std::size_t supplied_event_projection_count{};
  std::size_t transition_group_overlay_count{};
  std::size_t group_supplied_event_reference_count{};
  std::size_t group_with_supplied_event_count{};
  std::size_t group_without_supplied_event_count{};

  friend bool operator==(
      const ExactCriticalEventGammaOverlayCounters&,
      const ExactCriticalEventGammaOverlayCounters&) = default;
};

struct ExactCriticalEventGammaOverlayResult {
  static constexpr const char* proof_basis =
      "exact_supplied_complete_critical_arm_gamma_event_cofaces_"
      "reconciled_with_exhaustive_equal_level_gamma_transition_v1";

  ExactCriticalEventGammaOverlayBudget requested_overlay_budget{};
  ExactStrictGammaBudget requested_gamma_budget{};
  std::vector<ExactCriticalEventGammaOverlayRequest>
      canonical_event_requests;
  std::size_t required_event_count{};
  std::size_t required_total_arm_count{};
  std::size_t common_order{};
  exact::ExactLevel common_squared_level;
  std::vector<ExactCriticalArmGammaResult> event_classifications;
  std::optional<ExactGammaTransitionResult> gamma_transition;
  std::vector<ExactCriticalEventGammaProjection> event_projections;
  std::vector<ExactCriticalEventGammaGroupOverlay> group_overlays;
  bool event_requests_canonical_certified{false};
  bool supplied_event_preflight_size_certified{false};
  bool all_event_classifications_fresh_replay_certified{false};
  bool common_order_and_exact_level_derived{false};
  bool all_strict_gamma_cores_match_transition{false};
  bool critical_event_cofaces_distinct_and_equal_level{false};
  bool every_event_deletion_incidence_reconciled{false};
  bool every_supplied_event_projected_once{false};
  bool group_overlays_partition_transition_groups{false};
  bool supplied_event_provenance_order_independent{false};
  ExactCriticalEventGammaOverlayCounters counters{};
  ExactCriticalEventGammaOverlayDecision decision{
      ExactCriticalEventGammaOverlayDecision::not_certified};
  ExactCriticalEventGammaOverlayScope scope{
      ExactCriticalEventGammaOverlayScope::unspecified};
};

struct ExactCriticalEventGammaOverlayVerification {
  bool requested_overlay_budget_certified{false};
  bool requested_gamma_budget_certified{false};
  bool canonical_event_requests_certified{false};
  bool preflight_counts_certified{false};
  bool event_classifications_certified{false};
  bool common_order_and_level_certified{false};
  bool gamma_transition_presence_certified{false};
  bool gamma_transition_certified{false};
  bool event_projections_certified{false};
  bool group_overlays_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_event_gamma_overlay_decision_certified{false};
};

[[nodiscard]] ExactCriticalEventGammaOverlayResult
build_exact_supplied_critical_event_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests,
    ExactCriticalEventGammaOverlayBudget overlay_budget,
    ExactStrictGammaBudget gamma_budget);

[[nodiscard]] ExactCriticalEventGammaOverlayVerification
verify_exact_supplied_critical_event_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactCriticalEventGammaOverlayRequest>
        event_requests,
    ExactCriticalEventGammaOverlayBudget overlay_budget,
    ExactStrictGammaBudget gamma_budget,
    const ExactCriticalEventGammaOverlayResult& result);

}  // namespace morsehgp3d::hierarchy
