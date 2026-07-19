#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "phase5_k1_boruvka_internal.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cfenv>
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

std::atomic<FakeK1BoruvkaCorruption> proposal_corruption{
    FakeK1BoruvkaCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_chunk_callback_count{0U};
std::atomic<std::size_t> proposal_epoch_advance_count{0U};
std::atomic<std::size_t> proposal_budget_enforcement_count{0U};
std::atomic<std::size_t> proposal_last_point_count{0U};
std::atomic<std::size_t> proposal_last_node_count{0U};

}  // namespace

void configure_fake_gpu_k1_boruvka(
    FakeK1BoruvkaConfiguration configuration) noexcept {
  proposal_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_k1_boruvka() noexcept {
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_chunk_callback_count.store(0U, std::memory_order_relaxed);
  proposal_epoch_advance_count.store(0U, std::memory_order_relaxed);
  proposal_budget_enforcement_count.store(0U, std::memory_order_relaxed);
  proposal_last_point_count.store(0U, std::memory_order_relaxed);
  proposal_last_node_count.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_chunk_callback_count() noexcept {
  return proposal_chunk_callback_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_epoch_advance_count() noexcept {
  return proposal_epoch_advance_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_budget_enforcement_count() noexcept {
  return proposal_budget_enforcement_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_last_point_count() noexcept {
  return proposal_last_point_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_last_node_count() noexcept {
  return proposal_last_node_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

struct DirectedLowerBound {
  double value{};
  bool valid{true};
};

struct TraversalCounters {
  std::size_t node_visits{};
  std::size_t uniform_component_prunes{};
  std::size_t strict_aabb_prunes{};
  std::size_t invalid_bound_descents{};
};

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] double subtract_down_nonnegative(
    double left, double right, bool& valid) {
  if (left == right) {
    return 0.0;
  }
  const double rounded = left - right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    valid = false;
    return 0.0;
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] double square_down_nonnegative(
    double value, bool& valid) {
  if (value == 0.0) {
    return 0.0;
  }
  const double rounded = value * value;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    valid = false;
    return 0.0;
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] double add_down_nonnegative(
    double left, double right, bool& valid) {
  const double rounded = left + right;
  if (!std::isfinite(rounded) || rounded < 0.0) {
    valid = false;
    return 0.0;
  }
  return rounded == 0.0
             ? 0.0
             : std::nextafter(
                   rounded, -std::numeric_limits<double>::infinity());
}

[[nodiscard]] DirectedLowerBound directed_lower_bound(
    const K1BoruvkaNodeInputRecord& node,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::size_t source_index) {
  DirectedLowerBound result;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double query = std::bit_cast<double>(
        coordinate_bits[axis * point_count + source_index]);
    const double lower = std::bit_cast<double>(node.lower_bits[axis]);
    const double upper = std::bit_cast<double>(node.upper_bits[axis]);
    if (!std::isfinite(query) || !std::isfinite(lower) ||
        !std::isfinite(upper) || lower > upper) {
      result.valid = false;
      return result;
    }

    double delta = 0.0;
    if (query < lower) {
      delta = subtract_down_nonnegative(lower, query, result.valid);
    } else if (query > upper) {
      delta = subtract_down_nonnegative(query, upper, result.valid);
    }
    if (!result.valid) {
      return result;
    }
    const double squared = square_down_nonnegative(delta, result.valid);
    if (!result.valid) {
      return result;
    }
    result.value =
        add_down_nonnegative(result.value, squared, result.valid);
    if (!result.valid) {
      return result;
    }
  }
  return result;
}

[[nodiscard]] std::size_t checked_index(
    std::uint64_t value, std::size_t upper_bound, const char* label) {
  if (!std::in_range<std::size_t>(value) ||
      static_cast<std::size_t>(value) >= upper_bound) {
    throw std::logic_error(label);
  }
  return static_cast<std::size_t>(value);
}

template <typename Emit>
[[nodiscard]] std::size_t traverse_source(
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits,
    std::size_t source_index,
    TraversalCounters& counters,
    Emit&& emit) {
  const std::uint64_t source_label = frozen_component_labels[source_index];
  const double cutoff =
      std::bit_cast<double>(seed_cutoff_upper_bits[source_index]);
  if (std::isnan(cutoff) || cutoff < 0.0) {
    throw std::logic_error(
        "the fake Phase 5 seed cutoff is not a nonnegative enclosure");
  }

  std::size_t candidate_count = 0U;
  std::uint64_t current = static_cast<std::uint64_t>(root_index);
  std::size_t steps = 0U;
  while (current != k1_boruvka_sentinel) {
    if (++steps > nodes.size()) {
      throw std::logic_error(
          "the fake Phase 5 rope traversal exceeded its node count");
    }
    const std::size_t node_index = checked_index(
        current, nodes.size(),
        "the fake Phase 5 rope reached an invalid node");
    ++counters.node_visits;
    const K1BoruvkaNodeInputRecord& node = nodes[node_index];
    const std::uint64_t escape = node.escape;
    if (escape != k1_boruvka_sentinel) {
      static_cast<void>(checked_index(
          escape, nodes.size(),
          "the fake Phase 5 rope has an invalid escape"));
    }

    const std::uint64_t tag = node_component_tags[node_index];
    if (tag == source_label) {
      ++counters.uniform_component_prunes;
      current = escape;
      continue;
    }
    if (tag != k1_boruvka_mixed_component && tag >= point_count) {
      throw std::logic_error(
          "the fake Phase 5 rope has an invalid component tag");
    }

    const DirectedLowerBound lower = directed_lower_bound(
        node, coordinate_bits, point_count, source_index);
    if (!lower.valid) {
      ++counters.invalid_bound_descents;
    } else if (lower.value > cutoff) {
      ++counters.strict_aabb_prunes;
      current = escape;
      continue;
    }

    const bool leaf = node.left_child == k1_boruvka_sentinel &&
                      node.right_child == k1_boruvka_sentinel;
    if (leaf) {
      const std::size_t target_index = checked_index(
          node.leaf_point_id, point_count,
          "the fake Phase 5 leaf has an invalid PointId");
      if (target_index == source_index ||
          frozen_component_labels[target_index] == source_label) {
        throw std::logic_error(
            "the fake Phase 5 traversal reached an unpruned same-component leaf");
      }
      emit(K1BoruvkaCandidateRecord{
          static_cast<std::uint64_t>(source_index),
          static_cast<std::uint64_t>(target_index)});
      ++candidate_count;
      current = escape;
      continue;
    }
    if (node.left_child == k1_boruvka_sentinel ||
        node.right_child == k1_boruvka_sentinel ||
        node.leaf_point_id != k1_boruvka_sentinel) {
      throw std::logic_error(
          "the fake Phase 5 rope contains a malformed internal node");
    }
    static_cast<void>(checked_index(
        node.left_child, nodes.size(),
        "the fake Phase 5 rope has an invalid left child"));
    static_cast<void>(checked_index(
        node.right_child, nodes.size(),
        "the fake Phase 5 rope has an invalid right child"));
    current = node.left_child;
  }
  return candidate_count;
}

void remove_first_source_segment(K1BoruvkaCandidateBatch& batch) {
  if (batch.candidate_offsets.size() < 2U) {
    throw std::logic_error(
        "missing-candidate corruption needs a nonempty CSR offset array");
  }
  const std::size_t removed = checked_index(
      batch.candidate_offsets[1], batch.records.size() + 1U,
      "missing-candidate corruption found an invalid first offset");
  if (removed == 0U) {
    throw std::logic_error(
        "missing-candidate corruption needs a candidate for source zero");
  }
  batch.records.erase(
      batch.records.begin(),
      batch.records.begin() +
          static_cast<std::vector<K1BoruvkaCandidateRecord>::difference_type>(
              removed));
  for (std::size_t index = 1U;
       index < batch.candidate_offsets.size();
       ++index) {
    batch.candidate_offsets[index] -= removed;
  }
  batch.output_capacity = batch.records.size();
}

[[nodiscard]] double morton_seed_squared_distance(
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::size_t source_index,
    std::size_t target_index) {
  volatile double squared_distance = 0.0;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const double source = std::bit_cast<double>(
        coordinate_bits[axis * point_count + source_index]);
    const double target = std::bit_cast<double>(
        coordinate_bits[axis * point_count + target_index]);
    if (!std::isfinite(source) || !std::isfinite(target)) {
      throw std::logic_error(
          "the fake Phase 5 Morton seed input is not finite");
    }
    const volatile double difference = source - target;
    const volatile double squared = difference * difference;
    const volatile double next = squared_distance + squared;
    if (std::isnan(difference) || std::isnan(squared) ||
        std::isnan(next) || squared < 0.0 || next < 0.0) {
      throw std::logic_error(
          "the fake Phase 5 Morton seed distance is invalid");
    }
    squared_distance = next;
  }
  return squared_distance;
}

void checked_add_seed_count(
    std::size_t& total, std::uint64_t increment, const char* message) {
  if (!std::in_range<std::size_t>(increment) ||
      static_cast<std::size_t>(increment) >
          std::numeric_limits<std::size_t>::max() - total) {
    throw std::overflow_error(message);
  }
  total += static_cast<std::size_t>(increment);
}

}  // namespace

K1BoruvkaMortonSeedProposalBatch
propose_k1_boruvka_morton_seeds_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const std::uint64_t> frozen_component_labels,
    std::size_t window_radius) {
  static_cast<void>(context);
  if (point_count == 0U || nodes.empty() || root_index >= nodes.size() ||
      point_count > std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount ||
      morton_point_ids.size() != point_count ||
      frozen_component_labels.size() != point_count ||
      window_radius == 0U ||
      window_radius > std::numeric_limits<std::size_t>::max() / 2U) {
    throw std::invalid_argument(
        "invalid fake Phase 5 Morton seed proposal input");
  }
  if (std::fegetround() != FE_TONEAREST) {
    throw std::runtime_error(
        "the fake Phase 5 Morton seed proposal requires round-to-nearest");
  }

  std::vector<std::size_t> morton_positions(
      point_count, std::numeric_limits<std::size_t>::max());
  for (std::size_t position = 0U; position < point_count; ++position) {
    const std::size_t point_index = checked_index(
        morton_point_ids[position], point_count,
        "the fake Phase 5 Morton order has an invalid PointId");
    if (morton_positions[point_index] !=
        std::numeric_limits<std::size_t>::max()) {
      throw std::logic_error(
          "the fake Phase 5 Morton order is not a permutation");
    }
    morton_positions[point_index] = position;
  }
  for (const std::uint64_t label : frozen_component_labels) {
    static_cast<void>(checked_index(
        label, point_count,
        "the fake Phase 5 Morton seed has an invalid component label"));
  }
  for (const std::uint64_t bits : coordinate_bits) {
    if (!std::isfinite(std::bit_cast<double>(bits))) {
      throw std::logic_error(
          "the fake Phase 5 Morton seed input is not finite");
    }
  }

  test_support::proposal_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  test_support::proposal_last_point_count.store(
      point_count, std::memory_order_relaxed);
  test_support::proposal_last_node_count.store(
      nodes.size(), std::memory_order_relaxed);

  K1BoruvkaMortonSeedProposalBatch batch;
  batch.records.resize(point_count);
  batch.window_radius = window_radius;
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    K1BoruvkaMortonSeedProposalRecord& record =
        batch.records[source_index];
    const std::size_t source_position = morton_positions[source_index];
    const std::size_t lower =
        source_position > window_radius
            ? source_position - window_radius
            : 0U;
    const std::size_t upper = std::min(
        point_count - 1U,
        window_radius > point_count - 1U - source_position
            ? point_count - 1U
            : source_position + window_radius);

    double best_distance = std::numeric_limits<double>::infinity();
    for (std::size_t position = lower; position <= upper; ++position) {
      if (position == source_position) {
        continue;
      }
      if (record.inspected_neighbor_count ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error(
            "the fake Phase 5 Morton inspected-neighbor count overflowed");
      }
      ++record.inspected_neighbor_count;

      const std::size_t target_index = checked_index(
          morton_point_ids[position], point_count,
          "the fake Phase 5 Morton order changed after validation");
      if (frozen_component_labels[target_index] ==
          frozen_component_labels[source_index]) {
        continue;
      }
      if (record.external_neighbor_count ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error(
            "the fake Phase 5 Morton external-neighbor count overflowed");
      }
      ++record.external_neighbor_count;

      const double distance = morton_seed_squared_distance(
          coordinate_bits, point_count, source_index, target_index);
      const std::uint64_t target_id =
          static_cast<std::uint64_t>(target_index);
      if (record.target_point_id == k1_boruvka_sentinel ||
          distance < best_distance ||
          (distance == best_distance &&
           target_id < record.target_point_id)) {
        best_distance = distance;
        record.target_point_id = target_id;
      }
    }

    checked_add_seed_count(
        batch.inspected_neighbor_count,
        record.inspected_neighbor_count,
        "the fake Phase 5 Morton inspected-neighbor total overflowed");
    checked_add_seed_count(
        batch.external_neighbor_count,
        record.external_neighbor_count,
        "the fake Phase 5 Morton external-neighbor total overflowed");
    if (record.target_point_id != k1_boruvka_sentinel) {
      if (batch.proposed_seed_count ==
          std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error(
            "the fake Phase 5 Morton proposed-seed count overflowed");
      }
      ++batch.proposed_seed_count;
    }
  }

  batch.kernel_launch_count = 1U;
  batch.synchronization_count = 1U;
  batch.complete_source_coverage = true;
  batch.bounded_window = true;
  return batch;
}

K1BoruvkaCandidateBatch propose_k1_boruvka_candidates_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits) {
  if (point_count == 0U || nodes.empty() || root_index >= nodes.size() ||
      point_count > std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount ||
      frozen_component_labels.size() != point_count ||
      node_component_tags.size() != nodes.size() ||
      seed_cutoff_upper_bits.size() != point_count) {
    throw std::invalid_argument(
        "invalid fake Phase 5 K1 Boruvka proposal input");
  }

  const test_support::FakeK1BoruvkaCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  if (corruption ==
      test_support::FakeK1BoruvkaCorruption::simulated_gpu_failure) {
    throw std::runtime_error("simulated Phase 5 K1 Boruvka GPU failure");
  }

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_point_count.store(
      point_count, std::memory_order_relaxed);
  test_support::proposal_last_node_count.store(
      nodes.size(), std::memory_order_relaxed);

  K1BoruvkaCandidateBatch batch;
  batch.candidate_offsets.resize(point_count + 1U, 0U);
  TraversalCounters count_counters;
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const std::size_t count = traverse_source(
        nodes,
        root_index,
        coordinate_bits,
        point_count,
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits,
        source_index,
        count_counters,
        [](const K1BoruvkaCandidateRecord&) {});
    const std::uint64_t previous = batch.candidate_offsets[source_index];
    if (count >
        static_cast<std::size_t>(
            std::numeric_limits<std::uint64_t>::max() - previous)) {
      throw std::overflow_error(
          "the fake Phase 5 candidate count exceeds uint64_t");
    }
    batch.candidate_offsets[source_index + 1U] =
        previous + static_cast<std::uint64_t>(count);
  }
  if (!std::in_range<std::size_t>(batch.candidate_offsets.back())) {
    throw std::overflow_error(
        "the fake Phase 5 candidate capacity exceeds size_t");
  }
  batch.output_capacity =
      static_cast<std::size_t>(batch.candidate_offsets.back());
  batch.records.reserve(batch.output_capacity);

  TraversalCounters emit_counters;
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const std::size_t segment_begin = batch.records.size();
    const std::size_t emitted = traverse_source(
        nodes,
        root_index,
        coordinate_bits,
        point_count,
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits,
        source_index,
        emit_counters,
        [&batch](const K1BoruvkaCandidateRecord& record) {
          batch.records.push_back(record);
        });
    const std::size_t expected_begin = static_cast<std::size_t>(
        batch.candidate_offsets[source_index]);
    const std::size_t expected_end = static_cast<std::size_t>(
        batch.candidate_offsets[source_index + 1U]);
    if (segment_begin != expected_begin ||
        emitted != expected_end - expected_begin ||
        batch.records.size() != expected_end) {
      throw std::logic_error(
          "the fake Phase 5 count and emit passes disagree");
    }
  }
  if (batch.records.size() != batch.output_capacity) {
    throw std::logic_error(
        "the fake Phase 5 emit pass did not fill its exact capacity");
  }

  batch.kernel_launch_count = 2U;
  batch.synchronization_count = 2U;
  batch.count_pass_node_visit_count = count_counters.node_visits;
  batch.emit_pass_node_visit_count = emit_counters.node_visits;
  batch.uniform_component_prune_count =
      count_counters.uniform_component_prunes;
  batch.strict_aabb_prune_count = count_counters.strict_aabb_prunes;
  batch.invalid_bound_descent_count = count_counters.invalid_bound_descents;
  batch.exact_capacity = true;
  batch.no_truncation = true;
  batch.buffer_epoch = context.advance_epoch();
  context.set_candidate_capacity_hint(std::max(
      context.candidate_capacity_hint(), batch.records.size()));
  test_support::proposal_epoch_advance_count.fetch_add(
      1U, std::memory_order_relaxed);

  switch (corruption) {
    case test_support::FakeK1BoruvkaCorruption::none:
    case test_support::FakeK1BoruvkaCorruption::missing_late_chunk_candidate:
      break;
    case test_support::FakeK1BoruvkaCorruption::offset_count_mismatch:
      ++batch.candidate_offsets.back();
      break;
    case test_support::FakeK1BoruvkaCorruption::missing_outgoing_candidate:
      remove_first_source_segment(batch);
      break;
    case test_support::FakeK1BoruvkaCorruption::same_component_target:
      if (batch.records.empty()) {
        throw std::logic_error(
            "same-component corruption needs at least one candidate");
      }
      batch.records.front().target_point_id =
          batch.records.front().source_point_id;
      break;
    case test_support::FakeK1BoruvkaCorruption::out_of_range_target:
      if (batch.records.empty()) {
        throw std::logic_error(
            "out-of-range corruption needs at least one candidate");
      }
      batch.records.front().target_point_id =
          static_cast<std::uint64_t>(point_count);
      break;
    case test_support::FakeK1BoruvkaCorruption::simulated_gpu_failure:
      throw std::logic_error(
          "the simulated Phase 5 failure must precede publication");
  }
  return batch;
}

std::size_t enforce_k1_boruvka_candidate_budget_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::size_t candidate_record_budget) {
  if (candidate_record_budget == 0U) {
    throw std::invalid_argument(
        "the fake Phase 5 candidate budget must be positive");
  }
  test_support::proposal_budget_enforcement_count.fetch_add(
      1U, std::memory_order_relaxed);
  if (context.candidate_capacity_hint() > candidate_record_budget) {
    context.set_candidate_capacity_hint(0U);
  }
  return context.candidate_capacity_hint();
}

K1BoruvkaChunkedCandidateSummary
propose_k1_boruvka_candidates_chunked_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits,
    std::size_t candidate_record_budget,
    const K1BoruvkaCandidateChunkConsumer& consume_chunk) {
  if (point_count == 0U || nodes.empty() || root_index >= nodes.size() ||
      point_count > std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount ||
      frozen_component_labels.size() != point_count ||
      node_component_tags.size() != nodes.size() ||
      seed_cutoff_upper_bits.size() != point_count ||
      candidate_record_budget == 0U || !consume_chunk) {
    throw std::invalid_argument(
        "invalid fake Phase 5 chunked K1 Boruvka proposal input");
  }

  const test_support::FakeK1BoruvkaCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);
  if (corruption ==
      test_support::FakeK1BoruvkaCorruption::simulated_gpu_failure) {
    throw std::runtime_error(
        "simulated Phase 5 chunked K1 Boruvka GPU failure");
  }

  if (context.candidate_capacity_hint() > candidate_record_budget) {
    context.set_candidate_capacity_hint(0U);
  }

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_point_count.store(
      point_count, std::memory_order_relaxed);
  test_support::proposal_last_node_count.store(
      nodes.size(), std::memory_order_relaxed);

  std::vector<std::uint64_t> global_offsets(point_count + 1U, 0U);
  TraversalCounters count_counters;
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const std::size_t count = traverse_source(
        nodes,
        root_index,
        coordinate_bits,
        point_count,
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits,
        source_index,
        count_counters,
        [](const K1BoruvkaCandidateRecord&) {});
    if (count > candidate_record_budget) {
      throw std::invalid_argument(
          "the fake Phase 5 candidate budget cannot hold one complete source");
    }
    const std::uint64_t previous = global_offsets[source_index];
    if (count > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max() - previous)) {
      throw std::overflow_error(
          "the fake Phase 5 chunked candidate count exceeds uint64_t");
    }
    global_offsets[source_index + 1U] =
        previous + static_cast<std::uint64_t>(count);
  }
  if (!std::in_range<std::size_t>(global_offsets.back())) {
    throw std::overflow_error(
        "the fake Phase 5 logical candidate count exceeds size_t");
  }

  K1BoruvkaChunkedCandidateSummary summary;
  summary.logical_candidate_count =
      static_cast<std::size_t>(global_offsets.back());
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    summary.max_source_candidate_count = std::max(
        summary.max_source_candidate_count,
        static_cast<std::size_t>(
            global_offsets[source_index + 1U] -
            global_offsets[source_index]));
  }
  summary.candidate_record_budget = candidate_record_budget;
  summary.count_pass_node_visit_count = count_counters.node_visits;
  summary.uniform_component_prune_count =
      count_counters.uniform_component_prunes;
  summary.strict_aabb_prune_count = count_counters.strict_aabb_prunes;
  summary.invalid_bound_descent_count =
      count_counters.invalid_bound_descents;

  std::uint64_t digest = kFnvOffsetBasis;
  for (const std::uint64_t offset : global_offsets) {
    hash_word(digest, offset);
  }

  TraversalCounters emit_counters;
  std::vector<std::uint64_t> chunk_offsets;
  std::vector<K1BoruvkaCandidateRecord> chunk_records;
  std::size_t source_begin = 0U;
  bool late_corruption_applied = false;
  while (source_begin < point_count) {
    std::size_t source_end = source_begin;
    std::size_t chunk_candidate_count = 0U;
    while (source_end < point_count) {
      const std::size_t source_count = static_cast<std::size_t>(
          global_offsets[source_end + 1U] - global_offsets[source_end]);
      if (source_count > candidate_record_budget - chunk_candidate_count) {
        break;
      }
      chunk_candidate_count += source_count;
      ++source_end;
    }
    if (source_end == source_begin) {
      throw std::logic_error(
          "the fake Phase 5 greedy chunk planner made no progress");
    }

    chunk_offsets.clear();
    chunk_records.clear();
    chunk_offsets.reserve(source_end - source_begin + 1U);
    chunk_records.reserve(chunk_candidate_count);
    chunk_offsets.push_back(0U);
    for (std::size_t source_index = source_begin;
         source_index < source_end;
         ++source_index) {
      const std::size_t segment_begin = chunk_records.size();
      const std::size_t emitted = traverse_source(
          nodes,
          root_index,
          coordinate_bits,
          point_count,
          frozen_component_labels,
          node_component_tags,
          seed_cutoff_upper_bits,
          source_index,
          emit_counters,
          [&chunk_records](const K1BoruvkaCandidateRecord& record) {
            chunk_records.push_back(record);
          });
      const std::size_t expected = static_cast<std::size_t>(
          global_offsets[source_index + 1U] -
          global_offsets[source_index]);
      if (segment_begin != chunk_offsets.back() || emitted != expected ||
          chunk_records.size() - segment_begin != expected) {
        throw std::logic_error(
            "the fake Phase 5 chunked count and emit passes disagree");
      }
      chunk_offsets.push_back(
          static_cast<std::uint64_t>(chunk_records.size()));
    }
    if (chunk_records.size() != chunk_candidate_count) {
      throw std::logic_error(
          "the fake Phase 5 chunked emit pass missed its exact capacity");
    }

    const bool corrupt_this_chunk =
        corruption == test_support::FakeK1BoruvkaCorruption::
                          missing_late_chunk_candidate &&
        source_begin != 0U && !late_corruption_applied;
    if (corrupt_this_chunk) {
      if (chunk_records.empty()) {
        throw std::logic_error(
            "late chunk corruption needs a nonempty candidate payload");
      }
      chunk_records.pop_back();
      late_corruption_applied = true;
    }

    test_support::proposal_chunk_callback_count.fetch_add(
        1U, std::memory_order_relaxed);
    consume_chunk(K1BoruvkaCandidateChunkView{
        source_begin,
        source_end,
        std::span<const std::uint64_t>{chunk_offsets},
        std::span<const K1BoruvkaCandidateRecord>{chunk_records}});
    if (corrupt_this_chunk) {
      throw std::runtime_error(
          "simulated Phase 5 late chunk candidate loss");
    }

    for (const K1BoruvkaCandidateRecord& record : chunk_records) {
      hash_word(digest, record.source_point_id);
      hash_word(digest, record.target_point_id);
    }
    ++summary.source_chunk_count;
    summary.peak_chunk_source_count = std::max(
        summary.peak_chunk_source_count, source_end - source_begin);
    summary.peak_chunk_candidate_count = std::max(
        summary.peak_chunk_candidate_count, chunk_candidate_count);
    summary.device_candidate_capacity_high_water = std::max(
        summary.device_candidate_capacity_high_water,
        chunk_candidate_count);
    summary.host_candidate_capacity_high_water = std::max(
        summary.host_candidate_capacity_high_water,
        chunk_candidate_count);
    source_begin = source_end;
  }

  context.set_candidate_capacity_hint(std::max(
      context.candidate_capacity_hint(),
      summary.peak_chunk_candidate_count));
  summary.device_candidate_capacity_high_water =
      context.candidate_capacity_hint();

  summary.kernel_launch_count = 1U + summary.source_chunk_count;
  summary.synchronization_count = summary.kernel_launch_count;
  summary.emit_pass_node_visit_count = emit_counters.node_visits;
  if (emit_counters.node_visits != count_counters.node_visits ||
      emit_counters.uniform_component_prunes !=
          count_counters.uniform_component_prunes ||
      emit_counters.strict_aabb_prunes !=
          count_counters.strict_aabb_prunes ||
      emit_counters.invalid_bound_descents !=
          count_counters.invalid_bound_descents) {
    throw std::logic_error(
        "the fake Phase 5 chunked traversal counters disagree");
  }
  summary.proposal_digest_fnv1a = digest;
  summary.complete_source_partition_certified = true;
  summary.count_emit_cardinality_and_visit_count_certified = true;
  summary.exact_capacity = true;
  summary.no_truncation = true;
  summary.buffer_epoch = context.advance_epoch();
  test_support::proposal_epoch_advance_count.fetch_add(
      1U, std::memory_order_relaxed);
  return summary;
}

}  // namespace morsehgp3d::gpu::detail
