#pragma once

#include "phase15_exact_higher_support_product_fixed.cuh"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined(__CUDACC__)
#define MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD __host__ __device__
#else
#define MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD
#endif

namespace morsehgp3d::gpu::detail::exact_higher_support_product_fixed256 {

inline constexpr std::size_t axis_count = 3U;
inline constexpr std::size_t maximum_support_size = 4U;
inline constexpr std::size_t limb_count = 4U;
inline constexpr std::size_t fixed_bit_count = 256U;

// If every aligned coordinate magnitude has bit width at most W, a
// coordinate difference is strictly smaller than 2^(W+1), and a 3D dot
// product is strictly smaller than 2^(2W+4).  Expanding the exact 2x2/3x3
// determinants and the final interval sums gives these worst-case bounds:
//
//   support-3: 4W+11        query-3: 6W+15
//   support-4: 6W+18        query-4: 8W+21
//
// The strict inequalities include every intermediate produced by the
// implementation below, not only its final predicate value.
struct ExpressionBitBound {
  std::uint64_t coordinate_multiplier{};
  std::uint64_t overhead{};
};

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline constexpr
ExpressionBitBound expression_bit_bound(
    std::size_t support_size,
    bool has_query) noexcept {
  if (support_size == 3U) {
    return has_query ? ExpressionBitBound{6U, 15U}
                     : ExpressionBitBound{4U, 11U};
  }
  if (support_size == 4U) {
    return has_query ? ExpressionBitBound{8U, 21U}
                     : ExpressionBitBound{6U, 18U};
  }
  return ExpressionBitBound{};
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline constexpr
std::uint64_t aligned_coordinate_bit_limit(
    std::size_t support_size,
    bool has_query) noexcept {
  const ExpressionBitBound bound =
      expression_bit_bound(support_size, has_query);
  return bound.coordinate_multiplier == 0U ||
          bound.overhead >= fixed_bit_count
      ? 0U
      : (fixed_bit_count - 1U - bound.overhead) /
          bound.coordinate_multiplier;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline constexpr bool
expression_fits(
    std::size_t support_size,
    bool has_query,
    std::uint64_t coordinate_width) noexcept {
  return coordinate_width <=
      aligned_coordinate_bit_limit(support_size, has_query);
}

inline constexpr std::size_t proven_maximum_expression_bit_count = 255U;
static_assert(proven_maximum_expression_bit_count < fixed_bit_count);
static_assert(aligned_coordinate_bit_limit(3U, false) == 61U);
static_assert(aligned_coordinate_bit_limit(3U, true) == 40U);
static_assert(aligned_coordinate_bit_limit(4U, false) == 39U);
static_assert(aligned_coordinate_bit_limit(4U, true) == 29U);

enum class Decision : std::uint8_t {
  certified = 0U,
  exact_false = 1U,
  requires_cpu_rational_fallback = 2U,
};

enum class TerminalGeometryDecision : std::uint8_t {
  affinely_dependent = 0U,
  boundary_reduced = 1U,
  exterior_circumcenter = 2U,
  minimal = 3U,
  requires_cpu_rational_fallback = 4U,
};

struct UInt256 {
  std::uint64_t limb[limb_count]{};
};

struct Signed256 {
  UInt256 magnitude{};
  bool negative{false};
};

struct Interval256 {
  Signed256 lower{};
  Signed256 upper{};
};

using Binary64Aabb3 =
    exact_higher_support_product_fixed::Binary64Aabb3;

struct BoxCoordinates {
  Signed256 lower[axis_count]{};
  Signed256 upper[axis_count]{};
};

struct DyadicWord {
  std::uint64_t magnitude{};
  int exponent{};
  bool negative{false};
};

using Vector3 = Interval256[axis_count];
using Matrix3 = Interval256[axis_count][axis_count];

struct AlignedProduct {
  std::size_t support_size{};
  BoxCoordinates support[maximum_support_size]{};
  BoxCoordinates query{};
  bool has_query{false};
};

struct SupportEvaluation {
  std::size_t support_size{};
  std::size_t dimension{};
  Vector3 directions[axis_count]{};
  Matrix3 gram{};
  Interval256 squared_direction_norms[axis_count]{};
  Interval256 cramer_numerators[axis_count]{};
  Interval256 gram_determinant{};
};

static_assert(std::is_standard_layout_v<UInt256>);
static_assert(std::is_trivially_copyable_v<UInt256>);
static_assert(sizeof(UInt256) == 32U);
static_assert(std::is_standard_layout_v<Signed256>);
static_assert(std::is_trivially_copyable_v<Signed256>);

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool is_zero(
    const UInt256& value) noexcept {
  for (std::size_t index = 0U; index < limb_count; ++index) {
    if (value.limb[index] != 0U) {
      return false;
    }
  }
  return true;
}

MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline void normalize(
    Signed256& value) noexcept {
  if (is_zero(value.magnitude)) {
    value.negative = false;
  }
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline unsigned int
bit_width_u64(std::uint64_t value) noexcept {
  unsigned int width = 0U;
  while (value != 0U) {
    ++width;
    value >>= 1U;
  }
  return width;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline unsigned int
bit_width(const UInt256& value) noexcept {
  for (std::size_t index = limb_count; index != 0U; --index) {
    if (value.limb[index - 1U] != 0U) {
      return static_cast<unsigned int>((index - 1U) * 64U) +
          bit_width_u64(value.limb[index - 1U]);
    }
  }
  return 0U;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline int compare(
    const UInt256& left,
    const UInt256& right) noexcept {
  for (std::size_t index = limb_count; index != 0U; --index) {
    if (left.limb[index - 1U] < right.limb[index - 1U]) {
      return -1;
    }
    if (left.limb[index - 1U] > right.limb[index - 1U]) {
      return 1;
    }
  }
  return 0;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline int compare(
    const Signed256& left,
    const Signed256& right) noexcept {
  const bool left_negative = left.negative && !is_zero(left.magnitude);
  const bool right_negative = right.negative && !is_zero(right.magnitude);
  if (left_negative != right_negative) {
    return left_negative ? -1 : 1;
  }
  const int magnitude_order = compare(left.magnitude, right.magnitude);
  return left_negative ? -magnitude_order : magnitude_order;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool is_negative(
    const Signed256& value) noexcept {
  return value.negative && !is_zero(value.magnitude);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool is_nonpositive(
    const Signed256& value) noexcept {
  return value.negative || is_zero(value.magnitude);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool add(
    const UInt256& left,
    const UInt256& right,
    UInt256& output) noexcept {
  output = UInt256{};
  std::uint64_t carry = 0U;
  for (std::size_t index = 0U; index < limb_count; ++index) {
    const std::uint64_t first = left.limb[index] + right.limb[index];
    const std::uint64_t carry_first =
        first < left.limb[index] ? 1U : 0U;
    const std::uint64_t second = first + carry;
    const std::uint64_t carry_second = second < first ? 1U : 0U;
    output.limb[index] = second;
    carry = carry_first | carry_second;
  }
  return carry == 0U;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline UInt256
subtract_magnitudes(
    const UInt256& greater,
    const UInt256& lesser) noexcept {
  UInt256 output{};
  std::uint64_t borrow = 0U;
  for (std::size_t index = 0U; index < limb_count; ++index) {
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

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool add_signed(
    const Signed256& left,
    const Signed256& right,
    Signed256& output) noexcept {
  output = Signed256{};
  const bool left_negative = left.negative && !is_zero(left.magnitude);
  const bool right_negative = right.negative && !is_zero(right.magnitude);
  if (left_negative == right_negative) {
    output.negative = left_negative;
    if (!add(left.magnitude, right.magnitude, output.magnitude)) {
      return false;
    }
  } else {
    const int ordering = compare(left.magnitude, right.magnitude);
    if (ordering >= 0) {
      output.magnitude =
          subtract_magnitudes(left.magnitude, right.magnitude);
      output.negative = left_negative;
    } else {
      output.magnitude =
          subtract_magnitudes(right.magnitude, left.magnitude);
      output.negative = right_negative;
    }
  }
  normalize(output);
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline Signed256 negate(
    Signed256 value) noexcept {
  if (!is_zero(value.magnitude)) {
    value.negative = !value.negative;
  }
  return value;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool subtract_signed(
    const Signed256& left,
    const Signed256& right,
    Signed256& output) noexcept {
  return add_signed(left, negate(right), output);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool shift_word(
    std::uint64_t magnitude,
    unsigned int shift,
    UInt256& output) noexcept {
  output = UInt256{};
  if (magnitude == 0U) {
    return true;
  }
  const unsigned int width = bit_width_u64(magnitude);
  if (shift > fixed_bit_count - width) {
    return false;
  }
  const unsigned int word_shift = shift / 64U;
  const unsigned int bit_shift = shift % 64U;
  output.limb[word_shift] = magnitude << bit_shift;
  if (bit_shift != 0U && word_shift + 1U < limb_count) {
    output.limb[word_shift + 1U] = magnitude >> (64U - bit_shift);
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool shift_left(
    const UInt256& input,
    unsigned int shift,
    UInt256& output) noexcept {
  output = UInt256{};
  const unsigned int width = bit_width(input);
  if (width == 0U) {
    return true;
  }
  if (shift > fixed_bit_count - width) {
    return false;
  }
  const unsigned int word_shift = shift / 64U;
  const unsigned int bit_shift = shift % 64U;
  for (std::size_t source = 0U; source < limb_count; ++source) {
    const std::size_t destination = source + word_shift;
    if (destination >= limb_count) {
      break;
    }
    output.limb[destination] |= input.limb[source] << bit_shift;
    if (bit_shift != 0U && destination + 1U < limb_count) {
      output.limb[destination + 1U] |=
          input.limb[source] >> (64U - bit_shift);
    }
  }
  return true;
}

MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline void multiply_u64(
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
      (product_cross_a & mask) + (product_cross_b & mask);
  low = (product_low & mask) | (middle << 32U);
  high = product_high + (product_cross_a >> 32U) +
      (product_cross_b >> 32U) + (middle >> 32U);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool add_word(
    UInt256& value,
    std::size_t index,
    std::uint64_t word) noexcept {
  while (word != 0U) {
    if (index >= limb_count) {
      return false;
    }
    const std::uint64_t previous = value.limb[index];
    value.limb[index] += word;
    word = value.limb[index] < previous ? 1U : 0U;
    ++index;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline std::size_t
active_limb_count(const UInt256& value) noexcept {
  for (std::size_t index = limb_count; index != 0U; --index) {
    if (value.limb[index - 1U] != 0U) {
      return index;
    }
  }
  return 0U;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool multiply(
    const UInt256& left,
    const UInt256& right,
    UInt256& output) noexcept {
  output = UInt256{};
  const std::size_t left_count = active_limb_count(left);
  const std::size_t right_count = active_limb_count(right);
  for (std::size_t left_index = 0U;
       left_index < left_count;
       ++left_index) {
    for (std::size_t right_index = 0U;
         right_index < right_count;
         ++right_index) {
      const std::size_t offset = left_index + right_index;
      if (offset >= limb_count) {
        return false;
      }
      std::uint64_t low = 0U;
      std::uint64_t high = 0U;
      multiply_u64(
          left.limb[left_index], right.limb[right_index], low, high);
      if (!add_word(output, offset, low) ||
          !add_word(output, offset + 1U, high)) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool multiply_signed(
    const Signed256& left,
    const Signed256& right,
    Signed256& output) noexcept {
  output = Signed256{};
  if (!multiply(left.magnitude, right.magnitude, output.magnitude)) {
    return false;
  }
  output.negative =
      !is_zero(output.magnitude) && left.negative != right.negative;
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool scale_by_two(
    const Signed256& input,
    Signed256& output) noexcept {
  output = Signed256{};
  output.negative = input.negative;
  if (!shift_left(input.magnitude, 1U, output.magnitude)) {
    return false;
  }
  normalize(output);
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline Interval256
singleton(const Signed256& value) noexcept {
  return Interval256{value, value};
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool add_interval(
    const Interval256& left,
    const Interval256& right,
    Interval256& output) noexcept {
  return add_signed(left.lower, right.lower, output.lower) &&
      add_signed(left.upper, right.upper, output.upper);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
subtract_interval(
    const Interval256& left,
    const Interval256& right,
    Interval256& output) noexcept {
  return subtract_signed(left.lower, right.upper, output.lower) &&
      subtract_signed(left.upper, right.lower, output.upper);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
multiply_interval(
    const Interval256& left,
    const Interval256& right,
    Interval256& output) noexcept {
  Signed256 candidates[4]{};
  if (!multiply_signed(left.lower, right.lower, candidates[0]) ||
      !multiply_signed(left.lower, right.upper, candidates[1]) ||
      !multiply_signed(left.upper, right.lower, candidates[2]) ||
      !multiply_signed(left.upper, right.upper, candidates[3])) {
    return false;
  }
  output.lower = candidates[0];
  output.upper = candidates[0];
  for (std::size_t index = 1U; index < 4U; ++index) {
    if (compare(candidates[index], output.lower) < 0) {
      output.lower = candidates[index];
    }
    if (compare(candidates[index], output.upper) > 0) {
      output.upper = candidates[index];
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool square_interval(
    const Interval256& input,
    Interval256& output) noexcept {
  Signed256 lower_squared{};
  Signed256 upper_squared{};
  if (!multiply_signed(input.lower, input.lower, lower_squared) ||
      !multiply_signed(input.upper, input.upper, upper_squared)) {
    return false;
  }
  output.upper = compare(lower_squared, upper_squared) >= 0
      ? lower_squared
      : upper_squared;
  if (is_nonpositive(input.lower) && !is_negative(input.upper)) {
    output.lower = Signed256{};
  } else {
    output.lower = compare(lower_squared, upper_squared) <= 0
        ? lower_squared
        : upper_squared;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
scale_interval_by_two(
    const Interval256& input,
    Interval256& output) noexcept {
  return scale_by_two(input.lower, output.lower) &&
      scale_by_two(input.upper, output.upper);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool dot(
    const Vector3& left,
    const Vector3& right,
    bool same_vector,
    Interval256& output) noexcept {
  output = singleton(Signed256{});
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    Interval256 term{};
    if (!(same_vector ? square_interval(left[axis], term)
                      : multiply_interval(left[axis], right[axis], term))) {
      return false;
    }
    Interval256 next{};
    if (!add_interval(output, term, next)) {
      return false;
    }
    output = next;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool determinant(
    const Matrix3& matrix,
    std::size_t dimension,
    Interval256& output) noexcept {
  if (dimension == 1U) {
    output = matrix[0][0];
    return true;
  }
  if (dimension == 2U) {
    Interval256 first{};
    Interval256 second{};
    return multiply_interval(matrix[0][0], matrix[1][1], first) &&
        multiply_interval(matrix[0][1], matrix[1][0], second) &&
        subtract_interval(first, second, output);
  }
  if (dimension != 3U) {
    return false;
  }
  Interval256 first_product{};
  Interval256 second_product{};
  Interval256 first_minor{};
  Interval256 second_minor{};
  Interval256 third_minor{};
  if (!multiply_interval(matrix[1][1], matrix[2][2], first_product) ||
      !multiply_interval(matrix[1][2], matrix[2][1], second_product) ||
      !subtract_interval(first_product, second_product, first_minor) ||
      !multiply_interval(matrix[1][0], matrix[2][2], first_product) ||
      !multiply_interval(matrix[1][2], matrix[2][0], second_product) ||
      !subtract_interval(first_product, second_product, second_minor) ||
      !multiply_interval(matrix[1][0], matrix[2][1], first_product) ||
      !multiply_interval(matrix[1][1], matrix[2][0], second_product) ||
      !subtract_interval(first_product, second_product, third_minor)) {
    return false;
  }
  Interval256 first_term{};
  Interval256 second_term{};
  Interval256 third_term{};
  Interval256 difference{};
  return multiply_interval(matrix[0][0], first_minor, first_term) &&
      multiply_interval(matrix[0][1], second_minor, second_term) &&
      multiply_interval(matrix[0][2], third_minor, third_term) &&
      subtract_interval(first_term, second_term, difference) &&
      add_interval(difference, third_term, output);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
is_finite_binary64_word(std::uint64_t bits) noexcept {
  return ((bits >> 52U) & UINT64_C(0x7ff)) != UINT64_C(0x7ff);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline std::uint64_t
canonical_binary64_word(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_mask = UINT64_C(1) << 63U;
  return bits == sign_mask ? 0U : bits;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline std::uint64_t
binary64_order_key(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_mask = UINT64_C(1) << 63U;
  const std::uint64_t canonical = canonical_binary64_word(bits);
  return (canonical & sign_mask) != 0U
      ? ~canonical
      : canonical ^ sign_mask;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
binary64_interval_ordered(
    std::uint64_t lower,
    std::uint64_t upper) noexcept {
  return is_finite_binary64_word(lower) &&
      is_finite_binary64_word(upper) &&
      binary64_order_key(lower) <= binary64_order_key(upper);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline unsigned int
trailing_zero_u64(std::uint64_t value) noexcept {
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

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool decode_binary64(
    std::uint64_t raw_bits,
    DyadicWord& output) noexcept {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff);
  const std::uint64_t bits = canonical_binary64_word(raw_bits);
  if (!is_finite_binary64_word(bits)) {
    return false;
  }
  const std::uint64_t exponent_bits = (bits >> 52U) & exponent_mask;
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
      magnitude,
      exponent,
      (bits & (UINT64_C(1) << 63U)) != 0U};
  return true;
}

MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline void update_minimum_exponent(
    const DyadicWord& word,
    bool& initialized,
    int& minimum_exponent) noexcept {
  if (word.magnitude != 0U &&
      (!initialized || word.exponent < minimum_exponent)) {
    initialized = true;
    minimum_exponent = word.exponent;
  }
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool scan_box(
    const Binary64Aabb3& box,
    bool& exponent_initialized,
    int& minimum_exponent) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (!binary64_interval_ordered(box.lower[axis], box.upper[axis])) {
      return false;
    }
    DyadicWord lower{};
    DyadicWord upper{};
    if (!decode_binary64(box.lower[axis], lower) ||
        !decode_binary64(box.upper[axis], upper)) {
      return false;
    }
    update_minimum_exponent(lower, exponent_initialized, minimum_exponent);
    update_minimum_exponent(upper, exponent_initialized, minimum_exponent);
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool align_word(
    const DyadicWord& word,
    int minimum_exponent,
    std::uint64_t coordinate_bit_limit,
    Signed256& output) noexcept {
  output = Signed256{};
  if (word.magnitude == 0U) {
    return true;
  }
  const int shift = word.exponent - minimum_exponent;
  const std::uint64_t width = bit_width_u64(word.magnitude);
  if (shift < 0 || width > coordinate_bit_limit ||
      static_cast<std::uint64_t>(shift) >
          coordinate_bit_limit - width ||
      !shift_word(
          word.magnitude,
          static_cast<unsigned int>(shift),
          output.magnitude)) {
    return false;
  }
  output.negative = word.negative;
  normalize(output);
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool align_box(
    const Binary64Aabb3& source,
    int minimum_exponent,
    std::uint64_t coordinate_bit_limit,
    BoxCoordinates& output) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    DyadicWord lower{};
    DyadicWord upper{};
    if (!decode_binary64(source.lower[axis], lower) ||
        !decode_binary64(source.upper[axis], upper) ||
        !align_word(
            lower,
            minimum_exponent,
            coordinate_bit_limit,
            output.lower[axis]) ||
        !align_word(
            upper,
            minimum_exponent,
            coordinate_bit_limit,
            output.upper[axis]) ||
        compare(output.lower[axis], output.upper[axis]) > 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool align_product(
    const Binary64Aabb3 support[maximum_support_size],
    std::size_t support_size,
    const Binary64Aabb3* query,
    AlignedProduct& output) noexcept {
  output = AlignedProduct{};
  if (support_size != 3U && support_size != 4U) {
    return false;
  }
  bool exponent_initialized = false;
  int minimum_exponent = 0;
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (!exact_higher_support_product_fixed256::scan_box(
            support[index], exponent_initialized, minimum_exponent)) {
      return false;
    }
  }
  if (query != nullptr &&
      !exact_higher_support_product_fixed256::scan_box(
          *query, exponent_initialized, minimum_exponent)) {
    return false;
  }
  output.support_size = support_size;
  output.has_query = query != nullptr;
  const std::uint64_t coordinate_bit_limit =
      aligned_coordinate_bit_limit(support_size, output.has_query);
  if (coordinate_bit_limit == 0U) {
    return false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (!align_box(
            support[index],
            minimum_exponent,
            coordinate_bit_limit,
            output.support[index])) {
      return false;
    }
  }
  return query == nullptr ||
      align_box(
          *query,
          minimum_exponent,
          coordinate_bit_limit,
          output.query);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool difference_box(
    const BoxCoordinates& left,
    const BoxCoordinates& right,
    Vector3& output) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (!subtract_signed(
            left.lower[axis], right.upper[axis], output[axis].lower) ||
        !subtract_signed(
            left.upper[axis], right.lower[axis], output[axis].upper)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool evaluate_support(
    const AlignedProduct& product,
    SupportEvaluation& output) noexcept {
  output = SupportEvaluation{};
  output.support_size = product.support_size;
  output.dimension = product.support_size - 1U;
  for (std::size_t direction = 0U;
       direction < output.dimension;
       ++direction) {
    if (!difference_box(
            product.support[direction + 1U],
            product.support[0],
            output.directions[direction])) {
      return false;
    }
  }
  for (std::size_t row = 0U; row < output.dimension; ++row) {
    for (std::size_t column = 0U;
         column < output.dimension;
         ++column) {
      if (!dot(
              output.directions[row],
              output.directions[column],
              row == column,
              output.gram[row][column])) {
        return false;
      }
    }
    output.squared_direction_norms[row] = output.gram[row][row];
  }
  if (!determinant(
          output.gram, output.dimension, output.gram_determinant)) {
    return false;
  }
  for (std::size_t column = 0U;
       column < output.dimension;
       ++column) {
    Matrix3 replaced{};
    for (std::size_t row = 0U; row < output.dimension; ++row) {
      for (std::size_t source_column = 0U;
           source_column < output.dimension;
           ++source_column) {
        replaced[row][source_column] =
            source_column == column
            ? output.squared_direction_norms[row]
            : output.gram[row][source_column];
      }
    }
    if (!determinant(
            replaced,
            output.dimension,
            output.cramer_numerators[column])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
barycentric_numerators(
    const SupportEvaluation& support,
    Interval256 output[maximum_support_size]) noexcept {
  Interval256 sum = singleton(Signed256{});
  for (std::size_t index = 0U; index < support.dimension; ++index) {
    output[index + 1U] = support.cramer_numerators[index];
    Interval256 next{};
    if (!add_interval(sum, support.cramer_numerators[index], next)) {
      return false;
    }
    sum = next;
  }
  Interval256 twice_determinant{};
  return scale_interval_by_two(
             support.gram_determinant, twice_determinant) &&
      subtract_interval(twice_determinant, sum, output[0]);
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
triangle_has_nonacute_vertex_for_every_tuple(
    const AlignedProduct& product,
    bool& certified) noexcept {
  certified = false;
  for (std::size_t vertex = 0U; vertex < 3U; ++vertex) {
    const std::size_t first = (vertex + 1U) % 3U;
    const std::size_t second = (vertex + 2U) % 3U;
    Signed256 sum{};
    for (std::size_t axis = 0U; axis < axis_count; ++axis) {
      bool initialized = false;
      Signed256 axis_maximum{};
      for (std::size_t vertex_selector = 0U;
           vertex_selector < 2U;
           ++vertex_selector) {
        const Signed256& vertex_value = vertex_selector == 0U
            ? product.support[vertex].lower[axis]
            : product.support[vertex].upper[axis];
        for (std::size_t first_selector = 0U;
             first_selector < 2U;
             ++first_selector) {
          const Signed256& first_value = first_selector == 0U
              ? product.support[first].lower[axis]
              : product.support[first].upper[axis];
          for (std::size_t second_selector = 0U;
               second_selector < 2U;
               ++second_selector) {
            const Signed256& second_value = second_selector == 0U
                ? product.support[second].lower[axis]
                : product.support[second].upper[axis];
            Signed256 first_difference{};
            Signed256 second_difference{};
            Signed256 candidate{};
            if (!subtract_signed(
                    first_value, vertex_value, first_difference) ||
                !subtract_signed(
                    second_value, vertex_value, second_difference) ||
                !multiply_signed(
                    first_difference, second_difference, candidate)) {
              return false;
            }
            if (!initialized || compare(candidate, axis_maximum) > 0) {
              axis_maximum = candidate;
              initialized = true;
            }
          }
        }
      }
      Signed256 next{};
      if (!initialized || !add_signed(sum, axis_maximum, next)) {
        return false;
      }
      sum = next;
    }
    if (is_nonpositive(sum)) {
      certified = true;
      return true;
    }
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
query_scaled_power_for_coordinates(
    const AlignedProduct& product,
    const SupportEvaluation& support,
    const BoxCoordinates& query,
    Interval256& output) noexcept {
  Vector3 delta{};
  Interval256 delta_square{};
  if (!difference_box(query, product.support[0], delta) ||
      !dot(delta, delta, true, delta_square) ||
      !multiply_interval(
          support.gram_determinant, delta_square, output)) {
    return false;
  }
  for (std::size_t index = 0U; index < support.dimension; ++index) {
    Interval256 direction_dot{};
    Interval256 term{};
    Interval256 next{};
    if (!dot(
            support.directions[index],
            delta,
            false,
            direction_dot) ||
        !multiply_interval(
            direction_dot,
            support.cramer_numerators[index],
            term) ||
        !subtract_interval(output, term, next)) {
      return false;
    }
    output = next;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline bool
query_scaled_power(
    const AlignedProduct& product,
    const SupportEvaluation& support,
    Interval256& output) noexcept {
  if (!product.has_query ||
      !query_scaled_power_for_coordinates(
          product, support, product.query, output)) {
    return false;
  }
  bool initialized = false;
  Signed256 corner_upper{};
  for (std::size_t selector = 0U; selector < 8U; ++selector) {
    BoxCoordinates corner{};
    for (std::size_t axis = 0U; axis < axis_count; ++axis) {
      const Signed256 coordinate =
          (selector & (std::size_t{1} << axis)) != 0U
          ? product.query.upper[axis]
          : product.query.lower[axis];
      corner.lower[axis] = coordinate;
      corner.upper[axis] = coordinate;
    }
    Interval256 candidate{};
    if (!query_scaled_power_for_coordinates(
            product, support, corner, candidate)) {
      return false;
    }
    if (!initialized || compare(candidate.upper, corner_upper) > 0) {
      initialized = true;
      corner_upper = candidate.upper;
    }
  }
  if (!initialized) {
    return false;
  }
  if (compare(corner_upper, output.upper) < 0) {
    output.upper = corner_upper;
  }
  return compare(output.lower, output.upper) <= 0;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline Decision
no_well_centered_support(
    const Binary64Aabb3 support_boxes[maximum_support_size],
    std::size_t support_size) noexcept {
  AlignedProduct product{};
  SupportEvaluation support{};
  if (!align_product(
          support_boxes, support_size, nullptr, product) ||
      !evaluate_support(product, support)) {
    return Decision::requires_cpu_rational_fallback;
  }
  if (is_nonpositive(support.gram_determinant.upper)) {
    return Decision::certified;
  }
  if (support_size == 3U) {
    bool nonacute = false;
    if (!triangle_has_nonacute_vertex_for_every_tuple(
            product, nonacute)) {
      return Decision::requires_cpu_rational_fallback;
    }
    if (nonacute) {
      return Decision::certified;
    }
  }
  Interval256 barycentric[maximum_support_size]{};
  if (!barycentric_numerators(support, barycentric)) {
    return Decision::requires_cpu_rational_fallback;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (is_nonpositive(barycentric[index].upper)) {
      return Decision::certified;
    }
  }
  return Decision::exact_false;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline
TerminalGeometryDecision terminal_support_geometry(
    const Binary64Aabb3 support_boxes[maximum_support_size],
    std::size_t support_size) noexcept {
  if (support_size != 3U && support_size != 4U) {
    return TerminalGeometryDecision::requires_cpu_rational_fallback;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    for (std::size_t axis = 0U; axis < axis_count; ++axis) {
      if (canonical_binary64_word(support_boxes[index].lower[axis]) !=
          canonical_binary64_word(support_boxes[index].upper[axis])) {
        return TerminalGeometryDecision::requires_cpu_rational_fallback;
      }
    }
  }

  AlignedProduct product{};
  SupportEvaluation support{};
  if (!align_product(
          support_boxes, support_size, nullptr, product) ||
      !evaluate_support(product, support) ||
      compare(
          support.gram_determinant.lower,
          support.gram_determinant.upper) != 0) {
    return TerminalGeometryDecision::requires_cpu_rational_fallback;
  }
  const Signed256 zero{};
  const int determinant_sign =
      compare(support.gram_determinant.lower, zero);
  if (determinant_sign == 0) {
    return TerminalGeometryDecision::affinely_dependent;
  }
  if (determinant_sign < 0) {
    return TerminalGeometryDecision::requires_cpu_rational_fallback;
  }

  Interval256 barycentric[maximum_support_size]{};
  if (!barycentric_numerators(support, barycentric)) {
    return TerminalGeometryDecision::requires_cpu_rational_fallback;
  }
  bool has_zero = false;
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (compare(barycentric[index].lower, barycentric[index].upper) != 0) {
      return TerminalGeometryDecision::requires_cpu_rational_fallback;
    }
    const int sign = compare(barycentric[index].lower, zero);
    if (sign < 0) {
      return TerminalGeometryDecision::exterior_circumcenter;
    }
    has_zero = has_zero || sign == 0;
  }
  return has_zero
      ? TerminalGeometryDecision::boundary_reduced
      : TerminalGeometryDecision::minimal;
}

[[nodiscard]] MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD inline Decision
query_strictly_inside_every_independent_sphere(
    const Binary64Aabb3 support_boxes[maximum_support_size],
    std::size_t support_size,
    const Binary64Aabb3& query_box) noexcept {
  AlignedProduct product{};
  SupportEvaluation support{};
  Interval256 power{};
  if (!align_product(
          support_boxes, support_size, &query_box, product) ||
      !evaluate_support(product, support) ||
      !query_scaled_power(product, support, power)) {
    return Decision::requires_cpu_rational_fallback;
  }
  return is_negative(power.upper)
      ? Decision::certified
      : Decision::exact_false;
}

}  // namespace morsehgp3d::gpu::detail::exact_higher_support_product_fixed256

#undef MORSEHGP3D_HIGHER_SUPPORT_FIXED_HD
