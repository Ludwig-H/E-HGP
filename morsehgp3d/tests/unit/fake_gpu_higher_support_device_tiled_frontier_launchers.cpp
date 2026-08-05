#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

// Scientific host fake for the Phase 15 higher-support device tiled seam.
// Unlike the scripted pair fake, this launcher executes the exact slot-local
// DFS for every rooted product: the universal no-well-centred gate, the
// positive all-well-centred gate, the bounded local Morton probe under the
// substitute v1 seed (the Morton-lowest support position, proposal-only),
// the categorical terminal decision and the exact expansion partition.  All
// decisions are the public exact primitives of higher_support_product.hpp;
// no floating-point value participates in any scientific decision and all
// slot masses are unsigned 128-bit mirrors of what the resident kernels
// would keep, revalidated in exact::BigInt by the host context.

namespace morsehgp3d::gpu::test_support {
namespace {

using FakeU128 = detail::Phase15HigherSupportDeviceTiledUnsigned128;

inline constexpr std::size_t fake_invalid_node_index =
    static_cast<std::size_t>(-1);

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::uint64_t checked_u64_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

void checked_increment(std::uint64_t& value, const char* message) {
  value = checked_u64_sum(value, UINT64_C(1), message);
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (value > std::numeric_limits<std::uint64_t>::max()) {
    throw std::length_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(message);
  }
  return static_cast<std::size_t>(value);
}

// Total order on canonical binary64 bit patterns, identical to the CPU LBVH
// witness rule: negative payloads are reversed so the key order matches the
// represented value order without any floating-point comparison.
[[nodiscard]] std::uint64_t fake_binary64_order_key(
    std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63U;
  return (bits & sign_bit) != 0U ? ~bits : bits ^ sign_bit;
}

// Read-only host mirror of one certified LBVH node.  The per-axis bounds are
// the exact canonical coordinate bit patterns of the CPU witness points, so
// mirror node boxes equal the private node_box of the CPU stream bit for
// bit.
struct FakeMirrorNode {
  std::size_t left_child{fake_invalid_node_index};
  std::size_t right_child{fake_invalid_node_index};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::array<std::uint64_t, 3> lower_bits{};
  std::array<std::uint64_t, 3> upper_bits{};

  [[nodiscard]] bool is_leaf() const noexcept {
    return left_child == fake_invalid_node_index;
  }
};

// Self-contained geometry mirror rebuilt from the public leaf records with
// the exact radix/median split rule of the CPU builder.  Nothing references
// the bound index or cloud after binding succeeds.
struct FakeBoundGeometry {
  std::vector<spatial::MortonLeafRecord> leaves;
  std::vector<FakeMirrorNode> nodes;
  std::size_t root_index{};
  std::size_t maximum_depth{};
};

struct FakeProbeCell {
  std::size_t root_node_index{};
  std::vector<std::size_t> fallback_leaf_node_indices;
};

// Persistent probe cursor of one slot.  Receipts are staged in plan order
// and committed only when the certified strict-interior mass reaches the
// policy threshold; a fail-open probe discards them without publication.
struct FakeSlotProbe {
  std::size_t threshold{};
  std::vector<FakeProbeCell> cells;
  std::size_t next_cell_index{};
  std::size_t next_fallback_index{};
  bool fallback_active{false};
  std::uint64_t certified_point_count{};
  std::vector<detail::Phase15HigherSupportDeviceTiledProbeReceipt>
      pending_receipts;
};

// Continuation state of one slot across chunks: the resident DFS stack, the
// probe cursor, the committed cumulative masses and the cumulative stack
// high water.  Masses are unsigned 128-bit only; exact::BigInt never enters
// this device mirror.
struct FakeSlotState {
  std::vector<hierarchy::ExactHigherSupportFrontierEntry> stack;
  std::optional<FakeSlotProbe> probe;
  bool pending_terminal{false};
  bool complete{false};
  FakeU128 universe_mass{};
  FakeU128 well_cumulative{};
  FakeU128 rank_cumulative{};
  FakeU128 terminal_cumulative{};
  std::uint64_t root_entry_digest{};
  std::uint64_t stack_high_water{};
};

struct FakeTileState {
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t source_snapshot_epoch{};
  std::size_t slot_count{};
  std::vector<FakeSlotState> slots;
};

std::mutex fake_mutex;
std::optional<FakeBoundGeometry> fake_geometry;
std::optional<FakeTileState> fake_tile;
FakeHigherSupportDeviceTiledFrontierCorruption fake_corruption{
    FakeHigherSupportDeviceTiledFrontierCorruption::none};
std::atomic<std::size_t> fake_launch_count{0U};

[[nodiscard]] std::size_t fake_find_split(
    std::span<const spatial::MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  if (begin + 1U >= end || end > leaves.size()) {
    throw std::logic_error(
        "the fake Phase 15 LBVH mirror split requires at least two leaves");
  }
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
    throw std::logic_error(
        "the fake Phase 15 LBVH mirror radix split did not divide its "
        "range");
  }
  return low;
}

// Strict postorder rebuild, identical to MortonLbvhIndex::build_range, so
// mirror node indices coincide with the certified CPU node indices that the
// canonical frontier entries reference.  The recursion depth is bounded by
// the proved 63 + 31 split-depth bound for every admissible cloud.
[[nodiscard]] std::size_t fake_build_mirror_range(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::MortonLeafRecord> leaves,
    std::vector<FakeMirrorNode>& nodes,
    std::size_t begin,
    std::size_t end,
    std::size_t depth,
    std::size_t& maximum_depth) {
  if (begin >= end || end > leaves.size()) {
    throw std::logic_error(
        "the fake Phase 15 LBVH mirror received an invalid leaf range");
  }
  maximum_depth = std::max(maximum_depth, depth);
  if (end - begin == 1U) {
    FakeMirrorNode leaf;
    leaf.leaf_begin = begin;
    leaf.leaf_end = end;
    const std::array<std::uint64_t, 3> bits =
        cloud.point(leaves[begin].point_id).canonical_input_bits();
    leaf.lower_bits = bits;
    leaf.upper_bits = bits;
    nodes.push_back(leaf);
    return nodes.size() - 1U;
  }
  const std::size_t split = fake_find_split(leaves, begin, end);
  const std::size_t left_child = fake_build_mirror_range(
      cloud,
      leaves,
      nodes,
      begin,
      split,
      checked_size_sum(
          depth,
          1U,
          "the fake Phase 15 LBVH mirror depth overflows size_t"),
      maximum_depth);
  const std::size_t right_child = fake_build_mirror_range(
      cloud,
      leaves,
      nodes,
      split,
      end,
      depth + 1U,
      maximum_depth);
  FakeMirrorNode node;
  node.left_child = left_child;
  node.right_child = right_child;
  node.leaf_begin = begin;
  node.leaf_end = end;
  const FakeMirrorNode& left = nodes[left_child];
  const FakeMirrorNode& right = nodes[right_child];
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    node.lower_bits[axis] =
        fake_binary64_order_key(right.lower_bits[axis]) <
                fake_binary64_order_key(left.lower_bits[axis])
            ? right.lower_bits[axis]
            : left.lower_bits[axis];
    node.upper_bits[axis] =
        fake_binary64_order_key(right.upper_bits[axis]) >
                fake_binary64_order_key(left.upper_bits[axis])
            ? right.upper_bits[axis]
            : left.upper_bits[axis];
  }
  nodes.push_back(node);
  return nodes.size() - 1U;
}

}  // namespace

void bind_fake_higher_support_device_tiled_geometry(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  if (!index.validated_for(cloud) || cloud.size() == 0U) {
    throw std::runtime_error(
        "the fake Phase 15 higher-support device tiled geometry binding "
        "requires a Morton LBVH validated for its canonical cloud");
  }
  const spatial::MortonLbvhBuildCounters& counters = index.build_counters();
  FakeBoundGeometry geometry;
  geometry.leaves.assign(index.leaves().begin(), index.leaves().end());
  if (geometry.leaves.size() != cloud.size() ||
      counters.point_count != cloud.size()) {
    throw std::runtime_error(
        "the fake Phase 15 higher-support device tiled geometry binding "
        "received an index whose leaves contradict its cloud");
  }
  geometry.nodes.reserve(checked_size_product(
      2U,
      cloud.size(),
      "the fake Phase 15 LBVH mirror node reserve overflows size_t"));
  geometry.root_index = fake_build_mirror_range(
      cloud,
      std::span<const spatial::MortonLeafRecord>{geometry.leaves},
      geometry.nodes,
      0U,
      geometry.leaves.size(),
      0U,
      geometry.maximum_depth);
  if (geometry.nodes.size() != counters.node_count ||
      geometry.root_index + 1U != geometry.nodes.size() ||
      geometry.maximum_depth != counters.maximum_depth) {
    throw std::runtime_error(
        "the fake Phase 15 LBVH mirror does not reproduce the certified "
        "build counters of the bound index");
  }
  fake_geometry.emplace(std::move(geometry));
  fake_tile.reset();
}

void set_fake_gpu_higher_support_device_tiled_frontier_corruption(
    FakeHigherSupportDeviceTiledFrontierCorruption corruption) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_corruption = corruption;
}

void reset_fake_gpu_higher_support_device_tiled_frontier() noexcept {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_geometry.reset();
  fake_tile.reset();
  fake_corruption = FakeHigherSupportDeviceTiledFrontierCorruption::none;
  fake_launch_count.store(0U, std::memory_order_relaxed);
}

std::size_t
fake_gpu_higher_support_device_tiled_frontier_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

using Corruption =
    test_support::FakeHigherSupportDeviceTiledFrontierCorruption;
using test_support::checked_increment;
using test_support::checked_size;
using test_support::checked_size_product;
using test_support::checked_size_sum;
using test_support::checked_u64;
using test_support::checked_u64_sum;
using test_support::FakeBoundGeometry;
using test_support::FakeMirrorNode;
using test_support::FakeProbeCell;
using test_support::FakeSlotProbe;
using test_support::FakeSlotState;
using test_support::FakeTileState;
using test_support::FakeU128;
using FrontierEntry = hierarchy::ExactHigherSupportFrontierEntry;
using NodeGroup = hierarchy::ExactHigherSupportNodeGroup;

// -------------------------------------------------------------------------
// Exact unsigned 128-bit helpers with fail-closed overflow behaviour.
// -------------------------------------------------------------------------

void fake_u128_accumulate(
    FakeU128& value,
    FakeU128 addend,
    const char* message) {
  if (!phase15_higher_support_device_tiled_u128_add(value, addend)) {
    throw std::overflow_error(message);
  }
}

[[nodiscard]] FakeU128 fake_u128_difference(
    FakeU128 value,
    FakeU128 subtrahend,
    const char* message) {
  if (!phase15_higher_support_device_tiled_u128_subtract(
          value, subtrahend)) {
    throw std::logic_error(message);
  }
  return value;
}

[[nodiscard]] FakeU128 fake_binomial(
    std::uint64_t range_size,
    std::uint64_t multiplicity) {
  FakeU128 out{};
  if (!phase15_higher_support_device_tiled_binomial_u128(
          range_size, multiplicity, out)) {
    throw std::overflow_error(
        "a fake Phase 15 higher-support binomial left the certified "
        "unsigned 128-bit range");
  }
  return out;
}

// Exact product of the per-group binomials of one entry.  At most one group
// can carry multiplicity three or four, so the running product and the wide
// factor never both exceed one 64-bit word and the product stays exact.
[[nodiscard]] FakeU128 fake_entry_mass(const FrontierEntry& entry) {
  FakeU128 value{1U, 0U};
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const NodeGroup& group = entry.groups[index];
    const FakeU128 factor = fake_binomial(
        group.leaf_end - group.leaf_begin, group.multiplicity);
    if (factor.hi == 0U) {
      if (!phase15_higher_support_device_tiled_u128_multiply_small(
              value, factor.lo)) {
        throw std::overflow_error(
            "a fake Phase 15 higher-support slot mass left the certified "
            "unsigned 128-bit range");
      }
      continue;
    }
    if (value.hi != 0U) {
      throw std::overflow_error(
          "a fake Phase 15 higher-support slot mass has two wide binomial "
          "factors");
    }
    FakeU128 widened = factor;
    if (!phase15_higher_support_device_tiled_u128_multiply_small(
            widened, value.lo)) {
      throw std::overflow_error(
          "a fake Phase 15 higher-support slot mass left the certified "
          "unsigned 128-bit range");
    }
    value = widened;
  }
  return value;
}

// -------------------------------------------------------------------------
// Mirror geometry accessors (exact transposition of the CPU stream).
// -------------------------------------------------------------------------

[[nodiscard]] std::size_t authenticate_mirror_group(
    const FakeBoundGeometry& geometry,
    const NodeGroup& group) {
  const std::size_t node_index = checked_size(
      group.node_index,
      "a fake Phase 15 higher-support node index does not fit size_t");
  if (node_index >= geometry.nodes.size()) {
    throw std::logic_error(
        "a fake Phase 15 higher-support group references a node outside "
        "the LBVH mirror");
  }
  const FakeMirrorNode& node = geometry.nodes[node_index];
  if (group.leaf_begin != checked_u64(
                              node.leaf_begin,
                              "a fake Phase 15 Morton range does not fit "
                              "uint64") ||
      group.leaf_end != checked_u64(
                            node.leaf_end,
                            "a fake Phase 15 Morton range does not fit "
                            "uint64")) {
    throw std::logic_error(
        "a fake Phase 15 higher-support group contradicts its LBVH mirror "
        "node receipt");
  }
  return node_index;
}

[[nodiscard]] spatial::ExactDyadicAabb3 mirror_node_box(
    const FakeBoundGeometry& geometry,
    std::size_t node_index) {
  const FakeMirrorNode& node = geometry.nodes[node_index];
  spatial::ExactDyadicAabb3 box{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    box.lower_binary64_bits[axis] = node.lower_bits[axis];
    box.upper_binary64_bits[axis] = node.upper_bits[axis];
  }
  return box;
}

[[nodiscard]] std::array<spatial::ExactDyadicAabb3, 4> mirror_support_boxes(
    const FakeBoundGeometry& geometry,
    const FrontierEntry& entry) {
  std::array<spatial::ExactDyadicAabb3, 4> boxes{};
  std::size_t output_index = 0U;
  for (std::size_t group_index = 0U;
       group_index < entry.group_count;
       ++group_index) {
    const NodeGroup& group = entry.groups[group_index];
    const spatial::ExactDyadicAabb3 box = mirror_node_box(
        geometry, authenticate_mirror_group(geometry, group));
    for (std::size_t copy = 0U; copy < group.multiplicity; ++copy) {
      if (output_index >= entry.support_size) {
        throw std::logic_error(
            "a fake Phase 15 higher-support product overflowed its "
            "support boxes");
      }
      boxes[output_index] = box;
      ++output_index;
    }
  }
  if (output_index != entry.support_size) {
    throw std::logic_error(
        "a fake Phase 15 higher-support product omitted a support box");
  }
  return boxes;
}

[[nodiscard]] bool mirror_entry_terminal(
    const FakeBoundGeometry& geometry,
    const FrontierEntry& entry) {
  if (entry.group_count != entry.support_size) {
    return false;
  }
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const std::size_t node_index =
        authenticate_mirror_group(geometry, entry.groups[index]);
    if (entry.groups[index].multiplicity != 1U ||
        !geometry.nodes[node_index].is_leaf()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool leaf_position_in_support_domain(
    const FakeBoundGeometry& geometry,
    std::size_t leaf_position,
    const FrontierEntry& entry) {
  static_cast<void>(geometry);
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const NodeGroup& group = entry.groups[index];
    if (group.leaf_begin <= leaf_position &&
        leaf_position < group.leaf_end) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool node_range_intersects_support_domain(
    const FakeMirrorNode& query,
    const FrontierEntry& entry) {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const NodeGroup& group = entry.groups[index];
    if (checked_u64(
            query.leaf_begin,
            "a fake Phase 15 Morton range does not fit uint64") <
            group.leaf_end &&
        group.leaf_begin < checked_u64(
                               query.leaf_end,
                               "a fake Phase 15 Morton range does not fit "
                               "uint64")) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::size_t possible_external_witness_count(
    const FakeBoundGeometry& geometry,
    const FrontierEntry& entry) {
  std::size_t excluded = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const NodeGroup& group = entry.groups[index];
    excluded = checked_size_sum(
        excluded,
        checked_size(
            group.leaf_end - group.leaf_begin,
            "a fake Phase 15 Morton range does not fit size_t"),
        "the fake Phase 15 exclusion count overflows size_t");
  }
  if (excluded > geometry.leaves.size()) {
    throw std::logic_error(
        "fake Phase 15 higher-support groups exclude more points than the "
        "cloud contains");
  }
  return geometry.leaves.size() - excluded;
}

// Highest ancestor of the leaf position whose point mass is at most
// maximum_point_count and whose Morton range does not intersect the support
// domain, found by descent from the root exactly as in the CPU stream.
[[nodiscard]] std::size_t bounded_external_node_index_at_position(
    const FakeBoundGeometry& geometry,
    std::size_t leaf_position,
    const FrontierEntry& entry,
    std::size_t maximum_point_count) {
  if (leaf_position >= geometry.leaves.size()) {
    throw std::out_of_range(
        "a fake Phase 15 local rank probe leaf position is outside the "
        "LBVH mirror");
  }
  if (maximum_point_count == 0U ||
      leaf_position_in_support_domain(geometry, leaf_position, entry)) {
    throw std::logic_error(
        "a fake Phase 15 local rank probe requires one external leaf and a "
        "positive cell bound");
  }
  std::size_t current_index = geometry.root_index;
  while (true) {
    const FakeMirrorNode& current = geometry.nodes[current_index];
    if (current.leaf_end - current.leaf_begin <= maximum_point_count &&
        !node_range_intersects_support_domain(current, entry)) {
      return current_index;
    }
    if (current.is_leaf()) {
      throw std::logic_error(
          "an external fake Phase 15 leaf did not yield a bounded "
          "external LBVH cell");
    }
    const FakeMirrorNode& left = geometry.nodes[current.left_child];
    const FakeMirrorNode& right = geometry.nodes[current.right_child];
    if (left.leaf_begin <= leaf_position &&
        leaf_position < left.leaf_end) {
      current_index = current.left_child;
    } else if (
        right.leaf_begin <= leaf_position &&
        leaf_position < right.leaf_end) {
      current_index = current.right_child;
    } else {
      throw std::logic_error(
          "a fake Phase 15 local rank probe lost its LBVH leaf position");
    }
  }
}

[[nodiscard]] std::size_t leaf_node_index_below(
    const FakeBoundGeometry& geometry,
    std::size_t ancestor_node_index,
    std::size_t leaf_position) {
  std::size_t current_index = ancestor_node_index;
  while (true) {
    const FakeMirrorNode& current = geometry.nodes[current_index];
    if (!(current.leaf_begin <= leaf_position &&
          leaf_position < current.leaf_end)) {
      throw std::logic_error(
          "a fake Phase 15 fallback leaf left its bounded candidate cell");
    }
    if (current.is_leaf()) {
      return current_index;
    }
    const FakeMirrorNode& left = geometry.nodes[current.left_child];
    const FakeMirrorNode& right = geometry.nodes[current.right_child];
    if (left.leaf_begin <= leaf_position &&
        leaf_position < left.leaf_end) {
      current_index = current.left_child;
    } else if (
        right.leaf_begin <= leaf_position &&
        leaf_position < right.leaf_end) {
      current_index = current.right_child;
    } else {
      throw std::logic_error(
          "a fake Phase 15 fallback path lost its leaf position");
    }
  }
}

// Slot-local probe plan under the frozen substitute v1 seed: the seed is
// the Morton-lowest support position (leaf_begin of the first group), never
// an exact centre descent.  Position order, linear deduplication, cell
// grouping by first appearance, per-position fallback leaves and the
// antichain contract follow the CPU stream verbatim.
[[nodiscard]] std::vector<FakeProbeCell> build_probe_plan(
    const FakeBoundGeometry& geometry,
    const FrontierEntry& entry,
    std::size_t threshold) {
  if (threshold == 0U ||
      threshold >
          hierarchy::higher_support_local_rank_probe_maximum_threshold) {
    throw std::logic_error(
        "a fake Phase 15 local rank probe threshold is outside [1, 9]");
  }
  const std::size_t leaf_count = geometry.leaves.size();
  std::vector<std::size_t> leaf_positions;
  leaf_positions.reserve(
      hierarchy::higher_support_local_rank_probe_maximum_candidate_count);
  const auto append_leaf_position =
      [&geometry, &entry, &leaf_positions, leaf_count](
          std::size_t position) {
        if (position >= leaf_count ||
            leaf_position_in_support_domain(geometry, position, entry) ||
            std::find(
                leaf_positions.begin(), leaf_positions.end(), position) !=
                leaf_positions.end()) {
          return;
        }
        leaf_positions.push_back(position);
      };

  const std::size_t seed = checked_size(
      entry.groups[0].leaf_begin,
      "a fake Phase 15 substitute probe seed does not fit size_t");
  append_leaf_position(seed);
  for (std::size_t distance = 1U; distance <= threshold; ++distance) {
    if (seed >= distance) {
      append_leaf_position(seed - distance);
    }
    if (distance < leaf_count && seed < leaf_count - distance) {
      append_leaf_position(seed + distance);
    }
  }
  for (std::size_t distance = 1U; distance <= threshold; ++distance) {
    for (std::size_t group_index = 0U;
         group_index < entry.group_count;
         ++group_index) {
      const std::size_t group_begin = checked_size(
          entry.groups[group_index].leaf_begin,
          "a fake Phase 15 Morton range does not fit size_t");
      const std::size_t group_end = checked_size(
          entry.groups[group_index].leaf_end,
          "a fake Phase 15 Morton range does not fit size_t");
      if (group_begin >= distance) {
        append_leaf_position(group_begin - distance);
      }
      if (distance <= leaf_count && group_end <= leaf_count - distance) {
        append_leaf_position(group_end + distance - 1U);
      }
    }
  }
  if (leaf_positions.size() >
      hierarchy::higher_support_local_rank_probe_maximum_candidate_count) {
    throw std::logic_error(
        "a fake Phase 15 local rank probe exceeded its fixed candidate "
        "bound");
  }

  std::vector<FakeProbeCell> candidate_cells;
  candidate_cells.reserve(leaf_positions.size());
  for (const std::size_t position : leaf_positions) {
    const std::size_t candidate_node_index =
        bounded_external_node_index_at_position(
            geometry, position, entry, threshold);
    auto candidate = std::find_if(
        candidate_cells.begin(),
        candidate_cells.end(),
        [candidate_node_index](const FakeProbeCell& cell) {
          return cell.root_node_index == candidate_node_index;
        });
    if (candidate == candidate_cells.end()) {
      candidate_cells.push_back(FakeProbeCell{candidate_node_index, {}});
      candidate = std::prev(candidate_cells.end());
    }
    candidate->fallback_leaf_node_indices.push_back(
        leaf_node_index_below(geometry, candidate_node_index, position));
  }

  std::size_t maximum_evaluation_count = candidate_cells.size();
  for (std::size_t left = 0U; left < candidate_cells.size(); ++left) {
    const FakeMirrorNode& left_node =
        geometry.nodes[candidate_cells[left].root_node_index];
    if (left_node.leaf_end - left_node.leaf_begin > threshold ||
        node_range_intersects_support_domain(left_node, entry) ||
        candidate_cells[left].fallback_leaf_node_indices.empty()) {
      throw std::logic_error(
          "a fake Phase 15 local rank root violates its bounded "
          "external-cell contract");
    }
    maximum_evaluation_count = checked_size_sum(
        maximum_evaluation_count,
        candidate_cells[left].fallback_leaf_node_indices.size(),
        "the fake Phase 15 local rank plan size overflows size_t");
    for (const std::size_t fallback_leaf_node_index :
         candidate_cells[left].fallback_leaf_node_indices) {
      const FakeMirrorNode& fallback_leaf =
          geometry.nodes[fallback_leaf_node_index];
      if (!fallback_leaf.is_leaf() ||
          fallback_leaf.leaf_begin < left_node.leaf_begin ||
          fallback_leaf.leaf_end > left_node.leaf_end) {
        throw std::logic_error(
            "a fake Phase 15 fallback is not a leaf of its bounded "
            "candidate cell");
      }
    }
    for (std::size_t right = left + 1U;
         right < candidate_cells.size();
         ++right) {
      const FakeMirrorNode& right_node =
          geometry.nodes[candidate_cells[right].root_node_index];
      if (left_node.leaf_begin < right_node.leaf_end &&
          right_node.leaf_begin < left_node.leaf_end) {
        throw std::logic_error(
            "fake Phase 15 local rank roots do not form an LBVH "
            "antichain");
      }
    }
  }
  if (maximum_evaluation_count >
      hierarchy::higher_support_local_rank_probe_maximum_evaluation_count) {
    throw std::logic_error(
        "a fake Phase 15 local rank plan exceeded its fixed evaluation "
        "bound");
  }
  return candidate_cells;
}

// Canonical child construction, identical to the CPU make_entry: groups are
// sorted by (leaf_begin, node_index) and every multiplicity is checked
// against its Morton range.
[[nodiscard]] FrontierEntry make_mirror_entry(
    const FakeBoundGeometry& geometry,
    std::size_t support_size,
    std::vector<std::pair<std::size_t, std::size_t>> groups) {
  std::sort(
      groups.begin(),
      groups.end(),
      [&geometry](const auto& left, const auto& right) {
        const FakeMirrorNode& left_node = geometry.nodes[left.first];
        const FakeMirrorNode& right_node = geometry.nodes[right.first];
        if (left_node.leaf_begin != right_node.leaf_begin) {
          return left_node.leaf_begin < right_node.leaf_begin;
        }
        return left.first < right.first;
      });
  if (groups.empty() || groups.size() > support_size ||
      groups.size() > 4U) {
    throw std::logic_error(
        "a fake Phase 15 higher-support product has an invalid group "
        "count");
  }
  FrontierEntry entry{};
  entry.support_size = static_cast<std::uint8_t>(support_size);
  entry.group_count = static_cast<std::uint8_t>(groups.size());
  for (std::size_t index = 0U; index < groups.size(); ++index) {
    const std::size_t node_index = groups[index].first;
    const std::size_t multiplicity = groups[index].second;
    if (node_index >= geometry.nodes.size()) {
      throw std::logic_error(
          "a fake Phase 15 higher-support child references a node outside "
          "the LBVH mirror");
    }
    const FakeMirrorNode& node = geometry.nodes[node_index];
    if (multiplicity == 0U || multiplicity > 4U ||
        node.leaf_end - node.leaf_begin < multiplicity) {
      throw std::logic_error(
          "a fake Phase 15 higher-support group has an invalid "
          "multiplicity");
    }
    entry.groups[index] = NodeGroup{
        checked_u64(
            node_index,
            "a fake Phase 15 node index does not fit uint64"),
        checked_u64(
            node.leaf_begin,
            "a fake Phase 15 Morton range does not fit uint64"),
        checked_u64(
            node.leaf_end,
            "a fake Phase 15 Morton range does not fit uint64"),
        static_cast<std::uint8_t>(multiplicity)};
  }
  return entry;
}

// Fail-closed mirror validation of one host-submitted root entry.
void validate_fake_root_entry(
    const FakeBoundGeometry& geometry,
    const FrontierEntry& entry,
    HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy) {
  const std::size_t support_size = entry.support_size;
  const std::size_t group_count = entry.group_count;
  const bool carrier = terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::
          strict_interior_carrier_q3;
  if ((support_size != 3U && support_size != 4U) || group_count == 0U ||
      group_count > support_size || group_count > entry.groups.size() ||
      (carrier && support_size != 3U)) {
    throw std::invalid_argument(
        "a fake Phase 15 higher-support root entry has an invalid arity "
        "for its terminal policy");
  }
  std::size_t multiplicity_sum = 0U;
  std::uint64_t previous_end = 0U;
  for (std::size_t index = 0U; index < group_count; ++index) {
    const NodeGroup& group = entry.groups[index];
    const std::size_t node_index = checked_size(
        group.node_index,
        "a fake Phase 15 node index does not fit size_t");
    if (node_index >= geometry.nodes.size()) {
      throw std::invalid_argument(
          "a fake Phase 15 higher-support root entry references a node "
          "outside the LBVH mirror");
    }
    const FakeMirrorNode& node = geometry.nodes[node_index];
    if (group.leaf_begin != checked_u64(
                                node.leaf_begin,
                                "a fake Phase 15 Morton range does not fit "
                                "uint64") ||
        group.leaf_end != checked_u64(
                              node.leaf_end,
                              "a fake Phase 15 Morton range does not fit "
                              "uint64") ||
        group.multiplicity == 0U || group.multiplicity > 4U ||
        group.leaf_begin >= group.leaf_end ||
        group.leaf_end - group.leaf_begin < group.multiplicity ||
        (index != 0U && previous_end > group.leaf_begin)) {
      throw std::invalid_argument(
          "a fake Phase 15 higher-support root entry has overlapping or "
          "unauthenticated groups");
    }
    previous_end = group.leaf_end;
    multiplicity_sum = checked_size_sum(
        multiplicity_sum,
        group.multiplicity,
        "a fake Phase 15 multiplicity sum overflows size_t");
  }
  const NodeGroup padding{};
  for (std::size_t index = group_count; index < entry.groups.size();
       ++index) {
    if (entry.groups[index] != padding) {
      throw std::invalid_argument(
          "a fake Phase 15 higher-support root entry has noncanonical "
          "padding");
    }
  }
  if (multiplicity_sum != support_size) {
    throw std::invalid_argument(
        "a fake Phase 15 higher-support root entry does not distribute "
        "its support size");
  }
}

// -------------------------------------------------------------------------
// Chunk workspace.
// -------------------------------------------------------------------------

// Host-retained record segments of one chunk.  These vectors are the
// authorities behind the batch views and are pinned by the drain lease as
// retained_output_owner for the whole chunk lifetime.
struct FakeOutputStorage {
  std::vector<Phase15HigherSupportDeviceTiledPruneRecord> prune_records;
  std::vector<Phase15HigherSupportDeviceTiledTerminalRecord>
      terminal_records;
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt> probe_receipts;
  std::vector<Phase15HigherSupportDeviceTiledSlotControl> slot_controls;
};

enum class FakeSlotRunState : std::uint8_t {
  active,
  paused,
  chunk_ready,
  complete,
  fatal,
};

struct FakeSlotChunk {
  FakeU128 well_delta{};
  FakeU128 rank_delta{};
  FakeU128 terminal_delta{};
  std::uint64_t expansion_count{};
  std::uint64_t gate_evaluation_count{};
  std::uint64_t probe_root_decision_count{};
  std::uint64_t probe_fallback_decision_count{};
  std::size_t prune_used{};
  std::size_t terminal_used{};
  std::size_t receipt_used{};
  FakeSlotRunState run_state{FakeSlotRunState::active};
  Phase15HigherSupportDeviceTiledYieldReason yield_reason{
      Phase15HigherSupportDeviceTiledYieldReason::none};
  Phase15HigherSupportDeviceTiledStopReason stop_reason{
      Phase15HigherSupportDeviceTiledStopReason::none};
  Phase15HigherSupportDeviceTiledFailureCode failure_code{
      Phase15HigherSupportDeviceTiledFailureCode::none};
};

enum class FakeProbeOutcome : std::uint8_t {
  paused,
  threshold_reached,
  fail_open,
};

// One-slot executor for one internal subdivision pass.  Every subdivision
// grants the slot a fresh quantum of gate evaluations; segment-full yields
// end the slot for the whole chunk while quantum pauses only end the pass.
class FakeSlotExecutor final {
 public:
  FakeSlotExecutor(
      const FakeBoundGeometry& geometry,
      const Phase15HigherSupportDeviceTiledRequest& request,
      FakeOutputStorage& storage,
      FakeSlotState& slot,
      FakeSlotChunk& chunk,
      std::size_t slot_index)
      : geometry_(geometry),
        request_(request),
        storage_(storage),
        slot_(slot),
        chunk_(chunk),
        slot_index_(slot_index) {}

  void run_subdivision() {
    budget_ = request_.gate_evaluations_per_slot_per_chunk;
    while (chunk_.run_state == FakeSlotRunState::active) {
      if (slot_.probe.has_value()) {
        if (slot_.stack.empty()) {
          throw std::logic_error(
              "a fake Phase 15 probe cursor is detached from its slot "
              "stack");
        }
        const FakeProbeOutcome outcome = continue_probe();
        if (outcome == FakeProbeOutcome::paused) {
          chunk_.run_state = FakeSlotRunState::paused;
          return;
        }
        if (outcome == FakeProbeOutcome::threshold_reached) {
          emit_rank_prune_from_probe();
          continue;
        }
        finish_fail_open_probe();
        continue;
      }
      if (slot_.pending_terminal) {
        classify_pending_terminal();
        continue;
      }
      if (slot_.stack.empty()) {
        chunk_.run_state = FakeSlotRunState::complete;
        return;
      }
      process_stack_top();
    }
  }

 private:
  [[nodiscard]] bool consume_gate() {
    if (budget_ == 0U) {
      return false;
    }
    --budget_;
    checked_increment(
        chunk_.gate_evaluation_count,
        "the fake Phase 15 gate evaluation count overflows uint64");
    return true;
  }

  void yield_chunk(Phase15HigherSupportDeviceTiledYieldReason reason) {
    chunk_.run_state = FakeSlotRunState::chunk_ready;
    chunk_.yield_reason = reason;
  }

  [[nodiscard]] std::size_t required_witness_count(
      std::size_t support_size) const {
    return higher_support_device_tiled_frontier_required_witness_count(
        request_.terminal_policy,
        support_size,
        request_.maximum_relevant_closed_rank);
  }

  [[nodiscard]] bool carrier_policy() const noexcept {
    return request_.terminal_policy ==
        HigherSupportDeviceTiledFrontierTerminalPolicy::
            strict_interior_carrier_q3;
  }

  void process_stack_top() {
    // The analyze stage evaluates at most two exact gates atomically, so a
    // fresh quantum of at least two always makes progress; a resumed slot
    // re-derives the same certified verdicts deterministically.
    if (budget_ < 2U) {
      chunk_.run_state = FakeSlotRunState::paused;
      return;
    }
    const FrontierEntry entry = slot_.stack.back();
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        mirror_support_boxes(geometry_, entry);
    const std::span<const spatial::ExactDyadicAabb3> support_span{
        boxes.data(), entry.support_size};
    static_cast<void>(consume_gate());
    if (hierarchy::exact_higher_support_product_no_well_centered_certified(
            support_span)) {
      emit_well_prune(entry);
      return;
    }
    const std::size_t threshold = required_witness_count(entry.support_size);
    if (threshold == 0U) {
      emit_rank_prune(
          entry,
          Phase15HigherSupportDeviceTiledPruneKind::rank_window,
          std::span<const Phase15HigherSupportDeviceTiledProbeReceipt>{});
      return;
    }
    static_cast<void>(consume_gate());
    if (hierarchy::exact_higher_support_product_all_well_centered_certified(
            support_span) &&
        possible_external_witness_count(geometry_, entry) >= threshold) {
      FakeSlotProbe probe;
      probe.threshold = threshold;
      probe.cells = build_probe_plan(geometry_, entry, threshold);
      slot_.probe.emplace(std::move(probe));
      return;
    }
    // Exact fail-open continuation: no support tuple is discarded and the
    // gates are reconsidered on tighter cells.
    if (mirror_entry_terminal(geometry_, entry)) {
      slot_.pending_terminal = true;
      return;
    }
    expand_stack_top();
  }

  [[nodiscard]] FakeProbeOutcome continue_probe() {
    FakeSlotProbe& probe = *slot_.probe;
    const FrontierEntry entry = slot_.stack.back();
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        mirror_support_boxes(geometry_, entry);
    const std::span<const spatial::ExactDyadicAabb3> support_span{
        boxes.data(), entry.support_size};
    while (probe.fallback_active ||
           probe.next_cell_index < probe.cells.size()) {
      if (!consume_gate()) {
        return FakeProbeOutcome::paused;
      }
      const bool evaluating_fallback = probe.fallback_active;
      const std::size_t cell_index = evaluating_fallback
          ? probe.next_cell_index - 1U
          : probe.next_cell_index;
      const FakeProbeCell& cell = probe.cells[cell_index];
      std::size_t query_node_index = cell.root_node_index;
      if (evaluating_fallback) {
        query_node_index =
            cell.fallback_leaf_node_indices[probe.next_fallback_index];
        ++probe.next_fallback_index;
        if (probe.next_fallback_index ==
            cell.fallback_leaf_node_indices.size()) {
          probe.next_fallback_index = 0U;
          probe.fallback_active = false;
        }
        checked_increment(
            chunk_.probe_fallback_decision_count,
            "the fake Phase 15 fallback decision count overflows uint64");
      } else {
        ++probe.next_cell_index;
        checked_increment(
            chunk_.probe_root_decision_count,
            "the fake Phase 15 root decision count overflows uint64");
      }
      const FakeMirrorNode& query = geometry_.nodes[query_node_index];
      if (query.leaf_end - query.leaf_begin > probe.threshold ||
          node_range_intersects_support_domain(query, entry)) {
        throw std::logic_error(
            "a fake Phase 15 local rank cursor left its bounded external "
            "LBVH cell");
      }
      const hierarchy::ExactHigherSupportProductQueryCellDecision decision =
          hierarchy::exact_higher_support_product_query_cell_decision(
              support_span, mirror_node_box(geometry_, query_node_index));
      if (decision ==
          hierarchy::ExactHigherSupportProductQueryCellDecision::
              strictly_inside_every_independent_sphere) {
        const std::uint64_t point_count = checked_u64(
            query.leaf_end - query.leaf_begin,
            "a fake Phase 15 probe cell mass does not fit uint64");
        probe.certified_point_count = checked_u64_sum(
            probe.certified_point_count,
            point_count,
            "the fake Phase 15 certified witness mass overflows uint64");
        probe.pending_receipts.push_back(
            Phase15HigherSupportDeviceTiledProbeReceipt{
                checked_u64(
                    query_node_index,
                    "a fake Phase 15 receipt node does not fit uint64"),
                checked_u64(
                    query.leaf_begin,
                    "a fake Phase 15 receipt range does not fit uint64"),
                checked_u64(
                    query.leaf_end,
                    "a fake Phase 15 receipt range does not fit uint64"),
                point_count});
        if (probe.certified_point_count >=
            checked_u64(
                probe.threshold,
                "a fake Phase 15 probe threshold does not fit uint64")) {
          probe.next_fallback_index = 0U;
          probe.fallback_active = false;
          return FakeProbeOutcome::threshold_reached;
        }
        continue;
      }
      if (decision ==
          hierarchy::ExactHigherSupportProductQueryCellDecision::
              outside_or_boundary_every_independent_sphere) {
        continue;
      }
      if (!evaluating_fallback && !query.is_leaf()) {
        probe.next_fallback_index = 0U;
        probe.fallback_active = true;
      }
    }
    return FakeProbeOutcome::fail_open;
  }

  void emit_rank_prune_from_probe() {
    const FrontierEntry entry = slot_.stack.back();
    const std::vector<Phase15HigherSupportDeviceTiledProbeReceipt>
        receipts = slot_.probe->pending_receipts;
    emit_rank_prune(
        entry,
        carrier_policy()
            ? Phase15HigherSupportDeviceTiledPruneKind::
                  strict_interior_carrier
            : Phase15HigherSupportDeviceTiledPruneKind::rank_window,
        std::span<const Phase15HigherSupportDeviceTiledProbeReceipt>{
            receipts});
  }

  void finish_fail_open_probe() {
    slot_.probe.reset();
    const FrontierEntry entry = slot_.stack.back();
    if (mirror_entry_terminal(geometry_, entry)) {
      slot_.pending_terminal = true;
      return;
    }
    expand_stack_top();
  }

  void fill_prune_identity(
      Phase15HigherSupportDeviceTiledPruneRecord& record,
      const FrontierEntry& entry) const {
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      record.group_node_index[index] = entry.groups[index].node_index;
      record.group_leaf_begin[index] = entry.groups[index].leaf_begin;
      record.group_leaf_end[index] = entry.groups[index].leaf_end;
      record.group_multiplicity[index] = entry.groups[index].multiplicity;
    }
    record.support_size = entry.support_size;
    record.group_count = entry.group_count;
    record.terminal_policy =
        static_cast<std::uint8_t>(request_.terminal_policy);
    record.flags = carrier_policy()
        ? phase15_higher_support_device_tiled_flag_carrier_policy
        : std::uint8_t{0U};
  }

  void emit_well_prune(const FrontierEntry& entry) {
    if (chunk_.prune_used == request_.prune_records_per_slot) {
      yield_chunk(
          Phase15HigherSupportDeviceTiledYieldReason::prune_segment_full);
      return;
    }
    const FakeU128 mass = fake_entry_mass(entry);
    Phase15HigherSupportDeviceTiledPruneRecord record{};
    fill_prune_identity(record, entry);
    record.pruned_mass_lo = mass.lo;
    record.pruned_mass_hi = mass.hi;
    record.prune_kind = static_cast<std::uint64_t>(
        Phase15HigherSupportDeviceTiledPruneKind::no_well_centered_support);
    record.receipt_count = 0U;
    record.receipt_segment_begin = checked_u64(
        chunk_.receipt_used,
        "a fake Phase 15 receipt offset does not fit uint64");
    storage_.prune_records[checked_size_sum(
        checked_size_product(
            slot_index_,
            request_.prune_records_per_slot,
            "the fake Phase 15 prune segment offset overflows size_t"),
        chunk_.prune_used,
        "the fake Phase 15 prune segment offset overflows size_t")] =
        record;
    ++chunk_.prune_used;
    fake_u128_accumulate(
        chunk_.well_delta,
        mass,
        "the fake Phase 15 well-prune delta overflows unsigned 128 bits");
    slot_.stack.pop_back();
    slot_.pending_terminal = false;
  }

  // Receipts, when present, are committed in plan order into the fixed
  // per-slot receipt segment; receipt_segment_begin is the offset within
  // this slot's segment for the current chunk.  A prune whose record or
  // receipts no longer fit yields the slot resumably and the untouched
  // product is re-derived on the next chunk.
  void emit_rank_prune(
      const FrontierEntry& entry,
      Phase15HigherSupportDeviceTiledPruneKind kind,
      std::span<const Phase15HigherSupportDeviceTiledProbeReceipt>
          receipts) {
    if (chunk_.prune_used == request_.prune_records_per_slot ||
        checked_size_sum(
            chunk_.receipt_used,
            receipts.size(),
            "the fake Phase 15 receipt count overflows size_t") >
            request_.probe_receipts_per_slot) {
      slot_.probe.reset();
      yield_chunk(
          Phase15HigherSupportDeviceTiledYieldReason::prune_segment_full);
      return;
    }
    const FakeU128 mass = fake_entry_mass(entry);
    Phase15HigherSupportDeviceTiledPruneRecord record{};
    fill_prune_identity(record, entry);
    record.pruned_mass_lo = mass.lo;
    record.pruned_mass_hi = mass.hi;
    record.prune_kind = static_cast<std::uint64_t>(kind);
    record.receipt_count = checked_u64(
        receipts.size(),
        "a fake Phase 15 receipt count does not fit uint64");
    record.receipt_segment_begin = checked_u64(
        chunk_.receipt_used,
        "a fake Phase 15 receipt offset does not fit uint64");
    const std::size_t receipt_base = checked_size_product(
        slot_index_,
        request_.probe_receipts_per_slot,
        "the fake Phase 15 receipt segment offset overflows size_t");
    for (std::size_t index = 0U; index < receipts.size(); ++index) {
      storage_.probe_receipts[checked_size_sum(
          receipt_base,
          chunk_.receipt_used + index,
          "the fake Phase 15 receipt segment offset overflows size_t")] =
          receipts[index];
    }
    storage_.prune_records[checked_size_sum(
        checked_size_product(
            slot_index_,
            request_.prune_records_per_slot,
            "the fake Phase 15 prune segment offset overflows size_t"),
        chunk_.prune_used,
        "the fake Phase 15 prune segment offset overflows size_t")] =
        record;
    ++chunk_.prune_used;
    chunk_.receipt_used = chunk_.receipt_used + receipts.size();
    fake_u128_accumulate(
        chunk_.rank_delta,
        mass,
        "the fake Phase 15 rank-prune delta overflows unsigned 128 bits");
    slot_.stack.pop_back();
    slot_.probe.reset();
    slot_.pending_terminal = false;
  }

  void classify_pending_terminal() {
    if (slot_.stack.empty()) {
      throw std::logic_error(
          "a fake Phase 15 pending terminal product is detached from its "
          "slot stack");
    }
    if (chunk_.terminal_used == request_.terminal_records_per_slot) {
      yield_chunk(
          Phase15HigherSupportDeviceTiledYieldReason::
              terminal_segment_full);
      return;
    }
    if (!consume_gate()) {
      chunk_.run_state = FakeSlotRunState::paused;
      return;
    }
    const FrontierEntry entry = slot_.stack.back();
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        mirror_support_boxes(geometry_, entry);
    hierarchy::ExactHigherSupportProductAabbDecisionBackend backend{};
    const hierarchy::ExactHigherSupportTerminalGeometryDecision decision =
        hierarchy::exact_higher_support_terminal_geometry_decision(
            std::span<const spatial::ExactDyadicAabb3>{
                boxes.data(), entry.support_size},
            &backend);
    std::array<std::uint64_t, 4> point_ids{};
    const std::size_t sorted_support_size = entry.support_size;
    if (sorted_support_size > point_ids.size()) {
      throw std::logic_error(
          "a fake Phase 15 terminal support size exceeds the point id "
          "buffer");
    }
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const std::size_t node_index =
          authenticate_mirror_group(geometry_, entry.groups[index]);
      point_ids[index] =
          geometry_.leaves[geometry_.nodes[node_index].leaf_begin].point_id;
    }
    for (std::size_t sorted = 1U; sorted < sorted_support_size; ++sorted) {
      const std::uint64_t value = point_ids[sorted];
      std::size_t slot = sorted;
      while (slot > 0U && point_ids[slot - 1U] > value) {
        point_ids[slot] = point_ids[slot - 1U];
        --slot;
      }
      point_ids[slot] = value;
    }
    if (std::adjacent_find(
            point_ids.begin(), point_ids.begin() + sorted_support_size) !=
        point_ids.begin() + sorted_support_size) {
      throw std::logic_error(
          "a fake Phase 15 terminal higher-support product repeated a "
          "point");
    }
    Phase15HigherSupportDeviceTiledTerminalRecord record{};
    for (std::size_t index = 0U; index < point_ids.size(); ++index) {
      record.point_ids[index] = point_ids[index];
    }
    record.slot_index = checked_u64(
        slot_index_,
        "a fake Phase 15 slot index does not fit uint64");
    record.support_size = entry.support_size;
    record.terminal_geometry_decision =
        static_cast<std::uint8_t>(decision);
    record.decision_backend = static_cast<std::uint8_t>(backend);
    record.terminal_policy =
        static_cast<std::uint8_t>(request_.terminal_policy);
    record.flags = carrier_policy()
        ? phase15_higher_support_device_tiled_flag_carrier_policy
        : std::uint8_t{0U};
    storage_.terminal_records[checked_size_sum(
        checked_size_product(
            slot_index_,
            request_.terminal_records_per_slot,
            "the fake Phase 15 terminal segment offset overflows size_t"),
        chunk_.terminal_used,
        "the fake Phase 15 terminal segment offset overflows size_t")] =
        record;
    ++chunk_.terminal_used;
    fake_u128_accumulate(
        chunk_.terminal_delta,
        FakeU128{1U, 0U},
        "the fake Phase 15 terminal delta overflows unsigned 128 bits");
    slot_.stack.pop_back();
    slot_.pending_terminal = false;
  }

  // Exact expansion: the non-leaf group of strictly largest extent (first
  // strict winner) splits into (left, a) and (right, m - a) children whose
  // unsigned 128-bit masses must partition the parent mass exactly.
  void expand_stack_top() {
    const FrontierEntry entry = slot_.stack.back();
    std::size_t split_group_index = entry.group_count;
    std::size_t largest_range = 0U;
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const std::size_t node_index =
          authenticate_mirror_group(geometry_, entry.groups[index]);
      const FakeMirrorNode& current = geometry_.nodes[node_index];
      const std::size_t range = current.leaf_end - current.leaf_begin;
      if (!current.is_leaf() && range > largest_range) {
        split_group_index = index;
        largest_range = range;
      }
    }
    if (split_group_index == entry.group_count) {
      throw std::logic_error(
          "a fake Phase 15 nonterminal higher-support product has no "
          "splittable group");
    }
    const NodeGroup& split_group = entry.groups[split_group_index];
    const std::size_t parent_node_index =
        authenticate_mirror_group(geometry_, split_group);
    const FakeMirrorNode& parent = geometry_.nodes[parent_node_index];
    const FakeMirrorNode& left = geometry_.nodes[parent.left_child];
    const FakeMirrorNode& right = geometry_.nodes[parent.right_child];
    const std::size_t multiplicity = split_group.multiplicity;
    const std::size_t left_size = left.leaf_end - left.leaf_begin;
    const std::size_t right_size = right.leaf_end - right.leaf_begin;
    const std::size_t minimum_left =
        multiplicity > right_size ? multiplicity - right_size : 0U;
    const std::size_t maximum_left = std::min(multiplicity, left_size);
    if (minimum_left > maximum_left) {
      throw std::logic_error(
          "a fake Phase 15 higher-support split has no feasible "
          "multiplicity distribution");
    }
    const FakeU128 parent_mass = fake_entry_mass(entry);
    std::vector<FrontierEntry> children;
    children.reserve(maximum_left - minimum_left + 1U);
    FakeU128 child_mass_sum{};
    for (std::size_t left_multiplicity = minimum_left;
         left_multiplicity <= maximum_left;
         ++left_multiplicity) {
      std::vector<std::pair<std::size_t, std::size_t>> groups;
      groups.reserve(4U);
      for (std::size_t index = 0U; index < entry.group_count; ++index) {
        if (index != split_group_index) {
          groups.emplace_back(
              authenticate_mirror_group(geometry_, entry.groups[index]),
              entry.groups[index].multiplicity);
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
      children.push_back(make_mirror_entry(
          geometry_, entry.support_size, std::move(groups)));
      fake_u128_accumulate(
          child_mass_sum,
          fake_entry_mass(children.back()),
          "the fake Phase 15 expansion partition overflows unsigned 128 "
          "bits");
    }
    if (child_mass_sum != parent_mass) {
      throw std::logic_error(
          "a fake Phase 15 grouped higher-support split did not partition "
          "its parent");
    }
    const std::size_t resident_after = checked_size_sum(
        slot_.stack.size() - 1U,
        children.size(),
        "the fake Phase 15 slot stack size overflows size_t");
    if (resident_after > request_.products_per_slot) {
      // The proved bound 16*d_T + 1 is contradicted: this is a fatal
      // internal-invariant stop, never a resumable yield.
      chunk_.run_state = FakeSlotRunState::fatal;
      chunk_.stop_reason =
          Phase15HigherSupportDeviceTiledStopReason::fatal_failure;
      chunk_.failure_code = Phase15HigherSupportDeviceTiledFailureCode::
          stack_capacity_exceeded;
      return;
    }
    slot_.stack.pop_back();
    for (auto iterator = children.rbegin(); iterator != children.rend();
         ++iterator) {
      slot_.stack.push_back(std::move(*iterator));
    }
    slot_.stack_high_water = std::max(
        slot_.stack_high_water,
        checked_u64(
            slot_.stack.size(),
            "a fake Phase 15 stack high water does not fit uint64"));
    checked_increment(
        chunk_.expansion_count,
        "the fake Phase 15 expansion count overflows uint64");
  }

  const FakeBoundGeometry& geometry_;
  const Phase15HigherSupportDeviceTiledRequest& request_;
  FakeOutputStorage& storage_;
  FakeSlotState& slot_;
  FakeSlotChunk& chunk_;
  std::size_t slot_index_{};
  std::size_t budget_{};
};

// Rolls one slot's chunk back to its previous commit: zero deltas and
// counters, no published record, cumulative masses untouched.  Used for
// every fatal outcome so a fatal slot can never publish partial output.
void rollback_slot_chunk(
    const Phase15HigherSupportDeviceTiledRequest& request,
    FakeOutputStorage& storage,
    FakeSlotChunk& chunk,
    std::size_t slot_index) {
  const std::size_t prune_base = checked_size_product(
      slot_index,
      request.prune_records_per_slot,
      "the fake Phase 15 prune segment offset overflows size_t");
  const std::size_t terminal_base = checked_size_product(
      slot_index,
      request.terminal_records_per_slot,
      "the fake Phase 15 terminal segment offset overflows size_t");
  const std::size_t receipt_base = checked_size_product(
      slot_index,
      request.probe_receipts_per_slot,
      "the fake Phase 15 receipt segment offset overflows size_t");
  for (std::size_t index = 0U; index < chunk.prune_used; ++index) {
    storage.prune_records[prune_base + index] =
        Phase15HigherSupportDeviceTiledPruneRecord{};
  }
  for (std::size_t index = 0U; index < chunk.terminal_used; ++index) {
    storage.terminal_records[terminal_base + index] =
        Phase15HigherSupportDeviceTiledTerminalRecord{};
  }
  for (std::size_t index = 0U; index < chunk.receipt_used; ++index) {
    storage.probe_receipts[receipt_base + index] =
        Phase15HigherSupportDeviceTiledProbeReceipt{};
  }
  chunk.well_delta = FakeU128{};
  chunk.rank_delta = FakeU128{};
  chunk.terminal_delta = FakeU128{};
  chunk.expansion_count = 0U;
  chunk.gate_evaluation_count = 0U;
  chunk.probe_root_decision_count = 0U;
  chunk.probe_fallback_decision_count = 0U;
  chunk.prune_used = 0U;
  chunk.terminal_used = 0U;
  chunk.receipt_used = 0U;
  chunk.yield_reason = Phase15HigherSupportDeviceTiledYieldReason::none;
}

[[nodiscard]] Phase15HigherSupportDeviceTiledSlotStatus slot_status(
    FakeSlotRunState run_state) {
  switch (run_state) {
    case FakeSlotRunState::chunk_ready:
      return Phase15HigherSupportDeviceTiledSlotStatus::chunk_ready;
    case FakeSlotRunState::complete:
      return Phase15HigherSupportDeviceTiledSlotStatus::complete;
    case FakeSlotRunState::fatal:
      return Phase15HigherSupportDeviceTiledSlotStatus::fatal;
    case FakeSlotRunState::active:
    case FakeSlotRunState::paused:
      break;
  }
  throw std::logic_error(
      "a fake Phase 15 slot finished a chunk without leaving its active "
      "state");
}

void apply_control_corruption(
    Corruption corruption,
    Phase15HigherSupportDeviceTiledBatch& batch) {
  if (batch.host_slot_controls.empty()) {
    throw std::logic_error(
        "the fake Phase 15 corruption hook requires at least one slot "
        "control");
  }
  Phase15HigherSupportDeviceTiledSlotControl& control =
      batch.host_slot_controls.front();
  switch (corruption) {
    case Corruption::none:
    case Corruption::metadata_digest_corruption:
      break;
    case Corruption::stale_tile_epoch:
      --control.tile_epoch;
      break;
    case Corruption::stale_chunk_sequence:
      --control.chunk_sequence;
      break;
    case Corruption::cumulative_rollback:
      if (control.well_prune_mass_cumulative_lo == 0U &&
          control.well_prune_mass_cumulative_hi == 0U) {
        control.well_prune_mass_cumulative_lo = 1U;
      } else if (control.well_prune_mass_cumulative_lo == 0U) {
        --control.well_prune_mass_cumulative_hi;
        control.well_prune_mass_cumulative_lo =
            std::numeric_limits<std::uint64_t>::max();
      } else {
        --control.well_prune_mass_cumulative_lo;
      }
      break;
    case Corruption::broken_partition:
      control.unresolved_mass_lo =
          control.unresolved_mass_lo == 0U
              ? 1U
              : control.unresolved_mass_lo - 1U;
      break;
    case Corruption::chunk_ready_without_full_segment:
      control.status = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledSlotStatus::chunk_ready);
      control.yield_reason = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledYieldReason::prune_segment_full);
      break;
    case Corruption::fatal_with_partial_output:
      control.status = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledSlotStatus::fatal);
      control.stop_reason = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledStopReason::fatal_failure);
      control.failure_code = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      if (control.well_prune_mass_delta_lo == 0U &&
          control.well_prune_mass_delta_hi == 0U &&
          control.rank_prune_mass_delta_lo == 0U &&
          control.rank_prune_mass_delta_hi == 0U &&
          control.terminal_mass_delta_lo == 0U &&
          control.terminal_mass_delta_hi == 0U) {
        control.terminal_mass_delta_lo = 1U;
        control.terminal_record_count = 1U;
      }
      break;
    case Corruption::active_slot_status:
      control.status = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledSlotStatus::active);
      break;
    case Corruption::mass_without_record:
      control.prune_record_count = 0U;
      if (control.well_prune_mass_delta_lo == 0U &&
          control.well_prune_mass_delta_hi == 0U &&
          control.rank_prune_mass_delta_lo == 0U &&
          control.rank_prune_mass_delta_hi == 0U) {
        control.well_prune_mass_delta_lo = 1U;
      }
      break;
    case Corruption::forged_root_digest:
      control.root_entry_digest ^= UINT64_C(1);
      break;
    case Corruption::stack_high_water_overflow:
      control.stack_high_water = checked_u64_sum(
          checked_u64(
              higher_support_device_tiled_frontier_products_per_slot,
              "the fake Phase 15 product cap does not fit uint64"),
          UINT64_C(1),
          "the fake Phase 15 forged high water overflows uint64");
      break;
    case Corruption::nonzero_failure_code:
      control.failure_code = static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      break;
    case Corruption::forged_cuda_execution:
      batch.execution_kind =
          Phase15HigherSupportDeviceTiledExecutionKind::cuda;
      batch.cuda_execution_contract_satisfied = true;
      batch.cuda_device = 0;
      break;
  }
}

}  // namespace

Phase15HigherSupportDeviceTiledAdoptedTraversal
adopt_phase15_higher_support_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  if (!traversal_lease.ready() || traversal_lease.cuda_resident() ||
      !traversal_lease.audit().host_fake_lifecycle_exercised) {
    throw std::invalid_argument(
        "the fake Phase 15 higher-support adoption requires a ready "
        "host-fake traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit = traversal_lease.audit();
  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(traversal_lease));

  Phase15HigherSupportDeviceTiledAdoptedTraversal adopted;
  adopted.retained_owner = owner;
  // The lease keeps its cloud token private and this seam has no friendship
  // over it, so the retained lease itself is the immutable identity
  // authority: the batch must return the very same aliased token and the
  // host compares ownership through this seam only.
  adopted.source_cloud_identity =
      std::shared_ptr<const void>{owner, owner.get()};
  adopted.device_coordinate_bits = nullptr;
  adopted.device_morton_point_ids = nullptr;
  adopted.device_nodes = nullptr;
  adopted.point_count = audit.point_count;
  adopted.certified_node_count = audit.certified_node_count;
  adopted.maximum_point_count = audit.maximum_point_count;
  adopted.maximum_node_count = audit.maximum_node_count;
  adopted.retained_coordinate_word_capacity =
      audit.retained_coordinate_word_capacity;
  adopted.retained_morton_point_id_capacity =
      audit.retained_morton_point_id_capacity;
  adopted.retained_node_capacity = audit.retained_node_capacity;
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.cuda_device = -1;
  adopted.execution_kind =
      Phase15HigherSupportDeviceTiledExecutionKind::host_fake;
  adopted.canonical_coordinate_words_retained =
      audit.canonical_coordinate_words_retained;
  adopted.active_morton_point_ids_retained =
      audit.active_morton_point_ids_retained;
  adopted.certified_device_nodes_retained =
      audit.certified_device_nodes_retained;
  adopted.host_fake_lifecycle_exercised = true;
  adopted.cuda_device_storage_retained = false;
  return adopted;
}

Phase15HigherSupportDeviceTiledBatch
build_phase15_higher_support_device_tiled_frontier_on_device(
    Phase15HigherSupportDeviceTiledFrontierContextState& context,
    const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
    const Phase15HigherSupportDeviceTiledRequest& request) {
  static_cast<void>(context);
  test_support::fake_launch_count.fetch_add(1U, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  const Corruption corruption = test_support::fake_corruption;

  if (!traversal.retained_owner || !traversal.source_cloud_identity ||
      traversal.execution_kind !=
          Phase15HigherSupportDeviceTiledExecutionKind::host_fake ||
      !traversal.host_fake_lifecycle_exercised ||
      traversal.cuda_device_storage_retained ||
      traversal.device_coordinate_bits != nullptr ||
      traversal.device_morton_point_ids != nullptr ||
      traversal.device_nodes != nullptr || traversal.cuda_device != -1 ||
      request.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      request.record_buffer_epoch == 0U || request.tile_epoch == 0U ||
      request.chunk_sequence == 0U ||
      request.point_count != traversal.point_count ||
      request.certified_node_count != traversal.certified_node_count ||
      request.point_count == 0U ||
      checked_u64(
          request.point_count,
          "the fake Phase 15 point count does not fit uint64") >
          higher_support_device_tiled_frontier_maximum_point_count ||
      request.certified_maximum_lbvh_depth >
          higher_support_device_tiled_frontier_maximum_certified_lbvh_depth ||
      higher_support_device_tiled_frontier_proved_stack_bound(
          request.certified_maximum_lbvh_depth) >
          request.products_per_slot ||
      request.slot_count == 0U ||
      request.slot_count >
          higher_support_device_tiled_frontier_maximum_slot_tile_capacity ||
      request.products_per_slot !=
          higher_support_device_tiled_frontier_products_per_slot ||
      request.prune_records_per_slot !=
          higher_support_device_tiled_frontier_prune_records_per_slot ||
      request.terminal_records_per_slot !=
          higher_support_device_tiled_frontier_terminal_records_per_slot ||
      request.probe_receipts_per_slot !=
          higher_support_device_tiled_frontier_probe_receipts_per_slot ||
      request.pending_decisions_per_slot !=
          higher_support_device_tiled_frontier_pending_decisions_per_slot ||
      !higher_support_device_tiled_frontier_terminal_policy_known(
          request.terminal_policy) ||
      request.maximum_relevant_closed_rank < 2U ||
      request.maximum_relevant_closed_rank >
          higher_support_device_tiled_frontier_maximum_closed_rank ||
      request.gate_evaluations_per_slot_per_chunk < 2U ||
      request.maximum_subdivision_count == 0U ||
      (request.resume_same_tile
           ? !request.root_entries.empty()
           : request.root_entries.size() != request.slot_count)) {
    throw std::invalid_argument(
        "the fake Phase 15 higher-support device tiled launcher received "
        "an invalid request or traversal authority");
  }
  if (!test_support::fake_geometry.has_value()) {
    throw std::runtime_error(
        "the fake Phase 15 higher-support device tiled launcher has no "
        "bound host geometry");
  }
  const FakeBoundGeometry& geometry = *test_support::fake_geometry;
  if (geometry.leaves.size() != request.point_count ||
      geometry.nodes.size() != request.certified_node_count ||
      geometry.maximum_depth > request.certified_maximum_lbvh_depth) {
    throw std::runtime_error(
        "the fake Phase 15 bound geometry contradicts the adopted "
        "traversal extents or the certified depth");
  }

  // Tile continuation: a fresh tile installs one rooted DFS stack per slot;
  // a resumed tile must preserve its complete identity and advance the
  // chunk sequence by exactly one.
  if (!request.resume_same_tile) {
    if (request.chunk_sequence != 1U) {
      throw std::invalid_argument(
          "a new fake Phase 15 higher-support tile did not start at chunk "
          "one");
    }
    FakeTileState tile;
    tile.tile_epoch = request.tile_epoch;
    tile.chunk_sequence = request.chunk_sequence;
    tile.source_snapshot_epoch = request.source_snapshot_epoch;
    tile.slot_count = request.slot_count;
    tile.slots.reserve(request.slot_count);
    for (std::size_t slot_index = 0U;
         slot_index < request.slot_count;
         ++slot_index) {
      const FrontierEntry& entry = request.root_entries[slot_index];
      validate_fake_root_entry(geometry, entry, request.terminal_policy);
      FakeSlotState slot;
      slot.stack.push_back(entry);
      slot.universe_mass = fake_entry_mass(entry);
      slot.root_entry_digest =
          phase15_higher_support_device_tiled_root_entry_digest(entry);
      slot.stack_high_water = 1U;
      tile.slots.push_back(std::move(slot));
    }
    test_support::fake_tile.emplace(std::move(tile));
  } else if (
      !test_support::fake_tile.has_value() ||
      test_support::fake_tile->tile_epoch != request.tile_epoch ||
      test_support::fake_tile->slot_count != request.slot_count ||
      test_support::fake_tile->source_snapshot_epoch !=
          request.source_snapshot_epoch ||
      request.chunk_sequence !=
          test_support::fake_tile->chunk_sequence + 1U) {
    throw std::invalid_argument(
        "a resumed fake Phase 15 higher-support tile changed its identity "
        "or sequence");
  } else {
    test_support::fake_tile->chunk_sequence = request.chunk_sequence;
  }
  FakeTileState& tile = *test_support::fake_tile;

  auto storage = std::make_shared<FakeOutputStorage>();
  storage->prune_records.resize(checked_size_product(
      request.slot_count,
      request.prune_records_per_slot,
      "the fake Phase 15 prune segment capacity overflows size_t"));
  storage->terminal_records.resize(checked_size_product(
      request.slot_count,
      request.terminal_records_per_slot,
      "the fake Phase 15 terminal segment capacity overflows size_t"));
  storage->probe_receipts.resize(checked_size_product(
      request.slot_count,
      request.probe_receipts_per_slot,
      "the fake Phase 15 receipt segment capacity overflows size_t"));

  std::vector<FakeSlotChunk> chunks(request.slot_count);
  for (std::size_t slot_index = 0U;
       slot_index < request.slot_count;
       ++slot_index) {
    if (tile.slots[slot_index].complete) {
      chunks[slot_index].run_state = FakeSlotRunState::complete;
    }
  }

  // Internal subdivision loop: the seam never returns an active slot.  Each
  // pass grants every unresolved slot a fresh gate-evaluation quantum; a
  // tile that cannot settle within maximum_subdivision_count passes stops
  // fatally with the capacity stop reason and a full chunk rollback of the
  // unresolved slots.
  std::size_t pass_count = 0U;
  const auto any_unresolved = [&chunks]() {
    for (const FakeSlotChunk& chunk : chunks) {
      if (chunk.run_state == FakeSlotRunState::active ||
          chunk.run_state == FakeSlotRunState::paused) {
        return true;
      }
    }
    return false;
  };
  while (any_unresolved()) {
    if (pass_count == request.maximum_subdivision_count) {
      for (std::size_t slot_index = 0U;
           slot_index < request.slot_count;
           ++slot_index) {
        FakeSlotChunk& chunk = chunks[slot_index];
        if (chunk.run_state == FakeSlotRunState::active ||
            chunk.run_state == FakeSlotRunState::paused) {
          rollback_slot_chunk(request, *storage, chunk, slot_index);
          chunk.run_state = FakeSlotRunState::fatal;
          chunk.stop_reason = Phase15HigherSupportDeviceTiledStopReason::
              subdivision_capacity;
          chunk.failure_code =
              Phase15HigherSupportDeviceTiledFailureCode::none;
          tile.slots[slot_index].probe.reset();
          tile.slots[slot_index].pending_terminal = false;
        }
      }
      break;
    }
    pass_count = checked_size_sum(
        pass_count,
        1U,
        "the fake Phase 15 subdivision count overflows size_t");
    for (std::size_t slot_index = 0U;
         slot_index < request.slot_count;
         ++slot_index) {
      FakeSlotChunk& chunk = chunks[slot_index];
      if (chunk.run_state == FakeSlotRunState::paused) {
        chunk.run_state = FakeSlotRunState::active;
      }
      if (chunk.run_state != FakeSlotRunState::active) {
        continue;
      }
      FakeSlotExecutor executor{
          geometry,
          request,
          *storage,
          tile.slots[slot_index],
          chunk,
          slot_index};
      executor.run_subdivision();
      if (chunk.run_state == FakeSlotRunState::fatal) {
        rollback_slot_chunk(request, *storage, chunk, slot_index);
        tile.slots[slot_index].probe.reset();
        tile.slots[slot_index].pending_terminal = false;
      }
    }
  }
  if (pass_count == 0U) {
    pass_count = 1U;
  }

  // A fatal slot censors the whole tile, and the protocol forbids a batch
  // that is simultaneously fatal and resumable.  Any slot that had yielded
  // chunk_ready in this chunk is therefore rolled back and collapsed into
  // the tile-level fatality, so a fatal batch never publishes any partial
  // continuation.
  bool tile_fatal = false;
  bool capacity_trigger = false;
  for (const FakeSlotChunk& chunk : chunks) {
    if (chunk.run_state == FakeSlotRunState::fatal) {
      tile_fatal = true;
      capacity_trigger = capacity_trigger ||
          chunk.stop_reason ==
              Phase15HigherSupportDeviceTiledStopReason::
                  subdivision_capacity;
    }
  }
  if (tile_fatal) {
    for (std::size_t slot_index = 0U;
         slot_index < request.slot_count;
         ++slot_index) {
      FakeSlotChunk& chunk = chunks[slot_index];
      if (chunk.run_state == FakeSlotRunState::chunk_ready) {
        rollback_slot_chunk(request, *storage, chunk, slot_index);
        chunk.run_state = FakeSlotRunState::fatal;
        chunk.stop_reason = capacity_trigger
            ? Phase15HigherSupportDeviceTiledStopReason::
                  subdivision_capacity
            : Phase15HigherSupportDeviceTiledStopReason::fatal_failure;
        chunk.failure_code =
            Phase15HigherSupportDeviceTiledFailureCode::none;
        tile.slots[slot_index].probe.reset();
        tile.slots[slot_index].pending_terminal = false;
        continue;
      }
      // Mirror of the pair-model two-stage rollback: a slot that completed
      // WITHIN the fatal chunk must also withhold its current output chunk.
      // After that retraction its universe is unresolved again, so no
      // atomic terminal batch exists and the launch fails instead of
      // publishing a mixed fatal/complete tile.  A slot complete from an
      // earlier chunk carries zero chunk activity and stays complete.
      const bool completed_this_chunk =
          chunk.run_state == FakeSlotRunState::complete &&
          (chunk.well_delta != FakeU128{} ||
           chunk.rank_delta != FakeU128{} ||
           chunk.terminal_delta != FakeU128{} ||
           chunk.expansion_count != 0U ||
           chunk.gate_evaluation_count != 0U ||
           chunk.prune_used != 0U || chunk.terminal_used != 0U ||
           chunk.receipt_used != 0U);
      if (completed_this_chunk) {
        rollback_slot_chunk(request, *storage, chunk, slot_index);
        throw std::runtime_error(
            "the fake Phase 15 higher-support subdivision-capacity stop "
            "could not form an atomic terminal batch after withholding "
            "its current output chunk");
      }
    }
  }

  Phase15HigherSupportDeviceTiledBatch batch;
  batch.retained_output_owner = storage;
  batch.source_cloud_identity_authority = traversal.source_cloud_identity;
  batch.host_slot_controls.reserve(request.slot_count);
  bool any_chunk_ready = false;
  bool any_fatal = false;
  for (std::size_t slot_index = 0U;
       slot_index < request.slot_count;
       ++slot_index) {
    FakeSlotState& slot = tile.slots[slot_index];
    const FakeSlotChunk& chunk = chunks[slot_index];
    FakeU128 well_new = slot.well_cumulative;
    fake_u128_accumulate(
        well_new,
        chunk.well_delta,
        "the fake Phase 15 well-prune cumulative overflows unsigned 128 "
        "bits");
    FakeU128 rank_new = slot.rank_cumulative;
    fake_u128_accumulate(
        rank_new,
        chunk.rank_delta,
        "the fake Phase 15 rank-prune cumulative overflows unsigned 128 "
        "bits");
    FakeU128 terminal_new = slot.terminal_cumulative;
    fake_u128_accumulate(
        terminal_new,
        chunk.terminal_delta,
        "the fake Phase 15 terminal cumulative overflows unsigned 128 "
        "bits");
    FakeU128 resolved = well_new;
    fake_u128_accumulate(
        resolved,
        rank_new,
        "the fake Phase 15 resolved mass overflows unsigned 128 bits");
    fake_u128_accumulate(
        resolved,
        terminal_new,
        "the fake Phase 15 resolved mass overflows unsigned 128 bits");
    const FakeU128 unresolved = fake_u128_difference(
        slot.universe_mass,
        resolved,
        "the fake Phase 15 slot partition lost mass");

    Phase15HigherSupportDeviceTiledSlotControl control{};
    control.root_entry_digest = slot.root_entry_digest;
    control.well_prune_mass_delta_lo = chunk.well_delta.lo;
    control.well_prune_mass_delta_hi = chunk.well_delta.hi;
    control.rank_prune_mass_delta_lo = chunk.rank_delta.lo;
    control.rank_prune_mass_delta_hi = chunk.rank_delta.hi;
    control.terminal_mass_delta_lo = chunk.terminal_delta.lo;
    control.terminal_mass_delta_hi = chunk.terminal_delta.hi;
    control.well_prune_mass_cumulative_lo = well_new.lo;
    control.well_prune_mass_cumulative_hi = well_new.hi;
    control.rank_prune_mass_cumulative_lo = rank_new.lo;
    control.rank_prune_mass_cumulative_hi = rank_new.hi;
    control.terminal_mass_cumulative_lo = terminal_new.lo;
    control.terminal_mass_cumulative_hi = terminal_new.hi;
    control.unresolved_mass_lo = unresolved.lo;
    control.unresolved_mass_hi = unresolved.hi;
    control.expansion_count = chunk.expansion_count;
    control.gate_evaluation_count = chunk.gate_evaluation_count;
    control.probe_root_decision_count = chunk.probe_root_decision_count;
    control.probe_fallback_decision_count =
        chunk.probe_fallback_decision_count;
    control.deferred_int512_count = 0U;
    control.deferred_int1024_count = 0U;
    control.rational_drain_count = 0U;
    control.prune_record_count = checked_u64(
        chunk.prune_used,
        "a fake Phase 15 prune record count does not fit uint64");
    control.terminal_record_count = checked_u64(
        chunk.terminal_used,
        "a fake Phase 15 terminal record count does not fit uint64");
    control.probe_receipt_count = checked_u64(
        chunk.receipt_used,
        "a fake Phase 15 receipt count does not fit uint64");
    // A slot that already completed in an earlier chunk performs no work
    // in this chunk, so it must report zero chunk activity, including a
    // zero stack high water, to satisfy the host's no-activity contract.
    control.stack_high_water = slot.complete ? 0U : slot.stack_high_water;
    control.status = static_cast<std::uint64_t>(
        slot_status(chunk.run_state));
    control.yield_reason =
        static_cast<std::uint64_t>(chunk.yield_reason);
    control.stop_reason = static_cast<std::uint64_t>(chunk.stop_reason);
    control.failure_code = static_cast<std::uint64_t>(chunk.failure_code);
    control.tile_epoch = request.tile_epoch;
    control.chunk_sequence = request.chunk_sequence;
    control.reserved_zero = 0U;
    batch.host_slot_controls.push_back(control);

    slot.well_cumulative = well_new;
    slot.rank_cumulative = rank_new;
    slot.terminal_cumulative = terminal_new;
    slot.complete = chunk.run_state == FakeSlotRunState::complete;
    any_chunk_ready = any_chunk_ready ||
        chunk.run_state == FakeSlotRunState::chunk_ready;
    any_fatal =
        any_fatal || chunk.run_state == FakeSlotRunState::fatal;
  }
  if (any_fatal) {
    // A fatal tile is terminally censored by the host and never resumes.
    test_support::fake_tile.reset();
  }

  batch.physical_product_record_capacity = checked_size_product(
      request.slot_count,
      request.products_per_slot,
      "the fake Phase 15 product capacity overflows size_t");
  batch.physical_prune_record_capacity = storage->prune_records.size();
  batch.physical_terminal_record_capacity =
      storage->terminal_records.size();
  batch.physical_probe_receipt_capacity = storage->probe_receipts.size();
  batch.physical_slot_control_capacity = request.slot_count;
  batch.physical_slot_checkpoint_capacity = request.slot_count;
  batch.physical_deferred_int512_capacity = checked_size_product(
      request.slot_count,
      request.pending_decisions_per_slot,
      "the fake Phase 15 deferred int512 capacity overflows size_t");
  batch.physical_deferred_int1024_capacity = checked_size_product(
      request.slot_count,
      request.pending_decisions_per_slot,
      "the fake Phase 15 deferred int1024 capacity overflows size_t");
  batch.physical_pending_slot_count_capacity = 1U;
  // Canonical would-be device arena accounting: the fixed record segments,
  // the slot controls, both deferred queues and one pending-slot counter.
  // The private per-slot checkpoints are host continuation state in the
  // fake and are deliberately not part of the accounted arena.
  std::size_t arena_bytes = checked_size_product(
      batch.physical_product_record_capacity,
      sizeof(Phase15HigherSupportDeviceTiledProductRecord),
      "the fake Phase 15 product arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_size_product(
          batch.physical_prune_record_capacity,
          sizeof(Phase15HigherSupportDeviceTiledPruneRecord),
          "the fake Phase 15 prune arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_size_product(
          batch.physical_terminal_record_capacity,
          sizeof(Phase15HigherSupportDeviceTiledTerminalRecord),
          "the fake Phase 15 terminal arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_size_product(
          batch.physical_probe_receipt_capacity,
          sizeof(Phase15HigherSupportDeviceTiledProbeReceipt),
          "the fake Phase 15 receipt arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_size_product(
          batch.physical_slot_control_capacity,
          sizeof(Phase15HigherSupportDeviceTiledSlotControl),
          "the fake Phase 15 control arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_size_product(
          checked_size_sum(
              batch.physical_deferred_int512_capacity,
              batch.physical_deferred_int1024_capacity,
              "the fake Phase 15 deferred capacity overflows size_t"),
          sizeof(Phase15HigherSupportDeviceTiledDeferredDecision),
          "the fake Phase 15 deferred arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  batch.physical_device_arena_capacity_bytes = checked_size_sum(
      arena_bytes,
      sizeof(std::uint64_t),
      "the fake Phase 15 pending-slot arena bytes overflow size_t");

  batch.slot_control_device_to_host_count = 0U;
  batch.slot_control_device_to_host_byte_count = 0U;
  batch.resume_control_device_to_host_count = 0U;
  batch.resume_control_device_to_host_byte_count = 0U;
  batch.kernel_launch_count = 0U;
  batch.synchronization_count = 0U;
  batch.traversal_subdivision_count = pass_count;
  batch.source_snapshot_epoch = request.source_snapshot_epoch;
  batch.record_buffer_epoch = request.record_buffer_epoch;
  batch.tile_epoch = request.tile_epoch;
  batch.chunk_sequence = request.chunk_sequence;
  batch.cuda_device = -1;
  batch.execution_kind =
      Phase15HigherSupportDeviceTiledExecutionKind::host_fake;
  batch.terminal_policy = request.terminal_policy;
  batch.maximum_relevant_closed_rank =
      request.maximum_relevant_closed_rank;
  batch.fixed_slot_segments_allocated = true;
  batch.output_owner_detached_for_tile_lifetime = true;
  batch.unsigned128_slot_mass_accounting_requested = true;
  batch.probe_plan_order_receipt_commit_requested = true;
  batch.substitute_probe_center_seed_policy_v1_requested = true;
  batch.closed_rank_window_receipts_sealed_to_policy =
      request.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window;
  batch.strict_interior_carrier_receipts_sealed_to_policy =
      request.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::
          strict_interior_carrier_q3;
  batch.strict_interior_carrier_never_authorizes_gamma2_drop = true;
  batch.censored_slot_outputs_invalidated = true;
  batch.bigint_support_mass_transferred = false;
  batch.floating_point_decision_performed = false;
  batch.exact_higher_support_terminal_classification_native_cuda = false;
  batch.dense_product_fallback_performed = false;
  batch.global_product_frontier_materialized = false;
  batch.ordinary_or_higher_order_delaunay_materialized = false;
  batch.global_cell_coface_or_incidence_arena_materialized = false;
  batch.cuda_execution_contract_satisfied = false;
  batch.fresh_tile_device_arena_allocated = false;
  batch.fresh_tile_device_arena_reused = false;
  batch.resume_same_tile = request.resume_same_tile;
  batch.capacity_yield_resumable = any_chunk_ready;
  batch.process_restart_resumable = false;

  apply_control_corruption(corruption, batch);
  storage->slot_controls = batch.host_slot_controls;
  batch.prune_records = storage->prune_records.data();
  batch.terminal_records = storage->terminal_records.data();
  batch.probe_receipts = storage->probe_receipts.data();
  batch.slot_controls = storage->slot_controls.data();
  batch.metadata_digest =
      phase15_higher_support_device_tiled_metadata_digest(batch);
  if (corruption == Corruption::metadata_digest_corruption) {
    batch.metadata_digest ^= UINT64_C(1);
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
