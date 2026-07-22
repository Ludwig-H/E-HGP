#include "fake_gpu_pair_support_phi_launchers.hpp"

#include "phase9_pair_support_phi_internal.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
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

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

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

}  // namespace morsehgp3d::gpu::detail
