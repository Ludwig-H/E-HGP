#pragma once

#include "morsehgp3d/exact/affine_provenance.hpp"
#include "morsehgp3d/exact/expansion.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>

namespace morsehgp3d::exact::detail::affine_polynomial {

template <typename Value>
[[nodiscard]] Value scalar(double value);

template <>
[[nodiscard]] inline Binary64Interval scalar<Binary64Interval>(double value) {
  return point_binary64_interval(value);
}

template <>
[[nodiscard]] inline FloatingExpansion scalar<FloatingExpansion>(double value) {
  return FloatingExpansion::scalar(value);
}

template <typename Value>
[[nodiscard]] Value add(const Value& left, const Value& right);

template <>
[[nodiscard]] inline Binary64Interval add(
    const Binary64Interval& left, const Binary64Interval& right) {
  return add_binary64_intervals(left, right);
}

template <>
[[nodiscard]] inline FloatingExpansion add(
    const FloatingExpansion& left, const FloatingExpansion& right) {
  return add_expansions(left, right);
}

template <typename Value>
[[nodiscard]] Value subtract(const Value& left, const Value& right);

template <>
[[nodiscard]] inline Binary64Interval subtract(
    const Binary64Interval& left, const Binary64Interval& right) {
  return subtract_binary64_intervals(left, right);
}

template <>
[[nodiscard]] inline FloatingExpansion subtract(
    const FloatingExpansion& left, const FloatingExpansion& right) {
  return subtract_expansions(left, right);
}

template <typename Value>
[[nodiscard]] Value multiply(const Value& left, const Value& right);

template <>
[[nodiscard]] inline Binary64Interval multiply(
    const Binary64Interval& left, const Binary64Interval& right) {
  return multiply_binary64_intervals(left, right);
}

template <>
[[nodiscard]] inline FloatingExpansion multiply(
    const FloatingExpansion& left, const FloatingExpansion& right) {
  return multiply_expansions(left, right);
}

template <typename Value>
using Vector3 = std::array<Value, 3>;

template <typename Value>
using Coefficients = std::array<Value, 4>;

template <typename Value>
using Matrix3 = std::array<std::array<Value, 3>, 3>;

template <typename Value>
using Matrix34 = std::array<std::array<Value, 4>, 3>;

template <typename Value>
using Matrix4 = std::array<std::array<Value, 4>, 4>;

template <typename Value>
[[nodiscard]] Vector3<Value> zero_vector() {
  return {scalar<Value>(0.0), scalar<Value>(0.0), scalar<Value>(0.0)};
}

template <typename Value>
[[nodiscard]] Coefficients<Value> zero_coefficients() {
  return {
      scalar<Value>(0.0),
      scalar<Value>(0.0),
      scalar<Value>(0.0),
      scalar<Value>(0.0)};
}

template <typename Value>
[[nodiscard]] Value negate(const Value& value) {
  return subtract(scalar<Value>(0.0), value);
}

template <typename Value>
[[nodiscard]] Vector3<Value> point_value(const CertifiedPoint3& point) {
  const CertifiedPoint3 canonical =
      affine_provenance_detail::canonical_point(point);
  return {
      scalar<Value>(canonical.binary64_coordinate(0U)),
      scalar<Value>(canonical.binary64_coordinate(1U)),
      scalar<Value>(canonical.binary64_coordinate(2U))};
}

template <typename Value>
[[nodiscard]] Vector3<Value> vector_subtract(
    const Vector3<Value>& left, const Vector3<Value>& right) {
  Vector3<Value> result = zero_vector<Value>();
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result[axis] = subtract(left[axis], right[axis]);
  }
  return result;
}

template <typename Value>
[[nodiscard]] Vector3<Value> cross(
    const Vector3<Value>& left, const Vector3<Value>& right) {
  return {
      subtract(
          multiply(left[1], right[2]), multiply(left[2], right[1])),
      subtract(
          multiply(left[2], right[0]), multiply(left[0], right[2])),
      subtract(
          multiply(left[0], right[1]), multiply(left[1], right[0]))};
}

template <typename Value>
[[nodiscard]] Value dot(
    const Vector3<Value>& left, const Vector3<Value>& right) {
  Value result = scalar<Value>(0.0);
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result = add(result, multiply(left[axis], right[axis]));
  }
  return result;
}

template <typename Value>
[[nodiscard]] Vector3<Value> evaluate_normal(
    const Binary64AffineProvenance& provenance) {
  Vector3<Value> result = zero_vector<Value>();
  if (provenance.origin() == Binary64AffineOrigin::explicit_coefficients) {
    const auto& bits = provenance.coefficient_bits();
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      result[axis] = scalar<Value>(std::bit_cast<double>(bits[axis]));
    }
  } else if (provenance.origin() == Binary64AffineOrigin::through_points) {
    const auto points = provenance.primary_points();
    if (points.size() != 3U || !provenance.secondary_points().empty()) {
      return result;
    }
    const Vector3<Value> anchor = point_value<Value>(points[0]);
    result = cross(
        vector_subtract(point_value<Value>(points[1]), anchor),
        vector_subtract(point_value<Value>(points[2]), anchor));
  } else {
    const Value two = scalar<Value>(2.0);
    for (const CertifiedPoint3& point : provenance.primary_points()) {
      const Vector3<Value> value = point_value<Value>(point);
      for (std::size_t axis = 0; axis < 3U; ++axis) {
        result[axis] = subtract(result[axis], multiply(two, value[axis]));
      }
    }
    for (const CertifiedPoint3& point : provenance.secondary_points()) {
      const Vector3<Value> value = point_value<Value>(point);
      for (std::size_t axis = 0; axis < 3U; ++axis) {
        result[axis] = add(result[axis], multiply(two, value[axis]));
      }
    }
  }
  if (provenance.orientation_multiplier() < 0) {
    for (Value& coordinate : result) {
      coordinate = negate(coordinate);
    }
  }
  return result;
}

template <typename Value>
[[nodiscard]] Coefficients<Value> evaluate_coefficients(
    const Binary64AffineProvenance& provenance) {
  Coefficients<Value> result = zero_coefficients<Value>();
  if (provenance.origin() == Binary64AffineOrigin::explicit_coefficients) {
    const auto& bits = provenance.coefficient_bits();
    for (std::size_t index = 0; index < result.size(); ++index) {
      result[index] = scalar<Value>(std::bit_cast<double>(bits[index]));
    }
  } else if (provenance.origin() == Binary64AffineOrigin::through_points) {
    const auto points = provenance.primary_points();
    if (points.size() != 3U || !provenance.secondary_points().empty()) {
      return result;
    }
    const Vector3<Value> anchor = point_value<Value>(points[0]);
    const Vector3<Value> normal = cross(
        vector_subtract(point_value<Value>(points[1]), anchor),
        vector_subtract(point_value<Value>(points[2]), anchor));
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      result[axis] = normal[axis];
    }
    result[3] = negate(dot(normal, anchor));
  } else {
    const Value two = scalar<Value>(2.0);
    const auto accumulate = [&result, &two](
                                std::span<const CertifiedPoint3> points,
                                bool primary) {
      for (const CertifiedPoint3& point : points) {
        const Vector3<Value> value = point_value<Value>(point);
        Value squared_norm = scalar<Value>(0.0);
        for (std::size_t axis = 0; axis < 3U; ++axis) {
          const Value doubled = multiply(two, value[axis]);
          result[axis] = primary ? subtract(result[axis], doubled)
                                 : add(result[axis], doubled);
          squared_norm = add(
              squared_norm, multiply(value[axis], value[axis]));
        }
        result[3] = primary ? add(result[3], squared_norm)
                            : subtract(result[3], squared_norm);
      }
    };
    accumulate(provenance.primary_points(), true);
    accumulate(provenance.secondary_points(), false);
  }
  if (provenance.orientation_multiplier() < 0) {
    for (Value& coefficient : result) {
      coefficient = negate(coefficient);
    }
  }
  return result;
}

template <typename Value>
[[nodiscard]] Value determinant_2x2(
    const Value& a, const Value& b, const Value& c, const Value& d) {
  return subtract(multiply(a, d), multiply(b, c));
}

template <typename Value>
[[nodiscard]] Value determinant_3x3(const Matrix3<Value>& matrix) {
  const Value first_minor = determinant_2x2(
      matrix[1][1], matrix[1][2], matrix[2][1], matrix[2][2]);
  const Value second_minor = determinant_2x2(
      matrix[1][0], matrix[1][2], matrix[2][0], matrix[2][2]);
  const Value third_minor = determinant_2x2(
      matrix[1][0], matrix[1][1], matrix[2][0], matrix[2][1]);
  return add(
      subtract(
          multiply(matrix[0][0], first_minor),
          multiply(matrix[0][1], second_minor)),
      multiply(matrix[0][2], third_minor));
}

template <typename Value>
[[nodiscard]] Value determinant_4x4(const Matrix4<Value>& matrix) {
  Value determinant = scalar<Value>(0.0);
  for (std::size_t excluded_column = 0; excluded_column < 4U;
       ++excluded_column) {
    Matrix3<Value> minor{{
        zero_vector<Value>(), zero_vector<Value>(), zero_vector<Value>()}};
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
    const Value term =
        multiply(matrix[0][excluded_column], determinant_3x3(minor));
    determinant = excluded_column % 2U == 0U
                      ? add(determinant, term)
                      : subtract(determinant, term);
  }
  return determinant;
}

template <typename Value>
[[nodiscard]] Matrix3<Value> normal_matrix(
    const std::array<const Binary64AffineProvenance*, 3>& provenances) {
  Matrix3<Value> result{{
      zero_vector<Value>(), zero_vector<Value>(), zero_vector<Value>()}};
  for (std::size_t row = 0; row < 3U; ++row) {
    result[row] = evaluate_normal<Value>(*provenances[row]);
  }
  return result;
}

template <typename Value>
[[nodiscard]] Matrix34<Value> augmented_matrix(
    const std::array<const Binary64AffineProvenance*, 3>& provenances) {
  Matrix34<Value> result{{
      zero_coefficients<Value>(),
      zero_coefficients<Value>(),
      zero_coefficients<Value>()}};
  for (std::size_t row = 0; row < 3U; ++row) {
    result[row] = evaluate_coefficients<Value>(*provenances[row]);
    result[row][3] = negate(result[row][3]);
  }
  return result;
}

struct RankSignature {
  std::size_t normal_rank;
  std::size_t augmented_rank;
  PredicateSign normal_determinant_sign;

  friend bool operator==(const RankSignature&, const RankSignature&) = default;
};

class RankAttempt {
 public:
  [[nodiscard]] static RankAttempt uncertain() noexcept {
    return RankAttempt{FilterState::uncertain, std::nullopt};
  }

  [[nodiscard]] static RankAttempt certified(RankSignature signature) {
    return RankAttempt{FilterState::certified, std::move(signature)};
  }

  [[nodiscard]] FilterState state() const noexcept { return state_; }

  [[nodiscard]] const std::optional<RankSignature>& signature() const noexcept {
    return signature_;
  }

 private:
  RankAttempt(
      FilterState state, std::optional<RankSignature> signature) noexcept
      : state_(state), signature_(std::move(signature)) {}

  FilterState state_;
  std::optional<RankSignature> signature_;
};

[[nodiscard]] inline RankAttempt filter_three_plane_ranks(
    const std::array<const Binary64AffineProvenance*, 3>& provenances) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return RankAttempt::uncertain();
  }
  const FilterResult determinant_sign = sign_of_binary64_interval(
      determinant_3x3(normal_matrix<Binary64Interval>(provenances)));
  const bool restored = environment.restore();
  if (!restored || determinant_sign.state() != FilterState::certified ||
      !determinant_sign.sign().has_value()) {
    return RankAttempt::uncertain();
  }
  return RankAttempt::certified(
      RankSignature{3U, 3U, *determinant_sign.sign()});
}

[[nodiscard]] inline std::optional<std::size_t> expansion_normal_rank(
    const Matrix3<FloatingExpansion>& matrix) {
  bool invalid = false;
  for (std::size_t first_row = 0U; first_row < 3U; ++first_row) {
    for (std::size_t second_row = first_row + 1U; second_row < 3U;
         ++second_row) {
      for (std::size_t first_column = 0U; first_column < 3U; ++first_column) {
        for (std::size_t second_column = first_column + 1U;
             second_column < 3U;
             ++second_column) {
          const FloatingExpansion minor = determinant_2x2(
              matrix[first_row][first_column],
              matrix[first_row][second_column],
              matrix[second_row][first_column],
              matrix[second_row][second_column]);
          invalid = invalid || !minor.valid();
          if (minor.valid() && minor.sign() != PredicateSign::zero) {
            return 2U;
          }
        }
      }
    }
  }
  return invalid ? std::nullopt : std::optional<std::size_t>{1U};
}

[[nodiscard]] inline std::optional<std::size_t> expansion_augmented_rank(
    const Matrix34<FloatingExpansion>& matrix, std::size_t normal_rank) {
  if (normal_rank == 2U) {
    bool invalid = false;
    for (std::size_t excluded_column = 0U; excluded_column < 4U;
         ++excluded_column) {
      Matrix3<FloatingExpansion> minor{{
          zero_vector<FloatingExpansion>(),
          zero_vector<FloatingExpansion>(),
          zero_vector<FloatingExpansion>()}};
      for (std::size_t row = 0U; row < 3U; ++row) {
        std::size_t target_column = 0U;
        for (std::size_t column = 0U; column < 4U; ++column) {
          if (column == excluded_column) {
            continue;
          }
          minor[row][target_column] = matrix[row][column];
          ++target_column;
        }
      }
      const FloatingExpansion determinant = determinant_3x3(minor);
      invalid = invalid || !determinant.valid();
      if (determinant.valid() && determinant.sign() != PredicateSign::zero) {
        return 3U;
      }
    }
    return invalid ? std::nullopt : std::optional<std::size_t>{2U};
  }

  bool invalid = false;
  for (std::size_t first_row = 0U; first_row < 3U; ++first_row) {
    for (std::size_t second_row = first_row + 1U; second_row < 3U;
         ++second_row) {
      for (std::size_t first_column = 0U; first_column < 4U; ++first_column) {
        for (std::size_t second_column = first_column + 1U;
             second_column < 4U;
             ++second_column) {
          const FloatingExpansion minor = determinant_2x2(
              matrix[first_row][first_column],
              matrix[first_row][second_column],
              matrix[second_row][first_column],
              matrix[second_row][second_column]);
          invalid = invalid || !minor.valid();
          if (minor.valid() && minor.sign() != PredicateSign::zero) {
            return 2U;
          }
        }
      }
    }
  }
  return invalid ? std::nullopt : std::optional<std::size_t>{1U};
}

[[nodiscard]] inline RankAttempt expansion_three_plane_ranks(
    const std::array<const Binary64AffineProvenance*, 3>& provenances) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return RankAttempt::uncertain();
  }
  const Matrix3<FloatingExpansion> normals =
      normal_matrix<FloatingExpansion>(provenances);
  const FloatingExpansion determinant = determinant_3x3(normals);
  std::optional<RankSignature> signature;
  if (determinant.valid() && determinant.sign() != PredicateSign::zero) {
    signature = RankSignature{3U, 3U, determinant.sign()};
  } else if (determinant.valid()) {
    const std::optional<std::size_t> normal_rank =
        expansion_normal_rank(normals);
    if (normal_rank.has_value()) {
      const Matrix34<FloatingExpansion> augmented =
          augmented_matrix<FloatingExpansion>(provenances);
      const std::optional<std::size_t> augmented_rank =
          expansion_augmented_rank(augmented, *normal_rank);
      if (augmented_rank.has_value()) {
        signature = RankSignature{
            *normal_rank, *augmented_rank, PredicateSign::zero};
      }
    }
  }
  const bool valid = signature.has_value() &&
                     expansion_arithmetic_exceptions_clear();
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return RankAttempt::uncertain();
  }
  return RankAttempt::certified(*signature);
}

template <typename Value>
[[nodiscard]] Value evaluate_orientation_2d(
    const Binary64AffineProvenance& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c) {
  const Vector3<Value> origin = point_value<Value>(a);
  const Vector3<Value> point_b = point_value<Value>(b);
  const Vector3<Value> point_c = point_value<Value>(c);
  return dot(
      evaluate_normal<Value>(plane),
      cross(
          vector_subtract(point_b, origin),
          vector_subtract(point_c, origin)));
}

[[nodiscard]] inline FilterResult filter_orientation_2d(
    const Binary64AffineProvenance& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }
  const FilterResult result = sign_of_binary64_interval(
      evaluate_orientation_2d<Binary64Interval>(plane, a, b, c));
  return environment.restore() ? result : FilterResult::uncertain();
}

[[nodiscard]] inline ExpansionResult expansion_orientation_2d(
    const Binary64AffineProvenance& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return ExpansionResult::uncertain();
  }
  return finish_expansion_sign(
      evaluate_orientation_2d<FloatingExpansion>(plane, a, b, c),
      environment);
}

template <typename Value>
[[nodiscard]] Value evaluate_plane_side(
    const Binary64AffineProvenance& plane, const CertifiedPoint3& point) {
  const Coefficients<Value> coefficients = evaluate_coefficients<Value>(plane);
  const Vector3<Value> value = point_value<Value>(point);
  Value result = coefficients[3];
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result = add(result, multiply(coefficients[axis], value[axis]));
  }
  return result;
}

[[nodiscard]] inline FilterResult filter_plane_side(
    const Binary64AffineProvenance& plane, const CertifiedPoint3& point) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }
  const FilterResult result = sign_of_binary64_interval(
      evaluate_plane_side<Binary64Interval>(plane, point));
  return environment.restore() ? result : FilterResult::uncertain();
}

[[nodiscard]] inline ExpansionResult expansion_plane_side(
    const Binary64AffineProvenance& plane, const CertifiedPoint3& point) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return ExpansionResult::uncertain();
  }
  return finish_expansion_sign(
      evaluate_plane_side<FloatingExpansion>(plane, point), environment);
}

template <typename Value>
[[nodiscard]] Value evaluate_fourth_plane_incidence(
    const std::array<const Binary64AffineProvenance*, 4>& provenances,
    Value* binding_determinant) {
  Matrix3<Value> normals{{
      zero_vector<Value>(), zero_vector<Value>(), zero_vector<Value>()}};
  Matrix4<Value> homogeneous{{
      zero_coefficients<Value>(),
      zero_coefficients<Value>(),
      zero_coefficients<Value>(),
      zero_coefficients<Value>()}};
  for (std::size_t row = 0U; row < 4U; ++row) {
    homogeneous[row] = evaluate_coefficients<Value>(*provenances[row]);
    if (row < 3U) {
      for (std::size_t column = 0U; column < 3U; ++column) {
        normals[row][column] = homogeneous[row][column];
      }
    }
  }
  *binding_determinant = determinant_3x3(normals);
  return determinant_4x4(homogeneous);
}

[[nodiscard]] inline FilterResult filter_fourth_plane_incidence(
    const std::array<const Binary64AffineProvenance*, 4>& provenances) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }
  Binary64Interval binding = point_binary64_interval(0.0);
  const Binary64Interval homogeneous =
      evaluate_fourth_plane_incidence<Binary64Interval>(provenances, &binding);
  const FilterResult binding_sign = sign_of_binary64_interval(binding);
  const FilterResult homogeneous_sign = sign_of_binary64_interval(homogeneous);
  std::optional<PredicateSign> sign;
  if (binding_sign.state() == FilterState::certified &&
      homogeneous_sign.state() == FilterState::certified &&
      binding_sign.sign().has_value() && homogeneous_sign.sign().has_value()) {
    sign = *binding_sign.sign() == *homogeneous_sign.sign()
               ? PredicateSign::positive
               : PredicateSign::negative;
  }
  const bool restored = environment.restore();
  if (!restored || !sign.has_value()) {
    return FilterResult::uncertain();
  }
  return FilterResult::certified(*sign);
}

[[nodiscard]] inline ExpansionResult expansion_fourth_plane_incidence(
    const std::array<const Binary64AffineProvenance*, 4>& provenances) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return ExpansionResult::uncertain();
  }
  FloatingExpansion binding = FloatingExpansion::scalar(0.0);
  const FloatingExpansion homogeneous =
      evaluate_fourth_plane_incidence<FloatingExpansion>(provenances, &binding);
  const bool valid = binding.valid() && homogeneous.valid() &&
                     binding.sign() != PredicateSign::zero &&
                     expansion_arithmetic_exceptions_clear();
  PredicateSign sign = PredicateSign::zero;
  if (valid && homogeneous.sign() != PredicateSign::zero) {
    sign = binding.sign() == homogeneous.sign() ? PredicateSign::positive
                                                 : PredicateSign::negative;
  }
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return ExpansionResult::uncertain();
  }
  return ExpansionResult::certified(sign);
}

}  // namespace morsehgp3d::exact::detail::affine_polynomial
