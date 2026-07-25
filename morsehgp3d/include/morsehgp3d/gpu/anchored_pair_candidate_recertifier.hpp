#pragma once

#include "morsehgp3d/gpu/anchored_pair_candidate_proposal.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_anchored_pair_candidate_recertifier_schema_version = 1U;
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_backend = "reference_cpu";
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_mode =
        "exact_sparsification_design";
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_public_status =
        "not_claimed";
inline constexpr std::string_view
    exact_anchored_pair_candidate_recertifier_proof_basis =
        "hostile_complete_single_segment_inverse_postorder_replay_"
        "and_exact_strict_witness_aabb_recertification_v1";

// Every successful output extent is bounded by one authenticated transcript
// extent.  The predicate cap is checked before exact geometry and equals the
// number of set witness bits that would be replayed.  These caps never size a
// global pair, cell, coface or incidence arena.
struct ExactAnchoredPairCandidateRecertifierBudget {
  std::size_t maximum_query_count{};
  std::size_t maximum_transcript_record_count{};
  std::size_t maximum_candidate_point_id_count{};
  std::size_t maximum_prune_receipt_count{};
  std::size_t maximum_exact_witness_predicate_count{};

  friend bool operator==(
      const ExactAnchoredPairCandidateRecertifierBudget&,
      const ExactAnchoredPairCandidateRecertifierBudget&) = default;
};

enum class ExactAnchoredPairCandidateRecertifierDecision : std::uint8_t {
  not_recertified,
  complete,
  rejected,
};

enum class ExactAnchoredPairCandidateRecertifierRejectionReason
    : std::uint8_t {
  none,
  proposal_batch_contract,
  index_cloud_identity,
  capacity_overflow,
  budget_exhausted,
  query_contract,
  query_digest,
  incomplete_segment,
  segment_partition,
  record_order_or_domain,
  false_candidate_record,
  missing_candidate_record,
  false_prune_record,
  traversal_jump,
  node_visit_count,
  incomplete_tree_coverage,
};

struct ExactAnchoredPairCandidatePruneReceipt {
  std::size_t lbvh_node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::array<spatial::PointId, 10> witness_point_ids{};
  std::size_t witness_count{};

  friend bool operator==(
      const ExactAnchoredPairCandidatePruneReceipt&,
      const ExactAnchoredPairCandidatePruneReceipt&) = default;
};

struct ExactAnchoredPairRecertifiedAnchorCandidates {
  std::size_t query_index{};
  std::uint64_t query_digest_fnv1a{};
  spatial::PointId anchor_point_id{};
  std::size_t maximum_closed_rank{};
  std::vector<spatial::PointId> candidate_point_ids;
  std::vector<ExactAnchoredPairCandidatePruneReceipt> prune_receipts;

  friend bool operator==(
      const ExactAnchoredPairRecertifiedAnchorCandidates&,
      const ExactAnchoredPairRecertifiedAnchorCandidates&) = default;
};

struct ExactAnchoredPairCandidateRecertifierAudit {
  std::size_t input_query_count{};
  std::size_t input_segment_count{};
  std::size_t input_transcript_record_count{};
  std::size_t required_candidate_point_id_count{};
  std::size_t required_prune_receipt_count{};
  std::size_t required_exact_witness_predicate_count{};
  std::size_t recertified_anchor_count{};
  std::size_t replayed_node_visit_count{};
  std::size_t recertified_candidate_point_id_count{};
  std::size_t recertified_prune_receipt_count{};
  std::size_t exact_witness_predicate_count{};
  std::size_t maximum_observed_witness_bank_size{};
  std::uint64_t source_snapshot_epoch{};

  bool proposal_batch_host_contract_validated{false};
  bool index_cloud_identity_validated{false};
  bool capacity_preflight_completed{false};
  bool capacity_preflight_satisfied{false};
  bool canonical_query_order_validated{false};
  bool query_digests_recomputed{false};
  bool every_segment_initial_to_complete{false};
  bool segment_partition_validated{false};
  bool records_strictly_decreasing{false};
  bool candidate_leaf_orientation_recertified{false};
  bool strict_witness_masks_recertified{false};
  bool inverse_postorder_jump_arithmetic_validated{false};
  bool node_visit_counts_replayed{false};
  bool complete_tree_coverage_replayed{false};
  bool batch_rejection_atomic{true};

  // This component closes only a candidate superset and local prune receipts.
  // P8c classification and every hierarchy/publication decision remain absent.
  bool exact_closed_ball_partition_published{false};
  bool candidate_classification_performed{false};
  bool scientific_morse_decision_published{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactAnchoredPairCandidateRecertifierAudit&,
      const ExactAnchoredPairCandidateRecertifierAudit&) = default;
};

struct ExactAnchoredPairCandidateRecertifierResult {
  static constexpr std::string_view backend =
      exact_anchored_pair_candidate_recertifier_backend;
  static constexpr std::string_view profile =
      exact_anchored_pair_candidate_recertifier_profile;
  static constexpr std::string_view mode =
      exact_anchored_pair_candidate_recertifier_mode;
  static constexpr std::string_view deployment_status =
      exact_anchored_pair_candidate_recertifier_deployment_status;
  static constexpr std::string_view public_status =
      exact_anchored_pair_candidate_recertifier_public_status;
  static constexpr std::string_view proof_basis =
      exact_anchored_pair_candidate_recertifier_proof_basis;

  std::uint32_t schema_version{
      exact_anchored_pair_candidate_recertifier_schema_version};
  ExactAnchoredPairCandidateRecertifierBudget requested_budget{};
  ExactAnchoredPairCandidateRecertifierDecision decision{
      ExactAnchoredPairCandidateRecertifierDecision::not_recertified};
  ExactAnchoredPairCandidateRecertifierRejectionReason rejection_reason{
      ExactAnchoredPairCandidateRecertifierRejectionReason::none};
  ExactAnchoredPairCandidateRecertifierAudit audit{};
  std::vector<ExactAnchoredPairRecertifiedAnchorCandidates> anchors;

  [[nodiscard]] bool complete_exact_candidate_stream() const noexcept;
  [[nodiscard]] bool atomic_rejection() const noexcept;
  [[nodiscard]] bool scientific_morse_outcome() const noexcept;

  friend bool operator==(
      const ExactAnchoredPairCandidateRecertifierResult&,
      const ExactAnchoredPairCandidateRecertifierResult&) = default;
};

// The initial scope accepts exactly one complete segment per canonical query.
// It reconstructs every omitted internal node and every omitted q<=anchor leaf
// from the certified CPU LBVH.  A mask-zero record must be the corresponding
// oriented leaf; a nonzero record is accepted only after every named witness
// certifies a strictly positive exact AABB minimum.  Any mismatch rejects the
// whole batch and publishes no per-anchor output.
[[nodiscard]] ExactAnchoredPairCandidateRecertifierResult
recertify_exact_anchored_pair_candidate_proposal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const AnchoredPairCandidateProposalQuery>
        canonical_queries,
    const AnchoredPairCandidateProposalBatchResult& proposal_batch,
    ExactAnchoredPairCandidateRecertifierBudget budget);

}  // namespace morsehgp3d::gpu
