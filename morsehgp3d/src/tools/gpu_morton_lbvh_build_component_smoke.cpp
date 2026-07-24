#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The Phase 14M component smoke requires a canonical Git SHA"
#endif

namespace {

struct Options {
  std::size_t point_count{50'000U};
  std::size_t repetition_count{1U};
  std::uint64_t seed{UINT64_C(0x14d3a5c79b)};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || value == 0U) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(
    std::string_view text,
    const char* role) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count =
          parse_size(argv[++index], "invalid --point-count");
    } else if (argument == "--repetitions" && index + 1 < argc) {
      options.repetition_count =
          parse_size(argv[++index], "invalid --repetitions");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed = parse_u64(argv[++index], "invalid --seed");
    } else {
      throw std::invalid_argument(
          "usage: gpu_morton_lbvh_build_component_smoke "
          "[--point-count N] [--repetitions N] [--seed N]");
    }
  }
  if (options.point_count >
      static_cast<std::size_t>(INT_MAX)) {
    throw std::length_error(
        "the bounded component smoke exceeds the CUB int item-count limit");
  }
  return options;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value =
      (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value =
      (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] std::size_t permutation_multiplier(
    std::size_t point_count) {
  std::size_t multiplier =
      static_cast<std::size_t>(
          UINT64_C(2654435761) %
          static_cast<std::uint64_t>(point_count));
  multiplier = std::max(multiplier, std::size_t{1});
  while (std::gcd(multiplier, point_count) != 1U) {
    ++multiplier;
  }
  return multiplier;
}

[[nodiscard]] std::size_t permuted_index(
    std::size_t index,
    std::size_t multiplier,
    std::size_t offset,
    std::size_t modulus) {
  return static_cast<std::size_t>(
      (static_cast<std::uint64_t>(index) *
           static_cast<std::uint64_t>(multiplier) +
       static_cast<std::uint64_t>(offset)) %
      static_cast<std::uint64_t>(modulus));
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_affine_uniform_cloud(const Options& options) {
  constexpr std::uint64_t kMantissaMask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t kOneExponent = UINT64_C(0x3ff0000000000000);
  const std::uint64_t point_count =
      static_cast<std::uint64_t>(options.point_count);
  const std::uint64_t x_step =
      std::max(UINT64_C(1), kMantissaMask / point_count);
  const std::size_t multiplier =
      permutation_multiplier(options.point_count);
  const std::size_t offset =
      static_cast<std::size_t>(options.seed % point_count);

  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(options.point_count);
  for (std::size_t source_index = 0U;
       source_index < options.point_count;
       ++source_index) {
    const std::size_t logical_index = permuted_index(
        source_index,
        multiplier,
        offset,
        options.point_count);
    const std::uint64_t identity =
        static_cast<std::uint64_t>(logical_index);
    const std::uint64_t x_mantissa = identity * x_step;
    const std::uint64_t y_mantissa =
        splitmix64(identity ^ options.seed) & kMantissaMask;
    const std::uint64_t z_mantissa =
        splitmix64(
            identity ^ std::rotl(options.seed, 23)) &
        kMantissaMask;
    points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64_bits(
            {kOneExponent | x_mantissa,
             kOneExponent | y_mantissa,
             kOneExponent | z_mantissa}));
  }
  return points;
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          duration)
          .count();
  if (value < 0) {
    throw std::runtime_error(
        "the monotonic qualification clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t peak_host_rss_bytes() {
#if defined(__linux__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 ||
      usage.ru_maxrss < 0) {
    throw std::runtime_error(
        "getrusage could not read the qualification peak RSS");
  }
  constexpr std::uint64_t kBytesPerKibibyte = UINT64_C(1024);
  const std::uint64_t kibibytes =
      static_cast<std::uint64_t>(usage.ru_maxrss);
  if (kibibytes >
      std::numeric_limits<std::uint64_t>::max() /
          kBytesPerKibibyte) {
    throw std::overflow_error(
        "the qualification peak RSS overflows uint64 bytes");
  }
  return kibibytes * kBytesPerKibibyte;
#else
  return 0U;
#endif
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    using Clock = std::chrono::steady_clock;

    const auto generation_start = Clock::now();
    std::vector<morsehgp3d::exact::CertifiedPoint3> input =
        make_affine_uniform_cloud(options);
    const auto generation_end = Clock::now();

    const auto canonicalization_start = Clock::now();
    morsehgp3d::spatial::CanonicalPointCloud cloud =
        morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(
            input);
    const auto canonicalization_end = Clock::now();
    input.clear();
    input.shrink_to_fit();

    morsehgp3d::gpu::MortonLbvhBuildContext context{
        options.point_count};
    std::vector<std::uint64_t> build_nanoseconds;
    build_nanoseconds.reserve(options.repetition_count);
    morsehgp3d::gpu::MortonLbvhDeviceBuildAudit last_audit;
    for (std::size_t repetition = 0U;
         repetition < options.repetition_count;
         ++repetition) {
      const auto build_start = Clock::now();
      morsehgp3d::gpu::MortonLbvhDeviceBuildResult result =
          context.build(cloud);
      const auto build_end = Clock::now();
      if (!result.cuda_qualified_build() ||
          !result.certified_index().validated_for(cloud)) {
        throw std::runtime_error(
            "the Phase 14M qualification did not return a CUDA-qualified "
            "certified index");
      }
      build_nanoseconds.push_back(
          nanoseconds(build_end - build_start));
      last_audit = result.audit();
    }

    std::vector<std::uint64_t> ordered = build_nanoseconds;
    std::sort(ordered.begin(), ordered.end());
    const std::uint64_t median =
        ordered[(ordered.size() - 1U) / 2U];
    const std::uint64_t maximum = ordered.back();
    std::cout
        << "{\"schema\":\"morsehgp3d.phase14m.component_smoke.v1\","
        << "\"git_sha\":\"" << MORSEHGP3D_GIT_SHA << "\","
        << "\"backend\":\"cuda_g4\","
        << "\"profile\":\"hgp_reduced\","
        << "\"mode\":\"device_morton_lbvh_snapshot_import\","
        << "\"family\":\"affine_uniform_binary64\","
        << "\"seed\":" << options.seed << ','
        << "\"point_count\":" << options.point_count << ','
        << "\"repetitions\":" << options.repetition_count << ','
        << "\"generation_ns\":"
        << nanoseconds(generation_end - generation_start) << ','
        << "\"canonicalization_ns\":"
        << nanoseconds(
               canonicalization_end - canonicalization_start)
        << ','
        << "\"build_median_ns\":" << median << ','
        << "\"build_max_ns\":" << maximum << ','
        << "\"ambiguous_axis_count\":"
        << last_audit.device_ambiguous_axis_count << ','
        << "\"project_kernel_launch_count\":"
        << last_audit.device_kernel_launch_count << ','
        << "\"library_submission_count\":"
        << last_audit.device_library_submission_count << ','
        << "\"synchronization_count\":"
        << last_audit.device_synchronization_count << ','
        << "\"snapshot_bytes\":"
        << last_audit.required_snapshot_byte_count << ','
        << "\"device_sort_temporary_bytes\":"
        << last_audit.device_sort_temporary_byte_capacity << ','
        << "\"total_fixed_device_capacity_bytes\":"
        << last_audit.total_fixed_device_byte_capacity << ','
        << "\"peak_host_rss_bytes\":" << peak_host_rss_bytes() << ','
        << "\"cuda_qualified\":true,"
        << "\"warm_e2e_slo_claimed\":false,"
        << "\"massive_product_path_claimed\":false,"
        << "\"public_status_claimed\":false}"
        << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
