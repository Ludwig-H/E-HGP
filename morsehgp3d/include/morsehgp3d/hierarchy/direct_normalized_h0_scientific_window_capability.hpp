#pragma once

#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_h0_authenticated_window_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_scientific_window_capability_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_scientific_window_capability_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_scientific_window_capability_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_scientific_window_capability_mode =
        "fresh_source_replay_manifest_bound_single_window_horizontal_h0_v1";
inline constexpr std::string_view
    direct_normalized_h0_scientific_window_capability_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_scientific_window_capability_deployment_status =
        "architecture_only_transient_global_compatibility_rebuild_no_"
        "forest_commit_relative_frozen_builder_available_no_durable_restart";

// The adapter caps bound the one initialization-only compatibility rebuild.
// The rebuilt plan is hashed into the expected manifest and destroyed before
// a session is published.  Persistent state is therefore O(1) plus at most
// one ticket-owned provider window, but initialization is not yet massive.
struct ExactDirectNormalizedH0ScientificWindowCapabilityBudget {
  ExactDirectNormalizedH0ResidentAdapterBudget compatibility_adapter{};
  std::size_t maximum_compatibility_facet_count{};
  std::size_t maximum_compatibility_batch_count{};
  ExactDirectNormalizedH0AuthenticatedWindowStreamBudget stream{};

  friend bool operator==(
      const ExactDirectNormalizedH0ScientificWindowCapabilityBudget&,
      const ExactDirectNormalizedH0ScientificWindowCapabilityBudget&) =
      default;
};

enum class ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_budget_rejected,
  no_horizontal_incidence_authority_rejected,
  no_compatibility_rebuild_rejected,
  no_scientific_manifest_mismatch,
  no_provider_verification_rejected,
  no_structural_stream_rejected,
  no_allocation_failed,
  complete_scientifically_bound_window_stream,
};

enum class ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_session_rejected,
  no_underlying_preparation_rejected,
  no_window_scientific_binding_rejected,
  no_allocation_failed,
  complete_scientifically_bound_window,
};

enum class ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision
    : std::uint8_t {
  not_committed,
  no_ticket_invalid_or_consumed,
  no_foreign_or_stale_ticket_rejected,
  no_underlying_commit_rejected,
  complete_scientific_window_commit,
};

enum class ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision
    : std::uint8_t {
  not_sealed,
  no_session_rejected,
  no_underlying_seal_rejected,
  no_scientific_prefix_rejected,
  complete_scientific_terminal_seal,
};

struct ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult;
class ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder;

class ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow {
 public:
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow() noexcept;
  ~ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow();
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow(
      ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&&)
      noexcept;
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow& operator=(
      ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&&)
      noexcept;
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow(
      const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&) =
      delete;
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow& operator=(
      const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&) =
      delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
  owned_window() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  [[nodiscard]] bool scientifically_attests_owned_window() const noexcept;
  [[nodiscard]] std::size_t stable_facet_token_count() const noexcept;

  explicit ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectNormalizedH0ScientificWindowCapabilitySession;
  friend class ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder;
};

struct ExactDirectNormalizedH0ScientificWindowCapabilityPreparationResult {
  std::optional<
      ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow>
      ticket;
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision
      structural_decision{
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              not_prepared};
  std::size_t provider_callback_count{};
  bool horizontal_incidence_source_bound{false};
  bool exact_manifest_and_chain_bound{false};
  bool global_compatibility_plan_retained{false};
  bool resident_scientific_state_mutated{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision
      decision{
          ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
              not_prepared};

  [[nodiscard]] bool certified_scientific_preparation() const noexcept;
};

struct ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult {
  ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult
      structural_commit{};
  std::size_t source_batch_index{};
  std::size_t committed_cursor{};
  std::size_t committed_epoch{};
  contract::CanonicalId committed_chain_digest{};
  bool ticket_consumed{false};
  bool scientific_prefix_advanced_once{false};
  bool horizontal_incidence_source_bound{false};
  bool resident_scientific_state_mutated{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision decision{
      ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
          not_committed};

  [[nodiscard]] bool certified_scientific_commit() const noexcept;
};

struct ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal {
  std::uint32_t schema_version{
      direct_normalized_h0_scientific_window_capability_schema_version};
  std::uint64_t scientific_authority_id{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId horizontal_incidence_authority_digest{};
  contract::CanonicalId final_chain_digest{};
  std::size_t committed_batch_count{};
  bool horizontal_incidence_authority_freshly_verified{false};
  bool expected_manifest_freshly_reconstructed{false};
  bool every_committed_window_scientifically_bound{false};
  bool global_compatibility_plan_retained{false};
  bool resident_fold_executed{false};
  bool vertical_maps_complete{false};
  bool durable_restart_supported{false};
  bool public_status_claimed{false};
  bool sealed{false};

  [[nodiscard]] bool certified_scientific_terminal_seal() const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal&,
      const ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal&) =
      default;
};

struct ExactDirectNormalizedH0ScientificWindowCapabilitySealResult {
  std::optional<ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal>
      seal;
  ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision
      structural_decision{
          ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
              not_sealed};
  bool newly_sealed{false};
  ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision decision{
      ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
          not_sealed};

  [[nodiscard]] bool certified_scientific_seal() const noexcept;
};

class ExactDirectNormalizedH0ScientificWindowCapabilitySession {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_scientific_window_capability_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_scientific_window_capability_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_scientific_window_capability_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_scientific_window_capability_public_status;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_scientific_window_capability_deployment_status;

  ExactDirectNormalizedH0ScientificWindowCapabilitySession() noexcept;
  ~ExactDirectNormalizedH0ScientificWindowCapabilitySession();
  ExactDirectNormalizedH0ScientificWindowCapabilitySession(
      ExactDirectNormalizedH0ScientificWindowCapabilitySession&&) noexcept;
  ExactDirectNormalizedH0ScientificWindowCapabilitySession& operator=(
      ExactDirectNormalizedH0ScientificWindowCapabilitySession&&) noexcept;
  ExactDirectNormalizedH0ScientificWindowCapabilitySession(
      const ExactDirectNormalizedH0ScientificWindowCapabilitySession&) =
      delete;
  ExactDirectNormalizedH0ScientificWindowCapabilitySession& operator=(
      const ExactDirectNormalizedH0ScientificWindowCapabilitySession&) =
      delete;

  [[nodiscard]] bool certified_scientific_window_stream() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool sealed() const noexcept;
  [[nodiscard]] std::size_t batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] const contract::CanonicalId& current_chain_digest()
      const noexcept;
  [[nodiscard]] const void* provider_identity() const noexcept;
  [[nodiscard]] bool horizontal_incidence_source_bound() const noexcept;
  [[nodiscard]] bool global_compatibility_plan_retained() const noexcept;
  [[nodiscard]] bool public_status_claimed() const noexcept;

  [[nodiscard]]
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparationResult
  prepare_next();
  [[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult
  commit(
      ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&&
          ticket) noexcept;
  [[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilitySealResult
  seal() noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectNormalizedH0ScientificWindowCapabilitySession(
      std::unique_ptr<Impl>) noexcept;
  friend struct
      ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult;
  friend ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
  initialize_exact_direct_normalized_h0_scientific_window_capability(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const ExactDirectSupportTerminalFacade&,
      const ExactDirectMorseEventJournalResult&,
      const ExactDirectSaddleArmSeedBudget&,
      const ExactDirectSaddleArmSeedJournalResult&,
      const ExactDirectClosedSaddleIncidenceBudget&,
      const ExactDirectClosedSaddleIncidenceJournalResult&,
      const ExactDirectSparseGatewayCandidateBudget&,
      spatial::LbvhTraversalOrder,
      const ExactDirectSparseGatewayCandidateJournalResult&,
      const ExactDirectNormalizedH0SourcePlanBudget&,
      const ExactDirectNormalizedH0SourcePlanResult&,
      const ExactDirectRankWindowSaturatedH0Authority&,
      const ExactDirectNormalizedH0IncidenceReductionAuthority&,
      const ExactDirectNormalizedH0ResidentSourceManifest&,
      ExactDirectNormalizedH0ResidentBatchProviderView,
      std::uint64_t,
      const ExactDirectNormalizedH0ScientificWindowCapabilityBudget&);
};

struct ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult {
  std::optional<ExactDirectNormalizedH0ScientificWindowCapabilitySession>
      session;
  std::size_t horizontal_incidence_authority_verification_count{};
  // One replay belongs to the horizontal-incidence authority verification and
  // one to the transient compatibility rebuild.  Keeping the total explicit
  // prevents this architecture-only seam from hiding its duplicate global
  // source-plan cost.
  std::size_t normalized_source_plan_verification_count{};
  std::size_t provider_full_replay_count{};
  bool horizontal_incidence_authority_freshly_verified{false};
  bool compatibility_plan_transiently_reconstructed{false};
  bool expected_manifest_freshly_reconstructed{false};
  bool observed_manifest_recursively_equal{false};
  bool provider_identity_bound{false};
  bool every_provider_window_authenticated{false};
  bool global_compatibility_plan_retained{false};
  bool resident_scientific_state_initialized{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision
      decision{
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              not_initialized};

  [[nodiscard]] bool certified_scientific_initialization() const noexcept;
};

[[nodiscard]]
ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_exact_direct_normalized_h0_scientific_window_capability(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority& rank_window_authority,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        incidence_authority,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    std::uint64_t scientific_authority_id,
    const ExactDirectNormalizedH0ScientificWindowCapabilityBudget& budget);

}  // namespace morsehgp3d::hierarchy
