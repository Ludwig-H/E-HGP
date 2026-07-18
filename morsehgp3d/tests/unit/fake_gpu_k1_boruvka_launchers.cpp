#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "phase5_k1_boruvka_internal.hpp"

#include <algorithm>
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

std::atomic<FakeK1BoruvkaCorruption> proposal_corruption{
    FakeK1BoruvkaCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
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
  proposal_last_point_count.store(0U, std::memory_order_relaxed);
  proposal_last_node_count.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_k1_boruvka_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
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

}  // namespace

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

  switch (corruption) {
    case test_support::FakeK1BoruvkaCorruption::none:
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

}  // namespace morsehgp3d::gpu::detail
