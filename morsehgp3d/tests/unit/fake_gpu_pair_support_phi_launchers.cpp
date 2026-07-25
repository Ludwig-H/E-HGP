#include "fake_gpu_pair_support_phi_launchers.hpp"

#include "phase9_pair_support_phi_internal.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<FakePairSupportPhiCorruption> proposal_corruption{
    FakePairSupportPhiCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_last_node_count{0U};
std::atomic<std::size_t> proposal_last_query_count{0U};
std::atomic<std::size_t> proposal_last_record_capacity{0U};
std::atomic<std::uint64_t> proposal_last_returned_epoch{0U};
std::atomic<std::size_t> rank_launch_count{0U};
std::atomic<std::size_t> rank_last_product_count{0U};
std::atomic<std::size_t> rank_last_work_item_capacity{0U};
std::atomic<std::size_t> rank_last_receipt_capacity{0U};
std::atomic<std::uint64_t> rank_last_returned_epoch{0U};

}  // namespace

void configure_fake_gpu_pair_support_phi(
    FakePairSupportPhiCorruption corruption) noexcept {
  proposal_corruption.store(corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_pair_support_phi() noexcept {
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::none);
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_last_node_count.store(0U, std::memory_order_relaxed);
  proposal_last_query_count.store(0U, std::memory_order_relaxed);
  proposal_last_record_capacity.store(0U, std::memory_order_relaxed);
  proposal_last_returned_epoch.store(0U, std::memory_order_relaxed);
  rank_launch_count.store(0U, std::memory_order_relaxed);
  rank_last_product_count.store(0U, std::memory_order_relaxed);
  rank_last_work_item_capacity.store(0U, std::memory_order_relaxed);
  rank_last_receipt_capacity.store(0U, std::memory_order_relaxed);
  rank_last_returned_epoch.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_phi_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_phi_last_node_count() noexcept {
  return proposal_last_node_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_phi_last_query_count() noexcept {
  return proposal_last_query_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_phi_last_record_capacity() noexcept {
  return proposal_last_record_capacity.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_rank_launch_count() noexcept {
  return rank_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_rank_last_product_count() noexcept {
  return rank_last_product_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_pair_support_rank_last_work_item_capacity() noexcept {
  return rank_last_work_item_capacity.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_pair_support_rank_last_receipt_capacity() noexcept {
  return rank_last_receipt_capacity.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

struct FakeRankResources {
  bool snapshot_uploaded{false};
};

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value, const char* message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::invalid_argument(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] spatial::ExactDyadicAabb3 node_box(
    const PairSupportPhiNodeInputRecord& node) {
  return spatial::ExactDyadicAabb3{
      {node.lower_bits[0U], node.lower_bits[1U], node.lower_bits[2U]},
      {node.upper_bits[0U], node.upper_bits[1U], node.upper_bits[2U]}};
}

void validate_anchor_leaf(
    const PairSupportPhiNodeInputRecord& anchor,
    std::uint64_t expected_leaf_begin) {
  if (anchor.leaf_begin != expected_leaf_begin ||
      anchor.leaf_begin >= anchor.leaf_end ||
      anchor.leaf_end - anchor.leaf_begin != 1U ||
      anchor.left_child != pair_support_phi_sentinel ||
      anchor.right_child != pair_support_phi_sentinel) {
    throw std::runtime_error(
        "the fake Phase 9 rank-prune anchor is not its authenticated leaf");
  }
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (anchor.lower_bits[axis] != anchor.upper_bits[axis]) {
      throw std::runtime_error(
          "the fake Phase 9 rank-prune anchor leaf is not degenerate");
    }
    static_cast<void>(
        exact::ExactRational::from_binary64_bits(anchor.lower_bits[axis]));
  }
}

[[nodiscard]] exact::ExactRational exact_anchor_interval_lower(
    const PairSupportPhiNodeInputRecord& first_anchor,
    const PairSupportPhiNodeInputRecord& second_anchor,
    const PairSupportPhiNodeInputRecord& witness) {
  exact::ExactRational sum;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational x_lower =
        exact::ExactRational::from_binary64_bits(
            witness.lower_bits[axis]);
    const exact::ExactRational x_upper =
        exact::ExactRational::from_binary64_bits(
            witness.upper_bits[axis]);
    const exact::ExactRational u =
        exact::ExactRational::from_binary64_bits(
            first_anchor.lower_bits[axis]);
    const exact::ExactRational v =
        exact::ExactRational::from_binary64_bits(
            second_anchor.lower_bits[axis]);
    const std::array<exact::ExactRational, 4U> products{
        (x_lower - u) * (x_lower - v),
        (x_lower - u) * (x_upper - v),
        (x_upper - u) * (x_lower - v),
        (x_upper - u) * (x_upper - v)};
    const exact::ExactRational& axis_lower =
        *std::min_element(products.begin(), products.end());
    sum = sum + axis_lower;
  }
  return sum;
}

[[nodiscard]] PairSupportPhiDeviceRecord make_record(
    std::size_t query_index,
    const PairSupportPhiQueryInputRecord& query,
    std::span<const PairSupportPhiNodeInputRecord> nodes) {
  const std::size_t first = checked_size(
      query.first_support_node_index,
      "the fake Phase 9 first support index does not fit size_t");
  const std::size_t second = checked_size(
      query.second_support_node_index,
      "the fake Phase 9 second support index does not fit size_t");
  const std::size_t witness = checked_size(
      query.witness_node_index,
      "the fake Phase 9 witness index does not fit size_t");
  if (first >= nodes.size() || second >= nodes.size() ||
      witness >= nodes.size()) {
    throw std::invalid_argument(
        "the fake Phase 9 launcher received an unknown LBVH node");
  }

  const hierarchy::ExactDiametralPhiAabbMaximum maximum =
      hierarchy::exact_diametral_phi_aabb_maximum(
          node_box(nodes[first]),
          node_box(nodes[second]),
          node_box(nodes[witness]));
  const DirectedEnclosure enclosure =
      enclose_rational(maximum.maximum_phi);
  const bool finite_strict_upper =
      enclosure.status != DirectedEnclosureStatus::unsupported_range &&
      std::bit_cast<double>(enclosure.upper_bits) < 0.0;

  PairSupportPhiDeviceRecord record;
  record.query_index = static_cast<std::uint64_t>(query_index);
  record.first_support_node_index = query.first_support_node_index;
  record.second_support_node_index = query.second_support_node_index;
  record.witness_node_index = query.witness_node_index;
  record.upper_phi_bits =
      enclosure.status == DirectedEnclosureStatus::unsupported_range
          ? kPositiveInfinityBits
          : enclosure.upper_bits;
  record.proposal_code =
      finite_strict_upper ? pair_support_phi_strict_interior_code
                          : pair_support_phi_requires_descent_code;
  return record;
}

void inject_corruption(
    PairSupportPhiDeviceBatch& batch,
    test_support::FakePairSupportPhiCorruption corruption) {
  using test_support::FakePairSupportPhiCorruption;
  switch (corruption) {
    case FakePairSupportPhiCorruption::none:
    case FakePairSupportPhiCorruption::zero_epoch:
    case FakePairSupportPhiCorruption::stale_epoch_without_advance:
    case FakePairSupportPhiCorruption::rank_capacity_metadata:
    case FakePairSupportPhiCorruption::rank_zero_epoch:
    case FakePairSupportPhiCorruption::rank_stale_epoch_without_advance:
    case FakePairSupportPhiCorruption::rank_changed_receipt:
    case FakePairSupportPhiCorruption::rank_false_strict_receipt:
    case FakePairSupportPhiCorruption::rank_stale_tail:
    case FakePairSupportPhiCorruption::rank_epoch_count_over_budget:
    case FakePairSupportPhiCorruption::rank_simulated_async_failure:
      return;
    case FakePairSupportPhiCorruption::duplicate_transcript_query:
      if (batch.record_count < 2U) {
        throw std::logic_error(
            "the fake Phase 9 duplicate needs two records");
      }
      batch.records[1U].query_index = batch.records[0U].query_index;
      return;
    case FakePairSupportPhiCorruption::changed_transcript_query:
      if (batch.record_count == 0U) {
        throw std::logic_error(
            "the fake Phase 9 changed query needs one record");
      }
      ++batch.records[0U].witness_node_index;
      return;
    case FakePairSupportPhiCorruption::stale_tail:
      if (batch.record_count >= batch.records.size()) {
        throw std::logic_error(
            "the fake Phase 9 stale tail needs spare capacity");
      }
      batch.records[batch.record_count].query_index = 0U;
      return;
    case FakePairSupportPhiCorruption::false_strict_interior: {
      const auto found = std::find_if(
          batch.records.begin(),
          batch.records.begin() +
              static_cast<std::ptrdiff_t>(batch.record_count),
          [](const PairSupportPhiDeviceRecord& record) {
            return record.proposal_code ==
                   pair_support_phi_requires_descent_code;
          });
      if (found == batch.records.begin() +
                       static_cast<std::ptrdiff_t>(batch.record_count)) {
        throw std::logic_error(
            "the fake Phase 9 false strict proposal needs a descent");
      }
      found->proposal_code = pair_support_phi_strict_interior_code;
      found->upper_phi_bits = std::bit_cast<std::uint64_t>(-1.0);
      return;
    }
    case FakePairSupportPhiCorruption::simulated_async_failure:
      throw std::runtime_error(
          "simulated asynchronous Phase 9 pair-support phi failure");
  }
  throw std::logic_error("unknown fake Phase 9 corruption mode");
}

}  // namespace

PairSupportPhiDeviceBatch propose_pair_support_phi_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::span<const PairSupportPhiQueryInputRecord> queries,
    std::size_t maximum_query_count) {
  if (nodes.empty() || queries.empty() ||
      queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "the fake Phase 9 launcher received invalid extents");
  }
  std::shared_ptr<void>& opaque = context.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<FakeRankResources>();
  }
  static_cast<FakeRankResources*>(opaque.get())->snapshot_uploaded = true;

  const test_support::FakePairSupportPhiCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  test_support::proposal_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  test_support::proposal_last_node_count.store(
      nodes.size(), std::memory_order_relaxed);
  test_support::proposal_last_query_count.store(
      queries.size(), std::memory_order_relaxed);
  test_support::proposal_last_record_capacity.store(
      maximum_query_count, std::memory_order_relaxed);

  PairSupportPhiDeviceBatch batch;
  batch.records.resize(maximum_query_count);
  batch.record_count = queries.size();
  batch.kernel_launch_count = 1U;
  if (corruption ==
      test_support::FakePairSupportPhiCorruption::zero_epoch) {
    batch.buffer_epoch = 0U;
  } else if (
      corruption == test_support::FakePairSupportPhiCorruption::
                        stale_epoch_without_advance) {
    batch.buffer_epoch =
        test_support::proposal_last_returned_epoch.load(
            std::memory_order_relaxed);
  } else {
    batch.buffer_epoch = context.advance_epoch();
    test_support::proposal_last_returned_epoch.store(
        batch.buffer_epoch, std::memory_order_relaxed);
  }

  for (std::size_t index = 0U; index < queries.size(); ++index) {
    batch.records[index] = make_record(index, queries[index], nodes);
  }
  // A valid device transcript is a permutation, not an ordering authority.
  std::reverse(
      batch.records.begin(),
      batch.records.begin() +
          static_cast<std::ptrdiff_t>(batch.record_count));
  inject_corruption(batch, corruption);
  return batch;
}

PairSupportRankDeviceBatch propose_pair_support_rank_prunes_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::uint64_t root_node_index,
    std::span<const PairSupportRankProductInputRecord> products,
    std::size_t required_strict_interior_point_count,
    std::size_t maximum_product_count,
    std::size_t maximum_work_item_count,
    std::size_t maximum_terminal_count,
    std::size_t maximum_epoch_count) {
  if (nodes.empty() || products.empty() ||
      root_node_index >= nodes.size() ||
      required_strict_interior_point_count == 0U ||
      maximum_product_count < products.size() ||
      maximum_work_item_count < products.size() ||
      maximum_terminal_count == 0U || maximum_epoch_count == 0U) {
    throw std::invalid_argument(
        "the fake Phase 9 rank-prune launcher received invalid extents");
  }

  const test_support::FakePairSupportPhiCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  test_support::rank_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  test_support::rank_last_product_count.store(
      products.size(), std::memory_order_relaxed);
  test_support::rank_last_work_item_capacity.store(
      maximum_work_item_count, std::memory_order_relaxed);
  test_support::rank_last_receipt_capacity.store(
      maximum_terminal_count, std::memory_order_relaxed);
  if (corruption ==
      test_support::FakePairSupportPhiCorruption::
          rank_simulated_async_failure) {
    throw std::runtime_error(
        "simulated asynchronous Phase 9 rank-prune failure");
  }

  PairSupportRankDeviceBatch batch;
  batch.terminals.resize(maximum_terminal_count);
  batch.input_product_count = products.size();
  batch.product_capacity = maximum_product_count;
  batch.work_item_capacity = maximum_work_item_count;
  batch.terminal_capacity = maximum_terminal_count;
  std::shared_ptr<void>& opaque = context.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<FakeRankResources>();
  }
  auto& fake_resources = *static_cast<FakeRankResources*>(opaque.get());
  if (!fake_resources.snapshot_uploaded) {
    batch.snapshot_h2d_byte_count =
        nodes.size() * sizeof(PairSupportPhiNodeInputRecord);
    fake_resources.snapshot_uploaded = true;
  }
  batch.active_product_h2d_byte_count =
      products.size() * sizeof(PairSupportRankProductInputRecord);
  batch.initial_frontier_h2d_byte_count =
      products.size() * sizeof(PairSupportRankWorkItem);
  batch.physical_terminal_d2h_byte_count =
      maximum_terminal_count * sizeof(PairSupportRankDeviceTerminal);
  batch.device_frontier_double_buffer_byte_capacity =
      2U * maximum_work_item_count * sizeof(PairSupportRankWorkItem);
  batch.device_terminal_byte_capacity =
      maximum_terminal_count * sizeof(PairSupportRankDeviceTerminal);
  batch.device_scan_workspace_byte_capacity =
      maximum_work_item_count * sizeof(std::uint64_t);
  batch.device_fixed_workspace_byte_capacity =
      maximum_product_count *
          (sizeof(PairSupportRankProductInputRecord) +
           sizeof(std::uint64_t)) +
      maximum_work_item_count *
          (2U * sizeof(PairSupportRankWorkItem) +
           6U * sizeof(std::uint64_t)) +
      maximum_terminal_count * sizeof(PairSupportRankDeviceTerminal) +
      sizeof(std::uint64_t) +
      batch.device_scan_workspace_byte_capacity;
  batch.anchor_ball_culling_enabled = true;

  for (const PairSupportRankProductInputRecord& product : products) {
    const std::size_t first_index = checked_size(
        product.first_support_node_index,
        "a fake Phase 9 rank-prune first index does not fit size_t");
    const std::size_t second_index = checked_size(
        product.second_support_node_index,
        "a fake Phase 9 rank-prune second index does not fit size_t");
    const std::size_t first_anchor_index = checked_size(
        product.first_anchor_leaf_node_index,
        "a fake Phase 9 rank-prune first anchor index does not fit size_t");
    const std::size_t second_anchor_index = checked_size(
        product.second_anchor_leaf_node_index,
        "a fake Phase 9 rank-prune second anchor index does not fit size_t");
    if (first_index >= nodes.size() || second_index >= nodes.size() ||
        first_anchor_index >= nodes.size() ||
        second_anchor_index >= nodes.size()) {
      throw std::runtime_error(
          "the fake Phase 9 rank-prune product is malformed");
    }
    const PairSupportPhiNodeInputRecord& first = nodes[first_index];
    const PairSupportPhiNodeInputRecord& second = nodes[second_index];
    validate_anchor_leaf(nodes[first_anchor_index], first.leaf_begin);
    validate_anchor_leaf(nodes[second_anchor_index], second.leaf_begin);
  }

  std::vector<PairSupportRankWorkItem> current;
  current.reserve(maximum_work_item_count);
  for (std::size_t product_slot = 0U;
       product_slot < products.size();
       ++product_slot) {
    current.push_back(PairSupportRankWorkItem{
        static_cast<std::uint64_t>(product_slot), root_node_index});
  }
  std::vector<std::uint64_t> strict_point_counts(products.size(), 0U);

  struct FakeDecision {
    bool emit_children{false};
    bool emit_terminal{false};
    bool strict_terminal{false};
  };

  while (!current.empty() &&
         batch.traversal_epoch_count < maximum_epoch_count) {
    ++batch.traversal_epoch_count;
    ++batch.count_kernel_launch_count;
    batch.exclusive_scan_count += 2U;
    batch.visited_work_item_count += current.size();
    batch.peak_frontier_count =
        std::max(batch.peak_frontier_count, current.size());

    std::vector<FakeDecision> decisions(current.size());
    std::size_t next_count = 0U;
    std::size_t epoch_terminal_count = 0U;
    for (std::size_t item_index = 0U;
         item_index < current.size();
         ++item_index) {
      const PairSupportRankWorkItem item = current[item_index];
      const std::size_t product_slot = checked_size(
          item.product_slot,
          "a fake Phase 9 rank-prune product slot does not fit size_t");
      const std::size_t witness_index = checked_size(
          item.witness_node_index,
          "a fake Phase 9 rank-prune witness index does not fit size_t");
      if (product_slot >= products.size() || witness_index >= nodes.size()) {
        throw std::runtime_error(
            "the fake Phase 9 rank-prune frontier is malformed");
      }
      if (strict_point_counts[product_slot] >=
          required_strict_interior_point_count) {
        continue;
      }
      const PairSupportRankProductInputRecord& product =
          products[product_slot];
      const std::size_t first_index = checked_size(
          product.first_support_node_index,
          "a fake Phase 9 rank-prune first index does not fit size_t");
      const std::size_t second_index = checked_size(
          product.second_support_node_index,
          "a fake Phase 9 rank-prune second index does not fit size_t");
      const std::size_t first_anchor_index = checked_size(
          product.first_anchor_leaf_node_index,
          "a fake Phase 9 rank-prune first anchor index does not fit size_t");
      const std::size_t second_anchor_index = checked_size(
          product.second_anchor_leaf_node_index,
          "a fake Phase 9 rank-prune second anchor index does not fit size_t");
      if (first_index >= nodes.size() || second_index >= nodes.size() ||
          first_anchor_index >= nodes.size() ||
          second_anchor_index >= nodes.size()) {
        throw std::runtime_error(
            "the fake Phase 9 rank-prune product is malformed");
      }
      const PairSupportPhiNodeInputRecord& witness = nodes[witness_index];
      const PairSupportPhiNodeInputRecord& first = nodes[first_index];
      const PairSupportPhiNodeInputRecord& second = nodes[second_index];
      const PairSupportPhiNodeInputRecord& first_anchor =
          nodes[first_anchor_index];
      const PairSupportPhiNodeInputRecord& second_anchor =
          nodes[second_anchor_index];
      validate_anchor_leaf(first_anchor, first.leaf_begin);
      validate_anchor_leaf(second_anchor, second.leaf_begin);
      const bool contained_in_support =
          (first.leaf_begin <= witness.leaf_begin &&
           witness.leaf_end <= first.leaf_end) ||
          (second.leaf_begin <= witness.leaf_begin &&
           witness.leaf_end <= second.leaf_end);
      if (contained_in_support) {
        continue;
      }
      const bool overlaps =
          (witness.leaf_begin < first.leaf_end &&
           first.leaf_begin < witness.leaf_end) ||
          (witness.leaf_begin < second.leaf_end &&
           second.leaf_begin < witness.leaf_end);
      if (!overlaps) {
        const hierarchy::ExactDiametralPhiAabbMaximum maximum =
            hierarchy::exact_diametral_phi_aabb_maximum(
                node_box(first), node_box(second), node_box(witness));
        const DirectedEnclosure enclosure =
            enclose_rational(maximum.maximum_phi);
        const bool finite_strict_upper =
            enclosure.status !=
                DirectedEnclosureStatus::unsupported_range &&
            std::bit_cast<double>(enclosure.upper_bits) < 0.0;
        if (finite_strict_upper) {
          decisions[item_index].emit_terminal = true;
          decisions[item_index].strict_terminal = true;
          ++epoch_terminal_count;
          continue;
        }
        if (first_index != second_index &&
            exact_anchor_interval_lower(
                first_anchor, second_anchor, witness).sign() >= 0) {
          decisions[item_index].emit_terminal = true;
          ++epoch_terminal_count;
          continue;
        }
      }
      const bool leaf =
          witness.left_child == pair_support_phi_sentinel &&
          witness.right_child == pair_support_phi_sentinel;
      if (!leaf) {
        if (witness.left_child >= nodes.size() ||
            witness.right_child >= nodes.size() ||
            witness.left_child == witness.right_child) {
          throw std::runtime_error(
              "the fake Phase 9 rank-prune node topology is malformed");
        }
        decisions[item_index].emit_children = true;
        next_count += 2U;
      } else if (overlaps) {
        throw std::runtime_error(
            "the fake Phase 9 rank-prune leaf partially overlaps a support");
      } else {
        decisions[item_index].emit_terminal = true;
        ++epoch_terminal_count;
      }
    }

    if (next_count > maximum_work_item_count) {
      batch.capacity_stop =
          PairSupportRankCapacityStop::work_item_capacity;
      break;
    }
    if (epoch_terminal_count >
        maximum_terminal_count - batch.terminal_count) {
      batch.capacity_stop =
          PairSupportRankCapacityStop::receipt_capacity;
      break;
    }

    ++batch.emit_kernel_launch_count;
    std::vector<PairSupportRankWorkItem> next;
    next.reserve(next_count);
    for (std::size_t item_index = 0U;
         item_index < current.size();
         ++item_index) {
      const PairSupportRankWorkItem item = current[item_index];
      const FakeDecision decision = decisions[item_index];
      const std::size_t product_slot =
          static_cast<std::size_t>(item.product_slot);
      const PairSupportPhiNodeInputRecord& witness =
          nodes[static_cast<std::size_t>(item.witness_node_index)];
      if (decision.emit_terminal) {
        const std::size_t output_index = batch.terminal_count++;
        batch.terminals[output_index] =
            PairSupportRankDeviceTerminal{
                item.product_slot, item.witness_node_index};
        if (decision.strict_terminal) {
          strict_point_counts[product_slot] +=
              witness.leaf_end - witness.leaf_begin;
        }
      }
      if (decision.emit_children) {
        next.push_back(PairSupportRankWorkItem{
            item.product_slot, witness.left_child});
        next.push_back(PairSupportRankWorkItem{
            item.product_slot, witness.right_child});
      }
    }
    current = std::move(next);
  }
  batch.frontier_exhausted = current.empty();
  batch.traversal_metadata_d2h_byte_count =
      batch.traversal_epoch_count *
          (5U * sizeof(std::uint64_t)) +
      sizeof(std::uint64_t);

  if (corruption ==
      test_support::FakePairSupportPhiCorruption::rank_zero_epoch) {
    batch.buffer_epoch = 0U;
  } else if (
      corruption == test_support::FakePairSupportPhiCorruption::
                        rank_stale_epoch_without_advance) {
    batch.buffer_epoch =
        test_support::rank_last_returned_epoch.load(
            std::memory_order_relaxed);
  } else {
    batch.buffer_epoch = context.advance_epoch();
    test_support::rank_last_returned_epoch.store(
        batch.buffer_epoch, std::memory_order_relaxed);
  }

  switch (corruption) {
    case test_support::FakePairSupportPhiCorruption::
        rank_capacity_metadata:
      ++batch.work_item_capacity;
      break;
    case test_support::FakePairSupportPhiCorruption::
        rank_changed_receipt:
      if (batch.terminal_count == 0U) {
        throw std::logic_error(
            "the fake Phase 9 changed rank terminal needs one record");
      }
      batch.terminals[0U].witness_node_index = root_node_index;
      break;
    case test_support::FakePairSupportPhiCorruption::
        rank_false_strict_receipt:
      if (batch.terminal_count == 0U ||
          batch.terminal_count >= batch.terminals.size()) {
        throw std::logic_error(
            "the fake Phase 9 duplicate terminal needs spare capacity");
      }
      batch.terminals[batch.terminal_count] = batch.terminals[0U];
      ++batch.terminal_count;
      break;
    case test_support::FakePairSupportPhiCorruption::rank_stale_tail:
      if (batch.terminal_count >= batch.terminals.size()) {
        throw std::logic_error(
            "the fake Phase 9 stale rank tail needs spare capacity");
      }
      batch.terminals[batch.terminal_count].product_slot = 0U;
      break;
    case test_support::FakePairSupportPhiCorruption::
        rank_epoch_count_over_budget:
      batch.traversal_epoch_count = maximum_epoch_count + 1U;
      break;
    case test_support::FakePairSupportPhiCorruption::none:
    case test_support::FakePairSupportPhiCorruption::
        duplicate_transcript_query:
    case test_support::FakePairSupportPhiCorruption::
        changed_transcript_query:
    case test_support::FakePairSupportPhiCorruption::stale_tail:
    case test_support::FakePairSupportPhiCorruption::
        false_strict_interior:
    case test_support::FakePairSupportPhiCorruption::zero_epoch:
    case test_support::FakePairSupportPhiCorruption::
        stale_epoch_without_advance:
    case test_support::FakePairSupportPhiCorruption::
        simulated_async_failure:
    case test_support::FakePairSupportPhiCorruption::rank_zero_epoch:
    case test_support::FakePairSupportPhiCorruption::
        rank_stale_epoch_without_advance:
    case test_support::FakePairSupportPhiCorruption::
        rank_simulated_async_failure:
      break;
  }
  batch.active_terminal_d2h_byte_count =
      batch.terminal_count * sizeof(PairSupportRankDeviceTerminal);
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
