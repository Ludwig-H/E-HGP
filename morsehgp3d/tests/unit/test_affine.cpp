#include "morsehgp3d/exact/plane.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::AffineFormKind;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactPlane3;
using morsehgp3d::exact::ExactPlane3Record;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::ThreePlaneIntersectionKind;
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

}  // namespace

int main() {
  test_affine_form_and_plane_canonicalization();
  test_plane_through_points();
  test_power_bisector_affine_form();
  test_orientation_2d();
  test_three_plane_intersections();
  test_plane_side_and_fourth_plane_incidence();

  if (failures != 0) {
    std::cerr << failures << " affine test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D affine exact tests passed\n";
  return 0;
}
