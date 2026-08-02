#include "morsehgp3d/hierarchy/direct_normalized_h0_relative_frozen_incidence_batch.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilitySession> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilitySession>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatch> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatch>);

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
  return {
      4096U,
      65536U,
      65536U,
      655360U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_first_incidence_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
generous_gateway_identity_budget() {
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

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget
generous_source_plan_budget() {
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
      generous_gateway_identity_budget(),
  };
}

[[nodiscard]] ExactDirectNormalizedH0ResidentAdapterBudget
generous_adapter_budget() {
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

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget
unlimited_frozen_budget() {
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
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
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

[[nodiscard]] ExactDirectMorseUnifiedResidentSessionBudget
generous_resident_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget;
  budget.locator = {
      1024U,
      1024U,
      10240U,
      1024U,
      1024U,
      1024U,
      1024U,
      1024U,
      10240U,
      4097U,
      4097U,
  };
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
  budget.sparse_delta = {
      1024U,
      10240U,
      1024U,
      1024U,
      10240U,
      1024U,
      10240U,
      10240U,
      1U,
  };
  return budget;
}

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchProviderBudget
provider_budget() {
  return {1024U, 1024U, 10240U, 1024U, 1024U, 10240U, 65536U};
}

[[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityBudget
capability_budget() {
  return {
      generous_adapter_budget(),
      1024U,
      1024U,
      {provider_budget(), 1U},
  };
}

[[nodiscard]] ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget
relative_batch_budget() {
  return {
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<std::size_t>::max(), unlimited_frozen_budget()};
}

struct RelativeBatchAuthority {
  std::vector<std::size_t> local_facet_indices;
  std::vector<ExactDirectNormalizedH0StableFacetResolution> resolutions;
  std::vector<ExactDirectFrozenUnifiedLatentCarrierCoverage> latent_coverages;
  std::vector<PointId> latent_coverage_points;
};

[[nodiscard]] RelativeBatchAuthority latent_authority_for(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned) {
  if (owned.local_plan.batches.size() != 1U ||
      owned.local_to_stable_facet_token_indices.size() !=
          owned.local_plan.facet_tokens.size()) {
    throw std::runtime_error(
        "the scientific window does not expose one bijective local plan");
  }
  const auto& batch = owned.local_plan.batches.front();
  std::vector<std::size_t> touched;
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    touched.push_back(
        owned.local_plan
            .coface_facet_references
                [batch.coface_facet_reference_offset + local]
            .facet_token_index);
  }
  std::sort(touched.begin(), touched.end());
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

  RelativeBatchAuthority authority;
  for (const std::size_t local_facet_index : touched) {
    if (local_facet_index >= owned.local_plan.facet_tokens.size()) {
      throw std::runtime_error(
          "the scientific window contains an out-of-range local facet");
    }
    const std::size_t stable_facet_index =
        owned.local_to_stable_facet_token_indices[local_facet_index];
    const ExactFrozenIncidenceTokenId token_id =
        10000U + stable_facet_index;
    authority.local_facet_indices.push_back(local_facet_index);
    authority.resolutions.push_back(
        {stable_facet_index,
         {ExactFrozenIncidenceTokenKind::latent_carrier, token_id},
         std::nullopt});
    const auto& key =
        owned.local_plan.facet_tokens[local_facet_index].facet_key;
    const std::size_t point_offset = authority.latent_coverage_points.size();
    authority.latent_coverage_points.insert(
        authority.latent_coverage_points.end(),
        key.point_ids.begin(),
        key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count));
    authority.latent_coverages.push_back(
        {token_id, point_offset, key.point_count});
  }
  return authority;
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
    const MortonLbvhIndex& index, const CanonicalPointCloud& cloud) {
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
  return {
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

struct E5Fixture {
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

[[nodiscard]] E5Fixture e5_fixture() {
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
  auto compatibility = initialize_exact_direct_normalized_h0_resident_session(
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
      UINT64_C(0xE5C001),
      generous_resident_budget());
  if (!compatibility.certified_initialized_session() ||
      !compatibility.session.has_value()) {
    throw std::runtime_error(
        "the real E5 normalized compatibility session did not initialize");
  }
  auto compatibility_plan = compatibility.session->plan();
  auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          compatibility_plan, incidence_authority);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      gateway_budget,
      std::move(gateway),
      source_plan_budget,
      std::move(source_plan),
      std::move(rank_authority),
      std::move(incidence_authority),
      std::move(compatibility_plan),
      std::move(manifest),
  };
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
              manifest,
              plan,
              index,
              chain_prefixes_.back(),
              scratch);
      if (window.source_batch_index != index) {
        throw std::runtime_error(
            "the real E5 compatibility provider could not index a window");
      }
      chain_prefixes_.push_back(window.successor_chain_digest);
    }
  }

  bool corrupt_chain{false};

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
    auto window =
        borrow_exact_direct_normalized_h0_resident_compatibility_window(
            *manifest_,
            *plan_,
            batch_index,
            chain_prefixes_[batch_index],
            scratch_);
    if (corrupt_chain) {
      window.source_chain_digest = {};
    }
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

[[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_capability(
    const E5Fixture& fixture,
    const ExactDirectNormalizedH0IncidenceReductionAuthority& authority,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    CompatibilityProvider& provider,
    std::uint64_t scientific_authority_id) {
  return initialize_exact_direct_normalized_h0_scientific_window_capability(
      fixture.index,
      fixture.cloud,
      fixture.source.facade,
      fixture.source.event_journal,
      fixture.source.arm_budget,
      fixture.source.arm_journal,
      fixture.source.incidence_budget,
      fixture.source.incidence_journal,
      fixture.gateway_budget,
      LbvhTraversalOrder::near_first,
      fixture.gateway,
      fixture.source_plan_budget,
      fixture.source_plan,
      fixture.rank_authority,
      authority,
      manifest,
      provider,
      scientific_authority_id,
      capability_budget());
}

void test_full_e5_stream_and_terminal_seal(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto initialized = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C101));
  check(
          initialized.certified_scientific_initialization() &&
          initialized.horizontal_incidence_authority_verification_count == 1U &&
          initialized.normalized_source_plan_verification_count == 2U &&
          initialized.provider_full_replay_count == 1U &&
          !initialized.global_compatibility_plan_retained &&
          !initialized.vertical_maps_complete &&
          !initialized.public_status_claimed,
      "the real E5 source mints one non-forgeable scientific window session after fresh recursive replay");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  check(
      fixture.manifest.batch_count == 9U &&
          fixture.manifest.batch_count ==
              fixture.compatibility_plan.batches.size() &&
          session.certified_scientific_window_stream() &&
          session.horizontal_incidence_source_bound() &&
          !session.global_compatibility_plan_retained() &&
          session.provider_identity() == &provider,
      "the order-two E5 scientific session binds all nine manifested batches to the exact provider without retaining the global plan");

  std::size_t committed = 0U;
  bool relative_batch_checked = false;
  std::size_t relative_source_batch_index = 0U;
  std::vector<std::size_t> retained_local_to_stable;
  std::optional<ExactDirectNormalizedH0RelativeFrozenIncidenceBatch>
      retained_relative_batch;
  while (!session.complete()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_scientific_preparation() &&
            prepared.ticket.has_value() &&
            prepared.ticket->owned_window().source_batch_index == committed &&
            prepared.horizontal_incidence_source_bound &&
            prepared.exact_manifest_and_chain_bound &&
            !prepared.global_compatibility_plan_retained,
        "every real E5 batch receives a window-local scientific capability");
    if (!prepared.ticket.has_value()) {
      return;
    }
    if (!relative_batch_checked) {
      const auto& owned = prepared.ticket->owned_window();
      const auto authority = latent_authority_for(owned);
      const bool stable_mapping_is_non_identity = std::any_of(
          authority.local_facet_indices.begin(),
          authority.local_facet_indices.end(),
          [&](std::size_t local_facet_index) {
            return owned.local_to_stable_facet_token_indices
                       [local_facet_index] != local_facet_index;
          });
      if (committed > 0U && !authority.resolutions.empty() &&
          stable_mapping_is_non_identity) {
        auto relative =
            ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                *prepared.ticket,
                authority.resolutions,
                {},
                {},
                authority.latent_coverages,
                authority.latent_coverage_points,
                relative_batch_budget());
        if (relative.certified_scientific_relative_build()) {
          check(
              relative.batch.has_value() &&
                  relative.batch->source_batch_index() == committed &&
                  owned.local_plan.batches.front().batch_index == committed &&
                  relative.batch->manifest_digest() ==
                      fixture.manifest.manifest_digest &&
                  std::equal(
                      relative.batch
                          ->local_to_stable_facet_token_indices()
                          .begin(),
                      relative.batch
                          ->local_to_stable_facet_token_indices()
                          .end(),
                      owned.local_to_stable_facet_token_indices.begin(),
                      owned.local_to_stable_facet_token_indices.end()) &&
                  relative.batch->frozen_batch()
                      .normalized_direct_source_authority &&
                  !relative.batch->frozen_batch()
                       .source_plan_freshly_verified &&
                  prepared.ticket->valid(),
              "a scientific window builds one exact frozen quotient/action batch through stable handles without consuming the window ticket");

          auto window_capped_budget = relative_batch_budget();
          window_capped_budget.maximum_window_facet_translation_count =
              owned.local_to_stable_facet_token_indices.size() - 1U;
          const auto window_capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  {},
                  {},
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  window_capped_budget);
          check(
              !window_capped.batch.has_value() &&
                  window_capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_translation_budget_rejected &&
                  prepared.ticket->valid(),
              "the relative builder rejects a window-mapping cap minus one without consuming the scientific ticket");

          auto capped_budget = relative_batch_budget();
          capped_budget.maximum_stable_resolution_translation_count =
              authority.resolutions.size() - 1U;
          const auto capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  {},
                  {},
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  capped_budget);
          check(
              !capped.batch.has_value() &&
                  capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_translation_budget_rejected &&
                  prepared.ticket->valid(),
              "the relative builder rejects a translation cap minus one without consuming the scientific ticket");

          auto duplicate_resolutions = authority.resolutions;
          duplicate_resolutions.push_back(authority.resolutions.front());
          const auto duplicate =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  duplicate_resolutions,
                  {},
                  {},
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  relative_batch_budget());
          check(
              !duplicate.batch.has_value() &&
                  duplicate.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_stable_resolution_order_rejected &&
                  prepared.ticket->valid(),
              "duplicate stable resolutions are rejected before frozen construction without consuming the scientific ticket");

          auto frozen_capped_budget = relative_batch_budget();
          frozen_capped_budget.frozen_batch.maximum_facet_resolution_count =
              authority.resolutions.size() - 1U;
          const auto frozen_capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  {},
                  {},
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  frozen_capped_budget);
          check(
              !frozen_capped.batch.has_value() &&
                  frozen_capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_frozen_batch_rejected &&
                  prepared.ticket->valid(),
              "the frozen resolution cap minus one rejects atomically without consuming the scientific ticket");

          auto outside_resolutions = authority.resolutions;
          outside_resolutions.back().stable_source_facet_token_index =
              fixture.manifest.stable_facet_token_count;
          const auto outside =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  outside_resolutions,
                  {},
                  {},
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  relative_batch_budget());
          check(
              !outside.batch.has_value() &&
                  outside.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_stable_resolution_outside_window_rejected &&
                  prepared.ticket->valid(),
              "a stable handle outside the authenticated window is rejected without consuming the scientific ticket");
          relative_batch_checked =
              window_capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_translation_budget_rejected &&
              capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_translation_budget_rejected &&
              duplicate.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_stable_resolution_order_rejected &&
              frozen_capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_frozen_batch_rejected &&
              outside.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_stable_resolution_outside_window_rejected;
          if (relative_batch_checked && relative.batch.has_value()) {
            relative_source_batch_index = committed;
            retained_local_to_stable.assign(
                relative.batch->local_to_stable_facet_token_indices().begin(),
                relative.batch->local_to_stable_facet_token_indices().end());
            retained_relative_batch.emplace(std::move(*relative.batch));
          }
        }
      }
    }
    const auto committed_window = session.commit(std::move(*prepared.ticket));
    check(
        committed_window.certified_scientific_commit() &&
            committed_window.source_batch_index == committed &&
            committed_window.committed_cursor == committed + 1U &&
            committed_window.committed_epoch == committed + 1U &&
            !committed_window.resident_scientific_state_mutated &&
            !committed_window.vertical_maps_complete,
        "every real E5 window advances exactly one scientific prefix");
    ++committed;
  }
  check(
      relative_batch_checked && relative_source_batch_index > 0U &&
          retained_relative_batch.has_value() &&
          retained_relative_batch->valid() &&
          std::equal(
              retained_relative_batch->local_to_stable_facet_token_indices()
                  .begin(),
              retained_relative_batch->local_to_stable_facet_token_indices()
                  .end(),
              retained_local_to_stable.begin(),
              retained_local_to_stable.end()) &&
          committed == fixture.manifest.batch_count &&
          session.complete() &&
          session.current_chain_digest() ==
              fixture.manifest.final_batch_chain_digest,
      "the non-identity relative frozen seam remains independently interpretable after its ticket is committed and the complete E5 scientific prefix reaches the exact terminal chain");
  const auto sealed = session.seal();
  check(
      sealed.certified_scientific_seal() && sealed.newly_sealed &&
          sealed.seal->committed_batch_count == fixture.manifest.batch_count &&
          sealed.seal->manifest_digest == fixture.manifest.manifest_digest &&
          sealed.seal->final_chain_digest ==
              fixture.manifest.final_batch_chain_digest &&
          sealed.seal->every_committed_window_scientifically_bound &&
          !sealed.seal->resident_fold_executed &&
          !sealed.seal->vertical_maps_complete &&
          !sealed.seal->durable_restart_supported &&
          !sealed.seal->public_status_claimed && session.sealed(),
      "the real E5 terminal seal remains horizontal, transient and non-public");
}

void test_forged_authority_is_rejected(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto forged = fixture.incidence_authority;
  forged.vertical_maps_complete = true;
  const auto rejected = initialize_capability(
      fixture,
      forged,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C201));
  check(
      !rejected.session.has_value() &&
          rejected.horizontal_incidence_authority_verification_count == 1U &&
          !rejected.horizontal_incidence_authority_freshly_verified &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
                  no_horizontal_incidence_authority_rejected,
      "a public authority forged into a vertical claim cannot mint a scientific window capability");
}

void test_structural_plan_manifest_mutation_is_rejected(
    const E5Fixture& fixture) {
  auto mutated_plan = fixture.compatibility_plan;
  if (mutated_plan.direct_references.empty() ||
      mutated_plan.source_role_record_count < 2U) {
    check(false, "the real E5 compatibility plan exposes a mutation target");
    return;
  }
  auto& mutated_reference = mutated_plan.direct_references.front();
  mutated_reference.source_role_record_index =
      (mutated_reference.source_role_record_index + 1U) %
      mutated_plan.source_role_record_count;
  auto mutated_manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          mutated_plan, fixture.incidence_authority);
  CompatibilityProvider provider{mutated_manifest, mutated_plan};
  const auto structural =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          mutated_manifest, provider, provider_budget());
  check(
      mutated_manifest.certified() && structural.result_certified &&
          structural.every_window_freshly_recertified &&
          structural.final_chain_matches_manifest &&
          mutated_manifest != fixture.manifest,
      "the mutated E5 plan and manifest remain self-consistent under the structure-only verifier");
  const auto rejected = initialize_capability(
      fixture,
      fixture.incidence_authority,
      mutated_manifest,
      provider,
      UINT64_C(0xE5C301));
  check(
      !rejected.session.has_value() &&
          rejected.expected_manifest_freshly_reconstructed &&
          !rejected.observed_manifest_recursively_equal &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
                  no_scientific_manifest_mismatch,
      "fresh exact E5 reconstruction rejects a structurally certified but scientifically mutated plan and manifest");
}

void test_foreign_and_replayed_tickets(const E5Fixture& fixture) {
  CompatibilityProvider first_provider{
      fixture.manifest, fixture.compatibility_plan};
  CompatibilityProvider second_provider{
      fixture.manifest, fixture.compatibility_plan};
  auto first = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      first_provider,
      UINT64_C(0xE5C401));
  auto second = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      second_provider,
      UINT64_C(0xE5C402));
  check(
      first.certified_scientific_initialization() &&
          second.certified_scientific_initialization() &&
          first.session->provider_identity() == &first_provider &&
          second.session->provider_identity() == &second_provider &&
          first.session->provider_identity() != second.session->provider_identity(),
      "two E5 capabilities retain distinct process-local provider authorities");
  if (!first.session.has_value() || !second.session.has_value()) {
    return;
  }
  auto foreign_ticket = first.session->prepare_next();
  check(
      foreign_ticket.certified_scientific_preparation(),
      "the first E5 provider prepares a ticket for the foreign-provider test");
  if (!foreign_ticket.ticket.has_value()) {
    return;
  }
  const auto foreign = second.session->commit(std::move(*foreign_ticket.ticket));
  check(
      foreign.ticket_consumed &&
          foreign.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
                  no_foreign_or_stale_ticket_rejected &&
          first.session->batch_cursor() == 0U &&
          second.session->batch_cursor() == 0U,
      "a scientific ticket cannot cross E5 provider/session authority boundaries");

  auto replay_ticket = first.session->prepare_next();
  check(
      replay_ticket.certified_scientific_preparation(),
      "the original E5 session recovers after its foreign ticket was consumed");
  if (!replay_ticket.ticket.has_value()) {
    return;
  }
  const auto committed =
      first.session->commit(std::move(*replay_ticket.ticket));
  const auto replayed =
      first.session->commit(std::move(*replay_ticket.ticket));
  check(
      committed.certified_scientific_commit() &&
          !replayed.ticket_consumed &&
          replayed.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
                  no_ticket_invalid_or_consumed &&
          first.session->batch_cursor() == 1U,
      "a consumed move-only E5 capability cannot be replayed");
}

void test_corrupt_provider_chain_is_rejected(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto initialized = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C501));
  check(
      initialized.certified_scientific_initialization(),
      "the clean E5 provider passes the mandatory full replay before corruption");
  if (!initialized.session.has_value()) {
    return;
  }
  provider.corrupt_chain = true;
  const auto rejected = initialized.session->prepare_next();
  check(
      !rejected.ticket.has_value() &&
          rejected.structural_decision ==
              ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
                  no_window_prefix_or_cursor_rejected &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
                  no_underlying_preparation_rejected &&
          initialized.session->batch_cursor() == 0U &&
          initialized.session->epoch() == 0U &&
          initialized.session->current_chain_digest() ==
              fixture.manifest.initial_batch_chain_digest,
      "a provider corrupted after full replay cannot advance the scientific E5 prefix");
  provider.corrupt_chain = false;
  auto recovered = initialized.session->prepare_next();
  check(
      recovered.certified_scientific_preparation(),
      "the E5 session remains unmodified and can retry after a corrupt chain rejection");
}

}  // namespace

int main() {
  try {
    const E5Fixture fixture = e5_fixture();
    check(
        fixture.source_plan.certified_complete_candidate_source_plan() &&
            fixture.rank_authority.certified() &&
            fixture.incidence_authority
                .certified_horizontal_incidence_reduction() &&
            fixture.incidence_authority.incidence_complete_reduction_proved &&
            fixture.manifest.certified(),
        "the real E5 fixture supplies every exact scientific prerequisite");
    test_full_e5_stream_and_terminal_seal(fixture);
    test_forged_authority_is_rejected(fixture);
    test_structural_plan_manifest_mutation_is_rejected(fixture);
    test_foreign_and_replayed_tickets(fixture);
    test_corrupt_provider_chain_is_rejected(fixture);
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures
              << " direct normalized-H0 scientific-window capability test(s) failed\n";
    return 1;
  }
  std::cout
      << "direct normalized-H0 scientific-window capability tests passed\n";
  return 0;
}
