#pragma once

#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t
    exact_anchored_pair_candidate_maximum_closed_rank = 11U;
inline constexpr std::size_t
    exact_anchored_pair_candidate_maximum_interior_count =
        exact_anchored_pair_candidate_maximum_closed_rank - 2U;
inline constexpr std::size_t
    exact_anchored_pair_candidate_maximum_frontier_entry_count =
        3U * spatial::MortonLbvhIndex::morton_bits_per_axis +
        std::numeric_limits<std::size_t>::digits + 1U;
inline constexpr std::string_view
    exact_anchored_pair_candidate_classifier_proof_basis =
        "exact_sparse_diametral_closed_ball_fixed_DFS_resume_v2";

enum class ExactAnchoredPairCandidateClassificationStatus : std::uint8_t {
  complete,
  above_rank,
  budget_exhausted,
};

enum class ExactAnchoredPairCandidateClassificationStopReason : std::uint8_t {
  none,
  node_visit_limit,
};

// This cap applies to one resumable advance.  One physical LBVH-node visit is
// charged before the frontier is popped or any scientific cursor is mutated.
// There is deliberately no independent stack budget: the fixed array is
// bounded by the certified Morton-radix depth plus the repeated-code split
// depth and cannot grow with the number of candidates.
struct ExactAnchoredPairCandidateClassificationBudget {
  std::size_t maximum_node_visit_count{};

  friend bool operator==(
      const ExactAnchoredPairCandidateClassificationBudget&,
      const ExactAnchoredPairCandidateClassificationBudget&) = default;
};

struct ExactAnchoredPairCandidateClassificationAudit {
  std::size_t advance_call_count{};
  std::size_t budget_exhaustion_count{};
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

// A complete result owns exactly one regular event or one relevant extra-shell
// diagnostic.  above_rank and budget_exhausted own neither.  The exterior is
// represented only by its exact count; the equality shell is traversed
// completely, but only its cardinality and canonical least extra witness are
// retained by the reused pair-support record types.
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

enum class ExactAnchoredPairCandidateClassificationStepKind : std::uint8_t {
  record_ready,
  above_rank,
  budget_exhausted,
  complete,
};

struct ExactAnchoredPairCandidateClassificationStepWork {
  std::size_t node_visit_count{};

  friend bool operator==(
      const ExactAnchoredPairCandidateClassificationStepWork&,
      const ExactAnchoredPairCandidateClassificationStepWork&) = default;
};

struct ExactAnchoredPairCandidateClassificationStep {
  ExactAnchoredPairCandidateClassificationStepKind kind{
      ExactAnchoredPairCandidateClassificationStepKind::budget_exhausted};
  ExactAnchoredPairCandidateClassificationStopReason stop_reason{
      ExactAnchoredPairCandidateClassificationStopReason::none};
  ExactAnchoredPairCandidateClassificationBudget requested_budget{};
  ExactAnchoredPairCandidateClassificationStepWork work{};
  bool terminal_after_step{false};

  friend bool operator==(
      const ExactAnchoredPairCandidateClassificationStep&,
      const ExactAnchoredPairCandidateClassificationStep&) = default;
};

enum class ExactAnchoredPairPendingRecordKind : std::uint8_t {
  event,
  relevant_extra_shell_diagnostic,
};

// Exact scalar capacity needed by the one record that can be emitted from a
// record_ready context.  It exposes no record payload and cannot authorize
// consumption; callers can therefore reserve atomically before take_result().
struct ExactAnchoredPairPendingRecordRequirements {
  ExactAnchoredPairPendingRecordKind kind{};
  std::size_t point_id_reference_count{};

  friend bool operator==(
      const ExactAnchoredPairPendingRecordRequirements&,
      const ExactAnchoredPairPendingRecordRequirements&) = default;
};

// One active exact closed-ball classification.  The context keeps a fixed DFS
// frontier, at most maximum_closed_rank - 2 relevant interior ids, exact
// partition counters, one prepared interval anchor and the two exact support
// bounds.  It materializes no cloud-sized output or pair universe and can
// resume after any node-visit cap.
class ExactAnchoredPairCandidateClassificationContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "resumable_exact_anchored_pair_closed_ball_classifier";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view proof_basis =
      exact_anchored_pair_candidate_classifier_proof_basis;

  [[nodiscard]] static ExactAnchoredPairCandidateClassificationContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::array<spatial::PointId, 2> support_ids,
      std::size_t maximum_closed_rank);
  [[nodiscard]] static ExactAnchoredPairCandidateClassificationContext start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::array<spatial::PointId, 2>,
      std::size_t) = delete;
  [[nodiscard]] static ExactAnchoredPairCandidateClassificationContext start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::array<spatial::PointId, 2>,
      std::size_t) = delete;

  ExactAnchoredPairCandidateClassificationContext(
      const ExactAnchoredPairCandidateClassificationContext&) = delete;
  ExactAnchoredPairCandidateClassificationContext& operator=(
      const ExactAnchoredPairCandidateClassificationContext&) = delete;
  ExactAnchoredPairCandidateClassificationContext(
      ExactAnchoredPairCandidateClassificationContext&&) noexcept = default;
  ExactAnchoredPairCandidateClassificationContext& operator=(
      ExactAnchoredPairCandidateClassificationContext&&) = delete;
  ~ExactAnchoredPairCandidateClassificationContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return cloud_identity_ != nullptr && lbvh_identity_ != nullptr;
  }

  [[nodiscard]] bool terminal() const noexcept {
    return ready() && terminal_status_.has_value();
  }

  [[nodiscard]] bool complete() const noexcept {
    return terminal() && terminal_consumed_;
  }

  [[nodiscard]] bool record_ready() const noexcept {
    return terminal() && !terminal_consumed_ &&
        *terminal_status_ ==
            ExactAnchoredPairCandidateClassificationStatus::complete;
  }

  [[nodiscard]] bool above_rank() const noexcept {
    return terminal() &&
        *terminal_status_ ==
            ExactAnchoredPairCandidateClassificationStatus::above_rank;
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

  [[nodiscard]] std::array<spatial::PointId, 2> support_ids() const noexcept {
    return support_ids_;
  }

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] const ExactAnchoredPairCandidateClassificationAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactAnchoredPairCandidateClassificationAudit& audit()
      const && = delete;

  [[nodiscard]] std::optional<ExactAnchoredPairPendingRecordRequirements>
  pending_record_requirements() const noexcept;

  [[nodiscard]] ExactAnchoredPairCandidateClassificationStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactAnchoredPairCandidateClassificationBudget budget) &;
  [[nodiscard]] ExactAnchoredPairCandidateClassificationStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactAnchoredPairCandidateClassificationBudget) && = delete;

  // Construct center and level only after the caller has reserved its output
  // capacity.  The exact pair-support ABI is moved out once, then the context
  // becomes complete.
  [[nodiscard]] ExactAnchoredPairCandidateClassificationResult take_result(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) &;
  [[nodiscard]] ExactAnchoredPairCandidateClassificationResult take_result(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&) && = delete;

 private:
  struct PrivateConstructionTag {};

  struct PreparedPhiIntervalAnchor {
    std::array<exact::detail::Binary64Interval, 3> midpoint{};
    std::array<exact::detail::Binary64Interval, 3> half_difference{};
    std::array<exact::detail::Binary64Interval, 3>
        squared_half_difference{};
  };

  ExactAnchoredPairCandidateClassificationContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::array<spatial::PointId, 2> support_ids,
      std::size_t maximum_closed_rank,
      PrivateConstructionTag);

  [[nodiscard]] static PreparedPhiIntervalAnchor prepare_interval_anchor(
      const spatial::CanonicalPointCloud& cloud,
      const std::array<spatial::PointId, 2>& support_ids);
  [[nodiscard]] static std::optional<int> filtered_phi_aabb_sign(
      const PreparedPhiIntervalAnchor& anchor,
      const spatial::ExactDyadicAabb3& query_bounds);
  [[nodiscard]] spatial::ExactDyadicAabb3 node_bounds(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t node_index) const;
  [[nodiscard]] ExactAnchoredPairCandidateClassificationResult make_result(
      ExactAnchoredPairCandidateClassificationStatus status,
      ExactAnchoredPairCandidateClassificationStopReason stop_reason,
      ExactAnchoredPairCandidateClassificationBudget budget) const;
  [[nodiscard]] ExactAnchoredPairCandidateClassificationStep make_step(
      ExactAnchoredPairCandidateClassificationStepKind kind,
      ExactAnchoredPairCandidateClassificationStopReason stop_reason,
      ExactAnchoredPairCandidateClassificationBudget budget,
      ExactAnchoredPairCandidateClassificationStepWork work) const;

  std::array<spatial::PointId, 2> support_ids_{};
  std::size_t maximum_closed_rank_{};
  spatial::ExactDyadicAabb3 first_support_bounds_{};
  spatial::ExactDyadicAabb3 second_support_bounds_{};
  PreparedPhiIntervalAnchor interval_anchor_{};
  std::array<std::size_t,
             exact_anchored_pair_candidate_maximum_frontier_entry_count>
      frontier_{};
  std::size_t frontier_entry_count_{};
  std::array<spatial::PointId,
             exact_anchored_pair_candidate_maximum_interior_count>
      interior_ids_{};
  std::size_t interior_count_{};
  std::size_t shell_count_{};
  std::optional<spatial::PointId> canonical_extra_shell_witness_id_;
  std::size_t exterior_count_{};
  std::uint8_t support_seen_mask_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;
  ExactAnchoredPairCandidateClassificationAudit audit_{};
  ExactAnchoredPairCandidateClassificationBudget terminal_requested_budget_{};
  std::optional<ExactAnchoredPairCandidateClassificationStatus>
      terminal_status_;
  bool terminal_consumed_{false};

  friend class ExactAnchoredPairCandidateClassifier;
};

// Historical one-shot wrapper.  Its public free function preserves the P8c
// API by starting the resumable context, advancing once and discarding the
// cursor on exhaustion; sparse production should own the context directly.
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
