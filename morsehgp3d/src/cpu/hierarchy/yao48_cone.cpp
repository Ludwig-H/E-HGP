#include "morsehgp3d/hierarchy/yao48_cone.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::array<std::uint8_t, 3>, 6> axis_permutations{{
    {{0U, 1U, 2U}},
    {{0U, 2U, 1U}},
    {{1U, 0U, 2U}},
    {{1U, 2U, 0U}},
    {{2U, 0U, 1U}},
    {{2U, 1U, 0U}},
}};

[[nodiscard]] std::size_t permutation_index(
    const std::array<std::uint8_t, 3>& axes) {
  const auto found =
      std::find(axis_permutations.begin(), axis_permutations.end(), axes);
  if (found == axis_permutations.end()) {
    throw std::logic_error("an exact Yao48 axis order is not a permutation");
  }
  return static_cast<std::size_t>(
      std::distance(axis_permutations.begin(), found));
}

}  // namespace

ExactYao48ConeKey classify_exact_yao48_cone(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target) {
  std::array<exact::ExactRational, 3> absolute_deltas{};
  std::array<std::uint8_t, 3> axes{{0U, 1U, 2U}};
  std::uint8_t negative_axis_mask = 0U;
  bool nonzero = false;
  for (std::size_t axis = 0U; axis < axes.size(); ++axis) {
    exact::ExactRational delta =
        target.coordinate(axis) - source.coordinate(axis);
    const int sign = delta.sign();
    if (sign != 0) {
      nonzero = true;
    }
    if (sign < 0) {
      negative_axis_mask = static_cast<std::uint8_t>(
          negative_axis_mask |
          static_cast<std::uint8_t>(std::uint8_t{1U} << axis));
      delta = -delta;
    }
    absolute_deltas[axis] = std::move(delta);
  }
  if (!nonzero) {
    throw std::invalid_argument(
        "an exact Yao48 cone requires two distinct points");
  }

  std::sort(
      axes.begin(),
      axes.end(),
      [&absolute_deltas](std::uint8_t left, std::uint8_t right) {
        const std::size_t left_axis = static_cast<std::size_t>(left);
        const std::size_t right_axis = static_cast<std::size_t>(right);
        if (absolute_deltas[left_axis] != absolute_deltas[right_axis]) {
          return absolute_deltas[left_axis] > absolute_deltas[right_axis];
        }
        return left < right;
      });
  const std::size_t order_index = permutation_index(axes);
  const std::size_t cone_index =
      static_cast<std::size_t>(negative_axis_mask) *
          axis_permutations.size() +
      order_index;
  if (cone_index >= exact_yao48_cone_count) {
    throw std::logic_error("an exact Yao48 cone index exceeds 47");
  }
  return ExactYao48ConeKey{negative_axis_mask, axes, cone_index};
}

}  // namespace morsehgp3d::hierarchy
