#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_scientific_window_capability.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::test {

using namespace hierarchy;
using exact::CertifiedPoint3;
using spatial::CanonicalPointCloud;
using spatial::LbvhTraversalOrder;
using spatial::MortonLbvhIndex;

[[nodiscard]] inline CertifiedPoint3 fixture_point(double x, double y) {
  return CertifiedPoint3::from_binary64(x, y, 0.0);
}

[[nodiscard]] inline ExactPairSupportStreamBudget pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] inline ExactHigherSupportStreamBudget higher_budget() {
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

[[nodiscard]] inline ExactDirectSaddleArmSeedBudget arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] inline ExactDirectClosedSaddleIncidenceBudget
incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] inline ExactDirectSparseFirstIncidenceBudget
first_incidence_budget() {
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

[[nodiscard]] inline ExactDirectSparseGatewayCandidateBudget
gateway_budget() {
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

[[nodiscard]] inline
ExactDirectSparseGatewayCandidateScientificIdentityBudget
gateway_identity_budget() {
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

[[nodiscard]] inline ExactDirectNormalizedH0SourcePlanBudget
source_plan_budget() {
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

[[nodiscard]] inline ExactDirectNormalizedH0ResidentAdapterBudget
adapter_budget() {
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

[[nodiscard]] inline ExactDirectFrozenUnifiedIncidenceBatchBudget
frozen_budget() {
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

[[nodiscard]] inline ExactDirectMorseUnifiedResidentSessionBudget
resident_budget() {
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

[[nodiscard]] inline ExactDirectNormalizedH0ResidentBatchProviderBudget
provider_budget() {
  return {1024U, 1024U, 10240U, 1024U, 1024U, 10240U, 65536U};
}

[[nodiscard]] inline
ExactDirectNormalizedH0ScientificWindowCapabilityBudget capability_budget() {
  return {
      adapter_budget(),
      1024U,
      1024U,
      {provider_budget(), 1U},
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

[[nodiscard]] inline DirectSources build_direct_sources(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal = build_exact_direct_morse_event_journal(cloud, facade);
  const auto source_arm_budget = arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, source_arm_budget);
  const auto source_incidence_budget = incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          source_arm_budget,
          arm_journal,
          source_incidence_budget);
  return {
      std::move(facade),
      std::move(event_journal),
      source_arm_budget,
      std::move(arm_journal),
      source_incidence_budget,
      std::move(incidence_journal),
  };
}

struct ScientificResidualFixture {
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

[[nodiscard]] inline ScientificResidualFixture residual_scientific_fixture() {
  // This permanent exact fixture has two first-incidence residual cofaces and
  // exercises residual/silent projection through the scientific wrapper.
  const std::array points{
      fixture_point(-12.0, -11.0),
      fixture_point(-9.0, -12.0),
      fixture_point(-6.0, -12.0),
      fixture_point(-3.0, 19.0),
      fixture_point(0.0, 14.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  auto index = MortonLbvhIndex::build(cloud);
  auto source = build_direct_sources(index, cloud);
  const auto source_gateway_budget = gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      source_gateway_budget,
      LbvhTraversalOrder::near_first);
  const auto plan_budget = source_plan_budget();
  auto source_plan = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      source_gateway_budget,
      LbvhTraversalOrder::near_first,
      gateway,
      plan_budget);
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
          source_gateway_budget,
          LbvhTraversalOrder::near_first,
          gateway,
          plan_budget,
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
      source_gateway_budget,
      LbvhTraversalOrder::near_first,
      gateway,
      plan_budget,
      source_plan,
      adapter_budget(),
      UINT64_C(0xC071B001),
      resident_budget());
  if (!compatibility.certified_initialized_session() ||
      !compatibility.session.has_value()) {
    throw std::runtime_error(
        "the residual normalized compatibility session did not initialize");
  }
  auto compatibility_plan = compatibility.session->plan();
  auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          compatibility_plan, incidence_authority);
  if (!source_plan.certified_complete_candidate_source_plan() ||
      source_plan.required_residual_coface_count != 2U ||
      compatibility_plan.required_residual_reference_count != 2U ||
      !incidence_authority.certified_horizontal_incidence_reduction() ||
      !manifest.certified()) {
    throw std::runtime_error(
        "the residual scientific fixture did not retain residual authority");
  }
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      source_gateway_budget,
      std::move(gateway),
      plan_budget,
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
            "the residual compatibility provider could not index a window");
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
  std::vector<contract::CanonicalId> chain_prefixes_;
  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch_;
};

[[nodiscard]] inline
ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_capability(
    const ScientificResidualFixture& fixture,
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
      fixture.incidence_authority,
      fixture.manifest,
      provider,
      scientific_authority_id,
      capability_budget());
}

}  // namespace morsehgp3d::test
