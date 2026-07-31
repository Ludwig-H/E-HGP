#include "morsehgp3d/hierarchy/capped_distinct_point_coverage.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::hierarchy::CappedDistinctPointCoverage;
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
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::vector<PointId> expected_smallest(
    const std::set<PointId>& reference,
    std::size_t threshold) {
  std::vector<PointId> expected;
  expected.reserve(std::min(reference.size(), threshold));
  for (const PointId point_id : reference) {
    if (expected.size() == threshold) {
      break;
    }
    expected.push_back(point_id);
  }
  return expected;
}

void check_against_reference(
    const CappedDistinctPointCoverage& summary,
    const std::set<PointId>& reference,
    const std::string& label) {
  const std::vector<PointId> expected =
      expected_smallest(reference, summary.threshold());
  const std::span<const PointId> observed =
      summary.smallest_distinct_point_ids();
  check(
      std::equal(
          observed.begin(), observed.end(),
          expected.begin(), expected.end()),
      label + ": retained identifiers are not the canonical smallest set");
  check(
      summary.retained_distinct_count() == expected.size(),
      label + ": retained count differs from the reference");
  check(
      summary.coverage_size_at_least_threshold() ==
          (reference.size() >= summary.threshold()),
      label + ": threshold predicate differs from exact std::set cardinality");
  const std::optional<std::size_t> exact =
      summary.exact_coverage_size_if_below_threshold();
  if (reference.size() < summary.threshold()) {
    check(
        exact == std::optional{reference.size()},
        label + ": an under-threshold summary lost its exact cardinality");
  } else {
    check(
        !exact.has_value(),
        label + ": a saturated summary claimed an exact cardinality");
  }
}

void test_positive_runtime_threshold_and_non_claims() {
  check_throws<std::invalid_argument>(
      []() {
        static_cast<void>(CappedDistinctPointCoverage{0U});
      },
      "threshold zero must be rejected");

  const CappedDistinctPointCoverage summary{20U};
  check(
      summary.threshold() == 20U &&
          summary.retained_distinct_count() == 0U &&
          !summary.coverage_size_at_least_threshold() &&
          summary.exact_coverage_size_if_below_threshold() ==
              std::optional<std::size_t>{0U},
      "a positive runtime threshold did not create the empty exact summary");
  check(
      CappedDistinctPointCoverage::backend == "reference_cpu" &&
          CappedDistinctPointCoverage::profile == "hgp_reduced" &&
          CappedDistinctPointCoverage::mode ==
              "downstream_capped_distinct_point_coverage" &&
          CappedDistinctPointCoverage::relation == "at_least" &&
          CappedDistinctPointCoverage::public_status == "not_claimed" &&
          !CappedDistinctPointCoverage::source_pruning_authority &&
          !CappedDistinctPointCoverage::hierarchy_authority &&
          !CappedDistinctPointCoverage::public_exactness_claimed,
      "the isolated downstream primitive exposed excessive authority");
}

void test_nineteen_to_twenty_after_one_coverage_delta() {
  CappedDistinctPointCoverage summary{20U};
  std::set<PointId> reference;
  for (PointId point_id = 0U; point_id < 19U; ++point_id) {
    summary.insert_point_id(point_id);
    reference.insert(point_id);
  }
  check_against_reference(summary, reference, "nineteen before batch");
  check(
      !summary.coverage_size_at_least_threshold(),
      "a component with nineteen distinct points became visible too early");

  const std::vector<std::vector<PointId>> added_facets{
      {18U, 19U, 19U}, {7U, 18U}, {19U, 7U}};
  const std::vector<PointId> added_points{19U, 19U, 18U};
  summary.insert_coverage_delta(
      std::span<const std::vector<PointId>>{added_facets},
      std::span<const PointId>{added_points});
  for (const auto& facet : added_facets) {
    reference.insert(facet.begin(), facet.end());
  }
  reference.insert(added_points.begin(), added_points.end());

  check_against_reference(summary, reference, "twenty after complete delta");
  check(
      summary.coverage_size_at_least_threshold() &&
          !summary.exact_coverage_size_if_below_threshold().has_value(),
      "the 19-to-20 batch crossing did not expose only the exact "
      "at-least relation");
}

void test_duplicates_overlaps_and_facets() {
  CappedDistinctPointCoverage twenty_copies{20U};
  std::vector<PointId> same_point(20U, 42U);
  twenty_copies.insert_facet(same_point);
  const std::set<PointId> one_distinct_point{42U};
  check_against_reference(
      twenty_copies,
      one_distinct_point,
      "twenty occurrences of one PointId");
  check(
      !twenty_copies.coverage_size_at_least_threshold(),
      "twenty duplicate occurrences were mistaken for twenty points");

  CappedDistinctPointCoverage duplicates{4U};
  const std::array<PointId, 7U> repeated{
      7U, 7U, 7U, 3U, 3U, 7U, 3U};
  duplicates.insert_facet(repeated);
  const std::set<PointId> duplicate_reference{3U, 7U};
  check_against_reference(
      duplicates, duplicate_reference, "duplicate occurrences");

  CappedDistinctPointCoverage first{20U};
  CappedDistinctPointCoverage second{20U};
  std::set<PointId> union_reference;
  for (PointId point_id = 0U; point_id < 16U; ++point_id) {
    first.insert_point_id(point_id);
    union_reference.insert(point_id);
  }
  for (PointId point_id = 10U; point_id < 31U; ++point_id) {
    second.insert_point_id(point_id);
    union_reference.insert(point_id);
  }
  first.merge_from(second);
  check_against_reference(
      first, union_reference, "overlapping component summaries");

  const CappedDistinctPointCoverage before_mismatch = first;
  const CappedDistinctPointCoverage wrong_threshold{19U};
  check_throws<std::invalid_argument>(
      [&]() {
        first.merge_from(wrong_threshold);
      },
      "merging summaries with different thresholds must fail closed");
  check(
      first == before_mismatch,
      "a rejected threshold mismatch mutated the destination summary");
}

void test_permutations_associativity_and_idempotence() {
  std::vector<PointId> values{
      40U, 2U, 7U, 2U, 99U, 1U, 5U, 40U, 3U, 8U, 6U,
      std::numeric_limits<PointId>::max(), 4U, 11U, 10U, 9U};
  CappedDistinctPointCoverage canonical{9U};
  for (const PointId point_id : values) {
    canonical.insert_point_id(point_id);
  }

  std::mt19937_64 random{0xC0FFEE1234567890ULL};
  for (std::size_t repetition = 0U; repetition < 128U; ++repetition) {
    std::shuffle(values.begin(), values.end(), random);
    CappedDistinctPointCoverage permuted{9U};
    for (const PointId point_id : values) {
      permuted.insert_point_id(point_id);
    }
    check(
        permuted == canonical,
        "an insertion permutation changed the canonical capped summary");
  }

  CappedDistinctPointCoverage first{5U};
  CappedDistinctPointCoverage second{5U};
  CappedDistinctPointCoverage third{5U};
  for (const PointId value : std::array<PointId, 5U>{50U, 2U, 4U, 8U, 2U}) {
    first.insert_point_id(value);
  }
  for (const PointId value : std::array<PointId, 4U>{3U, 4U, 60U, 3U}) {
    second.insert_point_id(value);
  }
  for (const PointId value : std::array<PointId, 4U>{1U, 7U, 8U, 70U}) {
    third.insert_point_id(value);
  }

  CappedDistinctPointCoverage left = first;
  left.merge_from(second);
  left.merge_from(third);
  CappedDistinctPointCoverage right = second;
  right.merge_from(third);
  CappedDistinctPointCoverage associated = first;
  associated.merge_from(right);
  check(
      left == associated,
      "summary merge is not associative");

  CappedDistinctPointCoverage commuted = third;
  commuted.merge_from(second);
  commuted.merge_from(first);
  check(
      commuted == left,
      "summary merge order changed the canonical result");

  const CappedDistinctPointCoverage stable = left;
  left.merge_from(left);
  left.merge_from(stable);
  check(
      left == stable,
      "summary merge is not idempotent");
}

void test_maximum_point_id_is_not_a_sentinel() {
  constexpr PointId maximum = std::numeric_limits<PointId>::max();
  const std::array<PointId, 6U> input{
      maximum, maximum - 1U, maximum - 2U,
      maximum, maximum - 1U, maximum};
  CappedDistinctPointCoverage summary{3U};
  summary.insert_facet(input);
  const std::set<PointId> reference{
      maximum - 2U, maximum - 1U, maximum};
  check_against_reference(summary, reference, "maximum PointId");

  summary.insert_point_id(0U);
  const std::set<PointId> extended{
      0U, maximum - 2U, maximum - 1U, maximum};
  check_against_reference(
      summary, extended, "maximum PointId after capped replacement");
}

void test_move_keeps_source_reusable_and_destination_canonical() {
  CappedDistinctPointCoverage source{3U};
  source.insert_point_id(7U);
  source.insert_point_id(2U);
  CappedDistinctPointCoverage destination{1U};
  destination = std::move(source);
  const std::array<PointId, 2U> expected_destination{2U, 7U};
  const std::span<const PointId> destination_ids =
      destination.smallest_distinct_point_ids();
  check(
      destination.threshold() == 3U &&
          std::equal(
              destination_ids.begin(), destination_ids.end(),
              expected_destination.begin(), expected_destination.end()) &&
          source.threshold() == 3U &&
          source.retained_distinct_count() == 0U,
      "move assignment did not preserve a reusable empty source");
  source.insert_point_id(11U);
  CappedDistinctPointCoverage moved{std::move(source)};
  const std::array<PointId, 1U> expected_moved{11U};
  const std::span<const PointId> moved_ids =
      moved.smallest_distinct_point_ids();
  check(
      moved.threshold() == 3U &&
          std::equal(
              moved_ids.begin(), moved_ids.end(),
              expected_moved.begin(), expected_moved.end()) &&
          source.threshold() == 3U &&
          source.retained_distinct_count() == 0U,
      "move construction did not preserve a reusable empty source");
}

void test_differential_against_std_set() {
  std::mt19937_64 random{0x5E7C0A6E5A11D1FFULL};
  for (std::size_t trial = 0U; trial < 192U; ++trial) {
    const std::size_t threshold = trial % 31U + 1U;
    CappedDistinctPointCoverage direct{threshold};
    std::array<CappedDistinctPointCoverage, 3U> partitions{
        CappedDistinctPointCoverage{threshold},
        CappedDistinctPointCoverage{threshold},
        CappedDistinctPointCoverage{threshold}};
    std::set<PointId> reference;

    for (std::size_t sample = 0U; sample < 1024U; ++sample) {
      const PointId point_id =
          sample % 127U == 0U
              ? std::numeric_limits<PointId>::max()
              : static_cast<PointId>(random() % 389U);
      direct.insert_point_id(point_id);
      partitions[sample % partitions.size()].insert_point_id(point_id);
      reference.insert(point_id);
      if (sample % 113U == 0U) {
        check_against_reference(
            direct, reference, "incremental std::set differential");
      }
    }
    check_against_reference(
        direct, reference, "final std::set differential");

    CappedDistinctPointCoverage left = partitions[0U];
    left.merge_from(partitions[1U]);
    left.merge_from(partitions[2U]);
    CappedDistinctPointCoverage right = partitions[1U];
    right.merge_from(partitions[2U]);
    CappedDistinctPointCoverage associated = partitions[0U];
    associated.merge_from(right);
    check(
        left == direct && associated == direct,
        "partitioned merge differs from direct std::set differential");
  }
}

}  // namespace

int main() {
  test_positive_runtime_threshold_and_non_claims();
  test_nineteen_to_twenty_after_one_coverage_delta();
  test_duplicates_overlaps_and_facets();
  test_permutations_associativity_and_idempotence();
  test_maximum_point_id_is_not_a_sentinel();
  test_move_keeps_source_reusable_and_destination_canonical();
  test_differential_against_std_set();

  if (failures != 0) {
    std::cerr << failures
              << " capped distinct-point coverage test(s) failed\n";
    return 1;
  }
  std::cout
      << "capped distinct-point coverage standalone tests passed\n";
  return 0;
}
