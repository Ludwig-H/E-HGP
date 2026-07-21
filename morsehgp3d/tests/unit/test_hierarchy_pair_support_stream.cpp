#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <bit>
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
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamResult;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::hierarchy::ExactPairSupportStreamVerification;
using morsehgp3d::hierarchy::build_exact_pair_support_stream;
using morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum;
using morsehgp3d::hierarchy::verify_exact_pair_support_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
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

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
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

[[nodiscard]] ExactPairSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] bool verification_closes(
    const ExactPairSupportStreamVerification& verification) {
  return verification.requested_budget_certified &&
         verification.requirements_certified &&
         verification.partial_records_individually_exact &&
         verification.completion_claim_certified &&
         verification.absence_claim_certified &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

void check_fresh_replay(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& result,
    const std::string& message) {
  check(
      verification_closes(verify_exact_pair_support_stream(
          index, cloud, maximum_order, budget, result)),
      message);
}

[[nodiscard]] std::uint64_t bits(double value) {
  return morsehgp3d::exact::canonicalize_binary64_bits(
      std::bit_cast<std::uint64_t>(value));
}

[[nodiscard]] ExactDyadicAabb3 box(
    std::array<double, 3> lower,
    std::array<double, 3> upper) {
  ExactDyadicAabb3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower_binary64_bits[axis] = bits(lower[axis]);
    result.upper_binary64_bits[axis] = bits(upper[axis]);
  }
  return result;
}

struct OraclePairRecords {
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic> diagnostics;
};

[[nodiscard]] OraclePairRecords brute_force_pair_records(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order) {
  OraclePairRecords records;
  const std::size_t effective_order =
      std::min(maximum_order, cloud.size());
  const std::size_t maximum_closed_rank =
      std::min(effective_order + 1U, cloud.size());
  if (maximum_closed_rank < 2U) {
    return records;
  }
  const std::size_t interior_cap = maximum_closed_rank - 2U;
  for (std::size_t first_index = 0U;
       first_index < cloud.size();
       ++first_index) {
    for (std::size_t second_index = first_index + 1U;
         second_index < cloud.size();
         ++second_index) {
      const PointId first = static_cast<PointId>(first_index);
      const PointId second = static_cast<PointId>(second_index);
      const auto sphere = morsehgp3d::exact::circumcenter(
          cloud.point(first), cloud.point(second));
      if (!sphere.center().has_value() ||
          !sphere.squared_level().has_value()) {
        throw std::logic_error(
            "an exhaustive pair did not define a unique sphere");
      }
      const auto partition = morsehgp3d::spatial::brute_force_closed_ball(
          cloud, *sphere.center(), *sphere.squared_level());
      if (partition.interior_ids().size() > interior_cap) {
        continue;
      }
      const std::array<PointId, 2> support{first, second};
      const std::vector<PointId> interior{
          partition.interior_ids().begin(),
          partition.interior_ids().end()};
      if (partition.shell_ids().size() == 2U &&
          std::equal(
              partition.shell_ids().begin(),
              partition.shell_ids().end(),
              support.begin(),
              support.end())) {
        records.events.push_back(ExactPairSupportEvent{
            support,
            *sphere.center(),
            *sphere.squared_level(),
            interior,
            partition.closed_rank(),
            partition.exterior_ids().size()});
        continue;
      }
      const auto extra = std::find_if(
          partition.shell_ids().begin(),
          partition.shell_ids().end(),
          [support](PointId point_id) {
            return point_id != support[0] && point_id != support[1];
          });
      if (extra == partition.shell_ids().end()) {
        throw std::logic_error(
            "an exhaustive extra shell omitted its extra point");
      }
      records.diagnostics.push_back(
          ExactPairSupportExtraShellDiagnostic{
              support,
              *sphere.center(),
              *sphere.squared_level(),
              interior,
              partition.shell_ids().size(),
              *extra,
              interior.size() + 2U,
              partition.closed_rank(),
              partition.exterior_ids().size()});
    }
  }
  return records;
}

void check_complete_accounting(
    const ExactPairSupportStreamResult& result,
    const std::string& message) {
  const auto& audit = result.audit;
  check(
      result.stream_complete() &&
          result.absence_of_additional_pair_supports_certified() &&
          audit.resolved_pair_count == audit.total_pair_count &&
          audit.remaining_frontier_pair_count == 0U &&
          audit.rank_pruned_pair_count +
                  audit.leaf_pair_classification_count ==
              audit.resolved_pair_count &&
          audit.accepted_event_count +
                  audit.relevant_extra_shell_diagnostic_count +
                  audit.above_rank_pair_count ==
              audit.leaf_pair_classification_count &&
          audit.pair_partition_accounting_certified &&
          result.self_product_partition_certified &&
          result.witness_antichains_certified &&
          result.all_rank_prunes_recertified &&
          result.all_rank_relevant_shells_complete &&
          result.no_forbidden_global_structure_materialized &&
          !result.hierarchy_reduction_performed,
      message);
}

void test_exact_phi_aabb_maximum() {
  const ExactDyadicAabb3 first = box(
      {-7.0, -7.0, -7.0}, {-6.0, -6.0, -6.0});
  const ExactDyadicAabb3 second = box(
      {-7.0, -2.0, -7.0}, {-6.0, -1.0, -6.0});
  const ExactDyadicAabb3 query = box(
      {-7.0, -5.0, -7.0}, {-5.0, -4.0, -5.0});
  const auto maximum = exact_diametral_phi_aabb_maximum(
      first, second, query);
  check(
      maximum.maximum_phi == ExactRational{BigInt{5}} &&
          maximum.query_endpoint ==
              std::array<std::uint8_t, 3>{1U, 0U, 1U} &&
          maximum.first_support_endpoint ==
              std::array<std::uint8_t, 3>{0U, 1U, 0U} &&
          maximum.second_support_endpoint ==
              std::array<std::uint8_t, 3>{0U, 0U, 0U},
      "the exact phi bound maximizes each trivariate endpoint box with a canonical witness");

  const ExactDyadicAabb3 left_point = box(
      {-1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0});
  const ExactDyadicAabb3 right_point = box(
      {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
  const ExactDyadicAabb3 inside_point = box(
      {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const ExactDyadicAabb3 shell_point = box(
      {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
  check(
      exact_diametral_phi_aabb_maximum(
          left_point, right_point, inside_point).maximum_phi ==
          ExactRational{BigInt{-1}} &&
          exact_diametral_phi_aabb_maximum(
              left_point, right_point, shell_point).maximum_phi.is_zero(),
      "strict negativity certifies an interior witness while equality remains non-prunable");

  const ExactDyadicAabb3 zero_query = box(
      {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const std::array<ExactDyadicAabb3, 2> correlated_first{
      box({2.0, 1.0, 0.0}, {2.0, 1.0, 0.0}),
      box({1.0, 2.0, 0.0}, {1.0, 2.0, 0.0})};
  const std::array<ExactDyadicAabb3, 2> correlated_second{
      box({1.0, -3.0, 0.0}, {1.0, -3.0, 0.0}),
      box({-3.0, 1.0, 0.0}, {-3.0, 1.0, 0.0})};
  bool every_real_pair_is_strict = true;
  for (const ExactDyadicAabb3& first_real : correlated_first) {
    for (const ExactDyadicAabb3& second_real : correlated_second) {
      every_real_pair_is_strict =
          every_real_pair_is_strict &&
          exact_diametral_phi_aabb_maximum(
              first_real, second_real, zero_query).maximum_phi.sign() < 0;
    }
  }
  const ExactDyadicAabb3 correlated_first_box = box(
      {1.0, 1.0, 0.0}, {2.0, 2.0, 0.0});
  const ExactDyadicAabb3 correlated_second_box = box(
      {-3.0, -3.0, 0.0}, {1.0, 1.0, 0.0});
  check(
      every_real_pair_is_strict &&
          exact_diametral_phi_aabb_maximum(
              correlated_first_box,
              correlated_second_box,
              zero_query).maximum_phi == ExactRational{BigInt{4}},
      "artificial AABB corners cannot certify the absence of a universal witness for correlated real sites");

  ExactDyadicAabb3 reversed = inside_point;
  reversed.lower_binary64_bits[0] = bits(2.0);
  reversed.upper_binary64_bits[0] = bits(1.0);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(exact_diametral_phi_aabb_maximum(
            left_point, right_point, reversed));
      },
      "the exact phi bound rejects a reversed AABB");
}

void test_complete_self_dual_partition_and_long_pair() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(0.0), point(1.0), point(4.0), point(10.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 10U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 10U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 6U &&
          result.relevant_extra_shell_diagnostics.empty() &&
          result.audit.total_pair_count == 6U &&
          result.audit.rank_pruned_pair_count == 0U &&
          result.audit.leaf_pair_classification_count == 6U &&
          result.audit.diagonal_leaf_discard_count == 4U &&
          result.audit.diagonal_product_rank_search_skip_count > 0U &&
          result.audit.global_closed_ball_query_count == 6U &&
          result.audit.point_classification_count == 24U,
      "the self-dual traversal emits every collinear pair exactly once without a global cell arena");
  const auto long_pair = std::find_if(
      result.events.begin(),
      result.events.end(),
      [](const ExactPairSupportEvent& event) {
        return event.support_ids == std::array<PointId, 2>{0U, 3U};
      });
  check(
      long_pair != result.events.end() &&
          long_pair->interior_ids == std::vector<PointId>{1U, 2U} &&
          long_pair->closed_rank == 4U,
      "the direct traversal retains a long pair that a fixed 1-NN list would miss");
  check_complete_accounting(
      result,
      "the complete collinear stream closes its unordered-pair accounting");
  check_fresh_replay(
      index,
      cloud,
      10U,
      budget,
      result,
      "the complete collinear stream passes fresh replay");
}

void test_leaf_rank_cap_and_sparse_queries() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0), point(0.0), point(1.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 2U &&
          result.audit.rank_pruned_pair_count == 0U &&
          result.audit.early_closed_rank_rejection_count == 1U &&
          result.audit.global_closed_ball_query_count == 3U &&
          result.audit.point_classification_count < 9U,
      "the sparse leaf query stops the long rank-three pair as soon as one strict interior is certified at K=1");
  check_complete_accounting(
      result,
      "the rank-capped stream still partitions every unordered pair");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the rank-capped stream passes exact fresh replay");
}

void test_extra_shell_and_equality_descent() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 2U &&
          result.relevant_extra_shell_diagnostics.size() == 1U,
      "the right triangle separates its two regular sides from the hypotenuse extra shell");
  if (result.relevant_extra_shell_diagnostics.size() == 1U) {
    const auto& diagnostic = result.relevant_extra_shell_diagnostics.front();
    check(
        diagnostic.center == ExactRational3{} &&
            diagnostic.squared_level ==
                morsehgp3d::exact::ExactLevel{BigInt{1}} &&
            diagnostic.interior_ids.empty() &&
            diagnostic.shell_count == 3U &&
            diagnostic.minimum_possible_closed_rank == 2U &&
            diagnostic.observed_closed_rank == 3U &&
            diagnostic.exterior_count == 0U &&
            diagnostic.canonical_extra_shell_witness_id !=
                diagnostic.support_ids[0] &&
            diagnostic.canonical_extra_shell_witness_id !=
                diagnostic.support_ids[1],
        "the extra-shell diagnostic stores one canonical witness and a complete count, not the full shell");
  }
  check_complete_accounting(
      result,
      "the extra-shell stream closes all pair categories");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the extra-shell stream passes fresh replay");
}

void test_real_anchor_shell_equality_exclusion() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0),
       point(0.0, -1.0),
       point(0.0, 1.0),
       point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.audit.certified_anchor_shell_tangent_subtree_count == 1U &&
          result.audit.certified_anchor_noninterior_subtree_count == 2U &&
          result.audit.rank_prune_search_count == 2U &&
          result.audit.rank_pruned_pair_count == 0U,
      "a real anchor may exclude an internal witness subtree at exact shell equality without fabricating a rank prune");
  check_complete_accounting(
      result,
      "the anchor-shell equality fixture keeps complete pair accounting");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the anchor-shell equality fixture passes exact fresh replay");
}

void check_budget_stop(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairSupportStreamBudget& budget,
    ExactPairSupportStopReason expected_reason,
    const std::string& message) {
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  check(
      result.status == ExactPairSupportStreamStatus::budget_exhausted &&
          result.stop_reason == expected_reason &&
          !result.stream_complete() &&
          !result.absence_of_additional_pair_supports_certified() &&
          !result.frontier_exhausted &&
          !result.remaining_frontier.empty() &&
          result.audit.resolved_pair_count +
                  result.audit.remaining_frontier_pair_count ==
              result.audit.total_pair_count &&
          result.audit.pair_partition_accounting_certified,
      message);
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      message + " passes fresh replay as an honest partial result");
}

void test_transactional_budgets() {
  const CanonicalPointCloud cloud = cloud_from({point(-1.0), point(1.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();

  ExactPairSupportStreamBudget budget = unlimited_budget();
  budget.maximum_work_unit_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::work_unit_limit,
      "zero work budget preserves the initial self product");

  budget = unlimited_budget();
  budget.maximum_frontier_entry_count = 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::frontier_entry_limit,
      "an undersized product frontier refuses a partial self expansion");

  budget = unlimited_budget();
  budget.maximum_emitted_record_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::emitted_record_limit,
      "zero record capacity leaves the terminal pair uncommitted");

  budget = unlimited_budget();
  budget.maximum_emitted_point_id_reference_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::emitted_point_id_reference_limit,
      "zero PointId capacity cannot be bypassed by a fixed-size support record");

  budget = unlimited_budget();
  budget.maximum_global_closed_ball_query_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::global_closed_ball_query_limit,
      "zero closed-ball capacity leaves the leaf pair atomic");

  budget = unlimited_budget();
  budget.maximum_point_classification_count = cloud.size() - 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::point_classification_limit,
      "a leaf query refuses to start without a full-cloud logical classification budget");

  budget = unlimited_budget();
  budget.maximum_auxiliary_frontier_entry_count = 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::auxiliary_frontier_entry_limit,
      "a sparse query refuses an auxiliary frontier below its certified depth bound");

  check_throws<std::invalid_argument>(
      [&] {
        ExactPairSupportStreamBudget invalid = unlimited_budget();
        invalid.maximum_frontier_entry_count = 0U;
        static_cast<void>(
            build_exact_pair_support_stream(index, cloud, 1U, invalid));
      },
      "a nonempty pair domain rejects an impossible zero-sized persisted frontier");

  const CanonicalPointCloud singleton = cloud_from({point(0.0)});
  const MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  const ExactPairSupportStreamBudget zero_budget{};
  const ExactPairSupportStreamResult singleton_result =
      build_exact_pair_support_stream(
          singleton_index, singleton, 10U, zero_budget);
  check(
      singleton_result.stream_complete() &&
          singleton_result.audit.total_pair_count == 0U &&
          singleton_result.events.empty() &&
          singleton_result.remaining_frontier.empty(),
      "a singleton completes without consuming any pair-stream budget");
  check_fresh_replay(
      singleton_index,
      singleton,
      10U,
      zero_budget,
      singleton_result,
      "the zero-work singleton result passes fresh replay");
  static_cast<void>(maximum);
}

void test_bounded_exhaustive_oracle_agreement() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    const double x = static_cast<double>(index) - 7.0;
    const double y = static_cast<double>((index * index + 3U) % 17U) - 8.0;
    const double z =
        static_cast<double>((index * index * index + 5U) % 19U) - 9.0;
    points.push_back(point(x, y, z));
  }
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  for (const std::size_t maximum_order : {1U, 4U, 9U, 10U}) {
    const ExactPairSupportStreamResult result =
        build_exact_pair_support_stream(
            index, cloud, maximum_order, budget);
    const OraclePairRecords oracle =
        brute_force_pair_records(cloud, maximum_order);
    check(
        result.events == oracle.events &&
            result.relevant_extra_shell_diagnostics == oracle.diagnostics,
        "the direct pair stream agrees with exhaustive closed-ball enumeration at n=14");
    if (maximum_order == 1U) {
      check(
          result.audit.rank_pruned_product_count > 0U &&
              result.audit.rank_pruned_pair_count > 0U &&
              result.audit.exact_anchor_ball_minimum_aabb_bound_count > 0U &&
              result.audit.certified_anchor_noninterior_subtree_count > 0U &&
              result.audit.certified_anchor_noninterior_point_count > 0U,
          "the n=14 K=1 differential exercises strict rank pruning and safe real-anchor witness exclusion");
    }
    check_complete_accounting(
        result,
        "the n=14 differential closes its complete pair partition");
    check_fresh_replay(
        index,
        cloud,
        maximum_order,
        budget,
        result,
        "the n=14 differential passes exact fresh replay");
  }
}

void test_hostile_replay_mutations() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult complete =
      build_exact_pair_support_stream(index, cloud, 2U, budget);
  check(
      !complete.events.empty() &&
          !complete.relevant_extra_shell_diagnostics.empty(),
      "the replay-mutation fixture exposes both record kinds");

  ExactPairSupportStreamResult mutated = complete;
  mutated.budget.maximum_work_unit_count -= 1U;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated embedded budget");

  mutated = complete;
  mutated.events.front().center = ExactRational3{
      BigInt{1}, BigInt{0}, BigInt{0}, BigInt{1}};
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated event center");

  mutated = complete;
  ++mutated.relevant_extra_shell_diagnostics.front().shell_count;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated sparse shell count");

  mutated = complete;
  ++mutated.audit.resolved_pair_count;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects mutated pair accounting");

  mutated = complete;
  mutated.hierarchy_reduction_performed = true;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a fabricated hierarchy reduction claim");

  mutated = complete;
  mutated.no_forbidden_global_structure_materialized = false;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a removed no-mosaic architecture assertion");

  ExactPairSupportStreamBudget partial_budget = unlimited_budget();
  partial_budget.maximum_work_unit_count = 0U;
  const ExactPairSupportStreamResult partial =
      build_exact_pair_support_stream(
          index, cloud, 2U, partial_budget);
  check(
      !partial.remaining_frontier.empty(),
      "the partial replay fixture retains a frontier entry");
  mutated = partial;
  ++mutated.remaining_frontier.front().first_leaf_end;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, partial_budget, mutated).result_certified,
      "fresh replay rejects a mutated residual Morton range");

  mutated = complete;
  mutated.status = ExactPairSupportStreamStatus::budget_exhausted;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a false completion status");
}

}  // namespace

int main() {
  test_exact_phi_aabb_maximum();
  test_complete_self_dual_partition_and_long_pair();
  test_leaf_rank_cap_and_sparse_queries();
  test_extra_shell_and_equality_descent();
  test_real_anchor_shell_equality_exclusion();
  test_transactional_budgets();
  test_bounded_exhaustive_oracle_agreement();
  test_hostile_replay_mutations();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "pair-support stream tests passed\n";
  return 0;
}
