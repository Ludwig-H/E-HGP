#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/exact_direct_pair_terminal_authority.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_support_terminal_certificate_schema_version = 5U;
inline constexpr std::string_view direct_support_terminal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_support_terminal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_support_terminal_mode =
    "certified";
inline constexpr std::string_view direct_support_terminal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_support_terminal_proof_basis =
    "exactly_one_of_fresh_exact_pair_v1_or_sealed_sparse_anchored_pair_"
    "session_v1_or_sealed_transactional_direct_pair_terminal_authority_"
    "v1_and_either_fresh_grouped_higher_v2_replay_or_sealed_root_anchored_"
    "fixed_chunk_higher_run_terminal_support_catalog_arities_two_through_"
    "four_with_complete_normalized_output_digest_v5";

// This is a certificate for the direct support catalogue only.  It does not
// construct a hierarchy, publish forest semantics, or promote a public exact
// result.  The two source budgets remain separate because their work and
// checkpoint protocols are intentionally different.
struct ExactDirectSupportTerminalBudget {
  ExactPairSupportStreamBudget pair{};
  ExactHigherSupportStreamBudget higher{};

  friend bool operator==(
      const ExactDirectSupportTerminalBudget&,
      const ExactDirectSupportTerminalBudget&) = default;
};

struct ExactDirectSupportTerminalRequirements {
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};

  friend bool operator==(
      const ExactDirectSupportTerminalRequirements&,
      const ExactDirectSupportTerminalRequirements&) = default;
};

// Common, owning projection of a regular support event.  Only the first
// support_size entries of support_ids are meaningful.  The projection is
// output-proportional; it is never an arena indexed by the candidate universe.
struct ExactDirectSupportEvent {
  std::size_t event_index{};
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};
  std::optional<std::size_t> birth_order;
  std::optional<std::size_t> saddle_order;

  friend bool operator==(
      const ExactDirectSupportEvent&,
      const ExactDirectSupportEvent&) = default;
};

// Extra-shell diagnostics remain support-local.  The source streams retain a
// complete shell cardinality and one canonical witness, not a materialized
// shell or a cross-support degeneracy quotient.
struct ExactDirectSupportExtraShellDiagnostic {
  std::size_t diagnostic_index{};
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactDirectSupportExtraShellDiagnostic&,
      const ExactDirectSupportExtraShellDiagnostic&) = default;
};

struct ExactDirectSupportArityTerminalCertificate {
  std::uint8_t support_size{};
  exact::BigInt exact_candidate_universe_size{0};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  bool candidate_universe_size_certified{false};
  bool terminal_absence_of_additional_supports_certified{false};

  friend bool operator==(
      const ExactDirectSupportArityTerminalCertificate&,
      const ExactDirectSupportArityTerminalCertificate&) = default;
};

enum class ExactDirectSupportTerminalDecision : std::uint8_t {
  not_certified,
  source_result_not_certified,
  source_stream_not_terminal,
  complete_direct_support_catalog,
  complete_direct_support_catalog_with_relevant_extra_shell_diagnostics,
};

enum class ExactDirectSupportTerminalScope : std::uint8_t {
  unspecified,
  direct_support_catalog_arities_two_through_four_only,
};

enum class ExactDirectSupportHigherSourceKind : std::uint8_t {
  unspecified,
  fresh_resident_replay,
  sealed_anchored_fixed_chunk_run,
};

enum class ExactDirectSupportPairSourceKind : std::uint8_t {
  unspecified,
  fresh_resident_replay,
  sealed_sparse_anchored_session,
  sealed_transactional_pair_terminal_authority,
};

struct ExactDirectSupportTerminalCertificate {
  static constexpr std::string_view backend =
      direct_support_terminal_backend;
  static constexpr std::string_view profile =
      direct_support_terminal_profile;
  static constexpr std::string_view mode = direct_support_terminal_mode;
  static constexpr std::string_view public_status =
      direct_support_terminal_public_status;
  static constexpr std::string_view proof_basis =
      direct_support_terminal_proof_basis;

  std::uint32_t schema_version{
      direct_support_terminal_certificate_schema_version};
  ExactDirectSupportTerminalRequirements requirements{};
  ExactDirectSupportTerminalBudget requested_budget{};
  // The two source streams domain-separate their provenance digests.  Pair
  // results are freshly replayed.  The higher lane is either freshly replayed
  // or consumed from one root-anchored, move-only in-process authority;
  // equality between lane digests would be a protocol error rather than
  // evidence of a shared authority.
  contract::CanonicalId pair_canonical_cloud_digest{};
  contract::CanonicalId higher_canonical_cloud_digest{};
  contract::CanonicalId pair_lbvh_digest{};
  contract::CanonicalId higher_lbvh_digest{};
  contract::CanonicalId pair_semantic_digest{};
  contract::CanonicalId higher_semantic_digest{};
  // Binds every normalized support-2/3/4 event and diagnostic, including
  // derived H0 roles.  Pair-specific digests remain separate provenance
  // domains; this digest closes the complete facade payload.
  contract::CanonicalId normalized_terminal_output_digest{};
  std::array<ExactDirectSupportArityTerminalCertificate, 3>
      arity_certificates{};
  exact::BigInt exact_candidate_universe_size{0};
  std::size_t normalized_event_count{};
  std::size_t normalized_extra_shell_diagnostic_count{};
  bool source_authorities_match{false};
  bool source_requirements_match{false};
  ExactDirectSupportPairSourceKind pair_source_kind{
      ExactDirectSupportPairSourceKind::unspecified};
  // Complete provenance of the provider-neutral unordered transactional cut
  // and its exact terminal classifier.  It remains default-initialized for
  // fresh and sparse-anchored pair sources.
  ExactDirectPairTerminalAudit pair_transactional_audit{};
  // budget.pair is meaningful only for the legacy fresh P7b source.  A P8l
  // source carries its own schedule and eight immutable physical caps below.
  bool pair_legacy_budget_applicable{false};
  bool pair_result_freshly_replayed{false};
  std::size_t pair_sparse_maximum_closed_rank{};
  ExactMortonGroupedAnchoredPairScheduleConfig pair_sparse_schedule_config{};
  ExactSparseAnchoredPairSessionTotalCapacity pair_sparse_total_capacity{};
  std::size_t pair_directed_pair_universe_size{};
  std::size_t pair_authenticated_pruned_directed_pair_count{};
  std::size_t pair_orientation_check_count{};
  std::size_t pair_reverse_or_self_orientation_skip_count{};
  std::size_t pair_admitted_candidate_count{};
  std::size_t pair_classification_terminal_count{};
  std::size_t pair_above_rank_count{};
  std::size_t pair_terminal_record_count{};
  bool pair_sparse_directed_coverage_certified{false};
  bool pair_sparse_orientation_partition_certified{false};
  bool pair_sparse_classification_partition_certified{false};
  bool pair_sparse_output_partition_certified{false};
  bool pair_terminal_authority_consumed{false};
  bool pair_terminal_records_captured_once{false};
  contract::CanonicalId pair_terminal_output_digest{};
  bool higher_result_freshly_replayed{false};
  ExactDirectSupportHigherSourceKind higher_source_kind{
      ExactDirectSupportHigherSourceKind::unspecified};
  std::size_t higher_maximum_chunk_count{};
  std::size_t higher_anchored_chunk_count{};
  bool higher_root_anchored_run_certified{false};
  bool higher_terminal_authority_consumed{false};
  bool higher_terminal_records_captured_once{false};
  contract::CanonicalId higher_output_chain_digest{};
  contract::CanonicalId higher_terminal_checkpoint_digest{};
  bool pair_stream_terminal{false};
  bool higher_stream_terminal{false};
  bool all_arities_terminal{false};
  bool exact_candidate_universe_size_certified{false};
  bool normalized_records_canonical_and_indexed{false};
  bool output_only_normalization_certified{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};
  bool common_durable_checkpoint_certified{false};
  bool hierarchy_or_forest_certified{false};
  bool public_status_claimed{false};
  ExactDirectSupportTerminalDecision decision{
      ExactDirectSupportTerminalDecision::not_certified};
  ExactDirectSupportTerminalScope scope{
      ExactDirectSupportTerminalScope::unspecified};

  [[nodiscard]] bool terminal_catalog_certified() const;

  friend bool operator==(
      const ExactDirectSupportTerminalCertificate&,
      const ExactDirectSupportTerminalCertificate&) = default;
};

// Terminal facade consumed by the next streaming stage.  It normalizes only
// emitted events and diagnostics, then discards all pair/higher traversal and
// prune internals.  A nonterminal or unverified source produces no payload.
struct ExactDirectSupportTerminalFacade {
  ExactDirectSupportTerminalCertificate certificate{};
  std::vector<ExactDirectSupportEvent> events;
  std::vector<ExactDirectSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;

  [[nodiscard]] bool terminal_catalog_certified() const;

  friend bool operator==(
      const ExactDirectSupportTerminalFacade&,
      const ExactDirectSupportTerminalFacade&) = default;
};

[[nodiscard]] ExactDirectSupportTerminalFacade
build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    const ExactHigherSupportStreamResult& higher_result);

// Consumes a process-local root-anchored higher-support authority.  Its fixed
// per-chunk budget must equal budget.higher.  This avoids a second complete
// higher-support traversal but does not claim fresh replay, durability,
// hierarchy reduction or public exact status.
[[nodiscard]] ExactDirectSupportTerminalFacade
build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    ExactHigherSupportTerminalAuthority higher_authority);

// Consumes both process-local terminal authorities.  The pair lane is bound to
// the externally derived relevant closed rank and carries its P8l schedule and
// total capacities; no P7b result, budget or traversal replay is synthesized.
// The only requested stream budget is therefore the still-unchanged higher
// fixed-chunk budget.
[[nodiscard]] ExactDirectSupportTerminalFacade
build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    ExactSparseAnchoredPairTerminalAuthority pair_authority,
    ExactHigherSupportTerminalAuthority higher_authority);

// Consumes a provider-neutral, move-only support-2 authority produced from an
// authenticated unordered transactional pair cut, plus the unchanged sealed
// higher-support authority.  The host/fake source kind is accepted here for
// contract tests; deployment qualification remains a concern of the caller.
[[nodiscard]] ExactDirectSupportTerminalFacade
build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    ExactDirectPairTerminalAuthority pair_authority,
    ExactHigherSupportTerminalAuthority higher_authority);

struct ExactDirectSupportTerminalVerification {
  bool pair_source_result_freshly_replayed{false};
  bool higher_source_result_freshly_replayed{false};
  bool source_terminality_certified{false};
  bool certificate_certified{false};
  bool normalized_events_certified{false};
  bool normalized_extra_shell_diagnostics_certified{false};
  bool terminal_claim_certified{false};
  bool fresh_composition_certified{false};
  bool result_certified{false};
};

// Rebuilds the facade from the external cloud, LBVH, order, budgets and both
// observed source results.  No field of the observed facade steers replay.
[[nodiscard]] ExactDirectSupportTerminalVerification
verify_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    const ExactHigherSupportStreamResult& higher_result,
    const ExactDirectSupportTerminalFacade& observed);

}  // namespace morsehgp3d::hierarchy
