#pragma once

#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/query.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu {
class K1BoruvkaCandidateContext;
class PairSupportPhiContext;
class SpatialLbvhContext;
}

namespace morsehgp3d::spatial {
class CanonicalPointCloud;
class MortonLbvhIndex;
}

namespace morsehgp3d::hierarchy {
class ExactHigherSupportStreamBuilder;
class ExactPairSupportStreamBuilder;
class ExactDirectSparseFirstIncidenceBuilder;
struct K1ExactBoruvkaResult;
struct K1BoruvkaVerification;
[[nodiscard]] K1ExactBoruvkaResult build_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud);
[[nodiscard]] K1BoruvkaVerification verify_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& result);
}

namespace morsehgp3d::spatial {

struct MortonLeafRecord {
  std::uint64_t morton_code;
  PointId point_id;

  friend bool operator==(const MortonLeafRecord&, const MortonLeafRecord&) = default;
};

struct MortonLbvhBuildCounters {
  std::size_t point_count{0U};
  std::size_t node_count{0U};
  std::size_t maximum_depth{0U};
  std::size_t morton_collision_group_count{0U};
  std::size_t maximum_morton_collision_size{0U};

  friend bool operator==(
      const MortonLbvhBuildCounters&,
      const MortonLbvhBuildCounters&) = default;
};

enum class LbvhTraversalOrder : std::uint8_t {
  near_first,
  far_first,
};

enum class ExactLbvhTopKStatus : std::uint8_t {
  not_certified,
  complete,
  budget_exhausted,
};

enum class ExactLbvhTopKStopReason : std::uint8_t {
  none,
  node_visit_limit,
  internal_node_expansion_limit,
  exact_aabb_bound_evaluation_limit,
  exact_point_distance_evaluation_limit,
  frontier_entry_limit,
  best_neighbor_entry_limit,
  cutoff_shell_entry_limit,
};

// Every cap is checked before the corresponding operation or allocation.
// The shell cap bounds retained PointIds at the current exact cutoff; an
// overflowing provisional shell is discarded only if that cutoff later
// decreases, and is a terminal exhaustion otherwise.
// If several zero/short caps block the same transition, the deterministic
// priority is best-neighbor, frontier, then root AABB during preflight, and
// then incumbent distance when proposals are supplied.  At an internal node,
// the priority is internal expansion, frontier, then child AABBs.
struct ExactLbvhTopKBudget {
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_internal_node_expansion_count{};
  std::size_t maximum_exact_aabb_bound_evaluation_count{};
  std::size_t maximum_exact_point_distance_evaluation_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_best_neighbor_entry_count{};
  std::size_t maximum_cutoff_shell_entry_count{};

  friend bool operator==(
      const ExactLbvhTopKBudget&,
      const ExactLbvhTopKBudget&) = default;
};

// Operational audit only: an exhausted result intentionally exposes neither a
// cutoff nor any shell PointId.  Retained shell storage never exceeds the
// requested cap, including while a provisional shell has overflowed.
struct ExactLbvhTopKAudit {
  std::size_t node_visit_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t exact_aabb_bound_evaluation_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t supplied_incumbent_point_count{};
  std::size_t exact_incumbent_distance_evaluation_count{};
  std::size_t peak_frontier_entry_count{};
  std::size_t peak_best_neighbor_entry_count{};
  std::size_t peak_retained_cutoff_shell_entry_count{};
  std::size_t provisional_cutoff_decrease_count{};
  std::size_t provisional_cutoff_shell_overflow_count{};
  bool traversal_complete{false};

  friend bool operator==(
      const ExactLbvhTopKAudit&,
      const ExactLbvhTopKAudit&) = default;
};

class ExactBudgetedLbvhTopKResult {
 public:
  ExactBudgetedLbvhTopKResult(const ExactBudgetedLbvhTopKResult&) = default;
  ExactBudgetedLbvhTopKResult& operator=(
      const ExactBudgetedLbvhTopKResult&) = default;
  ExactBudgetedLbvhTopKResult(
      ExactBudgetedLbvhTopKResult&& other) noexcept;
  ExactBudgetedLbvhTopKResult& operator=(
      ExactBudgetedLbvhTopKResult&& other) noexcept;

  [[nodiscard]] ExactLbvhTopKStatus status() const noexcept {
    return status_;
  }
  [[nodiscard]] ExactLbvhTopKStopReason stop_reason() const noexcept {
    return stop_reason_;
  }
  [[nodiscard]] bool complete() const noexcept {
    return status_ == ExactLbvhTopKStatus::complete;
  }
  [[nodiscard]] const ExactLbvhTopKBudget& requested_budget() const noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactLbvhTopKAudit& audit() const noexcept {
    return audit_;
  }
  [[nodiscard]] const TopKPartition& partition() const &;
  [[nodiscard]] const TopKPartition& partition() const && = delete;

 private:
  ExactBudgetedLbvhTopKResult(
      ExactLbvhTopKStatus status,
      ExactLbvhTopKStopReason stop_reason,
      ExactLbvhTopKBudget requested_budget,
      ExactLbvhTopKAudit audit,
      std::optional<TopKPartition> partition);

  ExactLbvhTopKStatus status_{ExactLbvhTopKStatus::not_certified};
  ExactLbvhTopKStopReason stop_reason_{ExactLbvhTopKStopReason::none};
  ExactLbvhTopKBudget requested_budget_{};
  ExactLbvhTopKAudit audit_{};
  std::optional<TopKPartition> partition_;

  friend ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      ExactLbvhTopKBudget budget,
      LbvhTraversalOrder traversal_order);
  friend ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      std::span<const PointId> incumbent_point_ids,
      ExactLbvhTopKBudget budget,
      LbvhTraversalOrder traversal_order);
};

class MortonLbvhIndex {
 public:
  static constexpr std::size_t morton_bits_per_axis = 21U;

  [[nodiscard]] static MortonLbvhIndex build(
      const CanonicalPointCloud& cloud);

  MortonLbvhIndex(const MortonLbvhIndex&) = delete;
  MortonLbvhIndex& operator=(const MortonLbvhIndex&) = delete;
  MortonLbvhIndex(MortonLbvhIndex&& other) noexcept;
  MortonLbvhIndex& operator=(MortonLbvhIndex&& other) noexcept;

  [[nodiscard]] bool ready() const noexcept {
    return structure_complete_ && cloud_identity_ != nullptr;
  }
  [[nodiscard]] bool validated_for(
      const CanonicalPointCloud& cloud) const noexcept {
    return ready() && cloud_identity_ == cloud.identity_;
  }
  [[nodiscard]] std::span<const MortonLeafRecord> leaves() const & noexcept {
    return ready() ? std::span<const MortonLeafRecord>{leaves_}
                   : std::span<const MortonLeafRecord>{};
  }
  [[nodiscard]] std::span<const MortonLeafRecord> leaves() const && = delete;
  [[nodiscard]] const MortonLbvhBuildCounters& build_counters() const & {
    if (!ready()) {
      throw std::logic_error("a moved-from Morton LBVH has no counters");
    }
    return build_counters_;
  }
  [[nodiscard]] const MortonLbvhBuildCounters& build_counters() const && = delete;
  [[nodiscard]] const ExactDyadicAabb3& root_aabb() const & {
    if (!ready()) {
      throw std::logic_error("a moved-from Morton LBVH has no root AABB");
    }
    return root_aabb_;
  }
  [[nodiscard]] const ExactDyadicAabb3& root_aabb() const && = delete;

 private:
  static constexpr std::size_t invalid_node_index =
      static_cast<std::size_t>(-1);

  struct Node {
    std::array<PointId, 3> lower_point_ids{};
    std::array<PointId, 3> upper_point_ids{};
    std::size_t left_child{invalid_node_index};
    std::size_t right_child{invalid_node_index};
    std::size_t leaf_begin{0U};
    std::size_t leaf_end{0U};

    [[nodiscard]] bool is_leaf() const noexcept {
      return left_child == invalid_node_index;
    }
  };

  MortonLbvhIndex() = default;

  [[nodiscard]] std::size_t build_range(
      const CanonicalPointCloud& cloud,
      std::size_t begin,
      std::size_t end,
      std::size_t depth);
  [[nodiscard]] std::size_t find_split(
      std::size_t begin,
      std::size_t end) const;
  void validate_structure(const CanonicalPointCloud& cloud) const;
  [[nodiscard]] std::size_t eligible_count_in_node(
      const Node& node,
      const ExclusionSet& exclusions) const;
  [[nodiscard]] exact::ExactLevel minimum_squared_distance_to_node(
      const CanonicalPointCloud& cloud,
      std::size_t node_index,
      const exact::ExactRational3& query) const;
  [[nodiscard]] exact::ExactLevel maximum_squared_distance_to_node(
      const CanonicalPointCloud& cloud,
      std::size_t node_index,
      const exact::ExactRational3& query) const;
  std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity_;
  std::size_t point_count_{0U};
  std::vector<MortonLeafRecord> leaves_;
  std::vector<std::size_t> leaf_position_by_point_id_;
  std::vector<Node> nodes_;
  std::size_t root_index_{invalid_node_index};
  MortonLbvhBuildCounters build_counters_;
  ExactDyadicAabb3 root_aabb_{};
  bool structure_complete_{false};

  friend TopKPartition lbvh_top_k(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      LbvhTraversalOrder traversal_order);
  friend ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      ExactLbvhTopKBudget budget,
      LbvhTraversalOrder traversal_order);
  friend ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      std::size_t requested_rank,
      const ExclusionSet& exclusions,
      std::span<const PointId> incumbent_point_ids,
      ExactLbvhTopKBudget budget,
      LbvhTraversalOrder traversal_order);
  friend ClosedBallPartition lbvh_closed_ball(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_radius,
      LbvhTraversalOrder traversal_order);
  friend hierarchy::K1ExactBoruvkaResult
  hierarchy::build_exact_lbvh_boruvka(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);
  friend hierarchy::K1BoruvkaVerification
  hierarchy::verify_exact_lbvh_boruvka(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const hierarchy::K1ExactBoruvkaResult& result);
  friend class hierarchy::ExactPairSupportStreamBuilder;
  friend class hierarchy::ExactHigherSupportStreamBuilder;
  friend class hierarchy::ExactDirectSparseFirstIncidenceBuilder;
  friend class gpu::K1BoruvkaCandidateContext;
  friend class gpu::PairSupportPhiContext;
  friend class gpu::SpatialLbvhContext;
};

[[nodiscard]] TopKPartition lbvh_top_k(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

[[nodiscard]] TopKPartition lbvh_nearest(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const ExclusionSet& exclusions,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

// No scientific partition is present on exhaustion.  In particular, a
// provisional cutoff or a prefix of its equality shell is never published.
// Complete results are exact and use strict AABB pruning only, so a bound equal
// to the current cutoff is always descended.
[[nodiscard]] ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    ExactLbvhTopKBudget budget,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

// Incumbents are bounded proposal data, not trusted distances.  Every supplied
// PointId is validated and re-evaluated exactly, then only seeds the same
// strict-pruning traversal and complete-cutoff-shell construction used above.
[[nodiscard]] ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    std::span<const PointId> incumbent_point_ids,
    ExactLbvhTopKBudget budget,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

[[nodiscard]] ClosedBallPartition lbvh_closed_ball(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const exact::ExactLevel& squared_radius,
    LbvhTraversalOrder traversal_order = LbvhTraversalOrder::near_first);

}  // namespace morsehgp3d::spatial
