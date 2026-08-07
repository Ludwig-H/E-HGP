#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierAdvance;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierConfig;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierContext;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierStatus;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierTerminalPolicy;
using morsehgp3d::gpu::HigherSupportDeviceTiledRecordDrainLease;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_maximum_point_count;
using morsehgp3d::gpu::higher_support_device_tiled_frontier_products_per_slot;
using morsehgp3d::gpu::higher_support_device_tiled_frontier_proved_stack_bound;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_required_witness_count;
using morsehgp3d::gpu::detail::
    phase15_higher_support_device_tiled_binomial_u128;
using morsehgp3d::gpu::detail::
    phase15_higher_support_device_tiled_flag_carrier_policy;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledDeferredDecision;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledProbeReceipt;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledProductRecord;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledPruneKind;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledPruneRecord;
using morsehgp3d::gpu::detail::
    Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledSlotControl;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledTerminalRecord;
using morsehgp3d::gpu::detail::Phase15HigherSupportDeviceTiledUnsigned128;
using morsehgp3d::gpu::test_support::
    bind_fake_higher_support_device_tiled_geometry;
using morsehgp3d::gpu::test_support::
    fake_gpu_higher_support_device_tiled_frontier_launch_count;
using morsehgp3d::gpu::test_support::
    FakeHigherSupportDeviceTiledFrontierCorruption;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_higher_support_device_tiled_frontier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::gpu::test_support::
    set_fake_gpu_higher_support_device_tiled_frontier_corruption;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::exact_higher_support_candidate_universe_size;
using morsehgp3d::hierarchy::exact_higher_support_product_aabb_analysis;
using morsehgp3d::hierarchy::exact_higher_support_product_query_cell_decision;
using morsehgp3d::hierarchy::exact_higher_support_terminal_geometry_decision;
using morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactHigherSupportNodeGroup;
using morsehgp3d::hierarchy::ExactHigherSupportProductQueryCellDecision;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamResult;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalGeometryDecision;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::MortonLeafRecord;
using morsehgp3d::spatial::PointId;

using BigInt = morsehgp3d::exact::BigInt;
using Corruption = FakeHigherSupportDeviceTiledFrontierCorruption;
using Policy = HigherSupportDeviceTiledFrontierTerminalPolicy;
using Status = HigherSupportDeviceTiledFrontierStatus;

// ---------------------------------------------------------------------------
// ABI static assertions (section 5.1 of the locked M1 architecture).
// ---------------------------------------------------------------------------

static_assert(sizeof(Phase15HigherSupportDeviceTiledProductRecord) == 128U);
static_assert(sizeof(Phase15HigherSupportDeviceTiledSlotControl) == 264U);
static_assert(sizeof(Phase15HigherSupportDeviceTiledProbeReceipt) == 32U);
static_assert(sizeof(Phase15HigherSupportDeviceTiledPruneRecord) == 160U);
static_assert(sizeof(Phase15HigherSupportDeviceTiledTerminalRecord) == 96U);
static_assert(sizeof(Phase15HigherSupportDeviceTiledDeferredDecision) == 176U);
static_assert(
    std::is_standard_layout_v<Phase15HigherSupportDeviceTiledProductRecord> &&
    std::is_trivially_copyable_v<
        Phase15HigherSupportDeviceTiledProductRecord>);
static_assert(
    std::is_standard_layout_v<Phase15HigherSupportDeviceTiledSlotControl> &&
    std::is_trivially_copyable_v<Phase15HigherSupportDeviceTiledSlotControl>);
static_assert(
    std::is_standard_layout_v<Phase15HigherSupportDeviceTiledProbeReceipt> &&
    std::is_trivially_copyable_v<
        Phase15HigherSupportDeviceTiledProbeReceipt>);
static_assert(
    std::is_standard_layout_v<Phase15HigherSupportDeviceTiledPruneRecord> &&
    std::is_trivially_copyable_v<Phase15HigherSupportDeviceTiledPruneRecord>);
static_assert(
    std::is_standard_layout_v<
        Phase15HigherSupportDeviceTiledTerminalRecord> &&
    std::is_trivially_copyable_v<
        Phase15HigherSupportDeviceTiledTerminalRecord>);
static_assert(
    std::is_standard_layout_v<
        Phase15HigherSupportDeviceTiledDeferredDecision> &&
    std::is_trivially_copyable_v<
        Phase15HigherSupportDeviceTiledDeferredDecision>);
static_assert(
    !std::is_copy_constructible_v<HigherSupportDeviceTiledRecordDrainLease>);
static_assert(std::is_nothrow_move_constructible_v<
              HigherSupportDeviceTiledRecordDrainLease>);
static_assert(
    !std::is_copy_constructible_v<HigherSupportDeviceTiledFrontierContext>);
static_assert(std::is_nothrow_move_constructible_v<
              HigherSupportDeviceTiledFrontierContext>);
static_assert(
    !std::is_copy_constructible_v<HigherSupportDeviceTiledFrontierAdvance>);
static_assert(std::is_nothrow_move_constructible_v<
              HigherSupportDeviceTiledFrontierAdvance>);
static_assert(
    higher_support_device_tiled_frontier_proved_stack_bound(94U) <=
    higher_support_device_tiled_frontier_products_per_slot);
static_assert(
    HigherSupportDeviceTiledFrontierContext::deployment_status ==
    "architecture_only");
static_assert(
    HigherSupportDeviceTiledFrontierContext::public_status == "not_claimed");

[[nodiscard]] constexpr std::uint64_t constexpr_binomial_lo(
    std::uint64_t range_size,
    std::uint64_t multiplicity) {
  Phase15HigherSupportDeviceTiledUnsigned128 out{};
  if (!phase15_higher_support_device_tiled_binomial_u128(
          range_size, multiplicity, out) ||
      out.hi != 0U) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return out.lo;
}
static_assert(constexpr_binomial_lo(64U, 3U) == 41'664U);
static_assert(constexpr_binomial_lo(64U, 4U) == 635'376U);
static_assert(constexpr_binomial_lo(3U, 4U) == 0U);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

// Exactly collinear adversarial cloud: every support is affinely dependent,
// so the whole universe must resolve through universal well prunes.
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

// Three well-separated clusters at three different scales.
[[nodiscard]] CanonicalPointCloud cluster_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),
      point(0.25, 0.0, 0.0),
      point(0.0, 0.25, 0.0),
      point(0.0, 0.0, 0.25),
      point(100.0, 100.0, 100.0),
      point(100.5, 100.0, 100.0),
      point(100.0, 100.5, 100.0),
      point(-200.0, 50.0, 300.0),
      point(-199.0, 50.0, 300.0),
      point(-200.0, 51.0, 300.0)};
  return cloud_from(points);
}

// Six exactly cospherical octahedron vertices plus two binary64-perturbed
// near-sphere points: rich extra-shell diagnostics and near-degenerate
// products that keep every gate honest.
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

// One exact acute (Gabriel-style) triangle with a single strictly interior
// witness near its circumcentre and two remote points.
[[nodiscard]] CanonicalPointCloud gabriel_witness_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(2.0, 3.0, 0.0),
      point(2.0, 0.8, 0.1),
      point(50.0, 50.0, 50.0),
      point(-40.0, 60.0, -30.0)};
  return cloud_from(points);
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum, maximum, maximum, maximum, maximum, maximum, maximum,
      maximum};
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    const CanonicalPointCloud& cloud) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  const auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

[[nodiscard]] BigInt bigint_from_words(std::uint64_t lo, std::uint64_t hi) {
  return (BigInt{hi} << 64U) + lo;
}

// Exact BigInt binomial for multiplicity in [1, 4]; the sequential integer
// divisions are exact because k consecutive integers contain the factor k!.
[[nodiscard]] BigInt bigint_binomial(
    std::uint64_t range_size,
    std::size_t multiplicity) {
  if (multiplicity == 0U || multiplicity > 4U) {
    throw std::logic_error(
        "a test binomial multiplicity must be in [1, 4]");
  }
  if (range_size < multiplicity) {
    return BigInt{0};
  }
  BigInt value{1};
  for (std::size_t step = 0U; step < multiplicity; ++step) {
    value *= BigInt{range_size - step};
  }
  for (std::size_t divisor = 2U; divisor <= multiplicity; ++divisor) {
    value /= BigInt{static_cast<std::uint64_t>(divisor)};
  }
  return value;
}

[[nodiscard]] PointId find_point_id(
    const CanonicalPointCloud& cloud,
    const CertifiedPoint3& target) {
  for (PointId id = 0U; id < cloud.size(); ++id) {
    if (cloud.point(id).canonical_input_bits() ==
        target.canonical_input_bits()) {
      return id;
    }
  }
  throw std::logic_error("a test point is missing from its canonical cloud");
}

// ---------------------------------------------------------------------------
// Host replay mirror of the certified Morton LBVH.  The node boxes are not
// part of the public index API, so the replay rebuilds them from the public
// leaf records with the exact radix/median split and witness rules of the
// CPU builder, and fails closed unless the certified counters are
// reproduced.  Every prune-record replay below is evaluated against this
// independent reconstruction, never against launcher state.
// ---------------------------------------------------------------------------

inline constexpr std::size_t invalid_mirror_node =
    static_cast<std::size_t>(-1);

struct MirrorNode {
  std::size_t left_child{invalid_mirror_node};
  std::size_t right_child{invalid_mirror_node};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};
  std::array<std::uint64_t, 3> lower_bits{};
  std::array<std::uint64_t, 3> upper_bits{};

  [[nodiscard]] bool is_leaf() const noexcept {
    return left_child == invalid_mirror_node;
  }
};

struct ReplayGeometry {
  std::vector<MortonLeafRecord> leaves;
  std::vector<MirrorNode> nodes;
  std::size_t root_index{};
  std::size_t maximum_depth{};
};

[[nodiscard]] std::uint64_t binary64_order_key(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63U;
  return (bits & sign_bit) != 0U ? ~bits : bits ^ sign_bit;
}

[[nodiscard]] std::size_t mirror_find_split(
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  if (begin + 1U >= end || end > leaves.size()) {
    throw std::logic_error(
        "the replay mirror split requires at least two leaves");
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
        "the replay mirror radix split did not divide its range");
  }
  return low;
}

[[nodiscard]] std::size_t mirror_build_range(
    const CanonicalPointCloud& cloud,
    std::span<const MortonLeafRecord> leaves,
    std::vector<MirrorNode>& nodes,
    std::size_t begin,
    std::size_t end,
    std::size_t depth,
    std::size_t& maximum_depth) {
  if (begin >= end || end > leaves.size()) {
    throw std::logic_error(
        "the replay mirror received an invalid leaf range");
  }
  maximum_depth = std::max(maximum_depth, depth);
  if (end - begin == 1U) {
    MirrorNode leaf;
    leaf.leaf_begin = begin;
    leaf.leaf_end = end;
    const std::array<std::uint64_t, 3> bits =
        cloud.point(leaves[begin].point_id).canonical_input_bits();
    leaf.lower_bits = bits;
    leaf.upper_bits = bits;
    nodes.push_back(leaf);
    return nodes.size() - 1U;
  }
  const std::size_t split = mirror_find_split(leaves, begin, end);
  const std::size_t left_child = mirror_build_range(
      cloud, leaves, nodes, begin, split, depth + 1U, maximum_depth);
  const std::size_t right_child = mirror_build_range(
      cloud, leaves, nodes, split, end, depth + 1U, maximum_depth);
  MirrorNode node;
  node.left_child = left_child;
  node.right_child = right_child;
  node.leaf_begin = begin;
  node.leaf_end = end;
  const MirrorNode& left = nodes[left_child];
  const MirrorNode& right = nodes[right_child];
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    node.lower_bits[axis] =
        binary64_order_key(right.lower_bits[axis]) <
                binary64_order_key(left.lower_bits[axis])
            ? right.lower_bits[axis]
            : left.lower_bits[axis];
    node.upper_bits[axis] =
        binary64_order_key(right.upper_bits[axis]) >
                binary64_order_key(left.upper_bits[axis])
            ? right.upper_bits[axis]
            : left.upper_bits[axis];
  }
  nodes.push_back(node);
  return nodes.size() - 1U;
}

[[nodiscard]] ReplayGeometry build_replay_geometry(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  if (!index.validated_for(cloud) || cloud.size() == 0U) {
    throw std::logic_error(
        "the replay mirror requires a Morton LBVH validated for its cloud");
  }
  ReplayGeometry geometry;
  geometry.leaves.assign(index.leaves().begin(), index.leaves().end());
  geometry.nodes.reserve(2U * cloud.size());
  geometry.root_index = mirror_build_range(
      cloud,
      std::span<const MortonLeafRecord>{geometry.leaves},
      geometry.nodes,
      0U,
      geometry.leaves.size(),
      0U,
      geometry.maximum_depth);
  const morsehgp3d::spatial::MortonLbvhBuildCounters& counters =
      index.build_counters();
  if (geometry.nodes.size() != counters.node_count ||
      geometry.root_index + 1U != geometry.nodes.size() ||
      geometry.maximum_depth != counters.maximum_depth) {
    throw std::logic_error(
        "the replay mirror does not reproduce the certified build "
        "counters");
  }
  return geometry;
}

[[nodiscard]] ExactDyadicAabb3 mirror_box(
    const ReplayGeometry& geometry,
    std::size_t node_index) {
  const MirrorNode& node = geometry.nodes[node_index];
  ExactDyadicAabb3 box{};
  box.lower_binary64_bits = node.lower_bits;
  box.upper_binary64_bits = node.upper_bits;
  return box;
}

[[nodiscard]] ExactHigherSupportFrontierEntry root_entry(
    std::size_t support_size,
    const ReplayGeometry& geometry) {
  ExactHigherSupportFrontierEntry entry{};
  entry.support_size = static_cast<std::uint8_t>(support_size);
  entry.group_count = 1U;
  entry.groups[0] = ExactHigherSupportNodeGroup{
      static_cast<std::uint64_t>(geometry.root_index),
      0U,
      static_cast<std::uint64_t>(geometry.leaves.size()),
      static_cast<std::uint8_t>(support_size)};
  return entry;
}

// ---------------------------------------------------------------------------
// Record drain and exact host replay.
// ---------------------------------------------------------------------------

struct DrainedPrune {
  Phase15HigherSupportDeviceTiledPruneRecord record{};
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt> receipts;
  std::size_t slot_index{};
};

struct DrainedTerminal {
  Phase15HigherSupportDeviceTiledTerminalRecord record{};
  std::size_t slot_index{};
};

struct DrainedRecords {
  std::vector<DrainedPrune> prunes;
  std::vector<DrainedTerminal> terminals;
};

void drain_record_lease(
    const HigherSupportDeviceTiledRecordDrainLease& lease,
    DrainedRecords& out,
    const std::string& label) {
  const auto views =
      Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess::inspect(
          lease);
  check(
      lease.ready() && lease.host_fake() && !lease.cuda_resident() &&
          views.ready && views.host_fake &&
          views.prune_records != nullptr &&
          views.terminal_records != nullptr &&
          views.probe_receipts != nullptr && views.slot_controls != nullptr,
      label + ": the drain lease exposes ready host-fake record views");
  if (!views.ready) {
    return;
  }
  std::size_t prune_total = 0U;
  std::size_t terminal_total = 0U;
  std::size_t receipt_total = 0U;
  for (std::size_t slot = views.slot_begin; slot < views.slot_end; ++slot) {
    const Phase15HigherSupportDeviceTiledSlotControl& control =
        views.slot_controls[slot];
    const auto prune_count =
        static_cast<std::size_t>(control.prune_record_count);
    const auto terminal_count =
        static_cast<std::size_t>(control.terminal_record_count);
    const auto receipt_count =
        static_cast<std::size_t>(control.probe_receipt_count);
    check(
        control.tile_epoch == lease.audit().tile_epoch &&
            control.chunk_sequence == lease.audit().chunk_sequence &&
            prune_count <= views.prune_record_stride &&
            terminal_count <= views.terminal_record_stride &&
            receipt_count <= views.probe_receipt_stride,
        label + ": a drained slot control stays inside its fixed segments");
    for (std::size_t index = 0U; index < prune_count; ++index) {
      DrainedPrune drained;
      drained.record =
          views.prune_records[slot * views.prune_record_stride + index];
      drained.slot_index = slot;
      const auto receipt_begin =
          static_cast<std::size_t>(drained.record.receipt_segment_begin);
      const auto record_receipts =
          static_cast<std::size_t>(drained.record.receipt_count);
      check(
          receipt_begin + record_receipts <= receipt_count,
          label + ": a prune record addresses only committed receipts");
      if (receipt_begin + record_receipts <= receipt_count) {
        for (std::size_t receipt = 0U; receipt < record_receipts;
             ++receipt) {
          drained.receipts.push_back(
              views.probe_receipts
                  [slot * views.probe_receipt_stride + receipt_begin +
                   receipt]);
        }
      }
      out.prunes.push_back(std::move(drained));
    }
    for (std::size_t index = 0U; index < terminal_count; ++index) {
      DrainedTerminal drained;
      drained.record =
          views
              .terminal_records[slot * views.terminal_record_stride + index];
      drained.slot_index = slot;
      out.terminals.push_back(drained);
    }
    prune_total += prune_count;
    terminal_total += terminal_count;
    receipt_total += receipt_count;
  }
  check(
      prune_total == lease.audit().prune_record_count &&
          terminal_total == lease.audit().terminal_record_count &&
          receipt_total == lease.audit().probe_receipt_count,
      label + ": the drained record totals match the lease audit");
}

struct TileRun {
  DrainedRecords records;
  BigInt universe{0};
  BigInt well{0};
  BigInt rank{0};
  BigInt terminal{0};
  BigInt resolved{0};
  BigInt unresolved{0};
  std::uint64_t maximum_stack_high_water{};
  std::size_t advance_count{};
  bool completed{false};
};

[[nodiscard]] TileRun run_active_tile(
    HigherSupportDeviceTiledFrontierContext& context,
    const std::string& label) {
  TileRun run;
  for (std::size_t iteration = 0U; iteration < 10'000U; ++iteration) {
    auto advance = context.advance();
    ++run.advance_count;
    run.maximum_stack_high_water = std::max(
        run.maximum_stack_high_water,
        advance.audit.transaction_stack_high_water);
    if (advance.record_drain.has_value()) {
      drain_record_lease(*advance.record_drain, run.records, label);
      advance.record_drain.reset();
    }
    if (advance.status == Status::frontier_tile_complete) {
      run.universe = advance.audit.tile_universe_support_mass;
      run.well = advance.audit.cumulative_well_prune_support_mass;
      run.rank = advance.audit.cumulative_rank_prune_support_mass;
      run.terminal = advance.audit.cumulative_terminal_support_mass;
      run.resolved = advance.audit.cumulative_resolved_support_mass;
      run.unresolved = advance.audit.unresolved_support_mass;
      run.completed = true;
      check(
          advance.audit.host_fake_launcher_exercised &&
              !advance.audit.cuda_execution_performed &&
              !advance.audit.floating_point_decision_performed &&
              !advance.audit.bigint_support_mass_transferred &&
              advance.audit.substitute_probe_center_seed_policy_v1 &&
              advance.audit.record_drain_lease_backpressure_bounded_to_one &&
              !advance.audit.global_product_frontier_materialized &&
              !advance.audit.slo_claimed &&
              !advance.audit.public_status_claimed,
          label + ": the closing audit keeps its host-fake proof flags");
      return run;
    }
    if (advance.status != Status::chunk_ready) {
      break;
    }
  }
  check(false, label + ": the slot tile did not run to completion");
  return run;
}

// Replays one drained prune record from the independent mirror: identity,
// exact mass, universal well-centring certificate, per-policy receipt
// thresholds and the disjoint receipt antichain.
void replay_prune_record(
    const ReplayGeometry& geometry,
    Policy policy,
    std::size_t maximum_relevant_closed_rank,
    const DrainedPrune& drained,
    const std::string& label) {
  const Phase15HigherSupportDeviceTiledPruneRecord& record = drained.record;
  const std::size_t support_size = record.support_size;
  const std::size_t group_count = record.group_count;
  const bool carrier = policy == Policy::strict_interior_carrier_q3;
  bool canonical = (support_size == 3U || support_size == 4U) &&
      group_count >= 1U && group_count <= support_size &&
      record.terminal_policy == static_cast<std::uint8_t>(policy) &&
      record.flags ==
          (carrier ? phase15_higher_support_device_tiled_flag_carrier_policy
                   : std::uint8_t{0U}) &&
      record.reserved_zero == 0U && (!carrier || support_size == 3U);
  std::array<ExactDyadicAabb3, 4> boxes{};
  std::size_t box_count = 0U;
  std::size_t multiplicity_sum = 0U;
  BigInt expected_mass{1};
  std::uint64_t previous_end = 0U;
  for (std::size_t index = 0U; canonical && index < group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(record.group_node_index[index]);
    const std::size_t multiplicity = record.group_multiplicity[index];
    if (node_index >= geometry.nodes.size()) {
      canonical = false;
      break;
    }
    const MirrorNode& node = geometry.nodes[node_index];
    if (record.group_leaf_begin[index] !=
            static_cast<std::uint64_t>(node.leaf_begin) ||
        record.group_leaf_end[index] !=
            static_cast<std::uint64_t>(node.leaf_end) ||
        multiplicity == 0U || multiplicity > 4U ||
        node.leaf_end - node.leaf_begin < multiplicity ||
        (index != 0U && record.group_leaf_begin[index] < previous_end)) {
      canonical = false;
      break;
    }
    previous_end = record.group_leaf_end[index];
    expected_mass *= bigint_binomial(
        record.group_leaf_end[index] - record.group_leaf_begin[index],
        multiplicity);
    for (std::size_t copy = 0U;
         canonical && copy < multiplicity;
         ++copy) {
      if (box_count >= support_size) {
        canonical = false;
        break;
      }
      boxes[box_count] = mirror_box(geometry, node_index);
      ++box_count;
    }
    multiplicity_sum += multiplicity;
  }
  for (std::size_t index = group_count; index < 4U; ++index) {
    canonical = canonical && record.group_node_index[index] == 0U &&
        record.group_leaf_begin[index] == 0U &&
        record.group_leaf_end[index] == 0U &&
        record.group_multiplicity[index] == 0U;
  }
  canonical = canonical && multiplicity_sum == support_size &&
      box_count == support_size;
  check(canonical, label + ": a drained prune record is canonically formed");
  if (!canonical) {
    return;
  }
  check(
      bigint_from_words(record.pruned_mass_lo, record.pruned_mass_hi) ==
          expected_mass,
      label + ": a drained prune record carries its exact BigInt mass");
  const std::span<const ExactDyadicAabb3> support_span{
      boxes.data(), support_size};
  if (record.prune_kind ==
      static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledPruneKind::no_well_centered_support)) {
    check(
        record.receipt_count == 0U && drained.receipts.empty(),
        label + ": a well prune carries no strict-interior receipts");
    const auto analysis =
        exact_higher_support_product_aabb_analysis(support_span);
    check(
        analysis.no_well_centered_support_certified(),
        label + ": a well prune replays its universal Gram/Cramer "
                "certificate");
    return;
  }
  const bool rank_window_kind = record.prune_kind ==
      static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledPruneKind::rank_window);
  const bool carrier_kind = record.prune_kind ==
      static_cast<std::uint64_t>(
          Phase15HigherSupportDeviceTiledPruneKind::strict_interior_carrier);
  check(
      (rank_window_kind || carrier_kind) &&
          (carrier ? carrier_kind : rank_window_kind),
      label + ": a rank prune kind is sealed to its terminal policy");
  if (!(rank_window_kind || carrier_kind)) {
    return;
  }
  const std::size_t required =
      higher_support_device_tiled_frontier_required_witness_count(
          policy, support_size, maximum_relevant_closed_rank);
  check(
      drained.receipts.size() ==
          static_cast<std::size_t>(record.receipt_count),
      label + ": a rank prune retains every committed receipt");
  if (required == 0U) {
    check(
        record.receipt_count == 0U,
        label + ": an above-window prune needs no witness receipt");
    return;
  }
  check(
      record.receipt_count >= 1U,
      label + ": a thresholded rank prune carries at least one receipt");
  BigInt certified{0};
  std::vector<std::pair<std::uint64_t, std::uint64_t>> intervals;
  bool receipts_valid = true;
  for (const Phase15HigherSupportDeviceTiledProbeReceipt& receipt :
       drained.receipts) {
    const auto node_index = static_cast<std::size_t>(receipt.node_index);
    if (node_index >= geometry.nodes.size()) {
      receipts_valid = false;
      break;
    }
    const MirrorNode& node = geometry.nodes[node_index];
    const std::uint64_t range =
        receipt.leaf_end > receipt.leaf_begin
            ? receipt.leaf_end - receipt.leaf_begin
            : 0U;
    bool external = true;
    for (std::size_t index = 0U; index < group_count; ++index) {
      if (receipt.leaf_begin < record.group_leaf_end[index] &&
          record.group_leaf_begin[index] < receipt.leaf_end) {
        external = false;
      }
    }
    if (receipt.leaf_begin !=
            static_cast<std::uint64_t>(node.leaf_begin) ||
        receipt.leaf_end != static_cast<std::uint64_t>(node.leaf_end) ||
        range == 0U || receipt.certified_point_count != range ||
        range > static_cast<std::uint64_t>(required) || !external) {
      receipts_valid = false;
      break;
    }
    const ExactHigherSupportProductQueryCellDecision decision =
        exact_higher_support_product_query_cell_decision(
            support_span, mirror_box(geometry, node_index));
    if (decision !=
        ExactHigherSupportProductQueryCellDecision::
            strictly_inside_every_independent_sphere) {
      receipts_valid = false;
      break;
    }
    certified += receipt.certified_point_count;
    intervals.emplace_back(receipt.leaf_begin, receipt.leaf_end);
  }
  std::sort(intervals.begin(), intervals.end());
  for (std::size_t index = 1U; index < intervals.size(); ++index) {
    if (intervals[index].first < intervals[index - 1U].second) {
      receipts_valid = false;
    }
  }
  check(
      receipts_valid,
      label + ": every rank receipt replays a real external strictly "
              "interior LBVH antichain cell");
  check(
      certified >= required,
      label + ": the replayed receipt masses reach the sealed policy "
              "threshold");
}

void replay_terminal_record(
    const CanonicalPointCloud& cloud,
    Policy policy,
    const DrainedTerminal& drained,
    const std::string& label) {
  const Phase15HigherSupportDeviceTiledTerminalRecord& record =
      drained.record;
  const std::size_t support_size = record.support_size;
  const bool carrier = policy == Policy::strict_interior_carrier_q3;
  bool canonical = (support_size == 3U || support_size == 4U) &&
      (!carrier || support_size == 3U) &&
      record.terminal_policy == static_cast<std::uint8_t>(policy) &&
      record.flags ==
          (carrier ? phase15_higher_support_device_tiled_flag_carrier_policy
                   : std::uint8_t{0U}) &&
      record.slot_index ==
          static_cast<std::uint64_t>(drained.slot_index) &&
      (record.decision_backend == 0U || record.decision_backend == 1U);
  for (std::size_t index = 0U; index < 3U; ++index) {
    canonical = canonical && record.reserved_zero_bytes[index] == 0U;
  }
  for (std::size_t index = 0U; index < 5U; ++index) {
    canonical = canonical && record.reserved_zero[index] == 0U;
  }
  for (std::size_t index = 0U; canonical && index < support_size; ++index) {
    canonical = record.point_ids[index] < cloud.size() &&
        (index == 0U ||
         record.point_ids[index - 1U] < record.point_ids[index]);
  }
  for (std::size_t index = support_size; index < 4U; ++index) {
    canonical = canonical && record.point_ids[index] == 0U;
  }
  check(
      canonical,
      label + ": a drained terminal record is canonically formed");
  if (!canonical) {
    return;
  }
  std::array<ExactDyadicAabb3, 4> boxes{};
  for (std::size_t index = 0U; index < support_size; ++index) {
    const std::array<std::uint64_t, 3> bits =
        cloud.point(record.point_ids[index]).canonical_input_bits();
    boxes[index].lower_binary64_bits = bits;
    boxes[index].upper_binary64_bits = bits;
  }
  const ExactHigherSupportTerminalGeometryDecision decision =
      exact_higher_support_terminal_geometry_decision(
          std::span<const ExactDyadicAabb3>{boxes.data(), support_size});
  check(
      static_cast<std::uint8_t>(decision) ==
          record.terminal_geometry_decision,
      label + ": a terminal record replays its exact geometry category");
}

// ---------------------------------------------------------------------------
// Host closed-ball classification of one fixed support (the exact consumer
// stage the fake deliberately leaves on the host in v1).
// ---------------------------------------------------------------------------

struct SupportKey {
  std::uint8_t support_size{};
  std::array<PointId, 4> support_ids{};

  friend bool operator==(const SupportKey&, const SupportKey&) = default;
  friend bool operator<(const SupportKey& left, const SupportKey& right) {
    if (left.support_size != right.support_size) {
      return left.support_size < right.support_size;
    }
    return left.support_ids < right.support_ids;
  }
};

struct ClosedBallSummary {
  bool minimal{false};
  std::size_t interior_count{};
  std::size_t shell_count{};
};

template <std::size_t SupportSize>
[[nodiscard]] ClosedBallSummary closed_ball_summary_sized(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids) {
  std::array<morsehgp3d::exact::ExactRational3, SupportSize> support{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    support[index] = cloud.point(support_ids[index]).exact();
  }
  const auto analysis =
      morsehgp3d::exact::analyze_circumcenter_support(support);
  ClosedBallSummary summary;
  if (analysis.status() !=
      morsehgp3d::exact::CircumcenterSupportStatus::minimal) {
    return summary;
  }
  summary.minimal = true;
  const auto& sphere = analysis.circumcenter_result();
  if (!sphere.center().has_value() || !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "a minimal replayed support omitted its exact sphere");
  }
  for (PointId id = 0U; id < cloud.size(); ++id) {
    const auto classification = morsehgp3d::exact::classify_sphere_point(
        *sphere.center(), *sphere.squared_level(), cloud.point(id));
    if (classification.location() ==
        morsehgp3d::exact::SpherePointLocation::strictly_inside) {
      ++summary.interior_count;
    } else if (
        classification.location() ==
        morsehgp3d::exact::SpherePointLocation::boundary) {
      ++summary.shell_count;
    }
  }
  return summary;
}

[[nodiscard]] ClosedBallSummary closed_ball_summary(
    const CanonicalPointCloud& cloud,
    const SupportKey& key) {
  return key.support_size == 3U
      ? closed_ball_summary_sized<3U>(cloud, key.support_ids)
      : closed_ball_summary_sized<4U>(cloud, key.support_ids);
}

[[nodiscard]] HigherSupportDeviceTiledFrontierConfig closed_rank_config(
    const MortonLbvhIndex& index,
    std::size_t maximum_relevant_closed_rank) {
  HigherSupportDeviceTiledFrontierConfig config;
  config.terminal_policy = Policy::closed_rank_window;
  config.maximum_relevant_closed_rank = maximum_relevant_closed_rank;
  config.certified_maximum_lbvh_depth =
      index.build_counters().maximum_depth;
  return config;
}

// ---------------------------------------------------------------------------
// 2. Scientific differential against the exact CPU stream oracle.
// ---------------------------------------------------------------------------

void run_differential_case(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const std::string& label) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamResult oracle =
      build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, unlimited_budget());
  check(oracle.stream_complete(), label + ": the CPU oracle completes");
  const std::size_t point_count = cloud.size();
  const std::size_t maximum_relevant_closed_rank = std::min(
      std::min(requested_maximum_order, point_count) + 1U, point_count);
  check(
      oracle.requirements.maximum_relevant_closed_rank ==
          maximum_relevant_closed_rank,
      label + ": the oracle derives the same closed-rank window");
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);

  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledFrontierContext context{
      std::move(lease),
      closed_rank_config(index, maximum_relevant_closed_rank)};
  std::vector<ExactHigherSupportFrontierEntry> roots;
  roots.push_back(root_entry(3U, geometry));
  if (point_count >= 4U) {
    roots.push_back(root_entry(4U, geometry));
  }
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{
          roots.data(), roots.size()});
  const TileRun run = run_active_tile(context, label);
  if (!run.completed) {
    return;
  }

  // Exact mass identities: tile universe == C(n,3)+C(n,4) in BigInt and
  // universe == well + rank + terminal with zero unresolved mass.
  check(
      run.universe ==
          exact_higher_support_candidate_universe_size(point_count),
      label + ": the tile universe equals C(n,3)+C(n,4) in exact BigInt");
  check(
      run.unresolved == 0 && run.resolved == run.universe &&
          run.well + run.rank + run.terminal == run.universe,
      label + ": the closed tile partitions its universe exactly");
  check(
      run.maximum_stack_high_water <=
          higher_support_device_tiled_frontier_proved_stack_bound(
              index.build_counters().maximum_depth),
      label + ": the observed slot stack respects the proved 16*depth + 1 "
              "bound");

  // Per-record closure: every drained record replays and their masses
  // re-add to the audited cumulative masses without any hidden channel.
  BigInt well_sum{0};
  BigInt rank_sum{0};
  for (const DrainedPrune& drained : run.records.prunes) {
    replay_prune_record(
        geometry,
        Policy::closed_rank_window,
        maximum_relevant_closed_rank,
        drained,
        label);
    const BigInt mass = bigint_from_words(
        drained.record.pruned_mass_lo, drained.record.pruned_mass_hi);
    if (drained.record.prune_kind ==
        static_cast<std::uint64_t>(
            Phase15HigherSupportDeviceTiledPruneKind::
                no_well_centered_support)) {
      well_sum += mass;
    } else {
      rank_sum += mass;
    }
  }
  check(
      well_sum == run.well && rank_sum == run.rank &&
          BigInt{static_cast<std::uint64_t>(run.records.terminals.size())} ==
              run.terminal,
      label + ": the drained record masses re-add to the audited "
              "cumulative masses");

  // Terminal records replay their exact geometry category, and the minimal
  // ones are classified on the host against the exact closed ball.
  std::set<SupportKey> minimal_terminals;
  for (const DrainedTerminal& drained : run.records.terminals) {
    replay_terminal_record(
        cloud, Policy::closed_rank_window, drained, label);
    if (drained.record.terminal_geometry_decision !=
        static_cast<std::uint8_t>(
            ExactHigherSupportTerminalGeometryDecision::minimal)) {
      continue;
    }
    SupportKey key;
    key.support_size = drained.record.support_size;
    for (std::size_t index = 0U; index < 4U; ++index) {
      key.support_ids[index] = drained.record.point_ids[index];
    }
    check(
        minimal_terminals.insert(key).second,
        label + ": a minimal terminal support is emitted exactly once");
  }

  std::set<SupportKey> oracle_events;
  for (const auto& event : oracle.events) {
    oracle_events.insert(SupportKey{event.support_size, event.support_ids});
  }
  std::set<SupportKey> oracle_diagnostics;
  for (const auto& diagnostic : oracle.relevant_extra_shell_diagnostics) {
    oracle_diagnostics.insert(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  bool oracle_covered = true;
  for (const SupportKey& key : oracle_events) {
    oracle_covered = oracle_covered && minimal_terminals.count(key) != 0U;
  }
  for (const SupportKey& key : oracle_diagnostics) {
    oracle_covered = oracle_covered && minimal_terminals.count(key) != 0U;
  }
  check(
      oracle_covered,
      label + ": every accepted oracle event and extra-shell diagnostic "
              "appears among the minimal terminal records");
  bool classification_agrees = true;
  for (const SupportKey& key : minimal_terminals) {
    const ClosedBallSummary summary = closed_ball_summary(cloud, key);
    if (!summary.minimal) {
      classification_agrees = false;
      continue;
    }
    const bool within_window =
        key.support_size + summary.interior_count <=
        maximum_relevant_closed_rank;
    if (within_window) {
      if (summary.shell_count == key.support_size) {
        classification_agrees =
            classification_agrees && oracle_events.count(key) != 0U;
      } else {
        classification_agrees =
            classification_agrees && oracle_diagnostics.count(key) != 0U;
      }
    } else {
      classification_agrees = classification_agrees &&
          oracle_events.count(key) == 0U &&
          oracle_diagnostics.count(key) == 0U;
    }
  }
  check(
      classification_agrees,
      label + ": the host closed-ball classification of every minimal "
              "terminal support agrees with the oracle event and "
              "diagnostic sets");
}

void test_scientific_differential_matches_exact_stream() {
  const std::array<std::size_t, 2> orders{3U, 5U};
  {
    const CanonicalPointCloud cloud = line_cloud(12U);
    for (const std::size_t order : orders) {
      run_differential_case(
          cloud, order, "line12/K=" + std::to_string(order));
    }
  }
  {
    const CanonicalPointCloud cloud = cluster_cloud();
    for (const std::size_t order : orders) {
      run_differential_case(
          cloud, order, "clusters10/K=" + std::to_string(order));
    }
  }
  {
    const CanonicalPointCloud cloud = perturbed_sphere_cloud();
    for (const std::size_t order : orders) {
      run_differential_case(
          cloud, order, "sphere8/K=" + std::to_string(order));
    }
  }
}

// ---------------------------------------------------------------------------
// 3. Anti-forge: every corruption of the fake seam is rejected fail-closed
// and poisons the context irreversibly.
// ---------------------------------------------------------------------------

void test_hostile_slot_controls_poison_fail_stop() {
  const std::vector<Corruption> corruptions{
      Corruption::stale_tile_epoch,
      Corruption::stale_chunk_sequence,
      Corruption::cumulative_rollback,
      Corruption::broken_partition,
      Corruption::chunk_ready_without_full_segment,
      Corruption::fatal_with_partial_output,
      Corruption::active_slot_status,
      Corruption::mass_without_record,
      Corruption::forged_root_digest,
      Corruption::stack_high_water_overflow,
      Corruption::nonzero_failure_code,
      Corruption::forged_cuda_execution,
      Corruption::metadata_digest_corruption};
  const CanonicalPointCloud cloud = line_cloud(8U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);
  for (const Corruption corruption : corruptions) {
    reset_fake_gpu_higher_support_device_tiled_frontier();
    auto lease = traversal_lease(cloud);
    bind_fake_higher_support_device_tiled_geometry(index, cloud);
    HigherSupportDeviceTiledFrontierContext context{
        std::move(lease), closed_rank_config(index, 4U)};
    const std::array<ExactHigherSupportFrontierEntry, 2> roots{
        root_entry(3U, geometry), root_entry(4U, geometry)};
    context.open_tile(
        std::span<const ExactHigherSupportFrontierEntry>{roots});
    set_fake_gpu_higher_support_device_tiled_frontier_corruption(
        corruption);
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a corrupted device batch is rejected fail-closed");
    check(
        context.poisoned() && !context.ready(),
        "a corrupted device batch poisons the context");
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a poisoned context refuses every further advance");
  }
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 4. Adversarial stack bound and certified-depth guards.
// ---------------------------------------------------------------------------

void test_line64_stack_bound_and_depth_guards() {
  const CanonicalPointCloud cloud = line_cloud(64U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);
  const std::size_t certified_depth = index.build_counters().maximum_depth;

  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledFrontierContext context{
      std::move(lease), closed_rank_config(index, 4U)};
  const std::array<ExactHigherSupportFrontierEntry, 2> roots{
      root_entry(3U, geometry), root_entry(4U, geometry)};
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{roots});
  const TileRun run = run_active_tile(context, "line64");
  check(
      run.completed && run.maximum_stack_high_water >= 1U &&
          run.maximum_stack_high_water <=
              higher_support_device_tiled_frontier_proved_stack_bound(
                  certified_depth) &&
          higher_support_device_tiled_frontier_proved_stack_bound(
              certified_depth) <=
              higher_support_device_tiled_frontier_products_per_slot,
      "line64: the audited stack high water respects the proved "
      "16*depth + 1 bound");
  check(
      run.completed &&
          run.universe == exact_higher_support_candidate_universe_size(64U) &&
          run.unresolved == 0,
      "line64: the adversarial tile closes its exact universe");

  // Certified-depth guards fail closed before any adoption or launch.
  auto rejected_depth_lease = traversal_lease(cloud);
  HigherSupportDeviceTiledFrontierConfig beyond_certified =
      closed_rank_config(index, 4U);
  beyond_certified.certified_maximum_lbvh_depth = 95U;
  check_throws<std::invalid_argument>(
      [&rejected_depth_lease, &beyond_certified] {
        HigherSupportDeviceTiledFrontierContext invalid{
            std::move(rejected_depth_lease), beyond_certified};
      },
      "a certified depth beyond the 94-level proof is rejected");
  check(
      rejected_depth_lease.ready(),
      "the rejected depth preflight preserves the source lease");

  auto rejected_stack_lease = traversal_lease(cloud);
  HigherSupportDeviceTiledFrontierConfig beyond_stack =
      closed_rank_config(index, 4U);
  beyond_stack.certified_maximum_lbvh_depth = 100U;
  check_throws<std::invalid_argument>(
      [&rejected_stack_lease, &beyond_stack] {
        HigherSupportDeviceTiledFrontierContext invalid{
            std::move(rejected_stack_lease), beyond_stack};
      },
      "a simulated 16*depth + 1 > 1536 stack requirement is rejected");
  check(
      rejected_stack_lease.ready(),
      "the rejected stack preflight preserves the source lease");

  // A gate-evaluation quantum of one could never retire a product (the
  // analyze stage consumes two gates atomically), so it must be rejected
  // at configuration time instead of surfacing as a poisoned launcher
  // failure on an otherwise valid context.
  auto rejected_quantum_lease = traversal_lease(cloud);
  HigherSupportDeviceTiledFrontierConfig unit_quantum =
      closed_rank_config(index, 4U);
  unit_quantum.gate_evaluations_per_slot_per_chunk = 1U;
  check_throws<std::out_of_range>(
      [&rejected_quantum_lease, &unit_quantum] {
        HigherSupportDeviceTiledFrontierContext invalid{
            std::move(rejected_quantum_lease), unit_quantum};
      },
      "a unit gate-evaluation quantum is rejected at configuration time");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// A chunk that mixes a slot completed within the chunk and a slot stopped
// by the subdivision-capacity budget cannot form an atomic terminal batch:
// the completed slot must withhold its current output chunk, which reopens
// its universe.  The launch fails and poisons, mirroring the pair-model
// two-stage rollback, instead of publishing a mixed fatal/complete tile.
void test_mixed_fatal_complete_chunk_fails_atomically() {
  const CanonicalPointCloud cloud = line_cloud(32U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  // Rank window 3 makes the support-4 root resolve in a single gate
  // (t_4 == 0), while one subdivision pass cannot settle the support-3
  // slot on 32 collinear points.
  HigherSupportDeviceTiledFrontierConfig config =
      closed_rank_config(index, 3U);
  config.gate_evaluations_per_slot_per_chunk = 8U;
  config.maximum_subdivision_count = 1U;
  HigherSupportDeviceTiledFrontierContext context{
      std::move(lease), config};
  const std::array<ExactHigherSupportFrontierEntry, 2> roots{
      root_entry(3U, geometry), root_entry(4U, geometry)};
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{roots});
  check_throws<std::runtime_error>(
      [&context] {
        auto advance = context.advance();
        static_cast<void>(advance);
      },
      "a chunk mixing a completed slot with a capacity-fatal slot fails "
      "atomically");
  check(
      context.poisoned(),
      "the mixed fatal/complete launch poisons the context");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 5. Unsigned 128-bit binomials against exact BigInt at both boundaries.
// ---------------------------------------------------------------------------

void test_u128_binomials_match_bigint() {
  std::vector<std::uint64_t> ranges;
  for (std::uint64_t value = 0U; value <= 12U; ++value) {
    ranges.push_back(value);
  }
  for (std::uint64_t value =
           higher_support_device_tiled_frontier_maximum_point_count - 4U;
       value <= higher_support_device_tiled_frontier_maximum_point_count;
       ++value) {
    ranges.push_back(value);
  }
  bool all_match = true;
  for (const std::uint64_t range : ranges) {
    for (std::uint64_t multiplicity = 2U; multiplicity <= 4U;
         ++multiplicity) {
      Phase15HigherSupportDeviceTiledUnsigned128 out{};
      if (!phase15_higher_support_device_tiled_binomial_u128(
              range, multiplicity, out)) {
        all_match = false;
        continue;
      }
      const BigInt observed = bigint_from_words(out.lo, out.hi);
      if (observed !=
          bigint_binomial(range, static_cast<std::size_t>(multiplicity))) {
        all_match = false;
      }
    }
  }
  check(
      all_match,
      "the unsigned 128-bit progressive binomials match exact BigInt on "
      "both boundaries");
  bool overflow_refused = true;
  for (std::uint64_t multiplicity = 2U; multiplicity <= 4U;
       ++multiplicity) {
    Phase15HigherSupportDeviceTiledUnsigned128 out{};
    overflow_refused = overflow_refused &&
        !phase15_higher_support_device_tiled_binomial_u128(
            higher_support_device_tiled_frontier_maximum_point_count + 1U,
            multiplicity,
            out);
  }
  check(
      overflow_refused,
      "a point count beyond 2^31 is refused by the certified binomial "
      "scheme");
}

// ---------------------------------------------------------------------------
// 6. Watertight terminal policies: a receipt minted under one policy never
// prunes under the other, and every record is sealed to its policy.
// ---------------------------------------------------------------------------

[[nodiscard]] TileRun run_policy_tile(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    const ReplayGeometry& geometry,
    Policy policy,
    std::size_t maximum_relevant_closed_rank,
    bool include_support_four,
    const std::string& label) {
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledFrontierConfig config =
      closed_rank_config(index, maximum_relevant_closed_rank);
  config.terminal_policy = policy;
  HigherSupportDeviceTiledFrontierContext context{
      std::move(lease), config};
  std::vector<ExactHigherSupportFrontierEntry> roots;
  roots.push_back(root_entry(3U, geometry));
  if (include_support_four) {
    roots.push_back(root_entry(4U, geometry));
  }
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{
          roots.data(), roots.size()});
  return run_active_tile(context, label);
}

void test_policy_watertightness() {
  check(
      higher_support_device_tiled_frontier_required_witness_count(
          Policy::closed_rank_window, 3U, 4U) == 2U &&
          higher_support_device_tiled_frontier_required_witness_count(
              Policy::closed_rank_window, 4U, 4U) == 1U &&
          higher_support_device_tiled_frontier_required_witness_count(
              Policy::closed_rank_window, 3U, 2U) == 0U &&
          higher_support_device_tiled_frontier_required_witness_count(
              Policy::strict_interior_carrier_q3, 3U, 11U) == 1U,
      "the sealed policy thresholds are t_m for the window and one for "
      "the carrier");

  const CanonicalPointCloud cloud = gabriel_witness_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);
  SupportKey triangle_key;
  triangle_key.support_size = 3U;
  {
    std::array<PointId, 3> ids{
        find_point_id(cloud, point(0.0, 0.0, 0.0)),
        find_point_id(cloud, point(4.0, 0.0, 0.0)),
        find_point_id(cloud, point(2.0, 3.0, 0.0))};
    std::sort(ids.begin(), ids.end());
    for (std::size_t index_in_key = 0U; index_in_key < 3U;
         ++index_in_key) {
      triangle_key.support_ids[index_in_key] = ids[index_in_key];
    }
  }
  const ClosedBallSummary triangle_ball =
      closed_ball_summary(cloud, triangle_key);
  check(
      triangle_ball.minimal && triangle_ball.interior_count == 1U &&
          triangle_ball.shell_count == 3U,
      "the Gabriel triangle fixture has exactly one strict interior "
      "witness and a bare shell");

  // Carrier run: one strict interior witness suffices, so the triangle is
  // pruned by a one-receipt strict-interior-carrier certificate and never
  // reaches terminal classification.
  const TileRun carrier_run = run_policy_tile(
      cloud,
      index,
      geometry,
      Policy::strict_interior_carrier_q3,
      4U,
      false,
      "carrier");
  bool carrier_one_receipt_prune = false;
  bool carrier_records_sealed = true;
  for (const DrainedPrune& drained : carrier_run.records.prunes) {
    replay_prune_record(
        geometry,
        Policy::strict_interior_carrier_q3,
        4U,
        drained,
        "carrier");
    carrier_records_sealed = carrier_records_sealed &&
        drained.record.prune_kind !=
            static_cast<std::uint64_t>(
                Phase15HigherSupportDeviceTiledPruneKind::rank_window);
    if (drained.record.prune_kind ==
            static_cast<std::uint64_t>(
                Phase15HigherSupportDeviceTiledPruneKind::
                    strict_interior_carrier) &&
        drained.record.receipt_count == 1U &&
        drained.receipts.size() == 1U &&
        drained.receipts[0].certified_point_count == 1U) {
      carrier_one_receipt_prune = true;
    }
  }
  bool carrier_triangle_terminal = false;
  for (const DrainedTerminal& drained : carrier_run.records.terminals) {
    replay_terminal_record(
        cloud, Policy::strict_interior_carrier_q3, drained, "carrier");
    SupportKey key;
    key.support_size = drained.record.support_size;
    for (std::size_t index_in_key = 0U; index_in_key < 4U;
         ++index_in_key) {
      key.support_ids[index_in_key] =
          drained.record.point_ids[index_in_key];
    }
    carrier_triangle_terminal =
        carrier_triangle_terminal || key == triangle_key;
  }
  check(
      carrier_run.completed && carrier_one_receipt_prune &&
          carrier_records_sealed && !carrier_triangle_terminal,
      "under the carrier policy a single strict interior receipt prunes "
      "the Gabriel triangle before terminal classification");

  // Closed-rank-window run on the same cloud: the same product demands t_m
  // receipts, the lone witness cannot reach the threshold, and the triangle
  // surfaces as a minimal terminal record instead.
  const TileRun closed_run = run_policy_tile(
      cloud,
      index,
      geometry,
      Policy::closed_rank_window,
      4U,
      true,
      "closed-window");
  bool closed_records_sealed = true;
  for (const DrainedPrune& drained : closed_run.records.prunes) {
    replay_prune_record(
        geometry, Policy::closed_rank_window, 4U, drained, "closed-window");
    closed_records_sealed = closed_records_sealed &&
        drained.record.prune_kind !=
            static_cast<std::uint64_t>(
                Phase15HigherSupportDeviceTiledPruneKind::
                    strict_interior_carrier);
  }
  bool closed_triangle_minimal = false;
  for (const DrainedTerminal& drained : closed_run.records.terminals) {
    replay_terminal_record(
        cloud, Policy::closed_rank_window, drained, "closed-window");
    SupportKey key;
    key.support_size = drained.record.support_size;
    for (std::size_t index_in_key = 0U; index_in_key < 4U;
         ++index_in_key) {
      key.support_ids[index_in_key] =
          drained.record.point_ids[index_in_key];
    }
    if (key == triangle_key &&
        drained.record.terminal_geometry_decision ==
            static_cast<std::uint8_t>(
                ExactHigherSupportTerminalGeometryDecision::minimal)) {
      closed_triangle_minimal = true;
    }
  }
  check(
      closed_run.completed && closed_records_sealed &&
          closed_triangle_minimal,
      "under the closed-rank window the one-witness receipt never prunes "
      "and the Gabriel triangle stays a minimal terminal support");

  // A support-four entry is inadmissible under the carrier policy at
  // tile-open time, and the rejection is reversible.
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto carrier_lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledFrontierConfig carrier_config =
      closed_rank_config(index, 4U);
  carrier_config.terminal_policy = Policy::strict_interior_carrier_q3;
  HigherSupportDeviceTiledFrontierContext carrier_context{
      std::move(carrier_lease), carrier_config};
  const std::array<ExactHigherSupportFrontierEntry, 2> mixed_roots{
      root_entry(3U, geometry), root_entry(4U, geometry)};
  check_throws<std::invalid_argument>(
      [&carrier_context, &mixed_roots] {
        carrier_context.open_tile(
            std::span<const ExactHigherSupportFrontierEntry>{mixed_roots});
      },
      "a support-four entry is rejected by the carrier policy at tile "
      "open");
  check(
      !carrier_context.poisoned() && !carrier_context.tile_active(),
      "the rejected carrier tile leaves the context reusable");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 7. Backpressure: a live record drain lease is a reversible precondition
// failure, never a poisoning launcher failure.
// ---------------------------------------------------------------------------

void test_backpressure_is_reversible() {
  const CanonicalPointCloud cloud = line_cloud(8U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ReplayGeometry geometry = build_replay_geometry(index, cloud);
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledFrontierContext context{
      std::move(lease), closed_rank_config(index, 4U)};
  const std::array<ExactHigherSupportFrontierEntry, 2> roots{
      root_entry(3U, geometry), root_entry(4U, geometry)};
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{roots});
  auto advance = context.advance();
  check(
      advance.status == Status::frontier_tile_complete &&
          advance.record_drain.has_value(),
      "the backpressure fixture publishes one record drain lease");
  if (!advance.record_drain.has_value()) {
    return;
  }
  const std::size_t launches_before =
      fake_gpu_higher_support_device_tiled_frontier_launch_count();
  check_throws<std::logic_error>(
      [&context] { (void)context.advance(); },
      "a live record drain lease applies one-lease backpressure on "
      "advance");
  check_throws<std::logic_error>(
      [&context, &roots] {
        context.open_tile(
            std::span<const ExactHigherSupportFrontierEntry>{roots});
      },
      "a live record drain lease applies one-lease backpressure on "
      "open_tile");
  check(
      !context.poisoned() && context.ready() &&
          fake_gpu_higher_support_device_tiled_frontier_launch_count() ==
              launches_before,
      "backpressure launches no work and does not poison the context");
  advance.record_drain.reset();
  context.open_tile(
      std::span<const ExactHigherSupportFrontierEntry>{roots});
  auto resumed = context.advance();
  check(
      resumed.status == Status::frontier_tile_complete &&
          !context.poisoned(),
      "the context resumes normally once the drain lease is destroyed");
  resumed.record_drain.reset();
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 8. AABB corner-regression fixture: corner agreement never becomes a
// universal certificate, so the exact decision stays inconclusive.
// ---------------------------------------------------------------------------

[[nodiscard]] std::vector<std::string> bracket_tokens(
    const std::string& text,
    std::size_t& cursor,
    const std::string& key) {
  const std::size_t key_position = text.find('"' + key + '"', cursor);
  if (key_position == std::string::npos) {
    return {};
  }
  const std::size_t open = text.find('[', key_position);
  if (open == std::string::npos) {
    return {};
  }
  std::vector<std::string> tokens;
  std::string current;
  const auto flush = [&tokens, &current]() {
    std::string cleaned;
    for (const char character : current) {
      if (character != ' ' && character != '\n' && character != '\r' &&
          character != '\t') {
        cleaned.push_back(character);
      }
    }
    if (!cleaned.empty()) {
      tokens.push_back(cleaned);
    }
    current.clear();
  };
  std::size_t depth = 1U;
  std::size_t position = open + 1U;
  while (position < text.size() && depth > 0U) {
    const char character = text[position];
    if (character == '[') {
      ++depth;
      flush();
    } else if (character == ']') {
      --depth;
      flush();
    } else if (character == ',') {
      flush();
    } else {
      current.push_back(character);
    }
    ++position;
  }
  flush();
  cursor = position;
  return tokens;
}

// Parses an exact dyadic rational token ("-1/2", "33/16", 2, "t"...) into
// its exact binary64 value.  Only powers of two are admissible denominators
// and the numerator must stay below 2^53, so the conversion is exact by
// construction and no floating-point rounding participates.
[[nodiscard]] bool parse_dyadic_token(
    const std::string& token,
    double& out) {
  std::string cleaned;
  for (const char character : token) {
    if (character != '"') {
      cleaned.push_back(character);
    }
  }
  if (cleaned.empty()) {
    return false;
  }
  std::size_t position = 0U;
  bool negative = false;
  if (cleaned[0] == '-') {
    negative = true;
    position = 1U;
  }
  if (position >= cleaned.size() || cleaned[position] < '0' ||
      cleaned[position] > '9') {
    return false;
  }
  std::uint64_t numerator = 0U;
  while (position < cleaned.size() && cleaned[position] >= '0' &&
         cleaned[position] <= '9') {
    numerator = numerator * 10U +
        static_cast<std::uint64_t>(cleaned[position] - '0');
    ++position;
  }
  std::uint64_t denominator = 1U;
  if (position < cleaned.size()) {
    if (cleaned[position] != '/') {
      return false;
    }
    ++position;
    if (position >= cleaned.size()) {
      return false;
    }
    denominator = 0U;
    while (position < cleaned.size() && cleaned[position] >= '0' &&
           cleaned[position] <= '9') {
      denominator = denominator * 10U +
          static_cast<std::uint64_t>(cleaned[position] - '0');
      ++position;
    }
    if (position != cleaned.size() || denominator == 0U) {
      return false;
    }
  }
  if ((denominator & (denominator - 1U)) != 0U ||
      numerator >= (UINT64_C(1) << 53U)) {
    return false;
  }
  // std::bit_width returns int per the standard, but the width type differs
  // across libstdc++ versions and the G4 toolchain rejects the narrowing
  // under -Werror=conversion.  The value is at most 64, so the cast is exact.
  const int shift = static_cast<int>(std::bit_width(denominator)) - 1;
  const double magnitude =
      std::ldexp(static_cast<double>(numerator), -shift);
  out = negative ? -magnitude : magnitude;
  return true;
}

[[nodiscard]] ExactDyadicAabb3 dyadic_box(
    double x_lower,
    double x_upper,
    double y_lower,
    double y_upper,
    double z_lower,
    double z_upper) {
  ExactDyadicAabb3 box{};
  box.lower_binary64_bits = {
      std::bit_cast<std::uint64_t>(x_lower),
      std::bit_cast<std::uint64_t>(y_lower),
      std::bit_cast<std::uint64_t>(z_lower)};
  box.upper_binary64_bits = {
      std::bit_cast<std::uint64_t>(x_upper),
      std::bit_cast<std::uint64_t>(y_upper),
      std::bit_cast<std::uint64_t>(z_upper)};
  return box;
}

[[nodiscard]] ExactDyadicAabb3 singleton_box(double x, double y, double z) {
  return dyadic_box(x, x, y, y, z, z);
}

void test_aabb_corner_fixture_stays_inconclusive() {
  std::ifstream stream{
      MORSEHGP3D_HIGHER_SUPPORT_AABB_CORNER_REGRESSION_FIXTURE};
  check(
      stream.good(),
      "the higher-support AABB corner regression fixture is readable");
  if (!stream.good()) {
    return;
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const std::string text = buffer.str();
  const std::size_t id_key = text.find("\"fixture_id\"");
  const std::size_t id_value =
      text.find("higher-support-aabb-corner-regression-v1", id_key);
  check(
      id_key != std::string::npos && id_value != std::string::npos &&
          id_value - id_key < 100U,
      "the fixture identifies itself as the v1 corner regression");

  std::size_t cursor = 0U;
  const std::vector<std::string> fixed_tokens =
      bracket_tokens(text, cursor, "fixed_support_points");
  cursor = 0U;
  const std::vector<std::string> moving_tokens =
      bracket_tokens(text, cursor, "moving_support");
  std::size_t well_cursor = text.find("\"well_centering_case\"");
  const std::vector<std::string> well_interval =
      bracket_tokens(text, well_cursor, "t_interval");
  std::size_t power_cursor = text.find("\"query_power_case\"");
  const std::vector<std::string> power_interval =
      bracket_tokens(text, power_cursor, "t_interval");
  const std::vector<std::string> query_tokens =
      bracket_tokens(text, power_cursor, "query_point");
  check(
      fixed_tokens.size() == 6U && moving_tokens.size() == 3U &&
          well_interval.size() == 2U && power_interval.size() == 2U &&
          query_tokens.size() == 3U,
      "the fixture exposes both cases with their exact fields");
  if (fixed_tokens.size() != 6U || moving_tokens.size() != 3U ||
      well_interval.size() != 2U || power_interval.size() != 2U ||
      query_tokens.size() != 3U) {
    return;
  }
  std::array<double, 6> fixed_values{};
  bool parsed = true;
  for (std::size_t index = 0U; index < fixed_values.size(); ++index) {
    parsed =
        parsed && parse_dyadic_token(fixed_tokens[index], fixed_values[index]);
  }
  double moving_y = 0.0;
  double moving_z = 0.0;
  parsed = parsed && parse_dyadic_token(moving_tokens[1], moving_y) &&
      parse_dyadic_token(moving_tokens[2], moving_z);
  double well_lower = 0.0;
  double well_upper = 0.0;
  parsed = parsed && parse_dyadic_token(well_interval[0], well_lower) &&
      parse_dyadic_token(well_interval[1], well_upper);
  double power_lower = 0.0;
  double power_upper = 0.0;
  parsed = parsed && parse_dyadic_token(power_interval[0], power_lower) &&
      parse_dyadic_token(power_interval[1], power_upper);
  std::array<double, 3> query_values{};
  for (std::size_t index = 0U; index < query_values.size(); ++index) {
    parsed =
        parsed && parse_dyadic_token(query_tokens[index], query_values[index]);
  }
  check(
      parsed && well_lower < well_upper && power_lower < power_upper,
      "every fixture field parses as an exact dyadic rational");
  if (!parsed) {
    return;
  }

  const ExactDyadicAabb3 first_fixed =
      singleton_box(fixed_values[0], fixed_values[1], fixed_values[2]);
  const ExactDyadicAabb3 second_fixed =
      singleton_box(fixed_values[3], fixed_values[4], fixed_values[5]);

  // Well-centring case: both corner triangles are non-well-centred while
  // an interior support is well-centred, so neither one-sided universal
  // certificate may fire on the reconstructed product.
  const std::array<ExactDyadicAabb3, 3> well_boxes{
      first_fixed,
      second_fixed,
      dyadic_box(
          well_lower, well_upper, moving_y, moving_y, moving_z, moving_z)};
  const auto analysis = exact_higher_support_product_aabb_analysis(
      std::span<const ExactDyadicAabb3>{well_boxes.data(),
                                        well_boxes.size()});
  check(
      !analysis.no_well_centered_support_certified() &&
          !analysis.all_supports_well_centered_certified(),
      "corner-agreeing well-centring verdicts never certify the full box "
      "product");

  // Query-power case: both corner spheres strictly contain the query while
  // an interior support excludes it, so the exact two-sided query-cell
  // decision must remain inconclusive.
  const std::array<ExactDyadicAabb3, 3> power_boxes{
      first_fixed,
      second_fixed,
      dyadic_box(
          power_lower, power_upper, moving_y, moving_y, moving_z,
          moving_z)};
  const ExactDyadicAabb3 query_box = singleton_box(
      query_values[0], query_values[1], query_values[2]);
  const ExactHigherSupportProductQueryCellDecision decision =
      exact_higher_support_product_query_cell_decision(
          std::span<const ExactDyadicAabb3>{power_boxes.data(),
                                            power_boxes.size()},
          query_box);
  check(
      decision == ExactHigherSupportProductQueryCellDecision::inconclusive,
      "the corner-regression query-cell decision preserves its ambiguity");
}

}  // namespace

int main() {
  test_u128_binomials_match_bigint();
  test_scientific_differential_matches_exact_stream();
  test_hostile_slot_controls_poison_fail_stop();
  test_line64_stack_bound_and_depth_guards();
  test_mixed_fatal_complete_chunk_fails_atomically();
  test_policy_watertightness();
  test_backpressure_is_reversible();
  test_aabb_corner_fixture_stays_inconclusive();
  if (failures != 0) {
    std::cerr << failures
              << " device tiled higher-support contract test(s) failed\n";
    return 1;
  }
  std::cout << "device tiled higher-support contract tests passed\n";
  return 0;
}
