#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace morsehgp3d::exact {

using BigInt = boost::multiprecision::cpp_int;

inline BigInt magnitude(BigInt value) {
  return value < 0 ? -value : value;
}

// Same mathematical function as the textbook Euclid loop, taken from the
// backend's binary/Lehmer implementation.  This is not a micro-detail: every
// normalized rational operation calls it, and on the ~220-bit operands a
// circumcenter actually produces, the naive loop ran about 130 bignum
// divisions -- two orders of magnitude more than the multiplications it was
// reducing.
inline BigInt greatest_common_divisor(BigInt left, BigInt right) {
  return boost::multiprecision::gcd(
      magnitude(std::move(left)), magnitude(std::move(right)));
}

// True when value is a strictly positive power of two.  One bit scan, no
// division.
inline bool is_power_of_two(const BigInt& value) {
  return value > 0 &&
         boost::multiprecision::lsb(value) ==
             boost::multiprecision::msb(value);
}

// value * factor, taking the shift when factor is a power of two.  Exactly
// the same integer in both branches.  This is the multiplication that every
// normalized rational operation performs against a denominator, and every
// quantity built from binary64 coordinates by addition, subtraction and
// multiplication carries a power-of-two denominator -- so on the support
// product the general branch is never taken.
inline BigInt scaled_by(const BigInt& value, const BigInt& factor) {
  if (is_power_of_two(factor)) {
    return value << boost::multiprecision::lsb(factor);
  }
  return value * factor;
}

inline BigInt power_of_two(unsigned int exponent) {
  BigInt result = 1;
  return result << exponent;
}

inline std::string canonical_integer_string(const BigInt& value) {
  return value.str();
}

inline BigInt parse_canonical_integer(std::string_view text) {
  if (text.empty()) {
    throw std::invalid_argument("an exact integer cannot be empty");
  }

  bool negative = false;
  std::size_t index = 0;
  if (text.front() == '-') {
    negative = true;
    index = 1;
  } else if (text.front() == '+') {
    throw std::invalid_argument("a canonical exact integer cannot start with '+'");
  }

  if (index == text.size()) {
    throw std::invalid_argument("an exact integer must contain digits");
  }
  if (text[index] == '0' && text.size() - index != 1) {
    throw std::invalid_argument("a canonical exact integer cannot have leading zeroes");
  }

  BigInt value = 0;
  for (; index < text.size(); ++index) {
    const char character = text[index];
    if (character < '0' || character > '9') {
      throw std::invalid_argument("an exact integer must contain decimal digits only");
    }
    value *= 10;
    value += static_cast<unsigned int>(character - '0');
  }

  if (negative) {
    if (value == 0) {
      throw std::invalid_argument("negative zero is not canonical");
    }
    value = -value;
  }
  return value;
}

inline BigInt parse_canonical_nonnegative_integer(std::string_view text) {
  BigInt value = parse_canonical_integer(text);
  if (value < 0) {
    throw std::invalid_argument("the exact integer must be nonnegative");
  }
  return value;
}

inline BigInt parse_canonical_positive_integer(std::string_view text) {
  BigInt value = parse_canonical_integer(text);
  if (value <= 0) {
    throw std::invalid_argument("the exact integer must be strictly positive");
  }
  return value;
}

}  // namespace morsehgp3d::exact
