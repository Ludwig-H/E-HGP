#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

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
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneDecision;
using morsehgp3d::hierarchy::ExactGroupedAnchoredPairPruneStopReason;
using morsehgp3d::hierarchy::certify_exact_grouped_anchored_pair_prune;
using morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum_sign;
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

static_assert(
    !ExposesRvalueAudit<ExactGroupedAnchoredPairPruneCertificate>);
static_assert(
    !ExposesRvalueWitnessSpan<ExactGroupedAnchoredPairPruneCertificate>);

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
      result.maximum_closed_rank() == 4U &&
          result.required_witness_count() == 3U &&
          witnesses.size() == 3U && witnesses[0] == 2U &&
          witnesses[1] == 3U && witnesses[2] == 4U &&
          result.certified_witness_pool_mask() == UINT64_C(0x7),
      "the grouped certificate did not publish its canonical strict prefix");
  require(
      audit.anchor_count == 2U &&
          audit.witness_pool_entry_count == 4U &&
          audit.exact_predicate_count == 3U &&
          audit.strict_group_witness_count == 3U &&
          audit.input_canonical && audit.anchor_bounds_constructed &&
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
          partial_audit.exact_predicate_count == 3U &&
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

void test_correlated_boxes_fail_open() {
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
          predicate_exhausted.audit().strict_group_witness_count == 2U &&
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

}  // namespace

int main() {
  try {
    test_shared_certificate_and_provenance();
    test_inconclusive_results_publish_no_partial_authority();
    test_shell_equality_fails_open();
    test_correlated_boxes_fail_open();
    test_budgets_fail_atomically();
    test_structural_validation();
  } catch (const std::exception& error) {
    std::cerr << "grouped anchored-pair certificate test failure: "
              << error.what() << '\n';
    return 1;
  }
  std::cout << "grouped anchored-pair certificate checks passed\n";
  return 0;
}
