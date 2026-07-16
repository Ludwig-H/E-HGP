#include <cuda_runtime.h>

#include <cub/device/device_reduce.cuh>
#include <cub/version.cuh>
#include <dlpack/dlpack.h>
#include <nvtx3/nvToolsExt.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase3_runtime.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 3 runtime requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 3 runtime"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 3 runtime"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_PREC_DIV) && __CUDA_PREC_DIV != 1
#error "Imprecise division is forbidden for the Phase 3 runtime"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_PREC_SQRT) && __CUDA_PREC_SQRT != 1
#error "Imprecise square root is forbidden for the Phase 3 runtime"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 3 runtime must contain only sm_120 device code"
#endif

#ifndef MORSEHGP3D_GIT_SHA
#define MORSEHGP3D_GIT_SHA "unavailable"
#endif

namespace {

constexpr std::string_view kSchema = "morsehgp3d.phase3.runtime.v1";
constexpr std::size_t kDefaultAllocationCeiling = std::size_t{64} * 1024U * 1024U;
constexpr std::size_t kMinimumAllocationCeiling = std::size_t{4} * 1024U * 1024U;
constexpr std::size_t kMaximumElements = std::size_t{1} << 20U;
constexpr std::uint32_t kKernelSeed = UINT32_C(0x4d4f5253);

std::string encode_json_string(std::string_view value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string encoded;
  encoded.reserve(value.size() + 2U);
  encoded.push_back('"');
  for (const char raw_character : value) {
    const auto character = static_cast<unsigned char>(raw_character);
    switch (character) {
      case '"':
        encoded += "\\\"";
        break;
      case '\\':
        encoded += "\\\\";
        break;
      case '\b':
        encoded += "\\b";
        break;
      case '\f':
        encoded += "\\f";
        break;
      case '\n':
        encoded += "\\n";
        break;
      case '\r':
        encoded += "\\r";
        break;
      case '\t':
        encoded += "\\t";
        break;
      default:
        if (character < 0x20U || character >= 0x7fU) {
          encoded += "\\u00";
          encoded.push_back(kHex[(character >> 4U) & 0x0fU]);
          encoded.push_back(kHex[character & 0x0fU]);
        } else {
          encoded.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  encoded.push_back('"');
  return encoded;
}

template <typename Integer>
std::string encode_json_integer(Integer value) {
  std::array<char, 64> buffer{};
  const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (result.ec != std::errc{}) {
    throw std::runtime_error("integer JSON serialization failed");
  }
  return std::string(buffer.data(), result.ptr);
}

class JsonObject final {
 public:
  void add_string(std::string key, std::string_view value) {
    add_raw(std::move(key), encode_json_string(value));
  }

  void add_bool(std::string key, bool value) {
    add_raw(std::move(key), value ? "true" : "false");
  }

  template <typename Integer>
  void add_integer(std::string key, Integer value) {
    add_raw(std::move(key), encode_json_integer(value));
  }

  void add_null(std::string key) { add_raw(std::move(key), "null"); }

  void add_object(std::string key, const JsonObject& value) {
    add_raw(std::move(key), value.serialize());
  }

  [[nodiscard]] std::string serialize() const {
    std::string output{"{"};
    bool first = true;
    for (const auto& [key, value] : fields_) {
      if (!first) {
        output.push_back(',');
      }
      first = false;
      output += encode_json_string(key);
      output.push_back(':');
      output += value;
    }
    output.push_back('}');
    return output;
  }

 private:
  void add_raw(std::string key, std::string value) {
    const auto [unused, inserted] = fields_.emplace(std::move(key), std::move(value));
    static_cast<void>(unused);
    if (!inserted) {
      throw std::logic_error("duplicate JSON object key");
    }
  }

  std::map<std::string, std::string, std::less<>> fields_;
};

std::string environment_value(const char* name) {
  const char* value = std::getenv(name);
  return value == nullptr ? std::string{} : std::string{value};
}

std::string utc_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto since_epoch = now.time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch - seconds);
  const std::time_t time = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::time_point{seconds});
  std::tm broken_down{};
#if defined(_WIN32)
  if (gmtime_s(&broken_down, &time) != 0) {
    throw std::runtime_error("UTC timestamp conversion failed");
  }
#else
  if (gmtime_r(&time, &broken_down) == nullptr) {
    throw std::runtime_error("UTC timestamp conversion failed");
  }
#endif
  std::array<char, 32> date{};
  if (std::strftime(date.data(), date.size(), "%Y-%m-%dT%H:%M:%S", &broken_down) == 0U) {
    throw std::runtime_error("UTC timestamp formatting failed");
  }
  std::ostringstream output;
  output << date.data() << '.' << std::setw(3) << std::setfill('0')
         << milliseconds.count() << 'Z';
  return output.str();
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message_for(code, operation)),
        code_(code),
        operation_(std::move(operation)) {}

  [[nodiscard]] cudaError_t code() const noexcept { return code_; }
  [[nodiscard]] const std::string& operation() const noexcept { return operation_; }

 private:
  static std::string message_for(cudaError_t code, const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }

  cudaError_t code_;
  std::string operation_;
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

JsonObject cuda_error_json(cudaError_t code,
                           std::string_view operation,
                           bool expected,
                           bool context_healthy) {
  JsonObject error;
  error.add_integer("code", static_cast<int>(code));
  error.add_bool("context_healthy", context_healthy);
  error.add_bool("expected", expected);
  const char* message = cudaGetErrorString(code);
  const char* name = cudaGetErrorName(code);
  error.add_string("message", message == nullptr ? "unknown CUDA error" : message);
  error.add_string("name", name == nullptr ? "unknown" : name);
  error.add_string("operation", operation);
  return error;
}

class NvtxRange final {
 public:
  explicit NvtxRange(const char* label) { static_cast<void>(nvtxRangePushA(label)); }
  NvtxRange(const NvtxRange&) = delete;
  NvtxRange& operator=(const NvtxRange&) = delete;
  ~NvtxRange() { static_cast<void>(nvtxRangePop()); }
};

class CudaEvent final {
 public:
  CudaEvent() { check_cuda(cudaEventCreate(&event_), "cudaEventCreate"); }
  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;
  ~CudaEvent() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_ = nullptr;
};

class AsyncMemoryTracker final {
 public:
  AsyncMemoryTracker(int device, cudaStream_t stream, std::size_t ceiling)
      : stream_(stream), ceiling_(ceiling) {
    check_cuda(cudaDeviceGetDefaultMemPool(&pool_, device),
               "cudaDeviceGetDefaultMemPool");
  }

  AsyncMemoryTracker(const AsyncMemoryTracker&) = delete;
  AsyncMemoryTracker& operator=(const AsyncMemoryTracker&) = delete;

  ~AsyncMemoryTracker() {
    for (auto& allocation : allocations_) {
      if (allocation.active) {
        static_cast<void>(cudaFreeAsync(allocation.pointer, stream_));
        allocation.active = false;
      }
    }
    static_cast<void>(cudaStreamSynchronize(stream_));
    if (pool_ != nullptr) {
      static_cast<void>(cudaMemPoolTrimTo(pool_, 0U));
    }
  }

  void* allocate(std::size_t bytes, std::string_view purpose) {
    if (bytes == 0U) {
      throw std::invalid_argument("a tracked CUDA allocation cannot be empty");
    }
    if (bytes > ceiling_ - live_bytes_) {
      throw std::runtime_error("tracked CUDA allocation would exceed the configured ceiling");
    }
    // Reserve host bookkeeping before mutating the device. Once capacity is
    // available, appending the trivial Allocation record cannot allocate and
    // therefore cannot strand a successful cudaMallocAsync behind bad_alloc.
    allocations_.reserve(allocations_.size() + 1U);
    std::string operation = "cudaMallocAsync(" + std::string{purpose} + ")";
    void* pointer = nullptr;
    check_cuda(cudaMallocAsync(&pointer, bytes, stream_), std::move(operation));
    allocations_.push_back(Allocation{pointer, bytes, true});
    live_bytes_ += bytes;
    peak_bytes_ = std::max(peak_bytes_, live_bytes_);
    ++allocation_count_;
    return pointer;
  }

  void release(void* pointer, std::string_view purpose) {
    const auto found = std::find_if(
        allocations_.begin(), allocations_.end(),
        [pointer](const Allocation& allocation) {
          return allocation.pointer == pointer && allocation.active;
        });
    if (found == allocations_.end()) {
      throw std::logic_error("attempt to release an unknown tracked CUDA allocation");
    }
    std::string operation = "cudaFreeAsync(" + std::string{purpose} + ")";
    check_cuda(cudaFreeAsync(pointer, stream_), std::move(operation));
    found->active = false;
    live_bytes_ -= found->bytes;
    ++release_count_;
  }

  void synchronize_and_trim() {
    check_cuda(cudaStreamSynchronize(stream_), "cudaStreamSynchronize(memory tracker)");
    check_cuda(cudaMemPoolTrimTo(pool_, 0U), "cudaMemPoolTrimTo");
    check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize(memory tracker)");
  }

  [[nodiscard]] std::size_t live_bytes() const noexcept { return live_bytes_; }
  [[nodiscard]] std::size_t peak_bytes() const noexcept { return peak_bytes_; }
  [[nodiscard]] std::size_t allocation_count() const noexcept { return allocation_count_; }
  [[nodiscard]] std::size_t release_count() const noexcept { return release_count_; }

 private:
  struct Allocation {
    void* pointer;
    std::size_t bytes;
    bool active;
  };

  cudaStream_t stream_ = nullptr;
  cudaMemPool_t pool_ = nullptr;
  std::size_t ceiling_ = 0U;
  std::size_t live_bytes_ = 0U;
  std::size_t peak_bytes_ = 0U;
  std::size_t allocation_count_ = 0U;
  std::size_t release_count_ = 0U;
  std::vector<Allocation> allocations_;
};

extern "C" __global__ void morsehgp3d_phase3_deterministic_kernel(
    std::uint32_t* output,
    std::size_t count,
    std::uint32_t seed) {
  const auto index = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < count) {
    auto value = static_cast<std::uint32_t>(index) + seed;
    value ^= value >> 16U;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15U;
    value *= UINT32_C(0x846ca68b);
    value ^= value >> 16U;
    output[index] = value;
  }
}

std::uint32_t phase3_cpu_reference_value(std::size_t index) {
  auto value = static_cast<std::uint32_t>(index) + kKernelSeed;
  value ^= value >> 16U;
  value *= UINT32_C(0x7feb352d);
  value ^= value >> 15U;
  value *= UINT32_C(0x846ca68b);
  value ^= value >> 16U;
  return value;
}

struct Options {
  int device = 0;
  std::size_t allocation_ceiling = kDefaultAllocationCeiling;
  bool exercise_structured_error = false;
  bool show_help = false;
  std::optional<std::string> output_path;
};

std::uint64_t parse_unsigned(std::string_view value, std::string_view option) {
  if (value.empty()) {
    throw std::invalid_argument(std::string{option} + " requires a value");
  }
  std::uint64_t parsed = 0U;
  const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    throw std::invalid_argument(std::string{option} + " requires an unsigned integer");
  }
  return parsed;
}

std::string_view require_next(int& index, int argc, char** argv, std::string_view option) {
  if (index + 1 >= argc) {
    throw std::invalid_argument(std::string{option} + " requires a value");
  }
  ++index;
  return argv[index];
}

Options parse_options(int argc, char** argv) {
  Options options;
  bool ceiling_seen = false;
  bool device_seen = false;
  bool output_seen = false;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
    } else if (argument == "--exercise-structured-error") {
      options.exercise_structured_error = true;
    } else if (argument == "--allocation-bytes" ||
               argument == "--allocation-ceiling-bytes") {
      if (ceiling_seen) {
        throw std::invalid_argument("the allocation size was specified twice");
      }
      ceiling_seen = true;
      const auto parsed = parse_unsigned(
          require_next(index, argc, argv, argument), argument);
      if (parsed > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("the allocation size exceeds size_t");
      }
      options.allocation_ceiling = static_cast<std::size_t>(parsed);
    } else if (argument.starts_with("--allocation-bytes=") ||
               argument.starts_with("--allocation-ceiling-bytes=")) {
      if (ceiling_seen) {
        throw std::invalid_argument("the allocation size was specified twice");
      }
      ceiling_seen = true;
      const std::string_view prefix = argument.starts_with("--allocation-bytes=")
                                          ? "--allocation-bytes="
                                          : "--allocation-ceiling-bytes=";
      const auto parsed = parse_unsigned(
          argument.substr(prefix.size()), prefix.substr(0U, prefix.size() - 1U));
      if (parsed > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("the allocation size exceeds size_t");
      }
      options.allocation_ceiling = static_cast<std::size_t>(parsed);
    } else if (argument == "--device") {
      if (device_seen) {
        throw std::invalid_argument("--device was specified twice");
      }
      device_seen = true;
      const auto parsed = parse_unsigned(require_next(index, argc, argv, argument), argument);
      if (parsed > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("--device exceeds int");
      }
      options.device = static_cast<int>(parsed);
    } else if (argument == "--output") {
      if (output_seen) {
        throw std::invalid_argument("--output was specified twice");
      }
      output_seen = true;
      options.output_path = std::string{require_next(index, argc, argv, argument)};
      if (options.output_path->empty()) {
        throw std::invalid_argument("--output cannot be empty");
      }
    } else if (argument.starts_with("--output=")) {
      if (output_seen) {
        throw std::invalid_argument("--output was specified twice");
      }
      output_seen = true;
      options.output_path = std::string{
          argument.substr(std::string_view{"--output="}.size())};
      if (options.output_path->empty()) {
        throw std::invalid_argument("--output cannot be empty");
      }
    } else {
      throw std::invalid_argument("unknown option: " + std::string{argument});
    }
  }
  if (options.allocation_ceiling < kMinimumAllocationCeiling) {
    throw std::invalid_argument("the allocation ceiling must be at least 4194304 bytes");
  }
  return options;
}

void print_help(std::ostream& output) {
  output
      << "Usage: morsehgp3d_phase3_runtime [options]\n"
      << "  --allocation-bytes N          one async arena and total ceiling (default 67108864)\n"
      << "  --device N                    CUDA device index (default 0)\n"
      << "  --exercise-structured-error   verify a recoverable CUDA error\n"
      << "  --output PATH                 write canonical JSONL to PATH; '-' means stdout\n";
}

struct DeviceManifest {
  int device = 0;
  int driver_version = 0;
  int runtime_version = 0;
  cudaDeviceProp properties{};
  std::string image_ref;
  std::string image_id;
  std::string git_sha;
  bool complete = false;
};

std::string cuda_version_string(int version) {
  const int major = version / 1000;
  const int minor = (version % 1000) / 10;
  const int patch = version % 10;
  return std::to_string(major) + "." + std::to_string(minor) + "." +
         std::to_string(patch);
}

std::string uuid_string(const cudaUUID_t& uuid) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string encoded;
  encoded.reserve(36U);
  for (std::size_t index = 0U; index < sizeof(uuid.bytes); ++index) {
    if (index == 4U || index == 6U || index == 8U || index == 10U) {
      encoded.push_back('-');
    }
    const auto byte = static_cast<unsigned char>(uuid.bytes[index]);
    encoded.push_back(kHex[(byte >> 4U) & 0x0fU]);
    encoded.push_back(kHex[byte & 0x0fU]);
  }
  return encoded;
}

std::string host_compiler_string() {
#if defined(__clang__)
  return std::string{"clang-"} + std::to_string(__clang_major__) + "." +
         std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
  return std::string{"gcc-"} + std::to_string(__GNUC__) + "." +
         std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
  return std::string{"msvc-"} + std::to_string(_MSC_VER);
#else
  return "unknown";
#endif
}

std::string build_mode_string() {
#if defined(NDEBUG)
#if defined(MORSEHGP3D_CUDA_AUDIT) && MORSEHGP3D_CUDA_AUDIT
  return "release_audit";
#else
  return "release";
#endif
#else
  return "debug";
#endif
}

DeviceManifest collect_manifest(int device) {
  DeviceManifest manifest;
  manifest.device = device;
  manifest.image_ref = environment_value("MORSEHGP3D_CUDA_IMAGE_REF");
  manifest.image_id = environment_value("MORSEHGP3D_CUDA_IMAGE_ID");
  manifest.git_sha = MORSEHGP3D_GIT_SHA;
  check_cuda(cudaDriverGetVersion(&manifest.driver_version), "cudaDriverGetVersion");
  check_cuda(cudaRuntimeGetVersion(&manifest.runtime_version), "cudaRuntimeGetVersion");
  check_cuda(cudaGetDeviceProperties(&manifest.properties, device),
             "cudaGetDeviceProperties");
  manifest.complete = !manifest.image_ref.empty() && !manifest.image_id.empty() &&
                      !manifest.git_sha.empty() && manifest.git_sha != "unavailable";
  return manifest;
}

JsonObject manifest_json(const DeviceManifest& manifest) {
  JsonObject output;
  output.add_bool("aot_only", true);
  output.add_string("backend", "cuda_g4");
  output.add_string("build_mode", build_mode_string());
  output.add_integer("cccl_version", CCCL_VERSION);
  output.add_string(
      "cccl_version_string",
      std::to_string(CCCL_MAJOR_VERSION) + "." + std::to_string(CCCL_MINOR_VERSION) +
          "." + std::to_string(CCCL_PATCH_VERSION));
  output.add_integer("clock_rate_khz", manifest.properties.clockRate);
  output.add_string("clocks_source", "cudaDeviceProp");
  output.add_string("compiled_sm", "sm_120");
  output.add_bool("complete", manifest.complete);
  output.add_bool("compilation_during_measurement", false);
  output.add_integer("cub_version", CUB_VERSION);
  output.add_string(
      "cub_version_string",
      std::to_string(CUB_MAJOR_VERSION) + "." + std::to_string(CUB_MINOR_VERSION) +
          "." + std::to_string(CUB_SUBMINOR_VERSION));
  output.add_integer("cuda_compiler_version", __CUDACC_VER_BUILD__ +
                                                  __CUDACC_VER_MINOR__ * 1000 +
                                                  __CUDACC_VER_MAJOR__ * 1000000);
  output.add_string(
      "cuda_compiler_version_string",
      std::to_string(__CUDACC_VER_MAJOR__) + "." +
          std::to_string(__CUDACC_VER_MINOR__) + "." +
          std::to_string(__CUDACC_VER_BUILD__));
  output.add_integer("cuda_driver_version", manifest.driver_version);
  output.add_string("cuda_driver_version_string",
                    cuda_version_string(manifest.driver_version));
  output.add_string("cuda_module_loading",
                    environment_value("CUDA_MODULE_LOADING").empty()
                        ? "environment_unset"
                        : environment_value("CUDA_MODULE_LOADING"));
  output.add_integer("cuda_runtime_version", manifest.runtime_version);
  output.add_string("cuda_runtime_version_string",
                    cuda_version_string(manifest.runtime_version));
  output.add_integer("device_index", manifest.device);
  output.add_integer("dlpack_major_version", DLPACK_MAJOR_VERSION);
  output.add_integer("dlpack_minor_version", DLPACK_MINOR_VERSION);
  output.add_string("dlpack_version_string",
                    std::to_string(DLPACK_MAJOR_VERSION) + "." +
                        std::to_string(DLPACK_MINOR_VERSION));
  output.add_null("forest_semantics");
  output.add_string("git_sha", manifest.git_sha);
  output.add_integer("gpu_compute_capability_major", manifest.properties.major);
  output.add_integer("gpu_compute_capability_minor", manifest.properties.minor);
  output.add_string("gpu_name", manifest.properties.name);
  output.add_string("gpu_runtime_sm",
                    "sm_" + std::to_string(manifest.properties.major) +
                        std::to_string(manifest.properties.minor));
  output.add_string("gpu_uuid", uuid_string(manifest.properties.uuid));
  output.add_integer("gpu_vram_bytes",
                     static_cast<std::uint64_t>(manifest.properties.totalGlobalMem));
  output.add_string("host_compiler", host_compiler_string());
  output.add_string("image_id", manifest.image_id);
  output.add_string("image_ref", manifest.image_ref);
  output.add_integer("memory_clock_rate_khz", manifest.properties.memoryClockRate);
  output.add_string("mode", "certified");
  output.add_bool("nvrtc_used", false);
  output.add_string("profile", "hgp_reduced");
  output.add_bool("scientific_result_claimed", false);
  output.add_null("scientific_public_status");
  return output;
}

struct Measurement {
  std::string protocol;
  std::string start_utc;
  std::string end_utc;
  std::uint64_t gpu_duration_ns = 0U;
  std::uint64_t host_duration_ns = 0U;
};

template <typename Operation>
Measurement measure(std::string protocol, cudaStream_t stream, Operation&& operation) {
  CudaEvent start_event;
  CudaEvent end_event;
  Measurement result;
  result.protocol = std::move(protocol);
  result.start_utc = utc_timestamp();
  const auto host_start = std::chrono::steady_clock::now();
  check_cuda(cudaEventRecord(start_event.get(), stream), "cudaEventRecord(start)");
  std::forward<Operation>(operation)();
  check_cuda(cudaEventRecord(end_event.get(), stream), "cudaEventRecord(end)");
  check_cuda(cudaEventSynchronize(end_event.get()), "cudaEventSynchronize(end)");
  const auto host_end = std::chrono::steady_clock::now();
  result.end_utc = utc_timestamp();
  float elapsed_ms = 0.0F;
  check_cuda(cudaEventElapsedTime(&elapsed_ms, start_event.get(), end_event.get()),
             "cudaEventElapsedTime");
  result.gpu_duration_ns = static_cast<std::uint64_t>(
      std::llround(static_cast<double>(elapsed_ms) * 1000000.0));
  result.host_duration_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(host_end - host_start).count());
  return result;
}

std::uint64_t fnv1a_u32_little_endian(const std::vector<std::uint32_t>& values) {
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (const std::uint32_t value : values) {
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
      const auto byte = static_cast<std::uint8_t>((value >> shift) & UINT32_C(0xff));
      hash ^= byte;
      hash *= UINT64_C(1099511628211);
    }
  }
  return hash;
}

std::string hexadecimal_u64(std::uint64_t value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::array<char, 16> digits{};
  for (std::size_t index = digits.size(); index > 0U; --index) {
    digits[index - 1U] = kHex[value & UINT64_C(0x0f)];
    value >>= 4U;
  }
  return std::string{digits.data(), digits.size()};
}

struct RunEvidence {
  Measurement warm;
  Measurement resident;
  std::size_t element_count = 0U;
  std::uint32_t warm_sum = 0U;
  std::uint32_t resident_sum = 0U;
  std::uint32_t cpu_reference_sum = 0U;
  std::string output_hash;
  std::string cpu_reference_hash;
  bool bitwise_deterministic = false;
  bool cpu_reference_equal = false;
  bool dlpack_zero_copy = false;
  bool dlpack_device_identity = false;
  std::size_t free_bytes_before = 0U;
  std::size_t free_bytes_after = 0U;
  std::size_t peak_live_bytes = 0U;
  std::size_t final_live_bytes = 0U;
  std::size_t allocation_count = 0U;
  std::size_t release_count = 0U;
};

JsonObject expected_structured_error(int device) {
  cudaDeviceProp unused{};
  const cudaError_t result = cudaGetDeviceProperties(&unused, -1);
  static_cast<void>(cudaGetLastError());
  int device_after = -1;
  check_cuda(cudaGetDevice(&device_after), "cudaGetDevice(after structured error)");
  check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize(after structured error)");
  const bool expected = result == cudaErrorInvalidDevice;
  const bool healthy = device_after == device;
  if (!expected || !healthy) {
    throw std::runtime_error(
        "the recoverable structured CUDA error exercise did not meet its contract");
  }
  return cuda_error_json(result, "cudaGetDeviceProperties(device=-1)", true, true);
}

RunEvidence execute_runtime(const Options& options,
                            const DeviceManifest& manifest,
                            cudaStream_t stream) {
  RunEvidence evidence;
  std::size_t total_bytes = 0U;
  check_cuda(cudaMemGetInfo(&evidence.free_bytes_before, &total_bytes),
             "cudaMemGetInfo(before)");
  const std::size_t safe_total_limit = total_bytes - total_bytes / 5U;
  const std::size_t safe_free_limit =
      evidence.free_bytes_before - evidence.free_bytes_before / 5U;
  if (options.allocation_ceiling > std::min(safe_total_limit, safe_free_limit)) {
    throw std::invalid_argument(
        "the allocation ceiling exceeds 80 percent of total or currently free VRAM");
  }

  AsyncMemoryTracker memory(manifest.device, stream, options.allocation_ceiling);
  std::size_t element_count =
      std::min(kMaximumElements, options.allocation_ceiling / (sizeof(std::uint32_t) * 16U));
  std::size_t cub_temporary_bytes = 0U;
  constexpr std::size_t arena_alignment = 256U;
  const auto align_up = [](std::size_t offset, std::size_t alignment) {
    const std::size_t remainder = offset % alignment;
    return remainder == 0U ? offset : offset + alignment - remainder;
  };
  std::size_t first_offset = 0U;
  std::size_t second_offset = 0U;
  std::size_t first_sum_offset = 0U;
  std::size_t second_sum_offset = 0U;
  std::size_t cub_temporary_offset = 0U;
  std::size_t required_bytes = 0U;
  while (true) {
    cub_temporary_bytes = 0U;
    check_cuda(
        cub::DeviceReduce::Sum(
            nullptr, cub_temporary_bytes, static_cast<const std::uint32_t*>(nullptr),
            static_cast<std::uint32_t*>(nullptr), element_count, stream),
        "cub::DeviceReduce::Sum(size query)");
    std::size_t offset = 0U;
    first_offset = align_up(offset, alignof(std::uint32_t));
    offset = first_offset + element_count * sizeof(std::uint32_t);
    second_offset = align_up(offset, alignof(std::uint32_t));
    offset = second_offset + element_count * sizeof(std::uint32_t);
    first_sum_offset = align_up(offset, alignof(std::uint32_t));
    offset = first_sum_offset + sizeof(std::uint32_t);
    second_sum_offset = align_up(offset, alignof(std::uint32_t));
    offset = second_sum_offset + sizeof(std::uint32_t);
    cub_temporary_offset = align_up(offset, arena_alignment);
    required_bytes = cub_temporary_offset + cub_temporary_bytes;
    if (required_bytes <= options.allocation_ceiling) {
      break;
    }
    if (element_count <= 1024U) {
      throw std::runtime_error("the allocation ceiling is too small for the CUB exercise");
    }
    element_count /= 2U;
  }
  evidence.element_count = element_count;

  void* arena = nullptr;
  {
    NvtxRange range{"phase3/single_async_arena"};
    arena = memory.allocate(options.allocation_ceiling, "single Phase 3 arena");
    check_cuda(cudaMemsetAsync(arena, 0, options.allocation_ceiling, stream),
               "cudaMemsetAsync(single Phase 3 arena)");
  }
  auto* arena_bytes = static_cast<std::byte*>(arena);
  auto* first = reinterpret_cast<std::uint32_t*>(arena_bytes + first_offset);
  auto* second = reinterpret_cast<std::uint32_t*>(arena_bytes + second_offset);
  auto* first_sum = reinterpret_cast<std::uint32_t*>(arena_bytes + first_sum_offset);
  auto* second_sum = reinterpret_cast<std::uint32_t*>(arena_bytes + second_sum_offset);
  void* cub_temporary = arena_bytes + cub_temporary_offset;
  static_cast<void>(required_bytes);

  std::int64_t dlpack_shape[1] = {static_cast<std::int64_t>(element_count)};
  std::int64_t dlpack_strides[1] = {1};
  DLTensor tensor{};
  tensor.data = first;
  tensor.device = DLDevice{kDLCUDA, manifest.device};
  tensor.ndim = 1;
  tensor.dtype = DLDataType{static_cast<std::uint8_t>(kDLUInt),
                            static_cast<std::uint8_t>(32U),
                            static_cast<std::uint16_t>(1U)};
  tensor.shape = dlpack_shape;
  tensor.strides = dlpack_strides;
  tensor.byte_offset = 0U;
  const auto* dlpack_pointer =
      static_cast<const std::byte*>(tensor.data) + tensor.byte_offset;
  evidence.dlpack_zero_copy =
      dlpack_pointer == reinterpret_cast<const std::byte*>(first);
  cudaPointerAttributes attributes{};
  check_cuda(cudaPointerGetAttributes(&attributes, tensor.data),
             "cudaPointerGetAttributes(DLPack tensor)");
  evidence.dlpack_device_identity =
      tensor.device.device_type == kDLCUDA && tensor.device.device_id == manifest.device &&
      tensor.strides != nullptr && tensor.strides[0] == 1 &&
      attributes.type == cudaMemoryTypeDevice && attributes.device == manifest.device;

  constexpr unsigned int block_size = 256U;
  const auto grid_size_unsigned = static_cast<unsigned int>(
      (element_count + block_size - 1U) / block_size);
  const dim3 block{block_size, 1U, 1U};
  const dim3 grid{grid_size_unsigned, 1U, 1U};
  const auto launch_and_reduce = [&](std::uint32_t* output, std::uint32_t* sum) {
    morsehgp3d_phase3_deterministic_kernel<<<grid, block, 0U, stream>>>(
        output, element_count, kKernelSeed);
    check_cuda(cudaGetLastError(), "morsehgp3d_phase3_deterministic_kernel launch");
    check_cuda(cub::DeviceReduce::Sum(cub_temporary, cub_temporary_bytes, output, sum,
                                      element_count, stream),
               "cub::DeviceReduce::Sum(execution)");
  };

  {
    NvtxRange range{"phase3/aot_warmup"};
    launch_and_reduce(first, first_sum);
    check_cuda(cudaStreamSynchronize(stream), "cudaStreamSynchronize(AOT warmup)");
  }
  {
    NvtxRange range{"phase3/warm"};
    evidence.warm = measure("warm", stream, [&] { launch_and_reduce(first, first_sum); });
  }
  {
    NvtxRange range{"phase3/resident"};
    evidence.resident =
        measure("resident", stream, [&] { launch_and_reduce(second, second_sum); });
  }

  std::vector<std::uint32_t> first_host(element_count);
  std::vector<std::uint32_t> second_host(element_count);
  {
    NvtxRange range{"phase3/bitwise_verification"};
    check_cuda(cudaMemcpyAsync(first_host.data(), first,
                               element_count * sizeof(std::uint32_t),
                               cudaMemcpyDeviceToHost, stream),
               "cudaMemcpyAsync(first deterministic output)");
    check_cuda(cudaMemcpyAsync(second_host.data(), second,
                               element_count * sizeof(std::uint32_t),
                               cudaMemcpyDeviceToHost, stream),
               "cudaMemcpyAsync(second deterministic output)");
    check_cuda(cudaMemcpyAsync(&evidence.warm_sum, first_sum, sizeof(std::uint32_t),
                               cudaMemcpyDeviceToHost, stream),
               "cudaMemcpyAsync(first CUB sum)");
    check_cuda(cudaMemcpyAsync(&evidence.resident_sum, second_sum,
                               sizeof(std::uint32_t), cudaMemcpyDeviceToHost, stream),
               "cudaMemcpyAsync(second CUB sum)");
    check_cuda(cudaStreamSynchronize(stream), "cudaStreamSynchronize(verification)");
  }
  evidence.bitwise_deterministic =
      first_host == second_host && evidence.warm_sum == evidence.resident_sum;
  evidence.output_hash = hexadecimal_u64(fnv1a_u32_little_endian(first_host));
  std::vector<std::uint32_t> cpu_reference(element_count);
  for (std::size_t index = 0U; index < element_count; ++index) {
    const std::uint32_t value = phase3_cpu_reference_value(index);
    cpu_reference[index] = value;
    evidence.cpu_reference_sum += value;
  }
  evidence.cpu_reference_hash =
      hexadecimal_u64(fnv1a_u32_little_endian(cpu_reference));
  evidence.cpu_reference_equal =
      first_host == cpu_reference && second_host == cpu_reference &&
      evidence.warm_sum == evidence.cpu_reference_sum &&
      evidence.resident_sum == evidence.cpu_reference_sum &&
      evidence.output_hash == evidence.cpu_reference_hash;

  {
    NvtxRange range{"phase3/free_and_trim"};
    memory.release(arena, "single Phase 3 arena");
    memory.synchronize_and_trim();
  }
  evidence.peak_live_bytes = memory.peak_bytes();
  evidence.final_live_bytes = memory.live_bytes();
  evidence.allocation_count = memory.allocation_count();
  evidence.release_count = memory.release_count();
  std::size_t ignored_total = 0U;
  check_cuda(cudaMemGetInfo(&evidence.free_bytes_after, &ignored_total),
             "cudaMemGetInfo(after)");
  return evidence;
}

JsonObject allocation_json(const Options& options, const RunEvidence& evidence) {
  JsonObject allocation;
  allocation.add_integer("allocation_count", evidence.allocation_count);
  allocation.add_integer("configured_ceiling_bytes", options.allocation_ceiling);
  allocation.add_integer("exact_async_allocation_bytes", options.allocation_ceiling);
  allocation.add_integer("free_bytes_after", evidence.free_bytes_after);
  allocation.add_integer("free_bytes_before", evidence.free_bytes_before);
  const auto free_delta = static_cast<std::int64_t>(evidence.free_bytes_after) -
                          static_cast<std::int64_t>(evidence.free_bytes_before);
  allocation.add_integer("free_delta_bytes", free_delta);
  allocation.add_bool("leak_free", evidence.final_live_bytes == 0U);
  allocation.add_integer("live_bytes_final", evidence.final_live_bytes);
  allocation.add_integer("peak_live_bytes", evidence.peak_live_bytes);
  allocation.add_integer("release_count", evidence.release_count);
  allocation.add_bool("single_async_arena", true);
  allocation.add_bool("suballocated_without_extra_cuda_allocations", true);
  allocation.add_bool("trimmed_default_pool", true);
  allocation.add_bool("within_configured_ceiling",
                      evidence.peak_live_bytes <= options.allocation_ceiling);
  return allocation;
}

JsonObject determinism_json(const RunEvidence& evidence) {
  JsonObject determinism;
  determinism.add_bool("bitwise_equal", evidence.bitwise_deterministic);
  determinism.add_bool("cpu_reference_equal", evidence.cpu_reference_equal);
  determinism.add_string("cpu_reference_fnv1a64_u32_le",
                         evidence.cpu_reference_hash);
  determinism.add_integer("cpu_reference_sum", evidence.cpu_reference_sum);
  determinism.add_integer("element_count", evidence.element_count);
  determinism.add_string("kernel", "morsehgp3d_phase3_deterministic_kernel");
  determinism.add_integer("measured_kernel_runs_compared", 2);
  determinism.add_string("output_fnv1a64_u32_le", evidence.output_hash);
  determinism.add_integer("resident_cub_sum", evidence.resident_sum);
  determinism.add_bool("uses_cub_device_reduce", true);
  determinism.add_integer("warm_cub_sum", evidence.warm_sum);
  return determinism;
}

JsonObject dlpack_json(const RunEvidence& evidence) {
  JsonObject dlpack;
  dlpack.add_integer("byte_offset", 0);
  dlpack.add_integer("copy_operations", 0);
  dlpack.add_bool("device_identity", evidence.dlpack_device_identity);
  dlpack.add_bool("pointer_identity", evidence.dlpack_zero_copy);
  dlpack.add_bool("zero_copy", evidence.dlpack_zero_copy && evidence.dlpack_device_identity);
  return dlpack;
}

JsonObject duration_json(const Measurement& measurement) {
  JsonObject duration;
  duration.add_integer("gpu_ns", measurement.gpu_duration_ns);
  duration.add_integer("host_ns", measurement.host_duration_ns);
  return duration;
}

JsonObject result_json(const Options& options,
                       const DeviceManifest& manifest,
                       const RunEvidence& evidence,
                       const Measurement& measurement,
                       const std::optional<JsonObject>& structured_error) {
  JsonObject result;
  result.add_object("allocation", allocation_json(options, evidence));
  result.add_bool("compilation_during_measurement", false);
  result.add_object("determinism", determinism_json(evidence));
  result.add_object("dlpack", dlpack_json(evidence));
  result.add_object("duration", duration_json(measurement));
  result.add_string("kind", "phase3_runtime_result");
  result.add_object("manifest", manifest_json(manifest));
  result.add_string("protocol", measurement.protocol);
  result.add_string("schema", kSchema);
  result.add_string("status", "ok");
  if (structured_error.has_value()) {
    result.add_object("structured_cuda_error", *structured_error);
  } else {
    result.add_null("structured_cuda_error");
  }
  result.add_string("timestamp_end_utc", measurement.end_utc);
  result.add_string("timestamp_start_utc", measurement.start_utc);
  return result;
}

JsonObject structured_error_result_json(const DeviceManifest& manifest,
                                        const JsonObject& structured_error) {
  JsonObject result;
  result.add_bool("compilation_during_measurement", false);
  result.add_object("cuda_error", structured_error);
  result.add_string("kind", "phase3_structured_cuda_error_result");
  result.add_object("manifest", manifest_json(manifest));
  result.add_string("schema", kSchema);
  result.add_string("status", "ok");
  result.add_string("timestamp_utc", utc_timestamp());
  return result;
}

JsonObject fallback_manifest_json() {
  JsonObject manifest;
  const std::string image_id = environment_value("MORSEHGP3D_CUDA_IMAGE_ID");
  const std::string image_ref = environment_value("MORSEHGP3D_CUDA_IMAGE_REF");
  manifest.add_bool("aot_only", true);
  manifest.add_string("build_mode", build_mode_string());
  manifest.add_bool("complete", false);
  manifest.add_bool("compilation_during_measurement", false);
  manifest.add_integer("cuda_compiler_version", __CUDACC_VER_BUILD__ +
                                                  __CUDACC_VER_MINOR__ * 1000 +
                                                  __CUDACC_VER_MAJOR__ * 1000000);
  manifest.add_integer("dlpack_major_version", DLPACK_MAJOR_VERSION);
  manifest.add_integer("dlpack_minor_version", DLPACK_MINOR_VERSION);
  manifest.add_string("git_sha", MORSEHGP3D_GIT_SHA);
  manifest.add_string("image_id", image_id);
  manifest.add_string("image_ref", image_ref);
  manifest.add_bool("nvrtc_used", false);
  manifest.add_bool("scientific_result_claimed", false);
  manifest.add_null("scientific_public_status");
  return manifest;
}

JsonObject failure_json(std::string_view category,
                        std::string_view code,
                        std::string_view message,
                        const std::optional<JsonObject>& cuda_error) {
  JsonObject error;
  error.add_string("category", category);
  error.add_string("code", code);
  error.add_string("message", message);

  JsonObject result;
  result.add_bool("compilation_during_measurement", false);
  if (cuda_error.has_value()) {
    result.add_object("cuda_error", *cuda_error);
  } else {
    result.add_null("cuda_error");
  }
  result.add_object("error", error);
  result.add_string("kind", "phase3_runtime_error");
  result.add_object("manifest", fallback_manifest_json());
  result.add_string("schema", kSchema);
  result.add_string("status", "error");
  result.add_string("timestamp_utc", utc_timestamp());
  return result;
}

void write_jsonl(const std::vector<std::string>& records,
                 const std::optional<std::string>& path) {
  if (!path.has_value() || *path == "-") {
    for (const auto& record : records) {
      std::cout << record << '\n';
    }
    std::cout.flush();
    if (!std::cout) {
      throw std::runtime_error("failed to write JSONL to stdout");
    }
    return;
  }
  std::ofstream output{*path, std::ios::binary | std::ios::trunc};
  if (!output) {
    throw std::runtime_error("failed to open JSONL output: " + *path);
  }
  for (const auto& record : records) {
    output << record << '\n';
  }
  output.flush();
  if (!output) {
    throw std::runtime_error("failed to write JSONL output: " + *path);
  }
}

int run(const Options& options) {
  int device_count = 0;
  check_cuda(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount");
  if (options.device < 0 || options.device >= device_count) {
    throw std::invalid_argument("the requested CUDA device does not exist");
  }
  check_cuda(cudaSetDevice(options.device), "cudaSetDevice");
  check_cuda(cudaFree(nullptr), "cudaFree(nullptr) runtime initialization");
  int memory_pools_supported = 0;
  check_cuda(cudaDeviceGetAttribute(&memory_pools_supported,
                                    cudaDevAttrMemoryPoolsSupported,
                                    options.device),
             "cudaDeviceGetAttribute(cudaDevAttrMemoryPoolsSupported)");
  if (memory_pools_supported == 0) {
    throw std::runtime_error("the selected CUDA device does not support async memory pools");
  }

  const DeviceManifest manifest = collect_manifest(options.device);
  if (manifest.properties.major != 12 || manifest.properties.minor != 0) {
    throw std::runtime_error("the selected CUDA device is not compute capability 12.0");
  }
  if (!manifest.complete) {
    throw std::runtime_error(
        "the manifest requires MORSEHGP3D_GIT_SHA, MORSEHGP3D_CUDA_IMAGE_REF, and "
        "MORSEHGP3D_CUDA_IMAGE_ID");
  }

  cudaStream_t stream = nullptr;
  check_cuda(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking),
             "cudaStreamCreateWithFlags");
  try {
    const RunEvidence evidence = execute_runtime(options, manifest, stream);
    const std::optional<JsonObject> structured_error =
        options.exercise_structured_error
            ? std::optional<JsonObject>{expected_structured_error(options.device)}
            : std::nullopt;
    check_cuda(cudaStreamDestroy(stream), "cudaStreamDestroy");
    stream = nullptr;
    check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize(final)");

    if (!evidence.bitwise_deterministic || !evidence.cpu_reference_equal ||
        !evidence.dlpack_zero_copy ||
        !evidence.dlpack_device_identity || evidence.final_live_bytes != 0U ||
        evidence.peak_live_bytes != options.allocation_ceiling ||
        evidence.allocation_count != evidence.release_count) {
      throw std::runtime_error("a Phase 3 runtime invariant failed");
    }

    std::vector<std::string> records;
    records.push_back(
        result_json(options, manifest, evidence, evidence.warm, structured_error).serialize());
    records.push_back(result_json(options, manifest, evidence, evidence.resident,
                                  structured_error)
                          .serialize());
    if (structured_error.has_value()) {
      records.push_back(
          structured_error_result_json(manifest, *structured_error).serialize());
    }
    write_jsonl(records, options.output_path);
  } catch (...) {
    if (stream != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream));
    }
    throw;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::optional<std::string> output_path;
  try {
    const Options options = parse_options(argc, argv);
    output_path = options.output_path;
    if (options.show_help) {
      print_help(std::cout);
      return 0;
    }
    return run(options);
  } catch (const CudaFailure& failure) {
    try {
      const JsonObject structured =
          cuda_error_json(failure.code(), failure.operation(), false, false);
      const std::vector<std::string> records = {
          failure_json("cuda", encode_json_integer(static_cast<int>(failure.code())),
                       failure.what(), structured)
              .serialize()};
      write_jsonl(records, output_path);
    } catch (const std::exception& reporting_failure) {
      std::cerr << "failed to report CUDA failure: " << reporting_failure.what() << '\n';
    }
    return 2;
  } catch (const std::exception& failure) {
    try {
      const std::vector<std::string> records = {
          failure_json("runtime", "runtime_failure", failure.what(), std::nullopt)
              .serialize()};
      write_jsonl(records, output_path);
    } catch (const std::exception& reporting_failure) {
      std::cerr << "failed to report runtime failure: " << reporting_failure.what() << '\n';
    }
    return 2;
  }
}
