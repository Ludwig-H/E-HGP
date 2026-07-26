#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_candidate_cursor.hpp"

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

ExactMortonGroupedAnchoredPairCandidateContext
ExactMortonGroupedAnchoredPairCandidateContext::start(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config) {
  return ExactMortonGroupedAnchoredPairCandidateContext(
      index,
      cloud,
      maximum_closed_rank,
      schedule_config,
      PrivateConstructionTag{});
}

ExactMortonGroupedAnchoredPairCandidateContext::
    ExactMortonGroupedAnchoredPairCandidateContext(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::size_t maximum_closed_rank,
        ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
        PrivateConstructionTag)
    : schedule_(
          ExactMortonGroupedAnchoredPairScheduleContext::start(
              index,
              cloud,
              maximum_closed_rank,
              schedule_config)) {}

ExactMortonGroupedAnchoredPairCandidateStep
ExactMortonGroupedAnchoredPairCandidateContext::make_step(
    ExactMortonGroupedAnchoredPairCandidateStepKind kind,
    ExactMortonGroupedAnchoredPairCandidateStopReason stop_reason,
    ExactMortonGroupedAnchoredPairCandidateWorkBudget budget,
    ExactMortonGroupedAnchoredPairCandidateStepWork work) const {
  ExactMortonGroupedAnchoredPairCandidateStep step;
  step.kind_ = kind;
  step.stop_reason_ = stop_reason;
  step.requested_budget_ = budget;
  step.work_ = work;
  step.group_ordinal_ = schedule_.active_group_ordinal();
  step.anchor_leaf_begin_ = schedule_.active_anchor_leaf_begin();
  step.anchor_leaf_end_ = schedule_.active_anchor_leaf_end();
  step.cursor_complete_after_step_ = complete();
  return step;
}

ExactMortonGroupedAnchoredPairCandidateStep
ExactMortonGroupedAnchoredPairCandidateContext::advance(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactMortonGroupedAnchoredPairCandidateWorkBudget budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "an oriented grouped candidate advance requires its authentic cloud and LBVH");
  }
  checked_increment(
      audit_.advance_call_count,
      "the oriented grouped candidate advance-call count overflows size_t");

  ExactMortonGroupedAnchoredPairCandidateStepWork work;
  const auto local_budget_exhausted = [&](
                                          ExactMortonGroupedAnchoredPairCandidateStopReason
                                              reason) {
    if (reason ==
        ExactMortonGroupedAnchoredPairCandidateStopReason::
            schedule_advance_limit) {
      checked_increment(
          audit_.schedule_advance_budget_exhaustion_count,
          "the oriented grouped schedule-budget count overflows size_t");
    } else if (
        reason ==
        ExactMortonGroupedAnchoredPairCandidateStopReason::
            orientation_check_limit) {
      checked_increment(
          audit_.orientation_budget_exhaustion_count,
          "the oriented grouped orientation-budget count overflows size_t");
    } else {
      throw std::logic_error(
          "an oriented grouped local exhaustion used a foreign reason");
    }
    return make_step(
        ExactMortonGroupedAnchoredPairCandidateStepKind::budget_exhausted,
        reason,
        budget,
        work);
  };
  const auto attach_schedule_step = [](
                                        ExactMortonGroupedAnchoredPairCandidateStep&
                                            step,
                                        ExactMortonGroupedAnchoredPairScheduleStep&&
                                            schedule_step) {
    step.group_ordinal_ = schedule_step.group_ordinal();
    step.anchor_leaf_begin_ = schedule_step.anchor_leaf_begin();
    step.anchor_leaf_end_ = schedule_step.anchor_leaf_end();
    step.schedule_step_.emplace(std::move(schedule_step));
  };

  if (complete()) {
    audit_.complete = true;
    return make_step(
        ExactMortonGroupedAnchoredPairCandidateStepKind::complete,
        ExactMortonGroupedAnchoredPairCandidateStopReason::none,
        budget,
        work);
  }

  for (;;) {
    if (pending_leaf_range_) {
      const std::span<const PointId> anchors =
          {pending_anchor_point_ids_.data(), pending_anchor_count_};
      if (anchors.empty() ||
          !schedule_.active_group_ordinal().has_value() ||
          !schedule_.active_anchor_leaf_begin().has_value() ||
          !schedule_.active_anchor_leaf_end().has_value()) {
        throw std::logic_error(
            "an oriented grouped pending range lost its active group");
      }
      while (pending_leaf_cursor_ < pending_leaf_end_) {
        if (pending_anchor_offset_ == anchors.size()) {
          pending_anchor_offset_ = 0U;
          ++pending_leaf_cursor_;
          continue;
        }
        if (work.orientation_check_count >=
            budget.maximum_orientation_check_count) {
          return local_budget_exhausted(
              ExactMortonGroupedAnchoredPairCandidateStopReason::
                  orientation_check_limit);
        }
        if (pending_leaf_cursor_ >= index.leaves().size()) {
          throw std::logic_error(
              "an oriented grouped cursor reached an invalid Morton leaf");
        }

        const PointId anchor_point_id = anchors[pending_anchor_offset_];
        const PointId query_point_id =
            index.leaves()[pending_leaf_cursor_].point_id;
        ++pending_anchor_offset_;
        checked_increment(
            work.orientation_check_count,
            "the oriented grouped step orientation count overflows size_t");
        checked_increment(
            audit_.orientation_check_count,
            "the oriented grouped orientation count overflows size_t");
        if (anchor_point_id >= query_point_id) {
          checked_increment(
              work.reverse_or_self_orientation_skip_count,
              "the oriented grouped step skip count overflows size_t");
          checked_increment(
              audit_.reverse_or_self_orientation_skip_count,
              "the oriented grouped skip count overflows size_t");
          continue;
        }

        checked_increment(
            audit_.candidate_pair_count,
            "the oriented grouped candidate count overflows size_t");
        ExactMortonGroupedAnchoredPairCandidateStep step = make_step(
            ExactMortonGroupedAnchoredPairCandidateStepKind::candidate_pair,
            ExactMortonGroupedAnchoredPairCandidateStopReason::none,
            budget,
            work);
        step.group_ordinal_ = pending_group_ordinal_;
        step.anchor_leaf_begin_ = pending_anchor_leaf_begin_;
        step.anchor_leaf_end_ = pending_anchor_leaf_end_;
        step.support_ids_ =
            std::array<PointId, 2>{anchor_point_id, query_point_id};
        return step;
      }
      pending_leaf_range_ = false;
      pending_anchor_point_ids_ = {};
      pending_anchor_count_ = 0U;
      pending_group_ordinal_ = 0U;
      pending_anchor_leaf_begin_ = 0U;
      pending_anchor_leaf_end_ = 0U;
      pending_leaf_cursor_ = 0U;
      pending_leaf_end_ = 0U;
      pending_anchor_offset_ = 0U;
    }

    if (work.schedule_advance_count >=
        budget.maximum_schedule_advance_count) {
      return local_budget_exhausted(
          ExactMortonGroupedAnchoredPairCandidateStopReason::
              schedule_advance_limit);
    }
    if (work.grouped_traversal_node_visit_count >
            budget.maximum_grouped_traversal_node_visit_count ||
        work.grouped_traversal_exact_predicate_count >
            budget.maximum_grouped_traversal_exact_predicate_count) {
      throw std::logic_error(
          "an oriented grouped cursor exceeded its aggregate traversal budget");
    }
    const ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget{
        budget.maximum_grouped_traversal_node_visit_count -
            work.grouped_traversal_node_visit_count,
        budget.maximum_grouped_traversal_exact_predicate_count -
            work.grouped_traversal_exact_predicate_count};
    ExactMortonGroupedAnchoredPairScheduleStep schedule_step =
        schedule_.advance(index, cloud, traversal_budget);
    checked_increment(
        work.schedule_advance_count,
        "the oriented grouped step schedule count overflows size_t");
    checked_increment(
        audit_.schedule_advance_count,
        "the oriented grouped schedule count overflows size_t");
    checked_add_to(
        work.grouped_traversal_node_visit_count,
        schedule_step.work().traversal_node_visit_count,
        "the oriented grouped step node count overflows size_t");
    checked_add_to(
        work.grouped_traversal_exact_predicate_count,
        schedule_step.work().exact_predicate_count,
        "the oriented grouped step predicate count overflows size_t");
    checked_add_to(
        audit_.grouped_traversal_node_visit_count,
        schedule_step.work().traversal_node_visit_count,
        "the oriented grouped node count overflows size_t");
    checked_add_to(
        audit_.grouped_traversal_exact_predicate_count,
        schedule_step.work().exact_predicate_count,
        "the oriented grouped predicate count overflows size_t");

    switch (schedule_step.kind()) {
      case ExactMortonGroupedAnchoredPairScheduleStepKind::certified_prune: {
        checked_increment(
            audit_.certified_prune_count,
            "the oriented grouped prune count overflows size_t");
        ExactMortonGroupedAnchoredPairCandidateStep step = make_step(
            ExactMortonGroupedAnchoredPairCandidateStepKind::certified_prune,
            ExactMortonGroupedAnchoredPairCandidateStopReason::none,
            budget,
            work);
        attach_schedule_step(step, std::move(schedule_step));
        return step;
      }
      case ExactMortonGroupedAnchoredPairScheduleStepKind::
          singleton_fallback_started: {
        const ExactGroupedAnchoredPairTraversalStep* traversal_step =
            schedule_step.traversal_step();
        if (traversal_step == nullptr ||
            traversal_step->kind() !=
                ExactGroupedAnchoredPairTraversalStepKind::
                    inconclusive_subtree ||
            !traversal_step->lbvh_node_index().has_value() ||
            !traversal_step->leaf_begin().has_value() ||
            !traversal_step->leaf_end().has_value() ||
            schedule_step.anchor_point_ids().empty()) {
          throw std::logic_error(
              "an oriented grouped cursor received an invalid singleton frontier");
        }
        continue;
      }
      case ExactMortonGroupedAnchoredPairScheduleStepKind::unresolved_leaf:
      case ExactMortonGroupedAnchoredPairScheduleStepKind::fallback_subtree: {
        const ExactGroupedAnchoredPairTraversalStep* traversal_step =
            schedule_step.traversal_step();
        if (traversal_step == nullptr ||
            !traversal_step->leaf_begin().has_value() ||
            !traversal_step->leaf_end().has_value() ||
            !schedule_step.group_ordinal().has_value() ||
            !schedule_step.anchor_leaf_begin().has_value() ||
            !schedule_step.anchor_leaf_end().has_value() ||
            !schedule_.active_anchor_leaf_begin().has_value() ||
            !schedule_.active_anchor_leaf_end().has_value() ||
            *schedule_step.anchor_leaf_begin() >=
                *schedule_step.anchor_leaf_end() ||
            *schedule_step.anchor_leaf_begin() <
                *schedule_.active_anchor_leaf_begin() ||
            *schedule_step.anchor_leaf_end() >
                *schedule_.active_anchor_leaf_end() ||
            schedule_step.anchor_point_ids().size() !=
                *schedule_step.anchor_leaf_end() -
                    *schedule_step.anchor_leaf_begin() ||
            *traversal_step->leaf_begin() >=
                *traversal_step->leaf_end() ||
            *traversal_step->leaf_end() > index.leaves().size() ||
            schedule_step.anchor_point_ids().empty() ||
            !std::includes(
                schedule_.active_anchor_point_ids().begin(),
                schedule_.active_anchor_point_ids().end(),
                schedule_step.anchor_point_ids().begin(),
                schedule_step.anchor_point_ids().end())) {
          throw std::logic_error(
              "an oriented grouped terminal lost its authenticated range or anchors");
        }
        std::copy(
            schedule_step.anchor_point_ids().begin(),
            schedule_step.anchor_point_ids().end(),
            pending_anchor_point_ids_.begin());
        pending_anchor_count_ = schedule_step.anchor_point_ids().size();
        pending_group_ordinal_ = *schedule_step.group_ordinal();
        pending_anchor_leaf_begin_ = *schedule_step.anchor_leaf_begin();
        pending_anchor_leaf_end_ = *schedule_step.anchor_leaf_end();
        pending_leaf_cursor_ = *traversal_step->leaf_begin();
        pending_leaf_end_ = *traversal_step->leaf_end();
        pending_anchor_offset_ = 0U;
        pending_leaf_range_ = true;
        audit_.maximum_pending_leaf_count = std::max(
            audit_.maximum_pending_leaf_count,
            pending_leaf_end_ - pending_leaf_cursor_);
        if (schedule_step.kind() ==
            ExactMortonGroupedAnchoredPairScheduleStepKind::unresolved_leaf) {
          checked_increment(
              audit_.opened_unresolved_leaf_range_count,
              "the oriented grouped leaf-range count overflows size_t");
        } else {
          checked_increment(
              audit_.opened_fallback_subtree_range_count,
              "the oriented grouped fallback-range count overflows size_t");
        }
        continue;
      }
      case ExactMortonGroupedAnchoredPairScheduleStepKind::group_complete: {
        checked_increment(
            audit_.completed_group_count,
            "the oriented grouped completed-group count overflows size_t");
        if (complete()) {
          audit_.complete = true;
        }
        ExactMortonGroupedAnchoredPairCandidateStep step = make_step(
            ExactMortonGroupedAnchoredPairCandidateStepKind::group_complete,
            ExactMortonGroupedAnchoredPairCandidateStopReason::none,
            budget,
            work);
        attach_schedule_step(step, std::move(schedule_step));
        return step;
      }
      case ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted: {
        ExactMortonGroupedAnchoredPairCandidateStopReason stop_reason =
            ExactMortonGroupedAnchoredPairCandidateStopReason::none;
        switch (schedule_step.stop_reason()) {
          case ExactMortonGroupedAnchoredPairScheduleStopReason::
              traversal_node_visit_limit:
            stop_reason =
                ExactMortonGroupedAnchoredPairCandidateStopReason::
                    grouped_traversal_node_visit_limit;
            break;
          case ExactMortonGroupedAnchoredPairScheduleStopReason::
              traversal_exact_predicate_limit:
            stop_reason =
                ExactMortonGroupedAnchoredPairCandidateStopReason::
                    grouped_traversal_exact_predicate_limit;
            break;
          case ExactMortonGroupedAnchoredPairScheduleStopReason::none:
            throw std::logic_error(
                "an oriented grouped schedule exhausted without a reason");
        }
        checked_increment(
            audit_.grouped_traversal_budget_exhaustion_count,
            "the oriented grouped traversal-budget count overflows size_t");
        return make_step(
            ExactMortonGroupedAnchoredPairCandidateStepKind::budget_exhausted,
            stop_reason,
            budget,
            work);
      }
      case ExactMortonGroupedAnchoredPairScheduleStepKind::complete:
        audit_.complete = true;
        return make_step(
            ExactMortonGroupedAnchoredPairCandidateStepKind::complete,
            ExactMortonGroupedAnchoredPairCandidateStopReason::none,
            budget,
            work);
    }
  }
}

}  // namespace morsehgp3d::hierarchy
