#pragma once

#include "morsehgp3d/hierarchy/direct_at_least20_stream_view.hpp"
#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_resident_at_least20_adapter_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_resident_at_least20_adapter_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_resident_at_least20_adapter_profile = "hgp_reduced";
inline constexpr std::string_view direct_morse_resident_at_least20_adapter_mode =
    "certified_non_forgeable_atomic_resident_to_at_least20_adapter_v1";
inline constexpr std::string_view
    direct_morse_resident_at_least20_adapter_deployment_status =
        "bounded_reference_cpu_resident_downstream_integration";
inline constexpr std::string_view
    direct_morse_resident_at_least20_adapter_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_resident_at_least20_adapter_proof_basis =
        "genuine_move_only_resident_prepared_ticket_canonical_group_"
        "projection_all_view_allocations_and_digests_before_resident_commit_"
        "allocation_free_post_commit_record_comparison_and_view_publish_v1";

struct ExactDirectMorseResidentAtLeast20AdapterInitializationResult;
struct ExactDirectMorseResidentAtLeast20AdapterPreparationResult;
struct ExactDirectMorseResidentAtLeast20AdapterCommitResult;

enum class ExactDirectMorseResidentAtLeast20AdapterInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_resident_session_rejected,
  no_non_genesis_resident_session_rejected,
  no_resident_source_identity_rejected,
  no_view_initialization_rejected,
  no_allocation_failed,
  complete_certified_atomic_adapter_session,
};

enum class ExactDirectMorseResidentAtLeast20AdapterPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_adapter_not_initialized,
  no_resident_preparation_rejected,
  no_resident_authority_bundle_rejected,
  no_root_id_mirror_rejected,
  no_view_preparation_rejected,
  no_allocation_failed,
  complete_certified_atomic_prepared_batch,
};

enum class ExactDirectMorseResidentAtLeast20AdapterCommitDecision
    : std::uint8_t {
  not_committed,
  no_adapter_not_initialized,
  no_ticket_not_valid,
  no_foreign_ticket_rejected,
  no_resident_commit_rejected,
  complete_certified_atomic_resident_and_view_commit,
};

// Private, ownership-neutral seam shared by the autonomous adapter and the
// future single product coordinator.  It contains no resident capability and
// is callable only by those friends: each owner must first authenticate the
// bundle through its genuine resident or bridge ticket.  Keeping the mapping
// here prevents the product coordinator from duplicating root allocation or
// post-commit suffix verification while still letting it own K2Bridge+View
// instead of this autonomous Resident+View adapter.
class ExactDirectMorseResidentAtLeast20ProjectionSeam {
 private:
  enum class Decision : std::uint8_t {
    rejected,
    root_id_mirror_rejected,
    view_budget_rejected,
    allocation_failed,
    complete,
  };

  struct Projection {
    ExactDirectAtLeast20CommittedBatchInput view_input;
    std::vector<ExactDirectAtLeast20CommittedGroupInput> expected_groups;
    exact::ExactLevel squared_level{};
    std::size_t source_batch_index{};
    std::size_t source_epoch{};
    std::size_t order{};
    std::size_t group_record_count_before{};
    ExactDirectAtLeast20RootId next_root_id_before{};
    ExactDirectAtLeast20RootId next_root_id_after{};
  };

  struct Result {
    std::optional<Projection> projection;
    Decision decision{Decision::rejected};
  };

  [[nodiscard]] static Result project_verified_bundle(
      const ExactDirectMorseUnifiedResidentAuthorityBundle&,
      std::uint64_t,
      const contract::CanonicalId&,
      const contract::CanonicalId&,
      std::size_t,
      ExactDirectAtLeast20RootId,
      const ExactDirectAtLeast20StreamViewBudget&) noexcept;
  [[nodiscard]] static bool committed_suffix_matches(
      const Projection&,
      std::span<const ExactDirectMorseUnifiedResidentGroupRecord>) noexcept;

  friend class ExactDirectMorseResidentAtLeast20Adapter;
  friend class ExactDirectMorseResidentAtLeast20AdapterPreparedBatch;
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

// A ticket contains both private capabilities.  Public relative transcripts,
// copied receipts and scalar IDs cannot construct it or reach the private view
// preparation/commit seam.
class ExactDirectMorseResidentAtLeast20AdapterPreparedBatch {
 public:
  ExactDirectMorseResidentAtLeast20AdapterPreparedBatch() noexcept;
  ~ExactDirectMorseResidentAtLeast20AdapterPreparedBatch();
  ExactDirectMorseResidentAtLeast20AdapterPreparedBatch(
      ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&&) noexcept;
  ExactDirectMorseResidentAtLeast20AdapterPreparedBatch& operator=(
      ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&&) noexcept;
  ExactDirectMorseResidentAtLeast20AdapterPreparedBatch(
      const ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&) = delete;
  ExactDirectMorseResidentAtLeast20AdapterPreparedBatch& operator=(
      const ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] std::size_t source_batch_index() const noexcept;
  [[nodiscard]] ExactDirectAtLeast20RootId next_root_id_before()
      const noexcept;
  [[nodiscard]] ExactDirectAtLeast20RootId next_root_id_after()
      const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseResidentAtLeast20AdapterPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseResidentAtLeast20Adapter;
};

struct ExactDirectMorseResidentAtLeast20AdapterPreparationResult {
  std::optional<ExactDirectMorseResidentAtLeast20AdapterPreparedBatch> ticket;
  ExactDirectMorseUnifiedResidentPreparationDecision resident_decision{
      ExactDirectMorseUnifiedResidentPreparationDecision::not_certified};
  ExactDirectAtLeast20StreamConsumeDecision view_decision{
      ExactDirectAtLeast20StreamConsumeDecision::not_consumed};
  bool no_scientific_state_mutated{false};
  bool genuine_resident_ticket_capability_held{false};
  bool canonical_group_projection_certified{false};
  bool root_id_allocation_mirrored_from_genesis{false};
  bool view_allocations_and_digests_completed{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentAtLeast20AdapterPreparationDecision decision{
      ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
          not_prepared};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectMorseResidentAtLeast20AdapterCommitResult {
  ExactDirectMorseUnifiedResidentCommitResult resident_commit{};
  ExactDirectAtLeast20StreamConsumeResult view_commit{};
  bool ticket_consumed{false};
  bool expected_records_match_new_resident_suffix{false};
  bool view_commit_used_resident_capability{false};
  bool root_id_mirror_committed{false};
  bool source_and_view_chains_advanced_together{false};
  bool no_scientific_state_mutated_on_failure{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentAtLeast20AdapterCommitDecision decision{
      ExactDirectMorseResidentAtLeast20AdapterCommitDecision::not_committed};

  [[nodiscard]] bool certified_atomic_resident_and_view_commit()
      const noexcept;
};

// The autonomous integration seam deliberately owns both sessions.  Its
// resident accessor is const-only, so callers cannot commit around the view.
// A later normalized-H0 product coordinator may instead use the private view
// seam directly (it is already a friend of the view) to avoid duplicate
// resident ownership with the K2->K1 bridge.
class ExactDirectMorseResidentAtLeast20Adapter {
 public:
  static constexpr std::string_view backend =
      direct_morse_resident_at_least20_adapter_backend;
  static constexpr std::string_view profile =
      direct_morse_resident_at_least20_adapter_profile;
  static constexpr std::string_view mode =
      direct_morse_resident_at_least20_adapter_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_resident_at_least20_adapter_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_resident_at_least20_adapter_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_resident_at_least20_adapter_proof_basis;

  ExactDirectMorseResidentAtLeast20Adapter() noexcept;
  ~ExactDirectMorseResidentAtLeast20Adapter();
  ExactDirectMorseResidentAtLeast20Adapter(
      ExactDirectMorseResidentAtLeast20Adapter&&) noexcept;
  ExactDirectMorseResidentAtLeast20Adapter& operator=(
      ExactDirectMorseResidentAtLeast20Adapter&&) noexcept;
  ExactDirectMorseResidentAtLeast20Adapter(
      const ExactDirectMorseResidentAtLeast20Adapter&) = delete;
  ExactDirectMorseResidentAtLeast20Adapter& operator=(
      const ExactDirectMorseResidentAtLeast20Adapter&) = delete;

  [[nodiscard]] bool certified_adapter_session() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] std::size_t batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] ExactDirectAtLeast20RootId next_root_id() const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_scientific_digest()
      const noexcept;
  [[nodiscard]] const ExactDirectMorseUnifiedResidentSession& resident()
      const noexcept;
  [[nodiscard]] const ExactDirectAtLeast20StreamViewSession& view()
      const noexcept;

  [[nodiscard]] ExactDirectMorseResidentAtLeast20AdapterPreparationResult
  prepare_next() noexcept;
  [[nodiscard]] ExactDirectMorseResidentAtLeast20AdapterCommitResult commit(
      ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&& ticket) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseResidentAtLeast20Adapter(
      std::unique_ptr<Impl>) noexcept;
  friend ExactDirectMorseResidentAtLeast20AdapterInitializationResult
  initialize_exact_direct_morse_resident_at_least20_adapter(
      ExactDirectMorseUnifiedResidentSession&&,
      const ExactDirectAtLeast20StreamViewBudget&) noexcept;
};

struct ExactDirectMorseResidentAtLeast20AdapterInitializationResult {
  std::optional<ExactDirectMorseResidentAtLeast20Adapter> adapter;
  contract::CanonicalId source_scientific_digest{};
  bool resident_genesis_verified{false};
  bool source_identity_derived_from_verified_immutable_plan{false};
  bool root_id_allocation_mirrored_from_genesis{false};
  bool view_initialized_at_resident_genesis{false};
  bool no_global_facet_coface_or_gamma_catalog_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentAtLeast20AdapterInitializationDecision decision{
      ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
          not_initialized};

  [[nodiscard]] bool certified_initialized_adapter() const noexcept;
};

// This initializer is intentionally genesis-only.  No adapter checkpoint,
// restore or durable-resume claim exists in schema v1; a nonzero resident
// cursor must be rejected rather than paired with an invented view prefix.
[[nodiscard]] ExactDirectMorseResidentAtLeast20AdapterInitializationResult
initialize_exact_direct_morse_resident_at_least20_adapter(
    ExactDirectMorseUnifiedResidentSession&& resident,
    const ExactDirectAtLeast20StreamViewBudget& view_budget) noexcept;

}  // namespace morsehgp3d::hierarchy
