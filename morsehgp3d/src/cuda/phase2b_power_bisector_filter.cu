#include "phase2b_power_bisector_filter_internal.hpp"
#include "phase2b_interval.cuh"

#include "morsehgp3d/exact/binary64.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase2b_power_bisector_filter.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 2B power-bisector filter requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 2B power-bisector filter must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kLabelCount = 2U;
constexpr std::size_t kWitnessFieldCount = 4U;
constexpr std::size_t kMaximumCardinality =
    maximum_power_bisector_cardinality;
constexpr std::size_t kCoordinateFieldCount =
    kWitnessFieldCount +
    kLabelCount * kMaximumCardinality * kAxisCount;

using device::DeviceInterval;
using device::add_intervals;
using device::multiply_intervals;
using device::point_interval;
using device::subtract_intervals;

[[nodiscard]] __host__ __device__ constexpr std::size_t label_field(
    std::size_t label, std::size_t point, std::size_t axis) noexcept {
  return kWitnessFieldCount +
         (label * kMaximumCardinality + point) * kAxisCount + axis;
}

[[nodiscard]] __device__ DeviceInterval field_interval(
    const std::uint64_t* coordinate_bits,
    std::size_t count,
    std::size_t index,
    std::size_t field) noexcept {
  return point_interval(coordinate_bits[field * count + index]);
}

// For y=A/D and D>0, evaluate the sign-equivalent homogeneous polynomial
//
//   G = D H_{R,Q}(A/D)
//     = sum_{i,a} (q_ia-r_ia) [(A_a-D r_ia)+(A_a-D q_ia)].
//
// The paired form is independent of the deterministic pairing, avoids all
// divisions and squares, and fails closed whenever an interval is non-finite
// or still contains zero.
[[nodiscard]] __device__ FilterSign filter_power_bisector(
    const std::uint64_t* coordinate_bits,
    const std::uint32_t* cardinalities,
    std::size_t count,
    std::size_t index) noexcept {
  const DeviceInterval denominator =
      field_interval(coordinate_bits, count, index, 3U);
  DeviceInterval homogeneous_value = point_interval(0U);
  const std::size_t cardinality =
      static_cast<std::size_t>(cardinalities[index]);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const DeviceInterval numerator =
        field_interval(coordinate_bits, count, index, axis);
    DeviceInterval axis_value = point_interval(0U);
    for (std::size_t point = 0U; point < cardinality; ++point) {
      const DeviceInterval r = field_interval(
          coordinate_bits, count, index, label_field(0U, point, axis));
      const DeviceInterval q = field_interval(
          coordinate_bits, count, index, label_field(1U, point, axis));
      const DeviceInterval q_minus_r = subtract_intervals(q, r);
      const DeviceInterval a_minus_dr = subtract_intervals(
          numerator, multiply_intervals(denominator, r));
      const DeviceInterval a_minus_dq = subtract_intervals(
          numerator, multiply_intervals(denominator, q));
      const DeviceInterval paired_sum =
          add_intervals(a_minus_dr, a_minus_dq);
      const DeviceInterval term =
          multiply_intervals(q_minus_r, paired_sum);
      axis_value = add_intervals(axis_value, term);
      if (!axis_value.valid) {
        return FilterSign::unknown;
      }
    }
    homogeneous_value = add_intervals(homogeneous_value, axis_value);
    if (!homogeneous_value.valid) {
      return FilterSign::unknown;
    }
  }
  if (!homogeneous_value.valid) {
    return FilterSign::unknown;
  }
  if (homogeneous_value.lower > 0.0) {
    return FilterSign::positive;
  }
  if (homogeneous_value.upper < 0.0) {
    return FilterSign::negative;
  }
  return FilterSign::unknown;
}

// All scalar fields are stored field-major. For each numerator, denominator,
// label point and axis, a warp therefore performs one coalesced load.
__global__ void morsehgp3d_phase2b_power_bisector_filter_kernel(
    const std::uint64_t* replay_ids,
    const std::uint64_t* coordinate_bits,
    const std::uint32_t* cardinalities,
    RawPowerBisectorFilterOutput* outputs,
    std::size_t count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }
  outputs[index].replay_id = replay_ids[index];
  outputs[index].sign = filter_power_bisector(
      coordinate_bits, cardinalities, count, index);
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
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
    throw CudaFailure(code, std::move(operation));
  }
}

class Stream final {
 public:
  Stream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags");
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
  cudaStream_t stream_{nullptr};
};

template <typename Value>
class DeviceBuffer final {
 public:
  explicit DeviceBuffer(std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error("Phase 2B device allocation size overflow");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
        "cudaMalloc");
  }

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  ~DeviceBuffer() {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
    }
  }

  [[nodiscard]] Value* get() noexcept { return data_; }

 private:
  Value* data_{nullptr};
};

[[nodiscard]] bool point_less(
    const PowerBisectorLabelPoint& left,
    const PowerBisectorLabelPoint& right) {
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t left_key =
        exact::binary64_total_order_key(left.coordinate_bits[axis]);
    const std::uint64_t right_key =
        exact::binary64_total_order_key(right.coordinate_bits[axis]);
    if (left_key != right_key) {
      return left_key < right_key;
    }
  }
  return left.point_id < right.point_id;
}

}  // namespace

std::vector<RawPowerBisectorFilterOutput> filter_power_bisectors_on_gpu(
    std::span<const PowerBisectorFilterInput> inputs) {
  if (inputs.empty()) {
    return {};
  }
  if (inputs.size() >
      std::numeric_limits<std::size_t>::max() / kCoordinateFieldCount) {
    throw std::length_error("Phase 2B power-bisector input packing overflow");
  }
  const std::size_t block_count =
      (inputs.size() - 1U) / kThreadsPerBlock + 1U;
  if (block_count > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error(
        "Phase 2B power-bisector batch exceeds the CUDA grid");
  }

  std::vector<std::uint64_t> replay_ids(inputs.size());
  std::vector<std::uint32_t> cardinalities(inputs.size());
  std::vector<std::uint64_t> coordinate_bits(
      inputs.size() * kCoordinateFieldCount, 0U);
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    replay_ids[index] = inputs[index].replay_id;
    cardinalities[index] = inputs[index].cardinality;
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      coordinate_bits[axis * inputs.size() + index] =
          inputs[index].witness_numerator_bits[axis];
    }
    coordinate_bits[3U * inputs.size() + index] =
        inputs[index].witness_denominator_bits;

    const std::size_t cardinality =
        static_cast<std::size_t>(inputs[index].cardinality);
    for (std::size_t label = 0U; label < kLabelCount; ++label) {
      std::array<PowerBisectorLabelPoint, kMaximumCardinality> points =
          label == 0U ? inputs[index].r_points : inputs[index].q_points;
      std::sort(points.begin(), points.begin() + cardinality, point_less);
      for (std::size_t point = 0U; point < cardinality; ++point) {
        for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
          coordinate_bits[
              label_field(label, point, axis) * inputs.size() + index] =
              points[point].coordinate_bits[axis];
        }
      }
    }
  }

  Stream stream;
  DeviceBuffer<std::uint64_t> device_replay_ids(replay_ids.size());
  DeviceBuffer<std::uint64_t> device_coordinate_bits(coordinate_bits.size());
  DeviceBuffer<std::uint32_t> device_cardinalities(cardinalities.size());
  DeviceBuffer<RawPowerBisectorFilterOutput> device_outputs(inputs.size());
  std::vector<RawPowerBisectorFilterOutput> outputs(inputs.size());

  check_cuda(
      cudaMemcpyAsync(
          device_replay_ids.get(),
          replay_ids.data(),
          replay_ids.size() * sizeof(std::uint64_t),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync power-bisector replay identifiers host-to-device");
  check_cuda(
      cudaMemcpyAsync(
          device_coordinate_bits.get(),
          coordinate_bits.data(),
          coordinate_bits.size() * sizeof(std::uint64_t),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync power-bisector coordinates host-to-device");
  check_cuda(
      cudaMemcpyAsync(
          device_cardinalities.get(),
          cardinalities.data(),
          cardinalities.size() * sizeof(std::uint32_t),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync power-bisector cardinalities host-to-device");
  morsehgp3d_phase2b_power_bisector_filter_kernel
      <<<static_cast<unsigned int>(block_count), kThreadsPerBlock, 0U,
         stream.get()>>>(
          device_replay_ids.get(),
          device_coordinate_bits.get(),
          device_cardinalities.get(),
          device_outputs.get(),
          inputs.size());
  check_cuda(cudaGetLastError(), "Phase 2B power-bisector filter launch");
  check_cuda(
      cudaMemcpyAsync(
          outputs.data(),
          device_outputs.get(),
          outputs.size() * sizeof(RawPowerBisectorFilterOutput),
          cudaMemcpyDeviceToHost,
          stream.get()),
      "cudaMemcpyAsync power-bisector results device-to-host");
  check_cuda(cudaStreamSynchronize(stream.get()), "cudaStreamSynchronize");
  return outputs;
}

}  // namespace morsehgp3d::gpu::detail
