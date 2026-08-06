#include "morsehgp3d/gpu/higher_support_device_tiled_session_bridge.hpp"

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_closed_ball.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using exact::BigInt;
using hierarchy::ExactHigherSupportCheckpoint;
using hierarchy::ExactHigherSupportEvent;
using hierarchy::ExactHigherSupportExtraShellDiagnostic;
using hierarchy::ExactHigherSupportFrontierEntry;
using hierarchy::ExactHigherSupportNodeGroup;
using hierarchy::ExactHigherSupportStreamBudget;
using hierarchy::ExactHigherSupportStreamChunk;
using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::MortonLbvhIndex;
using spatial::MortonLeafRecord;
using spatial::PointId;
using PruneKind = detail::Phase15HigherSupportDeviceTiledPruneKind;
using PruneRecord = detail::Phase15HigherSupportDeviceTiledPruneRecord;
using ProbeReceipt = detail::Phase15HigherSupportDeviceTiledProbeReceipt;
using SlotControl = detail::Phase15HigherSupportDeviceTiledSlotControl;
using TerminalRecord =
    detail::Phase15HigherSupportDeviceTiledTerminalRecord;

[[noreturn]] void fail(const std::string& message) {
  throw std::logic_error(
      "Phase 15 higher-support session bridge: " + message);
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
    fail("a binomial multiplicity must be in [1, 4]");
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

// ---------------------------------------------------------------------------
// Independent host mirror of the certified Morton LBVH.  The node boxes are
// not part of the public index API, so the bridge rebuilds them from the
// public leaf records with the exact radix/median split and witness rules of
// the CPU builder, and fails closed unless the certified counters are
// reproduced.  Every entry authentication, canonical expansion and prune
// replay below is evaluated against this reconstruction, never against
// launcher state.
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

struct MirrorGeometry {
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
    fail("the mirror split requires at least two leaves");
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
    fail("the mirror radix split did not divide its range");
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
    fail("the mirror received an invalid leaf range");
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

[[nodiscard]] MirrorGeometry build_mirror_geometry(
    const spatial::MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  if (!index.validated_for(cloud) || cloud.size() == 0U) {
    fail("the mirror requires a Morton LBVH validated for its cloud");
  }
  MirrorGeometry geometry;
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
  const spatial::MortonLbvhBuildCounters& counters = index.build_counters();
  if (geometry.nodes.size() != counters.node_count ||
      geometry.root_index + 1U != geometry.nodes.size() ||
      geometry.maximum_depth != counters.maximum_depth) {
    fail("the mirror does not reproduce the certified build counters");
  }
  return geometry;
}

[[nodiscard]] ExactDyadicAabb3 mirror_box(
    const MirrorGeometry& geometry,
    std::size_t node_index) {
  const MirrorNode& node = geometry.nodes[node_index];
  ExactDyadicAabb3 box{};
  box.lower_binary64_bits = node.lower_bits;
  box.upper_binary64_bits = node.upper_bits;
  return box;
}

// Authenticates one canonical frontier entry against the mirror (arity,
// exact node receipts, sorted disjoint groups, multiplicity partition,
// canonical zero padding) and returns its exact BigInt support mass.
[[nodiscard]] BigInt authenticated_entry_mass(
    const MirrorGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) {
  const std::size_t support_size = entry.support_size;
  const std::size_t group_count = entry.group_count;
  if ((support_size != 3U && support_size != 4U) || group_count == 0U ||
      group_count > support_size || group_count > 4U) {
    fail("a frontier entry has an invalid arity");
  }
  BigInt mass{1};
  std::size_t multiplicity_sum = 0U;
  std::uint64_t previous_end = 0U;
  for (std::size_t index = 0U; index < group_count; ++index) {
    const ExactHigherSupportNodeGroup& group = entry.groups[index];
    const auto node_index = static_cast<std::size_t>(group.node_index);
    if (node_index >= geometry.nodes.size()) {
      fail("a frontier entry names an unknown mirror node");
    }
    const MirrorNode& node = geometry.nodes[node_index];
    const std::size_t multiplicity = group.multiplicity;
    if (group.leaf_begin != static_cast<std::uint64_t>(node.leaf_begin) ||
        group.leaf_end != static_cast<std::uint64_t>(node.leaf_end) ||
        multiplicity == 0U || multiplicity > 4U ||
        node.leaf_end - node.leaf_begin < multiplicity ||
        (index != 0U && group.leaf_begin < previous_end)) {
      fail("a frontier entry contradicts its mirror node receipt");
    }
    previous_end = group.leaf_end;
    multiplicity_sum += multiplicity;
    mass *= bigint_binomial(
        group.leaf_end - group.leaf_begin, multiplicity);
  }
  if (multiplicity_sum != support_size) {
    fail("a frontier entry does not partition its support size");
  }
  const ExactHigherSupportNodeGroup padding{};
  for (std::size_t index = group_count; index < entry.groups.size();
       ++index) {
    if (entry.groups[index] != padding) {
      fail("a frontier entry carries nonzero padding");
    }
  }
  return mass;
}

[[nodiscard]] bool entry_expandable(
    const MirrorGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) noexcept {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(entry.groups[index].node_index);
    if (node_index < geometry.nodes.size() &&
        !geometry.nodes[node_index].is_leaf()) {
      return true;
    }
  }
  return false;
}

// Canonical CPU split transposed for the bridge: split the non-leaf group
// with the largest Morton range, enumerate every feasible multiplicity
// distribution (L, a), (D, r-a), and canonicalize each child by sorting its
// groups by leaf_begin (node index as tie break).  The exact BigInt child
// coverage must re-add to the parent coverage: the expansion is a true
// partition, never a heuristic refinement.
[[nodiscard]] std::vector<ExactHigherSupportFrontierEntry>
expand_entry_canonically(
    const MirrorGeometry& geometry,
    const ExactHigherSupportFrontierEntry& entry) {
  std::size_t split_group_index = entry.group_count;
  std::size_t largest_range = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(entry.groups[index].node_index);
    const MirrorNode& current = geometry.nodes[node_index];
    const std::size_t range = current.leaf_end - current.leaf_begin;
    if (!current.is_leaf() && range > largest_range) {
      split_group_index = index;
      largest_range = range;
    }
  }
  if (split_group_index == entry.group_count) {
    fail("a nonterminal product has no splittable group");
  }
  const ExactHigherSupportNodeGroup& split_group =
      entry.groups[split_group_index];
  const MirrorNode& parent =
      geometry.nodes[static_cast<std::size_t>(split_group.node_index)];
  const MirrorNode& left = geometry.nodes[parent.left_child];
  const MirrorNode& right = geometry.nodes[parent.right_child];
  const std::size_t multiplicity = split_group.multiplicity;
  const std::size_t left_size = left.leaf_end - left.leaf_begin;
  const std::size_t right_size = right.leaf_end - right.leaf_begin;
  const std::size_t minimum_left =
      multiplicity > right_size ? multiplicity - right_size : 0U;
  const std::size_t maximum_left = std::min(multiplicity, left_size);
  if (minimum_left > maximum_left) {
    fail("a split has no feasible multiplicity distribution");
  }
  std::vector<ExactHigherSupportFrontierEntry> children;
  children.reserve(maximum_left - minimum_left + 1U);
  BigInt child_coverage{0};
  for (std::size_t left_multiplicity = minimum_left;
       left_multiplicity <= maximum_left;
       ++left_multiplicity) {
    std::vector<std::pair<std::size_t, std::size_t>> groups;
    groups.reserve(4U);
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      if (index != split_group_index) {
        groups.emplace_back(
            static_cast<std::size_t>(entry.groups[index].node_index),
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
          const MirrorNode& lhs_node = geometry.nodes[lhs.first];
          const MirrorNode& rhs_node = geometry.nodes[rhs.first];
          if (lhs_node.leaf_begin != rhs_node.leaf_begin) {
            return lhs_node.leaf_begin < rhs_node.leaf_begin;
          }
          return lhs.first < rhs.first;
        });
    if (groups.empty() || groups.size() > entry.support_size ||
        groups.size() > 4U) {
      fail("a canonical child has an invalid group count");
    }
    ExactHigherSupportFrontierEntry child{};
    child.support_size = entry.support_size;
    child.group_count = static_cast<std::uint8_t>(groups.size());
    for (std::size_t index = 0U; index < groups.size(); ++index) {
      const MirrorNode& node = geometry.nodes[groups[index].first];
      child.groups[index] = ExactHigherSupportNodeGroup{
          static_cast<std::uint64_t>(groups[index].first),
          static_cast<std::uint64_t>(node.leaf_begin),
          static_cast<std::uint64_t>(node.leaf_end),
          static_cast<std::uint8_t>(groups[index].second)};
    }
    child_coverage += authenticated_entry_mass(geometry, child);
    children.push_back(child);
  }
  if (child_coverage != authenticated_entry_mass(geometry, entry)) {
    fail("a canonical split did not partition its parent coverage");
  }
  return children;
}

// ---------------------------------------------------------------------------
// Device record drain and exact host replay.
// ---------------------------------------------------------------------------

struct DrainedPrune {
  PruneRecord record{};
  std::vector<ProbeReceipt> receipts;
};

struct DrainedTerminal {
  TerminalRecord record{};
  std::size_t slot_index{};
};

struct DrainedRecords {
  std::vector<DrainedPrune> prunes;
  std::vector<DrainedTerminal> terminals;
};

void drain_record_lease(
    const HigherSupportDeviceTiledRecordDrainLease& lease,
    DrainedRecords& out) {
  const auto views = detail::
      Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess::inspect(
          lease);
  if (!lease.ready() || !views.ready || views.prune_records == nullptr ||
      views.terminal_records == nullptr ||
      views.probe_receipts == nullptr || views.slot_controls == nullptr) {
    fail("a drain lease does not expose ready record views");
  }
  std::size_t prune_total = 0U;
  std::size_t terminal_total = 0U;
  std::size_t receipt_total = 0U;
  for (std::size_t slot = views.slot_begin; slot < views.slot_end; ++slot) {
    const SlotControl& control = views.slot_controls[slot];
    const auto prune_count =
        static_cast<std::size_t>(control.prune_record_count);
    const auto terminal_count =
        static_cast<std::size_t>(control.terminal_record_count);
    const auto receipt_count =
        static_cast<std::size_t>(control.probe_receipt_count);
    if (control.tile_epoch != lease.audit().tile_epoch ||
        control.chunk_sequence != lease.audit().chunk_sequence ||
        prune_count > views.prune_record_stride ||
        terminal_count > views.terminal_record_stride ||
        receipt_count > views.probe_receipt_stride) {
      fail("a drained slot control escapes its fixed segments");
    }
    for (std::size_t index = 0U; index < prune_count; ++index) {
      DrainedPrune drained;
      drained.record =
          views.prune_records[slot * views.prune_record_stride + index];
      const auto receipt_begin =
          static_cast<std::size_t>(drained.record.receipt_segment_begin);
      const auto record_receipts =
          static_cast<std::size_t>(drained.record.receipt_count);
      if (receipt_begin + record_receipts > receipt_count) {
        fail("a prune record addresses uncommitted receipts");
      }
      for (std::size_t receipt = 0U; receipt < record_receipts; ++receipt) {
        drained.receipts.push_back(
            views.probe_receipts
                [slot * views.probe_receipt_stride + receipt_begin +
                 receipt]);
      }
      out.prunes.push_back(std::move(drained));
    }
    for (std::size_t index = 0U; index < terminal_count; ++index) {
      DrainedTerminal drained;
      drained.record =
          views.terminal_records[slot * views.terminal_record_stride + index];
      drained.slot_index = slot;
      out.terminals.push_back(drained);
    }
    prune_total += prune_count;
    terminal_total += terminal_count;
    receipt_total += receipt_count;
  }
  if (prune_total != lease.audit().prune_record_count ||
      terminal_total != lease.audit().terminal_record_count ||
      receipt_total != lease.audit().probe_receipt_count) {
    fail("the drained record totals contradict the lease audit");
  }
}

// Rich host recomputation of one drained prune record: identity against the
// mirror, exact BigInt mass, universal Gram/Cramer certificate for a well
// prune, and the disjoint external strictly-interior receipt antichain for a
// rank prune under the sealed closed_rank_window policy.
[[nodiscard]] BigInt replay_prune_record(
    const MirrorGeometry& geometry,
    std::size_t maximum_relevant_closed_rank,
    const DrainedPrune& drained,
    bool& well_kind) {
  const PruneRecord& record = drained.record;
  const std::size_t support_size = record.support_size;
  const std::size_t group_count = record.group_count;
  if ((support_size != 3U && support_size != 4U) || group_count == 0U ||
      group_count > support_size ||
      record.terminal_policy !=
          static_cast<std::uint8_t>(
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  closed_rank_window) ||
      record.flags != 0U || record.reserved_zero != 0U) {
    fail("a drained prune record is not canonically formed");
  }
  std::array<ExactDyadicAabb3, 4> boxes{};
  std::size_t box_count = 0U;
  std::size_t multiplicity_sum = 0U;
  BigInt mass{1};
  std::uint64_t previous_end = 0U;
  for (std::size_t index = 0U; index < group_count; ++index) {
    const auto node_index =
        static_cast<std::size_t>(record.group_node_index[index]);
    const std::size_t multiplicity = record.group_multiplicity[index];
    if (node_index >= geometry.nodes.size()) {
      fail("a prune record names an unknown mirror node");
    }
    const MirrorNode& node = geometry.nodes[node_index];
    if (record.group_leaf_begin[index] !=
            static_cast<std::uint64_t>(node.leaf_begin) ||
        record.group_leaf_end[index] !=
            static_cast<std::uint64_t>(node.leaf_end) ||
        multiplicity == 0U || multiplicity > 4U ||
        node.leaf_end - node.leaf_begin < multiplicity ||
        (index != 0U && record.group_leaf_begin[index] < previous_end)) {
      fail("a prune record contradicts its mirror node receipt");
    }
    previous_end = record.group_leaf_end[index];
    mass *= bigint_binomial(
        record.group_leaf_end[index] - record.group_leaf_begin[index],
        multiplicity);
    for (std::size_t copy = 0U; copy < multiplicity; ++copy) {
      if (box_count >= support_size) {
        fail("a prune record overflows its support arity");
      }
      boxes[box_count] = mirror_box(geometry, node_index);
      ++box_count;
    }
    multiplicity_sum += multiplicity;
  }
  for (std::size_t index = group_count; index < 4U; ++index) {
    if (record.group_node_index[index] != 0U ||
        record.group_leaf_begin[index] != 0U ||
        record.group_leaf_end[index] != 0U ||
        record.group_multiplicity[index] != 0U) {
      fail("a prune record carries nonzero group padding");
    }
  }
  if (multiplicity_sum != support_size || box_count != support_size) {
    fail("a prune record does not partition its support size");
  }
  if (bigint_from_words(record.pruned_mass_lo, record.pruned_mass_hi) !=
      mass) {
    fail("a prune record contradicts its exact BigInt mass");
  }
  const std::span<const ExactDyadicAabb3> support_span{
      boxes.data(), support_size};
  if (record.prune_kind ==
      static_cast<std::uint64_t>(PruneKind::no_well_centered_support)) {
    if (record.receipt_count != 0U || !drained.receipts.empty()) {
      fail("a well prune carries strict-interior receipts");
    }
    const auto analysis =
        hierarchy::exact_higher_support_product_aabb_analysis(support_span);
    if (!analysis.no_well_centered_support_certified()) {
      fail("a well prune fails its universal Gram/Cramer replay");
    }
    well_kind = true;
    return mass;
  }
  if (record.prune_kind !=
      static_cast<std::uint64_t>(PruneKind::rank_window)) {
    fail("a prune kind is not sealed to the closed_rank_window policy");
  }
  well_kind = false;
  const std::size_t required =
      higher_support_device_tiled_frontier_required_witness_count(
          HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window,
          support_size,
          maximum_relevant_closed_rank);
  if (drained.receipts.size() !=
      static_cast<std::size_t>(record.receipt_count)) {
    fail("a rank prune loses committed receipts");
  }
  if (required == 0U) {
    if (record.receipt_count != 0U) {
      fail("an above-window prune carries witness receipts");
    }
    return mass;
  }
  if (record.receipt_count == 0U) {
    fail("a thresholded rank prune carries no receipt");
  }
  BigInt certified{0};
  std::vector<std::pair<std::uint64_t, std::uint64_t>> intervals;
  for (const ProbeReceipt& receipt : drained.receipts) {
    const auto node_index = static_cast<std::size_t>(receipt.node_index);
    if (node_index >= geometry.nodes.size()) {
      fail("a rank receipt names an unknown mirror node");
    }
    const MirrorNode& node = geometry.nodes[node_index];
    const std::uint64_t range = receipt.leaf_end > receipt.leaf_begin
        ? receipt.leaf_end - receipt.leaf_begin
        : 0U;
    bool external = true;
    for (std::size_t index = 0U; index < group_count; ++index) {
      if (receipt.leaf_begin < record.group_leaf_end[index] &&
          record.group_leaf_begin[index] < receipt.leaf_end) {
        external = false;
      }
    }
    if (receipt.leaf_begin != static_cast<std::uint64_t>(node.leaf_begin) ||
        receipt.leaf_end != static_cast<std::uint64_t>(node.leaf_end) ||
        range == 0U || receipt.certified_point_count != range ||
        range > static_cast<std::uint64_t>(required) || !external) {
      fail("a rank receipt is not an external bounded LBVH cell");
    }
    const hierarchy::ExactHigherSupportProductQueryCellDecision decision =
        hierarchy::exact_higher_support_product_query_cell_decision(
            support_span, mirror_box(geometry, node_index));
    if (decision !=
        hierarchy::ExactHigherSupportProductQueryCellDecision::
            strictly_inside_every_independent_sphere) {
      fail("a rank receipt fails its strict-interior replay");
    }
    certified += receipt.certified_point_count;
    intervals.emplace_back(receipt.leaf_begin, receipt.leaf_end);
  }
  std::sort(intervals.begin(), intervals.end());
  for (std::size_t index = 1U; index < intervals.size(); ++index) {
    if (intervals[index].first < intervals[index - 1U].second) {
      fail("rank receipts overlap: the antichain is not disjoint");
    }
  }
  if (certified < required) {
    fail("the replayed receipt masses miss the sealed policy threshold");
  }
  return mass;
}

struct SupportKey {
  std::uint8_t support_size{};
  std::array<PointId, 4> support_ids{};

  friend bool operator==(const SupportKey&, const SupportKey&) = default;
  friend bool operator<(const SupportKey& lhs, const SupportKey& rhs) {
    if (lhs.support_size != rhs.support_size) {
      return lhs.support_size < rhs.support_size;
    }
    return lhs.support_ids < rhs.support_ids;
  }
};

// Replays the exact geometry category of one drained terminal record from
// the immutable cloud and returns its support key.
[[nodiscard]] SupportKey replay_terminal_record(
    const CanonicalPointCloud& cloud,
    const DrainedTerminal& drained,
    bool& minimal) {
  const TerminalRecord& record = drained.record;
  const std::size_t support_size = record.support_size;
  bool canonical = (support_size == 3U || support_size == 4U) &&
      record.terminal_policy ==
          static_cast<std::uint8_t>(
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  closed_rank_window) &&
      record.flags == 0U &&
      record.slot_index == static_cast<std::uint64_t>(drained.slot_index) &&
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
  if (!canonical) {
    fail("a drained terminal record is not canonically formed");
  }
  std::array<ExactDyadicAabb3, 4> boxes{};
  for (std::size_t index = 0U; index < support_size; ++index) {
    const std::array<std::uint64_t, 3> bits =
        cloud.point(static_cast<PointId>(record.point_ids[index]))
            .canonical_input_bits();
    boxes[index].lower_binary64_bits = bits;
    boxes[index].upper_binary64_bits = bits;
  }
  const hierarchy::ExactHigherSupportTerminalGeometryDecision decision =
      hierarchy::exact_higher_support_terminal_geometry_decision(
          std::span<const ExactDyadicAabb3>{boxes.data(), support_size});
  if (static_cast<std::uint8_t>(decision) !=
      record.terminal_geometry_decision) {
    fail("a terminal record contradicts its exact geometry category");
  }
  minimal = decision ==
      hierarchy::ExactHigherSupportTerminalGeometryDecision::minimal;
  SupportKey key;
  key.support_size = record.support_size;
  for (std::size_t index = 0U; index < 4U; ++index) {
    key.support_ids[index] =
        static_cast<PointId>(record.point_ids[index]);
  }
  return key;
}

// Host closed-ball classification of one fixed minimal support: the exact
// consumer stage that stays on the host in v1
// (exact_higher_support_terminal_classification_native_cuda == false).
// The traversal is the one shared indexed query, so this stage costs the
// visited subtrees of the sphere and not a linear sweep of the cloud --
// the difference between O(output * n) and the product contract at 50k.
struct ClosedBallSummary {
  bool minimal{false};
  // False when the strict interior escapes the relevant closed rank, the
  // one condition under which the indexed query stops early and the
  // record is an above-rank leaf.
  bool within_relevant_rank{false};
  std::size_t shell_count{};
  std::size_t exterior_count{};
  std::vector<PointId> interior_ids;
  std::optional<PointId> canonical_extra_shell_witness_id;
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  hierarchy::ExactHigherSupportClosedBallCounters counters{};
  bool queried{false};
};

template <std::size_t SupportSize>
[[nodiscard]] ClosedBallSummary closed_ball_summary_sized(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids,
    std::size_t maximum_relevant_closed_rank) {
  std::array<exact::ExactRational3, SupportSize> support{};
  for (std::size_t index_in_support = 0U;
       index_in_support < SupportSize;
       ++index_in_support) {
    support[index_in_support] =
        cloud.point(support_ids[index_in_support]).exact();
  }
  const auto analysis = exact::analyze_circumcenter_support(support);
  ClosedBallSummary summary;
  if (analysis.status() != exact::CircumcenterSupportStatus::minimal) {
    return summary;
  }
  summary.minimal = true;
  const auto& sphere = analysis.circumcenter_result();
  if (!sphere.center().has_value() || !sphere.squared_level().has_value()) {
    fail("a minimal replayed support omitted its exact sphere");
  }
  summary.center = *sphere.center();
  summary.squared_level = *sphere.squared_level();
  // An intrinsically above-rank support carries no relevant closed ball,
  // exactly as the host generator's leaf preflight requires.
  if (SupportSize > maximum_relevant_closed_rank) {
    return summary;
  }
  hierarchy::ExactHigherSupportClosedBallClassification classification =
      hierarchy::ExactHigherSupportIndexedClosedBallQuery::classify(
          index,
          cloud,
          support_ids,
          SupportSize,
          summary.center,
          summary.squared_level,
          maximum_relevant_closed_rank - SupportSize,
          hierarchy::ExactHigherSupportIndexedClosedBallQuery::
              proved_traversal_frontier_bound(index));
  summary.queried = true;
  summary.counters = classification.counters;
  if (classification.outcome !=
      hierarchy::ExactHigherSupportClosedBallOutcome::complete) {
    return summary;
  }
  summary.within_relevant_rank = true;
  summary.interior_ids = std::move(classification.interior_ids);
  summary.shell_count = classification.shell_count;
  summary.exterior_count = classification.exterior_count;
  summary.canonical_extra_shell_witness_id =
      classification.canonical_extra_shell_witness_id;
  return summary;
}

[[nodiscard]] ClosedBallSummary closed_ball_summary(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const SupportKey& key,
    std::size_t maximum_relevant_closed_rank) {
  return key.support_size == 3U
      ? closed_ball_summary_sized<3U>(
            index, cloud, key.support_ids, maximum_relevant_closed_rank)
      : closed_ball_summary_sized<4U>(
            index, cloud, key.support_ids, maximum_relevant_closed_rank);
}

[[nodiscard]] std::size_t saturating_double(std::size_t value) noexcept {
  return value > std::numeric_limits<std::size_t>::max() / 2U
      ? std::numeric_limits<std::size_t>::max()
      : value * 2U;
}

// Only the work-unit capacity binds: every other axis is unlimited so the
// minimal-work-unit boundary theorem applies (the builder claims a new
// product only after consuming a work unit, so exhausting exactly the
// work-unit budget between products leaves no pending cursor and a clean
// successor frontier).
[[nodiscard]] ExactHigherSupportStreamBudget work_unit_probe_budget(
    std::size_t work_units) noexcept {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      work_units,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

// Length of the longest common frontier prefix between the source and the
// candidate successor checkpoint.
[[nodiscard]] std::size_t common_frontier_prefix_length(
    const ExactHigherSupportCheckpoint& source,
    const ExactHigherSupportStreamChunk& candidate) {
  const auto& next = candidate.next_checkpoint.frontier;
  const auto& cur = source.frontier;
  const std::size_t bound = std::min(next.size(), cur.size());
  std::size_t prefix = 0U;
  while (prefix < bound && next[prefix] == cur[prefix]) {
    ++prefix;
  }
  return prefix;
}

}  // namespace

namespace detail {

class Phase15HigherSupportDeviceTiledSessionBridgeState final {
 public:
  Phase15HigherSupportDeviceTiledSessionBridgeState(
      const hierarchy::ExactHigherSupportAuthorityContext& authority_context,
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      HigherSupportDeviceTiledSessionBridgeConfig bridge_config,
      const hierarchy::ExactSparseHigherSupportH0ChunkRunContext*
          wire_validation_context)
      : authority(&authority_context),
        config(bridge_config),
        wire_context(wire_validation_context),
        session(authority_context),
        geometry(build_mirror_geometry(
            authority_context.index(), authority_context.cloud())) {
    validate_configuration();
    source_checkpoint = anchored_checkpoint();
    install_frontier_context(std::move(traversal_lease));
    audit.point_count = authority->cloud().size();
    audit.maximum_relevant_closed_rank =
        authority->manifest().maximum_relevant_closed_rank;
    audit.strict_interior_carrier_rejected_at_construction = true;
    audit.tile_boundary_prefix_criterion_enforced = true;
    audit.sealed_largest_mass_pre_expansion_v1 = true;
    audit.induction_identity_reverified_each_commit = true;
    audit.no_commit_performed_on_censure = true;
    verify_induction_identity();
  }

  [[nodiscard]] HigherSupportDeviceTiledSessionBridgeAdvance
  advance_one_tile_transaction();
  [[nodiscard]] HigherSupportDeviceTiledSessionBridgeAdvance
  advance_one_tile_certified_transaction();
  void rebind(MortonLbvhDeviceTraversalLease&& traversal_lease);

  const hierarchy::ExactHigherSupportAuthorityContext* authority;
  HigherSupportDeviceTiledSessionBridgeConfig config;
  const hierarchy::ExactSparseHigherSupportH0ChunkRunContext* wire_context;
  hierarchy::ExactHigherSupportAnchoredSession session;
  // When borrowed, the external assembler owns the scientific session and
  // appropriates every verified transition; the owned member above stays
  // untouched at its initial checkpoint.
  hierarchy::ExactHigherSupportAnchoredStreamAssembler* external_assembler{
      nullptr};

  [[nodiscard]] const hierarchy::ExactHigherSupportCheckpoint&
  anchored_checkpoint() const noexcept {
    return external_assembler != nullptr
        ? external_assembler->trusted_checkpoint()
        : session.trusted_checkpoint();
  }
  [[nodiscard]] hierarchy::ExactHigherSupportStreamChunk anchored_prepare(
      const hierarchy::ExactHigherSupportStreamBudget& chunk_budget,
      const hierarchy::ExactHigherSupportCheckpoint& reinjected) const {
    return external_assembler != nullptr
        ? external_assembler->prepare_next(chunk_budget, reinjected)
        : session.prepare_next(chunk_budget, reinjected);
  }
  [[nodiscard]] hierarchy::ExactHigherSupportStreamChunkVerification
  anchored_commit(
      const hierarchy::ExactHigherSupportStreamBudget& chunk_budget,
      const hierarchy::ExactHigherSupportCheckpoint& reinjected,
      const hierarchy::ExactHigherSupportStreamChunk& candidate) {
    return external_assembler != nullptr
        ? external_assembler->commit_prepared(
              chunk_budget, reinjected, candidate)
        : session.commit_prepared(chunk_budget, reinjected, candidate);
  }
  MirrorGeometry geometry;
  std::optional<HigherSupportDeviceTiledFrontierContext> frontier;
  hierarchy::ExactHigherSupportCheckpoint source_checkpoint;
  HigherSupportDeviceTiledSessionBridgeAudit audit;
  // The frontier context reports context-lifetime cumulative masses; the
  // bridge validates per-tile deltas against these baselines, which reset
  // to zero whenever a fresh engine is bound.
  exact::BigInt device_well_baseline{0};
  exact::BigInt device_rank_baseline{0};
  exact::BigInt device_terminal_baseline{0};
  exact::BigInt device_resolved_baseline{0};
  std::size_t consecutive_expansion_transitions{};
  bool poisoned{false};
  bool rebind_required{false};

 private:
  void validate_configuration() const;
  void install_frontier_context(
      MortonLbvhDeviceTraversalLease&& traversal_lease);
  void verify_induction_identity();
  [[nodiscard]] std::vector<ExactHigherSupportFrontierEntry> pre_expand(
      std::vector<ExactHigherSupportFrontierEntry> entries);
  void cross_validate_tile(
      const std::vector<ExactHigherSupportFrontierEntry>& tile_roots,
      const std::vector<ExactHigherSupportFrontierEntry>& tile_entries,
      const ExactHigherSupportStreamChunk& candidate,
      const HigherSupportDeviceTiledFrontierAudit& device_audit,
      const DrainedRecords& records);
  void validate_committed_event_wire(const ExactHigherSupportEvent& event);
  [[nodiscard]] hierarchy::ExactHigherSupportTileCertifiedTile
  synthesize_tile_certified_payload(
      const std::vector<ExactHigherSupportFrontierEntry>& tile_roots,
      const HigherSupportDeviceTiledFrontierAudit& device_audit,
      const DrainedRecords& records);
};

void Phase15HigherSupportDeviceTiledSessionBridgeState::
    validate_configuration() const {
  if (config.frontier_config.terminal_policy !=
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window) {
    throw std::invalid_argument(
        "the higher-support session bridge admits only the "
        "closed_rank_window policy: the anchored session induction has no "
        "strict-interior carrier semantics");
  }
  if (config.initial_work_unit_probe == 0U) {
    throw std::invalid_argument(
        "the initial work-unit probe must be at least one");
  }
  if (config.maximum_work_unit_probe_doublings == 0U) {
    throw std::invalid_argument(
        "the work-unit probe doubling allowance must be at least one");
  }
  if (config.pre_expansion_target_entry_count == 0U ||
      config.pre_expansion_target_entry_count >
          config.frontier_config.slot_tile_capacity) {
    throw std::invalid_argument(
        "the pre-expansion target must be in [1, slot_tile_capacity]");
  }
  if (config.maximum_session_pre_expansion_transition_count == 0U) {
    throw std::invalid_argument(
        "the session pre-expansion backstop must be at least one");
  }
}

void Phase15HigherSupportDeviceTiledSessionBridgeState::
    install_frontier_context(
        MortonLbvhDeviceTraversalLease&& traversal_lease) {
  HigherSupportDeviceTiledFrontierConfig derived = config.frontier_config;
  derived.maximum_relevant_closed_rank =
      authority->manifest().maximum_relevant_closed_rank;
  derived.certified_maximum_lbvh_depth =
      authority->index().build_counters().maximum_depth;
  frontier.emplace(std::move(traversal_lease), derived);
  device_well_baseline = 0;
  device_rank_baseline = 0;
  device_terminal_baseline = 0;
  device_resolved_baseline = 0;
  rebind_required = false;
  audit.frontier_context_rebind_required = false;
}

// The provenance induction R_j + C(F_j) == C(n,3) + C(n,4) is recomputed in
// exact::BigInt from the anchored checkpoint after every commit, together
// with the checkpoint's own remaining-frontier accounting.
void Phase15HigherSupportDeviceTiledSessionBridgeState::
    verify_induction_identity() {
  const ExactHigherSupportCheckpoint& trusted =
      anchored_checkpoint();
  BigInt frontier_mass{0};
  for (const ExactHigherSupportFrontierEntry& entry : trusted.frontier) {
    frontier_mass += authenticated_entry_mass(geometry, entry);
  }
  const BigInt universe =
      hierarchy::exact_higher_support_candidate_universe_size(
          authority->cloud().size());
  if (trusted.cumulative_audit.resolved_support_count + frontier_mass !=
          universe ||
      trusted.cumulative_audit.remaining_frontier_support_count !=
          frontier_mass ||
      trusted.cumulative_audit.total_support_count != universe) {
    fail("the anchored induction identity R + C(F) == C(n,3)+C(n,4) "
         "does not hold");
  }
}

[[nodiscard]] std::vector<ExactHigherSupportFrontierEntry>
Phase15HigherSupportDeviceTiledSessionBridgeState::pre_expand(
    std::vector<ExactHigherSupportFrontierEntry> entries) {
  BigInt before{0};
  for (const ExactHigherSupportFrontierEntry& entry : entries) {
    before += authenticated_entry_mass(geometry, entry);
  }
  const std::size_t target = std::min(
      config.pre_expansion_target_entry_count,
      config.frontier_config.slot_tile_capacity);
  while (entries.size() < target &&
         entries.size() + 4U <= config.frontier_config.slot_tile_capacity) {
    // Sealed v1 rule: expand the entry of largest exact mass, ties broken
    // by the lowest current position.
    std::size_t chosen = entries.size();
    BigInt chosen_mass{0};
    for (std::size_t index = 0U; index < entries.size(); ++index) {
      if (!entry_expandable(geometry, entries[index])) {
        continue;
      }
      const BigInt mass = authenticated_entry_mass(geometry, entries[index]);
      if (chosen == entries.size() || mass > chosen_mass) {
        chosen = index;
        chosen_mass = mass;
      }
    }
    if (chosen == entries.size()) {
      break;
    }
    std::vector<ExactHigherSupportFrontierEntry> children =
        expand_entry_canonically(geometry, entries[chosen]);
    entries.erase(
        entries.begin() + static_cast<std::ptrdiff_t>(chosen));
    entries.insert(
        entries.begin() + static_cast<std::ptrdiff_t>(chosen),
        children.begin(),
        children.end());
    ++audit.device_pre_expansion_count;
  }
  BigInt after{0};
  for (const ExactHigherSupportFrontierEntry& entry : entries) {
    after += authenticated_entry_mass(geometry, entry);
  }
  if (after != before) {
    fail("pre-expansion changed the tile universe mass");
  }
  audit.pre_expansion_partition_bigint_verified = true;
  return entries;
}

void Phase15HigherSupportDeviceTiledSessionBridgeState::cross_validate_tile(
    const std::vector<ExactHigherSupportFrontierEntry>& tile_roots,
    const std::vector<ExactHigherSupportFrontierEntry>& tile_entries,
    const ExactHigherSupportStreamChunk& candidate,
    const HigherSupportDeviceTiledFrontierAudit& device_audit,
    const DrainedRecords& records) {
  static_cast<void>(tile_entries);
  // Exact universe identity: the device tile and the prepared session
  // transition resolve the same BigInt mass, and the device partition
  // closes with zero unresolved mass.
  BigInt roots_mass{0};
  for (const ExactHigherSupportFrontierEntry& root : tile_roots) {
    roots_mass += authenticated_entry_mass(geometry, root);
  }
  // The engine reports context-lifetime cumulative masses; this tile's
  // exact contribution is the delta against the committed baselines.
  const BigInt device_well =
      device_audit.cumulative_well_prune_support_mass -
      device_well_baseline;
  const BigInt device_rank =
      device_audit.cumulative_rank_prune_support_mass -
      device_rank_baseline;
  const BigInt device_terminal =
      device_audit.cumulative_terminal_support_mass -
      device_terminal_baseline;
  const BigInt device_resolved =
      device_audit.cumulative_resolved_support_mass -
      device_resolved_baseline;
  if (device_audit.tile_universe_support_mass != roots_mass ||
      device_audit.unresolved_support_mass != 0 ||
      device_resolved != roots_mass ||
      device_well + device_rank + device_terminal != roots_mass) {
    fail("the device tile does not close the tile universe partition");
  }
  const BigInt session_resolved_delta =
      candidate.cumulative_audit_after.resolved_support_count -
      candidate.cumulative_audit_before.resolved_support_count;
  if (session_resolved_delta != roots_mass) {
    fail("the session transition resolves a different mass than its tile");
  }
  const std::size_t maximum_relevant_closed_rank =
      authority->manifest().maximum_relevant_closed_rank;
  if (device_audit.transaction_stack_high_water >
      higher_support_device_tiled_frontier_proved_stack_bound(
          authority->index().build_counters().maximum_depth)) {
    fail("the device slot stack exceeds the proved 16*depth + 1 bound");
  }
  audit.device_and_session_masses_cross_validated = true;

  // Per-record soundness: every drained prune record replays richly and the
  // record masses re-add to the audited cumulative masses; every terminal
  // record replays its exact geometry category.
  BigInt well_sum{0};
  BigInt rank_sum{0};
  for (const DrainedPrune& drained : records.prunes) {
    bool well_kind = false;
    const BigInt mass = replay_prune_record(
        geometry, maximum_relevant_closed_rank, drained, well_kind);
    if (well_kind) {
      well_sum += mass;
    } else {
      rank_sum += mass;
    }
    ++audit.replayed_device_prune_record_count;
  }
  if (well_sum != device_well || rank_sum != device_rank ||
      BigInt{static_cast<std::uint64_t>(records.terminals.size())} !=
          device_terminal) {
    fail("the drained record masses do not re-add to the audited masses");
  }
  audit.per_record_prune_replay_enforced = true;

  std::set<SupportKey> minimal_terminals;
  for (const DrainedTerminal& drained : records.terminals) {
    bool minimal = false;
    const SupportKey key =
        replay_terminal_record(authority->cloud(), drained, minimal);
    ++audit.replayed_device_terminal_record_count;
    if (minimal && !minimal_terminals.insert(key).second) {
      fail("a minimal terminal support is emitted more than once");
    }
  }
  audit.terminal_geometry_replay_enforced = true;

  // Bidirectional equality between the session-classified records and the
  // host closed-ball classification of the drained minimal terminals.
  std::set<SupportKey> candidate_events;
  for (const ExactHigherSupportEvent& event : candidate.events) {
    candidate_events.insert(
        SupportKey{event.support_size, event.support_ids});
  }
  std::set<SupportKey> candidate_diagnostics;
  for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
       candidate.relevant_extra_shell_diagnostics) {
    candidate_diagnostics.insert(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  for (const SupportKey& key : candidate_events) {
    if (minimal_terminals.count(key) == 0U) {
      fail("a session event is missing from the minimal terminal records");
    }
  }
  for (const SupportKey& key : candidate_diagnostics) {
    if (minimal_terminals.count(key) == 0U) {
      fail("a session extra-shell diagnostic is missing from the minimal "
           "terminal records");
    }
  }
  for (const SupportKey& key : minimal_terminals) {
    const ClosedBallSummary summary = closed_ball_summary(
        authority->index(),
        authority->cloud(),
        key,
        maximum_relevant_closed_rank);
    ++audit.host_closed_ball_classification_count;
    if (!summary.minimal) {
      fail("a minimal terminal record fails its host closed-ball replay");
    }
    if (summary.within_relevant_rank) {
      if (summary.shell_count == key.support_size) {
        if (candidate_events.count(key) == 0U) {
          fail("an in-window exact-shell minimal terminal is not a "
               "session event");
        }
      } else if (candidate_diagnostics.count(key) == 0U) {
        fail("an in-window extra-shell minimal terminal is not a session "
             "diagnostic");
      }
    } else if (
        candidate_events.count(key) != 0U ||
        candidate_diagnostics.count(key) != 0U) {
      fail("an above-window minimal terminal leaked into the session "
           "records");
    }
  }
  audit.host_closed_ball_classification_performed = true;
  audit.bidirectional_event_equality_enforced = true;
  audit.bidirectional_extra_shell_equality_enforced = true;
}

hierarchy::ExactHigherSupportTileCertifiedTile
Phase15HigherSupportDeviceTiledSessionBridgeState::
    synthesize_tile_certified_payload(
        const std::vector<ExactHigherSupportFrontierEntry>& tile_roots,
        const HigherSupportDeviceTiledFrontierAudit& device_audit,
        const DrainedRecords& records) {
  // Exact universe identity of the tile: the device partition closes on
  // the same BigInt mass the trusted frontier roots carry.
  BigInt roots_mass{0};
  for (const ExactHigherSupportFrontierEntry& root : tile_roots) {
    roots_mass += authenticated_entry_mass(geometry, root);
  }
  const BigInt device_well =
      device_audit.cumulative_well_prune_support_mass - device_well_baseline;
  const BigInt device_rank =
      device_audit.cumulative_rank_prune_support_mass - device_rank_baseline;
  const BigInt device_terminal =
      device_audit.cumulative_terminal_support_mass -
      device_terminal_baseline;
  const BigInt device_resolved =
      device_audit.cumulative_resolved_support_mass -
      device_resolved_baseline;
  if (device_audit.tile_universe_support_mass != roots_mass ||
      device_audit.unresolved_support_mass != 0 ||
      device_resolved != roots_mass ||
      device_well + device_rank + device_terminal != roots_mass) {
    fail("the device tile does not close the tile universe partition");
  }
  if (device_audit.transaction_stack_high_water >
      higher_support_device_tiled_frontier_proved_stack_bound(
          authority->index().build_counters().maximum_depth)) {
    fail("the device slot stack exceeds the proved 16*depth + 1 bound");
  }
  audit.device_and_session_masses_cross_validated = true;

  const std::size_t maximum_relevant_closed_rank =
      authority->manifest().maximum_relevant_closed_rank;

  // Per-record soundness: rich prune replay and exact terminal geometry
  // replay, both re-adding to the audited device masses.
  BigInt well_sum{0};
  BigInt rank_sum{0};
  for (const DrainedPrune& drained : records.prunes) {
    bool well_kind = false;
    const BigInt mass = replay_prune_record(
        geometry, maximum_relevant_closed_rank, drained, well_kind);
    if (well_kind) {
      well_sum += mass;
    } else {
      rank_sum += mass;
    }
    ++audit.replayed_device_prune_record_count;
  }
  if (well_sum != device_well || rank_sum != device_rank ||
      BigInt{static_cast<std::uint64_t>(records.terminals.size())} !=
          device_terminal) {
    fail("the drained record masses do not re-add to the audited masses");
  }
  audit.per_record_prune_replay_enforced = true;

  hierarchy::ExactHigherSupportTileCertifiedTile tile;
  tile.consumed_root_count = tile_roots.size();
  tile.well_centering_pruned_support_mass = device_well;
  tile.rank_pruned_support_mass = device_rank;
  tile.leaf_classified_support_mass = device_terminal;

  std::set<SupportKey> minimal_terminals;
  for (const DrainedTerminal& drained : records.terminals) {
    bool minimal = false;
    const SupportKey key =
        replay_terminal_record(authority->cloud(), drained, minimal);
    ++audit.replayed_device_terminal_record_count;
    switch (static_cast<hierarchy::ExactHigherSupportTerminalGeometryDecision>(
        drained.record.terminal_geometry_decision)) {
      case hierarchy::ExactHigherSupportTerminalGeometryDecision::
          affinely_dependent:
        ++tile.affinely_dependent_leaf_count;
        break;
      case hierarchy::ExactHigherSupportTerminalGeometryDecision::
          boundary_reduced:
        ++tile.boundary_reduced_leaf_count;
        break;
      case hierarchy::ExactHigherSupportTerminalGeometryDecision::
          exterior_circumcenter:
        ++tile.exterior_circumcenter_leaf_count;
        break;
      case hierarchy::ExactHigherSupportTerminalGeometryDecision::minimal:
        break;
    }
    if (minimal && !minimal_terminals.insert(key).second) {
      fail("a minimal terminal support is emitted more than once");
    }
  }
  audit.terminal_geometry_replay_enforced = true;

  // Host closed-ball classification of every minimal terminal: the exact
  // consumer stage that produces the scientific records themselves.
  for (const SupportKey& key : minimal_terminals) {
    ClosedBallSummary summary = closed_ball_summary(
        authority->index(),
        authority->cloud(),
        key,
        maximum_relevant_closed_rank);
    ++audit.host_closed_ball_classification_count;
    if (summary.queried) {
      ++tile.closed_ball_query_count;
    }
    tile.point_classification_count +=
        summary.counters.point_classification_count;
    if (!summary.minimal) {
      fail("a minimal terminal record fails its host closed-ball replay");
    }
    if (!summary.within_relevant_rank) {
      ++tile.above_rank_leaf_count;
      continue;
    }
    const std::size_t observed_closed_rank =
        summary.interior_ids.size() + summary.shell_count;
    const std::size_t minimum_possible_closed_rank =
        summary.interior_ids.size() + key.support_size;
    if (minimum_possible_closed_rank > maximum_relevant_closed_rank) {
      fail("a complete closed-ball shell escaped its exact interior cap");
    }
    if (summary.shell_count == key.support_size) {
      ExactHigherSupportEvent event;
      event.support_size = key.support_size;
      event.support_ids = key.support_ids;
      event.center = summary.center;
      event.squared_level = summary.squared_level;
      event.interior_ids = std::move(summary.interior_ids);
      event.closed_rank = observed_closed_rank;
      event.exterior_count = summary.exterior_count;
      validate_committed_event_wire(event);
      tile.events.push_back(std::move(event));
    } else {
      if (!summary.canonical_extra_shell_witness_id.has_value()) {
        fail("a higher-support extra shell omitted its canonical witness");
      }
      ExactHigherSupportExtraShellDiagnostic diagnostic;
      diagnostic.support_size = key.support_size;
      diagnostic.support_ids = key.support_ids;
      diagnostic.center = summary.center;
      diagnostic.squared_level = summary.squared_level;
      diagnostic.interior_ids = std::move(summary.interior_ids);
      diagnostic.shell_count = summary.shell_count;
      diagnostic.canonical_extra_shell_witness_id =
          *summary.canonical_extra_shell_witness_id;
      diagnostic.minimum_possible_closed_rank = minimum_possible_closed_rank;
      diagnostic.observed_closed_rank = observed_closed_rank;
      diagnostic.exterior_count = summary.exterior_count;
      tile.diagnostics.push_back(std::move(diagnostic));
    }
  }
  audit.host_closed_ball_classification_performed = true;
  return tile;
}

void Phase15HigherSupportDeviceTiledSessionBridgeState::
    validate_committed_event_wire(const ExactHigherSupportEvent& event) {
  if (wire_context == nullptr) {
    return;
  }
  const std::size_t cap = event.support_size == 3U
      ? hierarchy::sparse_higher_support_triangle_exact_integer_byte_count
      : hierarchy::sparse_higher_support_tetrahedron_exact_integer_byte_count;
  const auto validate_value = [&](const exact::ExactRational& value) {
    const auto round_trip =
        wire_context->round_trip_exact_rational_wire_for_validation(
            event.support_size, value);
    if (round_trip.numerator_byte_count > cap ||
        round_trip.denominator_byte_count > cap) {
      fail("a committed event escapes the sealed exact wire byte caps");
    }
    ++audit.wire_round_trip_count;
    audit.maximum_wire_numerator_byte_count = std::max(
        audit.maximum_wire_numerator_byte_count,
        round_trip.numerator_byte_count);
    audit.maximum_wire_denominator_byte_count = std::max(
        audit.maximum_wire_denominator_byte_count,
        round_trip.denominator_byte_count);
  };
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    validate_value(event.center.coordinate(axis));
  }
  validate_value(event.squared_level.rational());
  audit.wire_caps_validated = true;
}

HigherSupportDeviceTiledSessionBridgeAdvance
Phase15HigherSupportDeviceTiledSessionBridgeState::
    advance_one_tile_certified_transaction() {
  if (poisoned) {
    throw std::runtime_error(
        "the higher-support session bridge is poisoned by a prior "
        "cross-validation failure");
  }
  HigherSupportDeviceTiledSessionBridgeAdvance advance;
  if (anchored_checkpoint().locally_complete()) {
    audit.session_terminal_reached = true;
    advance.status =
        HigherSupportDeviceTiledSessionBridgeStatus::session_terminal;
    advance.audit = audit;
    return advance;
  }
  if (external_assembler == nullptr) {
    throw std::runtime_error(
        "the tile-certified path requires a borrowed anchored stream "
        "assembler");
  }
  if (rebind_required || !frontier.has_value()) {
    throw std::runtime_error(
        "the higher-support session bridge requires a fresh frontier "
        "engine rebind before this transaction");
  }
  try {
    const std::vector<ExactHigherSupportFrontierEntry>& cur_frontier =
        source_checkpoint.frontier;
    if (cur_frontier.empty()) {
      fail("a non-terminal anchored checkpoint has an empty frontier");
    }

    // Canonical pre-expansion transition, synthesized directly: below the
    // pre-expansion target with a splittable back entry, the back entry is
    // replaced by its canonical children.  No record, no resolved mass;
    // the session's complete checkpoint verification recomputes the
    // frontier mass and so certifies the conservation.
    if (cur_frontier.size() < config.pre_expansion_target_entry_count &&
        entry_expandable(geometry, cur_frontier.back())) {
      if (consecutive_expansion_transitions >=
          config.maximum_session_pre_expansion_transition_count) {
        fail("the session pre-expansion backstop was exhausted");
      }
      const std::vector<ExactHigherSupportFrontierEntry> children =
          expand_entry_canonically(geometry, cur_frontier.back());
      hierarchy::ExactHigherSupportCheckpoint successor = source_checkpoint;
      successor.frontier.pop_back();
      for (std::size_t index = children.size(); index > 0U; --index) {
        successor.frontier.push_back(children[index - 1U]);
      }
      successor.next_chunk_sequence =
          source_checkpoint.next_chunk_sequence + 1U;
      successor.cumulative_audit.maximum_frontier_entry_count = std::max(
          successor.cumulative_audit.maximum_frontier_entry_count,
          successor.frontier.size());
      successor.cumulative_audit.support_product_expansion_count +=
          children.size();
      successor.cumulative_audit.generated_child_product_count +=
          children.size();
      successor.checkpoint_digest =
          hierarchy::compute_exact_higher_support_checkpoint_digest(
              successor);
      if (!external_assembler->commit_canonical_expansion(
              source_checkpoint, successor)) {
        fail("the anchored chain rejected a synthesized canonical "
             "expansion");
      }
      source_checkpoint = anchored_checkpoint();
      verify_induction_identity();
      ++audit.committed_transaction_count;
      ++audit.committed_expansion_transition_count;
      ++consecutive_expansion_transitions;
      audit.session_counted_canonical_pre_expansion = true;
      advance.status =
          HigherSupportDeviceTiledSessionBridgeStatus::transaction_committed;
      advance.expansion_transition = true;
      advance.audit = audit;
      return advance;
    }
    consecutive_expansion_transitions = 0U;

    // Tile transaction: the frontier's back suffix, bounded by the slot
    // tile capacity, is resolved wholly by the device.  No host generator
    // runs and no work-unit boundary is searched.
    const std::size_t root_count = std::min(
        cur_frontier.size(), config.frontier_config.slot_tile_capacity);
    std::vector<ExactHigherSupportFrontierEntry> tile_roots;
    for (std::size_t index = cur_frontier.size();
         index > cur_frontier.size() - root_count;
         --index) {
      tile_roots.push_back(cur_frontier[index - 1U]);
    }
    std::vector<ExactHigherSupportFrontierEntry> tile_entries =
        pre_expand(tile_roots);

    DrainedRecords records;
    std::optional<HigherSupportDeviceTiledFrontierAudit> device_audit;
    try {
      frontier->open_tile(
          std::span<const ExactHigherSupportFrontierEntry>{
              tile_entries.data(), tile_entries.size()});
      const std::size_t maximum_tile_advance_count =
          config.frontier_config.maximum_subdivision_count +
          tile_entries.size() *
              (config.frontier_config.slot_tile_capacity + 16U);
      std::size_t tile_advance_count = 0U;
      while (!device_audit.has_value()) {
        if (++tile_advance_count > maximum_tile_advance_count) {
          throw std::runtime_error(
              "the device tile did not converge within its advance "
              "backstop");
        }
        HigherSupportDeviceTiledFrontierAdvance tile_advance =
            frontier->advance();
        if (tile_advance.record_drain.has_value()) {
          drain_record_lease(*tile_advance.record_drain, records);
          tile_advance.record_drain.reset();
        }
        if (tile_advance.status ==
            HigherSupportDeviceTiledFrontierStatus::
                frontier_tile_complete) {
          device_audit.emplace(tile_advance.audit);
        } else if (
            tile_advance.status ==
            HigherSupportDeviceTiledFrontierStatus::censored) {
          throw std::runtime_error(
              "the device tile was censored before completion");
        }
      }
    } catch (...) {
      rebind_required = true;
      audit.frontier_context_rebind_required = true;
      ++audit.censored_without_commit_count;
      advance.status = HigherSupportDeviceTiledSessionBridgeStatus::
          censored_without_commit;
      advance.tile_root_count = tile_roots.size();
      advance.tile_slot_count = tile_entries.size();
      advance.audit = audit;
      return advance;
    }

    hierarchy::ExactHigherSupportTileCertifiedTile tile =
        synthesize_tile_certified_payload(
            tile_roots, *device_audit, records);
    const std::size_t committed_event_count = tile.events.size();
    const std::size_t committed_diagnostic_count = tile.diagnostics.size();
    const hierarchy::ExactHigherSupportStreamChunkVerification verification =
        external_assembler->commit_tile_certified(
            source_checkpoint, std::move(tile));
    if (!verification.chunk_transition_verified) {
      fail("the anchored chain rejected a tile-certified transition");
    }
    source_checkpoint = anchored_checkpoint();
    verify_induction_identity();

    ++audit.committed_transaction_count;
    ++audit.committed_tile_transaction_count;
    audit.committed_tile_root_count += tile_roots.size();
    audit.committed_tile_slot_count += tile_entries.size();
    audit.committed_event_count += committed_event_count;
    audit.committed_extra_shell_diagnostic_count +=
        committed_diagnostic_count;
    audit.committed_universe_support_mass +=
        device_audit->tile_universe_support_mass;
    audit.committed_device_well_prune_support_mass +=
        device_audit->cumulative_well_prune_support_mass -
        device_well_baseline;
    audit.committed_device_rank_prune_support_mass +=
        device_audit->cumulative_rank_prune_support_mass -
        device_rank_baseline;
    audit.committed_device_terminal_support_mass +=
        device_audit->cumulative_terminal_support_mass -
        device_terminal_baseline;
    audit.committed_session_resolved_support_mass +=
        device_audit->cumulative_resolved_support_mass -
        device_resolved_baseline;
    device_well_baseline =
        device_audit->cumulative_well_prune_support_mass;
    device_rank_baseline =
        device_audit->cumulative_rank_prune_support_mass;
    device_terminal_baseline =
        device_audit->cumulative_terminal_support_mass;
    device_resolved_baseline =
        device_audit->cumulative_resolved_support_mass;

    advance.status =
        HigherSupportDeviceTiledSessionBridgeStatus::transaction_committed;
    advance.tile_root_count = tile_roots.size();
    advance.tile_slot_count = tile_entries.size();
    advance.committed_events.clear();
    advance.committed_extra_shell_diagnostics.clear();
    advance.audit = audit;
    return advance;
  } catch (const std::exception&) {
    poisoned = true;
    throw;
  }
}

HigherSupportDeviceTiledSessionBridgeAdvance
Phase15HigherSupportDeviceTiledSessionBridgeState::
    advance_one_tile_transaction() {
  if (poisoned) {
    throw std::runtime_error(
        "the higher-support session bridge is poisoned by a prior "
        "cross-validation failure");
  }
  HigherSupportDeviceTiledSessionBridgeAdvance advance;
  if (anchored_checkpoint().locally_complete()) {
    audit.session_terminal_reached = true;
    advance.status =
        HigherSupportDeviceTiledSessionBridgeStatus::session_terminal;
    advance.audit = audit;
    return advance;
  }
  if (rebind_required || !frontier.has_value()) {
    throw std::runtime_error(
        "the higher-support session bridge requires a fresh frontier "
        "engine rebind before this transaction");
  }
  try {
    const std::vector<ExactHigherSupportFrontierEntry>& cur_frontier =
        source_checkpoint.frontier;
    if (cur_frontier.empty()) {
      fail("a non-terminal anchored checkpoint has an empty frontier");
    }
    // Transition intent.  Below the pre-expansion target with a splittable
    // back entry, the transition is one session-counted canonical
    // expansion; otherwise it fully resolves the back tile roots (the
    // sealed v1 entry-count rule).
    const bool expansion_intent =
        cur_frontier.size() < config.pre_expansion_target_entry_count &&
        entry_expandable(geometry, cur_frontier.back());
    if (expansion_intent &&
        consecutive_expansion_transitions >=
            config.maximum_session_pre_expansion_transition_count) {
      fail("the session pre-expansion backstop was exhausted");
    }

    // Boundary search over the work-unit budget.  The builder claims a new
    // product only after consuming a work unit, so a budget that runs out
    // exactly between products yields a candidate with no pending cursor
    // and a successor frontier that is an exact strict prefix of the
    // source (every removed suffix root fully resolved).  The expansion
    // predicate (the frontier changed at all) is monotone, so its minimal
    // satisfying budget is exactly the completion of the back entry's
    // first canonical step.  The tile predicate (clean strict prefix) is
    // not monotone beyond its first flip, but the deterministic doubling
    // plus binary search always converges to one of its flip points, and
    // every flip point is a valid whole-root tile boundary; the resolved
    // suffix length at that point defines the tile.
    const auto satisfied =
        [&](const ExactHigherSupportStreamChunk& candidate) {
          if (expansion_intent) {
            return candidate.next_checkpoint.frontier != cur_frontier;
          }
          const auto& next = candidate.next_checkpoint.frontier;
          return !candidate.next_checkpoint.pending_product.has_value() &&
              next.size() < cur_frontier.size() &&
              common_frontier_prefix_length(source_checkpoint, candidate) ==
              next.size();
        };
    const auto probe = [&](std::size_t work_units) {
      ++audit.work_unit_probe_count;
      return anchored_prepare(
          work_unit_probe_budget(work_units), source_checkpoint);
    };
    std::size_t lower = 0U;
    std::size_t upper = config.initial_work_unit_probe;
    std::optional<ExactHigherSupportStreamChunk> upper_candidate;
    for (std::size_t doubling = 0U;
         doubling <= config.maximum_work_unit_probe_doublings;
         ++doubling) {
      ExactHigherSupportStreamChunk prepared = probe(upper);
      if (satisfied(prepared)) {
        upper_candidate.emplace(std::move(prepared));
        break;
      }
      lower = upper;
      if (upper == std::numeric_limits<std::size_t>::max()) {
        break;
      }
      upper = saturating_double(upper);
    }
    if (!upper_candidate.has_value()) {
      fail("no admissible work-unit budget reaches the transition "
           "boundary");
    }
    while (lower + 1U < upper) {
      const std::size_t middle = lower + (upper - lower) / 2U;
      ExactHigherSupportStreamChunk prepared = probe(middle);
      if (satisfied(prepared)) {
        upper = middle;
        upper_candidate.emplace(std::move(prepared));
      } else {
        lower = middle;
      }
    }
    const std::size_t minimal_work_units = upper;
    const ExactHigherSupportStreamBudget budget =
        work_unit_probe_budget(minimal_work_units);
    std::optional<ExactHigherSupportStreamChunk> candidate =
        std::move(upper_candidate);
    audit.minimal_work_unit_boundary_theorem_applied = true;
    advance.minimal_work_unit_budget = minimal_work_units;

    // Classify the minimal candidate fail-closed: either one canonical
    // expansion of the back entry or a clean whole-root tile.
    if (candidate->next_checkpoint.pending_product.has_value()) {
      fail("the minimal boundary candidate retains a pending product: "
           "the boundary theorem was violated");
    }
    const std::size_t prefix_size =
        common_frontier_prefix_length(source_checkpoint, *candidate);
    const std::size_t removed_count = cur_frontier.size() - prefix_size;
    const std::size_t added_count =
        candidate->next_checkpoint.frontier.size() - prefix_size;
    const BigInt session_resolved_delta_probe =
        candidate->cumulative_audit_after.resolved_support_count -
        candidate->cumulative_audit_before.resolved_support_count;

    if (added_count != 0U) {
      // Session-counted canonical pre-expansion: the back entry is
      // replaced by exactly its canonical children, nothing is resolved
      // and no record is emitted.
      if (!expansion_intent || removed_count != 1U ||
          session_resolved_delta_probe != 0 ||
          !candidate->events.empty() ||
          !candidate->relevant_extra_shell_diagnostics.empty() ||
          !candidate->prune_certificates.empty()) {
        fail("a mixed expansion transition escapes both sealed "
             "transition shapes");
      }
      const std::vector<ExactHigherSupportFrontierEntry> children =
          expand_entry_canonically(geometry, cur_frontier.back());
      if (added_count != children.size()) {
        fail("a session expansion disagrees with the canonical split "
             "arity");
      }
      // The builder pushes the canonical children reversed so the frontier
      // back is the first enumerated child; the stored suffix therefore
      // reads in reverse enumeration order.
      for (std::size_t index = 0U; index < children.size(); ++index) {
        if (candidate->next_checkpoint.frontier[prefix_size + index] !=
            children[children.size() - 1U - index]) {
          fail("a session expansion disagrees with the canonical split "
               "children");
        }
      }
      const hierarchy::ExactHigherSupportStreamChunkVerification
          verification = anchored_commit(
              budget, source_checkpoint, *candidate);
      if (!verification.chunk_transition_verified) {
        fail("the anchored session rejected a canonical expansion "
             "transition");
      }
      source_checkpoint = anchored_checkpoint();
      verify_induction_identity();
      ++audit.committed_transaction_count;
      ++audit.committed_expansion_transition_count;
      ++consecutive_expansion_transitions;
      audit.session_counted_canonical_pre_expansion = true;
      advance.status =
          HigherSupportDeviceTiledSessionBridgeStatus::transaction_committed;
      advance.expansion_transition = true;
      advance.audit = audit;
      return advance;
    }

    if (removed_count == 0U ||
        (expansion_intent && removed_count != 1U)) {
      fail("the boundary candidate resolves an unexpected root count");
    }
    consecutive_expansion_transitions = 0U;

    // The tile is the resolved frontier suffix, ordered back first: the
    // exact resolution order of the anchored stream.
    std::vector<ExactHigherSupportFrontierEntry> tile_roots;
    for (std::size_t index = cur_frontier.size(); index > prefix_size;
         --index) {
      tile_roots.push_back(cur_frontier[index - 1U]);
    }
    std::vector<ExactHigherSupportFrontierEntry> tile_entries =
        pre_expand(tile_roots);

    // Device engine phase.  A censored tile or a fail-closed engine
    // rejection never commits: the anchored checkpoint Q_j is retained and
    // the engine must be rebound before an idempotent retry that
    // reproduces the identical prepared candidate.  Only cross-validation
    // and commit failures below poison the bridge itself.
    DrainedRecords records;
    std::optional<HigherSupportDeviceTiledFrontierAudit> device_audit;
    try {
      frontier->open_tile(
          std::span<const ExactHigherSupportFrontierEntry>{
              tile_entries.data(), tile_entries.size()});
      // Every advance makes progress (a resumable capacity yield drains at
      // least one full segment), so this bound is a fail-closed backstop
      // against a broken launcher, never a scientific budget.
      const std::size_t maximum_tile_advance_count =
          config.frontier_config.maximum_subdivision_count +
          tile_entries.size() *
              (config.frontier_config.slot_tile_capacity + 16U);
      std::size_t tile_advance_count = 0U;
      while (!device_audit.has_value()) {
        if (++tile_advance_count > maximum_tile_advance_count) {
          throw std::runtime_error(
              "the device tile did not converge within its advance "
              "backstop");
        }
        HigherSupportDeviceTiledFrontierAdvance tile_advance =
            frontier->advance();
        if (tile_advance.record_drain.has_value()) {
          drain_record_lease(*tile_advance.record_drain, records);
          tile_advance.record_drain.reset();
        }
        if (tile_advance.status ==
            HigherSupportDeviceTiledFrontierStatus::
                frontier_tile_complete) {
          device_audit.emplace(tile_advance.audit);
        } else if (
            tile_advance.status ==
            HigherSupportDeviceTiledFrontierStatus::censored) {
          throw std::runtime_error(
              "the device tile was censored before completion");
        }
      }
    } catch (...) {
      rebind_required = true;
      audit.frontier_context_rebind_required = true;
      ++audit.censored_without_commit_count;
      advance.status = HigherSupportDeviceTiledSessionBridgeStatus::
          censored_without_commit;
      advance.tile_root_count = tile_roots.size();
      advance.tile_slot_count = tile_entries.size();
      advance.audit = audit;
      return advance;
    }

    cross_validate_tile(
        tile_roots, tile_entries, *candidate, *device_audit, records);

    const hierarchy::ExactHigherSupportStreamChunkVerification
        verification = anchored_commit(
            budget, source_checkpoint, *candidate);
    if (!verification.chunk_transition_verified) {
      fail("the anchored session rejected a cross-validated tile "
           "transition");
    }
    source_checkpoint = anchored_checkpoint();
    verify_induction_identity();

    ++audit.committed_transaction_count;
    ++audit.committed_tile_transaction_count;
    audit.committed_tile_root_count += tile_roots.size();
    audit.committed_tile_slot_count += tile_entries.size();
    audit.committed_event_count += candidate->events.size();
    audit.committed_extra_shell_diagnostic_count +=
        candidate->relevant_extra_shell_diagnostics.size();
    audit.committed_prune_certificate_count +=
        candidate->prune_certificates.size();
    audit.committed_universe_support_mass +=
        device_audit->tile_universe_support_mass;
    audit.committed_device_well_prune_support_mass +=
        device_audit->cumulative_well_prune_support_mass -
        device_well_baseline;
    audit.committed_device_rank_prune_support_mass +=
        device_audit->cumulative_rank_prune_support_mass -
        device_rank_baseline;
    audit.committed_device_terminal_support_mass +=
        device_audit->cumulative_terminal_support_mass -
        device_terminal_baseline;
    audit.committed_session_resolved_support_mass +=
        device_audit->cumulative_resolved_support_mass -
        device_resolved_baseline;
    device_well_baseline =
        device_audit->cumulative_well_prune_support_mass;
    device_rank_baseline =
        device_audit->cumulative_rank_prune_support_mass;
    device_terminal_baseline =
        device_audit->cumulative_terminal_support_mass;
    device_resolved_baseline =
        device_audit->cumulative_resolved_support_mass;
    if (anchored_checkpoint().locally_complete()) {
      audit.session_terminal_reached = true;
    }

    for (const ExactHigherSupportEvent& event : candidate->events) {
      validate_committed_event_wire(event);
    }

    advance.status =
        HigherSupportDeviceTiledSessionBridgeStatus::transaction_committed;
    advance.committed_events = std::move(candidate->events);
    advance.committed_extra_shell_diagnostics =
        std::move(candidate->relevant_extra_shell_diagnostics);
    advance.committed_prune_certificate_count =
        candidate->prune_certificates.size();
    advance.tile_root_count = tile_roots.size();
    advance.tile_slot_count = tile_entries.size();
    advance.audit = audit;
    return advance;
  } catch (...) {
    poisoned = true;
    throw;
  }
}

void Phase15HigherSupportDeviceTiledSessionBridgeState::rebind(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  if (poisoned) {
    throw std::runtime_error(
        "a poisoned higher-support session bridge cannot rebind its "
        "frontier engine");
  }
  frontier.reset();
  install_frontier_context(std::move(traversal_lease));
}

}  // namespace detail

HigherSupportDeviceTiledSessionBridge::HigherSupportDeviceTiledSessionBridge(
    const hierarchy::ExactHigherSupportAuthorityContext& authority,
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    HigherSupportDeviceTiledSessionBridgeConfig config,
    const hierarchy::ExactSparseHigherSupportH0ChunkRunContext*
        wire_validation_context)
    : state_(
          std::make_unique<
              detail::Phase15HigherSupportDeviceTiledSessionBridgeState>(
              authority,
              std::move(traversal_lease),
              config,
              wire_validation_context)) {}

HigherSupportDeviceTiledSessionBridge::HigherSupportDeviceTiledSessionBridge(
    const hierarchy::ExactHigherSupportAuthorityContext& authority,
    hierarchy::ExactHigherSupportAnchoredStreamAssembler& stream_assembler,
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    HigherSupportDeviceTiledSessionBridgeConfig config,
    const hierarchy::ExactSparseHigherSupportH0ChunkRunContext*
        wire_validation_context)
    : state_(
          std::make_unique<
              detail::Phase15HigherSupportDeviceTiledSessionBridgeState>(
              authority,
              std::move(traversal_lease),
              config,
              wire_validation_context)) {
  // The borrowed assembler owns the scientific session; every anchored
  // read, prepare and verified commit dispatches to it, and the state's
  // own session stays untouched at its initial checkpoint.  The source
  // checkpoint captured during construction is re-anchored accordingly.
  state_->external_assembler = &stream_assembler;
  state_->source_checkpoint = state_->anchored_checkpoint();
}

HigherSupportDeviceTiledSessionBridge::
    ~HigherSupportDeviceTiledSessionBridge() noexcept = default;

HigherSupportDeviceTiledSessionBridgeAdvance
HigherSupportDeviceTiledSessionBridge::advance_one_tile_transaction() {
  return state_->config.tile_certified_commit
      ? state_->advance_one_tile_certified_transaction()
      : state_->advance_one_tile_transaction();
}

void HigherSupportDeviceTiledSessionBridge::rebind_frontier_context(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  state_->rebind(std::move(traversal_lease));
}

const hierarchy::ExactHigherSupportCheckpoint&
HigherSupportDeviceTiledSessionBridge::trusted_checkpoint() const noexcept {
  return state_->anchored_checkpoint();
}

bool HigherSupportDeviceTiledSessionBridge::session_terminal() const {
  return state_->anchored_checkpoint().locally_complete();
}

bool HigherSupportDeviceTiledSessionBridge::poisoned() const noexcept {
  return state_->poisoned;
}

bool HigherSupportDeviceTiledSessionBridge::frontier_context_rebind_required()
    const noexcept {
  return state_->rebind_required;
}

const HigherSupportDeviceTiledSessionBridgeAudit&
HigherSupportDeviceTiledSessionBridge::audit() const noexcept {
  return state_->audit;
}

const HigherSupportDeviceTiledSessionBridgeConfig&
HigherSupportDeviceTiledSessionBridge::config() const noexcept {
  return state_->config;
}

}  // namespace morsehgp3d::gpu
