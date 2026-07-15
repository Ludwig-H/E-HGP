#pragma once

#include "morsehgp3d/exact/expansion.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace morsehgp3d::exact::detail::support_polynomial {

template <std::size_t SupportSize>
struct CanonicalSupport {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  std::array<CertifiedPoint3, SupportSize> points;
  // original_indices[canonical_index] is the corresponding input position.
  std::array<std::size_t, SupportSize> original_indices{};
};

[[nodiscard]] inline bool point_less(
    const CertifiedPoint3& left, const CertifiedPoint3& right) {
  const auto left_bits = left.canonical_input_bits();
  const auto right_bits = right.canonical_input_bits();
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const std::uint64_t left_key = binary64_total_order_key(left_bits[axis]);
    const std::uint64_t right_key = binary64_total_order_key(right_bits[axis]);
    if (left_key != right_key) {
      return left_key < right_key;
    }
  }
  return false;
}

[[nodiscard]] inline CertifiedPoint3 canonical_point(
    const CertifiedPoint3& point) {
  return CertifiedPoint3::from_binary64_bits(point.canonical_input_bits());
}

template <std::size_t SupportSize>
[[nodiscard]] CanonicalSupport<SupportSize> canonicalize_support(
    const std::array<CertifiedPoint3, SupportSize>& support) {
  CanonicalSupport<SupportSize> result{support, {}};
  for (std::size_t index = 0; index < SupportSize; ++index) {
    result.points[index] = canonical_point(result.points[index]);
    result.original_indices[index] = index;
  }
  for (std::size_t index = 1U; index < SupportSize; ++index) {
    CertifiedPoint3 point = result.points[index];
    const std::size_t original_index = result.original_indices[index];
    std::size_t position = index;
    while (position > 0U && point_less(point, result.points[position - 1U])) {
      result.points[position] = result.points[position - 1U];
      result.original_indices[position] = result.original_indices[position - 1U];
      --position;
    }
    result.points[position] = std::move(point);
    result.original_indices[position] = original_index;
  }
  return result;
}

template <std::size_t SupportSize>
class SignVectorAttempt {
 public:
  [[nodiscard]] static SignVectorAttempt uncertain() noexcept {
    return SignVectorAttempt{FilterState::uncertain, std::nullopt};
  }

  [[nodiscard]] static SignVectorAttempt certified(
      std::array<PredicateSign, SupportSize> signs) {
    return SignVectorAttempt{FilterState::certified, std::move(signs)};
  }

  [[nodiscard]] FilterState state() const noexcept { return state_; }

  [[nodiscard]] const std::optional<std::array<PredicateSign, SupportSize>>&
  signs() const noexcept {
    return signs_;
  }

 private:
  SignVectorAttempt(
      FilterState state,
      std::optional<std::array<PredicateSign, SupportSize>> signs) noexcept
      : state_(state), signs_(std::move(signs)) {}

  FilterState state_;
  std::optional<std::array<PredicateSign, SupportSize>> signs_;
};

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
using Matrix3 = std::array<std::array<Value, 3>, 3>;

template <typename Value>
[[nodiscard]] Value dot(const Vector3<Value>& left, const Vector3<Value>& right) {
  Value result = scalar<Value>(0.0);
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result = add(result, multiply(left[axis], right[axis]));
  }
  return result;
}

template <std::size_t Dimension, typename Value>
[[nodiscard]] Value determinant(const Matrix3<Value>& matrix) {
  static_assert(Dimension <= 3U);
  if constexpr (Dimension == 0U) {
    return scalar<Value>(1.0);
  } else if constexpr (Dimension == 1U) {
    return matrix[0][0];
  } else if constexpr (Dimension == 2U) {
    return subtract(
        multiply(matrix[0][0], matrix[1][1]),
        multiply(matrix[0][1], matrix[1][0]));
  } else {
    const Value first_minor = subtract(
        multiply(matrix[1][1], matrix[2][2]),
        multiply(matrix[1][2], matrix[2][1]));
    const Value second_minor = subtract(
        multiply(matrix[1][0], matrix[2][2]),
        multiply(matrix[1][2], matrix[2][0]));
    const Value third_minor = subtract(
        multiply(matrix[1][0], matrix[2][1]),
        multiply(matrix[1][1], matrix[2][0]));
    return add(
        subtract(
            multiply(matrix[0][0], first_minor),
            multiply(matrix[0][1], second_minor)),
        multiply(matrix[0][2], third_minor));
  }
}

template <typename Value, std::size_t SupportSize>
struct SupportEvaluation {
  static constexpr std::size_t dimension = SupportSize - 1U;
  std::array<Vector3<Value>, 3> directions{};
  Matrix3<Value> gram{};
  std::array<Value, 3> squared_direction_norms{};
  std::array<Value, 3> circumcenter_cramer_numerators{};
  Value gram_determinant{};
  CertifiedPoint3 anchor;
};

template <typename Value, std::size_t SupportSize>
[[nodiscard]] SupportEvaluation<Value, SupportSize> evaluate_support(
    const CanonicalSupport<SupportSize>& support) {
  constexpr std::size_t dimension = SupportSize - 1U;
  SupportEvaluation<Value, SupportSize> result{
      {}, {}, {}, {}, scalar<Value>(1.0), support.points[0]};
  for (std::size_t direction = 0; direction < dimension; ++direction) {
    for (std::size_t axis = 0; axis < 3U; ++axis) {
      result.directions[direction][axis] = subtract(
          scalar<Value>(support.points[direction + 1U].binary64_coordinate(axis)),
          scalar<Value>(support.points[0].binary64_coordinate(axis)));
    }
  }
  for (std::size_t row = 0; row < dimension; ++row) {
    for (std::size_t column = 0; column < dimension; ++column) {
      result.gram[row][column] =
          dot(result.directions[row], result.directions[column]);
    }
    result.squared_direction_norms[row] = result.gram[row][row];
  }
  result.gram_determinant = determinant<dimension>(result.gram);
  for (std::size_t column = 0; column < dimension; ++column) {
    Matrix3<Value> replaced = result.gram;
    for (std::size_t row = 0; row < dimension; ++row) {
      replaced[row][column] = result.squared_direction_norms[row];
    }
    if constexpr (dimension > 0U) {
      if (column < dimension) {
        switch (dimension) {
          case 1U:
            result.circumcenter_cramer_numerators[column] =
                determinant<1U>(replaced);
            break;
          case 2U:
            result.circumcenter_cramer_numerators[column] =
                determinant<2U>(replaced);
            break;
          case 3U:
            result.circumcenter_cramer_numerators[column] =
                determinant<3U>(replaced);
            break;
        }
      }
    }
  }
  return result;
}

template <typename Value, std::size_t SupportSize>
[[nodiscard]] Vector3<Value> query_delta(
    const CertifiedPoint3& query,
    const SupportEvaluation<Value, SupportSize>& support) {
  Vector3<Value> result{};
  const CertifiedPoint3 canonical_query = canonical_point(query);
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result[axis] = subtract(
        scalar<Value>(canonical_query.binary64_coordinate(axis)),
        scalar<Value>(support.anchor.binary64_coordinate(axis)));
  }
  return result;
}

template <typename Value, std::size_t SupportSize>
[[nodiscard]] std::array<Value, SupportSize>
evaluate_query_barycentric_numerators(
    const CertifiedPoint3& query,
    const SupportEvaluation<Value, SupportSize>& support) {
  constexpr std::size_t dimension = SupportSize - 1U;
  std::array<Value, SupportSize> numerators{};
  if constexpr (SupportSize == 1U) {
    numerators[0] = scalar<Value>(1.0);
    return numerators;
  }
  const Vector3<Value> delta = query_delta(query, support);
  std::array<Value, 3> right_hand_side{};
  Value sum = scalar<Value>(0.0);
  for (std::size_t row = 0; row < dimension; ++row) {
    right_hand_side[row] = dot(support.directions[row], delta);
  }
  for (std::size_t column = 0; column < dimension; ++column) {
    Matrix3<Value> replaced = support.gram;
    for (std::size_t row = 0; row < dimension; ++row) {
      replaced[row][column] = right_hand_side[row];
    }
    if constexpr (dimension == 1U) {
      numerators[column + 1U] = determinant<1U>(replaced);
    } else if constexpr (dimension == 2U) {
      numerators[column + 1U] = determinant<2U>(replaced);
    } else {
      numerators[column + 1U] = determinant<3U>(replaced);
    }
    sum = add(sum, numerators[column + 1U]);
  }
  numerators[0] = subtract(support.gram_determinant, sum);
  return numerators;
}

template <typename Value, std::size_t SupportSize>
[[nodiscard]] std::array<Value, SupportSize>
evaluate_circumcenter_barycentric_numerators(
    const SupportEvaluation<Value, SupportSize>& support) {
  constexpr std::size_t dimension = SupportSize - 1U;
  std::array<Value, SupportSize> numerators{};
  if constexpr (SupportSize == 1U) {
    numerators[0] = scalar<Value>(1.0);
    return numerators;
  }
  Value sum = scalar<Value>(0.0);
  for (std::size_t index = 0; index < dimension; ++index) {
    numerators[index + 1U] = support.circumcenter_cramer_numerators[index];
    sum = add(sum, numerators[index + 1U]);
  }
  numerators[0] = subtract(
      multiply(scalar<Value>(2.0), support.gram_determinant), sum);
  return numerators;
}

template <typename Value, std::size_t SupportSize>
[[nodiscard]] Value evaluate_sphere_side(
    const CertifiedPoint3& query,
    const SupportEvaluation<Value, SupportSize>& support) {
  constexpr std::size_t dimension = SupportSize - 1U;
  const Vector3<Value> delta = query_delta(query, support);
  Value result = multiply(support.gram_determinant, dot(delta, delta));
  for (std::size_t index = 0; index < dimension; ++index) {
    result = subtract(
        result,
        multiply(
            dot(support.directions[index], delta),
            support.circumcenter_cramer_numerators[index]));
  }
  return result;
}

template <typename Value, std::size_t SupportSize>
[[nodiscard]] Value evaluate_level_numerator(
    const SupportEvaluation<Value, SupportSize>& support) {
  constexpr std::size_t dimension = SupportSize - 1U;
  Value result = scalar<Value>(0.0);
  for (std::size_t index = 0; index < dimension; ++index) {
    result = add(
        result,
        multiply(
            support.squared_direction_norms[index],
            support.circumcenter_cramer_numerators[index]));
  }
  return result;
}

template <typename Value, std::size_t LeftSize, std::size_t RightSize>
[[nodiscard]] Value evaluate_level_order(
    const SupportEvaluation<Value, LeftSize>& left,
    const SupportEvaluation<Value, RightSize>& right) {
  return subtract(
      multiply(evaluate_level_numerator(left), right.gram_determinant),
      multiply(evaluate_level_numerator(right), left.gram_determinant));
}

[[nodiscard]] inline bool interval_is_strictly_positive(
    const Binary64Interval& value) {
  const FilterResult sign = sign_of_binary64_interval(value);
  return sign.state() == FilterState::certified && sign.sign().has_value() &&
         *sign.sign() == PredicateSign::positive;
}

[[nodiscard]] inline bool expansion_is_strictly_positive(
    const FloatingExpansion& value) {
  return value.valid() && value.sign() == PredicateSign::positive;
}

template <std::size_t SupportSize, typename NumeratorBuilder>
[[nodiscard]] SignVectorAttempt<SupportSize> filter_sign_vector(
    const CanonicalSupport<SupportSize>& support,
    NumeratorBuilder&& build_numerators) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return SignVectorAttempt<SupportSize>::uncertain();
  }
  const auto evaluation = evaluate_support<Binary64Interval>(support);
  bool valid = interval_is_strictly_positive(evaluation.gram_determinant);
  const auto numerators = build_numerators(evaluation);
  std::array<PredicateSign, SupportSize> signs{};
  for (std::size_t index = 0; index < SupportSize; ++index) {
    const FilterResult sign = sign_of_binary64_interval(numerators[index]);
    valid = valid && sign.state() == FilterState::certified &&
            sign.sign().has_value();
    if (sign.sign().has_value()) {
      signs[index] = *sign.sign();
    }
  }
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return SignVectorAttempt<SupportSize>::uncertain();
  }
  return SignVectorAttempt<SupportSize>::certified(std::move(signs));
}

template <std::size_t SupportSize, typename NumeratorBuilder>
[[nodiscard]] SignVectorAttempt<SupportSize> expansion_sign_vector(
    const CanonicalSupport<SupportSize>& support,
    NumeratorBuilder&& build_numerators) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return SignVectorAttempt<SupportSize>::uncertain();
  }
  const auto evaluation = evaluate_support<FloatingExpansion>(support);
  bool valid = expansion_is_strictly_positive(evaluation.gram_determinant);
  const auto numerators = build_numerators(evaluation);
  std::array<PredicateSign, SupportSize> signs{};
  for (std::size_t index = 0; index < SupportSize; ++index) {
    valid = valid && numerators[index].valid();
    if (numerators[index].valid()) {
      signs[index] = numerators[index].sign();
    }
  }
  valid = valid && expansion_arithmetic_exceptions_clear();
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return SignVectorAttempt<SupportSize>::uncertain();
  }
  return SignVectorAttempt<SupportSize>::certified(std::move(signs));
}

template <std::size_t SupportSize>
[[nodiscard]] SignVectorAttempt<SupportSize> filter_query_barycentric_signs(
    const CertifiedPoint3& query,
    const CanonicalSupport<SupportSize>& support) {
  return filter_sign_vector(
      support,
      [&query](const auto& evaluation) {
        return evaluate_query_barycentric_numerators(query, evaluation);
      });
}

template <std::size_t SupportSize>
[[nodiscard]] SignVectorAttempt<SupportSize> expansion_query_barycentric_signs(
    const CertifiedPoint3& query,
    const CanonicalSupport<SupportSize>& support) {
  return expansion_sign_vector(
      support,
      [&query](const auto& evaluation) {
        return evaluate_query_barycentric_numerators(query, evaluation);
      });
}

template <std::size_t SupportSize>
[[nodiscard]] SignVectorAttempt<SupportSize>
filter_circumcenter_barycentric_signs(
    const CanonicalSupport<SupportSize>& support) {
  return filter_sign_vector(
      support,
      [](const auto& evaluation) {
        return evaluate_circumcenter_barycentric_numerators(evaluation);
      });
}

template <std::size_t SupportSize>
[[nodiscard]] SignVectorAttempt<SupportSize>
expansion_circumcenter_barycentric_signs(
    const CanonicalSupport<SupportSize>& support) {
  return expansion_sign_vector(
      support,
      [](const auto& evaluation) {
        return evaluate_circumcenter_barycentric_numerators(evaluation);
      });
}

template <std::size_t SupportSize>
[[nodiscard]] FilterResult filter_sphere_side(
    const CertifiedPoint3& query,
    const CanonicalSupport<SupportSize>& support) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }
  const auto evaluation = evaluate_support<Binary64Interval>(support);
  const bool determinant_positive =
      interval_is_strictly_positive(evaluation.gram_determinant);
  const FilterResult result = sign_of_binary64_interval(
      evaluate_sphere_side(query, evaluation));
  const bool restored = environment.restore();
  if (!determinant_positive || !restored) {
    return FilterResult::uncertain();
  }
  return result;
}

template <std::size_t SupportSize>
[[nodiscard]] ExpansionResult expansion_sphere_side(
    const CertifiedPoint3& query,
    const CanonicalSupport<SupportSize>& support) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return ExpansionResult::uncertain();
  }
  const auto evaluation = evaluate_support<FloatingExpansion>(support);
  const FloatingExpansion result = evaluate_sphere_side(query, evaluation);
  const bool valid = expansion_is_strictly_positive(
                         evaluation.gram_determinant) &&
                     result.valid() && expansion_arithmetic_exceptions_clear();
  const PredicateSign sign = valid ? result.sign() : PredicateSign::zero;
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return ExpansionResult::uncertain();
  }
  return ExpansionResult::certified(sign);
}

template <std::size_t LeftSize, std::size_t RightSize>
[[nodiscard]] FilterResult filter_level_order(
    const CanonicalSupport<LeftSize>& left,
    const CanonicalSupport<RightSize>& right) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }
  const auto left_evaluation = evaluate_support<Binary64Interval>(left);
  const auto right_evaluation = evaluate_support<Binary64Interval>(right);
  const bool determinants_positive =
      interval_is_strictly_positive(left_evaluation.gram_determinant) &&
      interval_is_strictly_positive(right_evaluation.gram_determinant);
  const FilterResult result = sign_of_binary64_interval(
      evaluate_level_order(left_evaluation, right_evaluation));
  const bool restored = environment.restore();
  if (!determinants_positive || !restored) {
    return FilterResult::uncertain();
  }
  return result;
}

template <std::size_t LeftSize, std::size_t RightSize>
[[nodiscard]] ExpansionResult expansion_level_order(
    const CanonicalSupport<LeftSize>& left,
    const CanonicalSupport<RightSize>& right) {
  Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return ExpansionResult::uncertain();
  }
  const auto left_evaluation = evaluate_support<FloatingExpansion>(left);
  const auto right_evaluation = evaluate_support<FloatingExpansion>(right);
  const FloatingExpansion result =
      evaluate_level_order(left_evaluation, right_evaluation);
  const bool valid = expansion_is_strictly_positive(
                         left_evaluation.gram_determinant) &&
                     expansion_is_strictly_positive(
                         right_evaluation.gram_determinant) &&
                     result.valid() && expansion_arithmetic_exceptions_clear();
  const PredicateSign sign = valid ? result.sign() : PredicateSign::zero;
  const bool restored = environment.restore();
  if (!valid || !restored) {
    return ExpansionResult::uncertain();
  }
  return ExpansionResult::certified(sign);
}

}  // namespace morsehgp3d::exact::detail::support_polynomial
