#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    exact_low_order_gabriel_skeleton_receipt_schema_version = 1U;
inline constexpr std::string_view exact_low_order_gabriel_skeleton_backend =
    "reference_cpu";
inline constexpr std::string_view exact_low_order_gabriel_skeleton_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_low_order_gabriel_skeleton_mode =
    "exact_low_order_gabriel_skeleton";

using ExactLowOrderGabrielEdgeIds = std::array<spatial::PointId, 2U>;
using ExactLowOrderGabrielTriangleIds = std::array<spatial::PointId, 3U>;

struct ExactLowOrderGabrielEdgeRecord {
  ExactLowOrderGabrielEdgeIds point_ids{};
  exact::ExactLevel squared_level{};

  friend bool operator==(
      const ExactLowOrderGabrielEdgeRecord&,
      const ExactLowOrderGabrielEdgeRecord&) = default;
};

enum class ExactLowOrderGabrielTriangleProvenance : std::uint8_t {
  pair = 1U,
  higher = 2U,
  pair_and_higher = 3U,
};

struct ExactLowOrderGabrielTriangleRecord {
  ExactLowOrderGabrielTriangleIds point_ids{};
  exact::ExactLevel squared_level{};
  ExactLowOrderGabrielTriangleProvenance provenance{
      ExactLowOrderGabrielTriangleProvenance::pair};

  friend bool operator==(
      const ExactLowOrderGabrielTriangleRecord&,
      const ExactLowOrderGabrielTriangleRecord&) = default;
};

enum class ExactLowOrderGabrielEventSource : std::uint8_t {
  pair,
  higher,
};

// A small binding receipt for an already produced stream.  It is not a new
// geometric authority: callers must obtain it from the same complete source
// result whose events are projected below.  The digest binds every field that
// can affect eligibility or a resulting PointId edge or triple.
struct ExactLowOrderGabrielEventSourceReceipt {
  std::uint32_t schema_version{
      exact_low_order_gabriel_skeleton_receipt_schema_version};
  ExactLowOrderGabrielEventSource source{ExactLowOrderGabrielEventSource::pair};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  contract::CanonicalId projection_payload_digest{};
  bool stream_complete{false};
  bool frontier_empty{false};
  bool absence_of_additional_supports_certified{false};
  bool relevant_extra_shell_diagnostics_absent{false};

  friend bool operator==(
      const ExactLowOrderGabrielEventSourceReceipt&,
      const ExactLowOrderGabrielEventSourceReceipt&) = default;
};

// These helpers only bind an observed result and expose its local terminal
// predicates.  They do not replay either source and cannot promote a result
// that the source itself did not close.
[[nodiscard]] ExactLowOrderGabrielEventSourceReceipt
make_exact_low_order_gabriel_pair_source_receipt(
    const ExactPairSupportStreamResult& source);

[[nodiscard]] ExactLowOrderGabrielEventSourceReceipt
make_exact_low_order_gabriel_higher_source_receipt(
    const ExactHigherSupportStreamResult& source);

enum class ExactLowOrderGabrielSkeletonDecision : std::uint8_t {
  invalid_source_requirements,
  pair_source_not_complete,
  higher_source_not_complete,
  relevant_extra_shell_diagnostics_present,
  receipt_mismatch,
  invalid_pair_event,
  invalid_higher_event,
  contradictory_rank_two_edge_levels,
  contradictory_rank_three_triangle_levels,
  locally_verified_regular_low_order_skeleton_projection,
};

struct ExactLowOrderGabrielSkeletonAudit {
  std::size_t pair_event_count{};
  std::size_t higher_event_count{};
  std::size_t rank_two_edge_occurrence_count{};
  std::size_t pair_triangle_occurrence_count{};
  std::size_t higher_triangle_occurrence_count{};
  std::size_t duplicate_rank_two_edge_occurrence_count{};
  std::size_t duplicate_triangle_occurrence_count{};
  std::size_t contradictory_rank_two_edge_level_count{};
  std::size_t contradictory_rank_three_triangle_level_count{};
  bool pair_receipt_verified{false};
  bool higher_receipt_verified{false};
  bool source_requirements_match{false};
  bool relevant_extra_shell_diagnostics_absent{false};
  bool rank_two_edges_canonical_sorted_unique{false};
  bool rank_three_triangles_canonical_sorted_unique{false};
  bool no_delaunay_or_forbidden_global_structure_materialized{true};
  bool hierarchy_reduction_performed{false};
  bool source_authority_replay_performed{false};

  friend bool operator==(
      const ExactLowOrderGabrielSkeletonAudit&,
      const ExactLowOrderGabrielSkeletonAudit&) = default;
};

// Local, output-proportional regular low-order Gabriel skeleton.  Pair events
// with closed_rank == 2 contribute rank_two_edges together with their exact
// squared level.  Under the regular-domain assumptions this graph contains an
// EMST, but this adapter performs neither Kruskal nor any forest reduction.
// Pair events with closed_rank == 3 contribute support[0], support[1],
// interior[0] to rank_three_triangles; higher events contribute their three
// support ids exactly when support_size == 3, closed_rank == 3 and the strict
// interior is empty.  Triangle provenance records whether either or both
// source lanes emitted the simplex.
//
// Records are sorted canonically by (squared_level, point_ids).  Repeated
// occurrences of one simplex are merged only when their exact levels agree;
// conflicting levels reject the whole projection.  The receipts below are
// locally derived payload bindings, not authority replays.  Callers must first
// replay/certify both source streams against their canonical cloud and LBVH
// authorities before treating this local projection as scientifically exact.
//
// Any relevant extra-shell diagnostic rejects the entire projection.  In
// particular, a degenerate k=1 EMST may need a Gabriel edge carried by an
// authenticated pair extra-shell diagnostic with no strict interior.  At
// k=2, the same pair lane can carry a rank-three right/cospherical simplex via
// a shell witness instead of a strict interior.  That separate authenticated
// diagnostic projection is intentionally unresolved here, so this adapter is
// regular-domain only.  No Delaunay structure, Gamma incidence, hierarchy or
// public status is produced.
struct ExactLowOrderGabrielSkeletonResult {
  ExactLowOrderGabrielSkeletonDecision decision{
      ExactLowOrderGabrielSkeletonDecision::invalid_source_requirements};
  std::vector<ExactLowOrderGabrielEdgeRecord> rank_two_edges;
  std::vector<ExactLowOrderGabrielTriangleRecord> rank_three_triangles;
  ExactLowOrderGabrielSkeletonAudit audit{};

  [[nodiscard]] bool locally_projected() const noexcept {
    return decision ==
               ExactLowOrderGabrielSkeletonDecision::
                   locally_verified_regular_low_order_skeleton_projection &&
           audit.pair_receipt_verified &&
           audit.higher_receipt_verified &&
           audit.source_requirements_match &&
           audit.relevant_extra_shell_diagnostics_absent &&
           audit.rank_two_edges_canonical_sorted_unique &&
           audit.rank_three_triangles_canonical_sorted_unique &&
           audit.no_delaunay_or_forbidden_global_structure_materialized &&
           !audit.hierarchy_reduction_performed &&
           !audit.source_authority_replay_performed;
  }

  friend bool operator==(
      const ExactLowOrderGabrielSkeletonResult&,
      const ExactLowOrderGabrielSkeletonResult&) = default;
};

[[nodiscard]] ExactLowOrderGabrielSkeletonResult
project_exact_low_order_gabriel_skeleton_from_events(
    const ExactPairSupportStreamResult& pair_source,
    const ExactLowOrderGabrielEventSourceReceipt& pair_receipt,
    const ExactHigherSupportStreamResult& higher_source,
    const ExactLowOrderGabrielEventSourceReceipt& higher_receipt);

}  // namespace morsehgp3d::hierarchy
