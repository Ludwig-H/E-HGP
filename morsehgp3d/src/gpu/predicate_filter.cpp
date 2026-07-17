#include "morsehgp3d/gpu/predicate_filter.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "phase2b_distance_filter_internal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using exact::CertifiedPoint3;
using exact::CertificationStage;
using exact::PredicateDecision;
using exact::PredicateFilterPolicy;
using exact::PredicateSign;

struct IndexedDecision {
  std::size_t index{0};
  PredicateDecision decision{
      PredicateSign::zero, CertificationStage::cpu_multiprecision};
};

void require_finite_words(
    const std::array<std::uint64_t, 3>& words,
    const char* label) {
  for (const std::uint64_t word : words) {
    if (!exact::is_finite_binary64_bits(word)) {
      throw std::invalid_argument(
          std::string{"Phase 2B "} + label +
          " coordinates must contain finite binary64 words");
    }
  }
}

void validate_inputs(std::span<const SquaredDistanceFilterInput> inputs) {
  std::unordered_set<std::uint64_t> replay_ids;
  replay_ids.reserve(inputs.size());
  for (const SquaredDistanceFilterInput& input : inputs) {
    if (!replay_ids.insert(input.replay_id).second) {
      throw std::invalid_argument(
          "Phase 2B replay identifiers must be unique within a batch");
    }
    require_finite_words(input.witness_bits, "witness");
    require_finite_words(input.left_bits, "left");
    require_finite_words(input.right_bits, "right");
  }
}

[[nodiscard]] CertifiedPoint3 point_from_words(
    const std::array<std::uint64_t, 3>& words) {
  return CertifiedPoint3::from_binary64_bits(words);
}

[[nodiscard]] PredicateDecision cpu_decision(
    const SquaredDistanceFilterInput& input,
    PredicateFilterPolicy policy) {
  return exact::decide_squared_distance_order(
      point_from_words(input.witness_bits),
      point_from_words(input.left_bits),
      point_from_words(input.right_bits),
      nullptr,
      policy);
}

[[nodiscard]] PredicateSign predicate_sign_from_gpu(FilterSign sign) {
  switch (sign) {
    case FilterSign::negative:
      return PredicateSign::negative;
    case FilterSign::positive:
      return PredicateSign::positive;
    case FilterSign::unknown:
      break;
  }
  throw std::logic_error("a GPU unknown cannot be promoted to a predicate sign");
}

void record_cpu_stage(
    const PredicateDecision& decision,
    SquaredDistanceFilterCounters& counters) {
  switch (decision.certification_stage()) {
    case CertificationStage::fp64_filtered:
      ++counters.cpu_fp64_filtered_certified;
      break;
    case CertificationStage::expansion:
      ++counters.cpu_expansion_certified;
      break;
    case CertificationStage::cpu_multiprecision:
      ++counters.cpu_multiprecision_certified;
      break;
  }
  if (decision.sign() == PredicateSign::zero) {
    ++counters.exact_zeros;
  }
}

[[nodiscard]] std::vector<IndexedDecision> resolve_unknowns(
    const std::vector<SquaredDistanceFilterInput>& inputs,
    const std::vector<std::size_t>& unknown_indices) {
  std::vector<IndexedDecision> decisions;
  decisions.reserve(unknown_indices.size());
  for (const std::size_t index : unknown_indices) {
    decisions.push_back(IndexedDecision{
        index,
        cpu_decision(inputs[index], PredicateFilterPolicy::allow_adaptive)});
  }
  return decisions;
}

[[nodiscard]] SquaredDistanceBatchResult decide_batch(
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options) {
  validate_inputs(inputs);
  const std::vector<detail::RawSquaredDistanceFilterOutput> gpu_outputs =
      detail::filter_squared_distances_on_gpu(inputs);
  if (gpu_outputs.size() != inputs.size()) {
    throw std::runtime_error("the Phase 2B GPU output cardinality changed");
  }

  SquaredDistanceBatchResult result;
  result.decisions.resize(inputs.size());
  result.counters.gpu_inputs = static_cast<std::uint64_t>(inputs.size());
  std::vector<std::size_t> unknown_indices;
  unknown_indices.reserve(inputs.size());

  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    const detail::RawSquaredDistanceFilterOutput& output = gpu_outputs[index];
    if (output.replay_id != inputs[index].replay_id) {
      throw std::runtime_error(
          "the Phase 2B GPU changed replay identifier ordering");
    }
    switch (output.sign) {
      case FilterSign::negative:
      case FilterSign::positive:
        ++result.counters.gpu_fp64_certified;
        result.decisions[index] = SquaredDistanceDecision{
            output.replay_id,
            output.sign,
            predicate_sign_from_gpu(output.sign),
            CertificationStage::fp64_filtered};
        break;
      case FilterSign::unknown:
        ++result.counters.gpu_unknown_forwarded;
        unknown_indices.push_back(index);
        break;
      default:
        throw std::runtime_error("the Phase 2B GPU returned an invalid tri-state");
    }
  }

  std::future<std::vector<IndexedDecision>> fallback_future;
  if (!unknown_indices.empty()) {
    ++result.counters.async_fallback_batches;
    fallback_future = std::async(
        std::launch::async,
        [&inputs, indices = unknown_indices] {
          return resolve_unknowns(inputs, indices);
        });
  }

  if (options.audit_gpu_signs) {
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
      if (gpu_outputs[index].sign == FilterSign::unknown) {
        continue;
      }
      const PredicateDecision oracle = cpu_decision(
          inputs[index], PredicateFilterPolicy::multiprecision_only);
      if (oracle.sign() != result.decisions[index].sign) {
        throw std::runtime_error(
            "the Phase 2B GPU filter contradicted the CPU multiprecision oracle");
      }
      ++result.counters.gpu_known_audited;
    }
  }

  if (!unknown_indices.empty()) {
    for (const IndexedDecision& resolved : fallback_future.get()) {
      const std::size_t index = resolved.index;
      result.decisions[index] = SquaredDistanceDecision{
          inputs[index].replay_id,
          FilterSign::unknown,
          resolved.decision.sign(),
          resolved.decision.certification_stage()};
      record_cpu_stage(resolved.decision, result.counters);
    }
  }
  result.counters.remaining_unknown = 0U;
  return result;
}

}  // namespace

std::future<SquaredDistanceBatchResult> decide_squared_distance_batch_async(
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options) {
  return std::async(
      std::launch::async,
      [owned_inputs = std::move(inputs), options]() mutable {
        return decide_batch(std::move(owned_inputs), options);
      });
}

}  // namespace morsehgp3d::gpu
