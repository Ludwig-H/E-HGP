#include "morsehgp3d/hierarchy/yao48_ranked_pair_candidates.hpp"

#include "morsehgp3d/exact/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

struct DirectedNeighbor {
  spatial::PointId point_id{};
  exact::ExactLevel squared_distance{};
};

struct UnorderedPair {
  std::array<spatial::PointId, 2> support_ids{};
  std::array<std::size_t, 2> directed_cone_indices{};
  exact::ExactLevel squared_distance{};
};

using NeighborCones =
    std::array<std::vector<DirectedNeighbor>, exact_yao48_cone_count>;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_pair_count(std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  std::size_t left = point_count;
  std::size_t right = point_count - 1U;
  if ((left & std::size_t{1}) == 0U) {
    left /= 2U;
  } else {
    right /= 2U;
  }
  return checked_product(
      left,
      right,
      "the Yao48 ranked-pair unordered-pair count overflows size_t");
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t point_index) {
  if (point_index > static_cast<std::size_t>(
                        spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error(
        "a Yao48 ranked-pair point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(point_index);
}

[[nodiscard]] bool neighbor_less(
    const DirectedNeighbor& left,
    const DirectedNeighbor& right) {
  if (left.squared_distance != right.squared_distance) {
    return left.squared_distance < right.squared_distance;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] exact::ExactLevel triple_level(
    const exact::ExactLevel& level) {
  return exact::ExactLevel{
      exact::BigInt{3} * level.numerator(), level.denominator()};
}

[[nodiscard]] exact::ExactLevel squared_level(
    const exact::ExactRational& value) {
  return exact::ExactLevel{value * value};
}

[[nodiscard]] bool candidate_less(
    const ExactYao48RankedPairCandidate& left,
    const ExactYao48RankedPairCandidate& right) noexcept {
  if (left.support_ids != right.support_ids) {
    return left.support_ids < right.support_ids;
  }
  return left.directed_cone_indices < right.directed_cone_indices;
}

}  // namespace

bool ExactYao48RankedPairCandidateResult::locally_structurally_valid() const {
  if (schema_version !=
          exact_yao48_ranked_pair_candidate_schema_version ||
      point_count < 2U ||
      point_count >
          exact_yao48_ranked_pair_candidate_reference_max_point_count ||
      requested_order == 0U ||
      requested_order >
          exact_yao48_ranked_pair_candidate_maximum_order ||
      requested_order >= point_count || counters.point_count != point_count) {
    return false;
  }

  std::size_t pair_count = 0U;
  std::size_t point_cone_count = 0U;
  try {
    pair_count = checked_pair_count(point_count);
    point_cone_count = checked_product(
        point_count,
        exact_yao48_cone_count,
        "the local Yao48 point-cone count overflows size_t");
  } catch (const std::exception&) {
    return false;
  }
  std::size_t cutoff_partition_count = 0U;
  std::size_t pair_partition_count = 0U;
  std::size_t prune_kind_partition_count = 0U;
  try {
    cutoff_partition_count = checked_add(
        counters.finite_cutoff_count,
        counters.underfull_cone_count,
        "the local Yao48 cutoff partition overflows size_t");
    pair_partition_count = checked_add(
        counters.candidate_pair_count,
        counters.cutoff_pruned_pair_count,
        "the local Yao48 pair partition overflows size_t");
    prune_kind_partition_count = checked_add(
        counters.uniform_radial_pruned_pair_count,
        counters.directional_refinement_pruned_pair_count,
        "the local Yao48 prune-kind partition overflows size_t");
  } catch (const std::exception&) {
    return false;
  }
  if (counters.unordered_pair_count != pair_count ||
      counters.exact_squared_distance_evaluation_count != pair_count ||
      counters.directed_cone_classification_count != 2U * pair_count ||
      counters.point_cone_count != point_cone_count ||
      cutoff_receipts.size() != point_cone_count ||
      cutoff_partition_count != point_cone_count ||
      counters.candidate_pair_count != candidates.size() ||
      pair_partition_count != pair_count ||
      prune_kind_partition_count != counters.cutoff_pruned_pair_count ||
      !std::is_sorted(candidates.begin(), candidates.end(), candidate_less)) {
    return false;
  }

  std::size_t retained_reference_count = 0U;
  for (std::size_t receipt_index = 0U;
       receipt_index < cutoff_receipts.size();
       ++receipt_index) {
    const ExactYao48RankCutoffReceipt& receipt =
        cutoff_receipts[receipt_index];
    const std::size_t expected_anchor =
        receipt_index / exact_yao48_cone_count;
    const std::size_t expected_cone =
        receipt_index % exact_yao48_cone_count;
    const std::size_t expected_retained =
        std::min(requested_order, receipt.cone_point_count);
    if (receipt.anchor_point_id != checked_point_id(expected_anchor) ||
        receipt.cone_index != expected_cone ||
        receipt.retained_nearest_point_count != expected_retained ||
        receipt.retained_nearest_point_count >
            receipt.nearest_point_ids.size() ||
        receipt.kth_squared_distance.has_value() !=
            (receipt.cone_point_count >= requested_order)) {
      return false;
    }
    for (std::size_t index = 0U;
         index < receipt.retained_nearest_point_count;
         ++index) {
      if (receipt.nearest_point_ids[index] >= point_count ||
          receipt.nearest_point_ids[index] == receipt.anchor_point_id) {
        return false;
      }
      for (std::size_t earlier = 0U; earlier < index; ++earlier) {
        if (receipt.nearest_point_ids[earlier] ==
            receipt.nearest_point_ids[index]) {
          return false;
        }
      }
    }
    retained_reference_count += receipt.retained_nearest_point_count;
  }
  if (retained_reference_count !=
      counters.retained_witness_reference_count) {
    return false;
  }

  for (std::size_t index = 0U; index < candidates.size(); ++index) {
    const ExactYao48RankedPairCandidate& candidate = candidates[index];
    if (candidate.support_ids[0] >= candidate.support_ids[1] ||
        candidate.support_ids[1] >= point_count ||
        candidate.directed_cone_indices[0] >= exact_yao48_cone_count ||
        candidate.directed_cone_indices[1] >= exact_yao48_cone_count ||
        (index != 0U &&
         candidates[index - 1U].support_ids == candidate.support_ids)) {
      return false;
    }
  }
  return true;
}

bool exact_yao48_directional_rank_cutoff_certifies_above(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& kth_squared_distance) {
  if (!(kth_squared_distance > exact::ExactLevel{})) {
    throw std::invalid_argument(
        "a Yao48 rank cutoff requires a positive Kth squared distance");
  }
  const ExactYao48ConeKey key = classify_exact_yao48_cone(anchor, target);
  std::array<exact::ExactRational, 3> oriented_deltas{};
  for (std::size_t position = 0U; position < 3U; ++position) {
    const std::size_t axis = static_cast<std::size_t>(
        key.axes_by_descending_absolute_delta[position]);
    exact::ExactRational delta =
        target.coordinate(axis) - anchor.coordinate(axis);
    if (delta.sign() < 0) {
      delta = -delta;
    }
    oriented_deltas[position] = std::move(delta);
  }
  const exact::ExactRational first = oriented_deltas[0];
  const exact::ExactRational first_two = first + oriented_deltas[1];
  const exact::ExactRational all_three =
      first_two + oriented_deltas[2];
  return squared_level(first) >= kth_squared_distance &&
      squared_level(first_two) >= exact::ExactLevel{
          exact::BigInt{2} * kth_squared_distance.numerator(),
          kth_squared_distance.denominator()} &&
      squared_level(all_three) >= triple_level(kth_squared_distance);
}

ExactYao48RankedPairCandidateResult
build_exact_yao48_ranked_pair_candidate_reference(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_order) {
  const std::size_t point_count = cloud.size();
  if (point_count < 2U ||
      point_count >
          exact_yao48_ranked_pair_candidate_reference_max_point_count) {
    throw std::length_error(
        "the Yao48 ranked-pair reference requires 2 to 512 points");
  }
  if (requested_order == 0U ||
      requested_order >
          exact_yao48_ranked_pair_candidate_maximum_order ||
      requested_order >= point_count) {
    throw std::out_of_range(
        "the Yao48 ranked-pair reference requires 1<=K<=10 and K<n");
  }

  const std::size_t unordered_pair_count = checked_pair_count(point_count);
  const std::size_t point_cone_count = checked_product(
      point_count,
      exact_yao48_cone_count,
      "the Yao48 ranked-pair point-cone count overflows size_t");
  std::vector<NeighborCones> neighbors_by_anchor(point_count);
  std::vector<UnorderedPair> pairs;
  if (unordered_pair_count > pairs.max_size()) {
    throw std::length_error(
        "the Yao48 ranked-pair universe exceeds vector capacity");
  }
  pairs.reserve(unordered_pair_count);

  ExactYao48RankedPairCandidateResult result;
  result.point_count = point_count;
  result.requested_order = requested_order;
  result.counters.point_count = point_count;
  result.counters.unordered_pair_count = unordered_pair_count;
  result.counters.point_cone_count = point_cone_count;

  for (std::size_t first_index = 0U;
       first_index < point_count;
       ++first_index) {
    const spatial::PointId first = checked_point_id(first_index);
    for (std::size_t second_index = first_index + 1U;
         second_index < point_count;
         ++second_index) {
      const spatial::PointId second = checked_point_id(second_index);
      exact::ExactLevel squared_distance{exact::squared_distance(
          cloud.point(first), cloud.point(second))};
      const std::size_t first_cone =
          classify_exact_yao48_cone(
              cloud.point(first), cloud.point(second))
              .cone_index;
      const std::size_t second_cone =
          classify_exact_yao48_cone(
              cloud.point(second), cloud.point(first))
              .cone_index;
      neighbors_by_anchor[first_index][first_cone].push_back(
          DirectedNeighbor{second, squared_distance});
      neighbors_by_anchor[second_index][second_cone].push_back(
          DirectedNeighbor{first, squared_distance});
      pairs.push_back(UnorderedPair{
          {first, second},
          {first_cone, second_cone},
          std::move(squared_distance)});
      ++result.counters.exact_squared_distance_evaluation_count;
      result.counters.directed_cone_classification_count = checked_add(
          result.counters.directed_cone_classification_count,
          2U,
          "the Yao48 directed cone count overflows size_t");
    }
  }

  if (point_cone_count > result.cutoff_receipts.max_size()) {
    throw std::length_error(
        "the Yao48 cutoff receipt table exceeds vector capacity");
  }
  result.cutoff_receipts.reserve(point_cone_count);
  for (std::size_t anchor_index = 0U;
       anchor_index < point_count;
       ++anchor_index) {
    for (std::size_t cone_index = 0U;
         cone_index < exact_yao48_cone_count;
         ++cone_index) {
      std::vector<DirectedNeighbor>& neighbors =
          neighbors_by_anchor[anchor_index][cone_index];
      std::sort(neighbors.begin(), neighbors.end(), neighbor_less);
      ExactYao48RankCutoffReceipt receipt;
      receipt.anchor_point_id = checked_point_id(anchor_index);
      receipt.cone_index = cone_index;
      receipt.cone_point_count = neighbors.size();
      receipt.retained_nearest_point_count =
          std::min(requested_order, neighbors.size());
      for (std::size_t index = 0U;
           index < receipt.retained_nearest_point_count;
           ++index) {
        receipt.nearest_point_ids[index] = neighbors[index].point_id;
      }
      result.counters.retained_witness_reference_count = checked_add(
          result.counters.retained_witness_reference_count,
          receipt.retained_nearest_point_count,
          "the Yao48 retained witness count overflows size_t");
      if (neighbors.size() >= requested_order) {
        receipt.kth_squared_distance =
            neighbors[requested_order - 1U].squared_distance;
        ++result.counters.finite_cutoff_count;
      } else {
        ++result.counters.underfull_cone_count;
      }
      result.cutoff_receipts.push_back(std::move(receipt));
    }
  }

  if (unordered_pair_count > result.candidates.max_size()) {
    throw std::length_error(
        "the Yao48 candidate superset exceeds vector capacity");
  }
  result.candidates.reserve(unordered_pair_count);
  for (const UnorderedPair& pair : pairs) {
    const std::size_t first_anchor_index =
        static_cast<std::size_t>(pair.support_ids[0]);
    const std::size_t second_anchor_index =
        static_cast<std::size_t>(pair.support_ids[1]);
    const ExactYao48RankCutoffReceipt& first_receipt =
        result.cutoff_receipts[
            first_anchor_index * exact_yao48_cone_count +
            pair.directed_cone_indices[0]];
    const ExactYao48RankCutoffReceipt& second_receipt =
        result.cutoff_receipts[
            second_anchor_index * exact_yao48_cone_count +
            pair.directed_cone_indices[1]];
    const bool pruned_at_first =
        first_receipt.kth_squared_distance.has_value() &&
        exact_yao48_directional_rank_cutoff_certifies_above(
            cloud.point(pair.support_ids[0]),
            cloud.point(pair.support_ids[1]),
            *first_receipt.kth_squared_distance);
    const bool pruned_at_second =
        second_receipt.kth_squared_distance.has_value() &&
        exact_yao48_directional_rank_cutoff_certifies_above(
            cloud.point(pair.support_ids[1]),
            cloud.point(pair.support_ids[0]),
            *second_receipt.kth_squared_distance);
    const bool admitted_at_first = !pruned_at_first;
    const bool admitted_at_second = !pruned_at_second;
    if (admitted_at_first && admitted_at_second) {
      result.candidates.push_back(ExactYao48RankedPairCandidate{
          pair.support_ids, pair.directed_cone_indices});
    } else {
      ++result.counters.cutoff_pruned_pair_count;
      const bool uniform_radial_prune =
          (first_receipt.kth_squared_distance.has_value() &&
           pair.squared_distance >=
               triple_level(*first_receipt.kth_squared_distance)) ||
          (second_receipt.kth_squared_distance.has_value() &&
           pair.squared_distance >=
               triple_level(*second_receipt.kth_squared_distance));
      if (uniform_radial_prune) {
        ++result.counters.uniform_radial_pruned_pair_count;
      } else {
        ++result.counters.directional_refinement_pruned_pair_count;
      }
    }
  }
  result.counters.candidate_pair_count = result.candidates.size();

  if (!result.locally_structurally_valid()) {
    throw std::logic_error(
        "the Yao48 ranked-pair candidate oracle did not close its contract");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
