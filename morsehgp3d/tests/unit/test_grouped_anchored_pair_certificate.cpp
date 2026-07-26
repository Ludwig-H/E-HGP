#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"
#include "morsehgp3d/hierarchy/symmetric_grouped_anchored_pair_prune_receipt.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
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
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneBudget;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneCertificate;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneCertificateOrigin;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneDecision;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneStopReason;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalContext;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalStep;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalStepKind;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalStopReason;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairTraversalWorkBudget;
using morsehgp3d::hierarchy::ExactSymmetricGroupedAnchoredPairPruneReceipt;
using morsehgp3d::hierarchy::certify_exact_grouped_anchored_pair_prune;
using morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum_sign;
using morsehgp3d::hierarchy::recertify_exact_symmetric_grouped_anchored_pair_prune;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_aggregate_v<ExactGroupedAnchoredPairPruneCertificate>);
static_assert(
    !std::is_default_constructible_v<
        ExactGroupedAnchoredPairPruneCertificate>);

template <class Certificate>
concept ExposesRvalueAudit = requires(Certificate&& certificate) {
  std::move(certificate).audit();
};

template <class Certificate>
concept ExposesRvalueWitnessSpan = requires(Certificate&& certificate) {
  std::move(certificate).certified_witness_point_ids();
};

template <class Index, class Cloud>
concept StartsGroupedTraversalAtRoot =
    requires(Index&& index, Cloud&& cloud) {
      ExactGroupedAnchoredPairTraversalContext::start_at_root(
          std::forward<Index>(index),
          std::forward<Cloud>(cloud),
          std::span<const PointId>{},
          std::span<const PointId>{},
          2U);
    };

template <class Index, class Cloud>
concept StartsGroupedTraversalAtNode =
    requires(Index&& index, Cloud&& cloud) {
      ExactGroupedAnchoredPairTraversalContext::start_at_node(
          std::forward<Index>(index),
          std::forward<Cloud>(cloud),
          std::span<const PointId>{},
          std::span<const PointId>{},
          0U,
          2U);
    };

static_assert(
    !ExposesRvalueAudit<ExactGroupedAnchoredPairPruneCertificate>);
static_assert(
    !ExposesRvalueWitnessSpan<ExactGroupedAnchoredPairPruneCertificate>);
static_assert(
    !std::is_aggregate_v<ExactGroupedAnchoredPairTraversalContext>);
static_assert(
    !std::is_default_constructible_v<
        ExactGroupedAnchoredPairTraversalContext>);
static_assert(
    !std::is_copy_constructible_v<
        ExactGroupedAnchoredPairTraversalContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactGroupedAnchoredPairTraversalContext>);
static_assert(
    !std::is_move_assignable_v<
        ExactGroupedAnchoredPairTraversalContext>);
static_assert(
    !std::is_constructible_v<
        ExactGroupedAnchoredPairTraversalContext,
        std::uint64_t>);
static_assert(
    !std::is_aggregate_v<ExactGroupedAnchoredPairTraversalStep>);
static_assert(
    !std::is_default_constructible_v<
        ExactGroupedAnchoredPairTraversalStep>);
static_assert(
    StartsGroupedTraversalAtRoot<
        MortonLbvhIndex&,
        CanonicalPointCloud&>);
static_assert(
    StartsGroupedTraversalAtNode<
        const MortonLbvhIndex&,
        const CanonicalPointCloud&>);
static_assert(
    !StartsGroupedTraversalAtRoot<
        MortonLbvhIndex,
        CanonicalPointCloud&>);
static_assert(
    !StartsGroupedTraversalAtRoot<
        MortonLbvhIndex&,
        CanonicalPointCloud>);
static_assert(
    !StartsGroupedTraversalAtNode<
        MortonLbvhIndex,
        CanonicalPointCloud&>);
static_assert(
    !StartsGroupedTraversalAtNode<
        MortonLbvhIndex&,
        CanonicalPointCloud>);

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <class Exception, class Callable>
void require_throws(Callable&& callable, const std::string& message) {
  bool threw = false;
  try {
    callable();
  } catch (const Exception&) {
    threw = true;
  }
  require(threw, message);
}

[[nodiscard]] std::uint64_t bits(double value) {
  return morsehgp3d::exact::canonicalize_binary64_bits(
      std::bit_cast<std::uint64_t>(value));
}

[[nodiscard]] ExactDyadicAabb3 box(
    const std::array<double, 3>& lower,
    const std::array<double, 3>& upper) {
  return ExactDyadicAabb3{
      {bits(lower[0]), bits(lower[1]), bits(lower[2])},
      {bits(upper[0]), bits(upper[1]), bits(upper[2])}};
}

[[nodiscard]] ExactDyadicAabb3 point_box(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

[[nodiscard]] CanonicalPointCloud make_cloud(
    std::span<const std::array<double, 3>> coordinates) {
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const std::array<double, 3>& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_line_cloud(std::size_t point_count) {
  std::vector<std::array<double, 3>> coordinates;
  coordinates.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    coordinates.push_back({static_cast<double>(index), 0.0, 0.0});
  }
  return make_cloud(coordinates);
}

[[nodiscard]] CanonicalPointCloud make_nonprefix_witness_cloud() {
  const std::array<std::array<double, 3>, 6> coordinates{
      std::array<double, 3>{0.0, 0.0, 0.0},
      std::array<double, 3>{1.0, 3.0, 0.0},
      std::array<double, 3>{2.0, 4.0, 0.0},
      std::array<double, 3>{3.0, 0.0, 0.0},
      std::array<double, 3>{10.0, 0.0, 0.0},
      std::array<double, 3>{10.0, 1.0, 0.0}};
  return make_cloud(coordinates);
}

[[nodiscard]] ExactGroupedAnchoredPairPruneBudget roomy_budget() {
  ExactGroupedAnchoredPairPruneBudget budget;
  budget.maximum_anchor_count = 32U;
  budget.maximum_witness_pool_entry_count = 64U;
  budget.maximum_exact_predicate_count = 64U;
  return budget;
}

struct LineFixture {
  CanonicalPointCloud cloud{make_line_cloud(12U)};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};
  std::array<PointId, 2> anchors{0U, 1U};
  std::array<PointId, 4> witness_pool{2U, 3U, 4U, 5U};
};

struct NonPrefixWitnessFixture {
  CanonicalPointCloud cloud{make_nonprefix_witness_cloud()};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};
  std::array<PointId, 1> anchors{0U};
  std::array<PointId, 3> witness_pool{1U, 2U, 3U};
};

struct FrontierFixture {
  CanonicalPointCloud cloud{make_line_cloud(4U)};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};
  std::array<PointId, 2> anchors{0U, 1U};
  std::array<PointId, 1> witness_pool{3U};
};

[[nodiscard]] ExactGroupedAnchoredPairPruneCertificate certify_node(
    const LineFixture& fixture,
    std::span<const PointId> witness_pool,
    std::size_t node_index,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget) {
  return certify_exact_grouped_anchored_pair_prune(
      fixture.index,
      fixture.cloud,
      fixture.anchors,
      witness_pool,
      node_index,
      maximum_closed_rank,
      budget);
}

[[nodiscard]] ExactGroupedAnchoredPairPruneCertificate certificate_for_leaf(
    const LineFixture& fixture,
    std::span<const PointId> witness_pool,
    PointId leaf_point_id,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget) {
  const ExactDyadicAabb3 expected_bounds =
      point_box(fixture.cloud, leaf_point_id);
  for (std::size_t node_index = 0U;
       node_index < fixture.index.build_counters().node_count;
       ++node_index) {
    ExactGroupedAnchoredPairPruneCertificate result = certify_node(
        fixture,
        witness_pool,
        node_index,
        maximum_closed_rank,
        budget);
    if (result.query_bounds() == expected_bounds &&
        result.leaf_end() - result.leaf_begin() == 1U) {
      return result;
    }
  }
  throw std::runtime_error("a fixture point has no certified LBVH leaf node");
}

[[nodiscard]] ExactGroupedAnchoredPairPruneCertificate certificate_for_leaf(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> anchors,
    std::span<const PointId> witness_pool,
    PointId leaf_point_id,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget) {
  const ExactDyadicAabb3 expected_bounds =
      point_box(cloud, leaf_point_id);
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    ExactGroupedAnchoredPairPruneCertificate result =
        certify_exact_grouped_anchored_pair_prune(
            index,
            cloud,
            anchors,
            witness_pool,
            node_index,
            maximum_closed_rank,
            budget);
    if (result.query_bounds() == expected_bounds &&
        result.leaf_end() - result.leaf_begin() == 1U) {
      return result;
    }
  }
  throw std::runtime_error("a fixture point has no certified LBVH leaf node");
}

[[nodiscard]] std::size_t find_node_by_range_and_bounds(
    const LineFixture& fixture,
    std::size_t leaf_begin,
    std::size_t leaf_end,
    const ExactDyadicAabb3& expected_bounds) {
  for (std::size_t node_index = 0U;
       node_index < fixture.index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate result = certify_node(
        fixture,
        fixture.witness_pool,
        node_index,
        4U,
        roomy_budget());
    if (result.leaf_begin() == leaf_begin &&
        result.leaf_end() == leaf_end &&
        result.query_bounds() == expected_bounds) {
      return node_index;
    }
  }
  throw std::runtime_error("the requested LBVH fixture node was not found");
}

void test_shared_certificate_and_provenance() {
  const LineFixture fixture;
  const ExactGroupedAnchoredPairPruneCertificate result =
      certificate_for_leaf(
          fixture,
          fixture.witness_pool,
          8U,
          4U,
          roomy_budget());
  const auto witnesses = result.certified_witness_point_ids();
  const auto& audit = result.audit();

  require(result.certified(), "three group witnesses were not certified");
  require(
      result.origin() ==
              ExactGroupedAnchoredPairPruneCertificateOrigin::single_node &&
          result.maximum_closed_rank() == 4U &&
          result.required_witness_count() == 3U &&
          witnesses.size() == 3U && witnesses[0] == 2U &&
          witnesses[1] == 3U && witnesses[2] == 4U &&
          result.certified_witness_pool_mask() == UINT64_C(0x7),
      "the grouped certificate did not publish its canonical strict prefix");
  require(
      audit.anchor_count == 2U &&
          audit.witness_pool_entry_count == 4U &&
          audit.exact_predicate_count == 6U &&
          audit.strict_group_witness_count == 3U &&
          audit.requested_budget_applies && audit.input_canonical &&
          audit.anchor_bounds_constructed &&
          audit.lbvh_node_authority_verified && audit.complete,
      "the grouped certificate audit is incomplete");
  require(
      result.anchor_bounds() ==
          box({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}),
      "the grouped anchor AABB is not the exact coordinate hull");
  require(
      result.validated_for(fixture.index, fixture.cloud) &&
          result.certifies(
              fixture.index,
              fixture.cloud,
              result.lbvh_node_index(),
              4U,
              fixture.anchors),
      "the certificate rejected its authentic cloud, node or anchor group");
  require(
      !result.certifies(
          fixture.index,
          fixture.cloud,
          result.lbvh_node_index() + 1U,
          4U,
          fixture.anchors),
      "the certificate was reusable for another LBVH node");
  require(
      !result.certifies(
          fixture.index,
          fixture.cloud,
          result.lbvh_node_index(),
          11U,
          fixture.anchors),
      "a rank-four certificate was reusable for rank eleven");
  const std::array<PointId, 2> different_anchors{0U, 2U};
  require(
      !result.certifies(
          fixture.index,
          fixture.cloud,
          result.lbvh_node_index(),
          4U,
          different_anchors),
      "the certificate was reusable for another anchor group");

  CanonicalPointCloud other_cloud = make_line_cloud(12U);
  MortonLbvhIndex other_index = MortonLbvhIndex::build(other_cloud);
  require(
      !result.validated_for(other_index, other_cloud) &&
          !result.certifies(
              other_index,
              other_cloud,
              result.lbvh_node_index(),
              4U,
              fixture.anchors),
      "the certificate crossed its process-local cloud/LBVH authority");

  for (const PointId anchor : fixture.anchors) {
    for (std::size_t witness_offset = 0U;
         witness_offset < witnesses.size();
         ++witness_offset) {
      require(
          exact_diametral_phi_aabb_maximum_sign(
              point_box(fixture.cloud, anchor),
              result.query_bounds(),
              point_box(fixture.cloud, witnesses[witness_offset])) < 0,
          "a published group witness is not strict for an actual anchor");
    }
  }
}

void test_inconclusive_results_publish_no_partial_authority() {
  const LineFixture fixture;
  const std::array<PointId, 3> partial_pool{2U, 3U, 8U};
  const ExactGroupedAnchoredPairPruneCertificate partial =
      certificate_for_leaf(
          fixture,
          partial_pool,
          8U,
          4U,
          roomy_budget());
  const auto& partial_audit = partial.audit();
  require(
      partial.decision() ==
              ExactGroupedAnchoredPairPruneDecision::inconclusive &&
          partial_audit.complete &&
          partial_audit.exact_predicate_count == 5U &&
          partial_audit.strict_group_witness_count == 2U &&
          partial.certified_witness_point_ids().empty() &&
          partial.certified_witness_pool_mask() == 0U,
      "an inconclusive node leaked partial witness authority");

  const std::array<PointId, 2> undersized_pool{2U, 3U};
  ExactGroupedAnchoredPairPruneBudget zero_work_budget = roomy_budget();
  zero_work_budget.maximum_exact_predicate_count = 0U;
  const ExactGroupedAnchoredPairPruneCertificate undersized =
      certificate_for_leaf(
          fixture,
          undersized_pool,
          8U,
          4U,
          zero_work_budget);
  require(
      undersized.decision() ==
              ExactGroupedAnchoredPairPruneDecision::inconclusive &&
          undersized.audit().complete &&
          undersized.audit().exact_predicate_count == 0U,
      "an undersized witness pool performed avoidable exact work");
}

void test_shell_equality_fails_open() {
  const LineFixture fixture;
  const std::array<PointId, 1> shell_pool{8U};
  const ExactGroupedAnchoredPairPruneCertificate equality =
      certificate_for_leaf(
          fixture,
          shell_pool,
          8U,
          2U,
          roomy_budget());
  require(
      equality.decision() ==
              ExactGroupedAnchoredPairPruneDecision::inconclusive &&
          equality.audit().exact_predicate_count == 1U &&
          equality.audit().strict_group_witness_count == 0U &&
          equality.certified_witness_point_ids().empty() &&
          equality.certified_witness_pool_mask() == 0U,
      "a distinct witness on the diametral shell certified a strict prune");
}

void test_discrete_common_certificate_closes_hybrid_corner_gap() {
  const ExactDyadicAabb3 anchor_bounds =
      box({-2.0, -2.0, 0.0}, {1.0, 1.0, 0.0});
  const ExactDyadicAabb3 query_point =
      box({1.0, 1.0, 0.0}, {1.0, 1.0, 0.0});
  const ExactDyadicAabb3 witness_point =
      box({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const std::array<ExactDyadicAabb3, 2> actual_anchors{
      box({1.0, -2.0, 0.0}, {1.0, -2.0, 0.0}),
      box({-2.0, 1.0, 0.0}, {-2.0, 1.0, 0.0})};
  for (const ExactDyadicAabb3& anchor : actual_anchors) {
    require(
        exact_diametral_phi_aabb_maximum_sign(
            anchor, query_point, witness_point) < 0,
        "the correlated fixture lacks its strict discrete witness");
  }
  require(
      exact_diametral_phi_aabb_maximum_sign(
          anchor_bounds, query_point, witness_point) >= 0,
      "the grouped AABB relaxation did not preserve its fail-open gap");

  const std::array<std::array<double, 3>, 4> coordinates{
      std::array<double, 3>{1.0, -2.0, 0.0},
      std::array<double, 3>{-2.0, 1.0, 0.0},
      std::array<double, 3>{0.0, 0.0, 0.0},
      std::array<double, 3>{1.0, 1.0, 0.0}};
  const CanonicalPointCloud cloud = make_cloud(coordinates);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2> anchors{0U, 2U};
  const std::array<PointId, 1> witnesses{1U};
  const PointId query_point_id = 3U;

  const ExactGroupedAnchoredPairPruneCertificate discrete =
      certificate_for_leaf(
          index,
          cloud,
          anchors,
          witnesses,
          query_point_id,
          2U,
          roomy_budget());
  require(
      discrete.certified() &&
          discrete.required_witness_count() == 1U &&
          discrete.certified_witness_point_ids().size() == 1U &&
          discrete.certified_witness_point_ids().front() == witnesses.front() &&
          discrete.certified_witness_pool_mask() == UINT64_C(0x1) &&
          discrete.audit().exact_predicate_count == 2U &&
          discrete.audit().strict_group_witness_count == 1U &&
          discrete.audit().complete &&
          discrete.certifies(
              index,
              cloud,
              discrete.lbvh_node_index(),
              2U,
              anchors),
      "the finite-anchor certificate did not close the hybrid-corner gap");

  ExactGroupedAnchoredPairPruneBudget one_predicate_budget = roomy_budget();
  one_predicate_budget.maximum_exact_predicate_count = 1U;
  const ExactGroupedAnchoredPairPruneCertificate exhausted =
      certificate_for_leaf(
          index,
          cloud,
          anchors,
          witnesses,
          query_point_id,
          2U,
          one_predicate_budget);
  require(
      exhausted.decision() ==
              ExactGroupedAnchoredPairPruneDecision::budget_exhausted &&
          exhausted.stop_reason() ==
              ExactGroupedAnchoredPairPruneStopReason::exact_predicate_limit &&
          exhausted.audit().exact_predicate_count == 1U &&
          exhausted.audit().strict_group_witness_count == 0U &&
          !exhausted.audit().complete &&
          exhausted.certified_witness_point_ids().empty() &&
          exhausted.certified_witness_pool_mask() == 0U,
      "the anchor-witness predicate cap leaked partial common authority");
}

void test_budgets_fail_atomically() {
  const LineFixture fixture;
  const ExactGroupedAnchoredPairPruneCertificate baseline =
      certificate_for_leaf(
          fixture,
          fixture.witness_pool,
          8U,
          4U,
          roomy_budget());
  require(baseline.certified(), "the budget fixture baseline did not certify");

  ExactGroupedAnchoredPairPruneBudget budget = roomy_budget();
  budget.maximum_anchor_count = 1U;
  const ExactGroupedAnchoredPairPruneCertificate anchors_exhausted =
      certify_node(
          fixture,
          fixture.witness_pool,
          baseline.lbvh_node_index(),
          4U,
          budget);
  require(
      anchors_exhausted.decision() ==
              ExactGroupedAnchoredPairPruneDecision::budget_exhausted &&
          anchors_exhausted.stop_reason() ==
              ExactGroupedAnchoredPairPruneStopReason::anchor_count_limit &&
          anchors_exhausted.certified_witness_point_ids().empty() &&
          anchors_exhausted.anchor_point_ids().empty() &&
          !anchors_exhausted.audit().input_canonical &&
          !anchors_exhausted.audit().anchor_bounds_constructed &&
          !anchors_exhausted.audit().complete,
      "the anchor cap did not fail open atomically");

  budget = roomy_budget();
  budget.maximum_witness_pool_entry_count = 3U;
  const ExactGroupedAnchoredPairPruneCertificate pool_exhausted =
      certify_node(
          fixture,
          fixture.witness_pool,
          baseline.lbvh_node_index(),
          4U,
          budget);
  require(
      pool_exhausted.stop_reason() ==
              ExactGroupedAnchoredPairPruneStopReason::
                  witness_pool_entry_limit &&
          pool_exhausted.anchor_point_ids().empty() &&
          pool_exhausted.witness_pool_point_ids().empty() &&
          pool_exhausted.certified_witness_point_ids().empty() &&
          !pool_exhausted.audit().input_canonical,
      "the witness-pool cap did not fail open");

  budget = roomy_budget();
  budget.maximum_exact_predicate_count = 2U;
  const ExactGroupedAnchoredPairPruneCertificate predicate_exhausted =
      certify_node(
          fixture,
          fixture.witness_pool,
          baseline.lbvh_node_index(),
          4U,
          budget);
  require(
      predicate_exhausted.stop_reason() ==
              ExactGroupedAnchoredPairPruneStopReason::
                  exact_predicate_limit &&
          predicate_exhausted.audit().exact_predicate_count == 2U &&
          predicate_exhausted.audit().strict_group_witness_count == 1U &&
          predicate_exhausted.certified_witness_point_ids().empty() &&
          predicate_exhausted.certified_witness_pool_mask() == 0U,
      "the exact-predicate cap leaked a partial certificate");
}

void test_structural_validation() {
  const LineFixture fixture;
  const ExactGroupedAnchoredPairPruneBudget budget = roomy_budget();
  const std::size_t node_index = 0U;

  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(certify_exact_grouped_anchored_pair_prune(
            fixture.index,
            fixture.cloud,
            std::span<const PointId>{},
            fixture.witness_pool,
            node_index,
            4U,
            budget));
      },
      "an empty anchor group was accepted");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(certify_node(
            fixture,
            fixture.witness_pool,
            node_index,
            1U,
            budget));
      },
      "a maximum closed rank below two was accepted");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(certify_node(
            fixture,
            fixture.witness_pool,
            node_index,
            12U,
            budget));
      },
      "a maximum closed rank above eleven was accepted");
  require_throws<std::out_of_range>(
      [&] {
        static_cast<void>(certify_node(
            fixture,
            fixture.witness_pool,
            fixture.index.build_counters().node_count,
            4U,
            budget));
      },
      "an invalid LBVH node was accepted");

  const std::array<PointId, 2> reversed_anchors{1U, 0U};
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(certify_exact_grouped_anchored_pair_prune(
            fixture.index,
            fixture.cloud,
            reversed_anchors,
            fixture.witness_pool,
            node_index,
            4U,
            budget));
      },
      "a noncanonical anchor group was accepted");

  const std::array<PointId, 3> duplicate_pool{2U, 2U, 3U};
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(certify_node(
            fixture,
            duplicate_pool,
            node_index,
            4U,
            budget));
      },
      "a duplicate witness-pool entry was accepted");

  const std::array<PointId, 3> own_anchor_pool{0U, 2U, 3U};
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(certify_node(
            fixture,
            own_anchor_pool,
            node_index,
            4U,
            budget));
      },
      "a witness pool containing an anchor was accepted");
}

void test_prepared_inherited_traversal_differential() {
  const LineFixture fixture;
  const ExactDyadicAabb3 target_bounds =
      box({3.0, 0.0, 0.0}, {5.0, 0.0, 0.0});
  const std::size_t target_node_index = find_node_by_range_and_bounds(
      fixture, 3U, 6U, target_bounds);

  std::size_t fresh_node_count = 0U;
  std::size_t fresh_predicate_count = 0U;
  std::size_t fresh_prune_count = 0U;
  std::optional<std::size_t> fresh_pruned_node_index;
  for (std::size_t node_index = 0U;
       node_index < fixture.index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate fresh = certify_node(
        fixture,
        fixture.witness_pool,
        node_index,
        4U,
        roomy_budget());
    if (fresh.leaf_begin() < 3U || fresh.leaf_end() > 6U) {
      continue;
    }
    ++fresh_node_count;
    fresh_predicate_count += fresh.audit().exact_predicate_count;
    if (fresh.certified()) {
      ++fresh_prune_count;
      fresh_pruned_node_index = node_index;
    }
  }
  require(
      fresh_node_count == 5U && fresh_predicate_count == 27U &&
          fresh_prune_count == 1U &&
          fresh_pruned_node_index.has_value(),
      "the fresh P8g subtree oracle lost its discriminating work profile");

  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::start_at_node(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          target_node_index,
          4U);
  require(
      traversal.ready() && !traversal.complete() &&
          traversal.validated_for(fixture.index, fixture.cloud) &&
          traversal.start_node_index() == target_node_index &&
          traversal.maximum_closed_rank() == 4U &&
          std::equal(
              traversal.anchor_point_ids().begin(),
              traversal.anchor_point_ids().end(),
              fixture.anchors.begin(),
              fixture.anchors.end()) &&
          std::equal(
              traversal.witness_pool_point_ids().begin(),
              traversal.witness_pool_point_ids().end(),
              fixture.witness_pool.begin(),
              fixture.witness_pool.end()),
      "the prepared grouped traversal lost its fixed authority");

  const ExactGroupedAnchoredPairTraversalWorkBudget roomy{64U, 64U};
  std::array<ExactGroupedAnchoredPairTraversalStepKind, 3> kinds{};
  std::array<PointId, 2> unresolved_points{};
  std::size_t step_count = 0U;
  std::size_t unresolved_count = 0U;
  std::optional<std::size_t> inherited_pruned_node_index;
  while (!traversal.complete()) {
    const ExactGroupedAnchoredPairTraversalStep step =
        traversal.advance(fixture.index, fixture.cloud, roomy);
    require(
        step.kind() !=
            ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted,
        "a roomy prepared traversal exhausted its work budget");
    require(step_count < kinds.size(), "the traversal emitted extra records");
    kinds[step_count] = step.kind();
    ++step_count;
    if (step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::certified_prune) {
      const ExactGroupedAnchoredPairPruneCertificate* certificate =
          step.prune_certificate();
      require(
          certificate != nullptr && step.lbvh_node_index().has_value() &&
              certificate->origin() ==
                  ExactGroupedAnchoredPairPruneCertificateOrigin::
                      prepared_inherited_traversal &&
              !certificate->audit().requested_budget_applies &&
              certificate->audit().inherited_strict_group_witness_count ==
                  1U &&
              certificate->audit().exact_predicate_count == 4U &&
              certificate->certifies(
                  fixture.index,
                  fixture.cloud,
                  *step.lbvh_node_index(),
                  4U,
                  fixture.anchors),
          "the inherited traversal emitted a nonauthoritative prune");
      inherited_pruned_node_index = step.lbvh_node_index();
    } else if (
        step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf) {
      require(
          step.unresolved_point_id().has_value() &&
              unresolved_count < unresolved_points.size(),
          "an unresolved leaf lost its bounded point payload");
      unresolved_points[unresolved_count] = *step.unresolved_point_id();
      ++unresolved_count;
    }
  }

  require(
      kinds ==
              std::array<ExactGroupedAnchoredPairTraversalStepKind, 3>{
                  ExactGroupedAnchoredPairTraversalStepKind::certified_prune,
                  ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf,
                  ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf} &&
          unresolved_points == std::array<PointId, 2>{4U, 3U} &&
          inherited_pruned_node_index == fresh_pruned_node_index,
      "the inherited traversal changed the fresh oracle terminal partition");

  const auto& audit = traversal.audit();
  require(
      audit.anchor_bounds_construction_count == 1U &&
          audit.prepared_witness_point_count == 4U &&
          audit.node_visit_count == 5U &&
          audit.exact_predicate_count == 19U &&
          audit.inherited_witness_reuse_count == 4U &&
          audit.witness_slot_scan_count == 19U &&
          fresh_predicate_count ==
              audit.exact_predicate_count +
                  fixture.anchors.size() *
                      audit.inherited_witness_reuse_count &&
          audit.strict_witness_discovery_count == 4U &&
          audit.internal_node_expansion_count == 2U &&
          audit.certified_prune_count == 1U &&
          audit.unresolved_leaf_count == 2U &&
          audit.maximum_pending_node_count == 2U && audit.complete &&
          audit.no_dynamic_traversal_or_output_arena,
      "the inherited traversal audit does not close 27 = 19 + 2*4");

  const ExactGroupedAnchoredPairTraversalStep complete =
      traversal.advance(fixture.index, fixture.cloud, roomy);
  require(
      complete.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::complete &&
          complete.work() ==
              morsehgp3d::hierarchy::
                  ExactGroupedAnchoredPairTraversalStepWork{} &&
          complete.traversal_complete_after_step(),
      "a terminal grouped traversal did not remain terminal");
}

void test_prepared_inheritance_preserves_nonprefix_canonical_order() {
  const NonPrefixWitnessFixture fixture;
  const ExactDyadicAabb3 target_bounds =
      box({10.0, 0.0, 0.0}, {10.0, 1.0, 0.0});
  std::optional<std::size_t> target_node_index;
  std::size_t target_leaf_begin = 0U;
  std::size_t target_leaf_end = 0U;
  for (std::size_t node_index = 0U;
       node_index < fixture.index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate fresh =
        certify_exact_grouped_anchored_pair_prune(
            fixture.index,
            fixture.cloud,
            fixture.anchors,
            fixture.witness_pool,
            node_index,
            3U,
            roomy_budget());
    if (fresh.leaf_end() == fresh.leaf_begin() + 2U &&
        fresh.query_bounds() == target_bounds) {
      target_node_index = node_index;
      target_leaf_begin = fresh.leaf_begin();
      target_leaf_end = fresh.leaf_end();
      break;
    }
  }
  require(
      target_node_index.has_value(),
      "the nonprefix inheritance fixture lost its target subtree");

  std::size_t fresh_node_count = 0U;
  std::size_t fresh_predicate_count = 0U;
  std::size_t fresh_prune_count = 0U;
  bool saw_fresh_q1_prune = false;
  for (std::size_t node_index = 0U;
       node_index < fixture.index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate fresh =
        certify_exact_grouped_anchored_pair_prune(
            fixture.index,
            fixture.cloud,
            fixture.anchors,
            fixture.witness_pool,
            node_index,
            3U,
            roomy_budget());
    if (fresh.leaf_begin() < target_leaf_begin ||
        fresh.leaf_end() > target_leaf_end) {
      continue;
    }
    ++fresh_node_count;
    fresh_predicate_count += fresh.audit().exact_predicate_count;
    if (node_index == *target_node_index) {
      require(
          fresh.decision() ==
                  ExactGroupedAnchoredPairPruneDecision::inconclusive &&
              fresh.audit().exact_predicate_count == 3U &&
              fresh.audit().strict_group_witness_count == 1U &&
              exact_diametral_phi_aabb_maximum_sign(
                  fresh.anchor_bounds(),
                  fresh.query_bounds(),
                  point_box(fixture.cloud, PointId{1U})) == 0 &&
              exact_diametral_phi_aabb_maximum_sign(
                  fresh.anchor_bounds(),
                  fresh.query_bounds(),
                  point_box(fixture.cloud, PointId{2U})) == 0 &&
              exact_diametral_phi_aabb_maximum_sign(
                  fresh.anchor_bounds(),
                  fresh.query_bounds(),
                  point_box(fixture.cloud, PointId{3U})) < 0,
          "the nonprefix parent lost its strict-late and exact-shell profile");
    }
    if (!fresh.certified()) {
      continue;
    }

    ++fresh_prune_count;
    require(
        fresh.leaf_end() == fresh.leaf_begin() + 1U,
        "the nonprefix fresh oracle unexpectedly pruned an internal node");
    const PointId pruned_point =
        fixture.index.leaves()[fresh.leaf_begin()].point_id;
    const auto witnesses = fresh.certified_witness_point_ids();
    require(
        witnesses.size() == 2U,
        "the rank-three fresh oracle did not publish two witnesses");
    if (pruned_point == PointId{5U}) {
      const std::array<PointId, 2> expected{1U, 2U};
      require(
          std::equal(
              witnesses.begin(), witnesses.end(), expected.begin()),
          "the fresh q1 prune lost its canonical witness prefix");
      saw_fresh_q1_prune = true;
    } else {
      require(false, "the nonprefix fresh oracle pruned an unexpected leaf");
    }
  }
  require(
      fresh_node_count == 3U && fresh_predicate_count == 8U &&
          fresh_prune_count == 1U && saw_fresh_q1_prune,
      "the nonprefix fresh oracle lost its 8-predicate terminal partition");

  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::start_at_node(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          *target_node_index,
          3U);
  const ExactGroupedAnchoredPairTraversalWorkBudget roomy{64U, 64U};
  std::array<ExactGroupedAnchoredPairTraversalStepKind, 2> kinds{};
  std::optional<PointId> pruned_point;
  std::array<PointId, 2> inherited_witnesses{};
  std::optional<PointId> unresolved_point;
  std::size_t step_count = 0U;
  std::size_t prune_count = 0U;
  while (!traversal.complete()) {
    const ExactGroupedAnchoredPairTraversalStep step =
        traversal.advance(fixture.index, fixture.cloud, roomy);
    require(
        step.kind() !=
            ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted,
        "the roomy nonprefix traversal exhausted its work budget");
    require(
        step_count < kinds.size(),
        "the nonprefix traversal emitted extra terminal records");
    kinds[step_count] = step.kind();
    ++step_count;

    if (step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::certified_prune) {
      require(
          prune_count == 0U &&
              step.lbvh_node_index().has_value() &&
              step.leaf_begin().has_value() && step.leaf_end().has_value() &&
              *step.leaf_end() == *step.leaf_begin() + 1U,
          "a nonprefix prune lost its authenticated singleton range");
      const ExactGroupedAnchoredPairPruneCertificate* certificate =
          step.prune_certificate();
      require(
          certificate != nullptr &&
              certificate->certifies(
                  fixture.index,
                  fixture.cloud,
                  *step.lbvh_node_index(),
                  3U,
                  fixture.anchors),
          "a nonprefix inherited prune lost its rank or group provenance");
      const auto witnesses = certificate->certified_witness_point_ids();
      require(
          witnesses.size() == 2U,
          "a nonprefix inherited prune did not publish two witnesses");
      require(
          certificate->audit().inherited_strict_group_witness_count == 0U &&
              certificate->audit().exact_predicate_count == 2U,
          "an unseen late parent witness shortened the canonical q1 prefix");
      pruned_point =
          fixture.index.leaves()[*step.leaf_begin()].point_id;
      inherited_witnesses = {witnesses[0], witnesses[1]};
      ++prune_count;
    } else if (
        step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf) {
      require(
          step.unresolved_point_id().has_value(),
          "the nonprefix traversal lost its unresolved leaf");
      unresolved_point = step.unresolved_point_id();
    } else {
      require(false, "the nonprefix traversal emitted an unexpected record");
    }
  }

  require(
      kinds ==
              std::array<ExactGroupedAnchoredPairTraversalStepKind, 2>{
                  ExactGroupedAnchoredPairTraversalStepKind::certified_prune,
                  ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf} &&
          prune_count == 1U && pruned_point == std::optional<PointId>{5U} &&
          inherited_witnesses == std::array<PointId, 2>{1U, 2U} &&
          unresolved_point == std::optional<PointId>{4U},
      "nonprefix inheritance changed the inverse-postorder oracle records");

  const auto& audit = traversal.audit();
  require(
      audit.anchor_bounds_construction_count == 1U &&
          audit.prepared_witness_point_count == 3U &&
          audit.node_visit_count == 3U &&
          audit.exact_predicate_count == 7U &&
          audit.inherited_witness_reuse_count == 1U &&
          audit.witness_slot_scan_count == 8U &&
          fresh_predicate_count ==
              audit.exact_predicate_count +
                  audit.inherited_witness_reuse_count &&
          audit.strict_witness_discovery_count == 3U &&
          audit.internal_node_expansion_count == 1U &&
          audit.certified_prune_count == 1U &&
          audit.unresolved_leaf_count == 1U && audit.complete,
      "nonprefix inheritance does not close the exact identity 8 = 7 + 1");
}

void test_prepared_traversal_resumes_atomically() {
  const LineFixture fixture;
  const std::size_t target_node_index = find_node_by_range_and_bounds(
      fixture,
      3U,
      6U,
      box({3.0, 0.0, 0.0}, {5.0, 0.0, 0.0}));
  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::start_at_node(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          target_node_index,
          4U);

  const ExactGroupedAnchoredPairTraversalStep first = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U});
  require(
      first.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted &&
          first.stop_reason() ==
              ExactGroupedAnchoredPairTraversalStopReason::
                  exact_predicate_limit &&
          first.work().node_visit_count == 1U &&
          first.work().witness_slot_scan_count == 0U &&
          first.work().exact_predicate_count == 1U &&
          first.prune_certificate() == nullptr,
      "the first traversal chunk did not stop within one physical witness test");

  const ExactGroupedAnchoredPairTraversalStep second = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{0U, 4U});
  require(
      second.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted &&
          second.stop_reason() ==
              ExactGroupedAnchoredPairTraversalStopReason::node_visit_limit &&
          second.work().node_visit_count == 0U &&
          second.work().witness_slot_scan_count == 4U &&
          second.work().exact_predicate_count == 4U &&
          second.work().inherited_witness_reuse_count == 0U &&
          second.prune_certificate() == nullptr,
      "the second traversal chunk did not finish the active node atomically");

  const ExactGroupedAnchoredPairTraversalStep third = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 4U});
  require(
      third.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::certified_prune &&
          third.work().node_visit_count == 1U &&
          third.work().exact_predicate_count == 4U &&
          third.work().inherited_witness_reuse_count == 1U &&
          third.prune_certificate() != nullptr,
      "a resumed active node was recharged or lost its strict prefix");

  const ExactGroupedAnchoredPairTraversalStep fourth = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{3U, 7U});
  require(
      fourth.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf &&
          fourth.unresolved_point_id() == std::optional<PointId>{4U} &&
          fourth.work().node_visit_count == 2U &&
          fourth.work().exact_predicate_count == 7U &&
          fourth.work().inherited_witness_reuse_count == 2U,
      "the resumed traversal changed its right-first sibling order");

  const ExactGroupedAnchoredPairTraversalStep fifth = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{1U, 3U});
  require(
      fifth.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf &&
          fifth.unresolved_point_id() == std::optional<PointId>{3U} &&
          fifth.work().node_visit_count == 1U &&
          fifth.work().exact_predicate_count == 3U &&
          fifth.work().inherited_witness_reuse_count == 1U &&
          fifth.traversal_complete_after_step() && traversal.complete(),
      "the final resumed traversal chunk did not close its subtree");

  const auto& audit = traversal.audit();
  require(
      audit.node_visit_count == 5U &&
          audit.exact_predicate_count == 19U &&
          audit.inherited_witness_reuse_count == 4U &&
          audit.certified_prune_count == 1U &&
          audit.unresolved_leaf_count == 2U &&
          audit.budget_exhaustion_count == 2U && audit.complete,
      "segmentation changed the grouped traversal totals");
}

void test_prepared_traversal_fallback_and_provenance() {
  const LineFixture fixture;
  const std::array<PointId, 2> undersized_pool{2U, 3U};
  ExactGroupedAnchoredPairTraversalContext fallback =
      ExactGroupedAnchoredPairTraversalContext::start_at_root(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          undersized_pool,
          4U);
  const ExactGroupedAnchoredPairTraversalStep fallback_step =
      fallback.advance(
          fixture.index,
          fixture.cloud,
          ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  require(
      fallback_step.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::fallback_subtree &&
          fallback_step.work().node_visit_count == 1U &&
          fallback_step.work().exact_predicate_count == 0U &&
          fallback_step.prune_certificate() == nullptr &&
          fallback_step.lbvh_node_index().has_value() &&
          fallback_step.traversal_complete_after_step() &&
          fallback.audit().fallback_subtree_count == 1U &&
          fallback.complete(),
      "an undersized grouped pool traversed leaves instead of falling back");

  ExactGroupedAnchoredPairTraversalContext original =
      ExactGroupedAnchoredPairTraversalContext::start_at_root(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          4U);
  ExactGroupedAnchoredPairTraversalContext moved{std::move(original)};
  require(
      !original.ready() && moved.ready(),
      "moving a grouped traversal did not revoke its source");
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(original.advance(
            fixture.index,
            fixture.cloud,
            ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U}));
      },
      "a moved-from grouped traversal remained usable");

  CanonicalPointCloud other_cloud = make_line_cloud(12U);
  MortonLbvhIndex other_index = MortonLbvhIndex::build(other_cloud);
  const std::size_t calls_before = moved.audit().advance_call_count;
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(moved.advance(
            other_index,
            other_cloud,
            ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U}));
      },
      "a grouped traversal accepted a geometrically equal foreign authority");
  require(
      moved.audit().advance_call_count == calls_before &&
          !moved.validated_for(other_index, other_cloud),
      "a foreign authority mutated the grouped traversal cursor");
}

void test_frontier_descends_diagonal_without_signs() {
  const FrontierFixture fixture;
  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          2U);

  const ExactGroupedAnchoredPairTraversalStep at_frontier = traversal.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{64U, 0U});
  const auto& diagonal_audit = traversal.audit();
  require(
      at_frontier.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted &&
          at_frontier.stop_reason() ==
              ExactGroupedAnchoredPairTraversalStopReason::
                  exact_predicate_limit &&
          at_frontier.work().node_visit_count > 1U &&
          at_frontier.work().witness_slot_scan_count == 0U &&
          at_frontier.work().exact_predicate_count == 0U &&
          at_frontier.work().strict_witness_discovery_count == 0U &&
          at_frontier.prune_certificate() == nullptr &&
          diagonal_audit.diagonal_node_descent_count > 0U &&
          diagonal_audit.node_visit_count ==
              diagonal_audit.diagonal_node_descent_count + 1U &&
          diagonal_audit.internal_node_expansion_count ==
              diagonal_audit.diagonal_node_descent_count &&
          diagonal_audit.exact_predicate_count == 0U &&
          diagonal_audit.inconclusive_subtree_count == 0U &&
          !traversal.complete(),
      "the grouped frontier charged a sign before leaving the diagonal");
}

void test_frontier_inconclusive_subtree_resumes_stably() {
  const FrontierFixture fixture;
  ExactGroupedAnchoredPairTraversalContext roomy =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          2U);
  const ExactGroupedAnchoredPairTraversalStep roomy_frontier = roomy.advance(
      fixture.index,
      fixture.cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{64U, 64U});
  require(
      roomy_frontier.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::
                  inconclusive_subtree &&
          roomy_frontier.prune_certificate() == nullptr &&
          !roomy_frontier.unresolved_point_id().has_value() &&
          roomy_frontier.lbvh_node_index().has_value() &&
          roomy_frontier.leaf_begin().has_value() &&
          roomy_frontier.leaf_end().has_value() &&
          roomy_frontier.work().exact_predicate_count == 1U &&
          roomy.audit().certified_prune_count == 0U &&
          roomy.audit().inconclusive_subtree_count == 1U,
      "the roomy grouped frontier published authority for an inconclusive subtree");

  ExactGroupedAnchoredPairTraversalContext node_probe =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_node(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          *roomy_frontier.lbvh_node_index(),
          2U);
  const ExactGroupedAnchoredPairTraversalStep node_probe_frontier =
      node_probe.advance(
          fixture.index,
          fixture.cloud,
          ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U});
  require(
      node_probe_frontier.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::
                  inconclusive_subtree &&
          node_probe_frontier.lbvh_node_index() ==
              roomy_frontier.lbvh_node_index() &&
          node_probe_frontier.leaf_begin() == roomy_frontier.leaf_begin() &&
          node_probe_frontier.leaf_end() == roomy_frontier.leaf_end() &&
          node_probe_frontier.work().node_visit_count == 1U &&
          node_probe_frontier.work().exact_predicate_count == 1U &&
          node_probe.complete(),
      "the bounded node frontier probe expanded an inconclusive subtree");

  ExactGroupedAnchoredPairTraversalContext segmented =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
          fixture.index,
          fixture.cloud,
          fixture.anchors,
          fixture.witness_pool,
          2U);
  std::size_t budget_stop_count = 0U;
  std::optional<std::size_t> terminal_node_index;
  std::optional<std::size_t> terminal_leaf_begin;
  std::optional<std::size_t> terminal_leaf_end;
  for (std::size_t call = 0U; call < 16U; ++call) {
    const ExactGroupedAnchoredPairTraversalStep step = segmented.advance(
        fixture.index,
        fixture.cloud,
        ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U});
    require(
        step.prune_certificate() == nullptr,
        "a segmented grouped frontier leaked a prune certificate");
    if (step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted) {
      ++budget_stop_count;
      continue;
    }
    require(
        step.kind() ==
                ExactGroupedAnchoredPairTraversalStepKind::
                    inconclusive_subtree &&
            !step.unresolved_point_id().has_value(),
        "the segmented grouped frontier changed its terminal kind");
    terminal_node_index = step.lbvh_node_index();
    terminal_leaf_begin = step.leaf_begin();
    terminal_leaf_end = step.leaf_end();
    break;
  }

  require(
      budget_stop_count > 0U &&
          terminal_node_index == roomy_frontier.lbvh_node_index() &&
          terminal_leaf_begin == roomy_frontier.leaf_begin() &&
          terminal_leaf_end == roomy_frontier.leaf_end(),
      "unit budgets changed the grouped frontier work or terminal authority");

  const auto& roomy_audit = roomy.audit();
  const auto& segmented_audit = segmented.audit();
  require(
      segmented_audit.node_visit_count == roomy_audit.node_visit_count &&
          segmented_audit.witness_slot_scan_count ==
              roomy_audit.witness_slot_scan_count &&
          segmented_audit.inherited_witness_reuse_count ==
              roomy_audit.inherited_witness_reuse_count &&
          segmented_audit.exact_predicate_count ==
              roomy_audit.exact_predicate_count &&
          segmented_audit.internal_node_expansion_count ==
              roomy_audit.internal_node_expansion_count &&
          segmented_audit.diagonal_node_descent_count ==
              roomy_audit.diagonal_node_descent_count &&
          segmented_audit.certified_prune_count == 0U &&
          segmented_audit.inconclusive_subtree_count == 1U &&
          segmented_audit.budget_exhaustion_count == budget_stop_count,
      "unit-budget resumption changed the grouped frontier cumulative audit");
}

void test_bounded_witness_subtree_proposal_replays_through_p8g() {
  CanonicalPointCloud cloud = make_line_cloud(32U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 1> anchors{0U};
  const std::array<PointId, 3> nonstrict_halo{29U, 30U, 31U};

  ExactGroupedAnchoredPairTraversalContext frontier =
      ExactGroupedAnchoredPairTraversalContext::start_frontier_at_root(
          index, cloud, anchors, nonstrict_halo, 4U);
  const ExactGroupedAnchoredPairTraversalStep frontier_step =
      frontier.advance(
          index,
          cloud,
          ExactGroupedAnchoredPairTraversalWorkBudget{128U, 128U});
  require(
      frontier_step.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::
                  inconclusive_subtree &&
          frontier_step.lbvh_node_index().has_value(),
      "the witness-subtree fixture did not expose an off-diagonal frontier");

  ExactGroupedAnchoredPairTraversalContext roomy =
      ExactGroupedAnchoredPairTraversalContext::
          start_witness_subtree_first_frontier_at_node(
              index,
              cloud,
              anchors,
              nonstrict_halo,
              *frontier_step.lbvh_node_index(),
              4U);
  const ExactGroupedAnchoredPairTraversalStep roomy_step = roomy.advance(
      index,
      cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{256U, 256U});
  const ExactGroupedAnchoredPairPruneCertificate* roomy_certificate =
      roomy_step.prune_certificate();
  require(
      roomy_step.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::certified_prune &&
          roomy_certificate != nullptr && roomy_certificate->certifies(
              index,
              cloud,
              *frontier_step.lbvh_node_index(),
              4U,
              anchors) &&
          roomy_certificate->certified_witness_point_ids().size() == 3U &&
          roomy.audit().witness_subtree_success_count == 1U &&
          roomy.audit().witness_subtree_fail_open_count == 0U &&
          roomy.audit().witness_subtree_receipt_count > 0U &&
          roomy.audit().witness_subtree_receipt_count <= 3U &&
          roomy.audit().witness_subtree_node_visit_count > 0U &&
          roomy.audit().witness_subtree_node_visit_count <=
              morsehgp3d::hierarchy::
                  exact_grouped_anchored_pair_maximum_witness_subtree_node_visit_count &&
          roomy_step.work().witness_subtree_node_visit_count ==
              roomy.audit().witness_subtree_node_visit_count &&
          roomy_step.work().witness_subtree_exact_predicate_count ==
              roomy.audit().witness_subtree_exact_predicate_count &&
          roomy_step.work().exact_predicate_count >
              roomy_step.work().witness_subtree_exact_predicate_count,
      "the bounded witness-subtree proposal bypassed exact P8g replay");

  const ExactGroupedAnchoredPairPruneCertificate replay =
      certify_exact_grouped_anchored_pair_prune(
          index,
          cloud,
          anchors,
          roomy_certificate->witness_pool_point_ids(),
          *frontier_step.lbvh_node_index(),
          4U,
          ExactGroupedAnchoredPairPruneBudget{1U, 3U, 3U});
  require(
      replay.certifies(
          index,
          cloud,
          *frontier_step.lbvh_node_index(),
          4U,
          anchors),
      "the witness-subtree proposal could not be replayed by P8g");

  ExactGroupedAnchoredPairTraversalContext segmented =
      ExactGroupedAnchoredPairTraversalContext::
          start_witness_subtree_first_frontier_at_node(
              index,
              cloud,
              anchors,
              nonstrict_halo,
              *frontier_step.lbvh_node_index(),
              4U);
  std::vector<PointId> segmented_witnesses;
  std::size_t exhaustion_count = 0U;
  for (std::size_t call = 0U; call < 256U; ++call) {
    const ExactGroupedAnchoredPairTraversalStep step = segmented.advance(
        index,
        cloud,
        ExactGroupedAnchoredPairTraversalWorkBudget{1U, 1U});
    if (step.kind() ==
        ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted) {
      ++exhaustion_count;
      continue;
    }
    require(
        step.kind() ==
                ExactGroupedAnchoredPairTraversalStepKind::certified_prune &&
            step.prune_certificate() != nullptr,
        "unit budgets changed the witness-subtree terminal decision");
    segmented_witnesses.assign(
        step.prune_certificate()->certified_witness_point_ids().begin(),
        step.prune_certificate()->certified_witness_point_ids().end());
    break;
  }
  require(
      exhaustion_count > 0U &&
          segmented_witnesses ==
              std::vector<PointId>{
                  roomy_certificate->certified_witness_point_ids().begin(),
                  roomy_certificate->certified_witness_point_ids().end()} &&
          segmented.audit().witness_subtree_node_visit_count ==
              roomy.audit().witness_subtree_node_visit_count &&
          segmented.audit().witness_subtree_exact_predicate_count ==
              roomy.audit().witness_subtree_exact_predicate_count,
      "unit budgets changed the bounded witness-subtree proposal");
}

void test_bounded_witness_subtree_cap_fails_open() {
  CanonicalPointCloud cloud = make_line_cloud(128U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 1> anchors{0U};
  const std::array<PointId, 1> nonstrict_halo{127U};
  const ExactGroupedAnchoredPairPruneCertificate query_leaf =
      certificate_for_leaf(
          index,
          cloud,
          anchors,
          nonstrict_halo,
          1U,
          2U,
          ExactGroupedAnchoredPairPruneBudget{1U, 1U, 1U});
  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::
          start_witness_subtree_first_at_node(
              index,
              cloud,
              anchors,
              nonstrict_halo,
              query_leaf.lbvh_node_index(),
              2U);
  const ExactGroupedAnchoredPairTraversalStep step = traversal.advance(
      index,
      cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{256U, 256U});
  require(
      step.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::unresolved_leaf &&
          step.prune_certificate() == nullptr &&
          traversal.audit().witness_subtree_success_count == 0U &&
          traversal.audit().witness_subtree_fail_open_count == 1U &&
          traversal.audit().witness_subtree_receipt_count == 0U &&
          traversal.audit().witness_subtree_node_visit_count ==
              morsehgp3d::hierarchy::
                  exact_grouped_anchored_pair_maximum_witness_subtree_node_visit_count &&
          traversal.complete(),
      "the bounded witness-subtree cap published partial authority");
}

void test_witness_subtree_search_restarts_and_restores_halo_per_query_node() {
  CanonicalPointCloud cloud = make_line_cloud(32U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 1> anchors{0U};
  const std::array<PointId, 3> halo{29U, 30U, 31U};
  std::size_t root_node_index = index.build_counters().node_count;
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate probe =
        certify_exact_grouped_anchored_pair_prune(
            index,
            cloud,
            anchors,
            halo,
            node_index,
            4U,
            ExactGroupedAnchoredPairPruneBudget{1U, 3U, 3U});
    if (probe.leaf_begin() == 0U &&
        probe.leaf_end() == cloud.size()) {
      root_node_index = node_index;
      break;
    }
  }
  require(
      root_node_index < index.build_counters().node_count,
      "the per-query witness-subtree fixture lost its LBVH root");

  ExactGroupedAnchoredPairTraversalContext traversal =
      ExactGroupedAnchoredPairTraversalContext::
          start_witness_subtree_first_at_node(
              index, cloud, anchors, halo, root_node_index, 4U);
  const auto active_pool_equals_halo = [&]() {
    const std::span<const PointId> active_pool =
        traversal.witness_pool_point_ids();
    return active_pool.size() == halo.size() && std::equal(
        active_pool.begin(), active_pool.end(), halo.begin());
  };
  const ExactGroupedAnchoredPairTraversalStep child_prune = traversal.advance(
      index,
      cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget{256U, 256U});
  require(
      child_prune.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::certified_prune &&
          child_prune.lbvh_node_index().has_value() &&
          *child_prune.lbvh_node_index() != root_node_index &&
          child_prune.prune_certificate() != nullptr &&
          traversal.audit().witness_subtree_fail_open_count == 1U &&
          traversal.audit().witness_subtree_success_count == 1U &&
          !active_pool_equals_halo(),
      "the witness-subtree search did not restart below a failed root core");

  const ExactGroupedAnchoredPairTraversalStep sibling_preflight =
      traversal.advance(
          index,
          cloud,
          ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  require(
      sibling_preflight.kind() ==
              ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted &&
          sibling_preflight.stop_reason() ==
              ExactGroupedAnchoredPairTraversalStopReason::
                  exact_predicate_limit &&
          sibling_preflight.work().node_visit_count == 1U &&
          sibling_preflight.work().exact_predicate_count == 0U &&
          active_pool_equals_halo(),
      "a child core pool or its bit meaning leaked into the sibling query");
}

void test_symmetric_receipt_recertifies_positive_cross_block() {
  const CanonicalPointCloud cloud = make_line_cloud(4U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  require(
      index.leaves().size() == 4U &&
          index.leaves()[0].point_id == 0U &&
          index.leaves()[1].point_id == 1U,
      "the symmetric-receipt fixture lost its Morton prefix");
  const std::array<PointId, 1> anchors{0U};
  const std::array<PointId, 1> witnesses{1U};
  const ExactGroupedAnchoredPairPruneBudget budget{1U, 1U, 1U};
  std::size_t query_node_index = index.build_counters().node_count;
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const ExactGroupedAnchoredPairPruneCertificate probe =
        certify_exact_grouped_anchored_pair_prune(
            index, cloud, anchors, witnesses, node_index, 2U, budget);
    if (probe.leaf_begin() == 2U && probe.leaf_end() == 4U) {
      require(probe.certified(),
              "the strict line witness did not certify {0} x {2,3}");
      query_node_index = node_index;
      break;
    }
  }
  require(
      query_node_index < index.build_counters().node_count,
      "the symmetric-receipt fixture lost its native query subtree");
  const ExactGroupedAnchoredPairPruneCertificate source =
      certify_exact_grouped_anchored_pair_prune(
          index,
          cloud,
          anchors,
          witnesses,
          query_node_index,
          2U,
          budget);
  const ExactSymmetricGroupedAnchoredPairPruneReceipt receipt =
      recertify_exact_symmetric_grouped_anchored_pair_prune(
          index, cloud, source, 0U, 1U, query_node_index, 2U);
  require(
      receipt.validated_for(index, cloud) &&
          receipt.anchor_leaf_begin() == 0U &&
          receipt.anchor_leaf_end() == 1U &&
          receipt.query_leaf_begin() == 2U &&
          receipt.query_leaf_end() == 4U &&
          receipt.unordered_mass() == 2U &&
          receipt.directed_mass() == 4U &&
          receipt.audit().source_certificate_recertified,
      "the symmetric receipt lost its recertified positive mass");
  require_throws<std::invalid_argument>(
      [&]() {
        (void)recertify_exact_symmetric_grouped_anchored_pair_prune(
            index, cloud, source, 1U, 2U, query_node_index, 2U);
      },
      "the symmetric receipt accepted a rebound anchor range");
  const std::size_t foreign_query_node_index =
      query_node_index == 0U ? 1U : 0U;
  require_throws<std::invalid_argument>(
      [&]() {
        (void)recertify_exact_symmetric_grouped_anchored_pair_prune(
            index,
            cloud,
            source,
            0U,
            1U,
            foreign_query_node_index,
            2U);
      },
      "the symmetric receipt accepted a rebound query node");
  const MortonLbvhIndex rebuilt_index = MortonLbvhIndex::build(cloud);
  require(
      !receipt.validated_for(rebuilt_index, cloud),
      "the process-local receipt accepted a rebuilt LBVH authority");
}

}  // namespace

int main() {
  try {
    test_shared_certificate_and_provenance();
    test_inconclusive_results_publish_no_partial_authority();
    test_shell_equality_fails_open();
    test_discrete_common_certificate_closes_hybrid_corner_gap();
    test_budgets_fail_atomically();
    test_structural_validation();
    test_prepared_inherited_traversal_differential();
    test_prepared_inheritance_preserves_nonprefix_canonical_order();
    test_prepared_traversal_resumes_atomically();
    test_prepared_traversal_fallback_and_provenance();
    test_frontier_descends_diagonal_without_signs();
    test_frontier_inconclusive_subtree_resumes_stably();
    test_bounded_witness_subtree_proposal_replays_through_p8g();
    test_bounded_witness_subtree_cap_fails_open();
    test_witness_subtree_search_restarts_and_restores_halo_per_query_node();
    test_symmetric_receipt_recertifies_positive_cross_block();
  } catch (const std::exception& error) {
    std::cerr << "grouped anchored-pair certificate test failure: "
              << error.what() << '\n';
    return 1;
  }
  std::cout << "grouped anchored-pair certificate checks passed\n";
  return 0;
}
