#include "morsehgp3d/hierarchy/morton_triangular_block_pair_schedule.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

void checked_increment(std::size_t& value, const char* message) {
  if (value == std::numeric_limits<std::size_t>::max()) {
    throw std::overflow_error(message);
  }
  ++value;
}

void checked_add_to(
    std::size_t& value,
    std::size_t increment,
    const char* message) {
  if (increment > std::numeric_limits<std::size_t>::max() - value) {
    throw std::overflow_error(message);
  }
  value += increment;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error("a triangular block-pair mass overflows size_t");
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_unordered_pair_count(
    std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  const std::size_t left =
      (point_count % 2U == 0U) ? point_count / 2U : point_count;
  const std::size_t right =
      (point_count % 2U == 0U) ? point_count - 1U
                               : (point_count - 1U) / 2U;
  return checked_product(left, right);
}

}  // namespace

ExactMortonTriangularBlockPairScheduleContext
ExactMortonTriangularBlockPairScheduleContext::start(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactMortonTriangularBlockPairScheduleConfig config) {
  return ExactMortonTriangularBlockPairScheduleContext(
      index, cloud, config, PrivateConstructionTag{});
}

ExactMortonTriangularBlockPairScheduleContext::
    ExactMortonTriangularBlockPairScheduleContext(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        ExactMortonTriangularBlockPairScheduleConfig config,
        PrivateConstructionTag)
    : config_(config),
      point_count_(cloud.size()),
      lbvh_identity_(index.identity_) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a triangular block-pair schedule requires its cloud's exact LBVH");
  }
  if (config.maximum_anchor_count_per_cross_block == 0U ||
      config.maximum_anchor_count_per_cross_block >
          exact_morton_triangular_block_pair_maximum_anchor_count) {
    throw std::out_of_range(
        "a triangular block-pair schedule requires 1 to 32 anchors");
  }
  if (point_count_ == 0U || index.leaves_.size() != point_count_ ||
      index.root_index_ >= index.nodes_.size()) {
    throw std::logic_error(
        "a triangular block-pair schedule received an incomplete LBVH");
  }
  if (index.build_counters_.maximum_depth >
      exact_morton_triangular_block_pair_maximum_lbvh_depth) {
    throw std::logic_error(
        "a triangular block-pair schedule LBVH exceeds its fixed depth bound");
  }

  const auto& root = index.nodes_[index.root_index_];
  push_authority(PendingBlockPair{
      PendingKind::diagonal,
      index.root_index_,
      root.leaf_begin,
      root.leaf_end,
      index.root_index_,
      root.leaf_begin,
      root.leaf_end});
}

bool ExactMortonTriangularBlockPairScheduleContext::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const noexcept {
  return ready() && index.validated_for(cloud) &&
      lbvh_identity_.get() == index.identity_.get();
}

void ExactMortonTriangularBlockPairScheduleContext::validate_authority(
    const spatial::MortonLbvhIndex& index,
    const PendingBlockPair& authority) const {
  if (authority.anchor_node_index >= index.nodes_.size() ||
      authority.query_node_index >= index.nodes_.size()) {
    throw std::logic_error(
        "a triangular block-pair authority names a foreign LBVH node");
  }
  const auto& anchor = index.nodes_[authority.anchor_node_index];
  const auto& query = index.nodes_[authority.query_node_index];
  if (authority.anchor_leaf_begin >= authority.anchor_leaf_end ||
      authority.query_leaf_begin >= authority.query_leaf_end ||
      authority.anchor_leaf_end > point_count_ ||
      authority.query_leaf_end > point_count_ ||
      anchor.leaf_begin != authority.anchor_leaf_begin ||
      anchor.leaf_end != authority.anchor_leaf_end ||
      query.leaf_begin != authority.query_leaf_begin ||
      query.leaf_end != authority.query_leaf_end) {
    throw std::logic_error(
        "a triangular block-pair authority lost its native LBVH range");
  }
  if (authority.kind == PendingKind::diagonal) {
    if (authority.anchor_node_index != authority.query_node_index ||
        authority.anchor_leaf_begin != authority.query_leaf_begin ||
        authority.anchor_leaf_end != authority.query_leaf_end) {
      throw std::logic_error(
          "a triangular diagonal authority is not T(N)");
    }
  } else if (authority.anchor_leaf_end > authority.query_leaf_begin) {
    throw std::logic_error(
        "a triangular cross authority does not satisfy A before Q");
  }
}

void ExactMortonTriangularBlockPairScheduleContext::push_authority(
    const PendingBlockPair& authority) {
  if (pending_block_pair_count_ >= pending_block_pairs_.size()) {
    throw std::logic_error(
        "a triangular block-pair schedule exceeded its fixed stack");
  }
  pending_block_pairs_[pending_block_pair_count_] = authority;
  ++pending_block_pair_count_;
  audit_.maximum_pending_block_pair_count = std::max(
      audit_.maximum_pending_block_pair_count, pending_block_pair_count_);
}

void ExactMortonTriangularBlockPairScheduleContext::push_diagonal_partition(
    const spatial::MortonLbvhIndex& index,
    const PendingBlockPair& authority) {
  const auto& parent = index.nodes_[authority.anchor_node_index];
  if (parent.is_leaf() || parent.left_child >= index.nodes_.size() ||
      parent.right_child >= index.nodes_.size() ||
      parent.left_child == parent.right_child ||
      3U > pending_block_pairs_.size() - pending_block_pair_count_) {
    throw std::logic_error(
        "a triangular diagonal authority cannot be split natively");
  }
  const auto& left = index.nodes_[parent.left_child];
  const auto& right = index.nodes_[parent.right_child];
  if (left.leaf_begin != parent.leaf_begin ||
      left.leaf_end != right.leaf_begin ||
      right.leaf_end != parent.leaf_end) {
    throw std::logic_error(
        "a triangular diagonal split lost native child ranges");
  }
  const PendingBlockPair left_diagonal{
      PendingKind::diagonal,
      parent.left_child,
      left.leaf_begin,
      left.leaf_end,
      parent.left_child,
      left.leaf_begin,
      left.leaf_end};
  const PendingBlockPair cross{
      PendingKind::cross,
      parent.left_child,
      left.leaf_begin,
      left.leaf_end,
      parent.right_child,
      right.leaf_begin,
      right.leaf_end};
  const PendingBlockPair right_diagonal{
      PendingKind::diagonal,
      parent.right_child,
      right.leaf_begin,
      right.leaf_end,
      parent.right_child,
      right.leaf_begin,
      right.leaf_end};
  validate_authority(index, left_diagonal);
  validate_authority(index, cross);
  validate_authority(index, right_diagonal);
  push_authority(right_diagonal);
  push_authority(cross);
  push_authority(left_diagonal);
}

void ExactMortonTriangularBlockPairScheduleContext::push_anchor_partition(
    const spatial::MortonLbvhIndex& index,
    const PendingBlockPair& authority) {
  const auto& anchor = index.nodes_[authority.anchor_node_index];
  if (anchor.is_leaf() || anchor.left_child >= index.nodes_.size() ||
      anchor.right_child >= index.nodes_.size() ||
      anchor.left_child == anchor.right_child ||
      2U > pending_block_pairs_.size() - pending_block_pair_count_) {
    throw std::logic_error(
        "a triangular cross authority cannot split its native anchor");
  }
  const auto& left = index.nodes_[anchor.left_child];
  const auto& right = index.nodes_[anchor.right_child];
  if (left.leaf_begin != anchor.leaf_begin ||
      left.leaf_end != right.leaf_begin ||
      right.leaf_end != anchor.leaf_end) {
    throw std::logic_error(
        "a triangular anchor split lost native child ranges");
  }
  const PendingBlockPair left_cross{
      PendingKind::cross,
      anchor.left_child,
      left.leaf_begin,
      left.leaf_end,
      authority.query_node_index,
      authority.query_leaf_begin,
      authority.query_leaf_end};
  const PendingBlockPair right_cross{
      PendingKind::cross,
      anchor.right_child,
      right.leaf_begin,
      right.leaf_end,
      authority.query_node_index,
      authority.query_leaf_begin,
      authority.query_leaf_end};
  validate_authority(index, left_cross);
  validate_authority(index, right_cross);
  push_authority(right_cross);
  push_authority(left_cross);
}

void ExactMortonTriangularBlockPairScheduleContext::
    finish_if_partition_complete() {
  if (pending_block_pair_count_ != 0U || awaiting_cross_block_.has_value()) {
    return;
  }
  std::size_t observed_pair_count = audit_.certified_unordered_pair_count;
  checked_add_to(
      observed_pair_count,
      audit_.opened_unordered_pair_count,
      "the triangular terminal pair partition overflows size_t");
  if (audit_.emitted_diagonal_self_count != point_count_ ||
      observed_pair_count != checked_unordered_pair_count(point_count_)) {
    throw std::logic_error(
        "a triangular schedule did not certify a complete disjoint partition");
  }
  complete_ = true;
  audit_.complete = true;
  audit_.triangular_partition_complete = true;
}

ExactMortonTriangularBlockPairScheduleStep
ExactMortonTriangularBlockPairScheduleContext::advance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactMortonTriangularBlockPairScheduleBudget budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a triangular block-pair schedule cannot resume foreign authority");
  }
  if (awaiting_cross_block_.has_value()) {
    throw std::logic_error(
        "a triangular cross block requires a consumer response before resume");
  }
  checked_increment(
      audit_.advance_call_count,
      "the triangular schedule advance count overflows size_t");

  ExactMortonTriangularBlockPairScheduleStep step;
  step.requested_budget_ = budget;
  if (complete_) {
    step.kind_ = ExactMortonTriangularBlockPairScheduleStepKind::complete;
    step.schedule_complete_after_step_ = true;
    return step;
  }

  while (pending_block_pair_count_ != 0U) {
    if (step.work_.block_pair_visit_count >=
        budget.maximum_block_pair_visit_count) {
      step.kind_ =
          ExactMortonTriangularBlockPairScheduleStepKind::budget_exhausted;
      step.stop_reason_ =
          ExactMortonTriangularBlockPairScheduleStopReason::
              block_pair_visit_limit;
      checked_increment(
          audit_.budget_exhaustion_count,
          "the triangular schedule exhaustion count overflows size_t");
      return step;
    }

    --pending_block_pair_count_;
    const PendingBlockPair authority =
        pending_block_pairs_[pending_block_pair_count_];
    validate_authority(index, authority);
    checked_increment(
        step.work_.block_pair_visit_count,
        "the triangular per-step visit count overflows size_t");
    checked_increment(
        audit_.block_pair_visit_count,
        "the triangular visit count overflows size_t");

    if (authority.kind == PendingKind::diagonal) {
      const auto& node = index.nodes_[authority.anchor_node_index];
      if (!node.is_leaf()) {
        push_diagonal_partition(index, authority);
        checked_increment(
            step.work_.diagonal_split_count,
            "the triangular per-step diagonal split count overflows size_t");
        checked_increment(
            audit_.diagonal_split_count,
            "the triangular diagonal split count overflows size_t");
        continue;
      }
      if (authority.anchor_leaf_end - authority.anchor_leaf_begin != 1U) {
        throw std::logic_error(
            "a triangular diagonal leaf does not contain one point");
      }
      step.kind_ =
          ExactMortonTriangularBlockPairScheduleStepKind::diagonal_self;
      step.anchor_node_index_ = authority.anchor_node_index;
      step.anchor_leaf_begin_ = authority.anchor_leaf_begin;
      step.anchor_leaf_end_ = authority.anchor_leaf_end;
      step.self_point_id_ =
          index.leaves_[authority.anchor_leaf_begin].point_id;
      checked_increment(
          audit_.emitted_diagonal_self_count,
          "the triangular self count overflows size_t");
      finish_if_partition_complete();
      step.schedule_complete_after_step_ = complete_;
      return step;
    }

    const std::size_t anchor_count =
        authority.anchor_leaf_end - authority.anchor_leaf_begin;
    if (anchor_count > config_.maximum_anchor_count_per_cross_block) {
      push_anchor_partition(index, authority);
      checked_increment(
          step.work_.oversized_anchor_split_count,
          "the triangular per-step anchor split count overflows size_t");
      checked_increment(
          audit_.oversized_anchor_split_count,
          "the triangular oversized-anchor split count overflows size_t");
      continue;
    }

    step.kind_ = ExactMortonTriangularBlockPairScheduleStepKind::cross_block;
    step.anchor_node_index_ = authority.anchor_node_index;
    step.anchor_leaf_begin_ = authority.anchor_leaf_begin;
    step.anchor_leaf_end_ = authority.anchor_leaf_end;
    step.query_node_index_ = authority.query_node_index;
    step.query_leaf_begin_ = authority.query_leaf_begin;
    step.query_leaf_end_ = authority.query_leaf_end;
    step.anchor_point_count_ = anchor_count;
    for (std::size_t offset = 0U; offset < anchor_count; ++offset) {
      step.anchor_point_ids_[offset] =
          index.leaves_[authority.anchor_leaf_begin + offset].point_id;
    }
    std::sort(
        step.anchor_point_ids_.begin(),
        step.anchor_point_ids_.begin() +
            static_cast<std::ptrdiff_t>(anchor_count));
    awaiting_cross_block_ = authority;
    checked_increment(
        audit_.emitted_cross_block_count,
        "the triangular emitted-cross count overflows size_t");
    return step;
  }

  finish_if_partition_complete();
  step.kind_ = ExactMortonTriangularBlockPairScheduleStepKind::complete;
  step.schedule_complete_after_step_ = true;
  return step;
}

ExactMortonTriangularBlockPairResponse
ExactMortonTriangularBlockPairScheduleContext::respond_to_cross_block(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactMortonTriangularBlockPairDecision decision) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a triangular block-pair response names foreign authority");
  }
  if (!awaiting_cross_block_.has_value()) {
    throw std::logic_error(
        "a triangular block-pair response has no active cross block");
  }
  if (decision != ExactMortonTriangularBlockPairDecision::certified &&
      decision != ExactMortonTriangularBlockPairDecision::inconclusive) {
    throw std::invalid_argument(
        "a triangular block-pair response has an invalid decision");
  }
  checked_increment(
      audit_.response_call_count,
      "the triangular response count overflows size_t");

  const PendingBlockPair authority = *awaiting_cross_block_;
  validate_authority(index, authority);
  const std::size_t anchor_count =
      authority.anchor_leaf_end - authority.anchor_leaf_begin;
  const std::size_t query_count =
      authority.query_leaf_end - authority.query_leaf_begin;
  const std::size_t pair_count = checked_product(anchor_count, query_count);

  ExactMortonTriangularBlockPairResponse response;
  response.anchor_node_index_ = authority.anchor_node_index;
  response.anchor_leaf_begin_ = authority.anchor_leaf_begin;
  response.anchor_leaf_end_ = authority.anchor_leaf_end;
  response.query_node_index_ = authority.query_node_index;
  response.query_leaf_begin_ = authority.query_leaf_begin;
  response.query_leaf_end_ = authority.query_leaf_end;

  if (decision == ExactMortonTriangularBlockPairDecision::certified) {
    response.kind_ =
        ExactMortonTriangularBlockPairResponseKind::certified_closed;
    response.certified_unordered_pair_count_ = pair_count;
    checked_increment(
        audit_.certified_cross_block_count,
        "the triangular certified-block count overflows size_t");
    checked_add_to(
        audit_.certified_unordered_pair_count,
        pair_count,
        "the triangular certified-pair count overflows size_t");
    awaiting_cross_block_.reset();
    finish_if_partition_complete();
    response.schedule_complete_after_response_ = complete_;
    return response;
  }

  checked_increment(
      audit_.inconclusive_cross_block_count,
      "the triangular inconclusive-block count overflows size_t");
  if (anchor_count > 1U) {
    push_anchor_partition(index, authority);
    checked_increment(
        audit_.consumer_anchor_split_count,
        "the triangular consumer anchor-split count overflows size_t");
    awaiting_cross_block_.reset();
    response.kind_ =
        ExactMortonTriangularBlockPairResponseKind::anchor_split;
    return response;
  }

  response.kind_ = ExactMortonTriangularBlockPairResponseKind::
      terminal_singleton_cross_block;
  response.opened_unordered_pair_count_ = pair_count;
  response.terminal_anchor_point_id_ =
      index.leaves_[authority.anchor_leaf_begin].point_id;
  checked_increment(
      audit_.opened_singleton_cross_block_count,
      "the triangular terminal singleton count overflows size_t");
  checked_add_to(
      audit_.opened_unordered_pair_count,
      pair_count,
      "the triangular opened-pair count overflows size_t");
  awaiting_cross_block_.reset();
  finish_if_partition_complete();
  response.schedule_complete_after_response_ = complete_;
  return response;
}

}  // namespace morsehgp3d::hierarchy
