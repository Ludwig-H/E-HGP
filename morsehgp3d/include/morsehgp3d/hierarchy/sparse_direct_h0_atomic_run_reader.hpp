#pragma once

#include "morsehgp3d/hierarchy/sparse_direct_h0_candidate_merge.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    sparse_direct_h0_atomic_run_reader_schema_version = 1U;
inline constexpr std::string_view
    sparse_direct_h0_atomic_run_reader_backend = "reference_cpu";
inline constexpr std::string_view
    sparse_direct_h0_atomic_run_reader_profile = "hgp_reduced";
inline constexpr std::string_view
    sparse_direct_h0_atomic_run_reader_mode =
        "certified_bounded_atomic_pair_run_reader";
inline constexpr std::string_view
    sparse_direct_h0_atomic_run_reader_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    sparse_direct_h0_atomic_run_reader_public_status = "not_claimed";

struct ExactSparseDirectH0AtomicPairRunReaderLimits {
  ExactSparseAnchoredPairH0RunLimits pair_projection;
  ExactSparseDirectH0CandidateRunLimits common_projection;
  std::size_t maximum_run_count{};
  std::size_t maximum_total_candidate_count{};
  std::size_t maximum_total_diagnostic_count{};
  std::size_t maximum_total_point_id_reference_count{};

  friend bool operator==(
      const ExactSparseDirectH0AtomicPairRunReaderLimits&,
      const ExactSparseDirectH0AtomicPairRunReaderLimits&) = default;
};

struct ExactSparseDirectH0AtomicPairRunReaderAudit {
  std::size_t visited_transition_count{};
  std::size_t emitted_run_count{};
  std::size_t emitted_candidate_count{};
  std::size_t emitted_diagnostic_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t terminal_projection_count{};
  std::size_t maximum_reader_owned_live_run_count{};
  std::size_t retained_reader_owned_run_history_count{};
  std::size_t source_retained_transition_history_count{};
  std::size_t source_global_gamma_cell_count{};
  std::size_t source_higher_order_delaunay_cell_count{};
  AtomicLinearRunExternalAnchor final_anchor{};
  bool final_anchor_exactly_verified{false};
  bool terminal_projection_unique_and_last{false};
  bool runs_individually_strictly_sorted{false};
  bool callback_runs_borrowed{false};
  bool global_merge_performed{false};
  bool global_event_indices_assigned{false};
  bool hierarchy_reduction_performed{false};
  bool complete_h0_authority_claimed{false};

  friend bool operator==(
      const ExactSparseDirectH0AtomicPairRunReaderAudit&,
      const ExactSparseDirectH0AtomicPairRunReaderAudit&) = default;
};

// The run reference is borrowed and remains valid only for the synchronous
// callback.  The reader owns at most one common run and retains no run history;
// a caller that copies callback data must account for that storage separately.
//
// Callback effects are provisional until this function returns successfully.
// A later transition or final-anchor failure requires the caller to discard all
// partial derived state, as required by AtomicLinearRunStore recovery visitors.
using ExactSparseDirectH0AtomicPairRunVisitor = std::function<void(
    std::size_t,
    const ExactSparseDirectH0CandidateRun&)>;

// Reads one finalized Phase-14V pair store.  The required anchor must name the
// exact final prefix, not merely an older accepted prefix.  Every committed
// transition is freshly recertified by the supplied 14V context, projected by
// 14W and normalized by 14X before its one bounded run is borrowed by visitor.
// Runs are individually sorted; this function performs no cross-run merge.
[[nodiscard]] ExactSparseDirectH0AtomicPairRunReaderAudit
read_exact_sparse_direct_h0_atomic_pair_runs(
    const ExactSparseAnchoredPairChunkRunContext& source_context,
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits,
    AtomicLinearRunExternalAnchor final_anchor,
    std::size_t point_count,
    std::size_t effective_maximum_order,
    ExactSparseDirectH0AtomicPairRunReaderLimits limits,
    ExactSparseDirectH0AtomicPairRunVisitor visitor);

}  // namespace morsehgp3d::hierarchy
