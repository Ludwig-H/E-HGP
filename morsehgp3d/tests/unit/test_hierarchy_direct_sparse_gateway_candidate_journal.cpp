#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
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

template <typename Function>
[[nodiscard]] bool throws_invalid_argument(Function&& function) {
  try {
    function();
  } catch (const std::invalid_argument&) {
    return true;
  } catch (...) {
  }
  return false;
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

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
generous_first_incidence_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget generous_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      generous_first_incidence_budget(),
  };
}

struct DirectSources {
  ExactDirectSupportTerminalBudget terminal_budget;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const ExactDirectSaddleArmSeedBudget arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const ExactDirectClosedSaddleIncidenceBudget incidence_budget =
      unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      terminal_budget,
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

[[nodiscard]] bool five_scientific_arenas_empty(
    const ExactDirectSparseGatewayCandidateJournalResult& result) {
  return result.deletion_projections.empty() && result.facet_tokens.empty() &&
         result.gateway_candidates.empty() && result.batches.empty() &&
         result.batch_facet_token_indices.empty();
}

[[nodiscard]] bool complete_verification(
    const ExactDirectSparseGatewayCandidateVerification& verification) {
  return verification.observed_storage_within_budget &&
         verification.source_incidence_journal_freshly_replayed &&
         verification.deletion_projections_freshly_replayed &&
         verification.facet_tokens_freshly_replayed &&
         verification.gateway_candidates_freshly_replayed &&
         verification.batches_freshly_replayed &&
         verification.counters_and_result_facts_freshly_replayed &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateJournalResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    const ExactDirectSparseGatewayCandidateBudget& budget,
    LbvhTraversalOrder traversal_order,
    const std::string& context) {
  const auto result = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      budget,
      traversal_order);
  const auto verification =
      verify_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          budget,
          traversal_order,
          result);
  check(
      complete_verification(verification),
      context + " closes under a fresh replay of all external authorities");
  return result;
}

[[nodiscard]] std::vector<PointId> key_ids(
    const ExactDirectSparseFacetKey& key) {
  return {
      key.point_ids.begin(),
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
  };
}

[[nodiscard]] ExactDirectSparseFacetKey projection_source_key(
    const DirectSources& source,
    const ExactDirectSparseGatewayDeletionProjection& projection) {
  ExactDirectSaddleArmFacet facet;
  if (projection.source ==
      ExactDirectSparseGatewayDeletionSource::strict_arm_seed) {
    facet = reconstruct_exact_direct_saddle_arm_facet(
        source.facade,
        source.arm_journal,
        projection.source_deletion_index);
  } else {
    facet = reconstruct_exact_direct_equal_level_saddle_facet(
        source.facade,
        source.arm_journal,
        source.incidence_journal,
        projection.source_deletion_index);
  }
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  std::copy_n(facet.point_ids.begin(), facet.point_count, key.point_ids.begin());
  return key;
}

[[nodiscard]] std::vector<ExactDirectSparseGatewayFacetToken>
scientific_tokens_without_operational_audits(
    std::vector<ExactDirectSparseGatewayFacetToken> tokens) {
  for (auto& token : tokens) {
    token.first_incidence_audit = {};
  }
  return tokens;
}

[[nodiscard]] const ExactDirectSparseGatewayFacetToken* find_token(
    const ExactDirectSparseGatewayCandidateJournalResult& result,
    const std::vector<PointId>& ids) {
  const auto found = std::find_if(
      result.facet_tokens.begin(),
      result.facet_tokens.end(),
      [&ids](const ExactDirectSparseGatewayFacetToken& token) {
        return key_ids(token.source_facet_key) == ids;
      });
  return found == result.facet_tokens.end() ? nullptr : &*found;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget exact_global_budget_for(
    const ExactDirectSparseGatewayCandidateJournalResult& result,
    const ExactDirectSparseFirstIncidenceBudget& first_incidence_budget) {
  return {
      result.required_source_family_scan_count,
      result.required_deletion_reference_count,
      result.required_distinct_facet_count,
      result.required_facet_key_point_count,
      result.required_gateway_candidate_count,
      result.required_batch_count,
      result.required_batch_facet_reference_count,
      result.logical_storage_entry_count,
      first_incidence_budget,
  };
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
exact_child_budget_for(
    const ExactDirectSparseGatewayCandidateJournalResult& result) {
  ExactDirectSparseFirstIncidenceBudget budget;
  for (const auto& token : result.facet_tokens) {
    const auto& audit = token.first_incidence_audit;
    budget.maximum_source_support_enumeration_count = std::max(
        budget.maximum_source_support_enumeration_count,
        audit.source_support_enumeration_count);
    budget.maximum_node_visit_count = std::max(
        budget.maximum_node_visit_count, audit.node_visit_count);
    budget.maximum_internal_node_expansion_count = std::max(
        budget.maximum_internal_node_expansion_count,
        audit.internal_node_expansion_count);
    budget.maximum_exact_aabb_bound_evaluation_count = std::max(
        budget.maximum_exact_aabb_bound_evaluation_count,
        audit.exact_aabb_bound_evaluation_count);
    budget.maximum_exact_point_evaluation_count = std::max(
        budget.maximum_exact_point_evaluation_count,
        audit.exact_point_evaluation_count);
    budget.maximum_coface_support_enumeration_count = std::max(
        budget.maximum_coface_support_enumeration_count,
        audit.coface_support_enumeration_count);
    budget.maximum_candidate_point_classification_count = std::max(
        budget.maximum_candidate_point_classification_count,
        audit.candidate_point_classification_count);
    budget.maximum_frontier_entry_count = std::max(
        budget.maximum_frontier_entry_count,
        audit.peak_frontier_entry_count);
    budget.maximum_cominimizer_count = std::max(
        budget.maximum_cominimizer_count,
        audit.peak_cominimizer_entry_count);
  }
  return budget;
}

struct ExhaustiveFirstIncidence {
  ExactLevel squared_level;
  std::vector<PointId> added_point_ids;
};

[[nodiscard]] ExhaustiveFirstIncidence
exhaustive_all_one_point_cofaces(
    const CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source) {
  const auto source_begin = source.point_ids.begin();
  const auto source_end =
      source_begin + static_cast<std::ptrdiff_t>(source.point_count);
  std::optional<ExactLevel> incumbent;
  std::vector<PointId> minimizers;
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId added_point_id = static_cast<PointId>(point_index);
    if (std::binary_search(source_begin, source_end, added_point_id)) {
      continue;
    }
    std::vector<PointId> coface{source_begin, source_end};
    coface.push_back(added_point_id);
    std::sort(coface.begin(), coface.end());
    const ExactFacetMiniballResult miniball =
        build_exact_facet_miniball(cloud, coface);
    if (!incumbent.has_value() ||
        miniball.squared_radius < *incumbent) {
      incumbent = miniball.squared_radius;
      minimizers.assign(1U, added_point_id);
    } else if (miniball.squared_radius == *incumbent) {
      minimizers.push_back(added_point_id);
    }
  }
  if (!incumbent.has_value()) {
    throw std::logic_error(
        "an exhaustive direct-facet fixture has no eligible coface");
  }
  std::sort(minimizers.begin(), minimizers.end());
  return {*incumbent, std::move(minimizers)};
}

void test_complete_empty_source_issues_no_geometry_query() {
  const std::array<CertifiedPoint3, 1U> points{point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 1U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      ExactDirectSparseGatewayCandidateBudget{},
      LbvhTraversalOrder::near_first,
      "the complete empty direct-saddle source");

  check(
      source.incidence_journal.families.empty() &&
          result.certified_partial_refinement() &&
          result.required_source_family_scan_count == 0U &&
          result.required_deletion_reference_count == 0U &&
          result.required_distinct_facet_count == 0U &&
          result.required_first_incidence_call_count == 0U &&
          result.required_gateway_candidate_count == 0U &&
          result.required_batch_count == 0U &&
          result.logical_storage_entry_count == 0U &&
          five_scientific_arenas_empty(result),
      "J=R=D=0 completes with five empty arenas and no 10.6 call");
}

void test_small_terminal_source_projects_every_deletion() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 3U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the small terminal deletion journal");

  const auto family = std::find_if(
      source.incidence_journal.families.begin(),
      source.incidence_journal.families.end(),
      [&source](const ExactDirectClosedSaddleIncidenceFamilyRecord& candidate) {
        const auto& event = source.facade.events[candidate.source_event_index];
        return candidate.order == 2U && event.support_size == 2U &&
               event.interior_ids.size() == 1U;
      });
  check(
      family != source.incidence_journal.families.end(),
      "the obtuse three-point source exposes its two strict and one equal deletion");
  if (family == source.incidence_journal.families.end()) {
    return;
  }

  std::vector<const ExactDirectSparseGatewayDeletionProjection*> projections;
  for (const auto& projection : result.deletion_projections) {
    if (projection.source_family_index == family->family_index) {
      projections.push_back(&projection);
    }
  }
  std::size_t strict_count = 0U;
  std::size_t equal_count = 0U;
  bool every_projection_exact = projections.size() == family->closed_facet_count;
  for (const auto* projection : projections) {
    strict_count += static_cast<std::size_t>(
        projection->source ==
        ExactDirectSparseGatewayDeletionSource::strict_arm_seed);
    equal_count += static_cast<std::size_t>(
        projection->source ==
        ExactDirectSparseGatewayDeletionSource::equal_level_facet_seed);
    every_projection_exact =
        every_projection_exact &&
        projection->source_event_index == family->source_event_index &&
        projection->source_order == family->order &&
        projection->facet_token_index < result.facet_tokens.size() &&
        projection_source_key(source, *projection) ==
            result.facet_tokens[projection->facet_token_index].source_facet_key &&
        result.facet_tokens[projection->facet_token_index]
                .first_incidence_squared_level <=
            projection->saddle_squared_level &&
        ((result.facet_tokens[projection->facet_token_index]
                      .first_incidence_squared_level <
                  projection->saddle_squared_level &&
              projection->level_relation ==
                  ExactDirectSparseGatewayLevelRelation::
                      first_incidence_strictly_below_saddle) ||
         (result.facet_tokens[projection->facet_token_index]
                      .first_incidence_squared_level ==
                  projection->saddle_squared_level &&
              projection->level_relation ==
                  ExactDirectSparseGatewayLevelRelation::
                      first_incidence_equal_to_saddle)) &&
        projection->removed_point_is_first_incidence_cominimizer ==
            (projection->level_relation ==
             ExactDirectSparseGatewayLevelRelation::
                 first_incidence_equal_to_saddle);
  }
  check(
      result.certified_partial_refinement() &&
          result.required_deletion_reference_count ==
              result.deletion_projections.size() &&
          result.required_first_incidence_call_count ==
              result.required_distinct_facet_count &&
          result.facet_tokens.size() == result.required_distinct_facet_count &&
          strict_count == 2U && equal_count == 1U && every_projection_exact,
      "every source deletion is reconstructed, classified and projected to one complete-key token");
}

void test_shared_facet_is_deduplicated_once_and_near_far_agree() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 4U);
  const auto near = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the near-first mirror gateway candidates");
  const auto far = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::far_first,
      "the far-first mirror gateway candidates");

  std::set<std::size_t> mirror_family_indices;
  for (const auto& family : source.incidence_journal.families) {
    if (family.order == 2U &&
        family.critical_squared_level == level(169, 36)) {
      mirror_family_indices.insert(family.family_index);
    }
  }
  std::vector<const ExactDirectSparseGatewayDeletionProjection*>
      mirror_projections;
  std::set<std::size_t> mirror_token_indices;
  for (const auto& projection : near.deletion_projections) {
    if (mirror_family_indices.contains(projection.source_family_index)) {
      mirror_projections.push_back(&projection);
      mirror_token_indices.insert(projection.facet_token_index);
    }
  }
  const auto* shared = find_token(near, {0U, 3U});
  std::vector<PointId> shared_minimizers;
  if (shared != nullptr) {
    for (std::size_t local = 0U;
         local < shared->gateway_candidate_count;
         ++local) {
      shared_minimizers.push_back(
          near.gateway_candidates[shared->gateway_candidate_offset + local]
              .added_point_id);
    }
  }
  check(
      mirror_family_indices.size() == 2U &&
          mirror_projections.size() == 6U &&
          mirror_token_indices.size() == 5U && shared != nullptr &&
          shared->deletion_projection_count == 2U &&
          shared->first_incidence_squared_level == level(169, 36) &&
          shared_minimizers == std::vector<PointId>({1U, 2U}) &&
          near.one_first_incidence_call_per_distinct_facet &&
          near.required_first_incidence_call_count == near.facet_tokens.size(),
      "the two mirror provenances share one full-key token, one 10.6 call and the complete equality shell");

  check(
      near.deletion_projections == far.deletion_projections &&
          scientific_tokens_without_operational_audits(near.facet_tokens) ==
              scientific_tokens_without_operational_audits(far.facet_tokens) &&
          near.gateway_candidates == far.gateway_candidates &&
          near.batches == far.batches &&
          near.batch_facet_token_indices == far.batch_facet_token_indices &&
          near.required_distinct_facet_count == far.required_distinct_facet_count &&
          near.required_gateway_candidate_count ==
              far.required_gateway_candidate_count,
      "near-first and far-first preserve every scientific token, candidate and canonical batch");
}

void test_permanent_ac_fixture_uses_real_direct_provenance() {
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 5U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the permanent AC silent-incidence candidate");

  const auto* ac = find_token(result, {1U, 3U});
  std::vector<PointId> minimizers;
  bool has_strict_later_projection = false;
  if (ac != nullptr) {
    for (std::size_t local = 0U; local < ac->gateway_candidate_count; ++local) {
      minimizers.push_back(
          result.gateway_candidates[ac->gateway_candidate_offset + local]
              .added_point_id);
    }
    for (std::size_t local = 0U; local < ac->deletion_projection_count; ++local) {
      const auto& projection =
          result.deletion_projections[ac->deletion_projection_offset + local];
      has_strict_later_projection =
          has_strict_later_projection ||
          projection.level_relation ==
              ExactDirectSparseGatewayLevelRelation::
                  first_incidence_strictly_below_saddle;
    }
  }
  check(
      ac != nullptr && ac->source_miniball_squared_level == level(33, 2) &&
          ac->first_incidence_squared_level == level(33, 2) &&
          minimizers == std::vector<PointId>({0U, 4U}) &&
          has_strict_later_projection,
      "the real direct stream reuses AC later while its two silent D/E incidences remain at 33/2");
}

void test_bounded_direct_keys_match_all_explicit_one_point_cofaces() {
  const std::array<CertifiedPoint3, 6U> points{
      point(-3.0, 1.0, 0.0),
      point(-1.0, -2.0, 1.0),
      point(0.0, 3.0, -1.0),
      point(2.0, -1.0, 2.0),
      point(4.0, 2.0, 1.0),
      point(1.0, 1.0, 5.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 4U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the bounded direct-key explicit-coface differential");

  std::set<std::vector<PointId>> expected_direct_keys;
  std::size_t expected_reference_count = 0U;
  for (const auto& family : source.incidence_journal.families) {
    expected_reference_count += family.closed_facet_count;
    for (std::size_t local = 0U;
         local < family.strict_arm_seed_count;
         ++local) {
      const auto facet = reconstruct_exact_direct_saddle_arm_facet(
          source.facade,
          source.arm_journal,
          family.strict_arm_seed_offset + local);
      expected_direct_keys.emplace(
          facet.point_ids.begin(),
          facet.point_ids.begin() +
              static_cast<std::ptrdiff_t>(facet.point_count));
    }
    for (std::size_t local = 0U;
         local < family.equal_level_facet_seed_count;
         ++local) {
      const auto facet =
          reconstruct_exact_direct_equal_level_saddle_facet(
              source.facade,
              source.arm_journal,
              source.incidence_journal,
              family.equal_level_facet_seed_offset + local);
      expected_direct_keys.emplace(
          facet.point_ids.begin(),
          facet.point_ids.begin() +
              static_cast<std::ptrdiff_t>(facet.point_count));
    }
  }
  std::set<std::vector<PointId>> observed_direct_keys;
  for (const auto& token : result.facet_tokens) {
    observed_direct_keys.insert(key_ids(token.source_facet_key));
  }
  bool every_token_matches =
      cloud.size() <= 14U && !result.facet_tokens.empty() &&
      expected_reference_count == result.deletion_projections.size() &&
      expected_direct_keys == observed_direct_keys;
  std::size_t checked_token_count = 0U;
  for (const auto& token : result.facet_tokens) {
    const ExhaustiveFirstIncidence expected =
        exhaustive_all_one_point_cofaces(cloud, token.source_facet_key);
    const ExactFacetMiniballResult source_miniball =
        build_exact_facet_miniball(
            cloud, key_ids(token.source_facet_key));
    std::vector<PointId> observed;
    for (std::size_t local = 0U;
         local < token.gateway_candidate_count;
         ++local) {
      const auto& candidate =
          result.gateway_candidates[token.gateway_candidate_offset + local];
      every_token_matches =
          every_token_matches &&
          candidate.facet_token_index == token.facet_token_index;
      observed.push_back(candidate.added_point_id);
    }
    every_token_matches =
        every_token_matches &&
        token.source_miniball_squared_level ==
            source_miniball.squared_radius &&
        token.first_incidence_squared_level == expected.squared_level &&
        observed == expected.added_point_ids;
    ++checked_token_count;
  }
  check(
      every_token_matches &&
          checked_token_count == result.required_distinct_facet_count,
      "for n<=14 every deduplicated direct key agrees with the independent exhaustive miniball oracle");
}

void test_input_permutations_preserve_the_canonical_event_stream() {
  std::array<CertifiedPoint3, 6U> points{
      point(-3.0, 1.0, 0.0),
      point(-1.0, -2.0, 1.0),
      point(0.0, 3.0, -1.0),
      point(2.0, -1.0, 2.0),
      point(4.0, 2.0, 1.0),
      point(1.0, 1.0, 5.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 4U);
  std::reverse(points.begin(), points.end());
  const CanonicalPointCloud permuted_cloud = canonical_cloud(points);
  const MortonLbvhIndex permuted_index =
      MortonLbvhIndex::build(permuted_cloud);
  const DirectSources permuted_source =
      direct_sources(permuted_cloud, 4U);

  const auto original = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the canonical event stream from the original input order");
  const auto permuted = build_and_verify(
      permuted_index,
      permuted_cloud,
      permuted_source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the canonical event stream from a reversed input order");
  check(
      source.facade.events == permuted_source.facade.events &&
          source.event_journal == permuted_source.event_journal &&
          source.arm_journal == permuted_source.arm_journal &&
          source.incidence_journal == permuted_source.incidence_journal &&
          original == permuted,
      "reversing the raw point/event discovery order preserves the canonical certified event stream and all five arenas");

  if (source.facade.events.size() < 2U) {
    check(false, "the permutation fixture needs at least two direct events");
    return;
  }
  auto noncanonical_source = source;
  std::reverse(
      noncanonical_source.facade.events.begin(),
      noncanonical_source.facade.events.end());
  const auto rejected =
      build_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          noncanonical_source.facade,
          noncanonical_source.event_journal,
          noncanonical_source.arm_budget,
          noncanonical_source.arm_journal,
          noncanonical_source.incidence_budget,
          noncanonical_source.incidence_journal,
          generous_budget(),
          LbvhTraversalOrder::near_first);
  check(
      rejected.decision ==
              ExactDirectSparseGatewayCandidateDecision::
                  no_gateway_candidate_source_not_certified &&
          five_scientific_arenas_empty(rejected),
      "a raw noncanonical event permutation is an authority mutation and fails closed");
}

void test_k10_factorization_and_k_plus_one_deletion_helper() {
  const std::array<CertifiedPoint3, 11U> points{
      point(-5.0),
      point(-4.0),
      point(-3.0),
      point(-2.0),
      point(-1.0),
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 10U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the K=10 factorized candidate journal");

  const auto family = std::find_if(
      source.incidence_journal.families.begin(),
      source.incidence_journal.families.end(),
      [&source](const ExactDirectClosedSaddleIncidenceFamilyRecord& candidate) {
        const auto& event = source.facade.events[candidate.source_event_index];
        return candidate.order == 10U && event.support_size == 2U &&
               event.interior_ids.size() == 9U;
      });
  check(
      family != source.incidence_journal.families.end(),
      "the collinear boundary exposes the order-ten diameter family");
  if (family == source.incidence_journal.families.end()) {
    return;
  }

  std::vector<const ExactDirectSparseGatewayDeletionProjection*> projections;
  std::set<std::size_t> token_indices;
  std::size_t strict_count = 0U;
  std::size_t equal_count = 0U;
  for (const auto& projection : result.deletion_projections) {
    if (projection.source_family_index != family->family_index) {
      continue;
    }
    projections.push_back(&projection);
    token_indices.insert(projection.facet_token_index);
    strict_count += static_cast<std::size_t>(
        projection.source ==
        ExactDirectSparseGatewayDeletionSource::strict_arm_seed);
    equal_count += static_cast<std::size_t>(
        projection.source ==
        ExactDirectSparseGatewayDeletionSource::equal_level_facet_seed);
  }

  bool fixed_width_tokens = true;
  std::size_t candidate_index = result.gateway_candidates.size();
  for (const std::size_t token_index : token_indices) {
    fixed_width_tokens =
        fixed_width_tokens && token_index < result.facet_tokens.size() &&
        result.facet_tokens[token_index].source_facet_key.point_count == 10U &&
        result.facet_tokens[token_index].gateway_candidate_count == 1U;
    if (candidate_index == result.gateway_candidates.size() &&
        token_index < result.facet_tokens.size()) {
      candidate_index =
          result.facet_tokens[token_index].gateway_candidate_offset;
    }
  }

  std::set<std::vector<PointId>> reconstructed_deletions;
  std::set<std::vector<PointId>> expected_deletions;
  if (candidate_index < result.gateway_candidates.size()) {
    const auto& candidate = result.gateway_candidates[candidate_index];
    std::vector<PointId> union_ids = key_ids(
        result.facet_tokens[candidate.facet_token_index].source_facet_key);
    union_ids.push_back(candidate.added_point_id);
    std::sort(union_ids.begin(), union_ids.end());
    for (std::size_t removed = 0U; removed < 11U; ++removed) {
      const auto deletion =
          reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
              result, candidate_index, removed);
      fixed_width_tokens = fixed_width_tokens && deletion.point_count == 10U;
      reconstructed_deletions.insert(key_ids(deletion));
      std::vector<PointId> expected = union_ids;
      expected.erase(
          expected.begin() + static_cast<std::ptrdiff_t>(removed));
      expected_deletions.insert(std::move(expected));
    }
  }
  check(
      projections.size() == 11U && token_indices.size() == 11U &&
          strict_count == 2U && equal_count == 9U && fixed_width_tokens &&
          reconstructed_deletions.size() == 11U &&
          reconstructed_deletions == expected_deletions &&
          !result.eleven_point_coface_keys_materialized &&
          std::all_of(
              result.gateway_candidates.begin(),
              result.gateway_candidates.end(),
              [](const ExactDirectSparseGatewayCandidateRecord& candidate) {
                return candidate.positive_support_point_count <= 4U;
              }),
      "K=10 keeps eleven factorized deletions and reconstructs them through one transient eleven-slot union only");
}

void test_global_and_nested_budgets_publish_five_empty_arenas() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 4U);
  const auto baseline = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the gateway exact-budget baseline");
  const ExactDirectSparseGatewayCandidateBudget exact =
      exact_global_budget_for(
          baseline, generous_first_incidence_budget());
  const auto exact_result = build_and_verify(
      index,
      cloud,
      source,
      exact,
      LbvhTraversalOrder::near_first,
      "all exact global gateway caps");
  check(
      exact_result.deletion_projections == baseline.deletion_projections &&
          exact_result.facet_tokens == baseline.facet_tokens &&
          exact_result.gateway_candidates == baseline.gateway_candidates &&
          exact_result.batches == baseline.batches &&
          exact_result.batch_facet_token_indices ==
              baseline.batch_facet_token_indices,
      "the exact eight global caps reproduce all five scientific arenas");

  const bool every_global_cap_exercised =
      exact.maximum_source_family_scan_count != 0U &&
      exact.maximum_deletion_reference_count != 0U &&
      exact.maximum_distinct_facet_count != 0U &&
      exact.maximum_facet_key_point_count != 0U &&
      exact.maximum_gateway_candidate_count != 0U &&
      exact.maximum_batch_count != 0U &&
      exact.maximum_batch_facet_reference_count != 0U &&
      exact.maximum_logical_storage_entry_count != 0U;
  check(
      every_global_cap_exercised,
      "the mirror fixture exercises all eight independent global caps");
  if (!every_global_cap_exercised) {
    return;
  }
  std::vector<ExactDirectSparseGatewayCandidateBudget> insufficient;
  {
    auto budget = exact;
    --budget.maximum_source_family_scan_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_deletion_reference_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_distinct_facet_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_facet_key_point_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_gateway_candidate_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_batch_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_batch_facet_reference_count;
    insufficient.push_back(budget);
  }
  {
    auto budget = exact;
    --budget.maximum_logical_storage_entry_count;
    insufficient.push_back(budget);
  }

  for (std::size_t index_in_cases = 0U;
       index_in_cases < insufficient.size();
       ++index_in_cases) {
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        insufficient[index_in_cases],
        LbvhTraversalOrder::near_first,
        "global one-short gateway cap " +
            std::to_string(index_in_cases));
    check(
        result.decision == ExactDirectSparseGatewayCandidateDecision::
                               no_gateway_candidate_budget_exhausted &&
            five_scientific_arenas_empty(result) &&
            result.no_partial_scientific_payload_published,
        "each one-short global cap publishes none of the five scientific arenas");
  }

  const ExactDirectSparseFirstIncidenceBudget exact_child =
      exact_child_budget_for(baseline);
  const ExactDirectSparseGatewayCandidateBudget exact_all =
      exact_global_budget_for(baseline, exact_child);
  const auto exact_child_result = build_and_verify(
      index,
      cloud,
      source,
      exact_all,
      LbvhTraversalOrder::near_first,
      "all nine exact per-facet 10.6 caps");
  check(
      exact_child_result.deletion_projections ==
              baseline.deletion_projections &&
          exact_child_result.facet_tokens == baseline.facet_tokens &&
          exact_child_result.gateway_candidates ==
              baseline.gateway_candidates &&
          exact_child_result.batches == baseline.batches &&
          exact_child_result.batch_facet_token_indices ==
              baseline.batch_facet_token_indices,
      "the componentwise exact 10.6 budget reproduces all five arenas");

  using ChildBudgetMember =
      std::size_t ExactDirectSparseFirstIncidenceBudget::*;
  const std::array<std::pair<ChildBudgetMember, std::string_view>, 9U>
      child_caps{{
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_source_support_enumeration_count,
           "source support"},
          {&ExactDirectSparseFirstIncidenceBudget::maximum_node_visit_count,
           "node visit"},
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_internal_node_expansion_count,
           "internal expansion"},
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_exact_aabb_bound_evaluation_count,
           "exact AABB"},
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_exact_point_evaluation_count,
           "exact point"},
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_coface_support_enumeration_count,
           "coface support"},
          {&ExactDirectSparseFirstIncidenceBudget::
               maximum_candidate_point_classification_count,
           "candidate classification"},
          {&ExactDirectSparseFirstIncidenceBudget::maximum_frontier_entry_count,
           "frontier"},
          {&ExactDirectSparseFirstIncidenceBudget::maximum_cominimizer_count,
           "co-minimizer"},
      }};
  const bool every_child_cap_exercised =
      std::all_of(
          child_caps.begin(),
          child_caps.end(),
          [&exact_child](const auto& entry) {
            return exact_child.*(entry.first) != 0U;
          });
  check(
      every_child_cap_exercised,
      "the mirror fixture exercises all nine independent 10.6 caps");
  if (every_child_cap_exercised) {
    for (const auto& [member, name] : child_caps) {
      auto budget = exact_all;
      --(budget.first_incidence_budget.*member);
      const auto result = build_and_verify(
          index,
          cloud,
          source,
          budget,
          LbvhTraversalOrder::near_first,
          "one-short per-facet 10.6 " + std::string{name} + " cap");
      check(
          result.decision ==
                  ExactDirectSparseGatewayCandidateDecision::
                      no_gateway_candidate_first_incidence_budget_exhausted &&
              five_scientific_arenas_empty(result) &&
              result.no_partial_scientific_payload_published &&
              !result.all_positive_support_candidates_retained_atomically,
          "each one-short per-facet 10.6 cap clears all five arenas");
    }
  }

  auto nested = generous_budget();
  nested.first_incidence_budget.maximum_source_support_enumeration_count = 0U;
  const auto nested_exhausted = build_and_verify(
      index,
      cloud,
      source,
      nested,
      LbvhTraversalOrder::near_first,
      "the nested 10.6 source-support exhaustion");
  check(
      nested_exhausted.decision ==
              ExactDirectSparseGatewayCandidateDecision::
                  no_gateway_candidate_first_incidence_budget_exhausted &&
          five_scientific_arenas_empty(nested_exhausted) &&
          nested_exhausted.no_partial_scientific_payload_published &&
          !nested_exhausted.all_positive_support_candidates_retained_atomically,
      "an internal 10.6 stop clears projections, tokens, candidates, batches and their index arena together");
}

void test_hostile_verifier_bounds_storage_before_source_replay() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 4U);
  const auto baseline = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      generous_budget(),
      LbvhTraversalOrder::near_first);
  const auto budget = exact_global_budget_for(
      baseline, generous_first_incidence_budget());
  const auto original = build_and_verify(
      index,
      cloud,
      source,
      budget,
      LbvhTraversalOrder::near_first,
      "the hostile gateway verifier baseline");

  const auto verify = [&](const auto& observed) {
    return verify_exact_direct_sparse_gateway_candidate_journal(
        index,
        cloud,
        source.facade,
        source.event_journal,
        source.arm_budget,
        source.arm_journal,
        source.incidence_budget,
        source.incidence_journal,
        budget,
        LbvhTraversalOrder::near_first,
        observed);
  };
  const auto rejected_before_replay = [](const auto& verification) {
    return !verification.observed_storage_within_budget &&
           !verification.source_incidence_journal_freshly_replayed &&
           !verification.fresh_replay_certified &&
           !verification.result_certified;
  };

  auto oversized_projections = original;
  oversized_projections.deletion_projections.resize(
      budget.maximum_deletion_reference_count + 1U);
  auto oversized_tokens = original;
  oversized_tokens.facet_tokens.resize(
      budget.maximum_distinct_facet_count + 1U);
  auto oversized_candidates = original;
  oversized_candidates.gateway_candidates.resize(
      budget.maximum_gateway_candidate_count + 1U);
  auto oversized_batches = original;
  oversized_batches.batches.resize(budget.maximum_batch_count + 1U);
  auto oversized_batch_indices = original;
  oversized_batch_indices.batch_facet_token_indices.resize(
      budget.maximum_batch_facet_reference_count + 1U);
  auto oversized_logical_storage = original;
  oversized_logical_storage.logical_storage_entry_count =
      budget.maximum_logical_storage_entry_count + 1U;
  auto understated_logical_storage = original;
  understated_logical_storage.logical_storage_entry_count = 0U;
  check(
      rejected_before_replay(verify(oversized_projections)) &&
          rejected_before_replay(verify(oversized_tokens)) &&
          rejected_before_replay(verify(oversized_candidates)) &&
          rejected_before_replay(verify(oversized_batches)) &&
          rejected_before_replay(verify(oversized_batch_indices)) &&
          rejected_before_replay(verify(oversized_logical_storage)) &&
          rejected_before_replay(verify(understated_logical_storage)),
      "each hostile oversized arena or falsified logical count is rejected before source replay or record scanning");

  const bool mutation_fixture_nonempty =
      !original.deletion_projections.empty() &&
      !original.facet_tokens.empty() &&
      !original.gateway_candidates.empty() && !original.batches.empty();
  check(
      mutation_fixture_nonempty,
      "the hostile mutation fixture must expose every scientific layer");
  if (!mutation_fixture_nonempty) {
    return;
  }

  auto bad_projection = original;
  bad_projection.deletion_projections.front().facet_token_index =
      original.facet_tokens.size();
  const auto projection_verification = verify(bad_projection);
  auto bad_token = original;
  bad_token.facet_tokens.front().first_incidence_squared_level = level(999);
  const auto token_verification = verify(bad_token);
  auto bad_candidate = original;
  bad_candidate.gateway_candidates.front().added_point_id =
      static_cast<PointId>(cloud.size());
  const auto candidate_verification = verify(bad_candidate);
  auto cross_linked_candidate = original;
  const bool has_distinct_target_token =
      cross_linked_candidate.facet_tokens.size() > 1U;
  if (has_distinct_target_token) {
    auto& candidate = cross_linked_candidate.gateway_candidates.front();
    candidate.facet_token_index =
        (candidate.facet_token_index + 1U) %
        cross_linked_candidate.facet_tokens.size();
  }
  const bool local_predicate_rejects_cross_link =
      has_distinct_target_token &&
      !cross_linked_candidate.certified_partial_refinement() &&
      throws_invalid_argument([&] {
        static_cast<void>(
            reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
                cross_linked_candidate, 0U, 0U));
      });
  auto bad_batch = original;
  ++bad_batch.batches.front().facet_token_index_count;
  const auto batch_verification = verify(bad_batch);
  auto forbidden = original;
  forbidden.gateway_attach_published = true;
  const auto forbidden_verification = verify(forbidden);
  check(
      !projection_verification.deletion_projections_freshly_replayed &&
          !projection_verification.result_certified &&
          !token_verification.facet_tokens_freshly_replayed &&
          !token_verification.result_certified &&
          !candidate_verification.gateway_candidates_freshly_replayed &&
          !candidate_verification.result_certified &&
          local_predicate_rejects_cross_link &&
          !batch_verification.batches_freshly_replayed &&
          !batch_verification.result_certified &&
          !forbidden_verification.no_forbidden_global_structure_materialized &&
          !forbidden_verification.result_certified,
      "fresh replay and the local helper gate reject in-budget cross-layer mutations and an invented attach");
}

void test_authority_source_and_lbvh_divergences_fail_closed() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const std::array<CertifiedPoint3, 3U> foreign_points{
      point(10.0, 0.0), point(14.0, 0.0), point(11.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const CanonicalPointCloud foreign_cloud = canonical_cloud(foreign_points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const MortonLbvhIndex foreign_index = MortonLbvhIndex::build(foreign_cloud);
  const DirectSources source = direct_sources(cloud, 3U);
  const DirectSources foreign_source = direct_sources(foreign_cloud, 3U);
  const auto budget = generous_budget();

  const auto foreign_source_result =
      build_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          foreign_source.facade,
          foreign_source.event_journal,
          foreign_source.arm_budget,
          foreign_source.arm_journal,
          foreign_source.incidence_budget,
          foreign_source.incidence_journal,
          budget,
          LbvhTraversalOrder::near_first);
  auto broken_incidence = source.incidence_journal;
  broken_incidence.families.front().source_event_index =
      source.facade.events.size();
  const auto broken_source_result =
      build_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          broken_incidence,
          budget,
          LbvhTraversalOrder::near_first);

  check(
      foreign_source_result.decision ==
              ExactDirectSparseGatewayCandidateDecision::
                  no_gateway_candidate_source_not_certified &&
          five_scientific_arenas_empty(foreign_source_result) &&
          broken_source_result.decision ==
              ExactDirectSparseGatewayCandidateDecision::
                  no_gateway_candidate_source_not_certified &&
          five_scientific_arenas_empty(broken_source_result),
      "foreign and internally forged source journals fail closed with five empty arenas");
  check(
      throws_invalid_argument([&] {
        static_cast<void>(
            build_exact_direct_sparse_gateway_candidate_journal(
                foreign_index,
                cloud,
                source.facade,
                source.event_journal,
                source.arm_budget,
                source.arm_journal,
                source.incidence_budget,
                source.incidence_journal,
                budget,
                LbvhTraversalOrder::near_first));
      }) &&
          throws_invalid_argument([&] {
            static_cast<void>(
                build_exact_direct_sparse_gateway_candidate_journal(
                    index,
                    foreign_cloud,
                    source.facade,
                    source.event_journal,
                    source.arm_budget,
                    source.arm_journal,
                    source.incidence_budget,
                    source.incidence_journal,
                    budget,
                    LbvhTraversalOrder::near_first));
          }) &&
          throws_invalid_argument([&] {
            static_cast<void>(
                build_exact_direct_sparse_gateway_candidate_journal(
                    index,
                    cloud,
                    source.facade,
                    source.event_journal,
                    source.arm_budget,
                    source.arm_journal,
                    source.incidence_budget,
                    source.incidence_journal,
                    budget,
                    static_cast<LbvhTraversalOrder>(UINT8_C(255))));
          }),
      "a foreign LBVH, divergent cloud and invalid traversal order are rejected before scientific work");
}

void test_contract_metadata_and_sparse_scope() {
  check(
      ExactDirectSparseGatewayCandidateJournalResult::backend ==
              "reference_cpu" &&
          ExactDirectSparseGatewayCandidateJournalResult::profile ==
              "hgp_reduced" &&
          ExactDirectSparseGatewayCandidateJournalResult::mode ==
              "certified" &&
          ExactDirectSparseGatewayCandidateJournalResult::refinement_status ==
              "partial_refinement" &&
          ExactDirectSparseGatewayCandidateJournalResult::public_status ==
              "not_claimed" &&
          ExactDirectSparseGatewayCandidateJournalResult::proof_basis ==
              direct_sparse_gateway_candidate_journal_proof_basis,
      "the gateway candidate journal advertises only its bounded partial-refinement scope");
}

}  // namespace

int main() {
  test_contract_metadata_and_sparse_scope();
  test_complete_empty_source_issues_no_geometry_query();
  test_small_terminal_source_projects_every_deletion();
  test_shared_facet_is_deduplicated_once_and_near_far_agree();
  test_permanent_ac_fixture_uses_real_direct_provenance();
  test_bounded_direct_keys_match_all_explicit_one_point_cofaces();
  test_input_permutations_preserve_the_canonical_event_stream();
  test_k10_factorization_and_k_plus_one_deletion_helper();
  test_global_and_nested_budgets_publish_five_empty_arenas();
  test_hostile_verifier_bounds_storage_before_source_replay();
  test_authority_source_and_lbvh_divergences_fail_closed();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse gateway-candidate test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse gateway-candidate tests passed\n";
  return 0;
}
