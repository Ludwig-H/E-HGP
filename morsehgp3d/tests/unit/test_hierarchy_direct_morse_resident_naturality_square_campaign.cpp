#include "morsehgp3d/hierarchy/direct_morse_resident_naturality_square_campaign.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    direct_morse_resident_naturality_square_campaign_schema_version == 1U);
static_assert(
    direct_morse_resident_vertical_external_target_authority_binding_schema_version ==
    1U);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(!std::is_copy_assignable_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(std::is_nothrow_move_assignable_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

// ---------------------------------------------------------------------------
// Fixture chain reused verbatim from
// test_hierarchy_direct_morse_normalized_h0_product_session.cpp: it is the
// only in-tree chain that seals the all-orders vertical bridge over the live
// normalized incidence-complete v7 candidate source that the campaign
// requires (pair session -> higher support -> facade -> gateway -> source
// plan -> rank window -> incidence reduction authority -> normalized resident
// session -> K2/K1 bridge -> all-orders bridge -> prepare/commit -> seal).
// ---------------------------------------------------------------------------

[[nodiscard]] ExactPairSupportStreamBudget pair_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget higher_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget arm_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget incidence_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget first_incidence_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget gateway_budget() {
  return {
      4096U,
      65536U,
      65536U,
      655360U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      first_incidence_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
gateway_identity_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
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

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget source_plan_budget() {
  return {
      4096U,
      65536U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      gateway_identity_budget(),
  };
}

[[nodiscard]] ExactDirectNormalizedH0ResidentAdapterBudget adapter_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      67108864U,
      67108864U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      268435456U,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget closure_budget() {
  return {
      1024U,
      4096U,
      4096U,
      8193U,
      {
          ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U},
          morsehgp3d::spatial::ExactLbvhTopKBudget{
              1048576U,
              1048576U,
              1048576U,
              1048576U,
              1024U,
              10U,
              10U},
          ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U},
      },
  };
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget frozen_budget() {
  const auto maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum,
  };
}

[[nodiscard]] ExactDirectMorseUnifiedResidentSessionBudget resident_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget{};
  budget.locator = {
      1024U, 1024U, 10240U, 1024U, 1024U, 1024U,
      1024U, 1024U, 10240U, 4097U, 4097U,
  };
  budget.probe = {4097U, 1024U};
  budget.frozen_batch = frozen_budget();
  budget.maximum_facet_resolution_count = 1024U;
  budget.maximum_prior_root_coverage_count = 1024U;
  budget.maximum_prior_root_coverage_point_reference_count = 10240U;
  budget.maximum_latent_carrier_coverage_count = 1024U;
  budget.maximum_latent_carrier_coverage_point_reference_count = 10240U;
  budget.maximum_fresh_facet_miniball_count = 1024U;
  budget.maximum_fresh_facet_miniball_support_enumeration_count = 1048576U;
  budget.maximum_normalized_facet_observation_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_simultaneous_scratch_entry_count =
      16384U;
  budget.maximum_resident_root_count = 1024U;
  budget.maximum_resident_root_point_reference_count = 10240U;
  budget.maximum_resident_component_latent_point_reference_count = 10240U;
  budget.maximum_group_record_count = 1024U;
  budget.maximum_group_child_reference_count = 10240U;
  budget.maximum_group_coverage_delta_point_reference_count = 10240U;
  budget.sparse_delta = {
      1024U, 10240U, 1024U, 1024U, 10240U,
      1024U, 10240U, 10240U, 2U,
  };
  return budget;
}

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionBudget k1_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return {
      point_count,
      64U,
      edge_count,
      edge_count,
      edge_count,
      2U * edge_count,
      point_count,
      point_count,
      point_count,
      point_count,
      point_count,
      2U * point_count,
      4096U,
  };
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult events;
  ExactDirectSaddleArmSeedBudget arm;
  ExactDirectSaddleArmSeedJournalResult arms;
  ExactDirectClosedSaddleIncidenceBudget incidence;
  ExactDirectClosedSaddleIncidenceJournalResult incidences;
};

struct Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources direct;
  ExactDirectSparseGatewayCandidateBudget gateway_cap;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectNormalizedH0SourcePlanBudget plan_cap;
  ExactDirectNormalizedH0SourcePlanResult plan;
  ExactDirectRankWindowSaturatedH0Authority rank;
  ExactDirectNormalizedH0IncidenceReductionAuthority incidence_authority;
  K1ExactBoruvkaResult boruvka;
};

[[nodiscard]] Fixture fixture_from_points(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order) {
  auto cloud = CanonicalPointCloud::rejecting_duplicates(points);
  auto index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal,
      pair,
      higher);
  auto events = build_exact_direct_morse_event_journal(cloud, facade);
  auto arm = arm_budget();
  auto arms = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, events, arm);
  auto incidence = incidence_budget();
  auto incidences = build_exact_direct_closed_saddle_incidence_journal(
      cloud, facade, events, arm, arms, incidence);
  DirectSources direct{
      std::move(facade),
      std::move(events),
      arm,
      std::move(arms),
      incidence,
      std::move(incidences),
  };
  auto gateway_cap = gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      direct.facade,
      direct.events,
      direct.arm,
      direct.arms,
      direct.incidence,
      direct.incidences,
      gateway_cap,
      LbvhTraversalOrder::near_first);
  auto plan_cap = source_plan_budget();
  auto plan = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      direct.facade,
      direct.events,
      direct.arm,
      direct.arms,
      direct.incidence,
      direct.incidences,
      gateway_cap,
      LbvhTraversalOrder::near_first,
      gateway,
      plan_cap);
  auto rank = build_exact_direct_rank_window_saturated_h0_authority(
      direct.facade);
  auto authority =
      build_exact_direct_normalized_h0_incidence_reduction_authority(
          index,
          cloud,
          direct.facade,
          direct.events,
          direct.arm,
          direct.arms,
          direct.incidence,
          direct.incidences,
          gateway_cap,
          LbvhTraversalOrder::near_first,
          gateway,
          plan_cap,
          plan,
          rank);
  auto boruvka = build_exact_lbvh_boruvka(index, cloud);
  return {
      std::move(cloud),
      std::move(index),
      std::move(direct),
      gateway_cap,
      std::move(gateway),
      plan_cap,
      std::move(plan),
      std::move(rank),
      std::move(authority),
      std::move(boruvka),
  };
}

// The n=6, K=4 exact moment curve: the smallest fixture whose sealed
// normalized bridge commits K2, K3 and K4 batches, so the campaign replays
// adjacent order pairs 2->1, 3->2 and 4->3 across consecutive closed cuts.
// K=4 < n=6 keeps the terminal K=n component image out of scope here.
[[nodiscard]] Fixture campaign_fixture() {
  const std::array points{
      point(-2.0, 4.0, -8.0),
      point(-1.0, 1.0, -1.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(2.0, 4.0, 8.0),
      point(3.0, 9.0, 27.0),
  };
  return fixture_from_points(points, 4U);
}

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
resident(Fixture& source, std::uint64_t authority_id) {
  return initialize_exact_direct_normalized_h0_incidence_complete_resident_session(
      source.index,
      source.cloud,
      source.direct.facade,
      source.direct.events,
      source.direct.arm,
      source.direct.arms,
      source.direct.incidence,
      source.direct.incidences,
      source.gateway_cap,
      LbvhTraversalOrder::near_first,
      source.gateway,
      source.plan_cap,
      source.plan,
      source.rank,
      source.incidence_authority,
      closure_budget(),
      {},
      LbvhTraversalOrder::near_first,
      adapter_budget(),
      authority_id,
      resident_budget());
}

[[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutBridgeBudget
k2_bridge_budget(const ExactDirectSparseUnifiedLevelPlanResult& plan) {
  ExactDirectMorseResidentK2K1ClosedCutBridgeBudget budget;
  std::size_t batch_count = 0U;
  std::size_t saddle_count = 0U;
  std::size_t birth_count = 0U;
  std::size_t maximum_batch_saddle_count = 0U;
  std::size_t maximum_batch_birth_count = 0U;
  for (const auto& batch : plan.batches) {
    if (batch.order != 2U) {
      continue;
    }
    ++batch_count;
    std::size_t batch_saddle_count = 0U;
    std::size_t batch_birth_count = 0U;
    for (std::size_t local = 0U;
         local < batch.direct_reference_count;
         ++local) {
      if (plan.direct_references[batch.direct_reference_offset + local].role ==
          ExactDirectMorseH0Role::saddle) {
        ++batch_saddle_count;
        ++saddle_count;
      } else {
        ++batch_birth_count;
        ++birth_count;
      }
    }
    maximum_batch_saddle_count =
        std::max(maximum_batch_saddle_count, batch_saddle_count);
    maximum_batch_birth_count =
        std::max(maximum_batch_birth_count, batch_birth_count);
  }
  budget.maximum_committed_k2_batch_count = batch_count;
  budget.maximum_committed_k2_group_count = 4096U;
  budget.maximum_prepared_k2_group_count = 4096U;
  budget.maximum_committed_k2_direct_saddle_group_binding_count =
      saddle_count;
  budget.maximum_prepared_k2_direct_saddle_group_binding_count =
      maximum_batch_saddle_count;
  budget.maximum_committed_k2_direct_birth_k1_binding_count = birth_count;
  budget.maximum_prepared_k2_direct_birth_k1_binding_count =
      maximum_batch_birth_count;
  budget.maximum_direct_birth_k1_singleton_root_query_count =
      2U * maximum_batch_birth_count;
  budget.maximum_group_coverage_point_reference_scan_count = 65536U;
  budget.maximum_group_point_scratch_count = 65536U;
  budget.maximum_distinct_group_point_count = 65536U;
  budget.maximum_singleton_root_query_count = 65536U;
  return budget;
}

[[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalBridgeBudget
vertical_budget(const ExactDirectSparseUnifiedLevelPlanResult& plan) {
  std::size_t higher_batch_count = 0U;
  for (const auto& batch : plan.batches) {
    higher_batch_count += batch.order >= 3U ? 1U : 0U;
  }
  ExactDirectMorseResidentAllOrdersVerticalBridgeBudget budget{};
  budget.maximum_committed_higher_batch_count = higher_batch_count;
  budget.maximum_committed_higher_group_count = 4096U;
  budget.maximum_prepared_higher_group_count = 4096U;
  std::size_t higher_saddle_count = 0U;
  std::size_t maximum_batch_higher_saddle_count = 0U;
  for (const auto& batch : plan.batches) {
    if (batch.order < 3U) {
      continue;
    }
    std::size_t batch_saddle_count = 0U;
    for (std::size_t local = 0U;
         local < batch.direct_reference_count;
         ++local) {
      if (plan.direct_references[batch.direct_reference_offset + local].role ==
          ExactDirectMorseH0Role::saddle) {
        ++batch_saddle_count;
        ++higher_saddle_count;
      }
    }
    maximum_batch_higher_saddle_count =
        std::max(maximum_batch_higher_saddle_count, batch_saddle_count);
  }
  budget.maximum_committed_higher_direct_saddle_group_binding_count =
      higher_saddle_count;
  budget.maximum_prepared_higher_direct_saddle_group_binding_count =
      maximum_batch_higher_saddle_count;
  budget.maximum_persistent_source_root_witness_count = 4096U;
  budget.maximum_prior_root_witness_probe_count = 65536U;
  budget.maximum_final_root_witness_probe_count = 4096U;
  budget.maximum_source_facet_resolution_scan_count = 1048576U;
  budget.maximum_projected_target_facet_probe_count = 1048576U;
  budget.maximum_sparse_target_closure_count = 1048576U;
  budget.maximum_expected_group_child_root_reference_count = 65536U;
  budget.maximum_expected_group_coverage_delta_point_reference_count =
      65536U;
  budget.maximum_query_replay_token =
      std::numeric_limits<std::uint64_t>::max();
  budget.target_probe = {4097U, 1024U};
  budget.target_closure = {
      4096U,
      4096U,
      4096U,
      8193U,
      {ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U},
       morsehgp3d::spatial::ExactLbvhTopKBudget{
           1048576U,
           1048576U,
           1048576U,
           1048576U,
           1024U,
           10U,
           10U},
       ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U}},
  };
  budget.target_closure_traversal_order = LbvhTraversalOrder::near_first;
  return budget;
}

[[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalBridge
sealed_vertical_bridge(Fixture& source, std::uint64_t authority_id) {
  auto resident_initialization = resident(source, authority_id);
  check(
      resident_initialization.certified_initialized_session() &&
          resident_initialization.session.has_value(),
      "the normalized v7 resident source initializes");
  if (!resident_initialization.session.has_value()) {
    return {};
  }
  const auto compatibility_plan = resident_initialization.session->plan();
  auto bridge_k1 = build_exact_direct_k1_boruvka_closed_cut_session(
      source.index,
      source.cloud,
      source.boruvka,
      k1_budget(source.cloud.size()));
  check(
      bridge_k1.certified_ready_session(),
      "the campaign K1 closed-cut source initializes");
  if (!bridge_k1.certified_ready_session()) {
    return {};
  }
  auto k2 = initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      std::move(*resident_initialization.session),
      std::move(bridge_k1.session),
      k2_bridge_budget(compatibility_plan));
  check(
      k2.certified_ready_bridge(),
      "the campaign K2/K1 seam initializes");
  if (!k2.certified_ready_bridge()) {
    return {};
  }
  auto vertical =
      initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
          std::move(k2.bridge), vertical_budget(compatibility_plan));
  check(
      vertical.certified_ready_bridge(),
      "the campaign all-orders bridge initializes");
  if (!vertical.certified_ready_bridge()) {
    return {};
  }
  while (!vertical.bridge.resident_complete()) {
    auto prepared = vertical.bridge.prepare_next();
    check(
        prepared.certified_prepared_batch() && prepared.ticket.has_value(),
        "each campaign resident batch prepares");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto committed =
        vertical.bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "each campaign resident batch commits");
    if (!committed.certified_committed_batch()) {
      break;
    }
  }
  const auto sealed = vertical.bridge.seal();
  check(
      sealed.certified_final_vertical_seal() &&
          vertical.bridge.final_vertical_sealed(),
      "the campaign fixture seals the live normalized all-orders bridge");
  return std::move(vertical.bridge);
}

// ---------------------------------------------------------------------------
// Campaign budgets.
// ---------------------------------------------------------------------------

[[nodiscard]] ExactDirectNormalizedVerticalSquareBudget
generous_square_budget() {
  ExactDirectNormalizedVerticalSquareBudget budget{};
  budget.maximum_source_label_scan_count = 1048576U;
  budget.maximum_source_tree_edge_scan_count = 1048576U;
  budget.maximum_target_binding_scan_count = 1048576U;
  budget.maximum_key_point_scan_count = 16777216U;
  budget.maximum_key_lookup_comparison_count = 16777216U;
  budget.maximum_source_subset_lookup_count = 1048576U;
  budget.maximum_source_subset_comparison_count = 16777216U;
  budget.maximum_tree_scratch_count = 1048576U;
  budget.maximum_exact_level_comparison_count = 1048576U;
  budget.maximum_single_exact_level_integer_bit_count = 4096U;
  budget.maximum_logical_output_entry_count = 1048576U;
  return budget;
}

[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignBudget
generous_campaign_budget() {
  ExactDirectMorseResidentNaturalitySquareCampaignBudget budget{};
  budget.maximum_adjacent_order_pair_count = 64U;
  budget.maximum_consecutive_closed_cut_pair_count = 4096U;
  budget.maximum_replayed_square_count = 4096U;
  budget.maximum_retained_square_attestation_count = 4096U;
  budget.maximum_component_scan_count = 65536U;
  budget.maximum_component_label_count = 4096U;
  budget.maximum_component_tree_edge_count = 4096U;
  budget.maximum_target_binding_count = 4096U;
  budget.maximum_source_batch_scan_count = 65536U;
  budget.maximum_group_record_scan_count = 65536U;
  budget.maximum_component_state_scan_count = 65536U;
  budget.maximum_root_coverage_point_scan_count = 65536U;
  budget.maximum_root_witness_scan_count = 65536U;
  budget.maximum_source_plan_birth_reference_scan_count = 1048576U;
  budget.maximum_source_plan_coface_scan_count = 1048576U;
  budget.maximum_coface_reconstruction_count = 1048576U;
  budget.maximum_key_point_scan_count = 16777216U;
  budget.maximum_key_lookup_comparison_count = 16777216U;
  budget.maximum_exact_level_comparison_count = 1048576U;
  budget.maximum_single_exact_level_integer_bit_count = 4096U;
  budget.maximum_transient_scratch_entry_count = 1048576U;
  budget.maximum_logical_output_entry_count = 1048576U;
  budget.per_square_budget = generous_square_budget();
  return budget;
}

struct CampaignSurfaces {
  Fixture* source{};
  const ExactDirectMorseResidentAllOrdersVerticalBridge* bridge{};
  ExactDirectMorseResidentAllOrdersVerticalFinalSeal seal{};
  ExactDirectSparseGatewayCandidateScientificIdentityResult gateway_identity{};
  ExactDirectMorseResidentNaturalitySquareCampaignBudget budget{};
};

[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignResult
build_campaign(
    const CampaignSurfaces& surfaces,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget& budget) {
  return build_exact_direct_morse_resident_naturality_square_campaign(
      *surfaces.bridge,
      surfaces.seal,
      surfaces.source->plan,
      surfaces.source->gateway,
      surfaces.gateway_identity,
      surfaces.source->incidence_authority,
      budget);
}

[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignVerification
verify_campaign(
    const CampaignSurfaces& surfaces,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget& budget,
    const ExactDirectMorseResidentNaturalitySquareCampaignResult& observed) {
  return verify_exact_direct_morse_resident_naturality_square_campaign(
      *surfaces.bridge,
      surfaces.seal,
      surfaces.source->plan,
      surfaces.source->gateway,
      surfaces.gateway_identity,
      surfaces.source->incidence_authority,
      budget,
      observed);
}

void report_campaign_diagnostics(
    const ExactDirectMorseResidentNaturalitySquareCampaignResult& result) {
  std::cerr << "campaign decision=" << static_cast<int>(result.decision)
            << ", scope=" << static_cast<int>(result.scope)
            << ", attestations=" << result.square_attestations.size()
            << ", order pairs=" << result.counters.adjacent_order_pair_count
            << ", cut pairs="
            << result.counters.consecutive_closed_cut_pair_count
            << ", squares=" << result.counters.replayed_square_count
            << ", singleton skips="
            << result.counters.singleton_component_skip_count
            << ", component scans=" << result.counters.component_scan_count
            << ", terminal squares="
            << result.counters.terminal_component_square_count;
  if (result.rejected_adjacent_order_pair_index.has_value()) {
    std::cerr << ", rejected order pair="
              << *result.rejected_adjacent_order_pair_index;
  }
  if (result.rejected_consecutive_closed_cut_pair_index.has_value()) {
    std::cerr << ", rejected cut pair="
              << *result.rejected_consecutive_closed_cut_pair_index;
  }
  if (result.rejected_source_component_handle.has_value()) {
    std::cerr << ", rejected component handle="
              << *result.rejected_source_component_handle;
  }
  if (result.rejected_square_decision.has_value()) {
    std::cerr << ", rejected square decision="
              << static_cast<int>(*result.rejected_square_decision);
  }
  std::cerr << '\n';
}

// Test 1: the positive end-to-end campaign over the sealed normalized bridge.
[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignResult
test_positive_campaign(const CampaignSurfaces& surfaces) {
  auto positive = build_campaign(surfaces, surfaces.budget);
  if (!positive.certified_conditional_all_naturality_squares_replayed() ||
      std::getenv("MORSEHGP3D_CAMPAIGN_REPORT") != nullptr) {
    report_campaign_diagnostics(positive);
  }
  check(
      positive.certified_conditional_all_naturality_squares_replayed() &&
          positive.certified_outcome() && !positive.certified_atomic_failure(),
      "the campaign replays every naturality square over the sealed bridge");
  check(
      positive.decision ==
              ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                  complete_conditional_all_naturality_squares_replayed &&
          positive.scope ==
              ExactDirectMorseResidentNaturalitySquareCampaignScope::
                  conditional_on_sealed_bridge_and_normalized_source &&
          positive.conditional_on_sealed_bridge_and_normalized_source &&
          positive.all_naturality_squares_replayed,
      "the positive fact is claimed only under the conditional scope");
  check(
      !positive.public_status_claimed && !positive.vertical_maps_complete &&
          !positive.external_target_authority_replayed &&
          !positive.global_morse_obligation_replayed &&
          !positive.full_pi0_membership_claimed && !positive.m1_replayed &&
          !positive.pair_matrix_materialized &&
          !positive.global_facet_coface_gamma_or_higher_delaunay_materialized,
      "the campaign never promotes unsupported global or public claims");
  check(
      positive.sealed_bridge_live_verified &&
          positive.final_seal_cross_verified_against_live_bridge &&
          positive.normalized_v7_source_capability_live_required &&
          positive.normalized_source_surface_freshly_verified &&
          positive.canonical_point_namespace_identity_certified &&
          positive.exact_product_order_adjacent_schedule_reconstructed &&
          positive
              .every_component_membership_derived_incrementally_and_released &&
          positive
              .every_spanning_tree_derived_from_certified_source_adjacencies &&
          positive
              .every_target_carrier_binding_derived_from_sealed_bridge_images &&
          positive.every_horizontal_inclusion_bound_to_consecutive_closed_cuts &&
          positive.every_square_receipt_freshly_built &&
          positive.every_square_receipt_freshly_verified &&
          positive.every_singleton_component_skip_justified &&
          !positive.no_partial_scientific_payload_published_on_failure,
      "every conditional campaign fact is asserted on the success path");

  const auto& counters = positive.counters;
  check(
      positive.square_attestations.size() == counters.replayed_square_count &&
          counters.retained_square_attestation_count ==
              counters.replayed_square_count &&
          counters.logical_output_entry_count ==
              counters.replayed_square_count &&
          counters.replayed_square_count +
                  counters.singleton_component_skip_count ==
              counters.component_scan_count,
      "every scanned component per consecutive cut pair is one replayed square or one justified singleton skip");
  check(
      counters.terminal_component_square_count ==
              surfaces.seal.sealed_terminal_component_image_count &&
          positive.terminal_component_square_replayed ==
              (surfaces.seal.sealed_terminal_component_image_count != 0U),
      "the terminal K=n singleton square exists exactly when the seal committed one terminal image");
  // DECISION 1 (maintainer): membership derivation moved to the certified
  // candidate-complete plan scope, so resident component states and root
  // coverages are legitimately never scanned while root witnesses are.
  check(
      counters.component_state_scan_count == 0U &&
          counters.root_coverage_point_scan_count == 0U &&
          counters.root_witness_scan_count ==
              surfaces.seal.sealed_persistent_source_root_witness_count,
      "membership is plan-relative: state and coverage scans stay zero while every sealed root witness is re-certified");
  check(
      counters.adjacent_order_pair_count >= 3U &&
          counters.consecutive_closed_cut_pair_count != 0U &&
          !positive.square_attestations.empty(),
      "the n=6, K=4 fixture replays nontrivial squares over at least the 2->1, 3->2 and 4->3 order pairs");
  check(
      positive.point_count == surfaces.source->plan.point_count &&
          positive.effective_maximum_order ==
              surfaces.source->incidence_authority.effective_maximum_order &&
          positive.sealed_bridge_stamp == surfaces.seal.sealed_stamp &&
          positive.normalized_source_authority_id ==
              surfaces.seal.sealed_stamp.resident_session_authority_id &&
          positive.target_carrier_authority_id ==
              surfaces.seal.terminal_locator_stamp.external_authority_id &&
          positive.target_carrier_authority_id ==
              positive.normalized_source_authority_id &&
          positive.canonical_cloud_digest ==
              surfaces.seal.sealed_stamp.canonical_cloud_digest,
      "the campaign result binds the sealed authority ids and canonical digests");

  bool attestations_dense_and_certified = true;
  for (std::size_t index = 0U; index < positive.square_attestations.size();
       ++index) {
    const auto& attestation = positive.square_attestations[index];
    attestations_dense_and_certified =
        attestations_dense_and_certified &&
        attestation.square_attestation_index == index &&
        attestation.certified_conditional_square_attestation() &&
        attestation.source_order >= 2U &&
        attestation.source_order <= positive.effective_maximum_order &&
        attestation.target_order + 1U == attestation.source_order &&
        attestation.square_decision ==
            ExactDirectNormalizedVerticalSquareDecision::
                complete_conditional_normalized_common_facet_vertical_square;
  }
  check(
      attestations_dense_and_certified,
      "every retained attestation is dense, certified and bound to one adjacent order pair");
  return positive;
}

// Test 2: the fresh verifier accepts the faithful result and rejects forgeries.
void test_fresh_verification(
    const CampaignSurfaces& surfaces,
    ExactDirectMorseResidentNaturalitySquareCampaignResult& positive) {
  const auto accepted = verify_campaign(surfaces, surfaces.budget, positive);
  check(
      accepted.result_certified && accepted.sealed_bridge_accepted &&
          accepted.trusted_normalized_source_surface_accepted &&
          accepted.observed_storage_within_budget &&
          accepted.expected_result_freshly_reconstructed &&
          accepted.observed_recursively_equal &&
          accepted.unsupported_global_claims_remain_false,
      "the fresh campaign verifier rebuilds and accepts the faithful positive result");

  if (!positive.square_attestations.empty()) {
    ++positive.square_attestations.front().to_closed_target_root_id;
    const auto tampered = verify_campaign(surfaces, surfaces.budget, positive);
    check(
        !tampered.result_certified && !tampered.observed_recursively_equal,
        "flipping one attestation target root id is rejected by recursive reconstruction");
    --positive.square_attestations.front().to_closed_target_root_id;
  }
}

// Test 3: fail-closed preconditions.
void test_precondition_rejections(CampaignSurfaces& surfaces) {
  const ExactDirectMorseResidentAllOrdersVerticalBridge unsealed_bridge{};
  const auto unsealed =
      build_exact_direct_morse_resident_naturality_square_campaign(
          unsealed_bridge,
          surfaces.seal,
          surfaces.source->plan,
          surfaces.source->gateway,
          surfaces.gateway_identity,
          surfaces.source->incidence_authority,
          surfaces.budget);
  check(
      unsealed.decision ==
              ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                  no_campaign_bridge_not_sealed &&
          unsealed.certified_atomic_failure() &&
          !unsealed.certified_conditional_all_naturality_squares_replayed(),
      "an unsealed bridge rejects atomically before any replay");
  // A faithfully transported atomic failure is itself verifiable against the
  // same surfaces without promoting the positive fact.
  const auto transported_failure =
      verify_exact_direct_morse_resident_naturality_square_campaign(
          unsealed_bridge,
          surfaces.seal,
          surfaces.source->plan,
          surfaces.source->gateway,
          surfaces.gateway_identity,
          surfaces.source->incidence_authority,
          surfaces.budget,
          unsealed);
  check(
      transported_failure.result_certified &&
          transported_failure.observed_recursively_equal &&
          !transported_failure.sealed_bridge_accepted,
      "a faithfully transported atomic failure verifies without a sealed bridge");

  auto mutated_seal = surfaces.seal;
  ++mutated_seal.sealed_k2_group_count;
  const auto mismatched =
      build_exact_direct_morse_resident_naturality_square_campaign(
          *surfaces.bridge,
          mutated_seal,
          surfaces.source->plan,
          surfaces.source->gateway,
          surfaces.gateway_identity,
          surfaces.source->incidence_authority,
          surfaces.budget);
  check(
      mismatched.decision ==
              ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                  no_campaign_final_seal_mismatch &&
          mismatched.certified_atomic_failure(),
      "a mutated trusted seal copy cannot cross-verify against the live bridge");

  // Mutating one byte of the plan's pair-cloud digest preserves the plan's
  // own structural certificate, so the campaign must fall through to the
  // canonical point-namespace identity comparison and reject there.
  auto digest_bytes =
      surfaces.source->plan.source_pair_canonical_cloud_digest.bytes();
  digest_bytes[0] ^= 0x01U;
  const auto original_digest =
      surfaces.source->plan.source_pair_canonical_cloud_digest;
  surfaces.source->plan.source_pair_canonical_cloud_digest =
      morsehgp3d::contract::CanonicalId{digest_bytes};
  const auto identity_rejected = build_campaign(surfaces, surfaces.budget);
  check(
      identity_rejected.decision ==
              ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                  no_campaign_point_namespace_identity_rejected &&
          identity_rejected.certified_atomic_failure(),
      "a mutated source-plan digest byte breaks the canonical point namespace identity");
  surfaces.source->plan.source_pair_canonical_cloud_digest = original_digest;
}

// Test 4: cap-minus-one budgets derived from the positive run purge
// atomically with no_campaign_budget_exhausted.
void test_budget_cap_minus_one(
    const CampaignSurfaces& surfaces,
    const ExactDirectMorseResidentNaturalitySquareCampaignCounters& counters) {
  std::size_t exercised_axis_count = 0U;
  const auto reject_cap = [&](auto mutate, const std::string& message) {
    auto insufficient = surfaces.budget;
    mutate(insufficient);
    const auto rejected = build_campaign(surfaces, insufficient);
    if (rejected.decision !=
        ExactDirectMorseResidentNaturalitySquareCampaignDecision::
            no_campaign_budget_exhausted) {
      report_campaign_diagnostics(rejected);
    }
    check(
        rejected.decision ==
                ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                    no_campaign_budget_exhausted &&
            rejected.certified_atomic_failure() &&
            rejected.square_attestations.empty() &&
            rejected.counters ==
                ExactDirectMorseResidentNaturalitySquareCampaignCounters{},
        message);
    ++exercised_axis_count;
  };

  if (counters.source_batch_scan_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_source_batch_scan_count =
              counters.source_batch_scan_count - 1U;
        },
        "source-batch scan cap minus one purges the campaign atomically");
  }
  if (counters.group_record_scan_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_group_record_scan_count =
              counters.group_record_scan_count - 1U;
        },
        "group-record scan cap minus one purges the campaign atomically");
  }
  if (counters.coface_reconstruction_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_coface_reconstruction_count =
              counters.coface_reconstruction_count - 1U;
        },
        "coface reconstruction cap minus one purges the campaign atomically");
  }
  if (counters.key_lookup_comparison_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_key_lookup_comparison_count =
              counters.key_lookup_comparison_count - 1U;
        },
        "key lookup comparison cap minus one purges the campaign atomically");
  }
  if (counters.replayed_square_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_replayed_square_count =
              counters.replayed_square_count - 1U;
        },
        "replayed square cap minus one purges the campaign atomically");
  }
  if (counters.root_witness_scan_count != 0U) {
    reject_cap(
        [&](auto& budget) {
          budget.maximum_root_witness_scan_count =
              counters.root_witness_scan_count - 1U;
        },
        "root witness scan cap minus one purges the campaign atomically");
  }
  check(
      exercised_axis_count >= 4U,
      "at least four campaign budget axes are exercised at cap minus one");
}

// Test 2b: a forged positive claim on a purged failure result is rejected.
void test_forged_failure_claim(
    const CampaignSurfaces& surfaces,
    const ExactDirectMorseResidentNaturalitySquareCampaignCounters& counters) {
  if (counters.replayed_square_count == 0U) {
    return;
  }
  auto insufficient = surfaces.budget;
  insufficient.maximum_replayed_square_count =
      counters.replayed_square_count - 1U;
  auto exhausted = build_campaign(surfaces, insufficient);
  check(
      exhausted.decision ==
              ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                  no_campaign_budget_exhausted &&
          exhausted.certified_atomic_failure(),
      "the forged-claim scenario starts from one authentic purged failure");
  exhausted.all_naturality_squares_replayed = true;
  const auto forged = verify_campaign(surfaces, insufficient, exhausted);
  check(
      !forged.result_certified && !forged.observed_recursively_equal &&
          !exhausted.certified_outcome(),
      "forcing all_naturality_squares_replayed on a failure result is rejected");
}

// Test 5: the named external target authority binding.
void test_external_target_authority_binding(const CampaignSurfaces& surfaces) {
  const auto binding =
      build_exact_direct_morse_resident_vertical_external_target_authority_binding(
          *surfaces.bridge, surfaces.seal);
  check(
      binding.certified_named_external_target_authority_binding() &&
          binding.decision ==
              ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision::
                  complete_named_external_target_authority_binding,
      "the sealed bridge yields one certified named external target authority binding");
  check(
      binding.external_target_authority_id != 0U &&
          binding.external_target_authority_id ==
              surfaces.seal.terminal_locator_stamp.external_authority_id &&
          binding.external_target_authority_id ==
              binding.resident_session_authority_id &&
          binding.external_target_authority_named_only &&
          !binding.external_target_authority_replayed &&
          !binding.all_naturality_squares_replayed &&
          !binding.public_status_claimed &&
          binding.vertical_journal_config().external_target_authority_id ==
              binding.external_target_authority_id,
      "the binding names the sealed nonzero authority without ever claiming replay");

  const auto accepted =
      verify_exact_direct_morse_resident_vertical_external_target_authority_binding(
          *surfaces.bridge, surfaces.seal, binding);
  check(
      accepted.result_certified && accepted.sealed_bridge_accepted &&
          accepted.expected_binding_freshly_reconstructed &&
          accepted.observed_recursively_equal &&
          accepted.replay_never_claimed,
      "the fresh binding verifier rebuilds and accepts the faithful binding");

  const ExactDirectMorseResidentAllOrdersVerticalBridge unsealed_bridge{};
  const auto unsealed =
      build_exact_direct_morse_resident_vertical_external_target_authority_binding(
          unsealed_bridge, surfaces.seal);
  check(
      unsealed.decision ==
              ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision::
                  no_binding_bridge_not_sealed &&
          !unsealed.certified_named_external_target_authority_binding(),
      "an unsealed bridge rejects the naming binding");

  auto mutated_seal = surfaces.seal;
  ++mutated_seal.sealed_final_root_witness_probe_count;
  const auto mismatched =
      build_exact_direct_morse_resident_vertical_external_target_authority_binding(
          *surfaces.bridge, mutated_seal);
  check(
      mismatched.decision ==
              ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision::
                  no_binding_final_seal_mismatch &&
          !mismatched.certified_named_external_target_authority_binding(),
      "a mutated trusted seal copy rejects the naming binding");

  auto forged = binding;
  forged.external_target_authority_replayed = true;
  const auto forged_verification =
      verify_exact_direct_morse_resident_vertical_external_target_authority_binding(
          *surfaces.bridge, surfaces.seal, forged);
  check(
      !forged_verification.result_certified &&
          !forged_verification.replay_never_claimed,
      "a forged replay claim on the binding is rejected by the fresh verifier");

  auto altered = binding;
  ++altered.external_target_authority_id;
  const auto altered_verification =
      verify_exact_direct_morse_resident_vertical_external_target_authority_binding(
          *surfaces.bridge, surfaces.seal, altered);
  check(
      !altered_verification.result_certified &&
          !altered_verification.observed_recursively_equal,
      "an altered authority id on the binding is rejected by recursive reconstruction");
}

}  // namespace

int main() {
  auto source = campaign_fixture();
  std::size_t k2_batches = 0U;
  std::size_t higher_batches = 0U;
  for (const auto& batch : source.plan.batches) {
    k2_batches += batch.order == 2U ? 1U : 0U;
    higher_batches += batch.order >= 3U ? 1U : 0U;
  }
  check(
      source.plan.certified_complete_candidate_source_plan() &&
          source.incidence_authority
              .certified_horizontal_incidence_reduction() &&
          k2_batches != 0U && higher_batches != 0U,
      "the n=6, K=4 fixture supplies a certified normalized source surface with K2 and higher batches");
  if (!source.plan.certified_complete_candidate_source_plan()) {
    std::cerr << "plan decision=" << static_cast<int>(source.plan.decision)
              << ", batches=" << source.plan.batches.size()
              << ", k2=" << k2_batches << ", higher=" << higher_batches
              << '\n';
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }

  auto bridge = sealed_vertical_bridge(source, 99101U);
  check(
      bridge.final_vertical_sealed() &&
          bridge.final_vertical_seal() != nullptr &&
          bridge.normalized_incidence_complete_v7_source_capability(),
      "the campaign fixture owns a sealed bridge with the live normalized v7 capability");
  if (!bridge.final_vertical_sealed() ||
      bridge.final_vertical_seal() == nullptr) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }

  auto gateway_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          source.gateway, source.plan_cap.source_gateway_identity_budget);
  check(
      gateway_identity.certified_identity() &&
          gateway_identity.scientific_identity_digest ==
              source.plan.source_gateway_scientific_identity_digest,
      "the recomputed gateway scientific identity matches the certified plan association");

  CampaignSurfaces surfaces{};
  surfaces.source = &source;
  surfaces.bridge = &bridge;
  surfaces.seal = *bridge.final_vertical_seal();
  surfaces.gateway_identity = std::move(gateway_identity);
  surfaces.budget = generous_campaign_budget();

  auto positive = test_positive_campaign(surfaces);
  if (positive.certified_conditional_all_naturality_squares_replayed()) {
    test_fresh_verification(surfaces, positive);
    test_budget_cap_minus_one(surfaces, positive.counters);
    test_forged_failure_claim(surfaces, positive.counters);
  }
  test_precondition_rejections(surfaces);
  test_external_target_authority_binding(surfaces);

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "resident naturality square campaign tests passed\n";
  return 0;
}
