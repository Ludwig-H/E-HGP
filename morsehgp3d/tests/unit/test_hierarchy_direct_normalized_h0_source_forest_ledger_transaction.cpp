#include "morsehgp3d/hierarchy/direct_normalized_h0_source_forest_ledger_transaction.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0SourceForestLedgerPreparedWindow> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0SourceForestLedgerPreparedWindow>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0SourceForestLedgerTransactionResult> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0SourceForestLedgerTransactionResult>);
static_assert(
    ExactDirectNormalizedH0SourceForestLedgerTransactionResult::mode ==
    std::string_view{"exact_source_forest_ledger_transaction_v1"});

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y) {
  return CertifiedPoint3::from_binary64(x, y, 0.0);
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum};
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
  return {1024U, 65536U, 65536U, 1048576U, 65536U,
          1048576U, 16777216U, 65536U, 65536U};
}

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget
generous_gateway_budget() {
  return {4096U,
          65536U,
          65536U,
          655360U,
          1048576U,
          1048576U,
          1048576U,
          8388608U,
          generous_first_incidence_budget()};
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
generous_gateway_identity_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget
generous_source_plan_budget() {
  return {4096U,
          65536U,
          1048576U,
          1048576U,
          1048576U,
          1048576U,
          1048576U,
          1048576U,
          1048576U,
          8388608U,
          generous_gateway_identity_budget()};
}

[[nodiscard]] ExactDirectNormalizedH0ResidentAdapterBudget
generous_adapter_budget() {
  return {1048576U, 1048576U, 1048576U, 8388608U,
          8388608U, 67108864U, 67108864U, 1048576U,
          8388608U, 8388608U, 1048576U, 268435456U};
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget
unlimited_frozen_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectMorseUnifiedResidentSessionBudget
generous_resident_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget;
  budget.locator = {1024U, 1024U, 10240U, 1024U, 1024U, 1024U,
                    1024U, 1024U, 10240U, 4097U, 4097U};
  budget.probe = {4097U, 1024U};
  budget.frozen_batch = unlimited_frozen_budget();
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
  budget.maximum_group_child_reference_count = 1024U;
  budget.maximum_group_coverage_delta_point_reference_count = 10240U;
  budget.sparse_delta = {1024U, 10240U, 1024U, 1024U, 10240U,
                         1024U, 10240U, 10240U, 1U};
  return budget;
}

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchProviderBudget
provider_budget() {
  return {1024U, 1024U, 10240U, 1024U, 1024U, 10240U, 65536U};
}

[[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityBudget
capability_budget() {
  return {generous_adapter_budget(), 1024U, 1024U,
          {provider_budget(), 1U}};
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget forest_budget() {
  return {4096U, 65536U, 65536U, 4096U, 4096U,
          65536U, 65536U, 65536U, 131072U, 4U};
}

[[nodiscard]] ExactDirectSparseRootLedgerBudget ledger_budget() {
  return {4096U, 8192U, 8192U, 32768U, 65536U, 32768U, 4096U,
          4096U, 4096U, 16384U, 16384U, 16384U, 262144U, 4U};
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureBudget
closure_budget() {
  const ExactDirectSparseStableFacetPositiveProbeBudget probe{
      4096U, 4096U, 4096U, 4096U};
  return {4096U,
          4096U,
          4096U,
          8193U,
          67108864U,
          probe,
          {65536U, 65536U, 65536U, 65536U, 1024U, 64U, 64U}};
}

[[nodiscard]] ExactDirectNormalizedH0SourceForestLedgerTransactionBudget
transaction_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactDirectNormalizedH0SourceForestLedgerTransactionBudget budget;
  budget.maximum_window_facet_scan_count = 4096U;
  budget.maximum_direct_reference_scan_count = 4096U;
  budget.maximum_coface_facet_reference_scan_count = 65536U;
  budget.maximum_equal_facet_resolution_count = 4096U;
  budget.maximum_merged_resolution_count = 4096U;
  budget.maximum_group_count = 4096U;
  budget.maximum_group_token_scan_count = 65536U;
  budget.maximum_parent_root_reference_count = 65536U;
  budget.maximum_group_point_delta_reference_count = 65536U;
  budget.maximum_standalone_birth_count = 4096U;
  budget.maximum_standalone_birth_point_reference_count = 65536U;
  budget.maximum_preview_requested_handle_count = 8192U;
  budget.maximum_scratch_entry_count = 131072U;
  budget.carrier_origin = {
      4096U,
      4096U,
      65536U,
      {4096U},
      {4096U,
       16777216U,
       16777216U,
       16777216U,
       4096U,
       {4096U, 8192U, 32768U, 65536U, 65536U, 262144U}}};
  budget.relative_batch = {maximum, maximum, unlimited_frozen_budget()};
  budget.forest_projection = {maximum, maximum, maximum,
                              maximum, maximum, maximum,
                              maximum, maximum, maximum};
  budget.forest_preview = {8192U, 32768U, 65536U, 65536U};
  return budget;
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
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {std::move(facade),
          std::move(event_journal),
          arm_budget,
          std::move(arm_journal),
          incidence_budget,
          std::move(incidence_journal)};
}

struct Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseGatewayCandidateBudget gateway_budget;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectNormalizedH0SourcePlanBudget source_plan_budget;
  ExactDirectNormalizedH0SourcePlanResult source_plan;
  ExactDirectRankWindowSaturatedH0Authority rank_authority;
  ExactDirectNormalizedH0IncidenceReductionAuthority incidence_authority;
  ExactDirectSparseUnifiedLevelPlanResult compatibility_plan;
  ExactDirectNormalizedH0ResidentSourceManifest manifest;
};

[[nodiscard]] Fixture fixture() {
  const std::array points{point(-2.0, -1.0), point(-2.0, 1.0),
                          point(0.0, 0.0), point(3.0, 2.0),
                          point(4.0, -1.0)};
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(index, cloud);
  const auto gateway_budget = generous_gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
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
  const auto source_plan_budget = generous_source_plan_budget();
  auto source_plan = build_exact_direct_normalized_h0_source_plan(
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
      gateway,
      source_plan_budget);
  auto rank_authority =
      build_exact_direct_rank_window_saturated_h0_authority(source.facade);
  auto incidence_authority =
      build_exact_direct_normalized_h0_incidence_reduction_authority(
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
          gateway,
          source_plan_budget,
          source_plan,
          rank_authority);
  auto compatibility =
      initialize_exact_direct_normalized_h0_resident_session(
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
          gateway,
          source_plan_budget,
          source_plan,
          generous_adapter_budget(),
          UINT64_C(0x5F17E001),
          generous_resident_budget());
  if (!compatibility.certified_initialized_session() ||
      !compatibility.session.has_value()) {
    throw std::runtime_error("compatibility fixture initialization failed");
  }
  auto compatibility_plan = compatibility.session->plan();
  auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          compatibility_plan, incidence_authority);
  return {std::move(cloud),
          std::move(index),
          std::move(source),
          gateway_budget,
          std::move(gateway),
          source_plan_budget,
          std::move(source_plan),
          std::move(rank_authority),
          std::move(incidence_authority),
          std::move(compatibility_plan),
          std::move(manifest)};
}

class CompatibilityProvider {
 public:
  CompatibilityProvider(
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
      const ExactDirectSparseUnifiedLevelPlanResult& plan)
      : manifest_(&manifest), plan_(&plan) {
    chain_prefixes_.push_back(manifest.initial_batch_chain_digest);
    ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch;
    for (std::size_t index = 0U; index < manifest.batch_count; ++index) {
      const auto window =
          borrow_exact_direct_normalized_h0_resident_compatibility_window(
              manifest, plan, index, chain_prefixes_.back(), scratch);
      if (window.source_batch_index != index) {
        throw std::runtime_error("provider chain construction failed");
      }
      chain_prefixes_.push_back(window.successor_chain_digest);
    }
  }

  [[nodiscard]] ExactDirectNormalizedH0ResidentBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectNormalizedH0ResidentBatchConsumerView consumer) {
    if (!consumer) {
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_consumer_rejected;
    }
    if (batch_index >= manifest_->batch_count) {
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_batch_out_of_range;
    }
    const auto window =
        borrow_exact_direct_normalized_h0_resident_compatibility_window(
            *manifest_,
            *plan_,
            batch_index,
            chain_prefixes_[batch_index],
            scratch_);
    return consumer(window)
               ? ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     complete_synchronous_visit
               : ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     no_consumer_rejected;
  }

 private:
  const ExactDirectNormalizedH0ResidentSourceManifest* manifest_{};
  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  std::vector<morsehgp3d::contract::CanonicalId> chain_prefixes_;
  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch_;
};

[[nodiscard]]
ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_session(
    const Fixture& value,
    CompatibilityProvider& provider,
    std::uint64_t authority_id) {
  return initialize_exact_direct_normalized_h0_scientific_window_capability(
      value.index,
      value.cloud,
      value.source.facade,
      value.source.event_journal,
      value.source.arm_budget,
      value.source.arm_journal,
      value.source.incidence_budget,
      value.source.incidence_journal,
      value.gateway_budget,
      LbvhTraversalOrder::near_first,
      value.gateway,
      value.source_plan_budget,
      value.source_plan,
      value.rank_authority,
      value.incidence_authority,
      value.manifest,
      provider,
      authority_id,
      capability_budget());
}

struct CarrierInput {
  std::vector<ExactDirectSparseStableFacetDescentClosureSeed> seeds;
  std::vector<ExactDirectSparseCarrierOriginSeedBinding> bindings;
};

[[nodiscard]] CarrierInput carrier_input(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned) {
  CarrierInput output;
  const auto& plan = owned.local_plan;
  const auto& batch = plan.batches.front();
  std::vector<std::uint8_t> births(plan.facet_tokens.size(), 0U);
  std::vector<std::uint8_t> touched(plan.facet_tokens.size(), 0U);
  for (std::size_t offset = 0U; offset < batch.direct_reference_count;
       ++offset) {
    const auto& reference =
        plan.direct_references[batch.direct_reference_offset + offset];
    if (reference.role == ExactDirectMorseH0Role::birth) {
      births[*reference.direct_birth_facet_token_index] = 1U;
    }
  }
  for (std::size_t offset = 0U;
       offset < batch.coface_facet_reference_count;
       ++offset) {
    touched[plan
                .coface_facet_references
                    [batch.coface_facet_reference_offset + offset]
                .facet_token_index] = 1U;
  }
  for (std::size_t local = 0U; local < plan.facet_tokens.size(); ++local) {
    if (touched[local] == 0U || births[local] != 0U) {
      continue;
    }
    const std::size_t seed_index = output.seeds.size();
    output.seeds.push_back({seed_index, plan.facet_tokens[local].facet_key});
    output.bindings.push_back(
        {seed_index, owned.local_to_stable_facet_token_indices[local]});
  }
  return output;
}

struct State {
  ExactDirectSparseStableFacetForest forest;
  ExactDirectSparseRootLedger ledger;
};

[[nodiscard]] State initialize_state(
    const ExactDirectNormalizedH0ScientificSourceStamp& stamp) {
  auto bounded_forest_budget = forest_budget();
  bounded_forest_budget.maximum_observed_handle_count =
      stamp.stable_facet_token_count();
  auto forest = initialize_exact_direct_sparse_stable_facet_forest(
      {stamp.stable_facet_token_count(), stamp.source_identity_digest(), 0U},
      bounded_forest_budget);
  if (!forest.certified_initialized() || !forest.forest.has_value()) {
    throw std::runtime_error("transaction forest initialization failed");
  }
  auto ledger = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *forest.forest);
  if (!ledger.certified_initialized() || !ledger.ledger.has_value()) {
    throw std::runtime_error("transaction ledger initialization failed");
  }
  return {std::move(*forest.forest), std::move(*ledger.ledger)};
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureResult
build_carrier_closure(
    const Fixture& value,
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned,
    const CarrierInput& carriers,
    const ExactDirectSparseStableFacetForest& forest) {
  return build_exact_direct_sparse_stable_facet_descent_closure(
      value.index,
      value.cloud,
      carriers.seeds,
      owned.local_plan.batches.front().squared_level,
      forest,
      closure_budget(),
      {},
      LbvhTraversalOrder::near_first);
}

void test_complete_stream_and_bounded_retry(const Fixture& value) {
  CompatibilityProvider provider{value.manifest, value.compatibility_plan};
  auto initialized =
      initialize_session(value, provider, UINT64_C(0x5F17E101));
  check(
      initialized.certified_scientific_initialization() &&
          initialized.session.has_value(),
      "the transaction fixture initializes one exact scientific session");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  auto state = initialize_state(session.scientific_source_stamp());

  bool cap_retry_exercised = false;
  bool equal_nonbirth_regression_exercised = false;
  while (!session.complete()) {
    auto prepared =
        prepare_exact_direct_normalized_h0_source_forest_ledger_window(
            session);
    check(
        prepared.certified_prepared_window(),
        "one transaction-owned scientific window prepares");
    if (!prepared.window.has_value()) {
      return;
    }
    const auto carriers = carrier_input(prepared.window->owned_window());
    auto closure = build_carrier_closure(
        value, prepared.window->owned_window(), carriers, state.forest);
    check(
        closure.certified_complete_relative_positive_closure(),
        "the exact strict-below carrier complement closes on the current forest");

    if (!cap_retry_exercised) {
      auto capped = transaction_budget();
      if (prepared.window->owned_window().local_plan.facet_tokens.empty()) {
        capped.maximum_scratch_entry_count = 0U;
      } else {
        capped.maximum_window_facet_scan_count =
            prepared.window->owned_window().local_plan.facet_tokens.size() -
            1U;
      }
      const auto source_cursor = session.batch_cursor();
      const auto forest_stamp = state.forest.current_stamp();
      const auto ledger_stamp = state.ledger.current_stamp();
      auto rejected =
          ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
              session,
              std::move(*prepared.window),
              closure,
              carriers.bindings,
              state.forest,
              state.ledger,
              capped);
      check(
          rejected.certified_no_commit_failure() &&
              rejected.disposition() ==
                  ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                      budget_exhausted &&
              session.batch_cursor() == source_cursor &&
              state.forest.current_stamp() == forest_stamp &&
              state.ledger.current_stamp() == ledger_stamp,
          "a cap-minus-one failure advances neither source, forest nor ledger");
      cap_retry_exercised = true;

      prepared =
          prepare_exact_direct_normalized_h0_source_forest_ledger_window(
              session);
      if (!prepared.window.has_value()) {
        return;
      }
      const auto retry_carriers =
          carrier_input(prepared.window->owned_window());
      closure = build_carrier_closure(
          value,
          prepared.window->owned_window(),
          retry_carriers,
          state.forest);
      auto committed =
          ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
              session,
              std::move(*prepared.window),
              closure,
              retry_carriers.bindings,
              state.forest,
              state.ledger,
              transaction_budget());
      check(
          committed.certified_complete_transaction(),
          "the exact same source window succeeds after a bounded retry");
      if (!committed.certified_complete_transaction()) {
        std::cerr << "retry disposition="
                  << static_cast<unsigned>(committed.disposition())
                  << " decision="
                  << static_cast<unsigned>(committed.decision()) << '\n';
        return;
      }
      continue;
    }

    bool matches_equal_nonbirth_regression = false;
    if (session.batch_cursor() == 7U &&
        prepared.window->owned_window().local_plan.batches.front().order ==
            2U) {
      const auto found = std::find_if(
          carriers.bindings.begin(),
          carriers.bindings.end(),
          [](const auto& binding) {
            return binding.stable_source_facet_token_index == 4U;
          });
      if (found != carriers.bindings.end()) {
        const auto& projection =
            closure.seed_projections()[found->closure_seed_index];
        const auto& root = closure.nodes()[projection.root_node_index];
        const auto center = root.exact_center.has_value()
                                ? root.exact_center->to_record()
                                : morsehgp3d::exact::ExactRational3Record{};
        matches_equal_nonbirth_regression =
            root.facet_key.point_count == 2U &&
            root.facet_key.point_ids[0] == 1U &&
            root.facet_key.point_ids[1] == 3U &&
            root.exact_squared_level.has_value() &&
            root.exact_squared_level->numerator_string() == "13" &&
            root.exact_squared_level->denominator_string() == "2" &&
            *root.exact_squared_level ==
                closure.closed_batch_squared_level() &&
            center.x_numerator == "1" && center.y_numerator == "3" &&
            center.z_numerator == "0" && center.denominator == "2";
      }
    }

    auto committed =
        ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
            session,
            std::move(*prepared.window),
            closure,
            carriers.bindings,
            state.forest,
            state.ledger,
            transaction_budget());
    const bool certified =
        committed.certified_complete_transaction() &&
            committed
                .equal_facet_authority_derived_from_scientific_window_and_exact_closure() &&
            committed.carrier_and_equal_partition_exhaustive() &&
            committed.counters().source_commit_attempt_count == 1U &&
            !committed.partial_commit_requires_rebuild() &&
            !committed.durable_atomicity_claimed() &&
            !committed.global_forbidden_structure_materialized() &&
            !committed.durable_restart_supported() &&
            !committed.vertical_maps_complete() &&
            !committed.public_status_claimed();
    check(
        certified,
        "each window commits forest+ledger before exactly one source commit without a forbidden global structure");
    if (!certified) {
      std::cerr << "stream disposition="
                << static_cast<unsigned>(committed.disposition())
                << " decision="
                << static_cast<unsigned>(committed.decision())
                << " batch=" << committed.source_batch_index()
                << " touched=" << committed.counters().touched_facet_count
                << " equal="
                << committed.counters().equal_facet_resolution_count
                << " carriers="
                << committed.counters().carrier_resolution_count
                << " seeds=" << carriers.bindings.size()
                << " projections=" << closure.seed_projections().size()
                << " nodes=" << closure.nodes().size() << '\n';
      return;
    }
    if (committed.source_batch_index() == 7U) {
      equal_nonbirth_regression_exercised =
          matches_equal_nonbirth_regression &&
          committed.counters()
                  .direct_birth_equal_facet_resolution_count == 0U &&
          committed.counters()
                  .closure_exact_equal_facet_resolution_count == 1U &&
          committed.counters().equal_facet_resolution_count == 1U &&
          committed.counters().carrier_resolution_count == 2U &&
          committed.counters().touched_facet_count == 3U;
      check(
          equal_nonbirth_regression_exercised,
          "the permanent E5 batch-7 counterexample is classified equal by its exact source-facet miniball");
    }
  }

  check(
      cap_retry_exercised && equal_nonbirth_regression_exercised &&
          session.batch_cursor() == value.manifest.batch_count &&
          state.forest.current_stamp().committed_batch_count ==
              value.manifest.batch_count &&
          state.ledger.current_stamp().committed_batch_count ==
              value.manifest.batch_count &&
          state.ledger.current_stamp().aligned_forest_stamp ==
              state.forest.current_stamp(),
      "the complete bounded E5 stream leaves source, forest and ledger on one exact batch cursor");
}

void test_foreign_source_window_rejected(const Fixture& value) {
  CompatibilityProvider provider_a{value.manifest, value.compatibility_plan};
  CompatibilityProvider provider_b{value.manifest, value.compatibility_plan};
  auto initialized_a =
      initialize_session(value, provider_a, UINT64_C(0x5F17E201));
  auto initialized_b =
      initialize_session(value, provider_b, UINT64_C(0x5F17E202));
  if (!initialized_a.session.has_value() ||
      !initialized_b.session.has_value()) {
    check(false, "two independent scientific sessions initialize");
    return;
  }
  auto& session_a = *initialized_a.session;
  auto& session_b = *initialized_b.session;
  auto state = initialize_state(session_a.scientific_source_stamp());
  auto foreign =
      prepare_exact_direct_normalized_h0_source_forest_ledger_window(
          session_b);
  if (!foreign.window.has_value()) {
    check(false, "the foreign transaction window prepares");
    return;
  }
  const auto carriers = carrier_input(foreign.window->owned_window());
  const auto closure = build_carrier_closure(
      value, foreign.window->owned_window(), carriers, state.forest);
  const auto forest_stamp = state.forest.current_stamp();
  const auto ledger_stamp = state.ledger.current_stamp();
  auto rejected =
      ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
          session_a,
          std::move(*foreign.window),
          closure,
          carriers.bindings,
          state.forest,
          state.ledger,
          transaction_budget());
  check(
      rejected.certified_no_commit_failure() &&
          rejected.decision() ==
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  no_foreign_or_stale_source_window_rejected &&
          session_a.batch_cursor() == 0U && session_b.batch_cursor() == 0U &&
          state.forest.current_stamp() == forest_stamp &&
          state.ledger.current_stamp() == ledger_stamp,
      "a privately minted window from a sibling session is rejected before any commit");
}

void test_sibling_ticket_and_foreign_closure_rejected(
    const Fixture& value) {
  CompatibilityProvider provider{value.manifest, value.compatibility_plan};
  auto initialized =
      initialize_session(value, provider, UINT64_C(0x5F17E301));
  if (!initialized.session.has_value()) {
    check(false, "the closure-rejection scientific session initializes");
    return;
  }
  auto& session = *initialized.session;
  auto source_state = initialize_state(session.scientific_source_stamp());
  auto foreign_state = initialize_state(session.scientific_source_stamp());
  auto prepared =
      prepare_exact_direct_normalized_h0_source_forest_ledger_window(
          session);
  if (!prepared.window.has_value()) {
    check(false, "the first private transaction window prepares");
    return;
  }

  const auto cursor = session.batch_cursor();
  auto sibling =
      prepare_exact_direct_normalized_h0_source_forest_ledger_window(
          session);
  check(
      !sibling.window.has_value() &&
          sibling.decision ==
              ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
                  no_source_window_rejected &&
          session.batch_cursor() == cursor && prepared.window->valid(),
      "the source capability refuses a sibling transaction ticket while preserving the first");

  const auto carriers = carrier_input(prepared.window->owned_window());
  const auto foreign_closure = build_carrier_closure(
      value,
      prepared.window->owned_window(),
      carriers,
      source_state.forest);
  check(
      foreign_closure.certified_complete_relative_positive_closure() &&
          !foreign_closure.certified_for(foreign_state.forest.current_stamp()),
      "the strict-below closure is privately stamped to its source forest");
  const auto forest_stamp = foreign_state.forest.current_stamp();
  const auto ledger_stamp = foreign_state.ledger.current_stamp();
  auto rejected =
      ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
          session,
          std::move(*prepared.window),
          foreign_closure,
          carriers.bindings,
          foreign_state.forest,
          foreign_state.ledger,
          transaction_budget());
  check(
      rejected.certified_no_commit_failure() &&
          rejected.decision() ==
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  no_carrier_closure_partition_rejected &&
          session.batch_cursor() == cursor &&
          foreign_state.forest.current_stamp() == forest_stamp &&
          foreign_state.ledger.current_stamp() == ledger_stamp &&
          !rejected.partial_commit_requires_rebuild(),
      "a complete closure stamped by another forest is rejected without advancing any state");
}

}  // namespace

int main() {
  try {
    const auto value = fixture();
    test_complete_stream_and_bounded_retry(value);
    test_foreign_source_window_rejected(value);
    test_sibling_ticket_and_foreign_closure_rejected(value);
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " source-forest-ledger transaction checks failed\n";
    return 1;
  }
  std::cout << "source-forest-ledger transaction checks passed\n";
  return 0;
}
