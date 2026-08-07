#pragma once

// Binary64 interval filter in front of the integer width cascade of the
// higher-support product gates.
//
// Why it exists.  The device engine evaluates every product gate directly in
// int256 interval arithmetic, and the measurements of 7 August 2026 show that
// every decision already falls at that narrowest width: `deferred_int512` and
// `deferred_int1024` are zero across the six n=32 cases, and the exhaustive
// census measures 34.5 to 96.9 microseconds per support on one core.  That cost
// is therefore pure int256 work, not the price of exactness.  The pair stage,
// on the same cloud, filters 97.8% of its exact predicates in fp64 first.  This
// header gives the higher stage the same rung.
//
// Why it is safe.  Each elementary operation is evaluated in binary64 and then
// widened outward by one unit in the last place, so the computed interval is a
// guaranteed superset of the exact interval that the integer cascade would
// produce on the same expression -- the standard syntactic induction of
// `docs/math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md` section 3, where losing
// dependencies can only widen an interval and therefore can only prevent a
// decision, never fabricate one.  Concretely: a superset that is strictly
// positive proves the exact value is strictly positive, and a superset that is
// nonpositive proves the exact value is nonpositive.  Every other case defers
// to the integer cascade, unchanged.  The filter can therefore only move work,
// never a verdict.
//
// It deliberately mirrors only the GENERIC certificates -- Gram determinant and
// barycentric numerators.  The tighter triangle specialisation of
// `no_well_centered_support` is not reproduced here; for a three-point support
// the filter refuses to answer `refuted` and defers, so the integer path keeps
// its opportunity to certify through that specialisation.
//
// This cannot include `morsehgp3d/exact/fp64_interval.hpp`, whose certified
// environment guard is host-only x86 MXCSR code.  The rounding discipline below
// is the same one, restated in `__host__ __device__` form: widen every result
// outward by one ulp, and treat any non-finite intermediate as a deferral.

#include "phase15_exact_higher_support_product_fixed.cuh"

#include <cstddef>
#include <cstdint>
#include <cmath>

#if defined(__CUDACC__)
#define MORSEHGP3D_PHASE15_HS_FP64_HD __host__ __device__
#else
#define MORSEHGP3D_PHASE15_HS_FP64_HD
#endif

namespace morsehgp3d::gpu::detail::phase15_higher_support_fp64_filter {

inline constexpr std::size_t axis_count = 3U;
inline constexpr std::size_t maximum_support_size = 4U;

using Binary64Aabb3 =
    exact_higher_support_product_fixed::Binary64Aabb3;

// A closed binary64 interval, or the invalid marker that forces a deferral.
struct Interval {
  double lower{0.0};
  double upper{0.0};
  bool valid{false};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
invalid_interval() noexcept {
  return Interval{};
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline bool
is_finite_word(std::uint64_t bits) noexcept {
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7FF0000000000000);
  return (bits & exponent_mask) != exponent_mask;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline double
word_to_double(std::uint64_t bits) noexcept {
  double value = 0.0;
  __builtin_memcpy(&value, &bits, sizeof(value));
  return value;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline bool
finite(double value) noexcept {
  return value - value == 0.0;
}

// One outward widening: the exact result of the operation lies within one ulp
// of the rounded result, so nudging both ends away from it encloses it.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
widen(double lower_rounded, double upper_rounded) noexcept {
  if (!finite(lower_rounded) || !finite(upper_rounded)) {
    return invalid_interval();
  }
  const double lower = ::nextafter(lower_rounded, -1.0e308 * 10.0);
  const double upper = ::nextafter(upper_rounded, 1.0e308 * 10.0);
  if (!finite(lower) || !finite(upper) || lower > upper) {
    return invalid_interval();
  }
  return Interval{lower, upper, true};
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
add(const Interval& left, const Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return widen(left.lower + right.lower, left.upper + right.upper);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
subtract(const Interval& left, const Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return widen(left.lower - right.upper, left.upper - right.lower);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
multiply(const Interval& left, const Interval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  const double a = left.lower * right.lower;
  const double b = left.lower * right.upper;
  const double c = left.upper * right.lower;
  const double d = left.upper * right.upper;
  double minimum = a;
  double maximum = a;
  minimum = b < minimum ? b : minimum;
  maximum = b > maximum ? b : maximum;
  minimum = c < minimum ? c : minimum;
  maximum = c > maximum ? c : maximum;
  minimum = d < minimum ? d : minimum;
  maximum = d > maximum ? d : maximum;
  return widen(minimum, maximum);
}

// The square of one variable, not the product of two independent ones: an
// interval straddling zero has a nonnegative square, exactly as
// `square_interval` decides it in the integer widths.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
square(const Interval& value) noexcept {
  if (!value.valid) {
    return invalid_interval();
  }
  const double low = value.lower * value.lower;
  const double high = value.upper * value.upper;
  const double maximum = low > high ? low : high;
  const double minimum =
      (value.lower <= 0.0 && value.upper >= 0.0)
      ? 0.0
      : (low < high ? low : high);
  return widen(minimum, maximum);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
scale_by_two(const Interval& value) noexcept {
  if (!value.valid) {
    return invalid_interval();
  }
  return widen(value.lower + value.lower, value.upper + value.upper);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
box_axis(const Binary64Aabb3& box, std::size_t axis) noexcept {
  const std::uint64_t low_bits = box.lower[axis];
  const std::uint64_t high_bits = box.upper[axis];
  if (!is_finite_word(low_bits) || !is_finite_word(high_bits)) {
    return invalid_interval();
  }
  const double low = word_to_double(low_bits);
  const double high = word_to_double(high_bits);
  if (!finite(low) || !finite(high) || low > high) {
    return invalid_interval();
  }
  return Interval{low, high, true};
}

// The evaluation mirrors `evaluate_support` term for term: directions, Gram
// matrix, its determinant, and the Cramer numerators obtained by replacing one
// column with the squared direction norms.
struct SupportEvaluation {
  std::size_t dimension{};
  Interval gram[3][3]{};
  Interval squared_norms[3]{};
  Interval determinant{};
  Interval cramer[3]{};
  bool valid{false};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Interval
determinant_of(const Interval matrix[3][3], std::size_t dimension) noexcept {
  if (dimension == 2U) {
    return subtract(
        multiply(matrix[0][0], matrix[1][1]),
        multiply(matrix[0][1], matrix[1][0]));
  }
  const Interval first = multiply(
      matrix[0][0],
      subtract(
          multiply(matrix[1][1], matrix[2][2]),
          multiply(matrix[1][2], matrix[2][1])));
  const Interval second = multiply(
      matrix[0][1],
      subtract(
          multiply(matrix[1][0], matrix[2][2]),
          multiply(matrix[1][2], matrix[2][0])));
  const Interval third = multiply(
      matrix[0][2],
      subtract(
          multiply(matrix[1][0], matrix[2][1]),
          multiply(matrix[1][1], matrix[2][0])));
  return add(subtract(first, second), third);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline SupportEvaluation
evaluate(
    const Binary64Aabb3 boxes[maximum_support_size],
    std::size_t support_size) noexcept {
  SupportEvaluation output;
  if (support_size != 3U && support_size != 4U) {
    return output;
  }
  output.dimension = support_size - 1U;
  Interval directions[3][axis_count]{};
  for (std::size_t direction = 0U; direction < output.dimension; ++direction) {
    for (std::size_t axis = 0U; axis < axis_count; ++axis) {
      directions[direction][axis] = subtract(
          box_axis(boxes[direction + 1U], axis), box_axis(boxes[0], axis));
      if (!directions[direction][axis].valid) {
        return output;
      }
    }
  }
  for (std::size_t row = 0U; row < output.dimension; ++row) {
    for (std::size_t column = 0U; column < output.dimension; ++column) {
      Interval sum = Interval{0.0, 0.0, true};
      for (std::size_t axis = 0U; axis < axis_count; ++axis) {
        const Interval term = row == column
            ? square(directions[row][axis])
            : multiply(directions[row][axis], directions[column][axis]);
        sum = add(sum, term);
      }
      if (!sum.valid) {
        return output;
      }
      output.gram[row][column] = sum;
    }
    output.squared_norms[row] = output.gram[row][row];
  }
  output.determinant = determinant_of(output.gram, output.dimension);
  if (!output.determinant.valid) {
    return output;
  }
  for (std::size_t column = 0U; column < output.dimension; ++column) {
    Interval replaced[3][3]{};
    for (std::size_t row = 0U; row < output.dimension; ++row) {
      for (std::size_t source = 0U; source < output.dimension; ++source) {
        replaced[row][source] = source == column
            ? output.squared_norms[row]
            : output.gram[row][source];
      }
    }
    output.cramer[column] = determinant_of(replaced, output.dimension);
    if (!output.cramer[column].valid) {
      return output;
    }
  }
  output.valid = true;
  return output;
}

// Barycentric numerators, exactly as `barycentric_numerators` derives them:
// the Cramer numerators shifted by one, and twice the determinant minus their
// sum in the leading slot.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline bool
barycentric_numerators(
    const SupportEvaluation& support,
    Interval output[maximum_support_size]) noexcept {
  if (!support.valid) {
    return false;
  }
  Interval sum = Interval{0.0, 0.0, true};
  for (std::size_t index = 0U; index < support.dimension; ++index) {
    output[index + 1U] = support.cramer[index];
    sum = add(sum, support.cramer[index]);
  }
  if (!sum.valid) {
    return false;
  }
  output[0] = subtract(scale_by_two(support.determinant), sum);
  return output[0].valid;
}

// A filter verdict.  `defer` is the only answer the filter is allowed to give
// when it cannot prove a strict sign, and it costs nothing but the evaluation.
enum class Verdict : std::uint8_t {
  defer = 0U,
  certified = 1U,
  refuted = 2U,
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline bool
certified_positive(const Interval& value) noexcept {
  return value.valid && value.lower > 0.0;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline bool
certified_nonpositive(const Interval& value) noexcept {
  return value.valid && value.upper <= 0.0;
}

// `certified` means: no tuple of the product is minimal well centred, which the
// integer path proves by a nonpositive upper bound on the Gram determinant or
// on one barycentric numerator.  A superset that is nonpositive proves the
// exact one is.
//
// `refuted` is only returned for a four-point support.  For three points the
// integer path owns a tighter triangle specialisation that this filter does not
// reproduce, so proving the generic conditions fail proves nothing about the
// final verdict, and the filter defers.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Verdict
no_well_centered(
    const Binary64Aabb3 boxes[maximum_support_size],
    std::size_t support_size) noexcept {
  const SupportEvaluation support = evaluate(boxes, support_size);
  if (!support.valid) {
    return Verdict::defer;
  }
  if (certified_nonpositive(support.determinant)) {
    return Verdict::certified;
  }
  Interval barycentric[maximum_support_size]{};
  if (!barycentric_numerators(support, barycentric)) {
    return Verdict::defer;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (certified_nonpositive(barycentric[index])) {
      return Verdict::certified;
    }
  }
  if (support_size == 3U) {
    return Verdict::defer;
  }
  if (!certified_positive(support.determinant)) {
    return Verdict::defer;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (!certified_positive(barycentric[index])) {
      return Verdict::defer;
    }
  }
  return Verdict::refuted;
}

// `certified` means: every tuple of the product is minimal well centred, which
// the integer path proves by strictly positive lower bounds on the Gram
// determinant and on every barycentric numerator.  `refuted` is proved by a
// nonpositive upper bound on any of them, since a nonpositive upper bound
// forces a nonpositive lower bound.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_FP64_HD inline Verdict
all_well_centered(
    const Binary64Aabb3 boxes[maximum_support_size],
    std::size_t support_size) noexcept {
  const SupportEvaluation support = evaluate(boxes, support_size);
  if (!support.valid) {
    return Verdict::defer;
  }
  Interval barycentric[maximum_support_size]{};
  if (!barycentric_numerators(support, barycentric)) {
    return Verdict::defer;
  }
  if (certified_nonpositive(support.determinant)) {
    return Verdict::refuted;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (certified_nonpositive(barycentric[index])) {
      return Verdict::refuted;
    }
  }
  if (!certified_positive(support.determinant)) {
    return Verdict::defer;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (!certified_positive(barycentric[index])) {
      return Verdict::defer;
    }
  }
  return Verdict::certified;
}

}  // namespace morsehgp3d::gpu::detail::phase15_higher_support_fp64_filter
