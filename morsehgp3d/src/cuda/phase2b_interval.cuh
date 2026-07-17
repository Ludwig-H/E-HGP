#pragma once

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

#if !defined(__CUDACC__)
#error "phase2b_interval.cuh is a CUDA device-only contract"
#endif

namespace morsehgp3d::gpu::detail::device {

inline constexpr std::uint64_t kBinary64ExponentMask =
    UINT64_C(0x7ff0000000000000);

struct DeviceInterval {
  double lower{0.0};
  double upper{0.0};
  bool valid{false};
};

[[nodiscard]] inline __device__ bool is_finite(double value) noexcept {
  return isfinite(value) != 0;
}

[[nodiscard]] inline __device__ DeviceInterval invalid_interval() noexcept {
  return DeviceInterval{};
}

[[nodiscard]] inline __device__ DeviceInterval point_interval(
    std::uint64_t bits) noexcept {
  if ((bits & kBinary64ExponentMask) == kBinary64ExponentMask) {
    return invalid_interval();
  }
  const double value =
      __longlong_as_double(static_cast<long long int>(bits));
  return DeviceInterval{value, value, true};
}

[[nodiscard]] inline __device__ DeviceInterval checked_interval(
    double lower, double upper) noexcept {
  if (!is_finite(lower) || !is_finite(upper) || lower > upper) {
    return invalid_interval();
  }
  return DeviceInterval{lower, upper, true};
}

[[nodiscard]] inline __device__ DeviceInterval add_intervals(
    const DeviceInterval& left, const DeviceInterval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return checked_interval(
      __dadd_rd(left.lower, right.lower),
      __dadd_ru(left.upper, right.upper));
}

[[nodiscard]] inline __device__ DeviceInterval subtract_intervals(
    const DeviceInterval& left, const DeviceInterval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return checked_interval(
      __dsub_rd(left.lower, right.upper),
      __dsub_ru(left.upper, right.lower));
}

// Multiplication is monotone within a known sign quadrant, where two directed
// products suffice. Only an interval crossing zero needs all endpoint pairs.
[[nodiscard]] inline __device__ DeviceInterval multiply_intervals(
    const DeviceInterval& left, const DeviceInterval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  if (left.lower >= 0.0 && right.lower >= 0.0) {
    return checked_interval(
        __dmul_rd(left.lower, right.lower),
        __dmul_ru(left.upper, right.upper));
  }
  if (left.upper <= 0.0 && right.lower >= 0.0) {
    return checked_interval(
        __dmul_rd(left.lower, right.upper),
        __dmul_ru(left.upper, right.lower));
  }
  if (left.lower >= 0.0 && right.upper <= 0.0) {
    return checked_interval(
        __dmul_rd(left.upper, right.lower),
        __dmul_ru(left.lower, right.upper));
  }
  if (left.upper <= 0.0 && right.upper <= 0.0) {
    return checked_interval(
        __dmul_rd(left.upper, right.upper),
        __dmul_ru(left.lower, right.lower));
  }

  // At least one interval crosses zero. The monotone sign quadrants no
  // longer determine both extrema, so evaluate all endpoint pairs.
  const double lower_products[4]{
      __dmul_rd(left.lower, right.lower),
      __dmul_rd(left.lower, right.upper),
      __dmul_rd(left.upper, right.lower),
      __dmul_rd(left.upper, right.upper)};
  const double upper_products[4]{
      __dmul_ru(left.lower, right.lower),
      __dmul_ru(left.lower, right.upper),
      __dmul_ru(left.upper, right.lower),
      __dmul_ru(left.upper, right.upper)};
  double lower = lower_products[0];
  double upper = upper_products[0];
  for (std::size_t index = 0U; index < 4U; ++index) {
    if (!is_finite(lower_products[index]) ||
        !is_finite(upper_products[index])) {
      return invalid_interval();
    }
    lower = lower_products[index] < lower ? lower_products[index] : lower;
    upper = upper_products[index] > upper ? upper_products[index] : upper;
  }
  return checked_interval(lower, upper);
}

[[nodiscard]] inline __device__ DeviceInterval square_interval(
    const DeviceInterval& value) noexcept {
  if (!value.valid) {
    return invalid_interval();
  }

  double lower = 0.0;
  double upper = 0.0;
  if (value.lower > 0.0) {
    lower = __dmul_rd(value.lower, value.lower);
    upper = __dmul_ru(value.upper, value.upper);
  } else if (value.upper < 0.0) {
    lower = __dmul_rd(value.upper, value.upper);
    upper = __dmul_ru(value.lower, value.lower);
  } else {
    const double lower_square = __dmul_ru(value.lower, value.lower);
    const double upper_square = __dmul_ru(value.upper, value.upper);
    lower = 0.0;
    upper = lower_square > upper_square ? lower_square : upper_square;
  }
  return checked_interval(lower, upper);
}

}  // namespace morsehgp3d::gpu::detail::device
