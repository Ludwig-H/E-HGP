#pragma once

#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"
#include "morsehgp3d/hierarchy/morton_triangular_block_pair_schedule.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

// A schedule never owns a cloud-sized anchor table.  It prepares one
// contiguous Morton group and its bounded common halo at a time.  When the
// common traversal opens a frontier, P8q keeps one bounded DFS of contiguous
// anchor subranges.  At most one subgroup probe or singleton traversal and one
// additional bounded halo coexist with the suspended common context; both
// delegate the exact scientific decision to P8h/P8g.
struct ExactMortonGroupedAnchoredPairScheduleConfig {
  std::size_t maximum_anchor_count_per_group{};
  std::size_t proposed_witness_pool_size{};
  bool use_triangular_block_pair_schedule{false};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleConfig&,
      const ExactMortonGroupedAnchoredPairScheduleConfig&) = default;
};

enum class ExactMortonGroupedAnchoredPairScheduleStepKind : std::uint8_t {
  certified_prune,
  diagonal_self,
  singleton_fallback_started,
  anchor_subgroup_split,
  unresolved_leaf,
  fallback_subtree,
  budget_exhausted,
  group_complete,
  complete,
};

enum class ExactMortonGroupedAnchoredPairScheduleStopReason : std::uint8_t {
  none,
  traversal_node_visit_limit,
  traversal_exact_predicate_limit,
};

struct ExactMortonGroupedAnchoredPairScheduleStepWork {
  std::size_t traversal_node_visit_count{};
  std::size_t witness_subtree_node_visit_count{};
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  std::size_t exact_predicate_count{};
  std::size_t witness_subtree_exact_predicate_count{};
  std::size_t strict_witness_discovery_count{};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleStepWork&,
      const ExactMortonGroupedAnchoredPairScheduleStepWork&) = default;
};

struct ExactMortonGroupedAnchoredPairScheduleAudit {
  std::size_t advance_call_count{};
  std::size_t prepared_group_count{};
  std::size_t completed_group_count{};
  std::size_t scheduled_anchor_count{};
  std::size_t proposed_witness_pool_entry_count{};
  std::size_t traversal_advance_count{};
  std::size_t traversal_node_visit_count{};
  std::size_t common_traversal_node_visit_count{};
  std::size_t anchor_subgroup_node_visit_count{};
  std::size_t singleton_node_visit_count{};
  std::size_t witness_subtree_node_visit_count{};
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  std::size_t exact_predicate_count{};
  std::size_t common_exact_predicate_count{};
  std::size_t anchor_subgroup_exact_predicate_count{};
  std::size_t singleton_exact_predicate_count{};
  std::size_t witness_subtree_exact_predicate_count{};
  std::size_t witness_subtree_receipt_count{};
  std::size_t witness_subtree_success_count{};
  std::size_t witness_subtree_fail_open_count{};
  std::size_t strict_witness_discovery_count{};
  std::size_t diagonal_node_descent_count{};
  std::size_t certified_prune_count{};
  std::size_t common_frontier_count{};
  std::size_t delegated_frontier_anchor_count{};
  std::size_t prepared_anchor_subgroup_probe_count{};
  std::size_t proposed_anchor_subgroup_witness_pool_entry_count{};
  std::size_t query_facing_fallback_witness_pool_entry_count{};
  std::size_t anchor_subgroup_split_count{};
  std::size_t anchor_subgroup_certified_prune_count{};
  std::size_t anchor_subgroup_certified_anchor_count{};
  std::size_t prepared_singleton_fallback_count{};
  std::size_t completed_singleton_fallback_count{};
  std::size_t proposed_singleton_witness_pool_entry_count{};
  std::size_t singleton_certified_prune_count{};
  std::size_t unresolved_leaf_count{};
  std::size_t fallback_subtree_count{};
  std::size_t budget_exhaustion_count{};
  std::size_t maximum_active_anchor_count{};
  std::size_t maximum_active_witness_pool_entry_count{};
  std::size_t maximum_pending_anchor_subgroup_count{};
  std::size_t triangular_block_pair_visit_count{};
  std::size_t triangular_diagonal_split_count{};
  std::size_t triangular_oversized_anchor_split_count{};
  std::size_t triangular_self_pair_count{};
  std::size_t triangular_cross_block_count{};
  std::size_t triangular_certified_cross_block_count{};
  std::size_t triangular_certified_unordered_pair_count{};
  std::size_t triangular_opened_singleton_cross_block_count{};
  std::size_t triangular_opened_unordered_pair_count{};
  std::size_t triangular_maximum_pending_block_pair_count{};
  bool complete{false};
  bool morton_anchor_partition_complete{false};
  bool triangular_partition_complete{false};
  bool no_global_anchor_pair_or_output_arena_materialized{true};
  bool no_dynamic_dual_tree_or_pair_arena_materialized{true};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleAudit&,
      const ExactMortonGroupedAnchoredPairScheduleAudit&) = default;
};

// Every scientific step snapshots the active contiguous anchor subrange it
// needs.  A terminal singleton therefore snapshots only its own anchor and
// Morton leaf; a certified subgroup prune snapshots that whole subrange.  The
// witness pool is copied only for a prune or group boundary.  Budget exhaustion
// copies neither fixed array, while frontier/split events carry no witness pool
// and no positive authority.  Positive authority remains only in the nested
// P8h step and P8g certificate.  A caller must orient every unresolved range as
// p < q and classify it exactly; routing events are not Morse decisions.
class ExactMortonGroupedAnchoredPairScheduleStep {
 public:
  ExactMortonGroupedAnchoredPairScheduleStep(
      const ExactMortonGroupedAnchoredPairScheduleStep&) = default;
  ExactMortonGroupedAnchoredPairScheduleStep(
      ExactMortonGroupedAnchoredPairScheduleStep&&) noexcept = default;
  ExactMortonGroupedAnchoredPairScheduleStep& operator=(
      const ExactMortonGroupedAnchoredPairScheduleStep&) = default;
  ExactMortonGroupedAnchoredPairScheduleStep& operator=(
      ExactMortonGroupedAnchoredPairScheduleStep&&) noexcept = default;
  ~ExactMortonGroupedAnchoredPairScheduleStep() = default;

  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStepKind kind() const
      noexcept {
    return kind_;
  }

  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason()
      const noexcept {
    return stop_reason_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairTraversalWorkBudget&
  requested_traversal_budget() const & noexcept {
    return requested_traversal_budget_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairTraversalWorkBudget&
  requested_traversal_budget() const && = delete;

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleStepWork& work()
      const & noexcept {
    return work_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleStepWork& work()
      const && = delete;

  [[nodiscard]] std::optional<std::size_t> group_ordinal() const noexcept {
    return group_ordinal_;
  }

  [[nodiscard]] std::optional<std::size_t> anchor_leaf_begin() const noexcept {
    return anchor_leaf_begin_;
  }

  [[nodiscard]] std::optional<std::size_t> anchor_leaf_end() const noexcept {
    return anchor_leaf_end_;
  }

  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const
      & noexcept {
    return {anchor_point_ids_.data(), anchor_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const & noexcept {
    return {witness_pool_point_ids_.data(), witness_pool_entry_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const && = delete;

  [[nodiscard]] const ExactGroupedAnchoredPairTraversalStep* traversal_step()
      const & noexcept {
    return traversal_step_.has_value() ? &*traversal_step_ : nullptr;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairTraversalStep* traversal_step()
      const && = delete;

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleStep&,
      const ExactMortonGroupedAnchoredPairScheduleStep&) = default;

 private:
  ExactMortonGroupedAnchoredPairScheduleStep() = default;

  ExactMortonGroupedAnchoredPairScheduleStepKind kind_{
      ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted};
  ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason_{
      ExactMortonGroupedAnchoredPairScheduleStopReason::none};
  ExactGroupedAnchoredPairTraversalWorkBudget requested_traversal_budget_{};
  ExactMortonGroupedAnchoredPairScheduleStepWork work_{};
  std::optional<std::size_t> group_ordinal_;
  std::optional<std::size_t> anchor_leaf_begin_;
  std::optional<std::size_t> anchor_leaf_end_;
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_ids_{};
  std::size_t anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_pool_point_ids_{};
  std::size_t witness_pool_entry_count_{};
  std::optional<ExactGroupedAnchoredPairTraversalStep> traversal_step_;

  friend class ExactMortonGroupedAnchoredPairScheduleContext;
};

class ExactMortonGroupedAnchoredPairScheduleContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "bounded_morton_group_schedule";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view proof_basis =
      "contiguous_morton_anchor_partition_bounded_alternating_halo_and_"
      "common_first_recursive_anchor_subrange_partition_then_singleton_"
      "fallback_v3";

  [[nodiscard]] static ExactMortonGroupedAnchoredPairScheduleContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig config);
  [[nodiscard]] static ExactMortonGroupedAnchoredPairScheduleContext start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig) = delete;
  [[nodiscard]] static ExactMortonGroupedAnchoredPairScheduleContext start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig) = delete;

  ExactMortonGroupedAnchoredPairScheduleContext(
      const ExactMortonGroupedAnchoredPairScheduleContext&) = delete;
  ExactMortonGroupedAnchoredPairScheduleContext& operator=(
      const ExactMortonGroupedAnchoredPairScheduleContext&) = delete;
  ExactMortonGroupedAnchoredPairScheduleContext(
      ExactMortonGroupedAnchoredPairScheduleContext&&) noexcept = default;
  ExactMortonGroupedAnchoredPairScheduleContext& operator=(
      ExactMortonGroupedAnchoredPairScheduleContext&&) = delete;
  ~ExactMortonGroupedAnchoredPairScheduleContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return cloud_identity_ != nullptr && lbvh_identity_ != nullptr;
  }

  [[nodiscard]] bool complete() const noexcept {
    return ready() && complete_;
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig& config()
      const & noexcept {
    return config_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig& config()
      const && = delete;

  [[nodiscard]] std::optional<std::size_t> active_group_ordinal() const
      noexcept {
    return active_traversal_.has_value()
        ? std::optional<std::size_t>{active_group_ordinal_}
        : std::nullopt;
  }

  [[nodiscard]] std::optional<std::size_t> active_anchor_leaf_begin() const
      noexcept {
    return active_traversal_.has_value()
        ? std::optional<std::size_t>{active_anchor_leaf_begin_}
        : std::nullopt;
  }

  [[nodiscard]] std::optional<std::size_t> active_anchor_leaf_end() const
      noexcept {
    return active_traversal_.has_value()
        ? std::optional<std::size_t>{active_anchor_leaf_end_}
        : std::nullopt;
  }

  [[nodiscard]] std::span<const spatial::PointId> active_anchor_point_ids()
      const & noexcept {
    return {active_anchor_point_ids_.data(), active_anchor_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> active_anchor_point_ids()
      const && = delete;

  [[nodiscard]] std::span<const spatial::PointId>
  active_witness_pool_point_ids() const & noexcept {
    return {active_witness_pool_point_ids_.data(),
            active_witness_pool_entry_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId>
  active_witness_pool_point_ids() const && = delete;

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit& audit()
      const && = delete;

  // The supplied budget charges P8h traversal work only.  Preparing a group or
  // one DFS anchor subrange is fixed by G <= 32 and W <= 64; every pool and
  // subrange preparation is reported separately by the audit.
  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) &;
  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactGroupedAnchoredPairTraversalWorkBudget) && = delete;

 private:
  struct PrivateConstructionTag {};

  struct PendingAnchorLeafRange {
    std::size_t begin{};
    std::size_t end{};
  };

  ExactMortonGroupedAnchoredPairScheduleContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig config,
      PrivateConstructionTag);

  void prepare_group(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t anchor_leaf_begin);

  void prepare_triangular_cross_block(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const ExactMortonTriangularBlockPairScheduleStep* structural_step,
      bool terminal_singleton);

  void synchronize_triangular_audit();

  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStep
  advance_triangular(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget);

  void prepare_next_anchor_partition_fallback(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);

  void push_anchor_subrange_children(
      std::size_t anchor_leaf_begin,
      std::size_t anchor_leaf_end);

  [[nodiscard]] ExactMortonGroupedAnchoredPairScheduleStep snapshot_step(
      ExactMortonGroupedAnchoredPairScheduleStepKind kind,
      ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason,
      ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) const;

  std::size_t maximum_closed_rank_{};
  ExactMortonGroupedAnchoredPairScheduleConfig config_{};
  std::size_t point_count_{};
  std::size_t active_group_ordinal_{};
  std::size_t active_anchor_leaf_begin_{};
  std::size_t active_anchor_leaf_end_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      active_anchor_point_ids_{};
  std::size_t active_anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      active_witness_pool_point_ids_{};
  std::size_t active_witness_pool_entry_count_{};
  std::optional<ExactGroupedAnchoredPairTraversalContext> active_traversal_;
  std::optional<ExactGroupedAnchoredPairTraversalContext>
      active_singleton_traversal_;
  std::optional<ExactMortonTriangularBlockPairScheduleContext>
      triangular_schedule_;
  std::size_t singleton_frontier_node_index_{};
  std::size_t singleton_frontier_leaf_begin_{};
  std::size_t singleton_frontier_leaf_end_{};
  std::array<PendingAnchorLeafRange,
             exact_grouped_anchored_pair_maximum_anchor_count>
      pending_anchor_subranges_{};
  std::size_t pending_anchor_subrange_count_{};
  std::size_t active_fallback_anchor_leaf_begin_{};
  std::size_t active_fallback_anchor_leaf_end_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      active_fallback_anchor_point_ids_{};
  std::size_t active_fallback_anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      active_singleton_witness_pool_point_ids_{};
  std::size_t active_singleton_witness_pool_entry_count_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;
  ExactMortonGroupedAnchoredPairScheduleAudit audit_{};
  bool singleton_frontier_active_{false};
  bool active_fallback_is_singleton_{false};
  bool group_completion_pending_{false};
  bool triangular_probe_active_{false};
  bool triangular_terminal_active_{false};
  bool complete_{false};
};

}  // namespace morsehgp3d::hierarchy
