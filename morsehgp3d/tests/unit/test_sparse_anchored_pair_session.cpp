#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleConfig;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairRecord;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSession;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionAdvanceBudget;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionStepKind;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionStopReason;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionTotalCapacity;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairTerminalAuthority;
using morsehgp3d::hierarchy::build_exact_pair_support_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

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
  for (const auto& coordinate : coordinates) {
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

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity roomy_capacity() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactSparseAnchoredPairSessionTotalCapacity{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget roomy_budget(
    std::size_t maximum_closed_rank) {
  return ExactSparseAnchoredPairSessionAdvanceBudget{
      {4096U, 4096U, 4096U, 4096U},
      {4096U},
      1U,
      maximum_closed_rank + 1U};
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget segmented_budget(
    std::size_t maximum_closed_rank) {
  return ExactSparseAnchoredPairSessionAdvanceBudget{
      {1U, 1U, 1U, 1U},
      {1U},
      1U,
      maximum_closed_rank + 1U};
}

[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority finish(
    ExactSparseAnchoredPairSession&& session,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactSparseAnchoredPairSessionAdvanceBudget budget) {
  for (std::size_t call = 0U; call < 1000000U; ++call) {
    const auto step = session.advance(index, cloud, budget);
    require(
        step.kind() !=
            ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted,
        "a roomy sparse session exhausted an immutable total capacity");
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
  }
  throw std::runtime_error("a sparse anchored session did not terminate");
}

struct CanonicalRecords {
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic> diagnostics;

  friend bool operator==(const CanonicalRecords&, const CanonicalRecords&) =
      default;
};

[[nodiscard]] CanonicalRecords canonical_records(
    std::span<const ExactSparseAnchoredPairRecord> records) {
  CanonicalRecords result;
  for (const ExactSparseAnchoredPairRecord& record : records) {
    if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      result.events.push_back(*event);
    } else {
      result.diagnostics.push_back(
          std::get<ExactPairSupportExtraShellDiagnostic>(record));
    }
  }
  const auto by_support = [](const auto& left, const auto& right) {
    return left.support_ids < right.support_ids;
  };
  std::sort(result.events.begin(), result.events.end(), by_support);
  std::sort(
      result.diagnostics.begin(), result.diagnostics.end(), by_support);
  return result;
}

[[nodiscard]] CanonicalRecords historical_records(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  const auto historical = build_exact_pair_support_stream(
      index,
      cloud,
      maximum_closed_rank - 1U,
      ExactPairSupportStreamBudget{
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum});
  require(historical.stream_complete(), "the bounded P7b oracle did not close");
  CanonicalRecords result{
      historical.events, historical.relevant_extra_shell_diagnostics};
  const auto by_support = [](const auto& left, const auto& right) {
    return left.support_ids < right.support_ids;
  };
  std::sort(result.events.begin(), result.events.end(), by_support);
  std::sort(
      result.diagnostics.begin(), result.diagnostics.end(), by_support);
  return result;
}

void check_terminal_identities(
    const ExactSparseAnchoredPairTerminalAuthority& authority) {
  const auto& audit = authority.audit();
  const auto& candidate = authority.candidate_audit();
  const auto& schedule = authority.schedule_audit();
  require(
      audit.complete && audit.every_prune_recertified &&
          audit.directed_coverage_certified &&
          audit.orientation_partition_certified &&
          audit.candidate_classification_partition_certified &&
          audit.output_partition_certified &&
          audit.retained_records_certified &&
          audit.no_forbidden_global_structure_materialized &&
          authority.sealed_in_process_terminal_authority(),
      "a terminal authority lost one of its certified partitions");
  require(
      audit.authenticated_pruned_directed_pair_count +
              candidate.orientation_check_count ==
          audit.directed_pair_universe_size,
      "the directed prune/orientation coverage did not close");
  require(
      candidate.candidate_pair_count +
              candidate.reverse_or_self_orientation_skip_count ==
          candidate.orientation_check_count,
      "the oriented candidate partition did not close");
  require(
      candidate.candidate_pair_count == audit.admitted_candidate_count &&
          audit.admitted_candidate_count == audit.candidate_started_count &&
          audit.admitted_candidate_count ==
              audit.classification_terminal_count &&
          audit.classification_terminal_count ==
              audit.above_rank_count + audit.emitted_record_count &&
          audit.emitted_record_count ==
              audit.accepted_event_count +
                  audit.relevant_extra_shell_diagnostic_count &&
          audit.candidate_record_ready_count == audit.emitted_record_count,
      "the exact candidate/output partitions did not close");
  require(
      schedule.complete && schedule.morton_anchor_partition_complete &&
          schedule.scheduled_anchor_count == audit.point_count &&
          schedule.prepared_group_count == schedule.completed_group_count &&
          candidate.no_dynamic_candidate_or_output_arena_materialized &&
          schedule.no_global_anchor_pair_or_output_arena_materialized,
      "the Morton schedule lost its bounded terminal partition");
}

void test_fallback_terminal_matches_p7b() {
  CanonicalPointCloud cloud = make_three_dimensional_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactMortonGroupedAnchoredPairScheduleConfig config{5U, 2U};
  ExactSparseAnchoredPairSession session = ExactSparseAnchoredPairSession::start(
      index, cloud, maximum_closed_rank, config, roomy_capacity());
  ExactSparseAnchoredPairTerminalAuthority authority = finish(
      std::move(session), index, cloud, roomy_budget(maximum_closed_rank));

  check_terminal_identities(authority);
  require(
      authority.candidate_audit().candidate_pair_count == 276U &&
          authority.candidate_audit().orientation_check_count == 576U &&
          authority.candidate_audit()
                  .reverse_or_self_orientation_skip_count ==
              300U &&
          authority.audit().authenticated_prune_count == 0U,
      "the all-fallback directed universe changed its exact cardinalities");
  require(
      canonical_records(authority.records()) ==
          historical_records(index, cloud, maximum_closed_rank),
      "the sparse fallback stream changed the independent P7b science");
}

void test_pruned_segmentation_matches_p7b() {
  CanonicalPointCloud cloud = make_line_cloud(20U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactMortonGroupedAnchoredPairScheduleConfig config{4U, 12U};

  ExactSparseAnchoredPairSession roomy = ExactSparseAnchoredPairSession::start(
      index, cloud, maximum_closed_rank, config, roomy_capacity());
  ExactSparseAnchoredPairTerminalAuthority roomy_authority = finish(
      std::move(roomy), index, cloud, roomy_budget(maximum_closed_rank));
  ExactSparseAnchoredPairSession segmented =
      ExactSparseAnchoredPairSession::start(
          index, cloud, maximum_closed_rank, config, roomy_capacity());
  ExactSparseAnchoredPairTerminalAuthority segmented_authority = finish(
      std::move(segmented),
      index,
      cloud,
      segmented_budget(maximum_closed_rank));

  check_terminal_identities(roomy_authority);
  check_terminal_identities(segmented_authority);
  const CanonicalRecords expected =
      historical_records(index, cloud, maximum_closed_rank);
  require(
      roomy_authority.audit().authenticated_prune_count > 0U &&
          roomy_authority.audit()
                  .authenticated_pruned_directed_pair_count >
              0U &&
          canonical_records(roomy_authority.records()) == expected &&
          canonical_records(segmented_authority.records()) == expected,
      "pruning or one-node segmentation changed the P7b science");
  require(
      roomy_authority.audit().authenticated_pruned_directed_pair_count ==
              segmented_authority.audit()
                  .authenticated_pruned_directed_pair_count &&
          roomy_authority.audit().classification_node_visit_count ==
              segmented_authority.audit().classification_node_visit_count &&
          roomy_authority.candidate_audit().orientation_check_count ==
              segmented_authority.candidate_audit()
                  .orientation_check_count &&
          roomy_authority.candidate_audit()
                  .grouped_traversal_node_visit_count ==
              segmented_authority.candidate_audit()
                  .grouped_traversal_node_visit_count &&
          roomy_authority.candidate_audit()
                  .grouped_traversal_exact_predicate_count ==
              segmented_authority.candidate_audit()
                  .grouped_traversal_exact_predicate_count,
      "segmentation changed physical sparse-session work");
}

void test_atomic_output_budgets_and_total_capacity() {
  CanonicalPointCloud cloud = make_line_cloud(4U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t maximum_closed_rank = 3U;
  const ExactMortonGroupedAnchoredPairScheduleConfig config{4U, 0U};

  ExactSparseAnchoredPairSession session = ExactSparseAnchoredPairSession::start(
      index, cloud, maximum_closed_rank, config, roomy_capacity());
  require_throws<std::logic_error>(
      [&] { static_cast<void>(std::move(session).seal()); },
      "an incomplete sparse session minted terminal authority");
  require(session.ready(), "a rejected premature seal corrupted the session");

  bool ready_record_seen = false;
  for (std::size_t call = 0U; call < 4096U; ++call) {
    const auto step =
        session.advance(index, cloud, roomy_budget(maximum_closed_rank));
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::candidate_record_ready) {
      ready_record_seen = true;
      break;
    }
  }
  require(ready_record_seen, "the atomic-output fixture emitted no record");
  const auto candidate_before = session.candidate_audit();
  const std::size_t records_before = session.records().size();

  ExactSparseAnchoredPairSessionAdvanceBudget no_record =
      roomy_budget(maximum_closed_rank);
  no_record.maximum_emitted_record_count = 0U;
  const auto record_stop = session.advance(index, cloud, no_record);
  require(
      record_stop.kind() ==
              ExactSparseAnchoredPairSessionStepKind::budget_exhausted &&
          record_stop.stop_reason() ==
              ExactSparseAnchoredPairSessionStopReason::emitted_record_limit &&
          session.records().size() == records_before &&
          session.candidate_audit() == candidate_before &&
          session.active_candidate(),
      "a zero record budget consumed or bypassed the ready P8k result");

  ExactSparseAnchoredPairSessionAdvanceBudget no_references =
      roomy_budget(maximum_closed_rank);
  no_references.maximum_emitted_point_id_reference_count = 0U;
  const auto reference_stop = session.advance(index, cloud, no_references);
  require(
      reference_stop.kind() ==
              ExactSparseAnchoredPairSessionStepKind::budget_exhausted &&
          reference_stop.stop_reason() ==
              ExactSparseAnchoredPairSessionStopReason::
                  emitted_point_id_reference_limit &&
          session.records().size() == records_before &&
          session.candidate_audit() == candidate_before &&
          session.active_candidate(),
      "a zero reference budget consumed or bypassed the ready P8k result");

  const auto committed =
      session.advance(index, cloud, roomy_budget(maximum_closed_rank));
  require(
      committed.kind() ==
              ExactSparseAnchoredPairSessionStepKind::record_committed &&
          session.records().size() == records_before + 1U &&
          !session.active_candidate(),
      "a retry did not commit the exact ready record once");

  ExactSparseAnchoredPairSessionTotalCapacity no_output_capacity =
      roomy_capacity();
  no_output_capacity.maximum_output_record_count = 0U;
  no_output_capacity.maximum_output_point_id_reference_count = 0U;
  ExactSparseAnchoredPairSession exhausted =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          maximum_closed_rank,
          config,
          no_output_capacity);
  bool total_stop_seen = false;
  for (std::size_t call = 0U; call < 4096U; ++call) {
    const auto step = exhausted.advance(
        index, cloud, roomy_budget(maximum_closed_rank));
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted) {
      total_stop_seen = true;
      require(
          step.stop_reason() ==
              ExactSparseAnchoredPairSessionStopReason::
                  total_output_record_capacity,
          "the immutable output cap reported the wrong typed axis");
      break;
    }
  }
  require(
      total_stop_seen && exhausted.total_capacity_exhausted(),
      "a zero immutable output cap did not fail closed");
  require_throws<std::logic_error>(
      [&] { static_cast<void>(std::move(exhausted).seal()); },
      "a capacity-exhausted session minted terminal authority");
}

void test_authority_binding_move_release_and_singleton() {
  CanonicalPointCloud singleton = make_line_cloud(1U);
  MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactMortonGroupedAnchoredPairScheduleConfig config{4U, 0U};
  ExactSparseAnchoredPairSessionTotalCapacity singleton_capacity =
      roomy_capacity();
  singleton_capacity.maximum_admitted_candidate_count = 0U;
  singleton_capacity.maximum_classification_node_visit_count = 0U;
  singleton_capacity.maximum_output_record_count = 0U;
  singleton_capacity.maximum_output_point_id_reference_count = 0U;
  ExactSparseAnchoredPairSession singleton_original =
      ExactSparseAnchoredPairSession::start(
          singleton_index,
          singleton,
          maximum_closed_rank,
          config,
          singleton_capacity);
  ExactSparseAnchoredPairSession singleton_session(
      std::move(singleton_original));
  require(
      !singleton_original.ready() &&
          !singleton_original.active_candidate() &&
          singleton_session.ready(),
      "moving a sparse session did not revoke exactly the source scope");
  ExactSparseAnchoredPairTerminalAuthority singleton_authority = finish(
      std::move(singleton_session),
      singleton_index,
      singleton,
      roomy_budget(maximum_closed_rank));
  check_terminal_identities(singleton_authority);
  require(
      singleton_authority.records().empty() &&
          singleton_authority.audit().directed_pair_universe_size == 1U &&
          singleton_authority.audit().authenticated_prune_count == 0U &&
          singleton_authority.candidate_audit().orientation_check_count ==
              1U &&
          singleton_authority.candidate_audit()
                  .reverse_or_self_orientation_skip_count ==
              1U &&
          singleton_authority.candidate_audit().candidate_pair_count == 0U,
      "the singleton lost its nonempty directed-universe partition");

  CanonicalPointCloud two_points = make_line_cloud(2U);
  MortonLbvhIndex two_point_index = MortonLbvhIndex::build(two_points);
  const ExactSparseAnchoredPairSessionTotalCapacity capacity =
      roomy_capacity();
  ExactSparseAnchoredPairSession two_point_session =
      ExactSparseAnchoredPairSession::start(
          two_point_index,
          two_points,
          maximum_closed_rank,
          config,
          capacity);
  ExactSparseAnchoredPairTerminalAuthority authority = finish(
      std::move(two_point_session),
      two_point_index,
      two_points,
      roomy_budget(maximum_closed_rank));
  require(
      authority.records().size() == 1U &&
          authority.bound_to(
              two_point_index,
              two_points,
              maximum_closed_rank,
              config,
              capacity),
      "the two-point authority lost its authentic binding or record");

  CanonicalPointCloud identity_alias = two_points;
  require(
      authority.bound_to(
          two_point_index,
          identity_alias,
          maximum_closed_rank,
          config,
          capacity),
      "the authority rejected an immutable cloud alias with the same token");
  CanonicalPointCloud foreign_cloud = make_line_cloud(2U);
  MortonLbvhIndex foreign_index = MortonLbvhIndex::build(foreign_cloud);
  require(
      !authority.bound_to(
          foreign_index,
          foreign_cloud,
          maximum_closed_rank,
          config,
          capacity),
      "the authority accepted foreign objects with equal coordinates");

  ExactSparseAnchoredPairTerminalAuthority moved(std::move(authority));
  require(
      !authority.sealed_in_process_terminal_authority() &&
          authority.maximum_closed_rank() == 0U &&
          authority.records().empty() &&
          moved.sealed_in_process_terminal_authority(),
      "moving terminal authority did not revoke exactly the source");
  std::vector<ExactSparseAnchoredPairRecord> released =
      std::move(moved).release_records();
  require(
      released.size() == 1U &&
          !moved.sealed_in_process_terminal_authority() &&
          moved.maximum_closed_rank() == 0U &&
          moved.candidate_audit().candidate_pair_count == 0U &&
          moved.schedule_audit().scheduled_anchor_count == 0U,
      "one-way record release lost its payload or retained authority");
  require_throws<std::logic_error>(
      [&] { static_cast<void>(std::move(moved).release_records()); },
      "a revoked authority released its records twice");
}

}  // namespace

int main() {
  static_assert(!std::is_default_constructible_v<ExactSparseAnchoredPairSession>);
  static_assert(!std::is_copy_constructible_v<ExactSparseAnchoredPairSession>);
  static_assert(
      std::is_nothrow_move_constructible_v<ExactSparseAnchoredPairSession>);
  static_assert(
      !std::is_default_constructible_v<
          ExactSparseAnchoredPairTerminalAuthority>);
  static_assert(
      !std::is_copy_constructible_v<
          ExactSparseAnchoredPairTerminalAuthority>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactSparseAnchoredPairTerminalAuthority>);

  try {
    test_fallback_terminal_matches_p7b();
    test_pruned_segmentation_matches_p7b();
    test_atomic_output_budgets_and_total_capacity();
    test_authority_binding_move_release_and_singleton();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse anchored pair session tests passed\n";
  return 0;
}
