#pragma once

// PRIVATE local-MEB route only: no terminal, BVH traversal or FULL authority.
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "anchor_meb_key.cuh"
#include "../tree/cloud_index.hpp"

namespace mhgp7::gpu_meb_key_route_private {
namespace primitive = gpu_meb_private;
namespace key_helper = gpu_meb_key_private;

// Explicit 64-bit little-endian process ABI, not a portable archive format.
struct alignas(8) WireRequest {
  u64 snapshot = 0, batch = 0;
  u32 ordinal = 0, count = 0;
  i32 sites[10]{};
  u32 reserved[2]{};
};
struct alignas(8) WireSelection {
  u64 snapshot = 0, batch = 0;
  u32 ordinal = 0, status = 0xee, q = 0, shell = 0;
  u32 slots[4]{};
  u64 calls = 0, supports[5]{}, powers = 0, reserved = 0;
  u64 key_words[10]{}, key_materializations = 0;
  u64 key_status = static_cast<u64>(key_helper::KeyStatus::kUnwritten);
};
static_assert(sizeof(void*) == 8 && std::endian::native == std::endian::little);
static_assert(std::is_trivially_copyable_v<WireRequest> && std::is_standard_layout_v<WireRequest>);
static_assert(std::is_trivially_copyable_v<WireSelection> && std::is_standard_layout_v<WireSelection>);
static_assert(sizeof(WireRequest) == 72 && alignof(WireRequest) == 8 && offsetof(WireRequest, sites) == 24);
static_assert(sizeof(WireSelection) == 208 && alignof(WireSelection) == 8 &&
    offsetof(WireSelection, slots) == 32 && offsetof(WireSelection, calls) == 48 &&
    offsetof(WireSelection, supports) == 56 && offsetof(WireSelection, powers) == 96);
static_assert(offsetof(WireSelection, key_words) == 112 && offsetof(WireSelection, key_materializations) == 192 &&
    offsetof(WireSelection, key_status) == 200);
static_assert(sizeof(P3) == 24 && alignof(P3) == 8 && offsetof(P3, y) == 8 && offsetof(P3, z) == 16);

inline constexpr u32 kAbiVersion = 2;
inline constexpr u32 kOmitLast = 0x100, kBadLastShell = 0x200, kWrongBatch = 0x400,
    kWrongSnapshot = 0x800, kBadLastInput = 0x1000, kBadReserved = 0x2000,
    kBadSlot = 0x4000, kBadStatus = 0x8000, kBadCounter = 0x10000;
inline constexpr u32 kBadKeyStatus = 0x800000, kBadKeyCounter = 0x1000000;
inline constexpr u32 key_mutation_flag(key_helper::KeyMutation mutation) {
  return static_cast<u32>(mutation) << 17;
}

MHGP7_HD inline void process_one(const P3* positions, u64 position_count,
    const WireRequest* input, u32 count, WireSelection* output, u64 snapshot, u64 batch,
    u64 gid, u32 flags) {
  if (gid >= count || ((flags & kOmitLast) && gid + 1 == count)) return;
  WireRequest request = input[gid];
  if ((flags & kBadLastInput) && gid + 1 == count) request.sites[0] = -1;
  WireSelection selected;
  selected.snapshot = request.snapshot; selected.batch = request.batch; selected.ordinal = request.ordinal;
  selected.status = static_cast<u32>(primitive::Status::kInvalidInput); selected.calls = 1;
  if (request.snapshot == snapshot && request.batch == batch && request.count <= 10 &&
      request.reserved[0] == 0 && request.reserved[1] == 0) {
    primitive::Request local;
    local.ordinal = request.ordinal; local.count = static_cast<u8>(request.count);
    for (u8 i = 0; i < 10; ++i) local.sites[i] = request.sites[i];
    const auto ball = key_helper::select_meb_key(positions, position_count, local, flags & 31,
        static_cast<key_helper::KeyMutation>((flags >> 17) & 7));
    const auto& result = ball.selected;
    selected.status = static_cast<u32>(result.status); selected.q = result.q; selected.shell = result.shell;
    for (u8 i = 0; i < 4; ++i) selected.slots[i] = result.slots[i];
    selected.calls = result.calls; selected.powers = result.powers;
    for (u8 q = 0; q < 5; ++q) selected.supports[q] = result.supports[q];
    for (u8 word = 0; word < 10; ++word) selected.key_words[word] = ball.key.words[word];
    selected.key_materializations = ball.key_materializations;
    selected.key_status = static_cast<u64>(ball.key_status);
  }
  if (gid + 1 == count) {
    if (flags & kBadLastShell) selected.shell = 11;
    if (flags & kWrongBatch) ++selected.batch;
    if (flags & kWrongSnapshot) ++selected.snapshot;
    if (flags & kBadReserved) selected.reserved = 1;
    if (flags & kBadSlot) selected.slots[0] = 300;
    if (flags & kBadStatus) selected.status = 300;
    if (flags & kBadCounter) selected.calls = 0;
    if (flags & kBadKeyStatus) selected.key_status = 100;
    if (flags & kBadKeyCounter) selected.key_materializations = 0;
  }
  output[gid] = selected;
}

inline constexpr std::array<u64, 16> host_abi{
    kAbiVersion, sizeof(P3), alignof(P3), sizeof(WireRequest), alignof(WireRequest),
    offsetof(WireRequest, sites), sizeof(WireSelection), alignof(WireSelection),
    offsetof(WireSelection, slots), offsetof(WireSelection, calls),
    offsetof(WireSelection, supports), offsetof(WireSelection, powers), offsetof(WireSelection, key_words),
    offsetof(WireSelection, key_materializations), offsetof(WireSelection, key_status), sizeof(key_helper::PrimitiveKeyWords)};
#if defined(__CUDACC__)
__global__ void route_kernel(const P3* positions, u64 position_count, const WireRequest* input,
    u32 count, WireSelection* output, u64 snapshot, u64 batch, u32 flags) {
  const u64 gid = static_cast<u64>(blockIdx.x) * blockDim.x + threadIdx.x;
  process_one(positions, position_count, input, count, output, snapshot, batch, gid, flags);
}
__global__ void abi_kernel(u64* result) {
  const u64 values[16]{kAbiVersion, sizeof(P3), alignof(P3), sizeof(WireRequest), alignof(WireRequest),
      offsetof(WireRequest, sites), sizeof(WireSelection), alignof(WireSelection),
      offsetof(WireSelection, slots), offsetof(WireSelection, calls),
      offsetof(WireSelection, supports), offsetof(WireSelection, powers), offsetof(WireSelection, key_words),
      offsetof(WireSelection, key_materializations), offsetof(WireSelection, key_status), sizeof(key_helper::PrimitiveKeyWords)};
  for (u8 i = 0; i < 16; ++i) result[i] = values[i];
}
#endif

struct Error { const char* reason; };
enum class Fault { kNone, kAllocation, kUpload, kLaunch, kSynchronize, kDownload, kHostPublish, kRelease };
struct Phases {
  u64 allocations = 0, h2d_bytes = 0, d2h_bytes = 0, launches = 0, synchronizations = 0;
  u64 host_validation_powers = 0, host_materializations = 0;
  u64 reported_selection_calls = 0, reported_selection_powers = 0, reported_supports[5]{};
  u64 reported_key_materializations = 0;
  bool observed_complete = false;
};

class IndexSnapshot {
  struct Payload {
    const CloudIndex index;
    const u64 serial;
    Payload(const CloudIndex& source, u64 token) : index(source), serial(token) {}
  };
  std::shared_ptr<const Payload> owner_;
  explicit IndexSnapshot(std::shared_ptr<const Payload> owner) : owner_(std::move(owner)) {}
 public:
  IndexSnapshot() = default;
  static IndexSnapshot capture(const CloudIndex& index) {
    if (!index.valid || index.upos.size() != index.keys.size() ||
        index.upos.size() > static_cast<size_t>(std::numeric_limits<i32>::max())) throw Error{"snapshot_invalid"};
    for (size_t i = 0; i < index.upos.size(); ++i)
      if (!p3_in_profile(index.upos[i]) || index.keys[i] != morton48(index.upos[i]) ||
          (i && index.keys[i - 1] >= index.keys[i])) throw Error{"snapshot_geometry_invalid"};
    static std::mutex mutex;
    static u64 next = 1;
    std::lock_guard<std::mutex> lock(mutex);
    if (next == std::numeric_limits<u64>::max()) throw Error{"snapshot_serial_exhausted"};
    return IndexSnapshot(std::make_shared<const Payload>(index, next++));
  }
  bool valid() const { return static_cast<bool>(owner_); }
  bool same_owner(const IndexSnapshot& other) const { return owner_ && owner_ == other.owner_; }
  const CloudIndex& index() const { if (!owner_) throw Error{"snapshot_missing"}; return owner_->index; }
  u64 serial() const { if (!owner_) throw Error{"snapshot_missing"}; return owner_->serial; }
};

class Backend {
  bool closed_ = false;
 public:
  std::string device_name = "host_stub";
  int major = 0, minor = 0;
  Fault fault = Fault::kNone;
  unsigned fault_after = 0;
  Phases work;
  u64 allocation_bytes = 0, free_attempts = 0, free_failures = 0, certified_free_bytes = 0;
  bool cleanup_certified = true;
  Backend() {
#if defined(__CUDACC__)
    check(cudaSetDevice(0));
    cudaDeviceProp properties{};
    check(cudaGetDeviceProperties(&properties, 0));
    major = properties.major; minor = properties.minor; device_name = properties.name;
    if (major != 12 || minor != 0) throw Error{"expected_sm_120"};
#endif
  }
  Backend(const Backend&) = delete;
  Backend& operator=(const Backend&) = delete;
#if defined(__CUDACC__)
  static void check(cudaError_t status) { if (status != cudaSuccess) throw Error{cudaGetErrorName(status)}; }
#endif
  void injection(Fault point) {
    if (fault != point) return;
    if (fault_after) { --fault_after; return; }
    throw Error{"injected_transport_failure"};
  }
  void* allocate(size_t bytes) {
    injection(Fault::kAllocation);
    void* result = nullptr;
#if defined(__CUDACC__)
    check(cudaMalloc(&result, bytes));
#else
    result = ::operator new(bytes);
#endif
    ++work.allocations; allocation_bytes += bytes; return result;
  }
  bool release(void* pointer, size_t bytes) noexcept {
    if (!pointer) return true;
    ++free_attempts;
    bool success = true;
#if defined(__CUDACC__)
    success = cudaSetDevice(0) == cudaSuccess && cudaFree(pointer) == cudaSuccess;
#else
    ::operator delete(pointer);
#endif
    // Fault reports an UNCERTIFIED release after the physical test allocation
    // is freed, so fault tests do not intentionally leak. A real CUDA failure
    // is never retried or reclassified as successful by this injection.
    if (fault == Fault::kRelease) {
      if (fault_after) --fault_after;
      else success = false;
    }
    if (!success) { ++free_failures; cleanup_certified = false; }
    else certified_free_bytes += bytes;
    return success;
  }
  void upload(void* destination, const void* source, size_t bytes) {
    injection(Fault::kUpload);
#if defined(__CUDACC__)
    check(cudaMemcpy(destination, source, bytes, cudaMemcpyHostToDevice));
#else
    std::memcpy(destination, source, bytes);
#endif
    work.h2d_bytes += bytes;
  }
  void download(void* destination, const void* source, size_t bytes) {
    injection(Fault::kDownload);
#if defined(__CUDACC__)
    check(cudaMemcpy(destination, source, bytes, cudaMemcpyDeviceToHost));
#else
    std::memcpy(destination, source, bytes);
#endif
    work.d2h_bytes += bytes;
  }
  void synchronize() {
    injection(Fault::kSynchronize);
#if defined(__CUDACC__)
    check(cudaDeviceSynchronize());
#endif
    ++work.synchronizations;
  }
  void launch(const P3* positions, u64 n, const WireRequest* requests, u32 count,
      WireSelection* output, u64 snapshot, u64 batch, u32 flags) {
    injection(Fault::kLaunch);
    if (count == 0) throw Error{"internal_zero_launch"};
#if defined(__CUDACC__)
    const auto blocks = static_cast<unsigned>((static_cast<u64>(count) + 127) / 128);
    route_kernel<<<blocks, 128>>>(positions, n, requests, count, output, snapshot, batch, flags);
    check(cudaGetLastError());
#else
    for (u64 gid = 0; gid < static_cast<u64>(count) + 3; ++gid)
      process_one(positions, n, requests, count, output, snapshot, batch, gid, flags);
#endif
    ++work.launches;
  }
  void validate_abi() {
    void* storage = allocate(sizeof(host_abi));
    try {
      std::array<u64, 16> observed{};
#if defined(__CUDACC__)
      abi_kernel<<<1, 1>>>(static_cast<u64*>(storage));
      check(cudaGetLastError()); ++work.launches;
#else
      std::memcpy(storage, host_abi.data(), sizeof(host_abi)); ++work.launches;
#endif
      synchronize(); download(observed.data(), storage, sizeof(observed));
      if (observed != host_abi) throw Error{"device_abi_mismatch"};
    } catch (...) { (void)release(storage, sizeof(host_abi)); throw; }
    if (!release(storage, sizeof(host_abi))) throw Error{"abi_release_failed"};
  }
  void bind_thread_device() {
    if (closed_) throw Error{"backend_closed"};
#if defined(__CUDACC__)
    int device = -1; check(cudaGetDevice(&device));
    if (device != 0) throw Error{"device_owner_mismatch"};
#endif
  }
  void mark_closed() { closed_ = true; }
};

struct AcceptedMebKey {
  key_helper::PrimitiveKeyWords key;
  u8 support_size = 0, selected_shell_count = 0;
  std::array<u8, 4> support_slots{};
};

// Metadata-only acceptance. No power, form, GCD or key materialization on the
// host path. Geometry/key exactness is the qualified producer's obligation.
inline bool accept_metadata(const primitive::Request& request, const WireSelection& raw) {
  if (raw.calls != 1 || raw.supports[0] || raw.q < 1 || raw.q > 4 || raw.q > request.count ||
      raw.shell < raw.q || raw.shell > request.count || (request.count == 1) != (raw.q == 1) ||
      raw.key_status != static_cast<u64>(key_helper::KeyStatus::kOk) || raw.key_materializations != 1 ||
      ((raw.key_words[0] | raw.key_words[1]) == 0) || (raw.key_words[1] >> 63)) return false;
  for (u8 i = 0; i < 4; ++i) {
    if (i < raw.q) {
      if (raw.slots[i] >= request.count || (i && raw.slots[i - 1] >= raw.slots[i])) return false;
    } else if (raw.slots[i]) return false;
  }
  u64 rank = 0, attempts = 0;
  for (u32 i = 0; i < raw.q; ++i) {
    const u32 begin = i == 0 ? 0 : raw.slots[i - 1] + 1;
    for (u32 v = begin; v < raw.slots[i]; ++v)
      rank += primitive::choose(request.count - v - 1, raw.q - i - 1);
  }
  for (u32 q = 1; q <= 4; ++q) {
    const u64 expected = q == raw.q ? rank + 1 : (q >= 2 && q < raw.q ? primitive::choose(request.count, q) : 0);
    if (raw.supports[q] != expected) return false;
    attempts += expected;
  }
  return raw.q == 1 ? raw.powers == 0 : raw.powers >= request.count && raw.powers <= attempts * request.count;
}
struct BatchResult {
  bool passed = false;
  const char* reason = "not_run";
  std::vector<AcceptedMebKey> values;  // no ExactLevel, no host-generated key
  std::vector<WireSelection> observed; // diagnostics, never geometry authority
  Phases work;
};

class Context {
  IndexSnapshot snapshot_;
  Backend backend_;
  const std::thread::id thread_ = std::this_thread::get_id();
  P3* positions_ = nullptr;
  WireRequest* requests_ = nullptr;
  WireSelection* results_ = nullptr;
  size_t capacity_ = 0;
  u64 next_batch_ = 1;
  bool poisoned_ = false, closed_ = false;
  bool close_result_ = true;
  Phases initialization_;
  void grow(size_t requested) {
    if (requested <= capacity_) return;
    size_t capacity = capacity_ ? capacity_ : 1;
    while (capacity < requested) {
      if (capacity > std::numeric_limits<size_t>::max() / 2) throw Error{"capacity_overflow"};
      capacity *= 2;
    }
    if (capacity > std::numeric_limits<size_t>::max() / sizeof(WireSelection)) throw Error{"byte_size_overflow"};
    auto* requests = static_cast<WireRequest*>(backend_.allocate(capacity * sizeof(WireRequest)));
    WireSelection* results = nullptr;
    try { results = static_cast<WireSelection*>(backend_.allocate(capacity * sizeof(WireSelection))); }
    catch (...) { (void)backend_.release(requests, capacity * sizeof(WireRequest)); throw; }
    const bool first = backend_.release(requests_, capacity_ * sizeof(WireRequest));
    const bool second = backend_.release(results_, capacity_ * sizeof(WireSelection));
    requests_ = requests; results_ = results; capacity_ = capacity;
    if (!first || !second) throw Error{"buffer_release_failed"};
  }
 public:
  explicit Context(const IndexSnapshot& snapshot, Fault initialization_fault = Fault::kNone,
      unsigned fault_after = 0) : snapshot_(snapshot) {
    if (!snapshot.valid()) throw Error{"snapshot_missing"};
    backend_.fault = initialization_fault;
    backend_.fault_after = fault_after;
    try {
      backend_.validate_abi();
      if (!snapshot_.index().upos.empty()) {
        positions_ = static_cast<P3*>(backend_.allocate(snapshot_.index().upos.size() * sizeof(P3)));
        backend_.upload(positions_, snapshot_.index().upos.data(), snapshot_.index().upos.size() * sizeof(P3));
      }
      initialization_ = backend_.work;
    } catch (...) {
      (void)backend_.release(positions_, snapshot_.index().upos.size() * sizeof(P3)); positions_ = nullptr;
      if (!backend_.cleanup_certified) throw Error{"initialization_cleanup_uncertified"};
      throw;
    }
  }
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  ~Context() { (void)close(); }
  const Backend& backend() const { return backend_; }
  Phases initialization_work() const { return initialization_; }
  bool poisoned() const { return poisoned_; }
  bool close() noexcept {
    if (closed_) return close_result_;
    const bool a = backend_.release(positions_, snapshot_.index().upos.size() * sizeof(P3));
    const bool b = backend_.release(requests_, capacity_ * sizeof(WireRequest));
    const bool c = backend_.release(results_, capacity_ * sizeof(WireSelection));
    positions_ = nullptr; requests_ = nullptr; results_ = nullptr;
    closed_ = true; backend_.mark_closed();
    close_result_ = a && b && c && backend_.cleanup_certified; return close_result_;
  }
  BatchResult run(const IndexSnapshot& snapshot, std::span<const primitive::Request> input,
      Fault fault = Fault::kNone, u32 flags = 0, unsigned fault_after = 0) {
    BatchResult result;
    if (closed_ || poisoned_ || thread_ != std::this_thread::get_id()) {
      result.reason = "context_unavailable"; return result;
    }
    if (!snapshot_.same_owner(snapshot)) { result.reason = "snapshot_owner_mismatch"; return result; }
    if (input.size() > std::numeric_limits<u32>::max()) { result.reason = "request_count_overflow"; return result; }
    if (input.empty()) { result.passed = true; result.reason = "empty_batch"; return result; }
    backend_.work = {}; backend_.fault = fault; backend_.fault_after = fault_after;
    try {
      // Validate the WHOLE input before allocation or device access.
      std::vector<u32> ordinals;
      ordinals.reserve(input.size());
      for (const auto& request : input) {
        if (!primitive::valid_input(snapshot.index().upos.data(), snapshot.index().upos.size(), request)) {
          result.reason = "invalid_request"; return result;
        }
        ordinals.push_back(request.ordinal);
      }
      std::sort(ordinals.begin(), ordinals.end());
      if (std::adjacent_find(ordinals.begin(), ordinals.end()) != ordinals.end()) {
        result.reason = "duplicate_ordinal"; return result;
      }
      if (next_batch_ == std::numeric_limits<u64>::max()) throw Error{"batch_serial_exhausted"};
      const u64 batch = next_batch_++;
      backend_.bind_thread_device(); grow(input.size());
      std::vector<WireRequest> wire(input.size());
      std::vector<WireSelection> observed(input.size());
      std::vector<AcceptedMebKey> values;
      values.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i) {
        wire[i].snapshot = snapshot_.serial(); wire[i].batch = batch;
        wire[i].ordinal = input[i].ordinal; wire[i].count = input[i].count;
        for (u8 slot = 0; slot < 10; ++slot) wire[i].sites[slot] = input[i].sites[slot];
      }
      backend_.upload(requests_, wire.data(), wire.size() * sizeof(WireRequest));
      // Fresh sentinels defeat stale-buffer and omitted-write acceptance.
      backend_.upload(results_, observed.data(), observed.size() * sizeof(WireSelection));
      backend_.launch(positions_, snapshot_.index().upos.size(), requests_, static_cast<u32>(wire.size()),
          results_, snapshot_.serial(), batch, flags);
      backend_.synchronize();
      backend_.download(observed.data(), results_, observed.size() * sizeof(WireSelection));
      result.observed = std::move(observed); backend_.work.observed_complete = true;
      // Untrusted diagnostic totals are checked for overflow and never used
      // as proof of execution. Retain the raw rows even if geometry is refused.
      const auto add = [](u64& total, u64 value) {
        if (value > std::numeric_limits<u64>::max() - total) throw Error{"reported_counter_overflow"};
        total += value;
      };
      for (const auto& raw : result.observed) {
        add(backend_.work.reported_selection_calls, raw.calls);
        add(backend_.work.reported_selection_powers, raw.powers);
        add(backend_.work.reported_key_materializations, raw.key_materializations);
        for (u8 q = 0; q < 5; ++q) add(backend_.work.reported_supports[q], raw.supports[q]);
      }
      for (size_t i = 0; i < input.size(); ++i) {
        const auto& raw = result.observed[i];
        if (raw.snapshot != snapshot_.serial() || raw.batch != batch || raw.ordinal != input[i].ordinal ||
            raw.status != static_cast<u32>(primitive::Status::kOk) || raw.q > 4 || raw.shell > 10 || raw.reserved)
          throw Error{"invalid_device_output"};
        if (!accept_metadata(input[i], raw)) throw Error{"metadata_refused"};
        AcceptedMebKey accepted;
        accepted.support_size = static_cast<u8>(raw.q); accepted.selected_shell_count = static_cast<u8>(raw.shell);
        for (u8 slot = 0; slot < 4; ++slot) accepted.support_slots[slot] = static_cast<u8>(raw.slots[slot]);
        for (u8 word = 0; word < 10; ++word) accepted.key.words[word] = raw.key_words[word];
        values.push_back(accepted);
      }
      backend_.injection(Fault::kHostPublish);
      result.values = std::move(values); result.passed = true; result.reason = "whole_batch_device_key_accepted";
    } catch (const Error& error) { poisoned_ = true; result.reason = error.reason; }
      catch (const std::bad_alloc&) { poisoned_ = true; result.reason = "host_allocation_failed"; }
    result.work = backend_.work;
    return result;
  }
};

}  // namespace mhgp7::gpu_meb_key_route_private
