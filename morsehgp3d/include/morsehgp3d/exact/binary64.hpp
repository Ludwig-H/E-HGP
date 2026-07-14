#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace morsehgp3d::exact {

inline constexpr std::uint64_t binary64_sign_mask = std::uint64_t{1} << 63U;
inline constexpr std::uint64_t binary64_exponent_mask = std::uint64_t{0x7ff} << 52U;
inline constexpr std::uint64_t binary64_negative_zero = binary64_sign_mask;

[[nodiscard]] inline bool is_finite_binary64_bits(std::uint64_t bits) noexcept {
  return (bits & binary64_exponent_mask) != binary64_exponent_mask;
}

[[nodiscard]] inline std::uint64_t canonicalize_binary64_bits(std::uint64_t bits) {
  if (!is_finite_binary64_bits(bits)) {
    throw std::domain_error("binary64 coordinates must be finite");
  }
  return bits == binary64_negative_zero ? 0U : bits;
}

[[nodiscard]] inline std::uint64_t binary64_total_order_key(std::uint64_t bits) {
  const std::uint64_t canonical_bits = canonicalize_binary64_bits(bits);
  if ((canonical_bits & binary64_sign_mask) != 0U) {
    return ~canonical_bits;
  }
  return canonical_bits ^ binary64_sign_mask;
}

[[nodiscard]] inline std::string binary64_hex(std::uint64_t bits) {
  constexpr char digits[] = "0123456789abcdef";
  std::string output(16U, '0');
  for (std::size_t index = output.size(); index != 0U; --index) {
    output[index - 1U] = digits[bits & 0x0fU];
    bits >>= 4U;
  }
  return output;
}

[[nodiscard]] inline std::uint64_t parse_binary64_hex(
    std::string_view text, bool require_canonical = true) {
  if (text.size() != 16U) {
    throw std::invalid_argument("a binary64 word must contain 16 hexadecimal digits");
  }
  std::uint64_t bits = 0;
  for (const char character : text) {
    unsigned int digit = 0;
    if (character >= '0' && character <= '9') {
      digit = static_cast<unsigned int>(character - '0');
    } else if (character >= 'a' && character <= 'f') {
      digit = static_cast<unsigned int>(character - 'a') + 10U;
    } else {
      throw std::invalid_argument("a binary64 word must use lowercase hexadecimal digits");
    }
    bits = (bits << 4U) | digit;
  }
  const std::uint64_t canonical_bits = canonicalize_binary64_bits(bits);
  if (require_canonical && canonical_bits != bits) {
    throw std::invalid_argument("negative zero is not a canonical embedded coordinate");
  }
  return bits;
}

}  // namespace morsehgp3d::exact
