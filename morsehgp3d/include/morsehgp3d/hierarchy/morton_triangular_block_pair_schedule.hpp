#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t
    exact_morton_triangular_block_pair_maximum_anchor_count = 32U;
inline constexpr std::size_t
    exact_morton_triangular_block_pair_maximum_lbvh_depth =
        3U * spatial::MortonLbvhIndex::morton_bits_per_axis +
        std::numeric_limits<std::size_t>::digits;
inline constexpr std::size_t
    exact_morton_triangular_block_pair_maximum_pending_count =
        3U * exact_morton_triangular_block_pair_maximum_lbvh_depth + 1U;

// P8s walks the upper triangle of one certified native LBVH.  A diagonal
// authority T(N) is partitioned as T(L), C(L,R), T(R).  A cross authority is
// either closed by a consumer certificate or split along a native LBVH node.
// The optional symmetric policy splits the larger side (query first on a
// tie), while the default preserves the Phase 14S anchor-only behavior.  A
// separate opt-in changes only traversal order by visiting C(L,R) first.  The
// fixed stack is independent of the number of points and no dual-tree or
// output-pair arena is retained.
struct ExactMortonTriangularBlockPairScheduleConfig {
  std::size_t maximum_anchor_count_per_cross_block{};
  bool use_symmetric_inconclusive_cross_block_splitting{false};
  bool prioritize_cross_blocks{false};

  friend bool operator==(
      const ExactMortonTriangularBlockPairScheduleConfig&,
      const ExactMortonTriangularBlockPairScheduleConfig&) = default;
};

struct ExactMortonTriangularBlockPairScheduleBudget {
  std::size_t maximum_block_pair_visit_count{};

  friend bool operator==(
      const ExactMortonTriangularBlockPairScheduleBudget&,
      const ExactMortonTriangularBlockPairScheduleBudget&) = default;
};

enum class ExactMortonTriangularBlockPairScheduleStepKind : std::uint8_t {
  diagonal_self,
  cross_block,
  budget_exhausted,
  complete,
};

enum class ExactMortonTriangularBlockPairScheduleStopReason : std::uint8_t {
  none,
  block_pair_visit_limit,
};

struct ExactMortonTriangularBlockPairScheduleStepWork {
  std::size_t block_pair_visit_count{};
  std::size_t diagonal_split_count{};
  std::size_t oversized_anchor_split_count{};

  friend bool operator==(
      const ExactMortonTriangularBlockPairScheduleStepWork&,
      const ExactMortonTriangularBlockPairScheduleStepWork&) = default;
};

struct ExactMortonTriangularBlockPairScheduleAudit {
  std::size_t advance_call_count{};
  std::size_t response_call_count{};
  std::size_t block_pair_visit_count{};
  std::size_t diagonal_split_count{};
  std::size_t oversized_anchor_split_count{};
  std::size_t emitted_diagonal_self_count{};
  std::size_t emitted_cross_block_count{};
  std::size_t certified_cross_block_count{};
  std::size_t certified_unordered_pair_count{};
  std::size_t inconclusive_cross_block_count{};
  std::size_t consumer_anchor_split_count{};
  std::size_t consumer_query_split_count{};
  std::size_t opened_singleton_cross_block_count{};
  std::size_t opened_unordered_pair_count{};
  std::size_t maximum_pending_block_pair_count{};
  std::size_t budget_exhaustion_count{};
  bool complete{false};
  bool triangular_partition_complete{false};
  bool no_dynamic_dual_tree_or_pair_arena_materialized{true};

  friend bool operator==(
      const ExactMortonTriangularBlockPairScheduleAudit&,
      const ExactMortonTriangularBlockPairScheduleAudit&) = default;
};

class ExactMortonTriangularBlockPairScheduleStep {
 public:
  ExactMortonTriangularBlockPairScheduleStep(
      const ExactMortonTriangularBlockPairScheduleStep&) = default;
  ExactMortonTriangularBlockPairScheduleStep(
      ExactMortonTriangularBlockPairScheduleStep&&) noexcept = default;
  ExactMortonTriangularBlockPairScheduleStep& operator=(
      const ExactMortonTriangularBlockPairScheduleStep&) = default;
  ExactMortonTriangularBlockPairScheduleStep& operator=(
      ExactMortonTriangularBlockPairScheduleStep&&) noexcept = default;
  ~ExactMortonTriangularBlockPairScheduleStep() = default;

  [[nodiscard]] ExactMortonTriangularBlockPairScheduleStepKind kind() const
      noexcept {
    return kind_;
  }

  [[nodiscard]] ExactMortonTriangularBlockPairScheduleStopReason stop_reason()
      const noexcept {
    return stop_reason_;
  }

  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleBudget&
  requested_budget() const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleBudget&
  requested_budget() const && = delete;

  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleStepWork& work()
      const & noexcept {
    return work_;
  }
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleStepWork& work()
      const && = delete;

  [[nodiscard]] std::optional<std::size_t> anchor_node_index() const noexcept {
    return anchor_node_index_;
  }
  [[nodiscard]] std::optional<std::size_t> anchor_leaf_begin() const noexcept {
    return anchor_leaf_begin_;
  }
  [[nodiscard]] std::optional<std::size_t> anchor_leaf_end() const noexcept {
    return anchor_leaf_end_;
  }
  [[nodiscard]] std::optional<std::size_t> query_node_index() const noexcept {
    return query_node_index_;
  }
  [[nodiscard]] std::optional<std::size_t> query_leaf_begin() const noexcept {
    return query_leaf_begin_;
  }
  [[nodiscard]] std::optional<std::size_t> query_leaf_end() const noexcept {
    return query_leaf_end_;
  }
  [[nodiscard]] std::optional<spatial::PointId> self_point_id() const noexcept {
    return self_point_id_;
  }

  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const &
      noexcept {
    return {anchor_point_ids_.data(), anchor_point_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const && =
      delete;

  [[nodiscard]] bool schedule_complete_after_step() const noexcept {
    return schedule_complete_after_step_;
  }

  friend bool operator==(
      const ExactMortonTriangularBlockPairScheduleStep&,
      const ExactMortonTriangularBlockPairScheduleStep&) = default;

 private:
  ExactMortonTriangularBlockPairScheduleStep() = default;

  ExactMortonTriangularBlockPairScheduleStepKind kind_{
      ExactMortonTriangularBlockPairScheduleStepKind::budget_exhausted};
  ExactMortonTriangularBlockPairScheduleStopReason stop_reason_{
      ExactMortonTriangularBlockPairScheduleStopReason::none};
  ExactMortonTriangularBlockPairScheduleBudget requested_budget_{};
  ExactMortonTriangularBlockPairScheduleStepWork work_{};
  std::optional<std::size_t> anchor_node_index_;
  std::optional<std::size_t> anchor_leaf_begin_;
  std::optional<std::size_t> anchor_leaf_end_;
  std::optional<std::size_t> query_node_index_;
  std::optional<std::size_t> query_leaf_begin_;
  std::optional<std::size_t> query_leaf_end_;
  std::optional<spatial::PointId> self_point_id_;
  std::array<spatial::PointId,
             exact_morton_triangular_block_pair_maximum_anchor_count>
      anchor_point_ids_{};
  std::size_t anchor_point_count_{};
  bool schedule_complete_after_step_{false};

  friend class ExactMortonTriangularBlockPairScheduleContext;
};

enum class ExactMortonTriangularBlockPairDecision : std::uint8_t {
  certified,
  inconclusive,
};

enum class ExactMortonTriangularBlockPairResponseKind : std::uint8_t {
  certified_closed,
  anchor_split,
  query_split,
  terminal_singleton_cross_block,
};

class ExactMortonTriangularBlockPairResponse {
 public:
  [[nodiscard]] ExactMortonTriangularBlockPairResponseKind kind() const
      noexcept {
    return kind_;
  }
  [[nodiscard]] std::size_t anchor_node_index() const noexcept {
    return anchor_node_index_;
  }
  [[nodiscard]] std::size_t anchor_leaf_begin() const noexcept {
    return anchor_leaf_begin_;
  }
  [[nodiscard]] std::size_t anchor_leaf_end() const noexcept {
    return anchor_leaf_end_;
  }
  [[nodiscard]] std::size_t query_node_index() const noexcept {
    return query_node_index_;
  }
  [[nodiscard]] std::size_t query_leaf_begin() const noexcept {
    return query_leaf_begin_;
  }
  [[nodiscard]] std::size_t query_leaf_end() const noexcept {
    return query_leaf_end_;
  }
  [[nodiscard]] std::size_t certified_unordered_pair_count() const noexcept {
    return certified_unordered_pair_count_;
  }
  [[nodiscard]] std::size_t opened_unordered_pair_count() const noexcept {
    return opened_unordered_pair_count_;
  }
  [[nodiscard]] std::optional<spatial::PointId> terminal_anchor_point_id()
      const noexcept {
    return terminal_anchor_point_id_;
  }
  [[nodiscard]] bool schedule_complete_after_response() const noexcept {
    return schedule_complete_after_response_;
  }

  friend bool operator==(
      const ExactMortonTriangularBlockPairResponse&,
      const ExactMortonTriangularBlockPairResponse&) = default;

 private:
  ExactMortonTriangularBlockPairResponse() = default;

  ExactMortonTriangularBlockPairResponseKind kind_{
      ExactMortonTriangularBlockPairResponseKind::certified_closed};
  std::size_t anchor_node_index_{};
  std::size_t anchor_leaf_begin_{};
  std::size_t anchor_leaf_end_{};
  std::size_t query_node_index_{};
  std::size_t query_leaf_begin_{};
  std::size_t query_leaf_end_{};
  std::size_t certified_unordered_pair_count_{};
  std::size_t opened_unordered_pair_count_{};
  std::optional<spatial::PointId> terminal_anchor_point_id_;
  bool schedule_complete_after_response_{false};

  friend class ExactMortonTriangularBlockPairScheduleContext;
};

class ExactMortonTriangularBlockPairScheduleContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "bounded_morton_triangular_block_pair_schedule";
  static constexpr std::string_view deployment_status = "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view phase_marker = "phase14S_P8s";
  static constexpr std::string_view proof_basis =
      "native_lbvh_upper_triangle_TL_CLR_TR_fixed_stack_optional_mass_balanced_opening_v2";

  [[nodiscard]] static ExactMortonTriangularBlockPairScheduleContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactMortonTriangularBlockPairScheduleConfig config);
  [[nodiscard]] static ExactMortonTriangularBlockPairScheduleContext start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      ExactMortonTriangularBlockPairScheduleConfig) = delete;
  [[nodiscard]] static ExactMortonTriangularBlockPairScheduleContext start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      ExactMortonTriangularBlockPairScheduleConfig) = delete;

  ExactMortonTriangularBlockPairScheduleContext(
      const ExactMortonTriangularBlockPairScheduleContext&) = delete;
  ExactMortonTriangularBlockPairScheduleContext& operator=(
      const ExactMortonTriangularBlockPairScheduleContext&) = delete;
  ExactMortonTriangularBlockPairScheduleContext(
      ExactMortonTriangularBlockPairScheduleContext&&) noexcept = default;
  ExactMortonTriangularBlockPairScheduleContext& operator=(
      ExactMortonTriangularBlockPairScheduleContext&&) = delete;
  ~ExactMortonTriangularBlockPairScheduleContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return lbvh_identity_ != nullptr;
  }
  [[nodiscard]] bool complete() const noexcept {
    return ready() && complete_;
  }
  [[nodiscard]] bool awaiting_cross_block_response() const noexcept {
    return ready() && awaiting_cross_block_.has_value();
  }
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;
  [[nodiscard]] std::size_t point_count() const noexcept {
    return point_count_;
  }
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleConfig& config()
      const & noexcept {
    return config_;
  }
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleConfig& config()
      const && = delete;
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactMortonTriangularBlockPairScheduleAudit& audit()
      const && = delete;

  // The cap is per call.  Each charged visit pops exactly one recertified
  // authority before it can mutate the fixed structural cursor.
  [[nodiscard]] ExactMortonTriangularBlockPairScheduleStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactMortonTriangularBlockPairScheduleBudget budget) &;
  [[nodiscard]] ExactMortonTriangularBlockPairScheduleStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactMortonTriangularBlockPairScheduleBudget) && = delete;

  // A certified response closes |A||Q| unordered pairs.  By default an
  // inconclusive response preserves Phase 14S and splits native A only.  The
  // opt-in symmetric policy splits the larger non-leaf side, choosing Q on a
  // tie, and opens a terminal authority only at singleton x singleton.
  [[nodiscard]] ExactMortonTriangularBlockPairResponse respond_to_cross_block(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactMortonTriangularBlockPairDecision decision) &;
  [[nodiscard]] ExactMortonTriangularBlockPairResponse respond_to_cross_block(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactMortonTriangularBlockPairDecision) && = delete;

 private:
  enum class PendingKind : std::uint8_t {
    diagonal,
    cross,
  };

  struct PendingBlockPair {
    PendingKind kind{PendingKind::diagonal};
    std::size_t anchor_node_index{};
    std::size_t anchor_leaf_begin{};
    std::size_t anchor_leaf_end{};
    std::size_t query_node_index{};
    std::size_t query_leaf_begin{};
    std::size_t query_leaf_end{};
  };

  struct PrivateConstructionTag {};

  ExactMortonTriangularBlockPairScheduleContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactMortonTriangularBlockPairScheduleConfig config,
      PrivateConstructionTag);

  void validate_authority(
      const spatial::MortonLbvhIndex& index,
      const PendingBlockPair& authority) const;
  void push_authority(const PendingBlockPair& authority);
  void push_diagonal_partition(
      const spatial::MortonLbvhIndex& index,
      const PendingBlockPair& authority);
  void push_anchor_partition(
      const spatial::MortonLbvhIndex& index,
      const PendingBlockPair& authority);
  void push_query_partition(
      const spatial::MortonLbvhIndex& index,
      const PendingBlockPair& authority);
  void finish_if_partition_complete();

  ExactMortonTriangularBlockPairScheduleConfig config_{};
  std::size_t point_count_{};
  std::array<PendingBlockPair,
             exact_morton_triangular_block_pair_maximum_pending_count>
      pending_block_pairs_{};
  std::size_t pending_block_pair_count_{};
  std::optional<PendingBlockPair> awaiting_cross_block_;
  std::shared_ptr<const void> lbvh_identity_;
  ExactMortonTriangularBlockPairScheduleAudit audit_{};
  bool complete_{false};
};

}  // namespace morsehgp3d::hierarchy
