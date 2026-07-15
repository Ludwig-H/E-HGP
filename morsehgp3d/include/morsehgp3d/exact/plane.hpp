#pragma once

#include "morsehgp3d/exact/affine_provenance.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace morsehgp3d::exact {

struct ExactPlane3Record {
  std::string schema_version;
  std::string a;
  std::string b;
  std::string c;
  std::string d;

  friend bool operator==(const ExactPlane3Record&, const ExactPlane3Record&) = default;
};

// Exact oriented affine form A*x + B*y + C*z + D. The rational coefficients
// retain their scale, while oriented_key() exposes the primitive homogeneous
// representative used for sign-equivalent geometric classification. Unlike
// ExactPlane3, this type deliberately permits a zero normal.
class ExactAffineForm3 {
 public:
  static constexpr const char* schema_version = "2.0.0";

  [[nodiscard]] static ExactAffineForm3 from_integer_coefficients(
      std::array<BigInt, 4> coefficients) {
    std::array<ExactRational, 4> rationals{};
    for (std::size_t index = 0; index < coefficients.size(); ++index) {
      rationals[index] = ExactRational{std::move(coefficients[index])};
    }
    return ExactAffineForm3{std::move(rationals)};
  }

  [[nodiscard]] static ExactAffineForm3 from_rational_coefficients(
      std::array<ExactRational, 4> coefficients) {
    return ExactAffineForm3{std::move(coefficients)};
  }

  [[nodiscard]] static ExactAffineForm3 from_binary64_coefficient_bits(
      std::array<std::uint64_t, 4> coefficient_bits) {
    return from_binary64_provenance(
        Binary64AffineProvenance::from_coefficient_bits(coefficient_bits));
  }

  [[nodiscard]] static ExactAffineForm3 from_binary64_coefficients(
      const std::array<double, 4>& coefficients) {
    return from_binary64_provenance(
        Binary64AffineProvenance::from_coefficients(coefficients));
  }

  [[nodiscard]] static ExactAffineForm3 from_binary64_provenance(
      Binary64AffineProvenance provenance) {
    return ExactAffineForm3{
        provenance.exact_coefficients(), std::move(provenance)};
  }

  [[nodiscard]] const ExactRational& coefficient(std::size_t index) const {
    if (index >= coefficients_.size()) {
      throw std::out_of_range(
          "an ExactAffineForm3 coefficient index must be between zero and three");
    }
    return coefficients_[index];
  }

  [[nodiscard]] const ExactRational& a() const noexcept { return coefficients_[0]; }
  [[nodiscard]] const ExactRational& b() const noexcept { return coefficients_[1]; }
  [[nodiscard]] const ExactRational& c() const noexcept { return coefficients_[2]; }
  [[nodiscard]] const ExactRational& d() const noexcept { return coefficients_[3]; }

  [[nodiscard]] const BigInt& primitive_coefficient(std::size_t index) const {
    if (index >= primitive_coefficients_.size()) {
      throw std::out_of_range(
          "an ExactAffineForm3 primitive coefficient index must be between zero and three");
    }
    return primitive_coefficients_[index];
  }

  [[nodiscard]] bool has_zero_normal() const noexcept {
    return a().is_zero() && b().is_zero() && c().is_zero();
  }

  [[nodiscard]] bool is_identically_zero() const noexcept {
    return has_zero_normal() && d().is_zero();
  }

  [[nodiscard]] const std::optional<Binary64AffineProvenance>&
  binary64_provenance() const noexcept {
    return binary64_provenance_;
  }

  [[nodiscard]] ExactRational evaluate(const ExactRational3& point) const {
    ExactRational value = d();
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      value = value + coefficient(axis) * point.coordinate(axis);
    }
    return value;
  }

  [[nodiscard]] ExactRational evaluate(const CertifiedPoint3& point) const {
    return evaluate(point.exact());
  }

  [[nodiscard]] std::string oriented_key() const {
    return canonical_integer_string(primitive_coefficients_[0]) + ":" +
           canonical_integer_string(primitive_coefficients_[1]) + ":" +
           canonical_integer_string(primitive_coefficients_[2]) + ":" +
           canonical_integer_string(primitive_coefficients_[3]);
  }

  friend bool operator==(
      const ExactAffineForm3& left, const ExactAffineForm3& right) noexcept {
    return left.coefficients_ == right.coefficients_;
  }

 private:
  explicit ExactAffineForm3(
      std::array<ExactRational, 4> coefficients,
      std::optional<Binary64AffineProvenance> binary64_provenance = std::nullopt)
      : coefficients_(std::move(coefficients)),
        binary64_provenance_(std::move(binary64_provenance)) {
    if (binary64_provenance_.has_value() &&
        binary64_provenance_->exact_coefficients() != coefficients_) {
      throw std::invalid_argument(
          "binary64 affine provenance must reproduce every exact coefficient");
    }
    BigInt common_denominator = 1;
    for (const ExactRational& coefficient_value : coefficients_) {
      const BigInt divisor = greatest_common_divisor(
          common_denominator, coefficient_value.denominator());
      common_denominator =
          (common_denominator / divisor) * coefficient_value.denominator();
    }
    for (std::size_t index = 0; index < coefficients_.size(); ++index) {
      primitive_coefficients_[index] =
          coefficients_[index].numerator() *
          (common_denominator / coefficients_[index].denominator());
    }
    BigInt divisor = 0;
    for (const BigInt& coefficient_value : primitive_coefficients_) {
      divisor = greatest_common_divisor(divisor, coefficient_value);
    }
    if (divisor == 0) {
      return;
    }
    for (BigInt& coefficient_value : primitive_coefficients_) {
      coefficient_value /= divisor;
    }
  }

  std::array<ExactRational, 4> coefficients_{};
  std::array<BigInt, 4> primitive_coefficients_{};
  std::optional<Binary64AffineProvenance> binary64_provenance_;
};

// Oriented plane A*x + B*y + C*z + D = 0. Coefficients are primitive
// homogeneous integers. Positive rescaling is canonicalized away; negation
// reverses the orientation and is intentionally preserved.
class ExactPlane3 {
 public:
  static constexpr const char* schema_version = "2.0.0";

  [[nodiscard]] static ExactPlane3 from_integer_coefficients(
      std::array<BigInt, 4> coefficients) {
    return ExactPlane3{std::move(coefficients)};
  }

  [[nodiscard]] static ExactPlane3 from_rational_coefficients(
      const std::array<ExactRational, 4>& coefficients) {
    const ExactAffineForm3 form =
        ExactAffineForm3::from_rational_coefficients(coefficients);
    return from_integer_coefficients({
        form.primitive_coefficient(0U),
        form.primitive_coefficient(1U),
        form.primitive_coefficient(2U),
        form.primitive_coefficient(3U)});
  }

  [[nodiscard]] static ExactPlane3 from_binary64_coefficient_bits(
      std::array<std::uint64_t, 4> coefficient_bits) {
    return from_binary64_provenance(
        Binary64AffineProvenance::from_coefficient_bits(coefficient_bits));
  }

  [[nodiscard]] static ExactPlane3 from_binary64_coefficients(
      const std::array<double, 4>& coefficients) {
    return from_binary64_provenance(
        Binary64AffineProvenance::from_coefficients(coefficients));
  }

  [[nodiscard]] static ExactPlane3 from_binary64_provenance(
      Binary64AffineProvenance provenance) {
    ExactPlane3 result =
        from_rational_coefficients(provenance.exact_coefficients());
    result.binary64_provenance_ = std::move(provenance);
    return result;
  }

  [[nodiscard]] static ExactPlane3 from_affine_form(
      const ExactAffineForm3& form) {
    if (form.has_zero_normal()) {
      throw std::invalid_argument(
          "an affine form with zero normal cannot define an ExactPlane3");
    }
    if (form.binary64_provenance().has_value()) {
      return from_binary64_provenance(*form.binary64_provenance());
    }
    return from_integer_coefficients({
        form.primitive_coefficient(0U),
        form.primitive_coefficient(1U),
        form.primitive_coefficient(2U),
        form.primitive_coefficient(3U)});
  }

  [[nodiscard]] static ExactPlane3 from_record(const ExactPlane3Record& record) {
    if (record.schema_version != schema_version) {
      throw std::invalid_argument("ExactPlane3.schema_version must be 2.0.0");
    }
    ExactPlane3 plane = from_integer_coefficients({
        parse_canonical_integer(record.a),
        parse_canonical_integer(record.b),
        parse_canonical_integer(record.c),
        parse_canonical_integer(record.d)});
    if (plane.to_record() != record) {
      throw std::invalid_argument("ExactPlane3 coefficients must be primitive canonically");
    }
    return plane;
  }

  [[nodiscard]] static ExactPlane3 through_points(
      const ExactRational3& a,
      const ExactRational3& b,
      const ExactRational3& c) {
    std::array<ExactRational, 3> u{};
    std::array<ExactRational, 3> v{};
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      u[axis] = b.coordinate(axis) - a.coordinate(axis);
      v[axis] = c.coordinate(axis) - a.coordinate(axis);
    }
    const std::array<ExactRational, 3> normal{
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0]};
    if (normal[0].is_zero() && normal[1].is_zero() && normal[2].is_zero()) {
      throw std::invalid_argument(
          "three affinely dependent points do not define an oriented plane");
    }
    ExactRational offset;
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      offset = offset - normal[axis] * a.coordinate(axis);
    }
    return from_rational_coefficients(
        {normal[0], normal[1], normal[2], std::move(offset)});
  }

  [[nodiscard]] static ExactPlane3 through_points(
      const CertifiedPoint3& a,
      const CertifiedPoint3& b,
      const CertifiedPoint3& c) {
    Binary64AffineProvenance provenance =
        Binary64AffineProvenance::through_points(a, b, c);
    const auto coefficients = provenance.exact_coefficients();
    if (coefficients[0].is_zero() && coefficients[1].is_zero() &&
        coefficients[2].is_zero()) {
      throw std::invalid_argument(
          "three affinely dependent points do not define an oriented plane");
    }
    return from_binary64_provenance(std::move(provenance));
  }

  [[nodiscard]] const BigInt& coefficient(std::size_t index) const {
    if (index >= coefficients_.size()) {
      throw std::out_of_range("an ExactPlane3 coefficient index must be between zero and three");
    }
    return coefficients_[index];
  }

  [[nodiscard]] const BigInt& a() const noexcept { return coefficients_[0]; }
  [[nodiscard]] const BigInt& b() const noexcept { return coefficients_[1]; }
  [[nodiscard]] const BigInt& c() const noexcept { return coefficients_[2]; }
  [[nodiscard]] const BigInt& d() const noexcept { return coefficients_[3]; }

  [[nodiscard]] ExactRational evaluate(const ExactRational3& point) const {
    ExactRational value{d()};
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      value = value + ExactRational{coefficient(axis)} * point.coordinate(axis);
    }
    return value;
  }

  [[nodiscard]] ExactRational evaluate(const CertifiedPoint3& point) const {
    return evaluate(point.exact());
  }

  [[nodiscard]] const std::optional<Binary64AffineProvenance>&
  binary64_provenance() const noexcept {
    return binary64_provenance_;
  }

  [[nodiscard]] ExactPlane3 opposite() const {
    if (binary64_provenance_.has_value()) {
      return from_binary64_provenance(binary64_provenance_->opposite());
    }
    return from_integer_coefficients({-a(), -b(), -c(), -d()});
  }

  [[nodiscard]] bool same_geometric_plane(const ExactPlane3& other) const {
    return *this == other || opposite() == other;
  }

  [[nodiscard]] std::string oriented_key() const {
    return coefficient_key(coefficients_);
  }

  [[nodiscard]] std::string unoriented_key() const {
    std::array<BigInt, 4> canonical = coefficients_;
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      if (canonical[axis] == 0) {
        continue;
      }
      if (canonical[axis] < 0) {
        for (BigInt& coefficient_value : canonical) {
          coefficient_value = -coefficient_value;
        }
      }
      break;
    }
    return coefficient_key(canonical);
  }

  [[nodiscard]] ExactPlane3Record to_record() const {
    return ExactPlane3Record{
        schema_version,
        canonical_integer_string(a()),
        canonical_integer_string(b()),
        canonical_integer_string(c()),
        canonical_integer_string(d())};
  }

  [[nodiscard]] std::string canonical_json() const {
    const ExactPlane3Record record = to_record();
    return "{\"a\":\"" + record.a + "\",\"b\":\"" + record.b +
           "\",\"c\":\"" + record.c + "\",\"d\":\"" + record.d +
           "\",\"schema_version\":\"" + record.schema_version + "\"}";
  }

  friend bool operator==(
      const ExactPlane3& left, const ExactPlane3& right) noexcept {
    return left.coefficients_ == right.coefficients_;
  }

 private:
  explicit ExactPlane3(std::array<BigInt, 4> coefficients)
      : coefficients_(std::move(coefficients)) {
    if (a() == 0 && b() == 0 && c() == 0) {
      throw std::invalid_argument("an ExactPlane3 normal cannot be zero");
    }
    BigInt divisor = 0;
    for (const BigInt& coefficient_value : coefficients_) {
      divisor = greatest_common_divisor(divisor, coefficient_value);
    }
    if (divisor <= 0) {
      throw std::logic_error("an ExactPlane3 primitive divisor must be positive");
    }
    for (BigInt& coefficient_value : coefficients_) {
      coefficient_value /= divisor;
    }
  }

  [[nodiscard]] static std::string coefficient_key(
      const std::array<BigInt, 4>& coefficients) {
    return canonical_integer_string(coefficients[0]) + ":" +
           canonical_integer_string(coefficients[1]) + ":" +
           canonical_integer_string(coefficients[2]) + ":" +
           canonical_integer_string(coefficients[3]);
  }

  std::array<BigInt, 4> coefficients_{};
  std::optional<Binary64AffineProvenance> binary64_provenance_;
};

enum class AffineFormKind {
  proper_plane,
  constant_negative,
  constant_positive,
  identically_zero,
};

[[nodiscard]] inline std::string_view to_string(AffineFormKind kind) {
  switch (kind) {
    case AffineFormKind::proper_plane:
      return "proper_plane";
    case AffineFormKind::constant_negative:
      return "constant_negative";
    case AffineFormKind::constant_positive:
      return "constant_positive";
    case AffineFormKind::identically_zero:
      return "identically_zero";
  }
  throw std::invalid_argument("affine-form kind is invalid");
}

class AffineFormClassification {
 public:
  [[nodiscard]] AffineFormKind kind() const noexcept { return kind_; }

  [[nodiscard]] const std::optional<ExactPlane3>& plane() const noexcept {
    return plane_;
  }

 private:
  friend AffineFormClassification classify_affine_form(
      const ExactAffineForm3& form);

  AffineFormClassification(
      AffineFormKind kind, std::optional<ExactPlane3> plane)
      : kind_(kind), plane_(std::move(plane)) {
    if ((kind_ == AffineFormKind::proper_plane) != plane_.has_value()) {
      throw std::invalid_argument(
          "a proper affine form must contain exactly one plane");
    }
  }

  AffineFormKind kind_;
  std::optional<ExactPlane3> plane_;
};

[[nodiscard]] inline AffineFormClassification classify_affine_form(
    const ExactAffineForm3& form) {
  if (!form.has_zero_normal()) {
    return AffineFormClassification{
        AffineFormKind::proper_plane,
        ExactPlane3::from_affine_form(form)};
  }
  if (form.d().sign() < 0) {
    return AffineFormClassification{
        AffineFormKind::constant_negative, std::nullopt};
  }
  if (form.d().sign() > 0) {
    return AffineFormClassification{
        AffineFormKind::constant_positive, std::nullopt};
  }
  return AffineFormClassification{
      AffineFormKind::identically_zero, std::nullopt};
}

enum class ThreePlaneIntersectionKind {
  unique,
  empty,
  affine_family,
};

[[nodiscard]] inline std::string_view to_string(ThreePlaneIntersectionKind kind) {
  switch (kind) {
    case ThreePlaneIntersectionKind::unique:
      return "unique";
    case ThreePlaneIntersectionKind::empty:
      return "empty";
    case ThreePlaneIntersectionKind::affine_family:
      return "affine_family";
  }
  throw std::invalid_argument("three-plane intersection kind is invalid");
}

class ThreePlaneIntersection {
 public:
  [[nodiscard]] static ThreePlaneIntersection unique(ExactRational3 point) {
    return ThreePlaneIntersection{
        ThreePlaneIntersectionKind::unique, std::move(point), 3U, 3U, 0U};
  }

  [[nodiscard]] static ThreePlaneIntersection empty(
      std::size_t normal_rank, std::size_t augmented_rank) {
    return ThreePlaneIntersection{
        ThreePlaneIntersectionKind::empty,
        std::nullopt,
        normal_rank,
        augmented_rank,
        std::nullopt};
  }

  [[nodiscard]] static ThreePlaneIntersection affine_family(
      std::size_t normal_rank) {
    return ThreePlaneIntersection{
        ThreePlaneIntersectionKind::affine_family,
        std::nullopt,
        normal_rank,
        normal_rank,
        3U - normal_rank};
  }

  [[nodiscard]] ThreePlaneIntersectionKind kind() const noexcept { return kind_; }

  [[nodiscard]] const std::optional<ExactRational3>& point() const noexcept {
    return point_;
  }

  [[nodiscard]] std::size_t normal_rank() const noexcept { return normal_rank_; }

  [[nodiscard]] std::size_t augmented_rank() const noexcept {
    return augmented_rank_;
  }

  [[nodiscard]] const std::optional<std::size_t>& affine_dimension() const noexcept {
    return affine_dimension_;
  }

 private:
  ThreePlaneIntersection(
      ThreePlaneIntersectionKind kind,
      std::optional<ExactRational3> point,
      std::size_t normal_rank,
      std::size_t augmented_rank,
      std::optional<std::size_t> affine_dimension)
      : kind_(kind),
        point_(std::move(point)),
        normal_rank_(normal_rank),
        augmented_rank_(augmented_rank),
        affine_dimension_(affine_dimension) {
    if ((kind_ == ThreePlaneIntersectionKind::unique) != point_.has_value()) {
      throw std::invalid_argument(
          "a unique three-plane intersection must contain exactly one point");
    }
    if (normal_rank_ == 0U || normal_rank_ > 3U ||
        augmented_rank_ < normal_rank_ || augmented_rank_ > 3U) {
      throw std::invalid_argument("three-plane intersection ranks are invalid");
    }
    if (kind_ == ThreePlaneIntersectionKind::unique &&
        (normal_rank_ != 3U || augmented_rank_ != 3U ||
         !affine_dimension_.has_value() || *affine_dimension_ != 0U)) {
      throw std::invalid_argument("a unique three-plane intersection must have rank three");
    }
    if (kind_ == ThreePlaneIntersectionKind::empty &&
        (normal_rank_ >= 3U || augmented_rank_ != normal_rank_ + 1U ||
         affine_dimension_.has_value())) {
      throw std::invalid_argument("an empty three-plane intersection must be inconsistent");
    }
    if (kind_ == ThreePlaneIntersectionKind::affine_family &&
        (normal_rank_ >= 3U || augmented_rank_ != normal_rank_ ||
         !affine_dimension_.has_value() ||
         *affine_dimension_ != 3U - normal_rank_)) {
      throw std::invalid_argument("an affine three-plane family has invalid dimension");
    }
  }

  ThreePlaneIntersectionKind kind_;
  std::optional<ExactRational3> point_;
  std::size_t normal_rank_;
  std::size_t augmented_rank_;
  std::optional<std::size_t> affine_dimension_;
};

namespace detail {

[[nodiscard]] inline BigInt determinant_2x2(
    const BigInt& a,
    const BigInt& b,
    const BigInt& c,
    const BigInt& d) {
  return a * d - b * c;
}

[[nodiscard]] inline BigInt determinant_3x3(
    const std::array<std::array<BigInt, 3>, 3>& matrix) {
  return matrix[0][0] *
             determinant_2x2(
                 matrix[1][1], matrix[1][2], matrix[2][1], matrix[2][2]) -
         matrix[0][1] *
             determinant_2x2(
                 matrix[1][0], matrix[1][2], matrix[2][0], matrix[2][2]) +
         matrix[0][2] *
             determinant_2x2(
                 matrix[1][0], matrix[1][1], matrix[2][0], matrix[2][1]);
}

[[nodiscard]] inline BigInt determinant_4x4(
    const std::array<std::array<BigInt, 4>, 4>& matrix) {
  BigInt determinant = 0;
  for (std::size_t excluded_column = 0; excluded_column < 4U; ++excluded_column) {
    std::array<std::array<BigInt, 3>, 3> minor{};
    for (std::size_t row = 1U; row < 4U; ++row) {
      std::size_t minor_column = 0U;
      for (std::size_t column = 0U; column < 4U; ++column) {
        if (column == excluded_column) {
          continue;
        }
        minor[row - 1U][minor_column] = matrix[row][column];
        ++minor_column;
      }
    }
    const BigInt term =
        matrix[0][excluded_column] * determinant_3x3(minor);
    if (excluded_column % 2U == 0U) {
      determinant += term;
    } else {
      determinant -= term;
    }
  }
  return determinant;
}

template <std::size_t Columns>
[[nodiscard]] inline std::size_t matrix_rank_3xn(
    const std::array<std::array<BigInt, Columns>, 3>& matrix) {
  if constexpr (Columns >= 3U) {
    for (std::size_t first = 0; first < Columns; ++first) {
      for (std::size_t second = first + 1U; second < Columns; ++second) {
        for (std::size_t third = second + 1U; third < Columns; ++third) {
          const std::array<std::array<BigInt, 3>, 3> minor{{
              {matrix[0][first], matrix[0][second], matrix[0][third]},
              {matrix[1][first], matrix[1][second], matrix[1][third]},
              {matrix[2][first], matrix[2][second], matrix[2][third]}}};
          if (determinant_3x3(minor) != 0) {
            return 3U;
          }
        }
      }
    }
  }
  for (std::size_t first_row = 0; first_row < 3U; ++first_row) {
    for (std::size_t second_row = first_row + 1U; second_row < 3U; ++second_row) {
      for (std::size_t first_column = 0; first_column < Columns; ++first_column) {
        for (std::size_t second_column = first_column + 1U;
             second_column < Columns;
             ++second_column) {
          if (determinant_2x2(
                  matrix[first_row][first_column],
                  matrix[first_row][second_column],
                  matrix[second_row][first_column],
                  matrix[second_row][second_column]) != 0) {
            return 2U;
          }
        }
      }
    }
  }
  for (const auto& row : matrix) {
    for (const BigInt& value : row) {
      if (value != 0) {
        return 1U;
      }
    }
  }
  return 0U;
}

}  // namespace detail

[[nodiscard]] inline ThreePlaneIntersection intersect_three_planes(
    const ExactPlane3& first,
    const ExactPlane3& second,
    const ExactPlane3& third) {
  const std::array<const ExactPlane3*, 3> planes{&first, &second, &third};
  std::array<std::array<BigInt, 3>, 3> normals{};
  std::array<std::array<BigInt, 4>, 3> augmented{};
  for (std::size_t row = 0; row < planes.size(); ++row) {
    for (std::size_t column = 0; column < 3U; ++column) {
      normals[row][column] = planes[row]->coefficient(column);
      augmented[row][column] = planes[row]->coefficient(column);
    }
    augmented[row][3] = -planes[row]->d();
  }

  const BigInt denominator = detail::determinant_3x3(normals);
  if (denominator != 0) {
    auto numerator_matrix = normals;
    for (std::size_t row = 0; row < planes.size(); ++row) {
      numerator_matrix[row][0] = -planes[row]->d();
    }
    const BigInt x_numerator = detail::determinant_3x3(numerator_matrix);
    numerator_matrix = normals;
    for (std::size_t row = 0; row < planes.size(); ++row) {
      numerator_matrix[row][1] = -planes[row]->d();
    }
    const BigInt y_numerator = detail::determinant_3x3(numerator_matrix);
    numerator_matrix = normals;
    for (std::size_t row = 0; row < planes.size(); ++row) {
      numerator_matrix[row][2] = -planes[row]->d();
    }
    ExactRational3 point{
        x_numerator,
        y_numerator,
        detail::determinant_3x3(numerator_matrix),
        denominator};
    if (!first.evaluate(point).is_zero() || !second.evaluate(point).is_zero() ||
        !third.evaluate(point).is_zero()) {
      throw std::logic_error("Cramer's rule produced a nonincident exact point");
    }
    return ThreePlaneIntersection::unique(std::move(point));
  }

  const std::size_t normal_rank = detail::matrix_rank_3xn(normals);
  const std::size_t augmented_rank = detail::matrix_rank_3xn(augmented);
  if (normal_rank < augmented_rank) {
    return ThreePlaneIntersection::empty(normal_rank, augmented_rank);
  }
  return ThreePlaneIntersection::affine_family(normal_rank);
}

}  // namespace morsehgp3d::exact
