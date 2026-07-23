#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_localization.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t locator_authority_id = UINT64_C(0x1095a7);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Action>
void check_invalid_argument(Action&& action, const std::string& message) {
  bool rejected = false;
  try {
    std::forward<Action>(action)();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
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

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget
generous_gateway_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationBudget
generous_localization_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      {maximum, maximum},
  };
}

struct DirectSources {
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
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

struct GatewayFixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseGatewayCandidateBudget gateway_budget;
  ExactDirectSparseGatewayCandidateJournalResult gateway_journal;
};

template <std::size_t Size>
[[nodiscard]] GatewayFixture gateway_fixture(
    const std::array<CertifiedPoint3, Size>& points,
    std::size_t requested_maximum_order,
    const std::string& context) {
  CanonicalPointCloud cloud = canonical_cloud(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  DirectSources source = direct_sources(cloud, requested_maximum_order);
  const auto gateway_budget = generous_gateway_budget();
  auto gateway_journal =
      build_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          gateway_budget,
          LbvhTraversalOrder::near_first);
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
          gateway_budget,
          LbvhTraversalOrder::near_first,
          gateway_journal);
  check(
      gateway_journal.certified_partial_refinement() &&
          verification.result_certified,
      context + " has a freshly certified 10.7 source journal");
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      gateway_budget,
      std::move(gateway_journal),
  };
}

[[nodiscard]] GatewayFixture e5_fixture() {
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return gateway_fixture(points, 5U, "the permanent E5 fixture");
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::copy(point_ids.begin(), point_ids.end(), result.point_ids.begin());
  return result;
}

[[nodiscard]] std::vector<PointId> key_ids(
    const ExactDirectSparseFacetKey& facet_key) {
  return {
      facet_key.point_ids.begin(),
      facet_key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet_key.point_count),
  };
}

[[nodiscard]] ExactDirectSparseFacetWitness witness(
    std::uint64_t replay_token) {
  return {locator_authority_id, replay_token};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
      8U,
      32U,
      320U,
      32U,
      32U,
      32U,
      32U,
      32U,
      640U,
      65U,
      65U,
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator empty_locator(
    std::uint64_t fingerprint_mask = 0U) {
  return build_exact_direct_sparse_positive_facet_locator(
      8U,
      locator_budget(),
      {locator_authority_id, fingerprint_mask});
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator e5_collision_locator() {
  auto locator = empty_locator(0U);
  const std::array<ExactDirectSparseFacetBinding, 2U> bindings{{
      {0U, key({0U, 1U}), 2U, witness(100U)},
      {1U, key({1U, 3U}), 5U, witness(101U)},
  }};
  const auto inserted = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      bindings);
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, 5U, 1U, witness(102U)},
  }};
  const auto united = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      unions,
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      locator.certified_positive_locator() &&
          inserted.certified_committed_batch() &&
          united.certified_committed_batch(),
      "the E5 locator contains two colliding full keys and one parent hop");
  return locator;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationVerification
verify_localization(
    const GatewayFixture& fixture,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    const ExactDirectSparseGatewayCandidateLocalizationResult& observed) {
  return verify_exact_direct_sparse_gateway_candidate_localization(
      fixture.index,
      fixture.cloud,
      fixture.source.facade,
      fixture.source.event_journal,
      fixture.source.arm_budget,
      fixture.source.arm_journal,
      fixture.source.incidence_budget,
      fixture.source.incidence_journal,
      fixture.gateway_budget,
      fixture.gateway_journal,
      query_witness,
      locator,
      budget,
      LbvhTraversalOrder::near_first,
      observed);
}

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationResult
build_verified_localization(
    const GatewayFixture& fixture,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    const std::string& context) {
  auto result = build_exact_direct_sparse_gateway_candidate_localization(
      fixture.index,
      fixture.cloud,
      fixture.source.facade,
      fixture.source.event_journal,
      fixture.source.arm_budget,
      fixture.source.arm_journal,
      fixture.source.incidence_budget,
      fixture.source.incidence_journal,
      fixture.gateway_budget,
      fixture.gateway_journal,
      query_witness,
      locator,
      budget,
      LbvhTraversalOrder::near_first);
  const auto verification =
      verify_localization(fixture, query_witness, locator, budget, result);
  check(
      verification.result_certified,
      context + " is accepted by the fresh 10.9 verifier");
  return result;
}

[[nodiscard]] bool two_scientific_arenas_empty(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result) {
  return result.deletion_projections.empty() &&
         result.localized_facet_tokens.empty();
}

[[nodiscard]] const ExactDirectSparseGatewayLocalizedFacetToken*
find_localized_token(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result,
    const std::vector<PointId>& ids) {
  const auto found = std::find_if(
      result.localized_facet_tokens.begin(),
      result.localized_facet_tokens.end(),
      [&ids](const ExactDirectSparseGatewayLocalizedFacetToken& token) {
        return key_ids(token.facet_key) == ids;
      });
  return found == result.localized_facet_tokens.end() ? nullptr : &*found;
}

[[nodiscard]] ExactDirectSparsePositiveFacetProbeBudget
exact_probe_budget_for(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  ExactDirectSparsePositiveFacetProbeBudget exact{};
  const auto unlimited = generous_localization_budget().facet_probe_budget;
  for (const auto& token : result.localized_facet_tokens) {
    const auto probe = locator.probe_positive_facet(
        token.facet_key, query_witness, unlimited);
    check(
        probe.certified_positive_hit() ||
            probe.certified_unresolved_miss(),
        "every baseline token has a complete independently replayed probe");
    exact.maximum_slot_visit_count = std::max(
        exact.maximum_slot_visit_count, probe.slot_visit_count);
    exact.maximum_component_parent_hop_count = std::max(
        exact.maximum_component_parent_hop_count,
        probe.component_parent_hop_count);
  }
  return exact;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationBudget
exact_budget_for(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result,
    const ExactDirectSparsePositiveFacetProbeBudget& facet_probe_budget) {
  return {
      result.required_source_candidate_scan_count,
      result.required_deletion_reference_count,
      result.required_distinct_facet_count,
      result.required_facet_key_point_count,
      result.counters.slot_visit_count,
      result.counters.component_parent_hop_count,
      result.logical_storage_entry_count,
      facet_probe_budget,
  };
}

void test_complete_empty_source_issues_no_locator_probe() {
  const std::array<CertifiedPoint3, 1U> points{point(0.0, 0.0, 0.0)};
  const auto fixture =
      gateway_fixture(points, 1U, "the complete empty 10.9 source");
  const auto locator = empty_locator();
  const auto result = build_verified_localization(
      fixture,
      witness(50U),
      locator,
      ExactDirectSparseGatewayCandidateLocalizationBudget{},
      "the zero-candidate localization");

  check(
      fixture.gateway_journal.gateway_candidates.empty() &&
          result.certified_partial_refinement() &&
          result.required_source_candidate_scan_count == 0U &&
          result.required_deletion_reference_count == 0U &&
          result.required_distinct_facet_count == 0U &&
          result.required_facet_key_point_count == 0U &&
          result.required_locator_probe_count == 0U &&
          result.logical_storage_entry_count == 0U &&
          result.counters.locator_probe_count == 0U &&
          result.counters.locator_snapshot_check_count == 2U &&
          result.one_locator_probe_per_distinct_full_key &&
          result.common_frozen_locator_snapshot_certified &&
          two_scientific_arenas_empty(result),
      "C=P=D=0 certifies two empty arenas, no probe and exactly entry/final stamp checks");
}

void test_input_authority_gates(const GatewayFixture& fixture) {
  const auto locator = empty_locator();
  const auto budget = generous_localization_budget();
  const auto invoke = [&](const MortonLbvhIndex& index,
                          const ExactDirectSparseFacetWitness& query_witness,
                          const ExactDirectSparsePositiveFacetLocator&
                              selected_locator,
                          LbvhTraversalOrder traversal_order) {
    return build_exact_direct_sparse_gateway_candidate_localization(
        index,
        fixture.cloud,
        fixture.source.facade,
        fixture.source.event_journal,
        fixture.source.arm_budget,
        fixture.source.arm_journal,
        fixture.source.incidence_budget,
        fixture.source.incidence_journal,
        fixture.gateway_budget,
        fixture.gateway_journal,
        query_witness,
        selected_locator,
        budget,
        traversal_order);
  };

  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            fixture.index,
            ExactDirectSparseFacetWitness{},
            locator,
            LbvhTraversalOrder::near_first));
      },
      "a null locator query witness is rejected at entry");
  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            fixture.index,
            {locator_authority_id + 1U, 1U},
            locator,
            LbvhTraversalOrder::near_first));
      },
      "a divergent locator authority is rejected at entry");
  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            fixture.index,
            {locator_authority_id, 0U},
            locator,
            LbvhTraversalOrder::near_first));
      },
      "a zero locator replay token is rejected at entry");

  const ExactDirectSparsePositiveFacetLocator uninitialized_locator;
  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            fixture.index,
            witness(60U),
            uninitialized_locator,
            LbvhTraversalOrder::near_first));
      },
      "an uncertified locator is rejected at entry");

  const std::array<CertifiedPoint3, 1U> foreign_points{
      point(123.0, 456.0, 789.0)};
  const auto foreign_cloud = canonical_cloud(foreign_points);
  const auto foreign_index = MortonLbvhIndex::build(foreign_cloud);
  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            foreign_index,
            witness(61U),
            locator,
            LbvhTraversalOrder::near_first));
      },
      "an LBVH bound to a foreign cloud is rejected at entry");
  check_invalid_argument(
      [&] {
        static_cast<void>(invoke(
            fixture.index,
            witness(62U),
            locator,
            static_cast<LbvhTraversalOrder>(255U)));
      },
      "an invalid traversal order is rejected at entry");

  const auto baseline = build_verified_localization(
      fixture,
      witness(63U),
      locator,
      budget,
      "the input-authority verifier baseline");
  const auto null_verification = verify_localization(
      fixture,
      ExactDirectSparseFacetWitness{},
      locator,
      budget,
      baseline);
  const auto foreign_authority_verification = verify_localization(
      fixture,
      {locator_authority_id + 1U, 1U},
      locator,
      budget,
      baseline);
  const auto zero_replay_token_verification = verify_localization(
      fixture,
      {locator_authority_id, 0U},
      locator,
      budget,
      baseline);
  check(
      !null_verification.trusted_inputs_certified &&
          !null_verification.result_certified &&
          !foreign_authority_verification.trusted_inputs_certified &&
          !foreign_authority_verification.result_certified &&
          !zero_replay_token_verification.trusted_inputs_certified &&
          !zero_replay_token_verification.result_certified,
      "the fresh verifier rejects null, divergent and zero-token locator witnesses before replay");
}

void test_e5_shared_collision_hits_and_latent_misses(
    const GatewayFixture& fixture) {
  auto locator = e5_collision_locator();
  const auto locator_before = locator;
  const auto query_witness = witness(200U);
  const auto result = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the E5 shared-key collision localization");

  std::vector<std::size_t> ac_candidate_indices;
  for (std::size_t candidate_index = 0U;
       candidate_index < fixture.gateway_journal.gateway_candidates.size();
       ++candidate_index) {
    const auto& candidate =
        fixture.gateway_journal.gateway_candidates[candidate_index];
    if (candidate.facet_token_index >=
        fixture.gateway_journal.facet_tokens.size()) {
      continue;
    }
    const auto& source_token =
        fixture.gateway_journal.facet_tokens[candidate.facet_token_index];
    if (key_ids(source_token.source_facet_key) ==
            std::vector<PointId>({1U, 3U}) &&
        (candidate.added_point_id == 0U ||
         candidate.added_point_id == 4U)) {
      ac_candidate_indices.push_back(candidate_index);
    }
  }

  std::size_t selected_projection_count = 0U;
  std::set<std::size_t> ac_localized_token_indices;
  bool every_projection_reconstructs_and_keeps_source_batch = true;
  for (const std::size_t candidate_index : ac_candidate_indices) {
    const auto& candidate =
        fixture.gateway_journal.gateway_candidates[candidate_index];
    const auto expected_batch =
        fixture.gateway_journal.facet_tokens[candidate.facet_token_index]
            .batch_index;
    for (const auto& projection : result.deletion_projections) {
      if (projection.gateway_candidate_index != candidate_index) {
        continue;
      }
      ++selected_projection_count;
      if (projection.localized_facet_token_index >=
          result.localized_facet_tokens.size()) {
        every_projection_reconstructs_and_keeps_source_batch = false;
        continue;
      }
      const auto reconstructed =
          reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
              fixture.gateway_journal,
              candidate_index,
              projection.removed_union_point_index);
      const auto& localized = result.localized_facet_tokens[
          projection.localized_facet_token_index];
      every_projection_reconstructs_and_keeps_source_batch =
          every_projection_reconstructs_and_keeps_source_batch &&
          projection.source_batch_index == expected_batch &&
          localized.facet_key == reconstructed;
      if (key_ids(reconstructed) == std::vector<PointId>({1U, 3U})) {
        ac_localized_token_indices.insert(
            projection.localized_facet_token_index);
      }
    }
  }

  const auto* ac = find_localized_token(result, {1U, 3U});
  const auto* other_hit = find_localized_token(result, {0U, 1U});
  const auto* collision_miss = find_localized_token(result, {0U, 3U});
  std::set<std::vector<PointId>> positive_keys;
  bool every_latent_token_has_no_payload = true;
  for (const auto& token : result.localized_facet_tokens) {
    if (token.disposition ==
        ExactDirectSparseGatewayFacetLocalizationDisposition::
            relative_positive) {
      positive_keys.insert(key_ids(token.facet_key));
    } else if (
        token.disposition ==
        ExactDirectSparseGatewayFacetLocalizationDisposition::
            latent_unresolved) {
      every_latent_token_has_no_payload =
          every_latent_token_has_no_payload &&
          !token.component_handle_present &&
          !token.source_binding_witness_present;
    }
  }
  const std::set<std::vector<PointId>> expected_positive_keys{
      {0U, 1U}, {1U, 3U}};
  check(
      ac_candidate_indices.size() == 2U &&
          selected_projection_count == 6U &&
          ac_localized_token_indices.size() == 1U &&
          every_projection_reconstructs_and_keeps_source_batch,
      "ACD and ACE reconstruct six deletions, retain their batch and share one AC token");
  check(
      ac != nullptr &&
          ac->disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  relative_positive &&
          ac->component_handle_present && ac->component_handle == 1U &&
          ac->source_binding_witness_present &&
          ac->source_binding_witness == witness(101U) &&
          other_hit != nullptr &&
          other_hit->disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  relative_positive &&
          other_hit->component_handle_present &&
          other_hit->component_handle == 2U &&
          collision_miss != nullptr &&
          collision_miss->disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  latent_unresolved &&
          !collision_miss->component_handle_present &&
          !collision_miss->source_binding_witness_present,
      "full-key comparison separates colliding hits from a latent miss and resolves the AC parent");
  check(
      result.counters.locator_probe_count ==
              result.localized_facet_tokens.size() &&
          result.counters.locator_probe_count ==
              result.required_locator_probe_count &&
          result.deletion_projections.size() ==
              result.required_deletion_reference_count &&
          result.localized_facet_tokens.size() ==
              result.required_distinct_facet_count &&
          result.required_deletion_reference_count <=
              11U * result.required_source_candidate_scan_count &&
          result.required_distinct_facet_count <=
              result.required_deletion_reference_count &&
          result.logical_storage_entry_count ==
              result.required_deletion_reference_count +
                  result.required_distinct_facet_count +
                  result.required_facet_key_point_count &&
          result.counters.locator_snapshot_check_count ==
              result.required_distinct_facet_count + 2U &&
          result.counters.relative_positive_facet_count == 2U &&
          result.required_distinct_facet_count >= 2U &&
          result.counters.latent_unresolved_facet_count ==
              result.required_distinct_facet_count - 2U &&
          positive_keys == expected_positive_keys &&
          every_latent_token_has_no_payload &&
          result.counters.equal_fingerprint_distinct_key_count != 0U &&
          result.counters.full_key_comparison_count != 0U &&
          result.one_locator_probe_per_distinct_full_key &&
          result.distinct_full_keys_globally_deduplicated,
      "E5 performs exactly one scientific probe per complete distinct key under forced collisions");
  check(
      locator == locator_before &&
          result.locator_snapshot_stamp == locator.snapshot_stamp() &&
          !result.locator_state_mutated &&
          !result.locator_batch_committed,
      "10.9 leaves every durable locator byte, counter, batch and stamp unchanged");
}

void test_k10_121_occurrences_collapse_to_eleven_tokens() {
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
  const auto fixture =
      gateway_fixture(points, 10U, "the K=10 collinear boundary");
  const auto locator = empty_locator();
  const auto query_witness = witness(300U);
  const auto result = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the K=10 candidate localization");

  const auto family = std::find_if(
      fixture.source.incidence_journal.families.begin(),
      fixture.source.incidence_journal.families.end(),
      [&fixture](
          const ExactDirectClosedSaddleIncidenceFamilyRecord& candidate) {
        const auto& event =
            fixture.source.facade.events[candidate.source_event_index];
        return candidate.order == 10U && event.support_size == 2U &&
               event.interior_ids.size() == 9U;
      });
  check(
      family != fixture.source.incidence_journal.families.end(),
      "the K=10 fixture exposes its diameter family");
  if (family == fixture.source.incidence_journal.families.end()) {
    return;
  }

  std::set<std::size_t> source_token_indices;
  for (const auto& projection :
       fixture.gateway_journal.deletion_projections) {
    if (projection.source_family_index == family->family_index) {
      source_token_indices.insert(projection.facet_token_index);
    }
  }
  std::set<std::size_t> candidate_indices;
  for (const std::size_t token_index : source_token_indices) {
    if (token_index >= fixture.gateway_journal.facet_tokens.size()) {
      continue;
    }
    const auto& token = fixture.gateway_journal.facet_tokens[token_index];
    for (std::size_t local = 0U;
         local < token.gateway_candidate_count;
         ++local) {
      candidate_indices.insert(token.gateway_candidate_offset + local);
    }
  }

  std::size_t projection_count = 0U;
  std::set<std::size_t> localized_token_indices;
  bool every_projection_is_exact_width_ten = true;
  for (const auto& projection : result.deletion_projections) {
    if (!candidate_indices.contains(projection.gateway_candidate_index)) {
      continue;
    }
    ++projection_count;
    if (projection.localized_facet_token_index >=
        result.localized_facet_tokens.size()) {
      every_projection_is_exact_width_ten = false;
      continue;
    }
    localized_token_indices.insert(projection.localized_facet_token_index);
    const auto reconstructed =
        reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
            fixture.gateway_journal,
            projection.gateway_candidate_index,
            projection.removed_union_point_index);
    every_projection_is_exact_width_ten =
        every_projection_is_exact_width_ten &&
        reconstructed.point_count == 10U &&
        result.localized_facet_tokens[projection.localized_facet_token_index]
                .facet_key == reconstructed;
  }
  for (const std::size_t token_index : localized_token_indices) {
    every_projection_is_exact_width_ten =
        every_projection_is_exact_width_ten &&
        token_index < result.localized_facet_tokens.size() &&
        result.localized_facet_tokens[token_index].facet_key.point_count ==
            10U;
  }

  check(
      source_token_indices.size() == 11U &&
          candidate_indices.size() == 11U && projection_count == 121U &&
          localized_token_indices.size() == 11U &&
          every_projection_is_exact_width_ten,
      "eleven factorized K=10 candidates globally deduplicate 121 deletion occurrences into eleven ten-point keys");
  check(
      std::all_of(
          result.localized_facet_tokens.begin(),
          result.localized_facet_tokens.end(),
          [](const ExactDirectSparseGatewayLocalizedFacetToken& token) {
            return token.facet_key.point_count <= 10U &&
                   token.disposition ==
                       ExactDirectSparseGatewayFacetLocalizationDisposition::
                           latent_unresolved &&
                   !token.component_handle_present &&
                   !token.source_binding_witness_present;
          }) &&
          result.counters.relative_positive_facet_count == 0U &&
          result.counters.latent_unresolved_facet_count ==
              result.required_distinct_facet_count &&
          !result.eleven_point_coface_keys_materialized &&
          !result.gamma_cells_or_higher_order_delaunay_materialized &&
          !result.forbidden_global_structure_materialized,
      "the K=10 boundary persists neither an eleven-point coface key nor a global higher-order structure");
}

void test_exact_and_one_short_budgets_are_atomic(
    const GatewayFixture& fixture) {
  const auto locator = e5_collision_locator();
  const auto query_witness = witness(400U);
  const auto baseline = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the localization budget baseline");
  const auto exact_probe =
      exact_probe_budget_for(baseline, query_witness, locator);
  const auto exact = exact_budget_for(baseline, exact_probe);
  const auto exact_result = build_verified_localization(
      fixture,
      query_witness,
      locator,
      exact,
      "all exact localization caps");
  check(
      exact_result.deletion_projections == baseline.deletion_projections &&
          exact_result.localized_facet_tokens ==
              baseline.localized_facet_tokens &&
          exact_result.counters == baseline.counters,
      "the seven exact global caps and two exact local caps reproduce both scientific arenas");

  const bool every_cap_exercised =
      exact.maximum_source_candidate_scan_count != 0U &&
      exact.maximum_deletion_reference_count != 0U &&
      exact.maximum_distinct_facet_count != 0U &&
      exact.maximum_facet_key_point_count != 0U &&
      exact.maximum_aggregate_slot_visit_count != 0U &&
      exact.maximum_aggregate_component_parent_hop_count != 0U &&
      exact.maximum_logical_storage_entry_count != 0U &&
      exact.facet_probe_budget.maximum_slot_visit_count != 0U &&
      exact.facet_probe_budget.maximum_component_parent_hop_count != 0U;
  check(
      every_cap_exercised,
      "the colliding E5 fixture exercises all seven global and both local caps");
  if (!every_cap_exercised) {
    return;
  }

  using BudgetMember =
      std::size_t ExactDirectSparseGatewayCandidateLocalizationBudget::*;
  const std::array<BudgetMember, 7U> global_caps{{
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_source_candidate_scan_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_deletion_reference_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_distinct_facet_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_facet_key_point_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_aggregate_slot_visit_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_aggregate_component_parent_hop_count,
      &ExactDirectSparseGatewayCandidateLocalizationBudget::
          maximum_logical_storage_entry_count,
  }};
  for (std::size_t cap_index = 0U;
       cap_index < global_caps.size();
       ++cap_index) {
    auto insufficient = exact;
    --(insufficient.*global_caps[cap_index]);
    const auto result = build_verified_localization(
        fixture,
        query_witness,
        locator,
        insufficient,
        "global one-short localization cap " +
            std::to_string(cap_index));
    check(
        result.decision ==
                ExactDirectSparseGatewayCandidateLocalizationDecision::
                    no_localization_budget_exhausted &&
            result.certified_atomic_failure() &&
            two_scientific_arenas_empty(result) &&
            result.no_partial_scientific_payload_published &&
            result.counters.slot_visit_count <=
                insufficient.maximum_aggregate_slot_visit_count &&
            result.counters.component_parent_hop_count <=
                insufficient
                    .maximum_aggregate_component_parent_hop_count,
        "a one-short global cap leaves both scientific arenas empty");
    if (cap_index == 0U) {
      check(
          !result.source_gateway_candidate_journal_freshly_replayed &&
              result.counters.source_candidate_scan_count == 0U &&
              result.required_deletion_reference_count == 0U &&
              result.counters.locator_probe_count == 0U,
          "the source-candidate cap fails before the expensive 10.7 replay, deletion scan and locator probes");
    }
  }

  auto short_slot = exact;
  --short_slot.facet_probe_budget.maximum_slot_visit_count;
  const auto slot_exhausted = build_verified_localization(
      fixture,
      query_witness,
      locator,
      short_slot,
      "the one-short per-facet slot cap");
  auto short_parent = exact;
  --short_parent.facet_probe_budget.maximum_component_parent_hop_count;
  const auto parent_exhausted = build_verified_localization(
      fixture,
      query_witness,
      locator,
      short_parent,
      "the one-short per-facet parent cap");
  check(
      slot_exhausted.decision ==
              ExactDirectSparseGatewayCandidateLocalizationDecision::
                  no_localization_locator_probe_budget_exhausted &&
          parent_exhausted.decision ==
              ExactDirectSparseGatewayCandidateLocalizationDecision::
                  no_localization_locator_probe_budget_exhausted &&
          slot_exhausted.certified_atomic_failure() &&
          parent_exhausted.certified_atomic_failure() &&
          two_scientific_arenas_empty(slot_exhausted) &&
          two_scientific_arenas_empty(parent_exhausted) &&
          slot_exhausted.no_partial_scientific_payload_published &&
          parent_exhausted.no_partial_scientific_payload_published,
      "slot and parent probe exhaustion are atomic failures, never latent misses");
}

void test_stamp_staleness_and_latent_promotion_require_new_build(
    const GatewayFixture& fixture) {
  auto locator = e5_collision_locator();
  const auto query_witness = witness(500U);
  const auto old_result = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the pre-mutation stamp baseline");
  const auto* old_latent = find_localized_token(old_result, {0U, 3U});
  check(
      old_latent != nullptr &&
          old_latent->disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  latent_unresolved,
      "CD starts as a latent locator miss without an isolation claim");

  const auto before_empty_commit = locator.snapshot_stamp();
  const auto empty_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  const auto after_empty_commit = locator.snapshot_stamp();
  const auto stale_verification = verify_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      old_result);
  check(
      empty_commit.certified_committed_batch() &&
          before_empty_commit != after_empty_commit &&
          !stale_verification.locator_snapshot_matches_observed_build &&
          !stale_verification.fresh_replay_certified &&
          !stale_verification.result_certified,
      "even an empty committed batch invalidates a previously observed locator stamp");

  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {0U, key({0U, 3U}), 3U, witness(501U)},
  }};
  const auto linked = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      binding);
  const auto fresh = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the post-binding localization rebuild");
  const auto* promoted = find_localized_token(fresh, {0U, 3U});
  check(
      linked.certified_committed_batch() && promoted != nullptr &&
          promoted->disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  relative_positive &&
          promoted->component_handle_present &&
          promoted->component_handle == 3U &&
          promoted->source_binding_witness_present &&
          promoted->source_binding_witness == witness(501U) &&
          fresh.locator_snapshot_stamp == locator.snapshot_stamp() &&
          fresh.locator_snapshot_stamp != old_result.locator_snapshot_stamp,
      "a latent key becomes positive only in a fresh build against the newly committed snapshot");
}

void test_hostile_verifier_bounds_and_replays_every_layer(
    const GatewayFixture& fixture) {
  const auto locator = e5_collision_locator();
  const auto query_witness = witness(600U);
  const auto baseline = build_verified_localization(
      fixture,
      query_witness,
      locator,
      generous_localization_budget(),
      "the hostile-verifier baseline");
  const auto exact_probe =
      exact_probe_budget_for(baseline, query_witness, locator);
  const auto exact = exact_budget_for(baseline, exact_probe);
  const auto original = build_verified_localization(
      fixture,
      query_witness,
      locator,
      exact,
      "the exact hostile-verifier baseline");

  const auto rejected_before_source_replay = [&](const auto& observed) {
    const auto verification =
        verify_localization(fixture, query_witness, locator, exact, observed);
    return !verification.observed_storage_within_budget &&
           !verification.source_gateway_candidate_journal_freshly_replayed &&
           !verification.fresh_replay_certified &&
           !verification.result_certified;
  };
  auto oversized_projections = original;
  oversized_projections.deletion_projections.resize(
      exact.maximum_deletion_reference_count + 1U);
  auto oversized_tokens = original;
  oversized_tokens.localized_facet_tokens.resize(
      exact.maximum_distinct_facet_count + 1U);
  auto oversized_key = original;
  oversized_key.localized_facet_tokens.front().facet_key.point_count = 11U;
  auto overstated_storage = original;
  overstated_storage.logical_storage_entry_count =
      exact.maximum_logical_storage_entry_count + 1U;
  auto understated_storage = original;
  understated_storage.logical_storage_entry_count = 0U;
  check(
      rejected_before_source_replay(oversized_projections) &&
          rejected_before_source_replay(oversized_tokens) &&
          rejected_before_source_replay(oversized_key) &&
          rejected_before_source_replay(overstated_storage) &&
          rejected_before_source_replay(understated_storage),
      "hostile arena sizes, key widths and logical counts are rejected before any 10.7 replay");

  check(
      !original.deletion_projections.empty() &&
          !original.localized_facet_tokens.empty(),
      "the hostile fixture exposes both scientific layers");
  if (original.deletion_projections.empty() ||
      original.localized_facet_tokens.empty()) {
    return;
  }
  const auto rejected = [&](auto observed, const std::string& context) {
    const auto verification =
        verify_localization(fixture, query_witness, locator, exact, observed);
    check(!verification.result_certified, context);
  };

  auto bad_projection_token = original;
  bad_projection_token.deletion_projections.front()
      .localized_facet_token_index = original.localized_facet_tokens.size();
  rejected(
      std::move(bad_projection_token),
      "an out-of-range projection token is rejected");
  auto bad_removed_position = original;
  bad_removed_position.deletion_projections.front()
      .removed_union_point_index = 11U;
  check(
      !bad_removed_position.certified_partial_refinement(),
      "the local shape predicate rejects an impossible removed position");
  rejected(
      std::move(bad_removed_position),
      "a falsified removed position is rejected");
  auto bad_source_batch = original;
  ++bad_source_batch.deletion_projections.front().source_batch_index;
  check(
      !bad_source_batch.certified_partial_refinement(),
      "the local shape predicate rejects a batch change inside one candidate slice");
  rejected(
      std::move(bad_source_batch),
      "a falsified source batch is rejected");
  auto bad_key = original;
  const auto same_width_key = std::find_if(
      original.localized_facet_tokens.begin() + 1,
      original.localized_facet_tokens.end(),
      [&original](const ExactDirectSparseGatewayLocalizedFacetToken& token) {
        return token.facet_key.point_count ==
                   original.localized_facet_tokens.front()
                       .facet_key.point_count &&
               token.facet_key !=
                   original.localized_facet_tokens.front().facet_key;
      });
  check(
      same_width_key != original.localized_facet_tokens.end(),
      "the hostile fixture exposes two distinct keys of one cardinality");
  if (same_width_key != original.localized_facet_tokens.end()) {
    bad_key.localized_facet_tokens.front().facet_key =
        same_width_key->facet_key;
  }
  rejected(std::move(bad_key), "a substituted complete facet key is rejected");
  auto bad_disposition = original;
  bad_disposition.localized_facet_tokens.front().disposition =
      ExactDirectSparseGatewayFacetLocalizationDisposition::not_certified;
  rejected(
      std::move(bad_disposition),
      "a falsified localization disposition is rejected");

  const auto positive = std::find_if(
      original.localized_facet_tokens.begin(),
      original.localized_facet_tokens.end(),
      [](const ExactDirectSparseGatewayLocalizedFacetToken& token) {
        return token.disposition ==
               ExactDirectSparseGatewayFacetLocalizationDisposition::
                   relative_positive;
      });
  check(
      positive != original.localized_facet_tokens.end(),
      "the hostile fixture contains a positive token payload");
  if (positive != original.localized_facet_tokens.end()) {
    const std::size_t positive_index = static_cast<std::size_t>(
        std::distance(original.localized_facet_tokens.begin(), positive));
    auto bad_handle = original;
    ++bad_handle.localized_facet_tokens[positive_index].component_handle;
    rejected(std::move(bad_handle), "a falsified positive handle is rejected");
    auto bad_witness = original;
    ++bad_witness.localized_facet_tokens[positive_index]
          .source_binding_witness.replay_token;
    rejected(
        std::move(bad_witness),
        "a falsified positive binding witness is rejected");
  }

  auto bad_counter = original;
  ++bad_counter.counters.locator_probe_count;
  rejected(std::move(bad_counter), "a falsified aggregate counter is rejected");
  auto bad_stamp = original;
  ++bad_stamp.locator_snapshot_stamp.committed_batch_count;
  rejected(std::move(bad_stamp), "a forged locator stamp is rejected");
  auto false_alignment = original;
  false_alignment.locator_snapshot_batch_level_alignment_claimed = true;
  rejected(
      std::move(false_alignment),
      "an invented snapshot-to-batch alignment claim is rejected");
  auto false_isolation = original;
  false_isolation.missing_facet_means_isolated = true;
  rejected(
      std::move(false_isolation),
      "an invented isolation claim is rejected");
  auto false_attach = original;
  false_attach.gateway_attach_published = true;
  rejected(
      std::move(false_attach),
      "an invented GatewayAttach publication is rejected");
}

void test_falsified_10_7_source_fails_closed(
    const GatewayFixture& fixture) {
  auto falsified_source = fixture.gateway_journal;
  check(
      !falsified_source.gateway_candidates.empty(),
      "the source-replay fixture exposes a 10.7 candidate to falsify");
  if (falsified_source.gateway_candidates.empty()) {
    return;
  }
  ++falsified_source.gateway_candidates.front().gateway_candidate_index;
  const auto locator = e5_collision_locator();
  const auto locator_before = locator;
  const auto result =
      build_exact_direct_sparse_gateway_candidate_localization(
          fixture.index,
          fixture.cloud,
          fixture.source.facade,
          fixture.source.event_journal,
          fixture.source.arm_budget,
          fixture.source.arm_journal,
          fixture.source.incidence_budget,
          fixture.source.incidence_journal,
          fixture.gateway_budget,
          falsified_source,
          witness(650U),
          locator,
          generous_localization_budget(),
          LbvhTraversalOrder::near_first);
  check(
      result.decision ==
              ExactDirectSparseGatewayCandidateLocalizationDecision::
                  no_localization_source_not_certified &&
          result.certified_atomic_failure() &&
          !result.source_gateway_candidate_journal_freshly_replayed &&
          result.counters.locator_probe_count == 0U &&
          two_scientific_arenas_empty(result) && locator == locator_before,
      "a falsified 10.7 journal fails closed before any locator probe or scientific publication");
}

void test_contract_metadata_and_partial_scope(const GatewayFixture& fixture) {
  const auto locator = e5_collision_locator();
  const auto result = build_verified_localization(
      fixture,
      witness(700U),
      locator,
      generous_localization_budget(),
      "the metadata fixture");
  check(
      ExactDirectSparseGatewayCandidateLocalizationResult::backend ==
              "reference_cpu" &&
          ExactDirectSparseGatewayCandidateLocalizationResult::profile ==
              "hgp_reduced" &&
          ExactDirectSparseGatewayCandidateLocalizationResult::mode ==
              "certified" &&
          ExactDirectSparseGatewayCandidateLocalizationResult::
                  refinement_status == "partial_refinement" &&
          ExactDirectSparseGatewayCandidateLocalizationResult::
                  public_status == "not_claimed" &&
          ExactDirectSparseGatewayCandidateLocalizationResult::proof_basis ==
              direct_sparse_gateway_candidate_localization_proof_basis,
      "10.9 advertises the reference CPU certified partial-refinement axes only");
  check(
      result.certified_partial_refinement() && result.certified_outcome() &&
          result.partial_refinement_only && !result.public_status_claimed &&
          result.scope ==
              ExactDirectSparseGatewayCandidateLocalizationScope::
                  candidate_deletion_facets_relative_to_frozen_positive_locator_only &&
          !result.external_binding_authority_replayed &&
          !result.locator_snapshot_batch_level_alignment_claimed &&
          !result.missing_facet_means_isolated &&
          !result.singleton_component_created &&
          !result.root_union_or_forest_mutated &&
          !result.gateway_attach_published &&
          !result.eleven_point_coface_keys_materialized &&
          !result.gamma_cells_or_higher_order_delaunay_materialized &&
          !result.forbidden_global_structure_materialized,
      "the successful result stays relative, non-temporal and free of singleton, quotient, forest, attach, Gamma and higher-order Delaunay claims");

  ExactDirectSparseGatewayCandidateLocalizationResult forged_failure;
  forged_failure.decision =
      ExactDirectSparseGatewayCandidateLocalizationDecision::
          no_localization_budget_exhausted;
  forged_failure.scope =
      ExactDirectSparseGatewayCandidateLocalizationScope::
          candidate_deletion_facets_relative_to_frozen_positive_locator_only;
  forged_failure.no_partial_scientific_payload_published = true;
  forged_failure.common_frozen_locator_snapshot_certified = true;
  forged_failure.partial_refinement_only = true;
  check(
      !forged_failure.certified_atomic_failure() &&
          !forged_failure.certified_outcome(),
      "an almost-empty hand-forged failure lacks a witness-bound locator stamp and is not self-certified");
}

}  // namespace

int main() {
  const auto fixture = e5_fixture();
  test_complete_empty_source_issues_no_locator_probe();
  test_input_authority_gates(fixture);
  test_e5_shared_collision_hits_and_latent_misses(fixture);
  test_exact_and_one_short_budgets_are_atomic(fixture);
  test_stamp_staleness_and_latent_promotion_require_new_build(fixture);
  test_hostile_verifier_bounds_and_replays_every_layer(fixture);
  test_falsified_10_7_source_fails_closed(fixture);
  test_contract_metadata_and_partial_scope(fixture);
  test_k10_121_occurrences_collapse_to_eleven_tokens();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse gateway-candidate localization test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse gateway-candidate localization tests passed\n";
  return 0;
}
