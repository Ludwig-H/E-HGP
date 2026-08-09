#include "phase15_jung_center_cover_prune_mass_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if !defined(__CUDACC__)
#error "phase15_jung_center_cover_prune_mass.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Jung center-cover profiler requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Jung center-cover interval filter"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Jung center-cover interval filter"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Jung center-cover profiler must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

using Clock = std::chrono::steady_clock;
using device::DeviceInterval;

constexpr std::size_t kAxisCount = 3U;
constexpr unsigned int kPatchThreadCount = 64U;
static_assert(kPatchThreadCount == jung_center_cover_patch_count);
constexpr std::uint32_t kFailureNone = 0U;
constexpr std::uint32_t kFailureMalformed = 1U;
constexpr std::uint32_t kFailureStackCapacity = 2U;
constexpr std::uint32_t kFailureReceiptCapacity = 3U;
constexpr std::uint32_t kFailureMassIdentity = 4U;

// CUDA 12.9's host standard library does not expose std::array::operator[]
// to device code.  Keep the public receipt ABI and index its contiguous scalar
// storage through the equivalent C-array view.
template <typename T, std::size_t N>
__device__ __forceinline__ T& device_array_element(
    std::array<T, N>& values,
    std::size_t index) noexcept {
  using Elements = T[N];
  static_assert(N > 0U);
  static_assert(std::is_standard_layout_v<std::array<T, N>>);
  static_assert(std::is_trivially_copyable_v<std::array<T, N>>);
  static_assert(sizeof(std::array<T, N>) == sizeof(T) * N);
  static_assert(alignof(std::array<T, N>) == alignof(T));
  return (*reinterpret_cast<Elements*>(&values))[index];
}

struct Phase15CenterCoverNode final {
  std::uint64_t lower_point_ids[kAxisCount];
  std::uint64_t upper_point_ids[kAxisCount];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(sizeof(Phase15CenterCoverNode) == 80U);
static_assert(alignof(Phase15CenterCoverNode) == 8U);
static_assert(std::is_standard_layout_v<Phase15CenterCoverNode>);
static_assert(std::is_trivially_copyable_v<Phase15CenterCoverNode>);
static_assert(std::is_same_v<spatial::PointId, std::uint64_t>);
static_assert(kAxisCount == jung_center_cover_axis_count);
static_assert(
    sizeof(Phase15CenterCoverNode) ==
    sizeof(spatial::MortonLbvhSnapshotNode));
static_assert(
    alignof(Phase15CenterCoverNode) ==
    alignof(spatial::MortonLbvhSnapshotNode));
static_assert(
    offsetof(Phase15CenterCoverNode, lower_point_ids) ==
    offsetof(spatial::MortonLbvhSnapshotNode, lower_point_ids));
static_assert(
    offsetof(Phase15CenterCoverNode, upper_point_ids) ==
    offsetof(spatial::MortonLbvhSnapshotNode, upper_point_ids));
static_assert(
    offsetof(Phase15CenterCoverNode, left_child) ==
    offsetof(spatial::MortonLbvhSnapshotNode, left_child));
static_assert(
    offsetof(Phase15CenterCoverNode, right_child) ==
    offsetof(spatial::MortonLbvhSnapshotNode, right_child));
static_assert(
    offsetof(Phase15CenterCoverNode, leaf_begin) ==
    offsetof(spatial::MortonLbvhSnapshotNode, leaf_begin));
static_assert(
    offsetof(Phase15CenterCoverNode, leaf_end) ==
    offsetof(spatial::MortonLbvhSnapshotNode, leaf_end));
static_assert(std::is_standard_layout_v<JungCenterCoverEndpointBlock>);
static_assert(std::is_trivially_copyable_v<JungCenterCoverEndpointBlock>);
static_assert(std::is_standard_layout_v<JungCenterCoverQualificationReceipt>);
static_assert(std::is_trivially_copyable_v<JungCenterCoverQualificationReceipt>);
static_assert(alignof(JungCenterCoverQualificationReceipt) == 8U);
static_assert(sizeof(JungCenterCoverQualificationReceipt) == 14'152U);
static_assert(
    offsetof(JungCenterCoverQualificationReceipt, block) == 0U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_bounds_valid_mask) == 40U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        infeasible_patch_mask) == 48U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        covered_patch_mask) == 56U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_lower_binary64_bits) == 64U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_upper_binary64_bits) == 1'600U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_counts) == 3'136U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_point_masses) == 3'392U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_indices) == 3'904U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_point_masses) == 5'952U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_leaf_begins) == 10'048U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_leaf_ends) == 12'096U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        disposition) == 14'144U);
static_assert(
    std::tuple_size_v<decltype(
        JungCenterCoverQualificationReceipt::patch_lower_binary64_bits)> ==
    jung_center_cover_patch_count * jung_center_cover_axis_count);
static_assert(
    std::tuple_size_v<decltype(
        JungCenterCoverQualificationReceipt::patch_upper_binary64_bits)> ==
    jung_center_cover_patch_count * jung_center_cover_axis_count);

struct DeviceGlobalControl final {
  std::uint64_t unordered_pair_universe_mass{};
  std::uint64_t generated_shard_count{};
  std::uint64_t next_shard_index{};
  std::uint64_t worker_cta_with_work_count{};
  std::uint64_t qualification_receipt_count{};
  std::uint64_t pruned_pair_mass{};
  std::uint64_t microtile_pair_mass{};
  std::uint64_t endpoint_block_visit_count{};
  std::uint64_t center_cover_attempt_count{};
  std::uint64_t pruned_block_count{};
  std::uint64_t microtile_block_count{};
  std::uint64_t diagonal_split_count{};
  std::uint64_t cross_split_count{};
  std::uint64_t patch_evaluation_count{};
  std::uint64_t infeasible_patch_count{};
  std::uint64_t covered_patch_count{};
  std::uint64_t unresolved_patch_count{};
  std::uint64_t witness_node_visit_count{};
  std::uint64_t accepted_witness_node_count{};
  std::uint64_t accepted_witness_point_mass{};
  std::uint64_t arithmetic_ambiguity_count{};
  std::uint64_t stack_capacity_failure_count{};
  std::uint64_t maximum_stack_high_water{};
  std::uint64_t maximum_shard_block_visits{};
  std::uint64_t maximum_shard_witness_node_visits{};
  std::uint64_t shard_block_visit_histogram[
      jung_center_cover_histogram_bin_count]{};
  std::uint64_t shard_witness_visit_histogram[
      jung_center_cover_histogram_bin_count]{};
  std::uint32_t failure_code{};
  std::uint8_t output_valid{};
};

struct DeviceShardControl final {
  std::uint64_t pruned_pair_mass{};
  std::uint64_t microtile_pair_mass{};
  std::uint64_t endpoint_block_visit_count{};
  std::uint64_t center_cover_attempt_count{};
  std::uint64_t pruned_block_count{};
  std::uint64_t microtile_block_count{};
  std::uint64_t diagonal_split_count{};
  std::uint64_t cross_split_count{};
  std::uint64_t patch_evaluation_count{};
  std::uint64_t infeasible_patch_count{};
  std::uint64_t covered_patch_count{};
  std::uint64_t unresolved_patch_count{};
  std::uint64_t witness_node_visit_count{};
  std::uint64_t accepted_witness_node_count{};
  std::uint64_t accepted_witness_point_mass{};
  std::uint64_t arithmetic_ambiguity_count{};
  std::uint64_t stack_capacity_failure_count{};
  std::uint64_t maximum_stack_high_water{};
};

struct Box3 final {
  DeviceInterval axis[kAxisCount];
  bool valid{};
};

struct CenterCoverGeometry final {
  Box3 first;
  Box3 second;
  Box3 midpoint;
  Box3 cover;
  double maximum_coordinate_separation{};
  double maximum_squared_diameter{};
  bool valid{};
};

struct PatchEvaluation final {
  std::uint64_t witness_node_visits{};
  std::uint64_t accepted_node_count{};
  std::uint64_t accepted_point_mass{};
  std::uint64_t arithmetic_ambiguities{};
  bool infeasible{};
  bool covered{};
};

struct QualificationPatchWitnesses final {
  std::uint32_t node_indices[jung_center_cover_support4_witness_threshold]{};
  std::uint64_t node_masses[jung_center_cover_support4_witness_threshold]{};
  std::uint32_t node_leaf_begins[
      jung_center_cover_support4_witness_threshold]{};
  std::uint32_t node_leaf_ends[
      jung_center_cover_support4_witness_threshold]{};
};

template <bool CollectQualificationReceipts>
struct QualificationPatchSharedStorage final {};

template <>
struct QualificationPatchSharedStorage<true> final {
  QualificationPatchWitnesses patches[jung_center_cover_patch_count];
};

template <bool CollectQualificationReceipts>
struct QualificationPatchBoxSharedStorage final {};

template <>
struct QualificationPatchBoxSharedStorage<true> final {
  Box3 patches[jung_center_cover_patch_count];
};

class CenterCoverCudaError final : public std::runtime_error {
 public:
  CenterCoverCudaError(cudaError_t code, const std::string& operation)
      : std::runtime_error(
            operation + ": " + cudaGetErrorName(code) + " (" +
            cudaGetErrorString(code) + ")") {}
};

void check_cuda(cudaError_t code, const std::string& operation) {
  if (code != cudaSuccess) {
    throw CenterCoverCudaError(code, operation);
  }
}

[[nodiscard]] std::size_t checked_bytes(
    std::size_t count,
    std::size_t width,
    const char* message) {
  if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width) {
    throw std::length_error(message);
  }
  return count * width;
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t unordered_pair_mass(std::size_t point_count) {
  const std::uint64_t n = static_cast<std::uint64_t>(point_count);
  return (n & 1U) == 0U ? (n / 2U) * (n - 1U)
                        : n * ((n - 1U) / 2U);
}

[[nodiscard]] std::uint64_t ns_between(
    Clock::time_point begin,
    Clock::time_point end) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  return elapsed < 0 ? 0U : static_cast<std::uint64_t>(elapsed);
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void allocate(std::size_t count, const char* operation) {
    if (data_ != nullptr) {
      throw std::logic_error(
          "a Jung center-cover device buffer was allocated twice");
    }
    if (count == 0U) {
      return;
    }
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&data_),
            checked_bytes(count, sizeof(Value), operation)),
        operation);
    count_ = count;
  }

  void reset() noexcept {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
      data_ = nullptr;
      count_ = 0U;
    }
  }

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] const Value* get() const noexcept { return data_; }
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

 private:
  Value* data_{};
  std::size_t count_{};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int requested) {
    check_cuda(
        cudaGetDevice(&previous_),
        "cudaGetDevice for Jung center-cover profiler");
    if (previous_ != requested) {
      check_cuda(
          cudaSetDevice(requested),
          "cudaSetDevice for Jung center-cover profiler");
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

 private:
  int previous_{};
  bool restore_{};
};

class CudaStream final {
 public:
  CudaStream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Jung center-cover profiler");
  }
  CudaStream(const CudaStream&) = delete;
  CudaStream& operator=(const CudaStream&) = delete;
  ~CudaStream() {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
    }
  }
  [[nodiscard]] cudaStream_t get() const noexcept { return stream_; }

 private:
  cudaStream_t stream_{};
};

class CudaEvent final {
 public:
  CudaEvent() {
    check_cuda(
        cudaEventCreate(&event_),
        "cudaEventCreate for Jung center-cover profiler");
  }
  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;
  ~CudaEvent() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  void record(cudaStream_t stream) {
    check_cuda(
        cudaEventRecord(event_, stream),
        "cudaEventRecord for Jung center-cover profiler");
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

[[nodiscard]] std::uint64_t elapsed_ns(
    const CudaEvent& begin,
    const CudaEvent& end) {
  float milliseconds = 0.0F;
  check_cuda(
      cudaEventElapsedTime(&milliseconds, begin.get(), end.get()),
      "cudaEventElapsedTime for Jung center-cover profiler");
  if (!(milliseconds >= 0.0F)) {
    throw std::runtime_error(
        "a Jung center-cover CUDA event returned a NaN duration");
  }
  const double nanoseconds = static_cast<double>(milliseconds) * 1'000'000.0;
  return nanoseconds >=
          static_cast<double>(std::numeric_limits<std::uint64_t>::max())
      ? std::numeric_limits<std::uint64_t>::max()
      : static_cast<std::uint64_t>(nanoseconds);
}

void require_device_pointer(
    const void* pointer,
    int expected_device,
    std::size_t alignment,
    const char* label) {
  if (pointer == nullptr ||
      reinterpret_cast<std::uintptr_t>(pointer) % alignment != 0U) {
    throw std::invalid_argument(
        std::string{"the Jung center-cover source "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{"cudaPointerGetAttributes for Jung center-cover "} + label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the Jung center-cover source "} + label +
        " is not on the authenticated CUDA device");
  }
}

[[nodiscard]] __device__ DeviceInterval scalar(double value) noexcept {
  return DeviceInterval{value, value, device::is_finite(value)};
}

[[nodiscard]] __device__ bool valid_node(
    const Phase15CenterCoverNode* nodes,
    std::uint64_t node_index,
    std::uint64_t node_count,
    std::uint64_t point_count) noexcept {
  if (node_index >= node_count) {
    return false;
  }
  const auto& node = nodes[node_index];
  if (node.leaf_begin >= node.leaf_end || node.leaf_end > point_count) {
    return false;
  }
  const std::uint64_t width = node.leaf_end - node.leaf_begin;
  if (width == 1U) {
    return true;
  }
  return node.left_child < node_index && node.right_child < node_index &&
      node.left_child < node_count && node.right_child < node_count;
}

[[nodiscard]] __device__ bool finish_or_skip_subtree(
    std::uint64_t width,
    std::uint64_t& cursor,
    bool& complete) noexcept {
  if (width == 0U || width > UINT64_MAX / 2U + 1U) {
    return false;
  }
  const std::uint64_t extent = 2U * width - 1U;
  if (extent > cursor + 1U) {
    return false;
  }
  complete = extent == cursor + 1U;
  if (!complete) {
    cursor -= extent;
  }
  return true;
}

[[nodiscard]] __device__ std::uint64_t node_width(
    const JungCenterCoverEndpointNode& node) noexcept {
  return static_cast<std::uint64_t>(node.leaf_end) - node.leaf_begin;
}

[[nodiscard]] __device__ std::uint64_t block_mass(
    const JungCenterCoverEndpointBlock& block) noexcept {
  const std::uint64_t first_width = node_width(block.first);
  if (block.kind == JungCenterCoverEndpointBlockKind::diagonal) {
    return (first_width & 1U) == 0U
        ? (first_width / 2U) * (first_width - 1U)
        : first_width * ((first_width - 1U) / 2U);
  }
  return first_width * node_width(block.second);
}

[[nodiscard]] __device__ JungCenterCoverEndpointNode endpoint_node(
    const Phase15CenterCoverNode& node,
    std::uint64_t node_index) noexcept {
  return JungCenterCoverEndpointNode{
      static_cast<std::uint32_t>(node_index),
      static_cast<std::uint32_t>(node.leaf_begin),
      static_cast<std::uint32_t>(node.leaf_end)};
}

[[nodiscard]] __device__ JungCenterCoverEndpointBlock diagonal_block(
    const Phase15CenterCoverNode* nodes,
    std::uint64_t node_index) noexcept {
  JungCenterCoverEndpointBlock result;
  result.first = endpoint_node(nodes[node_index], node_index);
  result.second = result.first;
  result.kind = JungCenterCoverEndpointBlockKind::diagonal;
  result.unordered_pair_mass = block_mass(result);
  return result;
}

[[nodiscard]] __device__ JungCenterCoverEndpointBlock cross_block(
    const Phase15CenterCoverNode* nodes,
    std::uint64_t first_index,
    std::uint64_t second_index) noexcept {
  auto first = endpoint_node(nodes[first_index], first_index);
  auto second = endpoint_node(nodes[second_index], second_index);
  if (second.leaf_begin < first.leaf_begin) {
    const auto temporary = first;
    first = second;
    second = temporary;
  }
  JungCenterCoverEndpointBlock result;
  result.first = first;
  result.second = second;
  result.kind = JungCenterCoverEndpointBlockKind::cross;
  result.unordered_pair_mass = block_mass(result);
  return result;
}

[[nodiscard]] __device__ bool load_box(
    const Phase15CenterCoverNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    Box3& box) noexcept {
  box.valid = true;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      box.valid = false;
      return false;
    }
    const auto lower = device::point_interval(
        coordinate_bits[axis * point_count + lower_id]);
    const auto upper = device::point_interval(
        coordinate_bits[axis * point_count + upper_id]);
    box.axis[axis] =
        device::checked_interval(lower.lower, upper.upper);
    box.valid = box.valid && box.axis[axis].valid;
  }
  return box.valid;
}

[[nodiscard]] __device__ double maximum_axis_separation(
    const DeviceInterval& first,
    const DeviceInterval& second,
    bool& valid) noexcept {
  const double forward = __dsub_ru(second.upper, first.lower);
  const double backward = __dsub_ru(first.upper, second.lower);
  const double separation =
      forward > backward ? forward : backward;
  if (!device::is_finite(separation) || separation < 0.0) {
    valid = false;
    return 0.0;
  }
  return separation;
}

[[nodiscard]] __device__ bool build_center_cover_geometry(
    const JungCenterCoverEndpointBlock& block,
    const Phase15CenterCoverNode* nodes,
    std::uint64_t node_count,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    CenterCoverGeometry& geometry) noexcept {
  if (block.first.node_index >= node_count ||
      block.second.node_index >= node_count ||
      !valid_node(nodes, block.first.node_index, node_count, point_count) ||
      !valid_node(nodes, block.second.node_index, node_count, point_count)) {
    return false;
  }
  const auto& first_node = nodes[block.first.node_index];
  const auto& second_node = nodes[block.second.node_index];
  if (first_node.leaf_begin != block.first.leaf_begin ||
      first_node.leaf_end != block.first.leaf_end ||
      second_node.leaf_begin != block.second.leaf_begin ||
      second_node.leaf_end != block.second.leaf_end ||
      !load_box(first_node, coordinate_bits, point_count, geometry.first) ||
      !load_box(second_node, coordinate_bits, point_count, geometry.second)) {
    return false;
  }

  bool valid = true;
  double maximum_separation = 0.0;
  double maximum_squared_diameter = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const auto& first = geometry.first.axis[axis];
    const auto& second = geometry.second.axis[axis];
    const double separation =
        maximum_axis_separation(first, second, valid);
    maximum_separation = separation > maximum_separation
        ? separation
        : maximum_separation;
    const double square = __dmul_ru(separation, separation);
    maximum_squared_diameter =
        __dadd_ru(maximum_squared_diameter, square);
    const double midpoint_lower = __dadd_rd(
        __dmul_rd(first.lower, 0.5), __dmul_rd(second.lower, 0.5));
    const double midpoint_upper = __dadd_ru(
        __dmul_ru(first.upper, 0.5), __dmul_ru(second.upper, 0.5));
    geometry.midpoint.axis[axis] =
        device::checked_interval(midpoint_lower, midpoint_upper);
    valid = valid && geometry.midpoint.axis[axis].valid;
  }
  const double expansion = __dmul_ru(maximum_separation, 0.625);
  valid = valid && device::is_finite(maximum_squared_diameter) &&
      device::is_finite(expansion);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    geometry.cover.axis[axis] = device::checked_interval(
        __dsub_rd(geometry.midpoint.axis[axis].lower, expansion),
        __dadd_ru(geometry.midpoint.axis[axis].upper, expansion));
    valid = valid && geometry.cover.axis[axis].valid;
  }
  geometry.midpoint.valid = valid;
  geometry.cover.valid = valid;
  geometry.maximum_coordinate_separation = maximum_separation;
  geometry.maximum_squared_diameter = maximum_squared_diameter;
  geometry.valid = valid;
  return valid;
}

[[nodiscard]] __device__ DeviceInterval patch_axis(
    const DeviceInterval& cover,
    unsigned int ordinal) noexcept {
  if (!cover.valid || ordinal >= 4U) {
    return device::invalid_interval();
  }
  const double width_lower = __dsub_rd(cover.upper, cover.lower);
  const double width_upper = __dsub_ru(cover.upper, cover.lower);
  const double lower_fraction = static_cast<double>(ordinal) * 0.25;
  const double upper_fraction = static_cast<double>(ordinal + 1U) * 0.25;
  return device::checked_interval(
      __dadd_rd(
          cover.lower, __dmul_rd(width_lower, lower_fraction)),
      __dadd_ru(
          cover.lower, __dmul_ru(width_upper, upper_fraction)));
}

[[nodiscard]] __device__ Box3 make_patch(
    const Box3& cover,
    unsigned int patch_index) noexcept {
  Box3 patch;
  patch.valid = cover.valid && patch_index < jung_center_cover_patch_count;
  unsigned int remaining = patch_index;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const unsigned int ordinal = remaining & 3U;
    remaining >>= 2U;
    patch.axis[axis] = patch_axis(cover.axis[axis], ordinal);
    patch.valid = patch.valid && patch.axis[axis].valid;
  }
  return patch;
}

[[nodiscard]] __device__ double squared_difference_lower(
    double left,
    double right,
    bool& valid) noexcept {
  const double difference = __dsub_rd(left, right);
  const double alternate = __dsub_ru(left, right);
  const double lower = difference <= 0.0 && alternate >= 0.0
      ? 0.0
      : (difference > 0.0
             ? __dmul_rd(difference, difference)
             : __dmul_rd(alternate, alternate));
  valid = valid && device::is_finite(lower) && lower >= 0.0;
  return lower;
}

[[nodiscard]] __device__ double squared_difference_upper(
    double left,
    double right,
    bool& valid) noexcept {
  const double difference_lower = __dsub_rd(left, right);
  const double difference_upper = __dsub_ru(left, right);
  const double lower_square =
      __dmul_ru(difference_lower, difference_lower);
  const double upper_square =
      __dmul_ru(difference_upper, difference_upper);
  const double upper = lower_square > upper_square
      ? lower_square
      : upper_square;
  valid = valid && device::is_finite(upper) && upper >= 0.0;
  return upper;
}

[[nodiscard]] __device__ double power_axis_candidate_lower(
    double center,
    const DeviceInterval& support,
    const DeviceInterval& witness,
    bool& valid) noexcept {
  double support_distance = 0.0;
  if (center < support.lower) {
    support_distance =
        squared_difference_lower(center, support.lower, valid);
  } else if (center > support.upper) {
    support_distance =
        squared_difference_lower(center, support.upper, valid);
  }
  const double witness_lower =
      squared_difference_upper(center, witness.lower, valid);
  const double witness_upper =
      squared_difference_upper(center, witness.upper, valid);
  const double farthest_witness =
      witness_lower > witness_upper ? witness_lower : witness_upper;
  const double result = __dsub_rd(support_distance, farthest_witness);
  valid = valid && device::is_finite(result);
  return result;
}

[[nodiscard]] __device__ double power_axis_lower(
    const DeviceInterval& center,
    const DeviceInterval& support,
    const DeviceInterval& witness,
    bool& valid) noexcept {
  if (!center.valid || !support.valid || !witness.valid) {
    valid = false;
    return 0.0;
  }
  double result = power_axis_candidate_lower(
      center.lower, support, witness, valid);
  const double upper_result = power_axis_candidate_lower(
      center.upper, support, witness, valid);
  result = upper_result < result ? upper_result : result;
  if (support.lower >= center.lower && support.lower <= center.upper) {
    const double candidate = power_axis_candidate_lower(
        support.lower, support, witness, valid);
    result = candidate < result ? candidate : result;
  }
  if (support.upper >= center.lower && support.upper <= center.upper) {
    const double candidate = power_axis_candidate_lower(
        support.upper, support, witness, valid);
    result = candidate < result ? candidate : result;
  }
  return result;
}

[[nodiscard]] __device__ double power_lower(
    const Box3& center,
    const Box3& support,
    const Box3& witness,
    bool& valid) noexcept {
  valid = valid && center.valid && support.valid && witness.valid;
  double result = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double axis_lower = power_axis_lower(
        center.axis[axis], support.axis[axis], witness.axis[axis], valid);
    result = __dadd_rd(result, axis_lower);
    valid = valid && device::is_finite(result);
  }
  return result;
}

[[nodiscard]] __device__ double power_upper(
    const Box3& center,
    const Box3& support,
    const Box3& witness,
    bool& valid) noexcept {
  const double swapped = power_lower(center, witness, support, valid);
  return -swapped;
}

[[nodiscard]] __device__ double squared_box_distance_lower(
    const Box3& first,
    const Box3& second,
    bool& valid) noexcept {
  valid = valid && first.valid && second.valid;
  double result = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    double distance = 0.0;
    if (first.axis[axis].upper < second.axis[axis].lower) {
      distance = __dsub_rd(
          second.axis[axis].lower, first.axis[axis].upper);
    } else if (second.axis[axis].upper < first.axis[axis].lower) {
      distance = __dsub_rd(
          first.axis[axis].lower, second.axis[axis].upper);
    }
    const double square = __dmul_rd(distance, distance);
    result = __dadd_rd(result, square);
    valid = valid && device::is_finite(result) && result >= 0.0;
  }
  return result;
}

[[nodiscard]] __device__ bool ranges_overlap(
    std::uint64_t first_begin,
    std::uint64_t first_end,
    std::uint64_t second_begin,
    std::uint64_t second_end) noexcept {
  return first_begin < second_end && second_begin < first_end;
}

[[nodiscard]] __device__ double spatial_width(
    const Phase15CenterCoverNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    bool& valid) noexcept {
  Box3 box;
  valid = valid && load_box(node, coordinate_bits, point_count, box);
  double width = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double axis_width =
        __dsub_ru(box.axis[axis].upper, box.axis[axis].lower);
    valid = valid && device::is_finite(axis_width) && axis_width >= 0.0;
    width = axis_width > width ? axis_width : width;
  }
  return width;
}

[[nodiscard]] __device__ unsigned int split_block(
    const JungCenterCoverEndpointBlock& block,
    const Phase15CenterCoverNode* nodes,
    std::uint64_t node_count,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    JungCenterCoverEndpointBlock children[3U],
    bool& valid) noexcept {
  if (block.first.node_index >= node_count ||
      block.second.node_index >= node_count) {
    valid = false;
    return 0U;
  }
  const auto& first = nodes[block.first.node_index];
  const auto& second = nodes[block.second.node_index];
  const std::uint64_t first_width = first.leaf_end - first.leaf_begin;
  const std::uint64_t second_width = second.leaf_end - second.leaf_begin;
  if (block.kind == JungCenterCoverEndpointBlockKind::diagonal) {
    if (first_width <= 1U || first.left_child >= node_count ||
        first.right_child >= node_count) {
      return 0U;
    }
    unsigned int count = 0U;
    const auto left_diagonal = diagonal_block(nodes, first.left_child);
    const auto cross = cross_block(nodes, first.left_child, first.right_child);
    const auto right_diagonal = diagonal_block(nodes, first.right_child);
    if (left_diagonal.unordered_pair_mass != 0U) {
      children[count++] = left_diagonal;
    }
    if (cross.unordered_pair_mass != 0U) {
      children[count++] = cross;
    }
    if (right_diagonal.unordered_pair_mass != 0U) {
      children[count++] = right_diagonal;
    }
    return count;
  }
  if (first_width <= 1U && second_width <= 1U) {
    return 0U;
  }
  bool geometry_valid = true;
  const double first_spatial_width = first_width > 1U
      ? spatial_width(first, coordinate_bits, point_count, geometry_valid)
      : -1.0;
  const double second_spatial_width = second_width > 1U
      ? spatial_width(second, coordinate_bits, point_count, geometry_valid)
      : -1.0;
  // Spatial width only schedules an exhaustive split.  Overflow or another
  // interval ambiguity must never turn into a rejection: fall back to the
  // canonical leaf-count/node-index order and keep the whole pair mass.
  const bool split_first = first_width > 1U &&
      (second_width <= 1U ||
       (geometry_valid
            ? (first_spatial_width > second_spatial_width ||
               (first_spatial_width == second_spatial_width &&
                (first_width > second_width ||
                 (first_width == second_width &&
                  block.first.node_index <= block.second.node_index))))
            : (first_width > second_width ||
               (first_width == second_width &&
                block.first.node_index <= block.second.node_index))));
  if (split_first) {
    if (first.left_child >= node_count || first.right_child >= node_count) {
      valid = false;
      return 0U;
    }
    children[0U] =
        cross_block(nodes, first.left_child, block.second.node_index);
    children[1U] =
        cross_block(nodes, first.right_child, block.second.node_index);
  } else {
    if (second.left_child >= node_count || second.right_child >= node_count) {
      valid = false;
      return 0U;
    }
    children[0U] =
        cross_block(nodes, block.first.node_index, second.left_child);
    children[1U] =
        cross_block(nodes, block.first.node_index, second.right_child);
  }
  return 2U;
}

__device__ void signal_failure(
    DeviceGlobalControl* control,
    std::uint32_t failure) noexcept {
  atomicCAS(&control->failure_code, kFailureNone, failure);
}

__global__ void generate_canonical_shards_kernel(
    const Phase15CenterCoverNode* nodes,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t requested_shards,
    JungCenterCoverEndpointBlock* shards,
    DeviceGlobalControl* control) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  if (point_count < 2U || node_count != 2U * point_count - 1U ||
      requested_shards == 0U ||
      !valid_node(nodes, node_count - 1U, node_count, point_count)) {
    signal_failure(control, kFailureMalformed);
    return;
  }
  shards[0U] = diagonal_block(nodes, node_count - 1U);
  std::uint64_t shard_count = 1U;
  while (shard_count < requested_shards) {
    std::uint64_t best_index = UINT64_MAX;
    std::uint64_t best_mass = 0U;
    unsigned int best_child_count = 0U;
    for (std::uint64_t index = 0U; index < shard_count; ++index) {
      JungCenterCoverEndpointBlock children[3U]{};
      bool valid = true;
      const unsigned int child_count = split_block(
          shards[index],
          nodes,
          node_count,
          coordinate_bits,
          point_count,
          children,
          valid);
      if (!valid) {
        signal_failure(control, kFailureMalformed);
        return;
      }
      if (child_count == 0U ||
          shard_count + child_count - 1U > requested_shards) {
        continue;
      }
      if (best_index == UINT64_MAX ||
          shards[index].unordered_pair_mass > best_mass) {
        best_index = index;
        best_mass = shards[index].unordered_pair_mass;
        best_child_count = child_count;
      }
    }
    if (best_index == UINT64_MAX) {
      break;
    }
    JungCenterCoverEndpointBlock children[3U]{};
    bool valid = true;
    const unsigned int child_count = split_block(
        shards[best_index],
        nodes,
        node_count,
        coordinate_bits,
        point_count,
        children,
        valid);
    if (!valid || child_count != best_child_count) {
      signal_failure(control, kFailureMalformed);
      return;
    }
    shards[best_index] = children[0U];
    for (unsigned int child = 1U; child < child_count; ++child) {
      shards[shard_count++] = children[child];
    }
  }
  std::uint64_t source_mass = 0U;
  for (std::uint64_t index = 0U; index < shard_count; ++index) {
    if (shards[index].unordered_pair_mass != block_mass(shards[index]) ||
        UINT64_MAX - source_mass < shards[index].unordered_pair_mass) {
      signal_failure(control, kFailureMassIdentity);
      return;
    }
    source_mass += shards[index].unordered_pair_mass;
  }
  if (source_mass != control->unordered_pair_universe_mass) {
    signal_failure(control, kFailureMassIdentity);
    return;
  }
  control->generated_shard_count = shard_count;
}

[[nodiscard]] __device__ bool patch_is_infeasible(
    const Box3& patch,
    const CenterCoverGeometry& geometry,
    bool& arithmetic_ambiguous) noexcept {
  bool valid = true;
  const double mediator_lower =
      power_lower(patch, geometry.first, geometry.second, valid);
  const double mediator_upper =
      power_upper(patch, geometry.first, geometry.second, valid);
  if (!valid) {
    arithmetic_ambiguous = true;
    return false;
  }
  if (mediator_lower > 0.0 || mediator_upper < 0.0) {
    return true;
  }
  const double distance_lower =
      squared_box_distance_lower(patch, geometry.midpoint, valid);
  const double jung_left = __dmul_rd(8.0, distance_lower);
  valid = valid && device::is_finite(jung_left);
  if (!valid) {
    arithmetic_ambiguous = true;
    return false;
  }
  return jung_left > geometry.maximum_squared_diameter;
}

template <bool CollectQualificationReceipts>
[[nodiscard]] __device__ PatchEvaluation evaluate_patch(
    const Box3& patch,
    const CenterCoverGeometry& geometry,
    const JungCenterCoverEndpointBlock& endpoint_block,
    const Phase15CenterCoverNode* nodes,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t node_count,
    QualificationPatchWitnesses* qualification_witnesses) noexcept {
  PatchEvaluation result;
  if (!patch.valid || !geometry.valid) {
    result.arithmetic_ambiguities = 1U;
    return result;
  }
  bool feasibility_ambiguous = false;
  if (patch_is_infeasible(patch, geometry, feasibility_ambiguous)) {
    result.infeasible = true;
    return result;
  }
  result.arithmetic_ambiguities += feasibility_ambiguous ? 1U : 0U;

  std::uint64_t cursor = node_count - 1U;
  while (true) {
    if (!valid_node(nodes, cursor, node_count, point_count)) {
      result.arithmetic_ambiguities += 1U;
      return result;
    }
    const auto& node = nodes[cursor];
    const std::uint64_t width = node.leaf_end - node.leaf_begin;
    ++result.witness_node_visits;
    const bool overlaps_first = ranges_overlap(
        node.leaf_begin,
        node.leaf_end,
        endpoint_block.first.leaf_begin,
        endpoint_block.first.leaf_end);
    const bool overlaps_second = ranges_overlap(
        node.leaf_begin,
        node.leaf_end,
        endpoint_block.second.leaf_begin,
        endpoint_block.second.leaf_end);
    const bool contained_in_first =
        node.leaf_begin >= endpoint_block.first.leaf_begin &&
        node.leaf_end <= endpoint_block.first.leaf_end;
    const bool contained_in_second =
        node.leaf_begin >= endpoint_block.second.leaf_begin &&
        node.leaf_end <= endpoint_block.second.leaf_end;
    bool skip = false;
    bool descend = false;
    if (contained_in_first || contained_in_second) {
      // Endpoint points can never be witness points.  A certified LBVH node
      // wholly contained in either endpoint range is therefore skipped as a
      // subtree; descending it would pay O(|A|+|B|) for every patch.
      skip = true;
    } else if (overlaps_first || overlaps_second) {
      descend = width > 1U;
      skip = !descend;
    } else {
      Box3 witness;
      if (!load_box(node, coordinate_bits, point_count, witness)) {
        ++result.arithmetic_ambiguities;
        if (width > 1U) {
          descend = true;
        } else {
          skip = true;
        }
      } else {
        bool valid = true;
        const double first_lower =
            power_lower(patch, geometry.first, witness, valid);
        const double second_lower =
            power_lower(patch, geometry.second, witness, valid);
        const double strict_lower =
            first_lower > second_lower ? first_lower : second_lower;
        const double first_upper =
            power_upper(patch, geometry.first, witness, valid);
        const double second_upper =
            power_upper(patch, geometry.second, witness, valid);
        const double exterior_upper =
            first_upper < second_upper ? first_upper : second_upper;
        if (!valid) {
          ++result.arithmetic_ambiguities;
          if (width > 1U) {
            descend = true;
          } else {
            skip = true;
          }
        } else if (strict_lower > 0.0) {
          const std::uint64_t slot = result.accepted_node_count;
          if (slot >= jung_center_cover_support4_witness_threshold) {
            ++result.arithmetic_ambiguities;
            return result;
          }
          if constexpr (CollectQualificationReceipts) {
            qualification_witnesses->node_indices[slot] =
                static_cast<std::uint32_t>(cursor);
            qualification_witnesses->node_masses[slot] = width;
            qualification_witnesses->node_leaf_begins[slot] =
                static_cast<std::uint32_t>(node.leaf_begin);
            qualification_witnesses->node_leaf_ends[slot] =
                static_cast<std::uint32_t>(node.leaf_end);
          }
          ++result.accepted_node_count;
          result.accepted_point_mass += width;
          skip = true;
          if (result.accepted_point_mass >=
              jung_center_cover_support4_witness_threshold) {
            result.covered = true;
            return result;
          }
        } else if (exterior_upper <= 0.0) {
          skip = true;
        } else if (width > 1U) {
          descend = true;
        } else {
          ++result.arithmetic_ambiguities;
          skip = true;
        }
      }
    }
    if (descend) {
      cursor = node.right_child;
      continue;
    }
    if (!skip) {
      ++result.arithmetic_ambiguities;
      return result;
    }
    bool complete = false;
    if (!finish_or_skip_subtree(width, cursor, complete)) {
      ++result.arithmetic_ambiguities;
      return result;
    }
    if (complete) {
      break;
    }
  }
  return result;
}

__device__ void publish_qualification_receipt(
    const JungCenterCoverEndpointBlock& block,
    std::uint64_t infeasible_mask,
    std::uint64_t covered_mask,
    const Box3* patch_boxes,
    const PatchEvaluation* patches,
    const QualificationPatchWitnesses* qualification_witnesses,
    JungCenterCoverTerminalDisposition disposition,
    JungCenterCoverQualificationReceipt* receipts,
    std::uint64_t receipt_capacity,
    DeviceGlobalControl* global) noexcept {
  if (receipts == nullptr || receipt_capacity == 0U) {
    return;
  }
  const std::uint64_t receipt_index =
      atomicAdd(
          reinterpret_cast<unsigned long long int*>(
              &global->qualification_receipt_count),
          1ULL);
  if (receipt_index >= receipt_capacity) {
    signal_failure(global, kFailureReceiptCapacity);
    return;
  }
  auto& receipt = receipts[receipt_index];
  receipt.block = block;
  receipt.patch_bounds_valid_mask = 0U;
  receipt.infeasible_patch_mask = infeasible_mask;
  receipt.covered_patch_mask = covered_mask;
  receipt.disposition = disposition;
  for (std::size_t patch = 0U;
       patch < jung_center_cover_patch_count;
       ++patch) {
    const auto& patch_box = patch_boxes[patch];
    if (patch_box.valid) {
      receipt.patch_bounds_valid_mask |= UINT64_C(1) << patch;
    }
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      const std::size_t bound_offset = patch * kAxisCount + axis;
      device_array_element(
          receipt.patch_lower_binary64_bits, bound_offset) =
          static_cast<std::uint64_t>(
              __double_as_longlong(patch_box.axis[axis].lower));
      device_array_element(
          receipt.patch_upper_binary64_bits, bound_offset) =
          static_cast<std::uint64_t>(
              __double_as_longlong(patch_box.axis[axis].upper));
    }
    const auto& evaluation = patches[patch];
    device_array_element(receipt.witness_node_counts, patch) =
        static_cast<std::uint32_t>(evaluation.accepted_node_count);
    device_array_element(receipt.witness_point_masses, patch) =
        evaluation.accepted_point_mass;
    for (std::size_t witness = 0U;
         witness < evaluation.accepted_node_count;
         ++witness) {
      const std::size_t offset =
          patch * jung_center_cover_support4_witness_threshold + witness;
      device_array_element(receipt.witness_node_indices, offset) =
          qualification_witnesses[patch].node_indices[witness];
      device_array_element(receipt.witness_node_point_masses, offset) =
          qualification_witnesses[patch].node_masses[witness];
      device_array_element(receipt.witness_node_leaf_begins, offset) =
          qualification_witnesses[patch].node_leaf_begins[witness];
      device_array_element(receipt.witness_node_leaf_ends, offset) =
          qualification_witnesses[patch].node_leaf_ends[witness];
    }
  }
}

template <bool CollectQualificationReceipts>
__global__ void profile_center_cover_shards_kernel(
    const JungCenterCoverEndpointBlock* shards,
    JungCenterCoverEndpointBlock* stacks,
    std::uint64_t stack_capacity_per_shard,
    std::uint64_t endpoint_microtile_pair_mass,
    const Phase15CenterCoverNode* nodes,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t node_count,
    DeviceShardControl* shard_controls,
    JungCenterCoverQualificationReceipt* receipts,
    std::uint64_t receipt_capacity,
    DeviceGlobalControl* global) {
  __shared__ std::uint64_t shard_index;
  __shared__ std::uint64_t stack_size;
  __shared__ bool shard_finished;
  __shared__ bool cta_has_work;
  __shared__ bool geometry_valid;
  __shared__ JungCenterCoverEndpointBlock current_block;
  __shared__ CenterCoverGeometry geometry;
  __shared__ PatchEvaluation patch_evaluations[
      jung_center_cover_patch_count];
  __shared__ QualificationPatchSharedStorage<CollectQualificationReceipts>
      qualification_storage;
  __shared__ QualificationPatchBoxSharedStorage<CollectQualificationReceipts>
      qualification_patch_box_storage;

  if (threadIdx.x == 0U) {
    cta_has_work = false;
  }
  __syncthreads();

  while (true) {
    if (threadIdx.x == 0U) {
      shard_index = atomicAdd(
          reinterpret_cast<unsigned long long int*>(
              &global->next_shard_index),
          1ULL);
      shard_finished = shard_index >= global->generated_shard_count;
      stack_size = 0U;
      if (!shard_finished) {
        if (!cta_has_work) {
          atomicAdd(
              reinterpret_cast<unsigned long long int*>(
                  &global->worker_cta_with_work_count),
              1ULL);
          cta_has_work = true;
        }
        stacks[shard_index * stack_capacity_per_shard] = shards[shard_index];
        stack_size = 1U;
      }
    }
    __syncthreads();
    if (shard_finished) {
      return;
    }

    DeviceShardControl shard{};
    shard.maximum_stack_high_water = 1U;
    while (true) {
      if (threadIdx.x == 0U) {
        shard_finished = stack_size == 0U ||
            global->failure_code != kFailureNone;
        if (!shard_finished) {
          --stack_size;
          current_block = stacks[
              shard_index * stack_capacity_per_shard + stack_size];
          geometry = CenterCoverGeometry{};
          geometry_valid = build_center_cover_geometry(
              current_block,
              nodes,
              node_count,
              coordinate_bits,
              point_count,
              geometry);
        }
      }
      __syncthreads();
      if (shard_finished) {
        break;
      }

      if (threadIdx.x < jung_center_cover_patch_count) {
        const Box3 patch =
            make_patch(geometry.cover, threadIdx.x);
        if constexpr (CollectQualificationReceipts) {
          qualification_patch_box_storage.patches[threadIdx.x] = patch;
        }
        QualificationPatchWitnesses* qualification_witnesses = nullptr;
        if constexpr (CollectQualificationReceipts) {
          qualification_witnesses =
              &qualification_storage.patches[threadIdx.x];
        }
        patch_evaluations[threadIdx.x] = geometry_valid
            ? evaluate_patch<CollectQualificationReceipts>(
                  patch,
                  geometry,
                  current_block,
                  nodes,
                  coordinate_bits,
                  point_count,
                  node_count,
                  qualification_witnesses)
            : PatchEvaluation{};
        if (!geometry_valid) {
          patch_evaluations[threadIdx.x].arithmetic_ambiguities = 1U;
        }
      }
      __syncthreads();

      if (threadIdx.x == 0U) {
        ++shard.endpoint_block_visit_count;
        ++shard.center_cover_attempt_count;
        shard.patch_evaluation_count += jung_center_cover_patch_count;
        std::uint64_t infeasible_mask = 0U;
        std::uint64_t covered_mask = 0U;
        for (std::size_t patch = 0U;
             patch < jung_center_cover_patch_count;
             ++patch) {
          const auto& evaluation = patch_evaluations[patch];
          infeasible_mask |= evaluation.infeasible
              ? UINT64_C(1) << patch
              : UINT64_C(0);
          covered_mask |= evaluation.covered
              ? UINT64_C(1) << patch
              : UINT64_C(0);
          shard.witness_node_visit_count +=
              evaluation.witness_node_visits;
          shard.accepted_witness_node_count +=
              evaluation.accepted_node_count;
          shard.accepted_witness_point_mass +=
              evaluation.accepted_point_mass;
          shard.arithmetic_ambiguity_count +=
              evaluation.arithmetic_ambiguities;
        }
        shard.infeasible_patch_count +=
            static_cast<std::uint64_t>(__popcll(infeasible_mask));
        shard.covered_patch_count +=
            static_cast<std::uint64_t>(__popcll(covered_mask));
        shard.unresolved_patch_count +=
            jung_center_cover_patch_count -
            static_cast<std::uint64_t>(
                __popcll(infeasible_mask | covered_mask));
        const bool pruned =
            (infeasible_mask | covered_mask) == UINT64_MAX;
        if (pruned) {
          shard.pruned_pair_mass += current_block.unordered_pair_mass;
          ++shard.pruned_block_count;
          if constexpr (CollectQualificationReceipts) {
            publish_qualification_receipt(
                current_block,
                infeasible_mask,
                covered_mask,
                qualification_patch_box_storage.patches,
                patch_evaluations,
                qualification_storage.patches,
                JungCenterCoverTerminalDisposition::pruned,
                receipts,
                receipt_capacity,
                global);
          }
        } else if (current_block.unordered_pair_mass <=
                   endpoint_microtile_pair_mass) {
          shard.microtile_pair_mass += current_block.unordered_pair_mass;
          ++shard.microtile_block_count;
          if constexpr (CollectQualificationReceipts) {
            publish_qualification_receipt(
                current_block,
                infeasible_mask,
                covered_mask,
                qualification_patch_box_storage.patches,
                patch_evaluations,
                qualification_storage.patches,
                JungCenterCoverTerminalDisposition::microtile,
                receipts,
                receipt_capacity,
                global);
          }
        } else {
          JungCenterCoverEndpointBlock children[3U]{};
          bool split_valid = true;
          const unsigned int child_count = split_block(
              current_block,
              nodes,
              node_count,
              coordinate_bits,
              point_count,
              children,
              split_valid);
          if (!split_valid || child_count == 0U) {
            signal_failure(global, kFailureMalformed);
          } else if (stack_size + child_count >
                     stack_capacity_per_shard) {
            ++shard.stack_capacity_failure_count;
            signal_failure(global, kFailureStackCapacity);
          } else {
            const std::uint64_t parent_mass =
                current_block.unordered_pair_mass;
            std::uint64_t child_mass = 0U;
            for (unsigned int child = 0U; child < child_count; ++child) {
              child_mass += children[child].unordered_pair_mass;
            }
            if (child_mass != parent_mass) {
              signal_failure(global, kFailureMassIdentity);
            } else {
              for (unsigned int reverse = child_count;
                   reverse > 0U;
                   --reverse) {
                stacks[shard_index * stack_capacity_per_shard + stack_size++] =
                    children[reverse - 1U];
              }
              shard.maximum_stack_high_water =
                  stack_size > shard.maximum_stack_high_water
                  ? stack_size
                  : shard.maximum_stack_high_water;
              if (current_block.kind ==
                  JungCenterCoverEndpointBlockKind::diagonal) {
                ++shard.diagonal_split_count;
              } else {
                ++shard.cross_split_count;
              }
            }
          }
        }
      }
      __syncthreads();
    }
    if (threadIdx.x == 0U) {
      shard_controls[shard_index] = shard;
    }
    __syncthreads();
  }
}

[[nodiscard]] __device__ std::size_t histogram_bin(
    std::uint64_t value) noexcept {
  std::size_t result = 0U;
  while (value > 1U &&
         result + 1U < jung_center_cover_histogram_bin_count) {
    value >>= 1U;
    ++result;
  }
  return result;
}

__global__ void finalize_center_cover_profile_kernel(
    const DeviceShardControl* shards,
    DeviceGlobalControl* global) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  if (global->failure_code != kFailureNone ||
      global->generated_shard_count == 0U) {
    return;
  }
  for (std::uint64_t index = 0U;
       index < global->generated_shard_count;
       ++index) {
    const auto& shard = shards[index];
    global->pruned_pair_mass += shard.pruned_pair_mass;
    global->microtile_pair_mass += shard.microtile_pair_mass;
    global->endpoint_block_visit_count += shard.endpoint_block_visit_count;
    global->center_cover_attempt_count += shard.center_cover_attempt_count;
    global->pruned_block_count += shard.pruned_block_count;
    global->microtile_block_count += shard.microtile_block_count;
    global->diagonal_split_count += shard.diagonal_split_count;
    global->cross_split_count += shard.cross_split_count;
    global->patch_evaluation_count += shard.patch_evaluation_count;
    global->infeasible_patch_count += shard.infeasible_patch_count;
    global->covered_patch_count += shard.covered_patch_count;
    global->unresolved_patch_count += shard.unresolved_patch_count;
    global->witness_node_visit_count += shard.witness_node_visit_count;
    global->accepted_witness_node_count +=
        shard.accepted_witness_node_count;
    global->accepted_witness_point_mass +=
        shard.accepted_witness_point_mass;
    global->arithmetic_ambiguity_count +=
        shard.arithmetic_ambiguity_count;
    global->stack_capacity_failure_count +=
        shard.stack_capacity_failure_count;
    global->maximum_stack_high_water =
        shard.maximum_stack_high_water > global->maximum_stack_high_water
        ? shard.maximum_stack_high_water
        : global->maximum_stack_high_water;
    global->maximum_shard_block_visits =
        shard.endpoint_block_visit_count > global->maximum_shard_block_visits
        ? shard.endpoint_block_visit_count
        : global->maximum_shard_block_visits;
    global->maximum_shard_witness_node_visits =
        shard.witness_node_visit_count >
            global->maximum_shard_witness_node_visits
        ? shard.witness_node_visit_count
        : global->maximum_shard_witness_node_visits;
    ++global->shard_block_visit_histogram[
        histogram_bin(shard.endpoint_block_visit_count)];
    ++global->shard_witness_visit_histogram[
        histogram_bin(shard.witness_node_visit_count)];
  }
  if (global->pruned_pair_mass >
          global->unordered_pair_universe_mass ||
      global->microtile_pair_mass !=
          global->unordered_pair_universe_mass -
              global->pruned_pair_mass) {
    signal_failure(global, kFailureMassIdentity);
    return;
  }
  global->output_valid = 1U;
}

[[nodiscard]] std::size_t installed_device_bytes(
    std::size_t shard_count,
    std::size_t stack_capacity,
    std::size_t receipt_capacity) {
  std::size_t total = checked_bytes(
      shard_count,
      sizeof(JungCenterCoverEndpointBlock),
      "Jung center-cover shard bytes overflow");
  const std::size_t stack_entry_count = checked_bytes(
      shard_count,
      stack_capacity,
      "Jung center-cover installed stack entry count overflow");
  total = checked_add(
      total,
      checked_bytes(
          stack_entry_count,
          sizeof(JungCenterCoverEndpointBlock),
          "Jung center-cover stack bytes overflow"),
      "Jung center-cover installed bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          shard_count,
          sizeof(DeviceShardControl),
          "Jung center-cover shard-control bytes overflow"),
      "Jung center-cover installed bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          receipt_capacity,
          sizeof(JungCenterCoverQualificationReceipt),
          "Jung center-cover receipt bytes overflow"),
      "Jung center-cover installed bytes overflow");
  return checked_add(
      total,
      sizeof(DeviceGlobalControl),
      "Jung center-cover installed bytes overflow");
}

}  // namespace

Phase15JungCenterCoverPruneMassReceipt
launch_phase15_jung_center_cover_prune_mass(
    const Phase15JungCenterCoverPruneMassRequest& request) {
  if (request.traversal.host_fake) {
    throw std::invalid_argument(
        "the native Jung center-cover launcher rejected a host fake");
  }
  if (!request.traversal.ready || request.traversal.cuda_device < 0 ||
      request.traversal.point_count < 2U ||
      request.traversal.point_count >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
      request.traversal.certified_node_count !=
          2U * request.traversal.point_count - 1U ||
      request.traversal.certified_node_count >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
      request.config.requested_shard_count < 2U ||
      request.config.stack_capacity_per_shard <
          jung_center_cover_minimum_stack_capacity ||
      request.config.endpoint_microtile_pair_mass == 0U ||
      (request.config.collect_n32_qualification_receipts &&
       request.traversal.point_count > 32U)) {
    throw std::invalid_argument(
        "malformed Jung center-cover native request");
  }

  DeviceGuard guard{request.traversal.cuda_device};
  require_device_pointer(
      request.traversal.device_coordinate_bits,
      request.traversal.cuda_device,
      alignof(std::uint64_t),
      "coordinate");
  require_device_pointer(
      request.traversal.device_morton_point_ids,
      request.traversal.cuda_device,
      alignof(std::uint64_t),
      "Morton PointId");
  require_device_pointer(
      request.traversal.device_nodes,
      request.traversal.cuda_device,
      alignof(Phase15CenterCoverNode),
      "LBVH node");

  const auto wall_begin = Clock::now();
  const std::size_t requested_shards = request.config.requested_shard_count;
  const std::size_t stack_capacity =
      request.config.stack_capacity_per_shard;
  const std::size_t stack_entry_count = checked_bytes(
      requested_shards,
      stack_capacity,
      "Jung center-cover stack entry count overflow");
  const std::uint64_t pair_mass =
      unordered_pair_mass(request.traversal.point_count);
  const std::size_t receipt_capacity =
      request.config.collect_n32_qualification_receipts
      ? static_cast<std::size_t>(pair_mass)
      : 0U;

  DeviceBuffer<JungCenterCoverEndpointBlock> shards;
  DeviceBuffer<JungCenterCoverEndpointBlock> stacks;
  DeviceBuffer<DeviceShardControl> shard_controls;
  DeviceBuffer<JungCenterCoverQualificationReceipt> receipts;
  DeviceBuffer<DeviceGlobalControl> global;
  shards.allocate(requested_shards, "cudaMalloc Jung center-cover shards");
  stacks.allocate(stack_entry_count, "cudaMalloc Jung center-cover stacks");
  shard_controls.allocate(
      requested_shards,
      "cudaMalloc Jung center-cover shard controls");
  receipts.allocate(
      receipt_capacity,
      "cudaMalloc Jung center-cover qualification receipts");
  global.allocate(1U, "cudaMalloc Jung center-cover global control");

  CudaStream stream;
  CudaEvent source_begin;
  CudaEvent source_end;
  CudaEvent cover_end;
  CudaEvent finalize_end;
  check_cuda(
      cudaMemsetAsync(
          global.get(), 0, sizeof(DeviceGlobalControl), stream.get()),
      "cudaMemsetAsync Jung center-cover global control");
  check_cuda(
      cudaMemsetAsync(
          shard_controls.get(),
          0,
          checked_bytes(
              requested_shards,
              sizeof(DeviceShardControl),
              "Jung center-cover shard-control memset bytes overflow"),
          stream.get()),
      "cudaMemsetAsync Jung center-cover shard controls");
  std::size_t memset_count = 2U;
  if (receipt_capacity != 0U) {
    check_cuda(
        cudaMemsetAsync(
            receipts.get(),
            0,
            checked_bytes(
                receipt_capacity,
                sizeof(JungCenterCoverQualificationReceipt),
                "Jung center-cover receipt memset bytes overflow"),
            stream.get()),
        "cudaMemsetAsync Jung center-cover qualification receipts");
    ++memset_count;
  }
  DeviceGlobalControl initial_control{};
  initial_control.unordered_pair_universe_mass = pair_mass;
  check_cuda(
      cudaMemcpyAsync(
          global.get(),
          &initial_control,
          sizeof(initial_control),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync Jung center-cover initial control");
  source_begin.record(stream.get());
  generate_canonical_shards_kernel<<<1U, 1U, 0U, stream.get()>>>(
      static_cast<const Phase15CenterCoverNode*>(
          request.traversal.device_nodes),
      request.traversal.device_coordinate_bits,
      static_cast<std::uint64_t>(request.traversal.point_count),
      static_cast<std::uint64_t>(request.traversal.certified_node_count),
      static_cast<std::uint64_t>(requested_shards),
      shards.get(),
      global.get());
  check_cuda(
      cudaGetLastError(),
      "launch Jung center-cover canonical shard source");
  source_end.record(stream.get());

  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(&properties, request.traversal.cuda_device),
      "cudaGetDeviceProperties for Jung center-cover profiler");
  const std::size_t resident_worker_target =
      static_cast<std::size_t>(properties.multiProcessorCount) * 4U;
  const std::size_t worker_cta_count = std::max<std::size_t>(
      2U,
      std::min(requested_shards, resident_worker_target));
  if (receipt_capacity == 0U) {
    profile_center_cover_shards_kernel<false><<<
        static_cast<unsigned int>(worker_cta_count),
        kPatchThreadCount,
        0U,
        stream.get()>>>(
        shards.get(),
        stacks.get(),
        static_cast<std::uint64_t>(stack_capacity),
        static_cast<std::uint64_t>(
            request.config.endpoint_microtile_pair_mass),
        static_cast<const Phase15CenterCoverNode*>(
            request.traversal.device_nodes),
        request.traversal.device_coordinate_bits,
        static_cast<std::uint64_t>(request.traversal.point_count),
        static_cast<std::uint64_t>(request.traversal.certified_node_count),
        shard_controls.get(),
        nullptr,
        0U,
        global.get());
  } else {
    profile_center_cover_shards_kernel<true><<<
        static_cast<unsigned int>(worker_cta_count),
        kPatchThreadCount,
        0U,
        stream.get()>>>(
        shards.get(),
        stacks.get(),
        static_cast<std::uint64_t>(stack_capacity),
        static_cast<std::uint64_t>(
            request.config.endpoint_microtile_pair_mass),
        static_cast<const Phase15CenterCoverNode*>(
            request.traversal.device_nodes),
        request.traversal.device_coordinate_bits,
        static_cast<std::uint64_t>(request.traversal.point_count),
        static_cast<std::uint64_t>(request.traversal.certified_node_count),
        shard_controls.get(),
        receipts.get(),
        static_cast<std::uint64_t>(receipt_capacity),
        global.get());
  }
  check_cuda(
      cudaGetLastError(),
      "launch Jung center-cover multi-CTA profiler");
  cover_end.record(stream.get());

  finalize_center_cover_profile_kernel<<<1U, 1U, 0U, stream.get()>>>(
      shard_controls.get(), global.get());
  check_cuda(
      cudaGetLastError(),
      "launch Jung center-cover finalizer");
  finalize_end.record(stream.get());

  DeviceGlobalControl host_control{};
  std::vector<JungCenterCoverQualificationReceipt> host_receipts(
      receipt_capacity);
  check_cuda(
      cudaMemcpyAsync(
          &host_control,
          global.get(),
          sizeof(host_control),
          cudaMemcpyDeviceToHost,
          stream.get()),
      "cudaMemcpyAsync Jung center-cover global audit");
  if (receipt_capacity != 0U) {
    check_cuda(
        cudaMemcpyAsync(
            host_receipts.data(),
            receipts.get(),
            checked_bytes(
                receipt_capacity,
                sizeof(JungCenterCoverQualificationReceipt),
                "Jung center-cover receipt D2H bytes overflow"),
            cudaMemcpyDeviceToHost,
            stream.get()),
        "cudaMemcpyAsync Jung center-cover qualification receipts");
  }
  check_cuda(
      cudaStreamSynchronize(stream.get()),
      "terminal cudaStreamSynchronize Jung center-cover profiler");
  const std::uint64_t shard_source_ns =
      elapsed_ns(source_begin, source_end);
  const std::uint64_t center_cover_ns =
      elapsed_ns(source_end, cover_end);
  const std::uint64_t finalize_ns =
      elapsed_ns(cover_end, finalize_end);
  const std::uint64_t source_cover_ns =
      elapsed_ns(source_begin, finalize_end);
  receipts.reset();
  shard_controls.reset();
  stacks.reset();
  shards.reset();
  global.reset();
  const auto wall_end = Clock::now();

  if (host_control.qualification_receipt_count > receipt_capacity) {
    host_control.failure_code = kFailureReceiptCapacity;
  } else {
    host_receipts.resize(static_cast<std::size_t>(
        host_control.qualification_receipt_count));
  }

  Phase15JungCenterCoverPruneMassReceipt result;
  result.status = host_control.failure_code == kFailureNone &&
          host_control.output_valid != 0U
      ? JungCenterCoverPruneMassStatus::complete
      : (host_control.failure_code == kFailureStackCapacity
             ? JungCenterCoverPruneMassStatus::stack_capacity_exhausted
             : JungCenterCoverPruneMassStatus::execution_failure);
  auto& audit = result.audit;
  audit.source_snapshot_epoch = request.traversal.source_snapshot_epoch;
  audit.point_count = request.traversal.point_count;
  audit.certified_node_count = request.traversal.certified_node_count;
  audit.requested_shard_count = requested_shards;
  audit.generated_shard_count = static_cast<std::size_t>(
      host_control.generated_shard_count);
  audit.worker_cta_count = worker_cta_count;
  audit.worker_cta_with_work_count = static_cast<std::size_t>(
      host_control.worker_cta_with_work_count);
  audit.stack_capacity_per_shard = stack_capacity;
  audit.maximum_stack_high_water = static_cast<std::size_t>(
      host_control.maximum_stack_high_water);
  audit.endpoint_microtile_pair_mass =
      request.config.endpoint_microtile_pair_mass;
  audit.unordered_pair_universe_mass = pair_mass;
  audit.pruned_pair_mass = host_control.pruned_pair_mass;
  audit.microtile_pair_mass = host_control.microtile_pair_mass;
  audit.endpoint_block_visit_count =
      host_control.endpoint_block_visit_count;
  audit.center_cover_attempt_count =
      host_control.center_cover_attempt_count;
  audit.pruned_block_count = host_control.pruned_block_count;
  audit.microtile_block_count = host_control.microtile_block_count;
  audit.diagonal_split_count = host_control.diagonal_split_count;
  audit.cross_split_count = host_control.cross_split_count;
  audit.patch_evaluation_count = host_control.patch_evaluation_count;
  audit.infeasible_patch_count = host_control.infeasible_patch_count;
  audit.covered_patch_count = host_control.covered_patch_count;
  audit.unresolved_patch_count = host_control.unresolved_patch_count;
  audit.witness_node_visit_count = host_control.witness_node_visit_count;
  audit.accepted_witness_node_count =
      host_control.accepted_witness_node_count;
  audit.accepted_witness_point_mass =
      host_control.accepted_witness_point_mass;
  audit.arithmetic_ambiguity_count =
      host_control.arithmetic_ambiguity_count;
  audit.stack_capacity_failure_count =
      host_control.stack_capacity_failure_count;
  audit.qualification_receipt_count =
      host_control.qualification_receipt_count;
  audit.qualification_receipt_capacity = receipt_capacity;
  audit.maximum_shard_block_visits =
      host_control.maximum_shard_block_visits;
  audit.maximum_shard_witness_node_visits =
      host_control.maximum_shard_witness_node_visits;
  for (std::size_t bin = 0U;
       bin < jung_center_cover_histogram_bin_count;
       ++bin) {
    audit.shard_block_visit_histogram[bin] =
        host_control.shard_block_visit_histogram[bin];
    audit.shard_witness_visit_histogram[bin] =
        host_control.shard_witness_visit_histogram[bin];
  }
  audit.physical_device_arena_byte_count = installed_device_bytes(
      requested_shards, stack_capacity, receipt_capacity);
  audit.source_device_byte_count =
      request.traversal.persistent_device_byte_capacity;
  audit.device_allocation_count = receipt_capacity == 0U ? 4U : 5U;
  audit.device_memset_count = memset_count;
  audit.cuda_kernel_launch_count = 3U;
  audit.cuda_synchronization_count = 1U;
  audit.host_to_device_byte_count = sizeof(DeviceGlobalControl);
  audit.device_to_host_byte_count = checked_add(
      sizeof(DeviceGlobalControl),
      checked_bytes(
          receipt_capacity,
          sizeof(JungCenterCoverQualificationReceipt),
          "Jung center-cover total D2H bytes overflow"),
      "Jung center-cover total D2H bytes overflow");
  audit.shard_source_device_nanoseconds = shard_source_ns;
  audit.center_cover_device_nanoseconds = center_cover_ns;
  audit.finalize_device_nanoseconds = finalize_ns;
  audit.source_cover_device_nanoseconds = source_cover_ns;
  audit.launcher_wall_nanoseconds = ns_between(wall_begin, wall_end);
  audit.source_identity_authenticated =
      request.traversal.source_cloud_identity != nullptr &&
      request.traversal.source_index_identity != nullptr;
  audit.source_epoch_authenticated =
      request.traversal.source_snapshot_epoch != 0U;
  audit.support4_only = true;
  audit.fixed_64_closed_patches_used = true;
  audit.directed_binary64_intervals_used = true;
  audit.strict_prune_only = true;
  audit.fail_open_on_ambiguity = true;
  audit.multi_cta_scheduler_used =
      host_control.worker_cta_with_work_count >= 2U;
  audit.private_per_shard_dfs_used = true;
  audit.exact_mass_identity_satisfied =
      result.status == JungCenterCoverPruneMassStatus::complete &&
      host_control.pruned_pair_mass <= pair_mass &&
      host_control.microtile_pair_mass ==
          pair_mass - host_control.pruned_pair_mass;
  audit.one_terminal_synchronization_used = true;
  audit.qualification_receipts_collected =
      result.status == JungCenterCoverPruneMassStatus::complete &&
      receipt_capacity != 0U;
  audit.qualification_patch_bounds_recorded =
      audit.qualification_receipts_collected;
  audit.aggregate_only_50k_path =
      request.traversal.point_count == 50'000U && receipt_capacity == 0U;
  audit.anchor_or_pair_payload_emitted = false;
  audit.endpoint_pair_array_materialized = false;
  audit.higher_order_delaunay_mosaic_materialized = false;
  audit.scientific_decision_published = false;
  audit.scientific_result_claimed = false;
  audit.component_only = true;
  audit.slo_eligible = false;
  audit.warm_e2e_slo_claimed = false;
  audit.anchor_completeness_claimed = false;
  audit.host_fake_launcher_exercised = false;
  audit.cuda_execution_performed = true;
  audit.public_status_claimed = false;
  if (result.status == JungCenterCoverPruneMassStatus::complete) {
    result.qualification_receipts = std::move(host_receipts);
  } else {
    host_receipts.clear();
  }
  return result;
}

}  // namespace morsehgp3d::gpu::detail
