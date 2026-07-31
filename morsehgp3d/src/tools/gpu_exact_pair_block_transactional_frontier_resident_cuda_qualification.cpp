#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
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
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

struct Options {
  std::size_t point_count{};
  std::size_t maximum_order{};
  bool require_complete{false};
  bool semantic_line_fixture{false};
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
    if (argument == "--semantic-line-fixture") {
      options.semantic_line_fixture = true;
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
      (options.maximum_order != 5U && options.maximum_order != 10U) ||
      (options.semantic_line_fixture && options.point_count != 16U)) {
    throw std::invalid_argument(
        "qualification requires point-count >= 2, K equal to 5 or 10, and "
        "the semantic line fixture requires exactly 16 points");
  }
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> semantic_line_points(
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate, coordinate / 16.0, coordinate / 256.0));
  }
  return points;
}

[[nodiscard]] std::size_t node_for_range(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const auto probe = morsehgp3d::hierarchy::
        certify_exact_block_rank_prune_receipt(
            index,
            cloud,
            node_index,
            node_index,
            std::span<const std::size_t>{},
            2U);
    if (probe.status() == morsehgp3d::hierarchy::
                              ExactBlockRankPruneStatus::
                                  rejected_support_overlap &&
        probe.audit().support_nodes[0].leaf_begin == leaf_begin &&
        probe.audit().support_nodes[0].leaf_end == leaf_end) {
      return node_index;
    }
  }
  throw std::runtime_error(
      "the semantic qualification LBVH range was not found");
}

[[nodiscard]] std::vector<
    ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>
semantic_line_recipes(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order) {
  const std::size_t first_support = node_for_range(index, cloud, 0U, 2U);
  const std::size_t second_support = node_for_range(index, cloud, 12U, 14U);
  ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe certified;
  certified.source = {
      morsehgp3d::gpu::ExactPairBlockAuthorityKind::cross,
      {first_support, 0U, 2U},
      {second_support, 12U, 14U},
      4U};
  certified.witness_node_count = maximum_order;
  for (std::size_t witness_index = 0U;
       witness_index < certified.witness_node_count;
       ++witness_index) {
    certified.witness_node_indices[witness_index] = node_for_range(
        index,
        cloud,
        witness_index + 2U,
        witness_index + 3U);
  }

  ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe fail_open;
  fail_open.source = {
      morsehgp3d::gpu::ExactPairBlockAuthorityKind::cross,
      {node_for_range(index, cloud, 0U, 8U), 0U, 8U},
      {node_for_range(index, cloud, 8U, 16U), 8U, 16U},
      64U};
  return {certified, fail_open};
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
  const std::vector<CertifiedPoint3> points = options.semantic_line_fixture
      ? semantic_line_points(options.point_count)
      : uniform_latin_points(options.point_count);
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
  const std::vector<
      ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>
      recipes = options.semantic_line_fixture
      ? semantic_line_recipes(index, cloud, options.maximum_order)
      : std::vector<
            ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>{};
  const Clock::time_point scheduler_begin = Clock::now();
  auto result = scheduler.run(recipes);
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
      audit.synchronization_count == 3U && mass_conserved &&
      (complete_replay || rollback_contract);
  const bool semantic_recipe_contract =
      audit.recipe_catalog_nonempty && audit.recipe_path_exercised &&
      audit.certified_prune_path_exercised &&
      audit.fail_open_path_exercised &&
      audit.fail_open_native_transition_validated;
  // A rollback is a useful runtime/capacity smoke, never a terminal
  // scientific qualification.  Keep the historical flag in the report for
  // invocation provenance, but do not let its absence promote an incomplete
  // cut.
  const bool qualified =
      runtime_contract && complete_replay && semantic_recipe_contract;

  std::cout
      << "{\"schema\":\"morsehgp3d.phase15.resident_transactional_"
         "frontier_qualification.v1\","
      << "\"backend\":\"cuda_g4\","
      << "\"profile\":\"hgp_reduced\","
      << "\"mode\":\"resident_transactional_pair_partition\","
      << "\"public_status\":\"not_claimed\","
      << "\"point_count\":" << options.point_count << ','
      << "\"maximum_order\":" << options.maximum_order << ','
      << "\"fixture\":\""
      << (options.semantic_line_fixture ? "semantic_line_16"
                                        : "uniform_latin")
      << "\","
      << "\"maximum_closed_rank\":"
      << audit.maximum_closed_rank << ','
      << "\"require_complete\":"
      << (options.require_complete ? "true" : "false") << ','
      << "\"status\":\"" << status_text(result.status()) << "\","
      << "\"qualified_runtime_contract\":"
      << (runtime_contract ? "true" : "false") << ','
      << "\"runtime_smoke_passed\":"
      << (runtime_contract ? "true" : "false") << ','
      << "\"rollback_contract_passed\":"
      << (rollback_contract ? "true" : "false") << ','
      << "\"semantic_recipe_contract_passed\":"
      << (semantic_recipe_contract ? "true" : "false") << ','
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
      << ",\"submitted_prune_recipes\":"
      << audit.submitted_prune_recipe_count
      << ",\"matched_prune_recipes\":"
      << audit.matched_prune_recipe_count
      << ",\"exact_prune_attempts\":"
      << audit.exact_prune_attempt_count
      << ",\"certified_prunes\":" << audit.certified_prune_count
      << ",\"fail_open_prunes\":"
      << audit.exact_prune_fail_open_count
      << "},"
      << "\"cuda\":{\"kernel_launches\":" << audit.kernel_launch_count
      << ",\"synchronizations\":" << audit.synchronization_count
      << ",\"intermediate_control_readbacks\":"
      << audit.intermediate_control_readback_count
      << ",\"kernel_elapsed_nanoseconds\":"
      << audit.kernel_elapsed_nanoseconds
      << ",\"device_arena_bytes\":" << audit.device_arena_byte_count
      << ",\"serial_device_reference\":"
      << (audit.serial_device_reference ? "true" : "false")
      << ",\"scale_eligible\":"
      << (audit.scale_eligible ? "true" : "false")
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
