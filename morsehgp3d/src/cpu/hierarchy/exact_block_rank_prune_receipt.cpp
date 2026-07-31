#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::MortonLbvhIndex;

inline constexpr std::size_t exact_block_rank_prune_maximum_lbvh_depth =
    3U * MortonLbvhIndex::morton_bits_per_axis +
    std::numeric_limits<std::size_t>::digits;
inline constexpr std::size_t exact_block_rank_prune_maximum_frontier_size =
    exact_block_rank_prune_maximum_lbvh_depth + 1U;

struct ExactBlockRankAutomaticSearchAudit {
  std::size_t node_visit_count{};
  std::size_t internal_expansion_count{};
  std::size_t support_subtree_skip_count{};
  std::size_t exact_phi_aabb_maximum_evaluation_count{};
  std::size_t maximum_frontier_node_count{};
  bool frontier_bound_validated{false};
  bool sufficient_mass_reached{false};
  bool exhausted_without_sufficient_mass{false};
};

[[nodiscard]] bool authority_less(
    const ExactBlockRankNodeAuthority& first,
    const ExactBlockRankNodeAuthority& second) noexcept {
  if (first.leaf_begin != second.leaf_begin) {
    return first.leaf_begin < second.leaf_begin;
  }
  if (first.leaf_end != second.leaf_end) {
    return first.leaf_end < second.leaf_end;
  }
  return first.node_index < second.node_index;
}

[[nodiscard]] bool ranges_are_disjoint(
    const ExactBlockRankNodeAuthority& first,
    const ExactBlockRankNodeAuthority& second) noexcept {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

[[nodiscard]] bool range_is_contained_by(
    const ExactBlockRankNodeAuthority& candidate,
    const ExactBlockRankNodeAuthority& container) noexcept {
  return container.leaf_begin <= candidate.leaf_begin &&
      candidate.leaf_end <= container.leaf_end;
}

[[nodiscard]] bool checked_add(
    std::size_t first,
    std::size_t second,
    std::size_t& sum) noexcept {
  constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
  if (first > maximum - second) {
    return false;
  }
  sum = first + second;
  return true;
}

[[nodiscard]] bool checked_pair_masses(
    std::size_t first_support_count,
    std::size_t second_support_count,
    std::size_t& unordered_pair_mass,
    std::size_t& directed_pair_mass) noexcept {
  constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
  if (first_support_count == 0U || second_support_count == 0U ||
      first_support_count > maximum / second_support_count) {
    return false;
  }
  unordered_pair_mass = first_support_count * second_support_count;
  if (unordered_pair_mass > maximum / 2U) {
    unordered_pair_mass = 0U;
    return false;
  }
  directed_pair_mass = 2U * unordered_pair_mass;
  return true;
}

}  // namespace

class ExactBlockRankPruneReceiptCertifier {
 public:
  [[nodiscard]] static ExactBlockRankNodeAuthority authority(
      const MortonLbvhIndex& index,
      std::size_t node_index) noexcept {
    const auto& node = index.nodes_[node_index];
    return ExactBlockRankNodeAuthority{
        node_index, node.leaf_begin, node.leaf_end};
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

  [[nodiscard]] static ExactBlockRankPruneResult result_without_receipt(
      ExactBlockRankPruneStatus status,
      ExactBlockRankPruneAudit audit) {
    return ExactBlockRankPruneResult(status, std::move(audit), std::nullopt);
  }

  static void apply_automatic_search_audit(
      ExactBlockRankPruneResult& result,
      const ExactBlockRankAutomaticSearchAudit& search) {
    result.audit_.automatic_search_node_visit_count =
        search.node_visit_count;
    result.audit_.automatic_search_internal_expansion_count =
        search.internal_expansion_count;
    result.audit_.automatic_search_support_subtree_skip_count =
        search.support_subtree_skip_count;
    result.audit_.automatic_search_exact_phi_aabb_maximum_evaluation_count =
        search.exact_phi_aabb_maximum_evaluation_count;
    result.audit_.automatic_search_maximum_frontier_node_count =
        search.maximum_frontier_node_count;
    result.audit_.automatic_witness_search_requested = true;
    result.audit_.automatic_witness_search_frontier_bound_validated =
        search.frontier_bound_validated;
    result.audit_.automatic_witness_search_sufficient_mass_reached =
        search.sufficient_mass_reached;
    result.audit_.automatic_witness_search_exhausted_without_sufficient_mass =
        search.exhausted_without_sufficient_mass;
    if (result.receipt_.has_value()) {
      result.receipt_->audit_ = result.audit_;
    }
  }

  [[nodiscard]] static ExactBlockRankPruneResult certify(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      std::size_t first_support_node_index,
      std::size_t second_support_node_index,
      std::span<const std::size_t> witness_node_indices,
      std::size_t maximum_closed_rank) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "an exact block-rank prune receipt requires its cloud's exact LBVH");
    }
    if (maximum_closed_rank < 2U ||
        maximum_closed_rank > exact_block_rank_prune_maximum_closed_rank) {
      throw std::out_of_range(
          "an exact block-rank maximum closed rank must be in [2, 11]");
    }

    ExactBlockRankPruneAudit audit;
    audit.proposed_witness_node_count = witness_node_indices.size();
    audit.required_witness_point_count = maximum_closed_rank - 1U;

    if (first_support_node_index >= index.nodes_.size() ||
        second_support_node_index >= index.nodes_.size()) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::rejected_invalid_node_authority,
          std::move(audit));
    }

    audit.support_nodes = {
        authority(index, first_support_node_index),
        authority(index, second_support_node_index)};
    std::sort(
        audit.support_nodes.begin(),
        audit.support_nodes.end(),
        authority_less);
    audit.support_nodes_authenticated = true;
    if (!ranges_are_disjoint(
            audit.support_nodes[0], audit.support_nodes[1])) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::rejected_support_overlap,
          std::move(audit));
    }
    audit.support_ranges_disjoint = true;

    if (!checked_pair_masses(
            audit.support_nodes[0].leaf_count(),
            audit.support_nodes[1].leaf_count(),
            audit.unordered_pair_mass,
            audit.directed_pair_mass)) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::rejected_arithmetic_overflow,
          std::move(audit));
    }
    audit.pair_mass_overflow_checked = true;

    audit.proposal_node_count_limit_checked = true;
    if (witness_node_indices.size() >
        exact_block_rank_prune_maximum_proposed_witness_node_count) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::rejected_witness_node_count_limit,
          std::move(audit));
    }

    std::array<ExactBlockRankNodeAuthority,
               exact_block_rank_prune_maximum_proposed_witness_node_count>
        canonical_witness_storage{};
    std::size_t canonical_witness_count = 0U;
    for (const std::size_t witness_node_index : witness_node_indices) {
      if (witness_node_index >= index.nodes_.size()) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_invalid_node_authority,
            std::move(audit));
      }
      canonical_witness_storage[canonical_witness_count] =
          authority(index, witness_node_index);
      ++canonical_witness_count;
    }
    audit.witness_nodes_authenticated = true;
    std::span<ExactBlockRankNodeAuthority> canonical_witnesses{
        canonical_witness_storage.data(), canonical_witness_count};
    std::sort(
        canonical_witnesses.begin(),
        canonical_witnesses.end(),
        authority_less);
    audit.canonical_witness_node_count = canonical_witness_count;
    audit.canonical_witness_order_enforced = true;

    for (const ExactBlockRankNodeAuthority& witness : canonical_witnesses) {
      if (!ranges_are_disjoint(witness, audit.support_nodes[0]) ||
          !ranges_are_disjoint(witness, audit.support_nodes[1])) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_witness_support_overlap,
            std::move(audit));
      }
    }
    audit.witness_support_ranges_disjoint = true;

    for (std::size_t offset = 1U;
         offset < canonical_witnesses.size();
         ++offset) {
      if (!ranges_are_disjoint(
              canonical_witnesses[offset - 1U],
              canonical_witnesses[offset])) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_witness_antichain_overlap,
            std::move(audit));
      }
    }
    audit.witness_antichain_validated = true;

    for (const ExactBlockRankNodeAuthority& witness : canonical_witnesses) {
      std::size_t new_mass = 0U;
      if (!checked_add(
              audit.proposed_witness_point_count,
              witness.leaf_count(),
              new_mass)) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_arithmetic_overflow,
            std::move(audit));
      }
      audit.proposed_witness_point_count = new_mass;
    }
    audit.witness_mass_overflow_checked = true;
    if (audit.proposed_witness_point_count <
        audit.required_witness_point_count) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::insufficient_witness_mass,
          std::move(audit));
    }

    const ExactDyadicAabb3 first_support_bounds =
        node_bounds(index, cloud, audit.support_nodes[0].node_index);
    const ExactDyadicAabb3 second_support_bounds =
        node_bounds(index, cloud, audit.support_nodes[1].node_index);
    std::array<ExactBlockRankNodeAuthority,
               exact_block_rank_prune_maximum_witness_node_count>
        strict_witnesses{};
    std::size_t strict_witness_count = 0U;
    for (const ExactBlockRankNodeAuthority& witness : canonical_witnesses) {
      ++audit.examined_witness_node_count;
      ++audit.exact_phi_aabb_maximum_evaluation_count;
      const int maximum_sign = exact_diametral_phi_aabb_maximum_sign(
          first_support_bounds,
          second_support_bounds,
          node_bounds(index, cloud, witness.node_index));
      if (maximum_sign >= 0) {
        ++audit.nonnegative_witness_node_count;
        continue;
      }

      if (strict_witness_count >= strict_witnesses.size()) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_arithmetic_overflow,
            std::move(audit));
      }
      strict_witnesses[strict_witness_count] = witness;
      ++strict_witness_count;
      audit.strict_witness_node_count = strict_witness_count;
      std::size_t new_mass = 0U;
      if (!checked_add(
              audit.certified_witness_point_count,
              witness.leaf_count(),
              new_mass)) {
        return result_without_receipt(
            ExactBlockRankPruneStatus::rejected_arithmetic_overflow,
            std::move(audit));
      }
      audit.certified_witness_point_count = new_mass;
      if (audit.certified_witness_point_count >=
          audit.required_witness_point_count) {
        break;
      }
    }

    if (audit.certified_witness_point_count <
        audit.required_witness_point_count) {
      return result_without_receipt(
          ExactBlockRankPruneStatus::inconclusive_nonnegative_phi,
          std::move(audit));
    }

    audit.strict_witness_mass_sufficient = true;
    audit.process_local_authority_retained = true;
    ExactBlockRankPruneReceipt receipt(
        audit.support_nodes,
        strict_witnesses,
        strict_witness_count,
        maximum_closed_rank,
        audit.required_witness_point_count,
        audit.certified_witness_point_count,
        audit.unordered_pair_mass,
        audit.directed_pair_mass,
        audit,
        cloud.identity_,
        index.identity_);
    return ExactBlockRankPruneResult(
        ExactBlockRankPruneStatus::certified_above_rank,
        audit,
        std::optional<ExactBlockRankPruneReceipt>{std::move(receipt)});
  }

  [[nodiscard]] static ExactBlockRankPruneResult generate(
      const MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      std::size_t first_support_node_index,
      std::size_t second_support_node_index,
      std::size_t maximum_closed_rank) {
    ExactBlockRankAutomaticSearchAudit search;
    ExactBlockRankPruneResult preflight = certify(
        index,
        cloud,
        first_support_node_index,
        second_support_node_index,
        std::span<const std::size_t>{},
        maximum_closed_rank);
    if (preflight.status() !=
        ExactBlockRankPruneStatus::insufficient_witness_mass) {
      apply_automatic_search_audit(preflight, search);
      return preflight;
    }
    if (index.build_counters_.maximum_depth >
        exact_block_rank_prune_maximum_lbvh_depth) {
      throw std::logic_error(
          "an exact block-rank automatic search exceeds its fixed LBVH "
          "depth bound");
    }
    search.frontier_bound_validated = true;

    const std::array<ExactBlockRankNodeAuthority, 2U> support_nodes =
        preflight.audit().support_nodes;
    const ExactDyadicAabb3 first_support_bounds =
        node_bounds(index, cloud, support_nodes[0].node_index);
    const ExactDyadicAabb3 second_support_bounds =
        node_bounds(index, cloud, support_nodes[1].node_index);
    const std::size_t required_witness_point_count =
        maximum_closed_rank - 1U;

    std::array<std::size_t,
               exact_block_rank_prune_maximum_witness_node_count>
        witness_node_indices{};
    std::size_t witness_node_count = 0U;
    std::size_t witness_point_count = 0U;
    std::array<std::size_t, exact_block_rank_prune_maximum_frontier_size>
        frontier{};
    std::size_t frontier_node_count = 1U;
    frontier[0] = index.root_index_;
    search.maximum_frontier_node_count = frontier_node_count;

    const auto push_children =
        [&index, &frontier, &frontier_node_count, &search](
            std::size_t node_index) {
          const auto& node = index.nodes_[node_index];
          if (node.is_leaf() || node.left_child >= index.nodes_.size() ||
              node.right_child >= index.nodes_.size() ||
              node.left_child == node.right_child) {
            throw std::logic_error(
                "an exact block-rank automatic search cannot expand its "
                "native LBVH node");
          }
          if (frontier_node_count > frontier.size() - 2U) {
            throw std::logic_error(
                "an exact block-rank automatic search exceeded its fixed "
                "frontier");
          }
          std::size_t first_child = node.left_child;
          std::size_t second_child = node.right_child;
          if (index.nodes_[second_child].leaf_begin <
              index.nodes_[first_child].leaf_begin) {
            std::swap(first_child, second_child);
          }
          frontier[frontier_node_count] = second_child;
          ++frontier_node_count;
          frontier[frontier_node_count] = first_child;
          ++frontier_node_count;
          ++search.internal_expansion_count;
          search.maximum_frontier_node_count = std::max(
              search.maximum_frontier_node_count,
              frontier_node_count);
        };

    while (frontier_node_count != 0U &&
           witness_point_count < required_witness_point_count) {
      const std::size_t node_index = frontier[frontier_node_count - 1U];
      --frontier_node_count;
      ++search.node_visit_count;
      const ExactBlockRankNodeAuthority candidate =
          authority(index, node_index);
      const bool overlaps_first =
          !ranges_are_disjoint(candidate, support_nodes[0]);
      const bool overlaps_second =
          !ranges_are_disjoint(candidate, support_nodes[1]);
      if (overlaps_first || overlaps_second) {
        if ((overlaps_first &&
             range_is_contained_by(candidate, support_nodes[0])) ||
            (overlaps_second &&
             range_is_contained_by(candidate, support_nodes[1]))) {
          ++search.support_subtree_skip_count;
          continue;
        }
        push_children(node_index);
        continue;
      }

      ++search.exact_phi_aabb_maximum_evaluation_count;
      const int maximum_sign = exact_diametral_phi_aabb_maximum_sign(
          first_support_bounds,
          second_support_bounds,
          node_bounds(index, cloud, node_index));
      if (maximum_sign < 0) {
        if (witness_node_count >= witness_node_indices.size()) {
          throw std::logic_error(
              "an exact block-rank automatic search exceeded its strict "
              "antichain bound");
        }
        witness_node_indices[witness_node_count] = node_index;
        ++witness_node_count;
        if (!checked_add(
                witness_point_count,
                candidate.leaf_count(),
                witness_point_count)) {
          ExactBlockRankPruneAudit audit = preflight.audit();
          ExactBlockRankPruneResult overflow = result_without_receipt(
              ExactBlockRankPruneStatus::rejected_arithmetic_overflow,
              std::move(audit));
          apply_automatic_search_audit(overflow, search);
          return overflow;
        }
        continue;
      }
      if (!index.nodes_[node_index].is_leaf()) {
        push_children(node_index);
      }
    }

    search.sufficient_mass_reached =
        witness_point_count >= required_witness_point_count;
    search.exhausted_without_sufficient_mass =
        !search.sufficient_mass_reached && frontier_node_count == 0U;
    ExactBlockRankPruneResult result = certify(
        index,
        cloud,
        first_support_node_index,
        second_support_node_index,
        std::span<const std::size_t>{
            witness_node_indices.data(), witness_node_count},
        maximum_closed_rank);
    apply_automatic_search_audit(result, search);
    return result;
  }
};

ExactBlockRankPruneReceipt::ExactBlockRankPruneReceipt(
    std::array<ExactBlockRankNodeAuthority, 2U> support_nodes,
    std::array<ExactBlockRankNodeAuthority,
               exact_block_rank_prune_maximum_witness_node_count>
        witness_nodes,
    std::size_t witness_node_count,
    std::size_t maximum_closed_rank,
    std::size_t required_witness_point_count,
    std::size_t certified_witness_point_count,
    std::size_t unordered_pair_mass,
    std::size_t directed_pair_mass,
    ExactBlockRankPruneAudit audit,
    std::shared_ptr<const void> cloud_identity,
    std::shared_ptr<const void> lbvh_identity)
    : support_nodes_(support_nodes),
      witness_nodes_(witness_nodes),
      witness_node_count_(witness_node_count),
      maximum_closed_rank_(maximum_closed_rank),
      required_witness_point_count_(required_witness_point_count),
      certified_witness_point_count_(certified_witness_point_count),
      unordered_pair_mass_(unordered_pair_mass),
      directed_pair_mass_(directed_pair_mass),
      audit_(audit),
      cloud_identity_(std::move(cloud_identity)),
      lbvh_identity_(std::move(lbvh_identity)) {}

bool ExactBlockRankPruneReceipt::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  if (!index.validated_for(cloud) ||
      cloud_identity_.get() != cloud.identity_.get() ||
      lbvh_identity_.get() != index.identity_.get() ||
      maximum_closed_rank_ < 2U ||
      maximum_closed_rank_ > exact_block_rank_prune_maximum_closed_rank ||
      required_witness_point_count_ != maximum_closed_rank_ - 1U ||
      witness_node_count_ == 0U ||
      witness_node_count_ > witness_nodes_.size()) {
    return false;
  }

  const auto authority_is_current = [&index](
                                        const ExactBlockRankNodeAuthority& value) {
    if (value.node_index >= index.nodes_.size()) {
      return false;
    }
    const auto& node = index.nodes_[value.node_index];
    return node.leaf_begin == value.leaf_begin &&
        node.leaf_end == value.leaf_end &&
        value.leaf_begin < value.leaf_end;
  };
  if (!authority_is_current(support_nodes_[0]) ||
      !authority_is_current(support_nodes_[1]) ||
      authority_less(support_nodes_[1], support_nodes_[0]) ||
      !ranges_are_disjoint(support_nodes_[0], support_nodes_[1])) {
    return false;
  }

  std::size_t expected_witness_mass = 0U;
  for (std::size_t offset = 0U; offset < witness_node_count_; ++offset) {
    const ExactBlockRankNodeAuthority& witness = witness_nodes_[offset];
    if (!authority_is_current(witness) ||
        !ranges_are_disjoint(witness, support_nodes_[0]) ||
        !ranges_are_disjoint(witness, support_nodes_[1]) ||
        (offset > 0U &&
         (!authority_less(witness_nodes_[offset - 1U], witness) ||
          !ranges_are_disjoint(witness_nodes_[offset - 1U], witness))) ||
        !checked_add(
            expected_witness_mass,
            witness.leaf_count(),
            expected_witness_mass)) {
      return false;
    }
  }

  std::size_t expected_unordered_mass = 0U;
  std::size_t expected_directed_mass = 0U;
  if (!checked_pair_masses(
          support_nodes_[0].leaf_count(),
          support_nodes_[1].leaf_count(),
          expected_unordered_mass,
          expected_directed_mass)) {
    return false;
  }
  return expected_witness_mass == certified_witness_point_count_ &&
      certified_witness_point_count_ >= required_witness_point_count_ &&
      expected_unordered_mass == unordered_pair_mass_ &&
      expected_directed_mass == directed_pair_mass_ &&
      audit_.support_nodes == support_nodes_ &&
      audit_.proposal_node_count_limit_checked &&
      audit_.required_witness_point_count == required_witness_point_count_ &&
      audit_.certified_witness_point_count == certified_witness_point_count_ &&
      audit_.strict_witness_node_count == witness_node_count_ &&
      audit_.unordered_pair_mass == unordered_pair_mass_ &&
      audit_.directed_pair_mass == directed_pair_mass_ &&
      audit_.support_nodes_authenticated && audit_.support_ranges_disjoint &&
      audit_.witness_nodes_authenticated &&
      audit_.witness_support_ranges_disjoint &&
      audit_.witness_antichain_validated &&
      audit_.canonical_witness_order_enforced &&
      audit_.witness_mass_overflow_checked &&
      audit_.pair_mass_overflow_checked &&
      audit_.strict_witness_mass_sufficient &&
      audit_.process_local_authority_retained &&
      audit_.no_pair_catalog_materialized &&
      audit_.no_global_witness_catalog_materialized &&
      audit_.no_facet_coface_or_incidence_materialized &&
      audit_.no_gamma_or_delaunay_structure_materialized &&
      !audit_.global_hierarchy_exactness_claimed &&
      !audit_.public_morse_exactness_claimed &&
      (!audit_.automatic_witness_search_requested ||
       (audit_.automatic_witness_search_frontier_bound_validated &&
        audit_.automatic_witness_search_sufficient_mass_reached &&
        !audit_.automatic_witness_search_exhausted_without_sufficient_mass &&
        audit_.automatic_search_node_visit_count != 0U &&
        audit_.automatic_search_maximum_frontier_node_count != 0U &&
        audit_.automatic_search_exact_phi_aabb_maximum_evaluation_count >=
            witness_node_count_));
}

bool ExactBlockRankPruneReceipt::certifies(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t first_support_node_index,
    std::size_t second_support_node_index,
    std::size_t maximum_closed_rank) const noexcept {
  if (!validated_for(index, cloud) ||
      maximum_closed_rank != maximum_closed_rank_ ||
      first_support_node_index >= index.nodes_.size() ||
      second_support_node_index >= index.nodes_.size()) {
    return false;
  }
  std::array<ExactBlockRankNodeAuthority, 2U> expected{
      ExactBlockRankPruneReceiptCertifier::authority(
          index, first_support_node_index),
      ExactBlockRankPruneReceiptCertifier::authority(
          index, second_support_node_index)};
  std::sort(expected.begin(), expected.end(), authority_less);
  return expected == support_nodes_;
}

ExactBlockRankPruneResult::ExactBlockRankPruneResult(
    ExactBlockRankPruneStatus status,
    ExactBlockRankPruneAudit audit,
    std::optional<ExactBlockRankPruneReceipt> receipt)
    : status_(status),
      audit_(std::move(audit)),
      receipt_(std::move(receipt)) {}

ExactBlockRankPruneResult certify_exact_block_rank_prune_receipt(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t first_support_node_index,
    std::size_t second_support_node_index,
    std::span<const std::size_t> witness_node_indices,
    std::size_t maximum_closed_rank) {
  return ExactBlockRankPruneReceiptCertifier::certify(
      index,
      cloud,
      first_support_node_index,
      second_support_node_index,
      witness_node_indices,
      maximum_closed_rank);
}

ExactBlockRankPruneResult generate_exact_block_rank_prune_receipt(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t first_support_node_index,
    std::size_t second_support_node_index,
    std::size_t maximum_closed_rank) {
  return ExactBlockRankPruneReceiptCertifier::generate(
      index,
      cloud,
      first_support_node_index,
      second_support_node_index,
      maximum_closed_rank);
}

}  // namespace morsehgp3d::hierarchy
