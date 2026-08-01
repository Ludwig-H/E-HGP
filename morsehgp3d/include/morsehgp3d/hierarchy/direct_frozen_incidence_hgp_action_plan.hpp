#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_incidence_quotient.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_frozen_incidence_hgp_action_plan_schema_version = 1U;
inline constexpr std::string_view
    direct_frozen_incidence_hgp_action_plan_backend = "reference_cpu";
inline constexpr std::string_view
    direct_frozen_incidence_hgp_action_plan_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_frozen_incidence_hgp_action_plan_mode =
        "certified_frozen_incidence_hgp_action_plan";
inline constexpr std::string_view
    direct_frozen_incidence_hgp_action_plan_public_status = "not_claimed";
inline constexpr std::string_view
    direct_frozen_incidence_hgp_action_plan_proof_basis =
        "fresh_typed_incidence_quotient_qr_action_partition_direct_"
        "or_silent_residual_scope_canonical_flat_plan_v1";

using ExactFrozenIncidencePriorRootId = std::uint64_t;

enum class ExactFrozenIncidenceHyperedgeProvenanceKind : std::uint8_t {
  direct_family,
  residual_incidence,
};

// Local delta metadata is deliberately scalar.  A zero-point certificate is
// authoritative only for this source incidence; no geometric cell, facet,
// Gamma structure or locator is consulted by this planner.
struct ExactFrozenIncidenceHyperedgeProvenance {
  std::size_t source_hyperedge_index{};
  ExactFrozenIncidenceHyperedgeProvenanceKind kind{
      ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family};
  std::size_t added_point_count{};
  bool zero_point_delta_certified{false};
  bool fully_redundant{false};

  friend bool operator==(
      const ExactFrozenIncidenceHyperedgeProvenance&,
      const ExactFrozenIncidenceHyperedgeProvenance&) = default;
};

// rooted_carrier.token_id is a carrier identity and is never reused as a
// prior-root identity.  This auxiliary table is therefore the minimal typed
// attachment needed by the HGP action layer.  Entries must be strictly
// carrier-id ordered and exhaustive over the quotient's rooted carriers.
struct ExactFrozenIncidenceRootAttachment {
  ExactFrozenIncidenceTokenId rooted_carrier_token_id{};
  ExactFrozenIncidencePriorRootId prior_root_id{};

  friend bool operator==(
      const ExactFrozenIncidenceRootAttachment&,
      const ExactFrozenIncidenceRootAttachment&) = default;
};

// All caps are checked before the quotient replay or plan allocations.  For
// H source hyperedges and R rooted attachments, conservative bounds are H
// groups, H direct references, H residual references, R prior-root
// references, R+3H scratch entries, and 2H+R flat output entries.
struct ExactFrozenIncidenceHgpActionPlanBudget {
  std::size_t maximum_source_hyperedge_count{};
  std::size_t maximum_provenance_count{};
  std::size_t maximum_root_attachment_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_direct_hyperedge_reference_count{};
  std::size_t maximum_residual_hyperedge_reference_count{};
  std::size_t maximum_prior_root_reference_count{};
  std::size_t maximum_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactFrozenIncidenceHgpActionPlanBudget&,
      const ExactFrozenIncidenceHgpActionPlanBudget&) = default;
};

enum class ExactFrozenIncidenceHgpAction : std::uint8_t {
  reduced_birth,
  continuation,
  multifusion,
};

// Direct and residual source-index slices are independently sorted.  The
// prior-root slice is strictly sorted, has cardinality q_R, and is empty for
// reduced_birth, singleton for continuation, and complete for multifusion.
// A fully redundant residual group is retained with its residual slice.
struct ExactFrozenIncidenceHgpActionGroup {
  std::size_t group_index{};
  std::size_t direct_hyperedge_offset{};
  std::size_t direct_hyperedge_count{};
  std::size_t residual_hyperedge_offset{};
  std::size_t residual_hyperedge_count{};
  std::size_t prior_root_offset{};
  std::size_t prior_root_count{};
  std::size_t q_r{};
  std::size_t direct_added_point_count{};
  std::size_t residual_added_point_count{};
  bool purely_residual{false};
  bool mixed_provenance{false};
  bool residual_zero_point_delta_certified{false};
  bool fully_redundant_residual_group{false};
  ExactFrozenIncidenceHgpAction action{
      ExactFrozenIncidenceHgpAction::reduced_birth};

  friend bool operator==(
      const ExactFrozenIncidenceHgpActionGroup&,
      const ExactFrozenIncidenceHgpActionGroup&) = default;
};

struct ExactFrozenIncidenceHgpActionPlanCounters {
  std::size_t source_hyperedge_count{};
  std::size_t provenance_count{};
  std::size_t root_attachment_count{};
  std::size_t group_count{};
  std::size_t direct_hyperedge_reference_count{};
  std::size_t residual_hyperedge_reference_count{};
  std::size_t prior_root_reference_count{};
  std::size_t reduced_birth_group_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t purely_residual_group_count{};
  std::size_t mixed_provenance_group_count{};
  std::size_t silent_residual_continuation_group_count{};
  std::size_t fully_redundant_residual_group_count{};
  std::size_t direct_added_point_count{};
  std::size_t residual_added_point_count{};
  std::size_t maximum_group_direct_hyperedge_count{};
  std::size_t maximum_group_residual_hyperedge_count{};
  std::size_t maximum_group_q_r{};
  std::size_t logical_output_entry_count{};
  std::size_t logical_scratch_entry_count{};

  friend bool operator==(
      const ExactFrozenIncidenceHgpActionPlanCounters&,
      const ExactFrozenIncidenceHgpActionPlanCounters&) = default;
};

enum class ExactFrozenIncidenceHgpActionPlanDecision : std::uint8_t {
  not_certified,
  no_frozen_incidence_hgp_action_plan_capacity_overflow,
  no_frozen_incidence_hgp_action_plan_budget_exhausted,
  no_frozen_incidence_hgp_action_plan_quotient_rejected,
  no_frozen_incidence_hgp_action_plan_input_shape_rejected,
  no_frozen_incidence_hgp_action_plan_scope_rejected,
  no_frozen_incidence_hgp_action_plan_allocation_failed,
  complete_certified_frozen_incidence_hgp_action_plan,
};

enum class ExactFrozenIncidenceHgpActionPlanScope : std::uint8_t {
  unspecified,
  exact_bounded_action_classification_from_fresh_frozen_incidence_quotient_only,
};

struct ExactFrozenIncidenceHgpActionPlanResult {
  static constexpr std::string_view backend =
      direct_frozen_incidence_hgp_action_plan_backend;
  static constexpr std::string_view profile =
      direct_frozen_incidence_hgp_action_plan_profile;
  static constexpr std::string_view mode =
      direct_frozen_incidence_hgp_action_plan_mode;
  static constexpr std::string_view public_status =
      direct_frozen_incidence_hgp_action_plan_public_status;
  static constexpr std::string_view proof_basis =
      direct_frozen_incidence_hgp_action_plan_proof_basis;

  std::uint32_t schema_version{
      direct_frozen_incidence_hgp_action_plan_schema_version};
  ExactFrozenIncidenceHgpActionPlanBudget requested_budget{};
  std::size_t required_source_hyperedge_capacity{};
  std::size_t required_provenance_capacity{};
  std::size_t required_root_attachment_capacity{};
  std::size_t required_group_capacity{};
  std::size_t required_direct_hyperedge_reference_capacity{};
  std::size_t required_residual_hyperedge_reference_capacity{};
  std::size_t required_prior_root_reference_capacity{};
  std::size_t required_scratch_entry_capacity{};
  std::size_t required_logical_output_entry_capacity{};
  std::vector<ExactFrozenIncidenceHgpActionGroup> groups;
  std::vector<std::size_t> direct_hyperedge_indices;
  std::vector<std::size_t> residual_hyperedge_indices;
  std::vector<ExactFrozenIncidencePriorRootId> prior_root_ids;
  ExactFrozenIncidenceHgpActionPlanCounters counters{};
  bool budget_preflight_certified{false};
  bool source_quotient_fresh_streaming_verified{false};
  bool provenance_shape_and_local_deltas_certified{false};
  bool rooted_carrier_attachments_exhaustive_and_injective{false};
  bool groups_and_source_slices_canonical{false};
  bool q_r_actions_certified{false};
  bool pure_residual_scope_rule_certified{false};
  bool carrier_presence_certified{false};
  bool fully_redundant_residual_groups_preserved{false};
  bool nodes_created_or_mutated{false};
  bool locator_or_caller_owned_state_mutated{false};
  bool geometry_gamma_facets_or_cells_materialized{false};
  bool public_status_claimed{false};
  ExactFrozenIncidenceHgpActionPlanDecision decision{
      ExactFrozenIncidenceHgpActionPlanDecision::not_certified};
  ExactFrozenIncidenceHgpActionPlanScope scope{
      ExactFrozenIncidenceHgpActionPlanScope::unspecified};

  // Constant-time completion predicate over sizes and self-reported facts.
  // Payload trust requires the fresh verifier below.
  [[nodiscard]] bool certified_frozen_incidence_hgp_action_plan()
      const noexcept;

  [[nodiscard]] bool atomic_empty_failure() const noexcept;

  friend bool operator==(
      const ExactFrozenIncidenceHgpActionPlanResult&,
      const ExactFrozenIncidenceHgpActionPlanResult&) = default;
};

// The trusted quotient CSR authority is supplied so the quotient can be
// freshly replayed rather than accepted through its constant-time predicate.
// This planner classifies actions only; it creates no node and mutates no
// locator, forest, disjoint set, caller arena, or global state.
[[nodiscard]] ExactFrozenIncidenceHgpActionPlanResult
build_exact_direct_frozen_incidence_hgp_action_plan(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget);

struct ExactFrozenIncidenceHgpActionPlanStreamingVerification {
  std::size_t source_hyperedge_scan_count{};
  std::size_t source_provenance_scan_count{};
  std::size_t source_root_attachment_scan_count{};
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool source_quotient_certified{false};
  bool groups_certified{false};
  bool direct_hyperedge_slices_certified{false};
  bool residual_hyperedge_slices_certified{false};
  bool prior_root_slices_certified{false};
  bool counters_certified{false};
  bool result_facts_certified{false};
  bool mutation_non_scope_certified{false};
  bool decision_and_scope_certified{false};
  bool no_second_persistent_output_arena_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactFrozenIncidenceHgpActionPlanStreamingVerification&,
      const ExactFrozenIncidenceHgpActionPlanStreamingVerification&) =
      default;
};

// Replays the quotient and local authorities, retaining only G expected group
// facts plus an R-sized prior-root index.  Observed flat slices are verified
// in place; no second H+G+R action payload is materialized.
[[nodiscard]] ExactFrozenIncidenceHgpActionPlanStreamingVerification
verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& trusted_budget,
    const ExactFrozenIncidenceHgpActionPlanResult& observed);

}  // namespace morsehgp3d::hierarchy
