#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_facet_descent_closure_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_mode = "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_closure_proof_basis =
        "fresh_10_5b_core_steps_full_facet_key_interning_exact_level_"
        "acyclic_suffix_sharing_common_frozen_locator_snapshot_v1";

inline constexpr std::size_t
    direct_sparse_facet_descent_closure_maximum_seed_count = 1048576U;
inline constexpr std::size_t
    direct_sparse_facet_descent_closure_maximum_node_count = 1048576U;
inline constexpr std::size_t
    direct_sparse_facet_descent_closure_maximum_step_call_count = 1048576U;
inline constexpr std::size_t
    direct_sparse_facet_descent_closure_maximum_memo_slot_count = 2097153U;

// seed_index is a durable caller identity, not a position in the supplied
// span.  The indices must be exactly [0, seed_count), but the records may be
// supplied in any order.  Repeated facet keys remain distinct seed
// references and share one interned graph node.
struct ExactDirectSparseFacetDescentClosureSeed {
  std::size_t seed_index{};
  ExactDirectSparseFacetKey source_facet_key{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureSeed&,
      const ExactDirectSparseFacetDescentClosureSeed&) = default;
};

// The memo table is scratch.  It is provisioned below one-half load by the
// explicit slot cap, and its fingerprint is never authoritative.  A zero mask
// is supported for collision fixtures; every candidate is still compared by
// its complete facet key.
struct ExactDirectSparseFacetDescentClosureConfig {
  std::uint64_t memo_fingerprint_mask{~std::uint64_t{0U}};

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureConfig&,
      const ExactDirectSparseFacetDescentClosureConfig&) = default;
};

struct ExactDirectSparseFacetDescentClosureBudget {
  std::size_t maximum_seed_count{};
  std::size_t maximum_node_count{};
  std::size_t maximum_step_call_count{};
  std::size_t maximum_memo_slot_count{};
  ExactDirectSparseFacetDescentStepBudget step_budget{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureBudget&,
      const ExactDirectSparseFacetDescentClosureBudget&) = default;
};

enum class ExactDirectSparseFacetDescentClosureDisposition : std::uint8_t {
  not_certified,
  relative_positive,
  unresolved,
  budget_exhausted,
  contradiction,
};

enum class ExactDirectSparseFacetDescentClosureDecision : std::uint8_t {
  not_certified,
  complete_empty_seed_set,
  complete_all_seeds_relative_positive,
  complete_all_seeds_with_unresolved_terminals,
  no_closure_preflight_budget_exhausted,
  certified_prefix_node_budget_exhausted,
  certified_prefix_step_call_budget_exhausted,
  certified_prefix_step_budget_exhausted,
  contradiction_certified_local_step,
  contradiction_cycle_or_incompatible_shared_target,
};

enum class ExactDirectSparseFacetDescentNodeKind : std::uint8_t {
  not_certified,
  evaluated_step_source,
  positive_locator_terminal,
  graph_budget_terminal,
};

enum class ExactDirectSparseFacetDescentClosureScope : std::uint8_t {
  unspecified,
  bounded_multi_source_memoized_strict_functional_forest_relative_positive_sink_propagation_only,
};

struct ExactDirectSparseFacetDescentClosureCounters {
  std::size_t input_seed_reference_count{};
  std::size_t processed_seed_reference_count{};
  std::size_t distinct_seed_key_count{};
  std::size_t duplicate_seed_key_reference_count{};
  std::size_t interned_node_count{};
  std::size_t evaluated_step_source_count{};
  std::size_t strict_edge_count{};
  std::size_t terminal_node_count{};
  std::size_t relative_positive_terminal_count{};
  std::size_t unresolved_terminal_count{};
  std::size_t budget_terminal_count{};
  std::size_t source_positive_hit_count{};
  std::size_t successor_positive_hit_count{};
  std::size_t memoized_seed_reuse_count{};
  std::size_t memoized_suffix_reuse_count{};
  std::size_t diagnostic_strict_witness_without_edge_count{};
  std::size_t memo_slot_visit_count{};
  std::size_t memo_full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  std::size_t locator_snapshot_check_count{};
  std::size_t distinct_cached_miniball_count{};
  std::size_t source_miniball_build_count{};
  std::size_t source_miniball_reuse_count{};
  std::size_t successor_miniball_build_count{};
  std::size_t successor_miniball_reuse_count{};
  ExactDirectSparseFacetDescentStepCounters aggregate_step_counters{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureCounters&,
      const ExactDirectSparseFacetDescentClosureCounters&) = default;
};

// A node persists only a fixed-width key, an optional exact center and level,
// one local 10.5b projection, one outgoing edge reference at most, its terminal
// pointer and an optional relative-positive binding.  Full miniballs remain
// transient build scratch and top-k partitions never enter this type.
struct ExactDirectSparseFacetDescentNode {
  std::size_t node_index{};
  ExactDirectSparseFacetKey facet_key{};
  std::optional<exact::ExactCenter3> exact_center;
  std::optional<exact::ExactLevel> exact_squared_level;
  std::optional<std::size_t> outgoing_edge_index;
  std::size_t terminal_node_index{};
  std::optional<ExactDirectSparseFacetDescentStepWitness>
      diagnostic_strict_step_witness;
  std::optional<ExactDirectSparseComponentHandle> resolved_component_handle;
  std::optional<ExactDirectSparseFacetWitness> resolved_binding_witness;
  ExactDirectSparseFacetDescentStepDisposition local_step_disposition{
      ExactDirectSparseFacetDescentStepDisposition::not_certified};
  ExactDirectSparseFacetDescentStepDecision local_step_decision{
      ExactDirectSparseFacetDescentStepDecision::not_certified};
  ExactDirectSparseFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseFacetDescentNodeKind kind{
      ExactDirectSparseFacetDescentNodeKind::not_certified};
  bool step_evaluated{false};
  bool exact_center_and_level_present{false};
  bool local_step_projection_certified{false};
  bool terminal_pointer_certified{false};
  bool full_miniball_not_persisted{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentNode&,
      const ExactDirectSparseFacetDescentNode&) = default;
};

struct ExactDirectSparseFacetDescentEdge {
  std::size_t edge_index{};
  std::size_t source_node_index{};
  std::size_t target_node_index{};
  ExactDirectSparseFacetDescentStepWitness strict_step_witness{};
  bool source_and_target_keys_match_nodes{false};
  bool target_center_and_level_match_node{false};
  bool strict_level_decrease_certified{false};
  bool same_closed_batch_level_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentEdge&,
      const ExactDirectSparseFacetDescentEdge&) = default;
};

struct ExactDirectSparseFacetDescentSeedProjection {
  std::size_t seed_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  std::size_t root_node_index{};
  std::size_t terminal_node_index{};
  ExactDirectSparseFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  bool reused_existing_node{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentSeedProjection&,
      const ExactDirectSparseFacetDescentSeedProjection&) = default;
};

struct ExactDirectSparseFacetDescentContradictionWitness {
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFacetKey target_facet_key{};
  ExactDirectSparseFacetDescentStepDecision local_step_decision{
      ExactDirectSparseFacetDescentStepDecision::not_certified};
  bool visiting_cycle_detected{false};
  bool shared_target_geometry_mismatch{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentContradictionWitness&,
      const ExactDirectSparseFacetDescentContradictionWitness&) = default;
};

struct ExactDirectSparseFacetDescentClosureResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_closure_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_closure_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_closure_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_facet_descent_closure_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_closure_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_closure_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_closure_schema_version};
  ExactDirectSparseFacetDescentClosureConfig config{};
  ExactDirectSparseFacetDescentClosureBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  exact::ExactLevel closed_batch_squared_level{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t common_facet_cardinality{};
  std::size_t required_memo_slot_count{};
  std::vector<ExactDirectSparseFacetDescentNode> nodes;
  std::vector<ExactDirectSparseFacetDescentEdge> edges;
  std::vector<ExactDirectSparseFacetDescentSeedProjection> seed_projections;
  std::optional<ExactDirectSparseFacetDescentContradictionWitness>
      contradiction_witness;
  ExactDirectSparseFacetDescentClosureCounters counters{};
  bool trusted_authorities_certified{false};
  bool input_shape_certified{false};
  bool budget_preflight_completed{false};
  bool budget_preflight_satisfied{false};
  bool common_locator_snapshot_certified{false};
  bool every_memo_fingerprint_candidate_compared_by_full_key{false};
  bool every_distinct_evaluated_key_called_step_core_once{false};
  bool cached_miniballs_reused_at_exact_seams{false};
  bool strict_functional_graph_certified{false};
  bool exact_level_acyclicity_certified{false};
  bool edge_node_terminal_identity_certified{false};
  bool all_seed_references_processed{false};
  bool no_half_edge_published{false};
  bool no_top_k_partition_or_shell_persisted{false};
  bool locator_state_mutated{false};
  bool locator_batch_committed{false};
  bool external_binding_authority_replayed{false};
  bool missing_facet_means_isolated{false};
  bool singleton_component_created{false};
  bool hierarchy_attachment_published{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseFacetDescentClosureDisposition disposition{
      ExactDirectSparseFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseFacetDescentClosureDecision decision{
      ExactDirectSparseFacetDescentClosureDecision::not_certified};
  ExactDirectSparseFacetDescentClosureScope scope{
      ExactDirectSparseFacetDescentClosureScope::unspecified};

  [[nodiscard]] bool certified_complete_relative_positive_closure()
      const noexcept;
  [[nodiscard]] bool certified_complete_with_unresolved_terminals()
      const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;
  [[nodiscard]] bool certified_fail_closed_contradiction() const noexcept;
  [[nodiscard]] bool certified_partial_refinement_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureResult&,
      const ExactDirectSparseFacetDescentClosureResult&) = default;
};

struct ExactDirectSparseFacetDescentClosureVerification {
  bool trusted_inputs_certified{false};
  bool observed_storage_within_budget{false};
  bool locator_snapshot_matches_observed_build{false};
  bool observed_outcome_well_formed{false};
  bool seeds_freshly_canonicalized{false};
  bool memoized_graph_freshly_replayed{false};
  bool local_step_projections_freshly_replayed{false};
  bool strict_edges_and_seams_freshly_replayed{false};
  bool functional_forest_cardinality_certified{false};
  bool terminal_dispositions_freshly_propagated{false};
  bool no_duplicate_top_k_or_miniball_work_certified{false};
  bool no_locator_mutation_or_batch_commit{false};
  bool external_binding_authority_replayed{false};
  bool no_isolation_singleton_or_attachment_invented{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentClosureVerification&,
      const ExactDirectSparseFacetDescentClosureVerification&) = default;
};

// The locator must remain externally frozen for the whole build.  Snapshot
// comparisons detect sequential mutations between calls but are not a thread
// synchronization mechanism and cannot make a concurrent writer safe.
[[nodiscard]] ExactDirectSparseFacetDescentClosureResult
build_exact_direct_sparse_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config = {},
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// The same external locator freeze is required throughout fresh replay.
[[nodiscard]] ExactDirectSparseFacetDescentClosureVerification
verify_exact_direct_sparse_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetDescentClosureResult& observed);

}  // namespace morsehgp3d::hierarchy
