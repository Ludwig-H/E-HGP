#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::gpu {

namespace detail {
class Phase15RankedDiametralPairCatalogContextState;
class Phase15RankedDiametralPairCatalogHostState;
}  // namespace detail

inline constexpr std::uint32_t
    ranked_diametral_pair_catalog_contract_schema_version = 2U;
inline constexpr std::size_t
    ranked_diametral_pair_catalog_maximum_level = 10U;
inline constexpr std::size_t
    ranked_diametral_pair_catalog_maximum_anchor_tile_size = 4'096U;
inline constexpr std::string_view
    ranked_diametral_pair_catalog_contract_backend =
        "future_cuda_g4";
inline constexpr std::string_view
    ranked_diametral_pair_catalog_contract_profile = "hgp_reduced";
inline constexpr std::string_view
    ranked_diametral_pair_catalog_contract_mode =
        "host_contract_fake_launcher_only";
inline constexpr std::string_view
    ranked_diametral_pair_catalog_contract_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    ranked_diametral_pair_catalog_contract_public_status = "not_claimed";
inline constexpr bool
    ranked_diametral_pair_catalog_cuda_implementation_available = false;

enum class RankedDiametralPairCatalogBuildStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class RankedDiametralPairCatalogStopReason : std::uint8_t {
  none,
  work_item_capacity,
  yao48_survivor_occurrence_capacity,
  canonical_candidate_pair_capacity,
  catalog_record_capacity,
  payload_point_id_capacity,
  exact_fallback_capacity,
};

struct RankedDiametralPairCatalogBudget {
  std::size_t maximum_anchor_tile_size{};
  std::size_t maximum_work_item_count{};
  // This arena receives only certified Yao48 survivors.  The Morton ownership
  // universe is traversed in fused form and is never written as a pair stream.
  std::size_t maximum_yao48_survivor_occurrence_count{};
  // Canonical PointId pairs after on-device sort/dedup and before exact rank.
  std::size_t maximum_canonical_candidate_pair_count{};
  std::size_t maximum_catalog_record_count{};
  std::size_t maximum_payload_point_id_count{};
  std::size_t maximum_exact_fallback_count{};

  friend bool operator==(
      const RankedDiametralPairCatalogBudget&,
      const RankedDiametralPairCatalogBudget&) = default;
};

enum class RankedPairYao48FilterPolicy : std::uint8_t {
  // One certified Yao48 report, anchored at the execution owner, is sufficient
  // for exhaustive closed-rank enumeration.  Morton chooses that owner only
  // to partition work; it never certifies a geometric decision.
  owner_unilateral,
  // An optional inverse Yao48 report may reject more owner-side survivors.
  // It is a performance strengthening, never an exhaustiveness requirement.
  bidirectional_intersection,
};

// This closure concerns only exhaustive closed-rank enumeration.  Its Yao
// witnesses may lie on the diametral shell, so none of its pruned mass is an
// authority for RelevantGP.  Morton/LBVH supplies only an exact-once execution
// partition of the unordered universe; no raw Morton pair stream is a
// scientific input.  Every Yao48 survivor is canonicalized as
// (min(PointId), max(PointId)) and deduplicated before exact classification.
struct ClosedRankCatalogClosure {
  RankedPairYao48FilterPolicy yao48_filter_policy{
      RankedPairYao48FilterPolicy::owner_unilateral};
  std::uint64_t owned_pair_universe_count{};
  std::uint64_t yao48_owner_pruned_pair_count{};
  std::uint64_t yao48_inverse_pruned_pair_count{};
  std::uint64_t yao48_survivor_occurrence_count{};
  std::uint64_t duplicate_survivor_occurrence_count{};
  std::uint64_t canonical_candidate_pair_count{};
  std::uint64_t above_window_pair_count{};
  std::uint64_t ranked_record_count{};
  std::uint64_t unresolved_owned_pair_count{};
  std::uint64_t unresolved_candidate_pair_count{};
  std::uint64_t unresolved_exact_predicate_count{};
  bool closed_rank_catalog_complete{false};

  // Receipt identities:
  //   owner_pruned + inverse_pruned + canonical_candidate + unresolved_owned
  //     == owned_pair_universe;
  //   canonical_candidate + duplicate_occurrence == survivor_occurrence;
  //   above_window + ranked_record + unresolved_candidate
  //     == canonical_candidate.

  friend bool operator==(
      const ClosedRankCatalogClosure&,
      const ClosedRankCatalogClosure&) = default;
};

enum class PairSupportRelevantGpDecision : std::uint8_t {
  unknown,
  satisfied,
  violated,
};

// The type name deliberately scopes this receipt to supports of size two.
// A hierarchy-level relevant_gp_complete flag must additionally reconcile the
// independent triangle and tetrahedron receipts.
struct PairSupportRelevantGpClosure {
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t strict_interior_pruned_pair_count{};
  std::uint64_t shell_scanned_pair_count{};
  std::uint64_t unresolved_pair_count{};
  std::uint64_t exact_violation_count{};
  spatial::PointId canonical_violation_support_u{};
  spatial::PointId canonical_violation_support_v{};
  spatial::PointId canonical_violation_extra_shell{};
  PairSupportRelevantGpDecision decision{
      PairSupportRelevantGpDecision::unknown};
  bool canonical_violation_present{false};
  bool relevant_gp_complete{false};

  friend bool operator==(
      const PairSupportRelevantGpClosure&,
      const PairSupportRelevantGpClosure&) = default;
};

struct RankedDiametralPairCatalogDeviceLeaseAudit {
  std::uint32_t contract_schema_version{
      ranked_diametral_pair_catalog_contract_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_level{};
  std::size_t catalog_record_count{};
  std::size_t strict_interior_point_id_count{};
  std::size_t extra_shell_point_id_count{};
  std::size_t rank_offset_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  bool closed_rank_catalog_complete{false};
  bool pair_support_relevant_gp_complete{false};
  bool traversal_owner_retained{false};
  bool catalog_owner_retained{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
  bool morton_ownership_partition_validated{false};
  bool yao48_filter_policy_validated{false};
  bool candidate_point_id_canonicalization_validated{false};
  bool candidate_deduplication_before_exact_classification_validated{false};
  bool morton_scientific_authority_claimed{false};
  bool raw_morton_pair_stream_materialized{false};
  bool global_relevant_gp_complete_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const RankedDiametralPairCatalogDeviceLeaseAudit&,
      const RankedDiametralPairCatalogDeviceLeaseAudit&) = default;
};

// The SoA pointers stay private.  Future device-side triangle and
// tetrahedron consumers will receive explicit friendship rather than a raw
// public device ABI.
class RankedDiametralPairCatalogDeviceLease final {
 public:
  ~RankedDiametralPairCatalogDeviceLease() noexcept = default;
  RankedDiametralPairCatalogDeviceLease(
      RankedDiametralPairCatalogDeviceLease&&) noexcept = default;
  RankedDiametralPairCatalogDeviceLease& operator=(
      RankedDiametralPairCatalogDeviceLease&&) noexcept = default;
  RankedDiametralPairCatalogDeviceLease(
      const RankedDiametralPairCatalogDeviceLease&) = delete;
  RankedDiametralPairCatalogDeviceLease& operator=(
      const RankedDiametralPairCatalogDeviceLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const RankedDiametralPairCatalogDeviceLeaseAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  RankedDiametralPairCatalogDeviceLease(
      RankedDiametralPairCatalogDeviceLeaseAudit audit,
      std::shared_ptr<void> retained_resources,
      const std::uint64_t* device_support_u,
      const std::uint64_t* device_support_v,
      const std::uint8_t* device_closed_rank,
      const std::uint64_t* device_rank_offsets,
      const std::uint64_t* device_strict_offsets,
      const std::uint64_t* device_strict_point_ids,
      const std::uint64_t* device_shell_offsets,
      const std::uint64_t* device_shell_point_ids,
      int cuda_device,
      bool host_fake);

  RankedDiametralPairCatalogDeviceLeaseAudit audit_{};
  std::shared_ptr<void> retained_resources_;
  const std::uint64_t* device_support_u_{};
  const std::uint64_t* device_support_v_{};
  const std::uint8_t* device_closed_rank_{};
  const std::uint64_t* device_rank_offsets_{};
  const std::uint64_t* device_strict_offsets_{};
  const std::uint64_t* device_strict_point_ids_{};
  const std::uint64_t* device_shell_offsets_{};
  const std::uint64_t* device_shell_point_ids_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend class RankedDiametralPairCatalogContext;
};

struct RankedDiametralPairCatalogBuildAudit {
  std::uint32_t contract_schema_version{
      ranked_diametral_pair_catalog_contract_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_level{};
  RankedDiametralPairCatalogBudget fixed_budget{};
  std::size_t catalog_record_count{};
  std::size_t strict_interior_point_id_count{};
  std::size_t extra_shell_point_id_count{};
  std::size_t rank_offset_count{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t intermediate_candidate_device_to_host_count{};
  std::size_t legacy_host_callback_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  std::uint64_t receipt_digest_fnv1a{};
  int cuda_device{-1};
  bool traversal_lease_owner_retained{false};
  bool traversal_lease_extents_validated{false};
  bool fixed_capacity_preflight_satisfied{false};
  bool fixed_capacity_echo_validated{false};
  bool one_multi_order_launch_validated{false};
  bool closed_rank_mass_validated{false};
  bool morton_ownership_partition_validated{false};
  bool yao48_filter_policy_validated{false};
  bool candidate_point_id_canonicalization_validated{false};
  bool candidate_deduplication_before_exact_classification_validated{false};
  bool rank_bucket_layout_validated{false};
  bool output_layout_validated{false};
  bool payload_partition_validated{false};
  bool pair_support_relevant_gp_mass_validated{false};
  bool closed_and_relevant_gp_proofs_separated{false};
  bool receipt_digest_validated{false};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool exact_gpu_predicates_implemented{false};
  bool scientific_decision_published{false};
  bool global_relevant_gp_complete_published{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool morton_scientific_authority_claimed{false};
  bool raw_morton_pair_stream_materialized{false};
  bool forbidden_global_pair_matrix_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const RankedDiametralPairCatalogBuildAudit&,
      const RankedDiametralPairCatalogBuildAudit&) = default;
};

class RankedDiametralPairCatalogBuildAttempt final {
 public:
  static constexpr std::string_view backend =
      ranked_diametral_pair_catalog_contract_backend;
  static constexpr std::string_view profile =
      ranked_diametral_pair_catalog_contract_profile;
  static constexpr std::string_view mode =
      ranked_diametral_pair_catalog_contract_mode;
  static constexpr std::string_view deployment_status =
      ranked_diametral_pair_catalog_contract_deployment_status;
  static constexpr std::string_view public_status =
      ranked_diametral_pair_catalog_contract_public_status;

  RankedDiametralPairCatalogBuildAttempt(
      RankedDiametralPairCatalogBuildAttempt&&) noexcept = default;
  RankedDiametralPairCatalogBuildAttempt& operator=(
      RankedDiametralPairCatalogBuildAttempt&&) noexcept = default;
  RankedDiametralPairCatalogBuildAttempt(
      const RankedDiametralPairCatalogBuildAttempt&) = delete;
  RankedDiametralPairCatalogBuildAttempt& operator=(
      const RankedDiametralPairCatalogBuildAttempt&) = delete;

  [[nodiscard]] RankedDiametralPairCatalogBuildStatus status()
      const noexcept {
    return status_;
  }
  [[nodiscard]] RankedDiametralPairCatalogStopReason stop_reason()
      const noexcept {
    return stop_reason_;
  }
  [[nodiscard]] const ClosedRankCatalogClosure& closed_rank_closure()
      const noexcept {
    return closed_rank_closure_;
  }
  [[nodiscard]] const PairSupportRelevantGpClosure&
  pair_support_relevant_gp_closure() const noexcept {
    return pair_support_relevant_gp_closure_;
  }
  [[nodiscard]] const RankedDiametralPairCatalogBuildAudit& audit()
      const noexcept {
    return audit_;
  }
  [[nodiscard]] bool validated_contract_attempt() const noexcept;
  [[nodiscard]] bool closed_rank_catalog_available() const noexcept;
  [[nodiscard]] bool scientific_outcome() const noexcept;
  [[nodiscard]] const RankedDiametralPairCatalogDeviceLease& catalog()
      const &;
  [[nodiscard]] const RankedDiametralPairCatalogDeviceLease& catalog()
      const && = delete;
  [[nodiscard]] RankedDiametralPairCatalogDeviceLease release_catalog();

 private:
  RankedDiametralPairCatalogBuildAttempt(
      RankedDiametralPairCatalogBuildStatus status,
      RankedDiametralPairCatalogStopReason stop_reason,
      ClosedRankCatalogClosure closed_rank_closure,
      PairSupportRelevantGpClosure pair_support_relevant_gp_closure,
      RankedDiametralPairCatalogBuildAudit audit,
      std::optional<RankedDiametralPairCatalogDeviceLease> catalog);

  RankedDiametralPairCatalogBuildStatus status_{
      RankedDiametralPairCatalogBuildStatus::budget_exhausted};
  RankedDiametralPairCatalogStopReason stop_reason_{
      RankedDiametralPairCatalogStopReason::work_item_capacity};
  ClosedRankCatalogClosure closed_rank_closure_{};
  PairSupportRelevantGpClosure pair_support_relevant_gp_closure_{};
  RankedDiametralPairCatalogBuildAudit audit_{};
  std::optional<RankedDiametralPairCatalogDeviceLease> catalog_;

  friend class RankedDiametralPairCatalogContext;
};

// One context owns one immutable LBVH traversal lease and performs exactly one
// multi-order build attempt.  The scaffold launcher is host-fake only; the
// result therefore never constitutes a scientific or public CUDA outcome.
class RankedDiametralPairCatalogContext final {
 public:
  RankedDiametralPairCatalogContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      RankedDiametralPairCatalogBudget fixed_budget);
  ~RankedDiametralPairCatalogContext() noexcept;

  RankedDiametralPairCatalogContext(
      RankedDiametralPairCatalogContext&&) noexcept;
  RankedDiametralPairCatalogContext& operator=(
      RankedDiametralPairCatalogContext&&) noexcept;
  RankedDiametralPairCatalogContext(
      const RankedDiametralPairCatalogContext&) = delete;
  RankedDiametralPairCatalogContext& operator=(
      const RankedDiametralPairCatalogContext&) = delete;

  [[nodiscard]] RankedDiametralPairCatalogBuildAttempt build(
      std::size_t maximum_level);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t certified_node_count() const noexcept;
  [[nodiscard]] const RankedDiametralPairCatalogBudget& fixed_budget()
      const noexcept;

 private:
  std::shared_ptr<detail::Phase15RankedDiametralPairCatalogContextState>
      state_;
  std::unique_ptr<detail::Phase15RankedDiametralPairCatalogHostState>
      host_;
  RankedDiametralPairCatalogBudget fixed_budget_{};
  std::uint64_t last_buffer_epoch_{};
  bool build_attempted_{false};
};

}  // namespace morsehgp3d::gpu
