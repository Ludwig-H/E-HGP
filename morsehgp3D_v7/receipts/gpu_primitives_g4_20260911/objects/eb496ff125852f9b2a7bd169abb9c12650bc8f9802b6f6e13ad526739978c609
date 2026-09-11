// MorseHGP3D v7 — reusable synchronous CUDA prefilter/census route.
// Geometry only: no F/FULL reduction, catalogue authority or GPU latency claim.
// The context owns an immutable device index snapshot and one device lot.
// Candidates must describe that snapshot. run() deliberately takes no second
// CloudIndex. All caller outputs are cleared on entry and published together
// only after every lot is validated. Errors invalidate the context.
#pragma once

#if defined(__CUDACC__) || defined(MHGP7_FAKE_DEVICE)

#if defined(__CUDACC__) && !defined(MHGP7_FAKE_DEVICE)
#include <cuda_runtime.h>
#endif

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../pipeline/expand.hpp"
#include "census_kernels.cuh"
#include "pack.hpp"

namespace mhgp7::gpu {

inline constexpr const char* kCensusRouteVersion = "cuda_census_route_sync_v1";

// Cumulative physical accounting, including completed operations before a
// refusal. Byte fields count successful transfers only. Timings are synchronous
// host wall times, not CUDA event durations. Stub timings are not device times.
struct CensusRouteCosts {
  double index_wire_ms = 0, pack_ms = 0, host_setup_ms = 0, setup_alloc_ms = 0;
  double h2d_ms = 0, kernel_ms = 0, d2h_ms = 0, rebuild_ms = 0, release_ms = 0;
  u64 h2d_index_bytes = 0, h2d_ball_bytes = 0, h2d_sentinel_bytes = 0, d2h_bytes = 0;
  u64 allocated_bytes = 0, freed_bytes = 0, allocations = 0, failed_cuda_frees = 0;
  u64 failed_cuda_free_bytes = 0;
  u64 device_resident_bytes = 0, peak_device_bytes = 0;
  u64 candidates = 0, lots_launched = 0, lots_reconstructed = 0;
};

namespace census_route_detail {

struct Timer {
  double& output;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  ~Timer() {
    output += std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - begin).count();
  }
};

struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

inline void checked(cudaError_t status, const char* operation) {
  if (status != cudaSuccess)
    throw Error(std::string("cuda census route : ") + operation + " : " + cudaGetErrorString(status));
}

template <class T>
struct DeviceBuffer {
  T* data = nullptr;
  size_t bytes = 0;
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { if (data != nullptr) (void)cudaFree(data); }

  void allocate(size_t count_bytes, CensusRouteCosts& costs) {
    if (count_bytes == 0) return;
    Timer timer{costs.setup_alloc_ms};
    checked(cudaMalloc(reinterpret_cast<void**>(&data), count_bytes), "cudaMalloc");
    bytes = count_bytes;
    ++costs.allocations;
    costs.allocated_bytes += bytes;
    costs.device_resident_bytes += bytes;
    costs.peak_device_bytes = std::max(costs.peak_device_bytes, costs.device_resident_bytes);
  }

  void release(CensusRouteCosts& costs) noexcept {
    if (data == nullptr) return;
    Timer timer{costs.release_ms};
    const cudaError_t status = cudaFree(data);
    // A failing free is reported, never promoted to certified release.
    if (status == cudaSuccess) {
      costs.freed_bytes += bytes;
    } else {
      ++costs.failed_cuda_frees;
      costs.failed_cuda_free_bytes += bytes;
    }
    costs.device_resident_bytes -= bytes;
    data = nullptr;
    bytes = 0;
  }
};

inline void transfer(void* dst, const void* src, size_t bytes, cudaMemcpyKind kind,
                     double& elapsed, u64& transferred) {
  if (bytes == 0) return;
  Timer timer{elapsed};
  checked(cudaMemcpy(dst, src, bytes, kind), "cudaMemcpy");
  transferred += bytes;
}

struct State {
  DeviceBuffer<i32> left, right, first, last, ids;
  DeviceBuffer<u16> boxes, positions;
  DeviceBuffer<u32> weights, candidate_indices;
  DeviceBuffer<u64> ball_words, count;
  DeviceBuffer<u8> pre_status, census_status, n_interior, n_shell;
  std::vector<u8> input;
  std::vector<u64> out_count;
  std::vector<u8> out_pre_status, out_census_status, out_interior, out_shell;
  std::vector<i32> out_ids;
  std::vector<u32> out_candidates;
  std::string index_digest;
  size_t lot = 0;
  u32 n_positions = 0;
  i32 root = 0;
  u64 total_mass = 0;

  void release(CensusRouteCosts& costs) noexcept {
    left.release(costs); right.release(costs); first.release(costs); last.release(costs);
    ids.release(costs); boxes.release(costs); positions.release(costs); weights.release(costs);
    candidate_indices.release(costs); ball_words.release(costs); count.release(costs);
    pre_status.release(costs); census_status.release(costs);
    n_interior.release(costs); n_shell.release(costs);
  }

  void sentinels(size_t n) {
    std::fill_n(out_count.begin(), n, ~u64{0});
    std::fill_n(out_pre_status.begin(), n, kSentinelStatus);
    std::fill_n(out_census_status.begin(), n, kSentinelStatus);
    std::fill_n(out_interior.begin(), n, u8{0xff});
    std::fill_n(out_shell.begin(), n, u8{0xff});
    std::fill_n(out_candidates.begin(), n, ~u32{0});
    std::fill_n(out_ids.begin(), n * kOutIdsPerBall, kSentinelId);
  }
};

}  // namespace census_route_detail

class CensusRouteContext {
 public:
  CensusRouteContext() = default;
  CensusRouteContext(const CensusRouteContext&) = delete;
  CensusRouteContext& operator=(const CensusRouteContext&) = delete;
  // DeviceBuffer destructors own the fallback cleanup even if close() was
  // forgotten. Explicit close() is required to observe release errors/costs.
  ~CensusRouteContext() = default;

  bool ready() const { return state_ != nullptr; }
  std::string index_digest() const { return state_ == nullptr ? "" : state_->index_digest; }
  size_t lot_capacity() const { return state_ == nullptr ? 0 : state_->lot; }

#ifdef MHGP7_TESTING
  // Test-only fault before the numbered allocation (1 based), never a product
  // budget. Previous real allocations must be released on this host exception.
  void test_fail_allocation_at(u64 ordinal) { allocation_failure_ = ordinal; }
  void test_kernel_mutant(u32 bits) { kernel_mutant_ = bits; }
#endif

  std::string close(CensusRouteCosts* costs = nullptr) {
    CensusRouteCosts ignored;
    CensusRouteCosts& c = costs == nullptr ? ignored : *costs;
    c.device_resident_bytes = owned_bytes_;
    const u64 failures = c.failed_cuda_frees;
    if (state_ != nullptr) {
      state_->release(c);
      state_.reset();
      owned_bytes_ = 0;
    }
    return c.failed_cuda_frees == failures ? "" : "cuda census route : cudaFree failed";
  }

  std::string prepare(const CloudIndex& ix, size_t lot, CensusRouteCosts* costs = nullptr) {
    CensusRouteCosts ignored;
    CensusRouteCosts& c = costs == nullptr ? ignored : *costs;
    if (const std::string error = close(costs); !error.empty()) return error;
    try {
      if (lot == 0 || lot > std::numeric_limits<u32>::max() ||
          lot > std::numeric_limits<size_t>::max() / kWireBallInBytes)
        throw census_route_detail::Error("invalid_input : CUDA lot outside wire representation");
      GpuCloudIndexWire wire;
      {
        census_route_detail::Timer timer{c.index_wire_ms};
        wire = build_index_wire(ix);
      }
      if (!wire.error.empty()) throw census_route_detail::Error(wire.error);
      state_ = std::make_unique<census_route_detail::State>();
      auto& s = *state_;
      s.root = wire.root; s.n_positions = wire.n_upos;
      s.total_mass = ix.wsum.back(); s.index_digest = wire.host_wire_digest; s.lot = lot;
      u64 allocation_ordinal = 0;
      const auto allocate = [&](auto& buffer, size_t bytes) {
        if (bytes == 0) return;
        ++allocation_ordinal;
#ifdef MHGP7_TESTING
        if (allocation_failure_ != 0 && allocation_ordinal == allocation_failure_)
          throw census_route_detail::Error("test : CUDA allocation failure");
#endif
        buffer.allocate(bytes, c);
        owned_bytes_ += bytes;
      };
      const auto upload = [&](auto& buffer, const std::vector<u8>& bytes) {
        allocate(buffer, bytes.size());
        census_route_detail::transfer(buffer.data, bytes.data(), bytes.size(), cudaMemcpyHostToDevice,
                                     c.h2d_ms, c.h2d_index_bytes);
      };
      upload(s.left, wire.node_left); upload(s.right, wire.node_right);
      upload(s.first, wire.node_first); upload(s.last, wire.node_last);
      upload(s.boxes, wire.node_box); upload(s.positions, wire.upos); upload(s.weights, wire.wsum);
      allocate(s.ball_words, lot * kWireBallInBytes); allocate(s.count, lot * sizeof(u64));
      allocate(s.pre_status, lot); allocate(s.ids, lot * kOutIdsPerBall * sizeof(i32));
      allocate(s.census_status, lot); allocate(s.n_interior, lot); allocate(s.n_shell, lot);
      allocate(s.candidate_indices, lot * sizeof(u32));
      {
        census_route_detail::Timer timer{c.host_setup_ms};
        s.input.resize(lot * kWireBallInBytes); s.out_count.resize(lot);
        s.out_pre_status.resize(lot); s.out_census_status.resize(lot);
        s.out_interior.resize(lot); s.out_shell.resize(lot);
        s.out_candidates.resize(lot); s.out_ids.resize(lot * kOutIdsPerBall);
      }
      return "";
    } catch (const census_route_detail::Error& e) {
      (void)close(&c); return e.what();
    } catch (const std::bad_alloc&) {
      (void)close(&c); return "resource_exhausted : CUDA route allocation";
    } catch (const std::length_error&) {
      (void)close(&c); return "resource_exhausted : CUDA route length";
    }
  }

  std::string run(const std::vector<BallCandidate>& candidates, u64 smax, size_t shell_cap,
                  std::vector<Survivor>* survivors, std::vector<BallData>* balls,
                  ExpandStats* stats, CensusRouteCosts* costs = nullptr) {
    if (survivors != nullptr) survivors->clear();
    if (balls != nullptr) balls->clear();
    CensusRouteCosts ignored;
    CensusRouteCosts& c = costs == nullptr ? ignored : *costs;
    c.device_resident_bytes = owned_bytes_;
    try {
      if (survivors == nullptr || balls == nullptr || stats == nullptr || state_ == nullptr)
        throw census_route_detail::Error("invalid_input : CUDA route outputs/context");
      if (smax < 2 || smax > 11 || shell_cap < 4 || shell_cap > 12 ||
          candidates.size() > std::numeric_limits<u32>::max())
        throw census_route_detail::Error("invalid_input : CUDA census profile");
      auto& s = *state_;
      std::vector<Survivor> next_survivors;
      std::vector<BallData> next_balls;
      u64 dead = 0, interior = 0, shell = 0;
      c.candidates += candidates.size();
      for (size_t base = 0; base < candidates.size(); base += s.lot) {
        const u32 n = static_cast<u32>(std::min(s.lot, candidates.size() - base));
        {
          census_route_detail::Timer timer{c.pack_ms};
          for (size_t j = 0; j < n; ++j)
            if (candidates[base + j].arity < 2 || candidates[base + j].arity > 4 ||
                candidates[base + j].arity > smax)
              throw census_route_detail::Error("invalid_input : CUDA candidate arity");
          const PackStatus status = pack_candidate_range(s.input.data(), s.input.size(), 0,
              CandidateSpan{candidates.data() + base, n}, smax);
          if (status != PackStatus::kOk) throw census_route_detail::Error(pack_status_name(status));
        }
        {
          census_route_detail::Timer timer{c.host_setup_ms};
          s.sentinels(n);
        }
        const auto up = [&](auto& device, const auto& host, size_t count, u64& bytes) {
          census_route_detail::transfer(device.data, host.data(), count * sizeof(host[0]),
                                       cudaMemcpyHostToDevice, c.h2d_ms, bytes);
        };
        up(s.ball_words, s.input, size_t{n} * kWireBallInBytes, c.h2d_ball_bytes);
        up(s.count, s.out_count, n, c.h2d_sentinel_bytes);
        up(s.pre_status, s.out_pre_status, n, c.h2d_sentinel_bytes);
        up(s.census_status, s.out_census_status, n, c.h2d_sentinel_bytes);
        up(s.n_interior, s.out_interior, n, c.h2d_sentinel_bytes);
        up(s.n_shell, s.out_shell, n, c.h2d_sentinel_bytes);
        up(s.ids, s.out_ids, size_t{n} * kOutIdsPerBall, c.h2d_sentinel_bytes);
        up(s.candidate_indices, s.out_candidates, n, c.h2d_sentinel_bytes);
        {
          census_route_detail::Timer timer{c.kernel_ms};
          u32 mutant = 0;
#ifdef MHGP7_TESTING
          mutant = kernel_mutant_;
#endif
          const u32 blocks = static_cast<u32>((u64{n} + 255) / 256);
          ++c.lots_launched;
          MHGP7_LAUNCH(k_prefilter, blocks, 256, s.left.data, s.right.data, s.first.data,
                      s.last.data, s.boxes.data, s.positions.data, s.weights.data, s.root,
                      s.ball_words.data, n, s.count.data, s.pre_status.data, mutant);
          census_route_detail::checked(cudaGetLastError(), "k_prefilter");
          MHGP7_LAUNCH(k_census, blocks, 256, s.left.data, s.right.data, s.boxes.data,
                      s.positions.data, s.root, s.ball_words.data, n, static_cast<u32>(base),
                      static_cast<u32>(shell_cap), s.ids.data, s.census_status.data,
                      s.n_interior.data, s.n_shell.data, s.candidate_indices.data, mutant);
          census_route_detail::checked(cudaGetLastError(), "k_census");
          census_route_detail::checked(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
        }
        const auto down = [&](auto& host, const auto& device, size_t count) {
          census_route_detail::transfer(host.data(), device.data, count * sizeof(host[0]),
                                       cudaMemcpyDeviceToHost, c.d2h_ms, c.d2h_bytes);
        };
        down(s.out_count, s.count, n); down(s.out_pre_status, s.pre_status, n);
        down(s.out_census_status, s.census_status, n); down(s.out_interior, s.n_interior, n);
        down(s.out_shell, s.n_shell, n); down(s.out_ids, s.ids, size_t{n} * kOutIdsPerBall);
        down(s.out_candidates, s.candidate_indices, n);
        {
          census_route_detail::Timer timer{c.rebuild_ms};
          for (u32 j = 0; j < n; ++j) {
            const auto& candidate = candidates[base + j];
            const i32* row = s.out_ids.data() + size_t{j} * kOutIdsPerBall;
            const u64 threshold = wire_threshold(smax, candidate.arity);
            if (const char* error = validate_ball_out(s.out_pre_status[j], s.out_census_status[j],
                s.out_interior[j], s.out_shell[j], s.out_candidates[j], static_cast<u32>(base + j),
                row, s.n_positions, s.out_count[j], threshold, s.total_mass))
              throw census_route_detail::Error(error);
            if (s.out_pre_status[j] == kBallStackOverflow || s.out_census_status[j] == kBallStackOverflow)
              throw census_route_detail::Error("invariant : CUDA census DFS stack overflow");
            if (s.out_pre_status[j] == kBallAtLeastH) { ++dead; continue; }
            if (s.out_census_status[j] == kBallShellOverflow)
              throw census_route_detail::Error("coquille au-dela du plafond (jamais de troncature)");
            if (s.out_census_status[j] == kBallInteriorOverflow || s.out_interior[j] != s.out_count[j])
              throw census_route_detail::Error("invariant : CUDA census contradicts strict count");
            next_survivors.push_back(Survivor{static_cast<u32>(base + j), s.out_count[j]});
            BallData ball;
            ball.key = candidate.key; ball.level = candidate.level; ball.arity = candidate.arity;
            ball.n_interior = s.out_interior[j]; ball.n_shell = s.out_shell[j];
            for (u8 k = 0; k < ball.n_interior; ++k) ball.interior_ids[k] = row[k];
            for (u8 k = 0; k < ball.n_shell; ++k) ball.shell_ids[k] = row[9 + k];
            next_balls.push_back(ball);
            interior += ball.n_interior; shell += ball.n_shell;
          }
          ++c.lots_reconstructed;
        }
      }
      survivors->swap(next_survivors); balls->swap(next_balls);
      stats->dead_depth = dead; stats->survivors = survivors->size();
      stats->census_interior += interior; stats->census_shell += shell;
      return "";
    } catch (const census_route_detail::Error& e) {
      (void)close(&c); return e.what();
    } catch (const std::bad_alloc&) {
      (void)close(&c); return "resource_exhausted : CUDA route allocation";
    } catch (const std::length_error&) {
      (void)close(&c); return "resource_exhausted : CUDA route length";
    }
  }

 private:
  std::unique_ptr<census_route_detail::State> state_;
  u64 owned_bytes_ = 0;
#ifdef MHGP7_TESTING
  u64 allocation_failure_ = 0;
  u32 kernel_mutant_ = 0;
#endif
};

// Cold wrapper: index construction/upload, allocations, all lots and explicit
// release are included in its enclosing wall time. lot=0 means one whole lot.
inline std::string device_prefilter_census_route(const CloudIndex& ix,
    const std::vector<BallCandidate>& candidates, u64 smax, size_t shell_cap,
    std::vector<Survivor>* survivors, std::vector<BallData>* balls, ExpandStats* stats,
    size_t lot = 1u << 21, CensusRouteCosts* costs = nullptr) {
  if (survivors != nullptr) survivors->clear();
  if (balls != nullptr) balls->clear();
  CensusRouteCosts ignored;
  CensusRouteCosts* c = costs == nullptr ? &ignored : costs;
  if (survivors == nullptr || balls == nullptr || stats == nullptr)
    return "invalid_input : CUDA route outputs/context";
  const u64 old_dead = stats->dead_depth, old_survivors = stats->survivors;
  const u64 old_interior = stats->census_interior, old_shell = stats->census_shell;
  CensusRouteContext context;
  const size_t actual_lot = std::max<size_t>(1,
      lot == 0 ? candidates.size() : std::min(lot, candidates.size()));
  if (const std::string error = context.prepare(ix, actual_lot, c); !error.empty()) return error;
  const std::string error = context.run(candidates, smax, shell_cap, survivors, balls, stats, c);
  const std::string release_error = context.close(c);
  if (!error.empty()) return error;
  if (!release_error.empty()) {
    survivors->clear(); balls->clear();
    stats->dead_depth = old_dead; stats->survivors = old_survivors;
    stats->census_interior = old_interior; stats->census_shell = old_shell;
  }
  return release_error;
}

}  // namespace mhgp7::gpu

#endif  // __CUDACC__ || MHGP7_FAKE_DEVICE
