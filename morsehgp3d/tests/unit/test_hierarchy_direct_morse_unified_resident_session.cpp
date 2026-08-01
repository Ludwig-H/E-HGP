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
    const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
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

struct E5Context {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget star_budget;
  ExactDirectSparseSuccessiveIncidenceStarJournalResult star;
  ExactDirectSparseUnifiedLevelPlanBudget plan_budget;
  ExactDirectSparseUnifiedLevelPlanResult plan;
};

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
              "bounded_sparse_resident_delta_single_batch_construction_v4",
      "the session advertises its single-construction attestation v4 scope");

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
  if (failures != 0) {
    std::cerr << failures << " resident-session test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse unified resident session tests passed\n";
  return 0;
}
