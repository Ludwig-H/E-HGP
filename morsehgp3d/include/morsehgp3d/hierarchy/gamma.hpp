#pragma once

#include "morsehgp3d/hierarchy/miniball.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactStrictGammaBudget {
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr std::size_t maximum_supported_source_facet_count = 4U;
  static constexpr std::size_t maximum_supported_facet_count = 3432U;
  static constexpr std::size_t maximum_supported_coface_count = 3432U;
  static constexpr std::size_t maximum_supported_union_attempt_count =
      21021U;

  std::size_t maximum_enumerated_facet_count{};
  std::size_t maximum_enumerated_coface_count{};
  std::size_t maximum_union_attempt_count{};

  friend bool operator==(
      const ExactStrictGammaBudget&,
      const ExactStrictGammaBudget&) = default;
};

enum class ExactStrictGammaDecision : std::uint8_t {
  not_certified,
  no_cut_preflight_budget_insufficient,
  complete_all_sources_active_and_classified,
  complete_with_inactive_sources,
};

enum class ExactStrictGammaScope : std::uint8_t {
  unspecified,
  bounded_exhaustive_strict_gamma_full_pi0_source_components_only,
};

struct ExactStrictGammaFacetWitness {
  std::vector<spatial::PointId> facet_point_ids;
  exact::ExactLevel squared_level;

  friend bool operator==(
      const ExactStrictGammaFacetWitness&,
      const ExactStrictGammaFacetWitness&) = default;
};

// At order ten the eleven-point coface level is the maximum of the levels of
// its ten-point deletion facets. The selected facet is the lexicographically
// first maximizer. Uniqueness of the minimum enclosing ball then forces its
// center to cover the omitted point as well.
struct ExactStrictGammaElevenPointWitness {
  std::vector<spatial::PointId> selected_deletion_facet_point_ids;
  spatial::PointId omitted_point_id{};
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;
  exact::ExactLevel omitted_point_squared_distance;
  bool selected_deletion_attains_maximum{false};
  bool selected_ball_covers_omitted_point{false};

  friend bool operator==(
      const ExactStrictGammaElevenPointWitness&,
      const ExactStrictGammaElevenPointWitness&) = default;
};

struct ExactStrictGammaCofaceWitness {
  std::vector<spatial::PointId> coface_point_ids;
  exact::ExactLevel squared_level;
  std::vector<std::vector<spatial::PointId>> facet_point_ids;
  std::optional<ExactStrictGammaElevenPointWitness>
      eleven_point_witness;

  friend bool operator==(
      const ExactStrictGammaCofaceWitness&,
      const ExactStrictGammaCofaceWitness&) = default;
};

struct ExactStrictGammaComponentWitness {
  std::vector<spatial::PointId> canonical_representative_facet_point_ids;
  std::vector<std::vector<spatial::PointId>> facet_point_ids;

  friend bool operator==(
      const ExactStrictGammaComponentWitness&,
      const ExactStrictGammaComponentWitness&) = default;
};

struct ExactStrictGammaSourceClassification {
  std::vector<spatial::PointId> source_facet_point_ids;
  exact::ExactLevel squared_level;
  bool active_strictly_below_cut{false};
  std::optional<std::size_t> component_index;

  friend bool operator==(
      const ExactStrictGammaSourceClassification&,
      const ExactStrictGammaSourceClassification&) = default;
};

struct ExactStrictGammaCounters {
  std::size_t preflight_count{};
  std::size_t required_facet_count{};
  std::size_t required_coface_count{};
  std::size_t required_union_attempt_count{};
  std::size_t enumerated_facet_count{};
  std::size_t facet_miniball_build_count{};
  std::size_t facet_strict_level_comparison_count{};
  std::size_t active_facet_count{};
  std::size_t enumerated_coface_count{};
  std::size_t direct_coface_miniball_build_count{};
  std::size_t eleven_point_coface_count{};
  std::size_t eleven_point_deletion_level_lookup_count{};
  std::size_t eleven_point_level_maximum_comparison_count{};
  std::size_t eleven_point_omitted_point_distance_evaluation_count{};
  std::size_t coface_strict_level_comparison_count{};
  std::size_t active_coface_count{};
  std::size_t coface_facet_lookup_count{};
  std::size_t disjoint_set_value_count{};
  std::size_t union_attempt_count{};
  std::size_t union_merge_count{};
  std::size_t component_count{};
  std::size_t isolated_component_count{};
  std::size_t source_lookup_count{};
  std::size_t active_source_count{};
  std::size_t inactive_source_count{};

  friend bool operator==(
      const ExactStrictGammaCounters&,
      const ExactStrictGammaCounters&) = default;
};

// The result is the complete full-pi0 Gamma cut for a bounded cloud and the
// component lookup of up to four caller-provided source facets. Components
// contain facets, not covered-point unions. No component index is a hierarchy
// root, Morse attachment, forest node, or public status.
struct ExactStrictGammaResult {
  static constexpr const char* proof_basis =
      "exact_bounded_exhaustive_strict_gamma_full_pi0_source_"
      "component_classification_v1";

  ExactStrictGammaBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  exact::ExactLevel strict_cut_squared_level;
  std::vector<std::vector<spatial::PointId>> source_facet_point_ids;
  std::size_t required_facet_count{};
  std::size_t required_coface_count{};
  std::size_t required_union_attempt_count{};
  std::vector<ExactStrictGammaFacetWitness> active_facets;
  std::vector<ExactStrictGammaCofaceWitness> active_cofaces;
  std::vector<ExactStrictGammaComponentWitness> components;
  std::vector<ExactStrictGammaSourceClassification>
      source_classifications;
  bool candidate_space_size_certified{false};
  bool strict_open_cut_certified{false};
  bool full_pi0_isolated_facets_included{false};
  bool exhaustive_active_catalog_certified{false};
  bool complete_source_classification_certified{false};
  bool all_sources_active_and_classified{false};
  ExactStrictGammaCounters counters{};
  ExactStrictGammaDecision decision{
      ExactStrictGammaDecision::not_certified};
  ExactStrictGammaScope scope{ExactStrictGammaScope::unspecified};

  friend bool operator==(
      const ExactStrictGammaResult&,
      const ExactStrictGammaResult&) = default;
};

struct ExactStrictGammaVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool preflight_counts_certified{false};
  bool active_facets_certified{false};
  bool active_cofaces_certified{false};
  bool components_certified{false};
  bool source_classifications_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_strict_gamma_decision_certified{false};
};

[[nodiscard]] ExactStrictGammaResult
build_exact_strict_gamma_source_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& strict_cut_squared_level,
    std::span<const std::vector<spatial::PointId>>
        canonical_source_facets,
    ExactStrictGammaBudget budget);

[[nodiscard]] ExactStrictGammaVerification
verify_exact_strict_gamma_source_classification(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const exact::ExactLevel& strict_cut_squared_level,
    std::span<const std::vector<spatial::PointId>>
        canonical_source_facets,
    ExactStrictGammaBudget budget,
    const ExactStrictGammaResult& result);

}  // namespace morsehgp3d::hierarchy
