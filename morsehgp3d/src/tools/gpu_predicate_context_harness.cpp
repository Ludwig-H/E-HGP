#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::gpu::Orientation3DBatchOptions;
using morsehgp3d::gpu::Orientation3DBatchResult;
using morsehgp3d::gpu::Orientation3DFilterInput;
using morsehgp3d::gpu::PowerBisectorBatchOptions;
using morsehgp3d::gpu::PowerBisectorBatchResult;
using morsehgp3d::gpu::PowerBisectorFilterInput;
using morsehgp3d::gpu::PredicateFilterContext;
using morsehgp3d::gpu::SquaredDistanceBatchOptions;
using morsehgp3d::gpu::SquaredDistanceBatchResult;
using morsehgp3d::gpu::SquaredDistanceFilterInput;
using morsehgp3d::gpu::decide_orientation_3d_batch_async;
using morsehgp3d::gpu::decide_power_bisector_batch_async;
using morsehgp3d::gpu::decide_squared_distance_batch_async;

struct Coverage {
  std::uint64_t batches{0U};
  std::uint64_t inputs{0U};
  std::uint64_t gpu_known{0U};
  std::uint64_t gpu_unknown{0U};
  std::uint64_t gpu_known_audited{0U};
  std::uint64_t fallback_batches{0U};
  std::uint64_t exact_zeros{0U};
};

[[noreturn]] void fail(std::string_view label, std::string_view detail) {
  throw std::runtime_error(
      std::string{label} + ": " + std::string{detail});
}

[[nodiscard]] std::uint64_t bits(double value) noexcept {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::array<std::uint64_t, 3> point_bits(
    double x, double y, double z) noexcept {
  return {bits(x), bits(y), bits(z)};
}

[[nodiscard]] std::vector<SquaredDistanceFilterInput> distance_batch(
    std::size_t count,
    std::uint64_t first_replay_id,
    std::uint32_t seed) {
  std::vector<SquaredDistanceFilterInput> inputs;
  inputs.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const std::uint32_t variant =
        (seed + static_cast<std::uint32_t>(index % 5U)) % 5U;
    const double offset = static_cast<double>(variant + 1U);
    const auto witness = point_bits(0.0, offset, 0.0);
    std::array<std::uint64_t, 3> left{};
    std::array<std::uint64_t, 3> right{};
    switch (index % 3U) {
      case 0U:
        left = point_bits(1.0 + offset, offset, 0.0);
        right = point_bits(6.0 + offset, offset, 0.0);
        break;
      case 1U:
        left = point_bits(7.0 + offset, offset, 0.0);
        right = point_bits(2.0 + offset, offset, 0.0);
        break;
      default:
        left = point_bits(3.0 + offset, offset, 0.0);
        right = point_bits(-(3.0 + offset), offset, 0.0);
        break;
    }
    inputs.push_back(SquaredDistanceFilterInput{
        first_replay_id + static_cast<std::uint64_t>(index),
        witness,
        left,
        right});
  }
  return inputs;
}

[[nodiscard]] std::vector<Orientation3DFilterInput> orientation_batch(
    std::size_t count,
    std::uint64_t first_replay_id,
    std::uint32_t seed) {
  std::vector<Orientation3DFilterInput> inputs;
  inputs.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const std::uint32_t variant =
        (seed + static_cast<std::uint32_t>(index % 4U)) % 4U;
    const double scale = static_cast<double>(variant + 1U);
    const double tx = static_cast<double>(seed % 3U);
    const double ty = static_cast<double>(index % 2U);
    const double tz = -static_cast<double>(seed % 2U);
    const auto a = point_bits(tx, ty, tz);
    const auto u = point_bits(tx + scale, ty, tz);
    const auto v = point_bits(tx, ty + scale + 1.0, tz);
    const auto w = point_bits(tx, ty, tz + scale + 2.0);
    Orientation3DFilterInput input;
    input.replay_id =
        first_replay_id + static_cast<std::uint64_t>(index);
    input.a_bits = a;
    switch (index % 3U) {
      case 0U:
        input.b_bits = u;
        input.c_bits = v;
        input.d_bits = w;
        break;
      case 1U:
        input.b_bits = v;
        input.c_bits = u;
        input.d_bits = w;
        break;
      default:
        input.b_bits = u;
        input.c_bits = v;
        input.d_bits = point_bits(
            tx + scale, ty + scale + 1.0, tz);
        break;
    }
    inputs.push_back(input);
  }
  return inputs;
}

[[nodiscard]] std::vector<PowerBisectorFilterInput> power_batch(
    std::size_t count,
    std::uint64_t first_replay_id,
    std::uint32_t seed) {
  std::vector<PowerBisectorFilterInput> inputs;
  inputs.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const std::uint32_t variant =
        (seed + static_cast<std::uint32_t>(index % 5U)) % 5U;
    const double witness_x = static_cast<double>(variant);
    const double radius = static_cast<double>(variant + 1U);
    PowerBisectorFilterInput input;
    input.replay_id =
        first_replay_id + static_cast<std::uint64_t>(index);
    input.witness_numerator_bits = point_bits(witness_x, 0.0, 0.0);
    input.witness_denominator_bits = bits(1.0);
    input.cardinality = 1U;
    input.r_points[0].point_id = 10U;
    input.q_points[0].point_id = 20U;
    switch (index % 3U) {
      case 0U:
        input.r_points[0].coordinate_bits =
            point_bits(witness_x + radius, 0.0, 0.0);
        input.q_points[0].coordinate_bits =
            point_bits(witness_x + radius + 5.0, 0.0, 0.0);
        break;
      case 1U:
        input.r_points[0].coordinate_bits =
            point_bits(witness_x + radius + 6.0, 0.0, 0.0);
        input.q_points[0].coordinate_bits =
            point_bits(witness_x + radius, 0.0, 0.0);
        break;
      default:
        input.r_points[0].coordinate_bits =
            point_bits(witness_x + radius, 0.0, 0.0);
        input.q_points[0].coordinate_bits =
            point_bits(witness_x - radius, 0.0, 0.0);
        break;
    }
    inputs.push_back(input);
  }
  return inputs;
}

template <typename Decision>
[[nodiscard]] bool same_decision(
    const Decision& left, const Decision& right) noexcept {
  return left.replay_id == right.replay_id &&
         left.gpu_filter_sign == right.gpu_filter_sign &&
         left.sign == right.sign &&
         left.certification_stage == right.certification_stage;
}

template <typename Counters>
[[nodiscard]] bool same_counters(
    const Counters& left, const Counters& right) noexcept {
  return left.gpu_inputs == right.gpu_inputs &&
         left.gpu_fp64_certified == right.gpu_fp64_certified &&
         left.gpu_unknown_forwarded == right.gpu_unknown_forwarded &&
         left.cpu_fp64_filtered_certified ==
             right.cpu_fp64_filtered_certified &&
         left.cpu_expansion_certified == right.cpu_expansion_certified &&
         left.cpu_multiprecision_certified ==
             right.cpu_multiprecision_certified &&
         left.exact_zeros == right.exact_zeros &&
         left.gpu_known_audited == right.gpu_known_audited &&
         left.async_fallback_batches == right.async_fallback_batches &&
         left.remaining_unknown == right.remaining_unknown;
}

template <typename BatchResult>
void compare_results(
    const BatchResult& resident,
    const BatchResult& ephemeral,
    std::string_view label) {
  if (!same_counters(resident.counters, ephemeral.counters)) {
    fail(label, "resident and ephemeral counters differ");
  }
  if (resident.decisions.size() != ephemeral.decisions.size()) {
    fail(label, "resident and ephemeral decision counts differ");
  }
  for (std::size_t index = 0U; index < resident.decisions.size(); ++index) {
    if (!same_decision(resident.decisions[index], ephemeral.decisions[index])) {
      fail(label, "resident and ephemeral decisions differ");
    }
  }
}

template <typename BatchResult>
void record_coverage(
    const BatchResult& result,
    Coverage& coverage,
    std::string_view label) {
  if (result.counters.remaining_unknown != 0U) {
    fail(label, "a published result retained an unknown decision");
  }
  if (result.counters.gpu_known_audited !=
      result.counters.gpu_fp64_certified) {
    fail(label, "audit mode did not cover every GPU-known sign");
  }
  ++coverage.batches;
  coverage.inputs += result.counters.gpu_inputs;
  coverage.gpu_known += result.counters.gpu_fp64_certified;
  coverage.gpu_unknown += result.counters.gpu_unknown_forwarded;
  coverage.gpu_known_audited += result.counters.gpu_known_audited;
  coverage.fallback_batches += result.counters.async_fallback_batches;
  coverage.exact_zeros += result.counters.exact_zeros;
}

void run_distance(
    PredicateFilterContext& context,
    const std::vector<SquaredDistanceFilterInput>& inputs,
    Coverage& coverage,
    std::string_view label) {
  constexpr SquaredDistanceBatchOptions options{true};
  const SquaredDistanceBatchResult resident =
      decide_squared_distance_batch_async(context, inputs, options).get();
  const SquaredDistanceBatchResult ephemeral =
      decide_squared_distance_batch_async(inputs, options).get();
  compare_results(resident, ephemeral, label);
  record_coverage(resident, coverage, label);
}

void run_orientation(
    PredicateFilterContext& context,
    const std::vector<Orientation3DFilterInput>& inputs,
    Coverage& coverage,
    std::string_view label) {
  constexpr Orientation3DBatchOptions options{true};
  const Orientation3DBatchResult resident =
      decide_orientation_3d_batch_async(context, inputs, options).get();
  const Orientation3DBatchResult ephemeral =
      decide_orientation_3d_batch_async(inputs, options).get();
  compare_results(resident, ephemeral, label);
  record_coverage(resident, coverage, label);
}

void run_power(
    PredicateFilterContext& context,
    const std::vector<PowerBisectorFilterInput>& inputs,
    Coverage& coverage,
    std::string_view label) {
  constexpr PowerBisectorBatchOptions options{true};
  const PowerBisectorBatchResult resident =
      decide_power_bisector_batch_async(context, inputs, options).get();
  const PowerBisectorBatchResult ephemeral =
      decide_power_bisector_batch_async(inputs, options).get();
  compare_results(resident, ephemeral, label);
  record_coverage(resident, coverage, label);
}

void run_first_independent_pair(
    PredicateFilterContext& primary,
    PredicateFilterContext& secondary,
    Coverage& distance,
    Coverage& orientation) {
  const auto distance_inputs = distance_batch(65U, 100000U, 31U);
  const auto orientation_inputs = orientation_batch(67U, 110000U, 37U);
  constexpr SquaredDistanceBatchOptions distance_options{true};
  constexpr Orientation3DBatchOptions orientation_options{true};
  std::future<SquaredDistanceBatchResult> distance_future =
      decide_squared_distance_batch_async(
          primary, distance_inputs, distance_options);
  std::future<Orientation3DBatchResult> orientation_future =
      decide_orientation_3d_batch_async(
          secondary, orientation_inputs, orientation_options);
  const SquaredDistanceBatchResult distance_resident = distance_future.get();
  const Orientation3DBatchResult orientation_resident =
      orientation_future.get();
  const SquaredDistanceBatchResult distance_ephemeral =
      decide_squared_distance_batch_async(distance_inputs, distance_options).get();
  const Orientation3DBatchResult orientation_ephemeral =
      decide_orientation_3d_batch_async(
          orientation_inputs, orientation_options)
          .get();
  compare_results(
      distance_resident, distance_ephemeral, "independent-distance");
  compare_results(
      orientation_resident, orientation_ephemeral, "independent-orientation");
  record_coverage(distance_resident, distance, "independent-distance");
  record_coverage(
      orientation_resident, orientation, "independent-orientation");
}

void run_second_independent_pair(
    PredicateFilterContext& primary,
    PredicateFilterContext& secondary,
    Coverage& power,
    Coverage& distance) {
  const auto power_inputs = power_batch(33U, 120000U, 41U);
  const auto distance_inputs = distance_batch(35U, 130000U, 43U);
  constexpr PowerBisectorBatchOptions power_options{true};
  constexpr SquaredDistanceBatchOptions distance_options{true};
  std::future<PowerBisectorBatchResult> power_future =
      decide_power_bisector_batch_async(primary, power_inputs, power_options);
  std::future<SquaredDistanceBatchResult> distance_future =
      decide_squared_distance_batch_async(
          secondary, distance_inputs, distance_options);
  const PowerBisectorBatchResult power_resident = power_future.get();
  const SquaredDistanceBatchResult distance_resident = distance_future.get();
  const PowerBisectorBatchResult power_ephemeral =
      decide_power_bisector_batch_async(power_inputs, power_options).get();
  const SquaredDistanceBatchResult distance_ephemeral =
      decide_squared_distance_batch_async(distance_inputs, distance_options).get();
  compare_results(power_resident, power_ephemeral, "independent-power");
  compare_results(
      distance_resident, distance_ephemeral, "independent-distance-reuse");
  record_coverage(power_resident, power, "independent-power");
  record_coverage(
      distance_resident, distance, "independent-distance-reuse");
}

void require_complete_coverage(
    const Coverage& coverage, std::string_view label) {
  if (coverage.inputs == 0U || coverage.gpu_known == 0U ||
      coverage.gpu_unknown == 0U || coverage.fallback_batches == 0U ||
      coverage.exact_zeros == 0U ||
      coverage.gpu_known_audited != coverage.gpu_known) {
    fail(label, "known, audited, fallback and exact-zero coverage is incomplete");
  }
}

int run() {
  PredicateFilterContext primary;
  PredicateFilterContext secondary;
  Coverage distance;
  Coverage orientation;
  Coverage power;

  // One resident context alternates predicates through large-small-large
  // sequences. Every batch uses a distinct seed and replay-id interval.
  run_distance(
      primary, distance_batch(513U, 1000U, 1U), distance, "distance-large-a");
  run_orientation(
      primary, orientation_batch(5U, 10000U, 3U), orientation,
      "orientation-small-a");
  run_power(
      primary, power_batch(385U, 20000U, 5U), power, "power-large-a");
  run_distance(
      primary, distance_batch(7U, 30000U, 7U), distance, "distance-small-b");
  run_orientation(
      primary, orientation_batch(321U, 40000U, 11U), orientation,
      "orientation-large-b");
  run_power(primary, power_batch(3U, 50000U, 13U), power, "power-small-b");
  run_distance(
      primary, distance_batch(257U, 60000U, 17U), distance,
      "distance-large-c");
  run_orientation(
      primary, orientation_batch(4U, 70000U, 19U), orientation,
      "orientation-small-c");
  run_power(
      primary, power_batch(289U, 80000U, 23U), power, "power-large-c");

  run_distance(primary, {}, distance, "distance-empty");
  run_orientation(primary, {}, orientation, "orientation-empty");
  run_power(primary, {}, power, "power-empty");

  // Both contexts are reused, and these pairs may overlap on independent
  // streams while retaining deterministic scientific results.
  run_first_independent_pair(
      primary, secondary, distance, orientation);
  run_second_independent_pair(primary, secondary, power, distance);

  require_complete_coverage(distance, "distance-coverage");
  require_complete_coverage(orientation, "orientation-coverage");
  require_complete_coverage(power, "power-coverage");

  const std::uint64_t batches =
      distance.batches + orientation.batches + power.batches;
  const std::uint64_t inputs =
      distance.inputs + orientation.inputs + power.inputs;
  const std::uint64_t gpu_known =
      distance.gpu_known + orientation.gpu_known + power.gpu_known;
  const std::uint64_t gpu_unknown =
      distance.gpu_unknown + orientation.gpu_unknown + power.gpu_unknown;
  std::cout
      << "{\"contexts\":2,\"empty_batches\":3,\"ephemeral_comparisons\":"
      << batches << ",\"gpu_known\":" << gpu_known
      << ",\"gpu_unknown_forwarded\":" << gpu_unknown
      << ",\"inputs\":" << inputs
      << ",\"resident_batches\":" << batches
      << ",\"schema\":\"morsehgp3d.phase2b.resident_context.v1\""
      << ",\"status\":\"ok\"}\n";
  if (!std::cout) {
    throw std::runtime_error("resident context harness output failed");
  }
  return 0;
}

}  // namespace

int main(int argc, char**) {
  try {
    if (argc != 1) {
      throw std::invalid_argument(
          "the resident context harness accepts no arguments");
    }
    return run();
  } catch (const std::exception& error) {
    std::cerr << "Phase 2B resident context harness failed: "
              << error.what() << '\n';
    return 1;
  }
}
