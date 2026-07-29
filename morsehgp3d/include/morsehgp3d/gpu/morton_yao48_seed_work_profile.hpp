#pragma once

#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    morton_yao48_seed_work_profile_schema_version = 1U;
inline constexpr std::size_t morton_yao48_seed_work_profile_cone_count = 48U;
inline constexpr std::size_t
    morton_yao48_seed_work_profile_maximum_bank_capacity = 10U;
inline constexpr std::string_view
    morton_yao48_seed_work_profile_scientific_status =
        "benchmark_only_binary64_seed_not_a_pair_frontier_certificate";

// This diagnostic summarizes a bounded Morton-neighbor transcript.  Its
// binary64 cone labels are proposals only: no count below certifies that a
// pair is absent from the exact Morton/Yao48 frontier.
struct MortonYao48SeedWorkProfile {
  std::uint32_t schema_version{
      morton_yao48_seed_work_profile_schema_version};
  std::size_t point_count{};
  std::size_t maximum_order{};
  std::size_t bank_capacity{};
  std::size_t point_cone_bank_count{};
  std::size_t bank_receipt_count{};
  std::size_t nonempty_bank_count{};
  std::size_t cutoff_ready_bank_count{};
  std::array<std::size_t,
             morton_yao48_seed_work_profile_maximum_bank_capacity + 1U>
      bank_fill_histogram{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t survivor_pair_mass{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  bool morton_owner_orientation_applied{};
  bool yao48_binary64_proposal_classification_applied{};
  bool exact_yao48_classification_claimed{};
  bool exact_pair_frontier_claimed{};
  bool dense_pair_fallback_performed{};
  bool global_pair_matrix_materialized{};
  bool public_status_claimed{};

  [[nodiscard]] bool locally_valid() const noexcept;

  friend bool operator==(
      const MortonYao48SeedWorkProfile&,
      const MortonYao48SeedWorkProfile&) = default;
};

// Half-open binary64 counterpart of the exact 8-sign x 6-axis-order
// partition.  It is deliberately named as a proposal: rounded subtraction
// is not an exact-rational cone certificate.
[[nodiscard]] std::size_t classify_binary64_yao48_cone_proposal(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target);

// source_point_ids_by_morton_position is a permutation of the cloud PointIds.
// neighbor_point_ids is row-major by Morton position with maximum_order
// entries per row.  Only a neighbor appearing in the higher Morton endpoint's
// row contributes to survivor_pair_mass, so every unordered pair is counted
// at most once.  Everything not retained stays unresolved; this routine never
// turns a heuristic omission into a prune certificate.
[[nodiscard]] MortonYao48SeedWorkProfile
summarize_morton_yao48_seed_work_profile(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId>
        source_point_ids_by_morton_position,
    std::span<const spatial::PointId> neighbor_point_ids,
    std::size_t maximum_order,
    std::size_t worker_count);

}  // namespace morsehgp3d::gpu
