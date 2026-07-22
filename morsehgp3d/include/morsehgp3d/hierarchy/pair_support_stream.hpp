#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t pair_support_maximum_requested_order = 10U;
inline constexpr std::uint32_t pair_support_checkpoint_schema_version = 1U;
inline constexpr std::uint32_t pair_support_traversal_version = 1U;
inline constexpr std::string_view pair_support_checkpoint_proof_basis =
    "exact_self_dual_unordered_pair_partition_strict_phi_"
    "safe_real_anchor_noninterior_exclusion_resumable_witness_cursor_"
    "sparse_closed_ball_v1";

// Exact maximum of (x-u).(x-v) over query_box x first_support_box x
// second_support_box.  Endpoint selectors are 0 for lower and 1 for upper;
// they form a compact replay witness while maximum_phi remains authoritative.
struct ExactDiametralPhiAabbMaximum {
  exact::ExactRational maximum_phi;
  std::array<std::uint8_t, 3> query_endpoint{};
  std::array<std::uint8_t, 3> first_support_endpoint{};
  std::array<std::uint8_t, 3> second_support_endpoint{};

  friend bool operator==(
      const ExactDiametralPhiAabbMaximum&,
      const ExactDiametralPhiAabbMaximum&) = default;
};

[[nodiscard]] ExactDiametralPhiAabbMaximum
exact_diametral_phi_aabb_maximum(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box);

enum class ExactPairSupportStreamStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class ExactPairSupportStopReason : std::uint8_t {
  none,
  work_unit_limit,
  frontier_entry_limit,
  auxiliary_frontier_entry_limit,
  emitted_record_limit,
  emitted_point_id_reference_limit,
  global_closed_ball_query_limit,
  point_classification_limit,
};

struct ExactPairSupportStreamBudget {
  // A work unit is either one support-product visit or one witness-node visit.
  // A global closed-ball query is an indivisible exact leaf operation and is
  // accounted separately in the audit.
  std::size_t maximum_work_unit_count{};
  std::size_t maximum_frontier_entry_count{1U};
  // Bounds either one witness search or one sparse closed-ball DFS.  These
  // auxiliary frontiers are ephemeral and never become a global cell arena.
  std::size_t maximum_auxiliary_frontier_entry_count{1U};
  std::size_t maximum_emitted_record_count{};
  // Includes fixed support ids and sparse interior/extra-shell witnesses.
  // Exterior ids and complete degenerate shells are never materialized.
  std::size_t maximum_emitted_point_id_reference_count{};
  std::size_t maximum_global_closed_ball_query_count{};
  // Checked conservatively before a leaf query: one complete global shell
  // classifies point_count sites, even when LBVH bulk decisions avoid an
  // exact point-distance evaluation.
  std::size_t maximum_point_classification_count{};

  friend bool operator==(
      const ExactPairSupportStreamBudget&,
      const ExactPairSupportStreamBudget&) = default;
};

struct ExactPairSupportRequirements {
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};

  friend bool operator==(
      const ExactPairSupportRequirements&,
      const ExactPairSupportRequirements&) = default;
};

// Fixed-width, pointer-free frontier record suitable for later GPU packing.
// Leaf ranges are half-open Morton intervals in the supplied LBVH.
struct ExactPairSupportFrontierEntry {
  std::uint64_t first_node_index{};
  std::uint64_t second_node_index{};
  std::uint64_t first_leaf_begin{};
  std::uint64_t first_leaf_end{};
  std::uint64_t second_leaf_begin{};
  std::uint64_t second_leaf_end{};
  std::uint8_t self_product{};

  friend bool operator==(
      const ExactPairSupportFrontierEntry&,
      const ExactPairSupportFrontierEntry&) = default;
};

static_assert(std::is_standard_layout_v<ExactPairSupportFrontierEntry>);
static_assert(std::is_trivially_copyable_v<ExactPairSupportFrontierEntry>);
static_assert(std::is_standard_layout_v<spatial::ExactDyadicAabb3>);
static_assert(std::is_trivially_copyable_v<spatial::ExactDyadicAabb3>);

// One LBVH node plus its immutable Morton interval.  Repeating the interval in
// the checkpoint makes stale or corrupted node indices fail closed before a
// resumed geometric decision is attempted.
struct ExactPairSupportWitnessNodeEntry {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};

  friend bool operator==(
      const ExactPairSupportWitnessNodeEntry&,
      const ExactPairSupportWitnessNodeEntry&) = default;
};

static_assert(std::is_standard_layout_v<ExactPairSupportWitnessNodeEntry>);
static_assert(std::is_trivially_copyable_v<ExactPairSupportWitnessNodeEntry>);

enum class ExactPairSupportPendingStage : std::uint8_t {
  rank_search,
  expand_product,
  classify_leaf,
};

// Cursor for the only support product whose product visit has been charged but
// whose transition has not yet finished.  Strict-witness receipts form a
// disjoint antichain; deferred_expansion_node records a node already decided
// once whose two children could not fit the current auxiliary-frontier cap.
struct ExactPairSupportPendingProduct {
  ExactPairSupportFrontierEntry product{};
  ExactPairSupportPendingStage stage{
      ExactPairSupportPendingStage::rank_search};
  bool rank_search_started{false};
  std::vector<ExactPairSupportWitnessNodeEntry> witness_frontier;
  std::vector<ExactPairSupportWitnessNodeEntry> strict_witness_receipts;
  std::optional<ExactPairSupportWitnessNodeEntry> deferred_expansion_node;
  std::size_t strict_witness_point_count{};

  friend bool operator==(
      const ExactPairSupportPendingProduct&,
      const ExactPairSupportPendingProduct&) = default;
};

struct ExactPairSupportCheckpointManifest {
  std::uint32_t schema_version{pair_support_checkpoint_schema_version};
  std::uint32_t traversal_version{pair_support_traversal_version};
  std::size_t point_count{};
  std::size_t lbvh_node_count{};
  std::size_t lbvh_leaf_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId lbvh_digest{};
  contract::CanonicalId semantic_digest{};

  friend bool operator==(
      const ExactPairSupportCheckpointManifest&,
      const ExactPairSupportCheckpointManifest&) = default;
};

// 9.3b-RCPU authority cache.  The referenced cloud and LBVH must outlive the
// context and must not be moved while it is in use.  Construction authenticates
// and hashes both authorities exactly once; chunk production and verification
// then reuse the cached manifest instead of rescanning O(n) data.
struct ExactPairSupportAuthorityContextAudit {
  std::size_t manifest_build_count{};
  std::size_t canonical_cloud_point_hash_count{};
  std::size_t lbvh_leaf_hash_count{};
  std::size_t lbvh_node_hash_count{};
  bool manifest_cached{false};

  friend bool operator==(
      const ExactPairSupportAuthorityContextAudit&,
      const ExactPairSupportAuthorityContextAudit&) = default;
};

class ExactPairSupportAuthorityContext {
 public:
  ExactPairSupportAuthorityContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order);
  ExactPairSupportAuthorityContext(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t) = delete;
  ExactPairSupportAuthorityContext(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t) = delete;
  ExactPairSupportAuthorityContext(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      std::size_t) = delete;
  ExactPairSupportAuthorityContext(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t) = delete;

  ExactPairSupportAuthorityContext& operator=(
      const ExactPairSupportAuthorityContext&) = delete;
  ExactPairSupportAuthorityContext(
      ExactPairSupportAuthorityContext&&) = delete;
  ExactPairSupportAuthorityContext& operator=(
      ExactPairSupportAuthorityContext&&) = delete;

  [[nodiscard]] const spatial::MortonLbvhIndex& index() const noexcept {
    return *index_;
  }
  [[nodiscard]] const spatial::CanonicalPointCloud& cloud() const noexcept {
    return *cloud_;
  }
  [[nodiscard]] std::size_t requested_maximum_order() const noexcept {
    return requested_maximum_order_;
  }
  [[nodiscard]] const ExactPairSupportCheckpointManifest& manifest()
      const noexcept {
    return manifest_;
  }
  [[nodiscard]] const ExactPairSupportAuthorityContextAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  ExactPairSupportAuthorityContext(
      const ExactPairSupportAuthorityContext&) = default;

  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  std::size_t requested_maximum_order_{};
  ExactPairSupportCheckpointManifest manifest_{};
  ExactPairSupportAuthorityContextAudit audit_{};

  friend class ExactPairSupportIncrementalVerifier;
};

struct ExactPairSupportEvent {
  std::array<spatial::PointId, 2> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactPairSupportEvent&,
      const ExactPairSupportEvent&) = default;
};

// A pair with an additional exact shell point is not emitted as a regular
// event.  It remains relevant when interior_count + support_size is inside the
// requested rank window, independently of the full shell cardinality.
struct ExactPairSupportExtraShellDiagnostic {
  std::array<spatial::PointId, 2> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  // A complete exact traversal certifies shell_count.  Only the canonical
  // least extra-shell id is retained, so a 10M-point cosphere cannot force a
  // 10M-entry resident diagnostic.
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactPairSupportExtraShellDiagnostic&,
      const ExactPairSupportExtraShellDiagnostic&) = default;
};

struct ExactPairSupportStreamAudit {
  std::size_t total_pair_count{};
  std::size_t work_unit_count{};
  std::size_t support_product_visit_count{};
  std::size_t support_product_expansion_count{};
  std::size_t self_product_expansion_count{};
  std::size_t cross_product_expansion_count{};
  std::size_t diagonal_leaf_discard_count{};
  std::size_t diagonal_product_rank_search_skip_count{};
  std::size_t rank_prune_search_count{};
  std::size_t witness_node_visit_count{};
  std::size_t exact_phi_aabb_bound_count{};
  std::size_t exact_anchor_ball_minimum_aabb_bound_count{};
  std::size_t certified_anchor_noninterior_subtree_count{};
  std::size_t certified_anchor_noninterior_point_count{};
  std::size_t certified_anchor_shell_tangent_subtree_count{};
  std::size_t equality_or_positive_bound_descent_count{};
  std::size_t strict_interior_witness_subtree_count{};
  std::size_t strict_interior_witness_point_count{};
  std::size_t rank_pruned_product_count{};
  std::size_t rank_pruned_pair_count{};
  std::size_t leaf_pair_classification_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t point_classification_count{};
  std::size_t closed_ball_node_visit_count{};
  std::size_t exact_closed_ball_minimum_aabb_bound_count{};
  std::size_t exact_closed_ball_maximum_aabb_bound_count{};
  std::size_t closed_ball_bulk_interior_subtree_count{};
  std::size_t closed_ball_bulk_interior_point_count{};
  std::size_t closed_ball_bulk_exterior_subtree_count{};
  std::size_t closed_ball_bulk_exterior_point_count{};
  std::size_t early_closed_rank_rejection_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t above_rank_pair_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_witness_frontier_entry_count{};
  std::size_t maximum_closed_ball_frontier_entry_count{};
  std::size_t remaining_frontier_pair_count{};
  std::size_t resolved_pair_count{};
  bool pair_partition_accounting_certified{false};

  friend bool operator==(
      const ExactPairSupportStreamAudit&,
      const ExactPairSupportStreamAudit&) = default;
};

struct ExactPairSupportCheckpoint {
  ExactPairSupportCheckpointManifest manifest{};
  std::uint64_t next_chunk_sequence{};
  // Logical prefix represented by this in-memory state.  These fields do not
  // imply that an external sink has durably published the records.
  std::size_t output_record_count{};
  contract::CanonicalId output_chain_digest{};
  std::vector<ExactPairSupportFrontierEntry> frontier;
  std::optional<ExactPairSupportPendingProduct> pending_product;
  ExactPairSupportStreamAudit cumulative_audit{};
  contract::CanonicalId checkpoint_digest{};

  // Local terminal predicate only.  Scientific lineage is established by
  // verify_exact_pair_support_stream_run(), not by this predicate or digest.
  [[nodiscard]] bool complete() const noexcept {
    return frontier.empty() && !pending_product.has_value() &&
           cumulative_audit.pair_partition_accounting_certified &&
           cumulative_audit.remaining_frontier_pair_count == 0U &&
           cumulative_audit.resolved_pair_count ==
               cumulative_audit.total_pair_count;
  }

  friend bool operator==(
      const ExactPairSupportCheckpoint&,
      const ExactPairSupportCheckpoint&) = default;
};

struct ExactPairSupportStreamResult {
  ExactPairSupportRequirements requirements{};
  ExactPairSupportStreamBudget budget{};
  ExactPairSupportStreamStatus status{
      ExactPairSupportStreamStatus::budget_exhausted};
  ExactPairSupportStopReason stop_reason{
      ExactPairSupportStopReason::work_unit_limit};
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  std::vector<ExactPairSupportFrontierEntry> remaining_frontier;
  ExactPairSupportStreamAudit audit{};
  bool self_product_partition_certified{false};
  bool witness_antichains_certified{false};
  bool all_rank_prunes_recertified{false};
  // Rank-rejected queries may stop as soon as the strict-interior cap is
  // exceeded.  Every query that can still emit a rank-relevant record visits
  // its complete shell.
  bool all_rank_relevant_shells_complete{false};
  bool frontier_exhausted{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  [[nodiscard]] bool stream_complete() const noexcept {
    return status == ExactPairSupportStreamStatus::complete &&
           stop_reason == ExactPairSupportStopReason::none &&
           frontier_exhausted && remaining_frontier.empty() &&
           self_product_partition_certified &&
           witness_antichains_certified &&
           all_rank_prunes_recertified &&
           all_rank_relevant_shells_complete &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed &&
           audit.pair_partition_accounting_certified &&
           audit.remaining_frontier_pair_count == 0U &&
           audit.resolved_pair_count == audit.total_pair_count;
  }

  [[nodiscard]] bool absence_of_additional_pair_supports_certified()
      const noexcept {
    return stream_complete();
  }

  friend bool operator==(
      const ExactPairSupportStreamResult&,
      const ExactPairSupportStreamResult&) = default;
};

[[nodiscard]] ExactPairSupportStreamResult build_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget);

// 9.3a-RCPU transition-preparation API.  A chunk contains only records emitted
// since the supplied trusted checkpoint.  The returned records and
// next_checkpoint form one mutable logical candidate for a future external
// atomic commit; no durable sink operation is performed here.  Discarding or
// mutating the value leaves the input checkpoint valid, so a sink can retry
// deterministically and fresh replay rejects altered candidates.
struct ExactPairSupportStreamChunk {
  ExactPairSupportCheckpointManifest manifest{};
  ExactPairSupportStreamBudget budget{};
  std::uint64_t chunk_sequence{};
  std::size_t first_output_record_index{};
  contract::CanonicalId source_checkpoint_digest{};
  contract::CanonicalId previous_output_chain_digest{};
  contract::CanonicalId output_chain_digest{};
  ExactPairSupportStreamStatus status{
      ExactPairSupportStreamStatus::budget_exhausted};
  ExactPairSupportStopReason stop_reason{
      ExactPairSupportStopReason::work_unit_limit};
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  enum class RecordKind : std::uint8_t {
    event,
    relevant_extra_shell_diagnostic,
  };
  // Traversal order across the two typed record vectors.  Consumers advance
  // the matching vector for each kind, so the output hash chain is replayable.
  std::vector<RecordKind> record_order;
  ExactPairSupportStreamAudit cumulative_audit_before{};
  ExactPairSupportStreamAudit cumulative_audit_after{};
  ExactPairSupportCheckpoint next_checkpoint{};
  bool candidate_prepared{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  // Completion relative to the supplied source checkpoint.  This is not a
  // durable publication or an independently anchored lineage claim.
  [[nodiscard]] bool relative_stream_complete() const noexcept {
    return candidate_prepared &&
           status == ExactPairSupportStreamStatus::complete &&
           stop_reason == ExactPairSupportStopReason::none &&
           next_checkpoint.complete() &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed;
  }

  friend bool operator==(
      const ExactPairSupportStreamChunk&,
      const ExactPairSupportStreamChunk&) = default;
};

[[nodiscard]] ExactPairSupportCheckpointManifest
make_exact_pair_support_checkpoint_manifest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order);

[[nodiscard]] ExactPairSupportCheckpoint
make_initial_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order);

[[nodiscard]] ExactPairSupportCheckpoint
make_initial_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority);

// Deterministic, unkeyed payload checksum.  This supports codecs and hostile
// mutation tests; it provides integrity only, never source provenance.
[[nodiscard]] contract::CanonicalId
compute_exact_pair_support_checkpoint_digest(
    const ExactPairSupportCheckpoint& checkpoint);

struct ExactPairSupportCheckpointVerification {
  struct ValidationAudit {
    std::size_t frontier_entry_validation_count{};
    std::size_t frontier_rectangle_count{};
    std::size_t frontier_sweep_event_count{};
    std::size_t frontier_active_set_operation_count{};
    std::size_t frontier_neighbor_test_count{};
    std::size_t active_witness_entry_validation_count{};
    std::size_t strict_witness_receipt_validation_count{};
    std::size_t witness_interval_count{};
    std::size_t witness_adjacent_interval_test_count{};
    std::size_t strict_receipt_geometry_recertification_count{};

    friend bool operator==(
        const ValidationAudit&,
        const ValidationAudit&) = default;
  };
  bool manifest_matches_authorities{false};
  bool checksum_matches_payload{false};
  bool frontier_locally_valid{false};
  bool pending_product_locally_valid{false};
  // Necessary identities derivable from the current cursor, not a replay of
  // every historical telemetry counter or a proof of source provenance.
  bool required_audit_identities_hold{false};
  // Integrity only: this does not prove descent from the initial checkpoint.
  bool integrity_verified{false};
  // Appended after the original v1 aggregate fields so existing source-level
  // aggregate initializers keep their positional meaning.
  ValidationAudit validation_audit{};
  // Frontier products are checked by a rectangle sweep and witness ranges by
  // one interval sort.  No pairwise frontier/receipt comparison is performed.
  bool quasi_linear_structure_validation_certified{false};
};

[[nodiscard]] ExactPairSupportCheckpointVerification
verify_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportCheckpoint& checkpoint);

[[nodiscard]] ExactPairSupportCheckpointVerification
verify_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportCheckpoint& checkpoint);

[[nodiscard]] ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint);

[[nodiscard]] ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint);

struct ExactPairSupportStreamChunkVerification {
  bool source_checkpoint_integrity_verified{false};
  bool requested_budget_certified{false};
  bool prepared_transition_chain_matches{false};
  bool records_individually_exact{false};
  bool next_checkpoint_integrity_verified{false};
  bool fresh_replay_certified{false};
  // Exact relative transition from the supplied source checkpoint.  Source
  // provenance is established only by the anchored run verifier below.
  bool chunk_transition_verified{false};
};

[[nodiscard]] ExactPairSupportStreamChunkVerification
verify_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed);

[[nodiscard]] ExactPairSupportStreamChunkVerification
verify_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed);

struct ExactPairSupportIncrementalVerifierStatus {
  bool initial_checkpoint_reconstructed{false};
  std::size_t verified_chunk_count{};
  bool every_transition_verified{false};
  bool terminal_checkpoint_reached{false};
  bool anchored_prefix_certified{false};
  bool anchored_run_certified{false};
  bool failed_closed{false};
  // The verifier retains only trusted_checkpoint(), never prior chunks.
  std::size_t retained_chunk_count{};

  friend bool operator==(
      const ExactPairSupportIncrementalVerifierStatus&,
      const ExactPairSupportIncrementalVerifierStatus&) = default;
};

// Stateful anchored verifier for streaming consumers.  verify_next() checks
// exactly one candidate against the current trusted checkpoint and advances
// only after success.  Any rejected or post-terminal candidate poisons the
// verifier, so a caller cannot skip a failed transition.
class ExactPairSupportIncrementalVerifier {
 public:
  class PreparedNext {
   public:
    PreparedNext(const PreparedNext&) = delete;
    PreparedNext& operator=(const PreparedNext&) = delete;
    PreparedNext(PreparedNext&& other) noexcept;
    PreparedNext& operator=(PreparedNext&& other) noexcept;
    ~PreparedNext() = default;

    [[nodiscard]] const ExactPairSupportStreamChunkVerification& verification()
        const noexcept {
      return verification_;
    }
    [[nodiscard]] bool prepared() const noexcept {
      return valid_ && verification_.chunk_transition_verified;
    }
    // This is the checkpoint freshly reconstructed by exact replay, not the
    // mutable checkpoint carried by the observed chunk.  Durable metadata
    // must derive its scientific digest from this value before commit.
    [[nodiscard]] const ExactPairSupportCheckpoint& trusted_next_checkpoint()
        const noexcept {
      return trusted_next_;
    }

   private:
    PreparedNext(
        ExactPairSupportIncrementalVerifier* owner,
        std::uint64_t source_epoch,
        std::size_t next_verified_chunk_count,
        ExactPairSupportStreamChunkVerification verification,
        ExactPairSupportCheckpoint trusted_next) noexcept;
    explicit PreparedNext(
        ExactPairSupportStreamChunkVerification verification) noexcept
        : verification_(verification) {}
    PreparedNext() noexcept = default;

    ExactPairSupportIncrementalVerifier* owner_{};
    std::uint64_t source_epoch_{};
    std::size_t next_verified_chunk_count_{};
    ExactPairSupportStreamChunkVerification verification_{};
    ExactPairSupportCheckpoint trusted_next_{};
    bool valid_{false};

    friend class ExactPairSupportIncrementalVerifier;
  };

  explicit ExactPairSupportIncrementalVerifier(
      const ExactPairSupportAuthorityContext& authority);
  explicit ExactPairSupportIncrementalVerifier(
      ExactPairSupportAuthorityContext&&) = delete;
  explicit ExactPairSupportIncrementalVerifier(
      const ExactPairSupportAuthorityContext&&) = delete;

  ExactPairSupportIncrementalVerifier(
      const ExactPairSupportIncrementalVerifier&) = delete;
  ExactPairSupportIncrementalVerifier& operator=(
      const ExactPairSupportIncrementalVerifier&) = delete;
  ExactPairSupportIncrementalVerifier(
      ExactPairSupportIncrementalVerifier&&) = delete;
  ExactPairSupportIncrementalVerifier& operator=(
      ExactPairSupportIncrementalVerifier&&) = delete;

  // Replays and certifies one relative transition without advancing the
  // trusted checkpoint.  A durable sink commits the returned token only after
  // its rename and directory synchronization have succeeded.  Discarding a
  // valid token is side-effect free and permits a deterministic retry.
  [[nodiscard]] PreparedNext prepare_next(
      const ExactPairSupportStreamBudget& chunk_budget,
      const ExactPairSupportStreamChunk& observed);

  // Noexcept memory commit for a token prepared against the current epoch.
  // A stale, foreign, invalid or reused token fails closed.
  [[nodiscard]] bool commit_prepared(PreparedNext&& prepared) noexcept;

  // Convenience path for non-durable consumers.
  [[nodiscard]] ExactPairSupportStreamChunkVerification verify_next(
      const ExactPairSupportStreamBudget& chunk_budget,
      const ExactPairSupportStreamChunk& observed);

  [[nodiscard]] const ExactPairSupportCheckpoint& trusted_checkpoint()
      const noexcept {
    return trusted_checkpoint_;
  }
  [[nodiscard]] const ExactPairSupportIncrementalVerifierStatus& status()
      const noexcept {
    return status_;
  }

 private:
  void poison() noexcept;

  // Private cache copy: callers need only keep the immutable cloud and LBVH
  // alive, not the context object used to construct this verifier.
  ExactPairSupportAuthorityContext authority_;
  ExactPairSupportCheckpoint trusted_checkpoint_{};
  ExactPairSupportIncrementalVerifierStatus status_{};
  std::uint64_t epoch_{};
};

struct ExactPairSupportStreamRunVerification {
  bool initial_checkpoint_reconstructed{false};
  std::size_t verified_chunk_count{};
  bool every_transition_verified{false};
  bool terminal_checkpoint_reached{false};
  bool anchored_run_certified{false};
};

// Reconstructs the initial checkpoint from the external cloud/LBVH/K
// authorities, then verifies every observed transition in order.  This is the
// scientific lineage check; local checkpoint integrity alone is insufficient.
[[nodiscard]] ExactPairSupportStreamRunVerification
verify_exact_pair_support_stream_run(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    std::span<const ExactPairSupportStreamBudget> chunk_budgets,
    std::span<const ExactPairSupportStreamChunk> chunks);

struct ExactPairSupportStreamVerification {
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool partial_records_individually_exact{false};
  bool completion_claim_certified{false};
  bool absence_claim_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

[[nodiscard]] ExactPairSupportStreamVerification verify_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& observed);

}  // namespace morsehgp3d::hierarchy
