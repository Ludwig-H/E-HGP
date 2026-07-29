#include "fake_gpu_morton_yao48_radial_subtree_filter_launchers.hpp"

#include "phase15_morton_yao48_radial_subtree_filter_internal.hpp"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {
std::mutex configuration_mutex;
std::vector<FakeMortonYao48RadialRecord> configured_records;
std::atomic<std::size_t> call_count{0U};
}

void configure_fake_morton_yao48_radial_records(
    std::vector<FakeMortonYao48RadialRecord> records) {
  std::lock_guard<std::mutex> lock{configuration_mutex};
  configured_records = std::move(records);
}

void reset_fake_morton_yao48_radial_records() noexcept {
  std::lock_guard<std::mutex> lock{configuration_mutex};
  configured_records.clear();
  call_count.store(0U, std::memory_order_relaxed);
}

std::size_t fake_morton_yao48_radial_call_count() noexcept {
  return call_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {

Phase15MortonYao48RadialDeviceBatch
propose_phase15_morton_yao48_radial_subtrees_on_gpu(
    Phase15MortonYao48RadialSubtreeFilterState&,
    std::span<const Phase15MortonYao48RadialDeviceInput> inputs,
    std::size_t maximum_query_count) {
  if (inputs.empty() || inputs.size() > maximum_query_count) {
    throw std::invalid_argument(
        "the fake radial-subtree launcher received invalid extents");
  }
  std::vector<test_support::FakeMortonYao48RadialRecord> configuration;
  {
    std::lock_guard<std::mutex> lock{
        test_support::configuration_mutex};
    configuration = test_support::configured_records;
  }
  if (!configuration.empty() && configuration.size() != inputs.size()) {
    throw std::invalid_argument(
        "the fake radial-subtree script has the wrong extent");
  }

  Phase15MortonYao48RadialDeviceBatch batch;
  batch.maximum_query_count = maximum_query_count;
  batch.launcher_call_count = 1U;
  batch.execution_kind =
      Phase15MortonYao48RadialExecutionKind::host_fake;
  batch.directed_round_down_aabb_implemented = true;
  batch.directed_round_up_three_radius_implemented = true;
  batch.equality_proposes_prune = true;
  batch.records.reserve(inputs.size());
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    const bool propose = !configuration.empty() &&
        configuration[index].propose_prune;
    const bool valid = !configuration.empty() &&
        configuration[index].directed_bounds_valid;
    batch.records.push_back(Phase15MortonYao48RadialDeviceRecord{
        inputs[index].replay_id,
        propose ? Phase15MortonYao48RadialProposal::prune
                : Phase15MortonYao48RadialProposal::descend,
        static_cast<std::uint8_t>(valid ? 1U : 0U),
        {}});
  }
  test_support::call_count.fetch_add(1U, std::memory_order_relaxed);
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
