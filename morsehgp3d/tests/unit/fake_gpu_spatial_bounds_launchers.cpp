#include "fake_gpu_spatial_bounds_launchers.hpp"

#include "phase4_spatial_bounds_internal.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

constexpr std::size_t kAxisCount = 3U;

std::atomic<FakeSpatialBoundsProposalPermutation> proposal_permutation{
    FakeSpatialBoundsProposalPermutation::canonical};
std::atomic<FakeSpatialBoundsProposalValues> proposal_values{
    FakeSpatialBoundsProposalValues::actual_interval_recipe};
std::atomic<FakeSpatialBoundsProposalCorruption> proposal_corruption{
    FakeSpatialBoundsProposalCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_last_box_count{0U};
std::array<std::atomic<std::uint64_t>, kAxisCount>
    proposal_last_query_lower_bits{};
std::array<std::atomic<std::uint64_t>, kAxisCount>
    proposal_last_query_upper_bits{};
std::array<std::atomic<std::uint64_t>, 2> proposal_last_cutoff_bits{};

template <std::size_t Size>
[[nodiscard]] std::array<std::uint64_t, Size> load_words(
    const std::array<std::atomic<std::uint64_t>, Size>& source) noexcept {
  std::array<std::uint64_t, Size> result{};
  for (std::size_t index = 0U; index < Size; ++index) {
    result[index] = source[index].load(std::memory_order_relaxed);
  }
  return result;
}

template <std::size_t Size>
void store_words(
    std::array<std::atomic<std::uint64_t>, Size>& destination,
    const std::array<std::uint64_t, Size>& source) noexcept {
  for (std::size_t index = 0U; index < Size; ++index) {
    destination[index].store(source[index], std::memory_order_relaxed);
  }
}

}  // namespace

void configure_fake_gpu_spatial_bounds(
    FakeSpatialBoundsProposalConfiguration configuration) noexcept {
  proposal_permutation.store(
      configuration.permutation, std::memory_order_relaxed);
  proposal_values.store(configuration.values, std::memory_order_relaxed);
  proposal_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_spatial_bounds() noexcept {
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_last_box_count.store(0U, std::memory_order_relaxed);
  store_words(proposal_last_query_lower_bits, std::array<std::uint64_t, 3>{});
  store_words(proposal_last_query_upper_bits, std::array<std::uint64_t, 3>{});
  store_words(proposal_last_cutoff_bits, std::array<std::uint64_t, 2>{});
}

std::size_t fake_gpu_spatial_bounds_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_spatial_bounds_last_box_count() noexcept {
  return proposal_last_box_count.load(std::memory_order_relaxed);
}

std::array<std::uint64_t, 3>
fake_gpu_spatial_bounds_last_query_lower_bits() noexcept {
  return load_words(proposal_last_query_lower_bits);
}

std::array<std::uint64_t, 3>
fake_gpu_spatial_bounds_last_query_upper_bits() noexcept {
  return load_words(proposal_last_query_upper_bits);
}

std::array<std::uint64_t, 2>
fake_gpu_spatial_bounds_last_cutoff_bits() noexcept {
  return load_words(proposal_last_cutoff_bits);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);

struct FakeDirectedSquaredDistance {
  double lower{0.0};
  double upper{0.0};
  bool valid{true};
};

[[nodiscard]] double subtract_down_nonnegative(
    double left, double right) {
  if (left == right) {
    return 0.0;
  }
  const double rounded = left - right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] double subtract_up_nonnegative(
    double left, double right) {
  if (left == right) {
    return 0.0;
  }
  const double rounded = left - right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return std::nextafter(
      rounded, std::numeric_limits<double>::infinity());
}

[[nodiscard]] double multiply_down_nonnegative(double value) {
  if (value == 0.0) {
    return 0.0;
  }
  const double rounded = value * value;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] double multiply_up_nonnegative(double value) {
  if (value == 0.0) {
    return 0.0;
  }
  const double rounded = value * value;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return std::nextafter(
      rounded, std::numeric_limits<double>::infinity());
}

[[nodiscard]] double add_down_nonnegative(double left, double right) {
  if (left == 0.0 && right == 0.0) {
    return 0.0;
  }
  const double rounded = left + right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] double add_up_nonnegative(double left, double right) {
  if (left == 0.0 && right == 0.0) {
    return 0.0;
  }
  const double rounded = left + right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return std::nextafter(
      rounded, std::numeric_limits<double>::infinity());
}

[[nodiscard]] FakeDirectedSquaredDistance fake_minimum_squared_distance(
    const SpatialBoundsInputRecord& box,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits) {
  FakeDirectedSquaredDistance result;
  for (std::size_t axis = 0U; axis < query_lower_bits.size(); ++axis) {
    const double lower = std::bit_cast<double>(box.lower_bits[axis]);
    const double upper = std::bit_cast<double>(box.upper_bits[axis]);
    const double query_lower =
        std::bit_cast<double>(query_lower_bits[axis]);
    const double query_upper =
        std::bit_cast<double>(query_upper_bits[axis]);
    double lower_delta = 0.0;
    if (query_upper < lower) {
      lower_delta = subtract_down_nonnegative(lower, query_upper);
    } else if (query_lower > upper) {
      lower_delta = subtract_down_nonnegative(query_lower, upper);
    }

    double upper_delta_from_lower = 0.0;
    if (query_lower < lower) {
      upper_delta_from_lower =
          subtract_up_nonnegative(lower, query_lower);
    }
    double upper_delta_from_upper = 0.0;
    if (query_upper > upper) {
      upper_delta_from_upper =
          subtract_up_nonnegative(query_upper, upper);
    }
    const double upper_delta =
        std::max(upper_delta_from_lower, upper_delta_from_upper);
    const double lower_square = multiply_down_nonnegative(lower_delta);
    const double upper_square = multiply_up_nonnegative(upper_delta);
    const double next_lower =
        add_down_nonnegative(result.lower, lower_square);
    const double next_upper =
        add_up_nonnegative(result.upper, upper_square);
    if (!std::isfinite(lower_delta) || !std::isfinite(upper_delta) ||
        !std::isfinite(lower_square) || !std::isfinite(upper_square) ||
        !std::isfinite(next_lower) || !std::isfinite(next_upper) ||
        lower_delta < 0.0 || upper_delta < 0.0 ||
        lower_delta > upper_delta || lower_square > upper_square ||
        next_lower > next_upper) {
      return FakeDirectedSquaredDistance{0.0, 0.0, false};
    }
    result.lower = next_lower;
    result.upper = next_upper;
  }
  return result;
}

[[nodiscard]] SpatialBoundsProposalRecord fake_record(
    std::size_t box_index,
    const SpatialBoundsInputRecord& box,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits,
    test_support::FakeSpatialBoundsProposalValues values) {
  const FakeDirectedSquaredDistance squared_distance =
      fake_minimum_squared_distance(
          box, query_lower_bits, query_upper_bits);
  if (!squared_distance.valid) {
    return SpatialBoundsProposalRecord{
        static_cast<std::uint64_t>(box_index),
        0U,
        kPositiveInfinityBits,
        spatial_bounds_unknown_code};
  }
  const std::uint64_t lower_squared_distance_bits =
      std::bit_cast<std::uint64_t>(squared_distance.lower);
  const std::uint64_t upper_squared_distance_bits =
      std::bit_cast<std::uint64_t>(squared_distance.upper);
  std::uint64_t decision = spatial_bounds_unknown_code;
  switch (values) {
    case test_support::FakeSpatialBoundsProposalValues::actual_interval_recipe:
      if (squared_distance.lower >
          std::bit_cast<double>(cutoff_upper_bits)) {
        decision = spatial_bounds_prune_code;
      } else if (squared_distance.upper <
                 std::bit_cast<double>(cutoff_lower_bits)) {
        decision = spatial_bounds_visit_code;
      } else {
        decision = spatial_bounds_unknown_code;
      }
      break;
    case test_support::FakeSpatialBoundsProposalValues::all_unknown:
      decision = spatial_bounds_unknown_code;
      break;
    case test_support::FakeSpatialBoundsProposalValues::all_visit:
      decision = spatial_bounds_visit_code;
      break;
    case test_support::FakeSpatialBoundsProposalValues::all_prune:
      decision = spatial_bounds_prune_code;
      break;
  }
  return SpatialBoundsProposalRecord{
      static_cast<std::uint64_t>(box_index),
      lower_squared_distance_bits,
      upper_squared_distance_bits,
      decision};
}

}  // namespace

SpatialBoundsProposalBatch propose_strict_aabb_prunes_on_gpu(
    SpatialBoundsContextState& context,
    std::span<const SpatialBoundsInputRecord> boxes,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  if (boxes.empty()) {
    throw std::invalid_argument(
        "the fake Phase 4 spatial-bounds launcher requires at least one box");
  }

  const test_support::FakeSpatialBoundsProposalPermutation permutation =
      test_support::proposal_permutation.load(std::memory_order_relaxed);
  const test_support::FakeSpatialBoundsProposalValues values =
      test_support::proposal_values.load(std::memory_order_relaxed);
  const test_support::FakeSpatialBoundsProposalCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  if (corruption ==
      test_support::FakeSpatialBoundsProposalCorruption::simulated_gpu_failure) {
    throw std::runtime_error("simulated Phase 4 spatial-bounds GPU failure");
  }

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_box_count.store(
      boxes.size(), std::memory_order_relaxed);
  test_support::store_words(
      test_support::proposal_last_query_lower_bits, query_lower_bits);
  test_support::store_words(
      test_support::proposal_last_query_upper_bits, query_upper_bits);
  test_support::store_words(
      test_support::proposal_last_cutoff_bits,
      std::array<std::uint64_t, 2>{
          cutoff_lower_bits, cutoff_upper_bits});

  SpatialBoundsProposalBatch batch;
  batch.records.reserve(boxes.size());
  for (std::size_t output_index = 0U; output_index < boxes.size();
       ++output_index) {
    const std::size_t box_index =
        permutation ==
                test_support::FakeSpatialBoundsProposalPermutation::canonical
            ? output_index
            : boxes.size() - 1U - output_index;
    batch.records.push_back(fake_record(
        box_index,
        boxes[box_index],
        query_lower_bits,
        query_upper_bits,
        cutoff_lower_bits,
        cutoff_upper_bits,
        values));
  }
  batch.buffer_epoch = context.advance_epoch();

  switch (corruption) {
    case test_support::FakeSpatialBoundsProposalCorruption::none:
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::missing_record:
      batch.records.pop_back();
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::duplicate_box_index:
      if (batch.records.size() > 1U) {
        batch.records.back().box_index = batch.records.front().box_index;
      }
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::out_of_range_box_index:
      batch.records.front().box_index =
          static_cast<std::uint64_t>(boxes.size());
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::invalid_decision:
      batch.records.front().decision_code = spatial_bounds_sentinel_code;
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::false_prune: {
      SpatialBoundsProposalRecord& record = batch.records.front();
      record.decision_code = spatial_bounds_prune_code;
      const double cutoff = std::bit_cast<double>(cutoff_upper_bits);
      const double claimed_lower = std::nextafter(
          cutoff, std::numeric_limits<double>::infinity());
      record.lower_squared_distance_bits =
          std::bit_cast<std::uint64_t>(claimed_lower);
      record.upper_squared_distance_bits =
          record.lower_squared_distance_bits;
      break;
    }
    case test_support::FakeSpatialBoundsProposalCorruption::simulated_gpu_failure:
      throw std::logic_error(
          "the simulated spatial-bounds failure must precede publication");
  }
  return batch;
}

SpatialLbvhCoverBatch propose_strict_lbvh_cover_on_gpu(
    SpatialLbvhContextState& context,
    std::span<const SpatialLbvhNodeInputRecord> nodes,
    std::size_t root_index,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  if (nodes.empty() || root_index >= nodes.size()) {
    throw std::invalid_argument(
        "the fake Phase 4 spatial-LBVH launcher requires a valid tree");
  }

  const test_support::FakeSpatialBoundsProposalPermutation permutation =
      test_support::proposal_permutation.load(std::memory_order_relaxed);
  const test_support::FakeSpatialBoundsProposalValues values =
      test_support::proposal_values.load(std::memory_order_relaxed);
  const test_support::FakeSpatialBoundsProposalCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  if (corruption ==
      test_support::FakeSpatialBoundsProposalCorruption::simulated_gpu_failure) {
    throw std::runtime_error("simulated Phase 4 spatial-LBVH GPU failure");
  }

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_box_count.store(
      nodes.size(), std::memory_order_relaxed);
  test_support::store_words(
      test_support::proposal_last_query_lower_bits, query_lower_bits);
  test_support::store_words(
      test_support::proposal_last_query_upper_bits, query_upper_bits);
  test_support::store_words(
      test_support::proposal_last_cutoff_bits,
      std::array<std::uint64_t, 2>{
          cutoff_lower_bits, cutoff_upper_bits});

  SpatialLbvhCoverBatch batch;
  batch.records.resize(nodes.size());
  std::vector<std::size_t> traversal_stack{root_index};
  while (!traversal_stack.empty()) {
    const std::size_t node_index = traversal_stack.back();
    traversal_stack.pop_back();
    if (node_index >= nodes.size()) {
      throw std::logic_error(
          "the fake spatial-LBVH traversal reached an invalid node");
    }
    const SpatialLbvhNodeInputRecord& node = nodes[node_index];
    const SpatialBoundsProposalRecord proposal = fake_record(
        node_index,
        node.bounds,
        query_lower_bits,
        query_upper_bits,
        cutoff_lower_bits,
        cutoff_upper_bits,
        values);
    const bool prune =
        proposal.decision_code == spatial_bounds_prune_code;
    const bool leaf =
        node.left_child == spatial_bounds_sentinel_code &&
        node.right_child == spatial_bounds_sentinel_code;
    if (prune || leaf) {
      if (batch.record_count >= batch.records.size()) {
        throw std::logic_error(
            "the fake spatial-LBVH cover exceeded its node capacity");
      }
      batch.records[batch.record_count++] = SpatialLbvhCoverRecord{
          static_cast<std::uint64_t>(node_index),
          proposal.lower_squared_distance_bits,
          proposal.upper_squared_distance_bits,
          prune ? spatial_lbvh_cover_prune_code
                : spatial_lbvh_cover_leaf_code};
      continue;
    }
    if (node.left_child == spatial_bounds_sentinel_code ||
        node.right_child == spatial_bounds_sentinel_code ||
        !std::in_range<std::size_t>(node.left_child) ||
        !std::in_range<std::size_t>(node.right_child)) {
      throw std::logic_error(
          "the fake spatial-LBVH traversal found invalid children");
    }
    traversal_stack.push_back(
        static_cast<std::size_t>(node.right_child));
    traversal_stack.push_back(
        static_cast<std::size_t>(node.left_child));
  }
  if (permutation ==
      test_support::FakeSpatialBoundsProposalPermutation::reversed) {
    std::reverse(
        batch.records.begin(),
        batch.records.begin() +
            static_cast<std::vector<SpatialLbvhCoverRecord>::difference_type>(
                batch.record_count));
  }
  batch.buffer_epoch = context.advance_epoch();

  switch (corruption) {
    case test_support::FakeSpatialBoundsProposalCorruption::none:
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::missing_record:
      if (batch.record_count != 0U) {
        --batch.record_count;
      }
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::duplicate_box_index:
      if (batch.record_count > 1U) {
        batch.records[batch.record_count - 1U].node_index =
            batch.records.front().node_index;
      }
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::out_of_range_box_index:
      batch.records.front().node_index =
          static_cast<std::uint64_t>(nodes.size());
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::invalid_decision:
      batch.records.front().kind = spatial_bounds_sentinel_code;
      break;
    case test_support::FakeSpatialBoundsProposalCorruption::false_prune: {
      SpatialLbvhCoverRecord& record = batch.records.front();
      record.kind = spatial_lbvh_cover_prune_code;
      const double cutoff = std::bit_cast<double>(cutoff_upper_bits);
      const double claimed_lower = std::nextafter(
          cutoff, std::numeric_limits<double>::infinity());
      record.lower_squared_distance_bits =
          std::bit_cast<std::uint64_t>(claimed_lower);
      record.upper_squared_distance_bits =
          record.lower_squared_distance_bits;
      break;
    }
    case test_support::FakeSpatialBoundsProposalCorruption::simulated_gpu_failure:
      throw std::logic_error(
          "the simulated spatial-LBVH failure must precede publication");
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
