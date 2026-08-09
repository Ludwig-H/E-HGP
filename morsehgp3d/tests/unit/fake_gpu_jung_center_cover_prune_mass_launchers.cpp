#include "fake_gpu_jung_center_cover_prune_mass_launchers.hpp"

#include "phase15_jung_center_cover_prune_mass_internal.hpp"

#include <atomic>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::test_support {
namespace {

std::mutex fake_mutex;
FakeJungCenterCoverConfiguration fake_configuration;
std::atomic<std::size_t> fake_launch_count{};

}  // namespace

void configure_fake_gpu_jung_center_cover_prune_mass(
    FakeJungCenterCoverConfiguration configuration) {
  std::lock_guard lock{fake_mutex};
  fake_configuration = std::move(configuration);
}

void reset_fake_gpu_jung_center_cover_prune_mass() noexcept {
  std::lock_guard lock{fake_mutex};
  fake_configuration = {};
  fake_launch_count.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_jung_center_cover_prune_mass_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

[[nodiscard]] std::uint64_t fake_pair_mass(std::size_t point_count) {
  const std::uint64_t n = static_cast<std::uint64_t>(point_count);
  return (n & 1U) == 0U ? (n / 2U) * (n - 1U)
                        : n * ((n - 1U) / 2U);
}

}  // namespace

Phase15JungCenterCoverPruneMassReceipt
launch_phase15_jung_center_cover_prune_mass(
    const Phase15JungCenterCoverPruneMassRequest& request) {
  if (!request.traversal.host_fake || request.traversal.cuda_device != -1 ||
      request.traversal.device_coordinate_bits != nullptr ||
      request.traversal.device_morton_point_ids != nullptr ||
      request.traversal.device_nodes != nullptr) {
    throw std::invalid_argument(
        "the fake Jung center-cover launcher rejected a native/mixed request");
  }
  test_support::FakeJungCenterCoverConfiguration configuration;
  {
    std::lock_guard lock{test_support::fake_mutex};
    configuration = test_support::fake_configuration;
  }
  test_support::fake_launch_count.fetch_add(1U, std::memory_order_relaxed);

  Phase15JungCenterCoverPruneMassReceipt result;
  const std::uint64_t universe = fake_pair_mass(request.traversal.point_count);
  auto& audit = result.audit;
  audit.source_snapshot_epoch = request.traversal.source_snapshot_epoch;
  audit.point_count = request.traversal.point_count;
  audit.certified_node_count = request.traversal.certified_node_count;
  audit.requested_shard_count = request.config.requested_shard_count;
  audit.generated_shard_count = request.config.requested_shard_count;
  audit.worker_cta_count = 2U;
  audit.worker_cta_with_work_count = 2U;
  audit.stack_capacity_per_shard = request.config.stack_capacity_per_shard;
  audit.maximum_stack_high_water = 3U;
  audit.endpoint_microtile_pair_mass =
      request.config.endpoint_microtile_pair_mass;
  audit.unordered_pair_universe_mass = universe;
  audit.pruned_pair_mass = configuration.pruned_pair_mass;
  audit.microtile_pair_mass = configuration.pruned_pair_mass <= universe
      ? universe - configuration.pruned_pair_mass
      : 0U;
  audit.endpoint_block_visit_count = 7U;
  audit.center_cover_attempt_count = 7U;
  audit.pruned_block_count = configuration.pruned_pair_mass == 0U ? 0U : 1U;
  audit.microtile_block_count = 6U;
  audit.patch_evaluation_count = 7U * jung_center_cover_patch_count;
  audit.covered_patch_count = 2U;
  audit.unresolved_patch_count =
      audit.patch_evaluation_count - audit.covered_patch_count;
  audit.witness_node_visit_count = 91U;
  audit.accepted_witness_node_count = 4U;
  audit.accepted_witness_point_mass = 9U;
  audit.qualification_receipt_count = configuration.receipts.size();
  audit.qualification_receipt_capacity =
      request.config.collect_n32_qualification_receipts ? universe : 0U;
  audit.maximum_shard_block_visits = 4U;
  audit.maximum_shard_witness_node_visits = 55U;
  audit.shard_block_visit_histogram[2U] =
      request.config.requested_shard_count;
  audit.shard_witness_visit_histogram[5U] =
      request.config.requested_shard_count;
  audit.physical_device_arena_byte_count = 4096U;
  audit.source_device_byte_count =
      request.traversal.persistent_device_byte_capacity;
  audit.device_allocation_count = 5U;
  audit.device_memset_count = 3U;
  audit.cuda_kernel_launch_count = 3U;
  audit.cuda_synchronization_count = 1U;
  audit.device_to_host_byte_count = sizeof(JungCenterCoverPruneMassAudit);
  audit.shard_source_device_nanoseconds = 100U;
  audit.center_cover_device_nanoseconds = 200U;
  audit.finalize_device_nanoseconds = 30U;
  audit.source_cover_device_nanoseconds = 330U;
  audit.launcher_wall_nanoseconds = 500U;
  audit.source_identity_authenticated = true;
  audit.source_epoch_authenticated = request.traversal.source_snapshot_epoch != 0U;
  audit.support4_only = true;
  audit.fixed_64_closed_patches_used = true;
  audit.directed_binary64_intervals_used = true;
  audit.strict_prune_only = true;
  audit.fail_open_on_ambiguity = true;
  audit.multi_cta_scheduler_used = audit.worker_cta_with_work_count >= 2U;
  audit.private_per_shard_dfs_used = true;
  audit.exact_mass_identity_satisfied =
      configuration.pruned_pair_mass <= universe;
  audit.one_terminal_synchronization_used = true;
  audit.qualification_receipts_collected =
      request.config.collect_n32_qualification_receipts;
  audit.qualification_patch_bounds_recorded =
      request.config.collect_n32_qualification_receipts;
  audit.aggregate_only_50k_path =
      request.traversal.point_count == 50'000U &&
      !request.config.collect_n32_qualification_receipts;
  audit.anchor_or_pair_payload_emitted = false;
  audit.endpoint_pair_array_materialized = false;
  audit.higher_order_delaunay_mosaic_materialized = false;
  audit.scientific_decision_published = false;
  audit.scientific_result_claimed = false;
  audit.component_only = true;
  audit.slo_eligible = false;
  audit.warm_e2e_slo_claimed = false;
  audit.anchor_completeness_claimed = false;
  audit.host_fake_launcher_exercised = true;
  audit.cuda_execution_performed = false;
  audit.public_status_claimed = false;
  result.qualification_receipts = std::move(configuration.receipts);

  std::uint64_t receipt_pruned_mass = 0U;
  std::uint64_t receipt_microtile_mass = 0U;
  bool receipt_mass_overflow = false;
  bool receipt_schema_valid = true;
  for (const auto& receipt : result.qualification_receipts) {
    const bool disposition_valid =
        receipt.disposition == JungCenterCoverTerminalDisposition::pruned ||
        receipt.disposition == JungCenterCoverTerminalDisposition::microtile;
    const bool kind_valid =
        receipt.block.kind == JungCenterCoverEndpointBlockKind::diagonal ||
        receipt.block.kind == JungCenterCoverEndpointBlockKind::cross;
    const bool microtile_threshold_valid =
        receipt.disposition != JungCenterCoverTerminalDisposition::microtile ||
        receipt.block.unordered_pair_mass <=
            request.config.endpoint_microtile_pair_mass;
    receipt_schema_valid = receipt_schema_valid && disposition_valid &&
        kind_valid && microtile_threshold_valid;
    if (!disposition_valid) {
      continue;
    }
    auto& destination =
        receipt.disposition == JungCenterCoverTerminalDisposition::pruned
        ? receipt_pruned_mass
        : receipt_microtile_mass;
    if (receipt.block.unordered_pair_mass > UINT64_MAX - destination) {
      receipt_mass_overflow = true;
    } else {
      destination += receipt.block.unordered_pair_mass;
    }
  }
  const bool receipt_partition_valid =
      !request.config.collect_n32_qualification_receipts ||
      (!receipt_mass_overflow &&
       receipt_schema_valid &&
       configuration.corruption !=
           test_support::FakeJungCenterCoverCorruption::
               missing_qualification_receipt &&
       receipt_pruned_mass == audit.pruned_pair_mass &&
       receipt_microtile_mass == audit.microtile_pair_mass &&
       receipt_pruned_mass <= universe &&
       receipt_microtile_mass == universe - receipt_pruned_mass);

  if (configuration.corruption ==
      test_support::FakeJungCenterCoverCorruption::stack_capacity_exhausted) {
    result.status = JungCenterCoverPruneMassStatus::stack_capacity_exhausted;
    ++audit.stack_capacity_failure_count;
    audit.exact_mass_identity_satisfied = false;
  } else if (configuration.corruption ==
                 test_support::FakeJungCenterCoverCorruption::execution_failure ||
             configuration.corruption ==
                 test_support::FakeJungCenterCoverCorruption::mass_mismatch ||
             configuration.pruned_pair_mass > universe ||
             !receipt_partition_valid) {
    result.status = JungCenterCoverPruneMassStatus::execution_failure;
    audit.exact_mass_identity_satisfied = false;
    if (configuration.corruption ==
        test_support::FakeJungCenterCoverCorruption::mass_mismatch) {
      ++audit.microtile_pair_mass;
    }
  } else {
    result.status = JungCenterCoverPruneMassStatus::complete;
  }
  if (result.status != JungCenterCoverPruneMassStatus::complete) {
    result.qualification_receipts.clear();
    audit.qualification_receipts_collected = false;
    audit.qualification_patch_bounds_recorded = false;
  }
  return result;
}

}  // namespace morsehgp3d::gpu::detail
