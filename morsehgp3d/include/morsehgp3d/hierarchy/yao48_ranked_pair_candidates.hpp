#pragma once

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/hierarchy/yao48_emst.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    exact_yao48_ranked_pair_candidate_schema_version = 2U;
inline constexpr std::size_t
    exact_yao48_ranked_pair_candidate_maximum_order = 10U;
// This producer deliberately materializes a candidate superset and scans every
// input pair.  Its small guard makes it a falsification/design oracle, never a
// product fallback under another name.
inline constexpr std::size_t
    exact_yao48_ranked_pair_candidate_reference_max_point_count = 512U;
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_backend = "reference_cpu";
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_mode =
        "bounded_exact_yao48_rank_cutoff_oracle";
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_public_status = "not_claimed";
inline constexpr std::string_view
    exact_yao48_ranked_pair_candidate_proof_basis =
        "yao48_closed_cone_extreme_ray_Kth_radius_directional_projection_"
        "witness_closed_diametral_rank_cutoff_strict_candidate_region_v2";

enum class ExactYao48DirectionalCutoffSemantics : std::uint8_t {
  closed_ball,
  strict_interior,
};

// One receipt owns the exact K nearest witnesses of one half-open Yao48
// chamber, ordered by (squared distance, PointId).  When the chamber contains
// fewer than K points there is no finite cutoff and every point in it remains
// a candidate.  Otherwise the Kth squared distance D_K proves that an endpoint
// q can be rejected once its three squared extreme-ray projections are at
// least D_K, 2*D_K, and 3*D_K.  The coarser |q-p|^2 >= 3*D_K test implies those
// inequalities, but the projection form is strictly tighter away from the
// worst chamber rays.  Either certificate gives K non-support closed witnesses
// and hence closed rank at least K+2.
struct ExactYao48RankCutoffReceipt {
  spatial::PointId anchor_point_id{};
  std::size_t cone_index{};
  std::size_t cone_point_count{};
  std::array<spatial::PointId,
             exact_yao48_ranked_pair_candidate_maximum_order>
      nearest_point_ids{};
  std::size_t retained_nearest_point_count{};
  std::optional<exact::ExactLevel> kth_squared_distance;

  friend bool operator==(
      const ExactYao48RankCutoffReceipt&,
      const ExactYao48RankCutoffReceipt&) = default;
};

// Every unordered pair is stored once by canonical PointId.  Both directed
// chambers are retained because a target pair must pass the exact directional
// cutoff at both endpoints; intersecting the two safe supersets is still safe
// and normally much tighter.  The exact diametral classifier remains the
// authority for below/exact/above.
struct ExactYao48RankedPairCandidate {
  std::array<spatial::PointId, 2> support_ids{};
  std::array<std::size_t, 2> directed_cone_indices{};

  friend bool operator==(
      const ExactYao48RankedPairCandidate&,
      const ExactYao48RankedPairCandidate&) = default;
};

struct ExactYao48RankedPairCandidateCounters {
  std::size_t point_count{};
  std::size_t unordered_pair_count{};
  std::size_t exact_squared_distance_evaluation_count{};
  std::size_t directed_cone_classification_count{};
  std::size_t point_cone_count{};
  std::size_t finite_cutoff_count{};
  std::size_t underfull_cone_count{};
  std::size_t retained_witness_reference_count{};
  std::size_t candidate_pair_count{};
  std::size_t cutoff_pruned_pair_count{};
  std::size_t uniform_radial_pruned_pair_count{};
  std::size_t directional_refinement_pruned_pair_count{};

  friend bool operator==(
      const ExactYao48RankedPairCandidateCounters&,
      const ExactYao48RankedPairCandidateCounters&) = default;
};

struct ExactYao48RankedPairCandidateResult {
  static constexpr std::string_view backend =
      exact_yao48_ranked_pair_candidate_backend;
  static constexpr std::string_view profile =
      exact_yao48_ranked_pair_candidate_profile;
  static constexpr std::string_view mode =
      exact_yao48_ranked_pair_candidate_mode;
  static constexpr std::string_view deployment_status =
      exact_yao48_ranked_pair_candidate_deployment_status;
  static constexpr std::string_view public_status =
      exact_yao48_ranked_pair_candidate_public_status;
  static constexpr std::string_view proof_basis =
      exact_yao48_ranked_pair_candidate_proof_basis;

  std::uint32_t schema_version{
      exact_yao48_ranked_pair_candidate_schema_version};
  std::size_t point_count{};
  std::size_t requested_order{};
  std::vector<ExactYao48RankCutoffReceipt> cutoff_receipts;
  std::vector<ExactYao48RankedPairCandidate> candidates;
  ExactYao48RankedPairCandidateCounters counters{};

  // Structural and accounting check only.  It does not authenticate geometry;
  // the bounded differential test independently classifies every pair.
  [[nodiscard]] bool locally_structurally_valid() const;

  friend bool operator==(
      const ExactYao48RankedPairCandidateResult&,
      const ExactYao48RankedPairCandidateResult&) = default;
};

// Exact one-direction decision used by both the bounded oracle and the future
// LBVH range reporter.  witness_squared_radius_upper_bound may be any positive
// upper bound for K distinct certified witnesses in the target's half-open
// chamber; the witnesses need not be the K nearest.  A looser bound only loses
// prunes.  closed_ball uses three non-strict comparisons.  strict_interior
// uses three strict comparisons and can therefore credit the RelevantGP lane.
// The caller remains responsible for the witness count, identities, chamber
// membership and distance bounds.  No floating square root or trigonometric
// value is evaluated.
[[nodiscard]] bool exact_yao48_directional_witness_radius_cutoff_certifies(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& witness_squared_radius_upper_bound,
    ExactYao48DirectionalCutoffSemantics semantics);

// Compatibility spelling for the exact-nearest bounded oracle.  Equality is
// a rejection because that oracle classifies the closed diametral rank.
[[nodiscard]] bool exact_yao48_directional_rank_cutoff_certifies_above(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& kth_squared_distance);

[[nodiscard]] ExactYao48RankedPairCandidateResult
build_exact_yao48_ranked_pair_candidate_reference(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_order);

}  // namespace morsehgp3d::hierarchy
