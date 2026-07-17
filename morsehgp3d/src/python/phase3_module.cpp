#include <cub/version.cuh>
#include <cuda_runtime_api.h>
#include <dlpack/dlpack.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nvtx3/nvToolsExt.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#if CUDART_VERSION < 12090 || CUDART_VERSION >= 12100
#error "The MorseHGP3D Phase 3 Python module requires CUDA Runtime 12.9.x"
#endif

#ifndef MORSEHGP3D_DLPACK_GIT_COMMIT
#error "MORSEHGP3D_DLPACK_GIT_COMMIT must come from the pinned Phase 3 dependency target"
#endif

#ifndef MORSEHGP3D_NANOBIND_GIT_COMMIT
#error "MORSEHGP3D_NANOBIND_GIT_COMMIT must come from the pinned Phase 3 dependency target"
#endif

namespace nb = nanobind;

namespace {

constexpr std::uint64_t kAllocationLimitBytes = 64ULL * 1024ULL * 1024ULL;
constexpr std::uint32_t kSchemaVersion = 1;
constexpr const char* kDLPackCapsuleName = "dltensor_versioned";
constexpr const char* kDLPackConsumedCapsuleName = "used_dltensor_versioned";

std::atomic<std::uint64_t> live_device_bytes{0};
std::atomic<std::uint64_t> dlpack_cleanup_failures{0};
std::mutex probe_mutex;
std::mutex dlpack_registry_mutex;
struct DLPackCapsuleContext;
std::unordered_map<DLManagedTensorVersioned*, DLPackCapsuleContext*>
    exported_dlpack_tensors;

class Phase3CudaError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

[[noreturn]] void throw_cuda_error(
    const char* operation,
    const cudaError_t status) {
  const char* const name = cudaGetErrorName(status);
  const char* const description = cudaGetErrorString(status);
  std::ostringstream message;
  message << "MorseHGP3D Phase 3 CUDA failure: operation=" << operation
          << ", code=" << static_cast<int>(status)
          << ", name=" << (name == nullptr ? "unknown" : name)
          << ", message="
          << (description == nullptr ? "unknown" : description);
  throw Phase3CudaError(message.str());
}

void require_cuda_success(
    const cudaError_t status,
    const char* const operation) {
  if (status != cudaSuccess) {
    throw_cuda_error(operation, status);
  }
}

std::string format_cuda_version(const int encoded) {
  if (encoded <= 0) {
    return "unavailable";
  }
  const int major = encoded / 1000;
  const int minor = (encoded % 1000) / 10;
  const int patch = encoded % 10;
  std::ostringstream version;
  version << major << '.' << minor << '.' << patch;
  return version.str();
}

class NvtxRange final {
public:
  explicit NvtxRange(const char* const name) noexcept {
    static_cast<void>(nvtxRangePushA(name));
  }

  NvtxRange(const NvtxRange&) = delete;
  NvtxRange& operator=(const NvtxRange&) = delete;

  ~NvtxRange() {
    static_cast<void>(nvtxRangePop());
  }
};

class CudaStream final {
public:
  CudaStream() {
    require_cuda_success(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags");
  }

  CudaStream(const CudaStream&) = delete;
  CudaStream& operator=(const CudaStream&) = delete;

  ~CudaStream() {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
    }
  }

  [[nodiscard]] cudaStream_t get() const noexcept {
    return stream_;
  }

  void synchronize() const {
    require_cuda_success(cudaStreamSynchronize(stream_), "cudaStreamSynchronize");
  }

  void close() {
    require_cuda_success(cudaStreamDestroy(stream_), "cudaStreamDestroy");
    stream_ = nullptr;
  }

private:
  cudaStream_t stream_{nullptr};
};

class AsyncDeviceBuffer final {
public:
  AsyncDeviceBuffer(const std::size_t bytes, const cudaStream_t stream)
      : bytes_(bytes), stream_(stream) {
    require_cuda_success(
        cudaMallocAsync(&pointer_, bytes_, stream_),
        "cudaMallocAsync");
    live_device_bytes.fetch_add(bytes_, std::memory_order_relaxed);
    accounted_ = true;
  }

  AsyncDeviceBuffer(const AsyncDeviceBuffer&) = delete;
  AsyncDeviceBuffer& operator=(const AsyncDeviceBuffer&) = delete;

  ~AsyncDeviceBuffer() {
    cleanup_noexcept();
  }

  [[nodiscard]] void* get() const noexcept {
    return pointer_;
  }

  void release() {
    require_cuda_success(
        cudaFreeAsync(pointer_, stream_),
        "cudaFreeAsync");
    free_enqueued_ = true;
    require_cuda_success(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize after cudaFreeAsync");
    finish_accounting();
  }

private:
  void finish_accounting() noexcept {
    pointer_ = nullptr;
    free_enqueued_ = false;
    if (accounted_) {
      live_device_bytes.fetch_sub(bytes_, std::memory_order_relaxed);
      accounted_ = false;
    }
  }

  void cleanup_noexcept() noexcept {
    if (pointer_ == nullptr) {
      return;
    }
    cudaError_t status = cudaSuccess;
    if (!free_enqueued_) {
      status = cudaFreeAsync(pointer_, stream_);
      free_enqueued_ = status == cudaSuccess;
    }
    if (free_enqueued_) {
      status = cudaStreamSynchronize(stream_);
    }
    if (status == cudaSuccess) {
      finish_accounting();
    }
  }

  void* pointer_{nullptr};
  std::size_t bytes_{0};
  cudaStream_t stream_{nullptr};
  bool accounted_{false};
  bool free_enqueued_{false};
};

struct DLPackCapsuleContext final {
  void* allocation{nullptr};
  cudaStream_t stream{nullptr};
  cudaMemPool_t pool{nullptr};
  int device_id{0};
  std::size_t bytes{0};
  bool accounted{false};
  std::int64_t shape[1]{0};
  std::int64_t strides[1]{1};
};

void release_dlpack_context(DLPackCapsuleContext* const context) noexcept {
  if (context == nullptr) {
    return;
  }
  bool cleanup_ok = true;
  bool release_synchronized = false;
  if (context->allocation != nullptr && context->stream != nullptr) {
    const cudaError_t free_status =
        cudaFreeAsync(context->allocation, context->stream);
    cleanup_ok = cleanup_ok && free_status == cudaSuccess;
    if (free_status == cudaSuccess) {
      const cudaError_t synchronize_status =
          cudaStreamSynchronize(context->stream);
      cleanup_ok = cleanup_ok && synchronize_status == cudaSuccess;
      release_synchronized = synchronize_status == cudaSuccess;
    }
  }
  if (context->pool != nullptr) {
    cleanup_ok =
        cudaMemPoolTrimTo(context->pool, 0) == cudaSuccess && cleanup_ok;
  }
  if (context->stream != nullptr) {
    cleanup_ok =
        cudaStreamDestroy(context->stream) == cudaSuccess && cleanup_ok;
  }
  if (context->accounted && release_synchronized) {
    live_device_bytes.fetch_sub(context->bytes, std::memory_order_relaxed);
    context->accounted = false;
  }
  if (!cleanup_ok || context->accounted) {
    dlpack_cleanup_failures.fetch_add(1, std::memory_order_relaxed);
  }
  delete context;
}

void delete_dlpack_managed_tensor(
    DLManagedTensorVersioned* const managed) noexcept {
  if (managed == nullptr) {
    return;
  }
  {
    std::scoped_lock lock(dlpack_registry_mutex);
    exported_dlpack_tensors.erase(managed);
  }
  auto* const context =
      static_cast<DLPackCapsuleContext*>(managed->manager_ctx);
  release_dlpack_context(context);
  delete managed;
}

void delete_unconsumed_dlpack_capsule(PyObject* const capsule) noexcept {
  if (capsule == nullptr ||
      PyCapsule_IsValid(capsule, kDLPackConsumedCapsuleName) != 0) {
    return;
  }
  if (PyCapsule_IsValid(capsule, kDLPackCapsuleName) == 0) {
    PyErr_Clear();
    return;
  }
  auto* const managed = static_cast<DLManagedTensorVersioned*>(
      PyCapsule_GetPointer(capsule, kDLPackCapsuleName));
  if (managed == nullptr) {
    PyErr_Clear();
    return;
  }
  DLPackCapsuleContext* registered_context = nullptr;
  {
    std::scoped_lock registry_lock(dlpack_registry_mutex);
    const auto entry = exported_dlpack_tensors.find(managed);
    if (entry != exported_dlpack_tensors.end()) {
      registered_context = entry->second;
    }
  }
  void* const capsule_context = PyCapsule_GetContext(capsule);
  if (capsule_context == nullptr && PyErr_Occurred() != nullptr) {
    PyErr_Clear();
  }
  const bool owned_capsule =
      registered_context != nullptr && capsule_context == registered_context &&
      managed->manager_ctx == registered_context;
  if (owned_capsule && managed->deleter != nullptr) {
    managed->deleter(managed);
  }
}

struct DeviceEnvironment final {
  int device_count{0};
  int device_id{0};
  int runtime_version{0};
  int driver_version{0};
  int memory_pools_supported{0};
  std::size_t free_bytes{0};
  std::size_t total_bytes{0};
  cudaDeviceProp properties{};
};

DeviceEnvironment inspect_device_environment() {
  DeviceEnvironment environment;
  require_cuda_success(
      cudaGetDeviceCount(&environment.device_count),
      "cudaGetDeviceCount");
  if (environment.device_count <= 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 CUDA failure: operation=cudaGetDeviceCount, "
        "message=no CUDA device is visible");
  }
  require_cuda_success(cudaGetDevice(&environment.device_id), "cudaGetDevice");
  require_cuda_success(
      cudaRuntimeGetVersion(&environment.runtime_version),
      "cudaRuntimeGetVersion");
  require_cuda_success(
      cudaDriverGetVersion(&environment.driver_version),
      "cudaDriverGetVersion");
  require_cuda_success(
      cudaGetDeviceProperties(
          &environment.properties,
          environment.device_id),
      "cudaGetDeviceProperties");
  require_cuda_success(
      cudaDeviceGetAttribute(
          &environment.memory_pools_supported,
          cudaDevAttrMemoryPoolsSupported,
          environment.device_id),
      "cudaDeviceGetAttribute(cudaDevAttrMemoryPoolsSupported)");
  require_cuda_success(
      cudaMemGetInfo(&environment.free_bytes, &environment.total_bytes),
      "cudaMemGetInfo");
  return environment;
}

nb::dict version_record(
    const std::uint64_t major,
    const std::uint64_t minor,
    const std::uint64_t patch,
    const char* const commit) {
  nb::dict record;
  record["major"] = major;
  record["minor"] = minor;
  record["patch"] = patch;
  record["commit"] = commit;
  return record;
}

nb::dict environment() {
  std::scoped_lock lock(probe_mutex);
  const DeviceEnvironment device = inspect_device_environment();

  nb::dict cuda_version;
  cuda_version["compile_encoded"] = static_cast<std::uint64_t>(CUDART_VERSION);
  cuda_version["compile"] = format_cuda_version(CUDART_VERSION);
  cuda_version["runtime_encoded"] =
      static_cast<std::uint64_t>(device.runtime_version);
  cuda_version["runtime"] = format_cuda_version(device.runtime_version);
  cuda_version["driver_encoded"] =
      static_cast<std::uint64_t>(device.driver_version);
  cuda_version["driver"] = format_cuda_version(device.driver_version);

  nb::dict versions;
  versions["dlpack"] = version_record(
      DLPACK_MAJOR_VERSION,
      DLPACK_MINOR_VERSION,
      0,
      MORSEHGP3D_DLPACK_GIT_COMMIT);
  versions["nanobind"] = version_record(
      NB_VERSION_MAJOR,
      NB_VERSION_MINOR,
      NB_VERSION_PATCH,
      MORSEHGP3D_NANOBIND_GIT_COMMIT);
  versions["cuda"] = cuda_version;
  versions["cub_compile_encoded"] = static_cast<std::uint64_t>(CUB_VERSION);

  nb::list compute_capability;
  compute_capability.append(device.properties.major);
  compute_capability.append(device.properties.minor);

  nb::dict gpu;
  gpu["device_count"] = device.device_count;
  gpu["device_id"] = device.device_id;
  gpu["name"] = device.properties.name;
  gpu["compute_capability"] = compute_capability;
  gpu["total_memory_bytes"] =
      static_cast<std::uint64_t>(device.total_bytes);
  gpu["free_memory_bytes"] =
      static_cast<std::uint64_t>(device.free_bytes);
  gpu["async_allocator_supported"] =
      device.memory_pools_supported != 0;

  nb::dict result;
  result["schema_version"] = kSchemaVersion;
  result["phase"] = 3;
  result["backend"] = "cuda_g4";
  result["profile"] = "hgp_reduced";
  result["mode"] = "certified";
  result["purpose"] = "reproducibility_infrastructure_qualification";
  result["scientific_result_claimed"] = false;
  result["scientific_public_status"] = nb::none();
  result["allocation_limit_bytes"] = kAllocationLimitBytes;
  result["live_device_bytes"] =
      live_device_bytes.load(std::memory_order_relaxed);
  result["dlpack_cleanup_failures"] =
      dlpack_cleanup_failures.load(std::memory_order_relaxed);
  result["versions"] = versions;
  result["gpu"] = gpu;
  return result;
}

void validate_probe_size(const std::uint64_t requested_bytes) {
  if (requested_bytes == 0) {
    throw nb::value_error("DLPack probe requires bytes greater than zero");
  }
  if (requested_bytes > kAllocationLimitBytes) {
    std::ostringstream message;
    message << "DLPack probe bytes=" << requested_bytes
            << " exceeds allocation_limit_bytes=" << kAllocationLimitBytes;
    throw nb::value_error(message.str().c_str());
  }
  if (requested_bytes > std::numeric_limits<std::size_t>::max()) {
    throw nb::value_error("DLPack probe bytes cannot be represented by size_t");
  }
}

nb::capsule make_dlpack_capsule(const std::uint64_t requested_bytes) {
  validate_probe_size(requested_bytes);
  std::scoped_lock lock(probe_mutex);
  if (live_device_bytes.load(std::memory_order_relaxed) != 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 memory counter is nonzero before DLPack export");
  }
  const DeviceEnvironment device = inspect_device_environment();
  if (device.memory_pools_supported == 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 requires CUDA asynchronous memory pools");
  }

  auto context = std::unique_ptr<DLPackCapsuleContext, decltype(&release_dlpack_context)>{
      new DLPackCapsuleContext{},
      &release_dlpack_context};
  context->device_id = device.device_id;
  context->bytes = static_cast<std::size_t>(requested_bytes);
  context->shape[0] = static_cast<std::int64_t>(requested_bytes);
  require_cuda_success(
      cudaStreamCreateWithFlags(&context->stream, cudaStreamNonBlocking),
      "cudaStreamCreateWithFlags(DLPack producer)");
  require_cuda_success(
      cudaDeviceGetDefaultMemPool(&context->pool, context->device_id),
      "cudaDeviceGetDefaultMemPool(DLPack producer)");
  require_cuda_success(
      cudaMallocAsync(
          &context->allocation,
          context->bytes,
          context->stream),
      "cudaMallocAsync(DLPack producer)");
  live_device_bytes.fetch_add(context->bytes, std::memory_order_relaxed);
  context->accounted = true;
  require_cuda_success(
      cudaMemsetAsync(
          context->allocation,
          0xA5,
          context->bytes,
          context->stream),
      "cudaMemsetAsync(DLPack producer)");
  require_cuda_success(
      cudaStreamSynchronize(context->stream),
      "cudaStreamSynchronize(DLPack producer)");

  auto managed = std::make_unique<DLManagedTensorVersioned>();
  managed->version = DLPackVersion{
      static_cast<std::uint32_t>(DLPACK_MAJOR_VERSION),
      static_cast<std::uint32_t>(DLPACK_MINOR_VERSION)};
  managed->manager_ctx = context.get();
  managed->deleter = &delete_dlpack_managed_tensor;
  managed->flags = 0;
  managed->dl_tensor.data = context->allocation;
  managed->dl_tensor.device = DLDevice{kDLCUDA, context->device_id};
  managed->dl_tensor.ndim = 1;
  managed->dl_tensor.dtype = DLDataType{
      static_cast<std::uint8_t>(kDLUInt),
      8,
      1};
  managed->dl_tensor.shape = context->shape;
  managed->dl_tensor.strides = context->strides;
  managed->dl_tensor.byte_offset = 0;
  PyObject* const capsule_object = PyCapsule_New(
      managed.get(),
      kDLPackCapsuleName,
      &delete_unconsumed_dlpack_capsule);
  if (capsule_object == nullptr) {
    nb::raise_python_error();
  }
  nb::capsule capsule = nb::steal<nb::capsule>(capsule_object);
  if (PyCapsule_SetContext(capsule.ptr(), context.get()) != 0) {
    nb::raise_python_error();
  }
  bool registered = false;
  {
    std::scoped_lock registry_lock(dlpack_registry_mutex);
    const auto [unused, inserted] =
        exported_dlpack_tensors.emplace(managed.get(), context.get());
    static_cast<void>(unused);
    registered = inserted;
  }
  if (!registered) {
    throw std::logic_error("duplicate Phase 3 DLPack managed tensor address");
  }
  context.release();
  managed.release();
  return capsule;
}

nb::dict consume_dlpack_capsule(const nb::capsule& capsule) {
  std::scoped_lock lock(probe_mutex);
  const char* const capsule_name = capsule.name();
  if (capsule_name == nullptr ||
      std::string_view{capsule_name} != kDLPackCapsuleName) {
    throw nb::value_error(
        "DLPack consumer requires an unconsumed dltensor_versioned capsule");
  }
  auto* const managed = static_cast<DLManagedTensorVersioned*>(
      capsule.data(kDLPackCapsuleName));
  DLPackCapsuleContext* registered_context = nullptr;
  {
    std::scoped_lock registry_lock(dlpack_registry_mutex);
    const auto entry = exported_dlpack_tensors.find(managed);
    if (entry != exported_dlpack_tensors.end()) {
      registered_context = entry->second;
    }
  }
  void* const capsule_context = PyCapsule_GetContext(capsule.ptr());
  if (capsule_context == nullptr && PyErr_Occurred() != nullptr) {
    nb::raise_python_error();
  }
  if (registered_context == nullptr || capsule_context != registered_context) {
    throw nb::value_error(
        "DLPack capsule is not the owning Phase 3 export or has an invalid "
        "ownership context");
  }
  if (managed == nullptr || managed->manager_ctx == nullptr ||
      managed->manager_ctx != registered_context || managed->deleter == nullptr) {
    throw nb::value_error("DLPack capsule does not own a managed tensor");
  }
  auto* const context =
      static_cast<DLPackCapsuleContext*>(managed->manager_ctx);
  const DLTensor& tensor = managed->dl_tensor;
  const bool version_supported =
      managed->version.major == DLPACK_MAJOR_VERSION &&
      managed->version.minor == DLPACK_MINOR_VERSION;
  const bool producer_declared_copy =
      (managed->flags & DLPACK_FLAG_BITMASK_IS_COPIED) != 0;
  const bool pointer_identity = tensor.data == context->allocation;
  const bool metadata_valid =
      tensor.device.device_type == kDLCUDA &&
      tensor.device.device_id == context->device_id && tensor.ndim == 1 &&
      tensor.dtype.code == static_cast<std::uint8_t>(kDLUInt) &&
      tensor.dtype.bits == 8 && tensor.dtype.lanes == 1 &&
      tensor.shape == context->shape && tensor.strides == context->strides &&
      tensor.shape[0] == static_cast<std::int64_t>(context->bytes) &&
      tensor.strides[0] == 1 && tensor.byte_offset == 0;
  cudaPointerAttributes attributes{};
  require_cuda_success(
      cudaPointerGetAttributes(&attributes, tensor.data),
      "cudaPointerGetAttributes(DLPack consumer)");
  const bool pointer_is_device =
      attributes.type == cudaMemoryTypeDevice &&
      attributes.device == context->device_id;
  if (!version_supported || producer_declared_copy || !pointer_identity ||
      !metadata_valid || !pointer_is_device) {
    throw Phase3CudaError(
        "DLPack capsule consumer rejected the zero-copy exchange invariants");
  }

  const std::uint64_t allocation_address = static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(context->allocation));
  const std::uint64_t data_address = static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(tensor.data));
  const std::uint64_t requested_bytes =
      static_cast<std::uint64_t>(context->bytes);
  const int device_id = context->device_id;
  const std::uint64_t cleanup_failures_before =
      dlpack_cleanup_failures.load(std::memory_order_relaxed);

  if (PyCapsule_SetName(capsule.ptr(), kDLPackConsumedCapsuleName) != 0) {
    nb::raise_python_error();
  }
  managed->deleter(managed);

  const std::uint64_t cleanup_failures_after =
      dlpack_cleanup_failures.load(std::memory_order_relaxed);
  const std::uint64_t live_after =
      live_device_bytes.load(std::memory_order_relaxed);
  if (cleanup_failures_after != cleanup_failures_before || live_after != 0) {
    throw Phase3CudaError(
        "DLPack consumer could not release and trim the producer allocation");
  }

  nb::list shape;
  shape.append(requested_bytes);
  nb::list strides;
  strides.append(1);
  nb::dict dtype;
  dtype["code"] = static_cast<std::uint64_t>(kDLUInt);
  dtype["bits"] = 8;
  dtype["lanes"] = 1;
  nb::dict dlpack;
  dlpack["version_major"] = DLPACK_MAJOR_VERSION;
  dlpack["version_minor"] = DLPACK_MINOR_VERSION;
  dlpack["flags"] = 0;
  dlpack["device_type"] = "kDLCUDA";
  dlpack["device_type_code"] = static_cast<std::uint64_t>(kDLCUDA);
  dlpack["device_id"] = device_id;
  dlpack["ndim"] = 1;
  dlpack["shape"] = shape;
  dlpack["strides"] = strides;
  dlpack["dtype"] = dtype;
  dlpack["byte_offset"] = 0;

  nb::dict result;
  result["schema_version"] = kSchemaVersion;
  result["operation"] = "dlpack_versioned_capsule_exchange";
  result["exchange_protocol"] = "python_array_api_dlpack_versioned_capsule";
  result["capsule_name_before"] = kDLPackCapsuleName;
  result["capsule_name_after"] = kDLPackConsumedCapsuleName;
  result["requested_bytes"] = requested_bytes;
  result["allocation_limit_bytes"] = kAllocationLimitBytes;
  result["allocation_address"] = allocation_address;
  result["dlpack_data_address"] = data_address;
  result["pointer_identity"] = pointer_identity;
  result["zero_copy"] = pointer_identity && !producer_declared_copy;
  result["copy_operations"] = 0;
  result["live_device_bytes_before"] = 0;
  result["peak_live_device_bytes"] = requested_bytes;
  result["live_device_bytes_after"] = live_after;
  result["dlpack"] = dlpack;
  result["scientific_result_claimed"] = false;
  result["scientific_public_status"] = nb::none();
  return result;
}

nb::dict dlpack_zero_copy_probe(const std::uint64_t requested_bytes) {
  if (requested_bytes == 0) {
    throw nb::value_error(
        "dlpack_zero_copy_probe requires bytes greater than zero");
  }
  if (requested_bytes > kAllocationLimitBytes) {
    std::ostringstream message;
    message << "dlpack_zero_copy_probe bytes=" << requested_bytes
            << " exceeds allocation_limit_bytes=" << kAllocationLimitBytes;
    throw nb::value_error(message.str().c_str());
  }
  if (requested_bytes > std::numeric_limits<std::size_t>::max()) {
    throw nb::value_error(
        "dlpack_zero_copy_probe bytes cannot be represented by size_t");
  }

  std::scoped_lock lock(probe_mutex);
  NvtxRange range("morsehgp3d.phase3.dlpack_zero_copy_probe");
  const std::uint64_t live_before =
      live_device_bytes.load(std::memory_order_relaxed);
  if (live_before != 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 memory counter is nonzero before the probe");
  }

  const DeviceEnvironment device = inspect_device_environment();
  if (device.memory_pools_supported == 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 requires CUDA asynchronous memory pools");
  }

  const std::size_t bytes = static_cast<std::size_t>(requested_bytes);
  CudaStream stream;
  AsyncDeviceBuffer allocation(bytes, stream.get());
  const std::uint64_t allocation_address = static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(allocation.get()));
  require_cuda_success(
      cudaMemsetAsync(allocation.get(), 0xA5, bytes, stream.get()),
      "cudaMemsetAsync");
  stream.synchronize();

  std::int64_t shape[1] = {static_cast<std::int64_t>(requested_bytes)};
  std::int64_t strides[1] = {1};
  DLTensor tensor{};
  tensor.data = allocation.get();
  tensor.device = DLDevice{kDLCUDA, device.device_id};
  tensor.ndim = 1;
  tensor.dtype = DLDataType{
      static_cast<std::uint8_t>(kDLUInt),
      8,
      1};
  tensor.shape = shape;
  tensor.strides = strides;
  tensor.byte_offset = 0;

  cudaPointerAttributes attributes{};
  require_cuda_success(
      cudaPointerGetAttributes(&attributes, allocation.get()),
      "cudaPointerGetAttributes");
  const std::uint64_t dlpack_address = static_cast<std::uint64_t>(
      reinterpret_cast<std::uintptr_t>(tensor.data));
  const bool pointer_identity = tensor.data == allocation.get();
  const bool pointer_is_device =
      attributes.type == cudaMemoryTypeDevice &&
      attributes.device == device.device_id;
  if (!pointer_identity || tensor.byte_offset != 0 || !pointer_is_device) {
    throw Phase3CudaError(
        "DLPack zero-copy invariant failed before releasing the allocation");
  }

  allocation.release();
  cudaMemPool_t default_pool = nullptr;
  require_cuda_success(
      cudaDeviceGetDefaultMemPool(&default_pool, device.device_id),
      "cudaDeviceGetDefaultMemPool");
  require_cuda_success(
      cudaMemPoolTrimTo(default_pool, 0),
      "cudaMemPoolTrimTo");
  stream.close();

  const std::uint64_t live_after =
      live_device_bytes.load(std::memory_order_relaxed);
  if (live_after != 0) {
    throw Phase3CudaError(
        "MorseHGP3D Phase 3 memory counter is nonzero after the probe");
  }

  nb::list result_shape;
  result_shape.append(requested_bytes);
  nb::list result_strides;
  result_strides.append(1);

  nb::dict dtype;
  dtype["code"] = static_cast<std::uint64_t>(tensor.dtype.code);
  dtype["bits"] = static_cast<std::uint64_t>(tensor.dtype.bits);
  dtype["lanes"] = static_cast<std::uint64_t>(tensor.dtype.lanes);

  nb::dict dlpack;
  dlpack["device_type"] = "kDLCUDA";
  dlpack["device_type_code"] =
      static_cast<std::uint64_t>(tensor.device.device_type);
  dlpack["device_id"] = tensor.device.device_id;
  dlpack["ndim"] = tensor.ndim;
  dlpack["shape"] = result_shape;
  dlpack["strides"] = result_strides;
  dlpack["dtype"] = dtype;
  dlpack["byte_offset"] = static_cast<std::uint64_t>(tensor.byte_offset);

  nb::dict result;
  result["schema_version"] = kSchemaVersion;
  result["operation"] = "dlpack_zero_copy_probe";
  result["requested_bytes"] = requested_bytes;
  result["allocation_limit_bytes"] = kAllocationLimitBytes;
  result["allocation_address"] = allocation_address;
  result["dlpack_data_address"] = dlpack_address;
  result["pointer_identity"] = pointer_identity;
  result["zero_copy"] = pointer_identity;
  result["live_device_bytes_before"] = live_before;
  result["peak_live_device_bytes"] = requested_bytes;
  result["live_device_bytes_after"] = live_after;
  result["dlpack"] = dlpack;
  result["scientific_result_claimed"] = false;
  result["scientific_public_status"] = nb::none();
  return result;
}

}  // namespace

NB_MODULE(morsehgp3d_phase3, module) {
  module.doc() =
      "MorseHGP3D Phase 3 CUDA/DLPack infrastructure qualification only";
  nb::exception<Phase3CudaError>(module, "Phase3CudaError", PyExc_RuntimeError);
  module.def(
      "environment",
      &environment,
      "Return pinned dependency and CUDA environment metadata without a scientific claim.");
  module.def(
      "make_dlpack_capsule",
      &make_dlpack_capsule,
      nb::arg("bytes").noconvert(),
      "Export one bounded CUDA allocation through a versioned DLPack capsule.");
  module.def(
      "consume_dlpack_capsule",
      &consume_dlpack_capsule,
      nb::arg("capsule"),
      "Consume a versioned DLPack capsule, prove pointer identity, and release it.");
  module.def(
      "dlpack_zero_copy_probe",
      &dlpack_zero_copy_probe,
      nb::arg("bytes").noconvert(),
      "Validate one bounded asynchronous CUDA allocation through an in-place DLTensor view.");
}
