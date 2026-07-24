#include "morsehgp3d/hierarchy/direct_morse_budget.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

using morsehgp3d::hierarchy::ExactDirectMorseBudgetBoundary;
using morsehgp3d::hierarchy::ExactDirectMorseBudgetDecision;
using morsehgp3d::hierarchy::ExactDirectMorseBudgetDemand;
using morsehgp3d::hierarchy::ExactDirectMorseBudgetRefusalReason;
using morsehgp3d::hierarchy::ExactDirectMorseBudgetSnapshot;
using morsehgp3d::hierarchy::ExactDirectMorseBudgetTracker;
using morsehgp3d::hierarchy::ExactDirectMorseEffectiveBudgetPolicy;
using morsehgp3d::hierarchy::
    verify_exact_direct_morse_budget_snapshot_digest;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_invalid_argument(
    Function&& function,
    const std::string& message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
}

[[nodiscard]] ExactDirectMorseEffectiveBudgetPolicy wide_policy() {
  return ExactDirectMorseEffectiveBudgetPolicy{
      1'000U, 1'000U, 1'000U, 1'000U, 1'000U};
}

// Every axis requires exactly 100 units once elapsed_ns reaches 40.
[[nodiscard]] ExactDirectMorseBudgetDemand hundred_unit_demand() {
  ExactDirectMorseBudgetDemand demand;
  demand.device.used_bytes = 40U;
  demand.device.reserved_bytes = 60U;
  demand.host.used_bytes = 45U;
  demand.host.reserved_bytes = 55U;
  demand.scratch.used_bytes = 10U;
  demand.scratch.temporary_reserved_bytes = 20U;
  demand.scratch.worst_merge_reserved_bytes = 30U;
  demand.scratch.checkpoint_reserved_bytes = 25U;
  demand.scratch.safety_margin_reserved_bytes = 15U;
  demand.output.used_bytes = 25U;
  demand.output.reserved_bytes = 75U;
  demand.time.operation_reserved_ns = 35U;
  demand.time.checkpoint_reserved_ns = 25U;
  return demand;
}

[[nodiscard]] ExactDirectMorseBudgetSnapshot evaluate(
    const ExactDirectMorseEffectiveBudgetPolicy& policy,
    const ExactDirectMorseBudgetDemand& demand,
    std::uint64_t elapsed_ns = 40U,
    ExactDirectMorseBudgetBoundary boundary =
        ExactDirectMorseBudgetBoundary::before_batch,
    std::uint64_t sequence_number = 7U,
    std::uint64_t committed_elapsed_ns = 0U) {
  std::uint64_t clock_ns = 10'000U;
  ExactDirectMorseBudgetTracker tracker{
      policy,
      committed_elapsed_ns,
      [&clock_ns] { return clock_ns; }};
  clock_ns += elapsed_ns;
  return tracker.snapshot(boundary, sequence_number, demand);
}

enum class TestAxis : std::uint8_t {
  device,
  host,
  scratch,
  output,
  time,
};

void set_limit(
    ExactDirectMorseEffectiveBudgetPolicy& policy,
    TestAxis axis,
    std::uint64_t limit) {
  switch (axis) {
    case TestAxis::device:
      policy.device_limit_bytes = limit;
      return;
    case TestAxis::host:
      policy.host_limit_bytes = limit;
      return;
    case TestAxis::scratch:
      policy.scratch_limit_bytes = limit;
      return;
    case TestAxis::output:
      policy.output_limit_bytes = limit;
      return;
    case TestAxis::time:
      policy.time_limit_ns = limit;
      return;
  }
}

[[nodiscard]] ExactDirectMorseBudgetRefusalReason refusal_for(
    TestAxis axis) {
  switch (axis) {
    case TestAxis::device:
      return ExactDirectMorseBudgetRefusalReason::device;
    case TestAxis::host:
      return ExactDirectMorseBudgetRefusalReason::host;
    case TestAxis::scratch:
      return ExactDirectMorseBudgetRefusalReason::scratch;
    case TestAxis::output:
      return ExactDirectMorseBudgetRefusalReason::output;
    case TestAxis::time:
      return ExactDirectMorseBudgetRefusalReason::time;
  }
  return ExactDirectMorseBudgetRefusalReason::none;
}

void test_all_boundaries_and_public_projection_limit() {
  constexpr std::array<ExactDirectMorseBudgetBoundary, 6U> boundaries{
      ExactDirectMorseBudgetBoundary::before_batch,
      ExactDirectMorseBudgetBoundary::before_run,
      ExactDirectMorseBudgetBoundary::before_merge,
      ExactDirectMorseBudgetBoundary::before_serialization,
      ExactDirectMorseBudgetBoundary::checkpoint,
      ExactDirectMorseBudgetBoundary::final};
  std::array<morsehgp3d::contract::CanonicalId, boundaries.size()>
      digests{};
  for (std::size_t index = 0U; index < boundaries.size(); ++index) {
    const ExactDirectMorseBudgetSnapshot snapshot = evaluate(
        wide_policy(),
        hundred_unit_demand(),
        40U,
        boundaries[index],
        static_cast<std::uint64_t>(index));
    digests[index] = snapshot.snapshot_digest;
    check(
        snapshot.accepted() &&
            snapshot.boundary == boundaries[index] &&
            snapshot.accounting_evaluation_complete &&
            snapshot.deterministic_refusal_order_applied &&
            snapshot.scratch_reserve_components_accounted_together,
        "every required Phase-15 boundary produces one complete accepted snapshot");
    check(
        !snapshot.public_v2_projection_claimed &&
            !snapshot.public_v2_time_reserve_representable &&
            snapshot.architecture_only &&
            !snapshot.hierarchy_or_scientific_state_mutated &&
            !snapshot.forbidden_global_structure_materialized &&
            !snapshot.public_status_claimed,
        "the architecture snapshot never claims the lossy public-v2 time-reserve projection or scientific work");
    check(
        verify_exact_direct_morse_budget_snapshot_digest(snapshot),
        "every required boundary has an authenticated canonical digest");
    if (index != 0U) {
      check(
          digests[index] != digests[index - 1U],
          "the canonical digest binds the transactional boundary");
    }
  }
}

void test_each_axis_one_below_exact_and_one_above() {
  constexpr std::array<TestAxis, 5U> axes{
      TestAxis::device,
      TestAxis::host,
      TestAxis::scratch,
      TestAxis::output,
      TestAxis::time};
  const ExactDirectMorseBudgetDemand demand = hundred_unit_demand();
  for (const TestAxis axis : axes) {
    ExactDirectMorseEffectiveBudgetPolicy below = wide_policy();
    set_limit(below, axis, 99U);
    const ExactDirectMorseBudgetSnapshot rejected =
        evaluate(below, demand);
    check(
        !rejected.accepted() &&
            rejected.first_refusal_reason == refusal_for(axis) &&
            rejected.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_limit_exceeded,
        "one unit below the exact need rejects on the selected typed axis");

    ExactDirectMorseEffectiveBudgetPolicy exact = wide_policy();
    set_limit(exact, axis, 100U);
    const ExactDirectMorseBudgetSnapshot accepted_exact =
        evaluate(exact, demand);
    check(
        accepted_exact.accepted(),
        "an exact typed budget boundary is accepted");

    ExactDirectMorseEffectiveBudgetPolicy above = wide_policy();
    set_limit(above, axis, 101U);
    const ExactDirectMorseBudgetSnapshot accepted_above =
        evaluate(above, demand);
    check(
        accepted_above.accepted(),
        "one unit above the exact need is accepted");
  }
}

void test_deterministic_first_refusal_order() {
  constexpr std::array<TestAxis, 5U> axes{
      TestAxis::device,
      TestAxis::host,
      TestAxis::scratch,
      TestAxis::output,
      TestAxis::time};
  for (std::size_t selected = 0U; selected < axes.size(); ++selected) {
    ExactDirectMorseEffectiveBudgetPolicy policy{
        99U, 99U, 99U, 99U, 99U};
    for (std::size_t earlier = 0U; earlier < selected; ++earlier) {
      set_limit(policy, axes[earlier], 100U);
    }
    const ExactDirectMorseBudgetSnapshot snapshot =
        evaluate(policy, hundred_unit_demand());
    check(
        snapshot.first_refusal_reason ==
            refusal_for(axes[selected]),
        "simultaneous refusals select device then host then scratch then output then time");
  }
}

void test_every_addition_overflow_fails_closed() {
  constexpr std::uint64_t maximum =
      std::numeric_limits<std::uint64_t>::max();
  const ExactDirectMorseEffectiveBudgetPolicy policy{
      maximum, maximum, maximum, maximum, maximum};

  {
    ExactDirectMorseBudgetDemand demand;
    demand.device.used_bytes = maximum;
    demand.device.reserved_bytes = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::device &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.device.addition_overflow &&
            snapshot.arithmetic_overflow_detected,
        "device used-plus-reserved overflow fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.host.used_bytes = maximum;
    demand.host.reserved_bytes = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::host &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.host.addition_overflow,
        "host used-plus-reserved overflow fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.scratch.temporary_reserved_bytes = maximum;
    demand.scratch.worst_merge_reserved_bytes = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::scratch &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.scratch.addition_overflow &&
            snapshot.scratch.reserved_bytes == maximum,
        "decomposed scratch-reserve overflow saturates its diagnostic and fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.scratch.used_bytes = maximum;
    demand.scratch.checkpoint_reserved_bytes = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::scratch &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.scratch.addition_overflow,
        "scratch used-plus-valid-transaction-reserve overflow fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.output.used_bytes = maximum;
    demand.output.reserved_bytes = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::output &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.output.addition_overflow,
        "output used-plus-reserved overflow fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.time.operation_reserved_ns = maximum;
    demand.time.checkpoint_reserved_ns = 1U;
    const auto snapshot = evaluate(policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::time &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.time.addition_overflow &&
            snapshot.time.reserved_ns == maximum,
        "time-reserve overflow fails closed without floating-point seconds");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    demand.time.operation_reserved_ns = 2U;
    const auto snapshot = evaluate(
        policy,
        demand,
        1U,
        ExactDirectMorseBudgetBoundary::before_run,
        0U,
        maximum - 2U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::time &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.time.addition_overflow,
        "elapsed-plus-valid-time-reserve overflow fails closed");
  }
  {
    ExactDirectMorseBudgetDemand demand;
    const auto snapshot =
        evaluate(policy, demand, 1U, ExactDirectMorseBudgetBoundary::before_run, 0U, maximum);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::time &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_arithmetic_overflow &&
            snapshot.time.elapsed_ns == maximum &&
            snapshot.time.addition_overflow,
        "committed elapsed plus process-epoch delta overflow fails closed");
  }
  {
    ExactDirectMorseEffectiveBudgetPolicy ordered_policy = policy;
    ordered_policy.device_limit_bytes = 0U;
    ExactDirectMorseBudgetDemand demand;
    demand.device.used_bytes = 1U;
    demand.host.used_bytes = maximum;
    demand.host.reserved_bytes = 1U;
    const auto snapshot = evaluate(ordered_policy, demand, 0U);
    check(
        snapshot.first_refusal_reason ==
                ExactDirectMorseBudgetRefusalReason::device &&
            snapshot.decision ==
                ExactDirectMorseBudgetDecision::
                    rejected_limit_exceeded &&
            snapshot.host.addition_overflow &&
            snapshot.arithmetic_overflow_detected,
        "the first refusal remains deterministic even when a later axis overflows");
  }
}

void test_injected_monotonic_clock_without_sleep() {
  std::uint64_t clock_ns = 1'000U;
  ExactDirectMorseBudgetTracker tracker{
      wide_policy(), 7U, [&clock_ns] { return clock_ns; }};
  ExactDirectMorseBudgetDemand demand;

  clock_ns = 1'010U;
  const auto first = tracker.snapshot(
      ExactDirectMorseBudgetBoundary::before_batch, 0U, demand);
  clock_ns = 1'025U;
  const auto second = tracker.snapshot(
      ExactDirectMorseBudgetBoundary::before_run, 1U, demand);
  check(
      first.time.elapsed_ns == 17U &&
          second.time.elapsed_ns == 32U &&
          first.monotonic_clock_certified &&
          second.monotonic_clock_certified,
      "the injected clock accumulates committed and process-epoch nanoseconds without sleeping");

  clock_ns = 1'024U;
  const auto regressed = tracker.snapshot(
      ExactDirectMorseBudgetBoundary::checkpoint, 2U, demand);
  clock_ns = 2'000U;
  const auto latched = tracker.snapshot(
      ExactDirectMorseBudgetBoundary::final, 3U, demand);
  check(
      regressed.first_refusal_reason ==
              ExactDirectMorseBudgetRefusalReason::time &&
          regressed.decision ==
              ExactDirectMorseBudgetDecision::
                  rejected_clock_regression &&
          regressed.time.elapsed_ns == 32U &&
          !regressed.monotonic_clock_certified &&
          latched.decision ==
              ExactDirectMorseBudgetDecision::
                  rejected_clock_regression,
      "a clock regression is latched and every later snapshot fails closed on time");
}

void test_digest_determinism_and_mutation_detection() {
  const ExactDirectMorseBudgetSnapshot first = evaluate(
      wide_policy(),
      hundred_unit_demand(),
      40U,
      ExactDirectMorseBudgetBoundary::before_merge,
      42U);
  const ExactDirectMorseBudgetSnapshot second = evaluate(
      wide_policy(),
      hundred_unit_demand(),
      40U,
      ExactDirectMorseBudgetBoundary::before_merge,
      42U);
  check(
      first == second &&
          verify_exact_direct_morse_budget_snapshot_digest(first),
      "identical trusted measurements produce byte-identical canonical snapshots and digests");

  ExactDirectMorseBudgetSnapshot mutated = first;
  ++mutated.requested_demand.scratch.safety_margin_reserved_bytes;
  check(
      !verify_exact_direct_morse_budget_snapshot_digest(mutated),
      "the digest detects a one-byte scratch-reserve mutation");

  mutated = first;
  mutated.public_v2_projection_claimed = true;
  check(
      !verify_exact_direct_morse_budget_snapshot_digest(mutated),
      "the digest binds the explicit refusal to claim a lossy public-v2 projection");
}

void test_invalid_boundary_is_not_interpreted() {
  std::uint64_t clock_ns = 0U;
  ExactDirectMorseBudgetTracker tracker{
      wide_policy(), 0U, [&clock_ns] { return clock_ns; }};
  check_invalid_argument(
      [&tracker] {
        static_cast<void>(tracker.snapshot(
            static_cast<ExactDirectMorseBudgetBoundary>(0U),
            0U,
            ExactDirectMorseBudgetDemand{}));
      },
      "an unknown transactional boundary is rejected before accounting");
}

}  // namespace

int main() {
  test_all_boundaries_and_public_projection_limit();
  test_each_axis_one_below_exact_and_one_above();
  test_deterministic_first_refusal_order();
  test_every_addition_overflow_fails_closed();
  test_injected_monotonic_clock_without_sleep();
  test_digest_determinism_and_mutation_detection();
  test_invalid_boundary_is_not_interpreted();

  if (failures != 0) {
    std::cerr << failures << " direct-morse budget test(s) failed\n";
    return 1;
  }
  std::cout << "all direct-morse budget tests passed\n";
  return 0;
}
