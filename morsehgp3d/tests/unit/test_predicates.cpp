#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <cmath>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::FilterResult;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateDecision;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::decide_orientation_3d;
using morsehgp3d::exact::decide_power_bisector_side;
using morsehgp3d::exact::decide_squared_distance_order;
using morsehgp3d::exact::evaluate_power_bisector;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::power_bisector_side;
using morsehgp3d::exact::squared_distance;

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

ExactLabelMoments label(std::initializer_list<CertifiedPoint3> points) {
  const std::vector<CertifiedPoint3> point_table(points);
  std::vector<std::uint32_t> ids;
  ids.reserve(point_table.size());
  for (std::size_t index = 0; index < point_table.size(); ++index) {
    ids.push_back(static_cast<std::uint32_t>(index));
  }
  return ExactLabelMoments::from_canonical_ids(ids, point_table);
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

  PredicateCounters decision_counters;
  const PredicateDecision decision = decide_squared_distance_order(
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      &decision_counters);
  check(decision.sign() == PredicateSign::zero &&
            decision_counters.cpu_multiprecision_certified() == 1U &&
            decision_counters.exact_zeros() == 1U,
        "the decision-only distance entry point preserves exact equality accounting");
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

  PredicateCounters decision_counters;
  const PredicateDecision decision = decide_orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 1.0, 0.0),
      &decision_counters);
  check(decision.sign() == PredicateSign::zero &&
            decision_counters.exact_zeros() == 1U,
        "the decision-only orientation entry point preserves exact coplanarity");
}

void test_power_bisector_side() {
  const ExactLabelMoments r = label({point(2.0, 0.0, 0.0)});
  const ExactLabelMoments q = label({point(1.0, 0.0, 0.0)});
  check(r.cardinality() == 1U &&
            r.coordinate_sum(0).canonical_key() == "2/1" &&
            r.squared_norm_sum().canonical_key() == "4/1",
        "exact label moments record cardinality, coordinate sum, and squared norm sum");

  PredicateCounters counters;
  const auto at_origin =
      power_bisector_side(point(0.0, 0.0, 0.0), r, q, &counters);
  check(at_origin.decision.sign() == PredicateSign::positive &&
            at_origin.witness.affine_value.canonical_key() == "3/1" &&
            at_origin.witness.delta_coordinate_sum.coordinate(0).canonical_key() ==
                "1/1" &&
            at_origin.witness.delta_squared_norm_sum.canonical_key() == "3/1" &&
            counters.cpu_multiprecision_certified() == 1U,
        "H_RQ is the exact R-minus-Q power cost at the witness");
  const auto reverse_at_origin =
      power_bisector_side(point(0.0, 0.0, 0.0), q, r);
  check(reverse_at_origin.decision.sign() == PredicateSign::negative &&
            at_origin.witness.affine_value ==
                -reverse_at_origin.witness.affine_value,
        "swapping nonzero power labels reverses both sign and exact value");

  const auto at_r = power_bisector_side(point(2.0, 0.0, 0.0), r, q);
  check(at_r.decision.sign() == PredicateSign::negative &&
            at_r.witness.affine_value.canonical_key() == "-1/1",
        "the power bisector changes side across its exact zero");

  const ExactRational3 exact_midpoint{
      BigInt{3}, BigInt{0}, BigInt{0}, BigInt{2}};
  PredicateCounters zero_counters;
  const PredicateDecision at_midpoint =
      decide_power_bisector_side(exact_midpoint, r, q, &zero_counters);
  check(at_midpoint.sign() == PredicateSign::zero &&
            zero_counters.exact_zeros() == 1U,
        "a rational witness on the power bisector is an exact zero");

  const ExactRational forward = evaluate_power_bisector(exact_midpoint, r, q);
  const ExactRational reverse = evaluate_power_bisector(exact_midpoint, q, r);
  check(forward == -reverse,
        "swapping R and Q negates the affine power comparison");

  const ExactLabelMoments first_order =
      label({point(4.0, -1.0, 0.5), point(-2.0, 3.0, 1.0)});
  const ExactLabelMoments second_order =
      label({point(-2.0, 3.0, 1.0), point(4.0, -1.0, 0.5)});
  check(first_order == second_order,
        "exact label moments are invariant under label permutation");

  const ExactLabelMoments empty;
  PredicateCounters empty_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(power_bisector_side(
            point(0.0, 0.0, 0.0), empty, empty, &empty_counters));
      },
      "top-k power labels cannot be empty");
  check(empty_counters.certified_decisions() == 0U,
        "an empty-label rejection does not increment certification counters");

  PredicateCounters rejected_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(power_bisector_side(
            point(0.0, 0.0, 0.0), r, first_order, &rejected_counters));
      },
      "power labels of different cardinalities are rejected");
  check(rejected_counters.certified_decisions() == 0U,
        "a rejected power comparison does not increment certification counters");
  check_throws<std::out_of_range>(
      [&] { static_cast<void>(r.coordinate_sum(3U)); },
      "label moment axes outside 3D are rejected");

  const std::vector<CertifiedPoint3> point_table{
      point(0.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  check_throws<std::invalid_argument>(
      [&] {
        const std::vector<std::uint32_t> duplicate_ids{0U, 0U};
        static_cast<void>(
            ExactLabelMoments::from_canonical_ids(duplicate_ids, point_table));
      },
      "label identifiers must be unique");
  check_throws<std::invalid_argument>(
      [&] {
        const std::vector<std::uint32_t> unsorted_ids{1U, 0U};
        static_cast<void>(
            ExactLabelMoments::from_canonical_ids(unsorted_ids, point_table));
      },
      "label identifiers must be sorted canonically");
  check_throws<std::out_of_range>(
      [&] {
        const std::vector<std::uint32_t> invalid_ids{2U};
        static_cast<void>(
            ExactLabelMoments::from_canonical_ids(invalid_ids, point_table));
      },
      "label identifiers must refer to the supplied point table");

  const double offset = std::ldexp(1.0, 40);
  const double near = offset + 1.0;
  const double one_ulp_farther =
      std::nextafter(near, std::numeric_limits<double>::infinity());
  const auto large_offset = power_bisector_side(
      point(offset, 0.0, 0.0),
      label({point(one_ulp_farther, 0.0, 0.0)}),
      label({point(near, 0.0, 0.0)}));
  const auto large_offset_reversed = power_bisector_side(
      point(offset, 0.0, 0.0),
      label({point(near, 0.0, 0.0)}),
      label({point(one_ulp_farther, 0.0, 0.0)}));
  const ExactRational exact_midpoint_coordinate =
      (ExactRational::from_binary64(near) +
       ExactRational::from_binary64(one_ulp_farther)) /
      ExactRational{BigInt{2}};
  const ExactRational3 exact_ulp_midpoint{
      std::array<ExactRational, 3>{
          exact_midpoint_coordinate, ExactRational{}, ExactRational{}}};
  const auto large_offset_zero = power_bisector_side(
      exact_ulp_midpoint,
      label({point(one_ulp_farther, 0.0, 0.0)}),
      label({point(near, 0.0, 0.0)}));
  check(large_offset.decision.sign() == PredicateSign::positive &&
            large_offset_reversed.decision.sign() == PredicateSign::negative &&
            large_offset_zero.decision.sign() == PredicateSign::zero,
        "a one-ULP power difference at large offset is decided on both sides and at zero");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const auto subnormal_side = power_bisector_side(
      point(0.0, 0.0, 0.0),
      label({point(subnormal, 0.0, 0.0)}),
      label({point(0.0, 0.0, 0.0)}));
  check(subnormal_side.decision.sign() == PredicateSign::positive,
        "a minimum-subnormal power difference remains strictly positive");

  const double maximum = std::numeric_limits<double>::max();
  const auto maximum_cancellation = power_bisector_side(
      point(maximum, -maximum, maximum),
      label({point(maximum, 0.0, 0.0)}),
      label({point(maximum, 0.0, 0.0)}));
  check(maximum_cancellation.decision.sign() == PredicateSign::zero,
        "maximum finite binary64 coordinates cancel exactly for identical labels");

  std::vector<CertifiedPoint3> ten_points;
  std::vector<std::uint32_t> ten_ids;
  for (std::uint32_t identifier = 0U; identifier < 10U; ++identifier) {
    ten_points.push_back(
        point(static_cast<double>(identifier), 0.0, 0.0));
    ten_ids.push_back(identifier);
  }
  const ExactLabelMoments ten_label =
      ExactLabelMoments::from_canonical_ids(ten_ids, ten_points);
  PredicateCounters ten_counters;
  const auto ten_equality = power_bisector_side(
      point(3.0, -2.0, 1.0), ten_label, ten_label, &ten_counters);
  check(ten_equality.decision.sign() == PredicateSign::zero &&
            ten_counters.exact_zeros() == 1U,
        "the maximum supported label cardinality ten preserves exact equality");
  ten_points.push_back(point(10.0, 0.0, 0.0));
  ten_ids.push_back(10U);
  const ExactLabelMoments eleven_label =
      ExactLabelMoments::from_canonical_ids(ten_ids, ten_points);
  PredicateCounters eleven_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(power_bisector_side(
            point(0.0, 0.0, 0.0),
            eleven_label,
            eleven_label,
            &eleven_counters));
      },
      "power labels above the supported maximum cardinality are rejected");
  check(eleven_counters.certified_decisions() == 0U,
        "an oversized-label rejection does not increment certification counters");
}

void test_power_bisector_relations() {
  const CertifiedPoint3 witness = point(0.25, -0.5, 1.0);
  const CertifiedPoint3 r_point = point(2.0, 1.0, -3.0);
  const CertifiedPoint3 q_point = point(-1.0, 4.0, 0.5);
  const ExactLabelMoments singleton_r = label({r_point});
  const ExactLabelMoments singleton_q = label({q_point});
  const auto distance =
      compare_squared_distances(witness, r_point, q_point);
  const auto power =
      power_bisector_side(witness, singleton_r, singleton_q);
  check(power.decision.sign() == distance.decision.sign() &&
            power.witness.affine_value ==
                distance.left_squared_distance.rational() -
                    distance.right_squared_distance.rational(),
        "the cardinality-one H_RQ predicate equals direct distance comparison");

  const CertifiedPoint3 shared = point(3.0, -2.0, 0.0);
  const ExactLabelMoments overlapping_r = label({r_point, shared});
  const ExactLabelMoments overlapping_q = label({q_point, shared});
  const ExactRational overlap_value =
      evaluate_power_bisector(witness.exact(), overlapping_r, overlapping_q);
  check(overlap_value ==
            squared_distance(witness, r_point) - squared_distance(witness, q_point),
        "points shared by R and Q cancel exactly without requiring disjoint labels");

  const std::vector<CertifiedPoint3> first_table{r_point, q_point, shared};
  const std::vector<std::uint32_t> first_r_ids{0U, 2U};
  const std::vector<std::uint32_t> first_q_ids{1U, 2U};
  const std::vector<CertifiedPoint3> permuted_table{shared, r_point, q_point};
  const std::vector<std::uint32_t> permuted_r_ids{0U, 1U};
  const std::vector<std::uint32_t> permuted_q_ids{0U, 2U};
  const auto first_table_value = power_bisector_side(
      witness,
      ExactLabelMoments::from_canonical_ids(first_r_ids, first_table),
      ExactLabelMoments::from_canonical_ids(first_q_ids, first_table));
  const auto permuted_table_value = power_bisector_side(
      witness,
      ExactLabelMoments::from_canonical_ids(permuted_r_ids, permuted_table),
      ExactLabelMoments::from_canonical_ids(permuted_q_ids, permuted_table));
  check(first_table_value.witness.affine_value ==
                permuted_table_value.witness.affine_value &&
            first_table_value.witness.delta_coordinate_sum ==
                permuted_table_value.witness.delta_coordinate_sum,
        "permuting the point table and remapping canonical identifiers preserves H_RQ");

  const ExactLabelMoments constant_r =
      label({point(0.0, 0.0, 0.0), point(2.0, 0.0, 0.0)});
  const ExactLabelMoments constant_q =
      label({point(0.5, 0.0, 0.0), point(1.5, 0.0, 0.0)});
  const auto constant_left =
      power_bisector_side(point(-10.0, 4.0, 2.0), constant_r, constant_q);
  const auto constant_right =
      power_bisector_side(point(10.0, -7.0, 8.0), constant_r, constant_q);
  check(constant_left.witness.delta_coordinate_sum == ExactRational3{} &&
            constant_left.witness.affine_value.canonical_key() == "3/2" &&
            constant_left.witness.affine_value == constant_right.witness.affine_value,
        "equal label coordinate sums produce the expected constant power form");

  const auto original = power_bisector_side(
      point(0.25, 0.0, 0.0),
      label({point(2.0, 0.0, 0.0)}),
      label({point(1.0, 0.0, 0.0)}));
  const auto translated = power_bisector_side(
      point(7.25, -3.0, 5.0),
      label({point(9.0, -3.0, 5.0)}),
      label({point(8.0, -3.0, 5.0)}));
  const auto reflected = power_bisector_side(
      point(-0.25, 0.0, 0.0),
      label({point(-2.0, 0.0, 0.0)}),
      label({point(-1.0, 0.0, 0.0)}));
  const auto scaled = power_bisector_side(
      point(0.5, 0.0, 0.0),
      label({point(4.0, 0.0, 0.0)}),
      label({point(2.0, 0.0, 0.0)}));
  check(translated.witness.affine_value == original.witness.affine_value &&
            reflected.witness.affine_value == original.witness.affine_value &&
            scaled.witness.affine_value ==
                ExactRational{BigInt{4}} * original.witness.affine_value,
        "H_RQ is translation invariant, signed-axis invariant, and quadratic under scale two");
}

}  // namespace

int main() {
  test_filter_and_counter_contract();
  test_distance_comparison();
  test_orientation_3d();
  test_power_bisector_side();
  test_power_bisector_relations();

  if (failures != 0) {
    std::cerr << failures << " predicate test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D predicate tests passed\n";
  return 0;
}
