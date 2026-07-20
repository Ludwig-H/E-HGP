#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"
#include "morsehgp3d/spatial/query.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

enum class ExactFacetMiniballStatus : std::uint8_t {
  not_certified,
  exact_facet_miniball_certified,
};

enum class ExactFacetMiniballScope : std::uint8_t {
  unspecified,
  local_facet_miniball_only,
};

struct ExactFacetMiniballCounters {
  std::size_t facet_point_count{};
  std::array<std::size_t, 4> enumerated_support_count_by_size{};
  std::size_t enumerated_support_count{};
  std::size_t affinely_dependent_support_count{};
  std::size_t boundary_reduced_support_count{};
  std::size_t exterior_circumcenter_support_count{};
  std::size_t minimal_support_candidate_count{};
  std::size_t candidate_point_classification_count{};
  std::size_t candidate_strictly_inside_classification_count{};
  std::size_t candidate_boundary_classification_count{};
  std::size_t candidate_outside_classification_count{};
  std::size_t enclosing_support_count{};
  std::size_t optimal_support_count{};
  std::size_t selected_support_size{};

  friend bool operator==(
      const ExactFacetMiniballCounters&,
      const ExactFacetMiniballCounters&) = default;
};

// This result certifies one bounded local facet only. The enclosing ball is
// unique, but its exact support need not be. Among all positive supports of
// minimum radius, the adapter chooses minimum cardinality and then the
// lexicographically smallest PointId vector. That deterministic choice is not
// a claim that the support is the unique essential support of a later descent.
struct ExactFacetMiniballResult {
  static constexpr std::size_t maximum_facet_point_count = 10U;
  static constexpr std::size_t maximum_support_point_count = 4U;
  static constexpr std::size_t maximum_enumerated_support_count = 385U;
  static constexpr const char* proof_basis =
      "exhaustive_exact_supports_up_to_four_facet_miniball_v1";

  std::vector<spatial::PointId> facet_point_ids;
  std::vector<spatial::PointId> support_point_ids;
  std::vector<spatial::PointId> strictly_inside_point_ids;
  std::vector<spatial::PointId> boundary_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_radius;
  ExactFacetMiniballCounters counters{};
  ExactFacetMiniballStatus status{
      ExactFacetMiniballStatus::not_certified};
  ExactFacetMiniballScope scope{ExactFacetMiniballScope::unspecified};
};

struct ExactFacetMiniballVerification {
  bool facet_identity_certified{false};
  bool exhaustive_support_enumeration_certified{false};
  bool exact_center_and_radius_certified{false};
  bool enclosing_partition_certified{false};
  bool canonical_support_certified{false};
  bool counters_certified{false};
  bool status_certified{false};
  bool local_scope_certified{false};
  bool fresh_replay_certified{false};
  bool local_exact_facet_miniball_certified{false};
};

enum class ExactFacetDescentPreconditionDecision : std::uint8_t {
  not_certified,
  strict_descent_admissible,
  already_active_at_own_center,
  unsupported_degeneracy,
};

enum class ExactFacetDescentPreconditionScope : std::uint8_t {
  unspecified,
  global_shell_and_top_k_preconditions_only,
};

struct ExactFacetDescentPreconditionCounters {
  std::size_t global_closed_ball_query_count{};
  std::size_t global_closed_ball_distance_evaluation_count{};
  std::size_t exact_top_k_query_count{};
  std::size_t exact_top_k_distance_evaluation_count{};
  std::size_t total_exact_point_distance_evaluation_count{};

  friend bool operator==(
      const ExactFacetDescentPreconditionCounters&,
      const ExactFacetDescentPreconditionCounters&) = default;
};

// This bounded reference result classifies only the exact preconditions for
// one facet. It neither constructs a successor nor certifies a descent arc,
// an attachment, a hierarchy reduction, or any public status.
struct ExactFacetDescentPreconditionResult {
  static constexpr const char* proof_basis =
      "exact_facet_miniball_global_closed_ball_exact_top_k_membership_v1";

  ExactFacetMiniballResult facet_miniball;
  std::optional<spatial::ClosedBallPartition> global_closed_ball;
  std::optional<spatial::TopKPartition> exact_top_k;
  bool local_boundary_equals_support{false};
  bool global_shell_equals_local_boundary{false};
  bool facet_is_exact_top_k_member{false};
  ExactFacetDescentPreconditionCounters counters{};
  ExactFacetDescentPreconditionDecision decision{
      ExactFacetDescentPreconditionDecision::not_certified};
  ExactFacetDescentPreconditionScope scope{
      ExactFacetDescentPreconditionScope::unspecified};
};

struct ExactFacetDescentPreconditionVerification {
  bool facet_miniball_certified{false};
  bool global_closed_ball_identity_certified{false};
  bool global_closed_ball_partition_certified{false};
  bool exact_top_k_identity_certified{false};
  bool exact_top_k_partition_certified{false};
  bool top_k_cutoff_bound_certified{false};
  bool local_boundary_decision_certified{false};
  bool global_shell_decision_certified{false};
  bool facet_top_k_membership_decision_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_descent_preconditions_certified{false};
};

enum class ExactFacetDescentArcDecision : std::uint8_t {
  not_certified,
  strict_descent_arc_certified,
  no_arc_already_active_at_own_center,
  no_arc_unsupported_degeneracy,
};

enum class ExactFacetDescentArcScope : std::uint8_t {
  unspecified,
  canonical_top_k_selected_strict_level_arc_only,
};

struct ExactFacetDescentArcCounters {
  std::size_t precondition_classification_count{};
  std::size_t canonical_top_k_selection_count{};
  std::size_t successor_miniball_build_count{};
  std::size_t exact_level_comparison_count{};

  friend bool operator==(
      const ExactFacetDescentArcCounters&,
      const ExactFacetDescentArcCounters&) = default;
};

// A strict result selects exactly the canonical member already exposed by the
// complete top-k partition, then freshly constructs and compares its exact
// miniball. Non-strict branches carry no successor payload. This result does
// not certify a geometric segment, a DAG, an attachment, or any public status.
struct ExactFacetDescentArcResult {
  static constexpr const char* proof_basis =
      "exact_descent_preconditions_canonical_top_k_member_fresh_miniball_strict_level_v1";

  ExactFacetDescentPreconditionResult source_preconditions;
  std::optional<std::vector<spatial::PointId>>
      successor_facet_point_ids;
  std::optional<ExactFacetMiniballResult> successor_miniball;
  bool successor_is_canonical_top_k_choice{false};
  bool successor_is_exact_top_k_member{false};
  bool successor_differs_from_source{false};
  bool successor_level_within_top_k_cutoff{false};
  bool strict_level_decrease{false};
  ExactFacetDescentArcCounters counters{};
  ExactFacetDescentArcDecision decision{
      ExactFacetDescentArcDecision::not_certified};
  ExactFacetDescentArcScope scope{
      ExactFacetDescentArcScope::unspecified};
};

struct ExactFacetDescentArcVerification {
  bool source_preconditions_certified{false};
  bool successor_payload_presence_certified{false};
  bool successor_facet_certified{false};
  bool successor_miniball_certified{false};
  bool successor_is_canonical_top_k_choice_certified{false};
  bool successor_is_exact_top_k_member_certified{false};
  bool successor_differs_from_source_certified{false};
  bool successor_level_within_top_k_cutoff_certified{false};
  bool strict_level_decrease_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_descent_arc_decision_certified{false};
};

enum class ExactFacetDescentSegmentDecision : std::uint8_t {
  not_certified,
  strict_half_open_segment_certified,
  no_segment_already_active_at_own_center,
  no_segment_unsupported_degeneracy,
};

enum class ExactFacetDescentSegmentScope : std::uint8_t {
  unspecified,
  canonical_strict_arc_half_open_sublevel_segment_only,
};

struct ExactFacetDescentSegmentWitness {
  exact::ExactLevel source_atom_level;
  exact::ExactLevel successor_atom_level;
  exact::ExactLevel center_squared_displacement;
  bool centers_equal{false};
  bool source_endpoint_strict_sublevel{false};
  bool quadratic_max_upper_bound_certified{false};
  bool closed_segment_nonstrict_sublevel{false};
  bool half_open_segment_strict_sublevel{false};

  friend bool operator==(
      const ExactFacetDescentSegmentWitness&,
      const ExactFacetDescentSegmentWitness&) = default;
};

struct ExactFacetDescentSegmentCounters {
  std::size_t source_arc_classification_count{};
  std::size_t source_atom_distance_evaluation_count{};
  std::size_t source_atom_maximum_comparison_count{};
  std::size_t center_displacement_evaluation_count{};
  std::size_t exact_level_relation_count{};
  std::size_t convex_identity_certification_count{};

  friend bool operator==(
      const ExactFacetDescentSegmentCounters&,
      const ExactFacetDescentSegmentCounters&) = default;
};

// The optional witness certifies the source-open, target-closed parameter
// range 0 < t <= 1 for one already-certified canonical strict arc. It carries
// no claim about a chain, DAG, attachment, hierarchy, or public status.
struct ExactFacetDescentSegmentResult {
  static constexpr const char* proof_basis =
      "exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1";

  ExactFacetDescentArcResult source_arc;
  std::optional<ExactFacetDescentSegmentWitness> segment_witness;
  ExactFacetDescentSegmentCounters counters{};
  ExactFacetDescentSegmentDecision decision{
      ExactFacetDescentSegmentDecision::not_certified};
  ExactFacetDescentSegmentScope scope{
      ExactFacetDescentSegmentScope::unspecified};
};

struct ExactFacetDescentSegmentVerification {
  bool source_arc_certified{false};
  bool segment_witness_presence_certified{false};
  bool source_atom_level_certified{false};
  bool successor_atom_level_certified{false};
  bool center_squared_displacement_certified{false};
  bool centers_equal_certified{false};
  bool source_endpoint_strict_sublevel_certified{false};
  bool quadratic_max_upper_bound_certified{false};
  bool closed_segment_nonstrict_sublevel_certified{false};
  bool half_open_segment_strict_sublevel_certified{false};
  bool exact_level_relations_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_descent_segment_decision_certified{false};
};

struct ExactFacetDescentChainBudget {
  static constexpr std::size_t
      maximum_supported_committed_strict_segment_count =
      4096U;

  std::size_t maximum_committed_strict_segment_count{};

  friend bool operator==(
      const ExactFacetDescentChainBudget&,
      const ExactFacetDescentChainBudget&) = default;
};

enum class ExactFacetDescentChainDecision : std::uint8_t {
  not_certified,
  complete_at_regular_active_facet,
  certified_prefix_blocked_unsupported_degeneracy,
  certified_prefix_strict_segment_budget_exhausted,
};

enum class ExactFacetDescentChainScope : std::uint8_t {
  unspecified,
  single_source_canonical_strict_descent_chain_only,
};

struct ExactFacetDescentChainNodeWitness {
  std::vector<spatial::PointId> facet_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;

  friend bool operator==(
      const ExactFacetDescentChainNodeWitness&,
      const ExactFacetDescentChainNodeWitness&) = default;
};

struct ExactFacetDescentChainCounters {
  std::size_t facet_probe_count{};
  std::size_t strict_segment_probe_count{};
  std::size_t committed_strict_segment_count{};
  std::size_t visited_facet_count{};
  std::size_t successor_cycle_lookup_count{};
  std::size_t inter_step_seam_replay_count{};
  std::size_t active_terminal_count{};
  std::size_t unsupported_terminal_count{};
  std::size_t structural_budget_stop_count{};
  ExactFacetDescentSegmentCounters accumulated_probe_counters{};

  friend bool operator==(
      const ExactFacetDescentChainCounters&,
      const ExactFacetDescentChainCounters&) = default;
};

// The compact nodes and aligned 6.4 witnesses certify one deterministic
// source orbit only. A single complete stopping probe retains the point-cloud
// identity and the exact reason for stopping without retaining O(L n) spatial
// partitions. Unsupported and budgeted outcomes certify only their strict
// prefix; none of the outcomes certifies a DAG, germ, attachment, hierarchy,
// or public status.
struct ExactFacetDescentChainResult {
  static constexpr const char* proof_basis =
      "exact_replayed_half_open_segments_exact_seams_strict_facet_"
      "potential_finite_orbit_v1";

  ExactFacetDescentChainBudget requested_budget{};
  std::size_t effective_maximum_committed_strict_segment_count{};
  std::vector<ExactFacetDescentChainNodeWitness> nodes;
  std::vector<ExactFacetDescentSegmentWitness>
      committed_segment_witnesses;
  std::optional<ExactFacetDescentSegmentResult> stopping_probe;
  bool exact_seams_certified{false};
  bool strict_facet_potential_certified{false};
  bool finite_strict_facet_orbit_theorem_certified{false};
  bool closed_polyline_nonstrict_initial_sublevel{false};
  bool source_open_polyline_strict_initial_sublevel{false};
  ExactFacetDescentChainCounters counters{};
  ExactFacetDescentChainDecision decision{
      ExactFacetDescentChainDecision::not_certified};
  ExactFacetDescentChainScope scope{
      ExactFacetDescentChainScope::unspecified};
};

struct ExactFacetDescentChainVerification {
  bool requested_budget_certified{false};
  bool effective_budget_certified{false};
  bool compact_path_shape_certified{false};
  bool initial_facet_identity_certified{false};
  bool compact_nodes_certified{false};
  bool committed_segment_witnesses_certified{false};
  bool stopping_probe_presence_certified{false};
  bool stopping_probe_certified{false};
  bool exact_seams_certified{false};
  bool strict_facet_potential_certified{false};
  bool finite_strict_facet_orbit_theorem_certified{false};
  bool closed_polyline_nonstrict_initial_sublevel_certified{false};
  bool source_open_polyline_strict_initial_sublevel_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_descent_chain_decision_certified{false};
};

// Enumerates every subset of one to four facet points. For a ten-point facet,
// this is exactly 385 supports. Every relatively interior circumcenter is
// classified against every facet point with exact rationals before the least
// enclosing radius and canonical support are selected.
[[nodiscard]] ExactFacetMiniballResult build_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids);

// Repeats the complete bounded enumeration without trusting any result field.
// It is a fresh execution of the same proved exhaustive algorithm, not an
// independent software oracle and not a full_pi0 public-status certificate.
[[nodiscard]] ExactFacetMiniballVerification verify_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    const ExactFacetMiniballResult& result);

// Reuses the exact local miniball, then classifies its complete global closed
// ball and the exact rank-|F| partition at its center. The top-k query is over
// the whole cloud with an explicitly empty exclusion set.
[[nodiscard]] ExactFacetDescentPreconditionResult
build_exact_facet_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids);

// Recomputes the local miniball and both complete spatial partitions from the
// input cloud and facet without trusting any field of the observed result.
[[nodiscard]] ExactFacetDescentPreconditionVerification
verify_exact_facet_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    const ExactFacetDescentPreconditionResult& result);

// Selects and certifies one canonical strict-level arc only when the embedded
// 6.2 preconditions are strictly admissible. Active and unsupported facets
// produce exact no-arc decisions with empty successor optionals.
[[nodiscard]] ExactFacetDescentArcResult build_exact_facet_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids);

// Replays 6.2 and derives the expected canonical successor from the fresh
// top-k partition before checking the observed successor and its miniball.
[[nodiscard]] ExactFacetDescentArcVerification
verify_exact_facet_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    const ExactFacetDescentArcResult& result);

// Certifies the exact squared-distance chord identity and its maximum-envelope
// bound for the half-open segment of a strict 6.3 arc. Non-arc branches retain
// an exact decision but carry no segment witness.
[[nodiscard]] ExactFacetDescentSegmentResult
build_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids);

// Rebuilds the complete 6.3 arc from the input facet before reconstructing all
// exact segment coefficients; no observed witness steers the fresh replay.
[[nodiscard]] ExactFacetDescentSegmentVerification
verify_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    const ExactFacetDescentSegmentResult& result);

// Iterates the canonical 6.4 transition from one source facet until the first
// regular active facet, unsupported degeneracy, or explicit strict-segment
// budget frontier. The budget is external policy and zero is valid. Repeated
// facets contradict the already-certified strict potential and fail closed.
[[nodiscard]] ExactFacetDescentChainResult build_exact_facet_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    ExactFacetDescentChainBudget budget);

// Reconstructs the expected orbit from the cloud, initial facet, and external
// budget only. No observed node, witness, stopping decision, or length steers
// the fresh replay.
[[nodiscard]] ExactFacetDescentChainVerification
verify_exact_facet_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    ExactFacetDescentChainBudget budget,
    const ExactFacetDescentChainResult& result);

}  // namespace morsehgp3d::hierarchy
