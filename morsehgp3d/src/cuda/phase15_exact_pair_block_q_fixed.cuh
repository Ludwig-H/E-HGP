#pragma once

#include "phase15_exact_diametral_phi_fixed.cuh"

#include <cstddef>
#include <cstdint>

#if defined(__CUDACC__)
#define MORSEHGP3D_EXACT_BLOCK_Q_HD __host__ __device__
#else
#define MORSEHGP3D_EXACT_BLOCK_Q_HD
#endif

namespace morsehgp3d::gpu::detail::exact_pair_block_q_fixed {

namespace fixed = exact_phi_fixed;

struct Binary64Aabb3 {
  std::uint64_t lower[3]{};
  std::uint64_t upper[3]{};
};

[[nodiscard]] MORSEHGP3D_EXACT_BLOCK_Q_HD inline std::uint64_t
binary64_order_key(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_mask = UINT64_C(1) << 63U;
  const std::uint64_t canonical_bits = bits == sign_mask ? 0U : bits;
  return (canonical_bits & sign_mask) != 0U
      ? ~canonical_bits
      : canonical_bits ^ sign_mask;
}

[[nodiscard]] MORSEHGP3D_EXACT_BLOCK_Q_HD inline bool
binary64_interval_ordered(
    std::uint64_t lower,
    std::uint64_t upper) noexcept {
  return fixed::is_finite_binary64_word(lower) &&
      fixed::is_finite_binary64_word(upper) &&
      binary64_order_key(lower) <= binary64_order_key(upper);
}

[[nodiscard]] MORSEHGP3D_EXACT_BLOCK_Q_HD inline bool compare_magnitude(
    const fixed::ExactProduct& left,
    const fixed::ExactProduct& right,
    int& ordering) noexcept {
  const bool left_zero = fixed::is_zero(left.value.magnitude);
  const bool right_zero = fixed::is_zero(right.value.magnitude);
  if (left_zero || right_zero) {
    ordering = left_zero == right_zero ? 0 : (left_zero ? -1 : 1);
    return true;
  }
  const int left_top = left.exponent +
      static_cast<int>(fixed::bit_width(left.value.magnitude)) - 1;
  const int right_top = right.exponent +
      static_cast<int>(fixed::bit_width(right.value.magnitude)) - 1;
  if (left_top != right_top) {
    ordering = left_top < right_top ? -1 : 1;
    return true;
  }
  const int common_exponent =
      left.exponent < right.exponent ? left.exponent : right.exponent;
  const int left_shift = left.exponent - common_exponent;
  const int right_shift = right.exponent - common_exponent;
  if (left_shift < 0 || right_shift < 0 || left_shift > 256 ||
      right_shift > 256) {
    return false;
  }
  fixed::UInt256 aligned_left{};
  fixed::UInt256 aligned_right{};
  if (!fixed::shift_left(
          left.value.magnitude,
          static_cast<unsigned int>(left_shift),
          aligned_left) ||
      !fixed::shift_left(
          right.value.magnitude,
          static_cast<unsigned int>(right_shift),
          aligned_right)) {
    return false;
  }
  ordering = fixed::compare(aligned_left, aligned_right);
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_BLOCK_Q_HD inline bool compare_product(
    const fixed::ExactProduct& left,
    const fixed::ExactProduct& right,
    int& ordering) noexcept {
  const bool left_zero = fixed::is_zero(left.value.magnitude);
  const bool right_zero = fixed::is_zero(right.value.magnitude);
  const bool left_negative = !left_zero && left.value.negative;
  const bool right_negative = !right_zero && right.value.negative;
  if (left_negative != right_negative) {
    ordering = left_negative ? -1 : 1;
    return true;
  }
  if (!compare_magnitude(left, right, ordering)) {
    return false;
  }
  if (left_negative) {
    ordering = -ordering;
  }
  return true;
}

[[nodiscard]] MORSEHGP3D_EXACT_BLOCK_Q_HD inline fixed::ResultCode
exact_maximum_sign(
    const Binary64Aabb3& first,
    const Binary64Aabb3& second,
    const Binary64Aabb3& witness) noexcept {
  fixed::ExactProduct axis_maximum[3]{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t first_endpoints[2]{
        first.lower[axis], first.upper[axis]};
    const std::uint64_t second_endpoints[2]{
        second.lower[axis], second.upper[axis]};
    const std::uint64_t witness_endpoints[2]{
        witness.lower[axis], witness.upper[axis]};
    if (!binary64_interval_ordered(
            first_endpoints[0], first_endpoints[1]) ||
        !binary64_interval_ordered(
            second_endpoints[0], second_endpoints[1]) ||
        !binary64_interval_ordered(
            witness_endpoints[0], witness_endpoints[1])) {
      return fixed::ResultCode::invalid_input;
    }
    bool initialized = false;
    for (std::size_t query_selector = 0U;
         query_selector < 2U;
         ++query_selector) {
      for (std::size_t first_selector = 0U;
           first_selector < 2U;
           ++first_selector) {
        for (std::size_t second_selector = 0U;
             second_selector < 2U;
             ++second_selector) {
          fixed::ExactDifference first_difference{};
          fixed::ExactDifference second_difference{};
          fixed::ExactProduct candidate{};
          if (!fixed::exact_difference(
                  witness_endpoints[query_selector],
                  first_endpoints[first_selector],
                  first_difference) ||
              !fixed::exact_difference(
                  witness_endpoints[query_selector],
                  second_endpoints[second_selector],
                  second_difference) ||
              !fixed::exact_product(
                  first_difference, second_difference, candidate)) {
            return fixed::ResultCode::requires_cpu_fallback;
          }
          int ordering = 0;
          if (!initialized) {
            axis_maximum[axis] = candidate;
            initialized = true;
          } else if (!compare_product(
                         candidate, axis_maximum[axis], ordering)) {
            return fixed::ResultCode::requires_cpu_fallback;
          } else if (ordering > 0) {
            axis_maximum[axis] = candidate;
          }
        }
      }
    }
  }

  bool exponent_initialized = false;
  int common_exponent = 0;
  for (const fixed::ExactProduct& term : axis_maximum) {
    if (!fixed::is_zero(term.value.magnitude) &&
        (!exponent_initialized || term.exponent < common_exponent)) {
      common_exponent = term.exponent;
      exponent_initialized = true;
    }
  }
  if (!exponent_initialized) {
    return fixed::ResultCode::zero;
  }

  fixed::Signed256 sum{};
  for (const fixed::ExactProduct& term : axis_maximum) {
    if (fixed::is_zero(term.value.magnitude)) {
      continue;
    }
    const int shift = term.exponent - common_exponent;
    if (shift < 0 || shift > 256) {
      return fixed::ResultCode::requires_cpu_fallback;
    }
    fixed::Signed256 aligned{};
    aligned.negative = term.value.negative;
    if (!fixed::shift_left(
            term.value.magnitude,
            static_cast<unsigned int>(shift),
            aligned.magnitude)) {
      return fixed::ResultCode::requires_cpu_fallback;
    }
    fixed::Signed256 next{};
    if (!fixed::add_signed(sum, aligned, next)) {
      return fixed::ResultCode::requires_cpu_fallback;
    }
    sum = next;
  }
  if (fixed::is_zero(sum.magnitude)) {
    return fixed::ResultCode::zero;
  }
  return sum.negative ? fixed::ResultCode::negative
                      : fixed::ResultCode::positive;
}

}  // namespace morsehgp3d::gpu::detail::exact_pair_block_q_fixed

#undef MORSEHGP3D_EXACT_BLOCK_Q_HD
