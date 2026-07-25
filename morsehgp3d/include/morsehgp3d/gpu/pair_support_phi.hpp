#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class PairSupportPhiContextState;
struct PairSupportPhiNodeInputRecord;
}  // namespace detail

// One fixed LBVH support product and one disjoint witness subtree.  The two
// support intervals must occur in this order in Morton space.  A canonical
// batch is strictly increasing in lexicographic order of the node-index
// triples; the three indices inside one triple need not be increasing.
struct PairSupportPhiWitnessQuery {
  std::uint64_t first_support_node_index{};
  std::uint64_t second_support_node_index{};
  std::uint64_t witness_node_index{};

  friend bool operator==(
      const PairSupportPhiWitnessQuery&,
      const PairSupportPhiWitnessQuery&) = default;
};

// This is only the device transcript.  proposed_strict_interior is not a
// scientific decision until the separate CPU record carries an exact receipt.
enum class PairSupportPhiProposalKind : std::uint8_t {
  proposed_strict_interior,
  requires_descent,
};

struct PairSupportPhiProposalRecord {
  PairSupportPhiWitnessQuery query{};
  PairSupportPhiProposalKind kind{
      PairSupportPhiProposalKind::requires_descent};
  // Directed binary64 upper enclosure of the continuous AABB maximum.  It may
  // be +infinity only for requires_descent.
  std::uint64_t upper_phi_binary64_bits{};

  friend bool operator==(
      const PairSupportPhiProposalRecord&,
      const PairSupportPhiProposalRecord&) = default;
};

enum class PairSupportPhiDecision : std::uint8_t {
  certified_strict_interior,
  descend,
};

struct PairSupportPhiCertifiedReceipt {
  // Fixed-width checkpoint-compatible identity of the recertified witness
  // subtree.  The range comes from the immutable host snapshot, not the GPU.
  hierarchy::ExactPairSupportWitnessNodeEntry witness_node{};
  hierarchy::ExactDiametralPhiAabbMaximum exact_phi_maximum{};

  friend bool operator==(
      const PairSupportPhiCertifiedReceipt&,
      const PairSupportPhiCertifiedReceipt&) = default;
};

struct PairSupportPhiDecisionRecord {
  PairSupportPhiWitnessQuery query{};
  PairSupportPhiDecision decision{PairSupportPhiDecision::descend};
  // Present exactly for certified_strict_interior.  It is recomputed from the
  // immutable exact dyadic LBVH snapshot and never trusted from the GPU.
  std::optional<PairSupportPhiCertifiedReceipt> exact_receipt;

  friend bool operator==(
      const PairSupportPhiDecisionRecord&,
      const PairSupportPhiDecisionRecord&) = default;
};

struct PairSupportPhiAudit {
  static constexpr const char* proposal_semantics =
      "cuda_outward_binary64_diametral_phi_upper_bound_proposal_only";
  static constexpr const char* decision_semantics =
      "cpu_exact_dyadic_aabb_phi_recertified_strict_witness_receipt_only";

  std::size_t resident_lbvh_node_count{};
  std::size_t maximum_query_count{};
  std::size_t canonical_query_count{};
  std::size_t gpu_output_record_count{};
  std::size_t gpu_strict_interior_proposal_count{};
  std::size_t gpu_requires_descent_count{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t cpu_exact_phi_recertification_count{};
  std::size_t certified_strict_interior_receipt_count{};
  std::uint64_t buffer_epoch{};
  std::uint64_t proposal_digest_fnv1a{};
  std::optional<exact::ExactRational> minimum_certified_strict_margin;
  bool immutable_lbvh_snapshot_validated{false};
  bool canonical_query_order_validated{false};
  bool exhaustive_proposal_permutation_validated{false};
  bool cpu_exact_recertification_complete{false};
  // A collection of disjoint receipts still has to meet the rank threshold in
  // the CPU pair-support traversal.  This primitive never decides that prune.
  bool global_support_product_prune_published{false};
  bool public_status_published{false};

  friend bool operator==(
      const PairSupportPhiAudit&,
      const PairSupportPhiAudit&) = default;
};

struct PairSupportPhiBatchResult {
  // The two arenas deliberately keep proposal and certified decision apart.
  // Both follow the canonical input-query order.
  std::vector<PairSupportPhiProposalRecord> proposals;
  std::vector<PairSupportPhiDecisionRecord> decisions;
  PairSupportPhiAudit audit;

  friend bool operator==(
      const PairSupportPhiBatchResult&,
      const PairSupportPhiBatchResult&) = default;
};

struct PairSupportPhiNodeDescriptor {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  spatial::ExactDyadicAabb3 exact_bounds{};

  friend bool operator==(
      const PairSupportPhiNodeDescriptor&,
      const PairSupportPhiNodeDescriptor&) = default;
};

// Fixed resident capacities for the bounded Phase 9.1-CUDA-P2 traversal.
// maximum_work_item_count bounds each of the two device frontiers, not their
// sum.  maximum_receipt_count is retained as a source-compatible name for the
// capacity C of the unified 16-byte terminal transcript returned by one call.
// Zero capacities leave P2 disabled while preserving the P1-only constructor.
struct PairSupportRankPruneCapacity {
  std::size_t maximum_product_count{};
  std::size_t maximum_work_item_count{};
  std::size_t maximum_receipt_count{};

  friend bool operator==(
      const PairSupportRankPruneCapacity&,
      const PairSupportRankPruneCapacity&) = default;
};

struct PairSupportRankPruneBudget {
  // One epoch is one stable count -> exclusive-scan -> emit transition over
  // the current device frontier.  A product whose transcript neither reaches
  // the strict threshold nor covers the root at this cap remains a fallback.
  std::size_t maximum_epoch_count{};

  friend bool operator==(
      const PairSupportRankPruneBudget&,
      const PairSupportRankPruneBudget&) = default;
};

struct PairSupportRankPruneProductProposal {
  std::size_t product_index{};
  hierarchy::ExactPairSupportFrontierEntry product{};
  // A disjoint LBVH antichain sorted by Morton range then node identity and
  // truncated to the first prefix reaching the threshold.  Every entry has
  // been independently replayed with exact dyadic arithmetic on the CPU
  // before it appears here.
  std::vector<hierarchy::ExactPairSupportWitnessNodeEntry>
      strict_witness_receipts;
  std::size_t strict_interior_point_count{};

  friend bool operator==(
      const PairSupportRankPruneProductProposal&,
      const PairSupportRankPruneProductProposal&) = default;
};

enum class PairSupportRankTerminalKind : std::uint8_t {
  strict_interior,
  anchor_noninterior,
  unresolved_external_leaf,
};

struct PairSupportRankTerminalCertificate {
  hierarchy::ExactPairSupportWitnessNodeEntry terminal{};
  PairSupportRankTerminalKind kind{
      PairSupportRankTerminalKind::unresolved_external_leaf};

  friend bool operator==(
      const PairSupportRankTerminalCertificate&,
      const PairSupportRankTerminalCertificate&) = default;
};

struct PairSupportRankKeepProductCertificate {
  std::size_t product_index{};
  hierarchy::ExactPairSupportFrontierEntry product{};
  // The two support ranges are implicit.  Together with this Morton-canonical
  // terminal antichain they exactly tile the resident root range.
  std::vector<PairSupportRankTerminalCertificate> canonical_terminals;
  std::size_t strict_interior_point_count{};

  friend bool operator==(
      const PairSupportRankKeepProductCertificate&,
      const PairSupportRankKeepProductCertificate&) = default;
};

struct PairSupportRankPruneAudit {
  static constexpr const char* proposal_semantics =
      "cuda_bounded_two_frontier_rank_prune_or_keep_certificate";
  static constexpr const char* receipt_semantics =
      "cpu_exact_unified_terminal_antichain_and_coverage_recertified";

  PairSupportRankPruneCapacity capacity{};
  PairSupportRankPruneBudget budget{};
  std::size_t input_product_count{};
  std::size_t required_strict_interior_point_count{};
  std::size_t gpu_traversal_epoch_count{};
  std::size_t gpu_count_kernel_launch_count{};
  std::size_t gpu_exclusive_scan_count{};
  std::size_t gpu_emit_kernel_launch_count{};
  std::size_t gpu_visited_work_item_count{};
  std::size_t gpu_peak_frontier_count{};
  std::size_t gpu_output_terminal_count{};
  std::size_t cpu_exact_terminal_recertification_count{};
  std::size_t strict_interior_terminal_count{};
  std::size_t anchor_noninterior_terminal_count{};
  std::size_t unresolved_external_leaf_terminal_count{};
  std::size_t prune_product_count{};
  std::size_t keep_certificate_product_count{};
  // Source-compatible diagnostic aliases.  A "receipt" now denotes any
  // recertified terminal record, not only a strict-interior terminal.
  std::size_t gpu_output_receipt_count{};
  std::size_t cpu_exact_phi_recertification_count{};
  std::size_t proposed_product_count{};
  std::size_t fallback_product_count{};
  std::size_t snapshot_h2d_byte_count{};
  std::size_t active_product_h2d_byte_count{};
  std::size_t initial_frontier_h2d_byte_count{};
  std::size_t traversal_metadata_d2h_byte_count{};
  // The host receives only the compact active prefix, so physical and active
  // terminal D2H bytes are equal.  Device capacity remains audited separately.
  std::size_t physical_terminal_d2h_byte_count{};
  std::size_t active_terminal_d2h_byte_count{};
  std::size_t device_terminal_byte_capacity{};
  // Source-compatible aliases of the three terminal-byte counters above.
  std::size_t physical_receipt_d2h_byte_count{};
  std::size_t active_receipt_d2h_byte_count{};
  std::size_t device_frontier_double_buffer_byte_capacity{};
  std::size_t device_receipt_byte_capacity{};
  std::size_t device_scan_workspace_byte_capacity{};
  // Exact P2-only resident total outside the shared LBVH snapshot and any P1
  // query buffers: 40P + 80W + 16C + 8 + scan_workspace bytes.
  std::size_t device_fixed_workspace_byte_capacity{};
  std::uint64_t buffer_epoch{};
  std::uint64_t terminal_digest_fnv1a{};
  // Source-compatible alias of terminal_digest_fnv1a.
  std::uint64_t receipt_digest_fnv1a{};
  bool work_item_capacity_exhausted{false};
  bool receipt_capacity_exhausted{false};
  bool epoch_budget_exhausted{false};
  bool device_frontier_exhausted{false};
  bool immutable_lbvh_snapshot_validated{false};
  bool product_records_validated{false};
  bool stable_terminal_transcript_validated{false};
  bool disjoint_terminal_antichains_validated{false};
  bool keep_coverage_recertification_complete{false};
  // Source-compatible aliases of the terminal validation flags above.
  bool stable_receipt_transcript_validated{false};
  bool disjoint_receipt_antichains_validated{false};
  bool cpu_exact_recertification_complete{false};
  // An authenticated singleton anchor from each support enables a conservative
  // non-witness cull.  P4 records it as a non-strict terminal for exact CPU
  // coverage replay; it never creates a strict receipt or a prune authority.
  bool anchor_ball_culling_enabled{false};
  // P2 only proposes a replayable local rank argument.  The CPU stream remains
  // the authority that decides whether a support product is globally pruned.
  bool global_support_product_prune_published{false};
  bool public_status_published{false};

  friend bool operator==(
      const PairSupportRankPruneAudit&,
      const PairSupportRankPruneAudit&) = default;
};

struct PairSupportRankPruneBatchResult {
  // Only products whose recertified strict-terminal cardinality reaches the
  // requested threshold occur here.
  std::vector<PairSupportRankPruneProductProposal> proposals;
  // A keep certificate is a conservative exact closure of the current
  // branch-and-bound rule, not a proof that no real support pair has any
  // interior witness.
  std::vector<PairSupportRankKeepProductCertificate> keep_certificates;
  PairSupportRankPruneAudit audit;

  friend bool operator==(
      const PairSupportRankPruneBatchResult&,
      const PairSupportRankPruneBatchResult&) = default;
};

// Phase 9.1-CUDA-P1/P2: owns one immutable exact AABB/range/topology snapshot
// of an LBVH.  The snapshot is uploaded once on the first nonempty GPU batch.
// P1 storage is bounded by maximum_query_count and optional P2 storage by its
// explicit capacity; there is no pair arena, Gamma, higher-order cell, coface
// or global incidence storage.
class PairSupportPhiContext final {
 public:
  PairSupportPhiContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_query_count);
  PairSupportPhiContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_query_count,
      PairSupportRankPruneCapacity rank_prune_capacity);
  ~PairSupportPhiContext() noexcept;

  PairSupportPhiContext(PairSupportPhiContext&&) noexcept;
  PairSupportPhiContext& operator=(PairSupportPhiContext&&) noexcept;

  PairSupportPhiContext(const PairSupportPhiContext&) = delete;
  PairSupportPhiContext& operator=(const PairSupportPhiContext&) = delete;

  [[nodiscard]] PairSupportPhiBatchResult classify_witnesses(
      std::span<const PairSupportPhiWitnessQuery> canonical_queries);

  // Phase 9.1-CUDA-P2: traverses the witness LBVH for a bounded batch of
  // already authenticated support products.  The device returns only a
  // bounded terminal transcript.  Exact CPU replay derives either a historical
  // prune proposal, an exact conservative keep certificate, or a fallback.
  [[nodiscard]] PairSupportRankPruneBatchResult propose_rank_prunes(
      std::span<const hierarchy::ExactPairSupportFrontierEntry> products,
      std::size_t required_strict_interior_point_count,
      PairSupportRankPruneBudget budget);

  // Convenience seam for leaf fixtures and leaf-stage batching.  PointIds
  // must be distinct.  The two supports are reordered by Morton range, so the
  // returned singleton query is canonical without exposing private numbering.
  [[nodiscard]] PairSupportPhiWitnessQuery make_leaf_witness_query(
      spatial::PointId first_support_id,
      spatial::PointId second_support_id,
      spatial::PointId witness_id) const;

  [[nodiscard]] std::size_t node_count() const noexcept;
  // Read-only O(1) view of the canonical resident snapshot.  It lets the
  // pair-support traversal and qualification fixtures form queries without
  // exposing MortonLbvhIndex internals or constructing a node-pair arena.
  [[nodiscard]] PairSupportPhiNodeDescriptor node_descriptor(
      std::size_t node_index) const;
  [[nodiscard]] std::size_t maximum_query_count() const noexcept {
    return maximum_query_count_;
  }
  [[nodiscard]] PairSupportRankPruneCapacity rank_prune_capacity()
      const noexcept {
    return rank_prune_capacity_;
  }

 private:
  std::shared_ptr<detail::PairSupportPhiContextState> state_;
  std::vector<detail::PairSupportPhiNodeInputRecord> nodes_;
  std::vector<std::uint64_t> leaf_node_index_by_point_id_;
  std::size_t maximum_query_count_{};
  std::uint64_t last_buffer_epoch_{};
  PairSupportRankPruneCapacity rank_prune_capacity_{};
  std::uint64_t root_node_index_{};
  std::uint64_t last_rank_prune_buffer_epoch_{};
};

}  // namespace morsehgp3d::gpu
