#include "fake_gpu_phase14_facet_top_k_proposal_launchers.hpp"

#include "phase14_facet_top_k_proposal_internal.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<FakePhase14FacetTopKProposalCorruption> proposal_corruption{
    FakePhase14FacetTopKProposalCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_epoch_advance_count{0U};
std::atomic<std::size_t> proposal_last_query_count{0U};
std::atomic<std::size_t> proposal_last_record_capacity{0U};
std::atomic<std::size_t> proposal_last_window_radius{0U};

}  // namespace

void configure_fake_gpu_phase14_facet_top_k_proposal(
    FakePhase14FacetTopKProposalConfiguration configuration) noexcept {
  proposal_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_phase14_facet_top_k_proposal() noexcept {
  configure_fake_gpu_phase14_facet_top_k_proposal(
      FakePhase14FacetTopKProposalConfiguration{});
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_epoch_advance_count.store(0U, std::memory_order_relaxed);
  proposal_last_query_count.store(0U, std::memory_order_relaxed);
  proposal_last_record_capacity.store(0U, std::memory_order_relaxed);
  proposal_last_window_radius.store(0U, std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_facet_top_k_proposal_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_facet_top_k_proposal_epoch_advance_count() noexcept {
  return proposal_epoch_advance_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_query_count() noexcept {
  return proposal_last_query_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_record_capacity() noexcept {
  return proposal_last_record_capacity.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_window_radius() noexcept {
  return proposal_last_window_radius.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

using Corruption =
    test_support::FakePhase14FacetTopKProposalCorruption;
using DeviceRecord = Phase14FacetTopKProposalDeviceRecord;
using InputRecord = Phase14FacetTopKProposalQueryInputRecord;

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value, const char* message) {
  if (!std::in_range<std::size_t>(value)) {
    throw std::logic_error(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::size_t inspection_bound(
    std::size_t position,
    std::size_t point_count,
    std::size_t window_radius) {
  const std::size_t left = std::min(position, window_radius);
  const std::size_t right = std::min(
      point_count - 1U - position, window_radius);
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(
        "the fake Phase 14 Morton inspection count overflowed");
  }
  return left + right;
}

[[nodiscard]] bool source_contains(
    const InputRecord& query,
    std::size_t source_count,
    std::uint64_t point_id) {
  return std::find(
             std::begin(query.point_ids),
             std::begin(query.point_ids) +
                 static_cast<std::ptrdiff_t>(source_count),
             point_id) !=
         std::begin(query.point_ids) +
             static_cast<std::ptrdiff_t>(source_count);
}

[[nodiscard]] bool candidate_inside_any_window(
    std::uint64_t candidate,
    const InputRecord& query,
    std::size_t source_count,
    std::span<const std::size_t> inverse_morton,
    std::size_t window_radius) {
  const std::size_t candidate_position =
      inverse_morton[static_cast<std::size_t>(candidate)];
  for (std::size_t source = 0U; source < source_count; ++source) {
    const std::size_t source_position =
        checked_size(
            query.morton_positions[source],
            "a fake Phase 14 source Morton position does not fit size_t");
    const std::size_t separation =
        candidate_position > source_position
            ? candidate_position - source_position
            : source_position - candidate_position;
    if (separation != 0U && separation <= window_radius) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<std::uint64_t> deterministic_candidates(
    const InputRecord& query,
    std::size_t source_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::size_t window_radius) {
  std::vector<std::uint64_t> candidates;
  for (std::size_t source = 0U; source < source_count; ++source) {
    const std::size_t position = checked_size(
        query.morton_positions[source],
        "a fake Phase 14 source Morton position does not fit size_t");
    const std::size_t begin =
        position > window_radius ? position - window_radius : 0U;
    const std::size_t end = std::min(
        morton_point_ids.size() - 1U, position + window_radius);
    for (std::size_t neighbor = begin; neighbor <= end; ++neighbor) {
      if (neighbor == position) {
        continue;
      }
      const std::uint64_t point_id = morton_point_ids[neighbor];
      if (!source_contains(query, source_count, point_id)) {
        candidates.push_back(point_id);
      }
    }
  }
  std::sort(candidates.begin(), candidates.end());
  candidates.erase(
      std::unique(candidates.begin(), candidates.end()),
      candidates.end());
  if (candidates.size() > source_count) {
    candidates.resize(source_count);
  }
  return candidates;
}

void require_inputs(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const InputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t window_radius) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / 3U ||
      coordinate_bits.size() != 3U * point_count ||
      morton_point_ids.size() != point_count || queries.empty() ||
      queries.size() > maximum_query_count || window_radius == 0U) {
    throw std::logic_error(
        "the fake Phase 14 launcher received malformed extents");
  }
}

}  // namespace

Phase14FacetTopKProposalDeviceBatch
propose_phase14_facet_top_k_candidates_on_gpu(
    Phase14FacetTopKProposalContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const Phase14FacetTopKProposalQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t morton_window_radius) {
  require_inputs(
      coordinate_bits,
      point_count,
      morton_point_ids,
      queries,
      maximum_query_count,
      morton_window_radius);

  const Corruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  test_support::proposal_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  test_support::proposal_last_query_count.store(
      queries.size(), std::memory_order_relaxed);
  test_support::proposal_last_record_capacity.store(
      maximum_query_count, std::memory_order_relaxed);
  test_support::proposal_last_window_radius.store(
      morton_window_radius, std::memory_order_relaxed);
  if (corruption == Corruption::simulated_async_failure) {
    throw std::runtime_error(
        "simulated asynchronous Phase 14 GPU failure");
  }

  std::vector<std::size_t> inverse_morton(point_count, point_count);
  for (std::size_t position = 0U;
       position < morton_point_ids.size();
       ++position) {
    const std::uint64_t point_id = morton_point_ids[position];
    if (point_id >= static_cast<std::uint64_t>(point_count) ||
        inverse_morton[static_cast<std::size_t>(point_id)] != point_count) {
      throw std::logic_error(
          "the fake Phase 14 Morton order is not a permutation");
    }
    inverse_morton[static_cast<std::size_t>(point_id)] = position;
  }

  Phase14FacetTopKProposalDeviceBatch batch;
  batch.records.resize(queries.size());
  batch.record_count = queries.size();
  batch.host_to_device_query_byte_count = queries.size_bytes();
  batch.initialized_output_byte_count =
      queries.size() * sizeof(DeviceRecord);
  batch.device_to_host_record_byte_count =
      batch.initialized_output_byte_count;
  batch.kernel_launch_count = 1U;
  batch.synchronization_count = 1U;
  if (corruption == Corruption::stale_epoch_without_advance) {
    batch.buffer_epoch = context.current_epoch();
  } else {
    const std::uint64_t advanced = context.advance_epoch();
    test_support::proposal_epoch_advance_count.fetch_add(
      1U, std::memory_order_relaxed);
    if (corruption == Corruption::zero_epoch) {
      batch.buffer_epoch = 0U;
    } else if (corruption == Corruption::jumped_epoch) {
      batch.buffer_epoch = advanced + UINT64_C(16);
    } else {
      batch.buffer_epoch = advanced;
    }
  }

  for (std::size_t record_index = 0U;
       record_index < queries.size();
       ++record_index) {
    const InputRecord& query = queries[record_index];
    const std::size_t source_count = checked_size(
        query.point_count,
        "a fake Phase 14 source count does not fit size_t");
    if (source_count == 0U ||
        source_count >
            phase14_facet_top_k_proposal_maximum_point_count) {
      throw std::logic_error(
          "the fake Phase 14 query has an invalid source count");
    }

    DeviceRecord& record = batch.records[record_index];
    record.query_index = query.query_index;
    record.key_fingerprint = query.key_fingerprint;
    record.buffer_epoch = batch.buffer_epoch;
    record.failure_code = static_cast<std::uint64_t>(
        Phase14FacetTopKProposalFailureCode::none);

    std::size_t inspected = 0U;
    for (std::size_t source = 0U; source < source_count; ++source) {
      const std::size_t position = checked_size(
          query.morton_positions[source],
          "a fake Phase 14 source Morton position does not fit size_t");
      if (position >= point_count ||
          query.point_ids[source] != morton_point_ids[position]) {
        throw std::logic_error(
            "the fake Phase 14 source is not at its declared Morton position");
      }
      const std::size_t next = inspection_bound(
          position, point_count, morton_window_radius);
      if (next > std::numeric_limits<std::size_t>::max() - inspected) {
        throw std::overflow_error(
            "the fake Phase 14 per-query inspection count overflowed");
      }
      inspected += next;
    }

    const std::vector<std::uint64_t> candidates =
        deterministic_candidates(
            query,
            source_count,
            morton_point_ids,
            morton_window_radius);
    record.candidate_count =
        static_cast<std::uint64_t>(candidates.size());
    record.inspected_neighbor_count =
        static_cast<std::uint64_t>(inspected);
    record.floating_distance_evaluation_count =
        static_cast<std::uint64_t>(inspected);
    record.floating_rejection_count =
        static_cast<std::uint64_t>(inspected - candidates.size());
    std::copy(
        candidates.begin(), candidates.end(), std::begin(record.candidates));
  }

  switch (corruption) {
    case Corruption::none:
    case Corruption::zero_epoch:
    case Corruption::stale_epoch_without_advance:
    case Corruption::jumped_epoch:
    case Corruption::simulated_async_failure:
      break;
    case Corruption::wrong_active_transfer_extent:
      ++batch.device_to_host_record_byte_count;
      break;
    case Corruption::duplicate_query_index:
      if (batch.record_count < 2U) {
        throw std::logic_error(
            "duplicate-query corruption requires two records");
      }
      batch.records[1U].query_index = batch.records[0U].query_index;
      break;
    case Corruption::wrong_key_fingerprint:
      batch.records[0U].key_fingerprint ^= UINT64_C(1);
      break;
    case Corruption::stale_active_candidate_tail: {
      DeviceRecord& record = batch.records[0U];
      const std::size_t candidate_count = checked_size(
          record.candidate_count,
          "a fake Phase 14 candidate count does not fit size_t");
      if (candidate_count >=
          phase14_facet_top_k_proposal_maximum_point_count) {
        throw std::logic_error(
            "active-tail corruption requires one unused candidate slot");
      }
      record.candidates[candidate_count] = 0U;
      break;
    }
    case Corruption::duplicate_candidate: {
      DeviceRecord& record = batch.records[0U];
      if (queries[0U].point_count < 2U ||
          record.candidate_count == 0U) {
        throw std::logic_error(
            "duplicate-candidate corruption requires rank two and a candidate");
      }
      record.candidate_count = 2U;
      record.candidates[1U] = record.candidates[0U];
      break;
    }
    case Corruption::out_of_domain_candidate: {
      DeviceRecord& record = batch.records[0U];
      record.candidate_count = 1U;
      record.candidates[0U] =
          static_cast<std::uint64_t>(point_count);
      break;
    }
    case Corruption::out_of_window_candidate: {
      const InputRecord& query = queries[0U];
      const std::size_t source_count = checked_size(
          query.point_count,
          "a fake Phase 14 source count does not fit size_t");
      std::uint64_t invalid_candidate =
          phase14_facet_top_k_proposal_sentinel;
      for (std::size_t point = 0U; point < point_count; ++point) {
        const std::uint64_t candidate =
            static_cast<std::uint64_t>(point);
        if (!source_contains(query, source_count, candidate) &&
            !candidate_inside_any_window(
                candidate,
                query,
                source_count,
                inverse_morton,
                morton_window_radius)) {
          invalid_candidate = candidate;
          break;
        }
      }
      if (invalid_candidate ==
          phase14_facet_top_k_proposal_sentinel) {
        throw std::logic_error(
            "out-of-window corruption needs a remote point");
      }
      DeviceRecord& record = batch.records[0U];
      record.candidate_count = 1U;
      record.candidates[0U] = invalid_candidate;
      break;
    }
    case Corruption::counter_overrun:
      ++batch.records[0U].floating_distance_evaluation_count;
      break;
    case Corruption::counter_partition_mismatch:
      if (batch.records[0U].floating_distance_evaluation_count == 0U) {
        throw std::logic_error(
            "counter-partition corruption requires one distance");
      }
      --batch.records[0U].floating_distance_evaluation_count;
      break;
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
