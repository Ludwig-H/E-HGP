#include "phase15_exact_higher_support_product_fixed.cuh"
#include "phase15_exact_higher_support_product_fixed256.cuh"
#include "phase15_exact_higher_support_product_fixed512.cuh"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>

namespace {

namespace fixed =
    morsehgp3d::gpu::detail::exact_higher_support_product_fixed;
namespace fixed256 =
    morsehgp3d::gpu::detail::exact_higher_support_product_fixed256;
namespace fixed512 =
    morsehgp3d::gpu::detail::exact_higher_support_product_fixed512;
using morsehgp3d::hierarchy::
    ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::
    ExactHigherSupportTerminalGeometryDecision;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_terminal_geometry_decision;
using morsehgp3d::spatial::ExactDyadicAabb3;

static_assert(fixed::limb_count == 16U);
static_assert(fixed::fixed_bit_count == 1024U);
static_assert(fixed::aligned_coordinate_bit_limit == 124U);
static_assert(fixed::proven_maximum_expression_bit_count == 1013U);
static_assert(fixed256::limb_count == 4U);
static_assert(fixed256::fixed_bit_count == 256U);
static_assert(fixed256::aligned_coordinate_bit_limit(3U, false) == 61U);
static_assert(fixed256::aligned_coordinate_bit_limit(3U, true) == 40U);
static_assert(fixed256::aligned_coordinate_bit_limit(4U, false) == 39U);
static_assert(fixed256::aligned_coordinate_bit_limit(4U, true) == 29U);
static_assert(fixed256::proven_maximum_expression_bit_count == 255U);
static_assert(fixed512::limb_count == 8U);
static_assert(fixed512::fixed_bit_count == 512U);
static_assert(fixed512::aligned_coordinate_bit_limit == 61U);
static_assert(fixed512::proven_maximum_expression_bit_count == 509U);

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

template <typename Decision>
[[nodiscard]]
std::optional<ExactHigherSupportTerminalGeometryDecision>
public_terminal_decision(Decision decision) {
  if (decision == Decision::requires_cpu_rational_fallback) {
    return std::nullopt;
  }
  return static_cast<ExactHigherSupportTerminalGeometryDecision>(
      static_cast<std::uint8_t>(decision));
}

template <std::size_t Size>
void check_terminal_geometry_parity(
    const std::array<ExactDyadicAabb3, Size>& boxes,
    const std::string& label) {
  const auto native = fixed_boxes(boxes);
  ExactHigherSupportProductAabbDecisionBackend backend{};
  const ExactHigherSupportTerminalGeometryDecision expected =
      exact_higher_support_terminal_geometry_decision(boxes, &backend);
  const auto wide = public_terminal_decision(
      fixed::terminal_support_geometry(native.data(), Size));
  if (backend == ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational) {
    check(!wide.has_value(), label + " requests rational fallback in int1024");
  } else {
    check(wide == expected, label + " matches the CPU category in int1024");
  }

  std::uint64_t coordinate_width = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native.data(), Size, nullptr, coordinate_width),
      label + " has a valid categorical width preflight");
  const auto narrow256 = public_terminal_decision(
      fixed256::terminal_support_geometry(native.data(), Size));
  if (fixed256::expression_fits(Size, false, coordinate_width)) {
    check(
        narrow256 == expected,
        label + " matches the CPU category in int256");
  } else {
    check(
        !narrow256.has_value(),
        label + " fails closed outside the int256 category envelope");
  }
  const auto narrow512 = public_terminal_decision(
      fixed512::terminal_support_geometry(native.data(), Size));
  if (coordinate_width <= fixed512::aligned_coordinate_bit_limit) {
    check(
        narrow512 == expected,
        label + " matches the CPU category in int512");
  } else {
    check(
        !narrow512.has_value(),
        label + " fails closed outside the int512 category envelope");
  }
}

template <std::size_t Size>
void check_terminal_geometry_fixed_width_permutations(
    const std::array<ExactDyadicAabb3, Size>& boxes,
    ExactHigherSupportTerminalGeometryDecision expected,
    const std::string& label) {
  std::array<std::size_t, Size> permutation{};
  for (std::size_t index = 0U; index < Size; ++index) {
    permutation[index] = index;
  }
  std::size_t permutation_index = 0U;
  do {
    std::array<ExactDyadicAabb3, Size> permuted{};
    for (std::size_t index = 0U; index < Size; ++index) {
      permuted[index] = boxes[permutation[index]];
    }
    const std::string permutation_label =
        label + " permutation " + std::to_string(permutation_index);
    check(
        exact_higher_support_terminal_geometry_decision(permuted) ==
            expected,
        permutation_label + " preserves the CPU category");
    check_terminal_geometry_parity(
        permuted, permutation_label + " preserves every fixed-width route");
    ++permutation_index;
  } while (std::next_permutation(
      permutation.begin(), permutation.end()));
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
  std::uint64_t coordinate_width = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native.data(), Size, nullptr, coordinate_width),
      label + " has a valid integer-only width preflight");
  const fixed256::Decision narrow256 =
      fixed256::no_well_centered_support(native.data(), Size);
  if (fixed256::expression_fits(Size, false, coordinate_width)) {
    check(
        narrow256 != fixed256::Decision::requires_cpu_rational_fallback &&
            (narrow256 == fixed256::Decision::certified) == expected,
        label + " matches the CPU decision through int256");
  } else {
    check(
        narrow256 == fixed256::Decision::requires_cpu_rational_fallback,
        label + " fails closed outside its arity-specific int256 envelope");
  }
  const fixed512::Decision narrow =
      fixed512::no_well_centered_support(native.data(), Size);
  if (coordinate_width <= fixed512::aligned_coordinate_bit_limit) {
    check(
        narrow != fixed512::Decision::requires_cpu_rational_fallback &&
            (narrow == fixed512::Decision::certified) == expected,
        label + " matches the CPU decision through int512");
  } else {
    check(
        narrow == fixed512::Decision::requires_cpu_rational_fallback,
        label + " fails closed outside the int512 width envelope");
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
  std::uint64_t coordinate_width = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native.data(), Size, &native_query, coordinate_width),
      label + " query has a valid integer-only width preflight");
  const fixed256::Decision narrow256 =
      fixed256::query_strictly_inside_every_independent_sphere(
          native.data(), Size, native_query);
  if (fixed256::expression_fits(Size, true, coordinate_width)) {
    check(
        narrow256 != fixed256::Decision::requires_cpu_rational_fallback &&
            (narrow256 == fixed256::Decision::certified) == expected,
        label + " query matches the CPU decision through int256");
  } else {
    check(
        narrow256 == fixed256::Decision::requires_cpu_rational_fallback,
        label +
            " query fails closed outside its arity-specific int256 envelope");
  }
  const fixed512::Decision narrow =
      fixed512::query_strictly_inside_every_independent_sphere(
          native.data(), Size, native_query);
  if (coordinate_width <= fixed512::aligned_coordinate_bit_limit) {
    check(
        narrow != fixed512::Decision::requires_cpu_rational_fallback &&
            (narrow == fixed512::Decision::certified) == expected,
        label + " query matches the CPU decision through int512");
  } else {
    check(
        narrow == fixed512::Decision::requires_cpu_rational_fallback,
        label + " query fails closed outside the int512 width envelope");
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

[[nodiscard]] boost::multiprecision::cpp_int cpp_value(
    const fixed256::UInt256& value) {
  boost::multiprecision::cpp_int result = 0;
  for (std::size_t index = fixed256::limb_count; index != 0U; --index) {
    result <<= 64U;
    result += value.limb[index - 1U];
  }
  return result;
}

[[nodiscard]] boost::multiprecision::cpp_int cpp_value(
    const fixed512::UInt512& value) {
  boost::multiprecision::cpp_int result = 0;
  for (std::size_t index = fixed512::limb_count; index != 0U; --index) {
    result <<= 64U;
    result += value.limb[index - 1U];
  }
  return result;
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

void test_checked_uint512_arithmetic() {
  fixed512::UInt512 low_256_ones{};
  for (std::size_t index = 0U; index < 4U; ++index) {
    low_256_ones.limb[index] = UINT64_MAX;
  }
  fixed512::UInt512 square{};
  check(
      fixed512::multiply(low_256_ones, low_256_ones, square),
      "the 8-limb product accepts (2^256-1)^2");
  const boost::multiprecision::cpp_int expected =
      (boost::multiprecision::cpp_int{1} << 256U) - 1;
  check(
      cpp_value(square) == expected * expected &&
          fixed512::bit_width(square) == 512U,
      "the int512 carry chain equals the arbitrary-precision product");

  fixed512::UInt512 all_ones{};
  for (std::uint64_t& limb : all_ones.limb) {
    limb = UINT64_MAX;
  }
  fixed512::UInt512 one{};
  one.limb[0] = 1U;
  fixed512::UInt512 sum{};
  check(
      !fixed512::add(all_ones, one, sum),
      "an int512 addition carry-out fails closed");
}

void test_checked_uint256_arithmetic() {
  fixed256::UInt256 low_128_ones{};
  for (std::size_t index = 0U; index < 2U; ++index) {
    low_128_ones.limb[index] = UINT64_MAX;
  }
  fixed256::UInt256 square{};
  check(
      fixed256::multiply(low_128_ones, low_128_ones, square),
      "the 4-limb product accepts (2^128-1)^2");
  const boost::multiprecision::cpp_int expected =
      (boost::multiprecision::cpp_int{1} << 128U) - 1;
  check(
      cpp_value(square) == expected * expected &&
          fixed256::bit_width(square) == 256U,
      "the int256 carry chain equals the arbitrary-precision product");

  fixed256::UInt256 all_ones{};
  for (std::uint64_t& limb : all_ones.limb) {
    limb = UINT64_MAX;
  }
  fixed256::UInt256 one{};
  one.limb[0] = 1U;
  fixed256::UInt256 sum{};
  check(
      !fixed256::add(all_ones, one, sum),
      "an int256 addition carry-out fails closed");
}

void test_int512_width_boundary() {
  const double bit_minus_60 = std::ldexp(1.0, -60);
  const double bit_minus_61 = std::ldexp(1.0, -61);
  const std::array<ExactDyadicAabb3, 3> width_61{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, bit_minus_60)};
  const std::array<ExactDyadicAabb3, 3> width_62{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, bit_minus_61)};
  const auto native_61 = fixed_boxes(width_61);
  const auto native_62 = fixed_boxes(width_62);
  std::uint64_t observed_width_61 = 0U;
  std::uint64_t observed_width_62 = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native_61.data(), 3U, nullptr, observed_width_61) &&
          observed_width_61 == 61U,
      "the selector certifies the int512 W=61 boundary");
  check(
      fixed::aligned_product_coordinate_bit_width(
          native_62.data(), 3U, nullptr, observed_width_62) &&
          observed_width_62 == 62U,
      "the selector distinguishes the first width outside int512");
  check(
      fixed512::no_well_centered_support(native_61.data(), 3U) !=
          fixed512::Decision::requires_cpu_rational_fallback,
      "the W=61 support stays in the proved 509-bit envelope");
  check(
      fixed512::no_well_centered_support(native_62.data(), 3U) ==
          fixed512::Decision::requires_cpu_rational_fallback,
      "the W=62 support fails closed toward int1024");

  const std::array<ExactDyadicAabb3, 4> tetrahedron_61{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, bit_minus_60)};
  const ExactDyadicAabb3 tetrahedron_query =
      point_box(0.25, 0.25, 0.0);
  const auto native_tetrahedron = fixed_boxes(tetrahedron_61);
  const fixed::Binary64Aabb3 native_query = fixed_box(tetrahedron_query);
  std::uint64_t tetrahedron_width = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native_tetrahedron.data(),
          4U,
          &native_query,
          tetrahedron_width) &&
          tetrahedron_width == 61U,
      "the tetrahedron query reaches the global int512 W=61 bound");
  check_query_parity(
      tetrahedron_61,
      tetrahedron_query,
      "W=61 tetrahedron query boundary");

  const std::array<ExactDyadicAabb3, 4> terminal_int512_boundary{
      point_box(-1.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, 40))};
  const std::array<ExactDyadicAabb3, 4> terminal_int1024_boundary{
      point_box(-1.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, 61))};
  const auto native_terminal_int512 =
      fixed_boxes(terminal_int512_boundary);
  const auto native_terminal_int1024 =
      fixed_boxes(terminal_int1024_boundary);
  std::uint64_t terminal_int512_width = 0U;
  std::uint64_t terminal_int1024_width = 0U;
  check(
      fixed::aligned_product_coordinate_bit_width(
          native_terminal_int512.data(),
          4U,
          nullptr,
          terminal_int512_width) &&
          terminal_int512_width == 41U &&
          !fixed256::expression_fits(4U, false, terminal_int512_width) &&
          fixed512::terminal_support_geometry(
              native_terminal_int512.data(), 4U) ==
              fixed512::TerminalGeometryDecision::boundary_reduced &&
          exact_higher_support_terminal_geometry_decision(
              terminal_int512_boundary) ==
              ExactHigherSupportTerminalGeometryDecision::boundary_reduced,
      "the schema-6 boundary tetrahedron selects native int512 categorically");
  check(
      fixed::aligned_product_coordinate_bit_width(
          native_terminal_int1024.data(),
          4U,
          nullptr,
          terminal_int1024_width) &&
          terminal_int1024_width == 62U &&
          fixed512::terminal_support_geometry(
              native_terminal_int1024.data(), 4U) ==
              fixed512::TerminalGeometryDecision::
                  requires_cpu_rational_fallback &&
          fixed::terminal_support_geometry(
              native_terminal_int1024.data(), 4U) ==
              fixed::TerminalGeometryDecision::boundary_reduced &&
          exact_higher_support_terminal_geometry_decision(
              terminal_int1024_boundary) ==
              ExactHigherSupportTerminalGeometryDecision::boundary_reduced,
      "the schema-6 boundary tetrahedron selects native int1024 categorically");
}

void test_int256_arity_boundaries() {
  const std::array<ExactDyadicAabb3, 3> support3_61{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, std::ldexp(1.0, -60))};
  const std::array<ExactDyadicAabb3, 3> support3_62{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, std::ldexp(1.0, -61))};
  const auto native_support3_61 = fixed_boxes(support3_61);
  const auto native_support3_62 = fixed_boxes(support3_62);
  check(
      fixed256::no_well_centered_support(
          native_support3_61.data(), 3U) !=
          fixed256::Decision::requires_cpu_rational_fallback &&
          fixed256::no_well_centered_support(
              native_support3_62.data(), 3U) ==
              fixed256::Decision::requires_cpu_rational_fallback,
      "support-3 int256 accepts W=61 and rejects W=62");

  const std::array<ExactDyadicAabb3, 3> query3_40{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, std::ldexp(1.0, -39))};
  const std::array<ExactDyadicAabb3, 3> query3_41{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, std::ldexp(1.0, -40))};
  const auto native_query3_40 = fixed_boxes(query3_40);
  const auto native_query3_41 = fixed_boxes(query3_41);
  const fixed::Binary64Aabb3 triangle_query =
      fixed_box(point_box(0.25, 0.0));
  check(
      fixed256::query_strictly_inside_every_independent_sphere(
          native_query3_40.data(), 3U, triangle_query) !=
          fixed256::Decision::requires_cpu_rational_fallback &&
          fixed256::query_strictly_inside_every_independent_sphere(
              native_query3_41.data(), 3U, triangle_query) ==
              fixed256::Decision::requires_cpu_rational_fallback,
      "query-3 int256 accepts W=40 and rejects W=41");

  const std::array<ExactDyadicAabb3, 4> support4_39{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, -38))};
  const std::array<ExactDyadicAabb3, 4> support4_40{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, -39))};
  const auto native_support4_39 = fixed_boxes(support4_39);
  const auto native_support4_40 = fixed_boxes(support4_40);
  check(
      fixed256::no_well_centered_support(
          native_support4_39.data(), 4U) !=
          fixed256::Decision::requires_cpu_rational_fallback &&
          fixed256::no_well_centered_support(
              native_support4_40.data(), 4U) ==
              fixed256::Decision::requires_cpu_rational_fallback,
      "support-4 int256 accepts W=39 and rejects W=40");

  const std::array<ExactDyadicAabb3, 4> query4_29{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, -28))};
  const std::array<ExactDyadicAabb3, 4> query4_30{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, std::ldexp(1.0, -29))};
  const auto native_query4_29 = fixed_boxes(query4_29);
  const auto native_query4_30 = fixed_boxes(query4_30);
  const fixed::Binary64Aabb3 tetrahedron_query =
      fixed_box(point_box(0.25, 0.25, 0.0));
  check(
      fixed256::query_strictly_inside_every_independent_sphere(
          native_query4_29.data(), 4U, tetrahedron_query) !=
          fixed256::Decision::requires_cpu_rational_fallback &&
          fixed256::query_strictly_inside_every_independent_sphere(
              native_query4_30.data(), 4U, tetrahedron_query) ==
              fixed256::Decision::requires_cpu_rational_fallback,
      "query-4 int256 accepts W=29 and rejects W=30");
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

void test_terminal_geometry_fixed_width_fixtures() {
  using Decision = ExactHigherSupportTerminalGeometryDecision;
  const std::array<ExactDyadicAabb3, 3> acute{
      point_box(0.0, 0.0),
      point_box(4.0, 0.0),
      point_box(1.0, 3.0)};
  const std::array<ExactDyadicAabb3, 3> right{
      point_box(0.0, 0.0),
      point_box(2.0, 0.0),
      point_box(0.0, 2.0)};
  const std::array<ExactDyadicAabb3, 3> obtuse{
      point_box(0.0, 0.0),
      point_box(4.0, 0.0),
      point_box(1.0, 1.0)};
  const std::array<ExactDyadicAabb3, 3> collinear{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(2.0, 0.0)};
  check_terminal_geometry_fixed_width_permutations(
      acute, Decision::minimal, "acute triangle category");
  check_terminal_geometry_fixed_width_permutations(
      right, Decision::boundary_reduced, "right triangle category");
  check_terminal_geometry_fixed_width_permutations(
      obtuse, Decision::exterior_circumcenter,
      "obtuse triangle category");
  check_terminal_geometry_fixed_width_permutations(
      collinear, Decision::affinely_dependent,
      "dependent triangle category");
  std::array<ExactDyadicAabb3, 3> signed_zero = right;
  signed_zero[0].lower_binary64_bits[2] = std::bit_cast<std::uint64_t>(-0.0);
  check_terminal_geometry_parity(
      signed_zero, "signed-zero right triangle category");

  const std::array<ExactDyadicAabb3, 4> regular{
      point_box(1.0, 1.0, 1.0),
      point_box(1.0, -1.0, -1.0),
      point_box(-1.0, 1.0, -1.0),
      point_box(-1.0, -1.0, 1.0)};
  const std::array<ExactDyadicAabb3, 4> boundary{
      point_box(1.0, 0.0, 0.0),
      point_box(-1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, 1.0)};
  const std::array<ExactDyadicAabb3, 4> exterior{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(0.0, 1.0, 0.0),
      point_box(0.0, 0.0, 1.0)};
  const std::array<ExactDyadicAabb3, 4> dependent{
      point_box(0.0, 0.0, 0.0),
      point_box(1.0, 0.0, 0.0),
      point_box(1.0, 1.0, 0.0),
      point_box(0.0, 1.0, 0.0)};
  const std::array<ExactDyadicAabb3, 4> negative_and_zero{
      point_box(0.0, 0.0, 0.0),
      point_box(4.0, 0.0, 0.0),
      point_box(1.0, 1.0, 0.0),
      point_box(3.0, -1.0, 2.0)};
  check_terminal_geometry_fixed_width_permutations(
      regular, Decision::minimal, "regular tetrahedron category");
  check_terminal_geometry_fixed_width_permutations(
      boundary, Decision::boundary_reduced,
      "boundary tetrahedron category");
  check_terminal_geometry_fixed_width_permutations(
      exterior, Decision::exterior_circumcenter,
      "exterior tetrahedron category");
  check_terminal_geometry_fixed_width_permutations(
      dependent, Decision::affinely_dependent,
      "dependent tetrahedron category");
  check_terminal_geometry_fixed_width_permutations(
      negative_and_zero,
      Decision::exterior_circumcenter,
      "negative-before-zero tetrahedron category");

  std::array<ExactDyadicAabb3, 3> nonsingleton = acute;
  nonsingleton[2] = box(1.0, 3.0, 0.0, 1.25, 3.0, 0.0);
  const auto native = fixed_boxes(nonsingleton);
  check(
      fixed256::terminal_support_geometry(native.data(), 3U) ==
              fixed256::TerminalGeometryDecision::
                  requires_cpu_rational_fallback &&
          fixed512::terminal_support_geometry(native.data(), 3U) ==
              fixed512::TerminalGeometryDecision::
                  requires_cpu_rational_fallback &&
          fixed::terminal_support_geometry(native.data(), 3U) ==
              fixed::TerminalGeometryDecision::
                  requires_cpu_rational_fallback,
      "all fixed widths reject a non-singleton categorical product");
}

void test_wide_exponent_fallback() {
  const double tiny = std::ldexp(1.0, -200);
  const std::array<ExactDyadicAabb3, 3> support{
      point_box(0.0, 0.0),
      point_box(1.0, 0.0),
      point_box(0.0, tiny)};
  check_support_parity(support, "wide-exponent support");
  check_terminal_geometry_parity(
      support, "wide-exponent terminal category");
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
  test_checked_uint256_arithmetic();
  test_checked_uint512_arithmetic();
  test_int256_arity_boundaries();
  test_int512_width_boundary();
  test_named_triangle_and_tetrahedron_fixtures();
  test_terminal_geometry_fixed_width_fixtures();
  test_wide_exponent_fallback();
  test_small_deterministic_dyadic_grid();
  if (failures != 0) {
    std::cerr << failures
              << " exact higher-support fixed-limb test(s) failed\n";
    return 1;
  }
  return 0;
}
