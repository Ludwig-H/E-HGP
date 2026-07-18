#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
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

#ifndef MORSEHGP3D_WARM_CONTEXT_CASE_COUNT
#define MORSEHGP3D_WARM_CONTEXT_CASE_COUNT 65536
#endif

#ifndef MORSEHGP3D_WARM_CONTEXT_REPEAT_COUNT
#define MORSEHGP3D_WARM_CONTEXT_REPEAT_COUNT 31
#endif

constexpr std::size_t warm_context_case_count =
    static_cast<std::size_t>(MORSEHGP3D_WARM_CONTEXT_CASE_COUNT);
constexpr std::size_t warm_context_repeat_count =
    static_cast<std::size_t>(MORSEHGP3D_WARM_CONTEXT_REPEAT_COUNT);
constexpr std::size_t warm_context_warmup_repeats = 1U;
static_assert(warm_context_case_count > 0U);
static_assert(warm_context_repeat_count > 0U);

struct Coverage {
  std::uint64_t batches{0U};
  std::uint64_t inputs{0U};
  std::uint64_t gpu_known{0U};
  std::uint64_t gpu_unknown{0U};
  std::uint64_t gpu_known_audited{0U};
  std::uint64_t fallback_batches{0U};
  std::uint64_t exact_zeros{0U};
};

struct BenchmarkCounters {
  std::uint64_t async_fallback_batches{0U};
  std::uint64_t cpu_expansion_certified{0U};
  std::uint64_t cpu_fp64_filtered_certified{0U};
  std::uint64_t cpu_multiprecision_certified{0U};
  std::uint64_t exact_zeros{0U};
  std::uint64_t gpu_fp64_certified{0U};
  std::uint64_t gpu_inputs{0U};
  std::uint64_t gpu_known_audited{0U};
  std::uint64_t gpu_unknown_forwarded{0U};
  std::uint64_t remaining_unknown{0U};
};

struct BenchmarkMeasurement {
  BenchmarkCounters counters;
  std::vector<std::uint64_t> samples_ns;
  std::uint64_t mad_ns{0U};
  std::uint64_t max_ns{0U};
  std::uint64_t min_ns{0U};
  std::uint64_t p50_ns{0U};
  std::uint64_t p95_ns{0U};
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

template <typename Counters>
void validate_benchmark_counters(
    const Counters& counters,
    std::size_t expected_cases,
    std::string_view label) {
  const std::uint64_t expected_inputs =
      static_cast<std::uint64_t>(expected_cases);
  if (counters.gpu_inputs != expected_inputs) {
    fail(label, "GPU input accounting does not match the benchmark batch");
  }
  if (counters.gpu_fp64_certified + counters.gpu_unknown_forwarded !=
      counters.gpu_inputs) {
    fail(label, "GPU tri-state accounting is not closed");
  }
  if (counters.cpu_fp64_filtered_certified +
          counters.cpu_expansion_certified +
          counters.cpu_multiprecision_certified !=
      counters.gpu_unknown_forwarded) {
    fail(label, "CPU fallback stage accounting is not closed");
  }
  if (counters.gpu_unknown_forwarded == 0U ||
      counters.async_fallback_batches != 1U ||
      counters.exact_zeros == 0U) {
    fail(label, "the benchmark batch does not exercise explicit CPU fallback");
  }
  if (counters.gpu_known_audited != 0U) {
    fail(label, "the warm benchmark unexpectedly enabled GPU-known auditing");
  }
  if (counters.remaining_unknown != 0U) {
    fail(label, "the warm benchmark published an unknown decision");
  }
}

void add_counter(std::uint64_t& total, std::uint64_t value) {
  total += value;
}

template <typename Counters>
void add_benchmark_counters(
    BenchmarkCounters& total, const Counters& counters) {
  add_counter(total.async_fallback_batches, counters.async_fallback_batches);
  add_counter(
      total.cpu_expansion_certified, counters.cpu_expansion_certified);
  add_counter(
      total.cpu_fp64_filtered_certified,
      counters.cpu_fp64_filtered_certified);
  add_counter(
      total.cpu_multiprecision_certified,
      counters.cpu_multiprecision_certified);
  add_counter(total.exact_zeros, counters.exact_zeros);
  add_counter(total.gpu_fp64_certified, counters.gpu_fp64_certified);
  add_counter(total.gpu_inputs, counters.gpu_inputs);
  add_counter(total.gpu_known_audited, counters.gpu_known_audited);
  add_counter(total.gpu_unknown_forwarded, counters.gpu_unknown_forwarded);
  add_counter(total.remaining_unknown, counters.remaining_unknown);
}

[[nodiscard]] std::uint64_t nearest_rank(
    const std::vector<std::uint64_t>& sorted_samples,
    std::size_t percentile) {
  if (sorted_samples.empty() || percentile == 0U || percentile > 100U) {
    fail("warm-context-percentile", "invalid nearest-rank request");
  }
  const std::size_t rank =
      (sorted_samples.size() * percentile + 99U) / 100U;
  return sorted_samples[rank - 1U];
}

[[nodiscard]] std::uint64_t absolute_difference(
    std::uint64_t left, std::uint64_t right) noexcept {
  return left >= right ? left - right : right - left;
}

template <typename BatchResult, typename Submit>
[[nodiscard]] BenchmarkMeasurement measure_warm_context(
    Submit&& submit, std::string_view label) {
  for (std::size_t warmup = 0U;
       warmup < warm_context_warmup_repeats;
       ++warmup) {
    const BatchResult result = submit();
    if (result.decisions.size() != warm_context_case_count) {
      fail(label, "warmup result cardinality changed");
    }
    validate_benchmark_counters(
        result.counters, warm_context_case_count, label);
  }

  BenchmarkMeasurement measurement;
  measurement.samples_ns.reserve(warm_context_repeat_count);
  for (std::size_t repeat = 0U;
       repeat < warm_context_repeat_count;
       ++repeat) {
    const auto started = std::chrono::steady_clock::now();
    const BatchResult result = submit();
    const auto stopped = std::chrono::steady_clock::now();
    if (result.decisions.size() != warm_context_case_count) {
      fail(label, "measured result cardinality changed");
    }
    validate_benchmark_counters(
        result.counters, warm_context_case_count, label);
    add_benchmark_counters(measurement.counters, result.counters);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            stopped - started)
            .count();
    if (elapsed <= 0) {
      fail(label, "steady-clock sample is not positive");
    }
    measurement.samples_ns.push_back(static_cast<std::uint64_t>(elapsed));
  }
  std::vector<std::uint64_t> sorted_samples = measurement.samples_ns;
  std::sort(sorted_samples.begin(), sorted_samples.end());
  measurement.min_ns = sorted_samples.front();
  measurement.max_ns = sorted_samples.back();
  measurement.p50_ns = nearest_rank(sorted_samples, 50U);
  measurement.p95_ns = nearest_rank(sorted_samples, 95U);
  std::vector<std::uint64_t> absolute_deviations;
  absolute_deviations.reserve(sorted_samples.size());
  for (const std::uint64_t sample : measurement.samples_ns) {
    absolute_deviations.push_back(
        absolute_difference(sample, measurement.p50_ns));
  }
  std::sort(absolute_deviations.begin(), absolute_deviations.end());
  measurement.mad_ns = nearest_rank(absolute_deviations, 50U);
  return measurement;
}

void write_benchmark_counters(
    std::ostream& output, const BenchmarkCounters& counters) {
  output
      << "{\"async_fallback_batches\":" << counters.async_fallback_batches
      << ",\"cpu_expansion_certified\":"
      << counters.cpu_expansion_certified
      << ",\"cpu_fp64_filtered_certified\":"
      << counters.cpu_fp64_filtered_certified
      << ",\"cpu_multiprecision_certified\":"
      << counters.cpu_multiprecision_certified
      << ",\"exact_zeros\":" << counters.exact_zeros
      << ",\"gpu_fp64_certified\":" << counters.gpu_fp64_certified
      << ",\"gpu_inputs\":" << counters.gpu_inputs
      << ",\"gpu_known_audited\":" << counters.gpu_known_audited
      << ",\"gpu_unknown_forwarded\":"
      << counters.gpu_unknown_forwarded
      << ",\"remaining_unknown\":" << counters.remaining_unknown << '}';
}

void write_benchmark_measurement(
    std::ostream& output,
    std::string_view predicate,
    const BenchmarkMeasurement& measurement) {
  output << "{\"counters\":";
  write_benchmark_counters(output, measurement.counters);
  output << ",\"mad_ns\":" << measurement.mad_ns
         << ",\"max_ns\":" << measurement.max_ns
         << ",\"min_ns\":" << measurement.min_ns
         << ",\"p50_ns\":" << measurement.p50_ns
         << ",\"p95_ns\":" << measurement.p95_ns
         << ",\"predicate\":\"" << predicate << "\""
         << ",\"samples_ns\":[";
  for (std::size_t index = 0U;
       index < measurement.samples_ns.size();
       ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << measurement.samples_ns[index];
  }
  output << "]}";
}

int run_warm_context_benchmark() {
  // Generate immutable batches before the first warmup. Copies into the
  // owning asynchronous API remain measured, while case generation does not.
  const auto distance_inputs =
      distance_batch(warm_context_case_count, 1000000000U, 101U);
  const auto orientation_inputs =
      orientation_batch(warm_context_case_count, 2000000000U, 103U);
  const auto power_inputs =
      power_batch(warm_context_case_count, 3000000000U, 107U);

  PredicateFilterContext context;
  constexpr SquaredDistanceBatchOptions distance_options{false};
  constexpr Orientation3DBatchOptions orientation_options{false};
  constexpr PowerBisectorBatchOptions power_options{false};
  const BenchmarkMeasurement distance =
      measure_warm_context<SquaredDistanceBatchResult>(
          [&] {
            return decide_squared_distance_batch_async(
                       context, distance_inputs, distance_options)
                .get();
          },
          "warm-context-distance");
  const BenchmarkMeasurement orientation =
      measure_warm_context<Orientation3DBatchResult>(
          [&] {
            return decide_orientation_3d_batch_async(
                       context, orientation_inputs, orientation_options)
                .get();
          },
          "warm-context-orientation");
  const BenchmarkMeasurement power =
      measure_warm_context<PowerBisectorBatchResult>(
          [&] {
            return decide_power_bisector_batch_async(
                       context, power_inputs, power_options)
                .get();
          },
          "warm-context-power");

  std::ostream& output = std::cout;
  output << "{\"audit_gpu_signs\":false,\"cases\":"
         << warm_context_case_count
         << ",\"fallback\":\"gpu_unknown_to_async_cpu_exact\""
         << ",\"repeats\":" << warm_context_repeat_count
         << ",\"results\":{\"distance\":";
  write_benchmark_measurement(
      output, "compare_squared_distances", distance);
  output << ",\"orientation\":";
  write_benchmark_measurement(output, "orientation_3d", orientation);
  output << ",\"power\":";
  write_benchmark_measurement(output, "power_bisector_side", power);
  output
      << "},\"schema\":\"morsehgp3d.phase2b.warm_context_e2e.v1\""
      << ",\"scope\":\"warm_context_e2e\""
      << ",\"timing_scope\":\"steady_clock_around_async_api_get_includes_"
         "validation_packing_transfers_kernel_fallback_synchronization_"
         "excludes_input_generation\""
      << ",\"warmup_repeats\":" << warm_context_warmup_repeats << "}\n";
  if (!output) {
    throw std::runtime_error("warm resident context benchmark output failed");
  }
  return 0;
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

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      return run();
    }
    if (argc == 2 &&
        std::string_view{argv[1]} == "--benchmark-warm-context-e2e") {
      return run_warm_context_benchmark();
    }
    throw std::invalid_argument(
        "the resident context harness accepts only "
        "--benchmark-warm-context-e2e");
  } catch (const std::exception& error) {
    std::cerr << "Phase 2B resident context harness failed: "
              << error.what() << '\n';
    return 1;
  }
}
