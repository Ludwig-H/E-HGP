#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::MortonLbvhIndex;
using spatial::PointId;

[[nodiscard]] double binary64_value(std::uint64_t bits) {
  return std::bit_cast<double>(exact::canonicalize_binary64_bits(bits));
}

[[nodiscard]] ExactDyadicAabb3 point_bounds(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

[[nodiscard]] ExactDyadicAabb3 build_anchor_bounds(
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids) {
  ExactDyadicAabb3 bounds = point_bounds(cloud, anchor_point_ids.front());
  for (std::size_t anchor_offset = 1U;
       anchor_offset < anchor_point_ids.size();
       ++anchor_offset) {
    const std::array<std::uint64_t, 3> words =
        cloud.point(anchor_point_ids[anchor_offset]).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (exact::binary64_total_order_key(words[axis]) <
          exact::binary64_total_order_key(
              bounds.lower_binary64_bits[axis])) {
        bounds.lower_binary64_bits[axis] = words[axis];
      }
      if (exact::binary64_total_order_key(words[axis]) >
          exact::binary64_total_order_key(
              bounds.upper_binary64_bits[axis])) {
        bounds.upper_binary64_bits[axis] = words[axis];
      }
    }
  }
  return bounds;
}

[[nodiscard]] bool contains_any_actual_anchor(
    const ExactDyadicAabb3& query_bounds,
    std::span<const ExactDyadicAabb3> anchor_point_bounds) noexcept {
  for (const ExactDyadicAabb3& anchor_bounds : anchor_point_bounds) {
    bool contained = true;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const std::uint64_t query_lower_key =
          exact::binary64_total_order_key(
              query_bounds.lower_binary64_bits[axis]);
      const std::uint64_t query_upper_key =
          exact::binary64_total_order_key(
              query_bounds.upper_binary64_bits[axis]);
      const std::uint64_t anchor_key =
          exact::binary64_total_order_key(
              anchor_bounds.lower_binary64_bits[axis]);
      if (anchor_key < query_lower_key || anchor_key > query_upper_key) {
        contained = false;
        break;
      }
    }
    if (contained) {
      return true;
    }
  }
  return false;
}

void require_incrementable(std::size_t value, const char* message) {
  if (value == std::numeric_limits<std::size_t>::max()) {
    throw std::overflow_error(message);
  }
}

void checked_increment(std::size_t& value, const char* message) {
  require_incrementable(value, message);
  ++value;
}

[[nodiscard]] bool half_open_ranges_overlap(
    std::size_t first_begin,
    std::size_t first_end,
    std::size_t second_begin,
    std::size_t second_end) noexcept {
  return first_begin < second_end && second_begin < first_end;
}

// Proposal-only long-double score of the same maximum used by P8g:
//
//   max_a max_{q in Q} (x-a).(x-q).
//
// The query maximum is separable by axis.  This value never authorizes a
// decision; non-finite results are simply ordered after every finite score.
[[nodiscard]] long double floating_grouped_phi_aabb_maximum_score(
    std::span<const ExactDyadicAabb3> anchor_point_bounds,
    const ExactDyadicAabb3& query_bounds,
    const ExactDyadicAabb3& witness_point_bounds) {
  long double maximum = -std::numeric_limits<long double>::infinity();
  for (const ExactDyadicAabb3& anchor_bounds : anchor_point_bounds) {
    long double value = 0.0L;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const long double anchor = static_cast<long double>(
          binary64_value(anchor_bounds.lower_binary64_bits[axis]));
      const long double witness = static_cast<long double>(
          binary64_value(witness_point_bounds.lower_binary64_bits[axis]));
      const long double lower = static_cast<long double>(
          binary64_value(query_bounds.lower_binary64_bits[axis]));
      const long double upper = static_cast<long double>(
          binary64_value(query_bounds.upper_binary64_bits[axis]));
      const long double first_factor = witness - anchor;
      const long double query_endpoint =
          first_factor < 0.0L ? upper : lower;
      value += first_factor * (witness - query_endpoint);
    }
    if (!std::isfinite(value)) {
      return std::numeric_limits<long double>::quiet_NaN();
    }
    maximum = std::max(maximum, value);
  }
  return maximum;
}

// Small outward FP64 filter for the exact P8g expression.  Unlike the
// centered candidate-classifier form, the witness is the query point of phi
// and Q is its second support, so the direct interval product preserves the
// required (x-a).(x-Q) semantics.  A strict interval sign is authoritative;
// an invalid interval or one containing zero deliberately falls back.
[[nodiscard]] std::optional<int> filtered_grouped_phi_aabb_sign(
    const ExactDyadicAabb3& anchor_point_bounds,
    const ExactDyadicAabb3& query_bounds,
    const ExactDyadicAabb3& witness_point_bounds) {
  exact::detail::Binary64Interval sum =
      exact::detail::point_binary64_interval(0.0);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double anchor =
        binary64_value(anchor_point_bounds.lower_binary64_bits[axis]);
    const double witness =
        binary64_value(witness_point_bounds.lower_binary64_bits[axis]);
    const double lower =
        binary64_value(query_bounds.lower_binary64_bits[axis]);
    const double upper =
        binary64_value(query_bounds.upper_binary64_bits[axis]);
    if (lower > upper) {
      throw std::logic_error(
          "a grouped pair interval filter received reversed bounds");
    }
    const exact::detail::Binary64Interval anchor_interval =
        exact::detail::point_binary64_interval(anchor);
    const exact::detail::Binary64Interval witness_interval =
        exact::detail::point_binary64_interval(witness);
    const exact::detail::Binary64Interval query_interval{
        lower, upper, true};
    sum = exact::detail::add_binary64_intervals(
        sum,
        exact::detail::multiply_binary64_intervals(
            exact::detail::subtract_binary64_intervals(
                witness_interval, anchor_interval),
            exact::detail::subtract_binary64_intervals(
                witness_interval, query_interval)));
  }
  const exact::FilterResult sign =
      exact::detail::sign_of_binary64_interval(sum);
  if (!sign.sign().has_value()) {
    return std::nullopt;
  }
  return *sign.sign() == exact::PredicateSign::positive ? 1 : -1;
}

// Proposal-only guide for the exact subtree search.  In one dimension, when
// the anchor interval lies before the query interval, every common diametral
// witness lies between the closest endpoints; their midpoint maximizes the
// symmetric margin.  The product of the three one-dimensional projections is
// only a traversal order.  Every accepted subtree and selected point is still
// decided by the exact predicates below.
[[nodiscard]] std::array<long double, 3> closest_interval_midpoint(
    const ExactDyadicAabb3& anchors,
    const ExactDyadicAabb3& query) {
  std::array<long double, 3> target{};
  for (std::size_t axis = 0U; axis < target.size(); ++axis) {
    const long double anchor_lower = static_cast<long double>(
        std::bit_cast<double>(anchors.lower_binary64_bits[axis]));
    const long double anchor_upper = static_cast<long double>(
        std::bit_cast<double>(anchors.upper_binary64_bits[axis]));
    const long double query_lower = static_cast<long double>(
        std::bit_cast<double>(query.lower_binary64_bits[axis]));
    const long double query_upper = static_cast<long double>(
        std::bit_cast<double>(query.upper_binary64_bits[axis]));
    long double anchor_projection{};
    long double query_projection{};
    if (anchor_upper < query_lower) {
      anchor_projection = anchor_upper;
      query_projection = query_lower;
    } else if (query_upper < anchor_lower) {
      anchor_projection = anchor_lower;
      query_projection = query_upper;
    } else {
      const long double overlap_lower =
          std::max(anchor_lower, query_lower);
      const long double overlap_upper =
          std::min(anchor_upper, query_upper);
      anchor_projection =
          overlap_lower + (overlap_upper - overlap_lower) / 2.0L;
      query_projection = anchor_projection;
    }
    target[axis] = anchor_projection +
        (query_projection - anchor_projection) / 2.0L;
  }
  return target;
}

[[nodiscard]] long double proposal_minimum_squared_distance(
    const std::array<long double, 3>& target,
    const ExactDyadicAabb3& bounds) {
  long double squared_distance = 0.0L;
  for (std::size_t axis = 0U; axis < target.size(); ++axis) {
    const long double lower = static_cast<long double>(
        std::bit_cast<double>(bounds.lower_binary64_bits[axis]));
    const long double upper = static_cast<long double>(
        std::bit_cast<double>(bounds.upper_binary64_bits[axis]));
    long double delta = 0.0L;
    if (target[axis] < lower) {
      delta = lower - target[axis];
    } else if (target[axis] > upper) {
      delta = target[axis] - upper;
    }
    squared_distance += delta * delta;
  }
  return squared_distance;
}

}  // namespace

class ExactGroupedAnchoredPairPruneCertifier {
 public:
  [[nodiscard]] static ExactGroupedAnchoredPairPruneCertificate
  exhausted_result(
      ExactGroupedAnchoredPairPruneCertificate result,
      ExactGroupedAnchoredPairPruneStopReason reason) {
    result.decision_ =
        ExactGroupedAnchoredPairPruneDecision::budget_exhausted;
    result.stop_reason_ = reason;
    result.certified_witness_point_ids_ = {};
    result.certified_witness_count_ = 0U;
    result.certified_witness_pool_mask_ = 0U;
    result.audit_.complete = false;
    return result;
  }

  [[nodiscard]] static ExactDyadicAabb3 node_bounds(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      std::size_t node_index) {
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
  }

  [[nodiscard]] static ExactGroupedAnchoredPairPruneCertificate certify(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      std::span<const PointId> anchor_point_ids,
      std::span<const PointId> witness_pool_point_ids,
      std::size_t lbvh_node_index,
      std::size_t maximum_closed_rank,
      ExactGroupedAnchoredPairPruneBudget budget) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "a grouped anchored-pair certificate requires its cloud's exact LBVH");
    }
    if (lbvh_node_index >= index.nodes_.size()) {
      throw std::out_of_range(
          "a grouped anchored-pair certificate names an invalid LBVH node");
    }
    if (anchor_point_ids.empty() ||
        anchor_point_ids.size() >
            exact_grouped_anchored_pair_maximum_anchor_count) {
      throw std::out_of_range(
          "a grouped anchored-pair certificate requires 1 to 32 anchors");
    }
    if (witness_pool_point_ids.size() >
        exact_grouped_anchored_pair_maximum_witness_pool_size) {
      throw std::out_of_range(
          "a grouped anchored-pair witness pool cannot exceed 64 points");
    }
    if (maximum_closed_rank < 2U ||
        maximum_closed_rank >
            exact_grouped_anchored_pair_maximum_closed_rank) {
      throw std::out_of_range(
          "a grouped anchored-pair maximum closed rank must be in [2, 11]");
    }

    ExactGroupedAnchoredPairPruneCertificate result;
    result.maximum_closed_rank_ = maximum_closed_rank;
    result.required_witness_count_ = maximum_closed_rank - 1U;
    result.requested_budget_ = budget;
    result.audit_.anchor_count = anchor_point_ids.size();
    result.audit_.witness_pool_entry_count = witness_pool_point_ids.size();
    result.audit_.requested_budget_applies = true;
    result.lbvh_node_index_ = lbvh_node_index;
    result.cloud_identity_ = cloud.identity_;
    result.lbvh_identity_ = index.identity_;

    if (anchor_point_ids.size() > budget.maximum_anchor_count) {
      return exhausted_result(
          std::move(result),
          ExactGroupedAnchoredPairPruneStopReason::anchor_count_limit);
    }
    if (witness_pool_point_ids.size() >
        budget.maximum_witness_pool_entry_count) {
      return exhausted_result(
          std::move(result),
          ExactGroupedAnchoredPairPruneStopReason::
              witness_pool_entry_limit);
    }

    result.anchor_count_ = anchor_point_ids.size();
    result.witness_pool_entry_count_ = witness_pool_point_ids.size();
    std::array<ExactDyadicAabb3,
               exact_grouped_anchored_pair_maximum_anchor_count>
        actual_anchor_bounds{};

    for (std::size_t anchor_offset = 0U;
         anchor_offset < anchor_point_ids.size();
         ++anchor_offset) {
      const PointId anchor_point_id = anchor_point_ids[anchor_offset];
      static_cast<void>(cloud.point(anchor_point_id));
      if (anchor_offset != 0U &&
          anchor_point_ids[anchor_offset - 1U] >= anchor_point_id) {
        throw std::invalid_argument(
            "grouped anchored-pair anchors must be strictly increasing");
      }
      result.anchor_point_ids_[anchor_offset] = anchor_point_id;
      actual_anchor_bounds[anchor_offset] =
          point_bounds(cloud, anchor_point_id);
    }

    for (std::size_t witness_offset = 0U;
         witness_offset < witness_pool_point_ids.size();
         ++witness_offset) {
      const PointId witness_point_id =
          witness_pool_point_ids[witness_offset];
      static_cast<void>(cloud.point(witness_point_id));
      if (witness_offset != 0U &&
          witness_pool_point_ids[witness_offset - 1U] >=
              witness_point_id) {
        throw std::invalid_argument(
            "a grouped anchored-pair witness pool must be strictly increasing");
      }
      if (std::binary_search(
              anchor_point_ids.begin(),
              anchor_point_ids.end(),
              witness_point_id)) {
        throw std::invalid_argument(
            "a grouped anchored-pair witness pool contains an anchor");
      }
      result.witness_pool_point_ids_[witness_offset] = witness_point_id;
    }

    result.audit_.input_canonical = true;
    result.anchor_bounds_ = build_anchor_bounds(cloud, anchor_point_ids);
    result.audit_.anchor_bounds_constructed = true;
    const auto& node = index.nodes_[lbvh_node_index];
    result.leaf_begin_ = node.leaf_begin;
    result.leaf_end_ = node.leaf_end;
    result.query_bounds_ = node_bounds(index, cloud, lbvh_node_index);
    result.audit_.lbvh_node_authority_verified = true;

    if (witness_pool_point_ids.size() < result.required_witness_count_) {
      result.decision_ = ExactGroupedAnchoredPairPruneDecision::inconclusive;
      result.stop_reason_ = ExactGroupedAnchoredPairPruneStopReason::none;
      result.audit_.complete = true;
      return result;
    }

    std::array<PointId,
               exact_grouped_anchored_pair_maximum_closed_rank - 1U>
        strict_witnesses{};
    std::size_t strict_witness_count = 0U;
    std::uint64_t strict_witness_mask = 0U;
    for (std::size_t witness_offset = 0U;
         witness_offset < witness_pool_point_ids.size();
         ++witness_offset) {
      const PointId witness_point_id =
          witness_pool_point_ids[witness_offset];
      const ExactDyadicAabb3 witness_bounds =
          point_bounds(cloud, witness_point_id);
      bool strict_for_every_actual_anchor = true;
      for (std::size_t anchor_offset = 0U;
           anchor_offset < anchor_point_ids.size();
           ++anchor_offset) {
        if (result.audit_.exact_predicate_count >=
            budget.maximum_exact_predicate_count) {
          return exhausted_result(
              std::move(result),
              ExactGroupedAnchoredPairPruneStopReason::exact_predicate_limit);
        }
        ++result.audit_.exact_predicate_count;
        const int maximum_sign = exact_diametral_phi_aabb_maximum_sign(
            actual_anchor_bounds[anchor_offset],
            result.query_bounds_,
            witness_bounds);
        if (maximum_sign >= 0) {
          strict_for_every_actual_anchor = false;
          break;
        }
      }
      if (!strict_for_every_actual_anchor) {
        continue;
      }

      strict_witnesses[strict_witness_count] = witness_point_id;
      ++strict_witness_count;
      strict_witness_mask |= std::uint64_t{1} << witness_offset;
      result.audit_.strict_group_witness_count = strict_witness_count;
      if (strict_witness_count == result.required_witness_count_) {
        result.decision_ = ExactGroupedAnchoredPairPruneDecision::certified;
        result.stop_reason_ = ExactGroupedAnchoredPairPruneStopReason::none;
        result.certified_witness_point_ids_ = strict_witnesses;
        result.certified_witness_count_ = strict_witness_count;
        result.certified_witness_pool_mask_ = strict_witness_mask;
        result.audit_.complete = true;
        return result;
      }
    }

    result.decision_ = ExactGroupedAnchoredPairPruneDecision::inconclusive;
    result.stop_reason_ = ExactGroupedAnchoredPairPruneStopReason::none;
    result.audit_.complete = true;
    return result;
  }
};

bool ExactGroupedAnchoredPairPruneCertificate::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  return index.validated_for(cloud) &&
      cloud_identity_.get() == cloud.identity_.get() &&
      lbvh_identity_.get() == index.identity_.get();
}

bool ExactGroupedAnchoredPairPruneCertificate::certifies(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t expected_lbvh_node_index,
    std::size_t expected_maximum_closed_rank,
    std::span<const PointId> expected_anchor_point_ids) const noexcept {
  if (!certified() || !validated_for(index, cloud) ||
      expected_lbvh_node_index != lbvh_node_index_ ||
      expected_maximum_closed_rank != maximum_closed_rank_ ||
      expected_lbvh_node_index >= index.nodes_.size() ||
      expected_anchor_point_ids.size() != anchor_count_) {
    return false;
  }
  for (std::size_t anchor_offset = 0U;
       anchor_offset < anchor_count_;
       ++anchor_offset) {
    if (expected_anchor_point_ids[anchor_offset] !=
        anchor_point_ids_[anchor_offset]) {
      return false;
    }
  }
  const auto& node = index.nodes_[expected_lbvh_node_index];
  return node.leaf_begin == leaf_begin_ &&
      node.leaf_end == leaf_end_ &&
      ExactGroupedAnchoredPairPruneCertifier::node_bounds(
          index, cloud, expected_lbvh_node_index) == query_bounds_;
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::start_at_root(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids,
    std::span<const PointId> witness_pool_point_ids,
    std::size_t maximum_closed_rank) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a grouped traversal requires its cloud's exact LBVH");
  }
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      index.root_index_,
      maximum_closed_rank,
      false,
      false,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids,
    std::span<const PointId> witness_pool_point_ids,
    std::size_t maximum_closed_rank) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a grouped frontier traversal requires its cloud's exact LBVH");
  }
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      index.root_index_,
      maximum_closed_rank,
      true,
      false,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::start_at_node(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids,
    std::span<const PointId> witness_pool_point_ids,
    std::size_t lbvh_node_index,
    std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      false,
      false,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::
    start_floating_witness_order_at_node(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::span<const PointId> anchor_point_ids,
        std::span<const PointId> witness_pool_point_ids,
        std::size_t lbvh_node_index,
        std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      false,
      false,
      true,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::start_frontier_at_node(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids,
    std::span<const PointId> witness_pool_point_ids,
    std::size_t lbvh_node_index,
    std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      true,
      false,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::
    start_floating_witness_order_frontier_at_node(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::span<const PointId> anchor_point_ids,
        std::span<const PointId> witness_pool_point_ids,
        std::size_t lbvh_node_index,
        std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      true,
      false,
      true,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::
    start_witness_subtree_first_at_node(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::span<const PointId> anchor_point_ids,
        std::span<const PointId> witness_pool_point_ids,
        std::size_t lbvh_node_index,
        std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      false,
      true,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext
ExactGroupedAnchoredPairTraversalContext::
    start_witness_subtree_first_frontier_at_node(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::span<const PointId> anchor_point_ids,
        std::span<const PointId> witness_pool_point_ids,
        std::size_t lbvh_node_index,
        std::size_t maximum_closed_rank) {
  return ExactGroupedAnchoredPairTraversalContext(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      true,
      true,
      false,
      PrivateConstructionTag{});
}

ExactGroupedAnchoredPairTraversalContext::
    ExactGroupedAnchoredPairTraversalContext(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::span<const PointId> anchor_point_ids,
        std::span<const PointId> witness_pool_point_ids,
        std::size_t lbvh_node_index,
        std::size_t maximum_closed_rank,
        bool emit_off_diagonal_inconclusive_subtree,
        bool search_witness_subtrees_first,
        bool order_witnesses_by_floating_score,
        PrivateConstructionTag) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a grouped traversal requires its cloud's exact LBVH");
  }
  if (lbvh_node_index >= index.nodes_.size()) {
    throw std::out_of_range(
        "a grouped traversal names an invalid LBVH start node");
  }
  if (anchor_point_ids.empty() ||
      anchor_point_ids.size() >
          exact_grouped_anchored_pair_maximum_anchor_count) {
    throw std::out_of_range(
        "a grouped traversal requires 1 to 32 anchors");
  }
  if (witness_pool_point_ids.size() >
      exact_grouped_anchored_pair_maximum_witness_pool_size) {
    throw std::out_of_range(
        "a grouped traversal witness pool cannot exceed 64 points");
  }
  if (maximum_closed_rank < 2U ||
      maximum_closed_rank >
          exact_grouped_anchored_pair_maximum_closed_rank) {
    throw std::out_of_range(
        "a grouped traversal maximum closed rank must be in [2, 11]");
  }
  if (index.build_counters_.maximum_depth >=
      exact_grouped_anchored_pair_maximum_traversal_stack_entry_count) {
    throw std::length_error(
        "the certified LBVH depth exceeds the fixed grouped traversal stack");
  }

  anchor_count_ = anchor_point_ids.size();
  for (std::size_t anchor_offset = 0U;
       anchor_offset < anchor_point_ids.size();
       ++anchor_offset) {
    const PointId anchor_point_id = anchor_point_ids[anchor_offset];
    static_cast<void>(cloud.point(anchor_point_id));
    if (anchor_offset != 0U &&
        anchor_point_ids[anchor_offset - 1U] >= anchor_point_id) {
      throw std::invalid_argument(
          "grouped traversal anchors must be strictly increasing");
    }
    anchor_point_ids_[anchor_offset] = anchor_point_id;
    anchor_point_bounds_[anchor_offset] =
        point_bounds(cloud, anchor_point_id);
    anchor_leaf_positions_[anchor_offset] =
        index.leaf_position_by_point_id_[anchor_point_id];
  }

  witness_pool_entry_count_ = witness_pool_point_ids.size();
  for (std::size_t witness_offset = 0U;
       witness_offset < witness_pool_point_ids.size();
       ++witness_offset) {
    const PointId witness_point_id =
        witness_pool_point_ids[witness_offset];
    static_cast<void>(cloud.point(witness_point_id));
    if (witness_offset != 0U &&
        witness_pool_point_ids[witness_offset - 1U] >=
            witness_point_id) {
      throw std::invalid_argument(
          "a grouped traversal witness pool must be strictly increasing");
    }
    if (std::binary_search(
            anchor_point_ids.begin(),
            anchor_point_ids.end(),
            witness_point_id)) {
      throw std::invalid_argument(
          "a grouped traversal witness pool contains an anchor");
    }
    witness_pool_point_ids_[witness_offset] = witness_point_id;
    witness_point_bounds_[witness_offset] =
        point_bounds(cloud, witness_point_id);
  }
  halo_witness_pool_point_ids_ = witness_pool_point_ids_;
  halo_witness_point_bounds_ = witness_point_bounds_;
  halo_witness_pool_entry_count_ = witness_pool_entry_count_;

  anchor_bounds_ = build_anchor_bounds(cloud, anchor_point_ids);
  maximum_closed_rank_ = maximum_closed_rank;
  required_witness_count_ = maximum_closed_rank - 1U;
  start_node_index_ = lbvh_node_index;
  cloud_identity_ = cloud.identity_;
  lbvh_identity_ = index.identity_;
  emit_off_diagonal_inconclusive_subtree_ =
      emit_off_diagonal_inconclusive_subtree;
  search_witness_subtrees_first_ = search_witness_subtrees_first;
  order_witnesses_by_floating_score_ =
      order_witnesses_by_floating_score;
  audit_.floating_witness_order_requested =
      order_witnesses_by_floating_score_;
  if (order_witnesses_by_floating_score_) {
    exact::detail::Fp64EnvironmentGuard filter_environment;
    const bool supported = filter_environment.supported();
    audit_.floating_witness_order_enabled = supported;
    audit_.fp64_interval_filter_enabled = supported;
  }
  witness_subtree_preflight_complete_ = true;
  witness_subtree_search_complete_ = true;

  const MortonLbvhIndex::Node& start_node =
      index.nodes_[lbvh_node_index];
  pending_nodes_[0] = NodeAuthority{
      lbvh_node_index,
      start_node.leaf_begin,
      start_node.leaf_end,
      0U};
  pending_node_count_ = 1U;
  audit_.anchor_bounds_construction_count = 1U;
  audit_.prepared_witness_point_count = witness_pool_entry_count_;
  audit_.maximum_pending_node_count = 1U;
}

void ExactGroupedAnchoredPairTraversalContext::
    prepare_floating_witness_order(ActiveNode& active_node) {
  if (!audit_.floating_witness_order_enabled) {
    return;
  }
  const std::span<const ExactDyadicAabb3> anchors{
      anchor_point_bounds_.data(), anchor_count_};
  for (std::size_t canonical_offset = 0U;
       canonical_offset < witness_pool_entry_count_;
       ++canonical_offset) {
    const long double score = floating_grouped_phi_aabb_maximum_score(
        anchors,
        active_node.query_bounds,
        witness_point_bounds_[canonical_offset]);
    active_node.floating_witness_proposals[canonical_offset] =
        FloatingWitnessProposal{canonical_offset, score};
    checked_increment(
        audit_.floating_witness_score_evaluation_count,
        "the floating witness score count overflows size_t");
    if (!std::isfinite(score)) {
      checked_increment(
          audit_.floating_witness_nonfinite_score_count,
          "the non-finite floating witness score count overflows size_t");
    }
  }

  const auto proposal_precedes = [&](
                                     const FloatingWitnessProposal& left,
                                     const FloatingWitnessProposal& right) {
    const bool left_inherited =
        (active_node.authority.inherited_strict_witness_mask &
         (std::uint64_t{1} << left.canonical_offset)) != 0U;
    const bool right_inherited =
        (active_node.authority.inherited_strict_witness_mask &
         (std::uint64_t{1} << right.canonical_offset)) != 0U;
    if (left_inherited != right_inherited) {
      return left_inherited;
    }
    if (left_inherited) {
      return left.canonical_offset < right.canonical_offset;
    }
    const bool left_finite = std::isfinite(left.score);
    const bool right_finite = std::isfinite(right.score);
    if (left_finite != right_finite) {
      return left_finite;
    }
    if (left_finite && left.score != right.score) {
      return left.score < right.score;
    }
    return left.canonical_offset < right.canonical_offset;
  };

  // Fixed-capacity insertion sort: no catalogue, allocation or unstable tie.
  for (std::size_t proposal_offset = 1U;
       proposal_offset < witness_pool_entry_count_;
       ++proposal_offset) {
    const FloatingWitnessProposal value =
        active_node.floating_witness_proposals[proposal_offset];
    std::size_t insertion = proposal_offset;
    while (insertion > 0U && proposal_precedes(
             value,
             active_node.floating_witness_proposals[insertion - 1U])) {
      active_node.floating_witness_proposals[insertion] =
          active_node.floating_witness_proposals[insertion - 1U];
      --insertion;
    }
    active_node.floating_witness_proposals[insertion] = value;
  }
  checked_increment(
      audit_.floating_witness_order_preparation_count,
      "the floating witness order preparation count overflows size_t");
}

bool ExactGroupedAnchoredPairTraversalContext::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  return ready() && index.validated_for(cloud) &&
      cloud_identity_.get() == cloud.identity_.get() &&
      lbvh_identity_.get() == index.identity_.get();
}

ExactGroupedAnchoredPairPruneCertificate
ExactGroupedAnchoredPairTraversalContext::mint_certificate(
    const ActiveNode& active_node) const {
  const std::size_t strict_witness_count =
      static_cast<std::size_t>(
          std::popcount(active_node.strict_witness_mask));
  if (strict_witness_count != required_witness_count_) {
    throw std::logic_error(
        "a grouped traversal cannot mint a noncanonical witness count");
  }

  ExactGroupedAnchoredPairPruneCertificate result;
  result.decision_ = ExactGroupedAnchoredPairPruneDecision::certified;
  result.stop_reason_ = ExactGroupedAnchoredPairPruneStopReason::none;
  result.origin_ =
      ExactGroupedAnchoredPairPruneCertificateOrigin::
          prepared_inherited_traversal;
  result.maximum_closed_rank_ = maximum_closed_rank_;
  result.required_witness_count_ = required_witness_count_;
  result.requested_budget_ = {};
  result.audit_.anchor_count = anchor_count_;
  result.audit_.witness_pool_entry_count = witness_pool_entry_count_;
  result.audit_.exact_predicate_count =
      active_node.node_exact_predicate_count;
  result.audit_.fp64_filtered_negative_predicate_count =
      active_node.node_fp64_filtered_negative_predicate_count;
  result.audit_.fp64_filtered_positive_predicate_count =
      active_node.node_fp64_filtered_positive_predicate_count;
  result.audit_.exact_fallback_predicate_count =
      active_node.node_exact_fallback_predicate_count;
  result.audit_.strict_group_witness_count = required_witness_count_;
  result.audit_.inherited_strict_group_witness_count =
      active_node.inherited_strict_witness_count;
  result.audit_.requested_budget_applies = false;
  result.audit_.input_canonical = true;
  result.audit_.anchor_bounds_constructed = true;
  result.audit_.lbvh_node_authority_verified = true;
  result.audit_.complete = true;
  result.anchor_point_ids_ = anchor_point_ids_;
  result.anchor_count_ = anchor_count_;
  result.witness_pool_point_ids_ = witness_pool_point_ids_;
  result.witness_pool_entry_count_ = witness_pool_entry_count_;
  result.anchor_bounds_ = anchor_bounds_;
  result.lbvh_node_index_ = active_node.authority.node_index;
  result.leaf_begin_ = active_node.authority.leaf_begin;
  result.leaf_end_ = active_node.authority.leaf_end;
  result.query_bounds_ = active_node.query_bounds;
  result.certified_witness_pool_mask_ =
      active_node.strict_witness_mask;
  result.cloud_identity_ = cloud_identity_;
  result.lbvh_identity_ = lbvh_identity_;

  for (std::size_t witness_offset = 0U;
       witness_offset < witness_pool_entry_count_;
       ++witness_offset) {
    const std::uint64_t witness_bit =
        std::uint64_t{1} << witness_offset;
    if ((active_node.strict_witness_mask & witness_bit) == 0U) {
      continue;
    }
    if (result.certified_witness_count_ >= required_witness_count_) {
      throw std::logic_error(
          "a grouped traversal witness mask exceeds its rank contract");
    }
    result.certified_witness_point_ids_[
        result.certified_witness_count_] =
        witness_pool_point_ids_[witness_offset];
    ++result.certified_witness_count_;
  }
  if (result.certified_witness_count_ != required_witness_count_) {
    throw std::logic_error(
        "a grouped traversal lost a certified witness");
  }
  return result;
}

ExactGroupedAnchoredPairTraversalStep
ExactGroupedAnchoredPairTraversalContext::advance(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactGroupedAnchoredPairTraversalWorkBudget budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "a grouped traversal advance requires its authentic cloud and LBVH");
  }
  checked_increment(
      audit_.advance_call_count,
      "the grouped traversal advance-call count overflows size_t");
  ExactGroupedAnchoredPairTraversalStepWork work;
  const auto make_step = [&](
                             ExactGroupedAnchoredPairTraversalStepKind kind,
                             ExactGroupedAnchoredPairTraversalStopReason
                                 stop_reason) {
    ExactGroupedAnchoredPairTraversalStep step;
    step.kind_ = kind;
    step.stop_reason_ = stop_reason;
    step.requested_budget_ = budget;
    step.work_ = work;
    step.advance_call_index_ = audit_.advance_call_count;
    step.traversal_complete_after_step_ = complete_;
    return step;
  };
  const auto budget_exhausted_step = [&](
                                         ExactGroupedAnchoredPairTraversalStopReason
                                             stop_reason) {
    checked_increment(
        audit_.budget_exhaustion_count,
        "the grouped traversal budget-exhaustion count overflows size_t");
    return make_step(
        ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted,
        stop_reason);
  };

  if (complete_) {
    return make_step(
        ExactGroupedAnchoredPairTraversalStepKind::complete,
        ExactGroupedAnchoredPairTraversalStopReason::none);
  }

  // Even when 14U was requested under an unsupported environment and has
  // therefore fallen back to canonical order plus dyadic signs, holding the
  // environment prevents proposal arithmetic from leaking flags.  A context
  // that started with the certified filter fails closed if that contract is
  // lost between resumable advances.
  std::optional<exact::detail::Fp64EnvironmentGuard> filter_environment;
  if (audit_.floating_witness_order_requested) {
    filter_environment.emplace();
    if (audit_.fp64_interval_filter_enabled &&
        !filter_environment->supported()) {
      throw std::runtime_error(
          "the grouped pair interval environment changed during a resumable traversal");
    }
  }

  for (;;) {
    if (!active_node_.has_value()) {
      if (pending_node_count_ == 0U) {
        complete_ = true;
        audit_.complete = true;
        return make_step(
            ExactGroupedAnchoredPairTraversalStepKind::complete,
            ExactGroupedAnchoredPairTraversalStopReason::none);
      }
      if (work.node_visit_count >= budget.maximum_node_visit_count) {
        return budget_exhausted_step(
            ExactGroupedAnchoredPairTraversalStopReason::node_visit_limit);
      }
      require_incrementable(
          audit_.node_visit_count,
          "the grouped traversal node-visit count overflows size_t");
      require_incrementable(
          work.node_visit_count,
          "the grouped traversal step node-visit count overflows size_t");

      const NodeAuthority authority =
          pending_nodes_[pending_node_count_ - 1U];
      if (authority.node_index >= index.nodes_.size()) {
        throw std::logic_error(
            "a grouped traversal pending authority names an invalid node");
      }
      const MortonLbvhIndex::Node& node =
          index.nodes_[authority.node_index];
      if (node.leaf_begin != authority.leaf_begin ||
          node.leaf_end != authority.leaf_end) {
        throw std::logic_error(
            "a grouped traversal pending authority lost its leaf range");
      }
      const ExactDyadicAabb3 query_bounds =
          ExactGroupedAnchoredPairPruneCertifier::node_bounds(
              index, cloud, authority.node_index);
      const std::size_t inherited_count =
          static_cast<std::size_t>(
              std::popcount(authority.inherited_strict_witness_mask));
      if (inherited_count >= required_witness_count_) {
        throw std::logic_error(
            "a grouped traversal expanded an already prunable parent");
      }

      --pending_node_count_;
      // A subtree proposal is local to one query node.  Restore the immutable
      // query-facing halo before binding the next authenticated authority so
      // its inherited mask keeps the same bit meaning as its parent.
      witness_pool_point_ids_ = halo_witness_pool_point_ids_;
      witness_point_bounds_ = halo_witness_point_bounds_;
      witness_pool_entry_count_ = halo_witness_pool_entry_count_;
      pending_witness_subtree_nodes_ = {};
      pending_witness_subtree_node_count_ = 0U;
      active_witness_subtree_node_.reset();
      witness_subtree_receipts_ = {};
      witness_subtree_receipt_count_ = 0U;
      witness_subtree_point_ids_ = {};
      witness_subtree_point_count_ = 0U;
      witness_subtree_node_visit_count_ = 0U;
      witness_subtree_preflight_complete_ =
          !search_witness_subtrees_first_;
      witness_subtree_search_complete_ =
          !search_witness_subtrees_first_;
      if (search_witness_subtrees_first_ &&
          witness_pool_entry_count_ >= required_witness_count_) {
        const MortonLbvhIndex::Node& witness_root =
            index.nodes_[index.root_index_];
        pending_witness_subtree_nodes_[0] = WitnessSubtreeNodeAuthority{
            index.root_index_,
            witness_root.leaf_begin,
            witness_root.leaf_end};
        pending_witness_subtree_node_count_ = 1U;
      }
      active_node_.emplace(ActiveNode{});
      active_node_->authority = authority;
      active_node_->query_bounds = query_bounds;
      prepare_floating_witness_order(*active_node_);
      ++work.node_visit_count;
      ++audit_.node_visit_count;
    }

    ActiveNode& active_node = *active_node_;
    const MortonLbvhIndex::Node& node =
        index.nodes_[active_node.authority.node_index];
    if (node.leaf_begin != active_node.authority.leaf_begin ||
        node.leaf_end != active_node.authority.leaf_end ||
        ExactGroupedAnchoredPairPruneCertifier::node_bounds(
            index, cloud, active_node.authority.node_index) !=
            active_node.query_bounds) {
      throw std::logic_error(
          "a grouped traversal active authority lost its certified node");
    }

    const auto emit_inconclusive_subtree = [&]() {
      require_incrementable(
          audit_.inconclusive_subtree_count,
          "the grouped traversal inconclusive-subtree count overflows size_t");
      ExactGroupedAnchoredPairTraversalStep step = make_step(
          ExactGroupedAnchoredPairTraversalStepKind::inconclusive_subtree,
          ExactGroupedAnchoredPairTraversalStopReason::none);
      step.lbvh_node_index_ = active_node.authority.node_index;
      step.leaf_begin_ = active_node.authority.leaf_begin;
      step.leaf_end_ = active_node.authority.leaf_end;
      active_node_.reset();
      ++audit_.inconclusive_subtree_count;
      if (pending_node_count_ == 0U) {
        complete_ = true;
        audit_.complete = true;
        step.traversal_complete_after_step_ = true;
      }
      return step;
    };

    const auto expand_active_internal_node = [&]() {
      if (node.is_leaf()) {
        throw std::logic_error(
            "a grouped traversal cannot expand a leaf node");
      }
      if (pending_node_count_ > pending_nodes_.size() - 2U) {
        throw std::logic_error(
            "the certified LBVH exceeded the fixed grouped traversal stack");
      }
      if (node.left_child >= index.nodes_.size() ||
          node.right_child >= index.nodes_.size()) {
        throw std::logic_error(
            "a grouped traversal internal node has an invalid child");
      }
      const MortonLbvhIndex::Node& left = index.nodes_[node.left_child];
      const MortonLbvhIndex::Node& right = index.nodes_[node.right_child];
      const std::uint64_t inherited_mask =
          active_node.uses_witness_subtree_pool
          ? active_node.authority.inherited_strict_witness_mask
          : active_node.strict_witness_mask;
      const NodeAuthority left_authority{
          node.left_child,
          left.leaf_begin,
          left.leaf_end,
          inherited_mask};
      const NodeAuthority right_authority{
          node.right_child,
          right.leaf_begin,
          right.leaf_end,
          inherited_mask};
      require_incrementable(
          audit_.internal_node_expansion_count,
          "the grouped traversal expansion count overflows size_t");
      pending_nodes_[pending_node_count_] = left_authority;
      pending_nodes_[pending_node_count_ + 1U] = right_authority;
      pending_node_count_ += 2U;
      active_node_.reset();
      ++audit_.internal_node_expansion_count;
      audit_.maximum_pending_node_count = std::max(
          audit_.maximum_pending_node_count,
          pending_node_count_);
    };

    if (witness_pool_entry_count_ < required_witness_count_) {
      require_incrementable(
          audit_.fallback_subtree_count,
          "the grouped traversal fallback count overflows size_t");
      ExactGroupedAnchoredPairTraversalStep step = make_step(
          ExactGroupedAnchoredPairTraversalStepKind::fallback_subtree,
          ExactGroupedAnchoredPairTraversalStopReason::none);
      step.lbvh_node_index_ = active_node.authority.node_index;
      step.leaf_begin_ = active_node.authority.leaf_begin;
      step.leaf_end_ = active_node.authority.leaf_end;
      active_node_.reset();
      pending_node_count_ = 0U;
      complete_ = true;
      audit_.complete = true;
      ++audit_.fallback_subtree_count;
      step.traversal_complete_after_step_ = true;
      return step;
    }

    if (emit_off_diagonal_inconclusive_subtree_ &&
        contains_any_actual_anchor(
            active_node.query_bounds,
            std::span<const ExactDyadicAabb3>{
                anchor_point_bounds_.data(), anchor_count_})) {
      if (node.is_leaf()) {
        return emit_inconclusive_subtree();
      }
      require_incrementable(
          audit_.diagonal_node_descent_count,
          "the grouped traversal diagonal-descent count overflows size_t");
      active_node.strict_witness_mask =
          active_node.authority.inherited_strict_witness_mask;
      ++audit_.diagonal_node_descent_count;
      expand_active_internal_node();
      continue;
    }

    if (search_witness_subtrees_first_ &&
        !witness_subtree_search_complete_ &&
        witness_pool_entry_count_ >= required_witness_count_ &&
        active_node.next_witness_offset == 0U &&
        active_node.next_anchor_offset == 0U &&
        active_node.strict_witness_mask == 0U) {
      const auto fail_open_witness_subtree_search = [&]() {
        checked_increment(
            audit_.witness_subtree_fail_open_count,
            "the grouped witness-subtree fail-open count overflows size_t");
        witness_subtree_search_complete_ = true;
        pending_witness_subtree_node_count_ = 0U;
        active_witness_subtree_node_.reset();
        witness_subtree_receipt_count_ = 0U;
        witness_subtree_point_count_ = 0U;
      };

      const auto witness_node_contains_anchor = [this](
                                                     std::size_t leaf_begin,
                                                     std::size_t leaf_end) {
        for (std::size_t anchor_offset = 0U;
             anchor_offset < anchor_count_;
             ++anchor_offset) {
          if (anchor_leaf_positions_[anchor_offset] >= leaf_begin &&
              anchor_leaf_positions_[anchor_offset] < leaf_end) {
            return true;
          }
        }
        return false;
      };

      const auto expand_active_witness_subtree_node = [&]() {
        if (!active_witness_subtree_node_.has_value()) {
          throw std::logic_error(
              "a grouped witness-subtree expansion lost its active node");
        }
        const WitnessSubtreeNodeAuthority authority =
            active_witness_subtree_node_->authority;
        const MortonLbvhIndex::Node& witness_node =
            index.nodes_[authority.node_index];
        if (witness_node.is_leaf()) {
          active_witness_subtree_node_.reset();
          return;
        }
        if (pending_witness_subtree_node_count_ >
            pending_witness_subtree_nodes_.size() - 2U) {
          throw std::logic_error(
              "the certified LBVH exceeded the fixed witness-subtree stack");
        }
        if (witness_node.left_child >= index.nodes_.size() ||
            witness_node.right_child >= index.nodes_.size()) {
          throw std::logic_error(
              "a grouped witness-subtree node has an invalid child");
        }
        const MortonLbvhIndex::Node& left =
            index.nodes_[witness_node.left_child];
        const MortonLbvhIndex::Node& right =
            index.nodes_[witness_node.right_child];
        const WitnessSubtreeNodeAuthority left_authority{
            witness_node.left_child, left.leaf_begin, left.leaf_end};
        const WitnessSubtreeNodeAuthority right_authority{
            witness_node.right_child, right.leaf_begin, right.leaf_end};
        const std::array<long double, 3> target =
            closest_interval_midpoint(anchor_bounds_, active_node.query_bounds);
        const ExactDyadicAabb3 left_bounds =
            ExactGroupedAnchoredPairPruneCertifier::node_bounds(
                index, cloud, left_authority.node_index);
        const ExactDyadicAabb3 right_bounds =
            ExactGroupedAnchoredPairPruneCertifier::node_bounds(
                index, cloud, right_authority.node_index);
        const bool left_first = proposal_minimum_squared_distance(
                                    target, left_bounds) <=
            proposal_minimum_squared_distance(target, right_bounds);
        // The stack pops its last entry: keep the midpoint-facing child last.
        pending_witness_subtree_nodes_[
            pending_witness_subtree_node_count_] =
            left_first ? right_authority : left_authority;
        pending_witness_subtree_nodes_[
            pending_witness_subtree_node_count_ + 1U] =
            left_first ? left_authority : right_authority;
        pending_witness_subtree_node_count_ += 2U;
        active_witness_subtree_node_.reset();
      };

      if (!witness_subtree_preflight_complete_) {
        if (work.exact_predicate_count >=
            budget.maximum_exact_predicate_count) {
          return budget_exhausted_step(
              ExactGroupedAnchoredPairTraversalStopReason::
                  exact_predicate_limit);
        }
        require_incrementable(
            work.exact_predicate_count,
            "the grouped witness-core preflight step count overflows size_t");
        require_incrementable(
            work.witness_subtree_exact_predicate_count,
            "the grouped witness-core specific step count overflows size_t");
        require_incrementable(
            audit_.exact_predicate_count,
            "the grouped witness-core preflight audit overflows size_t");
        require_incrementable(
            audit_.witness_subtree_exact_predicate_count,
            "the grouped witness-core specific audit overflows size_t");
        const int continuous_core_minimum_sign =
            exact_diametral_phi_continuous_core_minimum_sign(
                anchor_bounds_, active_node.query_bounds);
        ++work.exact_predicate_count;
        ++work.witness_subtree_exact_predicate_count;
        ++audit_.exact_predicate_count;
        ++audit_.witness_subtree_exact_predicate_count;
        witness_subtree_preflight_complete_ = true;
        if (continuous_core_minimum_sign >= 0) {
          fail_open_witness_subtree_search();
        }
      }

      while (!witness_subtree_search_complete_) {
        if (!active_witness_subtree_node_.has_value()) {
          if (pending_witness_subtree_node_count_ == 0U ||
              witness_subtree_node_visit_count_ >=
                  exact_grouped_anchored_pair_maximum_witness_subtree_node_visit_count) {
            fail_open_witness_subtree_search();
            break;
          }
          if (work.node_visit_count >= budget.maximum_node_visit_count) {
            return budget_exhausted_step(
                ExactGroupedAnchoredPairTraversalStopReason::node_visit_limit);
          }
          require_incrementable(
              work.node_visit_count,
              "the grouped witness-subtree step node count overflows size_t");
          require_incrementable(
              work.witness_subtree_node_visit_count,
              "the grouped witness-subtree specific step node count overflows size_t");
          require_incrementable(
              audit_.node_visit_count,
              "the grouped witness-subtree total node count overflows size_t");
          require_incrementable(
              audit_.witness_subtree_node_visit_count,
              "the grouped witness-subtree audit node count overflows size_t");
          require_incrementable(
              witness_subtree_node_visit_count_,
              "the grouped witness-subtree bounded node count overflows size_t");

          const WitnessSubtreeNodeAuthority authority =
              pending_witness_subtree_nodes_[
                  pending_witness_subtree_node_count_ - 1U];
          --pending_witness_subtree_node_count_;
          if (authority.node_index >= index.nodes_.size()) {
            throw std::logic_error(
                "a grouped witness-subtree authority names an invalid node");
          }
          const MortonLbvhIndex::Node& witness_node =
              index.nodes_[authority.node_index];
          if (witness_node.leaf_begin != authority.leaf_begin ||
              witness_node.leaf_end != authority.leaf_end) {
            throw std::logic_error(
                "a grouped witness-subtree authority lost its leaf range");
          }
          active_witness_subtree_node_.emplace(
              ActiveWitnessSubtreeNode{
                  authority,
                  ExactGroupedAnchoredPairPruneCertifier::node_bounds(
                      index, cloud, authority.node_index),
                  0U});
          ++work.node_visit_count;
          ++work.witness_subtree_node_visit_count;
          ++audit_.node_visit_count;
          ++audit_.witness_subtree_node_visit_count;
          ++witness_subtree_node_visit_count_;
        }

        ActiveWitnessSubtreeNode& witness_active =
            *active_witness_subtree_node_;
        const bool overlaps_query = half_open_ranges_overlap(
            witness_active.authority.leaf_begin,
            witness_active.authority.leaf_end,
            active_node.authority.leaf_begin,
            active_node.authority.leaf_end);
        if (overlaps_query || witness_node_contains_anchor(
                                  witness_active.authority.leaf_begin,
                                  witness_active.authority.leaf_end)) {
          expand_active_witness_subtree_node();
          continue;
        }

        bool rejected = false;
        while (witness_active.next_anchor_offset < anchor_count_) {
          if (work.exact_predicate_count >=
              budget.maximum_exact_predicate_count) {
            return budget_exhausted_step(
                ExactGroupedAnchoredPairTraversalStopReason::
                    exact_predicate_limit);
          }
          require_incrementable(
              work.exact_predicate_count,
              "the grouped witness-subtree total step predicate count overflows size_t");
          require_incrementable(
              work.witness_subtree_exact_predicate_count,
              "the grouped witness-subtree specific step predicate count overflows size_t");
          require_incrementable(
              audit_.exact_predicate_count,
              "the grouped witness-subtree total predicate count overflows size_t");
          require_incrementable(
              audit_.witness_subtree_exact_predicate_count,
              "the grouped witness-subtree audit predicate count overflows size_t");
          const int maximum_sign =
              exact_diametral_phi_aabb_maximum_sign(
                  anchor_point_bounds_[witness_active.next_anchor_offset],
                  active_node.query_bounds,
                  witness_active.witness_bounds);
          ++witness_active.next_anchor_offset;
          ++work.exact_predicate_count;
          ++work.witness_subtree_exact_predicate_count;
          ++audit_.exact_predicate_count;
          ++audit_.witness_subtree_exact_predicate_count;
          if (maximum_sign >= 0) {
            rejected = true;
            break;
          }
        }
        if (rejected) {
          expand_active_witness_subtree_node();
          continue;
        }
        if (witness_active.next_anchor_offset != anchor_count_) {
          throw std::logic_error(
              "a grouped witness-subtree receipt lost its anchor cursor");
        }
        if (witness_subtree_receipt_count_ >=
            witness_subtree_receipts_.size()) {
          throw std::logic_error(
              "a grouped witness-subtree receipt exceeded the rank bound");
        }
        for (std::size_t receipt_offset = 0U;
             receipt_offset < witness_subtree_receipt_count_;
             ++receipt_offset) {
          const WitnessSubtreeNodeAuthority& receipt =
              witness_subtree_receipts_[receipt_offset];
          if (half_open_ranges_overlap(
                  receipt.leaf_begin,
                  receipt.leaf_end,
                  witness_active.authority.leaf_begin,
                  witness_active.authority.leaf_end)) {
            throw std::logic_error(
                "grouped witness-subtree receipts are not an antichain");
          }
        }
        witness_subtree_receipts_[witness_subtree_receipt_count_] =
            witness_active.authority;
        ++witness_subtree_receipt_count_;
        checked_increment(
            audit_.witness_subtree_receipt_count,
            "the grouped witness-subtree receipt audit overflows size_t");
        for (std::size_t leaf_offset =
                 witness_active.authority.leaf_begin;
             leaf_offset < witness_active.authority.leaf_end &&
             witness_subtree_point_count_ < required_witness_count_;
             ++leaf_offset) {
          witness_subtree_point_ids_[witness_subtree_point_count_] =
              index.leaves_[leaf_offset].point_id;
          ++witness_subtree_point_count_;
        }
        active_witness_subtree_node_.reset();

        if (witness_subtree_point_count_ == required_witness_count_) {
          std::sort(
              witness_subtree_point_ids_.begin(),
              witness_subtree_point_ids_.begin() +
                  static_cast<std::ptrdiff_t>(required_witness_count_));
          witness_pool_point_ids_ = {};
          witness_point_bounds_ = {};
          witness_pool_entry_count_ = required_witness_count_;
          for (std::size_t witness_offset = 0U;
               witness_offset < required_witness_count_;
               ++witness_offset) {
            witness_pool_point_ids_[witness_offset] =
                witness_subtree_point_ids_[witness_offset];
            witness_point_bounds_[witness_offset] = point_bounds(
                cloud, witness_subtree_point_ids_[witness_offset]);
          }
          pending_witness_subtree_node_count_ = 0U;
          witness_subtree_search_complete_ = true;
          active_node.uses_witness_subtree_pool = true;
          checked_increment(
              audit_.witness_subtree_success_count,
              "the grouped witness-subtree success count overflows size_t");
        }
      }
    }

    std::size_t strict_witness_count =
        static_cast<std::size_t>(
            std::popcount(active_node.strict_witness_mask));
    while (strict_witness_count < required_witness_count_ &&
           active_node.next_witness_offset <
               witness_pool_entry_count_) {
      const std::size_t proposal_offset =
          active_node.next_witness_offset;
      const std::size_t witness_offset =
          order_witnesses_by_floating_score_ &&
              !active_node.uses_witness_subtree_pool
          ? active_node.floating_witness_proposals[proposal_offset]
                .canonical_offset
          : proposal_offset;
      const std::uint64_t witness_bit =
          std::uint64_t{1} << witness_offset;
      if (!active_node.uses_witness_subtree_pool &&
          (active_node.authority.inherited_strict_witness_mask &
           witness_bit) != 0U) {
        if (active_node.next_anchor_offset != 0U) {
          throw std::logic_error(
              "a grouped traversal inherited a partially tested witness");
        }
        require_incrementable(
            work.witness_slot_scan_count,
            "the grouped traversal step slot count overflows size_t");
        require_incrementable(
            work.inherited_witness_reuse_count,
            "the grouped traversal step reuse count overflows size_t");
        require_incrementable(
            audit_.witness_slot_scan_count,
            "the grouped traversal slot count overflows size_t");
        require_incrementable(
            audit_.inherited_witness_reuse_count,
            "the grouped traversal reuse count overflows size_t");
        require_incrementable(
            active_node.inherited_strict_witness_count,
            "a grouped traversal node reuse count overflows size_t");
        ++active_node.next_witness_offset;
        active_node.strict_witness_mask |= witness_bit;
        ++active_node.inherited_strict_witness_count;
        ++strict_witness_count;
        ++work.witness_slot_scan_count;
        ++work.inherited_witness_reuse_count;
        ++audit_.witness_slot_scan_count;
        ++audit_.inherited_witness_reuse_count;
        continue;
      }

      if (active_node.next_anchor_offset >= anchor_count_) {
        throw std::logic_error(
            "a grouped traversal has an invalid actual-anchor cursor");
      }
      if (work.exact_predicate_count >=
          budget.maximum_exact_predicate_count) {
        return budget_exhausted_step(
            ExactGroupedAnchoredPairTraversalStopReason::
                exact_predicate_limit);
      }
      require_incrementable(
          work.exact_predicate_count,
          "the grouped traversal step predicate count overflows size_t");
      require_incrementable(
          audit_.exact_predicate_count,
          "the grouped traversal predicate count overflows size_t");
      require_incrementable(
          active_node.node_exact_predicate_count,
          "a grouped traversal node predicate count overflows size_t");

      std::optional<int> filtered_sign;
      if (audit_.fp64_interval_filter_enabled) {
        filtered_sign = filtered_grouped_phi_aabb_sign(
            anchor_point_bounds_[active_node.next_anchor_offset],
            active_node.query_bounds,
            witness_point_bounds_[witness_offset]);
      }
      int maximum_sign = 0;
      if (filtered_sign.has_value()) {
        maximum_sign = *filtered_sign;
        if (maximum_sign < 0) {
          checked_increment(
              work.fp64_filtered_negative_predicate_count,
              "the grouped step filtered-negative count overflows size_t");
          checked_increment(
              audit_.fp64_filtered_negative_predicate_count,
              "the grouped filtered-negative count overflows size_t");
          checked_increment(
              active_node.node_fp64_filtered_negative_predicate_count,
              "the grouped node filtered-negative count overflows size_t");
        } else {
          checked_increment(
              work.fp64_filtered_positive_predicate_count,
              "the grouped step filtered-positive count overflows size_t");
          checked_increment(
              audit_.fp64_filtered_positive_predicate_count,
              "the grouped filtered-positive count overflows size_t");
          checked_increment(
              active_node.node_fp64_filtered_positive_predicate_count,
              "the grouped node filtered-positive count overflows size_t");
        }
      } else {
        maximum_sign = exact_diametral_phi_aabb_maximum_sign(
            anchor_point_bounds_[active_node.next_anchor_offset],
            active_node.query_bounds,
            witness_point_bounds_[witness_offset]);
        if (order_witnesses_by_floating_score_) {
          checked_increment(
              work.exact_fallback_predicate_count,
              "the grouped step exact-fallback count overflows size_t");
          checked_increment(
              audit_.exact_fallback_predicate_count,
              "the grouped exact-fallback count overflows size_t");
          checked_increment(
              active_node.node_exact_fallback_predicate_count,
              "the grouped node exact-fallback count overflows size_t");
        }
      }

      ++active_node.next_anchor_offset;
      ++active_node.node_exact_predicate_count;
      ++work.exact_predicate_count;
      ++audit_.exact_predicate_count;
      if (maximum_sign >= 0 ||
          active_node.next_anchor_offset == anchor_count_) {
        require_incrementable(
            work.witness_slot_scan_count,
            "the grouped traversal step slot count overflows size_t");
        require_incrementable(
            audit_.witness_slot_scan_count,
            "the grouped traversal slot count overflows size_t");
        ++active_node.next_witness_offset;
        active_node.next_anchor_offset = 0U;
        ++work.witness_slot_scan_count;
        ++audit_.witness_slot_scan_count;
        if (maximum_sign < 0) {
          require_incrementable(
              work.strict_witness_discovery_count,
              "the grouped traversal step strict count overflows size_t");
          require_incrementable(
              audit_.strict_witness_discovery_count,
              "the grouped traversal strict count overflows size_t");
          active_node.strict_witness_mask |= witness_bit;
          ++strict_witness_count;
          ++work.strict_witness_discovery_count;
          ++audit_.strict_witness_discovery_count;
        }
      }
    }

    if (strict_witness_count == required_witness_count_) {
      require_incrementable(
          audit_.certified_prune_count,
          "the grouped traversal prune count overflows size_t");
      ExactGroupedAnchoredPairTraversalStep step = make_step(
          ExactGroupedAnchoredPairTraversalStepKind::certified_prune,
          ExactGroupedAnchoredPairTraversalStopReason::none);
      step.lbvh_node_index_ = active_node.authority.node_index;
      step.leaf_begin_ = active_node.authority.leaf_begin;
      step.leaf_end_ = active_node.authority.leaf_end;
      step.prune_certificate_.emplace(mint_certificate(active_node));
      active_node_.reset();
      ++audit_.certified_prune_count;
      if (pending_node_count_ == 0U) {
        complete_ = true;
        audit_.complete = true;
        step.traversal_complete_after_step_ = true;
      }
      return step;
    }

    if (active_node.next_witness_offset !=
            witness_pool_entry_count_ ||
        active_node.next_anchor_offset != 0U) {
      throw std::logic_error(
          "a grouped traversal stopped its witness scan without a reason");
    }

    if (emit_off_diagonal_inconclusive_subtree_) {
      return emit_inconclusive_subtree();
    }

    if (node.is_leaf()) {
      if (node.leaf_end != node.leaf_begin + 1U ||
          node.leaf_begin >= index.leaves_.size()) {
        throw std::logic_error(
            "a grouped traversal reached an invalid LBVH leaf");
      }
      require_incrementable(
          audit_.unresolved_leaf_count,
          "the grouped traversal unresolved-leaf count overflows size_t");
      ExactGroupedAnchoredPairTraversalStep step = make_step(
          ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf,
          ExactGroupedAnchoredPairTraversalStopReason::none);
      step.lbvh_node_index_ = active_node.authority.node_index;
      step.leaf_begin_ = active_node.authority.leaf_begin;
      step.leaf_end_ = active_node.authority.leaf_end;
      step.unresolved_point_id_ =
          index.leaves_[node.leaf_begin].point_id;
      active_node_.reset();
      ++audit_.unresolved_leaf_count;
      if (pending_node_count_ == 0U) {
        complete_ = true;
        audit_.complete = true;
        step.traversal_complete_after_step_ = true;
      }
      return step;
    }
    expand_active_internal_node();
  }
}

ExactGroupedAnchoredPairPruneCertificate
certify_exact_grouped_anchored_pair_prune(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchor_point_ids,
    std::span<const PointId> witness_pool_point_ids,
    std::size_t lbvh_node_index,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget) {
  return ExactGroupedAnchoredPairPruneCertifier::certify(
      index,
      cloud,
      anchor_point_ids,
      witness_pool_point_ids,
      lbvh_node_index,
      maximum_closed_rank,
      budget);
}

}  // namespace morsehgp3d::hierarchy
