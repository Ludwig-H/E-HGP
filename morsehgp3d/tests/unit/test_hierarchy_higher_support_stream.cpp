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
using morsehgp3d::hierarchy::ExactHigherSupportStopReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamStatus;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::exact_higher_support_candidate_universe_size;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream;
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
  check(
      result.stream_complete() &&
          result.audit.total_support_count == 210 &&
          result.audit.rank_pruned_support_count == 32 &&
          result.audit.emitted_rank_receipt_count == 9U &&
          result.audit.above_rank_leaf_count == 12U &&
          result.audit.resolved_support_count == 210 &&
          result.audit.remaining_frontier_support_count == 0,
      "nine exact universal receipts prune 32 higher supports at Kmax three");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 3U, budget, result)
          .result_certified,
      "nonzero rank receipts survive independent fresh replay");
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

}  // namespace

int main() {
  test_bigint_universe();
  test_regular_tetrahedron_complete_and_fresh_replay();
  test_intrinsically_above_rank_and_budgeted_frontier();
  test_sparse_extra_shell_diagnostic();
  test_nonzero_universal_rank_receipts();
  test_input_contract();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "higher-support stream tests passed\n";
  return 0;
}
