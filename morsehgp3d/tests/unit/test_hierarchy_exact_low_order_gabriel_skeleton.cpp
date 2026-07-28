#include "morsehgp3d/hierarchy/exact_low_order_gabriel_skeleton.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactHigherSupportEvent;
using morsehgp3d::hierarchy::ExactHigherSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactHigherSupportStopReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamResult;
using morsehgp3d::hierarchy::ExactHigherSupportStreamStatus;
using morsehgp3d::hierarchy::ExactLowOrderGabrielSkeletonDecision;
using morsehgp3d::hierarchy::ExactLowOrderGabrielSkeletonResult;
using morsehgp3d::hierarchy::ExactLowOrderGabrielEventSource;
using morsehgp3d::hierarchy::ExactLowOrderGabrielEventSourceReceipt;
using morsehgp3d::hierarchy::ExactLowOrderGabrielEdgeRecord;
using morsehgp3d::hierarchy::ExactLowOrderGabrielTriangleProvenance;
using morsehgp3d::hierarchy::ExactLowOrderGabrielTriangleRecord;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportStreamResult;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::hierarchy::make_exact_low_order_gabriel_higher_source_receipt;
using morsehgp3d::hierarchy::make_exact_low_order_gabriel_pair_source_receipt;
using morsehgp3d::hierarchy::project_exact_low_order_gabriel_skeleton_from_events;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] ExactPairSupportEvent pair_event(
    PointId first,
    PointId second,
    std::vector<PointId> interior_ids,
    std::size_t point_count,
    ExactLevel squared_level = ExactLevel{1}) {
  const std::size_t closed_rank = 2U + interior_ids.size();
  return ExactPairSupportEvent{
      {first, second},
      {},
      std::move(squared_level),
      std::move(interior_ids),
      closed_rank,
      point_count - closed_rank};
}

[[nodiscard]] ExactHigherSupportEvent higher_event(
    std::array<PointId, 4U> support_ids,
    std::uint8_t support_size,
    std::vector<PointId> interior_ids,
    std::size_t point_count,
    ExactLevel squared_level = ExactLevel{1}) {
  const std::size_t closed_rank =
      static_cast<std::size_t>(support_size) + interior_ids.size();
  return ExactHigherSupportEvent{
      support_size,
      support_ids,
      {},
      std::move(squared_level),
      std::move(interior_ids),
      closed_rank,
      point_count - closed_rank};
}

[[nodiscard]] ExactPairSupportStreamResult complete_pair_source(
    std::size_t point_count = 8U,
    std::size_t requested_maximum_order = 2U) {
  ExactPairSupportStreamResult source;
  const std::size_t effective_maximum_order =
      std::min(point_count, requested_maximum_order);
  const std::size_t maximum_relevant_closed_rank =
      std::min(point_count, effective_maximum_order + 1U);
  source.requirements = {
      point_count,
      requested_maximum_order,
      effective_maximum_order,
      maximum_relevant_closed_rank};
  source.status = ExactPairSupportStreamStatus::complete;
  source.stop_reason = ExactPairSupportStopReason::none;
  source.self_product_partition_certified = true;
  source.witness_antichains_certified = true;
  source.all_rank_prunes_recertified = true;
  source.all_rank_relevant_shells_complete = true;
  source.frontier_exhausted = true;
  source.no_forbidden_global_structure_materialized = true;
  source.hierarchy_reduction_performed = false;
  source.audit.total_pair_count = 28U;
  source.audit.resolved_pair_count = 28U;
  source.audit.remaining_frontier_pair_count = 0U;
  source.audit.pair_partition_accounting_certified = true;
  return source;
}

[[nodiscard]] ExactHigherSupportStreamResult complete_higher_source(
    std::size_t point_count = 8U,
    std::size_t requested_maximum_order = 2U) {
  ExactHigherSupportStreamResult source;
  const std::size_t effective_maximum_order =
      std::min(point_count, requested_maximum_order);
  const std::size_t maximum_relevant_closed_rank =
      std::min(point_count, effective_maximum_order + 1U);
  source.requirements = {
      point_count,
      requested_maximum_order,
      effective_maximum_order,
      maximum_relevant_closed_rank};
  source.status = ExactHigherSupportStreamStatus::complete;
  source.stop_reason = ExactHigherSupportStopReason::none;
  source.grouped_frontier_partition_certified = true;
  source.all_prunes_replayable = true;
  source.all_rank_relevant_shells_complete = true;
  source.frontier_exhausted = true;
  source.no_forbidden_global_structure_materialized = true;
  source.hierarchy_reduction_performed = false;
  source.audit.total_support_count = BigInt{126U};
  source.audit.resolved_support_count = BigInt{126U};
  source.audit.remaining_frontier_support_count = BigInt{0U};
  source.audit.exact_bigint_universe_certified = true;
  source.audit.grouped_partition_accounting_certified = true;
  return source;
}

void synchronize_counts(ExactPairSupportStreamResult& source) {
  source.audit.accepted_event_count = source.events.size();
  source.audit.relevant_extra_shell_diagnostic_count =
      source.relevant_extra_shell_diagnostics.size();
}

void synchronize_counts(ExactHigherSupportStreamResult& source) {
  source.audit.accepted_event_count = source.events.size();
  source.audit.relevant_extra_shell_diagnostic_count =
      source.relevant_extra_shell_diagnostics.size();
}

[[nodiscard]] ExactLowOrderGabrielSkeletonResult project(
    const ExactPairSupportStreamResult& pair_source,
    const ExactHigherSupportStreamResult& higher_source) {
  return project_exact_low_order_gabriel_skeleton_from_events(
      pair_source,
      make_exact_low_order_gabriel_pair_source_receipt(pair_source),
      higher_source,
      make_exact_low_order_gabriel_higher_source_receipt(higher_source));
}

[[nodiscard]] bool empty_skeleton(
    const ExactLowOrderGabrielSkeletonResult& result) {
  return result.rank_two_edges.empty() &&
         result.rank_three_triangles.empty();
}

void test_pair_support_projection() {
  ExactPairSupportStreamResult pair_source = complete_pair_source(8U, 3U);
  ExactHigherSupportStreamResult higher_source =
      complete_higher_source(8U, 3U);
  pair_source.events.push_back(
      pair_event(1U, 4U, {3U}, 8U, ExactLevel{5}));
  pair_source.events.push_back(
      pair_event(0U, 2U, {}, 8U, ExactLevel{2}));
  pair_source.events.push_back(
      pair_event(5U, 7U, {1U, 6U}, 8U, ExactLevel{9}));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(result.locally_projected(),
          "a complete pair-supported low-order skeleton was rejected");
  require(
      result.rank_two_edges ==
          std::vector<ExactLowOrderGabrielEdgeRecord>{
              {{0U, 2U}, ExactLevel{2}}},
      "the regular rank-two Gabriel edge or level was not extracted");
  require(
      result.rank_three_triangles ==
          std::vector<ExactLowOrderGabrielTriangleRecord>{
              {{1U, 3U, 4U},
               ExactLevel{5},
               ExactLowOrderGabrielTriangleProvenance::pair}},
      "the pair-supported Gabriel triangle, level or provenance is wrong");
  require(
      result.audit.rank_two_edge_occurrence_count == 1U &&
          result.audit.pair_triangle_occurrence_count == 1U &&
          result.audit.higher_triangle_occurrence_count == 0U &&
          !result.audit.source_authority_replay_performed,
      "the pair-supported projection audit is inconsistent");
}

void test_higher_support_projection() {
  ExactPairSupportStreamResult pair_source = complete_pair_source(8U, 3U);
  ExactHigherSupportStreamResult higher_source =
      complete_higher_source(8U, 3U);
  higher_source.events.push_back(
      higher_event(
          {2U, 5U, 6U, 0U}, 3U, {}, 8U, ExactLevel{7}));
  higher_source.events.push_back(
      higher_event({0U, 1U, 2U, 3U}, 4U, {}, 8U));
  higher_source.events.push_back(
      higher_event({1U, 4U, 7U, 0U}, 3U, {2U}, 8U));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(result.locally_projected(),
          "a complete support-three k=2 projection was rejected");
  require(result.rank_two_edges.empty(),
          "a higher-only source invented a rank-two edge");
  require(
      result.rank_three_triangles ==
          std::vector<ExactLowOrderGabrielTriangleRecord>{
              {{2U, 5U, 6U},
               ExactLevel{7},
               ExactLowOrderGabrielTriangleProvenance::higher}},
      "the support-three Gabriel triangle, level or provenance is wrong");
  require(
      result.audit.pair_triangle_occurrence_count == 0U &&
          result.audit.higher_triangle_occurrence_count == 1U,
      "the higher-supported projection audit is inconsistent");
}

void test_sort_and_deduplicate() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  pair_source.events.push_back(
      pair_event(1U, 4U, {3U}, 8U, ExactLevel{3}));
  pair_source.events.push_back(
      pair_event(1U, 4U, {3U}, 8U, ExactLevel{3}));
  pair_source.events.push_back(
      pair_event(0U, 7U, {2U}, 8U, ExactLevel{1}));
  pair_source.events.push_back(
      pair_event(5U, 7U, {}, 8U, ExactLevel{2}));
  pair_source.events.push_back(
      pair_event(5U, 7U, {}, 8U, ExactLevel{2}));
  higher_source.events.push_back(
      higher_event(
          {1U, 3U, 4U, 0U}, 3U, {}, 8U, ExactLevel{3}));
  higher_source.events.push_back(
      higher_event(
          {0U, 5U, 6U, 0U}, 3U, {}, 8U, ExactLevel{2}));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(result.locally_projected(),
          "duplicate source occurrences invalidated the local catalogue");
  require(
      result.rank_two_edges ==
          std::vector<ExactLowOrderGabrielEdgeRecord>{
              {{5U, 7U}, ExactLevel{2}}},
      "the rank-two Gabriel graph is not sorted and unique");
  require(
      result.rank_three_triangles ==
          std::vector<ExactLowOrderGabrielTriangleRecord>{
              {{0U, 2U, 7U},
               ExactLevel{1},
               ExactLowOrderGabrielTriangleProvenance::pair},
              {{0U, 5U, 6U},
               ExactLevel{2},
               ExactLowOrderGabrielTriangleProvenance::higher},
              {{1U, 3U, 4U},
               ExactLevel{3},
               ExactLowOrderGabrielTriangleProvenance::pair_and_higher}},
      "the merged triangle catalogue is not level-sorted and unique");
  require(
      result.audit.rank_two_edge_occurrence_count == 2U &&
          result.audit.duplicate_rank_two_edge_occurrence_count == 1U &&
          result.audit.pair_triangle_occurrence_count == 3U &&
          result.audit.higher_triangle_occurrence_count == 2U &&
          result.audit.duplicate_triangle_occurrence_count == 2U,
      "duplicate occurrence accounting is incorrect");
}

void test_strict_k1_projection() {
  ExactPairSupportStreamResult pair_source = complete_pair_source(8U, 1U);
  ExactHigherSupportStreamResult higher_source =
      complete_higher_source(8U, 1U);
  pair_source.events.push_back(
      pair_event(2U, 6U, {}, 8U, ExactLevel{4}));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.locally_projected() &&
          result.rank_two_edges ==
              std::vector<ExactLowOrderGabrielEdgeRecord>{
                  {{2U, 6U}, ExactLevel{4}}} &&
          result.rank_three_triangles.empty(),
      "a strict Kmax=1 source did not produce its regular rank-two graph");
}

void test_contradictory_levels_fail_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  pair_source.events.push_back(
      pair_event(0U, 5U, {}, 8U, ExactLevel{1}));
  pair_source.events.push_back(
      pair_event(0U, 5U, {}, 8U, ExactLevel{2}));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             contradictory_rank_two_edge_levels &&
          result.audit.contradictory_rank_two_edge_level_count == 1U &&
          empty_skeleton(result),
      "contradictory exact levels for one edge did not fail closed");

  pair_source.events.clear();
  pair_source.events.push_back(
      pair_event(1U, 4U, {3U}, 8U, ExactLevel{3}));
  higher_source.events.push_back(
      higher_event(
          {1U, 3U, 4U, 0U}, 3U, {}, 8U, ExactLevel{4}));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);
  result = project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             contradictory_rank_three_triangle_levels &&
          result.audit.contradictory_rank_three_triangle_level_count == 1U &&
          empty_skeleton(result),
      "contradictory exact levels across triangle lanes did not fail closed");
}

void test_event_above_declared_rank_fails_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source(8U, 1U);
  ExactHigherSupportStreamResult higher_source =
      complete_higher_source(8U, 1U);
  pair_source.events.push_back(pair_event(1U, 4U, {3U}, 8U));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.decision ==
              ExactLowOrderGabrielSkeletonDecision::invalid_pair_event &&
          empty_skeleton(result),
      "an event above the source's declared rank escaped validation");
}

void test_incomplete_sources_fail_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  pair_source.frontier_exhausted = false;
  ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             pair_source_not_complete &&
          empty_skeleton(result) && !result.locally_projected(),
      "a residual pair frontier did not fail closed");

  pair_source.frontier_exhausted = true;
  higher_source.status = ExactHigherSupportStreamStatus::budget_exhausted;
  result = project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             higher_source_not_complete &&
          empty_skeleton(result),
      "an incomplete higher stream did not fail closed");
}

void test_extra_shell_diagnostics_fail_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  pair_source.relevant_extra_shell_diagnostics.push_back(
      ExactPairSupportExtraShellDiagnostic{
          {0U, 1U}, {}, ExactLevel{1}, {}, 3U, 2U, 2U, 3U, 5U});
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             relevant_extra_shell_diagnostics_present &&
          empty_skeleton(result),
      "an unresolved pair extra-shell diagnostic did not fail closed");

  pair_source.relevant_extra_shell_diagnostics.clear();
  higher_source.relevant_extra_shell_diagnostics.push_back(
      ExactHigherSupportExtraShellDiagnostic{
          3U,
          {0U, 1U, 2U, 0U},
          {},
          ExactLevel{1},
          {},
          4U,
          3U,
          3U,
          4U,
          4U});
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);
  result = project(pair_source, higher_source);
  require(
      result.decision == ExactLowOrderGabrielSkeletonDecision::
                             relevant_extra_shell_diagnostics_present &&
          empty_skeleton(result),
      "a relevant higher extra-shell diagnostic did not fail closed");
}

void test_receipt_and_payload_tamper_fail_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  pair_source.events.push_back(pair_event(1U, 4U, {3U}, 8U));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);
  ExactLowOrderGabrielEventSourceReceipt pair_receipt =
      make_exact_low_order_gabriel_pair_source_receipt(pair_source);
  const ExactLowOrderGabrielEventSourceReceipt higher_receipt =
      make_exact_low_order_gabriel_higher_source_receipt(higher_source);

  pair_source.events[0].squared_level = ExactLevel{2};
  ExactLowOrderGabrielSkeletonResult result =
      project_exact_low_order_gabriel_skeleton_from_events(
          pair_source, pair_receipt, higher_source, higher_receipt);
  require(
      result.decision ==
              ExactLowOrderGabrielSkeletonDecision::receipt_mismatch &&
          empty_skeleton(result),
      "an exact-level mutation escaped the receipt binding");

  pair_source.events[0].squared_level = ExactLevel{1};
  pair_source.events[0].interior_ids[0] = 5U;
  result =
      project_exact_low_order_gabriel_skeleton_from_events(
          pair_source, pair_receipt, higher_source, higher_receipt);
  require(
      result.decision ==
              ExactLowOrderGabrielSkeletonDecision::receipt_mismatch &&
          empty_skeleton(result),
      "a projection-relevant event mutation escaped the receipt binding");

  pair_source.events[0].interior_ids[0] = 3U;
  pair_receipt.source = ExactLowOrderGabrielEventSource::higher;
  result = project_exact_low_order_gabriel_skeleton_from_events(
      pair_source, pair_receipt, higher_source, higher_receipt);
  require(
      result.decision ==
              ExactLowOrderGabrielSkeletonDecision::receipt_mismatch &&
          empty_skeleton(result),
      "a source-kind receipt mutation escaped fail-closed validation");
}

void test_malformed_rank_three_event_fails_closed() {
  ExactPairSupportStreamResult pair_source = complete_pair_source();
  ExactHigherSupportStreamResult higher_source = complete_higher_source();
  pair_source.events.push_back(pair_event(1U, 1U, {3U}, 8U));
  synchronize_counts(pair_source);
  synchronize_counts(higher_source);

  const ExactLowOrderGabrielSkeletonResult result =
      project(pair_source, higher_source);
  require(
      result.decision ==
              ExactLowOrderGabrielSkeletonDecision::invalid_pair_event &&
          empty_skeleton(result),
      "a malformed rank-three pair event was silently canonicalized");
}

}  // namespace

int main() {
  try {
    test_pair_support_projection();
    test_higher_support_projection();
    test_sort_and_deduplicate();
    test_strict_k1_projection();
    test_contradictory_levels_fail_closed();
    test_event_above_declared_rank_fails_closed();
    test_incomplete_sources_fail_closed();
    test_extra_shell_diagnostics_fail_closed();
    test_receipt_and_payload_tamper_fail_closed();
    test_malformed_rank_three_event_fails_closed();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "all exact low-order Gabriel skeleton tests passed\n";
  return 0;
}
