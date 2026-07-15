#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::exact {

inline constexpr std::size_t maximum_power_label_cardinality = 10U;

// Exact moments used by the affine top-k power comparison. Canonical point IDs
// define the label; normalized rational sums make its numeric value deterministic.
class ExactLabelMoments {
 public:
  ExactLabelMoments() = default;

  [[nodiscard]] static ExactLabelMoments from_canonical_ids(
      std::span<const std::uint32_t> ids,
      std::span<const CertifiedPoint3> point_table) {
    ExactLabelMoments result;
    bool has_previous = false;
    std::uint32_t previous = 0;
    for (const std::uint32_t id : ids) {
      if (static_cast<std::size_t>(id) >= point_table.size()) {
        throw std::out_of_range("an exact label point identifier is out of range");
      }
      if (has_previous && id <= previous) {
        throw std::invalid_argument(
            "exact label point identifiers must be sorted and unique");
      }
      result.add_unchecked(point_table[static_cast<std::size_t>(id)]);
      previous = id;
      has_previous = true;
    }
    std::sort(
        result.source_points_.begin(),
        result.source_points_.end(),
        [](const CertifiedPoint3& left, const CertifiedPoint3& right) {
          const auto left_bits = left.canonical_input_bits();
          const auto right_bits = right.canonical_input_bits();
          for (std::size_t axis = 0; axis < 3U; ++axis) {
            const std::uint64_t left_key =
                binary64_total_order_key(left_bits[axis]);
            const std::uint64_t right_key =
                binary64_total_order_key(right_bits[axis]);
            if (left_key != right_key) {
              return left_key < right_key;
            }
          }
          return false;
        });
    return result;
  }

  [[nodiscard]] std::size_t cardinality() const noexcept { return cardinality_; }

  [[nodiscard]] const ExactRational& coordinate_sum(std::size_t axis) const {
    if (axis >= coordinate_sum_.size()) {
      throw std::out_of_range("an exact label moment axis must be 0, 1 or 2");
    }
    return coordinate_sum_[axis];
  }

  [[nodiscard]] const ExactRational& squared_norm_sum() const noexcept {
    return squared_norm_sum_;
  }

  // Canonically coordinate-sorted binary64 provenance for adaptive filters.
  // It is intentionally not part of moment equality: two labels with the same
  // exact moments remain equal even when they came from different point tables.
  [[nodiscard]] std::span<const CertifiedPoint3> source_points() const noexcept {
    return source_points_;
  }

  friend bool operator==(
      const ExactLabelMoments& left,
      const ExactLabelMoments& right) noexcept {
    return left.cardinality_ == right.cardinality_ &&
           left.coordinate_sum_ == right.coordinate_sum_ &&
           left.squared_norm_sum_ == right.squared_norm_sum_;
  }

 private:
  void add_unchecked(const CertifiedPoint3& point) {
    if (cardinality_ == std::numeric_limits<std::size_t>::max()) {
      throw std::overflow_error("an exact label cardinality cannot overflow");
    }
    ++cardinality_;
    source_points_.push_back(point);
    ExactRational squared_norm;
    for (std::size_t axis = 0; axis < coordinate_sum_.size(); ++axis) {
      const ExactRational coordinate = point.coordinate(axis);
      coordinate_sum_[axis] = coordinate_sum_[axis] + coordinate;
      squared_norm = squared_norm + coordinate * coordinate;
    }
    squared_norm_sum_ = squared_norm_sum_ + squared_norm;
  }
  std::size_t cardinality_{0};
  std::array<ExactRational, 3> coordinate_sum_{};
  ExactRational squared_norm_sum_{};
  std::vector<CertifiedPoint3> source_points_{};
};

}  // namespace morsehgp3d::exact
