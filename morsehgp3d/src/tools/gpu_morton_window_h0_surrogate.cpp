#include "phase14_morton_window_knn_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
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

struct Options {
  std::size_t point_count{50'000U};
  std::size_t maximum_order{10U};
  std::size_t morton_window_radius{256U};
  std::optional<std::size_t> reference_morton_window_radius;
  std::size_t validation_point_count{};
  bool resident_replay{false};
  std::size_t requested_cpu_workers{};
  std::uint64_t seed{UINT64_C(0x14a750c0ffee)};
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
  HierarchyRun hierarchy;
  bool transcript_match{};
  std::uint64_t launcher_wall_nanoseconds{};
  std::uint64_t replay_total_nanoseconds{};
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
    } else if (argument == "--reference-morton-window" && index + 1 < argc) {
      options.reference_morton_window_radius =
          parse_size(argv[++index], "invalid --reference-morton-window");
    } else if (argument == "--validation-points" && index + 1 < argc) {
      options.validation_point_count =
          parse_nonnegative_size(argv[++index], "invalid --validation-points");
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
          "[--max-order K] [--morton-window W] [--cpu-workers N] "
          "[--reference-morton-window W2] [--validation-points N] "
          "[--resident-replay] [--seed N]");
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
    std::size_t point_count) {
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
  return OrderSummary{
      order_index + 1U,
      proposed_count,
      edges.size(),
      materialized_merges.size(),
      distinct_level_count,
      component_count,
      root_squared_level,
      digest,
      nanoseconds(end - begin)};
}

[[nodiscard]] HierarchyRun build_surrogate_hierarchies(
    std::size_t point_count,
    std::size_t maximum_order,
    std::size_t used_cpu_workers,
    const std::vector<PointId>& point_ids_by_morton_position,
    const std::vector<std::size_t>& morton_position_by_point_id,
    const std::vector<double>& chain_squared_distances,
    const morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult& knn) {
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
              order, order_edges[order], point_count);
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

[[nodiscard]] std::string hex64(std::uint64_t value) {
  std::ostringstream output;
  output << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return output.str();
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
    morsehgp3d::spatial::MortonLbvhIndex morton_index =
        morsehgp3d::spatial::MortonLbvhIndex::build(cloud);
    const auto morton_build_end = Clock::now();
    const std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves =
        morton_index.leaves();
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
    morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult knn =
        morsehgp3d::gpu::detail::run_phase14_morton_window_knn_on_gpu(
            coordinates_by_morton_position,
            point_ids_by_morton_position,
            options.maximum_order,
            effective_window);
    const auto gpu_wall_end = Clock::now();
    if (knn.neighbor_point_ids.size() != neighbor_count ||
        knn.squared_distances.size() != neighbor_count) {
      throw std::runtime_error("the GPU returned a wrong neighbor extent");
    }

    const auto validation_begin = Clock::now();
    std::atomic<bool> invalid_gpu_output{false};
    parallel_shards(
        options.point_count,
        used_cpu_workers,
        [&](std::size_t, std::size_t begin, std::size_t end) {
          for (std::size_t position = begin; position < end; ++position) {
            const PointId source = point_ids_by_morton_position[position];
            for (std::size_t rank = 0U; rank < options.maximum_order; ++rank) {
              const std::size_t record =
                  position * options.maximum_order + rank;
              const PointId neighbor = knn.neighbor_point_ids[record];
              const double distance = knn.squared_distances[record];
              if (neighbor >= static_cast<PointId>(options.point_count) ||
                  neighbor == source || !std::isfinite(distance) ||
                  distance < 0.0) {
                invalid_gpu_output.store(true, std::memory_order_relaxed);
                continue;
              }
              if (rank != 0U) {
                const std::size_t previous = record - 1U;
                const double previous_distance =
                    knn.squared_distances[previous];
                const PointId previous_id = knn.neighbor_point_ids[previous];
                if (distance < previous_distance ||
                    (distance == previous_distance && neighbor <= previous_id)) {
                  invalid_gpu_output.store(true, std::memory_order_relaxed);
                }
              }
            }
          }
        });
    if (invalid_gpu_output.load(std::memory_order_relaxed)) {
      throw std::runtime_error(
          "the GPU Morton-window result failed its structural validation");
    }
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
        knn);
    const auto fast_path_end = Clock::now();

    std::optional<ResidentReplayResult> resident_replay;
    if (options.resident_replay) {
      const auto replay_begin = Clock::now();
      const auto replay_launcher_begin = Clock::now();
      morsehgp3d::gpu::detail::Phase14MortonWindowKnnResult replay_knn =
          morsehgp3d::gpu::detail::run_phase14_morton_window_knn_on_gpu(
              coordinates_by_morton_position,
              point_ids_by_morton_position,
              options.maximum_order,
              effective_window);
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
          replay_knn);
      const auto replay_end = Clock::now();
      resident_replay.emplace(ResidentReplayResult{
          std::move(replay_knn),
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
      std::atomic<bool> invalid_reference_output{false};
      parallel_shards(
          options.point_count,
          used_cpu_workers,
          [&](std::size_t, std::size_t begin, std::size_t end) {
            for (std::size_t position = begin; position < end; ++position) {
              const PointId source = point_ids_by_morton_position[position];
              for (std::size_t rank = 0U;
                   rank < options.maximum_order;
                   ++rank) {
                const std::size_t record =
                    position * options.maximum_order + rank;
                const PointId neighbor =
                    reference_knn->neighbor_point_ids[record];
                const double distance =
                    reference_knn->squared_distances[record];
                if (neighbor >= static_cast<PointId>(options.point_count) ||
                    neighbor == source || !std::isfinite(distance) ||
                    distance < 0.0) {
                  invalid_reference_output.store(
                      true, std::memory_order_relaxed);
                }
              }
            }
          });
      if (invalid_reference_output.load(std::memory_order_relaxed)) {
        throw std::runtime_error(
            "the reference-window GPU result failed structural validation");
      }
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
          *reference_knn));
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
    for (std::size_t position = 0U; position < options.point_count; ++position) {
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
              << "{\"schema_version\":\"morsehgp3d.phase14.morton_window_h0_surrogate.v1\""
              << ",\"git_sha\":" << std::quoted(MORSEHGP3D_GIT_SHA)
              << ",\"phase\":14"
              << ",\"backend\":\"cuda_heuristic_knn\""
              << ",\"profile\":\"hgp_reduced_surrogate\""
              << ",\"mode\":\"morton_window_knn\""
              << ",\"input_family\":\"splitmix_uniform_binary64_with_injective_x\""
              << ",\"seed_u64\":" << std::quoted(hex64(options.seed))
              << ",\"public_status\":\"not_claimed\""
              << ",\"approximation_status\":\"heuristic\""
              << ",\"morse_faithfulness\":\"not_certified\""
              << ",\"binary64_arithmetic_status\":\"not_certified\""
              << ",\"hierarchy_model\":\"morton_chain_plus_rank_knn_mutual_reachability\""
              << ",\"knn_edge_policy\":\"rank_k_only_plus_morton_chain\""
              << ",\"standard_mutual_reachability_graph\":false"
              << ",\"full_surrogate_h0_hierarchy_materialized\":true"
              << ",\"surrogate_hierarchy_retained_after_digest\":false"
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
              << (effective_window == options.point_count - 1U ? "true" : "false")
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
              << ",\"cpu_morton_lbvh_build\":"
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
    if (resident_replay.has_value()) {
      std::cout
          << ",\"resident_replay\":{"
          << "\"executed\":true"
          << ",\"resident_input_prepared\":true"
          << ",\"resident_input_scope\":\"host_coordinates_and_morton_order\""
          << ",\"device_state_reused\":false"
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
