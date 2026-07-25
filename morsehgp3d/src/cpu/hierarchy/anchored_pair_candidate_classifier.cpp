#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::PointId;

struct PreparedPhiIntervalAnchor {
  std::array<exact::detail::Binary64Interval, 3> midpoint{};
  std::array<exact::detail::Binary64Interval, 3> half_difference{};
  std::array<exact::detail::Binary64Interval, 3>
      squared_half_difference{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] ExactDyadicAabb3 point_bounds(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

[[nodiscard]] double binary64_value(std::uint64_t bits) {
  return std::bit_cast<double>(
      exact::canonicalize_binary64_bits(bits));
}

[[nodiscard]] PreparedPhiIntervalAnchor prepare_interval_anchor(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 2>& support_ids) {
  const std::array<std::uint64_t, 3> first_words =
      cloud.point(support_ids[0]).canonical_input_bits();
  const std::array<std::uint64_t, 3> second_words =
      cloud.point(support_ids[1]).canonical_input_bits();
  PreparedPhiIntervalAnchor anchor;
  const exact::detail::Binary64Interval half =
      exact::detail::point_binary64_interval(0.5);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::detail::Binary64Interval first =
        exact::detail::point_binary64_interval(
            binary64_value(first_words[axis]));
    const exact::detail::Binary64Interval second =
        exact::detail::point_binary64_interval(
            binary64_value(second_words[axis]));
    anchor.midpoint[axis] = exact::detail::multiply_binary64_intervals(
        exact::detail::add_binary64_intervals(first, second), half);
    anchor.half_difference[axis] =
        exact::detail::multiply_binary64_intervals(
            exact::detail::subtract_binary64_intervals(first, second),
            half);
    anchor.squared_half_difference[axis] =
        exact::detail::square_binary64_interval(
            anchor.half_difference[axis]);
  }
  return anchor;
}

// Centered outward interval extension of
//
//   phi(q) = sum_i ((q_i-m_i)^2-d_i^2),
//   m_i = (a_i+b_i)/2, d_i = (a_i-b_i)/2.
//
// The outward midpoint and half-difference intervals contain their exact
// dyadic values.  Subsequent outward subtraction, square and sum therefore
// contain phi over the whole AABB.  Strict exclusion of zero is authoritative;
// every interval containing zero remains deliberately inconclusive.
[[nodiscard]] std::optional<int> filtered_phi_aabb_sign(
    const PreparedPhiIntervalAnchor& anchor,
    const ExactDyadicAabb3& query_bounds) {
  exact::detail::Binary64Interval sum =
      exact::detail::point_binary64_interval(0.0);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double lower =
        binary64_value(query_bounds.lower_binary64_bits[axis]);
    const double upper =
        binary64_value(query_bounds.upper_binary64_bits[axis]);
    if (lower > upper) {
      throw std::logic_error(
          "an anchored pair interval filter received reversed bounds");
    }
    const exact::detail::Binary64Interval query{lower, upper, true};
    const exact::detail::Binary64Interval centered_query =
        exact::detail::subtract_binary64_intervals(
            query, anchor.midpoint[axis]);
    sum = exact::detail::add_binary64_intervals(
        sum,
        exact::detail::subtract_binary64_intervals(
            exact::detail::square_binary64_interval(centered_query),
            anchor.squared_half_difference[axis]));
  }
  const exact::FilterResult sign =
      exact::detail::sign_of_binary64_interval(sum);
  if (!sign.sign().has_value()) {
    return std::nullopt;
  }
  return *sign.sign() == exact::PredicateSign::positive ? 1 : -1;
}

[[nodiscard]] bool filter_partition_closes(
    const ExactAnchoredPairCandidateClassificationAudit& audit) {
  return audit.node_visit_count ==
      checked_add(
          checked_add(
              audit.interval_exterior_node_count,
              audit.interval_interior_node_count,
              "the anchored pair interval partition overflows size_t"),
          audit.exact_node_fallback_count,
          "the anchored pair interval partition overflows size_t");
}

void require_filter_partition(
    const ExactAnchoredPairCandidateClassificationAudit& audit) {
  if (!filter_partition_closes(audit)) {
    throw std::logic_error(
        "the anchored pair interval audit does not partition node visits");
  }
}

}  // namespace

ExactAnchoredPairCandidateClassificationResult
ExactAnchoredPairCandidateClassifier::classify(
    const spatial::MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::array<PointId, 2> support_ids,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairCandidateClassificationBudget budget) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "an anchored pair classifier requires its cloud's exact LBVH");
  }
  static_cast<void>(cloud.point(support_ids[0]));
  static_cast<void>(cloud.point(support_ids[1]));
  if (support_ids[0] == support_ids[1]) {
    throw std::invalid_argument(
        "an anchored pair classifier requires two distinct points");
  }
  if (support_ids[1] < support_ids[0]) {
    std::swap(support_ids[0], support_ids[1]);
  }
  if (maximum_closed_rank < 2U ||
      maximum_closed_rank >
          exact_anchored_pair_candidate_maximum_closed_rank) {
    throw std::out_of_range(
        "an anchored pair maximum closed rank must be in [2, 11]");
  }

  ExactAnchoredPairCandidateClassificationResult result;
  result.support_ids = support_ids;
  result.maximum_closed_rank = maximum_closed_rank;
  result.requested_budget = budget;

  const std::size_t interior_cap = maximum_closed_rank - 2U;
  const ExactDyadicAabb3 first_support_bounds =
      point_bounds(cloud, support_ids[0]);
  const ExactDyadicAabb3 second_support_bounds =
      point_bounds(cloud, support_ids[1]);
  exact::detail::Fp64EnvironmentGuard filter_environment;
  result.audit.fp64_interval_filter_enabled =
      filter_environment.supported();
  const PreparedPhiIntervalAnchor interval_anchor =
      prepare_interval_anchor(cloud, support_ids);

  std::vector<std::size_t> frontier;
  frontier.push_back(index.root_index_);
  result.audit.maximum_frontier_entry_count = 1U;
  std::vector<PointId> interior_ids;
  interior_ids.reserve(std::min(interior_cap, cloud.size()));
  std::size_t shell_count = 0U;
  std::optional<PointId> canonical_extra_shell_witness_id;
  std::size_t exterior_count = 0U;
  std::uint8_t support_seen_mask = 0U;

  const auto node_bounds = [&](std::size_t node_index) {
    const auto& node = index.nodes_[node_index];
    ExactDyadicAabb3 bounds{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      bounds.lower_binary64_bits[axis] =
          cloud.point(node.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      bounds.upper_binary64_bits[axis] =
          cloud.point(node.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    return bounds;
  };

  while (!frontier.empty()) {
    if (result.audit.node_visit_count >=
        budget.maximum_node_visit_count) {
      result.status =
          ExactAnchoredPairCandidateClassificationStatus::budget_exhausted;
      result.stop_reason =
          ExactAnchoredPairCandidateClassificationStopReason::node_visit_limit;
      require_filter_partition(result.audit);
      return result;
    }
    result.audit.node_visit_count = checked_add(
        result.audit.node_visit_count,
        1U,
        "the anchored pair node-visit count overflows size_t");

    const std::size_t node_index = frontier.back();
    frontier.pop_back();
    if (node_index >= index.nodes_.size()) {
      throw std::logic_error(
          "an anchored pair traversal reached an invalid LBVH node");
    }
    const auto& node = index.nodes_[node_index];
    const std::size_t subtree_size = node.leaf_end - node.leaf_begin;
    const ExactDyadicAabb3 query_bounds = node_bounds(node_index);

    std::optional<int> interval_sign;
    if (result.audit.fp64_interval_filter_enabled) {
      interval_sign = filtered_phi_aabb_sign(
          interval_anchor, query_bounds);
    }

    bool certified_exterior = false;
    bool certified_interior = false;
    int exact_minimum_phi_sign = 0;
    int exact_maximum_phi_sign = 0;
    if (interval_sign.has_value() && *interval_sign > 0) {
      result.audit.interval_exterior_node_count = checked_add(
          result.audit.interval_exterior_node_count,
          1U,
          "the anchored pair interval-exterior count overflows size_t");
      certified_exterior = true;
    } else if (interval_sign.has_value() && *interval_sign < 0) {
      result.audit.interval_interior_node_count = checked_add(
          result.audit.interval_interior_node_count,
          1U,
          "the anchored pair interval-interior count overflows size_t");
      certified_interior = true;
    } else {
      result.audit.exact_node_fallback_count = checked_add(
          result.audit.exact_node_fallback_count,
          1U,
          "the anchored pair exact-fallback count overflows size_t");
      exact_minimum_phi_sign =
          exact_diametral_anchor_phi_aabb_minimum_sign(
              first_support_bounds,
              second_support_bounds,
              query_bounds);
      result.audit.exact_minimum_phi_aabb_bound_count = checked_add(
          result.audit.exact_minimum_phi_aabb_bound_count,
          1U,
          "the anchored pair minimum-bound count overflows size_t");
      if (exact_minimum_phi_sign > 0) {
        certified_exterior = true;
      } else {
        exact_maximum_phi_sign =
            exact_diametral_phi_aabb_maximum_sign(
                first_support_bounds,
                second_support_bounds,
                query_bounds);
        result.audit.exact_maximum_phi_aabb_bound_count = checked_add(
            result.audit.exact_maximum_phi_aabb_bound_count,
            1U,
            "the anchored pair maximum-bound count overflows size_t");
        certified_interior = exact_maximum_phi_sign < 0;
      }
    }

    if (certified_exterior) {
      exterior_count = checked_add(
          exterior_count,
          subtree_size,
          "the anchored pair exterior count overflows size_t");
      result.audit.bulk_exterior_subtree_count = checked_add(
          result.audit.bulk_exterior_subtree_count,
          1U,
          "the anchored pair exterior-subtree count overflows size_t");
      result.audit.bulk_exterior_point_count = checked_add(
          result.audit.bulk_exterior_point_count,
          subtree_size,
          "the anchored pair exterior-point count overflows size_t");
      result.audit.classified_point_count = checked_add(
          result.audit.classified_point_count,
          subtree_size,
          "the anchored pair classified-point count overflows size_t");
      continue;
    }
    if (certified_interior) {
      result.audit.bulk_interior_subtree_count = checked_add(
          result.audit.bulk_interior_subtree_count,
          1U,
          "the anchored pair interior-subtree count overflows size_t");
      result.audit.bulk_interior_point_count = checked_add(
          result.audit.bulk_interior_point_count,
          subtree_size,
          "the anchored pair interior-point count overflows size_t");
      result.audit.classified_point_count = checked_add(
          result.audit.classified_point_count,
          subtree_size,
          "the anchored pair classified-point count overflows size_t");
      if (interior_ids.size() > interior_cap) {
        throw std::logic_error(
            "an anchored pair interior cursor exceeds its rank cap");
      }
      const std::size_t remaining_interior_capacity =
          interior_cap - interior_ids.size();
      if (subtree_size > remaining_interior_capacity) {
        result.status =
            ExactAnchoredPairCandidateClassificationStatus::above_rank;
        result.audit.early_above_rank_certificate = true;
        require_filter_partition(result.audit);
        return result;
      }
      for (std::size_t position = node.leaf_begin;
           position < node.leaf_end;
           ++position) {
        interior_ids.push_back(index.leaves_[position].point_id);
      }
      continue;
    }

    if (node.is_leaf()) {
      result.audit.leaf_visit_count = checked_add(
          result.audit.leaf_visit_count,
          1U,
          "the anchored pair leaf-visit count overflows size_t");
      result.audit.exact_point_classification_count = checked_add(
          result.audit.exact_point_classification_count,
          1U,
          "the anchored pair exact-point count overflows size_t");
      result.audit.classified_point_count = checked_add(
          result.audit.classified_point_count,
          1U,
          "the anchored pair classified-point count overflows size_t");
      if (exact_minimum_phi_sign != exact_maximum_phi_sign) {
        throw std::logic_error(
            "a degenerate exact LBVH box has inconsistent phi extrema");
      }
      const PointId point_id =
          index.leaves_[node.leaf_begin].point_id;
      if (exact_maximum_phi_sign < 0) {
        if (interior_ids.size() == interior_cap) {
          result.status =
              ExactAnchoredPairCandidateClassificationStatus::above_rank;
          result.audit.early_above_rank_certificate = true;
          require_filter_partition(result.audit);
          return result;
        }
        interior_ids.push_back(point_id);
      } else if (exact_maximum_phi_sign == 0) {
        shell_count = checked_add(
            shell_count,
            1U,
            "the anchored pair shell count overflows size_t");
        if (point_id == support_ids[0]) {
          support_seen_mask =
              static_cast<std::uint8_t>(support_seen_mask | 1U);
        } else if (point_id == support_ids[1]) {
          support_seen_mask =
              static_cast<std::uint8_t>(support_seen_mask | 2U);
        } else if (!canonical_extra_shell_witness_id.has_value() ||
                   point_id < *canonical_extra_shell_witness_id) {
          canonical_extra_shell_witness_id = point_id;
        }
      } else {
        exterior_count = checked_add(
            exterior_count,
            1U,
            "the anchored pair exterior count overflows size_t");
      }
      continue;
    }

    result.audit.internal_node_expansion_count = checked_add(
        result.audit.internal_node_expansion_count,
        1U,
        "the anchored pair expansion count overflows size_t");
    frontier.push_back(node.right_child);
    frontier.push_back(node.left_child);
    result.audit.maximum_frontier_entry_count = std::max(
        result.audit.maximum_frontier_entry_count,
        frontier.size());
  }

  const std::size_t classified_count = checked_add(
      checked_add(
          interior_ids.size(),
          shell_count,
          "the anchored pair partition count overflows size_t"),
      exterior_count,
      "the anchored pair partition count overflows size_t");
  if (classified_count != cloud.size() || support_seen_mask != 3U ||
      shell_count < 2U ||
      (shell_count == 2U) !=
          !canonical_extra_shell_witness_id.has_value()) {
    throw std::logic_error(
        "an anchored pair traversal did not close its exact partition");
  }
  if (result.audit.classified_point_count != cloud.size()) {
    throw std::logic_error(
        "an anchored pair classification audit did not close");
  }
  require_filter_partition(result.audit);

  std::sort(interior_ids.begin(), interior_ids.end());
  const exact::CircumcenterResult sphere = exact::circumcenter(
      cloud.point(support_ids[0]), cloud.point(support_ids[1]));
  if (sphere.kind() != exact::CircumcenterKind::unique ||
      !sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "two canonical distinct points did not define a unique sphere");
  }
  result.audit.center_and_level_constructed = true;

  const std::size_t observed_closed_rank = checked_add(
      interior_ids.size(),
      shell_count,
      "the anchored pair observed rank overflows size_t");
  const std::size_t minimum_possible_closed_rank = checked_add(
      interior_ids.size(),
      2U,
      "the anchored pair minimum rank overflows size_t");
  if (minimum_possible_closed_rank > maximum_closed_rank) {
    throw std::logic_error(
        "an anchored pair escaped its exact interior-rank cap");
  }

  if (shell_count == 2U) {
    result.event = ExactPairSupportEvent{
        support_ids,
        *sphere.center(),
        *sphere.squared_level(),
        std::move(interior_ids),
        observed_closed_rank,
        exterior_count};
  } else {
    if (!canonical_extra_shell_witness_id.has_value()) {
      throw std::logic_error(
          "an anchored extra shell omitted its canonical witness");
    }
    result.relevant_extra_shell_diagnostic =
        ExactPairSupportExtraShellDiagnostic{
            support_ids,
            *sphere.center(),
            *sphere.squared_level(),
            std::move(interior_ids),
            shell_count,
            *canonical_extra_shell_witness_id,
            minimum_possible_closed_rank,
            observed_closed_rank,
            exterior_count};
  }
  result.status =
      ExactAnchoredPairCandidateClassificationStatus::complete;
  result.stop_reason =
      ExactAnchoredPairCandidateClassificationStopReason::none;
  result.audit.complete_partition_certified = true;
  return result;
}

ExactAnchoredPairCandidateClassificationResult
classify_exact_anchored_pair_candidate(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::array<spatial::PointId, 2> support_ids,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairCandidateClassificationBudget budget) {
  return ExactAnchoredPairCandidateClassifier::classify(
      index,
      cloud,
      support_ids,
      maximum_closed_rank,
      budget);
}

}  // namespace morsehgp3d::hierarchy
