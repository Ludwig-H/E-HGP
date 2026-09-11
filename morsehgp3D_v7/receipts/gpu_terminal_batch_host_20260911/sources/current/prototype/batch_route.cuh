#pragma once

#include <limits>
#include <span>
#include "seed_owner.hpp"
#include "source/morsehgp3D_v7/src/gpu/anchor_meb_key_route.cuh"

namespace mhgp7::gpu_terminal_batch_private {
namespace terminal = gpu_terminal_private;
namespace transport = gpu_meb_key_route_private;

struct DeviceSnapshot {
  terminal::View geometry;
  terminal::SeedView seeds[11]{};
};
struct WireResult {
  terminal::Result result;
  u64 snapshot = 0, batch = 0;
  u32 written = 0;
};
static_assert(std::is_standard_layout_v<WireResult> && std::is_trivially_copyable_v<WireResult>);

// Mutations are test-only inputs to this private route, zero in the adapter.
enum class Mutation : u32 { kNone, kOmit, kOrdinal, kSnapshot, kBatch, kStatus, kTarget, kWork };
MHGP7_HD inline void process(const DeviceSnapshot& device, const terminal::Request* requests,
    u64 count, WireResult* output, u64 batch, u64 gid, Mutation mutation, u64 bad_slot) {
  if (gid >= count || (gid == bad_slot && mutation == Mutation::kOmit)) return;
  const auto& request = requests[gid];
  WireResult result;
  result.snapshot = device.geometry.index.snapshot; result.batch = batch; result.written = 1;
  if (request.k >= 2 && request.k <= 10)
    result.result = terminal::resolve(device.geometry, request, nullptr, device.seeds[request.k]);
  else result.result.ordinal = request.ordinal;
  if (gid == bad_slot) {
    if (mutation == Mutation::kOrdinal) result.result.ordinal ^= 1;
    if (mutation == Mutation::kSnapshot) result.snapshot ^= 1;
    if (mutation == Mutation::kBatch) result.batch ^= 1;
    if (mutation == Mutation::kStatus) result.result.status = terminal::Status::kMebFailure;
    if (mutation == Mutation::kTarget) result.result.target = terminal::kAbsentBall;
    if (mutation == Mutation::kWork) result.result.work.calls = 0;
  }
  output[gid] = result;
}
#if defined(__CUDACC__)
__global__ void kernel(DeviceSnapshot device, const terminal::Request* requests, u64 count,
    WireResult* output, u64 batch, Mutation mutation, u64 bad_slot) {
  const u64 stride = static_cast<u64>(gridDim.x) * blockDim.x;
  for (u64 gid = static_cast<u64>(blockIdx.x) * blockDim.x + threadIdx.x; gid < count; gid += stride)
    process(device, requests, count, output, batch, gid, mutation, bad_slot);
}
#endif

// Host-only publication. Geometry work stays in the quarantined raw rows;
// do not duplicate the complete per-request Work payload just to publish IDs.
struct AcceptedTarget {
  terminal::BallId target = terminal::kAbsentBall;
  u64 ordinal = 0;
};
static_assert(sizeof(AcceptedTarget) < sizeof(terminal::Result));
struct Batch {
  bool complete = false, raw_work_known = false;
  const char* failure = "not_executed";
  std::vector<AcceptedTarget> accepted;
  // Quarantined observations only, never a publishable prefix or a certificate.
  std::vector<WireResult> diagnostic;
};

class Context {
  terminal::SeedOwner owner_;
  transport::Backend backend_;
  const CloudIndex* source_index_;
  const BallData* source_balls_;
  size_t source_ball_count_;
  struct Allocation { void* pointer = nullptr; size_t bytes = 0; };
  std::vector<Allocation> resident_;
  DeviceSnapshot device_;
  terminal::Request* requests_ = nullptr;
  WireResult* results_ = nullptr;
  size_t capacity_ = 0;
  u64 next_batch_ = 1;
  u64 initialization_calls_ = 0, initialization_bytes_ = 0;
  bool closed_ = false, poisoned_ = false, closed_ok_ = true;

  static void need(bool good, const char* reason) { if (!good) throw transport::Error{reason}; }
  template <class T> T* upload_resident(const T* source, size_t count) {
    if (!count) return nullptr;
    need(source && count <= std::numeric_limits<size_t>::max() / sizeof(T), "resident_size");
    const size_t bytes = count * sizeof(T);
    resident_.push_back({nullptr, bytes});
    resident_.back().pointer = backend_.allocate(bytes);
    auto* result = static_cast<T*>(resident_.back().pointer);
    backend_.upload(result, source, bytes); return result;
  }
  void initialize_results(WireResult* results, size_t count) {
    const size_t bytes = count * sizeof(WireResult);  // reserve checked this product
    need(initialization_calls_ != ~u64{0} && initialization_bytes_ <= ~u64{0} - bytes,
        "batch_initialization_counter_overflow");
    // Test transport injection covers initial storage writes too. This device
    // clear is NOT H2D traffic and is accounted separately, once per reserve.
    backend_.injection(transport::Fault::kUpload);
#if defined(__CUDACC__)
    transport::Backend::check(cudaMemset(results, 0, bytes));
#else
    std::memset(static_cast<void*>(results), 0, bytes);
#endif
    ++initialization_calls_; initialization_bytes_ += bytes;
  }
  void reserve(size_t count) {
    if (count <= capacity_) return;
    need(count <= std::numeric_limits<size_t>::max() / sizeof(terminal::Request) &&
        count <= std::numeric_limits<size_t>::max() / sizeof(WireResult), "batch_byte_overflow");
    auto* requests = static_cast<terminal::Request*>(backend_.allocate(count * sizeof(terminal::Request)));
    WireResult* results = nullptr;
    try {
      results = static_cast<WireResult*>(backend_.allocate(count * sizeof(WireResult)));
      initialize_results(results, count);
    } catch (...) {
      (void)backend_.release(results, count * sizeof(WireResult));
      (void)backend_.release(requests, count * sizeof(terminal::Request));
      throw;
    }
    const bool first = backend_.release(requests_, capacity_ * sizeof(terminal::Request));
    const bool second = backend_.release(results_, capacity_ * sizeof(WireResult));
    requests_ = requests; results_ = results; capacity_ = count;
    need(first && second, "batch_old_buffer_release");
  }
  bool valid_result(const WireResult& output, const terminal::Request& input, u64 batch) const {
    const auto& r = output.result;
    const auto& w = r.work;
    const auto host = owner_.view();
    if (output.written != 1 || output.snapshot != host.index.snapshot || output.batch != batch ||
        r.ordinal != input.ordinal || r.status != terminal::Status::kOk || r.target >= host.catalog.count)
      return false;
    const auto& target = host.catalog.entries[r.target];
    if (input.k < static_cast<u64>(target.interior) + target.arity - 1 ||
        input.k > static_cast<u64>(target.interior) + target.shell ||
        compare_exact_level(terminal::decode_level(target.level), terminal::decode_level(input.before)) >= 0)
      return false;
    if (w.post_seed_hits > 1 || w.post_seed_terminals != w.post_seed_hits ||
        w.post_seed_queries != w.intruder_queries || w.anchor_hits != 1 - w.post_seed_hits ||
        !w.calls || w.materializations != w.calls || w.level_materializations != w.calls ||
        w.same_radius_steps > ~u64{0} - w.descending_steps) return false;
    const u64 steps = w.same_radius_steps + w.descending_steps;
    return w.max_chain_steps == steps && w.post_seed_queries == steps &&
        w.calls >= 1 - w.post_seed_hits && steps == w.calls - (1 - w.post_seed_hits) &&
        w.key_lookups == w.calls && w.intruder_queries <= ~u64{0} / 3 &&
        w.axis_divisions == 3 * w.intruder_queries;
  }
 public:
  // Synchronous, single-owner API. The caller keeps the source index/census
  // immutable during the bound Builder run; pointer binding is not a content
  // hash nor protection against mutation through a different alias.
  Context(const CloudIndex& index, std::span<const BallData> balls,
      transport::Fault initial_fault = transport::Fault::kNone, unsigned initial_after = 0)
      : owner_(index, balls), source_index_(&index), source_balls_(balls.data()), source_ball_count_(balls.size()) {
    backend_.fault = initial_fault; backend_.fault_after = initial_after;
    try {
      device_.geometry = owner_.view();
      const auto host = device_.geometry;
      device_.geometry.index.left = upload_resident(host.index.left, host.index.nodes);
      device_.geometry.index.right = upload_resident(host.index.right, host.index.nodes);
      device_.geometry.index.first = upload_resident(host.index.first, host.index.nodes);
      device_.geometry.index.last = upload_resident(host.index.last, host.index.nodes);
      device_.geometry.index.box6 = upload_resident(host.index.box6, 6ull * host.index.nodes);
      device_.geometry.index.positions3 = upload_resident(host.index.positions3, 3ull * host.index.positions);
      device_.geometry.catalog.entries = upload_resident(host.catalog.entries, host.catalog.count);
      device_.geometry.catalog.by_key = upload_resident(host.catalog.by_key, host.catalog.count);
      for (u32 k = 2; k <= 10; ++k) {
        device_.seeds[k] = owner_.seeds(k);
        device_.seeds[k].entries = upload_resident(device_.seeds[k].entries, device_.seeds[k].count);
      }
    } catch (...) { if (!close()) throw transport::Error{"resident_cleanup_uncertified"}; throw; }
  }
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  ~Context() { (void)close(); }
  bool bound_to(const CloudIndex& index, std::span<const BallData> balls) const {
    return !closed_ && !poisoned_ && &index == source_index_ && balls.data() == source_balls_ &&
        balls.size() == source_ball_count_;
  }
  u64 snapshot() const { return owner_.view().index.snapshot; }
  terminal::SeedView seeds(u32 k) const { return owner_.seeds(k); }
  const transport::Backend& backend() const { return backend_; }
  size_t capacity() const { return capacity_; }
  u64 initialization_calls() const { return initialization_calls_; }
  u64 initialization_bytes() const { return initialization_bytes_; }
  void inject(transport::Fault fault, unsigned after = 0) { backend_.fault = fault; backend_.fault_after = after; }
  bool close() noexcept {
    if (closed_) return closed_ok_;
    closed_ = true;
    const bool a = backend_.release(requests_, capacity_ * sizeof(terminal::Request));
    const bool b = backend_.release(results_, capacity_ * sizeof(WireResult));
    requests_ = nullptr; results_ = nullptr; capacity_ = 0;
    for (auto at = resident_.rbegin(); at != resident_.rend(); ++at)
      if (!backend_.release(at->pointer, at->bytes)) closed_ok_ = false;
    resident_.clear(); backend_.mark_closed();
    closed_ok_ = closed_ok_ && a && b && backend_.cleanup_certified;
    return closed_ok_;
  }
  Batch execute(std::span<const terminal::Request> input, Mutation mutation = Mutation::kNone,
      u64 bad_slot = ~u64{0}) {
    Batch result;
    try {
      need(!closed_ && !poisoned_, "batch_context_unavailable");
      backend_.bind_thread_device();
      // Prevalidation of the WHOLE batch; no launch/allocation on malformed input.
      for (const auto& request : input) {
        need(request.snapshot == snapshot() && request.k >= 2 && request.k <= 10 &&
            terminal::decode_level(request.before).den > 0, "batch_request_identity");
        for (u32 j = 0; j < 10; ++j)
          need(j < request.k ? (request.selected[j] >= 0 &&
              static_cast<u32>(request.selected[j]) < owner_.view().index.positions &&
              (!j || request.selected[j - 1] < request.selected[j])) : request.selected[j] == 0,
              "batch_request_sites");
      }
      if (input.empty()) { result.complete = true; result.raw_work_known = true; result.failure = nullptr; return result; }
      need(next_batch_ != ~u64{0}, "batch_generation_exhausted");
      const u64 batch = next_batch_++;
      reserve(input.size());
      result.diagnostic.resize(input.size());
      backend_.upload(requests_, input.data(), input.size_bytes());
      // Newly allocated rows have written=0. Reused rows carry a strictly
      // earlier batch epoch. The non-wrapping epoch check rejects both classes
      // if this kernel omits a row; no per-batch H2D sentinel reset is needed.
      backend_.injection(transport::Fault::kLaunch);
#if defined(__CUDACC__)
      // A finite grid with grid-stride loops, not a limit on the request count.
      const auto blocks = static_cast<unsigned>(std::min<u64>((input.size() + 127) / 128, 32768));
      kernel<<<blocks, 128>>>(device_, requests_, input.size(), results_, batch, mutation, bad_slot);
      transport::Backend::check(cudaGetLastError());
#else
      for (u64 gid = 0; gid < input.size(); ++gid)
        process(device_, requests_, input.size(), results_, batch, gid, mutation, bad_slot);
#endif
      ++backend_.work.launches;
      backend_.synchronize();
      backend_.download(result.diagnostic.data(), results_, result.diagnostic.size() * sizeof(WireResult));
      for (size_t i = 0; i < input.size(); ++i)
        need(valid_result(result.diagnostic[i], input[i], batch), "batch_result_rejected");
      result.raw_work_known = true;
      std::vector<AcceptedTarget> accepted;
      accepted.reserve(input.size());
      for (const auto& row : result.diagnostic)
        accepted.push_back({row.result.target, row.result.ordinal});
      backend_.injection(transport::Fault::kHostPublish);
      result.accepted = std::move(accepted); result.complete = true; result.failure = nullptr;
    } catch (const transport::Error& error) { result.failure = error.reason; poisoned_ = true; }
      catch (const std::exception&) { result.failure = "batch_host_exception"; poisoned_ = true; }
      catch (...) { result.failure = "batch_unknown_exception"; poisoned_ = true; }
    return result;
  }
};
}  // namespace mhgp7::gpu_terminal_batch_private
