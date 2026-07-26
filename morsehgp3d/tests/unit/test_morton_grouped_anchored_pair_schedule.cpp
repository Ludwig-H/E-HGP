#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_candidate_cursor.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_schedule.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationBudget;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationContext;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationResult;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationStepKind;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationStatus;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneBudget;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalStepKind;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalWorkBudget;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateAudit;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateContext;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateStep;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateStepKind;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateStopReason;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairCandidateWorkBudget;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleAudit;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleConfig;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleContext;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleStep;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleStepKind;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::build_exact_anchored_pair_witness_bank_candidates;
using morsehgp3d::hierarchy::certify_exact_grouped_anchored_pair_prune;
using morsehgp3d::hierarchy::classify_exact_anchored_pair_candidate;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Exception, typename Function>
void require_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  }
  throw std::runtime_error(message);
}

[[nodiscard]] CanonicalPointCloud make_cloud(
    std::span<const std::array<double, 3>> coordinates) {
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const std::array<double, 3>& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_line_cloud(std::size_t point_count) {
  std::vector<std::array<double, 3>> coordinates;
  coordinates.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    coordinates.push_back({static_cast<double>(index), 0.0, 0.0});
  }
  return make_cloud(coordinates);
}

[[nodiscard]] CanonicalPointCloud make_three_dimensional_cloud() {
  std::vector<std::array<double, 3>> coordinates;
  coordinates.reserve(24U);
  for (std::size_t x = 0U; x < 4U; ++x) {
    for (std::size_t y = 0U; y < 3U; ++y) {
      for (std::size_t z = 0U; z < 2U; ++z) {
        coordinates.push_back({
            static_cast<double>(3U * x),
            static_cast<double>(5U * y + (x % 2U)),
            static_cast<double>(7U * z + ((x + y) % 3U))});
      }
    }
  }
  return make_cloud(coordinates);
}

[[nodiscard]] ExactLbvhTopKBudget complete_top_k_budget(
    std::size_t point_count,
    std::size_t rank) {
  const std::size_t roomy = 8U * point_count + 32U;
  return ExactLbvhTopKBudget{
      roomy,
      roomy,
      roomy,
      roomy,
      roomy,
      rank,
      point_count};
}

[[nodiscard]] ExactAnchoredPairWitnessBankBudget complete_witness_budget(
    std::size_t point_count,
    std::size_t witness_bank_size) {
  ExactAnchoredPairWitnessBankBudget budget;
  budget.proposed_witness_bank_size = witness_bank_size;
  budget.witness_search_budget =
      complete_top_k_budget(point_count, witness_bank_size);
  budget.maximum_node_visit_count = 4U * point_count + 8U;
  budget.maximum_internal_node_expansion_count =
      2U * point_count + 8U;
  budget.maximum_traversal_stack_entry_count =
      2U * point_count + 8U;
  budget.maximum_witness_node_predicate_count =
      (4U * point_count + 8U) * (witness_bank_size + 1U);
  budget.maximum_candidate_entry_count = point_count;
  budget.maximum_prune_record_count = 2U * point_count + 8U;
  return budget;
}

struct TerminalRecord {
  ExactMortonGroupedAnchoredPairScheduleStepKind kind{};
  std::size_t event_index{};
  std::size_t group_ordinal{};
  std::size_t anchor_leaf_begin{};
  std::size_t anchor_leaf_end{};
  std::vector<PointId> anchor_point_ids;
  std::optional<std::size_t> node_index;
  std::optional<std::size_t> leaf_begin;
  std::optional<std::size_t> leaf_end;
  std::optional<PointId> unresolved_point_id;
  std::vector<PointId> certified_witness_point_ids;

  friend bool operator==(const TerminalRecord&, const TerminalRecord&) =
      default;
};

struct CommonFrontierRecord {
  std::size_t event_index{};
  std::size_t group_ordinal{};
  std::size_t anchor_leaf_begin{};
  std::size_t anchor_leaf_end{};
  std::size_t node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::vector<PointId> anchor_point_ids;

  friend bool operator==(
      const CommonFrontierRecord&,
      const CommonFrontierRecord&) = default;
};

struct AnchorSubgroupSplitRecord {
  std::size_t event_index{};
  std::size_t group_ordinal{};
  std::size_t anchor_leaf_begin{};
  std::size_t anchor_leaf_end{};
  std::size_t node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::vector<PointId> anchor_point_ids;

  friend bool operator==(
      const AnchorSubgroupSplitRecord&,
      const AnchorSubgroupSplitRecord&) = default;
};

struct GroupRecord {
  std::size_t ordinal{};
  std::size_t anchor_leaf_begin{};
  std::size_t anchor_leaf_end{};
  std::vector<PointId> anchor_point_ids;
  std::vector<PointId> witness_pool_point_ids;

  friend bool operator==(const GroupRecord&, const GroupRecord&) = default;
};

using CandidatePair = std::array<PointId, 2>;

struct ScheduleRun {
  std::vector<TerminalRecord> terminals;
  std::vector<CommonFrontierRecord> common_frontiers;
  std::vector<AnchorSubgroupSplitRecord> anchor_subgroup_splits;
  std::vector<GroupRecord> groups;
  std::set<CandidatePair> candidates;
  ExactMortonGroupedAnchoredPairScheduleAudit audit;
};

[[nodiscard]] std::vector<PointId> copied_ids(
    std::span<const PointId> ids) {
  return {ids.begin(), ids.end()};
}

[[nodiscard]] std::vector<PointId> expected_anchor_point_ids(
    const MortonLbvhIndex& index,
    std::size_t anchor_leaf_begin,
    std::size_t anchor_leaf_end) {
  require(
      anchor_leaf_begin < anchor_leaf_end &&
          anchor_leaf_end <= index.leaves().size(),
      "an anchor snapshot names an invalid Morton subrange");
  std::vector<PointId> result;
  result.reserve(anchor_leaf_end - anchor_leaf_begin);
  for (std::size_t leaf = anchor_leaf_begin;
       leaf < anchor_leaf_end;
       ++leaf) {
    result.push_back(index.leaves()[leaf].point_id);
  }
  std::sort(result.begin(), result.end());
  return result;
}

void insert_oriented_candidates(
    std::set<CandidatePair>& candidates,
    std::span<const PointId> anchors,
    const MortonLbvhIndex& index,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  require(
      leaf_begin <= leaf_end && leaf_end <= index.leaves().size(),
      "a scheduled fallback names an invalid Morton range");
  for (std::size_t leaf_offset = leaf_begin;
       leaf_offset < leaf_end;
       ++leaf_offset) {
    const PointId query_point_id = index.leaves()[leaf_offset].point_id;
    for (const PointId anchor_point_id : anchors) {
      if (anchor_point_id >= query_point_id) {
        continue;
      }
      const bool inserted =
          candidates.insert({anchor_point_id, query_point_id}).second;
      require(inserted, "the grouped schedule emitted a duplicate pair");
    }
  }
}

[[nodiscard]] ScheduleRun run_schedule(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig config,
    ExactGroupedAnchoredPairTraversalWorkBudget traversal_budget) {
  ExactMortonGroupedAnchoredPairScheduleContext schedule =
      ExactMortonGroupedAnchoredPairScheduleContext::start(
          index, cloud, maximum_closed_rank, config);
  ScheduleRun run;
  const std::size_t maximum_advance_count =
      256U * cloud.size() * cloud.size();
  std::size_t advance_count = 0U;
  std::size_t event_index = 0U;
  while (!schedule.complete()) {
    require(
        advance_count < maximum_advance_count,
        "the grouped schedule did not make bounded progress");
    ++advance_count;
    ExactMortonGroupedAnchoredPairScheduleStep step =
        schedule.advance(index, cloud, traversal_budget);
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::budget_exhausted) {
      require(
          step.traversal_step() != nullptr &&
              step.anchor_point_ids().empty() &&
              step.witness_pool_point_ids().empty(),
          "a schedule exhaustion lost its P8h cursor or copied group arrays");
      continue;
    }
    const std::size_t current_event_index = event_index;
    ++event_index;
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::group_complete) {
      require(
          step.traversal_step() == nullptr &&
              step.group_ordinal().has_value() &&
              step.anchor_leaf_begin().has_value() &&
              step.anchor_leaf_end().has_value(),
          "a group boundary lost its Morton provenance");
      run.groups.push_back(GroupRecord{
          *step.group_ordinal(),
          *step.anchor_leaf_begin(),
          *step.anchor_leaf_end(),
          copied_ids(step.anchor_point_ids()),
          copied_ids(step.witness_pool_point_ids())});
      continue;
    }
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::
            singleton_fallback_started) {
      const auto* traversal_step = step.traversal_step();
      require(
          traversal_step != nullptr &&
              traversal_step->kind() ==
                  ExactGroupedAnchoredPairTraversalStepKind::
                      inconclusive_subtree &&
              traversal_step->lbvh_node_index().has_value() &&
              traversal_step->leaf_begin().has_value() &&
              traversal_step->leaf_end().has_value() &&
              *traversal_step->leaf_begin() <
                  *traversal_step->leaf_end() &&
              traversal_step->prune_certificate() == nullptr &&
              !traversal_step->unresolved_point_id().has_value() &&
              step.group_ordinal().has_value() &&
              step.anchor_leaf_begin().has_value() &&
              step.anchor_leaf_end().has_value() &&
              !step.anchor_point_ids().empty() &&
              copied_ids(step.anchor_point_ids()) ==
                  expected_anchor_point_ids(
                      index,
                      *step.anchor_leaf_begin(),
                      *step.anchor_leaf_end()) &&
              step.witness_pool_point_ids().empty(),
          "a singleton fallback frontier gained authority or lost its exact range");
      run.common_frontiers.push_back(CommonFrontierRecord{
          current_event_index,
          *step.group_ordinal(),
          *step.anchor_leaf_begin(),
          *step.anchor_leaf_end(),
          *traversal_step->lbvh_node_index(),
          *traversal_step->leaf_begin(),
          *traversal_step->leaf_end(),
          copied_ids(step.anchor_point_ids())});
      continue;
    }
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::
            anchor_subgroup_split) {
      const auto* traversal_step = step.traversal_step();
      require(
          traversal_step != nullptr &&
              traversal_step->kind() ==
                  ExactGroupedAnchoredPairTraversalStepKind::
                      inconclusive_subtree &&
              traversal_step->lbvh_node_index().has_value() &&
              traversal_step->leaf_begin().has_value() &&
              traversal_step->leaf_end().has_value() &&
              *traversal_step->leaf_begin() <
                  *traversal_step->leaf_end() &&
              traversal_step->prune_certificate() == nullptr &&
              !traversal_step->unresolved_point_id().has_value() &&
              step.group_ordinal().has_value() &&
              step.anchor_leaf_begin().has_value() &&
              step.anchor_leaf_end().has_value() &&
              *step.anchor_leaf_end() - *step.anchor_leaf_begin() >= 2U &&
              copied_ids(step.anchor_point_ids()) ==
                  expected_anchor_point_ids(
                      index,
                      *step.anchor_leaf_begin(),
                      *step.anchor_leaf_end()) &&
              step.witness_pool_point_ids().empty(),
          "an anchor-subgroup split gained authority or lost its exact Morton subrange");
      run.anchor_subgroup_splits.push_back(AnchorSubgroupSplitRecord{
          current_event_index,
          *step.group_ordinal(),
          *step.anchor_leaf_begin(),
          *step.anchor_leaf_end(),
          *traversal_step->lbvh_node_index(),
          *traversal_step->leaf_begin(),
          *traversal_step->leaf_end(),
          copied_ids(step.anchor_point_ids())});
      continue;
    }
    require(
        step.kind() !=
            ExactMortonGroupedAnchoredPairScheduleStepKind::complete,
        "the schedule emitted completion before closing its final group after " +
            std::to_string(event_index) + " scientific/routing events and " +
            std::to_string(schedule.audit().completed_group_count) +
            " completed groups");
    require(
        step.group_ordinal().has_value() &&
            step.anchor_leaf_begin().has_value() &&
            step.anchor_leaf_end().has_value() &&
            copied_ids(step.anchor_point_ids()) ==
                expected_anchor_point_ids(
                    index,
                    *step.anchor_leaf_begin(),
                    *step.anchor_leaf_end()),
        "a schedule terminal lost its group provenance");
    const auto* traversal_step = step.traversal_step();
    require(
        traversal_step != nullptr,
        "a schedule terminal lost its exact P8h record");

    TerminalRecord terminal{
        step.kind(),
        current_event_index,
        *step.group_ordinal(),
        *step.anchor_leaf_begin(),
        *step.anchor_leaf_end(),
        copied_ids(step.anchor_point_ids()),
        traversal_step->lbvh_node_index(),
        traversal_step->leaf_begin(),
        traversal_step->leaf_end(),
        traversal_step->unresolved_point_id(),
        {}};
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::certified_prune) {
      require(
          traversal_step->kind() ==
                  ExactGroupedAnchoredPairTraversalStepKind::certified_prune &&
              traversal_step->lbvh_node_index().has_value(),
          "a scheduled prune lost its P8h node");
      const auto* certificate = traversal_step->prune_certificate();
      require(certificate != nullptr, "a scheduled prune lost its certificate");
      const ExactGroupedAnchoredPairPruneBudget fresh_budget{
          step.anchor_point_ids().size(),
          step.witness_pool_point_ids().size(),
          step.anchor_point_ids().size() *
              step.witness_pool_point_ids().size()};
      const auto fresh = certify_exact_grouped_anchored_pair_prune(
          index,
          cloud,
          step.anchor_point_ids(),
          step.witness_pool_point_ids(),
          *traversal_step->lbvh_node_index(),
          maximum_closed_rank,
          fresh_budget);
      require(
          certificate->certifies(
              index,
              cloud,
              *traversal_step->lbvh_node_index(),
              maximum_closed_rank,
              step.anchor_point_ids()) &&
              fresh.certifies(
                  index,
                  cloud,
                  *traversal_step->lbvh_node_index(),
                  maximum_closed_rank,
                  step.anchor_point_ids()) &&
              fresh.leaf_begin() == certificate->leaf_begin() &&
              fresh.leaf_end() == certificate->leaf_end() &&
              std::equal(
                  fresh.certified_witness_point_ids().begin(),
                  fresh.certified_witness_point_ids().end(),
                  certificate->certified_witness_point_ids().begin(),
                  certificate->certified_witness_point_ids().end()),
          "a scheduled P8h prune differs from its fresh P8g certificate");
      terminal.certified_witness_point_ids =
          copied_ids(certificate->certified_witness_point_ids());
    } else if (
        step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::unresolved_leaf) {
      require(
          traversal_step->unresolved_point_id().has_value() &&
              traversal_step->leaf_begin().has_value() &&
              traversal_step->leaf_end().has_value() &&
              step.witness_pool_point_ids().empty(),
          "a scheduled unresolved leaf lost its Morton record or copied its pool");
      insert_oriented_candidates(
          run.candidates,
          step.anchor_point_ids(),
          index,
          *traversal_step->leaf_begin(),
          *traversal_step->leaf_end());
    } else if (
        step.kind() ==
        ExactMortonGroupedAnchoredPairScheduleStepKind::fallback_subtree) {
      require(
          traversal_step->leaf_begin().has_value() &&
              traversal_step->leaf_end().has_value() &&
              step.witness_pool_point_ids().empty(),
          "a scheduled fallback lost its Morton range or copied its pool");
      insert_oriented_candidates(
          run.candidates,
          step.anchor_point_ids(),
          index,
          *traversal_step->leaf_begin(),
          *traversal_step->leaf_end());
    } else {
      throw std::logic_error("the schedule emitted an unknown terminal kind");
    }
    run.terminals.push_back(std::move(terminal));
  }
  const ExactMortonGroupedAnchoredPairScheduleStep completed =
      schedule.advance(index, cloud, traversal_budget);
  require(
      completed.kind() ==
              ExactMortonGroupedAnchoredPairScheduleStepKind::complete &&
          completed.traversal_step() == nullptr &&
          completed.anchor_point_ids().empty(),
      "a completed schedule retained an active group");
  run.audit = schedule.audit();
  return run;
}

[[nodiscard]] std::vector<PointId> expected_halo(
    const MortonLbvhIndex& index,
    std::size_t begin,
    std::size_t end,
    std::size_t capacity) {
  std::vector<PointId> result;
  const std::size_t left_available = begin;
  const std::size_t right_available = index.leaves().size() - end;
  const std::size_t maximum_offset =
      std::max(left_available, right_available);
  for (std::size_t offset = 1U;
       offset <= maximum_offset && result.size() < capacity;
       ++offset) {
    if (offset <= left_available) {
      result.push_back(index.leaves()[begin - offset].point_id);
    }
    if (offset <= right_available && result.size() < capacity) {
      result.push_back(index.leaves()[end + offset - 1U].point_id);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

void validate_group_partition(
    const MortonLbvhIndex& index,
    const ScheduleRun& run,
    ExactMortonGroupedAnchoredPairScheduleConfig config) {
  const std::size_t point_count = index.leaves().size();
  const std::size_t expected_group_count =
      (point_count + config.maximum_anchor_count_per_group - 1U) /
      config.maximum_anchor_count_per_group;
  require(
      run.groups.size() == expected_group_count,
      "the scheduler changed the bounded group count");
  std::vector<bool> seen_point_ids(point_count, false);
  std::size_t expected_begin = 0U;
  for (std::size_t ordinal = 0U; ordinal < run.groups.size(); ++ordinal) {
    const GroupRecord& group = run.groups[ordinal];
    require(
        group.ordinal == ordinal &&
            group.anchor_leaf_begin == expected_begin &&
            group.anchor_leaf_end > group.anchor_leaf_begin &&
            group.anchor_leaf_end - group.anchor_leaf_begin <=
                config.maximum_anchor_count_per_group,
        "the scheduler lost its contiguous Morton partition");
    std::vector<PointId> expected_anchors;
    for (std::size_t leaf = group.anchor_leaf_begin;
         leaf < group.anchor_leaf_end;
         ++leaf) {
      const PointId point_id = index.leaves()[leaf].point_id;
      expected_anchors.push_back(point_id);
      require(
          static_cast<std::size_t>(point_id) < seen_point_ids.size() &&
              !seen_point_ids[static_cast<std::size_t>(point_id)],
          "a PointId was scheduled as an anchor more than once");
      seen_point_ids[static_cast<std::size_t>(point_id)] = true;
    }
    std::sort(expected_anchors.begin(), expected_anchors.end());
    require(
        group.anchor_point_ids == expected_anchors &&
            group.witness_pool_point_ids == expected_halo(
                index,
                group.anchor_leaf_begin,
                group.anchor_leaf_end,
                config.proposed_witness_pool_size) &&
            std::is_sorted(
                group.anchor_point_ids.begin(),
                group.anchor_point_ids.end()) &&
            std::is_sorted(
                group.witness_pool_point_ids.begin(),
                group.witness_pool_point_ids.end()),
        "a group changed its canonical anchors or bounded Morton halo");
    for (const PointId witness : group.witness_pool_point_ids) {
      require(
          !std::binary_search(
              group.anchor_point_ids.begin(),
              group.anchor_point_ids.end(),
              witness),
          "a Morton halo retained one of its anchors");
    }
    expected_begin = group.anchor_leaf_end;
  }
  require(
      expected_begin == point_count &&
          std::all_of(
              seen_point_ids.begin(),
              seen_point_ids.end(),
              [](bool seen) { return seen; }) &&
          run.audit.prepared_group_count == expected_group_count &&
          run.audit.completed_group_count == expected_group_count &&
          run.audit.scheduled_anchor_count == point_count &&
          run.audit.complete &&
          run.audit.morton_anchor_partition_complete &&
          run.audit.no_global_anchor_pair_or_output_arena_materialized,
      "the scheduler did not close its exact anchor partition audit");

  std::size_t delegated_frontier_anchor_count = 0U;
  for (const CommonFrontierRecord& frontier : run.common_frontiers) {
    require(
        frontier.group_ordinal < run.groups.size(),
        "a common frontier names an invalid Morton group");
    const GroupRecord& group = run.groups[frontier.group_ordinal];
    require(
        frontier.anchor_leaf_begin == group.anchor_leaf_begin &&
            frontier.anchor_leaf_end == group.anchor_leaf_end &&
            frontier.anchor_point_ids == group.anchor_point_ids &&
            frontier.leaf_begin < frontier.leaf_end &&
            frontier.leaf_end <= point_count,
        "a common frontier lost its full-group or LBVH provenance");
    delegated_frontier_anchor_count +=
        frontier.anchor_point_ids.size();
  }
  require(
      run.audit.common_frontier_count == run.common_frontiers.size() &&
          run.audit.delegated_frontier_anchor_count ==
              delegated_frontier_anchor_count &&
          run.audit.anchor_subgroup_split_count ==
              run.anchor_subgroup_splits.size() &&
          run.audit.maximum_pending_anchor_subgroup_count <=
              config.maximum_anchor_count_per_group &&
          run.audit.singleton_certified_prune_count <=
              run.audit.certified_prune_count,
      "the scheduler audit lost its bounded recursive anchor partition");
}

void validate_common_frontier_recursive_anchor_partitions(
    const MortonLbvhIndex& index,
    const ScheduleRun& run,
    ExactMortonGroupedAnchoredPairScheduleConfig config) {
  require(
      !run.common_frontiers.empty(),
      "the P8q fixture did not open a common frontier");

  using AnchorLeafRange = std::pair<std::size_t, std::size_t>;
  std::size_t expected_subgroup_probe_count = 0U;
  std::size_t expected_subgroup_pool_entry_count = 0U;
  std::size_t expected_subgroup_prune_count = 0U;
  std::size_t expected_subgroup_certified_anchor_count = 0U;
  std::size_t expected_singleton_fallback_count = 0U;
  std::size_t expected_singleton_pool_entry_count = 0U;
  std::size_t expected_singleton_prune_count = 0U;

  for (const CommonFrontierRecord& frontier : run.common_frontiers) {
    std::size_t next_frontier_event =
        std::numeric_limits<std::size_t>::max();
    for (const CommonFrontierRecord& candidate : run.common_frontiers) {
      if (candidate.group_ordinal == frontier.group_ordinal &&
          candidate.event_index > frontier.event_index) {
        next_frontier_event =
            std::min(next_frontier_event, candidate.event_index);
      }
    }

    const auto belongs_to_frontier = [&](const TerminalRecord& terminal) {
      return terminal.group_ordinal == frontier.group_ordinal &&
          terminal.event_index > frontier.event_index &&
          terminal.event_index < next_frontier_event &&
          terminal.leaf_begin.has_value() &&
          terminal.leaf_end.has_value() &&
          frontier.leaf_begin <= *terminal.leaf_begin &&
          *terminal.leaf_end <= frontier.leaf_end;
    };

    std::set<AnchorLeafRange> split_ranges;
    std::set<AnchorLeafRange> subgroup_prune_ranges;
    std::set<AnchorLeafRange> singleton_ranges;
    for (const AnchorSubgroupSplitRecord& split :
         run.anchor_subgroup_splits) {
      if (split.group_ordinal != frontier.group_ordinal ||
          split.event_index <= frontier.event_index ||
          split.event_index >= next_frontier_event) {
        continue;
      }
      const AnchorLeafRange range{
          split.anchor_leaf_begin, split.anchor_leaf_end};
      require(
          frontier.anchor_leaf_begin <= split.anchor_leaf_begin &&
              split.anchor_leaf_end <= frontier.anchor_leaf_end &&
              split.anchor_leaf_end - split.anchor_leaf_begin >= 2U &&
              split.node_index == frontier.node_index &&
              split.leaf_begin == frontier.leaf_begin &&
              split.leaf_end == frontier.leaf_end &&
              split.anchor_point_ids == expected_anchor_point_ids(
                  index,
                  split.anchor_leaf_begin,
                  split.anchor_leaf_end) &&
              split_ranges.insert(range).second,
          "a recursive subgroup split is duplicated or escaped its common frontier");
      ++expected_subgroup_probe_count;
      expected_subgroup_pool_entry_count += expected_halo(
          index,
          split.anchor_leaf_begin,
          split.anchor_leaf_end,
          config.proposed_witness_pool_size).size();
    }

    for (const TerminalRecord& terminal : run.terminals) {
      if (!belongs_to_frontier(terminal)) {
        continue;
      }
      const AnchorLeafRange range{
          terminal.anchor_leaf_begin, terminal.anchor_leaf_end};
      require(
          terminal.node_index.has_value() &&
              terminal.leaf_begin.has_value() &&
              terminal.leaf_end.has_value() &&
              frontier.anchor_leaf_begin <= terminal.anchor_leaf_begin &&
              terminal.anchor_leaf_end <= frontier.anchor_leaf_end &&
              terminal.anchor_point_ids == expected_anchor_point_ids(
                  index,
                  terminal.anchor_leaf_begin,
                  terminal.anchor_leaf_end),
          "a recursive anchor fallback escaped its common frontier");
      if (terminal.anchor_point_ids.size() == 1U) {
        if (singleton_ranges.insert(range).second) {
          ++expected_singleton_fallback_count;
          expected_singleton_pool_entry_count += expected_halo(
              index,
              terminal.anchor_leaf_begin,
              terminal.anchor_leaf_end,
              config.proposed_witness_pool_size).size();
        }
        if (terminal.kind ==
            ExactMortonGroupedAnchoredPairScheduleStepKind::certified_prune) {
          ++expected_singleton_prune_count;
        }
      } else {
        require(
            terminal.kind ==
                    ExactMortonGroupedAnchoredPairScheduleStepKind::
                        certified_prune &&
                *terminal.node_index == frontier.node_index &&
                *terminal.leaf_begin == frontier.leaf_begin &&
                *terminal.leaf_end == frontier.leaf_end &&
                subgroup_prune_ranges.insert(range).second,
            "a subgroup terminal is not one exact prune of its whole frontier");
        ++expected_subgroup_probe_count;
        ++expected_subgroup_prune_count;
        expected_subgroup_certified_anchor_count +=
            terminal.anchor_point_ids.size();
        expected_subgroup_pool_entry_count += expected_halo(
            index,
            terminal.anchor_leaf_begin,
            terminal.anchor_leaf_end,
            config.proposed_witness_pool_size).size();
      }
    }

    const auto split_for = [&](AnchorLeafRange range) {
      return std::find_if(
          run.anchor_subgroup_splits.begin(),
          run.anchor_subgroup_splits.end(),
          [&](const AnchorSubgroupSplitRecord& split) {
            return split.group_ordinal == frontier.group_ordinal &&
                split.event_index > frontier.event_index &&
                split.event_index < next_frontier_event &&
                split.anchor_leaf_begin == range.first &&
                split.anchor_leaf_end == range.second;
          });
    };
    const auto first_terminal_event_for = [&](AnchorLeafRange range) {
      std::size_t event = std::numeric_limits<std::size_t>::max();
      for (const TerminalRecord& terminal : run.terminals) {
        if (belongs_to_frontier(terminal) &&
            terminal.anchor_leaf_begin == range.first &&
            terminal.anchor_leaf_end == range.second) {
          event = std::min(event, terminal.event_index);
        }
      }
      return event;
    };

    std::set<AnchorLeafRange> visited_splits;
    std::set<AnchorLeafRange> visited_leaves;
    std::function<void(AnchorLeafRange, std::size_t)> validate_subtree;
    validate_subtree = [&](AnchorLeafRange range, std::size_t parent_event) {
      require(
          frontier.anchor_leaf_begin <= range.first &&
              range.first < range.second &&
              range.second <= frontier.anchor_leaf_end,
          "a recursive anchor child is not a strict Morton subinterval");
      const auto split = split_for(range);
      if (split != run.anchor_subgroup_splits.end()) {
        require(
            split->event_index > parent_event &&
                split_ranges.contains(range) &&
                !subgroup_prune_ranges.contains(range) &&
                !singleton_ranges.contains(range) &&
                visited_splits.insert(range).second,
            "an anchor subrange was both split and terminal, or appeared out of order");
        const std::size_t midpoint =
            range.first + (range.second - range.first) / 2U;
        require(
            range.first < midpoint && midpoint < range.second,
            "an anchor-subgroup split did not create two strict children");
        validate_subtree(
            AnchorLeafRange{midpoint, range.second}, split->event_index);
        validate_subtree(
            AnchorLeafRange{range.first, midpoint}, split->event_index);
        return;
      }

      const std::size_t terminal_event = first_terminal_event_for(range);
      require(
          terminal_event != std::numeric_limits<std::size_t>::max() &&
              terminal_event > parent_event &&
              (subgroup_prune_ranges.contains(range) !=
               singleton_ranges.contains(range)) &&
              visited_leaves.insert(range).second,
          "a recursive anchor child has neither one subgroup prune nor one singleton traversal");
    };

    const AnchorLeafRange root{
        frontier.anchor_leaf_begin, frontier.anchor_leaf_end};
    if (root.second - root.first == 1U) {
      validate_subtree(root, frontier.event_index);
    } else {
      const std::size_t midpoint =
          root.first + (root.second - root.first) / 2U;
      require(
          root.first < midpoint && midpoint < root.second,
          "a common frontier did not admit two strict initial subgroups");
      validate_subtree(
          AnchorLeafRange{midpoint, root.second}, frontier.event_index);
      validate_subtree(
          AnchorLeafRange{root.first, midpoint}, frontier.event_index);
    }
    require(
        visited_splits == split_ranges &&
            visited_leaves.size() ==
                subgroup_prune_ranges.size() + singleton_ranges.size(),
        "the recursive anchor tree retained an unreachable split or terminal");
  }

  require(
      run.audit.prepared_anchor_subgroup_probe_count ==
              expected_subgroup_probe_count &&
          run.audit.proposed_anchor_subgroup_witness_pool_entry_count ==
              expected_subgroup_pool_entry_count &&
          run.audit.anchor_subgroup_split_count ==
              run.anchor_subgroup_splits.size() &&
          run.audit.anchor_subgroup_certified_prune_count ==
              expected_subgroup_prune_count &&
          run.audit.anchor_subgroup_certified_anchor_count ==
              expected_subgroup_certified_anchor_count &&
          run.audit.prepared_singleton_fallback_count ==
              expected_singleton_fallback_count &&
          run.audit.completed_singleton_fallback_count ==
              expected_singleton_fallback_count &&
          run.audit.proposed_singleton_witness_pool_entry_count ==
              expected_singleton_pool_entry_count &&
          run.audit.singleton_certified_prune_count ==
              expected_singleton_prune_count &&
          run.audit.maximum_pending_anchor_subgroup_count > 0U,
      "the P8q audit differs from the observed recursive anchor partition");
}

struct ScientificRecord {
  CandidatePair support_ids{};
  std::optional<ExactPairSupportEvent> event;
  std::optional<ExactPairSupportExtraShellDiagnostic> diagnostic;

  friend bool operator==(const ScientificRecord&, const ScientificRecord&) =
      default;
};

[[nodiscard]] std::vector<ScientificRecord> classify_candidates(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::set<CandidatePair>& candidates,
    std::size_t maximum_closed_rank) {
  std::vector<ScientificRecord> records;
  const ExactAnchoredPairCandidateClassificationBudget budget{
      4U * cloud.size() + 8U};
  for (const CandidatePair& support_ids : candidates) {
    const auto result = classify_exact_anchored_pair_candidate(
        index, cloud, support_ids, maximum_closed_rank, budget);
    require(
        result.status !=
            ExactAnchoredPairCandidateClassificationStatus::budget_exhausted,
        "the exact candidate classifier exhausted its roomy test budget");
    if (result.status ==
        ExactAnchoredPairCandidateClassificationStatus::above_rank) {
      continue;
    }
    records.push_back(ScientificRecord{
        support_ids, result.event, result.relevant_extra_shell_diagnostic});
  }
  return records;
}

[[nodiscard]] std::set<CandidatePair> anchored_path_candidates(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  const std::size_t witness_bank_size = std::min(
      cloud.size() - 1U,
      morsehgp3d::hierarchy::exact_anchored_pair_witness_bank_maximum_size);
  const ExactAnchoredPairWitnessBankBudget budget =
      complete_witness_budget(cloud.size(), witness_bank_size);
  std::set<CandidatePair> candidates;
  for (std::size_t anchor = 0U; anchor < cloud.size(); ++anchor) {
    const PointId anchor_point_id = static_cast<PointId>(anchor);
    const auto result = build_exact_anchored_pair_witness_bank_candidates(
        index,
        cloud,
        anchor_point_id,
        maximum_closed_rank,
        budget);
    require(result.complete(), "the exact anchored candidate path stopped");
    for (const PointId query_point_id : result.candidate_point_ids) {
      require(
          anchor_point_id < query_point_id,
          "the exact anchored path changed its PointId orientation");
      const bool inserted =
          candidates.insert({anchor_point_id, query_point_id}).second;
      require(inserted, "the exact anchored path duplicated a pair");
    }
  }
  return candidates;
}

struct CandidateAuthorityRecord {
  std::size_t group_ordinal{};
  std::vector<PointId> anchor_point_ids;
  std::size_t node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::vector<PointId> certified_witness_point_ids;

  friend bool operator==(
      const CandidateAuthorityRecord&,
      const CandidateAuthorityRecord&) = default;
};

struct CandidateCursorRun {
  std::vector<CandidatePair> candidate_pairs;
  std::vector<ScientificRecord> scientific_records;
  std::vector<CandidateAuthorityRecord> prune_records;
  std::vector<GroupRecord> groups;
  ExactMortonGroupedAnchoredPairCandidateAudit audit;
  ExactMortonGroupedAnchoredPairScheduleAudit schedule_audit;
  std::size_t classification_node_visit_count{};
  std::size_t classification_budget_exhaustion_count{};
};

[[nodiscard]] CandidateCursorRun run_candidate_cursor(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig config,
    ExactMortonGroupedAnchoredPairCandidateWorkBudget budget) {
  ExactMortonGroupedAnchoredPairCandidateContext cursor =
      ExactMortonGroupedAnchoredPairCandidateContext::start(
          index, cloud, maximum_closed_rank, config);
  CandidateCursorRun run;
  std::set<CandidatePair> seen_candidates;
  const std::size_t maximum_advance_count =
      512U * cloud.size() * cloud.size();
  std::size_t advance_count = 0U;
  while (!cursor.complete()) {
    require(
        advance_count < maximum_advance_count,
        "the oriented candidate cursor did not make bounded progress");
    ++advance_count;
    ExactMortonGroupedAnchoredPairCandidateStep step =
        cursor.advance(index, cloud, budget);
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairCandidateStepKind::budget_exhausted) {
      require(
          step.schedule_step() == nullptr,
          "an oriented cursor budget stop copied a nested schedule payload");
      continue;
    }
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairCandidateStepKind::candidate_pair) {
      require(
          step.support_ids().has_value() &&
              (*step.support_ids())[0] < (*step.support_ids())[1] &&
              step.schedule_step() == nullptr &&
              step.group_ordinal().has_value(),
          "an oriented cursor candidate lost its canonical provenance");
      const CandidatePair support_ids = *step.support_ids();
      require(
          seen_candidates.insert(support_ids).second,
          "the oriented candidate cursor emitted a duplicate pair");
      run.candidate_pairs.push_back(support_ids);
      ExactAnchoredPairCandidateClassificationContext classifier =
          ExactAnchoredPairCandidateClassificationContext::start(
              index, cloud, support_ids, maximum_closed_rank);
      std::optional<ExactAnchoredPairCandidateClassificationResult>
          classification;
      for (std::size_t classifier_advance = 0U;
           classifier_advance < 4096U && !classifier.complete();
           ++classifier_advance) {
        const auto classifier_step = classifier.advance(
            index,
            cloud,
            ExactAnchoredPairCandidateClassificationBudget{1U});
        if (classifier_step.kind ==
            ExactAnchoredPairCandidateClassificationStepKind::
                budget_exhausted) {
          continue;
        }
        if (classifier_step.kind ==
            ExactAnchoredPairCandidateClassificationStepKind::record_ready) {
          classification.emplace(classifier.take_result(index, cloud));
        } else {
          require(
              classifier_step.kind ==
                      ExactAnchoredPairCandidateClassificationStepKind::
                          above_rank &&
                  classifier.complete(),
              "P8k returned a nonterminal while consuming a P8j candidate");
        }
      }
      require(
          classifier.complete(),
          "P8k did not finish before the next P8j candidate");
      run.classification_node_visit_count +=
          classifier.audit().node_visit_count;
      run.classification_budget_exhaustion_count +=
          classifier.audit().budget_exhaustion_count;
      if (classification.has_value()) {
        run.scientific_records.push_back(ScientificRecord{
            support_ids,
            classification->event,
            classification->relevant_extra_shell_diagnostic});
      }
      continue;
    }

    const auto* schedule_step = step.schedule_step();
    require(
        schedule_step != nullptr,
        "a noncandidate cursor event lost its P8i record");
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairCandidateStepKind::certified_prune) {
      const auto* traversal_step = schedule_step->traversal_step();
      require(
          traversal_step != nullptr &&
              traversal_step->lbvh_node_index().has_value() &&
              traversal_step->leaf_begin().has_value() &&
              traversal_step->leaf_end().has_value() &&
              schedule_step->group_ordinal().has_value(),
          "an oriented cursor prune lost its P8i/P8h provenance");
      const auto* certificate = traversal_step->prune_certificate();
      require(
          certificate != nullptr &&
              certificate->certifies(
                  index,
                  cloud,
                  *traversal_step->lbvh_node_index(),
                  maximum_closed_rank,
                  schedule_step->anchor_point_ids()),
          "an oriented cursor prune lost its P8g authority");
      run.prune_records.push_back(CandidateAuthorityRecord{
          *schedule_step->group_ordinal(),
          copied_ids(schedule_step->anchor_point_ids()),
          *traversal_step->lbvh_node_index(),
          *traversal_step->leaf_begin(),
          *traversal_step->leaf_end(),
          copied_ids(certificate->certified_witness_point_ids())});
    } else if (
        step.kind() ==
        ExactMortonGroupedAnchoredPairCandidateStepKind::group_complete) {
      require(
          schedule_step->group_ordinal().has_value() &&
              schedule_step->anchor_leaf_begin().has_value() &&
              schedule_step->anchor_leaf_end().has_value(),
          "an oriented cursor group boundary lost its P8i provenance");
      run.groups.push_back(GroupRecord{
          *schedule_step->group_ordinal(),
          *schedule_step->anchor_leaf_begin(),
          *schedule_step->anchor_leaf_end(),
          copied_ids(schedule_step->anchor_point_ids()),
          copied_ids(schedule_step->witness_pool_point_ids())});
    } else {
      throw std::logic_error(
          "the oriented cursor completed before its final group boundary");
    }
  }
  const ExactMortonGroupedAnchoredPairCandidateStep completed =
      cursor.advance(index, cloud, budget);
  require(
      completed.kind() ==
              ExactMortonGroupedAnchoredPairCandidateStepKind::complete &&
          completed.cursor_complete_after_step() &&
          completed.schedule_step() == nullptr,
      "a completed oriented cursor retained a pending candidate or group");
  std::sort(
      run.scientific_records.begin(),
      run.scientific_records.end(),
      [](const ScientificRecord& left, const ScientificRecord& right) {
        return left.support_ids < right.support_ids;
      });
  run.audit = cursor.audit();
  run.schedule_audit = cursor.schedule_audit();
  return run;
}

void test_oriented_candidate_cursor_identity_and_budgets() {
  CanonicalPointCloud cloud = make_three_dimensional_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactMortonGroupedAnchoredPairScheduleConfig config{5U, 16U};
  const CandidateCursorRun roomy = run_candidate_cursor(
      index,
      cloud,
      maximum_closed_rank,
      config,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget{
          4096U, 4096U, 4096U, 4096U});
  const CandidateCursorRun segmented = run_candidate_cursor(
      index,
      cloud,
      maximum_closed_rank,
      config,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget{1U, 1U, 1U, 1U});

  const std::set<CandidatePair> anchored_candidates =
      anchored_path_candidates(index, cloud, maximum_closed_rank);
  const std::vector<ScientificRecord> anchored_science = classify_candidates(
      index, cloud, anchored_candidates, maximum_closed_rank);
  require(
      roomy.candidate_pairs == segmented.candidate_pairs &&
          roomy.scientific_records == segmented.scientific_records &&
          roomy.prune_records == segmented.prune_records &&
          roomy.groups == segmented.groups &&
          roomy.classification_node_visit_count ==
              segmented.classification_node_visit_count &&
          roomy.classification_budget_exhaustion_count ==
              segmented.classification_budget_exhaustion_count &&
          roomy.classification_budget_exhaustion_count > 0U &&
          roomy.scientific_records == anchored_science,
      "candidate-cursor segmentation or resumable P8k consumption changed the scientific stream");
  require(
      roomy.audit.candidate_pair_count == roomy.candidate_pairs.size() &&
          roomy.audit.orientation_check_count ==
              roomy.audit.candidate_pair_count +
                  roomy.audit.reverse_or_self_orientation_skip_count,
      "the oriented cursor audit lost its candidate-orientation identity");
  require(
      roomy.audit.grouped_traversal_node_visit_count ==
              segmented.audit.grouped_traversal_node_visit_count &&
          roomy.audit.grouped_traversal_exact_predicate_count ==
              segmented.audit.grouped_traversal_exact_predicate_count &&
          roomy.schedule_audit.diagonal_node_descent_count ==
              segmented.schedule_audit.diagonal_node_descent_count &&
          roomy.schedule_audit.common_frontier_count ==
              segmented.schedule_audit.common_frontier_count &&
          roomy.schedule_audit.delegated_frontier_anchor_count ==
              segmented.schedule_audit.delegated_frontier_anchor_count &&
          roomy.schedule_audit.prepared_anchor_subgroup_probe_count ==
              segmented.schedule_audit.prepared_anchor_subgroup_probe_count &&
          roomy.schedule_audit
                  .proposed_anchor_subgroup_witness_pool_entry_count ==
              segmented.schedule_audit
                  .proposed_anchor_subgroup_witness_pool_entry_count &&
          roomy.schedule_audit.anchor_subgroup_split_count ==
              segmented.schedule_audit.anchor_subgroup_split_count &&
          roomy.schedule_audit.anchor_subgroup_certified_prune_count ==
              segmented.schedule_audit
                  .anchor_subgroup_certified_prune_count &&
          roomy.schedule_audit.anchor_subgroup_certified_anchor_count ==
              segmented.schedule_audit
                  .anchor_subgroup_certified_anchor_count &&
          roomy.schedule_audit.prepared_singleton_fallback_count ==
              segmented.schedule_audit.prepared_singleton_fallback_count &&
          roomy.schedule_audit.completed_singleton_fallback_count ==
              segmented.schedule_audit.completed_singleton_fallback_count &&
          roomy.schedule_audit
                  .proposed_singleton_witness_pool_entry_count ==
              segmented.schedule_audit
                  .proposed_singleton_witness_pool_entry_count &&
          roomy.schedule_audit.singleton_certified_prune_count ==
              segmented.schedule_audit.singleton_certified_prune_count &&
          roomy.schedule_audit.maximum_pending_anchor_subgroup_count ==
              segmented.schedule_audit.maximum_pending_anchor_subgroup_count,
      "candidate-cursor segmentation changed the exact traversal work");
  require(
      roomy.audit.schedule_advance_budget_exhaustion_count == 0U &&
          roomy.audit.orientation_budget_exhaustion_count == 0U &&
          roomy.audit.grouped_traversal_budget_exhaustion_count == 0U,
      "the roomy oriented cursor unexpectedly exhausted a local budget");
  require(
      segmented.audit.schedule_advance_budget_exhaustion_count +
                  segmented.audit.orientation_budget_exhaustion_count +
                  segmented.audit.grouped_traversal_budget_exhaustion_count >
              0U,
      "the segmented oriented cursor did not exercise local exhaustion");
  require(
      roomy.audit.complete && segmented.audit.complete &&
          roomy.audit.no_dynamic_candidate_or_output_arena_materialized &&
          roomy.schedule_audit.morton_anchor_partition_complete &&
          segmented.schedule_audit.morton_anchor_partition_complete,
      "the oriented cursor audit lost its completion or bounded-state identity");

  const ExactMortonGroupedAnchoredPairScheduleConfig fallback_config{5U, 2U};
  const CandidateCursorRun fallback = run_candidate_cursor(
      index,
      cloud,
      maximum_closed_rank,
      fallback_config,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget{
          4096U, 4096U, 4096U, 4096U});
  const std::size_t all_pair_count =
      cloud.size() * (cloud.size() - 1U) / 2U;
  require(
      fallback.candidate_pairs.size() == all_pair_count &&
          fallback.audit.orientation_check_count ==
              cloud.size() * cloud.size() &&
          fallback.audit.reverse_or_self_orientation_skip_count ==
              cloud.size() * (cloud.size() + 1U) / 2U &&
          fallback.audit.opened_fallback_subtree_range_count ==
              fallback.groups.size() &&
          fallback.audit.certified_prune_count == 0U &&
          fallback.scientific_records == anchored_science,
      "the oriented fallback cursor lost a pair or changed P8c science");
}

void test_oriented_candidate_cursor_atomic_local_budgets() {
  CanonicalPointCloud cloud = make_line_cloud(8U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactMortonGroupedAnchoredPairCandidateContext cursor =
      ExactMortonGroupedAnchoredPairCandidateContext::start(
          index, cloud, 4U, {4U, 0U});
  const ExactMortonGroupedAnchoredPairCandidateStep no_schedule_work =
      cursor.advance(
          index,
          cloud,
          ExactMortonGroupedAnchoredPairCandidateWorkBudget{});
  require(
      no_schedule_work.kind() ==
              ExactMortonGroupedAnchoredPairCandidateStepKind::
                  budget_exhausted &&
          no_schedule_work.stop_reason() ==
              ExactMortonGroupedAnchoredPairCandidateStopReason::
                  schedule_advance_limit &&
          no_schedule_work.work() ==
              morsehgp3d::hierarchy::
                  ExactMortonGroupedAnchoredPairCandidateStepWork{} &&
          cursor.audit().schedule_advance_count == 0U,
      "a zero schedule budget mutated the nested P8i cursor");

  const ExactMortonGroupedAnchoredPairCandidateStep opened_range =
      cursor.advance(
          index,
          cloud,
          ExactMortonGroupedAnchoredPairCandidateWorkBudget{1U, 0U, 1U, 0U});
  require(
      opened_range.kind() ==
              ExactMortonGroupedAnchoredPairCandidateStepKind::
                  budget_exhausted &&
          opened_range.stop_reason() ==
              ExactMortonGroupedAnchoredPairCandidateStopReason::
                  orientation_check_limit &&
          opened_range.work().schedule_advance_count == 1U &&
          opened_range.work().grouped_traversal_node_visit_count == 1U &&
          cursor.audit().opened_fallback_subtree_range_count == 1U &&
          cursor.audit().candidate_pair_count == 0U,
      "opening a fallback range leaked a candidate through a zero orientation budget");

  std::optional<CandidatePair> first_candidate;
  for (std::size_t attempt = 0U;
       attempt < 16U && !first_candidate.has_value();
       ++attempt) {
    const ExactMortonGroupedAnchoredPairCandidateStep step = cursor.advance(
        index,
        cloud,
        ExactMortonGroupedAnchoredPairCandidateWorkBudget{0U, 1U, 0U, 0U});
    require(
        step.work().schedule_advance_count == 0U,
        "resuming a pending range advanced P8i prematurely");
    if (step.kind() ==
        ExactMortonGroupedAnchoredPairCandidateStepKind::candidate_pair) {
      first_candidate = step.support_ids();
    } else {
      require(
          step.kind() ==
                  ExactMortonGroupedAnchoredPairCandidateStepKind::
                      budget_exhausted &&
              step.stop_reason() ==
                  ExactMortonGroupedAnchoredPairCandidateStopReason::
                      orientation_check_limit,
          "a one-check pending-range resume changed its typed stop");
    }
  }
  require(
      first_candidate.has_value() &&
          (*first_candidate)[0] < (*first_candidate)[1] &&
          cursor.audit().schedule_advance_count == 1U,
      "the pending range did not resume to its first oriented candidate");

  ExactMortonGroupedAnchoredPairCandidateContext moved{std::move(cursor)};
  require(
      !cursor.ready() && moved.ready(),
      "moving an oriented candidate cursor did not revoke its source");
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(cursor.advance(
            index,
            cloud,
            ExactMortonGroupedAnchoredPairCandidateWorkBudget{1U, 1U, 1U, 1U}));
      },
      "a moved-from oriented candidate cursor remained usable");

  CanonicalPointCloud other_cloud = make_line_cloud(8U);
  MortonLbvhIndex other_index = MortonLbvhIndex::build(other_cloud);
  const std::size_t calls_before = moved.audit().advance_call_count;
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(moved.advance(
            other_index,
            other_cloud,
            ExactMortonGroupedAnchoredPairCandidateWorkBudget{1U, 1U, 1U, 1U}));
      },
      "an oriented candidate cursor accepted a foreign authority");
  require(
      moved.audit().advance_call_count == calls_before,
      "foreign cursor rejection mutated the oriented audit");
}

void test_morton_partition_fresh_p8g_and_segmented_identity() {
  CanonicalPointCloud cloud = make_line_cloud(20U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactMortonGroupedAnchoredPairScheduleConfig config{4U, 12U};
  const ScheduleRun roomy = run_schedule(
      index,
      cloud,
      4U,
      config,
      ExactGroupedAnchoredPairTraversalWorkBudget{4096U, 4096U});
  const ScheduleRun segmented = run_schedule(
      index,
      cloud,
      4U,
      config,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U});

  validate_group_partition(index, roomy, config);
  validate_group_partition(index, segmented, config);
  validate_common_frontier_recursive_anchor_partitions(
      index, roomy, config);
  validate_common_frontier_recursive_anchor_partitions(
      index, segmented, config);
  require(
      roomy.terminals == segmented.terminals &&
          roomy.common_frontiers == segmented.common_frontiers &&
          roomy.anchor_subgroup_splits == segmented.anchor_subgroup_splits &&
          roomy.groups == segmented.groups &&
          roomy.candidates == segmented.candidates,
      "budget segmentation changed the grouped scientific stream");
  require(
      roomy.audit.certified_prune_count > 0U &&
          roomy.audit.budget_exhaustion_count == 0U &&
          segmented.audit.budget_exhaustion_count > 0U &&
          roomy.audit.traversal_node_visit_count ==
              segmented.audit.traversal_node_visit_count &&
          roomy.audit.witness_slot_scan_count ==
              segmented.audit.witness_slot_scan_count &&
          roomy.audit.inherited_witness_reuse_count ==
              segmented.audit.inherited_witness_reuse_count &&
          roomy.audit.exact_predicate_count ==
              segmented.audit.exact_predicate_count &&
          roomy.audit.diagonal_node_descent_count ==
              segmented.audit.diagonal_node_descent_count &&
          roomy.audit.common_frontier_count ==
              segmented.audit.common_frontier_count &&
          roomy.audit.delegated_frontier_anchor_count ==
              segmented.audit.delegated_frontier_anchor_count &&
          roomy.audit.prepared_anchor_subgroup_probe_count ==
              segmented.audit.prepared_anchor_subgroup_probe_count &&
          roomy.audit.proposed_anchor_subgroup_witness_pool_entry_count ==
              segmented.audit
                  .proposed_anchor_subgroup_witness_pool_entry_count &&
          roomy.audit.anchor_subgroup_split_count ==
              segmented.audit.anchor_subgroup_split_count &&
          roomy.audit.anchor_subgroup_certified_prune_count ==
              segmented.audit.anchor_subgroup_certified_prune_count &&
          roomy.audit.anchor_subgroup_certified_anchor_count ==
              segmented.audit.anchor_subgroup_certified_anchor_count &&
          roomy.audit.prepared_singleton_fallback_count ==
              segmented.audit.prepared_singleton_fallback_count &&
          roomy.audit.completed_singleton_fallback_count ==
              segmented.audit.completed_singleton_fallback_count &&
          roomy.audit.proposed_singleton_witness_pool_entry_count ==
              segmented.audit.proposed_singleton_witness_pool_entry_count &&
          roomy.audit.singleton_certified_prune_count ==
              segmented.audit.singleton_certified_prune_count &&
          roomy.audit.maximum_pending_anchor_subgroup_count ==
              segmented.audit.maximum_pending_anchor_subgroup_count,
      "segmentation changed P8h work or failed to exercise fresh P8g prunes");
  const std::set<CandidatePair> anchored_candidates =
      anchored_path_candidates(index, cloud, 4U);
  require(
      classify_candidates(index, cloud, roomy.candidates, 4U) ==
          classify_candidates(index, cloud, anchored_candidates, 4U),
      "the recursive subgroup schedule changed the final exact anchored oracle");
}

void test_exact_anchored_candidate_path_identity_and_fallback() {
  CanonicalPointCloud cloud = make_three_dimensional_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  bool morton_differs_from_point_id_order = false;
  for (std::size_t leaf = 0U; leaf < index.leaves().size(); ++leaf) {
    if (index.leaves()[leaf].point_id != static_cast<PointId>(leaf)) {
      morton_differs_from_point_id_order = true;
      break;
    }
  }
  require(
      morton_differs_from_point_id_order,
      "the three-dimensional fixture lost its Morton/PointId distinction");

  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactMortonGroupedAnchoredPairScheduleConfig grouped_config{5U, 16U};
  const ScheduleRun grouped = run_schedule(
      index,
      cloud,
      maximum_closed_rank,
      grouped_config,
      ExactGroupedAnchoredPairTraversalWorkBudget{4096U, 4096U});
  validate_group_partition(index, grouped, grouped_config);
  validate_common_frontier_recursive_anchor_partitions(
      index, grouped, grouped_config);
  require(
      !grouped.groups.empty() &&
          grouped.groups.back().anchor_point_ids.size() == 4U &&
          grouped.audit.prepared_anchor_subgroup_probe_count > 0U &&
          grouped.audit.anchor_subgroup_split_count > 0U &&
          grouped.audit.prepared_singleton_fallback_count > 0U,
      "the scheduler did not exercise its short group, subgroup split, and singleton fallback");

  const std::set<CandidatePair> anchored_candidates =
      anchored_path_candidates(index, cloud, maximum_closed_rank);
  const std::vector<ScientificRecord> grouped_science = classify_candidates(
      index, cloud, grouped.candidates, maximum_closed_rank);
  const std::vector<ScientificRecord> anchored_science = classify_candidates(
      index, cloud, anchored_candidates, maximum_closed_rank);
  require(
      grouped_science == anchored_science,
      "the grouped schedule and exact anchored path changed the scientific pair output");

  const ExactMortonGroupedAnchoredPairScheduleConfig fallback_config{5U, 2U};
  const ScheduleRun fallback = run_schedule(
      index,
      cloud,
      maximum_closed_rank,
      fallback_config,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  validate_group_partition(index, fallback, fallback_config);
  const std::size_t all_pair_count =
      cloud.size() * (cloud.size() - 1U) / 2U;
  require(
      fallback.audit.fallback_subtree_count == fallback.groups.size() &&
          fallback.audit.certified_prune_count == 0U &&
          fallback.candidates.size() == all_pair_count &&
          classify_candidates(
              index,
              cloud,
              fallback.candidates,
              maximum_closed_rank) == anchored_science,
      "an undersized group pool failed to open to the exact candidate path");
}

void test_schedule_validation_move_and_foreign_authority() {
  CanonicalPointCloud singleton_cloud = make_line_cloud(1U);
  MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton_cloud);
  const ExactMortonGroupedAnchoredPairScheduleConfig maximum_config{32U, 64U};
  const ScheduleRun singleton = run_schedule(
      singleton_index,
      singleton_cloud,
      11U,
      maximum_config,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  validate_group_partition(singleton_index, singleton, maximum_config);
  require(
      singleton.groups.size() == 1U &&
          singleton.groups.front().anchor_point_ids.size() == 1U &&
          singleton.groups.front().witness_pool_point_ids.empty() &&
          singleton.audit.fallback_subtree_count == 1U &&
          singleton.candidates.empty(),
      "the singleton schedule lost its maximum-capacity fallback contract");

  CanonicalPointCloud cloud = make_line_cloud(8U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(
            ExactMortonGroupedAnchoredPairScheduleContext::start(
                index, cloud, 4U, {0U, 4U}));
      },
      "the scheduler accepted an empty group capacity");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(
            ExactMortonGroupedAnchoredPairScheduleContext::start(
                index, cloud, 4U, {33U, 4U}));
      },
      "the scheduler accepted an oversized anchor group");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(
            ExactMortonGroupedAnchoredPairScheduleContext::start(
                index, cloud, 4U, {4U, 65U}));
      },
      "the scheduler accepted an oversized witness pool");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(
            ExactMortonGroupedAnchoredPairScheduleContext::start(
                index, cloud, 1U, {4U, 4U}));
      },
      "the scheduler accepted a rank below two");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(
            ExactMortonGroupedAnchoredPairScheduleContext::start(
                index, cloud, 12U, {4U, 4U}));
      },
      "the scheduler accepted a rank above eleven");

  ExactMortonGroupedAnchoredPairScheduleContext original =
      ExactMortonGroupedAnchoredPairScheduleContext::start(
          index, cloud, 4U, {4U, 4U});
  ExactMortonGroupedAnchoredPairScheduleContext moved{std::move(original)};
  require(
      !original.ready() && moved.ready(),
      "moving a schedule did not revoke its source");
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(original.advance(
            index,
            cloud,
            ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U}));
      },
      "a moved-from schedule remained usable");

  CanonicalPointCloud other_cloud = make_line_cloud(8U);
  MortonLbvhIndex other_index = MortonLbvhIndex::build(other_cloud);
  const std::size_t calls_before = moved.audit().advance_call_count;
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(moved.advance(
            other_index,
            other_cloud,
            ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U}));
      },
      "a schedule accepted a geometrically equal foreign authority");
  require(
      moved.audit().advance_call_count == calls_before,
      "foreign authority rejection mutated the schedule audit");
}

}  // namespace

int main() {
  static_assert(
      !std::is_default_constructible_v<
          ExactMortonGroupedAnchoredPairScheduleContext>);
  static_assert(
      !std::is_copy_constructible_v<
          ExactMortonGroupedAnchoredPairScheduleContext>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactMortonGroupedAnchoredPairScheduleContext>);
  static_assert(
      !std::is_move_assignable_v<
          ExactMortonGroupedAnchoredPairScheduleContext>);
  static_assert(
      !std::is_default_constructible_v<
          ExactMortonGroupedAnchoredPairScheduleStep>);
  static_assert(
      !std::is_default_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateContext>);
  static_assert(
      !std::is_copy_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateContext>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateContext>);
  static_assert(
      !std::is_move_assignable_v<
          ExactMortonGroupedAnchoredPairCandidateContext>);
  static_assert(
      !std::is_default_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateStep>);
  static_assert(
      !std::is_copy_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateStep>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactMortonGroupedAnchoredPairCandidateStep>);

  try {
    test_morton_partition_fresh_p8g_and_segmented_identity();
    test_exact_anchored_candidate_path_identity_and_fallback();
    test_schedule_validation_move_and_foreign_authority();
    test_oriented_candidate_cursor_identity_and_budgets();
    test_oriented_candidate_cursor_atomic_local_budgets();
  } catch (const std::exception& error) {
    std::cerr << "Morton grouped schedule test failure: "
              << error.what() << '\n';
    return 1;
  }
  std::cout << "Morton grouped anchored-pair schedule checks passed\n";
  return 0;
}
