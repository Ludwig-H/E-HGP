#pragma once

#include "morsehgp3d/exact/level.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t relative_morse_boruvka_schema_version = 1U;
inline constexpr std::string_view relative_morse_boruvka_backend =
    "reference_cpu";
inline constexpr std::string_view relative_morse_boruvka_profile =
    "hgp_reduced";
inline constexpr std::string_view relative_morse_boruvka_mode =
    "certified_relative_morse_boruvka";
inline constexpr std::string_view relative_morse_boruvka_deployment_status =
    "architecture_only";
inline constexpr std::string_view relative_morse_boruvka_public_status =
    "not_claimed";
inline constexpr std::string_view relative_morse_boruvka_proof_basis =
    "canonical_star_graph_one_exact_cut_min_link_and_saddle_provenance_sandwich_v1";

// Dense identifiers are local to one already-constructed Morse authority.
// This kernel validates their combinatorial shape but cannot establish that
// the caller supplied every geometric minimum, saddle or strict arm.
struct ExactRelativeMorseBoruvkaBirth {
  std::size_t birth_index{};
  exact::ExactLevel squared_level{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaBirth&,
      const ExactRelativeMorseBoruvkaBirth&) = default;
};

// Arm multiplicity is retained in the input terminal_birth_indices arena.
// Repeated terminal births do not alter connectivity, but silently
// deduplicating them at the API boundary would lose evidence needed by later
// Morse obligations.  The CSR slice contains between one and four arms.
struct ExactRelativeMorseBoruvkaSaddle {
  std::size_t saddle_index{};
  exact::ExactLevel squared_level{};
  std::size_t terminal_birth_reference_offset{};
  std::size_t terminal_birth_reference_count{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaSaddle&,
      const ExactRelativeMorseBoruvkaSaddle&) = default;
};

struct ExactRelativeMorseBoruvkaInput {
  std::size_t point_count{};
  std::size_t order{};
  std::vector<ExactRelativeMorseBoruvkaBirth> births;
  std::vector<ExactRelativeMorseBoruvkaSaddle> saddles;
  std::vector<std::size_t> terminal_birth_indices;

  friend bool operator==(
      const ExactRelativeMorseBoruvkaInput&,
      const ExactRelativeMorseBoruvkaInput&) = default;
};

// Every saddle is expanded deterministically into a star over its distinct
// terminal births: the smallest birth is the anchor and the remaining births
// are spokes in increasing order.  The expansion is connectivity-equivalent
// to the saddle at every exact threshold, without copying its ExactLevel.
struct ExactRelativeMorseBoruvkaLink {
  std::size_t link_index{};
  std::size_t saddle_index{};
  std::size_t anchor_birth_index{};
  std::size_t terminal_birth_index{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaLink&,
      const ExactRelativeMorseBoruvkaLink&) = default;
};

struct ExactRelativeMorseBoruvkaBudget {
  std::size_t maximum_birth_count{};
  std::size_t maximum_saddle_count{};
  std::size_t maximum_terminal_birth_reference_count{};
  std::size_t maximum_canonical_link_count{};
  std::size_t maximum_round_count{};
  std::size_t maximum_root_choice_count{};
  std::size_t maximum_selected_link_count{};
  std::size_t maximum_retained_saddle_count{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaBudget&,
      const ExactRelativeMorseBoruvkaBudget&) = default;
};

struct ExactRelativeMorseBoruvkaRootChoice {
  std::size_t pre_round_root_birth_index{};
  // The count is certified only by a fresh rescan of the caller input.  The
  // selected link is the smallest link_index among all links at the exact
  // minimum level; its saddle references that level without copying it here.
  std::size_t exact_cominimizer_link_count{};
  std::size_t selected_canonical_link_index{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaRootChoice&,
      const ExactRelativeMorseBoruvkaRootChoice&) = default;
};

struct ExactRelativeMorseBoruvkaSelectedLink {
  std::size_t link_index{};
  std::size_t saddle_index{};
  std::size_t pre_round_left_root_birth_index{};
  std::size_t pre_round_right_root_birth_index{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaSelectedLink&,
      const ExactRelativeMorseBoruvkaSelectedLink&) = default;
};

struct ExactRelativeMorseBoruvkaRound {
  std::size_t round_index{};
  std::size_t pre_round_component_count{};
  std::size_t root_choice_offset{};
  std::size_t root_choice_count{};
  std::size_t selected_link_offset{};
  std::size_t selected_link_count{};
  std::size_t post_round_component_count{};
  bool roots_frozen_before_all_choices{false};
  bool every_choice_scans_all_exact_cominimizers{false};
  bool exactly_one_canonical_link_selected_per_root{false};
  bool quotient_committed_after_all_choices{false};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaRound&,
      const ExactRelativeMorseBoruvkaRound&) = default;
};

struct ExactRelativeMorseBoruvkaRequirements {
  std::size_t required_birth_capacity{};
  std::size_t required_saddle_capacity{};
  std::size_t required_terminal_birth_reference_capacity{};
  std::size_t required_canonical_link_capacity{};
  std::size_t required_round_capacity{};
  std::size_t required_root_choice_capacity{};
  std::size_t required_selected_link_capacity{};
  std::size_t required_retained_saddle_capacity{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaRequirements&,
      const ExactRelativeMorseBoruvkaRequirements&) = default;
};

struct ExactRelativeMorseBoruvkaAudit {
  std::size_t input_validation_pass_count{};
  std::size_t canonical_terminal_reference_authority_count{};
  std::size_t canonical_link_count{};
  std::size_t full_expansion_quotient_build_count{};
  std::size_t boruvka_round_count{};
  std::size_t boruvka_link_scan_count{};
  std::size_t exact_cominimizer_link_observation_count{};
  std::size_t root_choice_count{};
  std::size_t selected_link_count{};
  std::size_t selected_saddle_provenance_count{};
  std::size_t retained_saddle_count{};
  std::size_t frozen_quotient_build_count{};
  std::size_t initial_component_count{};
  std::size_t final_component_count{};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaAudit&,
      const ExactRelativeMorseBoruvkaAudit&) = default;
};

enum class ExactRelativeMorseBoruvkaDecision : std::uint8_t {
  not_certified,
  no_sparsification_capacity_overflow,
  no_sparsification_input_shape_rejected,
  no_sparsification_preflight_budget_insufficient,
  complete_relative_partition_sparsifier,
};

enum class ExactRelativeMorseBoruvkaScope : std::uint8_t {
  unspecified,
  exact_partition_sparsification_relative_to_one_caller_supplied_weighted_morse_hypergraph_only,
};

struct ExactRelativeMorseBoruvkaResult {
  static constexpr std::string_view backend = relative_morse_boruvka_backend;
  static constexpr std::string_view profile = relative_morse_boruvka_profile;
  static constexpr std::string_view mode = relative_morse_boruvka_mode;
  static constexpr std::string_view deployment_status =
      relative_morse_boruvka_deployment_status;
  static constexpr std::string_view public_status =
      relative_morse_boruvka_public_status;
  static constexpr std::string_view proof_basis =
      relative_morse_boruvka_proof_basis;

  std::uint32_t schema_version{relative_morse_boruvka_schema_version};
  ExactRelativeMorseBoruvkaBudget requested_budget{};
  ExactRelativeMorseBoruvkaRequirements requirements{};
  std::size_t point_count{};
  std::size_t order{};
  std::vector<ExactRelativeMorseBoruvkaRound> rounds;
  std::vector<ExactRelativeMorseBoruvkaLink> canonical_links;
  std::vector<ExactRelativeMorseBoruvkaRootChoice> root_choices;
  std::vector<ExactRelativeMorseBoruvkaSelectedLink> selected_links;
  std::vector<std::size_t> retained_saddle_indices;
  std::vector<std::size_t> final_component_representative_birth_indices;
  ExactRelativeMorseBoruvkaAudit audit{};
  bool input_shape_certified{false};
  bool budget_preflight_certified{false};
  bool canonical_star_expansion_certified{false};
  bool roots_frozen_in_every_round{false};
  bool all_exact_cominimizers_scanned_for_every_root_cut{false};
  bool exactly_one_canonical_link_selected_per_root{false};
  bool selected_links_are_a_canonical_expansion_subset{false};
  bool frozen_selected_links_contracted_atomically{false};
  bool retained_saddles_form_a_canonical_weighted_partition_basis{false};
  bool all_input_threshold_partitions_preserved_relative_to_input{false};
  bool caller_catalog_completeness_certified{false};
  bool silent_incidence_coverage_certified{false};
  bool m1_certified{false};
  bool hierarchy_or_forest_constructed{false};
  bool public_status_claimed{false};
  bool diagnostic_outcomes_have_no_payload{false};
  ExactRelativeMorseBoruvkaDecision decision{
      ExactRelativeMorseBoruvkaDecision::not_certified};
  ExactRelativeMorseBoruvkaScope scope{
      ExactRelativeMorseBoruvkaScope::unspecified};

  [[nodiscard]] bool certified_relative_partition_sparsifier()
      const noexcept;

  friend bool operator==(
      const ExactRelativeMorseBoruvkaResult&,
      const ExactRelativeMorseBoruvkaResult&) = default;
};

struct ExactRelativeMorseBoruvkaVerification {
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool rounds_certified{false};
  bool selected_links_and_saddles_certified{false};
  bool final_components_certified{false};
  bool audit_certified{false};
  bool result_facts_certified{false};
  bool decision_and_scope_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactRelativeMorseBoruvkaVerification&,
      const ExactRelativeMorseBoruvkaVerification&) = default;
};

[[nodiscard]] ExactRelativeMorseBoruvkaResult
build_exact_relative_morse_boruvka(
    const ExactRelativeMorseBoruvkaInput& input,
    const ExactRelativeMorseBoruvkaBudget& budget);

// Rebuilds the complete relative transcript from the same caller authority.
// It recertifies no omitted geometry, silent incidence or M.1 obligation.
[[nodiscard]] ExactRelativeMorseBoruvkaVerification
verify_exact_relative_morse_boruvka(
    const ExactRelativeMorseBoruvkaInput& input,
    const ExactRelativeMorseBoruvkaBudget& budget,
    const ExactRelativeMorseBoruvkaResult& observed);

}  // namespace morsehgp3d::hierarchy
