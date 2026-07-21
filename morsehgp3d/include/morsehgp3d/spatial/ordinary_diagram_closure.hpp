#pragma once

#include "morsehgp3d/spatial/ordinary_cell_closure.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::spatial {

struct ExactOrdinaryDiagramVertexOccurrence {
  PointId owner_id{};
  std::size_t cell_vertex_index{};

  friend bool operator==(
      const ExactOrdinaryDiagramVertexOccurrence&,
      const ExactOrdinaryDiagramVertexOccurrence&) = default;
};

struct ExactOrdinaryDiagramVertex {
  exact::ExactRational3 position;
  exact::ExactLevel nearest_squared_distance;
  std::vector<PointId> complete_nearest_shell_ids;
  std::uint8_t artificial_box_face_mask{};
  std::vector<ExactOrdinaryDiagramVertexOccurrence> cell_occurrences;

  friend bool operator==(
      const ExactOrdinaryDiagramVertex&,
      const ExactOrdinaryDiagramVertex&) = default;
};

enum class ExactOrdinaryDiagramContactKind : std::uint8_t {
  noncanonical_quotient_contact,
  natural_face,
  natural_edge,
  natural_vertex,
  box_supported_contact,
};

[[nodiscard]] std::string_view to_string(
    ExactOrdinaryDiagramContactKind kind);

// K_Q is the common intersection of the cells in query_ids.  The carrier
// shell is measured freshly at the average of every vertex of K_Q.  Only a
// contact whose query and carrier agree can be a canonical natural stratum;
// proper subsets remain explicit quotient contacts instead of fake faces
// under cospherical degeneracy.
struct ExactOrdinaryDiagramContact {
  std::vector<PointId> query_ids;
  std::vector<PointId> carrier_shell_ids;
  std::vector<std::size_t> global_vertex_indices;
  std::size_t affine_dimension{};
  std::size_t site_affine_rank{};
  std::uint8_t common_artificial_box_face_mask{};
  exact::ExactRational3 relative_interior_witness;
  exact::ExactLevel witness_nearest_squared_distance;
  ExactOrdinaryDiagramContactKind kind{
      ExactOrdinaryDiagramContactKind::noncanonical_quotient_contact};

  friend bool operator==(
      const ExactOrdinaryDiagramContact&,
      const ExactOrdinaryDiagramContact&) = default;
};

enum class ExactOrdinaryDiagramClosureDecision : std::uint8_t {
  complete,
  insufficient_budget,
};

[[nodiscard]] std::string_view to_string(
    ExactOrdinaryDiagramClosureDecision decision);

struct ExactOrdinaryDiagramClosureBudget {
  static constexpr std::size_t trusted_maximum_point_count = 8U;
  static constexpr std::size_t trusted_maximum_cell_count = 8U;
  static constexpr std::size_t
      trusted_maximum_cell_construction_count = 56U;
  static constexpr std::size_t
      trusted_maximum_cumulative_plane_triple_count = 7728U;
  static constexpr std::size_t
      trusted_maximum_cumulative_vertex_count = 7728U;
  static constexpr std::size_t
      trusted_maximum_cumulative_incidence_count = 86576U;
  static constexpr std::size_t trusted_maximum_vertex_query_count = 7728U;
  static constexpr std::size_t
      trusted_maximum_exact_distance_evaluation_count = 61824U;
  static constexpr std::size_t
      trusted_maximum_nearest_shell_entry_count = 61824U;
  static constexpr std::size_t
      trusted_maximum_owner_strict_feasibility_test_count = 56U;
  static constexpr std::size_t
      trusted_maximum_simultaneous_addition_batch_count = 48U;
  static constexpr std::size_t
      trusted_maximum_total_simultaneous_addition_count = 48U;
  static constexpr std::size_t
      trusted_maximum_simultaneous_batch_size = 6U;
  static constexpr std::size_t
      trusted_maximum_final_cell_vertex_occurrence_count = 2288U;
  static constexpr std::size_t trusted_maximum_global_vertex_count = 2288U;
  static constexpr std::size_t
      trusted_maximum_global_nearest_shell_entry_count = 18304U;
  static constexpr std::size_t
      trusted_maximum_contact_count = 247U;
  static constexpr std::size_t
      trusted_maximum_contact_query_id_count = 1016U;
  static constexpr std::size_t
      trusted_maximum_contact_carrier_shell_id_count = 1976U;
  static constexpr std::size_t
      trusted_maximum_contact_vertex_reference_count = 565136U;
  static constexpr std::size_t
      trusted_maximum_stratum_witness_query_count = 247U;
  static constexpr std::size_t
      trusted_maximum_stratum_witness_exact_distance_evaluation_count = 1976U;

  static_assert(trusted_maximum_cell_construction_count == 8U * 7U);
  static_assert(
      trusted_maximum_simultaneous_addition_batch_count == 8U * 6U);
  static_assert(
      trusted_maximum_total_simultaneous_addition_count == 8U * 6U);
  static_assert(
      trusted_maximum_cumulative_plane_triple_count == 8U * 966U);
  static_assert(
      trusted_maximum_cumulative_incidence_count == 8U * 10822U);
  static_assert(
      trusted_maximum_exact_distance_evaluation_count == 8U * 7728U);
  static_assert(
      trusted_maximum_final_cell_vertex_occurrence_count == 8U * 286U);
  static_assert(
      trusted_maximum_global_nearest_shell_entry_count == 8U * 2288U);
  static_assert(trusted_maximum_contact_count == 256U - 8U - 1U);
  static_assert(
      trusted_maximum_contact_query_id_count == 8U * 128U - 8U);
  static_assert(
      trusted_maximum_contact_carrier_shell_id_count == 8U * 247U);
  static_assert(
      trusted_maximum_contact_vertex_reference_count == 2288U * 247U);
  static_assert(
      trusted_maximum_stratum_witness_exact_distance_evaluation_count ==
      8U * 247U);

  std::size_t maximum_cell_count{trusted_maximum_cell_count};
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
  std::size_t maximum_simultaneous_addition_batch_count{
      trusted_maximum_simultaneous_addition_batch_count};
  std::size_t maximum_total_simultaneous_addition_count{
      trusted_maximum_total_simultaneous_addition_count};
  std::size_t maximum_simultaneous_batch_size{
      trusted_maximum_simultaneous_batch_size};
  std::size_t maximum_final_cell_vertex_occurrence_count{
      trusted_maximum_final_cell_vertex_occurrence_count};
  std::size_t maximum_global_vertex_count{
      trusted_maximum_global_vertex_count};
  std::size_t maximum_global_nearest_shell_entry_count{
      trusted_maximum_global_nearest_shell_entry_count};
  std::size_t maximum_contact_count{trusted_maximum_contact_count};
  std::size_t maximum_contact_query_id_count{
      trusted_maximum_contact_query_id_count};
  std::size_t maximum_contact_carrier_shell_id_count{
      trusted_maximum_contact_carrier_shell_id_count};
  std::size_t maximum_contact_vertex_reference_count{
      trusted_maximum_contact_vertex_reference_count};
  std::size_t maximum_stratum_witness_query_count{
      trusted_maximum_stratum_witness_query_count};
  std::size_t maximum_stratum_witness_exact_distance_evaluation_count{
      trusted_maximum_stratum_witness_exact_distance_evaluation_count};
};

struct ExactOrdinaryDiagramClosureRequirements {
  std::size_t point_count{};
  std::size_t conservative_cell_count{};
  std::size_t conservative_cell_construction_count{};
  std::size_t conservative_cumulative_plane_triple_count{};
  std::size_t conservative_cumulative_vertex_count{};
  std::size_t conservative_cumulative_incidence_count{};
  std::size_t conservative_vertex_query_count{};
  std::size_t conservative_exact_distance_evaluation_count{};
  std::size_t conservative_nearest_shell_entry_count{};
  std::size_t conservative_owner_strict_feasibility_test_count{};
  std::size_t conservative_simultaneous_addition_batch_count{};
  std::size_t conservative_total_simultaneous_addition_count{};
  std::size_t conservative_maximum_simultaneous_batch_size{};
  std::size_t conservative_final_cell_vertex_occurrence_count{};
  std::size_t conservative_global_vertex_count{};
  std::size_t conservative_global_nearest_shell_entry_count{};
  std::size_t conservative_contact_count{};
  std::size_t conservative_contact_query_id_count{};
  std::size_t conservative_contact_carrier_shell_id_count{};
  std::size_t conservative_contact_vertex_reference_count{};
  std::size_t conservative_stratum_witness_query_count{};
  std::size_t conservative_stratum_witness_exact_distance_evaluation_count{};

  friend bool operator==(
      const ExactOrdinaryDiagramClosureRequirements&,
      const ExactOrdinaryDiagramClosureRequirements&) = default;
};

struct ExactOrdinaryDiagramClosureAudit {
  std::size_t completed_cell_count{};
  std::size_t exact_cell_construction_count{};
  std::size_t cumulative_plane_triple_capacity{};
  std::size_t cumulative_vertex_capacity{};
  std::size_t cumulative_incidence_capacity{};
  std::size_t exact_vertex_query_count{};
  std::size_t exact_distance_evaluation_count{};
  std::size_t nearest_shell_entry_count{};
  std::size_t owner_strict_feasibility_test_count{};
  std::size_t simultaneous_addition_batch_count{};
  std::size_t simultaneously_added_competitor_count{};
  std::size_t maximum_simultaneous_batch_size{};
  std::size_t final_cell_vertex_occurrence_count{};
  std::size_t global_vertex_count{};
  std::size_t global_nearest_shell_entry_count{};
  std::size_t contact_count{};
  std::size_t contact_query_id_count{};
  std::size_t contact_carrier_shell_id_count{};
  std::size_t contact_vertex_reference_count{};
  std::size_t noncanonical_quotient_contact_count{};
  std::size_t natural_face_count{};
  std::size_t natural_edge_count{};
  std::size_t natural_vertex_count{};
  std::size_t box_supported_contact_count{};
  std::size_t exact_stratum_witness_query_count{};
  std::size_t exact_stratum_witness_distance_evaluation_count{};

  friend bool operator==(
      const ExactOrdinaryDiagramClosureAudit&,
      const ExactOrdinaryDiagramClosureAudit&) = default;
};

struct ExactOrdinaryDiagramClosureResult {
  static constexpr std::string_view schema =
      "morsehgp3d.phase8.exact_bounded_ordinary_diagram_closure.v1";
  static constexpr std::string_view proof_basis =
      "exact_all_owner_cell_closure_vertex_occurrence_bijection_barycentric_"
      "contact_carrier_quotient_v1";
  static constexpr std::string_view scope =
      "bounded_n8_all_ordinary_cells_auditable_contacts_and_reciprocal_"
      "natural_strata_only";

  ExactOrdinaryDiagramClosureDecision decision{
      ExactOrdinaryDiagramClosureDecision::insufficient_budget};
  std::vector<std::array<std::uint64_t, 3>> canonical_point_bits;
  ExactOrdinaryDiagramClosureRequirements requirements;
  ExactOrdinaryDiagramClosureAudit audit;
  StrictlyPaddedDyadicAabb3Result clipping_box;
  std::vector<ExactOrdinaryCellClosureResult> cells;
  std::vector<ExactOrdinaryDiagramVertex> global_vertices;
  std::vector<ExactOrdinaryDiagramContact> contacts;
  bool all_local_queues_empty_certified{false};
  bool all_cells_full_dimensional_nonempty_certified{false};
  bool global_vertex_occurrence_bijection_certified{false};
  bool natural_incidences_reconciled_certified{false};
  bool artificial_box_boundaries_certified{false};

  friend bool operator==(
      const ExactOrdinaryDiagramClosureResult&,
      const ExactOrdinaryDiagramClosureResult&) = default;
};

struct ExactOrdinaryDiagramClosureVerification {
  bool input_identity_certified{false};
  bool clipping_box_certified{false};
  bool decision_certified{false};
  bool requirements_certified{false};
  bool audit_certified{false};
  bool payload_shape_certified{false};
  bool transcript_replay_certified{false};
  bool all_cells_freshly_verified_certified{false};
  bool all_local_queues_empty_certified{false};
  bool all_cells_full_dimensional_nonempty_certified{false};
  bool global_vertex_occurrence_bijection_certified{false};
  bool natural_incidences_reconciled_certified{false};
  bool artificial_box_boundaries_certified{false};
  bool result_certified{false};
};

// Transactional bounded Phase 8 reference diagram.  Every owner starts from
// the canonical empty proposal, hence from the least exterior fallback when
// one exists.  No cell payload is published unless the aggregate preflight
// covers every local closure and every reconciliation arena.
[[nodiscard]] ExactOrdinaryDiagramClosureResult
build_exact_bounded_ordinary_diagram_closure(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    ExactOrdinaryDiagramClosureBudget budget = {});

[[nodiscard]] ExactOrdinaryDiagramClosureVerification
verify_exact_bounded_ordinary_diagram_closure(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    const ExactOrdinaryDiagramClosureResult& result,
    ExactOrdinaryDiagramClosureBudget budget = {});

}  // namespace morsehgp3d::spatial
