#pragma once

#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

class ExactGroupedAnchoredPairTraversalContext;

inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_anchor_count = 32U;
inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_witness_pool_size = 64U;
inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_closed_rank = 11U;
static_assert(
    exact_grouped_anchored_pair_maximum_witness_pool_size <=
    std::numeric_limits<std::uint64_t>::digits);
inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_traversal_stack_entry_count =
        3U * spatial::MortonLbvhIndex::morton_bits_per_axis +
        std::numeric_limits<std::size_t>::digits + 1U;
// A radix path consumes at most the 63 Morton bits.  Once a range has one
// repeated code, find_split halves it, adding at most digits(size_t) levels.
// A depth-first binary traversal needs at most maximum_depth + 1 frames.
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_backend = "reference_cpu";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_mode =
        "grouped_exact_certificate";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_public_status = "not_claimed";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_proof_basis =
        "bounded_group_witness_pool_and_exact_discrete_anchor_query_aabb_"
        "strict_diametral_phi_maximum_v2";

enum class ExactGroupedAnchoredPairPruneDecision : std::uint8_t {
  certified,
  inconclusive,
  budget_exhausted,
};

enum class ExactGroupedAnchoredPairPruneStopReason : std::uint8_t {
  none,
  anchor_count_limit,
  witness_pool_entry_limit,
  exact_predicate_limit,
};

enum class ExactGroupedAnchoredPairPruneCertificateOrigin : std::uint8_t {
  single_node,
  prepared_inherited_traversal,
};

// The built-in G <= 32 and W <= 64 limits are structural.  These caller caps
// may be smaller and make an otherwise valid request fail open without
// publishing partial witness authority.
struct ExactGroupedAnchoredPairPruneBudget {
  std::size_t maximum_anchor_count{};
  std::size_t maximum_witness_pool_entry_count{};
  std::size_t maximum_exact_predicate_count{};

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneBudget&,
      const ExactGroupedAnchoredPairPruneBudget&) = default;
};

struct ExactGroupedAnchoredPairPruneAudit {
  std::size_t anchor_count{};
  std::size_t witness_pool_entry_count{};
  // One physical predicate is one exact (actual anchor, witness, query AABB)
  // sign evaluation.  A proposed witness can therefore charge from one to G
  // predicates before it fails or is proved strict for all G actual anchors.
  std::size_t exact_predicate_count{};
  std::size_t strict_group_witness_count{};
  std::size_t inherited_strict_group_witness_count{};
  bool requested_budget_applies{false};
  bool input_canonical{false};
  bool anchor_bounds_constructed{false};
  bool lbvh_node_authority_verified{false};
  bool complete{false};

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneAudit&,
      const ExactGroupedAnchoredPairPruneAudit&) = default;
};

// A certified result proves one shared prune for exactly the bound anchor
// group and immutable LBVH node.  certified() reports the local decision;
// certifies() must be used before replay so process-local cloud/LBVH identity,
// node range and anchor provenance are checked together.  An inconclusive or
// exhausted result carries no witness authority.
class ExactGroupedAnchoredPairPruneCertificate {
 public:
  static constexpr std::string_view backend =
      exact_grouped_anchored_pair_certificate_backend;
  static constexpr std::string_view profile =
      exact_grouped_anchored_pair_certificate_profile;
  static constexpr std::string_view mode =
      exact_grouped_anchored_pair_certificate_mode;
  static constexpr std::string_view deployment_status =
      exact_grouped_anchored_pair_certificate_deployment_status;
  static constexpr std::string_view public_status =
      exact_grouped_anchored_pair_certificate_public_status;
  static constexpr std::string_view proof_basis =
      exact_grouped_anchored_pair_certificate_proof_basis;

  ExactGroupedAnchoredPairPruneCertificate(
      const ExactGroupedAnchoredPairPruneCertificate&) = default;
  ExactGroupedAnchoredPairPruneCertificate(
      ExactGroupedAnchoredPairPruneCertificate&&) noexcept = default;
  ExactGroupedAnchoredPairPruneCertificate& operator=(
      const ExactGroupedAnchoredPairPruneCertificate&) = default;
  ExactGroupedAnchoredPairPruneCertificate& operator=(
      ExactGroupedAnchoredPairPruneCertificate&&) noexcept = default;
  ~ExactGroupedAnchoredPairPruneCertificate() = default;

  [[nodiscard]] ExactGroupedAnchoredPairPruneDecision decision() const
      noexcept {
    return decision_;
  }

  [[nodiscard]] ExactGroupedAnchoredPairPruneStopReason stop_reason() const
      noexcept {
    return stop_reason_;
  }

  [[nodiscard]] ExactGroupedAnchoredPairPruneCertificateOrigin origin() const
      noexcept {
    return origin_;
  }

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] std::size_t required_witness_count() const noexcept {
    return required_witness_count_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairPruneBudget& requested_budget()
      const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneBudget& requested_budget()
      const && = delete;

  [[nodiscard]] const ExactGroupedAnchoredPairPruneAudit& audit() const
      & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneAudit& audit() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const
      & noexcept {
    return {anchor_point_ids_.data(), anchor_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const & noexcept {
    return {witness_pool_point_ids_.data(), witness_pool_entry_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const && = delete;

  [[nodiscard]] const spatial::ExactDyadicAabb3& anchor_bounds() const
      & noexcept {
    return anchor_bounds_;
  }
  [[nodiscard]] const spatial::ExactDyadicAabb3& anchor_bounds() const && =
      delete;

  [[nodiscard]] std::size_t lbvh_node_index() const noexcept {
    return lbvh_node_index_;
  }

  [[nodiscard]] std::size_t leaf_begin() const noexcept {
    return leaf_begin_;
  }

  [[nodiscard]] std::size_t leaf_end() const noexcept {
    return leaf_end_;
  }

  [[nodiscard]] const spatial::ExactDyadicAabb3& query_bounds() const
      & noexcept {
    return query_bounds_;
  }
  [[nodiscard]] const spatial::ExactDyadicAabb3& query_bounds() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId>
  certified_witness_point_ids() const & noexcept {
    return {certified_witness_point_ids_.data(), certified_witness_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId>
  certified_witness_point_ids() const && = delete;

  [[nodiscard]] std::uint64_t certified_witness_pool_mask() const noexcept {
    return certified_witness_pool_mask_;
  }

  [[nodiscard]] bool certified() const noexcept {
    return decision_ == ExactGroupedAnchoredPairPruneDecision::certified;
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

  [[nodiscard]] bool certifies(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t expected_lbvh_node_index,
      std::size_t expected_maximum_closed_rank,
      std::span<const spatial::PointId> expected_anchor_point_ids) const
      noexcept;

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneCertificate&,
      const ExactGroupedAnchoredPairPruneCertificate&) = default;

 private:
  ExactGroupedAnchoredPairPruneCertificate() = default;

  ExactGroupedAnchoredPairPruneDecision decision_{
      ExactGroupedAnchoredPairPruneDecision::inconclusive};
  ExactGroupedAnchoredPairPruneStopReason stop_reason_{
      ExactGroupedAnchoredPairPruneStopReason::none};
  ExactGroupedAnchoredPairPruneCertificateOrigin origin_{
      ExactGroupedAnchoredPairPruneCertificateOrigin::single_node};
  std::size_t maximum_closed_rank_{};
  std::size_t required_witness_count_{};
  ExactGroupedAnchoredPairPruneBudget requested_budget_{};
  ExactGroupedAnchoredPairPruneAudit audit_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_ids_{};
  std::size_t anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_pool_point_ids_{};
  std::size_t witness_pool_entry_count_{};
  spatial::ExactDyadicAabb3 anchor_bounds_{};
  std::size_t lbvh_node_index_{};
  std::size_t leaf_begin_{};
  std::size_t leaf_end_{};
  spatial::ExactDyadicAabb3 query_bounds_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_closed_rank - 1U>
      certified_witness_point_ids_{};
  std::size_t certified_witness_count_{};
  std::uint64_t certified_witness_pool_mask_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;

  friend class ExactGroupedAnchoredPairPruneCertifier;
  friend class ExactGroupedAnchoredPairTraversalContext;
};

enum class ExactGroupedAnchoredPairTraversalStepKind : std::uint8_t {
  certified_prune,
  // The common certificate is inconclusive for this whole authenticated
  // subtree.  Frontier mode delegates the exact product to a finer bounded
  // consumer instead of silently expanding it with the same anchor group.
  inconclusive_subtree,
  unresolved_leaf,
  fallback_subtree,
  budget_exhausted,
  complete,
};

enum class ExactGroupedAnchoredPairTraversalStopReason : std::uint8_t {
  none,
  node_visit_limit,
  exact_predicate_limit,
};

struct ExactGroupedAnchoredPairTraversalWorkBudget {
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_exact_predicate_count{};

  friend bool operator==(
      const ExactGroupedAnchoredPairTraversalWorkBudget&,
      const ExactGroupedAnchoredPairTraversalWorkBudget&) = default;
};

struct ExactGroupedAnchoredPairTraversalStepWork {
  std::size_t node_visit_count{};
  // A witness slot is charged once when it is rejected by one actual anchor,
  // proved strict for every actual anchor, or reused from a strict parent.
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  // Physical actual-anchor/witness exact signs, excluding inherited slots.
  std::size_t exact_predicate_count{};
  std::size_t strict_witness_discovery_count{};

  friend bool operator==(
      const ExactGroupedAnchoredPairTraversalStepWork&,
      const ExactGroupedAnchoredPairTraversalStepWork&) = default;
};

struct ExactGroupedAnchoredPairTraversalAudit {
  std::size_t advance_call_count{};
  std::size_t anchor_bounds_construction_count{};
  std::size_t prepared_witness_point_count{};
  std::size_t node_visit_count{};
  std::size_t witness_slot_scan_count{};
  std::size_t inherited_witness_reuse_count{};
  std::size_t exact_predicate_count{};
  std::size_t strict_witness_discovery_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t diagonal_node_descent_count{};
  std::size_t certified_prune_count{};
  std::size_t inconclusive_subtree_count{};
  std::size_t unresolved_leaf_count{};
  std::size_t fallback_subtree_count{};
  std::size_t budget_exhaustion_count{};
  std::size_t maximum_pending_node_count{};
  bool complete{false};
  bool no_dynamic_traversal_or_output_arena{true};

  friend bool operator==(
      const ExactGroupedAnchoredPairTraversalAudit&,
      const ExactGroupedAnchoredPairTraversalAudit&) = default;
};

// One immutable output event from the prepared traversal.  A positive event
// carries the existing P8g certificate and must still be consumed through
// certifies().  An unresolved leaf is only a candidate for later per-anchor
// orientation/classification, never G automatically valid pairs.  Budget
// exhaustion publishes neither a mask nor a partial certificate.
class ExactGroupedAnchoredPairTraversalStep {
 public:
  ExactGroupedAnchoredPairTraversalStep(
      const ExactGroupedAnchoredPairTraversalStep&) = default;
  ExactGroupedAnchoredPairTraversalStep(
      ExactGroupedAnchoredPairTraversalStep&&) noexcept = default;
  ExactGroupedAnchoredPairTraversalStep& operator=(
      const ExactGroupedAnchoredPairTraversalStep&) = default;
  ExactGroupedAnchoredPairTraversalStep& operator=(
      ExactGroupedAnchoredPairTraversalStep&&) noexcept = default;
  ~ExactGroupedAnchoredPairTraversalStep() = default;

  [[nodiscard]] ExactGroupedAnchoredPairTraversalStepKind kind() const
      noexcept {
    return kind_;
  }

  [[nodiscard]] ExactGroupedAnchoredPairTraversalStopReason stop_reason()
      const noexcept {
    return stop_reason_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairTraversalWorkBudget&
  requested_budget() const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairTraversalWorkBudget&
  requested_budget() const && = delete;

  [[nodiscard]] const ExactGroupedAnchoredPairTraversalStepWork& work() const
      & noexcept {
    return work_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairTraversalStepWork& work()
      const && = delete;

  [[nodiscard]] std::size_t advance_call_index() const noexcept {
    return advance_call_index_;
  }

  [[nodiscard]] std::optional<std::size_t> lbvh_node_index() const noexcept {
    return lbvh_node_index_;
  }

  [[nodiscard]] std::optional<std::size_t> leaf_begin() const noexcept {
    return leaf_begin_;
  }

  [[nodiscard]] std::optional<std::size_t> leaf_end() const noexcept {
    return leaf_end_;
  }

  [[nodiscard]] std::optional<spatial::PointId> unresolved_point_id() const
      noexcept {
    return unresolved_point_id_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairPruneCertificate*
  prune_certificate() const & noexcept {
    return prune_certificate_.has_value() ? &*prune_certificate_ : nullptr;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneCertificate*
  prune_certificate() const && = delete;

  [[nodiscard]] bool traversal_complete_after_step() const noexcept {
    return traversal_complete_after_step_;
  }

  friend bool operator==(
      const ExactGroupedAnchoredPairTraversalStep&,
      const ExactGroupedAnchoredPairTraversalStep&) = default;

 private:
  ExactGroupedAnchoredPairTraversalStep() = default;

  ExactGroupedAnchoredPairTraversalStepKind kind_{
      ExactGroupedAnchoredPairTraversalStepKind::budget_exhausted};
  ExactGroupedAnchoredPairTraversalStopReason stop_reason_{
      ExactGroupedAnchoredPairTraversalStopReason::none};
  ExactGroupedAnchoredPairTraversalWorkBudget requested_budget_{};
  ExactGroupedAnchoredPairTraversalStepWork work_{};
  std::size_t advance_call_index_{};
  std::optional<std::size_t> lbvh_node_index_;
  std::optional<std::size_t> leaf_begin_;
  std::optional<std::size_t> leaf_end_;
  std::optional<spatial::PointId> unresolved_point_id_;
  std::optional<ExactGroupedAnchoredPairPruneCertificate>
      prune_certificate_;
  bool traversal_complete_after_step_{false};

  friend class ExactGroupedAnchoredPairTraversalContext;
};

// Prepared P8h traversal.  The context itself is the only cursor: it owns a
// fixed-capacity DFS stack and one resumable active node.  Parent witness bits
// are minted internally for both children from the same immutable parent mask;
// callers can neither supply nor extract that mask.  The context allocates no
// traversal or output arena, and advance() emits at most one event.
class ExactGroupedAnchoredPairTraversalContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "prepared_grouped_exact_traversal";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view proof_basis =
      "prepared_discrete_actual_anchor_bounds_exact_child_aabb_monotone_"
      "inherited_common_strict_witness_mask_v2";

  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_root(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> anchor_point_ids,
      std::span<const spatial::PointId> witness_pool_point_ids,
      std::size_t maximum_closed_rank);
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_root(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t) = delete;

  // P8p keeps the P8o common certificate as a first line.  It descends nodes
  // whose AABB contains an actual anchor without spending witness signs, then
  // emits the first off-diagonal inconclusive subtree so a bounded singleton
  // fallback can use a different witness halo for each anchor.
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_root(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> anchor_point_ids,
      std::span<const spatial::PointId> witness_pool_point_ids,
      std::size_t maximum_closed_rank);
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_root(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t) = delete;
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_root(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t) = delete;

  // Probe one authenticated subtree with the same common-first semantics.
  // An off-diagonal inconclusive root is emitted as one frontier instead of
  // being expanded, which lets P8q partition only the bounded anchor range.
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_node(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> anchor_point_ids,
      std::span<const spatial::PointId> witness_pool_point_ids,
      std::size_t lbvh_node_index,
      std::size_t maximum_closed_rank);
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_node(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t,
      std::size_t) = delete;
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext
  start_frontier_at_node(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t,
      std::size_t) = delete;
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_root(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t) = delete;

  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_node(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> anchor_point_ids,
      std::span<const spatial::PointId> witness_pool_point_ids,
      std::size_t lbvh_node_index,
      std::size_t maximum_closed_rank);
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_node(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t,
      std::size_t) = delete;
  [[nodiscard]] static ExactGroupedAnchoredPairTraversalContext start_at_node(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::span<const spatial::PointId>,
      std::span<const spatial::PointId>,
      std::size_t,
      std::size_t) = delete;

  ExactGroupedAnchoredPairTraversalContext(
      const ExactGroupedAnchoredPairTraversalContext&) = delete;
  ExactGroupedAnchoredPairTraversalContext& operator=(
      const ExactGroupedAnchoredPairTraversalContext&) = delete;
  ExactGroupedAnchoredPairTraversalContext(
      ExactGroupedAnchoredPairTraversalContext&&) noexcept = default;
  ExactGroupedAnchoredPairTraversalContext& operator=(
      ExactGroupedAnchoredPairTraversalContext&&) = delete;
  ~ExactGroupedAnchoredPairTraversalContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return cloud_identity_ != nullptr && lbvh_identity_ != nullptr;
  }

  [[nodiscard]] bool complete() const noexcept {
    return ready() && complete_;
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const
      & noexcept {
    return {anchor_point_ids_.data(), anchor_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const & noexcept {
    return {witness_pool_point_ids_.data(), witness_pool_entry_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const && = delete;

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] std::size_t start_node_index() const noexcept {
    return start_node_index_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairTraversalAudit& audit() const
      & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairTraversalAudit& audit()
      const && = delete;

  [[nodiscard]] ExactGroupedAnchoredPairTraversalStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactGroupedAnchoredPairTraversalWorkBudget budget) &;
  [[nodiscard]] ExactGroupedAnchoredPairTraversalStep advance(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      ExactGroupedAnchoredPairTraversalWorkBudget) && = delete;

 private:
  struct NodeAuthority {
    std::size_t node_index{};
    std::size_t leaf_begin{};
    std::size_t leaf_end{};
    std::uint64_t inherited_strict_witness_mask{};
  };

  struct ActiveNode {
    NodeAuthority authority{};
    spatial::ExactDyadicAabb3 query_bounds{};
    std::size_t next_witness_offset{};
    std::size_t next_anchor_offset{};
    std::size_t node_exact_predicate_count{};
    std::size_t inherited_strict_witness_count{};
    std::uint64_t strict_witness_mask{};
  };

  struct PrivateConstructionTag {};

  ExactGroupedAnchoredPairTraversalContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> anchor_point_ids,
      std::span<const spatial::PointId> witness_pool_point_ids,
      std::size_t lbvh_node_index,
      std::size_t maximum_closed_rank,
      bool emit_off_diagonal_inconclusive_subtree,
      PrivateConstructionTag);

  [[nodiscard]] ExactGroupedAnchoredPairPruneCertificate mint_certificate(
      const ActiveNode& active_node) const;

  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_ids_{};
  std::array<spatial::ExactDyadicAabb3,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_bounds_{};
  std::size_t anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_pool_point_ids_{};
  std::array<spatial::ExactDyadicAabb3,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_point_bounds_{};
  std::size_t witness_pool_entry_count_{};
  spatial::ExactDyadicAabb3 anchor_bounds_{};
  std::size_t maximum_closed_rank_{};
  std::size_t required_witness_count_{};
  std::size_t start_node_index_{};
  std::array<NodeAuthority,
             exact_grouped_anchored_pair_maximum_traversal_stack_entry_count>
      pending_nodes_{};
  std::size_t pending_node_count_{};
  std::optional<ActiveNode> active_node_;
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;
  ExactGroupedAnchoredPairTraversalAudit audit_{};
  bool emit_off_diagonal_inconclusive_subtree_{false};
  bool complete_{false};
};

// The anchor ids and witness pool must each be strictly PointId-increasing.
// Every pool point must differ from every anchor.  The pool is only a bounded
// proposal and needs no recall guarantee or per-anchor bank membership.
//
// For the actual finite anchor set P, the certified LBVH node box Q and a pool
// witness x, the exact predicate is
//
//   max_{p in P} max_{q in Q} (x-p).(x-q) < 0.
//
// If maximum_closed_rank - 1 distinct witnesses pass, every actual pair has
// more than maximum_closed_rank closed-ball points.  No cloud-sized witness
// table, pair catalogue, cell, coface or higher-order Delaunay mosaic is
// built.  A witness costs between one and |P| physical exact signs because the
// first non-strict actual anchor rejects it.  The existing exact dyadic
// predicate may use bounded transient BigInt fallback, but this primitive owns
// no dynamic arena proportional to n.
[[nodiscard]] ExactGroupedAnchoredPairPruneCertificate
certify_exact_grouped_anchored_pair_prune(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> anchor_point_ids,
    std::span<const spatial::PointId> witness_pool_point_ids,
    std::size_t lbvh_node_index,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget);

}  // namespace morsehgp3d::hierarchy
