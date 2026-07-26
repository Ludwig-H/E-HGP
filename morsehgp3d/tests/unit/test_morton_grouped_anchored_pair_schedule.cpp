#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_schedule.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
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
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationStatus;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneBudget;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalStepKind;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalWorkBudget;
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
  std::size_t group_ordinal{};
  std::size_t anchor_leaf_begin{};
  std::size_t anchor_leaf_end{};
  std::optional<std::size_t> node_index;
  std::optional<std::size_t> leaf_begin;
  std::optional<std::size_t> leaf_end;
  std::optional<PointId> unresolved_point_id;
  std::vector<PointId> certified_witness_point_ids;

  friend bool operator==(const TerminalRecord&, const TerminalRecord&) =
      default;
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
  std::vector<GroupRecord> groups;
  std::set<CandidatePair> candidates;
  ExactMortonGroupedAnchoredPairScheduleAudit audit;
};

[[nodiscard]] std::vector<PointId> copied_ids(
    std::span<const PointId> ids) {
  return {ids.begin(), ids.end()};
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
    require(
        step.kind() !=
            ExactMortonGroupedAnchoredPairScheduleStepKind::complete,
        "the schedule emitted completion before closing its final group");
    require(
        step.group_ordinal().has_value() &&
            step.anchor_leaf_begin().has_value() &&
            step.anchor_leaf_end().has_value(),
        "a schedule terminal lost its group provenance");
    const auto* traversal_step = step.traversal_step();
    require(
        traversal_step != nullptr,
        "a schedule terminal lost its exact P8h record");

    TerminalRecord terminal{
        step.kind(),
        *step.group_ordinal(),
        *step.anchor_leaf_begin(),
        *step.anchor_leaf_end(),
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
  require(
      roomy.terminals == segmented.terminals &&
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
              segmented.audit.exact_predicate_count,
      "segmentation changed P8h work or failed to exercise fresh P8g prunes");
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
  require(
      !grouped.groups.empty() &&
          grouped.groups.back().anchor_point_ids.size() == 4U,
      "the scheduler did not exercise its short final group");

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

  try {
    test_morton_partition_fresh_p8g_and_segmented_identity();
    test_exact_anchored_candidate_path_identity_and_fallback();
    test_schedule_validation_move_and_foreign_authority();
  } catch (const std::exception& error) {
    std::cerr << "Morton grouped schedule test failure: "
              << error.what() << '\n';
    return 1;
  }
  std::cout << "Morton grouped anchored-pair schedule checks passed\n";
  return 0;
}
