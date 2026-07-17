#include "fake_gpu_predicate_launchers.hpp"

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::PredicateSign;
using morsehgp3d::gpu::FilterSign;
using morsehgp3d::gpu::Orientation3DBatchResult;
using morsehgp3d::gpu::Orientation3DFilterInput;
using morsehgp3d::gpu::PowerBisectorBatchResult;
using morsehgp3d::gpu::PowerBisectorFilterInput;
using morsehgp3d::gpu::PredicateFilterContext;
using morsehgp3d::gpu::SquaredDistanceBatchResult;
using morsehgp3d::gpu::SquaredDistanceFilterInput;
using morsehgp3d::gpu::decide_orientation_3d_batch_async;
using morsehgp3d::gpu::decide_power_bisector_batch_async;
using morsehgp3d::gpu::decide_squared_distance_batch_async;
using morsehgp3d::gpu::test_support::fake_gpu_maximum_concurrency;
using morsehgp3d::gpu::test_support::fake_gpu_section_count;
using morsehgp3d::gpu::test_support::invalid_filter_sign_replay_id;
using morsehgp3d::gpu::test_support::poison_replay_id;
using morsehgp3d::gpu::test_support::reset_fake_gpu_counters;

static_assert(!std::is_copy_constructible_v<PredicateFilterContext>);
static_assert(!std::is_copy_assignable_v<PredicateFilterContext>);
static_assert(std::is_nothrow_move_constructible_v<PredicateFilterContext>);
static_assert(std::is_nothrow_move_assignable_v<PredicateFilterContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t bits(double value) noexcept {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::array<std::uint64_t, 3> point_bits(
    double x, double y, double z) noexcept {
  return {bits(x), bits(y), bits(z)};
}

[[nodiscard]] SquaredDistanceFilterInput distance_input(
    std::uint64_t replay_id) {
  return SquaredDistanceFilterInput{
      replay_id,
      point_bits(0.0, 0.0, 0.0),
      point_bits(1.0, 0.0, 0.0),
      point_bits(2.0, 0.0, 0.0)};
}

[[nodiscard]] Orientation3DFilterInput orientation_input(
    std::uint64_t replay_id) {
  return Orientation3DFilterInput{
      replay_id,
      point_bits(0.0, 0.0, 0.0),
      point_bits(1.0, 0.0, 0.0),
      point_bits(0.0, 1.0, 0.0),
      point_bits(0.0, 0.0, 1.0)};
}

[[nodiscard]] PowerBisectorFilterInput power_input(
    std::uint64_t replay_id) {
  PowerBisectorFilterInput input;
  input.replay_id = replay_id;
  input.witness_numerator_bits = point_bits(0.0, 0.0, 0.0);
  input.witness_denominator_bits = bits(1.0);
  input.cardinality = 1U;
  input.r_points[0].point_id = 1U;
  input.r_points[0].coordinate_bits = point_bits(1.0, 0.0, 0.0);
  input.q_points[0].point_id = 2U;
  input.q_points[0].coordinate_bits = point_bits(2.0, 0.0, 0.0);
  return input;
}

void check_distance_result(
    const SquaredDistanceBatchResult& result,
    std::uint64_t replay_id) {
  check(result.decisions.size() == 1U, "distance result cardinality");
  if (result.decisions.size() != 1U) {
    return;
  }
  const auto& decision = result.decisions.front();
  check(decision.replay_id == replay_id, "distance replay id survives fallback");
  check(decision.gpu_filter_sign == FilterSign::unknown,
        "distance fake GPU unknown stays diagnostic-only");
  check(decision.sign == PredicateSign::negative,
        "distance unknown is resolved by the CPU oracle");
  check(result.counters.gpu_unknown_forwarded == 1U &&
            result.counters.remaining_unknown == 0U,
        "distance unknown accounting is closed before readiness");
}

void check_orientation_result(
    const Orientation3DBatchResult& result,
    std::uint64_t replay_id) {
  check(result.decisions.size() == 1U, "orientation result cardinality");
  if (result.decisions.size() != 1U) {
    return;
  }
  const auto& decision = result.decisions.front();
  check(decision.replay_id == replay_id,
        "orientation replay id survives fallback");
  check(decision.gpu_filter_sign == FilterSign::unknown,
        "orientation fake GPU unknown stays diagnostic-only");
  check(decision.sign == PredicateSign::positive,
        "orientation unknown is resolved by the CPU oracle");
  check(result.counters.remaining_unknown == 0U,
        "orientation unknown accounting is closed before readiness");
}

void check_power_result(
    const PowerBisectorBatchResult& result,
    std::uint64_t replay_id) {
  check(result.decisions.size() == 1U, "power result cardinality");
  if (result.decisions.size() != 1U) {
    return;
  }
  const auto& decision = result.decisions.front();
  check(decision.replay_id == replay_id, "power replay id survives fallback");
  check(decision.gpu_filter_sign == FilterSign::unknown,
        "power fake GPU unknown stays diagnostic-only");
  check(decision.sign == PredicateSign::negative,
        "power unknown is resolved by the exact CPU oracle");
  check(result.counters.remaining_unknown == 0U,
        "power unknown accounting is closed before readiness");
}

void test_historical_api_and_cpu_fallback() {
  reset_fake_gpu_counters();
  check_distance_result(
      decide_squared_distance_batch_async({distance_input(10U)}).get(), 10U);
  check_orientation_result(
      decide_orientation_3d_batch_async({orientation_input(11U)}).get(), 11U);
  check_power_result(
      decide_power_bisector_batch_async({power_input(12U)}).get(), 12U);
  check(fake_gpu_section_count() == 3,
        "all historical entry points use their simulated GPU launcher");
}

void test_empty_batches_do_not_enter_gpu_section() {
  reset_fake_gpu_counters();
  PredicateFilterContext context;
  check(decide_squared_distance_batch_async(
            context, std::vector<SquaredDistanceFilterInput>{})
            .get()
            .decisions.empty(),
        "empty distance batch completes");
  check(decide_orientation_3d_batch_async(
            context, std::vector<Orientation3DFilterInput>{})
            .get()
            .decisions.empty(),
        "empty orientation batch completes");
  check(decide_power_bisector_batch_async(
            context, std::vector<PowerBisectorFilterInput>{})
            .get()
            .decisions.empty(),
        "empty power batch completes");
  check(fake_gpu_section_count() == 0,
        "empty batches do not initialize or enter the GPU section");
}

void test_context_reuse_and_gpu_serialization() {
  reset_fake_gpu_counters();
  PredicateFilterContext context;
  auto distance_future = decide_squared_distance_batch_async(
      context, {distance_input(20U)});
  auto orientation_future = decide_orientation_3d_batch_async(
      context, {orientation_input(21U)});
  check_distance_result(distance_future.get(), 20U);
  check_orientation_result(orientation_future.get(), 21U);
  check(fake_gpu_section_count() == 2,
        "one context is reused by multiple predicate kinds");
  check(fake_gpu_maximum_concurrency() == 1,
        "one context serializes only its simulated GPU sections");

  check_power_result(
      decide_power_bisector_batch_async(context, {power_input(22U)}).get(),
      22U);
  check(fake_gpu_section_count() == 3,
        "a resident context remains reusable after futures complete");
}

void test_future_owns_state_across_move_and_destruction() {
  reset_fake_gpu_counters();
  std::future<SquaredDistanceBatchResult> future;
  {
    PredicateFilterContext original;
    future = decide_squared_distance_batch_async(
        original, {distance_input(30U)});
    PredicateFilterContext moved{std::move(original)};
    check_throws<std::logic_error>(
        [&] {
          static_cast<void>(decide_squared_distance_batch_async(
              original, {distance_input(31U)}));
        },
        "a moved-from context rejects new work");
  }
  check_distance_result(future.get(), 30U);
  check(fake_gpu_section_count() == 1,
        "the future keeps context state alive after handle destruction");
}

void test_gpu_failure_poisons_only_its_context() {
  reset_fake_gpu_counters();
  PredicateFilterContext context;
  auto failing = decide_squared_distance_batch_async(
      context, {distance_input(poison_replay_id)});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(failing.get()); },
      "a simulated GPU failure reaches the caller");

  auto rejected = decide_orientation_3d_batch_async(
      context, {orientation_input(40U)});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(rejected.get()); },
      "a context fails closed after its first GPU failure");

  check_distance_result(
      decide_squared_distance_batch_async({distance_input(41U)}).get(), 41U);
  check(fake_gpu_section_count() == 1,
        "poisoning is isolated and a fresh historical context still runs");
}

void test_post_gpu_failure_poisons_context() {
  reset_fake_gpu_counters();
  PredicateFilterContext context;
  auto invalid_output = decide_squared_distance_batch_async(
      context, {distance_input(invalid_filter_sign_replay_id)});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(invalid_output.get()); },
      "an invalid tri-state is rejected after a successful GPU section");

  auto rejected = decide_orientation_3d_batch_async(
      context, {orientation_input(50U)});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(rejected.get()); },
      "a post-GPU publication failure poisons the resident context");

  check_distance_result(
      decide_squared_distance_batch_async({distance_input(51U)}).get(), 51U);
  check(fake_gpu_section_count() == 2,
        "post-GPU poisoning is isolated from a fresh historical context");
}

void test_input_validation_does_not_poison_context() {
  reset_fake_gpu_counters();
  PredicateFilterContext context;
  std::vector<SquaredDistanceFilterInput> duplicate_replay_ids{
      distance_input(60U), distance_input(60U)};
  auto invalid = decide_squared_distance_batch_async(
      context, std::move(duplicate_replay_ids));
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(invalid.get()); },
      "input validation rejects duplicate replay identifiers");

  check_orientation_result(
      decide_orientation_3d_batch_async(
          context, {orientation_input(61U)})
          .get(),
      61U);
  check(fake_gpu_section_count() == 1,
        "pre-GPU validation failure does not poison the context");
}

}  // namespace

int main() {
  test_historical_api_and_cpu_fallback();
  test_empty_batches_do_not_enter_gpu_section();
  test_context_reuse_and_gpu_serialization();
  test_future_owns_state_across_move_and_destruction();
  test_gpu_failure_poisons_only_its_context();
  test_post_gpu_failure_poisons_context();
  test_input_validation_does_not_poison_context();
  if (failures != 0) {
    std::cerr << failures << " predicate context test(s) failed\n";
    return 1;
  }
  std::cout << "predicate context tests passed\n";
  return 0;
}
