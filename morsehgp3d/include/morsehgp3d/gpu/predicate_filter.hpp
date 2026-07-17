#pragma once

#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <cstdint>
#include <future>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu {

// This tri-state is intentionally distinct from PredicateSign. A zero-valued
// device result means that the GPU did not certify a sign; it never means an
// exact zero.
enum class FilterSign : std::int8_t {
  negative = -1,
  unknown = 0,
  positive = 1,
};
static_assert(static_cast<std::int8_t>(FilterSign::negative) == -1);
static_assert(static_cast<std::int8_t>(FilterSign::unknown) == 0);
static_assert(static_cast<std::int8_t>(FilterSign::positive) == 1);

struct SquaredDistanceFilterInput {
  std::uint64_t replay_id{0};
  std::array<std::uint64_t, 3> witness_bits{};
  std::array<std::uint64_t, 3> left_bits{};
  std::array<std::uint64_t, 3> right_bits{};
};
static_assert(std::is_trivially_copyable_v<SquaredDistanceFilterInput>);

struct SquaredDistanceDecision {
  std::uint64_t replay_id{0};
  FilterSign gpu_filter_sign{FilterSign::unknown};
  exact::PredicateSign sign{exact::PredicateSign::zero};
  exact::CertificationStage certification_stage{
      exact::CertificationStage::cpu_multiprecision};
};

struct SquaredDistanceFilterCounters {
  std::uint64_t gpu_inputs{0};
  std::uint64_t gpu_fp64_certified{0};
  std::uint64_t gpu_unknown_forwarded{0};
  std::uint64_t cpu_fp64_filtered_certified{0};
  std::uint64_t cpu_expansion_certified{0};
  std::uint64_t cpu_multiprecision_certified{0};
  std::uint64_t exact_zeros{0};
  std::uint64_t gpu_known_audited{0};
  std::uint64_t async_fallback_batches{0};
  std::uint64_t remaining_unknown{0};
};

struct SquaredDistanceBatchResult {
  std::vector<SquaredDistanceDecision> decisions;
  SquaredDistanceFilterCounters counters;
};

struct SquaredDistanceBatchOptions {
  // Qualification mode: independently recompute every GPU-known result with
  // the CPU multiprecision oracle and fail closed on any contradiction.
  bool audit_gpu_signs{false};
};

// The returned future owns the input batch. GPU filtering and the CPU fallback
// therefore run without borrowing caller memory. Every device unknown is
// resolved by the adaptive CPU predicate before the future becomes ready.
[[nodiscard]] std::future<SquaredDistanceBatchResult>
decide_squared_distance_batch_async(
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options = {});

}  // namespace morsehgp3d::gpu
