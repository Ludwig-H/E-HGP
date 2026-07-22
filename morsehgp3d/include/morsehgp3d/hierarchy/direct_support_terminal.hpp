#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_support_terminal_certificate_schema_version = 1U;
inline constexpr std::string_view direct_support_terminal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_support_terminal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_support_terminal_mode =
    "certified";
inline constexpr std::string_view direct_support_terminal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_support_terminal_proof_basis =
    "fresh_exact_pair_v1_and_grouped_higher_v2_replay_terminal_"
    "support_catalog_arities_two_through_four_v1";

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
  // The two source streams domain-separate their provenance digests.  Both
  // values are retained and freshly replayed; equality between lanes would be
  // a protocol error rather than evidence of a shared authority.
  contract::CanonicalId pair_canonical_cloud_digest{};
  contract::CanonicalId higher_canonical_cloud_digest{};
  contract::CanonicalId pair_lbvh_digest{};
  contract::CanonicalId higher_lbvh_digest{};
  contract::CanonicalId pair_semantic_digest{};
  contract::CanonicalId higher_semantic_digest{};
  std::array<ExactDirectSupportArityTerminalCertificate, 3>
      arity_certificates{};
  exact::BigInt exact_candidate_universe_size{0};
  std::size_t normalized_event_count{};
  std::size_t normalized_extra_shell_diagnostic_count{};
  bool source_authorities_match{false};
  bool source_requirements_match{false};
  bool pair_result_freshly_replayed{false};
  bool higher_result_freshly_replayed{false};
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
