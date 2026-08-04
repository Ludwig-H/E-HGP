#include "morsehgp3d/hierarchy/direct_morse_normalized_h0_product_session.hpp"
#include "morsehgp3d/hierarchy/direct_morse_resident_event_crosswalk_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
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

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseNormalizedH0ProductSession>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseNormalizedH0ProductSession>);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseNormalizedH0ProductPreparedBatch>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Mutator>
void check_checkpoint_tamper_rejected(
    const ExactDirectMorseNormalizedH0ProductSemanticCheckpoint& canonical,
    Mutator mutate,
    const std::string& message) {
  auto tampered = canonical;
  mutate(tampered);
  ExactDirectMorseNormalizedH0ProductCheckpointResult result;
  result.checkpoint.emplace(std::move(tampered));
  result.decision =
      ExactDirectMorseNormalizedH0ProductCheckpointDecision::
          complete_semantic_snapshot_restart_explicitly_unavailable;
  check(!result.certified_nonresumable_semantic_snapshot(), message);
}

template <typename Mutator>
void check_seal_tamper_rejected(
    const ExactDirectMorseNormalizedH0ProductFinalSeal& canonical,
    Mutator mutate,
    const std::string& message) {
  auto tampered = canonical;
  mutate(tampered);
  ExactDirectMorseNormalizedH0ProductSealResult result;
  result.seal =
      std::make_shared<const ExactDirectMorseNormalizedH0ProductFinalSeal>(
          std::move(tampered));
  result.final_seal_published = true;
  result.decision =
      ExactDirectMorseNormalizedH0ProductSealDecision::
          complete_certified_bounded_normalized_product_seal;
  check(!result.certified_bounded_product_seal(), message);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y) {
  return CertifiedPoint3::from_binary64(x, y, 0.0);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

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

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 1048576U;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 1024U;
  budget.maximum_logical_output_entry_count = capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = resident_budget().locator;
  budget.closure_budget = closure_budget();
  budget.quotient_budget = {1024U, 1024U, 1024U, 1024U, 65536U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = 97001U;
  return config;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkBudget link_budget(
    const ExactDirectMorseForestJournalResult& forest) {
  return {
      forest.implicit_order_one_prefix_count + forest.birth_records.size(),
      forest.saddle_records.size(),
      forest.arm_root_bindings.size(),
      forest.atomic_groups.size(),
      forest.batches.size(),
      forest.implicit_order_one_prefix_count + forest.nodes.size(),
      forest.birth_records.size(),
      forest.arm_root_bindings.size(),
      forest.birth_records.size() + forest.arm_root_bindings.size(),
  };
}

[[nodiscard]] ExactDirectAtLeast20StreamViewBudget view_budget() {
  return {
      1024U,
      10240U,
      10240U,
      1024U,
      10240U,
      2048U,
      1024U,
      20480U,
      65536U,
      2048U,
      20480U,
  };
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

[[nodiscard]] ExactDirectK1NormalizedProductStreamBudget k1_stream_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return {
      point_count,
      point_count,
      edge_count,
      2U * edge_count,
      point_count + 1U,
      point_count,
      2U,
      edge_count,
      2U * edge_count,
      point_count / 20U,
      point_count / 20U,
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
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      points);
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

[[nodiscard]] Fixture fixture() {
  const std::array points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  return fixture_from_points(points, 2U);
}

[[nodiscard]] Fixture crosswalk_moment_curve_fixture(
    std::size_t requested_maximum_order) {
  const std::array points{
      point(-2.0, 4.0, -8.0),
      point(-1.0, 1.0, -1.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(3.0, 9.0, 27.0),
  };
  return fixture_from_points(points, requested_maximum_order);
}

[[nodiscard]] Fixture crosswalk_k4_moment_curve_fixture() {
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
sealed_vertical_bridge(Fixture& source) {
  auto resident_initialization = resident(source, 98011U);
  check(
      resident_initialization.certified_initialized_session() &&
          resident_initialization.session.has_value(),
      "the crosswalk resident source initializes");
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
      "the crosswalk K1 closed-cut source initializes");
  if (!bridge_k1.certified_ready_session()) {
    return {};
  }
  auto k2 = initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      std::move(*resident_initialization.session),
      std::move(bridge_k1.session),
      k2_bridge_budget(compatibility_plan));
  check(
      k2.certified_ready_bridge(),
      "the crosswalk K2/K1 seam initializes");
  if (!k2.certified_ready_bridge()) {
    return {};
  }
  auto vertical =
      initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
          std::move(k2.bridge), vertical_budget(compatibility_plan));
  check(
      vertical.certified_ready_bridge(),
      "the crosswalk all-orders bridge initializes");
  if (!vertical.certified_ready_bridge()) {
    return {};
  }
  while (!vertical.bridge.resident_complete()) {
    auto prepared = vertical.bridge.prepare_next();
    check(
        prepared.certified_prepared_batch() && prepared.ticket.has_value(),
        "each crosswalk resident batch prepares");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto committed =
        vertical.bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "each crosswalk resident batch commits");
    if (!committed.certified_committed_batch()) {
      break;
    }
  }
  const auto sealed = vertical.bridge.seal();
  check(
      sealed.certified_final_vertical_seal() &&
          vertical.bridge.final_vertical_sealed(),
      "the crosswalk consumes and seals the live normalized bridge");
  return std::move(vertical.bridge);
}

[[nodiscard]] ExactDirectMorseResidentEventCrosswalkBudget crosswalk_budget(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult& links,
    const ExactDirectMorseResidentAllOrdersVerticalBridge& bridge) {
  std::size_t birth_binding_count = 0U;
  std::size_t saddle_binding_count = 0U;
  for (const auto& batch : bridge.committed_k2_batches()) {
    birth_binding_count += batch.direct_birth_k1_bindings.size();
    saddle_binding_count += batch.direct_saddle_group_bindings.size();
  }
  for (const auto& batch : bridge.committed_higher_batches()) {
    saddle_binding_count += batch.direct_saddle_group_bindings.size();
  }
  return {
      links.links.size(),
      bridge.resident_plan().batches.size(),
      bridge.committed_k2_batches().size(),
      bridge.committed_higher_batches().size(),
      birth_binding_count,
      saddle_binding_count,
      forest.birth_records.size(),
      bridge.resident_plan().direct_references.size(),
      birth_binding_count + saddle_binding_count,
      bridge.resident_plan().source_event_projection_count,
      links.links.size(),
      links.links.size(),
  };
}

[[nodiscard]] ExactDirectMorseNormalizedH0ProductInitialization
product(Fixture& source) {
  auto resident_initialization = resident(source, 98001U);
  check(
      resident_initialization.certified_initialized_session() &&
          resident_initialization.session.has_value(),
      "the incidence-complete normalized v7 resident initializes");
  if (!resident_initialization.session.has_value()) {
    return {};
  }
  const auto compatibility_plan = resident_initialization.session->plan();
  auto bridge_k1 = build_exact_direct_k1_boruvka_closed_cut_session(
      source.index,
      source.cloud,
      source.boruvka,
      k1_budget(source.cloud.size()));
  auto normalized_k1 = initialize_exact_direct_k1_normalized_product_stream(
      source.index,
      source.cloud,
      source.boruvka,
      k1_budget(source.cloud.size()),
      k1_stream_budget(source.cloud.size()));
  check(
      bridge_k1.certified_ready_session() &&
          normalized_k1.certified_initialized_stream(),
      "both K1 authorities recertify the same compact forest");
  if (!bridge_k1.certified_ready_session() ||
      !normalized_k1.certified_initialized_stream()) {
    return {};
  }
  auto k2 = initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      std::move(*resident_initialization.session),
      std::move(bridge_k1.session),
      k2_bridge_budget(compatibility_plan));
  check(k2.certified_ready_bridge(), "the normalized K2/K1 seam initializes");
  if (!k2.certified_ready_bridge()) {
    return {};
  }
  auto vertical =
      initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
          std::move(k2.bridge), vertical_budget(compatibility_plan));
  check(
      vertical.certified_ready_bridge() &&
          vertical.bridge.resident_normalized_direct_source_session() &&
          vertical.bridge
              .resident_normalized_horizontal_incidence_reduction_certified(),
      "the live all-orders bridge retains the normalized v7 capability");
  if (!vertical.certified_ready_bridge()) {
    return {};
  }
  const auto genesis = vertical.bridge.current_stamp();
  check(
      !vertical.bridge.resident_plan().source_star_freshly_verified &&
          !vertical.bridge.resident_plan()
               .direct_star_cofaces_crosschecked_bijectively &&
          vertical.bridge.resident_plan().batches_sorted_by_exact_level_then_order &&
          vertical.bridge.resident_plan().unique_batch_per_exact_level_and_order &&
          vertical.bridge.resident_plan().decision ==
              ExactDirectSparseUnifiedLevelPlanDecision::not_certified &&
          vertical.bridge.resident_plan().scope ==
              ExactDirectSparseUnifiedLevelPlanScope::unspecified,
      "the normalized compatibility cursor remains honest about not being a Star plan");
  check(
      genesis.canonical_cloud_digest == normalized_k1.session.canonical_cloud_digest(),
      "the product sources retain the same canonical cloud identity");
  check(
      vertical.bridge.verify_owned_k1_source_forest_digest(
          normalized_k1.session.source_forest_digest()),
      "the product sources retain the same independently recertified K1 forest");
  check(
      vertical.bridge.resident_plan().source_pair_canonical_cloud_digest !=
              morsehgp3d::contract::CanonicalId{} &&
          vertical.bridge.resident_plan().source_higher_canonical_cloud_digest !=
              morsehgp3d::contract::CanonicalId{} &&
          vertical.bridge.resident_plan().source_pair_semantic_digest !=
              morsehgp3d::contract::CanonicalId{} &&
          vertical.bridge.resident_plan().source_higher_semantic_digest !=
              morsehgp3d::contract::CanonicalId{},
      "the product resident plan retains every nonzero scientific source digest");
  ExactDirectMorseNormalizedH0ProductBudget budget{};
  budget.maximum_product_batch_count =
      compatibility_plan.batches.size() +
      normalized_k1.session.total_batch_count();
  budget.maximum_outstanding_prepared_batch_count = 1U;
  budget.higher_at_least20 = view_budget();
  budget.hartigan = {
      budget.maximum_product_batch_count,
      normalized_exact_hartigan_level_manifest_maximum_decimal_digits,
  };
  return initialize_exact_direct_morse_normalized_h0_product_session(
      std::move(vertical.bridge),
      std::move(normalized_k1.session),
      99001U,
      budget);
}

void test_complete_atomic_product() {
  auto source = fixture();
  auto initialized = product(source);
  check(
      initialized.certified_initialized_session() &&
          initialized.session.ready() &&
          initialized.requirements.maximum_order == 2U &&
          initialized.requirements.product_batch_count != 0U,
      "the unique normalized product coordinator initializes at genesis");
  if (!initialized.session.ready()) {
    std::cerr << "product init decision="
              << static_cast<int>(initialized.decision) << '\n';
    return;
  }
  auto& session = initialized.session;
  const auto genesis_checkpoint = session.make_semantic_checkpoint();
  check(
      genesis_checkpoint.certified_nonresumable_semantic_snapshot() &&
          !genesis_checkpoint.checkpoint->restart_supported &&
          !genesis_checkpoint.checkpoint->vertical_restore_available,
      "the current vertical limitation is reported without a false restart claim");

  bool saw_k1 = false;
  bool saw_resident = false;
  std::size_t committed = 0U;
  std::size_t hartigan_records = 0U;
  std::size_t empty_structural_transitions = 0U;
  std::optional<morsehgp3d::exact::ExactLevel> previous_level;
  std::size_t previous_order = 0U;
  while (!session.complete()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_prepared_batch() && prepared.ticket.has_value(),
        "each merged exact-level product batch prepares atomically");
    if (!prepared.ticket.has_value()) {
      std::cerr << "prepare decision="
                << static_cast<int>(prepared.decision)
                << " hartigan=" << static_cast<int>(prepared.hartigan_decision)
                << " committed=" << committed
                << " next_source=" << static_cast<int>(session.next_source())
                << '\n';
      break;
    }
    const auto source_kind = prepared.ticket->source();
    const auto level = prepared.ticket->squared_level();
    const auto order = prepared.ticket->order();
    check(
        !previous_level.has_value() || *previous_level < level ||
            (*previous_level == level && previous_order < order),
        "the product scheduler is strictly ordered by exact level then order");
    previous_level = level;
    previous_order = order;
    saw_k1 = saw_k1 ||
        source_kind == ExactDirectMorseNormalizedH0ProductNextSource::k1;
    saw_resident = saw_resident ||
        source_kind == ExactDirectMorseNormalizedH0ProductNextSource::resident;
    const bool emits_hartigan = prepared.ticket->emits_hartigan_record();
    const auto* hartigan = prepared.ticket->prepared_hartigan_record();
    if (emits_hartigan) {
      check(
          prepared.emits_hartigan_record && hartigan != nullptr &&
              hartigan->structurally_certified() &&
              hartigan->input.normalized_batch_index == hartigan_records &&
              hartigan->input.order == order &&
              hartigan->input.squared_level == level &&
              hartigan->complete_batch_qr_partition_certified &&
              !hartigan->binary64_serialization_used,
          "every nonempty product lot owns one exact rational Hartigan v4 record");
      ++hartigan_records;
    } else {
      check(
          !prepared.emits_hartigan_record && hartigan == nullptr &&
              source_kind ==
                  ExactDirectMorseNormalizedH0ProductNextSource::resident,
          "an empty resident transition is structural and emits no false Hartigan group");
      ++empty_structural_transitions;
    }
    const auto higher_source_commit_before =
        session.higher_at_least20().source_commit_digest();
    auto result = session.commit(std::move(*prepared.ticket));
    check(
        result.certified_atomic_product_commit() &&
            result.post_stamp.product_batch_cursor == committed + 1U &&
            result.emits_hartigan_record == emits_hartigan &&
            result.hartigan_commit.has_value() == emits_hartigan &&
            result.post_stamp.hartigan_record_count == hartigan_records,
        "the source, Hartigan and downstream view commit as one product step");
    if (source_kind ==
        ExactDirectMorseNormalizedH0ProductNextSource::resident) {
      check(
          result.higher_at_least20_commit.has_value() &&
              result.higher_at_least20_commit->source_commit_digest !=
                  higher_source_commit_before &&
              result.higher_at_least20_commit->source_commit_digest ==
                  session.higher_at_least20().source_commit_digest(),
          "the outer ticket publishes exactly the prepared post-lot cap-20 source digest");
    }
    if (!result.certified_atomic_product_commit()) {
      std::cerr << "commit decision=" << static_cast<int>(result.decision)
                << '\n';
      break;
    }
    ++committed;
  }
  check(
      session.complete() && saw_k1 && saw_resident &&
          committed == session.product_batch_count() &&
          hartigan_records != 0U && empty_structural_transitions != 0U,
      "the merged K1 and normalized resident streams are exhausted exactly once");
  const auto final_snapshot = session.make_semantic_checkpoint();
  check(
      final_snapshot.certified_nonresumable_semantic_snapshot() &&
          final_snapshot.checkpoint->stamp.product_batch_cursor == committed,
      "the terminal in-memory semantic snapshot binds every component cursor");
  if (final_snapshot.checkpoint.has_value()) {
    const auto& canonical = *final_snapshot.checkpoint;
    check_checkpoint_tamper_rejected(
        canonical,
        [](auto& value) { ++value.k1.total_merge_node_count; },
        "checkpoint certification rejects a tampered nested K1 arena count");
    check_checkpoint_tamper_rejected(
        canonical,
        [](auto& value) { ++value.vertical_stamp.next_query_replay_token; },
        "checkpoint certification rejects a tampered vertical replay token");
    check_checkpoint_tamper_rejected(
        canonical,
        [](auto& value) {
          value.higher_at_least20.view_digest =
              morsehgp3d::contract::CanonicalId{};
        },
        "checkpoint certification rejects a tampered cap-20 view digest");
    check_checkpoint_tamper_rejected(
        canonical,
        [](auto& value) {
          value.hartigan.current_rational_chain_digest =
              morsehgp3d::contract::CanonicalId{};
        },
        "checkpoint certification rejects a tampered Hartigan rational chain");
    check_checkpoint_tamper_rejected(
        canonical,
        [](auto& value) { ++value.hartigan.record_count; },
        "checkpoint certification rejects a broken Hartigan product-count link");
  }

  const auto first_seal = session.seal();
  check(
      first_seal.certified_bounded_product_seal() &&
      first_seal.final_seal_published && session.sealed() &&
          first_seal.seal->normalized_incidence_complete_v7_source_capability &&
          first_seal.seal->vertical.vertical_maps_complete &&
          first_seal.seal->hartigan.certified_seal() &&
          first_seal.seal->structural_source_transition_count == committed &&
          first_seal.seal->hartigan_record_count == hartigan_records &&
          first_seal.seal
              ->empty_structural_batches_excluded_from_hartigan_manifest &&
          !first_seal.seal->m1_replayed &&
          !first_seal.seal->public_status_claimed,
      "the final seal closes bounded horizontal, vertical, Hartigan and cap-20 outputs without public promotion");
  if (first_seal.seal != nullptr) {
    const auto& canonical = *first_seal.seal;
    check_seal_tamper_rejected(
        canonical,
        [](auto& value) {
          value.higher_at_least20.source_commit_digest =
              morsehgp3d::contract::CanonicalId{};
        },
        "seal certification rejects a tampered terminal cap-20 source chain");
    check_seal_tamper_rejected(
        canonical,
        [](auto& value) {
          value.hartigan.normalized_batch_final_chain_digest =
              morsehgp3d::contract::CanonicalId{};
        },
        "seal certification rejects a broken Hartigan terminal-chain link");
    check_seal_tamper_rejected(
        canonical,
        [](auto& value) {
          ++value.vertical.terminal_locator_stamp.binding_count;
        },
        "seal certification rejects a tampered nested vertical locator stamp");
    check_seal_tamper_rejected(
        canonical,
        [](auto& value) {
          ++value.hartigan.records_by_order[0U].normalized_group_count;
        },
        "seal certification rejects a tampered Hartigan order summary");
    check_seal_tamper_rejected(
        canonical,
        [](auto& value) { value.public_status_claimed = true; },
        "seal certification preserves public_status=not_claimed");
  }
  const auto repeated = session.seal();
  check(
      repeated.certified_bounded_product_seal() &&
          repeated.existing_final_seal_returned_without_mutation &&
          !repeated.final_seal_published &&
          repeated.seal == first_seal.seal,
      "the terminal product seal is idempotent and returns the same preallocated storage");
}

void test_resident_event_crosswalk_product_gate_precedes_link_replay() {
  auto source = crosswalk_moment_curve_fixture(4U);
  const auto forest = build_exact_direct_morse_forest_journal(
      source.index,
      source.cloud,
      source.direct.facade,
      source.direct.events,
      source.direct.arm,
      source.direct.arms,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first);
  check(
      forest.certified_conditional_h0_candidate() &&
          forest.point_count == 5U &&
          forest.effective_maximum_order == 4U,
      "the product-gate fixture owns a certified n=5, K=4 forest");

  const ExactDirectMorseEventRankTowerLinkBudget invalid_link_budget{};
  const ExactDirectMorseEventRankTowerLinkJournalResult invalid_link{};
  const ExactDirectMorseResidentAllOrdersVerticalBridge absent_bridge{};
  const ExactDirectMorseResidentEventCrosswalkBudget empty_budget{};
  const auto rejected =
      build_exact_direct_morse_resident_event_crosswalk_journal(
          forest,
          invalid_link_budget,
          invalid_link,
          absent_bridge,
          empty_budget);
  const auto verification =
      verify_exact_direct_morse_resident_event_crosswalk_journal(
          forest,
          invalid_link_budget,
          invalid_link,
          absent_bridge,
          empty_budget,
          rejected);
  check(
      rejected.decision ==
              ExactDirectMorseResidentEventCrosswalkDecision::
                  no_crosswalk_product_n_at_least_k_plus_two_rejected &&
          rejected.certified_atomic_failure(),
      "n<K+2 rejects canonically before consulting an invalid link journal");
  check(
      verification.result_certified &&
          verification.expected_journal_freshly_rebuilt &&
          verification.observed_structure_certified &&
          verification.observed_recursively_equal &&
          !verification.source_link_freshly_replayed_relative_to_forest &&
          !verification.live_bridge_final_seal_verified,
      "fresh verification certifies the early product-gate failure envelope while retaining later-source diagnostics");
}

void test_resident_event_crosswalk_triple_join_and_fail_closed_caps() {
  auto source = crosswalk_k4_moment_curve_fixture();
  const auto forest_cap = forest_budget();
  const auto forest_configuration = forest_config();
  const auto forest = build_exact_direct_morse_forest_journal(
      source.index,
      source.cloud,
      source.direct.facade,
      source.direct.events,
      source.direct.arm,
      source.direct.arms,
      forest_cap,
      forest_configuration,
      LbvhTraversalOrder::near_first);
  check(
      forest.certified_conditional_h0_candidate(),
      "the n=6, K=4 crosswalk forest is conditionally certified");
  if (!forest.certified_conditional_h0_candidate()) {
    std::cerr
        << "crosswalk n=6/K4 source diagnostics: terminal="
        << source.direct.facade.terminal_catalog_certified()
        << " events=" << source.direct.events.certified_partial_refinement()
        << " arms=" << source.direct.arms.certified_partial_refinement()
        << " incidences="
        << source.direct.incidences.certified_partial_refinement()
        << " gateway=" << source.gateway.certified_partial_refinement()
        << " plan=" << source.plan.certified_complete_candidate_source_plan()
        << " rank=" << source.rank.certified()
        << " incidence_authority="
        << source.incidence_authority
               .certified_horizontal_incidence_reduction()
        << " forest_decision=" << static_cast<int>(forest.decision) << '\n';
  }
  const auto links_cap = link_budget(forest);
  const auto links =
      build_exact_direct_morse_event_rank_tower_link_journal(
          forest, links_cap);
  check(
      links.certified_conditional_event_rank_tower_links() &&
          !links.links.empty(),
      "the n=6, K=4 forest exposes nonempty exact adjacent-rank links");
  auto bridge = sealed_vertical_bridge(source);
  check(
      bridge.final_vertical_sealed(),
      "the n=6, K=4 crosswalk owns a live sealed vertical capability");
  if (!forest.certified_conditional_h0_candidate() ||
      !links.certified_conditional_event_rank_tower_links() ||
      !bridge.final_vertical_sealed()) {
    return;
  }

  const auto budget = crosswalk_budget(forest, links, bridge);
  const auto journal =
      build_exact_direct_morse_resident_event_crosswalk_journal(
          forest, links_cap, links, bridge, budget);
  const auto verification =
      verify_exact_direct_morse_resident_event_crosswalk_journal(
          forest, links_cap, links, bridge, budget, journal);
  if (!journal.certified_conditional_resident_event_crosswalk()) {
    std::cerr << "crosswalk decision=" << static_cast<int>(journal.decision)
              << " forest_births=" << forest.birth_records.size()
              << " links=" << links.links.size()
              << " plan_direct="
              << bridge.resident_plan().direct_references.size()
              << " k2_batches=" << bridge.committed_k2_batches().size()
              << " records=" << journal.records.size() << '\n';
    const auto stamp = bridge.current_stamp();
    std::cerr
        << "namespace checks: link_point="
        << (links.point_count == forest.point_count)
        << " link_order="
        << (links.effective_maximum_order == forest.effective_maximum_order)
        << " link_projection="
        << (links.source_event_projection_count ==
            forest.source_event_projection_count)
        << " plan_point="
        << (bridge.resident_plan().point_count == forest.point_count)
        << " plan_projection="
        << (bridge.resident_plan().source_event_projection_count ==
            forest.source_event_projection_count)
        << " higher_cloud="
        << (bridge.resident_plan().source_higher_canonical_cloud_digest ==
            forest.source_higher_canonical_cloud_digest)
        << " pair_cloud="
        << (bridge.resident_plan().source_pair_canonical_cloud_digest ==
            forest.source_higher_canonical_cloud_digest)
        << " stamp_cloud="
        << (stamp.canonical_cloud_digest ==
            forest.source_higher_canonical_cloud_digest)
        << " stamp_higher_cloud="
        << (stamp.resident_higher_canonical_cloud_digest ==
            forest.source_higher_canonical_cloud_digest)
        << " pair_stamp_cloud="
        << (bridge.resident_plan().source_pair_canonical_cloud_digest ==
            stamp.canonical_cloud_digest)
        << " cursor="
        << (stamp.resident_batch_cursor ==
            bridge.resident_plan().batches.size())
        << '\n';
  }
  check(
      journal.certified_conditional_resident_event_crosswalk() &&
          verification.result_certified &&
          journal.every_resident_direct_birth_replayed_against_exact_forest_birth &&
          journal.every_resident_direct_birth_has_one_source_link &&
          journal.every_resident_direct_saddle_below_k_has_one_source_link &&
          journal.counters.forest_birth_record_scan_count ==
              forest.birth_records.size() &&
          journal.counters.resident_plan_direct_reference_scan_count ==
              bridge.resident_plan().direct_references.size(),
      "the bounded n=6, K=4 crosswalk certifies all three source/upper/lower joins");
  check(
      journal.counters.rank_two_k1_target_count > 0U &&
          journal.counters.higher_rank_o4_target_count > 0U,
      "the n=6, K=4 crosswalk exercises both typed K1 and higher-rank O4 target branches");

  std::size_t k2_to_k1_target_count = 0U;
  std::size_t k3_to_k2_target_count = 0U;
  std::size_t k4_to_k3_target_count = 0U;
  bool every_target_matches_live_bridge = !journal.records.empty();
  const auto& resident_plan = bridge.resident_plan();
  const auto& committed_k2_batches = bridge.committed_k2_batches();
  const auto& committed_higher_batches = bridge.committed_higher_batches();
  for (const auto& record : journal.records) {
    if (record.k1_birth_target.has_value()) {
      ++k2_to_k1_target_count;
      const auto& target = *record.k1_birth_target;
      if (target.k2_batch_record_index >= committed_k2_batches.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& batch =
          committed_k2_batches[target.k2_batch_record_index];
      if (target.binding_index >= batch.direct_birth_k1_bindings.size() ||
          target.source_batch_index >= resident_plan.batches.size() ||
          target.source_direct_reference_index >=
              resident_plan.direct_references.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& binding =
          batch.direct_birth_k1_bindings[target.binding_index];
      const auto& plan_batch =
          resident_plan.batches[target.source_batch_index];
      const auto& direct = resident_plan.direct_references[
          target.source_direct_reference_index];
      every_target_matches_live_bridge =
          every_target_matches_live_bridge && record.source_order == 2U &&
          record.target_order == 1U && batch.order == record.source_order &&
          batch.squared_level == record.squared_level &&
          batch.source_batch_index == target.source_batch_index &&
          plan_batch.batch_index == target.source_batch_index &&
          plan_batch.order == record.source_order &&
          plan_batch.squared_level == record.squared_level &&
          target.source_direct_reference_index >=
              plan_batch.direct_reference_offset &&
          target.source_direct_reference_index -
                  plan_batch.direct_reference_offset <
              plan_batch.direct_reference_count &&
          binding.binding_index == target.binding_index &&
          binding.source_direct_reference_index ==
              target.source_direct_reference_index &&
          binding.source_role_record_index ==
              target.source_role_record_index &&
          binding.source_event_projection_index ==
              record.source_event_projection_index &&
          binding.source_facet_token_index ==
              target.source_facet_token_index &&
          binding.closed_k1_root_node_id == target.closed_k1_node.value &&
          direct.direct_reference_index ==
              target.source_direct_reference_index &&
          direct.role == ExactDirectMorseH0Role::birth &&
          direct.source_role_record_index ==
              target.source_role_record_index &&
          direct.source_event_projection_index ==
              record.source_event_projection_index &&
          direct.direct_birth_facet_token_index.has_value() &&
          *direct.direct_birth_facet_token_index ==
              target.source_facet_token_index &&
          record.source_bridge_batch_membership_digest ==
              batch.o4_membership_digest &&
          canonical_exact_direct_morse_resident_k2_o4_membership_digest(
              batch) == batch.o4_membership_digest;
    } else if (record.o4_saddle_target.has_value() &&
               record.o4_saddle_target->target_batch_is_k2) {
      ++k3_to_k2_target_count;
      const auto& target = *record.o4_saddle_target;
      if (target.bridge_batch_record_index >= committed_k2_batches.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& batch =
          committed_k2_batches[target.bridge_batch_record_index];
      if (target.binding_index >= batch.direct_saddle_group_bindings.size() ||
          target.group_image_index >= batch.group_images.size() ||
          target.source_batch_index >= resident_plan.batches.size() ||
          target.source_direct_reference_index >=
              resident_plan.direct_references.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& binding =
          batch.direct_saddle_group_bindings[target.binding_index];
      const auto& image = batch.group_images[target.group_image_index];
      const auto& plan_batch =
          resident_plan.batches[target.source_batch_index];
      const auto& direct = resident_plan.direct_references[
          target.source_direct_reference_index];
      every_target_matches_live_bridge =
          every_target_matches_live_bridge && record.source_order == 3U &&
          record.target_order == 2U && batch.order == record.target_order &&
          batch.squared_level == record.squared_level &&
          batch.source_batch_index == target.source_batch_index &&
          plan_batch.batch_index == target.source_batch_index &&
          plan_batch.order == record.target_order &&
          plan_batch.squared_level == record.squared_level &&
          target.source_direct_reference_index >=
              plan_batch.direct_reference_offset &&
          target.source_direct_reference_index -
                  plan_batch.direct_reference_offset <
              plan_batch.direct_reference_count &&
          binding.binding_index == target.binding_index &&
          binding.source_direct_reference_index ==
              target.source_direct_reference_index &&
          binding.source_role_record_index ==
              target.source_role_record_index &&
          binding.source_event_projection_index ==
              record.source_event_projection_index &&
          binding.source_incidence_family_index ==
              target.source_incidence_family_index &&
          binding.source_hyperedge_index == target.source_hyperedge_index &&
          binding.owner_group_index == target.owner_group_index &&
          binding.group_image_index == target.group_image_index &&
          binding.resident_resultant_root_id ==
              target.resident_resultant_root.value &&
          image.owner_group_index == target.owner_group_index &&
          image.resident_resultant_root_id ==
              target.resident_resultant_root.value &&
          direct.direct_reference_index ==
              target.source_direct_reference_index &&
          direct.role == ExactDirectMorseH0Role::saddle &&
          direct.source_role_record_index ==
              target.source_role_record_index &&
          direct.source_event_projection_index ==
              record.source_event_projection_index &&
          direct.source_incidence_family_index.has_value() &&
          *direct.source_incidence_family_index ==
              target.source_incidence_family_index &&
          record.source_bridge_batch_membership_digest ==
              batch.o4_membership_digest &&
          canonical_exact_direct_morse_resident_k2_o4_membership_digest(
              batch) == batch.o4_membership_digest;
    } else if (record.o4_saddle_target.has_value()) {
      ++k4_to_k3_target_count;
      const auto& target = *record.o4_saddle_target;
      if (target.bridge_batch_record_index >=
          committed_higher_batches.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& batch =
          committed_higher_batches[target.bridge_batch_record_index];
      if (target.binding_index >= batch.direct_saddle_group_bindings.size() ||
          target.group_image_index >= batch.group_images.size() ||
          target.source_batch_index >= resident_plan.batches.size() ||
          target.source_direct_reference_index >=
              resident_plan.direct_references.size()) {
        every_target_matches_live_bridge = false;
        continue;
      }
      const auto& binding =
          batch.direct_saddle_group_bindings[target.binding_index];
      const auto& image = batch.group_images[target.group_image_index];
      const auto& plan_batch =
          resident_plan.batches[target.source_batch_index];
      const auto& direct = resident_plan.direct_references[
          target.source_direct_reference_index];
      every_target_matches_live_bridge =
          every_target_matches_live_bridge &&
          record.source_order == 4U && record.target_order == 3U &&
          record.source_order == batch.source_order + 1U &&
          record.target_order == batch.source_order &&
          batch.target_order + 1U == batch.source_order &&
          batch.squared_level == record.squared_level &&
          batch.source_batch_index == target.source_batch_index &&
          plan_batch.batch_index == target.source_batch_index &&
          plan_batch.order == record.target_order &&
          plan_batch.squared_level == record.squared_level &&
          target.source_direct_reference_index >=
              plan_batch.direct_reference_offset &&
          target.source_direct_reference_index -
                  plan_batch.direct_reference_offset <
              plan_batch.direct_reference_count &&
          binding.binding_index == target.binding_index &&
          binding.source_direct_reference_index ==
              target.source_direct_reference_index &&
          binding.source_role_record_index ==
              target.source_role_record_index &&
          binding.source_event_projection_index ==
              record.source_event_projection_index &&
          binding.source_incidence_family_index ==
              target.source_incidence_family_index &&
          binding.source_hyperedge_index == target.source_hyperedge_index &&
          binding.owner_group_index == target.owner_group_index &&
          binding.group_image_index == target.group_image_index &&
          binding.resident_resultant_root_id ==
              target.resident_resultant_root.value &&
          image.owner_group_index == target.owner_group_index &&
          image.resident_resultant_source_root_id ==
              target.resident_resultant_root.value &&
          direct.direct_reference_index ==
              target.source_direct_reference_index &&
          direct.role == ExactDirectMorseH0Role::saddle &&
          direct.source_role_record_index ==
              target.source_role_record_index &&
          direct.source_event_projection_index ==
              record.source_event_projection_index &&
          direct.source_incidence_family_index.has_value() &&
          *direct.source_incidence_family_index ==
              target.source_incidence_family_index &&
          record.source_bridge_batch_membership_digest ==
              batch.o4_membership_digest &&
          canonical_exact_direct_morse_resident_higher_o4_membership_digest(
              batch) == batch.o4_membership_digest;
    } else {
      every_target_matches_live_bridge = false;
    }
  }
  check(
      k2_to_k1_target_count > 0U && k3_to_k2_target_count > 0U &&
          k4_to_k3_target_count > 0U,
      "the n=6, K=4 crosswalk exercises K2-to-K1, K3-to-K2 and higher-saddle K4-to-K3 targets");
  check(
      every_target_matches_live_bridge,
      "an independent field-by-field oracle matches every crosswalk target to the live K1/K2/higher batch, binding, direct reference, group image, root and membership digest");

  bool every_record_owns_the_exact_upper_birth = !journal.records.empty();
  for (const auto& record : journal.records) {
    const auto forest_birth = std::find_if(
        forest.birth_records.begin(),
        forest.birth_records.end(),
        [&record](const auto& birth) {
          return birth.birth_record_index == record.source_birth_record_index;
        });
    if (forest_birth == forest.birth_records.end() ||
        record.source_upper_birth_direct_reference_index >=
            bridge.resident_plan().direct_references.size() ||
        record.source_upper_birth_facet_token_index >=
            bridge.resident_plan().facet_tokens.size()) {
      every_record_owns_the_exact_upper_birth = false;
      continue;
    }
    const auto& direct = bridge.resident_plan().direct_references[
        record.source_upper_birth_direct_reference_index];
    const auto& token = bridge.resident_plan().facet_tokens[
        record.source_upper_birth_facet_token_index];
    every_record_owns_the_exact_upper_birth =
        every_record_owns_the_exact_upper_birth &&
        direct.role == ExactDirectMorseH0Role::birth &&
        direct.source_role_record_index ==
            record.source_upper_birth_role_record_index &&
        direct.source_event_projection_index ==
            record.source_event_projection_index &&
        direct.direct_birth_facet_token_index.has_value() &&
        *direct.direct_birth_facet_token_index ==
            record.source_upper_birth_facet_token_index &&
        token.facet_token_index ==
            record.source_upper_birth_facet_token_index &&
        forest_birth->source_event_projection_index ==
            record.source_event_projection_index &&
        forest_birth->order == record.source_order &&
        forest_birth->facet_key == token.facet_key;
  }
  check(
      every_record_owns_the_exact_upper_birth,
      "every crosswalk record exposes the exact forest birth and resident upper birth provenance");

  std::size_t top_order_saddle_count = 0U;
  for (const auto& batch : bridge.resident_plan().batches) {
    if (batch.order != forest.effective_maximum_order) {
      continue;
    }
    for (std::size_t local = 0U; local < batch.direct_reference_count;
         ++local) {
      const auto& direct = bridge.resident_plan().direct_references[
          batch.direct_reference_offset + local];
      top_order_saddle_count +=
          direct.role == ExactDirectMorseH0Role::saddle ? 1U : 0U;
    }
  }
  check(
      top_order_saddle_count != 0U &&
          journal.certified_conditional_resident_event_crosswalk(),
      "top-order saddles remain out of the below-K source-link obligation");

  auto oversized_observed = journal;
  oversized_observed.records.push_back({});
  const auto oversized_verification =
      verify_exact_direct_morse_resident_event_crosswalk_journal(
          forest,
          links_cap,
          links,
          bridge,
          budget,
          oversized_observed);
  check(
      !oversized_verification.observed_storage_within_budget &&
          !oversized_verification.observed_structure_certified &&
          !oversized_verification.observed_recursively_equal &&
          !oversized_verification.result_certified,
      "an observed arena above the trusted record cap is rejected before structural digest replay or recursive comparison");

  if (!journal.records.empty()) {
    auto giant_exact_level_observed = journal;
    morsehgp3d::exact::BigInt giant_numerator{1};
    giant_numerator <<= 1048576U;
    giant_exact_level_observed.records[0U].squared_level =
        morsehgp3d::exact::ExactLevel{std::move(giant_numerator)};
    const auto giant_exact_level_verification =
        verify_exact_direct_morse_resident_event_crosswalk_journal(
            forest,
            links_cap,
            links,
            bridge,
            budget,
            giant_exact_level_observed);
    check(
        giant_exact_level_verification.observed_storage_within_budget &&
            giant_exact_level_verification.expected_journal_freshly_rebuilt &&
            !giant_exact_level_verification.observed_recursively_equal &&
            !giant_exact_level_verification.observed_structure_certified &&
            !giant_exact_level_verification.result_certified,
        "an in-cap observed record with a giant exact level is rejected by equality before observed structural digest replay");
  }

  auto relaxed_budget = budget;
  ++relaxed_budget.maximum_link_scan_count;
  const auto coherently_rehashed_relaxed_journal =
      build_exact_direct_morse_resident_event_crosswalk_journal(
          forest, links_cap, links, bridge, relaxed_budget);
  const auto relaxed_under_original_authority =
      verify_exact_direct_morse_resident_event_crosswalk_journal(
          forest,
          links_cap,
          links,
          bridge,
          budget,
          coherently_rehashed_relaxed_journal);
  check(
      coherently_rehashed_relaxed_journal
              .certified_conditional_resident_event_crosswalk() &&
          coherently_rehashed_relaxed_journal.crosswalk_digest !=
              journal.crosswalk_digest &&
          relaxed_under_original_authority.observed_storage_within_budget &&
          !relaxed_under_original_authority.observed_recursively_equal &&
          !relaxed_under_original_authority.observed_structure_certified &&
          !relaxed_under_original_authority.result_certified,
      "a coherently rehashed standalone result under a different budget remains non-authoritative and fresh replay under the original budget rejects it");

  check(
      budget.maximum_forest_birth_record_scan_count != 0U &&
          budget.maximum_resident_plan_direct_reference_scan_count != 0U,
      "the n=6, K=4 fixture exercises both newly enforced scan caps");
  if (budget.maximum_forest_birth_record_scan_count != 0U) {
    auto too_small = budget;
    --too_small.maximum_forest_birth_record_scan_count;
    const auto rejected =
        build_exact_direct_morse_resident_event_crosswalk_journal(
            forest, links_cap, links, bridge, too_small);
    check(
        rejected.decision ==
                ExactDirectMorseResidentEventCrosswalkDecision::
                    no_crosswalk_budget_exhausted &&
            rejected.certified_atomic_failure(),
        "forest birth scan cap minus one fails atomically before publication");
    const auto rejected_verification =
        verify_exact_direct_morse_resident_event_crosswalk_journal(
            forest,
            links_cap,
            links,
            bridge,
            too_small,
            rejected);
    check(
        rejected_verification.result_certified &&
            rejected_verification.expected_journal_freshly_rebuilt &&
            rejected_verification.observed_structure_certified &&
            rejected_verification.observed_recursively_equal,
        "fresh verification certifies the reconstructed atomic budget failure");

    auto leaked_flag = rejected;
    leaked_flag.every_resident_direct_birth_has_one_source_link = true;
    check(
        !leaked_flag.certified_atomic_failure(),
        "atomic failure certification rejects a leaked success flag");
    auto leaked_stamp = rejected;
    leaked_stamp.source_bridge_stamp.bridge_session_instance_id = 1U;
    check(
        !leaked_stamp.certified_atomic_failure(),
        "atomic failure certification rejects a leaked bridge stamp");
    auto leaked_global_claim = rejected;
    leaked_global_claim.gamma_cells_or_global_cofaces_materialized = true;
    check(
        !leaked_global_claim.certified_atomic_failure(),
        "atomic failure certification rejects a leaked global structure claim");
    auto mutated_schema = rejected;
    ++mutated_schema.schema_version;
    check(
        !mutated_schema.certified_atomic_failure(),
        "atomic failure certification rejects a mutated crosswalk schema version");
    auto mutated_scope = rejected;
    mutated_scope.scope =
        ExactDirectMorseResidentEventCrosswalkScope::unspecified;
    check(
        !mutated_scope.certified_atomic_failure(),
        "atomic failure certification rejects a mutated crosswalk scope");
    auto mutated_failure_decision = rejected;
    mutated_failure_decision.decision =
        ExactDirectMorseResidentEventCrosswalkDecision::
            no_crosswalk_source_namespace_mismatch;
    check(
        !mutated_failure_decision.certified_atomic_failure() &&
            !verify_exact_direct_morse_resident_event_crosswalk_journal(
                 forest,
                 links_cap,
                 links,
                 bridge,
                 too_small,
                 mutated_failure_decision)
                 .result_certified,
        "the operational failure digest and fresh replay reject a substituted failure decision");
    auto substituted_failure_digest = rejected;
    substituted_failure_digest.crosswalk_digest = journal.crosswalk_digest;
    check(
        !substituted_failure_digest.certified_atomic_failure() &&
            !verify_exact_direct_morse_resident_event_crosswalk_journal(
                 forest,
                 links_cap,
                 links,
                 bridge,
                 too_small,
                 substituted_failure_digest)
                 .result_certified,
        "fresh verification rejects a substituted failure-envelope digest");
    auto mutated_failure_budget = rejected;
    ++mutated_failure_budget.requested_budget.maximum_link_scan_count;
    check(
        !mutated_failure_budget.certified_atomic_failure(),
        "the operational failure digest rejects a mutated requested budget");
    auto missing_failure_digest = rejected;
    missing_failure_digest.crosswalk_digest = {};
    check(
        !missing_failure_digest.certified_atomic_failure(),
        "an atomic failure without its operational digest remains uncertified");
  }
  if (budget.maximum_resident_plan_direct_reference_scan_count != 0U) {
    auto too_small = budget;
    --too_small.maximum_resident_plan_direct_reference_scan_count;
    const auto rejected =
        build_exact_direct_morse_resident_event_crosswalk_journal(
            forest, links_cap, links, bridge, too_small);
    check(
        rejected.decision ==
                ExactDirectMorseResidentEventCrosswalkDecision::
                    no_crosswalk_budget_exhausted &&
            rejected.certified_atomic_failure(),
        "resident direct-reference scan cap minus one fails atomically before publication");
  }

  const auto check_certified_crosswalk_mutation_rejected =
      [&](auto mutate, const std::string& message) {
        auto mutation = journal;
        mutate(mutation);
        check(
            !mutation.certified_conditional_resident_event_crosswalk() &&
                !verify_exact_direct_morse_resident_event_crosswalk_journal(
                     forest,
                     links_cap,
                     links,
                     bridge,
                     budget,
                     mutation)
                     .result_certified,
            message);
      };
  if (!journal.records.empty()) {
    check_certified_crosswalk_mutation_rejected(
        [](auto& value) {
          ++value.records[0U].source_birth_record_index;
        },
        "crosswalk certification and fresh replay reject a record mutation without a matching digest");
  }
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        value.every_resident_direct_birth_has_one_source_link = false;
      },
      "crosswalk certification and fresh replay reject a missing coverage fact");
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        value.no_partial_scientific_payload_published = false;
      },
      "complete crosswalk certification rejects a partial-publication flag");
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        ++value.point_count;
      },
      "the result digest rejects a structurally plausible point-count mutation");
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        ++value.requested_budget.maximum_link_scan_count;
      },
      "the result digest rejects a relaxed trusted-budget mutation");
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        ++value.source_forest_final_locator_stamp.external_authority_id;
      },
      "the result digest rejects a forest locator-stamp mutation");
  check_certified_crosswalk_mutation_rejected(
      [](auto& value) {
        ++value.source_bridge_stamp.resident_epoch;
      },
      "the result digest rejects a live bridge-stamp mutation");
  check(
      journal.counters.resident_plan_batch_scan_count != 0U,
      "the n=6, K=4 crosswalk exposes a nonzero logical plan-batch cardinality");
  if (journal.counters.resident_plan_batch_scan_count != 0U) {
    check_certified_crosswalk_mutation_rejected(
        [](auto& value) {
          --value.counters.resident_plan_batch_scan_count;
        },
        "the result digest rejects an understated logical source cardinality");
  }
}

}  // namespace

int main() {
  test_complete_atomic_product();
  test_resident_event_crosswalk_product_gate_precedes_link_replay();
  test_resident_event_crosswalk_triple_join_and_fail_closed_caps();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "normalized H0 product session tests passed\n";
  return 0;
}
