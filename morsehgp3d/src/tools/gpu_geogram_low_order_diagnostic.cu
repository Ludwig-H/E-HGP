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

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
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
  std::optional<std::string> input_xyz;
  bool fixture_e5{};
  bool emit_records{};
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
    } else if (argument == "--input-xyz" && index + 1 < argc) {
      options.input_xyz = std::string{argv[++index]};
    } else if (argument == "--fixture-e5") {
      options.fixture_e5 = true;
    } else if (argument == "--emit-records") {
      options.emit_records = true;
    } else {
      throw std::invalid_argument(
          "usage: gpu_geogram_low_order_diagnostic "
          "[--point-count N] [--seed N] [--cpu-workers N] "
          "[--grid-resolution R] [--triangle-chunk-vertices N] "
          "[--input-xyz PATH|--fixture-e5] [--emit-records]");
    }
  }
  if (options.fixture_e5 && options.input_xyz.has_value()) {
    throw std::invalid_argument("--fixture-e5 and --input-xyz are exclusive");
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
  std::vector<ClassifiedTriangle> invalid_records;
  TriangleAudit audit;
  std::uint64_t grid_nanoseconds{};
  std::uint64_t enumeration_nanoseconds{};
  std::uint64_t classification_nanoseconds{};
  std::uint64_t compaction_sort_nanoseconds{};
  std::uint64_t device_to_host_nanoseconds{};
  std::uint64_t host_sort_nanoseconds{};
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

    const auto compaction_begin = Clock::now();
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
    thrust::device_vector<ClassifiedTriangle> invalid_device_records(
        static_cast<std::size_t>(invalid));
    if (invalid != 0U) {
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
        nanoseconds(Clock::now() - compaction_begin),
        "triangle compaction time overflows uint64");

    const std::size_t chunk_bytes =
        device_vector_bytes(candidate_count, sizeof(Triangle)) +
        device_vector_bytes(candidate_count, sizeof(ClassifiedTriangle)) +
        device_vector_bytes(
            static_cast<std::size_t>(invalid), sizeof(ClassifiedTriangle)) +
        device_vector_bytes(center_count + 1U, 2U * sizeof(std::uint64_t));
    result.audit.maximum_chunk_device_bytes = std::max(
        result.audit.maximum_chunk_device_bytes, chunk_bytes);
    result.maximum_accounted_live_device_bytes = std::max(
        result.maximum_accounted_live_device_bytes,
        result.persistent_device_bytes + chunk_bytes);
    const auto copy_begin = Clock::now();
    std::vector<ClassifiedTriangle> host_chunk(classified.size());
    thrust::copy(classified.begin(), classified.end(), host_chunk.begin());
    std::vector<ClassifiedTriangle> host_invalid_chunk(
        invalid_device_records.size());
    thrust::copy(
        invalid_device_records.begin(),
        invalid_device_records.end(),
        host_invalid_chunk.begin());
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
    std::span<const ClassifiedTriangle> retained) {
  std::vector<const ClassifiedTriangle*> accepted;
  accepted.reserve(retained.size());
  std::vector<Edge> facets;
  for (const ClassifiedTriangle& triangle : retained) {
    if (triangle.status != TriangleStatus::gabriel_binary64) {
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
  if (accepted.empty()) {
    return summary;
  }
  std::sort(facets.begin(), facets.end(), edge_less);
  facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
  summary.facet_count = facets.size();
  DisjointSet components(facets.size());
  bool has_previous = false;
  std::uint64_t previous_bits{};
  for (const ClassifiedTriangle* record : accepted) {
    const Triangle triangle = record->triangle;
    const std::uint64_t bits =
        std::bit_cast<std::uint64_t>(record->squared_level);
    if (!has_previous || bits != previous_bits) {
      ++summary.distinct_level_count;
      has_previous = true;
      previous_bits = bits;
    }
    digest_word(summary.accepted_triangle_digest, bits);
    digest_word(summary.accepted_triangle_digest, triangle.a);
    digest_word(summary.accepted_triangle_digest, triangle.b);
    digest_word(summary.accepted_triangle_digest, triangle.c);
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
    for (std::size_t index = 1U; index < ids.size(); ++index) {
      if (components.unite(ids[0], ids[index])) {
        ++summary.useful_union_count;
      } else {
        ++summary.redundant_union_count;
      }
    }
  }
  summary.final_component_count = components.component_count();
  summary.first_squared_level = accepted.front()->squared_level;
  summary.root_squared_level = accepted.back()->squared_level;
  return summary;
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

    const auto k1_begin = Clock::now();
    K1Summary k1 = reduce_k1(points, geogram.edges);
    timings.k1_reduction_nanoseconds = nanoseconds(Clock::now() - k1_begin);

    DevicePipelineResult gpu = run_triangle_pipeline(points, graph, options);
    timings.grid_nanoseconds = gpu.grid_nanoseconds;
    timings.triangle_enumeration_nanoseconds = gpu.enumeration_nanoseconds;
    timings.triangle_classification_nanoseconds = gpu.classification_nanoseconds;
    timings.triangle_compaction_sort_nanoseconds =
        gpu.compaction_sort_nanoseconds;
    timings.triangle_device_to_host_nanoseconds =
        gpu.device_to_host_nanoseconds;

    const auto k2_begin = Clock::now();
    K2Summary k2 = reduce_k2(gpu.retained_records);
    timings.k2_reduction_nanoseconds = nanoseconds(Clock::now() - k2_begin);
    timings.total_nanoseconds = nanoseconds(Clock::now() - total_begin);

    const auto write_triangle_records = [&](bool include_retained,
                                            bool include_invalid) {
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
                 "\"all_candidate_empty_ball_AABB_cell_queries_on_gpu\":true,"
                 "\"global_retained_record_sort_on_gpu\":false,"
                 "\"global_host_retained_record_arena_materialized\":true,"
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
              << ",\"accepted_triangle_digest\":\"" << std::hex
              << std::setw(16) << std::setfill('0')
              << k2.accepted_triangle_digest << std::dec
              << std::setfill(' ') << "\",\"retained_records\":";
    write_triangle_records(true, false);
    std::cout << ",\"invalid_records\":";
    write_triangle_records(false, true);
    std::cout << ",\"accepted_triangles_legacy_checker_alias\":true"
              << ",\"accepted_triangles\":";
    write_triangle_records(true, true);
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
              << ",\"gpu_triangle_compaction_sort\":"
              << timings.triangle_compaction_sort_nanoseconds
              << ",\"host_global_retained_record_sort\":"
              << gpu.host_sort_nanoseconds
              << ",\"triangle_device_to_host\":"
              << timings.triangle_device_to_host_nanoseconds
              << ",\"k1_reduction\":" << timings.k1_reduction_nanoseconds
              << ",\"k2_reduction\":" << timings.k2_reduction_nanoseconds
              << ",\"cold_e2e\":" << timings.total_nanoseconds << "}}\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "gpu_geogram_low_order_diagnostic: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
