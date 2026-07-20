#pragma once

#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

// This bounded reference catalogue exhausts every one-to-four-point support.
// It classifies local support geometry first and performs a global closed-ball
// query only for a positive minimal support.  It creates neither Gamma data,
// hierarchy roots, public births or merges, nor a public status.
struct ExactCriticalCatalogBudget {
  static constexpr std::size_t maximum_supported_candidate_count = 1470U;
  static constexpr std::size_t
      maximum_supported_point_classification_count = 20580U;

  std::size_t maximum_candidate_count{};
  std::size_t maximum_point_classification_count{};

  friend bool operator==(
      const ExactCriticalCatalogBudget&,
      const ExactCriticalCatalogBudget&) = default;
};

enum class ExactCriticalCatalogCandidateOutcome : std::uint8_t {
  not_classified,
  affinely_dependent_support,
  boundary_reduced_support,
  exterior_circumcenter_support,
  relevant_extra_shell_degeneracy,
  extra_shell_outside_relevant_window,
  minimal_support_above_rank_window,
  accepted_critical_event,
};

[[nodiscard]] std::string_view to_string(
    ExactCriticalCatalogCandidateOutcome outcome);

enum class ExactCriticalCatalogDecision : std::uint8_t {
  not_certified,
  no_catalog_preflight_budget_insufficient,
  complete_supported_critical_catalog,
  complete_catalog_with_relevant_extra_shell_degeneracy,
};

enum class ExactCriticalCatalogScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only,
};

struct ExactCriticalCatalogPredicateCounters {
  std::uint64_t fp64_filtered_certified_count{};
  std::uint64_t expansion_certified_count{};
  std::uint64_t cpu_multiprecision_certified_count{};
  std::uint64_t exact_zero_count{};
  std::uint64_t remaining_unknown_count{};

  friend bool operator==(
      const ExactCriticalCatalogPredicateCounters&,
      const ExactCriticalCatalogPredicateCounters&) = default;
};

struct ExactCriticalCatalogCandidate {
  std::size_t candidate_index{};
  std::vector<spatial::PointId> support_point_ids;
  exact::CircumcenterSupportStatus support_status{
      exact::CircumcenterSupportStatus::affinely_dependent};
  ExactCriticalCatalogCandidateOutcome outcome{
      ExactCriticalCatalogCandidateOutcome::not_classified};
  std::optional<exact::ExactCenter3> center;
  std::optional<exact::ExactLevel> squared_level;
  std::vector<exact::ExactRational> support_barycentric_coordinates;
  std::vector<exact::PredicateSign> support_barycentric_signs;
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<spatial::PointId> exterior_point_ids;
  std::vector<spatial::PointId> closed_point_ids;
  std::size_t observed_closed_rank{};
  // For an extra shell this is |I| plus the input support size.  It is the
  // RelevantGP window test even when observed_closed_rank exceeds s_max.
  std::size_t support_relevance_rank{};
  bool global_closed_ball_classified{false};
  std::optional<std::size_t> event_index;
  std::optional<std::size_t> extra_shell_degeneracy_index;

  friend bool operator==(
      const ExactCriticalCatalogCandidate&,
      const ExactCriticalCatalogCandidate&) = default;
};

struct ExactCriticalEvent {
  std::size_t event_index{};
  std::size_t source_candidate_index{};
  std::vector<spatial::PointId> support_point_ids;
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<spatial::PointId> closed_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;
  std::vector<exact::ExactRational> support_barycentric_coordinates;
  std::vector<exact::PredicateSign> support_barycentric_signs;
  std::size_t closed_rank{};
  std::optional<std::size_t> birth_order;
  std::optional<std::size_t> saddle_order;

  friend bool operator==(
      const ExactCriticalEvent&,
      const ExactCriticalEvent&) = default;
};

// Several positive minimal supports may expose the same globally closed ball.
// They are retained as exhaustive candidates but aggregated into one record.
struct ExactCriticalExtraShellDegeneracy {
  std::size_t degeneracy_index{};
  exact::ExactCenter3 center;
  exact::ExactLevel squared_level;
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<spatial::PointId> closed_point_ids;
  std::size_t observed_closed_rank{};
  std::vector<std::vector<spatial::PointId>> support_point_id_sets;
  std::vector<std::size_t> support_candidate_indices;
  std::vector<std::size_t> relevant_support_candidate_indices;
  bool has_relevant_support{false};

  friend bool operator==(
      const ExactCriticalExtraShellDegeneracy&,
      const ExactCriticalExtraShellDegeneracy&) = default;
};

struct ExactCriticalH0Batch {
  std::size_t order{};
  exact::ExactLevel squared_level;
  std::vector<std::size_t> birth_event_indices;
  std::vector<std::size_t> saddle_event_indices;

  friend bool operator==(
      const ExactCriticalH0Batch&,
      const ExactCriticalH0Batch&) = default;
};

struct ExactCriticalCatalogCounters {
  std::size_t preflight_count{};
  std::array<std::size_t, 4> enumerated_candidate_count_by_support_size{};
  std::size_t enumerated_candidate_count{};
  std::size_t support_analysis_count{};
  std::size_t affinely_dependent_support_count{};
  std::size_t boundary_reduced_support_count{};
  std::size_t exterior_circumcenter_support_count{};
  std::size_t minimal_support_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t global_point_classification_count{};
  std::size_t relevant_extra_shell_candidate_count{};
  std::size_t outside_window_extra_shell_candidate_count{};
  std::size_t deduplicated_extra_shell_degeneracy_count{};
  std::size_t above_rank_candidate_count{};
  std::size_t accepted_event_count{};
  std::size_t birth_event_reference_count{};
  std::size_t saddle_event_reference_count{};
  std::size_t h0_batch_count{};

  friend bool operator==(
      const ExactCriticalCatalogCounters&,
      const ExactCriticalCatalogCounters&) = default;
};

struct ExactCriticalCatalogResult {
  static constexpr std::size_t minimum_supported_point_count = 1U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_maximum_order = 1U;
  static constexpr std::size_t maximum_supported_maximum_order = 10U;
  static constexpr std::size_t maximum_support_point_count = 4U;
  static constexpr const char* proof_basis =
      "exhaustive_exact_supports_up_to_four_global_closed_ball_"
      "critical_catalog_h0_batches_v1";

  ExactCriticalCatalogBudget requested_budget{};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t required_candidate_count{};
  std::size_t required_point_classification_count{};
  std::vector<ExactCriticalCatalogCandidate> candidates;
  std::vector<ExactCriticalEvent> events;
  std::vector<ExactCriticalExtraShellDegeneracy>
      extra_shell_degeneracies;
  std::vector<ExactCriticalH0Batch> h0_batches;
  ExactCriticalCatalogPredicateCounters predicate_counters{};
  bool candidate_space_size_certified{false};
  bool preflight_budget_sufficient{false};
  bool geometry_started_after_successful_preflight{false};
  bool all_support_candidates_classified{false};
  bool global_closed_ball_queries_restricted_to_minimal_supports{false};
  bool all_minimal_support_global_partitions_complete{false};
  bool extra_shell_degeneracies_deduplicated{false};
  bool accepted_events_canonical_and_indexed{false};
  bool h0_batches_canonical_and_complete{false};
  bool no_relevant_extra_shell_degeneracy{false};
  ExactCriticalCatalogCounters counters{};
  ExactCriticalCatalogDecision decision{
      ExactCriticalCatalogDecision::not_certified};
  ExactCriticalCatalogScope scope{ExactCriticalCatalogScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogResult&,
      const ExactCriticalCatalogResult&) = default;
};

struct ExactCriticalCatalogVerification {
  bool requested_budget_certified{false};
  bool input_domain_certified{false};
  bool derived_sizes_certified{false};
  bool candidates_certified{false};
  bool events_certified{false};
  bool extra_shell_degeneracies_certified{false};
  bool h0_batches_certified{false};
  bool predicate_counters_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_decision_certified{false};
};

[[nodiscard]] ExactCriticalCatalogResult build_exact_critical_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactCriticalCatalogBudget budget);

// Rebuilds the complete bounded catalogue from the external cloud, order and
// trusted budget.  No candidate, event, degeneracy, batch or fact stored in
// the observed result steers the replay.
[[nodiscard]] ExactCriticalCatalogVerification verify_exact_critical_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactCriticalCatalogBudget budget,
    const ExactCriticalCatalogResult& result);

}  // namespace morsehgp3d::hierarchy
