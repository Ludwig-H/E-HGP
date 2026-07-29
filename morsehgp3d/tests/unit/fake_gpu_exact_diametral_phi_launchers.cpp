#include "fake_gpu_exact_diametral_phi_launchers.hpp"

#include "phase15_exact_diametral_phi_fixed.cuh"
#include "phase15_exact_diametral_phi_internal.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <thread>

namespace {

std::atomic<bool> concurrent_fixture_entered{false};

}  // namespace

namespace morsehgp3d::gpu::test_support {

bool exact_phi_concurrent_fixture_entered() noexcept {
  return ::concurrent_fixture_entered.load(std::memory_order_acquire);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

[[nodiscard]] ExactDiametralPhiDeviceSign decode_fixed_sign(
    exact_phi_fixed::ResultCode code) {
  switch (code) {
    case exact_phi_fixed::ResultCode::negative:
      return ExactDiametralPhiDeviceSign::negative;
    case exact_phi_fixed::ResultCode::zero:
      return ExactDiametralPhiDeviceSign::zero;
    case exact_phi_fixed::ResultCode::positive:
      return ExactDiametralPhiDeviceSign::positive;
    case exact_phi_fixed::ResultCode::requires_cpu_fallback:
      return ExactDiametralPhiDeviceSign::unresolved;
    case exact_phi_fixed::ResultCode::invalid_input:
      return ExactDiametralPhiDeviceSign::invalid;
  }
  return ExactDiametralPhiDeviceSign::invalid;
}

[[nodiscard]] ExactDiametralPhiDeviceRecord make_record(
    const ExactDiametralPhiInput& input,
    std::size_t query_index) {
  ExactDiametralPhiDeviceRecord record{};
  record.query_index = static_cast<std::uint64_t>(query_index);
  const exact_phi_fixed::ResultCode code = exact_phi_fixed::exact_sign(
      input.witness_bits.data(),
      input.first_support_bits.data(),
      input.second_support_bits.data());
  record.sign = decode_fixed_sign(code);
  record.stage = code == exact_phi_fixed::ResultCode::requires_cpu_fallback
                     ? ExactDiametralPhiDeviceStage::requires_cpu_fallback
                 : code == exact_phi_fixed::ResultCode::invalid_input
                     ? ExactDiametralPhiDeviceStage::invalid
                     : ExactDiametralPhiDeviceStage::fixed_limb_dyadic;

  if (input.replay_id == test_support::exact_phi_force_interval_replay_id) {
    if (record.sign == ExactDiametralPhiDeviceSign::zero ||
        record.sign == ExactDiametralPhiDeviceSign::unresolved ||
        record.sign == ExactDiametralPhiDeviceSign::invalid) {
      throw std::logic_error(
          "the fake interval fixture requires a strict exact sign");
    }
    record.stage = ExactDiametralPhiDeviceStage::fp64_interval;
  } else if (
      input.replay_id == test_support::exact_phi_force_fallback_replay_id) {
    record.sign = ExactDiametralPhiDeviceSign::unresolved;
    record.stage = ExactDiametralPhiDeviceStage::requires_cpu_fallback;
  } else if (
      input.replay_id == test_support::exact_phi_wrong_sign_replay_id ||
      input.replay_id ==
          test_support::exact_phi_concurrent_wrong_sign_replay_id) {
    record.sign = record.sign == ExactDiametralPhiDeviceSign::negative
                      ? ExactDiametralPhiDeviceSign::positive
                      : ExactDiametralPhiDeviceSign::negative;
    record.stage = ExactDiametralPhiDeviceStage::fixed_limb_dyadic;
  } else if (
      input.replay_id == test_support::exact_phi_invalid_stage_replay_id) {
    record.sign = ExactDiametralPhiDeviceSign::invalid;
    record.stage = ExactDiametralPhiDeviceStage::invalid;
  } else if (
      input.replay_id == test_support::exact_phi_reserved_byte_replay_id) {
    record.reserved[0] = 1U;
  }
  return record;
}

}  // namespace

ExactDiametralPhiDeviceBatch decide_exact_diametral_phi_on_gpu(
    ExactDiametralPhiContextState& context,
    std::span<const ExactDiametralPhiInput> inputs,
    std::size_t maximum_query_count) {
  if (!inputs.empty() &&
      inputs.front().replay_id == test_support::exact_phi_poison_replay_id) {
    throw std::runtime_error("simulated exact diametral-Phi GPU failure");
  }
  if (!inputs.empty() &&
      inputs.front().replay_id ==
          test_support::exact_phi_concurrent_wrong_sign_replay_id) {
    concurrent_fixture_entered.store(true, std::memory_order_release);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (context.waiting_call_count() == 0U) {
      if (std::chrono::steady_clock::now() >= deadline) {
        throw std::runtime_error(
            "the concurrent exact diametral-Phi fixture observed no waiter");
      }
      std::this_thread::yield();
    }
  }
  ExactDiametralPhiDeviceBatch batch;
  batch.records.reserve(inputs.size());
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    batch.records.push_back(make_record(inputs[index], index));
  }
  batch.maximum_query_count = maximum_query_count;
  batch.launcher_call_count = 1U;
  batch.execution_kind = ExactDiametralPhiExecutionKind::host_fake;
  batch.exact_fixed_limb_implemented = true;
  if (!inputs.empty() &&
      inputs.front().replay_id ==
          test_support::exact_phi_bad_metadata_replay_id) {
    batch.kernel_launch_count = 1U;
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
