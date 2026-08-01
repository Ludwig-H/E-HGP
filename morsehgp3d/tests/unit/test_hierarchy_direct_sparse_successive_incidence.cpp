#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] PointId point_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a source point is absent from its canonical cloud");
}

template <std::size_t Size>
[[nodiscard]] ExactDirectSparseFacetKey key_from_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  static_assert(Size >= 1U);
  static_assert(
      Size <= direct_sparse_successive_incidence_maximum_source_point_count);
  std::array<PointId, Size> point_ids{};
  for (std::size_t index = 0U; index < Size; ++index) {
    point_ids[index] = point_id_for_source(cloud, source_indices[index]);
  }
  std::sort(point_ids.begin(), point_ids.end());
  ExactDirectSparseFacetKey key;
  key.point_count = Size;
  std::copy(point_ids.begin(), point_ids.end(), key.point_ids.begin());
  return key;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget generous_budget() {
  return {
      1024U,
      4096U,
      4096U,
      65536U,
      4096U,
      65536U,
      262144U,
      4096U,
      64U};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget exact_budget_for(
    const ExactDirectSparseSuccessiveIncidenceResult& result) {
  return {
      result.audit.source_support_enumeration_count,
      result.audit.node_visit_count,
      result.audit.internal_node_expansion_count,
      result.audit.exact_aabb_bound_evaluation_count,
      result.audit.exact_point_evaluation_count,
      result.audit.coface_support_enumeration_count,
      result.audit.candidate_point_classification_count,
      result.audit.peak_frontier_entry_count,
      result.audit.peak_cominimizer_entry_count};
}

[[nodiscard]] bool verification_closes(
    const ExactDirectSparseSuccessiveIncidenceVerification& verification) {
  return verification.trusted_inputs_certified &&
         verification.observed_storage_within_budget &&
         verification.source_miniball_freshly_replayed &&
         verification.branch_and_bound_freshly_replayed &&
         verification.all_cominimizers_freshly_replayed &&
         verification.counters_and_decision_freshly_replayed &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source,
    std::optional<ExactLevel> threshold,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    LbvhTraversalOrder order,
    const std::string& context) {
  const ExactDirectSparseSuccessiveIncidenceResult result =
      build_exact_direct_sparse_successive_incidence(
          index, cloud, source, threshold, budget, order);
  const auto verification = verify_exact_direct_sparse_successive_incidence(
      index, cloud, source, threshold, budget, order, result);
  check(
      verification_closes(verification),
      context + " closes under a fresh exact replay");
  return result;
}

[[nodiscard]] std::vector<PointId> minimizer_ids(
    const ExactDirectSparseSuccessiveIncidenceResult& result) {
  std::vector<PointId> ids;
  ids.reserve(result.cominimizers.size());
  for (const auto& minimizer : result.cominimizers) {
    ids.push_back(minimizer.added_point_id);
  }
  return ids;
}

struct E5Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSparseFacetKey facet03;
  PointId point1{};
  PointId point2{};
  PointId point4{};

  E5Fixture()
      : cloud(canonical_cloud(std::array{
            point(-2.0, -1.0),
            point(-2.0, 1.0),
            point(0.0, 0.0),
            point(3.0, 2.0),
            point(4.0, -1.0)})),
        index(MortonLbvhIndex::build(cloud)),
        facet03(key_from_sources(
            cloud, std::array<std::size_t, 2U>{0U, 3U})),
        point1(point_id_for_source(cloud, 1U)),
        point2(point_id_for_source(cloud, 2U)),
        point4(point_id_for_source(cloud, 4U)) {}
};

void test_e5_first_then_second_incidence() {
  E5Fixture fixture;
  const auto first = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      std::nullopt,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "E5 facet 03 first incidence");
  std::vector<PointId> expected_first{fixture.point1, fixture.point2};
  std::sort(expected_first.begin(), expected_first.end());
  check(
      first.certified_complete_next_incidence() &&
          first.next_incidence_squared_level == level(17, 2) &&
          minimizer_ids(first) == expected_first &&
          first.audit.at_or_below_exclusive_threshold_point_count == 0U,
      "E5 facet 03 first emits both equal 013 and 023 cofaces at 17/2");

  const auto second = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(17, 2),
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "E5 facet 03 second incidence");
  check(
      second.certified_complete_next_incidence() &&
          second.exclusive_lower_squared_level == level(17, 2) &&
          second.next_incidence_squared_level == level(85, 9) &&
          minimizer_ids(second) == std::vector<PointId>{fixture.point4} &&
          second.audit.at_or_below_exclusive_threshold_point_count == 2U,
      "the strict cursor advances beyond the complete 17/2 shell and certifies 034 at 85/9");
  check(
      second.no_global_facet_or_coface_catalog_materialized &&
          second.no_gamma_or_higher_order_delaunay_materialized &&
          second.partial_refinement_only && !second.public_status_claimed,
      "the second E5 incidence remains a single supplied-facet sparse receipt");

  const auto terminal = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(85, 9),
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "E5 facet 03 terminal strict cursor");
  check(
      terminal.certified_complete_no_strictly_higher_coface() &&
          !terminal.next_incidence_squared_level.has_value() &&
          terminal.cominimizers.empty() &&
          terminal.audit.exact_point_evaluation_count == 3U &&
          terminal.audit.at_or_below_exclusive_threshold_point_count == 3U &&
          terminal.audit.pruned_eligible_point_count == 0U,
      "a threshold at 85/9 certifies that facet 03 has no later one-point coface");
}

void test_exact_threshold_equality_near_far_and_seed_invariance() {
  E5Fixture fixture;
  const auto near = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(17, 2),
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "near-first successive shell");
  const auto far = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(17, 2),
      generous_budget(),
      LbvhTraversalOrder::far_first,
      "far-first successive shell");
  check(
      near.next_incidence_squared_level == far.next_incidence_squared_level &&
          near.cominimizers == far.cominimizers &&
          near.audit.at_or_below_exclusive_threshold_point_count == 2U &&
          far.audit.at_or_below_exclusive_threshold_point_count == 2U,
      "near/far traversal cannot turn threshold-equal cofaces into exclusions or change the next shell");

  const std::array<PointId, 1U> seeds{fixture.point4};
  const auto seeded =
      build_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
          fixture.index,
          fixture.cloud,
          fixture.facet03,
          seeds,
          level(17, 2),
          generous_budget(),
          LbvhTraversalOrder::near_first);
  const auto verification =
      verify_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
          fixture.index,
          fixture.cloud,
          fixture.facet03,
          seeds,
          level(17, 2),
          generous_budget(),
          LbvhTraversalOrder::near_first,
          seeded);
  check(
      verification_closes(verification) &&
          seeded.next_incidence_squared_level ==
              near.next_incidence_squared_level &&
          seeded.cominimizers == near.cominimizers &&
          seeded.audit.supplied_incumbent_seed_point_count == 1U &&
          seeded.audit.exact_incumbent_seed_evaluation_count == 1U,
      "an exact seed accelerates only the incumbent proposal and preserves the complete strict shell");
}

void test_every_exact_budget_cap_fails_closed_at_minus_one() {
  E5Fixture fixture;
  const auto complete = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(17, 2),
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "E5 cap baseline");
  const ExactDirectSparseSuccessiveIncidenceBudget exact =
      exact_budget_for(complete);
  using Member =
      std::size_t ExactDirectSparseSuccessiveIncidenceBudget::*;
  const std::array<Member, 9U> caps{
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_source_support_enumeration_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::maximum_node_visit_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_internal_node_expansion_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_exact_aabb_bound_evaluation_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_exact_point_evaluation_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_coface_support_enumeration_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_candidate_point_classification_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::maximum_frontier_entry_count,
      &ExactDirectSparseSuccessiveIncidenceBudget::maximum_cominimizer_count};
  std::size_t exercised = 0U;
  for (const Member cap : caps) {
    ExactDirectSparseSuccessiveIncidenceBudget reduced = exact;
    if (reduced.*cap == 0U) {
      continue;
    }
    --(reduced.*cap);
    const auto exhausted = build_and_verify(
        fixture.index,
        fixture.cloud,
        fixture.facet03,
        level(17, 2),
        reduced,
        LbvhTraversalOrder::near_first,
        "E5 exact cap minus one");
    check(
        exhausted.certified_budget_exhaustion() &&
            !exhausted.next_incidence_squared_level.has_value() &&
            exhausted.cominimizers.empty() &&
            exhausted.no_partial_next_incidence_payload_published,
        "each active cap minus one publishes no partial next-level shell");
    ++exercised;
  }
  check(exercised == caps.size(), "all nine exact budget dimensions are active");
}

void test_fresh_verifier_rejects_mutations_and_oversized_payloads() {
  E5Fixture fixture;
  const auto result = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.facet03,
      level(17, 2),
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "E5 verifier mutation baseline");
  const auto verify = [&](
                          const ExactDirectSparseSuccessiveIncidenceResult&
                              observed) {
    return verify_exact_direct_sparse_successive_incidence(
        fixture.index,
        fixture.cloud,
        fixture.facet03,
        level(17, 2),
        generous_budget(),
        LbvhTraversalOrder::near_first,
        observed);
  };

  auto wrong_threshold = result;
  wrong_threshold.exclusive_lower_squared_level = level(13, 2);
  auto wrong_level = result;
  wrong_level.next_incidence_squared_level = level(10);
  auto wrong_audit = result;
  ++wrong_audit.audit.at_or_below_exclusive_threshold_point_count;
  auto forbidden = result;
  forbidden.no_global_facet_or_coface_catalog_materialized = false;
  auto dirty_support_tail = result;
  dirty_support_tail.cominimizers[0U].support_point_count = 1U;
  dirty_support_tail.cominimizers[0U].support_point_ids[1U] =
      std::numeric_limits<PointId>::max();
  auto oversized_nested = result;
  oversized_nested.source_facet_miniball->facet_point_ids.resize(
      direct_sparse_successive_incidence_maximum_source_point_count + 1U,
      fixture.point1);

  check(
      !verify(wrong_threshold).result_certified &&
          !verify(wrong_level).result_certified &&
          !verify(wrong_audit).result_certified &&
          !verify(forbidden).result_certified,
      "fresh replay rejects threshold, level, counter and architecture mutations");
  const auto dirty_tail_verification = verify(dirty_support_tail);
  const auto oversized_verification = verify(oversized_nested);
  check(
      !dirty_tail_verification.observed_storage_within_budget &&
          !dirty_tail_verification.result_certified &&
          !oversized_verification.observed_storage_within_budget &&
          !oversized_verification.result_certified,
      "preflight rejects dirty support tails and oversized nested miniball storage before replay comparison");
}

void test_domain_caps_and_n_equals_k() {
  static_assert(
      direct_sparse_successive_incidence_maximum_source_point_count == 10U);
  static_assert(
      direct_sparse_successive_incidence_source_support_enumeration_count_per_pass ==
      385U);
  static_assert(
      direct_sparse_successive_incidence_maximum_source_support_enumeration_count ==
      770U);
  static_assert(
      direct_sparse_successive_incidence_maximum_outside_coface_support_count ==
      176U);
  static_assert(
      direct_sparse_successive_incidence_maximum_outside_coface_classification_count ==
      1936U);

  const CanonicalPointCloud cloud = canonical_cloud(
      std::array{point(-1.0), point(0.0), point(2.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey all = key_from_sources(
      cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  const auto terminal = build_and_verify(
      index,
      cloud,
      all,
      std::nullopt,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "n equals k terminal facet");
  check(
      terminal.certified_complete_no_strictly_higher_coface() &&
          terminal.audit.eligible_coface_point_count == 0U &&
          terminal.audit.node_visit_count == 0U,
      "the unthresholded n=k case retains the certified terminal meaning");

  ExactDirectSparseFacetKey oversized;
  oversized.point_count = 11U;
  bool rejected = false;
  try {
    static_cast<void>(build_exact_direct_sparse_successive_incidence(
        index,
        cloud,
        oversized,
        std::nullopt,
        generous_budget(),
        LbvhTraversalOrder::near_first));
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(rejected, "a source facet above the public K=10 cap is rejected");
}

}  // namespace

int main() {
  test_e5_first_then_second_incidence();
  test_exact_threshold_equality_near_far_and_seed_invariance();
  test_every_exact_budget_cap_fails_closed_at_minus_one();
  test_fresh_verifier_rejects_mutations_and_oversized_payloads();
  test_domain_caps_and_n_equals_k();
  if (failures != 0) {
    std::cerr << failures << " successive-incidence checks failed\n";
    return 1;
  }
  return 0;
}
