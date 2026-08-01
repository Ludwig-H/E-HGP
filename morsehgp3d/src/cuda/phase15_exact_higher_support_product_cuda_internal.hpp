#pragma once

#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::size_t
    phase15_exact_higher_support_product_axis_count = 3U;
inline constexpr std::size_t
    phase15_exact_higher_support_product_maximum_support_size = 4U;

// This is the single host/device transfer ABI.  It deliberately contains
// only fixed-width scalars and raw arrays: the CUDA translation unit reads
// this exact type after cudaMemcpy instead of type-punning a host std::array
// aggregate through a layout-compatible device mirror.
struct Phase15ExactHigherSupportProductCudaRawAabb3 {
  std::uint64_t lower[phase15_exact_higher_support_product_axis_count]{};
  std::uint64_t upper[phase15_exact_higher_support_product_axis_count]{};
};

static_assert(
    std::is_standard_layout_v<
        Phase15ExactHigherSupportProductCudaRawAabb3> &&
    std::is_trivially_copyable_v<
        Phase15ExactHigherSupportProductCudaRawAabb3>);

enum class Phase15ExactHigherSupportProductCudaArithmeticWidth :
    std::uint8_t {
  int512 = 0U,
  int1024 = 1U,
  int256 = 2U,
};

struct Phase15ExactHigherSupportProductCudaTask {
  std::uint64_t task_id{};
  std::uint64_t source_snapshot_epoch{};
  ExactHigherSupportProductCudaTaskKind kind{
      ExactHigherSupportProductCudaTaskKind::support_prune};
  Phase15ExactHigherSupportProductCudaArithmeticWidth arithmetic_width{
      Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024};
  std::uint64_t aligned_coordinate_bit_width{};
  std::uint8_t support_size{};
  std::uint8_t support_group_count{};
  std::uint64_t support_node_indices[
      phase15_exact_higher_support_product_maximum_support_size]{};
  std::uint64_t support_leaf_begins[
      phase15_exact_higher_support_product_maximum_support_size]{};
  std::uint64_t support_leaf_ends[
      phase15_exact_higher_support_product_maximum_support_size]{};
  std::uint8_t support_multiplicities[
      phase15_exact_higher_support_product_maximum_support_size]{};
  // The host boxes drive only the exact fake and the rational fallback.  A
  // native launcher must reconstruct every box from the authenticated node
  // receipt and resident coordinate/node arrays before making a decision.
  Phase15ExactHigherSupportProductCudaRawAabb3 support_boxes[
      phase15_exact_higher_support_product_maximum_support_size]{};
  std::uint64_t query_node_index{};
  std::uint64_t query_leaf_begin{};
  std::uint64_t query_leaf_end{};
  Phase15ExactHigherSupportProductCudaRawAabb3 query_box{};
  std::uint8_t has_query_box{};
};

static_assert(
    std::is_standard_layout_v<
        Phase15ExactHigherSupportProductCudaTask> &&
    std::is_trivially_copyable_v<
        Phase15ExactHigherSupportProductCudaTask>);

enum class Phase15ExactHigherSupportProductCudaDeviceOutcome : std::uint8_t {
  certified = 0U,
  exact_false = 1U,
  requires_cpu_rational_fallback = 2U,
  requires_host_int1024_fallback = 3U,
  requires_host_int512_fallback = 4U,
};

enum class Phase15ExactHigherSupportProductCudaDeviceBackend : std::uint8_t {
  bounded_dyadic_int512 = 0U,
  bounded_dyadic_int1024 = 1U,
  arbitrary_precision_rational = 2U,
  bounded_dyadic_int256 = 3U,
};

struct Phase15ExactHigherSupportProductCudaDeviceRecord {
  std::uint64_t task_id{};
  ExactHigherSupportProductCudaTaskKind kind{
      ExactHigherSupportProductCudaTaskKind::support_prune};
  Phase15ExactHigherSupportProductCudaDeviceOutcome outcome{
      Phase15ExactHigherSupportProductCudaDeviceOutcome::exact_false};
  Phase15ExactHigherSupportProductCudaDeviceBackend backend{
      Phase15ExactHigherSupportProductCudaDeviceBackend::
          arbitrary_precision_rational};
};

static_assert(
    std::is_trivially_copyable_v<
        Phase15ExactHigherSupportProductCudaDeviceRecord>);

inline constexpr std::uint64_t
    phase15_exact_higher_support_product_digest_offset =
        UINT64_C(1469598103934665603);
inline constexpr std::uint64_t
    phase15_exact_higher_support_product_digest_prime =
        UINT64_C(1099511628211);

[[nodiscard]] inline std::uint64_t
phase15_exact_higher_support_product_digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int byte_index = 0U; byte_index < 8U; ++byte_index) {
    digest ^= word & UINT64_C(0xff);
    digest *= phase15_exact_higher_support_product_digest_prime;
    word >>= 8U;
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_exact_higher_support_product_task_digest(
    std::span<const Phase15ExactHigherSupportProductCudaTask> tasks) noexcept {
  std::uint64_t digest =
      phase15_exact_higher_support_product_digest_offset;
  digest = phase15_exact_higher_support_product_digest_word(
      digest, static_cast<std::uint64_t>(tasks.size()));
  for (const auto& task : tasks) {
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.task_id);
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.source_snapshot_epoch);
    digest = phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(task.kind));
    digest = phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(task.arithmetic_width));
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.aligned_coordinate_bit_width);
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.support_size);
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.support_group_count);
    for (std::size_t index = 0U;
         index < task.support_group_count;
         ++index) {
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.support_node_indices[index]);
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.support_leaf_begins[index]);
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.support_leaf_ends[index]);
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.support_multiplicities[index]);
    }
    for (std::size_t index = 0U; index < task.support_size; ++index) {
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        digest = phase15_exact_higher_support_product_digest_word(
            digest,
            task.support_boxes[index].lower[axis]);
        digest = phase15_exact_higher_support_product_digest_word(
            digest,
            task.support_boxes[index].upper[axis]);
      }
    }
    digest = phase15_exact_higher_support_product_digest_word(
        digest, task.has_query_box ? UINT64_C(1) : UINT64_C(0));
    if (task.has_query_box) {
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.query_node_index);
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.query_leaf_begin);
      digest = phase15_exact_higher_support_product_digest_word(
          digest, task.query_leaf_end);
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        digest = phase15_exact_higher_support_product_digest_word(
            digest, task.query_box.lower[axis]);
        digest = phase15_exact_higher_support_product_digest_word(
            digest, task.query_box.upper[axis]);
      }
    }
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_exact_higher_support_product_device_result_digest(
    std::span<const Phase15ExactHigherSupportProductCudaDeviceRecord>
        records) noexcept {
  std::uint64_t digest =
      phase15_exact_higher_support_product_digest_offset;
  digest = phase15_exact_higher_support_product_digest_word(
      digest, static_cast<std::uint64_t>(records.size()));
  for (const auto& record : records) {
    digest = phase15_exact_higher_support_product_digest_word(
        digest, record.task_id);
    digest = phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.kind));
    digest = phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.outcome));
    digest = phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.backend));
  }
  return digest;
}

struct Phase15ExactHigherSupportProductCudaRequest {
  std::span<const Phase15ExactHigherSupportProductCudaTask> tasks;
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_task_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t submitted_task_digest{};
  const std::uint64_t* device_coordinate_bits{};
  const void* device_nodes{};
  int cuda_device{-1};
  bool resident_lease_index_identity_validated{false};
  bool host_fake{false};
};

struct Phase15ExactHigherSupportProductCudaReceipt {
  std::vector<Phase15ExactHigherSupportProductCudaDeviceRecord> records;
  std::uint64_t submitted_task_digest{};
  std::uint64_t completed_result_digest{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t kernel_elapsed_ns{};
  int cuda_device{-1};
  bool source_identity_authenticated{false};
  bool resident_lease_index_identity_validated{false};
  bool kernel_elapsed_ns_available{false};
  bool every_task_classified_once{false};
  bool floating_point_decision_performed{false};
  bool bigint_support_mass_transferred{false};
  bool forbidden_intermediate_materialized{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool native_lbvh_nodes_read_on_device{false};
  bool narrow_int256_kernel_executed{false};
  bool narrow_int512_kernel_executed{false};
};

[[nodiscard]] Phase15ExactHigherSupportProductCudaReceipt
phase15_launch_exact_higher_support_product_cuda(
    const Phase15ExactHigherSupportProductCudaRequest& request);

}  // namespace morsehgp3d::gpu::detail
