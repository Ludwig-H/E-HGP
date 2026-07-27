#include "morsehgp3d/hierarchy/sparse_direct_h0_atomic_run_reader.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::size_t checked_accumulate(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum,
    const char* message) {
  if (increment > maximum || current > maximum - increment) {
    throw std::length_error(message);
  }
  return current + increment;
}

void validate_reader_contract(
    std::size_t point_count,
    std::size_t effective_maximum_order,
    const ExactSparseDirectH0AtomicPairRunReaderLimits& limits,
    const ExactSparseDirectH0AtomicPairRunVisitor& visitor) {
  if (point_count < 2U || effective_maximum_order == 0U ||
      effective_maximum_order > 10U ||
      effective_maximum_order > point_count || !visitor ||
      limits.pair_projection.maximum_source_record_count == 0U ||
      limits.pair_projection.maximum_event_candidate_count == 0U ||
      limits.pair_projection.maximum_point_id_reference_count == 0U ||
      limits.common_projection.maximum_candidate_count == 0U ||
      limits.common_projection.maximum_point_id_reference_count == 0U ||
      limits.maximum_run_count == 0U ||
      limits.maximum_total_candidate_count == 0U ||
      limits.maximum_total_point_id_reference_count == 0U) {
    throw std::invalid_argument(
        "a sparse direct H0 atomic pair reader contract is invalid");
  }
}

[[nodiscard]] bool strictly_sorted(
    const std::vector<ExactSparseDirectH0Candidate>& candidates) {
  for (std::size_t index = 1U; index < candidates.size(); ++index) {
    if (!exact_sparse_direct_h0_candidate_less(
            candidates[index - 1U], candidates[index])) {
      return false;
    }
  }
  return true;
}

}  // namespace

ExactSparseDirectH0AtomicPairRunReaderAudit
read_exact_sparse_direct_h0_atomic_pair_runs(
    const ExactSparseAnchoredPairChunkRunContext& source_context,
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits,
    AtomicLinearRunExternalAnchor final_anchor,
    std::size_t point_count,
    std::size_t effective_maximum_order,
    ExactSparseDirectH0AtomicPairRunReaderLimits limits,
    ExactSparseDirectH0AtomicPairRunVisitor visitor) {
  validate_reader_contract(
      point_count, effective_maximum_order, limits, visitor);
  const std::size_t expected_run_count = checked_size(
      final_anchor.committed_transition_count,
      "the sparse direct H0 final anchor count is not addressable");
  if (expected_run_count == 0U) {
    throw std::invalid_argument(
        "a sparse direct H0 finalized pair store cannot be empty");
  }
  if (expected_run_count > limits.maximum_run_count) {
    throw std::length_error(
        "a sparse direct H0 finalized pair store exceeds its run cap");
  }

  ExactSparseDirectH0AtomicPairRunReaderAudit audit;
  audit.callback_runs_borrowed = true;
  audit.runs_individually_strictly_sorted = true;
  std::size_t expected_source_advance_count = 0U;

  AtomicLinearRunStore store = source_context.open_existing_store(
      dedicated_directory,
      std::move(initial_output_chain_digest),
      std::move(store_limits),
      final_anchor,
      [&](const ExactSparseAnchoredPairRecertifiedChunkProjection& source) {
        const std::size_t run_index = audit.visited_transition_count;
        if (run_index >= expected_run_count ||
            source.source_advance_count !=
                expected_source_advance_count ||
            source.successor_advance_count <=
                source.source_advance_count) {
          throw std::invalid_argument(
              "a sparse direct H0 atomic pair prefix is not sequential");
        }
        const bool expected_terminal =
            run_index + 1U == expected_run_count;
        if (source.session_complete != expected_terminal ||
            source.total_capacity_exhausted ||
            source.run_advance_limit_reached) {
          throw std::invalid_argument(
              "a sparse direct H0 atomic pair prefix is not exactly finalized");
        }

        ExactSparseAnchoredPairH0Run pair_run =
            project_exact_sparse_anchored_pair_h0_run(
                source,
                point_count,
                effective_maximum_order,
                limits.pair_projection);
        ExactSparseDirectH0CandidateRun common_run =
            project_exact_sparse_direct_h0_pair_candidate_run(
                pair_run, limits.common_projection);
        if (common_run.point_count != point_count ||
            common_run.effective_maximum_order !=
                effective_maximum_order ||
            !common_run.contains_pair_candidates ||
            common_run.contains_higher_candidates ||
            !common_run.candidates_strictly_sorted_by_terminal_facade_key ||
            !common_run.diagnostics_preserved ||
            common_run.global_event_indices_assigned ||
            common_run.hierarchy_reduction_performed ||
            common_run.complete_h0_authority_claimed ||
            !strictly_sorted(common_run.candidates)) {
          throw std::logic_error(
              "a sparse direct H0 atomic reader projection broke 14X scope");
        }

        const std::size_t next_run_count = checked_accumulate(
            audit.emitted_run_count,
            1U,
            limits.maximum_run_count,
            "a sparse direct H0 atomic reader exceeded its run cap");
        const std::size_t next_candidate_count = checked_accumulate(
            audit.emitted_candidate_count,
            common_run.candidates.size(),
            limits.maximum_total_candidate_count,
            "a sparse direct H0 atomic reader exceeded its candidate cap");
        const std::size_t next_diagnostic_count = checked_accumulate(
            audit.emitted_diagnostic_count,
            common_run.diagnostics.size(),
            limits.maximum_total_diagnostic_count,
            "a sparse direct H0 atomic reader exceeded its diagnostic cap");
        const std::size_t next_reference_count = checked_accumulate(
            audit.emitted_point_id_reference_count,
            common_run.source_point_id_reference_count,
            limits.maximum_total_point_id_reference_count,
            "a sparse direct H0 atomic reader exceeded its reference cap");

        audit.maximum_reader_owned_live_run_count = std::max(
            audit.maximum_reader_owned_live_run_count, std::size_t{1U});
        visitor(run_index, common_run);

        audit.visited_transition_count = next_run_count;
        audit.emitted_run_count = next_run_count;
        audit.emitted_candidate_count = next_candidate_count;
        audit.emitted_diagnostic_count = next_diagnostic_count;
        audit.emitted_point_id_reference_count = next_reference_count;
        audit.terminal_projection_count +=
            static_cast<std::size_t>(source.session_complete);
        expected_source_advance_count =
            source.successor_advance_count;
      });

  const AtomicLinearRunStoreStatus& status = store.status();
  if (audit.visited_transition_count != expected_run_count ||
      audit.emitted_run_count != expected_run_count ||
      audit.terminal_projection_count != 1U ||
      status.committed_transition_count != expected_run_count ||
      status.recovered_transition_count != expected_run_count ||
      status.current_anchor != final_anchor ||
      !status.authoritative_head_certified ||
      !status.external_anchor_supplied ||
      !status.external_anchor_verified ||
      !status.linear_prefix_replayed ||
      status.retained_transition_history_count != 0U ||
      status.global_gamma_cell_count != 0U ||
      status.higher_order_delaunay_cell_count != 0U) {
    throw std::runtime_error(
        "a sparse direct H0 atomic pair reader did not certify its final prefix");
  }

  audit.source_retained_transition_history_count =
      status.retained_transition_history_count;
  audit.source_global_gamma_cell_count = status.global_gamma_cell_count;
  audit.source_higher_order_delaunay_cell_count =
      status.higher_order_delaunay_cell_count;
  audit.final_anchor = final_anchor;
  audit.final_anchor_exactly_verified = true;
  audit.terminal_projection_unique_and_last = true;
  return audit;
}

}  // namespace morsehgp3d::hierarchy
