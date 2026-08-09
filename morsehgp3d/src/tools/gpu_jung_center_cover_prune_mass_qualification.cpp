#include "pair_support_smoke_clouds.hpp"
#include "jung_center_cover_prune_mass_receipt_oracle.hpp"

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::JungCenterCoverEndpointBlock;
using morsehgp3d::gpu::JungCenterCoverEndpointBlockKind;
using morsehgp3d::gpu::JungCenterCoverPruneMassConfig;
using morsehgp3d::gpu::JungCenterCoverPruneMassContext;
using morsehgp3d::gpu::JungCenterCoverPruneMassResult;
using morsehgp3d::gpu::JungCenterCoverPruneMassStatus;
using morsehgp3d::gpu::JungCenterCoverQualificationReceipt;
using morsehgp3d::gpu::JungCenterCoverTerminalDisposition;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::detail::JungCenterCoverExactReceiptBox;
using morsehgp3d::gpu::detail::JungCenterCoverPatchDecodeStatus;
using morsehgp3d::gpu::detail::decode_jung_center_cover_receipt_patch;
using morsehgp3d::gpu::detail::
    jung_center_cover_patch_cover_violation_count;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLeafRecord;

inline constexpr std::string_view kSchema =
    "morsehgp3d.phase15.jung_center_cover_prune_mass.qualification.v2";
#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unavailable";
#endif

struct Options final {
  std::size_t point_count{50'000U};
  std::string family{"uniform_latin"};
  std::size_t shard_count{2048U};
  std::size_t endpoint_microtile_pair_mass{64U};
  std::size_t stack_capacity_per_shard{
      morsehgp3d::gpu::jung_center_cover_minimum_stack_capacity};
  bool oracle{};
};

using ExactBox = JungCenterCoverExactReceiptBox;

struct ExactCoverGeometry final {
  ExactBox first;
  ExactBox second;
  ExactBox midpoint;
  ExactBox cover;
  ExactRational maximum_coordinate_separation;
  ExactRational maximum_squared_diameter;
};

struct OracleAudit final {
  bool performed{};
  bool passed{};
  std::size_t receipt_count{};
  std::size_t receipt_coherence_violation_count{};
  std::size_t invalid_enum_value_count{};
  std::size_t invalid_endpoint_block_count{};
  std::size_t microtile_threshold_violation_count{};
  std::size_t pair_coverage_omission_count{};
  std::size_t pair_coverage_duplicate_count{};
  std::size_t terminal_mass_violation_count{};
  std::size_t patch_mask_violation_count{};
  std::size_t invalid_patch_bounds_count{};
  std::size_t patch_cover_violation_count{};
  std::size_t infeasible_patch_replay_failure_count{};
  std::size_t witness_node_identity_violation_count{};
  std::size_t witness_antichain_violation_count{};
  std::size_t witness_mass_violation_count{};
  std::size_t witness_strictness_replay_failure_count{};
  std::uint64_t replayed_pruned_pair_mass{};
  std::uint64_t replayed_microtile_pair_mass{};
};

struct AuditContract final {
  bool passed{};
  std::size_t violation_count{};
};

[[nodiscard]] std::uint64_t ns_between(
    Clock::time_point begin,
    Clock::time_point end) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  return elapsed < 0 ? 0U : static_cast<std::uint64_t>(elapsed);
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* option) {
  std::size_t value = 0U;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument(std::string{"invalid value for "} + option);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--oracle") {
      options.oracle = true;
      continue;
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{"missing value after "} + std::string{option});
    }
    const std::string_view value{argv[++index]};
    if (option == "--point-count") {
      options.point_count = parse_size(value, "--point-count");
    } else if (option == "--family") {
      options.family = std::string{value};
    } else if (option == "--shard-count") {
      options.shard_count = parse_size(value, "--shard-count");
    } else if (option == "--microtile-pair-mass") {
      options.endpoint_microtile_pair_mass =
          parse_size(value, "--microtile-pair-mass");
    } else if (option == "--stack-capacity") {
      options.stack_capacity_per_shard =
          parse_size(value, "--stack-capacity");
    } else {
      throw std::invalid_argument(
          std::string{"unknown option "} + std::string{option});
    }
  }
  if (options.point_count < 2U || options.shard_count < 2U ||
      options.endpoint_microtile_pair_mass == 0U ||
      options.stack_capacity_per_shard <
          morsehgp3d::gpu::jung_center_cover_minimum_stack_capacity ||
      (options.family != "uniform_latin" &&
       options.family != "eight_clusters") ||
      (options.oracle && options.point_count > 32U)) {
    throw std::invalid_argument(
        "inconsistent Jung center-cover qualification options");
  }
  return options;
}

[[nodiscard]] CanonicalPointCloud make_cloud(const Options& options) {
  auto points = options.family == "uniform_latin"
      ? morsehgp3d::tools::pair_support_smoke::uniform_latin_points(
            options.point_count)
      : morsehgp3d::tools::pair_support_smoke::eight_clusters_points(
            options.point_count);
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const morsehgp3d::exact::CertifiedPoint3>{points});
}

[[nodiscard]] ExactRational zero() {
  return ExactRational{BigInt{0}};
}

[[nodiscard]] ExactRational exact_integer(std::uint64_t value) {
  return ExactRational{BigInt{value}};
}

[[nodiscard]] ExactRational exact_abs(const ExactRational& value) {
  return value.sign() < 0 ? -value : value;
}

[[nodiscard]] ExactRational exact_min(
    const ExactRational& first,
    const ExactRational& second) {
  return second < first ? second : first;
}

[[nodiscard]] ExactRational exact_max(
    const ExactRational& first,
    const ExactRational& second) {
  return first < second ? second : first;
}

[[nodiscard]] ExactBox range_box(
    const CanonicalPointCloud& cloud,
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  if (begin >= end || end > leaves.size()) {
    throw std::invalid_argument("invalid exact LBVH range box");
  }
  ExactBox result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower[axis] =
        cloud.point(leaves[begin].point_id).coordinate(axis);
    result.upper[axis] = result.lower[axis];
    for (std::size_t position = begin + 1U; position < end; ++position) {
      const auto& value =
          cloud.point(leaves[position].point_id).coordinate(axis);
      result.lower[axis] = exact_min(result.lower[axis], value);
      result.upper[axis] = exact_max(result.upper[axis], value);
    }
  }
  return result;
}

[[nodiscard]] ExactRational squared(const ExactRational& value) {
  return value * value;
}

[[nodiscard]] ExactRational power_axis_at(
    const ExactRational& center,
    const ExactRational& support_lower,
    const ExactRational& support_upper,
    const ExactRational& witness_lower,
    const ExactRational& witness_upper) {
  ExactRational support_distance = zero();
  if (center < support_lower) {
    support_distance = squared(center - support_lower);
  } else if (support_upper < center) {
    support_distance = squared(center - support_upper);
  }
  const ExactRational farthest = exact_max(
      squared(center - witness_lower),
      squared(center - witness_upper));
  return support_distance - farthest;
}

[[nodiscard]] ExactRational power_axis_lower(
    const ExactRational& center_lower,
    const ExactRational& center_upper,
    const ExactRational& support_lower,
    const ExactRational& support_upper,
    const ExactRational& witness_lower,
    const ExactRational& witness_upper) {
  std::array<ExactRational, 4U> candidates{
      center_lower, center_upper, support_lower, support_upper};
  ExactRational result = power_axis_at(
      center_lower,
      support_lower,
      support_upper,
      witness_lower,
      witness_upper);
  for (std::size_t index = 1U; index < 4U; ++index) {
    if (candidates[index] < center_lower ||
        center_upper < candidates[index]) {
      continue;
    }
    result = exact_min(
        result,
        power_axis_at(
            candidates[index],
            support_lower,
            support_upper,
            witness_lower,
            witness_upper));
  }
  return result;
}

[[nodiscard]] ExactRational power_lower(
    const ExactBox& center,
    const ExactBox& support,
    const ExactBox& witness) {
  ExactRational result = zero();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result = result + power_axis_lower(
        center.lower[axis],
        center.upper[axis],
        support.lower[axis],
        support.upper[axis],
        witness.lower[axis],
        witness.upper[axis]);
  }
  return result;
}

[[nodiscard]] ExactRational power_upper(
    const ExactBox& center,
    const ExactBox& support,
    const ExactBox& witness) {
  return -power_lower(center, witness, support);
}

[[nodiscard]] ExactRational squared_box_distance(
    const ExactBox& first,
    const ExactBox& second) {
  ExactRational result = zero();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    ExactRational distance = zero();
    if (first.upper[axis] < second.lower[axis]) {
      distance = second.lower[axis] - first.upper[axis];
    } else if (second.upper[axis] < first.lower[axis]) {
      distance = first.lower[axis] - second.upper[axis];
    }
    result = result + squared(distance);
  }
  return result;
}

[[nodiscard]] ExactCoverGeometry exact_cover_geometry(
    const ExactBox& first,
    const ExactBox& second) {
  ExactCoverGeometry result;
  result.first = first;
  result.second = second;
  result.maximum_coordinate_separation = zero();
  result.maximum_squared_diameter = zero();
  const ExactRational two = exact_integer(2U);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational separation = exact_max(
        exact_abs(second.upper[axis] - first.lower[axis]),
        exact_abs(first.upper[axis] - second.lower[axis]));
    result.maximum_coordinate_separation = exact_max(
        result.maximum_coordinate_separation, separation);
    result.maximum_squared_diameter =
        result.maximum_squared_diameter + squared(separation);
    result.midpoint.lower[axis] =
        (first.lower[axis] + second.lower[axis]) / two;
    result.midpoint.upper[axis] =
        (first.upper[axis] + second.upper[axis]) / two;
  }
  const ExactRational expansion =
      result.maximum_coordinate_separation * exact_integer(5U) /
      exact_integer(8U);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.cover.lower[axis] = result.midpoint.lower[axis] - expansion;
    result.cover.upper[axis] = result.midpoint.upper[axis] + expansion;
  }
  return result;
}

[[nodiscard]] bool exact_patch_infeasible(
    const ExactBox& patch,
    const ExactCoverGeometry& geometry) {
  if (power_lower(patch, geometry.first, geometry.second).sign() > 0 ||
      power_upper(patch, geometry.first, geometry.second).sign() < 0) {
    return true;
  }
  return exact_integer(8U) *
          squared_box_distance(patch, geometry.midpoint) >
      geometry.maximum_squared_diameter;
}

[[nodiscard]] bool ranges_overlap(
    std::size_t first_begin,
    std::size_t first_end,
    std::size_t second_begin,
    std::size_t second_end) {
  return first_begin < second_end && second_begin < first_end;
}

[[nodiscard]] std::size_t find_split(
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  const std::uint64_t first_code = leaves[begin].morton_code;
  const std::uint64_t last_code = leaves[end - 1U].morton_code;
  if (first_code == last_code) {
    return begin + (end - begin) / 2U;
  }
  const unsigned int highest_bit = static_cast<unsigned int>(
      std::bit_width(first_code ^ last_code)) - 1U;
  const std::uint64_t mask = std::uint64_t{1} << highest_bit;
  std::size_t low = begin + 1U;
  std::size_t high = end;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2U;
    if ((leaves[middle].morton_code & mask) == 0U) {
      low = middle + 1U;
    } else {
      high = middle;
    }
  }
  return low;
}

[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>>
reconstruct_postorder_ranges(std::span<const MortonLeafRecord> leaves) {
  std::vector<std::pair<std::size_t, std::size_t>> result;
  result.reserve(2U * leaves.size() - 1U);
  const auto visit = [&](auto&& self,
                         std::size_t begin,
                         std::size_t end) -> void {
    if (end - begin > 1U) {
      const std::size_t split = find_split(leaves, begin, end);
      self(self, begin, split);
      self(self, split, end);
    }
    result.emplace_back(begin, end);
  };
  visit(visit, 0U, leaves.size());
  return result;
}

[[nodiscard]] std::uint64_t block_mass(
    const JungCenterCoverEndpointBlock& block) {
  const std::uint64_t first_width =
      static_cast<std::uint64_t>(block.first.leaf_end) -
      block.first.leaf_begin;
  return block.kind == JungCenterCoverEndpointBlockKind::diagonal
      ? ((first_width & 1U) == 0U
             ? (first_width / 2U) * (first_width - 1U)
             : first_width * ((first_width - 1U) / 2U))
      : first_width *
          (static_cast<std::uint64_t>(block.second.leaf_end) -
           block.second.leaf_begin);
}

void mark_block_pairs(
    const JungCenterCoverEndpointBlock& block,
    std::size_t point_count,
    std::vector<std::uint8_t>& coverage,
    OracleAudit& audit) {
  const auto mark = [&](std::size_t first, std::size_t second) {
    if (second < first) {
      std::swap(first, second);
    }
    const std::size_t offset = first * point_count + second;
    if (coverage[offset] != 0U) {
      ++audit.pair_coverage_duplicate_count;
    } else {
      coverage[offset] = 1U;
    }
  };
  if (block.kind == JungCenterCoverEndpointBlockKind::diagonal) {
    for (std::size_t first = block.first.leaf_begin;
         first < block.first.leaf_end;
         ++first) {
      for (std::size_t second = first + 1U;
           second < block.first.leaf_end;
           ++second) {
        mark(first, second);
      }
    }
  } else {
    for (std::size_t first = block.first.leaf_begin;
         first < block.first.leaf_end;
         ++first) {
      for (std::size_t second = block.second.leaf_begin;
           second < block.second.leaf_end;
           ++second) {
        mark(first, second);
      }
    }
  }
}

[[nodiscard]] OracleAudit run_oracle(
    const CanonicalPointCloud& cloud,
    std::span<const MortonLeafRecord> leaves,
    const JungCenterCoverPruneMassResult& result) {
  OracleAudit audit;
  audit.performed = true;
  audit.receipt_count = result.qualification_receipts.size();
  audit.receipt_coherence_violation_count +=
      !result.audit.qualification_receipts_collected;
  audit.receipt_coherence_violation_count +=
      !result.audit.qualification_patch_bounds_recorded;
  audit.receipt_coherence_violation_count +=
      result.audit.qualification_receipt_count !=
      result.qualification_receipts.size();
  audit.receipt_coherence_violation_count +=
      result.audit.qualification_receipt_capacity !=
      result.audit.unordered_pair_universe_mass;
  audit.receipt_coherence_violation_count +=
      result.audit.point_count != cloud.size();
  const auto node_ranges = reconstruct_postorder_ranges(leaves);
  std::vector<std::uint8_t> pair_coverage(cloud.size() * cloud.size(), 0U);
  for (const auto& receipt : result.qualification_receipts) {
    const auto& block = receipt.block;
    const bool kind_valid =
        block.kind == JungCenterCoverEndpointBlockKind::diagonal ||
        block.kind == JungCenterCoverEndpointBlockKind::cross;
    const bool disposition_valid =
        receipt.disposition == JungCenterCoverTerminalDisposition::pruned ||
        receipt.disposition == JungCenterCoverTerminalDisposition::microtile;
    if (!kind_valid || !disposition_valid) {
      ++audit.invalid_enum_value_count;
      continue;
    }
    bool block_valid = block.first.node_index < node_ranges.size() &&
        block.second.node_index < node_ranges.size() &&
        node_ranges[block.first.node_index] == std::pair{
            static_cast<std::size_t>(block.first.leaf_begin),
            static_cast<std::size_t>(block.first.leaf_end)} &&
        node_ranges[block.second.node_index] == std::pair{
            static_cast<std::size_t>(block.second.leaf_begin),
            static_cast<std::size_t>(block.second.leaf_end)} &&
        block.unordered_pair_mass == block_mass(block);
    if (block.kind == JungCenterCoverEndpointBlockKind::diagonal) {
      block_valid = block_valid && block.first == block.second;
    } else {
      block_valid = block_valid &&
          block.first.leaf_end <= block.second.leaf_begin;
    }
    if (!block_valid) {
      ++audit.invalid_endpoint_block_count;
      continue;
    }
    mark_block_pairs(block, cloud.size(), pair_coverage, audit);
    if (receipt.disposition == JungCenterCoverTerminalDisposition::pruned) {
      audit.replayed_pruned_pair_mass += block.unordered_pair_mass;
    } else {
      audit.replayed_microtile_pair_mass += block.unordered_pair_mass;
      audit.microtile_threshold_violation_count +=
          block.unordered_pair_mass >
          result.audit.endpoint_microtile_pair_mass;
    }

    const ExactBox first = range_box(
        cloud, leaves, block.first.leaf_begin, block.first.leaf_end);
    const ExactBox second = range_box(
        cloud, leaves, block.second.leaf_begin, block.second.leaf_end);
    const ExactCoverGeometry geometry =
        exact_cover_geometry(first, second);
    if ((receipt.infeasible_patch_mask & receipt.covered_patch_mask) != 0U ||
        ((receipt.infeasible_patch_mask | receipt.covered_patch_mask) &
         ~receipt.patch_bounds_valid_mask) != 0U ||
        (receipt.disposition == JungCenterCoverTerminalDisposition::pruned &&
         (receipt.infeasible_patch_mask | receipt.covered_patch_mask) !=
             UINT64_MAX)) {
      ++audit.patch_mask_violation_count;
    }
    std::array<
        ExactBox,
        morsehgp3d::gpu::jung_center_cover_patch_count> receipt_patches{};
    std::array<
        bool,
        morsehgp3d::gpu::jung_center_cover_patch_count> patch_bounds_valid{};
    bool all_patch_bounds_valid = true;
    for (std::size_t patch_index = 0U;
         patch_index < morsehgp3d::gpu::jung_center_cover_patch_count;
         ++patch_index) {
      const std::uint64_t patch_bit = UINT64_C(1) << patch_index;
      const JungCenterCoverPatchDecodeStatus decode_status =
          decode_jung_center_cover_receipt_patch(
              receipt, patch_index, receipt_patches[patch_index]);
      if (decode_status == JungCenterCoverPatchDecodeStatus::absent) {
        all_patch_bounds_valid = false;
        if (((receipt.infeasible_patch_mask |
              receipt.covered_patch_mask) & patch_bit) != 0U ||
            receipt.witness_node_counts[patch_index] != 0U ||
            receipt.witness_point_masses[patch_index] != 0U) {
          ++audit.invalid_patch_bounds_count;
        }
        continue;
      }
      if (decode_status == JungCenterCoverPatchDecodeStatus::invalid) {
        ++audit.invalid_patch_bounds_count;
        all_patch_bounds_valid = false;
        continue;
      }
      patch_bounds_valid[patch_index] = true;
    }
    if (all_patch_bounds_valid) {
      audit.patch_cover_violation_count +=
          jung_center_cover_patch_cover_violation_count(
              receipt_patches, geometry.cover);
    }
    for (std::size_t patch_index = 0U;
         patch_index < morsehgp3d::gpu::jung_center_cover_patch_count;
         ++patch_index) {
      const std::uint64_t patch_bit = UINT64_C(1) << patch_index;
      if (!patch_bounds_valid[patch_index]) {
        continue;
      }
      const ExactBox& patch = receipt_patches[patch_index];
      if ((receipt.infeasible_patch_mask & patch_bit) != 0U &&
          !exact_patch_infeasible(patch, geometry)) {
        ++audit.infeasible_patch_replay_failure_count;
      }
      const std::size_t witness_count =
          receipt.witness_node_counts[patch_index];
      if (witness_count >
          morsehgp3d::gpu::jung_center_cover_support4_witness_threshold) {
        ++audit.witness_node_identity_violation_count;
        continue;
      }
      std::uint64_t witness_mass = 0U;
      std::vector<std::pair<std::size_t, std::size_t>> accepted_ranges;
      for (std::size_t witness = 0U;
           witness < witness_count;
           ++witness) {
        const std::size_t offset =
            patch_index *
                morsehgp3d::gpu::
                    jung_center_cover_support4_witness_threshold +
            witness;
        const std::size_t node_index =
            receipt.witness_node_indices[offset];
        const std::size_t begin =
            receipt.witness_node_leaf_begins[offset];
        const std::size_t end = receipt.witness_node_leaf_ends[offset];
        const std::uint64_t mass =
            receipt.witness_node_point_masses[offset];
        if (node_index >= node_ranges.size() ||
            node_ranges[node_index] != std::pair{begin, end} ||
            begin >= end || end > leaves.size() ||
            mass != end - begin) {
          ++audit.witness_node_identity_violation_count;
          continue;
        }
        bool overlaps = ranges_overlap(
            begin, end, block.first.leaf_begin, block.first.leaf_end) ||
            ranges_overlap(
                begin, end, block.second.leaf_begin, block.second.leaf_end);
        for (const auto& prior : accepted_ranges) {
          overlaps = overlaps ||
              ranges_overlap(begin, end, prior.first, prior.second);
        }
        if (overlaps) {
          ++audit.witness_antichain_violation_count;
        }
        accepted_ranges.emplace_back(begin, end);
        witness_mass += mass;
        const ExactBox witness_box = range_box(cloud, leaves, begin, end);
        if (exact_max(
                power_lower(patch, first, witness_box),
                power_lower(patch, second, witness_box))
                .sign() <= 0) {
          ++audit.witness_strictness_replay_failure_count;
        }
      }
      if (witness_mass != receipt.witness_point_masses[patch_index] ||
          (((receipt.covered_patch_mask & patch_bit) != 0U) !=
           (witness_mass >= morsehgp3d::gpu::
                jung_center_cover_support4_witness_threshold))) {
        ++audit.witness_mass_violation_count;
      }
    }
  }
  for (std::size_t first = 0U; first < cloud.size(); ++first) {
    for (std::size_t second = first + 1U; second < cloud.size(); ++second) {
      audit.pair_coverage_omission_count +=
          pair_coverage[first * cloud.size() + second] == 0U;
    }
  }
  audit.terminal_mass_violation_count +=
      audit.replayed_pruned_pair_mass != result.audit.pruned_pair_mass;
  audit.terminal_mass_violation_count +=
      audit.replayed_microtile_pair_mass != result.audit.microtile_pair_mass;
  audit.terminal_mass_violation_count +=
      audit.replayed_pruned_pair_mass + audit.replayed_microtile_pair_mass !=
      result.audit.unordered_pair_universe_mass;
  audit.passed =
      result.status == JungCenterCoverPruneMassStatus::complete &&
      audit.receipt_coherence_violation_count == 0U &&
      audit.invalid_enum_value_count == 0U &&
      audit.invalid_endpoint_block_count == 0U &&
      audit.microtile_threshold_violation_count == 0U &&
      audit.pair_coverage_omission_count == 0U &&
      audit.pair_coverage_duplicate_count == 0U &&
      audit.terminal_mass_violation_count == 0U &&
      audit.patch_mask_violation_count == 0U &&
      audit.invalid_patch_bounds_count == 0U &&
      audit.patch_cover_violation_count == 0U &&
      audit.infeasible_patch_replay_failure_count == 0U &&
      audit.witness_node_identity_violation_count == 0U &&
      audit.witness_antichain_violation_count == 0U &&
      audit.witness_mass_violation_count == 0U &&
      audit.witness_strictness_replay_failure_count == 0U;
  return audit;
}

void print_boolean(bool value) {
  std::cout << (value ? "true" : "false");
}

template <std::size_t Extent>
[[nodiscard]] std::uint64_t checked_histogram_sum(
    const std::array<std::uint64_t, Extent>& values,
    bool& valid) {
  std::uint64_t result = 0U;
  for (const std::uint64_t value : values) {
    if (value > UINT64_MAX - result) {
      valid = false;
      return 0U;
    }
    result += value;
  }
  return result;
}

[[nodiscard]] AuditContract check_audit_contract(
    const Options& options,
    const JungCenterCoverPruneMassResult& result) {
  AuditContract contract;
  const auto& audit = result.audit;
  const auto require = [&contract](bool condition) {
    contract.violation_count += condition ? 0U : 1U;
  };
  bool block_partition_valid =
      audit.pruned_block_count <= UINT64_MAX - audit.microtile_block_count &&
      audit.pruned_block_count + audit.microtile_block_count <=
          UINT64_MAX - audit.diagonal_split_count &&
      audit.pruned_block_count + audit.microtile_block_count +
              audit.diagonal_split_count <=
          UINT64_MAX - audit.cross_split_count;
  const std::uint64_t classified_block_visits = block_partition_valid
      ? audit.pruned_block_count + audit.microtile_block_count +
          audit.diagonal_split_count + audit.cross_split_count
      : 0U;
  const bool patch_product_valid =
      audit.center_cover_attempt_count <=
      UINT64_MAX / morsehgp3d::gpu::jung_center_cover_patch_count;
  const std::uint64_t expected_patch_evaluations = patch_product_valid
      ? audit.center_cover_attempt_count *
          morsehgp3d::gpu::jung_center_cover_patch_count
      : 0U;
  const bool patch_partition_valid =
      audit.infeasible_patch_count <= UINT64_MAX - audit.covered_patch_count &&
      audit.infeasible_patch_count + audit.covered_patch_count <=
          UINT64_MAX - audit.unresolved_patch_count;
  const std::uint64_t classified_patches = patch_partition_valid
      ? audit.infeasible_patch_count + audit.covered_patch_count +
          audit.unresolved_patch_count
      : 0U;
  bool block_histogram_valid = true;
  bool witness_histogram_valid = true;
  const std::uint64_t block_histogram_sum = checked_histogram_sum(
      audit.shard_block_visit_histogram, block_histogram_valid);
  const std::uint64_t witness_histogram_sum = checked_histogram_sum(
      audit.shard_witness_visit_histogram, witness_histogram_valid);
  const std::size_t maximum_ctas_with_work =
      std::min(audit.worker_cta_count, audit.generated_shard_count);
  const bool receipt_mode_valid = options.oracle
      ? (audit.point_count <= 32U &&
         audit.qualification_receipts_collected &&
         audit.qualification_patch_bounds_recorded &&
         audit.qualification_receipt_count ==
             result.qualification_receipts.size() &&
         audit.qualification_receipt_capacity ==
             audit.unordered_pair_universe_mass)
      : (!audit.qualification_receipts_collected &&
         !audit.qualification_patch_bounds_recorded &&
         audit.qualification_receipt_count == 0U &&
         audit.qualification_receipt_capacity == 0U &&
         result.qualification_receipts.empty());

  require(result.status == JungCenterCoverPruneMassStatus::complete);
  require(audit.schema_version ==
          morsehgp3d::gpu::jung_center_cover_prune_mass_schema_version);
  require(audit.point_count == options.point_count);
  require(audit.requested_shard_count == options.shard_count);
  require(audit.generated_shard_count >= 1U &&
          audit.generated_shard_count <= audit.requested_shard_count);
  require(audit.worker_cta_count >= 2U);
  require(audit.worker_cta_with_work_count >= 1U &&
          audit.worker_cta_with_work_count <= maximum_ctas_with_work);
  require(audit.multi_cta_scheduler_used ==
          (audit.worker_cta_with_work_count >= 2U));
  require(options.point_count != 50'000U || audit.multi_cta_scheduler_used);
  require(audit.stack_capacity_per_shard ==
              options.stack_capacity_per_shard &&
          audit.stack_capacity_per_shard >=
              morsehgp3d::gpu::jung_center_cover_minimum_stack_capacity &&
          audit.maximum_stack_high_water <=
              audit.stack_capacity_per_shard &&
          audit.stack_capacity_failure_count == 0U);
  require(audit.endpoint_microtile_pair_mass ==
          options.endpoint_microtile_pair_mass);
  require(audit.exact_mass_identity_satisfied &&
          audit.pruned_pair_mass <= audit.unordered_pair_universe_mass &&
          audit.microtile_pair_mass ==
              audit.unordered_pair_universe_mass - audit.pruned_pair_mass);
  require(audit.endpoint_block_visit_count ==
          audit.center_cover_attempt_count);
  require(block_partition_valid &&
          classified_block_visits == audit.endpoint_block_visit_count);
  require(patch_product_valid &&
          audit.patch_evaluation_count == expected_patch_evaluations);
  require(patch_partition_valid &&
          classified_patches == audit.patch_evaluation_count);
  require(block_histogram_valid && witness_histogram_valid &&
          block_histogram_sum == audit.generated_shard_count &&
          witness_histogram_sum == audit.generated_shard_count);
  require(audit.source_identity_authenticated &&
          audit.source_epoch_authenticated);
  require(audit.support4_only && audit.fixed_64_closed_patches_used &&
          audit.directed_binary64_intervals_used && audit.strict_prune_only &&
          audit.fail_open_on_ambiguity &&
          audit.private_per_shard_dfs_used);
  require(audit.cuda_kernel_launch_count == 3U &&
          audit.cuda_synchronization_count == 1U &&
          audit.one_terminal_synchronization_used);
  require(audit.cuda_execution_performed &&
          !audit.host_fake_launcher_exercised);
  require(receipt_mode_valid);
  require(audit.aggregate_only_50k_path ==
          (options.point_count == 50'000U && !options.oracle));
  require(!audit.anchor_or_pair_payload_emitted &&
          !audit.endpoint_pair_array_materialized &&
          !audit.higher_order_delaunay_mosaic_materialized &&
          !audit.scientific_decision_published &&
          !audit.scientific_result_claimed && audit.component_only &&
          !audit.slo_eligible && !audit.warm_e2e_slo_claimed &&
          !audit.anchor_completeness_claimed &&
          !audit.public_status_claimed);
  contract.passed = contract.violation_count == 0U;
  return contract;
}

void print_histogram(const std::array<std::uint64_t, 32U>& values) {
  std::cout << '[';
  for (std::size_t index = 0U; index < values.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << values[index];
  }
  std::cout << ']';
}

void print_json(
    const Options& options,
    const JungCenterCoverPruneMassResult& result,
    const OracleAudit& oracle,
    const AuditContract& audit_contract,
    std::uint64_t cloud_ns,
    std::uint64_t build_ns,
    std::uint64_t profile_wall_ns) {
  const auto& audit = result.audit;
  std::cout << '{';
  std::cout << "\"schema\":\"" << kSchema << "\",";
  std::cout << "\"git_sha\":\"" << kGitSha << "\",";
  std::cout << "\"phase\":\""
            << morsehgp3d::gpu::jung_center_cover_prune_mass_phase
            << "\",";
  std::cout << "\"backend\":\""
            << morsehgp3d::gpu::jung_center_cover_prune_mass_backend
            << "\",";
  std::cout << "\"profile\":\""
            << morsehgp3d::gpu::jung_center_cover_prune_mass_profile
            << "\",";
  std::cout << "\"mode\":\""
            << morsehgp3d::gpu::jung_center_cover_prune_mass_mode
            << "\",";
  std::cout << "\"deployment_status\":\""
            << morsehgp3d::gpu::
                jung_center_cover_prune_mass_deployment_status
            << "\",";
  std::cout << "\"public_status\":\""
            << morsehgp3d::gpu::jung_center_cover_prune_mass_public_status
            << "\",";
  std::cout << "\"family\":\"" << options.family << "\",";
  std::cout << "\"point_count\":" << audit.point_count << ',';
  std::cout << "\"support_size\":4,";
  std::cout << "\"maximum_closed_rank\":11,";
  std::cout << "\"status\":"
            << static_cast<unsigned int>(result.status) << ',';
  std::cout << "\"requested_shard_count\":"
            << audit.requested_shard_count << ',';
  std::cout << "\"generated_shard_count\":"
            << audit.generated_shard_count << ',';
  std::cout << "\"worker_cta_count\":" << audit.worker_cta_count << ',';
  std::cout << "\"worker_cta_with_work_count\":"
            << audit.worker_cta_with_work_count << ',';
  std::cout << "\"stack_capacity_per_shard\":"
            << audit.stack_capacity_per_shard << ',';
  std::cout << "\"structural_stack_minimum\":"
            << morsehgp3d::gpu::jung_center_cover_minimum_stack_capacity
            << ',';
  std::cout << "\"maximum_stack_high_water\":"
            << audit.maximum_stack_high_water << ',';
  std::cout << "\"microtile_pair_mass_limit\":"
            << audit.endpoint_microtile_pair_mass << ',';
  std::cout << "\"unordered_pair_universe_mass\":"
            << audit.unordered_pair_universe_mass << ',';
  std::cout << "\"pruned_pair_mass\":" << audit.pruned_pair_mass << ',';
  std::cout << "\"microtile_pair_mass\":"
            << audit.microtile_pair_mass << ',';
  std::cout << "\"endpoint_block_visit_count\":"
            << audit.endpoint_block_visit_count << ',';
  std::cout << "\"center_cover_attempt_count\":"
            << audit.center_cover_attempt_count << ',';
  std::cout << "\"pruned_block_count\":"
            << audit.pruned_block_count << ',';
  std::cout << "\"microtile_block_count\":"
            << audit.microtile_block_count << ',';
  std::cout << "\"diagonal_split_count\":"
            << audit.diagonal_split_count << ',';
  std::cout << "\"cross_split_count\":"
            << audit.cross_split_count << ',';
  std::cout << "\"patch_evaluation_count\":"
            << audit.patch_evaluation_count << ',';
  std::cout << "\"infeasible_patch_count\":"
            << audit.infeasible_patch_count << ',';
  std::cout << "\"covered_patch_count\":"
            << audit.covered_patch_count << ',';
  std::cout << "\"unresolved_patch_count\":"
            << audit.unresolved_patch_count << ',';
  std::cout << "\"witness_node_visit_count\":"
            << audit.witness_node_visit_count << ',';
  std::cout << "\"accepted_witness_node_count\":"
            << audit.accepted_witness_node_count << ',';
  std::cout << "\"accepted_witness_point_mass\":"
            << audit.accepted_witness_point_mass << ',';
  std::cout << "\"arithmetic_ambiguity_count\":"
            << audit.arithmetic_ambiguity_count << ',';
  std::cout << "\"maximum_shard_block_visits\":"
            << audit.maximum_shard_block_visits << ',';
  std::cout << "\"maximum_shard_witness_node_visits\":"
            << audit.maximum_shard_witness_node_visits << ',';
  std::cout << "\"shard_block_visit_histogram\":";
  print_histogram(audit.shard_block_visit_histogram);
  std::cout << ",\"shard_witness_visit_histogram\":";
  print_histogram(audit.shard_witness_visit_histogram);
  std::cout << ",\"physical_device_arena_bytes\":"
            << audit.physical_device_arena_byte_count << ',';
  std::cout << "\"source_device_bytes\":"
            << audit.source_device_byte_count << ',';
  std::cout << "\"device_allocation_count\":"
            << audit.device_allocation_count << ',';
  std::cout << "\"device_memset_count\":"
            << audit.device_memset_count << ',';
  std::cout << "\"cuda_kernel_launch_count\":"
            << audit.cuda_kernel_launch_count << ',';
  std::cout << "\"cuda_synchronization_count\":"
            << audit.cuda_synchronization_count << ',';
  std::cout << "\"host_to_device_bytes\":"
            << audit.host_to_device_byte_count << ',';
  std::cout << "\"device_to_host_bytes\":"
            << audit.device_to_host_byte_count << ',';
  std::cout << "\"cloud_generation_ns\":" << cloud_ns << ',';
  std::cout << "\"lbvh_build_ns\":" << build_ns << ',';
  std::cout << "\"shard_source_device_ns\":"
            << audit.shard_source_device_nanoseconds << ',';
  std::cout << "\"center_cover_device_ns\":"
            << audit.center_cover_device_nanoseconds << ',';
  std::cout << "\"finalize_device_ns\":"
            << audit.finalize_device_nanoseconds << ',';
  std::cout << "\"source_cover_device_ns\":"
            << audit.source_cover_device_nanoseconds << ',';
  std::cout << "\"profile_wall_ns\":" << profile_wall_ns << ',';
  std::cout << "\"launcher_wall_ns\":"
            << audit.launcher_wall_nanoseconds << ',';
  std::cout << "\"oracle_performed\":";
  print_boolean(oracle.performed);
  std::cout << ",\"oracle_passed\":";
  print_boolean(oracle.passed);
  std::cout << ",\"oracle_receipt_count\":" << oracle.receipt_count;
  std::cout << ",\"oracle_receipt_coherence_violations\":"
            << oracle.receipt_coherence_violation_count;
  std::cout << ",\"oracle_invalid_enum_values\":"
            << oracle.invalid_enum_value_count;
  std::cout << ",\"oracle_invalid_endpoint_blocks\":"
            << oracle.invalid_endpoint_block_count;
  std::cout << ",\"oracle_microtile_threshold_violations\":"
            << oracle.microtile_threshold_violation_count;
  std::cout << ",\"oracle_pair_omissions\":"
            << oracle.pair_coverage_omission_count;
  std::cout << ",\"oracle_pair_duplicates\":"
            << oracle.pair_coverage_duplicate_count;
  std::cout << ",\"oracle_terminal_mass_violations\":"
            << oracle.terminal_mass_violation_count;
  std::cout << ",\"oracle_patch_mask_violations\":"
            << oracle.patch_mask_violation_count;
  std::cout << ",\"oracle_invalid_patch_bounds\":"
            << oracle.invalid_patch_bounds_count;
  std::cout << ",\"oracle_patch_cover_violations\":"
            << oracle.patch_cover_violation_count;
  std::cout << ",\"oracle_infeasible_replay_failures\":"
            << oracle.infeasible_patch_replay_failure_count;
  std::cout << ",\"oracle_witness_identity_violations\":"
            << oracle.witness_node_identity_violation_count;
  std::cout << ",\"oracle_witness_antichain_violations\":"
            << oracle.witness_antichain_violation_count;
  std::cout << ",\"oracle_witness_mass_violations\":"
            << oracle.witness_mass_violation_count;
  std::cout << ",\"oracle_witness_strictness_failures\":"
            << oracle.witness_strictness_replay_failure_count;
  std::cout << ",\"audit_contract_passed\":";
  print_boolean(audit_contract.passed);
  std::cout << ",\"audit_contract_violation_count\":"
            << audit_contract.violation_count;
  std::cout << ",\"exact_mass_identity_satisfied\":";
  print_boolean(audit.exact_mass_identity_satisfied);
  std::cout << ",\"multi_cta_scheduler_used\":";
  print_boolean(audit.multi_cta_scheduler_used);
  std::cout << ",\"one_terminal_synchronization_used\":";
  print_boolean(audit.one_terminal_synchronization_used);
  std::cout << ",\"qualification_receipts_collected\":";
  print_boolean(audit.qualification_receipts_collected);
  std::cout << ",\"qualification_patch_bounds_recorded\":";
  print_boolean(audit.qualification_patch_bounds_recorded);
  std::cout << ",\"aggregate_only_50k_path\":";
  print_boolean(audit.aggregate_only_50k_path);
  std::cout << ",\"anchor_or_pair_payload_emitted\":";
  print_boolean(audit.anchor_or_pair_payload_emitted);
  std::cout << ",\"endpoint_pair_array_materialized\":";
  print_boolean(audit.endpoint_pair_array_materialized);
  std::cout << ",\"higher_order_delaunay_mosaic_materialized\":";
  print_boolean(audit.higher_order_delaunay_mosaic_materialized);
  std::cout << ",\"component_only\":";
  print_boolean(audit.component_only);
  std::cout << ",\"slo_eligible\":";
  print_boolean(audit.slo_eligible);
  std::cout << ",\"warm_e2e_slo_claimed\":";
  print_boolean(audit.warm_e2e_slo_claimed);
  std::cout << ",\"anchor_completeness_claimed\":";
  print_boolean(audit.anchor_completeness_claimed);
  std::cout << ",\"scientific_result_claimed\":";
  print_boolean(audit.scientific_result_claimed);
  std::cout << ",\"public_status_claimed\":";
  print_boolean(audit.public_status_claimed);
  std::cout << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const auto cloud_begin = Clock::now();
    CanonicalPointCloud cloud = make_cloud(options);
    const auto cloud_end = Clock::now();
    MortonLbvhBuildContext builder{cloud.size()};
    const auto build_begin = Clock::now();
    auto build = builder.build(cloud);
    const auto build_end = Clock::now();
    if (!build.complete_certified_build() || !build.cuda_qualified_build()) {
      throw std::runtime_error(
          "the Jung center-cover qualification requires a certified CUDA LBVH");
    }
    const std::vector<MortonLeafRecord> leaves{
        build.certified_index().leaves().begin(),
        build.certified_index().leaves().end()};
    auto traversal = builder.release_device_traversal_lease(build);
    JungCenterCoverPruneMassContext profiler{
        std::move(traversal),
        JungCenterCoverPruneMassConfig{
            options.shard_count,
            options.endpoint_microtile_pair_mass,
            options.stack_capacity_per_shard,
            options.oracle}};
    const auto profile_begin = Clock::now();
    const JungCenterCoverPruneMassResult result = profiler.profile_once();
    const auto profile_end = Clock::now();
    OracleAudit oracle;
    if (options.oracle) {
      oracle = run_oracle(cloud, leaves, result);
    }
    const AuditContract audit_contract =
        check_audit_contract(options, result);
    print_json(
        options,
        result,
        oracle,
        audit_contract,
        ns_between(cloud_begin, cloud_end),
        ns_between(build_begin, build_end),
        ns_between(profile_begin, profile_end));
    return result.status == JungCenterCoverPruneMassStatus::complete &&
            audit_contract.passed &&
            (!options.oracle || oracle.passed)
        ? 0
        : 1;
  } catch (const std::exception& error) {
    std::cerr << "Jung center-cover qualification failed: " << error.what()
              << '\n';
    return 1;
  }
}
