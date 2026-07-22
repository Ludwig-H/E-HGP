#include "morsehgp3d/hierarchy/direct_closed_saddle_incidence_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::
    ExactDirectClosedSaddleIncidenceBudget;
using morsehgp3d::hierarchy::
    ExactDirectClosedSaddleIncidenceDecision;
using morsehgp3d::hierarchy::
    ExactDirectClosedSaddleIncidenceJournalResult;
using morsehgp3d::hierarchy::ExactDirectMorseEventJournalResult;
using morsehgp3d::hierarchy::ExactDirectSaddleArmFacet;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
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

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactDirectClosedSaddleIncidenceBudget{
      maximum, maximum, maximum, maximum};
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          terminal_budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index,
          cloud,
          requested_maximum_order,
          terminal_budget.higher);
  auto facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          terminal_budget,
          pair,
          higher);
  auto event_journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, facade);
  auto arm_journal =
      morsehgp3d::hierarchy::build_exact_direct_saddle_arm_seed_journal(
          cloud,
          facade,
          event_journal,
          unlimited_arm_budget());
  return DirectSources{
      std::move(facade),
      std::move(event_journal),
      std::move(arm_journal)};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceJournalResult incidences(
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    const ExactDirectClosedSaddleIncidenceBudget& budget =
        unlimited_incidence_budget()) {
  return morsehgp3d::hierarchy::
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          source.facade,
          source.event_journal,
          unlimited_arm_budget(),
          source.arm_journal,
          budget);
}

[[nodiscard]] std::vector<PointId> facet_ids(
    const ExactDirectSaddleArmFacet& facet) {
  return std::vector<PointId>(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
}

void test_obtuse_event_partitions_all_three_facets() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 3U);
  const auto result = incidences(cloud, source);
  const auto verification = morsehgp3d::hierarchy::
      verify_exact_direct_closed_saddle_incidence_journal_streaming(
          cloud,
          source.facade,
          source.event_journal,
          unlimited_arm_budget(),
          source.arm_journal,
          unlimited_incidence_budget(),
          result);

  const auto family = std::find_if(
      result.families.begin(),
      result.families.end(),
      [&source](const auto& candidate) {
        const auto& event =
            source.facade.events[candidate.source_event_index];
        return candidate.order == 2U && event.support_size == 2U &&
               event.interior_ids.size() == 1U;
      });
  bool exact = family != result.families.end();
  if (family != result.families.end()) {
    const auto& event = source.facade.events[family->source_event_index];
    const auto facet = facet_ids(
        morsehgp3d::hierarchy::
            reconstruct_exact_direct_equal_level_saddle_facet(
                source.facade,
                source.arm_journal,
                result,
                family->equal_level_facet_seed_offset));
    std::vector<PointId> expected{
        event.support_ids[0], event.support_ids[1]};
    std::sort(expected.begin(), expected.end());
    exact = facet == expected && family->strict_arm_seed_count == 2U &&
            family->equal_level_facet_seed_count == 1U &&
            family->closed_facet_count == 3U;
  }
  check(
      result.certified_partial_refinement() &&
          verification.result_certified && exact,
      "an obtuse rank-three saddle reuses two strict arms and adds its one equal-level support facet");
}

void test_k10_diameter_has_nine_equal_level_deletions() {
  const std::array<CertifiedPoint3, 11U> points{
      point(-5.0, 0.0),
      point(-4.0, 0.0),
      point(-3.0, 0.0),
      point(-2.0, 0.0),
      point(-1.0, 0.0),
      point(0.0, 0.0),
      point(1.0, 0.0),
      point(2.0, 0.0),
      point(3.0, 0.0),
      point(4.0, 0.0),
      point(5.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 10U);
  const auto result = incidences(cloud, source);
  const auto verification = morsehgp3d::hierarchy::
      verify_exact_direct_closed_saddle_incidence_journal_streaming(
          cloud,
          source.facade,
          source.event_journal,
          unlimited_arm_budget(),
          source.arm_journal,
          unlimited_incidence_budget(),
          result);

  const auto family = std::find_if(
      result.families.begin(),
      result.families.end(),
      [&source](const auto& candidate) {
        const auto& event =
            source.facade.events[candidate.source_event_index];
        return candidate.order == 10U && event.support_size == 2U &&
               event.interior_ids.size() == 9U;
      });
  bool exact = family != result.families.end();
  std::set<std::vector<PointId>> reconstructed;
  if (family != result.families.end()) {
    const auto& event = source.facade.events[family->source_event_index];
    const std::vector<PointId> expected_interior{
        1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    exact = family->strict_arm_seed_count == 2U &&
            family->equal_level_facet_seed_count == 9U &&
            family->closed_facet_count == 11U &&
            event.support_ids[0] == 0U && event.support_ids[1] == 10U &&
            event.interior_ids == expected_interior &&
            event.squared_level == ExactLevel{BigInt{25}, BigInt{1}};
    for (std::size_t local = 0U;
         local < family->equal_level_facet_seed_count;
         ++local) {
      const auto facet = facet_ids(
          morsehgp3d::hierarchy::
              reconstruct_exact_direct_equal_level_saddle_facet(
                  source.facade,
                  source.arm_journal,
                  result,
                  family->equal_level_facet_seed_offset + local));
      exact = exact && facet.size() == 10U;
      reconstructed.insert(facet);
    }
  }
  check(
      result.certified_partial_refinement() &&
          verification.result_certified && exact &&
          reconstructed.size() == 9U &&
          result.logical_added_storage_entry_count <=
              result.logical_added_storage_entry_limit &&
          result.combined_logical_storage_entry_count <=
              result.combined_logical_storage_entry_limit &&
          !result.facets_materialized_in_journal &&
          !result.non_direct_gateway_generation_complete &&
          !result.frozen_quotient_performed,
      "the K=10 diameter keeps two strict arms plus nine distinct equal-level facet seeds in linear storage");
}

void test_budget_and_seed_mutation_fail_closed() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectSources source = direct_sources(cloud, 3U);
  const auto complete = incidences(cloud, source);

  auto short_budget = unlimited_incidence_budget();
  short_budget.maximum_equal_level_facet_seed_count =
      complete.required_equal_level_facet_seed_count - 1U;
  const auto exhausted = incidences(cloud, source, short_budget);
  check(
      exhausted.decision == ExactDirectClosedSaddleIncidenceDecision::
                                no_incidence_journal_budget_exhausted &&
          exhausted.families.empty() &&
          exhausted.equal_level_facet_seeds.empty() &&
          !exhausted.source_arm_journal_freshly_replayed,
      "a one-short equal-level cap fails before replay and allocation");

  auto mutated = complete;
  mutated.equal_level_facet_seeds.front().removed_interior_point_id =
      static_cast<PointId>(cloud.size());
  const auto verification = morsehgp3d::hierarchy::
      verify_exact_direct_closed_saddle_incidence_journal_streaming(
          cloud,
          source.facade,
          source.event_journal,
          unlimited_arm_budget(),
          source.arm_journal,
          unlimited_incidence_budget(),
          mutated);
  check(
      !verification.result_certified,
      "streaming replay rejects a forged removed-interior identity");
}

}  // namespace

int main() {
  test_obtuse_event_partitions_all_three_facets();
  test_k10_diameter_has_nine_equal_level_deletions();
  test_budget_and_seed_mutation_fail_closed();
  if (failures != 0) {
    std::cerr << failures
              << " direct closed saddle incidence checks failed\n";
    return 1;
  }
  std::cout << "all direct closed saddle incidence checks passed\n";
  return 0;
}
