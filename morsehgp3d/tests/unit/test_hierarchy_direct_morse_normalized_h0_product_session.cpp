#include "morsehgp3d/hierarchy/direct_morse_normalized_h0_product_session.hpp"

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

[[nodiscard]] Fixture fixture() {
  const std::array points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  auto index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal, pair, higher);
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

}  // namespace

int main() {
  test_complete_atomic_product();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "normalized H0 product session tests passed\n";
  return 0;
}
