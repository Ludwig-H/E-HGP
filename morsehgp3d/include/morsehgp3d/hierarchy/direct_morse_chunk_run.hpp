#pragma once

#include "morsehgp3d/hierarchy/atomic_linear_run_store.hpp"
#include "morsehgp3d/hierarchy/direct_morse_budget.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_chunk_run_schema_version = 1U;
inline constexpr std::string_view direct_morse_chunk_run_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_chunk_run_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_chunk_run_mode =
    "budgeted";
inline constexpr std::string_view direct_morse_chunk_run_deployment_status =
    "architecture_only";
inline constexpr std::string_view direct_morse_chunk_run_public_status =
    "not_claimed";

// These are recovery authorities, not durable payload.  A fresh context must
// receive them again when reopening the run.  Their budgets and strict
// pre-batch locator stamps are bound into the wire, while the witness is both
// bound and retained as compact provenance.
struct ExactDirectMorseChunkBatchReplayAuthority {
  struct ResourceEnvelope {
    // Trusted upper bounds for reconstructing the external locator prefix
    // replaying this one batch and synchronously consuming its accepted
    // projection during recovery.  Batches are replayed and consumed
    // sequentially, so byte reservations are maximized across a chunk while
    // time reservations are summed.  A reducer's persistent arenas belong in
    // session usage; its per-fold scratch belongs in this envelope.
    std::uint64_t device_reserved_bytes{};
    std::uint64_t host_reserved_bytes{};
    std::uint64_t scratch_reserved_bytes{};
    std::uint64_t operation_reserved_ns{};

    friend bool operator==(
        const ResourceEnvelope&,
        const ResourceEnvelope&) = default;
  };

  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      strict_pre_batch_locator_stamp{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparseFacetDescentBatchExecutionBudget execution_budget{};
  ExactDirectSparseFacetDescentClosureBudget closure_budget{};
  ResourceEnvelope resource_envelope{};

  friend bool operator==(
      const ExactDirectMorseChunkBatchReplayAuthority&,
      const ExactDirectMorseChunkBatchReplayAuthority&) = default;
};

// The resolver may reconstruct a historical locator prefix on demand.  It
// returns shared immutable ownership for the complete replay call, allowing
// massive runs to avoid retaining one locator object per source batch.
using ExactDirectMorseChunkBatchLocatorResolverFunction = std::function<
    std::shared_ptr<const ExactDirectSparsePositiveFacetLocator>(
        std::size_t,
        const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&)>;

// A non-owning function view.  It can only be formed from an lvalue callable,
// so the context cannot accidentally own an opaque closure retaining one
// historical locator, DSU or forest per batch.  The callable and everything
// it references must outlive the context and every recertifier copied from it.
class ExactDirectMorseChunkBatchLocatorResolverView {
 public:
  using Result =
      std::shared_ptr<const ExactDirectSparsePositiveFacetLocator>;

  ExactDirectMorseChunkBatchLocatorResolverView() = default;

  template <
      class Resolver,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cv_t<Resolver>,
              ExactDirectMorseChunkBatchLocatorResolverView> &&
              std::is_invocable_r_v<
                  Result,
                  const Resolver&,
                  std::size_t,
                  const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&>,
          int> = 0>
  ExactDirectMorseChunkBatchLocatorResolverView(
      Resolver& resolver) noexcept
      : state_(std::addressof(resolver)),
        invoke_([](
                    const void* state,
                    std::size_t batch_index,
                    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
                        stamp) -> Result {
          return std::invoke(
              *static_cast<const Resolver*>(state), batch_index, stamp);
        }) {}

  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && invoke_ != nullptr;
  }

  [[nodiscard]] Result operator()(
      std::size_t batch_index,
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp)
      const {
    if (!*this) {
      return {};
    }
    return invoke_(state_, batch_index, stamp);
  }

 private:
  using Invoke = Result (*)(
      const void*,
      std::size_t,
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&);

  const void* state_{};
  Invoke invoke_{};
};

// Every allocation and count decoded from an untrusted payload is checked
// against these caps first.  The AtomicLinearRunStore keeps its independent
// encoded-transition and whole-run caps.
struct ExactDirectMorseChunkRunLimits {
  std::size_t maximum_batch_count_per_chunk{};
  std::size_t maximum_science_byte_count_per_batch{};
  std::size_t maximum_total_resolved_key_count_per_chunk{};
  std::size_t maximum_total_arm_join_count_per_chunk{};
  std::size_t maximum_payload_byte_count{};
  std::uint64_t serialization_reserved_ns{};
  std::uint64_t checkpoint_reserved_ns{};
  std::uint64_t safety_margin_reserved_bytes{};

  friend bool operator==(
      const ExactDirectMorseChunkRunLimits&,
      const ExactDirectMorseChunkRunLimits&) = default;
};

// Session-local occupancy already committed outside this context.  Unlike
// the effective policy, these values do not enter the durable application
// digest: a restart may honestly report a different current machine load
// while recertifying the same run contract.
struct ExactDirectMorseChunkRunSessionUsage {
  std::uint64_t device_used_bytes{};
  std::uint64_t host_used_bytes{};
  std::uint64_t scratch_used_bytes{};
  std::uint64_t output_used_bytes{};

  friend bool operator==(
      const ExactDirectMorseChunkRunSessionUsage&,
      const ExactDirectMorseChunkRunSessionUsage&) = default;
};

struct ExactDirectMorseChunkRunAudit {
  std::size_t source_plan_reconstruction_count{};
  std::size_t indexed_batch_cursor_count{};
  std::size_t arbitrary_batch_replay_count{};
  std::size_t publication_recertification_count{};
  std::size_t recovery_recertification_count{};
  std::size_t cleanup_recertification_count{};
  std::size_t rejected_recertification_count{};
  std::size_t maximum_chunk_resolved_key_count{};
  std::size_t maximum_chunk_arm_join_count{};
  std::size_t maximum_retained_batch_count{};
  std::size_t payload_pre_retention_refusal_count{};
  std::size_t cursor_progression_count{};
  std::size_t context_owned_process_local_ticket_count{};
  std::size_t context_owned_historical_locator_count{};
  std::size_t context_owned_dsu_parent_count{};
  std::size_t context_owned_forest_record_count{};
  std::size_t context_owned_global_facet_or_coface_count{};
  std::size_t context_owned_cell_gamma_or_delaunay_count{};
  std::size_t maximum_live_external_locator_count{};
  std::size_t fresh_budget_evaluation_count{};
  std::size_t rejected_budget_evaluation_count{};
  bool external_locator_resolver_residency_audited{false};
  bool external_locator_resolver_cache_absence_claimed{false};
};

// One immutable, non-authoritative reducer input projected directly from the
// fresh 14D result that accepted a recovered transition.  It is never decoded
// a second time from the wire and retains no locator, DSU or forest state.
struct ExactDirectMorseRecertifiedBatchProjection {
  std::size_t source_batch_index{};
  std::size_t source_chunk_index{};
  std::size_t source_family_begin_index{};
  std::size_t source_family_end_index{};
  std::size_t source_arm_seed_begin_index{};
  std::size_t source_arm_seed_end_index{};
  std::size_t order{};
  exact::ExactLevel closed_batch_squared_level{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  ExactDirectSparseFacetDescentClosureBudget requested_closure_budget{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      strict_pre_batch_locator_stamp{};
  std::vector<ExactDirectSparseFacetDescentBatchResolvedKey>
      resolved_keys;
  std::vector<ExactDirectSparseFacetDescentBatchArmJoin> arm_joins;
  ExactDirectSparseFacetDescentBatchClosureSummary closure_summary{};
  ExactDirectSparseFacetDescentBatchExecutionCounters
      execution_counters{};

  friend bool operator==(
      const ExactDirectMorseRecertifiedBatchProjection&,
      const ExactDirectMorseRecertifiedBatchProjection&) = default;
};

// Lifetime is one synchronous committed-prefix visitor call.  The store sees
// only the polymorphic base and does not cache this one-chunk projection.
struct ExactDirectMorseRecertifiedChunkProjection final
    : AtomicLinearRunAcceptedProjection {
  ExactDirectMorseIndustrialChunk source_chunk{};
  std::vector<ExactDirectMorseRecertifiedBatchProjection> batches;

  friend bool operator==(
      const ExactDirectMorseRecertifiedChunkProjection& left,
      const ExactDirectMorseRecertifiedChunkProjection& right) {
    return left.source_chunk == right.source_chunk &&
           left.batches == right.batches;
  }
};

using ExactDirectMorseRecertifiedChunkVisitor =
    std::function<void(
        const ExactDirectMorseRecertifiedChunkProjection&)>;

// Immutable replay authority for one complete freshly reconstructed 14C plan.
// It preindexes one cursor per exact source batch in a single pass.  Calls may
// replay batches or complete 14A chunks in any order and never advance a
// session cursor.  Every referenced external authority must outlive this
// object and every recertifier returned from it; only a non-owning resolver
// view is copied into the shared state.  The context exclusively owns its
// fresh, sequential budget tracker.
class ExactDirectMorseChunkRunContext {
 public:
  ExactDirectMorseChunkRunContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_event_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
      const ExactDirectMorseIndustrialPlanConfig& industrial_config,
      const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
      const ExactDirectSparseFacetDescentBatchPlanResult& observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView locator_resolver,
      std::span<const ExactDirectMorseChunkBatchReplayAuthority>
          batch_authorities,
      ExactDirectMorseChunkRunLimits limits,
      std::unique_ptr<ExactDirectMorseBudgetTracker> budget_tracker,
      ExactDirectMorseChunkRunSessionUsage session_usage = {},
      const ExactDirectSparseFacetDescentClosureConfig& closure_config = {},
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first);

  ~ExactDirectMorseChunkRunContext();

  ExactDirectMorseChunkRunContext(
      const ExactDirectMorseChunkRunContext&) = delete;
  ExactDirectMorseChunkRunContext& operator=(
      const ExactDirectMorseChunkRunContext&) = delete;
  ExactDirectMorseChunkRunContext(
      ExactDirectMorseChunkRunContext&&) = delete;
  ExactDirectMorseChunkRunContext& operator=(
      ExactDirectMorseChunkRunContext&&) = delete;

  [[nodiscard]] const contract::CanonicalId&
  application_contract_digest() const noexcept;

  [[nodiscard]] AtomicLinearRunContract run_contract(
      contract::CanonicalId initial_checkpoint_digest,
      contract::CanonicalId initial_output_chain_digest) const;

  // These are the only supported Phase-15B store entry points.  They bind the
  // scientific recertifier and stateful resource gate from this same context.
  // Creation also preflights the initial durable HEAD before the generic
  // store may mutate the dedicated directory.
  [[nodiscard]] AtomicLinearRunStore create_new_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_checkpoint_digest,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_checkpoint_digest,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor =
          std::nullopt) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_checkpoint_digest,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      ExactDirectMorseRecertifiedChunkVisitor
          committed_prefix_visitor) const;

  // Read-only validation seam for deterministic scientific replay tests.  It
  // is not a store factory and performs no resource authorization.
  [[nodiscard]] AtomicLinearRunRecertification
  recertify_for_validation(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) const;

  // Forms one whole 14A chunk proposal.  This lower-level validation seam
  // releases its temporary budget at return; callers must not accumulate
  // retained proposals.  The integrated publish_next_chunk path is the
  // supported bounded path.
  [[nodiscard]] AtomicLinearRunChunkProposal prepare_chunk(
      const AtomicLinearRunTrustedState& trusted_state,
      std::size_t chunk_index) const;

  [[nodiscard]] AtomicLinearRunPublishResult publish_next_chunk(
      AtomicLinearRunStore& store,
      AtomicLinearRunPublishOptions options = {}) const;

  [[nodiscard]] ExactDirectMorseChunkRunAudit audit() const noexcept;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
