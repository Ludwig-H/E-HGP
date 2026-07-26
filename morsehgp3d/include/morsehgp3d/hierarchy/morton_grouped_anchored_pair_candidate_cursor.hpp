#pragma once

#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_schedule.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

enum class ExactMortonGroupedAnchoredPairCandidateStepKind : std::uint8_t {
  candidate_pair,
  certified_prune,
  group_complete,
  budget_exhausted,
  complete,
};

enum class ExactMortonGroupedAnchoredPairCandidateStopReason : std::uint8_t {
  none,
  schedule_advance_limit,
  orientation_check_limit,
  grouped_traversal_node_visit_limit,
  grouped_traversal_exact_predicate_limit,
};

// All four caps apply to one candidate-cursor advance.  Node visits and exact
// predicates are aggregate limits across every nested P8i advance made by the
// call, not fresh limits for each nested step.
struct ExactMortonGroupedAnchoredPairCandidateWorkBudget {
  std::size_t maximum_schedule_advance_count{};
  std::size_t maximum_orientation_check_count{};
  std::size_t maximum_grouped_traversal_node_visit_count{};
  std::size_t maximum_grouped_traversal_exact_predicate_count{};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairCandidateWorkBudget&,
      const ExactMortonGroupedAnchoredPairCandidateWorkBudget&) = default;
};

struct ExactMortonGroupedAnchoredPairCandidateStepWork {
  std::size_t schedule_advance_count{};
  std::size_t orientation_check_count{};
  std::size_t reverse_or_self_orientation_skip_count{};
  std::size_t grouped_traversal_node_visit_count{};
  std::size_t grouped_traversal_exact_predicate_count{};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairCandidateStepWork&,
      const ExactMortonGroupedAnchoredPairCandidateStepWork&) = default;
};

struct ExactMortonGroupedAnchoredPairCandidateAudit {
  std::size_t advance_call_count{};
  std::size_t schedule_advance_count{};
  std::size_t opened_unresolved_leaf_range_count{};
  std::size_t opened_fallback_subtree_range_count{};
  std::size_t orientation_check_count{};
  std::size_t reverse_or_self_orientation_skip_count{};
  std::size_t candidate_pair_count{};
  std::size_t certified_prune_count{};
  std::size_t completed_group_count{};
  std::size_t grouped_traversal_node_visit_count{};
  std::size_t grouped_traversal_exact_predicate_count{};
  std::size_t schedule_advance_budget_exhaustion_count{};
  std::size_t orientation_budget_exhaustion_count{};
  std::size_t grouped_traversal_budget_exhaustion_count{};
  std::size_t maximum_pending_leaf_count{};
  bool complete{false};
  bool no_dynamic_candidate_or_output_arena_materialized{true};

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairCandidateAudit&,
      const ExactMortonGroupedAnchoredPairCandidateAudit&) = default;
};

// A candidate_pair is only an oriented support proposal for P8c/P7b.  A
// certified_prune carries the nested P8i/P8h/P8g authority; every other step
// has no scientific authority.  The cursor stores only one pending Morton leaf
// range and two scalar offsets, and emits at most one typed event per call.
class ExactMortonGroupedAnchoredPairCandidateStep {
 public:
  ExactMortonGroupedAnchoredPairCandidateStep(
      const ExactMortonGroupedAnchoredPairCandidateStep&) = delete;
  ExactMortonGroupedAnchoredPairCandidateStep(
      ExactMortonGroupedAnchoredPairCandidateStep&&) noexcept = default;
  ExactMortonGroupedAnchoredPairCandidateStep& operator=(
      const ExactMortonGroupedAnchoredPairCandidateStep&) = delete;
  ExactMortonGroupedAnchoredPairCandidateStep& operator=(
      ExactMortonGroupedAnchoredPairCandidateStep&&) noexcept = default;
  ~ExactMortonGroupedAnchoredPairCandidateStep() = default;

  [[nodiscard]] ExactMortonGroupedAnchoredPairCandidateStepKind kind() const
      noexcept {
    return kind_;
  }

  [[nodiscard]] ExactMortonGroupedAnchoredPairCandidateStopReason stop_reason()
      const noexcept {
    return stop_reason_;
  }

  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateWorkBudget&
  requested_budget() const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateWorkBudget&
  requested_budget() const && = delete;

  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateStepWork& work()
      const & noexcept {
    return work_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateStepWork& work()
      const && = delete;

  [[nodiscard]] std::optional<std::size_t> group_ordinal() const noexcept {
    return group_ordinal_;
  }

  [[nodiscard]] std::optional<std::size_t> anchor_leaf_begin() const noexcept {
    return anchor_leaf_begin_;
  }

  [[nodiscard]] std::optional<std::size_t> anchor_leaf_end() const noexcept {
    return anchor_leaf_end_;
  }

  [[nodiscard]] std::optional<std::array<spatial::PointId, 2>> support_ids()
      const noexcept {
    return support_ids_;
  }

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleStep*
  schedule_step() const & noexcept {
    return schedule_step_.has_value() ? &*schedule_step_ : nullptr;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleStep*
  schedule_step() const && = delete;

  [[nodiscard]] bool cursor_complete_after_step() const noexcept {
    return cursor_complete_after_step_;
  }

  friend bool operator==(
      const ExactMortonGroupedAnchoredPairCandidateStep&,
      const ExactMortonGroupedAnchoredPairCandidateStep&) = default;

 private:
  ExactMortonGroupedAnchoredPairCandidateStep() = default;

  ExactMortonGroupedAnchoredPairCandidateStepKind kind_{
      ExactMortonGroupedAnchoredPairCandidateStepKind::budget_exhausted};
  ExactMortonGroupedAnchoredPairCandidateStopReason stop_reason_{
      ExactMortonGroupedAnchoredPairCandidateStopReason::none};
  ExactMortonGroupedAnchoredPairCandidateWorkBudget requested_budget_{};
  ExactMortonGroupedAnchoredPairCandidateStepWork work_{};
  std::optional<std::size_t> group_ordinal_;
  std::optional<std::size_t> anchor_leaf_begin_;
  std::optional<std::size_t> anchor_leaf_end_;
  std::optional<std::array<spatial::PointId, 2>> support_ids_;
  std::optional<ExactMortonGroupedAnchoredPairScheduleStep> schedule_step_;
  bool cursor_complete_after_step_{false};

  friend class ExactMortonGroupedAnchoredPairCandidateContext;
};

class ExactMortonGroupedAnchoredPairCandidateContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "bounded_morton_oriented_candidate_cursor";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view proof_basis =
      "unique_lower_PointId_anchor_group_owner_and_exhaustive_fail_open_"
      "terminal_range_orientation_v1";

  [[nodiscard]] static ExactMortonGroupedAnchoredPairCandidateContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config);
  [[nodiscard]] static ExactMortonGroupedAnchoredPairCandidateContext start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig) = delete;
  [[nodiscard]] static ExactMortonGroupedAnchoredPairCandidateContext start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t,
      ExactMortonGroupedAnchoredPairScheduleConfig) = delete;

  ExactMortonGroupedAnchoredPairCandidateContext(
      const ExactMortonGroupedAnchoredPairCandidateContext&) = delete;
  ExactMortonGroupedAnchoredPairCandidateContext& operator=(
      const ExactMortonGroupedAnchoredPairCandidateContext&) = delete;
  ExactMortonGroupedAnchoredPairCandidateContext(
      ExactMortonGroupedAnchoredPairCandidateContext&&) noexcept = default;
  ExactMortonGroupedAnchoredPairCandidateContext& operator=(
      ExactMortonGroupedAnchoredPairCandidateContext&&) = delete;
  ~ExactMortonGroupedAnchoredPairCandidateContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return schedule_.ready();
  }

  [[nodiscard]] bool complete() const noexcept {
    return ready() && !pending_leaf_range_ && schedule_.complete();
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept {
    return schedule_.validated_for(index, cloud);
  }

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return schedule_.maximum_closed_rank();
  }

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const & noexcept {
    return schedule_.config();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleConfig&
  schedule_config() const && = delete;

  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairCandidateAudit& audit()
      const && = delete;

  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const & noexcept {
    return schedule_.audit();
  }
  [[nodiscard]] const ExactMortonGroupedAnchoredPairScheduleAudit&
  schedule_audit() const && = delete;

  [[nodiscard]] ExactMortonGroupedAnchoredPairCandidateStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget budget) &;
  [[nodiscard]] ExactMortonGroupedAnchoredPairCandidateStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget) && = delete;

 private:
  struct PrivateConstructionTag {};

  ExactMortonGroupedAnchoredPairCandidateContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
      PrivateConstructionTag);

  [[nodiscard]] ExactMortonGroupedAnchoredPairCandidateStep make_step(
      ExactMortonGroupedAnchoredPairCandidateStepKind kind,
      ExactMortonGroupedAnchoredPairCandidateStopReason stop_reason,
      ExactMortonGroupedAnchoredPairCandidateWorkBudget budget,
      ExactMortonGroupedAnchoredPairCandidateStepWork work) const;

  ExactMortonGroupedAnchoredPairScheduleContext schedule_;
  std::size_t pending_leaf_cursor_{};
  std::size_t pending_leaf_end_{};
  std::size_t pending_anchor_offset_{};
  ExactMortonGroupedAnchoredPairCandidateAudit audit_{};
  bool pending_leaf_range_{false};
};

}  // namespace morsehgp3d::hierarchy
