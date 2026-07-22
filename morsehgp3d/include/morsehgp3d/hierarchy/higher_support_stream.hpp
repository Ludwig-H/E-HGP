#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t higher_support_maximum_requested_order = 10U;
inline constexpr std::uint32_t higher_support_checkpoint_schema_version = 2U;
inline constexpr std::uint32_t higher_support_traversal_version = 1U;
inline constexpr std::string_view higher_support_stream_proof_basis =
    "exact_grouped_multiplicity_lbvh_support_partition_"
    "universal_gram_cramer_rank_receipts_sparse_closed_ball_anchored_v2";

enum class ExactHigherSupportStreamStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class ExactHigherSupportStopReason : std::uint8_t {
  none,
  work_unit_limit,
  frontier_entry_limit,
  auxiliary_frontier_entry_limit,
  emitted_record_limit,
  emitted_point_id_reference_limit,
  prune_receipt_limit,
  global_closed_ball_query_limit,
  point_classification_limit,
};

// Product visits and rank-witness node visits consume work units.  A leaf
// closed-ball query is an indivisible exact operation and has separate caps.
// No capacity is proportional to C(n,3)+C(n,4).
struct ExactHigherSupportStreamBudget {
  std::size_t maximum_work_unit_count{};
  // This capacity must hold the complete initial root set: zero entries for
  // n<3, one for n=3, and two for n>=4.  A smaller value is an invalid
  // configuration because no residual frontier could represent the universe.
  std::size_t maximum_frontier_entry_count{2U};
  std::size_t maximum_auxiliary_frontier_entry_count{1U};
  // Includes regular events, extra-shell diagnostics and prune certificates.
  std::size_t maximum_emitted_record_count{};
  // Includes support ids, retained strict-interior ids and an optional
  // canonical extra-shell witness.  Prune receipts have their own cap.
  std::size_t maximum_emitted_point_id_reference_count{};
  std::size_t maximum_prune_receipt_count{};
  std::size_t maximum_global_closed_ball_query_count{};
  // Conservatively reserves point_count classifications before each atomic
  // sparse closed-ball query, although exact LBVH bounds may classify a whole
  // subtree at once and an above-rank query may terminate early.
  std::size_t maximum_point_classification_count{};

  friend bool operator==(
      const ExactHigherSupportStreamBudget&,
      const ExactHigherSupportStreamBudget&) = default;
};

struct ExactHigherSupportRequirements {
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};

  friend bool operator==(
      const ExactHigherSupportRequirements&,
      const ExactHigherSupportRequirements&) = default;
};

// A group means: choose exactly multiplicity leaves from the immutable Morton
// interval of node_index.  Distinct groups in one product have disjoint
// intervals.  The semantic coverage of an entry is therefore the product of
// binomial(range_size, multiplicity), evaluated with exact::BigInt.
struct ExactHigherSupportNodeGroup {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::uint8_t multiplicity{};

  friend bool operator==(
      const ExactHigherSupportNodeGroup&,
      const ExactHigherSupportNodeGroup&) = default;
};

struct ExactHigherSupportFrontierEntry {
  std::uint8_t support_size{};
  std::uint8_t group_count{};
  std::array<ExactHigherSupportNodeGroup, 4> groups{};

  friend bool operator==(
      const ExactHigherSupportFrontierEntry&,
      const ExactHigherSupportFrontierEntry&) = default;
};

// Repeated Morton intervals authenticate node indices without exposing the
// private LBVH node representation.
struct ExactHigherSupportNodeReceipt {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};

  friend bool operator==(
      const ExactHigherSupportNodeReceipt&,
      const ExactHigherSupportNodeReceipt&) = default;
};

static_assert(std::is_standard_layout_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_standard_layout_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_standard_layout_v<ExactHigherSupportNodeReceipt>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeReceipt>);

enum class ExactHigherSupportPruneReason : std::uint8_t {
  no_well_centered_support,
  strict_interior_rank_bound,
};

struct ExactHigherSupportRankReceipt {
  ExactHigherSupportNodeReceipt query_node{};
  std::size_t certified_point_count{};
  // Replaying this bound must recover a strictly negative upper endpoint.
  ExactHigherSupportProductAabbAnalysis query_product_analysis{};

  friend bool operator==(
      const ExactHigherSupportRankReceipt&,
      const ExactHigherSupportRankReceipt&) = default;
};

// Every pruned product remains externally replayable.  A well-centring prune
// is justified by support_product_analysis alone.  A rank prune additionally
// carries a disjoint LBVH receipt antichain whose exact point-count sum reaches
// required_strict_interior_point_count.
struct ExactHigherSupportPruneCertificate {
  ExactHigherSupportFrontierEntry product{};
  ExactHigherSupportPruneReason reason{
      ExactHigherSupportPruneReason::no_well_centered_support};
  exact::BigInt pruned_support_count{0};
  ExactHigherSupportProductAabbAnalysis support_product_analysis{};
  std::size_t required_strict_interior_point_count{};
  exact::BigInt certified_strict_interior_point_count{0};
  std::vector<ExactHigherSupportRankReceipt> rank_receipts;

  friend bool operator==(
      const ExactHigherSupportPruneCertificate&,
      const ExactHigherSupportPruneCertificate&) = default;
};

enum class ExactHigherSupportPendingStage : std::uint8_t {
  analyze_product,
  rank_search,
  emit_well_prune,
  emit_rank_prune,
  expand_product,
  classify_leaf,
};

// The active product remains the back of the main frontier.  Its product
// visit has already been charged.  During rank_search, rank_frontier and
// strict_interior_receipts form one disjoint antichain of immutable Morton
// ranges.  Exact universal power certificates are recomputed, never persisted
// in the checkpoint cursor.
struct ExactHigherSupportPendingProduct {
  ExactHigherSupportFrontierEntry product{};
  ExactHigherSupportPendingStage stage{
      ExactHigherSupportPendingStage::analyze_product};
  bool rank_search_started{false};
  bool leaf_analysis_started{false};
  std::vector<ExactHigherSupportNodeReceipt> rank_frontier;
  // Only immutable node receipts persist.  Their exact power analyses are
  // deterministically recomputed from the authority when the prune is emitted
  // or the checkpoint is verified.
  std::vector<ExactHigherSupportNodeReceipt> strict_interior_receipts;
  exact::BigInt certified_strict_interior_point_count{0};

  friend bool operator==(
      const ExactHigherSupportPendingProduct&,
      const ExactHigherSupportPendingProduct&) = default;
};

struct ExactHigherSupportCheckpointManifest {
  std::uint32_t schema_version{higher_support_checkpoint_schema_version};
  std::uint32_t traversal_version{higher_support_traversal_version};
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
      const ExactHigherSupportCheckpointManifest&,
      const ExactHigherSupportCheckpointManifest&) = default;
};

struct ExactHigherSupportAuthorityContextAudit {
  std::size_t manifest_build_count{};
  std::size_t canonical_cloud_point_hash_count{};
  std::size_t lbvh_leaf_hash_count{};
  std::size_t lbvh_node_hash_count{};
  bool manifest_cached{false};

  friend bool operator==(
      const ExactHigherSupportAuthorityContextAudit&,
      const ExactHigherSupportAuthorityContextAudit&) = default;
};

// The cloud and LBVH must outlive this immutable authority cache.  It keeps
// O(n) manifest hashing out of the per-chunk hot path.
class ExactHigherSupportAuthorityContext {
 public:
  ExactHigherSupportAuthorityContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order);
  ExactHigherSupportAuthorityContext(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t) = delete;
  ExactHigherSupportAuthorityContext(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t) = delete;
  ExactHigherSupportAuthorityContext(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      std::size_t) = delete;
  ExactHigherSupportAuthorityContext(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t) = delete;

  ExactHigherSupportAuthorityContext(
      const ExactHigherSupportAuthorityContext&) = default;
  ExactHigherSupportAuthorityContext& operator=(
      const ExactHigherSupportAuthorityContext&) = delete;
  ExactHigherSupportAuthorityContext(
      ExactHigherSupportAuthorityContext&&) = delete;
  ExactHigherSupportAuthorityContext& operator=(
      ExactHigherSupportAuthorityContext&&) = delete;

  [[nodiscard]] const spatial::MortonLbvhIndex& index() const noexcept {
    return *index_;
  }
  [[nodiscard]] const spatial::CanonicalPointCloud& cloud() const noexcept {
    return *cloud_;
  }
  [[nodiscard]] std::size_t requested_maximum_order() const noexcept {
    return requested_maximum_order_;
  }
  [[nodiscard]] const ExactHigherSupportCheckpointManifest& manifest()
      const noexcept {
    return manifest_;
  }
  [[nodiscard]] const ExactHigherSupportAuthorityContextAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  std::size_t requested_maximum_order_{};
  ExactHigherSupportCheckpointManifest manifest_{};
  ExactHigherSupportAuthorityContextAudit audit_{};
};

struct ExactHigherSupportEvent {
  std::uint8_t support_size{};
  // Only the first support_size entries are meaningful and are sorted by
  // canonical PointId.  Remaining entries are zero padding.
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactHigherSupportEvent&,
      const ExactHigherSupportEvent&) = default;
};

struct ExactHigherSupportExtraShellDiagnostic {
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactHigherSupportExtraShellDiagnostic&,
      const ExactHigherSupportExtraShellDiagnostic&) = default;
};

struct ExactHigherSupportStreamAudit {
  exact::BigInt total_support_count{0};
  exact::BigInt well_centering_pruned_support_count{0};
  exact::BigInt rank_pruned_support_count{0};
  exact::BigInt leaf_classified_support_count{0};
  exact::BigInt resolved_support_count{0};
  exact::BigInt remaining_frontier_support_count{0};
  std::size_t work_unit_count{};
  std::size_t support_product_visit_count{};
  std::size_t support_product_expansion_count{};
  std::size_t generated_child_product_count{};
  std::size_t exact_product_analysis_count{};
  std::size_t rank_search_count{};
  std::size_t rank_witness_node_visit_count{};
  std::size_t emitted_prune_certificate_count{};
  std::size_t emitted_rank_receipt_count{};
  std::size_t leaf_support_analysis_count{};
  std::size_t affinely_dependent_leaf_count{};
  std::size_t boundary_reduced_leaf_count{};
  std::size_t exterior_circumcenter_leaf_count{};
  std::size_t minimal_leaf_count{};
  std::size_t above_rank_leaf_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t point_classification_count{};
  std::size_t closed_ball_node_visit_count{};
  std::size_t closed_ball_bulk_interior_subtree_count{};
  std::size_t closed_ball_bulk_exterior_subtree_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_record_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_rank_frontier_entry_count{};
  std::size_t maximum_closed_ball_frontier_entry_count{};
  bool exact_bigint_universe_certified{false};
  bool grouped_partition_accounting_certified{false};

  friend bool operator==(
      const ExactHigherSupportStreamAudit&,
      const ExactHigherSupportStreamAudit&) = default;
};

struct ExactHigherSupportStreamResult {
  ExactHigherSupportRequirements requirements{};
  ExactHigherSupportStreamBudget budget{};
  ExactHigherSupportStreamStatus status{
      ExactHigherSupportStreamStatus::budget_exhausted};
  ExactHigherSupportStopReason stop_reason{
      ExactHigherSupportStopReason::work_unit_limit};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  std::vector<ExactHigherSupportPruneCertificate> prune_certificates;
  std::vector<ExactHigherSupportFrontierEntry> remaining_frontier;
  ExactHigherSupportStreamAudit audit{};
  bool grouped_frontier_partition_certified{false};
  bool all_prunes_replayable{false};
  bool all_rank_relevant_shells_complete{false};
  bool frontier_exhausted{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  [[nodiscard]] bool stream_complete() const {
    return status == ExactHigherSupportStreamStatus::complete &&
           stop_reason == ExactHigherSupportStopReason::none &&
           frontier_exhausted && remaining_frontier.empty() &&
           grouped_frontier_partition_certified && all_prunes_replayable &&
           all_rank_relevant_shells_complete &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed &&
           audit.exact_bigint_universe_certified &&
           audit.grouped_partition_accounting_certified &&
           audit.remaining_frontier_support_count == 0 &&
           audit.resolved_support_count == audit.total_support_count;
  }

  [[nodiscard]] bool absence_of_additional_higher_supports_certified() const {
    return stream_complete();
  }

  friend bool operator==(
      const ExactHigherSupportStreamResult&,
      const ExactHigherSupportStreamResult&) = default;
};

struct ExactHigherSupportCheckpoint {
  ExactHigherSupportCheckpointManifest manifest{};
  std::uint64_t next_chunk_sequence{};
  std::size_t output_record_count{};
  contract::CanonicalId output_chain_digest{};
  std::vector<ExactHigherSupportFrontierEntry> frontier;
  std::optional<ExactHigherSupportPendingProduct> pending_product;
  ExactHigherSupportStreamAudit cumulative_audit{};
  contract::CanonicalId checkpoint_digest{};

  // This is only a local shape/audit predicate.  Scientific completeness
  // requires either an ExactHigherSupportAnchoredSession commit chain or
  // verify_exact_higher_support_stream_run from the canonical roots.
  [[nodiscard]] bool locally_complete() const {
    return frontier.empty() && !pending_product.has_value() &&
           cumulative_audit.grouped_partition_accounting_certified &&
           cumulative_audit.remaining_frontier_support_count == 0 &&
           cumulative_audit.resolved_support_count ==
               cumulative_audit.total_support_count;
  }

  friend bool operator==(
      const ExactHigherSupportCheckpoint&,
      const ExactHigherSupportCheckpoint&) = default;
};

struct ExactHigherSupportStreamChunk {
  ExactHigherSupportCheckpointManifest manifest{};
  ExactHigherSupportStreamBudget budget{};
  std::uint64_t chunk_sequence{};
  std::size_t first_output_record_index{};
  contract::CanonicalId source_checkpoint_digest{};
  contract::CanonicalId previous_output_chain_digest{};
  contract::CanonicalId output_chain_digest{};
  ExactHigherSupportStreamStatus status{
      ExactHigherSupportStreamStatus::budget_exhausted};
  ExactHigherSupportStopReason stop_reason{
      ExactHigherSupportStopReason::work_unit_limit};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  std::vector<ExactHigherSupportPruneCertificate> prune_certificates;
  enum class RecordKind : std::uint8_t {
    event,
    relevant_extra_shell_diagnostic,
    prune_certificate,
  };
  std::vector<RecordKind> record_order;
  ExactHigherSupportStreamAudit cumulative_audit_before{};
  ExactHigherSupportStreamAudit cumulative_audit_after{};
  ExactHigherSupportCheckpoint next_checkpoint{};
  bool candidate_prepared{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  [[nodiscard]] bool relative_stream_complete() const {
    return candidate_prepared &&
           status == ExactHigherSupportStreamStatus::complete &&
           stop_reason == ExactHigherSupportStopReason::none &&
           next_checkpoint.locally_complete() &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed;
  }

  friend bool operator==(
      const ExactHigherSupportStreamChunk&,
      const ExactHigherSupportStreamChunk&) = default;
};

struct ExactHigherSupportCheckpointVerification {
  bool manifest_matches_authorities{false};
  bool checksum_matches_payload{false};
  bool frontier_locally_valid{false};
  bool pending_product_locally_valid{false};
  bool pending_receipts_recertified{false};
  bool required_audit_identities_hold{false};
  bool local_integrity_verified{false};
};

struct ExactHigherSupportStreamChunkVerification {
  bool source_checkpoint_local_integrity_verified{false};
  bool source_checkpoint_anchored{false};
  bool requested_budget_certified{false};
  bool prepared_transition_chain_matches{false};
  bool records_individually_exact{false};
  bool next_checkpoint_local_integrity_verified{false};
  bool next_checkpoint_anchored{false};
  bool fresh_replay_certified{false};
  bool chunk_transition_verified{false};
};

// In-memory provenance authority.  A raw checkpoint can prove local integrity
// but cannot prove that its frontier is reachable from the canonical roots.
// This session owns that root anchor, prepares retryable transitions without
// advancing it, and advances only after an exact replay of the supplied
// candidate.  Durable recovery needs a separately authenticated protocol.
class ExactHigherSupportAnchoredSession {
 public:
  explicit ExactHigherSupportAnchoredSession(
      const ExactHigherSupportAuthorityContext& authority);

  ExactHigherSupportAnchoredSession(
      const ExactHigherSupportAnchoredSession&) = delete;
  ExactHigherSupportAnchoredSession& operator=(
      const ExactHigherSupportAnchoredSession&) = delete;
  ExactHigherSupportAnchoredSession(
      ExactHigherSupportAnchoredSession&&) = delete;
  ExactHigherSupportAnchoredSession& operator=(
      ExactHigherSupportAnchoredSession&&) = delete;

  [[nodiscard]] const ExactHigherSupportCheckpoint& trusted_checkpoint()
      const noexcept {
    return trusted_checkpoint_;
  }

  [[nodiscard]] ExactHigherSupportStreamChunk prepare_next(
      const ExactHigherSupportStreamBudget& chunk_budget,
      const ExactHigherSupportCheckpoint& reinjected_source) const;

  [[nodiscard]] ExactHigherSupportStreamChunkVerification commit_prepared(
      const ExactHigherSupportStreamBudget& chunk_budget,
      const ExactHigherSupportCheckpoint& reinjected_source,
      const ExactHigherSupportStreamChunk& candidate);

 private:
  ExactHigherSupportAuthorityContext authority_;
  ExactHigherSupportCheckpoint trusted_checkpoint_{};
};

struct ExactHigherSupportStreamRunVerification {
  bool initial_checkpoint_reconstructed{false};
  std::size_t verified_chunk_count{};
  bool every_transition_verified{false};
  bool terminal_checkpoint_reached{false};
  bool anchored_run_certified{false};
};

// Exact for every size_t point count, including the 10M product target.
[[nodiscard]] exact::BigInt exact_higher_support_candidate_universe_size(
    std::size_t point_count);

[[nodiscard]] ExactHigherSupportStreamResult
build_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget);

[[nodiscard]] ExactHigherSupportCheckpointManifest
make_exact_higher_support_checkpoint_manifest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order);

[[nodiscard]] ExactHigherSupportCheckpoint
make_initial_exact_higher_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order);

[[nodiscard]] ExactHigherSupportCheckpoint
make_initial_exact_higher_support_checkpoint(
    const ExactHigherSupportAuthorityContext& authority);

[[nodiscard]] contract::CanonicalId
compute_exact_higher_support_checkpoint_digest(
    const ExactHigherSupportCheckpoint& checkpoint);

[[nodiscard]] ExactHigherSupportCheckpointVerification
verify_exact_higher_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportCheckpoint& checkpoint);

[[nodiscard]] ExactHigherSupportCheckpointVerification
verify_exact_higher_support_checkpoint(
    const ExactHigherSupportAuthorityContext& authority,
    const ExactHigherSupportCheckpoint& checkpoint);

[[nodiscard]] ExactHigherSupportStreamRunVerification
verify_exact_higher_support_stream_run(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    std::span<const ExactHigherSupportStreamBudget> chunk_budgets,
    std::span<const ExactHigherSupportStreamChunk> chunks);

struct ExactHigherSupportStreamVerification {
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool exact_bigint_universe_certified{false};
  bool partial_records_individually_exact{false};
  bool prune_certificates_replayed{false};
  bool grouped_frontier_replayed{false};
  bool completion_claim_certified{false};
  bool absence_claim_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

// Rebuilds from the immutable cloud, LBVH, K and trusted budget.  No field of
// observed steers the traversal or any geometric decision.
[[nodiscard]] ExactHigherSupportStreamVerification
verify_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget,
    const ExactHigherSupportStreamResult& observed);

}  // namespace morsehgp3d::hierarchy
