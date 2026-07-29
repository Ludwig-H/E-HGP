#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::hierarchy {

// Canonical scientific payload for one admitted diametral pair.  The complete
// closed ball is support_ids union strict_interior_ids union extra_shell_ids.
// An exact-bucket producer emits this record only when both variable lists are
// complete and their union has the selected closed rank.  Backend, deployment,
// and public-status metadata belong to the producer result, not this payload.
struct ExactRankedDiametralPairRecord {
  std::array<spatial::PointId, 2> support_ids{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> strict_interior_ids;
  // Shell ids other than the two support endpoints.
  std::vector<spatial::PointId> extra_shell_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactRankedDiametralPairRecord&,
      const ExactRankedDiametralPairRecord&) = default;
};

// Candidate output sizes for a structurally validated support-2 record.  Every
// subset keeps the record's diametral miniball, exact level, closed rank, and
// saturation, but it is a geometric simplex only after an exact affine-
// independence filter.  The GPU implementation can therefore reserve both
// candidate streams with one count and one exclusive scan, without another
// diametral query.
[[nodiscard]] inline std::size_t exact_pair_supported_triplet_candidate_count(
    const ExactRankedDiametralPairRecord& record) {
  if (record.closed_rank < 2U) {
    throw std::logic_error(
        "a ranked diametral pair cannot have closed rank below two");
  }
  return record.closed_rank - 2U;
}

[[nodiscard]] inline std::size_t
exact_pair_supported_quadruplet_candidate_count(
    const ExactRankedDiametralPairRecord& record) {
  const std::size_t non_support_count =
      exact_pair_supported_triplet_candidate_count(record);
  if (non_support_count < 2U) {
    return 0U;
  }
  std::size_t left = non_support_count;
  std::size_t right = non_support_count - 1U;
  if ((left & std::size_t{1}) == 0U) {
    left /= 2U;
  } else {
    right /= 2U;
  }
  if (right != 0U &&
      left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(
        "the pair-supported quadruplet candidate count overflows size_t");
  }
  return left * right;
}

[[nodiscard]] inline bool
exact_ranked_diametral_pair_record_well_formed(
    const ExactRankedDiametralPairRecord& record,
    std::size_t point_count,
    std::size_t maximum_closed_rank) noexcept {
  const auto sorted_unique = [](const std::vector<spatial::PointId>& ids) {
    return std::is_sorted(ids.begin(), ids.end()) &&
        std::adjacent_find(ids.begin(), ids.end()) == ids.end();
  };
  if (record.support_ids[1] <= record.support_ids[0] ||
      record.support_ids[1] >= point_count ||
      !sorted_unique(record.strict_interior_ids) ||
      !sorted_unique(record.extra_shell_ids) ||
      record.squared_level.numerator() <= 0 ||
      record.closed_rank < 2U ||
      record.closed_rank > maximum_closed_rank ||
      record.closed_rank > point_count ||
      record.exterior_count != point_count - record.closed_rank) {
    return false;
  }
  const std::size_t expected_non_support_count = record.closed_rank - 2U;
  if (record.strict_interior_ids.size() > expected_non_support_count ||
      record.extra_shell_ids.size() !=
          expected_non_support_count - record.strict_interior_ids.size()) {
    return false;
  }
  const auto contains_support = [&record](spatial::PointId point_id) {
    return point_id == record.support_ids[0] ||
        point_id == record.support_ids[1];
  };
  const auto outside_cloud = [point_count](spatial::PointId point_id) {
    return point_id >= point_count;
  };
  if (std::any_of(
          record.strict_interior_ids.begin(),
          record.strict_interior_ids.end(),
          contains_support) ||
      std::any_of(
          record.extra_shell_ids.begin(),
          record.extra_shell_ids.end(),
          contains_support) ||
      std::any_of(
          record.strict_interior_ids.begin(),
          record.strict_interior_ids.end(),
          outside_cloud) ||
      std::any_of(
          record.extra_shell_ids.begin(),
          record.extra_shell_ids.end(),
          outside_cloud)) {
    return false;
  }
  return std::none_of(
      record.strict_interior_ids.begin(),
      record.strict_interior_ids.end(),
      [&record](spatial::PointId point_id) {
        return std::binary_search(
            record.extra_shell_ids.begin(),
            record.extra_shell_ids.end(),
            point_id);
      });
}

[[nodiscard]] inline std::vector<spatial::PointId>
exact_ranked_diametral_pair_closed_point_ids(
    const ExactRankedDiametralPairRecord& record) {
  std::vector<spatial::PointId> result;
  result.reserve(record.closed_rank);
  result.insert(
      result.end(), record.support_ids.begin(), record.support_ids.end());
  result.insert(
      result.end(),
      record.strict_interior_ids.begin(),
      record.strict_interior_ids.end());
  result.insert(
      result.end(),
      record.extra_shell_ids.begin(),
      record.extra_shell_ids.end());
  std::sort(result.begin(), result.end());
  if (result.size() != record.closed_rank ||
      std::adjacent_find(result.begin(), result.end()) != result.end()) {
    throw std::logic_error(
        "a ranked diametral pair record has an invalid closed-point union");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
