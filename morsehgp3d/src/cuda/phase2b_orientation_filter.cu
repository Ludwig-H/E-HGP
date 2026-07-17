#include "phase2b_orientation_filter_internal.hpp"
#include "phase2b_interval.cuh"
#include "phase2b_predicate_context.cuh"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#if !defined(__CUDACC__)
#error "phase2b_orientation_filter.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 2B orientation filter requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 2B orientation filter must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kPointCount = 4U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kCoordinateFieldCount = kPointCount * kAxisCount;

using device::DeviceInterval;
using device::add_intervals;
using device::multiply_intervals;
using device::point_interval;
using device::subtract_intervals;

[[nodiscard]] __device__ DeviceInterval coordinate_interval(
    const std::uint64_t* coordinate_bits,
    std::size_t count,
    std::size_t index,
    std::size_t point,
    std::size_t axis) noexcept {
  const std::size_t field = point * kAxisCount + axis;
  return point_interval(coordinate_bits[field * count + index]);
}

[[nodiscard]] __device__ DeviceInterval product_difference(
    const DeviceInterval& left_first,
    const DeviceInterval& left_second,
    const DeviceInterval& right_first,
    const DeviceInterval& right_second) noexcept {
  return subtract_intervals(
      multiply_intervals(left_first, left_second),
      multiply_intervals(right_first, right_second));
}

// Sign convention is exactly the CPU convention:
// det([b-a, c-a, d-a]); (0, e1, e2, e3) is positive.
[[nodiscard]] __device__ FilterSign filter_orientation_3d(
    const std::uint64_t* coordinate_bits,
    std::size_t count,
    std::size_t index) noexcept {
  DeviceInterval u[kAxisCount]{};
  DeviceInterval v[kAxisCount]{};
  DeviceInterval w[kAxisCount]{};
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const DeviceInterval origin =
        coordinate_interval(coordinate_bits, count, index, 0U, axis);
    u[axis] = subtract_intervals(
        coordinate_interval(coordinate_bits, count, index, 1U, axis),
        origin);
    v[axis] = subtract_intervals(
        coordinate_interval(coordinate_bits, count, index, 2U, axis),
        origin);
    w[axis] = subtract_intervals(
        coordinate_interval(coordinate_bits, count, index, 3U, axis),
        origin);
  }

  const DeviceInterval first_minor =
      product_difference(v[1], w[2], v[2], w[1]);
  const DeviceInterval second_minor =
      product_difference(v[0], w[2], v[2], w[0]);
  const DeviceInterval third_minor =
      product_difference(v[0], w[1], v[1], w[0]);
  const DeviceInterval first_term = multiply_intervals(u[0], first_minor);
  const DeviceInterval second_term = multiply_intervals(u[1], second_minor);
  const DeviceInterval third_term = multiply_intervals(u[2], third_minor);
  const DeviceInterval determinant = add_intervals(
      subtract_intervals(first_term, second_term), third_term);
  if (!determinant.valid) {
    return FilterSign::unknown;
  }
  if (determinant.lower > 0.0) {
    return FilterSign::positive;
  }
  if (determinant.upper < 0.0) {
    return FilterSign::negative;
  }
  return FilterSign::unknown;
}

// Coordinates are stored field-major: all a.x words, then all a.y words, and
// so on through d.z. A warp therefore performs one coalesced load per field.
__global__ void morsehgp3d_phase2b_orientation_3d_filter_kernel(
    const std::uint64_t* coordinate_bits,
    FilterSign* outputs,
    std::size_t count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }
  outputs[index] = filter_orientation_3d(coordinate_bits, count, index);
}

}  // namespace

std::vector<FilterSign> filter_orientation_3d_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const Orientation3DFilterInput> inputs) {
  if (inputs.empty()) {
    return {};
  }
  if (inputs.size() >
      std::numeric_limits<std::size_t>::max() / kCoordinateFieldCount) {
    throw std::length_error("Phase 2B orientation input packing overflow");
  }
  const std::size_t block_count =
      (inputs.size() - 1U) / kThreadsPerBlock + 1U;
  if (block_count > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error("Phase 2B orientation batch exceeds the CUDA grid");
  }

  std::vector<std::uint64_t> coordinate_bits(
      inputs.size() * kCoordinateFieldCount);
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      coordinate_bits[(0U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].a_bits[axis];
      coordinate_bits[(1U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].b_bits[axis];
      coordinate_bits[(2U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].c_bits[axis];
      coordinate_bits[(3U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].d_bits[axis];
    }
  }

  std::vector<FilterSign> outputs(inputs.size());

  execute_predicate_filter_gpu_section(
      context,
      coordinate_bits.size(),
      0U,
      inputs.size(),
      [&](const PredicateFilterExecutionResources& resources) {
        check_predicate_filter_cuda(
            cudaMemcpyAsync(
                resources.coordinate_bits,
                coordinate_bits.data(),
                coordinate_bits.size() * sizeof(std::uint64_t),
                cudaMemcpyHostToDevice,
                resources.stream),
            "cudaMemcpyAsync orientation coordinates host-to-device");
        morsehgp3d_phase2b_orientation_3d_filter_kernel
            <<<static_cast<unsigned int>(block_count), kThreadsPerBlock, 0U,
               resources.stream>>>(
                resources.coordinate_bits, resources.outputs, inputs.size());
        check_predicate_filter_cuda(
            cudaGetLastError(), "Phase 2B orientation filter launch");
        check_predicate_filter_cuda(
            cudaMemcpyAsync(
                outputs.data(),
                resources.outputs,
                outputs.size() * sizeof(FilterSign),
                cudaMemcpyDeviceToHost,
                resources.stream),
            "cudaMemcpyAsync orientation results device-to-host");
      });
  return outputs;
}

}  // namespace morsehgp3d::gpu::detail
