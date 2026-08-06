#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactHigherSupportPruneReason;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredSession;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredStreamAssembler;
using morsehgp3d::hierarchy::ExactHigherSupportTileCertifiedTile;
using morsehgp3d::hierarchy::ExactHigherSupportVerificationBasis;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportCheckpoint;
using morsehgp3d::hierarchy::ExactHigherSupportPendingStage;
using morsehgp3d::hierarchy::ExactHigherSupportStreamChunk;
using morsehgp3d::hierarchy::ExactHigherSupportStopReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSegment;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::hierarchy::ExactHigherSupportUnsealedDrainStatus;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::compute_exact_higher_support_checkpoint_digest;
using morsehgp3d::hierarchy::exact_higher_support_candidate_universe_size;
using morsehgp3d::hierarchy::make_initial_exact_higher_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_higher_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream_run;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    std::initializer_list<CertifiedPoint3> points) {
  const std::vector<CertifiedPoint3> storage{points};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{storage});
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

struct SupportKey {
  std::uint8_t support_size{};
  std::array<PointId, 4> support_ids{};

  friend bool operator==(const SupportKey&, const SupportKey&) = default;
  friend bool operator<(const SupportKey& left, const SupportKey& right) {
    if (left.support_size != right.support_size) {
      return left.support_size < right.support_size;
    }
    return left.support_ids < right.support_ids;
  }
};

struct ExhaustiveHigherDecision {
  std::vector<SupportKey> events;
  std::vector<SupportKey> diagnostics;
};

template <std::size_t SupportSize>
void classify_exhaustive_support(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, SupportSize>& support_ids,
    std::size_t maximum_rank,
    ExhaustiveHigherDecision& decision) {
  if (SupportSize > maximum_rank) {
    return;
  }
  std::array<morsehgp3d::exact::ExactRational3, SupportSize> points{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    points[index] = cloud.point(support_ids[index]).exact();
  }
  const auto analysis =
      morsehgp3d::exact::analyze_circumcenter_support(points);
  if (analysis.status() !=
      morsehgp3d::exact::CircumcenterSupportStatus::minimal) {
    return;
  }
  const auto& sphere = analysis.circumcenter_result();
  if (!sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "an exhaustive minimal support omitted its exact sphere");
  }
  std::size_t interior_count = 0U;
  std::size_t shell_count = 0U;
  for (PointId point_id = 0U; point_id < cloud.size(); ++point_id) {
    const auto classification = morsehgp3d::exact::classify_sphere_point(
        *sphere.center(), *sphere.squared_level(), cloud.point(point_id));
    if (classification.location() ==
        morsehgp3d::exact::SpherePointLocation::strictly_inside) {
      ++interior_count;
    } else if (classification.location() ==
               morsehgp3d::exact::SpherePointLocation::boundary) {
      ++shell_count;
    }
  }
  if (interior_count > maximum_rank - SupportSize) {
    return;
  }
  SupportKey key;
  key.support_size = static_cast<std::uint8_t>(SupportSize);
  std::copy(support_ids.begin(), support_ids.end(), key.support_ids.begin());
  if (shell_count == SupportSize) {
    decision.events.push_back(key);
  } else {
    decision.diagnostics.push_back(key);
  }
}

[[nodiscard]] ExhaustiveHigherDecision exhaustive_higher_decision(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_rank) {
  ExhaustiveHigherDecision decision;
  for (PointId first = 0U; first < cloud.size(); ++first) {
    for (PointId second = first + 1U; second < cloud.size(); ++second) {
      for (PointId third = second + 1U; third < cloud.size(); ++third) {
        classify_exhaustive_support<3U>(
            cloud, {first, second, third}, maximum_rank, decision);
        for (PointId fourth = third + 1U; fourth < cloud.size(); ++fourth) {
          classify_exhaustive_support<4U>(
              cloud,
              {first, second, third, fourth},
              maximum_rank,
              decision);
        }
      }
    }
  }
  std::sort(decision.events.begin(), decision.events.end());
  std::sort(decision.diagnostics.begin(), decision.diagnostics.end());
  return decision;
}

void test_bigint_universe() {
  check(
      exact_higher_support_candidate_universe_size(0U) == 0 &&
          exact_higher_support_candidate_universe_size(3U) == 1 &&
          exact_higher_support_candidate_universe_size(4U) == 5,
      "small higher-support universes equal C(n,3)+C(n,4)");
  const BigInt ten_million_expected{"416666583333329166667500000"};
  check(
      exact_higher_support_candidate_universe_size(10'000'000U) ==
              ten_million_expected &&
          ten_million_expected >
              BigInt{std::numeric_limits<std::uint64_t>::max()},
      "the 10M support universe is exact beyond 64 bits");
}

void test_regular_tetrahedron_complete_and_fresh_replay() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 10U, budget);
  const std::size_t triangle_count = static_cast<std::size_t>(
      std::count_if(
          result.events.begin(),
          result.events.end(),
          [](const auto& event) { return event.support_size == 3U; }));
  const std::size_t tetrahedron_count = static_cast<std::size_t>(
      std::count_if(
          result.events.begin(),
          result.events.end(),
          [](const auto& event) { return event.support_size == 4U; }));
  check(
      result.stream_complete() &&
          result.absence_of_additional_higher_supports_certified() &&
          result.audit.total_support_count == 5 &&
          result.audit.leaf_classified_support_count == 5 &&
          result.audit.resolved_support_count == 5 &&
          result.prune_certificates.empty() &&
          result.relevant_extra_shell_diagnostics.empty() &&
          triangle_count == 4U && tetrahedron_count == 1U,
      "the regular tetrahedron closes four triangles and one tetrahedron without a cell atlas");
  const auto verification = verify_exact_higher_support_stream(
      index, cloud, 10U, budget, result);
  check(
      verification.result_certified &&
          verification.prune_certificates_replayed &&
          verification.grouped_frontier_replayed &&
          verification.fresh_replay_certified,
      "a fresh authority replay certifies the complete regular-tetrahedron stream");

  auto mutated = result;
  mutated.audit.total_support_count += 1;
  check(
      !verify_exact_higher_support_stream(
           index, cloud, 10U, budget, mutated)
           .result_certified,
      "a mutated BigInt universe fails fresh verification");
}

void test_intrinsically_above_rank_and_budgeted_frontier() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto rank_two =
      build_exact_higher_support_stream(index, cloud, 1U, budget);
  check(
      rank_two.stream_complete() && rank_two.events.empty() &&
          rank_two.audit.total_support_count == 5 &&
          rank_two.audit.rank_pruned_support_count == 5 &&
          std::all_of(
              rank_two.prune_certificates.begin(),
              rank_two.prune_certificates.end(),
              [](const auto& certificate) {
                return certificate.reason ==
                           ExactHigherSupportPruneReason::
                               strict_interior_rank_bound &&
                       certificate.required_strict_interior_point_count ==
                           0U;
              }),
      "supports larger than s_max are exactly resolved without leaf geometry");

  ExactHigherSupportStreamBudget stopped_budget = unlimited_budget();
  stopped_budget.maximum_work_unit_count = 0U;
  const auto stopped = build_exact_higher_support_stream(
      index, cloud, 10U, stopped_budget);
  check(
      stopped.status ==
              ExactHigherSupportStreamStatus::budget_exhausted &&
          stopped.stop_reason ==
              ExactHigherSupportStopReason::work_unit_limit &&
          !stopped.stream_complete() &&
          !stopped.absence_of_additional_higher_supports_certified() &&
          stopped.remaining_frontier.size() == 2U &&
          stopped.audit.remaining_frontier_support_count == 5 &&
          stopped.audit.resolved_support_count == 0,
      "a zero-work run retains the exact triangle and tetrahedron frontier");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 10U, stopped_budget, stopped)
          .result_certified,
      "the budgeted residual frontier is freshly replayable");
}

void test_sparse_extra_shell_diagnostic() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, -1.0, -1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 10U, budget);
  const ExhaustiveHigherDecision exhaustive =
      exhaustive_higher_decision(
          cloud, result.requirements.maximum_relevant_closed_rank);
  std::vector<SupportKey> streamed_events;
  std::vector<SupportKey> streamed_diagnostics;
  for (const auto& event : result.events) {
    streamed_events.push_back(
        SupportKey{event.support_size, event.support_ids});
  }
  for (const auto& diagnostic :
       result.relevant_extra_shell_diagnostics) {
    streamed_diagnostics.push_back(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  std::sort(streamed_events.begin(), streamed_events.end());
  std::sort(streamed_diagnostics.begin(), streamed_diagnostics.end());
  const bool observed_five_point_shell = std::any_of(
      result.relevant_extra_shell_diagnostics.begin(),
      result.relevant_extra_shell_diagnostics.end(),
      [](const auto& diagnostic) {
        return diagnostic.support_size == 4U &&
               diagnostic.interior_ids.empty() &&
               diagnostic.shell_count == 5U &&
               diagnostic.minimum_possible_closed_rank == 4U &&
               diagnostic.observed_closed_rank == 5U &&
               diagnostic.exterior_count == 0U;
      });
  check(
      result.stream_complete() &&
          result.audit.total_support_count == 15 &&
          observed_five_point_shell &&
          streamed_events == exhaustive.events &&
          streamed_diagnostics == exhaustive.diagnostics,
      "the sparse five-site output agrees bidirectionally with exhaustive support enumeration");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 10U, budget, result)
          .result_certified,
      "the sparse extra-shell decision survives fresh replay");
}

void test_nonzero_universal_rank_receipts() {
  const std::vector<CertifiedPoint3> points{
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)};
  CanonicalPointCloud cloud = cloud_from(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 3U, budget);
  const ExhaustiveHigherDecision exhaustive = exhaustive_higher_decision(
      cloud, result.requirements.maximum_relevant_closed_rank);
  std::vector<SupportKey> streamed_events;
  std::vector<SupportKey> streamed_diagnostics;
  for (const auto& event : result.events) {
    streamed_events.push_back(
        SupportKey{event.support_size, event.support_ids});
  }
  for (const auto& diagnostic : result.relevant_extra_shell_diagnostics) {
    streamed_diagnostics.push_back(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  std::sort(streamed_events.begin(), streamed_events.end());
  std::sort(streamed_diagnostics.begin(), streamed_diagnostics.end());
  check(
      result.stream_complete() &&
          result.audit.total_support_count == 210 &&
          result.audit.rank_pruned_support_count == 44 &&
          result.audit.emitted_rank_receipt_count > 0U &&
          result.audit.above_rank_leaf_count == 0U &&
          result.audit.rank_query_outside_or_boundary_node_count > 0U &&
          result.audit.rank_query_outside_or_boundary_point_count >=
              result.audit.rank_query_outside_or_boundary_node_count &&
          result.audit.resolved_support_count == 210 &&
          result.audit.remaining_frontier_support_count == 0 &&
          streamed_events == exhaustive.events &&
          streamed_diagnostics == exhaustive.diagnostics,
      "two-sided exact query-cell decisions preserve the n=9 exhaustive result while skipping outside subtrees"
      " (rank_pruned=" +
          result.audit.rank_pruned_support_count.str() +
          ", receipts=" +
          std::to_string(result.audit.emitted_rank_receipt_count) +
          ", above_rank_leaves=" +
          std::to_string(result.audit.above_rank_leaf_count) +
          ", outside_nodes=" +
          std::to_string(
              result.audit.rank_query_outside_or_boundary_node_count) +
          ", outside_points=" +
          std::to_string(
              result.audit.rank_query_outside_or_boundary_point_count) +
          ", events=" + std::to_string(streamed_events.size()) + "/" +
          std::to_string(exhaustive.events.size()) +
          ", diagnostics=" +
          std::to_string(streamed_diagnostics.size()) + "/" +
          std::to_string(exhaustive.diagnostics.size()) +
          ")");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 3U, budget, result)
          .result_certified,
      "nonzero rank receipts survive independent fresh replay");

  ExactHigherSupportStreamBudget no_closed_ball_frontier =
      unlimited_budget();
  no_closed_ball_frontier.maximum_auxiliary_frontier_entry_count = 0U;
  const auto refused_closed_ball = build_exact_higher_support_stream(
      index, cloud, 3U, no_closed_ball_frontier);
  check(
      refused_closed_ball.status ==
              ExactHigherSupportStreamStatus::budget_exhausted &&
          refused_closed_ball.stop_reason ==
              ExactHigherSupportStopReason::auxiliary_frontier_entry_limit &&
          refused_closed_ball.audit.rank_witness_node_visit_count > 0U &&
          refused_closed_ball.audit.global_closed_ball_query_count == 0U &&
          refused_closed_ball.grouped_frontier_partition_certified &&
          refused_closed_ball.audit.resolved_support_count +
                  refused_closed_ball.audit.remaining_frontier_support_count ==
              refused_closed_ball.audit.total_support_count,
      "a zero auxiliary cap permits the cursor-only rank probe then fails closed before a terminal closed-ball DFS");
}

void test_n14_local_rank_probe_exhaustive_differential() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    const double x = static_cast<double>(index) - 7.0;
    const double y =
        static_cast<double>((index * index + 3U) % 17U) - 8.0;
    const double z =
        static_cast<double>((index * index * index + 5U) % 19U) - 9.0;
    points.push_back(point(x, y, z));
  }
  CanonicalPointCloud cloud = cloud_from(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 5U, budget);
  const ExhaustiveHigherDecision exhaustive = exhaustive_higher_decision(
      cloud, result.requirements.maximum_relevant_closed_rank);
  std::vector<SupportKey> streamed_events;
  std::vector<SupportKey> streamed_diagnostics;
  for (const auto& event : result.events) {
    streamed_events.push_back(
        SupportKey{event.support_size, event.support_ids});
  }
  for (const auto& diagnostic : result.relevant_extra_shell_diagnostics) {
    streamed_diagnostics.push_back(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  std::sort(streamed_events.begin(), streamed_events.end());
  std::sort(streamed_diagnostics.begin(), streamed_diagnostics.end());
  check(
      result.stream_complete() &&
          streamed_events == exhaustive.events &&
          streamed_diagnostics == exhaustive.diagnostics &&
          result.audit.rank_local_probe_attempt_count > 0U &&
          result.audit.rank_local_probe_geometric_gate_skip_count > 0U &&
          result.audit.rank_local_probe_candidate_evaluation_count <=
              result.audit.rank_local_probe_attempt_count *
                  morsehgp3d::hierarchy::
                      higher_support_local_rank_probe_maximum_evaluation_count &&
          result.audit.rank_local_probe_attempt_count ==
              result.audit.rank_local_probe_pruned_product_count +
                  result.audit.rank_local_probe_fail_open_product_count &&
          result.audit.maximum_rank_frontier_entry_count == 0U &&
          result.audit.maximum_rank_local_probe_candidate_count <=
              morsehgp3d::hierarchy::
                  higher_support_local_rank_probe_maximum_evaluation_count,
      "the bounded local Morton rank probe preserves the n=14 exhaustive oracle without a root DFS"
      " (attempts=" +
          std::to_string(result.audit.rank_local_probe_attempt_count) +
          ", candidates=" +
          std::to_string(
              result.audit.rank_local_probe_candidate_evaluation_count) +
          ", prunes=" +
          std::to_string(result.audit.rank_local_probe_pruned_product_count) +
          ", fail_open=" +
          std::to_string(
              result.audit.rank_local_probe_fail_open_product_count) +
          ", gate_skips=" +
          std::to_string(
              result.audit.rank_local_probe_geometric_gate_skip_count) +
          ")");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 5U, budget, result)
          .result_certified,
      "the n=14 local Morton rank-probe differential survives fresh replay");
}

void test_input_contract() {
  CanonicalPointCloud cloud = cloud_from({point(0.0, 0.0, 0.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            build_exact_higher_support_stream(index, cloud, 0U, budget));
      },
      "Kmax zero is outside the higher-support contract");
  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            build_exact_higher_support_stream(index, cloud, 11U, budget));
      },
      "Kmax above ten is outside the higher-support contract");

  CanonicalPointCloud tetrahedron = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex tetrahedron_index =
      MortonLbvhIndex::build(tetrahedron);
  ExactHigherSupportStreamBudget undersized = unlimited_budget();
  undersized.maximum_frontier_entry_count = 1U;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_higher_support_stream(
            tetrahedron_index, tetrahedron, 10U, undersized));
      },
      "the initial triangle and tetrahedron roots require two frontier slots");
}

void test_reinjectable_chunks_and_hostile_mutations() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 10U};
  ExactHigherSupportAnchoredSession session{authority};
  check(
      authority.audit().manifest_cached &&
          authority.audit().manifest_build_count == 1U &&
          authority.audit().canonical_cloud_point_hash_count == cloud.size(),
      "the higher-support authority manifest is built and cached once");

  ExactHigherSupportCheckpoint checkpoint = session.trusted_checkpoint();
  check(
      verify_exact_higher_support_checkpoint(authority, checkpoint)
          .local_integrity_verified &&
          checkpoint.next_chunk_sequence == 0U &&
          checkpoint.frontier.size() == 2U &&
          checkpoint.cumulative_audit.remaining_frontier_support_count == 5,
      "the two exact arity roots form an integrity-verified initial checkpoint");

  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  unit_budget.maximum_emitted_record_count = 8U;
  unit_budget.maximum_emitted_point_id_reference_count = 64U;
  unit_budget.maximum_prune_receipt_count = 8U;
  unit_budget.maximum_global_closed_ball_query_count = 8U;
  unit_budget.maximum_point_classification_count = 64U;
  std::vector<ExactHigherSupportStreamBudget> budgets;
  std::vector<ExactHigherSupportStreamChunk> chunks;
  for (std::size_t step = 0U;
       step < 256U && !checkpoint.locally_complete();
       ++step) {
    ExactHigherSupportStreamChunk chunk =
        session.prepare_next(unit_budget, checkpoint);
    const auto transition =
        session.commit_prepared(unit_budget, checkpoint, chunk);
    check(
        transition.chunk_transition_verified,
        "each unit-work higher-support transition is freshly replayed");
    budgets.push_back(unit_budget);
    chunks.push_back(std::move(chunk));
    checkpoint = session.trusted_checkpoint();
  }
  check(
      checkpoint.locally_complete() && chunks.size() > 1U,
      "unit-work chunks persist and resume every charged product stage");
  const auto resident =
      build_exact_higher_support_stream(index, cloud, 10U, unlimited_budget());
  check(
      checkpoint.cumulative_audit == resident.audit &&
          checkpoint.output_record_count ==
              resident.audit.emitted_record_count,
      "chunked execution reaches the resident exact audit without double charging");
  ExactHigherSupportAnchoredSession resident_session{authority};
  const ExactHigherSupportCheckpoint resident_source =
      resident_session.trusted_checkpoint();
  const ExactHigherSupportStreamChunk resident_chunk =
      resident_session.prepare_next(unlimited_budget(), resident_source);
  const auto resident_transition = resident_session.commit_prepared(
      unlimited_budget(), resident_source, resident_chunk);
  check(
      resident_transition.chunk_transition_verified &&
          resident_session.trusted_checkpoint().locally_complete() &&
          resident_session.trusted_checkpoint().output_chain_digest ==
              checkpoint.output_chain_digest &&
          resident_session.trusted_checkpoint().output_record_count ==
              checkpoint.output_record_count,
      "the three-kind output chain is independent of chunk boundaries");
  const auto run = verify_exact_higher_support_stream_run(
      index, cloud, 10U, budgets, chunks);
  check(
      run.anchored_run_certified &&
          run.verified_chunk_count == chunks.size(),
      "the terminal chunk lineage is anchored at the reconstructed roots");

  ExactHigherSupportCheckpoint invalid =
      make_initial_exact_higher_support_checkpoint(authority);
  invalid.frontier.back().groups[0].leaf_end += 1U;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed false Morton range fails closed");

  invalid = make_initial_exact_higher_support_checkpoint(authority);
  invalid.cumulative_audit.remaining_frontier_support_count += 1;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed BigInt partition mutation fails closed");

  invalid = make_initial_exact_higher_support_checkpoint(authority);
  invalid.cumulative_audit.rank_query_outside_or_boundary_node_count = 1U;
  invalid.cumulative_audit.rank_query_outside_or_boundary_point_count = 1U;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed outside-subtree audit without a rank-node visit fails closed");

  invalid = make_initial_exact_higher_support_checkpoint(authority);
  invalid.output_chain_digest = invalid.manifest.semantic_digest;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed nonempty output chain at record zero fails closed");

  ExactHigherSupportAnchoredSession mutation_session{authority};
  const ExactHigherSupportCheckpoint mutation_source =
      mutation_session.trusted_checkpoint();
  auto mutated_chunk =
      mutation_session.prepare_next(unit_budget, mutation_source);
  mutated_chunk.next_checkpoint.output_record_count += 1U;
  mutated_chunk.next_checkpoint.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(
          mutated_chunk.next_checkpoint);
  check(
      !mutation_session
           .commit_prepared(unit_budget, mutation_source, mutated_chunk)
           .chunk_transition_verified,
      "a self-rehashed successor mutation fails fresh transition replay");
  check(
      mutation_session.trusted_checkpoint() == mutation_source,
      "a rejected successor cannot advance the anchored session");
  const auto valid_mutation_chunk =
      mutation_session.prepare_next(unit_budget, mutation_source);
  check(
      mutation_session
          .commit_prepared(
              unit_budget, mutation_source, valid_mutation_chunk)
          .chunk_transition_verified,
      "a freshly replayed successor advances the anchored session");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(mutation_session.prepare_next(
            unit_budget, mutation_source));
      },
      "a previously valid source token becomes stale after commit");

  ExactHigherSupportCheckpoint locally_coherent_forgery =
      make_initial_exact_higher_support_checkpoint(authority);
  const auto tetrahedron_root = locally_coherent_forgery.frontier.back();
  locally_coherent_forgery.frontier.assign(5U, tetrahedron_root);
  locally_coherent_forgery.cumulative_audit.maximum_frontier_entry_count = 5U;
  locally_coherent_forgery.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(
          locally_coherent_forgery);
  check(
      verify_exact_higher_support_checkpoint(
          authority, locally_coherent_forgery)
          .local_integrity_verified,
      "local integrity intentionally does not claim frontier provenance");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(mutation_session.prepare_next(
            unit_budget, locally_coherent_forgery));
      },
      "an equal-cardinality forged frontier cannot enter the anchored session");

  const std::vector<CertifiedPoint3> receipt_points{
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)};
  CanonicalPointCloud receipt_cloud = cloud_from(receipt_points);
  MortonLbvhIndex receipt_index = MortonLbvhIndex::build(receipt_cloud);
  const ExactHigherSupportAuthorityContext receipt_authority{
      receipt_index, receipt_cloud, 3U};
  ExactHigherSupportCheckpoint receipt_checkpoint =
      make_initial_exact_higher_support_checkpoint(receipt_authority);
  ExactHigherSupportAnchoredSession receipt_session{receipt_authority};
  receipt_checkpoint = receipt_session.trusted_checkpoint();
  bool observed_rank_cursor = false;
  for (std::size_t step = 0U;
       step < 512U && !receipt_checkpoint.locally_complete() &&
       !observed_rank_cursor;
       ++step) {
    const auto chunk =
        receipt_session.prepare_next(unit_budget, receipt_checkpoint);
    const auto transition = receipt_session.commit_prepared(
        unit_budget, receipt_checkpoint, chunk);
    check(
        transition.chunk_transition_verified,
        "the anchored receipt session accepts its freshly replayed chunk");
    receipt_checkpoint = receipt_session.trusted_checkpoint();
    if (receipt_checkpoint.pending_product.has_value()) {
      const auto& pending = *receipt_checkpoint.pending_product;
      observed_rank_cursor =
          pending.stage == ExactHigherSupportPendingStage::rank_search &&
          pending.rank_search_started &&
          pending.rank_probe_next_candidate_index > 0U &&
          pending.rank_frontier.empty();
    }
  }
  check(
      observed_rank_cursor &&
          verify_exact_higher_support_checkpoint(
              receipt_authority, receipt_checkpoint)
              .local_integrity_verified,
      "a nonempty exact local Morton rank-probe cursor is independently recertified");
  if (observed_rank_cursor) {
    ExactHigherSupportCheckpoint mutated_receipt = receipt_checkpoint;
    mutated_receipt.pending_product->rank_probe_next_candidate_index =
        std::numeric_limits<std::size_t>::max();
    mutated_receipt.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(mutated_receipt);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, mutated_receipt)
             .local_integrity_verified,
        "a self-rehashed local rank cursor beyond its regenerated halo fails closed");

    ExactHigherSupportCheckpoint oversized_receipts = receipt_checkpoint;
    const auto& first_group =
        oversized_receipts.pending_product->product.groups[0];
    const morsehgp3d::hierarchy::ExactHigherSupportNodeReceipt receipt{
        first_group.node_index,
        first_group.leaf_begin,
        first_group.leaf_end};
    oversized_receipts.pending_product->strict_interior_receipts.assign(
        10U, receipt);
    oversized_receipts.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(oversized_receipts);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, oversized_receipts)
             .local_integrity_verified,
        "more than nine strict receipts fail before exact recomputation");

    ExactHigherSupportCheckpoint forbidden_rank_frontier =
        receipt_checkpoint;
    forbidden_rank_frontier.pending_product->rank_frontier.assign(
        1U, receipt);
    forbidden_rank_frontier.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(
            forbidden_rank_frontier);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, forbidden_rank_frontier)
             .local_integrity_verified,
        "a local rank frontier cannot be forged from a support-domain node");
  }

  CanonicalPointCloud singleton = cloud_from({point(0.0, 0.0, 0.0)});
  MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  const ExactHigherSupportAuthorityContext singleton_authority{
      singleton_index, singleton, 10U};
  ExactHigherSupportAnchoredSession singleton_session{singleton_authority};
  const auto terminal = make_initial_exact_higher_support_checkpoint(
      singleton_authority);
  check(
      terminal.locally_complete() &&
          verify_exact_higher_support_checkpoint(
              singleton_index, singleton, 10U, terminal)
              .local_integrity_verified,
      "a cloud below arity three starts at a certified terminal checkpoint");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(singleton_session.prepare_next(
            unlimited_budget(), singleton_session.trusted_checkpoint()));
      },
      "an anchored terminal checkpoint has no no-op successor");
  const std::vector<ExactHigherSupportStreamBudget> redundant_budgets{
      unlimited_budget()};
  const std::vector<ExactHigherSupportStreamChunk> redundant_chunks(1U);
  check(
      !verify_exact_higher_support_stream_run(
           singleton_index,
           singleton,
           10U,
           redundant_budgets,
           redundant_chunks)
           .anchored_run_certified,
      "an anchored run rejects every chunk after a terminal root state");

  ExactHigherSupportAnchoredSession temporary_authority_session{
      ExactHigherSupportAuthorityContext{index, cloud, 10U}};
  check(
      temporary_authority_session.trusted_checkpoint() ==
          make_initial_exact_higher_support_checkpoint(authority),
      "the session owns its immutable authority cache rather than its wrapper lifetime");
}

void test_internal_terminal_authority_and_clean_chunk_cap() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;

  ExactHigherSupportTerminalSession session{
      index, cloud, 10U, unit_budget, 256U};
  check(
      session.run_to_terminal() ==
              ExactHigherSupportTerminalRunStatus::terminal &&
          session.trusted_checkpoint().locally_complete() &&
          session.chunk_count() > 1U,
      "the internal producer reaches a terminal checkpoint through fixed-budget chunks");
  auto authority = std::move(session).seal();
  const bool observed_empty_segment = std::any_of(
      authority.segments().begin(),
      authority.segments().end(),
      [](const auto& segment) {
        return segment.emitted_record_count() == 0U;
      });
  check(
      authority.sealed_in_process_terminal_authority() &&
          authority.bound_to(index, cloud, 10U) &&
          authority.event_count() == 5U &&
          authority.relevant_extra_shell_diagnostic_count() == 0U &&
          authority.destroyed_prune_certificate_count() == 0U &&
          authority.output_record_count() == 5U &&
          authority.chunk_count() == authority.segments().size() &&
          observed_empty_segment &&
          !authority.fresh_replay_performed() &&
          !authority.durable_authority_claimed() &&
          !authority.public_status_claimed(),
      "the move-only authority retains every bounded segment without overstating its scope");
  const std::size_t sealed_chunk_count = authority.chunk_count();
  auto released_segments = std::move(authority).release_segments();
  check(
      released_segments.size() == sealed_chunk_count &&
          !authority.sealed_in_process_terminal_authority(),
      "terminal segments are released only through an rvalue authority");

  ExactHigherSupportTerminalSession capped{
      index, cloud, 10U, unit_budget, 1U};
  check(
      capped.run_to_terminal() ==
              ExactHigherSupportTerminalRunStatus::
                  maximum_chunk_count_reached &&
          capped.chunk_count() == 1U &&
          !capped.trusted_checkpoint().locally_complete(),
      "the internal producer reports its chunk cap without forging completion");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(std::move(capped).seal());
      },
      "a capped nonterminal session cannot mint an authority");
}

void test_unsealed_terminal_segment_drain() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  ExactHigherSupportTerminalSession resident_session{
      index, cloud, 10U, unit_budget, 256U};
  check(
      resident_session.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the resident comparison fixture reaches terminality");
  const auto resident = std::move(resident_session).seal();
  ExactHigherSupportTerminalSession session{
      index, cloud, 10U, unit_budget, 256U};
  const auto initial_checkpoint = session.trusted_checkpoint();
  auto drained = session.drain_next_unsealed_segment();
  const ExactHigherSupportTerminalSegment& segment = *drained.segment();
  check(
      drained.status() ==
              ExactHigherSupportUnsealedDrainStatus::segment_ready &&
          drained.segment_available() && drained.manifest() != nullptr &&
          *drained.manifest() ==
              session.trusted_checkpoint().manifest &&
          !resident.segments().empty() &&
          segment == resident.segments().front() &&
          segment.source_checkpoint_digest ==
              initial_checkpoint.checkpoint_digest &&
          session.unsealed_segment_drain_performed() &&
          session.unconsumed_segment_outstanding() &&
          session.chunk_count() == 1U &&
          session.released_segment_count() == 1U &&
          session.resident_segment_count() == 0U &&
          segment.first_output_record_index +
                  segment.emitted_record_count() ==
              session.trusted_checkpoint().output_record_count,
      "one unsealed drain transfers exactly the next provenance-bound bounded segment");
  const auto held_checkpoint = session.trusted_checkpoint();
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(session.drain_next_unsealed_segment());
      },
      "an unconsumed segment cannot be skipped by a second drain");
  auto moved = std::move(drained);
  check(
      drained.moved_from() && !drained.consumed() &&
          moved.segment_available() &&
          session.unconsumed_segment_outstanding() &&
          session.trusted_checkpoint() == held_checkpoint,
      "moving a drain token preserves its unique outstanding lease");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(session.run_to_terminal());
      },
      "a drained producer cannot switch back to the resident run mode");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(std::move(session).seal());
      },
      "the first successful unsealed drain permanently revokes resident sealing");

  ExactHigherSupportTerminalSession abandoned_session{
      index, cloud, 10U, unit_budget, 256U};
  {
    auto abandoned = abandoned_session.drain_next_unsealed_segment();
    check(
        abandoned.segment_available() &&
            abandoned_session.unconsumed_segment_outstanding(),
        "the abandonment fixture owns one outstanding segment");
  }
  check(
      abandoned_session.unconsumed_segment_outstanding(),
      "destroying an unconsumed drain token keeps the producer fail-closed");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(
            abandoned_session.drain_next_unsealed_segment());
      },
      "an abandoned segment cannot be skipped by a later drain");

  ExactHigherSupportTerminalSession capped{
      index, cloud, 10U, unit_budget, 0U};
  const auto capped_checkpoint = capped.trusted_checkpoint();
  auto cap = capped.drain_next_unsealed_segment();
  check(
      cap.status() == ExactHigherSupportUnsealedDrainStatus::
              maximum_chunk_count_reached &&
          !cap.segment_available() && cap.segment() == nullptr &&
          cap.manifest() == nullptr &&
          capped.trusted_checkpoint() == capped_checkpoint &&
          capped.chunk_count() == 0U &&
          capped.released_segment_count() == 0U &&
          capped.resident_segment_count() == 0U,
      "the unsealed drain reports its chunk cap without changing state");
}

void test_terminal_root_needs_no_chunk() {
  CanonicalPointCloud cloud = cloud_from({
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactHigherSupportTerminalSession session{
      index, cloud, 10U, unlimited_budget(), 0U};
  auto terminal = session.drain_next_unsealed_segment();
  check(
      terminal.status() ==
              ExactHigherSupportUnsealedDrainStatus::terminal &&
          !terminal.segment_available() &&
          session.run_to_terminal() ==
              ExactHigherSupportTerminalRunStatus::terminal &&
          session.chunk_count() == 0U,
      "a canonical root below support arity three is already terminal");
  auto authority = std::move(session).seal();
  auto moved_authority = std::move(authority);
  check(
      !authority.sealed_in_process_terminal_authority() &&
          moved_authority.sealed_in_process_terminal_authority() &&
          moved_authority.segments().empty() &&
          moved_authority.output_record_count() == 0U,
      "a terminal root seals without manufacturing an empty chunk and moving its zero-segment authority revokes the source");

  CanonicalPointCloud replacement_cloud = cloud_from({
      point(2.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0)});
  MortonLbvhIndex replacement_index =
      MortonLbvhIndex::build(replacement_cloud);
  cloud = std::move(replacement_cloud);
  index = std::move(replacement_index);
  check(
      !moved_authority.bound_to(index, cloud, 10U) &&
          !moved_authority.sealed_in_process_terminal_authority(),
      "reassigning cloud and LBVH at the same addresses revokes the sealed source identity");
  static_cast<void>(
      std::move(moved_authority).release_segments());
  check(
      !moved_authority.sealed_in_process_terminal_authority(),
      "releasing a zero-segment terminal authority consumes it");

  CanonicalPointCloud seal_cloud = cloud_from({
      point(4.0, 0.0, 0.0),
      point(5.0, 0.0, 0.0)});
  MortonLbvhIndex seal_index = MortonLbvhIndex::build(seal_cloud);
  ExactHigherSupportTerminalSession changed_before_seal{
      seal_index, seal_cloud, 10U, unlimited_budget(), 0U};
  check(
      changed_before_seal.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the source identity is intact while the terminal root executes");
  CanonicalPointCloud final_cloud = cloud_from({
      point(6.0, 0.0, 0.0),
      point(7.0, 0.0, 0.0)});
  MortonLbvhIndex final_index = MortonLbvhIndex::build(final_cloud);
  seal_cloud = std::move(final_cloud);
  seal_index = std::move(final_index);
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(std::move(changed_before_seal).seal());
      },
      "a source identity change between execution and seal fails closed");
}

}  // namespace


// R1 (reduced verification): a tile-certified commit chain must reach the
// same terminal science as the fresh-replay chain.  The tile payload here
// is derived from a prepared candidate exactly as the device bridge will
// derive it from its drained records and certified masses -- consumed root
// count, host-classified records, category counts and BigInt masses -- and
// nothing else crosses into the session.
[[nodiscard]] ExactHigherSupportTileCertifiedTile tile_from_candidate(
    const ExactHigherSupportCheckpoint& source,
    const ExactHigherSupportStreamChunk& candidate) {
  const auto& before = candidate.cumulative_audit_before;
  const auto& after = candidate.cumulative_audit_after;
  ExactHigherSupportTileCertifiedTile tile;
  tile.consumed_root_count =
      source.frontier.size() - candidate.next_checkpoint.frontier.size();
  tile.events = candidate.events;
  tile.diagnostics = candidate.relevant_extra_shell_diagnostics;
  tile.prune_certificates = candidate.prune_certificates;
  tile.well_centering_pruned_support_mass =
      after.well_centering_pruned_support_count -
      before.well_centering_pruned_support_count;
  tile.rank_pruned_support_mass =
      after.rank_pruned_support_count - before.rank_pruned_support_count;
  tile.leaf_classified_support_mass =
      after.leaf_classified_support_count -
      before.leaf_classified_support_count;
  tile.affinely_dependent_leaf_count =
      after.affinely_dependent_leaf_count -
      before.affinely_dependent_leaf_count;
  tile.boundary_reduced_leaf_count =
      after.boundary_reduced_leaf_count - before.boundary_reduced_leaf_count;
  tile.exterior_circumcenter_leaf_count =
      after.exterior_circumcenter_leaf_count -
      before.exterior_circumcenter_leaf_count;
  tile.above_rank_leaf_count =
      after.above_rank_leaf_count - before.above_rank_leaf_count;
  tile.closed_ball_query_count =
      after.global_closed_ball_query_count -
      before.global_closed_ball_query_count;
  tile.point_classification_count =
      after.point_classification_count - before.point_classification_count;
  return tile;
}

void test_tile_certified_commit_equals_fresh_replay() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(0.25, 0.5, -0.75)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 10U};
  const auto oracle = build_exact_higher_support_stream(
      index, cloud, 10U, unlimited_budget());
  check(oracle.stream_complete(), "the tile-certified oracle completes");

  // Reference chain: fresh replay through the assembler.
  ExactHigherSupportAnchoredStreamAssembler replay_assembler{authority};
  ExactHigherSupportAnchoredStreamAssembler tile_assembler{authority};
  std::size_t transitions = 0U;
  while (!tile_assembler.trusted_checkpoint().locally_complete() &&
         transitions < 64U) {
    ++transitions;
    ExactHigherSupportStreamBudget budget = unlimited_budget();
    budget.maximum_work_unit_count = 3U * transitions;
    const auto source = tile_assembler.trusted_checkpoint();
    const auto candidate = tile_assembler.prepare_next(budget, source);
    if (candidate.next_checkpoint.pending_product.has_value() ||
        candidate.next_checkpoint.frontier.size() >=
            source.frontier.size()) {
      continue;
    }
    // The reference chain commits the same candidate by fresh replay.
    const auto replay_source = replay_assembler.trusted_checkpoint();
    const auto replay_candidate =
        replay_assembler.prepare_next(budget, replay_source);
    check(
        replay_assembler
            .commit_prepared(budget, replay_source, replay_candidate)
            .chunk_transition_verified,
        "the reference fresh-replay chain commits its candidate");

    const auto verification = tile_assembler.commit_tile_certified(
        source, tile_from_candidate(source, candidate));
    check(
        verification.chunk_transition_verified &&
            verification.next_checkpoint_anchored &&
            verification.records_individually_exact,
        "a tile-certified transition commits without a generator rerun");
    if (!verification.chunk_transition_verified) {
      return;
    }
  }
  check(
      tile_assembler.trusted_checkpoint().locally_complete() &&
          replay_assembler.trusted_checkpoint().locally_complete(),
      "both chains reach the locally complete terminal");

  const auto tile_seal =
      tile_assembler.seal_terminal_stream(unlimited_budget());
  const auto replay_seal =
      replay_assembler.seal_terminal_stream(unlimited_budget());
  check(
      tile_seal.certified_sealed() && replay_seal.certified_sealed(),
      "both chains seal their terminal stream");
  if (!tile_seal.certified_sealed() || !replay_seal.certified_sealed()) {
    return;
  }
  check(
      tile_seal.result->events == oracle.events &&
          tile_seal.result->relevant_extra_shell_diagnostics ==
              oracle.relevant_extra_shell_diagnostics &&
          tile_seal.result->requirements == oracle.requirements,
      "the tile-certified chain reproduces the oracle records");
  check(
      tile_seal.result->audit.total_support_count ==
              oracle.audit.total_support_count &&
          tile_seal.result->audit.resolved_support_count ==
              oracle.audit.resolved_support_count &&
          tile_seal.result->audit.remaining_frontier_support_count == 0 &&
          tile_seal.result->stream_complete() &&
          tile_seal.result
              ->absence_of_additional_higher_supports_certified(),
      "the tile-certified chain closes the exact universe");
  check(
      tile_seal.certificate.verification_basis() ==
              ExactHigherSupportVerificationBasis::
                  device_search_host_exact_record_classification_bigint_closure &&
          replay_seal.certificate.verification_basis() ==
              ExactHigherSupportVerificationBasis::
                  fresh_cpu_replay_every_commit,
      "each certificate declares its own verification basis truthfully");
  check(
      tile_seal.certificate.certifies(*tile_seal.result),
      "the tile-certified certificate binds its own sealed result");
  // The scientific content is identical; only the CPU work counters
  // differ, and on the tile-certified chain they are exactly zero -- the
  // internal witness that no generator ran per transition.
  check(
      tile_seal.result->audit.work_unit_count == 0U &&
          tile_seal.result->audit.support_product_visit_count == 0U &&
          tile_seal.result->audit.rank_witness_node_visit_count == 0U &&
          tile_seal.result->audit.rank_search_count == 0U &&
          replay_seal.result->audit.work_unit_count != 0U,
      "the tile-certified chain counts no host generator work");
  check(
      tile_seal.result->events == replay_seal.result->events &&
          tile_seal.result->relevant_extra_shell_diagnostics ==
              replay_seal.result->relevant_extra_shell_diagnostics &&
          tile_seal.result->audit.accepted_event_count ==
              replay_seal.result->audit.accepted_event_count &&
          tile_seal.result->audit.resolved_support_count ==
              replay_seal.result->audit.resolved_support_count,
      "the two chains agree record for record and mass for mass");

  // Anti-forge: a falsified mass, a dropped record and a mutated category
  // must all fail closed before any state change.
  ExactHigherSupportAnchoredStreamAssembler forge{authority};
  const auto forge_source = forge.trusted_checkpoint();
  ExactHigherSupportStreamBudget forge_budget = unlimited_budget();
  const auto forge_candidate =
      forge.prepare_next(forge_budget, forge_source);
  const auto honest_tile =
      tile_from_candidate(forge_source, forge_candidate);

  auto inflated = honest_tile;
  inflated.leaf_classified_support_mass += 1;
  check(
      !forge.commit_tile_certified(forge_source, std::move(inflated))
           .chunk_transition_verified &&
          forge.trusted_checkpoint() == forge_source,
      "a falsified mass fails closed without advancing the chain");

  auto truncated = honest_tile;
  if (!truncated.events.empty()) {
    truncated.events.pop_back();
    check(
        !forge.commit_tile_certified(forge_source, std::move(truncated))
             .chunk_transition_verified &&
            forge.trusted_checkpoint() == forge_source,
        "a dropped record fails closed without advancing the chain");
  }

  auto miscategorized = honest_tile;
  ++miscategorized.above_rank_leaf_count;
  check(
      !forge.commit_tile_certified(forge_source, std::move(miscategorized))
           .chunk_transition_verified &&
          forge.trusted_checkpoint() == forge_source,
      "a mutated leaf category fails closed without advancing the chain");

  auto wrong_root_count = honest_tile;
  ++wrong_root_count.consumed_root_count;
  check(
      !forge.commit_tile_certified(forge_source, std::move(wrong_root_count))
           .chunk_transition_verified &&
          forge.trusted_checkpoint() == forge_source,
      "an overstated consumed root count fails closed");

  auto honest = honest_tile;
  check(
      forge.commit_tile_certified(forge_source, std::move(honest))
          .chunk_transition_verified,
      "the honest tile still commits after the rejected forgeries");
}

int main() {
  test_bigint_universe();
  test_regular_tetrahedron_complete_and_fresh_replay();
  test_intrinsically_above_rank_and_budgeted_frontier();
  test_sparse_extra_shell_diagnostic();
  test_nonzero_universal_rank_receipts();
  test_n14_local_rank_probe_exhaustive_differential();
  test_input_contract();
  test_reinjectable_chunks_and_hostile_mutations();
  test_internal_terminal_authority_and_clean_chunk_cap();
  test_unsealed_terminal_segment_drain();
  test_terminal_root_needs_no_chunk();
  test_tile_certified_commit_equals_fresh_replay();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "higher-support stream tests passed\n";
  return 0;
}
