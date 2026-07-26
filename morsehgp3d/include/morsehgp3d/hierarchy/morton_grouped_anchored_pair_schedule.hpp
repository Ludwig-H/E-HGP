#pragma once

#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

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
// common traversal opens a frontier, that context can coexist with at most one
// singleton traversal and one additional bounded halo; both delegate the exact
// scientific decision to P8h/P8g.
struct ExactMortonGroupedAnchoredPairScheduleConfig {
  std::size_t maximum_anchor_count_per_group{};
  std::size_t proposed_witness_pool_size{};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleConfig&,
      const ExactMortonGroupedAnchoredPairScheduleConfig&) = default;
};

enum class ExactMortonGroupedAnchoredPairScheduleStepKind : std::uint8_t {
  certified_prune,
  singleton_fallback_started,
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
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  std::size_t exact_predicate_count{};
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
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  std::size_t exact_predicate_count{};
  std::size_t strict_witness_discovery_count{};
  std::size_t diagonal_node_descent_count{};
  std::size_t certified_prune_count{};
  std::size_t common_frontier_count{};
  std::size_t prepared_singleton_fallback_count{};
  std::size_t proposed_singleton_witness_pool_entry_count{};
  std::size_t singleton_certified_prune_count{};
  std::size_t unresolved_leaf_count{};
  std::size_t fallback_subtree_count{};
  std::size_t budget_exhaustion_count{};
  std::size_t maximum_active_anchor_count{};
  std::size_t maximum_active_witness_pool_entry_count{};
  bool complete{false};
  bool morton_anchor_partition_complete{false};
  bool no_global_anchor_pair_or_output_arena_materialized{true};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairScheduleAudit&,
      const ExactMortonGroupedAnchoredPairScheduleAudit&) = default;
};

// Every scientific step snapshots the active anchors it needs.  A singleton
// terminal snapshots only its own anchor and Morton range.  The witness pool is
// copied only for a prune or group boundary; budget exhaustion copies neither
// fixed array, while the internal singleton-fallback frontier identifies the
// bounded group but carries no witness pool.  Positive authority is still
// carried only by the nested P8h step and its P8g certificate.  A caller must
// orient an unresolved leaf or fallback range as p < q and pass each resulting
// candidate to the exact anchored classifier; neither is a Morse decision by
// itself.
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
      "common_first_per_anchor_singleton_fallback_partition_v2";

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

  // The supplied budget charges P8h traversal work only.  Preparing the next
  // group is fixed by G <= 32 and W <= 64 and is reported separately through
  // prepared_group_count and proposed_witness_pool_entry_count.
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

  void prepare_next_singleton_fallback(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);

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
  std::size_t singleton_frontier_node_index_{};
  std::size_t singleton_frontier_leaf_begin_{};
  std::size_t singleton_frontier_leaf_end_{};
  std::size_t next_singleton_anchor_offset_{};
  std::size_t active_singleton_anchor_offset_{};
  std::size_t active_singleton_anchor_leaf_index_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      active_singleton_witness_pool_point_ids_{};
  std::size_t active_singleton_witness_pool_entry_count_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;
  ExactMortonGroupedAnchoredPairScheduleAudit audit_{};
  bool singleton_frontier_active_{false};
  bool group_completion_pending_{false};
  bool complete_{false};
};

}  // namespace morsehgp3d::hierarchy
