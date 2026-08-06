#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
class ExactHigherSupportProductCudaPositiveDecisionAdapter;
}

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t higher_support_maximum_requested_order = 10U;
inline constexpr std::size_t
    higher_support_local_rank_probe_maximum_threshold = 9U;
inline constexpr std::size_t
    higher_support_local_rank_probe_maximum_candidate_count =
        (2U * 4U + 2U) *
            higher_support_local_rank_probe_maximum_threshold +
        1U;
inline constexpr std::size_t
    higher_support_local_rank_probe_maximum_evaluation_count =
        2U * higher_support_local_rank_probe_maximum_candidate_count;
inline constexpr std::uint32_t higher_support_checkpoint_schema_version = 6U;
inline constexpr std::uint32_t higher_support_traversal_version = 5U;
inline constexpr std::string_view higher_support_stream_proof_basis =
    "exact_grouped_multiplicity_lbvh_support_partition_"
    "universal_gram_cramer_well_centered_probe_gate_"
    "bounded_local_morton_halo_antichain_cell_leaf_probe_"
    "two_sided_query_cell_rank_receipts_sparse_closed_ball_anchored_v6";

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

// One proposal-only query decision addressed by immutable Morton/LBVH
// receipts.  Batching these small records lets an accelerator evaluate the
// complete bounded local witness plan without transferring support masses or
// materializing any global pair, triangle or tetrahedron catalogue.
struct ExactHigherSupportQueryDecisionRequest {
  ExactHigherSupportFrontierEntry product{};
  ExactHigherSupportNodeReceipt query_node{};

  friend bool operator==(
      const ExactHigherSupportQueryDecisionRequest&,
      const ExactHigherSupportQueryDecisionRequest&) = default;
};

static_assert(std::is_standard_layout_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_standard_layout_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_standard_layout_v<ExactHigherSupportNodeReceipt>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeReceipt>);
static_assert(
    std::is_standard_layout_v<ExactHigherSupportQueryDecisionRequest>);
static_assert(
    std::is_trivially_copyable_v<ExactHigherSupportQueryDecisionRequest>);

// Sealed categorical source for one already-terminal support.  It is exposed
// only as the sibling of the existing positive source, so adding the
// classifier does not multiply stream/session constructor overloads.  A
// missing decision always falls back to the CPU singleton primitive.  A
// non-native producer, including the host fake, is replayed by the CPU before
// scientific use; only a sealed native exact authority may supply the
// categorical result directly.
class ExactHigherSupportTerminalGeometryDecisionSource final {
 public:
  ExactHigherSupportTerminalGeometryDecisionSource(
      const ExactHigherSupportTerminalGeometryDecisionSource&) = delete;
  ExactHigherSupportTerminalGeometryDecisionSource& operator=(
      const ExactHigherSupportTerminalGeometryDecisionSource&) = delete;
  ExactHigherSupportTerminalGeometryDecisionSource(
      ExactHigherSupportTerminalGeometryDecisionSource&&) = delete;
  ExactHigherSupportTerminalGeometryDecisionSource& operator=(
      ExactHigherSupportTerminalGeometryDecisionSource&&) = delete;

  [[nodiscard]] bool bound_to(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept {
    return bound_to_ != nullptr && bound_to_(state_, index, cloud);
  }

  [[nodiscard]] bool native_exact_authority() const noexcept {
    return native_exact_authority_ != nullptr &&
        native_exact_authority_(state_);
  }

  [[nodiscard]] std::size_t maximum_prefetch_task_count() const noexcept {
    return maximum_prefetch_task_count_ != nullptr
        ? maximum_prefetch_task_count_(state_)
        : 0U;
  }

  void prefetch(
      std::span<const ExactHigherSupportFrontierEntry> products) const
      noexcept {
    if (prefetch_ != nullptr) {
      prefetch_(state_, products);
    }
  }

  [[nodiscard]]
  std::optional<ExactHigherSupportTerminalGeometryDecision> decide(
      const ExactHigherSupportFrontierEntry& product) const noexcept {
    return decide_ != nullptr ? decide_(state_, product) : std::nullopt;
  }

 private:
  using BoundToFunction = bool (*)(
      void*,
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&) noexcept;
  using NativeExactAuthorityFunction = bool (*)(void*) noexcept;
  using MaximumPrefetchTaskCountFunction = std::size_t (*)(void*) noexcept;
  using PrefetchFunction = void (*)(
      void*, std::span<const ExactHigherSupportFrontierEntry>) noexcept;
  using DecideFunction =
      std::optional<ExactHigherSupportTerminalGeometryDecision> (*)(
          void*, const ExactHigherSupportFrontierEntry&) noexcept;

  friend class ::morsehgp3d::gpu::
      ExactHigherSupportProductCudaPositiveDecisionAdapter;

  ExactHigherSupportTerminalGeometryDecisionSource(
      void* state,
      BoundToFunction bound_to,
      NativeExactAuthorityFunction native_exact_authority,
      MaximumPrefetchTaskCountFunction maximum_prefetch_task_count,
      PrefetchFunction prefetch,
      DecideFunction decide) noexcept
      : state_(state),
        bound_to_(bound_to),
        native_exact_authority_(native_exact_authority),
        maximum_prefetch_task_count_(maximum_prefetch_task_count),
        prefetch_(prefetch),
        decide_(decide) {}

  void* state_{};
  BoundToFunction bound_to_{};
  NativeExactAuthorityFunction native_exact_authority_{};
  MaximumPrefetchTaskCountFunction maximum_prefetch_task_count_{};
  PrefetchFunction prefetch_{};
  DecideFunction decide_{};
};

// Sealed positive-only decision seam for an exact accelerator.  The stream
// remains fail-open: an absent or negative proposal is recomputed by the CPU
// decision DAG.  A non-native producer (including the host fake) can only
// propose a positive result, which the stream also replays on the CPU before
// using it.  Only the CUDA adapter can mint an instance, preventing callers
// from injecting an unauthenticated geometric decision.
class ExactHigherSupportPositiveDecisionSource final {
 public:
  ExactHigherSupportPositiveDecisionSource(
      const ExactHigherSupportPositiveDecisionSource&) = delete;
  ExactHigherSupportPositiveDecisionSource& operator=(
      const ExactHigherSupportPositiveDecisionSource&) = delete;
  ExactHigherSupportPositiveDecisionSource(
      ExactHigherSupportPositiveDecisionSource&&) = delete;
  ExactHigherSupportPositiveDecisionSource& operator=(
      ExactHigherSupportPositiveDecisionSource&&) = delete;

  [[nodiscard]] bool bound_to(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept {
    return bound_to_ != nullptr &&
        bound_to_(state_, index, cloud);
  }

  [[nodiscard]] bool native_exact_authority() const noexcept {
    return native_exact_authority_ != nullptr &&
        native_exact_authority_(state_);
  }

  [[nodiscard]] const ExactHigherSupportTerminalGeometryDecisionSource&
  terminal_geometry_source() const & noexcept {
    return *terminal_geometry_source_;
  }
  const ExactHigherSupportTerminalGeometryDecisionSource&
  terminal_geometry_source() const && = delete;

  // The scheduler only offers already-live sparse frontier entries and the
  // fixed-size local Morton witness plan.  These calls are performance hints:
  // a missing, negative or failed proposal is always recomputed by the CPU
  // decision DAG before it can affect the scientific stream.
  [[nodiscard]] std::size_t maximum_prefetch_task_count() const noexcept {
    return maximum_prefetch_task_count_ != nullptr
        ? maximum_prefetch_task_count_(state_)
        : 0U;
  }

  void prefetch_no_well_centered_supports(
      std::span<const ExactHigherSupportFrontierEntry> products) const
      noexcept {
    if (prefetch_no_well_centered_supports_ != nullptr) {
      prefetch_no_well_centered_supports_(state_, products);
    }
  }

  void prefetch_query_strictly_inside(
      std::span<const ExactHigherSupportQueryDecisionRequest> requests) const
      noexcept {
    if (prefetch_query_strictly_inside_ != nullptr) {
      prefetch_query_strictly_inside_(state_, requests);
    }
  }

  [[nodiscard]] bool certifies_no_well_centered_support(
      const ExactHigherSupportFrontierEntry& product) const {
    return certify_no_well_centered_support_ != nullptr &&
        certify_no_well_centered_support_(state_, product);
  }

  [[nodiscard]] bool certifies_query_strictly_inside(
      const ExactHigherSupportFrontierEntry& product,
      const ExactHigherSupportNodeReceipt& query_node) const {
    return certify_query_strictly_inside_ != nullptr &&
        certify_query_strictly_inside_(state_, product, query_node);
  }

 private:
  using BoundToFunction = bool (*)(
      void*,
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&) noexcept;
  using NativeExactAuthorityFunction = bool (*)(void*) noexcept;
  using MaximumPrefetchTaskCountFunction = std::size_t (*)(void*) noexcept;
  using PrefetchNoWellCenteredSupportsFunction = void (*)(
      void*, std::span<const ExactHigherSupportFrontierEntry>) noexcept;
  using PrefetchQueryStrictlyInsideFunction = void (*)(
      void*,
      std::span<const ExactHigherSupportQueryDecisionRequest>) noexcept;
  using CertifyNoWellCenteredSupportFunction = bool (*)(
      void*, const ExactHigherSupportFrontierEntry&);
  using CertifyQueryStrictlyInsideFunction = bool (*)(
      void*,
      const ExactHigherSupportFrontierEntry&,
      const ExactHigherSupportNodeReceipt&);

  friend class ::morsehgp3d::gpu::
      ExactHigherSupportProductCudaPositiveDecisionAdapter;

  ExactHigherSupportPositiveDecisionSource(
      void* state,
      BoundToFunction bound_to,
      NativeExactAuthorityFunction native_exact_authority,
      MaximumPrefetchTaskCountFunction maximum_prefetch_task_count,
      PrefetchNoWellCenteredSupportsFunction
          prefetch_no_well_centered_supports,
      PrefetchQueryStrictlyInsideFunction prefetch_query_strictly_inside,
      CertifyNoWellCenteredSupportFunction
          certify_no_well_centered_support,
      CertifyQueryStrictlyInsideFunction
          certify_query_strictly_inside,
      const ExactHigherSupportTerminalGeometryDecisionSource*
          terminal_geometry_source) noexcept
      : state_(state),
        bound_to_(bound_to),
        native_exact_authority_(native_exact_authority),
        maximum_prefetch_task_count_(maximum_prefetch_task_count),
        prefetch_no_well_centered_supports_(
            prefetch_no_well_centered_supports),
        prefetch_query_strictly_inside_(
            prefetch_query_strictly_inside),
        certify_no_well_centered_support_(
            certify_no_well_centered_support),
        certify_query_strictly_inside_(
            certify_query_strictly_inside),
        terminal_geometry_source_(terminal_geometry_source) {}

  void* state_{};
  BoundToFunction bound_to_{};
  NativeExactAuthorityFunction native_exact_authority_{};
  MaximumPrefetchTaskCountFunction maximum_prefetch_task_count_{};
  PrefetchNoWellCenteredSupportsFunction
      prefetch_no_well_centered_supports_{};
  PrefetchQueryStrictlyInsideFunction prefetch_query_strictly_inside_{};
  CertifyNoWellCenteredSupportFunction
      certify_no_well_centered_support_{};
  CertifyQueryStrictlyInsideFunction certify_query_strictly_inside_{};
  const ExactHigherSupportTerminalGeometryDecisionSource*
      terminal_geometry_source_{};
};

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
// visit has already been charged.  The product path uses a deterministic,
// bounded local Morton probe reconstructed from the product groups and this
// cursor; it never treats the absence of a local witness as exhaustive.  Each
// proposed leaf selects the highest external ancestor containing at most the
// rank threshold leaves.  The ancestor is tested once; only an inconclusive
// ancestor falls back to its originally proposed singleton leaves.
// rank_frontier stays empty: no local or global DFS is persisted.  Exact
// universal power certificates are recomputed, never persisted in the cursor.
struct ExactHigherSupportPendingProduct {
  ExactHigherSupportFrontierEntry product{};
  ExactHigherSupportPendingStage stage{
      ExactHigherSupportPendingStage::analyze_product};
  bool rank_search_started{false};
  bool leaf_analysis_started{false};
  std::vector<ExactHigherSupportNodeReceipt> rank_frontier;
  // next_candidate_index is the next bounded root.  When fallback_active is
  // true, that root is next_candidate_index-1 and the second cursor names its
  // next originally proposed singleton leaf.
  std::size_t rank_probe_next_candidate_index{};
  std::size_t rank_probe_next_fallback_leaf_index{};
  bool rank_probe_fallback_active{false};
  std::optional<std::uint64_t> rank_probe_center_seed_leaf_position;
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
  [[nodiscard]] bool bound_to_original_sources() const noexcept {
    return index_ != nullptr && cloud_ != nullptr &&
           index_identity_ != nullptr && cloud_identity_ != nullptr &&
           index_->identity_ == index_identity_ &&
           cloud_->identity_ == cloud_identity_ &&
           index_->validated_for(*cloud_);
  }

 private:
  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  std::size_t requested_maximum_order_{};
  ExactHigherSupportCheckpointManifest manifest_{};
  ExactHigherSupportAuthorityContextAudit audit_{};
  std::shared_ptr<const void> index_identity_;
  std::shared_ptr<const void> cloud_identity_;
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
  std::size_t rank_local_probe_attempt_count{};
  std::size_t rank_local_probe_center_seed_count{};
  std::size_t rank_local_probe_center_path_node_visit_count{};
  std::size_t rank_local_probe_candidate_evaluation_count{};
  std::size_t rank_local_probe_pruned_product_count{};
  std::size_t rank_local_probe_fail_open_product_count{};
  // A product that is not yet certified universally well-centred is split
  // immediately.  Skipping the positive-only rank probe cannot remove a
  // support; it only postpones that probe until tighter support cells exist.
  std::size_t rank_local_probe_geometric_gate_skip_count{};
  // Exact two-sided query-cell decisions whose nonnegative lower power bound
  // proves that no point in the complete Morton subtree is a strict interior
  // witness for any independent sphere in the active support product.
  std::size_t rank_query_outside_or_boundary_node_count{};
  std::size_t rank_query_outside_or_boundary_point_count{};
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
  std::size_t maximum_rank_local_probe_candidate_count{};
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

  // These are construction-path invariants, not a fresh scientific replay.
  // They bind the typed records, audit deltas, chain cursor and successor
  // checkpoint emitted by one ExactHigherSupportStreamBuilder execution.
  [[nodiscard]] bool builder_invariants_hold() const;

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

  // Adopts a synthesized tile-certified successor without a generator
  // rerun.  The successor must share the trusted manifest, advance the
  // chunk sequence by exactly one, keep a frontier that is a strict
  // prefix of the trusted frontier, and pass the complete local
  // checkpoint verification against this session's authority.
  [[nodiscard]] bool adopt_tile_certified_successor(
      const ExactHigherSupportCheckpoint& successor);

  // Adopts a canonical pre-expansion successor: the frontier grows, no
  // record is emitted, no mass is resolved, and the complete local
  // verification (which recomputes the frontier mass) certifies the
  // conservation.
  [[nodiscard]] bool adopt_expansion_successor(
      const ExactHigherSupportCheckpoint& successor);

 private:
  ExactHigherSupportAuthorityContext authority_;
  ExactHigherSupportCheckpoint trusted_checkpoint_{};
};

// One bounded in-memory segment produced by exactly one internally constructed
// chunk.  Prune certificates are deliberately absent: their count and their
// contribution to the authenticated output chain remain, but their potentially
// large payload is destroyed immediately after the transactional append.
struct ExactHigherSupportTerminalSegment {
  std::uint64_t chunk_sequence{};
  std::size_t first_output_record_index{};
  contract::CanonicalId source_checkpoint_digest{};
  contract::CanonicalId next_checkpoint_digest{};
  contract::CanonicalId previous_output_chain_digest{};
  contract::CanonicalId output_chain_digest{};
  ExactHigherSupportStreamStatus status{
      ExactHigherSupportStreamStatus::budget_exhausted};
  ExactHigherSupportStopReason stop_reason{
      ExactHigherSupportStopReason::work_unit_limit};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  // The authenticated chain already commits the discarded interleaving.
  // Retaining one marker per prune would recreate an unbounded global history,
  // so only its exact scalar length survives.
  std::size_t emitted_record_count_value{};
  std::size_t destroyed_prune_certificate_count{};
  std::size_t destroyed_rank_receipt_count{};
  std::size_t emitted_point_id_reference_count{};

  [[nodiscard]] std::size_t emitted_record_count() const noexcept {
    return emitted_record_count_value;
  }

  friend bool operator==(
      const ExactHigherSupportTerminalSegment&,
      const ExactHigherSupportTerminalSegment&) = default;
};

enum class ExactHigherSupportTerminalRunStatus : std::uint8_t {
  terminal,
  maximum_chunk_count_reached,
};

enum class ExactHigherSupportResidentAdvanceStatus : std::uint8_t {
  chunk_committed = 0U,
  terminal = 1U,
  maximum_chunk_count_reached = 2U,
};

// ------------------------------------------------------------------------
// 4-b2: the anchored stream assembler and its sealed certificate.  The
// anchored session is the only entity that saw and fresh-replayed every
// committed transition, so it owns the assembly: the assembler delegates
// the prepare/commit protocol to an external orchestrator (the device-tiled
// bridge), appropriates the committed records, and at terminal seals the
// canonically sorted stream result together with a privately minted
// certificate.  The facade accepts that pair without any exhaustive rerun.
// ------------------------------------------------------------------------

class ExactHigherSupportAnchoredStreamAssembler;

// Honest declaration of how every committed transition of an anchored
// chain was verified.  The fresh-replay basis is the historic M2 chain;
// the tile-certified basis accepts device-searched tiles whose record
// payloads are host-classified exactly and whose completeness is bound by
// the BigInt universe closure -- no per-transition generator rerun.
enum class ExactHigherSupportVerificationBasis : std::uint8_t {
  fresh_cpu_replay_every_commit,
  device_search_host_exact_record_classification_bigint_closure,
};

// One tile-certified transition: the resolved back suffix of the trusted
// frontier, its host-classified records and its device-certified masses.
// Events and diagnostics carry exact host-recomputed payloads; their
// stream indexes are assigned by the assembler at commit.
struct ExactHigherSupportTileCertifiedTile {
  std::size_t consumed_root_count{};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics;
  std::vector<ExactHigherSupportPruneCertificate> prune_certificates;
  exact::BigInt well_centering_pruned_support_mass{0};
  exact::BigInt rank_pruned_support_mass{0};
  exact::BigInt leaf_classified_support_mass{0};
  std::size_t affinely_dependent_leaf_count{};
  std::size_t boundary_reduced_leaf_count{};
  std::size_t exterior_circumcenter_leaf_count{};
  std::size_t above_rank_leaf_count{};
  std::size_t closed_ball_query_count{};
  std::size_t point_classification_count{};
};

// Combinatorial support mass of one frontier entry (the same definition
// the checkpoint verifier and the M2 bridge induction identity use).
[[nodiscard]] exact::BigInt
exact_higher_support_frontier_entry_support_count(
    const ExactHigherSupportFrontierEntry& entry);

class ExactHigherSupportAnchoredStreamCertificate {
 public:
  ExactHigherSupportAnchoredStreamCertificate() noexcept = default;

  [[nodiscard]] bool minted() const noexcept { return minted_; }
  [[nodiscard]] const ExactHigherSupportCheckpointManifest& manifest()
      const noexcept {
    return manifest_;
  }
  [[nodiscard]] const contract::CanonicalId& terminal_checkpoint_digest()
      const noexcept {
    return terminal_checkpoint_digest_;
  }
  [[nodiscard]] std::size_t committed_chunk_count() const noexcept {
    return committed_chunk_count_;
  }
  [[nodiscard]] ExactHigherSupportVerificationBasis verification_basis()
      const noexcept {
    return verification_basis_;
  }
  // Strict content re-digest of the presented result through the one
  // record-chain definition of this translation unit, plus count, audit,
  // requirement and completion equality.  A forged or reordered record,
  // an inflated audit or a foreign result fails closed.
  [[nodiscard]] bool certifies(const ExactHigherSupportStreamResult&) const;
  [[nodiscard]] bool certified_for_authority(
      const ExactHigherSupportCheckpointManifest&) const noexcept;

 private:
  friend class ExactHigherSupportAnchoredStreamAssembler;

  ExactHigherSupportCheckpointManifest manifest_{};
  contract::CanonicalId terminal_checkpoint_digest_{};
  contract::CanonicalId canonical_content_digest_{};
  ExactHigherSupportStreamAudit terminal_audit_{};
  std::size_t committed_chunk_count_{};
  std::size_t event_count_{};
  std::size_t diagnostic_count_{};
  ExactHigherSupportVerificationBasis verification_basis_{
      ExactHigherSupportVerificationBasis::fresh_cpu_replay_every_commit};
  bool minted_{false};
};

struct ExactHigherSupportAnchoredStreamSeal {
  std::optional<ExactHigherSupportStreamResult> result;
  ExactHigherSupportAnchoredStreamCertificate certificate{};

  [[nodiscard]] bool certified_sealed() const noexcept {
    return result.has_value() && certificate.minted();
  }
};

class ExactHigherSupportAnchoredStreamAssembler {
 public:
  explicit ExactHigherSupportAnchoredStreamAssembler(
      const ExactHigherSupportAuthorityContext& authority);
  ExactHigherSupportAnchoredStreamAssembler(
      const ExactHigherSupportAnchoredStreamAssembler&) = delete;
  ExactHigherSupportAnchoredStreamAssembler& operator=(
      const ExactHigherSupportAnchoredStreamAssembler&) = delete;
  ExactHigherSupportAnchoredStreamAssembler(
      ExactHigherSupportAnchoredStreamAssembler&&) = delete;
  ExactHigherSupportAnchoredStreamAssembler& operator=(
      ExactHigherSupportAnchoredStreamAssembler&&) = delete;

  [[nodiscard]] const ExactHigherSupportCheckpoint& trusted_checkpoint()
      const noexcept;
  [[nodiscard]] ExactHigherSupportStreamChunk prepare_next(
      const ExactHigherSupportStreamBudget& chunk_budget,
      const ExactHigherSupportCheckpoint& reinjected_source) const;
  // Delegates to the anchored session; on a verified transition the
  // candidate's events and diagnostics are appropriated into the assembly.
  [[nodiscard]] ExactHigherSupportStreamChunkVerification commit_prepared(
      const ExactHigherSupportStreamBudget& chunk_budget,
      const ExactHigherSupportCheckpoint& reinjected_source,
      const ExactHigherSupportStreamChunk& candidate);
  // Tile-certified commit (reduced verification): the tile's device-bound
  // masses must exactly cover the consumed back roots, the leaf category
  // and record identities must close, and the synthesized successor
  // checkpoint must pass the complete local-integrity verification before
  // adoption.  The first commit locks the chain's verification basis; a
  // chain never mixes bases.
  [[nodiscard]] ExactHigherSupportStreamChunkVerification
  commit_tile_certified(
      const ExactHigherSupportCheckpoint& reinjected_source,
      ExactHigherSupportTileCertifiedTile&& tile);
  // Canonical pre-expansion under the tile-certified basis: no record, no
  // resolved mass, the successor frontier is the caller's canonical split.
  [[nodiscard]] bool commit_canonical_expansion(
      const ExactHigherSupportCheckpoint& reinjected_source,
      const ExactHigherSupportCheckpoint& successor);
  // At the locally-complete terminal, sorts the appropriated records under
  // the public canonical order, assembles the stream result and mints the
  // sealed certificate exactly once.
  [[nodiscard]] ExactHigherSupportAnchoredStreamSeal seal_terminal_stream(
      const ExactHigherSupportStreamBudget& declared_budget);

 private:
  ExactHigherSupportAuthorityContext authority_;
  ExactHigherSupportAnchoredSession session_;
  std::vector<ExactHigherSupportEvent> events_;
  std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics_;
  std::size_t committed_chunk_count_{};
  std::optional<ExactHigherSupportVerificationBasis> locked_basis_;
  bool sealed_{false};
};

class ExactHigherSupportTerminalSession;
class ExactSparseHigherSupportH0ChunkRunContext;
struct ExactSparseDirectH0CandidateRun;
struct ExactSparseDirectH0CandidateRunLimits;

enum class ExactHigherSupportUnsealedDrainStatus : std::uint8_t {
  segment_ready,
  terminal,
  maximum_chunk_count_reached,
};

// Move-only process-local result of one producer advance.  A ready result
// binds the retained segment to the exact manifest copied from the same
// canonical-root session.  Callers may inspect that pair, but only the common
// H0 projection is allowed to consume it; a raw segment and an unrelated
// manifest therefore cannot be combined at the consuming boundary.
class ExactHigherSupportUnsealedDrain {
 public:
  ExactHigherSupportUnsealedDrain(
      const ExactHigherSupportUnsealedDrain&) = delete;
  ExactHigherSupportUnsealedDrain& operator=(
      const ExactHigherSupportUnsealedDrain&) = delete;
  ExactHigherSupportUnsealedDrain(
      ExactHigherSupportUnsealedDrain&& other) noexcept;
  ExactHigherSupportUnsealedDrain& operator=(
      ExactHigherSupportUnsealedDrain&& other) = delete;

  [[nodiscard]] ExactHigherSupportUnsealedDrainStatus status()
      const noexcept {
    return status_;
  }
  [[nodiscard]] bool segment_available() const noexcept {
    return status_ ==
               ExactHigherSupportUnsealedDrainStatus::segment_ready &&
        manifest_.has_value() && segment_.has_value();
  }
  [[nodiscard]] bool consumed() const noexcept {
    return successfully_consumed_;
  }
  [[nodiscard]] bool moved_from() const noexcept {
    return moved_from_;
  }
  [[nodiscard]] const ExactHigherSupportCheckpointManifest* manifest()
      const noexcept {
    return manifest_ ? &*manifest_ : nullptr;
  }
  [[nodiscard]] const ExactHigherSupportTerminalSegment* segment()
      const noexcept {
    return segment_ ? &*segment_ : nullptr;
  }

 private:
  friend class ExactHigherSupportTerminalSession;
  friend class ExactSparseHigherSupportH0ChunkRunContext;
  friend ExactSparseDirectH0CandidateRun
  project_exact_sparse_direct_h0_higher_candidate_run(
      ExactHigherSupportUnsealedDrain&& source,
      ExactSparseDirectH0CandidateRunLimits limits);

  explicit ExactHigherSupportUnsealedDrain(
      ExactHigherSupportUnsealedDrainStatus status) noexcept;
  ExactHigherSupportUnsealedDrain(
      ExactHigherSupportTerminalSegment&& segment,
      ExactHigherSupportCheckpointManifest&& manifest,
      std::shared_ptr<bool> outstanding_segment) noexcept;
  void consume() noexcept;
  void invalidate_moved_from() noexcept;

  ExactHigherSupportUnsealedDrainStatus status_{
      ExactHigherSupportUnsealedDrainStatus::terminal};
  std::optional<ExactHigherSupportCheckpointManifest> manifest_;
  std::optional<ExactHigherSupportTerminalSegment> segment_;
  std::shared_ptr<bool> outstanding_segment_;
  bool successfully_consumed_{false};
  bool moved_from_{false};
};

// Move-only, process-local authority over a terminal stream constructed from
// canonical roots.  It authenticates no durable recovery and performs no
// fresh replay.  The referenced cloud and LBVH must outlive this authority.
class ExactHigherSupportTerminalAuthority {
 public:
  ExactHigherSupportTerminalAuthority(
      const ExactHigherSupportTerminalAuthority&) = delete;
  ExactHigherSupportTerminalAuthority& operator=(
      const ExactHigherSupportTerminalAuthority&) = delete;
  ExactHigherSupportTerminalAuthority(
      ExactHigherSupportTerminalAuthority&& other) noexcept;
  ExactHigherSupportTerminalAuthority& operator=(
      ExactHigherSupportTerminalAuthority&& other) noexcept;

  // The source accessors require an unconsumed authority.  A moved-from or
  // released authority is intentionally revoked and must only be queried
  // through its boolean scope predicates.
  [[nodiscard]] const spatial::MortonLbvhIndex& index() const noexcept {
    return *index_;
  }
  [[nodiscard]] const spatial::CanonicalPointCloud& cloud() const noexcept {
    return *cloud_;
  }
  [[nodiscard]] std::size_t requested_maximum_order() const noexcept {
    return terminal_checkpoint_.manifest.requested_maximum_order;
  }
  [[nodiscard]] const ExactHigherSupportCheckpointManifest& manifest()
      const noexcept {
    return terminal_checkpoint_.manifest;
  }
  [[nodiscard]] const contract::CanonicalId& canonical_cloud_digest()
      const noexcept {
    return terminal_checkpoint_.manifest.canonical_cloud_digest;
  }
  [[nodiscard]] const contract::CanonicalId& lbvh_digest() const noexcept {
    return terminal_checkpoint_.manifest.lbvh_digest;
  }
  [[nodiscard]] const ExactHigherSupportStreamBudget& chunk_budget()
      const noexcept {
    return chunk_budget_;
  }
  [[nodiscard]] std::size_t maximum_chunk_count() const noexcept {
    return maximum_chunk_count_;
  }
  [[nodiscard]] std::size_t chunk_count() const noexcept {
    return chunk_count_;
  }
  [[nodiscard]] std::size_t event_count() const noexcept {
    return event_count_;
  }
  [[nodiscard]] std::size_t relevant_extra_shell_diagnostic_count()
      const noexcept {
    return relevant_extra_shell_diagnostic_count_;
  }
  [[nodiscard]] std::size_t destroyed_prune_certificate_count()
      const noexcept {
    return destroyed_prune_certificate_count_;
  }
  [[nodiscard]] std::size_t destroyed_rank_receipt_count() const noexcept {
    return destroyed_rank_receipt_count_;
  }
  [[nodiscard]] const ExactHigherSupportStreamAudit& terminal_audit()
      const noexcept {
    return terminal_checkpoint_.cumulative_audit;
  }
  [[nodiscard]] const ExactHigherSupportCheckpoint& terminal_checkpoint()
      const noexcept {
    return terminal_checkpoint_;
  }
  [[nodiscard]] const contract::CanonicalId& output_chain_digest()
      const noexcept {
    return terminal_checkpoint_.output_chain_digest;
  }
  [[nodiscard]] const contract::CanonicalId& checkpoint_digest()
      const noexcept {
    return terminal_checkpoint_.checkpoint_digest;
  }
  [[nodiscard]] std::size_t output_record_count() const noexcept {
    return terminal_checkpoint_.output_record_count;
  }
  [[nodiscard]] std::span<const ExactHigherSupportTerminalSegment> segments()
      const noexcept {
    return segments_;
  }

  [[nodiscard]] bool bound_to(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order) const noexcept {
    return index_ == &index && cloud_ == &cloud &&
           index_identity_ != nullptr && cloud_identity_ != nullptr &&
           index.identity_ == index_identity_ &&
           cloud.identity_ == cloud_identity_ &&
           index.validated_for(cloud) &&
           terminal_checkpoint_.manifest.requested_maximum_order ==
               requested_maximum_order;
  }
  [[nodiscard]] bool terminal_checkpoint_local_integrity_verified()
      const noexcept {
    return terminal_checkpoint_local_integrity_verified_;
  }
  [[nodiscard]] bool canonical_root_anchored_internal_production()
      const noexcept {
    return canonical_root_anchored_internal_production_;
  }
  [[nodiscard]] bool
  committed_chunks_captured_once_without_fresh_replay() const noexcept {
    return committed_chunks_captured_once_without_fresh_replay_;
  }
  [[nodiscard]] bool no_forbidden_global_structure_materialized()
      const noexcept {
    return no_forbidden_global_structure_materialized_;
  }
  [[nodiscard]] bool fresh_replay_performed() const noexcept {
    return false;
  }
  [[nodiscard]] bool durable_authority_claimed() const noexcept {
    return false;
  }
  [[nodiscard]] bool hierarchy_reduction_performed() const noexcept {
    return false;
  }
  [[nodiscard]] bool public_status_claimed() const noexcept {
    return false;
  }
  [[nodiscard]] bool sealed_in_process_terminal_authority() const noexcept;

  [[nodiscard]] std::vector<ExactHigherSupportTerminalSegment>
  release_segments() && noexcept {
    std::vector<ExactHigherSupportTerminalSegment> released =
        std::move(segments_);
    invalidate();
    return released;
  }
  std::vector<ExactHigherSupportTerminalSegment> release_segments() & =
      delete;
  std::vector<ExactHigherSupportTerminalSegment> release_segments()
      const& = delete;

 private:
  friend class ExactHigherSupportTerminalSession;

  ExactHigherSupportTerminalAuthority() noexcept = default;
  ExactHigherSupportTerminalAuthority(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactHigherSupportStreamBudget chunk_budget,
      std::size_t maximum_chunk_count,
      std::size_t event_count,
      std::size_t relevant_extra_shell_diagnostic_count,
      std::size_t destroyed_prune_certificate_count,
      std::size_t destroyed_rank_receipt_count,
      ExactHigherSupportCheckpoint&& terminal_checkpoint,
      std::vector<ExactHigherSupportTerminalSegment>&& segments) noexcept;
  void invalidate() noexcept;

  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  ExactHigherSupportStreamBudget chunk_budget_{};
  std::size_t maximum_chunk_count_{};
  std::size_t chunk_count_{};
  std::size_t event_count_{};
  std::size_t relevant_extra_shell_diagnostic_count_{};
  std::size_t destroyed_prune_certificate_count_{};
  std::size_t destroyed_rank_receipt_count_{};
  std::shared_ptr<const void> index_identity_;
  std::shared_ptr<const void> cloud_identity_;
  ExactHigherSupportCheckpoint terminal_checkpoint_{};
  std::vector<ExactHigherSupportTerminalSegment> segments_;
  bool terminal_checkpoint_local_integrity_verified_{false};
  bool canonical_root_anchored_internal_production_{false};
  bool committed_chunks_captured_once_without_fresh_replay_{false};
  bool no_forbidden_global_structure_materialized_{false};
  bool consumed_{true};
};

// Internal producer: it starts from canonical roots, owns the only live
// checkpoint, never accepts a checkpoint or chunk from its caller, and uses
// one fixed budget for every exact chunk execution.
class ExactHigherSupportTerminalSession {
 public:
  // Reuse an already hashed immutable authority when repeated fresh replay
  // sessions must start from the same canonical roots.  The session copies
  // the compact authority context; the referenced cloud and LBVH must still
  // outlive it.
  ExactHigherSupportTerminalSession(
      const ExactHigherSupportAuthorityContext& authority,
      ExactHigherSupportStreamBudget chunk_budget,
      std::size_t maximum_chunk_count);
  ExactHigherSupportTerminalSession(
      const ExactHigherSupportAuthorityContext& authority,
      const ExactHigherSupportPositiveDecisionSource&
          positive_decision_source,
      ExactHigherSupportStreamBudget chunk_budget,
      std::size_t maximum_chunk_count);
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactHigherSupportStreamBudget chunk_budget,
      std::size_t maximum_chunk_count);
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      const ExactHigherSupportPositiveDecisionSource&
          positive_decision_source,
      ExactHigherSupportStreamBudget chunk_budget,
      std::size_t maximum_chunk_count);
  ExactHigherSupportTerminalSession(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      std::size_t,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;
  ExactHigherSupportTerminalSession(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      const ExactHigherSupportPositiveDecisionSource&,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      const ExactHigherSupportPositiveDecisionSource&,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      std::size_t,
      const ExactHigherSupportPositiveDecisionSource&,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;
  ExactHigherSupportTerminalSession(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      std::size_t,
      const ExactHigherSupportPositiveDecisionSource&,
      ExactHigherSupportStreamBudget,
      std::size_t) = delete;

  ExactHigherSupportTerminalSession(
      const ExactHigherSupportTerminalSession&) = delete;
  ExactHigherSupportTerminalSession& operator=(
      const ExactHigherSupportTerminalSession&) = delete;
  ExactHigherSupportTerminalSession(
      ExactHigherSupportTerminalSession&&) = delete;
  ExactHigherSupportTerminalSession& operator=(
      ExactHigherSupportTerminalSession&&) = delete;

  [[nodiscard]] ExactHigherSupportTerminalRunStatus run_to_terminal();
  // Executes at most one scientific chunk while retaining the resident
  // segment prefix.  This cooperative boundary lets diagnostic callers
  // observe an operational deadline without replaying scientific work.
  [[nodiscard]] ExactHigherSupportResidentAdvanceStatus
  advance_one_resident_chunk() &;
  ExactHigherSupportResidentAdvanceStatus
  advance_one_resident_chunk() && = delete;
  // Executes at most one scientific chunk and returns an opaque, provenance-
  // bound retained segment, or reports terminality/chunk-cap without changing
  // the scientific state.  No terminal or durable authority is minted.  The
  // first ready segment permanently revokes the resident seal path.
  [[nodiscard]] ExactHigherSupportUnsealedDrain
  drain_next_unsealed_segment() &;
  ExactHigherSupportUnsealedDrain drain_next_unsealed_segment() && = delete;
  [[nodiscard]] ExactHigherSupportTerminalAuthority seal() &&;
  ExactHigherSupportTerminalAuthority seal() & = delete;

  [[nodiscard]] const ExactHigherSupportCheckpoint& trusted_checkpoint()
      const noexcept {
    return trusted_checkpoint_;
  }
  [[nodiscard]] const ExactHigherSupportStreamBudget& chunk_budget()
      const noexcept {
    return chunk_budget_;
  }
  [[nodiscard]] std::size_t maximum_chunk_count() const noexcept {
    return maximum_chunk_count_;
  }
  [[nodiscard]] std::size_t chunk_count() const noexcept {
    return chunk_count_;
  }
  [[nodiscard]] std::size_t released_segment_count() const noexcept {
    return released_segment_count_;
  }
  [[nodiscard]] std::size_t resident_segment_count() const noexcept {
    return segments_.size();
  }
  [[nodiscard]] bool unsealed_segment_drain_performed() const noexcept {
    return unsealed_segment_drain_performed_;
  }
  [[nodiscard]] bool unconsumed_segment_outstanding() const noexcept {
    return unsealed_segment_outstanding_ != nullptr &&
        *unsealed_segment_outstanding_;
  }

 private:
  void append_next_internal_chunk();
  [[nodiscard]] bool retained_segment_accounting_holds() const noexcept;

  ExactHigherSupportAuthorityContext authority_;
  ExactHigherSupportStreamBudget chunk_budget_{};
  std::size_t maximum_chunk_count_{};
  ExactHigherSupportCheckpoint trusted_checkpoint_{};
  std::vector<ExactHigherSupportTerminalSegment> segments_;
  std::size_t chunk_count_{};
  std::size_t event_count_{};
  std::size_t relevant_extra_shell_diagnostic_count_{};
  std::size_t destroyed_prune_certificate_count_{};
  std::size_t destroyed_rank_receipt_count_{};
  std::size_t released_segment_count_{};
  std::size_t released_event_count_{};
  std::size_t released_relevant_extra_shell_diagnostic_count_{};
  std::size_t released_destroyed_prune_certificate_count_{};
  std::size_t released_destroyed_rank_receipt_count_{};
  std::size_t released_output_record_count_{};
  std::size_t released_point_id_reference_count_{};
  contract::CanonicalId released_output_chain_digest_{};
  contract::CanonicalId released_checkpoint_digest_{};
  std::shared_ptr<bool> unsealed_segment_outstanding_;
  // Non-owning by design: the sealed CUDA adapter is non-movable and must
  // outlive the resident terminal session that consumes it.
  const ExactHigherSupportPositiveDecisionSource*
      positive_decision_source_{};
  bool unsealed_segment_drain_performed_{false};
  bool sealed_{false};
};

// Public canonical record ordering of the exhaustive oracle.  The stream
// builder sorts its result through these exact comparators; the device-tiled
// tower assembly reuses them so result-order equality never depends on a
// parallel definition.
void canonical_sort_exact_higher_support_events(
    std::vector<ExactHigherSupportEvent>& events);
void canonical_sort_exact_higher_support_extra_shell_diagnostics(
    std::vector<ExactHigherSupportExtraShellDiagnostic>& diagnostics);

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

[[nodiscard]] ExactHigherSupportStreamResult
build_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget,
    const ExactHigherSupportPositiveDecisionSource&
        positive_decision_source);

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
