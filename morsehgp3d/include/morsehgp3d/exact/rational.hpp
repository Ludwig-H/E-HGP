#pragma once

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"

#include <bit>
#include <compare>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace morsehgp3d::exact {

static_assert(
    sizeof(double) == sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559 &&
        std::numeric_limits<double>::radix == 2 &&
        std::numeric_limits<double>::digits == 53 &&
        std::numeric_limits<double>::min_exponent == -1021 &&
        std::numeric_limits<double>::max_exponent == 1024 &&
        std::numeric_limits<double>::has_denorm == std::denorm_present,
    "MorseHGP3D requires IEC 60559 binary64 doubles");

class ExactRational {
 public:
  ExactRational() = default;

  explicit ExactRational(BigInt numerator) : numerator_(std::move(numerator)) {}

  ExactRational(BigInt numerator, BigInt denominator)
      : numerator_(std::move(numerator)), denominator_(std::move(denominator)) {
    normalize();
  }

  [[nodiscard]] static ExactRational from_binary64(double value) {
    return from_binary64_bits(std::bit_cast<std::uint64_t>(value));
  }

  [[nodiscard]] static ExactRational from_binary64_bits(std::uint64_t bits) {
    constexpr std::uint64_t fraction_mask = (std::uint64_t{1} << 52U) - 1U;
    constexpr std::uint64_t exponent_mask = 0x7ffU;

    static_cast<void>(canonicalize_binary64_bits(bits));

    const bool negative = (bits >> 63U) != 0U;
    const std::uint64_t exponent_bits = (bits >> 52U) & exponent_mask;
    const std::uint64_t fraction_bits = bits & fraction_mask;

    if (exponent_bits == 0U && fraction_bits == 0U) {
      return ExactRational{};
    }

    BigInt significand = exponent_bits == 0U
                             ? BigInt{fraction_bits}
                             : BigInt{(std::uint64_t{1} << 52U) | fraction_bits};
    if (negative) {
      significand = -significand;
    }

    const int binary_exponent = exponent_bits == 0U
                                    ? -1074
                                    : static_cast<int>(exponent_bits) - 1023 - 52;
    if (binary_exponent >= 0) {
      return ExactRational{significand << static_cast<unsigned int>(binary_exponent)};
    }
    return ExactRational{
        std::move(significand), power_of_two(static_cast<unsigned int>(-binary_exponent))};
  }

  [[nodiscard]] static ExactRational parse_canonical(std::string_view text) {
    const std::size_t separator = text.find('/');
    if (separator == std::string_view::npos || text.find('/', separator + 1) != std::string_view::npos) {
      throw std::invalid_argument("an exact rational key must contain one '/' separator");
    }
    const std::string_view numerator_text = text.substr(0, separator);
    const std::string_view denominator_text = text.substr(separator + 1);
    ExactRational value{
        parse_canonical_integer(numerator_text),
        parse_canonical_positive_integer(denominator_text)};
    if (value.canonical_key() != text) {
      throw std::invalid_argument("the exact rational key is not reduced canonically");
    }
    return value;
  }

  [[nodiscard]] const BigInt& numerator() const noexcept { return numerator_; }
  [[nodiscard]] const BigInt& denominator() const noexcept { return denominator_; }
  [[nodiscard]] bool is_zero() const noexcept { return numerator_ == 0; }

  [[nodiscard]] int sign() const noexcept {
    if (numerator_ < 0) {
      return -1;
    }
    return numerator_ == 0 ? 0 : 1;
  }

  [[nodiscard]] std::string canonical_key() const {
    return canonical_integer_string(numerator_) + "/" + canonical_integer_string(denominator_);
  }

  friend bool operator==(const ExactRational& left, const ExactRational& right) noexcept {
    return left.numerator_ == right.numerator_ && left.denominator_ == right.denominator_;
  }

  friend std::strong_ordering operator<=>(const ExactRational& left, const ExactRational& right) {
    const BigInt difference =
        left.numerator_ * right.denominator_ - right.numerator_ * left.denominator_;
    if (difference < 0) {
      return std::strong_ordering::less;
    }
    if (difference > 0) {
      return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
  }

  friend ExactRational operator-(const ExactRational& value) {
    return ExactRational{-value.numerator_, value.denominator_};
  }

  friend ExactRational operator+(const ExactRational& left, const ExactRational& right) {
    return ExactRational{
        left.numerator_ * right.denominator_ + right.numerator_ * left.denominator_,
        left.denominator_ * right.denominator_};
  }

  friend ExactRational operator-(const ExactRational& left, const ExactRational& right) {
    return left + (-right);
  }

  friend ExactRational operator*(const ExactRational& left, const ExactRational& right) {
    return ExactRational{
        left.numerator_ * right.numerator_, left.denominator_ * right.denominator_};
  }

  friend ExactRational operator/(const ExactRational& left, const ExactRational& right) {
    if (right.numerator_ == 0) {
      throw std::domain_error("division by an exact zero is undefined");
    }
    return ExactRational{
        left.numerator_ * right.denominator_, left.denominator_ * right.numerator_};
  }

 private:
  void normalize() {
    if (denominator_ == 0) {
      throw std::domain_error("an exact rational denominator cannot be zero");
    }
    if (denominator_ < 0) {
      numerator_ = -numerator_;
      denominator_ = -denominator_;
    }
    if (numerator_ == 0) {
      denominator_ = 1;
      return;
    }
    const BigInt divisor = greatest_common_divisor(numerator_, denominator_);
    numerator_ /= divisor;
    denominator_ /= divisor;
  }

  BigInt numerator_{0};
  BigInt denominator_{1};
};

}  // namespace morsehgp3d::exact
