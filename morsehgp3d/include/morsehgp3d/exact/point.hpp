#pragma once

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <array>
#include <bit>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace morsehgp3d::exact {

struct ExactRational3Record {
  std::string schema_version;
  std::string x_numerator;
  std::string y_numerator;
  std::string z_numerator;
  std::string denominator;
  std::string unit;

  friend bool operator==(const ExactRational3Record&, const ExactRational3Record&) = default;
};

class ExactRational3 {
 public:
  static constexpr const char* schema_version = "2.0.0";
  static constexpr const char* unit = "input_coordinate_unit";

  ExactRational3() = default;

  ExactRational3(BigInt x_numerator, BigInt y_numerator, BigInt z_numerator, BigInt denominator)
      : numerators_{
            std::move(x_numerator), std::move(y_numerator), std::move(z_numerator)},
        denominator_(std::move(denominator)) {
    normalize();
  }

  explicit ExactRational3(const std::array<ExactRational, 3>& coordinates) {
    BigInt common_denominator = 1;
    for (const ExactRational& coordinate : coordinates) {
      const BigInt divisor =
          greatest_common_divisor(common_denominator, coordinate.denominator());
      common_denominator =
          (common_denominator / divisor) * coordinate.denominator();
    }
    for (std::size_t index = 0; index < coordinates.size(); ++index) {
      numerators_[index] = coordinates[index].numerator() *
                           (common_denominator / coordinates[index].denominator());
    }
    denominator_ = std::move(common_denominator);
    normalize();
  }

  [[nodiscard]] static ExactRational3 from_record(const ExactRational3Record& record) {
    if (record.schema_version != schema_version) {
      throw std::invalid_argument("ExactRational3.schema_version must be 2.0.0");
    }
    if (record.unit != unit) {
      throw std::invalid_argument("ExactRational3.unit must be input_coordinate_unit");
    }
    ExactRational3 value{
        parse_canonical_integer(record.x_numerator),
        parse_canonical_integer(record.y_numerator),
        parse_canonical_integer(record.z_numerator),
        parse_canonical_positive_integer(record.denominator)};
    if (value.to_record() != record) {
      throw std::invalid_argument("ExactRational3 must be reduced canonically");
    }
    return value;
  }

  [[nodiscard]] const BigInt& numerator(std::size_t axis) const {
    if (axis >= numerators_.size()) {
      throw std::out_of_range("an ExactRational3 axis must be 0, 1 or 2");
    }
    return numerators_[axis];
  }

  [[nodiscard]] const BigInt& denominator() const noexcept { return denominator_; }

  [[nodiscard]] ExactRational coordinate(std::size_t axis) const {
    return ExactRational{numerator(axis), denominator_};
  }

  [[nodiscard]] ExactRational3Record to_record() const {
    return ExactRational3Record{
        schema_version,
        canonical_integer_string(numerators_[0]),
        canonical_integer_string(numerators_[1]),
        canonical_integer_string(numerators_[2]),
        canonical_integer_string(denominator_),
        unit};
  }

  [[nodiscard]] std::string canonical_json() const {
    const ExactRational3Record record = to_record();
    return "{\"denominator\":\"" + record.denominator +
           "\",\"schema_version\":\"" + record.schema_version +
           "\",\"unit\":\"" + record.unit +
           "\",\"x_numerator\":\"" + record.x_numerator +
           "\",\"y_numerator\":\"" + record.y_numerator +
           "\",\"z_numerator\":\"" + record.z_numerator + "\"}";
  }

  friend bool operator==(const ExactRational3& left, const ExactRational3& right) noexcept {
    return left.numerators_ == right.numerators_ && left.denominator_ == right.denominator_;
  }

 private:
  void normalize() {
    if (denominator_ == 0) {
      throw std::domain_error("an ExactRational3 denominator cannot be zero");
    }
    if (denominator_ < 0) {
      denominator_ = -denominator_;
      for (BigInt& numerator_value : numerators_) {
        numerator_value = -numerator_value;
      }
    }

    BigInt divisor = denominator_;
    for (const BigInt& numerator_value : numerators_) {
      divisor = greatest_common_divisor(divisor, numerator_value);
    }
    for (BigInt& numerator_value : numerators_) {
      numerator_value /= divisor;
    }
    denominator_ /= divisor;
  }

  std::array<BigInt, 3> numerators_{BigInt{0}, BigInt{0}, BigInt{0}};
  BigInt denominator_{1};
};

class CertifiedPoint3 {
 public:
  [[nodiscard]] static CertifiedPoint3 from_binary64(double x, double y, double z) {
    return from_binary64_bits({
        std::bit_cast<std::uint64_t>(x),
        std::bit_cast<std::uint64_t>(y),
        std::bit_cast<std::uint64_t>(z)});
  }

  [[nodiscard]] static CertifiedPoint3 from_binary64_bits(
      std::array<std::uint64_t, 3> input_bits) {
    const std::array<ExactRational, 3> coordinates{
        ExactRational::from_binary64_bits(input_bits[0]),
        ExactRational::from_binary64_bits(input_bits[1]),
        ExactRational::from_binary64_bits(input_bits[2])};
    return CertifiedPoint3{std::move(input_bits), ExactRational3{coordinates}};
  }

  [[nodiscard]] const std::array<std::uint64_t, 3>& input_bits() const noexcept {
    return input_bits_;
  }

  [[nodiscard]] std::array<std::uint64_t, 3> canonical_input_bits() const {
    return {
        canonicalize_binary64_bits(input_bits_[0]),
        canonicalize_binary64_bits(input_bits_[1]),
        canonicalize_binary64_bits(input_bits_[2])};
  }

  [[nodiscard]] const ExactRational3& exact() const noexcept { return exact_; }

  [[nodiscard]] ExactRational coordinate(std::size_t axis) const {
    return exact_.coordinate(axis);
  }

  [[nodiscard]] std::string replay_key() const {
    std::string output;
    for (std::size_t index = 0; index < input_bits_.size(); ++index) {
      if (index != 0) {
        output += ':';
      }
      output += binary64_hex(input_bits_[index]);
    }
    return output;
  }

 private:
  CertifiedPoint3(std::array<std::uint64_t, 3> input_bits, ExactRational3 exact)
      : input_bits_(std::move(input_bits)), exact_(std::move(exact)) {}

  std::array<std::uint64_t, 3> input_bits_{};
  ExactRational3 exact_{};
};

}  // namespace morsehgp3d::exact
