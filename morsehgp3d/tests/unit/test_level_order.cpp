#include "morsehgp3d/exact/level_order.hpp"

#include <algorithm>
#include <array>
#include <cfenv>
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

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <xmmintrin.h>
#endif

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CanonicalLevelBatchResult;
using morsehgp3d::exact::CanonicalSupportIds;
using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterSupportStatus;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateFilterPolicy;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::SupportLevelEmission;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::canonical_level_batches;
using morsehgp3d::exact::compare_exact_levels;
using morsehgp3d::exact::compare_support_levels;
using morsehgp3d::exact::decide_exact_level_order;
using morsehgp3d::exact::exact_level_cross_product_difference;
using morsehgp3d::exact::power_of_two;
using morsehgp3d::exact::support_level_emission_from_analysis;

int failures = 0;

class ScopedFloatingEnvironment {
 public:
  ScopedFloatingEnvironment() noexcept
      : saved_(std::fegetenv(&environment_) == 0) {
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    mxcsr_ = _mm_getcsr();
#endif
  }

  ScopedFloatingEnvironment(const ScopedFloatingEnvironment&) = delete;
  ScopedFloatingEnvironment& operator=(const ScopedFloatingEnvironment&) =
      delete;

  ~ScopedFloatingEnvironment() {
    if (saved_) {
      static_cast<void>(std::fesetenv(&environment_));
    }
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    _mm_setcsr(mxcsr_);
#endif
  }

  [[nodiscard]] bool saved() const noexcept { return saved_; }

 private:
  std::fenv_t environment_{};
  bool saved_;
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  unsigned int mxcsr_{};
#endif
};

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

void check_single_terminal(
    const PredicateCounters& counters,
    CertificationStage expected_stage,
    std::uint64_t expected_zeros,
    const std::string& message) {
  check(
      counters.certified_decisions() == 1U &&
          counters.fp64_filtered_certified() ==
              (expected_stage == CertificationStage::fp64_filtered ? 1U : 0U) &&
          counters.expansion_certified() ==
              (expected_stage == CertificationStage::expansion ? 1U : 0U) &&
          counters.cpu_multiprecision_certified() ==
              (expected_stage == CertificationStage::cpu_multiprecision
                   ? 1U
                   : 0U) &&
          counters.exact_zeros() == expected_zeros &&
          counters.remaining_unknown() == 0U,
      message);
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

[[nodiscard]] ExactRational3 exact_point(
    long long x, long long y, long long z) {
  return ExactRational3{BigInt{x}, BigInt{y}, BigInt{z}, BigInt{1}};
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

void test_adaptive_support_array_level_order() {
  const std::array<CertifiedPoint3, 1> singleton{
      point(0.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> unit_pair{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> equal_unit_pair{
      point(0.0, 0.0, 0.0), point(2.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> radius_two_pair{
      point(-2.0, 0.0, 0.0), point(2.0, 0.0, 0.0)};

  PredicateCounters negative_counters;
  const auto negative = compare_support_levels(
      singleton,
      unit_pair,
      &negative_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      negative.decision.sign() == PredicateSign::negative &&
          negative.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative.cross_product_difference == -1,
      "support levels 0 and 1 have an FP64-certified negative witness");
  check_single_terminal(
      negative_counters,
      CertificationStage::fp64_filtered,
      0U,
      "a strict negative array comparison records one FP64 terminal");

  PredicateCounters positive_counters;
  const auto positive = compare_support_levels(
      unit_pair,
      singleton,
      &positive_counters,
      PredicateFilterPolicy::allow_fp64);
  check(
      positive.decision.sign() == PredicateSign::positive &&
          positive.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          positive.cross_product_difference == 1,
      "reversed support levels have an FP64-certified positive witness");
  check_single_terminal(
      positive_counters,
      CertificationStage::fp64_filtered,
      0U,
      "a strict positive array comparison records one FP64 terminal");

  PredicateCounters equality_counters;
  const auto equality = compare_support_levels(
      unit_pair,
      equal_unit_pair,
      &equality_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      equality.decision.sign() == PredicateSign::zero &&
          equality.decision.certification_stage() ==
              CertificationStage::expansion &&
          equality.cross_product_difference == 0,
      "equal levels from distinct supports are certified by expansion");
  check_single_terminal(
      equality_counters,
      CertificationStage::expansion,
      1U,
      "an equal array comparison records one expansion zero");

  PredicateCounters fp64_only_equality_counters;
  const auto fp64_only_equality = compare_support_levels(
      unit_pair,
      equal_unit_pair,
      &fp64_only_equality_counters,
      PredicateFilterPolicy::allow_fp64);
  check(
      fp64_only_equality.decision.sign() == PredicateSign::zero &&
          fp64_only_equality.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          fp64_only_equality.cross_product_difference ==
              equality.cross_product_difference,
      "the FP64-only policy falls back to MP for equal support levels");
  check_single_terminal(
      fp64_only_equality_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      "the FP64-only equality records one MP zero");

  PredicateCounters multiprecision_counters;
  const auto multiprecision = compare_support_levels(
      singleton,
      unit_pair,
      &multiprecision_counters,
      PredicateFilterPolicy::multiprecision_only);
  check(
      multiprecision.decision.sign() == negative.decision.sign() &&
          multiprecision.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          multiprecision.cross_product_difference ==
              negative.cross_product_difference,
      "MP-only array comparison retains the strict homogeneous witness");
  check_single_terminal(
      multiprecision_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "MP-only array comparison records exactly one terminal");

  const std::array<CertifiedPoint3, 3> right_triangle{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
  const std::array<CertifiedPoint3, 4> regular_tetrahedron{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const auto triangle_over_pair =
      compare_support_levels(right_triangle, unit_pair);
  const auto tetrahedron_over_triangle =
      compare_support_levels(regular_tetrahedron, right_triangle);
  check(
      triangle_over_pair.decision.sign() == PredicateSign::positive &&
          triangle_over_pair.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          tetrahedron_over_triangle.decision.sign() ==
              PredicateSign::positive &&
          tetrahedron_over_triangle.decision.certification_stage() ==
              CertificationStage::fp64_filtered,
      "triangle and tetrahedron support polynomials order levels 2 and 3 by FP64");

  const std::array<CertifiedPoint3, 2> reversed_unit_pair{
      unit_pair[1], unit_pair[0]};
  const std::array<CertifiedPoint3, 2> reversed_radius_two_pair{
      radius_two_pair[1], radius_two_pair[0]};
  const std::array<std::array<CertifiedPoint3, 2>, 2> unit_permutations{
      unit_pair, reversed_unit_pair};
  const std::array<std::array<CertifiedPoint3, 2>, 2> radius_permutations{
      radius_two_pair, reversed_radius_two_pair};
  for (const auto& left : unit_permutations) {
    for (const auto& right : radius_permutations) {
      PredicateCounters counters;
      const auto result = compare_support_levels(left, right, &counters);
      check(
          result.decision.sign() == PredicateSign::negative &&
              result.decision.certification_stage() ==
                  CertificationStage::fp64_filtered &&
              result.cross_product_difference == -3,
          "support permutations preserve the strict level witness and FP64 stage");
      check_single_terminal(
          counters,
          CertificationStage::fp64_filtered,
          0U,
          "each permuted array comparison records one FP64 terminal");
    }
  }

  check_throws<std::invalid_argument>(
      [&unit_pair] {
        const std::array<CertifiedPoint3, 3> dependent{
            point(0.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0),
            point(2.0, 0.0, 0.0)};
        static_cast<void>(compare_support_levels(dependent, unit_pair));
      },
      "array level comparison rejects an affinely dependent support");
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
  check(
      emission.binary64_level_support().empty(),
      "a structural exact-level emission cannot forge binary64 provenance");

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
  check(
      singleton_emission.binary64_level_support().size() == 1U &&
          singleton_emission.binary64_level_support()[0].exact() ==
              singleton[0].exact(),
      "binary singleton analysis exports its minimal level provenance");

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
  check(
      pair_emission.binary64_level_support().size() == 2U,
      "binary pair analysis exports its complete minimal level provenance");

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
  check(
      boundary_emission.binary64_level_support().size() == 2U &&
          boundary_emission.binary64_level_support()[0].exact() ==
              right_triangle[1].exact() &&
          boundary_emission.binary64_level_support()[1].exact() ==
              right_triangle[2].exact(),
      "boundary emission reduces binary64 provenance to the positive minimal support");

  const std::array<ExactRational3, 3> rational_right_triangle{
      exact_point(0, 0, 0),
      exact_point(2, 0, 0),
      exact_point(0, 2, 0)};
  const auto rational_boundary_analysis =
      analyze_circumcenter_support(rational_right_triangle);
  const SupportLevelEmission rational_boundary_emission =
      support_level_emission_from_analysis(
          positional_ids, rational_boundary_analysis);
  check(
      rational_boundary_emission == boundary_emission &&
          rational_boundary_emission.binary64_level_support().empty(),
      "rational and binary analyses emit the same science but only binary "
      "input has fast provenance");

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
  check(
      tetrahedron_emission.binary64_level_support().size() == 4U,
      "well-centered binary tetrahedron exports four provenance points");

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

void test_adaptive_emission_level_order() {
  const std::array<CertifiedPoint3, 1> singleton{
      point(0.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> unit_pair{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> equal_unit_pair{
      point(0.0, 0.0, 0.0), point(2.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> radius_two_pair{
      point(-2.0, 0.0, 0.0), point(2.0, 0.0, 0.0)};

  const std::array<std::uint64_t, 1> singleton_ids{10U};
  const std::array<std::uint64_t, 2> unit_ids{20U, 21U};
  const std::array<std::uint64_t, 2> equal_ids{30U, 31U};
  const std::array<std::uint64_t, 2> radius_two_ids{40U, 41U};
  const SupportLevelEmission singleton_emission =
      support_level_emission_from_analysis(
          singleton_ids, analyze_circumcenter_support(singleton));
  const SupportLevelEmission unit_emission =
      support_level_emission_from_analysis(
          unit_ids, analyze_circumcenter_support(unit_pair));
  const SupportLevelEmission equal_emission =
      support_level_emission_from_analysis(
          equal_ids, analyze_circumcenter_support(equal_unit_pair));
  const SupportLevelEmission radius_two_emission =
      support_level_emission_from_analysis(
          radius_two_ids, analyze_circumcenter_support(radius_two_pair));

  PredicateCounters negative_counters;
  const auto negative = compare_support_levels(
      singleton_emission,
      unit_emission,
      &negative_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      negative.decision.sign() == PredicateSign::negative &&
          negative.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative.cross_product_difference == -1,
      "provenance emissions use FP64 for a strict negative level order");
  check_single_terminal(
      negative_counters,
      CertificationStage::fp64_filtered,
      0U,
      "strict negative emission comparison records one FP64 terminal");

  PredicateCounters positive_counters;
  const auto positive = compare_support_levels(
      unit_emission,
      singleton_emission,
      &positive_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      positive.decision.sign() == PredicateSign::positive &&
          positive.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          positive.cross_product_difference == 1,
      "reversed provenance emissions use FP64 for a positive order");
  check_single_terminal(
      positive_counters,
      CertificationStage::fp64_filtered,
      0U,
      "strict positive emission comparison records one FP64 terminal");

  PredicateCounters equality_counters;
  const auto equality = compare_support_levels(
      unit_emission,
      equal_emission,
      &equality_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      equality.decision.sign() == PredicateSign::zero &&
          equality.decision.certification_stage() ==
              CertificationStage::expansion &&
          equality.cross_product_difference == 0,
      "equal provenance emissions use expansion for their exact zero");
  check_single_terminal(
      equality_counters,
      CertificationStage::expansion,
      1U,
      "equal emission comparison records one expansion zero");

  PredicateCounters multiprecision_counters;
  const auto multiprecision = compare_support_levels(
      singleton_emission,
      unit_emission,
      &multiprecision_counters,
      PredicateFilterPolicy::multiprecision_only);
  check(
      multiprecision.decision.sign() == negative.decision.sign() &&
          multiprecision.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          multiprecision.cross_product_difference ==
              negative.cross_product_difference,
      "MP-only emission comparison retains the homogeneous witness");
  check_single_terminal(
      multiprecision_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "MP-only emission comparison records exactly one terminal");

  const SupportLevelEmission structural_zero = SupportLevelEmission::create(
      singleton_emission.squared_level(),
      singleton_emission.minimal_support_ids(),
      singleton_emission.source_support_ids());
  const SupportLevelEmission structural_one = SupportLevelEmission::create(
      unit_emission.squared_level(),
      unit_emission.minimal_support_ids(),
      unit_emission.source_support_ids());
  PredicateCounters structural_counters;
  const auto structural = compare_support_levels(
      structural_zero,
      structural_one,
      &structural_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      structural.decision.sign() == PredicateSign::negative &&
          structural.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          structural.cross_product_difference ==
              negative.cross_product_difference,
      "emissions without geometric provenance fail closed to MP");
  check_single_terminal(
      structural_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "structural emission fallback records one MP terminal");

  PredicateCounters mixed_counters;
  const auto mixed = compare_support_levels(
      singleton_emission,
      structural_one,
      &mixed_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      mixed.decision.sign() == PredicateSign::negative &&
          mixed.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          mixed.cross_product_difference == negative.cross_product_difference,
      "one missing provenance operand disables both fast level stages");
  check_single_terminal(
      mixed_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "mixed-provenance comparison records one MP terminal");

  const std::array<CertifiedPoint3, 2> reversed_unit_pair{
      unit_pair[1], unit_pair[0]};
  const std::array<CertifiedPoint3, 2> reversed_radius_two_pair{
      radius_two_pair[1], radius_two_pair[0]};
  const std::array<std::uint64_t, 2> reversed_unit_ids{unit_ids[1], unit_ids[0]};
  const std::array<std::uint64_t, 2> reversed_radius_two_ids{
      radius_two_ids[1], radius_two_ids[0]};
  const SupportLevelEmission reversed_unit_emission =
      support_level_emission_from_analysis(
          reversed_unit_ids,
          analyze_circumcenter_support(reversed_unit_pair));
  const SupportLevelEmission reversed_radius_two_emission =
      support_level_emission_from_analysis(
          reversed_radius_two_ids,
          analyze_circumcenter_support(reversed_radius_two_pair));
  check(
      reversed_unit_emission == unit_emission &&
          reversed_radius_two_emission == radius_two_emission,
      "paired point/ID permutations preserve canonical emissions");
  PredicateCounters permutation_counters;
  const auto permutation_order = compare_support_levels(
      reversed_unit_emission,
      reversed_radius_two_emission,
      &permutation_counters);
  check(
      permutation_order.decision.sign() == PredicateSign::negative &&
          permutation_order.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          permutation_order.cross_product_difference == -3,
      "permuted emission provenance preserves the strict FP64 level order");
  check_single_terminal(
      permutation_counters,
      CertificationStage::fp64_filtered,
      0U,
      "permuted emission comparison records one FP64 terminal");
}

void test_adaptive_level_fenv_fail_closed() {
  ScopedFloatingEnvironment restore_at_exit;
  check(
      restore_at_exit.saved(),
      "the adaptive level FENV test saves the caller environment");
  if (!restore_at_exit.saved()) {
    return;
  }

  const std::array<CertifiedPoint3, 2> unit_pair{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 2> radius_two_pair{
      point(-2.0, 0.0, 0.0), point(2.0, 0.0, 0.0)};
  check(
      std::fesetround(FE_DOWNWARD) == 0 &&
          std::feclearexcept(FE_ALL_EXCEPT) == 0 &&
          std::feraiseexcept(FE_INVALID | FE_DIVBYZERO) == 0,
      "the platform accepts the non-nearest level FENV setup");
  const int flags_before = std::fetestexcept(FE_ALL_EXCEPT);
  PredicateCounters counters;
  const auto result = compare_support_levels(
      unit_pair,
      radius_two_pair,
      &counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      result.decision.sign() == PredicateSign::negative &&
          result.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          result.cross_product_difference == -3,
      "non-nearest rounding makes adaptive level comparison fail closed to MP");
  check(
      std::fegetround() == FE_DOWNWARD &&
          std::fetestexcept(FE_ALL_EXCEPT) == flags_before,
      "adaptive level comparison preserves caller rounding and exception flags");
  check_single_terminal(
      counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "non-nearest level comparison records exactly one MP terminal");

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  check(
      std::fesetround(FE_TONEAREST) == 0 &&
          std::feclearexcept(FE_ALL_EXCEPT) == 0,
      "the FTZ/DAZ level test selects a clean nearest environment");
  constexpr unsigned int flush_to_zero_mask = 1U << 15U;
  constexpr unsigned int denormals_are_zero_mask = 1U << 6U;
  constexpr unsigned int rounding_control_mask = 3U << 13U;
  constexpr unsigned int exception_status_mask = 0x3fU;
  constexpr unsigned int preserved_mask = flush_to_zero_mask |
                                           denormals_are_zero_mask |
                                           rounding_control_mask |
                                           exception_status_mask;
  const unsigned int original_mxcsr = _mm_getcsr();
  const std::array<unsigned int, 3> altered_modes{
      (original_mxcsr | denormals_are_zero_mask) & ~flush_to_zero_mask,
      (original_mxcsr | flush_to_zero_mask) & ~denormals_are_zero_mask,
      original_mxcsr | flush_to_zero_mask | denormals_are_zero_mask};
  for (const unsigned int altered_mode : altered_modes) {
    _mm_setcsr(altered_mode);
    PredicateCounters altered_counters;
    const auto altered_result = compare_support_levels(
        unit_pair,
        radius_two_pair,
        &altered_counters,
        PredicateFilterPolicy::allow_adaptive);
    check(
        altered_result.decision.sign() == PredicateSign::negative &&
            altered_result.decision.certification_stage() ==
                CertificationStage::cpu_multiprecision &&
            altered_result.cross_product_difference == -3,
        "FTZ or DAZ makes adaptive level comparison fail closed to MP");
    check(
        (_mm_getcsr() & preserved_mask) == (altered_mode & preserved_mask),
        "adaptive level comparison preserves caller FTZ/DAZ and MXCSR status");
    check_single_terminal(
        altered_counters,
        CertificationStage::cpu_multiprecision,
        0U,
        "FTZ/DAZ level comparison records exactly one MP terminal");
  }
  _mm_setcsr(original_mxcsr);
#endif
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
  test_adaptive_support_array_level_order();
  test_canonical_support_ids();
  test_support_emission_invariants();
  test_emission_from_local_support_analysis();
  test_adaptive_emission_level_order();
  test_adaptive_level_fenv_fail_closed();
  test_canonical_level_batches();

  if (failures != 0) {
    std::cerr << failures << " exact level-order test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D exact level-order tests passed\n";
  return 0;
}
