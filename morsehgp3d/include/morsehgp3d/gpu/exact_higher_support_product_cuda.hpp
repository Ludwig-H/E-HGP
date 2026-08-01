#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_higher_support_product_cuda_schema_version = 3U;
inline constexpr std::string_view exact_higher_support_product_cuda_backend =
    "cuda_g4_launcher_seam_plus_exact_host_fake";
inline constexpr std::string_view exact_higher_support_product_cuda_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_higher_support_product_cuda_mode =
    "batched_exact_support_prune_and_query_strict_interior";
inline constexpr std::string_view
    exact_higher_support_product_cuda_deployment_status =
        "native_cuda_bounded_qualified_scale_rejected";
inline constexpr std::string_view
    exact_higher_support_product_cuda_public_status = "not_claimed";

enum class ExactHigherSupportProductCudaTaskKind : std::uint8_t {
  support_prune = 0U,
  query_strict_interior = 1U,
};

// Product and query receipts are immutable LBVH authorities.  No support
// mass, cell, coface or incidence payload crosses this component boundary.
struct ExactHigherSupportProductCudaTask {
  std::uint64_t task_id{};
  std::uint64_t source_snapshot_epoch{};
  ExactHigherSupportProductCudaTaskKind kind{
      ExactHigherSupportProductCudaTaskKind::support_prune};
  hierarchy::ExactHigherSupportFrontierEntry product{};
  // Required only for query_strict_interior.  Its certified half-open leaf
  // range must be disjoint from every product group, exactly as in the rank
  // DFS; overlapping receipts fail closed before the launcher is called.
  std::optional<hierarchy::ExactHigherSupportNodeReceipt> query_node;

  friend bool operator==(
      const ExactHigherSupportProductCudaTask&,
      const ExactHigherSupportProductCudaTask&) = default;
};

enum class ExactHigherSupportProductCudaOutcome : std::uint8_t {
  certified = 0U,
  fail_open = 1U,
};

enum class ExactHigherSupportProductCudaBackend : std::uint8_t {
  bounded_dyadic_int1024 = 0U,
  arbitrary_precision_rational = 1U,
  bounded_dyadic_int512 = 2U,
  bounded_dyadic_int256 = 3U,
};

struct ExactHigherSupportProductCudaRecord {
  std::uint64_t task_id{};
  ExactHigherSupportProductCudaTaskKind kind{
      ExactHigherSupportProductCudaTaskKind::support_prune};
  ExactHigherSupportProductCudaOutcome outcome{
      ExactHigherSupportProductCudaOutcome::fail_open};
  ExactHigherSupportProductCudaBackend backend{
      ExactHigherSupportProductCudaBackend::arbitrary_precision_rational};
  bool cpu_fallback_performed{false};

  friend bool operator==(
      const ExactHigherSupportProductCudaRecord&,
      const ExactHigherSupportProductCudaRecord&) = default;
};

enum class ExactHigherSupportProductCudaStatus : std::uint8_t {
  complete_exact_batch = 0U,
  capacity_exhausted = 1U,
  invalid_batch = 2U,
};

enum class ExactHigherSupportProductCudaStopReason : std::uint8_t {
  none = 0U,
  task_capacity = 1U,
  invalid_task = 2U,
};

struct ExactHigherSupportProductCudaAudit {
  std::uint32_t schema_version{
      exact_higher_support_product_cuda_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_task_count{};
  std::size_t submitted_task_count{};
  std::size_t completed_task_count{};
  std::size_t support_prune_task_count{};
  std::size_t query_strict_interior_task_count{};
  std::size_t certified_count{};
  std::size_t fail_open_count{};
  std::size_t bounded_dyadic_int256_count{};
  std::size_t bounded_dyadic_int512_count{};
  std::size_t bounded_dyadic_int1024_count{};
  std::size_t arbitrary_precision_rational_fallback_count{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t kernel_elapsed_ns{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t submitted_task_digest{};
  std::uint64_t completed_result_digest{};
  int cuda_device{-1};
  bool source_authority_validated{false};
  bool resident_lease_index_identity_validated{false};
  bool kernel_elapsed_ns_available{false};
  bool every_task_validated_before_launch{false};
  bool every_result_validated_once{false};
  bool output_published_atomically{false};
  bool floating_point_decision_performed{false};
  bool bigint_support_mass_transferred{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool native_lbvh_nodes_read_on_device{false};
  bool narrow_int256_kernel_executed{false};
  bool narrow_int512_kernel_executed{false};
  bool global_product_frontier_mutated{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_cell_coface_or_incidence_arena_materialized{false};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactHigherSupportProductCudaAudit&,
      const ExactHigherSupportProductCudaAudit&) = default;
};

struct ExactHigherSupportProductCudaResult {
  ExactHigherSupportProductCudaStatus status{
      ExactHigherSupportProductCudaStatus::invalid_batch};
  ExactHigherSupportProductCudaStopReason stop_reason{
      ExactHigherSupportProductCudaStopReason::invalid_task};
  ExactHigherSupportProductCudaAudit audit{};
  std::vector<ExactHigherSupportProductCudaRecord> records;

  [[nodiscard]] bool complete() const noexcept {
    return status ==
        ExactHigherSupportProductCudaStatus::complete_exact_batch;
  }

  friend bool operator==(
      const ExactHigherSupportProductCudaResult&,
      const ExactHigherSupportProductCudaResult&) = default;
};

// Synchronous, fixed-capacity launcher seam.  The index and cloud must outlive
// the context.  Malformed launcher output poisons the context and is never
// partially published; an invalid input batch leaves the context reusable.
class ExactHigherSupportProductCudaContext final {
 public:
  ExactHigherSupportProductCudaContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      std::size_t maximum_task_count);
  ExactHigherSupportProductCudaContext(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      MortonLbvhDeviceTraversalLease&&,
      std::size_t) = delete;
  ExactHigherSupportProductCudaContext(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      MortonLbvhDeviceTraversalLease&&,
      std::size_t) = delete;
  ExactHigherSupportProductCudaContext(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      MortonLbvhDeviceTraversalLease&&,
      std::size_t) = delete;
  ExactHigherSupportProductCudaContext(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      MortonLbvhDeviceTraversalLease&&,
      std::size_t) = delete;
  ~ExactHigherSupportProductCudaContext() noexcept;

  ExactHigherSupportProductCudaContext(
      ExactHigherSupportProductCudaContext&&) noexcept;
  ExactHigherSupportProductCudaContext& operator=(
      ExactHigherSupportProductCudaContext&&) noexcept;
  ExactHigherSupportProductCudaContext(
      const ExactHigherSupportProductCudaContext&) = delete;
  ExactHigherSupportProductCudaContext& operator=(
      const ExactHigherSupportProductCudaContext&) = delete;

  [[nodiscard]] ExactHigherSupportProductCudaResult evaluate(
      std::span<const ExactHigherSupportProductCudaTask> tasks);

  [[nodiscard]] std::size_t maximum_task_count() const noexcept;
  [[nodiscard]] std::uint64_t source_snapshot_epoch() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::gpu
