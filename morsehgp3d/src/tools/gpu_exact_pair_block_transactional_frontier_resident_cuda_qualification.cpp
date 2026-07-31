#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::spatial::CanonicalPointCloud;

struct Options {
  std::size_t point_count{};
  std::size_t maximum_order{};
  bool require_complete{false};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  bool point_count_seen = false;
  bool maximum_order_seen = false;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--require-complete") {
      options.require_complete = true;
      continue;
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          "usage: --point-count N --K 5|10 [--require-complete]");
    }
    const std::string_view value{argv[++index]};
    if (argument == "--point-count" && !point_count_seen) {
      options.point_count = parse_size(value, "invalid --point-count");
      point_count_seen = true;
    } else if (
        (argument == "--K" || argument == "--maximum-order") &&
        !maximum_order_seen) {
      options.maximum_order = parse_size(value, "invalid --K");
      maximum_order_seen = true;
    } else {
      throw std::invalid_argument(
          "usage: --point-count N --K 5|10 [--require-complete]");
    }
  }
  if (!point_count_seen || !maximum_order_seen ||
      options.point_count < 2U ||
      (options.maximum_order != 5U && options.maximum_order != 10U)) {
    throw std::invalid_argument(
        "qualification requires point-count >= 2 and K equal to 5 or 10");
  }
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> uniform_latin_points(
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  const double denominator = static_cast<double>(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::uint64_t index64 = static_cast<std::uint64_t>(index);
    const std::uint64_t count64 =
        static_cast<std::uint64_t>(point_count);
    const std::uint64_t y_index =
        (index64 * UINT64_C(48271) + UINT64_C(17)) % count64;
    const std::uint64_t z_index =
        (index64 * UINT64_C(69621) + UINT64_C(31)) % count64;
    points.push_back(CertifiedPoint3::from_binary64(
        (static_cast<double>(index) + 0.5) / denominator,
        (static_cast<double>(y_index) + 0.5) / denominator,
        (static_cast<double>(z_index) + 0.5) / denominator));
  }
  return points;
}

[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    Clock::time_point begin,
    Clock::time_point end) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());
}

[[nodiscard]] std::string_view status_text(
    ExactPairBlockTransactionalFrontierResidentCudaStatus status) {
  switch (status) {
    case ExactPairBlockTransactionalFrontierResidentCudaStatus::
        qualified_cuda_terminal_authority:
      return "qualified_cuda_terminal_authority";
    case ExactPairBlockTransactionalFrontierResidentCudaStatus::
        non_authoritative_host_fake_terminal:
      return "non_authoritative_host_fake_terminal";
    case ExactPairBlockTransactionalFrontierResidentCudaStatus::
        capacity_exhausted_wave_rolled_back:
      return "capacity_exhausted_wave_rolled_back";
    case ExactPairBlockTransactionalFrontierResidentCudaStatus::
        preflight_invalid_recipe:
      return "preflight_invalid_recipe";
    case ExactPairBlockTransactionalFrontierResidentCudaStatus::
        arithmetic_capacity_rejected:
      return "arithmetic_capacity_rejected";
  }
  return "unknown";
}

int run(const Options& options) {
  const Clock::time_point total_begin = Clock::now();
  const Clock::time_point generation_begin = total_begin;
  const std::vector<CertifiedPoint3> points =
      uniform_latin_points(options.point_count);
  const Clock::time_point generation_end = Clock::now();
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  const Clock::time_point canonical_end = Clock::now();

  MortonLbvhBuildContext builder{options.point_count};
  auto build = builder.build(cloud);
  const Clock::time_point build_end = Clock::now();
  if (!build.cuda_qualified_build()) {
    throw std::runtime_error(
        "the resident qualification requires a CUDA-certified LBVH");
  }
  const auto& index = build.certified_index();
  auto traversal = builder.release_device_traversal_lease(build);
  auto scheduler = ExactPairBlockTransactionalFrontierResidentCudaContext::
      start(
          std::move(traversal),
          index,
          cloud,
          ExactPairBlockTransactionalFrontierResidentCudaConfig{
              options.maximum_order + 1U, 16U, 8U, 16U, 4U});
  const Clock::time_point scheduler_begin = Clock::now();
  auto result = scheduler.run({});
  const Clock::time_point scheduler_end = Clock::now();

  const auto& audit = result.audit();
  const bool mass_conserved =
      audit.pending_unordered_pair_mass + audit.inflight_unordered_pair_mass +
              audit.pruned_unordered_pair_mass +
              audit.terminal_unordered_pair_mass ==
          audit.unordered_pair_universe_mass;
  const bool complete = result.complete();
  const bool complete_replay = complete && result.validated_for(index, cloud);
  const bool rollback_contract =
      result.status() ==
          ExactPairBlockTransactionalFrontierResidentCudaStatus::
              capacity_exhausted_wave_rolled_back &&
      audit.capacity_wave_rollback_validated &&
      audit.inflight_unordered_pair_mass == 0U &&
      !result.pending_blocks().empty();
  const bool runtime_contract =
      audit.cuda_execution_performed &&
      audit.native_lbvh_authority_consumed &&
      audit.native_lbvh_nodes_read_on_device &&
      audit.resident_double_buffer_allocated_once &&
      audit.one_persistent_kernel_launch_validated &&
      audit.zero_intermediate_d2h_validated &&
      audit.kernel_launch_count == 1U &&
      audit.synchronization_count == 1U && mass_conserved &&
      (complete_replay || rollback_contract);
  const bool qualified = runtime_contract &&
      (!options.require_complete || complete_replay);

  std::cout
      << "{\"schema\":\"morsehgp3d.phase15.resident_transactional_"
         "frontier_qualification.v1\","
      << "\"backend\":\"cuda_g4\","
      << "\"profile\":\"hgp_reduced\","
      << "\"mode\":\"resident_transactional_pair_partition\","
      << "\"public_status\":\"not_claimed\","
      << "\"point_count\":" << options.point_count << ','
      << "\"maximum_order\":" << options.maximum_order << ','
      << "\"maximum_closed_rank\":"
      << audit.maximum_closed_rank << ','
      << "\"require_complete\":"
      << (options.require_complete ? "true" : "false") << ','
      << "\"status\":\"" << status_text(result.status()) << "\","
      << "\"qualified_runtime_contract\":"
      << (runtime_contract ? "true" : "false") << ','
      << "\"qualified\":" << (qualified ? "true" : "false") << ','
      << "\"global_pair_coverage_closed\":"
      << (audit.global_pair_coverage_closed ? "true" : "false") << ','
      << "\"terminal_authority_complete\":"
      << (complete_replay ? "true" : "false") << ','
      << "\"capacity_wave_rollback\":"
      << (rollback_contract ? "true" : "false") << ','
      << "\"mass\":{\"universe\":"
      << audit.unordered_pair_universe_mass
      << ",\"pending\":" << audit.pending_unordered_pair_mass
      << ",\"inflight\":" << audit.inflight_unordered_pair_mass
      << ",\"pruned\":" << audit.pruned_unordered_pair_mass
      << ",\"terminal\":" << audit.terminal_unordered_pair_mass
      << "},"
      << "\"counts\":{\"waves\":" << audit.wave_begin_count
      << ",\"commits\":" << audit.wave_commit_count
      << ",\"rollbacks\":" << audit.wave_rollback_count
      << ",\"pending_blocks\":" << audit.pending_block_count
      << ",\"terminal_pairs\":" << audit.terminal_pair_count
      << ",\"prune_receipts\":" << audit.prune_receipt_count
      << "},"
      << "\"cuda\":{\"kernel_launches\":" << audit.kernel_launch_count
      << ",\"synchronizations\":" << audit.synchronization_count
      << ",\"intermediate_control_readbacks\":"
      << audit.intermediate_control_readback_count
      << ",\"kernel_elapsed_nanoseconds\":"
      << audit.kernel_elapsed_nanoseconds
      << ",\"device_arena_bytes\":" << audit.device_arena_byte_count
      << "},"
      << "\"timings_nanoseconds\":{\"generation\":"
      << elapsed_nanoseconds(generation_begin, generation_end)
      << ",\"canonicalization\":"
      << elapsed_nanoseconds(generation_end, canonical_end)
      << ",\"lbvh_build\":"
      << elapsed_nanoseconds(canonical_end, build_end)
      << ",\"scheduler_wall\":"
      << elapsed_nanoseconds(scheduler_begin, scheduler_end)
      << ",\"total\":"
      << elapsed_nanoseconds(total_begin, scheduler_end)
      << "},"
      << "\"claims\":{\"pair_catalog_complete\":false,"
         "\"supports_3_4\":false,\"hierarchy_or_tree\":false,"
         "\"min_cluster_size_applied\":false,\"slo\":false},"
      << "\"qualified_scope\":\"pair_block_partition_scheduler_only\"}"
      << '\n';
  return qualified ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "resident qualification failed: " << error.what() << '\n';
    return 1;
  }
}
