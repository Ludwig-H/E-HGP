#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <bit>
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

  anchor_bounds_ = build_anchor_bounds(cloud, anchor_point_ids);
  maximum_closed_rank_ = maximum_closed_rank;
  required_witness_count_ = maximum_closed_rank - 1U;
  start_node_index_ = lbvh_node_index;
  cloud_identity_ = cloud.identity_;
  lbvh_identity_ = index.identity_;
  emit_off_diagonal_inconclusive_subtree_ =
      emit_off_diagonal_inconclusive_subtree;

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
      active_node_.emplace(ActiveNode{
          authority,
          query_bounds,
          0U,
          0U,
          0U,
          0U,
          0U});
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
          active_node.strict_witness_mask;
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

    std::size_t strict_witness_count =
        static_cast<std::size_t>(
            std::popcount(active_node.strict_witness_mask));
    while (strict_witness_count < required_witness_count_ &&
           active_node.next_witness_offset <
               witness_pool_entry_count_) {
      const std::size_t witness_offset =
          active_node.next_witness_offset;
      const std::uint64_t witness_bit =
          std::uint64_t{1} << witness_offset;
      if ((active_node.authority.inherited_strict_witness_mask &
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

      const int maximum_sign =
          exact_diametral_phi_aabb_maximum_sign(
              anchor_point_bounds_[active_node.next_anchor_offset],
              active_node.query_bounds,
              witness_point_bounds_[witness_offset]);

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
