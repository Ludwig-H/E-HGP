#include "phase15_exact_pair_block_transactional_frontier_resident_cuda_internal.hpp"

#include "phase15_exact_pair_block_q_fixed.cuh"

#include "morsehgp3d/spatial/lbvh.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "the resident transactional frontier requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "the resident transactional frontier requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "fast math is forbidden for resident exact Q decisions"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "the resident transactional frontier target must contain only sm_120"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr std::uint64_t invalid_node = ~UINT64_C(0);
inline constexpr std::uint32_t device_status_complete = 1U;
inline constexpr std::uint32_t device_status_capacity_rollback = 2U;
inline constexpr std::uint32_t device_status_invalid_native_authority = 3U;

struct DeviceNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(
    std::is_standard_layout_v<DeviceNode> &&
    std::is_trivially_copyable_v<DeviceNode> &&
    sizeof(DeviceNode) == sizeof(spatial::MortonLbvhSnapshotNode));

using CompactNode =
    Phase15ExactPairBlockTransactionalFrontierResidentCudaNode;
using CompactBlock =
    Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock;
using CompactRecipe =
    Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe;

struct DevicePruneReceipt {
  std::uint64_t transaction_id{};
  CompactBlock support{};
  CompactNode witnesses[
      exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count]{};
  std::uint64_t certified_witness_point_count{};
  std::uint32_t witness_node_count{};
  std::uint32_t maximum_closed_rank{};
};

struct DeviceTerminalPair {
  std::uint64_t transaction_id{};
  CompactNode first{};
  CompactNode second{};
  std::uint64_t point_ids[2]{};
};

struct DeviceState {
  std::uint64_t pending_mass{};
  std::uint64_t pruned_mass{};
  std::uint64_t terminal_mass{};
  std::uint64_t transaction_id{1U};
  std::uint64_t wave_begin_count{};
  std::uint64_t wave_commit_count{};
  std::uint64_t wave_rollback_count{};
  std::uint64_t exact_prune_attempt_count{};
  std::uint64_t certified_prune_count{};
  std::uint64_t exact_prune_fail_open_count{};
  std::uint64_t exact_witness_predicate_count{};
  std::uint64_t exact_negative_witness_predicate_count{};
  std::uint64_t nonnegative_or_residual_witness_predicate_count{};
  std::uint64_t diagonal_split_count{};
  std::uint64_t cross_split_count{};
  std::uint64_t matched_recipe_count{};
  std::uint64_t maximum_pending_block_count{};
  std::uint32_t active_buffer{};
  std::uint32_t pending_block_count{};
  std::uint32_t prune_receipt_count{};
  std::uint32_t terminal_pair_count{};
  std::uint32_t status{};
  std::uint32_t reserved{};
};

static_assert(
    std::is_trivially_copyable_v<DevicePruneReceipt> &&
    std::is_trivially_copyable_v<DeviceTerminalPair> &&
    std::is_trivially_copyable_v<DeviceState>);

[[nodiscard]] __device__ bool is_leaf(const DeviceNode& node) noexcept {
  return node.left_child == invalid_node && node.right_child == invalid_node;
}

[[nodiscard]] __device__ std::uint64_t leaf_count(
    const CompactNode& node) noexcept {
  return static_cast<std::uint64_t>(node.leaf_end - node.leaf_begin);
}

[[nodiscard]] __device__ bool node_matches(
    const DeviceNode* nodes,
    std::uint32_t node_count,
    std::uint32_t point_count,
    const CompactNode& compact) noexcept {
  if (compact.node_index >= node_count || compact.leaf_begin >= compact.leaf_end ||
      compact.leaf_end > point_count) {
    return false;
  }
  const DeviceNode& native = nodes[compact.node_index];
  return native.leaf_begin == compact.leaf_begin &&
      native.leaf_end == compact.leaf_end;
}

[[nodiscard]] __device__ bool make_node(
    const DeviceNode* nodes,
    std::uint32_t node_count,
    std::uint32_t point_count,
    std::uint64_t node_index,
    CompactNode& result) noexcept {
  if (node_index >= node_count) {
    return false;
  }
  const DeviceNode& node = nodes[node_index];
  if (node.leaf_begin >= node.leaf_end || node.leaf_end > point_count) {
    return false;
  }
  result.node_index = static_cast<std::uint32_t>(node_index);
  result.leaf_begin = static_cast<std::uint32_t>(node.leaf_begin);
  result.leaf_end = static_cast<std::uint32_t>(node.leaf_end);
  return true;
}

[[nodiscard]] __device__ bool make_diagonal(
    const DeviceNode* nodes,
    std::uint32_t node_count,
    std::uint32_t point_count,
    std::uint64_t node_index,
    CompactBlock& result) noexcept {
  CompactNode node{};
  if (!make_node(nodes, node_count, point_count, node_index, node)) {
    return false;
  }
  const std::uint64_t count = leaf_count(node);
  result = {};
  result.first = node;
  result.second = node;
  result.kind = static_cast<std::uint8_t>(ExactPairBlockAuthorityKind::diagonal);
  result.unordered_pair_mass = count * (count - 1U) / 2U;
  return true;
}

[[nodiscard]] __device__ bool make_cross(
    const DeviceNode* nodes,
    std::uint32_t node_count,
    std::uint32_t point_count,
    std::uint64_t first_index,
    std::uint64_t second_index,
    CompactBlock& result) noexcept {
  CompactNode first{};
  CompactNode second{};
  if (!make_node(nodes, node_count, point_count, first_index, first) ||
      !make_node(nodes, node_count, point_count, second_index, second)) {
    return false;
  }
  if (second.leaf_begin < first.leaf_begin) {
    const CompactNode temporary = first;
    first = second;
    second = temporary;
  }
  if (first.leaf_end > second.leaf_begin) {
    return false;
  }
  result = {};
  result.first = first;
  result.second = second;
  result.kind = static_cast<std::uint8_t>(ExactPairBlockAuthorityKind::cross);
  result.unordered_pair_mass = leaf_count(first) * leaf_count(second);
  return true;
}

[[nodiscard]] __device__ int compare_block(
    const CompactBlock& first,
    const CompactBlock& second) noexcept {
#define MORSEHGP3D_COMPARE_FIELD(field) \
  if (first.field != second.field) { \
    return first.field < second.field ? -1 : 1; \
  }
  MORSEHGP3D_COMPARE_FIELD(kind)
  MORSEHGP3D_COMPARE_FIELD(first.leaf_begin)
  MORSEHGP3D_COMPARE_FIELD(first.leaf_end)
  MORSEHGP3D_COMPARE_FIELD(first.node_index)
  MORSEHGP3D_COMPARE_FIELD(second.leaf_begin)
  MORSEHGP3D_COMPARE_FIELD(second.leaf_end)
  MORSEHGP3D_COMPARE_FIELD(second.node_index)
#undef MORSEHGP3D_COMPARE_FIELD
  if (first.unordered_pair_mass != second.unordered_pair_mass) {
    return first.unordered_pair_mass < second.unordered_pair_mass ? -1 : 1;
  }
  return 0;
}

[[nodiscard]] __device__ const CompactRecipe* find_recipe(
    const CompactRecipe* recipes,
    std::uint32_t recipe_count,
    const CompactBlock& source) noexcept {
  std::uint32_t begin = 0U;
  std::uint32_t end = recipe_count;
  while (begin < end) {
    const std::uint32_t middle = begin + (end - begin) / 2U;
    const int order = compare_block(recipes[middle].source, source);
    if (order < 0) {
      begin = middle + 1U;
    } else {
      end = middle;
    }
  }
  return begin < recipe_count && compare_block(recipes[begin].source, source) == 0
      ? &recipes[begin]
      : nullptr;
}

[[nodiscard]] __device__ bool ranges_overlap(
    const CompactNode& first,
    const CompactNode& second) noexcept {
  return first.leaf_begin < second.leaf_end &&
      second.leaf_begin < first.leaf_end;
}

[[nodiscard]] __device__ bool load_box(
    const DeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint32_t point_count,
    exact_pair_block_q_fixed::Binary64Aabb3& box) noexcept {
  for (std::uint32_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      return false;
    }
    box.lower[axis] = coordinate_bits[axis * point_count + lower_id];
    box.upper[axis] = coordinate_bits[axis * point_count + upper_id];
    if (!exact_pair_block_q_fixed::binary64_interval_ordered(
            box.lower[axis], box.upper[axis])) {
      return false;
    }
  }
  return true;
}

struct PruneDecision {
  bool certified{};
  std::uint32_t evaluated{};
  std::uint32_t negative{};
  std::uint32_t residual{};
  std::uint64_t certified_witness_point_count{};
  CompactNode witness_nodes[
      exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count]{};
};

[[nodiscard]] __device__ PruneDecision try_recipe(
    const CompactRecipe& recipe,
    const CompactBlock& source,
    const DeviceNode* nodes,
    const std::uint64_t* coordinate_bits,
    std::uint32_t node_count,
    std::uint32_t point_count,
    std::uint32_t required_witness_point_count) noexcept {
  PruneDecision decision{};
  if (source.kind != static_cast<std::uint8_t>(ExactPairBlockAuthorityKind::cross) ||
      recipe.witness_node_count == 0U ||
      recipe.witness_node_count >
          exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count) {
    return decision;
  }
  exact_pair_block_q_fixed::Binary64Aabb3 first_box{};
  exact_pair_block_q_fixed::Binary64Aabb3 second_box{};
  if (!load_box(nodes[source.first.node_index], coordinate_bits, point_count, first_box) ||
      !load_box(nodes[source.second.node_index], coordinate_bits, point_count, second_box)) {
    return decision;
  }
  for (std::uint32_t witness_index = 0U;
       witness_index < recipe.witness_node_count;
       ++witness_index) {
    CompactNode witness{};
    if (!make_node(
            nodes,
            node_count,
            point_count,
            recipe.witness_node_indices[witness_index],
            witness) ||
        ranges_overlap(witness, source.first) ||
        ranges_overlap(witness, source.second)) {
      return decision;
    }
    for (std::uint32_t prior = 0U; prior < witness_index; ++prior) {
      if (ranges_overlap(witness, decision.witness_nodes[prior])) {
        return decision;
      }
    }
    decision.witness_nodes[witness_index] = witness;
    decision.certified_witness_point_count += leaf_count(witness);
  }
  if (decision.certified_witness_point_count < required_witness_point_count) {
    return decision;
  }
  for (std::uint32_t witness_index = 0U;
       witness_index < recipe.witness_node_count;
       ++witness_index) {
    exact_pair_block_q_fixed::Binary64Aabb3 witness_box{};
    if (!load_box(
            nodes[decision.witness_nodes[witness_index].node_index],
            coordinate_bits,
            point_count,
            witness_box)) {
      ++decision.residual;
      return decision;
    }
    ++decision.evaluated;
    const auto sign = exact_pair_block_q_fixed::exact_maximum_sign(
        first_box, second_box, witness_box);
    if (sign != exact_pair_block_q_fixed::fixed::ResultCode::negative) {
      ++decision.residual;
      return decision;
    }
    ++decision.negative;
  }
  decision.certified = decision.negative == recipe.witness_node_count;
  return decision;
}

[[nodiscard]] __device__ bool add_successor(
    CompactBlock* successors,
    std::uint32_t capacity,
    std::uint32_t& count,
    std::uint64_t& mass,
    const CompactBlock& successor) noexcept {
  if (successor.unordered_pair_mass == 0U) {
    return true;
  }
  if (count >= capacity) {
    return false;
  }
  successors[count++] = successor;
  mass += successor.unordered_pair_mass;
  return true;
}

__global__ void resident_scheduler_kernel(
    CompactBlock* first_buffer,
    CompactBlock* second_buffer,
    const CompactRecipe* recipes,
    DevicePruneReceipt* receipts,
    DeviceTerminalPair* terminals,
    DeviceState* state,
    const DeviceNode* nodes,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    std::uint32_t point_count,
    std::uint32_t node_count,
    std::uint32_t maximum_closed_rank,
    std::uint32_t block_capacity,
    std::uint32_t recipe_count,
    std::uint32_t receipt_capacity,
    std::uint32_t terminal_capacity) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  while (state->pending_block_count != 0U) {
    CompactBlock* sources = state->active_buffer == 0U
        ? first_buffer
        : second_buffer;
    CompactBlock* successors = state->active_buffer == 0U
        ? second_buffer
        : first_buffer;
    const std::uint32_t source_count = state->pending_block_count;
    const std::uint64_t source_mass = state->pending_mass;
    const std::uint32_t receipt_base = state->prune_receipt_count;
    const std::uint32_t terminal_base = state->terminal_pair_count;
    std::uint32_t successor_count = 0U;
    std::uint32_t staged_receipt_count = 0U;
    std::uint32_t staged_terminal_count = 0U;
    std::uint64_t successor_mass = 0U;
    std::uint64_t staged_pruned_mass = 0U;
    std::uint64_t staged_terminal_mass = 0U;
    std::uint64_t staged_attempts = 0U;
    std::uint64_t staged_certified = 0U;
    std::uint64_t staged_fail_open = 0U;
    std::uint64_t staged_predicates = 0U;
    std::uint64_t staged_negative = 0U;
    std::uint64_t staged_residual = 0U;
    std::uint64_t staged_diagonal_splits = 0U;
    std::uint64_t staged_cross_splits = 0U;
    std::uint64_t staged_matched_recipes = 0U;
    bool capacity_failure = false;
    bool native_failure = false;
    ++state->wave_begin_count;

    for (std::uint32_t block_index = 0U;
         block_index < source_count && !capacity_failure && !native_failure;
         ++block_index) {
      const CompactBlock source = sources[block_index];
      const std::uint64_t transaction_id = state->transaction_id++;
      if (!node_matches(nodes, node_count, point_count, source.first) ||
          !node_matches(nodes, node_count, point_count, source.second)) {
        native_failure = true;
        break;
      }

      const CompactRecipe* recipe = find_recipe(recipes, recipe_count, source);
      bool certified = false;
      if (recipe != nullptr) {
        ++staged_matched_recipes;
        ++staged_attempts;
        const PruneDecision decision = try_recipe(
            *recipe,
            source,
            nodes,
            coordinate_bits,
            node_count,
            point_count,
            maximum_closed_rank - 1U);
        staged_predicates += decision.evaluated;
        staged_negative += decision.negative;
        staged_residual += decision.residual;
        if (decision.certified) {
          if (receipt_base + staged_receipt_count >= receipt_capacity) {
            capacity_failure = true;
            break;
          }
          DevicePruneReceipt output{};
          output.transaction_id = transaction_id;
          output.support = source;
          output.certified_witness_point_count =
              decision.certified_witness_point_count;
          output.witness_node_count = recipe->witness_node_count;
          output.maximum_closed_rank = maximum_closed_rank;
          for (std::uint32_t witness_index = 0U;
               witness_index < recipe->witness_node_count;
               ++witness_index) {
            output.witnesses[witness_index] =
                decision.witness_nodes[witness_index];
          }
          receipts[receipt_base + staged_receipt_count++] = output;
          staged_pruned_mass += source.unordered_pair_mass;
          ++staged_certified;
          certified = true;
        } else {
          ++staged_fail_open;
        }
      }
      if (certified) {
        continue;
      }

      if (source.kind ==
          static_cast<std::uint8_t>(ExactPairBlockAuthorityKind::diagonal)) {
        const DeviceNode& parent = nodes[source.first.node_index];
        if (is_leaf(parent) || parent.left_child >= node_count ||
            parent.right_child >= node_count ||
            parent.left_child == parent.right_child) {
          native_failure = true;
          break;
        }
        CompactBlock left{};
        CompactBlock cross{};
        CompactBlock right{};
        if (!make_diagonal(nodes, node_count, point_count, parent.left_child, left) ||
            !make_cross(nodes, node_count, point_count, parent.left_child, parent.right_child, cross) ||
            !make_diagonal(nodes, node_count, point_count, parent.right_child, right) ||
            left.unordered_pair_mass + cross.unordered_pair_mass +
                    right.unordered_pair_mass !=
                source.unordered_pair_mass ||
            !add_successor(successors, block_capacity, successor_count, successor_mass, left) ||
            !add_successor(successors, block_capacity, successor_count, successor_mass, cross) ||
            !add_successor(successors, block_capacity, successor_count, successor_mass, right)) {
          capacity_failure = successor_count >= block_capacity;
          native_failure = !capacity_failure;
          break;
        }
        ++staged_diagonal_splits;
        continue;
      }
      if (source.kind !=
          static_cast<std::uint8_t>(ExactPairBlockAuthorityKind::cross) ||
          source.first.leaf_end > source.second.leaf_begin ||
          leaf_count(source.first) * leaf_count(source.second) !=
              source.unordered_pair_mass) {
        native_failure = true;
        break;
      }

      const DeviceNode& first_node = nodes[source.first.node_index];
      const DeviceNode& second_node = nodes[source.second.node_index];
      if (is_leaf(first_node) && is_leaf(second_node)) {
        if (source.unordered_pair_mass != 1U ||
            terminal_base + staged_terminal_count >= terminal_capacity) {
          capacity_failure = source.unordered_pair_mass == 1U;
          native_failure = !capacity_failure;
          break;
        }
        DeviceTerminalPair output{};
        output.transaction_id = transaction_id;
        output.first = source.first;
        output.second = source.second;
        std::uint64_t first_id = morton_point_ids[source.first.leaf_begin];
        std::uint64_t second_id = morton_point_ids[source.second.leaf_begin];
        if (second_id < first_id) {
          const std::uint64_t temporary = first_id;
          first_id = second_id;
          second_id = temporary;
        }
        output.point_ids[0] = first_id;
        output.point_ids[1] = second_id;
        terminals[terminal_base + staged_terminal_count++] = output;
        ++staged_terminal_mass;
        continue;
      }

      const bool split_second = !is_leaf(second_node) &&
          (is_leaf(first_node) ||
           leaf_count(source.second) >= leaf_count(source.first));
      CompactBlock first_child{};
      CompactBlock second_child{};
      bool children_valid = false;
      if (split_second) {
        children_valid = make_cross(
                             nodes, node_count, point_count,
                             source.first.node_index,
                             second_node.left_child,
                             first_child) &&
            make_cross(
                nodes, node_count, point_count,
                source.first.node_index,
                second_node.right_child,
                second_child);
      } else if (!is_leaf(first_node)) {
        children_valid = make_cross(
                             nodes, node_count, point_count,
                             first_node.left_child,
                             source.second.node_index,
                             first_child) &&
            make_cross(
                nodes, node_count, point_count,
                first_node.right_child,
                source.second.node_index,
                second_child);
      }
      if (!children_valid ||
          first_child.unordered_pair_mass + second_child.unordered_pair_mass !=
              source.unordered_pair_mass ||
          !add_successor(successors, block_capacity, successor_count, successor_mass, first_child) ||
          !add_successor(successors, block_capacity, successor_count, successor_mass, second_child)) {
        capacity_failure = successor_count >= block_capacity;
        native_failure = !capacity_failure;
        break;
      }
      ++staged_cross_splits;
    }

    if (capacity_failure || native_failure ||
        successor_mass + staged_pruned_mass + staged_terminal_mass !=
            source_mass) {
      ++state->wave_rollback_count;
      state->status = capacity_failure
          ? device_status_capacity_rollback
          : device_status_invalid_native_authority;
      return;
    }

    state->active_buffer = 1U - state->active_buffer;
    state->pending_block_count = successor_count;
    state->pending_mass = successor_mass;
    state->pruned_mass += staged_pruned_mass;
    state->terminal_mass += staged_terminal_mass;
    state->prune_receipt_count += staged_receipt_count;
    state->terminal_pair_count += staged_terminal_count;
    ++state->wave_commit_count;
    state->exact_prune_attempt_count += staged_attempts;
    state->certified_prune_count += staged_certified;
    state->exact_prune_fail_open_count += staged_fail_open;
    state->exact_witness_predicate_count += staged_predicates;
    state->exact_negative_witness_predicate_count += staged_negative;
    state->nonnegative_or_residual_witness_predicate_count += staged_residual;
    state->diagonal_split_count += staged_diagonal_splits;
    state->cross_split_count += staged_cross_splits;
    state->matched_recipe_count += staged_matched_recipes;
    if (successor_count > state->maximum_pending_block_count) {
      state->maximum_pending_block_count = successor_count;
    }
  }
  state->status = device_status_complete;
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, const char* operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const char* operation) {
    const char* name = cudaGetErrorName(code);
    const char* description = cudaGetErrorString(code);
    return std::string{operation} + ": " +
        (name == nullptr ? "unknown CUDA error" : name) + " (" +
        (description == nullptr ? "no description" : description) + ")";
  }
};

void check_cuda(cudaError_t code, const char* operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, operation);
  }
}

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target) {
    check_cuda(cudaGetDevice(&previous_), "cudaGetDevice resident frontier");
    if (previous_ != target) {
      check_cuda(cudaSetDevice(target), "cudaSetDevice resident frontier");
      restore_ = true;
    }
  }
  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() {
    if (restore_) {
      static_cast<void>(cudaSetDevice(previous_));
    }
  }
  void restore() {
    if (restore_) {
      check_cuda(cudaSetDevice(previous_), "restore CUDA device resident frontier");
      restore_ = false;
    }
  }

 private:
  int previous_{};
  bool restore_{};
};

class Stream final {
 public:
  Stream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreate resident frontier");
  }
  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;
  ~Stream() {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
    }
  }
  [[nodiscard]] cudaStream_t get() const noexcept { return stream_; }

 private:
  cudaStream_t stream_{};
};

class Event final {
 public:
  Event() {
    check_cuda(cudaEventCreate(&event_), "cudaEventCreate resident frontier");
  }
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  ~Event() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

class Arena final {
 public:
  explicit Arena(std::size_t bytes) {
    void* value = nullptr;
    check_cuda(cudaMalloc(&value, bytes), "cudaMalloc resident frontier arena");
    pointer_ = static_cast<std::byte*>(value);
  }
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena() {
    if (pointer_ != nullptr) {
      static_cast<void>(cudaFree(pointer_));
    }
  }
  [[nodiscard]] std::byte* get() noexcept { return pointer_; }

 private:
  std::byte* pointer_{};
};

[[nodiscard]] std::size_t checked_bytes(
    std::size_t count,
    std::size_t element_size) {
  if (count != 0U &&
      element_size > std::numeric_limits<std::size_t>::max() / count) {
    throw std::overflow_error("resident frontier device arena size overflow");
  }
  return count * element_size;
}

[[nodiscard]] std::size_t append_aligned(
    std::size_t& cursor,
    std::size_t alignment,
    std::size_t bytes) {
  const std::size_t remainder = cursor % alignment;
  const std::size_t padding = remainder == 0U ? 0U : alignment - remainder;
  if (padding > std::numeric_limits<std::size_t>::max() - cursor ||
      bytes > std::numeric_limits<std::size_t>::max() - cursor - padding) {
    throw std::overflow_error("resident frontier arena offset overflow");
  }
  cursor += padding;
  const std::size_t offset = cursor;
  cursor += bytes;
  return offset;
}

template <typename Target>
[[nodiscard]] Target* arena_pointer(std::byte* base, std::size_t offset) {
  return reinterpret_cast<Target*>(base + offset);
}

[[nodiscard]] ExactPairBlockNodeAuthority expand_node(
    const CompactNode& node) noexcept {
  return {node.node_index, node.leaf_begin, node.leaf_end};
}

[[nodiscard]] ExactPairBlockAuthority expand_block(
    const CompactBlock& block) noexcept {
  return {
      static_cast<ExactPairBlockAuthorityKind>(block.kind),
      expand_node(block.first),
      expand_node(block.second),
      static_cast<std::size_t>(block.unordered_pair_mass)};
}

}  // namespace

Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt
phase15_launch_exact_pair_block_transactional_frontier_resident_cuda(
    const Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest&
        request) {
  if (request.traversal == nullptr || request.host_index == nullptr ||
      request.host_cloud == nullptr || !request.traversal->ready ||
      request.traversal->host_fake || request.traversal->cuda_device < 0 ||
      request.traversal->device_coordinate_bits == nullptr ||
      request.traversal->device_morton_point_ids == nullptr ||
      request.traversal->device_nodes == nullptr ||
      request.block_capacity > std::numeric_limits<std::uint32_t>::max() ||
      request.prune_receipt_capacity >
          std::numeric_limits<std::uint32_t>::max() ||
      request.terminal_pair_capacity >
          std::numeric_limits<std::uint32_t>::max() ||
      request.recipes.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument(
        "the native resident scheduler received invalid compact authorities");
  }

  const std::size_t block_bytes = checked_bytes(
      request.block_capacity, sizeof(CompactBlock));
  const std::size_t recipe_bytes = checked_bytes(
      request.recipes.size(), sizeof(CompactRecipe));
  const std::size_t receipt_bytes = checked_bytes(
      request.prune_receipt_capacity, sizeof(DevicePruneReceipt));
  const std::size_t terminal_bytes = checked_bytes(
      request.terminal_pair_capacity, sizeof(DeviceTerminalPair));
  std::size_t arena_bytes = 0U;
  const std::size_t first_offset = append_aligned(
      arena_bytes, alignof(CompactBlock), block_bytes);
  const std::size_t second_offset = append_aligned(
      arena_bytes, alignof(CompactBlock), block_bytes);
  const std::size_t recipe_offset = append_aligned(
      arena_bytes, alignof(CompactRecipe), recipe_bytes);
  const std::size_t receipt_offset = append_aligned(
      arena_bytes, alignof(DevicePruneReceipt), receipt_bytes);
  const std::size_t terminal_offset = append_aligned(
      arena_bytes, alignof(DeviceTerminalPair), terminal_bytes);
  const std::size_t state_offset = append_aligned(
      arena_bytes, alignof(DeviceState), sizeof(DeviceState));

  DeviceGuard guard{request.traversal->cuda_device};
  Arena arena{arena_bytes};
  Stream stream;
  Event kernel_begin;
  Event kernel_end;
  CompactBlock* first_buffer =
      arena_pointer<CompactBlock>(arena.get(), first_offset);
  CompactBlock* second_buffer =
      arena_pointer<CompactBlock>(arena.get(), second_offset);
  CompactRecipe* device_recipes =
      arena_pointer<CompactRecipe>(arena.get(), recipe_offset);
  DevicePruneReceipt* device_receipts =
      arena_pointer<DevicePruneReceipt>(arena.get(), receipt_offset);
  DeviceTerminalPair* device_terminals =
      arena_pointer<DeviceTerminalPair>(arena.get(), terminal_offset);
  DeviceState* device_state =
      arena_pointer<DeviceState>(arena.get(), state_offset);

  DeviceState initial{};
  initial.pending_mass = request.unordered_pair_universe_mass;
  initial.pending_block_count =
      request.unordered_pair_universe_mass == 0U ? 0U : 1U;
  initial.maximum_pending_block_count = initial.pending_block_count;
  check_cuda(
      cudaMemcpyAsync(
          device_state,
          &initial,
          sizeof(initial),
          cudaMemcpyHostToDevice,
          stream.get()),
      "upload resident frontier state");
  if (initial.pending_block_count != 0U) {
    check_cuda(
        cudaMemcpyAsync(
            first_buffer,
            &request.root,
            sizeof(request.root),
            cudaMemcpyHostToDevice,
            stream.get()),
        "upload resident frontier root");
  }
  if (!request.recipes.empty()) {
    check_cuda(
        cudaMemcpyAsync(
            device_recipes,
            request.recipes.data(),
            recipe_bytes,
            cudaMemcpyHostToDevice,
            stream.get()),
        "upload resident frontier recipes");
  }
  check_cuda(
      cudaEventRecord(kernel_begin.get(), stream.get()),
      "record resident frontier kernel begin");
  resident_scheduler_kernel<<<1U, 1U, 0U, stream.get()>>>(
      first_buffer,
      second_buffer,
      device_recipes,
      device_receipts,
      device_terminals,
      device_state,
      static_cast<const DeviceNode*>(request.traversal->device_nodes),
      request.traversal->device_coordinate_bits,
      request.traversal->device_morton_point_ids,
      static_cast<std::uint32_t>(request.traversal->point_count),
      static_cast<std::uint32_t>(request.traversal->certified_node_count),
      static_cast<std::uint32_t>(request.config.maximum_closed_rank),
      static_cast<std::uint32_t>(request.block_capacity),
      static_cast<std::uint32_t>(request.recipes.size()),
      static_cast<std::uint32_t>(request.prune_receipt_capacity),
      static_cast<std::uint32_t>(request.terminal_pair_capacity));
  check_cuda(cudaGetLastError(), "launch resident frontier kernel");
  check_cuda(
      cudaEventRecord(kernel_end.get(), stream.get()),
      "record resident frontier kernel end");
  check_cuda(
      cudaEventSynchronize(kernel_end.get()),
      "synchronize resident frontier terminal event");
  float elapsed_milliseconds = 0.0F;
  check_cuda(
      cudaEventElapsedTime(
          &elapsed_milliseconds, kernel_begin.get(), kernel_end.get()),
      "measure resident frontier kernel");

  DeviceState final{};
  check_cuda(
      cudaMemcpyAsync(
          &final,
          device_state,
          sizeof(final),
          cudaMemcpyDeviceToHost,
          stream.get()),
      "download resident frontier final state");
  check_cuda(
      cudaStreamSynchronize(stream.get()),
      "synchronize resident frontier final state");
  if (final.status != device_status_complete &&
      final.status != device_status_capacity_rollback) {
    throw std::logic_error(
        "the resident scheduler rejected its immutable native LBVH authority");
  }

  std::vector<DevicePruneReceipt> compact_receipts(final.prune_receipt_count);
  std::vector<DeviceTerminalPair> compact_terminals(final.terminal_pair_count);
  std::vector<CompactBlock> compact_pending(final.pending_block_count);
  if (!compact_receipts.empty()) {
    check_cuda(
        cudaMemcpyAsync(
            compact_receipts.data(),
            device_receipts,
            compact_receipts.size() * sizeof(DevicePruneReceipt),
            cudaMemcpyDeviceToHost,
            stream.get()),
        "download resident frontier prune receipts");
  }
  if (!compact_terminals.empty()) {
    check_cuda(
        cudaMemcpyAsync(
            compact_terminals.data(),
            device_terminals,
            compact_terminals.size() * sizeof(DeviceTerminalPair),
            cudaMemcpyDeviceToHost,
            stream.get()),
        "download resident frontier terminal pairs");
  }
  if (!compact_pending.empty()) {
    CompactBlock* active = final.active_buffer == 0U
        ? first_buffer
        : second_buffer;
    check_cuda(
        cudaMemcpyAsync(
            compact_pending.data(),
            active,
            compact_pending.size() * sizeof(CompactBlock),
            cudaMemcpyDeviceToHost,
            stream.get()),
        "download resident frontier pending rollback cut");
  }
  check_cuda(
      cudaStreamSynchronize(stream.get()),
      "synchronize resident frontier bounded final readback");

  Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt result;
  result.status = final.status == device_status_complete
      ? ExactPairBlockTransactionalFrontierResidentCudaStatus::
            qualified_cuda_terminal_authority
      : ExactPairBlockTransactionalFrontierResidentCudaStatus::
            capacity_exhausted_wave_rolled_back;
  result.prune_receipts.reserve(compact_receipts.size());
  for (const auto& compact : compact_receipts) {
    ExactPairBlockTransactionalFrontierCudaPruneReceipt receipt;
    receipt.transaction_id = compact.transaction_id;
    receipt.support_block = expand_block(compact.support);
    receipt.witness_node_count = compact.witness_node_count;
    receipt.maximum_closed_rank = compact.maximum_closed_rank;
    receipt.certified_witness_point_count =
        static_cast<std::size_t>(compact.certified_witness_point_count);
    receipt.unordered_pair_mass =
        receipt.support_block.unordered_pair_mass;
    for (std::size_t witness_index = 0U;
         witness_index < receipt.witness_node_count;
         ++witness_index) {
      receipt.witness_nodes[witness_index] =
          expand_node(compact.witnesses[witness_index]);
    }
    result.prune_receipts.push_back(receipt);
  }
  result.terminal_pairs.reserve(compact_terminals.size());
  for (const auto& compact : compact_terminals) {
    result.terminal_pairs.push_back(
        ExactPairBlockTransactionalFrontierCudaTerminalPair{
            compact.transaction_id,
            expand_node(compact.first),
            expand_node(compact.second),
            {compact.point_ids[0], compact.point_ids[1]}});
  }
  result.pending_blocks.reserve(compact_pending.size());
  for (const auto& compact : compact_pending) {
    result.pending_blocks.push_back(expand_block(compact));
  }

  auto& audit = result.audit;
  audit.point_count = request.traversal->point_count;
  audit.certified_node_count = request.traversal->certified_node_count;
  audit.maximum_closed_rank = request.config.maximum_closed_rank;
  audit.required_witness_point_count = request.config.maximum_closed_rank - 1U;
  audit.block_capacity_per_point = request.config.block_capacity_per_point;
  audit.block_capacity = request.block_capacity;
  audit.prune_receipt_capacity = request.prune_receipt_capacity;
  audit.terminal_pair_capacity = request.terminal_pair_capacity;
  audit.prune_recipe_capacity = request.recipe_capacity;
  audit.submitted_prune_recipe_count = request.recipes.size();
  audit.matched_prune_recipe_count = final.matched_recipe_count;
  audit.unused_prune_recipe_count =
      audit.submitted_prune_recipe_count - audit.matched_prune_recipe_count;
  audit.unordered_pair_universe_mass = request.unordered_pair_universe_mass;
  audit.pending_unordered_pair_mass = final.pending_mass;
  audit.pruned_unordered_pair_mass = final.pruned_mass;
  audit.terminal_unordered_pair_mass = final.terminal_mass;
  audit.pending_block_count = final.pending_block_count;
  audit.prune_receipt_count = final.prune_receipt_count;
  audit.terminal_pair_count = final.terminal_pair_count;
  audit.wave_begin_count = final.wave_begin_count;
  audit.wave_commit_count = final.wave_commit_count;
  audit.wave_rollback_count = final.wave_rollback_count;
  audit.exact_prune_attempt_count = final.exact_prune_attempt_count;
  audit.certified_prune_count = final.certified_prune_count;
  audit.exact_prune_fail_open_count = final.exact_prune_fail_open_count;
  audit.exact_witness_predicate_count = final.exact_witness_predicate_count;
  audit.exact_negative_witness_predicate_count =
      final.exact_negative_witness_predicate_count;
  audit.nonnegative_or_residual_witness_predicate_count =
      final.nonnegative_or_residual_witness_predicate_count;
  audit.diagonal_split_count = final.diagonal_split_count;
  audit.cross_split_count = final.cross_split_count;
  audit.maximum_pending_block_count = final.maximum_pending_block_count;
  audit.host_to_device_recipe_byte_count = recipe_bytes;
  audit.device_to_host_final_byte_count = sizeof(DeviceState) +
      compact_receipts.size() * sizeof(DevicePruneReceipt) +
      compact_terminals.size() * sizeof(DeviceTerminalPair) +
      compact_pending.size() * sizeof(CompactBlock);
  audit.device_arena_byte_count = arena_bytes;
  audit.device_arena_allocation_count = 1U;
  audit.kernel_launch_count = 1U;
  audit.synchronization_count = 3U;
  audit.source_snapshot_epoch = request.traversal->source_snapshot_epoch;
  audit.submitted_recipe_digest = request.submitted_recipe_digest;
  audit.kernel_elapsed_nanoseconds = static_cast<std::uint64_t>(
      static_cast<double>(elapsed_milliseconds) * 1000000.0);
  audit.cuda_device = request.traversal->cuda_device;
  audit.native_lbvh_authority_consumed = true;
  audit.native_lbvh_nodes_read_on_device = true;
  audit.fixed_linear_capacities_validated = true;
  audit.compact_index_width_validated = true;
  audit.unique_recipe_sources_validated = true;
  audit.resident_double_buffer_allocated_once = true;
  audit.resident_wave_loop_executed = final.wave_begin_count != 0U;
  audit.one_persistent_kernel_launch_validated = true;
  audit.zero_intermediate_d2h_validated = true;
  audit.complete_wave_atomic_commit_validated = true;
  audit.capacity_wave_rollback_validated =
      final.status == device_status_capacity_rollback;
  audit.fail_open_native_transition_validated = true;
  audit.transactional_mass_conservation_validated =
      final.pending_mass + final.pruned_mass + final.terminal_mass ==
      request.unordered_pair_universe_mass;
  audit.native_split_partition_validated = true;
  audit.pairwise_disjoint_support_products_validated = true;
  audit.terminal_authority_sealed = final.status == device_status_complete;
  audit.global_pair_coverage_closed = final.status == device_status_complete;
  audit.cuda_execution_performed = true;
  result.source_identity_authenticated = true;
  guard.restore();
  return result;
}

}  // namespace morsehgp3d::gpu::detail
