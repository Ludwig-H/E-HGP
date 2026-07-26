#pragma once

#include "morsehgp3d/hierarchy/atomic_linear_run_store.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    sparse_anchored_pair_chunk_run_schema_version = 1U;
inline constexpr std::string_view sparse_anchored_pair_chunk_run_backend =
    "reference_cpu";
inline constexpr std::string_view sparse_anchored_pair_chunk_run_profile =
    "hgp_reduced";
inline constexpr std::string_view sparse_anchored_pair_chunk_run_mode =
    "budgeted_sparse_pair_chunk_run";
inline constexpr std::string_view
    sparse_anchored_pair_chunk_run_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    sparse_anchored_pair_chunk_run_public_status = "not_claimed";

// All fields are immutable run-contract limits.  A chunk contains at most
// maximum_advance_count_per_chunk calls to P8l and only the records released
// by those calls.  The cumulative cap bounds a cold deterministic replay.
struct ExactSparseAnchoredPairChunkRunLimits {
  std::size_t maximum_advance_count_per_chunk{};
  std::size_t maximum_cumulative_advance_count{};
  std::size_t maximum_record_count_per_chunk{};
  std::size_t maximum_point_id_reference_count_per_chunk{};
  std::size_t maximum_exact_integer_byte_count{};
  std::size_t maximum_total_exact_integer_byte_count_per_chunk{};
  std::size_t maximum_payload_byte_count{};

  friend bool operator==(
      const ExactSparseAnchoredPairChunkRunLimits&,
      const ExactSparseAnchoredPairChunkRunLimits&) = default;
};

struct ExactSparseAnchoredPairChunkRunAudit {
  std::size_t producer_root_reconstruction_count{};
  std::size_t verifier_root_reconstruction_count{};
  std::size_t producer_advance_count{};
  std::size_t verifier_advance_count{};
  std::size_t prepared_chunk_count{};
  std::size_t durably_published_chunk_count{};
  std::size_t publication_recertification_count{};
  std::size_t recovery_recertification_count{};
  std::size_t cleanup_recertification_count{};
  std::size_t rejected_recertification_count{};
  std::size_t maximum_retained_record_count{};
  std::size_t maximum_retained_point_id_reference_count{};
  std::size_t retained_transition_history_count{};
  std::size_t context_owned_global_pair_count{};
  std::size_t context_owned_global_facet_or_coface_count{};
  std::size_t context_owned_cell_gamma_or_delaunay_count{};
  std::size_t maximum_live_session_cache_count{};
};

// Derived handoff built from the fresh P8l replay that accepted one durable
// transition.  It is never decoded from the payload and is valid only for the
// synchronous store callback that receives it.
struct ExactSparseAnchoredPairRecertifiedChunkProjection final
    : AtomicLinearRunAcceptedProjection {
  std::size_t source_advance_count{};
  std::size_t successor_advance_count{};
  ExactSparseAnchoredPairRecordSegment record_segment;
  ExactSparseAnchoredPairSessionAudit cumulative_audit{};
  ExactSparseAnchoredPairSessionStepKind last_step_kind{
      ExactSparseAnchoredPairSessionStepKind::budget_exhausted};
  ExactSparseAnchoredPairSessionStopReason last_stop_reason{
      ExactSparseAnchoredPairSessionStopReason::none};
  bool session_complete{false};
  bool total_capacity_exhausted{false};
  bool run_advance_limit_reached{false};

  friend bool operator==(
      const ExactSparseAnchoredPairRecertifiedChunkProjection& left,
      const ExactSparseAnchoredPairRecertifiedChunkProjection& right) {
    return left.source_advance_count == right.source_advance_count &&
        left.successor_advance_count == right.successor_advance_count &&
        left.record_segment == right.record_segment &&
        left.cumulative_audit == right.cumulative_audit &&
        left.last_step_kind == right.last_step_kind &&
        left.last_stop_reason == right.last_stop_reason &&
        left.session_complete == right.session_complete &&
        left.total_capacity_exhausted ==
            right.total_capacity_exhausted &&
        left.run_advance_limit_reached ==
            right.run_advance_limit_reached;
  }
};

using ExactSparseAnchoredPairRecertifiedChunkVisitor =
    std::function<void(
        const ExactSparseAnchoredPairRecertifiedChunkProjection&)>;

// Phase 14V application adapter for AtomicLinearRunStore.  The authority
// context, its cloud and its LBVH must outlive this context and every store
// made from it.  Neither a P8l cursor nor a drained segment is serialized.
class ExactSparseAnchoredPairChunkRunContext {
 public:
  ExactSparseAnchoredPairChunkRunContext(
      const ExactPairSupportAuthorityContext& authority,
      std::size_t maximum_closed_rank,
      ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
      ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
      ExactSparseAnchoredPairSessionAdvanceBudget advance_budget,
      ExactSparseAnchoredPairChunkRunLimits limits);
  ~ExactSparseAnchoredPairChunkRunContext();

  ExactSparseAnchoredPairChunkRunContext(
      const ExactSparseAnchoredPairChunkRunContext&) = delete;
  ExactSparseAnchoredPairChunkRunContext& operator=(
      const ExactSparseAnchoredPairChunkRunContext&) = delete;
  ExactSparseAnchoredPairChunkRunContext(
      ExactSparseAnchoredPairChunkRunContext&&) = delete;
  ExactSparseAnchoredPairChunkRunContext& operator=(
      ExactSparseAnchoredPairChunkRunContext&&) = delete;

  [[nodiscard]] const contract::CanonicalId&
  application_contract_digest() const noexcept;
  [[nodiscard]] const contract::CanonicalId&
  initial_checkpoint_digest() const noexcept;

  [[nodiscard]] AtomicLinearRunContract run_contract(
      contract::CanonicalId initial_output_chain_digest) const;

  [[nodiscard]] AtomicLinearRunStore create_new_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor =
          std::nullopt) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      contract::CanonicalId initial_output_chain_digest,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      ExactSparseAnchoredPairRecertifiedChunkVisitor
          committed_prefix_visitor) const;

  [[nodiscard]] AtomicLinearRunChunkProposal prepare_next_chunk(
      const AtomicLinearRunTrustedState& trusted_state) const;

  [[nodiscard]] AtomicLinearRunPublishResult publish_next_chunk(
      AtomicLinearRunStore& store,
      AtomicLinearRunPublishOptions options = {}) const;

  [[nodiscard]] AtomicLinearRunRecertification
  recertify_for_validation(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) const;

  [[nodiscard]] ExactSparseAnchoredPairChunkRunAudit audit() const noexcept;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
