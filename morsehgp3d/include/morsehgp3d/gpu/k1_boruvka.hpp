#pragma once

#include "morsehgp3d/hierarchy/boruvka.hpp"
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
  K1HybridBoruvkaExactDecision exact_decision;
  K1HybridBoruvkaCanonicalContraction canonical_contraction;
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

  std::size_t point_count{};
  std::vector<K1HybridBoruvkaRound> rounds;
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1HybridBoruvkaCounters counters{};
  K1HybridHierarchyReductionStatus hierarchy_reduction_status{
      K1HybridHierarchyReductionStatus::not_performed};
  K1HybridScientificStatus scientific_status{
      K1HybridScientificStatus::local_emst_witness_only};
  bool proposal_chain_certified{false};
  bool cpu_exact_decision_chain_certified{false};
  bool canonical_contraction_chain_certified{false};
  bool reference_cpu_witness_certified{false};
  bool emst_witness_certified{false};
};

struct K1HybridBoruvkaVerification {
  bool index_identity_certified{false};
  bool proposal_chain_certified{false};
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

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t node_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

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

// Reruns the complete GPU proposal chain in a fresh resident context, then
// recomputes the reference CPU Boruvka witness and all backend-neutral
// contraction/weight invariants. Result flags, per-round statuses, audits and
// hybrid counters are treated as untrusted inputs.
[[nodiscard]] K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result);

}  // namespace morsehgp3d::gpu
