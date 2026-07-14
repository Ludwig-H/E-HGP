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

inline BigInt greatest_common_divisor(BigInt left, BigInt right) {
  left = magnitude(std::move(left));
  right = magnitude(std::move(right));
  while (right != 0) {
    BigInt remainder = left % right;
    left = std::move(right);
    right = std::move(remainder);
  }
  return left;
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
