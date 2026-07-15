#pragma once

#include "morsehgp3d/exact/rational.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace morsehgp3d::exact {

class ExactLevel;

// Homogeneous witness for left - right. Both denominators are strictly
// positive, so its sign is exactly the order of the represented levels.
[[nodiscard]] inline BigInt exact_level_cross_product_difference(
    const ExactLevel& left, const ExactLevel& right);

struct ExactLevelRecord {
  std::string schema_version;
  std::string numerator;
  std::string denominator;
  std::string unit;

  friend bool operator==(const ExactLevelRecord&, const ExactLevelRecord&) = default;
};

class ExactLevel {
 public:
  static constexpr const char* schema_version = "2.0.0";
  static constexpr const char* unit = "input_coordinate_unit_squared";

  ExactLevel() = default;

  ExactLevel(BigInt numerator, BigInt denominator = 1)
      : value_(std::move(numerator), std::move(denominator)) {
    require_nonnegative();
  }

  explicit ExactLevel(ExactRational value) : value_(std::move(value)) {
    require_nonnegative();
  }

  [[nodiscard]] static ExactLevel from_record(const ExactLevelRecord& record) {
    if (record.schema_version != schema_version) {
      throw std::invalid_argument("ExactLevel.schema_version must be 2.0.0");
    }
    if (record.unit != unit) {
      throw std::invalid_argument("ExactLevel.unit must be input_coordinate_unit_squared");
    }

    ExactLevel level{
        parse_canonical_nonnegative_integer(record.numerator),
        parse_canonical_positive_integer(record.denominator)};
    if (level.numerator_string() != record.numerator ||
        level.denominator_string() != record.denominator) {
      throw std::invalid_argument("ExactLevel must be reduced canonically");
    }
    return level;
  }

  [[nodiscard]] const BigInt& numerator() const noexcept { return value_.numerator(); }
  [[nodiscard]] const BigInt& denominator() const noexcept { return value_.denominator(); }
  [[nodiscard]] const ExactRational& rational() const noexcept { return value_; }

  [[nodiscard]] std::string numerator_string() const {
    return canonical_integer_string(numerator());
  }

  [[nodiscard]] std::string denominator_string() const {
    return canonical_integer_string(denominator());
  }

  [[nodiscard]] std::string canonical_key() const { return value_.canonical_key(); }

  [[nodiscard]] ExactLevelRecord to_record() const {
    return ExactLevelRecord{
        schema_version, numerator_string(), denominator_string(), unit};
  }

  [[nodiscard]] std::string canonical_json() const {
    return "{\"denominator\":\"" + denominator_string() +
           "\",\"numerator\":\"" + numerator_string() +
           "\",\"schema_version\":\"" + std::string{schema_version} +
           "\",\"unit\":\"" + std::string{unit} + "\"}";
  }

  // Hash-table accelerator only. Public v2 identifiers use domain-separated SHA-256.
  [[nodiscard]] std::uint64_t table_hash64() const {
    constexpr std::uint64_t offset_basis = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    std::uint64_t hash = offset_basis;
    for (const char character : canonical_key()) {
      hash ^= static_cast<unsigned char>(character);
      hash *= prime;
    }
    return hash;
  }

  friend bool operator==(const ExactLevel& left, const ExactLevel& right) noexcept {
    return left.value_ == right.value_;
  }

  friend std::strong_ordering operator<=>(const ExactLevel& left, const ExactLevel& right) {
    const BigInt difference =
        exact_level_cross_product_difference(left, right);
    if (difference < 0) {
      return std::strong_ordering::less;
    }
    if (difference > 0) {
      return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
  }

 private:
  void require_nonnegative() const {
    if (value_.sign() < 0) {
      throw std::domain_error("an ExactLevel squared radius cannot be negative");
    }
  }

  ExactRational value_{};
};

[[nodiscard]] inline BigInt exact_level_cross_product_difference(
    const ExactLevel& left, const ExactLevel& right) {
  return left.numerator() * right.denominator() -
         right.numerator() * left.denominator();
}

struct ExactLevelHash {
  [[nodiscard]] std::size_t operator()(const ExactLevel& level) const {
    return static_cast<std::size_t>(level.table_hash64());
  }
};

}  // namespace morsehgp3d::exact
