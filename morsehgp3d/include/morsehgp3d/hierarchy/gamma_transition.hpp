#pragma once

#include "morsehgp3d/hierarchy/gamma.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// This transition freezes the exhaustive strict Gamma cut at one exact
// (order, level) pair, then applies every facet and coface at equality as one
// simultaneous batch.  It describes full-pi0 cut components only: no group is
// a persistent hierarchy root, public birth, public merge, or forest node.
enum class ExactGammaTransitionDecision : std::uint8_t {
  not_certified,
  no_transition_preflight_budget_insufficient,
  complete_exhaustive_open_to_closed_transition,
};

enum class ExactGammaTransitionScope : std::uint8_t {
  unspecified,
  bounded_exhaustive_gamma_equal_level_transition_only,
};

enum class ExactGammaTransitionGroupKind : std::uint8_t {
  new_closed_component_without_strict_component,
  one_strict_component_continuation,
  multiple_strict_component_coalescence,
};

struct ExactGammaEqualLevelFacetWitness {
  std::vector<spatial::PointId> facet_point_ids;
  exact::ExactLevel squared_level;

  friend bool operator==(
      const ExactGammaEqualLevelFacetWitness&,
      const ExactGammaEqualLevelFacetWitness&) = default;
};

struct ExactGammaTransitionIncidence {
  std::vector<spatial::PointId> coface_point_ids;
  std::vector<spatial::PointId> facet_point_ids;
  std::optional<std::size_t> strict_component_index;
  bool newly_active_at_level{false};

  friend bool operator==(
      const ExactGammaTransitionIncidence&,
      const ExactGammaTransitionIncidence&) = default;
};

struct ExactGammaTransitionGroup {
  std::size_t closed_component_index{};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  std::vector<std::size_t> strict_component_indices;
  std::vector<std::vector<spatial::PointId>>
      newly_active_facet_point_ids;
  std::vector<std::vector<spatial::PointId>>
      equal_level_coface_point_ids;
  ExactGammaTransitionGroupKind kind{
      ExactGammaTransitionGroupKind::
          new_closed_component_without_strict_component};

  friend bool operator==(
      const ExactGammaTransitionGroup&,
      const ExactGammaTransitionGroup&) = default;
};

struct ExactGammaTransitionCounters {
  std::size_t preflight_count{};
  std::size_t required_facet_count{};
  std::size_t required_coface_count{};
  std::size_t required_union_attempt_count{};
  std::size_t strict_gamma_build_count{};
  std::size_t enumerated_facet_count{};
  std::size_t facet_miniball_build_count{};
  std::size_t facet_level_comparison_count{};
  std::size_t strict_facet_replay_count{};
  std::size_t equal_level_facet_count{};
  std::size_t enumerated_coface_count{};
  std::size_t direct_coface_miniball_build_count{};
  std::size_t eleven_point_coface_count{};
  std::size_t eleven_point_deletion_level_lookup_count{};
  std::size_t eleven_point_level_maximum_comparison_count{};
  std::size_t eleven_point_omitted_point_distance_evaluation_count{};
  std::size_t coface_level_comparison_count{};
  std::size_t strict_coface_replay_count{};
  std::size_t equal_level_coface_count{};
  std::size_t equal_level_incidence_count{};
  std::size_t closed_facet_count{};
  std::size_t closed_coface_count{};
  std::size_t closed_disjoint_set_value_count{};
  std::size_t closed_coface_facet_lookup_count{};
  std::size_t closed_union_attempt_count{};
  std::size_t closed_union_merge_count{};
  std::size_t closed_component_count{};
  std::size_t strict_component_projection_count{};
  std::size_t equal_level_facet_projection_count{};
  std::size_t equal_level_coface_projection_count{};
  std::size_t transition_group_count{};
  std::size_t new_component_group_count{};
  std::size_t continuation_group_count{};
  std::size_t coalescence_group_count{};

  friend bool operator==(
      const ExactGammaTransitionCounters&,
      const ExactGammaTransitionCounters&) = default;
};

struct ExactGammaTransitionResult {
  static constexpr const char* proof_basis =
      "exact_bounded_exhaustive_gamma_strict_to_closed_equal_level_"
      "simultaneous_transition_v1";

  ExactStrictGammaBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  exact::ExactLevel squared_level;
  std::size_t required_facet_count{};
  std::size_t required_coface_count{};
  std::size_t required_union_attempt_count{};
  ExactStrictGammaResult strict_gamma;
  std::vector<ExactGammaEqualLevelFacetWitness> equal_level_facets;
  std::vector<ExactStrictGammaCofaceWitness> equal_level_cofaces;
  std::vector<ExactGammaTransitionIncidence>
      equal_level_incidences;
  std::vector<ExactStrictGammaComponentWitness> closed_components;
  std::vector<std::size_t>
      strict_component_to_closed_component_index;
  std::vector<ExactGammaTransitionGroup> transition_groups;
  bool candidate_space_size_certified{false};
  bool strict_open_cut_fresh_replay_certified{false};
  bool equal_level_catalog_exhaustive_certified{false};
  bool equal_level_incidences_tokenized{false};
  bool closed_cut_exhaustive_certified{false};
  bool strict_partition_refines_closed_partition{false};
  bool equal_level_batch_applied_simultaneously{false};
  bool transition_groups_partition_equal_level_changes{false};
  ExactGammaTransitionCounters counters{};
  ExactGammaTransitionDecision decision{
      ExactGammaTransitionDecision::not_certified};
  ExactGammaTransitionScope scope{
      ExactGammaTransitionScope::unspecified};

  friend bool operator==(
      const ExactGammaTransitionResult&,
      const ExactGammaTransitionResult&) = default;
};

struct ExactGammaTransitionVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool preflight_counts_certified{false};
  bool strict_gamma_certified{false};
  bool equal_level_facets_certified{false};
  bool equal_level_cofaces_certified{false};
  bool equal_level_incidences_certified{false};
  bool closed_components_certified{false};
  bool strict_component_projection_certified{false};
  bool transition_groups_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_gamma_transition_decision_certified{false};
};

[[nodiscard]] ExactGammaTransitionResult
build_exact_gamma_equal_level_transition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget);

[[nodiscard]] ExactGammaTransitionVerification
verify_exact_gamma_equal_level_transition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget,
    const ExactGammaTransitionResult& result);

}  // namespace morsehgp3d::hierarchy
