#pragma once

#include "morsehgp3d/hierarchy/atomic_linear_run_store.hpp"
#include "morsehgp3d/hierarchy/sparse_direct_h0_candidate_merge.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    sparse_higher_support_h0_chunk_run_schema_version = 1U;
inline constexpr std::size_t
    sparse_higher_support_h0_chunk_run_fixed_payload_byte_count = 278U;
inline constexpr std::string_view
    sparse_higher_support_h0_chunk_run_backend = "reference_cpu";
inline constexpr std::string_view
    sparse_higher_support_h0_chunk_run_profile = "hgp_reduced";
inline constexpr std::string_view
    sparse_higher_support_h0_chunk_run_mode =
        "durable_recertified_higher_h0_chunk_run";
inline constexpr std::string_view
    sparse_higher_support_h0_chunk_run_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    sparse_higher_support_h0_chunk_run_public_status = "not_claimed";

// The scientific chunk shape is fixed by ExactHigherSupportStreamBudget and
// the common-H0 projection limits.  These additional limits bound the durable
// representation and the total number of replayable transitions.
struct ExactSparseHigherSupportH0ChunkRunLimits {
  std::size_t maximum_chunk_count{};
  std::size_t maximum_exact_integer_byte_count{};
  std::size_t maximum_total_exact_integer_byte_count_per_chunk{};
  std::size_t maximum_payload_byte_count{};

  friend bool operator==(
      const ExactSparseHigherSupportH0ChunkRunLimits&,
      const ExactSparseHigherSupportH0ChunkRunLimits&) = default;
};

// The observed candidate/diagnostic/reference fields are high-water marks.
// The pending-segment and persistent-cache fields state architectural
// capacities; they are not a process RSS measurement.  A publication may
// temporarily hold producer, verifier, projection and wire buffers at once,
// all bounded by the explicit per-chunk caps.
struct ExactSparseHigherSupportH0ChunkRunAudit {
  std::size_t producer_root_reconstruction_count{};
  std::size_t verifier_root_reconstruction_count{};
  std::size_t producer_chunk_replay_count{};
  std::size_t verifier_chunk_replay_count{};
  std::size_t prepared_chunk_count{};
  std::size_t durably_published_chunk_count{};
  std::size_t publication_recertification_count{};
  std::size_t recovery_recertification_count{};
  std::size_t cleanup_recertification_count{};
  std::size_t rejected_recertification_count{};
  std::size_t maximum_retained_candidate_count{};
  std::size_t maximum_retained_diagnostic_count{};
  std::size_t maximum_retained_point_id_reference_count{};
  std::size_t maximum_pending_producer_segment_count{};
  std::size_t retained_transition_history_count{};
  std::size_t context_owned_global_support_count{};
  std::size_t context_owned_global_facet_or_coface_count{};
  std::size_t context_owned_cell_gamma_or_delaunay_count{};
  std::size_t maximum_persistent_session_cache_count{};

  friend bool operator==(
      const ExactSparseHigherSupportH0ChunkRunAudit&,
      const ExactSparseHigherSupportH0ChunkRunAudit&) = default;
};

// Synchronous handoff derived only from a fresh canonical-root replay.  The
// durable payload is never decoded into scientific authority.  The store and
// context retain neither this projection nor any preceding common H0 run.
struct ExactSparseHigherSupportH0RecertifiedChunkProjection final
    : AtomicLinearRunAcceptedProjection {
  std::size_t source_chunk_count{};
  std::size_t successor_chunk_count{};
  ExactSparseDirectH0CandidateRun run;
  ExactHigherSupportStreamAudit cumulative_audit_after{};
  bool session_complete{false};
  bool run_chunk_limit_reached{false};

  friend bool operator==(
      const ExactSparseHigherSupportH0RecertifiedChunkProjection& left,
      const ExactSparseHigherSupportH0RecertifiedChunkProjection& right) {
    return left.source_chunk_count == right.source_chunk_count &&
        left.successor_chunk_count == right.successor_chunk_count &&
        left.run == right.run &&
        left.cumulative_audit_after == right.cumulative_audit_after &&
        left.session_complete == right.session_complete &&
        left.run_chunk_limit_reached ==
            right.run_chunk_limit_reached;
  }
};

using ExactSparseHigherSupportH0RecertifiedChunkVisitor =
    std::function<void(
        const ExactSparseHigherSupportH0RecertifiedChunkProjection&)>;

// Phase-14AB adapter over AtomicLinearRunStore.  It starts producer and
// verifier sessions from the same immutable, already hashed higher-support
// authority, drains exactly one scientific segment per transition, projects
// it to the common H0 schema, and accepts bytes only after an independent
// fresh replay reproduces them exactly.  The authority, cloud and LBVH must
// outlive this context and every store created from it.
class ExactSparseHigherSupportH0ChunkRunContext {
 public:
  ExactSparseHigherSupportH0ChunkRunContext(
      const ExactHigherSupportAuthorityContext& authority,
      ExactHigherSupportStreamBudget chunk_budget,
      ExactSparseDirectH0CandidateRunLimits projection_limits,
      ExactSparseHigherSupportH0ChunkRunLimits limits);
  ~ExactSparseHigherSupportH0ChunkRunContext();

  ExactSparseHigherSupportH0ChunkRunContext(
      const ExactSparseHigherSupportH0ChunkRunContext&) = delete;
  ExactSparseHigherSupportH0ChunkRunContext& operator=(
      const ExactSparseHigherSupportH0ChunkRunContext&) = delete;
  ExactSparseHigherSupportH0ChunkRunContext(
      ExactSparseHigherSupportH0ChunkRunContext&&) = delete;
  ExactSparseHigherSupportH0ChunkRunContext& operator=(
      ExactSparseHigherSupportH0ChunkRunContext&&) = delete;

  [[nodiscard]] const contract::CanonicalId&
  application_contract_digest() const noexcept;
  [[nodiscard]] const contract::CanonicalId&
  initial_checkpoint_digest() const noexcept;

  [[nodiscard]] AtomicLinearRunContract run_contract() const;

  [[nodiscard]] AtomicLinearRunStore create_new_store(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunStoreLimits store_limits) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor =
          std::nullopt) const;

  [[nodiscard]] AtomicLinearRunStore open_existing_store(
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunStoreLimits store_limits,
      std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
      ExactSparseHigherSupportH0RecertifiedChunkVisitor
          committed_prefix_visitor) const;

  [[nodiscard]] AtomicLinearRunPublishResult publish_next_chunk(
      AtomicLinearRunStore& store,
      AtomicLinearRunPublishOptions options = {}) const;

  // Application-level scientific check for fault injection and validation
  // tooling.  It deliberately does not replace AtomicLinearRunStore's checks
  // of the run-contract digest, generic output chain or wire digest.
  [[nodiscard]] AtomicLinearRunRecertification
  recertify_for_validation(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) const;

  [[nodiscard]] ExactSparseHigherSupportH0ChunkRunAudit audit()
      const noexcept;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
