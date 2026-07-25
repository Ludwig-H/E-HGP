#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct IntegrityVerifiedCheckpointTag {};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_unordered_pair_count(
    std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  std::size_t first = point_count;
  std::size_t second = point_count - 1U;
  if ((first & 1U) == 0U) {
    first /= 2U;
  } else {
    second /= 2U;
  }
  return checked_multiply(
      first,
      second,
      "the pair-support unordered pair count overflows size_t");
}

[[nodiscard]] std::size_t historical_non_center_work_unit_count(
    const ExactPairSupportStreamAudit& audit) {
  return checked_add(
      checked_add(
          audit.support_product_visit_count,
          audit.witness_node_visit_count,
          "the pair-support historical non-center work overflows size_t"),
      audit.closed_ball_node_visit_count,
      "the pair-support historical non-center work overflows size_t");
}

[[nodiscard]] std::size_t center_cover_post_attempt_work_limit(
    std::size_t historical_non_center_work) {
  static_assert(
      pair_support_center_cover_historical_work_denominator > 0U);
  return checked_add(
      checked_multiply(
          2U,
          pair_support_center_cover_atomic_work_unit_count,
          "the pair-support center-cover burst limit overflows size_t"),
      historical_non_center_work /
          pair_support_center_cover_historical_work_denominator,
      "the pair-support center-cover amortized limit overflows size_t");
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    std::string_view message) {
  if (value > static_cast<std::size_t>(
                  std::numeric_limits<std::uint64_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    std::string_view message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] bool can_add_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  return current <= maximum && increment <= maximum - current;
}

// A common dyadic exponent turns every sign-only predicate below into integer
// arithmetic.  The conservative 124-bit coordinate cap proves all subsequent
// int256_t operations safe before any unchecked fixed-width operation occurs:
// coordinate differences use at most 125 bits, their products at most
// 250 bits, the three-axis maximum sum fewer than 252 bits, and the anchor
// identity multiplied by four fewer than 254 bits.  Inputs outside this
// envelope fall back to the arbitrary-precision path before a scientific
// decision is made.
using BoundedExactInteger = boost::multiprecision::int256_t;
constexpr std::uint64_t bounded_exact_coordinate_bit_limit = 124U;
static_assert(
    std::numeric_limits<BoundedExactInteger>::digits >= 254,
    "the bounded exact sign kernel needs at least 254 magnitude bits");

struct Binary64DyadicWord {
  std::uint64_t magnitude{};
  int exponent{};
  bool negative{};
};

[[nodiscard]] Binary64DyadicWord decode_binary64_dyadic_word(
    std::uint64_t input_bits) {
  constexpr std::uint64_t fraction_mask =
      (std::uint64_t{1} << 52U) - 1U;
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff);
  constexpr int exponent_bias = 1023;
  constexpr int fraction_bit_count = 52;
  constexpr int subnormal_exponent = -1074;

  const std::uint64_t bits =
      exact::canonicalize_binary64_bits(input_bits);
  const bool negative =
      (bits & exact::binary64_sign_mask) != 0U;
  const std::uint64_t exponent_bits =
      (bits >> 52U) & exponent_mask;
  std::uint64_t magnitude = bits & fraction_mask;
  if (exponent_bits != 0U) {
    magnitude |= std::uint64_t{1} << 52U;
  }
  if (magnitude == 0U) {
    return {};
  }
  int exponent = exponent_bits == 0U
      ? subnormal_exponent
      : static_cast<int>(exponent_bits) -
            exponent_bias - fraction_bit_count;
  const unsigned int trailing_zero_count =
      static_cast<unsigned int>(std::countr_zero(magnitude));
  magnitude >>= trailing_zero_count;
  exponent += static_cast<int>(trailing_zero_count);
  return Binary64DyadicWord{magnitude, exponent, negative};
}

template <std::size_t WordCount>
[[nodiscard]] std::optional<std::array<BoundedExactInteger, WordCount>>
try_align_bounded_exact_dyadics(
    const std::array<Binary64DyadicWord, WordCount>& words) {
  int minimum_exponent = 0;
  bool exponent_initialized = false;
  for (const Binary64DyadicWord& word : words) {
    if (word.magnitude == 0U) {
      continue;
    }
    if (!exponent_initialized || word.exponent < minimum_exponent) {
      minimum_exponent = word.exponent;
      exponent_initialized = true;
    }
  }

  std::array<BoundedExactInteger, WordCount> aligned{};
  if (!exponent_initialized) {
    return aligned;
  }
  for (std::size_t index = 0U; index < words.size(); ++index) {
    const Binary64DyadicWord& word = words[index];
    if (word.magnitude == 0U) {
      continue;
    }
    const std::int64_t shift =
        static_cast<std::int64_t>(word.exponent) -
        static_cast<std::int64_t>(minimum_exponent);
    const std::uint64_t magnitude_bit_count =
        static_cast<std::uint64_t>(std::bit_width(word.magnitude));
    if (shift < 0 ||
        static_cast<std::uint64_t>(shift) >
            bounded_exact_coordinate_bit_limit - magnitude_bit_count) {
      return std::nullopt;
    }
    BoundedExactInteger value{word.magnitude};
    value <<= static_cast<unsigned int>(shift);
    aligned[index] = word.negative ? -value : value;
  }
  return aligned;
}

void validate_exact_dyadic_box(
    const spatial::ExactDyadicAabb3& box) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (exact::binary64_total_order_key(
            box.upper_binary64_bits[axis]) <
        exact::binary64_total_order_key(
            box.lower_binary64_bits[axis])) {
      throw std::invalid_argument(
          "an exact dyadic AABB has a reversed axis");
    }
  }
}

[[nodiscard]] int bounded_exact_sign(
    const BoundedExactInteger& value) noexcept {
  if (value < 0) {
    return -1;
  }
  return value == 0 ? 0 : 1;
}

[[nodiscard]] std::optional<int>
try_bounded_exact_diametral_phi_aabb_maximum_sign(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  validate_exact_dyadic_box(first_support_box);
  validate_exact_dyadic_box(second_support_box);
  validate_exact_dyadic_box(query_box);

  std::array<Binary64DyadicWord, 18U> words{};
  const std::array<const spatial::ExactDyadicAabb3*, 3U> boxes{
      &first_support_box, &second_support_box, &query_box};
  for (std::size_t box_index = 0U;
       box_index < boxes.size();
       ++box_index) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      words[box_index * 6U + axis] =
          decode_binary64_dyadic_word(
              boxes[box_index]->lower_binary64_bits[axis]);
      words[box_index * 6U + 3U + axis] =
          decode_binary64_dyadic_word(
              boxes[box_index]->upper_binary64_bits[axis]);
    }
  }
  const std::optional<std::array<BoundedExactInteger, 18U>>
      aligned = try_align_bounded_exact_dyadics(words);
  if (!aligned.has_value()) {
    return std::nullopt;
  }

  BoundedExactInteger total_maximum = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    bool initialized = false;
    BoundedExactInteger axis_maximum = 0;
    for (std::size_t query_selector = 0U;
         query_selector < 2U;
         ++query_selector) {
      const BoundedExactInteger& query =
          (*aligned)[12U + query_selector * 3U + axis];
      for (std::size_t first_selector = 0U;
           first_selector < 2U;
           ++first_selector) {
        const BoundedExactInteger& first =
            (*aligned)[first_selector * 3U + axis];
        for (std::size_t second_selector = 0U;
             second_selector < 2U;
             ++second_selector) {
          const BoundedExactInteger& second =
              (*aligned)[6U + second_selector * 3U + axis];
          const BoundedExactInteger candidate =
              (query - first) * (query - second);
          if (!initialized || axis_maximum < candidate) {
            initialized = true;
            axis_maximum = candidate;
          }
        }
      }
    }
    total_maximum += axis_maximum;
  }
  return bounded_exact_sign(total_maximum);
}

// For one coordinate, let A=[a0,a1], B=[b0,b1] and
//
//   g(x) = max_{a in A,b in B} (x-a)(x-b).
//
// When the intervals are disjoint, the nearest endpoint pair dominates in
// their gap and min g is minus one quarter of the squared gap.  When they
// overlap strictly, the two crossed endpoint products are dominated and the
// lower/lower and upper/upper parabolas meet at the minimum.  Writing
// wa=a1-a0 and wb=b1-b0 gives
//
//   min g = wa wb (b1-a0)(a1-b0) / (wa+wb)^2.
//
// Support coordinates are independent between axes, so the three-dimensional
// minimax is the sum of these three one-dimensional minima.  Under the
// 124-bit aligned-coordinate envelope, a numerator accumulated over all three
// axes needs fewer than 1006 magnitude bits: an overlapping-axis numerator
// needs fewer than 500, its denominator fewer than 252, and a common
// denominator for three axes contributes at most another 504 bits.
using ContinuousCoreExactInteger = boost::multiprecision::int1024_t;
static_assert(
    std::numeric_limits<ContinuousCoreExactInteger>::digits >= 1006,
    "the continuous diametral-core kernel needs at least 1006 magnitude bits");

template <typename Integer>
struct ExactUnreducedFraction {
  Integer numerator{};
  Integer denominator{1};
};

template <typename Integer>
[[nodiscard]] int continuous_core_minimum_sign_from_aligned_endpoints(
    const std::array<Integer, 12U>& endpoints) {
  ExactUnreducedFraction<Integer> total;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const Integer& first_lower = endpoints[axis];
    const Integer& first_upper = endpoints[3U + axis];
    const Integer& second_lower = endpoints[6U + axis];
    const Integer& second_upper = endpoints[9U + axis];
    if (first_upper < first_lower ||
        second_upper < second_lower) {
      throw std::logic_error(
          "aligned continuous-core endpoints form a reversed box");
    }

    ExactUnreducedFraction<Integer> coordinate;
    if (first_upper <= second_lower) {
      const Integer gap = second_lower - first_upper;
      coordinate.numerator = -(gap * gap);
      coordinate.denominator = 4;
    } else if (second_upper <= first_lower) {
      const Integer gap = first_lower - second_upper;
      coordinate.numerator = -(gap * gap);
      coordinate.denominator = 4;
    } else {
      const Integer first_width = first_upper - first_lower;
      const Integer second_width = second_upper - second_lower;
      const Integer width_sum = first_width + second_width;
      if (width_sum <= 0) {
        throw std::logic_error(
            "strictly overlapping intervals have zero total width");
      }
      coordinate.numerator =
          first_width * second_width *
          (second_upper - first_lower) *
          (first_upper - second_lower);
      coordinate.denominator = width_sum * width_sum;
    }
    total.numerator =
        total.numerator * coordinate.denominator +
        coordinate.numerator * total.denominator;
    total.denominator *= coordinate.denominator;
  }
  if (total.denominator <= 0) {
    throw std::logic_error(
        "the continuous-core denominator is not positive");
  }
  if (total.numerator < 0) {
    return -1;
  }
  return total.numerator == 0 ? 0 : 1;
}

[[nodiscard]] std::array<Binary64DyadicWord, 12U>
continuous_core_endpoint_words(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box) {
  validate_exact_dyadic_box(first_support_box);
  validate_exact_dyadic_box(second_support_box);
  std::array<Binary64DyadicWord, 12U> words{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    words[axis] = decode_binary64_dyadic_word(
        first_support_box.lower_binary64_bits[axis]);
    words[3U + axis] = decode_binary64_dyadic_word(
        first_support_box.upper_binary64_bits[axis]);
    words[6U + axis] = decode_binary64_dyadic_word(
        second_support_box.lower_binary64_bits[axis]);
    words[9U + axis] = decode_binary64_dyadic_word(
        second_support_box.upper_binary64_bits[axis]);
  }
  return words;
}

[[nodiscard]] std::optional<int>
try_bounded_exact_diametral_phi_continuous_core_minimum_sign(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box) {
  const std::array<Binary64DyadicWord, 12U> words =
      continuous_core_endpoint_words(
          first_support_box, second_support_box);
  const std::optional<std::array<BoundedExactInteger, 12U>> aligned =
      try_align_bounded_exact_dyadics(words);
  if (!aligned.has_value()) {
    return std::nullopt;
  }
  std::array<ContinuousCoreExactInteger, 12U> widened{};
  for (std::size_t index = 0U; index < widened.size(); ++index) {
    widened[index] = (*aligned)[index];
  }
  return continuous_core_minimum_sign_from_aligned_endpoints(widened);
}

struct BoundedExactAnchorPhi {
  std::array<Binary64DyadicWord, 3U> first{};
  std::array<Binary64DyadicWord, 3U> second{};
};

[[nodiscard]] BoundedExactAnchorPhi bounded_exact_anchor_phi(
    const std::array<std::uint64_t, 3U>& first_bits,
    const std::array<std::uint64_t, 3U>& second_bits) {
  BoundedExactAnchorPhi result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.first[axis] =
        decode_binary64_dyadic_word(first_bits[axis]);
    result.second[axis] =
        decode_binary64_dyadic_word(second_bits[axis]);
  }
  return result;
}

[[nodiscard]] std::optional<std::array<BoundedExactInteger, 12U>>
try_align_bounded_exact_anchor_query(
    const BoundedExactAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  validate_exact_dyadic_box(query_box);
  std::array<Binary64DyadicWord, 12U> words{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    words[axis] = anchor.first[axis];
    words[3U + axis] = anchor.second[axis];
    words[6U + axis] = decode_binary64_dyadic_word(
        query_box.lower_binary64_bits[axis]);
    words[9U + axis] = decode_binary64_dyadic_word(
        query_box.upper_binary64_bits[axis]);
  }
  return try_align_bounded_exact_dyadics(words);
}

[[nodiscard]] std::optional<int>
try_bounded_exact_anchor_phi_aabb_minimum_sign(
    const BoundedExactAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<std::array<BoundedExactInteger, 12U>> aligned =
      try_align_bounded_exact_anchor_query(anchor, query_box);
  if (!aligned.has_value()) {
    return std::nullopt;
  }

  // Four times the exact identity has the same sign and remains integral:
  // outside the midpoint interval it is 4(x-u)(x-v), while at the midpoint
  // it is -(u-v)^2.  This avoids materializing either a rational midpoint or
  // a rational radius.
  BoundedExactInteger four_times_minimum = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const BoundedExactInteger& first = (*aligned)[axis];
    const BoundedExactInteger& second = (*aligned)[3U + axis];
    const BoundedExactInteger& lower = (*aligned)[6U + axis];
    const BoundedExactInteger& upper = (*aligned)[9U + axis];
    const BoundedExactInteger midpoint_numerator = first + second;
    const BoundedExactInteger twice_lower = 2 * lower;
    const BoundedExactInteger twice_upper = 2 * upper;
    if (midpoint_numerator < twice_lower) {
      four_times_minimum +=
          4 * (lower - first) * (lower - second);
    } else if (twice_upper < midpoint_numerator) {
      four_times_minimum +=
          4 * (upper - first) * (upper - second);
    } else {
      const BoundedExactInteger difference = first - second;
      four_times_minimum -= difference * difference;
    }
  }
  return bounded_exact_sign(four_times_minimum);
}

[[nodiscard]] std::optional<int>
try_bounded_exact_diametral_point_phi_aabb_maximum_sign(
    const BoundedExactAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<std::array<BoundedExactInteger, 12U>> aligned =
      try_align_bounded_exact_anchor_query(anchor, query_box);
  if (!aligned.has_value()) {
    return std::nullopt;
  }

  BoundedExactInteger total_maximum = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const BoundedExactInteger& first = (*aligned)[axis];
    const BoundedExactInteger& second = (*aligned)[3U + axis];
    const BoundedExactInteger lower_candidate =
        ((*aligned)[6U + axis] - first) *
        ((*aligned)[6U + axis] - second);
    const BoundedExactInteger upper_candidate =
        ((*aligned)[9U + axis] - first) *
        ((*aligned)[9U + axis] - second);
    total_maximum += std::max(lower_candidate, upper_candidate);
  }
  return bounded_exact_sign(total_maximum);
}

// Binary64 endpoints are dyadic.  Keeping an unnormalized
// significand * 2^exponent form avoids constructing and reducing a rational
// at every difference, product, comparison, and axis sum.  Zero alone has a
// canonical internal exponent; nonzero values deliberately retain their
// operation exponent until the single final ExactRational conversion.
struct UnnormalizedExactDyadic {
  exact::BigInt significand{};
  int exponent{};
};

[[nodiscard]] UnnormalizedExactDyadic make_dyadic(
    exact::BigInt significand,
    int exponent) {
  if (significand == 0) {
    return {};
  }
  return UnnormalizedExactDyadic{std::move(significand), exponent};
}

[[nodiscard]] UnnormalizedExactDyadic dyadic_from_binary64_bits(
    std::uint64_t input_bits) {
  constexpr std::uint64_t fraction_mask =
      (std::uint64_t{1} << 52U) - 1U;
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff);
  constexpr int exponent_bias = 1023;
  constexpr int fraction_bit_count = 52;
  constexpr int subnormal_exponent = -1074;

  const std::uint64_t bits =
      exact::canonicalize_binary64_bits(input_bits);
  const bool negative =
      (bits & exact::binary64_sign_mask) != 0U;
  const std::uint64_t exponent_bits =
      (bits >> 52U) & exponent_mask;
  const std::uint64_t fraction_bits = bits & fraction_mask;
  if (exponent_bits == 0U && fraction_bits == 0U) {
    return {};
  }

  exact::BigInt significand = exponent_bits == 0U
      ? exact::BigInt{fraction_bits}
      : exact::BigInt{
            (std::uint64_t{1} << 52U) | fraction_bits};
  if (negative) {
    significand = -significand;
  }
  const int exponent = exponent_bits == 0U
      ? subnormal_exponent
      : static_cast<int>(exponent_bits) -
            exponent_bias - fraction_bit_count;
  return make_dyadic(std::move(significand), exponent);
}

[[nodiscard]] unsigned int checked_dyadic_shift(
    int exponent,
    int common_exponent) {
  const std::int64_t shift =
      static_cast<std::int64_t>(exponent) -
      static_cast<std::int64_t>(common_exponent);
  if (shift < 0 ||
      static_cast<std::uint64_t>(shift) >
          std::numeric_limits<unsigned int>::max()) {
    throw std::overflow_error(
        "an exact dyadic alignment shift overflows unsigned int");
  }
  return static_cast<unsigned int>(shift);
}

[[nodiscard]] std::array<exact::BigInt, 12U>
exactly_align_continuous_core_endpoint_words(
    const std::array<Binary64DyadicWord, 12U>& words) {
  int minimum_exponent = 0;
  bool exponent_initialized = false;
  for (const Binary64DyadicWord& word : words) {
    if (word.magnitude != 0U &&
        (!exponent_initialized ||
         word.exponent < minimum_exponent)) {
      minimum_exponent = word.exponent;
      exponent_initialized = true;
    }
  }
  std::array<exact::BigInt, 12U> aligned{};
  if (!exponent_initialized) {
    return aligned;
  }
  for (std::size_t index = 0U; index < words.size(); ++index) {
    const Binary64DyadicWord& word = words[index];
    if (word.magnitude == 0U) {
      continue;
    }
    exact::BigInt value{word.magnitude};
    value <<= checked_dyadic_shift(
        word.exponent, minimum_exponent);
    aligned[index] = word.negative ? -value : value;
  }
  return aligned;
}

[[nodiscard]] int checked_dyadic_exponent_add(
    int left,
    int right) {
  const std::int64_t sum =
      static_cast<std::int64_t>(left) +
      static_cast<std::int64_t>(right);
  if (sum < std::numeric_limits<int>::min() ||
      sum > std::numeric_limits<int>::max()) {
    throw std::overflow_error(
        "an exact dyadic exponent addition overflows int");
  }
  return static_cast<int>(sum);
}

[[nodiscard]] exact::BigInt dyadic_significand_at(
    const UnnormalizedExactDyadic& value,
    int common_exponent) {
  exact::BigInt scaled = value.significand;
  scaled <<= checked_dyadic_shift(value.exponent, common_exponent);
  return scaled;
}

[[nodiscard]] bool dyadic_less(
    const UnnormalizedExactDyadic& left,
    const UnnormalizedExactDyadic& right) {
  if (left.significand == 0) {
    return right.significand > 0;
  }
  if (right.significand == 0) {
    return left.significand < 0;
  }
  if ((left.significand < 0) != (right.significand < 0)) {
    return left.significand < 0;
  }
  const int common_exponent =
      std::min(left.exponent, right.exponent);
  return dyadic_significand_at(left, common_exponent) <
      dyadic_significand_at(right, common_exponent);
}

[[nodiscard]] UnnormalizedExactDyadic subtract_dyadics(
    const UnnormalizedExactDyadic& left,
    const UnnormalizedExactDyadic& right) {
  if (right.significand == 0) {
    return left;
  }
  if (left.significand == 0) {
    return make_dyadic(-right.significand, right.exponent);
  }
  const int common_exponent =
      std::min(left.exponent, right.exponent);
  return make_dyadic(
      dyadic_significand_at(left, common_exponent) -
          dyadic_significand_at(right, common_exponent),
      common_exponent);
}

[[nodiscard]] UnnormalizedExactDyadic multiply_dyadics(
    const UnnormalizedExactDyadic& left,
    const UnnormalizedExactDyadic& right) {
  if (left.significand == 0 || right.significand == 0) {
    return {};
  }
  return make_dyadic(
      left.significand * right.significand,
      checked_dyadic_exponent_add(left.exponent, right.exponent));
}

[[nodiscard]] UnnormalizedExactDyadic add_dyadics(
    const UnnormalizedExactDyadic& left,
    const UnnormalizedExactDyadic& right) {
  if (left.significand == 0) {
    return right;
  }
  if (right.significand == 0) {
    return left;
  }
  const int common_exponent =
      std::min(left.exponent, right.exponent);
  return make_dyadic(
      dyadic_significand_at(left, common_exponent) +
          dyadic_significand_at(right, common_exponent),
      common_exponent);
}

[[nodiscard]] exact::ExactRational exact_rational_from_dyadic(
    UnnormalizedExactDyadic value) {
  if (value.significand == 0) {
    return {};
  }
  if (value.exponent >= 0) {
    value.significand <<= checked_dyadic_shift(value.exponent, 0);
    return exact::ExactRational{std::move(value.significand)};
  }
  const std::int64_t denominator_exponent =
      -static_cast<std::int64_t>(value.exponent);
  if (static_cast<std::uint64_t>(denominator_exponent) >
      std::numeric_limits<unsigned int>::max()) {
    throw std::overflow_error(
        "an exact dyadic denominator exponent overflows unsigned int");
  }
  return exact::ExactRational{
      std::move(value.significand),
      exact::power_of_two(
          static_cast<unsigned int>(denominator_exponent))};
}

struct ExactDyadicBoxCoordinates {
  std::array<UnnormalizedExactDyadic, 3> lower{};
  std::array<UnnormalizedExactDyadic, 3> upper{};
};

[[nodiscard]] ExactDyadicBoxCoordinates exact_dyadic_box_coordinates(
    const spatial::ExactDyadicAabb3& box) {
  ExactDyadicBoxCoordinates coordinates;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    coordinates.lower[axis] =
        dyadic_from_binary64_bits(box.lower_binary64_bits[axis]);
    coordinates.upper[axis] =
        dyadic_from_binary64_bits(box.upper_binary64_bits[axis]);
    if (dyadic_less(coordinates.upper[axis], coordinates.lower[axis])) {
      throw std::invalid_argument(
          "an exact dyadic AABB has a reversed axis");
    }
  }
  return coordinates;
}

struct UnnormalizedExactDiametralPhiAabbMaximum {
  UnnormalizedExactDyadic maximum;
  std::array<std::uint8_t, 3> query_endpoint{};
  std::array<std::uint8_t, 3> first_support_endpoint{};
  std::array<std::uint8_t, 3> second_support_endpoint{};
};

[[nodiscard]] UnnormalizedExactDiametralPhiAabbMaximum
exact_diametral_phi_aabb_maximum_dyadic(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactDyadicBoxCoordinates first =
      exact_dyadic_box_coordinates(first_support_box);
  const ExactDyadicBoxCoordinates second =
      exact_dyadic_box_coordinates(second_support_box);
  const ExactDyadicBoxCoordinates query =
      exact_dyadic_box_coordinates(query_box);
  UnnormalizedExactDiametralPhiAabbMaximum result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<const UnnormalizedExactDyadic*, 2>
        first_endpoints{&first.lower[axis], &first.upper[axis]};
    const std::array<const UnnormalizedExactDyadic*, 2>
        second_endpoints{&second.lower[axis], &second.upper[axis]};
    const std::array<const UnnormalizedExactDyadic*, 2>
        query_endpoints{&query.lower[axis], &query.upper[axis]};
    bool initialized = false;
    UnnormalizedExactDyadic axis_maximum;
    for (std::size_t query_selector = 0U;
         query_selector < query_endpoints.size();
         ++query_selector) {
      for (std::size_t first_selector = 0U;
           first_selector < first_endpoints.size();
           ++first_selector) {
        for (std::size_t second_selector = 0U;
             second_selector < second_endpoints.size();
             ++second_selector) {
          UnnormalizedExactDyadic candidate = multiply_dyadics(
              subtract_dyadics(
                  *query_endpoints[query_selector],
                  *first_endpoints[first_selector]),
              subtract_dyadics(
                  *query_endpoints[query_selector],
                  *second_endpoints[second_selector]));
          if (!initialized || dyadic_less(axis_maximum, candidate)) {
            initialized = true;
            axis_maximum = std::move(candidate);
            result.query_endpoint[axis] =
                static_cast<std::uint8_t>(query_selector);
            result.first_support_endpoint[axis] =
                static_cast<std::uint8_t>(first_selector);
            result.second_support_endpoint[axis] =
                static_cast<std::uint8_t>(second_selector);
          }
        }
      }
    }
    if (!initialized) {
      throw std::logic_error(
          "the exact diametral AABB maximum has no endpoint candidate");
    }
    result.maximum = add_dyadics(result.maximum, axis_maximum);
  }
  return result;
}

[[nodiscard]] int dyadic_sign(
    const UnnormalizedExactDyadic& value) noexcept {
  if (value.significand < 0) {
    return -1;
  }
  return value.significand == 0 ? 0 : 1;
}

[[nodiscard]] int exact_diametral_phi_aabb_maximum_dyadic_sign(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<int> bounded_sign =
      try_bounded_exact_diametral_phi_aabb_maximum_sign(
          first_support_box, second_support_box, query_box);
  if (bounded_sign.has_value()) {
    return *bounded_sign;
  }
  return dyadic_sign(
      exact_diametral_phi_aabb_maximum_dyadic(
          first_support_box, second_support_box, query_box)
          .maximum);
}

[[nodiscard]] UnnormalizedExactDyadic halve_dyadic(
    const UnnormalizedExactDyadic& value) {
  if (value.significand == 0) {
    return {};
  }
  return make_dyadic(
      value.significand,
      checked_dyadic_exponent_add(value.exponent, -1));
}

struct ExactDyadicAnchorPhi {
  std::array<UnnormalizedExactDyadic, 3> first{};
  std::array<UnnormalizedExactDyadic, 3> second{};
  std::array<UnnormalizedExactDyadic, 3> vertex{};
  BoundedExactAnchorPhi bounded{};
};

[[nodiscard]] ExactDyadicAnchorPhi exact_dyadic_anchor_phi(
    const exact::CertifiedPoint3& first,
    const exact::CertifiedPoint3& second) {
  const std::array<std::uint64_t, 3> first_bits =
      first.canonical_input_bits();
  const std::array<std::uint64_t, 3> second_bits =
      second.canonical_input_bits();
  ExactDyadicAnchorPhi result;
  result.bounded = bounded_exact_anchor_phi(first_bits, second_bits);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.first[axis] =
        dyadic_from_binary64_bits(first_bits[axis]);
    result.second[axis] =
        dyadic_from_binary64_bits(second_bits[axis]);
    result.vertex[axis] = halve_dyadic(
        add_dyadics(result.first[axis], result.second[axis]));
  }
  return result;
}

// The squared-distance identity is exact:
// ||x-(u+v)/2||^2 - ||u-v||^2/4 = (x-u).(x-v).
// Each axis is a convex quadratic, so clamping its vertex to the AABB gives
// the exact global minimum without constructing a rational center or radius.
[[nodiscard]] UnnormalizedExactDyadic
exact_anchor_phi_aabb_minimum(
    const ExactDyadicAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactDyadicBoxCoordinates query =
      exact_dyadic_box_coordinates(query_box);
  UnnormalizedExactDyadic total_minimum;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic* minimizer = &anchor.vertex[axis];
    if (dyadic_less(anchor.vertex[axis], query.lower[axis])) {
      minimizer = &query.lower[axis];
    } else if (dyadic_less(query.upper[axis], anchor.vertex[axis])) {
      minimizer = &query.upper[axis];
    }
    total_minimum = add_dyadics(
        total_minimum,
        multiply_dyadics(
            subtract_dyadics(*minimizer, anchor.first[axis]),
            subtract_dyadics(*minimizer, anchor.second[axis])));
  }
  return total_minimum;
}

[[nodiscard]] int exact_anchor_phi_aabb_minimum_sign(
    const ExactDyadicAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<int> bounded_sign =
      try_bounded_exact_anchor_phi_aabb_minimum_sign(
          anchor.bounded, query_box);
  if (bounded_sign.has_value()) {
    return *bounded_sign;
  }
  return dyadic_sign(exact_anchor_phi_aabb_minimum(anchor, query_box));
}

[[nodiscard]] UnnormalizedExactDyadic
exact_diametral_point_phi_aabb_maximum(
    const ExactDyadicAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactDyadicBoxCoordinates query =
      exact_dyadic_box_coordinates(query_box);
  UnnormalizedExactDyadic total_maximum;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic lower_candidate =
        multiply_dyadics(
            subtract_dyadics(query.lower[axis], anchor.first[axis]),
            subtract_dyadics(query.lower[axis], anchor.second[axis]));
    const UnnormalizedExactDyadic upper_candidate =
        multiply_dyadics(
            subtract_dyadics(query.upper[axis], anchor.first[axis]),
            subtract_dyadics(query.upper[axis], anchor.second[axis]));
    total_maximum = add_dyadics(
        total_maximum,
        dyadic_less(lower_candidate, upper_candidate)
            ? upper_candidate
            : lower_candidate);
  }
  return total_maximum;
}

[[nodiscard]] int exact_diametral_point_phi_aabb_maximum_sign(
    const ExactDyadicAnchorPhi& anchor,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<int> bounded_sign =
      try_bounded_exact_diametral_point_phi_aabb_maximum_sign(
          anchor.bounded, query_box);
  if (bounded_sign.has_value()) {
    return *bounded_sign;
  }
  return dyadic_sign(
      exact_diametral_point_phi_aabb_maximum(anchor, query_box));
}

// P7a works with doubled centres s=u+v.  This keeps every endpoint dyadic and
// turns the strict-ball implication into the division-free inequality
//
//   ||2x-s||^2 < L(C) <= ||u-v||^2,
//
// where L(C) combines the support separation with the distances from C to
// both doubled support boxes.
//
// Hence x is strictly interior to every diametral ball whose doubled
// centre lies in the cell.  Cells may use different Morton antichains.
struct ExactDoubledCenterCell {
  std::array<UnnormalizedExactDyadic, 3U> lower{};
  std::array<UnnormalizedExactDyadic, 3U> upper{};
};

struct ExactCenterCoverCellCertificate {
  ExactDoubledCenterCell cell{};
  std::vector<ExactPairSupportWitnessNodeEntry> witness_receipts;
  std::size_t witness_point_count{};
};

struct ExactCenterCoverAttemptAudit {
  std::size_t cell_visit_count{};
  std::size_t cell_split_count{};
  std::size_t certified_cell_count{};
  std::size_t witness_node_visit_count{};
  std::size_t strict_witness_subtree_count{};
  std::size_t strict_witness_point_count{};
};

struct ExactCenterCoverAttemptResult {
  bool rank_pruned{false};
  ExactCenterCoverAttemptAudit audit{};
  std::vector<ExactCenterCoverCellCertificate> certificate_cells;
};

[[nodiscard]] bool dyadic_equal(
    const UnnormalizedExactDyadic& left,
    const UnnormalizedExactDyadic& right) {
  return !dyadic_less(left, right) && !dyadic_less(right, left);
}

[[nodiscard]] UnnormalizedExactDyadic square_dyadic(
    const UnnormalizedExactDyadic& value) {
  return multiply_dyadics(value, value);
}

[[nodiscard]] UnnormalizedExactDyadic doubled_dyadic(
    const UnnormalizedExactDyadic& value) {
  return add_dyadics(value, value);
}

[[nodiscard]] ExactDoubledCenterCell make_doubled_center_cell(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box) {
  const ExactDyadicBoxCoordinates first =
      exact_dyadic_box_coordinates(first_support_box);
  const ExactDyadicBoxCoordinates second =
      exact_dyadic_box_coordinates(second_support_box);
  ExactDoubledCenterCell cell;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    cell.lower[axis] =
        add_dyadics(first.lower[axis], second.lower[axis]);
    cell.upper[axis] =
        add_dyadics(first.upper[axis], second.upper[axis]);
  }
  return cell;
}

[[nodiscard]] UnnormalizedExactDyadic
minimum_support_squared_distance(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box) {
  const ExactDyadicBoxCoordinates first =
      exact_dyadic_box_coordinates(first_support_box);
  const ExactDyadicBoxCoordinates second =
      exact_dyadic_box_coordinates(second_support_box);
  UnnormalizedExactDyadic result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    UnnormalizedExactDyadic gap;
    if (dyadic_less(first.upper[axis], second.lower[axis])) {
      gap = subtract_dyadics(
          second.lower[axis], first.upper[axis]);
    } else if (dyadic_less(second.upper[axis], first.lower[axis])) {
      gap = subtract_dyadics(
          first.lower[axis], second.upper[axis]);
    }
    result = add_dyadics(result, square_dyadic(gap));
  }
  return result;
}

[[nodiscard]] UnnormalizedExactDyadic
minimum_doubled_center_support_squared_distance(
    const ExactDoubledCenterCell& cell,
    const spatial::ExactDyadicAabb3& support_box) {
  const ExactDyadicBoxCoordinates support =
      exact_dyadic_box_coordinates(support_box);
  UnnormalizedExactDyadic result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic doubled_support_lower =
        doubled_dyadic(support.lower[axis]);
    const UnnormalizedExactDyadic doubled_support_upper =
        doubled_dyadic(support.upper[axis]);
    UnnormalizedExactDyadic gap;
    if (dyadic_less(cell.upper[axis], doubled_support_lower)) {
      gap = subtract_dyadics(
          doubled_support_lower, cell.upper[axis]);
    } else if (dyadic_less(
                   doubled_support_upper, cell.lower[axis])) {
      gap = subtract_dyadics(
          cell.lower[axis], doubled_support_upper);
    }
    result = add_dyadics(result, square_dyadic(gap));
  }
  return result;
}

// For an actual support pair u in A, v in B whose doubled centre s=u+v
// belongs to C,
//
//   ||u-v||^2 = ||s-2u||^2 = ||s-2v||^2.
//
// Each box distance below is therefore a lower bound for that pair's
// squared chord.  Taking their maximum strengthens the radius without
// weakening the strict certificate.
[[nodiscard]] UnnormalizedExactDyadic
doubled_center_cell_chord_squared_lower_bound(
    const ExactDoubledCenterCell& cell,
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const UnnormalizedExactDyadic&
        minimum_support_pair_squared_distance) {
  UnnormalizedExactDyadic result =
      minimum_support_pair_squared_distance;
  const UnnormalizedExactDyadic first_bound =
      minimum_doubled_center_support_squared_distance(
          cell, first_support_box);
  if (dyadic_less(result, first_bound)) {
    result = first_bound;
  }
  const UnnormalizedExactDyadic second_bound =
      minimum_doubled_center_support_squared_distance(
          cell, second_support_box);
  if (dyadic_less(result, second_bound)) {
    result = second_bound;
  }
  return result;
}

[[nodiscard]] UnnormalizedExactDyadic
minimum_doubled_center_query_squared_distance(
    const ExactDoubledCenterCell& cell,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactDyadicBoxCoordinates query =
      exact_dyadic_box_coordinates(query_box);
  UnnormalizedExactDyadic result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic midpoint = halve_dyadic(
        add_dyadics(cell.lower[axis], cell.upper[axis]));
    const UnnormalizedExactDyadic doubled_query_lower =
        doubled_dyadic(query.lower[axis]);
    const UnnormalizedExactDyadic doubled_query_upper =
        doubled_dyadic(query.upper[axis]);
    const UnnormalizedExactDyadic* minimizer = &midpoint;
    if (dyadic_less(midpoint, doubled_query_lower)) {
      minimizer = &doubled_query_lower;
    } else if (dyadic_less(doubled_query_upper, midpoint)) {
      minimizer = &doubled_query_upper;
    }
    const UnnormalizedExactDyadic lower_candidate = square_dyadic(
        subtract_dyadics(*minimizer, cell.lower[axis]));
    const UnnormalizedExactDyadic upper_candidate = square_dyadic(
        subtract_dyadics(*minimizer, cell.upper[axis]));
    result = add_dyadics(
        result,
        dyadic_less(lower_candidate, upper_candidate)
            ? upper_candidate
            : lower_candidate);
  }
  return result;
}

[[nodiscard]] UnnormalizedExactDyadic
minimum_doubled_center_core_squared_distance(
    const ExactDoubledCenterCell& cell) {
  UnnormalizedExactDyadic result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic width =
        subtract_dyadics(cell.upper[axis], cell.lower[axis]);
    if (width.significand < 0) {
      throw std::logic_error(
          "a doubled-center cell has a reversed axis");
    }
    result = add_dyadics(
        result, square_dyadic(halve_dyadic(width)));
  }
  return result;
}

[[nodiscard]] UnnormalizedExactDyadic
maximum_doubled_center_query_squared_distance(
    const ExactDoubledCenterCell& cell,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactDyadicBoxCoordinates query =
      exact_dyadic_box_coordinates(query_box);
  UnnormalizedExactDyadic result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<const UnnormalizedExactDyadic*, 2U>
        center_endpoints{&cell.lower[axis], &cell.upper[axis]};
    const std::array<const UnnormalizedExactDyadic*, 2U>
        query_endpoints{&query.lower[axis], &query.upper[axis]};
    bool initialized = false;
    UnnormalizedExactDyadic maximum;
    for (const UnnormalizedExactDyadic* center : center_endpoints) {
      for (const UnnormalizedExactDyadic* query_endpoint :
           query_endpoints) {
        const UnnormalizedExactDyadic delta = subtract_dyadics(
            doubled_dyadic(*query_endpoint), *center);
        const UnnormalizedExactDyadic candidate =
            square_dyadic(delta);
        if (!initialized || dyadic_less(maximum, candidate)) {
          maximum = candidate;
          initialized = true;
        }
      }
    }
    if (!initialized) {
      throw std::logic_error(
          "a doubled-center query maximum has no endpoint");
    }
    result = add_dyadics(result, maximum);
  }
  return result;
}

[[nodiscard]] bool query_box_strictly_inside_every_cell_ball(
    const ExactDoubledCenterCell& cell,
    const UnnormalizedExactDyadic& minimum_squared_distance,
    const spatial::ExactDyadicAabb3& query_box) {
  return dyadic_less(
      maximum_doubled_center_query_squared_distance(cell, query_box),
      minimum_squared_distance);
}

[[nodiscard]] bool split_doubled_center_cell(
    const ExactDoubledCenterCell& parent,
    ExactDoubledCenterCell& first,
    ExactDoubledCenterCell& second) {
  std::size_t split_axis = 3U;
  UnnormalizedExactDyadic largest_width;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const UnnormalizedExactDyadic width =
        subtract_dyadics(parent.upper[axis], parent.lower[axis]);
    if (width.significand < 0) {
      throw std::logic_error(
          "a doubled-center cell has a reversed axis");
    }
    if (width.significand != 0 &&
        (split_axis == 3U || dyadic_less(largest_width, width))) {
      split_axis = axis;
      largest_width = width;
    }
  }
  if (split_axis == 3U) {
    return false;
  }
  const UnnormalizedExactDyadic midpoint = halve_dyadic(
      add_dyadics(
          parent.lower[split_axis], parent.upper[split_axis]));
  if (dyadic_equal(midpoint, parent.lower[split_axis]) ||
      dyadic_equal(midpoint, parent.upper[split_axis])) {
    throw std::logic_error(
        "a nondegenerate exact doubled-center cell did not bisect");
  }
  first = parent;
  second = parent;
  first.upper[split_axis] = midpoint;
  second.lower[split_axis] = midpoint;
  return true;
}

[[nodiscard]] bool event_less(
    const ExactPairSupportEvent& left,
    const ExactPairSupportEvent& right) {
  return left.support_ids < right.support_ids;
}

[[nodiscard]] bool diagnostic_less(
    const ExactPairSupportExtraShellDiagnostic& left,
    const ExactPairSupportExtraShellDiagnostic& right) {
  return left.support_ids < right.support_ids;
}

[[nodiscard]] bool frontier_entry_less(
    const ExactPairSupportFrontierEntry& left,
    const ExactPairSupportFrontierEntry& right) noexcept {
  if (left.first_leaf_begin != right.first_leaf_begin) {
    return left.first_leaf_begin < right.first_leaf_begin;
  }
  if (left.first_leaf_end != right.first_leaf_end) {
    return left.first_leaf_end < right.first_leaf_end;
  }
  if (left.second_leaf_begin != right.second_leaf_begin) {
    return left.second_leaf_begin < right.second_leaf_begin;
  }
  if (left.second_leaf_end != right.second_leaf_end) {
    return left.second_leaf_end < right.second_leaf_end;
  }
  if (left.first_node_index != right.first_node_index) {
    return left.first_node_index < right.first_node_index;
  }
  if (left.second_node_index != right.second_node_index) {
    return left.second_node_index < right.second_node_index;
  }
  return left.self_product < right.self_product;
}

[[nodiscard]] bool witness_entry_less(
    const ExactPairSupportWitnessNodeEntry& left,
    const ExactPairSupportWitnessNodeEntry& right) noexcept {
  if (left.leaf_begin != right.leaf_begin) {
    return left.leaf_begin < right.leaf_begin;
  }
  if (left.leaf_end != right.leaf_end) {
    return left.leaf_end < right.leaf_end;
  }
  return left.node_index < right.node_index;
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) {
    text(domain);
  }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) {
    byte(static_cast<std::uint8_t>(value ? 1U : 0U));
  }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(value, "a canonical digest size does not fit uint64"));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void identifier(const contract::CanonicalId& identifier) {
    builder_.update(identifier.bytes());
  }

  void rational(const exact::ExactRational& value) {
    text(value.canonical_key());
  }

  void center(const exact::ExactCenter3& value) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      rational(value.coordinate(axis));
    }
  }

  void level(const exact::ExactLevel& value) {
    text(value.canonical_key());
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

void append_frontier_entry(
    DigestWriter& writer,
    const ExactPairSupportFrontierEntry& entry) {
  writer.u64(entry.first_node_index);
  writer.u64(entry.second_node_index);
  writer.u64(entry.first_leaf_begin);
  writer.u64(entry.first_leaf_end);
  writer.u64(entry.second_leaf_begin);
  writer.u64(entry.second_leaf_end);
  writer.byte(entry.self_product);
}

void append_witness_node_entry(
    DigestWriter& writer,
    const ExactPairSupportWitnessNodeEntry& entry) {
  writer.u64(entry.node_index);
  writer.u64(entry.leaf_begin);
  writer.u64(entry.leaf_end);
}

void append_audit(
    DigestWriter& writer,
    const ExactPairSupportStreamAudit& audit) {
#define MORSEHGP3D_APPEND_AUDIT_SIZE(field) writer.size(audit.field)
  MORSEHGP3D_APPEND_AUDIT_SIZE(total_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(work_unit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(support_product_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(support_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(self_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(cross_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(diagonal_leaf_discard_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(diagonal_product_rank_search_skip_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_prune_search_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(witness_node_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_phi_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_anchor_ball_minimum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_noninterior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_noninterior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_shell_tangent_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(equality_or_positive_bound_descent_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(strict_interior_witness_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(strict_interior_witness_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_work_preflight_skip_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_attempt_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_inconclusive_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_pruned_product_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_work_unit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_cell_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_cell_split_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_certified_cell_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_witness_node_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_strict_witness_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(center_cover_strict_witness_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_pruned_product_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_pruned_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(leaf_pair_classification_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(global_closed_ball_query_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(point_classification_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_node_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_closed_ball_minimum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_closed_ball_maximum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_interior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_interior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_exterior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_exterior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(early_closed_rank_rejection_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_point_distance_evaluation_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(accepted_event_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(relevant_extra_shell_diagnostic_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(emitted_point_id_reference_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(above_rank_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_witness_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_closed_ball_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(remaining_frontier_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(resolved_pair_count);
#undef MORSEHGP3D_APPEND_AUDIT_SIZE
  writer.boolean(audit.pair_partition_accounting_certified);
}

[[nodiscard]] contract::CanonicalId initial_output_chain_digest() {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/empty"};
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactPairSupportEvent& event) {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/event"};
  writer.identifier(previous);
  for (const PointId point_id : event.support_ids) {
    writer.u64(point_id);
  }
  writer.center(event.center);
  writer.level(event.squared_level);
  writer.size(event.interior_ids.size());
  for (const PointId point_id : event.interior_ids) {
    writer.u64(point_id);
  }
  writer.size(event.closed_rank);
  writer.size(event.exterior_count);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactPairSupportExtraShellDiagnostic& diagnostic) {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/extra-shell"};
  writer.identifier(previous);
  for (const PointId point_id : diagnostic.support_ids) {
    writer.u64(point_id);
  }
  writer.center(diagnostic.center);
  writer.level(diagnostic.squared_level);
  writer.size(diagnostic.interior_ids.size());
  for (const PointId point_id : diagnostic.interior_ids) {
    writer.u64(point_id);
  }
  writer.size(diagnostic.shell_count);
  writer.u64(diagnostic.canonical_extra_shell_witness_id);
  writer.size(diagnostic.minimum_possible_closed_rank);
  writer.size(diagnostic.observed_closed_rank);
  writer.size(diagnostic.exterior_count);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const ExactPairSupportCheckpoint& checkpoint) {
  DigestWriter writer{
      "MorseHGP3D/phase14q/pair-support/checkpoint/v2"};
  const ExactPairSupportCheckpointManifest& manifest = checkpoint.manifest;
  writer.u32(manifest.schema_version);
  writer.u32(manifest.traversal_version);
  writer.size(manifest.point_count);
  writer.size(manifest.lbvh_node_count);
  writer.size(manifest.lbvh_leaf_count);
  writer.size(manifest.requested_maximum_order);
  writer.size(manifest.effective_maximum_order);
  writer.size(manifest.maximum_relevant_closed_rank);
  writer.identifier(manifest.canonical_cloud_digest);
  writer.identifier(manifest.lbvh_digest);
  writer.identifier(manifest.semantic_digest);
  writer.u64(checkpoint.next_chunk_sequence);
  writer.size(checkpoint.output_record_count);
  writer.identifier(checkpoint.output_chain_digest);
  writer.size(checkpoint.frontier.size());
  for (const ExactPairSupportFrontierEntry& entry : checkpoint.frontier) {
    append_frontier_entry(writer, entry);
  }
  writer.boolean(checkpoint.pending_product.has_value());
  if (checkpoint.pending_product.has_value()) {
    const ExactPairSupportPendingProduct& pending =
        *checkpoint.pending_product;
    append_frontier_entry(writer, pending.product);
    writer.byte(static_cast<std::uint8_t>(pending.stage));
    writer.boolean(pending.rank_search_started);
    writer.size(pending.witness_frontier.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.witness_frontier) {
      append_witness_node_entry(writer, entry);
    }
    writer.size(pending.strict_witness_receipts.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.strict_witness_receipts) {
      append_witness_node_entry(writer, entry);
    }
    writer.boolean(pending.deferred_expansion_node.has_value());
    if (pending.deferred_expansion_node.has_value()) {
      append_witness_node_entry(
          writer, *pending.deferred_expansion_node);
    }
    writer.size(pending.strict_witness_point_count);
    writer.boolean(pending.closed_ball.has_value());
    if (pending.closed_ball.has_value()) {
      const ExactPairSupportPendingClosedBall& closed_ball =
          *pending.closed_ball;
      writer.byte(static_cast<std::uint8_t>(closed_ball.stage));
      writer.size(closed_ball.node_visit_count);
      writer.size(closed_ball.frontier.size());
      for (const ExactPairSupportWitnessNodeEntry& entry :
           closed_ball.frontier) {
        append_witness_node_entry(writer, entry);
      }
      writer.size(closed_ball.interior_ids.size());
      for (const PointId point_id : closed_ball.interior_ids) {
        writer.u64(point_id);
      }
      writer.size(closed_ball.shell_count);
      writer.boolean(
          closed_ball.canonical_extra_shell_witness_id.has_value());
      if (closed_ball.canonical_extra_shell_witness_id.has_value()) {
        writer.u64(
            *closed_ball.canonical_extra_shell_witness_id);
      }
      writer.size(closed_ball.exterior_count);
      writer.byte(closed_ball.support_seen_mask);
    }
  }
  append_audit(writer, checkpoint.cumulative_audit);
  return writer.finalize();
}

}  // namespace

ExactDiametralPhiAabbMaximum exact_diametral_phi_aabb_maximum(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  UnnormalizedExactDiametralPhiAabbMaximum dyadic =
      exact_diametral_phi_aabb_maximum_dyadic(
          first_support_box, second_support_box, query_box);
  ExactDiametralPhiAabbMaximum result;
  result.query_endpoint = dyadic.query_endpoint;
  result.first_support_endpoint = dyadic.first_support_endpoint;
  result.second_support_endpoint = dyadic.second_support_endpoint;
  result.maximum_phi =
      exact_rational_from_dyadic(std::move(dyadic.maximum));
  return result;
}

int exact_diametral_phi_aabb_maximum_sign(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  return exact_diametral_phi_aabb_maximum_dyadic_sign(
      first_support_box, second_support_box, query_box);
}

int exact_diametral_phi_continuous_core_minimum_sign(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box) {
  const std::optional<int> bounded_sign =
      try_bounded_exact_diametral_phi_continuous_core_minimum_sign(
          first_support_box, second_support_box);
  if (bounded_sign.has_value()) {
    return *bounded_sign;
  }
  const std::array<Binary64DyadicWord, 12U> words =
      continuous_core_endpoint_words(
          first_support_box, second_support_box);
  return continuous_core_minimum_sign_from_aligned_endpoints(
      exactly_align_continuous_core_endpoint_words(words));
}

int exact_diametral_anchor_phi_aabb_minimum_sign(
    const spatial::ExactDyadicAabb3& first_support_point_box,
    const spatial::ExactDyadicAabb3& second_support_point_box,
    const spatial::ExactDyadicAabb3& query_box) {
  validate_exact_dyadic_box(first_support_point_box);
  validate_exact_dyadic_box(second_support_point_box);
  validate_exact_dyadic_box(query_box);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (exact::canonicalize_binary64_bits(
            first_support_point_box.lower_binary64_bits[axis]) !=
            exact::canonicalize_binary64_bits(
                first_support_point_box.upper_binary64_bits[axis]) ||
        exact::canonicalize_binary64_bits(
            second_support_point_box.lower_binary64_bits[axis]) !=
            exact::canonicalize_binary64_bits(
                second_support_point_box.upper_binary64_bits[axis])) {
      throw std::invalid_argument(
          "an exact diametral anchor support box is not degenerate");
    }
  }
  const BoundedExactAnchorPhi bounded_anchor =
      bounded_exact_anchor_phi(
          first_support_point_box.lower_binary64_bits,
          second_support_point_box.lower_binary64_bits);
  const std::optional<int> bounded_sign =
      try_bounded_exact_anchor_phi_aabb_minimum_sign(
          bounded_anchor, query_box);
  if (bounded_sign.has_value()) {
    return *bounded_sign;
  }

  const ExactDyadicBoxCoordinates first =
      exact_dyadic_box_coordinates(first_support_point_box);
  const ExactDyadicBoxCoordinates second =
      exact_dyadic_box_coordinates(second_support_point_box);
  ExactDyadicAnchorPhi anchor;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (dyadic_less(first.lower[axis], first.upper[axis]) ||
        dyadic_less(second.lower[axis], second.upper[axis])) {
      throw std::invalid_argument(
          "an exact diametral anchor support box is not degenerate");
    }
    anchor.first[axis] = first.lower[axis];
    anchor.second[axis] = second.lower[axis];
    anchor.vertex[axis] = halve_dyadic(
        add_dyadics(anchor.first[axis], anchor.second[axis]));
  }
  return dyadic_sign(
      exact_anchor_phi_aabb_minimum(anchor, query_box));
}

class ExactPairSupportStreamBuilder {
 public:
  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget)
      : index_(index),
        cloud_(cloud),
        output_chain_digest_(initial_output_chain_digest()),
        canonical_sort_records_(true) {
    validate_inputs(requested_maximum_order, budget);
    result_.requirements = requirements_for(requested_maximum_order);
    result_.budget = budget;
    result_.audit.total_pair_count =
        checked_unordered_pair_count(cloud_.size());
    if (result_.audit.total_pair_count != 0U) {
      if (budget.maximum_frontier_entry_count == 0U) {
        throw std::invalid_argument(
            "a nonempty pair frontier requires at least one budgeted entry");
      }
      frontier_.push_back(make_entry(index_.root_index_, index_.root_index_));
      result_.audit.maximum_frontier_entry_count = 1U;
    }
    chunk_audit_origin_ = result_.audit;
  }

  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget,
      const ExactPairSupportCheckpoint& checkpoint,
      IntegrityVerifiedCheckpointTag)
      : index_(index),
        cloud_(cloud),
        output_chain_digest_(checkpoint.output_chain_digest),
        output_record_count_(checkpoint.output_record_count),
        canonical_sort_records_(false) {
    validate_inputs(requested_maximum_order, budget);
    result_.requirements = requirements_for(requested_maximum_order);
    result_.budget = budget;
    result_.audit = checkpoint.cumulative_audit;
    frontier_ = checkpoint.frontier;
    pending_product_ = checkpoint.pending_product;
    if (!frontier_.empty() &&
        budget.maximum_frontier_entry_count < frontier_.size()) {
      throw std::invalid_argument(
          "the chunk frontier capacity is below the persisted checkpoint frontier");
    }
    if (pending_product_.has_value()) {
      std::size_t persisted_auxiliary_entry_count = checked_add(
          pending_product_->witness_frontier.size(),
          pending_product_->deferred_expansion_node.has_value() ? 1U : 0U,
          "the persisted auxiliary frontier count overflows size_t");
      if (pending_product_->closed_ball.has_value()) {
        persisted_auxiliary_entry_count = checked_add(
            persisted_auxiliary_entry_count,
            pending_product_->closed_ball->frontier.size(),
            "the persisted closed-ball frontier count overflows size_t");
      }
      if (budget.maximum_auxiliary_frontier_entry_count <
          persisted_auxiliary_entry_count) {
        throw std::invalid_argument(
            "the chunk auxiliary-frontier capacity is below the persisted checkpoint cursor");
      }
    }
    chunk_audit_origin_ = checkpoint.cumulative_audit;
    result_.audit.remaining_frontier_pair_count = 0U;
    result_.audit.pair_partition_accounting_certified = false;
  }

  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget,
      const ExactPairSupportCheckpoint& checkpoint,
      IntegrityVerifiedCheckpointTag integrity_verified,
      std::size_t maximum_products_per_batch,
      const ExactPairSupportRankPruneBatchCallback& proposal_callback)
      : ExactPairSupportStreamBuilder(
            index,
            cloud,
            requested_maximum_order,
            budget,
            checkpoint,
            integrity_verified) {
    if (maximum_products_per_batch == 0U) {
      throw std::invalid_argument(
          "a pair-support proposal batch must contain at least one product");
    }
    if (!proposal_callback) {
      throw std::invalid_argument(
          "a pair-support proposal build requires a batch callback");
    }
    maximum_products_per_batch_ = maximum_products_per_batch;
    proposal_callback_ = &proposal_callback;
    proposal_audit_.maximum_products_per_batch =
        maximum_products_per_batch;
  }

  void execute() {
    while ((!frontier_.empty() || pending_product_.has_value()) && !stopped_) {
      if (pending_product_.has_value()) {
        continue_pending_product();
      } else {
        visit_frontier_back();
      }
    }
    finish_result();
  }

  [[nodiscard]] ExactPairSupportStreamResult take_result() {
    return std::move(result_);
  }

  [[nodiscard]] const contract::CanonicalId& output_chain_digest()
      const noexcept {
    return output_chain_digest_;
  }

  [[nodiscard]] std::vector<ExactPairSupportStreamChunk::RecordKind>
  take_record_order() {
    return std::move(record_order_);
  }

  [[nodiscard]] std::vector<ExactPairSupportRankPruneProposal>
  take_consumed_rank_prune_proposals() {
    proposal_batch_products_.clear();
    proposal_cache_.clear();
    keep_certificate_cache_.clear();
    proposal_batch_active_ = false;
    return std::move(consumed_rank_prune_proposals_);
  }

  [[nodiscard]] std::vector<ExactPairSupportRankKeepCertificate>
  take_consumed_rank_keep_certificates() {
    return std::move(consumed_rank_keep_certificates_);
  }

  [[nodiscard]] ExactPairSupportRankPruneProposalAudit
  rank_prune_proposal_audit() const noexcept {
    return proposal_audit_;
  }

  [[nodiscard]] ExactPairSupportCheckpoint snapshot_checkpoint(
      const ExactPairSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) const {
    ExactPairSupportCheckpoint checkpoint;
    checkpoint.manifest = manifest;
    checkpoint.next_chunk_sequence = next_chunk_sequence;
    checkpoint.output_record_count = output_record_count_;
    checkpoint.output_chain_digest = output_chain_digest_;
    checkpoint.frontier = frontier_;
    checkpoint.pending_product = pending_product_;
    checkpoint.cumulative_audit = audit_snapshot();
    checkpoint.checkpoint_digest = checkpoint_digest(checkpoint);
    return checkpoint;
  }

  [[nodiscard]] ExactPairSupportCheckpoint take_checkpoint(
      const ExactPairSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) {
    ExactPairSupportCheckpoint checkpoint;
    checkpoint.manifest = manifest;
    checkpoint.next_chunk_sequence = next_chunk_sequence;
    checkpoint.output_record_count = output_record_count_;
    checkpoint.output_chain_digest = output_chain_digest_;
    checkpoint.cumulative_audit = audit_snapshot();
    checkpoint.frontier = std::move(frontier_);
    checkpoint.pending_product = std::move(pending_product_);
    checkpoint.checkpoint_digest = checkpoint_digest(checkpoint);
    return checkpoint;
  }

  [[nodiscard]] static ExactPairSupportCheckpointManifest manifest_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order) {
    return manifest_for_with_audit(
        index, cloud, requested_maximum_order, nullptr);
  }

  [[nodiscard]] static ExactPairSupportCheckpointManifest
  manifest_for_with_audit(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportAuthorityContextAudit* audit) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "a pair-support manifest requires the supplied cloud's Morton LBVH");
    }
    if (cloud.size() == 0U || requested_maximum_order == 0U ||
        requested_maximum_order > pair_support_maximum_requested_order) {
      throw std::invalid_argument(
          "a pair-support manifest requires a nonempty cloud and 1<=Kmax<=10");
    }
    if (audit != nullptr) {
      audit->manifest_build_count = checked_add(
          audit->manifest_build_count,
          1U,
          "the pair-support manifest build count overflows size_t");
      audit->manifest_cached = false;
    }
    ExactPairSupportCheckpointManifest manifest;
    manifest.point_count = cloud.size();
    manifest.lbvh_node_count = index.nodes_.size();
    manifest.lbvh_leaf_count = index.leaves_.size();
    manifest.requested_maximum_order = requested_maximum_order;
    manifest.effective_maximum_order =
        std::min(requested_maximum_order, cloud.size());
    manifest.maximum_relevant_closed_rank = std::min(
        checked_add(
            manifest.effective_maximum_order,
            1U,
            "the pair-support manifest rank overflows size_t"),
        cloud.size());

    DigestWriter cloud_writer{
        "MorseHGP3D/phase9/pair-support/canonical-cloud/v1"};
    cloud_writer.size(cloud.size());
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      const PointId point_id = checked_u64(
          point_index, "a canonical point index does not fit PointId");
      const std::array<std::uint64_t, 3> bits =
          cloud.point(point_id).canonical_input_bits();
      for (const std::uint64_t word : bits) {
        cloud_writer.u64(word);
      }
      if (audit != nullptr) {
        audit->canonical_cloud_point_hash_count = checked_add(
            audit->canonical_cloud_point_hash_count,
            1U,
            "the pair-support manifest point hash count overflows size_t");
      }
    }
    manifest.canonical_cloud_digest = cloud_writer.finalize();

    DigestWriter lbvh_writer{
        "MorseHGP3D/phase9/pair-support/morton-lbvh/v1"};
    lbvh_writer.size(spatial::MortonLbvhIndex::morton_bits_per_axis);
    lbvh_writer.size(index.point_count_);
    lbvh_writer.size(index.nodes_.size());
    lbvh_writer.size(index.leaves_.size());
    lbvh_writer.size(index.root_index_);
    lbvh_writer.size(index.build_counters_.point_count);
    lbvh_writer.size(index.build_counters_.node_count);
    lbvh_writer.size(index.build_counters_.maximum_depth);
    lbvh_writer.size(index.build_counters_.morton_collision_group_count);
    lbvh_writer.size(index.build_counters_.maximum_morton_collision_size);
    for (const std::uint64_t word : index.root_aabb_.lower_binary64_bits) {
      lbvh_writer.u64(word);
    }
    for (const std::uint64_t word : index.root_aabb_.upper_binary64_bits) {
      lbvh_writer.u64(word);
    }
    for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
      lbvh_writer.u64(leaf.morton_code);
      lbvh_writer.u64(leaf.point_id);
      if (audit != nullptr) {
        audit->lbvh_leaf_hash_count = checked_add(
            audit->lbvh_leaf_hash_count,
            1U,
            "the pair-support manifest leaf hash count overflows size_t");
      }
    }
    for (const Node& current : index.nodes_) {
      for (const PointId point_id : current.lower_point_ids) {
        lbvh_writer.u64(point_id);
      }
      for (const PointId point_id : current.upper_point_ids) {
        lbvh_writer.u64(point_id);
      }
      if (current.is_leaf()) {
        lbvh_writer.u64(std::numeric_limits<std::uint64_t>::max());
        lbvh_writer.u64(std::numeric_limits<std::uint64_t>::max());
      } else {
        lbvh_writer.size(current.left_child);
        lbvh_writer.size(current.right_child);
      }
      lbvh_writer.size(current.leaf_begin);
      lbvh_writer.size(current.leaf_end);
      if (audit != nullptr) {
        audit->lbvh_node_hash_count = checked_add(
            audit->lbvh_node_hash_count,
            1U,
            "the pair-support manifest node hash count overflows size_t");
      }
    }
    manifest.lbvh_digest = lbvh_writer.finalize();

    DigestWriter semantic_writer{
        "MorseHGP3D/phase14q/pair-support/checkpoint-manifest/v2"};
    semantic_writer.u32(manifest.schema_version);
    semantic_writer.u32(manifest.traversal_version);
    semantic_writer.text(pair_support_checkpoint_proof_basis);
    semantic_writer.text("reference_cpu");
    semantic_writer.text("hgp_reduced");
    semantic_writer.text("certified");
    semantic_writer.size(2U);
    semantic_writer.size(manifest.point_count);
    semantic_writer.size(manifest.lbvh_node_count);
    semantic_writer.size(manifest.lbvh_leaf_count);
    semantic_writer.size(manifest.requested_maximum_order);
    semantic_writer.size(manifest.effective_maximum_order);
    semantic_writer.size(manifest.maximum_relevant_closed_rank);
    semantic_writer.identifier(manifest.canonical_cloud_digest);
    semantic_writer.identifier(manifest.lbvh_digest);
    manifest.semantic_digest = semantic_writer.finalize();
    if (audit != nullptr) {
      audit->manifest_cached = true;
    }
    return manifest;
  }

  [[nodiscard]] static ExactPairSupportCheckpointVerification
  verify_checkpoint_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      const ExactPairSupportCheckpointManifest& authority_manifest,
      const ExactPairSupportCheckpoint& checkpoint) {
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    ExactPairSupportStreamBuilder validator{
        index,
        cloud,
        requested_maximum_order,
        ExactPairSupportStreamBudget{
            maximum,
            maximum,
            maximum,
            maximum,
            maximum,
            maximum,
            maximum}};
    return validator.checkpoint_verification(
        authority_manifest, checkpoint);
  }

 private:
  using Node = spatial::MortonLbvhIndex::Node;

  enum class RankSearchOutcome : std::uint8_t {
    keep,
    prune,
    budget_exhausted,
  };

  enum class SparseBallOutcome : std::uint8_t {
    complete,
    rank_exceeded,
    budget_exhausted,
  };

  struct ProductRectangle {
    std::uint64_t x_begin{};
    std::uint64_t x_end{};
    std::uint64_t y_begin{};
    std::uint64_t y_end{};
  };

  struct ProductRectangleSweepEvent {
    std::uint64_t x{};
    std::size_t rectangle_index{};
    bool is_start{false};
  };

  struct ActiveProductRectangle {
    std::uint64_t y_end{};
    std::size_t rectangle_index{};
  };

  struct WitnessInterval {
    std::uint64_t begin{};
    std::uint64_t end{};
  };

  [[nodiscard]] static bool product_rectangles_are_disjoint(
      const std::vector<ProductRectangle>& rectangles,
      ExactPairSupportCheckpointVerification::ValidationAudit& audit) {
    std::vector<ProductRectangleSweepEvent> events;
    events.reserve(checked_multiply(
        rectangles.size(),
        2U,
        "the pair-support rectangle sweep event count overflows size_t"));
    for (std::size_t rectangle_index = 0U;
         rectangle_index < rectangles.size();
         ++rectangle_index) {
      const ProductRectangle& rectangle = rectangles[rectangle_index];
      if (rectangle.x_begin >= rectangle.x_end ||
          rectangle.y_begin >= rectangle.y_end) {
        return false;
      }
      events.push_back(ProductRectangleSweepEvent{
          rectangle.x_begin, rectangle_index, true});
      events.push_back(ProductRectangleSweepEvent{
          rectangle.x_end, rectangle_index, false});
    }
    std::sort(
        events.begin(),
        events.end(),
        [&rectangles](const ProductRectangleSweepEvent& left,
                      const ProductRectangleSweepEvent& right) {
          if (left.x != right.x) {
            return left.x < right.x;
          }
          // Half-open rectangles that only touch at x are disjoint.
          if (left.is_start != right.is_start) {
            return !left.is_start;
          }
          const ProductRectangle& left_rectangle =
              rectangles[left.rectangle_index];
          const ProductRectangle& right_rectangle =
              rectangles[right.rectangle_index];
          if (left_rectangle.y_begin != right_rectangle.y_begin) {
            return left_rectangle.y_begin < right_rectangle.y_begin;
          }
          return left.rectangle_index < right.rectangle_index;
        });

    // Until the first contradiction, active y intervals are disjoint.  This
    // makes predecessor/successor checks sufficient and keeps the sweep
    // O(F log F) with O(F) transient storage.
    std::map<std::uint64_t, ActiveProductRectangle> active;
    for (const ProductRectangleSweepEvent& event : events) {
      audit.frontier_active_set_operation_count = checked_add(
          audit.frontier_active_set_operation_count,
          1U,
          "the checkpoint frontier active-set operation count overflows size_t");
      const ProductRectangle& rectangle =
          rectangles[event.rectangle_index];
      if (!event.is_start) {
        const auto found = active.find(rectangle.y_begin);
        if (found == active.end() ||
            found->second.y_end != rectangle.y_end ||
            found->second.rectangle_index != event.rectangle_index) {
          return false;
        }
        active.erase(found);
        continue;
      }

      const auto successor = active.lower_bound(rectangle.y_begin);
      if (successor != active.end()) {
        audit.frontier_neighbor_test_count = checked_add(
            audit.frontier_neighbor_test_count,
            1U,
            "the checkpoint frontier neighbor count overflows size_t");
        if (successor->first < rectangle.y_end) {
          return false;
        }
      }
      if (successor != active.begin()) {
        audit.frontier_neighbor_test_count = checked_add(
            audit.frontier_neighbor_test_count,
            1U,
            "the checkpoint frontier neighbor count overflows size_t");
        const auto predecessor = std::prev(successor);
        if (predecessor->second.y_end > rectangle.y_begin) {
          return false;
        }
      }
      const auto [inserted, unique] = active.emplace(
          rectangle.y_begin,
          ActiveProductRectangle{
              rectangle.y_end, event.rectangle_index});
      static_cast<void>(inserted);
      if (!unique) {
        return false;
      }
    }
    return active.empty();
  }

  [[nodiscard]] static bool witness_intervals_are_disjoint(
      std::vector<WitnessInterval> intervals,
      ExactPairSupportCheckpointVerification::ValidationAudit& audit) {
    std::sort(
        intervals.begin(),
        intervals.end(),
        [](const WitnessInterval& left, const WitnessInterval& right) {
          if (left.begin != right.begin) {
            return left.begin < right.begin;
          }
          return left.end < right.end;
        });
    bool initialized = false;
    std::uint64_t maximum_end = 0U;
    for (const WitnessInterval& interval : intervals) {
      if (interval.begin >= interval.end) {
        return false;
      }
      if (initialized) {
        audit.witness_adjacent_interval_test_count = checked_add(
            audit.witness_adjacent_interval_test_count,
            1U,
            "the checkpoint witness adjacency count overflows size_t");
        if (interval.begin < maximum_end) {
          return false;
        }
      }
      initialized = true;
      maximum_end = std::max(maximum_end, interval.end);
    }
    return true;
  }

  [[nodiscard]] ExactPairSupportStreamAudit audit_snapshot() const {
    ExactPairSupportStreamAudit snapshot = result_.audit;
    snapshot.remaining_frontier_pair_count = 0U;
    for (const ExactPairSupportFrontierEntry& entry : frontier_) {
      snapshot.remaining_frontier_pair_count = checked_add(
          snapshot.remaining_frontier_pair_count,
          entry_pair_count(entry),
          "the checkpoint remaining-pair count overflows size_t");
    }
    snapshot.pair_partition_accounting_certified =
        checked_add(
            snapshot.resolved_pair_count,
            snapshot.remaining_frontier_pair_count,
            "the checkpoint pair accounting overflows size_t") ==
            snapshot.total_pair_count &&
        snapshot.resolved_pair_count == checked_add(
            snapshot.rank_pruned_pair_count,
            snapshot.leaf_pair_classification_count,
            "the checkpoint terminal accounting overflows size_t");
    return snapshot;
  }

  [[nodiscard]] ExactPairSupportCheckpointVerification
  checkpoint_verification(
      const ExactPairSupportCheckpointManifest& authority_manifest,
      const ExactPairSupportCheckpoint& checkpoint) const {
    ExactPairSupportCheckpointVerification verification;
    verification.manifest_matches_authorities =
        checkpoint.manifest == authority_manifest;
    verification.checksum_matches_payload =
        checkpoint.checkpoint_digest == checkpoint_digest(checkpoint);
    if (!verification.manifest_matches_authorities ||
        !verification.checksum_matches_payload) {
      return verification;
    }

    std::size_t remaining_pair_count = 0U;
    bool frontier_valid = true;
    try {
      std::vector<ProductRectangle> rectangles;
      rectangles.reserve(checked_multiply(
          checkpoint.frontier.size(),
          2U,
          "the checkpoint frontier rectangle count overflows size_t"));
      for (std::size_t index = 0U;
           index < checkpoint.frontier.size();
           ++index) {
        const ExactPairSupportFrontierEntry& entry =
            checkpoint.frontier[index];
        static_cast<void>(entry_nodes(entry));
        verification.validation_audit.frontier_entry_validation_count =
            checked_add(
                verification.validation_audit
                    .frontier_entry_validation_count,
                1U,
                "the checkpoint frontier validation count overflows size_t");
        remaining_pair_count = checked_add(
            remaining_pair_count,
            entry_pair_count(entry),
            "the checkpoint frontier pair count overflows size_t");
        rectangles.push_back(ProductRectangle{
            entry.first_leaf_begin,
            entry.first_leaf_end,
            entry.second_leaf_begin,
            entry.second_leaf_end});
        if (entry.self_product == 0U) {
          rectangles.push_back(ProductRectangle{
              entry.second_leaf_begin,
              entry.second_leaf_end,
              entry.first_leaf_begin,
              entry.first_leaf_end});
        }
      }
      verification.validation_audit.frontier_rectangle_count =
          rectangles.size();
      verification.validation_audit.frontier_sweep_event_count =
          checked_multiply(
              rectangles.size(),
              2U,
              "the checkpoint frontier sweep count overflows size_t");
      frontier_valid = product_rectangles_are_disjoint(
          rectangles, verification.validation_audit);
    } catch (const std::exception&) {
      frontier_valid = false;
    }
    verification.frontier_locally_valid = frontier_valid;

    bool pending_valid = frontier_valid;
    bool pending_rank_search_started = false;
    std::size_t pending_active_witness_entry_count = 0U;
    std::size_t pending_strict_receipt_count = 0U;
    std::size_t pending_strict_receipt_point_count = 0U;
    std::size_t pending_self_expansion_count = 0U;
    std::size_t pending_active_closed_ball_entry_count = 0U;
    std::size_t pending_closed_ball_classified_count = 0U;
    std::size_t pending_closed_ball_interior_count = 0U;
    std::size_t pending_closed_ball_node_visit_count = 0U;
    bool pending_closed_ball_started = false;
    if (checkpoint.pending_product.has_value()) {
      if (checkpoint.frontier.empty() ||
          checkpoint.pending_product->product != checkpoint.frontier.back()) {
        pending_valid = false;
      } else {
        try {
          const ExactPairSupportPendingProduct& pending =
              *checkpoint.pending_product;
          const auto [first_node_index, second_node_index] =
              entry_nodes(pending.product);
          const Node& first = node(first_node_index);
          const Node& second = node(second_node_index);
          const bool empty_rank_payload =
              !pending.rank_search_started &&
              pending.witness_frontier.empty() &&
              pending.strict_witness_receipts.empty() &&
              !pending.deferred_expansion_node.has_value() &&
              pending.strict_witness_point_count == 0U;
          switch (pending.stage) {
            case ExactPairSupportPendingStage::rank_search: {
              if (first_node_index == second_node_index ||
                  (first.is_leaf() && second.is_leaf()) ||
                  pending.closed_ball.has_value() ||
                  (!pending.rank_search_started && !empty_rank_payload)) {
                pending_valid = false;
                break;
              }
              std::vector<ExactPairSupportWitnessNodeEntry> active_entries =
                  pending.witness_frontier;
              if (pending.deferred_expansion_node.has_value()) {
                if (node(witness_node_index(
                             *pending.deferred_expansion_node))
                        .is_leaf()) {
                  pending_valid = false;
                }
                active_entries.push_back(*pending.deferred_expansion_node);
              }
              pending_rank_search_started = pending.rank_search_started;
              pending_active_witness_entry_count = active_entries.size();
              pending_strict_receipt_count =
                  pending.strict_witness_receipts.size();
              verification.validation_audit
                  .active_witness_entry_validation_count =
                  active_entries.size();
              verification.validation_audit
                  .strict_witness_receipt_validation_count =
                  pending.strict_witness_receipts.size();
              std::vector<WitnessInterval> witness_intervals;
              witness_intervals.reserve(checked_add(
                  active_entries.size(),
                  pending.strict_witness_receipts.size(),
                  "the checkpoint witness interval count overflows size_t"));
              std::size_t receipt_point_count = 0U;
              const spatial::ExactDyadicAabb3 first_box =
                  node_box(first_node_index);
              const spatial::ExactDyadicAabb3 second_box =
                  node_box(second_node_index);
              for (std::size_t receipt_index = 0U;
                   receipt_index < pending.strict_witness_receipts.size();
                   ++receipt_index) {
                const ExactPairSupportWitnessNodeEntry& receipt =
                    pending.strict_witness_receipts[receipt_index];
                const std::size_t receipt_node_index =
                    witness_node_index(receipt);
                const Node& receipt_node = node(receipt_node_index);
                verification.validation_audit
                    .strict_receipt_geometry_recertification_count =
                    checked_add(
                        verification.validation_audit
                            .strict_receipt_geometry_recertification_count,
                        1U,
                        "the checkpoint receipt recertification count overflows size_t");
                if (node_range_intersects(receipt_node, first) ||
                    node_range_intersects(receipt_node, second) ||
                    exact_diametral_phi_aabb_maximum_sign(
                        first_box,
                        second_box,
                        node_box(receipt_node_index)) >= 0) {
                  pending_valid = false;
                }
                receipt_point_count = checked_add(
                    receipt_point_count,
                    receipt_node.leaf_end - receipt_node.leaf_begin,
                    "the checkpoint witness receipt count overflows size_t");
                witness_intervals.push_back(WitnessInterval{
                    receipt.leaf_begin, receipt.leaf_end});
              }
              for (const ExactPairSupportWitnessNodeEntry& active :
                   active_entries) {
                static_cast<void>(witness_node_index(active));
                witness_intervals.push_back(WitnessInterval{
                    active.leaf_begin, active.leaf_end});
              }
              verification.validation_audit.witness_interval_count =
                  witness_intervals.size();
              if (!witness_intervals_are_disjoint(
                      std::move(witness_intervals),
                      verification.validation_audit)) {
                pending_valid = false;
              }
              const std::size_t witness_threshold =
                  checkpoint.manifest.maximum_relevant_closed_rank - 1U;
              if (receipt_point_count !=
                      pending.strict_witness_point_count ||
                  receipt_point_count >= witness_threshold) {
                pending_valid = false;
              }
              pending_strict_receipt_point_count = receipt_point_count;
              break;
            }
            case ExactPairSupportPendingStage::expand_product:
              if (!empty_rank_payload ||
                  pending.closed_ball.has_value() ||
                  (first.is_leaf() && second.is_leaf())) {
                pending_valid = false;
              } else if (first_node_index == second_node_index) {
                pending_self_expansion_count = 1U;
              }
              break;
            case ExactPairSupportPendingStage::classify_leaf:
              if (!empty_rank_payload || first_node_index == second_node_index ||
                  !first.is_leaf() || !second.is_leaf()) {
                pending_valid = false;
              } else if (pending.closed_ball.has_value()) {
                const ExactPairSupportPendingClosedBall& closed_ball =
                    *pending.closed_ball;
                pending_closed_ball_started = true;
                pending_closed_ball_node_visit_count =
                    closed_ball.node_visit_count;
                pending_closed_ball_interior_count =
                    closed_ball.interior_ids.size();
                pending_active_closed_ball_entry_count =
                    closed_ball.frontier.size();
                if (closed_ball.support_seen_mask > 3U ||
                    closed_ball.interior_ids.size() >
                        checkpoint.manifest.maximum_relevant_closed_rank -
                            2U) {
                  pending_valid = false;
                  break;
                }
                std::array<PointId, 2U> support_ids{
                    index_.leaves_[first.leaf_begin].point_id,
                    index_.leaves_[second.leaf_begin].point_id};
                if (support_ids[1] < support_ids[0]) {
                  std::swap(support_ids[0], support_ids[1]);
                }
                const ExactDyadicAnchorPhi anchor =
                    exact_dyadic_anchor_phi(
                        cloud_.point(support_ids[0]),
                        cloud_.point(support_ids[1]));
                pending_closed_ball_classified_count = checked_add(
                    checked_add(
                        closed_ball.interior_ids.size(),
                        closed_ball.shell_count,
                        "the pending closed-ball classified count overflows size_t"),
                    closed_ball.exterior_count,
                    "the pending closed-ball classified count overflows size_t");
                if (pending_closed_ball_classified_count >
                        cloud_.size() ||
                    closed_ball.node_visit_count >
                        index_.nodes_.size()) {
                  pending_valid = false;
                } else {
                  // Validation-only replay of this query's committed
                  // canonical DFS.  It mirrors the product's exact subtree
                  // decisions, reconstructs at most the rank-bounded
                  // interior ids, and never scans the classified point mass.
                  // Cost is O(committed node visits + K), memory O(H + K).
                  std::vector<ExactPairSupportWitnessNodeEntry>
                      recertified_frontier{
                          make_witness_entry(index_.root_index_)};
                  std::vector<PointId> recertified_interior_ids;
                  recertified_interior_ids.reserve(
                      closed_ball.interior_ids.size());
                  std::size_t recertified_shell_count = 0U;
                  std::size_t recertified_exterior_count = 0U;
                  std::uint8_t recertified_support_seen_mask = 0U;
                  std::optional<PointId>
                      recertified_canonical_extra_shell_witness_id;
                  bool canonical_replay_valid = true;
                  for (std::size_t visit = 0U;
                       visit < closed_ball.node_visit_count;
                       ++visit) {
                    if (recertified_frontier.empty()) {
                      canonical_replay_valid = false;
                      break;
                    }
                    const ExactPairSupportWitnessNodeEntry entry =
                        recertified_frontier.back();
                    recertified_frontier.pop_back();
                    const std::size_t replay_node_index =
                        witness_node_index(entry);
                    const Node& replay_node =
                        node(replay_node_index);
                    verification.validation_audit
                            .closed_ball_node_recertification_count =
                        checked_add(
                            verification.validation_audit
                                .closed_ball_node_recertification_count,
                            1U,
                            "the checkpoint closed-ball node validation count overflows size_t");
                    const spatial::ExactDyadicAabb3 replay_box =
                        node_box(replay_node_index);
                    const int minimum_phi_sign =
                        exact_anchor_phi_aabb_minimum_sign(
                            anchor, replay_box);
                    if (minimum_phi_sign > 0) {
                      recertified_exterior_count = checked_add(
                          recertified_exterior_count,
                          replay_node.leaf_end -
                              replay_node.leaf_begin,
                          "the checkpoint recertified exterior count overflows size_t");
                      continue;
                    }
                    const int maximum_phi_sign =
                        exact_diametral_point_phi_aabb_maximum_sign(
                            anchor, replay_box);
                    if (maximum_phi_sign < 0) {
                      const std::size_t interior_count = checked_add(
                          recertified_interior_ids.size(),
                          replay_node.leaf_end -
                              replay_node.leaf_begin,
                          "the checkpoint recertified interior count overflows size_t");
                      if (interior_count >
                          checkpoint.manifest
                                  .maximum_relevant_closed_rank -
                              2U) {
                        canonical_replay_valid = false;
                        break;
                      }
                      for (std::size_t position =
                               replay_node.leaf_begin;
                           position < replay_node.leaf_end;
                           ++position) {
                        recertified_interior_ids.push_back(
                            index_.leaves_[position].point_id);
                        verification.validation_audit
                                .closed_ball_interior_id_recertification_count =
                            checked_add(
                                verification.validation_audit
                                    .closed_ball_interior_id_recertification_count,
                                1U,
                                "the checkpoint closed-ball interior-id validation count overflows size_t");
                      }
                      continue;
                    }
                    if (replay_node.is_leaf()) {
                      if (minimum_phi_sign != maximum_phi_sign) {
                        canonical_replay_valid = false;
                        break;
                      }
                      const PointId point_id =
                          index_.leaves_[replay_node.leaf_begin]
                              .point_id;
                      if (maximum_phi_sign == 0) {
                        recertified_shell_count = checked_add(
                            recertified_shell_count,
                            1U,
                            "the checkpoint recertified shell count overflows size_t");
                        if (point_id == support_ids[0]) {
                          recertified_support_seen_mask =
                              static_cast<std::uint8_t>(
                                  recertified_support_seen_mask |
                                  1U);
                        } else if (point_id == support_ids[1]) {
                          recertified_support_seen_mask =
                              static_cast<std::uint8_t>(
                                  recertified_support_seen_mask |
                                  2U);
                        } else if (
                            !recertified_canonical_extra_shell_witness_id
                                 .has_value() ||
                            point_id <
                                *recertified_canonical_extra_shell_witness_id) {
                          recertified_canonical_extra_shell_witness_id =
                              point_id;
                        }
                      } else {
                        recertified_exterior_count = checked_add(
                            recertified_exterior_count,
                            1U,
                            "the checkpoint recertified exterior count overflows size_t");
                      }
                      continue;
                    }
                    recertified_frontier.push_back(
                        make_witness_entry(
                            replay_node.right_child));
                    recertified_frontier.push_back(
                        make_witness_entry(
                            replay_node.left_child));
                  }
                  switch (closed_ball.stage) {
                    case ExactPairSupportPendingClosedBallStage::
                        traversing:
                      if (recertified_interior_ids !=
                          closed_ball.interior_ids) {
                        canonical_replay_valid = false;
                      }
                      break;
                    case ExactPairSupportPendingClosedBallStage::
                        ready_to_emit: {
                      std::sort(
                          recertified_interior_ids.begin(),
                          recertified_interior_ids.end());
                      const bool persisted_strictly_sorted =
                          std::adjacent_find(
                              closed_ball.interior_ids.begin(),
                              closed_ball.interior_ids.end(),
                              [](PointId left, PointId right) {
                                return !(left < right);
                              }) ==
                          closed_ball.interior_ids.end();
                      if (!persisted_strictly_sorted ||
                          recertified_interior_ids !=
                              closed_ball.interior_ids) {
                        canonical_replay_valid = false;
                      }
                      break;
                    }
                    default:
                      canonical_replay_valid = false;
                      break;
                  }
                  if (!canonical_replay_valid ||
                      recertified_shell_count !=
                          closed_ball.shell_count ||
                      recertified_exterior_count !=
                          closed_ball.exterior_count ||
                      recertified_support_seen_mask !=
                          closed_ball.support_seen_mask ||
                      recertified_canonical_extra_shell_witness_id !=
                          closed_ball
                              .canonical_extra_shell_witness_id ||
                      recertified_frontier !=
                          closed_ball.frontier) {
                    pending_valid = false;
                  }
                }
                std::size_t expected_leaf_begin =
                    pending_closed_ball_classified_count;
                for (auto iterator =
                         closed_ball.frontier.rbegin();
                     iterator != closed_ball.frontier.rend();
                     ++iterator) {
                  const std::size_t active_index =
                      witness_node_index(*iterator);
                  const Node& active = node(active_index);
                  if (active.leaf_begin != expected_leaf_begin) {
                    pending_valid = false;
                  }
                  expected_leaf_begin = active.leaf_end;
                }
                if (expected_leaf_begin != cloud_.size()) {
                  pending_valid = false;
                }
                switch (closed_ball.stage) {
                  case ExactPairSupportPendingClosedBallStage::
                      traversing:
                    if (closed_ball.frontier.empty() ||
                        pending_closed_ball_classified_count >=
                            cloud_.size()) {
                      pending_valid = false;
                    }
                    break;
                  case ExactPairSupportPendingClosedBallStage::
                      ready_to_emit:
                    if (!closed_ball.frontier.empty() ||
                        pending_closed_ball_classified_count !=
                            cloud_.size() ||
                        closed_ball.support_seen_mask != 3U ||
                        closed_ball.shell_count < 2U ||
                        ((closed_ball.shell_count == 2U) !=
                         !closed_ball
                              .canonical_extra_shell_witness_id
                              .has_value())) {
                      pending_valid = false;
                    }
                    break;
                  default:
                    pending_valid = false;
                    break;
                }
              }
              break;
            default:
              pending_valid = false;
              break;
          }
        } catch (const std::exception&) {
          pending_valid = false;
        }
      }
    }
    verification.pending_product_locally_valid = pending_valid;

    const ExactPairSupportStreamAudit& audit = checkpoint.cumulative_audit;
    bool audit_valid = false;
    try {
      const std::size_t terminal_count = checked_add(
          audit.rank_pruned_pair_count,
          audit.leaf_pair_classification_count,
          "the checkpoint terminal count overflows size_t");
      const std::size_t classified_categories = checked_add(
          checked_add(
              audit.accepted_event_count,
              audit.relevant_extra_shell_diagnostic_count,
              "the checkpoint record count overflows size_t"),
          audit.above_rank_pair_count,
          "the checkpoint leaf category count overflows size_t");
      const std::size_t completed_product_visits = checked_add(
          checked_add(
              audit.support_product_expansion_count,
              audit.diagonal_leaf_discard_count,
              "the checkpoint product visit count overflows size_t"),
          checked_add(
              audit.rank_pruned_product_count,
              audit.leaf_pair_classification_count,
              "the checkpoint product visit count overflows size_t"),
          "the checkpoint product visit count overflows size_t");
      const std::size_t expected_product_visits = checked_add(
          completed_product_visits,
          checkpoint.pending_product.has_value() ? 1U : 0U,
          "the checkpoint active product count overflows size_t");
      const std::size_t expected_phi_bound_count = checked_add(
          audit.strict_interior_witness_subtree_count,
          audit.exact_anchor_ball_minimum_aabb_bound_count,
          "the checkpoint phi-bound partition overflows size_t");
      const std::size_t expected_anchor_bound_count = checked_add(
          audit.certified_anchor_noninterior_subtree_count,
          audit.equality_or_positive_bound_descent_count,
          "the checkpoint anchor-bound partition overflows size_t");
      const std::size_t expected_closed_ball_node_visits = checked_add(
          audit.exact_closed_ball_maximum_aabb_bound_count,
          audit.closed_ball_bulk_exterior_subtree_count,
          "the checkpoint closed-ball node partition overflows size_t");
      const std::size_t expected_point_classifications = checked_add(
          checked_add(
              audit.closed_ball_bulk_interior_point_count,
              audit.closed_ball_bulk_exterior_point_count,
              "the checkpoint bulk point classification count overflows size_t"),
          audit.exact_point_distance_evaluation_count,
          "the checkpoint point classification count overflows size_t");
      const std::size_t historical_non_center_work =
          historical_non_center_work_unit_count(audit);
      const std::size_t maximum_amortized_center_cover_work =
          center_cover_post_attempt_work_limit(
              historical_non_center_work);
      const std::size_t maximum_center_cover_cell_visit_count =
          checked_multiply(
              audit.center_cover_attempt_count,
              pair_support_center_cover_maximum_cell_visit_count,
              "the checkpoint center-cover cell envelope overflows size_t");
      const std::size_t maximum_center_cover_witness_node_visit_count =
          checked_multiply(
              audit.center_cover_attempt_count,
              pair_support_center_cover_maximum_witness_node_visit_count,
              "the checkpoint center-cover witness envelope overflows size_t");
      const std::size_t center_cover_product_decision_count =
          checked_add(
              audit.center_cover_attempt_count,
              audit.center_cover_work_preflight_skip_count,
              "the checkpoint center-cover decision count overflows size_t");
      const std::size_t center_cover_finished_cell_count =
          checked_add(
              audit.center_cover_certified_cell_count,
              audit.center_cover_cell_split_count,
              "the checkpoint center-cover finished-cell count overflows size_t");
      const std::size_t center_cover_tree_cell_visit_limit =
          checked_add(
              audit.center_cover_attempt_count,
              checked_multiply(
                  audit.center_cover_cell_split_count,
                  2U,
                  "the checkpoint center-cover doubled split count overflows size_t"),
              "the checkpoint center-cover tree visit limit overflows size_t");
      const std::size_t center_cover_tree_leaf_limit =
          checked_add(
              audit.center_cover_attempt_count,
              audit.center_cover_cell_split_count,
              "the checkpoint center-cover tree leaf limit overflows size_t");
      const std::size_t center_cover_required_strict_witness_point_count =
          checked_multiply(
              audit.center_cover_certified_cell_count,
              checkpoint.manifest.maximum_relevant_closed_rank == 0U
                  ? 0U
                  : checkpoint.manifest.maximum_relevant_closed_rank -
                        1U,
              "the checkpoint center-cover required witness count overflows size_t");
      const std::size_t minimum_closed_ball_node_visit_count =
          checked_add(
              audit.leaf_pair_classification_count,
              pending_closed_ball_node_visit_count,
              "the checkpoint completed-plus-active closed-ball node count overflows size_t");
      const std::size_t minimum_point_classification_count =
          checked_add(
              audit.leaf_pair_classification_count,
              pending_closed_ball_classified_count,
              "the checkpoint completed-plus-active point count overflows size_t");
      audit_valid =
          audit.total_pair_count ==
              checked_unordered_pair_count(cloud_.size()) &&
          audit.remaining_frontier_pair_count == remaining_pair_count &&
          checked_add(
              audit.resolved_pair_count,
              remaining_pair_count,
              "the checkpoint accounting sum overflows size_t") ==
              audit.total_pair_count &&
          audit.resolved_pair_count == terminal_count &&
          audit.leaf_pair_classification_count == classified_categories &&
          audit.support_product_visit_count == expected_product_visits &&
          audit.support_product_expansion_count == checked_add(
              audit.self_product_expansion_count,
              audit.cross_product_expansion_count,
              "the checkpoint expansion sum overflows size_t") &&
          audit.diagonal_product_rank_search_skip_count == checked_add(
              audit.self_product_expansion_count,
              pending_self_expansion_count,
              "the checkpoint diagonal-skip count overflows size_t") &&
          audit.work_unit_count == checked_add(
              historical_non_center_work,
              audit.center_cover_work_unit_count,
              "the checkpoint work sum overflows size_t") &&
          audit.center_cover_work_unit_count <=
              maximum_amortized_center_cover_work &&
          audit.center_cover_work_unit_count == checked_add(
              audit.center_cover_cell_visit_count,
              audit.center_cover_witness_node_visit_count,
              "the checkpoint center-cover work sum overflows size_t") &&
          audit.center_cover_attempt_count == checked_add(
              audit.center_cover_inconclusive_count,
              audit.center_cover_pruned_product_count,
              "the checkpoint center-cover outcome sum overflows size_t") &&
          audit.center_cover_attempt_count <=
              audit.center_cover_cell_visit_count &&
          audit.center_cover_cell_visit_count <=
              maximum_center_cover_cell_visit_count &&
          audit.center_cover_cell_visit_count <=
              center_cover_tree_cell_visit_limit &&
          audit.center_cover_witness_node_visit_count <=
              maximum_center_cover_witness_node_visit_count &&
          center_cover_product_decision_count <=
              audit.support_product_visit_count &&
          audit.center_cover_pruned_product_count <=
              audit.rank_pruned_product_count &&
          audit.center_cover_pruned_product_count <=
              audit.center_cover_certified_cell_count &&
          audit.center_cover_cell_split_count <=
              audit.center_cover_cell_visit_count &&
          audit.center_cover_certified_cell_count <=
              audit.center_cover_cell_visit_count &&
          audit.center_cover_certified_cell_count <=
              center_cover_tree_leaf_limit &&
          center_cover_finished_cell_count <=
              audit.center_cover_cell_visit_count &&
          audit.center_cover_certified_cell_count <=
              audit.center_cover_strict_witness_subtree_count &&
          audit.center_cover_strict_witness_subtree_count <=
              audit.center_cover_witness_node_visit_count &&
          audit.center_cover_strict_witness_point_count >=
              audit.center_cover_strict_witness_subtree_count &&
          audit.center_cover_strict_witness_point_count >=
              center_cover_required_strict_witness_point_count &&
          audit.exact_phi_aabb_bound_count == expected_phi_bound_count &&
          audit.exact_anchor_ball_minimum_aabb_bound_count ==
              expected_anchor_bound_count &&
          audit.certified_anchor_shell_tangent_subtree_count <=
              audit.certified_anchor_noninterior_subtree_count &&
          audit.rank_pruned_product_count <= checked_add(
              audit.rank_prune_search_count,
              audit.center_cover_pruned_product_count,
              "the checkpoint rank-prune source count overflows size_t") &&
          audit.rank_prune_search_count <= checked_add(
              audit.witness_node_visit_count,
              pending_rank_search_started ? 1U : 0U,
              "the checkpoint rank-search lower work bound overflows size_t") &&
          audit.global_closed_ball_query_count == checked_add(
              audit.leaf_pair_classification_count,
              pending_closed_ball_started ? 1U : 0U,
              "the checkpoint active closed-ball query count overflows size_t") &&
          audit.closed_ball_node_visit_count ==
              audit.exact_closed_ball_minimum_aabb_bound_count &&
          audit.closed_ball_node_visit_count ==
              expected_closed_ball_node_visits &&
          audit.point_classification_count ==
              expected_point_classifications &&
          audit.early_closed_rank_rejection_count <=
              audit.global_closed_ball_query_count &&
          audit.closed_ball_node_visit_count >=
              minimum_closed_ball_node_visit_count &&
          (audit.global_closed_ball_query_count == 0U ||
           audit.maximum_closed_ball_frontier_entry_count > 0U) &&
          audit.maximum_frontier_entry_count >= checkpoint.frontier.size() &&
          audit.maximum_witness_frontier_entry_count >=
              pending_active_witness_entry_count &&
          audit.maximum_closed_ball_frontier_entry_count >=
              pending_active_closed_ball_entry_count &&
          audit.point_classification_count >=
              minimum_point_classification_count &&
          audit.strict_interior_witness_subtree_count >=
              pending_strict_receipt_count &&
          audit.strict_interior_witness_point_count >=
              pending_strict_receipt_point_count &&
          audit.exact_phi_aabb_bound_count >=
              pending_strict_receipt_count &&
          audit.witness_node_visit_count >=
              checked_add(
                  pending_strict_receipt_count,
                  checkpoint.pending_product.has_value() &&
                          checkpoint.pending_product
                              ->deferred_expansion_node.has_value()
                      ? 1U
                      : 0U,
                  "the checkpoint pending witness lower bound overflows size_t") &&
          (!pending_rank_search_started ||
           audit.rank_prune_search_count > 0U) &&
          checkpoint.output_record_count == checked_add(
              audit.accepted_event_count,
              audit.relevant_extra_shell_diagnostic_count,
              "the checkpoint output record count overflows size_t") &&
          audit.pair_partition_accounting_certified;
    } catch (const std::exception&) {
      audit_valid = false;
    }
    verification.required_audit_identities_hold = audit_valid;
    verification.quasi_linear_structure_validation_certified =
        verification.frontier_locally_valid &&
        verification.pending_product_locally_valid &&
        verification.validation_audit.frontier_sweep_event_count ==
            checked_multiply(
                verification.validation_audit.frontier_rectangle_count,
                2U,
                "the checkpoint certified sweep count overflows size_t") &&
        verification.validation_audit
                .frontier_active_set_operation_count ==
            verification.validation_audit.frontier_sweep_event_count &&
        verification.validation_audit.frontier_neighbor_test_count <=
            checked_multiply(
                verification.validation_audit.frontier_rectangle_count,
                2U,
                "the checkpoint certified neighbor count overflows size_t") &&
        verification.validation_audit.witness_interval_count ==
            checked_add(
                verification.validation_audit
                    .active_witness_entry_validation_count,
                verification.validation_audit
                    .strict_witness_receipt_validation_count,
                "the checkpoint certified witness count overflows size_t") &&
        verification.validation_audit
                .witness_adjacent_interval_test_count ==
            (verification.validation_audit.witness_interval_count == 0U
                 ? 0U
                 : verification.validation_audit.witness_interval_count -
                       1U) &&
        verification.validation_audit
                .strict_receipt_geometry_recertification_count ==
            verification.validation_audit
                .strict_witness_receipt_validation_count &&
        verification.validation_audit
                .closed_ball_interior_id_recertification_count ==
            pending_closed_ball_interior_count &&
        verification.validation_audit
                .closed_ball_node_recertification_count ==
            pending_closed_ball_node_visit_count;
    verification.integrity_verified =
        verification.manifest_matches_authorities &&
        verification.checksum_matches_payload &&
        verification.frontier_locally_valid &&
        verification.pending_product_locally_valid &&
        verification.required_audit_identities_hold &&
        verification.quasi_linear_structure_validation_certified;
    return verification;
  }

  void validate_inputs(
      std::size_t requested_maximum_order,
      const ExactPairSupportStreamBudget& budget) const {
    if (!index_.validated_for(cloud_)) {
      throw std::invalid_argument(
          "the pair-support stream requires the supplied cloud's Morton LBVH");
    }
    if (cloud_.size() == 0U) {
      throw std::invalid_argument(
          "the pair-support stream requires a nonempty point cloud");
    }
    if (requested_maximum_order == 0U ||
        requested_maximum_order > pair_support_maximum_requested_order) {
      throw std::out_of_range(
          "the pair-support stream requires 1<=Kmax<=10");
    }
    static_cast<void>(checked_unordered_pair_count(cloud_.size()));
    static_cast<void>(budget);
  }

  [[nodiscard]] ExactPairSupportRequirements requirements_for(
      std::size_t requested_maximum_order) const {
    ExactPairSupportRequirements requirements;
    requirements.point_count = cloud_.size();
    requirements.requested_maximum_order = requested_maximum_order;
    requirements.effective_maximum_order =
        std::min(requested_maximum_order, cloud_.size());
    requirements.maximum_relevant_closed_rank = std::min(
        checked_add(
            requirements.effective_maximum_order,
            1U,
            "the pair-support relevant closed rank overflows size_t"),
        cloud_.size());
    return requirements;
  }

  [[nodiscard]] const Node& node(std::size_t node_index) const {
    if (node_index >= index_.nodes_.size()) {
      throw std::logic_error(
          "a pair-support frontier references an invalid LBVH node");
    }
    return index_.nodes_[node_index];
  }

  [[nodiscard]] spatial::ExactDyadicAabb3 node_box(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    spatial::ExactDyadicAabb3 box{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] =
          cloud_.point(current.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      box.upper_binary64_bits[axis] =
          cloud_.point(current.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    return box;
  }

  [[nodiscard]] spatial::ExactDyadicAabb3 point_box(
      PointId point_id) const {
    if (point_id >= cloud_.size()) {
      throw std::logic_error(
          "a pair-support cursor references an invalid point id");
    }
    spatial::ExactDyadicAabb3 box{};
    const auto& bits =
        cloud_.point(point_id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] = bits[axis];
      box.upper_binary64_bits[axis] = bits[axis];
    }
    return box;
  }

  [[nodiscard]] exact::ExactLevel node_squared_diagonal(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    exact::ExactRational squared;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const exact::ExactRational lower =
          cloud_.point(current.lower_point_ids[axis]).coordinate(axis);
      const exact::ExactRational upper =
          cloud_.point(current.upper_point_ids[axis]).coordinate(axis);
      const exact::ExactRational delta = upper - lower;
      squared = squared + delta * delta;
    }
    return exact::ExactLevel{std::move(squared)};
  }

  [[nodiscard]] ExactPairSupportFrontierEntry make_entry(
      std::size_t first_node_index,
      std::size_t second_node_index) const {
    const Node* first = &node(first_node_index);
    const Node* second = &node(second_node_index);
    if (first_node_index != second_node_index &&
        second->leaf_begin < first->leaf_begin) {
      std::swap(first_node_index, second_node_index);
      std::swap(first, second);
    }
    const bool self_product = first_node_index == second_node_index;
    if (!self_product && first->leaf_end > second->leaf_begin) {
      throw std::logic_error(
          "a pair-support cross product has overlapping Morton ranges");
    }
    return ExactPairSupportFrontierEntry{
        checked_u64(
            first_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            second_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            first->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            first->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        static_cast<std::uint8_t>(self_product ? 1U : 0U)};
  }

  [[nodiscard]] std::pair<std::size_t, std::size_t> entry_nodes(
      const ExactPairSupportFrontierEntry& entry) const {
    const std::size_t first_index = checked_size(
        entry.first_node_index,
        "a pair-support frontier node index does not fit size_t");
    const std::size_t second_index = checked_size(
        entry.second_node_index,
        "a pair-support frontier node index does not fit size_t");
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const bool self_product = entry.self_product == 1U;
    if (entry.self_product > 1U ||
        self_product != (first_index == second_index) ||
        entry.first_leaf_begin != checked_u64(
            first.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.first_leaf_end != checked_u64(
            first.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_begin != checked_u64(
            second.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_end != checked_u64(
            second.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        (!self_product && first.leaf_end > second.leaf_begin)) {
      throw std::logic_error(
          "a pair-support frontier entry contradicts its LBVH nodes");
    }
    return {first_index, second_index};
  }

  [[nodiscard]] ExactPairSupportWitnessNodeEntry make_witness_entry(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    return ExactPairSupportWitnessNodeEntry{
        checked_u64(
            node_index,
            "a pair-support witness node index does not fit uint64"),
        checked_u64(
            current.leaf_begin,
            "a pair-support witness range does not fit uint64"),
        checked_u64(
            current.leaf_end,
            "a pair-support witness range does not fit uint64")};
  }

  [[nodiscard]] std::size_t witness_node_index(
      const ExactPairSupportWitnessNodeEntry& entry) const {
    const std::size_t node_index = checked_size(
        entry.node_index,
        "a pair-support witness node index does not fit size_t");
    const Node& current = node(node_index);
    if (entry.leaf_begin != checked_u64(
                                current.leaf_begin,
                                "a witness range does not fit uint64") ||
        entry.leaf_end != checked_u64(
                              current.leaf_end,
                              "a witness range does not fit uint64")) {
      throw std::logic_error(
          "a pair-support witness entry contradicts its LBVH node");
    }
    return node_index;
  }

  [[nodiscard]] std::size_t entry_pair_count(
      const ExactPairSupportFrontierEntry& entry) const {
    const auto [first_index, second_index] = entry_nodes(entry);
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const std::size_t first_size = first.leaf_end - first.leaf_begin;
    if (first_index == second_index) {
      return checked_unordered_pair_count(first_size);
    }
    const std::size_t second_size = second.leaf_end - second.leaf_begin;
    return checked_multiply(
        first_size,
        second_size,
        "a pair-support product coverage overflows size_t");
  }

  [[nodiscard]] static std::size_t consumed_since(
      std::size_t current,
      std::size_t origin,
      std::string_view message) {
    if (current < origin) {
      throw std::logic_error(std::string{message});
    }
    return current - origin;
  }

  [[nodiscard]] bool can_consume_work_units(
      std::size_t count) const {
    const std::size_t consumed = consumed_since(
        result_.audit.work_unit_count,
        chunk_audit_origin_.work_unit_count,
        "the pair-support work audit moved backwards");
    return can_add_within(
        consumed, count, result_.budget.maximum_work_unit_count);
  }

  [[nodiscard]] bool consume_work_units(std::size_t count) {
    if (!can_consume_work_units(count)) {
      stop(ExactPairSupportStopReason::work_unit_limit);
      return false;
    }
    result_.audit.work_unit_count = checked_add(
        result_.audit.work_unit_count,
        count,
        "the pair-support work count overflows size_t");
    return true;
  }

  [[nodiscard]] bool consume_work_unit() {
    return consume_work_units(1U);
  }

  void stop(ExactPairSupportStopReason reason) {
    if (!stopped_) {
      stopped_ = true;
      result_.status = ExactPairSupportStreamStatus::budget_exhausted;
      result_.stop_reason = reason;
    }
  }

  [[nodiscard]] bool node_range_intersects(
      const Node& query_node,
      const Node& support_node) const noexcept {
    return query_node.leaf_begin < support_node.leaf_end &&
           support_node.leaf_begin < query_node.leaf_end;
  }

  [[nodiscard]] static std::size_t node_range_intersection_size(
      const Node& left,
      const Node& right) noexcept {
    const std::size_t begin =
        std::max(left.leaf_begin, right.leaf_begin);
    const std::size_t end =
        std::min(left.leaf_end, right.leaf_end);
    return begin < end ? end - begin : 0U;
  }

  enum class CenterCoverCellSearchOutcome : std::uint8_t {
    certified,
    insufficient_closed_rank,
    resource_cap_reached,
  };

  enum class CenterCoverRankPruneOutcome : std::uint8_t {
    not_pruned,
    pruned,
    budget_exhausted,
  };

  struct CenterCoverCellSearchResult {
    CenterCoverCellSearchOutcome outcome{
        CenterCoverCellSearchOutcome::insufficient_closed_rank};
    ExactCenterCoverCellCertificate certificate{};
  };

  [[nodiscard]] CenterCoverCellSearchResult
  search_center_cover_cell(
      const ExactDoubledCenterCell& cell,
      const UnnormalizedExactDyadic& minimum_squared_distance,
      const Node& first_support,
      const Node& second_support,
      std::size_t required_strict_witness_count,
      std::size_t maximum_attempt_work_unit_count,
      ExactCenterCoverAttemptAudit& audit) const {
    CenterCoverCellSearchResult result;
    result.certificate.cell = cell;
    const std::size_t auxiliary_cap = std::min(
        pair_support_center_cover_maximum_auxiliary_entry_count,
        result_.budget.maximum_auxiliary_frontier_entry_count);
    if (auxiliary_cap == 0U) {
      result.outcome =
          CenterCoverCellSearchOutcome::resource_cap_reached;
      return result;
    }
    std::vector<std::size_t> frontier;
    frontier.reserve(auxiliary_cap);
    frontier.push_back(index_.root_index_);
    while (!frontier.empty()) {
      const std::size_t attempt_work_unit_count = checked_add(
          audit.cell_visit_count,
          audit.witness_node_visit_count,
          "the pair-support center-cover attempt work overflows size_t");
      if (audit.witness_node_visit_count >=
              pair_support_center_cover_maximum_witness_node_visit_count ||
          attempt_work_unit_count >=
              maximum_attempt_work_unit_count) {
        result.outcome =
            CenterCoverCellSearchOutcome::resource_cap_reached;
        return result;
      }
      const std::size_t query_node_index = frontier.back();
      frontier.pop_back();
      const Node& query = node(query_node_index);
      const spatial::ExactDyadicAabb3 query_box =
          node_box(query_node_index);
      audit.witness_node_visit_count = checked_add(
          audit.witness_node_visit_count,
          1U,
          "the pair-support center-cover witness visit count overflows size_t");
      // mu(C,Q) is the exact minimum, over doubled query points y in 2Q,
      // of max_{s in C} ||y-s||^2.  The Cartesian maximum separates by
      // axis and its minimizer is the cell midpoint clamped to 2Q.
      // Equality cannot satisfy the retained strict predicate.
      if (!dyadic_less(
              minimum_doubled_center_query_squared_distance(
                  cell, query_box),
              minimum_squared_distance)) {
        continue;
      }
      const bool overlaps_support =
          node_range_intersection_size(query, first_support) != 0U ||
          node_range_intersection_size(query, second_support) != 0U;
      if (!overlaps_support &&
          query_box_strictly_inside_every_cell_ball(
              cell,
              minimum_squared_distance,
              query_box)) {
        const std::size_t point_count =
            query.leaf_end - query.leaf_begin;
        result.certificate.witness_point_count = checked_add(
            result.certificate.witness_point_count,
            point_count,
            "the pair-support center-cover witness count overflows size_t");
        result.certificate.witness_receipts.push_back(
            make_witness_entry(query_node_index));
        audit.strict_witness_subtree_count = checked_add(
            audit.strict_witness_subtree_count,
            1U,
            "the pair-support center-cover strict-subtree count overflows size_t");
        audit.strict_witness_point_count = checked_add(
            audit.strict_witness_point_count,
            point_count,
            "the pair-support center-cover strict-point count overflows size_t");
        if (result.certificate.witness_point_count >=
            required_strict_witness_count) {
          result.outcome = CenterCoverCellSearchOutcome::certified;
          return result;
        }
        continue;
      }
      if (query.is_leaf()) {
        continue;
      }
      if (!can_add_within(frontier.size(), 2U, auxiliary_cap)) {
        result.outcome =
            CenterCoverCellSearchOutcome::resource_cap_reached;
        return result;
      }
      frontier.push_back(query.right_child);
      frontier.push_back(query.left_child);
    }
    result.outcome =
        CenterCoverCellSearchOutcome::insufficient_closed_rank;
    return result;
  }

  [[nodiscard]] ExactCenterCoverAttemptResult
  build_center_cover_rank_certificate(
      const ExactPairSupportFrontierEntry& product,
      std::size_t maximum_attempt_work_unit_count) const {
    ExactCenterCoverAttemptResult result;
    if (maximum_attempt_work_unit_count == 0U ||
        maximum_attempt_work_unit_count >
            pair_support_center_cover_atomic_work_unit_count) {
      throw std::logic_error(
          "a pair-support center cover has an invalid local work cap");
    }
    const auto [first_node_index, second_node_index] =
        entry_nodes(product);
    if (first_node_index == second_node_index) {
      throw std::logic_error(
          "a pair-support center cover requires a cross product");
    }
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (node_range_intersection_size(first, second) != 0U) {
      throw std::logic_error(
          "a pair-support center cover requires disjoint Morton supports");
    }
    const spatial::ExactDyadicAabb3 first_box =
        node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box =
        node_box(second_node_index);
    const UnnormalizedExactDyadic
        minimum_support_pair_squared_distance =
            minimum_support_squared_distance(first_box, second_box);
    std::vector<ExactDoubledCenterCell> frontier;
    frontier.reserve(
        pair_support_center_cover_maximum_cell_visit_count);
    frontier.push_back(
        make_doubled_center_cell(first_box, second_box));
    result.certificate_cells.reserve(
        pair_support_center_cover_maximum_cell_visit_count);
    const std::size_t required_strict_witness_count =
        result_.requirements.maximum_relevant_closed_rank - 1U;
    while (!frontier.empty()) {
      const std::size_t attempt_work_unit_count = checked_add(
          result.audit.cell_visit_count,
          result.audit.witness_node_visit_count,
          "the pair-support center-cover attempt work overflows size_t");
      if (result.audit.cell_visit_count >=
              pair_support_center_cover_maximum_cell_visit_count ||
          attempt_work_unit_count >=
              maximum_attempt_work_unit_count) {
        result.certificate_cells.clear();
        return result;
      }
      ExactDoubledCenterCell cell = std::move(frontier.back());
      frontier.pop_back();
      result.audit.cell_visit_count = checked_add(
          result.audit.cell_visit_count,
          1U,
          "the pair-support center-cover cell count overflows size_t");
      const UnnormalizedExactDyadic minimum_squared_distance =
          doubled_center_cell_chord_squared_lower_bound(
              cell,
              first_box,
              second_box,
              minimum_support_pair_squared_distance);
      // rho(C)=sum_i ((upper_i-lower_i)/2)^2 is the unconstrained
      // minimum of max_{s in C} ||y-s||^2.  If rho >= L(C), the strict
      // common core is empty and no LBVH traversal can certify this cell.
      if (dyadic_less(
              minimum_doubled_center_core_squared_distance(cell),
              minimum_squared_distance)) {
        CenterCoverCellSearchResult cell_result =
            search_center_cover_cell(
                cell,
                minimum_squared_distance,
                first,
                second,
                required_strict_witness_count,
                maximum_attempt_work_unit_count,
                result.audit);
        if (cell_result.outcome ==
            CenterCoverCellSearchOutcome::certified) {
          result.audit.certified_cell_count = checked_add(
              result.audit.certified_cell_count,
              1U,
              "the pair-support certified center-cell count overflows size_t");
          result.certificate_cells.push_back(
              std::move(cell_result.certificate));
          continue;
        }
        if (cell_result.outcome ==
            CenterCoverCellSearchOutcome::resource_cap_reached) {
          result.certificate_cells.clear();
          return result;
        }
      }
      ExactDoubledCenterCell left;
      ExactDoubledCenterCell right;
      if (!split_doubled_center_cell(cell, left, right) ||
          !can_add_within(
              checked_add(
                  result.audit.cell_visit_count,
                  frontier.size(),
                  "the pair-support center-cover live-cell count overflows size_t"),
              2U,
              pair_support_center_cover_maximum_cell_visit_count)) {
        result.certificate_cells.clear();
        return result;
      }
      result.audit.cell_split_count = checked_add(
          result.audit.cell_split_count,
          1U,
          "the pair-support center-cover split count overflows size_t");
      frontier.push_back(std::move(right));
      frontier.push_back(std::move(left));
    }
    if (result.certificate_cells.empty() ||
        result.audit.certified_cell_count !=
            result.certificate_cells.size()) {
      throw std::logic_error(
          "a pair-support center cover omitted its certified cells");
    }
    result.rank_pruned = true;
    return result;
  }

  [[nodiscard]] CenterCoverRankPruneOutcome
  try_atomic_center_cover_rank_prune(
      const ExactPairSupportFrontierEntry& product) {
    // No closed rank can exceed n.  Avoid both an overflowing n+1 threshold
    // and a geometrically impossible attempt at the terminal requested rank.
    if (result_.requirements.maximum_relevant_closed_rank >=
        cloud_.size()) {
      return CenterCoverRankPruneOutcome::not_pruned;
    }
    // The center-cover is fail-open and performance-only.  Its persistent
    // token bucket grants two initial atomic envelopes, then refills at one
    // work unit per 16 historical non-center units.  The local cap is the
    // smaller of one atomic envelope and the exact remaining credit.
    // Deriving both values exclusively from cumulative audited work makes
    // the decision invariant under chunking and checkpoint resume.
    const std::size_t historical_non_center_work =
        historical_non_center_work_unit_count(result_.audit);
    const std::size_t maximum_cumulative_center_cover_work =
        center_cover_post_attempt_work_limit(
            historical_non_center_work);
    if (result_.audit.center_cover_work_unit_count >=
        maximum_cumulative_center_cover_work) {
      result_.audit.center_cover_work_preflight_skip_count = checked_add(
          result_.audit.center_cover_work_preflight_skip_count,
          1U,
          "the pair-support center-cover token-bucket skip count overflows size_t");
      return CenterCoverRankPruneOutcome::not_pruned;
    }
    const std::size_t remaining_center_cover_work =
        maximum_cumulative_center_cover_work -
        result_.audit.center_cover_work_unit_count;
    const std::size_t local_attempt_work_unit_count =
        std::min(
            pair_support_center_cover_atomic_work_unit_count,
            remaining_center_cover_work);
    if (result_.budget.maximum_work_unit_count <
        pair_support_center_cover_atomic_work_unit_count) {
      result_.audit.center_cover_work_preflight_skip_count = checked_add(
          result_.audit.center_cover_work_preflight_skip_count,
          1U,
          "the pair-support center-cover preflight skip count overflows size_t");
      return CenterCoverRankPruneOutcome::not_pruned;
    }
    if (!can_consume_work_units(
            local_attempt_work_unit_count)) {
      stop(ExactPairSupportStopReason::work_unit_limit);
      return CenterCoverRankPruneOutcome::budget_exhausted;
    }

    ExactCenterCoverAttemptResult attempt =
        build_center_cover_rank_certificate(
            product, local_attempt_work_unit_count);
    const std::size_t exact_work_unit_count = checked_add(
        attempt.audit.cell_visit_count,
        attempt.audit.witness_node_visit_count,
        "the pair-support center-cover work count overflows size_t");
    if (exact_work_unit_count == 0U ||
        exact_work_unit_count >
            local_attempt_work_unit_count ||
        !consume_work_units(exact_work_unit_count)) {
      throw std::logic_error(
          "a preflighted pair-support center cover escaped its atomic work envelope");
    }
    result_.audit.center_cover_attempt_count = checked_add(
        result_.audit.center_cover_attempt_count,
        1U,
        "the pair-support center-cover attempt count overflows size_t");
    result_.audit.center_cover_work_unit_count = checked_add(
        result_.audit.center_cover_work_unit_count,
        exact_work_unit_count,
        "the pair-support center-cover cumulative work count overflows size_t");
#define MORSEHGP3D_ADD_CENTER_COVER_AUDIT(field) \
    result_.audit.center_cover_##field = checked_add( \
        result_.audit.center_cover_##field, \
        attempt.audit.field, \
        "the pair-support center-cover audit count overflows size_t")
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(cell_visit_count);
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(cell_split_count);
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(certified_cell_count);
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(witness_node_visit_count);
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(strict_witness_subtree_count);
    MORSEHGP3D_ADD_CENTER_COVER_AUDIT(strict_witness_point_count);
#undef MORSEHGP3D_ADD_CENTER_COVER_AUDIT
    if (attempt.rank_pruned) {
      result_.audit.center_cover_pruned_product_count = checked_add(
          result_.audit.center_cover_pruned_product_count,
          1U,
          "the pair-support center-cover prune count overflows size_t");
      return CenterCoverRankPruneOutcome::pruned;
    }
    result_.audit.center_cover_inconclusive_count = checked_add(
        result_.audit.center_cover_inconclusive_count,
        1U,
        "the pair-support inconclusive center-cover count overflows size_t");
    return CenterCoverRankPruneOutcome::not_pruned;
  }

  [[nodiscard]] bool proposal_product_is_eligible(
      const ExactPairSupportFrontierEntry& product,
      std::size_t witness_threshold) const {
    const auto [first_node_index, second_node_index] =
        entry_nodes(product);
    if (first_node_index == second_node_index) {
      return false;
    }
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first.is_leaf() && second.is_leaf()) {
      return false;
    }
    const std::size_t excluded_count = checked_add(
        first.leaf_end - first.leaf_begin,
        second.leaf_end - second.leaf_begin,
        "pair-support proposal exclusion count overflows size_t");
    return excluded_count <= cloud_.size() &&
           cloud_.size() - excluded_count >= witness_threshold;
  }

  void request_rank_prune_proposal_batch(
      const ExactPairSupportFrontierEntry& active_product,
      std::size_t witness_threshold) {
    if (proposal_callback_ == nullptr ||
        maximum_products_per_batch_ == 0U) {
      throw std::logic_error(
          "a pair-support proposal batch has no callback contract");
    }
    if (proposal_batch_active_) {
      proposal_audit_.cache_replacement_count = checked_add(
          proposal_audit_.cache_replacement_count,
          1U,
          "the pair-support proposal cache replacement count overflows size_t");
    }
    proposal_batch_products_.clear();
    proposal_cache_.clear();
    keep_certificate_cache_.clear();

    std::size_t inspected_count = 0U;
    for (auto iterator = frontier_.rbegin();
         iterator != frontier_.rend() &&
         inspected_count < maximum_products_per_batch_;
         ++iterator, ++inspected_count) {
      if (proposal_product_is_eligible(*iterator, witness_threshold)) {
        proposal_batch_products_.push_back(*iterator);
      }
    }
    std::sort(
        proposal_batch_products_.begin(),
        proposal_batch_products_.end(),
        frontier_entry_less);
    if (proposal_batch_products_.empty() ||
        !std::binary_search(
            proposal_batch_products_.begin(),
            proposal_batch_products_.end(),
            active_product,
            frontier_entry_less)) {
      throw std::logic_error(
          "a pair-support proposal batch omitted its active product");
    }
    for (std::size_t index = 1U;
         index < proposal_batch_products_.size();
         ++index) {
      if (!frontier_entry_less(
              proposal_batch_products_[index - 1U],
              proposal_batch_products_[index])) {
        throw std::logic_error(
            "a pair-support proposal request contains duplicate products");
      }
    }

    proposal_audit_.batch_request_count = checked_add(
        proposal_audit_.batch_request_count,
        1U,
        "the pair-support proposal batch-request count overflows size_t");
    proposal_audit_.requested_product_count = checked_add(
        proposal_audit_.requested_product_count,
        proposal_batch_products_.size(),
        "the pair-support proposal requested-product count overflows size_t");
    proposal_audit_.maximum_requested_batch_product_count = std::max(
        proposal_audit_.maximum_requested_batch_product_count,
        proposal_batch_products_.size());

    ExactPairSupportRankPruneBatchProposal response =
        (*proposal_callback_)(ExactPairSupportRankPruneBatchRequest{
            std::span<const ExactPairSupportFrontierEntry>{
                proposal_batch_products_},
            witness_threshold});
    if (response.proposals.size() > proposal_batch_products_.size() ||
        response.keep_certificates.size() >
            proposal_batch_products_.size() ||
        response.proposals.size() >
            proposal_batch_products_.size() -
                response.keep_certificates.size()) {
      throw std::invalid_argument(
          "a pair-support proposal batch exceeds its request");
    }
    const auto require_requested_product =
        [this](const ExactPairSupportFrontierEntry& product) {
          const auto requested = std::lower_bound(
              proposal_batch_products_.begin(),
              proposal_batch_products_.end(),
              product,
              frontier_entry_less);
          if (requested == proposal_batch_products_.end() ||
              *requested != product) {
            throw std::invalid_argument(
                "a pair-support proposal references an unrequested product");
          }
        };
    for (std::size_t index = 0U;
         index < response.proposals.size();
         ++index) {
      const ExactPairSupportRankPruneProposal& proposal =
          response.proposals[index];
      if (index != 0U &&
          !frontier_entry_less(
              response.proposals[index - 1U].product,
              proposal.product)) {
        throw std::invalid_argument(
            "pair-support proposals must be strictly canonical");
      }
      require_requested_product(proposal.product);
      if (proposal.strict_witness_receipts.empty() ||
          proposal.strict_witness_receipts.size() > witness_threshold) {
        throw std::invalid_argument(
            "a pair-support proposal has an invalid receipt count");
      }
    }
    for (std::size_t index = 0U;
         index < response.keep_certificates.size();
         ++index) {
      const ExactPairSupportRankKeepCertificate& certificate =
          response.keep_certificates[index];
      if (index != 0U &&
          !frontier_entry_less(
              response.keep_certificates[index - 1U].product,
              certificate.product)) {
        throw std::invalid_argument(
            "pair-support keep certificates must be strictly canonical");
      }
      require_requested_product(certificate.product);
    }
    std::size_t prune_index = 0U;
    std::size_t keep_index = 0U;
    while (prune_index < response.proposals.size() &&
           keep_index < response.keep_certificates.size()) {
      const ExactPairSupportFrontierEntry& prune_product =
          response.proposals[prune_index].product;
      const ExactPairSupportFrontierEntry& keep_product =
          response.keep_certificates[keep_index].product;
      if (prune_product == keep_product) {
        throw std::invalid_argument(
            "a pair-support product occurs in both proposal caches");
      }
      if (frontier_entry_less(prune_product, keep_product)) {
        ++prune_index;
      } else {
        ++keep_index;
      }
    }
    proposal_cache_ = std::move(response.proposals);
    keep_certificate_cache_ =
        std::move(response.keep_certificates);
    proposal_batch_active_ = true;
  }

  [[nodiscard]] std::optional<RankSearchOutcome>
  consume_rank_prune_proposal(
      ExactPairSupportRankPruneProposal proposal,
      const ExactPairSupportFrontierEntry& active_product,
      std::size_t witness_threshold) {
    const std::size_t receipt_count =
        proposal.strict_witness_receipts.size();
    if (!can_consume_work_units(receipt_count)) {
      // An unaffordable hint is never consumed or geometrically replayed.
      // Historical CPU fallback preserves both the chunk cursor and a
      // consumed-only transcript without hidden exact work.
      proposal_audit_.cpu_fallback_product_count = checked_add(
          proposal_audit_.cpu_fallback_product_count,
          1U,
          "the pair-support proposal fallback count overflows size_t");
      return std::nullopt;
    }
    const auto [first_node_index, second_node_index] =
        entry_nodes(active_product);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    const spatial::ExactDyadicAabb3 first_box =
        node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box =
        node_box(second_node_index);
    std::size_t strict_witness_point_count = 0U;
    for (std::size_t receipt_index = 0U;
         receipt_index < proposal.strict_witness_receipts.size();
         ++receipt_index) {
      const ExactPairSupportWitnessNodeEntry& receipt =
          proposal.strict_witness_receipts[receipt_index];
      if (receipt_index != 0U &&
          (!witness_entry_less(
               proposal.strict_witness_receipts[receipt_index - 1U],
               receipt) ||
           proposal.strict_witness_receipts[receipt_index - 1U].leaf_end >
               receipt.leaf_begin)) {
        throw std::invalid_argument(
            "pair-support proposal receipts are not a canonical antichain");
      }
      std::size_t witness_node = 0U;
      try {
        witness_node = witness_node_index(receipt);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            "a pair-support proposal receipt contradicts the LBVH");
      }
      const Node& witness = node(witness_node);
      if (node_range_intersects(witness, first) ||
          node_range_intersects(witness, second)) {
        throw std::invalid_argument(
            "a pair-support proposal receipt intersects its support");
      }
      if (exact_diametral_phi_aabb_maximum_sign(
              first_box, second_box, node_box(witness_node)) >= 0) {
        throw std::invalid_argument(
            "a pair-support proposal receipt is not strictly interior");
      }
      if (strict_witness_point_count >= witness_threshold) {
        throw std::invalid_argument(
            "a pair-support proposal contains a nonminimal receipt suffix");
      }
      strict_witness_point_count = checked_add(
          strict_witness_point_count,
          witness.leaf_end - witness.leaf_begin,
          "the pair-support proposal witness count overflows size_t");
    }
    if (strict_witness_point_count < witness_threshold) {
      throw std::invalid_argument(
          "a pair-support proposal does not certify the rank threshold");
    }

    if (!consume_work_units(receipt_count)) {
      throw std::logic_error(
          "an affordable pair-support proposal failed its work preflight");
    }

    result_.audit.rank_prune_search_count = checked_add(
        result_.audit.rank_prune_search_count,
        1U,
        "the pair-support proposal rank-search count overflows size_t");
    result_.audit.witness_node_visit_count = checked_add(
        result_.audit.witness_node_visit_count,
        receipt_count,
        "the pair-support proposal witness-node count overflows size_t");
    result_.audit.exact_phi_aabb_bound_count = checked_add(
        result_.audit.exact_phi_aabb_bound_count,
        receipt_count,
        "the pair-support proposal phi-bound count overflows size_t");
    result_.audit.strict_interior_witness_subtree_count = checked_add(
        result_.audit.strict_interior_witness_subtree_count,
        receipt_count,
        "the pair-support proposal witness-subtree count overflows size_t");
    result_.audit.strict_interior_witness_point_count = checked_add(
        result_.audit.strict_interior_witness_point_count,
        strict_witness_point_count,
        "the pair-support proposal witness-point count overflows size_t");
    result_.audit.maximum_witness_frontier_entry_count = std::max(
        result_.audit.maximum_witness_frontier_entry_count,
        receipt_count);

    proposal_audit_.consumed_proposal_count = checked_add(
        proposal_audit_.consumed_proposal_count,
        1U,
        "the pair-support consumed-proposal count overflows size_t");
    proposal_audit_.exact_receipt_recertification_count = checked_add(
        proposal_audit_.exact_receipt_recertification_count,
        receipt_count,
        "the pair-support exact-recertification count overflows size_t");
    proposal_audit_.consumed_receipt_count = checked_add(
        proposal_audit_.consumed_receipt_count,
        receipt_count,
        "the pair-support consumed-receipt count overflows size_t");
    proposal_audit_.consumed_witness_point_count = checked_add(
        proposal_audit_.consumed_witness_point_count,
        strict_witness_point_count,
        "the pair-support consumed-witness point count overflows size_t");
    proposal_audit_.proposal_pruned_pair_count = checked_add(
        proposal_audit_.proposal_pruned_pair_count,
        entry_pair_count(active_product),
        "the pair-support proposal-pruned pair count overflows size_t");
    consumed_rank_prune_proposals_.push_back(std::move(proposal));
    return RankSearchOutcome::prune;
  }

  [[nodiscard]] std::optional<RankSearchOutcome>
  consume_rank_keep_certificate(
      ExactPairSupportRankKeepCertificate certificate,
      const ExactPairSupportFrontierEntry& active_product,
      std::size_t witness_threshold) {
    const std::size_t terminal_count =
        certificate.canonical_terminals.size();
    if (!can_consume_work_units(terminal_count)) {
      // As for a prune hint, affordability is decided before LBVH
      // authentication or exact geometry.  The historical path then owns the
      // unchanged work budget.
      proposal_audit_.cpu_fallback_product_count = checked_add(
          proposal_audit_.cpu_fallback_product_count,
          1U,
          "the pair-support proposal fallback count overflows size_t");
      return std::nullopt;
    }
    if (terminal_count == 0U || terminal_count > cloud_.size()) {
      throw std::invalid_argument(
          "a pair-support keep certificate has an invalid terminal count");
    }

    const auto [first_node_index, second_node_index] =
        entry_nodes(active_product);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    const Node& root = node(index_.root_index_);

    struct CoverageInterval {
      std::size_t leaf_begin{};
      std::size_t leaf_end{};
    };
    std::vector<CoverageInterval> coverage;
    coverage.reserve(terminal_count + 2U);
    coverage.push_back(
        CoverageInterval{first.leaf_begin, first.leaf_end});
    coverage.push_back(
        CoverageInterval{second.leaf_begin, second.leaf_end});
    std::vector<std::size_t> terminal_node_indices;
    terminal_node_indices.reserve(terminal_count);
    for (std::size_t terminal_index = 0U;
         terminal_index < terminal_count;
         ++terminal_index) {
      const ExactPairSupportWitnessNodeEntry& terminal =
          certificate.canonical_terminals[terminal_index];
      if (terminal_index != 0U &&
          (!witness_entry_less(
               certificate.canonical_terminals[terminal_index - 1U],
               terminal) ||
           certificate.canonical_terminals[terminal_index - 1U].leaf_end >
               terminal.leaf_begin)) {
        throw std::invalid_argument(
            "pair-support keep terminals are not a canonical antichain");
      }
      std::size_t terminal_node_index = 0U;
      try {
        terminal_node_index = witness_node_index(terminal);
      } catch (const std::exception&) {
        throw std::invalid_argument(
            "a pair-support keep terminal contradicts the LBVH");
      }
      const Node& terminal_node = node(terminal_node_index);
      if (node_range_intersects(terminal_node, first) ||
          node_range_intersects(terminal_node, second)) {
        throw std::invalid_argument(
            "a pair-support keep terminal intersects its support");
      }
      terminal_node_indices.push_back(terminal_node_index);
      coverage.push_back(
          CoverageInterval{
              terminal_node.leaf_begin, terminal_node.leaf_end});
    }
    std::sort(
        coverage.begin(),
        coverage.end(),
        [](const CoverageInterval& left,
           const CoverageInterval& right) {
          if (left.leaf_begin != right.leaf_begin) {
            return left.leaf_begin < right.leaf_begin;
          }
          return left.leaf_end < right.leaf_end;
        });
    std::size_t coverage_cursor = root.leaf_begin;
    for (const CoverageInterval& interval : coverage) {
      if (interval.leaf_begin != coverage_cursor ||
          interval.leaf_begin >= interval.leaf_end ||
          interval.leaf_end > root.leaf_end) {
        throw std::invalid_argument(
            "a pair-support keep certificate does not exactly tile the LBVH root");
      }
      coverage_cursor = interval.leaf_end;
    }
    if (coverage_cursor != root.leaf_end) {
      throw std::invalid_argument(
          "a pair-support keep certificate leaves a gap in the LBVH root");
    }

    const spatial::ExactDyadicAabb3 first_box =
        node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box =
        node_box(second_node_index);
    const PointId first_anchor_id =
        index_.leaves_[first.leaf_begin].point_id;
    const PointId second_anchor_id =
        index_.leaves_[second.leaf_begin].point_id;
    const ExactDyadicAnchorPhi anchor = exact_dyadic_anchor_phi(
        cloud_.point(first_anchor_id), cloud_.point(second_anchor_id));

    std::size_t strict_terminal_count = 0U;
    std::size_t strict_witness_point_count = 0U;
    std::size_t anchor_terminal_count = 0U;
    std::size_t anchor_point_count = 0U;
    std::size_t anchor_tangent_count = 0U;
    std::size_t external_leaf_terminal_count = 0U;
    for (const std::size_t terminal_node_index :
         terminal_node_indices) {
      const Node& terminal_node = node(terminal_node_index);
      if (exact_diametral_phi_aabb_maximum_sign(
              first_box,
              second_box,
              node_box(terminal_node_index)) < 0) {
        strict_terminal_count = checked_add(
            strict_terminal_count,
            1U,
            "the pair-support keep strict-terminal count overflows size_t");
        strict_witness_point_count = checked_add(
            strict_witness_point_count,
            terminal_node.leaf_end - terminal_node.leaf_begin,
            "the pair-support keep strict-point count overflows size_t");
        continue;
      }
      const int anchor_minimum_sign =
          exact_anchor_phi_aabb_minimum_sign(
              anchor, node_box(terminal_node_index));
      if (anchor_minimum_sign >= 0) {
        anchor_terminal_count = checked_add(
            anchor_terminal_count,
            1U,
            "the pair-support keep anchor-terminal count overflows size_t");
        anchor_point_count = checked_add(
            anchor_point_count,
            terminal_node.leaf_end - terminal_node.leaf_begin,
            "the pair-support keep anchor-point count overflows size_t");
        if (anchor_minimum_sign == 0) {
          anchor_tangent_count = checked_add(
              anchor_tangent_count,
              1U,
              "the pair-support keep anchor-tangent count overflows size_t");
        }
        continue;
      }
      if (!terminal_node.is_leaf()) {
        throw std::invalid_argument(
            "a pair-support keep terminal is an unresolved internal node");
      }
      external_leaf_terminal_count = checked_add(
          external_leaf_terminal_count,
          1U,
          "the pair-support keep external-leaf count overflows size_t");
    }
    if (strict_witness_point_count >= witness_threshold) {
      throw std::invalid_argument(
          "a pair-support keep certificate reaches the rank threshold");
    }

    if (!consume_work_units(terminal_count)) {
      throw std::logic_error(
          "an affordable pair-support keep certificate failed its work preflight");
    }
    const std::size_t nonstrict_terminal_count =
        terminal_count - strict_terminal_count;

    result_.audit.rank_prune_search_count = checked_add(
        result_.audit.rank_prune_search_count,
        1U,
        "the pair-support keep rank-search count overflows size_t");
    result_.audit.witness_node_visit_count = checked_add(
        result_.audit.witness_node_visit_count,
        terminal_count,
        "the pair-support keep witness-node count overflows size_t");
    result_.audit.exact_phi_aabb_bound_count = checked_add(
        result_.audit.exact_phi_aabb_bound_count,
        terminal_count,
        "the pair-support keep phi-bound count overflows size_t");
    result_.audit.exact_anchor_ball_minimum_aabb_bound_count = checked_add(
        result_.audit.exact_anchor_ball_minimum_aabb_bound_count,
        nonstrict_terminal_count,
        "the pair-support keep anchor-bound count overflows size_t");
    result_.audit.strict_interior_witness_subtree_count = checked_add(
        result_.audit.strict_interior_witness_subtree_count,
        strict_terminal_count,
        "the pair-support keep strict-subtree count overflows size_t");
    result_.audit.strict_interior_witness_point_count = checked_add(
        result_.audit.strict_interior_witness_point_count,
        strict_witness_point_count,
        "the pair-support keep strict-point count overflows size_t");
    result_.audit.certified_anchor_noninterior_subtree_count = checked_add(
        result_.audit.certified_anchor_noninterior_subtree_count,
        anchor_terminal_count,
        "the pair-support keep anchor-subtree count overflows size_t");
    result_.audit.certified_anchor_noninterior_point_count = checked_add(
        result_.audit.certified_anchor_noninterior_point_count,
        anchor_point_count,
        "the pair-support keep anchor-point count overflows size_t");
    result_.audit.certified_anchor_shell_tangent_subtree_count = checked_add(
        result_.audit.certified_anchor_shell_tangent_subtree_count,
        anchor_tangent_count,
        "the pair-support keep anchor-tangent count overflows size_t");
    result_.audit.equality_or_positive_bound_descent_count = checked_add(
        result_.audit.equality_or_positive_bound_descent_count,
        external_leaf_terminal_count,
        "the pair-support keep leaf-classification count overflows size_t");
    result_.audit.maximum_witness_frontier_entry_count = std::max(
        result_.audit.maximum_witness_frontier_entry_count,
        terminal_count);

    proposal_audit_.consumed_keep_certificate_count = checked_add(
        proposal_audit_.consumed_keep_certificate_count,
        1U,
        "the pair-support consumed-keep count overflows size_t");
    proposal_audit_.exact_keep_terminal_recertification_count = checked_add(
        proposal_audit_.exact_keep_terminal_recertification_count,
        terminal_count,
        "the pair-support keep recertification count overflows size_t");
    proposal_audit_.keep_terminal_classification_count = checked_add(
        proposal_audit_.keep_terminal_classification_count,
        terminal_count,
        "the pair-support keep classification count overflows size_t");
    proposal_audit_.keep_strict_interior_terminal_count = checked_add(
        proposal_audit_.keep_strict_interior_terminal_count,
        strict_terminal_count,
        "the pair-support keep strict-terminal audit overflows size_t");
    proposal_audit_.keep_strict_interior_point_count = checked_add(
        proposal_audit_.keep_strict_interior_point_count,
        strict_witness_point_count,
        "the pair-support keep strict-point audit overflows size_t");
    proposal_audit_.keep_anchor_noninterior_terminal_count = checked_add(
        proposal_audit_.keep_anchor_noninterior_terminal_count,
        anchor_terminal_count,
        "the pair-support keep anchor-terminal audit overflows size_t");
    proposal_audit_.keep_anchor_noninterior_point_count = checked_add(
        proposal_audit_.keep_anchor_noninterior_point_count,
        anchor_point_count,
        "the pair-support keep anchor-point audit overflows size_t");
    proposal_audit_.keep_external_leaf_terminal_count = checked_add(
        proposal_audit_.keep_external_leaf_terminal_count,
        external_leaf_terminal_count,
        "the pair-support keep external-leaf audit overflows size_t");
    consumed_rank_keep_certificates_.push_back(
        std::move(certificate));
    return RankSearchOutcome::keep;
  }

  [[nodiscard]] std::optional<RankSearchOutcome>
  try_rank_prune_proposal(
      const ExactPairSupportFrontierEntry& active_product,
      std::size_t witness_threshold) {
    if (proposal_callback_ == nullptr) {
      return std::nullopt;
    }
    if (!proposal_batch_active_ ||
        !std::binary_search(
            proposal_batch_products_.begin(),
            proposal_batch_products_.end(),
            active_product,
            frontier_entry_less)) {
      request_rank_prune_proposal_batch(
          active_product, witness_threshold);
    }

    // A historical strict-rank prune remains the first accelerated
    // transition considered.  Batch validation forbids a product from also
    // appearing in the conservative-keep cache.
    auto prune = std::lower_bound(
        proposal_cache_.begin(),
        proposal_cache_.end(),
        active_product,
        [](const ExactPairSupportRankPruneProposal& proposal,
           const ExactPairSupportFrontierEntry& product) {
          return frontier_entry_less(proposal.product, product);
        });
    if (prune != proposal_cache_.end() &&
        prune->product == active_product) {
      ExactPairSupportRankPruneProposal proposal =
          std::move(*prune);
      proposal_cache_.erase(prune);
      return consume_rank_prune_proposal(
          std::move(proposal), active_product, witness_threshold);
    }

    auto keep = std::lower_bound(
        keep_certificate_cache_.begin(),
        keep_certificate_cache_.end(),
        active_product,
        [](const ExactPairSupportRankKeepCertificate& certificate,
           const ExactPairSupportFrontierEntry& product) {
          return frontier_entry_less(certificate.product, product);
        });
    if (keep != keep_certificate_cache_.end() &&
        keep->product == active_product) {
      ExactPairSupportRankKeepCertificate certificate =
          std::move(*keep);
      keep_certificate_cache_.erase(keep);
      return consume_rank_keep_certificate(
          std::move(certificate), active_product, witness_threshold);
    }

    proposal_audit_.cpu_fallback_product_count = checked_add(
        proposal_audit_.cpu_fallback_product_count,
        1U,
        "the pair-support proposal fallback count overflows size_t");
    return std::nullopt;
  }

  [[nodiscard]] RankSearchOutcome continue_rank_prune_search() {
    if (!pending_product_.has_value() ||
        pending_product_->stage != ExactPairSupportPendingStage::rank_search ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a pair-support rank search has no matching active product");
    }
    ExactPairSupportPendingProduct& pending = *pending_product_;
    const auto [first_node_index, second_node_index] =
        entry_nodes(pending.product);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    const std::size_t excluded_count = first_node_index == second_node_index
                                           ? first.leaf_end - first.leaf_begin
                                           : checked_add(
                                                 first.leaf_end - first.leaf_begin,
                                                 second.leaf_end - second.leaf_begin,
                                                 "pair-support exclusion count overflows size_t");
    if (excluded_count > cloud_.size()) {
      throw std::logic_error(
          "pair-support ranges exclude more points than the cloud contains");
    }
    const std::size_t witness_threshold =
        result_.requirements.maximum_relevant_closed_rank - 1U;
    if (cloud_.size() - excluded_count < witness_threshold) {
      return RankSearchOutcome::keep;
    }
    // A strict universal-rank certificate needs actual cloud points inside
    // every diametral ball represented by this product.  If even the
    // continuous AABB relaxation has an empty common strict core, its global
    // witness traversal is provably futile.  Returning keep here is
    // deliberately fail-open: the independent center-cover proof is still
    // attempted by the caller before exhaustive product expansion.
    if (!pending.rank_search_started &&
        exact_diametral_phi_continuous_core_minimum_sign(
            node_box(first_node_index),
            node_box(second_node_index)) >= 0) {
      return RankSearchOutcome::keep;
    }
    if (!pending.rank_search_started) {
      const std::optional<RankSearchOutcome> proposed =
          try_rank_prune_proposal(
              pending.product, witness_threshold);
      if (proposed.has_value()) {
        return *proposed;
      }
      if (result_.budget.maximum_auxiliary_frontier_entry_count == 0U) {
        stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
        return RankSearchOutcome::budget_exhausted;
      }
      pending.rank_search_started = true;
      pending.witness_frontier.push_back(
          make_witness_entry(index_.root_index_));
      result_.audit.rank_prune_search_count = checked_add(
          result_.audit.rank_prune_search_count,
          1U,
          "the pair-support rank-search count overflows size_t");
      result_.audit.maximum_witness_frontier_entry_count = std::max(
          result_.audit.maximum_witness_frontier_entry_count,
          pending.witness_frontier.size());
    }
    const spatial::ExactDyadicAabb3 first_box = node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box = node_box(second_node_index);
    const PointId first_anchor_id =
        index_.leaves_[first.leaf_begin].point_id;
    const PointId second_anchor_id =
        index_.leaves_[second.leaf_begin].point_id;
    const ExactDyadicAnchorPhi anchor = exact_dyadic_anchor_phi(
        cloud_.point(first_anchor_id), cloud_.point(second_anchor_id));
    while (!pending.witness_frontier.empty() ||
           pending.deferred_expansion_node.has_value()) {
      if (pending.deferred_expansion_node.has_value()) {
        if (!can_add_within(
                pending.witness_frontier.size(),
                2U,
                result_.budget.maximum_auxiliary_frontier_entry_count)) {
          stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
          return RankSearchOutcome::budget_exhausted;
        }
        const std::size_t deferred_index = witness_node_index(
            *pending.deferred_expansion_node);
        const Node& deferred = node(deferred_index);
        if (deferred.is_leaf()) {
          throw std::logic_error(
              "a deferred witness expansion references a leaf");
        }
        pending.witness_frontier.push_back(
            make_witness_entry(deferred.right_child));
        pending.witness_frontier.push_back(
            make_witness_entry(deferred.left_child));
        pending.deferred_expansion_node.reset();
        result_.audit.maximum_witness_frontier_entry_count = std::max(
            result_.audit.maximum_witness_frontier_entry_count,
            pending.witness_frontier.size());
        continue;
      }
      if (!consume_work_unit()) {
        return RankSearchOutcome::budget_exhausted;
      }
      const ExactPairSupportWitnessNodeEntry query_entry =
          pending.witness_frontier.back();
      pending.witness_frontier.pop_back();
      const std::size_t query_node_index = witness_node_index(query_entry);
      result_.audit.witness_node_visit_count = checked_add(
          result_.audit.witness_node_visit_count,
          1U,
          "the pair-support witness-node count overflows size_t");
      const Node& query_node = node(query_node_index);
      const bool overlaps_support =
          node_range_intersects(query_node, first) ||
          (first_node_index != second_node_index &&
           node_range_intersects(query_node, second));
      if (!overlaps_support) {
        const int maximum_sign =
            exact_diametral_phi_aabb_maximum_sign(
                first_box,
                second_box,
                node_box(query_node_index));
        result_.audit.exact_phi_aabb_bound_count = checked_add(
            result_.audit.exact_phi_aabb_bound_count,
            1U,
            "the pair-support phi-bound count overflows size_t");
        if (maximum_sign < 0) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          pending.strict_witness_point_count = checked_add(
              pending.strict_witness_point_count,
              subtree_size,
              "the pair-support witness count overflows size_t");
          pending.strict_witness_receipts.push_back(query_entry);
          result_.audit.strict_interior_witness_subtree_count = checked_add(
              result_.audit.strict_interior_witness_subtree_count,
              1U,
              "the pair-support witness-subtree count overflows size_t");
          result_.audit.strict_interior_witness_point_count = checked_add(
              result_.audit.strict_interior_witness_point_count,
              subtree_size,
              "the pair-support witness-point audit overflows size_t");
          if (pending.strict_witness_point_count >= witness_threshold) {
            return RankSearchOutcome::prune;
          }
          continue;
        }
        result_.audit.exact_anchor_ball_minimum_aabb_bound_count = checked_add(
            result_.audit.exact_anchor_ball_minimum_aabb_bound_count,
            1U,
            "the pair-support anchor-bound count overflows size_t");
        const int anchor_minimum_sign =
            exact_anchor_phi_aabb_minimum_sign(
                anchor, node_box(query_node_index));
        if (anchor_minimum_sign >= 0) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          if (anchor_minimum_sign == 0) {
            result_.audit.certified_anchor_shell_tangent_subtree_count =
                checked_add(
                    result_.audit
                        .certified_anchor_shell_tangent_subtree_count,
                    1U,
                    "the pair-support anchor-tangent count overflows size_t");
          }
          result_.audit.certified_anchor_noninterior_subtree_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_subtree_count,
                  1U,
                  "the pair-support anchor-subtree count overflows size_t");
          result_.audit.certified_anchor_noninterior_point_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_point_count,
                  subtree_size,
                  "the pair-support anchor-point count overflows size_t");
          continue;
        }
        result_.audit.equality_or_positive_bound_descent_count = checked_add(
            result_.audit.equality_or_positive_bound_descent_count,
            1U,
            "the pair-support nonnegative-bound count overflows size_t");
      }
      if (query_node.is_leaf()) {
        continue;
      }
      if (!can_add_within(
              pending.witness_frontier.size(),
              2U,
              result_.budget.maximum_auxiliary_frontier_entry_count)) {
        pending.deferred_expansion_node = query_entry;
        result_.audit.maximum_witness_frontier_entry_count = std::max(
            result_.audit.maximum_witness_frontier_entry_count,
            checked_add(
                pending.witness_frontier.size(),
                1U,
                "the deferred witness frontier size overflows"));
        stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
        return RankSearchOutcome::budget_exhausted;
      }
      pending.witness_frontier.push_back(
          make_witness_entry(query_node.right_child));
      pending.witness_frontier.push_back(
          make_witness_entry(query_node.left_child));
      result_.audit.maximum_witness_frontier_entry_count = std::max(
          result_.audit.maximum_witness_frontier_entry_count,
          pending.witness_frontier.size());
    }
    return RankSearchOutcome::keep;
  }

  [[nodiscard]] bool leaf_query_preflight() {
    if (consumed_since(
            result_.audit.global_closed_ball_query_count,
            chunk_audit_origin_.global_closed_ball_query_count,
            "the pair-support query audit moved backwards") >=
        result_.budget.maximum_global_closed_ball_query_count) {
      stop(ExactPairSupportStopReason::global_closed_ball_query_limit);
      return false;
    }
    const std::size_t required_closed_ball_frontier = checked_add(
        index_.build_counters().maximum_depth,
        1U,
        "the sparse closed-ball frontier bound overflows size_t");
    if (required_closed_ball_frontier >
        result_.budget.maximum_auxiliary_frontier_entry_count) {
      stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
      return false;
    }
    return true;
  }

  [[nodiscard]] bool leaf_emission_preflight() {
    const std::size_t emitted_record_count = checked_add(
        result_.events.size(),
        result_.relevant_extra_shell_diagnostics.size(),
        "the pair-support emitted-record count overflows size_t");
    if (emitted_record_count >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactPairSupportStopReason::emitted_record_limit);
      return false;
    }
    const std::size_t maximum_record_references = checked_add(
        result_.requirements.maximum_relevant_closed_rank,
        1U,
        "the pair-support record reference bound overflows size_t");
    if (!can_add_within(
            consumed_since(
                result_.audit.emitted_point_id_reference_count,
                chunk_audit_origin_.emitted_point_id_reference_count,
                "the pair-support emitted-reference audit moved backwards"),
            maximum_record_references,
            result_.budget.maximum_emitted_point_id_reference_count)) {
      stop(ExactPairSupportStopReason::emitted_point_id_reference_limit);
      return false;
    }
    return true;
  }

  void add_point_classifications(std::size_t count) {
    result_.audit.point_classification_count = checked_add(
        result_.audit.point_classification_count,
        count,
        "the sparse closed-ball classification count overflows size_t");
  }

  [[nodiscard]] bool consume_closed_ball_node_visit() {
    if (!can_consume_work_units(1U)) {
      stop(ExactPairSupportStopReason::work_unit_limit);
      return false;
    }
    const std::size_t consumed_node_visits = consumed_since(
        result_.audit.closed_ball_node_visit_count,
        chunk_audit_origin_.closed_ball_node_visit_count,
        "the closed-ball node-visit audit moved backwards");
    if (consumed_node_visits >=
        result_.budget.maximum_closed_ball_node_visit_count) {
      stop(ExactPairSupportStopReason::closed_ball_node_visit_limit);
      return false;
    }
    if (!consume_work_unit()) {
      return false;
    }
    result_.audit.closed_ball_node_visit_count = checked_add(
        result_.audit.closed_ball_node_visit_count,
        1U,
        "the sparse closed-ball node count overflows size_t");
    return true;
  }

  [[nodiscard]] SparseBallOutcome continue_sparse_closed_ball(
      const std::array<PointId, 2>& support_ids) {
    if (!pending_product_.has_value() ||
        pending_product_->stage !=
            ExactPairSupportPendingStage::classify_leaf) {
      throw std::logic_error(
          "a sparse closed-ball continuation has no leaf product");
    }
    ExactPairSupportPendingProduct& pending = *pending_product_;
    const std::size_t interior_cap =
        result_.requirements.maximum_relevant_closed_rank - 2U;
    if (!pending.closed_ball.has_value()) {
      if (!leaf_query_preflight()) {
        return SparseBallOutcome::budget_exhausted;
      }
      pending.closed_ball.emplace();
      pending.closed_ball->interior_ids.reserve(interior_cap);
      pending.closed_ball->frontier.push_back(
          make_witness_entry(index_.root_index_));
      result_.audit.global_closed_ball_query_count = checked_add(
          result_.audit.global_closed_ball_query_count,
          1U,
          "the sparse closed-ball query count overflows size_t");
      result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
          result_.audit.maximum_closed_ball_frontier_entry_count,
          pending.closed_ball->frontier.size());
    }
    ExactPairSupportPendingClosedBall& classification =
        *pending.closed_ball;
    if (classification.stage ==
        ExactPairSupportPendingClosedBallStage::ready_to_emit) {
      return SparseBallOutcome::complete;
    }
    if (classification.stage !=
        ExactPairSupportPendingClosedBallStage::traversing) {
      throw std::logic_error(
          "a sparse closed-ball cursor has an invalid stage");
    }
    const ExactDyadicAnchorPhi anchor = exact_dyadic_anchor_phi(
        cloud_.point(support_ids[0]), cloud_.point(support_ids[1]));
    while (!classification.frontier.empty()) {
      const ExactPairSupportWitnessNodeEntry query_entry =
          classification.frontier.back();
      const std::size_t node_index = witness_node_index(query_entry);
      const Node& current = node(node_index);
      // A split pops one entry and pushes two, hence needs one net slot.
      // Conservatively reserve it before charging or mutating the cursor.
      // This makes a resume under a lower auxiliary cap a clean refusal with
      // no repeated physical visit.  A later larger-budget chunk sees the
      // byte-identical scientific cursor.
      if (!current.is_leaf() &&
          !can_add_within(
              classification.frontier.size(),
              1U,
              result_.budget
                  .maximum_auxiliary_frontier_entry_count)) {
        stop(
            ExactPairSupportStopReason::
                auxiliary_frontier_entry_limit);
        return SparseBallOutcome::budget_exhausted;
      }
      if (!consume_closed_ball_node_visit()) {
        return SparseBallOutcome::budget_exhausted;
      }
      classification.node_visit_count = checked_add(
          classification.node_visit_count,
          1U,
          "the pending closed-ball node count overflows size_t");
      classification.frontier.pop_back();
      const std::size_t subtree_size =
          current.leaf_end - current.leaf_begin;
      const spatial::ExactDyadicAabb3 query_box = node_box(node_index);
      const int minimum_phi_sign =
          exact_anchor_phi_aabb_minimum_sign(anchor, query_box);
      result_.audit.exact_closed_ball_minimum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_minimum_aabb_bound_count,
          1U,
          "the sparse closed-ball minimum-bound count overflows size_t");
      if (minimum_phi_sign > 0) {
        classification.exterior_count = checked_add(
            classification.exterior_count,
            subtree_size,
            "the sparse closed-ball exterior count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_subtree_count,
            1U,
            "the sparse closed-ball exterior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_point_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_point_count,
            subtree_size,
            "the sparse closed-ball exterior-point count overflows size_t");
        add_point_classifications(subtree_size);
        continue;
      }
      const int maximum_phi_sign =
          exact_diametral_point_phi_aabb_maximum_sign(
              anchor, query_box);
      result_.audit.exact_closed_ball_maximum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_maximum_aabb_bound_count,
          1U,
          "the sparse closed-ball maximum-bound count overflows size_t");
      if (maximum_phi_sign < 0) {
        const std::size_t interior_count = checked_add(
            classification.interior_ids.size(),
            subtree_size,
            "the sparse closed-ball interior count overflows size_t");
        result_.audit.closed_ball_bulk_interior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_interior_subtree_count,
            1U,
            "the sparse closed-ball interior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_interior_point_count = checked_add(
            result_.audit.closed_ball_bulk_interior_point_count,
            subtree_size,
            "the sparse closed-ball interior-point count overflows size_t");
        add_point_classifications(subtree_size);
        if (interior_count > interior_cap) {
          result_.audit.early_closed_rank_rejection_count = checked_add(
              result_.audit.early_closed_rank_rejection_count,
              1U,
              "the sparse closed-ball early-rejection count overflows size_t");
          return SparseBallOutcome::rank_exceeded;
        }
        for (std::size_t position = current.leaf_begin;
             position < current.leaf_end;
             ++position) {
          classification.interior_ids.push_back(
              index_.leaves_[position].point_id);
        }
        continue;
      }
      if (current.is_leaf()) {
        const PointId point_id =
            index_.leaves_[current.leaf_begin].point_id;
        if (minimum_phi_sign != maximum_phi_sign) {
          throw std::logic_error(
              "a degenerate exact AABB has inconsistent diametral extrema");
        }
        result_.audit.exact_point_distance_evaluation_count = checked_add(
            result_.audit.exact_point_distance_evaluation_count,
            1U,
            "the sparse closed-ball exact leaf-classification count "
            "overflows size_t");
        add_point_classifications(1U);
        if (maximum_phi_sign < 0) {
          const std::size_t interior_count = checked_add(
              classification.interior_ids.size(),
              1U,
              "the sparse closed-ball interior count overflows size_t");
          if (interior_count > interior_cap) {
            result_.audit.early_closed_rank_rejection_count = checked_add(
                result_.audit.early_closed_rank_rejection_count,
                1U,
                "the sparse closed-ball early-rejection count overflows size_t");
            return SparseBallOutcome::rank_exceeded;
          }
          classification.interior_ids.push_back(point_id);
        } else if (maximum_phi_sign == 0) {
          classification.shell_count = checked_add(
              classification.shell_count,
              1U,
              "the sparse closed-ball shell count overflows size_t");
          if (point_id == support_ids[0]) {
            classification.support_seen_mask =
                static_cast<std::uint8_t>(
                    classification.support_seen_mask | 1U);
          } else if (point_id == support_ids[1]) {
            classification.support_seen_mask =
                static_cast<std::uint8_t>(
                    classification.support_seen_mask | 2U);
          } else if (!classification.canonical_extra_shell_witness_id.has_value() ||
                     point_id <
                         *classification.canonical_extra_shell_witness_id) {
            classification.canonical_extra_shell_witness_id = point_id;
          }
        } else {
          classification.exterior_count = checked_add(
              classification.exterior_count,
              1U,
              "the sparse closed-ball exterior count overflows size_t");
        }
        continue;
      }
      classification.frontier.push_back(
          make_witness_entry(current.right_child));
      classification.frontier.push_back(
          make_witness_entry(current.left_child));
      if (classification.frontier.size() >
          result_.budget.maximum_auxiliary_frontier_entry_count) {
        throw std::logic_error(
            "a sparse closed-ball DFS exceeded its preflight frontier bound");
      }
      result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
          result_.audit.maximum_closed_ball_frontier_entry_count,
          classification.frontier.size());
    }
    const std::size_t classified_count = checked_add(
        checked_add(
            classification.interior_ids.size(),
            classification.shell_count,
            "the sparse closed-ball partition count overflows size_t"),
        classification.exterior_count,
        "the sparse closed-ball partition count overflows size_t");
    if (classified_count != cloud_.size() ||
        classification.support_seen_mask != 3U ||
        classification.shell_count < 2U ||
        (classification.shell_count == 2U) !=
            !classification.canonical_extra_shell_witness_id.has_value()) {
      throw std::logic_error(
          "a sparse closed-ball traversal did not close its exact partition");
    }
    std::sort(
        classification.interior_ids.begin(),
        classification.interior_ids.end());
    classification.stage =
        ExactPairSupportPendingClosedBallStage::ready_to_emit;
    return SparseBallOutcome::complete;
  }

  [[nodiscard]] bool classify_leaf_pair(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (!first.is_leaf() || !second.is_leaf() ||
        first_node_index == second_node_index) {
      throw std::logic_error(
          "a pair-support leaf classifier received a nonterminal product");
    }
    std::array<PointId, 2> support_ids{
        index_.leaves_[first.leaf_begin].point_id,
        index_.leaves_[second.leaf_begin].point_id};
    if (support_ids[1] < support_ids[0]) {
      std::swap(support_ids[0], support_ids[1]);
    }
    const SparseBallOutcome outcome =
        continue_sparse_closed_ball(support_ids);
    if (outcome == SparseBallOutcome::budget_exhausted) {
      return false;
    }
    if (outcome == SparseBallOutcome::rank_exceeded) {
      frontier_.pop_back();
      result_.audit.leaf_pair_classification_count = checked_add(
          result_.audit.leaf_pair_classification_count,
          1U,
          "the pair-support leaf-classification count overflows size_t");
      result_.audit.resolved_pair_count = checked_add(
          result_.audit.resolved_pair_count,
          1U,
          "the pair-support resolved-pair count overflows size_t");
      result_.audit.above_rank_pair_count = checked_add(
          result_.audit.above_rank_pair_count,
          1U,
          "the pair-support above-rank count overflows size_t");
      return true;
    }
    if (!pending_product_->closed_ball.has_value() ||
        pending_product_->closed_ball->stage !=
            ExactPairSupportPendingClosedBallStage::ready_to_emit) {
      throw std::logic_error(
          "a complete sparse closed-ball query has no emission cursor");
    }
    if (!leaf_emission_preflight()) {
      return false;
    }
    ExactPairSupportPendingClosedBall& classification =
        *pending_product_->closed_ball;
    result_.audit.leaf_pair_classification_count = checked_add(
        result_.audit.leaf_pair_classification_count,
        1U,
        "the pair-support leaf-classification count overflows size_t");
    result_.audit.resolved_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        1U,
        "the pair-support resolved-pair count overflows size_t");

    // The sparse decision needs only the exact diametral identity.  Construct
    // the rational level once, and only for a record that survives the rank
    // cap.
    const exact::CircumcenterResult sphere = exact::circumcenter(
        cloud_.point(support_ids[0]),
        cloud_.point(support_ids[1]));
    if (sphere.kind() != exact::CircumcenterKind::unique ||
        !sphere.center().has_value() ||
        !sphere.squared_level().has_value()) {
      throw std::logic_error(
          "two canonical distinct points did not define a unique sphere");
    }
    const std::size_t observed_closed_rank = checked_add(
        classification.interior_ids.size(),
        classification.shell_count,
        "the pair-support observed rank overflows size_t");
    const std::size_t minimum_possible_closed_rank = checked_add(
        classification.interior_ids.size(),
        2U,
        "the pair-support minimum rank overflows size_t");
    if (minimum_possible_closed_rank >
        result_.requirements.maximum_relevant_closed_rank) {
      throw std::logic_error(
          "a complete sparse shell escaped its interior-rank cap");
    }
    std::size_t emitted_references = 0U;
    if (classification.shell_count == 2U) {
      ExactPairSupportEvent event;
      event.support_ids = support_ids;
      event.center = *sphere.center();
      event.squared_level = *sphere.squared_level();
      event.interior_ids = std::move(classification.interior_ids);
      event.closed_rank = observed_closed_rank;
      event.exterior_count = classification.exterior_count;
      emitted_references = checked_add(
          2U,
          event.interior_ids.size(),
          "the pair-support event reference count overflows size_t");
      output_chain_digest_ = extend_output_chain(output_chain_digest_, event);
      output_record_count_ = checked_add(
          output_record_count_,
          1U,
          "the pair-support output record count overflows size_t");
      result_.events.push_back(std::move(event));
      if (!canonical_sort_records_) {
        record_order_.push_back(
            ExactPairSupportStreamChunk::RecordKind::event);
      }
      result_.audit.accepted_event_count = checked_add(
          result_.audit.accepted_event_count,
          1U,
          "the pair-support event count overflows size_t");
    } else {
      if (!classification.canonical_extra_shell_witness_id.has_value()) {
        throw std::logic_error(
            "an extra-shell pair omitted its canonical extra witness");
      }
      ExactPairSupportExtraShellDiagnostic diagnostic;
      diagnostic.support_ids = support_ids;
      diagnostic.center = *sphere.center();
      diagnostic.squared_level = *sphere.squared_level();
      diagnostic.interior_ids = std::move(classification.interior_ids);
      diagnostic.shell_count = classification.shell_count;
      diagnostic.canonical_extra_shell_witness_id =
          *classification.canonical_extra_shell_witness_id;
      diagnostic.minimum_possible_closed_rank =
          minimum_possible_closed_rank;
      diagnostic.observed_closed_rank = observed_closed_rank;
      diagnostic.exterior_count = classification.exterior_count;
      emitted_references = checked_add(
          3U,
          diagnostic.interior_ids.size(),
          "the pair-support diagnostic reference count overflows size_t");
      output_chain_digest_ =
          extend_output_chain(output_chain_digest_, diagnostic);
      output_record_count_ = checked_add(
          output_record_count_,
          1U,
          "the pair-support output record count overflows size_t");
      result_.relevant_extra_shell_diagnostics.push_back(
          std::move(diagnostic));
      if (!canonical_sort_records_) {
        record_order_.push_back(
            ExactPairSupportStreamChunk::RecordKind::
                relevant_extra_shell_diagnostic);
      }
      result_.audit.relevant_extra_shell_diagnostic_count = checked_add(
          result_.audit.relevant_extra_shell_diagnostic_count,
          1U,
          "the pair-support diagnostic count overflows size_t");
    }
    result_.audit.emitted_point_id_reference_count = checked_add(
        result_.audit.emitted_point_id_reference_count,
        emitted_references,
        "the pair-support emitted reference count overflows size_t");
    if (consumed_since(
            result_.audit.emitted_point_id_reference_count,
            chunk_audit_origin_.emitted_point_id_reference_count,
            "the pair-support emitted-reference audit moved backwards") >
        result_.budget.maximum_emitted_point_id_reference_count) {
      throw std::logic_error(
          "a pair-support record exceeded its conservative reference preflight");
    }
    frontier_.pop_back();
    return true;
  }

  [[nodiscard]] bool push_replacement_entries(
      std::vector<ExactPairSupportFrontierEntry> replacements) {
    const std::size_t base_size = frontier_.size() - 1U;
    const std::size_t new_size = checked_add(
        base_size,
        replacements.size(),
        "the pair-support frontier size overflows size_t");
    if (new_size > result_.budget.maximum_frontier_entry_count) {
      stop(ExactPairSupportStopReason::frontier_entry_limit);
      return false;
    }
    frontier_.pop_back();
    for (auto iterator = replacements.rbegin();
         iterator != replacements.rend();
         ++iterator) {
      frontier_.push_back(*iterator);
    }
    result_.audit.maximum_frontier_entry_count = std::max(
        result_.audit.maximum_frontier_entry_count,
        frontier_.size());
    return true;
  }

  [[nodiscard]] bool expand_product(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index) {
      if (first.is_leaf()) {
        throw std::logic_error(
            "a pair-support diagonal leaf cannot be expanded");
      }
      if (!push_replacement_entries({
          make_entry(first.left_child, first.left_child),
          make_entry(first.left_child, first.right_child),
          make_entry(first.right_child, first.right_child)})) {
        return false;
      }
      result_.audit.support_product_expansion_count = checked_add(
          result_.audit.support_product_expansion_count,
          1U,
          "the pair-support product-expansion count overflows size_t");
      result_.audit.self_product_expansion_count = checked_add(
          result_.audit.self_product_expansion_count,
          1U,
          "the pair-support self-expansion count overflows size_t");
      return true;
    }
    const bool first_leaf = first.is_leaf();
    const bool second_leaf = second.is_leaf();
    bool split_first = false;
    if (!first_leaf && second_leaf) {
      split_first = true;
    } else if (first_leaf && !second_leaf) {
      split_first = false;
    } else if (!first_leaf && !second_leaf) {
      const exact::ExactLevel first_diagonal =
          node_squared_diagonal(first_node_index);
      const exact::ExactLevel second_diagonal =
          node_squared_diagonal(second_node_index);
      const std::size_t first_size = first.leaf_end - first.leaf_begin;
      const std::size_t second_size = second.leaf_end - second.leaf_begin;
      if (first_diagonal != second_diagonal) {
        split_first = first_diagonal > second_diagonal;
      } else if (first_size != second_size) {
        split_first = first_size > second_size;
      } else {
        split_first = first_node_index > second_node_index;
      }
    } else {
      throw std::logic_error(
          "a pair-support leaf cross product escaped classification");
    }
    if (split_first) {
      if (!push_replacement_entries({
          make_entry(first.left_child, second_node_index),
          make_entry(first.right_child, second_node_index)})) {
        return false;
      }
    } else {
      if (!push_replacement_entries({
          make_entry(first_node_index, second.left_child),
          make_entry(first_node_index, second.right_child)})) {
        return false;
      }
    }
    result_.audit.support_product_expansion_count = checked_add(
        result_.audit.support_product_expansion_count,
        1U,
        "the pair-support product-expansion count overflows size_t");
    result_.audit.cross_product_expansion_count = checked_add(
        result_.audit.cross_product_expansion_count,
        1U,
        "the pair-support cross-expansion count overflows size_t");
    return true;
  }

  void visit_frontier_back() {
    if (frontier_.empty() || pending_product_.has_value()) {
      throw std::logic_error(
          "a pair-support product visit requires one unclaimed frontier back");
    }
    if (!consume_work_unit()) {
      return;
    }
    result_.audit.support_product_visit_count = checked_add(
        result_.audit.support_product_visit_count,
        1U,
        "the pair-support product-visit count overflows size_t");
    const ExactPairSupportFrontierEntry entry = frontier_.back();
    const auto [first_node_index, second_node_index] = entry_nodes(entry);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index && first.is_leaf()) {
      frontier_.pop_back();
      result_.audit.diagonal_leaf_discard_count = checked_add(
          result_.audit.diagonal_leaf_discard_count,
          1U,
          "the pair-support diagonal count overflows size_t");
      return;
    }
    // A diagonal product cannot satisfy the strict phi prune: the relaxed
    // support boxes allow u = v at one endpoint of A, hence
    // phi(x, u, u) = ||x-u||^2 >= 0 for every witness box.  Avoid a global
    // witness traversal whose exact outcome is known in advance.
    if (first_node_index == second_node_index) {
      result_.audit.diagonal_product_rank_search_skip_count = checked_add(
          result_.audit.diagonal_product_rank_search_skip_count,
          1U,
          "the pair-support diagonal skip count overflows size_t");
      pending_product_.emplace();
      pending_product_->product = entry;
      pending_product_->stage =
          ExactPairSupportPendingStage::expand_product;
      continue_pending_product();
      return;
    }
    // At a leaf pair, the sparse closed-ball traversal already performs the
    // exact rank cap.  Running a separate global phi witness search here
    // would traverse the same LBVH twice for every surviving support.
    if (first.is_leaf() && second.is_leaf()) {
      pending_product_.emplace();
      pending_product_->product = entry;
      pending_product_->stage = ExactPairSupportPendingStage::classify_leaf;
      continue_pending_product();
      return;
    }

    pending_product_.emplace();
    pending_product_->product = entry;
    pending_product_->stage = ExactPairSupportPendingStage::rank_search;
    continue_pending_product();
  }

  void continue_pending_product() {
    if (!pending_product_.has_value() || frontier_.empty() ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a pair-support pending product is detached from the frontier");
    }
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::rank_search) {
      RankSearchOutcome rank_search = continue_rank_prune_search();
      if (rank_search == RankSearchOutcome::budget_exhausted) {
        return;
      }
      if (rank_search == RankSearchOutcome::keep) {
        const CenterCoverRankPruneOutcome center_cover =
            try_atomic_center_cover_rank_prune(
                pending_product_->product);
        if (center_cover ==
            CenterCoverRankPruneOutcome::budget_exhausted) {
          return;
        }
        if (center_cover == CenterCoverRankPruneOutcome::pruned) {
          rank_search = RankSearchOutcome::prune;
        }
      }
      if (rank_search == RankSearchOutcome::prune) {
        const std::size_t pair_count =
            entry_pair_count(pending_product_->product);
        frontier_.pop_back();
        result_.audit.rank_pruned_product_count = checked_add(
            result_.audit.rank_pruned_product_count,
            1U,
            "the pair-support pruned-product count overflows size_t");
        result_.audit.rank_pruned_pair_count = checked_add(
            result_.audit.rank_pruned_pair_count,
            pair_count,
            "the pair-support pruned-pair count overflows size_t");
        result_.audit.resolved_pair_count = checked_add(
            result_.audit.resolved_pair_count,
            pair_count,
            "the pair-support resolved-pair count overflows size_t");
        pending_product_.reset();
        return;
      }
      pending_product_->stage =
          ExactPairSupportPendingStage::expand_product;
      pending_product_->rank_search_started = false;
      pending_product_->witness_frontier.clear();
      pending_product_->strict_witness_receipts.clear();
      pending_product_->deferred_expansion_node.reset();
      pending_product_->strict_witness_point_count = 0U;
      pending_product_->closed_ball.reset();
    }

    const auto [first_node_index, second_node_index] =
        entry_nodes(pending_product_->product);
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::expand_product) {
      if (expand_product(first_node_index, second_node_index)) {
        pending_product_.reset();
      }
      return;
    }
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::classify_leaf) {
      if (classify_leaf_pair(first_node_index, second_node_index)) {
        pending_product_.reset();
      }
      return;
    }
    throw std::logic_error("a pair-support pending stage is invalid");
  }

  void finish_result() {
    if (canonical_sort_records_) {
      std::sort(result_.events.begin(), result_.events.end(), event_less);
      std::sort(
          result_.relevant_extra_shell_diagnostics.begin(),
          result_.relevant_extra_shell_diagnostics.end(),
          diagnostic_less);
    }
    if (canonical_sort_records_) {
      result_.remaining_frontier = frontier_;
    }
    result_.audit.remaining_frontier_pair_count = 0U;
    for (const ExactPairSupportFrontierEntry& entry : frontier_) {
      result_.audit.remaining_frontier_pair_count = checked_add(
          result_.audit.remaining_frontier_pair_count,
          entry_pair_count(entry),
          "the pair-support remaining-pair count overflows size_t");
    }
    const std::size_t accounted_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        result_.audit.remaining_frontier_pair_count,
        "the pair-support accounting sum overflows size_t");
    result_.audit.pair_partition_accounting_certified =
        accounted_pair_count == result_.audit.total_pair_count &&
        result_.audit.resolved_pair_count == checked_add(
            result_.audit.rank_pruned_pair_count,
            result_.audit.leaf_pair_classification_count,
            "the pair-support terminal accounting overflows size_t");
    if (!result_.audit.pair_partition_accounting_certified) {
      throw std::logic_error(
          "the pair-support frontier does not partition all unordered pairs");
    }
    result_.self_product_partition_certified = true;
    result_.witness_antichains_certified = true;
    result_.all_rank_prunes_recertified = true;
    // A stopped, resumable leaf traversal has not yet closed its global
    // shell.  A classify-leaf product certifies completion only with an
    // explicit ready-to-emit cursor; query/auxiliary preflight can stop before
    // that cursor exists.
    result_.all_rank_relevant_shells_complete =
        !pending_product_.has_value() ||
        pending_product_->stage !=
            ExactPairSupportPendingStage::classify_leaf ||
        (pending_product_->closed_ball.has_value() &&
         pending_product_->closed_ball->stage ==
             ExactPairSupportPendingClosedBallStage::ready_to_emit);
    result_.frontier_exhausted = frontier_.empty();
    result_.no_forbidden_global_structure_materialized = true;
    result_.hierarchy_reduction_performed = false;
    if (frontier_.empty()) {
      result_.status = ExactPairSupportStreamStatus::complete;
      result_.stop_reason = ExactPairSupportStopReason::none;
    } else if (!stopped_) {
      throw std::logic_error(
          "a nonempty pair-support frontier has no budget stop reason");
    }
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  ExactPairSupportStreamResult result_;
  std::vector<ExactPairSupportFrontierEntry> frontier_;
  std::optional<ExactPairSupportPendingProduct> pending_product_;
  ExactPairSupportStreamAudit chunk_audit_origin_{};
  contract::CanonicalId output_chain_digest_{};
  std::size_t output_record_count_{0U};
  std::vector<ExactPairSupportStreamChunk::RecordKind> record_order_;
  std::size_t maximum_products_per_batch_{0U};
  const ExactPairSupportRankPruneBatchCallback* proposal_callback_{nullptr};
  std::vector<ExactPairSupportFrontierEntry> proposal_batch_products_;
  std::vector<ExactPairSupportRankPruneProposal> proposal_cache_;
  std::vector<ExactPairSupportRankKeepCertificate>
      keep_certificate_cache_;
  std::vector<ExactPairSupportRankPruneProposal>
      consumed_rank_prune_proposals_;
  std::vector<ExactPairSupportRankKeepCertificate>
      consumed_rank_keep_certificates_;
  ExactPairSupportRankPruneProposalAudit proposal_audit_{};
  bool proposal_batch_active_{false};
  bool canonical_sort_records_{false};
  bool stopped_{false};
};

ExactPairSupportStreamResult build_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget) {
  ExactPairSupportStreamBuilder builder{
      index, cloud, requested_maximum_order, budget};
  builder.execute();
  return builder.take_result();
}

ExactPairSupportCheckpointManifest
make_exact_pair_support_checkpoint_manifest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  return ExactPairSupportStreamBuilder::manifest_for(
      index, cloud, requested_maximum_order);
}

ExactPairSupportAuthorityContext::ExactPairSupportAuthorityContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order)
    : index_(&index),
      cloud_(&cloud),
      requested_maximum_order_(requested_maximum_order) {
  manifest_ = ExactPairSupportStreamBuilder::manifest_for_with_audit(
      index, cloud, requested_maximum_order, &audit_);
}

ExactPairSupportCheckpoint make_initial_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return make_initial_exact_pair_support_checkpoint(authority);
}

ExactPairSupportCheckpoint make_initial_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority) {
  ExactPairSupportStreamBuilder builder{
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      ExactPairSupportStreamBudget{
          0U,
          1U,
          0U,
          0U,
          0U,
          0U,
          0U}};
  return builder.snapshot_checkpoint(authority.manifest(), 0U);
}

contract::CanonicalId compute_exact_pair_support_checkpoint_digest(
    const ExactPairSupportCheckpoint& checkpoint) {
  return checkpoint_digest(checkpoint);
}

ExactPairSupportCheckpointVerification verify_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_pair_support_checkpoint(authority, checkpoint);
}

ExactPairSupportCheckpointVerification verify_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportCheckpoint& checkpoint) {
  return ExactPairSupportStreamBuilder::verify_checkpoint_for(
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      authority.manifest(),
      checkpoint);
}

namespace {

[[nodiscard]] ExactPairSupportStreamChunk
build_exact_pair_support_stream_chunk_after_source_verification(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  ExactPairSupportStreamChunk chunk;
  chunk.manifest = checkpoint.manifest;
  chunk.budget = chunk_budget;
  chunk.chunk_sequence = checkpoint.next_chunk_sequence;
  chunk.first_output_record_index = checkpoint.output_record_count;
  chunk.source_checkpoint_digest = checkpoint.checkpoint_digest;
  chunk.previous_output_chain_digest =
      checkpoint.output_chain_digest;
  chunk.cumulative_audit_before = checkpoint.cumulative_audit;
  chunk.no_forbidden_global_structure_materialized = true;
  chunk.hierarchy_reduction_performed = false;
  if (checkpoint.complete()) {
    chunk.output_chain_digest =
        checkpoint.output_chain_digest;
    chunk.status = ExactPairSupportStreamStatus::complete;
    chunk.stop_reason = ExactPairSupportStopReason::none;
    chunk.cumulative_audit_after = checkpoint.cumulative_audit;
    chunk.next_checkpoint = checkpoint;
    chunk.candidate_prepared = true;
    return chunk;
  }
  if (checkpoint.next_chunk_sequence ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the pair-support chunk sequence overflows uint64");
  }

  ExactPairSupportStreamBuilder builder{
      index,
      cloud,
      requested_maximum_order,
      chunk_budget,
      checkpoint,
      IntegrityVerifiedCheckpointTag{}};
  builder.execute();
  chunk.next_checkpoint = builder.take_checkpoint(
      checkpoint.manifest, checkpoint.next_chunk_sequence + 1U);
  ExactPairSupportStreamResult result = builder.take_result();
  chunk.output_chain_digest = builder.output_chain_digest();
  chunk.status = result.status;
  chunk.stop_reason = result.stop_reason;
  chunk.events = std::move(result.events);
  chunk.relevant_extra_shell_diagnostics =
      std::move(result.relevant_extra_shell_diagnostics);
  chunk.record_order = builder.take_record_order();
  chunk.cumulative_audit_after = result.audit;
  chunk.candidate_prepared = true;
  return chunk;
}

[[nodiscard]] ExactPairSupportRankPruneProposedChunk
build_exact_pair_support_stream_proposed_chunk_after_source_verification(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint,
    std::size_t maximum_products_per_batch,
    const ExactPairSupportRankPruneBatchCallback& callback) {
  ExactPairSupportRankPruneProposedChunk proposed;
  ExactPairSupportStreamChunk& chunk = proposed.candidate_chunk;
  proposed.audit.maximum_products_per_batch =
      maximum_products_per_batch;
  chunk.manifest = checkpoint.manifest;
  chunk.budget = chunk_budget;
  chunk.chunk_sequence = checkpoint.next_chunk_sequence;
  chunk.first_output_record_index = checkpoint.output_record_count;
  chunk.source_checkpoint_digest = checkpoint.checkpoint_digest;
  chunk.previous_output_chain_digest =
      checkpoint.output_chain_digest;
  chunk.cumulative_audit_before = checkpoint.cumulative_audit;
  chunk.no_forbidden_global_structure_materialized = true;
  chunk.hierarchy_reduction_performed = false;
  if (checkpoint.complete()) {
    chunk.output_chain_digest = checkpoint.output_chain_digest;
    chunk.status = ExactPairSupportStreamStatus::complete;
    chunk.stop_reason = ExactPairSupportStopReason::none;
    chunk.cumulative_audit_after = checkpoint.cumulative_audit;
    chunk.next_checkpoint = checkpoint;
    chunk.candidate_prepared = true;
    return proposed;
  }
  if (checkpoint.next_chunk_sequence ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the pair-support chunk sequence overflows uint64");
  }

  ExactPairSupportStreamBuilder builder{
      index,
      cloud,
      requested_maximum_order,
      chunk_budget,
      checkpoint,
      IntegrityVerifiedCheckpointTag{},
      maximum_products_per_batch,
      callback};
  builder.execute();
  chunk.next_checkpoint = builder.take_checkpoint(
      checkpoint.manifest, checkpoint.next_chunk_sequence + 1U);
  ExactPairSupportStreamResult result = builder.take_result();
  chunk.output_chain_digest = builder.output_chain_digest();
  chunk.status = result.status;
  chunk.stop_reason = result.stop_reason;
  chunk.events = std::move(result.events);
  chunk.relevant_extra_shell_diagnostics =
      std::move(result.relevant_extra_shell_diagnostics);
  chunk.record_order = builder.take_record_order();
  chunk.cumulative_audit_after = result.audit;
  chunk.candidate_prepared = true;
  proposed.audit = builder.rank_prune_proposal_audit();
  proposed.consumed_proposals =
      builder.take_consumed_rank_prune_proposals();
  proposed.consumed_keep_certificates =
      builder.take_consumed_rank_keep_certificates();
  return proposed;
}

}  // namespace

ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return build_exact_pair_support_stream_chunk(
      authority, chunk_budget, checkpoint);
}

ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportCheckpointVerification source_verification =
      verify_exact_pair_support_checkpoint(authority, checkpoint);
  if (!source_verification.integrity_verified) {
    throw std::invalid_argument(
        "a pair-support chunk requires an integrity-verified trusted source checkpoint");
  }
  return build_exact_pair_support_stream_chunk_after_source_verification(
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      chunk_budget,
      checkpoint);
}

ExactPairSupportRankPruneProposedChunk
build_exact_pair_support_stream_chunk_with_rank_prune_proposals(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint,
    std::size_t maximum_products_per_batch,
    const ExactPairSupportRankPruneBatchCallback& callback) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return build_exact_pair_support_stream_chunk_with_rank_prune_proposals(
      authority,
      chunk_budget,
      checkpoint,
      maximum_products_per_batch,
      callback);
}

ExactPairSupportRankPruneProposedChunk
build_exact_pair_support_stream_chunk_with_rank_prune_proposals(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint,
    std::size_t maximum_products_per_batch,
    const ExactPairSupportRankPruneBatchCallback& callback) {
  if (maximum_products_per_batch == 0U) {
    throw std::invalid_argument(
        "a pair-support proposal batch must contain at least one product");
  }
  if (!callback) {
    throw std::invalid_argument(
        "a pair-support proposal build requires a batch callback");
  }
  const ExactPairSupportCheckpointVerification source_verification =
      verify_exact_pair_support_checkpoint(authority, checkpoint);
  if (!source_verification.integrity_verified) {
    throw std::invalid_argument(
        "a pair-support proposal chunk requires an integrity-verified source checkpoint");
  }
  return
      build_exact_pair_support_stream_proposed_chunk_after_source_verification(
          authority.index(),
          authority.cloud(),
          authority.requested_maximum_order(),
          chunk_budget,
          checkpoint,
          maximum_products_per_batch,
          callback);
}

ExactPairSupportStreamChunkVerification verify_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_pair_support_stream_chunk(
      authority, chunk_budget, source_checkpoint, observed);
}

namespace {

[[nodiscard]] ExactPairSupportStreamChunkVerification
verify_exact_pair_support_stream_chunk_with_trusted_next(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed,
    bool source_checkpoint_already_trusted,
    ExactPairSupportCheckpoint* trusted_next_checkpoint) {
  ExactPairSupportStreamChunkVerification verification;
  verification.source_checkpoint_integrity_verified =
      source_checkpoint_already_trusted ||
      verify_exact_pair_support_checkpoint(authority, source_checkpoint)
          .integrity_verified;
  verification.requested_budget_certified =
      observed.budget == chunk_budget;
  if (!verification.source_checkpoint_integrity_verified) {
    return verification;
  }
  ExactPairSupportStreamChunk expected =
      build_exact_pair_support_stream_chunk_after_source_verification(
          authority.index(),
          authority.cloud(),
          authority.requested_maximum_order(),
          chunk_budget,
          source_checkpoint);
  verification.prepared_transition_chain_matches =
      observed.manifest == expected.manifest &&
      observed.chunk_sequence == expected.chunk_sequence &&
      observed.first_output_record_index ==
          expected.first_output_record_index &&
      observed.source_checkpoint_digest ==
          expected.source_checkpoint_digest &&
      observed.previous_output_chain_digest ==
          expected.previous_output_chain_digest &&
      observed.output_chain_digest ==
          expected.output_chain_digest &&
      observed.candidate_prepared == expected.candidate_prepared;
  verification.records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics &&
      observed.record_order == expected.record_order;
  verification.next_checkpoint_integrity_verified =
      observed.next_checkpoint == expected.next_checkpoint &&
      verify_exact_pair_support_checkpoint(
          authority, observed.next_checkpoint)
          .integrity_verified;
  verification.fresh_replay_certified = observed == expected;
  verification.chunk_transition_verified =
      verification.source_checkpoint_integrity_verified &&
      verification.requested_budget_certified &&
      verification.prepared_transition_chain_matches &&
      verification.records_individually_exact &&
      verification.next_checkpoint_integrity_verified &&
      verification.fresh_replay_certified;
  if (verification.chunk_transition_verified &&
      trusted_next_checkpoint != nullptr) {
    static_assert(
        std::is_nothrow_move_assignable_v<ExactPairSupportCheckpoint>);
    *trusted_next_checkpoint = std::move(expected.next_checkpoint);
  }
  return verification;
}

}  // namespace

ExactPairSupportStreamChunkVerification verify_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed) {
  return verify_exact_pair_support_stream_chunk_with_trusted_next(
      authority,
      chunk_budget,
      source_checkpoint,
      observed,
      false,
      nullptr);
}

ExactPairSupportRankPruneProposedChunkVerification
verify_exact_pair_support_stream_proposed_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    std::size_t maximum_products_per_batch,
    const ExactPairSupportRankPruneProposedChunk& observed) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_pair_support_stream_proposed_chunk(
      authority,
      chunk_budget,
      source_checkpoint,
      maximum_products_per_batch,
      observed);
}

ExactPairSupportRankPruneProposedChunkVerification
verify_exact_pair_support_stream_proposed_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    std::size_t maximum_products_per_batch,
    const ExactPairSupportRankPruneProposedChunk& observed) {
  ExactPairSupportRankPruneProposedChunkVerification verification;
  verification.durable_wire_v1_claimed = false;
  ExactPairSupportStreamChunkVerification& chunk_verification =
      verification.candidate_chunk_verification;
  chunk_verification.source_checkpoint_integrity_verified =
      verify_exact_pair_support_checkpoint(authority, source_checkpoint)
          .integrity_verified;
  chunk_verification.requested_budget_certified =
      observed.candidate_chunk.budget == chunk_budget;
  if (!chunk_verification.source_checkpoint_integrity_verified ||
      maximum_products_per_batch == 0U) {
    return verification;
  }

  if (observed.consumed_proposals.size() >
          chunk_budget.maximum_work_unit_count ||
      observed.consumed_keep_certificates.size() >
          chunk_budget.maximum_work_unit_count -
              observed.consumed_proposals.size()) {
    return verification;
  }
  std::size_t transcript_terminal_count = 0U;
  for (const ExactPairSupportRankPruneProposal& proposal :
       observed.consumed_proposals) {
    if (!can_add_within(
            transcript_terminal_count,
            proposal.strict_witness_receipts.size(),
            chunk_budget.maximum_work_unit_count)) {
      return verification;
    }
    transcript_terminal_count +=
        proposal.strict_witness_receipts.size();
  }
  for (const ExactPairSupportRankKeepCertificate& certificate :
       observed.consumed_keep_certificates) {
    if (!can_add_within(
            transcript_terminal_count,
            certificate.canonical_terminals.size(),
            chunk_budget.maximum_work_unit_count)) {
      return verification;
    }
    transcript_terminal_count +=
        certificate.canonical_terminals.size();
  }

  std::vector<const ExactPairSupportRankPruneProposal*>
      prune_transcript_by_product;
  prune_transcript_by_product.reserve(
      observed.consumed_proposals.size());
  for (const ExactPairSupportRankPruneProposal& proposal :
       observed.consumed_proposals) {
    prune_transcript_by_product.push_back(&proposal);
  }
  std::sort(
      prune_transcript_by_product.begin(),
      prune_transcript_by_product.end(),
      [](const ExactPairSupportRankPruneProposal* left,
         const ExactPairSupportRankPruneProposal* right) {
        return frontier_entry_less(
            left->product, right->product);
      });
  for (std::size_t index = 1U;
       index < prune_transcript_by_product.size();
       ++index) {
    if (!frontier_entry_less(
            prune_transcript_by_product[index - 1U]->product,
            prune_transcript_by_product[index]->product)) {
      return verification;
    }
  }

  std::vector<const ExactPairSupportRankKeepCertificate*>
      keep_transcript_by_product;
  keep_transcript_by_product.reserve(
      observed.consumed_keep_certificates.size());
  for (const ExactPairSupportRankKeepCertificate& certificate :
       observed.consumed_keep_certificates) {
    keep_transcript_by_product.push_back(&certificate);
  }
  std::sort(
      keep_transcript_by_product.begin(),
      keep_transcript_by_product.end(),
      [](const ExactPairSupportRankKeepCertificate* left,
         const ExactPairSupportRankKeepCertificate* right) {
        return frontier_entry_less(
            left->product, right->product);
      });
  for (std::size_t index = 1U;
       index < keep_transcript_by_product.size();
       ++index) {
    if (!frontier_entry_less(
            keep_transcript_by_product[index - 1U]->product,
            keep_transcript_by_product[index]->product)) {
      return verification;
    }
  }
  verification.no_duplicate_keep_certificate_retained = true;

  std::size_t prune_index = 0U;
  std::size_t keep_index = 0U;
  while (prune_index < prune_transcript_by_product.size() &&
         keep_index < keep_transcript_by_product.size()) {
    const ExactPairSupportFrontierEntry& prune_product =
        prune_transcript_by_product[prune_index]->product;
    const ExactPairSupportFrontierEntry& keep_product =
        keep_transcript_by_product[keep_index]->product;
    if (prune_product == keep_product) {
      return verification;
    }
    if (frontier_entry_less(prune_product, keep_product)) {
      ++prune_index;
    } else {
      ++keep_index;
    }
  }

  const ExactPairSupportRankPruneBatchCallback replay_callback =
      [&prune_transcript_by_product, &keep_transcript_by_product](
          const ExactPairSupportRankPruneBatchRequest& request) {
        ExactPairSupportRankPruneBatchProposal response;
        response.proposals.reserve(
            request.canonical_products.size());
        response.keep_certificates.reserve(
            request.canonical_products.size());
        for (const ExactPairSupportFrontierEntry& product :
             request.canonical_products) {
          const auto prune = std::lower_bound(
              prune_transcript_by_product.begin(),
              prune_transcript_by_product.end(),
              product,
              [](const ExactPairSupportRankPruneProposal* proposal,
                 const ExactPairSupportFrontierEntry& requested) {
                return frontier_entry_less(
                    proposal->product, requested);
              });
          if (prune != prune_transcript_by_product.end() &&
              (*prune)->product == product) {
            response.proposals.push_back(**prune);
            continue;
          }
          const auto keep = std::lower_bound(
              keep_transcript_by_product.begin(),
              keep_transcript_by_product.end(),
              product,
              [](const ExactPairSupportRankKeepCertificate* certificate,
                 const ExactPairSupportFrontierEntry& requested) {
                return frontier_entry_less(
                    certificate->product, requested);
              });
          if (keep != keep_transcript_by_product.end() &&
              (*keep)->product == product) {
            response.keep_certificates.push_back(**keep);
          }
        }
        return response;
      };

  ExactPairSupportRankPruneProposedChunk expected;
  try {
    expected =
        build_exact_pair_support_stream_proposed_chunk_after_source_verification(
            authority.index(),
            authority.cloud(),
            authority.requested_maximum_order(),
            chunk_budget,
            source_checkpoint,
            maximum_products_per_batch,
            replay_callback);
  } catch (const std::exception&) {
    return verification;
  }

  const ExactPairSupportStreamChunk& observed_chunk =
      observed.candidate_chunk;
  const ExactPairSupportStreamChunk& expected_chunk =
      expected.candidate_chunk;
  chunk_verification.prepared_transition_chain_matches =
      observed_chunk.manifest == expected_chunk.manifest &&
      observed_chunk.chunk_sequence == expected_chunk.chunk_sequence &&
      observed_chunk.first_output_record_index ==
          expected_chunk.first_output_record_index &&
      observed_chunk.source_checkpoint_digest ==
          expected_chunk.source_checkpoint_digest &&
      observed_chunk.previous_output_chain_digest ==
          expected_chunk.previous_output_chain_digest &&
      observed_chunk.output_chain_digest ==
          expected_chunk.output_chain_digest &&
      observed_chunk.candidate_prepared ==
          expected_chunk.candidate_prepared;
  chunk_verification.records_individually_exact =
      observed_chunk.events == expected_chunk.events &&
      observed_chunk.relevant_extra_shell_diagnostics ==
          expected_chunk.relevant_extra_shell_diagnostics &&
      observed_chunk.record_order == expected_chunk.record_order;
  chunk_verification.next_checkpoint_integrity_verified =
      observed_chunk.next_checkpoint == expected_chunk.next_checkpoint &&
      verify_exact_pair_support_checkpoint(
          authority, observed_chunk.next_checkpoint)
          .integrity_verified;
  chunk_verification.fresh_replay_certified =
      observed_chunk == expected_chunk;
  chunk_verification.chunk_transition_verified =
      chunk_verification.source_checkpoint_integrity_verified &&
      chunk_verification.requested_budget_certified &&
      chunk_verification.prepared_transition_chain_matches &&
      chunk_verification.records_individually_exact &&
      chunk_verification.next_checkpoint_integrity_verified &&
      chunk_verification.fresh_replay_certified;

  verification.every_consumed_proposal_replayed_once =
      expected.consumed_proposals == observed.consumed_proposals &&
      expected.audit.consumed_proposal_count ==
          observed.consumed_proposals.size() &&
      expected.audit.exact_receipt_recertification_count ==
          expected.audit.consumed_receipt_count;
  verification.no_unconsumed_proposal_retained =
      expected.consumed_proposals.size() ==
          prune_transcript_by_product.size();
  verification.every_consumed_keep_certificate_replayed_once =
      expected.consumed_keep_certificates ==
          observed.consumed_keep_certificates &&
      expected.audit.consumed_keep_certificate_count ==
          observed.consumed_keep_certificates.size() &&
      expected.audit.exact_keep_terminal_recertification_count ==
          expected.audit.keep_terminal_classification_count;
  verification.no_unconsumed_keep_certificate_retained =
      expected.consumed_keep_certificates.size() ==
          keep_transcript_by_product.size();
  verification.keep_candidate_transition_consistent =
      chunk_verification.chunk_transition_verified &&
      verification.every_consumed_keep_certificate_replayed_once &&
      verification.no_unconsumed_keep_certificate_retained &&
      verification.no_duplicate_keep_certificate_retained;
  verification.proposed_transition_verified =
      chunk_verification.chunk_transition_verified &&
      verification.every_consumed_proposal_replayed_once &&
      verification.no_unconsumed_proposal_retained &&
      verification.every_consumed_keep_certificate_replayed_once &&
      verification.no_unconsumed_keep_certificate_retained &&
      verification.no_duplicate_keep_certificate_retained &&
      verification.keep_candidate_transition_consistent &&
      observed == expected &&
      !observed.audit.durable_wire_v1_claimed &&
      observed.audit.transcript_ephemeral &&
      observed.audit.no_forbidden_global_structure_materialized;
  return verification;
}

ExactPairSupportIncrementalVerifier::ExactPairSupportIncrementalVerifier(
    const ExactPairSupportAuthorityContext& authority)
    : authority_(authority),
      trusted_checkpoint_(
          make_initial_exact_pair_support_checkpoint(authority_)) {
  status_.initial_checkpoint_reconstructed =
      verify_exact_pair_support_checkpoint(
          authority_, trusted_checkpoint_)
          .integrity_verified;
  status_.every_transition_verified =
      status_.initial_checkpoint_reconstructed;
  status_.terminal_checkpoint_reached =
      status_.initial_checkpoint_reconstructed &&
      trusted_checkpoint_.complete();
  status_.anchored_prefix_certified =
      status_.initial_checkpoint_reconstructed;
  status_.anchored_run_certified =
      status_.anchored_prefix_certified &&
      status_.terminal_checkpoint_reached;
  status_.failed_closed =
      !status_.initial_checkpoint_reconstructed;
  status_.retained_chunk_count = 0U;
}

ExactPairSupportIncrementalVerifier::PreparedNext::PreparedNext(
    ExactPairSupportIncrementalVerifier* owner,
    std::uint64_t source_epoch,
    std::size_t next_verified_chunk_count,
    ExactPairSupportStreamChunkVerification verification,
    ExactPairSupportCheckpoint trusted_next) noexcept
    : owner_(owner),
      source_epoch_(source_epoch),
      next_verified_chunk_count_(next_verified_chunk_count),
      verification_(verification),
      trusted_next_(std::move(trusted_next)),
      valid_(true) {
  static_assert(
      std::is_nothrow_move_constructible_v<ExactPairSupportCheckpoint>);
}

ExactPairSupportIncrementalVerifier::PreparedNext::PreparedNext(
    PreparedNext&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)),
      source_epoch_(other.source_epoch_),
      next_verified_chunk_count_(other.next_verified_chunk_count_),
      verification_(other.verification_),
      trusted_next_(std::move(other.trusted_next_)),
      valid_(std::exchange(other.valid_, false)) {}

ExactPairSupportIncrementalVerifier::PreparedNext&
ExactPairSupportIncrementalVerifier::PreparedNext::operator=(
    PreparedNext&& other) noexcept {
  if (this != &other) {
    owner_ = std::exchange(other.owner_, nullptr);
    source_epoch_ = other.source_epoch_;
    next_verified_chunk_count_ = other.next_verified_chunk_count_;
    verification_ = other.verification_;
    trusted_next_ = std::move(other.trusted_next_);
    valid_ = std::exchange(other.valid_, false);
  }
  return *this;
}

void ExactPairSupportIncrementalVerifier::poison() noexcept {
  status_.every_transition_verified = false;
  status_.anchored_prefix_certified = false;
  status_.anchored_run_certified = false;
  status_.failed_closed = true;
}

ExactPairSupportIncrementalVerifier::PreparedNext
ExactPairSupportIncrementalVerifier::prepare_next(
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportStreamChunk& observed) {
  if (status_.failed_closed ||
      !status_.initial_checkpoint_reconstructed ||
      status_.terminal_checkpoint_reached ||
      epoch_ == std::numeric_limits<std::uint64_t>::max()) {
    poison();
    return PreparedNext{};
  }

  ExactPairSupportCheckpoint trusted_next;
  ExactPairSupportStreamChunkVerification verification;
  try {
    verification =
        verify_exact_pair_support_stream_chunk_with_trusted_next(
            authority_,
            chunk_budget,
            trusted_checkpoint_,
            observed,
            true,
            &trusted_next);
  } catch (...) {
    poison();
    throw;
  }
  if (!verification.chunk_transition_verified) {
    poison();
    return PreparedNext{verification};
  }

  std::size_t verified_chunk_count = 0U;
  try {
    verified_chunk_count = checked_add(
        status_.verified_chunk_count,
        1U,
        "the incremental pair-support verified chunk count overflows size_t");
  } catch (...) {
    poison();
    throw;
  }
  return PreparedNext{
      this,
      epoch_,
      verified_chunk_count,
      verification,
      std::move(trusted_next)};
}

bool ExactPairSupportIncrementalVerifier::commit_prepared(
    PreparedNext&& prepared) noexcept {
  if (!prepared.valid_ || prepared.owner_ != this ||
      prepared.source_epoch_ != epoch_ || status_.failed_closed ||
      status_.terminal_checkpoint_reached ||
      !prepared.verification_.chunk_transition_verified) {
    poison();
    prepared.valid_ = false;
    prepared.owner_ = nullptr;
    return false;
  }

  static_assert(
      std::is_nothrow_move_assignable_v<ExactPairSupportCheckpoint>);
  trusted_checkpoint_ = std::move(prepared.trusted_next_);
  status_.verified_chunk_count = prepared.next_verified_chunk_count_;
  status_.terminal_checkpoint_reached = trusted_checkpoint_.complete();
  status_.anchored_prefix_certified = true;
  status_.anchored_run_certified = status_.terminal_checkpoint_reached;
  status_.retained_chunk_count = 0U;
  ++epoch_;
  prepared.valid_ = false;
  prepared.owner_ = nullptr;
  return true;
}

ExactPairSupportStreamChunkVerification
ExactPairSupportIncrementalVerifier::verify_next(
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportStreamChunk& observed) {
  PreparedNext prepared = prepare_next(chunk_budget, observed);
  ExactPairSupportStreamChunkVerification verification =
      prepared.verification();
  if (prepared.prepared() && !commit_prepared(std::move(prepared))) {
    verification.chunk_transition_verified = false;
  }
  return verification;
}

ExactPairSupportStreamRunVerification verify_exact_pair_support_stream_run(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    std::span<const ExactPairSupportStreamBudget> chunk_budgets,
    std::span<const ExactPairSupportStreamChunk> chunks) {
  ExactPairSupportStreamRunVerification verification;
  if (chunk_budgets.size() != chunks.size()) {
    return verification;
  }

  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  ExactPairSupportIncrementalVerifier incremental{authority};
  for (std::size_t chunk_index = 0U;
       chunk_index < chunks.size();
       ++chunk_index) {
    const ExactPairSupportStreamChunkVerification transition =
        incremental.verify_next(
            chunk_budgets[chunk_index],
            chunks[chunk_index]);
    if (!transition.chunk_transition_verified) {
      break;
    }
  }

  const ExactPairSupportIncrementalVerifierStatus& status =
      incremental.status();
  verification.initial_checkpoint_reconstructed =
      status.initial_checkpoint_reconstructed;
  verification.verified_chunk_count = status.verified_chunk_count;
  verification.every_transition_verified =
      status.every_transition_verified &&
      status.verified_chunk_count == chunks.size();
  verification.terminal_checkpoint_reached =
      status.terminal_checkpoint_reached;
  verification.anchored_run_certified =
      verification.initial_checkpoint_reconstructed &&
      verification.every_transition_verified &&
      verification.verified_chunk_count == chunks.size() &&
      verification.terminal_checkpoint_reached &&
      status.anchored_run_certified &&
      !status.failed_closed &&
      status.retained_chunk_count == 0U;
  return verification;
}

ExactPairSupportStreamVerification verify_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& observed) {
  const ExactPairSupportStreamResult expected =
      build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, budget);
  ExactPairSupportStreamVerification verification;
  verification.requested_budget_certified = observed.budget == budget;
  verification.requirements_certified =
      observed.requirements == expected.requirements;
  verification.partial_records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics;
  verification.completion_claim_certified =
      observed.stream_complete() == expected.stream_complete() &&
      observed.status == expected.status &&
      observed.stop_reason == expected.stop_reason &&
      observed.frontier_exhausted == expected.frontier_exhausted;
  verification.absence_claim_certified =
      observed.absence_of_additional_pair_supports_certified() ==
      expected.absence_of_additional_pair_supports_certified();
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.partial_records_individually_exact &&
      verification.completion_claim_certified &&
      verification.absence_claim_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
