#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(direct_morse_unified_resident_session_schema_version == 6U);
static_assert(direct_frozen_unified_incidence_batch_schema_version == 3U);
static_assert(
    static_cast<std::uint8_t>(
        ExactDirectMorseUnifiedResidentInitializationDecision::
            complete_certified_bounded_resident_session) == 5U);
static_assert(
    static_cast<std::uint8_t>(
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_adapter_budget_rejected) == 6U);
static_assert(
    static_cast<std::uint8_t>(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            complete_certified_prepared_batch) == 14U);
static_assert(
    static_cast<std::uint8_t>(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_normalized_h0_retraction_authority_for_strictly_earlier_facet) ==
    15U);

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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return {BigInt{numerator}, BigInt{denominator}};
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

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget
generous_successive_budget() {
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
generous_normalized_plan_budget() {
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
generous_normalized_adapter_budget() {
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

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
generous_strict_facet_closure_budget() {
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

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
generous_star_budget() {
  return {
      1024U,
      11264U,
      11264U,
      112640U,
      131072U,
      131072U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_successive_budget(),
  };
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanBudget generous_plan_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      8388608U,
      1048576U,
      1048576U,
      1048576U,
      16777216U,
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
generous_session_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget{};
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
      2U,
  };
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
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order = 2U) {
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

struct E5Context {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget star_budget;
  ExactDirectSparseSuccessiveIncidenceStarJournalResult star;
  ExactDirectSparseUnifiedLevelPlanBudget plan_budget;
  ExactDirectSparseUnifiedLevelPlanResult plan;
};

struct NormalizedContext {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseGatewayCandidateBudget gateway_budget;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectNormalizedH0SourcePlanBudget plan_budget;
  ExactDirectNormalizedH0SourcePlanResult plan;
  ExactDirectRankWindowSaturatedH0Authority rank_authority;
};

[[nodiscard]] NormalizedContext normalized_context(
    CanonicalPointCloud cloud) {
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(cloud);
  auto gateway_budget = generous_gateway_budget();
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
  auto plan_budget = generous_normalized_plan_budget();
  auto plan = build_exact_direct_normalized_h0_source_plan(
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
      plan_budget);
  auto rank_authority =
      build_exact_direct_rank_window_saturated_h0_authority(source.facade);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      gateway_budget,
      std::move(gateway),
      plan_budget,
      std::move(plan),
      std::move(rank_authority),
  };
}

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize_normalized_candidate(
    const NormalizedContext& context,
    std::uint64_t authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& session_budget =
        generous_session_budget()) {
  return initialize_exact_direct_normalized_h0_resident_session(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      context.gateway_budget,
      LbvhTraversalOrder::near_first,
      context.gateway,
      context.plan_budget,
      context.plan,
      generous_normalized_adapter_budget(),
      authority_id,
      session_budget);
}

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize_normalized_certified(
    const NormalizedContext& context,
    const ExactDirectRankWindowSaturatedH0Authority& rank_authority,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    std::uint64_t authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& session_budget =
        generous_session_budget()) {
  return initialize_exact_direct_normalized_h0_certified_resident_session(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      context.gateway_budget,
      LbvhTraversalOrder::near_first,
      context.gateway,
      context.plan_budget,
      context.plan,
      rank_authority,
      closure_budget,
      {},
      LbvhTraversalOrder::near_first,
      generous_normalized_adapter_budget(),
      authority_id,
      session_budget);
}

[[nodiscard]] bool two_point_key_is(
    const ExactDirectSparseFacetKey& key,
    PointId first,
    PointId second) noexcept {
  return key.point_count == 2U && key.point_ids[0] == first &&
         key.point_ids[1] == second;
}

[[nodiscard]] E5Context e5_context() {
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
  auto source = direct_sources(cloud);
  auto star_budget = generous_star_budget();
  auto star = build_exact_direct_sparse_successive_incidence_star_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      LbvhTraversalOrder::near_first);
  auto plan_budget = generous_plan_budget();
  auto plan = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      plan_budget);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      star_budget,
      std::move(star),
      plan_budget,
      std::move(plan),
  };
}

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize(
    const E5Context& context,
    std::uint64_t authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) {
  return initialize_exact_direct_morse_unified_resident_session(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      context.star_budget,
      LbvhTraversalOrder::near_first,
      context.star,
      context.plan_budget,
      context.plan,
      authority_id,
      budget);
}

void test_ticket_guards(const E5Context& context) {
  auto first_init = initialize(context, 7001U, generous_session_budget());
  auto second_init = initialize(context, 7002U, generous_session_budget());
  check(
      first_init.certified_initialized_session() &&
          second_init.certified_initialized_session(),
      "two independent resident sessions initialize");
  first_init.global_regularity_authority_certified = true;
  check(
      !first_init.certified_initialized_session(),
      "a forged successive-Star global-regularity claim invalidates the initialization receipt");
  first_init.global_regularity_authority_certified = false;
  first_init.omitted_late_cofaces_qr1_noop_certified = true;
  check(
      !first_init.certified_initialized_session(),
      "a forged successive-Star omitted-no-op claim invalidates the initialization receipt");
  first_init.omitted_late_cofaces_qr1_noop_certified = false;
  first_init.normalized_verticality_certified = true;
  check(
      !first_init.certified_initialized_session(),
      "a forged successive-Star verticality claim invalidates the initialization receipt");
  first_init.normalized_verticality_certified = false;
  check(
      first_init.certified_initialized_session(),
      "the successive-Star initialization receipt recertifies after removing every forged unsupported claim");
  if (!first_init.certified_initialized_session() ||
      !second_init.certified_initialized_session()) {
    const auto debug_budget = generous_session_budget();
    const auto debug_locator =
        build_exact_direct_sparse_positive_facet_locator(
            context.plan.facet_tokens.size(),
            debug_budget.locator,
            {7001U, ~std::uint64_t{0U}});
    std::cerr << "ticket init decisions: "
              << static_cast<int>(first_init.decision) << ", "
              << static_cast<int>(second_init.decision)
              << "; locator=" << debug_locator.certified_positive_locator()
              << ", locator decision="
              << static_cast<int>(debug_locator.initialization_decision())
              << ", facets=" << context.plan.facet_tokens.size() << '\n';
  }
  if (!first_init.session.has_value() || !second_init.session.has_value()) {
    return;
  }
  auto& first = *first_init.session;
  auto& second = *second_init.session;

  auto foreign_preparation = first.prepare_next();
  check(
      foreign_preparation.certified_prepared_batch(),
      "a foreign-ticket fixture prepares");
  if (foreign_preparation.ticket.has_value()) {
    const auto* attested_batch_address =
        &foreign_preparation.ticket->authority_bundle().frozen_batch;
    ExactDirectMorseUnifiedResidentPreparedBatch moved_ticket =
        std::move(*foreign_preparation.ticket);
    check(
        moved_ticket.valid() && foreign_preparation.ticket->consumed() &&
            &moved_ticket.authority_bundle().frozen_batch ==
                attested_batch_address,
        "moving a ticket preserves the stable attested batch address and revalidates its capability");
    const auto foreign = second.commit(std::move(moved_ticket));
    check(
        foreign.decision ==
                ExactDirectMorseUnifiedResidentCommitDecision::
                    no_foreign_ticket_rejected &&
            second.batch_cursor() == 0U,
        "a foreign move-only ticket fails closed");
  }

  auto stale_a = second.prepare_next();
  auto stale_b = second.prepare_next();
  check(
      stale_a.certified_prepared_batch() &&
          stale_b.certified_prepared_batch(),
      "two tickets may observe the same strict pre-batch epoch");
  const auto third_ticket = second.prepare_next();
  check(
      third_ticket.decision ==
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_outstanding_ticket_budget_exhausted &&
          !third_ticket.ticket.has_value() && second.batch_cursor() == 0U,
      "the explicit two-ticket resident scratch cap rejects a third sibling");
  if (stale_a.ticket.has_value() && stale_b.ticket.has_value()) {
    const auto committed = second.commit(std::move(*stale_a.ticket));
    const auto stale = second.commit(std::move(*stale_b.ticket));
    check(
        committed.certified_committed_batch() &&
            stale.decision ==
                ExactDirectMorseUnifiedResidentCommitDecision::
                    no_stale_ticket_rejected,
        "the sibling ticket becomes stale after the first commit");
    const auto double_consumed =
        second.commit(std::move(*stale_a.ticket));
    check(
        double_consumed.decision ==
            ExactDirectMorseUnifiedResidentCommitDecision::
                no_ticket_already_consumed,
        "a consumed ticket cannot be committed twice");
  }
}

void test_cap_minus_one(const E5Context& context) {
  auto baseline_init = initialize(context, 7003U, generous_session_budget());
  check(
      baseline_init.certified_initialized_session(),
      "the sparse-delta cap fixture initializes its baseline session");
  if (!baseline_init.session.has_value()) {
    return;
  }
  auto& baseline = *baseline_init.session;
  std::size_t maximum_component_patch_count = 0U;
  while (!baseline.complete()) {
    auto prepared = baseline.prepare_next();
    if (!prepared.certified_prepared_batch() || !prepared.ticket.has_value()) {
      check(false, "the sparse-delta baseline prepares every E5 batch");
      return;
    }
    maximum_component_patch_count = std::max(
        maximum_component_patch_count,
        prepared.ticket->authority_bundle()
            .counters.sparse_delta_component_patch_count);
    if (!baseline.commit(std::move(*prepared.ticket))
             .certified_committed_batch()) {
      check(false, "the sparse-delta baseline commits every E5 batch");
      return;
    }
  }
  check(
      maximum_component_patch_count != 0U,
      "E5 exercises at least one sparse resident component patch");
  if (maximum_component_patch_count == 0U) {
    return;
  }

  auto budget = generous_session_budget();
  budget.sparse_delta.maximum_component_patch_count =
      maximum_component_patch_count - 1U;
  auto initialized = initialize(context, 7013U, budget);
  check(
      initialized.certified_initialized_session(),
      "the sparse-delta cap-minus-one session initializes before prepare");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  bool exact_atomic_rejection_observed = false;
  while (!session.complete()) {
    const std::size_t cursor_before = session.batch_cursor();
    const auto stamp_before = session.locator().snapshot_stamp();
    const auto components_before = session.component_states();
    const auto roots_before = session.root_coverages();
    const auto groups_before = session.group_records();
    auto prepared = session.prepare_next();
    if (!prepared.certified_prepared_batch()) {
      exact_atomic_rejection_observed =
          prepared.decision ==
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_sparse_delta_budget_exhausted &&
          !prepared.ticket.has_value() &&
          session.batch_cursor() == cursor_before &&
          session.locator().snapshot_stamp() == stamp_before &&
          session.component_states() == components_before &&
          session.root_coverages() == roots_before &&
          session.group_records() == groups_before;
      break;
    }
    if (!prepared.ticket.has_value() ||
        !session.commit(std::move(*prepared.ticket))
             .certified_committed_batch()) {
      break;
    }
  }
  check(
      exact_atomic_rejection_observed,
      "the exact sparse-delta component cap minus one rejects before every scientific mutation");
}

void test_locator_rejection_rolls_back_staged_delta(
    const E5Context& context) {
  auto baseline_initialized =
      initialize(context, 7014U, generous_session_budget());
  check(
      baseline_initialized.certified_initialized_session(),
      "the locator-rollback baseline session initializes");
  if (!baseline_initialized.session.has_value()) {
    return;
  }
  auto& baseline = *baseline_initialized.session;
  std::optional<std::size_t> target_cursor;
  std::size_t committed_binding_cap = 0U;
  while (!baseline.complete()) {
    auto prepared = baseline.prepare_next();
    if (!prepared.certified_prepared_batch() ||
        !prepared.ticket.has_value()) {
      break;
    }
    const auto& counters = prepared.ticket->authority_bundle().counters;
    const bool non_vacuous_resident_delta =
        counters.sparse_delta_component_patch_count != 0U &&
        (counters.sparse_delta_root_replacement_count != 0U ||
         counters.sparse_delta_new_root_count != 0U) &&
        counters.sparse_delta_group_append_count != 0U;
    const bool locator_binding_requested =
        counters.planned_equal_binding_count != 0U ||
        counters.planned_birth_binding_count != 0U;
    const auto stamp_before = baseline.locator().snapshot_stamp();
    const auto committed = baseline.commit(std::move(*prepared.ticket));
    if (!committed.certified_committed_batch()) {
      break;
    }
    const auto stamp_after = baseline.locator().snapshot_stamp();
    if (non_vacuous_resident_delta && locator_binding_requested &&
        stamp_after.inserted_key_count > stamp_before.inserted_key_count) {
      target_cursor = committed.committed_batch_index;
      committed_binding_cap = stamp_before.inserted_key_count;
      break;
    }
  }
  check(
      target_cursor.has_value(),
      "E5 exposes a rollback target with component, root, group and new locator binding work");
  if (!target_cursor.has_value()) {
    return;
  }

  auto budget = generous_session_budget();
  budget.locator.maximum_committed_binding_count = committed_binding_cap;
  auto initialized = initialize(context, 7015U, budget);
  check(
      initialized.certified_initialized_session(),
      "the locator-rollback fixture initializes at the exact pre-target binding cap");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  bool rollback_observed = false;
  while (!session.complete()) {
    const std::size_t cursor_before = session.batch_cursor();
    const std::size_t epoch_before = session.epoch();
    const auto stamp_before = session.locator().snapshot_stamp();
    const auto components_before = session.component_states();
    const auto roots_before = session.root_coverages();
    const auto groups_before = session.group_records();
    auto prepared = session.prepare_next();
    if (!prepared.certified_prepared_batch() ||
        !prepared.ticket.has_value()) {
      break;
    }
    const auto& counters = prepared.ticket->authority_bundle().counters;
    const bool non_vacuous_resident_delta =
        counters.sparse_delta_component_patch_count != 0U &&
        (counters.sparse_delta_root_replacement_count != 0U ||
         counters.sparse_delta_new_root_count != 0U) &&
        counters.sparse_delta_group_append_count != 0U;
    const bool locator_binding_requested =
        counters.planned_equal_binding_count != 0U ||
        counters.planned_birth_binding_count != 0U;
    const auto committed = session.commit(std::move(*prepared.ticket));
    if (committed.certified_committed_batch()) {
      continue;
    }
    rollback_observed =
        committed.decision ==
            ExactDirectMorseUnifiedResidentCommitDecision::
                no_locator_transaction_rejected &&
        committed.ticket_consumed &&
        committed.exactly_one_locator_apply_batch_called &&
        committed.sparse_delta_staged_with_rollback_before_locator &&
        !committed.staged_sparse_delta_released_after_locator_commit &&
        committed.no_scientific_state_mutated_on_failure &&
        cursor_before == *target_cursor && non_vacuous_resident_delta &&
        locator_binding_requested &&
        committed.locator_batch.counters.binding_request_count != 0U &&
        stamp_before.inserted_key_count == committed_binding_cap &&
        session.batch_cursor() == cursor_before &&
        session.epoch() == epoch_before &&
        session.locator().snapshot_stamp() == stamp_before &&
        session.component_states() == components_before &&
        session.root_coverages() == roots_before &&
        session.group_records() == groups_before;
    break;
  }
  check(
      rollback_observed,
      "a non-vacuous locator rejection restores every pre-staged resident arena and scalar");
}

void test_e5_live_twelve_batches(const E5Context& context) {
  auto initialized = initialize(context, 7004U, generous_session_budget());
  check(
      initialized.certified_initialized_session() &&
          initialized.source_plan_initial_verification_count == 1U,
      "E5 initializes from exactly one explicit session plan verification");
  if (!initialized.certified_initialized_session()) {
    std::cerr << "live init decision: "
              << static_cast<int>(initialized.decision) << '\n';
  }
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  check(
      context.plan.batches.size() == 12U &&
          session.source_plan_initial_verification_count() == 1U &&
          ExactDirectMorseUnifiedResidentSession::public_status ==
              "not_claimed" &&
          ExactDirectMorseUnifiedResidentSession::deployment_status ==
              "architecture_only_not_strict_miss_integration_qualified_v6",
      "the session advertises the certified-carrier architecture without claiming strict-miss integration qualification");

  std::optional<ExactFrozenIncidencePriorRootId> residual_root;
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp stamp_after_17_2{};
  bool residual_17_2_checked = false;
  bool level_9_depends_on_live_commit = false;
  std::size_t empty_group_record_count = 0U;

  while (!session.complete()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_prepared_batch(),
        "each E5 batch produces a certified strict pre-batch ticket");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto& bundle = prepared.ticket->authority_bundle();
    check(
        bundle.identity.batch_cursor == session.batch_cursor() &&
            bundle.identity.epoch == session.epoch() &&
            bundle.identity.locator_instance_id != 0U &&
            bundle.identity.locator_stamp ==
                session.locator().snapshot_stamp() &&
            !bundle.global_facet_coface_or_gamma_catalog_materialized &&
            !bundle.supplied_star_global_completeness_claimed &&
            !bundle.public_status_claimed &&
            bundle.counters.resident_state_full_copy_count == 0U &&
            !bundle.frozen_batch.source_plan_freshly_verified &&
            bundle.frozen_batch
                .source_plan_verified_once_by_immutable_resident_authority &&
            bundle.frozen_batch_receipt
                .certified_single_construction_receipt() &&
            bundle.frozen_batch_receipt.frozen_batch_construction_count ==
                1U &&
            bundle.frozen_batch_receipt
                    .quotient_streaming_verification_count == 1U &&
            bundle.frozen_batch_receipt
                    .action_plan_streaming_verification_count == 1U &&
            bundle.frozen_batch_receipt.structural_certification_count ==
                1U &&
            !bundle.frozen_batch_receipt
                 .independent_expected_batch_freshly_reconstructed &&
            bundle.counters.sparse_delta_group_append_count ==
                bundle.counters.planned_group_record_count,
        "the authority shares one live locator identity and carries one honestly attested frozen construction with one quotient/action streaming verification");

    if (bundle.squared_level == level(17, 2)) {
      residual_17_2_checked =
          bundle.frozen_batch.residual_incidence_records.size() == 2U &&
          bundle.frozen_batch.counters.residual_hyperedge_count == 2U &&
          bundle.frozen_batch.counters.direct_saddle_hyperedge_count == 0U &&
          bundle.frozen_batch.coverage_deltas.size() == 1U;
      if (!bundle.frozen_batch.coverage_deltas.empty()) {
        const auto& delta = bundle.frozen_batch.coverage_deltas.front();
        const auto& group = bundle.frozen_batch.action_plan.groups.front();
        if (group.q_r == 1U && group.prior_root_count == 1U) {
          residual_root = bundle.frozen_batch.action_plan.prior_root_ids
              [group.prior_root_offset];
        }
        if (delta.point_reference_count == 0U) {
          ++empty_group_record_count;
          residual_17_2_checked =
              residual_17_2_checked &&
              bundle.counters.sparse_delta_root_replacement_count == 0U;
        }
      }
    }
    if (bundle.squared_level == level(9)) {
      bool names_residual_root = false;
      for (const auto& resolution : bundle.facet_resolutions) {
        names_residual_root =
            names_residual_root ||
            (residual_root.has_value() &&
             resolution.prior_root_id == residual_root);
      }
      level_9_depends_on_live_commit =
          residual_root.has_value() && names_residual_root &&
          bundle.identity.locator_stamp == stamp_after_17_2;
    }

    const ExactLevel current_level = bundle.squared_level;
    const auto committed =
        session.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "each E5 ticket commits through one locator transaction");
    if (current_level == level(17, 2)) {
      stamp_after_17_2 = session.locator().snapshot_stamp();
    }
  }

  check(
      session.complete() && session.batch_cursor() == 12U &&
          session.epoch() == 12U &&
          session.locator().snapshot_stamp().committed_batch_count == 12U,
      "all twelve unified E5 batches are committed in one resident history");
  check(
      residual_17_2_checked,
      "the live 17/2 batch is the exact two-incidence residual case");
  check(
      level_9_depends_on_live_commit,
      "the level-9 authority names the root and locator stamp committed at 17/2");
  check(
      session.group_records().size() == 7U &&
          std::count_if(
              session.group_records().begin(),
              session.group_records().end(),
              [](const auto& record) {
                return record.empty_coverage_delta;
              }) >= 1 &&
          empty_group_record_count == 1U,
      "one durable group record survives even for the empty 17/2 point delta");
  std::size_t retained_delta_point_count = 0U;
  for (const auto& record : session.group_records()) {
    retained_delta_point_count += record.coverage_delta_points.size();
  }
  check(
      retained_delta_point_count == 6U,
      "E5 retains exactly six persistently budgeted group-delta points");
  check(
      session.frozen_batch_source_replay_count() == 0U &&
          session.frozen_batch_reconstruction_count() == 12U,
      "E5 performs zero per-batch source-plan replays and exactly one attested frozen construction for each of its twelve batches");
}

void test_group_delta_point_caps(const E5Context& context) {
  const auto run = [&](std::size_t cap,
                       std::uint64_t authority_id,
                       bool expect_complete) {
    auto budget = generous_session_budget();
    budget.maximum_group_coverage_delta_point_reference_count = cap;
    auto initialized = initialize(context, authority_id, budget);
    if (!initialized.certified_initialized_session() ||
        !initialized.session.has_value()) {
      return false;
    }
    auto& session = *initialized.session;
    while (!session.complete()) {
      const auto stamp_before = session.locator().snapshot_stamp();
      const std::size_t cursor_before = session.batch_cursor();
      auto prepared = session.prepare_next();
      if (!prepared.certified_prepared_batch()) {
        return !expect_complete &&
            prepared.decision ==
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_prepared_state_rejected &&
            session.batch_cursor() == cursor_before &&
            session.locator().snapshot_stamp() == stamp_before;
      }
      if (!prepared.ticket.has_value() ||
          !session.commit(std::move(*prepared.ticket))
               .certified_committed_batch()) {
        return false;
      }
    }
    return expect_complete;
  };
  check(
      run(6U, 7005U, true),
      "the exact persistent group-delta point cap admits all E5 batches");
  check(
      run(5U, 7006U, false),
      "the persistent group-delta point cap minus one fails before commit");
}

void test_normalized_source_resident_atomic_fold(const E5Context& context) {
  const auto gateway_budget = generous_gateway_budget();
  const auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      gateway_budget,
      LbvhTraversalOrder::near_first);
  const auto normalized_plan_budget = generous_normalized_plan_budget();
  const auto normalized_plan = build_exact_direct_normalized_h0_source_plan(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      gateway_budget,
      LbvhTraversalOrder::near_first,
      gateway,
      normalized_plan_budget);
  check(
      gateway.certified_partial_refinement() &&
          normalized_plan.certified_complete_candidate_source_plan(),
      "the normalized resident fixture starts from the certified direct candidate source");
  if (!normalized_plan.certified_complete_candidate_source_plan()) {
    return;
  }

  const auto initialize_normalized = [&](
      const ExactDirectNormalizedH0SourcePlanResult& candidate,
      const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
      std::uint64_t authority_id,
      const ExactDirectMorseUnifiedResidentSessionBudget& session_budget =
          generous_session_budget()) {
    return initialize_exact_direct_normalized_h0_resident_session(
        context.index,
        context.cloud,
        context.source.facade,
        context.source.event_journal,
        context.source.arm_budget,
        context.source.arm_journal,
        context.source.incidence_budget,
        context.source.incidence_journal,
        gateway_budget,
        LbvhTraversalOrder::near_first,
        gateway,
        normalized_plan_budget,
        candidate,
        adapter_budget,
        authority_id,
        session_budget);
  };

  auto forged_required_count = normalized_plan;
  forged_required_count.required_direct_coface_count =
      std::numeric_limits<std::size_t>::max();
  const auto forged_required_rejected = initialize_normalized(
      forged_required_count,
      generous_normalized_adapter_budget(),
      7016U);
  check(
      !forged_required_rejected.session.has_value() &&
          forged_required_rejected.source_plan_initial_verification_count ==
              0U &&
          forged_required_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_adapter_budget_rejected,
      "a forged huge required counter fails in allocation-free adapter preflight before source replay");

  auto locator_component_budget = generous_session_budget();
  locator_component_budget.locator.maximum_component_handle_count = 0U;
  const auto locator_component_cap_rejected = initialize_normalized(
      normalized_plan,
      generous_normalized_adapter_budget(),
      7017U,
      locator_component_budget);
  check(
      !locator_component_cap_rejected.session.has_value() &&
          locator_component_cap_rejected
                  .source_plan_initial_verification_count ==
              0U &&
          locator_component_cap_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_adapter_budget_rejected,
      "the normalized facet-token upper bound meets the locator component cap before source replay or plan allocation");

  auto locator_batch_budget = generous_session_budget();
  locator_batch_budget.locator.maximum_committed_batch_count = 0U;
  const auto locator_batch_cap_rejected = initialize_normalized(
      normalized_plan,
      generous_normalized_adapter_budget(),
      7018U,
      locator_batch_budget);
  check(
      !locator_batch_cap_rejected.session.has_value() &&
          locator_batch_cap_rejected.source_plan_initial_verification_count ==
              0U &&
          locator_batch_cap_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_adapter_budget_rejected,
      "the normalized batch count meets the locator committed-batch cap before source replay or plan allocation");

  auto scratch_budget = generous_normalized_adapter_budget();
  scratch_budget.maximum_batch_coface_index_scratch_count = 0U;
  const auto scratch_cap_rejected = initialize_normalized(
      normalized_plan, scratch_budget, 7019U);
  check(
      !scratch_cap_rejected.session.has_value() &&
          scratch_cap_rejected.source_plan_initial_verification_count == 0U &&
          scratch_cap_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_adapter_budget_rejected,
      "the largest batch-coface index scratch is capped before any reserve");

  auto coexistence_budget = generous_normalized_adapter_budget();
  coexistence_budget.maximum_simultaneous_adapter_entry_count = 0U;
  const auto coexistence_cap_rejected = initialize_normalized(
      normalized_plan, coexistence_budget, 7020U);
  check(
      !coexistence_cap_rejected.session.has_value() &&
          coexistence_cap_rejected.source_plan_initial_verification_count ==
              0U &&
          coexistence_cap_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_adapter_budget_rejected,
      "the simultaneous scratch-plus-plan peak is capped before any adapter allocation");

  auto forged_source_claim = normalized_plan;
  forged_source_claim.public_status_claimed = true;
  const auto forged_capability_rejected = initialize_normalized(
      forged_source_claim,
      generous_normalized_adapter_budget(),
      7021U);
  check(
      !forged_capability_rejected.session.has_value() &&
          forged_capability_rejected.source_plan_initial_verification_count ==
              1U &&
          forged_capability_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_source_plan_not_freshly_verified,
      "the integrated move-only plan authority is never issued for a forged normalized source claim");

  auto initialized = initialize_normalized(
      normalized_plan, generous_normalized_adapter_budget(), 7022U);
  check(
      initialized.certified_initialized_session() &&
          initialized.source_plan_initial_verification_count == 1U &&
          initialized.normalized_source_plan_consumed_directly &&
          initialized.normalized_sparse_compatibility_plan_certified &&
          initialized.every_normalized_coface_reconstructed_transiently &&
          initialized.normalized_h0_retraction_mode ==
              ExactDirectNormalizedH0ResidentRetractionMode::
                  candidate_fail_open_without_h0_retraction_authority &&
          !initialized.normalized_h0_retraction_authority_certified &&
          initialized
              .normalized_candidate_fails_open_on_strictly_earlier_facet &&
          !initialized.successive_incidence_star_materialized_by_adapter &&
          !initialized.global_regularity_authority_certified &&
          !initialized.omitted_late_cofaces_qr1_noop_certified &&
          !initialized.normalized_verticality_certified &&
          !initialized.public_status_claimed,
      "the normalized adapter verifies once and exposes no unsupported global, no-op, vertical or public claim");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  check(
      session.normalized_direct_source_session() &&
          session.normalized_h0_retraction_mode() ==
              ExactDirectNormalizedH0ResidentRetractionMode::
                  candidate_fail_open_without_h0_retraction_authority &&
          !session.normalized_h0_retraction_authority_certified() &&
          session.plan().required_direct_birth_reference_count ==
              normalized_plan.direct_birth_references.size() &&
          !session.plan().source_star_freshly_verified &&
          !session.plan().direct_star_cofaces_crosschecked_bijectively &&
          !session.plan().bounded_star_global_completeness_claimed &&
          session.plan().no_k_plus_one_coface_key_persisted &&
          !session.plan().public_status_claimed,
      "the resident cursor preserves direct births while remaining explicitly non-Star and non-public");

  bool observed_action_group = false;
  while (session.batch_cursor() < session.plan().batches.size()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_prepared_batch() &&
            prepared.ticket.has_value(),
        "each normalized batch freezes a strict pre-batch authority");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto& bundle = prepared.ticket->authority_bundle();
    observed_action_group = observed_action_group ||
        !bundle.frozen_batch.action_plan.groups.empty();
    check(
        bundle.frozen_batch.normalized_direct_source_authority &&
            !bundle.frozen_batch.successive_star_source_authority &&
            bundle.frozen_batch.scope ==
                ExactDirectFrozenUnifiedIncidenceBatchScope::
                    exact_selected_batch_relative_to_verified_normalized_direct_source_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only &&
            bundle.frozen_batch
                .frozen_hgp_action_plan_freshly_streaming_verified &&
            !bundle.global_facet_coface_or_gamma_catalog_materialized &&
            !bundle.supplied_star_global_completeness_claimed &&
            !bundle.public_status_claimed,
        "the normalized ticket retains q_R actions under the normalized immutable authority");
    check(
        session.commit(std::move(*prepared.ticket))
            .certified_committed_batch(),
        "the normalized sparse delta and locator commit atomically");
  }
  check(
      session.source_cursor_exhausted() && !session.complete() &&
          observed_action_group &&
          session.frozen_batch_source_replay_count() == 0U &&
          session.frozen_batch_reconstruction_count() ==
              session.plan().batches.size(),
      "the normalized candidate cursor is exhausted without claiming a complete H0 fold or replaying the source per batch");
}

void test_certified_normalized_strict_carrier_fold() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  const bool canonical_ids_preserved =
      cloud.size() == points.size() && cloud.source_index(0U) == 3U &&
      cloud.source_index(1U) == 0U && cloud.source_index(2U) == 1U &&
      cloud.source_index(3U) == 2U && cloud.source_index(4U) == 4U;
  auto context = normalized_context(std::move(cloud));
  check(
      canonical_ids_preserved,
      "the permanent AC-to-DE input preserves the documented canonical D,A,B,C,E ids");
  check(
      context.gateway.certified_partial_refinement(),
      "the permanent AC-to-DE cloud supplies the certified sparse gateway source");
  check(
      context.plan.certified_complete_candidate_source_plan(),
      "the permanent AC-to-DE cloud supplies the certified normalized source");
  check(
      context.rank_authority.certified(),
      "the permanent AC-to-DE cloud supplies the certified rank-window authority");
  if (!context.plan.certified_complete_candidate_source_plan() ||
      !context.rank_authority.certified()) {
    return;
  }

  auto forged_rank = context.rank_authority;
  forged_rank.public_status_claimed = true;
  const auto forged_rank_rejected = initialize_normalized_certified(
      context,
      forged_rank,
      generous_strict_facet_closure_budget(),
      7030U);
  check(
      !forged_rank_rejected.session.has_value() &&
          forged_rank_rejected.source_plan_initial_verification_count == 1U &&
          forged_rank_rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_rank_window_saturated_h0_authority_rejected,
      "a forged rank-window authority is rejected after the single normalized-source replay and before a certified session is issued");

  auto candidate_initialized =
      initialize_normalized_candidate(context, 7031U);
  check(
      candidate_initialized.certified_initialized_session() &&
          candidate_initialized.session.has_value(),
      "the unchanged candidate initializer remains independently constructible on the strict AC fixture");
  if (candidate_initialized.session.has_value()) {
    auto& candidate = *candidate_initialized.session;
    while (candidate.batch_cursor() < candidate.plan().batches.size()) {
      auto prepared = candidate.prepare_next();
      if (!prepared.certified_prepared_batch() ||
          !candidate.commit(std::move(*prepared.ticket))
               .certified_committed_batch()) {
        break;
      }
    }
    check(
        candidate.source_cursor_exhausted() && !candidate.complete() &&
            candidate.normalized_h0_retraction_mode() ==
                ExactDirectNormalizedH0ResidentRetractionMode::
                    candidate_fail_open_without_h0_retraction_authority,
        "the unchanged candidate initializer and fail-open completion semantics remain intact on the AC fixture");
  }

  constexpr std::uint64_t certified_authority_id = 7032U;
  auto initialized = initialize_normalized_certified(
      context,
      context.rank_authority,
      generous_strict_facet_closure_budget(),
      certified_authority_id);
  check(
      initialized.certified_initialized_session() &&
          initialized.session.has_value() &&
          initialized.normalized_h0_retraction_mode ==
              ExactDirectNormalizedH0ResidentRetractionMode::
                  certified_rank_window_and_sparse_strict_facet_closure &&
          initialized.normalized_h0_retraction_authority_certified &&
          !initialized
               .normalized_candidate_fails_open_on_strictly_earlier_facet &&
          initialized.rank_window_saturated_h0_authority_freshly_verified &&
          initialized.strict_facet_closure_session_capability_issued,
      "the second initializer issues the private certified carrier capability without a caller bool");
  if (!initialized.session.has_value()) {
    return;
  }

  auto& session = *initialized.session;
  const auto ac_token = std::find_if(
      session.plan().facet_tokens.begin(),
      session.plan().facet_tokens.end(),
      [](const ExactDirectSparseUnifiedLevelPlanFacetToken& token) {
        return two_point_key_is(token.facet_key, 1U, 3U);
      });
  const auto de_token = std::find_if(
      session.plan().facet_tokens.begin(),
      session.plan().facet_tokens.end(),
      [](const ExactDirectSparseUnifiedLevelPlanFacetToken& token) {
        return two_point_key_is(token.facet_key, 0U, 4U);
      });
  bool later_ac_resolved_to_de = false;
  while (session.batch_cursor() < session.plan().batches.size()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_prepared_batch() && prepared.ticket.has_value(),
        "every certified normalized batch prepares under the rank-window and sparse-closure capability");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto& bundle = prepared.ticket->authority_bundle();
    check(
        bundle.rank_window_saturated_h0_authority_certified &&
            bundle.every_locator_miss_has_fresh_exact_miniball &&
            bundle
                .every_strict_facet_miss_has_certified_relative_positive_closure &&
            bundle.strict_facet_closure_bound_to_pre_batch_locator_snapshot,
        "each certified ticket remains bound to the rank-window and strict-closure carrier scope");
    if (bundle.squared_level == level(83886, 3563) &&
        ac_token != session.plan().facet_tokens.end() &&
        de_token != session.plan().facet_tokens.end()) {
      const auto ac_resolution = std::find_if(
          bundle.facet_resolutions.begin(),
          bundle.facet_resolutions.end(),
          [&](const ExactDirectFrozenUnifiedFacetResolution& resolution) {
            return resolution.facet_token_index == ac_token->facet_token_index;
          });
      const auto de_probe = session.locator().probe_positive_facet(
          de_token->facet_key,
          {certified_authority_id, 9000001U},
          generous_session_budget().probe);
      later_ac_resolved_to_de =
          ac_resolution != bundle.facet_resolutions.end() &&
          de_probe.certified_positive_hit() &&
          ac_resolution->token.kind !=
              ExactFrozenIncidenceTokenKind::equal_facet &&
          ac_resolution->token.token_id == de_probe.component_handle;
    }
    check(
        session.commit(std::move(*prepared.ticket))
            .certified_committed_batch(),
        "the certified carrier resolutions, locator bindings and sparse resident delta commit atomically");
  }
  check(
      later_ac_resolved_to_de && session.source_cursor_exhausted() &&
          session.complete(),
      "the later AC arm is already carried by DE through the resident positive cache and all certified normalized commits make complete() true");

  if (ac_token == session.plan().facet_tokens.end() ||
      de_token == session.plan().facet_tokens.end()) {
    return;
  }
  constexpr std::uint64_t helper_authority_id = 7033U;
  auto helper_locator = build_exact_direct_sparse_positive_facet_locator(
      session.plan().facet_tokens.size(),
      generous_session_budget().locator,
      {helper_authority_id, ~std::uint64_t{0U}});
  const std::array<ExactDirectSparseFacetBinding, 1U> de_binding{{
      {0U,
       de_token->facet_key,
       de_token->facet_token_index,
       {helper_authority_id, 1U}},
  }};
  const auto seeded = helper_locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      de_binding);
  const auto helper_stamp = helper_locator.snapshot_stamp();
  const std::array<ExactDirectSparseFacetKey, 1U> ac_seed{{
      ac_token->facet_key,
  }};
  const auto closure =
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          context.index,
          context.cloud,
          ac_seed,
          level(33, 2),
          {helper_authority_id, 2U},
          helper_locator,
          generous_strict_facet_closure_budget(),
          {},
          LbvhTraversalOrder::near_first);
  auto cap_minus_one = generous_strict_facet_closure_budget();
  cap_minus_one.maximum_seed_count = 0U;
  const auto rejected_closure =
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          context.index,
          context.cloud,
          ac_seed,
          level(33, 2),
          {helper_authority_id, 3U},
          helper_locator,
          cap_minus_one,
          {},
          LbvhTraversalOrder::near_first);
  check(
      seeded.certified_committed_batch() &&
          closure.certified_complete_relative_positive_closure() &&
          closure.seed_projections.size() == 1U &&
          closure.nodes[closure.seed_projections[0].terminal_node_index]
                  .resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{
                  de_token->facet_token_index} &&
          rejected_closure.certified_budget_exhaustion() &&
          helper_locator.snapshot_stamp() == helper_stamp,
      "the separate AC-to-DE closure helper resolves one strict seed, while its seed cap minus one rejects without mutating the locator");
}

void test_certified_normalized_terminal_order_rejected() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0),
      point(1.0, 0.0),
      point(0.0, 1.0),
  };
  const auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  const auto index = MortonLbvhIndex::build(cloud);
  const auto source = direct_sources(cloud, cloud.size());
  const auto rank =
      build_exact_direct_rank_window_saturated_h0_authority(source.facade);
  const ExactDirectSparseGatewayCandidateBudget gateway_budget{};
  const ExactDirectSparseGatewayCandidateJournalResult gateway{};
  const ExactDirectNormalizedH0SourcePlanBudget plan_budget{};
  const ExactDirectNormalizedH0SourcePlanResult plan{};
  const auto rejected =
      initialize_exact_direct_normalized_h0_certified_resident_session(
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
          plan_budget,
          plan,
          rank,
          generous_strict_facet_closure_budget(),
          {},
          LbvhTraversalOrder::near_first,
          generous_normalized_adapter_budget(),
          7034U,
          generous_session_budget());
  check(
      source.facade.terminal_catalog_certified() &&
          source.facade.certificate.requirements.effective_maximum_order ==
              cloud.size() &&
          !rejected.session.has_value() &&
          rejected.source_plan_initial_verification_count == 0U &&
          rejected.decision ==
              ExactDirectMorseUnifiedResidentInitializationDecision::
                  no_normalized_terminal_order_equal_point_count_unsupported,
      "the certified initializer rejects K=n before normalized replay or adapter allocation");
}

void test_normalized_candidate_birth_classifier() {
  check(
      classify_exact_direct_normalized_h0_candidate_facet_birth(
          level(1), level(2)) ==
          ExactDirectNormalizedH0CandidateFacetDisposition::
              fail_open_strictly_earlier_without_retraction_authority &&
          classify_exact_direct_normalized_h0_candidate_facet_birth(
              level(2), level(2)) ==
              ExactDirectNormalizedH0CandidateFacetDisposition::
                  equal_at_active_level &&
          classify_exact_direct_normalized_h0_candidate_facet_birth(
              level(3), level(2)) ==
              ExactDirectNormalizedH0CandidateFacetDisposition::
                  contradiction_strictly_later_than_active_level,
      "a strict birth<level is classified as candidate fail-open until an H0 retraction capability exists");
}

}  // namespace

int main() {
  const E5Context context = e5_context();
  check(
      context.plan.certified_bounded_plan(),
      "the E5 unified source plan is certified");
  test_ticket_guards(context);
  test_cap_minus_one(context);
  test_locator_rejection_rolls_back_staged_delta(context);
  test_e5_live_twelve_batches(context);
  test_group_delta_point_caps(context);
  test_normalized_candidate_birth_classifier();
  test_normalized_source_resident_atomic_fold(context);
  test_certified_normalized_strict_carrier_fold();
  test_certified_normalized_terminal_order_rejected();
  if (failures != 0) {
    std::cerr << failures << " resident-session test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse unified resident session tests passed\n";
  return 0;
}
