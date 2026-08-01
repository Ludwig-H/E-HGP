#include "morsehgp3d/hierarchy/direct_morse_resident_at_least20_adapter.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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

template <class Session>
concept PublicResidentViewPreparationCallable = requires(
    Session& session,
    ExactDirectAtLeast20CommittedBatchInput&& input) {
  session.prepare_resident_batch(std::move(input));
};

template <class Session>
concept PublicPreparedViewCommitCallable = requires(
    Session& session,
    ExactDirectAtLeast20StreamViewPreparedBatch&& ticket) {
  session.commit_prepared_batch(std::move(ticket), true);
};

static_assert(direct_morse_resident_at_least20_adapter_schema_version == 1U);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch>);
static_assert(
    !std::is_constructible_v<
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch,
        ExactDirectAtLeast20CommittedBatch&&>);
static_assert(
    !std::is_constructible_v<
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch,
        ExactDirectAtLeast20CommittedBatchInput&&>);
static_assert(
    !PublicResidentViewPreparationCallable<
        ExactDirectAtLeast20StreamViewSession>);
static_assert(
    !PublicPreparedViewCommitCallable<
        ExactDirectAtLeast20StreamViewSession>);

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
generous_resident_budget() {
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

[[nodiscard]] ExactDirectAtLeast20StreamViewBudget generous_view_budget() {
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

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(const CanonicalPointCloud& cloud) {
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
initialize_resident(const E5Context& context, std::uint64_t authority_id) {
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
      generous_resident_budget());
}

[[nodiscard]] ExactDirectMorseResidentAtLeast20AdapterInitializationResult
initialize_adapter(
    const E5Context& context,
    std::uint64_t authority_id,
    const ExactDirectAtLeast20StreamViewBudget& view_budget) {
  auto resident = initialize_resident(context, authority_id);
  if (!resident.certified_initialized_session() ||
      !resident.session.has_value()) {
    return {};
  }
  return initialize_exact_direct_morse_resident_at_least20_adapter(
      std::move(resident.session.value()), view_budget);
}

[[nodiscard]] bool verify_root_mirror_suffix(
    std::span<const ExactDirectMorseUnifiedResidentGroupRecord> records,
    ExactDirectAtLeast20RootId before,
    ExactDirectAtLeast20RootId expected_after,
    bool& saw_continuation,
    bool& saw_reduced_birth,
    bool& saw_multifusion) {
  ExactDirectAtLeast20RootId mirrored = before;
  for (const auto& record : records) {
    if (record.q_r == 1U) {
      saw_continuation = true;
      if (record.child_root_ids.size() != 1U ||
          record.resultant_root_id != record.child_root_ids.front()) {
        return false;
      }
      continue;
    }
    saw_reduced_birth = saw_reduced_birth || record.q_r == 0U;
    saw_multifusion = saw_multifusion || record.q_r >= 2U;
    if (mirrored == 0U || record.resultant_root_id != mirrored) {
      return false;
    }
    mirrored = mirrored ==
            std::numeric_limits<ExactDirectAtLeast20RootId>::max()
        ? 0U
        : mirrored + 1U;
  }
  return mirrored == expected_after;
}

void test_public_transcript_cannot_mint_capability() {
  const auto budget = generous_view_budget();
  morsehgp3d::contract::CanonicalSha256Builder science_builder;
  science_builder.update("forged-public-relative-transcript");
  const auto science = science_builder.finalize();
  ExactDirectAtLeast20CommittedBatchInput input;
  input.source_session_authority_id = 901U;
  input.source_batch_cursor_after = 1U;
  input.source_epoch_after = 1U;
  input.order = 2U;
  input.squared_level = {morsehgp3d::exact::BigInt{1},
                         morsehgp3d::exact::BigInt{1}};
  input.source_scientific_digest = science;
  input.groups.push_back(
      {0U,
       ExactDirectAtLeast20SourceAction::reduced_birth,
       0U,
       1U,
       {},
       {0U}});
  input.complete_equal_level_batch_committed = true;
  input.source_actions_qr_root_successors_and_coverage_delta_certified = true;
  auto sealed =
      seal_exact_direct_at_least20_committed_batch_relative_transcript(
          std::move(input), budget);
  auto view = initialize_exact_direct_at_least20_stream_view_session(
      901U, 0U, 0U, science, {}, budget);
  check(
      sealed.certified_relative_transcript() &&
          view.certified_initialized_session(),
      "the public test transcript is valid only as a relative premise");
  if (!sealed.batch.has_value() || !view.session.has_value()) {
    return;
  }
  const auto relative =
      view.session->consume(std::move(sealed.batch.value()));
  check(
      relative.certified_atomic_downstream_batch_fold() &&
          !relative.upstream_source_commit_capability_verified,
      "a public transcript was incorrectly promoted to resident capability");
}

void test_atomic_resident_view_chain_and_sibling_staleness(
    const E5Context& context) {
  auto initialized = initialize_adapter(context, 9101U, generous_view_budget());
  check(
      initialized.certified_initialized_adapter() &&
          initialized.adapter.has_value(),
      "the fresh E5 resident did not initialize the atomic adapter");
  if (!initialized.adapter.has_value()) {
    return;
  }
  auto& adapter = initialized.adapter.value();
  check(
      adapter.next_root_id() == 1U && adapter.batch_cursor() == 0U &&
          adapter.epoch() == 0U && adapter.view().root_state_count() == 0U,
      "the adapter did not mirror the resident genesis exactly");

  auto winner = adapter.prepare_next();
  auto stale_sibling = adapter.prepare_next();
  check(
      winner.certified_prepared_batch() &&
          stale_sibling.certified_prepared_batch() &&
          winner.ticket.has_value() && stale_sibling.ticket.has_value(),
      "two bounded sibling tickets were not prepared from one snapshot");
  if (!winner.ticket.has_value() || !stale_sibling.ticket.has_value()) {
    return;
  }
  const auto sibling_root_before =
      stale_sibling.ticket->next_root_id_before();
  const auto sibling_root_after = stale_sibling.ticket->next_root_id_after();
  check(
      winner.ticket->next_root_id_before() == sibling_root_before &&
          winner.ticket->next_root_id_after() == sibling_root_after,
      "sibling tickets did not derive the same canonical root-ID suffix");

  bool saw_continuation = false;
  bool saw_reduced_birth = false;
  bool saw_multifusion = false;
  std::size_t records_before = adapter.resident().group_records().size();
  const auto first_source_digest_before = adapter.view().source_commit_digest();
  const auto first_view_digest_before = adapter.view().view_digest();
  const auto first_commit = adapter.commit(std::move(winner.ticket.value()));
  check(
      first_commit.certified_atomic_resident_and_view_commit() &&
          first_commit.view_commit.upstream_source_commit_capability_verified &&
          adapter.view().source_commit_digest() != first_source_digest_before &&
          adapter.view().view_digest() != first_view_digest_before,
      "the winning resident/view transaction did not advance both chains");
  const auto first_records = std::span{
      adapter.resident().group_records()}.subspan(records_before);
  check(
      verify_root_mirror_suffix(
          first_records,
          sibling_root_before,
          sibling_root_after,
          saw_continuation,
          saw_reduced_birth,
          saw_multifusion) &&
          adapter.next_root_id() == sibling_root_after,
      "the winning ticket did not commit the exact resident root-ID rule");

  const auto cursor_after_winner = adapter.batch_cursor();
  const auto root_after_winner = adapter.next_root_id();
  const auto source_after_winner = adapter.view().source_commit_digest();
  const auto view_after_winner = adapter.view().view_digest();
  const auto groups_after_winner = adapter.resident().group_records().size();
  const auto rejected =
      adapter.commit(std::move(stale_sibling.ticket.value()));
  check(
      rejected.decision ==
              ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
                  no_resident_commit_rejected &&
          rejected.resident_commit.decision ==
              ExactDirectMorseUnifiedResidentCommitDecision::
                  no_stale_ticket_rejected &&
          rejected.no_scientific_state_mutated_on_failure &&
          adapter.batch_cursor() == cursor_after_winner &&
          adapter.next_root_id() == root_after_winner &&
          adapter.view().source_commit_digest() == source_after_winner &&
          adapter.view().view_digest() == view_after_winner &&
          adapter.resident().group_records().size() == groups_after_winner,
      "the losing sibling advanced resident, view or root-ID mirror state");

  while (!adapter.complete()) {
    auto prepared = adapter.prepare_next();
    check(
        prepared.certified_prepared_batch() && prepared.ticket.has_value(),
        "an E5 resident batch did not prepare through the adapter");
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto root_before = prepared.ticket->next_root_id_before();
    const auto root_after = prepared.ticket->next_root_id_after();
    records_before = adapter.resident().group_records().size();
    const auto source_before = adapter.view().source_commit_digest();
    const auto view_before = adapter.view().view_digest();
    const auto committed =
        adapter.commit(std::move(prepared.ticket.value()));
    check(
        committed.certified_atomic_resident_and_view_commit() &&
            committed.view_commit.upstream_source_commit_capability_verified &&
            adapter.view().source_commit_digest() != source_before &&
            adapter.view().view_digest() != view_before,
        "an E5 adapter commit did not preserve the authenticated digest chain");
    const auto suffix = std::span{
        adapter.resident().group_records()}.subspan(records_before);
    check(
        verify_root_mirror_suffix(
            suffix,
            root_before,
            root_after,
            saw_continuation,
            saw_reduced_birth,
            saw_multifusion) &&
            adapter.next_root_id() == root_after,
        "q_R=1 retention or q_R!=1 sequential allocation diverged");
  }

  check(
      adapter.complete() && adapter.certified_adapter_session() &&
          adapter.batch_cursor() == context.plan.batches.size() &&
          adapter.batch_cursor() == adapter.view().next_source_batch_cursor() &&
          adapter.epoch() == adapter.view().source_epoch() &&
          adapter.resident().group_records().size() == 7U &&
          saw_continuation && saw_reduced_birth && saw_multifusion,
      "the complete E5 chain did not observe q_R=0, q_R=1 and q_R>=2 root rules");
}

void test_non_genesis_resident_is_not_a_durable_restore(
    const E5Context& context) {
  auto initialized = initialize_resident(context, 9151U);
  check(
      initialized.certified_initialized_session() &&
          initialized.session.has_value(),
      "the non-genesis control resident did not initialize");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& resident = initialized.session.value();
  auto prepared = resident.prepare_next();
  if (!prepared.ticket.has_value()) {
    check(false, "the non-genesis control batch did not prepare");
    return;
  }
  const auto committed = resident.commit(std::move(prepared.ticket.value()));
  check(
      committed.certified_committed_batch(),
      "the non-genesis control batch did not commit");
  auto rejected =
      initialize_exact_direct_morse_resident_at_least20_adapter(
          std::move(resident), generous_view_budget());
  check(
      rejected.decision ==
              ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
                  no_non_genesis_resident_session_rejected &&
          !rejected.adapter.has_value(),
      "a non-genesis resident was accepted as an invented durable view prefix");
}

void test_view_cap_minus_one_rejects_before_resident_commit(
    const E5Context& context) {
  auto baseline_init =
      initialize_adapter(context, 9201U, generous_view_budget());
  check(
      baseline_init.certified_initialized_adapter() &&
          baseline_init.adapter.has_value(),
      "the cap baseline adapter did not initialize");
  if (!baseline_init.adapter.has_value()) {
    return;
  }
  auto& baseline = baseline_init.adapter.value();
  std::size_t target_cursor = 0U;
  std::size_t required_staging = 0U;
  while (!baseline.complete() && required_staging == 0U) {
    target_cursor = baseline.batch_cursor();
    auto prepared = baseline.prepare_next();
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto committed =
        baseline.commit(std::move(prepared.ticket.value()));
    if (!committed.certified_atomic_resident_and_view_commit()) {
      break;
    }
    required_staging =
        committed.view_commit.counters.required_staging_entry_count;
  }
  check(
      required_staging > 1U,
      "no nonempty E5 view batch was available for cap-minus-one");
  if (required_staging <= 1U) {
    return;
  }

  auto limited_budget = generous_view_budget();
  limited_budget.maximum_staging_entry_count = required_staging - 1U;
  auto limited_init = initialize_adapter(context, 9202U, limited_budget);
  check(
      limited_init.certified_initialized_adapter() &&
          limited_init.adapter.has_value(),
      "the cap-minus-one adapter did not initialize");
  if (!limited_init.adapter.has_value()) {
    return;
  }
  auto& limited = limited_init.adapter.value();
  while (limited.batch_cursor() < target_cursor) {
    auto prepared = limited.prepare_next();
    if (!prepared.ticket.has_value()) {
      break;
    }
    const auto committed =
        limited.commit(std::move(prepared.ticket.value()));
    if (!committed.certified_atomic_resident_and_view_commit()) {
      break;
    }
  }
  check(
      limited.batch_cursor() == target_cursor,
      "the cap-minus-one replay did not reach its target batch");
  const auto cursor_before = limited.batch_cursor();
  const auto epoch_before = limited.epoch();
  const auto root_before = limited.next_root_id();
  const auto groups_before = limited.resident().group_records().size();
  const auto source_before = limited.view().source_commit_digest();
  const auto view_before = limited.view().view_digest();
  const auto rejected = limited.prepare_next();
  check(
      rejected.decision ==
              ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
                  no_view_preparation_rejected &&
          rejected.view_decision ==
              ExactDirectAtLeast20StreamConsumeDecision::
                  no_staging_budget_exhausted &&
          rejected.no_scientific_state_mutated &&
          !rejected.ticket.has_value() &&
          limited.batch_cursor() == cursor_before &&
          limited.epoch() == epoch_before &&
          limited.next_root_id() == root_before &&
          limited.resident().group_records().size() == groups_before &&
          limited.view().source_commit_digest() == source_before &&
          limited.view().view_digest() == view_before,
      "cap-minus-one did not reject before the resident transaction");
}

}  // namespace

int main() {
  test_public_transcript_cannot_mint_capability();
  const E5Context context = e5_context();
  test_atomic_resident_view_chain_and_sibling_staleness(context);
  test_non_genesis_resident_is_not_a_durable_restore(context);
  test_view_cap_minus_one_rejects_before_resident_commit(context);
  if (failures != 0) {
    std::cerr << failures << " failure(s)\n";
    return 1;
  }
  std::cout
      << "direct resident -> at-least20 atomic adapter checks passed\n";
  return 0;
}
