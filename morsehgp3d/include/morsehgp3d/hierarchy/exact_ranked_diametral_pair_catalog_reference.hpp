#pragma once

#include "morsehgp3d/hierarchy/ranked_diametral_pair_catalog.hpp"
#include "morsehgp3d/hierarchy/yao48_ranked_pair_candidates.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    exact_ranked_diametral_pair_catalog_reference_schema_version = 1U;
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_backend = "reference_cpu";
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_mode =
        "bounded_yao48_candidate_exact_rank_catalog_oracle";
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_public_status =
        "not_claimed";
inline constexpr std::string_view
    exact_ranked_diametral_pair_catalog_reference_proof_basis =
        "exhaustive_bounded_yao48_directional_superset_composed_with_exact_"
        "diametral_rank_trichotomy_complete_closed_point_payload_v1";

struct ExactRankedDiametralPairCatalogReferenceCounters {
  std::size_t point_count{};
  std::size_t unordered_pair_count{};
  std::size_t yao48_cutoff_pruned_pair_count{};
  std::size_t classified_candidate_pair_count{};
  std::size_t below_rank_candidate_count{};
  std::size_t exact_rank_candidate_count{};
  std::size_t above_rank_candidate_count{};
  std::size_t exact_classifier_node_visit_count{};
  std::size_t output_record_count{};
  std::size_t output_closed_point_reference_count{};
  bool complete_pair_mass_accounting_closed{false};
  bool complete_candidate_trichotomy_accounting_closed{false};
  bool bounded_quadratic_reference_only{true};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactRankedDiametralPairCatalogReferenceCounters&,
      const ExactRankedDiametralPairCatalogReferenceCounters&) = default;
};

// End-to-end falsification oracle for the exact bucket requested by the user.
// It deliberately reuses the bounded O(n^2) Yao48 producer, then sends every
// survivor to the exact LBVH rank trichotomy.  It is the executable authority
// for small differential tests, not the scalable product path.  The latter
// must replace only the candidate producer by a complete LBVH top-K/range
// session while preserving this classifier and record contract.
struct ExactRankedDiametralPairCatalogReferenceResult {
  static constexpr std::string_view backend =
      exact_ranked_diametral_pair_catalog_reference_backend;
  static constexpr std::string_view profile =
      exact_ranked_diametral_pair_catalog_reference_profile;
  static constexpr std::string_view mode =
      exact_ranked_diametral_pair_catalog_reference_mode;
  static constexpr std::string_view deployment_status =
      exact_ranked_diametral_pair_catalog_reference_deployment_status;
  static constexpr std::string_view public_status =
      exact_ranked_diametral_pair_catalog_reference_public_status;
  static constexpr std::string_view proof_basis =
      exact_ranked_diametral_pair_catalog_reference_proof_basis;

  std::uint32_t schema_version{
      exact_ranked_diametral_pair_catalog_reference_schema_version};
  std::size_t point_count{};
  std::size_t requested_order{};
  std::size_t target_closed_rank{};
  ExactYao48RankedPairCandidateResult proposal;
  std::vector<ExactRankedDiametralPairRecord> records;
  ExactRankedDiametralPairCatalogReferenceCounters counters{};

  // Structure and accounting only.  Geometry is authenticated by rebuilding
  // this bounded result or comparing it with the independent all-pair oracle.
  [[nodiscard]] bool locally_structurally_valid() const;

  friend bool operator==(
      const ExactRankedDiametralPairCatalogReferenceResult&,
      const ExactRankedDiametralPairCatalogReferenceResult&) = default;
};

[[nodiscard]] ExactRankedDiametralPairCatalogReferenceResult
build_exact_ranked_diametral_pair_catalog_reference(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_order);

}  // namespace morsehgp3d::hierarchy
