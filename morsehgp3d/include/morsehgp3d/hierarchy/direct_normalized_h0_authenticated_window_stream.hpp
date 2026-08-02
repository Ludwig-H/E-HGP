#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_resident_batch_provider.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_authenticated_window_stream_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_authenticated_window_stream_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_authenticated_window_stream_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_authenticated_window_stream_mode =
        "provisional_sequential_authenticated_single_window_stream_v1";
inline constexpr std::string_view
    direct_normalized_h0_authenticated_window_stream_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_authenticated_window_stream_deployment_status =
        "architecture_only_no_resident_fold_no_source_exactness_authority";

// This cap admits exactly one ticket-owned window and O(1) session metadata.
// The batch-provider caps bound every variable arena copied into that ticket.
// A value other than one for maximum_outstanding_ticket_count is rejected:
// multiple prepared windows would invalidate the intended memory bound.
struct ExactDirectNormalizedH0AuthenticatedWindowStreamBudget {
  ExactDirectNormalizedH0ResidentBatchProviderBudget provider_window{};
  std::size_t maximum_outstanding_ticket_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget&,
      const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget&) =
      default;
};

enum class ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_manifest_rejected,
  no_provider_verification_rejected,
  no_provider_identity_rejected,
  no_budget_rejected,
  no_allocation_failed,
  complete_provisional_bound_stream,
};

enum class ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision
    : std::uint8_t {
  not_prepared,
  no_session_rejected,
  no_stream_sealed,
  no_source_exhausted,
  no_outstanding_ticket_budget_exhausted,
  no_provider_identity_rejected,
  no_provider_visit_rejected,
  no_provider_callback_cardinality_rejected,
  no_window_prefix_or_cursor_rejected,
  no_window_copy_rejected,
  no_allocation_failed,
  complete_provisional_owned_window,
};

enum class ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision
    : std::uint8_t {
  not_committed,
  no_ticket_invalid_or_consumed,
  no_foreign_ticket_rejected,
  no_stale_ticket_rejected,
  no_owned_window_rejected,
  complete_provisional_cursor_and_chain_commit,
};

enum class ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision
    : std::uint8_t {
  not_sealed,
  no_session_rejected,
  no_outstanding_ticket_rejected,
  no_source_not_exhausted,
  no_terminal_chain_mismatch,
  complete_provisional_terminal_seal,
};

enum class ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpointDecision
    : std::uint8_t {
  not_available,
  complete_honest_nonrestartable_metadata_checkpoint,
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult;

class ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow {
 public:
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow() noexcept;
  ~ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow();
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow(
      ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&&)
      noexcept;
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow& operator=(
      ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&&)
      noexcept;
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&) =
      delete;
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow& operator=(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&) =
      delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
  owned_window() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectNormalizedH0AuthenticatedWindowStreamSession;
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationResult {
  std::optional<
      ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow>
      ticket;
  std::size_t provider_callback_count{};
  bool one_window_owned{false};
  bool cursor_and_chain_unchanged{false};
  bool resident_scientific_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision decision{
      ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
          not_prepared};

  [[nodiscard]] bool certified_provisional_preparation() const noexcept;
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult {
  std::size_t source_batch_index{};
  std::size_t committed_cursor{};
  std::size_t committed_epoch{};
  contract::CanonicalId committed_chain_digest{};
  bool ticket_consumed{false};
  bool cursor_advanced_once{false};
  bool chain_advanced_to_ticket_successor{false};
  bool no_session_metadata_mutated_on_failure{false};
  bool resident_scientific_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision decision{
      ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
          not_committed};

  [[nodiscard]] bool certified_provisional_commit() const noexcept;
};

// Deliberately not a durable restore image.  In particular, the process-local
// provider identity and the absence of resident/locator state make restart
// unsupported.  This record exists so callers cannot mistake cursor metadata
// for a resumable scientific checkpoint.
struct ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint {
  std::uint32_t schema_version{
      direct_normalized_h0_authenticated_window_stream_schema_version};
  std::uint64_t stream_authority_id{};
  const void* process_local_provider_identity{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId current_chain_digest{};
  std::size_t batch_cursor{};
  std::size_t epoch{};
  bool stream_complete{false};
  bool stream_sealed{false};
  bool provider_identity_process_local_only{false};
  bool provisional_metadata_only{false};
  bool resident_or_locator_state_serialized{false};
  bool checkpoint_restore_supported{false};
  bool restartable{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpointDecision decision{
      ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpointDecision::
          not_available};

  [[nodiscard]] bool certified_honest_nonrestartable_checkpoint()
      const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint&,
      const ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint&) =
      default;
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal {
  std::uint32_t schema_version{
      direct_normalized_h0_authenticated_window_stream_schema_version};
  std::uint64_t stream_authority_id{};
  const void* process_local_provider_identity{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId final_chain_digest{};
  std::size_t committed_batch_count{};
  bool provider_identity_bound{false};
  bool complete_cursor_reached{false};
  bool terminal_chain_matches_manifest{false};
  bool provisional_metadata_only{false};
  bool resident_fold_executed{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  bool sealed{false};

  [[nodiscard]] bool certified_provisional_terminal_seal() const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal&,
      const ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal&) =
      default;
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamSealResult {
  std::optional<ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal>
      seal;
  bool newly_sealed{false};
  ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision decision{
      ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
          not_sealed};

  [[nodiscard]] bool certified_provisional_seal() const noexcept;
};

// The provider and its underlying object remain caller-owned, immutable and
// externally serialized for this session's lifetime.  This provisional seam
// authenticates the structural manifest/chain/cursor protocol only.  It never
// turns ProviderVerification into a scientific source authority and never
// mutates a resident locator, forest, hierarchy or output transcript.
class ExactDirectNormalizedH0AuthenticatedWindowStreamSession {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_authenticated_window_stream_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_authenticated_window_stream_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_authenticated_window_stream_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_authenticated_window_stream_public_status;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_authenticated_window_stream_deployment_status;

  ExactDirectNormalizedH0AuthenticatedWindowStreamSession() noexcept;
  ~ExactDirectNormalizedH0AuthenticatedWindowStreamSession();
  ExactDirectNormalizedH0AuthenticatedWindowStreamSession(
      ExactDirectNormalizedH0AuthenticatedWindowStreamSession&&) noexcept;
  ExactDirectNormalizedH0AuthenticatedWindowStreamSession& operator=(
      ExactDirectNormalizedH0AuthenticatedWindowStreamSession&&) noexcept;
  ExactDirectNormalizedH0AuthenticatedWindowStreamSession(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamSession&) =
      delete;
  ExactDirectNormalizedH0AuthenticatedWindowStreamSession& operator=(
      const ExactDirectNormalizedH0AuthenticatedWindowStreamSession&) =
      delete;

  [[nodiscard]] bool provisional_bound_stream() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool sealed() const noexcept;
  [[nodiscard]] std::size_t batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] const contract::CanonicalId& current_chain_digest()
      const noexcept;
  [[nodiscard]] const void* provider_identity() const noexcept;
  [[nodiscard]] bool source_exactness_claimed() const noexcept;
  [[nodiscard]] bool public_status_claimed() const noexcept;

  [[nodiscard]]
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationResult
  prepare_next();
  [[nodiscard]] ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult
  commit(
      ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&& ticket)
      noexcept;
  [[nodiscard]] ExactDirectNormalizedH0AuthenticatedWindowStreamSealResult
  seal() noexcept;
  [[nodiscard]] ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint
  make_checkpoint() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectNormalizedH0AuthenticatedWindowStreamSession(
      std::unique_ptr<Impl>) noexcept;
  friend struct
      ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult;
  friend ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult
  initialize_exact_direct_normalized_h0_authenticated_window_stream(
      const ExactDirectNormalizedH0ResidentSourceManifest&,
      const ExactDirectNormalizedH0ResidentProviderVerification&,
      ExactDirectNormalizedH0ResidentBatchProviderView,
      std::uint64_t,
      const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget&);
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult {
  std::optional<ExactDirectNormalizedH0AuthenticatedWindowStreamSession>
      session;
  bool manifest_certified{false};
  bool provider_verification_bound{false};
  bool provider_identity_bound{false};
  bool one_window_memory_bound{false};
  bool resident_scientific_state_initialized{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision
      decision{
          ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
              not_initialized};

  [[nodiscard]] bool certified_provisional_initialization() const noexcept;
};

[[nodiscard]]
ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult
initialize_exact_direct_normalized_h0_authenticated_window_stream(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentProviderVerification& verification,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    std::uint64_t stream_authority_id,
    const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget& budget);

}  // namespace morsehgp3d::hierarchy
