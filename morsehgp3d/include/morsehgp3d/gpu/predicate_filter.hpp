#pragma once

#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <cstdint>
#include <future>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::size_t maximum_power_bisector_cardinality = 10U;

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

struct Orientation3DFilterInput {
  std::uint64_t replay_id{0};
  std::array<std::uint64_t, 3> a_bits{};
  std::array<std::uint64_t, 3> b_bits{};
  std::array<std::uint64_t, 3> c_bits{};
  std::array<std::uint64_t, 3> d_bits{};
};
static_assert(std::is_trivially_copyable_v<Orientation3DFilterInput>);

struct Orientation3DDecision {
  std::uint64_t replay_id{0};
  FilterSign gpu_filter_sign{FilterSign::unknown};
  exact::PredicateSign sign{exact::PredicateSign::zero};
  exact::CertificationStage certification_stage{
      exact::CertificationStage::cpu_multiprecision};
};

struct Orientation3DFilterCounters {
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

struct Orientation3DBatchResult {
  std::vector<Orientation3DDecision> decisions;
  Orientation3DFilterCounters counters;
};

struct Orientation3DBatchOptions {
  // Qualification mode: independently recompute every GPU-known result with
  // the CPU multiprecision oracle and fail closed on any contradiction.
  bool audit_gpu_signs{false};
};

// Sign convention: det([b-a, c-a, d-a]); the unit tetrahedron is positive.
// Every device unknown is resolved by the adaptive CPU predicate before the
// owning future becomes ready.
[[nodiscard]] std::future<Orientation3DBatchResult>
decide_orientation_3d_batch_async(
    std::vector<Orientation3DFilterInput> inputs,
    Orientation3DBatchOptions options = {});

struct PowerBisectorLabelPoint {
  std::uint32_t point_id{0};
  std::array<std::uint64_t, 3> coordinate_bits{};
};
static_assert(std::is_trivially_copyable_v<PowerBisectorLabelPoint>);

struct PowerBisectorFilterInput {
  std::uint64_t replay_id{0};
  // The exact witness is y = A / D. The GPU path only admits binary64 words
  // that represent the integer coordinates of A exactly and a strictly
  // positive exactly represented integer D. The four integers form a reduced
  // homogeneous tuple; no rounded conversion from a general rational witness
  // is permitted.
  std::array<std::uint64_t, 3> witness_numerator_bits{};
  std::uint64_t witness_denominator_bits{0};
  std::uint32_t cardinality{0};
  std::array<PowerBisectorLabelPoint,
             maximum_power_bisector_cardinality>
      r_points{};
  std::array<PowerBisectorLabelPoint,
             maximum_power_bisector_cardinality>
      q_points{};
};
static_assert(std::is_trivially_copyable_v<PowerBisectorFilterInput>);

struct PowerBisectorDecision {
  std::uint64_t replay_id{0};
  FilterSign gpu_filter_sign{FilterSign::unknown};
  exact::PredicateSign sign{exact::PredicateSign::zero};
  exact::CertificationStage certification_stage{
      exact::CertificationStage::cpu_multiprecision};
};

struct PowerBisectorFilterCounters {
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

struct PowerBisectorBatchResult {
  std::vector<PowerBisectorDecision> decisions;
  PowerBisectorFilterCounters counters;
};

struct PowerBisectorBatchOptions {
  bool audit_gpu_signs{false};
};

// Sign convention:
// H_{R,Q}(y) = sum_{r in R} ||y-r||^2 - sum_{q in Q} ||y-q||^2.
// The device evaluates the sign-equivalent homogeneous polynomial
// D H_{R,Q}(A/D) without division. Every device unknown is resolved by the
// exact rational CPU predicate before the owning future becomes ready.
[[nodiscard]] std::future<PowerBisectorBatchResult>
decide_power_bisector_batch_async(
    std::vector<PowerBisectorFilterInput> inputs,
    PowerBisectorBatchOptions options = {});

}  // namespace morsehgp3d::gpu
