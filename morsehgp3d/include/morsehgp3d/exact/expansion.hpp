#pragma once

#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/exact/predicate.hpp"

#include <bit>
#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace morsehgp3d::exact {
namespace detail {

// A zero-eliminated floating expansion ordered from the least significant
// component to the most significant one. All arithmetic is performed inside
// Fp64EnvironmentGuard. Operations fail closed when a binary64 operation is
// non-finite or raises an exception that can invalidate an error-free
// transform. FE_INEXACT is expected and deliberately ignored.
class FloatingExpansion {
 public:
  [[nodiscard]] static FloatingExpansion scalar(double value) {
    if (!std::isfinite(value)) {
      return invalid();
    }
    if (value == 0.0) {
      value = 0.0;
    }
    return FloatingExpansion{true, std::vector<double>{value}};
  }

  [[nodiscard]] static FloatingExpansion invalid() {
    return FloatingExpansion{false, {}};
  }

  [[nodiscard]] bool valid() const noexcept { return valid_; }

  [[nodiscard]] const std::vector<double>& components() const noexcept {
    return components_;
  }

  [[nodiscard]] PredicateSign sign() const {
    if (!valid_ || components_.empty()) {
      throw std::logic_error("an invalid expansion has no certified sign");
    }
    const double most_significant = components_.back();
    if (most_significant < 0.0) {
      return PredicateSign::negative;
    }
    return most_significant > 0.0 ? PredicateSign::positive
                                  : PredicateSign::zero;
  }

 private:
  friend FloatingExpansion grow_expansion(
      const FloatingExpansion&, double);

  FloatingExpansion(bool valid, std::vector<double> components) noexcept
      : valid_(valid), components_(std::move(components)) {}

  bool valid_{false};
  std::vector<double> components_{};
};

struct ErrorFreePair {
  double rounded{0.0};
  double error{0.0};
  bool valid{false};
};

struct Binary64IntegerScale {
  std::uint64_t significand{0U};
  int binary_exponent{0};
};

[[nodiscard]] inline bool expansion_arithmetic_exceptions_clear() noexcept {
  constexpr int rejected =
      FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW;
  return std::fetestexcept(rejected) == 0;
}

[[nodiscard]] inline Binary64IntegerScale binary64_integer_scale(
    double value) noexcept {
  constexpr std::uint64_t fraction_mask =
      (std::uint64_t{1} << 52U) - std::uint64_t{1};
  constexpr std::uint64_t exponent_mask = 0x7ffU;
  const std::uint64_t bits = std::bit_cast<std::uint64_t>(std::fabs(value));
  const std::uint64_t exponent_bits = (bits >> 52U) & exponent_mask;
  const std::uint64_t fraction_bits = bits & fraction_mask;
  if (exponent_bits == 0U) {
    return Binary64IntegerScale{fraction_bits, -1074};
  }
  return Binary64IntegerScale{
      (std::uint64_t{1} << 52U) | fraction_bits,
      static_cast<int>(exponent_bits) - 1023 - 52};
}

// The exact product must be expressible as a sum of binary64 values. If its
// least nonzero dyadic bit lies below 2^-1074, no floating expansion can retain
// it; reject before asking the FMA for an unrepresentable residual.
[[nodiscard]] inline bool product_has_representable_low_bit(
    double left, double right) noexcept {
  const Binary64IntegerScale left_scale = binary64_integer_scale(left);
  const Binary64IntegerScale right_scale = binary64_integer_scale(right);
  if (left_scale.significand == 0U || right_scale.significand == 0U) {
    return true;
  }
  const int lowest_bit_exponent =
      left_scale.binary_exponent + right_scale.binary_exponent +
      static_cast<int>(std::countr_zero(left_scale.significand)) +
      static_cast<int>(std::countr_zero(right_scale.significand));
  return lowest_bit_exponent >= -1074;
}

[[nodiscard]] inline ErrorFreePair two_sum(double left, double right) noexcept {
  const double rounded = rounded_add(left, right);
  if (!std::isfinite(rounded)) {
    return ErrorFreePair{};
  }
  const double right_virtual = rounded_subtract(rounded, left);
  const double left_virtual = rounded_subtract(rounded, right_virtual);
  const double right_roundoff = rounded_subtract(right, right_virtual);
  const double left_roundoff = rounded_subtract(left, left_virtual);
  const double error = rounded_add(left_roundoff, right_roundoff);
  if (!std::isfinite(right_virtual) || !std::isfinite(left_virtual) ||
      !std::isfinite(right_roundoff) || !std::isfinite(left_roundoff) ||
      !std::isfinite(error) || !expansion_arithmetic_exceptions_clear()) {
    return ErrorFreePair{};
  }
  return ErrorFreePair{rounded, error == 0.0 ? 0.0 : error, true};
}

[[nodiscard]] inline ErrorFreePair two_product(
    double left, double right) noexcept {
  if (!std::isfinite(left) || !std::isfinite(right) ||
      !product_has_representable_low_bit(left, right)) {
    return ErrorFreePair{};
  }
  const double rounded = rounded_multiply(left, right);
  if (!std::isfinite(rounded)) {
    return ErrorFreePair{};
  }
  // An explicit fused operation computes the residual of the rounded product.
  // If that residual cannot be represented because of underflow, both this
  // transform and the final public-stage check reject the expansion attempt.
  const double error = std::fma(left, right, -rounded);
  if (!std::isfinite(error) || !expansion_arithmetic_exceptions_clear()) {
    return ErrorFreePair{};
  }
  return ErrorFreePair{rounded, error == 0.0 ? 0.0 : error, true};
}

[[nodiscard]] inline FloatingExpansion grow_expansion(
    const FloatingExpansion& expansion, double value) {
  if (!expansion.valid() || !std::isfinite(value)) {
    return FloatingExpansion::invalid();
  }
  double accumulator = value == 0.0 ? 0.0 : value;
  std::vector<double> output;
  output.reserve(expansion.components().size() + 1U);
  for (const double component : expansion.components()) {
    const ErrorFreePair sum = two_sum(accumulator, component);
    if (!sum.valid) {
      return FloatingExpansion::invalid();
    }
    if (sum.error != 0.0) {
      output.push_back(sum.error);
    }
    accumulator = sum.rounded;
  }
  if (accumulator != 0.0 || output.empty()) {
    output.push_back(accumulator == 0.0 ? 0.0 : accumulator);
  }
  return FloatingExpansion{true, std::move(output)};
}

[[nodiscard]] inline FloatingExpansion add_expansions(
    const FloatingExpansion& left,
    const FloatingExpansion& right) {
  if (!left.valid() || !right.valid()) {
    return FloatingExpansion::invalid();
  }
  FloatingExpansion result = left;
  for (const double component : right.components()) {
    result = grow_expansion(result, component);
    if (!result.valid()) {
      return result;
    }
  }
  return result;
}

[[nodiscard]] inline FloatingExpansion negate_expansion(
    const FloatingExpansion& value) {
  if (!value.valid()) {
    return FloatingExpansion::invalid();
  }
  FloatingExpansion result = FloatingExpansion::scalar(0.0);
  for (const double component : value.components()) {
    result = grow_expansion(result, -component);
    if (!result.valid()) {
      return result;
    }
  }
  return result;
}

[[nodiscard]] inline FloatingExpansion subtract_expansions(
    const FloatingExpansion& left,
    const FloatingExpansion& right) {
  return add_expansions(left, negate_expansion(right));
}

[[nodiscard]] inline FloatingExpansion multiply_expansions(
    const FloatingExpansion& left,
    const FloatingExpansion& right) {
  if (!left.valid() || !right.valid()) {
    return FloatingExpansion::invalid();
  }
  FloatingExpansion result = FloatingExpansion::scalar(0.0);
  for (const double left_component : left.components()) {
    for (const double right_component : right.components()) {
      const ErrorFreePair product =
          two_product(left_component, right_component);
      if (!product.valid) {
        return FloatingExpansion::invalid();
      }
      if (product.error != 0.0) {
        result = grow_expansion(result, product.error);
        if (!result.valid()) {
          return result;
        }
      }
      result = grow_expansion(result, product.rounded);
      if (!result.valid()) {
        return result;
      }
    }
  }
  return result;
}

[[nodiscard]] inline FloatingExpansion difference_expansion(
    double left, double right) {
  return subtract_expansions(
      FloatingExpansion::scalar(left), FloatingExpansion::scalar(right));
}

[[nodiscard]] inline ExpansionResult finish_expansion_sign(
    const FloatingExpansion& expansion,
    Fp64EnvironmentGuard& environment) noexcept {
  const bool arithmetic_valid =
      expansion.valid() && expansion_arithmetic_exceptions_clear();
  const PredicateSign sign = arithmetic_valid ? expansion.sign()
                                                : PredicateSign::zero;
  const bool restored = environment.restore();
  if (!arithmetic_valid || !restored) {
    return ExpansionResult::uncertain();
  }
  return ExpansionResult::certified(sign);
}

}  // namespace detail
}  // namespace morsehgp3d::exact
