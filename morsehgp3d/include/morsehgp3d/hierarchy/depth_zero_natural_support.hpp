#pragma once

#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/spatial/ordinary_diagram_closure.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

// Phase 8.5 extracts the two-to-four-point critical supports whose strict
// interior is empty from a freshly certified ordinary diagram.  It does not
// create H0 batches, a CatalogCertificate, or closed_parent_orders[1].
struct ExactDepthZeroNaturalSupportBudget {
  static constexpr std::size_t trusted_maximum_source_contact_count = 247U;
  static constexpr std::size_t
      trusted_maximum_raw_support_proposal_count = 4704U;
  static constexpr std::size_t
      trusted_maximum_raw_support_point_id_reference_count = 13440U;
  static constexpr std::size_t trusted_maximum_unique_support_count = 154U;
  static constexpr std::size_t
      trusted_maximum_unique_support_point_id_reference_count = 504U;
  static constexpr std::size_t
      trusted_maximum_point_classification_count = 1232U;

  std::size_t maximum_source_contact_count{
      trusted_maximum_source_contact_count};
  std::size_t maximum_raw_support_proposal_count{
      trusted_maximum_raw_support_proposal_count};
  std::size_t maximum_raw_support_point_id_reference_count{
      trusted_maximum_raw_support_point_id_reference_count};
  std::size_t maximum_unique_support_count{
      trusted_maximum_unique_support_count};
  std::size_t maximum_unique_support_point_id_reference_count{
      trusted_maximum_unique_support_point_id_reference_count};
  std::size_t maximum_point_classification_count{
      trusted_maximum_point_classification_count};

  friend bool operator==(
      const ExactDepthZeroNaturalSupportBudget&,
      const ExactDepthZeroNaturalSupportBudget&) = default;
};

struct ExactDepthZeroNaturalSupportRequirements {
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  // RelevantGP uses |I|+|U|, not the observed closed rank |I|+|S|.
  // At depth zero, after I is proved empty, this is simply |U|<=s_max.
  std::size_t maximum_relevant_support_rank{};
  std::size_t conservative_source_contact_count{};
  std::size_t conservative_raw_support_proposal_count{};
  std::size_t conservative_raw_support_point_id_reference_count{};
  std::size_t conservative_unique_support_count{};
  std::size_t conservative_unique_support_point_id_reference_count{};
  std::size_t conservative_point_classification_count{};

  friend bool operator==(
      const ExactDepthZeroNaturalSupportRequirements&,
      const ExactDepthZeroNaturalSupportRequirements&) = default;
};

enum class ExactDepthZeroNaturalSupportCandidateOutcome : std::uint8_t {
  not_classified,
  affinely_dependent_support,
  boundary_reduced_support,
  exterior_circumcenter_support,
  minimal_with_strict_interior_deferred,
  relevant_extra_shell_degeneracy,
  extra_shell_outside_rank_window,
  minimal_support_above_rank_window,
  accepted_empty_interior_support,
};

[[nodiscard]] std::string_view to_string(
    ExactDepthZeroNaturalSupportCandidateOutcome outcome);

struct ExactDepthZeroNaturalSupportCandidate {
  std::size_t candidate_index{};
  std::vector<spatial::PointId> support_point_ids;
  std::size_t first_proposal_contact_index{};
  exact::CircumcenterSupportStatus support_status{
      exact::CircumcenterSupportStatus::affinely_dependent};
  ExactDepthZeroNaturalSupportCandidateOutcome outcome{
      ExactDepthZeroNaturalSupportCandidateOutcome::not_classified};
  std::optional<exact::ExactCenter3> center;
  std::optional<exact::ExactLevel> squared_level;
  std::vector<exact::ExactRational> support_barycentric_coordinates;
  std::vector<exact::PredicateSign> support_barycentric_signs;
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<spatial::PointId> exterior_point_ids;
  std::size_t observed_closed_rank{};
  std::size_t support_relevance_rank{};
  bool global_closed_ball_classified{false};
  std::optional<std::size_t> carrier_contact_index;
  std::optional<std::size_t> accepted_support_index;
  std::optional<std::size_t> relevant_extra_shell_diagnostic_index;

  friend bool operator==(
      const ExactDepthZeroNaturalSupportCandidate&,
      const ExactDepthZeroNaturalSupportCandidate&) = default;
};

struct ExactDepthZeroNaturalSupport {
  std::size_t support_index{};
  std::size_t source_candidate_index{};
  std::size_t natural_contact_index{};
  std::vector<spatial::PointId> support_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;
  std::vector<exact::ExactRational> support_barycentric_coordinates;
  std::vector<exact::PredicateSign> support_barycentric_signs;
  std::size_t closed_rank{};
  spatial::ExactOrdinaryDiagramContactKind natural_contact_kind{
      spatial::ExactOrdinaryDiagramContactKind::natural_face};
  std::size_t contact_affine_dimension{};
  std::size_t contact_site_affine_rank{};

  friend bool operator==(
      const ExactDepthZeroNaturalSupport&,
      const ExactDepthZeroNaturalSupport&) = default;
};

struct ExactDepthZeroNaturalExtraShellDiagnostic {
  std::size_t diagnostic_index{};
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;
  std::vector<spatial::PointId> shell_point_ids;
  std::size_t observed_closed_rank{};
  std::size_t carrier_contact_index{};
  std::vector<std::vector<spatial::PointId>> support_point_id_sets;
  std::vector<std::size_t> source_candidate_indices;

  friend bool operator==(
      const ExactDepthZeroNaturalExtraShellDiagnostic&,
      const ExactDepthZeroNaturalExtraShellDiagnostic&) = default;
};

struct ExactDepthZeroNaturalSupportAudit {
  std::size_t source_contact_count{};
  std::size_t natural_carrier_contact_count{};
  std::size_t raw_support_proposal_count{};
  std::size_t raw_support_point_id_reference_count{};
  std::size_t unique_support_count{};
  std::size_t unique_support_point_id_reference_count{};
  std::size_t support_analysis_count{};
  std::size_t minimal_support_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t point_classification_count{};
  std::size_t affinely_dependent_support_count{};
  std::size_t boundary_reduced_support_count{};
  std::size_t exterior_circumcenter_support_count{};
  std::size_t deferred_strict_interior_support_count{};
  std::size_t relevant_extra_shell_support_count{};
  std::size_t outside_window_extra_shell_support_count{};
  std::size_t above_rank_support_count{};
  std::size_t accepted_support_count{};
  std::size_t deduplicated_relevant_extra_shell_diagnostic_count{};
  std::size_t overflow_count{};

  friend bool operator==(
      const ExactDepthZeroNaturalSupportAudit&,
      const ExactDepthZeroNaturalSupportAudit&) = default;
};

struct ExactDepthZeroNaturalSupportPredicateCounters {
  std::uint64_t fp64_filtered_certified_count{};
  std::uint64_t expansion_certified_count{};
  std::uint64_t cpu_multiprecision_certified_count{};
  std::uint64_t exact_zero_count{};
  std::uint64_t remaining_unknown_count{};

  friend bool operator==(
      const ExactDepthZeroNaturalSupportPredicateCounters&,
      const ExactDepthZeroNaturalSupportPredicateCounters&) = default;
};

enum class ExactDepthZeroNaturalSupportDecision : std::uint8_t {
  source_diagram_not_complete_or_not_certified,
  insufficient_budget,
  complete_supported_extraction,
  complete_extraction_with_relevant_extra_shell_degeneracy,
};

[[nodiscard]] std::string_view to_string(
    ExactDepthZeroNaturalSupportDecision decision);

struct ExactDepthZeroNaturalSupportResult {
  static constexpr std::size_t minimum_supported_point_count = 1U;
  static constexpr std::size_t maximum_supported_point_count = 8U;
  static constexpr std::size_t minimum_supported_maximum_order = 1U;
  static constexpr std::size_t maximum_supported_maximum_order = 10U;
  static constexpr std::size_t minimum_support_point_count = 2U;
  static constexpr std::size_t maximum_support_point_count = 4U;
  static constexpr std::string_view schema =
      "morsehgp3d.phase8.exact_bounded_depth_zero_natural_supports.v1";
  static constexpr std::string_view proof_basis =
      "verified_ordinary_natural_carriers_complete_subsupports_exact_"
      "circumcenter_global_closed_ball_depth_zero_v1";
  static constexpr std::string_view scope =
      "bounded_n8_verified_ordinary_diagram_depth_zero_candidates_2_to_4_"
      "accepted_only_in_relevant_rank_window";

  ExactDepthZeroNaturalSupportBudget requested_budget;
  std::size_t requested_maximum_order{};
  std::vector<std::array<std::uint64_t, 3>> canonical_point_bits;
  spatial::StrictlyPaddedDyadicAabb3Result clipping_box;
  ExactDepthZeroNaturalSupportRequirements requirements;
  std::optional<spatial::ExactOrdinaryDiagramClosureResult> source_diagram;
  std::vector<ExactDepthZeroNaturalSupportCandidate> candidates;
  std::vector<ExactDepthZeroNaturalSupport> supports;
  std::vector<ExactDepthZeroNaturalExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  ExactDepthZeroNaturalSupportPredicateCounters predicate_counters;
  ExactDepthZeroNaturalSupportAudit audit;
  bool preflight_budget_sufficient{false};
  bool extraction_started_after_successful_preflight{false};
  bool source_diagram_freshly_verified{false};
  bool proposal_space_complete{false};
  bool all_candidates_classified{false};
  bool all_minimal_support_partitions_complete{false};
  bool accepted_supports_natural_and_indexed{false};
  bool relevant_extra_shell_diagnostics_complete{false};
  bool candidate_queue_empty{false};
  bool no_depth_zero_relevant_extra_shell_degeneracy{false};
  bool no_artificial_support_emitted{false};
  bool no_remaining_unknown_predicates{false};
  ExactDepthZeroNaturalSupportDecision decision{
      ExactDepthZeroNaturalSupportDecision::insufficient_budget};

  friend bool operator==(
      const ExactDepthZeroNaturalSupportResult&,
      const ExactDepthZeroNaturalSupportResult&) = default;
};

struct ExactDepthZeroNaturalSupportVerification {
  bool requested_budget_certified{false};
  bool input_identity_certified{false};
  bool clipping_box_certified{false};
  bool requirements_certified{false};
  bool source_diagram_certified{false};
  bool candidates_certified{false};
  bool supports_certified{false};
  bool relevant_extra_shell_diagnostics_certified{false};
  bool predicate_counters_certified{false};
  bool audit_certified{false};
  bool result_facts_certified{false};
  bool decision_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

[[nodiscard]] ExactDepthZeroNaturalSupportResult
build_exact_bounded_depth_zero_natural_supports(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    std::size_t requested_maximum_order,
    ExactDepthZeroNaturalSupportBudget budget = {});

// Rebuilds every extraction decision from the external cloud, box, source
// diagram, maximum order, and trusted budget.  No observed support, carrier,
// diagnostic, counter, or fact steers the replay.
[[nodiscard]] ExactDepthZeroNaturalSupportVerification
verify_exact_bounded_depth_zero_natural_supports(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    std::size_t requested_maximum_order,
    ExactDepthZeroNaturalSupportBudget budget,
    const ExactDepthZeroNaturalSupportResult& result);

}  // namespace morsehgp3d::hierarchy
