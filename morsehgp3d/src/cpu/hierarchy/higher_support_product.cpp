#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include "morsehgp3d/exact/binary64.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
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

}  // namespace morsehgp3d::hierarchy
