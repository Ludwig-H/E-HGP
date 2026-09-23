// MorseHGP3D v9 (23 septembre 2026) — CUDA run of the exact q3/q4 witness
// filter on a WSPD rectangle population (see filter_runner.hpp).

#include "filter_runner.hpp"

#include <cub/device/device_scan.cuh>
#include <cuda_runtime.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace mhgp9::gpu {
namespace {

struct CudaFailure {
  std::string what;
};

void check(cudaError_t code, const char* expression) {
  if (code != cudaSuccess) throw CudaFailure{std::string(expression) + ": " + cudaGetErrorString(code)};
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

    cudaEvent_t e[6];
    for (auto& event : e) MHGP9_CUDA(cudaEventCreate(&event));
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
    for (auto& event : e) cudaEventDestroy(event);
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
    MHGP9_CUDA(cudaFree(nullptr));
    int sms = 0;
    MHGP9_CUDA(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int threads = 128;
    const auto blocks_for = [&](unsigned long long items) {
      return static_cast<int>(std::max<unsigned long long>(1, std::min<unsigned long long>(
          (items + threads - 1) / threads, static_cast<unsigned long long>(sms) * 64)));
    };
    cudaEvent_t e[7];
    for (auto& event : e) MHGP9_CUDA(cudaEventCreate(&event));
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
    if (pairs > static_cast<unsigned long long>(0x7fffffff)) throw CudaFailure{"expanded pairs exceed the CUB int range"};
    std::size_t free_bytes = 0, total_bytes = 0;
    MHGP9_CUDA(cudaMemGetInfo(&free_bytes, &total_bytes));
    if (pairs * 10 > free_bytes / 2) throw CudaFailure{"pair buffers exceed half of the free device memory"};
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
    for (auto& event : e) cudaEventDestroy(event);
  } catch (const CudaFailure& failure) {
    out.error = failure.what;
  } catch (const std::exception& failure) {
    out.error = std::string("host: ") + failure.what();
  }
  return out;
}

}  // namespace mhgp9::gpu
