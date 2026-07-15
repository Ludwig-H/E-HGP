#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>

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

  friend bool operator==(
      const ExactLabelMoments&, const ExactLabelMoments&) noexcept = default;

 private:
  void add_unchecked(const CertifiedPoint3& point) {
    if (cardinality_ == std::numeric_limits<std::size_t>::max()) {
      throw std::overflow_error("an exact label cardinality cannot overflow");
    }
    ++cardinality_;
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
};

}  // namespace morsehgp3d::exact
