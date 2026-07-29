#include "morsehgp3d/gpu/morton_yao48_seed_work_profile.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using Coordinate3 = std::array<double, 3>;

constexpr std::array<std::array<std::uint8_t, 3>, 6>
    kAxisPermutations{{
        {{0U, 1U, 2U}},
        {{0U, 2U, 1U}},
        {{1U, 0U, 2U}},
        {{1U, 2U, 0U}},
        {{2U, 0U, 1U}},
        {{2U, 1U, 0U}},
    }};

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

[[nodiscard]] std::uint64_t unordered_pair_count(std::size_t count) {
  const std::uint64_t value = static_cast<std::uint64_t>(count);
  if (value != count) {
    throw std::length_error("the work-profile point count exceeds uint64");
  }
  const std::uint64_t even = (value & UINT64_C(1)) == 0U ? value : value - 1U;
  const std::uint64_t other = (value & UINT64_C(1)) == 0U ? value - 1U : value;
  if (even / 2U != 0U &&
      other > std::numeric_limits<std::uint64_t>::max() / (even / 2U)) {
    throw std::length_error("the unordered-pair universe exceeds uint64");
  }
  return (even / 2U) * other;
}

[[nodiscard]] std::size_t permutation_index(
    const std::array<std::uint8_t, 3>& axes) {
  const auto found =
      std::find(kAxisPermutations.begin(), kAxisPermutations.end(), axes);
  if (found == kAxisPermutations.end()) {
    throw std::logic_error("a binary64 Yao48 axis order is not a permutation");
  }
  return static_cast<std::size_t>(
      std::distance(kAxisPermutations.begin(), found));
}

[[nodiscard]] std::size_t classify_coordinates(
    const Coordinate3& source,
    const Coordinate3& target) {
  std::array<double, 3> absolute_deltas{};
  std::array<std::uint8_t, 3> axes{{0U, 1U, 2U}};
  std::uint8_t negative_axis_mask = 0U;
  bool nonzero = false;
  for (std::size_t axis = 0U; axis < axes.size(); ++axis) {
    const double delta = target[axis] - source[axis];
    if (!std::isfinite(delta)) {
      throw std::invalid_argument(
          "a binary64 Yao48 proposal delta must be finite");
    }
    if (delta != 0.0) {
      nonzero = true;
    }
    if (delta < 0.0) {
      negative_axis_mask = static_cast<std::uint8_t>(
          negative_axis_mask |
          static_cast<std::uint8_t>(std::uint8_t{1U} << axis));
    }
    absolute_deltas[axis] = std::abs(delta);
  }
  if (!nonzero) {
    throw std::invalid_argument(
        "a binary64 Yao48 proposal requires distinct rounded points");
  }
  std::sort(
      axes.begin(),
      axes.end(),
      [&absolute_deltas](std::uint8_t left, std::uint8_t right) {
        const std::size_t left_axis = static_cast<std::size_t>(left);
        const std::size_t right_axis = static_cast<std::size_t>(right);
        if (absolute_deltas[left_axis] != absolute_deltas[right_axis]) {
          return absolute_deltas[left_axis] > absolute_deltas[right_axis];
        }
        return left < right;
      });
  return static_cast<std::size_t>(negative_axis_mask) *
             kAxisPermutations.size() +
         permutation_index(axes);
}

struct ShardSummary {
  std::size_t bank_receipt_count{};
  std::size_t nonempty_bank_count{};
  std::size_t cutoff_ready_bank_count{};
  std::array<std::size_t,
             morton_yao48_seed_work_profile_maximum_bank_capacity + 1U>
      histogram{};
  std::uint64_t survivor_pair_mass{};
};

}  // namespace

bool MortonYao48SeedWorkProfile::locally_valid() const noexcept {
  if (schema_version != morton_yao48_seed_work_profile_schema_version ||
      point_count < 2U || maximum_order == 0U ||
      maximum_order >
          morton_yao48_seed_work_profile_maximum_bank_capacity ||
      bank_capacity != maximum_order ||
      point_cone_bank_count !=
          point_count * morton_yao48_seed_work_profile_cone_count ||
      bank_receipt_count != point_count * maximum_order ||
      cutoff_ready_bank_count > nonempty_bank_count ||
      nonempty_bank_count > point_cone_bank_count ||
      survivor_pair_mass > unordered_pair_universe_count ||
      certified_pruned_pair_mass >
          unordered_pair_universe_count - survivor_pair_mass ||
      unresolved_pair_mass !=
          unordered_pair_universe_count - survivor_pair_mass -
              certified_pruned_pair_mass ||
      !morton_owner_orientation_applied ||
      !yao48_binary64_proposal_classification_applied ||
      exact_yao48_classification_claimed || exact_pair_frontier_claimed ||
      dense_pair_fallback_performed || global_pair_matrix_materialized ||
      public_status_claimed) {
    return false;
  }
  std::size_t histogram_bank_count = 0U;
  std::size_t histogram_receipt_count = 0U;
  for (std::size_t fill = 0U; fill <= bank_capacity; ++fill) {
    if (bank_fill_histogram[fill] >
        std::numeric_limits<std::size_t>::max() - histogram_bank_count) {
      return false;
    }
    histogram_bank_count += bank_fill_histogram[fill];
    if (fill != 0U && bank_fill_histogram[fill] >
                          std::numeric_limits<std::size_t>::max() / fill) {
      return false;
    }
    const std::size_t receipts = fill * bank_fill_histogram[fill];
    if (receipts >
        std::numeric_limits<std::size_t>::max() - histogram_receipt_count) {
      return false;
    }
    histogram_receipt_count += receipts;
  }
  return histogram_bank_count == point_cone_bank_count &&
         histogram_receipt_count == bank_receipt_count &&
         bank_fill_histogram[bank_capacity] == cutoff_ready_bank_count;
}

std::size_t classify_binary64_yao48_cone_proposal(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target) {
  Coordinate3 source_coordinates{};
  Coordinate3 target_coordinates{};
  for (std::size_t axis = 0U; axis < source_coordinates.size(); ++axis) {
    source_coordinates[axis] = source.binary64_coordinate(axis);
    target_coordinates[axis] = target.binary64_coordinate(axis);
  }
  return classify_coordinates(source_coordinates, target_coordinates);
}

MortonYao48SeedWorkProfile summarize_morton_yao48_seed_work_profile(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId>
        source_point_ids_by_morton_position,
    std::span<const spatial::PointId> neighbor_point_ids,
    std::size_t maximum_order,
    std::size_t worker_count) {
  const std::size_t point_count = cloud.size();
  if (point_count < 2U || point_count <= maximum_order ||
      maximum_order == 0U ||
      maximum_order >
          morton_yao48_seed_work_profile_maximum_bank_capacity ||
      worker_count == 0U ||
      source_point_ids_by_morton_position.size() != point_count ||
      neighbor_point_ids.size() != checked_product(
          point_count,
          maximum_order,
          "the work-profile neighbor extent overflows size_t")) {
    throw std::invalid_argument(
        "the Morton/Yao48 seed work profile received invalid extents");
  }

  std::vector<std::size_t> morton_position_by_point_id(point_count);
  std::vector<unsigned char> seen(point_count, 0U);
  std::vector<Coordinate3> coordinates(point_count);
  for (std::size_t position = 0U; position < point_count; ++position) {
    const spatial::PointId point_id =
        source_point_ids_by_morton_position[position];
    if (point_id >= static_cast<spatial::PointId>(point_count)) {
      throw std::invalid_argument(
          "the work-profile Morton order contains an invalid PointId");
    }
    const std::size_t point = static_cast<std::size_t>(point_id);
    if (seen[point] != 0U) {
      throw std::invalid_argument(
          "the work-profile Morton order is not a PointId permutation");
    }
    seen[point] = 1U;
    morton_position_by_point_id[point] = position;
  }
  for (std::size_t point = 0U; point < point_count; ++point) {
    for (std::size_t axis = 0U; axis < coordinates[point].size(); ++axis) {
      coordinates[point][axis] =
          cloud.point(static_cast<spatial::PointId>(point))
              .binary64_coordinate(axis);
    }
  }

  const std::size_t effective_worker_count =
      std::min(point_count, worker_count);
  std::vector<ShardSummary> shards(effective_worker_count);
  std::atomic<bool> invalid_transcript{false};
  std::vector<std::thread> workers;
  workers.reserve(effective_worker_count);
  for (std::size_t worker = 0U; worker < effective_worker_count; ++worker) {
    workers.emplace_back([&, worker] {
      ShardSummary& shard = shards[worker];
      const std::size_t begin = point_count * worker / effective_worker_count;
      const std::size_t end =
          point_count * (worker + 1U) / effective_worker_count;
      try {
        for (std::size_t position = begin; position < end; ++position) {
          std::array<std::uint8_t,
                     morton_yao48_seed_work_profile_cone_count>
              fills{};
          const spatial::PointId source_id =
              source_point_ids_by_morton_position[position];
          const std::size_t source = static_cast<std::size_t>(source_id);
          const std::size_t row = position * maximum_order;
          for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
            const spatial::PointId target_id = neighbor_point_ids[row + rank];
            if (target_id >= static_cast<spatial::PointId>(point_count) ||
                target_id == source_id) {
              throw std::invalid_argument(
                  "the work-profile transcript contains an invalid neighbor");
            }
            for (std::size_t previous = 0U; previous < rank; ++previous) {
              if (neighbor_point_ids[row + previous] == target_id) {
                throw std::invalid_argument(
                    "the work-profile transcript repeats a row neighbor");
              }
            }
            const std::size_t target = static_cast<std::size_t>(target_id);
            const std::size_t cone =
                classify_coordinates(coordinates[source], coordinates[target]);
            if (cone >= fills.size()) {
              throw std::logic_error(
                  "the binary64 Yao48 proposal cone exceeds 47");
            }
            ++fills[cone];
            ++shard.bank_receipt_count;
            if (morton_position_by_point_id[target] < position) {
              ++shard.survivor_pair_mass;
            }
          }
          for (const std::uint8_t fill : fills) {
            const std::size_t value = static_cast<std::size_t>(fill);
            ++shard.histogram[value];
            if (value != 0U) {
              ++shard.nonempty_bank_count;
            }
            if (value == maximum_order) {
              ++shard.cutoff_ready_bank_count;
            }
          }
        }
      } catch (...) {
        invalid_transcript.store(true, std::memory_order_relaxed);
      }
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
  if (invalid_transcript.load(std::memory_order_relaxed)) {
    throw std::invalid_argument(
        "the Morton/Yao48 seed transcript failed parallel validation");
  }

  MortonYao48SeedWorkProfile result;
  result.point_count = point_count;
  result.maximum_order = maximum_order;
  result.bank_capacity = maximum_order;
  result.point_cone_bank_count = checked_product(
      point_count,
      morton_yao48_seed_work_profile_cone_count,
      "the work-profile point-cone bank count overflows size_t");
  for (const ShardSummary& shard : shards) {
    result.bank_receipt_count += shard.bank_receipt_count;
    result.nonempty_bank_count += shard.nonempty_bank_count;
    result.cutoff_ready_bank_count += shard.cutoff_ready_bank_count;
    result.survivor_pair_mass += shard.survivor_pair_mass;
    for (std::size_t fill = 0U; fill <= maximum_order; ++fill) {
      result.bank_fill_histogram[fill] += shard.histogram[fill];
    }
  }
  result.unordered_pair_universe_count = unordered_pair_count(point_count);
  result.certified_pruned_pair_mass = 0U;
  if (result.survivor_pair_mass > result.unordered_pair_universe_count) {
    throw std::logic_error("the seed survivor mass exceeds the pair universe");
  }
  result.unresolved_pair_mass =
      result.unordered_pair_universe_count - result.survivor_pair_mass;
  result.morton_owner_orientation_applied = true;
  result.yao48_binary64_proposal_classification_applied = true;
  if (!result.locally_valid()) {
    throw std::logic_error(
        "the Morton/Yao48 seed work-profile counters did not close");
  }
  return result;
}

}  // namespace morsehgp3d::gpu
