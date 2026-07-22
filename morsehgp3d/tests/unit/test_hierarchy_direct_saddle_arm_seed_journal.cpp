#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactDirectMorseEventJournalResult;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedDecision;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedJournalResult;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalBudget;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalFacade;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::spatial::CanonicalPointCloud;
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
    double y,
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

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactDirectSaddleArmSeedBudget{
      maximum, maximum, maximum, maximum};
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, budget.higher);
  ExactDirectSupportTerminalFacade facade =
      morsehgp3d::hierarchy::
          build_exact_direct_support_terminal_facade(
              index,
              cloud,
              requested_maximum_order,
              budget,
              pair,
              higher);
  ExactDirectMorseEventJournalResult journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, facade);
  return DirectSources{std::move(facade), std::move(journal)};
}

[[nodiscard]] std::vector<PointId> facet_ids(
    const morsehgp3d::hierarchy::ExactDirectSaddleArmFacet& facet) {
  return std::vector<PointId>(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
}

void test_regular_tetrahedron_factorization_and_bounds() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 10U);
  const auto streaming_source_verification =
      morsehgp3d::hierarchy::
          verify_exact_direct_morse_event_journal_streaming(
              cloud, source.facade, source.journal);
  const auto result =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              unlimited_arm_budget());
  const auto verification =
      morsehgp3d::hierarchy::
          verify_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              unlimited_arm_budget(),
              result);

  check(
      result.certified_partial_refinement() &&
          verification.result_certified &&
          streaming_source_verification.result_certified &&
          streaming_source_verification
              .constant_auxiliary_record_storage_certified &&
          streaming_source_verification.canonical_cloud_digest_pass_count ==
              2U &&
          streaming_source_verification.event_projection_scan_count == 15U &&
          streaming_source_verification.role_record_scan_count == 26U &&
          streaming_source_verification.batch_scan_count == 7U &&
          result.families.size() == 11U &&
          result.arm_seeds.size() == 28U,
      "the constant-storage source replay feeds eleven direct saddle families and twenty-eight factorized arm seeds");

  std::array<std::size_t, 3U> event_histogram{};
  std::array<std::size_t, 3U> arm_histogram{};
  for (const auto& family : result.families) {
    const auto& event = source.facade.events[family.source_event_index];
    const std::size_t arity_index =
        static_cast<std::size_t>(event.support_size - 2U);
    ++event_histogram[arity_index];
    arm_histogram[arity_index] += family.arm_seed_count;
  }
  check(
      event_histogram == std::array<std::size_t, 3U>{6U, 4U, 1U} &&
          arm_histogram ==
              std::array<std::size_t, 3U>{12U, 12U, 4U},
      "tetrahedron saddle and arm counts close independently by support arity");

  const auto tetra_family = std::find_if(
      result.families.begin(),
      result.families.end(),
      [&source](const auto& family) {
        return source.facade.events[family.source_event_index]
                   .support_size == 4U;
      });
  bool opposite_facets_are_exact = tetra_family != result.families.end();
  if (tetra_family != result.families.end()) {
    const std::array<std::vector<PointId>, 4U> expected{
        std::vector<PointId>{1U, 2U, 3U},
        std::vector<PointId>{0U, 2U, 3U},
        std::vector<PointId>{0U, 1U, 3U},
        std::vector<PointId>{0U, 1U, 2U}};
    for (std::size_t local = 0U; local < 4U; ++local) {
      const std::size_t seed_index =
          tetra_family->arm_seed_offset + local;
      opposite_facets_are_exact =
          opposite_facets_are_exact &&
          facet_ids(morsehgp3d::hierarchy::
                        reconstruct_exact_direct_saddle_arm_facet(
                            source.facade, result, seed_index)) ==
              expected[local];
    }
  }
  check(
      opposite_facets_are_exact,
      "the arity-four saddle reconstructs its four opposite triangular facets in canonical removed-point order");

  check(
      result.logical_added_storage_entry_count == 39U &&
          result.logical_added_storage_entry_count <=
              result.logical_added_storage_entry_limit &&
          result.combined_logical_storage_entry_count <=
              result.combined_logical_storage_entry_limit &&
          !result.facets_materialized_in_journal &&
          !result.miniballs_or_global_partitions_computed &&
          result.no_forbidden_global_structure_materialized &&
          !result.hierarchy_reduction_performed &&
          !result.forest_or_gateway_attach_performed &&
          !result.public_status_claimed,
      "the added journal stays within S+A<=5E and retains no facet, miniball, Gamma object, forest or attachment");
}

void test_interior_points_belong_to_every_arm() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 3U);
  const auto result =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              unlimited_arm_budget());

  const auto diameter_family = std::find_if(
      result.families.begin(),
      result.families.end(),
      [&source](const auto& family) {
        const auto& event =
            source.facade.events[family.source_event_index];
        return event.support_size == 2U &&
               event.support_ids[0] == 0U &&
               event.support_ids[1] == 2U &&
               event.interior_ids == std::vector<PointId>{1U} &&
               event.squared_level == level(4);
      });
  bool exact = diameter_family != result.families.end();
  if (diameter_family != result.families.end()) {
    exact =
        facet_ids(morsehgp3d::hierarchy::
                      reconstruct_exact_direct_saddle_arm_facet(
                          source.facade,
                          result,
                          diameter_family->arm_seed_offset)) ==
            std::vector<PointId>({1U, 2U}) &&
        facet_ids(morsehgp3d::hierarchy::
                      reconstruct_exact_direct_saddle_arm_facet(
                          source.facade,
                          result,
                          diameter_family->arm_seed_offset + 1U)) ==
            std::vector<PointId>({0U, 1U});
  }
  check(
      exact,
      "an obtuse diameter event reconstructs (I union U) minus u rather than only U minus u");
}

void test_equal_level_events_keep_distinct_arm_provenance() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 4U);
  const auto result =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              unlimited_arm_budget());

  std::vector<std::size_t> simultaneous_family_indices;
  for (const auto& family : result.families) {
    if (family.order == 2U &&
        family.critical_squared_level == level(169, 36)) {
      simultaneous_family_indices.push_back(family.family_index);
    }
  }
  std::size_t matching_shared_facets = 0U;
  std::size_t simultaneous_arm_count = 0U;
  for (const std::size_t family_index : simultaneous_family_indices) {
    const auto& family = result.families[family_index];
    simultaneous_arm_count += family.arm_seed_count;
    for (std::size_t local = 0U; local < family.arm_seed_count; ++local) {
      const auto facet = facet_ids(
          morsehgp3d::hierarchy::
              reconstruct_exact_direct_saddle_arm_facet(
                  source.facade,
                  result,
                  family.arm_seed_offset + local));
      if (facet == std::vector<PointId>({0U, 3U})) {
        ++matching_shared_facets;
      }
    }
  }
  check(
      simultaneous_family_indices.size() == 2U &&
          simultaneous_arm_count == 6U && matching_shared_facets == 2U,
      "two simultaneous mirror saddles retain six event-qualified seeds even when one reconstructed facet is shared");
}

void test_budget_and_mutation_fail_closed() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 10U);
  const auto complete =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              unlimited_arm_budget());

  ExactDirectSaddleArmSeedBudget short_budget = unlimited_arm_budget();
  short_budget.maximum_arm_seed_count =
      complete.required_arm_seed_count - 1U;
  const auto exhausted =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              source.journal,
              short_budget);
  check(
      exhausted.decision == ExactDirectSaddleArmSeedDecision::
                                no_seed_journal_budget_exhausted &&
          exhausted.families.empty() && exhausted.arm_seeds.empty() &&
          !exhausted.source_journal_freshly_replayed,
      "a one-short arm cap fails atomically before source replay and output allocation");

  auto mutated = complete;
  mutated.arm_seeds.front().removed_support_point_id =
      static_cast<PointId>(cloud.size());
  check(
      !morsehgp3d::hierarchy::
           verify_exact_direct_saddle_arm_seed_journal(
               cloud,
               source.facade,
               source.journal,
               unlimited_arm_budget(),
               mutated)
           .result_certified,
      "fresh replay rejects a mutated removed-support identity");

  auto broken_source = source.journal;
  broken_source.role_records.back().event_projection_index = 0U;
  const auto rejected =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud,
              source.facade,
              broken_source,
              unlimited_arm_budget());
  check(
      rejected.decision == ExactDirectSaddleArmSeedDecision::
                               no_seed_journal_source_not_certified &&
          rejected.families.empty() && rejected.arm_seeds.empty(),
      "a broken Phase-10 role join cannot produce any arm seed payload");

  const std::array<CertifiedPoint3, 4U> translated_points{
      point(3.0, 1.0, 1.0),
      point(3.0, -1.0, -1.0),
      point(1.0, 1.0, -1.0),
      point(1.0, -1.0, 1.0)};
  const CanonicalPointCloud translated_cloud =
      canonical_cloud(translated_points);
  const DirectSources translated_source =
      direct_sources(translated_cloud, 10U);
  bool foreign_authority_rejected = false;
  try {
    static_cast<void>(morsehgp3d::hierarchy::
                          reconstruct_exact_direct_saddle_arm_facet(
                              translated_source.facade, complete, 0U));
  } catch (const std::invalid_argument&) {
    foreign_authority_rejected = true;
  }
  check(
      foreign_authority_rejected,
      "on-demand reconstruction rejects a same-cardinality facade bound to a different canonical cloud");

  auto stale_payload = source.facade;
  const auto& first_family = complete.families.front();
  auto& stale_event =
      stale_payload.events[first_family.source_event_index];
  std::swap(stale_event.support_ids[0], stale_event.support_ids[1]);
  bool stale_event_identity_rejected = false;
  try {
    static_cast<void>(morsehgp3d::hierarchy::
                          reconstruct_exact_direct_saddle_arm_facet(
                              stale_payload,
                              complete,
                              first_family.arm_seed_offset));
  } catch (const std::invalid_argument&) {
    stale_event_identity_rejected = true;
  }
  check(
      stale_event_identity_rejected,
      "on-demand reconstruction rejects an event payload mutated under otherwise matching facade digests");
}

}  // namespace

int main() {
  test_regular_tetrahedron_factorization_and_bounds();
  test_interior_points_belong_to_every_arm();
  test_equal_level_events_keep_distinct_arm_provenance();
  test_budget_and_mutation_fail_closed();
  if (failures != 0) {
    std::cerr << failures << " direct saddle arm seed checks failed\n";
    return 1;
  }
  std::cout << "all direct saddle arm seed checks passed\n";
  return 0;
}
