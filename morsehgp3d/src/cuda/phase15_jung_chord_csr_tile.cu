#include "phase15_jung_chord_csr_tile_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <cub/device/device_scan.cuh>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if !defined(__CUDACC__)
#error "phase15_jung_chord_csr_tile.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Jung-chord CSR prototype requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Jung-chord directed interval filter"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Jung-chord interval filter"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Jung-chord CSR prototype must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr std::size_t kAxisCount = 3U;
constexpr unsigned int kThreadsPerBlock = 128U;
constexpr std::uint32_t kFailureNone = 0U;
constexpr std::uint32_t kFailureMalformed = 1U;
constexpr std::uint32_t kFailureCountEmitMismatch = 2U;

struct Phase15JungChordNode final {
  std::uint64_t lower_point_ids[kAxisCount];
  std::uint64_t upper_point_ids[kAxisCount];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(sizeof(Phase15JungChordNode) == 80U);
static_assert(alignof(Phase15JungChordNode) == 8U);
static_assert(std::is_standard_layout_v<Phase15JungChordNode>);
static_assert(std::is_trivially_copyable_v<Phase15JungChordNode>);

class JungChordCudaError final : public std::runtime_error {
 public:
  JungChordCudaError(cudaError_t code, const std::string& operation)
      : std::runtime_error(
            operation + ": " + cudaGetErrorName(code) + " (" +
            cudaGetErrorString(code) + ")") {}
};

void check_cuda(cudaError_t code, const std::string& operation) {
  if (code != cudaSuccess) {
    throw JungChordCudaError(code, operation);
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

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void allocate(std::size_t count, const char* operation) {
    if (data_ != nullptr) {
      throw std::logic_error("a Jung-chord device buffer was allocated twice");
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
    check_cuda(cudaGetDevice(&previous_), "cudaGetDevice for Jung-chord CSR");
    if (previous_ != requested) {
      check_cuda(cudaSetDevice(requested), "cudaSetDevice for Jung-chord CSR");
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

class CudaEvent final {
 public:
  CudaEvent() {
    check_cuda(cudaEventCreate(&event_), "cudaEventCreate for Jung-chord CSR");
  }
  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;
  ~CudaEvent() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  void record(cudaStream_t stream) {
    check_cuda(cudaEventRecord(event_, stream), "cudaEventRecord Jung-chord CSR");
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

[[nodiscard]] std::uint64_t event_elapsed_ns(
    const CudaEvent& begin,
    const CudaEvent& end) {
  float milliseconds = 0.0F;
  check_cuda(
      cudaEventElapsedTime(&milliseconds, begin.get(), end.get()),
      "cudaEventElapsedTime Jung-chord CSR");
  if (!(milliseconds >= 0.0F)) {
    throw std::runtime_error("a Jung-chord CUDA event returned a NaN duration");
  }
  const double nanoseconds = static_cast<double>(milliseconds) * 1'000'000.0;
  if (nanoseconds >
      static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return static_cast<std::uint64_t>(nanoseconds);
}

class OutputOwner final {
 public:
  explicit OutputOwner(int device) : device_(device) {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Jung-chord CSR");
  }
  OutputOwner(const OutputOwner&) = delete;
  OutputOwner& operator=(const OutputOwner&) = delete;
  ~OutputOwner() {
    if (device_ >= 0) {
      int previous = 0;
      if (cudaGetDevice(&previous) == cudaSuccess) {
        static_cast<void>(cudaSetDevice(device_));
        anchors.reset();
        offsets.reset();
        chord_ids.reset();
        chord_flags.reset();
        constant_counts.reset();
        flags.reset();
        if (stream_ != nullptr) {
          static_cast<void>(cudaStreamDestroy(stream_));
        }
        static_cast<void>(cudaSetDevice(previous));
      }
    }
  }

  DeviceBuffer<JungChordAnchor> anchors;
  DeviceBuffer<std::uint64_t> offsets;
  DeviceBuffer<spatial::PointId> chord_ids;
  DeviceBuffer<std::uint8_t> chord_flags;
  DeviceBuffer<std::uint64_t> constant_counts;
  DeviceBuffer<std::uint8_t> flags;
  cudaStream_t stream() const noexcept { return stream_; }

 private:
  int device_{-1};
  cudaStream_t stream_{};
};

void require_device_pointer(
    const void* pointer,
    int expected_device,
    std::size_t alignment,
    const char* label) {
  if (pointer == nullptr ||
      reinterpret_cast<std::uintptr_t>(pointer) % alignment != 0U) {
    throw std::invalid_argument(
        std::string{"the Jung-chord source "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{"cudaPointerGetAttributes for Jung-chord "} + label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the Jung-chord source "} + label +
        " is not on the authenticated CUDA device");
  }
}

[[nodiscard]] __device__ bool valid_node(
    const Phase15JungChordNode* nodes,
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
  // The immutable topology was certified before the traversal lease was
  // released.  Per-anchor traversal retains only bounds/progress guards; it
  // does not re-read both children to re-prove the tree at every visit.
  return node_index != 0U && node.right_child < node_index &&
      node.right_child < node_count;
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

struct JungIntervals final {
  device::DeviceInterval b;
  device::DeviceInterval q;
  bool valid{};
};

struct AnchorGeometry final {
  device::DeviceInterval p[kAxisCount];
  device::DeviceInterval q_point[kAxisCount];
  device::DeviceInterval d[kAxisCount];
  device::DeviceInterval d2;
};

struct RadialIntervals final {
  device::DeviceInterval a[kAxisCount];
  device::DeviceInterval a2;
  bool valid{};
};

[[nodiscard]] __device__ device::DeviceInterval scalar(double value) noexcept {
  return device::DeviceInterval{value, value, true};
}

[[nodiscard]] __device__ RadialIntervals evaluate_radial_intervals(
    const device::DeviceInterval x[kAxisCount],
    const AnchorGeometry& geometry) noexcept {
  RadialIntervals radial{};
  device::DeviceInterval a2 = scalar(0.0);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const auto twice_x = device::add_intervals(x[axis], x[axis]);
    const auto p_plus_q = device::add_intervals(
        geometry.p[axis], geometry.q_point[axis]);
    radial.a[axis] = device::subtract_intervals(twice_x, p_plus_q);
    a2 = device::add_intervals(
        a2, device::square_interval(radial.a[axis]));
  }
  radial.a2 = a2;
  radial.valid = a2.valid;
  return radial;
}

[[nodiscard]] __device__ JungIntervals evaluate_jung_intervals(
    const AnchorGeometry& geometry,
    const RadialIntervals& radial,
    std::uint64_t support_size) noexcept {
  const auto b = device::subtract_intervals(radial.a2, geometry.d2);
  const auto c01 = device::subtract_intervals(
      device::multiply_intervals(geometry.d[0], radial.a[1]),
      device::multiply_intervals(geometry.d[1], radial.a[0]));
  const auto c02 = device::subtract_intervals(
      device::multiply_intervals(geometry.d[0], radial.a[2]),
      device::multiply_intervals(geometry.d[2], radial.a[0]));
  const auto c12 = device::subtract_intervals(
      device::multiply_intervals(geometry.d[1], radial.a[2]),
      device::multiply_intervals(geometry.d[2], radial.a[1]));
  auto gram = device::add_intervals(
      device::square_interval(c01), device::square_interval(c02));
  gram = device::add_intervals(gram, device::square_interval(c12));
  const auto gram_factor = device::multiply_intervals(
      support_size == 3U ? scalar(4.0) : scalar(2.0), gram);
  const auto b_square_factor = device::multiply_intervals(
      support_size == 3U ? scalar(3.0) : scalar(1.0),
      device::square_interval(b));
  const auto jung_q =
      device::subtract_intervals(gram_factor, b_square_factor);
  return JungIntervals{
      b,
      jung_q,
      geometry.d2.valid && radial.valid && b.valid && jung_q.valid};
}

[[nodiscard]] __device__ bool outside_certified_jung_range(
    const RadialIntervals& radial,
    const device::DeviceInterval& d2,
    std::uint64_t support_size) noexcept {
  if (!radial.valid || !d2.valid) {
    return false;
  }
  if (support_size == 3U) {
    const auto bound = device::multiply_intervals(scalar(3.0), d2);
    return bound.valid && radial.a2.lower > bound.upper;
  }
  const auto left = device::multiply_intervals(scalar(4.0), radial.a2);
  const auto right = device::multiply_intervals(scalar(15.0), d2);
  return left.valid && right.valid && left.lower > right.upper;
}

enum class Terminal : std::uint8_t {
  descend,
  range_exterior,
  constant_interior,
  constant_exterior,
  active_certified,
  active_ambiguous,
};

enum class LensEligibility : std::uint8_t {
  ineligible_certified,
  eligible_certified,
  eligible_ambiguous,
};

[[nodiscard]] __device__ LensEligibility diameter_lens_eligibility(
    const device::DeviceInterval x[kAxisCount],
    const device::DeviceInterval p[kAxisCount],
    const device::DeviceInterval q_point[kAxisCount],
    const device::DeviceInterval& d2) noexcept {
  device::DeviceInterval xp2 = scalar(0.0);
  device::DeviceInterval xq2 = scalar(0.0);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    xp2 = device::add_intervals(
        xp2,
        device::square_interval(device::subtract_intervals(x[axis], p[axis])));
    xq2 = device::add_intervals(
        xq2,
        device::square_interval(
            device::subtract_intervals(x[axis], q_point[axis])));
  }
  if (!xp2.valid || !xq2.valid || !d2.valid) {
    return LensEligibility::eligible_ambiguous;
  }
  if (xp2.lower > d2.upper || xq2.lower > d2.upper) {
    return LensEligibility::ineligible_certified;
  }
  if (xp2.upper <= d2.lower && xq2.upper <= d2.lower) {
    return LensEligibility::eligible_certified;
  }
  return LensEligibility::eligible_ambiguous;
}

[[nodiscard]] __device__ Terminal classify_intervals(
    const JungIntervals& value,
    bool leaf) noexcept {
  if (!value.valid) {
    return leaf ? Terminal::active_ambiguous : Terminal::descend;
  }
  if (value.q.upper < 0.0 && value.b.upper < 0.0) {
    return Terminal::constant_interior;
  }
  if (value.q.upper < 0.0 && value.b.lower > 0.0) {
    return Terminal::constant_exterior;
  }
  if (!leaf) {
    return Terminal::descend;
  }
  return value.q.lower >= 0.0 ? Terminal::active_certified
                              : Terminal::active_ambiguous;
}

__device__ void signal_failure(
    Phase15JungChordDeviceControl* global,
    std::uint32_t failure) noexcept {
  atomicCAS(&global->failure_code, kFailureNone, failure);
}

template <bool Emit>
__device__ bool traverse_anchor(
    const JungChordAnchor& anchor,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase15JungChordNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t support_size,
    std::uint64_t maximum_constant_interior,
    std::uint64_t output_begin,
    spatial::PointId* output,
    std::uint8_t* output_flags,
    Phase15JungChordAnchorControl& control) noexcept {
  AnchorGeometry geometry{};
  geometry.d2 = scalar(0.0);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    geometry.p[axis] = device::point_interval(
        coordinate_bits[axis * point_count + anchor.support_u]);
    geometry.q_point[axis] = device::point_interval(
        coordinate_bits[axis * point_count + anchor.support_v]);
    geometry.d[axis] = device::subtract_intervals(
        geometry.q_point[axis], geometry.p[axis]);
    geometry.d2 = device::add_intervals(
        geometry.d2, device::square_interval(geometry.d[axis]));
  }
  std::uint64_t cursor = node_count - 1U;
  std::uint64_t emitted = 0U;
  std::uint64_t constant_interior = 0U;
  while (true) {
    if (!valid_node(nodes, cursor, node_count, point_count)) {
      return false;
    }
    const auto& node = nodes[cursor];
    const std::uint64_t width = node.leaf_end - node.leaf_begin;
    if constexpr (Emit) {
      ++control.emit_node_visits;
    } else {
      ++control.count_node_visits;
    }
    const bool leaf = width == 1U;
    device::DeviceInterval x[kAxisCount];
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      const std::uint64_t lower_id = node.lower_point_ids[axis];
      const std::uint64_t upper_id = node.upper_point_ids[axis];
      if (lower_id >= point_count || upper_id >= point_count) {
        return false;
      }
      const auto lower = device::point_interval(
          coordinate_bits[axis * point_count + lower_id]);
      const auto upper = device::point_interval(
          coordinate_bits[axis * point_count + upper_id]);
      x[axis] = device::checked_interval(lower.lower, upper.upper);
    }
    if (leaf) {
      if constexpr (Emit) {
        ++control.emit_leaf_tests;
      } else {
        ++control.count_leaf_tests;
      }
      const std::uint64_t point_id = morton_point_ids[node.leaf_begin];
      if (point_id >= point_count) {
        return false;
      }
      if (point_id == anchor.support_u || point_id == anchor.support_v) {
        bool complete = false;
        if (!finish_or_skip_subtree(width, cursor, complete)) {
          return false;
        }
        if (complete) {
          break;
        }
        continue;
      }
    }

    const RadialIntervals radial = evaluate_radial_intervals(x, geometry);
    const bool outside = outside_certified_jung_range(
        radial, geometry.d2, support_size);
    const JungIntervals value = outside
        ? JungIntervals{}
        : evaluate_jung_intervals(geometry, radial, support_size);
    const Terminal terminal =
        outside ? Terminal::range_exterior : classify_intervals(value, leaf);
    if (terminal == Terminal::descend) {
      cursor = node.right_child;
      continue;
    }

    if (!Emit) {
      if (terminal == Terminal::range_exterior) {
        control.range_pruned_points += width;
      } else {
        control.range_candidate_count += width;
      }
      if (terminal == Terminal::constant_interior) {
        control.bulk_constant_interior_points += width;
      } else if (terminal == Terminal::constant_exterior) {
        control.bulk_constant_exterior_points += width;
      }
    }
    if (terminal == Terminal::constant_interior) {
      constant_interior += width;
      if (constant_interior > maximum_constant_interior) {
        if constexpr (!Emit) {
          control.constant_interior_lower_bound =
              maximum_constant_interior + 1U;
          control.chord_count = 0U;
          control.flags = phase15_jung_chord_anchor_depth_rejected |
              phase15_jung_chord_anchor_complete;
        }
        return !Emit;
      }
    } else if (terminal == Terminal::active_certified ||
               terminal == Terminal::active_ambiguous) {
      if (!leaf) {
        return false;
      }
      if constexpr (Emit) {
        output[output_begin + emitted] =
            morton_point_ids[node.leaf_begin];
      } else {
        if (terminal == Terminal::active_certified) {
          ++control.certified_active_chords;
        } else {
          ++control.ambiguous_active_chords;
        }
      }
      const LensEligibility lens = diameter_lens_eligibility(
          x, geometry.p, geometry.q_point, geometry.d2);
      if (lens != LensEligibility::ineligible_certified) {
        if constexpr (Emit) {
          output_flags[output_begin + emitted] =
              phase15_jung_chord_support_eligible |
              (lens == LensEligibility::eligible_ambiguous
                   ? phase15_jung_chord_support_eligible_ambiguous
                   : 0U);
        } else {
          ++control.support_eligible_chords;
          control.support_eligible_ambiguous_chords +=
              lens == LensEligibility::eligible_ambiguous;
        }
      } else if constexpr (Emit) {
        output_flags[output_begin + emitted] = 0U;
      }
      ++emitted;
    }

    bool complete = false;
    if (!finish_or_skip_subtree(width, cursor, complete)) {
      return false;
    }
    if (complete) {
      break;
    }
  }
  if constexpr (Emit) {
    return emitted == control.chord_count &&
        constant_interior == control.constant_interior_lower_bound;
  }
  control.chord_count = emitted;
  control.constant_interior_lower_bound = constant_interior;
  control.flags = phase15_jung_chord_anchor_complete;
  return true;
}

__global__ void count_jung_chords_kernel(
    const JungChordAnchor* anchors,
    std::uint64_t anchor_count,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase15JungChordNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t support_size,
    std::uint64_t maximum_constant_interior,
    Phase15JungChordAnchorControl* controls,
    std::uint64_t* counts,
    std::uint64_t* constant_counts,
    std::uint8_t* flags,
    Phase15JungChordDeviceControl* global) {
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= anchor_count) {
    return;
  }
  Phase15JungChordAnchorControl control{};
  const JungChordAnchor anchor = anchors[index];
  if (anchor.support_u >= anchor.support_v || anchor.support_v >= point_count ||
      !traverse_anchor<false>(
          anchor,
          coordinate_bits,
          morton_point_ids,
          nodes,
          point_count,
          node_count,
          support_size,
          maximum_constant_interior,
          0U,
          nullptr,
          nullptr,
          control)) {
    signal_failure(global, kFailureMalformed);
    control.flags = 0U;
    control.chord_count = 0U;
  }
  controls[index] = control;
  counts[index] = control.chord_count;
  constant_counts[index] = control.constant_interior_lower_bound;
  flags[index] = control.flags;
}

__global__ void preflight_jung_chords_kernel(
    const std::uint64_t* offsets,
    std::uint64_t anchor_count,
    std::uint64_t chord_capacity,
    Phase15JungChordDeviceControl* global) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  global->required_chord_count = offsets[anchor_count];
  global->commit_allowed =
      global->failure_code == kFailureNone &&
      global->required_chord_count <= chord_capacity;
}

__global__ void emit_jung_chords_kernel(
    const JungChordAnchor* anchors,
    std::uint64_t anchor_count,
    const std::uint64_t* offsets,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase15JungChordNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t support_size,
    std::uint64_t maximum_constant_interior,
    spatial::PointId* chord_ids,
    std::uint8_t* chord_flags,
    Phase15JungChordAnchorControl* controls,
    Phase15JungChordDeviceControl* global) {
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= anchor_count || global->commit_allowed == 0U) {
    return;
  }
  auto control = controls[index];
  if ((control.flags & phase15_jung_chord_anchor_depth_rejected) == 0U &&
      !traverse_anchor<true>(
          anchors[index],
          coordinate_bits,
          morton_point_ids,
          nodes,
          point_count,
          node_count,
          support_size,
          maximum_constant_interior,
          offsets[index],
          chord_ids,
          chord_flags,
          control)) {
    signal_failure(global, kFailureCountEmitMismatch);
  }
  controls[index] = control;
}

__global__ void finalize_jung_chords_kernel(
    Phase15JungChordDeviceControl* global) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  if (global->commit_allowed != 0U && global->failure_code == kFailureNone) {
    global->emitted_chord_count = global->required_chord_count;
    global->output_valid = 1U;
  }
}

[[nodiscard]] std::size_t output_arena_bytes(
    std::size_t anchor_count,
    std::size_t chord_count) {
  std::size_t total = checked_bytes(
      anchor_count,
      sizeof(JungChordAnchor),
      "Jung anchor capacity bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          anchor_count + 1U,
          sizeof(std::uint64_t),
          "Jung offset capacity bytes overflow"),
      "Jung installed arena bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          chord_count,
          sizeof(spatial::PointId) + sizeof(std::uint8_t),
          "Jung chord capacity bytes overflow"),
      "Jung installed arena bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          anchor_count,
          sizeof(std::uint64_t) + sizeof(std::uint8_t),
          "Jung output sidecar bytes overflow"),
      "Jung installed arena bytes overflow");
  return total;
}

[[nodiscard]] std::size_t working_arena_bytes(
    std::size_t anchor_count,
    std::size_t scan_bytes) {
  std::size_t total = checked_bytes(
      anchor_count,
      sizeof(Phase15JungChordAnchorControl),
      "Jung controls bytes overflow");
  total = checked_add(
      total,
      checked_bytes(
          anchor_count + 1U,
          sizeof(std::uint64_t),
          "Jung counts bytes overflow"),
      "Jung working bytes overflow");
  total = checked_add(
      total,
      sizeof(Phase15JungChordDeviceControl),
      "Jung working bytes overflow");
  return checked_add(total, scan_bytes, "Jung working bytes overflow");
}

}  // namespace

Phase15JungChordCsrTileReceipt launch_phase15_jung_chord_csr_tile(
    const Phase15JungChordCsrTileRequest& request) {
  if (request.host_fake) {
    throw std::logic_error(
        "the native Jung-chord launcher received a host-fake request");
  }
  if (request.anchors.empty() ||
      request.anchors.size() > request.config.anchor_capacity ||
      request.point_count < 2U ||
      request.certified_node_count != 2U * request.point_count - 1U ||
      request.cuda_device < 0) {
    throw std::invalid_argument("malformed native Jung-chord CSR request");
  }
  DeviceGuard guard{request.cuda_device};
  require_device_pointer(
      request.device_coordinate_bits,
      request.cuda_device,
      alignof(std::uint64_t),
      "coordinate");
  require_device_pointer(
      request.device_morton_point_ids,
      request.cuda_device,
      alignof(std::uint64_t),
      "Morton PointId");
  require_device_pointer(
      request.device_nodes,
      request.cuda_device,
      alignof(Phase15JungChordNode),
      "node");

  auto output = std::make_shared<OutputOwner>(request.cuda_device);
  const std::size_t anchor_count = request.anchors.size();
  output->anchors.allocate(anchor_count, "cudaMalloc Jung-chord anchors");
  output->offsets.allocate(anchor_count + 1U, "cudaMalloc Jung-chord offsets");
  output->constant_counts.allocate(
      anchor_count, "cudaMalloc Jung-chord constant counts");
  output->flags.allocate(anchor_count, "cudaMalloc Jung-chord flags");
  output->chord_ids.allocate(
      request.config.chord_point_id_capacity,
      "cudaMalloc Jung-chord resident ID capacity");
  output->chord_flags.allocate(
      request.config.chord_point_id_capacity,
      "cudaMalloc Jung-chord resident flag capacity");
  DeviceBuffer<Phase15JungChordAnchorControl> controls;
  DeviceBuffer<std::uint64_t> counts;
  DeviceBuffer<Phase15JungChordDeviceControl> global;
  controls.allocate(anchor_count, "cudaMalloc Jung-chord controls");
  counts.allocate(anchor_count + 1U, "cudaMalloc Jung-chord counts");
  global.allocate(1U, "cudaMalloc Jung-chord global control");
  CudaEvent total_begin_event;
  CudaEvent total_end_event;
  total_begin_event.record(output->stream());
  check_cuda(
      cudaMemsetAsync(
          controls.get(),
          0,
          anchor_count * sizeof(Phase15JungChordAnchorControl),
          output->stream()),
      "cudaMemsetAsync Jung-chord controls");
  check_cuda(
      cudaMemsetAsync(
          counts.get(), 0, (anchor_count + 1U) * sizeof(std::uint64_t),
          output->stream()),
      "cudaMemsetAsync Jung-chord counts");
  check_cuda(
      cudaMemsetAsync(
          global.get(), 0, sizeof(Phase15JungChordDeviceControl),
          output->stream()),
      "cudaMemsetAsync Jung-chord global control");

  CudaEvent upload_begin_event;
  CudaEvent upload_end_event;
  CudaEvent count_end_event;
  CudaEvent scan_end_event;
  CudaEvent preflight_end_event;
  CudaEvent emit_begin_event;
  CudaEvent emit_end_event;
  upload_begin_event.record(output->stream());
  check_cuda(
      cudaMemcpyAsync(
          output->anchors.get(),
          request.anchors.data(),
          anchor_count * sizeof(JungChordAnchor),
          cudaMemcpyHostToDevice,
          output->stream()),
      "cudaMemcpyAsync Jung-chord anchors H2D");
  upload_end_event.record(output->stream());
  const dim3 block{kThreadsPerBlock, 1U, 1U};
  const auto block_count = static_cast<unsigned int>(
      (anchor_count + kThreadsPerBlock - 1U) / kThreadsPerBlock);
  const dim3 grid{block_count, 1U, 1U};
  count_jung_chords_kernel<<<grid, block, 0U, output->stream()>>>(
      output->anchors.get(),
      static_cast<std::uint64_t>(anchor_count),
      request.device_coordinate_bits,
      request.device_morton_point_ids,
      static_cast<const Phase15JungChordNode*>(request.device_nodes),
      static_cast<std::uint64_t>(request.point_count),
      static_cast<std::uint64_t>(request.certified_node_count),
      static_cast<std::uint64_t>(request.config.support_size),
      static_cast<std::uint64_t>(
          request.config.maximum_relevant_closed_rank -
          request.config.support_size),
      controls.get(),
      counts.get(),
      output->constant_counts.get(),
      output->flags.get(),
      global.get());
  check_cuda(cudaGetLastError(), "launch Jung-chord count kernel");
  count_end_event.record(output->stream());

  std::size_t scan_bytes = 0U;
  check_cuda(
      cub::DeviceScan::ExclusiveSum(
          nullptr,
          scan_bytes,
          counts.get(),
          output->offsets.get(),
          static_cast<int>(anchor_count + 1U),
          output->stream()),
      "CUB size query for Jung-chord scan");
  std::size_t configured_scan_bytes = 0U;
  check_cuda(
      cub::DeviceScan::ExclusiveSum(
          nullptr,
          configured_scan_bytes,
          static_cast<const std::uint64_t*>(nullptr),
          static_cast<std::uint64_t*>(nullptr),
          static_cast<int>(request.config.anchor_capacity + 1U),
          output->stream()),
      "CUB configured-capacity size query for Jung-chord scan");
  DeviceBuffer<std::byte> scan_temp;
  scan_temp.allocate(scan_bytes, "cudaMalloc Jung-chord CUB scan temporary");
  check_cuda(
      cub::DeviceScan::ExclusiveSum(
          scan_temp.get(),
          scan_bytes,
          counts.get(),
          output->offsets.get(),
          static_cast<int>(anchor_count + 1U),
          output->stream()),
      "CUB Jung-chord exclusive scan");
  scan_end_event.record(output->stream());

  preflight_jung_chords_kernel<<<1U, 1U, 0U, output->stream()>>>(
      output->offsets.get(),
      static_cast<std::uint64_t>(anchor_count),
      static_cast<std::uint64_t>(request.config.chord_point_id_capacity),
      global.get());
  check_cuda(cudaGetLastError(), "launch Jung-chord capacity preflight");
  preflight_end_event.record(output->stream());
  Phase15JungChordDeviceControl host_global{};
  std::vector<Phase15JungChordAnchorControl> host_controls(anchor_count);
  emit_begin_event.record(output->stream());
  emit_jung_chords_kernel<<<grid, block, 0U, output->stream()>>>(
      output->anchors.get(),
      static_cast<std::uint64_t>(anchor_count),
      output->offsets.get(),
      request.device_coordinate_bits,
      request.device_morton_point_ids,
      static_cast<const Phase15JungChordNode*>(request.device_nodes),
      static_cast<std::uint64_t>(request.point_count),
      static_cast<std::uint64_t>(request.certified_node_count),
      static_cast<std::uint64_t>(request.config.support_size),
      static_cast<std::uint64_t>(
          request.config.maximum_relevant_closed_rank -
          request.config.support_size),
      output->chord_ids.get(),
      output->chord_flags.get(),
      controls.get(),
      global.get());
  check_cuda(cudaGetLastError(), "launch Jung-chord emit kernel");
  emit_end_event.record(output->stream());
  finalize_jung_chords_kernel<<<1U, 1U, 0U, output->stream()>>>(global.get());
  check_cuda(cudaGetLastError(), "launch Jung-chord finalize kernel");
  total_end_event.record(output->stream());
  check_cuda(
      cudaMemcpyAsync(
          &host_global,
          global.get(),
          sizeof(host_global),
          cudaMemcpyDeviceToHost,
          output->stream()),
      "cudaMemcpyAsync Jung-chord final control D2H");
  if (request.config.collect_per_anchor_diagnostics) {
    check_cuda(
        cudaMemcpyAsync(
            host_controls.data(),
            controls.get(),
            anchor_count * sizeof(Phase15JungChordAnchorControl),
            cudaMemcpyDeviceToHost,
            output->stream()),
        "cudaMemcpyAsync Jung-chord diagnostic controls D2H");
  }
  check_cuda(
      cudaStreamSynchronize(output->stream()),
      "cudaStreamSynchronize Jung-chord emit");
  Phase15JungChordCsrTileReceipt receipt;
  receipt.required_chord_count =
      static_cast<std::size_t>(host_global.required_chord_count);
  if (request.config.collect_per_anchor_diagnostics) {
    receipt.anchor_controls = std::move(host_controls);
  }
  receipt.cub_scan_temporary_bytes = scan_bytes;
  receipt.anchor_h2d_bytes = anchor_count * sizeof(JungChordAnchor);
  receipt.control_d2h_bytes = sizeof(host_global) +
      (request.config.collect_per_anchor_diagnostics
           ? anchor_count * sizeof(Phase15JungChordAnchorControl)
           : 0U);
  receipt.chord_d2h_bytes = 0U;
  receipt.kernel_launches = 4U;
  receipt.library_submissions = 1U;
  receipt.synchronizations = 1U;
  receipt.anchor_upload_ns =
      event_elapsed_ns(upload_begin_event, upload_end_event);
  receipt.count_kernel_ns =
      event_elapsed_ns(upload_end_event, count_end_event);
  receipt.scan_ns = event_elapsed_ns(count_end_event, scan_end_event);
  receipt.capacity_preflight_ns =
      event_elapsed_ns(scan_end_event, preflight_end_event);
  receipt.emit_kernel_ns = event_elapsed_ns(emit_begin_event, emit_end_event);
  receipt.total_device_ns =
      event_elapsed_ns(total_begin_event, total_end_event);
  receipt.host_fake = false;
  receipt.cuda_executed = true;
  const std::size_t working_bytes = working_arena_bytes(anchor_count, scan_bytes);
  receipt.logical_output_device_bytes =
      output_arena_bytes(anchor_count, receipt.required_chord_count);
  receipt.required_device_arena_bytes = checked_add(
      receipt.logical_output_device_bytes,
      working_bytes,
      "Jung required device bytes overflow");
  receipt.physical_device_arena_bytes = checked_add(
      output_arena_bytes(
          anchor_count, request.config.chord_point_id_capacity),
      working_bytes,
      "Jung allocated device bytes overflow");
  receipt.configured_capacity_upper_bound_bytes = checked_add(
      output_arena_bytes(
          request.config.anchor_capacity,
          request.config.chord_point_id_capacity),
      working_arena_bytes(
          request.config.anchor_capacity, configured_scan_bytes),
      "Jung configured upper-bound bytes overflow");
  if (host_global.failure_code != kFailureNone) {
    receipt.status = JungChordCsrTileStatus::execution_failure;
    receipt.stop_reason = host_global.failure_code == kFailureCountEmitMismatch
        ? JungChordCsrTileStopReason::count_emit_mismatch
        : JungChordCsrTileStopReason::traversal_failure;
    return receipt;
  }
  if (host_global.commit_allowed == 0U) {
    receipt.status = JungChordCsrTileStatus::capacity_exhausted;
    receipt.stop_reason = JungChordCsrTileStopReason::chord_point_id_capacity;
    return receipt;
  }
  if (host_global.output_valid == 0U ||
      host_global.emitted_chord_count != host_global.required_chord_count) {
    receipt.status = JungChordCsrTileStatus::execution_failure;
    receipt.stop_reason = JungChordCsrTileStopReason::count_emit_mismatch;
    return receipt;
  }

  receipt.status = JungChordCsrTileStatus::tile_complete;
  receipt.stop_reason = JungChordCsrTileStopReason::none;
  receipt.committed_chord_count = receipt.required_chord_count;
  receipt.output_valid = true;
  receipt.retained_output_device_bytes =
      output_arena_bytes(
          anchor_count, request.config.chord_point_id_capacity);
  receipt.device_anchors = output->anchors.get();
  receipt.device_offsets = output->offsets.get();
  receipt.device_chord_point_ids = output->chord_ids.get();
  receipt.device_chord_flags = output->chord_flags.get();
  receipt.device_constant_interior_counts = output->constant_counts.get();
  receipt.device_anchor_flags = output->flags.get();
  receipt.output_owner = output;
  return receipt;
}

Phase15JungChordCsrQualificationSnapshot
copy_phase15_jung_chord_csr_qualification_snapshot(
    const JungChordCsrTileLease& lease) {
  const auto view =
      Phase15JungChordCsrTilePrivateViewAccess::resident_view(lease);
  DeviceGuard guard{view.cuda_device};
  Phase15JungChordCsrQualificationSnapshot snapshot;
  snapshot.anchors.resize(view.anchor_count);
  snapshot.offsets.resize(view.anchor_count + 1U);
  snapshot.chord_point_ids.resize(view.chord_count);
  snapshot.chord_flags.resize(view.chord_count);
  snapshot.constant_interior_counts.resize(view.anchor_count);
  snapshot.anchor_flags.resize(view.anchor_count);
  check_cuda(
      cudaMemcpy(
          snapshot.anchors.data(),
          view.anchors,
          view.anchor_count * sizeof(JungChordAnchor),
          cudaMemcpyDeviceToHost),
      "qualification copy Jung-chord anchors");
  check_cuda(
      cudaMemcpy(
          snapshot.offsets.data(),
          view.offsets,
          (view.anchor_count + 1U) * sizeof(std::uint64_t),
          cudaMemcpyDeviceToHost),
      "qualification copy Jung-chord offsets");
  if (view.chord_count != 0U) {
    check_cuda(
        cudaMemcpy(
            snapshot.chord_point_ids.data(),
            view.chord_point_ids,
            view.chord_count * sizeof(spatial::PointId),
            cudaMemcpyDeviceToHost),
        "qualification copy Jung-chord IDs");
    check_cuda(
        cudaMemcpy(
            snapshot.chord_flags.data(),
            view.chord_flags,
            view.chord_count * sizeof(std::uint8_t),
            cudaMemcpyDeviceToHost),
        "qualification copy Jung-chord flags");
  }
  check_cuda(
      cudaMemcpy(
          snapshot.constant_interior_counts.data(),
          view.constant_interior_counts,
          view.anchor_count * sizeof(std::uint64_t),
          cudaMemcpyDeviceToHost),
      "qualification copy Jung-chord constant counts");
  check_cuda(
      cudaMemcpy(
          snapshot.anchor_flags.data(),
          view.anchor_flags,
          view.anchor_count * sizeof(std::uint8_t),
          cudaMemcpyDeviceToHost),
      "qualification copy Jung-chord flags");
  return snapshot;
}

}  // namespace morsehgp3d::gpu::detail
