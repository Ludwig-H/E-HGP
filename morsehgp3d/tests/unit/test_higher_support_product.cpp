#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::BigInt;
using morsehgp3d::hierarchy::ExactHigherSupportProductAabbAnalysis;
using morsehgp3d::hierarchy::
    ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::exact_higher_support_product_aabb_analysis;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified;
using morsehgp3d::spatial::ExactDyadicAabb3;

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

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 box(
    double lower_x,
    double lower_y,
    double lower_z,
    double upper_x,
    double upper_y,
    double upper_z) {
  return ExactDyadicAabb3{
      {bits(lower_x), bits(lower_y), bits(lower_z)},
      {bits(upper_x), bits(upper_y), bits(upper_z)}};
}

[[nodiscard]] ExactDyadicAabb3 point_box(
    double x,
    double y,
    double z = 0.0) {
  return box(x, y, z, x, y, z);
}

void test_triangle_gram_cramer_certificates() {
  const std::array<ExactDyadicAabb3, 3> acute{
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 2.0)};
  const ExactHigherSupportProductAabbAnalysis analysis =
      exact_higher_support_product_aabb_analysis(acute);
  check(
      analysis.support_size == 3U,
      "the triangle analysis records its support size");
  check(
      analysis.gram_determinant.lower == ExactRational{BigInt{16}} &&
          analysis.gram_determinant.upper ==
              ExactRational{BigInt{16}},
      "the singleton triangle product has exact Gram determinant sixteen");
  const std::array<long long, 3> expected_numerators{10, 10, 12};
  for (std::size_t index = 0U; index < expected_numerators.size(); ++index) {
    const ExactRational expected{BigInt{expected_numerators[index]}};
    check(
        analysis.barycentric_numerators[index].lower == expected &&
            analysis.barycentric_numerators[index].upper == expected,
        "the acute triangle exposes its exact positive Cramer numerator");
  }
  check(
      !analysis.no_well_centered_support_certified(),
      "an acute singleton triangle product is not pruned");
  check(
      !analysis.query_strictly_inside_every_independent_sphere_certified(),
      "an omitted query never fabricates an interior certificate");

  const std::array<ExactDyadicAabb3, 3> obtuse_product{
      point_box(0.0, 0.0),
      point_box(2.0, 0.0),
      box(0.25, 0.0, 0.0, 0.5, 0.1, 0.0)};
  const auto obtuse =
      exact_higher_support_product_aabb_analysis(obtuse_product);
  check(
      obtuse.no_well_centered_support_certified(),
      "a full box product whose third angle is nonacute is pruned");
  check(
      obtuse.barycentric_numerators[2].upper.sign() < 0,
      "the obtuse product prune carries a strictly negative numerator upper bound");

  const std::array<ExactDyadicAabb3, 3> collinear{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(2.0, 0.0)};
  const auto dependent =
      exact_higher_support_product_aabb_analysis(collinear);
  check(
      dependent.all_supports_affinely_dependent_certified() &&
          dependent.no_well_centered_support_certified(),
      "a singleton collinear product is rejected by its zero Gram determinant");
}

void test_triangle_scaled_power_certificate() {
  const std::array<ExactDyadicAabb3, 3> support{
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 2.0)};
  const ExactDyadicAabb3 interior_query =
      box(-0.1, 0.7, 0.0, 0.1, 0.8, 0.0);
  const auto interior = exact_higher_support_product_aabb_analysis(
      support, interior_query);
  check(
      interior.query_scaled_power.has_value() &&
          interior.query_scaled_power->upper.sign() < 0 &&
          interior.query_strictly_inside_every_independent_sphere_certified(),
      "one rational power interval certifies a whole query box strictly interior");

  const ExactDyadicAabb3 convexity_query =
      box(-0.75, 0.25, 0.0, 0.75, 1.25, 0.0);
  const auto convexity = exact_higher_support_product_aabb_analysis(
      support, convexity_query);
  check(
      convexity.query_scaled_power.has_value() &&
          convexity.query_scaled_power->upper ==
              ExactRational{BigInt{-12}},
      "query-box convexity recovers the exact negative corner maximum");

  const auto boundary = exact_higher_support_product_aabb_analysis(
      support, support[0]);
  check(
      boundary.query_scaled_power.has_value() &&
          boundary.query_scaled_power->lower.is_zero() &&
          boundary.query_scaled_power->upper.is_zero() &&
          !boundary.query_strictly_inside_every_independent_sphere_certified(),
      "sphere equality is never accepted as a strict-interior witness");
}

void test_tetrahedron_gram_cramer_certificates() {
  const std::array<ExactDyadicAabb3, 4> regular{
      point_box(1.0, 1.0, 1.0),
      point_box(1.0, -1.0, -1.0),
      point_box(-1.0, 1.0, -1.0),
      point_box(-1.0, -1.0, 1.0)};
  const auto regular_analysis =
      exact_higher_support_product_aabb_analysis(
          regular, point_box(0.0, 0.0, 0.0));
  check(
      !regular_analysis.no_well_centered_support_certified(),
      "a regular tetrahedron remains in the support stream");
  for (std::size_t index = 0U; index < regular.size(); ++index) {
    check(
        regular_analysis.barycentric_numerators[index].lower.sign() > 0,
        "each regular-tetrahedron circumcentre barycentric is positive");
  }
  check(
      regular_analysis
          .query_strictly_inside_every_independent_sphere_certified(),
      "the regular tetrahedron centre is certified strictly inside its sphere");
  const auto boundary = exact_higher_support_product_aabb_analysis(
      regular, regular[2]);
  check(
      boundary.query_scaled_power.has_value() &&
          boundary.query_scaled_power->upper.is_zero() &&
          !boundary.query_strictly_inside_every_independent_sphere_certified(),
      "a tetrahedron support point remains a non-strict shell witness");

  const std::array<ExactDyadicAabb3, 4> exterior{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, 1.0)};
  const auto exterior_analysis =
      exact_higher_support_product_aabb_analysis(exterior);
  check(
      exterior_analysis.no_well_centered_support_certified(),
      "an orthogonal tetrahedron with exterior circumcentre is pruned");
  check(
      exterior_analysis.barycentric_numerators[0].upper.sign() < 0,
      "the exterior tetrahedron prune identifies the anchor barycentric numerator");
}

void test_support_box_corner_regressions() {
  const std::array<ExactDyadicAabb3, 3> well_centering_product{
      box(-2.0, 2.0, 0.0, 2.0, 2.0, 0.0),
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0)};
  const auto whole = exact_higher_support_product_aabb_analysis(
      well_centering_product);
  check(
      !whole.no_well_centered_support_certified(),
      "nonacute support-box corners do not hide the acute interior triangle");
  std::array<ExactDyadicAabb3, 3> singleton = well_centering_product;
  singleton[0] = point_box(-2.0, 2.0);
  check(
      exact_higher_support_product_aabb_analysis(singleton)
          .no_well_centered_support_certified(),
      "the negative endpoint triangle is individually non-well-centred");
  singleton[0] = point_box(2.0, 2.0);
  check(
      exact_higher_support_product_aabb_analysis(singleton)
          .no_well_centered_support_certified(),
      "the positive endpoint triangle is individually non-well-centred");
  singleton[0] = point_box(0.0, 2.0);
  check(
      !exact_higher_support_product_aabb_analysis(singleton)
           .no_well_centered_support_certified(),
      "the interior triangle is exactly well-centred");

  const std::array<ExactDyadicAabb3, 3> power_product{
      box(-0.5, 2.0, 0.0, 0.5, 2.0, 0.0),
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0)};
  const ExactDyadicAabb3 query = point_box(0.0, 33.0 / 16.0);
  const auto universal = exact_higher_support_product_aabb_analysis(
      power_product, query);
  check(
      !universal
           .query_strictly_inside_every_independent_sphere_certified(),
      "negative endpoint powers do not fabricate a universal interior witness");
  singleton = power_product;
  singleton[0] = point_box(-0.5, 2.0);
  check(
      exact_higher_support_product_aabb_analysis(singleton, query)
          .query_strictly_inside_every_independent_sphere_certified(),
      "the negative power at the left support corner is recognized");
  singleton[0] = point_box(0.5, 2.0);
  check(
      exact_higher_support_product_aabb_analysis(singleton, query)
          .query_strictly_inside_every_independent_sphere_certified(),
      "the negative power at the right support corner is recognized");
  singleton[0] = point_box(0.0, 2.0);
  check(
      !exact_higher_support_product_aabb_analysis(singleton, query)
           .query_strictly_inside_every_independent_sphere_certified(),
      "the positive power at the interior support is not lost");
}

void test_bounded_decision_parity() {
  const std::array<ExactDyadicAabb3, 3> acute{
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 2.0)};
  const std::array<ExactDyadicAabb3, 3> obtuse{
      point_box(0.0, 0.0),
      point_box(2.0, 0.0),
      point_box(0.25, 0.05)};
  const std::array<ExactDyadicAabb3, 4> regular{
      point_box(1.0, 1.0, 1.0),
      point_box(1.0, -1.0, -1.0),
      point_box(-1.0, 1.0, -1.0),
      point_box(-1.0, -1.0, 1.0)};
  ExactHigherSupportProductAabbDecisionBackend backend =
      ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational;

  const bool acute_decision =
      exact_higher_support_product_no_well_centered_certified(
          acute,
          &backend);
  check(
      backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024 &&
          acute_decision ==
              exact_higher_support_product_aabb_analysis(acute)
                  .no_well_centered_support_certified(),
      "the bounded triangle non-prune matches the rational interval DAG");

  const bool obtuse_decision =
      exact_higher_support_product_no_well_centered_certified(
          obtuse,
          &backend);
  check(
      backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024 &&
          obtuse_decision ==
              exact_higher_support_product_aabb_analysis(obtuse)
                  .no_well_centered_support_certified(),
      "the bounded triangle prune matches the rational interval DAG");

  const ExactDyadicAabb3 center = point_box(0.0, 0.0, 0.0);
  const bool tetrahedron_decision =
      exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          regular,
          center,
          &backend);
  check(
      backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024 &&
          tetrahedron_decision ==
              exact_higher_support_product_aabb_analysis(
                  regular,
                  center)
                  .query_strictly_inside_every_independent_sphere_certified(),
      "the bounded tetrahedron power decision matches the rational interval DAG");
}

void test_bounded_decision_equalities_and_support_box_regression() {
  const std::array<ExactDyadicAabb3, 3> support{
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 2.0)};
  ExactHigherSupportProductAabbDecisionBackend backend =
      ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational;
  check(
      !exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          support,
          support[0],
          &backend) &&
          backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024,
      "the bounded kernel rejects exact sphere equality as strict interior");

  const std::array<ExactDyadicAabb3, 3> collinear{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(2.0, 0.0)};
  check(
      exact_higher_support_product_no_well_centered_certified(
          collinear,
          &backend) &&
          backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024,
      "the bounded kernel accepts a zero Gram upper bound for the non-strict prune");

  const std::array<ExactDyadicAabb3, 3> power_product{
      box(-0.5, 2.0, 0.0, 0.5, 2.0, 0.0),
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0)};
  const ExactDyadicAabb3 query = point_box(0.0, 33.0 / 16.0);
  check(
      !exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          power_product,
          query,
          &backend) &&
          backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  bounded_dyadic_int1024,
      "the bounded full support-box interval preserves the corner regression");
}

void test_rational_decision_fallback() {
  const double tiny = std::ldexp(1.0, -200);
  const std::array<ExactDyadicAabb3, 3> support{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, tiny)};
  const ExactDyadicAabb3 query = point_box(0.5, tiny / 2.0);
  ExactHigherSupportProductAabbDecisionBackend backend =
      ExactHigherSupportProductAabbDecisionBackend::
          bounded_dyadic_int1024;

  const bool no_well_centered =
      exact_higher_support_product_no_well_centered_certified(
          support,
          &backend);
  check(
      backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  arbitrary_precision_rational &&
          no_well_centered ==
              exact_higher_support_product_aabb_analysis(support)
                  .no_well_centered_support_certified(),
      "an exponent span above 124 bits falls back integrally for well-centring");

  const bool strictly_inside =
      exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          support,
          query,
          &backend);
  check(
      backend ==
              ExactHigherSupportProductAabbDecisionBackend::
                  arbitrary_precision_rational &&
          strictly_inside ==
              exact_higher_support_product_aabb_analysis(
                  support,
                  query)
                  .query_strictly_inside_every_independent_sphere_certified(),
      "an exponent span above 124 bits falls back integrally for sphere power");
}

void test_input_contract() {
  const std::array<ExactDyadicAabb3, 2> too_small{
      point_box(0.0, 0.0), point_box(1.0, 0.0)};
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            exact_higher_support_product_aabb_analysis(too_small));
      },
      "two boxes are outside the higher-support contract");

  std::array<ExactDyadicAabb3, 3> reversed{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 1.0)};
  reversed[1].lower_binary64_bits[0] = bits(2.0);
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            exact_higher_support_product_aabb_analysis(reversed));
      },
      "a reversed dyadic AABB fails closed");
}

}  // namespace

int main() {
  test_triangle_gram_cramer_certificates();
  test_triangle_scaled_power_certificate();
  test_tetrahedron_gram_cramer_certificates();
  test_support_box_corner_regressions();
  test_bounded_decision_parity();
  test_bounded_decision_equalities_and_support_box_regression();
  test_rational_decision_fallback();
  test_input_contract();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "higher-support product tests passed\n";
  return 0;
}
