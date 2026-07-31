#include "fake_gpu_morton_yao48_ranked_pair_h0_projection_launchers.hpp"

#include "phase15_morton_yao48_ranked_pair_h0_projection_internal.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<std::size_t> fake_launch_count{0U};

}  // namespace

void reset_fake_gpu_morton_yao48_ranked_pair_h0_projection() noexcept {
  fake_launch_count.store(0U, std::memory_order_relaxed);
}

std::size_t
fake_gpu_morton_yao48_ranked_pair_h0_projection_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

struct FakeProjectionStorage final {
  std::vector<Phase15MortonYao48RankedPairH0Record> records;
};

[[nodiscard]] bool valid_range(
    const std::uint64_t* offsets,
    std::size_t index,
    std::size_t payload_count) {
  return offsets != nullptr && offsets[index] <= offsets[index + 1U] &&
      offsets[index + 1U] <= payload_count;
}

}  // namespace

Phase15MortonYao48RankedPairH0ProjectionReceipt
phase15_launch_morton_yao48_ranked_pair_h0_projection(
    const Phase15MortonYao48RankedPairH0ProjectionRequest& request) {
  test_support::fake_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  if (!request.host_fake || request.cuda_device != -1 ||
      request.record_count > request.record_capacity ||
      (request.record_count != 0U &&
       (request.support_u == nullptr || request.support_v == nullptr ||
        request.closed_rank == nullptr ||
        request.strict_offsets == nullptr ||
        request.shell_offsets == nullptr))) {
    throw std::invalid_argument(
        "the fake ranked-pair H0 projection request is invalid");
  }

  auto storage = std::make_shared<FakeProjectionStorage>();
  storage->records.resize(request.record_count);
  std::size_t regular_count = 0U;
  std::size_t diagnostic_count = 0U;
  for (std::size_t index = 0U; index < request.record_count; ++index) {
    if (!valid_range(
            request.strict_offsets,
            index,
            request.strict_point_id_count) ||
        !valid_range(
            request.shell_offsets,
            index,
            request.shell_point_id_count)) {
      throw std::runtime_error(
          "the fake ranked-pair H0 projection received invalid offsets");
    }
    const std::uint64_t strict_begin = request.strict_offsets[index];
    const std::uint64_t strict_end = request.strict_offsets[index + 1U];
    const std::uint64_t shell_begin = request.shell_offsets[index];
    const std::uint64_t shell_end = request.shell_offsets[index + 1U];
    const std::uint64_t strict_count = strict_end - strict_begin;
    const std::uint64_t shell_count = shell_end - shell_begin;
    const std::uint64_t closed_rank = request.closed_rank[index];
    if (request.support_u[index] >= request.support_v[index] ||
        request.support_v[index] >= request.point_count ||
        closed_rank < 2U ||
        closed_rank > request.maximum_closed_rank ||
        closed_rank != 2U + strict_count + shell_count) {
      throw std::runtime_error(
          "the fake ranked-pair H0 projection received a malformed record");
    }
    auto& record = storage->records[index];
    record.squared_level.numerator.limb[0U] =
        static_cast<std::uint64_t>(2U * index + 1U);
    record.squared_level.exponent = 0;
    record.squared_level.limb_count = 1U;
    record.support_u = request.support_u[index];
    record.support_v = request.support_v[index];
    record.strict_begin = strict_begin;
    record.strict_end = strict_end;
    record.shell_begin = shell_begin;
    record.shell_end = shell_end;
    record.closed_rank = static_cast<std::uint8_t>(closed_rank);
    if (shell_count == 0U) {
      record.kind =
          Phase15MortonYao48RankedPairH0RecordKind::
              regular_pair_candidate;
      ++regular_count;
    } else {
      record.kind =
          Phase15MortonYao48RankedPairH0RecordKind::
              diagnostic_or_carrier_candidate;
      ++diagnostic_count;
    }
  }

  Phase15MortonYao48RankedPairH0ProjectionReceipt receipt;
  receipt.output_owner = storage;
  receipt.records =
      storage->records.empty() ? nullptr : storage->records.data();
  receipt.point_count = request.point_count;
  receipt.maximum_closed_rank = request.maximum_closed_rank;
  receipt.record_capacity = request.record_capacity;
  receipt.source_record_count = request.record_count;
  receipt.projected_record_count = request.record_count;
  receipt.regular_pair_candidate_count = regular_count;
  receipt.diagnostic_or_carrier_candidate_count = diagnostic_count;
  receipt.strict_point_id_count = request.strict_point_id_count;
  receipt.shell_point_id_count = request.shell_point_id_count;
  receipt.execution_kind =
      Phase15MortonYao48RankedPairH0ProjectionExecutionKind::host_fake;
  receipt.fixed_linear_capacity_validated = true;
  receipt.one_record_per_source_validated = true;
  receipt.exact_level_payload_constructed = true;
  receipt.strict_and_shell_payload_ranges_validated = true;
  receipt.regular_pair_closed_rank_identity_validated = true;
  receipt.scientific_birth_and_saddle_roles_withheld = true;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
