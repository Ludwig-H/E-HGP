#pragma once

#include "morsehgp3d/hierarchy/boruvka.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class K1BoruvkaCandidateContextState;
class K1BoruvkaCandidateHostState;
}  // namespace detail

struct K1BoruvkaSeed {
  spatial::PointId source_point_id{};
  spatial::PointId target_point_id{};
  exact::ExactLevel exact_squared_cutoff{};

  friend bool operator==(const K1BoruvkaSeed&, const K1BoruvkaSeed&) = default;
};

struct K1BoruvkaCandidate {
  spatial::PointId source_point_id{};
  spatial::PointId target_point_id{};

  friend bool operator==(
      const K1BoruvkaCandidate&, const K1BoruvkaCandidate&) = default;
};

struct K1BoruvkaCandidateAudit {
  static constexpr const char* proposal_semantics =
      "gpu_stackless_lbvh_fixed_seed_candidate_superset";
  static constexpr const char* decision_semantics =
      "cpu_exact_seed_replay_and_kappa_resolution";

  std::size_t resident_point_count{};
  std::size_t resident_node_count{};
  std::size_t frozen_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};
  std::size_t exact_seed_count{};
  std::size_t gpu_candidate_count{};
  // Logical CSR extent. In chunked mode this is not an allocated capacity;
  // physical device/host high-water marks live in the emission audit.
  std::size_t gpu_output_capacity{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  std::size_t gpu_count_pass_node_visit_count{};
  std::size_t gpu_emit_pass_node_visit_count{};
  std::size_t gpu_uniform_component_prune_count{};
  std::size_t gpu_strict_aabb_prune_count{};
  std::size_t gpu_invalid_bound_descent_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_required_candidate_count{};
  std::size_t cpu_exact_candidate_distance_evaluation_count{};
  std::uint64_t buffer_epoch{};
  // Deterministic producer/replay diagnostic; no exactness certificate relies
  // on collision resistance or on trusting this digest alone.
  std::uint64_t proposal_digest_fnv1a{};
  bool frozen_labels_certified{false};
  bool rope_topology_certified{false};
  bool exact_capacity_certified{false};
  bool no_truncation_certified{false};
  bool candidate_superset_certified{false};
  bool cpu_exact_resolution_complete{false};

  friend bool operator==(
      const K1BoruvkaCandidateAudit&, const K1BoruvkaCandidateAudit&) = default;
};

// The GPU output is a proposal only. candidate_offsets groups candidates by
// source PointId; the CPU-certified component minima are a separate field and
// no contraction or public MorseHGP3D status is performed by this API.
struct K1BoruvkaRoundProposal {
  std::vector<spatial::PointId> frozen_component_labels;
  std::vector<K1BoruvkaSeed> seeds;
  std::vector<std::size_t> candidate_offsets;
  std::vector<K1BoruvkaCandidate> candidates;
  std::vector<hierarchy::K1BoruvkaComponentMinimum>
      cpu_exact_component_minima;
  K1BoruvkaCandidateAudit audit;
};

struct K1BoruvkaChunkingPolicy {
  std::size_t max_candidate_records_per_chunk{};

  friend bool operator==(
      const K1BoruvkaChunkingPolicy&,
      const K1BoruvkaChunkingPolicy&) = default;
};

struct K1BoruvkaMortonSeedPolicy {
  std::size_t window_radius{};

  friend bool operator==(
      const K1BoruvkaMortonSeedPolicy&,
      const K1BoruvkaMortonSeedPolicy&) = default;
};

enum class K1BoruvkaSeedMode : std::uint8_t {
  canonical_external_fallback,
  gpu_morton_window_cpu_exact_monotone,
};

enum class K1BoruvkaSeedStatus : std::uint8_t {
  not_certified,
  bounded_morton_window_external_exact_monotone_certified,
};

struct K1BoruvkaMortonSeedAudit {
  std::size_t source_count{};
  std::size_t window_radius{};
  std::size_t neighbor_inspection_budget_per_source{};
  std::size_t maximum_inspected_neighbor_count_per_source{};
  std::size_t inspected_neighbor_count{};
  std::size_t external_neighbor_count{};
  std::size_t floating_proposal_count{};
  std::size_t exact_selected_proposal_count{};
  std::size_t exact_strict_improvement_count{};
  std::size_t exact_fallback_count{};
  std::size_t exact_seed_distance_evaluation_count{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  bool complete_source_coverage_certified{false};
  bool bounded_window_certified{false};
  bool external_targets_recertified{false};
  bool exact_monotone_cutoff_certified{false};

  friend bool operator==(
      const K1BoruvkaMortonSeedAudit&,
      const K1BoruvkaMortonSeedAudit&) = default;
};

struct K1BoruvkaPointMinimum {
  spatial::PointId source_point_id{};
  hierarchy::ExactEmstEdge outgoing_edge{};

  friend bool operator==(
      const K1BoruvkaPointMinimum&,
      const K1BoruvkaPointMinimum&) = default;
};

enum class K1BoruvkaExactSearchStatus : std::uint8_t {
  not_certified,
  exact_external_1nn_branch_and_bound_certified,
};

struct K1BoruvkaExactSearchAudit {
  static constexpr const char* proposal_semantics =
      "gpu_bounded_morton_seed_only";
  static constexpr const char* decision_semantics =
      "cpu_exact_external_1nn_lbvh_branch_and_bound";
  static constexpr const char* proof_basis =
      "strict_exact_point_aabb_frontier_exhaustion_v1";

  std::size_t resident_point_count{};
  std::size_t resident_node_count{};
  std::size_t frozen_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};
  std::size_t point_query_count{};
  std::size_t seed_incumbent_count{};
  std::size_t point_minimum_count{};
  std::size_t component_minimum_count{};
  std::size_t maximum_cpu_node_visit_count_per_source{};
  std::size_t maximum_cpu_exact_point_distance_evaluation_count_per_source{};
  std::size_t maximum_cpu_frontier_size_per_source{};
  std::size_t cpu_node_visit_count{};
  std::size_t cpu_internal_node_expansion_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_exact_point_distance_evaluation_count{};
  std::size_t cpu_seed_leaf_distance_reuse_count{};
  std::size_t cpu_uniform_component_prune_count{};
  std::size_t cpu_strict_aabb_prune_count{};
  bool frozen_labels_certified{false};
  bool lbvh_topology_and_exact_aabbs_certified{false};
  bool complete_source_seed_coverage_certified{false};
  bool external_seed_targets_recertified{false};
  bool exact_seed_cutoffs_recertified{false};
  bool uniform_component_prunes_certified{false};
  bool strict_only_aabb_pruning_certified{false};
  bool complete_frontier_exhaustion_certified{false};
  bool canonical_kappa_resolution_certified{false};
  bool point_minima_complete{false};
  bool component_minima_complete{false};

  friend bool operator==(
      const K1BoruvkaExactSearchAudit&,
      const K1BoruvkaExactSearchAudit&) = default;
};

// The GPU contributes only one bounded Morton seed proposal per source. The
// CPU recertifies that incumbent, exhausts an exact best-first point-to-LBVH
// frontier, and prunes solely uniform internal nodes or nodes whose exact AABB
// lower bound is strictly larger than the current exact edge. Equal bounds are
// always descended so the canonical (squared_length, u, v) order is preserved.
// The result contains one exact edge per source and one reduction per frozen
// component; it performs neither contraction nor hierarchy publication.
struct K1BoruvkaSeededExactRoundResolution {
  std::vector<K1BoruvkaPointMinimum> point_minima;
  std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaExactSearchAudit search_audit;
  K1BoruvkaSeedStatus seed_status{K1BoruvkaSeedStatus::not_certified};
  K1BoruvkaExactSearchStatus search_status{
      K1BoruvkaExactSearchStatus::not_certified};

  friend bool operator==(
      const K1BoruvkaSeededExactRoundResolution&,
      const K1BoruvkaSeededExactRoundResolution&) = default;
};

enum class K1BoruvkaDualTreeSearchStatus : std::uint8_t {
  not_certified,
  exact_external_1nn_shared_lbvh_dual_tree_certified,
};

struct K1BoruvkaDualTreeSearchAudit {
  static constexpr const char* proposal_semantics =
      "gpu_bounded_morton_seed_only";
  static constexpr const char* decision_semantics =
      "cpu_exact_external_1nn_shared_lbvh_dual_tree";
  static constexpr const char* proof_basis =
      "strict_exact_aabb_pair_dynamic_incumbent_bounded_dfs_exhaustion_v3";

  std::size_t resident_point_count{};
  std::size_t resident_node_count{};
  std::size_t frozen_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};
  std::size_t seed_incumbent_count{};
  std::size_t dynamic_incumbent_node_count{};
  std::size_t point_minimum_count{};
  std::size_t component_minimum_count{};
  std::size_t unordered_point_pair_count{};
  std::size_t covered_unordered_point_pair_count{};
  std::size_t lbvh_maximum_depth{};
  std::size_t certified_depth_first_frontier_bound{};
  std::size_t certified_node_pair_visit_bound{};
  std::size_t maximum_cpu_frontier_size{};
  std::size_t cpu_node_pair_visit_count{};
  std::size_t cpu_node_pair_expansion_count{};
  std::size_t cpu_exact_aabb_pair_bound_evaluation_count{};
  std::size_t cpu_exact_point_pair_distance_evaluation_count{};
  std::size_t cpu_strict_incumbent_decrease_count{};
  std::size_t cpu_incumbent_ancestor_update_count{};
  std::size_t cpu_uniform_same_component_pair_prune_count{};
  std::size_t cpu_strict_aabb_pair_prune_count{};
  bool frozen_labels_certified{false};
  bool lbvh_topology_and_exact_aabbs_certified{false};
  bool complete_source_seed_coverage_certified{false};
  bool external_seed_targets_recertified{false};
  bool exact_seed_cutoffs_recertified{false};
  bool dynamic_incumbent_tree_certified{false};
  bool canonical_unordered_pair_partition_certified{false};
  bool uniform_component_pair_prunes_certified{false};
  bool strict_only_aabb_pair_pruning_certified{false};
  bool depth_first_frontier_bound_certified{false};
  bool node_pair_visit_bound_certified{false};
  bool complete_frontier_exhaustion_certified{false};
  bool canonical_kappa_resolution_certified{false};
  bool point_minima_complete{false};
  bool component_minima_complete{false};

  friend bool operator==(
      const K1BoruvkaDualTreeSearchAudit&,
      const K1BoruvkaDualTreeSearchAudit&) = default;
};

// The certified Morton proposal initializes one exact incumbent per point.
// A shared locally-near-first depth-first traversal partitions every unordered
// pair of LBVH leaves exactly once. Its logical stack contains at most twice
// the certified LBVH depth plus one entry and it visits at most n(n+1)-1 node
// pairs. A node pair is discarded only when
// both nodes are uniform in the same component or when its exact AABB--AABB
// lower bound is strictly larger than every current exact point incumbent
// stored below the two nodes. Incumbents only decrease from their certified
// seeds and equal bounds are always descended. The result preserves exact
// point and component minima; it performs neither contraction nor hierarchy
// publication.
struct K1BoruvkaSeededDualTreeRoundResolution {
  std::vector<K1BoruvkaPointMinimum> point_minima;
  std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaDualTreeSearchAudit search_audit;
  K1BoruvkaSeedStatus seed_status{K1BoruvkaSeedStatus::not_certified};
  K1BoruvkaDualTreeSearchStatus search_status{
      K1BoruvkaDualTreeSearchStatus::not_certified};

  friend bool operator==(
      const K1BoruvkaSeededDualTreeRoundResolution&,
      const K1BoruvkaSeededDualTreeRoundResolution&) = default;
};

enum class K1BoruvkaComponentDualTreeSearchStatus : std::uint8_t {
  not_certified,
  exact_component_minima_shared_lbvh_dual_tree_certified,
};

// This audit belongs to the component-direct resolver. Every recertified point
// seed is offered to both incident components before the frozen envelope is
// built, but no per-point minimum is materialized.
struct K1BoruvkaComponentDualTreeSearchAudit {
  static constexpr const char* proposal_semantics =
      "gpu_bounded_morton_seed_only";
  static constexpr const char* decision_semantics =
      "cpu_exact_component_minima_shared_lbvh_dual_tree";
  static constexpr const char* proof_basis =
      "strict_exact_aabb_pair_bidirectional_component_seed_frozen_upper_envelope_bounded_dfs_v2";

  std::size_t resident_point_count{};
  std::size_t resident_node_count{};
  std::size_t frozen_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};
  // There are n source-oriented seeds and n additional target-component
  // offers. The latter reuse the already recertified exact seed distances.
  std::size_t point_seed_count{};
  std::size_t component_seed_incumbent_count{};
  std::size_t target_component_seed_offer_count{};
  std::size_t target_component_seed_kappa_update_count{};
  std::size_t target_component_seed_strict_cutoff_decrease_count{};
  std::size_t component_cutoff_upper_envelope_node_count{};
  std::size_t component_minimum_count{};
  std::size_t unordered_point_pair_count{};
  std::size_t covered_unordered_point_pair_count{};
  std::size_t lbvh_maximum_depth{};
  std::size_t certified_depth_first_frontier_bound{};
  std::size_t certified_node_pair_visit_bound{};
  std::size_t maximum_cpu_frontier_size{};
  std::size_t cpu_node_pair_visit_count{};
  std::size_t cpu_node_pair_expansion_count{};
  std::size_t cpu_exact_aabb_pair_bound_evaluation_count{};
  std::size_t cpu_exact_point_pair_distance_evaluation_count{};
  std::size_t cpu_component_kappa_update_count{};
  std::size_t cpu_strict_component_cutoff_decrease_count{};
  std::size_t cpu_uniform_same_component_pair_prune_count{};
  std::size_t cpu_strict_aabb_pair_prune_count{};
  bool frozen_labels_certified{false};
  bool lbvh_topology_and_exact_aabbs_certified{false};
  bool complete_source_seed_coverage_certified{false};
  bool external_seed_targets_recertified{false};
  bool exact_seed_cutoffs_recertified{false};
  bool component_seed_reduction_certified{false};
  bool bidirectional_component_seed_reduction_certified{false};
  bool component_cutoff_upper_envelope_certified{false};
  bool canonical_unordered_pair_partition_certified{false};
  bool uniform_component_pair_prunes_certified{false};
  bool strict_only_aabb_pair_pruning_certified{false};
  bool depth_first_frontier_bound_certified{false};
  bool node_pair_visit_bound_certified{false};
  bool complete_frontier_exhaustion_certified{false};
  bool canonical_kappa_resolution_certified{false};
  bool component_minima_complete{false};

  friend bool operator==(
      const K1BoruvkaComponentDualTreeSearchAudit&,
      const K1BoruvkaComponentDualTreeSearchAudit&) = default;
};

// The bidirectionally seeded frozen component envelope is rebuilt for every
// nonterminal round. It remains a conservative upper bound while component
// incumbents decrease. Leaf pairs relax the two components directly; point
// minima never exist here, and no contraction or hierarchy is published.
struct K1BoruvkaSeededComponentDualTreeRoundResolution {
  std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaComponentDualTreeSearchAudit search_audit;
  K1BoruvkaSeedStatus seed_status{K1BoruvkaSeedStatus::not_certified};
  K1BoruvkaComponentDualTreeSearchStatus search_status{
      K1BoruvkaComponentDualTreeSearchStatus::not_certified};

  friend bool operator==(
      const K1BoruvkaSeededComponentDualTreeRoundResolution&,
      const K1BoruvkaSeededComponentDualTreeRoundResolution&) = default;
};

enum class K1BoruvkaEmissionStatus : std::uint8_t {
  not_certified,
  complete_source_ranges_candidate_payload_bound_certified,
};

enum class K1HybridBoruvkaEmissionMode : std::uint8_t {
  monolithic_round_payload,
  bounded_complete_source_ranges,
};

struct K1BoruvkaChunkedEmissionAudit {
  std::size_t logical_candidate_count{};
  std::size_t source_chunk_count{};
  std::size_t peak_chunk_source_count{};
  std::size_t peak_chunk_candidate_count{};
  std::size_t max_source_candidate_count{};
  std::size_t candidate_record_budget{};
  // Capacities are measured after the entry bound has released any inherited
  // oversized workspace and through the last synchronous chunk callback.
  std::size_t device_candidate_capacity_high_water{};
  std::size_t host_candidate_capacity_high_water{};
  std::size_t candidate_record_size_bytes{};
  std::size_t candidate_payload_peak_bytes{};
  std::size_t count_kernel_launch_count{};
  std::size_t emit_kernel_launch_count{};
  std::size_t synchronization_count{};
  bool complete_source_partition_certified{false};
  bool count_emit_cardinality_and_visit_count_certified{false};
  bool candidate_payload_physical_bound_certified{false};

  friend bool operator==(
      const K1BoruvkaChunkedEmissionAudit&,
      const K1BoruvkaChunkedEmissionAudit&) = default;
};

// A chunk callback is resolved immediately on reference_cpu. Candidate
// records, source-relative offsets and fixed seeds never enter this result.
// No hierarchy reduction or public MorseHGP3D status is performed here.
struct K1BoruvkaChunkedRoundResolution {
  std::vector<hierarchy::K1BoruvkaComponentMinimum>
      cpu_exact_component_minima;
  K1BoruvkaCandidateAudit proposal_audit;
  K1BoruvkaChunkedEmissionAudit emission_audit;
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaEmissionStatus emission_status{
      K1BoruvkaEmissionStatus::not_certified};
  K1BoruvkaSeedStatus seed_status{
      K1BoruvkaSeedStatus::not_certified};

  friend bool operator==(
      const K1BoruvkaChunkedRoundResolution&,
      const K1BoruvkaChunkedRoundResolution&) = default;
};

enum class K1HybridBoruvkaProposalStatus : std::uint8_t {
  not_certified,
  candidate_superset_certified,
};

enum class K1HybridBoruvkaDecisionStatus : std::uint8_t {
  not_certified,
  cpu_exact_kappa_minima_certified,
};

enum class K1HybridBoruvkaContractionStatus : std::uint8_t {
  not_certified,
  cpu_exact_canonical_contraction_certified,
};

enum class K1HybridHierarchyReductionStatus : std::uint8_t {
  not_performed,
};

enum class K1HybridScientificStatus : std::uint8_t {
  local_emst_witness_only,
};

struct K1HybridBoruvkaExactDecision {
  std::size_t round_index{};
  std::size_t frozen_component_count{};
  std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;

  friend bool operator==(
      const K1HybridBoruvkaExactDecision&,
      const K1HybridBoruvkaExactDecision&) = default;
};

struct K1HybridBoruvkaCanonicalContraction {
  std::vector<hierarchy::ExactEmstEdge> accepted_edges;
  std::size_t post_round_component_count{};

  friend bool operator==(
      const K1HybridBoruvkaCanonicalContraction&,
      const K1HybridBoruvkaCanonicalContraction&) = default;
};

// Candidates and fixed seeds are deliberately released after every round.
// The persistent record keeps the proposal audit, exact CPU decision and
// canonical contraction in three disjoint payloads.
struct K1HybridBoruvkaRound {
  K1BoruvkaCandidateAudit proposal_audit;
  K1BoruvkaChunkedEmissionAudit chunked_emission_audit;
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1HybridBoruvkaExactDecision exact_decision;
  K1HybridBoruvkaCanonicalContraction canonical_contraction;
  K1BoruvkaEmissionStatus chunked_emission_status{
      K1BoruvkaEmissionStatus::not_certified};
  K1BoruvkaSeedStatus seed_status{
      K1BoruvkaSeedStatus::not_certified};
  K1HybridBoruvkaProposalStatus proposal_status{
      K1HybridBoruvkaProposalStatus::not_certified};
  K1HybridBoruvkaDecisionStatus decision_status{
      K1HybridBoruvkaDecisionStatus::not_certified};
  K1HybridBoruvkaContractionStatus contraction_status{
      K1HybridBoruvkaContractionStatus::not_certified};

  friend bool operator==(
      const K1HybridBoruvkaRound&,
      const K1HybridBoruvkaRound&) = default;
};

struct K1HybridBoruvkaChunkedEmissionCounters {
  std::size_t round_count{};
  std::size_t logical_candidate_count{};
  std::size_t source_chunk_count{};
  std::size_t peak_chunk_source_count{};
  std::size_t peak_chunk_candidate_count{};
  std::size_t max_source_candidate_count{};
  std::size_t candidate_record_budget{};
  std::size_t device_candidate_capacity_high_water{};
  std::size_t host_candidate_capacity_high_water{};
  std::size_t candidate_record_size_bytes{};
  std::size_t candidate_payload_peak_bytes{};
  std::size_t count_kernel_launch_count{};
  std::size_t emit_kernel_launch_count{};
  std::size_t synchronization_count{};

  friend bool operator==(
      const K1HybridBoruvkaChunkedEmissionCounters&,
      const K1HybridBoruvkaChunkedEmissionCounters&) = default;
};

struct K1HybridBoruvkaMortonSeedCounters {
  std::size_t round_count{};
  std::size_t source_count{};
  std::size_t window_radius{};
  std::size_t neighbor_inspection_budget_per_source{};
  std::size_t maximum_inspected_neighbor_count_per_source{};
  std::size_t inspected_neighbor_count{};
  std::size_t external_neighbor_count{};
  std::size_t floating_proposal_count{};
  std::size_t exact_selected_proposal_count{};
  std::size_t exact_strict_improvement_count{};
  std::size_t exact_fallback_count{};
  std::size_t exact_seed_distance_evaluation_count{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};

  friend bool operator==(
      const K1HybridBoruvkaMortonSeedCounters&,
      const K1HybridBoruvkaMortonSeedCounters&) = default;
};

// Aggregates only the producer chain persisted in rounds. The independent
// verifier runs a second GPU proposal chain and reports its own work below.
struct K1HybridBoruvkaCounters {
  std::size_t point_count{};
  std::size_t lbvh_node_count{};
  std::size_t round_count{};
  std::size_t theoretical_max_round_count{};
  std::size_t frozen_component_label_count{};
  std::size_t component_minimum_count{};
  std::size_t accepted_edge_count{};
  std::size_t component_contraction_count{};
  std::size_t gpu_candidate_count{};
  // Logical extents retained for comparison with the historical monolithic
  // path. Never interpret these two fields as physical memory in chunked mode.
  std::size_t gpu_output_capacity_sum{};
  std::size_t peak_gpu_output_capacity{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  std::size_t gpu_count_pass_node_visit_count{};
  std::size_t gpu_emit_pass_node_visit_count{};
  std::size_t gpu_uniform_component_prune_count{};
  std::size_t gpu_strict_aabb_prune_count{};
  std::size_t gpu_invalid_bound_descent_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_required_candidate_count{};
  std::size_t cpu_exact_candidate_distance_evaluation_count{};
  std::uint64_t first_buffer_epoch{};
  std::uint64_t last_buffer_epoch{};
  std::size_t final_component_count{};

  friend bool operator==(
      const K1HybridBoruvkaCounters&,
      const K1HybridBoruvkaCounters&) = default;
};

// This is a local exact EMST witness, not a MorseHGP3D public hierarchy. The
// exact edges can be passed separately to build_compact_k1_forest only after
// the caller accepts this explicitly distinct hierarchy reduction step.
struct K1HybridBoruvkaResult {
  static constexpr const char* proof_basis =
      "gpu_candidate_superset_cpu_exact_boruvka_v1";
  static constexpr const char* bounded_emission_proof_basis =
      "gpu_complete_source_ranges_bounded_candidate_payload_v1";
  static constexpr const char* monotone_seed_proof_basis =
      "gpu_bounded_morton_seed_cpu_exact_monotone_cutoff_v1";

  std::size_t point_count{};
  std::vector<K1HybridBoruvkaRound> rounds;
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1HybridBoruvkaCounters counters{};
  K1HybridBoruvkaChunkedEmissionCounters chunked_emission_counters{};
  K1HybridBoruvkaMortonSeedCounters morton_seed_counters{};
  K1BoruvkaChunkingPolicy chunking_policy{};
  K1BoruvkaMortonSeedPolicy morton_seed_policy{};
  K1HybridBoruvkaEmissionMode emission_mode{
      K1HybridBoruvkaEmissionMode::monolithic_round_payload};
  K1BoruvkaSeedMode seed_mode{
      K1BoruvkaSeedMode::canonical_external_fallback};
  K1HybridHierarchyReductionStatus hierarchy_reduction_status{
      K1HybridHierarchyReductionStatus::not_performed};
  K1HybridScientificStatus scientific_status{
      K1HybridScientificStatus::local_emst_witness_only};
  bool proposal_chain_certified{false};
  bool bounded_candidate_emission_chain_certified{false};
  bool bounded_morton_seed_chain_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contraction_chain_certified{false};
  bool reference_cpu_witness_certified{false};
  bool emst_witness_certified{false};
};

struct K1HybridBoruvkaVerification {
  bool index_identity_certified{false};
  bool emission_mode_certified{false};
  bool seed_mode_certified{false};
  bool proposal_chain_certified{false};
  bool bounded_candidate_emission_chain_certified{false};
  bool bounded_morton_seed_chain_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contractions_certified{false};
  bool round_count_bound_certified{false};
  bool spanning_tree_certified{false};
  bool exact_weights_certified{false};
  bool reference_cpu_witness_certified{false};
  bool counters_certified{false};
  bool hierarchy_status_separation_certified{false};
  bool emst_witness_certified{false};
  std::size_t reference_round_count{};
  std::size_t reference_component_minimum_count{};
  std::size_t gpu_replayed_round_count{};
  std::size_t gpu_replayed_component_minimum_count{};
  std::size_t gpu_replay_kernel_launch_count{};
  std::size_t gpu_replay_synchronization_count{};
  std::size_t gpu_replay_source_chunk_count{};
  std::size_t gpu_replay_peak_chunk_candidate_count{};
  std::size_t gpu_replay_candidate_payload_peak_bytes{};
  std::size_t gpu_replay_seed_inspected_neighbor_count{};
  std::size_t gpu_replay_seed_selected_proposal_count{};
  std::size_t gpu_replay_seed_strict_improvement_count{};
  std::size_t gpu_replay_seed_kernel_launch_count{};
  std::size_t gpu_replay_seed_synchronization_count{};
};

// Persistent full-chain records deliberately omit the per-source minima:
// they are reduced immediately after each certified external-1NN search.
// Only the bounded seed audit, exact search audit, component decision and
// explicit canonical contraction survive the round.
struct K1SeededExactBoruvkaRound {
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaExactSearchAudit exact_search_audit;
  K1HybridBoruvkaExactDecision exact_decision;
  K1HybridBoruvkaCanonicalContraction canonical_contraction;
  K1BoruvkaSeedStatus seed_status{K1BoruvkaSeedStatus::not_certified};
  K1BoruvkaExactSearchStatus search_status{
      K1BoruvkaExactSearchStatus::not_certified};
  K1HybridBoruvkaDecisionStatus decision_status{
      K1HybridBoruvkaDecisionStatus::not_certified};
  K1HybridBoruvkaContractionStatus contraction_status{
      K1HybridBoruvkaContractionStatus::not_certified};

  friend bool operator==(
      const K1SeededExactBoruvkaRound&,
      const K1SeededExactBoruvkaRound&) = default;
};

struct K1SeededExactBoruvkaCounters {
  std::size_t point_count{};
  std::size_t lbvh_node_count{};
  std::size_t round_count{};
  std::size_t theoretical_max_round_count{};
  std::size_t frozen_component_label_count{};
  std::size_t component_minimum_count{};
  std::size_t accepted_edge_count{};
  std::size_t component_contraction_count{};
  std::size_t uniform_lbvh_node_tag_count{};
  std::size_t mixed_lbvh_node_tag_count{};
  std::size_t morton_seed_source_count{};
  std::size_t morton_inspected_neighbor_count{};
  std::size_t morton_external_neighbor_count{};
  std::size_t morton_floating_proposal_count{};
  std::size_t morton_exact_selected_proposal_count{};
  std::size_t morton_exact_strict_improvement_count{};
  std::size_t morton_exact_fallback_count{};
  std::size_t morton_exact_seed_distance_evaluation_count{};
  std::size_t morton_gpu_kernel_launch_count{};
  std::size_t morton_gpu_synchronization_count{};
  std::size_t exact_point_query_count{};
  std::size_t exact_seed_incumbent_count{};
  std::size_t exact_point_minimum_count{};
  std::size_t maximum_exact_node_visit_count_per_source{};
  std::size_t maximum_exact_point_distance_evaluation_count_per_source{};
  std::size_t maximum_exact_frontier_size_per_source{};
  std::size_t exact_node_visit_count{};
  std::size_t exact_internal_node_expansion_count{};
  std::size_t exact_aabb_bound_evaluation_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t exact_seed_leaf_distance_reuse_count{};
  std::size_t exact_uniform_component_prune_count{};
  std::size_t exact_strict_aabb_prune_count{};
  std::size_t final_component_count{};

  friend bool operator==(
      const K1SeededExactBoruvkaCounters&,
      const K1SeededExactBoruvkaCounters&) = default;
};

// Local exact EMST witness only. No candidate universe is materialized and no
// hierarchy reduction or MorseHGP3D public status is performed by this API.
struct K1SeededExactBoruvkaResult {
  static constexpr const char* proof_basis =
      "gpu_bounded_morton_seed_cpu_exact_external_1nn_boruvka_v1";

  std::size_t point_count{};
  std::vector<K1SeededExactBoruvkaRound> rounds;
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1SeededExactBoruvkaCounters counters{};
  K1BoruvkaMortonSeedPolicy morton_seed_policy{};
  K1HybridHierarchyReductionStatus hierarchy_reduction_status{
      K1HybridHierarchyReductionStatus::not_performed};
  K1HybridScientificStatus scientific_status{
      K1HybridScientificStatus::local_emst_witness_only};
  bool bounded_morton_seed_chain_certified{false};
  bool exact_external_1nn_chain_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contraction_chain_certified{false};
  bool fresh_replay_certified{false};
  bool reference_cpu_witness_certified{false};
  bool emst_witness_certified{false};
};

struct K1SeededExactBoruvkaVerification {
  bool index_identity_certified{false};
  bool trusted_seed_policy_certified{false};
  bool bounded_morton_seed_chain_certified{false};
  bool exact_external_1nn_chain_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contractions_certified{false};
  bool fresh_replay_certified{false};
  bool round_count_bound_certified{false};
  bool spanning_tree_certified{false};
  bool exact_weights_certified{false};
  bool reference_cpu_witness_certified{false};
  bool counters_certified{false};
  bool hierarchy_status_separation_certified{false};
  bool emst_witness_certified{false};
  std::size_t reference_round_count{};
  std::size_t reference_component_minimum_count{};
  std::size_t replayed_round_count{};
  std::size_t replayed_component_minimum_count{};
  std::size_t replayed_seed_kernel_launch_count{};
  std::size_t replayed_seed_synchronization_count{};
  std::size_t replayed_exact_node_visit_count{};
  std::size_t replayed_exact_aabb_bound_evaluation_count{};
  std::size_t replayed_exact_point_distance_evaluation_count{};
};

// Persistent shared-search rounds decide component minima directly after
// offering each recertified seed to both incident components. Only the seed
// audit, frozen component-envelope audit, exact component decision and
// canonical contraction survive; point minima and dynamic ancestor updates do
// not exist in this result.
struct K1DualTreeExactBoruvkaRound {
  K1BoruvkaMortonSeedAudit morton_seed_audit;
  K1BoruvkaComponentDualTreeSearchAudit dual_tree_search_audit;
  K1HybridBoruvkaExactDecision exact_decision;
  K1HybridBoruvkaCanonicalContraction canonical_contraction;
  K1BoruvkaSeedStatus seed_status{K1BoruvkaSeedStatus::not_certified};
  K1BoruvkaComponentDualTreeSearchStatus search_status{
      K1BoruvkaComponentDualTreeSearchStatus::not_certified};
  K1HybridBoruvkaDecisionStatus decision_status{
      K1HybridBoruvkaDecisionStatus::not_certified};
  K1HybridBoruvkaContractionStatus contraction_status{
      K1HybridBoruvkaContractionStatus::not_certified};

  friend bool operator==(
      const K1DualTreeExactBoruvkaRound&,
      const K1DualTreeExactBoruvkaRound&) = default;
};

// Local exact EMST witness only. A fresh shared traversal and the independent
// reference_cpu Boruvka anchor must both agree before certification. No point
// minima, candidate universe or hierarchy reduction persists in this result.
struct K1DualTreeExactBoruvkaResult {
  static constexpr const char* proof_basis =
      "gpu_bounded_morton_seed_cpu_exact_bidirectional_component_dual_tree_boruvka_v3";

  std::size_t point_count{};
  std::vector<K1DualTreeExactBoruvkaRound> rounds;
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1BoruvkaMortonSeedPolicy morton_seed_policy{};
  K1HybridHierarchyReductionStatus hierarchy_reduction_status{
      K1HybridHierarchyReductionStatus::not_performed};
  K1HybridScientificStatus scientific_status{
      K1HybridScientificStatus::local_emst_witness_only};
  bool bounded_morton_seed_chain_certified{false};
  bool exact_dual_tree_chain_certified{false};
  bool component_minima_only_persistence_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contraction_chain_certified{false};
  bool fresh_replay_certified{false};
  bool reference_cpu_witness_certified{false};
  bool emst_witness_certified{false};
};

struct K1DualTreeExactBoruvkaVerification {
  bool index_identity_certified{false};
  bool trusted_seed_policy_certified{false};
  bool bounded_morton_seed_chain_certified{false};
  bool exact_dual_tree_chain_certified{false};
  bool component_minima_only_persistence_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contractions_certified{false};
  bool fresh_replay_certified{false};
  bool round_count_bound_certified{false};
  bool spanning_tree_certified{false};
  bool exact_weights_certified{false};
  bool reference_cpu_witness_certified{false};
  bool hierarchy_status_separation_certified{false};
  bool emst_witness_certified{false};
  std::size_t reference_round_count{};
  std::size_t reference_component_minimum_count{};
  std::size_t replayed_round_count{};
  std::size_t replayed_component_minimum_count{};
  std::size_t replayed_seed_kernel_launch_count{};
  std::size_t replayed_seed_synchronization_count{};
  std::size_t replayed_node_pair_visit_count{};
  std::size_t replayed_aabb_pair_bound_evaluation_count{};
  std::size_t replayed_point_pair_distance_evaluation_count{};
};

// This status belongs only to the explicit local k=1 adapter below. It never
// mutates the source EMST witness and is not a MorseHGP3D public status.
enum class K1SeededExactHierarchyReductionStatus : std::uint8_t {
  not_certified,
  compact_k1_forest_certified,
};

enum class K1SeededExactHierarchyScope : std::uint8_t {
  unspecified,
  local_k1_compact_forest_only,
};

struct K1SeededExactCompactHierarchy {
  static constexpr const char* proof_basis =
      "verified_exact_emst_equal_level_compact_k1_reduction_v1";

  hierarchy::K1CompactForest forest;
  K1SeededExactHierarchyReductionStatus reduction_status{
      K1SeededExactHierarchyReductionStatus::not_certified};
  K1SeededExactHierarchyScope scope{
      K1SeededExactHierarchyScope::unspecified};
};

// The five persistent compact arenas are checked against both a fresh
// reduction of the supplied witness and a second reduction of the independent
// exact LBVH Boruvka anchor. No materialized coverage vectors or all-level cut
// tables enter this compact-storage verifier. Its exact-search work retains
// the separately documented complexity status of the source verifier.
struct K1SeededExactCompactHierarchyVerification {
  bool source_emst_witness_certified{false};
  bool source_status_separation_certified{false};
  bool source_tree_binding_certified{false};
  bool exact_weights_certified{false};
  bool equal_level_batches_certified{false};
  bool canonical_multifusions_certified{false};
  bool compact_topology_certified{false};
  bool counters_certified{false};
  bool linear_storage_certified{false};
  bool reduction_status_certified{false};
  bool local_scope_certified{false};
  bool local_k1_hierarchy_certified{false};
};

class K1BoruvkaCandidateContext final {
 public:
  K1BoruvkaCandidateContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);
  ~K1BoruvkaCandidateContext() noexcept;

  K1BoruvkaCandidateContext(K1BoruvkaCandidateContext&&) noexcept;
  K1BoruvkaCandidateContext& operator=(
      K1BoruvkaCandidateContext&&) noexcept;

  K1BoruvkaCandidateContext(const K1BoruvkaCandidateContext&) = delete;
  K1BoruvkaCandidateContext& operator=(
      const K1BoruvkaCandidateContext&) = delete;

  [[nodiscard]] K1BoruvkaRoundProposal propose_round(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels);

  [[nodiscard]] K1BoruvkaChunkedRoundResolution propose_round_chunked(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaChunkingPolicy policy);

  [[nodiscard]] K1BoruvkaChunkedRoundResolution propose_round_chunked(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaChunkingPolicy chunking_policy,
      K1BoruvkaMortonSeedPolicy seed_policy);

  [[nodiscard]] K1BoruvkaSeededExactRoundResolution
  resolve_round_exact_external_1nn(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaMortonSeedPolicy seed_policy);

  [[nodiscard]] K1BoruvkaSeededDualTreeRoundResolution
  resolve_round_exact_external_1nn_dual_tree(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaMortonSeedPolicy seed_policy);

  [[nodiscard]] K1BoruvkaSeededComponentDualTreeRoundResolution
  resolve_round_exact_component_minima_dual_tree(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaMortonSeedPolicy seed_policy);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t node_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  [[nodiscard]] K1BoruvkaChunkedRoundResolution
  propose_round_chunked_impl(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels,
      K1BoruvkaChunkingPolicy chunking_policy,
      const K1BoruvkaMortonSeedPolicy* seed_policy);

  std::shared_ptr<detail::K1BoruvkaCandidateContextState> state_;
  std::unique_ptr<detail::K1BoruvkaCandidateHostState> host_;
};

// Runs one resident GPU proposal context through all Boruvka rounds. Every
// proposal is recertified and resolved on reference_cpu before the pure CPU
// contraction. No partial result or CPU fallback is returned after a GPU or
// certificate failure. A separate reference CPU witness is compared before
// the local EMST certificate is published.
[[nodiscard]] K1HybridBoruvkaResult
build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud);

// Runs the same independently replayed hybrid chain while forcing every
// proposal round through complete, contiguous source ranges whose candidate
// payload has the supplied hard record bound. No source is split and no exact
// component minimum is published before the final chunk is recertified.
[[nodiscard]] K1HybridBoruvkaResult
build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy chunking_policy);

// Adds a bounded floating Morton-neighborhood heuristic for each seed. The
// certificate relies only on the returned PointId being recertified as an
// external target inside the trusted window; reference_cpu selects it only
// when its exact canonical edge key improves the canonical fallback key.
[[nodiscard]] K1HybridBoruvkaResult
build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy chunking_policy,
    K1BoruvkaMortonSeedPolicy seed_policy);

// Reruns the complete GPU proposal chain in a fresh resident context, then
// recomputes the reference CPU Boruvka witness and all backend-neutral
// contraction/weight invariants. Result flags, per-round statuses, audits and
// hybrid counters are treated as untrusted inputs.
[[nodiscard]] K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result);

// The chunk budget is trusted caller policy, not data recovered from the
// untrusted result. The verifier compares it before any replay allocation.
[[nodiscard]] K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    const K1HybridBoruvkaResult& result);

// Both policies are trusted caller inputs and are checked before the fresh
// GPU replay. Neither bound is recovered only from the untrusted result.
[[nodiscard]] K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1HybridBoruvkaResult& result);

// Runs every nonterminal round through one bounded GPU Morton seed proposal
// per source followed by an exact CPU point-to-LBVH branch-and-bound. Point
// minima are reduced before the canonical CPU contraction and are not kept in
// the persistent result. A fresh seeded search chain and the independent CPU
// Boruvka anchor must both agree before the local EMST witness is certified.
[[nodiscard]] K1SeededExactBoruvkaResult
build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy seed_policy);

// The trusted seed radius is supplied separately from the untrusted result.
// Verification reruns every GPU seed proposal and exact CPU search in a fresh
// resident context before comparing contractions, weights and the CPU anchor.
[[nodiscard]] K1SeededExactBoruvkaVerification
verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& result);

// Runs every nonterminal round through the shared exact component-direct
// dual-tree resolver. Recertified seeds are offered to both incident
// components, the frozen cutoff envelope is rebuilt after every contraction,
// only component decisions persist, and a fresh direct replay plus the
// reference_cpu anchor certifies the local EMST witness.
[[nodiscard]] K1DualTreeExactBoruvkaResult
build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy seed_policy);

// The trusted seed radius is supplied separately from the untrusted result.
// Every shared round is rerun in a new resident context before contractions,
// weights and the independent reference_cpu witness are compared.
[[nodiscard]] K1DualTreeExactBoruvkaVerification
verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1DualTreeExactBoruvkaResult& result);

// Explicitly reduces a separately recertified local EMST witness. The source
// remains hierarchy_reduction_status=not_performed and
// scientific_status=local_emst_witness_only; only this returned adapter owns a
// local compact-hierarchy status.
[[nodiscard]] K1SeededExactCompactHierarchy
build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& source);

// Treats the source, the compact arenas and all reported local statuses as
// untrusted. The source chain is replayed before two fresh compact reductions
// are compared with the supplied reduction.
[[nodiscard]] K1SeededExactCompactHierarchyVerification
verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& source,
    const K1SeededExactCompactHierarchy& reduction);

}  // namespace morsehgp3d::gpu
