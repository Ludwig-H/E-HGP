#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

class ExactDirectSparseStableFacetDescentClosureBuilder;

inline constexpr std::uint32_t
    direct_sparse_stable_facet_descent_closure_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_mode =
        "stable_forest_full_key_strict_facet_descent_closure_v1";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_deployment_status =
        "architecture_only_reference_cpu_not_massive_qualified";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_stable_facet_descent_closure_proof_basis =
        "bounded_full_key_stable_forest_probes_fresh_exact_miniballs_"
        "complete_lbvh_top_k_strict_level_functional_forest_v1";

inline constexpr std::size_t
    direct_sparse_stable_facet_descent_closure_maximum_seed_count = 1048576U;
inline constexpr std::size_t
    direct_sparse_stable_facet_descent_closure_maximum_node_count = 1048576U;
inline constexpr std::size_t
    direct_sparse_stable_facet_descent_closure_maximum_step_call_count =
        1048576U;
inline constexpr std::size_t
    direct_sparse_stable_facet_descent_closure_maximum_memo_slot_count =
        2097153U;

struct ExactDirectSparseStableFacetDescentClosureSeed {
  // Physical input order is irrelevant.  The durable identities must be a
  // dense unique permutation of [0, seed_count); output projections are
  // ordered by seed_index.  Duplicate full keys share one interned graph node.
  std::size_t seed_index{};
  ExactDirectSparseFacetKey source_facet_key{};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureSeed&,
      const ExactDirectSparseStableFacetDescentClosureSeed&) = default;
};

struct ExactDirectSparseStableFacetDescentClosureConfig {
  // Scratch-only acceleration for both the node memo and the transient
  // successor-miniball memo.  Zero deliberately forces collisions; every
  // candidate still undergoes a complete-key comparison.
  std::uint64_t memo_fingerprint_mask{~std::uint64_t{0U}};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureConfig&,
      const ExactDirectSparseStableFacetDescentClosureConfig&) = default;
};

struct ExactDirectSparseStableFacetDescentClosureBudget {
  std::size_t maximum_seed_count{};
  std::size_t maximum_node_count{};
  std::size_t maximum_step_call_count{};
  // Per full-key scratch table.  The builder owns exactly two bounded tables
  // of this shape: interned nodes and transient successor miniballs.
  std::size_t maximum_memo_slot_count{};
  // Aggregate cap for positive-key slots, full-key comparisons, handle-index
  // slots and parent hops.  Before every probe the builder conservatively
  // reserves the sum of all four per-probe maxima; a cap short by one fails
  // closed before calling the forest.
  std::size_t maximum_lookup_work_count{};
  ExactDirectSparseStableFacetPositiveProbeBudget positive_probe{};
  spatial::ExactLbvhTopKBudget top_k_query{};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureBudget&,
      const ExactDirectSparseStableFacetDescentClosureBudget&) = default;
};

enum class ExactDirectSparseStableFacetDescentClosureDisposition
    : std::uint8_t {
  not_certified,
  relative_positive,
  unresolved,
  budget_exhausted,
  contradiction,
};

enum class ExactDirectSparseStableFacetDescentClosureDecision : std::uint8_t {
  not_certified,
  complete_empty_seed_set,
  complete_all_seeds_relative_positive,
  complete_all_seeds_with_unresolved_terminals,
  no_preflight_budget_exhausted,
  no_input_shape_rejected,
  no_node_budget_exhausted,
  no_step_call_budget_exhausted,
  no_forest_probe_budget_exhausted,
  no_top_k_budget_exhausted,
  contradiction_forest_probe,
  contradiction_forest_stamp_changed,
  contradiction_exact_geometry,
  contradiction_cycle_or_inconsistent_shared_target,
  no_allocation_failed,
};

enum class ExactDirectSparseStableFacetDescentLocalDecision : std::uint8_t {
  not_certified,
  complete_source_stable_positive_hit,
  complete_unresolved_source_above_closed_batch_level,
  complete_unresolved_source_is_canonical_top_k_choice,
  complete_unresolved_non_strict_canonical_successor,
  complete_strict_successor_unobserved,
  complete_strict_successor_stable_positive_hit,
};

enum class ExactDirectSparseStableFacetDescentNodeKind : std::uint8_t {
  not_certified,
  evaluated_source,
  stable_positive_terminal,
  unresolved_terminal,
};

struct ExactDirectSparseStableFacetPositiveAttachment {
  ExactDirectSparseStableFacetHandle bound_handle{};
  ExactDirectSparseStableFacetHandle root_handle{};
  std::size_t component_size{};

  friend bool operator==(
      const ExactDirectSparseStableFacetPositiveAttachment&,
      const ExactDirectSparseStableFacetPositiveAttachment&) = default;
};

struct ExactDirectSparseStableFacetDescentClosureCounters {
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
  std::size_t memoized_seed_reuse_count{};
  std::size_t memoized_suffix_reuse_count{};
  std::size_t memo_slot_visit_count{};
  std::size_t memo_full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  std::size_t stable_forest_probe_count{};
  std::size_t positive_key_slot_visit_count{};
  std::size_t full_key_comparison_count{};
  std::size_t handle_index_slot_visit_count{};
  std::size_t parent_hop_count{};
  std::size_t aggregate_lookup_work_count{};
  // A first exact build for each authoritative full key, whether its center
  // and level live on an interned node or in the transient successor cache.
  std::size_t distinct_cached_miniball_count{};
  std::size_t source_miniball_build_count{};
  std::size_t source_miniball_reuse_count{};
  std::size_t successor_miniball_build_count{};
  std::size_t successor_miniball_reuse_count{};
  std::size_t top_k_query_count{};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureCounters&,
      const ExactDirectSparseStableFacetDescentClosureCounters&) = default;
};

struct ExactDirectSparseStableFacetDescentNode {
  std::size_t node_index{};
  ExactDirectSparseFacetKey facet_key{};
  std::optional<exact::ExactCenter3> exact_center;
  std::optional<exact::ExactLevel> exact_squared_level;
  std::optional<std::size_t> outgoing_edge_index;
  std::size_t terminal_node_index{};
  std::optional<ExactDirectSparseStableFacetPositiveAttachment>
      stable_positive_attachment;
  ExactDirectSparseStableFacetDescentLocalDecision local_decision{
      ExactDirectSparseStableFacetDescentLocalDecision::not_certified};
  ExactDirectSparseStableFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseStableFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseStableFacetDescentNodeKind kind{
      ExactDirectSparseStableFacetDescentNodeKind::not_certified};
  bool step_evaluated{false};
  bool exact_center_and_level_present{false};
  bool terminal_pointer_certified{false};
  bool full_miniball_not_persisted{false};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentNode&,
      const ExactDirectSparseStableFacetDescentNode&) = default;
};

struct ExactDirectSparseStableFacetDescentEdge {
  std::size_t edge_index{};
  std::size_t source_node_index{};
  std::size_t target_node_index{};
  ExactDirectSparseFacetDescentStepWitness strict_step_witness{};
  bool source_and_target_keys_match_nodes{false};
  bool target_center_and_level_match_node{false};
  bool strict_level_decrease_certified{false};
  bool same_closed_batch_level_certified{false};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentEdge&,
      const ExactDirectSparseStableFacetDescentEdge&) = default;
};

struct ExactDirectSparseStableFacetDescentSeedProjection {
  std::size_t seed_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  std::size_t root_node_index{};
  std::size_t terminal_node_index{};
  ExactDirectSparseStableFacetDescentClosureDisposition closure_disposition{
      ExactDirectSparseStableFacetDescentClosureDisposition::not_certified};
  bool reused_existing_node{false};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentSeedProjection&,
      const ExactDirectSparseStableFacetDescentSeedProjection&) = default;
};

// The graph is immutable behind accessors and carries a private mint, avoiding
// a second deep snapshot of its sparse arenas.  The captured stamp must equal
// the forest stamp after every probe and at exit.  This synchronous builder is
// not a concurrency primitive: callers must externally freeze the forest.
// Equal-level seeds are supported for standalone certification (for example
// AC at its closed level); the future coordinator must supply only its
// contractually strict-below carrier seeds.
class ExactDirectSparseStableFacetDescentClosureResult final {
 public:
  static constexpr std::string_view backend =
      direct_sparse_stable_facet_descent_closure_backend;
  static constexpr std::string_view profile =
      direct_sparse_stable_facet_descent_closure_profile;
  static constexpr std::string_view mode =
      direct_sparse_stable_facet_descent_closure_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_stable_facet_descent_closure_deployment_status;
  static constexpr std::string_view refinement_status =
      direct_sparse_stable_facet_descent_closure_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_stable_facet_descent_closure_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_stable_facet_descent_closure_proof_basis;

  ExactDirectSparseStableFacetDescentClosureResult() noexcept = default;
  ~ExactDirectSparseStableFacetDescentClosureResult() = default;
  ExactDirectSparseStableFacetDescentClosureResult(
      const ExactDirectSparseStableFacetDescentClosureResult&) = default;
  ExactDirectSparseStableFacetDescentClosureResult& operator=(
      const ExactDirectSparseStableFacetDescentClosureResult&) = default;
  ExactDirectSparseStableFacetDescentClosureResult(
      ExactDirectSparseStableFacetDescentClosureResult&&) noexcept = default;
  ExactDirectSparseStableFacetDescentClosureResult& operator=(
      ExactDirectSparseStableFacetDescentClosureResult&&) noexcept = default;

  [[nodiscard]] std::uint32_t schema_version() const noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureConfig&
  config() const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureConfig&
  config() const && = delete;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureBudget&
  requested_budget() const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureBudget&
  requested_budget() const && = delete;
  [[nodiscard]] spatial::LbvhTraversalOrder traversal_order() const noexcept;
  [[nodiscard]] const exact::ExactLevel& closed_batch_squared_level()
      const & noexcept;
  [[nodiscard]] const exact::ExactLevel& closed_batch_squared_level()
      const && = delete;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp& forest_stamp()
      const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp& forest_stamp()
      const && = delete;
  [[nodiscard]] std::size_t common_facet_cardinality() const noexcept;
  [[nodiscard]] std::size_t required_memo_slot_count() const noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentNode> nodes()
      const & noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentNode> nodes()
      const && = delete;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentEdge> edges()
      const & noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentEdge> edges()
      const && = delete;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentSeedProjection>
  seed_projections() const & noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseStableFacetDescentSeedProjection>
  seed_projections() const && = delete;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureCounters&
  counters() const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetDescentClosureCounters&
  counters() const && = delete;
  [[nodiscard]] ExactDirectSparseStableFacetDescentClosureDisposition
  disposition() const noexcept;
  [[nodiscard]] ExactDirectSparseStableFacetDescentClosureDecision decision()
      const noexcept;

  [[nodiscard]] bool certified_complete_relative_positive_closure()
      const noexcept;
  [[nodiscard]] bool certified_complete_with_unresolved_terminals()
      const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;
  [[nodiscard]] bool certified_fail_closed_contradiction() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;
  [[nodiscard]] bool certified_for(
      const ExactDirectSparseStableFacetForestStamp&) const noexcept;
  [[nodiscard]] bool certified_for(
      const ExactDirectSparseStableFacetForest&) const noexcept;

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureResult&,
      const ExactDirectSparseStableFacetDescentClosureResult&) = default;

 private:
  std::uint32_t schema_version_{
      direct_sparse_stable_facet_descent_closure_schema_version};
  ExactDirectSparseStableFacetDescentClosureConfig config_{};
  ExactDirectSparseStableFacetDescentClosureBudget requested_budget_{};
  spatial::LbvhTraversalOrder traversal_order_{
      spatial::LbvhTraversalOrder::near_first};
  exact::ExactLevel closed_batch_squared_level_{};
  ExactDirectSparseStableFacetForestStamp forest_stamp_{};
  std::size_t common_facet_cardinality_{};
  std::size_t required_memo_slot_count_{};
  std::vector<ExactDirectSparseStableFacetDescentNode> nodes_;
  std::vector<ExactDirectSparseStableFacetDescentEdge> edges_;
  std::vector<ExactDirectSparseStableFacetDescentSeedProjection>
      seed_projections_;
  ExactDirectSparseStableFacetDescentClosureCounters counters_{};
  bool trusted_authorities_certified_{false};
  bool input_shape_certified_{false};
  bool budget_preflight_completed_{false};
  bool budget_preflight_satisfied_{false};
  bool common_forest_stamp_certified_{false};
  bool every_memo_candidate_compared_by_full_key_{false};
  bool strict_functional_graph_certified_{false};
  bool exact_level_acyclicity_certified_{false};
  bool edge_node_terminal_identity_certified_{false};
  bool all_seed_references_processed_{false};
  bool no_partial_graph_published_{false};
  bool no_top_k_partition_or_shell_persisted_{false};
  bool forest_state_mutated_{false};
  bool global_closed_ball_materialized_{false};
  bool forbidden_global_structure_materialized_{false};
  bool source_exactness_claimed_{false};
  bool hierarchy_attachment_published_{false};
  bool public_status_claimed_{false};
  ExactDirectSparseStableFacetDescentClosureDisposition disposition_{
      ExactDirectSparseStableFacetDescentClosureDisposition::not_certified};
  ExactDirectSparseStableFacetDescentClosureDecision decision_{
      ExactDirectSparseStableFacetDescentClosureDecision::not_certified};
  bool minted_{false};

  [[nodiscard]] bool certified_common() const noexcept;
  [[nodiscard]] bool certified_graph() const noexcept;

  friend ExactDirectSparseStableFacetDescentClosureResult
  build_exact_direct_sparse_stable_facet_descent_closure(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      std::span<const ExactDirectSparseStableFacetDescentClosureSeed>,
      const exact::ExactLevel&,
      const ExactDirectSparseStableFacetForest&,
      const ExactDirectSparseStableFacetDescentClosureBudget&,
      const ExactDirectSparseStableFacetDescentClosureConfig&,
      spatial::LbvhTraversalOrder);
  friend class ExactDirectSparseStableFacetDescentClosureBuilder;
};

struct ExactDirectSparseStableFacetDescentClosureVerification {
  bool trusted_inputs_certified{false};
  bool observed_stamp_matches_forest{false};
  bool fresh_replay_completed{false};
  bool fresh_replay_equal_to_observed{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseStableFacetDescentClosureVerification&,
      const ExactDirectSparseStableFacetDescentClosureVerification&) =
      default;
};

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureResult
build_exact_direct_sparse_stable_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    const ExactDirectSparseStableFacetDescentClosureConfig& config = {},
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// The observed result never steers the replay.  The verifier rebuilds from
// the trusted cloud, LBVH, seeds, level, live forest, budget and config, then
// compares the entire immutable result and requires the same forest stamp.
[[nodiscard]] ExactDirectSparseStableFacetDescentClosureVerification
verify_exact_direct_sparse_stable_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    const ExactDirectSparseStableFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseStableFacetDescentClosureResult& observed);

}  // namespace morsehgp3d::hierarchy
