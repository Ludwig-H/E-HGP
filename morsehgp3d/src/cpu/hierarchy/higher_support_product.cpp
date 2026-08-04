#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include "morsehgp3d/exact/binary64.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using Interval = ExactRationalInterval;
using Vector3 = std::array<Interval, 3>;
using Matrix3 = std::array<std::array<Interval, 3>, 3>;

struct ExactBoxCoordinates {
  std::array<exact::ExactRational, 3> lower{};
  std::array<exact::ExactRational, 3> upper{};
};

[[nodiscard]] Interval singleton(exact::ExactRational value) {
  return Interval{value, std::move(value)};
}

[[nodiscard]] Interval add(const Interval& left, const Interval& right) {
  return Interval{left.lower + right.lower, left.upper + right.upper};
}

[[nodiscard]] Interval subtract(
    const Interval& left,
    const Interval& right) {
  return Interval{left.lower - right.upper, left.upper - right.lower};
}

[[nodiscard]] Interval multiply(
    const Interval& left,
    const Interval& right) {
  const std::array<exact::ExactRational, 4> candidates{
      left.lower * right.lower,
      left.lower * right.upper,
      left.upper * right.lower,
      left.upper * right.upper};
  const auto [minimum, maximum] =
      std::minmax_element(candidates.begin(), candidates.end());
  return Interval{*minimum, *maximum};
}

[[nodiscard]] Interval square(const Interval& value) {
  const exact::ExactRational lower_squared = value.lower * value.lower;
  const exact::ExactRational upper_squared = value.upper * value.upper;
  const exact::ExactRational maximum =
      std::max(lower_squared, upper_squared);
  const exact::ExactRational zero;
  if (value.lower <= zero && zero <= value.upper) {
    return Interval{zero, maximum};
  }
  return Interval{std::min(lower_squared, upper_squared), maximum};
}

[[nodiscard]] Interval scale_by_two(const Interval& value) {
  const exact::ExactRational two{exact::BigInt{2}};
  return Interval{two * value.lower, two * value.upper};
}

[[nodiscard]] Interval dot(
    const Vector3& left,
    const Vector3& right,
    bool same_vector) {
  Interval result = singleton(exact::ExactRational{});
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result = add(
        result,
        same_vector ? square(left[axis])
                    : multiply(left[axis], right[axis]));
  }
  return result;
}

[[nodiscard]] Interval determinant(
    const Matrix3& matrix,
    std::size_t dimension) {
  if (dimension == 1U) {
    return matrix[0][0];
  }
  if (dimension == 2U) {
    return subtract(
        multiply(matrix[0][0], matrix[1][1]),
        multiply(matrix[0][1], matrix[1][0]));
  }
  if (dimension != 3U) {
    throw std::invalid_argument(
        "a higher-support determinant requires dimension two or three");
  }
  const Interval first_minor = subtract(
      multiply(matrix[1][1], matrix[2][2]),
      multiply(matrix[1][2], matrix[2][1]));
  const Interval second_minor = subtract(
      multiply(matrix[1][0], matrix[2][2]),
      multiply(matrix[1][2], matrix[2][0]));
  const Interval third_minor = subtract(
      multiply(matrix[1][0], matrix[2][1]),
      multiply(matrix[1][1], matrix[2][0]));
  return add(
      subtract(
          multiply(matrix[0][0], first_minor),
          multiply(matrix[0][1], second_minor)),
      multiply(matrix[0][2], third_minor));
}

[[nodiscard]] ExactBoxCoordinates exact_box_coordinates(
    const spatial::ExactDyadicAabb3& box) {
  ExactBoxCoordinates coordinates;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t lower_bits =
        exact::canonicalize_binary64_bits(box.lower_binary64_bits[axis]);
    const std::uint64_t upper_bits =
        exact::canonicalize_binary64_bits(box.upper_binary64_bits[axis]);
    coordinates.lower[axis] =
        exact::ExactRational::from_binary64_bits(lower_bits);
    coordinates.upper[axis] =
        exact::ExactRational::from_binary64_bits(upper_bits);
    if (coordinates.upper[axis] < coordinates.lower[axis]) {
      throw std::invalid_argument(
          "an exact dyadic AABB has a reversed axis");
    }
  }
  return coordinates;
}

[[nodiscard]] Vector3 difference_box(
    const ExactBoxCoordinates& left,
    const ExactBoxCoordinates& right) {
  Vector3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result[axis] = Interval{
        left.lower[axis] - right.upper[axis],
        left.upper[axis] - right.lower[axis]};
  }
  return result;
}

[[nodiscard]] std::array<exact::ExactRational, 3>
triangle_vertex_dot_upper_bounds(
    std::span<const ExactBoxCoordinates> boxes) {
  if (boxes.size() != 3U) {
    throw std::invalid_argument(
        "triangle angle bounds require exactly three support boxes");
  }
  std::array<exact::ExactRational, 3> result{};
  for (std::size_t vertex = 0U; vertex < boxes.size(); ++vertex) {
    const std::size_t first = (vertex + 1U) % boxes.size();
    const std::size_t second = (vertex + 2U) % boxes.size();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const std::array<exact::ExactRational, 2> vertex_endpoints{
          boxes[vertex].lower[axis], boxes[vertex].upper[axis]};
      const std::array<exact::ExactRational, 2> first_endpoints{
          boxes[first].lower[axis], boxes[first].upper[axis]};
      const std::array<exact::ExactRational, 2> second_endpoints{
          boxes[second].lower[axis], boxes[second].upper[axis]};
      bool initialized = false;
      exact::ExactRational axis_maximum;
      for (const exact::ExactRational& vertex_value : vertex_endpoints) {
        for (const exact::ExactRational& first_value : first_endpoints) {
          for (const exact::ExactRational& second_value : second_endpoints) {
            const exact::ExactRational candidate =
                (first_value - vertex_value) *
                (second_value - vertex_value);
            if (!initialized || candidate > axis_maximum) {
              initialized = true;
              axis_maximum = candidate;
            }
          }
        }
      }
      if (!initialized) {
        throw std::logic_error(
            "a triangle angle bound omitted every endpoint candidate");
      }
      result[vertex] = result[vertex] + axis_maximum;
    }
  }
  return result;
}

struct SupportIntervalEvaluation {
  std::size_t support_size{};
  std::size_t dimension{};
  std::array<ExactBoxCoordinates, 4> boxes{};
  ExactBoxCoordinates anchor{};
  std::array<Vector3, 3> directions{};
  Matrix3 gram{};
  std::array<Interval, 3> squared_direction_norms{};
  std::array<Interval, 3> cramer_numerators{};
  Interval gram_determinant{};
};

[[nodiscard]] SupportIntervalEvaluation evaluate_support(
    std::span<const spatial::ExactDyadicAabb3> support_boxes) {
  if (support_boxes.size() != 3U && support_boxes.size() != 4U) {
    throw std::invalid_argument(
        "a higher-support AABB product requires three or four boxes");
  }
  SupportIntervalEvaluation result;
  result.support_size = support_boxes.size();
  result.dimension = support_boxes.size() - 1U;
  for (std::size_t index = 0U; index < support_boxes.size(); ++index) {
    result.boxes[index] = exact_box_coordinates(support_boxes[index]);
  }
  result.anchor = result.boxes[0];
  for (std::size_t direction = 0U;
       direction < result.dimension;
       ++direction) {
    result.directions[direction] =
        difference_box(result.boxes[direction + 1U], result.boxes[0]);
  }
  for (std::size_t row = 0U; row < result.dimension; ++row) {
    for (std::size_t column = 0U;
         column < result.dimension;
         ++column) {
      result.gram[row][column] = dot(
          result.directions[row],
          result.directions[column],
          row == column);
    }
    result.squared_direction_norms[row] = result.gram[row][row];
  }
  result.gram_determinant =
      determinant(result.gram, result.dimension);
  for (std::size_t column = 0U;
       column < result.dimension;
       ++column) {
    Matrix3 replaced = result.gram;
    for (std::size_t row = 0U; row < result.dimension; ++row) {
      replaced[row][column] = result.squared_direction_norms[row];
    }
    result.cramer_numerators[column] =
        determinant(replaced, result.dimension);
  }
  return result;
}

[[nodiscard]] std::array<Interval, 4> barycentric_numerators(
    const SupportIntervalEvaluation& support) {
  std::array<Interval, 4> result{};
  Interval sum = singleton(exact::ExactRational{});
  for (std::size_t index = 0U; index < support.dimension; ++index) {
    result[index + 1U] = support.cramer_numerators[index];
    sum = add(sum, support.cramer_numerators[index]);
  }
  result[0] = subtract(scale_by_two(support.gram_determinant), sum);
  return result;
}

[[nodiscard]] Interval query_scaled_power_for_coordinates(
    const SupportIntervalEvaluation& support,
    const ExactBoxCoordinates& query) {
  const Vector3 delta = difference_box(query, support.anchor);
  Interval result = multiply(
      support.gram_determinant,
      dot(delta, delta, true));
  for (std::size_t index = 0U; index < support.dimension; ++index) {
    result = subtract(
        result,
        multiply(
            dot(support.directions[index], delta, false),
            support.cramer_numerators[index]));
  }
  return result;
}

[[nodiscard]] Interval query_scaled_power(
    const SupportIntervalEvaluation& support,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactBoxCoordinates query = exact_box_coordinates(query_box);
  Interval result = query_scaled_power_for_coordinates(support, query);

  // For every actual support, Delta is a Gram determinant and is therefore
  // nonnegative.  The query polynomial has Hessian 2 Delta I, so its maximum
  // on a box is attained at a corner.  Bounding the support variables at each
  // of the eight query corners is consequently a second safe upper bound.
  bool initialized = false;
  exact::ExactRational corner_upper;
  for (std::size_t selector = 0U; selector < 8U; ++selector) {
    ExactBoxCoordinates corner;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const bool upper = (selector & (std::size_t{1} << axis)) != 0U;
      const exact::ExactRational coordinate =
          upper ? query.upper[axis] : query.lower[axis];
      corner.lower[axis] = coordinate;
      corner.upper[axis] = coordinate;
    }
    const Interval candidate =
        query_scaled_power_for_coordinates(support, corner);
    if (!initialized || candidate.upper > corner_upper) {
      initialized = true;
      corner_upper = candidate.upper;
    }
  }
  if (!initialized) {
    throw std::logic_error(
        "a higher-support query box omitted every corner");
  }
  result.upper = std::min(result.upper, corner_upper);
  if (result.upper < result.lower) {
    throw std::logic_error(
        "intersected higher-support power bounds are reversed");
  }
  return result;
}

// A common dyadic exponent turns the decision-only interval predicates into
// exact integer arithmetic.  With at most 124 magnitude bits per aligned
// coordinate, a direction needs at most 125 bits, a Gram entry at most
// 252 bits, a tetrahedral Gram/Cramer determinant at most 759 bits, and
// the complete scaled-power expression at most 1013 bits.  Inputs outside
// this envelope fall back to the arbitrary-precision rational analysis before
// any scientific decision is returned.
// Writing every endpoint as x=2^e X also proves decision parity: triangle
// dot bounds have degree 2, Gram/Cramer and barycentric numerators degree 2d,
// and scaled power degree 2d+2.  Each integer interval endpoint is therefore
// its rational counterpart multiplied by one strictly positive common power
// of two, including exact zero and the eight-corner query intersection.
using BoundedExactInteger = boost::multiprecision::int1024_t;
constexpr std::uint64_t bounded_exact_coordinate_bit_limit = 124U;
static_assert(
    std::numeric_limits<BoundedExactInteger>::digits >= 1013,
    "the bounded higher-support kernel needs at least 1013 magnitude bits");

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

constexpr std::size_t maximum_bounded_endpoint_count = 30U;

[[nodiscard]] std::optional<
    std::array<BoundedExactInteger, maximum_bounded_endpoint_count>>
try_align_bounded_exact_dyadics(
    const std::array<
        Binary64DyadicWord,
        maximum_bounded_endpoint_count>& words,
    std::size_t word_count) {
  if (word_count > words.size()) {
    throw std::logic_error(
        "a bounded higher-support alignment exceeds its fixed word count");
  }
  int minimum_exponent = 0;
  bool exponent_initialized = false;
  for (std::size_t index = 0U; index < word_count; ++index) {
    const Binary64DyadicWord& word = words[index];
    if (word.magnitude == 0U) {
      continue;
    }
    if (!exponent_initialized || word.exponent < minimum_exponent) {
      minimum_exponent = word.exponent;
      exponent_initialized = true;
    }
  }

  std::array<
      BoundedExactInteger,
      maximum_bounded_endpoint_count> aligned{};
  if (!exponent_initialized) {
    return aligned;
  }
  for (std::size_t index = 0U; index < word_count; ++index) {
    const Binary64DyadicWord& word = words[index];
    if (word.magnitude == 0U) {
      continue;
    }
    const std::int64_t shift =
        static_cast<std::int64_t>(word.exponent) -
        static_cast<std::int64_t>(minimum_exponent);
    const std::uint64_t magnitude_bit_count =
        static_cast<std::uint64_t>(std::bit_width(word.magnitude));
    if (magnitude_bit_count > bounded_exact_coordinate_bit_limit ||
        shift < 0 ||
        static_cast<std::uint64_t>(shift) >
            bounded_exact_coordinate_bit_limit -
                magnitude_bit_count) {
      return std::nullopt;
    }
    BoundedExactInteger value{word.magnitude};
    value <<= static_cast<unsigned int>(shift);
    aligned[index] = word.negative ? -value : value;
  }
  return aligned;
}

struct BoundedInterval {
  BoundedExactInteger lower{};
  BoundedExactInteger upper{};
};

using BoundedVector3 = std::array<BoundedInterval, 3>;
using BoundedMatrix3 =
    std::array<std::array<BoundedInterval, 3>, 3>;

struct BoundedBoxCoordinates {
  std::array<BoundedExactInteger, 3> lower{};
  std::array<BoundedExactInteger, 3> upper{};
};

struct BoundedProductCoordinates {
  std::size_t support_size{};
  std::array<BoundedBoxCoordinates, 4> support_boxes{};
  std::optional<BoundedBoxCoordinates> query_box;
};

[[nodiscard]] BoundedInterval bounded_singleton(
    BoundedExactInteger value) {
  return BoundedInterval{value, std::move(value)};
}

[[nodiscard]] BoundedInterval bounded_add(
    const BoundedInterval& left,
    const BoundedInterval& right) {
  return BoundedInterval{
      left.lower + right.lower,
      left.upper + right.upper};
}

[[nodiscard]] BoundedInterval bounded_subtract(
    const BoundedInterval& left,
    const BoundedInterval& right) {
  return BoundedInterval{
      left.lower - right.upper,
      left.upper - right.lower};
}

[[nodiscard]] BoundedInterval bounded_multiply(
    const BoundedInterval& left,
    const BoundedInterval& right) {
  const std::array<BoundedExactInteger, 4> candidates{
      left.lower * right.lower,
      left.lower * right.upper,
      left.upper * right.lower,
      left.upper * right.upper};
  const auto [minimum, maximum] =
      std::minmax_element(candidates.begin(), candidates.end());
  return BoundedInterval{*minimum, *maximum};
}

[[nodiscard]] BoundedInterval bounded_square(
    const BoundedInterval& value) {
  const BoundedExactInteger lower_squared =
      value.lower * value.lower;
  const BoundedExactInteger upper_squared =
      value.upper * value.upper;
  const BoundedExactInteger maximum =
      std::max(lower_squared, upper_squared);
  const BoundedExactInteger zero{};
  if (value.lower <= zero && zero <= value.upper) {
    return BoundedInterval{zero, maximum};
  }
  return BoundedInterval{
      std::min(lower_squared, upper_squared),
      maximum};
}

[[nodiscard]] BoundedInterval bounded_scale_by_two(
    const BoundedInterval& value) {
  const BoundedExactInteger two{2};
  return BoundedInterval{
      two * value.lower,
      two * value.upper};
}

[[nodiscard]] BoundedInterval bounded_dot(
    const BoundedVector3& left,
    const BoundedVector3& right,
    bool same_vector) {
  BoundedInterval result =
      bounded_singleton(BoundedExactInteger{});
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result = bounded_add(
        result,
        same_vector
            ? bounded_square(left[axis])
            : bounded_multiply(left[axis], right[axis]));
  }
  return result;
}

[[nodiscard]] BoundedInterval bounded_determinant(
    const BoundedMatrix3& matrix,
    std::size_t dimension) {
  if (dimension == 1U) {
    return matrix[0][0];
  }
  if (dimension == 2U) {
    return bounded_subtract(
        bounded_multiply(matrix[0][0], matrix[1][1]),
        bounded_multiply(matrix[0][1], matrix[1][0]));
  }
  if (dimension != 3U) {
    throw std::invalid_argument(
        "a bounded higher-support determinant requires dimension two or three");
  }
  const BoundedInterval first_minor = bounded_subtract(
      bounded_multiply(matrix[1][1], matrix[2][2]),
      bounded_multiply(matrix[1][2], matrix[2][1]));
  const BoundedInterval second_minor = bounded_subtract(
      bounded_multiply(matrix[1][0], matrix[2][2]),
      bounded_multiply(matrix[1][2], matrix[2][0]));
  const BoundedInterval third_minor = bounded_subtract(
      bounded_multiply(matrix[1][0], matrix[2][1]),
      bounded_multiply(matrix[1][1], matrix[2][0]));
  return bounded_add(
      bounded_subtract(
          bounded_multiply(matrix[0][0], first_minor),
          bounded_multiply(matrix[0][1], second_minor)),
      bounded_multiply(matrix[0][2], third_minor));
}

[[nodiscard]] BoundedBoxCoordinates bounded_box_coordinates(
    const std::array<
        BoundedExactInteger,
        maximum_bounded_endpoint_count>& aligned,
    std::size_t offset) {
  BoundedBoxCoordinates result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower[axis] = aligned[offset + axis];
    result.upper[axis] = aligned[offset + 3U + axis];
  }
  return result;
}

[[nodiscard]] std::optional<BoundedProductCoordinates>
try_bounded_product_coordinates(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3* query_box) {
  if (support_boxes.size() != 3U && support_boxes.size() != 4U) {
    throw std::invalid_argument(
        "a higher-support AABB product requires three or four boxes");
  }

  std::array<
      Binary64DyadicWord,
      maximum_bounded_endpoint_count> words{};
  const auto append_box =
      [&words](
          const spatial::ExactDyadicAabb3& box,
          std::size_t offset) {
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
          const std::uint64_t lower_bits =
              exact::canonicalize_binary64_bits(
                  box.lower_binary64_bits[axis]);
          const std::uint64_t upper_bits =
              exact::canonicalize_binary64_bits(
                  box.upper_binary64_bits[axis]);
          if (exact::binary64_total_order_key(upper_bits) <
              exact::binary64_total_order_key(lower_bits)) {
            throw std::invalid_argument(
                "an exact dyadic AABB has a reversed axis");
          }
          words[offset + axis] =
              decode_binary64_dyadic_word(lower_bits);
          words[offset + 3U + axis] =
              decode_binary64_dyadic_word(upper_bits);
        }
      };

  for (std::size_t index = 0U;
       index < support_boxes.size();
       ++index) {
    append_box(support_boxes[index], index * 6U);
  }
  const std::size_t support_word_count =
      support_boxes.size() * 6U;
  std::size_t word_count = support_word_count;
  if (query_box != nullptr) {
    append_box(*query_box, support_word_count);
    word_count += 6U;
  }

  const std::optional<std::array<
      BoundedExactInteger,
      maximum_bounded_endpoint_count>> aligned =
      try_align_bounded_exact_dyadics(words, word_count);
  if (!aligned.has_value()) {
    return std::nullopt;
  }

  BoundedProductCoordinates result;
  result.support_size = support_boxes.size();
  for (std::size_t index = 0U;
       index < support_boxes.size();
       ++index) {
    result.support_boxes[index] =
        bounded_box_coordinates(*aligned, index * 6U);
  }
  if (query_box != nullptr) {
    result.query_box =
        bounded_box_coordinates(*aligned, support_word_count);
  }
  return result;
}

[[nodiscard]] BoundedVector3 bounded_difference_box(
    const BoundedBoxCoordinates& left,
    const BoundedBoxCoordinates& right) {
  BoundedVector3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result[axis] = BoundedInterval{
        left.lower[axis] - right.upper[axis],
        left.upper[axis] - right.lower[axis]};
  }
  return result;
}

[[nodiscard]] std::array<BoundedExactInteger, 3>
bounded_triangle_vertex_dot_upper_bounds(
    std::span<const BoundedBoxCoordinates> boxes) {
  if (boxes.size() != 3U) {
    throw std::invalid_argument(
        "bounded triangle angle bounds require exactly three support boxes");
  }
  std::array<BoundedExactInteger, 3> result{};
  for (std::size_t vertex = 0U; vertex < boxes.size(); ++vertex) {
    const std::size_t first = (vertex + 1U) % boxes.size();
    const std::size_t second = (vertex + 2U) % boxes.size();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const std::array<BoundedExactInteger, 2> vertex_endpoints{
          boxes[vertex].lower[axis],
          boxes[vertex].upper[axis]};
      const std::array<BoundedExactInteger, 2> first_endpoints{
          boxes[first].lower[axis],
          boxes[first].upper[axis]};
      const std::array<BoundedExactInteger, 2> second_endpoints{
          boxes[second].lower[axis],
          boxes[second].upper[axis]};
      bool initialized = false;
      BoundedExactInteger axis_maximum{};
      for (const BoundedExactInteger& vertex_value :
           vertex_endpoints) {
        for (const BoundedExactInteger& first_value :
             first_endpoints) {
          for (const BoundedExactInteger& second_value :
               second_endpoints) {
            const BoundedExactInteger candidate =
                (first_value - vertex_value) *
                (second_value - vertex_value);
            if (!initialized || candidate > axis_maximum) {
              initialized = true;
              axis_maximum = candidate;
            }
          }
        }
      }
      if (!initialized) {
        throw std::logic_error(
            "a bounded triangle angle bound omitted every endpoint candidate");
      }
      result[vertex] += axis_maximum;
    }
  }
  return result;
}

struct BoundedSupportIntervalEvaluation {
  std::size_t support_size{};
  std::size_t dimension{};
  std::array<BoundedBoxCoordinates, 4> boxes{};
  BoundedBoxCoordinates anchor{};
  std::array<BoundedVector3, 3> directions{};
  BoundedMatrix3 gram{};
  std::array<BoundedInterval, 3> squared_direction_norms{};
  std::array<BoundedInterval, 3> cramer_numerators{};
  BoundedInterval gram_determinant{};
};

[[nodiscard]] BoundedSupportIntervalEvaluation
bounded_evaluate_support(
    const BoundedProductCoordinates& coordinates) {
  BoundedSupportIntervalEvaluation result;
  result.support_size = coordinates.support_size;
  result.dimension = coordinates.support_size - 1U;
  result.boxes = coordinates.support_boxes;
  result.anchor = result.boxes[0];
  for (std::size_t direction = 0U;
       direction < result.dimension;
       ++direction) {
    result.directions[direction] =
        bounded_difference_box(
            result.boxes[direction + 1U],
            result.boxes[0]);
  }
  for (std::size_t row = 0U; row < result.dimension; ++row) {
    for (std::size_t column = 0U;
         column < result.dimension;
         ++column) {
      result.gram[row][column] = bounded_dot(
          result.directions[row],
          result.directions[column],
          row == column);
    }
    result.squared_direction_norms[row] =
        result.gram[row][row];
  }
  result.gram_determinant =
      bounded_determinant(result.gram, result.dimension);
  for (std::size_t column = 0U;
       column < result.dimension;
       ++column) {
    BoundedMatrix3 replaced = result.gram;
    for (std::size_t row = 0U; row < result.dimension; ++row) {
      replaced[row][column] =
          result.squared_direction_norms[row];
    }
    result.cramer_numerators[column] =
        bounded_determinant(replaced, result.dimension);
  }
  return result;
}

[[nodiscard]] std::array<BoundedInterval, 4>
bounded_barycentric_numerators(
    const BoundedSupportIntervalEvaluation& support) {
  std::array<BoundedInterval, 4> result{};
  BoundedInterval sum =
      bounded_singleton(BoundedExactInteger{});
  for (std::size_t index = 0U;
       index < support.dimension;
       ++index) {
    result[index + 1U] = support.cramer_numerators[index];
    sum = bounded_add(sum, support.cramer_numerators[index]);
  }
  result[0] = bounded_subtract(
      bounded_scale_by_two(support.gram_determinant),
      sum);
  return result;
}

[[nodiscard]] BoundedInterval
bounded_query_scaled_power_for_coordinates(
    const BoundedSupportIntervalEvaluation& support,
    const BoundedBoxCoordinates& query) {
  const BoundedVector3 delta =
      bounded_difference_box(query, support.anchor);
  BoundedInterval result = bounded_multiply(
      support.gram_determinant,
      bounded_dot(delta, delta, true));
  for (std::size_t index = 0U;
       index < support.dimension;
       ++index) {
    result = bounded_subtract(
        result,
        bounded_multiply(
            bounded_dot(
                support.directions[index],
                delta,
                false),
            support.cramer_numerators[index]));
  }
  return result;
}

[[nodiscard]] BoundedInterval bounded_query_scaled_power(
    const BoundedSupportIntervalEvaluation& support,
    const BoundedBoxCoordinates& query) {
  BoundedInterval result =
      bounded_query_scaled_power_for_coordinates(support, query);
  bool initialized = false;
  BoundedExactInteger corner_upper{};
  for (std::size_t selector = 0U; selector < 8U; ++selector) {
    BoundedBoxCoordinates corner;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const bool upper =
          (selector & (std::size_t{1} << axis)) != 0U;
      const BoundedExactInteger coordinate =
          upper ? query.upper[axis] : query.lower[axis];
      corner.lower[axis] = coordinate;
      corner.upper[axis] = coordinate;
    }
    const BoundedInterval candidate =
        bounded_query_scaled_power_for_coordinates(
            support,
            corner);
    if (!initialized || candidate.upper > corner_upper) {
      initialized = true;
      corner_upper = candidate.upper;
    }
  }
  if (!initialized) {
    throw std::logic_error(
        "a bounded higher-support query box omitted every corner");
  }
  result.upper = std::min(result.upper, corner_upper);
  if (result.upper < result.lower) {
    throw std::logic_error(
        "intersected bounded higher-support power bounds are reversed");
  }
  return result;
}

[[nodiscard]] std::optional<bool>
try_bounded_no_well_centered_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes) {
  const std::optional<BoundedProductCoordinates> coordinates =
      try_bounded_product_coordinates(support_boxes, nullptr);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  const BoundedSupportIntervalEvaluation support =
      bounded_evaluate_support(*coordinates);
  const BoundedExactInteger zero{};
  if (support.gram_determinant.upper <= zero) {
    return true;
  }
  if (support.support_size == 3U) {
    const std::array<BoundedExactInteger, 3> angle_bounds =
        bounded_triangle_vertex_dot_upper_bounds(
            std::span<const BoundedBoxCoordinates>{
                support.boxes.data(),
                support.support_size});
    for (const BoundedExactInteger& upper : angle_bounds) {
      if (upper <= zero) {
        return true;
      }
    }
  }
  const std::array<BoundedInterval, 4> barycentric =
      bounded_barycentric_numerators(support);
  for (std::size_t index = 0U;
       index < support.support_size;
       ++index) {
    if (barycentric[index].upper <= zero) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<bool>
try_bounded_all_well_centered_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes) {
  const std::optional<BoundedProductCoordinates> coordinates =
      try_bounded_product_coordinates(support_boxes, nullptr);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  const BoundedSupportIntervalEvaluation support =
      bounded_evaluate_support(*coordinates);
  const BoundedExactInteger zero{};
  if (support.gram_determinant.lower <= zero) {
    return false;
  }
  const std::array<BoundedInterval, 4> barycentric =
      bounded_barycentric_numerators(support);
  for (std::size_t index = 0U;
       index < support.support_size;
       ++index) {
    if (barycentric[index].lower <= zero) {
      return false;
    }
  }
  return true;
}

void validate_terminal_singleton_support_boxes(
    std::span<const spatial::ExactDyadicAabb3> support_boxes) {
  if (support_boxes.size() != 3U && support_boxes.size() != 4U) {
    throw std::invalid_argument(
        "a terminal higher-support decision requires three or four boxes");
  }
  for (const spatial::ExactDyadicAabb3& box : support_boxes) {
    const ExactBoxCoordinates coordinates = exact_box_coordinates(box);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (coordinates.lower[axis] != coordinates.upper[axis]) {
        throw std::invalid_argument(
            "a terminal higher-support decision requires singleton boxes");
      }
    }
  }
}

[[nodiscard]] std::optional<ExactHigherSupportTerminalGeometryDecision>
try_bounded_terminal_geometry_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes) {
  const std::optional<BoundedProductCoordinates> coordinates =
      try_bounded_product_coordinates(support_boxes, nullptr);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  const BoundedSupportIntervalEvaluation support =
      bounded_evaluate_support(*coordinates);
  const BoundedExactInteger zero{};
  if (support.gram_determinant.lower !=
      support.gram_determinant.upper) {
    throw std::logic_error(
        "a singleton support produced a nonsingleton Gram determinant");
  }
  if (support.gram_determinant.lower == zero) {
    return ExactHigherSupportTerminalGeometryDecision::affinely_dependent;
  }
  if (support.gram_determinant.lower < zero) {
    throw std::logic_error(
        "an exact singleton Gram determinant became negative");
  }

  const std::array<BoundedInterval, 4> barycentric =
      bounded_barycentric_numerators(support);
  bool has_zero = false;
  for (std::size_t index = 0U; index < support.support_size; ++index) {
    if (barycentric[index].lower != barycentric[index].upper) {
      throw std::logic_error(
          "a singleton support produced a nonsingleton barycentric value");
    }
    if (barycentric[index].lower < zero) {
      return ExactHigherSupportTerminalGeometryDecision::
          exterior_circumcenter;
    }
    has_zero = has_zero || barycentric[index].lower == zero;
  }
  return has_zero
      ? ExactHigherSupportTerminalGeometryDecision::boundary_reduced
      : ExactHigherSupportTerminalGeometryDecision::minimal;
}

[[nodiscard]] std::optional<ExactHigherSupportProductQueryCellDecision>
try_bounded_query_cell_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3& query_box) {
  const std::optional<BoundedProductCoordinates> coordinates =
      try_bounded_product_coordinates(support_boxes, &query_box);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  if (!coordinates->query_box.has_value()) {
    throw std::logic_error(
        "a bounded higher-support query decision omitted its query box");
  }
  const BoundedSupportIntervalEvaluation support =
      bounded_evaluate_support(*coordinates);
  const BoundedInterval power = bounded_query_scaled_power(
      support,
      *coordinates->query_box);
  const BoundedExactInteger zero{};
  if (power.upper < zero) {
    return ExactHigherSupportProductQueryCellDecision::
        strictly_inside_every_independent_sphere;
  }
  if (power.lower >= zero) {
    return ExactHigherSupportProductQueryCellDecision::
        outside_or_boundary_every_independent_sphere;
  }
  return ExactHigherSupportProductQueryCellDecision::inconclusive;
}

}  // namespace

bool ExactHigherSupportProductAabbAnalysis::
all_supports_affinely_dependent_certified() const {
  return support_size >= 3U && support_size <= 4U &&
         gram_determinant.upper <= exact::ExactRational{};
}

bool ExactHigherSupportProductAabbAnalysis::
no_well_centered_support_certified() const {
  if (support_size != 3U && support_size != 4U) {
    return false;
  }
  if (all_supports_affinely_dependent_certified()) {
    return true;
  }
  const exact::ExactRational zero;
  if (triangle_vertex_dot_upper_bounds.has_value()) {
    for (const exact::ExactRational& upper :
         *triangle_vertex_dot_upper_bounds) {
      if (upper <= zero) {
        return true;
      }
    }
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (barycentric_numerators[index].upper <= zero) {
      return true;
    }
  }
  return false;
}

bool ExactHigherSupportProductAabbAnalysis::
all_supports_well_centered_certified() const {
  if (support_size != 3U && support_size != 4U) {
    return false;
  }
  const exact::ExactRational zero;
  if (gram_determinant.lower <= zero) {
    return false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (barycentric_numerators[index].lower <= zero) {
      return false;
    }
  }
  return true;
}

bool ExactHigherSupportProductAabbAnalysis::
query_strictly_inside_every_independent_sphere_certified() const {
  return (support_size == 3U || support_size == 4U) &&
         query_scaled_power.has_value() &&
         query_scaled_power->upper.sign() < 0;
}

ExactHigherSupportProductAabbAnalysis
exact_higher_support_product_aabb_analysis(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    std::optional<spatial::ExactDyadicAabb3> query_box) {
  const SupportIntervalEvaluation support = evaluate_support(support_boxes);
  ExactHigherSupportProductAabbAnalysis result;
  result.support_size = support.support_size;
  result.gram_determinant = support.gram_determinant;
  result.barycentric_numerators = barycentric_numerators(support);
  if (support.support_size == 3U) {
    result.triangle_vertex_dot_upper_bounds =
        triangle_vertex_dot_upper_bounds(
            std::span<const ExactBoxCoordinates>{
                support.boxes.data(), support.support_size});
  }
  if (query_box.has_value()) {
    result.query_scaled_power =
        query_scaled_power(support, *query_box);
  }
  return result;
}

bool exact_higher_support_product_no_well_centered_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend) {
  const std::optional<bool> bounded =
      try_bounded_no_well_centered_decision(support_boxes);
  if (bounded.has_value()) {
    if (backend != nullptr) {
      *backend =
          ExactHigherSupportProductAabbDecisionBackend::
              bounded_dyadic_int1024;
    }
    return *bounded;
  }
  if (backend != nullptr) {
    *backend =
        ExactHigherSupportProductAabbDecisionBackend::
            arbitrary_precision_rational;
  }
  return exact_higher_support_product_aabb_analysis(support_boxes)
      .no_well_centered_support_certified();
}

bool exact_higher_support_product_all_well_centered_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend) {
  const std::optional<bool> bounded =
      try_bounded_all_well_centered_decision(support_boxes);
  if (bounded.has_value()) {
    if (backend != nullptr) {
      *backend =
          ExactHigherSupportProductAabbDecisionBackend::
              bounded_dyadic_int1024;
    }
    return *bounded;
  }
  if (backend != nullptr) {
    *backend =
        ExactHigherSupportProductAabbDecisionBackend::
            arbitrary_precision_rational;
  }
  return exact_higher_support_product_aabb_analysis(support_boxes)
      .all_supports_well_centered_certified();
}

ExactHigherSupportProductQueryCellDecision
exact_higher_support_product_query_cell_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3& query_box,
    ExactHigherSupportProductAabbDecisionBackend* backend) {
  const std::optional<ExactHigherSupportProductQueryCellDecision> bounded =
      try_bounded_query_cell_decision(
          support_boxes,
          query_box);
  if (bounded.has_value()) {
    if (backend != nullptr) {
      *backend =
          ExactHigherSupportProductAabbDecisionBackend::
              bounded_dyadic_int1024;
    }
    return *bounded;
  }
  if (backend != nullptr) {
    *backend =
        ExactHigherSupportProductAabbDecisionBackend::
            arbitrary_precision_rational;
  }
  const ExactHigherSupportProductAabbAnalysis analysis =
      exact_higher_support_product_aabb_analysis(
          support_boxes,
          query_box);
  if (!analysis.query_scaled_power.has_value()) {
    throw std::logic_error(
        "a higher-support query-cell analysis omitted its power interval");
  }
  if (analysis.query_scaled_power->upper.sign() < 0) {
    return ExactHigherSupportProductQueryCellDecision::
        strictly_inside_every_independent_sphere;
  }
  if (analysis.query_scaled_power->lower.sign() >= 0) {
    return ExactHigherSupportProductQueryCellDecision::
        outside_or_boundary_every_independent_sphere;
  }
  return ExactHigherSupportProductQueryCellDecision::inconclusive;
}

ExactHigherSupportTerminalGeometryDecision
exact_higher_support_terminal_geometry_decision(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    ExactHigherSupportProductAabbDecisionBackend* backend) {
  validate_terminal_singleton_support_boxes(support_boxes);
  const std::optional<ExactHigherSupportTerminalGeometryDecision> bounded =
      try_bounded_terminal_geometry_decision(support_boxes);
  if (bounded.has_value()) {
    if (backend != nullptr) {
      *backend = ExactHigherSupportProductAabbDecisionBackend::
          bounded_dyadic_int1024;
    }
    return *bounded;
  }

  if (backend != nullptr) {
    *backend = ExactHigherSupportProductAabbDecisionBackend::
        arbitrary_precision_rational;
  }
  const ExactHigherSupportProductAabbAnalysis analysis =
      exact_higher_support_product_aabb_analysis(support_boxes);
  const exact::ExactRational zero;
  if (analysis.gram_determinant.lower !=
      analysis.gram_determinant.upper) {
    throw std::logic_error(
        "a singleton support produced a nonsingleton rational determinant");
  }
  if (analysis.gram_determinant.lower == zero) {
    return ExactHigherSupportTerminalGeometryDecision::affinely_dependent;
  }
  if (analysis.gram_determinant.lower < zero) {
    throw std::logic_error(
        "an exact rational singleton Gram determinant became negative");
  }
  bool has_zero = false;
  for (std::size_t index = 0U; index < analysis.support_size; ++index) {
    const ExactRationalInterval& numerator =
        analysis.barycentric_numerators[index];
    if (numerator.lower != numerator.upper) {
      throw std::logic_error(
          "a singleton support produced a nonsingleton rational "
          "barycentric value");
    }
    if (numerator.lower < zero) {
      return ExactHigherSupportTerminalGeometryDecision::
          exterior_circumcenter;
    }
    has_zero = has_zero || numerator.lower == zero;
  }
  return has_zero
      ? ExactHigherSupportTerminalGeometryDecision::boundary_reduced
      : ExactHigherSupportTerminalGeometryDecision::minimal;
}

bool
exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
    std::span<const spatial::ExactDyadicAabb3> support_boxes,
    const spatial::ExactDyadicAabb3& query_box,
    ExactHigherSupportProductAabbDecisionBackend* backend) {
  return exact_higher_support_product_query_cell_decision(
             support_boxes,
             query_box,
             backend) ==
      ExactHigherSupportProductQueryCellDecision::
          strictly_inside_every_independent_sphere;
}

}  // namespace morsehgp3d::hierarchy
