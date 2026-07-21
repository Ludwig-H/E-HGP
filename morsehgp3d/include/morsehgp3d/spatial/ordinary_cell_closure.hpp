#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/point_cloud_aabb.hpp"
#include "morsehgp3d/spatial/power_cell_reference.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::spatial {

enum class ExactOrdinaryCellVertexClassification : std::uint8_t {
  owner_strict_nearest,
  violating_nearest_shell,
  missing_active_nearest_shell,
  reconciled_active_nearest_shell,
};

[[nodiscard]] std::string_view to_string(
    ExactOrdinaryCellVertexClassification classification);

struct ExactOrdinaryCellVertexQueryRecord {
  std::size_t candidate_vertex_index{};
  exact::ExactLevel nearest_squared_distance;
  std::vector<PointId> complete_nearest_shell_ids;
  std::vector<PointId> newly_required_competitor_ids;
  ExactOrdinaryCellVertexClassification classification{
      ExactOrdinaryCellVertexClassification::owner_strict_nearest};

  friend bool operator==(
      const ExactOrdinaryCellVertexQueryRecord&,
      const ExactOrdinaryCellVertexQueryRecord&) = default;
};

struct ExactOrdinaryCellClosureRound {
  std::size_t round_index{};
  std::vector<PointId> canonical_candidate_competitor_ids;
  ExactPowerCellReferenceResult candidate_cell;
  std::vector<ExactOrdinaryCellVertexQueryRecord> vertex_queries;
  std::vector<PointId> simultaneously_added_competitor_ids;
  bool local_queue_empty{false};

  friend bool operator==(
      const ExactOrdinaryCellClosureRound&,
      const ExactOrdinaryCellClosureRound&) = default;
};

enum class ExactOrdinaryCellClosureDecision : std::uint8_t {
  complete_nonempty,
  insufficient_budget,
};

[[nodiscard]] std::string_view to_string(
    ExactOrdinaryCellClosureDecision decision);

struct ExactOrdinaryCellClosureBudget {
  static constexpr std::size_t trusted_maximum_point_count = 8U;
  static constexpr std::size_t
      trusted_maximum_cell_construction_count = 7U;
  static constexpr std::size_t
      trusted_maximum_cumulative_plane_triple_count = 966U;
  static constexpr std::size_t
      trusted_maximum_cumulative_vertex_count = 966U;
  static constexpr std::size_t
      trusted_maximum_cumulative_incidence_count = 10822U;
  static constexpr std::size_t trusted_maximum_vertex_query_count = 966U;
  static constexpr std::size_t
      trusted_maximum_exact_distance_evaluation_count = 7728U;
  static constexpr std::size_t
      trusted_maximum_nearest_shell_entry_count = 7728U;
  static constexpr std::size_t
      trusted_maximum_owner_strict_feasibility_test_count = 7U;
  static constexpr std::size_t
      trusted_maximum_simultaneous_addition_count = 6U;

  std::size_t maximum_cell_construction_count{
      trusted_maximum_cell_construction_count};
  std::size_t maximum_cumulative_plane_triple_count{
      trusted_maximum_cumulative_plane_triple_count};
  std::size_t maximum_cumulative_vertex_count{
      trusted_maximum_cumulative_vertex_count};
  std::size_t maximum_cumulative_incidence_count{
      trusted_maximum_cumulative_incidence_count};
  std::size_t maximum_vertex_query_count{
      trusted_maximum_vertex_query_count};
  std::size_t maximum_exact_distance_evaluation_count{
      trusted_maximum_exact_distance_evaluation_count};
  std::size_t maximum_nearest_shell_entry_count{
      trusted_maximum_nearest_shell_entry_count};
  std::size_t maximum_owner_strict_feasibility_test_count{
      trusted_maximum_owner_strict_feasibility_test_count};
  std::size_t maximum_simultaneous_addition_count{
      trusted_maximum_simultaneous_addition_count};
};

struct ExactOrdinaryCellClosureRequirements {
  std::size_t point_count{};
  std::size_t complete_competitor_count{};
  std::size_t requested_initial_competitor_count{};
  std::size_t effective_initial_competitor_count{};
  std::size_t conservative_cell_construction_count{};
  std::size_t conservative_cumulative_plane_triple_count{};
  std::size_t conservative_cumulative_vertex_count{};
  std::size_t conservative_cumulative_incidence_count{};
  std::size_t conservative_vertex_query_count{};
  std::size_t conservative_exact_distance_evaluation_count{};
  std::size_t conservative_nearest_shell_entry_count{};
  std::size_t conservative_owner_strict_feasibility_test_count{};
  std::size_t conservative_simultaneous_addition_count{};

  friend bool operator==(
      const ExactOrdinaryCellClosureRequirements&,
      const ExactOrdinaryCellClosureRequirements&) = default;
};

struct ExactOrdinaryCellClosureAudit {
  std::size_t exact_cell_construction_count{};
  std::size_t cumulative_plane_triple_capacity{};
  std::size_t cumulative_vertex_capacity{};
  std::size_t cumulative_incidence_capacity{};
  std::size_t exact_vertex_query_count{};
  std::size_t exact_distance_evaluation_count{};
  std::size_t nearest_shell_entry_count{};
  std::size_t owner_strict_vertex_count{};
  std::size_t violating_vertex_count{};
  std::size_t owner_tie_vertex_count{};
  std::size_t missing_active_vertex_count{};
  std::size_t simultaneous_addition_batch_count{};
  std::size_t simultaneously_added_competitor_count{};
  std::size_t maximum_simultaneous_addition_count{};
  std::size_t owner_strict_feasibility_test_count{};

  friend bool operator==(
      const ExactOrdinaryCellClosureAudit&,
      const ExactOrdinaryCellClosureAudit&) = default;
};

struct ExactOrdinaryCellClosureResult {
  static constexpr std::string_view schema =
      "morsehgp3d.phase8.exact_ordinary_cell_vertex_nn_closure.v1";
  static constexpr std::string_view proof_basis =
      "exact_affine_vertex_revelation_complete_nearest_shell_monotone_queue_v1";
  static constexpr std::string_view scope =
      "bounded_n8_single_ordinary_cell_only";

  ExactOrdinaryCellClosureDecision decision{
      ExactOrdinaryCellClosureDecision::insufficient_budget};
  PointId owner_id{};
  ExactOrdinaryCellClosureRequirements requirements;
  ExactOrdinaryCellClosureAudit audit;
  StrictlyPaddedDyadicAabb3Result clipping_box;
  std::vector<PointId> complete_competitor_ids;
  std::vector<PointId> canonical_requested_initial_competitor_ids;
  std::vector<PointId> canonical_effective_initial_competitor_ids;
  std::vector<PointId> canonical_closed_competitor_ids;
  std::vector<ExactOrdinaryCellClosureRound> rounds;
  bool fallback_seed_injected{false};
  bool local_queue_empty_certified{false};
  bool owner_strict_feasible_certified{false};
  bool full_dimensional_nonempty_certified{false};
  bool active_nearest_shells_reconciled_certified{false};
  bool artificial_box_boundaries_certified{false};

  [[nodiscard]] const ExactPowerCellReferenceResult* final_cell()
      const & noexcept {
    return decision == ExactOrdinaryCellClosureDecision::complete_nonempty &&
                   !rounds.empty()
               ? &rounds.back().candidate_cell
               : nullptr;
  }
  [[nodiscard]] const ExactPowerCellReferenceResult* final_cell()
      const && = delete;

  friend bool operator==(
      const ExactOrdinaryCellClosureResult&,
      const ExactOrdinaryCellClosureResult&) = default;
};

struct ExactOrdinaryCellClosureVerification {
  bool input_identity_certified{false};
  bool clipping_box_certified{false};
  bool decision_certified{false};
  bool requirements_certified{false};
  bool audit_certified{false};
  bool payload_shape_certified{false};
  bool transcript_replay_certified{false};
  bool monotone_queue_certified{false};
  bool owner_strict_feasible_certified{false};
  bool full_dimensional_nonempty_certified{false};
  bool active_nearest_shells_reconciled_certified{false};
  bool artificial_box_boundaries_certified{false};
  bool complete_oracle_projection_certified{false};
  bool result_certified{false};
};

// Bounded Phase 8 reference loop for one ordinary Voronoi cell.  An empty
// proposal receives the least exterior PointId as a recorded canonical seed.
// Every nonterminal round adds the complete union of newly revealed nearest
// shells and every successful result ends with a freshly empty local queue.
[[nodiscard]] ExactOrdinaryCellClosureResult
build_exact_bounded_ordinary_cell_closure(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_competitor_ids,
    ExactOrdinaryCellClosureBudget budget = {});

[[nodiscard]] ExactOrdinaryCellClosureVerification
verify_exact_bounded_ordinary_cell_closure(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_competitor_ids,
    const ExactOrdinaryCellClosureResult& result,
    ExactOrdinaryCellClosureBudget budget = {});

}  // namespace morsehgp3d::spatial
