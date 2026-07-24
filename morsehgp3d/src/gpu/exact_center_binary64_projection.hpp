#pragma once

#include "morsehgp3d/exact/center.hpp"

#include <boost/multiprecision/integer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint64_t nearest_binary64_sign_bit =
    UINT64_C(0x8000000000000000);
inline constexpr std::uint64_t nearest_binary64_hidden_bit =
    UINT64_C(1) << 52U;
inline constexpr std::uint64_t nearest_binary64_significand_limit =
    UINT64_C(1) << 53U;

struct ExactCenterBinary64Projection {
  std::array<std::uint64_t, 3U> coordinate_bits{};
  std::size_t integer_division_count{};
  bool supported{true};
};

[[nodiscard]] inline int floor_binary_exponent(
    const exact::BigInt& positive_numerator,
    const exact::BigInt& positive_denominator,
    std::size_t denominator_most_significant_bit) {
  const std::size_t numerator_most_significant_bit =
      static_cast<std::size_t>(
          boost::multiprecision::msb(positive_numerator));
  int exponent = 0;
  if (numerator_most_significant_bit >=
      denominator_most_significant_bit) {
    const std::size_t difference =
        numerator_most_significant_bit -
        denominator_most_significant_bit;
    if (difference > 1024U) {
      throw std::logic_error(
          "a supported binary64 projection exponent is too large");
    }
    exponent = static_cast<int>(difference);
  } else {
    const std::size_t difference =
        denominator_most_significant_bit -
        numerator_most_significant_bit;
    if (difference > 1022U) {
      throw std::logic_error(
          "a normal binary64 projection exponent is too small");
    }
    exponent = -static_cast<int>(difference);
  }

  const bool estimate_is_too_large =
      exponent >= 0
          ? positive_numerator <
                (positive_denominator
                 << static_cast<unsigned int>(exponent))
          : (positive_numerator
             << static_cast<unsigned int>(-exponent)) <
                positive_denominator;
  return estimate_is_too_large ? exponent - 1 : exponent;
}

[[nodiscard]] inline bool round_magnitude_up(
    const exact::BigInt& remainder,
    const exact::BigInt& divisor,
    bool negative) {
  const exact::BigInt twice_remainder = remainder << 1U;
  return negative ? twice_remainder >= divisor
                  : twice_remainder > divisor;
}

struct Binary64CoordinateProjection {
  std::uint64_t bits{};
  std::size_t integer_division_count{};
  bool supported{true};
};

[[nodiscard]] inline Binary64CoordinateProjection
project_exact_coordinate_to_nearest_binary64(
    const exact::BigInt& numerator,
    const exact::BigInt& positive_denominator,
    std::size_t denominator_most_significant_bit,
    const exact::BigInt& maximum_finite_scaled_numerator) {
  if (positive_denominator <= 0) {
    throw std::logic_error(
        "an exact-center projection denominator must be positive");
  }
  if (numerator == 0) {
    return {};
  }

  const bool negative = numerator < 0;
  const exact::BigInt magnitude =
      negative ? -numerator : numerator;
  if (magnitude > maximum_finite_scaled_numerator) {
    return Binary64CoordinateProjection{0U, 0U, false};
  }

  exact::BigInt quotient;
  exact::BigInt remainder;
  if ((magnitude << 1022U) < positive_denominator) {
    const exact::BigInt scaled_numerator = magnitude << 1074U;
    boost::multiprecision::divide_qr(
        scaled_numerator,
        positive_denominator,
        quotient,
        remainder);
    if (round_magnitude_up(
            remainder, positive_denominator, negative)) {
      ++quotient;
    }
    if (quotient < 0 ||
        quotient > exact::BigInt{nearest_binary64_hidden_bit}) {
      throw std::logic_error(
          "a subnormal binary64 projection produced an invalid significand");
    }
    const std::uint64_t magnitude_bits =
        quotient.convert_to<std::uint64_t>();
    return Binary64CoordinateProjection{
        magnitude_bits == 0U
            ? 0U
            : magnitude_bits |
                  (negative ? nearest_binary64_sign_bit : 0U),
        1U,
        true};
  }

  int exponent = floor_binary_exponent(
      magnitude,
      positive_denominator,
      denominator_most_significant_bit);
  if (exponent < -1022 || exponent > 1023) {
    throw std::logic_error(
        "a normal binary64 projection exponent is outside the finite range");
  }

  exact::BigInt scaled_numerator = magnitude;
  exact::BigInt scaled_denominator = positive_denominator;
  const int significand_shift = 52 - exponent;
  if (significand_shift >= 0) {
    scaled_numerator <<=
        static_cast<unsigned int>(significand_shift);
  } else {
    scaled_denominator <<=
        static_cast<unsigned int>(-significand_shift);
  }
  boost::multiprecision::divide_qr(
      scaled_numerator,
      scaled_denominator,
      quotient,
      remainder);
  if (round_magnitude_up(
          remainder, scaled_denominator, negative)) {
    ++quotient;
  }

  if (quotient ==
      exact::BigInt{nearest_binary64_significand_limit}) {
    quotient >>= 1U;
    ++exponent;
  }
  if (exponent < -1022 || exponent > 1023 ||
      quotient < exact::BigInt{nearest_binary64_hidden_bit} ||
      quotient >=
          exact::BigInt{nearest_binary64_significand_limit}) {
    throw std::logic_error(
        "a normal binary64 projection produced an invalid rounded word");
  }

  const std::uint64_t significand =
      quotient.convert_to<std::uint64_t>();
  const std::uint64_t exponent_bits =
      static_cast<std::uint64_t>(exponent + 1023) << 52U;
  const std::uint64_t fraction_bits =
      significand - nearest_binary64_hidden_bit;
  return Binary64CoordinateProjection{
      exponent_bits | fraction_bits |
          (negative ? nearest_binary64_sign_bit : 0U),
      1U,
      true};
}

[[nodiscard]] inline ExactCenterBinary64Projection
project_exact_center_to_nearest_binary64(
    const exact::ExactCenter3& center) {
  const exact::BigInt& denominator = center.denominator();
  if (denominator <= 0) {
    throw std::logic_error(
        "an exact-center projection denominator must be positive");
  }
  const std::size_t denominator_most_significant_bit =
      static_cast<std::size_t>(
          boost::multiprecision::msb(denominator));
  const exact::BigInt maximum_significand =
      (exact::BigInt{1} << 53U) - 1;
  const exact::BigInt maximum_finite_scaled_numerator =
      (denominator * maximum_significand) << 971U;

  ExactCenterBinary64Projection result;
  for (std::size_t axis = 0U;
       axis < result.coordinate_bits.size();
       ++axis) {
    const Binary64CoordinateProjection coordinate =
        project_exact_coordinate_to_nearest_binary64(
            center.numerator(axis),
            denominator,
            denominator_most_significant_bit,
            maximum_finite_scaled_numerator);
    result.coordinate_bits[axis] = coordinate.bits;
    result.integer_division_count +=
        coordinate.integer_division_count;
    result.supported = result.supported && coordinate.supported;
  }
  return result;
}

}  // namespace morsehgp3d::gpu::detail
