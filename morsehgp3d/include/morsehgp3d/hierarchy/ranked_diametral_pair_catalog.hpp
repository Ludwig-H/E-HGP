#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <numeric>
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

// Number of abstract Gabriel subsets of target_cardinality carried by one
// exact ball record and one fixed minimal support.  A carried subset must
// contain the complete minimal support and every strict-interior point; any
// extra-shell point is optional.  Callers that aggregate multiple minimal
// supports for the same ball must union and deduplicate their PointId subsets.
// Hence the count is
//
//   choose(extra_shell_count,
//          target_cardinality - support_size - strict_interior_count).
//
// This is an abstract-simplex count.  Affine independence is a separate,
// optional filter for geometric triangle/tetrahedron catalogues and must not
// remove an abstract Gabriel simplex from the HGP stream.
[[nodiscard]] inline std::size_t exact_ball_supported_gabriel_subset_count(
    std::size_t support_size,
    std::size_t strict_interior_count,
    std::size_t extra_shell_count,
    std::size_t target_cardinality) {
  if (support_size == 0U) {
    throw std::invalid_argument(
        "a Gabriel ball record requires a nonempty support");
  }
  if (strict_interior_count >
      std::numeric_limits<std::size_t>::max() - support_size) {
    throw std::overflow_error(
        "the mandatory Gabriel subset cardinality overflows size_t");
  }
  const std::size_t mandatory_cardinality =
      support_size + strict_interior_count;
  if (target_cardinality < mandatory_cardinality) {
    return 0U;
  }
  const std::size_t selected_shell_count =
      target_cardinality - mandatory_cardinality;
  if (selected_shell_count > extra_shell_count) {
    return 0U;
  }

  const std::size_t reduced_shell_count =
      std::min(selected_shell_count,
               extra_shell_count - selected_shell_count);
  std::size_t result = 1U;
  for (std::size_t factor = 1U;
       factor <= reduced_shell_count;
       ++factor) {
    std::size_t numerator =
        extra_shell_count - reduced_shell_count + factor;
    std::size_t denominator = factor;
    const std::size_t numerator_divisor =
        std::gcd(numerator, denominator);
    numerator /= numerator_divisor;
    denominator /= numerator_divisor;
    const std::size_t result_divisor = std::gcd(result, denominator);
    result /= result_divisor;
    denominator /= result_divisor;
    if (denominator != 1U) {
      throw std::logic_error(
          "the Gabriel subset binomial recurrence did not divide exactly");
    }
    if (numerator != 0U &&
        result > std::numeric_limits<std::size_t>::max() / numerator) {
      throw std::overflow_error(
          "the Gabriel subset count overflows size_t");
    }
    result *= numerator;
  }
  return result;
}

[[nodiscard]] inline std::size_t
exact_ranked_diametral_pair_gabriel_subset_count(
    const ExactRankedDiametralPairRecord& record,
    std::size_t target_cardinality) {
  if (record.strict_interior_ids.size() >
      std::numeric_limits<std::size_t>::max() - 2U ||
      record.extra_shell_ids.size() >
          std::numeric_limits<std::size_t>::max() - 2U -
              record.strict_interior_ids.size() ||
      record.closed_rank !=
          2U + record.strict_interior_ids.size() +
              record.extra_shell_ids.size()) {
    throw std::logic_error(
        "a ranked diametral pair record has inconsistent cardinalities");
  }
  return exact_ball_supported_gabriel_subset_count(
      2U,
      record.strict_interior_ids.size(),
      record.extra_shell_ids.size(),
      target_cardinality);
}

// Candidate output sizes for a structurally validated support-2 record.  Every
// candidate keeps the record's diametral miniball, exact level, closed rank,
// and saturation.  These two helpers deliberately count all same-miniball
// candidates, not Gabriel subsets: Gabriel membership additionally requires
// every strict-interior point to belong to the subset.  The GPU implementation
// can reserve both candidate streams with one count and one exclusive scan,
// without another diametral query.
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
