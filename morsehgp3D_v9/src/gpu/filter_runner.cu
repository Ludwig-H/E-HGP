// MorseHGP3D v9 (23 septembre 2026) — CUDA run of the exact q3/q4 witness
// filter on a WSPD rectangle population (see filter_runner.hpp).

#include "filter_runner.hpp"

#include <cub/device/device_scan.cuh>
#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <mutex>
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
    count_ = capacity_ = 0;
    if (count != 0) MHGP9_CUDA(cudaMalloc(reinterpret_cast<void**>(&pointer_), count * sizeof(T)));
    count_ = capacity_ = count;
  }
  // Grow-only (resident buffers, v9 H1): keeps the allocation when it holds
  // `count` elements, reallocates otherwise.
  void reserve(std::size_t count) {
    if (count <= capacity_) {
      count_ = count;
      return;
    }
    allocate(count);
  }
  T* get() const { return pointer_; }
  std::size_t size() const { return count_; }
  std::size_t capacity_bytes() const { return capacity_ * sizeof(T); }

 private:
  T* pointer_ = nullptr;
  std::size_t count_ = 0, capacity_ = 0;
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
  template <unsigned N, class Code>
  __host__ __device__ void ballot_bits(u32 base, u32 count, Code code, u32 (&planes)[N]) const {
#if defined(__CUDA_ARCH__)
    const u32 i = base + lane;
    const u32 c = i < count ? code(i) : 0U;
    for (unsigned k = 0; k < N; ++k) planes[k] = __ballot_sync(0xffffffffU, ((c >> k) & 1U) != 0);
#else
    (void)base; (void)count; (void)code;
    for (unsigned k = 0; k < N; ++k) planes[k] = 0;
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

// ---- S4b tasks (24 septembre 2026, lanes plan step 2): the call in three
// steps of gpu/lanes_tasks.hpp, on one stream, with two host reads of
// counters (after P: the cover arena and the task count; after T: the
// staging arena). Persistent warps take the next edge or task from a global
// counter; the output never depends on which warp ran what, nor when.

// P: each warp takes the next edge, builds its cover in its walk slab,
// reserves its sites in the cover arena (one atomic, counter always
// advanced), writes the scan order and seeds there, then the edge's plan,
// prologue ledger and empty task slot. A cover beyond the arena leaves the
// plan without tasks: the host refuses the call before T.
__global__ void __launch_bounds__(lanes_threads, 4) lanes_plan_kernel(LanesIndex index, const u32* edge_a,
    const u32* edge_b, const u8* edge_lanes_in, u32 edges, unsigned kmax, u32 capacity, u64 budget, u32* ranges,
    u32* scratch, std::int32_t* cover_points, u32* cover_ranks, u32* cover_seeds, unsigned long long cover_capacity,
    unsigned long long* next_edge, unsigned long long* cover_reserved, LanesPlan* plans, EdgeQ3Work* plan_work,
    LanesTaskWork* slots, unsigned long long* task_counts, u32 warps) {
  const u32 warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
  if (warp >= warps) return;
  const WarpGroup group{threadIdx.x & 31U};
  const std::size_t c = capacity;
  const LanesSlab walk{ranges + 2 * c * warp, nullptr, nullptr, nullptr, scratch + c * warp, nullptr, capacity, 0};
  static_assert(sizeof(LanesTaskWork) % sizeof(u64) == 0, "the task slot is a whole number of u64 words");
  constexpr u32 slot_words = sizeof(LanesTaskWork) / sizeof(u64);
  for (;;) {
    unsigned long long edge = 0;
    if (group.leader()) edge = atomicAdd(next_edge, 1ULL);
    edge = __shfl_sync(0xffffffffU, edge, 0);
    if (edge >= edges) break;
    const u8 lanes = edge_lanes_in == nullptr ? u8{2} : edge_lanes_in[edge];
    EdgeQ3Work local;
    u32 range_count = 0, sites = 0;
    const auto status = lanes_plan_cover(group, index, edge_a[edge], edge_b[edge], lanes, kmax, walk, range_count,
                                         sites, local);
    LanesPlan plan{};
    plan.lanes = lanes;
    plan.status = static_cast<u8>(status);
    if (status == CertificateStatus::decided) {
      unsigned long long offset = 0;
      if (group.leader()) offset = atomicAdd(cover_reserved, static_cast<unsigned long long>(sites));
      offset = __shfl_sync(0xffffffffU, offset, 0);
      if (offset + sites <= cover_capacity) {
        const LanesSlab placed{walk.ranges, cover_points + 3 * offset, cover_ranks + offset, cover_seeds + offset,
                               walk.scratch, nullptr, capacity, 0};
        plan.seeds = lanes_plan_order(group, index, edge_a[edge], edge_b[edge], lanes, placed, range_count, sites,
                                      local);
        plan.offset = offset;
        plan.sites = sites;
        plan.tasks = lanes_task_count(sites, plan.seeds, budget);
      }
    }
    u64* words = reinterpret_cast<u64*>(slots + edge);
    group.for_each(slot_words, [&](u32 i) { words[i] = 0; });
    if (group.leader()) {
      plans[edge] = plan;
      plan_work[edge] = local;
      task_counts[edge] = plan.tasks;
    }
    group.sync();
  }
}

// The task table: (edge, first seed, last seed) of every task, in (edge,
// range) order, at the edge's exclusive prefix of the task counts.
__global__ void lanes_fill_kernel(const LanesPlan* plans, const unsigned long long* task_first, u32 edges,
                                  u64 budget, LanesTask* tasks) {
  const std::size_t stride = static_cast<std::size_t>(gridDim.x) * blockDim.x;
  for (std::size_t e = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x; e < edges; e += stride) {
    const LanesPlan plan = plans[e];
    const u64 per = lanes_seeds_per_task(plan.sites, budget);
    const unsigned long long first = task_first[e];
    for (u32 k = 0; k < plan.tasks; ++k) {
      LanesTask task{};
      task.edge = static_cast<u32>(e);
      task.first = static_cast<u32>(k * per);
      const u64 last = k * per + per;
      task.last = static_cast<u32>(last < plan.seeds ? last : plan.seeds);
      tasks[first + k] = task;
    }
  }
}

// T: each warp takes the next task, runs its seeds in its record slab, then
// (no failure) reserves its records in the staging arena with one atomic
// (counter always advanced; beyond the arena the host refuses the call) and
// copies them. Lane 0 writes the task back and adds its work to the edge's
// slot (integer sums and maxima: order-free).
__global__ void __launch_bounds__(lanes_threads, 4) lanes_task_kernel(LanesIndex index, const u32* edge_a,
    const u32* edge_b, unsigned kmax, u32 capacity, u32 record_capacity, u32 event_capacity, const LanesPlan* plans,
    std::int32_t* cover_points, u32* cover_ranks, u32* cover_seeds, LanesTask* tasks,
    unsigned long long task_count, LaneRecord* slab_records, u32* events, LaneRecord* staging,
    unsigned long long staging_capacity, unsigned long long* next_task, unsigned long long* staged,
    unsigned long long* max_steps, LanesTaskWork* slots, u32 warps) {
  const u32 warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
  __shared__ Q4Work task4[lanes_warps_per_block];  // the current task's q4 work (leader lane)
  Q4Work& w4 = task4[threadIdx.x / 32];
  if (warp >= warps) return;
  const WarpGroup group{threadIdx.x & 31U};
  const std::size_t e = event_capacity;
  const Q4Slab q4{events + 3 * e * warp, events + 3 * e * warp + e, events + 3 * e * warp + 2 * e, event_capacity};
  LaneRecord* records = slab_records + static_cast<std::size_t>(record_capacity) * warp;
  for (;;) {
    unsigned long long t = 0;
    if (group.leader()) t = atomicAdd(next_task, 1ULL);
    t = __shfl_sync(0xffffffffU, t, 0);
    if (t >= task_count) break;
    LanesTask task = tasks[t];
    const LanesPlan plan = plans[task.edge];
    const LanesSlab slab{nullptr, cover_points + 3 * plan.offset, cover_ranks + plan.offset,
                         cover_seeds + plan.offset, nullptr, records, capacity, record_capacity};
    const u64 steps = lanes_task(group, index, edge_a[task.edge], edge_b[task.edge], plan.lanes, kmax, slab,
                                 plan.sites, task.first, task.last, q4, task, w4, slots[task.edge]);
    unsigned long long begin = 0;
    const u32 n = task.q3 + task.q4;
    if (task.fail_phase == 0 && n != 0) {
      if (group.leader()) begin = atomicAdd(staged, static_cast<unsigned long long>(n));
      begin = __shfl_sync(0xffffffffU, begin, 0);
      if (begin + n <= staging_capacity) {
        // Word by word (16 u64 a record): coalesced, and no record held in registers.
        static_assert(sizeof(LaneRecord) % sizeof(u64) == 0, "a record is a whole number of u64 words");
        constexpr u32 words = sizeof(LaneRecord) / sizeof(u64);
        const u64* from = reinterpret_cast<const u64*>(records);
        u64* to = reinterpret_cast<u64*>(staging + begin);
        group.for_each(n * words, [&](u32 i) { to[i] = from[i]; });
      }
    }
    if (group.leader()) {
      task.begin = begin;
      tasks[t] = task;
      atomicMax(max_steps, static_cast<unsigned long long>(steps));
    }
    group.sync();
  }
}

// C, first part: the replay of each edge's tasks (lanes_replay), before the
// arena rule; the counts of the edges still decided feed the scan.
__global__ void lanes_replay_kernel(const LanesPlan* plans, const unsigned long long* task_first,
                                    const LanesTask* tasks, u32 edges, u32 record_capacity, u8* pre_status,
                                    unsigned long long* counts) {
  const std::size_t stride = static_cast<std::size_t>(gridDim.x) * blockDim.x;
  for (std::size_t e = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x; e < edges; e += stride) {
    const LanesPlan plan = plans[e];
    auto status = static_cast<CertificateStatus>(plan.status);
    u32 count = 0;
    if (status == CertificateStatus::decided)
      status = lanes_replay(tasks + task_first[e], plan.tasks, plan.lanes, record_capacity, count);
    pre_status[e] = static_cast<u8>(status);
    counts[e] = status == CertificateStatus::decided ? count : 0;
  }
}

// C, second part: one warp per edge. The arena rule in edge order (the
// exclusive prefix of the counts: the counter always advanced), the answer,
// the records of a decided edge gathered at its prefix in (phase, task)
// order, and its ledger (lane 0, shared memory, then per warp).
__global__ void __launch_bounds__(lanes_threads) lanes_gather_kernel(const LanesPlan* plans,
    const EdgeQ3Work* plan_work, const LanesTaskWork* slots, const unsigned long long* task_first,
    const LanesTask* tasks, const u8* pre_status, const unsigned long long* counts,
    const unsigned long long* prefix, u32 edges, unsigned long long arena_capacity, const LaneRecord* staging,
    LaneRecord* arena, u8* out_status, u32* out_begin, u32* out_count, unsigned long long* decided_records,
    Q3Work* warp_work, Q4Work* warp_work4, u32 warps) {
  const u32 warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
  __shared__ Q3Work totals[lanes_warps_per_block];
  __shared__ Q4Work totals4[lanes_warps_per_block];
  Q3Work& work = totals[threadIdx.x / 32];
  Q4Work& work4 = totals4[threadIdx.x / 32];
  if (warp >= warps) return;
  const WarpGroup group{threadIdx.x & 31U};
  if (group.leader()) {
    work = Q3Work{};
    work4 = Q4Work{};
  }
  group.sync();
  for (std::size_t e = warp; e < edges; e += warps) {
    auto status = static_cast<CertificateStatus>(pre_status[e]);
    const unsigned long long begin = prefix[e], count = counts[e];
    if (status == CertificateStatus::decided && begin + count > arena_capacity) status = CertificateStatus::deferred;
    const bool decided = status == CertificateStatus::decided;
    const LanesPlan plan = plans[e];
    if (decided) {
      const LanesTask* own = tasks + task_first[e];
      LaneRecord* to = arena + begin;
      for (u32 phase = 2; phase <= 4; phase += 2)
        for (u32 t = 0; t < plan.tasks; ++t) {
          const u32 n = phase == 2 ? own[t].q3 : own[t].q4;
          const LaneRecord* from = staging + own[t].begin + (phase == 2 ? 0U : own[t].q3);
          group.for_each(n, [&](u32 r) {
            LaneRecord record = from[r];
            record.edge = static_cast<u32>(e);
            to[r] = record;
          });
          to += n;
        }
    }
    if (group.leader()) {
      out_status[e] = static_cast<u8>(status);
      out_begin[e] = decided ? static_cast<u32>(begin) : 0U;
      out_count[e] = decided ? static_cast<u32>(count) : 0U;
      if (decided) {
        EdgeQ3Work e3;
        Q4Work e4;
        lanes_edge_work(plan_work[e], slots[e], plan.lanes, e3, e4);
        add_q3_edge(work, e3);
        add_q4(work4, e4);
        if (count != 0) atomicAdd(decided_records, count);
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

namespace {

// v9 H1 (24 septembre 2026): the lanes call's device buffers are RESIDENT,
// grow-only and reused (no cudaMalloc/cudaFree of gigabytes per call);
// warm_up_lanes reserves the per-warp slabs and the cover arena during q2.
// The device properties and the occupancies are probed once. Calls are
// serialised by the mutex. The structure is leaked on purpose: no cudaFree
// after the runtime's teardown. Sizing adds the resident bytes to the free
// memory, or reads the total memory, so the decisions (warps, arenas,
// refusals) do not depend on the history of calls.
//
// S4b tasks: per-warp slabs are the walk (P: ranges and scratch, 12 bytes a
// site) and the records and events (T); the cover arena holds 20 bytes a
// site (coordinates 12, rank 4, seed position 4: the portable functions read
// slab.points as before), by default a sixth of the device's TOTAL memory
// (97 887 MiB on G4: about 855 M sites, 2.25 times the largest call measured,
// 08/000200 K10 with 379 M). Per edge about 470 bytes (plan, prologue
// ledger, task slot, scans, answer), per task 32 bytes.
struct LanesResident {
  std::mutex mu;
  bool probed = false;
  std::string name;
  int sms = 0, plan_blocks = 0, task_blocks = 0, gather_blocks = 0;
  std::size_t total_memory = 0;
  DeviceBuffer<FlatNode> nodes;
  DeviceBuffer<u32> escapes, rank_ids, edge_a, edge_b, ranges, scratch, events, out_begin, out_count;
  DeviceBuffer<u32> cover_ranks, cover_seeds;
  DeviceBuffer<u8> lanes_in, out_status, pre_status;
  DeviceBuffer<Q4Work> warp_work4;
  DeviceBuffer<std::int32_t> rank_points, cover_points;
  DeviceBuffer<LaneRecord> slab_records, arena, staging;
  DeviceBuffer<unsigned long long> counters, task_counts, task_first, record_counts, record_prefix;
  DeviceBuffer<Q3Work> warp_work;
  DeviceBuffer<LanesPlan> plans;
  DeviceBuffer<EdgeQ3Work> plan_work;
  DeviceBuffer<LanesTaskWork> slots;
  DeviceBuffer<LanesTask> tasks;
  DeviceBuffer<unsigned char> scan_storage;
  std::size_t device_bytes() const {
    return nodes.capacity_bytes() + escapes.capacity_bytes() + rank_ids.capacity_bytes() + edge_a.capacity_bytes() +
           edge_b.capacity_bytes() + ranges.capacity_bytes() + scratch.capacity_bytes() + events.capacity_bytes() +
           out_begin.capacity_bytes() + out_count.capacity_bytes() + cover_ranks.capacity_bytes() +
           cover_seeds.capacity_bytes() + lanes_in.capacity_bytes() + out_status.capacity_bytes() +
           pre_status.capacity_bytes() + warp_work4.capacity_bytes() + rank_points.capacity_bytes() +
           cover_points.capacity_bytes() + slab_records.capacity_bytes() + arena.capacity_bytes() +
           staging.capacity_bytes() + counters.capacity_bytes() + task_counts.capacity_bytes() +
           task_first.capacity_bytes() + record_counts.capacity_bytes() + record_prefix.capacity_bytes() +
           warp_work.capacity_bytes() + plans.capacity_bytes() + plan_work.capacity_bytes() +
           slots.capacity_bytes() + tasks.capacity_bytes() + scan_storage.capacity_bytes();
  }
  // Device name, SM count, total memory and occupancies, once (the caller
  // holds mu).
  void probe() {
    if (probed) return;
    cudaDeviceProp properties{};
    MHGP9_CUDA(cudaGetDeviceProperties(&properties, 0));
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    MHGP9_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&plan_blocks, lanes_plan_kernel, lanes_threads, 0));
    MHGP9_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&task_blocks, lanes_task_kernel, lanes_threads, 0));
    MHGP9_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&gather_blocks, lanes_gather_kernel, lanes_threads, 0));
    if (plan_blocks <= 0 || task_blocks <= 0 || gather_blocks <= 0)
      throw CudaFailure{"lanes kernels cannot be resident on this device", true};
    std::size_t free_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_memory));
    name = properties.name;
    probed = true;
  }
  std::size_t max_warps(int blocks_per_sm) const {
    return static_cast<std::size_t>(sms) * static_cast<std::size_t>(blocks_per_sm) * (lanes_threads / 32);
  }
  // Bytes of one warp's slabs: the walk (P) and the records and events (T).
  static std::size_t walk_bytes(u32 capacity) { return static_cast<std::size_t>(capacity) * 3 * sizeof(u32); }
  static std::size_t task_slab_bytes(u32 record_capacity, u32 event_capacity) {
    return static_cast<std::size_t>(record_capacity) * sizeof(LaneRecord) +
           3 * static_cast<std::size_t>(event_capacity) * sizeof(u32);
  }
  static constexpr std::size_t cover_site_bytes = 3 * sizeof(std::int32_t) + 2 * sizeof(u32);
  std::size_t default_cover_capacity() const { return total_memory / 6 / cover_site_bytes; }
  std::size_t task_capacity() const { return std::min<std::size_t>(total_memory / 16 / sizeof(LanesTask), 0xffffffffULL); }
  // Warps of P and T that the memory allows (an eighth of the free memory
  // each), capped by residency.
  std::size_t plan_warps(std::size_t free_bytes, u32 capacity) const {
    return std::min(max_warps(plan_blocks), (free_bytes / 8) / walk_bytes(capacity));
  }
  std::size_t task_warps(std::size_t free_bytes, u32 record_capacity, u32 event_capacity) const {
    return std::min(max_warps(task_blocks), (free_bytes / 8) / task_slab_bytes(record_capacity, event_capacity));
  }
  // Slabs for `pw` P warps and `tw` T warps, the cover arena (grow-only).
  void reserve_slabs(std::size_t pw, std::size_t tw, u32 capacity, u32 record_capacity, u32 event_capacity,
                     std::size_t cover_sites) {
    const std::size_t cap = capacity;
    ranges.reserve(2 * cap * pw);
    scratch.reserve(cap * pw);
    events.reserve(3 * static_cast<std::size_t>(event_capacity) * tw);
    slab_records.reserve(static_cast<std::size_t>(record_capacity) * tw);
    cover_points.reserve(3 * cover_sites);
    cover_ranks.reserve(cover_sites);
    cover_seeds.reserve(cover_sites);
    counters.reserve(8);
  }
};
LanesResident& lanes_resident() {
  static LanesResident* resident = new LanesResident;  // leaked on purpose (see above)
  return *resident;
}

double host_ms_since(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// cub::DeviceScan::ExclusiveSum of `count` u64 into the resident storage.
void exclusive_scan(LanesResident& res, const unsigned long long* in, unsigned long long* out, std::size_t count) {
  if (count == 0) return;
  if (count > static_cast<std::size_t>(0x7fffffff)) throw CudaFailure{"lanes scan exceeds the CUB int range", true};
  std::size_t bytes = 0;
  MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(nullptr, bytes, in, out, static_cast<int>(count)));
  res.scan_storage.reserve(std::max<std::size_t>(bytes, 1));
  MHGP9_CUDA(cub::DeviceScan::ExclusiveSum(res.scan_storage.get(), bytes, in, out, static_cast<int>(count)));
}

// The total of an exclusive scan: last prefix plus last item.
unsigned long long scan_total(const unsigned long long* in, const unsigned long long* out, std::size_t count) {
  if (count == 0) return 0;
  unsigned long long tail[2] = {0, 0};
  MHGP9_CUDA(cudaMemcpy(&tail[0], out + count - 1, sizeof(unsigned long long), cudaMemcpyDeviceToHost));
  MHGP9_CUDA(cudaMemcpy(&tail[1], in + count - 1, sizeof(unsigned long long), cudaMemcpyDeviceToHost));
  return tail[0] + tail[1];
}

// A refused or failed call publishes nothing but its device and its error.
LanesOutput refused(const LanesOutput& from, std::string error, BatchError kind) {
  LanesOutput out;
  out.available = from.available;
  out.device = from.device;
  out.error = std::move(error);
  out.error_kind = kind;
  return out;
}

}  // namespace

LanesOutput run_lanes_batch(const LanesInput& input) {
  const auto entry = std::chrono::steady_clock::now();
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
    auto& res = lanes_resident();
    std::lock_guard<std::mutex> lock(res.mu);
    res.probe();
    out.device = res.name;
    out.available = true;
    MHGP9_CUDA(cudaFree(nullptr));
    const std::size_t edges = input.edge_count;
    if (edges == 0) return out;  // no kernel: warps stays 0 (the edge arrays may be null)
    const u32 capacity = input.capacity == 0 ? default_lanes_capacity : input.capacity;
    const u32 record_capacity = input.record_capacity == 0 ? default_record_capacity : input.record_capacity;
    const u32 event_capacity = input.event_capacity == 0 ? default_event_capacity : input.event_capacity;
    const u64 budget = input.task_budget == 0 ? default_task_budget : input.task_budget;
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    free_bytes += res.device_bytes();  // resident buffers count as free: history-independent sizing
    // The default staging arena is clamped to an eighth of the free memory
    // and the final arena to half of that (review of 23 September, night;
    // review before R18: staging stays twice the arena when the clamps bind).
    // An overflow of the final arena defers edges to the CPU tail; an
    // overflow of the staging arena defers every non-faulty edge (below):
    // neither refuses the call. A failed allocation of the buffers
    // themselves is still a capacity refusal (auditor,
    // AUDIT_S4A_VALIDATION_ET_ARENE). An explicit capacity is taken as given.
    const std::size_t clamp = std::max<std::size_t>(1, free_bytes / 8 / sizeof(LaneRecord));
    const std::size_t arena_capacity = input.arena_capacity != 0
        ? input.arena_capacity
        : std::max<std::size_t>(1, std::min(default_arena_capacity(edges), clamp / 2));
    const std::size_t staging_capacity = input.staging_capacity != 0
        ? static_cast<std::size_t>(input.staging_capacity)
        : std::min(default_staging_capacity(edges), clamp);
    const std::size_t cover_capacity = input.cover_capacity != 0 ? static_cast<std::size_t>(input.cover_capacity)
                                                                  : res.default_cover_capacity();
    out.status.assign(edges, 0);
    out.record_begin.assign(edges, 0);
    out.record_count.assign(edges, 0);
    if (arena_capacity * sizeof(LaneRecord) > free_bytes / 4)
      throw CudaFailure{"record arena exceeds a quarter of the free device memory", true};
    if (staging_capacity * sizeof(LaneRecord) > free_bytes / 4)
      throw CudaFailure{"staging arena exceeds a quarter of the free device memory", true};
    if (cover_capacity * LanesResident::cover_site_bytes > free_bytes / 2)
      throw CudaFailure{"cover arena exceeds half of the free device memory", true};
    const std::size_t plan_warps = std::min(res.plan_warps(free_bytes, capacity), edges);
    const std::size_t task_warps_max = res.task_warps(free_bytes, record_capacity, event_capacity);
    const std::size_t gather_warps = std::min(res.max_warps(res.gather_blocks), edges);
    if (plan_warps == 0 || task_warps_max == 0)
      throw CudaFailure{"no lanes slab fits in an eighth of the free device memory", true};
    out.capacity = capacity;
    out.record_capacity = record_capacity;
    EventSet<6> e;
    e.create();
    out.setup_ms = host_ms_since(entry);
    MHGP9_CUDA(cudaEventRecord(e[0]));
    res.nodes.reserve(input.index.node_count);
    res.escapes.reserve(input.index.node_count);
    res.rank_points.reserve(3 * input.index.rank_count);
    res.rank_ids.reserve(input.index.rank_count);
    res.edge_a.reserve(edges);
    res.edge_b.reserve(edges);
    res.out_status.reserve(edges);
    res.out_begin.reserve(edges);
    res.out_count.reserve(edges);
    res.pre_status.reserve(edges);
    res.plans.reserve(edges);
    res.plan_work.reserve(edges);
    res.slots.reserve(edges);
    res.task_counts.reserve(edges);
    res.task_first.reserve(edges);
    res.record_counts.reserve(edges);
    res.record_prefix.reserve(edges);
    res.reserve_slabs(plan_warps, task_warps_max, capacity, record_capacity, event_capacity, cover_capacity);
    if (input.edge_lanes != nullptr) res.lanes_in.reserve(edges);
    res.arena.reserve(arena_capacity);
    res.staging.reserve(staging_capacity);
    res.warp_work.reserve(gather_warps);
    res.warp_work4.reserve(gather_warps);
    MHGP9_CUDA(cudaMemcpy(res.nodes.get(), input.index.nodes, input.index.node_count * sizeof(FlatNode),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(res.escapes.get(), input.escapes, input.index.node_count * sizeof(u32),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(res.rank_points.get(), input.index.rank_points,
                          3 * input.index.rank_count * sizeof(std::int32_t), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(res.rank_ids.get(), input.rank_ids, input.index.rank_count * sizeof(u32),
                          cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(res.edge_a.get(), input.edge_a, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemcpy(res.edge_b.get(), input.edge_b, edges * sizeof(u32), cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaMemset(res.counters.get(), 0, 8 * sizeof(unsigned long long)));
    if (input.edge_lanes != nullptr)
      MHGP9_CUDA(cudaMemcpy(res.lanes_in.get(), input.edge_lanes, edges, cudaMemcpyHostToDevice));
    MHGP9_CUDA(cudaEventRecord(e[1]));
    const LanesIndex index{CertificateIndex{res.nodes.get(), res.escapes.get(),
                                            static_cast<u32>(input.index.node_count), res.rank_points.get()},
                           res.rank_ids.get()};
    const int threads = lanes_threads;
    unsigned long long* counter = res.counters.get();  // 0 next edge, 1 cover, 2 next task, 3 staged, 4 decided, 5 max steps
    // ---- P, then the task counts' scan.
    lanes_plan_kernel<<<static_cast<int>((plan_warps * 32 + threads - 1) / threads), threads>>>(index,
        res.edge_a.get(), res.edge_b.get(), input.edge_lanes != nullptr ? res.lanes_in.get() : nullptr,
        static_cast<u32>(edges), input.index.kmax, capacity, budget, res.ranges.get(), res.scratch.get(),
        res.cover_points.get(), res.cover_ranks.get(), res.cover_seeds.get(), cover_capacity, counter, counter + 1,
        res.plans.get(), res.plan_work.get(), res.slots.get(), res.task_counts.get(), static_cast<u32>(plan_warps));
    MHGP9_CUDA(cudaGetLastError());
    exclusive_scan(res, res.task_counts.get(), res.task_first.get(), edges);
    MHGP9_CUDA(cudaEventRecord(e[2]));
    unsigned long long cover_used = 0;
    MHGP9_CUDA(cudaMemcpy(&cover_used, counter + 1, sizeof(cover_used), cudaMemcpyDeviceToHost));
    const unsigned long long task_count = scan_total(res.task_counts.get(), res.task_first.get(), edges);
    if (cover_used > cover_capacity) throw CudaFailure{"lanes cover arena exceeded", true};
    if (task_count > res.task_capacity()) throw CudaFailure{"lanes task table exceeds its capacity", true};
    // ---- The task table, then T.
    res.tasks.reserve(std::max<std::size_t>(task_count, 1));
    const std::size_t task_warps = std::min<std::size_t>(task_warps_max, std::max<unsigned long long>(task_count, 1));
    const int fill_blocks = static_cast<int>(std::min<std::size_t>((edges + threads - 1) / threads,
                                                                   static_cast<std::size_t>(res.sms) * 64));
    if (task_count != 0) {
      lanes_fill_kernel<<<fill_blocks, threads>>>(res.plans.get(), res.task_first.get(), static_cast<u32>(edges),
                                                  budget, res.tasks.get());
      MHGP9_CUDA(cudaGetLastError());
      lanes_task_kernel<<<static_cast<int>((task_warps * 32 + threads - 1) / threads), threads>>>(index,
          res.edge_a.get(), res.edge_b.get(), input.index.kmax, capacity, record_capacity, event_capacity,
          res.plans.get(), res.cover_points.get(), res.cover_ranks.get(), res.cover_seeds.get(), res.tasks.get(),
          task_count, res.slab_records.get(), res.events.get(), res.staging.get(), staging_capacity, counter + 2,
          counter + 3, counter + 5, res.slots.get(), static_cast<u32>(task_warps));
      MHGP9_CUDA(cudaGetLastError());
    }
    MHGP9_CUDA(cudaEventRecord(e[3]));
    unsigned long long staged = 0;
    MHGP9_CUDA(cudaMemcpy(&staged, counter + 3, sizeof(staged), cudaMemcpyDeviceToHost));
    // A staging overflow never refuses the call (review before R18: an arena
    // overflow defers, never refuses): every edge but a faulty one is
    // deferred to the CPU tail, with no record and no ledger. It depends on
    // the input only (the staged total), the same on the host twin.
    const bool overflow = staged > staging_capacity;
    // ---- C: replay, the counts' scan (arena rule), gather and ledger.
    lanes_replay_kernel<<<fill_blocks, threads>>>(res.plans.get(), res.task_first.get(), res.tasks.get(),
        static_cast<u32>(edges), record_capacity, res.pre_status.get(), res.record_counts.get());
    MHGP9_CUDA(cudaGetLastError());
    if (!overflow) {
      exclusive_scan(res, res.record_counts.get(), res.record_prefix.get(), edges);
      lanes_gather_kernel<<<static_cast<int>((gather_warps * 32 + threads - 1) / threads), threads>>>(
          res.plans.get(), res.plan_work.get(), res.slots.get(), res.task_first.get(), res.tasks.get(),
          res.pre_status.get(), res.record_counts.get(), res.record_prefix.get(), static_cast<u32>(edges),
          arena_capacity, res.staging.get(), res.arena.get(), res.out_status.get(), res.out_begin.get(),
          res.out_count.get(), counter + 4, res.warp_work.get(), res.warp_work4.get(),
          static_cast<u32>(gather_warps));
      MHGP9_CUDA(cudaGetLastError());
    }
    MHGP9_CUDA(cudaEventRecord(e[4]));
    std::vector<Q3Work> works(overflow ? 0 : gather_warps);
    std::vector<Q4Work> works4(overflow ? 0 : gather_warps);
    unsigned long long counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    MHGP9_CUDA(cudaMemcpy(counts, counter, sizeof(counts), cudaMemcpyDeviceToHost));
    if (overflow) {
      MHGP9_CUDA(cudaMemcpy(out.status.data(), res.pre_status.get(), edges, cudaMemcpyDeviceToHost));
      for (auto& status : out.status)
        if (status != static_cast<u8>(CertificateStatus::fault)) status = static_cast<u8>(CertificateStatus::deferred);
      counts[4] = 0;
    } else {
    MHGP9_CUDA(cudaMemcpy(out.status.data(), res.out_status.get(), edges, cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.record_begin.data(), res.out_begin.get(), edges * sizeof(u32),
                          cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(out.record_count.data(), res.out_count.get(), edges * sizeof(u32),
                          cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaMemcpy(works.data(), res.warp_work.get(), gather_warps * sizeof(Q3Work), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(
        cudaMemcpy(works4.data(), res.warp_work4.get(), gather_warps * sizeof(Q4Work), cudaMemcpyDeviceToHost));
    }
    const std::size_t used = static_cast<std::size_t>(counts[4]);
    if (used > arena_capacity) throw CudaFailure{"lanes decided records exceed the arena", false};
    out.records.resize(used);  // written whole by the copy below (no zero fill)
    if (used != 0)
      MHGP9_CUDA(cudaMemcpy(out.records.data(), res.arena.get(), used * sizeof(LaneRecord), cudaMemcpyDeviceToHost));
    MHGP9_CUDA(cudaEventRecord(e[5]));
    MHGP9_CUDA(cudaEventSynchronize(e[5]));
    const auto finish = std::chrono::steady_clock::now();
    for (const auto& w : works) add_q3(out.work, w);
    for (const auto& w : works4) add_q4(out.work4, w);
    for (std::size_t i = 0; i < edges; ++i) {
      if (out.status[i] == static_cast<u8>(CertificateStatus::deferred)) ++out.deferred;
      else if (out.status[i] != static_cast<u8>(CertificateStatus::decided)) ++out.faults;
    }
    out.warps = static_cast<u32>(task_warps);
    out.tasks = task_count;
    out.max_task_steps = counts[5];
    out.upload_ms = elapsed(e[0], e[1]);
    out.plan_ms = elapsed(e[1], e[2]);
    out.task_ms = elapsed(e[2], e[3]);
    out.compact_ms = elapsed(e[3], e[4]);
    out.kernel_ms = elapsed(e[1], e[4]);
    out.download_ms = elapsed(e[4], e[5]);
    out.total_ms = elapsed(e[0], e[5]);
    out.finish_ms = host_ms_since(finish);
  } catch (const CudaFailure& failure) {
    return refused(out, failure.what, failure.capacity ? BatchError::capacity : BatchError::device_fault);
  } catch (const std::bad_alloc& failure) {
    return refused(out, std::string("host: ") + failure.what(), BatchError::capacity);
  } catch (const std::length_error& failure) {
    return refused(out, std::string("host: ") + failure.what(), BatchError::capacity);
  } catch (const std::exception& failure) {
    return refused(out, std::string("host: ") + failure.what(), BatchError::device_fault);
  }
  return out;
}

std::string warm_up_lanes(u32 capacity, u32 record_capacity, u32 event_capacity) {
  try {
    MHGP9_CUDA(cudaSetDevice(0));
    auto& res = lanes_resident();
    std::lock_guard<std::mutex> lock(res.mu);
    res.probe();
    const u32 cap = capacity == 0 ? default_lanes_capacity : capacity;
    const u32 rec = record_capacity == 0 ? default_record_capacity : record_capacity;
    const u32 evt = event_capacity == 0 ? default_event_capacity : event_capacity;
    // Sized as the call (resident bytes counted as free; the default cover
    // arena from the total memory): the call never needs more than this.
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    free_bytes += res.device_bytes();
    const std::size_t pw = res.plan_warps(free_bytes, cap), tw = res.task_warps(free_bytes, rec, evt);
    if (pw != 0 && tw != 0) res.reserve_slabs(pw, tw, cap, rec, evt, res.default_cover_capacity());
    return {};
  } catch (const CudaFailure& failure) {
    return failure.what;  // the batch call classifies any error again
  }
}

std::string warm_up() {
  int devices = 0;
  if (cudaGetDeviceCount(&devices) != cudaSuccess || devices == 0) return "no CUDA device";
  if (cudaSetDevice(0) != cudaSuccess) return "cudaSetDevice failed";
  if (cudaFree(nullptr) != cudaSuccess) return "context creation failed";
  return {};
}

}  // namespace mhgp9::gpu
