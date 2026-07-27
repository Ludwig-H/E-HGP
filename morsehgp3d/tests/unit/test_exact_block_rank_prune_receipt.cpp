#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactBlockRankPruneReceipt;
using morsehgp3d::hierarchy::ExactBlockRankPruneResult;
using morsehgp3d::hierarchy::ExactBlockRankPruneStatus;
using morsehgp3d::hierarchy::certify_exact_block_rank_prune_receipt;
using morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum_sign;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_aggregate_v<ExactBlockRankPruneReceipt>);
static_assert(!std::is_default_constructible_v<ExactBlockRankPruneReceipt>);
static_assert(!std::is_copy_constructible_v<ExactBlockRankPruneReceipt>);
static_assert(std::is_nothrow_move_constructible_v<ExactBlockRankPruneReceipt>);
static_assert(!std::is_default_constructible_v<ExactBlockRankPruneResult>);

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_line_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(16U);
  for (std::size_t offset = 0U; offset < 16U; ++offset) {
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(offset), 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_zero_phi_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(12U);
  points.push_back(CertifiedPoint3::from_binary64(0.0, 0.0, 0.0));
  points.push_back(CertifiedPoint3::from_binary64(10.0, 0.0, 0.0));
  for (std::size_t offset = 1U; offset < 10U; ++offset) {
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(offset), 0.0, 0.0));
  }
  points.push_back(CertifiedPoint3::from_binary64(5.0, 5.0, 0.0));
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] std::size_t node_for_range(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const ExactBlockRankPruneResult probe =
        certify_exact_block_rank_prune_receipt(
            index,
            cloud,
            node_index,
            node_index,
            std::span<const std::size_t>{},
            2U);
    if (probe.status() ==
            ExactBlockRankPruneStatus::rejected_support_overlap &&
        probe.audit().support_nodes[0].leaf_begin == leaf_begin &&
        probe.audit().support_nodes[0].leaf_end == leaf_end) {
      return node_index;
    }
  }
  throw std::runtime_error("the requested LBVH fixture range was not found");
}

[[nodiscard]] PointId point_id_for_source_index(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (PointId point_id = 0U;
       point_id < static_cast<PointId>(cloud.size());
       ++point_id) {
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::runtime_error("the fixture source point has no canonical id");
}

[[nodiscard]] std::size_t leaf_node_for_point_id(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves =
      index.leaves();
  for (std::size_t leaf_position = 0U;
       leaf_position < leaves.size();
       ++leaf_position) {
    if (leaves[leaf_position].point_id == point_id) {
      return node_for_range(
          index, cloud, leaf_position, leaf_position + 1U);
    }
  }
  throw std::runtime_error("the fixture point has no LBVH leaf");
}

[[nodiscard]] ExactDyadicAabb3 point_box(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3U> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

struct Fixture {
  CanonicalPointCloud cloud{make_line_cloud()};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};

  [[nodiscard]] std::size_t node_for_range(
      std::size_t leaf_begin,
      std::size_t leaf_end) const {
    return ::node_for_range(index, cloud, leaf_begin, leaf_end);
  }

  [[nodiscard]] std::size_t leaf_node(std::size_t leaf_position) const {
    return node_for_range(leaf_position, leaf_position + 1U);
  }
};

[[nodiscard]] std::vector<std::size_t> ten_unit_witnesses(
    const Fixture& fixture) {
  std::vector<std::size_t> witnesses;
  witnesses.reserve(10U);
  for (std::size_t leaf_position = 2U;
       leaf_position < 12U;
       ++leaf_position) {
    witnesses.push_back(fixture.leaf_node(leaf_position));
  }
  return witnesses;
}

void test_k10_multileaf_success_mass_and_determinism() {
  const Fixture fixture;
  const std::size_t first_support = fixture.node_for_range(0U, 2U);
  const std::size_t second_support = fixture.node_for_range(12U, 14U);

  std::vector<std::size_t> witnesses;
  witnesses.reserve(9U);
  witnesses.push_back(fixture.node_for_range(2U, 4U));
  for (std::size_t leaf_position = 4U;
       leaf_position < 12U;
       ++leaf_position) {
    witnesses.push_back(fixture.leaf_node(leaf_position));
  }
  std::vector<std::size_t> reversed = witnesses;
  std::reverse(reversed.begin(), reversed.end());

  const ExactBlockRankPruneResult forward =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          witnesses,
          11U);
  const ExactBlockRankPruneResult reverse =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          second_support,
          first_support,
          reversed,
          11U);
  require(forward.certified() && reverse.certified(),
          "the K=10 multileaf block was not certified");
  require(forward.audit() == reverse.audit(),
          "canonical ordering did not make the audit deterministic");

  const ExactBlockRankPruneReceipt* receipt = forward.receipt();
  const ExactBlockRankPruneReceipt* reversed_receipt = reverse.receipt();
  require(receipt != nullptr && reversed_receipt != nullptr,
          "a certified block did not retain a receipt");
  require(
      receipt->maximum_closed_rank() == 11U &&
          receipt->required_witness_point_count() == 10U &&
          receipt->certified_witness_point_count() == 10U &&
          receipt->witness_nodes().size() == 9U &&
          receipt->support_nodes()[0].leaf_count() == 2U &&
          receipt->support_nodes()[1].leaf_count() == 2U &&
          receipt->unordered_pair_mass() == 4U &&
          receipt->directed_pair_mass() == 8U,
      "the K=10 receipt exposed incorrect block or witness masses");
  const std::span<const morsehgp3d::hierarchy::ExactBlockRankNodeAuthority>
      canonical_witnesses = receipt->witness_nodes();
  const std::span<const morsehgp3d::hierarchy::ExactBlockRankNodeAuthority>
      reversed_canonical_witnesses = reversed_receipt->witness_nodes();
  require(
      receipt->witness_nodes()[0].leaf_begin == 2U &&
          receipt->witness_nodes()[0].leaf_end == 4U &&
          canonical_witnesses.size() == reversed_canonical_witnesses.size() &&
          std::equal(
              canonical_witnesses.begin(),
              canonical_witnesses.end(),
              reversed_canonical_witnesses.begin()),
      "the receipt did not retain the canonical multileaf antichain");
  require(
      receipt->validated_for(fixture.index, fixture.cloud) &&
          receipt->certifies(
              fixture.index,
              fixture.cloud,
              second_support,
              first_support,
              11U) &&
          !receipt->certifies(
              fixture.index,
              fixture.cloud,
              first_support,
              second_support,
              10U),
      "the process-local block authority did not replay exactly");

  const auto& audit = forward.audit();
  require(
      audit.exact_phi_aabb_maximum_evaluation_count == 9U &&
          audit.nonnegative_witness_node_count == 0U &&
          audit.strict_witness_mass_sufficient &&
          audit.pair_mass_overflow_checked &&
          audit.witness_mass_overflow_checked &&
          audit.no_pair_catalog_materialized &&
          audit.no_facet_coface_or_incidence_materialized &&
          audit.no_gamma_or_delaunay_structure_materialized &&
          !audit.global_hierarchy_exactness_claimed &&
          !audit.public_morse_exactness_claimed &&
          ExactBlockRankPruneReceipt::public_status == "not_claimed",
      "the local-only receipt audit claimed excessive authority");
}

void test_insufficient_mass_and_overlaps_are_separate() {
  const Fixture fixture;
  const std::size_t first_support = fixture.node_for_range(0U, 2U);
  const std::size_t second_support = fixture.node_for_range(12U, 14U);
  const std::vector<std::size_t> ten_witnesses =
      ten_unit_witnesses(fixture);
  const std::vector<std::size_t> oversized_proposal(
      65U, fixture.leaf_node(2U));
  const ExactBlockRankPruneResult oversized =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          oversized_proposal,
          11U);
  require(
      oversized.status() ==
              ExactBlockRankPruneStatus::rejected_witness_node_count_limit &&
          oversized.receipt() == nullptr &&
          oversized.audit().support_nodes_authenticated &&
          oversized.audit().support_ranges_disjoint &&
          oversized.audit().pair_mass_overflow_checked &&
          oversized.audit().proposal_node_count_limit_checked &&
          oversized.audit().support_nodes[0].leaf_count() == 2U &&
          oversized.audit().support_nodes[1].leaf_count() == 2U &&
          oversized.audit().unordered_pair_mass == 4U &&
          oversized.audit().directed_pair_mass == 8U &&
          !oversized.audit().witness_nodes_authenticated &&
          oversized.audit().exact_phi_aabb_maximum_evaluation_count == 0U,
      "the fixed proposal bound ran before authentic block preflight");

  const std::span<const std::size_t> nine_witnesses{
      ten_witnesses.data(), 9U};

  const ExactBlockRankPruneResult insufficient =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          nine_witnesses,
          11U);
  require(
      insufficient.status() ==
              ExactBlockRankPruneStatus::insufficient_witness_mass &&
          insufficient.receipt() == nullptr &&
          insufficient.audit().proposed_witness_point_count == 9U &&
          insufficient.audit().exact_phi_aabb_maximum_evaluation_count == 0U,
      "nine singleton witnesses did not fail K=10 at preflight");

  const ExactBlockRankPruneResult support_overlap =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          fixture.leaf_node(0U),
          ten_witnesses,
          11U);
  require(
      support_overlap.status() ==
          ExactBlockRankPruneStatus::rejected_support_overlap,
      "nested A/B support nodes were not rejected");

  std::vector<std::size_t> support_witnesses;
  support_witnesses.reserve(10U);
  support_witnesses.push_back(fixture.leaf_node(0U));
  for (std::size_t leaf_position = 3U;
       leaf_position < 12U;
       ++leaf_position) {
    support_witnesses.push_back(fixture.leaf_node(leaf_position));
  }
  const ExactBlockRankPruneResult witness_support_overlap =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          support_witnesses,
          11U);
  require(
      witness_support_overlap.status() ==
          ExactBlockRankPruneStatus::rejected_witness_support_overlap,
      "a witness overlapping A was not rejected separately");

  std::vector<std::size_t> nested_witnesses;
  nested_witnesses.reserve(10U);
  nested_witnesses.push_back(fixture.node_for_range(2U, 4U));
  nested_witnesses.push_back(fixture.leaf_node(2U));
  for (std::size_t leaf_position = 4U;
       leaf_position < 12U;
       ++leaf_position) {
    nested_witnesses.push_back(fixture.leaf_node(leaf_position));
  }
  const ExactBlockRankPruneResult witness_antichain_overlap =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          nested_witnesses,
          11U);
  require(
      witness_antichain_overlap.status() ==
          ExactBlockRankPruneStatus::rejected_witness_antichain_overlap,
      "nested witness nodes were not rejected as a non-antichain");
}

void test_positive_exact_phi_is_inconclusive() {
  const Fixture fixture;
  const std::size_t first_support = fixture.node_for_range(0U, 2U);
  const std::size_t second_support = fixture.node_for_range(12U, 14U);
  std::vector<std::size_t> witnesses;
  witnesses.reserve(10U);
  for (std::size_t leaf_position = 2U;
       leaf_position < 11U;
       ++leaf_position) {
    witnesses.push_back(fixture.leaf_node(leaf_position));
  }
  witnesses.push_back(fixture.leaf_node(14U));

  const ExactBlockRankPruneResult result =
      certify_exact_block_rank_prune_receipt(
          fixture.index,
          fixture.cloud,
          first_support,
          second_support,
          witnesses,
          11U);
  require(
      result.status() ==
              ExactBlockRankPruneStatus::inconclusive_nonnegative_phi &&
          result.receipt() == nullptr &&
          result.audit().certified_witness_point_count == 9U &&
          result.audit().nonnegative_witness_node_count == 1U &&
          result.audit().exact_phi_aabb_maximum_evaluation_count == 10U,
      "a nonnegative exact AABB maximum minted a block receipt");
}

void test_zero_exact_phi_is_inconclusive() {
  const CanonicalPointCloud cloud = make_zero_phi_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const PointId first_support_point = point_id_for_source_index(cloud, 0U);
  const PointId second_support_point = point_id_for_source_index(cloud, 1U);
  const PointId zero_witness_point = point_id_for_source_index(cloud, 11U);
  require(
      exact_diametral_phi_aabb_maximum_sign(
          point_box(cloud, first_support_point),
          point_box(cloud, second_support_point),
          point_box(cloud, zero_witness_point)) == 0,
      "the boundary fixture does not have exact M=0");

  const std::size_t first_support =
      leaf_node_for_point_id(index, cloud, first_support_point);
  const std::size_t second_support =
      leaf_node_for_point_id(index, cloud, second_support_point);
  std::vector<std::size_t> witnesses;
  witnesses.reserve(10U);
  for (std::size_t source_index = 2U;
       source_index < 11U;
       ++source_index) {
    witnesses.push_back(leaf_node_for_point_id(
        index,
        cloud,
        point_id_for_source_index(cloud, source_index)));
  }
  witnesses.push_back(
      leaf_node_for_point_id(index, cloud, zero_witness_point));

  const ExactBlockRankPruneResult result =
      certify_exact_block_rank_prune_receipt(
          index,
          cloud,
          first_support,
          second_support,
          witnesses,
          11U);
  require(
      result.status() ==
              ExactBlockRankPruneStatus::inconclusive_nonnegative_phi &&
          result.receipt() == nullptr &&
          result.audit().certified_witness_point_count == 9U &&
          result.audit().nonnegative_witness_node_count == 1U &&
          result.audit().exact_phi_aabb_maximum_evaluation_count == 10U,
      "an exact-zero AABB maximum minted a block receipt");
}

}  // namespace

int main() {
  try {
    test_k10_multileaf_success_mass_and_determinism();
    test_insufficient_mass_and_overlaps_are_separate();
    test_positive_exact_phi_is_inconclusive();
    test_zero_exact_phi_is_inconclusive();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
