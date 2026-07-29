#pragma once

#include "morsehgp3d/hierarchy/emst.hpp"
#include "morsehgp3d/hierarchy/yao48_cone.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t exact_yao48_emst_schema_version = 1U;
// The exhaustive pair scan is a design oracle, never a production path.
// Keeping the guard in the API prevents an accidental quadratic deployment.
inline constexpr std::size_t exact_yao48_emst_reference_max_point_count =
    4096U;
inline constexpr std::string_view exact_yao48_emst_backend =
    "reference_cpu";
inline constexpr std::string_view exact_yao48_emst_profile = "hgp_reduced";
inline constexpr std::string_view exact_yao48_emst_mode =
    "exact_yao48_emst_reference";
inline constexpr std::string_view exact_yao48_emst_deployment_status =
    "architecture_only";
inline constexpr std::string_view exact_yao48_emst_public_status =
    "not_claimed";
inline constexpr std::string_view exact_yao48_emst_proof_basis =
    "canonical_48_signed_absolute_coordinate_order_cones_"
    "angular_diameter_arccos_one_over_sqrt_three_strictly_below_pi_over_"
    "three_exact_nearest_edge_per_point_cone_canonical_kruskal_v1";

struct ExactYao48DirectedCandidate {
  spatial::PointId source_point_id{};
  std::size_t cone_index{};
  ExactEmstEdge edge{};

  friend bool operator==(
      const ExactYao48DirectedCandidate&,
      const ExactYao48DirectedCandidate&) = default;
};

struct ExactYao48EmstCounters {
  std::size_t point_count{};
  std::size_t unordered_pair_count{};
  std::size_t exact_squared_distance_evaluation_count{};
  std::size_t directed_cone_classification_count{};
  std::size_t directed_candidate_count{};
  std::size_t empty_point_cone_count{};
  std::size_t unique_candidate_edge_count{};
  std::size_t selected_edge_count{};
  std::size_t redundant_candidate_edge_count{};
  std::size_t final_component_count{};

  friend bool operator==(
      const ExactYao48EmstCounters&, const ExactYao48EmstCounters&) = default;
};

// Reference-only O(n^2) construction.  It never builds a Delaunay object,
// higher-order cell, coface or pair matrix.  directed_candidates is bounded by
// 48*n; the quadratic work comes solely from the exhaustive unordered-pair
// scan used to choose the exact nearest edge of every point/chamber.  The Yao
// candidates are not claimed to form a Gabriel catalogue and carry no
// support-three information: this result is a k=1 EMST reference, not the
// common rank-at-most-three k=1/k=2 skeleton.
struct ExactYao48EmstResult {
  static constexpr std::string_view backend = exact_yao48_emst_backend;
  static constexpr std::string_view profile = exact_yao48_emst_profile;
  static constexpr std::string_view mode = exact_yao48_emst_mode;
  static constexpr std::string_view deployment_status =
      exact_yao48_emst_deployment_status;
  static constexpr std::string_view public_status =
      exact_yao48_emst_public_status;
  static constexpr std::string_view proof_basis =
      exact_yao48_emst_proof_basis;

  std::uint32_t schema_version{exact_yao48_emst_schema_version};
  std::size_t point_count{};
  std::vector<ExactYao48DirectedCandidate> directed_candidates;
  std::vector<ExactEmstEdge> unique_candidate_edges;
  std::vector<ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  ExactYao48EmstCounters counters{};

  // Structural self-check only.  It cannot authenticate geometry or prove
  // that the stored candidates are the exact per-cone minima; use the fresh
  // verifier below for that statement.
  [[nodiscard]] bool locally_structurally_valid() const;

  friend bool operator==(
      const ExactYao48EmstResult&, const ExactYao48EmstResult&) = default;
};

// Yao's containment theorem applies because every chamber is isometric to
// x>=y>=z>=0, whose spherical diameter is arccos(1/sqrt(3)) < pi/3.  The
// canonical tie case is also strict: if uv is omitted at u in favour of an
// earlier equal-length uw, then |wv| < |uv|.  Both uw and wv precede uv in
// canonical Kruskal, contradicting selection of uv.  Thus choosing by
// (squared_length,u,v) retains the canonical complete-graph EMST, not merely
// some equal-weight EMST.  This producer is a bounded design oracle; a
// scalable implementation must replace only its exhaustive pair scan, not
// its exact cone and edge semantics.
[[nodiscard]] ExactYao48EmstResult build_exact_yao48_emst_reference(
    const spatial::CanonicalPointCloud& cloud);

struct ExactYao48EmstVerification {
  bool input_within_reference_bound{false};
  bool result_structurally_valid{false};
  bool directed_candidate_transcript_exact{false};
  bool unique_candidate_transcript_exact{false};
  bool canonical_kruskal_transcript_exact{false};
  bool exact_totals_match{false};
  bool counters_match{false};
  bool full_transcript_exact{false};

  [[nodiscard]] bool transcript_certified() const noexcept {
    return input_within_reference_bound && result_structurally_valid &&
           directed_candidate_transcript_exact &&
           unique_candidate_transcript_exact &&
           canonical_kruskal_transcript_exact && exact_totals_match &&
           counters_match && full_transcript_exact;
  }

  friend bool operator==(
      const ExactYao48EmstVerification&,
      const ExactYao48EmstVerification&) = default;
};

// Fresh reference replay.  Its implementation deliberately does not call the
// producer, its cone classifier, its edge constructor/comparator or its DSU.
// It recomputes and compares the complete candidate/dedup/Kruskal transcript.
[[nodiscard]] ExactYao48EmstVerification verify_exact_yao48_emst_reference(
    const spatial::CanonicalPointCloud& cloud,
    const ExactYao48EmstResult& result);

}  // namespace morsehgp3d::hierarchy
