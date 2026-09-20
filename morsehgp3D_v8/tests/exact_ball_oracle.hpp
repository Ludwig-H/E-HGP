#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include "core/types.hpp"

// Test-only exact rational geometry. This Gram elimination is independent of
// the product's q3 closed form, q4 Cramer formula and root determinant. It is
// shared by two new judges, not evidence inherited from an older qualification.
namespace mhgp8_test::ball_oracle {
using Big = boost::multiprecision::cpp_int;
using Rational = boost::rational<Big>;
using Vector = std::array<Big, 3>;
using Center = std::array<Rational, 3>;
using Coefficients = std::array<Big, 5>;
using mhgp8::Point3;

inline Big absolute(Big value) { return value < 0 ? Big(-value) : value; }
inline Big gcd(Big first, Big second) {
  first = absolute(std::move(first));
  second = absolute(std::move(second));
  while (second != 0) {
    Big remainder = first % second;
    first = std::move(second);
    second = std::move(remainder);
  }
  return first;
}
inline mhgp8::u64 bits(Big value) {
  value = absolute(std::move(value));
  return value == 0 ? 0 : static_cast<mhgp8::u64>(boost::multiprecision::msb(value)) + 1;
}
inline Vector difference(Point3 a, Point3 b) {
  return {Big(a.x) - b.x, Big(a.y) - b.y, Big(a.z) - b.z};
}
inline Big dot(const Vector& a, const Vector& b) {
  Big result = 0;
  for (std::size_t axis = 0; axis != 3; ++axis) result += a[axis] * b[axis];
  return result;
}

inline std::optional<std::vector<Rational>> solve(std::vector<std::vector<Rational>> matrix) {
  const auto dimension = matrix.size();
  for (std::size_t column = 0; column != dimension; ++column) {
    std::size_t pivot = column;
    while (pivot != dimension && matrix[pivot][column].numerator() == 0) ++pivot;
    if (pivot == dimension) return std::nullopt;
    std::swap(matrix[pivot], matrix[column]);
    const auto divisor = matrix[column][column];
    for (std::size_t entry = column; entry <= dimension; ++entry) matrix[column][entry] /= divisor;
    for (std::size_t row = 0; row != dimension; ++row) {
      if (row == column) continue;
      const auto factor = matrix[row][column];
      for (std::size_t entry = column; entry <= dimension; ++entry)
        matrix[row][entry] -= factor * matrix[column][entry];
    }
  }
  std::vector<Rational> result;
  for (std::size_t row = 0; row != dimension; ++row) result.push_back(matrix[row][dimension]);
  return result;
}

struct Ball {
  Center center;
  Rational radius_squared;
  Coefficients coefficients;
  [[nodiscard]] Rational power(Point3 point) const {
    Rational result = -radius_squared;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      const Rational distance = Rational(Big(point[axis])) - center[axis];
      result += distance * distance;
    }
    return result;
  }
};

enum class Refusal { None, Rank, Boundary, Exterior };
struct Result {
  std::optional<Ball> ball;
  Refusal refusal{Refusal::None};
};

// A non-positive but full-rank support still has a circumsphere. The seed
// consumer judge needs it to group all roots before applying positivity.
inline Result make(std::span<const Point3> points, bool strict_positive = true) {
  if (points.size() < 2 || points.size() > 4) throw std::runtime_error("oracle support arity");
  const auto dimension = points.size() - 1;
  std::vector<Vector> directions;
  for (std::size_t i = 1; i != points.size(); ++i) directions.push_back(difference(points[i], points[0]));
  std::vector<std::vector<Rational>> matrix(dimension, std::vector<Rational>(dimension + 1));
  for (std::size_t row = 0; row != dimension; ++row) {
    for (std::size_t column = 0; column != dimension; ++column)
      matrix[row][column] = Rational(2 * dot(directions[row], directions[column]));
    matrix[row][dimension] = Rational(dot(directions[row], directions[row]));
  }
  const auto lambda = solve(std::move(matrix));
  if (!lambda) return {{}, Refusal::Rank};
  Rational first_weight(1);
  bool boundary = false, exterior = false;
  for (const auto& value : *lambda) {
    first_weight -= value;
    boundary = boundary || value.numerator() == 0;
    exterior = exterior || value.numerator() < 0;
  }
  boundary = boundary || first_weight.numerator() == 0;
  exterior = exterior || first_weight.numerator() < 0;
  const auto refusal = exterior ? Refusal::Exterior : boundary ? Refusal::Boundary : Refusal::None;
  if (strict_positive && refusal != Refusal::None) return {{}, refusal};
  Ball result{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    Rational relative(0);
    for (std::size_t index = 0; index != dimension; ++index)
      relative += (*lambda)[index] * Rational(directions[index][axis]);
    result.center[axis] = Rational(Big(points[0][axis])) + relative;
    result.radius_squared += relative * relative;
  }
  std::array<Rational, 5> polynomial{};
  polynomial[0] = Rational(1);
  polynomial[4] = -result.radius_squared;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    polynomial[axis + 1] = Rational(-2) * result.center[axis];
    polynomial[4] += result.center[axis] * result.center[axis];
  }
  Big denominator = 1;
  for (const auto& coefficient : polynomial)
    denominator = (denominator / gcd(denominator, coefficient.denominator())) * coefficient.denominator();
  for (std::size_t index = 0; index != polynomial.size(); ++index)
    result.coefficients[index] = polynomial[index].numerator() * (denominator / polynomial[index].denominator());
  Big common = result.coefficients[0];
  for (const auto& coefficient : result.coefficients) common = gcd(common, coefficient);
  for (auto& coefficient : result.coefficients) coefficient /= common;
  return {std::move(result), refusal};
}
}  // namespace mhgp8_test::ball_oracle
