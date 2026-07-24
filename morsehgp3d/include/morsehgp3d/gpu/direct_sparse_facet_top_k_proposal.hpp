#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_top_k_proposal_transcript.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace morsehgp3d::gpu {

namespace detail {
class Phase14FacetTopKProposalContextState;
class Phase14FacetTopKProposalHostState;
}  // namespace detail

inline constexpr std::uint32_t
    direct_sparse_facet_top_k_gpu_proposal_schema_version = 3U;
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_backend = "cuda_g4";
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_mode = "proposal_only";
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_top_k_gpu_proposal_proof_basis =
        "bounded_two_sided_morton_windows_per_source_facet_vertex_"
        "fixed_k_candidate_output_host_domain_exclusion_window_epoch_"
        "active_prefix_candidate_tail_sentinel_and_digest_validation_then_"
        "direct_integer_binary64_center_projection_reference_cpu_exact_"
        "point_of_use_recertification_v3";

// The center is an operational search hint only.  It normally comes from the
// already certified local facet miniball.  Even a mismatched center cannot
// publish a scientific decision: the 14F/14H consumer recomputes every
// distance at the exact center and completes the exact LBVH traversal.
struct DirectSparseFacetTopKProposalQuery {
  hierarchy::ExactDirectSparseFacetKey source_facet_key{};
  exact::ExactCenter3 query_center{};

  friend bool operator==(
      const DirectSparseFacetTopKProposalQuery&,
      const DirectSparseFacetTopKProposalQuery&) = default;
};

struct DirectSparseFacetTopKProposalPolicy {
  std::size_t morton_window_radius{};

  friend bool operator==(
      const DirectSparseFacetTopKProposalPolicy&,
      const DirectSparseFacetTopKProposalPolicy&) = default;
};

struct DirectSparseFacetTopKProposalAudit {
  static constexpr std::string_view proposal_semantics =
      "cuda_bounded_morton_window_floating_top_k_point_ids_only";
  static constexpr std::string_view decision_semantics =
      "reference_cpu_exact_facet_union_top_k_and_full_lbvh_closure";

  // These capacities describe the immutable snapshot that is allocated
  // lazily by the first supported GPU batch.  They do not claim that an
  // empty or all-unsupported call materialized CUDA storage.
  std::size_t snapshot_point_count{};
  std::size_t static_device_coordinate_word_capacity{};
  std::size_t static_device_morton_point_id_capacity{};
  std::size_t static_device_snapshot_byte_capacity{};
  std::size_t host_inverse_morton_entry_count{};
  std::size_t host_snapshot_byte_capacity{};
  std::size_t maximum_query_count{};
  std::size_t physical_device_record_capacity{};
  std::size_t physical_device_query_capacity{};
  std::size_t static_device_record_buffer_byte_capacity{};
  std::size_t static_device_query_buffer_byte_capacity{};
  std::size_t host_record_copy_byte_capacity{};
  // Per-call traffic is proportional to the supported query count D.  Only
  // active output records are initialized and copied.  Inactive physical
  // storage is never consumed, and every later active prefix is initialized
  // before launch.  The unused candidate tail of every active record remains
  // an all-ones sentinel and is authenticated on the host.
  std::size_t active_host_to_device_query_record_count{};
  std::size_t active_host_to_device_query_byte_count{};
  std::size_t initialized_device_output_record_count{};
  std::size_t initialized_device_output_byte_count{};
  std::size_t copied_device_to_host_record_count{};
  std::size_t copied_device_to_host_byte_count{};
  std::size_t exact_center_projection_axis_count{};
  std::size_t exact_center_projection_integer_division_count{};
  std::size_t canonical_query_count{};
  std::size_t gpu_supported_center_query_count{};
  std::size_t unsupported_center_query_count{};
  std::size_t gpu_output_record_count{};
  std::size_t morton_window_radius{};
  std::size_t maximum_inspection_count_per_query{};
  std::size_t aggregate_inspection_count_upper_bound{};
  std::size_t inspected_neighbor_count{};
  std::size_t floating_distance_evaluation_count{};
  std::size_t floating_distance_rejection_count{};
  std::size_t proposed_candidate_count{};
  std::size_t nonempty_proposal_record_count{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  std::uint64_t buffer_epoch{};
  // Deterministic operational fingerprint.  It is not collision-resistant
  // authority and is deliberately ignored by exact 14F/14H decisions.
  std::uint64_t proposal_digest_fnv1a{};
  bool matching_immutable_point_namespace_validated{false};
  bool canonical_query_order_validated{false};
  bool homogeneous_facet_cardinality_validated{false};
  bool fixed_capacity_preflight_satisfied{false};
  bool exact_center_projection_division_bound_validated{false};
  bool supported_query_permutation_validated{false};
  bool active_record_candidate_tail_sentinel_validated{false};
  bool every_candidate_domain_validated{false};
  bool every_candidate_source_facet_exclusion_validated{false};
  bool every_candidate_morton_window_validated{false};
  bool candidate_distinctness_validated{false};
  bool bounded_work_validated{false};
  bool transcript_builder_invoked{false};
  bool gpu_execution_performed{false};
  bool floating_ordering_only{true};
  bool exact_distance_or_partition_published{false};
  bool scientific_decision_published{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const DirectSparseFacetTopKProposalAudit&,
      const DirectSparseFacetTopKProposalAudit&) = default;
};

struct DirectSparseFacetTopKProposalBatchResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_top_k_gpu_proposal_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_top_k_gpu_proposal_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_top_k_gpu_proposal_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_top_k_gpu_proposal_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_top_k_gpu_proposal_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_top_k_gpu_proposal_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_top_k_gpu_proposal_schema_version};
  hierarchy::ExactDirectSparseFacetTopKProposalTranscriptResult transcript;
  DirectSparseFacetTopKProposalAudit audit;

  [[nodiscard]] bool complete_proposal_batch() const noexcept;
  [[nodiscard]] bool certified_atomic_transcript_rejection() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const DirectSparseFacetTopKProposalBatchResult&,
      const DirectSparseFacetTopKProposalBatchResult&) = default;
};

// Phase 14I owns one immutable coordinate/Morton-order snapshot.  A batch
// contains at most maximum_query_count complete facet keys and never allocates
// a D-by-n table, a cover, a coface arena, Gamma, a cell complex or a
// higher-order Delaunay mosaic.  Calls are synchronous and serialized by the
// context.  Any malformed device transcript poisons the context.
class DirectSparseFacetTopKProposalContext final {
 public:
  DirectSparseFacetTopKProposalContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_query_count);
  ~DirectSparseFacetTopKProposalContext() noexcept;

  DirectSparseFacetTopKProposalContext(
      DirectSparseFacetTopKProposalContext&&) noexcept;
  DirectSparseFacetTopKProposalContext& operator=(
      DirectSparseFacetTopKProposalContext&&) noexcept;

  DirectSparseFacetTopKProposalContext(
      const DirectSparseFacetTopKProposalContext&) = delete;
  DirectSparseFacetTopKProposalContext& operator=(
      const DirectSparseFacetTopKProposalContext&) = delete;

  // Empty batches and batches whose centers all exceed binary64 range produce
  // an empty 14F transcript without a CUDA launch.  Otherwise the supported
  // subset is launched once.  Only nonempty proposal rows are retained.
  [[nodiscard]] DirectSparseFacetTopKProposalBatchResult build(
      const spatial::CanonicalPointCloud& cloud,
      const hierarchy::
          ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata,
      std::span<const DirectSparseFacetTopKProposalQuery> canonical_queries,
      DirectSparseFacetTopKProposalPolicy policy,
      const hierarchy::
          ExactDirectSparseFacetTopKProposalTranscriptBudget& transcript_budget);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t maximum_query_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  std::shared_ptr<detail::Phase14FacetTopKProposalContextState> state_;
  std::unique_ptr<detail::Phase14FacetTopKProposalHostState> host_;
  std::size_t maximum_query_count_{};
  std::uint64_t last_buffer_epoch_{};
};

}  // namespace morsehgp3d::gpu
