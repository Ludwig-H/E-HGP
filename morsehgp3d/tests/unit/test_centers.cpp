#include "morsehgp3d/exact/center.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <string>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterKind;
using morsehgp3d::exact::CircumcenterResult;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::circumcenter;
using morsehgp3d::exact::power_of_two;

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

ExactRational3 point(long long x, long long y, long long z) {
  return ExactRational3{BigInt{x}, BigInt{y}, BigInt{z}, BigInt{1}};
}

CertifiedPoint3 binary_point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

ExactRational3 rational_point(
    ExactRational x, ExactRational y, ExactRational z) {
  return ExactRational3{std::array<ExactRational, 3>{
      std::move(x), std::move(y), std::move(z)}};
}

ExactLevel squared_distance(
    const ExactRational3& left, const ExactRational3& right) {
  ExactRational value;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const ExactRational delta = left.coordinate(axis) - right.coordinate(axis);
    value = value + delta * delta;
  }
  return ExactLevel{std::move(value)};
}

template <std::size_t SupportSize>
CircumcenterResult center_of(
    const std::array<ExactRational3, SupportSize>& support) {
  static_assert(SupportSize >= 2U && SupportSize <= 4U);
  if constexpr (SupportSize == 2U) {
    return circumcenter(support[0], support[1]);
  } else if constexpr (SupportSize == 3U) {
    return circumcenter(support[0], support[1], support[2]);
  } else {
    return circumcenter(support[0], support[1], support[2], support[3]);
  }
}

template <std::size_t SupportSize>
CircumcenterResult center_of(
    const std::array<CertifiedPoint3, SupportSize>& support) {
  static_assert(SupportSize >= 2U && SupportSize <= 4U);
  if constexpr (SupportSize == 2U) {
    return circumcenter(support[0], support[1]);
  } else if constexpr (SupportSize == 3U) {
    return circumcenter(support[0], support[1], support[2]);
  } else {
    return circumcenter(support[0], support[1], support[2], support[3]);
  }
}

void check_unique(
    const CircumcenterResult& result,
    std::size_t support_size,
    const ExactRational3& expected_center,
    const ExactLevel& expected_level,
    const std::string& message) {
  check(
      result.kind() == CircumcenterKind::unique &&
          result.support_size() == support_size &&
          result.affine_dimension() + 1U == support_size &&
          result.center().has_value() && result.squared_level().has_value(),
      message + " has a structurally valid unique result");
  if (result.center().has_value()) {
    check(*result.center() == expected_center, message + " has the expected exact center");
  }
  if (result.squared_level().has_value()) {
    check(
        *result.squared_level() == expected_level,
        message + " has the expected exact squared level");
  }
}

void check_dependent(
    const CircumcenterResult& result,
    std::size_t support_size,
    std::size_t affine_dimension,
    const std::string& message) {
  check(
      result.kind() == CircumcenterKind::affinely_dependent &&
          result.support_size() == support_size &&
          result.affine_dimension() == affine_dimension &&
          !result.center().has_value() && !result.squared_level().has_value(),
      message);
}

template <std::size_t SupportSize>
void check_equidistant(
    const CircumcenterResult& result,
    const std::array<ExactRational3, SupportSize>& support,
    const std::string& message) {
  if (!result.center().has_value() || !result.squared_level().has_value()) {
    check(false, message + " lacks its exact witnesses");
    return;
  }
  for (std::size_t index = 0; index < support.size(); ++index) {
    check(
        squared_distance(*result.center(), support[index]) ==
            *result.squared_level(),
        message + " is exactly equidistant from support point " +
            std::to_string(index));
  }
}

ExactRational3 translate(
    const ExactRational3& value, const ExactRational3& delta) {
  std::array<ExactRational, 3> coordinates{};
  for (std::size_t axis = 0; axis < coordinates.size(); ++axis) {
    coordinates[axis] = value.coordinate(axis) + delta.coordinate(axis);
  }
  return ExactRational3{coordinates};
}

ExactRational3 scale(const ExactRational3& value, const ExactRational& factor) {
  std::array<ExactRational, 3> coordinates{};
  for (std::size_t axis = 0; axis < coordinates.size(); ++axis) {
    coordinates[axis] = value.coordinate(axis) * factor;
  }
  return ExactRational3{coordinates};
}

ExactRational3 signed_axis_permutation(const ExactRational3& value) {
  return rational_point(
      -value.coordinate(2U), value.coordinate(0U), -value.coordinate(1U));
}

void test_two_point_centers() {
  const std::array<ExactRational3, 2> support{
      point(-1, 2, 3), point(3, -2, 5)};
  const CircumcenterResult result = center_of(support);
  check_unique(result, 2U, point(1, 0, 4), ExactLevel{BigInt{9}}, "two-point support");
  check_equidistant(result, support, "two-point support");
  check(center_of(std::array<ExactRational3, 2>{support[1], support[0]}) == result,
        "swapping a two-point support preserves its result");

  const std::array<ExactRational3, 2> rational_support{
      rational_point(
          ExactRational{BigInt{1}, BigInt{3}},
          ExactRational{BigInt{-1}, BigInt{5}},
          ExactRational{}),
      rational_point(
          ExactRational{BigInt{2}, BigInt{3}},
          ExactRational{BigInt{4}, BigInt{5}},
          ExactRational{BigInt{2}, BigInt{7}})};
  const CircumcenterResult rational_result = center_of(rational_support);
  check_unique(
      rational_result,
      2U,
      rational_point(
          ExactRational{BigInt{1}, BigInt{2}},
          ExactRational{BigInt{3}, BigInt{10}},
          ExactRational{BigInt{1}, BigInt{7}}),
      ExactLevel{BigInt{263}, BigInt{882}},
      "non-dyadic rational pair");
  check_equidistant(rational_result, rational_support, "non-dyadic rational pair");

  const std::array<CertifiedPoint3, 2> binary_support{
      binary_point(-1.0, 2.0, 3.0), binary_point(3.0, -2.0, 5.0)};
  check(
      center_of(binary_support) == result,
      "the CertifiedPoint3 pair overload preserves the exact rational result");

  check_dependent(
      circumcenter(point(7, -3, 2), point(7, -3, 2)),
      2U,
      0U,
      "an identical pair is classified with affine dimension zero");
  check_dependent(
      circumcenter(
          binary_point(0.0, -0.0, 0.0), binary_point(-0.0, 0.0, -0.0)),
      2U,
      0U,
      "signed binary64 zeroes do not manufacture a distinct support");
}

void test_three_point_centers() {
  const std::array<ExactRational3, 3> non_dyadic{
      point(0, 0, 0), point(2, 0, 0), point(1, 3, 0)};
  const CircumcenterResult non_dyadic_result = center_of(non_dyadic);
  check_unique(
      non_dyadic_result,
      3U,
      rational_point(
          ExactRational{BigInt{1}},
          ExactRational{BigInt{4}, BigInt{3}},
          ExactRational{}),
      ExactLevel{BigInt{25}, BigInt{9}},
      "integer triangle with a non-dyadic circumcenter");
  check_equidistant(
      non_dyadic_result, non_dyadic, "integer triangle with a non-dyadic circumcenter");

  const std::array<ExactRational3, 3> obtuse{
      point(0, 0, 0), point(4, 0, 0), point(1, 1, 0)};
  const CircumcenterResult obtuse_result = center_of(obtuse);
  check_unique(
      obtuse_result,
      3U,
      point(2, -1, 0),
      ExactLevel{BigInt{5}},
      "obtuse triangle");
  check_equidistant(obtuse_result, obtuse, "obtuse triangle");
  if (obtuse_result.center().has_value()) {
    check(
        obtuse_result.center()->coordinate(1U).sign() < 0,
        "an obtuse support remains valid even when its center lies outside the triangle");
  }

  std::array<std::size_t, 3> permutation{0U, 1U, 2U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<ExactRational3, 3> permuted{
        non_dyadic[permutation[0]],
        non_dyadic[permutation[1]],
        non_dyadic[permutation[2]]};
    check(
        center_of(permuted) == non_dyadic_result,
        "triangle result is invariant under support permutation " +
            std::to_string(permutation_count));
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 6U, "all six triangle permutations were exercised");

  check_dependent(
      circumcenter(point(1, 1, 1), point(1, 1, 1), point(1, 1, 1)),
      3U,
      0U,
      "three identical points have affine dimension zero");
  check_dependent(
      circumcenter(point(0, 0, 0), point(1, 2, 3), point(2, 4, 6)),
      3U,
      1U,
      "three distinct collinear points have affine dimension one");
  check_dependent(
      circumcenter(point(0, 0, 0), point(0, 0, 0), point(0, 1, 0)),
      3U,
      1U,
      "a duplicate plus a distinct point has affine dimension one");
}

void test_four_point_centers() {
  const std::array<ExactRational3, 4> standard{
      point(0, 0, 0), point(1, 0, 0), point(0, 1, 0), point(0, 0, 1)};
  const CircumcenterResult standard_result = center_of(standard);
  check_unique(
      standard_result,
      4U,
      ExactRational3{BigInt{1}, BigInt{1}, BigInt{1}, BigInt{2}},
      ExactLevel{BigInt{3}, BigInt{4}},
      "standard tetrahedron");
  check_equidistant(standard_result, standard, "standard tetrahedron");

  const std::array<ExactRational3, 4> non_dyadic{
      point(0, 0, 0), point(1, 0, 0), point(0, 1, 0), point(2, 0, 3)};
  const CircumcenterResult non_dyadic_result = center_of(non_dyadic);
  check_unique(
      non_dyadic_result,
      4U,
      ExactRational3{BigInt{3}, BigInt{3}, BigInt{11}, BigInt{6}},
      ExactLevel{BigInt{139}, BigInt{36}},
      "integer tetrahedron with a non-dyadic circumcenter");
  check_equidistant(
      non_dyadic_result,
      non_dyadic,
      "integer tetrahedron with a non-dyadic circumcenter");

  std::array<std::size_t, 4> permutation{0U, 1U, 2U, 3U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<ExactRational3, 4> permuted{
        non_dyadic[permutation[0]],
        non_dyadic[permutation[1]],
        non_dyadic[permutation[2]],
        non_dyadic[permutation[3]]};
    check(
        center_of(permuted) == non_dyadic_result,
        "tetrahedron result is invariant under support permutation " +
            std::to_string(permutation_count));
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 24U, "all 24 tetrahedron permutations were exercised");

  check_dependent(
      circumcenter(point(2, 2, 2), point(2, 2, 2), point(2, 2, 2), point(2, 2, 2)),
      4U,
      0U,
      "four identical points have affine dimension zero");
  check_dependent(
      circumcenter(point(0, 0, 0), point(1, 1, 1), point(2, 2, 2), point(-1, -1, -1)),
      4U,
      1U,
      "four collinear points have affine dimension one");
  check_dependent(
      circumcenter(point(0, 0, 0), point(1, 0, 0), point(0, 1, 0), point(1, 1, 0)),
      4U,
      2U,
      "a coplanar cocyclic quadruplet remains a dependent size-four support");
}

void test_exact_metamorphisms() {
  const std::array<ExactRational3, 4> support{
      point(0, 0, 0), point(1, 0, 0), point(0, 1, 0), point(2, 0, 3)};
  const CircumcenterResult base = center_of(support);
  check(base.center().has_value() && base.squared_level().has_value(),
        "the metamorphic base support has both witnesses");
  if (!base.center().has_value() || !base.squared_level().has_value()) {
    return;
  }

  const ExactRational3 translation = rational_point(
      ExactRational{BigInt{8}},
      ExactRational{BigInt{-4}},
      ExactRational{BigInt{1}, BigInt{2}});
  std::array<ExactRational3, 4> translated{};
  for (std::size_t index = 0; index < support.size(); ++index) {
    translated[index] = translate(support[index], translation);
  }
  check_unique(
      center_of(translated),
      4U,
      translate(*base.center(), translation),
      *base.squared_level(),
      "exactly representable dyadic translation");

  std::array<ExactRational3, 4> axes_transformed{};
  for (std::size_t index = 0; index < support.size(); ++index) {
    axes_transformed[index] = signed_axis_permutation(support[index]);
  }
  check_unique(
      center_of(axes_transformed),
      4U,
      signed_axis_permutation(*base.center()),
      *base.squared_level(),
      "signed axis permutation");

  const ExactRational scale_factor{BigInt{8}};
  std::array<ExactRational3, 4> scaled{};
  for (std::size_t index = 0; index < support.size(); ++index) {
    scaled[index] = scale(support[index], scale_factor);
  }
  check_unique(
      center_of(scaled),
      4U,
      scale(*base.center(), scale_factor),
      ExactLevel{
          base.squared_level()->rational() * scale_factor * scale_factor},
      "power-of-two homothety");
}

void test_extreme_binary64_inputs() {
  const double subnormal = std::numeric_limits<double>::denorm_min();
  const std::array<CertifiedPoint3, 2> subnormal_support{
      binary_point(0.0, 0.0, 0.0), binary_point(subnormal, 0.0, 0.0)};
  const CircumcenterResult subnormal_result = center_of(subnormal_support);
  check_unique(
      subnormal_result,
      2U,
      rational_point(
          ExactRational{BigInt{1}, power_of_two(1075U)},
          ExactRational{},
          ExactRational{}),
      ExactLevel{BigInt{1}, power_of_two(2150U)},
      "minimum-subnormal pair");

  const double maximum = std::numeric_limits<double>::max();
  const std::array<CertifiedPoint3, 2> maximum_support{
      binary_point(maximum, 0.0, 0.0), binary_point(-maximum, 0.0, 0.0)};
  const ExactRational maximum_exact = ExactRational::from_binary64(maximum);
  check_unique(
      center_of(maximum_support),
      2U,
      ExactRational3{},
      ExactLevel{maximum_exact * maximum_exact},
      "opposite maximum-finite coordinates");

  const double one_ulp_above_one = std::nextafter(1.0, 2.0);
  const std::array<CertifiedPoint3, 4> quasi_coplanar{
      binary_point(0.0, 0.0, 1.0),
      binary_point(1.0, 0.0, 1.0),
      binary_point(0.0, 1.0, 1.0),
      binary_point(0.0, 0.0, one_ulp_above_one)};
  const CircumcenterResult quasi_coplanar_result = center_of(quasi_coplanar);
  check_unique(
      quasi_coplanar_result,
      4U,
      rational_point(
          ExactRational{BigInt{1}, BigInt{2}},
          ExactRational{BigInt{1}, BigInt{2}},
          ExactRational{power_of_two(53U) + 1, power_of_two(53U)}),
      ExactLevel{power_of_two(105U) + 1, power_of_two(106U)},
      "tetrahedron one ULP away from coplanarity");
  check(
      quasi_coplanar_result.affine_dimension() == 3U,
      "one binary64 ULP of height is not collapsed to a coplanar support");
}

void test_result_invariants_and_domain_errors() {
  check(
      morsehgp3d::exact::to_string(CircumcenterKind::unique) == "unique" &&
          morsehgp3d::exact::to_string(CircumcenterKind::affinely_dependent) ==
              "affinely_dependent",
      "circumcenter kinds have stable diagnostic spellings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::exact::to_string(
            static_cast<CircumcenterKind>(99)));
      },
      "an invalid circumcenter kind is rejected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::unique(
            1U, ExactRational3{}, ExactLevel{}));
      },
      "a unique result rejects a support smaller than two");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::unique(
            5U, ExactRational3{}, ExactLevel{}));
      },
      "a unique result rejects a support larger than four");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::unique(
            2U, ExactRational3{}, ExactLevel{}));
      },
      "a unique support cannot expose a zero squared radius");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::affinely_dependent(2U, 1U));
      },
      "a dependent pair cannot claim full affine dimension");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::affinely_dependent(4U, 3U));
      },
      "a dependent tetrahedral support cannot claim dimension three");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(CircumcenterResult::affinely_dependent(1U, 0U));
      },
      "a dependent result rejects an unsupported cardinality");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::exact::center_detail::equidistance_plane(
            point(1, 2, 3), point(1, 2, 3)));
      },
      "identical points do not define an arbitrary equidistance plane");
}

}  // namespace

int main() {
  test_two_point_centers();
  test_three_point_centers();
  test_four_point_centers();
  test_exact_metamorphisms();
  test_extreme_binary64_inputs();
  test_result_invariants_and_domain_errors();

  if (failures != 0) {
    std::cerr << failures << " center test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D exact center tests passed\n";
  return 0;
}
