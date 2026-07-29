#include "fake_gpu_exact_diametral_phi_launchers.hpp"

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/gpu/exact_diametral_phi.hpp"
#include "phase15_exact_diametral_phi_fixed.cuh"

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::gpu::ExactDiametralPhiBatchOptions;
using morsehgp3d::gpu::ExactDiametralPhiCertificationStage;
using morsehgp3d::gpu::ExactDiametralPhiContext;
using morsehgp3d::gpu::ExactDiametralPhiInput;
namespace fixed = morsehgp3d::gpu::detail::exact_phi_fixed;

static_assert(!std::is_copy_constructible_v<ExactDiametralPhiContext>);
static_assert(!std::is_copy_assignable_v<ExactDiametralPhiContext>);
static_assert(std::is_nothrow_move_constructible_v<ExactDiametralPhiContext>);
static_assert(std::is_nothrow_move_assignable_v<ExactDiametralPhiContext>);
static_assert(fixed::difference_limb_bit_count == 128U);
static_assert(fixed::accumulator_limb_bit_count == 256U);
static_assert(fixed::guaranteed_global_coordinate_bit_count == 126U);

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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::array<std::uint64_t, 3U> point(
    double x,
    double y,
    double z) {
  return {bits(x), bits(y), bits(z)};
}

[[nodiscard]] ExactDiametralPhiInput input(
    std::uint64_t replay_id,
    std::array<std::uint64_t, 3U> witness,
    std::array<std::uint64_t, 3U> first,
    std::array<std::uint64_t, 3U> second) {
  return ExactDiametralPhiInput{replay_id, witness, first, second};
}

[[nodiscard]] PredicateSign oracle_sign(const ExactDiametralPhiInput& value) {
  ExactRational phi;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational witness =
        ExactRational::from_binary64_bits(value.witness_bits[axis]);
    const ExactRational first =
        ExactRational::from_binary64_bits(value.first_support_bits[axis]);
    const ExactRational second =
        ExactRational::from_binary64_bits(value.second_support_bits[axis]);
    phi = phi + (witness - first) * (witness - second);
  }
  return morsehgp3d::exact::predicate_sign(phi.sign());
}

[[nodiscard]] fixed::ResultCode fixed_sign(
    const ExactDiametralPhiInput& value) {
  return fixed::exact_sign(
      value.witness_bits.data(),
      value.first_support_bits.data(),
      value.second_support_bits.data());
}

[[nodiscard]] PredicateSign predicate_sign(fixed::ResultCode value) {
  switch (value) {
    case fixed::ResultCode::negative:
      return PredicateSign::negative;
    case fixed::ResultCode::zero:
      return PredicateSign::zero;
    case fixed::ResultCode::positive:
      return PredicateSign::positive;
    case fixed::ResultCode::requires_cpu_fallback:
    case fixed::ResultCode::invalid_input:
      break;
  }
  throw std::logic_error("a fallback has no fixed-limb sign");
}

[[nodiscard]] std::uint64_t finite_word(
    std::uint64_t random,
    bool bounded_exponent) {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  const std::uint64_t sign = random & (UINT64_C(1) << 63U);
  const std::uint64_t fraction = random & fraction_mask;
  const std::uint64_t exponent = bounded_exponent
                                     ? 990U + ((random >> 53U) % 68U)
                                     : ((random >> 52U) % 0x7ffU);
  return sign | (exponent << 52U) | fraction;
}

[[nodiscard]] std::uint64_t next_random(std::uint64_t& state) {
  state ^= state << 13U;
  state ^= state >> 7U;
  state ^= state << 17U;
  return state;
}

void test_low_level_boundaries() {
  fixed::UInt128 value{};
  check(
      fixed::shift_word_to_u128(1U, 63U, value) &&
          value.limb[0] == (UINT64_C(1) << 63U) && value.limb[1] == 0U,
      "a 63-bit shift must remain in the low limb");
  check(
      fixed::shift_word_to_u128(1U, 64U, value) &&
          value.limb[0] == 0U && value.limb[1] == 1U,
      "a 64-bit shift must cross exactly one limb");
  check(
      fixed::shift_word_to_u128(1U, 127U, value) &&
          value.limb[1] == (UINT64_C(1) << 63U),
      "a 127-bit shift must fit exactly");
  check(
      !fixed::shift_word_to_u128(1U, 128U, value),
      "a 128-bit shift of one must fail open");
  check(
      fixed::shift_word_to_u128(
          (UINT64_C(1) << 52U) | 1U, 75U, value),
      "bit_width plus shift equal to 128 must fit");
  check(
      !fixed::shift_word_to_u128(
          (UINT64_C(1) << 52U) | 1U, 76U, value),
      "bit_width plus shift above 128 must fail open");

  fixed::UInt256 one{};
  one.limb[0] = 1U;
  fixed::UInt256 shifted{};
  check(
      fixed::shift_left(one, 191U, shifted) &&
          shifted.limb[2] == (UINT64_C(1) << 63U),
      "a 191-bit product alignment shift must fit");
  check(
      fixed::shift_left(one, 192U, shifted) && shifted.limb[3] == 1U,
      "a 192-bit product alignment shift must cross exactly three limbs");
  check(
      fixed::shift_left(one, 255U, shifted) &&
          shifted.limb[3] == (UINT64_C(1) << 63U),
      "a 255-bit product alignment shift must fit");
  check(
      !fixed::shift_left(one, 256U, shifted),
      "a 256-bit product alignment shift must fail open");

  fixed::Signed256 maximum{};
  for (std::uint64_t& limb : maximum.magnitude.limb) {
    limb = std::numeric_limits<std::uint64_t>::max();
  }
  fixed::Signed256 positive_one{};
  positive_one.magnitude.limb[0] = 1U;
  fixed::Signed256 sum{};
  check(
      !fixed::add_signed(maximum, positive_one, sum),
      "a same-sign 256-bit overflow must fail open");

  std::uint64_t product_low = 0U;
  std::uint64_t product_high = 0U;
  fixed::multiply_u64(
      std::numeric_limits<std::uint64_t>::max(),
      std::numeric_limits<std::uint64_t>::max(),
      product_low,
      product_high);
  check(
      product_low == 1U &&
          product_high == std::numeric_limits<std::uint64_t>::max() - 1U,
      "the all-ones 64-bit product must propagate every cross carry");

  fixed::UInt128 all_ones{};
  all_ones.limb[0] = std::numeric_limits<std::uint64_t>::max();
  all_ones.limb[1] = std::numeric_limits<std::uint64_t>::max();
  fixed::UInt256 square{};
  check(
      fixed::multiply(all_ones, all_ones, square) &&
          square.limb[0] == 1U && square.limb[1] == 0U &&
          square.limb[2] ==
              std::numeric_limits<std::uint64_t>::max() - 1U &&
          square.limb[3] == std::numeric_limits<std::uint64_t>::max(),
      "the all-ones 128-bit square must propagate its full carry chain");

  fixed::Signed256 negative_maximum = maximum;
  negative_maximum.negative = true;
  check(
      fixed::add_signed(maximum, negative_maximum, sum) &&
          fixed::is_zero(sum.magnitude) && !sum.negative,
      "opposite all-ones magnitudes must cancel to canonical zero");
}

[[nodiscard]] ExactDiametralPhiInput overflow_input(std::uint64_t replay_id) {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t internal_exponent_75 = 1150U;
  const std::uint64_t high =
      (internal_exponent_75 << 52U) | fraction_mask;
  const std::uint64_t negative_one = bits(-1.0);
  return ExactDiametralPhiInput{
      replay_id,
      {high, high, 0U},
      {negative_one, negative_one, 0U},
      {negative_one, negative_one, 0U}};
}

void test_exact_fixtures() {
  const std::uint64_t minimum_subnormal = 1U;
  const std::uint64_t twice_minimum_subnormal = 2U;
  const ExactDiametralPhiInput negative =
      input(1U, point(0.0, 0.0, 0.0), point(-1.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0));
  const ExactDiametralPhiInput positive =
      input(2U, point(2.0, 0.0, 0.0), point(-1.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0));
  const ExactDiametralPhiInput shell =
      input(3U, point(3.0, -2.0, 7.0), point(3.0, -2.0, 7.0),
            point(-4.0, 5.0, 9.0));
  const ExactDiametralPhiInput cancellation =
      input(4U, point(2.0, 2.0, 0.0), point(1.0, 1.0, 1.0),
            point(1.0, 1.0, -2.0));
  const ExactDiametralPhiInput signed_zero = ExactDiametralPhiInput{
      5U,
      {UINT64_C(0x8000000000000000), 0U, 0U},
      {0U, UINT64_C(0x8000000000000000), 0U},
      {0U, 0U, UINT64_C(0x8000000000000000)}};
  const ExactDiametralPhiInput subnormal = ExactDiametralPhiInput{
      6U,
      {minimum_subnormal, 0U, 0U},
      {0U, 0U, 0U},
      {twice_minimum_subnormal, 0U, 0U}};
  const ExactDiametralPhiInput exponent_span = ExactDiametralPhiInput{
      7U,
      {UINT64_C(0x7fefffffffffffff), 0U, 0U},
      {minimum_subnormal, 0U, 0U},
      {0U, 0U, 0U}};
  ExactDiametralPhiInput nonfinite = negative;
  nonfinite.witness_bits[0] = UINT64_C(0x7ff0000000000000);

  check(
      fixed_sign(negative) == fixed::ResultCode::negative,
      "the strict interior fixture must be negative");
  check(
      fixed_sign(positive) == fixed::ResultCode::positive,
      "the exterior fixture must be positive");
  check(
      fixed_sign(shell) == fixed::ResultCode::zero,
      "a support endpoint must be exact shell zero");
  check(
      fixed_sign(cancellation) == fixed::ResultCode::zero,
      "three-axis cancellation must be exact zero");
  check(
      fixed_sign(signed_zero) == fixed::ResultCode::zero,
      "all signed-zero spellings must canonicalize to exact zero");
  check(
      fixed_sign(subnormal) == fixed::ResultCode::negative,
      "minimum subnormals must remain exact");
  check(
      fixed_sign(exponent_span) ==
          fixed::ResultCode::requires_cpu_fallback,
      "a max-finite/min-subnormal span must fail open");
  check(
      fixed_sign(nonfinite) == fixed::ResultCode::invalid_input,
      "the internal fixed-limb contract must identify nonfinite input");
  const ExactDiametralPhiInput overflow = overflow_input(8U);
  check(
      fixed_sign(overflow) == fixed::ResultCode::requires_cpu_fallback,
      "a same-sign multi-axis accumulator overflow must fail open");

  const std::array fixtures{
      negative, positive, shell, cancellation, signed_zero, subnormal};
  for (const ExactDiametralPhiInput& fixture : fixtures) {
    check(
        predicate_sign(fixed_sign(fixture)) == oracle_sign(fixture),
        "each fixed fixture must agree with BigInt");
  }
  check(
      oracle_sign(exponent_span) == PredicateSign::positive &&
          oracle_sign(overflow) == PredicateSign::positive,
      "fallback fixtures must retain their BigInt signs");
}

void test_random_differential() {
  std::uint64_t state = UINT64_C(0x42d1a6f3789bc05e);
  std::size_t fixed_count = 0U;
  std::size_t fallback_count = 0U;
  for (std::size_t index = 0U; index < 4096U; ++index) {
    ExactDiametralPhiInput value{};
    value.replay_id = static_cast<std::uint64_t>(index);
    const bool bounded = index < 2048U;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      value.witness_bits[axis] = finite_word(next_random(state), bounded);
      value.first_support_bits[axis] = finite_word(next_random(state), bounded);
      value.second_support_bits[axis] = finite_word(next_random(state), bounded);
    }
    const fixed::ResultCode result = fixed_sign(value);
    if (result == fixed::ResultCode::requires_cpu_fallback) {
      ++fallback_count;
      continue;
    }
    check(
        result != fixed::ResultCode::invalid_input,
        "finite random words must never become invalid");
    if (result != fixed::ResultCode::invalid_input) {
      ++fixed_count;
      check(
          predicate_sign(result) == oracle_sign(value),
          "every decided random fixed-limb sign must agree with BigInt");
    }
  }
  check(fixed_count >= 1024U, "the random differential must exercise fixed limbs");
  check(fallback_count != 0U, "the random differential must exercise fallback");
}

void test_context_and_fallback_queue() {
  using namespace morsehgp3d::gpu::test_support;
  const ExactDiametralPhiInput interval =
      input(exact_phi_force_interval_replay_id,
            point(0.0, 0.0, 0.0), point(-1.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0));
  const ExactDiametralPhiInput zero =
      input(10U, point(1.0, 2.0, 3.0), point(1.0, 2.0, 3.0),
            point(4.0, 5.0, 6.0));
  const ExactDiametralPhiInput fallback =
      input(exact_phi_force_fallback_replay_id,
            point(2.0, 0.0, 0.0), point(-1.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0));
  const std::array batch{interval, zero, fallback};
  ExactDiametralPhiContext context{batch.size()};
  const auto result = context.decide(
      std::span<const ExactDiametralPhiInput>{batch},
      ExactDiametralPhiBatchOptions{true});
  check(result.decisions.size() == 3U, "the batch must preserve cardinality");
  check(
      result.decisions[0].sign == PredicateSign::negative &&
          result.decisions[0].certification_stage ==
              ExactDiametralPhiCertificationStage::gpu_fp64_interval,
      "the fake strict interval sign must remain negative");
  check(
      result.decisions[1].sign == PredicateSign::zero &&
          result.decisions[1].certification_stage ==
              ExactDiametralPhiCertificationStage::gpu_fixed_limb_dyadic,
      "zero must be certified by exact limbs, never the interval");
  check(
      result.decisions[2].sign == PredicateSign::positive &&
          result.decisions[2].certification_stage ==
              ExactDiametralPhiCertificationStage::cpu_multiprecision,
      "a device unknown must be resolved exactly by CPU fallback");
  check(
      result.audit.gpu_fp64_interval_count == 1U &&
          result.audit.gpu_fixed_limb_dyadic_count == 1U &&
          result.audit.gpu_exact_fallback_count == 1U &&
          result.audit.cpu_multiprecision_count == 1U &&
          result.audit.exact_zero_count == 1U &&
          result.audit.gpu_known_audited_count == 2U &&
          result.audit.disagreement_count == 0U,
      "the exact stage and audit partition must close");
  check(
      result.audit.host_fake_launcher_exercised &&
          !result.audit.cuda_execution_performed &&
          result.audit.all_results_exact &&
          result.audit.interval_zero_certification_forbidden &&
          !result.audit.epsilon_used &&
          !result.audit.scientific_catalog_decision_published &&
          !result.audit.public_status_claimed,
      "the fake audit must not forge CUDA, catalog or public authority");

  const auto empty = context.decide({});
  check(
      empty.decisions.empty() && empty.audit.all_results_exact &&
          empty.audit.launcher_call_count == 0U,
      "an empty batch must be exact without launching");
}

void test_fail_closed_contracts() {
  using namespace morsehgp3d::gpu::test_support;
  check_throws<std::invalid_argument>(
      [] { ExactDiametralPhiContext invalid{0U}; },
      "a zero-capacity context must be rejected");

  const ExactDiametralPhiInput ordinary =
      input(20U, point(0.0, 0.0, 0.0), point(-1.0, 0.0, 0.0),
            point(1.0, 0.0, 0.0));
  ExactDiametralPhiContext capacity{1U};
  const std::array too_many{ordinary, input(21U, ordinary.witness_bits,
                                            ordinary.first_support_bits,
                                            ordinary.second_support_bits)};
  check_throws<std::length_error>(
      [&] { static_cast<void>(capacity.decide(too_many)); },
      "a batch above fixed capacity must be rejected before launch");
  const std::array duplicates{ordinary, ordinary};
  ExactDiametralPhiContext duplicate_context{2U};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(duplicate_context.decide(duplicates)); },
      "duplicate replay identifiers must be rejected");
  ExactDiametralPhiInput nonfinite = ordinary;
  nonfinite.witness_bits[0] = UINT64_C(0x7ff0000000000000);
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(capacity.decide(
                std::span<const ExactDiametralPhiInput>{&nonfinite, 1U})); },
      "nonfinite words must be rejected before launch");

  auto corruption_input = [&](std::uint64_t replay_id) {
    return input(replay_id, point(0.0, 0.0, 0.0),
                 point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0));
  };
  for (const std::uint64_t replay_id :
       {exact_phi_invalid_stage_replay_id,
        exact_phi_reserved_byte_replay_id,
        exact_phi_bad_metadata_replay_id}) {
    ExactDiametralPhiContext corrupted{1U};
    const ExactDiametralPhiInput value = corruption_input(replay_id);
    check_throws<std::runtime_error>(
        [&] { static_cast<void>(corrupted.decide(
                  std::span<const ExactDiametralPhiInput>{&value, 1U})); },
        "malformed device output must fail closed");
    check_throws<std::runtime_error>(
        [&] { static_cast<void>(corrupted.decide(
                  std::span<const ExactDiametralPhiInput>{&ordinary, 1U})); },
        "a post-publication validation failure must poison the context");
  }

  ExactDiametralPhiContext wrong_sign_context{1U};
  const ExactDiametralPhiInput wrong_sign =
      corruption_input(exact_phi_wrong_sign_replay_id);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(wrong_sign_context.decide(
            std::span<const ExactDiametralPhiInput>{&wrong_sign, 1U},
            ExactDiametralPhiBatchOptions{true}));
      },
      "qualification audit must reject a contradictory GPU sign");

  ExactDiametralPhiContext poison_context{1U};
  const ExactDiametralPhiInput poison =
      corruption_input(exact_phi_poison_replay_id);
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(poison_context.decide(
                std::span<const ExactDiametralPhiInput>{&poison, 1U})); },
      "a launcher failure must propagate");
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(poison_context.decide(
                std::span<const ExactDiametralPhiInput>{&ordinary, 1U})); },
      "a launcher failure must poison the context");

  ExactDiametralPhiContext moved{1U};
  ExactDiametralPhiContext destination{std::move(moved)};
  static_cast<void>(destination.decide(
      std::span<const ExactDiametralPhiInput>{&ordinary, 1U}));
  check_throws<std::logic_error>(
      [&] { static_cast<void>(moved.decide(
                std::span<const ExactDiametralPhiInput>{&ordinary, 1U})); },
      "a moved-from context must be revoked");
}

void test_concurrent_poison_rejects_waiting_publication() {
  using namespace morsehgp3d::gpu::test_support;
  const ExactDiametralPhiInput wrong_sign = input(
      exact_phi_concurrent_wrong_sign_replay_id,
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0));
  const ExactDiametralPhiInput ordinary = input(
      30U,
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0));
  ExactDiametralPhiContext context{1U};
  std::atomic<bool> first_rejected{false};
  std::atomic<bool> second_rejected{false};

  std::thread first{[&] {
    try {
      static_cast<void>(context.decide(
          std::span<const ExactDiametralPhiInput>{&wrong_sign, 1U},
          ExactDiametralPhiBatchOptions{true}));
    } catch (const std::runtime_error&) {
      first_rejected.store(true, std::memory_order_release);
    }
  }};

  const auto entry_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (!exact_phi_concurrent_fixture_entered() &&
         std::chrono::steady_clock::now() < entry_deadline) {
    std::this_thread::yield();
  }
  check(
      exact_phi_concurrent_fixture_entered(),
      "the first concurrent call must enter the serialized transaction");

  std::thread second{[&] {
    try {
      static_cast<void>(context.decide(
          std::span<const ExactDiametralPhiInput>{&ordinary, 1U}));
    } catch (const std::runtime_error&) {
      second_rejected.store(true, std::memory_order_release);
    }
  }};
  first.join();
  second.join();

  check(
      first_rejected.load(std::memory_order_acquire),
      "the audited contradictory publication must be rejected");
  check(
      second_rejected.load(std::memory_order_acquire),
      "a waiting call must observe poison and publish nothing");
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(context.decide(
            std::span<const ExactDiametralPhiInput>{&ordinary, 1U}));
      },
      "the context must remain poisoned after the concurrent contradiction");
}

}  // namespace

int main() {
  test_low_level_boundaries();
  test_exact_fixtures();
  test_random_differential();
  test_context_and_fallback_queue();
  test_fail_closed_contracts();
  test_concurrent_poison_rejects_waiting_publication();
  if (failures != 0) {
    std::cerr << failures << " exact diametral-Phi test(s) failed\n";
    return 1;
  }
  return 0;
}
