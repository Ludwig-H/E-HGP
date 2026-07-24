#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// Phase 15A is a generic local-filesystem transport.  It serializes compact
// application payloads, never process-local 14H ticket capabilities.  The
// caller-owned recertifier remains the scientific authority at publication
// and at every recovery replay.
inline constexpr std::uint32_t atomic_linear_run_store_schema_version = 1U;
inline constexpr std::uint32_t atomic_linear_run_transition_wire_version = 1U;
inline constexpr std::uint32_t atomic_linear_run_head_wire_version = 1U;
inline constexpr std::size_t
    atomic_linear_run_transition_fixed_wire_byte_count = 264U;
inline constexpr std::size_t atomic_linear_run_head_wire_byte_count = 192U;

struct AtomicLinearRunContract {
  // A digest of the application-level schema and authorities.  The store
  // derives a separate run_contract_digest that also binds its wire version
  // and the complete initial cursor and effective store limits.
  contract::CanonicalId application_contract_digest{};
  contract::CanonicalId initial_checkpoint_digest{};
  contract::CanonicalId initial_output_chain_digest{};
  std::uint64_t initial_chunk_index{};
  std::uint64_t initial_batch_index{};

  friend bool operator==(
      const AtomicLinearRunContract&,
      const AtomicLinearRunContract&) = default;
};

struct AtomicLinearRunStoreLimits {
  std::size_t maximum_committed_transition_count{};
  std::size_t maximum_payload_byte_count{};
  std::size_t maximum_encoded_transition_byte_count{};
  std::size_t maximum_total_encoded_transition_byte_count{};
  std::uint64_t maximum_batch_span{};

  friend bool operator==(
      const AtomicLinearRunStoreLimits&,
      const AtomicLinearRunStoreLimits&) = default;
};

// The store fills sequence, source checkpoint and output-chain fields.  The
// proposal contains no serialized in-process commit capability.
struct AtomicLinearRunChunkProposal {
  std::uint64_t chunk_index{};
  std::uint64_t batch_begin_index{};
  std::uint64_t batch_end_index{};
  contract::CanonicalId successor_checkpoint_digest{};
  contract::CanonicalId budget_snapshot_digest{};
  std::vector<std::uint8_t> payload;

  friend bool operator==(
      const AtomicLinearRunChunkProposal&,
      const AtomicLinearRunChunkProposal&) = default;
};

struct AtomicLinearRunTransition {
  std::uint32_t schema_version{
      atomic_linear_run_store_schema_version};
  contract::CanonicalId run_contract_digest{};
  std::uint64_t sequence{};
  std::uint64_t chunk_index{};
  std::uint64_t batch_begin_index{};
  std::uint64_t batch_end_index{};
  contract::CanonicalId source_checkpoint_digest{};
  contract::CanonicalId successor_checkpoint_digest{};
  contract::CanonicalId output_chain_digest{};
  contract::CanonicalId budget_snapshot_digest{};
  std::vector<std::uint8_t> payload;
  contract::CanonicalId wire_sha256{};

  friend bool operator==(
      const AtomicLinearRunTransition&,
      const AtomicLinearRunTransition&) = default;
};

enum class AtomicLinearRunRecertificationPhase : std::uint8_t {
  publication,
  recovery,
  recovery_uncommitted_cleanup,
};

struct AtomicLinearRunRecertification {
  bool transition_recertified{false};
  bool payload_is_canonical{false};
  bool process_local_capability_absent{false};

  [[nodiscard]] bool accepted() const noexcept {
    return transition_recertified && payload_is_canonical &&
           process_local_capability_absent;
  }

  friend bool operator==(
      const AtomicLinearRunRecertification&,
      const AtomicLinearRunRecertification&) = default;
};

// A recertifier is a pure, idempotent authority: invocation is not a commit
// signal and must not mutate external state.  Publication can still fail
// after acceptance, and recovery also recertifies a valid uncommitted suffix
// before deleting it under recovery_uncommitted_cleanup.
using AtomicLinearRunRecertifier = std::function<
    AtomicLinearRunRecertification(
        const AtomicLinearRunTransition&,
        AtomicLinearRunRecertificationPhase)>;

struct AtomicLinearRunTrustedState {
  std::uint64_t next_sequence{};
  std::uint64_t next_chunk_index{};
  std::uint64_t next_batch_index{};
  contract::CanonicalId checkpoint_digest{};
  contract::CanonicalId output_chain_digest{};

  friend bool operator==(
      const AtomicLinearRunTrustedState&,
      const AtomicLinearRunTrustedState&) = default;
};

// This compact witness belongs in an independent monotone store.  A local
// HEAD alone cannot detect coordinated rollback of the entire directory.
struct AtomicLinearRunExternalAnchor {
  std::uint64_t committed_transition_count{};
  std::uint64_t next_chunk_index{};
  std::uint64_t next_batch_index{};
  contract::CanonicalId checkpoint_digest{};
  contract::CanonicalId output_chain_digest{};

  friend bool operator==(
      const AtomicLinearRunExternalAnchor&,
      const AtomicLinearRunExternalAnchor&) = default;
};

enum class AtomicLinearRunPublishStage : std::uint8_t {
  transition_temporary_file_written,
  transition_temporary_file_synchronized_and_reread,
  transition_immutable_link_created,
  transition_temporary_link_removed,
  transition_directory_synchronized,
  head_temporary_file_written,
  head_temporary_file_synchronized_and_reread,
  head_replaced,
  head_directory_synchronized,
};

using AtomicLinearRunPublishObserver = void (*)(
    AtomicLinearRunPublishStage,
    void*) noexcept;

struct AtomicLinearRunPublishOptions {
  AtomicLinearRunPublishObserver observer{};
  void* observer_state{};
};

enum class AtomicLinearRunPublishDecision : std::uint8_t {
  durably_published,
  transition_shape_rejected,
  recertification_rejected,
  store_limit_rejected,
  retryable_io_failure,
  indeterminate_io_failure_reopen_required,
};

struct AtomicLinearRunPublishResult {
  AtomicLinearRunPublishDecision decision{
      AtomicLinearRunPublishDecision::transition_shape_rejected};
  AtomicLinearRunRecertification recertification{};
  AtomicLinearRunExternalAnchor current_anchor{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_transition_byte_count{};
  std::size_t encoded_transition_byte_count{};
  int system_error_number{};
  bool trusted_state_advanced{false};
};

struct AtomicLinearRunStoreStatus {
  std::size_t recovered_transition_count{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_transition_byte_count{};
  std::size_t maximum_observed_payload_byte_count{};
  std::size_t maximum_observed_encoded_transition_byte_count{};
  std::size_t publication_recertification_count{};
  std::size_t recovery_recertification_count{};
  std::size_t uncommitted_cleanup_recertification_count{};
  std::size_t removed_uncommitted_temporary_file_count{};
  std::size_t removed_uncommitted_final_file_count{};
  AtomicLinearRunExternalAnchor current_anchor{};
  bool writer_lock_acquired{false};
  bool authoritative_head_certified{false};
  bool external_anchor_supplied{false};
  bool external_anchor_verified{false};
  bool linear_prefix_replayed{false};
  bool failed_closed_reopen_required{false};
  bool process_local_ticket_serialized{false};
  std::size_t retained_transition_history_count{};
  std::size_t global_gamma_cell_count{};
  std::size_t higher_order_delaunay_cell_count{};
};

[[nodiscard]] contract::CanonicalId
compute_atomic_linear_run_contract_digest(
    const AtomicLinearRunContract& contract,
    const AtomicLinearRunStoreLimits& limits);

class AtomicLinearRunStore {
 public:
  // The directory must already exist and be dedicated to this run.  Both
  // entry points acquire a nonblocking cooperative single-writer lock.
  [[nodiscard]] static AtomicLinearRunStore create_new(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier);

  [[nodiscard]] static AtomicLinearRunStore open_existing(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor =
          std::nullopt);

  ~AtomicLinearRunStore();

  AtomicLinearRunStore(const AtomicLinearRunStore&) = delete;
  AtomicLinearRunStore& operator=(const AtomicLinearRunStore&) = delete;
  AtomicLinearRunStore(AtomicLinearRunStore&&) = delete;
  AtomicLinearRunStore& operator=(AtomicLinearRunStore&&) = delete;

  [[nodiscard]] AtomicLinearRunPublishResult publish_next(
      AtomicLinearRunChunkProposal proposal,
      AtomicLinearRunPublishOptions options = {});

  [[nodiscard]] const AtomicLinearRunTrustedState& trusted_state()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& run_contract_digest()
      const noexcept;
  [[nodiscard]] const AtomicLinearRunStoreStatus& status() const noexcept;

 private:
  enum class OpenMode : std::uint8_t {
    create_new,
    open_existing,
  };

  AtomicLinearRunStore(
      OpenMode mode,
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
