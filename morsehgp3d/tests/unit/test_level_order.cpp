#include "morsehgp3d/exact/level_order.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CanonicalLevelBatchResult;
using morsehgp3d::exact::CanonicalSupportIds;
using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterSupportStatus;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::SupportLevelEmission;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::canonical_level_batches;
using morsehgp3d::exact::compare_exact_levels;
using morsehgp3d::exact::decide_exact_level_order;
using morsehgp3d::exact::exact_level_cross_product_difference;
using morsehgp3d::exact::power_of_two;
using morsehgp3d::exact::support_level_emission_from_analysis;

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
    function();
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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalSupportIds support_ids(
    std::initializer_list<std::uint64_t> identifiers) {
  return CanonicalSupportIds::from_ids(
      std::span<const std::uint64_t>{identifiers.begin(), identifiers.size()});
}

void check_support_ids(
    const CanonicalSupportIds& actual,
    std::initializer_list<std::uint64_t> expected,
    const std::string& message) {
  const std::vector<std::uint64_t> actual_values{
      actual.ids().begin(), actual.ids().end()};
  const std::vector<std::uint64_t> expected_values{expected};
  check(actual_values == expected_values, message);
}

[[nodiscard]] int sign_of_ordering(std::strong_ordering ordering) {
  if (ordering == std::strong_ordering::less) {
    return -1;
  }
  return ordering == std::strong_ordering::greater ? 1 : 0;
}

[[nodiscard]] int sign_of_predicate(PredicateSign sign) {
  return static_cast<int>(sign);
}

[[nodiscard]] int sign_of_bigint(const BigInt& value) {
  if (value < 0) {
    return -1;
  }
  return value == 0 ? 0 : 1;
}

void test_exact_level_comparison_and_counters() {
  const BigInt scale = power_of_two(4096U);
  const ExactLevel lower{scale, scale + 1};
  const ExactLevel upper{scale + 1, scale + 2};

  check(
      exact_level_cross_product_difference(lower, upper) == -1,
      "huge adjacent rational levels have the exact -1 cross witness");
  check(
      exact_level_cross_product_difference(upper, lower) == 1,
      "reversing huge adjacent rational levels has the exact +1 cross witness");
  check(lower < upper, "ExactLevel ordering uses the shared exact witness");

  PredicateCounters counters;
  const auto lower_decision = decide_exact_level_order(lower, upper, &counters);
  check(
      lower_decision.sign() == PredicateSign::negative &&
          lower_decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "decision-only level comparison certifies the negative exact sign");
  check(
      counters.certified_decisions() == 1U &&
          counters.cpu_multiprecision_certified() == 1U &&
          counters.exact_zeros() == 0U,
      "decision-only level comparison records exactly one certification");

  const auto upper_result = compare_exact_levels(upper, lower, &counters);
  check(
      upper_result.decision.sign() == PredicateSign::positive &&
          upper_result.cross_product_difference == 1,
      "diagnostic level comparison retains its positive exact witness");
  check(
      counters.certified_decisions() == 2U &&
          counters.cpu_multiprecision_certified() == 2U &&
          counters.exact_zeros() == 0U,
      "diagnostic level comparison adds exactly one certification");

  const auto equal_result = compare_exact_levels(
      ExactLevel{BigInt{2}, BigInt{4}},
      ExactLevel{BigInt{1}, BigInt{2}},
      &counters);
  check(
      equal_result.decision.sign() == PredicateSign::zero &&
          equal_result.cross_product_difference == 0,
      "different input fractions canonicalize to one exact equal level");
  check(
      counters.certified_decisions() == 3U &&
          counters.cpu_multiprecision_certified() == 3U &&
          counters.exact_zeros() == 1U &&
          counters.remaining_unknown() == 0U,
      "an explicit exact equality records one zero certification");
}

void test_exact_level_order_laws() {
  const BigInt large = power_of_two(1024U);
  const std::array<ExactLevel, 7> levels{
      ExactLevel{},
      ExactLevel{BigInt{1}, large + 1},
      ExactLevel{BigInt{1}, BigInt{3}},
      ExactLevel{BigInt{2}, BigInt{6}},
      ExactLevel{large, large + 1},
      ExactLevel{large + 1, large + 2},
      ExactLevel{large}};

  for (std::size_t left = 0; left < levels.size(); ++left) {
    for (std::size_t right = 0; right < levels.size(); ++right) {
      const auto result = compare_exact_levels(levels[left], levels[right]);
      const auto reverse = compare_exact_levels(levels[right], levels[left]);
      const int predicate = sign_of_predicate(result.decision.sign());
      const int ordering = sign_of_ordering(levels[left] <=> levels[right]);
      check(
          predicate == ordering,
          "instrumented comparison agrees with ExactLevel strong ordering");
      check(
          predicate == -sign_of_predicate(reverse.decision.sign()),
          "exact level comparison is antisymmetric");
      check(
          sign_of_bigint(result.cross_product_difference) == predicate,
          "cross-product witness agrees with its certified sign");
    }
  }

  for (const ExactLevel& first : levels) {
    for (const ExactLevel& second : levels) {
      for (const ExactLevel& third : levels) {
        if (first <= second && second <= third) {
          check(first <= third, "exact level ordering is transitive");
        }
      }
    }
  }
}

void test_canonical_support_ids() {
  const CanonicalSupportIds unordered = support_ids({9U, 2U, 5U, 1U});
  check_support_ids(
      unordered, {1U, 2U, 5U, 9U}, "support identifiers sort numerically");
  check(unordered.size() == 4U, "support size is retained");
  check(unordered.id(2U) == 5U, "canonical support indexed access");
  check(unordered.contains(2U) && !unordered.contains(3U),
        "canonical support membership");

  const CanonicalSupportIds singleton = support_ids({99U});
  const CanonicalSupportIds pair = support_ids({0U, 1U});
  check(
      singleton < pair,
      "support tie-break compares size before identifiers");
  check(
      support_ids({1U, 8U}) < support_ids({2U, 3U}),
      "equal-size supports compare identifiers lexicographically");

  const std::uint64_t maximum = CanonicalSupportIds::maximum_point_id;
  check_support_ids(
      support_ids({maximum}),
      {maximum},
      "the largest exactly representable v2 PointId is accepted");
  check_throws<std::out_of_range>(
      [] {
        static_cast<void>(support_ids(
            {CanonicalSupportIds::maximum_point_id + std::uint64_t{1}}));
      },
      "PointId 2^53 is rejected");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(support_ids({})); },
      "empty support rejection");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(support_ids({0U, 1U, 2U, 3U, 4U})); },
      "support larger than four rejection");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(support_ids({4U, 1U, 4U})); },
      "duplicate support identifier rejection after sorting");
  check_throws<std::out_of_range>(
      [&unordered] { static_cast<void>(unordered.id(4U)); },
      "support indexed access rejects the first unused slot");
}

void test_support_emission_invariants() {
  const SupportLevelEmission emission = SupportLevelEmission::create(
      ExactLevel{BigInt{3}, BigInt{4}},
      support_ids({7U, 2U}),
      support_ids({9U, 2U, 7U}));
  check_support_ids(
      emission.minimal_support_ids(),
      {2U, 7U},
      "emission retains a canonical minimal support");
  check_support_ids(
      emission.source_support_ids(),
      {2U, 7U, 9U},
      "emission retains canonical source provenance");
  check(
      emission.squared_level() == ExactLevel{BigInt{3}, BigInt{4}},
      "emission retains its exact level");

  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(SupportLevelEmission::create(
            ExactLevel{}, support_ids({1U, 3U}), support_ids({1U, 2U})));
      },
      "minimal support outside its source is rejected");
}

void test_emission_from_local_support_analysis() {
  const std::array<CertifiedPoint3, 1> singleton{
      point(4.0, -2.0, 1.0)};
  const auto singleton_analysis = analyze_circumcenter_support(singleton);
  const std::array<std::uint64_t, 1> singleton_ids{11U};
  const SupportLevelEmission singleton_emission =
      support_level_emission_from_analysis(
          singleton_ids, singleton_analysis);
  check_support_ids(
      singleton_emission.minimal_support_ids(),
      {11U},
      "singleton analysis exports its one canonical support identifier");
  check(
      singleton_emission.squared_level() == ExactLevel{},
      "singleton analysis exports the exact zero level");

  const std::array<CertifiedPoint3, 2> pair{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const auto pair_analysis = analyze_circumcenter_support(pair);
  check(
      pair_analysis.status() == CircumcenterSupportStatus::minimal,
      "diameter pair is a minimal local support sphere");
  const std::array<std::uint64_t, 2> pair_ids{7U, 3U};
  const SupportLevelEmission pair_emission =
      support_level_emission_from_analysis(pair_ids, pair_analysis);
  check_support_ids(
      pair_emission.minimal_support_ids(),
      {3U, 7U},
      "minimal analysis uses its complete canonical support");
  check_support_ids(
      pair_emission.source_support_ids(),
      {3U, 7U},
      "minimal analysis retains source provenance");
  check(
      pair_emission.squared_level() == ExactLevel{BigInt{1}},
      "minimal analysis exports its exact squared level");

  const std::array<CertifiedPoint3, 3> right_triangle{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
  const auto boundary_analysis = analyze_circumcenter_support(right_triangle);
  check(
      boundary_analysis.status() ==
          CircumcenterSupportStatus::boundary_reduced,
      "right triangle has a certified boundary reduction");
  const std::array<std::uint64_t, 3> positional_ids{9U, 2U, 5U};
  const SupportLevelEmission boundary_emission =
      support_level_emission_from_analysis(
          positional_ids, boundary_analysis);
  check_support_ids(
      boundary_emission.minimal_support_ids(),
      {2U, 5U},
      "reduction mask is applied to positions before identifier sorting");
  check_support_ids(
      boundary_emission.source_support_ids(),
      {2U, 5U, 9U},
      "boundary reduction retains the full sorted source provenance");
  check(
      boundary_emission.squared_level() == ExactLevel{BigInt{2}},
      "boundary reduction retains the unchanged exact sphere level");

  std::array<std::size_t, 3> permutation{0U, 1U, 2U};
  do {
    std::array<CertifiedPoint3, 3> permuted_points{
        right_triangle[permutation[0]],
        right_triangle[permutation[1]],
        right_triangle[permutation[2]]};
    std::array<std::uint64_t, 3> permuted_ids{
        positional_ids[permutation[0]],
        positional_ids[permutation[1]],
        positional_ids[permutation[2]]};
    const auto permuted_analysis =
        analyze_circumcenter_support(permuted_points);
    const SupportLevelEmission permuted_emission =
        support_level_emission_from_analysis(
            permuted_ids, permuted_analysis);
    check(
        permuted_emission == boundary_emission,
        "paired point and identifier permutations preserve reduced emission");
  } while (std::next_permutation(permutation.begin(), permutation.end()));

  const std::array<CertifiedPoint3, 4> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const auto tetrahedron_analysis =
      analyze_circumcenter_support(tetrahedron);
  const std::array<std::uint64_t, 4> tetrahedron_ids{8U, 2U, 6U, 4U};
  const SupportLevelEmission tetrahedron_emission =
      support_level_emission_from_analysis(
          tetrahedron_ids, tetrahedron_analysis);
  check_support_ids(
      tetrahedron_emission.minimal_support_ids(),
      {2U, 4U, 6U, 8U},
      "well-centered tetrahedron exports its complete canonical support");
  check(
      tetrahedron_emission.squared_level() == ExactLevel{BigInt{3}},
      "well-centered tetrahedron exports its exact squared level");

  const std::array<CertifiedPoint3, 3> obtuse_triangle{
      point(0.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0)};
  const auto exterior_analysis = analyze_circumcenter_support(obtuse_triangle);
  check(
      exterior_analysis.status() ==
          CircumcenterSupportStatus::exterior_circumcenter,
      "obtuse triangle exposes only an exterior circumcentre");
  check_throws<std::invalid_argument>(
      [&exterior_analysis] {
        const std::array<std::uint64_t, 3> ids{0U, 1U, 2U};
        static_cast<void>(
            support_level_emission_from_analysis(ids, exterior_analysis));
      },
      "exterior circumcentre is not promoted to a local support sphere item");

  const std::array<CertifiedPoint3, 2> duplicate_pair{
      point(0.0, 0.0, 0.0), point(0.0, 0.0, 0.0)};
  const auto dependent_analysis = analyze_circumcenter_support(duplicate_pair);
  check(
      dependent_analysis.status() ==
          CircumcenterSupportStatus::affinely_dependent,
      "duplicate pair analysis is affinely dependent");
  check_throws<std::invalid_argument>(
      [&dependent_analysis] {
        const std::array<std::uint64_t, 2> ids{0U, 1U};
        static_cast<void>(
            support_level_emission_from_analysis(ids, dependent_analysis));
      },
      "dependent support is not promoted to a local support sphere item");

  check_throws<std::invalid_argument>(
      [&boundary_analysis] {
        const std::array<std::uint64_t, 2> too_few_ids{0U, 1U};
        static_cast<void>(support_level_emission_from_analysis(
            too_few_ids, boundary_analysis));
      },
      "analysis and positional identifier counts must match");
  check_throws<std::invalid_argument>(
      [&boundary_analysis] {
        const std::array<std::uint64_t, 3> duplicate_ids{9U, 2U, 2U};
        static_cast<void>(support_level_emission_from_analysis(
            duplicate_ids, boundary_analysis));
      },
      "source provenance rejects duplicate identifiers");
}

[[nodiscard]] std::vector<SupportLevelEmission> batch_fixture_emissions() {
  const ExactLevel half{BigInt{1}, BigInt{2}};
  const CanonicalSupportIds reduced = support_ids({9U, 5U});
  const SupportLevelEmission triangle_source = SupportLevelEmission::create(
      half, reduced, support_ids({9U, 2U, 5U}));
  const SupportLevelEmission duplicate_triangle_source =
      SupportLevelEmission::create(
          ExactLevel{BigInt{2}, BigInt{4}},
          reduced,
          support_ids({2U, 5U, 9U}));
  const SupportLevelEmission pair_source = SupportLevelEmission::create(
      half, reduced, reduced);
  const SupportLevelEmission distinct_equal_level =
      SupportLevelEmission::create(
          half, support_ids({7U, 1U}), support_ids({1U, 7U}));
  const SupportLevelEmission later = SupportLevelEmission::create(
      ExactLevel{BigInt{3}, BigInt{4}},
      support_ids({0U}),
      support_ids({0U}));
  return {
      later,
      triangle_source,
      distinct_equal_level,
      pair_source,
      duplicate_triangle_source};
}

void check_batch_fixture(const CanonicalLevelBatchResult& result) {
  check(result.emission_count == 5U, "batch result counts all emissions");
  check(
      result.unique_emission_count == 4U,
      "batch result counts unique level/minimal/source emissions");
  check(
      result.duplicate_emission_count == 1U,
      "batch result counts an exact duplicate emission");
  check(
      result.unique_emission_count + result.duplicate_emission_count ==
          result.emission_count,
      "batch summary counters form an exact partition");
  check(result.batches.size() == 2U, "exact equality forms two level batches");
  if (result.batches.size() != 2U) {
    return;
  }

  const auto& half_batch = result.batches[0];
  check(
      half_batch.squared_level == ExactLevel{BigInt{1}, BigInt{2}} &&
          half_batch.emission_count == 4U,
      "first exact batch groups canonical 1/2 representations");
  check(
      half_batch.supports.size() == 2U,
      "distinct supports at one exact level remain distinct");
  if (half_batch.supports.size() == 2U) {
    check_support_ids(
        half_batch.supports[0].minimal_support_ids,
        {1U, 7U},
        "equal-level supports use the canonical support tie-break");
    check(
        half_batch.supports[0].emission_count == 1U,
        "distinct equal-level support has one emission");

    const auto& reduced = half_batch.supports[1];
    check_support_ids(
        reduced.minimal_support_ids,
        {5U, 9U},
        "reduced support follows the lexicographically earlier peer");
    check(
        reduced.emission_count == 3U &&
            reduced.source_provenance.size() == 2U,
        "one reduced support aggregates all source provenance");
    if (reduced.source_provenance.size() == 2U) {
      check_support_ids(
          reduced.source_provenance[0].source_support_ids,
          {5U, 9U},
          "smaller source support sorts first in provenance");
      check(
          reduced.source_provenance[0].emission_count == 1U,
          "pair source provenance has one emission");
      check_support_ids(
          reduced.source_provenance[1].source_support_ids,
          {2U, 5U, 9U},
          "larger boundary source remains explicit provenance");
      check(
          reduced.source_provenance[1].emission_count == 2U,
          "duplicate boundary source emissions retain multiplicity");
    }
  }

  const auto& later_batch = result.batches[1];
  check(
      later_batch.squared_level == ExactLevel{BigInt{3}, BigInt{4}} &&
          later_batch.emission_count == 1U &&
          later_batch.supports.size() == 1U,
      "strictly later exact level remains a separate batch");
}

void test_canonical_level_batches() {
  const std::vector<SupportLevelEmission> emissions =
      batch_fixture_emissions();
  const CanonicalLevelBatchResult expected = canonical_level_batches(emissions);
  check_batch_fixture(expected);

  std::array<std::size_t, 5> permutation{0U, 1U, 2U, 3U, 4U};
  std::size_t permutations_checked = 0U;
  do {
    std::vector<SupportLevelEmission> permuted;
    permuted.reserve(permutation.size());
    for (const std::size_t index : permutation) {
      permuted.push_back(emissions[index]);
    }
    check(
        canonical_level_batches(permuted) == expected,
        "all arrival orders produce the same canonical level batches");
    ++permutations_checked;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutations_checked == 120U, "all five-item permutations were checked");

  const std::vector<SupportLevelEmission> empty;
  const CanonicalLevelBatchResult empty_result = canonical_level_batches(empty);
  check(
      empty_result.batches.empty() && empty_result.emission_count == 0U &&
          empty_result.unique_emission_count == 0U &&
          empty_result.duplicate_emission_count == 0U,
      "empty input has an empty canonical batch result and zero counters");

  const CanonicalSupportIds shared_minimal = support_ids({3U, 8U});
  const std::vector<SupportLevelEmission> contradictory{
      SupportLevelEmission::create(
          ExactLevel{BigInt{1}, BigInt{2}},
          shared_minimal,
          shared_minimal),
      SupportLevelEmission::create(
          ExactLevel{BigInt{3}, BigInt{4}},
          shared_minimal,
          support_ids({1U, 3U, 8U}))};
  check_throws<std::invalid_argument>(
      [&contradictory] {
        static_cast<void>(canonical_level_batches(contradictory));
      },
      "one canonical minimal support at different levels is rejected");
}

}  // namespace

int main() {
  test_exact_level_comparison_and_counters();
  test_exact_level_order_laws();
  test_canonical_support_ids();
  test_support_emission_invariants();
  test_emission_from_local_support_analysis();
  test_canonical_level_batches();

  if (failures != 0) {
    std::cerr << failures << " exact level-order test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D exact level-order tests passed\n";
  return 0;
}
