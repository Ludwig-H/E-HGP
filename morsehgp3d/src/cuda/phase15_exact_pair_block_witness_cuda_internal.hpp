#pragma once

#include "morsehgp3d/gpu/exact_pair_block_witness_cuda.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct Phase15ExactPairBlockWitnessCudaAdoptedTraversal {
  std::shared_ptr<void> retained_owner;
  std::shared_ptr<const void> source_cloud_identity;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

class Phase15ExactPairBlockWitnessCudaTraversalAccess final {
 public:
  [[nodiscard]] static Phase15ExactPairBlockWitnessCudaAdoptedTraversal
  consume(MortonLbvhDeviceTraversalLease&& lease);
};

struct Phase15ExactPairBlockWitnessCudaTask {
  std::uint64_t task_id{};
  std::uint64_t unordered_pair_mass{};
  std::uint32_t first_node_index{};
  std::uint32_t first_leaf_begin{};
  std::uint32_t first_leaf_end{};
  std::uint32_t second_node_index{};
  std::uint32_t second_leaf_begin{};
  std::uint32_t second_leaf_end{};
  std::uint32_t witness_node_index{};
  std::uint32_t witness_leaf_begin{};
  std::uint32_t witness_leaf_end{};
  std::uint32_t host_input_flags{};
};

static_assert(
    std::is_trivially_copyable_v<Phase15ExactPairBlockWitnessCudaTask>);
static_assert(sizeof(Phase15ExactPairBlockWitnessCudaTask) == 56U);

inline constexpr std::uint32_t
    phase15_exact_pair_block_witness_host_invalid = 1U;

struct Phase15ExactPairBlockWitnessCudaDeviceRecord {
  std::uint64_t task_id{};
  std::uint64_t unordered_pair_mass{};
  std::uint8_t decision{};
  std::uint8_t proposal_state{};
  std::int8_t exact_sign{};
  std::uint8_t fixed_limb_evaluation_performed{};
  std::uint32_t reserved{};
};

static_assert(
    std::is_trivially_copyable_v<
        Phase15ExactPairBlockWitnessCudaDeviceRecord>);
static_assert(sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord) == 24U);

inline constexpr std::uint64_t
    phase15_exact_pair_block_witness_digest_offset =
        UINT64_C(1469598103934665603);
inline constexpr std::uint64_t
    phase15_exact_pair_block_witness_digest_prime = UINT64_C(1099511628211);

[[nodiscard]] inline std::uint64_t
phase15_exact_pair_block_witness_digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int byte_index = 0U; byte_index < 8U; ++byte_index) {
    digest ^= word & UINT64_C(0xff);
    digest *= phase15_exact_pair_block_witness_digest_prime;
    word >>= 8U;
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_exact_pair_block_witness_task_digest(
    std::span<const Phase15ExactPairBlockWitnessCudaTask> tasks) noexcept {
  std::uint64_t digest = phase15_exact_pair_block_witness_digest_offset;
  digest = phase15_exact_pair_block_witness_digest_word(
      digest, static_cast<std::uint64_t>(tasks.size()));
  for (const Phase15ExactPairBlockWitnessCudaTask& task : tasks) {
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.task_id);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.unordered_pair_mass);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.first_node_index);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.first_leaf_begin);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.first_leaf_end);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.second_node_index);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.second_leaf_begin);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.second_leaf_end);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.witness_node_index);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.witness_leaf_begin);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.witness_leaf_end);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, task.host_input_flags);
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_exact_pair_block_witness_result_digest(
    std::span<const ExactPairBlockWitnessCudaRecord> records) noexcept {
  std::uint64_t digest = phase15_exact_pair_block_witness_digest_offset;
  digest = phase15_exact_pair_block_witness_digest_word(
      digest, static_cast<std::uint64_t>(records.size()));
  for (const ExactPairBlockWitnessCudaRecord& record : records) {
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, record.task_id);
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, static_cast<std::uint64_t>(record.decision));
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, static_cast<std::uint64_t>(record.proposal_state));
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(record.exact_sign)));
    digest = phase15_exact_pair_block_witness_digest_word(
        digest, static_cast<std::uint64_t>(record.unordered_pair_mass));
    digest = phase15_exact_pair_block_witness_digest_word(
        digest,
        record.fixed_limb_evaluation_performed ? UINT64_C(1)
                                               : UINT64_C(0));
  }
  return digest;
}

struct Phase15ExactPairBlockWitnessCudaRequest {
  const Phase15ExactPairBlockWitnessCudaAdoptedTraversal* traversal{};
  std::span<const Phase15ExactPairBlockWitnessCudaTask> tasks;
  ExactPairBlockWitnessCudaConfig config{};
  std::size_t task_capacity{};
  std::uint64_t submitted_task_digest{};
};

struct Phase15ExactPairBlockWitnessCudaReceipt {
  std::vector<ExactPairBlockWitnessCudaRecord> records;
  ExactPairBlockWitnessCudaAudit audit{};
  bool source_identity_authenticated{false};
  bool every_task_classified_once{false};
  bool forbidden_intermediate_readback_performed{false};
  std::uint64_t submitted_task_digest{};
  std::uint64_t completed_result_digest{};
};

enum class Phase15ExactPairBlockWitnessCudaOutcome : std::uint8_t {
  invalid_authority = 0U,
  support_overlap = 1U,
  insufficient_witness = 2U,
  directed_nonnegative = 3U,
  directed_negative_exact_negative = 4U,
  directed_negative_exact_zero = 5U,
  directed_negative_exact_positive = 6U,
  directed_negative_fixed_overflow = 7U,
  directed_ambiguous_exact_negative = 8U,
  directed_ambiguous_exact_zero = 9U,
  directed_ambiguous_exact_positive = 10U,
  directed_ambiguous_fixed_overflow = 11U,
  directed_overflow_exact_negative = 12U,
  directed_overflow_exact_zero = 13U,
  directed_overflow_exact_positive = 14U,
  directed_overflow_fixed_overflow = 15U,
};

struct Phase15ExactPairBlockWitnessCudaRepeatedRequest {
  const Phase15ExactPairBlockWitnessCudaAdoptedTraversal* traversal{};
  std::span<const Phase15ExactPairBlockWitnessCudaTask> pattern;
  ExactPairBlockWitnessCudaConfig config{};
  std::size_t repetition_count{};
  std::size_t logical_task_count{};
  std::size_t task_capacity{};
  std::uint64_t first_task_id{};
  std::uint64_t repeated_task_recipe_digest{};
};

struct Phase15ExactPairBlockWitnessCudaRepeatedReceipt {
  std::vector<std::uint8_t> outcomes;
  std::size_t host_to_device_pattern_byte_count{};
  std::size_t device_to_host_outcome_byte_count{};
  std::size_t device_arena_byte_count{};
  std::size_t device_arena_allocation_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t kernel_elapsed_nanoseconds{};
  int cuda_device{-1};
  bool source_identity_authenticated{false};
  bool every_logical_task_classified_once{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool native_lbvh_nodes_read_on_device{false};
};

[[nodiscard]] Phase15ExactPairBlockWitnessCudaReceipt
phase15_launch_exact_pair_block_witness_cuda(
    const Phase15ExactPairBlockWitnessCudaRequest& request);

[[nodiscard]] Phase15ExactPairBlockWitnessCudaRepeatedReceipt
phase15_launch_repeated_exact_pair_block_witness_cuda(
    const Phase15ExactPairBlockWitnessCudaRepeatedRequest& request);

}  // namespace morsehgp3d::gpu::detail
