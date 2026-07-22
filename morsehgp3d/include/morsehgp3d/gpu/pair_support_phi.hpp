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

// Phase 9.1-CUDA-P1: owns one immutable exact AABB/range snapshot of an LBVH.
// The snapshot is uploaded once on the first nonempty batch.  Per-batch device
// storage is bounded by maximum_query_count; there is no pair arena, Gamma,
// higher-order cell, coface or global incidence storage.
class PairSupportPhiContext final {
 public:
  PairSupportPhiContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_query_count);
  ~PairSupportPhiContext() noexcept;

  PairSupportPhiContext(PairSupportPhiContext&&) noexcept;
  PairSupportPhiContext& operator=(PairSupportPhiContext&&) noexcept;

  PairSupportPhiContext(const PairSupportPhiContext&) = delete;
  PairSupportPhiContext& operator=(const PairSupportPhiContext&) = delete;

  [[nodiscard]] PairSupportPhiBatchResult classify_witnesses(
      std::span<const PairSupportPhiWitnessQuery> canonical_queries);

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

 private:
  std::shared_ptr<detail::PairSupportPhiContextState> state_;
  std::vector<detail::PairSupportPhiNodeInputRecord> nodes_;
  std::vector<std::uint64_t> leaf_node_index_by_point_id_;
  std::size_t maximum_query_count_{};
  std::uint64_t last_buffer_epoch_{};
};

}  // namespace morsehgp3d::gpu
