#pragma once

#include "morsehgp3d/exact/predicate.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cfloat>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <xmmintrin.h>
#endif

namespace morsehgp3d::exact {
namespace detail {

[[nodiscard]] inline bool fp64_filter_environment_supported_unguarded() noexcept {
#if defined(__FAST_MATH__) || FLT_EVAL_METHOD != 0
  return false;
#else
  if (!std::numeric_limits<double>::is_iec559 ||
      std::numeric_limits<double>::radix != 2 ||
      std::numeric_limits<double>::digits != 53 ||
      std::numeric_limits<double>::has_denorm != std::denorm_present ||
      std::fegetround() != FE_TONEAREST) {
    return false;
  }
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  constexpr unsigned int flush_to_zero_mask = 1U << 15U;
  constexpr unsigned int denormals_are_zero_mask = 1U << 6U;
  constexpr unsigned int rounding_control_mask = 3U << 13U;
  if ((_mm_getcsr() & (flush_to_zero_mask | denormals_are_zero_mask |
                       rounding_control_mask)) != 0U) {
    return false;
  }
#endif
  const volatile double probe = std::numeric_limits<double>::denorm_min();
  const volatile double two = 2.0;
  const volatile double preserved = probe * two;
  return std::bit_cast<std::uint64_t>(static_cast<double>(preserved)) == 2U;
#endif
}

class Fp64EnvironmentGuard {
 public:
  Fp64EnvironmentGuard() noexcept
      : held_(std::feholdexcept(&saved_environment_) == 0) {}

  Fp64EnvironmentGuard(const Fp64EnvironmentGuard&) = delete;
  Fp64EnvironmentGuard& operator=(const Fp64EnvironmentGuard&) = delete;

  ~Fp64EnvironmentGuard() {
    if (held_) {
      static_cast<void>(std::fesetenv(&saved_environment_));
    }
  }

  [[nodiscard]] bool supported() const noexcept {
    return held_ && fp64_filter_environment_supported_unguarded();
  }

  [[nodiscard]] bool restore() noexcept {
    if (!held_) {
      return false;
    }
    if (std::fesetenv(&saved_environment_) != 0) {
      return false;
    }
    held_ = false;
    return true;
  }

 private:
  std::fenv_t saved_environment_{};
  bool held_{false};
};

struct Binary64Interval {
  double lower{0.0};
  double upper{0.0};
  bool valid{false};
};

[[nodiscard]] inline Binary64Interval invalid_binary64_interval() noexcept {
  return Binary64Interval{};
}

[[nodiscard]] inline Binary64Interval point_binary64_interval(double value) noexcept {
  if (!std::isfinite(value)) {
    return invalid_binary64_interval();
  }
  return Binary64Interval{value, value, true};
}

[[nodiscard]] inline double rounded_add(double left, double right) noexcept {
  const volatile double result = left + right;
  return result;
}

[[nodiscard]] inline double rounded_subtract(double left, double right) noexcept {
  const volatile double result = left - right;
  return result;
}

[[nodiscard]] inline double rounded_multiply(double left, double right) noexcept {
  const volatile double result = left * right;
  return result;
}

[[nodiscard]] inline Binary64Interval outward_interval(
    double lower_rounded, double upper_rounded) noexcept {
  if (!std::isfinite(lower_rounded) || !std::isfinite(upper_rounded)) {
    return invalid_binary64_interval();
  }
  const double lower = std::nextafter(
      lower_rounded, -std::numeric_limits<double>::infinity());
  const double upper = std::nextafter(
      upper_rounded, std::numeric_limits<double>::infinity());
  if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
    return invalid_binary64_interval();
  }
  return Binary64Interval{lower, upper, true};
}

[[nodiscard]] inline Binary64Interval add_binary64_intervals(
    const Binary64Interval& left, const Binary64Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_binary64_interval();
  }
  return outward_interval(
      rounded_add(left.lower, right.lower),
      rounded_add(left.upper, right.upper));
}

[[nodiscard]] inline Binary64Interval subtract_binary64_intervals(
    const Binary64Interval& left, const Binary64Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_binary64_interval();
  }
  return outward_interval(
      rounded_subtract(left.lower, right.upper),
      rounded_subtract(left.upper, right.lower));
}

[[nodiscard]] inline Binary64Interval multiply_binary64_intervals(
    const Binary64Interval& left, const Binary64Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_binary64_interval();
  }
  const std::array<double, 4> products{
      rounded_multiply(left.lower, right.lower),
      rounded_multiply(left.lower, right.upper),
      rounded_multiply(left.upper, right.lower),
      rounded_multiply(left.upper, right.upper)};
  if (!std::all_of(products.begin(), products.end(), [](double value) {
        return std::isfinite(value);
      })) {
    return invalid_binary64_interval();
  }
  const auto [minimum, maximum] =
      std::minmax_element(products.begin(), products.end());
  return outward_interval(*minimum, *maximum);
}

[[nodiscard]] inline Binary64Interval square_binary64_interval(
    const Binary64Interval& value) noexcept {
  if (!value.valid) {
    return invalid_binary64_interval();
  }
  const double maximum_magnitude =
      std::max(std::fabs(value.lower), std::fabs(value.upper));
  const double upper_rounded =
      rounded_multiply(maximum_magnitude, maximum_magnitude);
  if (!std::isfinite(upper_rounded)) {
    return invalid_binary64_interval();
  }
  double lower_rounded = 0.0;
  if (value.lower > 0.0 || value.upper < 0.0) {
    const double minimum_magnitude =
        std::min(std::fabs(value.lower), std::fabs(value.upper));
    lower_rounded = rounded_multiply(minimum_magnitude, minimum_magnitude);
    if (!std::isfinite(lower_rounded)) {
      return invalid_binary64_interval();
    }
  }
  Binary64Interval result = outward_interval(lower_rounded, upper_rounded);
  if (result.valid && result.lower < 0.0) {
    result.lower = 0.0;
  }
  return result;
}

[[nodiscard]] inline FilterResult sign_of_binary64_interval(
    const Binary64Interval& value) {
  if (!value.valid) {
    return FilterResult::uncertain();
  }
  if (value.lower > 0.0) {
    return FilterResult::certified(PredicateSign::positive);
  }
  if (value.upper < 0.0) {
    return FilterResult::certified(PredicateSign::negative);
  }
  return FilterResult::uncertain();
}

}  // namespace detail

// The interval filters require the strict floating-point flags exported by the
// morsehgp3d::exact CMake target. This runtime query preserves the caller's
// exception flags and also rejects altered rounding modes or FTZ/DAZ behavior.
[[nodiscard]] inline bool fp64_filter_environment_supported() noexcept {
  detail::Fp64EnvironmentGuard environment;
  const bool supported = environment.supported();
  return environment.restore() && supported;
}

}  // namespace morsehgp3d::exact
