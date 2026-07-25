#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t
    exact_anchored_pair_candidate_maximum_closed_rank = 11U;
inline constexpr std::string_view
    exact_anchored_pair_candidate_classifier_proof_basis =
        "exact_sparse_diametral_closed_ball_lbvh_partition_v1";

enum class ExactAnchoredPairCandidateClassificationStatus : std::uint8_t {
  complete,
  above_rank,
  budget_exhausted,
};

enum class ExactAnchoredPairCandidateClassificationStopReason : std::uint8_t {
  none,
  node_visit_limit,
};

// One physical LBVH-node visit is charged before the node is read or the
// traversal cursor is mutated.  There is deliberately no independent stack
// budget: a non-resumable call retains at most one DFS path plus siblings, and
// its growth is already bounded by the number of charged visits.
struct ExactAnchoredPairCandidateClassificationBudget {
  std::size_t maximum_node_visit_count{};

  friend bool operator==(
      const ExactAnchoredPairCandidateClassificationBudget&,
      const ExactAnchoredPairCandidateClassificationBudget&) = default;
};

struct ExactAnchoredPairCandidateClassificationAudit {
  std::size_t node_visit_count{};
  // Exact partition of node_visit_count.  Strict interval decisions are
  // authoritative because their outward bounds exclude zero; every uncertain
  // interval, including equality, is charged once to exact_node_fallback.
  std::size_t interval_exterior_node_count{};
  std::size_t interval_interior_node_count{};
  std::size_t exact_node_fallback_count{};
  std::size_t exact_minimum_phi_aabb_bound_count{};
  std::size_t exact_maximum_phi_aabb_bound_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t leaf_visit_count{};
  std::size_t exact_point_classification_count{};
  std::size_t bulk_interior_subtree_count{};
  std::size_t bulk_interior_point_count{};
  std::size_t bulk_exterior_subtree_count{};
  std::size_t bulk_exterior_point_count{};
  std::size_t classified_point_count{};
  std::size_t maximum_frontier_entry_count{};
  bool early_above_rank_certificate{false};
  bool complete_partition_certified{false};
  bool center_and_level_constructed{false};
  bool fp64_interval_filter_enabled{false};
  bool no_global_cell_or_incidence_structure_materialized{true};

  friend bool operator==(
      const ExactAnchoredPairCandidateClassificationAudit&,
      const ExactAnchoredPairCandidateClassificationAudit&) = default;
};

// A complete result owns exactly one regular event or one relevant
// extra-shell diagnostic.  above_rank and budget_exhausted own neither.  The
// exterior is represented only by its exact count; the equality shell is
// traversed completely, but only its cardinality and canonical least extra
// witness are retained by the reused pair-support record types.
struct ExactAnchoredPairCandidateClassificationResult {
  ExactAnchoredPairCandidateClassificationStatus status{
      ExactAnchoredPairCandidateClassificationStatus::budget_exhausted};
  ExactAnchoredPairCandidateClassificationStopReason stop_reason{
      ExactAnchoredPairCandidateClassificationStopReason::none};
  std::array<spatial::PointId, 2> support_ids{};
  std::size_t maximum_closed_rank{};
  ExactAnchoredPairCandidateClassificationBudget requested_budget{};
  ExactAnchoredPairCandidateClassificationAudit audit{};
  std::optional<ExactPairSupportEvent> event;
  std::optional<ExactPairSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostic;

  [[nodiscard]] bool complete() const noexcept {
    return status ==
        ExactAnchoredPairCandidateClassificationStatus::complete;
  }

  [[nodiscard]] bool above_rank() const noexcept {
    return status ==
        ExactAnchoredPairCandidateClassificationStatus::above_rank;
  }
};

// This class name is the narrow private-access seam required in
// spatial::MortonLbvhIndex.  The public free function below is the intended
// call site for a future anchored candidate stream.
class ExactAnchoredPairCandidateClassifier {
 public:
  [[nodiscard]] static ExactAnchoredPairCandidateClassificationResult
  classify(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::array<spatial::PointId, 2> support_ids,
      std::size_t maximum_closed_rank,
      ExactAnchoredPairCandidateClassificationBudget budget);
};

[[nodiscard]] ExactAnchoredPairCandidateClassificationResult
classify_exact_anchored_pair_candidate(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::array<spatial::PointId, 2> support_ids,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairCandidateClassificationBudget budget);

}  // namespace morsehgp3d::hierarchy
