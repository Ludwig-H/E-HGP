#include "phase14_morton_window_knn_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/binary64_lbvh_top_k.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The Phase 14 heuristic diagnostic requires a canonical Git SHA"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::spatial::PointId;

constexpr std::size_t kMaximumSupportedOrder = 10U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kMaximumValidationDistanceEvaluationCount =
    250'000'000U;
constexpr std::size_t kMaximumQualityPairCount = 262'144U;

enum class KnnBackend : std::uint8_t {
  morton_window,
  binary64_lbvh,
};

struct Options {
  std::size_t point_count{50'000U};
  std::size_t maximum_order{10U};
  std::size_t morton_window_radius{256U};
  std::optional<std::size_t> reference_morton_window_radius;
  std::size_t validation_point_count{};
  std::size_t quality_pair_count{};
  bool resident_replay{false};
  std::size_t requested_cpu_workers{};
  std::uint64_t seed{UINT64_C(0x14a750c0ffee)};
  KnnBackend knn_backend{KnnBackend::morton_window};
};

struct SurrogateEdge {
  double squared_weight{};
  PointId first{};
  PointId second{};
};

struct MergeRecord {
  double squared_level{};
  PointId first{};
  PointId second{};
};

struct OrderSummary {
  std::size_t order{};
  std::size_t proposed_edge_count{};
  std::size_t unique_edge_count{};
  std::size_t materialized_merge_record_count{};
  std::size_t distinct_level_count{};
  std::size_t final_component_count{};
  double root_squared_level{};
  std::uint64_t digest{};
  std::uint64_t cpu_nanoseconds{};
  std::vector<MergeRecord> retained_quality_merges;
};

struct HierarchyRun {
  std::vector<OrderSummary> summaries;
  std::uint64_t edge_build_nanoseconds{};
  std::uint64_t hierarchy_reduction_nanoseconds{};
  std::size_t order_worker_count{};
};

struct NeighborComparison {
  std::vector<std::size_t> prefix_match_counts;
  std::vector<std::size_t> prefix_totals;
  std::size_t exact_rank_match_count{};
  std::size_t total_neighbor_count{};
};

struct ExhaustiveValidation {
  std::size_t query_count{};
  std::size_t distance_evaluation_count{};
  NeighborComparison comparison;
  std::uint64_t cpu_nanoseconds{};
};

struct ResidentReplayResult {
  morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult knn;
  std::optional<morsehgp3d::gpu::Binary64LbvhTopKAudit>
      binary64_lbvh_audit;
  HierarchyRun hierarchy;
  bool transcript_match{};
  std::uint64_t launcher_wall_nanoseconds{};
  std::uint64_t replay_total_nanoseconds{};
};

struct BarcodeQuality {
  std::size_t exact_level_bit_match_count{};
  std::size_t finite_death_count{};
  double sorted_death_level_l1_raw{};
  std::optional<double> sorted_death_level_l1_normalized_reference_l1;
  double sorted_death_level_linf_raw{};
  std::optional<double> sorted_death_level_linf_normalized_reference_root;
};

struct CopheneticQuality {
  std::size_t exact_level_bit_match_count{};
  std::size_t sampled_pair_count{};
  double mean_absolute_error_raw{};
  std::optional<double> mean_absolute_error_normalized_reference_root;
  double maximum_absolute_error_raw{};
  std::optional<double> maximum_absolute_error_normalized_reference_root;
  std::optional<double> pearson;
};

struct ClusterThresholdQuality {
  unsigned int quantile_percent{};
  double reference_squared_level{};
  std::size_t agreement_count{};
  std::size_t pair_count{};
  std::size_t both_same_cluster_count{};
  std::size_t both_different_cluster_count{};
  std::size_t fast_only_same_cluster_count{};
  std::size_t reference_only_same_cluster_count{};
};

struct OrderHierarchyQuality {
  std::size_t order{};
  BarcodeQuality barcode;
  CopheneticQuality cophenetic;
  std::vector<ClusterThresholdQuality> cluster_thresholds;
  std::size_t cluster_agreement_count{};
  std::size_t cluster_comparison_count{};
};

struct AggregateHierarchyQuality {
  BarcodeQuality barcode;
  CopheneticQuality cophenetic;
  std::size_t cluster_agreement_count{};
  std::size_t cluster_comparison_count{};
};

struct HierarchyQuality {
  std::uint64_t sample_seed{};
  std::size_t sampled_pair_count{};
  std::vector<OrderHierarchyQuality> orders;
  AggregateHierarchyQuality aggregate;
  std::uint64_t cpu_nanoseconds{};
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

[[nodiscard]] std::size_t parse_nonnegative_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
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
  const unsigned int hardware = std::thread::hardware_concurrency();
  options.requested_cpu_workers =
      hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware);
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count = parse_size(argv[++index], "invalid --point-count");
    } else if (argument == "--max-order" && index + 1 < argc) {
      options.maximum_order = parse_size(argv[++index], "invalid --max-order");
    } else if (argument == "--morton-window" && index + 1 < argc) {
      options.morton_window_radius =
          parse_size(argv[++index], "invalid --morton-window");
    } else if (argument == "--knn-backend" && index + 1 < argc) {
      const std::string_view value{argv[++index]};
      if (value == "morton-window") {
        options.knn_backend = KnnBackend::morton_window;
      } else if (value == "binary64-lbvh") {
        options.knn_backend = KnnBackend::binary64_lbvh;
      } else {
        throw std::invalid_argument(
            "--knn-backend must be morton-window or binary64-lbvh");
      }
    } else if (argument == "--reference-morton-window" && index + 1 < argc) {
      options.reference_morton_window_radius =
          parse_size(argv[++index], "invalid --reference-morton-window");
    } else if (argument == "--validation-points" && index + 1 < argc) {
      options.validation_point_count =
          parse_nonnegative_size(argv[++index], "invalid --validation-points");
    } else if (argument == "--quality-pairs" && index + 1 < argc) {
      options.quality_pair_count =
          parse_nonnegative_size(argv[++index], "invalid --quality-pairs");
    } else if (argument == "--resident-replay") {
      options.resident_replay = true;
    } else if (argument == "--cpu-workers" && index + 1 < argc) {
      options.requested_cpu_workers =
          parse_size(argv[++index], "invalid --cpu-workers");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed = parse_u64(argv[++index], "invalid --seed");
    } else {
      throw std::invalid_argument(
          "usage: gpu_morton_window_h0_surrogate [--point-count N] "
          "[--max-order K] [--morton-window W] "
          "[--knn-backend morton-window|binary64-lbvh] [--cpu-workers N] "
          "[--reference-morton-window W2] [--validation-points N] "
          "[--quality-pairs Q] [--resident-replay] [--seed N]");
    }
  }
  if (options.maximum_order > kMaximumSupportedOrder ||
      options.point_count <= options.maximum_order ||
      options.morton_window_radius < options.maximum_order) {
    throw std::invalid_argument(
        "the heuristic requires 1 <= K <= 10, n > K and W >= K");
  }
  if (options.reference_morton_window_radius.has_value() &&
      *options.reference_morton_window_radius <=
          options.morton_window_radius) {
    throw std::invalid_argument(
        "--reference-morton-window must be strictly larger than the fast window");
  }
  if (options.validation_point_count > options.point_count) {
    throw std::invalid_argument(
        "--validation-points cannot exceed --point-count");
  }
  if (options.validation_point_count != 0U &&
      options.point_count - 1U >
          kMaximumValidationDistanceEvaluationCount /
              options.validation_point_count) {
    throw std::invalid_argument(
        "--validation-points would exceed the 250000000-distance hard cap");
  }
  if (options.quality_pair_count > kMaximumQualityPairCount) {
    throw std::invalid_argument(
        "--quality-pairs exceeds the hard cap of 262144");
  }
  if (options.quality_pair_count != 0U &&
      !options.reference_morton_window_radius.has_value()) {
    throw std::invalid_argument(
        "--quality-pairs requires --reference-morton-window");
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

[[nodiscard]] std::size_t checked_twice_minus_one(std::size_t value) {
  if (value == 0U ||
      value > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error("the heuristic edge extent overflows size_t");
  }
  return value * 2U - 1U;
}

void checked_accumulate_nanoseconds(
    std::uint64_t& total,
    std::uint64_t value) {
  if (value > std::numeric_limits<std::uint64_t>::max() - total) {
    throw std::overflow_error("the diagnostic duration sum overflows uint64");
  }
  total += value;
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (value < 0) {
    throw std::runtime_error("the monotonic diagnostic clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult
take_compatible_knn_transcript(
    morsehgp3d::gpu::Binary64LbvhTopKResult&& source) {
  const morsehgp3d::gpu::Binary64LbvhTopKAudit& audit = source.audit;
  if (!source.validated_complete_binary64_transcript() ||
      audit.persistent_input_device_byte_capacity >
          std::numeric_limits<std::size_t>::max() -
              audit.transient_output_device_byte_capacity) {
    throw std::runtime_error(
        "the binary64 LBVH result cannot be adapted to the hierarchy transcript");
  }
  morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult result;
  result.neighbor_point_ids = std::move(source.neighbor_point_ids);
  result.squared_distances = std::move(source.squared_distances);
  result.allocation_nanoseconds = audit.allocation_nanoseconds;
  result.host_to_device_nanoseconds = 0U;
  result.kernel_nanoseconds = audit.kernel_nanoseconds;
  result.device_to_host_nanoseconds = audit.device_to_host_nanoseconds;
  result.device_byte_capacity =
      audit.persistent_input_device_byte_capacity +
      audit.transient_output_device_byte_capacity;
  result.kernel_block_count = audit.kernel_block_count;
  result.kernel_thread_count = audit.kernel_thread_count;
  result.cuda_device = audit.cuda_device;
  result.multiprocessor_count = audit.multiprocessor_count;
  result.device_name = audit.device_name;
  return result;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value =
      (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value =
      (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] double unit_binary64(std::uint64_t bits) noexcept {
  constexpr double kInverse53 = 1.0 / 9007199254740992.0;
  return static_cast<double>(bits >> 11U) * kInverse53;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_uniform_cloud(const Options& options) {
  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(options.point_count);
  const double denominator = static_cast<double>(options.point_count);
  for (std::size_t index = 0U; index < options.point_count; ++index) {
    const std::uint64_t identity = static_cast<std::uint64_t>(index);
    // x is injective over the supported diagnostic range; y and z decorrelate
    // the Morton order without introducing a pair matrix or a hidden oracle.
    const double x = (static_cast<double>(index) + 0.5) / denominator;
    const double y = unit_binary64(splitmix64(identity ^ options.seed));
    const double z = unit_binary64(
        splitmix64(identity ^ std::rotl(options.seed, 29)));
    points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64(x, y, z));
  }
  return points;
}

template <typename Operation>
void parallel_shards(
    std::size_t item_count,
    std::size_t worker_count,
    Operation operation) {
  if (item_count == 0U) {
    return;
  }
  const std::size_t effective_workers =
      std::max(std::size_t{1}, std::min(item_count, worker_count));
  std::vector<std::thread> workers;
  workers.reserve(effective_workers);
  for (std::size_t worker = 0U; worker < effective_workers; ++worker) {
    workers.emplace_back([&, worker] {
      const std::size_t begin = item_count * worker / effective_workers;
      const std::size_t end = item_count * (worker + 1U) / effective_workers;
      operation(worker, begin, end);
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
}

void validate_knn_transcript(
    const morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult& knn,
    const std::vector<PointId>& point_ids_by_morton_position,
    std::size_t point_count,
    std::size_t maximum_order,
    std::size_t used_cpu_workers,
    const char* role) {
  const std::size_t expected_neighbor_count = checked_product(
      point_count,
      maximum_order,
      "the structural kNN transcript extent overflows size_t");
  if (point_ids_by_morton_position.size() != point_count ||
      knn.neighbor_point_ids.size() != expected_neighbor_count ||
      knn.squared_distances.size() != expected_neighbor_count) {
    throw std::runtime_error(
        std::string{role} + " returned a wrong transcript extent");
  }
  std::atomic<bool> invalid{false};
  parallel_shards(
      point_count,
      used_cpu_workers,
      [&](std::size_t, std::size_t begin, std::size_t end) {
        for (std::size_t position = begin; position < end; ++position) {
          const PointId source = point_ids_by_morton_position[position];
          const std::size_t offset = position * maximum_order;
          for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
            const std::size_t record = offset + rank;
            const PointId neighbor = knn.neighbor_point_ids[record];
            const double distance = knn.squared_distances[record];
            if (neighbor >= static_cast<PointId>(point_count) ||
                neighbor == source || !std::isfinite(distance) ||
                distance < 0.0) {
              invalid.store(true, std::memory_order_relaxed);
            }
            if (rank != 0U) {
              const PointId previous_id =
                  knn.neighbor_point_ids[record - 1U];
              const double previous_distance =
                  knn.squared_distances[record - 1U];
              if (distance < previous_distance ||
                  (distance == previous_distance && neighbor <= previous_id)) {
                invalid.store(true, std::memory_order_relaxed);
              }
            }
            for (std::size_t previous_rank = 0U;
                 previous_rank < rank;
                 ++previous_rank) {
              if (knn.neighbor_point_ids[offset + previous_rank] == neighbor) {
                invalid.store(true, std::memory_order_relaxed);
              }
            }
          }
        }
      });
  if (invalid.load(std::memory_order_relaxed)) {
    throw std::runtime_error(
        std::string{role} + " failed strict structural validation");
  }
}

[[nodiscard]] double squared_distance(
    const std::vector<double>& coordinates_by_point_id,
    PointId first,
    PointId second) {
  const std::size_t first_index = static_cast<std::size_t>(first);
  const std::size_t second_index = static_cast<std::size_t>(second);
  const double dx =
      coordinates_by_point_id[first_index * kAxisCount] -
      coordinates_by_point_id[second_index * kAxisCount];
  const double dy =
      coordinates_by_point_id[first_index * kAxisCount + 1U] -
      coordinates_by_point_id[second_index * kAxisCount + 1U];
  const double dz =
      coordinates_by_point_id[first_index * kAxisCount + 2U] -
      coordinates_by_point_id[second_index * kAxisCount + 2U];
  return dx * dx + dy * dy + dz * dz;
}

[[nodiscard]] SurrogateEdge make_edge(
    PointId first,
    PointId second,
    double squared_weight) noexcept {
  if (second < first) {
    std::swap(first, second);
  }
  return SurrogateEdge{squared_weight, first, second};
}

class DisjointSet final {
 public:
  explicit DisjointSet(std::size_t size)
      : parent_(size), sizes_(size, std::size_t{1}) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t item) noexcept {
    std::size_t root = item;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[item] != item) {
      const std::size_t next = parent_[item];
      parent_[item] = root;
      item = next;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t first, std::size_t second) noexcept {
    first = find(first);
    second = find(second);
    if (first == second) {
      return false;
    }
    if (sizes_[first] < sizes_[second] ||
        (sizes_[first] == sizes_[second] && second < first)) {
      std::swap(first, second);
    }
    parent_[second] = first;
    sizes_[first] += sizes_[second];
    return true;
  }

 private:
  std::vector<std::size_t> parent_;
  std::vector<std::size_t> sizes_;
};

void digest_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
  for (unsigned int byte = 0U; byte < 8U; ++byte) {
    digest ^= (word >> (byte * 8U)) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] OrderSummary reduce_surrogate_order(
    std::size_t order_index,
    std::vector<SurrogateEdge>& edges,
    std::size_t point_count,
    bool retain_quality_merges) {
  const auto begin = Clock::now();
  const std::size_t proposed_count = edges.size();
  std::sort(
      edges.begin(),
      edges.end(),
      [](const SurrogateEdge& left, const SurrogateEdge& right) {
        if (left.first != right.first) {
          return left.first < right.first;
        }
        if (left.second != right.second) {
          return left.second < right.second;
        }
        return left.squared_weight < right.squared_weight;
      });
  std::size_t write = 0U;
  for (const SurrogateEdge& edge : edges) {
    if (write != 0U && edges[write - 1U].first == edge.first &&
        edges[write - 1U].second == edge.second) {
      edges[write - 1U].squared_weight =
          std::min(edges[write - 1U].squared_weight, edge.squared_weight);
    } else {
      edges[write++] = edge;
    }
  }
  edges.resize(write);
  std::sort(
      edges.begin(),
      edges.end(),
      [](const SurrogateEdge& left, const SurrogateEdge& right) {
        if (left.squared_weight != right.squared_weight) {
          return left.squared_weight < right.squared_weight;
        }
        if (left.first != right.first) {
          return left.first < right.first;
        }
        return left.second < right.second;
      });

  DisjointSet disjoint_set(point_count);
  std::vector<MergeRecord> materialized_merges;
  materialized_merges.reserve(point_count - 1U);
  std::size_t component_count = point_count;
  for (const SurrogateEdge& edge : edges) {
    if (disjoint_set.unite(
            static_cast<std::size_t>(edge.first),
            static_cast<std::size_t>(edge.second))) {
      materialized_merges.push_back(
          MergeRecord{edge.squared_weight, edge.first, edge.second});
      --component_count;
    }
  }
  if (component_count != 1U || materialized_merges.size() != point_count - 1U) {
    throw std::runtime_error(
        "the Morton-chain surrogate unexpectedly failed to span the cloud");
  }

  std::size_t distinct_level_count = 0U;
  std::uint64_t previous_level_bits = 0U;
  bool has_previous_level = false;
  std::uint64_t digest = UINT64_C(1469598103934665603);
  digest_word(digest, static_cast<std::uint64_t>(order_index + 1U));
  for (const MergeRecord& merge : materialized_merges) {
    const std::uint64_t level_bits =
        std::bit_cast<std::uint64_t>(merge.squared_level);
    if (!has_previous_level || level_bits != previous_level_bits) {
      ++distinct_level_count;
      previous_level_bits = level_bits;
      has_previous_level = true;
    }
    digest_word(digest, level_bits);
    digest_word(digest, merge.first);
    digest_word(digest, merge.second);
  }
  const double root_squared_level = materialized_merges.back().squared_level;
  const auto end = Clock::now();
  OrderSummary summary{
      order_index + 1U,
      proposed_count,
      edges.size(),
      materialized_merges.size(),
      distinct_level_count,
      component_count,
      root_squared_level,
      digest,
      nanoseconds(end - begin),
      {}};
  if (retain_quality_merges) {
    summary.retained_quality_merges = std::move(materialized_merges);
  }
  return summary;
}

[[nodiscard]] HierarchyRun build_surrogate_hierarchies(
    std::size_t point_count,
    std::size_t maximum_order,
    std::size_t used_cpu_workers,
    const std::vector<PointId>& point_ids_by_morton_position,
    const std::vector<std::size_t>& morton_position_by_point_id,
    const std::vector<double>& chain_squared_distances,
    const morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult& knn,
    bool retain_quality_merges) {
  const std::size_t edge_count_per_order =
      checked_twice_minus_one(point_count);
  const auto edge_build_begin = Clock::now();
  std::vector<std::vector<SurrogateEdge>> order_edges(maximum_order);
  for (std::vector<SurrogateEdge>& edges : order_edges) {
    edges.resize(edge_count_per_order);
  }
  const std::size_t edge_task_count = checked_product(
      maximum_order,
      used_cpu_workers,
      "the heuristic edge task count overflows size_t");
  parallel_shards(
      edge_task_count,
      used_cpu_workers,
      [&](std::size_t, std::size_t task_begin, std::size_t task_end) {
        for (std::size_t task = task_begin; task < task_end; ++task) {
          const std::size_t order_index = task / used_cpu_workers;
          const std::size_t shard = task % used_cpu_workers;
          const std::size_t begin = point_count * shard / used_cpu_workers;
          const std::size_t end =
              point_count * (shard + 1U) / used_cpu_workers;
          std::vector<SurrogateEdge>& edges = order_edges[order_index];
          const auto core_distance = [&](PointId point_id) {
            const std::size_t point = static_cast<std::size_t>(point_id);
            const std::size_t position = morton_position_by_point_id[point];
            return knn.squared_distances[
                position * maximum_order + order_index];
          };
          for (std::size_t position = begin; position < end; ++position) {
            const PointId source = point_ids_by_morton_position[position];
            if (position != 0U) {
              const PointId previous =
                  point_ids_by_morton_position[position - 1U];
              const double weight = std::max(
                  chain_squared_distances[position - 1U],
                  std::max(core_distance(source), core_distance(previous)));
              edges[position - 1U] = make_edge(source, previous, weight);
            }
            const std::size_t neighbor_record =
                position * maximum_order + order_index;
            const PointId neighbor = knn.neighbor_point_ids[neighbor_record];
            const double weight = std::max(
                knn.squared_distances[neighbor_record],
                std::max(core_distance(source), core_distance(neighbor)));
            edges[point_count - 1U + position] =
                make_edge(source, neighbor, weight);
          }
        }
      });
  const auto edge_build_end = Clock::now();

  const auto hierarchy_begin = Clock::now();
  std::vector<OrderSummary> summaries(maximum_order);
  std::atomic<std::size_t> next_order{0U};
  std::exception_ptr hierarchy_failure;
  std::mutex hierarchy_failure_mutex;
  const std::size_t order_worker_count =
      std::min(maximum_order, used_cpu_workers);
  std::vector<std::thread> order_workers;
  order_workers.reserve(order_worker_count);
  for (std::size_t worker = 0U; worker < order_worker_count; ++worker) {
    order_workers.emplace_back([&] {
      try {
        while (true) {
          const std::size_t order =
              next_order.fetch_add(1U, std::memory_order_relaxed);
          if (order >= maximum_order) {
            break;
          }
          summaries[order] = reduce_surrogate_order(
              order,
              order_edges[order],
              point_count,
              retain_quality_merges);
        }
      } catch (...) {
        std::lock_guard<std::mutex> lock{hierarchy_failure_mutex};
        if (hierarchy_failure == nullptr) {
          hierarchy_failure = std::current_exception();
        }
      }
    });
  }
  for (std::thread& worker : order_workers) {
    worker.join();
  }
  if (hierarchy_failure != nullptr) {
    std::rethrow_exception(hierarchy_failure);
  }
  const auto hierarchy_end = Clock::now();
  return HierarchyRun{
      std::move(summaries),
      nanoseconds(edge_build_end - edge_build_begin),
      nanoseconds(hierarchy_end - hierarchy_begin),
      order_worker_count};
}

[[nodiscard]] NeighborComparison compare_neighbors(
    const morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult& candidate,
    const std::vector<PointId>& reference_neighbor_ids,
    std::size_t row_count,
    std::size_t maximum_order) {
  const std::size_t expected_count = checked_product(
      row_count,
      maximum_order,
      "the neighbor comparison extent overflows size_t");
  if (candidate.neighbor_point_ids.size() < expected_count ||
      reference_neighbor_ids.size() != expected_count) {
    throw std::invalid_argument("the neighbor comparison has wrong extents");
  }
  NeighborComparison comparison;
  comparison.prefix_match_counts.resize(maximum_order);
  comparison.prefix_totals.resize(maximum_order);
  comparison.total_neighbor_count = expected_count;
  for (std::size_t row = 0U; row < row_count; ++row) {
    const std::size_t offset = row * maximum_order;
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      if (candidate.neighbor_point_ids[offset + rank] ==
          reference_neighbor_ids[offset + rank]) {
        ++comparison.exact_rank_match_count;
      }
      std::size_t prefix_matches = 0U;
      for (std::size_t candidate_rank = 0U;
           candidate_rank <= rank;
           ++candidate_rank) {
        const PointId candidate_id =
            candidate.neighbor_point_ids[offset + candidate_rank];
        bool matched = false;
        for (std::size_t reference_rank = 0U;
             reference_rank <= rank;
             ++reference_rank) {
          matched = matched ||
                    candidate_id ==
                        reference_neighbor_ids[offset + reference_rank];
        }
        prefix_matches += matched ? 1U : 0U;
      }
      comparison.prefix_match_counts[rank] += prefix_matches;
      comparison.prefix_totals[rank] += rank + 1U;
    }
  }
  return comparison;
}

[[nodiscard]] ExhaustiveValidation validate_against_exhaustive_binary64_knn(
    std::size_t query_count,
    std::size_t point_count,
    std::size_t maximum_order,
    std::size_t used_cpu_workers,
    const std::vector<double>& coordinates_by_point_id,
    const std::vector<PointId>& point_ids_by_morton_position,
    const morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult& candidate) {
  const auto begin = Clock::now();
  const std::size_t validation_neighbor_count = checked_product(
      query_count,
      maximum_order,
      "the exhaustive validation output extent overflows size_t");
  std::vector<PointId> exhaustive_neighbor_ids(
      validation_neighbor_count,
      std::numeric_limits<PointId>::max());
  parallel_shards(
      query_count,
      used_cpu_workers,
      [&](std::size_t, std::size_t query_begin, std::size_t query_end) {
        for (std::size_t query = query_begin; query < query_end; ++query) {
          const std::size_t position = query * point_count / query_count;
          const PointId source = point_ids_by_morton_position[position];
          std::array<double, kMaximumSupportedOrder> best_distances{};
          std::array<PointId, kMaximumSupportedOrder> best_ids{};
          best_distances.fill(std::numeric_limits<double>::max());
          best_ids.fill(std::numeric_limits<PointId>::max());
          for (std::size_t point = 0U; point < point_count; ++point) {
            const PointId candidate_id = static_cast<PointId>(point);
            if (candidate_id == source) {
              continue;
            }
            const double distance = squared_distance(
                coordinates_by_point_id, source, candidate_id);
            const auto nearer = [&](std::size_t rank) {
              return distance < best_distances[rank] ||
                     (distance == best_distances[rank] &&
                      candidate_id < best_ids[rank]);
            };
            if (!nearer(maximum_order - 1U)) {
              continue;
            }
            std::size_t insertion = maximum_order - 1U;
            while (insertion > 0U && nearer(insertion - 1U)) {
              best_distances[insertion] = best_distances[insertion - 1U];
              best_ids[insertion] = best_ids[insertion - 1U];
              --insertion;
            }
            best_distances[insertion] = distance;
            best_ids[insertion] = candidate_id;
          }
          for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
            exhaustive_neighbor_ids[query * maximum_order + rank] =
                best_ids[rank];
          }
        }
      });

  // Gather the sampled Morton rows so the generic comparison has contiguous
  // rows while the production output remains untouched.
  morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult sampled_candidate;
  sampled_candidate.neighbor_point_ids.resize(validation_neighbor_count);
  for (std::size_t query = 0U; query < query_count; ++query) {
    const std::size_t position = query * point_count / query_count;
    std::copy_n(
        candidate.neighbor_point_ids.begin() +
            static_cast<std::ptrdiff_t>(position * maximum_order),
        maximum_order,
        sampled_candidate.neighbor_point_ids.begin() +
            static_cast<std::ptrdiff_t>(query * maximum_order));
  }
  NeighborComparison comparison = compare_neighbors(
      sampled_candidate,
      exhaustive_neighbor_ids,
      query_count,
      maximum_order);
  const std::size_t distance_evaluations = checked_product(
      query_count,
      point_count - 1U,
      "the exhaustive validation counter overflows size_t");
  const auto end = Clock::now();
  return ExhaustiveValidation{
      query_count,
      distance_evaluations,
      std::move(comparison),
      nanoseconds(end - begin)};
}

class WeightedTreeMaximumQuery final {
 public:
  WeightedTreeMaximumQuery(
      std::size_t point_count,
      const std::vector<MergeRecord>& merges)
      : point_count_(point_count) {
    if (point_count_ < 2U || merges.size() != point_count_ - 1U) {
      throw std::invalid_argument(
          "a quality MST must contain exactly n-1 merge edges");
    }
    std::vector<std::size_t> degrees(point_count_);
    for (const MergeRecord& merge : merges) {
      if (merge.first >= static_cast<PointId>(point_count_) ||
          merge.second >= static_cast<PointId>(point_count_) ||
          merge.first == merge.second ||
          !std::isfinite(merge.squared_level) ||
          merge.squared_level < 0.0) {
        throw std::invalid_argument(
            "a quality MST contains an invalid merge edge");
      }
      ++degrees[static_cast<std::size_t>(merge.first)];
      ++degrees[static_cast<std::size_t>(merge.second)];
    }
    std::vector<std::size_t> offsets(point_count_ + 1U);
    for (std::size_t point = 0U; point < point_count_; ++point) {
      if (degrees[point] >
          std::numeric_limits<std::size_t>::max() - offsets[point]) {
        throw std::length_error("the quality MST adjacency overflows size_t");
      }
      offsets[point + 1U] = offsets[point] + degrees[point];
    }
    if (offsets.back() != checked_product(
                              merges.size(),
                              std::size_t{2},
                              "the quality MST edge extent overflows size_t")) {
      throw std::logic_error("the quality MST adjacency count is inconsistent");
    }
    std::vector<std::size_t> neighbors(offsets.back());
    std::vector<double> weights(offsets.back());
    std::vector<std::size_t> cursors = offsets;
    for (const MergeRecord& merge : merges) {
      const std::size_t first = static_cast<std::size_t>(merge.first);
      const std::size_t second = static_cast<std::size_t>(merge.second);
      const std::size_t first_slot = cursors[first]++;
      const std::size_t second_slot = cursors[second]++;
      neighbors[first_slot] = second;
      weights[first_slot] = merge.squared_level;
      neighbors[second_slot] = first;
      weights[second_slot] = merge.squared_level;
    }

    constexpr std::size_t kInvalid =
        std::numeric_limits<std::size_t>::max();
    depth_.assign(point_count_, 0U);
    std::vector<std::size_t> parent(point_count_, kInvalid);
    std::vector<double> parent_weight(point_count_, 0.0);
    std::vector<std::size_t> stack;
    stack.reserve(point_count_);
    parent[0] = 0U;
    stack.push_back(0U);
    std::size_t visited_count = 0U;
    while (!stack.empty()) {
      const std::size_t point = stack.back();
      stack.pop_back();
      ++visited_count;
      for (std::size_t edge = offsets[point];
           edge < offsets[point + 1U];
           ++edge) {
        const std::size_t neighbor = neighbors[edge];
        if (neighbor == parent[point]) {
          continue;
        }
        if (parent[neighbor] != kInvalid) {
          throw std::logic_error("the quality merge graph is not a tree");
        }
        parent[neighbor] = point;
        parent_weight[neighbor] = weights[edge];
        if (depth_[point] == std::numeric_limits<std::size_t>::max()) {
          throw std::length_error("the quality MST depth overflows size_t");
        }
        depth_[neighbor] = depth_[point] + 1U;
        stack.push_back(neighbor);
      }
    }
    if (visited_count != point_count_) {
      throw std::logic_error("the quality merge tree is disconnected");
    }

    level_count_ = 1U;
    std::size_t covered_depth = 1U;
    while (covered_depth < point_count_) {
      if (covered_depth > std::numeric_limits<std::size_t>::max() / 2U) {
        throw std::length_error("the quality LCA level count overflows size_t");
      }
      covered_depth *= 2U;
      ++level_count_;
    }
    const std::size_t table_size = checked_product(
        level_count_,
        point_count_,
        "the quality LCA table extent overflows size_t");
    ancestors_.resize(table_size);
    maximum_weights_.resize(table_size);
    std::copy(parent.begin(), parent.end(), ancestors_.begin());
    std::copy(
        parent_weight.begin(), parent_weight.end(), maximum_weights_.begin());
    for (std::size_t level = 1U; level < level_count_; ++level) {
      const std::size_t previous_offset = (level - 1U) * point_count_;
      const std::size_t offset = level * point_count_;
      for (std::size_t point = 0U; point < point_count_; ++point) {
        const std::size_t middle = ancestors_[previous_offset + point];
        ancestors_[offset + point] = ancestors_[previous_offset + middle];
        maximum_weights_[offset + point] = std::max(
            maximum_weights_[previous_offset + point],
            maximum_weights_[previous_offset + middle]);
      }
    }
  }

  [[nodiscard]] double maximum_on_path(
      std::size_t first,
      std::size_t second) const {
    if (first >= point_count_ || second >= point_count_) {
      throw std::out_of_range("a quality pair endpoint is outside the MST");
    }
    double maximum = 0.0;
    if (depth_[first] < depth_[second]) {
      std::swap(first, second);
    }
    std::size_t difference = depth_[first] - depth_[second];
    std::size_t level = 0U;
    while (difference != 0U) {
      if ((difference & std::size_t{1}) != 0U) {
        maximum = std::max(
            maximum, maximum_weights_[level * point_count_ + first]);
        first = ancestors_[level * point_count_ + first];
      }
      difference >>= 1U;
      ++level;
    }
    if (first == second) {
      return maximum;
    }
    for (std::size_t reverse = level_count_; reverse-- > 0U;) {
      const std::size_t first_ancestor =
          ancestors_[reverse * point_count_ + first];
      const std::size_t second_ancestor =
          ancestors_[reverse * point_count_ + second];
      if (first_ancestor != second_ancestor) {
        maximum = std::max(
            maximum,
            std::max(
                maximum_weights_[reverse * point_count_ + first],
                maximum_weights_[reverse * point_count_ + second]));
        first = first_ancestor;
        second = second_ancestor;
      }
    }
    return std::max(
        maximum,
        std::max(
            maximum_weights_[first],
            maximum_weights_[second]));
  }

 private:
  std::size_t point_count_{};
  std::size_t level_count_{};
  std::vector<std::size_t> depth_;
  std::vector<std::size_t> ancestors_;
  std::vector<double> maximum_weights_;
};

class PearsonAccumulator final {
 public:
  void add(double first, double second) noexcept {
    ++count_;
    const long double x = static_cast<long double>(first);
    const long double y = static_cast<long double>(second);
    const long double delta_x = x - mean_x_;
    const long double delta_y = y - mean_y_;
    mean_x_ += delta_x / static_cast<long double>(count_);
    mean_y_ += delta_y / static_cast<long double>(count_);
    covariance_ += delta_x * (y - mean_y_);
    variance_x_ += delta_x * (x - mean_x_);
    variance_y_ += delta_y * (y - mean_y_);
  }

  [[nodiscard]] std::optional<double> value() const {
    if (count_ < 2U || !(variance_x_ > 0.0L) ||
        !(variance_y_ > 0.0L)) {
      return std::nullopt;
    }
    const long double denominator =
        std::sqrt(variance_x_ * variance_y_);
    if (!(denominator > 0.0L) || !std::isfinite(denominator)) {
      return std::nullopt;
    }
    long double value = covariance_ / denominator;
    if (!std::isfinite(value)) {
      return std::nullopt;
    }
    value = std::max(-1.0L, std::min(1.0L, value));
    return static_cast<double>(value);
  }

 private:
  std::size_t count_{};
  long double mean_x_{};
  long double mean_y_{};
  long double covariance_{};
  long double variance_x_{};
  long double variance_y_{};
};

[[nodiscard]] std::optional<double> normalized_ratio(
    long double numerator,
    long double denominator) {
  if (denominator == 0.0L) {
    return numerator == 0.0L ? std::optional<double>{0.0}
                             : std::nullopt;
  }
  if (!(denominator > 0.0L) || !std::isfinite(numerator) ||
      !std::isfinite(denominator)) {
    return std::nullopt;
  }
  const long double ratio = numerator / denominator;
  if (!std::isfinite(ratio)) {
    return std::nullopt;
  }
  return static_cast<double>(ratio);
}

[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>>
make_quality_pairs(
    std::size_t point_count,
    std::size_t pair_count,
    std::uint64_t sample_seed) {
  std::vector<std::pair<std::size_t, std::size_t>> pairs;
  pairs.reserve(pair_count);
  const std::uint64_t modulus = static_cast<std::uint64_t>(point_count);
  const std::uint64_t second_modulus = modulus - UINT64_C(1);
  for (std::size_t index = 0U; index < pair_count; ++index) {
    const std::uint64_t sequence = static_cast<std::uint64_t>(index);
    std::size_t first = static_cast<std::size_t>(
        splitmix64(sample_seed + sequence * UINT64_C(0x9e3779b97f4a7c15)) %
        modulus);
    std::size_t second = static_cast<std::size_t>(
        splitmix64(
            sample_seed ^
            (sequence * UINT64_C(0xd1b54a32d192ed03) +
             UINT64_C(0x94d049bb133111eb))) %
        second_modulus);
    if (second >= first) {
      ++second;
    }
    if (second < first) {
      std::swap(first, second);
    }
    pairs.emplace_back(first, second);
  }
  return pairs;
}

[[nodiscard]] std::size_t quantile_floor_index(
    std::size_t final_index,
    unsigned int percent) {
  const std::size_t quotient = final_index / 100U;
  const std::size_t remainder = final_index % 100U;
  return quotient * static_cast<std::size_t>(percent) +
         remainder * static_cast<std::size_t>(percent) / 100U;
}

[[nodiscard]] HierarchyQuality evaluate_hierarchy_quality(
    const HierarchyRun& fast,
    const HierarchyRun& reference,
    std::size_t point_count,
    std::size_t pair_count,
    std::uint64_t sample_seed) {
  const auto begin = Clock::now();
  if (fast.summaries.size() != reference.summaries.size() ||
      fast.summaries.empty() || pair_count == 0U) {
    throw std::invalid_argument("the hierarchy quality extents are invalid");
  }
  const std::vector<std::pair<std::size_t, std::size_t>> pairs =
      make_quality_pairs(point_count, pair_count, sample_seed);
  HierarchyQuality quality;
  quality.sample_seed = sample_seed;
  quality.sampled_pair_count = pair_count;
  quality.orders.reserve(fast.summaries.size());

  long double aggregate_barcode_absolute_sum = 0.0L;
  long double aggregate_barcode_reference_l1 = 0.0L;
  double aggregate_barcode_linf = 0.0;
  double aggregate_barcode_normalized_linf = 0.0;
  bool aggregate_barcode_linf_normalization_defined = true;
  long double aggregate_cophenetic_absolute_sum = 0.0L;
  long double aggregate_cophenetic_normalized_sum = 0.0L;
  double aggregate_cophenetic_maximum = 0.0;
  double aggregate_cophenetic_normalized_maximum = 0.0;
  bool aggregate_cophenetic_normalization_defined = true;
  PearsonAccumulator aggregate_pearson;

  for (std::size_t order = 0U; order < fast.summaries.size(); ++order) {
    const OrderSummary& fast_summary = fast.summaries[order];
    const OrderSummary& reference_summary = reference.summaries[order];
    const std::vector<MergeRecord>& fast_merges =
        fast_summary.retained_quality_merges;
    const std::vector<MergeRecord>& reference_merges =
        reference_summary.retained_quality_merges;
    if (fast_merges.size() != point_count - 1U ||
        reference_merges.size() != point_count - 1U) {
      throw std::logic_error(
          "quality evaluation requires retained finite MST deaths");
    }

    OrderHierarchyQuality order_quality;
    order_quality.order = order + 1U;
    order_quality.barcode.finite_death_count = fast_merges.size();
    long double barcode_absolute_sum = 0.0L;
    long double barcode_reference_l1 = 0.0L;
    double barcode_linf = 0.0;
    for (std::size_t death = 0U; death < fast_merges.size(); ++death) {
      const double fast_level = fast_merges[death].squared_level;
      const double reference_level = reference_merges[death].squared_level;
      if (death != 0U &&
          (fast_level < fast_merges[death - 1U].squared_level ||
           reference_level < reference_merges[death - 1U].squared_level)) {
        throw std::logic_error("quality finite deaths are not sorted");
      }
      if (std::bit_cast<std::uint64_t>(fast_level) ==
          std::bit_cast<std::uint64_t>(reference_level)) {
        ++order_quality.barcode.exact_level_bit_match_count;
      }
      const double difference = std::abs(fast_level - reference_level);
      barcode_absolute_sum += static_cast<long double>(difference);
      barcode_reference_l1 += static_cast<long double>(
          std::abs(reference_level));
      barcode_linf = std::max(barcode_linf, difference);
    }
    const double reference_root = reference_summary.root_squared_level;
    order_quality.barcode.sorted_death_level_l1_raw =
        static_cast<double>(barcode_absolute_sum);
    order_quality.barcode.sorted_death_level_l1_normalized_reference_l1 =
        normalized_ratio(barcode_absolute_sum, barcode_reference_l1);
    order_quality.barcode.sorted_death_level_linf_raw = barcode_linf;
    order_quality.barcode.sorted_death_level_linf_normalized_reference_root =
        normalized_ratio(
            static_cast<long double>(barcode_linf),
            static_cast<long double>(reference_root));

    WeightedTreeMaximumQuery fast_tree(point_count, fast_merges);
    WeightedTreeMaximumQuery reference_tree(point_count, reference_merges);
    std::vector<double> fast_cophenetic(pair_count);
    std::vector<double> reference_cophenetic(pair_count);
    long double cophenetic_absolute_sum = 0.0L;
    long double cophenetic_normalized_sum = 0.0L;
    double cophenetic_maximum = 0.0;
    double cophenetic_normalized_maximum = 0.0;
    PearsonAccumulator order_pearson;
    for (std::size_t pair = 0U; pair < pair_count; ++pair) {
      const double fast_level = fast_tree.maximum_on_path(
          pairs[pair].first, pairs[pair].second);
      const double reference_level = reference_tree.maximum_on_path(
          pairs[pair].first, pairs[pair].second);
      fast_cophenetic[pair] = fast_level;
      reference_cophenetic[pair] = reference_level;
      if (std::bit_cast<std::uint64_t>(fast_level) ==
          std::bit_cast<std::uint64_t>(reference_level)) {
        ++order_quality.cophenetic.exact_level_bit_match_count;
      }
      const double difference = std::abs(fast_level - reference_level);
      cophenetic_absolute_sum += static_cast<long double>(difference);
      cophenetic_maximum = std::max(cophenetic_maximum, difference);
      if (reference_root > 0.0) {
        const double normalized_difference = difference / reference_root;
        cophenetic_normalized_sum +=
            static_cast<long double>(normalized_difference);
        cophenetic_normalized_maximum = std::max(
            cophenetic_normalized_maximum, normalized_difference);
      }
      order_pearson.add(fast_level, reference_level);
      aggregate_pearson.add(fast_level, reference_level);
    }
    order_quality.cophenetic.sampled_pair_count = pair_count;
    order_quality.cophenetic.mean_absolute_error_raw = static_cast<double>(
        cophenetic_absolute_sum / static_cast<long double>(pair_count));
    order_quality.cophenetic.mean_absolute_error_normalized_reference_root =
        reference_root > 0.0
            ? std::optional<double>{static_cast<double>(
                  cophenetic_normalized_sum /
                  static_cast<long double>(pair_count))}
            : normalized_ratio(cophenetic_absolute_sum, 0.0L);
    order_quality.cophenetic.maximum_absolute_error_raw =
        cophenetic_maximum;
    order_quality.cophenetic.maximum_absolute_error_normalized_reference_root =
        normalized_ratio(
            static_cast<long double>(cophenetic_maximum),
            static_cast<long double>(reference_root));
    order_quality.cophenetic.pearson = order_pearson.value();

    constexpr std::array<unsigned int, 9> kQuantiles{
        10U, 20U, 30U, 40U, 50U, 60U, 70U, 80U, 90U};
    std::vector<double> sorted_reference_cophenetic = reference_cophenetic;
    std::sort(
        sorted_reference_cophenetic.begin(),
        sorted_reference_cophenetic.end());
    order_quality.cluster_thresholds.reserve(kQuantiles.size());
    for (const unsigned int percent : kQuantiles) {
      const std::size_t cophenetic_index = quantile_floor_index(
          sorted_reference_cophenetic.size() - 1U, percent);
      const double threshold =
          sorted_reference_cophenetic[cophenetic_index];
      ClusterThresholdQuality threshold_quality;
      threshold_quality.quantile_percent = percent;
      threshold_quality.reference_squared_level = threshold;
      threshold_quality.pair_count = pair_count;
      for (std::size_t pair = 0U; pair < pair_count; ++pair) {
        const bool fast_same = fast_cophenetic[pair] <= threshold;
        const bool reference_same = reference_cophenetic[pair] <= threshold;
        threshold_quality.agreement_count +=
            fast_same == reference_same ? 1U : 0U;
        threshold_quality.both_same_cluster_count +=
            fast_same && reference_same ? 1U : 0U;
        threshold_quality.both_different_cluster_count +=
            !fast_same && !reference_same ? 1U : 0U;
        threshold_quality.fast_only_same_cluster_count +=
            fast_same && !reference_same ? 1U : 0U;
        threshold_quality.reference_only_same_cluster_count +=
            !fast_same && reference_same ? 1U : 0U;
      }
      order_quality.cluster_agreement_count +=
          threshold_quality.agreement_count;
      order_quality.cluster_comparison_count += pair_count;
      order_quality.cluster_thresholds.push_back(threshold_quality);
    }

    quality.aggregate.barcode.exact_level_bit_match_count +=
        order_quality.barcode.exact_level_bit_match_count;
    quality.aggregate.barcode.finite_death_count +=
        order_quality.barcode.finite_death_count;
    aggregate_barcode_absolute_sum += barcode_absolute_sum;
    aggregate_barcode_reference_l1 += barcode_reference_l1;
    aggregate_barcode_linf = std::max(
        aggregate_barcode_linf, barcode_linf);
    if (order_quality.barcode
            .sorted_death_level_linf_normalized_reference_root.has_value()) {
      aggregate_barcode_normalized_linf = std::max(
          aggregate_barcode_normalized_linf,
          *order_quality.barcode
               .sorted_death_level_linf_normalized_reference_root);
    } else {
      aggregate_barcode_linf_normalization_defined = false;
    }
    quality.aggregate.cophenetic.exact_level_bit_match_count +=
        order_quality.cophenetic.exact_level_bit_match_count;
    quality.aggregate.cophenetic.sampled_pair_count += pair_count;
    aggregate_cophenetic_absolute_sum += cophenetic_absolute_sum;
    aggregate_cophenetic_normalized_sum += cophenetic_normalized_sum;
    aggregate_cophenetic_maximum = std::max(
        aggregate_cophenetic_maximum, cophenetic_maximum);
    aggregate_cophenetic_normalized_maximum = std::max(
        aggregate_cophenetic_normalized_maximum,
        cophenetic_normalized_maximum);
    if (reference_root == 0.0 && cophenetic_absolute_sum != 0.0L) {
      aggregate_cophenetic_normalization_defined = false;
    }
    quality.aggregate.cluster_agreement_count +=
        order_quality.cluster_agreement_count;
    quality.aggregate.cluster_comparison_count +=
        order_quality.cluster_comparison_count;
    quality.orders.push_back(std::move(order_quality));
  }

  quality.aggregate.barcode.sorted_death_level_l1_raw =
      static_cast<double>(aggregate_barcode_absolute_sum);
  quality.aggregate.barcode.sorted_death_level_l1_normalized_reference_l1 =
      normalized_ratio(
          aggregate_barcode_absolute_sum,
          aggregate_barcode_reference_l1);
  quality.aggregate.barcode.sorted_death_level_linf_raw =
      aggregate_barcode_linf;
  quality.aggregate.barcode
      .sorted_death_level_linf_normalized_reference_root =
      aggregate_barcode_linf_normalization_defined
          ? std::optional<double>{aggregate_barcode_normalized_linf}
          : std::nullopt;
  const std::size_t total_cophenetic_count =
      quality.aggregate.cophenetic.sampled_pair_count;
  quality.aggregate.cophenetic.mean_absolute_error_raw = static_cast<double>(
      aggregate_cophenetic_absolute_sum /
      static_cast<long double>(total_cophenetic_count));
  quality.aggregate.cophenetic.mean_absolute_error_normalized_reference_root =
      aggregate_cophenetic_normalization_defined
          ? std::optional<double>{static_cast<double>(
                aggregate_cophenetic_normalized_sum /
                static_cast<long double>(total_cophenetic_count))}
          : std::nullopt;
  quality.aggregate.cophenetic.maximum_absolute_error_raw =
      aggregate_cophenetic_maximum;
  quality.aggregate.cophenetic.maximum_absolute_error_normalized_reference_root =
      aggregate_cophenetic_normalization_defined
          ? std::optional<double>{aggregate_cophenetic_normalized_maximum}
          : std::nullopt;
  quality.aggregate.cophenetic.pearson = aggregate_pearson.value();
  quality.cpu_nanoseconds = nanoseconds(Clock::now() - begin);
  return quality;
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
  std::ostringstream output;
  output << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return output.str();
}

void write_optional_json_number(const std::optional<double>& value) {
  if (value.has_value()) {
    std::cout << *value;
  } else {
    std::cout << "null";
  }
}

void write_barcode_quality_json(const BarcodeQuality& quality) {
  std::cout
      << "{\"finite_death_count\":" << quality.finite_death_count
      << ",\"levels_sorted_nondecreasing\":true"
      << ",\"comparison_model\":\"sorted_finite_death_counting_measure_without_diagonal_matching\""
      << ",\"exact_level_bit_matches\":"
      << quality.exact_level_bit_match_count
      << ",\"level_comparison_total\":" << quality.finite_death_count
      << ",\"sorted_death_level_l1_raw\":"
      << quality.sorted_death_level_l1_raw
      << ",\"sorted_death_level_l1_normalized_reference_l1\":";
  write_optional_json_number(
      quality.sorted_death_level_l1_normalized_reference_l1);
  std::cout << ",\"sorted_death_level_linf_raw\":"
            << quality.sorted_death_level_linf_raw
            << ",\"sorted_death_level_linf_normalized_reference_root\":";
  write_optional_json_number(
      quality.sorted_death_level_linf_normalized_reference_root);
  std::cout << '}';
}

void write_cophenetic_quality_json(const CopheneticQuality& quality) {
  std::cout
      << "{\"sampled_pair_count\":" << quality.sampled_pair_count
      << ",\"exact_level_bit_matches\":"
      << quality.exact_level_bit_match_count
      << ",\"level_comparison_total\":" << quality.sampled_pair_count
      << ",\"mean_absolute_error_raw\":"
      << quality.mean_absolute_error_raw
      << ",\"mean_absolute_error_normalized_reference_root\":";
  write_optional_json_number(
      quality.mean_absolute_error_normalized_reference_root);
  std::cout << ",\"maximum_absolute_error_raw\":"
            << quality.maximum_absolute_error_raw
            << ",\"maximum_absolute_error_normalized_reference_root\":";
  write_optional_json_number(
      quality.maximum_absolute_error_normalized_reference_root);
  std::cout << ",\"pearson_defined_finite_non_degenerate\":"
            << (quality.pearson.has_value() ? "true" : "false")
            << ",\"pearson\":";
  write_optional_json_number(quality.pearson);
  std::cout << '}';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const unsigned int hardware_query = std::thread::hardware_concurrency();
    const std::size_t available_cpu_workers =
        hardware_query == 0U ? std::size_t{1}
                             : static_cast<std::size_t>(hardware_query);
    const std::size_t used_cpu_workers = std::max(
        std::size_t{1},
        std::min(
            options.point_count,
            std::min(options.requested_cpu_workers, available_cpu_workers)));
    const std::size_t effective_window =
        std::min(options.morton_window_radius, options.point_count - 1U);
    const std::size_t coordinate_count = checked_product(
        options.point_count,
        kAxisCount,
        "the heuristic coordinate extent overflows size_t");
    const std::size_t neighbor_count = checked_product(
        options.point_count,
        options.maximum_order,
        "the heuristic neighbor extent overflows size_t");

    const auto total_begin = Clock::now();
    const auto generation_begin = Clock::now();
    std::vector<morsehgp3d::exact::CertifiedPoint3> input =
        make_uniform_cloud(options);
    const auto generation_end = Clock::now();

    const auto canonicalization_begin = Clock::now();
    morsehgp3d::spatial::CanonicalPointCloud cloud =
        morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(input);
    const auto canonicalization_end = Clock::now();
    input.clear();
    input.shrink_to_fit();

    const auto morton_build_begin = Clock::now();
    std::optional<morsehgp3d::spatial::MortonLbvhIndex> cpu_morton_index;
    std::optional<morsehgp3d::gpu::MortonLbvhBuildContext>
        gpu_morton_build_context;
    std::optional<morsehgp3d::gpu::MortonLbvhDeviceBuildResult>
        gpu_morton_build_result;
    std::optional<morsehgp3d::gpu::MortonLbvhDeviceBuildAudit>
        gpu_morton_build_audit;
    std::optional<morsehgp3d::gpu::Binary64LbvhTopKContext>
        binary64_lbvh_context;
    const morsehgp3d::spatial::MortonLbvhIndex* morton_index = nullptr;
    if (options.knn_backend == KnnBackend::binary64_lbvh) {
      gpu_morton_build_context.emplace(options.point_count);
      gpu_morton_build_result.emplace(
          gpu_morton_build_context->build(cloud));
      if (!gpu_morton_build_result->complete_certified_build() ||
          !gpu_morton_build_result->cuda_qualified_build()) {
        throw std::runtime_error(
            "the binary64 LBVH backend requires a certified CUDA LBVH build");
      }
      gpu_morton_build_audit.emplace(gpu_morton_build_result->audit());
      morton_index = &gpu_morton_build_result->certified_index();
      morsehgp3d::gpu::MortonLbvhDeviceTraversalLease traversal_lease =
          gpu_morton_build_context->release_device_traversal_lease(
              *gpu_morton_build_result);
      binary64_lbvh_context.emplace(cloud, std::move(traversal_lease));
    } else {
      cpu_morton_index.emplace(
          morsehgp3d::spatial::MortonLbvhIndex::build(cloud));
      morton_index = &*cpu_morton_index;
    }
    const auto morton_build_end = Clock::now();
    const std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves =
        morton_index->leaves();
    if (leaves.size() != options.point_count) {
      throw std::runtime_error("the CPU Morton index returned a wrong leaf count");
    }

    const auto coordinate_export_begin = Clock::now();
    std::vector<double> coordinates_by_point_id(coordinate_count);
    parallel_shards(
        options.point_count,
        used_cpu_workers,
        [&](std::size_t, std::size_t begin, std::size_t end) {
          for (std::size_t point = begin; point < end; ++point) {
            const auto& value = cloud.point(static_cast<PointId>(point));
            for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
              coordinates_by_point_id[point * kAxisCount + axis] =
                  value.binary64_coordinate(axis);
            }
          }
        });
    const auto coordinate_export_end = Clock::now();

    const auto morton_pack_begin = Clock::now();
    std::vector<double> coordinates_by_morton_position(coordinate_count);
    std::vector<PointId> point_ids_by_morton_position(options.point_count);
    std::vector<std::size_t> morton_position_by_point_id(options.point_count);
    parallel_shards(
        options.point_count,
        used_cpu_workers,
        [&](std::size_t, std::size_t begin, std::size_t end) {
          for (std::size_t position = begin; position < end; ++position) {
            const PointId point_id = leaves[position].point_id;
            const std::size_t point = static_cast<std::size_t>(point_id);
            point_ids_by_morton_position[position] = point_id;
            morton_position_by_point_id[point] = position;
            for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
              coordinates_by_morton_position[position * kAxisCount + axis] =
                  coordinates_by_point_id[point * kAxisCount + axis];
            }
          }
        });
    const auto morton_pack_end = Clock::now();

    const auto gpu_wall_begin = Clock::now();
    morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult knn;
    std::optional<morsehgp3d::gpu::Binary64LbvhTopKAudit>
        binary64_lbvh_audit;
    if (options.knn_backend == KnnBackend::binary64_lbvh) {
      morsehgp3d::gpu::Binary64LbvhTopKResult binary64_result =
          binary64_lbvh_context->query_all(
              cloud, options.maximum_order, effective_window);
      binary64_lbvh_audit.emplace(binary64_result.audit);
      knn = take_compatible_knn_transcript(std::move(binary64_result));
    } else {
      knn = morsehgp3d::gpu::detail::
          run_phase14_morton_window_knn_on_gpu(
              coordinates_by_morton_position,
              point_ids_by_morton_position,
              options.maximum_order,
              effective_window);
    }
    const auto gpu_wall_end = Clock::now();
    const auto validation_begin = Clock::now();
    validate_knn_transcript(
        knn,
        point_ids_by_morton_position,
        options.point_count,
        options.maximum_order,
        used_cpu_workers,
        options.knn_backend == KnnBackend::binary64_lbvh
            ? "the binary64 LBVH GPU transcript"
            : "the fast-window GPU transcript");
    const auto validation_end = Clock::now();

    const auto chain_distance_begin = Clock::now();
    std::vector<double> chain_squared_distances(options.point_count - 1U);
    parallel_shards(
        options.point_count - 1U,
        used_cpu_workers,
        [&](std::size_t, std::size_t begin, std::size_t end) {
          for (std::size_t edge = begin; edge < end; ++edge) {
            chain_squared_distances[edge] = squared_distance(
                coordinates_by_point_id,
                point_ids_by_morton_position[edge],
                point_ids_by_morton_position[edge + 1U]);
          }
        });
    const auto chain_distance_end = Clock::now();

    HierarchyRun fast_hierarchy = build_surrogate_hierarchies(
        options.point_count,
        options.maximum_order,
        used_cpu_workers,
        point_ids_by_morton_position,
        morton_position_by_point_id,
        chain_squared_distances,
        knn,
        options.quality_pair_count != 0U);
    const auto fast_path_end = Clock::now();

    std::optional<ResidentReplayResult> resident_replay;
    if (options.resident_replay) {
      const auto replay_begin = Clock::now();
      const auto replay_launcher_begin = Clock::now();
      morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult replay_knn;
      std::optional<morsehgp3d::gpu::Binary64LbvhTopKAudit>
          replay_binary64_lbvh_audit;
      if (options.knn_backend == KnnBackend::binary64_lbvh) {
        morsehgp3d::gpu::Binary64LbvhTopKResult replay_binary64_result =
            binary64_lbvh_context->query_all(
                cloud, options.maximum_order, effective_window);
        replay_binary64_lbvh_audit.emplace(replay_binary64_result.audit);
        replay_knn = take_compatible_knn_transcript(
            std::move(replay_binary64_result));
      } else {
        replay_knn = morsehgp3d::gpu::detail::
            run_phase14_morton_window_knn_on_gpu(
                coordinates_by_morton_position,
                point_ids_by_morton_position,
                options.maximum_order,
                effective_window);
      }
      const auto replay_launcher_end = Clock::now();
      if (replay_knn.neighbor_point_ids.size() !=
              knn.neighbor_point_ids.size() ||
          replay_knn.squared_distances.size() !=
              knn.squared_distances.size()) {
        throw std::runtime_error(
            "the resident replay returned a different transcript extent");
      }
      std::atomic<bool> transcript_mismatch{false};
      parallel_shards(
          neighbor_count,
          used_cpu_workers,
          [&](std::size_t, std::size_t begin, std::size_t end) {
            for (std::size_t record = begin; record < end; ++record) {
              if (replay_knn.neighbor_point_ids[record] !=
                      knn.neighbor_point_ids[record] ||
                  std::bit_cast<std::uint64_t>(
                      replay_knn.squared_distances[record]) !=
                      std::bit_cast<std::uint64_t>(
                          knn.squared_distances[record])) {
                transcript_mismatch.store(true, std::memory_order_relaxed);
              }
            }
          });
      HierarchyRun replay_hierarchy = build_surrogate_hierarchies(
          options.point_count,
          options.maximum_order,
          used_cpu_workers,
          point_ids_by_morton_position,
          morton_position_by_point_id,
          chain_squared_distances,
          replay_knn,
          false);
      const auto replay_end = Clock::now();
      resident_replay.emplace(ResidentReplayResult{
          std::move(replay_knn),
          std::move(replay_binary64_lbvh_audit),
          std::move(replay_hierarchy),
          !transcript_mismatch.load(std::memory_order_relaxed),
          nanoseconds(replay_launcher_end - replay_launcher_begin),
          nanoseconds(replay_end - replay_begin)});
    }

    std::optional<morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult>
        reference_knn;
    std::optional<HierarchyRun> reference_hierarchy;
    std::optional<NeighborComparison> reference_comparison;
    std::uint64_t reference_gpu_wall_nanoseconds = 0U;
    std::uint64_t reference_validation_nanoseconds = 0U;
    std::size_t effective_reference_window = 0U;
    if (options.reference_morton_window_radius.has_value()) {
      effective_reference_window = std::min(
          *options.reference_morton_window_radius,
          options.point_count - 1U);
      if (effective_reference_window <= effective_window) {
        throw std::invalid_argument(
            "the effective reference window must exceed the fast window");
      }
      const auto reference_gpu_begin = Clock::now();
      reference_knn.emplace(
          morsehgp3d::gpu::detail::run_phase14_morton_window_knn_on_gpu(
              coordinates_by_morton_position,
              point_ids_by_morton_position,
              options.maximum_order,
              effective_reference_window));
      const auto reference_gpu_end = Clock::now();
      reference_gpu_wall_nanoseconds =
          nanoseconds(reference_gpu_end - reference_gpu_begin);
      const auto reference_validation_begin = Clock::now();
      validate_knn_transcript(
          *reference_knn,
          point_ids_by_morton_position,
          options.point_count,
          options.maximum_order,
          used_cpu_workers,
          "the reference-window GPU transcript");
      reference_comparison.emplace(compare_neighbors(
          knn,
          reference_knn->neighbor_point_ids,
          options.point_count,
          options.maximum_order));
      reference_validation_nanoseconds = nanoseconds(
          Clock::now() - reference_validation_begin);
      reference_hierarchy.emplace(build_surrogate_hierarchies(
          options.point_count,
          options.maximum_order,
          used_cpu_workers,
          point_ids_by_morton_position,
          morton_position_by_point_id,
          chain_squared_distances,
          *reference_knn,
          options.quality_pair_count != 0U));
    }

    std::optional<HierarchyQuality> hierarchy_quality;
    if (options.quality_pair_count != 0U) {
      const std::uint64_t quality_seed = splitmix64(
          options.seed ^ UINT64_C(0x51a17c0f3e2d9b47));
      hierarchy_quality.emplace(evaluate_hierarchy_quality(
          fast_hierarchy,
          *reference_hierarchy,
          options.point_count,
          options.quality_pair_count,
          quality_seed));
    }

    std::optional<ExhaustiveValidation> exhaustive_validation;
    if (options.validation_point_count != 0U) {
      exhaustive_validation.emplace(validate_against_exhaustive_binary64_knn(
          options.validation_point_count,
          options.point_count,
          options.maximum_order,
          used_cpu_workers,
          coordinates_by_point_id,
          point_ids_by_morton_position,
          knn));
    }
    const auto total_end = Clock::now();

    std::size_t gpu_candidate_distance_evaluations = 0U;
    if (binary64_lbvh_audit.has_value()) {
      if (binary64_lbvh_audit->seed_distance_evaluation_count >
          std::numeric_limits<std::size_t>::max() -
              binary64_lbvh_audit->
                  traversal_leaf_distance_evaluation_count) {
        throw std::length_error(
            "the GPU candidate evaluation counter overflows size_t");
      }
      gpu_candidate_distance_evaluations =
          binary64_lbvh_audit->seed_distance_evaluation_count +
          binary64_lbvh_audit->traversal_leaf_distance_evaluation_count;
    } else {
      for (std::size_t position = 0U;
           position < options.point_count;
           ++position) {
        const std::size_t left = std::min(position, effective_window);
        const std::size_t right = std::min(
            options.point_count - 1U - position, effective_window);
        if (left > std::numeric_limits<std::size_t>::max() - right ||
            gpu_candidate_distance_evaluations >
                std::numeric_limits<std::size_t>::max() - left - right) {
          throw std::length_error(
              "the GPU candidate evaluation counter overflows size_t");
        }
        gpu_candidate_distance_evaluations += left + right;
      }
    }
    std::size_t reference_candidate_distance_evaluations = 0U;
    if (reference_knn.has_value()) {
      for (std::size_t position = 0U;
           position < options.point_count;
           ++position) {
        const std::size_t left =
            std::min(position, effective_reference_window);
        const std::size_t right = std::min(
            options.point_count - 1U - position,
            effective_reference_window);
        if (left > std::numeric_limits<std::size_t>::max() - right ||
            reference_candidate_distance_evaluations >
                std::numeric_limits<std::size_t>::max() - left - right) {
          throw std::length_error(
              "the reference candidate evaluation counter overflows size_t");
        }
        reference_candidate_distance_evaluations += left + right;
      }
    }

    const std::uint64_t fast_path_nanoseconds =
        nanoseconds(fast_path_end - total_begin);
    const std::uint64_t total_nanoseconds = nanoseconds(total_end - total_begin);
    const std::uint64_t canonicalization_nanoseconds =
        nanoseconds(canonicalization_end - canonicalization_begin);
    const std::uint64_t morton_build_nanoseconds =
        nanoseconds(morton_build_end - morton_build_begin);
    const std::uint64_t coordinate_export_nanoseconds =
        nanoseconds(coordinate_export_end - coordinate_export_begin);
    const std::uint64_t morton_pack_nanoseconds =
        nanoseconds(morton_pack_end - morton_pack_begin);
    const std::uint64_t chain_distance_nanoseconds =
        nanoseconds(chain_distance_end - chain_distance_begin);
    std::optional<std::uint64_t> reconstructed_input_to_result_nanoseconds;
    if (resident_replay.has_value()) {
      std::uint64_t estimate = 0U;
      checked_accumulate_nanoseconds(estimate, canonicalization_nanoseconds);
      checked_accumulate_nanoseconds(estimate, morton_build_nanoseconds);
      checked_accumulate_nanoseconds(estimate, coordinate_export_nanoseconds);
      checked_accumulate_nanoseconds(estimate, morton_pack_nanoseconds);
      checked_accumulate_nanoseconds(estimate, chain_distance_nanoseconds);
      checked_accumulate_nanoseconds(
          estimate, resident_replay->replay_total_nanoseconds);
      reconstructed_input_to_result_nanoseconds = estimate;
    }
    std::cout << std::setprecision(17)
              << "{\"schema_version\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "morsehgp3d.phase14.binary64_lbvh_h0_surrogate.v1"
                         : "morsehgp3d.phase14.morton_window_h0_surrogate.v1")
              << ",\"git_sha\":" << std::quoted(MORSEHGP3D_GIT_SHA)
              << ",\"phase\":14"
              << ",\"backend\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "cuda_binary64_lbvh_top_k"
                         : "cuda_heuristic_knn")
              << ",\"profile\":\"hgp_reduced_surrogate\""
              << ",\"mode\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "stackless_aabb_branch_and_bound"
                         : "morton_window_knn")
              << ",\"input_family\":\"splitmix_uniform_binary64_with_injective_x\""
              << ",\"seed_u64\":" << std::quoted(hex64(options.seed))
              << ",\"public_status\":\"not_claimed\""
              << ",\"approximation_status\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "complete_binary64_neighbor_diagnostic"
                         : "heuristic")
              << ",\"morse_faithfulness\":\"not_certified\""
              << ",\"binary64_arithmetic_status\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "fixed_recipe_not_yet_formally_qualified"
                         : "not_certified")
              << ",\"hierarchy_model\":\"morton_chain_plus_rank_knn_mutual_reachability\""
              << ",\"knn_edge_policy\":\"rank_k_only_plus_morton_chain\""
              << ",\"standard_mutual_reachability_graph\":false"
              << ",\"full_surrogate_h0_hierarchy_materialized\":true"
              << ",\"surrogate_hierarchy_retained_after_digest\":"
              << (options.quality_pair_count != 0U ? "true" : "false")
              << ",\"exact_morse_hierarchy_materialized\":false"
              << ",\"candidate_recall_certified\":false"
              << ",\"higher_order_delaunay_materialized\":false"
              << ",\"gamma_graph_materialized\":false"
              << ",\"cofaces_materialized\":false"
              << ",\"global_pair_matrix_materialized\":false"
              << ",\"point_count\":" << options.point_count
              << ",\"maximum_order\":" << options.maximum_order
              << ",\"requested_morton_window_radius\":"
              << options.morton_window_radius
              << ",\"effective_morton_window_radius\":" << effective_window
              << ",\"morton_window_exhaustive\":"
              << (options.knn_backend == KnnBackend::morton_window &&
                          effective_window == options.point_count - 1U
                      ? "true"
                      : "false")
              << ",\"morton_window_role\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "upper_bound_seed_only"
                         : "candidate_restriction")
              << ",\"knn_branch_and_bound_complete\":"
              << (binary64_lbvh_audit.has_value() &&
                          binary64_lbvh_audit->complete_query_coverage
                      ? "true"
                      : "false")
              << ",\"neighbors_returned\":" << neighbor_count
              << ",\"gpu_candidate_distance_evaluation_count\":"
              << gpu_candidate_distance_evaluations
              << ",\"cpu_workers_requested\":"
              << options.requested_cpu_workers
              << ",\"cpu_workers_available\":" << available_cpu_workers
              << ",\"cpu_workers_used_for_shards\":" << used_cpu_workers
              << ",\"cpu_workers_used_for_order_reductions\":"
              << fast_hierarchy.order_worker_count
              << ",\"cuda_device\":" << knn.cuda_device
              << ",\"cuda_device_name\":" << std::quoted(knn.device_name)
              << ",\"cuda_multiprocessor_count\":"
              << knn.multiprocessor_count
              << ",\"gpu_kernel_launch_count\":"
              << (1 + (resident_replay.has_value() ? 1 : 0) +
                  (reference_knn.has_value() ? 1 : 0))
              << ",\"gpu_kernel_block_count\":" << knn.kernel_block_count
              << ",\"gpu_kernel_thread_count\":" << knn.kernel_thread_count
              << ",\"gpu_device_byte_capacity\":" << knn.device_byte_capacity
              << ",\"timings_nanoseconds\":{"
              << "\"generation\":" << nanoseconds(generation_end - generation_begin)
              << ",\"canonicalization\":"
              << canonicalization_nanoseconds
              << ",\"morton_lbvh_build_backend\":"
              << std::quoted(
                     options.knn_backend == KnnBackend::binary64_lbvh
                         ? "cuda_certified_snapshot_import"
                         : "reference_cpu")
              << ",\"morton_lbvh_build\":"
              << morton_build_nanoseconds
              << ",\"coordinate_export\":"
              << coordinate_export_nanoseconds
              << ",\"morton_pack\":"
              << morton_pack_nanoseconds
              << ",\"gpu_allocation\":" << knn.allocation_nanoseconds
              << ",\"gpu_h2d\":" << knn.host_to_device_nanoseconds
              << ",\"gpu_kernel\":" << knn.kernel_nanoseconds
              << ",\"gpu_d2h\":" << knn.device_to_host_nanoseconds
              << ",\"gpu_launcher_wall\":"
              << nanoseconds(gpu_wall_end - gpu_wall_begin)
              << ",\"gpu_output_structural_validation\":"
              << nanoseconds(validation_end - validation_begin)
              << ",\"cpu_morton_chain_distances\":"
              << chain_distance_nanoseconds
              << ",\"cpu_parallel_edge_build\":"
              << fast_hierarchy.edge_build_nanoseconds
              << ",\"cpu_parallel_hierarchy_reduction\":"
              << fast_hierarchy.hierarchy_reduction_nanoseconds
              << ",\"fast_cold_total\":" << fast_path_nanoseconds
              << ",\"diagnostic_total_with_optional_validation\":"
              << total_nanoseconds << "}"
              << ",\"single_fast_cold_total_under_100ms\":"
              << (fast_path_nanoseconds < UINT64_C(100000000) ? "true" : "false")
              << ",\"latency_objective_status\":\"unassessed_single_cold_run\""
              << ",\"warm_p95_claimed\":false"
              << ",\"orders\":[";
    for (std::size_t order = 0U;
         order < fast_hierarchy.summaries.size();
         ++order) {
      if (order != 0U) {
        std::cout << ',';
      }
      const OrderSummary& summary = fast_hierarchy.summaries[order];
      std::cout << "{\"order\":" << summary.order
                << ",\"proposed_edge_count\":"
                << summary.proposed_edge_count
                << ",\"unique_edge_count\":" << summary.unique_edge_count
                << ",\"materialized_merge_record_count\":"
                << summary.materialized_merge_record_count
                << ",\"distinct_level_count\":"
                << summary.distinct_level_count
                << ",\"final_component_count\":"
                << summary.final_component_count
                << ",\"root_squared_level\":" << summary.root_squared_level
                << ",\"digest\":" << std::quoted(hex64(summary.digest))
                << ",\"cpu_nanoseconds\":" << summary.cpu_nanoseconds
                << '}';
    }
    std::cout << ']';
    if (binary64_lbvh_audit.has_value()) {
      const morsehgp3d::gpu::Binary64LbvhTopKAudit& audit =
          *binary64_lbvh_audit;
      std::cout
          << ",\"binary64_lbvh_top_k\":{"
          << "\"status\":\"complete_diagnostic_transcript\""
          << ",\"scientific_morse_status\":\"not_certified\""
          << ",\"seed_window_radius\":" << audit.seed_window_radius
          << ",\"seed_distance_evaluation_count\":"
          << audit.seed_distance_evaluation_count
          << ",\"traversal_leaf_distance_evaluation_count\":"
          << audit.traversal_leaf_distance_evaluation_count
          << ",\"node_visit_count\":" << audit.node_visit_count
          << ",\"strict_aabb_prune_count\":"
          << audit.strict_aabb_prune_count
          << ",\"seed_covered_subtree_skip_count\":"
          << audit.seed_covered_subtree_skip_count
          << ",\"invalid_aabb_bound_descent_count\":"
          << audit.invalid_aabb_bound_descent_count
          << ",\"maximum_node_visit_count_per_query\":"
          << audit.maximum_node_visit_count_per_query
          << ",\"median_node_visit_count_per_query\":"
          << audit.median_node_visit_count_per_query
          << ",\"p95_node_visit_count_per_query\":"
          << audit.p95_node_visit_count_per_query
          << ",\"p99_node_visit_count_per_query\":"
          << audit.p99_node_visit_count_per_query
          << ",\"full_tree_query_count\":"
          << audit.full_tree_query_count
          << ",\"completed_query_count\":"
          << audit.completed_query_count
          << ",\"failed_query_count\":" << audit.failed_query_count
          << ",\"fixed_round_to_nearest_distance_recipe_requested\":"
          << (audit.fixed_round_to_nearest_distance_recipe_requested
                  ? "true"
                  : "false")
          << ",\"directed_round_down_aabb_recipe_requested\":"
          << (audit.directed_round_down_aabb_recipe_requested
                  ? "true"
                  : "false")
          << ",\"strict_prune_requested\":"
          << (audit.strict_prune_requested ? "true" : "false")
          << ",\"stackless_postorder_traversal_requested\":"
          << (audit.stackless_postorder_traversal_requested
                  ? "true"
                  : "false")
          << ",\"complete_query_coverage\":"
          << (audit.complete_query_coverage ? "true" : "false")
          << ",\"no_candidate_truncation\":"
          << (audit.no_candidate_truncation ? "true" : "false")
          << ",\"persistent_input_device_byte_capacity\":"
          << audit.persistent_input_device_byte_capacity
          << ",\"transient_output_device_byte_capacity\":"
          << audit.transient_output_device_byte_capacity
          << '}';
    }
    if (gpu_morton_build_audit.has_value()) {
      std::cout
          << ",\"cuda_morton_lbvh_build\":{"
          << "\"snapshot_import_certified\":"
          << (gpu_morton_build_audit->snapshot_import_certified
                  ? "true"
                  : "false")
          << ",\"gpu_execution_performed\":"
          << (gpu_morton_build_audit->gpu_execution_performed
                  ? "true"
                  : "false")
          << ",\"device_kernel_launch_count\":"
          << gpu_morton_build_audit->device_kernel_launch_count
          << ",\"device_library_submission_count\":"
          << gpu_morton_build_audit->device_library_submission_count
          << ",\"total_fixed_device_byte_capacity\":"
          << gpu_morton_build_audit->total_fixed_device_byte_capacity
          << '}';
    }
    if (resident_replay.has_value()) {
      std::cout
          << ",\"resident_replay\":{"
          << "\"executed\":true"
          << ",\"resident_input_prepared\":true"
          << ",\"resident_input_scope\":"
          << std::quoted(
                 options.knn_backend == KnnBackend::binary64_lbvh
                     ? "certified_device_coordinates_morton_order_and_lbvh"
                     : "host_coordinates_and_morton_order")
          << ",\"device_state_reused\":"
          << (options.knn_backend == KnnBackend::binary64_lbvh
                  ? "true"
                  : "false")
          << ",\"formal_warm_e2e_protocol\":false"
          << ",\"transcript_comparison\":\"point_ids_and_distance_bits\""
          << ",\"transcript_match\":"
          << (resident_replay->transcript_match ? "true" : "false")
          << ",\"gpu_allocation_ns\":"
          << resident_replay->knn.allocation_nanoseconds
          << ",\"gpu_h2d_ns\":"
          << resident_replay->knn.host_to_device_nanoseconds
          << ",\"gpu_kernel_ns\":"
          << resident_replay->knn.kernel_nanoseconds
          << ",\"gpu_d2h_ns\":"
          << resident_replay->knn.device_to_host_nanoseconds
          << ",\"gpu_launcher_wall_ns\":"
          << resident_replay->launcher_wall_nanoseconds
          << ",\"cpu_parallel_edge_build_ns\":"
          << resident_replay->hierarchy.edge_build_nanoseconds
          << ",\"cpu_parallel_hierarchy_reduction_ns\":"
          << resident_replay->hierarchy.hierarchy_reduction_nanoseconds
          << ",\"replay_total_ns\":"
          << resident_replay->replay_total_nanoseconds
          << ",\"reconstructed_input_to_result_ns\":"
          << *reconstructed_input_to_result_nanoseconds
          << ",\"reconstructed_input_to_result_status\":\"diagnostic_estimate_not_protocol\""
          << ",\"reconstructed_input_to_result_under_100ms\":"
          << (*reconstructed_input_to_result_nanoseconds <
                      UINT64_C(100000000)
                  ? "true"
                  : "false")
          << ",\"per_order\":[";
      for (std::size_t order = 0U; order < options.maximum_order; ++order) {
        if (order != 0U) {
          std::cout << ',';
        }
        const OrderSummary& initial = fast_hierarchy.summaries[order];
        const OrderSummary& replay =
            resident_replay->hierarchy.summaries[order];
        std::cout
            << "{\"order\":" << order + 1U
            << ",\"digest_match\":"
            << (initial.digest == replay.digest ? "true" : "false")
            << ",\"root_squared_level_match\":"
            << (std::bit_cast<std::uint64_t>(initial.root_squared_level) ==
                        std::bit_cast<std::uint64_t>(
                            replay.root_squared_level)
                    ? "true"
                    : "false")
            << ",\"replay_digest\":"
            << std::quoted(hex64(replay.digest))
            << ",\"replay_root_squared_level\":"
            << replay.root_squared_level << '}';
      }
      std::cout << "]}";
    } else {
      std::cout
          << ",\"resident_replay\":{\"executed\":false"
          << ",\"formal_warm_e2e_protocol\":false"
          << ",\"reconstructed_input_to_result_ns\":null"
          << ",\"reconstructed_input_to_result_status\":\"diagnostic_estimate_not_protocol\"}";
    }
    if (reference_knn.has_value()) {
      const NeighborComparison& comparison = *reference_comparison;
      const std::size_t final_prefix = options.maximum_order - 1U;
      const double recall =
          static_cast<double>(comparison.prefix_match_counts[final_prefix]) /
          static_cast<double>(comparison.prefix_totals[final_prefix]);
      std::cout
          << ",\"reference_window\":{"
          << "\"requested_radius\":"
          << *options.reference_morton_window_radius
          << ",\"effective_radius\":" << effective_reference_window
          << ",\"reference_window_exhaustive\":"
          << (effective_reference_window == options.point_count - 1U
                  ? "true"
                  : "false")
          << ",\"arithmetic_status\":\"binary64_not_certified\""
          << ",\"reference_morse_faithfulness\":\"not_certified\""
          << ",\"reference_is_wider_heuristic_unless_exhaustive\":true"
          << ",\"candidate_distance_evaluation_count\":"
          << reference_candidate_distance_evaluations
          << ",\"neighbor_prefix_set_matches\":"
          << comparison.prefix_match_counts[final_prefix]
          << ",\"total_neighbors\":"
          << comparison.prefix_totals[final_prefix]
          << ",\"neighbor_recall\":" << recall
          << ",\"same_rank_neighbor_id_matches\":"
          << comparison.exact_rank_match_count
          << ",\"rank_comparison_total\":"
          << comparison.total_neighbor_count
          << ",\"gpu_device_byte_capacity\":"
          << reference_knn->device_byte_capacity
          << ",\"timings_nanoseconds\":{"
          << "\"gpu_allocation\":"
          << reference_knn->allocation_nanoseconds
          << ",\"gpu_h2d\":"
          << reference_knn->host_to_device_nanoseconds
          << ",\"gpu_kernel\":" << reference_knn->kernel_nanoseconds
          << ",\"gpu_d2h\":"
          << reference_knn->device_to_host_nanoseconds
          << ",\"gpu_launcher_wall\":"
          << reference_gpu_wall_nanoseconds
          << ",\"comparison_validation\":"
          << reference_validation_nanoseconds
          << ",\"cpu_parallel_edge_build\":"
          << reference_hierarchy->edge_build_nanoseconds
          << ",\"cpu_parallel_hierarchy_reduction\":"
          << reference_hierarchy->hierarchy_reduction_nanoseconds
          << "},\"per_order\":[";
      for (std::size_t order = 0U; order < options.maximum_order; ++order) {
        if (order != 0U) {
          std::cout << ',';
        }
        const double order_recall =
            static_cast<double>(comparison.prefix_match_counts[order]) /
            static_cast<double>(comparison.prefix_totals[order]);
        const OrderSummary& fast_summary = fast_hierarchy.summaries[order];
        const OrderSummary& reference_summary =
            reference_hierarchy->summaries[order];
        std::cout
            << "{\"order\":" << order + 1U
            << ",\"neighbor_matches\":"
            << comparison.prefix_match_counts[order]
            << ",\"neighbor_total\":" << comparison.prefix_totals[order]
            << ",\"neighbor_recall\":" << order_recall
            << ",\"hierarchy_digest_match\":"
            << (fast_summary.digest == reference_summary.digest
                    ? "true"
                    : "false")
            << ",\"root_squared_level_match\":"
            << (std::bit_cast<std::uint64_t>(fast_summary.root_squared_level) ==
                        std::bit_cast<std::uint64_t>(
                            reference_summary.root_squared_level)
                    ? "true"
                    : "false")
            << ",\"reference_digest\":"
            << std::quoted(hex64(reference_summary.digest))
            << ",\"reference_root_squared_level\":"
            << reference_summary.root_squared_level << '}';
      }
      std::cout << "]}";
    } else {
      std::cout << ",\"reference_window\":null";
    }

    if (hierarchy_quality.has_value()) {
      const HierarchyQuality& quality = *hierarchy_quality;
      const AggregateHierarchyQuality& aggregate = quality.aggregate;
      const double aggregate_cluster_agreement =
          static_cast<double>(aggregate.cluster_agreement_count) /
          static_cast<double>(aggregate.cluster_comparison_count);
      std::cout
          << ",\"hierarchy_quality\":{"
          << "\"executed\":true"
          << ",\"reference_scope\":"
          << std::quoted(
                 effective_reference_window == options.point_count - 1U
                     ? "exhaustive_binary64_neighbors_within_same_rank_k_plus_morton_chain_surrogate_not_exact_morse"
                     : "wider_morton_window_binary64_neighbors_within_same_rank_k_plus_morton_chain_surrogate_not_exact_morse")
          << ",\"reference_window_exhaustive\":"
          << (effective_reference_window == options.point_count - 1U
                  ? "true"
                  : "false")
          << ",\"arithmetic_status\":\"binary64_not_certified\""
          << ",\"digest_only_is_sufficient_quality_metric\":false"
          << ",\"retained_mst_merges_for_quality\":true"
          << ",\"retained_mst_merges_outside_quality_mode\":false"
          << ",\"level_unit\":\"squared_input_coordinate\""
          << ",\"sample_pair_count_requested\":"
          << options.quality_pair_count
          << ",\"sample_pair_count_used\":" << quality.sampled_pair_count
          << ",\"sample_pair_hard_cap\":" << kMaximumQualityPairCount
          << ",\"sample_seed_u64\":"
          << std::quoted(hex64(quality.sample_seed))
          << ",\"sample_policy\":\"splitmix64_canonical_point_id_pairs_with_replacement\""
          << ",\"cpu_nanoseconds\":" << quality.cpu_nanoseconds
          << ",\"aggregate\":{\"barcode_h0\":";
      write_barcode_quality_json(aggregate.barcode);
      std::cout << ",\"cophenetic_single_linkage\":";
      write_cophenetic_quality_json(aggregate.cophenetic);
      std::cout
          << ",\"cluster_co_membership\":{"
          << "\"quantile_threshold_count_per_order\":9"
          << ",\"agreement_count\":"
          << aggregate.cluster_agreement_count
          << ",\"comparison_count\":"
          << aggregate.cluster_comparison_count
          << ",\"agreement\":" << aggregate_cluster_agreement
          << "}},\"per_order\":[";
      for (std::size_t order = 0U; order < quality.orders.size(); ++order) {
        if (order != 0U) {
          std::cout << ',';
        }
        const OrderHierarchyQuality& order_quality = quality.orders[order];
        const double cluster_agreement =
            static_cast<double>(order_quality.cluster_agreement_count) /
            static_cast<double>(order_quality.cluster_comparison_count);
        std::cout << "{\"order\":" << order_quality.order
                  << ",\"barcode_h0\":";
        write_barcode_quality_json(order_quality.barcode);
        std::cout << ",\"cophenetic_single_linkage\":";
        write_cophenetic_quality_json(order_quality.cophenetic);
        std::cout
            << ",\"cluster_co_membership\":{"
            << "\"quantile_source\":\"reference_sampled_pair_cophenetic_squared_levels\""
            << ",\"quantile_rule\":\"floor(percent*(sample_pair_count-1)/100)\""
            << ",\"agreement_count\":"
            << order_quality.cluster_agreement_count
            << ",\"comparison_count\":"
            << order_quality.cluster_comparison_count
            << ",\"agreement\":" << cluster_agreement
            << ",\"thresholds\":[";
        for (std::size_t threshold = 0U;
             threshold < order_quality.cluster_thresholds.size();
             ++threshold) {
          if (threshold != 0U) {
            std::cout << ',';
          }
          const ClusterThresholdQuality& value =
              order_quality.cluster_thresholds[threshold];
          const double agreement =
              static_cast<double>(value.agreement_count) /
              static_cast<double>(value.pair_count);
          std::cout
              << "{\"quantile_percent\":" << value.quantile_percent
              << ",\"reference_squared_level\":"
              << value.reference_squared_level
              << ",\"agreement_count\":" << value.agreement_count
              << ",\"pair_count\":" << value.pair_count
              << ",\"agreement\":" << agreement
              << ",\"both_same_cluster_count\":"
              << value.both_same_cluster_count
              << ",\"both_different_cluster_count\":"
              << value.both_different_cluster_count
              << ",\"fast_only_same_cluster_count\":"
              << value.fast_only_same_cluster_count
              << ",\"reference_only_same_cluster_count\":"
              << value.reference_only_same_cluster_count << '}';
        }
        std::cout << "]}}";
      }
      std::cout << "]}";
    } else {
      std::cout << ",\"hierarchy_quality\":null";
    }

    if (exhaustive_validation.has_value()) {
      const ExhaustiveValidation& validation = *exhaustive_validation;
      const NeighborComparison& comparison = validation.comparison;
      const std::size_t final_prefix = options.maximum_order - 1U;
      const double recall =
          static_cast<double>(comparison.prefix_match_counts[final_prefix]) /
          static_cast<double>(comparison.prefix_totals[final_prefix]);
      std::cout
          << ",\"exhaustive_binary64_validation\":{"
          << "\"query_count\":" << validation.query_count
          << ",\"candidate_point_count_per_query\":"
          << options.point_count - 1U
          << ",\"distance_evaluation_count\":"
          << validation.distance_evaluation_count
          << ",\"distance_evaluation_hard_cap\":"
          << kMaximumValidationDistanceEvaluationCount
          << ",\"candidate_enumeration_exhaustive\":true"
          << ",\"arithmetic_status\":\"binary64_not_certified\""
          << ",\"morse_faithfulness\":\"not_evaluated\""
          << ",\"neighbor_prefix_set_matches\":"
          << comparison.prefix_match_counts[final_prefix]
          << ",\"total_neighbors\":"
          << comparison.prefix_totals[final_prefix]
          << ",\"neighbor_recall\":" << recall
          << ",\"same_rank_neighbor_id_matches\":"
          << comparison.exact_rank_match_count
          << ",\"rank_comparison_total\":"
          << comparison.total_neighbor_count
          << ",\"cpu_nanoseconds\":" << validation.cpu_nanoseconds
          << ",\"per_order\":[";
      for (std::size_t order = 0U; order < options.maximum_order; ++order) {
        if (order != 0U) {
          std::cout << ',';
        }
        const double order_recall =
            static_cast<double>(comparison.prefix_match_counts[order]) /
            static_cast<double>(comparison.prefix_totals[order]);
        std::cout << "{\"order\":" << order + 1U
                  << ",\"neighbor_matches\":"
                  << comparison.prefix_match_counts[order]
                  << ",\"neighbor_total\":"
                  << comparison.prefix_totals[order]
                  << ",\"neighbor_recall\":" << order_recall << '}';
      }
      std::cout << "]}";
    } else {
      std::cout << ",\"exhaustive_binary64_validation\":null";
    }
    std::cout << "}\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "gpu_morton_window_h0_surrogate: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
