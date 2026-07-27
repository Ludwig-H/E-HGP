#include "phase14_binary64_lbvh_top_k_internal.hpp"

#include <cuda_runtime.h>
#include <cuda/std/bit>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase14_binary64_lbvh_top_k.cu must be compiled with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The binary64 LBVH top-k prototype requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for binary64 LBVH top-k"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The binary64 LBVH top-k prototype must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

static_assert(sizeof(spatial::PointId) == sizeof(std::uint64_t));

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kBlocksPerMultiprocessor = 32U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kMaximumOrder = 10U;
constexpr std::uint64_t kInvalidIndex = UINT64_MAX;
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);

struct Binary64LbvhDeviceNode {
  std::uint64_t lower_point_ids[kAxisCount];
  std::uint64_t upper_point_ids[kAxisCount];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(sizeof(Binary64LbvhDeviceNode) == 80U);
static_assert(alignof(Binary64LbvhDeviceNode) == alignof(std::uint64_t));
static_assert(std::is_standard_layout_v<Binary64LbvhDeviceNode>);
static_assert(std::is_trivially_copyable_v<Binary64LbvhDeviceNode>);
static_assert(
    sizeof(Binary64LbvhDeviceNode) ==
    sizeof(spatial::MortonLbvhSnapshotNode));
static_assert(
    alignof(Binary64LbvhDeviceNode) ==
    alignof(spatial::MortonLbvhSnapshotNode));
static_assert(
    offsetof(Binary64LbvhDeviceNode, lower_point_ids) ==
    offsetof(spatial::MortonLbvhSnapshotNode, lower_point_ids));
static_assert(
    offsetof(Binary64LbvhDeviceNode, upper_point_ids) ==
    offsetof(spatial::MortonLbvhSnapshotNode, upper_point_ids));
static_assert(
    offsetof(Binary64LbvhDeviceNode, left_child) ==
    offsetof(spatial::MortonLbvhSnapshotNode, left_child));
static_assert(
    offsetof(Binary64LbvhDeviceNode, right_child) ==
    offsetof(spatial::MortonLbvhSnapshotNode, right_child));
static_assert(
    offsetof(Binary64LbvhDeviceNode, leaf_begin) ==
    offsetof(spatial::MortonLbvhSnapshotNode, leaf_begin));
static_assert(
    offsetof(Binary64LbvhDeviceNode, leaf_end) ==
    offsetof(spatial::MortonLbvhSnapshotNode, leaf_end));

struct Binary64LbvhDeviceQueryAudit {
  std::uint64_t node_visit_count{};
  std::uint64_t strict_aabb_prune_count{};
  std::uint64_t seed_covered_subtree_skip_count{};
  std::uint64_t leaf_distance_evaluation_count{};
  std::uint32_t invalid_aabb_bound_descent_count{};
  std::uint32_t failure_code{};
};

static_assert(sizeof(Binary64LbvhDeviceQueryAudit) == 40U);

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(
            std::move(operation) + " failed: " + cudaGetErrorString(code)) {}
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

class DeviceGuard final {
 public:
  explicit DeviceGuard(int requested_device) {
    check_cuda(cudaGetDevice(&previous_device_), "cudaGetDevice");
    if (previous_device_ != requested_device) {
      check_cuda(cudaSetDevice(requested_device), "cudaSetDevice");
      changed_ = true;
    }
  }

  ~DeviceGuard() {
    if (changed_) {
      static_cast<void>(cudaSetDevice(previous_device_));
    }
  }

  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;

 private:
  int previous_device_{};
  bool changed_{};
};

template <typename Value>
class DeviceBuffer final {
 public:
  explicit DeviceBuffer(std::size_t count) {
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "the binary64 LBVH top-k device allocation extent is invalid");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
        "cudaMalloc binary64 LBVH top-k");
  }

  ~DeviceBuffer() {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
    }
  }

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] const Value* get() const noexcept { return data_; }

 private:
  Value* data_{};
};

class CudaEvent final {
 public:
  CudaEvent() { check_cuda(cudaEventCreate(&event_), "cudaEventCreate"); }
  ~CudaEvent() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;

  void record() { check_cuda(cudaEventRecord(event_), "cudaEventRecord"); }
  void synchronize() {
    check_cuda(cudaEventSynchronize(event_), "cudaEventSynchronize");
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

template <typename Duration>
[[nodiscard]] std::uint64_t duration_nanoseconds(Duration duration) {
  const auto count =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (count < 0) {
    throw std::runtime_error(
        "the binary64 LBVH top-k steady clock moved backwards");
  }
  return static_cast<std::uint64_t>(count);
}

[[nodiscard]] std::uint64_t event_nanoseconds(
    const CudaEvent& begin,
    const CudaEvent& end) {
  float milliseconds = 0.0F;
  check_cuda(
      cudaEventElapsedTime(&milliseconds, begin.get(), end.get()),
      "cudaEventElapsedTime");
  if (!(milliseconds >= 0.0F)) {
    throw std::runtime_error("CUDA returned a negative event duration");
  }
  return static_cast<std::uint64_t>(
      static_cast<double>(milliseconds) * 1'000'000.0);
}

[[nodiscard]] __device__ bool finite_bits(std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

[[nodiscard]] __device__ bool finite_value(double value) noexcept {
  return finite_bits(cuda::std::bit_cast<std::uint64_t>(value));
}

[[nodiscard]] __device__ double value_from_bits(
    std::uint64_t bits) noexcept {
  return cuda::std::bit_cast<double>(bits);
}

[[nodiscard]] __device__ bool nearer(
    double candidate_distance,
    std::uint64_t candidate_id,
    double incumbent_distance,
    std::uint64_t incumbent_id) noexcept {
  return candidate_distance < incumbent_distance ||
         (candidate_distance == incumbent_distance &&
          candidate_id < incumbent_id);
}

[[nodiscard]] __device__ double fixed_squared_distance(
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    std::uint64_t first,
    std::uint64_t second) noexcept {
  const double first_x = value_from_bits(coordinate_bits[first]);
  const double first_y =
      value_from_bits(coordinate_bits[point_count + first]);
  const double first_z =
      value_from_bits(coordinate_bits[2U * point_count + first]);
  const double second_x = value_from_bits(coordinate_bits[second]);
  const double second_y =
      value_from_bits(coordinate_bits[point_count + second]);
  const double second_z =
      value_from_bits(coordinate_bits[2U * point_count + second]);
  const double dx = __dsub_rn(first_x, second_x);
  const double dy = __dsub_rn(first_y, second_y);
  const double dz = __dsub_rn(first_z, second_z);
  const double xx = __dmul_rn(dx, dx);
  const double yy = __dmul_rn(dy, dy);
  const double zz = __dmul_rn(dz, dz);
  return __dadd_rn(__dadd_rn(xx, yy), zz);
}

__device__ void insert_candidate(
    double candidate_distance,
    std::uint64_t candidate_id,
    std::size_t maximum_order,
    double* best_distances,
    std::uint64_t* best_ids) noexcept {
  if (!nearer(
          candidate_distance,
          candidate_id,
          best_distances[maximum_order - 1U],
          best_ids[maximum_order - 1U])) {
    return;
  }
  std::size_t insertion = maximum_order - 1U;
  while (insertion > 0U &&
         nearer(
             candidate_distance,
             candidate_id,
             best_distances[insertion - 1U],
             best_ids[insertion - 1U])) {
    best_distances[insertion] = best_distances[insertion - 1U];
    best_ids[insertion] = best_ids[insertion - 1U];
    --insertion;
  }
  best_distances[insertion] = candidate_distance;
  best_ids[insertion] = candidate_id;
}

struct DirectedAabbLowerBound {
  double value{};
  bool valid{true};
};

[[nodiscard]] __device__ DirectedAabbLowerBound directed_aabb_lower_bound(
    const Binary64LbvhDeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    std::uint64_t query_id) noexcept {
  DirectedAabbLowerBound result;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      result.valid = false;
      return result;
    }
    const std::uint64_t lower_bits =
        coordinate_bits[axis * point_count + lower_id];
    const std::uint64_t upper_bits =
        coordinate_bits[axis * point_count + upper_id];
    const std::uint64_t query_bits =
        coordinate_bits[axis * point_count + query_id];
    if (!finite_bits(lower_bits) || !finite_bits(upper_bits) ||
        !finite_bits(query_bits)) {
      result.valid = false;
      return result;
    }
    const double lower = value_from_bits(lower_bits);
    const double upper = value_from_bits(upper_bits);
    const double query = value_from_bits(query_bits);
    if (lower > upper) {
      result.valid = false;
      return result;
    }
    double delta = 0.0;
    if (query < lower) {
      delta = __dsub_rd(lower, query);
    } else if (query > upper) {
      delta = __dsub_rd(query, upper);
    }
    const double squared = __dmul_rd(delta, delta);
    const double next = __dadd_rd(result.value, squared);
    if (!finite_value(delta) || !finite_value(squared) ||
        !finite_value(next) || delta < 0.0 || squared < 0.0 ||
        next < 0.0) {
      result.valid = false;
      return result;
    }
    result.value = next;
  }
  return result;
}

__global__ void binary64_lbvh_top_k_kernel(
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Binary64LbvhDeviceNode* nodes,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t maximum_order,
    std::size_t seed_window_radius,
    std::uint64_t* output_ids,
    double* output_distances,
    Binary64LbvhDeviceQueryAudit* query_audits) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) *
      static_cast<std::size_t>(gridDim.x);
  std::size_t query_position =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  for (; query_position < point_count; query_position += stride) {
    Binary64LbvhDeviceQueryAudit audit;
    double best_distances[kMaximumOrder];
    std::uint64_t best_ids[kMaximumOrder];
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      best_distances[rank] = value_from_bits(kPositiveInfinityBits);
      best_ids[rank] = kInvalidIndex;
    }

    const std::uint64_t query_id = morton_point_ids[query_position];
    if (query_id >= point_count) {
      audit.failure_code = 1U;
    }
    const std::size_t seed_begin =
        query_position > seed_window_radius
            ? query_position - seed_window_radius
            : 0U;
    const std::size_t remaining = point_count - 1U - query_position;
    const std::size_t seed_end =
        query_position +
        (seed_window_radius < remaining ? seed_window_radius : remaining) +
        1U;
    if (audit.failure_code == 0U) {
      for (std::size_t candidate_position = seed_begin;
           candidate_position < seed_end;
           ++candidate_position) {
        if (candidate_position == query_position) {
          continue;
        }
        const std::uint64_t candidate_id =
            morton_point_ids[candidate_position];
        if (candidate_id >= point_count) {
          audit.failure_code = 2U;
          break;
        }
        const double distance = fixed_squared_distance(
            coordinate_bits, point_count, query_id, candidate_id);
        if (distance != distance) {
          audit.failure_code = 3U;
          break;
        }
        insert_candidate(
            distance,
            candidate_id,
            maximum_order,
            best_distances,
            best_ids);
      }
    }
    if (audit.failure_code == 0U &&
        best_ids[maximum_order - 1U] == kInvalidIndex) {
      audit.failure_code = 4U;
    }

    std::uint64_t cursor = static_cast<std::uint64_t>(node_count - 1U);
    bool complete = false;
    while (audit.failure_code == 0U && !complete) {
      if (cursor >= node_count ||
          audit.node_visit_count >= node_count) {
        audit.failure_code = 5U;
        break;
      }
      const Binary64LbvhDeviceNode& node = nodes[cursor];
      if (node.leaf_begin >= node.leaf_end ||
          node.leaf_end > point_count) {
        audit.failure_code = 6U;
        break;
      }
      const std::uint64_t leaf_count = node.leaf_end - node.leaf_begin;
      if (leaf_count > cursor / UINT64_C(2) + UINT64_C(1)) {
        audit.failure_code = 7U;
        break;
      }
      ++audit.node_visit_count;

      const bool seed_covered =
          node.leaf_begin >= seed_begin && node.leaf_end <= seed_end;
      DirectedAabbLowerBound lower;
      if (!seed_covered) {
        lower = directed_aabb_lower_bound(
            node, coordinate_bits, point_count, query_id);
      }
      const bool aabb_prune =
          !seed_covered && lower.valid &&
          lower.value > best_distances[maximum_order - 1U];
      const bool prune = seed_covered || aabb_prune;
      if (!seed_covered && !lower.valid) {
        if (audit.invalid_aabb_bound_descent_count == UINT32_MAX) {
          audit.failure_code = 8U;
          break;
        }
        ++audit.invalid_aabb_bound_descent_count;
      }

      if (!prune && leaf_count == UINT64_C(1)) {
        const std::size_t candidate_position =
            static_cast<std::size_t>(node.leaf_begin);
        if (candidate_position < seed_begin ||
            candidate_position >= seed_end) {
          const std::uint64_t candidate_id =
              morton_point_ids[candidate_position];
          if (candidate_id >= point_count) {
            audit.failure_code = 9U;
            break;
          }
          if (candidate_id != query_id) {
            const double distance = fixed_squared_distance(
                coordinate_bits, point_count, query_id, candidate_id);
            if (distance != distance) {
              audit.failure_code = 10U;
              break;
            }
            insert_candidate(
                distance,
                candidate_id,
                maximum_order,
                best_distances,
                best_ids);
            ++audit.leaf_distance_evaluation_count;
          }
        }
      }

      if (prune) {
        if (seed_covered) {
          ++audit.seed_covered_subtree_skip_count;
        } else {
          ++audit.strict_aabb_prune_count;
        }
        const std::uint64_t subtree_node_count =
            UINT64_C(2) * leaf_count - UINT64_C(1);
        if (cursor + UINT64_C(1) < subtree_node_count) {
          audit.failure_code = 11U;
          break;
        }
        complete = cursor + UINT64_C(1) == subtree_node_count;
        if (!complete) {
          cursor -= subtree_node_count;
        }
      } else {
        complete = cursor == 0U;
        if (!complete) {
          --cursor;
        }
      }
    }

    const std::size_t output_begin = query_position * maximum_order;
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      output_ids[output_begin + rank] = best_ids[rank];
      output_distances[output_begin + rank] = best_distances[rank];
    }
    query_audits[query_position] = audit;
  }
}

void checked_accumulate(
    std::size_t& total,
    std::uint64_t increment,
    const char* message) {
  if (increment >
          static_cast<std::uint64_t>(
              std::numeric_limits<std::size_t>::max()) ||
      total > std::numeric_limits<std::size_t>::max() -
                  static_cast<std::size_t>(increment)) {
    throw std::length_error(message);
  }
  total += static_cast<std::size_t>(increment);
}

}  // namespace

Binary64LbvhTopKResult run_phase14_binary64_lbvh_top_k_on_gpu(
    const std::uint64_t* device_coordinate_bits,
    const std::uint64_t* device_morton_point_ids,
    const void* device_nodes,
    std::size_t point_count,
    std::size_t certified_node_count,
    std::size_t persistent_input_device_byte_capacity,
    std::uint64_t source_snapshot_epoch,
    std::size_t maximum_order,
    std::size_t seed_window_radius,
    int cuda_device) {
  using Clock = std::chrono::steady_clock;
  const auto launcher_begin = Clock::now();
  if (device_coordinate_bits == nullptr ||
      device_morton_point_ids == nullptr || device_nodes == nullptr ||
      point_count <= maximum_order || maximum_order == 0U ||
      maximum_order > kMaximumOrder ||
      seed_window_radius < maximum_order ||
      seed_window_radius >= point_count ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U ||
      certified_node_count != point_count * 2U - 1U ||
      source_snapshot_epoch == 0U ||
      cuda_device < 0) {
    throw std::invalid_argument(
        "the binary64 LBVH top-k launcher received invalid extents");
  }
  const std::size_t neighbor_count = checked_product(
      point_count,
      maximum_order,
      "the binary64 LBVH top-k neighbor extent overflows size_t");
  const std::size_t neighbor_id_bytes = checked_product(
      neighbor_count,
      sizeof(std::uint64_t),
      "the binary64 LBVH top-k PointId bytes overflow size_t");
  const std::size_t source_point_id_bytes = checked_product(
      point_count,
      sizeof(std::uint64_t),
      "the binary64 LBVH source PointId bytes overflow size_t");
  const std::size_t neighbor_distance_bytes = checked_product(
      neighbor_count,
      sizeof(double),
      "the binary64 LBVH top-k distance bytes overflow size_t");
  const std::size_t query_audit_bytes = checked_product(
      point_count,
      sizeof(Binary64LbvhDeviceQueryAudit),
      "the binary64 LBVH top-k audit bytes overflow size_t");

  DeviceGuard guard{cuda_device};
  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(&properties, cuda_device),
      "cudaGetDeviceProperties binary64 LBVH top-k");
  if (properties.multiProcessorCount <= 0) {
    throw std::runtime_error("CUDA reported no multiprocessor");
  }

  Binary64LbvhTopKResult result;
  result.source_point_ids_by_morton_position.resize(point_count);
  result.neighbor_point_ids.resize(neighbor_count);
  result.squared_distances.resize(neighbor_count);
  std::vector<Binary64LbvhDeviceQueryAudit> query_audits(point_count);

  const auto allocation_begin = Clock::now();
  DeviceBuffer<std::uint64_t> output_ids(neighbor_count);
  DeviceBuffer<double> output_distances(neighbor_count);
  DeviceBuffer<Binary64LbvhDeviceQueryAudit> device_query_audits(point_count);
  const auto allocation_end = Clock::now();

  const std::size_t desired_block_count =
      (point_count + static_cast<std::size_t>(kThreadsPerBlock) - 1U) /
      static_cast<std::size_t>(kThreadsPerBlock);
  const std::size_t occupancy_block_count = checked_product(
      static_cast<std::size_t>(properties.multiProcessorCount),
      kBlocksPerMultiprocessor,
      "the binary64 LBVH top-k occupancy extent overflows size_t");
  const std::size_t block_count = std::max(
      std::size_t{1}, std::min(desired_block_count, occupancy_block_count));
  if (block_count >
      static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())) {
    throw std::length_error(
        "the binary64 LBVH top-k grid exceeds unsigned int");
  }

  CudaEvent kernel_begin;
  CudaEvent kernel_end;
  CudaEvent output_end;
  kernel_begin.record();
  binary64_lbvh_top_k_kernel<<<
      static_cast<unsigned int>(block_count), kThreadsPerBlock>>>(
      device_coordinate_bits,
      device_morton_point_ids,
      static_cast<const Binary64LbvhDeviceNode*>(device_nodes),
      point_count,
      certified_node_count,
      maximum_order,
      seed_window_radius,
      output_ids.get(),
      output_distances.get(),
      device_query_audits.get());
  check_cuda(
      cudaGetLastError(), "binary64_lbvh_top_k_kernel launch");
  kernel_end.record();
  check_cuda(
      cudaMemcpy(
          result.source_point_ids_by_morton_position.data(),
          device_morton_point_ids,
          source_point_id_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy binary64 LBVH source Morton PointIds D2H");
  check_cuda(
      cudaMemcpy(
          result.neighbor_point_ids.data(),
          output_ids.get(),
          neighbor_id_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy binary64 LBVH top-k PointIds D2H");
  check_cuda(
      cudaMemcpy(
          result.squared_distances.data(),
          output_distances.get(),
          neighbor_distance_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy binary64 LBVH top-k distances D2H");
  check_cuda(
      cudaMemcpy(
          query_audits.data(),
          device_query_audits.get(),
          query_audit_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy binary64 LBVH top-k audits D2H");
  output_end.record();
  output_end.synchronize();

  Binary64LbvhTopKAudit& audit = result.audit;
  audit.point_count = point_count;
  audit.certified_node_count = certified_node_count;
  audit.maximum_order = maximum_order;
  audit.seed_window_radius = seed_window_radius;
  audit.neighbor_record_count = neighbor_count;
  std::vector<std::size_t> node_visit_counts;
  node_visit_counts.reserve(point_count);
  for (std::size_t position = 0U; position < point_count; ++position) {
    const std::size_t left = std::min(position, seed_window_radius);
    const std::size_t right = std::min(
        point_count - 1U - position, seed_window_radius);
    checked_accumulate(
        audit.seed_distance_evaluation_count,
        static_cast<std::uint64_t>(left + right),
        "the binary64 LBVH seed count overflows size_t");
    const Binary64LbvhDeviceQueryAudit& query = query_audits[position];
    checked_accumulate(
        audit.node_visit_count,
        query.node_visit_count,
        "the binary64 LBVH node-visit count overflows size_t");
    checked_accumulate(
        audit.strict_aabb_prune_count,
        query.strict_aabb_prune_count,
        "the binary64 LBVH prune count overflows size_t");
    checked_accumulate(
        audit.seed_covered_subtree_skip_count,
        query.seed_covered_subtree_skip_count,
        "the binary64 LBVH seed-subtree skip count overflows size_t");
    checked_accumulate(
        audit.traversal_leaf_distance_evaluation_count,
        query.leaf_distance_evaluation_count,
        "the binary64 LBVH leaf-distance count overflows size_t");
    checked_accumulate(
        audit.invalid_aabb_bound_descent_count,
        query.invalid_aabb_bound_descent_count,
        "the binary64 LBVH invalid-bound count overflows size_t");
    audit.maximum_node_visit_count_per_query = std::max(
        audit.maximum_node_visit_count_per_query,
        static_cast<std::size_t>(query.node_visit_count));
    node_visit_counts.push_back(
        static_cast<std::size_t>(query.node_visit_count));
    if (query.node_visit_count == certified_node_count) {
      ++audit.full_tree_query_count;
    }
    if (query.failure_code == 0U) {
      ++audit.completed_query_count;
    } else {
      ++audit.failed_query_count;
    }
  }
  const auto percentile = [&node_visit_counts](std::size_t percent) {
    const std::size_t index =
        (percent * (node_visit_counts.size() - 1U) + 99U) / 100U;
    std::nth_element(
        node_visit_counts.begin(),
        node_visit_counts.begin() + static_cast<std::ptrdiff_t>(index),
        node_visit_counts.end());
    return node_visit_counts[index];
  };
  audit.median_node_visit_count_per_query = percentile(50U);
  audit.p95_node_visit_count_per_query = percentile(95U);
  audit.p99_node_visit_count_per_query = percentile(99U);
  audit.persistent_input_device_byte_capacity =
      persistent_input_device_byte_capacity;
  audit.transient_output_device_byte_capacity = checked_sum(
      checked_sum(
          neighbor_id_bytes,
          neighbor_distance_bytes,
          "the binary64 LBVH output bytes overflow size_t"),
      query_audit_bytes,
      "the binary64 LBVH transient bytes overflow size_t");
  audit.kernel_block_count = block_count;
  audit.kernel_thread_count = checked_product(
      block_count,
      static_cast<std::size_t>(kThreadsPerBlock),
      "the binary64 LBVH kernel thread count overflows size_t");
  audit.allocation_nanoseconds =
      duration_nanoseconds(allocation_end - allocation_begin);
  audit.kernel_nanoseconds = event_nanoseconds(kernel_begin, kernel_end);
  audit.device_to_host_nanoseconds =
      event_nanoseconds(kernel_end, output_end);
  audit.launcher_wall_nanoseconds =
      duration_nanoseconds(Clock::now() - launcher_begin);
  audit.source_snapshot_epoch = source_snapshot_epoch;
  audit.cuda_device = cuda_device;
  audit.multiprocessor_count = properties.multiProcessorCount;
  audit.device_name = properties.name;
  audit.traversal_lease_adopted = true;
  audit.fixed_round_to_nearest_distance_recipe_requested = true;
  audit.directed_round_down_aabb_recipe_requested = true;
  audit.strict_prune_requested = true;
  audit.stackless_postorder_traversal_requested = true;
  audit.complete_query_coverage =
      audit.completed_query_count == point_count &&
      audit.failed_query_count == 0U;
  audit.no_candidate_truncation = audit.complete_query_coverage;
  audit.higher_order_delaunay_mosaic_materialized = false;
  audit.global_pair_matrix_materialized = false;
  audit.exact_morse_hierarchy_claimed = false;
  audit.public_status_claimed = false;
  return result;
}

}  // namespace morsehgp3d::gpu::detail
