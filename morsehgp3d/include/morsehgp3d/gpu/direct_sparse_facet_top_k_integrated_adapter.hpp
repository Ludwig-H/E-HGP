#pragma once

#include "morsehgp3d/gpu/direct_sparse_facet_top_k_proposal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    direct_sparse_facet_top_k_integrated_adapter_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_top_k_integrated_adapter_backend =
        "cuda_g4_plus_reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_top_k_integrated_adapter_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_top_k_integrated_adapter_mode =
        "proposal_only_then_certified";
inline constexpr std::string_view
    direct_sparse_facet_top_k_integrated_adapter_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_top_k_integrated_adapter_public_status =
        "not_claimed";

struct DirectSparseFacetTopKIntegratedAdapterAudit {
  std::size_t prepared_chunk_count{};
  std::size_t supported_query_count{};
  std::size_t snapshot_host_to_device_byte_count{};
  std::size_t active_host_to_device_query_byte_count{};
  std::size_t copied_device_to_host_byte_count{};
  std::size_t host_snapshot_byte_capacity{};
  std::size_t persistent_device_snapshot_byte_capacity{};
  std::uint64_t source_morton_lbvh_lease_epoch{};
  std::uint64_t first_buffer_epoch{};
  std::uint64_t last_buffer_epoch{};
  bool every_chunk_used_adopted_device_snapshot{true};
  bool every_chunk_retained_snapshot_owner{true};
  bool every_chunk_was_proposal_only{true};
  bool aggregate_transcript_sealed{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const DirectSparseFacetTopKIntegratedAdapterAudit&,
      const DirectSparseFacetTopKIntegratedAdapterAudit&) = default;
};

// Binds the adopted Phase-14O proposal context to the existing Phase-14L
// callback seam.  The adapter owns no scientific state and never advances a
// 14H cursor: it only converts already certified local exact centers to one
// bounded CUDA proposal call per chunk, then seals the aggregate transcript
// with the existing reference-CPU builder.
class DirectSparseFacetTopKIntegratedAdapter final {
 public:
  static constexpr std::string_view backend =
      direct_sparse_facet_top_k_integrated_adapter_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_top_k_integrated_adapter_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_top_k_integrated_adapter_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_top_k_integrated_adapter_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_top_k_integrated_adapter_public_status;

  DirectSparseFacetTopKIntegratedAdapter(
      DirectSparseFacetTopKProposalContext& proposal_context,
      const spatial::CanonicalPointCloud& cloud,
      DirectSparseFacetTopKProposalPolicy policy,
      hierarchy::ExactDirectSparseFacetTopKProposalTranscriptBudget
          per_chunk_transcript_budget);
  DirectSparseFacetTopKIntegratedAdapter(
      DirectSparseFacetTopKProposalContext&,
      const spatial::CanonicalPointCloud&&,
      DirectSparseFacetTopKProposalPolicy,
      hierarchy::ExactDirectSparseFacetTopKProposalTranscriptBudget) =
      delete;

  DirectSparseFacetTopKIntegratedAdapter(
      const DirectSparseFacetTopKIntegratedAdapter&) = delete;
  DirectSparseFacetTopKIntegratedAdapter& operator=(
      const DirectSparseFacetTopKIntegratedAdapter&) = delete;
  DirectSparseFacetTopKIntegratedAdapter(
      DirectSparseFacetTopKIntegratedAdapter&&) = delete;
  DirectSparseFacetTopKIntegratedAdapter& operator=(
      DirectSparseFacetTopKIntegratedAdapter&&) = delete;

  [[nodiscard]] std::optional<hierarchy::
      ExactDirectSparseFacetDescentBatchRunNextPreparedChunk>
  prepare_inputs(
      const hierarchy::
          ExactDirectSparseFacetDescentBatchRunNextPrepareInputs& inputs);

  [[nodiscard]] std::optional<hierarchy::
      ExactDirectSparseFacetTopKProposalTranscriptResult>
  seal_inputs(
      const hierarchy::
          ExactDirectSparseFacetDescentBatchRunNextSealInputs& inputs);

  // Both callbacks borrow this adapter.  They may only be retained or invoked
  // while the adapter, its proposal context and its lvalue cloud are alive;
  // the intended executor seam invokes them synchronously.
  [[nodiscard]] hierarchy::
      ExactDirectSparseFacetDescentBatchPrepareInputsCallback
  prepare_inputs_callback();

  [[nodiscard]] hierarchy::
      ExactDirectSparseFacetDescentBatchSealInputsCallback
  seal_inputs_callback();

  [[nodiscard]] std::size_t maximum_query_count_per_chunk() const noexcept {
    return maximum_query_count_per_chunk_;
  }
  [[nodiscard]] const DirectSparseFacetTopKIntegratedAdapterAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  DirectSparseFacetTopKProposalContext* proposal_context_{};
  const spatial::CanonicalPointCloud* cloud_{};
  DirectSparseFacetTopKProposalPolicy policy_{};
  hierarchy::ExactDirectSparseFacetTopKProposalTranscriptBudget
      per_chunk_transcript_budget_{};
  std::size_t maximum_query_count_per_chunk_{};
  DirectSparseFacetTopKIntegratedAdapterAudit audit_{};
};

}  // namespace morsehgp3d::gpu
