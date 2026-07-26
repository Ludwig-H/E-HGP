#pragma once

#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_candidate_cursor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace morsehgp3d::hierarchy {

using ExactSparseAnchoredPairRecord =
    std::variant<ExactPairSupportEvent,
                 ExactPairSupportExtraShellDiagnostic>;

// One non-authoritative handoff of the records retained since the
// preceding release.  The offsets are global in the owning session, so a
// future bounded durable sink can concatenate segments without retaining the
// complete output vector.  Releasing even an empty segment irreversibly opts
// the session out of the resident terminal-authority path.
struct ExactSparseAnchoredPairRecordSegment {
  std::size_t first_output_record_index{};
  std::size_t first_output_point_id_reference_index{};
  std::size_t event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t point_id_reference_count{};
  std::vector<ExactSparseAnchoredPairRecord> records;

  [[nodiscard]] std::size_t record_count() const noexcept {
    return records.size();
  }

  friend bool operator==(
      const ExactSparseAnchoredPairRecordSegment&,
      const ExactSparseAnchoredPairRecordSegment&) = default;
};

enum class ExactSparseAnchoredPairRecordKind : std::uint8_t {
  event,
  relevant_extra_shell_diagnostic,
};

struct ExactSparseAnchoredPairSessionAdvanceBudget {
  ExactMortonGroupedAnchoredPairCandidateWorkBudget candidate_cursor{};
  ExactAnchoredPairCandidateClassificationBudget classifier{};
  std::size_t maximum_emitted_record_count{};
  std::size_t maximum_emitted_point_id_reference_count{};

  friend bool operator==(
      const ExactSparseAnchoredPairSessionAdvanceBudget&,
      const ExactSparseAnchoredPairSessionAdvanceBudget&) = default;
};

// Immutable caps for one complete in-process session.  They bound every
// physical producer axis and the only dynamic output retained by P8l.  The
// directed prune mass is logical coverage and is deliberately not a work cap.
struct ExactSparseAnchoredPairSessionTotalCapacity {
  std::size_t maximum_schedule_advance_count{};
  std::size_t maximum_orientation_check_count{};
  std::size_t maximum_grouped_traversal_node_visit_count{};
  std::size_t maximum_grouped_traversal_exact_predicate_count{};
  std::size_t maximum_admitted_candidate_count{};
  std::size_t maximum_classification_node_visit_count{};
  std::size_t maximum_output_record_count{};
  std::size_t maximum_output_point_id_reference_count{};

  friend bool operator==(
      const ExactSparseAnchoredPairSessionTotalCapacity&,
      const ExactSparseAnchoredPairSessionTotalCapacity&) = default;
};

enum class ExactSparseAnchoredPairSessionStepKind : std::uint8_t {
  candidate_started,
  candidate_above_rank,
  candidate_record_ready,
  record_committed,
  certified_prune_accounted,
  group_complete,
  budget_exhausted,
  total_capacity_exhausted,
  complete,
};

enum class ExactSparseAnchoredPairSessionStopReason : std::uint8_t {
  none,
  schedule_advance_limit,
  orientation_check_limit,
  grouped_traversal_node_visit_limit,
  grouped_traversal_exact_predicate_limit,
  classification_node_visit_limit,
  emitted_record_limit,
  emitted_point_id_reference_limit,
  total_schedule_advance_capacity,
  total_orientation_check_capacity,
  total_grouped_traversal_node_visit_capacity,
  total_grouped_traversal_exact_predicate_capacity,
  total_admitted_candidate_capacity,
  total_classification_node_visit_capacity,
  total_output_record_capacity,
  total_output_point_id_reference_capacity,
};

struct ExactSparseAnchoredPairSessionStepWork {
  ExactMortonGroupedAnchoredPairCandidateStepWork candidate_cursor{};
  std::size_t classification_node_visit_count{};
  std::size_t emitted_record_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t authenticated_pruned_directed_pair_count{};

  friend bool operator==(
      const ExactSparseAnchoredPairSessionStepWork&,
      const ExactSparseAnchoredPairSessionStepWork&) = default;
};

struct ExactSparseAnchoredPairSessionAudit {
  std::size_t advance_call_count{};
  std::size_t point_count{};
  std::size_t directed_pair_universe_size{};
  std::size_t authenticated_prune_count{};
  std::size_t authenticated_pruned_directed_pair_count{};
  std::size_t admitted_candidate_count{};
  std::size_t candidate_started_count{};
  std::size_t classification_advance_count{};
  std::size_t classification_budget_exhaustion_count{};
  std::size_t classification_node_visit_count{};
  std::size_t classification_terminal_count{};
  std::size_t above_rank_count{};
  std::size_t candidate_record_ready_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_record_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t budget_exhaustion_count{};
  std::size_t total_capacity_exhaustion_count{};
  std::size_t maximum_live_candidate_count{};
  bool every_prune_recertified{true};
  bool directed_coverage_certified{false};
  bool orientation_partition_certified{false};
  bool candidate_classification_partition_certified{false};
  bool output_partition_certified{false};
  bool retained_records_certified{false};
  bool no_forbidden_global_structure_materialized{true};
  bool complete{false};
  bool poisoned{false};

  friend bool operator==(
      const ExactSparseAnchoredPairSessionAudit&,
      const ExactSparseAnchoredPairSessionAudit&) = default;
};

// One scalar observation from the owning session.  P8j certificates and P8k
// results are consumed internally and never become caller-supplied authority.
class ExactSparseAnchoredPairSessionStep {
 public:
  ExactSparseAnchoredPairSessionStep(
      const ExactSparseAnchoredPairSessionStep&) = delete;
  ExactSparseAnchoredPairSessionStep(
      ExactSparseAnchoredPairSessionStep&&) noexcept = default;
  ExactSparseAnchoredPairSessionStep& operator=(
      const ExactSparseAnchoredPairSessionStep&) = delete;
  ExactSparseAnchoredPairSessionStep& operator=(
      ExactSparseAnchoredPairSessionStep&&) noexcept = default;
  ~ExactSparseAnchoredPairSessionStep() = default;

  [[nodiscard]] ExactSparseAnchoredPairSessionStepKind kind() const noexcept {
    return kind_;
  }
  [[nodiscard]] ExactSparseAnchoredPairSessionStopReason stop_reason() const
      noexcept {
    return stop_reason_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionAdvanceBudget&
  requested_budget() const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionAdvanceBudget&
  requested_budget() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionAdvanceBudget&
  effective_budget() const & noexcept {
    return effective_budget_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionAdvanceBudget&
  effective_budget() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionStepWork& work() const
      & noexcept {
    return work_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionStepWork& work() const && =
      delete;
  [[nodiscard]] std::optional<std::array<spatial::PointId, 2>> support_ids()
      const noexcept {
    return support_ids_;
  }
  [[nodiscard]] std::optional<std::size_t> group_ordinal() const noexcept {
    return group_ordinal_;
  }
  [[nodiscard]] std::optional<std::size_t> output_record_index() const
      noexcept {
    return output_record_index_;
  }
  [[nodiscard]] std::optional<ExactSparseAnchoredPairRecordKind> record_kind()
      const noexcept {
    return record_kind_;
  }
  [[nodiscard]] bool session_complete_after_step() const noexcept {
    return session_complete_after_step_;
  }

  friend bool operator==(
      const ExactSparseAnchoredPairSessionStep&,
      const ExactSparseAnchoredPairSessionStep&) = default;

 private:
  ExactSparseAnchoredPairSessionStep() = default;

  ExactSparseAnchoredPairSessionStepKind kind_{
      ExactSparseAnchoredPairSessionStepKind::budget_exhausted};
  ExactSparseAnchoredPairSessionStopReason stop_reason_{
      ExactSparseAnchoredPairSessionStopReason::none};
  ExactSparseAnchoredPairSessionAdvanceBudget requested_budget_{};
  ExactSparseAnchoredPairSessionAdvanceBudget effective_budget_{};
  ExactSparseAnchoredPairSessionStepWork work_{};
  std::optional<std::array<spatial::PointId, 2>> support_ids_;
  std::optional<std::size_t> group_ordinal_;
  std::optional<std::size_t> output_record_index_;
  std::optional<ExactSparseAnchoredPairRecordKind> record_kind_;
  bool session_complete_after_step_{false};

  friend class ExactSparseAnchoredPairSession;
};

class ExactSparseAnchoredPairSession;

// Non-forgeable process-local proof that P8i--P8k covered the complete
// directed anchor/query product and retained every rank-relevant pair record.
// It is not a durable checkpoint, a hierarchy reduction or public exactness.
class ExactSparseAnchoredPairTerminalAuthority {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "bounded_sparse_anchored_pair_terminal_session";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view proof_basis =
      "authenticated_group_prune_mass_plus_exhaustive_orientations_and_"
      "single_active_exact_classifier_v1";

  ExactSparseAnchoredPairTerminalAuthority(
      const ExactSparseAnchoredPairTerminalAuthority&) = delete;
  ExactSparseAnchoredPairTerminalAuthority& operator=(
      const ExactSparseAnchoredPairTerminalAuthority&) = delete;
  ExactSparseAnchoredPairTerminalAuthority(
      ExactSparseAnchoredPairTerminalAuthority&& other) noexcept;
  ExactSparseAnchoredPairTerminalAuthority& operator=(
      ExactSparseAnchoredPairTerminalAuthority&&) = delete;
  ~ExactSparseAnchoredPairTerminalAuthority() = default;

  [[nodiscard]] bool bound_to(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
      ExactSparseAnchoredPairSessionTotalCapacity total_capacity) const
      noexcept;
  [[nodiscard]] bool sealed_in_process_terminal_authority() const noexcept;

  // Cursor-derived views return their zero/default sentinel after authority
  // revocation; sealed_in_process_terminal_authority() remains the scope gate.
  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const & noexcept;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionTotalCapacity&
  total_capacity() const & noexcept {
    return total_capacity_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionTotalCapacity&
  total_capacity() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionAudit& audit() const
      & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionAudit& audit() const && =
      delete;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit&
  candidate_audit() const & noexcept;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit&
  candidate_audit() const && = delete;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const & noexcept;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const && = delete;
  [[nodiscard]] std::span<const ExactSparseAnchoredPairRecord> records() const
      & noexcept {
    return records_;
  }
  [[nodiscard]] std::span<const ExactSparseAnchoredPairRecord> records() const
      && = delete;

  // One-way handoff for the next certified facade.  Successful release
  // revokes this authority so the retained records cannot be consumed twice.
  [[nodiscard]] std::vector<ExactSparseAnchoredPairRecord> release_records()
      &&;
  std::vector<ExactSparseAnchoredPairRecord> release_records() & = delete;

 private:
  ExactSparseAnchoredPairTerminalAuthority(
      ExactMortonGroupedAnchoredPairCandidateContext&& terminal_cursor,
      ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
      ExactSparseAnchoredPairSessionAudit audit,
      std::vector<ExactSparseAnchoredPairRecord>&& records) noexcept;

  std::optional<ExactMortonGroupedAnchoredPairCandidateContext>
      terminal_cursor_;
  ExactSparseAnchoredPairSessionTotalCapacity total_capacity_{};
  ExactSparseAnchoredPairSessionAudit audit_{};
  std::vector<ExactSparseAnchoredPairRecord> records_;

  friend class ExactSparseAnchoredPairSession;
};

class ExactSparseAnchoredPairSession {
 public:
  static constexpr std::string_view backend =
      ExactSparseAnchoredPairTerminalAuthority::backend;
  static constexpr std::string_view profile =
      ExactSparseAnchoredPairTerminalAuthority::profile;
  static constexpr std::string_view mode =
      ExactSparseAnchoredPairTerminalAuthority::mode;
  static constexpr std::string_view deployment_status =
      ExactSparseAnchoredPairTerminalAuthority::deployment_status;
  static constexpr std::string_view public_status =
      ExactSparseAnchoredPairTerminalAuthority::public_status;
  static constexpr std::string_view proof_basis =
      ExactSparseAnchoredPairTerminalAuthority::proof_basis;

  [[nodiscard]] static ExactSparseAnchoredPairSession start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
      ExactSparseAnchoredPairSessionTotalCapacity total_capacity);
  [[nodiscard]] static ExactSparseAnchoredPairSession start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig,
      ExactSparseAnchoredPairSessionTotalCapacity) = delete;
  [[nodiscard]] static ExactSparseAnchoredPairSession start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig,
      ExactSparseAnchoredPairSessionTotalCapacity) = delete;

  ExactSparseAnchoredPairSession(const ExactSparseAnchoredPairSession&) =
      delete;
  ExactSparseAnchoredPairSession& operator=(
      const ExactSparseAnchoredPairSession&) = delete;
  ExactSparseAnchoredPairSession(ExactSparseAnchoredPairSession&&) noexcept =
      default;
  ExactSparseAnchoredPairSession& operator=(
      ExactSparseAnchoredPairSession&&) = delete;
  ~ExactSparseAnchoredPairSession() = default;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool active_candidate() const noexcept {
    return ready() &&
        (pending_support_ids_.has_value() || active_classifier_.has_value());
  }
  [[nodiscard]] bool complete() const noexcept {
    return ready() && audit_.complete;
  }
  [[nodiscard]] bool total_capacity_exhausted() const noexcept {
    return total_capacity_exhausted_;
  }
  [[nodiscard]] bool poisoned() const noexcept {
    return audit_.poisoned;
  }
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;
  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return candidate_cursor_.maximum_closed_rank();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const & noexcept {
    return candidate_cursor_.schedule_config();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionTotalCapacity&
  total_capacity() const & noexcept {
    return total_capacity_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionTotalCapacity&
  total_capacity() const && = delete;
  [[nodiscard]] const ExactSparseAnchoredPairSessionAudit& audit() const
      & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactSparseAnchoredPairSessionAudit& audit() const && =
      delete;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit&
  candidate_audit() const & noexcept {
    return candidate_cursor_.audit();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit&
  candidate_audit() const && = delete;
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const & noexcept {
    return candidate_cursor_.schedule_audit();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const && = delete;
  [[nodiscard]] std::span<const ExactSparseAnchoredPairRecord> records() const
      & noexcept {
    return records_;
  }
  [[nodiscard]] std::span<const ExactSparseAnchoredPairRecord> records() const
      && = delete;

  // Moves only the currently resident suffix.  Scientific audit counters stay
  // cumulative, while subsequent record indices and segment offsets remain
  // global.  This handoff is not an authority or a durable publication.
  [[nodiscard]] ExactSparseAnchoredPairRecordSegment
  release_unsealed_record_segment() &;
  ExactSparseAnchoredPairRecordSegment
  release_unsealed_record_segment() && = delete;

  [[nodiscard]] ExactSparseAnchoredPairSessionStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactSparseAnchoredPairSessionAdvanceBudget budget) &;
  [[nodiscard]] ExactSparseAnchoredPairSessionStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactSparseAnchoredPairSessionAdvanceBudget) && = delete;

  [[nodiscard]] ExactSparseAnchoredPairTerminalAuthority seal() &&;
  ExactSparseAnchoredPairTerminalAuthority seal() & = delete;

 private:
  struct PrivateConstructionTag {};

  ExactSparseAnchoredPairSession(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
      ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
      std::size_t directed_pair_universe_size,
      PrivateConstructionTag);

  [[nodiscard]] ExactSparseAnchoredPairSessionStep make_step(
      ExactSparseAnchoredPairSessionStepKind kind,
      ExactSparseAnchoredPairSessionStopReason stop_reason,
      ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
      ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
      ExactSparseAnchoredPairSessionStepWork work) const;
  [[nodiscard]] ExactSparseAnchoredPairSessionStep exhaust_total_capacity(
      ExactSparseAnchoredPairSessionStopReason reason,
      ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
      ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
      ExactSparseAnchoredPairSessionStepWork work);
  [[nodiscard]] ExactSparseAnchoredPairSessionStep start_pending_candidate(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
      ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
      ExactSparseAnchoredPairSessionStepWork work);
  [[nodiscard]] ExactSparseAnchoredPairSessionStep advance_active_classifier(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
      ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
      ExactSparseAnchoredPairSessionStepWork work);
  [[nodiscard]] ExactSparseAnchoredPairSessionStep commit_ready_record(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactSparseAnchoredPairSessionAdvanceBudget requested_budget,
      ExactSparseAnchoredPairSessionAdvanceBudget effective_budget,
      ExactSparseAnchoredPairSessionStepWork work);
  [[nodiscard]] bool terminal_invariants_hold() const noexcept;
  void poison() noexcept {
    audit_.poisoned = true;
  }

  ExactMortonGroupedAnchoredPairCandidateContext candidate_cursor_;
  ExactSparseAnchoredPairSessionTotalCapacity total_capacity_{};
  std::optional<std::array<spatial::PointId, 2>> pending_support_ids_;
  std::optional<ExactAnchoredPairCandidateClassificationContext>
      active_classifier_;
  std::size_t active_classifier_node_visit_count_{};
  std::vector<ExactSparseAnchoredPairRecord> records_;
  std::size_t released_record_count_{};
  std::size_t released_event_count_{};
  std::size_t released_relevant_extra_shell_diagnostic_count_{};
  std::size_t released_point_id_reference_count_{};
  ExactSparseAnchoredPairSessionAudit audit_{};
  ExactSparseAnchoredPairSessionStopReason total_capacity_stop_reason_{
      ExactSparseAnchoredPairSessionStopReason::none};
  bool total_capacity_exhausted_{false};
  bool record_segment_released_{false};
  bool sealed_{false};
};

}  // namespace morsehgp3d::hierarchy
