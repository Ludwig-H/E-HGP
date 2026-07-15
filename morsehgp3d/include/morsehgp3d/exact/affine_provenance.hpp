#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::exact {

enum class Binary64AffineOrigin {
  explicit_coefficients,
  through_points,
  power_bisector,
};

namespace affine_provenance_detail {

[[nodiscard]] inline CertifiedPoint3 canonical_point(
    const CertifiedPoint3& point) {
  return CertifiedPoint3::from_binary64_bits(point.canonical_input_bits());
}

[[nodiscard]] inline bool point_less(
    const CertifiedPoint3& left, const CertifiedPoint3& right) {
  const auto left_bits = left.canonical_input_bits();
  const auto right_bits = right.canonical_input_bits();
  for (std::size_t axis = 0; axis < left_bits.size(); ++axis) {
    const std::uint64_t left_key = binary64_total_order_key(left_bits[axis]);
    const std::uint64_t right_key = binary64_total_order_key(right_bits[axis]);
    if (left_key != right_key) {
      return left_key < right_key;
    }
  }
  return false;
}

[[nodiscard]] inline std::vector<CertifiedPoint3> canonical_points(
    std::span<const CertifiedPoint3> points) {
  std::vector<CertifiedPoint3> result;
  result.reserve(points.size());
  for (const CertifiedPoint3& point : points) {
    result.push_back(canonical_point(point));
  }
  std::sort(result.begin(), result.end(), point_less);
  return result;
}

[[nodiscard]] inline bool point_sequence_less(
    const std::vector<CertifiedPoint3>& left,
    const std::vector<CertifiedPoint3>& right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(), point_less);
}

using ExactCoefficients = std::array<ExactRational, 4>;

[[nodiscard]] inline ExactCoefficients through_point_coefficients(
    std::span<const CertifiedPoint3> points) {
  if (points.size() != 3U) {
    throw std::logic_error(
        "through-point affine provenance must contain exactly three points");
  }
  std::array<ExactRational, 3> first_direction{};
  std::array<ExactRational, 3> second_direction{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    first_direction[axis] =
        points[1].coordinate(axis) - points[0].coordinate(axis);
    second_direction[axis] =
        points[2].coordinate(axis) - points[0].coordinate(axis);
  }
  ExactCoefficients coefficients{};
  coefficients[0] = first_direction[1] * second_direction[2] -
                    first_direction[2] * second_direction[1];
  coefficients[1] = first_direction[2] * second_direction[0] -
                    first_direction[0] * second_direction[2];
  coefficients[2] = first_direction[0] * second_direction[1] -
                    first_direction[1] * second_direction[0];
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    coefficients[3] =
        coefficients[3] - coefficients[axis] * points[0].coordinate(axis);
  }
  return coefficients;
}

[[nodiscard]] inline ExactCoefficients power_bisector_coefficients(
    std::span<const CertifiedPoint3> r,
    std::span<const CertifiedPoint3> q) {
  if (r.empty() || r.size() != q.size()) {
    throw std::logic_error(
        "power-bisector affine provenance must contain two equal nonempty labels");
  }
  ExactCoefficients coefficients{};
  const auto accumulate = [&coefficients](
                              std::span<const CertifiedPoint3> points,
                              int coordinate_sign,
                              int norm_sign) {
    const ExactRational signed_two{BigInt{2 * coordinate_sign}};
    const ExactRational signed_one{BigInt{norm_sign}};
    for (const CertifiedPoint3& point : points) {
      ExactRational squared_norm;
      for (std::size_t axis = 0; axis < 3U; ++axis) {
        const ExactRational coordinate = point.coordinate(axis);
        coefficients[axis] =
            coefficients[axis] + signed_two * coordinate;
        squared_norm = squared_norm + coordinate * coordinate;
      }
      coefficients[3] = coefficients[3] + signed_one * squared_norm;
    }
  };
  // H_RQ(y) = -2 <y, S_R-S_Q> + (N_R-N_Q).
  accumulate(r, -1, 1);
  accumulate(q, 1, -1);
  return coefficients;
}

}  // namespace affine_provenance_detail

// A closed recipe from finite binary64 inputs to an oriented affine form.
// The recipe is deliberately separate from scientific identity and canonical
// serialization. Its only role is to let interval and expansion stages
// reevaluate homogeneous polynomials without converting primitive BigInt
// coefficients back to floating point.
class Binary64AffineProvenance {
 public:
  [[nodiscard]] static Binary64AffineProvenance from_coefficient_bits(
      std::array<std::uint64_t, 4> coefficient_bits) {
    for (std::uint64_t& bits : coefficient_bits) {
      bits = canonicalize_binary64_bits(bits);
      static_cast<void>(ExactRational::from_binary64_bits(bits));
    }
    return Binary64AffineProvenance{
        Binary64AffineOrigin::explicit_coefficients,
        coefficient_bits,
        {},
        {},
        1};
  }

  [[nodiscard]] static Binary64AffineProvenance from_coefficients(
      const std::array<double, 4>& coefficients) {
    std::array<std::uint64_t, 4> bits{};
    for (std::size_t index = 0; index < bits.size(); ++index) {
      bits[index] = std::bit_cast<std::uint64_t>(coefficients[index]);
    }
    return from_coefficient_bits(bits);
  }

  [[nodiscard]] static Binary64AffineProvenance through_points(
      const CertifiedPoint3& a,
      const CertifiedPoint3& b,
      const CertifiedPoint3& c) {
    struct IndexedPoint {
      CertifiedPoint3 point;
      std::size_t original_index;
    };
    std::array<IndexedPoint, 3> indexed{{
        {affine_provenance_detail::canonical_point(a), 0U},
        {affine_provenance_detail::canonical_point(b), 1U},
        {affine_provenance_detail::canonical_point(c), 2U}}};
    std::sort(
        indexed.begin(),
        indexed.end(),
        [](const IndexedPoint& left, const IndexedPoint& right) {
          return affine_provenance_detail::point_less(left.point, right.point);
        });
    std::size_t inversions = 0U;
    for (std::size_t first = 0; first < indexed.size(); ++first) {
      for (std::size_t second = first + 1U; second < indexed.size(); ++second) {
        if (indexed[first].original_index > indexed[second].original_index) {
          ++inversions;
        }
      }
    }
    std::vector<CertifiedPoint3> points;
    points.reserve(indexed.size());
    for (IndexedPoint& entry : indexed) {
      points.push_back(std::move(entry.point));
    }
    return Binary64AffineProvenance{
        Binary64AffineOrigin::through_points,
        {},
        std::move(points),
        {},
        inversions % 2U == 0U ? 1 : -1};
  }

  [[nodiscard]] static Binary64AffineProvenance power_bisector(
      std::span<const CertifiedPoint3> r,
      std::span<const CertifiedPoint3> q) {
    if (r.empty() || r.size() != q.size() || r.size() > 10U) {
      throw std::invalid_argument(
          "binary64 power-bisector provenance requires equal label sizes from one to ten");
    }
    std::vector<CertifiedPoint3> primary =
        affine_provenance_detail::canonical_points(r);
    std::vector<CertifiedPoint3> secondary =
        affine_provenance_detail::canonical_points(q);
    int orientation_multiplier = 1;
    if (affine_provenance_detail::point_sequence_less(secondary, primary)) {
      std::swap(primary, secondary);
      orientation_multiplier = -1;
    }
    return Binary64AffineProvenance{
        Binary64AffineOrigin::power_bisector,
        {},
        std::move(primary),
        std::move(secondary),
        orientation_multiplier};
  }

  [[nodiscard]] Binary64AffineOrigin origin() const noexcept { return origin_; }

  [[nodiscard]] std::span<const CertifiedPoint3> primary_points() const noexcept {
    return primary_points_;
  }

  [[nodiscard]] std::span<const CertifiedPoint3> secondary_points() const noexcept {
    return secondary_points_;
  }

  [[nodiscard]] int orientation_multiplier() const noexcept {
    return orientation_multiplier_;
  }

  [[nodiscard]] const std::array<std::uint64_t, 4>& coefficient_bits()
      const noexcept {
    return coefficient_bits_;
  }

  [[nodiscard]] std::array<ExactRational, 4> exact_coefficients() const {
    std::array<ExactRational, 4> coefficients{};
    if (origin_ == Binary64AffineOrigin::explicit_coefficients) {
      for (std::size_t index = 0; index < coefficients.size(); ++index) {
        coefficients[index] =
            ExactRational::from_binary64_bits(coefficient_bits_[index]);
      }
    } else if (origin_ == Binary64AffineOrigin::through_points) {
      coefficients =
          affine_provenance_detail::through_point_coefficients(primary_points_);
    } else {
      coefficients = affine_provenance_detail::power_bisector_coefficients(
          primary_points_, secondary_points_);
    }
    if (orientation_multiplier_ < 0) {
      for (ExactRational& coefficient : coefficients) {
        coefficient = -coefficient;
      }
    }
    return coefficients;
  }

  [[nodiscard]] Binary64AffineProvenance opposite() const {
    if (origin_ == Binary64AffineOrigin::explicit_coefficients) {
      std::array<std::uint64_t, 4> opposite_bits = coefficient_bits_;
      for (std::uint64_t& bits : opposite_bits) {
        bits = canonicalize_binary64_bits(bits ^ (std::uint64_t{1} << 63U));
      }
      return from_coefficient_bits(opposite_bits);
    }
    Binary64AffineProvenance result = *this;
    result.orientation_multiplier_ = -result.orientation_multiplier_;
    return result;
  }

  [[nodiscard]] std::string canonical_key() const {
    std::string result = std::to_string(static_cast<int>(origin_)) + ":" +
                         std::to_string(orientation_multiplier_);
    if (origin_ == Binary64AffineOrigin::explicit_coefficients) {
      for (const std::uint64_t bits : coefficient_bits_) {
        result += ":" + binary64_hex(bits);
      }
      return result;
    }
    const auto append_points = [&result](
                                   std::span<const CertifiedPoint3> points) {
      result += ":" + std::to_string(points.size());
      for (const CertifiedPoint3& point : points) {
        result += ":" + point.replay_key();
      }
    };
    append_points(primary_points_);
    append_points(secondary_points_);
    return result;
  }

 private:
  Binary64AffineProvenance(
      Binary64AffineOrigin origin,
      std::array<std::uint64_t, 4> coefficient_bits,
      std::vector<CertifiedPoint3> primary_points,
      std::vector<CertifiedPoint3> secondary_points,
      int orientation_multiplier)
      : origin_(origin),
        coefficient_bits_(coefficient_bits),
        primary_points_(std::move(primary_points)),
        secondary_points_(std::move(secondary_points)),
        orientation_multiplier_(orientation_multiplier) {
    if (orientation_multiplier_ != -1 && orientation_multiplier_ != 1) {
      throw std::invalid_argument(
          "binary64 affine provenance orientation must be negative or positive one");
    }
  }

  Binary64AffineOrigin origin_;
  std::array<std::uint64_t, 4> coefficient_bits_{};
  std::vector<CertifiedPoint3> primary_points_;
  std::vector<CertifiedPoint3> secondary_points_;
  int orientation_multiplier_;
};

}  // namespace morsehgp3d::exact
