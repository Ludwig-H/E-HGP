#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__CUDACC__)
#define MORSEHGP3D_EXACT_PHI_HD __host__ __device__
#else
#define MORSEHGP3D_EXACT_PHI_HD
#endif

namespace morsehgp3d::gpu::detail::exact_phi_fixed {

inline constexpr std::size_t axis_count = 3U;
inline constexpr std::size_t difference_limb_bit_count = 128U;
inline constexpr std::size_t accumulator_limb_bit_count = 256U;
// If all nine coordinates align globally in at most 126 magnitude bits, every
// difference has magnitude strictly below 2^127, every product has magnitude
// strictly below 2^254, and the absolute three-term sum is strictly below
// 3*2^254 < 2^256. The actual
// implementation is less restrictive: it aligns each difference separately
// and accepts every case whose checked 128/256-bit operations fit.
inline constexpr std::size_t guaranteed_global_coordinate_bit_count = 126U;

enum class ResultCode : std::uint8_t {
  negative = 0U,
  zero = 1U,
  positive = 2U,
  requires_cpu_fallback = 3U,
  invalid_input = 4U,
};

struct UInt128 {
  std::uint64_t limb[2]{};
};

struct UInt256 {
  std::uint64_t limb[4]{};
};

struct Signed128 {
  UInt128 magnitude{};
  bool negative{};
};

struct Signed256 {
  UInt256 magnitude{};
  bool negative{};
};

struct DyadicWord {
  std::uint64_t magnitude{};
  int exponent{};
  bool negative{};
};

struct ExactDifference {
  Signed128 value{};
  int exponent{};
};

struct ExactProduct {
  Signed256 value{};
  int exponent{};
};

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool is_finite_binary64_word(
    std::uint64_t bits) noexcept {
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff);
  return ((bits >> 52U) & exponent_mask) != exponent_mask;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool is_zero(
    const UInt128& value) noexcept {
  return value.limb[0] == 0U && value.limb[1] == 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool is_zero(
    const UInt256& value) noexcept {
  return value.limb[0] == 0U && value.limb[1] == 0U &&
         value.limb[2] == 0U && value.limb[3] == 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline unsigned int bit_width_u64(
    std::uint64_t value) noexcept {
  unsigned int width = 0U;
  while (value != 0U) {
    ++width;
    value >>= 1U;
  }
  return width;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline unsigned int trailing_zero_u64(
    std::uint64_t value) noexcept {
  if (value == 0U) {
    return 64U;
  }
  unsigned int count = 0U;
  while ((value & UINT64_C(1)) == 0U) {
    ++count;
    value >>= 1U;
  }
  return count;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline unsigned int bit_width(
    const UInt256& value) noexcept {
  for (std::size_t index = 4U; index != 0U; --index) {
    if (value.limb[index - 1U] != 0U) {
      return static_cast<unsigned int>((index - 1U) * 64U) +
             bit_width_u64(value.limb[index - 1U]);
    }
  }
  return 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline unsigned int trailing_zero_count(
    const UInt128& value) noexcept {
  if (value.limb[0] != 0U) {
    return trailing_zero_u64(value.limb[0]);
  }
  return value.limb[1] == 0U
             ? 128U
             : 64U + trailing_zero_u64(value.limb[1]);
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline unsigned int trailing_zero_count(
    const UInt256& value) noexcept {
  for (std::size_t index = 0U; index < 4U; ++index) {
    if (value.limb[index] != 0U) {
      return static_cast<unsigned int>(index * 64U) +
             trailing_zero_u64(value.limb[index]);
    }
  }
  return 256U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline int compare(
    const UInt128& left,
    const UInt128& right) noexcept {
  for (std::size_t index = 2U; index != 0U; --index) {
    if (left.limb[index - 1U] < right.limb[index - 1U]) {
      return -1;
    }
    if (left.limb[index - 1U] > right.limb[index - 1U]) {
      return 1;
    }
  }
  return 0;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline int compare(
    const UInt256& left,
    const UInt256& right) noexcept {
  for (std::size_t index = 4U; index != 0U; --index) {
    if (left.limb[index - 1U] < right.limb[index - 1U]) {
      return -1;
    }
    if (left.limb[index - 1U] > right.limb[index - 1U]) {
      return 1;
    }
  }
  return 0;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool add(
    const UInt128& left,
    const UInt128& right,
    UInt128& output) noexcept {
  std::uint64_t carry = 0U;
  for (std::size_t index = 0U; index < 2U; ++index) {
    const std::uint64_t first = left.limb[index] + right.limb[index];
    const std::uint64_t carry_first = first < left.limb[index] ? 1U : 0U;
    const std::uint64_t second = first + carry;
    const std::uint64_t carry_second = second < first ? 1U : 0U;
    output.limb[index] = second;
    carry = carry_first | carry_second;
  }
  return carry == 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool add(
    const UInt256& left,
    const UInt256& right,
    UInt256& output) noexcept {
  std::uint64_t carry = 0U;
  for (std::size_t index = 0U; index < 4U; ++index) {
    const std::uint64_t first = left.limb[index] + right.limb[index];
    const std::uint64_t carry_first = first < left.limb[index] ? 1U : 0U;
    const std::uint64_t second = first + carry;
    const std::uint64_t carry_second = second < first ? 1U : 0U;
    output.limb[index] = second;
    carry = carry_first | carry_second;
  }
  return carry == 0U;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline UInt128 subtract_magnitudes(
    const UInt128& greater,
    const UInt128& lesser) noexcept {
  UInt128 output{};
  std::uint64_t borrow = 0U;
  for (std::size_t index = 0U; index < 2U; ++index) {
    const std::uint64_t first = greater.limb[index] - lesser.limb[index];
    const std::uint64_t borrow_first =
        greater.limb[index] < lesser.limb[index] ? 1U : 0U;
    const std::uint64_t second = first - borrow;
    const std::uint64_t borrow_second = first < borrow ? 1U : 0U;
    output.limb[index] = second;
    borrow = borrow_first | borrow_second;
  }
  return output;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline UInt256 subtract_magnitudes(
    const UInt256& greater,
    const UInt256& lesser) noexcept {
  UInt256 output{};
  std::uint64_t borrow = 0U;
  for (std::size_t index = 0U; index < 4U; ++index) {
    const std::uint64_t first = greater.limb[index] - lesser.limb[index];
    const std::uint64_t borrow_first =
        greater.limb[index] < lesser.limb[index] ? 1U : 0U;
    const std::uint64_t second = first - borrow;
    const std::uint64_t borrow_second = first < borrow ? 1U : 0U;
    output.limb[index] = second;
    borrow = borrow_first | borrow_second;
  }
  return output;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool add_signed(
    const Signed128& left,
    const Signed128& right,
    Signed128& output) noexcept {
  if (left.negative == right.negative) {
    output.negative = left.negative;
    if (!add(left.magnitude, right.magnitude, output.magnitude)) {
      return false;
    }
  } else {
    const int ordering = compare(left.magnitude, right.magnitude);
    if (ordering >= 0) {
      output.magnitude = subtract_magnitudes(left.magnitude, right.magnitude);
      output.negative = left.negative;
    } else {
      output.magnitude = subtract_magnitudes(right.magnitude, left.magnitude);
      output.negative = right.negative;
    }
  }
  if (is_zero(output.magnitude)) {
    output.negative = false;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool add_signed(
    const Signed256& left,
    const Signed256& right,
    Signed256& output) noexcept {
  if (left.negative == right.negative) {
    output.negative = left.negative;
    if (!add(left.magnitude, right.magnitude, output.magnitude)) {
      return false;
    }
  } else {
    const int ordering = compare(left.magnitude, right.magnitude);
    if (ordering >= 0) {
      output.magnitude = subtract_magnitudes(left.magnitude, right.magnitude);
      output.negative = left.negative;
    } else {
      output.magnitude = subtract_magnitudes(right.magnitude, left.magnitude);
      output.negative = right.negative;
    }
  }
  if (is_zero(output.magnitude)) {
    output.negative = false;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool shift_word_to_u128(
    std::uint64_t magnitude,
    unsigned int shift,
    UInt128& output) noexcept {
  output = UInt128{};
  if (magnitude == 0U) {
    return true;
  }
  const unsigned int width = bit_width_u64(magnitude);
  if (shift > 128U - width) {
    return false;
  }
  if (shift < 64U) {
    output.limb[0] = magnitude << shift;
    output.limb[1] = shift == 0U ? 0U : magnitude >> (64U - shift);
  } else {
    output.limb[1] = magnitude << (shift - 64U);
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline UInt128 shift_right(
    const UInt128& input,
    unsigned int shift) noexcept {
  if (shift >= 128U) {
    return UInt128{};
  }
  const unsigned int word_shift = shift / 64U;
  const unsigned int bit_shift = shift % 64U;
  UInt128 output{};
  for (std::size_t destination = 0U; destination < 2U; ++destination) {
    const std::size_t source = destination + word_shift;
    if (source >= 2U) {
      continue;
    }
    output.limb[destination] = input.limb[source] >> bit_shift;
    if (bit_shift != 0U && source + 1U < 2U) {
      output.limb[destination] |=
          input.limb[source + 1U] << (64U - bit_shift);
    }
  }
  return output;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline UInt256 shift_right(
    const UInt256& input,
    unsigned int shift) noexcept {
  if (shift >= 256U) {
    return UInt256{};
  }
  const unsigned int word_shift = shift / 64U;
  const unsigned int bit_shift = shift % 64U;
  UInt256 output{};
  for (std::size_t destination = 0U; destination < 4U; ++destination) {
    const std::size_t source = destination + word_shift;
    if (source >= 4U) {
      continue;
    }
    output.limb[destination] = input.limb[source] >> bit_shift;
    if (bit_shift != 0U && source + 1U < 4U) {
      output.limb[destination] |=
          input.limb[source + 1U] << (64U - bit_shift);
    }
  }
  return output;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool shift_left(
    const UInt256& input,
    unsigned int shift,
    UInt256& output) noexcept {
  output = UInt256{};
  const unsigned int width = bit_width(input);
  if (width == 0U) {
    return true;
  }
  if (shift > 256U - width) {
    return false;
  }
  const unsigned int word_shift = shift / 64U;
  const unsigned int bit_shift = shift % 64U;
  for (std::size_t source = 0U; source < 4U; ++source) {
    const std::size_t destination = source + word_shift;
    if (destination >= 4U) {
      continue;
    }
    output.limb[destination] |= input.limb[source] << bit_shift;
    if (bit_shift != 0U && destination + 1U < 4U) {
      output.limb[destination + 1U] |=
          input.limb[source] >> (64U - bit_shift);
    }
  }
  return true;
}

MORSEHGP3D_EXACT_PHI_HD inline void multiply_u64(
    std::uint64_t left,
    std::uint64_t right,
    std::uint64_t& low,
    std::uint64_t& high) noexcept {
  constexpr std::uint64_t mask = UINT64_C(0xffffffff);
  const std::uint64_t left_low = left & mask;
  const std::uint64_t left_high = left >> 32U;
  const std::uint64_t right_low = right & mask;
  const std::uint64_t right_high = right >> 32U;
  const std::uint64_t product_low = left_low * right_low;
  const std::uint64_t product_cross_a = left_low * right_high;
  const std::uint64_t product_cross_b = left_high * right_low;
  const std::uint64_t product_high = left_high * right_high;
  const std::uint64_t middle = (product_low >> 32U) +
                               (product_cross_a & mask) +
                               (product_cross_b & mask);
  low = (product_low & mask) | (middle << 32U);
  high = product_high + (product_cross_a >> 32U) +
         (product_cross_b >> 32U) + (middle >> 32U);
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool add_word(
    UInt256& value,
    std::size_t index,
    std::uint64_t word) noexcept {
  while (word != 0U) {
    if (index >= 4U) {
      return false;
    }
    const std::uint64_t previous = value.limb[index];
    value.limb[index] += word;
    word = value.limb[index] < previous ? 1U : 0U;
    ++index;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool multiply(
    const UInt128& left,
    const UInt128& right,
    UInt256& output) noexcept {
  output = UInt256{};
  for (std::size_t left_index = 0U; left_index < 2U; ++left_index) {
    for (std::size_t right_index = 0U; right_index < 2U; ++right_index) {
      std::uint64_t low = 0U;
      std::uint64_t high = 0U;
      multiply_u64(
          left.limb[left_index], right.limb[right_index], low, high);
      const std::size_t offset = left_index + right_index;
      if (!add_word(output, offset, low) ||
          !add_word(output, offset + 1U, high)) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool decode_binary64(
    std::uint64_t bits,
    DyadicWord& output) noexcept {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff);
  const std::uint64_t exponent_bits = (bits >> 52U) & exponent_mask;
  if (!is_finite_binary64_word(bits)) {
    return false;
  }
  std::uint64_t magnitude = bits & fraction_mask;
  if (exponent_bits != 0U) {
    magnitude |= UINT64_C(1) << 52U;
  }
  if (magnitude == 0U) {
    output = DyadicWord{};
    return true;
  }
  int exponent = exponent_bits == 0U
                     ? -1074
                     : static_cast<int>(exponent_bits) - 1023 - 52;
  const unsigned int trailing = trailing_zero_u64(magnitude);
  magnitude >>= trailing;
  exponent += static_cast<int>(trailing);
  output = DyadicWord{
      magnitude, exponent, (bits & (UINT64_C(1) << 63U)) != 0U};
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool exact_difference(
    std::uint64_t left_bits,
    std::uint64_t right_bits,
    ExactDifference& output) noexcept {
  DyadicWord left{};
  DyadicWord right{};
  if (!decode_binary64(left_bits, left) ||
      !decode_binary64(right_bits, right)) {
    return false;
  }
  if (left.magnitude == 0U && right.magnitude == 0U) {
    output = ExactDifference{};
    return true;
  }
  int common_exponent = 0;
  if (left.magnitude == 0U) {
    common_exponent = right.exponent;
  } else if (right.magnitude == 0U) {
    common_exponent = left.exponent;
  } else {
    common_exponent = left.exponent < right.exponent
                          ? left.exponent
                          : right.exponent;
  }
  const int left_shift = left.magnitude == 0U
                             ? 0
                             : left.exponent - common_exponent;
  const int right_shift = right.magnitude == 0U
                              ? 0
                              : right.exponent - common_exponent;
  if (left_shift < 0 || right_shift < 0 || left_shift > 128 ||
      right_shift > 128) {
    return false;
  }
  Signed128 left_value{};
  Signed128 negated_right{};
  if (!shift_word_to_u128(
          left.magnitude, static_cast<unsigned int>(left_shift),
          left_value.magnitude) ||
      !shift_word_to_u128(
          right.magnitude, static_cast<unsigned int>(right_shift),
          negated_right.magnitude)) {
    return false;
  }
  left_value.negative = left.negative;
  negated_right.negative = !right.negative && right.magnitude != 0U;
  if (right.negative && right.magnitude != 0U) {
    negated_right.negative = false;
  }
  Signed128 difference{};
  if (!add_signed(left_value, negated_right, difference)) {
    return false;
  }
  if (is_zero(difference.magnitude)) {
    output = ExactDifference{};
    return true;
  }
  const unsigned int trailing = trailing_zero_count(difference.magnitude);
  difference.magnitude = shift_right(difference.magnitude, trailing);
  output = ExactDifference{
      difference, common_exponent + static_cast<int>(trailing)};
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline bool exact_product(
    const ExactDifference& left,
    const ExactDifference& right,
    ExactProduct& output) noexcept {
  if (is_zero(left.value.magnitude) || is_zero(right.value.magnitude)) {
    output = ExactProduct{};
    return true;
  }
  Signed256 product{};
  if (!multiply(left.value.magnitude, right.value.magnitude,
                product.magnitude)) {
    return false;
  }
  product.negative = left.value.negative != right.value.negative;
  const unsigned int trailing = trailing_zero_count(product.magnitude);
  product.magnitude = shift_right(product.magnitude, trailing);
  output = ExactProduct{
      product,
      left.exponent + right.exponent + static_cast<int>(trailing)};
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_PHI_HD inline ResultCode exact_sign(
    const std::uint64_t witness_bits[3],
    const std::uint64_t first_support_bits[3],
    const std::uint64_t second_support_bits[3]) noexcept {
  ExactProduct terms[3]{};
  bool exponent_initialized = false;
  int common_exponent = 0;
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (!is_finite_binary64_word(witness_bits[axis]) ||
        !is_finite_binary64_word(first_support_bits[axis]) ||
        !is_finite_binary64_word(second_support_bits[axis])) {
      return ResultCode::invalid_input;
    }
    ExactDifference first_difference{};
    ExactDifference second_difference{};
    if (!exact_difference(
            witness_bits[axis], first_support_bits[axis], first_difference) ||
        !exact_difference(
            witness_bits[axis], second_support_bits[axis],
            second_difference) ||
        !exact_product(first_difference, second_difference, terms[axis])) {
      return ResultCode::requires_cpu_fallback;
    }
    if (!is_zero(terms[axis].value.magnitude) &&
        (!exponent_initialized || terms[axis].exponent < common_exponent)) {
      common_exponent = terms[axis].exponent;
      exponent_initialized = true;
    }
  }
  if (!exponent_initialized) {
    return ResultCode::zero;
  }

  Signed256 sum{};
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (is_zero(terms[axis].value.magnitude)) {
      continue;
    }
    const int shift = terms[axis].exponent - common_exponent;
    if (shift < 0 || shift > 256) {
      return ResultCode::requires_cpu_fallback;
    }
    Signed256 aligned{};
    aligned.negative = terms[axis].value.negative;
    if (!shift_left(
            terms[axis].value.magnitude,
            static_cast<unsigned int>(shift), aligned.magnitude)) {
      return ResultCode::requires_cpu_fallback;
    }
    Signed256 next{};
    if (!add_signed(sum, aligned, next)) {
      return ResultCode::requires_cpu_fallback;
    }
    sum = next;
  }
  if (is_zero(sum.magnitude)) {
    return ResultCode::zero;
  }
  return sum.negative ? ResultCode::negative : ResultCode::positive;
}

}  // namespace morsehgp3d::gpu::detail::exact_phi_fixed

#undef MORSEHGP3D_EXACT_PHI_HD
