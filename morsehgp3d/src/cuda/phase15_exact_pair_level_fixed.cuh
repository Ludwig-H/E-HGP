#pragma once

#include "phase15_exact_diametral_phi_fixed.cuh"

#include <cstddef>
#include <cstdint>

#if defined(__CUDACC__)
#define MORSEHGP3D_EXACT_PAIR_LEVEL_HD __host__ __device__
#else
#define MORSEHGP3D_EXACT_PAIR_LEVEL_HD
#endif

namespace morsehgp3d::gpu::detail::exact_pair_level_fixed {

namespace fixed = exact_phi_fixed;

inline constexpr std::size_t axis_count = 3U;
inline constexpr std::size_t maximum_limb_count = 4U;
inline constexpr std::size_t maximum_fixed_bit_count = 256U;
inline constexpr int minimum_canonical_exponent = -2150;
inline constexpr int maximum_canonical_exponent = 2047;
inline constexpr int minimum_top_bit_exponent = -2150;
inline constexpr int maximum_top_bit_exponent = 2049;
inline constexpr unsigned int radix_top_exponent_bit_count = 13U;
inline constexpr unsigned int radix_significand_prefix_bit_count =
    64U - radix_top_exponent_bit_count;

static_assert(radix_significand_prefix_bit_count == 51U);
static_assert(
    maximum_top_bit_exponent - minimum_top_bit_exponent + 1 <=
    (1 << radix_top_exponent_bit_count));

enum class ResultCode : std::uint8_t {
  exact = 0U,
  zero_duplicate = 1U,
  requires_wide = 2U,
  invalid_input = 3U,
};

// Canonical positive dyadic level N*2^exponent.  For an exact result N is
// nonzero and odd, unused high limbs are zero, and limb_count is the minimal
// extent in little-endian 64-bit limbs.
struct CanonicalLevel256 {
  fixed::UInt256 numerator{};
  std::int16_t exponent{};
  std::uint8_t limb_count{};
  std::uint8_t reserved_zero{};
};

struct Result {
  CanonicalLevel256 level{};
  ResultCode code{ResultCode::invalid_input};
};

enum class ComparisonCode : std::uint8_t {
  less = 0U,
  equal = 1U,
  greater = 2U,
  not_comparable = 3U,
};

struct RadixPrefix {
  std::uint64_t key{};
  bool available{false};
};

// Equal prefixes are deliberately not equality witnesses.  They only define
// a run that an exact comparator or a later radix refinement must resolve.
enum class RadixPrefixComparison : std::uint8_t {
  less = 0U,
  greater = 1U,
  collision_requires_exact_refinement = 2U,
  not_comparable = 3U,
};

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline std::uint8_t limb_count(
    const fixed::UInt256& value) noexcept {
  for (std::size_t index = maximum_limb_count; index != 0U; --index) {
    if (value.limb[index - 1U] != 0U) {
      return static_cast<std::uint8_t>(index);
    }
  }
  return 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline bool
canonical_level_well_formed(const CanonicalLevel256& level) noexcept {
  if (level.reserved_zero != 0U || level.limb_count == 0U ||
      static_cast<std::size_t>(level.limb_count) > maximum_limb_count ||
      level.exponent < minimum_canonical_exponent ||
      level.exponent > maximum_canonical_exponent ||
      (level.numerator.limb[0] & UINT64_C(1)) == 0U ||
      level.numerator.limb[
          static_cast<std::size_t>(level.limb_count) - 1U] == 0U) {
    return false;
  }
  for (std::size_t index = static_cast<std::size_t>(level.limb_count);
       index < maximum_limb_count;
       ++index) {
    if (level.numerator.limb[index] != 0U) {
      return false;
    }
  }
  const unsigned int width = fixed::bit_width(level.numerator);
  const int top = static_cast<int>(level.exponent) +
      static_cast<int>(width) - 1;
  return top >= minimum_top_bit_exponent &&
      top <= maximum_top_bit_exponent;
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline bool result_is_exact(
    const Result& result) noexcept {
  return result.code == ResultCode::exact &&
      canonical_level_well_formed(result.level);
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline Result exact_pair_level(
    const std::uint64_t first_support_bits[axis_count],
    const std::uint64_t second_support_bits[axis_count]) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (!fixed::is_finite_binary64_word(first_support_bits[axis]) ||
        !fixed::is_finite_binary64_word(second_support_bits[axis])) {
      return Result{{}, ResultCode::invalid_input};
    }
  }

  fixed::ExactProduct squared_differences[axis_count]{};
  bool exponent_initialized = false;
  int common_exponent = 0;
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    fixed::ExactDifference difference{};
    if (!fixed::exact_difference(
            first_support_bits[axis],
            second_support_bits[axis],
            difference)) {
      return Result{{}, ResultCode::requires_wide};
    }
    if (!fixed::exact_product(
            difference, difference, squared_differences[axis])) {
      return Result{{}, ResultCode::requires_wide};
    }
    const fixed::ExactProduct& square = squared_differences[axis];
    if (square.value.negative) {
      return Result{{}, ResultCode::invalid_input};
    }
    if (!fixed::is_zero(square.value.magnitude) &&
        (!exponent_initialized || square.exponent < common_exponent)) {
      common_exponent = square.exponent;
      exponent_initialized = true;
    }
  }

  if (!exponent_initialized) {
    return Result{{}, ResultCode::zero_duplicate};
  }

  fixed::UInt256 sum{};
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    const fixed::ExactProduct& square = squared_differences[axis];
    if (fixed::is_zero(square.value.magnitude)) {
      continue;
    }
    const int shift = square.exponent - common_exponent;
    if (shift < 0 || shift > static_cast<int>(maximum_fixed_bit_count)) {
      return Result{{}, ResultCode::requires_wide};
    }
    fixed::UInt256 aligned{};
    if (!fixed::shift_left(
            square.value.magnitude,
            static_cast<unsigned int>(shift),
            aligned)) {
      return Result{{}, ResultCode::requires_wide};
    }
    fixed::UInt256 next{};
    if (!fixed::add(sum, aligned, next)) {
      return Result{{}, ResultCode::requires_wide};
    }
    sum = next;
  }

  if (fixed::is_zero(sum)) {
    // A sum of nonnegative exact squares cannot cancel.  Refuse to publish an
    // impossible exact payload if an internal arithmetic invariant changes.
    return Result{{}, ResultCode::invalid_input};
  }
  const unsigned int trailing = fixed::trailing_zero_count(sum);
  const fixed::UInt256 numerator = fixed::shift_right(sum, trailing);
  const int exponent = common_exponent + static_cast<int>(trailing) - 2;
  if (exponent < minimum_canonical_exponent ||
      exponent > maximum_canonical_exponent) {
    return Result{{}, ResultCode::requires_wide};
  }
  const std::uint8_t count = limb_count(numerator);
  const CanonicalLevel256 level{
      numerator,
      static_cast<std::int16_t>(exponent),
      count,
      0U};
  if (!canonical_level_well_formed(level)) {
    return Result{{}, ResultCode::invalid_input};
  }
  return Result{level, ResultCode::exact};
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline ComparisonCode compare(
    const Result& left,
    const Result& right) noexcept {
  if (!result_is_exact(left) || !result_is_exact(right)) {
    return ComparisonCode::not_comparable;
  }
  const unsigned int left_width = fixed::bit_width(left.level.numerator);
  const unsigned int right_width = fixed::bit_width(right.level.numerator);
  const int left_top =
      static_cast<int>(left.level.exponent) +
      static_cast<int>(left_width) - 1;
  const int right_top =
      static_cast<int>(right.level.exponent) +
      static_cast<int>(right_width) - 1;
  if (left_top < right_top) {
    return ComparisonCode::less;
  }
  if (left_top > right_top) {
    return ComparisonCode::greater;
  }

  const unsigned int common_width =
      left_width > right_width ? left_width : right_width;
  fixed::UInt256 aligned_left{};
  fixed::UInt256 aligned_right{};
  if (!fixed::shift_left(
          left.level.numerator,
          common_width - left_width,
          aligned_left) ||
      !fixed::shift_left(
          right.level.numerator,
          common_width - right_width,
          aligned_right)) {
    return ComparisonCode::not_comparable;
  }
  const int ordering = fixed::compare(aligned_left, aligned_right);
  return ordering < 0 ? ComparisonCode::less
                      : ordering > 0 ? ComparisonCode::greater
                                     : ComparisonCode::equal;
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline RadixPrefix radix_prefix(
    const Result& result) noexcept {
  if (!result_is_exact(result)) {
    return {};
  }
  const unsigned int width = fixed::bit_width(result.level.numerator);
  const int top = static_cast<int>(result.level.exponent) +
      static_cast<int>(width) - 1;
  if (top < minimum_top_bit_exponent ||
      top > maximum_top_bit_exponent) {
    return {};
  }

  fixed::UInt256 prefix_value{};
  if (width >= radix_significand_prefix_bit_count) {
    prefix_value = fixed::shift_right(
        result.level.numerator,
        width - radix_significand_prefix_bit_count);
  } else if (!fixed::shift_left(
                 result.level.numerator,
                 radix_significand_prefix_bit_count - width,
                 prefix_value)) {
    return {};
  }
  constexpr std::uint64_t prefix_mask =
      (UINT64_C(1) << radix_significand_prefix_bit_count) - UINT64_C(1);
  const std::uint64_t biased_top = static_cast<std::uint64_t>(
      top - minimum_top_bit_exponent);
  return RadixPrefix{
      (biased_top << radix_significand_prefix_bit_count) |
          (prefix_value.limb[0] & prefix_mask),
      true};
}

[[nodiscard]] MORSEHGP3D_EXACT_PAIR_LEVEL_HD inline
RadixPrefixComparison compare_radix_prefixes(
    const RadixPrefix& left,
    const RadixPrefix& right) noexcept {
  if (!left.available || !right.available) {
    return RadixPrefixComparison::not_comparable;
  }
  if (left.key < right.key) {
    return RadixPrefixComparison::less;
  }
  if (left.key > right.key) {
    return RadixPrefixComparison::greater;
  }
  return RadixPrefixComparison::collision_requires_exact_refinement;
}

}  // namespace morsehgp3d::gpu::detail::exact_pair_level_fixed

#undef MORSEHGP3D_EXACT_PAIR_LEVEL_HD
