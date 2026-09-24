// MorseHGP3D v9 (23 septembre 2026) — CUDA run of the exact q3/q4 witness
// filter on a WSPD rectangle population (see filter_runner.hpp).

#include "filter_runner.hpp"

#include <cub/device/device_scan.cuh>
#include <cuda_runtime.h>

#include <algorithm>
#include <cstdio>
#include <new>
#include <stdexcept>
#include <string>

namespace mhgp9::gpu {
namespace {

struct CudaFailure {
  std::string what;
  bool capacity = false;  // a size/memory limit, not a fault of the pass
};

void check(cudaError_t code, const char* expression) {
  if (code != cudaSuccess)
    throw CudaFailure{std::string(expression) + ": " + cudaGetErrorString(code), code == cudaErrorMemoryAllocation};
}
#define MHGP9_CUDA(expression) check((expression), #expression)

template <class T>
class DeviceBuffer {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() {
    if (pointer_ != nullptr) cudaFree(pointer_);
  }
  void allocate(std::size_t count) {
    if (pointer_ != nullptr) cudaFree(pointer_);
    pointer_ = nullptr;
    count_ = count;
    if (count != 0) MHGP9_CUDA(cudaMalloc(reinterpret_cast<void**>(&pointer_), count * sizeof(T)));
  }
  T* get() const { return pointer_; }
  std::size_t size() const { return count_; }

 private:
  T* pointer_ = nullptr;
  std::size_t count_ = 0;
};

// Events destroyed on every path, only those actually created.
template <int N>
class EventSet {
 public:
  EventSet() = default;
  EventSet(const EventSet&) = delete;
  EventSet& operator=(const EventSet&) = delete;
  ~EventSet() {
    for (int i = 0; i < created_; ++i) cudaEventDestroy(events_[i]);
  }
  void create() {
    for (auto& event : events_) {
      MHGP9_CUDA(cudaEventCreate(&event));
      ++created_;
    }
  }
  cudaEvent_t operator[](int i) const { return events_[i]; }

 private:
  cudaEvent_t events_[N]{};
  int created_ = 0;
};

// One atomic per warp for the visit counters.
__device__ void add_visits(unsigned long long* total, unsigned long long value) {
  for (int offset = 16; offset > 0; offset /= 2) value += __shfl_down_sync(0xffffffffU, value, offset);
  if ((threadIdx.x & 31U) == 0 && value != 0) atomicAdd(total, value);
}

__global__ void rectangle_kernel(const FlatNode* nodes, const u32* rect_a, const u32* rect_b, const u8* rect_mask,
                                 std::size_t count, unsigned kmax, u8* out_mask, unsigned long long* out_mass,
                                 unsigned long long* visits, int* failure) {
  unsigned long long local = 0;
  const std::size_t stride = static_cast<std::size_t>(gridDim.x) * blockDim.x;
  // Every lane of a warp runs the same number of outer iterations, so the
  // warp reduction below sees all 32 lanes.
  const std::size_t rounded = (count + stride - 1) / stride * stride;
  for (std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < rounded; i += stride) {
    if (i >= count) continue;
    const FlatNode& a = nodes[rect_a[i]];
    const FlatNode& b = nodes[rect_b[i]];
    std::uint64_t v = 0;
    u8 mask = filter_boxes(nodes, a.box, b.box, kmax, rect_mask[i], v);
    local += v;
    if (mask == stack_failure) {
      atomicExch(failure, 1);
      mask = 0;
    }
    out_mask[i] = mask;
    out_mass[i] = mask != 0 ? static_cast<unsigned long long>(a.last - a.first) * (b.last - b.first) : 0ULL;
  }
  add_visits(visits, local);
}

__global__ void pair_kernel(const FlatNode* nodes, const std::int32_t* rank_points, const u32* rect_a,
                            const u32* rect_b, const u8* filtered, const unsigned long long* offsets,
                            std::size_t rect_count, unsigned long long pairs, unsigned kmax, u8* out_mask,
                            unsigned long long* visits, unsigned long long* lane_rejections, int* failure) {
  unsigned long long local = 0, q3 = 0, q4 = 0;
  const unsigned long long stride = static_cast<unsigned long long>(gridDim.x) * blockDim.x;
  const unsigned long long rounded = (pairs + stride - 1) / stride * stride;
  for (unsigned long long p = static_cast<unsigned long long>(blockIdx.x) * blockDim.x + threadIdx.x; p < rounded;
       p += stride) {
    if (p >= pairs) continue;
    // Last rectangle whose exclusive offset is <= p: it has positive mass.
    std::size_t low = 0, high = rect_count;
    while (high - low > 1) {
      const std::size_t middle = low + (high - low) / 2;
      if (offsets[middle] <= p) low = middle;
      else high = middle;
    }
    const FlatNode& a = nodes[rect_a[low]];
    const FlatNode& b = nodes[rect_b[low]];
    const unsigned long long local_index = p - offsets[low];
    const unsigned long long columns = b.last - b.first;
    const u32 ai = a.first + static_cast<u32>(local_index / columns);
    const u32 bi = b.first + static_cast<u32>(local_index % columns);
    FlatBox pa, pb;
    for (int axis = 0; axis < 3; ++axis) {
      pa.low[axis] = pa.high[axis] = rank_points[3 * static_cast<std::size_t>(ai) + axis];
      pb.low[axis] = pb.high[axis] = rank_points[3 * static_cast<std::size_t>(bi) + axis];
    }
    std::uint64_t v = 0;
    const u8 lanes = filtered[low];
    u8 mask = filter<true>(nodes, pa, pb, kmax, lanes, v);
    local += v;
    if (mask == stack_failure) {
      atomicExch(failure, 1);
      mask = 0;
    }
    q3 += (lanes & 2U) != 0 && (mask & 2U) == 0;
    q4 += (lanes & 4U) != 0 && (mask & 4U) == 0;
    out_mask[p] = mask;
  }
  add_visits(visits, local);
  add_visits(lane_rejections, q3);
  add_visits(lane_rejections + 1, q4);
}

__global__ void flag_kernel(const u8* masks, unsigned long long pairs, u32* flags) {
  const unsigned long long stride = static_cast<unsigned long long>(gridDim.x) * blockDim.x;
  for (unsigned long long p = static_cast<unsigned long long>(blockIdx.x) * blockDim.x + threadIdx.x; p < pairs;
       p += stride)
    flags[p] = masks[p] != 0 ? 1U : 0U;
}

// Surviving pair p goes to slot positions[p] (exclusive scan of the flags):
// rectangle order, row-major inside each rectangle, like the engine.
__global__ void scatter_kernel(const FlatNode* nodes, const u32* rect_a, const u32* rect_b,
                               const unsigned long long* offsets, std::size_t rect_count, const u8* masks,
                               unsigned long long pairs, const u32* positions, u32* out_a, u32* out_b, u8* out_mask) {
  const unsigned long long stride = static_cast<unsigned long long>(gridDim.x) * blockDim.x;
  for (unsigned long long p = static_cast<unsigned long long>(blockIdx.x) * blockDim.x + threadIdx.x; p < pairs;
       p += stride) {
    const u8 mask = masks[p];
    if (mask == 0) continue;
    std::size_t low = 0, high = rect_count;
    while (high - low > 1) {
      const std::size_t middle = low + (high - low) / 2;
      if (offsets[middle] <= p) low = middle;
      else high = middle;
    }
    const FlatNode& a = nodes[rect_a[low]];
    const FlatNode& b = nodes[rect_b[low]];
    const unsigned long long local_index = p - offsets[low];
    const unsigned long long columns = b.last - b.first;
    const u32 j = positions[p];
    out_a[j] = a.first + static_cast<u32>(local_index / columns);
    out_b[j] = b.first + static_cast<u32>(local_index % columns);
    out_mask[j] = mask;
  }
}

float elapsed(cudaEvent_t start, cudaEvent_t stop) {
  float ms = 0;
  MHGP9_CUDA(cudaEventElapsedTime(&ms, start, stop));
  return ms;
}

}  // namespace

FilterOutput run_filters(const FilterInput& input) {
  FilterOutput out;
  out.error = validate_filter_input(input);
  if (!out.error.empty()) return out;
  try {
    int devices = 0;
    if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) {
      out.error = "no CUDA device";
      return out;
    }
    MHGP9_CUDA(cudaSetDevice(0));
    cudaDeviceProp properties{};
    MHGP9_CUDA(cudaGetDeviceProperties(&properties, 0));
    out.device = properties.name;
    out.available = true;
    MHGP9_CUDA(cudaFree(nullptr));  // context creation, outside every timing

    DeviceBuffer<FlatNode> nodes;
    DeviceBuffer<std::int32_t> rank_points;
    DeviceBuffer<u32> rect_a, rect_b;
    DeviceBuffer<u8> rect_mask, filtered, pair_mask;
    DeviceBuffer<unsigned long long> mass, offsets, counters;
    DeviceBuffer<int> failure;
    DeviceBuffer<unsigned char> scan_storage;
    nodes.allocate(input.node_count);
    rank_points.allocate(3 * input.rank_count);
    rect_a.allocate(input.rect_count);
    rect_b.allocate(input.rect_count);
    rect_mask.allocate(input.rect_count);
    filtered.allocate(input.rect_count);
    mass.allocate(input.rect_count);
    offsets.allocate(input.rect_count);
    counters.allocate(4);
    failure.allocate(1);
    std::size_t scan_bytes = 0;
    MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(nullptr, scan_bytes, mass.get(), offsets.get(),
                                             static_cast<int>(input.rect_count)));
    scan_storage.allocate(std::max<std::size_t>(scan_bytes, 1));

    EventSet<6> e;
    e.create();
    const int threads = 128;
    int sms = 0;
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const unsigned repeats = std::max(1U, input.repeats);
    for (unsigned pass = 0; pass < repeats; ++pass) {
      MHGP9_CUDA(cudaEventRecord(e[0]));
      MHGP9_CUDA(cudaMemcpy(nodes.get(), input.nodes, input.node_count * sizeof(FlatNode), cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rank_points.get(), input.rank_points, 3 * input.rank_count * sizeof(std::int32_t),
                            cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rect_a.get(), input.rect_a, input.rect_count * sizeof(u32), cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rect_b.get(), input.rect_b, input.rect_count * sizeof(u32), cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rect_mask.get(), input.rect_mask, input.rect_count, cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemset(counters.get(), 0, 4 * sizeof(unsigned long long)));
      MHGP9_CUDA(cudaMemset(failure.get(), 0, sizeof(int)));
      MHGP9_CUDA(cudaEventRecord(e[1]));
      const int rect_blocks = static_cast<int>(std::min<std::size_t>((input.rect_count + threads - 1) / threads,
                                                                      static_cast<std::size_t>(sms) * 64));
      rectangle_kernel<<<std::max(rect_blocks, 1), threads>>>(nodes.get(), rect_a.get(), rect_b.get(),
          rect_mask.get(), input.rect_count, input.kmax, filtered.get(), mass.get(), counters.get(), failure.get());
      MHGP9_CUDA(cudaGetLastError());
      MHGP9_CUDA(cudaEventRecord(e[2]));
      MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(scan_storage.get(), scan_bytes, mass.get(), offsets.get(),
                                               static_cast<int>(input.rect_count)));
      unsigned long long tail[2] = {0, 0};
      if (input.rect_count != 0) {
        MHGP9_CUDA(cudaMemcpy(&tail[0], offsets.get() + input.rect_count - 1, sizeof(unsigned long long),
                              cudaMemcpyDeviceToHost));
        MHGP9_CUDA(cudaMemcpy(&tail[1], mass.get() + input.rect_count - 1, sizeof(unsigned long long),
                              cudaMemcpyDeviceToHost));
      }
      const unsigned long long pairs = tail[0] + tail[1];
      if (pair_mask.size() < pairs) {
        // One mask byte per expanded pair: refuse rather than exhaust the device.
        std::size_t free_bytes = 0, total_bytes = 0;
        MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
        if (pairs > free_bytes / 2) throw CudaFailure{"pair masks exceed half of the free device memory"};
        pair_mask.allocate(pairs);
      }
      MHGP9_CUDA(cudaEventRecord(e[3]));
      if (pairs != 0) {
        const int pair_blocks = static_cast<int>(std::min<unsigned long long>((pairs + threads - 1) / threads,
                                                                             static_cast<unsigned long long>(sms) * 64));
        pair_kernel<<<std::max(pair_blocks, 1), threads>>>(nodes.get(), rank_points.get(), rect_a.get(),
            rect_b.get(), filtered.get(), offsets.get(), input.rect_count, pairs, input.kmax, pair_mask.get(),
            counters.get() + 1, counters.get() + 2, failure.get());
        MHGP9_CUDA(cudaGetLastError());
      }
      MHGP9_CUDA(cudaEventRecord(e[4]));
      out.rect_masks.resize(input.rect_count);
      out.pair_masks.resize(pairs);
      MHGP9_CUDA(cudaMemcpy(out.rect_masks.data(), filtered.get(), input.rect_count, cudaMemcpyDeviceToHost));
      if (pairs != 0)
        MHGP9_CUDA(cudaMemcpy(out.pair_masks.data(), pair_mask.get(), pairs, cudaMemcpyDeviceToHost));
      unsigned long long visit_counts[2] = {0, 0};
      int failed = 0;
      MHGP9_CUDA(cudaMemcpy(visit_counts, counters.get(), sizeof(visit_counts), cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaMemcpy(&failed, failure.get(), sizeof(int), cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaEventRecord(e[5]));
      MHGP9_CUDA(cudaEventSynchronize(e[5]));
      const double upload = elapsed(e[0], e[1]), rect = elapsed(e[1], e[2]), scan = elapsed(e[2], e[3]),
                   pair = elapsed(e[3], e[4]), download = elapsed(e[4], e[5]), total = elapsed(e[0], e[5]);
      if (pass == 0) out.first_total_ms = total;
      if (pass == 0 || total < out.total_ms) {
        out.upload_ms = upload;
        out.rect_ms = rect;
        out.scan_ms = scan;
        out.pair_ms = pair;
        out.download_ms = download;
        out.total_ms = total;
      }
      out.pairs = pairs;
      out.rect_visits = visit_counts[0];
      out.pair_visits = visit_counts[1];
      out.stack_failure = out.stack_failure || failed != 0;
    }
  } catch (const CudaFailure& failure) {
    out.error = failure.what;
  } catch (const std::exception& failure) {  // host side of the pass (e.g. bad_alloc of the mask copies)
    out.error = std::string("host: ") + failure.what();
  }
  return out;
}

BatchOutput run_filter_batch(const FilterInput& input) {
  BatchOutput out;
  out.error = validate_filter_input(input);
  if (!out.error.empty()) {
    out.error_kind = BatchError::input_guard;
    return out;
  }
  try {
    int devices = 0;
    if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) {
      out.error = "no CUDA device";
      out.error_kind = BatchError::no_device;
      return out;
    }
    MHGP9_CUDA(cudaSetDevice(0));
    cudaDeviceProp properties{};
    MHGP9_CUDA(cudaGetDeviceProperties(&properties, 0));
    out.device = properties.name;
    out.available = true;
    MHGP9_CUDA(cudaFree(nullptr));
    int sms = 0;
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int threads = 128;
    const auto blocks_for = [&](unsigned long long items) {
      return static_cast<int>(std::max<unsigned long long>(1, std::min<unsigned long long>(
          (items + threads - 1) / threads, static_cast<unsigned long long>(sms) * 64)));
    };
    EventSet<7> e;
    e.create();
    DeviceBuffer<FlatNode> nodes;
    DeviceBuffer<std::int32_t> rank_points;
    DeviceBuffer<u32> rect_a, rect_b, flags, positions, out_a, out_b;
    DeviceBuffer<u8> rect_mask, filtered, pair_mask, out_mask;
    DeviceBuffer<unsigned long long> mass, offsets, counters;
    DeviceBuffer<int> failure;
    DeviceBuffer<unsigned char> scan_storage, flag_storage;
    MHGP9_CUDA(cudaEventRecord(e[0]));
    nodes.allocate(input.node_count);
    rank_points.allocate(3 * input.rank_count);
    rect_a.allocate(input.rect_count);
    rect_b.allocate(input.rect_count);
    rect_mask.allocate(input.rect_count);
    filtered.allocate(input.rect_count);
    mass.allocate(input.rect_count);
    offsets.allocate(input.rect_count);
    counters.allocate(4);
    failure.allocate(1);
    MHGP9_CUDA(cudaMemcpy(nodes.get(), input.nodes, input.node_count * sizeof(FlatNode), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(rank_points.get(), input.rank_points, 3 * input.rank_count * sizeof(std::int32_t),
                          cudaMemcpyHostToDevice));
    if (input.rect_count != 0) {
      MHGP9_CUDA(cudaMemcpy(rect_a.get(), input.rect_a, input.rect_count * sizeof(u32), cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rect_b.get(), input.rect_b, input.rect_count * sizeof(u32), cudaMemcpyHostToDevice));
      MHGP9_CUDA(cudaMemcpy(rect_mask.get(), input.rect_mask, input.rect_count, cudaMemcpyHostToDevice));
    }
    MHGP9_CUDA(cudaMemset(counters.get(), 0, 4 * sizeof(unsigned long long)));
    MHGP9_CUDA(cudaMemset(failure.get(), 0, sizeof(int)));
    MHGP9_CUDA(cudaEventRecord(e[1]));
    if (input.rect_count != 0) {
      rectangle_kernel<<<blocks_for(input.rect_count), threads>>>(nodes.get(), rect_a.get(), rect_b.get(),
          rect_mask.get(), input.rect_count, input.kmax, filtered.get(), mass.get(), counters.get(), failure.get());
      MHGP9_CUDA(cudaGetLastError());
    }
    MHGP9_CUDA(cudaEventRecord(e[2]));
    unsigned long long pairs = 0;
    if (input.rect_count != 0) {
      std::size_t scan_bytes = 0;
      MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(nullptr, scan_bytes, mass.get(), offsets.get(),
                                               static_cast<int>(input.rect_count)));
      scan_storage.allocate(std::max<std::size_t>(scan_bytes, 1));
      MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(scan_storage.get(), scan_bytes, mass.get(), offsets.get(),
                                               static_cast<int>(input.rect_count)));
      unsigned long long tail[2] = {0, 0};
      MHGP9_CUDA(cudaMemcpy(&tail[0], offsets.get() + input.rect_count - 1, sizeof(unsigned long long),
                            cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaMemcpy(&tail[1], mass.get() + input.rect_count - 1, sizeof(unsigned long long),
                            cudaMemcpyDeviceToHost));
      pairs = tail[0] + tail[1];
    }
    if (pairs > static_cast<unsigned long long>(0x7fffffff))
      throw CudaFailure{"expanded pairs exceed the CUB int range", true};
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    if (pairs * 10 > free_bytes / 2) throw CudaFailure{"pair buffers exceed half of the free device memory", true};
    MHGP9_CUDA(cudaEventRecord(e[3]));
    unsigned long long survivors = 0;
    if (pairs != 0) {
      pair_mask.allocate(pairs);
      pair_kernel<<<blocks_for(pairs), threads>>>(nodes.get(), rank_points.get(), rect_a.get(), rect_b.get(),
          filtered.get(), offsets.get(), input.rect_count, pairs, input.kmax, pair_mask.get(), counters.get() + 1,
          counters.get() + 2, failure.get());
      MHGP9_CUDA(cudaGetLastError());
    }
    MHGP9_CUDA(cudaEventRecord(e[4]));
    if (pairs != 0) {
      flags.allocate(pairs);
      positions.allocate(pairs);
      flag_kernel<<<blocks_for(pairs), threads>>>(pair_mask.get(), pairs, flags.get());
      MHGP9_CUDA(cudaGetLastError());
      std::size_t flag_bytes = 0;
      MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(nullptr, flag_bytes, flags.get(), positions.get(),
                                               static_cast<int>(pairs)));
      flag_storage.allocate(std::max<std::size_t>(flag_bytes, 1));
      MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(flag_storage.get(), flag_bytes, flags.get(), positions.get(),
                                               static_cast<int>(pairs)));
      u32 tail[2] = {0, 0};
      MHGP9_CUDA(cudaMemcpy(&tail[0], positions.get() + pairs - 1, sizeof(u32), cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaMemcpy(&tail[1], flags.get() + pairs - 1, sizeof(u32), cudaMemcpyDeviceToHost));
      survivors = static_cast<unsigned long long>(tail[0]) + tail[1];
      out_a.allocate(survivors);
      out_b.allocate(survivors);
      out_mask.allocate(survivors);
      if (survivors != 0) {
        scatter_kernel<<<blocks_for(pairs), threads>>>(nodes.get(), rect_a.get(), rect_b.get(), offsets.get(),
            input.rect_count, pair_mask.get(), pairs, positions.get(), out_a.get(), out_b.get(), out_mask.get());
        MHGP9_CUDA(cudaGetLastError());
      }
    }
    MHGP9_CUDA(cudaEventRecord(e[5]));
    out.rect_masks.resize(input.rect_count);
    out.survivor_a.resize(survivors);
    out.survivor_b.resize(survivors);
    out.survivor_mask.resize(survivors);
    if (input.rect_count != 0)
      MHGP9_CUDA(cudaMemcpy(out.rect_masks.data(), filtered.get(), input.rect_count, cudaMemcpyDeviceToHost));
    if (survivors != 0) {
      MHGP9_CUDA(cudaMemcpy(out.survivor_a.data(), out_a.get(), survivors * sizeof(u32), cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaMemcpy(out.survivor_b.data(), out_b.get(), survivors * sizeof(u32), cudaMemcpyDeviceToHost));
      MHGP9_CUDA(cudaMemcpy(out.survivor_mask.data(), out_mask.get(), survivors, cudaMemcpyDeviceToHost));
    }
    unsigned long long counts[4] = {0, 0, 0, 0};
    int failed = 0;
    MHGP9_CUDA(cudaMemcpy(counts, counters.get(), sizeof(counts), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(&failed, failure.get(), sizeof(int), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaEventRecord(e[6]));
    MHGP9_CUDA(cudaEventSynchronize(e[6]));
    out.upload_ms = elapsed(e[0], e[1]);
    out.rect_ms = elapsed(e[1], e[2]);
    out.scan_ms = elapsed(e[2], e[3]);
    out.pair_ms = elapsed(e[3], e[4]);
    out.select_ms = elapsed(e[4], e[5]);
    out.download_ms = elapsed(e[5], e[6]);
    out.total_ms = elapsed(e[0], e[6]);
    out.pairs = pairs;
    out.rect_visits = counts[0];
    out.pair_visits = counts[1];
    out.pair_q3_rejected = counts[2];
    out.pair_q4_rejected = counts[3];
    out.stack_failure = failed != 0;
  } catch (const CudaFailure& failure) {
    out.error = failure.what;
    out.error_kind = failure.capacity ? BatchError::capacity : BatchError::device_fault;
  } catch (const std::bad_alloc& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::length_error& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::exception& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::device_fault;
  }
  return out;
}

namespace {

constexpr int certificate_threads = 128;
constexpr int certificate_warps_per_block = certificate_threads / 32;

// A CUDA warp as the 32-lane group of gpu/certificate.hpp. Every lane runs
// the same uniform control flow; the host versions are never called.
struct WarpGroup {
  static constexpr u32 size = 32;
  u32 lane;
  template <class Code>
  __host__ __device__ void ballot2(u32 base, u32 count, Code code, u32& first, u32& second) const {
#if defined(__CUDA_ARCH__)
    const u32 i = base + lane;
    const u32 c = i < count ? code(i) : 0U;
    first = __ballot_sync(0xffffffffU, (c & 1U) != 0);
    second = __ballot_sync(0xffffffffU, (c & 2U) != 0);
#else
    (void)base; (void)count; (void)code;
    first = second = 0;
#endif
  }
  template <class F>
  __host__ __device__ void for_set(u32 mask, F f) const {
    if (((mask >> lane) & 1U) != 0) f(lane, popcount32(mask & ((1U << lane) - 1U)));
  }
  template <class F>
  __host__ __device__ void for_each(u32 count, F f) const {
    for (u32 i = lane; i < count; i += 32) f(i);
  }
  template <class H>
  __host__ __device__ void fingerprint(u32 mask, H h, u64& sum, u64& x) const {
#if defined(__CUDA_ARCH__)
    const u64 v = ((mask >> lane) & 1U) != 0 ? h(lane) : 0ULL;
    u64 s = v, y = v;
    for (int offset = 16; offset > 0; offset >>= 1) {
      s += __shfl_xor_sync(0xffffffffU, s, offset);
      y ^= __shfl_xor_sync(0xffffffffU, y, offset);
    }
    sum += s;
    x ^= y;
#else
    (void)mask; (void)h; (void)sum; (void)x;
#endif
  }
  template <class Code>
  __host__ __device__ u32 reduce_min(u32 base, u32 count, Code code) const {
#if defined(__CUDA_ARCH__)
    const u32 i = base + lane;
    return __reduce_min_sync(0xffffffffU, i < count ? code(i) : 0xffffffffU);
#else
    (void)base; (void)count; (void)code;
    return 0xffffffffU;
#endif
  }
  template <class Code>
  __host__ __device__ void vote(u32 base, u32 count, Code code, u32& w0, u32& w1, u32& first, u32& second) const {
#if defined(__CUDA_ARCH__)
    const u32 i = base + lane;
    u32 a = 0, b = 0;
    bool f = false, s = false;
    if (i < count) {
      const auto v = code(i);
      a = v.w0;
      b = v.w1;
      f = v.first;
      s = v.second;
    }
    w0 = __reduce_add_sync(0xffffffffU, a);
    w1 = __reduce_add_sync(0xffffffffU, b);
    first = __ballot_sync(0xffffffffU, f);
    second = __ballot_sync(0xffffffffU, s);
#else
    (void)base; (void)count; (void)code;
    w0 = w1 = first = second = 0;
#endif
  }
  __host__ __device__ bool leader() const { return lane == 0; }
  __host__ __device__ void sync() const {
#if defined(__CUDA_ARCH__)
    __syncwarp();
#endif
  }
};

// 128 registers (ptxas, sm_120, CUDA 12.9: no spill) so that four blocks, 16
// warps, are resident per SM; the occupancy query sizes the grid from it.
// Persistent warps: each takes the next edge from a global counter, runs it
// in its own slab, and keeps its work; lane 0 writes the per-edge answer and,
// at the end, the warp's work (summed on the host, in integers).
__global__ void __launch_bounds__(certificate_threads, 4) certificate_kernel(CertificateIndex index, const u32* edge_a, const u32* edge_b,
                                   const u8* edge_mask, u32 edges, unsigned kmax, bool dead_core, u32 capacity,
                                   u32* ranges, i64* form_constant, i64* form_x, i64* form_y, u32* frontiers,
                                   unsigned long long* next_edge, u8* out_mask, u8* out_status,
                                   CertificateWork* warp_work, u32 warps) {
  const u32 warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
  // The warp's totals live in shared memory, written by its lane 0 only:
  // two per-lane u64 ledgers would take most of the register file.
  __shared__ CertificateWork totals[certificate_warps_per_block];
  CertificateWork& work = totals[threadIdx.x / 32];
  if (warp >= warps) return;  // whole warps only: blockDim is a multiple of 32
  const WarpGroup group{threadIdx.x & 31U};
  if (group.leader()) work = CertificateWork{};
  group.sync();
  const std::size_t c = capacity;
  const CertificateSlab slab{ranges + 2 * c * warp, form_constant + c * warp, form_x + c * warp,
                             form_y + c * warp, frontiers + prover_levels * c * warp, capacity};
  for (;;) {
    unsigned long long edge = 0;
    if (group.leader()) edge = atomicAdd(next_edge, 1ULL);
    edge = __shfl_sync(0xffffffffU, edge, 0);
    if (edge >= edges) break;
    const auto result = certify_edge(group, index, edge_a[edge], edge_b[edge], edge_mask[edge], kmax, dead_core,
                                     slab, work);
    if (group.leader()) {
      out_mask[edge] = result.mask;
      out_status[edge] = static_cast<u8>(result.status);
    }
    group.sync();
  }
  if (group.leader()) warp_work[warp] = work;
}

}  // namespace

CertificateOutput run_certificate_batch(const CertificateInput& input) {
  CertificateOutput out;
  out.error = validate_certificate_input(input);
  if (!out.error.empty()) {
    out.error_kind = BatchError::input_guard;
    return out;
  }
  try {
    int devices = 0;
    if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) {
      out.error = "no CUDA device";
      out.error_kind = BatchError::no_device;
      return out;
    }
    MHGP9_CUDA(cudaSetDevice(0));
    cudaDeviceProp properties{};
    MHGP9_CUDA(cudaGetDeviceProperties(&properties, 0));
    out.device = properties.name;
    out.available = true;
    MHGP9_CUDA(cudaFree(nullptr));
    const std::size_t edges = input.edge_count;
    if (edges == 0) return out;  // no kernel: warps stays 0 (the edge arrays may be null)
    out.masks.assign(input.edge_mask, input.edge_mask + edges);
    out.status.assign(edges, 0);
    const u32 capacity = input.capacity == 0 ? default_certificate_capacity : input.capacity;
    // Slab bytes per warp: ranges, three forms, the frontiers of each scanned depth.
    const std::size_t slab_bytes = static_cast<std::size_t>(capacity) *
        (2 * sizeof(u32) + 3 * sizeof(i64) + prover_levels * sizeof(u32));
    int sms = 0;
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    // The warps that can be resident at once (the kernel's register budget
    // decides, measured by the occupancy query): a slab for a warp that could
    // only start after every edge is claimed would be wasted memory. At most
    // a quarter of the free memory goes to slabs.
    const int threads = certificate_threads;
    int blocks_per_sm = 0;
    MHGP9_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks_per_sm, certificate_kernel, threads, 0));
    if (blocks_per_sm <= 0) throw CudaFailure{"certificate kernel cannot be resident on this device", true};
    const std::size_t by_memory = free_bytes / 4 / slab_bytes;
    const std::size_t wanted = std::min<std::size_t>(
        static_cast<std::size_t>(sms) * static_cast<std::size_t>(blocks_per_sm) * (threads / 32), edges);
    const std::size_t warps = std::min(wanted, by_memory);
    if (warps == 0) throw CudaFailure{"no certificate slab fits in a quarter of the free device memory", true};
    out.capacity = capacity;
    out.warps = static_cast<u32>(warps);
    const std::size_t cap = capacity;
    EventSet<4> e;
    e.create();
    DeviceBuffer<FlatNode> nodes;
    DeviceBuffer<u32> escapes, edge_a, edge_b, ranges, frontiers;
    DeviceBuffer<std::int32_t> rank_points;
    DeviceBuffer<u8> edge_mask, out_mask, out_status;
    DeviceBuffer<i64> form_constant, form_x, form_y;
    DeviceBuffer<unsigned long long> next_edge;
    DeviceBuffer<CertificateWork> warp_work;
    MHGP9_CUDA(cudaEventRecord(e[0]));
    nodes.allocate(input.index.node_count);
    escapes.allocate(input.index.node_count);
    rank_points.allocate(3 * input.index.rank_count);
    edge_a.allocate(edges);
    edge_b.allocate(edges);
    edge_mask.allocate(edges);
    out_mask.allocate(edges);
    out_status.allocate(edges);
    ranges.allocate(2 * cap * warps);
    form_constant.allocate(cap * warps);
    form_x.allocate(cap * warps);
    form_y.allocate(cap * warps);
    frontiers.allocate(prover_levels * cap * warps);
    next_edge.allocate(1);
    warp_work.allocate(warps);
    MHGP9_CUDA(cudaMemcpy(nodes.get(), input.index.nodes, input.index.node_count * sizeof(FlatNode),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(escapes.get(), input.escapes, input.index.node_count * sizeof(u32),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(rank_points.get(), input.index.rank_points,
                          3 * input.index.rank_count * sizeof(std::int32_t), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(edge_a.get(), input.edge_a, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(edge_b.get(), input.edge_b, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(edge_mask.get(), input.edge_mask, edges, cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemset(next_edge.get(), 0, sizeof(unsigned long long)));
    MHGP9_CUDA(cudaMemset(warp_work.get(), 0, warps * sizeof(CertificateWork)));
    MHGP9_CUDA(cudaEventRecord(e[1]));
    const CertificateIndex index{nodes.get(), escapes.get(), static_cast<u32>(input.index.node_count),
                                 rank_points.get()};
    const int blocks = static_cast<int>((warps * 32 + threads - 1) / threads);
    certificate_kernel<<<blocks, threads>>>(index, edge_a.get(), edge_b.get(), edge_mask.get(),
        static_cast<u32>(edges), input.index.kmax, input.dead_core, capacity, ranges.get(), form_constant.get(),
        form_x.get(), form_y.get(), frontiers.get(), next_edge.get(), out_mask.get(), out_status.get(),
        warp_work.get(), static_cast<u32>(warps));
    MHGP9_CUDA(cudaGetLastError());
    MHGP9_CUDA(cudaEventRecord(e[2]));
    std::vector<CertificateWork> works(warps);
    MHGP9_CUDA(cudaMemcpy(out.masks.data(), out_mask.get(), edges, cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.status.data(), out_status.get(), edges, cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(works.data(), warp_work.get(), warps * sizeof(CertificateWork), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaEventRecord(e[3]));
    MHGP9_CUDA(cudaEventSynchronize(e[3]));
    for (const auto& w : works) add_certificate(out.work, w);
    for (std::size_t i = 0; i < edges; ++i) {
      if (out.status[i] == static_cast<u8>(CertificateStatus::deferred)) {
        ++out.deferred;
        out.masks[i] = input.edge_mask[i];
      } else if (out.status[i] != static_cast<u8>(CertificateStatus::decided)) {
        ++out.faults;
      }
    }
    out.upload_ms = elapsed(e[0], e[1]);
    out.kernel_ms = elapsed(e[1], e[2]);
    out.download_ms = elapsed(e[2], e[3]);
    out.total_ms = elapsed(e[0], e[3]);
  } catch (const CudaFailure& failure) {
    out.error = failure.what;
    out.error_kind = failure.capacity ? BatchError::capacity : BatchError::device_fault;
  } catch (const std::bad_alloc& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::length_error& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::exception& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::device_fault;
  }
  return out;
}

namespace {

constexpr int lanes_threads = 128;
constexpr int lanes_warps_per_block = lanes_threads / 32;

// Persistent warps as the certificate kernel: each takes the next edge, runs
// its q3 lane in its own slab, then (decided) reserves its records in the
// arena with one atomic and copies them; an edge whose reservation passes the
// arena end is deferred (the counter advances anyway: every later
// reservation fails too). Lane 0 writes the per-edge answer and the warp's
// work (shared memory).
__global__ void __launch_bounds__(lanes_threads, 4) lanes_kernel(LanesIndex index, const u32* edge_a,
    const u32* edge_b, const u8* edge_lanes_in, u32 edges, unsigned kmax, u32 capacity, u32 record_capacity,
    u32 event_capacity, u32* ranges, std::int32_t* points, u32* ranks, u32* seeds, u32* scratch,
    LaneRecord* slab_records, u32* events, LaneRecord* arena, unsigned long long arena_capacity,
    unsigned long long* next_edge, unsigned long long* reserved, u8* out_status, u32* out_begin, u32* out_count,
    Q3Work* warp_work, Q4Work* warp_work4, u32 warps) {
  const u32 warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
  __shared__ Q3Work totals[lanes_warps_per_block];
  __shared__ Q4Work totals4[lanes_warps_per_block];
  __shared__ Q4Work edge4[lanes_warps_per_block];  // the current edge's q4 work (leader lane)
  Q3Work& work = totals[threadIdx.x / 32];
  Q4Work& work4 = totals4[threadIdx.x / 32];
  Q4Work& local4 = edge4[threadIdx.x / 32];
  if (warp >= warps) return;
  const WarpGroup group{threadIdx.x & 31U};
  if (group.leader()) {
    work = Q3Work{};
    work4 = Q4Work{};
  }
  group.sync();
  const std::size_t c = capacity;
  const LanesSlab slab{ranges + 2 * c * warp, points + 3 * c * warp, ranks + c * warp, seeds + c * warp,
                       scratch + c * warp, slab_records + static_cast<std::size_t>(record_capacity) * warp, capacity,
                       record_capacity};
  const std::size_t e = event_capacity;
  const Q4Slab q4{events + 3 * e * warp, events + 3 * e * warp + e, events + 3 * e * warp + 2 * e, event_capacity};
  for (;;) {
    unsigned long long edge = 0;
    if (group.leader()) edge = atomicAdd(next_edge, 1ULL);
    edge = __shfl_sync(0xffffffffU, edge, 0);
    if (edge >= edges) break;
    u32 count = 0;
    EdgeQ3Work local;
    const u8 lanes = edge_lanes_in == nullptr ? u8{2} : edge_lanes_in[edge];
    auto status = edge_lanes(group, index, edge_a[edge], edge_b[edge], lanes, kmax, slab, q4, count, local, local4);
    unsigned long long begin = 0;
    if (status == CertificateStatus::decided) {
      if (group.leader()) begin = atomicAdd(reserved, static_cast<unsigned long long>(count));
      begin = __shfl_sync(0xffffffffU, begin, 0);
      if (begin + count > arena_capacity) {
        status = CertificateStatus::deferred;
      } else {
        group.for_each(count, [&](u32 r) {
          arena[begin + r] = slab.records[r];
          arena[begin + r].edge = static_cast<u32>(edge);
        });
      }
    }
    if (group.leader()) {
      out_status[edge] = static_cast<u8>(status);
      out_begin[edge] = static_cast<u32>(status == CertificateStatus::decided ? begin : 0);
      out_count[edge] = status == CertificateStatus::decided ? count : 0;
      if (status == CertificateStatus::decided) {
        add_q3_edge(work, local);
        add_q4(work4, local4);
      }
    }
    group.sync();
  }
  if (group.leader()) {
    warp_work[warp] = work;
    warp_work4[warp] = work4;
  }
}

}  // namespace

LanesOutput run_lanes_batch(const LanesInput& input) {
  LanesOutput out;
  out.error = validate_lanes_input(input);
  if (!out.error.empty()) {
    out.error_kind = BatchError::input_guard;
    return out;
  }
  try {
    int devices = 0;
    if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) {
      out.error = "no CUDA device";
      out.error_kind = BatchError::no_device;
      return out;
    }
    MHGP9_CUDA(cudaSetDevice(0));
    cudaDeviceProp properties{};
    MHGP9_CUDA(cudaGetDeviceProperties(&properties, 0));
    out.device = properties.name;
    out.available = true;
    MHGP9_CUDA(cudaFree(nullptr));
    const std::size_t edges = input.edge_count;
    if (edges == 0) return out;  // no kernel: warps stays 0 (the edge arrays may be null)
    const u32 capacity = input.capacity == 0 ? default_lanes_capacity : input.capacity;
    const u32 record_capacity = input.record_capacity == 0 ? default_record_capacity : input.record_capacity;
    const u32 event_capacity = input.event_capacity == 0 ? default_event_capacity : input.event_capacity;
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    // The default arena is clamped to an eighth of the free memory (review of
    // 23 September, night): at scale an overflow of the ALLOCATED arena defers
    // edges to the CPU tail. A failed allocation of the buffers themselves is
    // still a capacity refusal (auditor, AUDIT_S4A_VALIDATION_ET_ARENE). An
    // explicit arena is taken as given.
    const std::size_t arena_capacity = input.arena_capacity != 0
        ? input.arena_capacity
        : std::max<std::size_t>(1, std::min(default_arena_capacity(edges), free_bytes / 8 / sizeof(LaneRecord)));
    out.status.assign(edges, 0);
    out.record_begin.assign(edges, 0);
    out.record_count.assign(edges, 0);
    const std::size_t slab_bytes = static_cast<std::size_t>(capacity) *
        (2 * sizeof(u32) + 3 * sizeof(std::int32_t) + 3 * sizeof(u32)) +
        static_cast<std::size_t>(record_capacity) * sizeof(LaneRecord) +
        3 * static_cast<std::size_t>(event_capacity) * sizeof(u32);
    int sms = 0;
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int threads = lanes_threads;
    int blocks_per_sm = 0;
    MHGP9_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks_per_sm, lanes_kernel, threads, 0));
    if (blocks_per_sm <= 0) throw CudaFailure{"lanes kernel cannot be resident on this device", true};
    const std::size_t arena_bytes = arena_capacity * sizeof(LaneRecord);
    if (arena_bytes > free_bytes / 4) throw CudaFailure{"record arena exceeds a quarter of the free device memory", true};
    const std::size_t by_memory = (free_bytes / 4) / slab_bytes;
    const std::size_t wanted = std::min<std::size_t>(
        static_cast<std::size_t>(sms) * static_cast<std::size_t>(blocks_per_sm) * (threads / 32), edges);
    const std::size_t warps = std::min(wanted, by_memory);
    if (warps == 0) throw CudaFailure{"no lanes slab fits in a quarter of the free device memory", true};
    out.capacity = capacity;
    out.record_capacity = record_capacity;
    out.warps = static_cast<u32>(warps);
    const std::size_t cap = capacity;
    EventSet<4> e;
    e.create();
    DeviceBuffer<FlatNode> nodes;
    DeviceBuffer<u32> escapes, rank_ids, edge_a, edge_b, ranges, ranks, seeds, scratch, events, out_begin, out_count;
    DeviceBuffer<u8> lanes_in;
    DeviceBuffer<Q4Work> warp_work4;
    DeviceBuffer<std::int32_t> rank_points, points;
    DeviceBuffer<u8> out_status;
    DeviceBuffer<LaneRecord> slab_records, arena;
    DeviceBuffer<unsigned long long> counters;
    DeviceBuffer<Q3Work> warp_work;
    MHGP9_CUDA(cudaEventRecord(e[0]));
    nodes.allocate(input.index.node_count);
    escapes.allocate(input.index.node_count);
    rank_points.allocate(3 * input.index.rank_count);
    rank_ids.allocate(input.index.rank_count);
    edge_a.allocate(edges);
    edge_b.allocate(edges);
    out_status.allocate(edges);
    out_begin.allocate(edges);
    out_count.allocate(edges);
    ranges.allocate(2 * cap * warps);
    points.allocate(3 * cap * warps);
    ranks.allocate(cap * warps);
    seeds.allocate(cap * warps);
    scratch.allocate(cap * warps);
    events.allocate(3 * static_cast<std::size_t>(event_capacity) * warps);
    warp_work4.allocate(warps);
    if (input.edge_lanes != nullptr) lanes_in.allocate(edges);
    slab_records.allocate(static_cast<std::size_t>(record_capacity) * warps);
    arena.allocate(arena_capacity);
    counters.allocate(2);
    warp_work.allocate(warps);
    MHGP9_CUDA(cudaMemcpy(nodes.get(), input.index.nodes, input.index.node_count * sizeof(FlatNode),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(escapes.get(), input.escapes, input.index.node_count * sizeof(u32),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(rank_points.get(), input.index.rank_points,
                          3 * input.index.rank_count * sizeof(std::int32_t), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(rank_ids.get(), input.rank_ids, input.index.rank_count * sizeof(u32),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(edge_a.get(), input.edge_a, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(edge_b.get(), input.edge_b, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemset(counters.get(), 0, 2 * sizeof(unsigned long long)));
    MHGP9_CUDA(cudaMemset(warp_work.get(), 0, warps * sizeof(Q3Work)));
    MHGP9_CUDA(cudaMemset(warp_work4.get(), 0, warps * sizeof(Q4Work)));
    if (input.edge_lanes != nullptr)
      MHGP9_CUDA(cudaMemcpy(lanes_in.get(), input.edge_lanes, edges, cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaEventRecord(e[1]));
    const LanesIndex index{CertificateIndex{nodes.get(), escapes.get(), static_cast<u32>(input.index.node_count),
                                            rank_points.get()},
                           rank_ids.get()};
    const int blocks = static_cast<int>((warps * 32 + threads - 1) / threads);
    lanes_kernel<<<blocks, threads>>>(index, edge_a.get(), edge_b.get(),
        input.edge_lanes != nullptr ? lanes_in.get() : nullptr, static_cast<u32>(edges), input.index.kmax, capacity,
        record_capacity, event_capacity, ranges.get(), points.get(), ranks.get(), seeds.get(), scratch.get(),
        slab_records.get(), events.get(), arena.get(), arena_capacity, counters.get(), counters.get() + 1,
        out_status.get(), out_begin.get(), out_count.get(), warp_work.get(), warp_work4.get(),
        static_cast<u32>(warps));
    MHGP9_CUDA(cudaGetLastError());
    MHGP9_CUDA(cudaEventRecord(e[2]));
    std::vector<Q3Work> works(warps);
    std::vector<Q4Work> works4(warps);
    unsigned long long counts[2] = {0, 0};
    MHGP9_CUDA(cudaMemcpy(counts, counters.get(), sizeof(counts), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.status.data(), out_status.get(), edges, cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.record_begin.data(), out_begin.get(), edges * sizeof(u32), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.record_count.data(), out_count.get(), edges * sizeof(u32), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(works.data(), warp_work.get(), warps * sizeof(Q3Work), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(works4.data(), warp_work4.get(), warps * sizeof(Q4Work), cudaMemcpyDeviceToHost));
    const std::size_t used = static_cast<std::size_t>(std::min<unsigned long long>(counts[1], arena_capacity));
    std::vector<LaneRecord> reserved(used);
    if (used != 0)
      MHGP9_CUDA(cudaMemcpy(reserved.data(), arena.get(), used * sizeof(LaneRecord), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaEventRecord(e[3]));
    MHGP9_CUDA(cudaEventSynchronize(e[3]));
    for (const auto& w : works) add_q3(out.work, w);
    for (const auto& w : works4) add_q4(out.work4, w);
    for (std::size_t i = 0; i < edges; ++i) {
      if (out.status[i] == static_cast<u8>(CertificateStatus::deferred)) ++out.deferred;
      else if (out.status[i] != static_cast<u8>(CertificateStatus::decided)) ++out.faults;
    }
    if (counts[1] <= arena_capacity) {
      out.records = std::move(reserved);  // every reservation was decided: the slices partition it
    } else {
      // A deferred reservation left a hole: keep the decided slices only.
      for (std::size_t i = 0; i < edges; ++i) {
        if (out.status[i] != static_cast<u8>(CertificateStatus::decided)) continue;
        const std::size_t begin = out.record_begin[i];
        out.record_begin[i] = static_cast<u32>(out.records.size());
        out.records.insert(out.records.end(), reserved.begin() + static_cast<std::ptrdiff_t>(begin),
                           reserved.begin() + static_cast<std::ptrdiff_t>(begin + out.record_count[i]));
      }
    }
    out.upload_ms = elapsed(e[0], e[1]);
    out.kernel_ms = elapsed(e[1], e[2]);
    out.download_ms = elapsed(e[2], e[3]);
    out.total_ms = elapsed(e[0], e[3]);
  } catch (const CudaFailure& failure) {
    out.error = failure.what;
    out.error_kind = failure.capacity ? BatchError::capacity : BatchError::device_fault;
  } catch (const std::bad_alloc& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::length_error& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::capacity;
  } catch (const std::exception& failure) {
    out.error = std::string("host: ") + failure.what();
    out.error_kind = BatchError::device_fault;
  }
  return out;
}

std::string warm_up() {
  int devices = 0;
  if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) return "no CUDA device";
  if (cudaSetDevice(0) != cudaSuccess) return "cudaSetDevice failed";
  if (cudaFree(nullptr) != cudaSuccess) return "context creation failed";
  return {};
}

}  // namespace mhgp9::gpu
