#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace morsehgp3d::exact {

// The same decision `analyze_circumcenter_support` takes -- affinely
// dependent, minimal, boundary reduced or exterior circumcentre, with the
// exact centre and squared level of the unique circumsphere -- computed with
// integer determinants instead of normalized rational arithmetic.
//
// Why this exists.  The rational path reduces every intermediate value, and
// reading one coordinate out of an ExactRational3 already builds a reduced
// ExactRational.  On a support of four cloud points that is about a hundred
// and twenty reductions for one analysis, and the higher-support stream runs
// one analysis per terminal support.  Nothing in the decision needs a reduced
// intermediate: only the two published values, the centre and the squared
// level, are canonical, and they are reduced exactly once at the end.
//
// Exactness.  With p_i = N_i / d_i and u_i = p_i - p_0 = U_i / D_i where
// U_i = N_i d_0 - N_0 d_i and D_i = d_i d_0, the circumcentre condition
// |c - p_i|^2 = |c - p_0|^2 is the integer linear system
// (2 D_i U_i) . y = |U_i|^2 in y = c - p_0.  Cramer over the integers gives
// y = Y / det, the centre is (N_0 det + d_0 Y) / (d_0 det), and the squared
// level is |Y|^2 / det^2.  The barycentric signs that classify the support
// are the signs of the same determinants, so no division is ever taken.
struct IntegerCircumcenterAnalysis {
  CircumcenterSupportStatus status{
      CircumcenterSupportStatus::affinely_dependent};
  std::optional<ExactCenter3> center;
  std::optional<ExactLevel> squared_level;
  // Set exactly as CircumcenterSupportAnalysis sets it: every index when the
  // support is minimal, the positive barycentric indices when the centre sits
  // on a face, absent when the centre is outside.
  std::optional<std::uint8_t> reduced_support_mask;
};

namespace integer_circumcenter_detail {

using IntegerVector3 = std::array<BigInt, 3>;

[[nodiscard]] inline IntegerVector3 cross(
    const IntegerVector3& left,
    const IntegerVector3& right) {
  return {
      left[1] * right[2] - left[2] * right[1],
      left[2] * right[0] - left[0] * right[2],
      left[0] * right[1] - left[1] * right[0]};
}

[[nodiscard]] inline BigInt dot(
    const IntegerVector3& left,
    const IntegerVector3& right) {
  return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

[[nodiscard]] inline BigInt determinant(
    const IntegerVector3& first,
    const IntegerVector3& second,
    const IntegerVector3& third) {
  return dot(first, cross(second, third));
}

[[nodiscard]] inline bool is_zero(const IntegerVector3& vector) {
  return vector[0] == 0 && vector[1] == 0 && vector[2] == 0;
}

// u_i = p_i - p_0 in the exact form U / D, with D > 0.
struct RelativeVector {
  IntegerVector3 numerator{};
  BigInt denominator{1};
};

[[nodiscard]] inline RelativeVector relative_vector(
    const ExactRational3& origin,
    const ExactRational3& point) {
  RelativeVector relative;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    relative.numerator[axis] =
        point.numerator(axis) * origin.denominator() -
        origin.numerator(axis) * point.denominator();
  }
  relative.denominator = point.denominator() * origin.denominator();
  return relative;
}

[[nodiscard]] inline int sign_of(const BigInt& value) {
  if (value > 0) {
    return 1;
  }
  return value < 0 ? -1 : 0;
}

// Shared tail: the centre, the squared level and the barycentric signs, from
// the Cramer solution Y / determinant of the support-relative system.
template <std::size_t SupportSize>
[[nodiscard]] IntegerCircumcenterAnalysis finish(
    const ExactRational3& origin,
    const IntegerVector3& solution,
    const BigInt& determinant_value,
    const std::array<BigInt, SupportSize - 1U>& barycentric_numerator,
    const BigInt& barycentric_denominator) {
  IntegerCircumcenterAnalysis analysis;
  analysis.center.emplace(
      origin.numerator(0) * determinant_value +
          origin.denominator() * solution[0],
      origin.numerator(1) * determinant_value +
          origin.denominator() * solution[1],
      origin.numerator(2) * determinant_value +
          origin.denominator() * solution[2],
      origin.denominator() * determinant_value);
  analysis.squared_level.emplace(
      dot(solution, solution), determinant_value * determinant_value);

  // lambda_i = barycentric_numerator[i] / barycentric_denominator for the
  // support points other than the origin, and lambda_origin is the
  // complement.  Only the signs are needed, so the fraction is never taken.
  const int denominator_sign = sign_of(barycentric_denominator);
  std::array<int, SupportSize> signs{};
  BigInt origin_numerator = barycentric_denominator;
  for (std::size_t index = 0U; index + 1U < SupportSize; ++index) {
    signs[index + 1U] =
        sign_of(barycentric_numerator[index]) * denominator_sign;
    origin_numerator -= barycentric_numerator[index];
  }
  signs[0] = sign_of(origin_numerator) * denominator_sign;

  bool has_negative = false;
  bool has_zero = false;
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    has_negative = has_negative || signs[index] < 0;
    has_zero = has_zero || signs[index] == 0;
  }
  if (has_negative) {
    analysis.status = CircumcenterSupportStatus::exterior_circumcenter;
    return analysis;
  }
  std::uint8_t mask = 0U;
  if (has_zero) {
    analysis.status = CircumcenterSupportStatus::boundary_reduced;
    for (std::size_t index = 0U; index < SupportSize; ++index) {
      if (signs[index] > 0) {
        mask = static_cast<std::uint8_t>(
            mask | static_cast<std::uint8_t>(1U << index));
      }
    }
  } else {
    analysis.status = CircumcenterSupportStatus::minimal;
    for (std::size_t index = 0U; index < SupportSize; ++index) {
      mask = static_cast<std::uint8_t>(
          mask | static_cast<std::uint8_t>(1U << index));
    }
  }
  analysis.reduced_support_mask = mask;
  return analysis;
}

}  // namespace integer_circumcenter_detail

[[nodiscard]] inline IntegerCircumcenterAnalysis
analyze_circumcenter_support_integer(
    const std::array<ExactRational3, 4U>& support) {
  using namespace integer_circumcenter_detail;
  std::array<RelativeVector, 3U> relative{
      relative_vector(support[0], support[1]),
      relative_vector(support[0], support[2]),
      relative_vector(support[0], support[3])};
  const BigInt support_determinant = determinant(
      relative[0].numerator, relative[1].numerator, relative[2].numerator);
  if (support_determinant == 0) {
    return IntegerCircumcenterAnalysis{};
  }

  // Rows 2 D_i U_i, right-hand sides |U_i|^2.
  std::array<IntegerVector3, 3U> row{};
  std::array<BigInt, 3U> right_hand_side{};
  for (std::size_t index = 0U; index < 3U; ++index) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      row[index][axis] =
          2 * relative[index].denominator * relative[index].numerator[axis];
    }
    right_hand_side[index] =
        dot(relative[index].numerator, relative[index].numerator);
  }
  // Cramer: the determinant of the system, then one column at a time.
  const auto column_replaced =
      [&](std::size_t replaced_axis) -> std::array<IntegerVector3, 3U> {
    std::array<IntegerVector3, 3U> replaced = row;
    for (std::size_t index = 0U; index < 3U; ++index) {
      replaced[index][replaced_axis] = right_hand_side[index];
    }
    return replaced;
  };
  const BigInt system_determinant = determinant(row[0], row[1], row[2]);
  IntegerVector3 solution{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<IntegerVector3, 3U> replaced = column_replaced(axis);
    solution[axis] = determinant(replaced[0], replaced[1], replaced[2]);
  }
  // The rows are transposed relative to the vector convention used by
  // `determinant`, but a determinant is invariant under transposition, so the
  // same helper serves both.

  // Barycentric numerators: D_i * det(... Y in slot i ...), over
  // system_determinant * support_determinant.
  std::array<BigInt, 3U> barycentric_numerator{
      relative[0].denominator *
          determinant(solution, relative[1].numerator, relative[2].numerator),
      relative[1].denominator *
          determinant(relative[0].numerator, solution, relative[2].numerator),
      relative[2].denominator *
          determinant(relative[0].numerator, relative[1].numerator, solution)};
  return finish<4U>(
      support[0],
      solution,
      system_determinant,
      barycentric_numerator,
      system_determinant * support_determinant);
}

[[nodiscard]] inline IntegerCircumcenterAnalysis
analyze_circumcenter_support_integer(
    const std::array<ExactRational3, 3U>& support) {
  using namespace integer_circumcenter_detail;
  std::array<RelativeVector, 2U> relative{
      relative_vector(support[0], support[1]),
      relative_vector(support[0], support[2])};
  const IntegerVector3 normal =
      cross(relative[0].numerator, relative[1].numerator);
  if (is_zero(normal)) {
    return IntegerCircumcenterAnalysis{};
  }

  // Two equidistance rows, then the plane row that keeps the centre in the
  // affine hull of the triangle.
  std::array<IntegerVector3, 3U> row{};
  std::array<BigInt, 3U> right_hand_side{};
  for (std::size_t index = 0U; index < 2U; ++index) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      row[index][axis] =
          2 * relative[index].denominator * relative[index].numerator[axis];
    }
    right_hand_side[index] =
        dot(relative[index].numerator, relative[index].numerator);
  }
  row[2] = normal;
  right_hand_side[2] = 0;

  const auto column_replaced =
      [&](std::size_t replaced_axis) -> std::array<IntegerVector3, 3U> {
    std::array<IntegerVector3, 3U> replaced = row;
    for (std::size_t index = 0U; index < 3U; ++index) {
      replaced[index][replaced_axis] = right_hand_side[index];
    }
    return replaced;
  };
  const BigInt system_determinant = determinant(row[0], row[1], row[2]);
  IntegerVector3 solution{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<IntegerVector3, 3U> replaced = column_replaced(axis);
    solution[axis] = determinant(replaced[0], replaced[1], replaced[2]);
  }

  // In the plane, the barycentric denominator is det(U_0, U_1, n) = |n|^2.
  const BigInt plane_determinant = dot(normal, normal);
  std::array<BigInt, 2U> barycentric_numerator{
      relative[0].denominator *
          determinant(solution, relative[1].numerator, normal),
      relative[1].denominator *
          determinant(relative[0].numerator, solution, normal)};
  return finish<3U>(
      support[0],
      solution,
      system_determinant,
      barycentric_numerator,
      system_determinant * plane_determinant);
}

}  // namespace morsehgp3d::exact
