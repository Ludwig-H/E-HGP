#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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

// A synchronous, derived handoff produced while recertifying one transition.
// It carries no durable or scientific authority.  The generic store neither
// interprets nor retains it; application code may derive a bounded projection
// type and consume it through the committed-prefix visitor below.
struct AtomicLinearRunAcceptedProjection {
  virtual ~AtomicLinearRunAcceptedProjection() = default;
};

struct AtomicLinearRunRecertification {
  AtomicLinearRunRecertification() = default;

  AtomicLinearRunRecertification(
      bool supplied_transition_recertified,
      bool supplied_payload_is_canonical,
      bool supplied_process_local_capability_absent,
      std::shared_ptr<const AtomicLinearRunAcceptedProjection>
          supplied_accepted_projection = {},
      bool supplied_operational_resource_failure = false)
      : transition_recertified(supplied_transition_recertified),
        payload_is_canonical(supplied_payload_is_canonical),
        process_local_capability_absent(
            supplied_process_local_capability_absent),
        accepted_projection(
            std::move(supplied_accepted_projection)),
        operational_resource_failure(
            supplied_operational_resource_failure) {}

  bool transition_recertified{false};
  bool payload_is_canonical{false};
  bool process_local_capability_absent{false};
  std::shared_ptr<const AtomicLinearRunAcceptedProjection>
      accepted_projection;
  // This is an operational refusal, never a negative scientific
  // certificate.  In particular, projection/replay allocation failures set
  // this bit and leave every scientific acceptance bit false.
  bool operational_resource_failure{false};

  [[nodiscard]] bool accepted() const noexcept {
    return transition_recertified && payload_is_canonical &&
           process_local_capability_absent &&
           !operational_resource_failure;
  }

  // Projection object identity is observational and deliberately absent from
  // equality.  Resource exhaustion remains part of the public decision.
  friend bool operator==(
      const AtomicLinearRunRecertification& left,
      const AtomicLinearRunRecertification& right) noexcept {
    return left.transition_recertified ==
               right.transition_recertified &&
           left.payload_is_canonical == right.payload_is_canonical &&
           left.process_local_capability_absent ==
               right.process_local_capability_absent &&
           left.operational_resource_failure ==
               right.operational_resource_failure;
  }
};

// A recertifier is decision-pure and scientifically idempotent under the same
// immutable authorities: invocation is not a commit signal and must not
// mutate scientific or durable state.  Observational counters and
// non-authoritative reconstruction caches may change.  Publication can still
// fail after acceptance, and recovery also recertifies a valid uncommitted
// suffix before deleting it under recovery_uncommitted_cleanup.
using AtomicLinearRunRecertifier = std::function<
    AtomicLinearRunRecertification(
        const AtomicLinearRunTransition&,
        AtomicLinearRunRecertificationPhase)>;

// Optional, derived replay sink for an already committed prefix.  Recovery
// invokes it once, in sequence order, after scientific recertification and
// before the transition's after-resource-gate boundary.  Its work is
// therefore included in the same measured recovery envelope.  It is never an
// authority and is not retained by the store; the transition reference and
// borrowed projection pointer are valid only for that invocation.  If it
// throws, the after gate is still closed and open_existing fails; callers must
// discard any partial derived state accumulated by the visitor after this or
// any later open failure.  The projection pointer may be null and must not be
// retained.
using AtomicLinearRunCommittedPrefixVisitor =
    std::function<void(
        const AtomicLinearRunTransition&,
        const AtomicLinearRunAcceptedProjection*)>;

enum class AtomicLinearRunResourceGateBoundary : std::uint8_t {
  before_recertification,
  after_recertification,
};

// Unlike the scientific recertifier, this session-local resource gate is
// deliberately stateful.  The store invokes it immediately before each
// decision-pure recertification.  Recovery closes it immediately after replay;
// publication keeps it through all reversible writes and closes it just
// before replacing HEAD, serially under the single-writer contract.  A false
// result or exception fails before any durable publication.
using AtomicLinearRunResourceGate = std::function<
    bool(
        const AtomicLinearRunTransition&,
        AtomicLinearRunRecertificationPhase,
        AtomicLinearRunResourceGateBoundary)>;

enum class AtomicLinearRunRecoveryFailureReason : std::uint8_t {
  resource_gate_before_recertification_rejected,
  resource_gate_after_recertification_rejected,
  recertification_resource_exhausted,
  scientific_recertification_rejected,
};

class AtomicLinearRunRecoveryError : public std::runtime_error {
 public:
  AtomicLinearRunRecoveryError(
      AtomicLinearRunRecoveryFailureReason reason,
      AtomicLinearRunRecertificationPhase phase,
      std::uint64_t sequence,
      std::string message);

  [[nodiscard]] AtomicLinearRunRecoveryFailureReason reason()
      const noexcept {
    return reason_;
  }

  [[nodiscard]] AtomicLinearRunRecertificationPhase phase()
      const noexcept {
    return phase_;
  }

  [[nodiscard]] std::uint64_t sequence() const noexcept {
    return sequence_;
  }

 private:
  AtomicLinearRunRecoveryFailureReason reason_;
  AtomicLinearRunRecertificationPhase phase_;
  std::uint64_t sequence_;
};

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

// Optional single-use process-local participant in the publication
// transaction.  It is called exactly once after scientific recertification
// and every reversible file write, while the resource gate is still active,
// but before HEAD is replaced.  An atomic rejection certifies no external or
// scientific mutation.  A commit declares an irreversible in-process change:
// any later gate or I/O failure forces reconstruction from authoritative
// HEAD.  An indeterminate outcome also fails closed and requires that same
// reconstruction.  Neither the callback nor its state is serialized or
// retained.
enum class AtomicLinearRunPreHeadCommitOutcome : std::uint8_t {
  rejected_atomically,
  committed,
  indeterminate,
};

using AtomicLinearRunPreHeadCommit = AtomicLinearRunPreHeadCommitOutcome (*)(
    const AtomicLinearRunTransition&,
    const AtomicLinearRunAcceptedProjection*,
    void*) noexcept;

struct AtomicLinearRunPublishOptions {
  AtomicLinearRunPublishObserver observer{};
  void* observer_state{};
  AtomicLinearRunPreHeadCommit pre_head_commit{};
  void* pre_head_commit_state{};
};

enum class AtomicLinearRunPublishDecision : std::uint8_t {
  durably_published,
  transition_shape_rejected,
  resource_gate_rejected,
  recertification_resource_exhausted,
  recertification_rejected,
  process_local_commit_rejected,
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
  bool process_local_commit_attempted{false};
  bool process_local_commit_succeeded{false};
  bool process_local_commit_indeterminate{false};
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
  std::size_t resource_gate_evaluation_count{};
  std::size_t resource_gate_rejection_count{};
  std::size_t process_local_commit_attempt_count{};
  std::size_t process_local_commit_success_count{};
  std::size_t process_local_commit_rejection_count{};
  std::size_t process_local_commit_indeterminate_count{};
  std::size_t removed_uncommitted_temporary_file_count{};
  std::size_t removed_uncommitted_final_file_count{};
  AtomicLinearRunExternalAnchor current_anchor{};
  bool writer_lock_acquired{false};
  bool opened_existing_run{false};
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

class AtomicLinearRunStoreBinding {
 public:
  AtomicLinearRunStoreBinding();
  AtomicLinearRunStoreBinding(
      const AtomicLinearRunStoreBinding&) noexcept = default;
  AtomicLinearRunStoreBinding& operator=(
      const AtomicLinearRunStoreBinding&) noexcept = default;
  AtomicLinearRunStoreBinding(
      AtomicLinearRunStoreBinding&& other) noexcept;
  AtomicLinearRunStoreBinding& operator=(
      AtomicLinearRunStoreBinding&& other) noexcept;

 private:
  friend class AtomicLinearRunStore;
  std::shared_ptr<const std::uint8_t> identity_;
};

class AtomicLinearRunStore {
 public:
  // The directory must already exist and be dedicated to this run.  Both
  // entry points acquire a nonblocking cooperative single-writer lock.
  [[nodiscard]] static AtomicLinearRunStore create_new(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate);

  // An application binding is a process-local capability.  It is neither
  // serialized nor derivable from the run contract, and lets a typed adapter
  // reject a store whose callbacks were installed by another context.
  [[nodiscard]] static AtomicLinearRunStore create_new_bound(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate,
      const AtomicLinearRunStoreBinding& application_binding);

  [[nodiscard]] static AtomicLinearRunStore open_existing(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor =
          std::nullopt);

  [[nodiscard]] static AtomicLinearRunStore open_existing(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      AtomicLinearRunCommittedPrefixVisitor committed_prefix_visitor);

  [[nodiscard]] static AtomicLinearRunStore open_existing_bound(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      const AtomicLinearRunStoreBinding& application_binding);

  [[nodiscard]] static AtomicLinearRunStore open_existing_bound(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract contract,
      AtomicLinearRunStoreLimits limits,
      AtomicLinearRunRecertifier recertifier,
      AtomicLinearRunResourceGate resource_gate,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      AtomicLinearRunCommittedPrefixVisitor committed_prefix_visitor,
      const AtomicLinearRunStoreBinding& application_binding);

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
  [[nodiscard]] bool bound_to(
      const AtomicLinearRunStoreBinding& application_binding)
      const noexcept;

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
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      AtomicLinearRunResourceGate resource_gate,
      AtomicLinearRunCommittedPrefixVisitor committed_prefix_visitor,
      std::optional<AtomicLinearRunStoreBinding> application_binding);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
