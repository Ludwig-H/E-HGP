#include "fake_gpu_predicate_launchers.hpp"

#include "phase2b_distance_filter_internal.hpp"
#include "phase2b_orientation_filter_internal.hpp"
#include "phase2b_power_bisector_filter_internal.hpp"

#include <atomic>
#include <chrono>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<int> active_gpu_sections{0};
std::atomic<int> maximum_gpu_concurrency{0};
std::atomic<int> gpu_section_count{0};

void record_maximum_concurrency(int candidate) noexcept {
  int previous = maximum_gpu_concurrency.load(std::memory_order_relaxed);
  while (candidate > previous &&
         !maximum_gpu_concurrency.compare_exchange_weak(
             previous,
             candidate,
             std::memory_order_relaxed,
             std::memory_order_relaxed)) {
  }
}

template <typename Input>
[[nodiscard]] std::vector<FilterSign> simulate_gpu_filter(
    detail::PredicateFilterContextState& context,
    std::span<const Input> inputs) {
  if (inputs.empty()) {
    return {};
  }
  return context.with_gpu_section([&] {
    if (inputs.front().replay_id == poison_replay_id) {
      throw std::runtime_error("simulated Phase 2B GPU failure");
    }
    gpu_section_count.fetch_add(1, std::memory_order_relaxed);
    const int active =
        active_gpu_sections.fetch_add(1, std::memory_order_relaxed) + 1;
    record_maximum_concurrency(active);
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    active_gpu_sections.fetch_sub(1, std::memory_order_relaxed);
    std::vector<FilterSign> outputs(inputs.size(), FilterSign::unknown);
    if (inputs.front().replay_id == invalid_filter_sign_replay_id) {
      outputs.front() = static_cast<FilterSign>(2);
    }
    return outputs;
  });
}

}  // namespace

void reset_fake_gpu_counters() noexcept {
  active_gpu_sections.store(0, std::memory_order_relaxed);
  maximum_gpu_concurrency.store(0, std::memory_order_relaxed);
  gpu_section_count.store(0, std::memory_order_relaxed);
}

int fake_gpu_section_count() noexcept {
  return gpu_section_count.load(std::memory_order_relaxed);
}

int fake_gpu_maximum_concurrency() noexcept {
  return maximum_gpu_concurrency.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {

std::vector<FilterSign> filter_squared_distance_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const SquaredDistanceFilterInput> inputs) {
  return test_support::simulate_gpu_filter(context, inputs);
}

std::vector<FilterSign> filter_orientation_3d_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const Orientation3DFilterInput> inputs) {
  return test_support::simulate_gpu_filter(context, inputs);
}

std::vector<FilterSign> filter_power_bisector_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const PowerBisectorFilterInput> inputs) {
  return test_support::simulate_gpu_filter(context, inputs);
}

}  // namespace morsehgp3d::gpu::detail
