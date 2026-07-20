#pragma once

#include "morsehgp3d/hierarchy/gamma_transition.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// For 2 <= order < point_count <= 14 and order <= 10, hgp_reduced keeps
// exactly the nontrivial components of exhaustive Gamma.  This batch is a
// local, single-level semantic projection of ExactGammaTransitionResult.  It
// does not allocate persistent root identifiers and is not a hierarchy, DAG,
// forest, vertical map, M.1 certificate, or public-status decision.  The
// terminal order == point_count case has no equal-level 6.10 transition and
// is deliberately outside this contract.
enum class ExactReducedGammaBatchDecision : std::uint8_t {
  not_certified,
  no_batch_preflight_budget_insufficient,
  complete_exhaustive_reduced_gamma_batch,
};

enum class ExactReducedGammaBatchScope : std::uint8_t {
  unspecified,
  bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only,
};

enum class ExactReducedGammaStrictComponentKind : std::uint8_t {
  omitted_isolated_facet,
  prior_nontrivial_reduced_root,
};

enum class ExactReducedGammaBatchGroupKind : std::uint8_t {
  deferred_isolated_facet,
  birth,
  continuation,
  multifusion,
};

struct ExactReducedGammaStrictComponentClassification {
  std::size_t strict_component_index{};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  std::size_t facet_count{};
  std::vector<std::size_t> incident_strict_coface_indices;
  bool incident_to_strict_coface{false};
  bool facet_count_is_nontrivial{false};
  bool incidence_nontriviality_equivalence_certified{false};
  bool carries_prior_reduced_root{false};
  ExactReducedGammaStrictComponentKind kind{
      ExactReducedGammaStrictComponentKind::omitted_isolated_facet};

  friend bool operator==(
      const ExactReducedGammaStrictComponentClassification&,
      const ExactReducedGammaStrictComponentClassification&) = default;
};

struct ExactReducedGammaCoverageDelta {
  std::vector<std::vector<spatial::PointId>>
      added_facet_point_ids;
  std::vector<spatial::PointId> added_point_ids;
  bool fully_redundant{false};

  friend bool operator==(
      const ExactReducedGammaCoverageDelta&,
      const ExactReducedGammaCoverageDelta&) = default;
};

struct ExactReducedGammaBatchGroup {
  std::size_t transition_group_index{};
  std::size_t closed_component_index{};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  std::vector<std::size_t> strict_component_indices;
  std::vector<std::size_t>
      prior_reduced_root_strict_component_indices;
  std::vector<std::size_t>
      absorbed_isolated_strict_component_indices;
  std::vector<std::vector<spatial::PointId>>
      newly_active_facet_point_ids;
  std::vector<std::vector<spatial::PointId>>
      equal_level_coface_point_ids;
  std::optional<ExactReducedGammaCoverageDelta> coverage_delta;
  ExactReducedGammaBatchGroupKind kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};

  friend bool operator==(
      const ExactReducedGammaBatchGroup&,
      const ExactReducedGammaBatchGroup&) = default;
};

struct ExactReducedGammaBatchCounters {
  std::size_t gamma_transition_build_count{};
  std::size_t strict_component_classification_count{};
  std::size_t strict_component_facet_count_check_count{};
  std::size_t strict_coface_incidence_scan_count{};
  std::size_t strict_coface_facet_lookup_count{};
  std::size_t prior_reduced_root_count{};
  std::size_t omitted_isolated_strict_component_count{};
  std::size_t transition_group_classification_count{};
  std::size_t transition_group_strict_component_reference_count{};
  std::size_t prior_reduced_root_reference_count{};
  std::size_t absorbed_isolated_reference_count{};
  std::size_t deferred_isolated_facet_group_count{};
  std::size_t birth_group_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t closed_component_facet_scan_count{};
  std::size_t prior_root_facet_scan_count{};
  std::size_t coverage_delta_count{};
  std::size_t fully_redundant_coverage_delta_count{};

  friend bool operator==(
      const ExactReducedGammaBatchCounters&,
      const ExactReducedGammaBatchCounters&) = default;
};

struct ExactReducedGammaBatchResult {
  static constexpr const char* proof_basis =
      "exact_bounded_exhaustive_gamma_strict_nontrivial_component_"
      "reduction_and_equal_level_batch_semantics_v1";

  ExactStrictGammaBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  exact::ExactLevel squared_level;
  ExactGammaTransitionResult gamma_transition;
  std::vector<ExactReducedGammaStrictComponentClassification>
      strict_component_classifications;
  std::vector<ExactReducedGammaBatchGroup> groups;
  bool gamma_transition_fresh_replay_certified{false};
  bool strict_components_exhaustively_classified{false};
  bool strict_component_incidence_nontriviality_equivalence_certified{
      false};
  bool strict_reduced_roots_exactly_nontrivial_components{false};
  bool transition_groups_exhaustively_classified{false};
  bool strict_components_partitioned_within_groups{false};
  bool isolated_facets_deferred_without_reduced_root{false};
  bool equal_level_coface_groups_use_reduced_root_count{false};
  bool coverage_deltas_are_exact_set_differences{false};
  bool equal_level_batch_semantics_certified{false};
  ExactReducedGammaBatchCounters counters{};
  ExactReducedGammaBatchDecision decision{
      ExactReducedGammaBatchDecision::not_certified};
  ExactReducedGammaBatchScope scope{
      ExactReducedGammaBatchScope::unspecified};

  friend bool operator==(
      const ExactReducedGammaBatchResult&,
      const ExactReducedGammaBatchResult&) = default;
};

struct ExactReducedGammaBatchVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool gamma_transition_certified{false};
  bool strict_component_classifications_certified{false};
  bool groups_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_reduced_gamma_batch_decision_certified{false};
};

[[nodiscard]] ExactReducedGammaBatchResult
build_exact_reduced_gamma_batch(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget);

[[nodiscard]] ExactReducedGammaBatchVerification
verify_exact_reduced_gamma_batch(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    ExactStrictGammaBudget budget,
    const ExactReducedGammaBatchResult& result);

}  // namespace morsehgp3d::hierarchy
