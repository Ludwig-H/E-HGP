#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_forest_root_coverage_index_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_mode =
        "certified_forest_relative_closed_cut_direct_birth_point_union";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_refinement_status =
        "forest_relative_prerequisite_only";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_forest_root_coverage_index_proof_basis =
        "freshly_verified_exact_direct_forest_carrier_cut_every_resolved_"
        "carrier_birth_facet_union_grouped_by_reduced_root_and_bounded_"
        "reachable_forest_structure_audit_v1";

// The work owned by this builder after source authentication has independent
// caps for source scans, facet-key PointId inspections, reachable child edges,
// output records and transient allocations.  The mandatory fresh carrier-cut
// verifier is governed by trusted_cut_budget and currently reconstructs its
// complete source cut, including dense per-birth/per-node scratch.  Therefore
// this API is not a per-batch hot-path session.  Point-reference scratch here
// contains only pairs (root node ID, PointId); it never contains cells,
// cofaces or Gamma.
struct ExactDirectMorseForestRootCoverageIndexBudget {
  std::size_t maximum_cut_entry_scan_count{};
  std::size_t maximum_forest_birth_record_scan_count{};
  std::size_t maximum_forest_node_scan_count{};
  std::size_t maximum_forest_atomic_group_scan_count{};
  std::size_t maximum_forest_batch_scan_count{};
  std::size_t maximum_child_edge_scan_count{};
  std::size_t maximum_facet_key_point_scan_count{};
  std::size_t maximum_point_reference_scan_count{};
  // Sparse ownership records for nodes reachable from referenced roots only.
  std::size_t maximum_node_state_count{};
  std::size_t maximum_depth_first_scratch_count{};
  std::size_t maximum_resolved_carrier_scratch_count{};
  std::size_t maximum_point_reference_scratch_count{};
  std::size_t maximum_referenced_root_count{};
  std::size_t maximum_carrier_root_binding_count{};
  std::size_t maximum_coverage_point_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseForestRootCoverageIndexBudget&,
      const ExactDirectMorseForestRootCoverageIndexBudget&) = default;
};

// Roots are strictly increasing by reduced_root_node_id.  Their point slices
// form a canonical CSR: offsets are contiguous and every slice is nonempty
// and strictly increasing by PointId.  The last binding and the per-binding
// predecessor chain certify each carrier_binding_count in linear time while
// the public binding table remains searchable by component_handle.
struct ExactDirectMorseForestRootCoverage {
  std::size_t root_coverage_index{};
  ExactDirectMorseForestNodeId reduced_root_node_id{};
  std::size_t point_offset{};
  std::size_t point_count{};
  std::size_t carrier_binding_count{};
  std::optional<std::size_t> last_carrier_binding_index;

  friend bool operator==(
      const ExactDirectMorseForestRootCoverage&,
      const ExactDirectMorseForestRootCoverage&) = default;
};

// This table is exhaustive over resolved_reduced_root cut entries and is
// strictly increasing by component_handle.  root_local_binding_index and
// previous_root_binding_index form the canonical predecessor chain named by
// each root.  Latent and inactive carriers do not have a root and therefore
// do not enter this table.
struct ExactDirectMorseForestRootCoverageCarrierBinding {
  std::size_t binding_index{};
  ExactDirectSparseComponentHandle component_handle{};
  std::size_t birth_record_index{};
  std::size_t root_coverage_index{};
  std::size_t root_local_binding_index{};
  std::optional<std::size_t> previous_root_binding_index;

  friend bool operator==(
      const ExactDirectMorseForestRootCoverageCarrierBinding&,
      const ExactDirectMorseForestRootCoverageCarrierBinding&) = default;
};

struct ExactDirectMorseForestRootCoverageIndexCounters {
  std::size_t cut_entry_scan_count{};
  std::size_t forest_birth_record_scan_count{};
  std::size_t forest_node_scan_count{};
  std::size_t forest_atomic_group_scan_count{};
  std::size_t forest_batch_scan_count{};
  std::size_t child_edge_scan_count{};
  std::size_t facet_key_point_scan_count{};
  std::size_t point_reference_scan_count{};
  std::size_t node_state_count{};
  std::size_t resolved_carrier_scratch_count{};
  std::size_t point_reference_scratch_count{};
  std::size_t maximum_depth_first_scratch_count{};
  std::size_t referenced_root_count{};
  std::size_t carrier_root_binding_count{};
  std::size_t coverage_point_count{};

  friend bool operator==(
      const ExactDirectMorseForestRootCoverageIndexCounters&,
      const ExactDirectMorseForestRootCoverageIndexCounters&) = default;
};

enum class ExactDirectMorseForestRootCoverageIndexDecision
    : std::uint8_t {
  not_certified,
  no_coverage_capacity_overflow,
  no_coverage_allocation_failed,
  no_coverage_budget_exhausted,
  no_coverage_source_forest_rejected,
  no_coverage_source_cut_rejected,
  no_coverage_order_or_cut_mismatch,
  no_coverage_replay_contradiction,
  complete_certified_forest_relative_root_point_coverage_index,
};

enum class ExactDirectMorseForestRootCoverageIndexScope
    : std::uint8_t {
  unspecified,
  one_target_order_resolved_direct_forest_roots_at_one_closed_exact_cut_only,
};

struct ExactDirectMorseForestRootCoverageIndexResult {
  static constexpr std::string_view backend =
      direct_morse_forest_root_coverage_index_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_root_coverage_index_profile;
  static constexpr std::string_view mode =
      direct_morse_forest_root_coverage_index_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_forest_root_coverage_index_deployment_status;
  static constexpr std::string_view refinement_status =
      direct_morse_forest_root_coverage_index_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_forest_root_coverage_index_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_root_coverage_index_proof_basis;

  std::uint32_t schema_version{
      direct_morse_forest_root_coverage_index_schema_version};
  ExactDirectMorseForestRootCoverageIndexBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t target_order{};
  exact::ExactLevel closed_squared_level{};
  std::vector<ExactDirectMorseForestRootCoverage> roots;
  std::vector<spatial::PointId> point_ids;
  std::vector<ExactDirectMorseForestRootCoverageCarrierBinding>
      carrier_root_bindings;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseForestRootCoverageIndexCounters counters{};

  bool source_forest_certified_outcome_accepted{false};
  bool source_carrier_cut_index_freshly_verified{false};
  bool target_order_and_closed_cut_match{false};
  bool implicit_order_one_prefix_consumed_through_logical_view{false};
  bool physical_higher_order_suffix_consumed_through_logical_view{false};
  bool every_resolved_carrier_birth_revalidated{false};
  bool every_resolved_carrier_bound_exactly_once{false};
  bool every_referenced_root_revalidated{false};
  bool reachable_node_forest_acyclic_and_unshared{false};
  bool order_one_carrier_birth_nodes_reachable_from_bound_roots{false};
  bool canonical_root_point_csr_reconstructed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool conditional_on_caller_fresh_source_forest_replay{true};
  bool forest_relative_only{true};
  bool strict_pre_batch_alignment_replayed{false};
  bool strict_pre_batch_alignment_claimed{false};
  bool external_locator_authority_replayed{false};
  bool original_geometry_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestRootCoverageIndexDecision decision{
      ExactDirectMorseForestRootCoverageIndexDecision::not_certified};
  ExactDirectMorseForestRootCoverageIndexScope scope{
      ExactDirectMorseForestRootCoverageIndexScope::unspecified};

  [[nodiscard]] const ExactDirectMorseForestRootCoverage* find_root(
      ExactDirectMorseForestNodeId reduced_root_node_id) const noexcept;
  [[nodiscard]] std::span<const spatial::PointId> points_for_root(
      std::size_t root_coverage_index) const noexcept;
  [[nodiscard]]
  const ExactDirectMorseForestRootCoverageCarrierBinding* find_binding(
      ExactDirectSparseComponentHandle component_handle) const noexcept;
  [[nodiscard]] bool certified_forest_relative_root_coverage_index()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestRootCoverageIndexResult&,
      const ExactDirectMorseForestRootCoverageIndexResult&) = default;
};

struct ExactDirectMorseForestRootCoverageIndexVerification {
  bool trusted_source_forest_outcome_accepted{false};
  bool trusted_source_carrier_cut_index_freshly_verified{false};
  bool observed_storage_within_budget{false};
  bool expected_index_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool conditional_on_caller_fresh_source_forest_replay{true};
  bool strict_pre_batch_alignment_replayed{false};
  bool external_locator_authority_replayed{false};
  bool original_geometry_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseForestRootCoverageIndexVerification&,
      const ExactDirectMorseForestRootCoverageIndexVerification&) =
      default;
};

[[nodiscard]] ExactDirectMorseForestRootCoverageIndexResult
build_exact_direct_morse_forest_root_coverage_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_cut_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& source_cut_index,
    const ExactDirectMorseForestRootCoverageIndexBudget& budget);

[[nodiscard]] ExactDirectMorseForestRootCoverageIndexVerification
verify_exact_direct_morse_forest_root_coverage_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_cut_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& source_cut_index,
    const ExactDirectMorseForestRootCoverageIndexBudget& trusted_budget,
    const ExactDirectMorseForestRootCoverageIndexResult& observed);

}  // namespace morsehgp3d::hierarchy
