#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::MortonLbvhIndex;
using spatial::PointId;

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
      if (result.audit_.exact_predicate_count >=
          budget.maximum_exact_predicate_count) {
        return exhausted_result(
            std::move(result),
            ExactGroupedAnchoredPairPruneStopReason::exact_predicate_limit);
      }
      ++result.audit_.exact_predicate_count;
      const PointId witness_point_id =
          witness_pool_point_ids[witness_offset];
      const int maximum_sign = exact_diametral_phi_aabb_maximum_sign(
          result.anchor_bounds_,
          result.query_bounds_,
          point_bounds(cloud, witness_point_id));
      if (maximum_sign >= 0) {
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
