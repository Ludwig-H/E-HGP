#include "phase14_facet_top_k_proposal_internal.hpp"

#include <cuda_runtime.h>
#include <cuda/std/bit>

#include <algorithm>
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
#error "phase14_facet_top_k_proposal.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 14 facet top-k proposal requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 14 facet top-k proposal"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 14 facet top-k proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 14 facet top-k proposal must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);

class Phase14FacetTopKProposalCudaFailure final
    : public std::runtime_error {
 public:
  Phase14FacetTopKProposalCudaFailure(
      cudaError_t code,
      std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw Phase14FacetTopKProposalCudaFailure(
        code, std::move(operation));
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
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 14 facet top-k proposal device allocation size is "
          "invalid");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
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
        "cudaGetDevice before Phase 14 facet top-k proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 14 facet top-k proposal context");
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
          "cudaSetDevice restore after Phase 14 facet top-k proposal");
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

[[nodiscard]] __host__ __device__ constexpr bool finite_bits(
    std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

void validate_resident_snapshot(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::uint64_t>::max()) {
    throw std::invalid_argument(
        "the Phase 14 facet top-k resident point set is empty or not "
        "representable");
  }
  if (point_count >
      std::numeric_limits<std::size_t>::max() / kAxisCount) {
    throw std::length_error(
        "the Phase 14 facet top-k coordinate extent overflows");
  }
  if (coordinate_bits.size() != point_count * kAxisCount ||
      morton_point_ids.size() != point_count) {
    throw std::invalid_argument(
        "the Phase 14 facet top-k resident snapshot has inconsistent "
        "extents");
  }
  for (const std::uint64_t bits : coordinate_bits) {
    if (!finite_bits(bits)) {
      throw std::invalid_argument(
          "Phase 14 facet top-k coordinates must be finite binary64 "
          "values");
    }
  }

  std::vector<unsigned char> point_id_seen(point_count, 0U);
  for (const std::uint64_t point_id : morton_point_ids) {
    if (point_id >= static_cast<std::uint64_t>(point_count)) {
      throw std::invalid_argument(
          "a Phase 14 facet top-k Morton point ID is out of range");
    }
    unsigned char& seen =
        point_id_seen[static_cast<std::size_t>(point_id)];
    if (seen != 0U) {
      throw std::invalid_argument(
          "the Phase 14 facet top-k Morton order is not a permutation");
    }
    seen = 1U;
  }
}

void validate_launch_inputs(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const Phase14FacetTopKProposalQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t morton_window_radius) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount ||
      morton_point_ids.size() != point_count) {
    throw std::invalid_argument(
        "the Phase 14 facet top-k launch has inconsistent resident "
        "extents");
  }
  if (queries.empty() || maximum_query_count == 0U ||
      queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "the Phase 14 facet top-k launch has an invalid query extent");
  }
  if (maximum_query_count >
      std::numeric_limits<std::size_t>::max() /
          sizeof(Phase14FacetTopKProposalDeviceRecord)) {
    throw std::length_error(
        "the Phase 14 facet top-k fixed proposal capacity overflows");
  }
  if (morton_window_radius == 0U) {
    throw std::invalid_argument(
        "the Phase 14 facet top-k Morton window radius must be positive");
  }
  constexpr std::uint64_t maximum_inspection_multiplier =
      2U * phase14_facet_top_k_proposal_maximum_point_count;
  if (morton_window_radius >
      std::numeric_limits<std::uint64_t>::max() /
          maximum_inspection_multiplier) {
    throw std::length_error(
        "the Phase 14 facet top-k inspection counter could overflow");
  }

  for (const Phase14FacetTopKProposalQueryInputRecord& query : queries) {
    if (query.query_index == phase14_facet_top_k_proposal_sentinel ||
        query.point_count == 0U ||
        query.point_count >
            phase14_facet_top_k_proposal_maximum_point_count) {
      throw std::invalid_argument(
          "a Phase 14 facet top-k query has an invalid identity or point "
          "count");
    }
    const std::size_t query_point_count =
        static_cast<std::size_t>(query.point_count);
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      if (!finite_bits(query.center_bits[axis])) {
        throw std::invalid_argument(
            "a Phase 14 facet top-k query center is not finite");
      }
    }
    for (std::size_t point = 0U; point < query_point_count; ++point) {
      const std::uint64_t point_id = query.point_ids[point];
      const std::uint64_t morton_position =
          query.morton_positions[point];
      if (point_id >= static_cast<std::uint64_t>(point_count) ||
          morton_position >= static_cast<std::uint64_t>(point_count)) {
        throw std::invalid_argument(
            "a Phase 14 facet top-k query source is out of range");
      }
      if (point != 0U && query.point_ids[point - 1U] >= point_id) {
        throw std::invalid_argument(
            "a Phase 14 facet top-k query key is not strictly increasing");
      }
      if (morton_point_ids[static_cast<std::size_t>(morton_position)] !=
          point_id) {
        throw std::invalid_argument(
            "a Phase 14 facet top-k source does not match its Morton "
            "position");
      }
    }
    const auto active_point_ids = std::span<const std::uint64_t>{
        query.point_ids, query_point_count};
    if (query.key_fingerprint !=
        phase14_facet_top_k_proposal_key_fingerprint(
            query_point_count, active_point_ids)) {
      throw std::invalid_argument(
          "a Phase 14 facet top-k query carries a noncanonical key "
          "fingerprint");
    }
  }
}

class Phase14FacetTopKProposalCudaResources final {
 public:
  Phase14FacetTopKProposalCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 14 facet top-k proposal context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 14 facet top-k proposal "
        "context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional Phase 14 facet "
          "top-k grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 14 facet top-k proposal "
        "context");
  }

  Phase14FacetTopKProposalCudaResources(
      const Phase14FacetTopKProposalCudaResources&) = delete;
  Phase14FacetTopKProposalCudaResources& operator=(
      const Phase14FacetTopKProposalCudaResources&) = delete;

  ~Phase14FacetTopKProposalCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      records_.abandon();
      queries_.abandon();
      morton_point_ids_.abandon();
      coordinate_bits_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    records_.reset();
    queries_.reset();
    morton_point_ids_.reset();
    coordinate_bits_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize(
      std::span<const std::uint64_t> coordinate_bits,
      std::size_t point_count,
      std::span<const std::uint64_t> morton_point_ids,
      std::size_t maximum_query_count) {
    if (initialized_) {
      if (point_count_ != point_count ||
          coordinate_bits_.count() != coordinate_bits.size() ||
          morton_point_ids_.count() != morton_point_ids.size() ||
          maximum_query_count_ != maximum_query_count ||
          queries_.count() != maximum_query_count ||
          records_.count() != maximum_query_count) {
        throw std::logic_error(
            "a Phase 14 facet top-k context changed its resident snapshot "
            "or fixed query capacity");
      }
      return;
    }

    validate_resident_snapshot(
        coordinate_bits, point_count, morton_point_ids);
    coordinate_bits_.allocate(
        coordinate_bits.size(),
        "cudaMalloc Phase 14 facet top-k coordinate snapshot");
    morton_point_ids_.allocate(
        morton_point_ids.size(),
        "cudaMalloc Phase 14 facet top-k Morton order snapshot");
    queries_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 14 facet top-k fixed query buffer");
    records_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 14 facet top-k fixed proposal buffer");
    check_cuda(
        cudaMemcpyAsync(
            coordinate_bits_.get(),
            coordinate_bits.data(),
            coordinate_bits.size_bytes(),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 14 facet top-k coordinates "
        "host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            morton_point_ids_.get(),
            morton_point_ids.data(),
            morton_point_ids.size_bytes(),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 14 facet top-k Morton order "
        "host-to-device");
    point_count_ = point_count;
    maximum_query_count_ = maximum_query_count;
    initialized_ = true;
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] const std::uint64_t* coordinate_bits() const noexcept {
    return coordinate_bits_.get();
  }
  [[nodiscard]] const std::uint64_t* morton_point_ids() const noexcept {
    return morton_point_ids_.get();
  }
  [[nodiscard]] Phase14FacetTopKProposalQueryInputRecord* queries() noexcept {
    return queries_.get();
  }
  [[nodiscard]] Phase14FacetTopKProposalDeviceRecord* records() noexcept {
    return records_.get();
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 14 facet top-k proposal context");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{};
  std::size_t point_count_{};
  std::size_t maximum_query_count_{};
  bool initialized_{false};
  DeviceBuffer<std::uint64_t> coordinate_bits_;
  DeviceBuffer<std::uint64_t> morton_point_ids_;
  DeviceBuffer<Phase14FacetTopKProposalQueryInputRecord> queries_;
  DeviceBuffer<Phase14FacetTopKProposalDeviceRecord> records_;
};

[[nodiscard]] Phase14FacetTopKProposalCudaResources& resources(
    Phase14FacetTopKProposalContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<Phase14FacetTopKProposalCudaResources>();
  }
  return *static_cast<Phase14FacetTopKProposalCudaResources*>(opaque.get());
}

[[nodiscard]] inline __device__ double value_from_bits(
    std::uint64_t bits) noexcept {
  return cuda::std::bit_cast<double>(bits);
}

[[nodiscard]] inline __device__ std::uint64_t bits_from_value(
    double value) noexcept {
  return cuda::std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] inline __device__ bool finite_value(double value) noexcept {
  return finite_bits(bits_from_value(value));
}

[[nodiscard]] inline __device__ bool point_is_a_source(
    std::uint64_t point_id,
    const std::uint64_t* source_point_ids,
    std::size_t source_point_count) noexcept {
  for (std::size_t source = 0U; source < source_point_count; ++source) {
    if (source_point_ids[source] == point_id) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline __device__ bool covered_by_an_earlier_window(
    std::size_t neighbor_position,
    const std::uint64_t* source_morton_positions,
    std::size_t source_ordinal,
    std::size_t window_radius) noexcept {
  for (std::size_t prior = 0U; prior < source_ordinal; ++prior) {
    const std::size_t prior_position =
        static_cast<std::size_t>(source_morton_positions[prior]);
    const std::size_t difference =
        prior_position > neighbor_position
            ? prior_position - neighbor_position
            : neighbor_position - prior_position;
    if (difference != 0U && difference <= window_radius) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline __device__ bool candidate_is_better(
    double first_distance,
    std::uint64_t first_point_id,
    double second_distance,
    std::uint64_t second_point_id) noexcept {
  return first_distance < second_distance ||
         (first_distance == second_distance &&
          first_point_id < second_point_id);
}

[[nodiscard]] __forceinline__ __device__ bool inspect_neighbor(
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    const std::uint64_t* morton_point_ids,
    std::size_t neighbor_position,
    const double* center,
    const std::uint64_t* source_point_ids,
    const std::uint64_t* source_morton_positions,
    std::size_t source_point_count,
    std::size_t source_ordinal,
    std::size_t window_radius,
    std::uint64_t* candidate_point_ids,
    double* candidate_squared_distances,
    std::size_t& candidate_count,
    std::uint64_t& inspected_neighbor_count,
    std::uint64_t& floating_distance_evaluation_count,
    std::uint64_t& floating_rejection_count,
    std::uint64_t& failure_code) noexcept {
  ++inspected_neighbor_count;
  const std::uint64_t target_point_id =
      morton_point_ids[neighbor_position];
  if (target_point_id >= static_cast<std::uint64_t>(point_count)) {
    failure_code = static_cast<std::uint64_t>(
        Phase14FacetTopKProposalFailureCode::neighbor_point_out_of_range);
    return false;
  }
  if (point_is_a_source(
          target_point_id, source_point_ids, source_point_count) ||
      covered_by_an_earlier_window(
          neighbor_position,
          source_morton_positions,
          source_ordinal,
          window_radius)) {
    return true;
  }

  ++floating_distance_evaluation_count;
  double squared_distance = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double target_coordinate = value_from_bits(
        coordinate_bits[
            axis * point_count +
            static_cast<std::size_t>(target_point_id)]);
    const double difference = __dsub_rn(center[axis], target_coordinate);
    const double squared_difference = __dmul_rn(difference, difference);
    squared_distance = __dadd_rn(
        squared_distance, squared_difference);
  }
  if (squared_distance != squared_distance || squared_distance < 0.0) {
    failure_code = static_cast<std::uint64_t>(
        Phase14FacetTopKProposalFailureCode::non_orderable_distance);
    return false;
  }
  if (!finite_value(squared_distance)) {
    ++floating_rejection_count;
    return true;
  }

  if (candidate_count < source_point_count) {
    candidate_point_ids[candidate_count] = target_point_id;
    candidate_squared_distances[candidate_count] = squared_distance;
    ++candidate_count;
    return true;
  }

  std::size_t worst = 0U;
  for (std::size_t candidate = 1U;
       candidate < candidate_count;
       ++candidate) {
    if (candidate_is_better(
            candidate_squared_distances[worst],
            candidate_point_ids[worst],
            candidate_squared_distances[candidate],
            candidate_point_ids[candidate])) {
      worst = candidate;
    }
  }
  if (candidate_is_better(
          squared_distance,
          target_point_id,
          candidate_squared_distances[worst],
          candidate_point_ids[worst])) {
    candidate_point_ids[worst] = target_point_id;
    candidate_squared_distances[worst] = squared_distance;
  }
  ++floating_rejection_count;
  return true;
}

__global__ void morsehgp3d_phase14_facet_top_k_proposal_kernel(
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    const std::uint64_t* morton_point_ids,
    const Phase14FacetTopKProposalQueryInputRecord* queries,
    std::size_t query_count,
    std::size_t window_radius,
    std::uint64_t buffer_epoch,
    Phase14FacetTopKProposalDeviceRecord* records) {
  const std::size_t first_query =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t query_offset = first_query;
  while (query_offset < query_count) {
    // Keep the 208-byte query and 144-byte output record in resident global
    // storage.  Only the fixed-k selection scratch is thread-local; copying
    // either POD here materially inflates the Blackwell per-thread stack.
    const Phase14FacetTopKProposalQueryInputRecord* query =
        queries + query_offset;
    std::uint64_t inspected_neighbor_count = 0U;
    std::uint64_t floating_distance_evaluation_count = 0U;
    std::uint64_t floating_rejection_count = 0U;
    std::uint64_t failure_code = static_cast<std::uint64_t>(
        Phase14FacetTopKProposalFailureCode::none);
    const bool point_count_valid =
        query->point_count != 0U &&
        query->point_count <=
            phase14_facet_top_k_proposal_maximum_point_count;
    const std::size_t source_point_count =
        point_count_valid
            ? static_cast<std::size_t>(query->point_count)
            : 0U;
    bool source_valid = point_count_valid;
    if (!point_count_valid) {
      failure_code = static_cast<std::uint64_t>(
          Phase14FacetTopKProposalFailureCode::invalid_point_count);
    }
    for (std::size_t source = 0U;
         source < source_point_count;
         ++source) {
      const std::uint64_t source_point_id =
          query->point_ids[source];
      const std::uint64_t source_morton_position =
          query->morton_positions[source];
      if (source_point_id >= static_cast<std::uint64_t>(point_count)) {
        failure_code = static_cast<std::uint64_t>(
            Phase14FacetTopKProposalFailureCode::
                source_point_out_of_range);
        source_valid = false;
        break;
      }
      if (source_morton_position >=
          static_cast<std::uint64_t>(point_count)) {
        failure_code = static_cast<std::uint64_t>(
            Phase14FacetTopKProposalFailureCode::
                source_morton_position_out_of_range);
        source_valid = false;
        break;
      }
      if (morton_point_ids[
              static_cast<std::size_t>(source_morton_position)] !=
          source_point_id) {
        failure_code = static_cast<std::uint64_t>(
            Phase14FacetTopKProposalFailureCode::
                source_morton_position_mismatch);
        source_valid = false;
        break;
      }
      if (source != 0U &&
          query->point_ids[source - 1U] >= source_point_id) {
        failure_code = static_cast<std::uint64_t>(
            Phase14FacetTopKProposalFailureCode::
                non_canonical_source_key);
        source_valid = false;
        break;
      }
    }

    double center[kAxisCount];
    if (source_valid) {
      for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
        center[axis] = value_from_bits(query->center_bits[axis]);
        if (!finite_value(center[axis])) {
          failure_code = static_cast<std::uint64_t>(
              Phase14FacetTopKProposalFailureCode::
                  non_orderable_distance);
          source_valid = false;
          break;
        }
      }
    }

    std::uint64_t candidate_point_ids[
        phase14_facet_top_k_proposal_maximum_point_count];
    double candidate_squared_distances[
        phase14_facet_top_k_proposal_maximum_point_count];
    std::size_t candidate_count = 0U;
    if (source_valid) {
      for (std::size_t source = 0U;
           source < source_point_count && source_valid;
           ++source) {
        const std::size_t source_position =
            static_cast<std::size_t>(
                query->morton_positions[source]);
        const std::size_t left_count =
            window_radius < source_position
                ? window_radius
                : source_position;
        for (std::size_t offset = 1U;
             offset <= left_count;
             ++offset) {
          if (!inspect_neighbor(
                  coordinate_bits,
                  point_count,
                  morton_point_ids,
                  source_position - offset,
                  center,
                  query->point_ids,
                  query->morton_positions,
                  source_point_count,
                  source,
                  window_radius,
                  candidate_point_ids,
                  candidate_squared_distances,
                  candidate_count,
                  inspected_neighbor_count,
                  floating_distance_evaluation_count,
                  floating_rejection_count,
                  failure_code)) {
            source_valid = false;
            break;
          }
        }
        if (!source_valid) {
          break;
        }
        const std::size_t right_available =
            point_count - 1U - source_position;
        const std::size_t right_count =
            window_radius < right_available
                ? window_radius
                : right_available;
        for (std::size_t offset = 1U;
             offset <= right_count;
             ++offset) {
          if (!inspect_neighbor(
                  coordinate_bits,
                  point_count,
                  morton_point_ids,
                  source_position + offset,
                  center,
                  query->point_ids,
                  query->morton_positions,
                  source_point_count,
                  source,
                  window_radius,
                  candidate_point_ids,
                  candidate_squared_distances,
                  candidate_count,
                  inspected_neighbor_count,
                  floating_distance_evaluation_count,
                  floating_rejection_count,
                  failure_code)) {
            source_valid = false;
            break;
          }
        }
      }
    }

    for (std::size_t candidate = 1U;
         candidate < candidate_count;
         ++candidate) {
      const std::uint64_t point_id = candidate_point_ids[candidate];
      std::size_t insertion = candidate;
      while (insertion != 0U &&
             point_id < candidate_point_ids[insertion - 1U]) {
        candidate_point_ids[insertion] =
            candidate_point_ids[insertion - 1U];
        --insertion;
      }
      candidate_point_ids[insertion] = point_id;
    }
    Phase14FacetTopKProposalDeviceRecord* record =
        records + query_offset;
    record->query_index = query->query_index;
    record->key_fingerprint = query->key_fingerprint;
    record->buffer_epoch = buffer_epoch;
    record->candidate_count =
        static_cast<std::uint64_t>(candidate_count);
    record->inspected_neighbor_count = inspected_neighbor_count;
    record->floating_distance_evaluation_count =
        floating_distance_evaluation_count;
    record->floating_rejection_count = floating_rejection_count;
    record->failure_code = failure_code;
    for (std::size_t candidate = 0U;
         candidate < candidate_count;
         ++candidate) {
      record->candidates[candidate] = candidate_point_ids[candidate];
    }

    if (query_count - query_offset <= stride) {
      break;
    }
    query_offset += stride;
  }
}

}  // namespace

Phase14FacetTopKProposalDeviceBatch
propose_phase14_facet_top_k_candidates_on_gpu(
    Phase14FacetTopKProposalContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const Phase14FacetTopKProposalQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t morton_window_radius) {
  validate_launch_inputs(
      coordinate_bits,
      point_count,
      morton_point_ids,
      queries,
      maximum_query_count,
      morton_window_radius);
  if (context.current_epoch() ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the Phase 14 facet top-k proposal epoch cannot advance");
  }
  const std::uint64_t expected_epoch =
      context.current_epoch() + UINT64_C(1);

  Phase14FacetTopKProposalCudaResources& cuda = resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.initialize(
        coordinate_bits,
        point_count,
        morton_point_ids,
        maximum_query_count);
    check_cuda(
        cudaMemcpyAsync(
            cuda.queries(),
            queries.data(),
            queries.size_bytes(),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 14 facet top-k queries host-to-device");
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0xff,
            queries.size() *
                sizeof(Phase14FacetTopKProposalDeviceRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 14 active facet top-k proposal sentinels");

    const std::size_t requested_blocks =
        (queries.size() - 1U) / kThreadsPerBlock + 1U;
    const std::size_t bounded_blocks = std::min(
        requested_blocks,
        static_cast<std::size_t>(cuda.maximum_grid_x()));
    if (bounded_blocks == 0U ||
        bounded_blocks > std::numeric_limits<unsigned int>::max()) {
      throw std::length_error(
          "the Phase 14 facet top-k CUDA grid is not representable");
    }
    morsehgp3d_phase14_facet_top_k_proposal_kernel
        <<<static_cast<unsigned int>(bounded_blocks),
           kThreadsPerBlock,
           0U,
           cuda.stream()>>>(
            cuda.coordinate_bits(),
            point_count,
            cuda.morton_point_ids(),
            cuda.queries(),
            queries.size(),
            morton_window_radius,
            expected_epoch,
            cuda.records());
    check_cuda(
        cudaGetLastError(),
        "Phase 14 facet top-k proposal kernel launch");

    Phase14FacetTopKProposalDeviceBatch batch;
    batch.records.resize(queries.size());
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            queries.size() *
                sizeof(Phase14FacetTopKProposalDeviceRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 14 active facet top-k proposals device-to-host");
    cuda.synchronize();
    batch.record_count = queries.size();
    batch.kernel_launch_count = 1U;
    batch.synchronization_count = 1U;
    batch.buffer_epoch = context.advance_epoch();
    if (batch.buffer_epoch != expected_epoch) {
      throw std::logic_error(
          "the Phase 14 facet top-k CUDA epoch advanced unexpectedly");
    }
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
