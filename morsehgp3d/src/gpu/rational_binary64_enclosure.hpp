#pragma once

#include "morsehgp3d/gpu/spatial_bounds.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <cstdint>
#include <stdexcept>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint64_t kPositiveMaximumFiniteBits =
    UINT64_C(0x7fefffffffffffff);
inline constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
inline constexpr std::uint64_t kSignBit = UINT64_C(0x8000000000000000);
inline constexpr std::uint64_t kExponentMask =
    UINT64_C(0x7ff0000000000000);
inline constexpr std::uint64_t kFractionMask =
    UINT64_C(0x000fffffffffffff);

struct DirectedEnclosure {
  std::uint64_t lower_bits{0U};
  std::uint64_t upper_bits{0U};
  DirectedEnclosureStatus status{DirectedEnclosureStatus::exact};
};

[[nodiscard]] inline exact::ExactRational positive_binary64_rational(
    std::uint64_t bits) {
  if (bits > kPositiveMaximumFiniteBits) {
    throw std::logic_error(
        "a directed-enclosure search produced a non-finite binary64 word");
  }
  return exact::ExactRational::from_binary64_bits(bits);
}

[[nodiscard]] inline DirectedEnclosure enclose_nonnegative_rational(
    const exact::ExactRational& value) {
  if (value.sign() < 0) {
    throw std::logic_error(
        "a nonnegative directed enclosure received a negative rational");
  }
  const exact::ExactRational maximum =
      positive_binary64_rational(kPositiveMaximumFiniteBits);
  if (value > maximum) {
    return DirectedEnclosure{
        kPositiveMaximumFiniteBits,
        kPositiveMaximumFiniteBits,
        DirectedEnclosureStatus::unsupported_range};
  }

  std::uint64_t lower_bits = 0U;
  std::uint64_t upper_search_bits = kPositiveMaximumFiniteBits;
  while (lower_bits < upper_search_bits) {
    const std::uint64_t midpoint_bits =
        lower_bits + (upper_search_bits - lower_bits + 1U) / 2U;
    if (positive_binary64_rational(midpoint_bits) <= value) {
      lower_bits = midpoint_bits;
    } else {
      upper_search_bits = midpoint_bits - 1U;
    }
  }
  if (positive_binary64_rational(lower_bits) == value) {
    return DirectedEnclosure{
        lower_bits, lower_bits, DirectedEnclosureStatus::exact};
  }
  if (lower_bits == kPositiveMaximumFiniteBits) {
    throw std::logic_error(
        "a finite directed enclosure has no representable upper endpoint");
  }
  return DirectedEnclosure{
      lower_bits, lower_bits + 1U, DirectedEnclosureStatus::enclosed};
}

[[nodiscard]] inline DirectedEnclosure enclose_rational(
    const exact::ExactRational& value) {
  if (value.sign() >= 0) {
    return enclose_nonnegative_rational(value);
  }
  const DirectedEnclosure magnitude = enclose_nonnegative_rational(-value);
  if (magnitude.status == DirectedEnclosureStatus::unsupported_range) {
    return DirectedEnclosure{
        kSignBit | kPositiveMaximumFiniteBits,
        kSignBit | kPositiveMaximumFiniteBits,
        DirectedEnclosureStatus::unsupported_range};
  }
  const std::uint64_t lower_bits =
      magnitude.upper_bits == 0U ? 0U : kSignBit | magnitude.upper_bits;
  const std::uint64_t upper_bits =
      magnitude.lower_bits == 0U ? 0U : kSignBit | magnitude.lower_bits;
  return DirectedEnclosure{lower_bits, upper_bits, magnitude.status};
}

}  // namespace morsehgp3d::gpu::detail
