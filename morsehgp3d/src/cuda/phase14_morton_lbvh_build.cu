#include "phase14_morton_lbvh_build_internal.hpp"

#include <cub/device/device_radix_sort.cuh>
#include <cuda_runtime.h>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase14_morton_lbvh_build.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 14 Morton LBVH builder requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 14 Morton LBVH builder"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 14 Morton LBVH builder"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 14 Morton LBVH builder must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kExponentMask =
    UINT64_C(0x7ff0000000000000);
constexpr double kMortonGridSize = 2097152.0;
constexpr int kMortonRadixEndBit = 63;

struct Phase14MortonLbvhDeviceLeaf {
  std::uint64_t morton_code;
  std::uint64_t point_id;
};

struct Phase14MortonLbvhDeviceNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

struct Phase14MortonLbvhRange {
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
  std::uint64_t postorder_begin;
};

static_assert(
    std::is_standard_layout_v<Phase14MortonLbvhDeviceLeaf> &&
    std::is_trivially_copyable_v<Phase14MortonLbvhDeviceLeaf> &&
    sizeof(Phase14MortonLbvhDeviceLeaf) ==
        sizeof(spatial::MortonLbvhSnapshotLeaf) &&
    alignof(Phase14MortonLbvhDeviceLeaf) ==
        alignof(spatial::MortonLbvhSnapshotLeaf));
static_assert(
    offsetof(Phase14MortonLbvhDeviceLeaf, morton_code) ==
        offsetof(spatial::MortonLbvhSnapshotLeaf, morton_code) &&
    offsetof(Phase14MortonLbvhDeviceLeaf, point_id) ==
        offsetof(spatial::MortonLbvhSnapshotLeaf, point_id));
static_assert(
    std::is_standard_layout_v<Phase14MortonLbvhDeviceNode> &&
    std::is_trivially_copyable_v<Phase14MortonLbvhDeviceNode> &&
    sizeof(Phase14MortonLbvhDeviceNode) ==
        sizeof(spatial::MortonLbvhSnapshotNode) &&
    alignof(Phase14MortonLbvhDeviceNode) ==
        alignof(spatial::MortonLbvhSnapshotNode));
static_assert(
    offsetof(Phase14MortonLbvhDeviceNode, lower_point_ids) ==
        offsetof(spatial::MortonLbvhSnapshotNode, lower_point_ids) &&
    offsetof(Phase14MortonLbvhDeviceNode, upper_point_ids) ==
        offsetof(spatial::MortonLbvhSnapshotNode, upper_point_ids) &&
    offsetof(Phase14MortonLbvhDeviceNode, left_child) ==
        offsetof(spatial::MortonLbvhSnapshotNode, left_child) &&
    offsetof(Phase14MortonLbvhDeviceNode, right_child) ==
        offsetof(spatial::MortonLbvhSnapshotNode, right_child) &&
    offsetof(Phase14MortonLbvhDeviceNode, leaf_begin) ==
        offsetof(spatial::MortonLbvhSnapshotNode, leaf_begin) &&
    offsetof(Phase14MortonLbvhDeviceNode, leaf_end) ==
        offsetof(spatial::MortonLbvhSnapshotNode, leaf_end));
static_assert(
    std::is_standard_layout_v<Phase14MortonLbvhRange> &&
    std::is_trivially_copyable_v<Phase14MortonLbvhRange> &&
    sizeof(Phase14MortonLbvhRange) == 3U * sizeof(std::uint64_t));

enum class Phase14MortonLbvhDeviceFailure : unsigned long long {
  none = 0ULL,
  invalid_range = 1ULL,
  invalid_split = 2ULL,
  postorder_index_overflow = 3ULL,
  frontier_capacity = 4ULL,
  level_index_capacity = 5ULL,
  invalid_child = 6ULL,
  invalid_witness = 7ULL,
};

class Phase14MortonLbvhCudaFailure final
    : public std::runtime_error {
 public:
  Phase14MortonLbvhCudaFailure(
      cudaError_t code,
      std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr
                ? std::string{"unknown CUDA error"}
                : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw Phase14MortonLbvhCudaFailure(
        code, std::move(operation));
  }
}

[[nodiscard]] std::size_t checked_axis_count(
    std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / kAxisCount) {
    throw std::length_error(
        "the Phase 14 Morton axis extent overflows size_t");
  }
  return point_count * kAxisCount;
}

[[nodiscard]] std::size_t checked_node_count(
    std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the Phase 14 Morton node extent overflows size_t");
  }
  return point_count * 2U - 1U;
}

[[nodiscard]] std::size_t checked_byte_product(
    std::size_t count,
    std::size_t width,
    const char* message) {
  if (count != 0U &&
      width > std::numeric_limits<std::size_t>::max() / count) {
    throw std::length_error(message);
  }
  return count * width;
}

[[nodiscard]] std::size_t checked_byte_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t maximum_topology_round_count(
    std::size_t point_count) {
  std::size_t collision_depth = 0U;
  std::size_t remaining = point_count - 1U;
  while (remaining != 0U) {
    ++collision_depth;
    remaining >>= 1U;
  }
  // At most 63 distinct Morton bits can be consumed before one equal-code
  // collision range remains.  Midpoint recursion on that range has depth at
  // most ceil(log2(n)).  One extra round processes depth zero.
  if (collision_depth >
      std::numeric_limits<std::size_t>::max() - 64U) {
    throw std::length_error(
        "the Phase 14 Morton topology round bound overflows size_t");
  }
  return 64U + collision_depth;
}

[[nodiscard]] bool finite_bits(std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

void validate_proposal_inputs(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& lower_bits,
    const std::array<std::uint64_t, 3>& upper_bits,
    std::size_t maximum_point_count) {
  const std::size_t axis_count = checked_axis_count(point_count);
  if (maximum_point_count == 0U ||
      point_count > maximum_point_count ||
      coordinate_bits.size() != axis_count) {
    throw std::invalid_argument(
        "the Phase 14 Morton proposal has invalid fixed-capacity extents");
  }
  for (const std::uint64_t bits : coordinate_bits) {
    if (!finite_bits(bits)) {
      throw std::invalid_argument(
          "the Phase 14 Morton proposal requires finite coordinates");
    }
  }
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    if (!finite_bits(lower_bits[axis]) ||
        !finite_bits(upper_bits[axis])) {
      throw std::invalid_argument(
          "the Phase 14 Morton proposal requires finite bounds");
    }
  }
}

void validate_snapshot_inputs(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_codes,
    std::size_t maximum_point_count) {
  const std::size_t axis_count = checked_axis_count(point_count);
  static_cast<void>(checked_node_count(point_count));
  if (maximum_point_count == 0U ||
      point_count > maximum_point_count ||
      coordinate_bits.size() != axis_count ||
      morton_codes.size() != point_count) {
    throw std::invalid_argument(
        "the Phase 14 Morton snapshot has invalid fixed-capacity extents");
  }
  if (maximum_point_count >
      static_cast<std::size_t>(INT_MAX)) {
    throw std::length_error(
        "the Phase 14 Morton CUB radix-sort capacity exceeds INT_MAX");
  }
  for (const std::uint64_t bits : coordinate_bits) {
    if (!finite_bits(bits)) {
      throw std::invalid_argument(
          "the Phase 14 Morton snapshot requires finite coordinates");
    }
  }
  for (const std::uint64_t code : morton_codes) {
    if ((code >> kMortonRadixEndBit) != 0U) {
      throw std::invalid_argument(
          "a Phase 14 Morton code uses bits outside the 63-bit grid");
    }
  }
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void allocate(std::size_t count, const char* operation) {
    if (data_ != nullptr || count == 0U ||
        count >
            std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 14 Morton device allocation extent is invalid");
    }
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&data_),
            count * sizeof(Value)),
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

  void abandon() noexcept {
    data_ = nullptr;
    count_ = 0U;
  }

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] const Value* get() const noexcept { return data_; }
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

 private:
  Value* data_{nullptr};
  std::size_t count_{};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 14 Morton build");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 14 Morton build");
      restore_required_ = true;
    }
  }

  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() { restore_noexcept(); }

  void restore() {
    if (restore_required_) {
      check_cuda(
          cudaSetDevice(previous_device_),
          "cudaSetDevice restore after Phase 14 Morton build");
      restore_required_ = false;
    }
  }

 private:
  void restore_noexcept() noexcept {
    if (restore_required_) {
      static_cast<void>(cudaSetDevice(previous_device_));
      restore_required_ = false;
    }
  }

  int previous_device_{};
  bool restore_required_{false};
};

class Phase14MortonLbvhCudaResources final {
 public:
  explicit Phase14MortonLbvhCudaResources(
      std::size_t maximum_point_count)
      : maximum_point_count_(maximum_point_count),
        maximum_axis_count_(
            checked_axis_count(maximum_point_count)),
        maximum_node_count_(
            checked_node_count(maximum_point_count)) {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 14 Morton context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 14 Morton context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional Phase 14 Morton grid");
    }
    maximum_grid_x_ =
        static_cast<unsigned int>(properties.maxGridSize[0]);
    cudaStream_t created_stream = nullptr;
    check_cuda(
        cudaStreamCreateWithFlags(
            &created_stream, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 14 Morton context");
    try {
      coordinate_bits_.allocate(
          maximum_axis_count_,
          "cudaMalloc Phase 14 Morton coordinate capacity");
      encoded_bins_.allocate(
          maximum_axis_count_,
          "cudaMalloc Phase 14 Morton encoded-bin capacity");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(created_stream));
      throw;
    }
    stream_ = created_stream;
  }

  Phase14MortonLbvhCudaResources(
      const Phase14MortonLbvhCudaResources&) = delete;
  Phase14MortonLbvhCudaResources& operator=(
      const Phase14MortonLbvhCudaResources&) = delete;

  ~Phase14MortonLbvhCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      abandon_all();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    reset_all();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] std::size_t maximum_point_count() const noexcept {
    return maximum_point_count_;
  }
  [[nodiscard]] std::size_t maximum_node_count() const noexcept {
    return maximum_node_count_;
  }
  [[nodiscard]] std::size_t sort_temporary_byte_capacity() const noexcept {
    return sort_temporary_byte_capacity_;
  }
  [[nodiscard]] std::size_t resident_point_count() const noexcept {
    return resident_point_count_;
  }
  void set_resident_point_count(std::size_t point_count) noexcept {
    resident_point_count_ = point_count;
  }
  [[nodiscard]] std::uint64_t* coordinate_bits() noexcept {
    return coordinate_bits_.get();
  }
  [[nodiscard]] std::uint32_t* encoded_bins() noexcept {
    return encoded_bins_.get();
  }
  [[nodiscard]] std::uint64_t* morton_keys_a() noexcept {
    return morton_keys_a_.get();
  }
  [[nodiscard]] std::uint64_t* morton_keys_b() noexcept {
    return morton_keys_b_.get();
  }
  [[nodiscard]] std::uint64_t* point_ids_a() noexcept {
    return point_ids_a_.get();
  }
  [[nodiscard]] std::uint64_t* point_ids_b() noexcept {
    return point_ids_b_.get();
  }
  [[nodiscard]] Phase14MortonLbvhDeviceLeaf* leaves() noexcept {
    return leaves_.get();
  }
  [[nodiscard]] Phase14MortonLbvhDeviceNode* nodes() noexcept {
    return nodes_.get();
  }
  [[nodiscard]] Phase14MortonLbvhRange* frontier_a() noexcept {
    return frontier_a_.get();
  }
  [[nodiscard]] Phase14MortonLbvhRange* frontier_b() noexcept {
    return frontier_b_.get();
  }
  [[nodiscard]] std::uint64_t* level_node_indices() noexcept {
    return level_node_indices_.get();
  }
  [[nodiscard]] unsigned long long* frontier_count() noexcept {
    return frontier_count_.get();
  }
  [[nodiscard]] unsigned long long* failure_code() noexcept {
    return failure_code_.get();
  }
  [[nodiscard]] unsigned long long* collision_group_count() noexcept {
    return collision_group_count_.get();
  }
  [[nodiscard]] unsigned long long* maximum_collision_size() noexcept {
    return maximum_collision_size_.get();
  }
  [[nodiscard]] std::byte* sort_temporary_storage() noexcept {
    return sort_temporary_storage_.get();
  }

  void initialize_builder_buffers() {
    if (builder_initialized_) {
      return;
    }
    if (maximum_point_count_ >
        static_cast<std::size_t>(INT_MAX)) {
      throw std::length_error(
          "the Phase 14 Morton CUB radix-sort capacity exceeds INT_MAX");
    }

    morton_keys_a_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton input-key capacity");
    morton_keys_b_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton output-key capacity");
    point_ids_a_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton input-PointId capacity");
    point_ids_b_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton output-PointId capacity");
    leaves_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton leaf capacity");
    nodes_.allocate(
        maximum_node_count_,
        "cudaMalloc Phase 14 Morton node capacity");
    frontier_a_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton frontier-A capacity");
    frontier_b_.allocate(
        maximum_point_count_,
        "cudaMalloc Phase 14 Morton frontier-B capacity");
    level_node_indices_.allocate(
        maximum_node_count_,
        "cudaMalloc Phase 14 Morton level-index capacity");
    frontier_count_.allocate(
        1U,
        "cudaMalloc Phase 14 Morton frontier counter");
    failure_code_.allocate(
        1U,
        "cudaMalloc Phase 14 Morton failure code");
    collision_group_count_.allocate(
        1U,
        "cudaMalloc Phase 14 Morton collision-group counter");
    maximum_collision_size_.allocate(
        1U,
        "cudaMalloc Phase 14 Morton maximum-collision counter");

    cub::DoubleBuffer<std::uint64_t> keys{
        morton_keys_a_.get(), morton_keys_b_.get()};
    cub::DoubleBuffer<std::uint64_t> values{
        point_ids_a_.get(), point_ids_b_.get()};
    std::size_t temporary_bytes = 0U;
    check_cuda(
        cub::DeviceRadixSort::SortPairs(
            nullptr,
            temporary_bytes,
            keys,
            values,
            static_cast<int>(maximum_point_count_),
            0,
            kMortonRadixEndBit,
            stream_),
        "CUB Phase 14 Morton radix-sort storage query");
    if (temporary_bytes == 0U) {
      throw std::runtime_error(
          "CUB returned an empty Phase 14 Morton sort workspace");
    }
    sort_temporary_storage_.allocate(
        temporary_bytes,
        "cudaMalloc Phase 14 Morton radix-sort workspace");
    sort_temporary_byte_capacity_ = temporary_bytes;
    builder_initialized_ = true;
  }

 private:
  void abandon_all() noexcept {
    sort_temporary_storage_.abandon();
    maximum_collision_size_.abandon();
    collision_group_count_.abandon();
    failure_code_.abandon();
    frontier_count_.abandon();
    level_node_indices_.abandon();
    frontier_b_.abandon();
    frontier_a_.abandon();
    nodes_.abandon();
    leaves_.abandon();
    point_ids_b_.abandon();
    point_ids_a_.abandon();
    morton_keys_b_.abandon();
    morton_keys_a_.abandon();
    encoded_bins_.abandon();
    coordinate_bits_.abandon();
  }

  void reset_all() noexcept {
    sort_temporary_storage_.reset();
    maximum_collision_size_.reset();
    collision_group_count_.reset();
    failure_code_.reset();
    frontier_count_.reset();
    level_node_indices_.reset();
    frontier_b_.reset();
    frontier_a_.reset();
    nodes_.reset();
    leaves_.reset();
    point_ids_b_.reset();
    point_ids_a_.reset();
    morton_keys_b_.reset();
    morton_keys_a_.reset();
    encoded_bins_.reset();
    coordinate_bits_.reset();
  }

  int device_{};
  unsigned int maximum_grid_x_{};
  cudaStream_t stream_{nullptr};
  std::size_t maximum_point_count_{};
  std::size_t maximum_axis_count_{};
  std::size_t maximum_node_count_{};
  std::size_t resident_point_count_{};
  std::size_t sort_temporary_byte_capacity_{};
  bool builder_initialized_{false};
  DeviceBuffer<std::uint64_t> coordinate_bits_;
  DeviceBuffer<std::uint32_t> encoded_bins_;
  DeviceBuffer<std::uint64_t> morton_keys_a_;
  DeviceBuffer<std::uint64_t> morton_keys_b_;
  DeviceBuffer<std::uint64_t> point_ids_a_;
  DeviceBuffer<std::uint64_t> point_ids_b_;
  DeviceBuffer<Phase14MortonLbvhDeviceLeaf> leaves_;
  DeviceBuffer<Phase14MortonLbvhDeviceNode> nodes_;
  DeviceBuffer<Phase14MortonLbvhRange> frontier_a_;
  DeviceBuffer<Phase14MortonLbvhRange> frontier_b_;
  DeviceBuffer<std::uint64_t> level_node_indices_;
  DeviceBuffer<unsigned long long> frontier_count_;
  DeviceBuffer<unsigned long long> failure_code_;
  DeviceBuffer<unsigned long long> collision_group_count_;
  DeviceBuffer<unsigned long long> maximum_collision_size_;
  DeviceBuffer<std::byte> sort_temporary_storage_;
};

[[nodiscard]] std::shared_ptr<Phase14MortonLbvhCudaResources>
require_resources(
    Phase14MortonLbvhBuildContextState& context,
    std::size_t maximum_point_count) {
  std::shared_ptr<void>& opaque = context.cuda_resources();
  if (!opaque) {
    auto resources =
        std::make_shared<Phase14MortonLbvhCudaResources>(
            maximum_point_count);
    opaque = resources;
    return resources;
  }
  auto resources =
      std::static_pointer_cast<Phase14MortonLbvhCudaResources>(
          opaque);
  if (resources->maximum_point_count() != maximum_point_count) {
    throw std::logic_error(
        "the Phase 14 Morton context changed its fixed point capacity");
  }
  return resources;
}

[[nodiscard]] __device__ std::uint32_t proposed_morton_bin(
    std::uint64_t coordinate_word,
    std::uint64_t lower_word,
    std::uint64_t upper_word) {
  const double coordinate =
      __longlong_as_double(
          static_cast<long long>(coordinate_word));
  const double lower =
      __longlong_as_double(static_cast<long long>(lower_word));
  const double upper =
      __longlong_as_double(static_cast<long long>(upper_word));

  if (lower == upper) {
    return 0U;
  }
  if (coordinate == lower) {
    return 0U;
  }
  if (coordinate == upper) {
    return phase14_morton_bin_value_mask;
  }
  if (!(lower < coordinate && coordinate < upper)) {
    return phase14_morton_bin_ambiguous_bit;
  }

  // Outward-rounded interval for
  // 2^21 (x-l)/(u-l).  A common floor proves the exact dyadic quotient's
  // bin.  Any overflow, underflow loss, integer-boundary overlap or other
  // uncertainty is represented by one ambiguity bit and resolved exactly
  // on the CPU.
  const double numerator_lower = __dsub_rd(coordinate, lower);
  const double numerator_upper = __dsub_ru(coordinate, lower);
  const double denominator_lower = __dsub_rd(upper, lower);
  const double denominator_upper = __dsub_ru(upper, lower);
  if (!(denominator_lower > 0.0) ||
      !isfinite(numerator_lower) ||
      !isfinite(numerator_upper) ||
      !isfinite(denominator_lower) ||
      !isfinite(denominator_upper)) {
    return phase14_morton_bin_ambiguous_bit;
  }
  const double ratio_lower =
      __ddiv_rd(numerator_lower, denominator_upper);
  const double ratio_upper =
      __ddiv_ru(numerator_upper, denominator_lower);
  const double scaled_lower =
      __dmul_rd(ratio_lower, kMortonGridSize);
  const double scaled_upper =
      __dmul_ru(ratio_upper, kMortonGridSize);
  if (!(scaled_lower >= 0.0) ||
      !(scaled_upper <= kMortonGridSize) ||
      !isfinite(scaled_lower) ||
      !isfinite(scaled_upper)) {
    return phase14_morton_bin_ambiguous_bit;
  }
  const unsigned long long lower_bin =
      __double2ull_rd(scaled_lower);
  const unsigned long long upper_bin =
      __double2ull_rd(scaled_upper);
  if (lower_bin != upper_bin ||
      lower_bin >
          static_cast<unsigned long long>(
              phase14_morton_bin_value_mask)) {
    return phase14_morton_bin_ambiguous_bit;
  }
  return static_cast<std::uint32_t>(lower_bin);
}

struct Phase14MortonBoundsWords {
  std::uint64_t lower[3];
  std::uint64_t upper[3];
};

__global__ void propose_morton_bins_kernel(
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    Phase14MortonBoundsWords bounds,
    std::uint32_t* encoded_bins,
    std::size_t axis_count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  if (index >= axis_count) {
    return;
  }
  const std::size_t axis = index / point_count;
  encoded_bins[index] = proposed_morton_bin(
      coordinate_bits[index],
      bounds.lower[axis],
      bounds.upper[axis]);
}

[[nodiscard]] __host__ __device__ constexpr unsigned long long
device_failure_word(
    Phase14MortonLbvhDeviceFailure failure) noexcept {
  return static_cast<unsigned long long>(failure);
}

__device__ void report_device_failure(
    unsigned long long* failure_code,
    Phase14MortonLbvhDeviceFailure failure) {
  static_cast<void>(
      atomicCAS(
          failure_code,
          device_failure_word(
              Phase14MortonLbvhDeviceFailure::none),
          device_failure_word(failure)));
}

__global__ void initialize_point_ids_kernel(
    std::uint64_t* point_ids,
    std::size_t point_count) {
  const std::size_t point_index =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  if (point_index < point_count) {
    point_ids[point_index] =
        static_cast<std::uint64_t>(point_index);
  }
}

__global__ void emit_morton_leaves_kernel(
    const std::uint64_t* sorted_codes,
    const std::uint64_t* sorted_point_ids,
    Phase14MortonLbvhDeviceLeaf* leaves,
    std::size_t point_count) {
  const std::size_t position =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  if (position < point_count) {
    leaves[position].morton_code = sorted_codes[position];
    leaves[position].point_id = sorted_point_ids[position];
  }
}

__global__ void initialize_root_range_kernel(
    Phase14MortonLbvhRange* frontier,
    std::uint64_t point_count) {
  if (blockIdx.x == 0U && threadIdx.x == 0U) {
    frontier[0U] =
        Phase14MortonLbvhRange{0U, point_count, 0U};
  }
}

[[nodiscard]] __device__ std::uint64_t device_find_split(
    const std::uint64_t* sorted_codes,
    std::uint64_t begin,
    std::uint64_t end,
    unsigned long long* failure_code) {
  if (begin + UINT64_C(1) >= end) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_range);
    return begin;
  }
  const std::uint64_t first_code = sorted_codes[begin];
  const std::uint64_t last_code = sorted_codes[end - UINT64_C(1)];
  if (first_code == last_code) {
    return begin + (end - begin) / UINT64_C(2);
  }
  const std::uint64_t difference = first_code ^ last_code;
  const unsigned int highest_bit =
      63U - static_cast<unsigned int>(__clzll(difference));
  const std::uint64_t mask =
      std::uint64_t{1} << highest_bit;
  std::uint64_t low = begin + UINT64_C(1);
  std::uint64_t high = end;
  while (low < high) {
    const std::uint64_t middle =
        low + (high - low) / UINT64_C(2);
    if ((sorted_codes[middle] & mask) == 0U) {
      low = middle + UINT64_C(1);
    } else {
      high = middle;
    }
  }
  if (low <= begin || low >= end) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_split);
    return begin;
  }
  return low;
}

__global__ void build_morton_topology_level_kernel(
    const Phase14MortonLbvhRange* current_frontier,
    std::uint64_t current_count,
    const std::uint64_t* sorted_codes,
    const std::uint64_t* sorted_point_ids,
    Phase14MortonLbvhDeviceNode* nodes,
    std::uint64_t node_count,
    Phase14MortonLbvhRange* next_frontier,
    unsigned long long* next_count,
    std::uint64_t frontier_capacity,
    std::uint64_t* level_node_indices,
    std::uint64_t level_begin,
    unsigned long long* failure_code) {
  const std::uint64_t range_index =
      static_cast<std::uint64_t>(blockIdx.x) *
          static_cast<std::uint64_t>(blockDim.x) +
      static_cast<std::uint64_t>(threadIdx.x);
  if (range_index >= current_count) {
    return;
  }
  if (level_begin > node_count ||
      range_index >= node_count - level_begin) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::level_index_capacity);
    return;
  }

  const Phase14MortonLbvhRange range =
      current_frontier[range_index];
  if (range.leaf_begin >= range.leaf_end ||
      range.leaf_end > frontier_capacity) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_range);
    return;
  }
  const std::uint64_t leaf_count =
      range.leaf_end - range.leaf_begin;
  if (leaf_count - UINT64_C(1) >
          std::numeric_limits<std::uint64_t>::max() /
              UINT64_C(2)) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::postorder_index_overflow);
    return;
  }
  const std::uint64_t root_offset =
      UINT64_C(2) * (leaf_count - UINT64_C(1));
  if (range.postorder_begin >
      std::numeric_limits<std::uint64_t>::max() - root_offset) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::postorder_index_overflow);
    return;
  }
  const std::uint64_t root_index =
      range.postorder_begin + root_offset;
  if (root_index >= node_count) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::postorder_index_overflow);
    return;
  }
  level_node_indices[level_begin + range_index] = root_index;

  Phase14MortonLbvhDeviceNode& node = nodes[root_index];
  node.leaf_begin = range.leaf_begin;
  node.leaf_end = range.leaf_end;
  if (leaf_count == UINT64_C(1)) {
    const std::uint64_t point_id =
        sorted_point_ids[range.leaf_begin];
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      node.lower_point_ids[axis] = point_id;
      node.upper_point_ids[axis] = point_id;
    }
    node.left_child =
        spatial::morton_lbvh_snapshot_invalid_node_index;
    node.right_child =
        spatial::morton_lbvh_snapshot_invalid_node_index;
    return;
  }

  const std::uint64_t split = device_find_split(
      sorted_codes,
      range.leaf_begin,
      range.leaf_end,
      failure_code);
  if (split <= range.leaf_begin || split >= range.leaf_end) {
    return;
  }
  const std::uint64_t left_leaf_count =
      split - range.leaf_begin;
  const std::uint64_t right_leaf_count =
      range.leaf_end - split;
  const std::uint64_t right_postorder_begin =
      range.postorder_begin +
      UINT64_C(2) * left_leaf_count - UINT64_C(1);
  const std::uint64_t left_root =
      range.postorder_begin +
      UINT64_C(2) * left_leaf_count - UINT64_C(2);
  const std::uint64_t right_root =
      right_postorder_begin +
      UINT64_C(2) * right_leaf_count - UINT64_C(2);
  if (left_root >= root_index ||
      right_root >= root_index ||
      right_root + UINT64_C(1) != root_index) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::postorder_index_overflow);
    return;
  }
  node.left_child = left_root;
  node.right_child = right_root;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    node.lower_point_ids[axis] =
        spatial::morton_lbvh_snapshot_invalid_node_index;
    node.upper_point_ids[axis] =
        spatial::morton_lbvh_snapshot_invalid_node_index;
  }

  const unsigned long long slot =
      atomicAdd(next_count, 2ULL);
  if (slot >= frontier_capacity ||
      slot + 1ULL >= frontier_capacity) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::frontier_capacity);
    return;
  }
  next_frontier[slot] = Phase14MortonLbvhRange{
      range.leaf_begin,
      split,
      range.postorder_begin};
  next_frontier[slot + 1ULL] = Phase14MortonLbvhRange{
      split,
      range.leaf_end,
      right_postorder_begin};
}

[[nodiscard]] __device__ double binary64_from_bits(
    std::uint64_t bits) {
  return __longlong_as_double(static_cast<long long>(bits));
}

[[nodiscard]] __device__ std::uint64_t device_extremum_witness(
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t left,
    std::uint64_t right,
    std::size_t axis,
    bool maximum,
    unsigned long long* failure_code) {
  if (left >= point_count || right >= point_count) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_witness);
    return 0U;
  }
  const double left_coordinate = binary64_from_bits(
      coordinate_bits[
          static_cast<std::uint64_t>(axis) * point_count + left]);
  const double right_coordinate = binary64_from_bits(
      coordinate_bits[
          static_cast<std::uint64_t>(axis) * point_count + right]);
  if (maximum) {
    if (right_coordinate > left_coordinate ||
        (right_coordinate == left_coordinate && right < left)) {
      return right;
    }
    return left;
  }
  if (right_coordinate < left_coordinate ||
      (right_coordinate == left_coordinate && right < left)) {
    return right;
  }
  return left;
}

__global__ void reduce_morton_aabb_level_kernel(
    const std::uint64_t* level_node_indices,
    std::uint64_t level_begin,
    std::uint64_t level_count,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    Phase14MortonLbvhDeviceNode* nodes,
    std::uint64_t node_count,
    unsigned long long* failure_code) {
  const std::uint64_t level_index =
      static_cast<std::uint64_t>(blockIdx.x) *
          static_cast<std::uint64_t>(blockDim.x) +
      static_cast<std::uint64_t>(threadIdx.x);
  if (level_index >= level_count) {
    return;
  }
  const std::uint64_t node_index =
      level_node_indices[level_begin + level_index];
  if (node_index >= node_count) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_child);
    return;
  }
  Phase14MortonLbvhDeviceNode& node = nodes[node_index];
  if (node.left_child ==
      spatial::morton_lbvh_snapshot_invalid_node_index) {
    return;
  }
  if (node.left_child >= node_index ||
      node.right_child >= node_index ||
      node.left_child >= node_count ||
      node.right_child >= node_count) {
    report_device_failure(
        failure_code,
        Phase14MortonLbvhDeviceFailure::invalid_child);
    return;
  }
  const Phase14MortonLbvhDeviceNode& left =
      nodes[node.left_child];
  const Phase14MortonLbvhDeviceNode& right =
      nodes[node.right_child];
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    node.lower_point_ids[axis] = device_extremum_witness(
        coordinate_bits,
        point_count,
        left.lower_point_ids[axis],
        right.lower_point_ids[axis],
        axis,
        false,
        failure_code);
    node.upper_point_ids[axis] = device_extremum_witness(
        coordinate_bits,
        point_count,
        left.upper_point_ids[axis],
        right.upper_point_ids[axis],
        axis,
        true,
        failure_code);
  }
}

__global__ void count_morton_collisions_kernel(
    const std::uint64_t* sorted_codes,
    std::size_t point_count,
    unsigned long long* collision_group_count,
    unsigned long long* maximum_collision_size) {
  const std::size_t position =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  if (position == 0U || position >= point_count ||
      sorted_codes[position] != sorted_codes[position - 1U] ||
      (position + 1U < point_count &&
       sorted_codes[position] == sorted_codes[position + 1U])) {
    return;
  }
  const std::uint64_t code = sorted_codes[position];
  std::size_t low = 0U;
  std::size_t high = position;
  while (low < high) {
    const std::size_t middle =
        low + (high - low) / 2U;
    if (sorted_codes[middle] < code) {
      low = middle + 1U;
    } else {
      high = middle;
    }
  }
  const unsigned long long group_size =
      static_cast<unsigned long long>(position - low + 1U);
  atomicAdd(collision_group_count, 1ULL);
  atomicMax(maximum_collision_size, group_size);
}

[[nodiscard]] unsigned int block_count(
    std::size_t work_count,
    unsigned int maximum_grid_x) {
  const std::size_t blocks =
      (work_count + kThreadsPerBlock - 1U) /
      kThreadsPerBlock;
  if (blocks == 0U ||
      blocks > static_cast<std::size_t>(maximum_grid_x) ||
      blocks > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error(
        "the Phase 14 Morton proposal grid exceeds CUDA limits");
  }
  return static_cast<unsigned int>(blocks);
}

[[noreturn]] void throw_device_topology_failure(
    unsigned long long failure_code) {
  const auto failure =
      static_cast<Phase14MortonLbvhDeviceFailure>(failure_code);
  switch (failure) {
    case Phase14MortonLbvhDeviceFailure::none:
      break;
    case Phase14MortonLbvhDeviceFailure::invalid_range:
      throw std::runtime_error(
          "the Phase 14 Morton device topology produced an invalid range");
    case Phase14MortonLbvhDeviceFailure::invalid_split:
      throw std::runtime_error(
          "the Phase 14 Morton device find_split did not divide its range");
    case Phase14MortonLbvhDeviceFailure::postorder_index_overflow:
      throw std::runtime_error(
          "the Phase 14 Morton device postorder index overflowed");
    case Phase14MortonLbvhDeviceFailure::frontier_capacity:
      throw std::runtime_error(
          "the Phase 14 Morton device frontier exceeded fixed capacity");
    case Phase14MortonLbvhDeviceFailure::level_index_capacity:
      throw std::runtime_error(
          "the Phase 14 Morton device level index exceeded fixed capacity");
    case Phase14MortonLbvhDeviceFailure::invalid_child:
      throw std::runtime_error(
          "the Phase 14 Morton device AABB reduction found an invalid child");
    case Phase14MortonLbvhDeviceFailure::invalid_witness:
      throw std::runtime_error(
          "the Phase 14 Morton device AABB reduction found an invalid "
          "witness");
  }
  throw std::runtime_error(
      "the Phase 14 Morton device reported an unknown failure code");
}

}  // namespace

Phase14MortonBinProposalBatch propose_phase14_morton_bins_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& lower_coordinate_bits,
    const std::array<std::uint64_t, 3>& upper_coordinate_bits,
    std::size_t maximum_point_count) {
  validate_proposal_inputs(
      axis_major_coordinate_bits,
      point_count,
      lower_coordinate_bits,
      upper_coordinate_bits,
      maximum_point_count);
  const std::size_t axis_count =
      checked_axis_count(point_count);
  auto resources =
      require_resources(context, maximum_point_count);
  DeviceGuard guard{resources->device()};

  check_cuda(
      cudaMemcpyAsync(
          resources->coordinate_bits(),
          axis_major_coordinate_bits.data(),
          axis_major_coordinate_bits.size_bytes(),
          cudaMemcpyHostToDevice,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton coordinates host-to-device");
  const unsigned int blocks =
      block_count(axis_count, resources->maximum_grid_x());
  const Phase14MortonBoundsWords bounds{
      {lower_coordinate_bits[0U],
       lower_coordinate_bits[1U],
       lower_coordinate_bits[2U]},
      {upper_coordinate_bits[0U],
       upper_coordinate_bits[1U],
       upper_coordinate_bits[2U]}};
  propose_morton_bins_kernel<<<
      blocks,
      kThreadsPerBlock,
      0U,
      resources->stream()>>>(
      resources->coordinate_bits(),
      point_count,
      bounds,
      resources->encoded_bins(),
      axis_count);
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 14 Morton bin proposal kernel launch");

  Phase14MortonBinProposalBatch batch;
  batch.encoded_bins.resize(axis_count);
  check_cuda(
      cudaMemcpyAsync(
          batch.encoded_bins.data(),
          resources->encoded_bins(),
          batch.encoded_bins.size() * sizeof(std::uint32_t),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton bins device-to-host");
  check_cuda(
      cudaStreamSynchronize(resources->stream()),
      "cudaStreamSynchronize Phase 14 Morton bin proposal");
  resources->set_resident_point_count(point_count);
  batch.axis_count = axis_count;
  batch.host_to_device_coordinate_byte_count =
      axis_major_coordinate_bits.size_bytes();
  batch.device_to_host_encoded_bin_byte_count =
      batch.encoded_bins.size() * sizeof(std::uint32_t);
  batch.kernel_launch_count = 1U;
  batch.synchronization_count = 1U;
  batch.buffer_epoch = context.advance_epoch();
  batch.execution_kind = Phase14MortonLbvhExecutionKind::cuda;
  batch.cuda_path_qualified = true;
  guard.restore();
  return batch;
}

Phase14MortonLbvhSnapshotBatch
build_phase14_morton_lbvh_snapshot_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> certified_morton_codes,
    std::size_t maximum_point_count) {
  validate_snapshot_inputs(
      axis_major_coordinate_bits,
      point_count,
      certified_morton_codes,
      maximum_point_count);
  auto resources =
      require_resources(context, maximum_point_count);
  DeviceGuard guard{resources->device()};
  if (resources->resident_point_count() != point_count) {
    throw std::logic_error(
        "the Phase 14 Morton snapshot does not match the resident bin "
        "proposal coordinates");
  }
  resources->initialize_builder_buffers();

  const std::size_t node_count =
      checked_node_count(point_count);
  const std::uint64_t point_count_u64 =
      static_cast<std::uint64_t>(point_count);
  const std::uint64_t node_count_u64 =
      static_cast<std::uint64_t>(node_count);
  const unsigned int point_blocks =
      block_count(point_count, resources->maximum_grid_x());

  check_cuda(
      cudaMemcpyAsync(
          resources->morton_keys_a(),
          certified_morton_codes.data(),
          certified_morton_codes.size_bytes(),
          cudaMemcpyHostToDevice,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton codes host-to-device");
  initialize_point_ids_kernel<<<
      point_blocks,
      kThreadsPerBlock,
      0U,
      resources->stream()>>>(
      resources->point_ids_a(), point_count);
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 14 Morton PointId initialization kernel launch");

  cub::DoubleBuffer<std::uint64_t> keys{
      resources->morton_keys_a(),
      resources->morton_keys_b()};
  cub::DoubleBuffer<std::uint64_t> point_ids{
      resources->point_ids_a(),
      resources->point_ids_b()};
  std::size_t active_sort_temporary_bytes =
      resources->sort_temporary_byte_capacity();
  check_cuda(
      cub::DeviceRadixSort::SortPairs(
          resources->sort_temporary_storage(),
          active_sort_temporary_bytes,
          keys,
          point_ids,
          static_cast<int>(point_count),
          0,
          kMortonRadixEndBit,
          resources->stream()),
      "CUB Phase 14 stable Morton/PointId radix sort");
  if (active_sort_temporary_bytes >
      resources->sort_temporary_byte_capacity()) {
    throw std::runtime_error(
        "CUB exceeded the fixed Phase 14 Morton sort workspace");
  }
  const std::uint64_t* const sorted_codes = keys.Current();
  const std::uint64_t* const sorted_point_ids =
      point_ids.Current();

  emit_morton_leaves_kernel<<<
      point_blocks,
      kThreadsPerBlock,
      0U,
      resources->stream()>>>(
      sorted_codes,
      sorted_point_ids,
      resources->leaves(),
      point_count);
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 14 Morton leaf emission kernel launch");

  check_cuda(
      cudaMemsetAsync(
          resources->failure_code(),
          0,
          sizeof(unsigned long long),
          resources->stream()),
      "cudaMemsetAsync Phase 14 Morton failure code");
  check_cuda(
      cudaMemsetAsync(
          resources->collision_group_count(),
          0,
          sizeof(unsigned long long),
          resources->stream()),
      "cudaMemsetAsync Phase 14 Morton collision-group counter");
  check_cuda(
      cudaMemsetAsync(
          resources->maximum_collision_size(),
          0,
          sizeof(unsigned long long),
          resources->stream()),
      "cudaMemsetAsync Phase 14 Morton maximum-collision counter");
  initialize_root_range_kernel<<<
      1U,
      1U,
      0U,
      resources->stream()>>>(
      resources->frontier_a(), point_count_u64);
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 14 Morton root-range initialization kernel launch");

  struct LevelRecord {
    std::uint64_t begin;
    std::uint64_t count;
  };
  const std::size_t topology_round_limit =
      maximum_topology_round_count(point_count);
  std::vector<LevelRecord> levels;
  levels.reserve(topology_round_limit);
  Phase14MortonLbvhRange* current_frontier =
      resources->frontier_a();
  Phase14MortonLbvhRange* next_frontier =
      resources->frontier_b();
  unsigned long long current_count = 1ULL;
  std::uint64_t level_begin = 0U;
  std::size_t maximum_depth = 0U;
  std::size_t topology_kernel_count = 0U;
  std::size_t synchronization_count = 0U;

  while (current_count != 0ULL) {
    if (levels.size() >= topology_round_limit ||
        level_begin > node_count_u64 ||
        current_count >
            static_cast<unsigned long long>(
                node_count_u64 - level_begin)) {
      throw std::runtime_error(
          "the Phase 14 Morton topology exceeded its proven round or node "
          "bound");
    }
    levels.push_back(LevelRecord{
        level_begin,
        static_cast<std::uint64_t>(current_count)});
    check_cuda(
        cudaMemsetAsync(
            resources->frontier_count(),
            0,
            sizeof(unsigned long long),
            resources->stream()),
        "cudaMemsetAsync Phase 14 Morton next-frontier count");
    const unsigned int topology_blocks = block_count(
        static_cast<std::size_t>(current_count),
        resources->maximum_grid_x());
    build_morton_topology_level_kernel<<<
        topology_blocks,
        kThreadsPerBlock,
        0U,
        resources->stream()>>>(
        current_frontier,
        static_cast<std::uint64_t>(current_count),
        sorted_codes,
        sorted_point_ids,
        resources->nodes(),
        node_count_u64,
        next_frontier,
        resources->frontier_count(),
        point_count_u64,
        resources->level_node_indices(),
        level_begin,
        resources->failure_code());
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 14 Morton topology-level kernel launch");
    ++topology_kernel_count;

    unsigned long long next_count = 0ULL;
    unsigned long long failure_code = 0ULL;
    check_cuda(
        cudaMemcpyAsync(
            &next_count,
            resources->frontier_count(),
            sizeof(next_count),
            cudaMemcpyDeviceToHost,
            resources->stream()),
        "cudaMemcpyAsync Phase 14 Morton next-frontier count");
    check_cuda(
        cudaMemcpyAsync(
            &failure_code,
            resources->failure_code(),
            sizeof(failure_code),
            cudaMemcpyDeviceToHost,
            resources->stream()),
        "cudaMemcpyAsync Phase 14 Morton topology failure code");
    check_cuda(
        cudaStreamSynchronize(resources->stream()),
        "cudaStreamSynchronize Phase 14 Morton topology level");
    ++synchronization_count;
    if (failure_code !=
        device_failure_word(
            Phase14MortonLbvhDeviceFailure::none)) {
      throw_device_topology_failure(failure_code);
    }
    if (next_count >
        static_cast<unsigned long long>(point_count)) {
      throw std::runtime_error(
          "the Phase 14 Morton device returned an oversized frontier");
    }
    level_begin += static_cast<std::uint64_t>(current_count);
    maximum_depth = levels.size() - 1U;
    current_count = next_count;
    std::swap(current_frontier, next_frontier);
  }
  if (level_begin != node_count_u64 || levels.empty()) {
    throw std::runtime_error(
        "the Phase 14 Morton topology did not emit exactly 2n-1 nodes");
  }

  std::size_t aabb_kernel_count = 0U;
  for (std::size_t level_position = levels.size();
       level_position > 1U;
       --level_position) {
    const LevelRecord& level =
        levels[level_position - 2U];
    const unsigned int aabb_blocks = block_count(
        static_cast<std::size_t>(level.count),
        resources->maximum_grid_x());
    reduce_morton_aabb_level_kernel<<<
        aabb_blocks,
        kThreadsPerBlock,
        0U,
        resources->stream()>>>(
        resources->level_node_indices(),
        level.begin,
        level.count,
        resources->coordinate_bits(),
        point_count_u64,
        resources->nodes(),
        node_count_u64,
        resources->failure_code());
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 14 Morton AABB reduction kernel launch");
    ++aabb_kernel_count;
  }

  count_morton_collisions_kernel<<<
      point_blocks,
      kThreadsPerBlock,
      0U,
      resources->stream()>>>(
      sorted_codes,
      point_count,
      resources->collision_group_count(),
      resources->maximum_collision_size());
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 14 Morton collision counter kernel launch");

  Phase14MortonLbvhSnapshotBatch batch;
  batch.point_count = point_count_u64;
  batch.root_node_index = node_count_u64 - UINT64_C(1);
  batch.leaves.resize(point_count);
  batch.nodes.resize(node_count);
  check_cuda(
      cudaMemcpyAsync(
          batch.leaves.data(),
          resources->leaves(),
          batch.leaves.size() *
              sizeof(spatial::MortonLbvhSnapshotLeaf),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton leaves device-to-host");
  check_cuda(
      cudaMemcpyAsync(
          batch.nodes.data(),
          resources->nodes(),
          batch.nodes.size() *
              sizeof(spatial::MortonLbvhSnapshotNode),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton nodes device-to-host");

  unsigned long long final_failure_code = 0ULL;
  unsigned long long collision_group_count = 0ULL;
  unsigned long long maximum_collision_size = 0ULL;
  check_cuda(
      cudaMemcpyAsync(
          &final_failure_code,
          resources->failure_code(),
          sizeof(final_failure_code),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton final failure code");
  check_cuda(
      cudaMemcpyAsync(
          &collision_group_count,
          resources->collision_group_count(),
          sizeof(collision_group_count),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton collision-group count");
  check_cuda(
      cudaMemcpyAsync(
          &maximum_collision_size,
          resources->maximum_collision_size(),
          sizeof(maximum_collision_size),
          cudaMemcpyDeviceToHost,
          resources->stream()),
      "cudaMemcpyAsync Phase 14 Morton maximum collision size");
  check_cuda(
      cudaStreamSynchronize(resources->stream()),
      "cudaStreamSynchronize Phase 14 Morton snapshot");
  ++synchronization_count;
  if (final_failure_code !=
      device_failure_word(
          Phase14MortonLbvhDeviceFailure::none)) {
    throw_device_topology_failure(final_failure_code);
  }

  batch.proposed_counters = spatial::MortonLbvhSnapshotCounters{
      point_count_u64,
      node_count_u64,
      static_cast<std::uint64_t>(maximum_depth),
      static_cast<std::uint64_t>(collision_group_count),
      static_cast<std::uint64_t>(maximum_collision_size)};
  batch.host_to_device_morton_code_byte_count =
      certified_morton_codes.size_bytes();
  batch.device_to_host_leaf_byte_count =
      batch.leaves.size() *
      sizeof(spatial::MortonLbvhSnapshotLeaf);
  batch.device_to_host_node_byte_count =
      batch.nodes.size() *
      sizeof(spatial::MortonLbvhSnapshotNode);
  const std::size_t maximum_axis_count =
      checked_axis_count(resources->maximum_point_count());
  batch.device_coordinate_byte_capacity = checked_byte_product(
      maximum_axis_count,
      sizeof(std::uint64_t),
      "the Phase 14 Morton coordinate byte capacity overflows");
  batch.device_encoded_bin_byte_capacity = checked_byte_product(
      maximum_axis_count,
      sizeof(std::uint32_t),
      "the Phase 14 Morton encoded-bin byte capacity overflows");
  batch.device_morton_code_double_buffer_byte_capacity =
      checked_byte_product(
          resources->maximum_point_count(),
          2U * sizeof(std::uint64_t),
          "the Phase 14 Morton code double-buffer capacity overflows");
  batch.device_point_id_double_buffer_byte_capacity =
      checked_byte_product(
          resources->maximum_point_count(),
          2U * sizeof(std::uint64_t),
          "the Phase 14 Morton PointId double-buffer capacity overflows");
  batch.device_leaf_byte_capacity = checked_byte_product(
      resources->maximum_point_count(),
      sizeof(Phase14MortonLbvhDeviceLeaf),
      "the Phase 14 Morton leaf byte capacity overflows");
  batch.device_node_byte_capacity = checked_byte_product(
      resources->maximum_node_count(),
      sizeof(Phase14MortonLbvhDeviceNode),
      "the Phase 14 Morton node byte capacity overflows");
  batch.device_frontier_double_buffer_byte_capacity =
      checked_byte_product(
          resources->maximum_point_count(),
          2U * sizeof(Phase14MortonLbvhRange),
          "the Phase 14 Morton frontier byte capacity overflows");
  batch.device_level_schedule_byte_capacity = checked_byte_product(
      resources->maximum_node_count(),
      sizeof(std::uint64_t),
      "the Phase 14 Morton level-schedule byte capacity overflows");
  batch.device_control_byte_capacity =
      4U * sizeof(unsigned long long);
  batch.sort_temporary_byte_capacity =
      resources->sort_temporary_byte_capacity();
  const auto add_device_capacity =
      [&batch](std::size_t bytes) {
        batch.total_fixed_device_byte_capacity = checked_byte_sum(
            batch.total_fixed_device_byte_capacity,
            bytes,
            "the Phase 14 total fixed device capacity overflows");
      };
  add_device_capacity(batch.device_coordinate_byte_capacity);
  add_device_capacity(batch.device_encoded_bin_byte_capacity);
  add_device_capacity(
      batch.device_morton_code_double_buffer_byte_capacity);
  add_device_capacity(
      batch.device_point_id_double_buffer_byte_capacity);
  add_device_capacity(batch.device_leaf_byte_capacity);
  add_device_capacity(batch.device_node_byte_capacity);
  add_device_capacity(
      batch.device_frontier_double_buffer_byte_capacity);
  add_device_capacity(batch.device_level_schedule_byte_capacity);
  add_device_capacity(batch.device_control_byte_capacity);
  add_device_capacity(batch.sort_temporary_byte_capacity);
  // Only the project-owned kernels named above are counted as launches.
  // CUB's stable radix-sort call is one separate opaque library submission;
  // its implementation-private kernels are deliberately not guessed here.
  batch.kernel_launch_count =
      4U + topology_kernel_count + aabb_kernel_count;
  batch.library_submission_count = 1U;
  batch.synchronization_count = synchronization_count;
  batch.buffer_epoch = context.advance_epoch();
  batch.execution_kind =
      Phase14MortonLbvhExecutionKind::cuda;
  batch.cuda_path_qualified = true;
  guard.restore();
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
