#pragma once

#include "morsehgp3d/hierarchy/miniball.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

// The source decision is deliberately separate from the segment decision.
// An unsupported source is still an exact, freshly replayable classification;
// it simply does not enter the strict single-arm theorem.
enum class ExactCriticalArmSourceDecision : std::uint8_t {
  not_certified,
  critical_source_certified,
  unsupported_nonminimal_or_nonpositive_shell,
  unsupported_incomplete_global_shell,
  unsupported_closed_rank_or_order,
};

enum class ExactCriticalArmInitialSegmentDecision : std::uint8_t {
  not_certified,
  strict_initial_arm_segment_certified,
  no_segment_unsupported_critical_source,
};

enum class ExactCriticalArmInitialSegmentScope : std::uint8_t {
  unspecified,
  single_index_one_critical_arm_initial_germ_segment_only,
};

// For an exterior point p, A_p = ||c-p||^2-a is strictly positive and
// B_p = 2(c-p).d is the complete exact linear coefficient. Only B_p < 0
// produces a constraint. The deliberately conservative stored bound is
// A_p/(-2 B_p), as fixed by the initial-germ proof contract.
struct ExactCriticalArmExteriorConstraintWitness {
  spatial::PointId point_id{};
  exact::ExactLevel source_clearance_above_critical_level;
  exact::ExactRational source_outgoing_linear_coefficient;
  exact::ExactRational parameter_upper_bound;

  friend bool operator==(
      const ExactCriticalArmExteriorConstraintWitness&,
      const ExactCriticalArmExteriorConstraintWitness&) = default;
};

struct ExactCriticalArmInitialSegmentCounters {
  std::size_t critical_shell_miniball_build_count{};
  std::size_t critical_shell_support_check_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t global_closed_ball_distance_evaluation_count{};
  std::size_t global_shell_identity_check_count{};
  std::size_t closed_rank_order_check_count{};
  std::size_t arm_facet_construction_count{};
  std::size_t arm_miniball_build_count{};
  std::size_t arm_source_distance_evaluation_count{};
  std::size_t center_displacement_evaluation_count{};
  std::size_t exact_level_relation_count{};
  std::size_t removed_point_target_distance_evaluation_count{};
  std::size_t removed_point_directional_coefficient_evaluation_count{};
  std::size_t exterior_point_clearance_evaluation_count{};
  std::size_t exterior_point_directional_dot_product_evaluation_count{};
  std::size_t negative_exterior_direction_constraint_count{};
  std::size_t parameter_bound_candidate_count{};
  std::size_t parameter_bound_minimum_comparison_count{};
  std::size_t convex_identity_certification_count{};

  friend bool operator==(
      const ExactCriticalArmInitialSegmentCounters&,
      const ExactCriticalArmInitialSegmentCounters&) = default;
};

// This result starts one local index-one arm from a complete critical shell U
// and one removed u in U. It never builds a miniball of I union U: at closed
// rank eleven the only large object is the ten-point arm facet F_u. The
// ExactFacetDescentSegmentWitness is reused solely for the analytic
// coefficients of c -> c_F; no ExactFacetDescentSegmentResult is involved.
//
// Unsupported source decisions retain every source witness that was reached
// before the exact fail-closed classification. Once critical_source_certified
// is established, failure of any strict arm consequence is a logic error.
struct ExactCriticalArmInitialSegmentResult {
  static constexpr std::size_t minimum_critical_shell_point_count = 2U;
  static constexpr std::size_t maximum_critical_shell_point_count = 4U;
  static constexpr std::size_t maximum_supported_closed_rank = 11U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_complete_positive_critical_shell_removed_arm_fresh_miniball_"
      "outgoing_direction_local_exterior_bound_v1";

  std::vector<spatial::PointId> critical_shell_point_ids;
  spatial::PointId removed_shell_point_id{};
  ExactFacetMiniballResult critical_shell_miniball;
  std::optional<spatial::ClosedBallPartition> global_closed_ball;
  std::size_t closed_rank{};
  std::size_t order{};
  std::vector<spatial::PointId> arm_facet_point_ids;
  std::optional<ExactFacetMiniballResult> arm_miniball;
  std::optional<ExactFacetDescentSegmentWitness>
      initial_segment_coefficients;
  std::optional<exact::ExactLevel>
      removed_point_target_squared_distance;
  std::optional<exact::ExactRational>
      removed_point_outgoing_linear_coefficient;
  std::vector<ExactCriticalArmExteriorConstraintWitness>
      negative_exterior_direction_constraints;
  std::optional<exact::ExactRational>
      strict_local_parameter_upper_bound;
  bool critical_shell_is_positive_minimal_support{false};
  bool global_shell_matches_critical_shell{false};
  bool closed_rank_and_order_supported{false};
  bool critical_source_certified{false};
  bool arm_facet_cardinality_certified{false};
  bool arm_miniball_strict_decrease_certified{false};
  bool positive_center_displacement_certified{false};
  bool removed_point_outgoing_direction_certified{false};
  bool removed_point_target_outside_arm_ball_certified{false};
  bool exterior_prefix_bound_certified{false};
  bool closed_initial_segment_nonstrict_critical_sublevel{false};
  bool half_open_initial_segment_strict_critical_sublevel{false};
  ExactCriticalArmInitialSegmentCounters counters{};
  ExactCriticalArmSourceDecision source_decision{
      ExactCriticalArmSourceDecision::not_certified};
  ExactCriticalArmInitialSegmentDecision decision{
      ExactCriticalArmInitialSegmentDecision::not_certified};
  ExactCriticalArmInitialSegmentScope scope{
      ExactCriticalArmInitialSegmentScope::unspecified};
};

struct ExactCriticalArmInitialSegmentVerification {
  bool input_shell_identity_certified{false};
  bool removed_shell_point_identity_certified{false};
  bool critical_shell_miniball_certified{false};
  bool global_closed_ball_presence_certified{false};
  bool global_closed_ball_certified{false};
  bool source_facts_certified{false};
  bool source_decision_certified{false};
  bool arm_payload_presence_certified{false};
  bool arm_facet_certified{false};
  bool arm_miniball_certified{false};
  bool analytic_segment_coefficients_certified{false};
  bool removed_point_witnesses_certified{false};
  bool exterior_constraint_witnesses_certified{false};
  bool strict_local_parameter_bound_certified{false};
  bool strict_arm_consequences_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_arm_initial_segment_decision_certified{false};
};

enum class ExactCriticalArmDescentDecision : std::uint8_t {
  not_certified,
  no_descent_unsupported_critical_source,
  complete_at_regular_active_facet,
  certified_prefix_blocked_unsupported_degeneracy,
  certified_prefix_strict_segment_budget_exhausted,
};

enum class ExactCriticalArmDescentScope : std::uint8_t {
  unspecified,
  single_index_one_critical_arm_plus_canonical_strict_chain_only,
};

struct ExactCriticalArmDescentCounters {
  std::size_t initial_segment_probe_count{};
  std::size_t certified_initial_segment_count{};
  std::size_t facet_chain_build_count{};
  std::size_t initial_to_chain_seam_replay_count{};
  std::size_t committed_chain_strict_segment_count{};
  // Counts the initial segment plus committed 6.5 segments. A certified but
  // uncommitted stopping probe is deliberately outside this path count.
  std::size_t committed_composite_path_segment_count{};
  std::size_t source_unsupported_terminal_count{};
  std::size_t active_terminal_count{};
  std::size_t unsupported_chain_terminal_count{};
  std::size_t chain_budget_terminal_count{};

  friend bool operator==(
      const ExactCriticalArmDescentCounters&,
      const ExactCriticalArmDescentCounters&) = default;
};

// The 6.5 budget applies only after the dedicated initial critical-arm
// segment. A supported source always owns exactly one initial segment, even
// when maximum_committed_strict_segment_count is zero.
struct ExactCriticalArmDescentResult {
  static constexpr const char* proof_basis =
      "exact_critical_arm_initial_segment_exact_seam_single_source_"
      "canonical_facet_descent_chain_v1";

  ExactFacetDescentChainBudget requested_chain_budget{};
  ExactCriticalArmInitialSegmentResult initial_segment;
  std::optional<ExactFacetDescentChainResult> facet_descent_chain;
  bool initial_segment_excluded_from_chain_budget{false};
  bool exact_initial_to_chain_seam_certified{false};
  bool source_open_composite_path_strict_critical_sublevel{false};
  ExactCriticalArmDescentCounters counters{};
  ExactCriticalArmDescentDecision decision{
      ExactCriticalArmDescentDecision::not_certified};
  ExactCriticalArmDescentScope scope{
      ExactCriticalArmDescentScope::unspecified};
};

struct ExactCriticalArmDescentVerification {
  bool requested_chain_budget_certified{false};
  bool initial_segment_certified{false};
  bool facet_chain_presence_certified{false};
  bool facet_chain_certified{false};
  bool initial_segment_budget_separation_certified{false};
  bool exact_initial_to_chain_seam_certified{false};
  bool source_open_composite_path_certified{false};
  bool counters_certified{false};
  bool decision_mapping_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_arm_descent_decision_certified{false};
};

// Closing a family means enumerating every u in the complete critical shell
// and replaying one independent 6.6 descent for F_u. Equal terminal facet
// labels are grouped exactly, with the removed-point provenance retained.
// Distinct labels are deliberately not claimed to lie in distinct Gamma
// components: that requires a separate strict pre-event Gamma closure.
enum class ExactCriticalArmFamilyDecision : std::uint8_t {
  not_certified,
  no_family_unsupported_critical_source,
  all_arms_complete_at_regular_active_facets,
  incomplete_unsupported_degeneracy,
  incomplete_chain_budget_exhausted,
  incomplete_mixed_stops,
};

enum class ExactCriticalArmFamilyScope : std::uint8_t {
  unspecified,
  all_index_one_critical_arms_independent_canonical_strict_chains_terminal_labels_only,
};

struct ExactCriticalArmFamilyArmResult {
  spatial::PointId removed_shell_point_id{};
  ExactCriticalArmDescentResult descent;
  std::optional<ExactFacetDescentChainNodeWitness> active_terminal;
  std::optional<std::size_t> terminal_label_class_index;
};

struct ExactCriticalArmTerminalLabelClass {
  ExactFacetDescentChainNodeWitness canonical_terminal;
  std::vector<spatial::PointId> removed_shell_point_ids;

  friend bool operator==(
      const ExactCriticalArmTerminalLabelClass&,
      const ExactCriticalArmTerminalLabelClass&) = default;
};

struct ExactCriticalArmFamilyCounters {
  std::size_t requested_arm_count{};
  std::size_t arm_descent_build_count{};
  std::size_t shared_source_replay_comparison_count{};
  std::size_t complete_active_arm_count{};
  std::size_t unsupported_source_arm_count{};
  std::size_t unsupported_chain_arm_count{};
  std::size_t budget_exhausted_arm_count{};
  std::size_t terminal_label_pair_comparison_count{};
  std::size_t distinct_terminal_label_count{};
  std::size_t committed_chain_strict_segment_count{};
  std::size_t committed_composite_path_segment_count{};

  friend bool operator==(
      const ExactCriticalArmFamilyCounters&,
      const ExactCriticalArmFamilyCounters&) = default;
};

// Terminal classes may describe a certified partial observation when one or
// more arms stop early. They form the complete terminal-label partition only
// when complete_terminal_label_partition_certified is true. Even then they
// are identity classes of active terminal facets, never roots, attachments,
// or connected components of the strict Gamma cut.
struct ExactCriticalArmFamilyResult {
  static constexpr const char* proof_basis =
      "exact_all_index_one_critical_arms_independent_strict_chains_"
      "terminal_facet_label_identity_partition_v1";

  ExactFacetDescentChainBudget requested_per_arm_chain_budget{};
  std::vector<spatial::PointId> critical_shell_point_ids;
  std::vector<ExactCriticalArmFamilyArmResult> arms;
  std::vector<ExactCriticalArmTerminalLabelClass>
      terminal_label_classes;
  bool every_shell_point_enumerated_once{false};
  bool shared_critical_source_replay_certified{false};
  bool all_supported_composite_paths_strict_below_critical_level{false};
  bool terminal_labels_canonical{false};
  bool complete_terminal_label_partition_certified{false};
  ExactCriticalArmFamilyCounters counters{};
  ExactCriticalArmFamilyDecision decision{
      ExactCriticalArmFamilyDecision::not_certified};
  ExactCriticalArmFamilyScope scope{
      ExactCriticalArmFamilyScope::unspecified};
};

struct ExactCriticalArmFamilyVerification {
  bool requested_per_arm_chain_budget_certified{false};
  bool input_shell_identity_certified{false};
  bool every_shell_point_enumerated_once_certified{false};
  bool per_arm_descents_certified{false};
  bool active_terminals_certified{false};
  bool terminal_label_classes_certified{false};
  bool shared_critical_source_replay_certified{false};
  bool supported_composite_paths_certified{false};
  bool terminal_label_partition_status_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_arm_family_decision_certified{false};
};

// Reconstructs the exact critical sphere from U alone, closes its global
// shell, derives k=s-1 and F_u, then certifies the dedicated initial germ
// segment. The input shell must contain two to four distinct valid PointIds
// and removed_shell_point_id must belong to it; violations are API errors.
[[nodiscard]] ExactCriticalArmInitialSegmentResult
build_exact_critical_arm_initial_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    spatial::PointId removed_shell_point_id);

// Freshly reconstructs the source and every strict consequence from the
// cloud, U and u. No observed payload, decision or counter steers the replay.
[[nodiscard]] ExactCriticalArmInitialSegmentVerification
verify_exact_critical_arm_initial_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    spatial::PointId removed_shell_point_id,
    const ExactCriticalArmInitialSegmentResult& result);

// Optionally continues the certified initial germ with the existing 6.5
// single-source chain. The trusted budget excludes the initial segment.
[[nodiscard]] ExactCriticalArmDescentResult build_exact_critical_arm_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    spatial::PointId removed_shell_point_id,
    ExactFacetDescentChainBudget chain_budget);

// Replays both layers from the external inputs and trusted chain budget only.
[[nodiscard]] ExactCriticalArmDescentVerification
verify_exact_critical_arm_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    spatial::PointId removed_shell_point_id,
    ExactFacetDescentChainBudget chain_budget,
    const ExactCriticalArmDescentResult& result);

// Enumerates the complete canonical shell and independently replays 6.6 once
// per removed shell point under the same trusted per-arm chain budget.
[[nodiscard]] ExactCriticalArmFamilyResult
build_exact_critical_arm_family_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget);

// Reconstructs the whole family and its terminal-label identity partition
// from the external cloud, shell and common budget only.
[[nodiscard]] ExactCriticalArmFamilyVerification
verify_exact_critical_arm_family_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> critical_shell_point_ids,
    ExactFacetDescentChainBudget per_arm_chain_budget,
    const ExactCriticalArmFamilyResult& result);

}  // namespace morsehgp3d::hierarchy
