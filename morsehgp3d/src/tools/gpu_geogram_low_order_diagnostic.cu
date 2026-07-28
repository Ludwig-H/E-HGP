#include <cuda_runtime.h>

#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/extrema.h>
#include <thrust/functional.h>
#include <thrust/remove.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/transform_reduce.h>

#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/common.h>
#include <geogram/basic/process.h>
#include <geogram/delaunay/delaunay.h>

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/binary64_lbvh_top_k.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "gpu_geogram_low_order_diagnostic.cu must be compiled with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Geogram low-order diagnostic requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Geogram low-order diagnostic"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Geogram low-order diagnostic must contain only sm_120 code"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using PointId = std::uint64_t;

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr double kPredicateToleranceMultiplier = 1024.0;
constexpr std::size_t kDefaultTargetPointsPerCell = 8U;

struct Point3 {
  double x{};
  double y{};
  double z{};
};

struct Edge {
  PointId u{};
  PointId v{};

  friend bool operator==(const Edge&, const Edge&) = default;
};

struct Triangle {
  PointId a{};
  PointId b{};
  PointId c{};
};

enum class TriangleStatus : std::uint32_t {
  blocked = 0U,
  gabriel_binary64 = 1U,
  ambiguous = 2U,
  degenerate_or_invalid = 3U,
};

struct ClassifiedTriangle {
  Triangle triangle{};
  double squared_level{};
  std::uint64_t visited_cell_count{};
  std::uint64_t tested_point_count{};
  TriangleStatus status{TriangleStatus::degenerate_or_invalid};
  std::uint32_t support_cardinality{};
};

static_assert(std::is_trivially_copyable_v<Point3>);
static_assert(std::is_trivially_copyable_v<Edge>);
static_assert(std::is_trivially_copyable_v<Triangle>);
static_assert(std::is_trivially_copyable_v<ClassifiedTriangle>);

struct Options {
  std::size_t point_count{50'000U};
  std::uint64_t seed{UINT64_C(0x4d4f525345484750)};
  std::size_t cpu_workers{std::numeric_limits<std::size_t>::max()};
  std::size_t grid_resolution{};
  std::size_t triangle_chunk_vertices{};
  std::size_t surrogate_guardrail_max_order{};
  std::size_t surrogate_seed_window_radius{32U};
  std::size_t gabriel_neighbor_rank_maximum{};
  std::size_t neighbor_rank_seed_window_radius{256U};
  std::size_t neighbor_rank_query_batch_size{1U << 20U};
  std::optional<std::string> input_xyz;
  bool fixture_e5{};
  bool emit_records{};
  bool gabriel_coverage_only{};
  bool allow_conditional_guardrail{};
};

struct Timings {
  std::uint64_t input_nanoseconds{};
  std::uint64_t geogram_nanoseconds{};
  std::uint64_t edge_extraction_nanoseconds{};
  std::uint64_t csr_nanoseconds{};
  std::uint64_t grid_nanoseconds{};
  std::uint64_t triangle_enumeration_nanoseconds{};
  std::uint64_t triangle_classification_nanoseconds{};
  std::uint64_t triangle_compaction_sort_nanoseconds{};
  std::uint64_t triangle_device_to_host_nanoseconds{};
  std::uint64_t k1_reduction_nanoseconds{};
  std::uint64_t k2_reduction_nanoseconds{};
  std::uint64_t surrogate_guardrail_nanoseconds{};
  std::uint64_t neighbor_rank_diagnostic_nanoseconds{};
  std::uint64_t total_nanoseconds{};
};

struct TriangleAudit {
  std::size_t chunk_count{};
  std::uint64_t raw_wedge_count{};
  std::uint64_t canonical_candidate_count{};
  std::uint64_t blocked_count{};
  std::uint64_t accepted_count{};
  std::uint64_t ambiguous_count{};
  std::uint64_t invalid_count{};
  std::uint64_t visited_cell_count{};
  std::uint64_t tested_point_count{};
  std::size_t maximum_chunk_candidate_count{};
  std::size_t maximum_chunk_device_bytes{};
};

struct K1Summary {
  std::vector<std::tuple<double, PointId, PointId>> selected_edges;
  std::size_t distinct_level_count{};
  double root_squared_distance{};
  double root_squared_level{};
  std::uint64_t surrogate_compatible_digest{};
};

struct K2Summary {
  std::size_t facet_count{};
  std::size_t final_component_count{};
  std::size_t useful_union_count{};
  std::size_t redundant_union_count{};
  std::size_t distinct_level_count{};
  double first_squared_level{};
  double root_squared_level{};
  std::uint64_t accepted_triangle_digest{};
  std::size_t support_two_count{};
  std::size_t support_three_count{};
  std::size_t necessary_triangle_count{};
  std::size_t strictly_lower_connected_triangle_count{};
  std::size_t coverage_violation_count{};
  std::string accepted_triangle_sha256;
  std::string necessary_triangle_sha256;
  std::vector<ClassifiedTriangle> emitted_necessary_records;
};

struct ReconstructibleInputDigests {
  std::string point_cloud_sha256;
  std::string ordinary_delaunay_edges_sha256;
  std::string canonical_wedge_universe_sha256;
};

struct GabrielSafetyBatchManifest {
  std::size_t ambiguous_triangle_count{};
  std::size_t total_explicit_triangle_count{};
  std::string ambiguous_triangle_sha256;
  std::string composite_batch_sha256;
  ClassifiedTriangle first_ambiguous_triangle{};
  bool first_ambiguous_triangle_present{};
};

struct SurrogateTreeEdge {
  double squared_weight{};
  PointId u{};
  PointId v{};
};

static_assert(sizeof(SurrogateTreeEdge) == 24U);

enum class CorrectedLevelEncoding : std::uint64_t {
  raw_squared_weight_exact_dyadic_divide_by_4 = 1U,
  direct_Gabriel_squared_level = 2U,
};

struct CorrectedSurrogateTreeEdge {
  double encoded_binary64{};
  PointId u{};
  PointId v{};
  CorrectedLevelEncoding level_encoding{
      CorrectedLevelEncoding::direct_Gabriel_squared_level};
};

static_assert(sizeof(CorrectedSurrogateTreeEdge) == 32U);

struct SurrogateFailureWitness {
  ClassifiedTriangle source{};
  std::string classification;
  double observed_connection_raw_squared_weight{};
  bool observed_connection_level_present{};
  bool present{};
};

struct SurrogateGuardrailOrderSummary {
  std::size_t order{};
  std::size_t proposed_edge_count{};
  std::size_t unique_edge_count{};
  std::size_t tree_edge_count{};
  std::size_t distinct_tree_level_count{};
  double root_squared_weight{};
  std::size_t source_triangle_count{};
  std::size_t supported_triangle_count{};
  std::size_t connected_before_count{};
  std::size_t connected_at_count{};
  std::size_t late_count{};
  std::size_t never_count{};
  std::size_t unsupported_count{};
  std::size_t correction_triangle_count{};
  std::size_t useful_correction_union_count{};
  std::size_t corrected_postcondition_violation_count{};
  std::size_t corrected_connected_before_count{};
  std::size_t corrected_connected_at_count{};
  std::size_t corrected_late_count{};
  std::size_t corrected_never_count{};
  std::size_t corrected_unsupported_count{};
  std::size_t corrected_tree_edge_count{};
  std::size_t corrected_tree_distinct_level_count{};
  CorrectedSurrogateTreeEdge corrected_tree_root_edge{};
  std::string tree_sha256;
  std::string surrogate_compatible_digest;
  std::string decision_sha256;
  std::string correction_sha256;
  std::string corrected_tree_sha256;
  std::string corrected_decision_sha256;
  SurrogateFailureWitness first_failure;
  SurrogateFailureWitness first_late;
  SurrogateFailureWitness first_unsupported;
  SurrogateFailureWitness corrected_first_failure;
  SurrogateFailureWitness corrected_first_late;
  SurrogateFailureWitness corrected_first_unsupported;
  std::uint64_t edge_build_nanoseconds{};
  std::uint64_t tree_reduction_nanoseconds{};
  std::uint64_t deadline_replay_nanoseconds{};
  std::vector<SurrogateTreeEdge> emitted_tree_edges;
  std::vector<CorrectedSurrogateTreeEdge> emitted_corrected_tree_edges;
  std::vector<ClassifiedTriangle> emitted_correction_records;
};

struct SurrogateGuardrailRun {
  std::vector<SurrogateGuardrailOrderSummary> orders;
  morsehgp3d::gpu::Binary64LbvhTopKAudit top_k_audit;
  std::string canonical_to_source_mapping_sha256;
  std::string remapped_top_k_transcript_sha256;
  std::uint64_t canonicalization_nanoseconds{};
  std::uint64_t lbvh_build_nanoseconds{};
  std::uint64_t top_k_query_nanoseconds{};
  std::size_t order_worker_count{};
};

struct NeighborRankHistogram {
  std::vector<std::uint64_t> exact_rank_counts;
  std::uint64_t overflow_count{};
  std::uint64_t captured_count{};
  std::size_t maximum_exact_rank{};
};

struct NeighborRankVariantSummary {
  NeighborRankHistogram histogram;
  std::uint64_t witness_triangle_count{};
  std::uint64_t missing_witness_triangle_count{};
  std::uint64_t considered_root_count{};
  ClassifiedTriangle first_missing_witness_triangle{};
  bool first_missing_witness_triangle_present{};
};

struct GabrielNeighborRankSummary {
  std::size_t maximum_rank{};
  std::uint64_t accepted_triangle_count{};
  std::uint64_t complete_three_pair_triangle_count{};
  std::uint64_t partial_two_pair_witness_triangle_count{};
  std::uint64_t insufficient_pair_witness_triangle_count{};
  NeighborRankVariantSummary directed_root_star;
  NeighborRankVariantSummary symmetric_union_star;
  NeighborRankVariantSummary mutual_star;
};

struct NeighborRankBruteforceReplay {
  static constexpr std::size_t maximum_point_count = 4096U;

  std::size_t point_count{};
  std::size_t compared_arc_count{};
  std::size_t mismatch_count{};
  PointId first_mismatch_source{};
  PointId first_mismatch_target{};
  std::uint16_t first_mismatch_gpu_rank{};
  std::uint16_t first_mismatch_cpu_rank{};
  bool performed{};
  bool passed{};
  bool first_mismatch_present{};
  bool canonical_source_mapping_non_identity_observed{};
  bool binary64_distance_tie_observed{};
  bool censored_rank_observed{};
};

struct GabrielNeighborRankRun {
  GabrielNeighborRankSummary summary;
  NeighborRankBruteforceReplay bruteforce_replay;
  morsehgp3d::gpu::Binary64LbvhCsrNeighborRankAudit rank_audit;
  std::string canonical_to_source_mapping_sha256;
  std::uint64_t canonicalization_nanoseconds{};
  std::uint64_t lbvh_build_nanoseconds{};
  std::uint64_t rank_query_nanoseconds{};
  std::uint64_t triangle_reduction_nanoseconds{};
};

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (value < 0) {
    throw std::runtime_error("the monotonic clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::uint64_t checked_add_u64(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t parse_size(
    const char* text,
    std::string_view label,
    bool allow_zero = false) {
  std::size_t value{};
  const std::string_view input{text};
  const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() ||
      (!allow_zero && value == 0U)) {
    throw std::invalid_argument(std::string{"invalid "} + std::string{label});
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(const char* text, std::string_view label) {
  std::uint64_t value{};
  const std::string_view input{text};
  const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size()) {
    throw std::invalid_argument(std::string{"invalid "} + std::string{label});
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count = parse_size(argv[++index], "--point-count");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed = parse_u64(argv[++index], "--seed");
    } else if (argument == "--cpu-workers" && index + 1 < argc) {
      options.cpu_workers = parse_size(argv[++index], "--cpu-workers");
    } else if (argument == "--grid-resolution" && index + 1 < argc) {
      options.grid_resolution =
          parse_size(argv[++index], "--grid-resolution", true);
    } else if (argument == "--triangle-chunk-vertices" && index + 1 < argc) {
      options.triangle_chunk_vertices =
          parse_size(argv[++index], "--triangle-chunk-vertices", true);
    } else if (
        argument == "--surrogate-guardrail-max-order" &&
        index + 1 < argc) {
      options.surrogate_guardrail_max_order =
          parse_size(argv[++index], "--surrogate-guardrail-max-order");
    } else if (
        argument == "--surrogate-seed-window-radius" &&
        index + 1 < argc) {
      options.surrogate_seed_window_radius =
          parse_size(argv[++index], "--surrogate-seed-window-radius");
    } else if (
        argument == "--gabriel-neighbor-rank-max" &&
        index + 1 < argc) {
      options.gabriel_neighbor_rank_maximum =
          parse_size(argv[++index], "--gabriel-neighbor-rank-max");
    } else if (
        argument == "--neighbor-rank-seed-window-radius" &&
        index + 1 < argc) {
      options.neighbor_rank_seed_window_radius =
          parse_size(argv[++index], "--neighbor-rank-seed-window-radius");
    } else if (
        argument == "--neighbor-rank-query-batch-size" &&
        index + 1 < argc) {
      options.neighbor_rank_query_batch_size =
          parse_size(argv[++index], "--neighbor-rank-query-batch-size");
    } else if (argument == "--input-xyz" && index + 1 < argc) {
      options.input_xyz = std::string{argv[++index]};
    } else if (argument == "--fixture-e5") {
      options.fixture_e5 = true;
    } else if (argument == "--emit-records") {
      options.emit_records = true;
    } else if (argument == "--gabriel-coverage-only") {
      options.gabriel_coverage_only = true;
    } else if (argument == "--allow-conditional-guardrail") {
      options.allow_conditional_guardrail = true;
    } else {
      throw std::invalid_argument(
          "usage: gpu_geogram_low_order_diagnostic "
          "[--point-count N] [--seed N] [--cpu-workers N] "
          "[--grid-resolution R] [--triangle-chunk-vertices N] "
          "[--input-xyz PATH|--fixture-e5] [--emit-records] "
          "[--gabriel-coverage-only] "
          "[--surrogate-guardrail-max-order 2..10] "
          "[--surrogate-seed-window-radius W] "
          "[--gabriel-neighbor-rank-max 2..256] "
          "[--neighbor-rank-seed-window-radius W] "
          "[--neighbor-rank-query-batch-size N] "
          "[--allow-conditional-guardrail]");
    }
  }
  if (options.fixture_e5 && options.input_xyz.has_value()) {
    throw std::invalid_argument("--fixture-e5 and --input-xyz are exclusive");
  }
  if (options.gabriel_coverage_only &&
      options.triangle_chunk_vertices == 0U) {
    throw std::invalid_argument(
        "--gabriel-coverage-only requires --triangle-chunk-vertices N so "
        "the GPU candidate arena is explicitly bounded");
  }
  if (options.surrogate_guardrail_max_order != 0U) {
    if (!options.gabriel_coverage_only) {
      throw std::invalid_argument(
          "--surrogate-guardrail-max-order requires "
          "--gabriel-coverage-only");
    }
    if (options.surrogate_guardrail_max_order < 2U ||
        options.surrogate_guardrail_max_order > 10U) {
      throw std::invalid_argument(
          "the Gabriel guardrail requires orders 1 and 2 and supports at "
          "most the product rank window 10");
    }
    if (options.surrogate_seed_window_radius <
        options.surrogate_guardrail_max_order) {
      throw std::invalid_argument(
          "--surrogate-seed-window-radius must be at least the requested "
          "guardrail order");
    }
  }
  if (options.allow_conditional_guardrail &&
      options.surrogate_guardrail_max_order == 0U &&
      options.gabriel_neighbor_rank_maximum == 0U) {
    throw std::invalid_argument(
        "--allow-conditional-guardrail requires a Phase 15 guardrail or "
        "neighbor-rank diagnostic");
  }
  if (options.gabriel_neighbor_rank_maximum != 0U) {
    if (!options.gabriel_coverage_only) {
      throw std::invalid_argument(
          "--gabriel-neighbor-rank-max requires --gabriel-coverage-only");
    }
    if (options.gabriel_neighbor_rank_maximum < 2U ||
        options.gabriel_neighbor_rank_maximum >
            morsehgp3d::gpu::binary64_lbvh_csr_neighbor_rank_maximum) {
      throw std::invalid_argument(
          "the Gabriel neighbor-rank diagnostic requires 2 <= M <= 256");
    }
    if (options.neighbor_rank_seed_window_radius <
        options.gabriel_neighbor_rank_maximum) {
      throw std::invalid_argument(
          "--neighbor-rank-seed-window-radius must be at least M");
    }
    if (options.surrogate_guardrail_max_order != 0U) {
      throw std::invalid_argument(
          "the surrogate guardrail and compact neighbor-rank diagnostic are "
          "separate offline modes");
    }
  }
  return options;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] double unit_binary64(std::uint64_t bits) noexcept {
  constexpr double kInverse53 = 1.0 / 9007199254740992.0;
  return static_cast<double>(bits >> 11U) * kInverse53;
}

[[nodiscard]] bool point_lexicographic_less(
    const Point3& left,
    const Point3& right) noexcept {
  if (left.x != right.x) {
    return left.x < right.x;
  }
  if (left.y != right.y) {
    return left.y < right.y;
  }
  return left.z < right.z;
}

void reject_duplicate_sites(std::span<const Point3> points) {
  std::vector<std::size_t> order(points.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
    return point_lexicographic_less(points[left], points[right]);
  });
  for (std::size_t position = 1U; position < order.size(); ++position) {
    const std::size_t left = order[position - 1U];
    const std::size_t right = order[position];
    if (points[left].x == points[right].x &&
        points[left].y == points[right].y &&
        points[left].z == points[right].z) {
      throw std::invalid_argument(
          "--input-xyz contains duplicate sites at point IDs " +
          std::to_string(left) + " and " + std::to_string(right));
    }
  }
}

[[nodiscard]] std::vector<Point3> make_points(const Options& options) {
  if (options.fixture_e5) {
    return {{0.0, 0.0, 7.0},
            {0.0, 9.0, 6.0},
            {1.0, 4.0, 0.0},
            {0.0, 0.0, 1.0},
            {4.0, 1.0, 2.0}};
  }
  if (options.input_xyz.has_value()) {
    std::ifstream input{*options.input_xyz};
    if (!input) {
      throw std::runtime_error("cannot open --input-xyz");
    }
    std::vector<Point3> points;
    Point3 point;
    while (input >> point.x >> point.y >> point.z) {
      if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
          !std::isfinite(point.z)) {
        throw std::invalid_argument("--input-xyz contains a non-finite point");
      }
      points.push_back(point);
    }
    if (!input.eof()) {
      throw std::invalid_argument("--input-xyz is not a plain XYZ stream");
    }
    if (points.size() < 4U) {
      throw std::invalid_argument("--input-xyz needs at least four points");
    }
    reject_duplicate_sites(points);
    return points;
  }
  std::vector<Point3> points;
  points.reserve(options.point_count);
  const double denominator = static_cast<double>(options.point_count);
  for (std::size_t index = 0U; index < options.point_count; ++index) {
    const std::uint64_t identity = static_cast<std::uint64_t>(index);
    points.push_back(Point3{
        (static_cast<double>(index) + 0.5) / denominator,
        unit_binary64(splitmix64(identity ^ options.seed)),
        unit_binary64(splitmix64(
            identity ^ std::rotl(options.seed, 29)))});
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
  const std::size_t count =
      std::max(std::size_t{1}, std::min(item_count, worker_count));
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t worker = 0U; worker < count; ++worker) {
    workers.emplace_back([&, worker] {
      operation(
          worker,
          item_count * worker / count,
          item_count * (worker + 1U) / count);
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
}

[[nodiscard]] bool edge_less(const Edge& left, const Edge& right) noexcept {
  return left.u < right.u || (left.u == right.u && left.v < right.v);
}

struct GeogramResult {
  std::vector<Edge> edges;
  std::size_t cell_count{};
  std::size_t cell_size{};
  std::uint64_t triangulation_nanoseconds{};
  std::uint64_t extraction_nanoseconds{};
};

[[nodiscard]] GeogramResult build_geogram_edges(
    const std::vector<Point3>& points,
    std::size_t cpu_workers) {
  static_assert(sizeof(Point3) == 3U * sizeof(double));
  GEO::initialize();
  GEO::CmdLine::import_arg_group("global");
  GEO::CmdLine::import_arg_group("algo");
  GEO::CmdLine::import_arg_group("standard");
  GEO::CmdLine::set_arg("sys:multithread", "true");
  GEO::Process::set_max_threads(static_cast<GEO::index_t>(cpu_workers));
  if (points.size() >
      static_cast<std::size_t>(std::numeric_limits<GEO::index_t>::max())) {
    throw std::length_error("the cloud exceeds Geogram's PointId domain");
  }
  if (!GEO::DelaunayFactory::has_creator("PDEL")) {
    throw std::runtime_error("the pinned Geogram build has no PDEL creator");
  }

  const auto triangulation_begin = Clock::now();
  GEO::Delaunay_var delaunay = GEO::Delaunay::create(3U, "PDEL");
  if (!delaunay) {
    throw std::runtime_error("Geogram could not create PDEL");
  }
  delaunay->set_stores_neighbors(false);
  delaunay->set_stores_cicl(false);
  delaunay->set_keeps_infinite(false);
  delaunay->set_reorder(true);
  delaunay->set_vertices(
      static_cast<GEO::index_t>(points.size()),
      reinterpret_cast<const double*>(points.data()));
  const auto triangulation_end = Clock::now();

  const GEO::index_t cell_count = delaunay->nb_cells();
  const GEO::index_t cell_size = delaunay->cell_size();
  if (cell_size != 4U || cell_count == 0U) {
    throw std::runtime_error(
        "Geogram returned no finite-dimensional tetrahedralization");
  }

  const auto extraction_begin = Clock::now();
  const std::size_t worker_count = std::max(
      std::size_t{1},
      std::min(cpu_workers, static_cast<std::size_t>(cell_count)));
  std::vector<std::vector<Edge>> local_edges(worker_count);
  std::atomic<bool> invalid_cell_vertex{false};
  parallel_shards(
      static_cast<std::size_t>(cell_count),
      worker_count,
      [&](std::size_t worker, std::size_t begin, std::size_t end) {
        std::vector<Edge>& output = local_edges[worker];
        output.reserve(checked_product(end - begin, 6U, "edge reserve overflow"));
        for (std::size_t cell = begin; cell < end; ++cell) {
          if (invalid_cell_vertex.load(std::memory_order_relaxed)) {
            break;
          }
          std::array<PointId, 4U> vertices{};
          bool valid = true;
          for (std::size_t local = 0U; local < 4U; ++local) {
            const GEO::index_t vertex = delaunay->cell_vertex(
                static_cast<GEO::index_t>(cell),
                static_cast<GEO::index_t>(local));
            if (vertex == GEO::NO_INDEX ||
                static_cast<std::size_t>(vertex) >= points.size()) {
              valid = false;
              invalid_cell_vertex.store(true, std::memory_order_relaxed);
              break;
            }
            vertices[local] = static_cast<PointId>(vertex);
          }
          if (!valid) {
            break;
          }
          for (std::size_t left = 0U; left < 4U; ++left) {
            for (std::size_t right = left + 1U; right < 4U; ++right) {
              PointId u = vertices[left];
              PointId v = vertices[right];
              if (v < u) {
                std::swap(u, v);
              }
              output.push_back(Edge{u, v});
            }
          }
        }
        std::sort(output.begin(), output.end(), edge_less);
        output.erase(
            std::unique(output.begin(), output.end()), output.end());
      });
  if (invalid_cell_vertex.load(std::memory_order_relaxed)) {
    throw std::runtime_error(
        "Geogram returned an invalid vertex in a finite PDEL cell");
  }

  std::size_t total_edge_occurrences{};
  for (const std::vector<Edge>& local : local_edges) {
    if (local.size() >
        std::numeric_limits<std::size_t>::max() - total_edge_occurrences) {
      throw std::length_error("Geogram edge occurrence count overflows size_t");
    }
    total_edge_occurrences += local.size();
  }
  std::vector<Edge> edges;
  edges.reserve(total_edge_occurrences);
  for (std::vector<Edge>& local : local_edges) {
    edges.insert(edges.end(), local.begin(), local.end());
    std::vector<Edge>{}.swap(local);
  }
  std::sort(edges.begin(), edges.end(), edge_less);
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  const auto extraction_end = Clock::now();

  return GeogramResult{
      std::move(edges),
      static_cast<std::size_t>(cell_count),
      static_cast<std::size_t>(cell_size),
      nanoseconds(triangulation_end - triangulation_begin),
      nanoseconds(extraction_end - extraction_begin)};
}

struct CsrGraph {
  std::vector<std::uint64_t> offsets;
  std::vector<PointId> neighbors;
  std::size_t maximum_degree{};
  std::uint64_t raw_wedge_count{};
};

[[nodiscard]] CsrGraph build_csr(
    std::size_t point_count,
    std::span<const Edge> edges,
    std::size_t cpu_workers) {
  CsrGraph graph;
  graph.offsets.assign(point_count + 1U, 0U);
  for (const Edge& edge : edges) {
    if (edge.u >= edge.v || edge.v >= point_count) {
      throw std::logic_error("Geogram returned an invalid canonical edge");
    }
    ++graph.offsets[static_cast<std::size_t>(edge.u) + 1U];
    ++graph.offsets[static_cast<std::size_t>(edge.v) + 1U];
  }
  for (std::size_t point = 0U; point < point_count; ++point) {
    graph.offsets[point + 1U] = checked_add_u64(
        graph.offsets[point],
        graph.offsets[point + 1U],
        "CSR offset overflows uint64");
  }
  if (graph.offsets.back() > std::numeric_limits<std::size_t>::max()) {
    throw std::length_error("CSR adjacency count exceeds size_t");
  }
  graph.neighbors.resize(static_cast<std::size_t>(graph.offsets.back()));
  std::vector<std::uint64_t> cursors = graph.offsets;
  for (const Edge& edge : edges) {
    graph.neighbors[static_cast<std::size_t>(
        cursors[static_cast<std::size_t>(edge.u)]++)] = edge.v;
    graph.neighbors[static_cast<std::size_t>(
        cursors[static_cast<std::size_t>(edge.v)]++)] = edge.u;
  }
  parallel_shards(
      point_count,
      cpu_workers,
      [&](std::size_t, std::size_t begin, std::size_t end) {
        for (std::size_t point = begin; point < end; ++point) {
          const std::size_t first =
              static_cast<std::size_t>(graph.offsets[point]);
          const std::size_t last =
              static_cast<std::size_t>(graph.offsets[point + 1U]);
          std::sort(
              graph.neighbors.begin() + static_cast<std::ptrdiff_t>(first),
              graph.neighbors.begin() + static_cast<std::ptrdiff_t>(last));
        }
      });
  for (std::size_t point = 0U; point < point_count; ++point) {
    const std::uint64_t degree =
        graph.offsets[point + 1U] - graph.offsets[point];
    graph.maximum_degree = std::max(
        graph.maximum_degree, static_cast<std::size_t>(degree));
    if (degree > 1U) {
      const std::uint64_t left = degree;
      const std::uint64_t right = degree - 1U;
      const std::uint64_t wedges =
          ((left & UINT64_C(1)) == 0U ? left / 2U : left) *
          ((left & UINT64_C(1)) == 0U ? right : right / 2U);
      graph.raw_wedge_count = checked_add_u64(
          graph.raw_wedge_count, wedges, "raw wedge count overflows uint64");
    }
  }
  return graph;
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(
            std::move(operation) + " failed: " + cudaGetErrorString(code)) {}
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

struct Bbox {
  Point3 lower{};
  Point3 upper{};
};

struct CellAabb {
  Point3 lower{};
  Point3 upper{};
};

__device__ void atomic_min_finite_double(double* address, double value) noexcept {
  auto* bits = reinterpret_cast<unsigned long long*>(address);
  unsigned long long observed = atomicCAS(bits, 0ULL, 0ULL);
  while (value < __longlong_as_double(static_cast<long long>(observed))) {
    const unsigned long long desired = static_cast<unsigned long long>(
        __double_as_longlong(value));
    const unsigned long long previous = atomicCAS(bits, observed, desired);
    if (previous == observed) {
      return;
    }
    observed = previous;
  }
}

__device__ void atomic_max_finite_double(double* address, double value) noexcept {
  auto* bits = reinterpret_cast<unsigned long long*>(address);
  unsigned long long observed = atomicCAS(bits, 0ULL, 0ULL);
  while (value > __longlong_as_double(static_cast<long long>(observed))) {
    const unsigned long long desired = static_cast<unsigned long long>(
        __double_as_longlong(value));
    const unsigned long long previous = atomicCAS(bits, observed, desired);
    if (previous == observed) {
      return;
    }
    observed = previous;
  }
}

[[nodiscard]] Bbox point_bbox(std::span<const Point3> points) {
  Bbox bbox{points.front(), points.front()};
  for (const Point3& point : points.subspan(1U)) {
    bbox.lower.x = std::min(bbox.lower.x, point.x);
    bbox.lower.y = std::min(bbox.lower.y, point.y);
    bbox.lower.z = std::min(bbox.lower.z, point.z);
    bbox.upper.x = std::max(bbox.upper.x, point.x);
    bbox.upper.y = std::max(bbox.upper.y, point.y);
    bbox.upper.z = std::max(bbox.upper.z, point.z);
  }
  if (!(bbox.lower.x < bbox.upper.x) || !(bbox.lower.y < bbox.upper.y) ||
      !(bbox.lower.z < bbox.upper.z)) {
    throw std::invalid_argument(
        "the grid diagnostic requires a full-dimensional coordinate bbox");
  }
  return bbox;
}

[[nodiscard]] std::size_t automatic_grid_resolution(std::size_t point_count) {
  const double target_cells = std::max(
      1.0,
      static_cast<double>(point_count) /
          static_cast<double>(kDefaultTargetPointsPerCell));
  const double root = std::cbrt(target_cells);
  const auto resolution = static_cast<std::size_t>(std::ceil(root));
  return std::max(std::size_t{1}, resolution);
}

[[nodiscard]] __device__ std::uint64_t grid_coordinate(
    double value,
    double lower,
    double upper,
    std::uint64_t resolution) noexcept {
  double scaled = (value - lower) / (upper - lower);
  scaled = fmin(1.0, fmax(0.0, scaled));
  std::uint64_t result = static_cast<std::uint64_t>(
      floor(scaled * static_cast<double>(resolution)));
  if (result >= resolution) {
    result = resolution - 1U;
  }
  return result;
}

[[nodiscard]] __device__ std::uint64_t flatten_cell(
    std::uint64_t x,
    std::uint64_t y,
    std::uint64_t z,
    std::uint64_t resolution) noexcept {
  return x + resolution * (y + resolution * z);
}

__global__ void build_grid_keys_kernel(
    const Point3* points,
    std::size_t point_count,
    Bbox bbox,
    std::uint64_t resolution,
    std::uint64_t* keys,
    PointId* point_ids,
    std::uint64_t* counts,
    CellAabb* cell_bounds) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) * gridDim.x;
  for (std::size_t index =
           static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
       index < point_count;
       index += stride) {
    const Point3 point = points[index];
    const std::uint64_t x =
        grid_coordinate(point.x, bbox.lower.x, bbox.upper.x, resolution);
    const std::uint64_t y =
        grid_coordinate(point.y, bbox.lower.y, bbox.upper.y, resolution);
    const std::uint64_t z =
        grid_coordinate(point.z, bbox.lower.z, bbox.upper.z, resolution);
    const std::uint64_t key = flatten_cell(x, y, z, resolution);
    keys[index] = key;
    point_ids[index] = static_cast<PointId>(index);
    atomicAdd(reinterpret_cast<unsigned long long*>(counts + key), 1ULL);
    atomic_min_finite_double(&cell_bounds[key].lower.x, point.x);
    atomic_min_finite_double(&cell_bounds[key].lower.y, point.y);
    atomic_min_finite_double(&cell_bounds[key].lower.z, point.z);
    atomic_max_finite_double(&cell_bounds[key].upper.x, point.x);
    atomic_max_finite_double(&cell_bounds[key].upper.y, point.y);
    atomic_max_finite_double(&cell_bounds[key].upper.z, point.z);
  }
}

[[nodiscard]] __device__ bool csr_contains(
    const std::uint64_t* offsets,
    const PointId* neighbors,
    PointId source,
    PointId target) noexcept {
  std::uint64_t begin = offsets[source];
  std::uint64_t end = offsets[source + 1U];
  while (begin < end) {
    const std::uint64_t middle = begin + (end - begin) / 2U;
    const PointId value = neighbors[middle];
    if (value < target) {
      begin = middle + 1U;
    } else {
      end = middle;
    }
  }
  return begin < offsets[source + 1U] && neighbors[begin] == target;
}

[[nodiscard]] __device__ bool owns_triangle(
    PointId center,
    PointId first,
    PointId second,
    const std::uint64_t* offsets,
    const PointId* neighbors) noexcept {
  const bool third_edge = csr_contains(offsets, neighbors, first, second);
  return !third_edge || (center < first && center < second);
}

__global__ void count_owned_wedges_kernel(
    const std::uint64_t* offsets,
    const PointId* neighbors,
    std::size_t center_begin,
    std::size_t center_count,
    std::uint64_t* counts) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) * gridDim.x;
  for (std::size_t local =
           static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
       local < center_count;
       local += stride) {
    const PointId center = static_cast<PointId>(center_begin + local);
    std::uint64_t count = 0U;
    const std::uint64_t begin = offsets[center];
    const std::uint64_t end = offsets[center + 1U];
    for (std::uint64_t left = begin; left < end; ++left) {
      const PointId first = neighbors[left];
      for (std::uint64_t right = left + 1U; right < end; ++right) {
        const PointId second = neighbors[right];
        count += owns_triangle(center, first, second, offsets, neighbors)
                     ? UINT64_C(1)
                     : UINT64_C(0);
      }
    }
    counts[local] = count;
  }
}

__global__ void emit_owned_wedges_kernel(
    const std::uint64_t* offsets,
    const PointId* neighbors,
    std::size_t center_begin,
    std::size_t center_count,
    const std::uint64_t* output_offsets,
    Triangle* triangles) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) * gridDim.x;
  for (std::size_t local =
           static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
       local < center_count;
       local += stride) {
    const PointId center = static_cast<PointId>(center_begin + local);
    std::uint64_t output = output_offsets[local];
    const std::uint64_t begin = offsets[center];
    const std::uint64_t end = offsets[center + 1U];
    for (std::uint64_t left = begin; left < end; ++left) {
      const PointId first = neighbors[left];
      for (std::uint64_t right = left + 1U; right < end; ++right) {
        const PointId second = neighbors[right];
        if (!owns_triangle(center, first, second, offsets, neighbors)) {
          continue;
        }
        PointId a = center;
        PointId b = first;
        PointId c = second;
        if (b < a) {
          const PointId temporary = a;
          a = b;
          b = temporary;
        }
        if (c < b) {
          const PointId temporary = b;
          b = c;
          c = temporary;
        }
        if (b < a) {
          const PointId temporary = a;
          a = b;
          b = temporary;
        }
        triangles[output++] = Triangle{a, b, c};
      }
    }
  }
}

struct Miniball3 {
  Point3 center{};
  double squared_radius{};
  std::uint32_t support_cardinality{};
  bool valid{};
  bool ambiguous{};
};

[[nodiscard]] __device__ double squared_distance(
    Point3 left,
    Point3 right) noexcept {
  const double x = left.x - right.x;
  const double y = left.y - right.y;
  const double z = left.z - right.z;
  return x * x + y * y + z * z;
}

[[nodiscard]] __device__ Point3 midpoint(Point3 left, Point3 right) noexcept {
  return Point3{
      left.x + (right.x - left.x) * 0.5,
      left.y + (right.y - left.y) * 0.5,
      left.z + (right.z - left.z) * 0.5};
}

[[nodiscard]] __device__ Miniball3 triangle_miniball(
    Point3 a,
    Point3 b,
    Point3 c) noexcept {
  const double ux = b.x - a.x;
  const double uy = b.y - a.y;
  const double uz = b.z - a.z;
  const double vx = c.x - a.x;
  const double vy = c.y - a.y;
  const double vz = c.z - a.z;
  const double ab2 = ux * ux + uy * uy + uz * uz;
  const double ac2 = vx * vx + vy * vy + vz * vz;
  const double dot = ux * vx + uy * vy + uz * vz;
  const double bc2 = ab2 + ac2 - 2.0 * dot;
  if (!(ab2 > 0.0) || !(ac2 > 0.0) || !(bc2 > 0.0) ||
      !isfinite(ab2) || !isfinite(ac2) || !isfinite(bc2)) {
    return {};
  }
  const double scale = ab2 + ac2 + bc2;
  if (!isfinite(scale)) {
    return {};
  }
  const double comparison_tolerance =
      kPredicateToleranceMultiplier * DBL_EPSILON * fmax(scale, DBL_MIN);
  const bool near_ab_angle = fabs(bc2 - (ab2 + ac2)) <= comparison_tolerance;
  const bool near_ac_angle = fabs(ac2 - (ab2 + bc2)) <= comparison_tolerance;
  const bool near_bc_angle = fabs(ab2 - (ac2 + bc2)) <= comparison_tolerance;
  const bool near_right = near_ab_angle || near_ac_angle || near_bc_angle;
  if (bc2 >= ab2 + ac2) {
    return Miniball3{midpoint(b, c), 0.25 * bc2, 2U, true, near_right};
  }
  if (ac2 >= ab2 + bc2) {
    return Miniball3{midpoint(a, c), 0.25 * ac2, 2U, true, near_right};
  }
  if (ab2 >= ac2 + bc2) {
    return Miniball3{midpoint(a, b), 0.25 * ab2, 2U, true, near_right};
  }
  const double determinant = ab2 * ac2 - dot * dot;
  const double determinant_scale = fmax(ab2 * ac2, dot * dot);
  if (!isfinite(determinant) || !isfinite(determinant_scale)) {
    return {};
  }
  const double determinant_tolerance =
      kPredicateToleranceMultiplier * DBL_EPSILON *
      fmax(determinant_scale, DBL_MIN);
  if (!(determinant > determinant_tolerance)) {
    Point3 left = a;
    Point3 right = b;
    double diameter2 = ab2;
    if (ac2 > diameter2) {
      right = c;
      diameter2 = ac2;
    }
    if (bc2 > diameter2) {
      left = b;
      right = c;
      diameter2 = bc2;
    }
    return Miniball3{
        midpoint(left, right), 0.25 * diameter2, 2U, true, true};
  }
  const double denominator = 2.0 * determinant;
  const double alpha = ac2 * (ab2 - dot) / denominator;
  const double beta = ab2 * (ac2 - dot) / denominator;
  Point3 center{
      a.x + alpha * ux + beta * vx,
      a.y + alpha * uy + beta * vy,
      a.z + alpha * uz + beta * vz};
  const double radius = squared_distance(center, a);
  return Miniball3{
      center,
      radius,
      3U,
      isfinite(center.x) && isfinite(center.y) && isfinite(center.z) &&
          isfinite(radius) && radius > 0.0,
      near_right};
}

[[nodiscard]] __device__ double numerical_tolerance(
    Point3 center,
    double left,
    double right) noexcept {
  const double coordinate_scale = fmax(
      fmax(center.x * center.x, center.y * center.y),
      center.z * center.z);
  return kPredicateToleranceMultiplier * DBL_EPSILON *
         (fabs(left) + fabs(right) + coordinate_scale + 1.0);
}

[[nodiscard]] __device__ double cell_aabb_lower_bound(
    Point3 center,
    CellAabb bounds) noexcept {
  const double dx = center.x < bounds.lower.x
      ? __dsub_rd(bounds.lower.x, center.x)
      : (center.x > bounds.upper.x
             ? __dsub_rd(center.x, bounds.upper.x)
             : 0.0);
  const double dy = center.y < bounds.lower.y
      ? __dsub_rd(bounds.lower.y, center.y)
      : (center.y > bounds.upper.y
             ? __dsub_rd(center.y, bounds.upper.y)
             : 0.0);
  const double dz = center.z < bounds.lower.z
      ? __dsub_rd(bounds.lower.z, center.z)
      : (center.z > bounds.upper.z
             ? __dsub_rd(center.z, bounds.upper.z)
             : 0.0);
  const double xy = __dadd_rd(__dmul_rd(dx, dx), __dmul_rd(dy, dy));
  return __dadd_rd(xy, __dmul_rd(dz, dz));
}

__global__ void classify_triangles_kernel(
    const Point3* points,
    std::size_t point_count,
    const Triangle* triangles,
    std::size_t triangle_count,
    const PointId* sorted_point_ids,
    const std::uint64_t* cell_offsets,
    const CellAabb* cell_bounds,
    std::uint64_t resolution,
    Bbox bbox,
    ClassifiedTriangle* output) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) * gridDim.x;
  for (std::size_t index =
           static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
       index < triangle_count;
       index += stride) {
    const Triangle triangle = triangles[index];
    ClassifiedTriangle result;
    result.triangle = triangle;
    const Miniball3 ball = triangle_miniball(
        points[triangle.a], points[triangle.b], points[triangle.c]);
    result.squared_level = ball.squared_radius;
    result.support_cardinality = ball.support_cardinality;
    if (!ball.valid) {
      result.status = TriangleStatus::degenerate_or_invalid;
      output[index] = result;
      continue;
    }
    if (ball.ambiguous) {
      result.status = TriangleStatus::ambiguous;
      output[index] = result;
      continue;
    }
    result.status = TriangleStatus::blocked;

    const double radius = __dsqrt_ru(ball.squared_radius);
    const std::uint64_t x0 = grid_coordinate(
        __dsub_rd(ball.center.x, radius),
        bbox.lower.x,
        bbox.upper.x,
        resolution);
    const std::uint64_t x1 = grid_coordinate(
        __dadd_ru(ball.center.x, radius),
        bbox.lower.x,
        bbox.upper.x,
        resolution);
    const std::uint64_t y0 = grid_coordinate(
        __dsub_rd(ball.center.y, radius),
        bbox.lower.y,
        bbox.upper.y,
        resolution);
    const std::uint64_t y1 = grid_coordinate(
        __dadd_ru(ball.center.y, radius),
        bbox.lower.y,
        bbox.upper.y,
        resolution);
    const std::uint64_t z0 = grid_coordinate(
        __dsub_rd(ball.center.z, radius),
        bbox.lower.z,
        bbox.upper.z,
        resolution);
    const std::uint64_t z1 = grid_coordinate(
        __dadd_ru(ball.center.z, radius),
        bbox.lower.z,
        bbox.upper.z,
        resolution);

    bool ambiguous = false;
    bool blocked = false;
    for (std::uint64_t z = z0; z <= z1 && !blocked; ++z) {
      for (std::uint64_t y = y0; y <= y1 && !blocked; ++y) {
        for (std::uint64_t x = x0; x <= x1 && !blocked; ++x) {
          const std::uint64_t cell = flatten_cell(x, y, z, resolution);
          if (cell_offsets[cell] == cell_offsets[cell + 1U]) {
            continue;
          }
          ++result.visited_cell_count;
          const double lower =
              cell_aabb_lower_bound(ball.center, cell_bounds[cell]);
          const double lower_tolerance = numerical_tolerance(
              ball.center, lower, ball.squared_radius);
          if (lower > ball.squared_radius + lower_tolerance) {
            continue;
          }
          for (std::uint64_t position = cell_offsets[cell];
               position < cell_offsets[cell + 1U];
               ++position) {
            const PointId point_id = sorted_point_ids[position];
            if (point_id == triangle.a || point_id == triangle.b ||
                point_id == triangle.c) {
              continue;
            }
            if (point_id >= point_count) {
              result.status = TriangleStatus::degenerate_or_invalid;
              output[index] = result;
              blocked = true;
              break;
            }
            ++result.tested_point_count;
            const double distance =
                squared_distance(ball.center, points[point_id]);
            if (!isfinite(distance)) {
              result.status = TriangleStatus::degenerate_or_invalid;
              output[index] = result;
              blocked = true;
              break;
            }
            const double tolerance = numerical_tolerance(
                ball.center, distance, ball.squared_radius);
            if (distance < ball.squared_radius - tolerance) {
              blocked = true;
              result.status = TriangleStatus::blocked;
              break;
            }
            if (fabs(distance - ball.squared_radius) <= tolerance) {
              ambiguous = true;
            }
          }
        }
      }
    }
    if (result.status == TriangleStatus::degenerate_or_invalid) {
      continue;
    }
    if (!blocked) {
      result.status = ambiguous ? TriangleStatus::ambiguous
                                : TriangleStatus::gabriel_binary64;
    }
    output[index] = result;
  }
}

struct StatusIs {
  TriangleStatus value{};
  [[nodiscard]] __host__ __device__ bool operator()(
      const ClassifiedTriangle& triangle) const noexcept {
    return triangle.status == value;
  }
};

struct DiscardNonRetainedRecord {
  [[nodiscard]] __host__ __device__ bool operator()(
      const ClassifiedTriangle& triangle) const noexcept {
    return triangle.status != TriangleStatus::gabriel_binary64 &&
           triangle.status != TriangleStatus::ambiguous;
  }
};

struct IsValidRestrictedGammaRecord {
  [[nodiscard]] __host__ __device__ bool operator()(
      const ClassifiedTriangle& triangle) const noexcept {
    return triangle.status != TriangleStatus::degenerate_or_invalid;
  }
};

struct ClassifiedTriangleLess {
  [[nodiscard]] __host__ __device__ bool operator()(
      const ClassifiedTriangle& left,
      const ClassifiedTriangle& right) const noexcept {
    if (left.squared_level != right.squared_level) {
      return left.squared_level < right.squared_level;
    }
    if (left.triangle.a != right.triangle.a) {
      return left.triangle.a < right.triangle.a;
    }
    if (left.triangle.b != right.triangle.b) {
      return left.triangle.b < right.triangle.b;
    }
    return left.triangle.c < right.triangle.c;
  }
};

struct VisitProjection {
  [[nodiscard]] __host__ __device__ std::uint64_t operator()(
      const ClassifiedTriangle& triangle) const noexcept {
    return triangle.visited_cell_count;
  }
};

struct TestProjection {
  [[nodiscard]] __host__ __device__ std::uint64_t operator()(
      const ClassifiedTriangle& triangle) const noexcept {
    return triangle.tested_point_count;
  }
};

struct DevicePipelineResult {
  std::vector<ClassifiedTriangle> retained_records;
  std::vector<ClassifiedTriangle> restricted_gamma_records;
  std::vector<ClassifiedTriangle> invalid_records;
  TriangleAudit audit;
  std::uint64_t grid_nanoseconds{};
  std::uint64_t enumeration_nanoseconds{};
  std::uint64_t classification_nanoseconds{};
  std::uint64_t audit_nanoseconds{};
  std::uint64_t compaction_sort_nanoseconds{};
  std::uint64_t restricted_gamma_compaction_sort_nanoseconds{};
  std::uint64_t device_to_host_nanoseconds{};
  std::uint64_t restricted_gamma_device_to_host_nanoseconds{};
  std::uint64_t host_sort_nanoseconds{};
  std::uint64_t restricted_gamma_host_sort_nanoseconds{};
  std::size_t grid_resolution{};
  std::size_t grid_cell_count{};
  std::size_t maximum_cell_occupancy{};
  std::size_t cuda_multiprocessor_count{};
  std::size_t persistent_device_bytes{};
  std::size_t maximum_accounted_live_device_bytes{};
  std::string cuda_device_name;
};

[[nodiscard]] std::size_t device_vector_bytes(
    std::size_t count,
    std::size_t element_size) {
  return checked_product(count, element_size, "device byte count overflows");
}

[[nodiscard]] DevicePipelineResult run_triangle_pipeline(
    const std::vector<Point3>& points,
    const CsrGraph& graph,
    const Options& options) {
  DevicePipelineResult result;
  int device{};
  check_cuda(cudaGetDevice(&device), "cudaGetDevice");
  cudaDeviceProp properties{};
  check_cuda(cudaGetDeviceProperties(&properties, device), "cudaGetDeviceProperties");
  if (properties.multiProcessorCount <= 0) {
    throw std::runtime_error("CUDA reported no multiprocessor");
  }
  result.cuda_multiprocessor_count =
      static_cast<std::size_t>(properties.multiProcessorCount);
  result.cuda_device_name = properties.name;

  const Bbox bbox = point_bbox(points);
  const std::size_t resolution = options.grid_resolution == 0U
      ? automatic_grid_resolution(points.size())
      : options.grid_resolution;
  const std::size_t resolution_squared = checked_product(
      resolution, resolution, "grid resolution squared overflows");
  const std::size_t cell_count = checked_product(
      resolution_squared, resolution, "grid cell count overflows");
  result.grid_resolution = resolution;
  result.grid_cell_count = cell_count;

  const auto grid_begin = Clock::now();
  thrust::device_vector<Point3> device_points(points.begin(), points.end());
  thrust::device_vector<std::uint64_t> grid_keys(points.size());
  thrust::device_vector<PointId> sorted_point_ids(points.size());
  thrust::device_vector<std::uint64_t> cell_counts(cell_count + 1U, 0U);
  thrust::device_vector<std::uint64_t> cell_offsets(cell_count + 1U, 0U);
  const double infinity = std::numeric_limits<double>::infinity();
  const CellAabb empty_cell{
      Point3{infinity, infinity, infinity},
      Point3{-infinity, -infinity, -infinity}};
  thrust::device_vector<CellAabb> cell_bounds(cell_count, empty_cell);
  result.maximum_accounted_live_device_bytes =
      device_vector_bytes(points.size(), sizeof(Point3)) +
      device_vector_bytes(points.size(), sizeof(std::uint64_t)) +
      device_vector_bytes(points.size(), sizeof(PointId)) +
      device_vector_bytes(cell_count + 1U, 2U * sizeof(std::uint64_t)) +
      device_vector_bytes(cell_count, sizeof(CellAabb));
  const std::size_t point_blocks = std::max(
      std::size_t{1},
      std::min(
          (points.size() + kThreadsPerBlock - 1U) / kThreadsPerBlock,
          result.cuda_multiprocessor_count * 32U));
  build_grid_keys_kernel<<<
      static_cast<unsigned int>(point_blocks), kThreadsPerBlock>>>(
      thrust::raw_pointer_cast(device_points.data()),
      points.size(),
      bbox,
      static_cast<std::uint64_t>(resolution),
      thrust::raw_pointer_cast(grid_keys.data()),
      thrust::raw_pointer_cast(sorted_point_ids.data()),
      thrust::raw_pointer_cast(cell_counts.data()),
      thrust::raw_pointer_cast(cell_bounds.data()));
  check_cuda(cudaGetLastError(), "build_grid_keys_kernel launch");
  thrust::sort_by_key(
      thrust::device,
      grid_keys.begin(),
      grid_keys.end(),
      sorted_point_ids.begin());
  thrust::exclusive_scan(
      thrust::device,
      cell_counts.begin(),
      cell_counts.end(),
      cell_offsets.begin());
  const auto max_occupancy = thrust::max_element(
      thrust::device, cell_counts.begin(), cell_counts.end() - 1);
  result.maximum_cell_occupancy =
      static_cast<std::size_t>(*max_occupancy);
  check_cuda(cudaDeviceSynchronize(), "grid construction synchronization");
  result.grid_nanoseconds = nanoseconds(Clock::now() - grid_begin);
  thrust::device_vector<std::uint64_t>{}.swap(grid_keys);
  thrust::device_vector<std::uint64_t>{}.swap(cell_counts);

  thrust::device_vector<std::uint64_t> csr_offsets(
      graph.offsets.begin(), graph.offsets.end());
  thrust::device_vector<PointId> csr_neighbors(
      graph.neighbors.begin(), graph.neighbors.end());
  const std::size_t chunk_vertices = options.triangle_chunk_vertices == 0U
      ? points.size()
      : std::min(options.triangle_chunk_vertices, points.size());
  result.persistent_device_bytes =
      device_vector_bytes(points.size(), sizeof(Point3)) +
      device_vector_bytes(points.size(), sizeof(PointId)) +
      device_vector_bytes(cell_count + 1U, sizeof(std::uint64_t)) +
      device_vector_bytes(cell_count, sizeof(CellAabb)) +
      device_vector_bytes(graph.offsets.size(), sizeof(std::uint64_t)) +
      device_vector_bytes(graph.neighbors.size(), sizeof(PointId));

  for (std::size_t center_begin = 0U;
       center_begin < points.size();
       center_begin += chunk_vertices) {
    const std::size_t center_count =
        std::min(chunk_vertices, points.size() - center_begin);
    ++result.audit.chunk_count;
    const std::size_t blocks = std::max(
        std::size_t{1},
        std::min(
            (center_count + kThreadsPerBlock - 1U) / kThreadsPerBlock,
            result.cuda_multiprocessor_count * 32U));

    const auto enumeration_begin = Clock::now();
    thrust::device_vector<std::uint64_t> candidate_counts(center_count + 1U, 0U);
    thrust::device_vector<std::uint64_t> candidate_offsets(center_count + 1U, 0U);
    count_owned_wedges_kernel<<<
        static_cast<unsigned int>(blocks), kThreadsPerBlock>>>(
        thrust::raw_pointer_cast(csr_offsets.data()),
        thrust::raw_pointer_cast(csr_neighbors.data()),
        center_begin,
        center_count,
        thrust::raw_pointer_cast(candidate_counts.data()));
    check_cuda(cudaGetLastError(), "count_owned_wedges_kernel launch");
    thrust::exclusive_scan(
        thrust::device,
        candidate_counts.begin(),
        candidate_counts.end(),
        candidate_offsets.begin());
    const std::uint64_t candidate_count_u64 = candidate_offsets.back();
    if (candidate_count_u64 > std::numeric_limits<std::size_t>::max()) {
      throw std::length_error("triangle candidate count exceeds size_t");
    }
    const std::size_t candidate_count =
        static_cast<std::size_t>(candidate_count_u64);
    result.audit.canonical_candidate_count = checked_add_u64(
        result.audit.canonical_candidate_count,
        candidate_count_u64,
        "candidate audit count overflows uint64");
    result.audit.maximum_chunk_candidate_count = std::max(
        result.audit.maximum_chunk_candidate_count, candidate_count);
    thrust::device_vector<Triangle> triangles(candidate_count);
    if (candidate_count != 0U) {
      emit_owned_wedges_kernel<<<
          static_cast<unsigned int>(blocks), kThreadsPerBlock>>>(
          thrust::raw_pointer_cast(csr_offsets.data()),
          thrust::raw_pointer_cast(csr_neighbors.data()),
          center_begin,
          center_count,
          thrust::raw_pointer_cast(candidate_offsets.data()),
          thrust::raw_pointer_cast(triangles.data()));
      check_cuda(cudaGetLastError(), "emit_owned_wedges_kernel launch");
    }
    check_cuda(cudaDeviceSynchronize(), "triangle enumeration synchronization");
    result.enumeration_nanoseconds = checked_add_u64(
        result.enumeration_nanoseconds,
        nanoseconds(Clock::now() - enumeration_begin),
        "triangle enumeration time overflows uint64");

    const auto classification_begin = Clock::now();
    thrust::device_vector<ClassifiedTriangle> classified(candidate_count);
    if (candidate_count != 0U) {
      const std::size_t triangle_blocks = std::max(
          std::size_t{1},
          std::min(
              (candidate_count + kThreadsPerBlock - 1U) / kThreadsPerBlock,
              result.cuda_multiprocessor_count * 32U));
      classify_triangles_kernel<<<
          static_cast<unsigned int>(triangle_blocks), kThreadsPerBlock>>>(
          thrust::raw_pointer_cast(device_points.data()),
          points.size(),
          thrust::raw_pointer_cast(triangles.data()),
          candidate_count,
          thrust::raw_pointer_cast(sorted_point_ids.data()),
          thrust::raw_pointer_cast(cell_offsets.data()),
          thrust::raw_pointer_cast(cell_bounds.data()),
          static_cast<std::uint64_t>(resolution),
          bbox,
          thrust::raw_pointer_cast(classified.data()));
      check_cuda(cudaGetLastError(), "classify_triangles_kernel launch");
    }
    check_cuda(cudaDeviceSynchronize(), "triangle classification synchronization");
    result.classification_nanoseconds = checked_add_u64(
        result.classification_nanoseconds,
        nanoseconds(Clock::now() - classification_begin),
        "triangle classification time overflows uint64");

    const auto audit_begin = Clock::now();
    const std::uint64_t blocked = static_cast<std::uint64_t>(thrust::count_if(
        thrust::device,
        classified.begin(),
        classified.end(),
        StatusIs{TriangleStatus::blocked}));
    const std::uint64_t accepted = static_cast<std::uint64_t>(thrust::count_if(
        thrust::device,
        classified.begin(),
        classified.end(),
        StatusIs{TriangleStatus::gabriel_binary64}));
    const std::uint64_t ambiguous = static_cast<std::uint64_t>(thrust::count_if(
        thrust::device,
        classified.begin(),
        classified.end(),
        StatusIs{TriangleStatus::ambiguous}));
    const std::uint64_t invalid = static_cast<std::uint64_t>(thrust::count_if(
        thrust::device,
        classified.begin(),
        classified.end(),
        StatusIs{TriangleStatus::degenerate_or_invalid}));
    result.audit.blocked_count = checked_add_u64(
        result.audit.blocked_count, blocked, "blocked count overflows");
    result.audit.accepted_count = checked_add_u64(
        result.audit.accepted_count, accepted, "accepted count overflows");
    result.audit.ambiguous_count = checked_add_u64(
        result.audit.ambiguous_count, ambiguous, "ambiguous count overflows");
    result.audit.invalid_count = checked_add_u64(
        result.audit.invalid_count, invalid, "invalid count overflows");
    result.audit.visited_cell_count = checked_add_u64(
        result.audit.visited_cell_count,
        thrust::transform_reduce(
            thrust::device,
            classified.begin(),
            classified.end(),
            VisitProjection{},
            UINT64_C(0),
            thrust::plus<std::uint64_t>{}),
        "visited cell count overflows");
    result.audit.tested_point_count = checked_add_u64(
        result.audit.tested_point_count,
        thrust::transform_reduce(
            thrust::device,
            classified.begin(),
            classified.end(),
            TestProjection{},
            UINT64_C(0),
            thrust::plus<std::uint64_t>{}),
        "tested point count overflows");
    const std::uint64_t classified_count = checked_add_u64(
        checked_add_u64(blocked, accepted, "classified count overflows"),
        checked_add_u64(ambiguous, invalid, "classified count overflows"),
        "classified count overflows");
    if (classified_count != candidate_count_u64) {
      throw std::logic_error("triangle status partition does not close");
    }
    result.audit_nanoseconds = checked_add_u64(
        result.audit_nanoseconds,
        nanoseconds(Clock::now() - audit_begin),
        "triangle audit time overflows uint64");

    const std::size_t restricted_gamma_count = options.gabriel_coverage_only
        ? 0U
        : candidate_count - static_cast<std::size_t>(invalid);
    const auto restricted_gamma_compaction_begin = Clock::now();
    thrust::device_vector<ClassifiedTriangle> restricted_gamma_device_records(
        restricted_gamma_count);
    if (!options.gabriel_coverage_only && restricted_gamma_count != 0U) {
      const auto restricted_gamma_end = thrust::copy_if(
          thrust::device,
          classified.begin(),
          classified.end(),
          restricted_gamma_device_records.begin(),
          IsValidRestrictedGammaRecord{});
      if (restricted_gamma_end != restricted_gamma_device_records.end()) {
        throw std::logic_error(
            "restricted Gamma triangle extraction count mismatch");
      }
      thrust::sort(
          thrust::device,
          restricted_gamma_device_records.begin(),
          restricted_gamma_device_records.end(),
          ClassifiedTriangleLess{});
    }
    check_cuda(
        cudaDeviceSynchronize(),
        "restricted Gamma triangle compaction synchronization");
    result.restricted_gamma_compaction_sort_nanoseconds = checked_add_u64(
        result.restricted_gamma_compaction_sort_nanoseconds,
        nanoseconds(Clock::now() - restricted_gamma_compaction_begin),
        "restricted Gamma compaction time overflows uint64");

    const auto gabriel_compaction_begin = Clock::now();
    const std::size_t retained_invalid_count = options.gabriel_coverage_only
        ? 0U
        : static_cast<std::size_t>(invalid);
    thrust::device_vector<ClassifiedTriangle> invalid_device_records(
        retained_invalid_count);
    if (retained_invalid_count != 0U) {
      const auto invalid_end = thrust::copy_if(
          thrust::device,
          classified.begin(),
          classified.end(),
          invalid_device_records.begin(),
          StatusIs{TriangleStatus::degenerate_or_invalid});
      if (invalid_end != invalid_device_records.end()) {
        throw std::logic_error("invalid triangle extraction count mismatch");
      }
    }
    const auto retained_end = thrust::remove_if(
        thrust::device,
        classified.begin(),
        classified.end(),
        DiscardNonRetainedRecord{});
    classified.erase(retained_end, classified.end());
    thrust::sort(
        thrust::device,
        classified.begin(),
        classified.end(),
        ClassifiedTriangleLess{});
    check_cuda(cudaDeviceSynchronize(), "triangle compaction synchronization");
    result.compaction_sort_nanoseconds = checked_add_u64(
        result.compaction_sort_nanoseconds,
        nanoseconds(Clock::now() - gabriel_compaction_begin),
        "triangle compaction time overflows uint64");

    const std::size_t chunk_bytes =
        device_vector_bytes(candidate_count, sizeof(Triangle)) +
        device_vector_bytes(candidate_count, sizeof(ClassifiedTriangle)) +
        device_vector_bytes(
            restricted_gamma_count, sizeof(ClassifiedTriangle)) +
        device_vector_bytes(
            retained_invalid_count, sizeof(ClassifiedTriangle)) +
        device_vector_bytes(center_count + 1U, 2U * sizeof(std::uint64_t));
    result.audit.maximum_chunk_device_bytes = std::max(
        result.audit.maximum_chunk_device_bytes, chunk_bytes);
    result.maximum_accounted_live_device_bytes = std::max(
        result.maximum_accounted_live_device_bytes,
        result.persistent_device_bytes + chunk_bytes);

    const auto restricted_gamma_copy_begin = Clock::now();
    std::vector<ClassifiedTriangle> host_restricted_gamma_chunk;
    if (!options.gabriel_coverage_only) {
      host_restricted_gamma_chunk.resize(
          restricted_gamma_device_records.size());
      thrust::copy(
          restricted_gamma_device_records.begin(),
          restricted_gamma_device_records.end(),
          host_restricted_gamma_chunk.begin());
      check_cuda(
          cudaDeviceSynchronize(),
          "restricted Gamma triangle output synchronization");
    }
    result.restricted_gamma_device_to_host_nanoseconds = checked_add_u64(
        result.restricted_gamma_device_to_host_nanoseconds,
        nanoseconds(Clock::now() - restricted_gamma_copy_begin),
        "restricted Gamma triangle D2H time overflows uint64");
    if (host_restricted_gamma_chunk.size() >
        std::numeric_limits<std::size_t>::max() -
            result.restricted_gamma_records.size()) {
      throw std::length_error("restricted Gamma triangle arena overflows size_t");
    }
    result.restricted_gamma_records.insert(
        result.restricted_gamma_records.end(),
        host_restricted_gamma_chunk.begin(),
        host_restricted_gamma_chunk.end());

    const auto copy_begin = Clock::now();
    std::vector<ClassifiedTriangle> host_chunk(classified.size());
    thrust::copy(classified.begin(), classified.end(), host_chunk.begin());
    std::vector<ClassifiedTriangle> host_invalid_chunk;
    if (!options.gabriel_coverage_only) {
      host_invalid_chunk.resize(invalid_device_records.size());
      thrust::copy(
          invalid_device_records.begin(),
          invalid_device_records.end(),
          host_invalid_chunk.begin());
    }
    check_cuda(cudaDeviceSynchronize(), "triangle output synchronization");
    result.device_to_host_nanoseconds = checked_add_u64(
        result.device_to_host_nanoseconds,
        nanoseconds(Clock::now() - copy_begin),
        "triangle D2H time overflows uint64");
    if (host_chunk.size() > std::numeric_limits<std::size_t>::max() -
                                result.retained_records.size()) {
      throw std::length_error("retained triangle arena overflows size_t");
    }
    result.retained_records.insert(
        result.retained_records.end(), host_chunk.begin(), host_chunk.end());
    for (ClassifiedTriangle& invalid_record : host_invalid_chunk) {
      invalid_record.squared_level = 0.0;
    }
    if (host_invalid_chunk.size() >
        std::numeric_limits<std::size_t>::max() -
            result.invalid_records.size()) {
      throw std::length_error("invalid triangle arena overflows size_t");
    }
    result.invalid_records.insert(
        result.invalid_records.end(),
        host_invalid_chunk.begin(),
        host_invalid_chunk.end());
  }

  result.audit.raw_wedge_count = graph.raw_wedge_count;
  const auto host_sort_begin = Clock::now();
  std::sort(
      result.retained_records.begin(),
      result.retained_records.end(),
      ClassifiedTriangleLess{});
  result.host_sort_nanoseconds = nanoseconds(Clock::now() - host_sort_begin);
  const auto restricted_gamma_host_sort_begin = Clock::now();
  std::sort(
      result.restricted_gamma_records.begin(),
      result.restricted_gamma_records.end(),
      ClassifiedTriangleLess{});
  result.restricted_gamma_host_sort_nanoseconds =
      nanoseconds(Clock::now() - restricted_gamma_host_sort_begin);
  return result;
}

[[nodiscard]] double point_squared_distance(
    const Point3& left,
    const Point3& right) noexcept {
  const double x = left.x - right.x;
  const double y = left.y - right.y;
  const double z = left.z - right.z;
  return x * x + y * y + z * z;
}

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t size)
      : parent_(size), size_(size, 1U), component_count_(size) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) noexcept {
    std::size_t root = value;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[value] != value) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) noexcept {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    if (size_[left] < size_[right] ||
        (size_[left] == size_[right] && right < left)) {
      std::swap(left, right);
    }
    parent_[right] = left;
    size_[left] += size_[right];
    --component_count_;
    return true;
  }

  [[nodiscard]] std::size_t component_count() const noexcept {
    return component_count_;
  }

 private:
  std::vector<std::size_t> parent_;
  std::vector<std::size_t> size_;
  std::size_t component_count_{};
};

void digest_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
  for (unsigned int byte = 0U; byte < 8U; ++byte) {
    digest ^= (word >> (byte * 8U)) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
  std::ostringstream output;
  output << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return output.str();
}

void sha256_word(
    morsehgp3d::contract::CanonicalSha256Builder& builder,
    std::uint64_t word) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        word >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(std::span<const std::uint8_t>{bytes});
}

void sha256_triangle(
    morsehgp3d::contract::CanonicalSha256Builder& builder,
    const ClassifiedTriangle& record) {
  sha256_word(builder, std::bit_cast<std::uint64_t>(record.squared_level));
  sha256_word(builder, record.triangle.a);
  sha256_word(builder, record.triangle.b);
  sha256_word(builder, record.triangle.c);
  sha256_word(builder, record.support_cardinality);
}

[[nodiscard]] SurrogateTreeEdge make_surrogate_tree_edge(
    PointId first,
    PointId second,
    double squared_weight) {
  if (!std::isfinite(squared_weight) || squared_weight < 0.0 ||
      first == second) {
    throw std::runtime_error(
        "the surrogate graph contains an invalid weighted edge");
  }
  if (second < first) {
    std::swap(first, second);
  }
  return SurrogateTreeEdge{squared_weight, first, second};
}

[[nodiscard]] CorrectedSurrogateTreeEdge make_corrected_surrogate_tree_edge(
    PointId first,
    PointId second,
    double encoded_binary64,
    CorrectedLevelEncoding level_encoding) {
  if (!std::isfinite(encoded_binary64) || encoded_binary64 < 0.0 ||
      first == second) {
    throw std::runtime_error(
        "the corrected surrogate contains an invalid merge edge");
  }
  if (second < first) {
    std::swap(first, second);
  }
  return CorrectedSurrogateTreeEdge{
      encoded_binary64, first, second, level_encoding};
}

[[nodiscard]] bool surrogate_endpoint_less(
    const SurrogateTreeEdge& left,
    const SurrogateTreeEdge& right) noexcept {
  if (left.u != right.u) {
    return left.u < right.u;
  }
  if (left.v != right.v) {
    return left.v < right.v;
  }
  return left.squared_weight < right.squared_weight;
}

[[nodiscard]] bool surrogate_weight_less(
    const SurrogateTreeEdge& left,
    const SurrogateTreeEdge& right) noexcept {
  if (left.squared_weight != right.squared_weight) {
    return left.squared_weight < right.squared_weight;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] bool triangle_vertices_connected(
    DisjointSet& components,
    const Triangle& triangle) {
  const std::size_t first =
      components.find(static_cast<std::size_t>(triangle.a));
  return components.find(static_cast<std::size_t>(triangle.b)) == first &&
         components.find(static_cast<std::size_t>(triangle.c)) == first;
}

void remember_surrogate_failure(
    SurrogateFailureWitness& destination,
    const ClassifiedTriangle& source,
    std::string_view classification) {
  if (!destination.present) {
    destination.source = source;
    destination.classification = classification;
    destination.present = true;
  }
}

[[nodiscard]] std::string surrogate_tree_sha256(
    std::size_t point_count,
    std::size_t order,
    std::span<const SurrogateTreeEdge> tree) {
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(
      "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
      "surrogate-tree-v1/sha256/");
  sha256_word(builder, static_cast<std::uint64_t>(point_count));
  sha256_word(builder, static_cast<std::uint64_t>(order));
  sha256_word(builder, static_cast<std::uint64_t>(tree.size()));
  for (const SurrogateTreeEdge& edge : tree) {
    sha256_word(builder, std::bit_cast<std::uint64_t>(edge.squared_weight));
    sha256_word(builder, edge.u);
    sha256_word(builder, edge.v);
  }
  return builder.finalize().to_lower_hex();
}

[[nodiscard]] std::string corrected_surrogate_tree_sha256(
    std::size_t point_count,
    std::size_t order,
    std::span<const CorrectedSurrogateTreeEdge> tree) {
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(
      "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
      "corrected-surrogate-level-tree-v1/sha256/");
  sha256_word(builder, static_cast<std::uint64_t>(point_count));
  sha256_word(builder, static_cast<std::uint64_t>(order));
  sha256_word(builder, static_cast<std::uint64_t>(tree.size()));
  for (const CorrectedSurrogateTreeEdge& edge : tree) {
    sha256_word(builder, static_cast<std::uint64_t>(edge.level_encoding));
    sha256_word(builder, std::bit_cast<std::uint64_t>(edge.encoded_binary64));
    sha256_word(builder, edge.u);
    sha256_word(builder, edge.v);
  }
  return builder.finalize().to_lower_hex();
}

[[nodiscard]] std::string surrogate_decision_sha256(
    std::size_t order,
    std::span<const ClassifiedTriangle> sources,
    std::span<const std::uint8_t> decisions,
    bool corrected) {
  if (sources.size() != decisions.size()) {
    throw std::logic_error(
        "the surrogate guardrail decision transcript has a wrong extent");
  }
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(
      corrected
          ? "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
            "corrected-decisions-v1/sha256/"
          : "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
            "raw-decisions-v1/sha256/");
  sha256_word(builder, static_cast<std::uint64_t>(order));
  sha256_word(builder, static_cast<std::uint64_t>(sources.size()));
  for (std::size_t index = 0U; index < sources.size(); ++index) {
    const ClassifiedTriangle& source = sources[index];
    const std::uint8_t decision = decisions[index];
    sha256_word(builder, std::bit_cast<std::uint64_t>(source.squared_level));
    sha256_word(builder, source.triangle.a);
    sha256_word(builder, source.triangle.b);
    sha256_word(builder, source.triangle.c);
    sha256_word(builder, static_cast<std::uint64_t>(decision));
  }
  return builder.finalize().to_lower_hex();
}

struct NonnegativeBinary64Dyadic {
  std::uint64_t significand{};
  int exponent{};
};

[[nodiscard]] NonnegativeBinary64Dyadic decode_nonnegative_binary64_dyadic(
    double value,
    const char* role) {
  if (!std::isfinite(value) || value < 0.0) {
    throw std::runtime_error(std::string{role} + " is not finite non-negative");
  }
  const std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
  const std::uint64_t exponent_field = (bits >> 52U) & UINT64_C(0x7ff);
  const std::uint64_t fraction = bits & UINT64_C(0x000fffffffffffff);
  NonnegativeBinary64Dyadic result;
  if (exponent_field == 0U) {
    result.significand = fraction;
    result.exponent = -1074;
  } else {
    result.significand = fraction | (UINT64_C(1) << 52U);
    result.exponent =
        static_cast<int>(exponent_field) - 1023 - 52;
  }
  if (result.significand != 0U) {
    const int trailing = std::countr_zero(result.significand);
    result.significand >>= trailing;
    result.exponent += trailing;
  }
  return result;
}

[[nodiscard]] int compare_dyadics(
    NonnegativeBinary64Dyadic left,
    NonnegativeBinary64Dyadic right) noexcept {
  if (left.significand == 0U || right.significand == 0U) {
    if (left.significand == right.significand) {
      return 0;
    }
    return left.significand == 0U ? -1 : 1;
  }
  const int left_width =
      static_cast<int>(std::bit_width(left.significand));
  const int right_width =
      static_cast<int>(std::bit_width(right.significand));
  const int left_top = left.exponent + left_width;
  const int right_top = right.exponent + right_width;
  if (left_top != right_top) {
    return left_top < right_top ? -1 : 1;
  }
  if (left_width < right_width) {
    left.significand <<= right_width - left_width;
  } else if (right_width < left_width) {
    right.significand <<= left_width - right_width;
  }
  if (left.significand == right.significand) {
    return 0;
  }
  return left.significand < right.significand ? -1 : 1;
}

[[nodiscard]] int compare_surrogate_weight_to_gabriel_level(
    double raw_squared_weight,
    double gabriel_squared_radius) {
  NonnegativeBinary64Dyadic converted =
      decode_nonnegative_binary64_dyadic(
          raw_squared_weight, "a surrogate squared weight");
  converted.exponent -= 2;
  return compare_dyadics(
      converted,
      decode_nonnegative_binary64_dyadic(
          gabriel_squared_radius, "a Gabriel squared radius"));
}

[[nodiscard]] NonnegativeBinary64Dyadic corrected_tree_edge_level(
    const CorrectedSurrogateTreeEdge& edge) {
  NonnegativeBinary64Dyadic level = decode_nonnegative_binary64_dyadic(
      edge.encoded_binary64, "a corrected surrogate encoded level");
  switch (edge.level_encoding) {
    case CorrectedLevelEncoding::raw_squared_weight_exact_dyadic_divide_by_4:
      level.exponent -= 2;
      return level;
    case CorrectedLevelEncoding::direct_Gabriel_squared_level:
      return level;
  }
  throw std::logic_error(
      "the corrected surrogate has an invalid level encoding");
}

[[nodiscard]] int compare_corrected_tree_edge_levels(
    const CorrectedSurrogateTreeEdge& left,
    const CorrectedSurrogateTreeEdge& right) {
  return compare_dyadics(
      corrected_tree_edge_level(left), corrected_tree_edge_level(right));
}

[[nodiscard]] const char* corrected_level_encoding_name(
    CorrectedLevelEncoding encoding) {
  switch (encoding) {
    case CorrectedLevelEncoding::raw_squared_weight_exact_dyadic_divide_by_4:
      return "raw_squared_weight_exact_dyadic_divide_by_4";
    case CorrectedLevelEncoding::direct_Gabriel_squared_level:
      return "direct_Gabriel_squared_level";
  }
  throw std::logic_error(
      "the corrected surrogate has an invalid level encoding");
}

[[nodiscard]] std::optional<double> first_surrogate_connection_raw_weight(
    std::span<const SurrogateTreeEdge> tree,
    const Triangle& triangle) {
  DisjointSet components(tree.size() + 1U);
  std::size_t plateau_begin = 0U;
  while (plateau_begin < tree.size()) {
    const double raw_squared_weight = tree[plateau_begin].squared_weight;
    std::size_t plateau_end = plateau_begin + 1U;
    while (plateau_end < tree.size() &&
           tree[plateau_end].squared_weight == raw_squared_weight) {
      ++plateau_end;
    }
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      static_cast<void>(components.unite(
          static_cast<std::size_t>(tree[index].u),
          static_cast<std::size_t>(tree[index].v)));
    }
    if (triangle_vertices_connected(components, triangle)) {
      return raw_squared_weight;
    }
    plateau_begin = plateau_end;
  }
  return std::nullopt;
}

void replay_surrogate_deadlines(
    std::span<const ClassifiedTriangle> sources,
    std::span<const SurrogateTreeEdge> tree,
    SurrogateGuardrailOrderSummary& summary,
    bool emit_records) {
  constexpr std::uint8_t kConnectedBefore = 1U;
  constexpr std::uint8_t kConnectedAt = 2U;
  constexpr std::uint8_t kLate = 3U;
  constexpr std::uint8_t kNever = 4U;
  constexpr std::uint8_t kUnsupported = 5U;
  const auto begin = Clock::now();
  summary.source_triangle_count = sources.size();
  for (std::size_t index = 1U; index < sources.size(); ++index) {
    if (ClassifiedTriangleLess{}(sources[index], sources[index - 1U])) {
      throw std::logic_error(
          "the surrogate guardrail source stream is not canonical");
    }
  }
  std::vector<std::uint8_t> decisions(sources.size(), 0U);
  std::vector<std::uint8_t> corrected_decisions(sources.size(), 0U);
  DisjointSet components(summary.tree_edge_count + 1U);
  DisjointSet corrected_components(summary.tree_edge_count + 1U);
  std::vector<CorrectedSurrogateTreeEdge> corrected_tree;
  corrected_tree.reserve(summary.tree_edge_count);
  const auto apply_raw_tree_edge = [&](const SurrogateTreeEdge& edge) {
    const std::size_t u = static_cast<std::size_t>(edge.u);
    const std::size_t v = static_cast<std::size_t>(edge.v);
    if (!components.unite(u, v)) {
      throw std::logic_error(
          "the authenticated surrogate tree contains a cycle");
    }
    if (corrected_components.unite(u, v)) {
      corrected_tree.push_back(make_corrected_surrogate_tree_edge(
          edge.u,
          edge.v,
          edge.squared_weight,
          CorrectedLevelEncoding::
              raw_squared_weight_exact_dyadic_divide_by_4));
    }
  };
  morsehgp3d::contract::CanonicalSha256Builder correction_builder;
  correction_builder.update(
      "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
      "canonical-greedy-overlay-v1/sha256/");
  sha256_word(correction_builder, static_cast<std::uint64_t>(summary.order));
  std::size_t tree_position = 0U;
  std::size_t plateau_begin = 0U;
  while (plateau_begin < sources.size()) {
    const double source_level = sources[plateau_begin].squared_level;
    if (!std::isfinite(source_level) || source_level < 0.0) {
      throw std::runtime_error(
          "the surrogate guardrail source contains an invalid level");
    }
    std::size_t plateau_end = plateau_begin + 1U;
    while (plateau_end < sources.size() &&
           sources[plateau_end].squared_level == source_level) {
      ++plateau_end;
    }
    while (
        tree_position < tree.size() &&
        compare_surrogate_weight_to_gabriel_level(
            tree[tree_position].squared_weight, source_level) < 0) {
      apply_raw_tree_edge(tree[tree_position]);
      ++tree_position;
    }
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      const ClassifiedTriangle& source = sources[index];
      if (source.status == TriangleStatus::ambiguous) {
        decisions[index] = kUnsupported;
        corrected_decisions[index] = kUnsupported;
      } else if (source.status != TriangleStatus::gabriel_binary64) {
        throw std::logic_error(
            "the guardrail source stream contains a non-retained status");
      } else if (triangle_vertices_connected(components, source.triangle)) {
        decisions[index] = kConnectedBefore;
      }
      if (source.status == TriangleStatus::gabriel_binary64 &&
          triangle_vertices_connected(
              corrected_components, source.triangle)) {
        corrected_decisions[index] = kConnectedBefore;
      }
    }
    while (
        tree_position < tree.size() &&
        compare_surrogate_weight_to_gabriel_level(
            tree[tree_position].squared_weight, source_level) == 0) {
      apply_raw_tree_edge(tree[tree_position]);
      ++tree_position;
    }
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      if (decisions[index] != 0U) {
        continue;
      }
      decisions[index] = triangle_vertices_connected(
                             components, sources[index].triangle)
                             ? kConnectedAt
                             : kLate;
    }
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      const ClassifiedTriangle& source = sources[index];
      if (corrected_decisions[index] == kUnsupported ||
          corrected_decisions[index] == kConnectedBefore) {
        continue;
      }
      if (!triangle_vertices_connected(
              corrected_components, source.triangle)) {
        sha256_word(
            correction_builder,
            std::bit_cast<std::uint64_t>(source.squared_level));
        sha256_word(correction_builder, source.triangle.a);
        sha256_word(correction_builder, source.triangle.b);
        sha256_word(correction_builder, source.triangle.c);
        ++summary.correction_triangle_count;
        if (emit_records) {
          summary.emitted_correction_records.push_back(source);
        }
        if (corrected_components.unite(
                static_cast<std::size_t>(source.triangle.a),
                static_cast<std::size_t>(source.triangle.b))) {
          ++summary.useful_correction_union_count;
          corrected_tree.push_back(make_corrected_surrogate_tree_edge(
              source.triangle.a,
              source.triangle.b,
              source.squared_level,
              CorrectedLevelEncoding::direct_Gabriel_squared_level));
        }
        if (corrected_components.unite(
                static_cast<std::size_t>(source.triangle.a),
                static_cast<std::size_t>(source.triangle.c))) {
          ++summary.useful_correction_union_count;
          corrected_tree.push_back(make_corrected_surrogate_tree_edge(
              source.triangle.a,
              source.triangle.c,
              source.squared_level,
              CorrectedLevelEncoding::direct_Gabriel_squared_level));
        }
      }
    }
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      if (sources[index].status != TriangleStatus::gabriel_binary64) {
        continue;
      }
      if (!triangle_vertices_connected(
              corrected_components, sources[index].triangle)) {
        ++summary.corrected_postcondition_violation_count;
        corrected_decisions[index] = kLate;
      } else if (corrected_decisions[index] == 0U) {
        corrected_decisions[index] = kConnectedAt;
      }
    }
    plateau_begin = plateau_end;
  }

  while (tree_position < tree.size()) {
    apply_raw_tree_edge(tree[tree_position]);
    ++tree_position;
  }
  if (components.component_count() != 1U ||
      corrected_components.component_count() != 1U ||
      corrected_tree.size() != summary.tree_edge_count) {
    throw std::logic_error(
        "the surrogate tree or its corrected overlay did not close");
  }
  for (std::size_t index = 1U; index < corrected_tree.size(); ++index) {
    if (compare_corrected_tree_edge_levels(
            corrected_tree[index], corrected_tree[index - 1U]) < 0) {
      throw std::logic_error(
          "the corrected surrogate merge tree is not level ordered");
    }
  }
  summary.corrected_tree_edge_count = corrected_tree.size();
  summary.corrected_tree_root_edge = corrected_tree.back();
  CorrectedSurrogateTreeEdge previous_corrected_level;
  bool has_previous_corrected_level = false;
  for (const CorrectedSurrogateTreeEdge& edge : corrected_tree) {
    if (!has_previous_corrected_level ||
        compare_corrected_tree_edge_levels(
            edge, previous_corrected_level) != 0) {
      ++summary.corrected_tree_distinct_level_count;
      previous_corrected_level = edge;
      has_previous_corrected_level = true;
    }
  }
  summary.corrected_tree_sha256 = corrected_surrogate_tree_sha256(
      summary.tree_edge_count + 1U, summary.order, corrected_tree);
  if (emit_records) {
    summary.emitted_corrected_tree_edges = corrected_tree;
  }

  for (std::size_t index = 0U; index < decisions.size(); ++index) {
    const ClassifiedTriangle& source = sources[index];
    switch (decisions[index]) {
      case kConnectedBefore:
        ++summary.connected_before_count;
        ++summary.supported_triangle_count;
        break;
      case kConnectedAt:
        ++summary.connected_at_count;
        ++summary.supported_triangle_count;
        break;
      case kLate:
        ++summary.late_count;
        ++summary.supported_triangle_count;
        remember_surrogate_failure(summary.first_failure, source, "late");
        remember_surrogate_failure(summary.first_late, source, "late");
        break;
      case kNever:
        ++summary.never_count;
        ++summary.supported_triangle_count;
        remember_surrogate_failure(summary.first_failure, source, "never");
        break;
      case kUnsupported:
        ++summary.unsupported_count;
        remember_surrogate_failure(
            summary.first_failure, source, "unsupported");
        remember_surrogate_failure(
            summary.first_unsupported, source, "unsupported");
        break;
      default:
        throw std::logic_error(
          "the surrogate guardrail left a source unclassified");
    }
  }
  for (std::size_t index = 0U; index < corrected_decisions.size(); ++index) {
    const ClassifiedTriangle& source = sources[index];
    switch (corrected_decisions[index]) {
      case kConnectedBefore:
        ++summary.corrected_connected_before_count;
        break;
      case kConnectedAt:
        ++summary.corrected_connected_at_count;
        break;
      case kLate:
        ++summary.corrected_late_count;
        remember_surrogate_failure(
            summary.corrected_first_failure, source, "late");
        remember_surrogate_failure(
            summary.corrected_first_late, source, "late");
        break;
      case kNever:
        ++summary.corrected_never_count;
        remember_surrogate_failure(
            summary.corrected_first_failure, source, "never");
        break;
      case kUnsupported:
        ++summary.corrected_unsupported_count;
        remember_surrogate_failure(
            summary.corrected_first_failure, source, "unsupported");
        remember_surrogate_failure(
            summary.corrected_first_unsupported, source, "unsupported");
        break;
      default:
        throw std::logic_error(
            "the corrected surrogate guardrail left a source unclassified");
    }
  }
  const std::size_t partition_count =
      summary.connected_before_count + summary.connected_at_count +
      summary.late_count + summary.never_count + summary.unsupported_count;
  const std::size_t corrected_partition_count =
      summary.corrected_connected_before_count +
      summary.corrected_connected_at_count + summary.corrected_late_count +
      summary.corrected_never_count + summary.corrected_unsupported_count;
  if (partition_count != summary.source_triangle_count ||
      summary.supported_triangle_count + summary.unsupported_count !=
          summary.source_triangle_count ||
      corrected_partition_count != summary.source_triangle_count ||
      summary.corrected_late_count + summary.corrected_never_count !=
          summary.corrected_postcondition_violation_count) {
    throw std::logic_error(
        "the surrogate guardrail classifications do not close");
  }
  if (summary.first_late.present) {
    const std::optional<double> connection_raw_weight =
        first_surrogate_connection_raw_weight(
            tree, summary.first_late.source.triangle);
    if (!connection_raw_weight.has_value() ||
        compare_surrogate_weight_to_gabriel_level(
            *connection_raw_weight,
            summary.first_late.source.squared_level) <= 0) {
      throw std::logic_error(
          "a late surrogate witness has no strictly later connection level");
    }
    summary.first_late.observed_connection_raw_squared_weight =
        *connection_raw_weight;
    summary.first_late.observed_connection_level_present = true;
    if (summary.first_failure.present &&
        summary.first_failure.classification == "late" &&
        summary.first_failure.source.triangle.a ==
            summary.first_late.source.triangle.a &&
        summary.first_failure.source.triangle.b ==
            summary.first_late.source.triangle.b &&
        summary.first_failure.source.triangle.c ==
            summary.first_late.source.triangle.c) {
      summary.first_failure.observed_connection_raw_squared_weight =
          *connection_raw_weight;
      summary.first_failure.observed_connection_level_present = true;
    }
  }
  correction_builder.update("/count/");
  sha256_word(
      correction_builder,
      static_cast<std::uint64_t>(summary.correction_triangle_count));
  summary.correction_sha256 =
      correction_builder.finalize().to_lower_hex();
  summary.decision_sha256 = surrogate_decision_sha256(
      summary.order, sources, decisions, false);
  summary.corrected_decision_sha256 = surrogate_decision_sha256(
      summary.order, sources, corrected_decisions, true);
  summary.deadline_replay_nanoseconds =
      nanoseconds(Clock::now() - begin);
}

[[nodiscard]] SurrogateGuardrailOrderSummary
build_surrogate_guardrail_order(
    std::span<const Point3> points,
    const morsehgp3d::gpu::Binary64LbvhTopKResult& top_k,
    std::span<const std::size_t> morton_position_by_canonical_id,
    std::span<const PointId> source_point_id_by_canonical_id,
    std::span<const ClassifiedTriangle> sources,
    std::size_t order,
    bool emit_records) {
  const std::size_t point_count = points.size();
  if (point_count >
      std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error("the surrogate edge count overflows size_t");
  }
  const std::size_t edge_count = point_count * 2U - 1U;
  const std::size_t order_index = order - 1U;
  const auto edge_begin = Clock::now();
  std::vector<SurrogateTreeEdge> edges(edge_count);
  const auto core_distance = [&](morsehgp3d::spatial::PointId point_id) {
    const std::size_t point = static_cast<std::size_t>(point_id);
    const std::size_t position = morton_position_by_canonical_id[point];
    return top_k.squared_distances[
        position * top_k.audit.maximum_order + order_index];
  };
  for (std::size_t position = 0U; position < point_count; ++position) {
    const morsehgp3d::spatial::PointId canonical_source =
        top_k.source_point_ids_by_morton_position[position];
    if (position != 0U) {
      const morsehgp3d::spatial::PointId canonical_previous =
          top_k.source_point_ids_by_morton_position[position - 1U];
      const PointId source = source_point_id_by_canonical_id[
          static_cast<std::size_t>(canonical_source)];
      const PointId previous = source_point_id_by_canonical_id[
          static_cast<std::size_t>(canonical_previous)];
      const double distance = point_squared_distance(
          points[static_cast<std::size_t>(source)],
          points[static_cast<std::size_t>(previous)]);
      const double weight = std::max(
          distance,
          std::max(
              core_distance(canonical_source),
              core_distance(canonical_previous)));
      edges[position - 1U] = make_surrogate_tree_edge(
          canonical_source, canonical_previous, weight);
    }
    const std::size_t neighbor_record =
        position * top_k.audit.maximum_order + order_index;
    const morsehgp3d::spatial::PointId canonical_neighbor =
        top_k.neighbor_point_ids[neighbor_record];
    const double weight = std::max(
        top_k.squared_distances[neighbor_record],
        std::max(
            core_distance(canonical_source),
            core_distance(canonical_neighbor)));
    edges[point_count - 1U + position] = make_surrogate_tree_edge(
        canonical_source, canonical_neighbor, weight);
  }
  const std::uint64_t edge_build_nanoseconds =
      nanoseconds(Clock::now() - edge_begin);

  const auto reduction_begin = Clock::now();
  std::sort(edges.begin(), edges.end(), surrogate_endpoint_less);
  std::size_t write = 0U;
  for (const SurrogateTreeEdge& edge : edges) {
    if (write != 0U && edges[write - 1U].u == edge.u &&
        edges[write - 1U].v == edge.v) {
      edges[write - 1U].squared_weight =
          std::min(edges[write - 1U].squared_weight, edge.squared_weight);
    } else {
      edges[write++] = edge;
    }
  }
  edges.resize(write);
  std::sort(edges.begin(), edges.end(), surrogate_weight_less);

  DisjointSet components(point_count);
  std::vector<SurrogateTreeEdge> tree;
  tree.reserve(point_count - 1U);
  for (const SurrogateTreeEdge& edge : edges) {
    if (components.unite(
            static_cast<std::size_t>(edge.u),
            static_cast<std::size_t>(edge.v))) {
      tree.push_back(edge);
      if (tree.size() == point_count - 1U) {
        break;
      }
    }
  }
  if (tree.size() != point_count - 1U ||
      components.component_count() != 1U) {
    throw std::runtime_error(
        "the Morton-chain surrogate failed to produce a spanning tree");
  }
  std::uint64_t compatible_digest = UINT64_C(1469598103934665603);
  digest_word(compatible_digest, static_cast<std::uint64_t>(order));
  for (const SurrogateTreeEdge& edge : tree) {
    digest_word(
        compatible_digest,
        std::bit_cast<std::uint64_t>(edge.squared_weight));
    digest_word(compatible_digest, edge.u);
    digest_word(compatible_digest, edge.v);
  }
  for (SurrogateTreeEdge& edge : tree) {
    edge = make_surrogate_tree_edge(
        source_point_id_by_canonical_id[static_cast<std::size_t>(edge.u)],
        source_point_id_by_canonical_id[static_cast<std::size_t>(edge.v)],
        edge.squared_weight);
  }
  std::sort(tree.begin(), tree.end(), surrogate_weight_less);
  std::size_t distinct_level_count = 0U;
  std::uint64_t previous_level_bits{};
  bool has_previous_level = false;
  for (const SurrogateTreeEdge& edge : tree) {
    const std::uint64_t bits =
        std::bit_cast<std::uint64_t>(edge.squared_weight);
    if (!has_previous_level || bits != previous_level_bits) {
      ++distinct_level_count;
      previous_level_bits = bits;
      has_previous_level = true;
    }
  }

  SurrogateGuardrailOrderSummary summary;
  summary.order = order;
  summary.proposed_edge_count = edge_count;
  summary.unique_edge_count = edges.size();
  summary.tree_edge_count = tree.size();
  summary.distinct_tree_level_count = distinct_level_count;
  summary.root_squared_weight = tree.back().squared_weight;
  summary.tree_sha256 = surrogate_tree_sha256(point_count, order, tree);
  summary.surrogate_compatible_digest = hex64(compatible_digest);
  summary.edge_build_nanoseconds = edge_build_nanoseconds;
  summary.tree_reduction_nanoseconds =
      nanoseconds(Clock::now() - reduction_begin);
  std::vector<SurrogateTreeEdge>().swap(edges);
  replay_surrogate_deadlines(sources, tree, summary, emit_records);
  if (emit_records) {
    summary.emitted_tree_edges = tree;
  }
  return summary;
}

[[nodiscard]] SurrogateGuardrailRun run_surrogate_guardrails(
    std::span<const Point3> points,
    std::span<const ClassifiedTriangle> sources,
    std::size_t maximum_order,
    std::size_t seed_window_radius,
    std::size_t cpu_workers,
    bool emit_records) {
  SurrogateGuardrailRun run;
  morsehgp3d::gpu::Binary64LbvhTopKResult top_k;
  std::vector<std::size_t> morton_position_by_canonical_id(points.size());
  std::vector<PointId> source_point_id_by_canonical_id(points.size());
  {
    const auto canonicalization_begin = Clock::now();
    std::vector<morsehgp3d::exact::CertifiedPoint3> certified_points;
    certified_points.reserve(points.size());
    for (const Point3& point : points) {
      certified_points.push_back(
          morsehgp3d::exact::CertifiedPoint3::from_binary64(
              point.x, point.y, point.z));
    }
    morsehgp3d::spatial::CanonicalPointCloud cloud =
        morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(
            certified_points);
    certified_points.clear();
    certified_points.shrink_to_fit();
    run.canonicalization_nanoseconds =
        nanoseconds(Clock::now() - canonicalization_begin);

    const auto lbvh_begin = Clock::now();
    morsehgp3d::gpu::MortonLbvhBuildContext build_context(points.size());
    morsehgp3d::gpu::MortonLbvhDeviceBuildResult build_result =
        build_context.build(cloud);
    if (!build_result.complete_certified_build() ||
        !build_result.cuda_qualified_build()) {
      throw std::runtime_error(
          "the surrogate guardrail requires a certified CUDA LBVH build");
    }
    morsehgp3d::gpu::MortonLbvhDeviceTraversalLease traversal_lease =
        build_context.release_device_traversal_lease(build_result);
    morsehgp3d::gpu::Binary64LbvhTopKContext top_k_context(
        cloud, std::move(traversal_lease));
    run.lbvh_build_nanoseconds = nanoseconds(Clock::now() - lbvh_begin);

    const auto top_k_begin = Clock::now();
    top_k = top_k_context.query_all(
        cloud, maximum_order, seed_window_radius);
    run.top_k_query_nanoseconds = nanoseconds(Clock::now() - top_k_begin);
    if (!top_k.validated_complete_binary64_transcript()) {
      throw std::runtime_error(
          "the surrogate guardrail received an incomplete top-k transcript");
    }
    run.top_k_audit = top_k.audit;

    std::vector<std::uint8_t> source_seen(points.size(), 0U);
    morsehgp3d::contract::CanonicalSha256Builder mapping_builder;
    mapping_builder.update(
        "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        "canonical-to-source-v1/sha256/");
    sha256_word(mapping_builder, static_cast<std::uint64_t>(points.size()));
    for (std::size_t canonical_index = 0U;
         canonical_index < points.size();
         ++canonical_index) {
      const auto canonical_id =
          static_cast<morsehgp3d::spatial::PointId>(canonical_index);
      const std::size_t source_index = cloud.source_index(canonical_id);
      if (source_index >= points.size() || source_seen[source_index] != 0U) {
        throw std::runtime_error(
            "the canonical-to-source point map is not a permutation");
      }
      source_seen[source_index] = 1U;
      source_point_id_by_canonical_id[canonical_index] =
          static_cast<PointId>(source_index);
      sha256_word(mapping_builder, static_cast<std::uint64_t>(canonical_index));
      sha256_word(mapping_builder, static_cast<std::uint64_t>(source_index));
      for (const std::uint64_t bits :
           cloud.point(canonical_id).canonical_input_bits()) {
        sha256_word(mapping_builder, bits);
      }
    }
    run.canonical_to_source_mapping_sha256 =
        mapping_builder.finalize().to_lower_hex();

    morsehgp3d::contract::CanonicalSha256Builder transcript_builder;
    transcript_builder.update(
        "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        "remapped-top-k-v1/sha256/");
    sha256_word(transcript_builder, static_cast<std::uint64_t>(points.size()));
    sha256_word(transcript_builder, static_cast<std::uint64_t>(maximum_order));
    for (std::size_t position = 0U; position < points.size(); ++position) {
      const auto canonical_id =
          top_k.source_point_ids_by_morton_position[position];
      const std::size_t canonical_index =
          static_cast<std::size_t>(canonical_id);
      const PointId source_id =
          source_point_id_by_canonical_id[canonical_index];
      morton_position_by_canonical_id[canonical_index] = position;
      sha256_word(transcript_builder, static_cast<std::uint64_t>(position));
      sha256_word(transcript_builder, source_id);
      for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
        const std::size_t record = position * maximum_order + rank;
        const auto canonical_neighbor = top_k.neighbor_point_ids[record];
        const PointId neighbor_id = source_point_id_by_canonical_id[
            static_cast<std::size_t>(canonical_neighbor)];
        sha256_word(transcript_builder, static_cast<std::uint64_t>(rank));
        sha256_word(transcript_builder, neighbor_id);
        sha256_word(
            transcript_builder,
            std::bit_cast<std::uint64_t>(top_k.squared_distances[record]));
      }
    }
    run.remapped_top_k_transcript_sha256 =
        transcript_builder.finalize().to_lower_hex();
  }

  run.orders.resize(maximum_order);
  run.order_worker_count = std::max(
      std::size_t{1},
      std::min(
          maximum_order,
          cpu_workers));
  std::atomic<std::size_t> next_order_index{0U};
  std::atomic<bool> stop_order_workers{false};
  std::exception_ptr order_failure;
  std::mutex order_failure_mutex;
  std::vector<std::thread> order_workers;
  order_workers.reserve(run.order_worker_count);
  for (std::size_t worker = 0U; worker < run.order_worker_count; ++worker) {
    order_workers.emplace_back([&] {
      while (!stop_order_workers.load(std::memory_order_relaxed)) {
        const std::size_t order_index =
            next_order_index.fetch_add(1U, std::memory_order_relaxed);
        if (order_index >= maximum_order) {
          return;
        }
        try {
          run.orders[order_index] = build_surrogate_guardrail_order(
              points,
              top_k,
              morton_position_by_canonical_id,
              source_point_id_by_canonical_id,
              sources,
              order_index + 1U,
              emit_records);
        } catch (...) {
          {
            std::scoped_lock lock(order_failure_mutex);
            if (!order_failure) {
              order_failure = std::current_exception();
            }
          }
          stop_order_workers.store(true, std::memory_order_relaxed);
          return;
        }
      }
    });
  }
  for (std::thread& worker : order_workers) {
    worker.join();
  }
  if (order_failure) {
    std::rethrow_exception(order_failure);
  }
  return run;
}

[[nodiscard]] std::optional<std::uint16_t> find_directed_csr_rank(
    const CsrGraph& graph,
    std::span<const std::uint16_t> directed_ranks,
    PointId source,
    PointId target) {
  if (source + 1U >= graph.offsets.size() ||
      directed_ranks.size() != graph.neighbors.size()) {
    throw std::logic_error("the compact CSR rank transcript is misaligned");
  }
  const std::size_t begin =
      static_cast<std::size_t>(graph.offsets[static_cast<std::size_t>(source)]);
  const std::size_t end = static_cast<std::size_t>(
      graph.offsets[static_cast<std::size_t>(source) + 1U]);
  const auto first =
      graph.neighbors.begin() + static_cast<std::ptrdiff_t>(begin);
  const auto last = graph.neighbors.begin() + static_cast<std::ptrdiff_t>(end);
  const auto found = std::lower_bound(first, last, target);
  if (found == last || *found != target) {
    return std::nullopt;
  }
  const std::size_t position = static_cast<std::size_t>(
      std::distance(graph.neighbors.begin(), found));
  return directed_ranks[position];
}

void add_neighbor_rank_observation(
    NeighborRankHistogram& histogram,
    std::size_t maximum_rank,
    std::uint16_t observed_rank) {
  if (observed_rank >= 1U && observed_rank <= maximum_rank) {
    histogram.exact_rank_counts[observed_rank] = checked_add_u64(
        histogram.exact_rank_counts[observed_rank],
        UINT64_C(1),
        "the neighbor-rank histogram overflows uint64");
    histogram.captured_count = checked_add_u64(
        histogram.captured_count,
        UINT64_C(1),
        "the neighbor-rank captured count overflows uint64");
    histogram.maximum_exact_rank = std::max(
        histogram.maximum_exact_rank,
        static_cast<std::size_t>(observed_rank));
    return;
  }
  if (observed_rank != maximum_rank + 1U) {
    throw std::logic_error(
        "the compact CSR transcript contains a rank outside its schema");
  }
  histogram.overflow_count = checked_add_u64(
      histogram.overflow_count,
      UINT64_C(1),
      "the neighbor-rank overflow count overflows uint64");
}

[[nodiscard]] GabrielNeighborRankSummary reduce_gabriel_neighbor_ranks(
    const CsrGraph& graph,
    std::span<const ClassifiedTriangle> records,
    const morsehgp3d::gpu::Binary64LbvhCsrNeighborRankResult& rank_result,
    std::uint64_t expected_accepted_triangle_count) {
  if (!rank_result.validated_complete_binary64_rank_prefix()) {
    throw std::invalid_argument(
        "Gabriel neighbor ranks require a complete compact LBVH transcript");
  }
  GabrielNeighborRankSummary summary;
  summary.maximum_rank = rank_result.audit.maximum_rank;
  const std::size_t histogram_extent = summary.maximum_rank + 1U;
  summary.directed_root_star.histogram.exact_rank_counts.assign(
      histogram_extent, 0U);
  summary.symmetric_union_star.histogram.exact_rank_counts.assign(
      histogram_extent, 0U);
  summary.mutual_star.histogram.exact_rank_counts.assign(
      histogram_extent, 0U);
  const auto require_rank = [&](PointId source, PointId target) {
    const std::optional<std::uint16_t> rank = find_directed_csr_rank(
        graph, rank_result.directed_csr_ranks, source, target);
    if (rank.has_value() &&
        (*rank < 1U || *rank > summary.maximum_rank + 1U)) {
      throw std::logic_error("a directed CSR rank violates the compact schema");
    }
    return rank;
  };
  const auto reduce_variant = [&summary](
                                  NeighborRankVariantSummary& variant,
                                  const ClassifiedTriangle& record,
                                  const std::array<
                                      std::optional<std::uint16_t>,
                                      3U>& root_ranks) {
    std::optional<std::uint16_t> best;
    for (const std::optional<std::uint16_t>& root_rank : root_ranks) {
      if (!root_rank.has_value()) {
        continue;
      }
      variant.considered_root_count = checked_add_u64(
          variant.considered_root_count,
          UINT64_C(1),
          "the PDEL witness-root count overflows uint64");
      best = best.has_value()
                 ? std::optional<std::uint16_t>{std::min(*best, *root_rank)}
                 : root_rank;
    }
    if (!best.has_value()) {
      variant.missing_witness_triangle_count = checked_add_u64(
          variant.missing_witness_triangle_count,
          UINT64_C(1),
          "the missing PDEL witness count overflows uint64");
      if (!variant.first_missing_witness_triangle_present) {
        variant.first_missing_witness_triangle = record;
        variant.first_missing_witness_triangle_present = true;
      }
      return;
    }
    add_neighbor_rank_observation(
        variant.histogram, summary.maximum_rank, *best);
    variant.witness_triangle_count = checked_add_u64(
        variant.witness_triangle_count,
        UINT64_C(1),
        "the PDEL witness triangle count overflows uint64");
  };

  for (const ClassifiedTriangle& record : records) {
    if (record.status != TriangleStatus::gabriel_binary64) {
      continue;
    }
    summary.accepted_triangle_count = checked_add_u64(
        summary.accepted_triangle_count,
        UINT64_C(1),
        "the accepted Gabriel rank count overflows uint64");
    const PointId a = record.triangle.a;
    const PointId b = record.triangle.b;
    const PointId c = record.triangle.c;
    const std::optional<std::uint16_t> ab = require_rank(a, b);
    const std::optional<std::uint16_t> ba = require_rank(b, a);
    const std::optional<std::uint16_t> ac = require_rank(a, c);
    const std::optional<std::uint16_t> ca = require_rank(c, a);
    const std::optional<std::uint16_t> bc = require_rank(b, c);
    const std::optional<std::uint16_t> cb = require_rank(c, b);
    if (ab.has_value() != ba.has_value() ||
        ac.has_value() != ca.has_value() ||
        bc.has_value() != cb.has_value()) {
      throw std::logic_error(
          "the ordinary-Delaunay CSR must contain both directions of every "
          "edge");
    }

    const std::size_t pair_count =
        static_cast<std::size_t>(ab.has_value()) +
        static_cast<std::size_t>(ac.has_value()) +
        static_cast<std::size_t>(bc.has_value());
    if (pair_count == 3U) {
      summary.complete_three_pair_triangle_count = checked_add_u64(
          summary.complete_three_pair_triangle_count,
          UINT64_C(1),
          "the complete PDEL triangle count overflows uint64");
    } else if (pair_count == 2U) {
      summary.partial_two_pair_witness_triangle_count = checked_add_u64(
          summary.partial_two_pair_witness_triangle_count,
          UINT64_C(1),
          "the support-two PDEL triangle count overflows uint64");
    } else {
      summary.insufficient_pair_witness_triangle_count = checked_add_u64(
          summary.insufficient_pair_witness_triangle_count,
          UINT64_C(1),
          "the insufficient PDEL witness count overflows uint64");
    }

    const auto directed_root = [](const auto& first, const auto& second) {
      return first.has_value() && second.has_value()
                 ? std::optional<std::uint16_t>{
                       std::max(*first, *second)}
                 : std::nullopt;
    };
    const auto union_root = [](
                                const auto& first_out,
                                const auto& first_in,
                                const auto& second_out,
                                const auto& second_in) {
      return first_out.has_value() && first_in.has_value() &&
                     second_out.has_value() && second_in.has_value()
                 ? std::optional<std::uint16_t>{std::max(
                       std::min(*first_out, *first_in),
                       std::min(*second_out, *second_in))}
                 : std::nullopt;
    };
    const auto mutual_root = [](
                                 const auto& first_out,
                                 const auto& first_in,
                                 const auto& second_out,
                                 const auto& second_in) {
      return first_out.has_value() && first_in.has_value() &&
                     second_out.has_value() && second_in.has_value()
                 ? std::optional<std::uint16_t>{std::max(
                       std::max(*first_out, *first_in),
                       std::max(*second_out, *second_in))}
                 : std::nullopt;
    };
    const std::array<std::optional<std::uint16_t>, 3U> directed_roots{
        directed_root(ab, ac),
        directed_root(ba, bc),
        directed_root(ca, cb)};
    const std::array<std::optional<std::uint16_t>, 3U> union_roots{
        union_root(ab, ba, ac, ca),
        union_root(ba, ab, bc, cb),
        union_root(ca, ac, cb, bc)};
    const std::array<std::optional<std::uint16_t>, 3U> mutual_roots{
        mutual_root(ab, ba, ac, ca),
        mutual_root(ba, ab, bc, cb),
        mutual_root(ca, ac, cb, bc)};
    reduce_variant(summary.directed_root_star, record, directed_roots);
    reduce_variant(summary.symmetric_union_star, record, union_roots);
    reduce_variant(summary.mutual_star, record, mutual_roots);

    const auto best_rank = [](const auto& roots) {
      std::optional<std::uint16_t> best;
      for (const auto& rank : roots) {
        if (rank.has_value()) {
          best = best.has_value()
                     ? std::optional<std::uint16_t>{std::min(*best, *rank)}
                     : rank;
        }
      }
      return best;
    };
    const std::optional<std::uint16_t> directed_rank =
        best_rank(directed_roots);
    const std::optional<std::uint16_t> union_rank = best_rank(union_roots);
    const std::optional<std::uint16_t> mutual_rank = best_rank(mutual_roots);
    if (directed_rank.has_value() != union_rank.has_value() ||
        directed_rank.has_value() != mutual_rank.has_value()) {
      throw std::logic_error(
          "the PDEL witness-root availability differs between variants");
    }
    if (directed_rank.has_value() &&
        (*union_rank > *directed_rank || *directed_rank > *mutual_rank)) {
      throw std::logic_error(
          "the union/directed/mutual neighbor-rank ordering is inconsistent");
    }
  }
  if (summary.accepted_triangle_count != expected_accepted_triangle_count ||
      summary.complete_three_pair_triangle_count +
              summary.partial_two_pair_witness_triangle_count +
              summary.insufficient_pair_witness_triangle_count !=
          summary.accepted_triangle_count) {
    throw std::logic_error(
        "the Gabriel neighbor-rank triangle partition does not close");
  }
  const auto closes = [&](const NeighborRankVariantSummary& variant) {
    return variant.witness_triangle_count +
                   variant.missing_witness_triangle_count ==
               summary.accepted_triangle_count &&
           variant.histogram.captured_count +
                   variant.histogram.overflow_count ==
               variant.witness_triangle_count;
  };
  if (!closes(summary.directed_root_star) ||
      !closes(summary.symmetric_union_star) ||
      !closes(summary.mutual_star)) {
    throw std::logic_error(
        "a Gabriel neighbor-rank histogram does not close");
  }
  return summary;
}

[[nodiscard]] double fixed_binary64_squared_distance_cpu(
    const morsehgp3d::exact::CertifiedPoint3& first,
    const morsehgp3d::exact::CertifiedPoint3& second) noexcept {
  // Volatile temporaries make the host replay use the same elementary
  // round-to-nearest operations as __dsub_rn/__dmul_rn/__dadd_rn on device;
  // in particular, neither FMA contraction nor reassociation is admissible.
  const volatile double dx =
      first.binary64_coordinate(0U) - second.binary64_coordinate(0U);
  const volatile double dy =
      first.binary64_coordinate(1U) - second.binary64_coordinate(1U);
  const volatile double dz =
      first.binary64_coordinate(2U) - second.binary64_coordinate(2U);
  const volatile double xx = dx * dx;
  const volatile double yy = dy * dy;
  const volatile double zz = dz * dz;
  const volatile double xy = xx + yy;
  const volatile double squared_distance = xy + zz;
  return squared_distance;
}

[[nodiscard]] NeighborRankBruteforceReplay
replay_neighbor_ranks_by_bounded_bruteforce(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    const CsrGraph& graph,
    std::span<const PointId> source_point_id_by_canonical_id,
    const morsehgp3d::gpu::Binary64LbvhCsrNeighborRankResult& rank_result) {
  NeighborRankBruteforceReplay replay;
  replay.point_count = cloud.size();
  if (cloud.size() > NeighborRankBruteforceReplay::maximum_point_count) {
    return replay;
  }
  if (std::fegetround() != FE_TONEAREST) {
    throw std::runtime_error(
        "the bounded binary64 rank replay requires FE_TONEAREST");
  }
  if (graph.offsets.size() != cloud.size() + 1U ||
      graph.neighbors.size() != rank_result.directed_csr_ranks.size() ||
      source_point_id_by_canonical_id.size() != cloud.size()) {
    throw std::logic_error(
        "the bounded binary64 rank replay received misaligned extents");
  }

  std::vector<PointId> canonical_point_id_by_source_id(cloud.size());
  std::vector<std::uint8_t> source_seen(cloud.size(), 0U);
  for (std::size_t canonical = 0U; canonical < cloud.size(); ++canonical) {
    const PointId source = source_point_id_by_canonical_id[canonical];
    if (source >= cloud.size() || source_seen[source] != 0U) {
      throw std::logic_error(
          "the bounded binary64 rank replay mapping is not a permutation");
    }
    source_seen[source] = 1U;
    canonical_point_id_by_source_id[source] =
        static_cast<PointId>(canonical);
    replay.canonical_source_mapping_non_identity_observed =
        replay.canonical_source_mapping_non_identity_observed ||
        source != canonical;
  }

  std::vector<std::pair<double, PointId>> ordered_candidates;
  ordered_candidates.reserve(cloud.size() - 1U);
  std::vector<std::size_t> exact_rank_by_canonical_id(cloud.size());
  const std::uint16_t overflow_rank = static_cast<std::uint16_t>(
      rank_result.audit.maximum_rank + 1U);
  for (std::size_t source_canonical = 0U;
       source_canonical < cloud.size();
       ++source_canonical) {
    ordered_candidates.clear();
    const PointId source_canonical_id =
        static_cast<PointId>(source_canonical);
    for (std::size_t candidate = 0U; candidate < cloud.size(); ++candidate) {
      if (candidate == source_canonical) {
        continue;
      }
      const PointId candidate_id = static_cast<PointId>(candidate);
      const double squared_distance = fixed_binary64_squared_distance_cpu(
          cloud.point(source_canonical_id), cloud.point(candidate_id));
      if (std::isnan(squared_distance)) {
        throw std::runtime_error(
            "the bounded binary64 rank replay produced NaN");
      }
      ordered_candidates.emplace_back(squared_distance, candidate_id);
    }
    std::sort(
        ordered_candidates.begin(),
        ordered_candidates.end(),
        [](const auto& left, const auto& right) {
          return left.first < right.first ||
                 (left.first == right.first && left.second < right.second);
        });
    for (std::size_t index = 0U; index < ordered_candidates.size(); ++index) {
      if (index != 0U &&
          ordered_candidates[index - 1U].first ==
              ordered_candidates[index].first) {
        replay.binary64_distance_tie_observed = true;
      }
      exact_rank_by_canonical_id[ordered_candidates[index].second] =
          index + 1U;
    }

    const PointId source_id =
        source_point_id_by_canonical_id[source_canonical];
    const std::size_t begin =
        static_cast<std::size_t>(graph.offsets[source_id]);
    const std::size_t end =
        static_cast<std::size_t>(graph.offsets[source_id + 1U]);
    for (std::size_t position = begin; position < end; ++position) {
      const PointId target_source_id = graph.neighbors[position];
      if (target_source_id >= cloud.size()) {
        throw std::logic_error(
            "the bounded binary64 rank replay found an invalid CSR target");
      }
      const PointId target_canonical_id =
          canonical_point_id_by_source_id[target_source_id];
      const std::size_t exact_rank =
          exact_rank_by_canonical_id[target_canonical_id];
      const std::uint16_t cpu_rank =
          exact_rank <= rank_result.audit.maximum_rank
              ? static_cast<std::uint16_t>(exact_rank)
              : overflow_rank;
      replay.censored_rank_observed =
          replay.censored_rank_observed || cpu_rank == overflow_rank;
      ++replay.compared_arc_count;
      const std::uint16_t gpu_rank =
          rank_result.directed_csr_ranks[position];
      if (gpu_rank != cpu_rank) {
        ++replay.mismatch_count;
        if (!replay.first_mismatch_present) {
          replay.first_mismatch_present = true;
          replay.first_mismatch_source = source_id;
          replay.first_mismatch_target = target_source_id;
          replay.first_mismatch_gpu_rank = gpu_rank;
          replay.first_mismatch_cpu_rank = cpu_rank;
        }
      }
    }
  }
  replay.performed = true;
  replay.passed = replay.mismatch_count == 0U;
  return replay;
}

[[nodiscard]] GabrielNeighborRankRun run_gabriel_neighbor_rank_diagnostic(
    std::span<const Point3> points,
    const CsrGraph& graph,
    std::span<const ClassifiedTriangle> records,
    std::uint64_t expected_accepted_triangle_count,
    std::size_t maximum_rank,
    std::size_t seed_window_radius,
    std::size_t query_batch_size) {
  GabrielNeighborRankRun run;
  const auto canonicalization_begin = Clock::now();
  std::vector<morsehgp3d::exact::CertifiedPoint3> certified_points;
  certified_points.reserve(points.size());
  for (const Point3& point : points) {
    certified_points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64(
            point.x, point.y, point.z));
  }
  morsehgp3d::spatial::CanonicalPointCloud cloud =
      morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(
          certified_points);
  certified_points.clear();
  certified_points.shrink_to_fit();
  std::vector<PointId> source_point_id_by_canonical_id(points.size());
  std::vector<std::uint8_t> source_seen(points.size(), 0U);
  morsehgp3d::contract::CanonicalSha256Builder mapping_builder;
  mapping_builder.update(
      "MorseHGP3D/phase15/gabriel-neighbor-rank/"
      "canonical-to-source-v1/sha256/");
  sha256_word(mapping_builder, static_cast<std::uint64_t>(points.size()));
  for (std::size_t canonical_index = 0U;
       canonical_index < points.size();
       ++canonical_index) {
    const auto canonical_id =
        static_cast<morsehgp3d::spatial::PointId>(canonical_index);
    const std::size_t source_index = cloud.source_index(canonical_id);
    if (source_index >= points.size() || source_seen[source_index] != 0U) {
      throw std::runtime_error(
          "the neighbor-rank canonical-to-source map is not a permutation");
    }
    source_seen[source_index] = 1U;
    source_point_id_by_canonical_id[canonical_index] =
        static_cast<PointId>(source_index);
    sha256_word(mapping_builder, static_cast<std::uint64_t>(canonical_index));
    sha256_word(mapping_builder, static_cast<std::uint64_t>(source_index));
    for (const std::uint64_t bits :
         cloud.point(canonical_id).canonical_input_bits()) {
      sha256_word(mapping_builder, bits);
    }
  }
  run.canonical_to_source_mapping_sha256 =
      mapping_builder.finalize().to_lower_hex();
  run.canonicalization_nanoseconds =
      nanoseconds(Clock::now() - canonicalization_begin);

  const auto lbvh_begin = Clock::now();
  morsehgp3d::gpu::MortonLbvhBuildContext build_context(points.size());
  morsehgp3d::gpu::MortonLbvhDeviceBuildResult build_result =
      build_context.build(cloud);
  if (!build_result.complete_certified_build() ||
      !build_result.cuda_qualified_build()) {
    throw std::runtime_error(
        "the Gabriel neighbor-rank diagnostic requires a certified CUDA "
        "LBVH build");
  }
  morsehgp3d::gpu::MortonLbvhDeviceTraversalLease traversal_lease =
      build_context.release_device_traversal_lease(build_result);
  morsehgp3d::gpu::Binary64LbvhTopKContext rank_context(
      cloud, std::move(traversal_lease));
  run.lbvh_build_nanoseconds = nanoseconds(Clock::now() - lbvh_begin);

  const auto rank_begin = Clock::now();
  morsehgp3d::gpu::Binary64LbvhCsrNeighborRankResult rank_result =
      rank_context.query_csr_neighbor_ranks(
          cloud,
          graph.offsets,
          graph.neighbors,
          source_point_id_by_canonical_id,
          maximum_rank,
          seed_window_radius,
          query_batch_size);
  run.rank_query_nanoseconds = nanoseconds(Clock::now() - rank_begin);
  if (!rank_result.validated_complete_binary64_rank_prefix()) {
    throw std::runtime_error(
        "the Gabriel diagnostic received an incomplete neighbor-rank prefix");
  }
  run.rank_audit = rank_result.audit;
  run.bruteforce_replay = replay_neighbor_ranks_by_bounded_bruteforce(
      cloud, graph, source_point_id_by_canonical_id, rank_result);
  if (run.bruteforce_replay.performed &&
      !run.bruteforce_replay.passed) {
    throw std::runtime_error(
        "the bounded CPU replay contradicts the CUDA CSR neighbor ranks");
  }

  const auto reduction_begin = Clock::now();
  run.summary = reduce_gabriel_neighbor_ranks(
      graph,
      records,
      rank_result,
      expected_accepted_triangle_count);
  run.triangle_reduction_nanoseconds =
      nanoseconds(Clock::now() - reduction_begin);
  return run;
}

[[nodiscard]] ReconstructibleInputDigests make_reconstructible_input_digests(
    std::span<const Point3> points,
    std::span<const Edge> edges,
    std::uint64_t raw_wedge_count,
    std::uint64_t canonical_candidate_count) {
  using morsehgp3d::contract::CanonicalId;
  using morsehgp3d::contract::CanonicalSha256Builder;

  CanonicalSha256Builder point_builder;
  point_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/point-cloud/v1/sha256/");
  sha256_word(point_builder, static_cast<std::uint64_t>(points.size()));
  for (std::size_t index = 0U; index < points.size(); ++index) {
    sha256_word(point_builder, static_cast<std::uint64_t>(index));
    sha256_word(point_builder, std::bit_cast<std::uint64_t>(points[index].x));
    sha256_word(point_builder, std::bit_cast<std::uint64_t>(points[index].y));
    sha256_word(point_builder, std::bit_cast<std::uint64_t>(points[index].z));
  }
  const CanonicalId point_id = point_builder.finalize();

  CanonicalSha256Builder edge_builder;
  edge_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/delaunay-edges/v1/sha256/");
  sha256_word(edge_builder, static_cast<std::uint64_t>(edges.size()));
  for (const Edge& edge : edges) {
    sha256_word(edge_builder, edge.u);
    sha256_word(edge_builder, edge.v);
  }
  const CanonicalId edge_id = edge_builder.finalize();

  CanonicalSha256Builder wedge_builder;
  wedge_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/canonical-wedges/v1/sha256/");
  wedge_builder.update(std::span<const std::uint8_t>{point_id.bytes()});
  wedge_builder.update(std::span<const std::uint8_t>{edge_id.bytes()});
  wedge_builder.update("owns_triangle_smallest_Delaunay_center_v1");
  sha256_word(wedge_builder, raw_wedge_count);
  sha256_word(wedge_builder, canonical_candidate_count);
  const CanonicalId wedge_id = wedge_builder.finalize();

  return ReconstructibleInputDigests{
      point_id.to_lower_hex(),
      edge_id.to_lower_hex(),
      wedge_id.to_lower_hex()};
}

[[nodiscard]] K1Summary reduce_k1(
    std::span<const Point3> points,
    std::span<const Edge> edges) {
  std::vector<std::tuple<double, PointId, PointId>> weighted;
  weighted.reserve(edges.size());
  for (const Edge& edge : edges) {
    weighted.emplace_back(
        point_squared_distance(
            points[static_cast<std::size_t>(edge.u)],
            points[static_cast<std::size_t>(edge.v)]),
        edge.u,
        edge.v);
  }
  std::sort(weighted.begin(), weighted.end());
  DisjointSet components(points.size());
  K1Summary summary;
  summary.selected_edges.reserve(points.size() - 1U);
  for (const auto& [weight, u, v] : weighted) {
    if (components.unite(
            static_cast<std::size_t>(u), static_cast<std::size_t>(v))) {
      summary.selected_edges.emplace_back(weight, u, v);
      if (summary.selected_edges.size() == points.size() - 1U) {
        break;
      }
    }
  }
  if (summary.selected_edges.size() != points.size() - 1U ||
      components.component_count() != 1U) {
    throw std::runtime_error("the ordinary Delaunay graph did not yield a tree");
  }
  bool has_previous = false;
  std::uint64_t previous_bits{};
  summary.surrogate_compatible_digest = UINT64_C(1469598103934665603);
  digest_word(summary.surrogate_compatible_digest, UINT64_C(1));
  for (const auto& [weight, u, v] : summary.selected_edges) {
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(weight);
    if (!has_previous || bits != previous_bits) {
      ++summary.distinct_level_count;
      has_previous = true;
      previous_bits = bits;
    }
    digest_word(summary.surrogate_compatible_digest, bits);
    digest_word(summary.surrogate_compatible_digest, u);
    digest_word(summary.surrogate_compatible_digest, v);
  }
  summary.root_squared_distance = std::get<0>(summary.selected_edges.back());
  summary.root_squared_level = 0.25 * summary.root_squared_distance;
  return summary;
}

[[nodiscard]] Edge facet(PointId left, PointId right) noexcept {
  return left < right ? Edge{left, right} : Edge{right, left};
}

[[nodiscard]] K2Summary reduce_k2(
    std::span<const ClassifiedTriangle> records,
    bool gabriel_only,
    bool emit_necessary_records = false) {
  std::vector<const ClassifiedTriangle*> accepted;
  accepted.reserve(records.size());
  std::vector<Edge> facets;
  facets.reserve(checked_product(
      records.size(), 3U, "k=2 facet arena reserve overflows size_t"));
  for (const ClassifiedTriangle& triangle : records) {
    if (gabriel_only &&
        triangle.status != TriangleStatus::gabriel_binary64) {
      continue;
    }
    if (!gabriel_only &&
        triangle.status == TriangleStatus::degenerate_or_invalid) {
      continue;
    }
    accepted.push_back(&triangle);
    facets.push_back(facet(triangle.triangle.a, triangle.triangle.b));
    facets.push_back(facet(triangle.triangle.a, triangle.triangle.c));
    facets.push_back(facet(triangle.triangle.b, triangle.triangle.c));
  }
  K2Summary summary;
  summary.accepted_triangle_digest = UINT64_C(1469598103934665603);
  digest_word(summary.accepted_triangle_digest, UINT64_C(2));
  morsehgp3d::contract::CanonicalSha256Builder accepted_builder;
  accepted_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/accepted/v1/sha256/");
  sha256_word(accepted_builder, static_cast<std::uint64_t>(accepted.size()));
  morsehgp3d::contract::CanonicalSha256Builder necessary_builder;
  necessary_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/necessary/v1/sha256/");
  if (accepted.empty()) {
    summary.accepted_triangle_sha256 =
        accepted_builder.finalize().to_lower_hex();
    necessary_builder.update("/count/");
    sha256_word(necessary_builder, UINT64_C(0));
    summary.necessary_triangle_sha256 =
        necessary_builder.finalize().to_lower_hex();
    return summary;
  }
  for (std::size_t index = 1U; index < accepted.size(); ++index) {
    if (ClassifiedTriangleLess{}(*accepted[index], *accepted[index - 1U])) {
      throw std::logic_error("the k=2 input records are not globally sorted");
    }
  }
  std::sort(facets.begin(), facets.end(), edge_less);
  facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
  summary.facet_count = facets.size();
  DisjointSet components(facets.size());
  const auto facet_ids = [&](const Triangle& triangle) {
    const std::array<Edge, 3U> triangle_facets{
        facet(triangle.a, triangle.b),
        facet(triangle.a, triangle.c),
        facet(triangle.b, triangle.c)};
    std::array<std::size_t, 3U> ids{};
    for (std::size_t index = 0U; index < ids.size(); ++index) {
      const auto found = std::lower_bound(
          facets.begin(), facets.end(), triangle_facets[index], edge_less);
      if (found == facets.end() || !(*found == triangle_facets[index])) {
        throw std::logic_error("a retained triangle facet disappeared");
      }
      ids[index] = static_cast<std::size_t>(found - facets.begin());
    }
    return ids;
  };

  std::size_t plateau_begin = 0U;
  while (plateau_begin < accepted.size()) {
    const double plateau_level = accepted[plateau_begin]->squared_level;
    if (!std::isfinite(plateau_level) || plateau_level < 0.0) {
      throw std::logic_error("a retained k=2 record has an invalid level");
    }
    std::size_t plateau_end = plateau_begin + 1U;
    while (plateau_end < accepted.size() &&
           accepted[plateau_end]->squared_level == plateau_level) {
      ++plateau_end;
    }
    ++summary.distinct_level_count;
    std::vector<std::array<std::size_t, 3U>> plateau_facet_ids;
    plateau_facet_ids.reserve(plateau_end - plateau_begin);
    for (std::size_t index = plateau_begin; index < plateau_end; ++index) {
      const ClassifiedTriangle& record = *accepted[index];
      const Triangle triangle = record.triangle;
      const std::uint64_t bits =
          std::bit_cast<std::uint64_t>(record.squared_level);
      digest_word(summary.accepted_triangle_digest, bits);
      digest_word(summary.accepted_triangle_digest, triangle.a);
      digest_word(summary.accepted_triangle_digest, triangle.b);
      digest_word(summary.accepted_triangle_digest, triangle.c);
      sha256_triangle(accepted_builder, record);
      const std::array<std::size_t, 3U> ids = facet_ids(triangle);
      plateau_facet_ids.push_back(ids);

      if (gabriel_only) {
        if (record.support_cardinality == 2U) {
          ++summary.support_two_count;
        } else if (record.support_cardinality == 3U) {
          ++summary.support_three_count;
        } else {
          throw std::logic_error(
              "an accepted Gabriel triangle has invalid support cardinality");
        }
        const std::size_t root = components.find(ids[0]);
        const bool strictly_lower_connected =
            components.find(ids[1]) == root &&
            components.find(ids[2]) == root;
        if (strictly_lower_connected) {
          ++summary.strictly_lower_connected_triangle_count;
        } else {
          ++summary.necessary_triangle_count;
          sha256_triangle(necessary_builder, record);
          if (emit_necessary_records) {
            summary.emitted_necessary_records.push_back(record);
          }
        }
      }
    }

    // The plateau is deliberately inserted only after every strict-lower test.
    // Equal-level triangles can therefore never certify one another.
    for (const std::array<std::size_t, 3U>& ids : plateau_facet_ids) {
      for (std::size_t index = 1U; index < ids.size(); ++index) {
        if (components.unite(ids[0], ids[index])) {
          ++summary.useful_union_count;
        } else {
          ++summary.redundant_union_count;
        }
      }
    }
    plateau_begin = plateau_end;
  }
  if (gabriel_only &&
      summary.necessary_triangle_count +
              summary.strictly_lower_connected_triangle_count !=
          accepted.size()) {
    summary.coverage_violation_count = 1U;
  }
  summary.final_component_count = components.component_count();
  summary.first_squared_level = accepted.front()->squared_level;
  summary.root_squared_level = accepted.back()->squared_level;
  summary.accepted_triangle_sha256 =
      accepted_builder.finalize().to_lower_hex();
  necessary_builder.update("/count/");
  sha256_word(
      necessary_builder,
      static_cast<std::uint64_t>(summary.necessary_triangle_count));
  summary.necessary_triangle_sha256 =
      necessary_builder.finalize().to_lower_hex();
  return summary;
}

[[nodiscard]] K2Summary summarize_gabriel_source_inclusion(
    std::span<const ClassifiedTriangle> records,
    bool emit_records) {
  for (std::size_t index = 1U; index < records.size(); ++index) {
    if (ClassifiedTriangleLess{}(records[index], records[index - 1U])) {
      throw std::logic_error(
          "the Gabriel source-inclusion stream is not canonical");
    }
  }
  const std::size_t accepted_count = static_cast<std::size_t>(std::count_if(
      records.begin(),
      records.end(),
      [](const ClassifiedTriangle& record) {
        return record.status == TriangleStatus::gabriel_binary64;
      }));
  K2Summary summary;
  summary.accepted_triangle_digest = UINT64_C(1469598103934665603);
  digest_word(summary.accepted_triangle_digest, UINT64_C(2));
  morsehgp3d::contract::CanonicalSha256Builder accepted_builder;
  accepted_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/accepted/v1/sha256/");
  sha256_word(
      accepted_builder, static_cast<std::uint64_t>(accepted_count));
  morsehgp3d::contract::CanonicalSha256Builder necessary_builder;
  necessary_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/necessary/v1/sha256/");
  std::uint64_t previous_level_bits{};
  bool has_previous_level = false;
  for (const ClassifiedTriangle& record : records) {
    if (record.status != TriangleStatus::gabriel_binary64) {
      continue;
    }
    if (!std::isfinite(record.squared_level) || record.squared_level < 0.0) {
      throw std::logic_error(
          "a Gabriel source-inclusion record has an invalid level");
    }
    if (record.support_cardinality == 2U) {
      ++summary.support_two_count;
    } else if (record.support_cardinality == 3U) {
      ++summary.support_three_count;
    } else {
      throw std::logic_error(
          "a Gabriel source-inclusion record has an invalid support");
    }
    const std::uint64_t level_bits =
        std::bit_cast<std::uint64_t>(record.squared_level);
    if (!has_previous_level || level_bits != previous_level_bits) {
      ++summary.distinct_level_count;
      previous_level_bits = level_bits;
      has_previous_level = true;
    }
    digest_word(summary.accepted_triangle_digest, level_bits);
    digest_word(summary.accepted_triangle_digest, record.triangle.a);
    digest_word(summary.accepted_triangle_digest, record.triangle.b);
    digest_word(summary.accepted_triangle_digest, record.triangle.c);
    sha256_triangle(accepted_builder, record);
    sha256_triangle(necessary_builder, record);
    ++summary.necessary_triangle_count;
    if (emit_records) {
      summary.emitted_necessary_records.push_back(record);
    }
    if (summary.necessary_triangle_count == 1U) {
      summary.first_squared_level = record.squared_level;
    }
    summary.root_squared_level = record.squared_level;
  }
  if (summary.necessary_triangle_count != accepted_count ||
      summary.support_two_count + summary.support_three_count !=
          accepted_count) {
    summary.coverage_violation_count = 1U;
  }
  summary.accepted_triangle_sha256 =
      accepted_builder.finalize().to_lower_hex();
  necessary_builder.update("/count/");
  sha256_word(
      necessary_builder,
      static_cast<std::uint64_t>(summary.necessary_triangle_count));
  summary.necessary_triangle_sha256 =
      necessary_builder.finalize().to_lower_hex();
  return summary;
}

[[nodiscard]] GabrielSafetyBatchManifest make_gabriel_safety_batch_manifest(
    std::span<const ClassifiedTriangle> retained_records,
    const K2Summary& accepted_coverage) {
  using morsehgp3d::contract::CanonicalId;
  using morsehgp3d::contract::CanonicalSha256Builder;

  CanonicalSha256Builder ambiguous_builder;
  ambiguous_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/ambiguous-safety/v1/sha256/");
  std::size_t ambiguous_count{};
  ClassifiedTriangle first_ambiguous_triangle;
  bool first_ambiguous_triangle_present = false;
  for (const ClassifiedTriangle& record : retained_records) {
    if (record.status == TriangleStatus::ambiguous) {
      if (!first_ambiguous_triangle_present) {
        first_ambiguous_triangle = record;
        first_ambiguous_triangle_present = true;
      }
      sha256_triangle(ambiguous_builder, record);
      ++ambiguous_count;
    }
  }
  ambiguous_builder.update("/count/");
  sha256_word(ambiguous_builder, static_cast<std::uint64_t>(ambiguous_count));
  const CanonicalId ambiguous_id = ambiguous_builder.finalize();
  const CanonicalId necessary_id = CanonicalId::from_lower_hex(
      accepted_coverage.necessary_triangle_sha256);

  CanonicalSha256Builder composite_builder;
  composite_builder.update(
      "MorseHGP3D/phase15/gabriel-coverage/composite-safety-batch/v1/sha256/");
  composite_builder.update(
      std::span<const std::uint8_t>{necessary_id.bytes()});
  sha256_word(
      composite_builder,
      static_cast<std::uint64_t>(
          accepted_coverage.necessary_triangle_count));
  composite_builder.update(
      std::span<const std::uint8_t>{ambiguous_id.bytes()});
  sha256_word(composite_builder, static_cast<std::uint64_t>(ambiguous_count));
  const CanonicalId composite_id = composite_builder.finalize();

  if (ambiguous_count >
      std::numeric_limits<std::size_t>::max() -
          accepted_coverage.necessary_triangle_count) {
    throw std::length_error("Gabriel safety batch count overflows size_t");
  }
  return GabrielSafetyBatchManifest{
      ambiguous_count,
      accepted_coverage.necessary_triangle_count + ambiguous_count,
      ambiguous_id.to_lower_hex(),
      composite_id.to_lower_hex(),
      first_ambiguous_triangle,
      first_ambiguous_triangle_present};
}

void write_point_json(const Point3& point) {
  std::cout << '[' << std::setprecision(17) << point.x << ',' << point.y << ','
            << point.z << ']';
}

void write_edge_json(double squared_distance, PointId u, PointId v) {
  std::cout << "{\"u\":" << u << ",\"v\":" << v
            << ",\"squared_distance\":" << std::setprecision(17)
            << squared_distance << ",\"squared_level\":"
            << 0.25 * squared_distance << '}';
}

[[nodiscard]] const char* status_name(TriangleStatus status) noexcept {
  switch (status) {
    case TriangleStatus::blocked:
      return "blocked";
    case TriangleStatus::gabriel_binary64:
      return "gabriel_binary64";
    case TriangleStatus::ambiguous:
      return "ambiguous_requires_cpu_recertification";
    case TriangleStatus::degenerate_or_invalid:
      return "degenerate_or_invalid";
  }
  return "invalid_enum";
}

void write_triangle_json(const ClassifiedTriangle& record) {
  std::cout << "{\"a\":" << record.triangle.a
            << ",\"b\":" << record.triangle.b
            << ",\"c\":" << record.triangle.c
            << ",\"squared_level\":" << std::setprecision(17)
            << record.squared_level << ",\"status\":\""
            << status_name(record.status) << "\",\"support_cardinality\":"
            << record.support_cardinality << '}';
}

void write_surrogate_failure_json(
    const SurrogateFailureWitness& witness) {
  if (!witness.present) {
    std::cout << "null";
    return;
  }
  std::cout << "{\"classification\":" << std::quoted(witness.classification)
            << ",\"source\":";
  write_triangle_json(witness.source);
  std::cout << ",\"observed_first_connection\":";
  if (witness.observed_connection_level_present) {
    std::cout
        << "{\"raw_squared_weight\":" << std::setprecision(17)
        << witness.observed_connection_raw_squared_weight
        << ",\"level_conversion\":\"exact_dyadic_divide_by_4\"}"
        << ",\"strictly_after_source_level\":true";
  } else {
    std::cout << "null";
  }
  std::cout << '}';
}

[[nodiscard]] std::uint64_t histogram_captured_through(
    const NeighborRankHistogram& histogram,
    std::size_t rank) {
  const std::size_t end = std::min(
      rank,
      histogram.exact_rank_counts.empty()
          ? std::size_t{0}
          : histogram.exact_rank_counts.size() - 1U);
  std::uint64_t cumulative{};
  for (std::size_t current = 1U; current <= end; ++current) {
    cumulative = checked_add_u64(
        cumulative,
        histogram.exact_rank_counts[current],
        "the neighbor-rank cumulative histogram overflows uint64");
  }
  return cumulative;
}

[[nodiscard]] std::optional<std::size_t> histogram_quantile(
    const NeighborRankHistogram& histogram,
    std::uint64_t total_count,
    std::size_t percent) {
  if (total_count == 0U || percent == 0U || percent > 100U) {
    return std::nullopt;
  }
  const std::uint64_t quotient = total_count / UINT64_C(100);
  const std::uint64_t remainder = total_count % UINT64_C(100);
  const std::uint64_t remainder_product =
      remainder * static_cast<std::uint64_t>(percent);
  const std::uint64_t target = checked_add_u64(
      quotient * static_cast<std::uint64_t>(percent),
      (remainder_product + UINT64_C(99)) / UINT64_C(100),
      "the neighbor-rank quantile target overflows uint64");
  std::uint64_t cumulative{};
  for (std::size_t rank = 1U;
       rank < histogram.exact_rank_counts.size();
       ++rank) {
    cumulative = checked_add_u64(
        cumulative,
        histogram.exact_rank_counts[rank],
        "the neighbor-rank quantile cumulative count overflows uint64");
    if (cumulative >= target) {
      return rank;
    }
  }
  return std::nullopt;
}

void write_optional_rank_json(const std::optional<std::size_t>& rank) {
  if (rank.has_value()) {
    std::cout << *rank;
  } else {
    std::cout << "null";
  }
}

void write_neighbor_rank_histogram_json(
    const NeighborRankHistogram& histogram,
    std::uint64_t total_count) {
  std::cout << "{\"exact_rank_counts_1_through_M\":[";
  for (std::size_t rank = 1U;
       rank < histogram.exact_rank_counts.size();
       ++rank) {
    if (rank != 1U) {
      std::cout << ',';
    }
    std::cout << histogram.exact_rank_counts[rank];
  }
  std::cout << "]"
            << ",\"captured_count\":" << histogram.captured_count
            << ",\"overflow_count\":" << histogram.overflow_count
            << ",\"maximum_observed_rank\":";
  if (histogram.overflow_count == 0U && total_count != 0U) {
    std::cout << histogram.maximum_exact_rank;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"p50_smallest_M\":";
  write_optional_rank_json(histogram_quantile(histogram, total_count, 50U));
  std::cout << ",\"p90_smallest_M\":";
  write_optional_rank_json(histogram_quantile(histogram, total_count, 90U));
  std::cout << ",\"p95_smallest_M\":";
  write_optional_rank_json(histogram_quantile(histogram, total_count, 95U));
  std::cout << ",\"p99_smallest_M\":";
  write_optional_rank_json(histogram_quantile(histogram, total_count, 99U));
  std::cout << '}';
}

void write_gabriel_neighbor_rank_json(
    const GabrielNeighborRankRun& run,
    std::size_t point_count) {
  const GabrielNeighborRankSummary& summary = run.summary;
  const morsehgp3d::gpu::Binary64LbvhCsrNeighborRankAudit& audit =
      run.rank_audit;
  const bool compact_rank_gate_passed =
      audit.complete_query_coverage && audit.failed_query_count == 0U &&
      audit.no_search_work_cap && audit.exact_binary64_prefix_complete &&
      audit.source_mapping_permutation_validated &&
      audit.csr_structure_validated && audit.host_rank_transcript_validated;
  const bool all_variants_have_pdel_witness_root =
      summary.directed_root_star.missing_witness_triangle_count == 0U &&
      summary.symmetric_union_star.missing_witness_triangle_count == 0U &&
      summary.mutual_star.missing_witness_triangle_count == 0U;
  std::cout
      << "{\"schema\":\"morsehgp3d.phase15_gabriel_neighbor_rank.v1\""
      << ",\"status\":\"offline_proposal_diagnostic_not_a_proof\""
      << ",\"scientific_scope\":\"PDEL_witness_root_neighbor_wedge_thresholds_on_the_accepted_Gabriel_set_not_a_Gabriel_or_Gamma2_catalogue\""
      << ",\"distance_key\":\"fixed_binary64_squared_distance_then_canonical_PointId\""
      << ",\"canonical_to_source_mapping_semantics\":\"canonical_PointId_remains_the_tie_key_source_PointId_only_addresses_the_PDEL_CSR\""
      << ",\"rank_semantics\":\"exact_prefix_for_the_declared_binary64_key\""
      << ",\"rank_overflow_semantics\":\"exact_rank_strictly_greater_than_M_value_censored\""
      << ",\"maximum_rank_M\":" << summary.maximum_rank
      << ",\"one_complete_LBVH_traversal_per_source_not_per_arc\":true"
      << ",\"two_incident_edges_form_one_wedge_proposal\":true"
      << ",\"support_two_PDEL_witnesses_allowed\":true"
      << ",\"root_scope\":\"minimum_over_available_PDEL_witness_roots\""
      << ",\"partial_root_threshold_relation\":\"safe_upper_bound_on_the_minimum_over_all_three_roots\""
      << ",\"absolute_minimum_over_all_three_roots_claimed\":false"
      << ",\"Gabriel_catalogue_claimed\":false"
      << ",\"exact_Gamma2_claimed\":false"
      << ",\"CSR_row_writes_race_free_by_source_permutation\":true"
      << ",\"maximum_logical_heap_storage_bytes_per_query\":4096"
      << ",\"CUDA_local_memory_spill_and_throughput_require_G4_qualification\":true"
      << ",\"query_cost_model\":\"one_branch_and_bound_search_per_source_with_log_M_heap_updates_not_one_search_per_arc\""
      << ",\"n_by_M_table_materialized\":false"
      << ",\"additional_global_triangle_table_materialized\":false"
      << ",\"directed_CSR_rank_bytes_per_arc\":2"
      << ",\"compact_rank_gate_passed\":"
      << (compact_rank_gate_passed ? "true" : "false")
      << ",\"all_variants_have_PDEL_witness_root\":"
      << (all_variants_have_pdel_witness_root ? "true" : "false")
      << ",\"accepted_triangle_count\":"
      << summary.accepted_triangle_count
      << ",\"complete_three_pair_triangle_count\":"
      << summary.complete_three_pair_triangle_count
      << ",\"partial_two_pair_witness_triangle_count\":"
      << summary.partial_two_pair_witness_triangle_count
      << ",\"insufficient_pair_witness_triangle_count\":"
      << summary.insufficient_pair_witness_triangle_count;
  const auto write_variant = [&](std::string_view definition,
                                 const NeighborRankVariantSummary& variant) {
    std::cout
        << "{\"definition\":" << std::quoted(definition)
        << ",\"threshold_scope\":\"minimum_over_available_PDEL_witness_roots\""
        << ",\"witness_triangle_count\":"
        << variant.witness_triangle_count
        << ",\"missing_witness_triangle_count\":"
        << variant.missing_witness_triangle_count
        << ",\"considered_root_count\":" << variant.considered_root_count
        << ",\"first_missing_witness_triangle\":";
    if (variant.first_missing_witness_triangle_present) {
      write_triangle_json(variant.first_missing_witness_triangle);
    } else {
      std::cout << "null";
    }
    std::cout << ",\"histogram\":";
    write_neighbor_rank_histogram_json(
        variant.histogram, variant.witness_triangle_count);
    std::cout << '}';
  };
  std::cout << ",\"variants\":{\"directed_root_star\":";
  write_variant(
      "min_over_available_roots_max_of_two_outgoing_neighbor_ranks",
      summary.directed_root_star);
  std::cout << ",\"symmetric_union_star\":";
  write_variant(
      "min_over_available_roots_max_of_two_incident_min_direction_ranks",
      summary.symmetric_union_star);
  std::cout << ",\"mutual_star\":";
  write_variant(
      "min_over_available_roots_max_of_two_incident_max_direction_ranks",
      summary.mutual_star);
  std::cout << "}"
            << ",\"bounded_bruteforce_rank_replay_point_cap\":"
            << NeighborRankBruteforceReplay::maximum_point_count
            << ",\"bounded_bruteforce_rank_replay_performed\":"
            << (run.bruteforce_replay.performed ? "true" : "false")
            << ",\"bounded_bruteforce_rank_replay_passed\":"
            << (run.bruteforce_replay.passed ? "true" : "false")
            << ",\"bounded_bruteforce_rank_replay\":{"
            << "\"point_count\":" << run.bruteforce_replay.point_count
            << ",\"compared_arc_count\":"
            << run.bruteforce_replay.compared_arc_count
            << ",\"mismatch_count\":"
            << run.bruteforce_replay.mismatch_count
            << ",\"canonical_source_mapping_non_identity_observed\":"
            << (run.bruteforce_replay
                        .canonical_source_mapping_non_identity_observed
                    ? "true"
                    : "false")
            << ",\"binary64_distance_tie_observed\":"
            << (run.bruteforce_replay.binary64_distance_tie_observed
                    ? "true"
                    : "false")
            << ",\"censored_rank_observed\":"
            << (run.bruteforce_replay.censored_rank_observed
                    ? "true"
                    : "false")
            << ",\"first_mismatch\":";
  if (run.bruteforce_replay.first_mismatch_present) {
    std::cout << "{\"source\":"
              << run.bruteforce_replay.first_mismatch_source
              << ",\"target\":"
              << run.bruteforce_replay.first_mismatch_target
              << ",\"gpu_rank\":"
              << run.bruteforce_replay.first_mismatch_gpu_rank
              << ",\"cpu_rank\":"
              << run.bruteforce_replay.first_mismatch_cpu_rank << '}';
  } else {
    std::cout << "null";
  }
  std::cout << "}"
            << ",\"proposal_policy_K_log_n\":{"
            << "\"K\":2,\"natural_log\":true,"
            << "\"proposal_only_not_a_completeness_proof\":true,"
            << "\"samples\":[";
  constexpr std::array<std::size_t, 4U> kPolicyMultipliers{1U, 2U, 4U, 8U};
  for (std::size_t index = 0U; index < kPolicyMultipliers.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const std::size_t multiplier = kPolicyMultipliers[index];
    const double raw_rank =
        static_cast<double>(2U * multiplier) *
        std::log(static_cast<double>(point_count));
    const std::size_t policy_rank = std::max(
        std::size_t{2}, static_cast<std::size_t>(std::ceil(raw_rank)));
    std::cout << "{\"c\":" << multiplier << ",\"M\":" << policy_rank
              << ",\"within_measured_cap\":"
              << (policy_rank <= summary.maximum_rank ? "true" : "false")
              << ",\"directed_root_star_captured_count\":";
    if (policy_rank <= summary.maximum_rank) {
      std::cout << histogram_captured_through(
          summary.directed_root_star.histogram, policy_rank);
    } else {
      std::cout << "null";
    }
    std::cout << ",\"symmetric_union_star_captured_count\":";
    if (policy_rank <= summary.maximum_rank) {
      std::cout << histogram_captured_through(
          summary.symmetric_union_star.histogram, policy_rank);
    } else {
      std::cout << "null";
    }
    std::cout << ",\"mutual_star_captured_count\":";
    if (policy_rank <= summary.maximum_rank) {
      std::cout << histogram_captured_through(
          summary.mutual_star.histogram, policy_rank);
    } else {
      std::cout << "null";
    }
    std::cout << '}';
  }
  std::cout
      << "]}"
      << ",\"lbvh_audit\":{\"point_count\":" << audit.point_count
      << ",\"certified_node_count\":" << audit.certified_node_count
      << ",\"directed_CSR_arc_count\":" << audit.directed_csr_arc_count
      << ",\"maximum_rank\":" << audit.maximum_rank
      << ",\"overflow_rank\":" << audit.overflow_rank
      << ",\"ranked_arc_count\":" << audit.ranked_arc_count
      << ",\"overflow_arc_count\":" << audit.overflow_arc_count
      << ",\"seed_window_radius\":" << audit.seed_window_radius
      << ",\"query_batch_size\":" << audit.query_batch_size
      << ",\"query_batch_count\":" << audit.query_batch_count
      << ",\"completed_query_count\":" << audit.completed_query_count
      << ",\"failed_query_count\":" << audit.failed_query_count
      << ",\"node_visit_count\":" << audit.node_visit_count
      << ",\"maximum_node_visit_count_per_query\":"
      << audit.maximum_node_visit_count_per_query
      << ",\"full_tree_query_count\":" << audit.full_tree_query_count
      << ",\"strict_AABB_prune_count\":"
      << audit.strict_aabb_prune_count
      << ",\"AABB_equality_descent_count\":"
      << audit.aabb_equality_descent_count
      << ",\"invalid_AABB_bound_descent_count\":"
      << audit.invalid_aabb_bound_descent_count
      << ",\"seed_covered_subtree_skip_count\":"
      << audit.seed_covered_subtree_skip_count
      << ",\"seed_distance_evaluation_count\":"
      << audit.seed_distance_evaluation_count
      << ",\"traversal_leaf_distance_evaluation_count\":"
      << audit.traversal_leaf_distance_evaluation_count
      << ",\"persistent_input_device_bytes\":"
      << audit.persistent_input_device_byte_capacity
      << ",\"transient_CSR_device_bytes\":"
      << audit.transient_csr_device_byte_capacity
      << ",\"maximum_batch_audit_device_bytes\":"
      << audit.maximum_batch_audit_device_byte_capacity
      << ",\"kernel_block_count_maximum\":"
      << audit.kernel_block_count_maximum
      << ",\"kernel_thread_count_maximum\":"
      << audit.kernel_thread_count_maximum
      << ",\"kernel_local_size_bytes_per_thread\":"
      << audit.kernel_local_size_bytes_per_thread
      << ",\"source_snapshot_epoch\":" << audit.source_snapshot_epoch
      << ",\"fixed_round_to_nearest_distance_recipe_requested\":"
      << (audit.fixed_round_to_nearest_distance_recipe_requested
              ? "true"
              : "false")
      << ",\"directed_round_down_AABB_recipe_requested\":"
      << (audit.directed_round_down_aabb_recipe_requested ? "true" : "false")
      << ",\"strict_prune_requested\":"
      << (audit.strict_prune_requested ? "true" : "false")
      << ",\"equality_descends_requested\":"
      << (audit.equality_descends_requested ? "true" : "false")
      << ",\"stackless_postorder_traversal_requested\":"
      << (audit.stackless_postorder_traversal_requested ? "true" : "false")
      << ",\"complete_query_coverage\":"
      << (audit.complete_query_coverage ? "true" : "false")
      << ",\"no_search_work_cap\":"
      << (audit.no_search_work_cap ? "true" : "false")
      << ",\"exact_binary64_prefix_complete\":"
      << (audit.exact_binary64_prefix_complete ? "true" : "false")
      << ",\"ranks_above_maximum_censored\":"
      << (audit.ranks_above_maximum_censored ? "true" : "false")
      << ",\"source_mapping_permutation_validated\":"
      << (audit.source_mapping_permutation_validated ? "true" : "false")
      << ",\"csr_structure_validated\":"
      << (audit.csr_structure_validated ? "true" : "false")
      << ",\"host_rank_transcript_validated\":"
      << (audit.host_rank_transcript_validated ? "true" : "false")
      << ",\"n_by_maximum_rank_table_materialized\":"
      << (audit.n_by_maximum_rank_table_materialized ? "true" : "false")
      << ",\"higher_order_delaunay_mosaic_materialized\":"
      << (audit.higher_order_delaunay_mosaic_materialized ? "true" : "false")
      << ",\"global_pair_matrix_materialized\":"
      << (audit.global_pair_matrix_materialized ? "true" : "false")
      << ",\"exact_morse_hierarchy_claimed\":"
      << (audit.exact_morse_hierarchy_claimed ? "true" : "false")
      << ",\"public_status_claimed\":"
      << (audit.public_status_claimed ? "true" : "false")
      << ",\"canonical_to_source_mapping_sha256\":\""
      << run.canonical_to_source_mapping_sha256 << "\"}"
      << ",\"timings_nanoseconds\":{\"canonicalization\":"
      << run.canonicalization_nanoseconds
      << ",\"lbvh_build\":" << run.lbvh_build_nanoseconds
      << ",\"rank_all_sources\":" << run.rank_query_nanoseconds
      << ",\"rank_kernel\":" << audit.kernel_nanoseconds
      << ",\"rank_device_to_host\":" << audit.device_to_host_nanoseconds
      << ",\"triangle_histogram_reduction\":"
      << run.triangle_reduction_nanoseconds << "}}";
}

[[nodiscard]] std::string direct_variant_decision_sha256(
    std::string_view variant,
    const K2Summary& coverage,
    const GabrielSafetyBatchManifest& safety_batch) {
  using morsehgp3d::contract::CanonicalId;
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(
      "MorseHGP3D/phase15/gabriel-fusion-guardrail/"
      "direct-source-inclusion-deadline-upper-bound-v1/sha256/");
  builder.update(variant);
  const CanonicalId accepted =
      CanonicalId::from_lower_hex(coverage.accepted_triangle_sha256);
  const CanonicalId ambiguous =
      CanonicalId::from_lower_hex(safety_batch.ambiguous_triangle_sha256);
  builder.update(std::span<const std::uint8_t>{accepted.bytes()});
  builder.update(std::span<const std::uint8_t>{ambiguous.bytes()});
  sha256_word(
      builder,
      static_cast<std::uint64_t>(
          coverage.necessary_triangle_count +
          coverage.strictly_lower_connected_triangle_count));
  sha256_word(
      builder,
      static_cast<std::uint64_t>(
          safety_batch.ambiguous_triangle_count));
  return builder.finalize().to_lower_hex();
}

void write_variant_guardrails_json(
    const SurrogateGuardrailRun& run,
    const K2Summary& coverage,
    const GabrielSafetyBatchManifest& safety_batch,
    std::string_view point_cloud_sha256,
    bool emit_records) {
  const std::size_t direct_source_count =
      coverage.necessary_triangle_count +
      coverage.strictly_lower_connected_triangle_count +
      safety_batch.ambiguous_triangle_count;
  const bool raw_surrogate_regular_guardrail_passed =
      std::all_of(
          run.orders.begin(),
          run.orders.end(),
          [](const SurrogateGuardrailOrderSummary& summary) {
            return summary.late_count == 0U && summary.never_count == 0U;
          });
  const bool corrected_surrogate_regular_guardrail_passed =
      std::all_of(
          run.orders.begin(),
          run.orders.end(),
          [](const SurrogateGuardrailOrderSummary& summary) {
            return summary.corrected_late_count == 0U &&
                   summary.corrected_never_count == 0U &&
                   summary.corrected_tree_edge_count ==
                       summary.tree_edge_count;
          });
  const bool corrected_surrogate_fail_closed_guardrail_passed =
      corrected_surrogate_regular_guardrail_passed &&
      safety_batch.ambiguous_triangle_count == 0U &&
      std::all_of(
          run.orders.begin(),
          run.orders.end(),
          [](const SurrogateGuardrailOrderSummary& summary) {
            return summary.corrected_unsupported_count == 0U;
          });
  SurrogateFailureWitness direct_unsupported_witness;
  if (safety_batch.first_ambiguous_triangle_present) {
    direct_unsupported_witness.source =
        safety_batch.first_ambiguous_triangle;
    direct_unsupported_witness.classification = "unsupported";
    direct_unsupported_witness.present = true;
  }
  std::cout
      << "{\"schema\":\"morsehgp3d.phase15_gabriel_fusion_guardrails.v1\""
      << ",\"criterion\":\"gabriel_fusion_deadline_v1\""
      << ",\"boundary\":\"closed_after_complete_candidate_plateau\""
      << ",\"early_connections_accepted\":true"
      << ",\"scientific_scope\":\"conditional_binary64_guardrail_not_Gamma2_exactness\""
      << ",\"raw_surrogate_regular_guardrail_passed\":"
      << (raw_surrogate_regular_guardrail_passed ? "true" : "false")
      << ",\"corrected_surrogate_regular_guardrail_passed\":"
      << (corrected_surrogate_regular_guardrail_passed ? "true" : "false")
      << ",\"corrected_surrogate_fail_closed_guardrail_passed\":"
      << (corrected_surrogate_fail_closed_guardrail_passed
              ? "true"
              : "false")
      << ",\"source\":{\"accepted_gabriel_binary64_count\":"
      << coverage.necessary_triangle_count +
             coverage.strictly_lower_connected_triangle_count
      << ",\"unsupported_ambiguous_count\":"
      << safety_batch.ambiguous_triangle_count
      << ",\"source_triangle_count\":" << direct_source_count
      << ",\"level_convention\":\"binary64_squared_Gabriel_radius_recipe\""
      << ",\"point_cloud_sha256\":" << std::quoted(std::string{point_cloud_sha256})
      << ",\"accepted_source_sha256\":\""
      << coverage.accepted_triangle_sha256 << "\"}"
      << ",\"direct_facet_variants\":[";
  constexpr std::array<std::string_view, 5U> kDirectVariants{
      "two_edge",
      "closed_star",
      "square_clique",
      "link_face_fan",
      "one_edge"};
  for (std::size_t index = 0U; index < kDirectVariants.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const std::string_view variant = kDirectVariants[index];
    const bool fail_closed = safety_batch.ambiguous_triangle_count == 0U;
    std::cout
        << "{\"variant\":" << std::quoted(std::string{variant})
        << ",\"representation\":\"k2_pair_facet_relation_stream\""
        << ",\"certificate_kind\":\"source_event_inclusion_deadline_upper_bound_v1\""
        << ",\"full_variant_first_connection_replayed\":false"
        << ",\"source_inclusion_basis\":\"two_Delaunay_edge_source_and_declared_variant_superset_v1\""
        << ",\"source_triangle_included_by_definition\":true"
        << ",\"counts\":{\"source_triangle_count\":"
        << direct_source_count
        << ",\"accepted_source_triangle_count\":"
        << coverage.necessary_triangle_count +
               coverage.strictly_lower_connected_triangle_count
        << ",\"guaranteed_no_later_than_deadline_count\":"
        << coverage.necessary_triangle_count +
               coverage.strictly_lower_connected_triangle_count
        << ",\"violation_count\":0,\"unsupported_count\":"
        << safety_batch.ambiguous_triangle_count << '}'
        << ",\"deadline_upper_bound_partition_closed\":true"
        << ",\"regular_binary64_guardrail_passed\":true"
        << ",\"fail_closed_guardrail_passed\":"
        << (fail_closed ? "true" : "false")
        << ",\"decision_sha256\":\""
        << direct_variant_decision_sha256(
               variant, coverage, safety_batch)
        << "\""
        << ",\"first_failure_witness\":";
    write_surrogate_failure_json(direct_unsupported_witness);
    std::cout << ",\"first_unsupported_witness\":";
    write_surrogate_failure_json(direct_unsupported_witness);
    std::cout << '}';
  }
  std::cout
      << "]"
      << ",\"surrogate\":{"
         "\"representation\":\"point_weighted_spanning_tree_binary64_v1\","
         "\"adapter\":{"
         "\"adapter_id\":\"point_component_clique_lift_v1\","
         "\"component_lift\":\"one_lifted_component_per_point_component_with_all_internal_pair_facets\","
         "\"input_level_convention\":\"binary64_mutual_reachability_squared_distance_recipe\","
         "\"output_level_convention\":\"binary64_squared_Gabriel_radius_recipe\","
         "\"level_conversion\":\"exact_dyadic_divide_by_4\","
         "\"exact_dyadic_comparison_of_binary64_recipe_outputs\":true,"
         "\"native_k2_facet_domain\":false,"
         "\"Gamma2_exactness_claimed\":false},"
         "\"canonical_to_source_mapping_sha256\":\""
      << run.canonical_to_source_mapping_sha256
      << "\",\"remapped_top_k_transcript_sha256\":\""
      << run.remapped_top_k_transcript_sha256 << "\""
      << ",\"top_k_audit\":{\"backend\":\"cuda_binary64_lbvh_top_k\""
      << ",\"maximum_order\":" << run.top_k_audit.maximum_order
      << ",\"seed_window_radius\":"
      << run.top_k_audit.seed_window_radius
      << ",\"neighbor_record_count\":"
      << run.top_k_audit.neighbor_record_count
      << ",\"point_count\":" << run.top_k_audit.point_count
      << ",\"certified_node_count\":"
      << run.top_k_audit.certified_node_count
      << ",\"completed_query_count\":"
      << run.top_k_audit.completed_query_count
      << ",\"failed_query_count\":"
      << run.top_k_audit.failed_query_count
      << ",\"source_snapshot_epoch\":"
      << run.top_k_audit.source_snapshot_epoch
      << ",\"traversal_lease_adopted\":"
      << (run.top_k_audit.traversal_lease_adopted ? "true" : "false")
      << ",\"invalid_aabb_bound_descent_count\":"
      << run.top_k_audit.invalid_aabb_bound_descent_count
      << ",\"complete_query_coverage\":"
      << (run.top_k_audit.complete_query_coverage ? "true" : "false")
      << ",\"no_candidate_truncation\":"
      << (run.top_k_audit.no_candidate_truncation ? "true" : "false")
      << ",\"source_morton_permutation_validated\":"
      << (run.top_k_audit.source_morton_permutation_validated
              ? "true"
              : "false")
      << ",\"host_transcript_structure_validated\":"
      << (run.top_k_audit.host_transcript_structure_validated
              ? "true"
              : "false")
      << ",\"fixed_round_to_nearest_distance_recipe_requested\":"
      << (run.top_k_audit.fixed_round_to_nearest_distance_recipe_requested
              ? "true"
              : "false")
      << ",\"directed_round_down_aabb_recipe_requested\":"
      << (run.top_k_audit.directed_round_down_aabb_recipe_requested
              ? "true"
              : "false")
      << ",\"strict_prune_requested\":"
      << (run.top_k_audit.strict_prune_requested ? "true" : "false")
      << ",\"stackless_postorder_traversal_requested\":"
      << (run.top_k_audit.stackless_postorder_traversal_requested
              ? "true"
              : "false")
      << ",\"persistent_input_device_byte_capacity\":"
      << run.top_k_audit.persistent_input_device_byte_capacity
      << ",\"transient_output_device_byte_capacity\":"
      << run.top_k_audit.transient_output_device_byte_capacity
      << ",\"higher_order_delaunay_mosaic_materialized\":"
      << (run.top_k_audit.higher_order_delaunay_mosaic_materialized
              ? "true"
              : "false")
      << ",\"global_pair_matrix_materialized\":"
      << (run.top_k_audit.global_pair_matrix_materialized
              ? "true"
              : "false")
      << ",\"exact_morse_hierarchy_claimed\":"
      << (run.top_k_audit.exact_morse_hierarchy_claimed ? "true" : "false")
      << ",\"public_status_claimed\":"
      << (run.top_k_audit.public_status_claimed ? "true" : "false")
      << ",\"kernel_nanoseconds\":"
      << run.top_k_audit.kernel_nanoseconds
      << ",\"launcher_wall_nanoseconds\":"
      << run.top_k_audit.launcher_wall_nanoseconds
      << ",\"device_name\":" << std::quoted(run.top_k_audit.device_name)
      << '}'
      << ",\"orders\":[";
  for (std::size_t order_index = 0U;
       order_index < run.orders.size();
       ++order_index) {
    if (order_index != 0U) {
      std::cout << ',';
    }
    const SurrogateGuardrailOrderSummary& summary =
        run.orders[order_index];
    const std::size_t satisfied =
        summary.connected_before_count + summary.connected_at_count;
    const std::size_t violation =
        summary.late_count + summary.never_count + summary.unsupported_count;
    const bool regular_passed =
        summary.late_count == 0U && summary.never_count == 0U;
    const bool fail_closed_passed =
        regular_passed && summary.unsupported_count == 0U;
    std::cout
        << "{\"order\":" << summary.order
        << ",\"tree\":{\"proposed_edge_count\":"
        << summary.proposed_edge_count
        << ",\"unique_edge_count\":" << summary.unique_edge_count
        << ",\"selected_tree_edge_count\":" << summary.tree_edge_count
        << ",\"final_component_count\":1"
        << ",\"distinct_level_count\":"
        << summary.distinct_tree_level_count
        << ",\"root_raw_squared_weight\":" << std::setprecision(17)
        << summary.root_squared_weight
        << ",\"tree_sha256\":\"" << summary.tree_sha256 << "\""
        << ",\"surrogate_compatible_digest\":\""
        << summary.surrogate_compatible_digest << "\""
        << ",\"tree_edges\":";
    if (emit_records) {
      std::cout << '[';
      for (std::size_t edge_index = 0U;
           edge_index < summary.emitted_tree_edges.size();
           ++edge_index) {
        if (edge_index != 0U) {
          std::cout << ',';
        }
        const SurrogateTreeEdge& edge =
            summary.emitted_tree_edges[edge_index];
        std::cout << "{\"u\":" << edge.u << ",\"v\":" << edge.v
                  << ",\"raw_squared_weight\":"
                  << std::setprecision(17) << edge.squared_weight << '}';
      }
      std::cout << ']';
    } else {
      std::cout << "null";
    }
    std::cout
        << '}'
        << ",\"counts\":{\"source_triangle_count\":"
        << summary.source_triangle_count
        << ",\"supported_triangle_count\":"
        << summary.supported_triangle_count
        << ",\"connected_before_count\":"
        << summary.connected_before_count
        << ",\"connected_at_count\":" << summary.connected_at_count
        << ",\"late_count\":" << summary.late_count
        << ",\"never_count\":" << summary.never_count
        << ",\"unsupported_count\":" << summary.unsupported_count
        << ",\"satisfied_count\":" << satisfied
        << ",\"violation_count\":" << violation << '}'
        << ",\"classification_partition_closed\":"
        << ((satisfied + violation == summary.source_triangle_count)
                ? "true"
                : "false")
        << ",\"regular_binary64_guardrail_passed\":"
        << (regular_passed ? "true" : "false")
        << ",\"fail_closed_guardrail_passed\":"
        << (fail_closed_passed ? "true" : "false")
        << ",\"decision_sha256\":\"" << summary.decision_sha256 << "\""
        << ",\"first_failure_witness\":";
    write_surrogate_failure_json(summary.first_failure);
    std::cout << ",\"first_late_witness\":";
    write_surrogate_failure_json(summary.first_late);
    std::cout << ",\"first_unsupported_witness\":";
    write_surrogate_failure_json(summary.first_unsupported);
    std::cout
        << ",\"correction\":{"
           "\"algorithm\":\"canonical_greedy_closed_plateau_Gabriel_triangle_overlay_v1\""
        << ",\"correction_triangle_count\":"
        << summary.correction_triangle_count
        << ",\"useful_union_count\":"
        << summary.useful_correction_union_count
        << ",\"corrected_postcondition_violation_count\":"
        << summary.corrected_postcondition_violation_count
        << ",\"regular_binary64_guardrail_passed\":"
        << (summary.corrected_late_count == 0U &&
                    summary.corrected_never_count == 0U
                ? "true"
                : "false")
        << ",\"fail_closed_guardrail_passed\":"
        << (summary.corrected_late_count == 0U &&
                    summary.corrected_never_count == 0U &&
                    summary.corrected_unsupported_count == 0U
                ? "true"
                : "false")
        << ",\"overlay_sha256\":\"" << summary.correction_sha256 << "\""
        << ",\"corrected_decision_sha256\":\""
        << summary.corrected_decision_sha256 << "\""
        << ",\"counts\":{\"source_triangle_count\":"
        << summary.source_triangle_count
        << ",\"connected_before_count\":"
        << summary.corrected_connected_before_count
        << ",\"connected_at_count\":"
        << summary.corrected_connected_at_count
        << ",\"late_count\":" << summary.corrected_late_count
        << ",\"never_count\":" << summary.corrected_never_count
        << ",\"unsupported_count\":"
        << summary.corrected_unsupported_count
        << ",\"satisfied_count\":"
        << summary.corrected_connected_before_count +
               summary.corrected_connected_at_count
        << ",\"violation_count\":"
        << summary.corrected_late_count + summary.corrected_never_count +
               summary.corrected_unsupported_count
        << '}'
        << ",\"classification_partition_closed\":"
        << (summary.corrected_connected_before_count +
                        summary.corrected_connected_at_count +
                        summary.corrected_late_count +
                        summary.corrected_never_count +
                        summary.corrected_unsupported_count ==
                    summary.source_triangle_count
                ? "true"
                : "false")
        << ",\"correction_applied_to_transient_verified_tree\":true"
        << ",\"corrected_tree_records_serialized\":"
        << (emit_records ? "true" : "false")
        << ",\"first_failure_witness\":";
    write_surrogate_failure_json(summary.corrected_first_failure);
    std::cout << ",\"first_late_witness\":";
    write_surrogate_failure_json(summary.corrected_first_late);
    std::cout << ",\"first_unsupported_witness\":";
    write_surrogate_failure_json(summary.corrected_first_unsupported);
    std::cout
        << ",\"corrected_tree\":{"
           "\"representation\":\"point_merge_tree_at_Gabriel_squared_levels_v1\","
           "\"selected_tree_edge_count\":"
        << summary.corrected_tree_edge_count
        << ",\"final_component_count\":1"
        << ",\"distinct_level_count\":"
        << summary.corrected_tree_distinct_level_count
        << ",\"root_level\":{\"level_encoding\":"
        << std::quoted(corrected_level_encoding_name(
               summary.corrected_tree_root_edge.level_encoding))
        << ",\"encoded_binary64\":" << std::setprecision(17)
        << summary.corrected_tree_root_edge.encoded_binary64 << '}'
        << ",\"tree_sha256\":\"" << summary.corrected_tree_sha256 << "\""
        << ",\"records_serialized\":"
        << (emit_records ? "true" : "false")
        << ",\"tree_edges\":";
    if (emit_records) {
      std::cout << '[';
      for (std::size_t edge_index = 0U;
           edge_index < summary.emitted_corrected_tree_edges.size();
           ++edge_index) {
        if (edge_index != 0U) {
          std::cout << ',';
        }
        const CorrectedSurrogateTreeEdge& edge =
            summary.emitted_corrected_tree_edges[edge_index];
        std::cout << "{\"u\":" << edge.u << ",\"v\":" << edge.v
                  << ",\"level_encoding\":"
                  << std::quoted(
                         corrected_level_encoding_name(edge.level_encoding))
                  << ",\"encoded_binary64\":" << std::setprecision(17)
                  << edge.encoded_binary64 << '}';
      }
      std::cout << ']';
    } else {
      std::cout << "null";
    }
    std::cout
        << '}'
        << ",\"overlay_records\":";
    if (emit_records) {
      std::cout << '[';
      for (std::size_t record_index = 0U;
           record_index < summary.emitted_correction_records.size();
           ++record_index) {
        if (record_index != 0U) {
          std::cout << ',';
        }
        write_triangle_json(summary.emitted_correction_records[record_index]);
      }
      std::cout << ']';
    } else {
      std::cout << "null";
    }
    std::cout
        << '}'
        << ",\"timings_nanoseconds\":{\"edge_build\":"
        << summary.edge_build_nanoseconds
        << ",\"tree_reduction\":"
        << summary.tree_reduction_nanoseconds
        << ",\"deadline_replay_and_overlay\":"
        << summary.deadline_replay_nanoseconds << "}}";
  }
  std::cout
      << "]}"
      << ",\"architecture\":{"
         "\"higher_order_Delaunay_mosaic_materialized\":false,"
         "\"global_pair_matrix_materialized\":false,"
         "\"weighted_tree_maximum_query_materialized\":false,"
         "\"orders_built_in_parallel\":true,"
         "\"order_worker_count\":"
      << run.order_worker_count
      << ",\"corrected_merge_tree_materialized_transiently\":true,"
         "\"massive_corrected_tree_records_serialized\":false,"
         "\"massive_overlay_records_materialized\":false}"
      << ",\"timings_nanoseconds\":{\"canonicalization\":"
      << run.canonicalization_nanoseconds
      << ",\"certified_LBVH_build\":" << run.lbvh_build_nanoseconds
      << ",\"top_k_query\":" << run.top_k_query_nanoseconds << "}}";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const auto total_begin = Clock::now();
    Options options = parse_options(argc, argv);
    const unsigned int hardware = std::thread::hardware_concurrency();
    const std::size_t available =
        hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware);
    const std::size_t cpu_workers = std::max(
        std::size_t{1}, std::min(options.cpu_workers, available));

    Timings timings;
    const auto input_begin = Clock::now();
    std::vector<Point3> points = make_points(options);
    options.point_count = points.size();
    if (points.size() < 4U) {
      throw std::invalid_argument("the diagnostic needs at least four points");
    }
    if (options.emit_records && points.size() > 256U) {
      throw std::invalid_argument(
          "--emit-records is intentionally limited to n <= 256; computation "
          "itself has no such cap");
    }
    timings.input_nanoseconds = nanoseconds(Clock::now() - input_begin);

    GeogramResult geogram = build_geogram_edges(points, cpu_workers);
    timings.geogram_nanoseconds = geogram.triangulation_nanoseconds;
    timings.edge_extraction_nanoseconds = geogram.extraction_nanoseconds;

    const auto csr_begin = Clock::now();
    CsrGraph graph =
        build_csr(points.size(), geogram.edges, cpu_workers);
    timings.csr_nanoseconds = nanoseconds(Clock::now() - csr_begin);

    DevicePipelineResult gpu = run_triangle_pipeline(points, graph, options);
    timings.grid_nanoseconds = gpu.grid_nanoseconds;
    timings.triangle_enumeration_nanoseconds = gpu.enumeration_nanoseconds;
    timings.triangle_classification_nanoseconds = gpu.classification_nanoseconds;
    timings.triangle_compaction_sort_nanoseconds =
        gpu.compaction_sort_nanoseconds;
    timings.triangle_device_to_host_nanoseconds =
        gpu.device_to_host_nanoseconds;

    if (options.gabriel_coverage_only) {
      const auto k2_begin = Clock::now();
      K2Summary coverage =
          options.surrogate_guardrail_max_order == 0U &&
                  options.gabriel_neighbor_rank_maximum == 0U
              ? reduce_k2(
                    gpu.retained_records, true, options.emit_records)
              : summarize_gabriel_source_inclusion(
                    gpu.retained_records, options.emit_records);
      timings.k2_reduction_nanoseconds = nanoseconds(Clock::now() - k2_begin);
      const ReconstructibleInputDigests input_digests =
          make_reconstructible_input_digests(
              points,
              geogram.edges,
              gpu.audit.raw_wedge_count,
              gpu.audit.canonical_candidate_count);
      const GabrielSafetyBatchManifest safety_batch =
          make_gabriel_safety_batch_manifest(
              gpu.retained_records, coverage);
      const std::size_t ordinary_edge_count = geogram.edges.size();
      const std::size_t maximum_vertex_degree = graph.maximum_degree;
      std::optional<SurrogateGuardrailRun> surrogate_guardrails;
      std::optional<GabrielNeighborRankRun> neighbor_rank_diagnostic;
      if (options.surrogate_guardrail_max_order != 0U) {
        if (points.size() <= options.surrogate_guardrail_max_order) {
          throw std::invalid_argument(
              "the cloud must contain more points than the surrogate order");
        }
        if (!options.emit_records) {
          std::vector<Edge>().swap(geogram.edges);
          std::vector<std::uint64_t>().swap(graph.offsets);
          std::vector<PointId>().swap(graph.neighbors);
        }
        const auto surrogate_begin = Clock::now();
        surrogate_guardrails.emplace(run_surrogate_guardrails(
            points,
            gpu.retained_records,
            options.surrogate_guardrail_max_order,
            options.surrogate_seed_window_radius,
            cpu_workers,
            options.emit_records));
        timings.surrogate_guardrail_nanoseconds =
            nanoseconds(Clock::now() - surrogate_begin);
      }
      if (options.gabriel_neighbor_rank_maximum != 0U) {
        if (points.size() <= options.gabriel_neighbor_rank_maximum) {
          throw std::invalid_argument(
              "the cloud must contain more points than the neighbor-rank cap");
        }
        if (!options.emit_records) {
          std::vector<Edge>().swap(geogram.edges);
        }
        const auto neighbor_rank_begin = Clock::now();
        neighbor_rank_diagnostic.emplace(
            run_gabriel_neighbor_rank_diagnostic(
                points,
                graph,
                gpu.retained_records,
                gpu.audit.accepted_count,
                options.gabriel_neighbor_rank_maximum,
                options.neighbor_rank_seed_window_radius,
                options.neighbor_rank_query_batch_size));
        timings.neighbor_rank_diagnostic_nanoseconds =
            nanoseconds(Clock::now() - neighbor_rank_begin);
      }
      const bool accepted_partition_closed =
          coverage.necessary_triangle_count +
                  coverage.strictly_lower_connected_triangle_count ==
              gpu.audit.accepted_count &&
          coverage.support_two_count + coverage.support_three_count ==
              gpu.audit.accepted_count &&
          coverage.coverage_violation_count == 0U;
      const bool status_partition_closed =
          gpu.audit.blocked_count + gpu.audit.accepted_count +
                  gpu.audit.ambiguous_count + gpu.audit.invalid_count ==
              gpu.audit.canonical_candidate_count;
      const bool binary64_gate_passed =
          accepted_partition_closed && status_partition_closed &&
          safety_batch.ambiguous_triangle_count ==
              gpu.audit.ambiguous_count &&
          gpu.audit.invalid_count == 0U;
      const bool corrected_surrogate_regular_guardrail_passed =
          !surrogate_guardrails.has_value() ||
          std::all_of(
              surrogate_guardrails->orders.begin(),
              surrogate_guardrails->orders.end(),
              [](const SurrogateGuardrailOrderSummary& summary) {
                return summary.corrected_late_count == 0U &&
                       summary.corrected_never_count == 0U &&
                       summary.corrected_tree_edge_count ==
                           summary.tree_edge_count;
              });
      const bool compact_neighbor_rank_gate_passed =
          !neighbor_rank_diagnostic.has_value() ||
          (neighbor_rank_diagnostic->rank_audit.complete_query_coverage &&
           neighbor_rank_diagnostic->rank_audit.failed_query_count == 0U &&
           neighbor_rank_diagnostic->rank_audit.no_search_work_cap &&
           neighbor_rank_diagnostic->rank_audit
               .exact_binary64_prefix_complete &&
           neighbor_rank_diagnostic->summary.directed_root_star
                   .missing_witness_triangle_count == 0U &&
           neighbor_rank_diagnostic->summary.symmetric_union_star
                   .missing_witness_triangle_count == 0U &&
           neighbor_rank_diagnostic->summary.mutual_star
                   .missing_witness_triangle_count == 0U);
      const bool conditional_cross_variant_gate_passed =
          binary64_gate_passed &&
          corrected_surrogate_regular_guardrail_passed &&
          compact_neighbor_rank_gate_passed;
      const bool fail_closed_cross_variant_gate_passed =
          conditional_cross_variant_gate_passed &&
          safety_batch.ambiguous_triangle_count == 0U &&
          (!surrogate_guardrails.has_value() ||
           std::all_of(
               surrogate_guardrails->orders.begin(),
               surrogate_guardrails->orders.end(),
               [](const SurrogateGuardrailOrderSummary& summary) {
                 return summary.corrected_unsupported_count == 0U;
               }));
      const bool selected_process_gate_passed =
          options.allow_conditional_guardrail
              ? conditional_cross_variant_gate_passed
              : fail_closed_cross_variant_gate_passed;
      timings.total_nanoseconds = nanoseconds(Clock::now() - total_begin);

      std::cout
          << "{\"schema\":"
          << std::quoted(
                 neighbor_rank_diagnostic.has_value()
                     ? "morsehgp3d.phase15_delaunay_gabriel_neighbor_rank.v1"
                 : surrogate_guardrails.has_value()
                     ? "morsehgp3d.phase15_delaunay_gabriel_fusion_guardrails.v1"
                     : "morsehgp3d.phase15_delaunay_gabriel_coverage.v1")
          << ",\"git_sha\":\"" << MORSEHGP3D_GIT_SHA << "\""
          << ",\"phase\":\"15\""
          << ",\"backend\":"
          << std::quoted(
                 neighbor_rank_diagnostic.has_value()
                     ? "geogram_PDEL_plus_cuda_Gabriel_grid_plus_binary64_Morton_LBVH_CSR_ranks"
                 : surrogate_guardrails.has_value()
                     ? "ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid_plus_binary64_LBVH_surrogate"
                     : "ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid")
          << ",\"profile\":\"hgp_reduced\""
          << ",\"mode\":"
          << std::quoted(
                 neighbor_rank_diagnostic.has_value()
                     ? "offline_neighbor_rank"
                 : surrogate_guardrails.has_value()
                     ? "gabriel_necessary_batch_plus_variant_fusion_guardrails"
                     : "gabriel_necessary_batch")
          << ",\"deployment_status\":\"diagnostic_sidecar\""
          << ",\"public_status\":\"not_claimed\""
          << ",\"criterion\":"
          << std::quoted(
                 neighbor_rank_diagnostic.has_value()
                     ? "smallest_neighbor_budget_for_each_accepted_Gabriel_triangle"
                     : "triangle_explicit_or_three_facets_connected_at_strictly_lower_level")
          << ",\"coverage_reduction_mode\":"
          << std::quoted(
                 surrogate_guardrails.has_value() ||
                         neighbor_rank_diagnostic.has_value()
                     ? "all_accepted_Gabriel_sources_explicit_no_global_facet_DSU"
                     : "strict_lower_Gabriel_facet_DSU")
          << ",\"proof_scope\":{"
             "\"general_position_required\":true,"
             "\"complete_Voronoi_nerve_one_skeleton_required\":true,"
             "\"Geogram_SoS_topology_exactly_recertified\":false,"
             "\"every_regular_Gabriel_triangle_has_at_least_two_Delaunay_edges\":true,"
             "\"support_cardinality_two_and_three_covered\":true,"
             "\"exact_Gamma2_claimed\":false,"
             "\"exact_Gabriel_classification_claimed\":false}"
          << ",\"architecture\":{"
             "\"edge_times_point_product_enumerated\":false,"
             "\"higher_order_Delaunay_mosaic_materialized\":false,"
             "\"ordinary_Delaunay_edges_only\":true,"
             "\"canonical_wedges_enumerated_in_bounded_GPU_chunks\":true,"
             "\"global_restricted_Gamma2_arena_materialized\":false,"
             "\"global_Gabriel_proposal_arena_materialized_for_level_sort\":true,"
             "\"global_n_by_M_neighbor_table_materialized\":false,"
             "\"additional_global_triangle_rank_table_materialized\":false,"
             "\"same_level_candidates_can_certify_each_other\":false}"
          << ",\"input\":{\"point_count\":" << points.size()
          << ",\"seed\":" << options.seed
          << ",\"cpu_workers_requested\":" << options.cpu_workers
          << ",\"cpu_workers_used\":" << cpu_workers
          << ",\"point_cloud_sha256\":\""
          << input_digests.point_cloud_sha256 << "\"}"
          << ",\"delaunay\":{\"engine\":\"PDEL\",\"cell_count\":"
          << geogram.cell_count
          << ",\"ordinary_edge_count\":" << ordinary_edge_count
          << ",\"maximum_vertex_degree\":" << maximum_vertex_degree
          << ",\"ordinary_edges_sha256\":\""
          << input_digests.ordinary_delaunay_edges_sha256 << "\"}"
          << ",\"candidate_universe\":{\"generator\":\"canonical_Delaunay_CSR_wedges\""
          << ",\"raw_wedge_count\":" << gpu.audit.raw_wedge_count
          << ",\"canonical_candidate_count\":"
          << gpu.audit.canonical_candidate_count
          << ",\"canonical_wedge_universe_sha256\":\""
          << input_digests.canonical_wedge_universe_sha256 << "\""
          << ",\"chunk_count\":" << gpu.audit.chunk_count
          << ",\"triangle_chunk_vertices\":"
          << options.triangle_chunk_vertices
          << ",\"maximum_chunk_candidate_count\":"
          << gpu.audit.maximum_chunk_candidate_count << "}"
          << ",\"classification\":{\"blocked_count\":"
          << gpu.audit.blocked_count
          << ",\"gabriel_binary64_count\":" << gpu.audit.accepted_count
          << ",\"ambiguous_safety_count\":" << gpu.audit.ambiguous_count
          << ",\"invalid_count\":" << gpu.audit.invalid_count
          << ",\"status_partition_closed\":"
          << (status_partition_closed ? "true" : "false")
          << ",\"visited_cell_count\":" << gpu.audit.visited_cell_count
          << ",\"tested_point_count\":" << gpu.audit.tested_point_count
          << "}"
          << ",\"coverage\":{\"source_triangle_count\":"
          << gpu.audit.accepted_count
          << ",\"support_two_count\":" << coverage.support_two_count
          << ",\"support_three_count\":" << coverage.support_three_count
          << ",\"explicit_necessary_accepted_count\":"
          << coverage.necessary_triangle_count
          << ",\"strictly_lower_connected_count\":"
          << coverage.strictly_lower_connected_triangle_count
          << ",\"coverage_violation_count\":"
          << coverage.coverage_violation_count
          << ",\"accepted_partition_closed\":"
          << (accepted_partition_closed ? "true" : "false")
          << ",\"binary64_gate_passed\":"
          << (binary64_gate_passed ? "true" : "false")
          << ",\"conditional_cross_variant_gate_passed\":"
          << (conditional_cross_variant_gate_passed ? "true" : "false")
          << ",\"compact_neighbor_rank_gate_passed\":"
          << (compact_neighbor_rank_gate_passed ? "true" : "false")
          << ",\"fail_closed_cross_variant_gate_passed\":"
          << (fail_closed_cross_variant_gate_passed ? "true" : "false")
          << ",\"allow_conditional_guardrail\":"
          << (options.allow_conditional_guardrail ? "true" : "false")
          << ",\"selected_process_gate_passed\":"
          << (selected_process_gate_passed ? "true" : "false")
          << ",\"accepted_source_sha256\":\""
          << coverage.accepted_triangle_sha256
          << "\",\"necessary_accepted_sha256\":\""
          << coverage.necessary_triangle_sha256
          << "\",\"ambiguous_safety_sha256\":\""
          << safety_batch.ambiguous_triangle_sha256
          << "\",\"explicit_safety_batch_count\":"
          << safety_batch.total_explicit_triangle_count
          << ",\"explicit_safety_batch_sha256\":\""
          << safety_batch.composite_batch_sha256
          << "\",\"explicit_safety_batch_semantics\":\"necessary_accepted_plus_all_ambiguous\""
          << ",\"necessary_records\":";
      if (options.emit_records) {
        std::cout << '[';
        bool first_record = true;
        for (const ClassifiedTriangle& record :
             coverage.emitted_necessary_records) {
          if (!first_record) {
            std::cout << ',';
          }
          first_record = false;
          write_triangle_json(record);
        }
        std::cout << ']';
      } else {
        std::cout << "null";
      }
      std::cout
          << ",\"ambiguous_safety_records\":";
      if (options.emit_records) {
        std::cout << '[';
        bool first_record = true;
        for (const ClassifiedTriangle& record : gpu.retained_records) {
          if (record.status != TriangleStatus::ambiguous) {
            continue;
          }
          if (!first_record) {
            std::cout << ',';
          }
          first_record = false;
          write_triangle_json(record);
        }
        std::cout << ']';
      } else {
        std::cout << "null";
      }
      std::cout << "}"
                << ",\"emitted_fixture\":";
      if (options.emit_records) {
        std::cout << "{\"points\":[";
        for (std::size_t index = 0U; index < points.size(); ++index) {
          if (index != 0U) {
            std::cout << ',';
          }
          write_point_json(points[index]);
        }
        std::cout << "],\"ordinary_delaunay_edges\":[";
        for (std::size_t index = 0U; index < geogram.edges.size(); ++index) {
          if (index != 0U) {
            std::cout << ',';
          }
          std::cout << "{\"u\":" << geogram.edges[index].u
                    << ",\"v\":" << geogram.edges[index].v << '}';
        }
        std::cout << "],\"source_records\":[";
        for (std::size_t index = 0U;
             index < gpu.retained_records.size();
             ++index) {
          if (index != 0U) {
            std::cout << ',';
          }
          write_triangle_json(gpu.retained_records[index]);
        }
        std::cout << "]}";
      } else {
        std::cout << "null";
      }
      std::cout << ",\"variant_guardrails\":";
      if (surrogate_guardrails.has_value()) {
        write_variant_guardrails_json(
            *surrogate_guardrails,
            coverage,
            safety_batch,
            input_digests.point_cloud_sha256,
            options.emit_records);
      } else {
        std::cout << "null";
      }
      std::cout << ",\"neighbor_rank_diagnostic\":";
      if (neighbor_rank_diagnostic.has_value()) {
        write_gabriel_neighbor_rank_json(
            *neighbor_rank_diagnostic, points.size());
      } else {
        std::cout << "null";
      }
      std::cout
          << ",\"gpu\":{\"device_name\":" << std::quoted(gpu.cuda_device_name)
          << ",\"multiprocessor_count\":" << gpu.cuda_multiprocessor_count
          << ",\"grid_resolution\":" << gpu.grid_resolution
          << ",\"maximum_cell_occupancy\":"
          << gpu.maximum_cell_occupancy
          << ",\"persistent_device_bytes\":" << gpu.persistent_device_bytes
          << ",\"maximum_chunk_device_bytes\":"
          << gpu.audit.maximum_chunk_device_bytes
          << ",\"maximum_accounted_live_device_bytes\":"
          << gpu.maximum_accounted_live_device_bytes << "}"
          << ",\"timings_nanoseconds\":{\"input\":"
          << timings.input_nanoseconds
          << ",\"geogram_delaunay\":" << timings.geogram_nanoseconds
          << ",\"geogram_edge_extraction\":"
          << timings.edge_extraction_nanoseconds
          << ",\"csr\":" << timings.csr_nanoseconds
          << ",\"gpu_grid\":" << timings.grid_nanoseconds
          << ",\"gpu_triangle_enumeration\":"
          << timings.triangle_enumeration_nanoseconds
          << ",\"gpu_triangle_classification\":"
          << timings.triangle_classification_nanoseconds
          << ",\"gpu_triangle_compaction_sort\":"
          << timings.triangle_compaction_sort_nanoseconds
          << ",\"host_global_Gabriel_record_sort\":"
          << gpu.host_sort_nanoseconds
          << ",\"triangle_device_to_host\":"
          << timings.triangle_device_to_host_nanoseconds
          << ",\"coverage_reduction\":" << timings.k2_reduction_nanoseconds
          << ",\"surrogate_guardrail\":"
          << timings.surrogate_guardrail_nanoseconds
          << ",\"neighbor_rank_diagnostic\":"
          << timings.neighbor_rank_diagnostic_nanoseconds
          << ",\"cold_e2e\":" << timings.total_nanoseconds << "}}\n";
      return selected_process_gate_passed ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    const auto k1_begin = Clock::now();
    K1Summary k1 = reduce_k1(points, geogram.edges);
    timings.k1_reduction_nanoseconds = nanoseconds(Clock::now() - k1_begin);

    const auto k2_begin = Clock::now();
    K2Summary k2 = reduce_k2(gpu.retained_records, true);
    timings.k2_reduction_nanoseconds = nanoseconds(Clock::now() - k2_begin);
    const auto restricted_gamma_k2_begin = Clock::now();
    K2Summary restricted_gamma_k2 =
        reduce_k2(gpu.restricted_gamma_records, false);
    const std::uint64_t restricted_gamma_k2_reduction_nanoseconds =
        nanoseconds(Clock::now() - restricted_gamma_k2_begin);
    timings.total_nanoseconds = nanoseconds(Clock::now() - total_begin);

    const auto write_triangle_records = [&](bool include_retained,
                                            bool include_invalid,
                                            bool include_restricted_gamma) {
      if (!options.emit_records) {
        std::cout << "null";
        return;
      }
      std::cout << '[';
      bool first = true;
      const auto write_range = [&](const auto& records) {
        for (const ClassifiedTriangle& triangle : records) {
          if (!first) {
            std::cout << ',';
          }
          first = false;
          write_triangle_json(triangle);
        }
      };
      if (include_retained) {
        write_range(gpu.retained_records);
      }
      if (include_invalid) {
        write_range(gpu.invalid_records);
      }
      if (include_restricted_gamma) {
        write_range(gpu.restricted_gamma_records);
      }
      std::cout << ']';
    };

    std::cout << "{\"schema\":\"morsehgp3d.phase14.geogram_low_order_gpu.v1\""
              << ",\"git_sha\":\"" << MORSEHGP3D_GIT_SHA << "\""
              << ",\"phase\":\"14\""
              << ",\"backend\":\"geogram_pdel_plus_cuda_g4_aabb_grid\""
              << ",\"profile\":\"hgp_reduced\""
              << ",\"mode\":\"proposal_only_external_small_n_exact_comparison\""
              << ",\"deployment_status\":\"diagnostic_only\""
              << ",\"public_status\":\"not_claimed\""
              << ",\"approximations\":["
                 "\"triangle_miniballs_and_empty_ball_predicates_are_binary64_with_explicit_support_and_point_ambiguity_bands\","
                 "\"cell_pruning_uses_observed_per_cell_AABBs_and_downward_rounded_lower_bounds\","
                 "\"ordinary_Delaunay_combinatorics_are_supplied_by_Geogram_PDEL\","
                 "\"the_raw_Gabriel_hierarchy_is_only_a_partial_refinement_of_Gamma_2\""
                 "]"
              << ",\"architecture\":{"
                 "\"all_hypothetical_input_triangles_enumerated\":false,"
                 "\"all_Delaunay_CSR_wedge_candidates_enumerated_on_gpu\":true,"
                 "\"Delaunay_CSR_wedge_candidate_universe_proven_complete\":false,"
                 "\"candidate_canonical_ownership_and_dedup_on_gpu\":true,"
                 "\"all_candidate_miniballs_on_gpu\":true,"
                 "\"all_nonambiguous_candidate_empty_ball_AABB_cell_queries_on_gpu\":true,"
                 "\"all_valid_Delaunay_CSR_wedge_cofaces_in_restricted_Gamma2\":true,"
                 "\"global_retained_record_sort_on_gpu\":false,"
                 "\"global_host_retained_record_arena_materialized\":true,"
                 "\"global_host_restricted_Gamma2_record_arena_materialized\":true,"
                 "\"k2_reduction_on_gpu\":false,"
                 "\"six_Morton_orders_materialized\":false,"
                 "\"higher_order_Delaunay_mosaic_materialized\":false,"
                 "\"ordinary_Delaunay_only\":true}"
              << ",\"input\":{\"point_count\":" << points.size()
              << ",\"seed\":" << options.seed
              << ",\"cpu_workers_requested\":" << options.cpu_workers
              << ",\"cpu_workers_used\":" << cpu_workers
              << ",\"points\":";
    if (options.emit_records) {
      std::cout << '[';
      for (std::size_t index = 0U; index < points.size(); ++index) {
        if (index != 0U) {
          std::cout << ',';
        }
        write_point_json(points[index]);
      }
      std::cout << ']';
    } else {
      std::cout << "null";
    }
    std::cout << "}"
              << ",\"geogram\":{\"engine\":\"PDEL\",\"cell_count\":"
              << geogram.cell_count << ",\"cell_size\":" << geogram.cell_size
              << ",\"ordinary_Delaunay_edge_count\":"
              << geogram.edges.size() << ",\"maximum_vertex_degree\":"
              << graph.maximum_degree << '}'
              << ",\"gpu\":{\"device_name\":"
              << std::quoted(gpu.cuda_device_name)
              << ",\"multiprocessor_count\":"
              << gpu.cuda_multiprocessor_count
              << ",\"grid_resolution\":" << gpu.grid_resolution
              << ",\"grid_cell_count\":" << gpu.grid_cell_count
              << ",\"maximum_cell_occupancy\":"
              << gpu.maximum_cell_occupancy
              << ",\"triangle_chunk_vertices_requested\":"
              << options.triangle_chunk_vertices
              << ",\"zero_chunk_vertices_means_single_uncapped_chunk\":true"
              << ",\"chunk_candidate_hard_limit_enabled\":false"
              << ",\"persistent_device_bytes\":"
              << gpu.persistent_device_bytes
              << ",\"maximum_accounted_live_device_bytes\":"
              << gpu.maximum_accounted_live_device_bytes
              << ",\"accounted_memory_includes_thrust_temporary_storage\":false}"
              << ",\"k1\":{\"status\":\"delaunay_emst_binary64_witness\""
              << ",\"selected_edge_count\":" << k1.selected_edges.size()
              << ",\"distinct_level_count\":" << k1.distinct_level_count
              << ",\"root_squared_distance\":" << std::setprecision(17)
              << k1.root_squared_distance
              << ",\"root_squared_level\":" << k1.root_squared_level
              << ",\"surrogate_compatible_digest\":\"" << std::hex
              << std::setw(16) << std::setfill('0')
              << k1.surrogate_compatible_digest << std::dec
              << std::setfill(' ') << "\",\"selected_edges\":";
    if (options.emit_records) {
      std::cout << '[';
      for (std::size_t index = 0U; index < k1.selected_edges.size(); ++index) {
        if (index != 0U) {
          std::cout << ',';
        }
        const auto& [weight, u, v] = k1.selected_edges[index];
        write_edge_json(weight, u, v);
      }
      std::cout << ']';
    } else {
      std::cout << "null";
    }
    std::cout << "}"
              << ",\"k2\":{\"status\":\"delaunay_CSR_wedge_Gabriel_binary64_partial_refinement\""
              << ",\"candidate_universe\":\"canonical_Delaunay_CSR_wedges_not_all_input_triangles\""
              << ",\"raw_wedge_count\":" << gpu.audit.raw_wedge_count
              << ",\"canonical_candidate_count\":"
              << gpu.audit.canonical_candidate_count
              << ",\"blocked_count\":" << gpu.audit.blocked_count
              << ",\"accepted_triangle_count\":"
              << gpu.audit.accepted_count
              << ",\"ambiguous_triangle_count\":"
              << gpu.audit.ambiguous_count
              << ",\"invalid_triangle_count\":" << gpu.audit.invalid_count
              << ",\"visited_cell_count\":" << gpu.audit.visited_cell_count
              << ",\"tested_point_count\":" << gpu.audit.tested_point_count
              << ",\"chunk_count\":" << gpu.audit.chunk_count
              << ",\"maximum_chunk_candidate_count\":"
              << gpu.audit.maximum_chunk_candidate_count
              << ",\"maximum_chunk_device_bytes\":"
              << gpu.audit.maximum_chunk_device_bytes
              << ",\"retained_record_count\":"
              << gpu.retained_records.size()
              << ",\"invalid_records_omitted_from_all_sorts\":true"
              << ",\"invalid_record_squared_level_serialization\":\"zero_sentinel_not_a_geometric_level\""
              << ",\"facet_count\":" << k2.facet_count
              << ",\"final_component_count\":"
              << k2.final_component_count
              << ",\"useful_union_count\":" << k2.useful_union_count
              << ",\"redundant_union_count\":" << k2.redundant_union_count
              << ",\"distinct_level_count\":" << k2.distinct_level_count
              << ",\"first_squared_level\":" << k2.first_squared_level
              << ",\"root_squared_level\":" << k2.root_squared_level
              << ",\"restricted_gamma_status\":\"restricted_Delaunay_wedge_Gamma2_binary64\""
              << ",\"restricted_gamma_record_count\":"
              << gpu.restricted_gamma_records.size()
              << ",\"restricted_gamma_facet_count\":"
              << restricted_gamma_k2.facet_count
              << ",\"restricted_gamma_final_component_count\":"
              << restricted_gamma_k2.final_component_count
              << ",\"restricted_gamma_useful_union_count\":"
              << restricted_gamma_k2.useful_union_count
              << ",\"restricted_gamma_redundant_union_count\":"
              << restricted_gamma_k2.redundant_union_count
              << ",\"restricted_gamma_distinct_level_count\":"
              << restricted_gamma_k2.distinct_level_count
              << ",\"restricted_gamma_first_squared_level\":"
              << restricted_gamma_k2.first_squared_level
              << ",\"restricted_gamma_root_squared_level\":"
              << restricted_gamma_k2.root_squared_level
              << ",\"restricted_gamma_digest\":\"" << std::hex
              << std::setw(16) << std::setfill('0')
              << restricted_gamma_k2.accepted_triangle_digest << std::dec
              << std::setfill(' ') << "\""
              << ",\"accepted_triangle_digest\":\"" << std::hex
              << std::setw(16) << std::setfill('0')
              << k2.accepted_triangle_digest << std::dec
              << std::setfill(' ') << "\",\"retained_records\":";
    write_triangle_records(true, false, false);
    std::cout << ",\"invalid_records\":";
    write_triangle_records(false, true, false);
    std::cout << ",\"accepted_triangles_legacy_checker_alias\":true"
              << ",\"accepted_triangles\":";
    write_triangle_records(true, true, false);
    std::cout << ",\"restricted_gamma_records\":";
    write_triangle_records(false, false, true);
    std::cout << "}"
              << ",\"timings_nanoseconds\":{\"input\":"
              << timings.input_nanoseconds
              << ",\"geogram_delaunay\":" << timings.geogram_nanoseconds
              << ",\"geogram_edge_extraction\":"
              << timings.edge_extraction_nanoseconds
              << ",\"csr\":" << timings.csr_nanoseconds
              << ",\"gpu_grid\":" << timings.grid_nanoseconds
              << ",\"gpu_triangle_enumeration\":"
              << timings.triangle_enumeration_nanoseconds
              << ",\"gpu_triangle_classification\":"
              << timings.triangle_classification_nanoseconds
              << ",\"gpu_triangle_status_audit\":"
              << gpu.audit_nanoseconds
              << ",\"gpu_triangle_compaction_sort\":"
              << timings.triangle_compaction_sort_nanoseconds
              << ",\"gpu_restricted_gamma_compaction_sort\":"
              << gpu.restricted_gamma_compaction_sort_nanoseconds
              << ",\"host_global_retained_record_sort\":"
              << gpu.host_sort_nanoseconds
              << ",\"host_global_restricted_gamma_record_sort\":"
              << gpu.restricted_gamma_host_sort_nanoseconds
              << ",\"triangle_device_to_host\":"
              << timings.triangle_device_to_host_nanoseconds
              << ",\"restricted_gamma_triangle_device_to_host\":"
              << gpu.restricted_gamma_device_to_host_nanoseconds
              << ",\"k1_reduction\":" << timings.k1_reduction_nanoseconds
              << ",\"k2_reduction\":" << timings.k2_reduction_nanoseconds
              << ",\"restricted_gamma_k2_reduction\":"
              << restricted_gamma_k2_reduction_nanoseconds
              << ",\"cold_e2e\":" << timings.total_nanoseconds << "}}\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "gpu_geogram_low_order_diagnostic: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
