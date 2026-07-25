#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t
    exact_anchored_pair_witness_bank_maximum_closed_rank = 11U;
inline constexpr std::size_t
    exact_anchored_pair_witness_bank_maximum_size = 64U;
inline constexpr std::string_view
    exact_anchored_pair_witness_bank_proof_basis =
        "exact_bounded_anchor_top_k_witness_bank_uniform_strict_"
        "diametral_aabb_exclusion_v1";

enum class ExactAnchoredPairCandidateStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class ExactAnchoredPairCandidateStopReason : std::uint8_t {
  none,
  node_visit_limit,
  internal_node_expansion_limit,
  traversal_stack_entry_limit,
  candidate_entry_limit,
};

// The top-k query proposes a bounded witness bank.  Failure of that query is
// deliberately nonterminal: traversal continues with an empty bank and emits
// every q > anchor.  Predicate and prune-record caps are fail-open for the same
// reason.  Only traversal/output caps can make the candidate stream incomplete.
struct ExactAnchoredPairWitnessBankBudget {
  std::size_t proposed_witness_bank_size{};
  spatial::ExactLbvhTopKBudget witness_search_budget{};
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_internal_node_expansion_count{};
  std::size_t maximum_traversal_stack_entry_count{};
  std::size_t maximum_witness_node_predicate_count{};
  std::size_t maximum_candidate_entry_count{};
  std::size_t maximum_prune_record_count{};

  friend bool operator==(
      const ExactAnchoredPairWitnessBankBudget&,
      const ExactAnchoredPairWitnessBankBudget&) = default;
};

// A record is a process-local, replayable proof that every point in the named
// LBVH node has at least witness_count strict diametral witnesses.  At most ten
// witnesses are ever required because the product contract has s_max <= 11.
struct ExactAnchoredPairWitnessBankPruneRecord {
  std::size_t lbvh_node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::array<spatial::PointId, 10> witness_point_ids{};
  std::size_t witness_count{};

  friend bool operator==(
      const ExactAnchoredPairWitnessBankPruneRecord&,
      const ExactAnchoredPairWitnessBankPruneRecord&) = default;
};

struct ExactAnchoredPairWitnessBankAudit {
  std::size_t requested_witness_bank_size{};
  std::size_t effective_witness_bank_size{};
  bool witness_search_attempted{false};
  bool witness_search_complete{false};
  spatial::ExactLbvhTopKStatus witness_search_status{
      spatial::ExactLbvhTopKStatus::not_certified};
  spatial::ExactLbvhTopKStopReason witness_search_stop_reason{
      spatial::ExactLbvhTopKStopReason::none};
  spatial::ExactLbvhTopKAudit witness_search_audit{};
  bool witness_bank_sufficient_for_rank_pruning{false};
  std::size_t node_visit_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t leaf_visit_count{};
  std::size_t witness_node_predicate_count{};
  std::size_t fp64_interval_positive_count{};
  std::size_t fp64_interval_negative_count{};
  std::size_t exact_witness_node_predicate_count{};
  std::size_t predicate_budget_fail_open_count{};
  std::size_t certified_pruned_subtree_count{};
  // This counts every leaf in a pruned Morton range.  It is only an upper
  // bound on oriented q > anchor candidates because PointId order and Morton
  // order are independent; obtaining the exact oriented count would require
  // scanning the omitted leaves or adding a per-anchor global side structure.
  std::size_t certified_pruned_leaf_upper_bound{};
  std::size_t prune_record_capacity_fail_open_count{};
  std::size_t candidate_entry_count{};
  std::size_t peak_traversal_stack_entry_count{};
  bool traversal_complete{false};

  friend bool operator==(
      const ExactAnchoredPairWitnessBankAudit&,
      const ExactAnchoredPairWitnessBankAudit&) = default;
};

// candidate_point_ids is a bounded per-anchor stream image, in deterministic
// LBVH DFS order.  Canonical PointId order, not Morton order, orients each
// unordered pair as anchor < q; Morton order controls emission only.  A
// complete result contains every q > anchor not covered by one of the exact
// prune records; in particular it contains every pair whose closed rank is at
// most maximum_closed_rank.  It is a candidate superset, not a public Morse
// decision.
struct ExactAnchoredPairWitnessBankResult {
  ExactAnchoredPairCandidateStatus status{
      ExactAnchoredPairCandidateStatus::budget_exhausted};
  ExactAnchoredPairCandidateStopReason stop_reason{
      ExactAnchoredPairCandidateStopReason::none};
  spatial::PointId anchor_point_id{};
  std::size_t maximum_closed_rank{};
  ExactAnchoredPairWitnessBankBudget requested_budget{};
  ExactAnchoredPairWitnessBankAudit audit{};
  std::vector<spatial::PointId> witness_bank_point_ids;
  std::vector<spatial::PointId> candidate_point_ids;
  std::vector<ExactAnchoredPairWitnessBankPruneRecord> prune_records;

  [[nodiscard]] bool complete() const noexcept {
    return status == ExactAnchoredPairCandidateStatus::complete;
  }
};

// For p=anchor, witness r and an exact LBVH AABB Q, this returns the sign of
// min_{q in Q} (r-p).(q-p) - ||r-p||^2.  A positive sign is the only pruning
// certificate; zero and negative signs are inconclusive.
[[nodiscard]] int exact_anchored_pair_witness_aabb_minimum_sign(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId anchor_point_id,
    spatial::PointId witness_point_id,
    const spatial::ExactDyadicAabb3& query_bounds);

[[nodiscard]] ExactAnchoredPairWitnessBankResult
build_exact_anchored_pair_witness_bank_candidates(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId anchor_point_id,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairWitnessBankBudget budget);

}  // namespace morsehgp3d::hierarchy
