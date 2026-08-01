#include "phase15_exact_higher_support_product_fixed.cuh"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>

namespace {

namespace fixed =
    morsehgp3d::gpu::detail::exact_higher_support_product_fixed;
using morsehgp3d::hierarchy::
    ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified;
using morsehgp3d::spatial::ExactDyadicAabb3;

static_assert(fixed::limb_count == 16U);
static_assert(fixed::fixed_bit_count == 1024U);
static_assert(fixed::aligned_coordinate_bit_limit == 124U);
static_assert(fixed::proven_maximum_expression_bit_count == 1013U);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
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

[[nodiscard]] fixed::Binary64Aabb3 fixed_box(
    const ExactDyadicAabb3& source) {
  fixed::Binary64Aabb3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower[axis] = source.lower_binary64_bits[axis];
    result.upper[axis] = source.upper_binary64_bits[axis];
  }
  return result;
}

template <std::size_t Size>
[[nodiscard]] std::array<fixed::Binary64Aabb3, 4> fixed_boxes(
    const std::array<ExactDyadicAabb3, Size>& source) {
  static_assert(Size == 3U || Size == 4U);
  std::array<fixed::Binary64Aabb3, 4> result{};
  for (std::size_t index = 0U; index < Size; ++index) {
    result[index] = fixed_box(source[index]);
  }
  return result;
}

[[nodiscard]] bool certified(fixed::Decision decision) {
  return decision == fixed::Decision::certified;
}

template <std::size_t Size>
void check_support_parity(
    const std::array<ExactDyadicAabb3, Size>& boxes,
    const std::string& label) {
  const auto native = fixed_boxes(boxes);
  ExactHigherSupportProductAabbDecisionBackend backend{};
  const bool expected =
      exact_higher_support_product_no_well_centered_certified(
          boxes, &backend);
  const fixed::Decision actual =
      fixed::no_well_centered_support(native.data(), Size);
  if (backend == ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational) {
    check(
        actual == fixed::Decision::requires_cpu_rational_fallback,
        label + " returns the integral fallback sentinel");
  } else {
    check(
        actual != fixed::Decision::requires_cpu_rational_fallback &&
            certified(actual) == expected,
        label + " matches the CPU int1024 decision DAG");
  }
}

template <std::size_t Size>
void check_query_parity(
    const std::array<ExactDyadicAabb3, Size>& boxes,
    const ExactDyadicAabb3& query,
    const std::string& label) {
  const auto native = fixed_boxes(boxes);
  const fixed::Binary64Aabb3 native_query = fixed_box(query);
  ExactHigherSupportProductAabbDecisionBackend backend{};
  const bool expected =
      exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          boxes, query, &backend);
  const fixed::Decision actual =
      fixed::query_strictly_inside_every_independent_sphere(
          native.data(), Size, native_query);
  if (backend == ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational) {
    check(
        actual == fixed::Decision::requires_cpu_rational_fallback,
        label + " returns the integral fallback sentinel");
  } else {
    check(
        actual != fixed::Decision::requires_cpu_rational_fallback &&
            certified(actual) == expected,
        label + " matches the CPU int1024 decision DAG");
  }
}

[[nodiscard]] boost::multiprecision::cpp_int cpp_value(
    const fixed::UInt1024& value) {
  boost::multiprecision::cpp_int result = 0;
  for (std::size_t index = fixed::limb_count; index != 0U; --index) {
    result <<= 64U;
    result += value.limb[index - 1U];
  }
  return result;
}

[[nodiscard]] fixed::UInt1024 power_of_two(unsigned int exponent) {
  fixed::UInt1024 value{};
  value.limb[exponent / 64U] = UINT64_C(1) << (exponent % 64U);
  return value;
}

void test_checked_uint1024_arithmetic() {
  fixed::UInt1024 low_512_ones{};
  for (std::size_t index = 0U; index < 8U; ++index) {
    low_512_ones.limb[index] = UINT64_MAX;
  }
  fixed::UInt1024 square{};
  check(
      fixed::multiply(low_512_ones, low_512_ones, square),
      "the 16-limb schoolbook product accepts (2^512-1)^2");
  const boost::multiprecision::cpp_int expected =
      (boost::multiprecision::cpp_int{1} << 512U) - 1;
  check(
      cpp_value(square) == expected * expected &&
          fixed::bit_width(square) == 1024U,
      "the full-width carry chain equals the arbitrary-precision product");

  const fixed::UInt1024 bit_506 = power_of_two(506U);
  fixed::UInt1024 boundary{};
  check(
      fixed::multiply(bit_506, bit_506, boundary) &&
          fixed::bit_width(boundary) ==
              fixed::proven_maximum_expression_bit_count,
      "a 1013-bit result fits the proved higher-support envelope");

  const fixed::UInt1024 bit_600 = power_of_two(600U);
  const fixed::UInt1024 bit_424 = power_of_two(424U);
  fixed::UInt1024 overflow{};
  check(
      !fixed::multiply(bit_600, bit_424, overflow),
      "a bit 1024 product fails closed instead of wrapping");

  fixed::UInt1024 all_ones{};
  for (std::uint64_t& limb : all_ones.limb) {
    limb = UINT64_MAX;
  }
  fixed::UInt1024 one{};
  one.limb[0] = 1U;
  fixed::UInt1024 sum{};
  check(
      !fixed::add(all_ones, one, sum),
      "a 16-limb addition carry-out fails closed");
}

void test_named_triangle_and_tetrahedron_fixtures() {
  const std::array<ExactDyadicAabb3, 3> acute{
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, 2.0)};
  const std::array<ExactDyadicAabb3, 3> obtuse{
      point_box(0.0, 0.0),
      point_box(2.0, 0.0),
      point_box(0.25, 0.05)};
  const std::array<ExactDyadicAabb3, 3> collinear{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(2.0, 0.0)};
  check_support_parity(acute, "acute triangle non-prune");
  check_support_parity(obtuse, "obtuse triangle prune");
  check_support_parity(collinear, "zero-Gram triangle equality");
  check_query_parity(
      acute, point_box(0.0, 0.75), "triangle strict interior");
  check_query_parity(acute, acute[0], "triangle sphere equality");
  check_query_parity(
      acute,
      box(-0.1, 0.7, 0.0, 0.1, 0.8, 0.0),
      "triangle interval-query strict interior");

  const std::array<ExactDyadicAabb3, 3> obtuse_product{
      point_box(0.0, 0.0),
      point_box(2.0, 0.0),
      box(0.25, 0.0, 0.0, 0.5, 0.1, 0.0)};
  check_support_parity(
      obtuse_product, "non-singleton obtuse support product");

  const std::array<ExactDyadicAabb3, 3> power_product{
      box(-0.5, 2.0, 0.0, 0.5, 2.0, 0.0),
      point_box(-1.0, 0.0),
      point_box(1.0, 0.0)};
  check_query_parity(
      power_product,
      point_box(0.0, 33.0 / 16.0),
      "non-singleton power interval regression");

  const std::array<ExactDyadicAabb3, 4> regular{
      point_box(1.0, 1.0, 1.0),
      point_box(1.0, -1.0, -1.0),
      point_box(-1.0, 1.0, -1.0),
      point_box(-1.0, -1.0, 1.0)};
  const std::array<ExactDyadicAabb3, 4> exterior{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, 1.0)};
  check_support_parity(regular, "regular tetrahedron non-prune");
  check_support_parity(exterior, "orthogonal tetrahedron prune");
  check_query_parity(
      regular, point_box(0.0, 0.0, 0.0),
      "regular tetrahedron strict interior");
  check_query_parity(
      regular, regular[2], "tetrahedron sphere equality");
}

void test_wide_exponent_fallback() {
  const double tiny = std::ldexp(1.0, -200);
  const std::array<ExactDyadicAabb3, 3> support{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, tiny)};
  check_support_parity(support, "wide-exponent support");
  check_query_parity(
      support,
      point_box(0.5, tiny / 2.0),
      "wide-exponent query");
}

void test_small_deterministic_dyadic_grid() {
  constexpr std::array<int, 5> scale_exponents{-20, -5, 0, 7, 20};
  for (const int exponent : scale_exponents) {
    const double scale = std::ldexp(1.0, exponent);
    const std::array<ExactDyadicAabb3, 3> triangle{
        point_box(-scale, 0.0),
        point_box(scale, 0.0),
        point_box(0.0, 2.0 * scale)};
    check_support_parity(
        triangle,
        "deterministic scaled triangle " + std::to_string(exponent));
    check_query_parity(
        triangle,
        point_box(0.0, 0.75 * scale),
        "deterministic scaled triangle query " +
            std::to_string(exponent));

    const std::array<ExactDyadicAabb3, 4> tetrahedron{
        point_box(scale, scale, scale),
        point_box(scale, -scale, -scale),
        point_box(-scale, scale, -scale),
        point_box(-scale, -scale, scale)};
    check_support_parity(
        tetrahedron,
        "deterministic scaled tetrahedron " +
            std::to_string(exponent));
    check_query_parity(
        tetrahedron,
        point_box(0.0, 0.0, 0.0),
        "deterministic scaled tetrahedron query " +
            std::to_string(exponent));
  }
}

}  // namespace

int main() {
  test_checked_uint1024_arithmetic();
  test_named_triangle_and_tetrahedron_fixtures();
  test_wide_exponent_fallback();
  test_small_deterministic_dyadic_grid();
  if (failures != 0) {
    std::cerr << failures
              << " exact higher-support fixed-limb test(s) failed\n";
    return 1;
  }
  return 0;
}
