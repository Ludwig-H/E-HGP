#include "phase14_morton_window_knn_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/gpu/morton_yao48_seed_work_profile.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
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
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The Morton/Yao48 seed work profile requires a canonical Git SHA"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::spatial::PointId;

constexpr std::size_t kMaximumPointCount = 30'000'000U;
constexpr std::size_t kMaximumWindowRadius = 4096U;

struct Options {
  std::size_t point_count{50'000U};
  std::size_t maximum_order{10U};
  std::size_t morton_window_radius{64U};
  std::size_t warm_repetition_count{1U};
  std::size_t worker_count{1U};
  std::uint64_t maximum_directed_pair_check_count{UINT64_C(4000000000)};
  std::size_t maximum_neighbor_record_count{300'000'000U};
  std::uint64_t seed{UINT64_C(0x15a048c0ffee)};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role,
    bool allow_zero = false) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end ||
      (!allow_zero && value == 0U)) {
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
  if (result.ec != std::errc{} || result.ptr != end || value == 0U) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  const unsigned int hardware = std::thread::hardware_concurrency();
  options.worker_count =
      hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware);
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count =
          parse_size(argv[++index], "invalid --point-count");
    } else if (argument == "--max-order" && index + 1 < argc) {
      options.maximum_order =
          parse_size(argv[++index], "invalid --max-order");
    } else if (argument == "--morton-window" && index + 1 < argc) {
      options.morton_window_radius =
          parse_size(argv[++index], "invalid --morton-window");
    } else if (argument == "--warm-repetitions" && index + 1 < argc) {
      options.warm_repetition_count = parse_size(
          argv[++index], "invalid --warm-repetitions", true);
    } else if (argument == "--cpu-workers" && index + 1 < argc) {
      options.worker_count =
          parse_size(argv[++index], "invalid --cpu-workers");
    } else if (argument == "--maximum-directed-pair-checks" &&
               index + 1 < argc) {
      options.maximum_directed_pair_check_count = parse_u64(
          argv[++index], "invalid --maximum-directed-pair-checks");
    } else if (argument == "--maximum-neighbor-records" &&
               index + 1 < argc) {
      options.maximum_neighbor_record_count = parse_size(
          argv[++index], "invalid --maximum-neighbor-records");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed = parse_u64(argv[++index], "invalid --seed");
    } else {
      throw std::invalid_argument(
          "usage: morsehgp3d_gpu_morton_yao48_seed_work_profile "
          "[--point-count N] [--max-order K] [--morton-window W] "
          "[--warm-repetitions N] [--cpu-workers N] "
          "[--maximum-directed-pair-checks N] "
          "[--maximum-neighbor-records N] [--seed N]");
    }
  }
  if (options.point_count > kMaximumPointCount ||
      options.point_count > static_cast<std::size_t>(INT_MAX) ||
      options.maximum_order == 0U ||
      options.maximum_order >
          morsehgp3d::gpu::
              morton_yao48_seed_work_profile_maximum_bank_capacity ||
      options.point_count <= options.maximum_order ||
      options.morton_window_radius < options.maximum_order ||
      options.morton_window_radius > kMaximumWindowRadius) {
    throw std::invalid_argument(
        "the bounded profile requires K <= 10, n <= 30000000, n > K and "
        "K <= W <= 4096");
  }
  return options;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::uint64_t checked_u64_product(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::uint64_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::uint64_t directed_window_pair_checks(
    std::size_t point_count,
    std::size_t radius) {
  const std::uint64_t count = static_cast<std::uint64_t>(point_count);
  const std::uint64_t window = static_cast<std::uint64_t>(
      std::min(radius, point_count - 1U));
  return checked_u64_product(
      window,
      2U * count - window - 1U,
      "the directed Morton-window work count overflows uint64");
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
  std::size_t multiplier = static_cast<std::size_t>(
      UINT64_C(2654435761) % static_cast<std::uint64_t>(point_count));
  multiplier = std::max(multiplier, std::size_t{1});
  while (std::gcd(multiplier, point_count) != 1U) {
    ++multiplier;
  }
  return multiplier;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_affine_uniform_cloud(const Options& options) {
  constexpr std::uint64_t kMantissaMask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t kOneExponent = UINT64_C(0x3ff0000000000000);
  const std::uint64_t count = static_cast<std::uint64_t>(options.point_count);
  const std::uint64_t x_step =
      std::max(UINT64_C(1), kMantissaMask / count);
  const std::size_t multiplier = permutation_multiplier(options.point_count);
  const std::size_t offset =
      static_cast<std::size_t>(options.seed % count);

  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(options.point_count);
  for (std::size_t source_index = 0U;
       source_index < options.point_count;
       ++source_index) {
    const std::size_t logical_index = static_cast<std::size_t>(
        (static_cast<std::uint64_t>(source_index) *
             static_cast<std::uint64_t>(multiplier) +
         static_cast<std::uint64_t>(offset)) %
        count);
    const std::uint64_t identity =
        static_cast<std::uint64_t>(logical_index);
    points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64_bits(
            {kOneExponent | (identity * x_step),
             kOneExponent |
                 (splitmix64(identity ^ options.seed) & kMantissaMask),
             kOneExponent |
                 (splitmix64(identity ^ std::rotl(options.seed, 23)) &
                  kMantissaMask)}));
  }
  return points;
}

template <typename Operation>
void parallel_shards(
    std::size_t item_count,
    std::size_t worker_count,
    Operation operation) {
  const std::size_t effective_workers =
      std::max(std::size_t{1}, std::min(item_count, worker_count));
  std::vector<std::thread> workers;
  workers.reserve(effective_workers);
  for (std::size_t worker = 0U; worker < effective_workers; ++worker) {
    workers.emplace_back([&, worker] {
      operation(
          item_count * worker / effective_workers,
          item_count * (worker + 1U) / effective_workers);
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (value < 0) {
    throw std::runtime_error("the work-profile monotonic clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t peak_host_rss_bytes() {
#if defined(__linux__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
    throw std::runtime_error("getrusage could not read peak RSS");
  }
  return static_cast<std::uint64_t>(usage.ru_maxrss) * UINT64_C(1024);
#else
  return 0U;
#endif
}

[[nodiscard]] std::string json_escape(std::string_view value) {
  constexpr char kHex[] = "0123456789abcdef";
  std::string escaped;
  escaped.reserve(value.size());
  for (const char raw_character : value) {
    const auto character = static_cast<unsigned char>(raw_character);
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (character < 0x20U) {
          escaped += "\\u00";
          escaped.push_back(kHex[character >> 4U]);
          escaped.push_back(kHex[character & 0x0fU]);
        } else {
          escaped.push_back(static_cast<char>(character));
        }
    }
  }
  return escaped;
}

struct RunMeasurement {
  morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult knn;
  morsehgp3d::gpu::MortonYao48SeedWorkProfile profile;
  std::uint64_t launcher_wall_nanoseconds{};
  std::uint64_t bank_aggregation_nanoseconds{};
  std::uint64_t end_to_end_nanoseconds{};
};

[[nodiscard]] RunMeasurement run_once(
    const Options& options,
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    const std::vector<double>& coordinates_by_morton_position,
    const std::vector<PointId>& point_ids_by_morton_position) {
  const auto begin = Clock::now();
  const auto launcher_begin = Clock::now();
  auto knn = morsehgp3d::gpu::detail::
      run_phase14_morton_window_knn_on_gpu(
          coordinates_by_morton_position,
          point_ids_by_morton_position,
          options.maximum_order,
          options.morton_window_radius);
  const auto launcher_end = Clock::now();
  const auto aggregation_begin = Clock::now();
  auto profile =
      morsehgp3d::gpu::summarize_morton_yao48_seed_work_profile(
          cloud,
          point_ids_by_morton_position,
          knn.neighbor_point_ids,
          options.maximum_order,
          options.worker_count);
  const auto end = Clock::now();
  return RunMeasurement{
      std::move(knn),
      std::move(profile),
      nanoseconds(launcher_end - launcher_begin),
      nanoseconds(end - aggregation_begin),
      nanoseconds(end - begin)};
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const std::size_t neighbor_record_count = checked_product(
        options.point_count,
        options.maximum_order,
        "the neighbor-record count overflows size_t");
    const std::uint64_t directed_pair_check_count =
        directed_window_pair_checks(
            options.point_count, options.morton_window_radius);
    if (neighbor_record_count > options.maximum_neighbor_record_count ||
        directed_pair_check_count >
            options.maximum_directed_pair_check_count) {
      throw std::length_error(
          "the requested run exceeds an explicit work-profile cap");
    }

    const auto total_begin = Clock::now();
    const auto generation_begin = Clock::now();
    auto input = make_affine_uniform_cloud(options);
    const auto generation_end = Clock::now();
    const auto canonicalization_begin = Clock::now();
    auto cloud =
        morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(input);
    const auto canonicalization_end = Clock::now();
    input.clear();
    input.shrink_to_fit();

    const auto build_begin = Clock::now();
    morsehgp3d::gpu::MortonLbvhBuildContext build_context{
        options.point_count};
    auto build_result = build_context.build(cloud);
    const auto build_end = Clock::now();
    if (!build_result.cuda_qualified_build() ||
        !build_result.certified_index().validated_for(cloud)) {
      throw std::runtime_error(
          "the work profile requires a CUDA-qualified certified Morton LBVH");
    }
    const auto build_audit = build_result.audit();
    const auto& leaves = build_result.certified_index().leaves();

    const auto pack_begin = Clock::now();
    std::vector<PointId> point_ids_by_morton_position(options.point_count);
    std::vector<double> coordinates_by_morton_position(
        checked_product(
            options.point_count,
            std::size_t{3},
            "the packed coordinate count overflows size_t"));
    parallel_shards(
        options.point_count,
        options.worker_count,
        [&](std::size_t begin, std::size_t end) {
          for (std::size_t position = begin; position < end; ++position) {
            const PointId point_id = leaves[position].point_id;
            point_ids_by_morton_position[position] = point_id;
            for (std::size_t axis = 0U; axis < 3U; ++axis) {
              coordinates_by_morton_position[position * 3U + axis] =
                  cloud.point(point_id).binary64_coordinate(axis);
            }
          }
        });
    const auto pack_end = Clock::now();

    const auto lease_begin = Clock::now();
    auto traversal_lease = build_context.release_device_traversal_lease(
        build_result);
    const auto lease_end = Clock::now();
    if (!traversal_lease.ready() || !traversal_lease.cuda_resident()) {
      throw std::runtime_error(
          "the work profile did not retain the certified traversal lease");
    }
    const auto lease_audit = traversal_lease.audit();

    RunMeasurement measurement = run_once(
        options, cloud, coordinates_by_morton_position,
        point_ids_by_morton_position);
    const std::uint64_t cold_end_to_end_nanoseconds =
        nanoseconds(Clock::now() - total_begin);
    std::vector<std::uint64_t> warm_end_to_end;
    std::vector<std::uint64_t> warm_kernel;
    warm_end_to_end.reserve(options.warm_repetition_count);
    warm_kernel.reserve(options.warm_repetition_count);
    for (std::size_t repetition = 0U;
         repetition < options.warm_repetition_count;
         ++repetition) {
      RunMeasurement replay = run_once(
          options, cloud, coordinates_by_morton_position,
          point_ids_by_morton_position);
      if (replay.profile != measurement.profile) {
        throw std::runtime_error(
            "the warm Morton/Yao48 seed replay changed its counters");
      }
      warm_end_to_end.push_back(replay.end_to_end_nanoseconds);
      warm_kernel.push_back(replay.knn.kernel_nanoseconds);
      measurement = std::move(replay);
    }
    const auto median = [](std::vector<std::uint64_t> values) {
      if (values.empty()) {
        return UINT64_C(0);
      }
      std::sort(values.begin(), values.end());
      return values[(values.size() - 1U) / 2U];
    };
    const std::uint64_t warm_e2e_nanoseconds = median(warm_end_to_end);
    const std::uint64_t warm_kernel_nanoseconds = median(warm_kernel);
    const std::size_t peak_device_capacity =
        lease_audit.persistent_device_byte_capacity >
                std::numeric_limits<std::size_t>::max() -
                    measurement.knn.device_byte_capacity
            ? throw std::length_error("the peak device capacity overflows")
            : lease_audit.persistent_device_byte_capacity +
                  measurement.knn.device_byte_capacity;

    const auto& profile = measurement.profile;
    std::cout
        << "{\"schema\":\"morsehgp3d.phase15.morton_yao48_seed_work_profile.v1\","
        << "\"artifact_role\":\"benchmark_only\","
        << "\"git_sha\":\"" << MORSEHGP3D_GIT_SHA << "\","
        << "\"phase\":15,\"backend\":\"cuda_g4_plus_host_binary64\","
        << "\"profile\":\"hgp_reduced\","
        << "\"mode\":\"bounded_morton_window_yao48_seed_proxy\","
        << "\"scientific_status\":\""
        << morsehgp3d::gpu::morton_yao48_seed_work_profile_scientific_status
        << "\",\"input_family\":\"affine_uniform_binary64\","
        << "\"seed\":" << options.seed << ','
        << "\"point_count\":" << options.point_count << ','
        << "\"maximum_order\":" << options.maximum_order << ','
        << "\"morton_window_radius\":" << options.morton_window_radius << ','
        << "\"warm_repetition_count\":"
        << options.warm_repetition_count << ','
        << "\"caps\":{\"maximum_point_count\":" << kMaximumPointCount
        << ",\"maximum_morton_window_radius\":" << kMaximumWindowRadius
        << ",\"maximum_directed_pair_checks\":"
        << options.maximum_directed_pair_check_count
        << ",\"maximum_neighbor_records\":"
        << options.maximum_neighbor_record_count << "},"
        << "\"work\":{\"directed_morton_window_pair_checks\":"
        << directed_pair_check_count
        << ",\"neighbor_records\":" << neighbor_record_count
        << ",\"lbvh_node_visits\":null,\"lbvh_strict_prunes\":null},"
        << "\"banks\":{\"point_cone_bank_count\":"
        << profile.point_cone_bank_count
        << ",\"receipt_count\":" << profile.bank_receipt_count
        << ",\"nonempty_count\":" << profile.nonempty_bank_count
        << ",\"cutoff_ready_count\":" << profile.cutoff_ready_bank_count
        << ",\"cutoff_applied_count\":0,\"fill_histogram\":[";
    for (std::size_t fill = 0U; fill <= options.maximum_order; ++fill) {
      if (fill != 0U) {
        std::cout << ',';
      }
      std::cout << profile.bank_fill_histogram[fill];
    }
    std::cout
        << "]},\"pair_mass\":{\"universe\":"
        << profile.unordered_pair_universe_count
        << ",\"survivors\":" << profile.survivor_pair_mass
        << ",\"certified_pruned\":"
        << profile.certified_pruned_pair_mass
        << ",\"unresolved\":" << profile.unresolved_pair_mass
        << ",\"identity_closed\":"
        << (profile.locally_valid() ? "true" : "false") << "},"
        << "\"timings_ns\":{\"generation\":"
        << nanoseconds(generation_end - generation_begin)
        << ",\"canonicalization\":"
        << nanoseconds(canonicalization_end - canonicalization_begin)
        << ",\"morton_lbvh_build_wall\":"
        << nanoseconds(build_end - build_begin)
        << ",\"morton_pack\":" << nanoseconds(pack_end - pack_begin)
        << ",\"traversal_lease_release\":"
        << nanoseconds(lease_end - lease_begin)
        << ",\"seed_allocation\":"
        << measurement.knn.allocation_nanoseconds
        << ",\"seed_h2d\":" << measurement.knn.host_to_device_nanoseconds
        << ",\"seed_kernel\":" << measurement.knn.kernel_nanoseconds
        << ",\"seed_d2h\":" << measurement.knn.device_to_host_nanoseconds
        << ",\"seed_launcher_wall\":"
        << measurement.launcher_wall_nanoseconds
        << ",\"yao48_bank_aggregation\":"
        << measurement.bank_aggregation_nanoseconds
        << ",\"cold_e2e\":" << cold_end_to_end_nanoseconds
        << ",\"warm_e2e_median\":" << warm_e2e_nanoseconds
        << ",\"warm_kernel_median\":" << warm_kernel_nanoseconds << "},"
        << "\"memory_bytes\":{\"build_fixed_device_capacity\":"
        << build_audit.total_fixed_device_byte_capacity
        << ",\"traversal_lease_persistent_device_capacity\":"
        << lease_audit.persistent_device_byte_capacity
        << ",\"seed_kernel_device_capacity\":"
        << measurement.knn.device_byte_capacity
        << ",\"estimated_peak_live_device_capacity\":"
        << peak_device_capacity
        << ",\"peak_host_rss\":" << peak_host_rss_bytes() << "},"
        << "\"cuda\":{\"device\":" << measurement.knn.cuda_device
        << ",\"device_name\":\"" << json_escape(measurement.knn.device_name)
        << "\",\"multiprocessor_count\":"
        << measurement.knn.multiprocessor_count
        << ",\"seed_kernel_block_count\":"
        << measurement.knn.kernel_block_count
        << ",\"seed_kernel_thread_count\":"
        << measurement.knn.kernel_thread_count << "},"
        << "\"claims\":{\"bounded_window_work_linear_in_n\":true,"
        << "\"dense_pair_fallback_performed\":false,"
        << "\"global_pair_matrix_materialized\":false,"
        << "\"delaunay_materialized\":false,"
        << "\"geogram_invoked\":false,\"pdel_invoked\":false,"
        << "\"emst_or_boruvka_invoked\":false,"
        << "\"exact_yao48_classification_claimed\":false,"
        << "\"exact_pair_frontier_claimed\":false,"
        << "\"candidate_recall_certified\":false,"
        << "\"frontier_cuda_kernel_implemented\":false,"
        << "\"scientific_exactness_claimed\":false,"
        << "\"scalability_claimed\":false,"
        << "\"public_status_claimed\":false},"
        << "\"timeout_contract\":{\"external_timeout_required\":true,"
        << "\"internal_kernel_preemption_available\":false,"
        << "\"timeout_yields_no_artifact\":true}}" << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
