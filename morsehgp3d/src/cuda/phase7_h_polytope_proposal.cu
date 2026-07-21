#include "phase7_h_polytope_proposal_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase7_h_polytope_proposal.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 7 H-polytope proposal requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 7 H-polytope proposal"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 7 H-polytope proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 7 H-polytope proposal must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kMaximumBoundaryCount = 61U;
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);

class HPolytopeProposalCudaFailure final : public std::runtime_error {
 public:
  HPolytopeProposalCudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code, const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw HPolytopeProposalCudaFailure(code, std::move(operation));
  }
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void ensure_capacity(std::size_t count, const char* operation) {
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 7 H-polytope device allocation size is invalid");
    }
    if (count <= count_) {
      return;
    }
    Value* replacement = nullptr;
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&replacement), count * sizeof(Value)),
        operation);
    if (data_ != nullptr) {
      Value* previous = data_;
      data_ = nullptr;
      count_ = 0U;
      const cudaError_t release_status = cudaFree(previous);
      if (release_status != cudaSuccess) {
        static_cast<void>(cudaFree(replacement));
        throw HPolytopeProposalCudaFailure(
            release_status,
            "cudaFree while growing a Phase 7 H-polytope device buffer");
      }
    }
    data_ = replacement;
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

 private:
  Value* data_{nullptr};
  std::size_t count_{0U};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 7 H-polytope proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 7 H-polytope context");
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
          "cudaSetDevice restore after Phase 7 H-polytope proposal");
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

  int previous_device_{0};
  bool restore_required_{false};
};

class HPolytopeProposalCudaResources final {
 public:
  HPolytopeProposalCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 7 H-polytope context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 7 H-polytope context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional H-polytope grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 7 H-polytope context");
  }

  HPolytopeProposalCudaResources(
      const HPolytopeProposalCudaResources&) = delete;
  HPolytopeProposalCudaResources& operator=(
      const HPolytopeProposalCudaResources&) = delete;

  ~HPolytopeProposalCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      cells_.abandon();
      boundaries_.abandon();
      cell_record_offsets_.abandon();
      records_.abandon();
      failure_code_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    cells_.reset();
    boundaries_.reset();
    cell_record_offsets_.reset();
    records_.reset();
    failure_code_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void ensure_capacity(
      std::size_t cell_count,
      std::size_t boundary_count,
      std::size_t record_capacity) {
    cells_.ensure_capacity(
        cell_count, "cudaMalloc Phase 7 H-polytope cells");
    boundaries_.ensure_capacity(
        boundary_count, "cudaMalloc Phase 7 H-polytope boundaries");
    cell_record_offsets_.ensure_capacity(
        cell_count + 1U,
        "cudaMalloc Phase 7 H-polytope cell record offsets");
    records_.ensure_capacity(
        record_capacity, "cudaMalloc Phase 7 H-polytope records");
    failure_code_.ensure_capacity(
        1U, "cudaMalloc Phase 7 H-polytope failure code");
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] HPolytopeProposalInputCell* cells() noexcept {
    return cells_.get();
  }
  [[nodiscard]] HPolytopeProposalInputBoundary* boundaries() noexcept {
    return boundaries_.get();
  }
  [[nodiscard]] std::uint64_t* cell_record_offsets() noexcept {
    return cell_record_offsets_.get();
  }
  [[nodiscard]] HPolytopeProposalDeviceRecord* records() noexcept {
    return records_.get();
  }
  [[nodiscard]] std::uint64_t* failure_code() noexcept {
    return failure_code_.get();
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 7 H-polytope context");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{0U};
  DeviceBuffer<HPolytopeProposalInputCell> cells_;
  DeviceBuffer<HPolytopeProposalInputBoundary> boundaries_;
  DeviceBuffer<std::uint64_t> cell_record_offsets_;
  DeviceBuffer<HPolytopeProposalDeviceRecord> records_;
  DeviceBuffer<std::uint64_t> failure_code_;
};

[[nodiscard]] HPolytopeProposalCudaResources& resources(
    HPolytopeProposalContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<HPolytopeProposalCudaResources>();
  }
  return *static_cast<HPolytopeProposalCudaResources*>(opaque.get());
}

[[nodiscard]] std::size_t plane_triple_count(std::size_t boundary_count) {
  if (boundary_count < 3U) {
    return 0U;
  }
  return boundary_count * (boundary_count - 1U) *
         (boundary_count - 2U) / 6U;
}

using device::DeviceInterval;

struct DeviceInterval3 {
  DeviceInterval x;
  DeviceInterval y;
  DeviceInterval z;
};

struct DeviceIntervalPlane4 {
  DeviceInterval3 normal;
  DeviceInterval offset;
  bool valid{false};
};

[[nodiscard]] inline __device__ double value_from_bits(
    std::uint64_t bits) noexcept {
  return __longlong_as_double(static_cast<long long int>(bits));
}

[[nodiscard]] inline __device__ std::uint64_t canonical_zero_bits(
    double value) noexcept {
  const double endpoint = value == 0.0 ? 0.0 : value;
  return static_cast<std::uint64_t>(__double_as_longlong(endpoint));
}

[[nodiscard]] inline __device__ bool finite_bits(
    std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

[[nodiscard]] inline __device__ DeviceInterval interval_from_bits(
    std::uint64_t lower_bits, std::uint64_t upper_bits) noexcept {
  if (!finite_bits(lower_bits) || !finite_bits(upper_bits)) {
    return device::invalid_interval();
  }
  return device::checked_interval(
      value_from_bits(lower_bits), value_from_bits(upper_bits));
}

[[nodiscard]] inline __device__ bool contains_zero(
    const DeviceInterval& value) noexcept {
  return value.valid && value.lower <= 0.0 && value.upper >= 0.0;
}

[[nodiscard]] inline __device__ DeviceInterval negate_interval(
    const DeviceInterval& value) noexcept {
  if (!value.valid) {
    return device::invalid_interval();
  }
  return device::checked_interval(-value.upper, -value.lower);
}

[[nodiscard]] inline __device__ DeviceInterval divide_intervals(
    const DeviceInterval& numerator,
    const DeviceInterval& denominator) noexcept {
  if (!numerator.valid || !denominator.valid ||
      contains_zero(denominator)) {
    return device::invalid_interval();
  }
  const double lower_candidates[4]{
      __ddiv_rd(numerator.lower, denominator.lower),
      __ddiv_rd(numerator.lower, denominator.upper),
      __ddiv_rd(numerator.upper, denominator.lower),
      __ddiv_rd(numerator.upper, denominator.upper)};
  const double upper_candidates[4]{
      __ddiv_ru(numerator.lower, denominator.lower),
      __ddiv_ru(numerator.lower, denominator.upper),
      __ddiv_ru(numerator.upper, denominator.lower),
      __ddiv_ru(numerator.upper, denominator.upper)};
  double lower = lower_candidates[0U];
  double upper = upper_candidates[0U];
  for (std::size_t index = 0U; index < 4U; ++index) {
    if (!device::is_finite(lower_candidates[index]) ||
        !device::is_finite(upper_candidates[index])) {
      return device::invalid_interval();
    }
    lower = lower_candidates[index] < lower ? lower_candidates[index] : lower;
    upper = upper_candidates[index] > upper ? upper_candidates[index] : upper;
  }
  return device::checked_interval(lower, upper);
}

[[nodiscard]] inline __device__ DeviceInterval3 cross_intervals(
    const DeviceInterval3& left,
    const DeviceInterval3& right) noexcept {
  return DeviceInterval3{
      device::subtract_intervals(
          device::multiply_intervals(left.y, right.z),
          device::multiply_intervals(left.z, right.y)),
      device::subtract_intervals(
          device::multiply_intervals(left.z, right.x),
          device::multiply_intervals(left.x, right.z)),
      device::subtract_intervals(
          device::multiply_intervals(left.x, right.y),
          device::multiply_intervals(left.y, right.x))};
}

[[nodiscard]] inline __device__ DeviceInterval dot_intervals(
    const DeviceInterval3& left,
    const DeviceInterval3& right) noexcept {
  return device::add_intervals(
      device::add_intervals(
          device::multiply_intervals(left.x, right.x),
          device::multiply_intervals(left.y, right.y)),
      device::multiply_intervals(left.z, right.z));
}

[[nodiscard]] inline __device__ DeviceInterval3 scale_intervals(
    const DeviceInterval& scale,
    const DeviceInterval3& value) noexcept {
  return DeviceInterval3{
      device::multiply_intervals(scale, value.x),
      device::multiply_intervals(scale, value.y),
      device::multiply_intervals(scale, value.z)};
}

[[nodiscard]] inline __device__ DeviceInterval3 add_intervals3(
    const DeviceInterval3& first,
    const DeviceInterval3& second,
    const DeviceInterval3& third) noexcept {
  return DeviceInterval3{
      device::add_intervals(
          device::add_intervals(first.x, second.x), third.x),
      device::add_intervals(
          device::add_intervals(first.y, second.y), third.y),
      device::add_intervals(
          device::add_intervals(first.z, second.z), third.z)};
}

[[nodiscard]] inline __device__ DeviceIntervalPlane4 load_plane(
    const HPolytopeProposalInputBoundary& boundary) noexcept {
  DeviceIntervalPlane4 result;
  result.normal.x = interval_from_bits(
      boundary.coefficient_lower_bits[0U],
      boundary.coefficient_upper_bits[0U]);
  result.normal.y = interval_from_bits(
      boundary.coefficient_lower_bits[1U],
      boundary.coefficient_upper_bits[1U]);
  result.normal.z = interval_from_bits(
      boundary.coefficient_lower_bits[2U],
      boundary.coefficient_upper_bits[2U]);
  result.offset = interval_from_bits(
      boundary.coefficient_lower_bits[3U],
      boundary.coefficient_upper_bits[3U]);
  result.valid = result.normal.x.valid && result.normal.y.valid &&
                 result.normal.z.valid && result.offset.valid;
  return result;
}

[[nodiscard]] inline __device__ DeviceInterval evaluate_interval(
    const DeviceIntervalPlane4& plane,
    const DeviceInterval3& point) noexcept {
  if (!plane.valid) {
    return device::invalid_interval();
  }
  return device::add_intervals(
      dot_intervals(plane.normal, point), plane.offset);
}

[[nodiscard]] inline __device__ bool unrank_lexicographic_triple(
    std::size_t boundary_count,
    std::size_t ordinal,
    std::size_t& first,
    std::size_t& second,
    std::size_t& third) noexcept {
  first = 0U;
  while (first + 2U < boundary_count) {
    const std::size_t remaining = boundary_count - first - 1U;
    const std::size_t first_block = remaining * (remaining - 1U) / 2U;
    if (ordinal < first_block) {
      break;
    }
    ordinal -= first_block;
    ++first;
  }
  if (first + 2U >= boundary_count) {
    return false;
  }
  second = first + 1U;
  while (second + 1U < boundary_count) {
    const std::size_t second_block = boundary_count - second - 1U;
    if (ordinal < second_block) {
      break;
    }
    ordinal -= second_block;
    ++second;
  }
  if (second + 1U >= boundary_count) {
    return false;
  }
  third = second + 1U + ordinal;
  return third < boundary_count;
}

inline __device__ void signal_failure(
    std::uint64_t* failure_code,
    std::uint64_t code) noexcept {
  static_assert(sizeof(std::uint64_t) == sizeof(unsigned long long));
  static_cast<void>(atomicCAS(
      reinterpret_cast<unsigned long long*>(failure_code),
      0ULL,
      static_cast<unsigned long long>(code)));
}

[[nodiscard]] inline __device__ HPolytopeProposalDeviceRecord base_record(
    std::uint64_t cell_id,
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::uint64_t epoch) noexcept {
  HPolytopeProposalDeviceRecord result{};
  result.initialized_slot_sentinel =
      kHPolytopeProposalInitializedSlotSentinel;
  result.buffer_epoch = epoch;
  result.cell_id = cell_id;
  result.first_boundary_index = static_cast<std::uint64_t>(first);
  result.second_boundary_index = static_cast<std::uint64_t>(second);
  result.third_boundary_index = static_cast<std::uint64_t>(third);
  result.status_code = static_cast<std::uint64_t>(
      HPolytopeProposalDeviceRecordStatus::unknown_requires_cpu_exact);
  result.strict_reject_boundary_witness =
      kHPolytopeProposalNoBoundaryWitness;
  return result;
}

[[nodiscard]] inline __device__ HPolytopeProposalDeviceRecord propose_record(
    std::uint64_t cell_id,
    const HPolytopeProposalInputBoundary* boundaries,
    std::size_t boundary_count,
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::uint64_t epoch) noexcept {
  HPolytopeProposalDeviceRecord result =
      base_record(cell_id, first, second, third, epoch);
  const DeviceIntervalPlane4 first_plane = load_plane(boundaries[first]);
  const DeviceIntervalPlane4 second_plane = load_plane(boundaries[second]);
  const DeviceIntervalPlane4 third_plane = load_plane(boundaries[third]);
  if (!first_plane.valid || !second_plane.valid || !third_plane.valid) {
    return result;
  }

  const DeviceInterval3 second_cross_third =
      cross_intervals(second_plane.normal, third_plane.normal);
  const DeviceInterval determinant =
      dot_intervals(first_plane.normal, second_cross_third);
  if (!determinant.valid || contains_zero(determinant)) {
    return result;
  }
  const DeviceInterval3 numerator = add_intervals3(
      scale_intervals(
          negate_interval(first_plane.offset), second_cross_third),
      scale_intervals(
          negate_interval(second_plane.offset),
          cross_intervals(third_plane.normal, first_plane.normal)),
      scale_intervals(
          negate_interval(third_plane.offset),
          cross_intervals(first_plane.normal, second_plane.normal)));
  const DeviceInterval3 point{
      divide_intervals(numerator.x, determinant),
      divide_intervals(numerator.y, determinant),
      divide_intervals(numerator.z, determinant)};
  if (!point.x.valid || !point.y.valid || !point.z.valid) {
    return result;
  }

  bool feasibility_unknown = false;
  std::uint64_t could_be_active_boundary_mask =
      (UINT64_C(1) << first) | (UINT64_C(1) << second) |
      (UINT64_C(1) << third);
  for (std::size_t boundary_index = 0U;
       boundary_index < boundary_count;
       ++boundary_index) {
    const DeviceInterval evaluation = evaluate_interval(
        load_plane(boundaries[boundary_index]), point);
    if (!evaluation.valid) {
      feasibility_unknown = true;
      continue;
    }
    if (evaluation.lower > 0.0) {
      result.status_code = static_cast<std::uint64_t>(
          HPolytopeProposalDeviceRecordStatus::proposed_strict_reject);
      result.strict_reject_boundary_witness =
          static_cast<std::uint64_t>(boundary_index);
      return result;
    }
    if (evaluation.upper > 0.0) {
      feasibility_unknown = true;
    }
    if (contains_zero(evaluation)) {
      could_be_active_boundary_mask |= UINT64_C(1) << boundary_index;
    }
  }
  if (feasibility_unknown) {
    return result;
  }

  result.status_code = static_cast<std::uint64_t>(
      HPolytopeProposalDeviceRecordStatus::proposed_survivor);
  result.coordinate_lower_bits[0U] = canonical_zero_bits(point.x.lower);
  result.coordinate_lower_bits[1U] = canonical_zero_bits(point.y.lower);
  result.coordinate_lower_bits[2U] = canonical_zero_bits(point.z.lower);
  result.coordinate_upper_bits[0U] = canonical_zero_bits(point.x.upper);
  result.coordinate_upper_bits[1U] = canonical_zero_bits(point.y.upper);
  result.coordinate_upper_bits[2U] = canonical_zero_bits(point.z.upper);
  result.could_be_active_boundary_mask = could_be_active_boundary_mask;
  return result;
}

__global__ void morsehgp3d_phase7_h_polytope_proposal_kernel(
    const HPolytopeProposalInputCell* cells,
    const HPolytopeProposalInputBoundary* boundaries,
    const std::uint64_t* cell_record_offsets,
    HPolytopeProposalDeviceRecord* records,
    std::uint64_t* failure_code,
    std::size_t cell_count,
    std::size_t boundary_count_total,
    std::size_t record_capacity,
    std::uint64_t epoch) {
  std::size_t cell_index = static_cast<std::size_t>(blockIdx.x);
  while (cell_index < cell_count) {
    const HPolytopeProposalInputCell cell = cells[cell_index];
    const std::size_t boundary_begin =
        static_cast<std::size_t>(cell.boundary_begin);
    const std::size_t boundary_end =
        static_cast<std::size_t>(cell.boundary_end);
    const std::size_t record_begin =
        static_cast<std::size_t>(cell_record_offsets[cell_index]);
    const std::size_t record_end =
        static_cast<std::size_t>(cell_record_offsets[cell_index + 1U]);
    if (boundary_begin > boundary_end ||
        boundary_end > boundary_count_total || record_begin > record_end ||
        record_end > record_capacity) {
      signal_failure(failure_code, UINT64_C(1));
    } else if (record_begin != record_end) {
      const std::size_t local_boundary_count = boundary_end - boundary_begin;
      const std::size_t required_record_count =
          local_boundary_count >= 3U
              ? local_boundary_count * (local_boundary_count - 1U) *
                    (local_boundary_count - 2U) / 6U
              : 0U;
      if (local_boundary_count < 6U ||
          local_boundary_count > kMaximumBoundaryCount ||
          record_end - record_begin != required_record_count) {
        signal_failure(failure_code, UINT64_C(2));
      } else {
        std::size_t ordinal = static_cast<std::size_t>(threadIdx.x);
        while (ordinal < required_record_count) {
          std::size_t first = 0U;
          std::size_t second = 0U;
          std::size_t third = 0U;
          if (!unrank_lexicographic_triple(
                  local_boundary_count,
                  ordinal,
                  first,
                  second,
                  third)) {
            signal_failure(failure_code, UINT64_C(3));
          } else {
            records[record_begin + ordinal] = propose_record(
                cell.cell_id,
                boundaries + boundary_begin,
                local_boundary_count,
                first,
                second,
                third,
                epoch);
          }
          ordinal += static_cast<std::size_t>(blockDim.x);
        }
      }
    }
    cell_index += static_cast<std::size_t>(gridDim.x);
  }
}

}  // namespace

HPolytopeProposalDeviceBatch propose_h_polytope_transcript_on_gpu(
    HPolytopeProposalContextState& context,
    std::span<const HPolytopeProposalInputCell> cells,
    std::span<const HPolytopeProposalInputBoundary> boundaries,
    std::size_t maximum_total_proposal_record_count) {
  if (cells.empty() || boundaries.empty() ||
      maximum_total_proposal_record_count == 0U) {
    throw std::invalid_argument(
        "a Phase 7 H-polytope CUDA proposal requires nonempty buffers");
  }
  if (!std::in_range<std::uint64_t>(
          maximum_total_proposal_record_count) ||
      cells.size() == std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(
        "a Phase 7 H-polytope CUDA proposal size is not representable");
  }

  HPolytopeProposalDeviceBatch batch;
  batch.cell_ids.reserve(cells.size());
  batch.cell_statuses.reserve(cells.size());
  batch.cell_record_offsets.reserve(cells.size() + 1U);
  batch.cell_record_offsets.push_back(0U);
  std::size_t record_count = 0U;
  std::size_t expected_boundary_begin = 0U;
  for (std::size_t cell_index = 0U; cell_index < cells.size(); ++cell_index) {
    const HPolytopeProposalInputCell& cell = cells[cell_index];
    if (cell.unsupported_projection > 1U ||
        cell.force_interval_fallback > 1U ||
        !std::in_range<std::size_t>(cell.boundary_begin) ||
        !std::in_range<std::size_t>(cell.boundary_end)) {
      throw std::invalid_argument(
          "a Phase 7 H-polytope input cell has malformed metadata");
    }
    const std::size_t boundary_begin =
        static_cast<std::size_t>(cell.boundary_begin);
    const std::size_t boundary_end =
        static_cast<std::size_t>(cell.boundary_end);
    if (boundary_begin != expected_boundary_begin ||
        boundary_begin > boundary_end || boundary_end > boundaries.size() ||
        (cell_index != 0U &&
         cells[cell_index - 1U].cell_id >= cell.cell_id)) {
      throw std::invalid_argument(
          "Phase 7 H-polytope cells or boundary offsets are not canonical");
    }
    expected_boundary_begin = boundary_end;
    const std::size_t boundary_count = boundary_end - boundary_begin;
    if (boundary_count < 6U || boundary_count > kMaximumBoundaryCount) {
      throw std::invalid_argument(
          "a Phase 7 H-polytope cell is outside 6 <= B <= 61");
    }
    const std::size_t required = plane_triple_count(boundary_count);
    batch.cell_ids.push_back(cell.cell_id);
    HPolytopeProposalDeviceCellStatus status =
        HPolytopeProposalDeviceCellStatus::validated_exhaustive_transcript;
    if (cell.unsupported_projection != 0U) {
      status = HPolytopeProposalDeviceCellStatus::exact_fallback_unsupported_projection;
    } else if (cell.force_interval_fallback != 0U) {
      status = HPolytopeProposalDeviceCellStatus::exact_fallback_interval_unknown;
    } else if (required >
               maximum_total_proposal_record_count - record_count) {
      status = HPolytopeProposalDeviceCellStatus::exact_fallback_capacity_exhausted;
    } else {
      record_count += required;
    }
    batch.cell_statuses.push_back(status);
    batch.cell_record_offsets.push_back(
        static_cast<std::uint64_t>(record_count));
  }
  if (expected_boundary_begin != boundaries.size()) {
    throw std::invalid_argument(
        "Phase 7 H-polytope boundary offsets do not close their CSR");
  }
  constexpr std::uint64_t negative_zero_bits =
      UINT64_C(0x8000000000000000);
  for (const HPolytopeProposalInputBoundary& boundary : boundaries) {
    for (std::size_t coefficient = 0U; coefficient < 4U; ++coefficient) {
      const std::uint64_t lower_bits =
          boundary.coefficient_lower_bits[coefficient];
      const std::uint64_t upper_bits =
          boundary.coefficient_upper_bits[coefficient];
      if ((lower_bits & kExponentMask) == kExponentMask ||
          (upper_bits & kExponentMask) == kExponentMask ||
          lower_bits == negative_zero_bits ||
          upper_bits == negative_zero_bits ||
          std::bit_cast<double>(lower_bits) >
              std::bit_cast<double>(upper_bits)) {
        throw std::invalid_argument(
            "Phase 7 H-polytope coefficient intervals are not canonical");
      }
    }
  }
  batch.record_count = static_cast<std::uint64_t>(record_count);
  batch.records.resize(maximum_total_proposal_record_count);

  if (context.current_epoch() ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the Phase 7 H-polytope CUDA epoch cannot advance");
  }
  const std::uint64_t expected_epoch =
      context.current_epoch() + UINT64_C(1);
  HPolytopeProposalCudaResources& cuda = resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.ensure_capacity(
        cells.size(),
        boundaries.size(),
        maximum_total_proposal_record_count);
    check_cuda(
        cudaMemcpyAsync(
            cuda.cells(),
            cells.data(),
            cells.size_bytes(),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 7 H-polytope cells host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            cuda.boundaries(),
            boundaries.data(),
            boundaries.size_bytes(),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 7 H-polytope boundaries host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            cuda.cell_record_offsets(),
            batch.cell_record_offsets.data(),
            batch.cell_record_offsets.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 7 H-polytope offsets host-to-device");
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0,
            maximum_total_proposal_record_count *
                sizeof(HPolytopeProposalDeviceRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 7 H-polytope physical record capacity");
    check_cuda(
        cudaMemsetAsync(
            cuda.failure_code(),
            0,
            sizeof(std::uint64_t),
            cuda.stream()),
        "cudaMemsetAsync Phase 7 H-polytope failure code");

    const std::size_t bounded_blocks = std::min(
        cells.size(), static_cast<std::size_t>(cuda.maximum_grid_x()));
    if (bounded_blocks == 0U ||
        bounded_blocks > std::numeric_limits<unsigned int>::max()) {
      throw std::length_error(
          "the Phase 7 H-polytope CUDA grid is not representable");
    }
    morsehgp3d_phase7_h_polytope_proposal_kernel
        <<<static_cast<unsigned int>(bounded_blocks),
           kThreadsPerBlock,
           0U,
           cuda.stream()>>>(
            cuda.cells(),
            cuda.boundaries(),
            cuda.cell_record_offsets(),
            cuda.records(),
            cuda.failure_code(),
            cells.size(),
            boundaries.size(),
            maximum_total_proposal_record_count,
            expected_epoch);
    check_cuda(
        cudaGetLastError(), "Phase 7 H-polytope proposal launch");

    std::uint64_t failure_code = 0U;
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            maximum_total_proposal_record_count *
                sizeof(HPolytopeProposalDeviceRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 7 H-polytope records device-to-host");
    check_cuda(
        cudaMemcpyAsync(
            &failure_code,
            cuda.failure_code(),
            sizeof(failure_code),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 7 H-polytope failure code device-to-host");
    cuda.synchronize();
    if (failure_code != 0U) {
      throw std::runtime_error(
          "the Phase 7 H-polytope kernel reported structural failure " +
          std::to_string(failure_code));
    }
    batch.buffer_epoch = context.advance_epoch();
    if (batch.buffer_epoch != expected_epoch) {
      throw std::logic_error(
          "the Phase 7 H-polytope CUDA epoch advanced unexpectedly");
    }
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
