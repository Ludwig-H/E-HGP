#include "morsehgp3d/hierarchy/symmetric_grouped_anchored_pair_prune_receipt.hpp"

#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::MortonLeafRecord;
using spatial::PointId;

[[nodiscard]] bool morton_ranges_are_disjoint(
    std::size_t first_begin,
    std::size_t first_end,
    std::size_t second_begin,
    std::size_t second_end) noexcept {
  return first_end <= second_begin || second_end <= first_begin;
}

[[nodiscard]] bool derive_canonical_anchor_ids(
    std::span<const MortonLeafRecord> leaves,
    std::size_t anchor_leaf_begin,
    std::size_t anchor_leaf_end,
    std::array<PointId,
               exact_grouped_anchored_pair_maximum_anchor_count>&
        canonical_anchor_ids) noexcept {
  if (anchor_leaf_begin >= anchor_leaf_end ||
      anchor_leaf_end > leaves.size() ||
      anchor_leaf_end - anchor_leaf_begin >
          exact_grouped_anchored_pair_maximum_anchor_count) {
    return false;
  }

  const std::size_t anchor_count = anchor_leaf_end - anchor_leaf_begin;
  for (std::size_t offset = 0U; offset < anchor_count; ++offset) {
    canonical_anchor_ids[offset] =
        leaves[anchor_leaf_begin + offset].point_id;
  }

  // G <= 32.  This fixed-capacity insertion sort avoids a dynamic arena and
  // keeps validated_for noexcept.
  for (std::size_t offset = 1U; offset < anchor_count; ++offset) {
    const PointId value = canonical_anchor_ids[offset];
    std::size_t insertion = offset;
    while (insertion > 0U &&
           value < canonical_anchor_ids[insertion - 1U]) {
      canonical_anchor_ids[insertion] =
          canonical_anchor_ids[insertion - 1U];
      --insertion;
    }
    canonical_anchor_ids[insertion] = value;
  }
  for (std::size_t offset = 1U; offset < anchor_count; ++offset) {
    if (canonical_anchor_ids[offset - 1U] >=
        canonical_anchor_ids[offset]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool checked_symmetric_masses(
    std::size_t anchor_count,
    std::size_t query_count,
    std::size_t& unordered_mass,
    std::size_t& directed_mass) noexcept {
  constexpr std::size_t maximum =
      std::numeric_limits<std::size_t>::max();
  if (anchor_count == 0U || query_count == 0U ||
      anchor_count > maximum / query_count) {
    return false;
  }
  unordered_mass = anchor_count * query_count;
  if (unordered_mass > maximum / 2U) {
    unordered_mass = 0U;
    return false;
  }
  directed_mass = 2U * unordered_mass;
  return true;
}

}  // namespace

ExactSymmetricGroupedAnchoredPairPruneReceipt::
    ExactSymmetricGroupedAnchoredPairPruneReceipt(
        const ExactGroupedAnchoredPairPruneCertificate& source_certificate,
        std::size_t anchor_leaf_begin,
        std::size_t anchor_leaf_end,
        std::size_t query_leaf_begin,
        std::size_t query_leaf_end,
        std::size_t query_lbvh_node_index,
        std::size_t maximum_closed_rank,
        std::size_t unordered_mass,
        std::size_t directed_mass,
        ExactSymmetricGroupedAnchoredPairPruneReceiptAudit audit)
    : source_certificate_(source_certificate),
      anchor_leaf_begin_(anchor_leaf_begin),
      anchor_leaf_end_(anchor_leaf_end),
      query_leaf_begin_(query_leaf_begin),
      query_leaf_end_(query_leaf_end),
      query_lbvh_node_index_(query_lbvh_node_index),
      maximum_closed_rank_(maximum_closed_rank),
      unordered_mass_(unordered_mass),
      directed_mass_(directed_mass),
      audit_(audit) {}

bool ExactSymmetricGroupedAnchoredPairPruneReceipt::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const noexcept {
  if (!index.validated_for(cloud)) {
    return false;
  }
  const std::span<const MortonLeafRecord> leaves = index.leaves();
  std::array<PointId, exact_grouped_anchored_pair_maximum_anchor_count>
      canonical_anchor_ids{};
  if (!derive_canonical_anchor_ids(
          leaves,
          anchor_leaf_begin_,
          anchor_leaf_end_,
          canonical_anchor_ids)) {
    return false;
  }
  const std::size_t anchor_count = anchor_leaf_end_ - anchor_leaf_begin_;
  const std::span<const PointId> expected_anchor_ids{
      canonical_anchor_ids.data(), anchor_count};
  if (!source_certificate_.certifies(
          index,
          cloud,
          query_lbvh_node_index_,
          maximum_closed_rank_,
          expected_anchor_ids) ||
      source_certificate_.leaf_begin() != query_leaf_begin_ ||
      source_certificate_.leaf_end() != query_leaf_end_ ||
      query_leaf_begin_ >= query_leaf_end_ ||
      query_leaf_end_ > leaves.size() ||
      !morton_ranges_are_disjoint(
          anchor_leaf_begin_,
          anchor_leaf_end_,
          query_leaf_begin_,
          query_leaf_end_)) {
    return false;
  }

  const std::size_t query_count = query_leaf_end_ - query_leaf_begin_;
  std::size_t expected_unordered_mass = 0U;
  std::size_t expected_directed_mass = 0U;
  if (!checked_symmetric_masses(
          anchor_count,
          query_count,
          expected_unordered_mass,
          expected_directed_mass) ||
      unordered_mass_ != expected_unordered_mass ||
      directed_mass_ != expected_directed_mass) {
    return false;
  }

  return audit_.anchor_count == anchor_count &&
      audit_.query_count == query_count &&
      audit_.source_certificate_recertified &&
      audit_.anchor_morton_range_authenticated &&
      audit_.query_morton_range_authenticated &&
      audit_.morton_ranges_disjoint &&
      audit_.unordered_mass_overflow_checked &&
      audit_.directed_mass_overflow_checked &&
      audit_.process_local_source_authority_retained;
}

ExactSymmetricGroupedAnchoredPairPruneReceipt
recertify_exact_symmetric_grouped_anchored_pair_prune(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactGroupedAnchoredPairPruneCertificate& source_certificate,
    std::size_t expected_anchor_leaf_begin,
    std::size_t expected_anchor_leaf_end,
    std::size_t expected_query_lbvh_node_index,
    std::size_t expected_maximum_closed_rank) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a symmetric grouped prune receipt requires its cloud's exact LBVH");
  }
  const std::span<const MortonLeafRecord> leaves = index.leaves();
  std::array<PointId, exact_grouped_anchored_pair_maximum_anchor_count>
      canonical_anchor_ids{};
  if (!derive_canonical_anchor_ids(
          leaves,
          expected_anchor_leaf_begin,
          expected_anchor_leaf_end,
          canonical_anchor_ids)) {
    throw std::invalid_argument(
        "a symmetric grouped prune receipt requires one authentic bounded anchor range");
  }
  const std::size_t anchor_count =
      expected_anchor_leaf_end - expected_anchor_leaf_begin;
  const std::span<const PointId> expected_anchor_ids{
      canonical_anchor_ids.data(), anchor_count};
  if (!source_certificate.certifies(
          index,
          cloud,
          expected_query_lbvh_node_index,
          expected_maximum_closed_rank,
          expected_anchor_ids)) {
    throw std::invalid_argument(
        "a symmetric grouped prune receipt rejected its P8g certificate");
  }

  const std::size_t query_leaf_begin = source_certificate.leaf_begin();
  const std::size_t query_leaf_end = source_certificate.leaf_end();
  if (query_leaf_begin >= query_leaf_end || query_leaf_end > leaves.size()) {
    throw std::logic_error(
        "a recertified P8g authority exposed an invalid Morton query range");
  }
  if (!morton_ranges_are_disjoint(
          expected_anchor_leaf_begin,
          expected_anchor_leaf_end,
          query_leaf_begin,
          query_leaf_end)) {
    throw std::invalid_argument(
        "a symmetric grouped prune receipt requires disjoint Morton support ranges");
  }

  const std::size_t query_count = query_leaf_end - query_leaf_begin;
  std::size_t unordered_mass = 0U;
  std::size_t directed_mass = 0U;
  if (!checked_symmetric_masses(
          anchor_count, query_count, unordered_mass, directed_mass)) {
    throw std::overflow_error(
        "a symmetric grouped prune receipt mass overflows size_t");
  }

  ExactSymmetricGroupedAnchoredPairPruneReceiptAudit audit;
  audit.anchor_count = anchor_count;
  audit.query_count = query_count;
  audit.source_certificate_recertified = true;
  audit.anchor_morton_range_authenticated = true;
  audit.query_morton_range_authenticated = true;
  audit.morton_ranges_disjoint = true;
  audit.unordered_mass_overflow_checked = true;
  audit.directed_mass_overflow_checked = true;
  audit.process_local_source_authority_retained = true;
  return ExactSymmetricGroupedAnchoredPairPruneReceipt(
      source_certificate,
      expected_anchor_leaf_begin,
      expected_anchor_leaf_end,
      query_leaf_begin,
      query_leaf_end,
      expected_query_lbvh_node_index,
      expected_maximum_closed_rank,
      unordered_mass,
      directed_mass,
      audit);
}

}  // namespace morsehgp3d::hierarchy
