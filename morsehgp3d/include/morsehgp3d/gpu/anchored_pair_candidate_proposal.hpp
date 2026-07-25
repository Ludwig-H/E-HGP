#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class Phase14AnchoredPairCandidateProposalContextState;
class Phase14AnchoredPairCandidateProposalHostState;
}  // namespace detail

inline constexpr std::uint32_t
    anchored_pair_candidate_gpu_proposal_schema_version = 1U;
inline constexpr std::size_t
    anchored_pair_candidate_gpu_maximum_witness_bank_size = 64U;
inline constexpr std::size_t
    anchored_pair_candidate_gpu_maximum_closed_rank = 11U;
inline constexpr std::uint64_t
    anchored_pair_candidate_gpu_invalid_node_index = ~UINT64_C(0);
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_backend = "cuda_g4";
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_profile = "hgp_reduced";
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_mode = "proposal_only";
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_public_status = "not_claimed";
inline constexpr std::string_view
    anchored_pair_candidate_gpu_proposal_proof_basis =
        "bounded_stackless_device_lbvh_anchor_witness_mask_transcript_"
        "hostile_host_validation_then_reference_cpu_exact_recertification_v1";

enum class AnchoredPairCandidateProposalCursorKind : std::uint8_t {
  initial,
  active,
  complete,
};

// A stackless traversal resumes from one device-node index without retaining
// rope/successor metadata.  Strict postorder makes the successor arithmetic:
// inverse root-right-left DFS descends from i to i-1, while skipping a subtree
// with m leaves jumps to i-(2m-1).  The digest binds an active cursor to one
// anchor, rank and witness bank; the lease epoch prevents replay against
// another device snapshot.  An initial cursor has zero digest/epoch and the
// invalid node sentinel.
struct AnchoredPairCandidateProposalCursor {
  AnchoredPairCandidateProposalCursorKind kind{
      AnchoredPairCandidateProposalCursorKind::initial};
  std::uint64_t next_node_index{
      anchored_pair_candidate_gpu_invalid_node_index};
  std::uint64_t query_digest_fnv1a{};
  std::uint64_t source_snapshot_epoch{};

  friend bool operator==(
      const AnchoredPairCandidateProposalCursor&,
      const AnchoredPairCandidateProposalCursor&) = default;
};

// Borrowed only for the duration of build().  The upstream (distance, PointId)
// order is semantically significant: it puts the best witnesses in the first
// warp wave and is preserved by the digest and mask bits.  PointIds need only
// be distinct, in domain and different from the anchor.  A bank with fewer
// than maximum_closed_rank-1 entries is valid but cannot propose a prune; it
// remains a fail-open candidate traversal.
struct AnchoredPairCandidateProposalQuery {
  spatial::PointId anchor_point_id{};
  std::size_t maximum_closed_rank{};
  std::span<const spatial::PointId> witness_bank_point_ids;
  AnchoredPairCandidateProposalCursor cursor{};
};

struct AnchoredPairCandidateProposalBudget {
  std::size_t maximum_node_visit_count_per_query{};
  std::size_t maximum_transcript_record_count_per_query{};
  std::size_t maximum_total_transcript_record_count{};

  friend bool operator==(
      const AnchoredPairCandidateProposalBudget&,
      const AnchoredPairCandidateProposalBudget&) = default;
};

// Exactly sixteen bytes on the host/device transcript boundary.  Node indices
// are strictly decreasing inside each inverse-postorder segment, which also
// certifies uniqueness without a host allocation.  mask==0 is only a
// candidate-leaf proposal.  A nonzero mask is only a prune proposal and must
// contain exactly maximum_closed_rank-1 in-bank bits.  Neither case is a
// scientific decision before the future CPU recertifier validates the named
// node and every claimed witness.
struct AnchoredPairCandidateProposalRecord {
  std::uint64_t node_index{anchored_pair_candidate_gpu_invalid_node_index};
  std::uint64_t witness_mask{};

  friend bool operator==(
      const AnchoredPairCandidateProposalRecord&,
      const AnchoredPairCandidateProposalRecord&) = default;
};

static_assert(
    std::is_standard_layout_v<AnchoredPairCandidateProposalRecord>);
static_assert(
    std::is_trivially_copyable_v<AnchoredPairCandidateProposalRecord>);
static_assert(sizeof(AnchoredPairCandidateProposalRecord) == 16U);

enum class AnchoredPairCandidateProposalSegmentStatus : std::uint8_t {
  complete,
  budget_exhausted,
  capacity_exhausted,
};

enum class AnchoredPairCandidateProposalStopReason : std::uint8_t {
  none,
  node_visit_limit,
  transcript_record_limit,
};

struct AnchoredPairCandidateProposalSegment {
  std::size_t query_index{};
  std::uint64_t query_digest_fnv1a{};
  AnchoredPairCandidateProposalCursor input_cursor{};
  AnchoredPairCandidateProposalCursor next_cursor{};
  AnchoredPairCandidateProposalSegmentStatus status{
      AnchoredPairCandidateProposalSegmentStatus::budget_exhausted};
  AnchoredPairCandidateProposalStopReason stop_reason{
      AnchoredPairCandidateProposalStopReason::node_visit_limit};
  std::size_t node_visit_count{};
  std::size_t record_begin{};
  std::size_t record_count{};

  friend bool operator==(
      const AnchoredPairCandidateProposalSegment&,
      const AnchoredPairCandidateProposalSegment&) = default;
};

struct AnchoredPairCandidateProposalAudit {
  static constexpr std::string_view proposal_semantics =
      "cuda_stackless_lbvh_node_and_witness_mask_hints_only";
  static constexpr std::string_view decision_semantics =
      "reference_cpu_exact_node_leaf_and_strict_witness_recertification";

  std::size_t maximum_query_count{};
  std::size_t physical_transcript_record_capacity{};
  std::size_t active_total_transcript_record_limit{};
  std::size_t compacted_host_transcript_record_count{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  std::size_t persistent_device_byte_capacity{};
  std::uint64_t source_snapshot_epoch{};
  std::size_t canonical_query_count{};
  std::size_t witness_point_id_reference_count{};
  std::size_t maximum_observed_witness_bank_size{};
  std::size_t active_host_to_device_query_byte_count{};
  std::size_t initialized_device_segment_byte_count{};
  std::size_t initialized_device_transcript_byte_count{};
  std::size_t copied_device_to_host_segment_byte_count{};
  std::size_t copied_device_to_host_transcript_byte_count{};
  std::size_t node_visit_count{};
  std::size_t candidate_leaf_proposal_count{};
  std::size_t prune_proposal_count{};
  std::size_t complete_segment_count{};
  std::size_t budget_exhausted_segment_count{};
  std::size_t capacity_exhausted_segment_count{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_compaction_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  int cuda_device{-1};
  std::uint64_t buffer_epoch{};
  std::uint64_t proposal_digest_fnv1a{};
  bool matching_immutable_point_namespace_validated{false};
  bool traversal_lease_owner_retained{false};
  bool traversal_lease_device_validated{false};
  bool traversal_lease_extents_validated{false};
  bool canonical_query_order_validated{false};
  bool witness_banks_validated{false};
  bool input_cursors_validated{false};
  bool fixed_capacity_preflight_satisfied{false};
  bool fixed_resident_capacity_bound_at_construction{false};
  bool deterministic_query_order_compaction_validated{false};
  bool segment_partition_validated{false};
  bool record_node_domains_validated{false};
  bool candidate_leaf_masks_validated{false};
  bool prune_mask_cardinalities_validated{false};
  bool transcript_digest_validated{false};
  bool gpu_execution_performed{false};
  bool device_scan_compaction_performed{false};
  bool candidate_leaf_status_recertified{false};
  bool strict_witness_masks_recertified{false};
  bool exact_closed_ball_partition_published{false};
  bool scientific_decision_published{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool forbidden_global_structure_materialized{false};
  bool transcript_accumulated_across_calls{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const AnchoredPairCandidateProposalAudit&,
      const AnchoredPairCandidateProposalAudit&) = default;
};

struct AnchoredPairCandidateProposalBatchResult {
  static constexpr std::string_view backend =
      anchored_pair_candidate_gpu_proposal_backend;
  static constexpr std::string_view profile =
      anchored_pair_candidate_gpu_proposal_profile;
  static constexpr std::string_view mode =
      anchored_pair_candidate_gpu_proposal_mode;
  static constexpr std::string_view deployment_status =
      anchored_pair_candidate_gpu_proposal_deployment_status;
  static constexpr std::string_view public_status =
      anchored_pair_candidate_gpu_proposal_public_status;
  static constexpr std::string_view proof_basis =
      anchored_pair_candidate_gpu_proposal_proof_basis;

  std::uint32_t schema_version{
      anchored_pair_candidate_gpu_proposal_schema_version};
  std::vector<AnchoredPairCandidateProposalSegment> segments;
  std::vector<AnchoredPairCandidateProposalRecord> records;
  AnchoredPairCandidateProposalAudit audit{};

  [[nodiscard]] bool validated_proposal_batch() const noexcept;
  [[nodiscard]] bool scientific_outcome() const noexcept;

  friend bool operator==(
      const AnchoredPairCandidateProposalBatchResult&,
      const AnchoredPairCandidateProposalBatchResult&) = default;
};

// The context consumes one move-only traversal lease and never creates a
// second O(n) snapshot.  Its resident storage is fixed by construction; every
// build returns only the current bounded transcript and retains no history.
// Calls are synchronous and serialized.  Any launcher or transcript validation
// failure poisons the context.
class AnchoredPairCandidateProposalContext final {
 public:
  AnchoredPairCandidateProposalContext(
      const spatial::CanonicalPointCloud& cloud,
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      std::size_t maximum_query_count,
      std::size_t maximum_transcript_record_count);
  ~AnchoredPairCandidateProposalContext() noexcept;

  AnchoredPairCandidateProposalContext(
      AnchoredPairCandidateProposalContext&&) noexcept;
  AnchoredPairCandidateProposalContext& operator=(
      AnchoredPairCandidateProposalContext&&) noexcept;

  AnchoredPairCandidateProposalContext(
      const AnchoredPairCandidateProposalContext&) = delete;
  AnchoredPairCandidateProposalContext& operator=(
      const AnchoredPairCandidateProposalContext&) = delete;

  [[nodiscard]] AnchoredPairCandidateProposalBatchResult build(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const AnchoredPairCandidateProposalQuery> canonical_queries,
      AnchoredPairCandidateProposalBudget budget);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t certified_node_count() const noexcept;
  [[nodiscard]] std::size_t maximum_query_count() const noexcept;
  [[nodiscard]] std::size_t maximum_transcript_record_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  std::shared_ptr<detail::Phase14AnchoredPairCandidateProposalContextState>
      state_;
  std::unique_ptr<detail::Phase14AnchoredPairCandidateProposalHostState>
      host_;
  std::size_t maximum_query_count_{};
  std::size_t maximum_transcript_record_count_{};
  std::uint64_t last_buffer_epoch_{};
};

}  // namespace morsehgp3d::gpu
