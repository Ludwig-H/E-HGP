#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_schedule.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::MortonLbvhIndex;
using spatial::PointId;

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

}  // namespace

ExactMortonGroupedAnchoredPairScheduleContext
ExactMortonGroupedAnchoredPairScheduleContext::start(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig config) {
  return ExactMortonGroupedAnchoredPairScheduleContext(
      index,
      cloud,
      maximum_closed_rank,
      config,
      PrivateConstructionTag{});
}

ExactMortonGroupedAnchoredPairScheduleContext::
    ExactMortonGroupedAnchoredPairScheduleContext(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::size_t maximum_closed_rank,
        ExactMortonGroupedAnchoredPairScheduleConfig config,
        PrivateConstructionTag)
    : maximum_closed_rank_(maximum_closed_rank),
      config_(config),
      point_count_(cloud.size()),
      cloud_identity_(cloud.identity_),
      lbvh_identity_(index.identity_) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a Morton grouped schedule requires its cloud's exact LBVH");
  }
  if (config.maximum_anchor_count_per_group == 0U ||
      config.maximum_anchor_count_per_group >
          exact_grouped_anchored_pair_maximum_anchor_count) {
    throw std::out_of_range(
        "a Morton grouped schedule requires 1 to 32 anchors per group");
  }
  if (config.proposed_witness_pool_size >
      exact_grouped_anchored_pair_maximum_witness_pool_size) {
    throw std::out_of_range(
        "a Morton grouped schedule witness pool cannot exceed 64 points");
  }
  if (maximum_closed_rank < 2U ||
      maximum_closed_rank >
          exact_grouped_anchored_pair_maximum_closed_rank) {
    throw std::out_of_range(
        "a Morton grouped schedule maximum closed rank must be in [2, 11]");
  }
  if (index.leaves().size() != point_count_) {
    throw std::logic_error(
        "a Morton grouped schedule received an incomplete LBVH leaf order");
  }

  prepare_group(index, cloud, 0U);
}

bool ExactMortonGroupedAnchoredPairScheduleContext::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  return ready() && index.validated_for(cloud) &&
      cloud_identity_.get() == cloud.identity_.get() &&
      lbvh_identity_.get() == index.identity_.get();
}

void ExactMortonGroupedAnchoredPairScheduleContext::prepare_group(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t anchor_leaf_begin) {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a Morton grouped schedule cannot prepare a foreign authority");
  }
  if (anchor_leaf_begin >= point_count_) {
    throw std::out_of_range(
        "a Morton grouped schedule cannot prepare an empty anchor suffix");
  }

  const std::span<const spatial::MortonLeafRecord> leaves = index.leaves();
  const std::size_t remaining_anchor_count =
      point_count_ - anchor_leaf_begin;
  const std::size_t anchor_count = std::min(
      config_.maximum_anchor_count_per_group,
      remaining_anchor_count);
  const std::size_t anchor_leaf_end = anchor_leaf_begin + anchor_count;

  std::array<PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_ids{};
  for (std::size_t offset = 0U; offset < anchor_count; ++offset) {
    anchor_point_ids[offset] =
        leaves[anchor_leaf_begin + offset].point_id;
  }
  std::sort(
      anchor_point_ids.begin(),
      anchor_point_ids.begin() +
          static_cast<std::ptrdiff_t>(anchor_count));

  std::array<PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_pool_point_ids{};
  std::size_t witness_pool_entry_count = 0U;
  const std::size_t left_available = anchor_leaf_begin;
  const std::size_t right_available = point_count_ - anchor_leaf_end;
  const std::size_t maximum_halo_offset =
      std::max(left_available, right_available);
  for (std::size_t halo_offset = 1U;
       halo_offset <= maximum_halo_offset &&
       witness_pool_entry_count < config_.proposed_witness_pool_size;
       ++halo_offset) {
    if (halo_offset <= left_available) {
      witness_pool_point_ids[witness_pool_entry_count] =
          leaves[anchor_leaf_begin - halo_offset].point_id;
      ++witness_pool_entry_count;
    }
    if (halo_offset <= right_available &&
        witness_pool_entry_count < config_.proposed_witness_pool_size) {
      witness_pool_point_ids[witness_pool_entry_count] =
          leaves[anchor_leaf_end + halo_offset - 1U].point_id;
      ++witness_pool_entry_count;
    }
  }
  std::sort(
      witness_pool_point_ids.begin(),
      witness_pool_point_ids.begin() +
          static_cast<std::ptrdiff_t>(witness_pool_entry_count));

  const std::span<const PointId> anchor_span{
      anchor_point_ids.data(), anchor_count};
  const std::span<const PointId> witness_span{
      witness_pool_point_ids.data(), witness_pool_entry_count};
  ExactGroupedAnchoredPairTraversalContext prepared_traversal =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
          index,
          cloud,
          anchor_span,
          witness_span,
          maximum_closed_rank_);

  active_group_ordinal_ =
      anchor_leaf_begin / config_.maximum_anchor_count_per_group;
  active_anchor_leaf_begin_ = anchor_leaf_begin;
  active_anchor_leaf_end_ = anchor_leaf_end;
  active_anchor_point_ids_ = anchor_point_ids;
  active_anchor_count_ = anchor_count;
  active_witness_pool_point_ids_ = witness_pool_point_ids;
  active_witness_pool_entry_count_ = witness_pool_entry_count;
  active_traversal_.reset();
  active_traversal_.emplace(std::move(prepared_traversal));
  active_singleton_traversal_.reset();
  pending_anchor_subranges_ = {};
  pending_anchor_subrange_count_ = 0U;
  active_fallback_anchor_leaf_begin_ = 0U;
  active_fallback_anchor_leaf_end_ = 0U;
  active_fallback_anchor_point_ids_ = {};
  active_fallback_anchor_count_ = 0U;
  active_singleton_witness_pool_point_ids_ = {};
  active_singleton_witness_pool_entry_count_ = 0U;
  singleton_frontier_active_ = false;
  active_fallback_is_singleton_ = false;
  group_completion_pending_ = false;

  checked_increment(
      audit_.prepared_group_count,
      "the Morton grouped prepared-group count overflows size_t");
  checked_add_to(
      audit_.scheduled_anchor_count,
      anchor_count,
      "the Morton grouped scheduled-anchor count overflows size_t");
  checked_add_to(
      audit_.proposed_witness_pool_entry_count,
      witness_pool_entry_count,
      "the Morton grouped witness-pool count overflows size_t");
  audit_.maximum_active_anchor_count = std::max(
      audit_.maximum_active_anchor_count, anchor_count);
  audit_.maximum_active_witness_pool_entry_count = std::max(
      audit_.maximum_active_witness_pool_entry_count,
      witness_pool_entry_count);
}

void ExactMortonGroupedAnchoredPairScheduleContext::
    push_anchor_subrange_children(
        std::size_t anchor_leaf_begin,
        std::size_t anchor_leaf_end) {
  if (anchor_leaf_begin < active_anchor_leaf_begin_ ||
      anchor_leaf_end > active_anchor_leaf_end_ ||
      anchor_leaf_begin >= anchor_leaf_end ||
      anchor_leaf_end - anchor_leaf_begin < 2U ||
      pending_anchor_subrange_count_ >
          pending_anchor_subranges_.size() - 2U) {
    throw std::logic_error(
        "a Morton grouped anchor subrange cannot be split safely");
  }
  const std::size_t midpoint =
      anchor_leaf_begin + (anchor_leaf_end - anchor_leaf_begin) / 2U;
  pending_anchor_subranges_[pending_anchor_subrange_count_] =
      PendingAnchorLeafRange{anchor_leaf_begin, midpoint};
  pending_anchor_subranges_[pending_anchor_subrange_count_ + 1U] =
      PendingAnchorLeafRange{midpoint, anchor_leaf_end};
  pending_anchor_subrange_count_ += 2U;
  audit_.maximum_pending_anchor_subgroup_count = std::max(
      audit_.maximum_pending_anchor_subgroup_count,
      pending_anchor_subrange_count_);
}

void ExactMortonGroupedAnchoredPairScheduleContext::
    prepare_next_anchor_partition_fallback(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud) {
  if (!validated_for(index, cloud) || !singleton_frontier_active_ ||
      active_singleton_traversal_.has_value() ||
      pending_anchor_subrange_count_ == 0U) {
    throw std::logic_error(
        "a Morton grouped schedule cannot prepare an invalid anchor fallback");
  }

  --pending_anchor_subrange_count_;
  const PendingAnchorLeafRange active_range =
      pending_anchor_subranges_[pending_anchor_subrange_count_];
  pending_anchor_subranges_[pending_anchor_subrange_count_] = {};
  if (active_range.begin < active_anchor_leaf_begin_ ||
      active_range.end > active_anchor_leaf_end_ ||
      active_range.begin >= active_range.end) {
    throw std::logic_error(
        "a Morton grouped schedule popped an invalid anchor subrange");
  }

  active_fallback_anchor_leaf_begin_ = active_range.begin;
  active_fallback_anchor_leaf_end_ = active_range.end;
  active_fallback_anchor_count_ = active_range.end - active_range.begin;
  active_fallback_anchor_point_ids_ = {};
  const std::span<const spatial::MortonLeafRecord> leaves = index.leaves();
  for (std::size_t offset = 0U;
       offset < active_fallback_anchor_count_;
       ++offset) {
    active_fallback_anchor_point_ids_[offset] =
        leaves[active_range.begin + offset].point_id;
  }
  std::sort(
      active_fallback_anchor_point_ids_.begin(),
      active_fallback_anchor_point_ids_.begin() +
          static_cast<std::ptrdiff_t>(active_fallback_anchor_count_));

  active_singleton_witness_pool_point_ids_ = {};
  active_singleton_witness_pool_entry_count_ = 0U;
  const std::size_t left_available = active_range.begin;
  const std::size_t right_available = point_count_ - active_range.end;
  active_fallback_is_singleton_ = active_fallback_anchor_count_ == 1U;
  const bool query_is_before =
      singleton_frontier_leaf_end_ <= active_range.begin;
  const bool query_is_after =
      singleton_frontier_leaf_begin_ >= active_range.end;
  std::size_t query_facing_entry_count = 0U;
  const auto append_witness = [&](bool from_left, std::size_t offset) {
    active_singleton_witness_pool_point_ids_[
        active_singleton_witness_pool_entry_count_] =
        from_left
        ? leaves[active_range.begin - offset].point_id
        : leaves[active_range.end + offset - 1U].point_id;
    ++active_singleton_witness_pool_entry_count_;
  };
  if (query_is_before != query_is_after) {
    const bool prefer_left = query_is_before;
    const std::size_t preferred_available =
        prefer_left ? left_available : right_available;
    const std::size_t opposite_available =
        prefer_left ? right_available : left_available;
    const std::size_t preferred_target =
        (3U * config_.proposed_witness_pool_size + 3U) / 4U;
    std::size_t preferred_offset = 1U;
    while (preferred_offset <= preferred_available &&
           query_facing_entry_count < preferred_target &&
           active_singleton_witness_pool_entry_count_ <
               config_.proposed_witness_pool_size) {
      append_witness(prefer_left, preferred_offset);
      ++preferred_offset;
      ++query_facing_entry_count;
    }
    std::size_t opposite_offset = 1U;
    while (opposite_offset <= opposite_available &&
           active_singleton_witness_pool_entry_count_ <
               config_.proposed_witness_pool_size) {
      append_witness(!prefer_left, opposite_offset);
      ++opposite_offset;
    }
    while (preferred_offset <= preferred_available &&
           active_singleton_witness_pool_entry_count_ <
               config_.proposed_witness_pool_size) {
      append_witness(prefer_left, preferred_offset);
      ++preferred_offset;
      ++query_facing_entry_count;
    }
  } else {
    const std::size_t maximum_halo_offset =
        std::max(left_available, right_available);
    for (std::size_t halo_offset = 1U;
         halo_offset <= maximum_halo_offset &&
         active_singleton_witness_pool_entry_count_ <
             config_.proposed_witness_pool_size;
         ++halo_offset) {
      if (halo_offset <= left_available) {
        append_witness(true, halo_offset);
      }
      if (halo_offset <= right_available &&
          active_singleton_witness_pool_entry_count_ <
              config_.proposed_witness_pool_size) {
        append_witness(false, halo_offset);
      }
    }
  }
  std::sort(
      active_singleton_witness_pool_point_ids_.begin(),
      active_singleton_witness_pool_point_ids_.begin() +
          static_cast<std::ptrdiff_t>(
              active_singleton_witness_pool_entry_count_));

  const std::span<const PointId> anchors{
      active_fallback_anchor_point_ids_.data(),
      active_fallback_anchor_count_};
  const std::span<const PointId> witnesses{
      active_singleton_witness_pool_point_ids_.data(),
      active_singleton_witness_pool_entry_count_};
  ExactGroupedAnchoredPairTraversalContext fallback_traversal =
      active_fallback_is_singleton_
      ? ExactGroupedAnchoredPairTraversalContext::
            start_witness_subtree_first_at_node(
            index,
            cloud,
            anchors,
            witnesses,
            singleton_frontier_node_index_,
            maximum_closed_rank_)
      : ExactGroupedAnchoredPairTraversalContext::
            start_witness_subtree_first_frontier_at_node(
            index,
            cloud,
            anchors,
            witnesses,
            singleton_frontier_node_index_,
            maximum_closed_rank_);
  active_singleton_traversal_.emplace(std::move(fallback_traversal));
  checked_add_to(
      audit_.query_facing_fallback_witness_pool_entry_count,
      query_facing_entry_count,
      "the Morton grouped query-facing witness count overflows size_t");

  if (active_fallback_is_singleton_) {
    checked_increment(
        audit_.prepared_singleton_fallback_count,
        "the Morton grouped singleton-fallback count overflows size_t");
    checked_add_to(
        audit_.proposed_singleton_witness_pool_entry_count,
        active_singleton_witness_pool_entry_count_,
        "the Morton grouped singleton witness-pool count overflows size_t");
  } else {
    checked_increment(
        audit_.prepared_anchor_subgroup_probe_count,
        "the Morton grouped subgroup-probe count overflows size_t");
    checked_add_to(
        audit_.proposed_anchor_subgroup_witness_pool_entry_count,
        active_singleton_witness_pool_entry_count_,
        "the Morton grouped subgroup witness-pool count overflows size_t");
  }
}

ExactMortonGroupedAnchoredPairScheduleStep
ExactMortonGroupedAnchoredPairScheduleContext::snapshot_step(
    ExactMortonGroupedAnchoredPairScheduleStepKind kind,
    ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason,
    ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) const {
  ExactMortonGroupedAnchoredPairScheduleStep step;
  step.kind_ = kind;
  step.stop_reason_ = stop_reason;
  step.requested_traversal_budget_ = traversal_budget;
  if (active_singleton_traversal_.has_value()) {
    step.group_ordinal_ = active_group_ordinal_;
    step.anchor_leaf_begin_ = active_fallback_anchor_leaf_begin_;
    step.anchor_leaf_end_ = active_fallback_anchor_leaf_end_;
    if (kind !=
        ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted) {
      step.anchor_point_ids_ = active_fallback_anchor_point_ids_;
      step.anchor_count_ = active_fallback_anchor_count_;
    }
    if (kind ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::certified_prune) {
      const std::span<const PointId> certified_pool =
          active_singleton_traversal_->witness_pool_point_ids();
      step.witness_pool_entry_count_ = certified_pool.size();
      std::copy(
          certified_pool.begin(),
          certified_pool.end(),
          step.witness_pool_point_ids_.begin());
    }
  } else if (active_traversal_.has_value()) {
    step.group_ordinal_ = active_group_ordinal_;
    step.anchor_leaf_begin_ = active_anchor_leaf_begin_;
    step.anchor_leaf_end_ = active_anchor_leaf_end_;
    if (kind !=
        ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted) {
      step.anchor_point_ids_ = active_anchor_point_ids_;
      step.anchor_count_ = active_anchor_count_;
    }
    if (kind ==
            ExactMortonGroupedAnchoredPairScheduleStepKind::
                certified_prune ||
        kind ==
            ExactMortonGroupedAnchoredPairScheduleStepKind::group_complete) {
      step.witness_pool_point_ids_ = active_witness_pool_point_ids_;
      step.witness_pool_entry_count_ = active_witness_pool_entry_count_;
    }
  }
  return step;
}

ExactMortonGroupedAnchoredPairScheduleStep
ExactMortonGroupedAnchoredPairScheduleContext::advance(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a Morton grouped schedule advance requires its authentic cloud and LBVH");
  }
  checked_increment(
      audit_.advance_call_count,
      "the Morton grouped advance-call count overflows size_t");

  if (complete_) {
    return snapshot_step(
        ExactMortonGroupedAnchoredPairScheduleStepKind::complete,
        ExactMortonGroupedAnchoredPairScheduleStopReason::none,
        traversal_budget);
  }
  if (!active_traversal_.has_value()) {
    throw std::logic_error(
        "an incomplete Morton grouped schedule has no active traversal");
  }

  if (singleton_frontier_active_ &&
      active_singleton_traversal_.has_value() &&
      active_singleton_traversal_->complete()) {
    if (active_fallback_is_singleton_) {
      checked_increment(
          audit_.completed_singleton_fallback_count,
          "the Morton grouped completed-singleton count overflows size_t");
    }
    active_singleton_traversal_.reset();
    active_fallback_anchor_leaf_begin_ = 0U;
    active_fallback_anchor_leaf_end_ = 0U;
    active_fallback_anchor_point_ids_ = {};
    active_fallback_anchor_count_ = 0U;
    active_singleton_witness_pool_point_ids_ = {};
    active_singleton_witness_pool_entry_count_ = 0U;
    active_fallback_is_singleton_ = false;
  }
  if (singleton_frontier_active_ &&
      !active_singleton_traversal_.has_value()) {
    if (pending_anchor_subrange_count_ > 0U) {
      prepare_next_anchor_partition_fallback(index, cloud);
    } else {
      singleton_frontier_active_ = false;
      singleton_frontier_node_index_ = 0U;
      singleton_frontier_leaf_begin_ = 0U;
      singleton_frontier_leaf_end_ = 0U;
      pending_anchor_subranges_ = {};
      pending_anchor_subrange_count_ = 0U;
      active_fallback_anchor_leaf_begin_ = 0U;
      active_fallback_anchor_leaf_end_ = 0U;
      active_fallback_anchor_point_ids_ = {};
      active_fallback_anchor_count_ = 0U;
      active_fallback_is_singleton_ = false;
    }
  }

  if (!singleton_frontier_active_ &&
      (group_completion_pending_ || active_traversal_->complete())) {
    ExactMortonGroupedAnchoredPairScheduleStep step = snapshot_step(
        ExactMortonGroupedAnchoredPairScheduleStepKind::group_complete,
        ExactMortonGroupedAnchoredPairScheduleStopReason::none,
        traversal_budget);
    const std::size_t next_anchor_leaf_begin = active_anchor_leaf_end_;
    if (next_anchor_leaf_begin == point_count_) {
      active_traversal_.reset();
      active_anchor_point_ids_ = {};
      active_anchor_count_ = 0U;
      active_witness_pool_point_ids_ = {};
      active_witness_pool_entry_count_ = 0U;
      active_singleton_traversal_.reset();
      pending_anchor_subranges_ = {};
      pending_anchor_subrange_count_ = 0U;
      active_fallback_anchor_leaf_begin_ = 0U;
      active_fallback_anchor_leaf_end_ = 0U;
      active_fallback_anchor_point_ids_ = {};
      active_fallback_anchor_count_ = 0U;
      active_singleton_witness_pool_point_ids_ = {};
      active_singleton_witness_pool_entry_count_ = 0U;
      singleton_frontier_active_ = false;
      active_fallback_is_singleton_ = false;
      group_completion_pending_ = false;
      complete_ = true;
      audit_.complete = true;
      audit_.morton_anchor_partition_complete =
          audit_.scheduled_anchor_count == point_count_;
    } else {
      prepare_group(index, cloud, next_anchor_leaf_begin);
    }
    checked_increment(
        audit_.completed_group_count,
        "the Morton grouped completed-group count overflows size_t");
    return step;
  }

  ExactGroupedAnchoredPairTraversalContext& selected_traversal =
      active_singleton_traversal_.has_value()
      ? *active_singleton_traversal_
      : *active_traversal_;
  const std::size_t diagonal_node_descent_count_before =
      selected_traversal.audit().diagonal_node_descent_count;
  const std::size_t witness_subtree_receipt_count_before =
      selected_traversal.audit().witness_subtree_receipt_count;
  const std::size_t witness_subtree_success_count_before =
      selected_traversal.audit().witness_subtree_success_count;
  const std::size_t witness_subtree_fail_open_count_before =
      selected_traversal.audit().witness_subtree_fail_open_count;
  ExactGroupedAnchoredPairTraversalStep traversal_step =
      selected_traversal.advance(index, cloud, traversal_budget);
  const std::size_t diagonal_node_descent_count_after =
      selected_traversal.audit().diagonal_node_descent_count;
  if (diagonal_node_descent_count_after <
      diagonal_node_descent_count_before) {
    throw std::logic_error(
        "a grouped traversal diagonal-descent audit regressed");
  }
  checked_add_to(
      audit_.diagonal_node_descent_count,
      diagonal_node_descent_count_after -
          diagonal_node_descent_count_before,
      "the Morton grouped diagonal-descent count overflows size_t");
  const auto accumulate_monotone_counter = [](
                                               std::size_t before,
                                               std::size_t after,
                                               std::size_t& destination,
                                               const char* regression_message,
                                               const char* overflow_message) {
    if (after < before) {
      throw std::logic_error(regression_message);
    }
    checked_add_to(destination, after - before, overflow_message);
  };
  accumulate_monotone_counter(
      witness_subtree_receipt_count_before,
      selected_traversal.audit().witness_subtree_receipt_count,
      audit_.witness_subtree_receipt_count,
      "a grouped traversal witness-subtree receipt audit regressed",
      "the Morton grouped witness-subtree receipt count overflows size_t");
  accumulate_monotone_counter(
      witness_subtree_success_count_before,
      selected_traversal.audit().witness_subtree_success_count,
      audit_.witness_subtree_success_count,
      "a grouped traversal witness-subtree success audit regressed",
      "the Morton grouped witness-subtree success count overflows size_t");
  accumulate_monotone_counter(
      witness_subtree_fail_open_count_before,
      selected_traversal.audit().witness_subtree_fail_open_count,
      audit_.witness_subtree_fail_open_count,
      "a grouped traversal witness-subtree fail-open audit regressed",
      "the Morton grouped witness-subtree fail-open count overflows size_t");
  checked_increment(
      audit_.traversal_advance_count,
      "the Morton grouped traversal-advance count overflows size_t");
  const ExactGroupedAnchoredPairTraversalStepWork& traversal_work =
      traversal_step.work();
  checked_add_to(
      audit_.traversal_node_visit_count,
      traversal_work.node_visit_count,
      "the Morton grouped node-visit count overflows size_t");
  checked_add_to(
      audit_.witness_subtree_node_visit_count,
      traversal_work.witness_subtree_node_visit_count,
      "the Morton grouped witness-subtree node count overflows size_t");
  if (!active_singleton_traversal_.has_value()) {
    checked_add_to(
        audit_.common_traversal_node_visit_count,
        traversal_work.node_visit_count,
        "the Morton grouped common node count overflows size_t");
  } else if (active_fallback_is_singleton_) {
    checked_add_to(
        audit_.singleton_node_visit_count,
        traversal_work.node_visit_count,
        "the Morton grouped singleton node count overflows size_t");
  } else {
    checked_add_to(
        audit_.anchor_subgroup_node_visit_count,
        traversal_work.node_visit_count,
        "the Morton grouped subgroup node count overflows size_t");
  }
  checked_add_to(
      audit_.witness_slot_scan_count,
      traversal_work.witness_slot_scan_count,
      "the Morton grouped witness-slot count overflows size_t");
  checked_add_to(
      audit_.inherited_witness_reuse_count,
      traversal_work.inherited_witness_reuse_count,
      "the Morton grouped inherited-witness count overflows size_t");
  checked_add_to(
      audit_.exact_predicate_count,
      traversal_work.exact_predicate_count,
      "the Morton grouped exact-predicate count overflows size_t");
  checked_add_to(
      audit_.witness_subtree_exact_predicate_count,
      traversal_work.witness_subtree_exact_predicate_count,
      "the Morton grouped witness-subtree predicate count overflows size_t");
  if (!active_singleton_traversal_.has_value()) {
    checked_add_to(
        audit_.common_exact_predicate_count,
        traversal_work.exact_predicate_count,
        "the Morton grouped common predicate count overflows size_t");
  } else if (active_fallback_is_singleton_) {
    checked_add_to(
        audit_.singleton_exact_predicate_count,
        traversal_work.exact_predicate_count,
        "the Morton grouped singleton predicate count overflows size_t");
  } else {
    checked_add_to(
        audit_.anchor_subgroup_exact_predicate_count,
        traversal_work.exact_predicate_count,
        "the Morton grouped subgroup predicate count overflows size_t");
  }
  checked_add_to(
      audit_.strict_witness_discovery_count,
      traversal_work.strict_witness_discovery_count,
      "the Morton grouped strict-witness count overflows size_t");

  ExactMortonGroupedAnchoredPairScheduleStepKind kind =
      ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted;
  ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason =
      ExactMortonGroupedAnchoredPairScheduleStopReason::none;
  switch (traversal_step.kind()) {
    case ExactGroupedAnchoredPairTraversalStepKind::certified_prune: {
      kind = ExactMortonGroupedAnchoredPairScheduleStepKind::certified_prune;
      const ExactGroupedAnchoredPairPruneCertificate* certificate =
          traversal_step.prune_certificate();
      const std::span<const PointId> expected_anchors =
          active_singleton_traversal_.has_value()
          ? std::span<const PointId>{
                active_fallback_anchor_point_ids_.data(),
                active_fallback_anchor_count_}
          : std::span<const PointId>{
                active_anchor_point_ids_.data(), active_anchor_count_};
      if (certificate == nullptr ||
          !traversal_step.lbvh_node_index().has_value() ||
          !certificate->certifies(
              index,
              cloud,
              *traversal_step.lbvh_node_index(),
              maximum_closed_rank_,
              expected_anchors)) {
        throw std::logic_error(
            "a Morton grouped schedule received an unauthenticated prune");
      }
      if (active_singleton_traversal_.has_value() &&
          !active_fallback_is_singleton_ &&
          (*traversal_step.lbvh_node_index() !=
               singleton_frontier_node_index_ ||
           certificate->leaf_begin() != singleton_frontier_leaf_begin_ ||
           certificate->leaf_end() != singleton_frontier_leaf_end_)) {
        throw std::logic_error(
            "a Morton grouped subgroup probe certified outside its frontier");
      }
      checked_increment(
          audit_.certified_prune_count,
          "the Morton grouped prune count overflows size_t");
      if (active_singleton_traversal_.has_value() &&
          active_fallback_is_singleton_) {
        checked_increment(
            audit_.singleton_certified_prune_count,
            "the Morton grouped singleton prune count overflows size_t");
      } else if (active_singleton_traversal_.has_value()) {
        checked_increment(
            audit_.anchor_subgroup_certified_prune_count,
            "the Morton grouped subgroup prune count overflows size_t");
        checked_add_to(
            audit_.anchor_subgroup_certified_anchor_count,
            active_fallback_anchor_count_,
            "the Morton grouped subgroup anchor mass overflows size_t");
      }
      break;
    }
    case ExactGroupedAnchoredPairTraversalStepKind::inconclusive_subtree: {
      if (!traversal_step.lbvh_node_index().has_value() ||
          !traversal_step.leaf_begin().has_value() ||
          !traversal_step.leaf_end().has_value() ||
          *traversal_step.leaf_begin() >= *traversal_step.leaf_end()) {
        throw std::logic_error(
            "a Morton grouped schedule received an invalid frontier");
      }
      if (active_singleton_traversal_.has_value()) {
        if (!singleton_frontier_active_ ||
            active_fallback_is_singleton_ ||
            active_fallback_anchor_count_ < 2U ||
            *traversal_step.lbvh_node_index() !=
                singleton_frontier_node_index_ ||
            *traversal_step.leaf_begin() !=
                singleton_frontier_leaf_begin_ ||
            *traversal_step.leaf_end() !=
                singleton_frontier_leaf_end_) {
          throw std::logic_error(
              "a Morton grouped subgroup probe escaped its frontier");
        }
        push_anchor_subrange_children(
            active_fallback_anchor_leaf_begin_,
            active_fallback_anchor_leaf_end_);
        kind = ExactMortonGroupedAnchoredPairScheduleStepKind::
            anchor_subgroup_split;
        checked_increment(
            audit_.anchor_subgroup_split_count,
            "the Morton grouped subgroup-split count overflows size_t");
        break;
      }
      if (singleton_frontier_active_) {
        throw std::logic_error(
            "a Morton grouped common traversal overlapped a live frontier");
      }
      singleton_frontier_node_index_ =
          *traversal_step.lbvh_node_index();
      singleton_frontier_leaf_begin_ = *traversal_step.leaf_begin();
      singleton_frontier_leaf_end_ = *traversal_step.leaf_end();
      singleton_frontier_active_ = true;
      pending_anchor_subranges_ = {};
      pending_anchor_subrange_count_ = 0U;
      if (active_anchor_count_ == 1U) {
        pending_anchor_subranges_[0] = PendingAnchorLeafRange{
            active_anchor_leaf_begin_, active_anchor_leaf_end_};
        pending_anchor_subrange_count_ = 1U;
        audit_.maximum_pending_anchor_subgroup_count = std::max(
            audit_.maximum_pending_anchor_subgroup_count,
            pending_anchor_subrange_count_);
      } else {
        push_anchor_subrange_children(
            active_anchor_leaf_begin_, active_anchor_leaf_end_);
      }
      kind = ExactMortonGroupedAnchoredPairScheduleStepKind::
          singleton_fallback_started;
      checked_increment(
          audit_.common_frontier_count,
          "the Morton grouped common-frontier count overflows size_t");
      checked_add_to(
          audit_.delegated_frontier_anchor_count,
          active_anchor_count_,
          "the Morton grouped delegated-anchor count overflows size_t");
      break;
    }
    case ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf:
      kind = ExactMortonGroupedAnchoredPairScheduleStepKind::unresolved_leaf;
      checked_increment(
          audit_.unresolved_leaf_count,
          "the Morton grouped unresolved-leaf count overflows size_t");
      break;
    case ExactGroupedAnchoredPairTraversalStepKind::fallback_subtree:
      kind = ExactMortonGroupedAnchoredPairScheduleStepKind::fallback_subtree;
      checked_increment(
          audit_.fallback_subtree_count,
          "the Morton grouped fallback count overflows size_t");
      break;
    case ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted:
      kind = ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted;
      checked_increment(
          audit_.budget_exhaustion_count,
          "the Morton grouped budget-exhaustion count overflows size_t");
      switch (traversal_step.stop_reason()) {
        case ExactGroupedAnchoredPairTraversalStopReason::node_visit_limit:
          stop_reason = ExactMortonGroupedAnchoredPairScheduleStopReason::
              traversal_node_visit_limit;
          break;
        case ExactGroupedAnchoredPairTraversalStopReason::
            exact_predicate_limit:
          stop_reason = ExactMortonGroupedAnchoredPairScheduleStopReason::
              traversal_exact_predicate_limit;
          break;
        case ExactGroupedAnchoredPairTraversalStopReason::none:
          throw std::logic_error(
              "a Morton grouped traversal exhausted without a stop reason");
      }
      break;
    case ExactGroupedAnchoredPairTraversalStepKind::complete:
      throw std::logic_error(
          "a Morton grouped schedule observed an unannounced traversal completion");
  }

  ExactMortonGroupedAnchoredPairScheduleStep step =
      snapshot_step(kind, stop_reason, traversal_budget);
  step.work_ = ExactMortonGroupedAnchoredPairScheduleStepWork{
      traversal_work.node_visit_count,
      traversal_work.witness_subtree_node_visit_count,
      traversal_work.witness_slot_scan_count,
      traversal_work.inherited_witness_reuse_count,
      traversal_work.exact_predicate_count,
      traversal_work.witness_subtree_exact_predicate_count,
      traversal_work.strict_witness_discovery_count};
  if (traversal_step.traversal_complete_after_step() &&
      !singleton_frontier_active_ &&
      !active_singleton_traversal_.has_value()) {
    group_completion_pending_ = true;
  }
  step.traversal_step_.emplace(std::move(traversal_step));
  return step;
}

}  // namespace morsehgp3d::hierarchy
