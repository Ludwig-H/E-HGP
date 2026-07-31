#include "phase15_morton_yao48_ranked_pair_h0_projection_internal.hpp"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if !defined(__CUDACC__)
#error "phase15_morton_yao48_ranked_pair_h0_projection.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 15 ranked-pair H0 projection requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the ranked-pair H0 projection"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the ranked-pair H0 projection"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The ranked-pair H0 projection must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 128U;
constexpr std::size_t kAxisCount = 3U;

struct alignas(16) DeviceControl {
  unsigned long long regular_count{};
  unsigned long long diagnostic_count{};
  unsigned long long exact_level_unresolved_count{};
  unsigned long long malformed_count{};
};

static_assert(std::is_trivially_copyable_v<DeviceControl>);

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, std::move(operation))) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      std::string operation) {
    const char* description = cudaGetErrorString(code);
    return std::move(operation) + " failed: " +
        (description == nullptr ? std::string{"unknown CUDA error"}
                                : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

class DeviceGuard final {
 public:
  explicit DeviceGuard(int requested_device) {
    check_cuda(cudaGetDevice(&prior_device_), "cudaGetDevice H0 projection");
    if (prior_device_ != requested_device) {
      check_cuda(
          cudaSetDevice(requested_device), "cudaSetDevice H0 projection");
      changed_ = true;
    }
  }

  ~DeviceGuard() noexcept {
    if (changed_) {
      (void)cudaSetDevice(prior_device_);
    }
  }

  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;

 private:
  int prior_device_{};
  bool changed_{false};
};

template <class T>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  ~DeviceBuffer() noexcept { reset(); }
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  void allocate(std::size_t count, const char* operation) {
    if (count == 0U) {
      return;
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::length_error(
          "a ranked-pair H0 device allocation overflows size_t");
    }
    T* next = nullptr;
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&next),
            count * sizeof(T)),
        operation);
    pointer_ = next;
  }

  [[nodiscard]] T* get() noexcept { return pointer_; }
  [[nodiscard]] const T* get() const noexcept { return pointer_; }

  void reset() noexcept {
    if (pointer_ != nullptr) {
      (void)cudaFree(pointer_);
      pointer_ = nullptr;
    }
  }

  T* pointer_{};
};

class ProjectionResources final {
 public:
  ProjectionResources(int cuda_device, std::size_t record_count)
      : cuda_device_(cuda_device) {
    records_.allocate(
        record_count, "cudaMalloc ranked-pair H0 projection records");
    control_.allocate(
        1U, "cudaMalloc ranked-pair H0 projection scalar control");
  }

  ~ProjectionResources() noexcept {
    int prior_device = -1;
    const bool prior_read =
        cudaGetDevice(&prior_device) == cudaSuccess;
    const bool changed =
        prior_read && prior_device != cuda_device_ &&
        cudaSetDevice(cuda_device_) == cudaSuccess;
    records_.reset();
    control_.reset();
    if (changed) {
      (void)cudaSetDevice(prior_device);
    }
  }

  ProjectionResources(const ProjectionResources&) = delete;
  ProjectionResources& operator=(const ProjectionResources&) = delete;

  [[nodiscard]] Phase15MortonYao48RankedPairH0Record* records() noexcept {
    return records_.get();
  }

  [[nodiscard]] DeviceControl* control() noexcept {
    return control_.get();
  }

 private:
  int cuda_device_{-1};
  DeviceBuffer<Phase15MortonYao48RankedPairH0Record> records_;
  DeviceBuffer<DeviceControl> control_;
};

[[nodiscard]] constexpr unsigned int launch_blocks(
    std::size_t count) noexcept {
  return static_cast<unsigned int>(
      (count + static_cast<std::size_t>(kThreadsPerBlock) - 1U) /
      static_cast<std::size_t>(kThreadsPerBlock));
}

[[nodiscard]] __device__ bool payload_range_valid(
    const std::uint64_t* point_ids,
    std::uint64_t begin,
    std::uint64_t end,
    std::uint64_t payload_count,
    std::uint64_t point_count,
    std::uint64_t support_u,
    std::uint64_t support_v) noexcept {
  if (begin > end || end > payload_count ||
      (begin != end && point_ids == nullptr)) {
    return false;
  }
  for (std::uint64_t index = begin; index < end; ++index) {
    const std::uint64_t point_id = point_ids[index];
    if (point_id >= point_count || point_id == support_u ||
        point_id == support_v ||
        (index != begin && point_ids[index - UINT64_C(1)] >= point_id)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] __device__ bool payload_ranges_disjoint(
    const std::uint64_t* strict_ids,
    std::uint64_t strict_begin,
    std::uint64_t strict_end,
    const std::uint64_t* shell_ids,
    std::uint64_t shell_begin,
    std::uint64_t shell_end) noexcept {
  for (std::uint64_t strict_index = strict_begin;
       strict_index < strict_end;
       ++strict_index) {
    for (std::uint64_t shell_index = shell_begin;
         shell_index < shell_end;
         ++shell_index) {
      if (strict_ids[strict_index] == shell_ids[shell_index]) {
        return false;
      }
    }
  }
  return true;
}

__global__ void project_ranked_pairs_kernel(
    Phase15MortonYao48RankedPairH0ProjectionRequest request,
    Phase15MortonYao48RankedPairH0Record* records,
    DeviceControl* control) {
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= request.record_count) {
    return;
  }

  const std::uint64_t support_u = request.support_u[index];
  const std::uint64_t support_v = request.support_v[index];
  const std::uint64_t strict_begin = request.strict_offsets[index];
  const std::uint64_t strict_end =
      request.strict_offsets[index + UINT64_C(1)];
  const std::uint64_t shell_begin = request.shell_offsets[index];
  const std::uint64_t shell_end =
      request.shell_offsets[index + UINT64_C(1)];
  const std::uint64_t closed_rank = request.closed_rank[index];
  const std::uint64_t strict_count = strict_end - strict_begin;
  const std::uint64_t shell_count = shell_end - shell_begin;

  bool malformed =
      support_u >= support_v || support_v >= request.point_count ||
      closed_rank < UINT64_C(2) ||
      closed_rank > request.maximum_closed_rank ||
      strict_begin > strict_end || shell_begin > shell_end ||
      !payload_range_valid(
          request.strict_point_ids,
          strict_begin,
          strict_end,
          request.strict_point_id_count,
          request.point_count,
          support_u,
          support_v) ||
      !payload_range_valid(
          request.shell_point_ids,
          shell_begin,
          shell_end,
          request.shell_point_id_count,
          request.point_count,
          support_u,
          support_v);
  if (!malformed) {
    malformed =
        strict_count > UINT64_MAX - shell_count ||
        strict_count + shell_count != closed_rank - UINT64_C(2) ||
        !payload_ranges_disjoint(
            request.strict_point_ids,
            strict_begin,
            strict_end,
            request.shell_point_ids,
            shell_begin,
            shell_end);
  }
  if (malformed) {
    atomicAdd(&control->malformed_count, 1ULL);
    return;
  }

  std::uint64_t first_support_bits[kAxisCount]{};
  std::uint64_t second_support_bits[kAxisCount]{};
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    first_support_bits[axis] =
        request.coordinate_bits[axis * request.point_count + support_u];
    second_support_bits[axis] =
        request.coordinate_bits[axis * request.point_count + support_v];
  }
  const exact_pair_level_fixed::Result exact_level =
      exact_pair_level_fixed::exact_pair_level(
          first_support_bits, second_support_bits);
  if (!exact_pair_level_fixed::result_is_exact(exact_level)) {
    atomicAdd(&control->exact_level_unresolved_count, 1ULL);
    return;
  }

  Phase15MortonYao48RankedPairH0Record record;
  record.squared_level = exact_level.level;
  record.support_u = support_u;
  record.support_v = support_v;
  record.strict_begin = strict_begin;
  record.strict_end = strict_end;
  record.shell_begin = shell_begin;
  record.shell_end = shell_end;
  record.closed_rank = static_cast<std::uint8_t>(closed_rank);
  if (shell_count == 0U) {
    if (closed_rank != strict_count + UINT64_C(2)) {
      atomicAdd(&control->malformed_count, 1ULL);
      return;
    }
    record.kind =
        Phase15MortonYao48RankedPairH0RecordKind::
            regular_pair_candidate;
    atomicAdd(&control->regular_count, 1ULL);
  } else {
    record.kind =
        Phase15MortonYao48RankedPairH0RecordKind::
            diagnostic_or_carrier_candidate;
    atomicAdd(&control->diagnostic_count, 1ULL);
  }
  records[index] = record;
}

}  // namespace

Phase15MortonYao48RankedPairH0ProjectionReceipt
phase15_launch_morton_yao48_ranked_pair_h0_projection(
    const Phase15MortonYao48RankedPairH0ProjectionRequest& request) {
  if (request.host_fake || request.cuda_device < 0 ||
      request.maximum_closed_rank < 2U ||
      request.maximum_closed_rank >
          morton_yao48_ranked_pair_h0_projection_maximum_closed_rank ||
      request.record_count > request.record_capacity ||
      (request.record_count != 0U &&
       (request.coordinate_bits == nullptr ||
        request.support_u == nullptr || request.support_v == nullptr ||
        request.closed_rank == nullptr ||
        request.strict_offsets == nullptr ||
        request.shell_offsets == nullptr)) ||
      (request.strict_point_id_count != 0U &&
       request.strict_point_ids == nullptr) ||
      (request.shell_point_id_count != 0U &&
       request.shell_point_ids == nullptr)) {
    throw std::invalid_argument(
        "the ranked-pair H0 CUDA projection request is invalid");
  }

  DeviceGuard guard{request.cuda_device};
  Phase15MortonYao48RankedPairH0ProjectionReceipt receipt;
  receipt.point_count = request.point_count;
  receipt.maximum_closed_rank = request.maximum_closed_rank;
  receipt.record_capacity = request.record_capacity;
  receipt.source_record_count = request.record_count;
  receipt.projected_record_count = request.record_count;
  receipt.strict_point_id_count = request.strict_point_id_count;
  receipt.shell_point_id_count = request.shell_point_id_count;
  receipt.cuda_device = request.cuda_device;
  receipt.execution_kind =
      Phase15MortonYao48RankedPairH0ProjectionExecutionKind::cuda;
  receipt.fixed_linear_capacity_validated = true;
  receipt.one_record_per_source_validated = true;
  receipt.strict_and_shell_payload_ranges_validated = true;
  receipt.regular_pair_closed_rank_identity_validated = true;
  receipt.scientific_birth_and_saddle_roles_withheld = true;

  if (request.record_count == 0U) {
    receipt.exact_level_payload_constructed = true;
    receipt.exact_levels_computed_from_source_coordinates = true;
    return receipt;
  }

  auto resources =
      std::make_shared<ProjectionResources>(
          request.cuda_device, request.record_count);
  check_cuda(
      cudaMemset(
          resources->control(), 0, sizeof(DeviceControl)),
      "cudaMemset ranked-pair H0 projection scalar control");
  project_ranked_pairs_kernel<<<
      launch_blocks(request.record_count),
      kThreadsPerBlock>>>(
      request, resources->records(), resources->control());
  check_cuda(
      cudaGetLastError(), "ranked-pair H0 projection kernel launch");

  DeviceControl host_control{};
  check_cuda(
      cudaMemcpyAsync(
          &host_control,
          resources->control(),
          sizeof(host_control),
          cudaMemcpyDeviceToHost),
      "cudaMemcpyAsync ranked-pair H0 scalar control D2H");
  check_cuda(
      cudaDeviceSynchronize(),
      "cudaDeviceSynchronize ranked-pair H0 projection");

  receipt.kernel_launch_count = 1U;
  receipt.synchronization_count = 1U;
  receipt.scalar_control_device_to_host_count = 1U;
  receipt.regular_pair_candidate_count =
      static_cast<std::size_t>(host_control.regular_count);
  receipt.diagnostic_or_carrier_candidate_count =
      static_cast<std::size_t>(host_control.diagnostic_count);
  receipt.exact_level_unresolved_count =
      static_cast<std::size_t>(
          host_control.exact_level_unresolved_count);
  receipt.output_owner = resources;
  receipt.records = resources->records();

  if (host_control.malformed_count != 0ULL ||
      host_control.regular_count + host_control.diagnostic_count +
              host_control.exact_level_unresolved_count !=
          request.record_count) {
    receipt.failure_code =
        Phase15MortonYao48RankedPairH0ProjectionFailureCode::
            malformed_source;
    return receipt;
  }
  if (host_control.exact_level_unresolved_count != 0ULL) {
    receipt.failure_code =
        Phase15MortonYao48RankedPairH0ProjectionFailureCode::
            exact_level_unresolved;
    return receipt;
  }
  receipt.exact_level_payload_constructed = true;
  receipt.exact_levels_computed_from_source_coordinates = true;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
