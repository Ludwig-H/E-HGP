#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <string>

namespace {

using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::FilterResult;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateDecision;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::orientation_3d;

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
    std::cerr << "FAIL: " << message << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

void test_filter_and_counter_contract() {
  const FilterResult uncertain = FilterResult::uncertain();
  check(uncertain.state() == morsehgp3d::exact::FilterState::uncertain &&
            !uncertain.sign().has_value(),
        "an uncertain filter exposes no scientific sign");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(FilterResult::certified(PredicateSign::zero)); },
      "a floating filter cannot certify exact zero");

  PredicateCounters counters;
  counters.record_fp32_proposal();
  counters.record_certification(
      PredicateDecision(PredicateSign::positive, CertificationStage::fp64_filtered));
  counters.record_certification(
      PredicateDecision(PredicateSign::negative, CertificationStage::expansion));
  counters.record_certification(
      PredicateDecision(PredicateSign::zero, CertificationStage::cpu_multiprecision));
  check(counters.fp32_proposals() == 1U && counters.certified_decisions() == 3U &&
            counters.exact_zeros() == 1U && counters.remaining_unknown() == 0U,
        "predicate proposal, certification, and exact-zero accounting are distinct");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(PredicateDecision(
            PredicateSign::zero, CertificationStage::fp64_filtered));
      },
      "an fp64 filter cannot construct an exact-zero decision");
}

void test_distance_comparison() {
  PredicateCounters counters;
  const auto ordered = compare_squared_distances(
      point(0.0, 0.0, 0.0), point(1.0, 0.0, 0.0), point(2.0, 0.0, 0.0), &counters);
  check(ordered.decision.sign() == PredicateSign::negative,
        "nearer left point gives a negative distance difference");
  check(ordered.left_squared_distance.canonical_key() == "1/1" &&
            ordered.right_squared_distance.canonical_key() == "4/1",
        "distance comparison materializes exact squared levels");
  check(ordered.decision.certification_stage() ==
                CertificationStage::cpu_multiprecision &&
            counters.cpu_multiprecision_certified() == 1U &&
            counters.certified_decisions() == 1U && counters.exact_zeros() == 0U &&
            counters.remaining_unknown() == 0U,
        "distance fallback is accounted as one exact decision");

  PredicateCounters zero_counters;
  const auto equal = compare_squared_distances(
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      &zero_counters);
  check(equal.decision.sign() == PredicateSign::zero &&
            zero_counters.cpu_multiprecision_certified() == 1U &&
            zero_counters.exact_zeros() == 1U,
        "symmetric distances produce and account for exact zero");

  const double below = std::nextafter(1.0, 0.0);
  const auto one_ulp = compare_squared_distances(
      point(0.0, 0.0, 0.0), point(below, 0.0, 0.0), point(1.0, 0.0, 0.0));
  check(one_ulp.decision.sign() == PredicateSign::negative,
        "one-ULP distance difference is not rounded away");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const auto subnormal_difference = compare_squared_distances(
      point(0.0, 0.0, 0.0), point(subnormal, 0.0, 0.0), point(0.0, 0.0, 0.0));
  check(subnormal_difference.decision.sign() == PredicateSign::positive,
        "a minimum-subnormal displacement remains strictly positive");
}

void test_orientation_3d() {
  PredicateCounters counters;
  const auto positive = orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, 1.0),
      &counters);
  check(positive.decision.sign() == PredicateSign::positive &&
            positive.determinant.canonical_key() == "1/1",
        "right-handed tetrahedron has positive exact orientation");
  check(counters.cpu_multiprecision_certified() == 1U &&
            counters.exact_zeros() == 0U && counters.remaining_unknown() == 0U,
        "orientation fallback is accounted exactly");

  const auto negative = orientation_3d(
      point(0.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 0.0, 1.0));
  check(negative.decision.sign() == PredicateSign::negative,
        "swapping two support points reverses orientation");

  PredicateCounters zero_counters;
  const auto coplanar = orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 1.0, 0.0),
      &zero_counters);
  check(coplanar.decision.sign() == PredicateSign::zero &&
            coplanar.determinant.is_zero() &&
            zero_counters.cpu_multiprecision_certified() == 1U &&
            zero_counters.exact_zeros() == 1U,
        "coplanarity is exact zero and is accounted independently of its stage");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const auto above_plane = orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 1.0, subnormal));
  check(above_plane.decision.sign() == PredicateSign::positive,
        "minimum-subnormal height has a strict exact orientation");
}

}  // namespace

int main() {
  test_filter_and_counter_contract();
  test_distance_comparison();
  test_orientation_3d();

  if (failures != 0) {
    std::cerr << failures << " predicate test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D predicate tests passed\n";
  return 0;
}
