#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_forest_carrier_cut_closure_adapter_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_closure_adapter_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_closure_adapter_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_closure_adapter_mode =
        "certified_epoch_checked_direct_sparse_facet_descent_closure_adapter";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_closure_adapter_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_closure_adapter_proof_basis =
        "caller_preserved_canonical_point_namespace_live_morse_forest_"
        "carrier_cut_epoch_and_frozen_locator_snapshot_"
        "single_10_5c_canonical_distinct_key_closure_compacted_to_terminal_"
        "carrier_summaries_before_transient_graph_destruction_v1";

// The closure budget bounds the transient 10.5c graph.  The remaining caps
// bound the adapter's scan, compact terminal projection and logical output.
// Point references count both each retained source key and its retained
// terminal key.
struct ExactDirectMorseForestCarrierCutClosureAdapterBudget {
  std::size_t maximum_canonical_key_scan_count{};
  std::size_t maximum_terminal_summary_count{};
  std::size_t maximum_terminal_key_point_reference_count{};
  std::size_t maximum_carrier_cut_lookup_count{};
  std::size_t maximum_logical_output_entry_count{};
  ExactDirectSparseFacetDescentClosureBudget closure_budget{};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutClosureAdapterBudget&,
      const ExactDirectMorseForestCarrierCutClosureAdapterBudget&) =
      default;
};

enum class ExactDirectMorseForestCarrierCutClosureTerminalDisposition
    : std::uint8_t {
  not_certified,
  closure_unresolved,
  positive_active_latent,
  positive_resolved_reduced_root,
};

struct ExactDirectMorseForestCarrierCutClosureTerminalSummary {
  std::size_t seed_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFacetKey terminal_facet_key{};
  std::optional<ExactDirectSparseComponentHandle>
      canonical_component_handle;
  std::optional<ExactDirectSparseFacetWitness> binding_witness;
  std::optional<ExactDirectMorseForestCarrierCutEntry> carrier_cut_entry;
  ExactDirectSparseFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  ExactDirectMorseForestCarrierCutClosureTerminalDisposition disposition{
      ExactDirectMorseForestCarrierCutClosureTerminalDisposition::
          not_certified};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutClosureTerminalSummary&,
      const ExactDirectMorseForestCarrierCutClosureTerminalSummary&) =
      default;
};

struct ExactDirectMorseForestCarrierCutClosureAdapterAudit {
  std::size_t canonical_key_scan_count{};
  std::size_t carrier_cut_lookup_count{};
  std::size_t closure_build_attempt_count{};
  std::size_t closure_build_completion_count{};
  std::size_t transient_node_count{};
  std::size_t transient_edge_count{};
  std::size_t transient_seed_projection_count{};
  std::size_t terminal_summary_count{};
  std::size_t retained_terminal_key_point_reference_count{};
  std::size_t retained_logical_output_entry_count{};
  bool view_epoch_checked_at_entry{false};
  bool cloud_point_count_matches_source_forest{false};
  bool matching_canonical_point_namespace_required{true};
  bool forest_to_cloud_namespace_identity_certified{false};
  bool target_order_bound_to_common_key_cardinality{false};
  bool canonical_distinct_input_revalidated{false};
  bool output_preallocated_before_closure{false};
  bool exactly_one_closure_built{false};
  bool closure_partial_refinement_outcome_accepted{false};
  bool closure_metadata_matches_live_cut{false};
  bool entry_and_post_locator_stamps_equal{false};
  bool pre_result_and_post_locator_stamps_equal{false};
  bool every_seed_projection_summarized_once{false};
  bool every_positive_terminal_mapped_to_active_target_carrier{false};
  bool terminal_summaries_sorted_unique_by_seed_index{false};
  bool transient_closure_graph_destroyed_before_return{false};
  bool no_closure_constructed_on_preflight_rejection{false};
  bool closure_nodes_edges_or_projections_persisted{false};
  bool locator_state_mutated{false};
  bool locator_batch_committed{false};
  bool session_poisoned{false};
  bool external_binding_authority_replayed{false};
  bool forest_relative_only{true};
  bool facet_coface_cell_gamma_or_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutClosureAdapterAudit&,
      const ExactDirectMorseForestCarrierCutClosureAdapterAudit&) =
      default;
};

enum class ExactDirectMorseForestCarrierCutClosureAdapterDecision
    : std::uint8_t {
  not_certified,
  no_adapter_session_not_ready,
  no_adapter_session_poisoned,
  no_adapter_stale_view_epoch,
  no_adapter_locator_snapshot_changed,
  no_adapter_capacity_overflow,
  no_adapter_allocation_failed,
  no_adapter_budget_exhausted,
  no_adapter_input_shape_rejected,
  no_adapter_closure_outcome_rejected,
  no_adapter_closure_budget_exhausted,
  no_adapter_closure_contradiction,
  no_adapter_terminal_projection_contradiction,
  complete_empty_key_set,
  complete_all_positive_terminal_summaries,
  complete_with_unresolved_terminal_summaries,
};

enum class ExactDirectMorseForestCarrierCutClosureAdapterScope
    : std::uint8_t {
  unspecified,
  one_live_carrier_cut_epoch_one_transient_10_5c_closure_to_compact_terminal_carrier_summaries_only,
};

// This result deliberately has no closure nodes, edges, seed projections,
// miniballs or locator reference.  A compact contradiction witness is the
// only diagnostic payload retained on a closure contradiction.
struct ExactDirectMorseForestCarrierCutClosureAdapterResult {
  static constexpr std::string_view backend =
      direct_morse_forest_carrier_cut_closure_adapter_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_carrier_cut_closure_adapter_profile;
  static constexpr std::string_view mode =
      direct_morse_forest_carrier_cut_closure_adapter_mode;
  static constexpr std::string_view public_status =
      direct_morse_forest_carrier_cut_closure_adapter_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_carrier_cut_closure_adapter_proof_basis;

  std::uint32_t schema_version{
      direct_morse_forest_carrier_cut_closure_adapter_schema_version};
  ExactDirectMorseForestCarrierCutClosureAdapterBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t target_order{};
  exact::ExactLevel closed_squared_level{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t common_facet_cardinality{};
  std::size_t required_terminal_summary_count{};
  std::size_t required_terminal_key_point_reference_count{};
  std::size_t required_logical_output_entry_count{};
  std::size_t required_memo_slot_count{};
  ExactDirectSparseFacetDescentClosureCounters closure_counters{};
  ExactDirectSparseFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseFacetDescentClosureDecision closure_decision{
      ExactDirectSparseFacetDescentClosureDecision::not_certified};
  std::vector<ExactDirectMorseForestCarrierCutClosureTerminalSummary>
      terminal_summaries;
  std::optional<ExactDirectSparseFacetDescentContradictionWitness>
      contradiction_witness;
  ExactDirectMorseForestCarrierCutClosureAdapterAudit audit{};
  ExactDirectMorseForestCarrierCutClosureAdapterDecision decision{
      ExactDirectMorseForestCarrierCutClosureAdapterDecision::not_certified};
  ExactDirectMorseForestCarrierCutClosureAdapterScope scope{
      ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified};

  [[nodiscard]] bool certified_compact_closure_summary() const noexcept;
  [[nodiscard]] bool certified_atomic_rejection() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutClosureAdapterResult&,
      const ExactDirectMorseForestCarrierCutClosureAdapterResult&) =
      default;
};

}  // namespace morsehgp3d::hierarchy
