#pragma once

#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_h0_run.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t sparse_direct_h0_candidate_merge_schema_version =
    1U;
inline constexpr std::string_view sparse_direct_h0_candidate_merge_backend =
    "reference_cpu";
inline constexpr std::string_view sparse_direct_h0_candidate_merge_profile =
    "hgp_reduced";
inline constexpr std::string_view sparse_direct_h0_candidate_merge_mode =
    "bounded_sparse_direct_h0_candidate_merge";
inline constexpr std::string_view
    sparse_direct_h0_candidate_merge_deployment_status = "architecture_only";
inline constexpr std::string_view
    sparse_direct_h0_candidate_merge_public_status = "not_claimed";

struct ExactSparseDirectH0PairSourceLocator {
  std::size_t source_output_record_index{};

  friend bool operator==(
      const ExactSparseDirectH0PairSourceLocator&,
      const ExactSparseDirectH0PairSourceLocator&) = default;
};

// Higher-support terminal segments no longer retain the interleaving of
// events, diagnostics and destroyed prune certificates.  A chunk/local-kind
// locator is therefore the strongest non-forged retained locator.
struct ExactSparseDirectH0HigherSourceLocator {
  std::uint64_t chunk_sequence{};
  std::size_t local_kind_index{};

  friend bool operator==(
      const ExactSparseDirectH0HigherSourceLocator&,
      const ExactSparseDirectH0HigherSourceLocator&) = default;
};

using ExactSparseDirectH0SourceLocator = std::variant<
    ExactSparseDirectH0PairSourceLocator,
    ExactSparseDirectH0HigherSourceLocator>;

struct ExactSparseDirectH0Candidate {
  ExactSparseDirectH0SourceLocator source_locator;
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4U> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};
  std::optional<std::size_t> birth_order;
  std::optional<std::size_t> saddle_order;

  friend bool operator==(
      const ExactSparseDirectH0Candidate&,
      const ExactSparseDirectH0Candidate&) = default;
};

struct ExactSparseDirectH0Diagnostic {
  ExactSparseDirectH0SourceLocator source_locator;
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4U> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactSparseDirectH0Diagnostic&,
      const ExactSparseDirectH0Diagnostic&) = default;
};

struct ExactSparseDirectH0CandidateRunLimits {
  std::size_t maximum_candidate_count{};
  std::size_t maximum_diagnostic_count{};
  std::size_t maximum_point_id_reference_count{};

  friend bool operator==(
      const ExactSparseDirectH0CandidateRunLimits&,
      const ExactSparseDirectH0CandidateRunLimits&) = default;
};

// One bounded sorted input run.  It deliberately has neither Phase-10 event
// indices nor terminal-facade, H0 or reduction authority.
struct ExactSparseDirectH0CandidateRun {
  std::uint32_t schema_version{
      sparse_direct_h0_candidate_merge_schema_version};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t source_point_id_reference_count{};
  std::vector<ExactSparseDirectH0Candidate> candidates;
  std::vector<ExactSparseDirectH0Diagnostic> diagnostics;
  bool contains_pair_candidates{false};
  bool contains_higher_candidates{false};
  bool candidates_strictly_sorted_by_terminal_facade_key{false};
  bool diagnostics_preserved{false};
  bool global_event_indices_assigned{false};
  bool hierarchy_reduction_performed{false};
  bool complete_h0_authority_claimed{false};

  friend bool operator==(
      const ExactSparseDirectH0CandidateRun&,
      const ExactSparseDirectH0CandidateRun&) = default;
};

// Exact cross-arity facade order: level, closed rank, strict interior ids,
// lexicographic support (shorter common prefix first), then exact center.
// Source locators and H0 roles do not participate in this key.
[[nodiscard]] bool exact_sparse_direct_h0_candidate_less(
    const ExactSparseDirectH0Candidate& left,
    const ExactSparseDirectH0Candidate& right);

[[nodiscard]] bool exact_sparse_direct_h0_candidate_key_equal(
    const ExactSparseDirectH0Candidate& left,
    const ExactSparseDirectH0Candidate& right);

[[nodiscard]] ExactSparseDirectH0CandidateRun
project_exact_sparse_direct_h0_pair_candidate_run(
    const ExactSparseAnchoredPairH0Run& source,
    ExactSparseDirectH0CandidateRunLimits limits);

// The authority remains unconsumed.  It must be a sealed terminal authority
// bound to its original cloud/LBVH, and segment_index selects one retained
// bounded segment.
[[nodiscard]] ExactSparseDirectH0CandidateRun
project_exact_sparse_direct_h0_higher_candidate_run(
    const ExactHigherSupportTerminalAuthority& authority,
    std::size_t segment_index,
    ExactSparseDirectH0CandidateRunLimits limits);

struct ExactSparseDirectH0MergeLimits {
  std::size_t maximum_input_run_count{};
  std::size_t maximum_output_candidate_count_per_page{};

  friend bool operator==(
      const ExactSparseDirectH0MergeLimits&,
      const ExactSparseDirectH0MergeLimits&) = default;
};

struct ExactSparseDirectH0MergeSourcePosition {
  std::size_t run_index{};
  std::size_t candidate_index{};

  friend bool operator==(
      const ExactSparseDirectH0MergeSourcePosition&,
      const ExactSparseDirectH0MergeSourcePosition&) = default;
};

// next_candidate_indices is aligned with the session-owned immutable runs.
// The last source position retains the complete boundary key without copying
// a candidate and its variable-size strict-interior list into every cursor.
struct ExactSparseDirectH0MergeCursor {
  std::vector<std::size_t> next_candidate_indices;
  std::size_t emitted_candidate_count{};
  std::optional<ExactSparseDirectH0MergeSourcePosition>
      last_emitted_position;

  friend bool operator==(
      const ExactSparseDirectH0MergeCursor&,
      const ExactSparseDirectH0MergeCursor&) = default;
};

struct ExactSparseDirectH0MergedPage {
  std::uint32_t schema_version{
      sparse_direct_h0_candidate_merge_schema_version};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::vector<ExactSparseDirectH0Candidate> candidates;
  ExactSparseDirectH0MergeCursor successor_cursor;
  bool merge_complete{false};
  bool candidates_strictly_sorted_by_terminal_facade_key{false};
  bool global_event_indices_assigned{false};
  bool hierarchy_reduction_performed{false};
  bool complete_h0_authority_claimed{false};

  friend bool operator==(
      const ExactSparseDirectH0MergedPage&,
      const ExactSparseDirectH0MergedPage&) = default;
};

struct ExactSparseDirectH0CandidateMergeAudit {
  std::size_t input_validation_pass_count{};
  std::size_t validated_run_count{};
  std::size_t validated_candidate_count{};
  std::size_t validated_diagnostic_count{};
  std::size_t emitted_page_count{};
  std::size_t emitted_candidate_count{};

  friend bool operator==(
      const ExactSparseDirectH0CandidateMergeAudit&,
      const ExactSparseDirectH0CandidateMergeAudit&) = default;
};

// Move the bounded runs into one in-process authority.  Construction performs
// the only full validation pass; subsequent pages inspect one head per run
// and at most the configured number of emitted candidates.  Owning the runs
// prevents caller mutation, cursor substitution and lifetime drift between
// pages.  This authority authenticates no durable recovery.
class ExactSparseDirectH0CandidateMergeSession {
 public:
  ExactSparseDirectH0CandidateMergeSession(
      std::vector<ExactSparseDirectH0CandidateRun>&& input_runs,
      ExactSparseDirectH0MergeLimits limits);

  ExactSparseDirectH0CandidateMergeSession(
      const ExactSparseDirectH0CandidateMergeSession&) = delete;
  ExactSparseDirectH0CandidateMergeSession& operator=(
      const ExactSparseDirectH0CandidateMergeSession&) = delete;
  ExactSparseDirectH0CandidateMergeSession(
      ExactSparseDirectH0CandidateMergeSession&&) = delete;
  ExactSparseDirectH0CandidateMergeSession& operator=(
      ExactSparseDirectH0CandidateMergeSession&&) = delete;

  [[nodiscard]] ExactSparseDirectH0MergedPage merge_next_page() &;
  ExactSparseDirectH0MergedPage merge_next_page() && = delete;

  [[nodiscard]] bool complete() const noexcept {
    return !poisoned_ &&
        cursor_.emitted_candidate_count == total_candidate_count_;
  }
  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_;
  }
  [[nodiscard]] std::size_t input_run_count() const noexcept {
    return input_runs_.size();
  }
  [[nodiscard]] const ExactSparseDirectH0MergeCursor& cursor()
      const noexcept {
    return cursor_;
  }
  [[nodiscard]] const ExactSparseDirectH0CandidateMergeAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  const std::vector<ExactSparseDirectH0CandidateRun> input_runs_;
  ExactSparseDirectH0MergeLimits limits_{};
  ExactSparseDirectH0MergeCursor cursor_{};
  std::size_t point_count_{};
  std::size_t effective_maximum_order_{};
  std::size_t total_candidate_count_{};
  ExactSparseDirectH0CandidateMergeAudit audit_{};
  bool poisoned_{false};
};

}  // namespace morsehgp3d::hierarchy
