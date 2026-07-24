#pragma once

#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_event_journal_schema_version =
    2U;
inline constexpr std::string_view direct_morse_event_journal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_event_journal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_event_journal_mode =
    "certified";
inline constexpr std::string_view
    direct_morse_event_journal_refinement_status = "partial_refinement";
inline constexpr std::string_view direct_morse_event_journal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_event_journal_proof_basis =
    "terminal_direct_supports_plus_implicit_canonical_singleton_prefix_"
    "exact_h0_role_projection_grouped_by_order_and_exact_level_v2";

// This first Phase-10 layer is a role journal, not a hierarchy.  It retains
// one constant-size projection per accepted direct support, one physical
// record per direct H0 role and one descriptor per non-empty (order, level)
// batch.  The canonical singleton projection and role prefixes are generated
// from PointId and never materialized.  Direct centers and interior sets
// remain authoritative in the source facade and are deliberately not copied.
enum class ExactDirectMorseEventSource : std::uint8_t {
  canonical_singleton,
  direct_support_terminal_event,
};

enum class ExactDirectMorseH0Role : std::uint8_t {
  birth,
  saddle,
};

struct ExactDirectMorseEventProjection {
  std::size_t event_projection_index{};
  ExactDirectMorseEventSource source{
      ExactDirectMorseEventSource::canonical_singleton};
  // A PointId for a singleton, an event_index for a direct support.
  std::size_t source_index{};
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactLevel squared_level{};
  std::size_t closed_rank{};
  std::optional<std::size_t> birth_order;
  std::optional<std::size_t> saddle_order;

  friend bool operator==(
      const ExactDirectMorseEventProjection&,
      const ExactDirectMorseEventProjection&) = default;
};

// Role records are stored in batch order.  The order and exact level live
// once in the referenced batch; the record itself carries no execution-order
// semantics between births and saddles at an equality level.
struct ExactDirectMorseH0RoleRecord {
  std::size_t role_record_index{};
  std::size_t batch_index{};
  std::size_t event_projection_index{};
  ExactDirectMorseH0Role role{ExactDirectMorseH0Role::birth};

  friend bool operator==(
      const ExactDirectMorseH0RoleRecord&,
      const ExactDirectMorseH0RoleRecord&) = default;
};

struct ExactDirectMorseH0Batch {
  std::size_t batch_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  std::size_t role_record_offset{};
  std::size_t role_record_count{};
  std::size_t birth_role_count{};
  std::size_t saddle_role_count{};

  friend bool operator==(
      const ExactDirectMorseH0Batch&,
      const ExactDirectMorseH0Batch&) = default;
};

enum class ExactDirectMorseEventJournalDecision : std::uint8_t {
  not_certified,
  no_journal_source_facade_not_terminal,
  no_journal_source_authority_mismatch,
  no_journal_source_facade_payload_inconsistent,
  no_journal_relevant_extra_shell_diagnostics,
  complete_certified_partial_refinement,
};

enum class ExactDirectMorseEventJournalScope : std::uint8_t {
  unspecified,
  canonical_singletons_and_terminal_direct_supports_h0_roles_only,
};

struct ExactDirectMorseEventJournalResult {
  static constexpr std::string_view backend =
      direct_morse_event_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_event_journal_profile;
  static constexpr std::string_view mode = direct_morse_event_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_morse_event_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_event_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_event_journal_proof_basis;

  std::uint32_t schema_version{direct_morse_event_journal_schema_version};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t source_direct_event_count{};
  std::size_t singleton_event_count{};
  std::size_t event_projection_count{};
  std::size_t role_record_count{};
  std::size_t batch_count{};
  std::size_t logical_linear_storage_entry_count{};
  std::size_t logical_linear_storage_entry_limit{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  // These two arenas contain only the direct suffix.  Their records retain
  // their logical indices after the implicit singleton prefix.
  std::vector<ExactDirectMorseEventProjection>
      materialized_direct_event_projections;
  std::vector<ExactDirectMorseH0RoleRecord>
      materialized_direct_role_records;
  std::vector<ExactDirectMorseH0Batch> batches;
  bool source_facade_terminal_certified{false};
  bool source_cloud_authorities_match{false};
  bool source_facade_payload_locally_consistent{false};
  bool no_relevant_extra_shell_diagnostics{false};
  bool canonical_singleton_births_complete{false};
  bool canonical_singletons_implicit_and_unmaterialized{false};
  bool direct_h0_roles_projected_exactly_once{false};
  bool batch_keys_strictly_increasing{false};
  bool role_records_canonical_and_partitioned{false};
  bool output_linear_in_singletons_and_direct_events{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};
  bool forest_or_gateway_attach_performed{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectMorseEventJournalDecision decision{
      ExactDirectMorseEventJournalDecision::not_certified};
  ExactDirectMorseEventJournalScope scope{
      ExactDirectMorseEventJournalScope::unspecified};

  [[nodiscard]] bool certified_partial_refinement() const;

  friend bool operator==(
      const ExactDirectMorseEventJournalResult&,
      const ExactDirectMorseEventJournalResult&) = default;
};

// A synchronous logical view over the physically sparse journal.  Generated
// singleton records are returned by value; direct records can be borrowed by
// reference.  The source result must remain alive, unmoved and immutable for
// the complete lifetime of the view.
class ExactDirectMorseEventJournalView {
 public:
  explicit ExactDirectMorseEventJournalView(
      const ExactDirectMorseEventJournalResult& source) noexcept;
  ExactDirectMorseEventJournalView(
      ExactDirectMorseEventJournalResult&&) = delete;
  ExactDirectMorseEventJournalView(
      const ExactDirectMorseEventJournalResult&&) = delete;

  [[nodiscard]] std::size_t event_projection_count() const noexcept;
  [[nodiscard]] std::size_t role_record_count() const noexcept;

  [[nodiscard]] ExactDirectMorseEventProjection event_projection_at(
      std::size_t logical_index) const;
  [[nodiscard]] ExactDirectMorseH0RoleRecord role_record_at(
      std::size_t logical_index) const;

  [[nodiscard]] std::span<const ExactDirectMorseEventProjection>
  materialized_direct_event_projections() const noexcept;
  [[nodiscard]] std::span<const ExactDirectMorseH0RoleRecord>
  materialized_direct_role_records() const noexcept;
  [[nodiscard]] const ExactDirectMorseEventProjection&
  materialized_direct_event_projection_at(
      std::size_t logical_index) const;
  [[nodiscard]] const ExactDirectMorseH0RoleRecord&
  materialized_direct_role_record_at(
      std::size_t logical_index) const;

 private:
  const ExactDirectMorseEventJournalResult* source_{};
};

// The cloud is external singleton authority.  The terminal facade remains
// external authority for arities two through four; this layer validates its
// local indexing/role invariants but does not replay Phase-9 geometry.
[[nodiscard]] ExactDirectMorseEventJournalResult
build_exact_direct_morse_event_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade);

struct ExactDirectMorseEventJournalVerification {
  bool source_facade_terminal_certified{false};
  bool source_authorities_accepted{false};
  bool event_projections_certified{false};
  bool role_records_certified{false};
  bool batches_certified{false};
  bool result_facts_certified{false};
  bool decision_and_scope_certified{false};
  bool fresh_projection_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventJournalVerification&,
      const ExactDirectMorseEventJournalVerification&) = default;
};

// Rebuilds every projection, role and batch from the external cloud and
// facade.  No observed journal field steers replay.
[[nodiscard]] ExactDirectMorseEventJournalVerification
verify_exact_direct_morse_event_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& observed);

// Validates the same complete source-relative contract without constructing
// a second projection/role/batch payload.  Exact cloud digests are replayed,
// then the canonical projections and the sorted role set are checked in
// place.  The input journal remains the only O(n+E) journal allocation.
struct ExactDirectMorseEventJournalStreamingVerification {
  std::size_t canonical_cloud_digest_pass_count{};
  std::size_t generated_singleton_projection_count{};
  std::size_t generated_singleton_role_record_count{};
  std::size_t event_projection_scan_count{};
  std::size_t role_record_scan_count{};
  std::size_t batch_scan_count{};
  bool source_facade_terminal_certified{false};
  bool source_authorities_accepted{false};
  bool event_projections_certified{false};
  bool role_records_certified{false};
  bool batches_certified{false};
  bool result_facts_certified{false};
  bool decision_and_scope_certified{false};
  bool constant_auxiliary_record_storage_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventJournalStreamingVerification&,
      const ExactDirectMorseEventJournalStreamingVerification&) = default;
};

[[nodiscard]] ExactDirectMorseEventJournalStreamingVerification
verify_exact_direct_morse_event_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
