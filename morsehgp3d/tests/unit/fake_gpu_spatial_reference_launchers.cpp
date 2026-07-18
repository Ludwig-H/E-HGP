#include "fake_gpu_spatial_reference_launchers.hpp"

#include "phase4_spatial_reference_internal.hpp"

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

constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kQuietNanBits = UINT64_C(0x7ff8000000000000);

std::atomic<FakeSpatialProposalPermutation> proposal_permutation{
    FakeSpatialProposalPermutation::canonical};
std::atomic<FakeSpatialProposalValues> proposal_values{
    FakeSpatialProposalValues::actual_binary64_recipe};
std::atomic<FakeSpatialProposalCorruption> proposal_corruption{
    FakeSpatialProposalCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_last_point_count{0U};
std::array<std::atomic<std::uint64_t>, kAxisCount> proposal_last_query_bits{};

[[nodiscard]] std::uint64_t fake_distance_bits(
    FakeSpatialProposalValues values,
    std::uint64_t point_id,
    std::size_t point_count) {
  switch (values) {
    case FakeSpatialProposalValues::actual_binary64_recipe:
      throw std::logic_error(
          "the actual fake recipe needs point and query coordinates");
    case FakeSpatialProposalValues::all_zero:
      return std::bit_cast<std::uint64_t>(0.0);
    case FakeSpatialProposalValues::ascending_by_point_id:
      return std::bit_cast<std::uint64_t>(
          static_cast<double>(point_id) + 1.0);
    case FakeSpatialProposalValues::descending_by_point_id:
      return std::bit_cast<std::uint64_t>(static_cast<double>(
          point_count - static_cast<std::size_t>(point_id)));
    case FakeSpatialProposalValues::positive_infinity:
      return std::bit_cast<std::uint64_t>(
          std::numeric_limits<double>::infinity());
  }
  throw std::logic_error("unknown fake Phase 4 proposal-value mode");
}

[[nodiscard]] std::uint64_t actual_distance_bits(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::size_t point_index,
    const std::array<std::uint64_t, 3>& query_bits) {
  double squared_distance = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double point_coordinate = std::bit_cast<double>(
        coordinate_bits[axis * point_count + point_index]);
    const double query_coordinate = std::bit_cast<double>(query_bits[axis]);
    const volatile double difference = point_coordinate - query_coordinate;
    const volatile double square = difference * difference;
    const volatile double accumulated = squared_distance + square;
    squared_distance = accumulated;
  }
  return std::bit_cast<std::uint64_t>(squared_distance);
}

}  // namespace

void configure_fake_gpu_spatial_reference(
    FakeSpatialProposalConfiguration configuration) noexcept {
  proposal_permutation.store(
      configuration.permutation, std::memory_order_relaxed);
  proposal_values.store(configuration.values, std::memory_order_relaxed);
  proposal_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_spatial_reference() noexcept {
  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{});
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_last_point_count.store(0U, std::memory_order_relaxed);
  for (auto& bits : proposal_last_query_bits) {
    bits.store(0U, std::memory_order_relaxed);
  }
}

std::size_t fake_gpu_spatial_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_spatial_last_point_count() noexcept {
  return proposal_last_point_count.load(std::memory_order_relaxed);
}

std::array<std::uint64_t, 3> fake_gpu_spatial_last_query_bits() noexcept {
  std::array<std::uint64_t, 3> result{};
  for (std::size_t axis = 0U; axis < result.size(); ++axis) {
    result[axis] =
        proposal_last_query_bits[axis].load(std::memory_order_relaxed);
  }
  return result;
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {

SpatialProposalBatch propose_squared_distances_on_gpu(
    SpatialReferenceContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& query_bits) {
  constexpr std::size_t axis_count = 3U;
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / axis_count ||
      coordinate_bits.size() != point_count * axis_count) {
    throw std::invalid_argument("invalid fake Phase 4 spatial proposal input");
  }

  const test_support::FakeSpatialProposalPermutation permutation =
      test_support::proposal_permutation.load(std::memory_order_relaxed);
  const test_support::FakeSpatialProposalValues values =
      test_support::proposal_values.load(std::memory_order_relaxed);
  const test_support::FakeSpatialProposalCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  if (corruption ==
      test_support::FakeSpatialProposalCorruption::simulated_gpu_failure) {
    throw std::runtime_error("simulated Phase 4 GPU proposal failure");
  }

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_point_count.store(
      point_count, std::memory_order_relaxed);
  for (std::size_t axis = 0U; axis < query_bits.size(); ++axis) {
    test_support::proposal_last_query_bits[axis].store(
        query_bits[axis], std::memory_order_relaxed);
  }

  SpatialProposalBatch batch;
  batch.records.reserve(point_count);
  for (std::size_t output_index = 0U; output_index < point_count;
       ++output_index) {
    const std::size_t point_index =
        permutation == test_support::FakeSpatialProposalPermutation::canonical
            ? output_index
            : point_count - 1U - output_index;
    const std::uint64_t point_id = static_cast<std::uint64_t>(point_index);
    const std::uint64_t squared_distance_bits =
        values == test_support::FakeSpatialProposalValues::actual_binary64_recipe
            ? test_support::actual_distance_bits(
                  coordinate_bits, point_count, point_index, query_bits)
            : test_support::fake_distance_bits(values, point_id, point_count);
    batch.records.push_back(
        SpatialProposalRecord{point_id, squared_distance_bits});
  }
  batch.buffer_epoch = context.advance_epoch();

  switch (corruption) {
    case test_support::FakeSpatialProposalCorruption::none:
      break;
    case test_support::FakeSpatialProposalCorruption::missing_record:
      batch.records.pop_back();
      break;
    case test_support::FakeSpatialProposalCorruption::duplicate_point_id:
      if (batch.records.size() > 1U) {
        batch.records.back().point_id = batch.records.front().point_id;
      }
      break;
    case test_support::FakeSpatialProposalCorruption::out_of_range_point_id:
      batch.records.front().point_id = static_cast<std::uint64_t>(point_count);
      break;
    case test_support::FakeSpatialProposalCorruption::nan_distance:
      batch.records.front().squared_distance_bits = test_support::kQuietNanBits;
      break;
    case test_support::FakeSpatialProposalCorruption::negative_distance:
      batch.records.front().squared_distance_bits =
          std::bit_cast<std::uint64_t>(-1.0);
      break;
    case test_support::FakeSpatialProposalCorruption::simulated_gpu_failure:
      throw std::logic_error(
          "the simulated failure must be raised before publication");
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
