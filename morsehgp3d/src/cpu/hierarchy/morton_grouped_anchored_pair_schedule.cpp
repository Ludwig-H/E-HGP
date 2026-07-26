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
      ExactGroupedAnchoredPairTraversalContext::start_at_root(
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

ExactMortonGroupedAnchoredPairScheduleStep
ExactMortonGroupedAnchoredPairScheduleContext::snapshot_step(
    ExactMortonGroupedAnchoredPairScheduleStepKind kind,
    ExactMortonGroupedAnchoredPairScheduleStopReason stop_reason,
    ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) const {
  ExactMortonGroupedAnchoredPairScheduleStep step;
  step.kind_ = kind;
  step.stop_reason_ = stop_reason;
  step.requested_traversal_budget_ = traversal_budget;
  if (active_traversal_.has_value()) {
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

  if (group_completion_pending_ || active_traversal_->complete()) {
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

  ExactGroupedAnchoredPairTraversalStep traversal_step =
      active_traversal_->advance(index, cloud, traversal_budget);
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
      if (certificate == nullptr ||
          !traversal_step.lbvh_node_index().has_value() ||
          !certificate->certifies(
              index,
              cloud,
              *traversal_step.lbvh_node_index(),
              maximum_closed_rank_,
              std::span<const PointId>{
                  active_anchor_point_ids_.data(), active_anchor_count_})) {
        throw std::logic_error(
            "a Morton grouped schedule received an unauthenticated prune");
      }
      checked_increment(
          audit_.certified_prune_count,
          "the Morton grouped prune count overflows size_t");
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
      traversal_work.witness_slot_scan_count,
      traversal_work.inherited_witness_reuse_count,
      traversal_work.exact_predicate_count,
      traversal_work.strict_witness_discovery_count};
  if (traversal_step.traversal_complete_after_step()) {
    group_completion_pending_ = true;
  }
  step.traversal_step_.emplace(std::move(traversal_step));
  return step;
}

}  // namespace morsehgp3d::hierarchy
