// Device parity harness of the Phase 15 higher-support device tiled
// frontier (M5).  The native CUDA seam is compared, chunk by chunk and
// without any field mask, against the host-compiled slot engine driver that
// the local parity suite certified bit-identical to the scientific fake:
// native == engine-host == fake by transitivity.  Runs on an sm_120 device
// inside the qualification container; exits nonzero on the first
// divergence.

#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include "../cuda/phase15_higher_support_device_tiled_frontier_internal.hpp"
#include "../cuda/phase15_higher_support_device_tiled_slot_engine.cuh"

#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::higher_support_device_tiled_frontier_products_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_probe_receipts_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_prune_records_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_terminal_records_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_pending_decisions_per_slot;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierTerminalPolicy;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::detail::
    adopt_phase15_higher_support_device_tiled_traversal;
using morsehgp3d::gpu::detail::
    build_phase15_higher_support_device_tiled_frontier_on_device;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledAdoptedTraversal;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledBatch;
using morsehgp3d::gpu::detail::
    Phase15HigherSupportDeviceTiledFrontierContextState;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledProbeReceipt;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledProductRecord;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledPruneRecord;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledRequest;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledSlotControl;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledSlotStatus;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledStopReason;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledTerminalRecord;
using morsehgp3d::hierarchy::
    exact_higher_support_product_all_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::exact_higher_support_product_query_cell_decision;
using morsehgp3d::hierarchy::exact_higher_support_terminal_geometry_decision;
using morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactHigherSupportNodeGroup;
using morsehgp3d::hierarchy::ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::ExactHigherSupportProductQueryCellDecision;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::MortonLeafRecord;
using morsehgp3d::spatial::PointId;

namespace engine = morsehgp3d::gpu::detail::higher_support_slot_engine;

using Policy = HigherSupportDeviceTiledFrontierTerminalPolicy;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

void check_cuda(cudaError_t code, const std::string& operation) {
  if (code != cudaSuccess) {
    throw std::runtime_error(
        operation + " failed: " + cudaGetErrorString(code));
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  return cloud_from(points);
}

[[nodiscard]] CanonicalPointCloud cluster_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),    point(0.25, 0.0, 0.0),
      point(0.0, 0.25, 0.0),   point(0.0, 0.0, 0.25),
      point(100.0, 100.0, 100.0), point(100.5, 100.0, 100.0),
      point(100.0, 100.5, 100.0), point(-200.0, 50.0, 300.0),
      point(-199.0, 50.0, 300.0), point(-200.0, 51.0, 300.0)};
  return cloud_from(points);
}

[[nodiscard]] CanonicalPointCloud perturbed_sphere_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(0.5773502691896258, 0.5773502691896258, 0.5773502691896257),
      point(-0.5773502691896258, -0.5773502691896258, 0.5773502691896258)};
  return cloud_from(points);
}

[[nodiscard]] CanonicalPointCloud wide_dyadic_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),
      point(1e200, 0.0, 0.0),
      point(0.0, 1e200, 0.0),
      point(1e-200, 1e-200, 1e-200),
      point(1e-200, 0.0, 1e-200),
      point(3.0, 5.0, 7.0)};
  return cloud_from(points);
}

// --------------------------------------------------------------------------
// Growth-profile clouds: verbatim transcription of the generators in
// tests/profiling/exact_higher_support_growth_profile.cpp, so the historic
// n=32/5000 no-go cases are re-measured on the exact same points.
// --------------------------------------------------------------------------

constexpr std::array<std::string_view, 3U> growth_profile_names{
    "uniform_dyadic", "separated_clusters", "multiscale_clusters"};

[[nodiscard]] std::uint64_t growth_splitmix64(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double growth_signed_dyadic(std::uint64_t value) {
  constexpr std::uint64_t mask = (std::uint64_t{1} << 20U) - 1U;
  constexpr double denominator =
      static_cast<double>(std::uint64_t{1} << 19U);
  return static_cast<double>(value & mask) / denominator - 1.0;
}

[[nodiscard]] CanonicalPointCloud growth_profile_cloud(
    std::string_view profile,
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  constexpr std::array<std::array<double, 3U>, 8U> centers{{
      {{-0.75, -0.75, -0.75}},
      {{0.75, -0.75, -0.75}},
      {{-0.75, 0.75, -0.75}},
      {{0.75, 0.75, -0.75}},
      {{-0.75, -0.75, 0.75}},
      {{0.75, -0.75, 0.75}},
      {{-0.75, 0.75, 0.75}},
      {{0.75, 0.75, 0.75}},
  }};
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::uint64_t key = static_cast<std::uint64_t>(index) + 1U;
    const double x = growth_signed_dyadic(growth_splitmix64(key * 3U));
    const double y = growth_signed_dyadic(growth_splitmix64(key * 3U + 1U));
    const double z = growth_signed_dyadic(growth_splitmix64(key * 3U + 2U));
    if (profile == growth_profile_names[0]) {
      points.push_back(point(x, y, z));
      continue;
    }
    const std::size_t cluster = index % centers.size();
    const double radius = profile == growth_profile_names[1]
                              ? 1.0 / 64.0
                              : 1.0 / static_cast<double>(
                                    std::uint64_t{1}
                                    << static_cast<unsigned int>(
                                           6U + 2U * (cluster % 4U)));
    points.push_back(point(
        centers[cluster][0] + radius * x,
        centers[cluster][1] + radius * y,
        centers[cluster][2] + radius * z));
  }
  return cloud_from(points);
}

// --------------------------------------------------------------------------
// Witness mirror (identical to the local slot-engine parity suite).
// --------------------------------------------------------------------------

[[nodiscard]] std::uint64_t order_key(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63U;
  return (bits & sign_bit) != 0U ? ~bits : bits ^ sign_bit;
}

struct WitnessGeometry {
  std::vector<engine::EngineNode> nodes;
  std::vector<std::uint64_t> coordinate_bits;
  std::vector<std::uint64_t> morton_point_ids;
  std::size_t maximum_depth{};
  std::size_t point_count{};
};

[[nodiscard]] std::size_t witness_find_split(
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  const std::uint64_t first_code = leaves[begin].morton_code;
  const std::uint64_t last_code = leaves[end - 1U].morton_code;
  if (first_code == last_code) {
    return begin + (end - begin) / 2U;
  }
  const std::uint64_t difference = first_code ^ last_code;
  const unsigned int highest_bit =
      static_cast<unsigned int>(std::bit_width(difference) - 1);
  const std::uint64_t mask = std::uint64_t{1} << highest_bit;
  std::size_t low = begin + 1U;
  std::size_t high = end;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2U;
    if ((leaves[middle].morton_code & mask) == 0U) {
      low = middle + 1U;
    } else {
      high = middle;
    }
  }
  if (low <= begin || low >= end) {
    throw std::logic_error("the witness mirror split failed");
  }
  return low;
}

[[nodiscard]] std::size_t witness_build_range(
    const CanonicalPointCloud& cloud,
    std::span<const MortonLeafRecord> leaves,
    WitnessGeometry& geometry,
    std::size_t begin,
    std::size_t end,
    std::size_t depth) {
  geometry.maximum_depth = std::max(geometry.maximum_depth, depth);
  if (end - begin == 1U) {
    engine::EngineNode leaf{};
    const PointId id = leaves[begin].point_id;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      leaf.lower_point_ids[axis] = id;
      leaf.upper_point_ids[axis] = id;
    }
    leaf.left_child = engine::engine_invalid_node_index;
    leaf.right_child = engine::engine_invalid_node_index;
    leaf.leaf_begin = begin;
    leaf.leaf_end = end;
    geometry.nodes.push_back(leaf);
    return geometry.nodes.size() - 1U;
  }
  const std::size_t split = witness_find_split(leaves, begin, end);
  const std::size_t left_child = witness_build_range(
      cloud, leaves, geometry, begin, split, depth + 1U);
  const std::size_t right_child = witness_build_range(
      cloud, leaves, geometry, split, end, depth + 1U);
  engine::EngineNode node{};
  node.left_child = left_child;
  node.right_child = right_child;
  node.leaf_begin = begin;
  node.leaf_end = end;
  const engine::EngineNode& left = geometry.nodes[left_child];
  const engine::EngineNode& right = geometry.nodes[right_child];
  const auto bits_of = [&](std::uint64_t id, std::size_t axis) {
    return cloud.point(id).canonical_input_bits()[axis];
  };
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    node.lower_point_ids[axis] =
        order_key(bits_of(right.lower_point_ids[axis], axis)) <
                order_key(bits_of(left.lower_point_ids[axis], axis))
            ? right.lower_point_ids[axis]
            : left.lower_point_ids[axis];
    node.upper_point_ids[axis] =
        order_key(bits_of(right.upper_point_ids[axis], axis)) >
                order_key(bits_of(left.upper_point_ids[axis], axis))
            ? right.upper_point_ids[axis]
            : left.upper_point_ids[axis];
  }
  geometry.nodes.push_back(node);
  return geometry.nodes.size() - 1U;
}

[[nodiscard]] WitnessGeometry build_witness_geometry(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  WitnessGeometry geometry;
  geometry.point_count = cloud.size();
  std::vector<MortonLeafRecord> leaves{
      index.leaves().begin(), index.leaves().end()};
  geometry.nodes.reserve(2U * cloud.size());
  const std::size_t root = witness_build_range(
      cloud,
      std::span<const MortonLeafRecord>{leaves},
      geometry,
      0U,
      leaves.size(),
      0U);
  const auto& counters = index.build_counters();
  if (geometry.nodes.size() != counters.node_count ||
      root + 1U != geometry.nodes.size() ||
      geometry.maximum_depth != counters.maximum_depth) {
    throw std::logic_error(
        "the witness mirror does not reproduce the certified counters");
  }
  geometry.coordinate_bits.assign(3U * cloud.size(), 0U);
  for (PointId id = 0U; id < cloud.size(); ++id) {
    const std::array<std::uint64_t, 3> bits =
        cloud.point(id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      geometry.coordinate_bits[axis * cloud.size() + id] = bits[axis];
    }
  }
  geometry.morton_point_ids.reserve(leaves.size());
  for (const MortonLeafRecord& leaf : leaves) {
    geometry.morton_point_ids.push_back(leaf.point_id);
  }
  return geometry;
}

[[nodiscard]] engine::EngineGeometryView geometry_view(
    const WitnessGeometry& geometry) {
  engine::EngineGeometryView view;
  view.coordinate_bits = geometry.coordinate_bits.data();
  view.morton_point_ids = geometry.morton_point_ids.data();
  view.nodes = geometry.nodes.data();
  view.point_count = geometry.point_count;
  view.node_count = geometry.nodes.size();
  return view;
}

[[nodiscard]] ExactDyadicAabb3 dyadic_box_from_engine(
    const engine::EngineBox& box) {
  ExactDyadicAabb3 out{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    out.lower_binary64_bits[axis] = box.lower[axis];
    out.upper_binary64_bits[axis] = box.upper[axis];
  }
  return out;
}

[[nodiscard]] Phase15HigherSupportDeviceTiledProductRecord product_of(
    const ExactHigherSupportFrontierEntry& entry) {
  Phase15HigherSupportDeviceTiledProductRecord record{};
  record.support_size = entry.support_size;
  record.group_count = entry.group_count;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    record.group_node_index[index] = entry.groups[index].node_index;
    record.group_leaf_begin[index] = entry.groups[index].leaf_begin;
    record.group_leaf_end[index] = entry.groups[index].leaf_end;
    record.group_multiplicity[index] = entry.groups[index].multiplicity;
  }
  return record;
}

[[nodiscard]] ExactHigherSupportFrontierEntry root_entry(
    std::size_t support_size,
    const WitnessGeometry& geometry) {
  ExactHigherSupportFrontierEntry entry{};
  entry.support_size = static_cast<std::uint8_t>(support_size);
  entry.group_count = 1U;
  entry.groups[0] = ExactHigherSupportNodeGroup{
      static_cast<std::uint64_t>(geometry.nodes.size() - 1U),
      0U,
      static_cast<std::uint64_t>(geometry.point_count),
      static_cast<std::uint8_t>(support_size)};
  return entry;
}

// --------------------------------------------------------------------------
// Exact BigInt frontier accounting and the canonical host pre-expansion
// (transposition of the sealed M2 bridge split onto the witness mirror).
// --------------------------------------------------------------------------

[[nodiscard]] BigInt bigint_binomial(std::size_t n, std::size_t k) {
  if (k > n) {
    return 0;
  }
  k = std::min(k, n - k);
  BigInt result = 1;
  for (std::size_t index = 1U; index <= k; ++index) {
    result *= n - k + index;
    result /= index;
  }
  return result;
}

[[nodiscard]] BigInt bigint_from_u128(std::uint64_t lo, std::uint64_t hi) {
  return (BigInt{hi} << 64U) + lo;
}

// Authenticates every group of the entry against the witness mirror and
// returns its exact support mass (the product of per-group binomials).
[[nodiscard]] BigInt witness_entry_mass(
    const WitnessGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) {
  if ((entry.support_size != 3U && entry.support_size != 4U) ||
      entry.group_count == 0U || entry.group_count > entry.support_size) {
    throw std::logic_error("a frontier entry is not canonically formed");
  }
  BigInt mass = 1;
  std::size_t multiplicity_total = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const ExactHigherSupportNodeGroup& group = entry.groups[index];
    const auto node_index = static_cast<std::size_t>(group.node_index);
    if (node_index >= geometry.nodes.size()) {
      throw std::logic_error("a frontier entry group node is out of range");
    }
    const engine::EngineNode& node = geometry.nodes[node_index];
    if (node.leaf_begin != group.leaf_begin ||
        node.leaf_end != group.leaf_end || group.multiplicity == 0U) {
      throw std::logic_error(
          "a frontier entry group does not authenticate against the mirror");
    }
    const std::size_t range =
        static_cast<std::size_t>(node.leaf_end - node.leaf_begin);
    multiplicity_total += group.multiplicity;
    mass *= bigint_binomial(range, group.multiplicity);
  }
  if (multiplicity_total != entry.support_size) {
    throw std::logic_error(
        "a frontier entry does not distribute its support size");
  }
  return mass;
}

[[nodiscard]] bool witness_entry_expandable(
    const WitnessGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(entry.groups[index].node_index);
    if (node_index < geometry.nodes.size() &&
        !engine::engine_node_is_leaf(geometry.nodes[node_index])) {
      return true;
    }
  }
  return false;
}

// Canonical split of the non-leaf group with the largest Morton range:
// every feasible multiplicity distribution (L, a), (D, m - a), children
// canonicalized by sorting groups by (leaf_begin, node index).  The exact
// BigInt child coverage must re-add to the parent coverage: a true
// partition, never a heuristic refinement.
[[nodiscard]] std::vector<ExactHigherSupportFrontierEntry>
expand_witness_entry(
    const WitnessGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) {
  std::size_t split_group_index = entry.group_count;
  std::size_t largest_range = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(entry.groups[index].node_index);
    const engine::EngineNode& current = geometry.nodes[node_index];
    const auto range =
        static_cast<std::size_t>(current.leaf_end - current.leaf_begin);
    if (!engine::engine_node_is_leaf(current) && range > largest_range) {
      split_group_index = index;
      largest_range = range;
    }
  }
  if (split_group_index == entry.group_count) {
    throw std::logic_error("a nonterminal product has no splittable group");
  }
  const ExactHigherSupportNodeGroup& split_group =
      entry.groups[split_group_index];
  const engine::EngineNode& parent =
      geometry.nodes[static_cast<std::size_t>(split_group.node_index)];
  const engine::EngineNode& left =
      geometry.nodes[static_cast<std::size_t>(parent.left_child)];
  const engine::EngineNode& right =
      geometry.nodes[static_cast<std::size_t>(parent.right_child)];
  const auto multiplicity =
      static_cast<std::size_t>(split_group.multiplicity);
  const auto left_size =
      static_cast<std::size_t>(left.leaf_end - left.leaf_begin);
  const auto right_size =
      static_cast<std::size_t>(right.leaf_end - right.leaf_begin);
  const std::size_t minimum_left =
      multiplicity > right_size ? multiplicity - right_size : 0U;
  const std::size_t maximum_left = std::min(multiplicity, left_size);
  if (minimum_left > maximum_left) {
    throw std::logic_error("a split has no feasible distribution");
  }
  std::vector<ExactHigherSupportFrontierEntry> children;
  children.reserve(maximum_left - minimum_left + 1U);
  BigInt child_coverage{0};
  for (std::size_t left_multiplicity = minimum_left;
       left_multiplicity <= maximum_left;
       ++left_multiplicity) {
    std::vector<std::pair<std::uint64_t, std::size_t>> groups;
    groups.reserve(4U);
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      if (index != split_group_index) {
        groups.emplace_back(
            entry.groups[index].node_index,
            static_cast<std::size_t>(entry.groups[index].multiplicity));
      }
    }
    if (left_multiplicity != 0U) {
      groups.emplace_back(parent.left_child, left_multiplicity);
    }
    const std::size_t right_multiplicity =
        multiplicity - left_multiplicity;
    if (right_multiplicity != 0U) {
      groups.emplace_back(parent.right_child, right_multiplicity);
    }
    std::sort(
        groups.begin(),
        groups.end(),
        [&geometry](const auto& lhs, const auto& rhs) {
          const engine::EngineNode& lhs_node =
              geometry.nodes[static_cast<std::size_t>(lhs.first)];
          const engine::EngineNode& rhs_node =
              geometry.nodes[static_cast<std::size_t>(rhs.first)];
          if (lhs_node.leaf_begin != rhs_node.leaf_begin) {
            return lhs_node.leaf_begin < rhs_node.leaf_begin;
          }
          return lhs.first < rhs.first;
        });
    if (groups.empty() || groups.size() > entry.support_size ||
        groups.size() > 4U) {
      throw std::logic_error("a canonical child has an invalid group count");
    }
    ExactHigherSupportFrontierEntry child{};
    child.support_size = entry.support_size;
    child.group_count = static_cast<std::uint8_t>(groups.size());
    for (std::size_t index = 0U; index < groups.size(); ++index) {
      const engine::EngineNode& node =
          geometry.nodes[static_cast<std::size_t>(groups[index].first)];
      child.groups[index] = ExactHigherSupportNodeGroup{
          groups[index].first,
          node.leaf_begin,
          node.leaf_end,
          static_cast<std::uint8_t>(groups[index].second)};
    }
    child_coverage += witness_entry_mass(geometry, child);
    children.push_back(child);
  }
  if (child_coverage != witness_entry_mass(geometry, entry)) {
    throw std::logic_error(
        "a canonical split did not partition its parent coverage");
  }
  return children;
}

// Deterministic canonical order of a frontier: mass descending (the sealed
// largest-mass-first device refinement), ties broken by the lexicographic
// group structure.
[[nodiscard]] bool frontier_entry_precedes(
    const std::pair<BigInt, ExactHigherSupportFrontierEntry>& lhs,
    const std::pair<BigInt, ExactHigherSupportFrontierEntry>& rhs) {
  if (lhs.first != rhs.first) {
    return lhs.first > rhs.first;
  }
  const ExactHigherSupportFrontierEntry& a = lhs.second;
  const ExactHigherSupportFrontierEntry& b = rhs.second;
  if (a.support_size != b.support_size) {
    return a.support_size < b.support_size;
  }
  if (a.group_count != b.group_count) {
    return a.group_count < b.group_count;
  }
  for (std::size_t index = 0U; index < a.group_count; ++index) {
    if (a.groups[index].leaf_begin != b.groups[index].leaf_begin) {
      return a.groups[index].leaf_begin < b.groups[index].leaf_begin;
    }
    if (a.groups[index].leaf_end != b.groups[index].leaf_end) {
      return a.groups[index].leaf_end < b.groups[index].leaf_end;
    }
    if (a.groups[index].node_index != b.groups[index].node_index) {
      return a.groups[index].node_index < b.groups[index].node_index;
    }
    if (a.groups[index].multiplicity != b.groups[index].multiplicity) {
      return a.groups[index].multiplicity < b.groups[index].multiplicity;
    }
  }
  return false;
}

// Largest-mass-first host pre-expansion of the root frontier up to the
// sealed tile capacity.  Every split is a BigInt-verified partition, so the
// returned frontier partitions C(n,3) + C(n,4) exactly; the final order is
// the deterministic canonical order above.
[[nodiscard]] std::vector<ExactHigherSupportFrontierEntry>
pre_expand_frontier(
    const WitnessGeometry& geometry,
    std::vector<ExactHigherSupportFrontierEntry> roots,
    std::size_t tile_capacity) {
  std::multimap<
      BigInt,
      ExactHigherSupportFrontierEntry,
      std::greater<BigInt>>
      expandable;
  std::vector<std::pair<BigInt, ExactHigherSupportFrontierEntry>> finished;
  for (const ExactHigherSupportFrontierEntry& root : roots) {
    BigInt mass = witness_entry_mass(geometry, root);
    if (witness_entry_expandable(geometry, root)) {
      expandable.emplace(std::move(mass), root);
    } else {
      finished.emplace_back(std::move(mass), root);
    }
  }
  while (!expandable.empty()) {
    const std::size_t frontier_size = expandable.size() + finished.size();
    if (frontier_size >= tile_capacity) {
      break;
    }
    const auto top = expandable.begin();
    const ExactHigherSupportFrontierEntry parent = top->second;
    const std::vector<ExactHigherSupportFrontierEntry> children =
        expand_witness_entry(geometry, parent);
    if (frontier_size - 1U + children.size() > tile_capacity) {
      break;
    }
    expandable.erase(top);
    for (const ExactHigherSupportFrontierEntry& child : children) {
      BigInt mass = witness_entry_mass(geometry, child);
      if (mass == 0) {
        throw std::logic_error(
            "a feasible canonical child cannot have zero mass");
      }
      if (witness_entry_expandable(geometry, child)) {
        expandable.emplace(std::move(mass), child);
      } else {
        finished.emplace_back(std::move(mass), child);
      }
    }
  }
  for (auto& [mass, entry] : expandable) {
    finished.emplace_back(mass, entry);
  }
  std::sort(finished.begin(), finished.end(), frontier_entry_precedes);
  std::vector<ExactHigherSupportFrontierEntry> frontier;
  frontier.reserve(finished.size());
  for (const auto& [mass, entry] : finished) {
    frontier.push_back(entry);
  }
  return frontier;
}

// --------------------------------------------------------------------------
// Transcripts.
// --------------------------------------------------------------------------

struct ChunkSnapshot {
  std::vector<Phase15HigherSupportDeviceTiledSlotControl> controls;
  std::vector<Phase15HigherSupportDeviceTiledPruneRecord> prunes;
  std::vector<Phase15HigherSupportDeviceTiledTerminalRecord> terminals;
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt> receipts;
  std::size_t subdivision_count{};
};

struct TileTranscript {
  std::vector<ChunkSnapshot> chunks;
  bool fatal{false};
  bool completed{false};
};

struct CaseSetup {
  Policy policy{Policy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{5U};
  std::size_t gate_quantum{2048U};
  std::size_t maximum_subdivision_count{4096U};
  std::size_t maximum_chunk_count{100'000U};
  // Wall-clock budget of one native run, checked at chunk boundaries; zero
  // disables the deadline.  Exceeding it is an honest censoring, never a
  // partial claim.
  double wall_clock_budget_seconds{0.0};
};

[[nodiscard]] Phase15HigherSupportDeviceTiledRequest base_request(
    const CaseSetup& setup,
    const Phase15HigherSupportDeviceTiledAdoptedTraversal& adopted,
    std::size_t certified_depth,
    std::size_t slot_count) {
  Phase15HigherSupportDeviceTiledRequest request;
  request.source_snapshot_epoch = adopted.source_snapshot_epoch;
  request.tile_epoch = 1U;
  request.point_count = adopted.point_count;
  request.certified_node_count = adopted.certified_node_count;
  request.certified_maximum_lbvh_depth = certified_depth;
  request.slot_count = slot_count;
  request.terminal_policy = setup.policy;
  request.maximum_relevant_closed_rank =
      setup.maximum_relevant_closed_rank;
  request.gate_evaluations_per_slot_per_chunk = setup.gate_quantum;
  request.maximum_subdivision_count = setup.maximum_subdivision_count;
  request.products_per_slot =
      higher_support_device_tiled_frontier_products_per_slot;
  request.prune_records_per_slot =
      higher_support_device_tiled_frontier_prune_records_per_slot;
  request.terminal_records_per_slot =
      higher_support_device_tiled_frontier_terminal_records_per_slot;
  request.probe_receipts_per_slot =
      higher_support_device_tiled_frontier_probe_receipts_per_slot;
  request.pending_decisions_per_slot =
      higher_support_device_tiled_frontier_pending_decisions_per_slot;
  return request;
}

// Native seam runner: one Request per chunk against the CUDA launcher.
// With keep_records, every published record segment is copied
// device-to-host and retained; without it, only the slot controls and the
// exact running totals are kept, so unbounded-volume scale probes never
// accumulate record memory on the host.
struct NativeRunSummary {
  TileTranscript transcript;
  std::vector<Phase15HigherSupportDeviceTiledSlotControl> final_controls;
  std::size_t chunk_count{};
  std::size_t subdivision_total{};
  std::uint64_t prune_record_total{};
  std::uint64_t terminal_record_total{};
  std::uint64_t receipt_total{};
  std::uint64_t gate_evaluation_total{};
  std::uint64_t expansion_total{};
  std::uint64_t deferred_int512_total{};
  std::uint64_t deferred_int1024_total{};
  std::uint64_t rational_drain_total{};
  std::uint64_t first_fatal_stop_reason{};
  std::uint64_t first_fatal_failure_code{};
  bool completed{false};
  bool fatal{false};
  bool deadline_exceeded{false};
};

[[nodiscard]] NativeRunSummary run_native_tile_summary(
    const CaseSetup& setup,
    const CanonicalPointCloud& cloud,
    std::size_t certified_depth,
    std::span<const ExactHigherSupportFrontierEntry> roots,
    const std::string& label,
    bool keep_records) {
  NativeRunSummary summary;
  const auto run_begin = std::chrono::steady_clock::now();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  const auto build = builder.build(cloud);
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  Phase15HigherSupportDeviceTiledAdoptedTraversal adopted =
      adopt_phase15_higher_support_device_tiled_traversal(std::move(lease));
  Phase15HigherSupportDeviceTiledFrontierContextState state;
  std::uint64_t record_buffer_epoch = 1U;
  std::vector<Phase15HigherSupportDeviceTiledPruneRecord> prune_arena;
  std::vector<Phase15HigherSupportDeviceTiledTerminalRecord> terminal_arena;
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt> receipt_arena;
  for (std::uint64_t chunk_sequence = 1U;
       chunk_sequence <= setup.maximum_chunk_count;
       ++chunk_sequence) {
    Phase15HigherSupportDeviceTiledRequest request = base_request(
        setup, adopted, certified_depth, roots.size());
    request.record_buffer_epoch = record_buffer_epoch++;
    request.chunk_sequence = chunk_sequence;
    request.resume_same_tile = chunk_sequence > 1U;
    request.root_entries = chunk_sequence > 1U
        ? std::span<const ExactHigherSupportFrontierEntry>{}
        : roots;
    const Phase15HigherSupportDeviceTiledBatch batch =
        build_phase15_higher_support_device_tiled_frontier_on_device(
            state, adopted, request);
    ChunkSnapshot snapshot;
    snapshot.controls = batch.host_slot_controls;
    snapshot.subdivision_count = batch.traversal_subdivision_count;
    ++summary.chunk_count;
    summary.subdivision_total += batch.traversal_subdivision_count;
    if (keep_records) {
      // Device record segments, copied bounded by the control counts.
      prune_arena.assign(roots.size() * request.prune_records_per_slot, {});
      terminal_arena.assign(
          roots.size() * request.terminal_records_per_slot, {});
      receipt_arena.assign(
          roots.size() * request.probe_receipts_per_slot, {});
      check_cuda(
          cudaMemcpy(
              prune_arena.data(),
              batch.prune_records,
              prune_arena.size() *
                  sizeof(Phase15HigherSupportDeviceTiledPruneRecord),
              cudaMemcpyDeviceToHost),
          label + ": prune segment drain");
      check_cuda(
          cudaMemcpy(
              terminal_arena.data(),
              batch.terminal_records,
              terminal_arena.size() *
                  sizeof(Phase15HigherSupportDeviceTiledTerminalRecord),
              cudaMemcpyDeviceToHost),
          label + ": terminal segment drain");
      check_cuda(
          cudaMemcpy(
              receipt_arena.data(),
              batch.probe_receipts,
              receipt_arena.size() *
                  sizeof(Phase15HigherSupportDeviceTiledProbeReceipt),
              cudaMemcpyDeviceToHost),
          label + ": receipt segment drain");
    }
    bool any_fatal = false;
    bool all_complete = true;
    for (std::size_t slot = 0U; slot < snapshot.controls.size(); ++slot) {
      const Phase15HigherSupportDeviceTiledSlotControl& control =
          snapshot.controls[slot];
      const bool slot_fatal = control.status ==
          static_cast<std::uint64_t>(
              Phase15HigherSupportDeviceTiledSlotStatus::fatal);
      if (slot_fatal && !any_fatal) {
        summary.first_fatal_stop_reason = control.stop_reason;
        summary.first_fatal_failure_code = control.failure_code;
      }
      any_fatal = any_fatal || slot_fatal;
      all_complete = all_complete &&
          control.status ==
              static_cast<std::uint64_t>(
                  Phase15HigherSupportDeviceTiledSlotStatus::complete);
      summary.prune_record_total += control.prune_record_count;
      summary.terminal_record_total += control.terminal_record_count;
      summary.receipt_total += control.probe_receipt_count;
      summary.gate_evaluation_total += control.gate_evaluation_count;
      summary.expansion_total += control.expansion_count;
      summary.deferred_int512_total += control.deferred_int512_count;
      summary.deferred_int1024_total += control.deferred_int1024_count;
      summary.rational_drain_total += control.rational_drain_count;
      if (keep_records) {
        for (std::uint64_t record = 0U;
             record < control.prune_record_count;
             ++record) {
          snapshot.prunes.push_back(
              prune_arena[slot * request.prune_records_per_slot + record]);
        }
        for (std::uint64_t record = 0U;
             record < control.terminal_record_count;
             ++record) {
          snapshot.terminals.push_back(
              terminal_arena
                  [slot * request.terminal_records_per_slot + record]);
        }
        for (std::uint64_t receipt = 0U;
             receipt < control.probe_receipt_count;
             ++receipt) {
          snapshot.receipts.push_back(
              receipt_arena
                  [slot * request.probe_receipts_per_slot + receipt]);
        }
      }
    }
    summary.final_controls = snapshot.controls;
    if (keep_records) {
      summary.transcript.chunks.push_back(std::move(snapshot));
    }
    if (any_fatal) {
      summary.fatal = true;
      summary.transcript.fatal = true;
      return summary;
    }
    if (all_complete) {
      summary.completed = true;
      summary.transcript.completed = true;
      return summary;
    }
    if (setup.wall_clock_budget_seconds > 0.0 &&
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() - run_begin)
                .count() > setup.wall_clock_budget_seconds) {
      summary.deadline_exceeded = true;
      return summary;
    }
  }
  return summary;
}

[[nodiscard]] TileTranscript run_native_tile(
    const CaseSetup& setup,
    const CanonicalPointCloud& cloud,
    std::size_t certified_depth,
    std::span<const ExactHigherSupportFrontierEntry> roots,
    const std::string& label) {
  NativeRunSummary summary = run_native_tile_summary(
      setup, cloud, certified_depth, roots, label, true);
  check(
      summary.completed || summary.fatal,
      label + ": the native tile did not settle");
  return std::move(summary.transcript);
}

// Host engine driver (identical to the local parity suite driver).
void resolve_wide_suspension(
    const engine::EngineGeometryView& view,
    const engine::EngineSlotContext& context,
    engine::EngineSlotCheckpoint& checkpoint,
    const std::string& label) {
  const Phase15HigherSupportDeviceTiledProductRecord& entry =
      context.stack[checkpoint.stack_top - 1U];
  engine::EngineBox boxes[4]{};
  check(
      engine::engine_support_boxes(view, entry, boxes),
      label + ": a suspended product authenticates its support boxes");
  std::array<ExactDyadicAabb3, 4> host_boxes{};
  for (std::size_t index = 0U; index < entry.support_size; ++index) {
    host_boxes[index] = dyadic_box_from_engine(boxes[index]);
  }
  const std::span<const ExactDyadicAabb3> support_span{
      host_boxes.data(), entry.support_size};
  const auto kind = static_cast<engine::EngineWideKind>(
      checkpoint.wide_kind);
  std::uint8_t verdict = 0U;
  switch (kind) {
    case engine::EngineWideKind::gate_no_well_centered:
      verdict = exact_higher_support_product_no_well_centered_certified(
                    support_span)
          ? 1U
          : 0U;
      break;
    case engine::EngineWideKind::gate_all_well_centered:
      verdict = exact_higher_support_product_all_well_centered_certified(
                    support_span)
          ? 1U
          : 0U;
      break;
    case engine::EngineWideKind::query_cell: {
      engine::EngineBox query_box{};
      engine::engine_node_box(
          view, checkpoint.wide_query_node_index, query_box);
      const ExactHigherSupportProductQueryCellDecision decision =
          exact_higher_support_product_query_cell_decision(
              support_span, dyadic_box_from_engine(query_box));
      verdict = decision ==
              ExactHigherSupportProductQueryCellDecision::
                  strictly_inside_every_independent_sphere
          ? static_cast<std::uint8_t>(
                engine::EngineQueryVerdict::strictly_inside)
          : decision ==
                  ExactHigherSupportProductQueryCellDecision::
                      outside_or_boundary_every_independent_sphere
              ? static_cast<std::uint8_t>(
                    engine::EngineQueryVerdict::outside_or_boundary)
              : static_cast<std::uint8_t>(
                    engine::EngineQueryVerdict::inconclusive);
      break;
    }
    case engine::EngineWideKind::terminal_geometry: {
      ExactHigherSupportProductAabbDecisionBackend backend{};
      const auto decision =
          exact_higher_support_terminal_geometry_decision(
              support_span, &backend);
      verdict = static_cast<std::uint8_t>(decision);
      if (backend ==
          ExactHigherSupportProductAabbDecisionBackend::
              arbitrary_precision_rational) {
        verdict = static_cast<std::uint8_t>(
            verdict | engine::engine_terminal_verdict_rational_flag);
      }
      break;
    }
    default:
      check(false, label + ": unknown wide kind");
      return;
  }
  checkpoint.wide_verdict = verdict;
  checkpoint.wide_verdict_valid = 1U;
  checkpoint.run_state =
      static_cast<std::uint8_t>(engine::EngineRunState::active);
}

[[nodiscard]] TileTranscript run_engine_tile(
    const CaseSetup& setup,
    const WitnessGeometry& geometry,
    std::span<const ExactHigherSupportFrontierEntry> roots,
    const std::string& label) {
  TileTranscript transcript;
  const engine::EngineGeometryView view = geometry_view(geometry);
  const std::size_t slot_count = roots.size();
  const std::size_t products_per_slot =
      higher_support_device_tiled_frontier_products_per_slot;
  const std::size_t prunes_per_slot =
      higher_support_device_tiled_frontier_prune_records_per_slot;
  const std::size_t terminals_per_slot =
      higher_support_device_tiled_frontier_terminal_records_per_slot;
  const std::size_t receipts_per_slot =
      higher_support_device_tiled_frontier_probe_receipts_per_slot;
  std::vector<Phase15HigherSupportDeviceTiledProductRecord> stacks(
      slot_count * products_per_slot);
  std::vector<Phase15HigherSupportDeviceTiledPruneRecord> prunes(
      slot_count * prunes_per_slot);
  std::vector<Phase15HigherSupportDeviceTiledTerminalRecord> terminals(
      slot_count * terminals_per_slot);
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt> receipts(
      slot_count * receipts_per_slot);
  std::vector<engine::EngineSlotCheckpoint> checkpoints(slot_count);
  const auto slot_context = [&](std::size_t slot) {
    engine::EngineSlotContext context;
    context.geometry = view;
    context.terminal_policy = static_cast<std::uint8_t>(setup.policy);
    context.maximum_relevant_closed_rank =
        setup.maximum_relevant_closed_rank;
    context.gate_evaluations_per_slot_per_chunk = setup.gate_quantum;
    context.products_per_slot = products_per_slot;
    context.prune_records_per_slot = prunes_per_slot;
    context.terminal_records_per_slot = terminals_per_slot;
    context.probe_receipts_per_slot = receipts_per_slot;
    context.stack = stacks.data() + slot * products_per_slot;
    context.prunes = prunes.data() + slot * prunes_per_slot;
    context.terminals = terminals.data() + slot * terminals_per_slot;
    context.receipts = receipts.data() + slot * receipts_per_slot;
    context.slot_index = slot;
    return context;
  };
  for (std::size_t slot = 0U; slot < slot_count; ++slot) {
    engine::EngineSlotCheckpoint& checkpoint = checkpoints[slot];
    const Phase15HigherSupportDeviceTiledProductRecord root =
        product_of(roots[slot]);
    stacks[slot * products_per_slot] = root;
    checkpoint.stack_top = 1U;
    checkpoint.stack_high_water = 1U;
    checkpoint.root_entry_digest = engine::engine_root_entry_digest(root);
    check(
        engine::engine_entry_mass(root, checkpoint.universe_mass),
        label + ": root mass fits u128");
    checkpoint.tile_epoch = 1U;
  }
  for (std::uint64_t chunk_sequence = 1U; chunk_sequence <= 100'000U;
       ++chunk_sequence) {
    for (std::size_t slot = 0U; slot < slot_count; ++slot) {
      engine::engine_begin_chunk(checkpoints[slot], chunk_sequence);
    }
    std::size_t pass_count = 0U;
    const auto any_unresolved = [&]() {
      for (const engine::EngineSlotCheckpoint& checkpoint : checkpoints) {
        if (checkpoint.run_state ==
                static_cast<std::uint8_t>(
                    engine::EngineRunState::active) ||
            checkpoint.run_state ==
                static_cast<std::uint8_t>(
                    engine::EngineRunState::paused)) {
          return true;
        }
      }
      return false;
    };
    while (any_unresolved()) {
      if (pass_count == setup.maximum_subdivision_count) {
        for (std::size_t slot = 0U; slot < slot_count; ++slot) {
          engine::EngineSlotCheckpoint& checkpoint = checkpoints[slot];
          if (checkpoint.run_state ==
                  static_cast<std::uint8_t>(
                      engine::EngineRunState::active) ||
              checkpoint.run_state ==
                  static_cast<std::uint8_t>(
                      engine::EngineRunState::paused)) {
            engine::engine_rollback_slot_chunk(
                slot_context(slot), checkpoint);
            checkpoint.run_state = static_cast<std::uint8_t>(
                engine::EngineRunState::fatal);
            checkpoint.stop_reason = static_cast<std::uint8_t>(
                Phase15HigherSupportDeviceTiledStopReason::
                    subdivision_capacity);
            checkpoint.failure_code = 0U;
          }
        }
        break;
      }
      ++pass_count;
      for (std::size_t slot = 0U; slot < slot_count; ++slot) {
        engine::EngineSlotCheckpoint& checkpoint = checkpoints[slot];
        if (checkpoint.run_state ==
            static_cast<std::uint8_t>(engine::EngineRunState::paused)) {
          checkpoint.run_state =
              static_cast<std::uint8_t>(engine::EngineRunState::active);
        }
        if (checkpoint.run_state !=
            static_cast<std::uint8_t>(engine::EngineRunState::active)) {
          continue;
        }
        const engine::EngineSlotContext context = slot_context(slot);
        engine::engine_run_subdivision(context, checkpoint, true);
        while (checkpoint.run_state ==
                   static_cast<std::uint8_t>(
                       engine::EngineRunState::paused) &&
               checkpoint.wide_pending != 0U) {
          resolve_wide_suspension(view, context, checkpoint, label);
          engine::engine_run_subdivision(context, checkpoint, false);
        }
        if (checkpoint.run_state ==
            static_cast<std::uint8_t>(engine::EngineRunState::fatal)) {
          engine::engine_rollback_slot_chunk(context, checkpoint);
        }
      }
    }
    if (pass_count == 0U) {
      pass_count = 1U;
    }
    bool tile_fatal = false;
    bool capacity_trigger = false;
    for (const engine::EngineSlotCheckpoint& checkpoint : checkpoints) {
      if (checkpoint.run_state ==
          static_cast<std::uint8_t>(engine::EngineRunState::fatal)) {
        tile_fatal = true;
        capacity_trigger = capacity_trigger ||
            checkpoint.stop_reason ==
                static_cast<std::uint8_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        subdivision_capacity);
      }
    }
    if (tile_fatal) {
      for (std::size_t slot = 0U; slot < slot_count; ++slot) {
        engine::EngineSlotCheckpoint& checkpoint = checkpoints[slot];
        if (checkpoint.run_state ==
            static_cast<std::uint8_t>(
                engine::EngineRunState::chunk_ready)) {
          engine::engine_rollback_slot_chunk(
              slot_context(slot), checkpoint);
          checkpoint.run_state =
              static_cast<std::uint8_t>(engine::EngineRunState::fatal);
          checkpoint.stop_reason = capacity_trigger
              ? static_cast<std::uint8_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        subdivision_capacity)
              : static_cast<std::uint8_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        fatal_failure);
          checkpoint.failure_code = 0U;
        }
      }
    }
    ChunkSnapshot snapshot;
    snapshot.subdivision_count = pass_count;
    snapshot.controls.resize(slot_count);
    bool any_fatal = false;
    bool all_complete = true;
    for (std::size_t slot = 0U; slot < slot_count; ++slot) {
      engine::EngineSlotCheckpoint& checkpoint = checkpoints[slot];
      check(
          engine::engine_write_slot_control(
              checkpoint, 1U, chunk_sequence, snapshot.controls[slot]),
          label + ": control assembles");
      check(
          engine::engine_commit_chunk(checkpoint),
          label + ": chunk commits");
      const Phase15HigherSupportDeviceTiledSlotControl& control =
          snapshot.controls[slot];
      any_fatal = any_fatal ||
          control.status ==
              static_cast<std::uint64_t>(
                  Phase15HigherSupportDeviceTiledSlotStatus::fatal);
      all_complete = all_complete &&
          control.status ==
              static_cast<std::uint64_t>(
                  Phase15HigherSupportDeviceTiledSlotStatus::complete);
      for (std::uint64_t record = 0U;
           record < control.prune_record_count;
           ++record) {
        snapshot.prunes.push_back(
            prunes[slot * prunes_per_slot + record]);
      }
      for (std::uint64_t record = 0U;
           record < control.terminal_record_count;
           ++record) {
        snapshot.terminals.push_back(
            terminals[slot * terminals_per_slot + record]);
      }
      for (std::uint64_t receipt = 0U;
           receipt < control.probe_receipt_count;
           ++receipt) {
        snapshot.receipts.push_back(
            receipts[slot * receipts_per_slot + receipt]);
      }
    }
    transcript.chunks.push_back(std::move(snapshot));
    if (any_fatal) {
      transcript.fatal = true;
      return transcript;
    }
    if (all_complete) {
      transcript.completed = true;
      return transcript;
    }
  }
  check(false, label + ": the engine tile did not settle");
  return transcript;
}

void compare_transcripts(
    const TileTranscript& native,
    const TileTranscript& host,
    const std::string& label) {
  std::size_t record_total = 0U;
  for (const ChunkSnapshot& chunk : host.chunks) {
    record_total += chunk.prunes.size() + chunk.terminals.size();
  }
  check(
      host.completed && !host.fatal && record_total > 0U,
      label + ": the host transcript completes with records");
  check(
      native.completed == host.completed && native.fatal == host.fatal &&
          native.chunks.size() == host.chunks.size(),
      label + ": both transcripts settle identically");
  const std::size_t chunk_count =
      std::min(native.chunks.size(), host.chunks.size());
  for (std::size_t chunk = 0U; chunk < chunk_count; ++chunk) {
    const ChunkSnapshot& left = native.chunks[chunk];
    const ChunkSnapshot& right = host.chunks[chunk];
    const std::string chunk_label =
        label + "/chunk" + std::to_string(chunk + 1U);
    check(
        left.subdivision_count == right.subdivision_count,
        chunk_label + ": subdivision counts agree");
    check(
        left.controls.size() == right.controls.size() &&
            std::memcmp(
                left.controls.data(),
                right.controls.data(),
                left.controls.size() *
                    sizeof(Phase15HigherSupportDeviceTiledSlotControl)) ==
                0,
        chunk_label + ": slot controls are bit-identical, unmasked");
    check(
        left.prunes.size() == right.prunes.size() &&
            std::memcmp(
                left.prunes.data(),
                right.prunes.data(),
                left.prunes.size() *
                    sizeof(Phase15HigherSupportDeviceTiledPruneRecord)) ==
                0,
        chunk_label + ": prune records are bit-identical");
    check(
        left.terminals.size() == right.terminals.size() &&
            std::memcmp(
                left.terminals.data(),
                right.terminals.data(),
                left.terminals.size() *
                    sizeof(
                        Phase15HigherSupportDeviceTiledTerminalRecord)) ==
                0,
        chunk_label + ": terminal records are bit-identical");
    check(
        left.receipts.size() == right.receipts.size() &&
            std::memcmp(
                left.receipts.data(),
                right.receipts.data(),
                left.receipts.size() *
                    sizeof(Phase15HigherSupportDeviceTiledProbeReceipt)) ==
                0,
        chunk_label + ": probe receipts are bit-identical");
  }
}

void run_parity_case(
    const CaseSetup& setup,
    const CanonicalPointCloud& cloud,
    std::span<const std::size_t> root_support_sizes,
    const std::string& label) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const WitnessGeometry geometry = build_witness_geometry(index, cloud);
  std::vector<ExactHigherSupportFrontierEntry> roots;
  roots.reserve(root_support_sizes.size());
  for (const std::size_t support_size : root_support_sizes) {
    roots.push_back(root_entry(support_size, geometry));
  }
  const TileTranscript host = run_engine_tile(
      setup,
      geometry,
      std::span<const ExactHigherSupportFrontierEntry>{roots},
      label);
  const TileTranscript native = run_native_tile(
      setup,
      cloud,
      index.build_counters().maximum_depth,
      std::span<const ExactHigherSupportFrontierEntry>{roots},
      label);
  compare_transcripts(native, host, label);
  std::cout << label << ": OK (" << host.chunks.size() << " chunks)\n";
}

// Scale mode: full-universe native resolution with intrinsic closure
// certification (every slot control already proves cumulative well + rank +
// terminal == universe with unresolved == 0 through the sealed u128
// partition), plus wall-clock timing.  Parity against the host engine is
// kept at n=32; larger clouds are native-only with closure checks.
void run_scale_case(
    std::size_t point_count,
    bool with_host_parity,
    const std::string& label) {
  const CanonicalPointCloud cloud = line_cloud(point_count);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const WitnessGeometry geometry = build_witness_geometry(index, cloud);
  const std::array<std::size_t, 2> both_supports{3U, 4U};
  std::vector<ExactHigherSupportFrontierEntry> roots;
  for (const std::size_t support_size : both_supports) {
    roots.push_back(root_entry(support_size, geometry));
  }
  CaseSetup setup;
  setup.maximum_relevant_closed_rank = 5U;
  const auto begin = std::chrono::steady_clock::now();
  const TileTranscript native = run_native_tile(
      setup,
      cloud,
      index.build_counters().maximum_depth,
      std::span<const ExactHigherSupportFrontierEntry>{roots},
      label);
  const double seconds =
      std::chrono::duration<double>(
          std::chrono::steady_clock::now() - begin)
          .count();
  check(
      native.completed && !native.fatal,
      label + ": the native tile resolves its complete universe");
  // Intrinsic closure: the final chunk's controls must close every slot
  // partition with zero unresolved mass and cumulative masses summing to
  // C(range,3) and C(range,4) respectively.
  const ChunkSnapshot& last = native.chunks.back();
  for (std::size_t slot = 0U; slot < last.controls.size(); ++slot) {
    const Phase15HigherSupportDeviceTiledSlotControl& control =
        last.controls[slot];
    engine::EngineU128 resolved{
        control.well_prune_mass_cumulative_lo,
        control.well_prune_mass_cumulative_hi};
    check(
        morsehgp3d::gpu::detail::
                phase15_higher_support_device_tiled_u128_add(
                    resolved,
                    engine::EngineU128{
                        control.rank_prune_mass_cumulative_lo,
                        control.rank_prune_mass_cumulative_hi}) &&
            morsehgp3d::gpu::detail::
                phase15_higher_support_device_tiled_u128_add(
                    resolved,
                    engine::EngineU128{
                        control.terminal_mass_cumulative_lo,
                        control.terminal_mass_cumulative_hi}),
        label + ": closure sum stays in range");
    engine::EngineU128 expected{};
    check(
        morsehgp3d::gpu::detail::
            phase15_higher_support_device_tiled_binomial_u128(
                point_count, slot == 0U ? 3U : 4U, expected),
        label + ": expected binomial in range");
    check(
        control.unresolved_mass_lo == 0U &&
            control.unresolved_mass_hi == 0U &&
            resolved == expected,
        label + ": the universe closes exactly on device");
  }
  if (with_host_parity) {
    const TileTranscript host = run_engine_tile(
        setup,
        geometry,
        std::span<const ExactHigherSupportFrontierEntry>{roots},
        label);
    compare_transcripts(native, host, label);
  }
  std::cout << label << ": OK in " << seconds << " s ("
            << native.chunks.size() << " chunks)\n";
}

// --------------------------------------------------------------------------
// Tiled full-universe resolution (M5b): host pre-expansion up to the sealed
// tile capacity, one native tile over the pre-expanded frontier, exact
// BigInt closure of the universe C(n,3) + C(n,4), and optional
// bit-identical host-engine parity on the very same tile.
// --------------------------------------------------------------------------

struct TiledCaseOutcome {
  bool closed{false};
  bool censored{false};
};

TiledCaseOutcome run_tiled_case(
    const CaseSetup& setup,
    const CanonicalPointCloud& cloud,
    bool with_host_parity,
    bool censure_is_failure,
    const std::string& label) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const WitnessGeometry geometry = build_witness_geometry(index, cloud);
  const std::size_t n = geometry.point_count;
  std::vector<ExactHigherSupportFrontierEntry> roots;
  roots.push_back(root_entry(3U, geometry));
  roots.push_back(root_entry(4U, geometry));
  const auto expand_begin = std::chrono::steady_clock::now();
  const std::vector<ExactHigherSupportFrontierEntry> frontier =
      pre_expand_frontier(
          geometry,
          std::move(roots),
          morsehgp3d::gpu::
              higher_support_device_tiled_frontier_maximum_slot_tile_capacity);
  const double expand_seconds =
      std::chrono::duration<double>(
          std::chrono::steady_clock::now() - expand_begin)
          .count();
  const BigInt universe =
      bigint_binomial(n, 3U) + bigint_binomial(n, 4U);
  std::vector<BigInt> slot_masses;
  slot_masses.reserve(frontier.size());
  BigInt frontier_mass{0};
  for (const ExactHigherSupportFrontierEntry& entry : frontier) {
    slot_masses.push_back(witness_entry_mass(geometry, entry));
    frontier_mass += slot_masses.back();
  }
  check(
      frontier_mass == universe,
      label + ": the pre-expansion partitions the universe");
  const auto native_begin = std::chrono::steady_clock::now();
  const NativeRunSummary native = run_native_tile_summary(
      setup,
      cloud,
      index.build_counters().maximum_depth,
      std::span<const ExactHigherSupportFrontierEntry>{frontier},
      label,
      with_host_parity);
  const double native_seconds =
      std::chrono::duration<double>(
          std::chrono::steady_clock::now() - native_begin)
          .count();
  TiledCaseOutcome outcome;
  if (native.fatal || !native.completed) {
    outcome.censored = true;
    std::cout << label << ": CENSORED without closure claim after "
              << native_seconds << " s (slots " << frontier.size()
              << ", chunks " << native.chunk_count << ", subdivisions "
              << native.subdivision_total << ", stop_reason "
              << native.first_fatal_stop_reason << ", failure_code "
              << native.first_fatal_failure_code
              << (native.fatal
                      ? ", fatal"
                      : native.deadline_exceeded
                          ? ", deadline exceeded"
                          : ", chunk budget exhausted")
              << ")\n";
    check(
        !censure_is_failure,
        label + ": the tiled universe must resolve completely");
    return outcome;
  }
  bool closure_ok = native.final_controls.size() == frontier.size();
  check(closure_ok, label + ": one final control per slot");
  BigInt well{0};
  BigInt rank{0};
  BigInt terminal{0};
  for (std::size_t slot = 0U;
       closure_ok && slot < native.final_controls.size();
       ++slot) {
    const Phase15HigherSupportDeviceTiledSlotControl& control =
        native.final_controls[slot];
    const BigInt slot_well = bigint_from_u128(
        control.well_prune_mass_cumulative_lo,
        control.well_prune_mass_cumulative_hi);
    const BigInt slot_rank = bigint_from_u128(
        control.rank_prune_mass_cumulative_lo,
        control.rank_prune_mass_cumulative_hi);
    const BigInt slot_terminal = bigint_from_u128(
        control.terminal_mass_cumulative_lo,
        control.terminal_mass_cumulative_hi);
    closure_ok = closure_ok && control.unresolved_mass_lo == 0U &&
        control.unresolved_mass_hi == 0U &&
        slot_well + slot_rank + slot_terminal == slot_masses[slot];
    well += slot_well;
    rank += slot_rank;
    terminal += slot_terminal;
  }
  check(closure_ok, label + ": every slot closes its exact sub-universe");
  check(
      well + rank + terminal == universe,
      label + ": the universe closes exactly on device");
  outcome.closed =
      closure_ok && well + rank + terminal == universe;
  std::cout << label << ": OK in " << native_seconds
            << " s (pre-expansion " << expand_seconds << " s, slots "
            << frontier.size() << ", chunks " << native.chunk_count
            << ", subdivisions " << native.subdivision_total
            << ", prune records " << native.prune_record_total
            << ", terminal records " << native.terminal_record_total
            << ", gates " << native.gate_evaluation_total
            << ", expansions " << native.expansion_total
            << ", deferred512 " << native.deferred_int512_total
            << ", deferred1024 " << native.deferred_int1024_total
            << ", rational drains " << native.rational_drain_total
            << ", well " << canonical_integer_string(well) << ", rank "
            << canonical_integer_string(rank) << ", terminal "
            << canonical_integer_string(terminal) << ")\n";
  if (with_host_parity) {
    const TileTranscript host = run_engine_tile(
        setup,
        geometry,
        std::span<const ExactHigherSupportFrontierEntry>{frontier},
        label);
    compare_transcripts(native.transcript, host, label);
    std::cout << label << ": host parity OK (" << host.chunks.size()
              << " chunks)\n";
  }
  return outcome;
}

// The historic no-go profile (audit section 8.1): three growth-profile
// families at n=32 with 5000 bounded work units per case ended in
// work_unit_limit for K=5 and K=10, and sizes 64 and 128 were never
// launched.  The re-measure resolves every case completely on the native
// tiled frontier, n-major so partial sessions still close whole sizes.
[[nodiscard]] int run_no_go_mode(bool with_host_parity) {
  const std::array<std::size_t, 3U> sizes{32U, 64U, 128U};
  const std::array<std::size_t, 2U> ranks{5U, 10U};
  try {
    for (const std::size_t size : sizes) {
      for (const std::string_view profile : growth_profile_names) {
        for (const std::size_t rank : ranks) {
          CaseSetup setup;
          setup.maximum_relevant_closed_rank = rank;
          const CanonicalPointCloud cloud =
              growth_profile_cloud(profile, size);
          const std::string label = std::string{"no-go/"} +
              std::string{profile} + "/n" + std::to_string(size) + "/K" +
              std::to_string(rank);
          const TiledCaseOutcome outcome = run_tiled_case(
              setup, cloud, with_host_parity && size == 32U, true, label);
          if (!outcome.closed) {
            std::cerr << failures << " no-go re-measure failures\n";
            return 1;
          }
        }
      }
    }
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " no-go re-measure failures\n";
    return 1;
  }
  std::cout << "higher-support device tiled frontier no-go re-measure "
               "passed\n";
  return 0;
}

// 50k SLO probe: the uniform_dyadic family at n=50000 on the native tiled
// frontier, exact closure or an honest censoring report.  Exit 0 claims
// closure of both ranks; exit 3 reports censoring without any claim.
[[nodiscard]] int run_slo50k_mode(double wall_clock_budget_seconds) {
  const std::array<std::size_t, 2U> ranks{5U, 10U};
  const auto mode_begin = std::chrono::steady_clock::now();
  bool all_closed = true;
  try {
    for (const std::size_t rank : ranks) {
      CaseSetup setup;
      setup.maximum_relevant_closed_rank = rank;
      setup.maximum_subdivision_count = 65'536U;
      setup.maximum_chunk_count = 10'000'000U;
      if (wall_clock_budget_seconds > 0.0) {
        const double elapsed =
            std::chrono::duration<double>(
                std::chrono::steady_clock::now() - mode_begin)
                .count();
        setup.wall_clock_budget_seconds =
            std::max(wall_clock_budget_seconds - elapsed, 1.0);
      }
      const CanonicalPointCloud cloud =
          growth_profile_cloud(growth_profile_names[0], 50'000U);
      const std::string label =
          "slo50k/uniform_dyadic/n50000/K" + std::to_string(rank);
      const TiledCaseOutcome outcome =
          run_tiled_case(setup, cloud, false, false, label);
      all_closed = all_closed && outcome.closed;
    }
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " 50k probe failures\n";
    return 1;
  }
  if (!all_closed) {
    std::cout << "higher-support device tiled frontier 50k probe censored "
                 "without closure claim\n";
    return 3;
  }
  std::cout << "higher-support device tiled frontier 50k probe closed\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string mode = argc > 1 ? std::string{argv[1]} : std::string{};
  if (mode == "--no-go") {
    return run_no_go_mode(true);
  }
  if (mode == "--no-go-native") {
    return run_no_go_mode(false);
  }
  if (mode == "--slo50k") {
    double budget_seconds = 0.0;
    if (argc > 2) {
      budget_seconds = std::strtod(argv[2], nullptr);
      if (!(budget_seconds > 0.0)) {
        std::cerr << "FAIL: the 50k probe budget must be positive seconds\n";
        return 1;
      }
    }
    return run_slo50k_mode(budget_seconds);
  }
  const bool scale_mode = mode == "--scale";
  if (scale_mode) {
    try {
      run_scale_case(32U, true, "scale/line32/parity");
      run_scale_case(64U, false, "scale/line64/native");
      run_scale_case(128U, false, "scale/line128/native");
    } catch (const std::exception& error) {
      std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
      return 1;
    }
    if (failures != 0) {
      std::cerr << failures << " scale failures\n";
      return 1;
    }
    std::cout << "higher-support device tiled frontier scale runs passed\n";
    return 0;
  }
  try {
    const std::array<std::size_t, 2> both_supports{3U, 4U};
    const std::array<std::size_t, 1> support_three{3U};
    {
      CaseSetup setup;
      setup.maximum_relevant_closed_rank = 5U;
      run_parity_case(
          setup,
          line_cloud(12U),
          std::span<const std::size_t>{both_supports},
          "device/line12/K5");
      setup.maximum_relevant_closed_rank = 3U;
      run_parity_case(
          setup,
          line_cloud(12U),
          std::span<const std::size_t>{both_supports},
          "device/line12/K3");
    }
    {
      CaseSetup setup;
      setup.maximum_relevant_closed_rank = 5U;
      run_parity_case(
          setup,
          cluster_cloud(),
          std::span<const std::size_t>{both_supports},
          "device/clusters10/K5");
    }
    {
      CaseSetup setup;
      setup.maximum_relevant_closed_rank = 3U;
      run_parity_case(
          setup,
          perturbed_sphere_cloud(),
          std::span<const std::size_t>{both_supports},
          "device/sphere8/K3");
    }
    {
      CaseSetup setup;
      setup.policy = Policy::strict_interior_carrier_q3;
      run_parity_case(
          setup,
          perturbed_sphere_cloud(),
          std::span<const std::size_t>{support_three},
          "device/sphere8/carrier");
    }
    {
      CaseSetup setup;
      setup.gate_quantum = 2U;
      setup.maximum_relevant_closed_rank = 5U;
      run_parity_case(
          setup,
          cluster_cloud(),
          std::span<const std::size_t>{both_supports},
          "device/clusters10/quantum2");
    }
    {
      CaseSetup setup;
      setup.maximum_relevant_closed_rank = 4U;
      run_parity_case(
          setup,
          wide_dyadic_cloud(),
          std::span<const std::size_t>{both_supports},
          "device/wide-dyadic/K4");
    }
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures
              << " higher-support device tiled frontier parity failures\n";
    return 1;
  }
  std::cout
      << "higher-support device tiled frontier device parity passed\n";
  return 0;
}
