#pragma once

#include "morsehgp3d/hierarchy/sparse_anchored_pair_chunk_run.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t sparse_anchored_pair_h0_run_schema_version =
    1U;
inline constexpr std::string_view sparse_anchored_pair_h0_run_backend =
    "reference_cpu";
inline constexpr std::string_view sparse_anchored_pair_h0_run_profile =
    "hgp_reduced";
inline constexpr std::string_view sparse_anchored_pair_h0_run_mode =
    "bounded_pair_only_event_candidates";
inline constexpr std::string_view
    sparse_anchored_pair_h0_run_deployment_status = "architecture_only";
inline constexpr std::string_view sparse_anchored_pair_h0_run_public_status =
    "not_claimed";

struct ExactSparseAnchoredPairH0RunLimits {
  std::size_t maximum_source_record_count{};
  std::size_t maximum_event_candidate_count{};
  std::size_t maximum_diagnostic_count{};
  std::size_t maximum_point_id_reference_count{};

  friend bool operator==(
      const ExactSparseAnchoredPairH0RunLimits&,
      const ExactSparseAnchoredPairH0RunLimits&) = default;
};

// source_output_record_index is a stable P8l output locator, not a final
// Phase-10 event_index or event_projection_index.
struct ExactSparseAnchoredPairH0EventCandidate {
  std::size_t source_output_record_index{};
  ExactPairSupportEvent event;
  std::optional<std::size_t> birth_order;
  std::optional<std::size_t> saddle_order;

  friend bool operator==(
      const ExactSparseAnchoredPairH0EventCandidate&,
      const ExactSparseAnchoredPairH0EventCandidate&) = default;
};

struct ExactSparseAnchoredPairH0Diagnostic {
  std::size_t source_output_record_index{};
  ExactPairSupportExtraShellDiagnostic diagnostic;

  friend bool operator==(
      const ExactSparseAnchoredPairH0Diagnostic&,
      const ExactSparseAnchoredPairH0Diagnostic&) = default;
};

// One bounded, pair-only derived run.  Event candidates use exactly the
// facade event ordering key.  Diagnostics retain P8l output order.
//
// TODO(14W): externally merge these bounded runs with the independently
// certified support-3/4 runs before assigning global Phase-10 indices or
// claiming a complete H0 authority.
struct ExactSparseAnchoredPairH0Run {
  std::uint32_t schema_version{sparse_anchored_pair_h0_run_schema_version};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t source_advance_count{};
  std::size_t successor_advance_count{};
  std::size_t first_source_output_record_index{};
  std::size_t first_source_point_id_reference_index{};
  std::size_t source_record_count{};
  std::size_t source_point_id_reference_count{};
  std::vector<ExactSparseAnchoredPairH0EventCandidate> event_candidates;
  std::vector<ExactSparseAnchoredPairH0Diagnostic> diagnostics;
  bool pair_only{true};
  bool candidates_sorted_by_terminal_facade_event_key{false};
  bool diagnostics_preserved{false};
  bool global_event_indices_assigned{false};
  bool supports_three_and_four_joined{false};
  bool complete_h0_authority_claimed{false};

  friend bool operator==(
      const ExactSparseAnchoredPairH0Run&,
      const ExactSparseAnchoredPairH0Run&) = default;
};

// This is kept public so a bounded external merge can use the exact same key
// without depending on private facade implementation details.
[[nodiscard]] bool exact_sparse_anchored_pair_h0_event_less(
    const ExactPairSupportEvent& left,
    const ExactPairSupportEvent& right);

[[nodiscard]] bool exact_sparse_anchored_pair_h0_event_candidate_less(
    const ExactSparseAnchoredPairH0EventCandidate& left,
    const ExactSparseAnchoredPairH0EventCandidate& right);

// Pure bounded projection of one already-recertified 14V chunk.  It performs
// local shape/accounting validation only and does not replay geometry.
[[nodiscard]] ExactSparseAnchoredPairH0Run
project_exact_sparse_anchored_pair_h0_run(
    const ExactSparseAnchoredPairRecertifiedChunkProjection& source,
    std::size_t point_count,
    std::size_t effective_maximum_order,
    ExactSparseAnchoredPairH0RunLimits limits);

}  // namespace morsehgp3d::hierarchy
