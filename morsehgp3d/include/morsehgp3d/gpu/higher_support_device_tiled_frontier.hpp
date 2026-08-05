#pragma once

#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::gpu {

namespace detail {
class Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess;
}

inline constexpr std::uint32_t
    higher_support_device_tiled_frontier_schema_version = 1U;
// Slot tiles are rooted on canonical host frontier entries.  Every capacity
// below is a schema constant, never data: the device arena extent is a pure
// function of the slot tile capacity.
inline constexpr std::size_t
    higher_support_device_tiled_frontier_maximum_slot_tile_capacity = 1024U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_products_per_slot = 1536U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_prune_records_per_slot = 256U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_terminal_records_per_slot = 256U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_probe_receipts_per_slot =
        hierarchy::higher_support_local_rank_probe_maximum_threshold;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_pending_decisions_per_slot = 64U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_maximum_closed_rank = 11U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_default_gate_evaluations_per_slot =
        2048U;
inline constexpr std::size_t
    higher_support_device_tiled_frontier_default_maximum_subdivision_count =
        4096U;

// The unsigned 128-bit slot-mass accounting is proved exact only for
// point_count <= 2^31: the progressive binomial scheme
// C(x,2) -> C(x,3) -> C(x,4) keeps every intermediate strictly below 2^128
// and every division exact.  A larger cloud must be rejected fail-closed.
inline constexpr std::uint64_t
    higher_support_device_tiled_frontier_maximum_point_count =
        UINT64_C(1) << 31U;
// Morton radix splits consume at least one distinct leading bit per level
// (63-bit keys), and equal-key collision runs split by median, so the LBVH
// depth is bounded by 63 + ceil(log2(point_count)) <= 63 + 31 = 94 for every
// admissible cloud.  The certified observed depth of the adopted index must
// also satisfy this bound.
inline constexpr std::size_t
    higher_support_device_tiled_frontier_maximum_certified_lbvh_depth = 94U;
// Each expansion replaces one product by at most five children, pushing at
// most four additional records, and one product chain is bounded by the sum
// of the remaining split depths of its at most four groups.  The resident
// per-slot stack therefore never exceeds 16*depth + 1 records.
[[nodiscard]] constexpr std::size_t
higher_support_device_tiled_frontier_proved_stack_bound(
    std::size_t certified_maximum_lbvh_depth) noexcept {
  return 16U * certified_maximum_lbvh_depth + 1U;
}
static_assert(
    higher_support_device_tiled_frontier_proved_stack_bound(
        higher_support_device_tiled_frontier_maximum_certified_lbvh_depth) <=
    higher_support_device_tiled_frontier_products_per_slot);

inline constexpr std::string_view
    higher_support_device_tiled_frontier_backend =
        "exact_host_fake_contract_native_cuda_pending";
inline constexpr std::string_view
    higher_support_device_tiled_frontier_profile = "hgp_reduced";
inline constexpr std::string_view higher_support_device_tiled_frontier_mode =
    "device_resident_budgeted_higher_support_slot_tiles";
inline constexpr std::string_view
    higher_support_device_tiled_frontier_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    higher_support_device_tiled_frontier_public_status = "not_claimed";
inline constexpr std::string_view
    higher_support_device_tiled_frontier_proof_basis =
        "exact_grouped_multiplicity_slot_rooted_dfs_expansion_partition_"
        "universal_gram_cramer_gate_positive_gate_then_bounded_local_"
        "morton_first_group_leaf_begin_substitute_seed_antichain_probe_"
        "plan_order_receipt_commit_unsigned128_slot_mass_host_bigint_"
        "revalidated_two_sealed_terminal_policies_v1";

// Two watertight terminal policies.  A receipt minted under one policy never
// prunes under the other.  The strict-interior carrier is defined for
// support size three only and never authorizes dropping a region from a
// Gamma_2 source: silent incidences must still be handed off.
enum class HigherSupportDeviceTiledFrontierTerminalPolicy : std::uint8_t {
  closed_rank_window = 0U,
  strict_interior_carrier_q3 = 1U,
};

[[nodiscard]] constexpr bool
higher_support_device_tiled_frontier_terminal_policy_known(
    HigherSupportDeviceTiledFrontierTerminalPolicy policy) noexcept {
  switch (policy) {
    case HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window:
    case HigherSupportDeviceTiledFrontierTerminalPolicy::
        strict_interior_carrier_q3:
      return true;
  }
  return false;
}

// Required certified strict-interior witness mass for one product of the
// given support size.  Under closed_rank_window this is the rank-window
// threshold t_m = maximum_relevant_closed_rank - support_size + 1, clamped
// to zero for supports already above the window.  Under the carrier policy a
// single strict witness rejects, and only support size three is admissible;
// a support-four query under the carrier policy is a caller error surfaced
// at tile-open time, not silently mapped here.
[[nodiscard]] constexpr std::size_t
higher_support_device_tiled_frontier_required_witness_count(
    HigherSupportDeviceTiledFrontierTerminalPolicy policy,
    std::size_t support_size,
    std::size_t maximum_relevant_closed_rank) noexcept {
  switch (policy) {
    case HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window:
      return support_size > maximum_relevant_closed_rank
          ? 0U
          : maximum_relevant_closed_rank - support_size + 1U;
    case HigherSupportDeviceTiledFrontierTerminalPolicy::
        strict_interior_carrier_q3:
      return 1U;
  }
  return 0U;
}

enum class HigherSupportDeviceTiledFrontierStatus : std::uint8_t {
  frontier_tile_complete,
  chunk_ready,
  censored,
};

enum class HigherSupportDeviceTiledFrontierStopReason : std::uint8_t {
  none = 0U,
  subdivision_capacity = 1U,
  fatal_failure = 2U,
};

// Record-segment capacities are resumable output boundaries, never terminal
// stop reasons.  The consumer must release the returned drain lease before
// asking the context to resume the same tile.
enum class HigherSupportDeviceTiledFrontierYieldReason : std::uint8_t {
  none = 0U,
  prune_segment_full = 1U,
  terminal_segment_full = 2U,
  deferred_queue_full = 3U,
  mixed_segments_full = 4U,
};

struct HigherSupportDeviceTiledFrontierConfig {
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{2U};
  std::size_t slot_tile_capacity{
      higher_support_device_tiled_frontier_maximum_slot_tile_capacity};
  std::size_t gate_evaluations_per_slot_per_chunk{
      higher_support_device_tiled_frontier_default_gate_evaluations_per_slot};
  std::size_t maximum_subdivision_count{
      higher_support_device_tiled_frontier_default_maximum_subdivision_count};
  // The certified observed maximum depth of the adopted Morton LBVH, taken
  // from MortonLbvhBuildCounters::maximum_depth of the exact index whose
  // snapshot the traversal lease retains.  The context fails closed unless
  // 16*depth + 1 fits the fixed per-slot product segment, so the proved
  // stack bound holds for every product this tile can ever expand.
  std::size_t certified_maximum_lbvh_depth{
      higher_support_device_tiled_frontier_maximum_certified_lbvh_depth};

  friend bool operator==(
      const HigherSupportDeviceTiledFrontierConfig&,
      const HigherSupportDeviceTiledFrontierConfig&) = default;
};

struct HigherSupportDeviceTiledRecordDrainLeaseAudit {
  std::uint32_t schema_version{
      higher_support_device_tiled_frontier_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t record_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t certified_maximum_lbvh_depth{};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t slot_begin{};
  std::size_t slot_end{};
  std::size_t prune_record_count{};
  std::size_t terminal_record_count{};
  std::size_t probe_receipt_count{};
  std::size_t physical_product_record_capacity{};
  std::size_t physical_prune_record_capacity{};
  std::size_t physical_terminal_record_capacity{};
  std::size_t physical_probe_receipt_capacity{};
  std::size_t physical_slot_control_capacity{};
  std::size_t fixed_products_per_slot{};
  std::size_t fixed_prune_records_per_slot{};
  std::size_t fixed_terminal_records_per_slot{};
  std::size_t fixed_probe_receipts_per_slot{};
  HigherSupportDeviceTiledFrontierYieldReason yield_reason{
      HigherSupportDeviceTiledFrontierYieldReason::none};
  bool resumes_same_tile{false};
  bool resumable_after_lease_release{false};
  bool process_restart_resumable{false};
  bool traversal_owner_retained{false};
  bool source_cloud_identity_retained{false};
  bool output_owner_retained{false};
  bool output_buffers_detached_for_tile_lifetime{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
  // The scientific host fake retains its record segments in host memory so
  // an exact consumer can replay them; the native path keeps them resident.
  bool host_fake_record_segments_resident{false};
  bool censored_slot_outputs_withheld{false};
  bool bigint_support_mass_transferred{false};
  bool floating_point_decision_performed{false};
  bool closed_rank_window_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_never_authorizes_gamma2_drop{false};
  bool exact_higher_support_terminal_classification_native_cuda{false};
  bool dense_product_fallback_performed{false};
  bool global_product_frontier_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_cell_coface_or_incidence_arena_materialized{false};
  bool slo_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const HigherSupportDeviceTiledRecordDrainLeaseAudit&,
      const HigherSupportDeviceTiledRecordDrainLeaseAudit&) = default;
};

// Move-only capability over one output chunk's categorical record segments.
// Product code can inspect counts and authority only; raw record views stay
// private so no unaudited record path can become part of the public API.
class HigherSupportDeviceTiledRecordDrainLease final {
 public:
  ~HigherSupportDeviceTiledRecordDrainLease() noexcept = default;
  HigherSupportDeviceTiledRecordDrainLease(
      HigherSupportDeviceTiledRecordDrainLease&&) noexcept = default;
  HigherSupportDeviceTiledRecordDrainLease& operator=(
      HigherSupportDeviceTiledRecordDrainLease&&) noexcept = default;
  HigherSupportDeviceTiledRecordDrainLease(
      const HigherSupportDeviceTiledRecordDrainLease&) = delete;
  HigherSupportDeviceTiledRecordDrainLease& operator=(
      const HigherSupportDeviceTiledRecordDrainLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const HigherSupportDeviceTiledRecordDrainLeaseAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  HigherSupportDeviceTiledRecordDrainLease(
      HigherSupportDeviceTiledRecordDrainLeaseAudit audit,
      std::shared_ptr<void> retained_owner,
      std::shared_ptr<void> source_owner_authority,
      std::shared_ptr<const void> source_cloud_identity_authority,
      const void* prune_records,
      const void* terminal_records,
      const void* probe_receipts,
      const void* slot_controls,
      std::size_t authorized_slot_control_extent,
      int cuda_device,
      bool host_fake);

  HigherSupportDeviceTiledRecordDrainLeaseAudit audit_{};
  std::shared_ptr<void> retained_owner_;
  std::shared_ptr<void> source_owner_authority_;
  std::shared_ptr<const void> source_cloud_identity_authority_;
  const void* prune_records_{};
  const void* terminal_records_{};
  const void* probe_receipts_{};
  const void* slot_controls_{};
  std::size_t authorized_slot_control_extent_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend class HigherSupportDeviceTiledFrontierContext;
  friend class detail::
      Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess;
};

struct HigherSupportDeviceTiledFrontierAudit {
  std::uint32_t schema_version{
      higher_support_device_tiled_frontier_schema_version};
  std::uint64_t advance_sequence{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t record_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t certified_maximum_lbvh_depth{};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t slot_tile_capacity{};
  std::size_t gate_evaluations_per_slot_per_chunk{};
  std::size_t maximum_subdivision_count{};
  std::size_t fixed_products_per_slot{};
  std::size_t fixed_prune_records_per_slot{};
  std::size_t fixed_terminal_records_per_slot{};
  std::size_t fixed_probe_receipts_per_slot{};
  std::size_t fixed_pending_decisions_per_slot{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  HigherSupportDeviceTiledFrontierYieldReason yield_reason{
      HigherSupportDeviceTiledFrontierYieldReason::none};
  bool resumes_same_tile{false};
  bool resumable_capacity_yield{false};
  bool process_restart_resumable{false};
  std::size_t transaction_slot_count{};
  std::size_t transaction_committed_slot_count{};
  std::size_t transaction_prune_record_count{};
  std::size_t transaction_terminal_record_count{};
  std::size_t transaction_probe_receipt_count{};
  std::size_t transaction_subdivision_count{};
  std::uint64_t transaction_expansion_count{};
  std::uint64_t transaction_gate_evaluation_count{};
  std::uint64_t transaction_probe_root_decision_count{};
  std::uint64_t transaction_probe_fallback_decision_count{};
  std::uint64_t transaction_stack_high_water{};
  // Exact BigInt masses revalidated by the host from the 128-bit slot
  // controls.  The BigInt values themselves never cross the device frontier.
  exact::BigInt tile_universe_support_mass{0};
  exact::BigInt cumulative_resolved_support_mass{0};
  exact::BigInt cumulative_well_prune_support_mass{0};
  exact::BigInt cumulative_rank_prune_support_mass{0};
  exact::BigInt cumulative_terminal_support_mass{0};
  exact::BigInt unresolved_support_mass{0};
  std::size_t completed_tile_count{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t slot_control_device_to_host_count{};
  std::size_t slot_control_device_to_host_byte_count{};
  std::size_t resume_control_device_to_host_count{};
  std::size_t resume_control_device_to_host_byte_count{};
  std::size_t deferred_int512_decision_count{};
  std::size_t deferred_int1024_decision_count{};
  std::size_t host_rational_drain_count{};
  int cuda_device{-1};
  bool source_traversal_lease_authenticated{false};
  bool certified_lbvh_depth_stack_bound_enforced{false};
  bool unsigned128_slot_mass_accounting_certified{false};
  bool slot_universe_masses_host_bigint_revalidated{false};
  bool fixed_per_slot_caps_enforced{false};
  bool atomic_tile_commit_enforced{false};
  bool censored_slot_outputs_withheld{false};
  bool slot_partition_validated{false};
  bool terminally_censored{false};
  bool traversal_lease_owner_retained{false};
  bool source_cloud_identity_retained{false};
  bool record_drain_lease_published{false};
  bool record_drain_lease_backpressure_bounded_to_one{false};
  bool record_drain_lease_outstanding{false};
  bool output_buffers_detached_for_tile_lifetime{false};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool bigint_support_mass_transferred{false};
  bool floating_point_decision_performed{false};
  bool probe_plan_order_receipt_commit_enforced{false};
  bool substitute_probe_center_seed_policy_v1{false};
  bool closed_rank_window_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_never_authorizes_gamma2_drop{false};
  bool exact_higher_support_terminal_classification_native_cuda{false};
  bool dense_product_fallback_performed{false};
  bool global_product_frontier_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_cell_coface_or_incidence_arena_materialized{false};
  bool scientific_decision_published{false};
  bool slo_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const HigherSupportDeviceTiledFrontierAudit&,
      const HigherSupportDeviceTiledFrontierAudit&) = default;
};

struct HigherSupportDeviceTiledFrontierAdvance {
  HigherSupportDeviceTiledFrontierStatus status{
      HigherSupportDeviceTiledFrontierStatus::censored};
  HigherSupportDeviceTiledFrontierStopReason stop_reason{
      HigherSupportDeviceTiledFrontierStopReason::fatal_failure};
  HigherSupportDeviceTiledFrontierYieldReason yield_reason{
      HigherSupportDeviceTiledFrontierYieldReason::none};
  std::optional<HigherSupportDeviceTiledRecordDrainLease> record_drain;
  HigherSupportDeviceTiledFrontierAudit audit{};

  HigherSupportDeviceTiledFrontierAdvance() = default;
  HigherSupportDeviceTiledFrontierAdvance(
      HigherSupportDeviceTiledFrontierAdvance&&) noexcept = default;
  HigherSupportDeviceTiledFrontierAdvance& operator=(
      HigherSupportDeviceTiledFrontierAdvance&&) noexcept = default;
  HigherSupportDeviceTiledFrontierAdvance(
      const HigherSupportDeviceTiledFrontierAdvance&) = delete;
  HigherSupportDeviceTiledFrontierAdvance& operator=(
      const HigherSupportDeviceTiledFrontierAdvance&) = delete;
};

namespace detail {
struct Phase15HigherSupportDeviceTiledAdoptedTraversal;
class Phase15HigherSupportDeviceTiledFrontierContextState;
class Phase15HigherSupportDeviceTiledFrontierHostState;

// The single private-lease access seam.  Host tests provide a scientific
// fake; the CUDA definition transfers the retained owner, immutable cloud
// identity and resident raw views without exposing them through the public
// API.
[[nodiscard]] Phase15HigherSupportDeviceTiledAdoptedTraversal
adopt_phase15_higher_support_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease);
}  // namespace detail

// Device-tiled work engine for the exact supports-3/4 frontier.  Scientific
// authority stays with the host induction (ExactHigherSupportAnchoredSession,
// Q_{j+1} = T_{b_j}(Q_j)); this context only resolves one host-rooted slot
// tile at a time and returns categorical records whose masses the host
// revalidates in exact::BigInt before any session commit.
class HigherSupportDeviceTiledFrontierContext final {
 public:
  static constexpr std::string_view backend =
      higher_support_device_tiled_frontier_backend;
  static constexpr std::string_view profile =
      higher_support_device_tiled_frontier_profile;
  static constexpr std::string_view mode =
      higher_support_device_tiled_frontier_mode;
  static constexpr std::string_view deployment_status =
      higher_support_device_tiled_frontier_deployment_status;
  static constexpr std::string_view public_status =
      higher_support_device_tiled_frontier_public_status;
  static constexpr std::string_view proof_basis =
      higher_support_device_tiled_frontier_proof_basis;

  HigherSupportDeviceTiledFrontierContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      HigherSupportDeviceTiledFrontierConfig config = {});
  ~HigherSupportDeviceTiledFrontierContext() noexcept;

  HigherSupportDeviceTiledFrontierContext(
      HigherSupportDeviceTiledFrontierContext&&) noexcept;
  HigherSupportDeviceTiledFrontierContext& operator=(
      HigherSupportDeviceTiledFrontierContext&&) noexcept;
  HigherSupportDeviceTiledFrontierContext(
      const HigherSupportDeviceTiledFrontierContext&) = delete;
  HigherSupportDeviceTiledFrontierContext& operator=(
      const HigherSupportDeviceTiledFrontierContext&) = delete;

  // Opens one slot tile rooted on canonical host frontier entries.  Every
  // entry is revalidated fail-closed (arity, sorted disjoint groups,
  // multiplicities, zero padding, Morton ranges within the adopted
  // traversal) and its exact BigInt support mass is certified to fit the
  // unsigned 128-bit slot accounting before any work is scheduled.  Under
  // the strict-interior carrier policy only support-three entries are
  // admissible.
  void open_tile(
      std::span<const hierarchy::ExactHigherSupportFrontierEntry> entries);

  [[nodiscard]] HigherSupportDeviceTiledFrontierAdvance advance();
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] bool tile_active() const noexcept;
  [[nodiscard]] bool terminally_censored() const noexcept {
    return terminally_censored_;
  }
  [[nodiscard]] const HigherSupportDeviceTiledFrontierConfig& config()
      const noexcept {
    return config_;
  }

 private:
  HigherSupportDeviceTiledFrontierConfig config_{};
  std::shared_ptr<
      detail::Phase15HigherSupportDeviceTiledFrontierContextState>
      state_;
  std::unique_ptr<detail::Phase15HigherSupportDeviceTiledFrontierHostState>
      host_;
  std::weak_ptr<void> active_record_drain_authority_;
  std::size_t point_count_{};
  std::size_t certified_node_count_{};
  std::size_t completed_tile_count_{};
  std::uint64_t advance_sequence_{};
  std::size_t launcher_call_count_{};
  bool terminally_censored_{false};
  bool host_fake_launcher_exercised_{false};
  bool cuda_execution_performed_{false};
  HigherSupportDeviceTiledFrontierStopReason terminal_stop_reason_{
      HigherSupportDeviceTiledFrontierStopReason::none};
};

}  // namespace morsehgp3d::gpu
