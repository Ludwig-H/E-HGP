#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"
#include "phase15_higher_support_device_tiled_slot_engine.cuh"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

// Host-compiled bit-parity differential of the Phase 15 higher-support slot
// engine against the scientific host fake, on identical tiles and identical
// chunk sequences, at the launcher seam.  The engine is the exact scientific
// core the native CUDA kernel executes per slot, so this suite certifies the
// whole M3/M4 decision surface without a GPU: every slot control field
// except the three backend routing counters (deferred_int512_count,
// deferred_int1024_count, rational_drain_count, which the sealed contract
// forces to zero on the fake and merely bounds on the native path) and every
// published prune record, terminal record and probe receipt must be
// bit-identical chunk by chunk.

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::higher_support_device_tiled_frontier_maximum_closed_rank;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_pending_decisions_per_slot;
using morsehgp3d::gpu::higher_support_device_tiled_frontier_products_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_probe_receipts_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_prune_records_per_slot;
using morsehgp3d::gpu::
    higher_support_device_tiled_frontier_terminal_records_per_slot;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierTerminalPolicy;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::detail::
    build_phase15_higher_support_device_tiled_frontier_on_device;
using morsehgp3d::gpu::detail::
    adopt_phase15_higher_support_device_tiled_traversal;
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
using morsehgp3d::gpu::test_support::
    bind_fake_higher_support_device_tiled_geometry;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_higher_support_device_tiled_frontier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
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

// Adversarial dyadic-exponent spread: aligning these coordinates to one
// common exponent exceeds the 124-bit int1024 envelope, so every gate on
// mixed-scale products must escalate through the ladder into the host
// rational drain.  The fake resolves the same decisions through its host
// primitives, so records must still match bit for bit.
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

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    const CanonicalPointCloud& cloud) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  const auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

// ---------------------------------------------------------------------------
// Witness-id mirror: the engine consumes the device-node ABI (extremum
// PointIds per axis), so the test rebuilds that arena with the exact
// radix/median split of the CPU builder and certifies, node by node, that
// the witness-derived box bits equal the propagated witness-rule bits.
// ---------------------------------------------------------------------------

[[nodiscard]] std::uint64_t order_key(std::uint64_t bits) noexcept {
  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63U;
  return (bits & sign_bit) != 0U ? ~bits : bits ^ sign_bit;
}

struct WitnessGeometry {
  std::vector<engine::EngineNode> nodes;
  std::vector<std::uint64_t> coordinate_bits;  // axis-major
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
  if (!index.validated_for(cloud) || cloud.size() == 0U) {
    throw std::logic_error(
        "the witness mirror requires a validated Morton LBVH");
  }
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

// ---------------------------------------------------------------------------
// Chunk snapshots and the masked control comparison.
// ---------------------------------------------------------------------------

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

[[nodiscard]] bool controls_equal_masked(
    const Phase15HigherSupportDeviceTiledSlotControl& left,
    const Phase15HigherSupportDeviceTiledSlotControl& right) noexcept {
  Phase15HigherSupportDeviceTiledSlotControl masked_left = left;
  Phase15HigherSupportDeviceTiledSlotControl masked_right = right;
  masked_left.deferred_int512_count = 0U;
  masked_left.deferred_int1024_count = 0U;
  masked_left.rational_drain_count = 0U;
  masked_right.deferred_int512_count = 0U;
  masked_right.deferred_int1024_count = 0U;
  masked_right.rational_drain_count = 0U;
  return std::memcmp(&masked_left, &masked_right, sizeof(masked_left)) == 0;
}

// ---------------------------------------------------------------------------
// Fake seam runner: drives the scientific host fake at the internal seam,
// one Request per chunk, exactly like the host context does.
// ---------------------------------------------------------------------------

struct CaseSetup {
  Policy policy{Policy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{5U};
  std::size_t gate_quantum{2048U};
  std::size_t maximum_subdivision_count{4096U};
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

[[nodiscard]] TileTranscript run_fake_tile(
    const CaseSetup& setup,
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    std::span<const ExactHigherSupportFrontierEntry> roots,
    const std::string& label) {
  TileTranscript transcript;
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  Phase15HigherSupportDeviceTiledAdoptedTraversal adopted =
      adopt_phase15_higher_support_device_tiled_traversal(std::move(lease));
  Phase15HigherSupportDeviceTiledFrontierContextState state;
  const std::size_t certified_depth =
      index.build_counters().maximum_depth;
  std::uint64_t record_buffer_epoch = 1U;
  for (std::uint64_t chunk_sequence = 1U; chunk_sequence <= 100'000U;
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
    bool any_fatal = false;
    bool all_complete = true;
    for (std::size_t slot = 0U; slot < snapshot.controls.size(); ++slot) {
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
            batch.prune_records
                [slot * request.prune_records_per_slot + record]);
      }
      for (std::uint64_t record = 0U;
           record < control.terminal_record_count;
           ++record) {
        snapshot.terminals.push_back(
            batch.terminal_records
                [slot * request.terminal_records_per_slot + record]);
      }
      for (std::uint64_t receipt = 0U;
           receipt < control.probe_receipt_count;
           ++receipt) {
        snapshot.receipts.push_back(
            batch.probe_receipts
                [slot * request.probe_receipts_per_slot + receipt]);
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
  check(false, label + ": the fake tile did not settle");
  return transcript;
}

// ---------------------------------------------------------------------------
// Engine driver: the host-compiled equivalent of the native CUDA launcher.
// Rational suspensions are resolved inline through the exact host
// primitives, exactly as the native launcher drains them between kernel
// relaunches.
// ---------------------------------------------------------------------------

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
      check(false, label + ": a suspension carries an unknown wide kind");
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
        label + ": a root universe mass fits unsigned 128 bits");
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
        // Host rational drain: resolve inline and continue on the
        // remaining budget, as the native launcher does between kernel
        // relaunches within one subdivision.
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

    // Tile-level fatal collapse, mirror of the fake launcher.
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
          continue;
        }
        const bool completed_this_chunk =
            checkpoint.run_state ==
                static_cast<std::uint8_t>(
                    engine::EngineRunState::complete) &&
            (!(checkpoint.chunk_well_delta == engine::EngineU128{}) ||
             !(checkpoint.chunk_rank_delta == engine::EngineU128{}) ||
             !(checkpoint.chunk_terminal_delta == engine::EngineU128{}) ||
             checkpoint.chunk_expansion_count != 0U ||
             checkpoint.chunk_gate_evaluation_count != 0U ||
             checkpoint.chunk_prune_used != 0U ||
             checkpoint.chunk_terminal_used != 0U ||
             checkpoint.chunk_receipt_used != 0U);
        if (completed_this_chunk) {
          engine::engine_rollback_slot_chunk(
              slot_context(slot), checkpoint);
          check(
              false,
              label + ": a fatal engine tile could not form an atomic "
                      "terminal batch");
          transcript.fatal = true;
          return transcript;
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
          label + ": a slot control assembles in the certified range");
      check(
          engine::engine_commit_chunk(checkpoint),
          label + ": a chunk commit stays in the certified range");
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

// ---------------------------------------------------------------------------
// Differential comparison.
// ---------------------------------------------------------------------------

std::uint64_t total_engine_rational_drains = 0U;
std::size_t last_case_subdivision_total = 0U;

void compare_transcripts(
    const TileTranscript& fake,
    const TileTranscript& engine_run,
    const std::string& label) {
  // The differential must bite: an empty transcript pair would compare
  // equal vacuously, so every case must complete and publish records.
  std::size_t fake_record_total = 0U;
  for (const ChunkSnapshot& chunk : fake.chunks) {
    fake_record_total +=
        chunk.prunes.size() + chunk.terminals.size();
  }
  check(
      fake.completed && !fake.fatal && fake_record_total > 0U,
      label + ": the fake transcript completes with published records (" +
          std::to_string(fake_record_total) + ")");
  check(
      fake.completed == engine_run.completed &&
          fake.fatal == engine_run.fatal &&
          fake.chunks.size() == engine_run.chunks.size(),
      label + ": both transcripts settle identically (" +
          std::to_string(fake.chunks.size()) + " vs " +
          std::to_string(engine_run.chunks.size()) + " chunks)");
  const std::size_t chunk_count =
      std::min(fake.chunks.size(), engine_run.chunks.size());
  for (std::size_t chunk = 0U; chunk < chunk_count; ++chunk) {
    const ChunkSnapshot& fake_chunk = fake.chunks[chunk];
    const ChunkSnapshot& engine_chunk = engine_run.chunks[chunk];
    const std::string chunk_label =
        label + "/chunk" + std::to_string(chunk + 1U);
    check(
        fake_chunk.subdivision_count == engine_chunk.subdivision_count,
        chunk_label + ": subdivision counts agree");
    last_case_subdivision_total += fake_chunk.subdivision_count;
    check(
        fake_chunk.controls.size() == engine_chunk.controls.size(),
        chunk_label + ": slot extents agree");
    for (std::size_t slot = 0U;
         slot < std::min(
             fake_chunk.controls.size(), engine_chunk.controls.size());
         ++slot) {
      check(
          controls_equal_masked(
              fake_chunk.controls[slot], engine_chunk.controls[slot]),
          chunk_label + "/slot" + std::to_string(slot) +
              ": slot controls are bit-identical outside the sealed "
              "routing counters");
      check(
          fake_chunk.controls[slot].deferred_int512_count == 0U &&
              fake_chunk.controls[slot].deferred_int1024_count == 0U &&
              fake_chunk.controls[slot].rational_drain_count == 0U,
          chunk_label + ": the fake keeps its routing counters at zero");
      total_engine_rational_drains +=
          engine_chunk.controls[slot].rational_drain_count;
    }
    check(
        fake_chunk.prunes.size() == engine_chunk.prunes.size() &&
            std::memcmp(
                fake_chunk.prunes.data(),
                engine_chunk.prunes.data(),
                fake_chunk.prunes.size() *
                    sizeof(Phase15HigherSupportDeviceTiledPruneRecord)) ==
                0,
        chunk_label + ": prune records are bit-identical");
    check(
        fake_chunk.terminals.size() == engine_chunk.terminals.size() &&
            std::memcmp(
                fake_chunk.terminals.data(),
                engine_chunk.terminals.data(),
                fake_chunk.terminals.size() *
                    sizeof(
                        Phase15HigherSupportDeviceTiledTerminalRecord)) ==
                0,
        chunk_label + ": terminal records are bit-identical");
    check(
        fake_chunk.receipts.size() == engine_chunk.receipts.size() &&
            std::memcmp(
                fake_chunk.receipts.data(),
                engine_chunk.receipts.data(),
                fake_chunk.receipts.size() *
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
  const TileTranscript fake = run_fake_tile(
      setup,
      cloud,
      index,
      std::span<const ExactHigherSupportFrontierEntry>{roots},
      label);
  const TileTranscript engine_run = run_engine_tile(
      setup,
      geometry,
      std::span<const ExactHigherSupportFrontierEntry>{roots},
      label);
  compare_transcripts(fake, engine_run, label);
}

// ---------------------------------------------------------------------------
// 1. Witness box construction certifies against the exact CPU witness rule.
// ---------------------------------------------------------------------------

void test_witness_boxes_match_certified_mirror() {
  for (const CanonicalPointCloud& cloud :
       {line_cloud(12U), cluster_cloud(), perturbed_sphere_cloud()}) {
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const WitnessGeometry geometry = build_witness_geometry(index, cloud);
    const engine::EngineGeometryView view = geometry_view(geometry);
    bool all_equal = true;
    for (std::size_t node_index = 0U;
         node_index < geometry.nodes.size();
         ++node_index) {
      engine::EngineBox box{};
      engine::engine_node_box(view, node_index, box);
      const engine::EngineNode& node = geometry.nodes[node_index];
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const std::uint64_t lower_bits =
            cloud.point(node.lower_point_ids[axis])
                .canonical_input_bits()[axis];
        const std::uint64_t upper_bits =
            cloud.point(node.upper_point_ids[axis])
                .canonical_input_bits()[axis];
        all_equal = all_equal && box.lower[axis] == lower_bits &&
            box.upper[axis] == upper_bits;
      }
    }
    check(
        all_equal,
        "witness-derived node boxes equal the canonical witness bits");
  }
}

// ---------------------------------------------------------------------------
// 2. Full seam differential: fake vs engine on identical tiles.
// ---------------------------------------------------------------------------

void test_engine_matches_fake_bit_for_bit() {
  const std::array<std::size_t, 2> both_supports{3U, 4U};
  const std::array<std::size_t, 1> support_three{3U};
  {
    CaseSetup setup;
    setup.maximum_relevant_closed_rank = 5U;
    run_parity_case(
        setup,
        line_cloud(12U),
        std::span<const std::size_t>{both_supports},
        "line12/K5");
    setup.maximum_relevant_closed_rank = 3U;
    run_parity_case(
        setup,
        line_cloud(12U),
        std::span<const std::size_t>{both_supports},
        "line12/K3");
  }
  {
    CaseSetup setup;
    setup.maximum_relevant_closed_rank = 5U;
    run_parity_case(
        setup,
        cluster_cloud(),
        std::span<const std::size_t>{both_supports},
        "clusters10/K5");
  }
  {
    CaseSetup setup;
    setup.maximum_relevant_closed_rank = 3U;
    run_parity_case(
        setup,
        perturbed_sphere_cloud(),
        std::span<const std::size_t>{both_supports},
        "sphere8/K3");
  }
  {
    CaseSetup setup;
    setup.policy = Policy::strict_interior_carrier_q3;
    setup.maximum_relevant_closed_rank = 5U;
    run_parity_case(
        setup,
        perturbed_sphere_cloud(),
        std::span<const std::size_t>{support_three},
        "sphere8/carrier");
  }
  {
    CaseSetup setup;
    setup.maximum_relevant_closed_rank = 5U;
    run_parity_case(
        setup,
        line_cloud(14U),
        std::span<const std::size_t>{both_supports},
        "line14/K5");
  }
}

// ---------------------------------------------------------------------------
// 3. Chunked parity under a minimal gate quantum: maximal pause pressure,
// many subdivisions, capacity yields when segments fill.
// ---------------------------------------------------------------------------

void test_engine_matches_fake_under_minimal_quantum() {
  last_case_subdivision_total = 0U;
  const std::array<std::size_t, 2> both_supports{3U, 4U};
  CaseSetup setup;
  setup.gate_quantum = 2U;
  setup.maximum_relevant_closed_rank = 5U;
  run_parity_case(
      setup,
      cluster_cloud(),
      std::span<const std::size_t>{both_supports},
      "clusters10/quantum2");
  setup.maximum_relevant_closed_rank = 3U;
  run_parity_case(
      setup,
      perturbed_sphere_cloud(),
      std::span<const std::size_t>{both_supports},
      "sphere8/quantum2");
  check(
      last_case_subdivision_total > 2U,
      "the minimal-quantum differential exercised repeated bounded "
      "subdivisions (" +
          std::to_string(last_case_subdivision_total) + ")");
}

// ---------------------------------------------------------------------------
// 4. Rational drain differential: the wide-dyadic cloud exceeds the 124-bit
// aligned envelope, so the engine must escalate through int512/int1024 into
// the suspended host rational drain while reproducing the fake bit for bit.
// ---------------------------------------------------------------------------

void test_engine_rational_drain_matches_fake() {
  const std::array<std::size_t, 2> both_supports{3U, 4U};
  const std::uint64_t before = total_engine_rational_drains;
  CaseSetup setup;
  setup.maximum_relevant_closed_rank = 4U;
  run_parity_case(
      setup,
      wide_dyadic_cloud(),
      std::span<const std::size_t>{both_supports},
      "wide-dyadic/K4");
  check(
      total_engine_rational_drains > before,
      "the wide-dyadic differential exercised the engine rational drain");
}

// ---------------------------------------------------------------------------
// 5. Corner-regression ambiguity per width: the sealed fixture semantics
// (corner agreement never certifies a box product) hold in every device
// width of the ladder.
// ---------------------------------------------------------------------------

void test_corner_fixture_ambiguous_in_every_width() {
  // The corner-regression product of
  // docs/research + tests/fixtures: two fixed singleton supports and one
  // moving support whose corner instances agree while an interior instance
  // disagrees.  Reconstructed here with the exact dyadic values of the
  // sealed fixture (power 41/256 - (33/32) t^2 family, section 8).
  const auto singleton = [](double x, double y, double z) {
    engine::EngineBox box{};
    const CertifiedPoint3 value = point(x, y, z);
    const std::array<std::uint64_t, 3> bits = value.canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower[axis] = bits[axis];
      box.upper[axis] = bits[axis];
    }
    return box;
  };
  const auto interval_x = [](double lower,
                             double upper,
                             double y,
                             double z) {
    engine::EngineBox box{};
    const std::array<std::uint64_t, 3> lower_bits =
        point(lower, y, z).canonical_input_bits();
    const std::array<std::uint64_t, 3> upper_bits =
        point(upper, y, z).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower[axis] = lower_bits[axis];
      box.upper[axis] = upper_bits[axis];
    }
    return box;
  };
  // Equilateral-height pair with a moving apex crossing the acute/obtuse
  // boundary: both corner triangles fail the positive gate while interior
  // apexes pass it, so no width may certify either universal verdict.
  engine::EngineBox well_boxes[4]{};
  well_boxes[0] = singleton(0.0, 0.0, 0.0);
  well_boxes[1] = singleton(1.0, 0.0, 0.0);
  well_boxes[2] = interval_x(-0.25, 1.25, 0.5, 0.0);
  const bool no_well_ambiguous =
      engine::engine_no_well_centered_256(well_boxes, 3U) ==
          engine::EngineGateVerdict::exact_false &&
      engine::engine_no_well_centered_512(well_boxes, 3U) ==
          engine::EngineGateVerdict::exact_false &&
      engine::engine_no_well_centered_1024(well_boxes, 3U) ==
          engine::EngineGateVerdict::exact_false;
  check(
      no_well_ambiguous,
      "the mixed acute/obtuse box product never certifies the universal "
      "gate in any width");

  // Query-power ambiguity: a query strictly inside the corner spheres and
  // strictly outside an interior sphere keeps the three-valued decision
  // inconclusive in every width.
  engine::EngineBox power_boxes[4]{};
  power_boxes[0] = singleton(-1.0, 0.0, 0.0);
  power_boxes[1] = singleton(1.0, 0.0, 0.0);
  power_boxes[2] = interval_x(-0.125, 0.125, 1.0, 0.0);
  const engine::EngineBox query = singleton(0.0, -0.9375, 0.0);
  const bool query_ambiguous =
      engine::engine_query_cell_256(power_boxes, 3U, query) ==
          engine::EngineQueryVerdict::inconclusive &&
      engine::engine_query_cell_512(power_boxes, 3U, query) ==
          engine::EngineQueryVerdict::inconclusive &&
      engine::engine_query_cell_1024(power_boxes, 3U, query) ==
          engine::EngineQueryVerdict::inconclusive;
  check(
      query_ambiguous,
      "the corner-agreeing query product stays inconclusive in every "
      "width");
  // Cross-check the width verdicts against the exact host primitives on
  // the same reconstructed products.
  std::array<ExactDyadicAabb3, 3> host_power{};
  for (std::size_t index = 0U; index < 3U; ++index) {
    host_power[index] = dyadic_box_from_engine(power_boxes[index]);
  }
  check(
      exact_higher_support_product_query_cell_decision(
          std::span<const ExactDyadicAabb3>{host_power.data(), 3U},
          dyadic_box_from_engine(query)) ==
          ExactHigherSupportProductQueryCellDecision::inconclusive,
      "the host primitive agrees the reconstructed query product is "
      "ambiguous");
}

}  // namespace

int main() {
  try {
    test_witness_boxes_match_certified_mirror();
    test_engine_matches_fake_bit_for_bit();
    test_engine_matches_fake_under_minimal_quantum();
    test_engine_rational_drain_matches_fake();
    test_corner_fixture_ambiguous_in_every_width();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " higher-support slot engine failures\n";
    return 1;
  }
  std::cout << "higher-support device tiled slot engine parity tests "
               "passed\n";
  return 0;
}
