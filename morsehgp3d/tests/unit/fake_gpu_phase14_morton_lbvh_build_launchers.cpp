#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "phase14_morton_lbvh_build_internal.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<FakePhase14MortonLbvhBuildCorruption> fake_corruption{
    FakePhase14MortonLbvhBuildCorruption::none};
std::atomic<std::size_t> fake_forced_ambiguous_axis_index{
    std::numeric_limits<std::size_t>::max()};
std::atomic<std::size_t> fake_proposal_count{0U};
std::atomic<std::size_t> fake_snapshot_count{0U};
std::atomic<std::size_t> fake_last_point_count{0U};
std::atomic<std::size_t> fake_last_point_capacity{0U};

}  // namespace

void configure_fake_gpu_phase14_morton_lbvh_build(
    FakePhase14MortonLbvhBuildConfiguration configuration) noexcept {
  fake_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
  fake_forced_ambiguous_axis_index.store(
      configuration.forced_ambiguous_axis_index,
      std::memory_order_relaxed);
}

void reset_fake_gpu_phase14_morton_lbvh_build() noexcept {
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{});
  fake_proposal_count.store(0U, std::memory_order_relaxed);
  fake_snapshot_count.store(0U, std::memory_order_relaxed);
  fake_last_point_count.store(0U, std::memory_order_relaxed);
  fake_last_point_capacity.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_phase14_morton_bin_proposal_count() noexcept {
  return fake_proposal_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_morton_lbvh_snapshot_count() noexcept {
  return fake_snapshot_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_morton_lbvh_last_point_count() noexcept {
  return fake_last_point_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_morton_lbvh_last_point_capacity() noexcept {
  return fake_last_point_capacity.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

using Corruption =
    test_support::FakePhase14MortonLbvhBuildCorruption;

constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kDeviceRangeByteSize =
    3U * sizeof(std::uint64_t);
constexpr std::size_t kDeviceControlByteSize =
    4U * sizeof(std::uint64_t);
constexpr std::uint64_t kMortonGridSize =
    std::uint64_t{1}
    << spatial::morton_lbvh_snapshot_morton_bits_per_axis;
constexpr std::uint64_t kMaximumMortonCoordinate =
    kMortonGridSize - UINT64_C(1);

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

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

void require_common_inputs(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::size_t maximum_point_count) {
  if (point_count == 0U || maximum_point_count == 0U ||
      point_count > maximum_point_count ||
      coordinate_bits.size() !=
          checked_product(
              point_count,
              kAxisCount,
              "the fake Phase 14 coordinate extent overflows")) {
    throw std::invalid_argument(
        "the fake Phase 14 Morton launcher received invalid extents");
  }
}

[[nodiscard]] exact::ExactRational exact_word(std::uint64_t bits) {
  return exact::ExactRational::from_binary64_bits(bits);
}

[[nodiscard]] std::uint64_t exact_bin(
    std::uint64_t coordinate_bits,
    std::uint64_t lower_bits,
    std::uint64_t upper_bits) {
  const exact::ExactRational coordinate = exact_word(coordinate_bits);
  const exact::ExactRational lower = exact_word(lower_bits);
  const exact::ExactRational upper = exact_word(upper_bits);
  if (lower == upper) {
    return 0U;
  }
  if (coordinate < lower || coordinate > upper) {
    throw std::logic_error(
        "a fake Phase 14 coordinate lies outside its global AABB");
  }
  const exact::ExactRational ratio =
      (coordinate - lower) / (upper - lower);
  const exact::BigInt scaled =
      ratio.numerator() * kMortonGridSize;
  const exact::BigInt quotient = scaled / ratio.denominator();
  if (quotient < 0 || quotient > kMortonGridSize) {
    throw std::logic_error(
        "a fake Phase 14 exact Morton bin lies outside its grid");
  }
  if (quotient == kMortonGridSize) {
    return kMaximumMortonCoordinate;
  }
  return quotient.convert_to<std::uint64_t>();
}

[[nodiscard]] bool leaf_less(
    const spatial::MortonLbvhSnapshotLeaf& left,
    const spatial::MortonLbvhSnapshotLeaf& right) noexcept {
  if (left.morton_code != right.morton_code) {
    return left.morton_code < right.morton_code;
  }
  return left.point_id < right.point_id;
}

class ExactFakeSnapshotBuilder final {
 public:
  ExactFakeSnapshotBuilder(
      std::span<const std::uint64_t> coordinate_bits,
      std::size_t point_count,
      std::span<const spatial::MortonLbvhSnapshotLeaf> leaves,
      std::vector<spatial::MortonLbvhSnapshotNode>& nodes)
      : coordinate_bits_(coordinate_bits),
        point_count_(point_count),
        leaves_(leaves),
        nodes_(nodes) {}

  [[nodiscard]] std::uint64_t build(
      std::size_t begin,
      std::size_t end,
      std::size_t depth) {
    if (begin >= end || end > leaves_.size()) {
      throw std::logic_error(
          "the fake Phase 14 LBVH received an invalid range");
    }
    maximum_depth_ = std::max(maximum_depth_, depth);
    if (end - begin == 1U) {
      spatial::MortonLbvhSnapshotNode leaf;
      leaf.leaf_begin = checked_u64(
          begin, "a fake leaf begin does not fit uint64");
      leaf.leaf_end = checked_u64(
          end, "a fake leaf end does not fit uint64");
      leaf.lower_point_ids.fill(leaves_[begin].point_id);
      leaf.upper_point_ids.fill(leaves_[begin].point_id);
      nodes_.push_back(leaf);
      return checked_u64(
          nodes_.size() - 1U,
          "a fake leaf index does not fit uint64");
    }

    const std::size_t split = find_split(begin, end);
    const std::uint64_t left = build(begin, split, depth + 1U);
    const std::uint64_t right = build(split, end, depth + 1U);
    spatial::MortonLbvhSnapshotNode node;
    node.left_child = left;
    node.right_child = right;
    node.leaf_begin = checked_u64(
        begin, "a fake internal begin does not fit uint64");
    node.leaf_end = checked_u64(
        end, "a fake internal end does not fit uint64");
    const auto& left_node =
        nodes_[static_cast<std::size_t>(left)];
    const auto& right_node =
        nodes_[static_cast<std::size_t>(right)];
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      node.lower_point_ids[axis] = extremum_witness(
          left_node.lower_point_ids[axis],
          right_node.lower_point_ids[axis],
          axis,
          false);
      node.upper_point_ids[axis] = extremum_witness(
          left_node.upper_point_ids[axis],
          right_node.upper_point_ids[axis],
          axis,
          true);
    }
    nodes_.push_back(node);
    return checked_u64(
        nodes_.size() - 1U,
        "a fake internal index does not fit uint64");
  }

  [[nodiscard]] std::size_t maximum_depth() const noexcept {
    return maximum_depth_;
  }

 private:
  [[nodiscard]] std::size_t find_split(
      std::size_t begin,
      std::size_t end) const {
    const std::uint64_t first_code = leaves_[begin].morton_code;
    const std::uint64_t last_code = leaves_[end - 1U].morton_code;
    if (first_code == last_code) {
      return begin + (end - begin) / 2U;
    }
    const std::uint64_t difference = first_code ^ last_code;
    const unsigned int highest_bit =
        static_cast<unsigned int>(
            std::bit_width(difference) - 1);
    const std::uint64_t mask =
        std::uint64_t{1} << highest_bit;
    std::size_t low = begin + 1U;
    std::size_t high = end;
    while (low < high) {
      const std::size_t middle =
          low + (high - low) / 2U;
      if ((leaves_[middle].morton_code & mask) == 0U) {
        low = middle + 1U;
      } else {
        high = middle;
      }
    }
    if (low <= begin || low >= end) {
      throw std::logic_error(
          "the fake Phase 14 radix split did not divide its range");
    }
    return low;
  }

  [[nodiscard]] exact::ExactRational coordinate(
      spatial::PointId point_id,
      std::size_t axis) const {
    if (point_id >= static_cast<std::uint64_t>(point_count_)) {
      throw std::logic_error(
          "a fake Phase 14 AABB witness is out of range");
    }
    return exact_word(
        coordinate_bits_[
            axis * point_count_ +
            static_cast<std::size_t>(point_id)]);
  }

  [[nodiscard]] spatial::PointId extremum_witness(
      spatial::PointId left,
      spatial::PointId right,
      std::size_t axis,
      bool maximum) const {
    const exact::ExactRational left_coordinate =
        coordinate(left, axis);
    const exact::ExactRational right_coordinate =
        coordinate(right, axis);
    if (maximum) {
      if (right_coordinate > left_coordinate ||
          (right_coordinate == left_coordinate && right < left)) {
        return right;
      }
      return left;
    }
    if (right_coordinate < left_coordinate ||
        (right_coordinate == left_coordinate && right < left)) {
      return right;
    }
    return left;
  }

  std::span<const std::uint64_t> coordinate_bits_;
  std::size_t point_count_{};
  std::span<const spatial::MortonLbvhSnapshotLeaf> leaves_;
  std::vector<spatial::MortonLbvhSnapshotNode>& nodes_;
  std::size_t maximum_depth_{};
};

}  // namespace

Phase14MortonBinProposalBatch propose_phase14_morton_bins_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& lower_coordinate_bits,
    const std::array<std::uint64_t, 3>& upper_coordinate_bits,
    std::size_t maximum_point_count) {
  require_common_inputs(
      axis_major_coordinate_bits,
      point_count,
      maximum_point_count);
  test_support::fake_proposal_count.fetch_add(
      1U, std::memory_order_relaxed);
  test_support::fake_last_point_count.store(
      point_count, std::memory_order_relaxed);
  test_support::fake_last_point_capacity.store(
      maximum_point_count, std::memory_order_relaxed);

  const Corruption corruption =
      test_support::fake_corruption.load(std::memory_order_relaxed);
  if (corruption == Corruption::simulated_async_failure) {
    throw std::runtime_error(
        "simulated asynchronous Phase 14 Morton proposal failure");
  }

  const std::size_t axis_count =
      checked_product(
          point_count,
          kAxisCount,
          "the fake Phase 14 axis count overflows");
  Phase14MortonBinProposalBatch batch;
  batch.encoded_bins.resize(axis_count);
  batch.axis_count = axis_count;
  batch.host_to_device_coordinate_byte_count =
      axis_major_coordinate_bits.size_bytes();
  batch.device_to_host_encoded_bin_byte_count =
      batch.encoded_bins.size() * sizeof(std::uint32_t);
  batch.buffer_epoch = context.advance_epoch();
  batch.execution_kind =
      Phase14MortonLbvhExecutionKind::host_fake;
  batch.cuda_path_qualified = false;

  const std::size_t forced_ambiguous =
      test_support::fake_forced_ambiguous_axis_index.load(
          std::memory_order_relaxed);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      const std::size_t index =
          axis * point_count + point_index;
      if (index == forced_ambiguous) {
        batch.encoded_bins[index] =
            phase14_morton_bin_ambiguous_bit;
        continue;
      }
      const std::uint64_t bin = exact_bin(
          axis_major_coordinate_bits[index],
          lower_coordinate_bits[axis],
          upper_coordinate_bits[axis]);
      batch.encoded_bins[index] =
          static_cast<std::uint32_t>(bin);
    }
  }

  switch (corruption) {
    case Corruption::none:
    case Corruption::wrong_snapshot_extent:
    case Corruption::wrong_snapshot_transfer_extent:
    case Corruption::wrong_snapshot_submission_count:
    case Corruption::wrong_postorder_child:
    case Corruption::wrong_aabb_witness:
    case Corruption::stale_snapshot_epoch:
    case Corruption::simulated_async_failure:
      break;
    case Corruption::wrong_proposal_extent:
      batch.encoded_bins.pop_back();
      break;
    case Corruption::reserved_proposal_bits:
      batch.encoded_bins[0U] = std::uint32_t{1} << 30U;
      break;
    case Corruption::wrong_unambiguous_bin: {
      const std::uint32_t current =
          batch.encoded_bins[0U] &
          phase14_morton_bin_value_mask;
      batch.encoded_bins[0U] =
          current == phase14_morton_bin_value_mask
              ? current - 1U
              : current + 1U;
      break;
    }
    case Corruption::stale_proposal_epoch:
      --batch.buffer_epoch;
      break;
  }
  return batch;
}

Phase14MortonLbvhSnapshotBatch
build_phase14_morton_lbvh_snapshot_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> certified_morton_codes,
    std::size_t maximum_point_count) {
  require_common_inputs(
      axis_major_coordinate_bits,
      point_count,
      maximum_point_count);
  if (certified_morton_codes.size() != point_count) {
    throw std::invalid_argument(
        "the fake Phase 14 Morton code extent is invalid");
  }
  test_support::fake_snapshot_count.fetch_add(
      1U, std::memory_order_relaxed);

  Phase14MortonLbvhSnapshotBatch batch;
  batch.point_count = checked_u64(
      point_count, "the fake point count does not fit uint64");
  batch.leaves.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    batch.leaves.push_back(
        spatial::MortonLbvhSnapshotLeaf{
            certified_morton_codes[point_index],
            checked_u64(
                point_index,
                "a fake Phase 14 PointId does not fit uint64")});
  }
  std::stable_sort(
      batch.leaves.begin(),
      batch.leaves.end(),
      [](const spatial::MortonLbvhSnapshotLeaf& left,
         const spatial::MortonLbvhSnapshotLeaf& right) {
        return left.morton_code < right.morton_code;
      });
  if (!std::is_sorted(
          batch.leaves.begin(),
          batch.leaves.end(),
          leaf_less)) {
    throw std::logic_error(
        "the fake Phase 14 stable sort lost PointId tie order");
  }

  const std::size_t node_count = point_count * 2U - 1U;
  batch.nodes.reserve(node_count);
  ExactFakeSnapshotBuilder builder{
      axis_major_coordinate_bits,
      point_count,
      batch.leaves,
      batch.nodes};
  batch.root_node_index =
      builder.build(0U, point_count, 0U);
  batch.proposed_counters.point_count =
      checked_u64(point_count, "the fake point count does not fit uint64");
  batch.proposed_counters.node_count =
      checked_u64(node_count, "the fake node count does not fit uint64");
  batch.proposed_counters.maximum_depth = checked_u64(
      builder.maximum_depth(),
      "the fake maximum depth does not fit uint64");

  std::size_t group_begin = 0U;
  while (group_begin < point_count) {
    std::size_t group_end = group_begin + 1U;
    while (group_end < point_count &&
           batch.leaves[group_end].morton_code ==
               batch.leaves[group_begin].morton_code) {
      ++group_end;
    }
    const std::size_t group_size = group_end - group_begin;
    if (group_size > 1U) {
      ++batch.proposed_counters.morton_collision_group_count;
      batch.proposed_counters.maximum_morton_collision_size =
          std::max(
              batch.proposed_counters.maximum_morton_collision_size,
              checked_u64(
                  group_size,
                  "the fake collision size does not fit uint64"));
    }
    group_begin = group_end;
  }

  batch.host_to_device_morton_code_byte_count =
      certified_morton_codes.size_bytes();
  batch.device_to_host_leaf_byte_count =
      batch.leaves.size() *
      sizeof(spatial::MortonLbvhSnapshotLeaf);
  batch.device_to_host_node_byte_count =
      batch.nodes.size() *
      sizeof(spatial::MortonLbvhSnapshotNode);
  const std::size_t maximum_axis_count = checked_product(
      maximum_point_count,
      kAxisCount,
      "the fake Phase 14 maximum axis count overflows");
  const std::size_t maximum_node_count =
      checked_product(
          maximum_point_count,
          2U,
          "the fake Phase 14 maximum node count overflows") -
      1U;
  batch.device_coordinate_byte_capacity = checked_product(
      maximum_axis_count,
      sizeof(std::uint64_t),
      "the fake Phase 14 device coordinate capacity overflows");
  batch.device_encoded_bin_byte_capacity = checked_product(
      maximum_axis_count,
      sizeof(std::uint32_t),
      "the fake Phase 14 device bin capacity overflows");
  batch.device_morton_code_double_buffer_byte_capacity =
      checked_product(
          maximum_point_count,
          2U * sizeof(std::uint64_t),
          "the fake Phase 14 device code capacity overflows");
  batch.device_point_id_double_buffer_byte_capacity =
      batch.device_morton_code_double_buffer_byte_capacity;
  batch.device_leaf_byte_capacity = checked_product(
      maximum_point_count,
      sizeof(spatial::MortonLbvhSnapshotLeaf),
      "the fake Phase 14 device leaf capacity overflows");
  batch.device_node_byte_capacity = checked_product(
      maximum_node_count,
      sizeof(spatial::MortonLbvhSnapshotNode),
      "the fake Phase 14 device node capacity overflows");
  batch.device_frontier_double_buffer_byte_capacity =
      checked_product(
          maximum_point_count,
          2U * kDeviceRangeByteSize,
          "the fake Phase 14 device frontier capacity overflows");
  batch.device_level_schedule_byte_capacity = checked_product(
      maximum_node_count,
      sizeof(std::uint64_t),
      "the fake Phase 14 device level capacity overflows");
  batch.device_control_byte_capacity = kDeviceControlByteSize;
  const auto add_device_capacity =
      [&batch](std::size_t capacity) {
        batch.total_fixed_device_byte_capacity = checked_sum(
            batch.total_fixed_device_byte_capacity,
            capacity,
            "the fake Phase 14 total device capacity overflows");
      };
  add_device_capacity(batch.device_coordinate_byte_capacity);
  add_device_capacity(batch.device_encoded_bin_byte_capacity);
  add_device_capacity(
      batch.device_morton_code_double_buffer_byte_capacity);
  add_device_capacity(
      batch.device_point_id_double_buffer_byte_capacity);
  add_device_capacity(batch.device_leaf_byte_capacity);
  add_device_capacity(batch.device_node_byte_capacity);
  add_device_capacity(
      batch.device_frontier_double_buffer_byte_capacity);
  add_device_capacity(batch.device_level_schedule_byte_capacity);
  add_device_capacity(batch.device_control_byte_capacity);
  batch.buffer_epoch = context.advance_epoch();
  batch.execution_kind =
      Phase14MortonLbvhExecutionKind::host_fake;
  batch.cuda_path_qualified = false;

  const Corruption corruption =
      test_support::fake_corruption.load(std::memory_order_relaxed);
  switch (corruption) {
    case Corruption::none:
    case Corruption::wrong_proposal_extent:
    case Corruption::reserved_proposal_bits:
    case Corruption::wrong_unambiguous_bin:
    case Corruption::stale_proposal_epoch:
    case Corruption::simulated_async_failure:
      break;
    case Corruption::wrong_snapshot_extent:
      batch.nodes.pop_back();
      break;
    case Corruption::wrong_snapshot_transfer_extent:
      ++batch.device_to_host_node_byte_count;
      break;
    case Corruption::wrong_snapshot_submission_count:
      ++batch.kernel_launch_count;
      break;
    case Corruption::wrong_postorder_child:
      if (point_count < 2U) {
        throw std::logic_error(
            "postorder corruption requires at least two points");
      }
      batch.nodes.back().left_child =
          batch.root_node_index;
      break;
    case Corruption::wrong_aabb_witness:
      if (point_count < 2U) {
        throw std::logic_error(
            "AABB corruption requires at least two points");
      }
      batch.nodes.back().lower_point_ids[0U] =
          (batch.nodes.back().lower_point_ids[0U] + UINT64_C(1)) %
          static_cast<std::uint64_t>(point_count);
      break;
    case Corruption::stale_snapshot_epoch:
      --batch.buffer_epoch;
      break;
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
