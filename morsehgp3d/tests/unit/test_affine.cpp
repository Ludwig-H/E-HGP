#include "morsehgp3d/exact/plane.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <array>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <immintrin.h>
#endif

namespace {

using morsehgp3d::exact::AffineFormKind;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::Binary64AffineOrigin;
using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactPlane3;
using morsehgp3d::exact::ExactPlane3Record;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::FourthPlaneIncidenceResult;
using morsehgp3d::exact::Orientation2DResult;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateDecision;
using morsehgp3d::exact::PredicateFilterPolicy;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::PlaneSideResult;
using morsehgp3d::exact::ThreePlaneIntersectionKind;
using morsehgp3d::exact::certified_intersect_three_planes;
using morsehgp3d::exact::classify_affine_form;
using morsehgp3d::exact::decide_fourth_plane_incidence;
using morsehgp3d::exact::fourth_plane_incidence;
using morsehgp3d::exact::intersect_three_planes;
using morsehgp3d::exact::orientation_2d_in_plane;
using morsehgp3d::exact::plane_side;
using morsehgp3d::exact::power_bisector_affine_form;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

void check_single_certification(
    const PredicateCounters& counters,
    CertificationStage stage,
    std::uint64_t exact_zeros,
    const std::string& message) {
  const bool matching_stage =
      (stage == CertificationStage::fp64_filtered &&
       counters.fp64_filtered_certified() == 1U &&
       counters.expansion_certified() == 0U &&
       counters.cpu_multiprecision_certified() == 0U) ||
      (stage == CertificationStage::expansion &&
       counters.fp64_filtered_certified() == 0U &&
       counters.expansion_certified() == 1U &&
       counters.cpu_multiprecision_certified() == 0U) ||
      (stage == CertificationStage::cpu_multiprecision &&
       counters.fp64_filtered_certified() == 0U &&
       counters.expansion_certified() == 0U &&
       counters.cpu_multiprecision_certified() == 1U);
  check(
      matching_stage && counters.certified_decisions() == 1U &&
          counters.exact_zeros() == exact_zeros &&
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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

ExactPlane3 plane(long long a, long long b, long long c, long long d) {
  return ExactPlane3::from_integer_coefficients(
      {BigInt{a}, BigInt{b}, BigInt{c}, BigInt{d}});
}

ExactPlane3 binary_plane(double a, double b, double c, double d) {
  return ExactPlane3::from_binary64_coefficients(
      std::array<double, 4>{a, b, c, d});
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

void test_affine_form_and_plane_canonicalization() {
  const ExactPlane3 reduced = plane(2, 4, 6, 8);
  check(reduced.oriented_key() == "1:2:3:4",
        "integer plane coefficients are reduced by one common positive gcd");
  check(reduced.unoriented_key() == "1:2:3:4",
        "a positive first normal coefficient already has canonical geometric orientation");
  check(reduced.canonical_json() ==
            "{\"a\":\"1\",\"b\":\"2\",\"c\":\"3\",\"d\":\"4\","
            "\"schema_version\":\"2.0.0\"}" &&
            ExactPlane3::from_record(reduced.to_record()) == reduced,
        "the exact plane record has a closed canonical round trip");
  const ExactPlane3 reversed = plane(-2, -4, -6, -8);
  check(reversed.oriented_key() == "-1:-2:-3:-4" &&
            reversed.unoriented_key() == reduced.unoriented_key() &&
            reversed == reduced.opposite() &&
            reversed.same_geometric_plane(reduced),
        "negative scaling reverses orientation but preserves the geometric plane");

  const ExactPlane3 rational = ExactPlane3::from_rational_coefficients({
      ExactRational{BigInt{1}, BigInt{2}},
      ExactRational{BigInt{1}, BigInt{3}},
      ExactRational{},
      ExactRational{BigInt{-1}, BigInt{6}}});
  check(rational.oriented_key() == "3:2:0:-1",
        "rational plane coefficients use a positive common denominator and gcd");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(ExactPlane3::from_integer_coefficients(
            {BigInt{0}, BigInt{0}, BigInt{0}, BigInt{1}}));
      },
      "a constant nonzero form is not silently converted to a plane");
  check_throws<std::out_of_range>(
      [&] { static_cast<void>(reduced.coefficient(4U)); },
      "plane coefficient indices outside the homogeneous quadruplet are rejected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(ExactPlane3::from_record(
            ExactPlane3Record{"2.0.0", "2", "0", "0", "0"}));
      },
      "a replay plane record must already be primitive");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(ExactPlane3::from_record(
            ExactPlane3Record{"2.0.0", "-0", "1", "0", "0"}));
      },
      "a replay plane record rejects negative zero");

  const ExactAffineForm3 constant_positive =
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{0}, BigInt{0}, BigInt{0}, BigInt{7}});
  const ExactAffineForm3 constant_negative =
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{0}, BigInt{0}, BigInt{0}, BigInt{-9}});
  const ExactAffineForm3 identically_zero =
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{0}, BigInt{0}, BigInt{0}, BigInt{0}});
  check(constant_positive.oriented_key() == "0:0:0:1" &&
            classify_affine_form(constant_positive).kind() ==
                AffineFormKind::constant_positive &&
            classify_affine_form(constant_negative).kind() ==
                AffineFormKind::constant_negative &&
            identically_zero.is_identically_zero() &&
            classify_affine_form(identically_zero).kind() ==
                AffineFormKind::identically_zero,
        "constant affine forms retain positive, negative, and identically-zero classes");
  const auto proper = classify_affine_form(
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{2}, BigInt{0}, BigInt{0}, BigInt{-4}}));
  check(proper.kind() == AffineFormKind::proper_plane &&
            proper.plane().has_value() &&
            proper.plane()->oriented_key() == "1:0:0:-2",
        "a nonconstant affine form classifies to its exact oriented plane");
}

void test_binary64_affine_provenance() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  using OrientationDecisionPointer = PredicateDecision (*)(
      const ExactPlane3&,
      const CertifiedPoint3&,
      const CertifiedPoint3&,
      const CertifiedPoint3&,
      PredicateCounters*);
  using OrientationResultPointer = Orientation2DResult (*)(
      const ExactPlane3&,
      const CertifiedPoint3&,
      const CertifiedPoint3&,
      const CertifiedPoint3&,
      PredicateCounters*);
  using PlaneDecisionPointer = PredicateDecision (*)(
      const ExactPlane3&, const CertifiedPoint3&, PredicateCounters*);
  using PlaneResultPointer = PlaneSideResult (*)(
      const ExactPlane3&, const CertifiedPoint3&, PredicateCounters*);
  using FourthDecisionPointer = PredicateDecision (*)(
      const ExactPlane3&,
      const ExactPlane3&,
      const ExactPlane3&,
      const ExactPlane3&,
      PredicateCounters*);
  using FourthResultPointer = FourthPlaneIncidenceResult (*)(
      const ExactPlane3&,
      const ExactPlane3&,
      const ExactPlane3&,
      const ExactPlane3&,
      PredicateCounters*);
  const auto legacy_orientation_decision = static_cast<OrientationDecisionPointer>(
      &morsehgp3d::exact::decide_orientation_2d_in_plane);
  const auto legacy_orientation_result = static_cast<OrientationResultPointer>(
      &morsehgp3d::exact::orientation_2d_in_plane);
  const auto legacy_plane_decision = static_cast<PlaneDecisionPointer>(
      &morsehgp3d::exact::decide_plane_side);
  const auto legacy_plane_result = static_cast<PlaneResultPointer>(
      &morsehgp3d::exact::plane_side);
  const auto legacy_fourth_decision = static_cast<FourthDecisionPointer>(
      &morsehgp3d::exact::decide_fourth_plane_incidence);
  const auto legacy_fourth_result = static_cast<FourthResultPointer>(
      &morsehgp3d::exact::fourth_plane_incidence);
  static_cast<void>(legacy_orientation_decision);
  static_cast<void>(legacy_orientation_result);
  static_cast<void>(legacy_plane_decision);
  static_cast<void>(legacy_plane_result);
  static_cast<void>(legacy_fourth_decision);
  static_cast<void>(legacy_fourth_result);
  const ExactPlane3 xy = ExactPlane3::through_points(origin, e1, e2);
  const ExactPlane3 cyclic = ExactPlane3::through_points(e1, e2, origin);
  const ExactPlane3 swapped = ExactPlane3::through_points(origin, e2, e1);
  const ExactPlane3 signed_zero = ExactPlane3::through_points(
      point(-0.0, 0.0, -0.0),
      point(1.0, -0.0, 0.0),
      point(-0.0, 1.0, -0.0));
  check(
      xy.binary64_provenance().has_value() &&
          cyclic.binary64_provenance().has_value() &&
          swapped.binary64_provenance().has_value() &&
          signed_zero.binary64_provenance().has_value() &&
          xy.binary64_provenance()->origin() ==
              Binary64AffineOrigin::through_points &&
          xy.binary64_provenance()->canonical_key() ==
              cyclic.binary64_provenance()->canonical_key() &&
          swapped.binary64_provenance()->canonical_key() ==
              xy.opposite().binary64_provenance()->canonical_key() &&
          signed_zero.binary64_provenance()->canonical_key() ==
              xy.binary64_provenance()->canonical_key(),
      "through-point provenance canonicalizes cyclic parity and signed zero");
  check(
      ExactPlane3::from_rational_coefficients(
          xy.binary64_provenance()->exact_coefficients()) == xy &&
          ExactPlane3::from_rational_coefficients(
              swapped.binary64_provenance()->exact_coefficients()) == swapped,
      "verified through-point provenance reproduces its exact oriented plane");

  const ExactPlane3 exact_xy = plane(0, 0, 1, 0);
  const ExactPlane3 replay_xy = ExactPlane3::from_record(xy.to_record());
  check(
      xy == exact_xy && replay_xy == xy &&
          !exact_xy.binary64_provenance().has_value() &&
          !replay_xy.binary64_provenance().has_value(),
      "plane provenance is excluded from equality and the closed exact record");

  const ExactPlane3 coefficient_xy = binary_plane(-0.0, 0.0, 2.0, -0.0);
  const ExactPlane3 canonical_coefficient_xy =
      binary_plane(0.0, -0.0, 2.0, 0.0);
  check(
      coefficient_xy == xy &&
          coefficient_xy.binary64_provenance().has_value() &&
          coefficient_xy.binary64_provenance()->origin() ==
              Binary64AffineOrigin::explicit_coefficients &&
          coefficient_xy.binary64_provenance()->canonical_key() ==
              canonical_coefficient_xy.binary64_provenance()->canonical_key() &&
          coefficient_xy.opposite().binary64_provenance()->canonical_key() ==
              binary_plane(0.0, 0.0, -2.0, 0.0)
                  .binary64_provenance()->canonical_key(),
      "explicit coefficient provenance normalizes every signed zero without changing identity");

  const ExactAffineForm3 coefficient_form =
      ExactAffineForm3::from_binary64_coefficients(
          std::array<double, 4>{2.0, -0.0, 0.0, -4.0});
  const ExactAffineForm3 exact_form =
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{2}, BigInt{0}, BigInt{0}, BigInt{-4}});
  check(
      coefficient_form == exact_form &&
          coefficient_form.binary64_provenance().has_value() &&
          !exact_form.binary64_provenance().has_value() &&
          coefficient_form.binary64_provenance()->exact_coefficients()[0] ==
              coefficient_form.a() &&
          coefficient_form.binary64_provenance()->exact_coefficients()[3] ==
              coefficient_form.d(),
      "affine-form provenance is verified coefficientwise but excluded from scientific equality");

  const ExactLabelMoments r = label({point(2.0, 0.0, 0.0)});
  const ExactLabelMoments q = label({point(1.0, 0.0, 0.0)});
  const ExactAffineForm3 power_form = power_bisector_affine_form(r, q);
  const ExactAffineForm3 explicit_power =
      ExactAffineForm3::from_binary64_coefficients(
          std::array<double, 4>{-2.0, 0.0, 0.0, 3.0});
  const auto power_classification = classify_affine_form(power_form);
  check(
      power_form == explicit_power &&
          power_form.binary64_provenance().has_value() &&
          power_form.binary64_provenance()->origin() ==
              Binary64AffineOrigin::power_bisector &&
          power_classification.plane().has_value() &&
          power_classification.plane()->binary64_provenance().has_value() &&
          power_classification.plane()->binary64_provenance()->origin() ==
              Binary64AffineOrigin::power_bisector &&
          ExactPlane3::from_rational_coefficients(
              power_form.binary64_provenance()->exact_coefficients()) ==
              *power_classification.plane(),
      "power provenance preserves exact scale on the form and propagates to its primitive plane");
  const ExactAffineForm3 reversed_power = power_bisector_affine_form(q, r);
  check(
      reversed_power.binary64_provenance().has_value() &&
          reversed_power.binary64_provenance()->canonical_key() ==
              power_form.binary64_provenance()->opposite().canonical_key(),
      "swapping power labels reverses the canonical provenance orientation");

  check_throws<std::domain_error>(
      [] {
        static_cast<void>(ExactPlane3::from_binary64_coefficients(
            std::array<double, 4>{
                std::numeric_limits<double>::infinity(), 0.0, 0.0, 0.0}));
      },
      "binary64 affine provenance rejects a nonfinite coefficient");
}

void test_plane_through_points() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  const ExactPlane3 xy = ExactPlane3::through_points(origin, e1, e2);
  const ExactPlane3 xy_cyclic = ExactPlane3::through_points(e1, e2, origin);
  const ExactPlane3 xy_swapped = ExactPlane3::through_points(origin, e2, e1);
  check(xy.oriented_key() == "0:0:1:0" && xy_cyclic == xy &&
            xy_swapped == xy.opposite(),
        "ordered points induce the cross-product orientation of their plane");
  check(xy.evaluate(origin).is_zero() && xy.evaluate(e1).is_zero() &&
            xy.evaluate(e2).is_zero(),
        "every defining point is exactly incident to its constructed plane");

  const ExactPlane3 translated = ExactPlane3::through_points(
      point(0.0, 0.0, 5.0),
      point(1.0, 0.0, 5.0),
      point(0.0, 1.0, 5.0));
  check(translated.oriented_key() == "0:0:1:-5",
        "plane construction preserves an exact translated offset");
  const ExactPlane3 rational = ExactPlane3::through_points(
      ExactRational3{},
      ExactRational3{BigInt{1}, BigInt{0}, BigInt{0}, BigInt{2}},
      ExactRational3{BigInt{0}, BigInt{1}, BigInt{0}, BigInt{3}});
  check(rational == xy,
        "plane construction accepts canonical non-dyadic rational points");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(ExactPlane3::through_points(
            origin, point(1.0, 1.0, 1.0), point(2.0, 2.0, 2.0)));
      },
      "collinear points do not define an arbitrary plane");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const ExactPlane3 subnormal_support = ExactPlane3::through_points(
      origin, point(subnormal, 0.0, 0.0), point(0.0, subnormal, 0.0));
  check(subnormal_support.oriented_key() == "0:0:1:0",
        "minimum-subnormal support vectors retain their exact plane");
  const ExactPlane3 one_ulp_support = ExactPlane3::through_points(
      point(1.0, 0.0, 0.0),
      point(std::nextafter(1.0, 2.0), 0.0, 0.0),
      point(1.0, 1.0, 0.0));
  check(one_ulp_support == xy,
        "a one-ULP support edge remains affinely independent exactly");
}

void test_power_bisector_affine_form() {
  const ExactLabelMoments r = label({point(2.0, 0.0, 0.0)});
  const ExactLabelMoments q = label({point(1.0, 0.0, 0.0)});
  const ExactAffineForm3 form = power_bisector_affine_form(r, q);
  const ExactAffineForm3 reversed = power_bisector_affine_form(q, r);
  check(form.oriented_key() == "-2:0:0:3" &&
            reversed.oriented_key() == "2:0:0:-3" &&
            classify_affine_form(form).kind() == AffineFormKind::proper_plane,
        "H_RQ becomes a canonical affine form and swapping labels reverses it");
  check(form.evaluate(point(0.0, 0.0, 0.0)).canonical_key() == "3/1" &&
            form.evaluate(point(2.0, 0.0, 0.0)).canonical_key() == "-1/1",
        "the exact H_RQ form preserves the expected value on both sides");
  const ExactAffineForm3 scaled_value = power_bisector_affine_form(
      label({point(2.0, 0.0, 0.0)}), label({point(0.0, 0.0, 0.0)}));
  check(scaled_value.oriented_key() == "-1:0:0:1" &&
            scaled_value.evaluate(point(0.0, 0.0, 0.0)).canonical_key() == "4/1" &&
            scaled_value.coefficient(0U).canonical_key() == "-4/1",
        "H_RQ retains its exact coefficient scale separately from its primitive plane key");

  const ExactLabelMoments constant_r =
      label({point(0.0, 0.0, 0.0), point(2.0, 0.0, 0.0)});
  const ExactLabelMoments constant_q =
      label({point(0.5, 0.0, 0.0), point(1.5, 0.0, 0.0)});
  check(classify_affine_form(
            power_bisector_affine_form(constant_r, constant_q)).kind() ==
            AffineFormKind::constant_positive &&
            classify_affine_form(
                power_bisector_affine_form(constant_q, constant_r)).kind() ==
                AffineFormKind::constant_negative,
        "equal coordinate sums retain the sign of a nonzero constant H_RQ form");
  check(classify_affine_form(power_bisector_affine_form(r, r)).kind() ==
            AffineFormKind::identically_zero,
        "identical label moments classify H_RQ as identically zero");
}

void test_orientation_2d() {
  const ExactPlane3 xy = plane(0, 0, 1, 0);
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  PredicateCounters positive_counters;
  const auto positive = orientation_2d_in_plane(
      xy, origin, e1, e2, &positive_counters);
  const auto negative = orientation_2d_in_plane(xy, origin, e2, e1);
  PredicateCounters zero_counters;
  const auto zero = orientation_2d_in_plane(
      xy, origin, e1, point(2.0, 0.0, 0.0), &zero_counters);
  check(positive.decision.sign() == PredicateSign::positive &&
            positive.orientation_value.canonical_key() == "1/1" &&
            positive.decision.certification_stage() ==
                CertificationStage::cpu_multiprecision &&
            positive_counters.cpu_multiprecision_certified() == 1U &&
            negative.decision.sign() == PredicateSign::negative &&
            zero.decision.sign() == PredicateSign::zero &&
            zero_counters.exact_zeros() == 1U,
        "orientation 2D decides positive, negative, and exact collinear cases");
  check(orientation_2d_in_plane(xy.opposite(), origin, e1, e2).decision.sign() ==
            PredicateSign::negative,
        "reversing the support-plane orientation reverses orientation 2D");

  const ExactRational3 rational_origin{};
  const ExactRational3 rational_x{BigInt{1}, BigInt{0}, BigInt{0}, BigInt{2}};
  const ExactRational3 rational_y{BigInt{0}, BigInt{1}, BigInt{0}, BigInt{3}};
  check(orientation_2d_in_plane(
            xy, rational_origin, rational_x, rational_y).orientation_value ==
            ExactRational{BigInt{1}, BigInt{6}},
        "orientation 2D accepts exact non-dyadic rational witnesses");

  PredicateCounters rejected_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(orientation_2d_in_plane(
            xy, origin, e1, point(0.0, 1.0, std::numeric_limits<double>::denorm_min()),
            &rejected_counters));
      },
      "a point one minimum-subnormal off the support is rejected exactly");
  check(rejected_counters.certified_decisions() == 0U,
        "an orientation-domain rejection leaves counters unchanged");
}

void test_adaptive_orientation_2d_and_plane_side() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  const ExactPlane3 xy = ExactPlane3::through_points(origin, e1, e2);

  PredicateCounters positive_counters;
  const auto positive = orientation_2d_in_plane(
      xy, origin, e1, e2, &positive_counters);
  PredicateCounters negative_counters;
  const auto negative = orientation_2d_in_plane(
      xy, origin, e2, e1, &negative_counters);
  PredicateCounters zero_counters;
  const auto zero = orientation_2d_in_plane(
      xy, origin, e1, point(2.0, 0.0, 0.0), &zero_counters);
  check(
      positive.decision.sign() == PredicateSign::positive &&
          positive.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative.decision.sign() == PredicateSign::negative &&
          negative.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          zero.decision.sign() == PredicateSign::zero &&
          zero.orientation_value.is_zero() &&
          zero.decision.certification_stage() ==
              CertificationStage::expansion,
      "binary64 plane provenance enables FP64 orientation signs and an expansion zero");
  check_single_certification(
      positive_counters,
      CertificationStage::fp64_filtered,
      0U,
      "positive orientation records one FP64 terminal certification");
  check_single_certification(
      negative_counters,
      CertificationStage::fp64_filtered,
      0U,
      "negative orientation records one FP64 terminal certification");
  check_single_certification(
      zero_counters,
      CertificationStage::expansion,
      1U,
      "collinear orientation records one exact expansion zero");

  PredicateCounters policy_counters;
  const auto policy_only = orientation_2d_in_plane(
      xy,
      origin,
      e1,
      e2,
      &policy_counters,
      PredicateFilterPolicy::multiprecision_only);
  PredicateCounters unsourced_counters;
  const auto unsourced = orientation_2d_in_plane(
      plane(0, 0, 1, 0), origin, e1, e2, &unsourced_counters);
  check(
      policy_only.decision.sign() == PredicateSign::positive &&
          policy_only.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          unsourced.decision.sign() == PredicateSign::positive &&
          unsourced.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "orientation policy and absent provenance independently force exact CPU authority");
  check_single_certification(
      policy_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "multiprecision-only orientation records one terminal certification");
  check_single_certification(
      unsourced_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "unsourced orientation records one terminal certification");

  const ExactPlane3 x_zero = binary_plane(1.0, 0.0, 0.0, 0.0);
  PredicateCounters positive_side_counters;
  const auto positive_side =
      plane_side(x_zero, point(1.0, 2.0, 3.0), &positive_side_counters);
  PredicateCounters negative_side_counters;
  const auto negative_side =
      plane_side(x_zero, point(-1.0, 2.0, 3.0), &negative_side_counters);
  PredicateCounters boundary_counters;
  const auto boundary = plane_side(x_zero, origin, &boundary_counters);
  check(
      positive_side.decision.sign() == PredicateSign::positive &&
          positive_side.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative_side.decision.sign() == PredicateSign::negative &&
          negative_side.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          boundary.decision.sign() == PredicateSign::zero &&
          boundary.signed_value.is_zero() &&
          boundary.decision.certification_stage() ==
              CertificationStage::expansion,
      "plane-side companion uses FP64 for strict sides and expansion for its boundary");
  check_single_certification(
      positive_side_counters,
      CertificationStage::fp64_filtered,
      0U,
      "positive plane side records one FP64 terminal certification");
  check_single_certification(
      negative_side_counters,
      CertificationStage::fp64_filtered,
      0U,
      "negative plane side records one FP64 terminal certification");
  check_single_certification(
      boundary_counters,
      CertificationStage::expansion,
      1U,
      "plane boundary records one expansion terminal certification");

  const auto power_classification = classify_affine_form(
      power_bisector_affine_form(
          label({point(2.0, 0.0, 0.0)}),
          label({point(1.0, 0.0, 0.0)})));
  check(
      power_classification.plane().has_value() &&
          plane_side(*power_classification.plane(), origin)
                  .decision.certification_stage() ==
              CertificationStage::fp64_filtered,
      "a classified power-bisector plane retains usable adaptive provenance");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const ExactPlane3 subnormal_xy = ExactPlane3::through_points(
      origin,
      point(subnormal, 0.0, 0.0),
      point(0.0, subnormal, 0.0));
  PredicateCounters subnormal_counters;
  const auto subnormal_orientation = orientation_2d_in_plane(
      subnormal_xy, origin, e1, e2, &subnormal_counters);
  check(
      subnormal_orientation.decision.sign() == PredicateSign::positive &&
          subnormal_orientation.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "an unrepresentable minimum-subnormal cross product fails closed to multiprecision");
  check_single_certification(
      subnormal_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "subnormal orientation records exactly one CPU terminal certification");
}

void test_allow_fp64_policy_skips_expansion() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  const ExactPlane3 x_zero = binary_plane(1.0, 0.0, 0.0, 0.0);
  const ExactPlane3 y_zero = binary_plane(0.0, 1.0, 0.0, 0.0);
  const ExactPlane3 z_zero = binary_plane(0.0, 0.0, 1.0, 0.0);

  PredicateCounters strict_orientation_counters;
  const auto strict_orientation = orientation_2d_in_plane(
      z_zero,
      origin,
      e1,
      e2,
      &strict_orientation_counters,
      PredicateFilterPolicy::allow_fp64);
  PredicateCounters zero_orientation_counters;
  const auto zero_orientation = orientation_2d_in_plane(
      z_zero,
      origin,
      e1,
      point(2.0, 0.0, 0.0),
      &zero_orientation_counters,
      PredicateFilterPolicy::allow_fp64);
  PredicateCounters zero_side_counters;
  const auto zero_side = plane_side(
      z_zero,
      origin,
      &zero_side_counters,
      PredicateFilterPolicy::allow_fp64);
  PredicateCounters singular_counters;
  const auto singular = certified_intersect_three_planes(
      x_zero,
      y_zero,
      binary_plane(1.0, 1.0, 0.0, 0.0),
      &singular_counters,
      PredicateFilterPolicy::allow_fp64);
  PredicateCounters zero_incidence_counters;
  const auto zero_incidence = fourth_plane_incidence(
      x_zero,
      y_zero,
      z_zero,
      binary_plane(1.0, 1.0, 1.0, 0.0),
      &zero_incidence_counters,
      PredicateFilterPolicy::allow_fp64);

  check(
      strict_orientation.decision.sign() == PredicateSign::positive &&
          strict_orientation.decision.certification_stage() ==
              CertificationStage::fp64_filtered,
      "allow_fp64 retains a strict interval certification");
  check(
      zero_orientation.decision.sign() == PredicateSign::zero &&
          zero_orientation.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          zero_side.decision.sign() == PredicateSign::zero &&
          zero_side.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          singular.intersection().kind() ==
              ThreePlaneIntersectionKind::affine_family &&
          singular.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          zero_incidence.decision.sign() == PredicateSign::zero &&
          zero_incidence.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "allow_fp64 bypasses expansion and falls back exactly on affine zeros");
  check_single_certification(
      strict_orientation_counters,
      CertificationStage::fp64_filtered,
      0U,
      "allow_fp64 records one strict interval certification");
  check_single_certification(
      zero_orientation_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      "allow_fp64 orientation zero records one exact fallback");
  check_single_certification(
      zero_side_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      "allow_fp64 plane-side zero records one exact fallback");
  check_single_certification(
      singular_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      "allow_fp64 singular ranks record one exact fallback");
  check_single_certification(
      zero_incidence_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      "allow_fp64 fourth-plane zero records one exact fallback");
}

void test_three_plane_intersections() {
  const ExactPlane3 x_zero = plane(1, 0, 0, 0);
  const ExactPlane3 y_zero = plane(0, 1, 0, 0);
  const ExactPlane3 z_zero = plane(0, 0, 1, 0);
  const auto origin = intersect_three_planes(x_zero, y_zero, z_zero);
  const auto permuted = intersect_three_planes(z_zero, x_zero, y_zero);
  const auto odd_permutation = intersect_three_planes(y_zero, x_zero, z_zero);
  check(origin.kind() == ThreePlaneIntersectionKind::unique &&
            origin.normal_rank() == 3U && origin.augmented_rank() == 3U &&
            origin.affine_dimension() == 0U && origin.point().has_value() &&
            *origin.point() == ExactRational3{} &&
            permuted.point() == origin.point() &&
            odd_permutation.point() == origin.point(),
        "three independent coordinate planes intersect uniquely and permutation-invariantly");

  const BigInt near_parallel_scale = BigInt{1} << 52U;
  const auto near_parallel = intersect_three_planes(
      x_zero,
      ExactPlane3::from_integer_coefficients(
          {near_parallel_scale, BigInt{1}, BigInt{0}, BigInt{0}}),
      z_zero);
  check(near_parallel.kind() == ThreePlaneIntersectionKind::unique &&
            near_parallel.point() == origin.point(),
        "an exactly resolvable near-parallel plane system remains unique");

  const auto thirds = intersect_three_planes(
      plane(1, 1, 1, -1), plane(1, -1, 0, 0), plane(0, 1, -1, 0));
  check(thirds.kind() == ThreePlaneIntersectionKind::unique &&
            thirds.point().has_value() &&
            *thirds.point() ==
                ExactRational3{BigInt{1}, BigInt{1}, BigInt{1}, BigInt{3}},
        "Cramer's rule returns a canonical non-dyadic one-third intersection");

  const auto empty = intersect_three_planes(
      plane(1, 0, 0, 0), plane(1, 0, 0, -1), plane(0, 1, 0, 0));
  check(empty.kind() == ThreePlaneIntersectionKind::empty &&
            empty.normal_rank() == 2U && empty.augmented_rank() == 3U &&
            !empty.point().has_value() && !empty.affine_dimension().has_value(),
        "parallel incompatible planes are classified empty by augmented rank");
  const auto empty_rank_one = intersect_three_planes(
      plane(1, 0, 0, 0), plane(1, 0, 0, -1), plane(-1, 0, 0, 0));
  check(empty_rank_one.kind() == ThreePlaneIntersectionKind::empty &&
            empty_rank_one.normal_rank() == 1U &&
            empty_rank_one.augmented_rank() == 2U,
        "incompatible coincident-normal planes realize the rank-one empty case");

  const auto line = intersect_three_planes(
      plane(1, 0, 0, 0), plane(0, 1, 0, 0), plane(1, 1, 0, 0));
  check(line.kind() == ThreePlaneIntersectionKind::affine_family &&
            line.normal_rank() == 2U && line.augmented_rank() == 2U &&
            line.affine_dimension() == 1U,
        "two independent constraints classify their common line with dimension one");
  const auto coincident = intersect_three_planes(
      plane(1, 0, 0, 0), plane(-1, 0, 0, 0), plane(2, 0, 0, 0));
  check(coincident.kind() == ThreePlaneIntersectionKind::affine_family &&
            coincident.normal_rank() == 1U &&
            coincident.affine_dimension() == 2U,
        "three coincident oriented planes classify their affine plane with dimension two");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(morsehgp3d::exact::ThreePlaneIntersection::empty(1U, 3U)); },
      "adding one augmented column cannot increase rank by two");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(
            morsehgp3d::exact::ThreePlaneIntersection::affine_family(3U));
      },
      "rank three cannot describe a positive-dimensional affine family");
}

void test_certified_three_plane_intersections() {
  const ExactPlane3 x_zero = binary_plane(1.0, 0.0, 0.0, 0.0);
  const ExactPlane3 y_zero = binary_plane(0.0, 1.0, 0.0, 0.0);
  const ExactPlane3 z_zero = binary_plane(0.0, 0.0, 1.0, 0.0);

  PredicateCounters negative_counters;
  const auto negative = certified_intersect_three_planes(
      x_zero, y_zero, z_zero, &negative_counters);
  PredicateCounters positive_counters;
  const auto positive = certified_intersect_three_planes(
      x_zero.opposite(), y_zero, z_zero, &positive_counters);
  PredicateCounters permuted_counters;
  const auto permuted = certified_intersect_three_planes(
      z_zero, x_zero, y_zero, &permuted_counters);
  check(
      negative.intersection().kind() == ThreePlaneIntersectionKind::unique &&
          negative.intersection().point() == ExactRational3{} &&
          negative.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative.canonical_normal_determinant_sign() ==
              PredicateSign::negative &&
          positive.intersection().kind() == ThreePlaneIntersectionKind::unique &&
          positive.intersection().point() == ExactRational3{} &&
          positive.certification_stage() ==
              CertificationStage::fp64_filtered &&
          positive.canonical_normal_determinant_sign() ==
              PredicateSign::positive &&
          positive == negative &&
          permuted == negative &&
          permuted.certification_stage() ==
              CertificationStage::fp64_filtered,
      "certified coordinate-plane intersections realize both determinant signs and canonical permutations at FP64");
  check_single_certification(
      negative_counters,
      CertificationStage::fp64_filtered,
      0U,
      "negative unique intersection records one FP64 terminal certification");
  check_single_certification(
      positive_counters,
      CertificationStage::fp64_filtered,
      0U,
      "positive unique intersection records one FP64 terminal certification");
  check_single_certification(
      permuted_counters,
      CertificationStage::fp64_filtered,
      0U,
      "permuted unique intersection records one FP64 terminal certification");

  const ExactPlane3 sum_zero = binary_plane(1.0, 1.0, 0.0, 0.0);
  PredicateCounters family_counters;
  const auto family = certified_intersect_three_planes(
      x_zero, y_zero, sum_zero, &family_counters);
  const ExactPlane3 shifted_sum = binary_plane(1.0, 1.0, 0.0, -1.0);
  PredicateCounters empty_counters;
  const auto empty = certified_intersect_three_planes(
      x_zero, y_zero, shifted_sum, &empty_counters);
  check(
      family.intersection().kind() ==
              ThreePlaneIntersectionKind::affine_family &&
          family.intersection().normal_rank() == 2U &&
          family.intersection().augmented_rank() == 2U &&
          family.intersection().affine_dimension() == 1U &&
          family.canonical_normal_determinant_sign() == PredicateSign::zero &&
          family.certification_stage() == CertificationStage::expansion &&
          empty.intersection().kind() == ThreePlaneIntersectionKind::empty &&
          empty.intersection().normal_rank() == 2U &&
          empty.intersection().augmented_rank() == 3U &&
          !empty.intersection().affine_dimension().has_value() &&
          empty.canonical_normal_determinant_sign() == PredicateSign::zero &&
          empty.certification_stage() == CertificationStage::expansion,
      "exactly singular affine-family and empty systems are collectively certified by expansion");
  check_single_certification(
      family_counters,
      CertificationStage::expansion,
      1U,
      "affine-family ranks record one expansion terminal certification");
  check_single_certification(
      empty_counters,
      CertificationStage::expansion,
      1U,
      "empty-system ranks record one expansion terminal certification");

  PredicateCounters policy_counters;
  const auto policy_only = certified_intersect_three_planes(
      x_zero,
      y_zero,
      z_zero,
      &policy_counters,
      PredicateFilterPolicy::multiprecision_only);
  PredicateCounters unsourced_counters;
  const auto unsourced = certified_intersect_three_planes(
      plane(1, 0, 0, 0),
      plane(0, 1, 0, 0),
      plane(0, 0, 1, 0),
      &unsourced_counters);
  check(
      policy_only.intersection().kind() ==
              ThreePlaneIntersectionKind::unique &&
          policy_only.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          unsourced.intersection().kind() ==
              ThreePlaneIntersectionKind::unique &&
          unsourced.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "three-plane policy and missing provenance independently select CPU multiprecision");
  check_single_certification(
      policy_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "multiprecision-only intersection records one terminal certification");
  check_single_certification(
      unsourced_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "unsourced intersection records one terminal certification");

  const double subnormal = std::numeric_limits<double>::denorm_min();
  const ExactPlane3 subnormal_x =
      binary_plane(subnormal, 0.0, 0.0, 0.0);
  const ExactPlane3 subnormal_y =
      binary_plane(0.0, subnormal, 0.0, 0.0);
  PredicateCounters subnormal_counters;
  const auto subnormal_unique = certified_intersect_three_planes(
      subnormal_x, subnormal_y, z_zero, &subnormal_counters);
  check(
      subnormal_unique.intersection().kind() ==
              ThreePlaneIntersectionKind::unique &&
          subnormal_unique.intersection().point() == ExactRational3{} &&
          subnormal_unique.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "a determinant below the binary64 expansion floor falls back without changing its exact intersection");
  check_single_certification(
      subnormal_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "subnormal intersection records exactly one CPU terminal certification");

  const auto x_power_classification = classify_affine_form(
      power_bisector_affine_form(
          label({point(-1.0, 0.0, 0.0)}),
          label({point(1.0, 0.0, 0.0)})));
  const ExactPlane3 y_through = ExactPlane3::through_points(
      point(0.0, 0.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(1.0, 0.0, 0.0));
  check(
      x_power_classification.plane().has_value() &&
          certified_intersect_three_planes(
              *x_power_classification.plane(), y_through, z_zero)
                  .certification_stage() ==
              CertificationStage::fp64_filtered,
      "mixed power, through-point, and coefficient recipes certify one common intersection");
}

void test_plane_side_and_fourth_plane_incidence() {
  const ExactPlane3 x_zero = plane(1, 0, 0, 0);
  PredicateCounters side_counters;
  const auto positive_side =
      plane_side(x_zero, point(1.0, 2.0, 3.0), &side_counters);
  const auto negative_side = plane_side(x_zero, point(-1.0, 2.0, 3.0));
  const auto boundary = plane_side(x_zero, ExactRational3{});
  check(positive_side.decision.sign() == PredicateSign::positive &&
            positive_side.signed_value.canonical_key() == "1/1" &&
            side_counters.cpu_multiprecision_certified() == 1U &&
            negative_side.decision.sign() == PredicateSign::negative &&
            boundary.decision.sign() == PredicateSign::zero,
        "plane-side evaluation preserves oriented negative, zero, and positive sides");

  const ExactPlane3 first = plane(1, 1, 1, -1);
  const ExactPlane3 second = plane(1, -1, 0, 0);
  const ExactPlane3 third = plane(0, 1, -1, 0);
  PredicateCounters incidence_counters;
  const auto incident = fourth_plane_incidence(
      first, second, third, plane(1, 1, 1, -1), &incidence_counters);
  const auto positive = fourth_plane_incidence(
      third.opposite(), first, second.opposite(), plane(1, 1, 1, 0));
  const auto negative = fourth_plane_incidence(
      second, third, first.opposite(), plane(-1, -1, -1, 0));
  PredicateCounters decision_counters;
  const auto direct_positive = decide_fourth_plane_incidence(
      third.opposite(), first, second.opposite(), plane(1, 1, 1, 0),
      &decision_counters);
  check(incident.decision.sign() == PredicateSign::zero &&
            incident.signed_value.is_zero() &&
            incident.intersection ==
                ExactRational3{BigInt{1}, BigInt{1}, BigInt{1}, BigInt{3}} &&
            incidence_counters.cpu_multiprecision_certified() == 1U &&
            incidence_counters.exact_zeros() == 1U &&
            positive.decision.sign() == PredicateSign::positive &&
            negative.decision.sign() == PredicateSign::negative &&
            direct_positive.sign() == PredicateSign::positive &&
            decision_counters.cpu_multiprecision_certified() == 1U,
        "fourth-plane incidence is invariant under binding-plane order/orientation");

  PredicateCounters rejected_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(fourth_plane_incidence(
            plane(1, 0, 0, 0),
            plane(0, 1, 0, 0),
            plane(1, 1, 0, 0),
            plane(0, 0, 1, 0),
            &rejected_counters));
      },
      "fourth-plane incidence rejects a nonunique binding intersection");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(decide_fourth_plane_incidence(
            plane(1, 0, 0, 0),
            plane(0, 1, 0, 0),
            plane(1, 1, 0, 0),
            plane(0, 0, 1, 0),
            &rejected_counters));
      },
      "decision-only fourth-plane incidence rejects a nonunique binding system");
  check(rejected_counters.certified_decisions() == 0U,
        "a nonunique fourth-plane request leaves counters unchanged");
}

void test_adaptive_fourth_plane_incidence() {
  const ExactPlane3 x_zero = binary_plane(1.0, 0.0, 0.0, 0.0);
  const ExactPlane3 y_zero = binary_plane(0.0, 1.0, 0.0, 0.0);
  const ExactPlane3 z_zero = binary_plane(0.0, 0.0, 1.0, 0.0);
  const ExactPlane3 positive_plane = binary_plane(1.0, 1.0, 1.0, 1.0);
  const ExactPlane3 negative_plane = binary_plane(1.0, 1.0, 1.0, -1.0);
  const ExactPlane3 incident_plane = binary_plane(1.0, 1.0, 1.0, 0.0);

  PredicateCounters positive_counters;
  const auto positive = fourth_plane_incidence(
      x_zero, y_zero, z_zero, positive_plane, &positive_counters);
  PredicateCounters negative_counters;
  const auto negative = fourth_plane_incidence(
      x_zero, y_zero, z_zero, negative_plane, &negative_counters);
  PredicateCounters zero_counters;
  const auto zero = fourth_plane_incidence(
      x_zero, y_zero, z_zero, incident_plane, &zero_counters);
  check(
      positive.decision.sign() == PredicateSign::positive &&
          positive.signed_value == ExactRational{BigInt{1}} &&
          positive.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          negative.decision.sign() == PredicateSign::negative &&
          negative.signed_value == ExactRational{BigInt{-1}} &&
          negative.decision.certification_stage() ==
              CertificationStage::fp64_filtered &&
          zero.decision.sign() == PredicateSign::zero &&
          zero.signed_value.is_zero() &&
          zero.decision.certification_stage() ==
              CertificationStage::expansion &&
          positive.intersection == ExactRational3{} &&
          negative.intersection == ExactRational3{} &&
          zero.intersection == ExactRational3{},
      "fourth-plane incidence certifies strict signs at FP64 and exact incidence at expansion");
  check_single_certification(
      positive_counters,
      CertificationStage::fp64_filtered,
      0U,
      "positive fourth-plane incidence records one FP64 terminal certification");
  check_single_certification(
      negative_counters,
      CertificationStage::fp64_filtered,
      0U,
      "negative fourth-plane incidence records one FP64 terminal certification");
  check_single_certification(
      zero_counters,
      CertificationStage::expansion,
      1U,
      "zero fourth-plane incidence records one expansion terminal certification");

  PredicateCounters direct_counters;
  const auto direct = decide_fourth_plane_incidence(
      z_zero.opposite(),
      x_zero,
      y_zero.opposite(),
      positive_plane,
      &direct_counters);
  const auto fourth_opposite = fourth_plane_incidence(
      y_zero, z_zero.opposite(), x_zero.opposite(), positive_plane.opposite());
  check(
      direct.sign() == PredicateSign::positive &&
          direct.certification_stage() ==
              CertificationStage::fp64_filtered &&
          fourth_opposite.decision.sign() == PredicateSign::negative &&
          fourth_opposite.decision.certification_stage() ==
              CertificationStage::fp64_filtered,
      "binding permutations and orientations preserve incidence while reversing the fourth plane reverses its sign");
  check_single_certification(
      direct_counters,
      CertificationStage::fp64_filtered,
      0U,
      "decision-only permuted incidence records one FP64 terminal certification");

  PredicateCounters policy_counters;
  const auto policy_only = fourth_plane_incidence(
      x_zero,
      y_zero,
      z_zero,
      positive_plane,
      &policy_counters,
      PredicateFilterPolicy::multiprecision_only);
  check(
      policy_only.decision.sign() == PredicateSign::positive &&
          policy_only.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "multiprecision-only fourth-plane incidence preserves its exact rich witness");
  check_single_certification(
      policy_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "multiprecision-only fourth incidence records one terminal certification");

  const ExactPlane3 sum_zero = binary_plane(1.0, 1.0, 0.0, 0.0);
  PredicateCounters rejected_counters;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(fourth_plane_incidence(
            x_zero,
            y_zero,
            sum_zero,
            z_zero,
            &rejected_counters));
      },
      "adaptive fourth-plane incidence rejects a sourced nonunique binding system");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(decide_fourth_plane_incidence(
            x_zero,
            y_zero,
            sum_zero,
            z_zero,
            &rejected_counters));
      },
      "adaptive decision-only incidence rejects a sourced nonunique binding system");
  check(
      rejected_counters.certified_decisions() == 0U,
      "adaptive incidence precondition failures occur before any terminal counter");
}

void test_affine_adaptive_environment() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  const CertifiedPoint3 e1 = point(1.0, 0.0, 0.0);
  const CertifiedPoint3 e2 = point(0.0, 1.0, 0.0);
  const ExactPlane3 xy = ExactPlane3::through_points(origin, e1, e2);
  const int original_rounding = std::fegetround();
  const std::array<int, 3> altered_rounding_modes{
      FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
  for (const int rounding_mode : altered_rounding_modes) {
    if (std::fesetround(rounding_mode) != 0) {
      continue;
    }
    PredicateCounters counters;
    const auto orientation = orientation_2d_in_plane(
        xy, origin, e1, e2, &counters);
    check(
        orientation.decision.sign() == PredicateSign::positive &&
            orientation.decision.certification_stage() ==
                CertificationStage::cpu_multiprecision &&
            std::fegetround() == rounding_mode,
        "a non-nearest rounding mode forces exact affine fallback and is preserved");
    check_single_certification(
        counters,
        CertificationStage::cpu_multiprecision,
        0U,
        "rounding-mode fallback records one CPU terminal certification");
    check(
        std::fesetround(original_rounding) == 0,
        "the affine rounding-mode test restores the caller mode");
  }

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  constexpr unsigned int flush_to_zero_mask = 1U << 15U;
  constexpr unsigned int denormals_are_zero_mask = 1U << 6U;
  const unsigned int original_mxcsr = _mm_getcsr();
  const unsigned int altered_mxcsr =
      original_mxcsr | flush_to_zero_mask | denormals_are_zero_mask;
  _mm_setcsr(altered_mxcsr);
  PredicateCounters flushed_counters;
  const auto flushed = orientation_2d_in_plane(
      xy, origin, e1, e2, &flushed_counters);
  check(
      flushed.decision.sign() == PredicateSign::positive &&
          flushed.decision.certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          (_mm_getcsr() &
           (flush_to_zero_mask | denormals_are_zero_mask)) ==
              (altered_mxcsr &
               (flush_to_zero_mask | denormals_are_zero_mask)),
      "FTZ and DAZ disable affine floating stages without leaking MXCSR state");
  check_single_certification(
      flushed_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "FTZ/DAZ fallback records one CPU terminal certification");
  _mm_setcsr(original_mxcsr);
#endif
}

}  // namespace

int main() {
  test_affine_form_and_plane_canonicalization();
  test_binary64_affine_provenance();
  test_plane_through_points();
  test_power_bisector_affine_form();
  test_orientation_2d();
  test_adaptive_orientation_2d_and_plane_side();
  test_allow_fp64_policy_skips_expansion();
  test_three_plane_intersections();
  test_certified_three_plane_intersections();
  test_plane_side_and_fourth_plane_incidence();
  test_adaptive_fourth_plane_incidence();
  test_affine_adaptive_environment();

  if (failures != 0) {
    std::cerr << failures << " affine test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D affine exact tests passed\n";
  return 0;
}
