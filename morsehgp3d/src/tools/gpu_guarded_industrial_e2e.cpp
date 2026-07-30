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
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
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
#include <unordered_set>
#include <utility>
#include <vector>

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The guarded industrial runner requires a canonical Git SHA"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using PointId = morsehgp3d::spatial::PointId;

constexpr std::size_t kMaximumOrder = 10U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kComponentBridgeNeighborCount = 6U;
constexpr std::size_t kComponentBridgeProjectionSupportCount = 8U;
constexpr std::size_t kComponentBridgeMaximumComponentCount = 256U;
constexpr std::size_t kComponentBridgeMemberProjectionBudgetFactor = 16U;
constexpr std::size_t kComponentBridgeMaximumPairCount =
    kComponentBridgeMaximumComponentCount * kComponentBridgeNeighborCount;
constexpr std::size_t kComponentBridgeMaximumCrossDistanceEvaluationCount =
    kComponentBridgeProjectionSupportCount *
    kComponentBridgeProjectionSupportCount *
    kComponentBridgeMaximumPairCount;
constexpr std::size_t kComponentBridgeMaximumCentroidDistanceEvaluationCount =
    kComponentBridgeMaximumComponentCount *
    (kComponentBridgeMaximumComponentCount - 1U);
constexpr std::uint64_t kSloNanoseconds = UINT64_C(100000000);
// The offline k=2 guard compares a direct binary64 core distance with the
// same support-two diameter reconstructed by the historical triangle
// polarization recipe.  Lowering only core contributions preserves the exact
// k=1 edge weights, the edge-distance floor and monotonicity in k, while
// covering the observed bounded roundoff envelope on the registered 3-D
// families.  This remains a guarded convention, not an exact Morse-level
// claim.
constexpr std::uint64_t kCoreDeadlineLowerEnvelopeUlps = 64U;

enum class Family : std::uint8_t {
  affine_uniform_binary64,
  jittered_dyadic_grid3d,
  balanced_multiscale_clusters,
};

struct Options {
  std::size_t point_count{50'000U};
  std::size_t repetitions{10U};
  std::size_t warmups{2U};
  Family family{Family::affine_uniform_binary64};
  std::uint64_t seed_base{1U};
  std::size_t maximum_order{kMaximumOrder};
  std::size_t seed_window{32U};
  std::size_t export_maximum_order{kMaximumOrder};
  std::optional<std::filesystem::path> emit_tree_path;
};

struct RunnerBinaryIdentity {
  std::string sha256;
  std::uint64_t size_bytes{};
};

struct BaseEdge {
  std::uint64_t pair_key{};
  double squared_distance{};
};

struct WeightedEdge {
  double squared_weight{};
  std::uint64_t pair_key{};
};

struct MergeRecord {
  PointId first{};
  PointId second{};
  double squared_level{};
};

struct OrderReduction {
  std::size_t order{};
  std::vector<MergeRecord> merges;
  std::size_t merge_record_count{};
  std::uint64_t digest{};
  double root_squared_level{};
};

struct Topology {
  std::vector<BaseEdge> edges;
  std::vector<double> core_squared_distances;
  std::size_t top_k_component_count{};
  std::size_t component_bridge_pair_count{};
  std::size_t component_bridge_support_evaluation_count{};
  std::size_t component_bridge_centroid_distance_evaluation_count{};
  std::size_t component_bridge_member_projection_evaluation_budget{};
  std::size_t component_bridge_member_projection_evaluation_count{};
  std::size_t component_bridge_cross_distance_evaluation_budget{
      kComponentBridgeMaximumCrossDistanceEvaluationCount};
  std::size_t component_bridge_cross_distance_evaluation_count{};
  bool component_bridge_budget_satisfied{};
  std::string component_bridge_budget_stop_reason{"not_evaluated"};
};

struct RunMetrics {
  std::size_t run_index{};
  std::optional<std::size_t> measured_index;
  bool warmup{};
  std::uint64_t seed{};
  std::uint64_t generation_ns{};
  std::uint64_t canonicalization_ns{};
  std::uint64_t lbvh_ns{};
  std::uint64_t top_k_ns{};
  std::uint64_t topology_ns{};
  std::uint64_t reductions_ns{};
  std::uint64_t output_materialization_ns{};
  std::uint64_t warm_e2e_ns{};
  std::uint64_t oracle_export_ns{};
  std::optional<std::string> oracle_export_path;
  std::size_t common_topology_edge_count{};
  std::size_t top_k_component_count{};
  std::size_t component_bridge_pair_count{};
  std::size_t component_bridge_support_evaluation_count{};
  std::size_t component_bridge_centroid_distance_evaluation_count{};
  std::size_t component_bridge_member_projection_evaluation_budget{};
  std::size_t component_bridge_member_projection_evaluation_count{};
  std::size_t component_bridge_cross_distance_evaluation_budget{};
  std::size_t component_bridge_cross_distance_evaluation_count{};
  bool component_bridge_budget_satisfied{};
  std::string component_bridge_budget_stop_reason;
  std::size_t neighbor_record_count{};
  std::size_t lbvh_kernel_launch_count{};
  std::size_t top_k_node_visit_count{};
  std::size_t top_k_strict_prune_count{};
  std::string device_name;
  std::size_t materialized_merge_record_count{};
  std::size_t released_merge_record_count{};
  std::size_t retained_merge_record_count{};
  bool merge_records_released_after_digest_and_export{};
  std::size_t exported_order_count{};
  std::size_t exported_merge_record_count{};
  std::vector<OrderReduction> reductions;
};

[[nodiscard]] std::size_t parse_positive_size(
    std::string_view text,
    const char* role) {
  std::size_t value{};
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size() || value == 0U) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] std::size_t parse_nonnegative_size(
    std::string_view text,
    const char* role) {
  std::size_t value{};
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(
    std::string_view text,
    const char* role) {
  std::uint64_t value{};
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Family parse_family(std::string_view text) {
  if (text == "affine_uniform_binary64") {
    return Family::affine_uniform_binary64;
  }
  if (text == "jittered_dyadic_grid3d") {
    return Family::jittered_dyadic_grid3d;
  }
  if (text == "balanced_multiscale_clusters") {
    return Family::balanced_multiscale_clusters;
  }
  throw std::invalid_argument(
      "--family must be affine_uniform_binary64, "
      "jittered_dyadic_grid3d, or balanced_multiscale_clusters");
}

[[nodiscard]] std::string_view family_name(Family family) noexcept {
  switch (family) {
    case Family::affine_uniform_binary64:
      return "affine_uniform_binary64";
    case Family::jittered_dyadic_grid3d:
      return "jittered_dyadic_grid3d";
    case Family::balanced_multiscale_clusters:
      return "balanced_multiscale_clusters";
  }
  return "invalid";
}

[[nodiscard]] std::filesystem::path running_executable_path(
    const char* argv0) {
#if defined(__linux__)
  static_cast<void>(argv0);
  return std::filesystem::path{"/proc/self/exe"};
#else
  if (argv0 == nullptr || std::string_view{argv0}.empty()) {
    throw std::runtime_error("argv[0] cannot identify the running executable");
  }
  std::error_code error;
  const std::filesystem::path resolved =
      std::filesystem::canonical(std::filesystem::path{argv0}, error);
  if (error) {
    throw std::runtime_error(
        "cannot resolve the running executable: " + error.message());
  }
  return resolved;
#endif
}

[[nodiscard]] RunnerBinaryIdentity hash_running_executable(const char* argv0) {
  const std::filesystem::path path = running_executable_path(argv0);
  std::ifstream input{path, std::ios::binary};
  if (!input) {
    throw std::runtime_error(
        "cannot open the running executable for SHA-256: " + path.string());
  }
  morsehgp3d::contract::CanonicalSha256Builder builder;
  std::array<std::uint8_t, 64U * 1024U> buffer{};
  std::uint64_t size_bytes = 0U;
  while (input) {
    input.read(
        reinterpret_cast<char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size()));
    const std::streamsize extracted = input.gcount();
    if (extracted <= 0) {
      continue;
    }
    const std::size_t count = static_cast<std::size_t>(extracted);
    if (static_cast<std::uint64_t>(count) >
        std::numeric_limits<std::uint64_t>::max() - size_bytes) {
      throw std::length_error("the running executable size overflows uint64");
    }
    builder.update(std::span<const std::uint8_t>{buffer.data(), count});
    size_bytes += static_cast<std::uint64_t>(count);
  }
  if (input.bad()) {
    throw std::runtime_error(
        "cannot read the running executable for SHA-256: " + path.string());
  }
  if (size_bytes == 0U) {
    throw std::runtime_error("the running executable is empty");
  }
  return RunnerBinaryIdentity{builder.finalize().to_lower_hex(), size_bytes};
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count =
          parse_positive_size(argv[++index], "invalid --point-count");
    } else if (argument == "--repetitions" && index + 1 < argc) {
      options.repetitions =
          parse_positive_size(argv[++index], "invalid --repetitions");
    } else if (argument == "--warmups" && index + 1 < argc) {
      options.warmups =
          parse_nonnegative_size(argv[++index], "invalid --warmups");
    } else if (argument == "--family" && index + 1 < argc) {
      options.family = parse_family(argv[++index]);
    } else if (argument == "--seed-base" && index + 1 < argc) {
      options.seed_base = parse_u64(argv[++index], "invalid --seed-base");
    } else if (argument == "--max-order" && index + 1 < argc) {
      options.maximum_order =
          parse_positive_size(argv[++index], "invalid --max-order");
    } else if (argument == "--seed-window" && index + 1 < argc) {
      options.seed_window =
          parse_positive_size(argv[++index], "invalid --seed-window");
    } else if (argument == "--export-max-order" && index + 1 < argc) {
      options.export_maximum_order =
          parse_positive_size(argv[++index], "invalid --export-max-order");
    } else if (argument == "--emit-tree-path" && index + 1 < argc) {
      const std::string value{argv[++index]};
      if (value.empty()) {
        throw std::invalid_argument("empty --emit-tree-path");
      }
      options.emit_tree_path.emplace(value);
    } else {
      throw std::invalid_argument(
          "usage: gpu_point_mst_mutual_reachability_surrogate [--point-count N] "
          "[--repetitions N] [--warmups N] "
          "[--family affine_uniform_binary64|jittered_dyadic_grid3d|"
          "balanced_multiscale_clusters] [--seed-base N] "
          "[--max-order 10] [--seed-window W] [--export-max-order N] "
          "[--emit-tree-path PATH]");
    }
  }
  if (options.point_count <= kMaximumOrder ||
      options.point_count > static_cast<std::size_t>(INT_MAX)) {
    throw std::invalid_argument(
        "--point-count must be greater than 10 and fit the CUDA item domain");
  }
  if (options.maximum_order != kMaximumOrder) {
    throw std::invalid_argument("the point-tree surrogate requires --max-order 10");
  }
  if (options.seed_window < options.maximum_order) {
    throw std::invalid_argument("--seed-window must be at least --max-order");
  }
  if (options.export_maximum_order > options.maximum_order) {
    throw std::invalid_argument(
        "--export-max-order must lie in 1..--max-order");
  }
  if (options.warmups > 1'000U || options.repetitions > 1'000U) {
    throw std::invalid_argument("warmup and repetition counts are bounded at 1000");
  }
  if (options.seed_base >
      std::numeric_limits<std::uint64_t>::max() -
          static_cast<std::uint64_t>(options.repetitions - 1U)) {
    throw std::overflow_error("the measured seed sequence overflows uint64");
  }
  return options;
}

template <class Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto count =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (count < 0) {
    throw std::runtime_error("the monotonic clock moved backwards");
  }
  return static_cast<std::uint64_t>(count);
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] std::size_t permutation_multiplier(std::size_t count) {
  std::size_t multiplier = static_cast<std::size_t>(
      UINT64_C(2654435761) % static_cast<std::uint64_t>(count));
  multiplier = std::max(multiplier, std::size_t{1});
  while (std::gcd(multiplier, count) != 1U) {
    ++multiplier;
  }
  return multiplier;
}

[[nodiscard]] std::size_t permuted_index(
    std::size_t index,
    std::size_t multiplier,
    std::size_t offset,
    std::size_t count) noexcept {
  return static_cast<std::size_t>(
      (static_cast<std::uint64_t>(index) *
           static_cast<std::uint64_t>(multiplier) +
       static_cast<std::uint64_t>(offset)) %
      static_cast<std::uint64_t>(count));
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_affine_uniform_cloud(std::size_t count, std::uint64_t seed) {
  constexpr std::uint64_t kMantissaMask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t kOneExponent = UINT64_C(0x3ff0000000000000);
  const std::uint64_t step = std::max(
      UINT64_C(1), kMantissaMask / static_cast<std::uint64_t>(count));
  const std::size_t multiplier = permutation_multiplier(count);
  const std::size_t offset = static_cast<std::size_t>(
      seed % static_cast<std::uint64_t>(count));
  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t source = 0U; source < count; ++source) {
    const std::size_t logical =
        permuted_index(source, multiplier, offset, count);
    const std::uint64_t identity = static_cast<std::uint64_t>(logical);
    points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64_bits(
            {kOneExponent | (identity * step),
             kOneExponent | (splitmix64(identity ^ seed) & kMantissaMask),
             kOneExponent |
                 (splitmix64(identity ^ std::rotl(seed, 23)) &
                  kMantissaMask)}));
  }
  return points;
}

[[nodiscard]] std::size_t grid_side_for(std::size_t count) {
  std::size_t side = static_cast<std::size_t>(
      std::ceil(std::cbrt(static_cast<double>(count))));
  side = std::max(side, std::size_t{1});
  while (side * side * side < count) {
    ++side;
  }
  return side;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_jittered_dyadic_cloud(std::size_t count, std::uint64_t seed) {
  constexpr std::int64_t kDenominator = INT64_C(1) << 30U;
  const std::size_t side = grid_side_for(count);
  const std::int64_t stride =
      (kDenominator - 16) / static_cast<std::int64_t>(side + 1U);
  const std::size_t multiplier = permutation_multiplier(count);
  const std::size_t offset = static_cast<std::size_t>(
      seed % static_cast<std::uint64_t>(count));
  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t source = 0U; source < count; ++source) {
    const std::size_t logical =
        permuted_index(source, multiplier, offset, count);
    const std::array<std::size_t, 3> cell{
        logical % side,
        (logical / side) % side,
        logical / (side * side)};
    std::array<double, 3> coordinate{};
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      const std::uint64_t random = splitmix64(
          static_cast<std::uint64_t>(logical) ^ seed ^
          (UINT64_C(0x9e3779b97f4a7c15) *
           static_cast<std::uint64_t>(axis + 1U)));
      const std::int64_t jitter =
          static_cast<std::int64_t>(random % UINT64_C(7)) - 3;
      const std::int64_t numerator =
          static_cast<std::int64_t>(cell[axis] + 1U) * stride + jitter;
      coordinate[axis] =
          1.0 + std::ldexp(static_cast<double>(numerator), -30);
    }
    points.push_back(morsehgp3d::exact::CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return points;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3>
make_balanced_multiscale_cloud(std::size_t count, std::uint64_t seed) {
  constexpr std::uint64_t kOneExponent = UINT64_C(0x3ff0000000000000);
  constexpr std::int64_t kCenterUnit = INT64_C(1) << 48U;
  const std::size_t multiplier = permutation_multiplier(count);
  const std::size_t offset = static_cast<std::size_t>(
      seed % static_cast<std::uint64_t>(count));
  std::vector<morsehgp3d::exact::CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t source = 0U; source < count; ++source) {
    const std::size_t logical =
        permuted_index(source, multiplier, offset, count);
    const std::size_t cluster = logical % 64U;
    const std::size_t local = logical / 64U;
    const std::size_t scale = local % 8U;
    const std::array<std::size_t, 3> grid{
        cluster % 4U, (cluster / 4U) % 4U, cluster / 16U};
    std::array<std::uint64_t, 3> bits{};
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      // The eight dyadic radii retain a 64x multiscale spread while their
      // absolute range keeps Qhull above its precision floor without any
      // input perturbation.  Independent integer offsets also avoid the exact
      // diagonal lines generated by the former shared local offset.
      const std::int64_t radius = INT64_C(1) << (scale + 38U);
      const std::uint64_t random = splitmix64(
          static_cast<std::uint64_t>(logical) ^ seed ^
          (UINT64_C(0xd6e8feb86659fd93) *
           static_cast<std::uint64_t>(axis + 1U)));
      const std::uint64_t width =
          static_cast<std::uint64_t>(radius * 2 + 1);
      const std::int64_t bucket =
          static_cast<std::int64_t>(random % width) - radius;
      const std::int64_t center =
          static_cast<std::int64_t>(grid[axis] * 2U + 1U) * kCenterUnit;
      const std::int64_t mantissa = center + bucket;
      if (mantissa <= 0 || mantissa >= (INT64_C(1) << 52U)) {
        throw std::runtime_error("multiscale mantissa construction overflowed");
      }
      bits[axis] = kOneExponent | static_cast<std::uint64_t>(mantissa);
    }
    points.push_back(
        morsehgp3d::exact::CertifiedPoint3::from_binary64_bits(bits));
  }
  return points;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3> make_cloud(
    Family family,
    std::size_t count,
    std::uint64_t seed) {
  switch (family) {
    case Family::affine_uniform_binary64:
      return make_affine_uniform_cloud(count, seed);
    case Family::jittered_dyadic_grid3d:
      return make_jittered_dyadic_cloud(count, seed);
    case Family::balanced_multiscale_clusters:
      return make_balanced_multiscale_cloud(count, seed);
  }
  throw std::logic_error("invalid family");
}

[[nodiscard]] double squared_distance(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    PointId first,
    PointId second) {
  const auto& left = cloud.point(first);
  const auto& right = cloud.point(second);
  const double dx = left.binary64_coordinate(0U) - right.binary64_coordinate(0U);
  const double dy = left.binary64_coordinate(1U) - right.binary64_coordinate(1U);
  const double dz = left.binary64_coordinate(2U) - right.binary64_coordinate(2U);
  return (dx * dx + dy * dy) + dz * dz;
}

[[nodiscard]] BaseEdge make_base_edge(
    PointId first,
    PointId second,
    double distance) {
  if (second < first) {
    std::swap(first, second);
  }
  if (first == second || !std::isfinite(distance) || distance < 0.0) {
    throw std::runtime_error("invalid common-topology edge");
  }
  if (first > std::numeric_limits<std::uint32_t>::max() ||
      second > std::numeric_limits<std::uint32_t>::max()) {
    throw std::runtime_error("the industrial pair key exceeds 32-bit PointIds");
  }
  const std::uint64_t pair_key =
      (static_cast<std::uint64_t>(first) << 32U) |
      static_cast<std::uint64_t>(second);
  return BaseEdge{pair_key, distance};
}

[[nodiscard]] PointId pair_first(std::uint64_t pair_key) noexcept {
  return static_cast<PointId>(pair_key >> 32U);
}

[[nodiscard]] PointId pair_second(std::uint64_t pair_key) noexcept {
  return static_cast<PointId>(pair_key & UINT64_C(0xffffffff));
}

[[nodiscard]] double lower_positive_binary64_by_ulps(
    double value,
    std::uint64_t ulps) {
  if (!std::isfinite(value) || value < 0.0) {
    throw std::runtime_error("cannot lower an invalid binary64 core distance");
  }
  if (value == 0.0 || ulps == 0U) {
    return value;
  }
  const std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
  const std::uint64_t lowered_bits = bits > ulps ? bits - ulps : 0U;
  return std::bit_cast<double>(lowered_bits);
}

[[noreturn]] void throw_component_bridge_budget_exhausted(
    std::string_view stop_reason,
    std::size_t actual,
    std::size_t budget) {
  std::ostringstream message;
  message << "component bridge budget exhausted: stop_reason=" << stop_reason
          << " actual=" << actual << " budget=" << budget;
  throw std::runtime_error(message.str());
}

[[nodiscard]] std::size_t checked_component_bridge_sum(
    std::size_t left,
    std::size_t right,
    std::string_view role) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw_component_bridge_budget_exhausted(
        role, std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max());
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_component_bridge_product(
    std::size_t left,
    std::size_t right,
    std::string_view role) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw_component_bridge_budget_exhausted(
        role, std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max());
  }
  return left * right;
}

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t count)
      : parent_(count), sizes_(count, 1U) {
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

struct TopKComponent {
  PointId minimum_point_id{};
  std::vector<PointId> members;
  std::array<double, kAxisCount> center{};
};

struct ComponentPair {
  std::size_t first{};
  std::size_t second{};

  friend bool operator==(const ComponentPair&, const ComponentPair&) = default;
};

struct ProjectedSupport {
  double projection{};
  PointId point_id{};
};

[[nodiscard]] bool component_pair_less(
    const ComponentPair& left,
    const ComponentPair& right) noexcept {
  return left.first < right.first ||
         (left.first == right.first && left.second < right.second);
}

[[nodiscard]] std::array<double, kAxisCount> point_coordinates(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    PointId point_id) {
  const auto& point = cloud.point(point_id);
  return {point.binary64_coordinate(0U), point.binary64_coordinate(1U),
          point.binary64_coordinate(2U)};
}

template <typename Better>
void retain_projected_support(
    std::array<ProjectedSupport, kComponentBridgeProjectionSupportCount>&
        supports,
    std::size_t& count,
    ProjectedSupport candidate,
    Better better) {
  if (count == supports.size()) {
    if (!better(candidate, supports[count - 1U])) {
      return;
    }
  } else {
    ++count;
  }
  std::size_t position = count - 1U;
  while (position != 0U && better(candidate, supports[position - 1U])) {
    supports[position] = supports[position - 1U];
    --position;
  }
  supports[position] = candidate;
}

void append_component_bridge_edges(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    std::span<const BaseEdge> top_k_edges,
    Topology& topology) {
  const std::size_t point_count = cloud.size();
  topology.component_bridge_member_projection_evaluation_budget =
      checked_component_bridge_product(
          point_count, kComponentBridgeMemberProjectionBudgetFactor,
          "member_projection_budget_overflow");
  topology.component_bridge_cross_distance_evaluation_budget =
      kComponentBridgeMaximumCrossDistanceEvaluationCount;
  DisjointSet point_components{point_count};
  for (const BaseEdge& edge : top_k_edges) {
    static_cast<void>(point_components.unite(
        static_cast<std::size_t>(pair_first(edge.pair_key)),
        static_cast<std::size_t>(pair_second(edge.pair_key))));
  }

  constexpr std::size_t kMissing = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> root_to_component(point_count, kMissing);
  std::vector<TopKComponent> components;
  components.reserve(point_count / (kMaximumOrder + 1U) + 1U);
  std::vector<std::array<double, kAxisCount>> coordinates(point_count);
  for (std::size_t point = 0U; point < point_count; ++point) {
    const PointId point_id = static_cast<PointId>(point);
    coordinates[point] = point_coordinates(cloud, point_id);
    const std::size_t root = point_components.find(point);
    std::size_t& component_index = root_to_component[root];
    if (component_index == kMissing) {
      component_index = components.size();
      components.push_back(TopKComponent{point_id, {}, {}});
    }
    components[component_index].members.push_back(point_id);
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      components[component_index].center[axis] += coordinates[point][axis];
    }
  }
  topology.top_k_component_count = components.size();
  if (components.size() > kComponentBridgeMaximumComponentCount) {
    throw_component_bridge_budget_exhausted(
        "component_count_limit", components.size(),
        kComponentBridgeMaximumComponentCount);
  }
  for (TopKComponent& component : components) {
    const double denominator = static_cast<double>(component.members.size());
    for (double& coordinate : component.center) {
      coordinate /= denominator;
      if (!std::isfinite(coordinate)) {
        throw_component_bridge_budget_exhausted(
            "non_finite_component_centroid", components.size(),
            kComponentBridgeMaximumComponentCount);
      }
    }
  }
  if (components.size() <= 1U) {
    topology.component_bridge_budget_satisfied = true;
    topology.component_bridge_budget_stop_reason = "none";
    return;
  }

  std::vector<ComponentPair> pairs;
  pairs.reserve(checked_component_bridge_product(
      components.size(), kComponentBridgeNeighborCount,
      "component_pair_capacity_overflow"));
  for (std::size_t source = 0U; source < components.size(); ++source) {
    std::vector<std::pair<double, std::size_t>> neighbors;
    neighbors.reserve(components.size() - 1U);
    for (std::size_t target = 0U; target < components.size(); ++target) {
      if (target == source) {
        continue;
      }
      double distance{};
      for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
        const double delta =
            components[source].center[axis] - components[target].center[axis];
        distance += delta * delta;
      }
      topology.component_bridge_centroid_distance_evaluation_count =
          checked_component_bridge_sum(
              topology.component_bridge_centroid_distance_evaluation_count,
              1U, "centroid_distance_evaluation_count_overflow");
      if (!std::isfinite(distance) || distance < 0.0) {
        throw_component_bridge_budget_exhausted(
            "non_finite_centroid_distance",
            topology.component_bridge_centroid_distance_evaluation_count,
            kComponentBridgeMaximumCentroidDistanceEvaluationCount);
      }
      neighbors.emplace_back(distance, target);
    }
    std::sort(
        neighbors.begin(), neighbors.end(),
        [&components](const auto& left, const auto& right) {
          if (left.first != right.first) {
            return left.first < right.first;
          }
          return components[left.second].minimum_point_id <
                 components[right.second].minimum_point_id;
        });
    const std::size_t retained = std::min(
        kComponentBridgeNeighborCount, neighbors.size());
    for (std::size_t index = 0U; index < retained; ++index) {
      const std::size_t target = neighbors[index].second;
      pairs.push_back(ComponentPair{
          std::min(source, target), std::max(source, target)});
    }
  }
  std::sort(pairs.begin(), pairs.end(), component_pair_less);
  pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
  topology.component_bridge_pair_count = pairs.size();
  if (topology.component_bridge_centroid_distance_evaluation_count >
      kComponentBridgeMaximumCentroidDistanceEvaluationCount) {
    throw_component_bridge_budget_exhausted(
        "centroid_distance_evaluation_limit",
        topology.component_bridge_centroid_distance_evaluation_count,
        kComponentBridgeMaximumCentroidDistanceEvaluationCount);
  }
  if (pairs.size() > kComponentBridgeMaximumPairCount) {
    throw_component_bridge_budget_exhausted(
        "component_pair_count_limit", pairs.size(),
        kComponentBridgeMaximumPairCount);
  }

  std::size_t member_projection_evaluation_count{};
  std::size_t cross_distance_evaluation_count{};
  for (const ComponentPair& pair : pairs) {
    const std::size_t pair_member_count = checked_component_bridge_sum(
        components[pair.first].members.size(),
        components[pair.second].members.size(),
        "member_projection_pair_count_overflow");
    member_projection_evaluation_count = checked_component_bridge_sum(
        member_projection_evaluation_count, pair_member_count,
        "member_projection_evaluation_count_overflow");
    const std::size_t pair_cross_count = checked_component_bridge_product(
        std::min(kComponentBridgeProjectionSupportCount,
                 components[pair.first].members.size()),
        std::min(kComponentBridgeProjectionSupportCount,
                 components[pair.second].members.size()),
        "cross_distance_pair_count_overflow");
    cross_distance_evaluation_count = checked_component_bridge_sum(
        cross_distance_evaluation_count, pair_cross_count,
        "cross_distance_evaluation_count_overflow");
  }
  topology.component_bridge_member_projection_evaluation_count =
      member_projection_evaluation_count;
  topology.component_bridge_cross_distance_evaluation_count =
      cross_distance_evaluation_count;
  topology.component_bridge_support_evaluation_count =
      checked_component_bridge_sum(
          member_projection_evaluation_count,
          cross_distance_evaluation_count,
          "support_evaluation_count_overflow");
  if (member_projection_evaluation_count >
      topology.component_bridge_member_projection_evaluation_budget) {
    throw_component_bridge_budget_exhausted(
        "member_projection_evaluation_limit",
        member_projection_evaluation_count,
        topology.component_bridge_member_projection_evaluation_budget);
  }
  if (cross_distance_evaluation_count >
      topology.component_bridge_cross_distance_evaluation_budget) {
    throw_component_bridge_budget_exhausted(
        "cross_distance_evaluation_limit", cross_distance_evaluation_count,
        topology.component_bridge_cross_distance_evaluation_budget);
  }
  topology.component_bridge_budget_satisfied = true;
  topology.component_bridge_budget_stop_reason = "none";

  DisjointSet component_graph{components.size()};
  for (const ComponentPair& pair : pairs) {
    static_cast<void>(component_graph.unite(pair.first, pair.second));
  }
  const std::size_t component_root = component_graph.find(0U);
  for (std::size_t component = 1U; component < components.size(); ++component) {
    if (component_graph.find(component) != component_root) {
      throw std::runtime_error(
          "the sparse component-bridge candidate graph is disconnected");
    }
  }

  std::vector<BaseEdge> bridges(pairs.size());
  std::atomic<std::size_t> next_pair{0U};
  const unsigned int hardware = std::thread::hardware_concurrency();
  const std::size_t worker_count = std::min(
      pairs.size(),
      hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware));
  std::vector<std::jthread> workers;
  workers.reserve(worker_count);
  for (std::size_t worker = 0U; worker < worker_count; ++worker) {
    workers.emplace_back([&] {
      while (true) {
        const std::size_t index =
            next_pair.fetch_add(1U, std::memory_order_relaxed);
        if (index >= pairs.size()) {
          break;
        }
        const ComponentPair pair = pairs[index];
        const TopKComponent& left = components[pair.first];
        const TopKComponent& right = components[pair.second];
        std::array<double, kAxisCount> direction{};
        for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
          direction[axis] = right.center[axis] - left.center[axis];
        }
        std::array<ProjectedSupport,
                   kComponentBridgeProjectionSupportCount>
            left_supports{};
        std::size_t left_support_count{};
        const auto left_better = [](const ProjectedSupport& first,
                                    const ProjectedSupport& second) {
          return first.projection > second.projection ||
                 (first.projection == second.projection &&
                  first.point_id < second.point_id);
        };
        for (const PointId point_id : left.members) {
          double projection{};
          for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
            projection += coordinates[static_cast<std::size_t>(point_id)][axis] *
                          direction[axis];
          }
          retain_projected_support(
              left_supports, left_support_count,
              ProjectedSupport{projection, point_id}, left_better);
        }
        std::array<ProjectedSupport,
                   kComponentBridgeProjectionSupportCount>
            right_supports{};
        std::size_t right_support_count{};
        const auto right_better = [](const ProjectedSupport& first,
                                     const ProjectedSupport& second) {
          return first.projection < second.projection ||
                 (first.projection == second.projection &&
                  first.point_id < second.point_id);
        };
        for (const PointId point_id : right.members) {
          double projection{};
          for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
            projection += coordinates[static_cast<std::size_t>(point_id)][axis] *
                          direction[axis];
          }
          retain_projected_support(
              right_supports, right_support_count,
              ProjectedSupport{projection, point_id}, right_better);
        }
        BaseEdge best_bridge{
            std::numeric_limits<std::uint64_t>::max(),
            std::numeric_limits<double>::infinity()};
        for (std::size_t left_index = 0U;
             left_index < left_support_count; ++left_index) {
          for (std::size_t right_index = 0U;
               right_index < right_support_count; ++right_index) {
            const PointId left_support = left_supports[left_index].point_id;
            const PointId right_support = right_supports[right_index].point_id;
            const BaseEdge candidate = make_base_edge(
                left_support, right_support,
                squared_distance(cloud, left_support, right_support));
            if (candidate.squared_distance < best_bridge.squared_distance ||
                (candidate.squared_distance == best_bridge.squared_distance &&
                 candidate.pair_key < best_bridge.pair_key)) {
              best_bridge = candidate;
            }
          }
        }
        bridges[index] = best_bridge;
      }
    });
  }
  for (std::jthread& worker : workers) {
    worker.join();
  }
  topology.edges.insert(topology.edges.end(), bridges.begin(), bridges.end());
}

[[nodiscard]] Topology build_topology(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    const morsehgp3d::gpu::Binary64LbvhTopKResult& top_k,
    std::size_t maximum_order) {
  const std::size_t point_count = cloud.size();
  if (!top_k.validated_complete_binary64_transcript() ||
      top_k.audit.point_count != point_count ||
      top_k.audit.maximum_order != maximum_order) {
    throw std::runtime_error("the top-k transcript is not complete");
  }
  Topology result;
  result.edges.reserve(point_count * maximum_order + point_count - 1U);
  result.core_squared_distances.resize(point_count * maximum_order);
  for (std::size_t position = 0U; position < point_count; ++position) {
    const PointId source = top_k.source_point_ids_by_morton_position[position];
    const std::size_t source_index = static_cast<std::size_t>(source);
    const std::size_t row = position * maximum_order;
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      const PointId neighbor = top_k.neighbor_point_ids[row + rank];
      const double distance = top_k.squared_distances[row + rank];
      result.core_squared_distances[rank * point_count + source_index] =
          distance;
      result.edges.push_back(make_base_edge(source, neighbor, distance));
    }
  }
  const std::size_t top_k_edge_count = result.edges.size();
  append_component_bridge_edges(
      cloud,
      std::span<const BaseEdge>{result.edges.data(), top_k_edge_count}, result);
  for (std::size_t position = 1U; position < point_count; ++position) {
    const PointId source = top_k.source_point_ids_by_morton_position[position];
    const PointId previous =
        top_k.source_point_ids_by_morton_position[position - 1U];
    result.edges.push_back(make_base_edge(
        previous, source, squared_distance(cloud, previous, source)));
  }
  std::sort(
      result.edges.begin(), result.edges.end(),
      [](const BaseEdge& left, const BaseEdge& right) {
        return left.pair_key < right.pair_key;
      });
  std::size_t write{};
  for (const BaseEdge& edge : result.edges) {
    if (write != 0U &&
        result.edges[write - 1U].pair_key == edge.pair_key) {
      result.edges[write - 1U].squared_distance =
          std::min(result.edges[write - 1U].squared_distance,
                   edge.squared_distance);
    } else {
      result.edges[write++] = edge;
    }
  }
  result.edges.resize(write);
  if (result.edges.size() < point_count - 1U) {
    throw std::runtime_error("the common topology cannot span the cloud");
  }
  return result;
}

[[nodiscard]] OrderReduction reduce_order(
    const Topology& topology,
    std::size_t point_count,
    std::size_t order_index) {
  std::vector<WeightedEdge> weighted;
  weighted.reserve(topology.edges.size());
  const std::size_t core_offset = order_index * point_count;
  for (const BaseEdge& edge : topology.edges) {
    const PointId first = pair_first(edge.pair_key);
    const PointId second = pair_second(edge.pair_key);
    double first_core = topology.core_squared_distances[
        core_offset + static_cast<std::size_t>(first)];
    double second_core = topology.core_squared_distances[
        core_offset + static_cast<std::size_t>(second)];
    if (order_index != 0U) {
      first_core = lower_positive_binary64_by_ulps(
          first_core, kCoreDeadlineLowerEnvelopeUlps);
      second_core = lower_positive_binary64_by_ulps(
          second_core, kCoreDeadlineLowerEnvelopeUlps);
    }
    const double weight = std::max(
        edge.squared_distance,
        std::max(first_core, second_core));
    weighted.push_back(WeightedEdge{weight, edge.pair_key});
  }
  std::sort(
      weighted.begin(), weighted.end(),
      [](const WeightedEdge& left, const WeightedEdge& right) {
        if (left.squared_weight != right.squared_weight) {
          return left.squared_weight < right.squared_weight;
        }
        return left.pair_key < right.pair_key;
      });
  OrderReduction reduction;
  reduction.order = order_index + 1U;
  reduction.merges.reserve(point_count - 1U);
  DisjointSet components{point_count};
  for (const WeightedEdge& edge : weighted) {
    const PointId first = pair_first(edge.pair_key);
    const PointId second = pair_second(edge.pair_key);
    if (components.unite(
            static_cast<std::size_t>(first),
            static_cast<std::size_t>(second))) {
      reduction.merges.push_back(
          MergeRecord{first, second, edge.squared_weight});
      if (reduction.merges.size() == point_count - 1U) {
        break;
      }
    }
  }
  if (reduction.merges.size() != point_count - 1U) {
    throw std::runtime_error("Kruskal did not materialize a spanning tree");
  }
  reduction.merge_record_count = reduction.merges.size();
  return reduction;
}

[[nodiscard]] std::vector<OrderReduction> reduce_all_orders(
    const Topology& topology,
    std::size_t point_count,
    std::size_t maximum_order) {
  std::vector<OrderReduction> reductions(maximum_order);
  std::atomic<std::size_t> next_order{0U};
  std::exception_ptr failure;
  std::mutex failure_mutex;
  const unsigned int hardware = std::thread::hardware_concurrency();
  const std::size_t worker_count = std::min(
      maximum_order,
      hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware));
  std::vector<std::thread> workers;
  workers.reserve(worker_count);
  for (std::size_t worker = 0U; worker < worker_count; ++worker) {
    workers.emplace_back([&] {
      try {
        while (true) {
          const std::size_t order =
              next_order.fetch_add(1U, std::memory_order_relaxed);
          if (order >= maximum_order) {
            break;
          }
          reductions[order] = reduce_order(topology, point_count, order);
        }
      } catch (...) {
        std::lock_guard<std::mutex> lock{failure_mutex};
        if (failure == nullptr) {
          failure = std::current_exception();
        }
      }
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
  if (failure != nullptr) {
    std::rethrow_exception(failure);
  }
  return reductions;
}

void digest_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  constexpr std::uint64_t kPrime = UINT64_C(1099511628211);
  for (unsigned int byte = 0U; byte < 8U; ++byte) {
    digest ^= (word >> (byte * 8U)) & UINT64_C(0xff);
    digest *= kPrime;
  }
}

void materialize_output(std::vector<OrderReduction>& reductions) {
  for (const OrderReduction& reduction : reductions) {
    if (reduction.merge_record_count == 0U ||
        reduction.merge_record_count != reduction.merges.size()) {
      throw std::logic_error(
          "an order reduction lost its materialized merge count");
    }
  }
  std::atomic<std::size_t> next_order{0U};
  const unsigned int hardware = std::thread::hardware_concurrency();
  const std::size_t worker_count = std::min(
      reductions.size(),
      hardware == 0U ? std::size_t{1} : static_cast<std::size_t>(hardware));
  std::vector<std::jthread> workers;
  workers.reserve(worker_count);
  for (std::size_t worker = 0U; worker < worker_count; ++worker) {
    workers.emplace_back([&] {
      while (true) {
        const std::size_t order =
            next_order.fetch_add(1U, std::memory_order_relaxed);
        if (order >= reductions.size()) {
          break;
        }
        OrderReduction& reduction = reductions[order];
        std::uint64_t digest = UINT64_C(1469598103934665603);
        digest_word(digest, static_cast<std::uint64_t>(reduction.order));
        for (const MergeRecord& merge : reduction.merges) {
          digest_word(digest, merge.first);
          digest_word(digest, merge.second);
          digest_word(
              digest, std::bit_cast<std::uint64_t>(merge.squared_level));
        }
        reduction.digest = digest;
        reduction.root_squared_level = reduction.merges.back().squared_level;
      }
    });
  }
}

[[nodiscard]] RunMetrics run_pipeline(
    const morsehgp3d::spatial::CanonicalPointCloud& cloud,
    morsehgp3d::gpu::MortonLbvhBuildContext& builder,
    const Options& options,
    std::uint64_t seed,
    std::size_t run_index,
    bool warmup,
    std::optional<std::size_t> measured_index,
    Clock::time_point e2e_start) {
  RunMetrics metrics;
  metrics.run_index = run_index;
  metrics.measured_index = measured_index;
  metrics.warmup = warmup;
  metrics.seed = seed;
  const auto lbvh_start = Clock::now();
  std::optional<morsehgp3d::gpu::MortonLbvhDeviceTraversalLease> lease;
  {
    morsehgp3d::gpu::MortonLbvhDeviceBuildResult build = builder.build(cloud);
    if (!build.complete_certified_build() || !build.cuda_qualified_build() ||
        !build.certified_index().validated_for(cloud)) {
      throw std::runtime_error("the fresh-cloud LBVH build is not CUDA-qualified");
    }
    metrics.lbvh_kernel_launch_count = build.audit().device_kernel_launch_count;
    lease.emplace(builder.release_device_traversal_lease(build));
    if (!lease->ready() || !lease->cuda_resident()) {
      throw std::runtime_error("the traversal lease is not CUDA resident");
    }
  }
  metrics.lbvh_ns = nanoseconds(Clock::now() - lbvh_start);

  const auto top_k_start = Clock::now();
  morsehgp3d::gpu::Binary64LbvhTopKResult top_k;
  {
    morsehgp3d::gpu::Binary64LbvhTopKContext context{
        cloud, std::move(*lease)};
    lease.reset();
    top_k = context.query_all(
        cloud, options.maximum_order, options.seed_window);
  }
  metrics.top_k_ns = nanoseconds(Clock::now() - top_k_start);
  metrics.neighbor_record_count = top_k.audit.neighbor_record_count;
  metrics.top_k_node_visit_count = top_k.audit.node_visit_count;
  metrics.top_k_strict_prune_count = top_k.audit.strict_aabb_prune_count;
  metrics.device_name = top_k.audit.device_name;

  const auto topology_start = Clock::now();
  Topology topology = build_topology(cloud, top_k, options.maximum_order);
  metrics.common_topology_edge_count = topology.edges.size();
  metrics.top_k_component_count = topology.top_k_component_count;
  metrics.component_bridge_pair_count = topology.component_bridge_pair_count;
  metrics.component_bridge_support_evaluation_count =
      topology.component_bridge_support_evaluation_count;
  metrics.component_bridge_centroid_distance_evaluation_count =
      topology.component_bridge_centroid_distance_evaluation_count;
  metrics.component_bridge_member_projection_evaluation_budget =
      topology.component_bridge_member_projection_evaluation_budget;
  metrics.component_bridge_member_projection_evaluation_count =
      topology.component_bridge_member_projection_evaluation_count;
  metrics.component_bridge_cross_distance_evaluation_budget =
      topology.component_bridge_cross_distance_evaluation_budget;
  metrics.component_bridge_cross_distance_evaluation_count =
      topology.component_bridge_cross_distance_evaluation_count;
  metrics.component_bridge_budget_satisfied =
      topology.component_bridge_budget_satisfied;
  metrics.component_bridge_budget_stop_reason =
      topology.component_bridge_budget_stop_reason;
  metrics.topology_ns = nanoseconds(Clock::now() - topology_start);

  const auto reductions_start = Clock::now();
  metrics.reductions =
      reduce_all_orders(topology, cloud.size(), options.maximum_order);
  metrics.reductions_ns = nanoseconds(Clock::now() - reductions_start);

  const auto materialization_start = Clock::now();
  materialize_output(metrics.reductions);
  for (const OrderReduction& reduction : metrics.reductions) {
    if (reduction.merge_record_count >
        std::numeric_limits<std::size_t>::max() -
            metrics.materialized_merge_record_count) {
      throw std::length_error(
          "the materialized merge record count overflows size_t");
    }
    metrics.materialized_merge_record_count += reduction.merge_record_count;
  }
  metrics.retained_merge_record_count =
      metrics.materialized_merge_record_count;
  metrics.output_materialization_ns =
      nanoseconds(Clock::now() - materialization_start);
  metrics.warm_e2e_ns = nanoseconds(Clock::now() - e2e_start);
  return metrics;
}

void write_bytes(std::ostream& output, std::string_view bytes) {
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void write_u32_le(std::ostream& output, std::uint32_t value) {
  for (unsigned int byte = 0U; byte < 4U; ++byte) {
    output.put(static_cast<char>((value >> (byte * 8U)) & UINT32_C(0xff)));
  }
}

void write_u64_le(std::ostream& output, std::uint64_t value) {
  for (unsigned int byte = 0U; byte < 8U; ++byte) {
    output.put(static_cast<char>((value >> (byte * 8U)) & UINT64_C(0xff)));
  }
}

[[nodiscard]] std::filesystem::path export_path_for(
    const Options& options,
    std::size_t measured_index,
    std::uint64_t seed) {
  const std::filesystem::path& base = *options.emit_tree_path;
  if (options.repetitions == 1U) {
    return base;
  }
  const std::string extension =
      base.has_extension() ? base.extension().string() : ".bin";
  return base.parent_path() /
         (base.stem().string() + ".run" + std::to_string(measured_index) +
          ".seed" + std::to_string(seed) + extension);
}

void export_trees(
    const std::filesystem::path& path,
    std::size_t point_count,
    std::uint64_t seed,
    std::size_t maximum_export_order,
    const std::vector<OrderReduction>& reductions) {
  if (maximum_export_order == 0U ||
      maximum_export_order > reductions.size()) {
    throw std::invalid_argument(
        "the tree export order interval must be a nonempty prefix");
  }
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output) {
    throw std::runtime_error("cannot open --emit-tree-path");
  }
  write_bytes(output, "MHGPIE2E");
  write_u32_le(output, 1U);
  write_u32_le(output, UINT32_C(0x01020304));
  write_u64_le(output, static_cast<std::uint64_t>(point_count));
  write_u64_le(output, seed);
  write_u64_le(output, static_cast<std::uint64_t>(maximum_export_order));
  for (std::size_t order_index = 0U;
       order_index < maximum_export_order; ++order_index) {
    const OrderReduction& reduction = reductions[order_index];
    if (reduction.order != order_index + 1U ||
        reduction.merge_record_count != reduction.merges.size()) {
      throw std::logic_error(
          "the tree export is not a complete order prefix");
    }
    write_bytes(output, "H0TREE01");
    write_u32_le(output, 1U);
    write_u32_le(output, 0U);
    write_u64_le(output, static_cast<std::uint64_t>(point_count));
    write_u64_le(output, static_cast<std::uint64_t>(reduction.order));
    write_u64_le(output, seed);
    write_u64_le(
        output, static_cast<std::uint64_t>(reduction.merge_record_count));
    for (const MergeRecord& merge : reduction.merges) {
      write_u64_le(output, merge.first);
      write_u64_le(output, merge.second);
      write_u64_le(
          output, std::bit_cast<std::uint64_t>(merge.squared_level));
    }
  }
  if (!output) {
    throw std::runtime_error("the canonical tree export failed");
  }
}

[[nodiscard]] std::size_t prefix_merge_record_count(
    const std::vector<OrderReduction>& reductions,
    std::size_t maximum_order) {
  if (maximum_order > reductions.size()) {
    throw std::invalid_argument(
        "the merge count prefix exceeds the materialized orders");
  }
  std::size_t count{};
  for (std::size_t order = 0U; order < maximum_order; ++order) {
    if (reductions[order].order != order + 1U ||
        reductions[order].merge_record_count >
            std::numeric_limits<std::size_t>::max() - count) {
      throw std::length_error(
          "the merge count prefix is invalid or overflows size_t");
    }
    count += reductions[order].merge_record_count;
  }
  return count;
}

void release_merge_records(RunMetrics& run) {
  std::size_t released{};
  for (OrderReduction& reduction : run.reductions) {
    if (reduction.merge_record_count != reduction.merges.size() ||
        reduction.merge_record_count >
            std::numeric_limits<std::size_t>::max() - released) {
      throw std::logic_error(
          "merge records changed before their guarded release");
    }
    released += reduction.merge_record_count;
  }
  if (released != run.materialized_merge_record_count ||
      released != run.retained_merge_record_count) {
    throw std::logic_error(
        "the guarded merge release disagrees with materialization");
  }
  for (OrderReduction& reduction : run.reductions) {
    std::vector<MergeRecord>{}.swap(reduction.merges);
  }
  run.released_merge_record_count = released;
  run.retained_merge_record_count = 0U;
  run.merge_records_released_after_digest_and_export = true;
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
  std::ostringstream output;
  output << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return output.str();
}

void write_json_string(std::ostream& output, std::string_view value) {
  output.put('"');
  for (const char character : value) {
    const auto byte = static_cast<unsigned char>(character);
    switch (byte) {
      case '"':
        output << "\\\"";
        break;
      case '\\':
        output << "\\\\";
        break;
      case '\b':
        output << "\\b";
        break;
      case '\f':
        output << "\\f";
        break;
      case '\n':
        output << "\\n";
        break;
      case '\r':
        output << "\\r";
        break;
      case '\t':
        output << "\\t";
        break;
      default:
        if (byte < 0x20U) {
          output << "\\u00" << std::hex << std::setw(2)
                 << std::setfill('0') << static_cast<unsigned int>(byte)
                 << std::dec;
        } else {
          output.put(static_cast<char>(byte));
        }
    }
  }
  output.put('"');
}

[[nodiscard]] std::uint64_t nearest_rank(
    std::vector<std::uint64_t> values,
    std::size_t percentile) {
  if (values.empty() || percentile == 0U || percentile > 100U) {
    throw std::invalid_argument("invalid nearest-rank request");
  }
  std::sort(values.begin(), values.end());
  const std::size_t rank =
      (percentile * values.size() + 99U) / 100U;
  return values[rank - 1U];
}

void write_result_json(
    const Options& options,
    const std::vector<RunMetrics>& runs,
    const RunnerBinaryIdentity& runner_binary,
    std::uint64_t p50,
    std::uint64_t p95,
    std::uint64_t p99) {
  std::cout << std::setprecision(17);
  std::cout << "{\"schema\":\"morsehgp3d.phase15.point_mst_mutual_reachability_surrogate.v6\""
            << ",\"result_kind\":\"point_mst_mutual_reachability_surrogate_benchmark\""
            << ",\"phase\":15,\"backend\":\"cuda_g4_plus_host_binary64\""
            << ",\"profile\":\"point_mst_mutual_reachability_surrogate\""
            << ",\"mode\":\"warm_fresh_cloud_lbvh_top10_capped_component_bridges_parallel_point_h0_surrogate\""
            << ",\"deployment_status\":\"diagnostic_surrogate_only\""
            << ",\"public_status\":\"not_a_morse_hgp_product_result\",\"git_sha\":";
  write_json_string(std::cout, MORSEHGP3D_GIT_SHA);
  std::cout << ",\"runner_binary_sha256\":";
  write_json_string(std::cout, runner_binary.sha256);
  std::cout << ",\"runner_binary_size_bytes\":"
            << runner_binary.size_bytes << ",\"family\":";
  write_json_string(std::cout, family_name(options.family));
  std::cout << ",\"point_count\":" << options.point_count
            << ",\"repetitions\":" << options.repetitions
            << ",\"warmups\":" << options.warmups
            << ",\"maximum_order\":" << options.maximum_order
            << ",\"seed_window\":" << options.seed_window
            << ",\"export_maximum_order\":"
            << options.export_maximum_order
            << ",\"cloud_generation_timed\":false"
            << ",\"canonicalization_timed\":true"
            << ",\"fresh_cloud_per_run\":true"
            << ",\"surrogate_pipeline_complete\":true"
            << ",\"point_vertex_universe_only\":true"
            << ",\"ten_point_spanning_tree_surrogates_materialized\":true"
            << ",\"morse_hgp_simplex_components_materialized\":false"
            << ",\"weight_representation\":\"squared_binary64_mutual_reachability_with_k2plus_core_lower_envelope\""
            << ",\"core_deadline_lower_envelope_ulps\":"
            << kCoreDeadlineLowerEnvelopeUlps
            << ",\"component_bridge_neighbor_count\":"
            << kComponentBridgeNeighborCount
            << ",\"component_bridge_maximum_component_count\":"
            << kComponentBridgeMaximumComponentCount
            << ",\"component_bridge_maximum_centroid_distance_evaluation_count\":"
            << kComponentBridgeMaximumCentroidDistanceEvaluationCount
            << ",\"component_bridge_member_projection_budget_factor\":"
            << kComponentBridgeMemberProjectionBudgetFactor
            << ",\"component_bridge_maximum_pair_count\":"
            << kComponentBridgeMaximumPairCount
            << ",\"component_bridge_maximum_cross_distance_evaluation_count\":"
            << kComponentBridgeMaximumCrossDistanceEvaluationCount
            << ",\"component_bridge_policy\":\"top10_components_capped_centroid_knn_top8_projection_cross_min\""
            << ",\"ordinary_delaunay_materialized\":false"
            << ",\"higher_order_delaunay_mosaic_materialized\":false"
            << ",\"global_pair_matrix_materialized\":false"
            << ",\"exact_morse_hierarchy_claimed\":false"
            << ",\"true_hgp_campaign_eligible\":false"
            << ",\"oracle_exports_outside_timed_region\":true"
            << ",\"runs\":[";
  for (std::size_t index = 0U; index < runs.size(); ++index) {
    if (index != 0U) {
      std::cout.put(',');
    }
    const RunMetrics& run = runs[index];
    std::cout << "{\"run_index\":" << run.run_index
              << ",\"measured_index\":";
    if (run.measured_index.has_value()) {
      std::cout << *run.measured_index;
    } else {
      std::cout << "null";
    }
    std::cout << ",\"warmup\":" << (run.warmup ? "true" : "false")
              << ",\"seed\":" << run.seed
              << ",\"fresh_cloud\":true"
              << ",\"generation_ns\":" << run.generation_ns
              << ",\"canonicalization_ns\":" << run.canonicalization_ns
              << ",\"timings_ns\":{\"lbvh\":" << run.lbvh_ns
              << ",\"topk\":" << run.top_k_ns
              << ",\"topology\":" << run.topology_ns
              << ",\"reductions\":" << run.reductions_ns
              << ",\"output_materialization\":"
              << run.output_materialization_ns
              << ",\"warm_e2e\":" << run.warm_e2e_ns << '}'
              << ",\"common_topology_edge_count\":"
              << run.common_topology_edge_count
              << ",\"top_k_component_count\":"
              << run.top_k_component_count
              << ",\"component_bridge_pair_count\":"
              << run.component_bridge_pair_count
              << ",\"component_bridge_support_evaluation_count\":"
              << run.component_bridge_support_evaluation_count
              << ",\"component_bridge_centroid_distance_evaluation_count\":"
              << run.component_bridge_centroid_distance_evaluation_count
              << ",\"component_bridge_member_projection_evaluation_budget\":"
              << run.component_bridge_member_projection_evaluation_budget
              << ",\"component_bridge_member_projection_evaluation_count\":"
              << run.component_bridge_member_projection_evaluation_count
              << ",\"component_bridge_cross_distance_evaluation_budget\":"
              << run.component_bridge_cross_distance_evaluation_budget
              << ",\"component_bridge_cross_distance_evaluation_count\":"
              << run.component_bridge_cross_distance_evaluation_count
              << ",\"component_bridge_budget_satisfied\":"
              << (run.component_bridge_budget_satisfied ? "true" : "false")
              << ",\"component_bridge_budget_stop_reason\":";
    write_json_string(std::cout, run.component_bridge_budget_stop_reason);
    std::cout << ",\"materialized_surrogate_edge_record_count\":"
              << run.materialized_merge_record_count
              << ",\"released_surrogate_edge_record_count\":"
              << run.released_merge_record_count
              << ",\"retained_surrogate_edge_record_count\":"
              << run.retained_merge_record_count
              << ",\"surrogate_edge_records_released_after_digest_and_export\":"
              << (run.merge_records_released_after_digest_and_export
                      ? "true"
                      : "false")
              << ",\"exported_surrogate_order_count\":"
              << run.exported_order_count
              << ",\"exported_surrogate_edge_record_count\":"
              << run.exported_merge_record_count
              << ",\"neighbor_record_count\":"
              << run.neighbor_record_count
              << ",\"lbvh_kernel_launch_count\":"
              << run.lbvh_kernel_launch_count
              << ",\"top_k_node_visit_count\":"
              << run.top_k_node_visit_count
              << ",\"top_k_strict_prune_count\":"
              << run.top_k_strict_prune_count
              << ",\"device_name\":";
    write_json_string(std::cout, run.device_name);
    std::cout << ",\"oracle_export_ns\":" << run.oracle_export_ns
              << ",\"oracle_export_path\":";
    if (run.oracle_export_path.has_value()) {
      write_json_string(std::cout, *run.oracle_export_path);
    } else {
      std::cout << "null";
    }
    std::cout << ",\"point_spanning_tree_surrogates\":[";
    for (std::size_t order = 0U; order < run.reductions.size(); ++order) {
      if (order != 0U) {
        std::cout.put(',');
      }
      const OrderReduction& reduction = run.reductions[order];
      std::cout << "{\"neighbor_rank\":" << reduction.order
                << ",\"edge_count\":"
                << reduction.merge_record_count
                << ",\"digest\":";
      write_json_string(std::cout, hex64(reduction.digest));
      std::cout << ",\"root_squared_level\":"
                << reduction.root_squared_level << '}';
    }
    std::cout << "]}";
  }
  std::cout << "]"
            << ",\"nearest_rank_warm_e2e_ns\":{\"p50\":" << p50
            << ",\"p95\":" << p95 << ",\"p99\":" << p99 << '}'
            << ",\"slo\":{\"point_count\":50000,\"threshold_ns\":"
            << kSloNanoseconds
            << ",\"statistic\":\"true_morse_hgp_p95_end_to_end\""
            << ",\"scope\":\"true_morse_hgp_hierarchy\""
            << ",\"applicable\":false,\"passed\":null"
            << ",\"reason\":\"point_tree_surrogate_does_not_materialize_morse_hgp_simplex_components\"}"
            << ",\"result\":\"surrogate_benchmark_completed\"}\n";
}

[[nodiscard]] std::vector<std::uint64_t> make_run_seeds(
    const Options& options) {
  std::vector<std::uint64_t> seeds;
  seeds.reserve(options.warmups + options.repetitions);
  std::unordered_set<std::uint64_t> used;
  for (std::size_t warmup = 0U; warmup < options.warmups; ++warmup) {
    std::uint64_t seed = splitmix64(
        options.seed_base ^ UINT64_C(0xa0761d6478bd642f) ^
        static_cast<std::uint64_t>(warmup));
    while (!used.insert(seed).second) {
      seed = splitmix64(seed);
    }
    seeds.push_back(seed);
  }
  for (std::size_t repetition = 0U;
       repetition < options.repetitions;
       ++repetition) {
    std::uint64_t seed =
        options.seed_base + static_cast<std::uint64_t>(repetition);
    while (!used.insert(seed).second) {
      seed = splitmix64(seed);
    }
    seeds.push_back(seed);
  }
  return seeds;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const RunnerBinaryIdentity runner_binary =
        hash_running_executable(argc > 0 ? argv[0] : nullptr);
    const Options options = parse_options(argc, argv);
    const std::vector<std::uint64_t> seeds = make_run_seeds(options);
    morsehgp3d::gpu::MortonLbvhBuildContext builder{options.point_count};
    std::vector<RunMetrics> runs;
    runs.reserve(seeds.size());
    std::vector<std::uint64_t> measured_e2e;
    measured_e2e.reserve(options.repetitions);
    for (std::size_t index = 0U; index < seeds.size(); ++index) {
      const bool warmup = index < options.warmups;
      const std::optional<std::size_t> measured_index =
          warmup ? std::nullopt
                 : std::optional<std::size_t>{index - options.warmups};
      const auto generation_start = Clock::now();
      std::vector<morsehgp3d::exact::CertifiedPoint3> input =
          make_cloud(options.family, options.point_count, seeds[index]);
      const std::uint64_t generation_ns =
          nanoseconds(Clock::now() - generation_start);
      const auto e2e_start = Clock::now();
      const auto canonicalization_start = e2e_start;
      morsehgp3d::spatial::CanonicalPointCloud cloud =
          morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(input);
      const std::uint64_t canonicalization_ns =
          nanoseconds(Clock::now() - canonicalization_start);
      input.clear();
      input.shrink_to_fit();
      RunMetrics run = run_pipeline(
          cloud, builder, options, seeds[index], index, warmup, measured_index,
          e2e_start);
      run.generation_ns = generation_ns;
      run.canonicalization_ns = canonicalization_ns;
      if (!warmup) {
        measured_e2e.push_back(run.warm_e2e_ns);
        if (options.emit_tree_path.has_value()) {
          const std::filesystem::path path = export_path_for(
              options, *measured_index, seeds[index]);
          const auto export_start = Clock::now();
          export_trees(
              path, options.point_count, seeds[index],
              options.export_maximum_order, run.reductions);
          run.oracle_export_ns = nanoseconds(Clock::now() - export_start);
          run.oracle_export_path = path.string();
          run.exported_order_count = options.export_maximum_order;
          run.exported_merge_record_count = prefix_merge_record_count(
              run.reductions, options.export_maximum_order);
        }
      }
      release_merge_records(run);
      runs.push_back(std::move(run));
    }
    const std::uint64_t p50 = nearest_rank(measured_e2e, 50U);
    const std::uint64_t p95 = nearest_rank(measured_e2e, 95U);
    const std::uint64_t p99 = nearest_rank(measured_e2e, 99U);
    write_result_json(options, runs, runner_binary, p50, p95, p99);
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "gpu_point_mst_mutual_reachability_surrogate: "
              << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
