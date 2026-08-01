#include "morsehgp3d/hierarchy/direct_at_least20_stream_view.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactDirectAtLeast20CommittedBatch;
using morsehgp3d::hierarchy::ExactDirectAtLeast20CommittedBatchInput;
using morsehgp3d::hierarchy::ExactDirectAtLeast20CommittedGroupInput;
using morsehgp3d::hierarchy::ExactDirectAtLeast20SourceAction;
using morsehgp3d::hierarchy::ExactDirectAtLeast20StreamConsumeDecision;
using morsehgp3d::hierarchy::ExactDirectAtLeast20StreamViewBudget;
using morsehgp3d::hierarchy::ExactDirectAtLeast20StreamViewSession;
using morsehgp3d::hierarchy::ExactDirectAtLeast20VisibleTransitionKind;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<ExactDirectAtLeast20CommittedBatch>);
static_assert(!std::is_copy_assignable_v<ExactDirectAtLeast20CommittedBatch>);
static_assert(
    !std::is_copy_constructible_v<ExactDirectAtLeast20StreamViewSession>);
static_assert(!std::is_copy_assignable_v<ExactDirectAtLeast20StreamViewSession>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}};
}

[[nodiscard]] CanonicalId scientific_source(std::uint8_t tag = 0xA5U) {
  std::array<std::uint8_t, CanonicalId::byte_count> bytes{};
  bytes.front() = tag;
  return CanonicalId{bytes};
}

[[nodiscard]] std::vector<PointId> point_range(
    PointId first,
    std::size_t count) {
  std::vector<PointId> result;
  result.reserve(count);
  for (std::size_t offset = 0U; offset < count; ++offset) {
    result.push_back(first + static_cast<PointId>(offset));
  }
  return result;
}

[[nodiscard]] ExactDirectAtLeast20StreamViewBudget generous_budget() {
  return ExactDirectAtLeast20StreamViewBudget{
      .maximum_batch_group_count = 16U,
      .maximum_batch_prior_root_reference_count = 64U,
      .maximum_batch_coverage_delta_point_reference_count = 512U,
      .maximum_batch_visible_transition_count = 16U,
      .maximum_batch_visible_child_reference_count = 64U,
      .maximum_root_state_count = 128U,
      .maximum_active_root_count = 64U,
      .maximum_retained_point_reference_count = 2560U,
      .maximum_staging_entry_count = 1024U,
      .maximum_checkpoint_root_state_count = 128U,
      .maximum_checkpoint_retained_point_reference_count = 2560U,
  };
}

[[nodiscard]] ExactDirectAtLeast20CommittedGroupInput birth_group(
    std::size_t source_group_index,
    std::uint64_t root_id,
    std::vector<PointId> delta) {
  return ExactDirectAtLeast20CommittedGroupInput{
      .source_group_index = source_group_index,
      .source_action = ExactDirectAtLeast20SourceAction::reduced_birth,
      .q_r = 0U,
      .resultant_root_id = root_id,
      .prior_root_ids = {},
      .coverage_delta_point_ids = std::move(delta),
  };
}

[[nodiscard]] ExactDirectAtLeast20CommittedGroupInput continuation_group(
    std::size_t source_group_index,
    std::uint64_t root_id,
    std::vector<PointId> delta) {
  return ExactDirectAtLeast20CommittedGroupInput{
      .source_group_index = source_group_index,
      .source_action = ExactDirectAtLeast20SourceAction::continuation,
      .q_r = 1U,
      .resultant_root_id = root_id,
      .prior_root_ids = {root_id},
      .coverage_delta_point_ids = std::move(delta),
  };
}

[[nodiscard]] ExactDirectAtLeast20CommittedBatchInput committed_input(
    std::uint64_t authority_id,
    std::size_t cursor,
    std::size_t epoch,
    const CanonicalId& previous_digest,
    std::int64_t squared_level,
    std::vector<ExactDirectAtLeast20CommittedGroupInput> groups) {
  return ExactDirectAtLeast20CommittedBatchInput{
      .source_session_authority_id = authority_id,
      .source_batch_cursor_before = cursor,
      .source_batch_cursor_after = cursor + 1U,
      .source_epoch_before = epoch,
      .source_epoch_after = epoch + 1U,
      .order = 1U,
      .squared_level = level(squared_level),
      .source_scientific_digest = scientific_source(),
      .previous_source_commit_digest = previous_digest,
      .groups = std::move(groups),
      .complete_equal_level_batch_committed = true,
      .source_actions_qr_root_successors_and_coverage_delta_certified = true,
      .source_pruned_by_min_cluster_size = false,
      .source_public_status_claimed = false,
  };
}

[[nodiscard]] ExactDirectAtLeast20CommittedBatch seal(
    ExactDirectAtLeast20CommittedBatchInput input,
    const ExactDirectAtLeast20StreamViewBudget& budget) {
  auto result = morsehgp3d::hierarchy::
      seal_exact_direct_at_least20_committed_batch_relative_transcript(
          std::move(input), budget);
  if (!result.certified_relative_transcript() || !result.batch.has_value()) {
    throw std::runtime_error("test fixture could not seal a committed batch");
  }
  return std::move(result.batch.value());
}

[[nodiscard]] ExactDirectAtLeast20StreamViewSession initialize(
    std::uint64_t authority_id,
    const ExactDirectAtLeast20StreamViewBudget& budget) {
  auto result = morsehgp3d::hierarchy::
      initialize_exact_direct_at_least20_stream_view_session(
          authority_id,
          0U,
          0U,
          scientific_source(),
          CanonicalId{},
          budget);
  if (!result.certified_initialized_session() || !result.session.has_value()) {
    throw std::runtime_error("test fixture could not initialize a session");
  }
  check(
      !result.source_pruning_enabled &&
          !result.upstream_resident_adapter_certified,
      "the autonomous downstream fixture acquired upstream authority");
  return std::move(result.session.value());
}

void test_threshold_nineteen_to_twenty_and_checkpoint_restore() {
  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  ExactDirectAtLeast20StreamViewSession session = initialize(101U, budget);

  std::vector<PointId> nineteen = point_range(0U, 19U);
  nineteen.insert(nineteen.begin(), 18U);
  std::reverse(nineteen.begin(), nineteen.end());
  auto first = seal(
      committed_input(
          101U, 0U, 0U, CanonicalId{}, 1,
          {birth_group(9U, 7U, std::move(nineteen))}),
      budget);
  const auto hidden = session.consume(std::move(first));
  check(
      hidden.certified_atomic_downstream_batch_fold() &&
          hidden.visible_transitions.empty() &&
          hidden.counters.hidden_result_count == 1U &&
          hidden.counters.retained_point_reference_count_after == 19U,
      "nineteen distinct PointIds were not retained as one hidden root");

  auto second = seal(
      committed_input(
          101U,
          session.next_source_batch_cursor(),
          session.source_epoch(),
          session.source_commit_digest(),
          2,
          {continuation_group(3U, 7U, {19U, 18U, 19U})}),
      budget);
  const auto crossing = session.consume(std::move(second));
  check(
      crossing.certified_atomic_downstream_batch_fold() &&
          crossing.visible_transitions.size() == 1U &&
          crossing.counters.threshold_entry_count == 1U,
      "the complete 19-to-20 equality batch did not emit one threshold entry");
  if (!crossing.visible_transitions.empty()) {
    const auto& transition = crossing.visible_transitions.front();
    check(
        transition.kind ==
                ExactDirectAtLeast20VisibleTransitionKind::threshold_entry &&
            transition.source_action ==
                ExactDirectAtLeast20SourceAction::continuation &&
            transition.source_q_r == 1U && transition.q_visible == 0U &&
            transition.retained_smallest_point_ids == point_range(0U, 20U),
        "the threshold entry lost source q_R or canonical cap-20 coverage");
  }

  auto checkpoint_result = session.make_checkpoint();
  check(
      checkpoint_result.certified_checkpoint() &&
          checkpoint_result.checkpoint.has_value(),
      "the threshold crossing did not produce a certified checkpoint");
  if (!checkpoint_result.checkpoint.has_value()) {
    return;
  }
  const CanonicalId expected_source_digest = session.source_commit_digest();
  const CanonicalId expected_view_digest = session.view_digest();
  constexpr std::uint64_t reminted_authority = 1001U;
  auto restore_result = morsehgp3d::hierarchy::
      restore_exact_direct_at_least20_stream_view_session(
          std::move(checkpoint_result.checkpoint.value()),
          reminted_authority,
          budget);
  check(
      restore_result.certified_initialized_session() &&
          restore_result.checkpoint_freshly_verified &&
          restore_result.session.has_value(),
      "the cap-20 checkpoint did not restore through fresh verification");
  if (restore_result.session.has_value()) {
    check(
        restore_result.session->source_commit_digest() ==
                expected_source_digest &&
            restore_result.session->view_digest() == expected_view_digest &&
            restore_result.session->source_session_authority_id() ==
                reminted_authority &&
            restore_result.session->source_scientific_digest() ==
                scientific_source() &&
            restore_result.session->active_root_count() == 1U &&
            restore_result.session->retained_point_reference_count() == 20U,
        "checkpoint remint changed a scientific digest or cap summary");

    auto reminted_batch = seal(
        committed_input(
            reminted_authority,
            restore_result.session->next_source_batch_cursor(),
            restore_result.session->source_epoch(),
            restore_result.session->source_commit_digest(),
            3,
            {continuation_group(4U, 7U, {19U, 19U})}),
        budget);
    check(
        restore_result.session->consume(std::move(reminted_batch))
            .certified_atomic_downstream_batch_fold(),
        "the freshly reminted authority could not continue the exact chain");
  }
}

void test_intra_batch_permutation_is_canonical() {
  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  ExactDirectAtLeast20StreamViewSession left = initialize(202U, budget);
  ExactDirectAtLeast20StreamViewSession right = initialize(212U, budget);
  check(
      left.view_digest() == right.view_digest(),
      "fresh operational authorities changed the scientific view anchor");

  const auto low = birth_group(2U, 11U, point_range(0U, 19U));
  auto high_points = point_range(30U, 19U);
  std::reverse(high_points.begin(), high_points.end());
  const auto high = birth_group(8U, 22U, std::move(high_points));
  auto left_batch = seal(
      committed_input(
          202U, 0U, 0U, CanonicalId{}, 1, {high, low}),
      budget);
  auto right_batch = seal(
      committed_input(
          212U, 0U, 0U, CanonicalId{}, 1, {low, high}),
      budget);
  check(
      left_batch.source_commit_digest() ==
          right_batch.source_commit_digest(),
      "authority or intra-batch permutation changed the source digest");

  const auto left_result = left.consume(std::move(left_batch));
  const auto right_result = right.consume(std::move(right_batch));
  check(
      left_result.certified_atomic_downstream_batch_fold() &&
          right_result.certified_atomic_downstream_batch_fold() &&
          left.view_digest() == right.view_digest(),
      "authority or intra-batch permutation changed the view digest");
  const auto left_checkpoint = left.make_checkpoint();
  const auto right_checkpoint = right.make_checkpoint();
  check(
      left_checkpoint.certified_checkpoint() &&
          right_checkpoint.certified_checkpoint() &&
          left_checkpoint.checkpoint.has_value() &&
          right_checkpoint.checkpoint.has_value(),
      "canonical sessions did not produce checkpoints");
  if (left_checkpoint.checkpoint.has_value() &&
      right_checkpoint.checkpoint.has_value()) {
    const auto& left_value = left_checkpoint.checkpoint.value();
    const auto& right_value = right_checkpoint.checkpoint.value();
    check(
        left_value.source_session_authority_id !=
                right_value.source_session_authority_id &&
            left_value.source_scientific_digest ==
                right_value.source_scientific_digest &&
            left_value.source_commit_digest ==
                right_value.source_commit_digest &&
            left_value.view_digest == right_value.view_digest &&
            left_value.roots == right_value.roots &&
            left_value.order_cursors == right_value.order_cursors,
        "operational authority leaked into checkpoint scientific state");
  }
}

void test_multifusion_with_overlapping_visible_roots() {
  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  ExactDirectAtLeast20StreamViewSession session = initialize(303U, budget);
  auto roots = seal(
      committed_input(
          303U,
          0U,
          0U,
          CanonicalId{},
          1,
          {birth_group(4U, 11U, point_range(0U, 20U)),
           birth_group(7U, 22U, point_range(10U, 20U))}),
      budget);
  const auto roots_result = session.consume(std::move(roots));
  check(
      roots_result.certified_atomic_downstream_batch_fold() &&
          roots_result.counters.threshold_entry_count == 2U &&
          roots_result.visible_transitions.size() == 2U,
      "two exact visible births with overlapping coverage did not commit");
  if (roots_result.visible_transitions.size() == 2U) {
    check(
        roots_result.visible_transitions[0U].retained_smallest_point_ids ==
                point_range(0U, 20U) &&
            roots_result.visible_transitions[1U]
                    .retained_smallest_point_ids ==
                point_range(10U, 20U),
        "legitimate inter-group PointId overlap was rejected or conflated");
  }

  ExactDirectAtLeast20CommittedGroupInput fusion{
      .source_group_index = 5U,
      .source_action = ExactDirectAtLeast20SourceAction::multifusion,
      .q_r = 2U,
      .resultant_root_id = 33U,
      .prior_root_ids = {22U, 11U},
      .coverage_delta_point_ids = {29U, 5U, 29U},
  };
  auto fusion_batch = seal(
      committed_input(
          303U,
          session.next_source_batch_cursor(),
          session.source_epoch(),
          session.source_commit_digest(),
          2,
          {std::move(fusion)}),
      budget);
  const auto fusion_result = session.consume(std::move(fusion_batch));
  check(
      fusion_result.certified_atomic_downstream_batch_fold() &&
          fusion_result.visible_transitions.size() == 1U &&
          fusion_result.counters.multifusion_count == 1U &&
          fusion_result.counters.active_root_count_after == 1U &&
          fusion_result.counters.retained_point_reference_count_after == 20U,
      "the overlapping two-root multifusion did not produce one cap-20 root");
  if (!fusion_result.visible_transitions.empty()) {
    const auto& transition = fusion_result.visible_transitions.front();
    check(
        transition.kind ==
                ExactDirectAtLeast20VisibleTransitionKind::multifusion &&
            transition.source_q_r == 2U && transition.q_visible == 2U &&
            transition.prior_visible_root_ids ==
                std::vector<std::uint64_t>{11U, 22U} &&
            transition.retained_smallest_point_ids == point_range(0U, 20U),
        "the visible multifusion lost deduplication or canonical child order");
  }
}

void test_foreign_stale_and_unknown_roots_reject_atomically() {
  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  ExactDirectAtLeast20StreamViewSession session = initialize(404U, budget);
  auto seed = seal(
      committed_input(
          404U, 0U, 0U, CanonicalId{}, 1,
          {birth_group(0U, 1U, point_range(0U, 19U))}),
      budget);
  check(
      session.consume(std::move(seed)).certified_atomic_downstream_batch_fold(),
      "atomic-rejection fixture seed did not commit");

  const CanonicalId source_before = session.source_commit_digest();
  const CanonicalId view_before = session.view_digest();
  const std::size_t cursor_before = session.next_source_batch_cursor();
  const std::size_t roots_before = session.root_state_count();

  auto foreign = seal(
      committed_input(
          999U,
          session.next_source_batch_cursor(),
          session.source_epoch(),
          session.source_commit_digest(),
          2,
          {birth_group(1U, 2U, {100U})}),
      budget);
  const auto foreign_result = session.consume(std::move(foreign));
  check(
      foreign_result.decision == ExactDirectAtLeast20StreamConsumeDecision::
          no_foreign_source_session_rejected,
      "a foreign authority was not rejected");
  check(
      session.source_commit_digest() == source_before &&
          session.view_digest() == view_before &&
          session.next_source_batch_cursor() == cursor_before &&
          session.root_state_count() == roots_before,
      "foreign-batch rejection mutated scientific state");

  auto foreign_science_input = committed_input(
      404U,
      session.next_source_batch_cursor(),
      session.source_epoch(),
      session.source_commit_digest(),
      2,
      {birth_group(2U, 3U, {101U})});
  foreign_science_input.source_scientific_digest = scientific_source(0xB6U);
  auto foreign_science = seal(std::move(foreign_science_input), budget);
  const auto foreign_science_result =
      session.consume(std::move(foreign_science));
  check(
      foreign_science_result.decision ==
              ExactDirectAtLeast20StreamConsumeDecision::
                  no_foreign_source_science_rejected &&
          session.source_commit_digest() == source_before &&
          session.view_digest() == view_before &&
          session.next_source_batch_cursor() == cursor_before,
      "a different scientific source was accepted or mutated the view");

  auto stale = seal(
      committed_input(
          404U, 0U, 0U, CanonicalId{}, 2,
          {birth_group(2U, 3U, {101U})}),
      budget);
  const auto stale_result = session.consume(std::move(stale));
  check(
      stale_result.decision == ExactDirectAtLeast20StreamConsumeDecision::
          no_stale_source_stamp_rejected &&
          session.source_commit_digest() == source_before &&
          session.view_digest() == view_before,
      "a stale stamp was accepted or mutated the view");

  auto unknown = seal(
      committed_input(
          404U,
          session.next_source_batch_cursor(),
          session.source_epoch(),
          session.source_commit_digest(),
          2,
          {continuation_group(3U, 77U, {19U})}),
      budget);
  const auto unknown_result = session.consume(std::move(unknown));
  check(
      unknown_result.decision == ExactDirectAtLeast20StreamConsumeDecision::
          no_unknown_or_inactive_prior_root_rejected &&
          !unknown_result.scientific_state_mutated_on_failure &&
          session.source_commit_digest() == source_before &&
          session.view_digest() == view_before &&
          session.next_source_batch_cursor() == cursor_before &&
          session.root_state_count() == roots_before,
      "unknown-root rejection was not atomic");
}

void test_global_exact_level_then_order_chronology() {
  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  ExactDirectAtLeast20StreamViewSession session = initialize(606U, budget);
  auto order_two_input = committed_input(
      606U,
      0U,
      0U,
      CanonicalId{},
      1,
      {birth_group(0U, 1U, {0U})});
  order_two_input.order = 2U;
  auto order_two = seal(std::move(order_two_input), budget);
  check(
      session.consume(std::move(order_two))
          .certified_atomic_downstream_batch_fold(),
      "the first exact (level, order) batch did not commit");

  const CanonicalId source_before = session.source_commit_digest();
  const CanonicalId view_before = session.view_digest();
  auto reversed_input = committed_input(
      606U,
      session.next_source_batch_cursor(),
      session.source_epoch(),
      session.source_commit_digest(),
      1,
      {birth_group(1U, 2U, {1U})});
  reversed_input.order = 1U;
  auto reversed = seal(std::move(reversed_input), budget);
  const auto reversed_result = session.consume(std::move(reversed));
  check(
      reversed_result.decision ==
              ExactDirectAtLeast20StreamConsumeDecision::
                  no_nonincreasing_order_level_rejected &&
          !reversed_result.scientific_state_mutated_on_failure &&
          reversed.valid_relative_transcript() &&
          session.source_commit_digest() == source_before &&
          session.view_digest() == view_before &&
          session.next_source_batch_cursor() == 1U &&
          session.root_state_count() == 1U,
      "global (exact level, order) reversal was not rejected atomically");
}

void test_staging_cap_minus_one_rejects_before_state_mutation() {
  ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  // One group + twenty staged PointIds + one visible transition = 22.
  budget.maximum_staging_entry_count = 21U;
  ExactDirectAtLeast20StreamViewSession session = initialize(505U, budget);
  auto batch = seal(
      committed_input(
          505U, 0U, 0U, CanonicalId{}, 1,
          {birth_group(0U, 1U, point_range(0U, 20U))}),
      budget);
  const CanonicalId source_before = session.source_commit_digest();
  const CanonicalId view_before = session.view_digest();
  const auto result = session.consume(std::move(batch));
  check(
      result.decision == ExactDirectAtLeast20StreamConsumeDecision::
          no_staging_budget_exhausted &&
          !result.scientific_state_mutated_on_failure &&
          session.source_commit_digest() == source_before &&
          session.view_digest() == view_before &&
          session.next_source_batch_cursor() == 0U &&
          session.source_epoch() == 0U && session.root_state_count() == 0U &&
          session.active_root_count() == 0U &&
          session.retained_point_reference_count() == 0U,
      "the exact staging cap-minus-one rejection mutated state");
}

void test_authority_and_public_non_claims() {
  check(
      ExactDirectAtLeast20StreamViewSession::backend == "reference_cpu" &&
          ExactDirectAtLeast20StreamViewSession::profile == "hgp_reduced" &&
          ExactDirectAtLeast20StreamViewSession::min_cluster_size == 20U &&
          ExactDirectAtLeast20StreamViewSession::public_status ==
              "not_claimed" &&
          !ExactDirectAtLeast20StreamViewSession::source_pruning_authority &&
          !ExactDirectAtLeast20StreamViewSession::
              source_hierarchy_authority &&
          !ExactDirectAtLeast20StreamViewSession::
              upstream_resident_adapter_certified &&
          !ExactDirectAtLeast20StreamViewSession::public_exactness_claimed,
      "the isolated stream view exposed source or public authority");

  const ExactDirectAtLeast20StreamViewBudget budget = generous_budget();
  const auto non_genesis = morsehgp3d::hierarchy::
      initialize_exact_direct_at_least20_stream_view_session(
          707U,
          1U,
          1U,
          scientific_source(),
          scientific_source(0xC7U),
          budget);
  check(
      !non_genesis.session.has_value() &&
          non_genesis.decision ==
              morsehgp3d::hierarchy::
                  ExactDirectAtLeast20SessionInitializationDecision::
                      no_session_stamp_rejected,
      "an empty session attached to an unreplayed non-genesis prefix");
}

}  // namespace

int main() {
  try {
    test_authority_and_public_non_claims();
    test_threshold_nineteen_to_twenty_and_checkpoint_restore();
    test_intra_batch_permutation_is_canonical();
    test_multifusion_with_overlapping_visible_roots();
    test_foreign_stale_and_unknown_roots_reject_atomically();
    test_global_exact_level_then_order_chronology();
    test_staging_cap_minus_one_rejects_before_state_mutation();
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected fixture exception: " << error.what()
              << '\n';
  }
  if (failures != 0) {
    std::cerr << failures << " failure(s)\n";
    return 1;
  }
  std::cout << "direct at_least20 stream view tests passed\n";
  return 0;
}
