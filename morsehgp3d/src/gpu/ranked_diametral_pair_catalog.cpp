#include "morsehgp3d/gpu/ranked_diametral_pair_catalog.hpp"

#include "../cuda/phase15_ranked_diametral_pair_catalog_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::gpu::detail {

class Phase15RankedDiametralPairCatalogHostState final {
 public:
  std::shared_ptr<const void> source_cloud_identity;
  std::shared_ptr<void> adopted_traversal_owner;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t coordinate_word_capacity{};
  std::size_t morton_point_id_capacity{};
  std::size_t node_capacity{};
  std::size_t point_count{};
  std::size_t node_count{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool cuda_resident{false};
  bool host_fake{false};

  [[nodiscard]] Phase15RankedPairCatalogAdoptedTraversal
  adopted_traversal() const {
    if (!adopted_traversal_owner || !source_cloud_identity) {
      throw std::logic_error(
          "the Phase 15 ranked-pair context has no traversal owner");
    }
    return Phase15RankedPairCatalogAdoptedTraversal{
        adopted_traversal_owner,
        source_cloud_identity,
        device_coordinate_bits,
        device_morton_point_ids,
        device_nodes,
        coordinate_word_capacity,
        morton_point_id_capacity,
        node_capacity,
        point_count,
        node_count,
        source_snapshot_epoch,
        cuda_device,
        cuda_resident,
        host_fake};
  }
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

struct CombinedCatalogOwner {
  std::shared_ptr<void> traversal_owner;
  std::shared_ptr<void> catalog_owner;
  std::shared_ptr<const void> source_cloud_identity;
};

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the ranked-pair catalog node count overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::length_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t checked_add_u64(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::runtime_error(message);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_mul_u64(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::uint64_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::uint64_t twice_unordered_pair_universe(
    std::size_t point_count) {
  const std::uint64_t count = checked_u64(
      point_count, "the ranked-pair point count does not fit uint64");
  return checked_mul_u64(
      count,
      count - UINT64_C(1),
      "twice the unordered pair universe overflows uint64");
}

void validate_fixed_budget(
    const RankedDiametralPairCatalogBudget& budget) {
  if (budget.maximum_anchor_tile_size == 0U ||
      budget.maximum_anchor_tile_size >
          ranked_diametral_pair_catalog_maximum_anchor_tile_size ||
      budget.maximum_work_item_count == 0U ||
      budget.maximum_yao48_survivor_occurrence_count == 0U ||
      budget.maximum_canonical_candidate_pair_count == 0U ||
      budget.maximum_catalog_record_count == 0U ||
      budget.maximum_payload_point_id_count == 0U ||
      budget.maximum_catalog_record_count >
          budget.maximum_canonical_candidate_pair_count) {
    throw std::invalid_argument(
        "the ranked-pair catalog requires nonzero fixed capacities, a tile "
        "of at most 4096 anchors and no more records than canonical "
        "candidates");
  }
  static_cast<void>(checked_product(
      checked_product(
          budget.maximum_anchor_tile_size,
          48U,
          "the ranked-pair chamber tile overflows size_t"),
      ranked_diametral_pair_catalog_maximum_level,
      "the ranked-pair witness tile overflows size_t"));
  static_cast<void>(checked_product(
      budget.maximum_yao48_survivor_occurrence_count,
      2U * sizeof(std::uint64_t),
      "the ranked-pair Yao48 survivor arena overflows size_t"));
  static_cast<void>(checked_product(
      budget.maximum_canonical_candidate_pair_count,
      2U * sizeof(std::uint64_t),
      "the ranked-pair canonical candidate arena overflows size_t"));
  static_cast<void>(checked_product(
      budget.maximum_catalog_record_count,
      3U * sizeof(std::uint64_t),
      "the ranked-pair catalog arena overflows size_t"));
  static_cast<void>(checked_product(
      budget.maximum_payload_point_id_count,
      sizeof(std::uint64_t),
      "the ranked-pair payload arena overflows size_t"));
}

[[nodiscard]] RankedDiametralPairCatalogBuildStatus decode_status(
    std::uint64_t status) {
  if (status == static_cast<std::uint64_t>(
                    RankedDiametralPairCatalogBuildStatus::complete)) {
    return RankedDiametralPairCatalogBuildStatus::complete;
  }
  if (status == static_cast<std::uint64_t>(
                    RankedDiametralPairCatalogBuildStatus::
                        budget_exhausted)) {
    return RankedDiametralPairCatalogBuildStatus::budget_exhausted;
  }
  throw std::runtime_error(
      "the ranked-pair launcher returned an invalid build status");
}

[[nodiscard]] RankedDiametralPairCatalogStopReason decode_stop_reason(
    std::uint64_t reason) {
  switch (reason) {
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::none):
      return RankedDiametralPairCatalogStopReason::none;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::work_item_capacity):
      return RankedDiametralPairCatalogStopReason::work_item_capacity;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::
            yao48_survivor_occurrence_capacity):
      return RankedDiametralPairCatalogStopReason::
          yao48_survivor_occurrence_capacity;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::
            canonical_candidate_pair_capacity):
      return RankedDiametralPairCatalogStopReason::
          canonical_candidate_pair_capacity;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::catalog_record_capacity):
      return RankedDiametralPairCatalogStopReason::
          catalog_record_capacity;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::
            payload_point_id_capacity):
      return RankedDiametralPairCatalogStopReason::
          payload_point_id_capacity;
    case static_cast<std::uint64_t>(
        RankedDiametralPairCatalogStopReason::exact_fallback_capacity):
      return RankedDiametralPairCatalogStopReason::
          exact_fallback_capacity;
  }
  throw std::runtime_error(
      "the ranked-pair launcher returned an invalid stop reason");
}

void validate_closed_rank_closure(
    const ClosedRankCatalogClosure& closure,
    std::uint64_t expected_owned_universe) {
  if (closure.yao48_filter_policy !=
          RankedPairYao48FilterPolicy::owner_unilateral &&
      closure.yao48_filter_policy !=
          RankedPairYao48FilterPolicy::bidirectional_intersection) {
    throw std::runtime_error(
        "the ranked-pair receipt names an invalid Yao48 filter policy");
  }
  if (closure.yao48_filter_policy ==
          RankedPairYao48FilterPolicy::owner_unilateral &&
      closure.yao48_inverse_pruned_pair_count != 0U) {
    throw std::runtime_error(
        "a unilateral Yao48 receipt reports an inverse-filter prune");
  }
  if (closure.owned_pair_universe_count != expected_owned_universe) {
    throw std::runtime_error(
        "the ranked-pair closed receipt names the wrong owned-pair universe");
  }
  const std::uint64_t all_yao48_prunes = checked_add_u64(
      closure.yao48_owner_pruned_pair_count,
      closure.yao48_inverse_pruned_pair_count,
      "the ranked-pair closed owned mass overflows");
  const std::uint64_t decided_owned = checked_add_u64(
      all_yao48_prunes,
      closure.canonical_candidate_pair_count,
      "the ranked-pair closed owned mass overflows");
  const std::uint64_t accounted_owned = checked_add_u64(
      decided_owned,
      closure.unresolved_owned_pair_count,
      "the ranked-pair closed owned mass overflows");
  if (accounted_owned != expected_owned_universe) {
    throw std::runtime_error(
        "the ranked-pair Yao48 prune/canonical-survivor/frontier mass does "
        "not close");
  }
  if (checked_add_u64(
          closure.canonical_candidate_pair_count,
          closure.duplicate_survivor_occurrence_count,
          "the ranked-pair survivor occurrence mass overflows") !=
      closure.yao48_survivor_occurrence_count) {
    throw std::runtime_error(
        "the ranked-pair Yao48 survivor canonicalization/deduplication mass "
        "is inconsistent");
  }
  const std::uint64_t classified = checked_add_u64(
      closure.above_window_pair_count,
      closure.ranked_record_count,
      "the ranked-pair classification mass overflows");
  if (checked_add_u64(
          classified,
          closure.unresolved_candidate_pair_count,
          "the ranked-pair classification mass overflows") !=
      closure.canonical_candidate_pair_count) {
    throw std::runtime_error(
        "the ranked-pair multi-order classification mass is inconsistent");
  }
  if (closure.closed_rank_catalog_complete &&
      (closure.unresolved_owned_pair_count != 0U ||
       closure.unresolved_candidate_pair_count != 0U ||
       closure.unresolved_exact_predicate_count != 0U)) {
    throw std::runtime_error(
        "a complete ranked-pair closed receipt retains an unresolved "
        "frontier or exact predicate");
  }
}

void validate_pair_support_relevant_gp_closure(
    const PairSupportRelevantGpClosure& closure,
    std::size_t point_count,
    std::uint64_t expected_unordered_universe) {
  if (closure.unordered_pair_universe_count !=
      expected_unordered_universe) {
    throw std::runtime_error(
        "the pair-support RelevantGP receipt names the wrong universe");
  }
  const std::uint64_t decided = checked_add_u64(
      closure.strict_interior_pruned_pair_count,
      closure.shell_scanned_pair_count,
      "the pair-support RelevantGP mass overflows");
  if (checked_add_u64(
          decided,
          closure.unresolved_pair_count,
          "the pair-support RelevantGP mass overflows") !=
      expected_unordered_universe) {
    throw std::runtime_error(
        "the pair-support RelevantGP strict/shell/frontier mass does not "
        "close");
  }

  switch (closure.decision) {
    case PairSupportRelevantGpDecision::unknown:
      if (closure.relevant_gp_complete ||
          closure.exact_violation_count != 0U ||
          closure.canonical_violation_present ||
          closure.canonical_violation_support_u != 0U ||
          closure.canonical_violation_support_v != 0U ||
          closure.canonical_violation_extra_shell != 0U) {
        throw std::runtime_error(
            "an unknown pair-support RelevantGP decision carries authority");
      }
      break;
    case PairSupportRelevantGpDecision::satisfied:
      if (!closure.relevant_gp_complete ||
          closure.unresolved_pair_count != 0U ||
          closure.exact_violation_count != 0U ||
          closure.canonical_violation_present ||
          closure.canonical_violation_support_u != 0U ||
          closure.canonical_violation_support_v != 0U ||
          closure.canonical_violation_extra_shell != 0U) {
        throw std::runtime_error(
            "a satisfied pair-support RelevantGP receipt is not closed");
      }
      break;
    case PairSupportRelevantGpDecision::violated:
      if (closure.relevant_gp_complete ||
          closure.exact_violation_count == 0U ||
          !closure.canonical_violation_present) {
        throw std::runtime_error(
            "a violated pair-support RelevantGP receipt lacks its exact "
            "witness");
      }
      if (closure.canonical_violation_support_u >=
              static_cast<spatial::PointId>(point_count) ||
          closure.canonical_violation_support_v >=
              static_cast<spatial::PointId>(point_count) ||
          closure.canonical_violation_extra_shell >=
              static_cast<spatial::PointId>(point_count) ||
          closure.canonical_violation_support_u >=
              closure.canonical_violation_support_v ||
          closure.canonical_violation_extra_shell ==
              closure.canonical_violation_support_u ||
          closure.canonical_violation_extra_shell ==
              closure.canonical_violation_support_v) {
        throw std::runtime_error(
            "the pair-support RelevantGP violation witness is malformed");
      }
      break;
  }
}

struct ValidatedReceipt {
  RankedDiametralPairCatalogBuildStatus status{};
  RankedDiametralPairCatalogStopReason stop_reason{};
  RankedDiametralPairCatalogBuildAudit audit{};
};

[[nodiscard]] ValidatedReceipt validate_receipt(
    const detail::Phase15RankedPairCatalogDeviceReceipt& receipt,
    const detail::Phase15RankedDiametralPairCatalogHostState& host,
    const RankedDiametralPairCatalogBudget& fixed_budget,
    std::size_t maximum_level,
    std::uint64_t last_buffer_epoch) {
  if (receipt.receipt_digest_fnv1a !=
      detail::phase15_ranked_pair_catalog_receipt_digest(receipt)) {
    throw std::runtime_error(
        "the ranked-pair launcher receipt digest is invalid");
  }
  if (receipt.source_snapshot_epoch != host.source_snapshot_epoch ||
      receipt.contract_schema_version !=
          ranked_diametral_pair_catalog_contract_schema_version ||
      receipt.buffer_epoch == 0U ||
      receipt.buffer_epoch <= last_buffer_epoch ||
      receipt.point_count != host.point_count ||
      receipt.node_count != host.node_count ||
      receipt.maximum_level != maximum_level ||
      receipt.fixed_budget != fixed_budget ||
      receipt.launcher_call_count != 1U) {
    throw std::runtime_error(
        "the ranked-pair launcher receipt does not match its immutable "
        "request");
  }
  if (receipt.execution_kind !=
          detail::Phase15RankedPairCatalogExecutionKind::host_fake ||
      receipt.cuda_device != -1 || receipt.cuda_path_qualified ||
      receipt.kernel_launch_count != 0U ||
      receipt.synchronization_count != 0U ||
      receipt.intermediate_candidate_device_to_host_count != 0U ||
      receipt.legacy_host_callback_count != 0U ||
      !receipt.morton_ownership_partition_validated ||
      !receipt.yao48_filter_policy_validated ||
      !receipt.candidate_point_id_canonicalization_validated ||
      !receipt
           .candidate_deduplication_before_exact_classification_validated ||
      receipt.exact_gpu_predicates_implemented ||
      receipt.scientific_decision_published ||
      receipt.global_relevant_gp_complete_published ||
      receipt.hierarchy_reduction_or_attachment_published ||
      receipt.morton_scientific_authority_claimed ||
      receipt.raw_morton_pair_stream_materialized ||
      receipt.forbidden_global_pair_matrix_materialized ||
      receipt.public_status_claimed) {
    throw std::runtime_error(
        "the host-only ranked-pair scaffold violates its execution-only "
        "Morton, Yao48 or canonical-candidate contract");
  }
  if (receipt.closed_prune_mass_reused_for_relevant_gp) {
    throw std::runtime_error(
        "a closed Yao prune was reused as pair-support RelevantGP proof");
  }

  const RankedDiametralPairCatalogBuildStatus status =
      decode_status(receipt.status);
  const RankedDiametralPairCatalogStopReason stop_reason =
      decode_stop_reason(receipt.stop_reason);
  const auto failure_code = static_cast<
      detail::Phase15RankedPairCatalogFailureCode>(receipt.failure_code);
  if ((status == RankedDiametralPairCatalogBuildStatus::complete &&
       (stop_reason != RankedDiametralPairCatalogStopReason::none ||
        failure_code != detail::Phase15RankedPairCatalogFailureCode::none)) ||
      (status == RankedDiametralPairCatalogBuildStatus::budget_exhausted &&
       (stop_reason == RankedDiametralPairCatalogStopReason::none ||
        failure_code !=
            detail::Phase15RankedPairCatalogFailureCode::
                capacity_exhausted))) {
    throw std::runtime_error(
        "the ranked-pair status, stop reason and failure code disagree");
  }

  const std::uint64_t twice_unordered_universe =
      twice_unordered_pair_universe(host.point_count);
  validate_closed_rank_closure(
      receipt.closed_rank_closure,
      twice_unordered_universe / UINT64_C(2));
  validate_pair_support_relevant_gp_closure(
      receipt.pair_support_relevant_gp_closure,
      host.point_count,
      twice_unordered_universe / UINT64_C(2));
  if (receipt.closed_rank_closure.yao48_survivor_occurrence_count >
          checked_u64(
              fixed_budget.maximum_yao48_survivor_occurrence_count,
              "the ranked-pair Yao48 survivor capacity does not fit uint64") ||
      receipt.closed_rank_closure.canonical_candidate_pair_count >
          checked_u64(
              fixed_budget.maximum_canonical_candidate_pair_count,
              "the ranked-pair canonical candidate capacity does not fit "
              "uint64")) {
    throw std::runtime_error(
        "the ranked-pair receipt exceeds a fixed Yao48 survivor or canonical "
        "candidate capacity");
  }

  const detail::Phase15RankedPairCatalogDeviceOutput& output =
      receipt.output;
  if (output.catalog_record_capacity !=
          fixed_budget.maximum_catalog_record_count ||
      output.payload_point_id_capacity !=
          fixed_budget.maximum_payload_point_id_count ||
      output.catalog_record_count > output.catalog_record_capacity ||
      output.strict_point_id_count > output.payload_point_id_capacity ||
      output.shell_point_id_count >
          output.payload_point_id_capacity - output.strict_point_id_count) {
    throw std::runtime_error(
        "the ranked-pair output exceeds or misreports its fixed capacities");
  }

  const bool complete =
      status == RankedDiametralPairCatalogBuildStatus::complete;
  if (complete) {
    if (!receipt.closed_rank_closure.closed_rank_catalog_complete ||
        output.catalog_record_count !=
            receipt.closed_rank_closure.ranked_record_count ||
        output.rank_offset_count != maximum_level + 2U || !output.owner ||
        output.rank_offsets == nullptr || output.strict_offsets == nullptr ||
        output.shell_offsets == nullptr ||
        (output.catalog_record_count != 0U &&
         (output.support_u == nullptr || output.support_v == nullptr ||
          output.closed_rank == nullptr)) ||
        (output.strict_point_id_count != 0U &&
         output.strict_point_ids == nullptr) ||
        (output.shell_point_id_count != 0U &&
         output.shell_point_ids == nullptr) ||
        !receipt.device_output_layout_validated ||
        !receipt.device_rank_offsets_validated ||
        !receipt.device_records_sorted_unique ||
        !receipt.device_payload_partition_validated) {
      throw std::runtime_error(
          "a complete ranked-pair receipt does not own a closed resident "
          "catalog layout");
    }
  } else {
    if (receipt.closed_rank_closure.closed_rank_catalog_complete ||
        output.owner || output.support_u != nullptr ||
        output.support_v != nullptr || output.closed_rank != nullptr ||
        output.rank_offsets != nullptr || output.strict_offsets != nullptr ||
        output.strict_point_ids != nullptr || output.shell_offsets != nullptr ||
        output.shell_point_ids != nullptr ||
        output.catalog_record_count != 0U ||
        output.strict_point_id_count != 0U ||
        output.shell_point_id_count != 0U ||
        output.rank_offset_count != 0U ||
        receipt.device_output_layout_validated ||
        receipt.device_rank_offsets_validated ||
        receipt.device_records_sorted_unique ||
        receipt.device_payload_partition_validated) {
      throw std::runtime_error(
          "an incomplete ranked-pair attempt published a catalog layout");
    }
  }

  RankedDiametralPairCatalogBuildAudit audit;
  audit.contract_schema_version = receipt.contract_schema_version;
  audit.point_count = host.point_count;
  audit.certified_node_count = host.node_count;
  audit.maximum_level = maximum_level;
  audit.fixed_budget = fixed_budget;
  audit.catalog_record_count = output.catalog_record_count;
  audit.strict_interior_point_id_count = output.strict_point_id_count;
  audit.extra_shell_point_id_count = output.shell_point_id_count;
  audit.rank_offset_count = output.rank_offset_count;
  audit.launcher_call_count = receipt.launcher_call_count;
  audit.cuda_kernel_launch_count = receipt.kernel_launch_count;
  audit.cuda_synchronization_count = receipt.synchronization_count;
  audit.intermediate_candidate_device_to_host_count =
      receipt.intermediate_candidate_device_to_host_count;
  audit.legacy_host_callback_count = receipt.legacy_host_callback_count;
  audit.source_snapshot_epoch = receipt.source_snapshot_epoch;
  audit.catalog_buffer_epoch = receipt.buffer_epoch;
  audit.receipt_digest_fnv1a = receipt.receipt_digest_fnv1a;
  audit.cuda_device = receipt.cuda_device;
  audit.traversal_lease_owner_retained =
      host.adopted_traversal_owner != nullptr;
  audit.traversal_lease_extents_validated = true;
  audit.fixed_capacity_preflight_satisfied = true;
  audit.fixed_capacity_echo_validated = true;
  audit.one_multi_order_launch_validated = true;
  audit.closed_rank_mass_validated = true;
  audit.morton_ownership_partition_validated =
      receipt.morton_ownership_partition_validated;
  audit.yao48_filter_policy_validated =
      receipt.yao48_filter_policy_validated;
  audit.candidate_point_id_canonicalization_validated =
      receipt.candidate_point_id_canonicalization_validated;
  audit.candidate_deduplication_before_exact_classification_validated =
      receipt
          .candidate_deduplication_before_exact_classification_validated;
  audit.rank_bucket_layout_validated = true;
  audit.output_layout_validated = true;
  audit.payload_partition_validated = true;
  audit.pair_support_relevant_gp_mass_validated = true;
  audit.closed_and_relevant_gp_proofs_separated = true;
  audit.receipt_digest_validated = true;
  audit.host_fake_launcher_exercised = true;
  audit.morton_scientific_authority_claimed =
      receipt.morton_scientific_authority_claimed;
  audit.raw_morton_pair_stream_materialized =
      receipt.raw_morton_pair_stream_materialized;
  return ValidatedReceipt{status, stop_reason, audit};
}

}  // namespace

RankedDiametralPairCatalogDeviceLease::
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
        bool host_fake)
    : audit_(audit),
      retained_resources_(std::move(retained_resources)),
      device_support_u_(device_support_u),
      device_support_v_(device_support_v),
      device_closed_rank_(device_closed_rank),
      device_rank_offsets_(device_rank_offsets),
      device_strict_offsets_(device_strict_offsets),
      device_strict_point_ids_(device_strict_point_ids),
      device_shell_offsets_(device_shell_offsets),
      device_shell_point_ids_(device_shell_point_ids),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool RankedDiametralPairCatalogDeviceLease::ready() const noexcept {
  return audit_.contract_schema_version ==
             ranked_diametral_pair_catalog_contract_schema_version &&
         retained_resources_ != nullptr &&
         audit_.closed_rank_catalog_complete &&
         audit_.traversal_owner_retained && audit_.catalog_owner_retained &&
         audit_.morton_ownership_partition_validated &&
         audit_.yao48_filter_policy_validated &&
         audit_.candidate_point_id_canonicalization_validated &&
         audit_
             .candidate_deduplication_before_exact_classification_validated &&
         device_rank_offsets_ != nullptr &&
         device_strict_offsets_ != nullptr &&
         device_shell_offsets_ != nullptr &&
         (audit_.catalog_record_count == 0U ||
          (device_support_u_ != nullptr && device_support_v_ != nullptr &&
           device_closed_rank_ != nullptr)) &&
         (audit_.strict_interior_point_id_count == 0U ||
          device_strict_point_ids_ != nullptr) &&
         (audit_.extra_shell_point_id_count == 0U ||
          device_shell_point_ids_ != nullptr) &&
         !audit_.global_relevant_gp_complete_published &&
         !audit_.morton_scientific_authority_claimed &&
         !audit_.raw_morton_pair_stream_materialized &&
         !audit_.public_status_claimed &&
         ((host_fake_ && cuda_device_ == -1 &&
           audit_.host_fake_lifecycle_exercised &&
           !audit_.cuda_device_storage_retained) ||
          (!host_fake_ && cuda_device_ >= 0 &&
           !audit_.host_fake_lifecycle_exercised &&
           audit_.cuda_device_storage_retained));
}

bool RankedDiametralPairCatalogDeviceLease::cuda_resident()
    const noexcept {
  return ready() && !host_fake_ && cuda_device_ >= 0;
}

bool RankedDiametralPairCatalogDeviceLease::host_fake() const noexcept {
  return ready() && host_fake_ && cuda_device_ == -1;
}

RankedDiametralPairCatalogBuildAttempt::
    RankedDiametralPairCatalogBuildAttempt(
        RankedDiametralPairCatalogBuildStatus status,
        RankedDiametralPairCatalogStopReason stop_reason,
        ClosedRankCatalogClosure closed_rank_closure,
        PairSupportRelevantGpClosure pair_support_relevant_gp_closure,
        RankedDiametralPairCatalogBuildAudit audit,
        std::optional<RankedDiametralPairCatalogDeviceLease> catalog)
    : status_(status),
      stop_reason_(stop_reason),
      closed_rank_closure_(closed_rank_closure),
      pair_support_relevant_gp_closure_(
          pair_support_relevant_gp_closure),
      audit_(audit),
      catalog_(std::move(catalog)) {}

bool RankedDiametralPairCatalogBuildAttempt::
    validated_contract_attempt() const noexcept {
  const bool complete =
      status_ == RankedDiametralPairCatalogBuildStatus::complete;
  return audit_.contract_schema_version ==
             ranked_diametral_pair_catalog_contract_schema_version &&
         audit_.launcher_call_count == 1U &&
         audit_.traversal_lease_owner_retained &&
         audit_.traversal_lease_extents_validated &&
         audit_.fixed_capacity_preflight_satisfied &&
         audit_.fixed_capacity_echo_validated &&
         audit_.one_multi_order_launch_validated &&
         audit_.closed_rank_mass_validated &&
         audit_.morton_ownership_partition_validated &&
         audit_.yao48_filter_policy_validated &&
         audit_.candidate_point_id_canonicalization_validated &&
         audit_
             .candidate_deduplication_before_exact_classification_validated &&
         audit_.rank_bucket_layout_validated && audit_.output_layout_validated &&
         audit_.payload_partition_validated &&
         audit_.pair_support_relevant_gp_mass_validated &&
         audit_.closed_and_relevant_gp_proofs_separated &&
         audit_.receipt_digest_validated &&
         audit_.host_fake_launcher_exercised &&
         !audit_.cuda_execution_performed &&
         !audit_.exact_gpu_predicates_implemented &&
         !audit_.scientific_decision_published &&
         !audit_.global_relevant_gp_complete_published &&
         !audit_.hierarchy_reduction_or_attachment_published &&
         !audit_.morton_scientific_authority_claimed &&
         !audit_.raw_morton_pair_stream_materialized &&
         !audit_.forbidden_global_pair_matrix_materialized &&
         !audit_.public_status_claimed &&
         ((complete &&
           stop_reason_ == RankedDiametralPairCatalogStopReason::none &&
           closed_rank_closure_.closed_rank_catalog_complete &&
           catalog_.has_value() && catalog_->ready()) ||
          (!complete &&
           stop_reason_ != RankedDiametralPairCatalogStopReason::none &&
           !closed_rank_closure_.closed_rank_catalog_complete &&
           !catalog_.has_value()));
}

bool RankedDiametralPairCatalogBuildAttempt::
    closed_rank_catalog_available() const noexcept {
  return status_ == RankedDiametralPairCatalogBuildStatus::complete &&
         closed_rank_closure_.closed_rank_catalog_complete &&
         catalog_.has_value() && catalog_->ready();
}

bool RankedDiametralPairCatalogBuildAttempt::scientific_outcome()
    const noexcept {
  return false;
}

const RankedDiametralPairCatalogDeviceLease&
RankedDiametralPairCatalogBuildAttempt::catalog() const & {
  if (!closed_rank_catalog_available()) {
    throw std::logic_error(
        "the ranked-pair attempt has no complete catalog lease");
  }
  return *catalog_;
}

RankedDiametralPairCatalogDeviceLease
RankedDiametralPairCatalogBuildAttempt::release_catalog() {
  if (!closed_rank_catalog_available()) {
    throw std::logic_error(
        "the ranked-pair attempt has no complete catalog lease to release");
  }
  RankedDiametralPairCatalogDeviceLease released =
      std::move(*catalog_);
  catalog_.reset();
  return released;
}

RankedDiametralPairCatalogContext::RankedDiametralPairCatalogContext(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    RankedDiametralPairCatalogBudget fixed_budget)
    : fixed_budget_(fixed_budget) {
  validate_fixed_budget(fixed_budget);
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a ranked-pair catalog context requires a ready traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit& lease_audit =
      traversal_lease.audit();
  const std::size_t point_count = lease_audit.point_count;
  const std::size_t node_count = checked_node_count(point_count);
  if (traversal_lease.source_cloud_identity_ == nullptr ||
      lease_audit.certified_node_count != node_count ||
      lease_audit.retained_coordinate_word_capacity <
          checked_product(
              point_count,
              3U,
              "the ranked-pair coordinate extent overflows size_t") ||
      lease_audit.retained_morton_point_id_capacity < point_count ||
      lease_audit.retained_node_capacity < node_count ||
      lease_audit.source_snapshot_epoch == 0U ||
      lease_audit.retained_host_snapshot_byte_count != 0U ||
      lease_audit.second_host_snapshot_retained ||
      lease_audit.higher_order_delaunay_mosaic_materialized ||
      lease_audit.global_cell_or_coface_arena_materialized ||
      lease_audit.public_status_claimed) {
    throw std::invalid_argument(
        "a ranked-pair traversal lease has invalid certified extents or "
        "forbidden retained structures");
  }
  const bool cuda_resident = traversal_lease.cuda_resident();
  const bool host_fake = lease_audit.host_fake_lifecycle_exercised;
  if (cuda_resident == host_fake ||
      (cuda_resident &&
       (traversal_lease.device_coordinate_bits_ == nullptr ||
        traversal_lease.device_morton_point_ids_ == nullptr ||
        traversal_lease.device_nodes_ == nullptr ||
        traversal_lease.cuda_device_ < 0)) ||
      (host_fake &&
       (traversal_lease.device_coordinate_bits_ != nullptr ||
        traversal_lease.device_morton_point_ids_ != nullptr ||
        traversal_lease.device_nodes_ != nullptr ||
        traversal_lease.cuda_device_ != -1))) {
    throw std::invalid_argument(
        "a ranked-pair traversal lease has ambiguous device ownership");
  }

  state_ = std::make_shared<
      detail::Phase15RankedDiametralPairCatalogContextState>(fixed_budget);
  host_ = std::make_unique<
      detail::Phase15RankedDiametralPairCatalogHostState>();
  host_->source_cloud_identity = traversal_lease.source_cloud_identity_;
  host_->point_count = point_count;
  host_->node_count = node_count;
  host_->coordinate_word_capacity =
      lease_audit.retained_coordinate_word_capacity;
  host_->morton_point_id_capacity =
      lease_audit.retained_morton_point_id_capacity;
  host_->node_capacity = lease_audit.retained_node_capacity;
  host_->source_snapshot_epoch = lease_audit.source_snapshot_epoch;
  host_->device_coordinate_bits = traversal_lease.device_coordinate_bits_;
  host_->device_morton_point_ids =
      traversal_lease.device_morton_point_ids_;
  host_->device_nodes = traversal_lease.device_nodes_;
  host_->cuda_device = traversal_lease.cuda_device_;
  host_->cuda_resident = cuda_resident;
  host_->host_fake = host_fake;
  host_->adopted_traversal_owner =
      std::move(traversal_lease.retained_resources_);
  traversal_lease.source_cloud_identity_.reset();
  traversal_lease.device_coordinate_bits_ = nullptr;
  traversal_lease.device_morton_point_ids_ = nullptr;
  traversal_lease.device_nodes_ = nullptr;
  traversal_lease.cuda_device_ = -1;
}

RankedDiametralPairCatalogContext::~RankedDiametralPairCatalogContext()
    noexcept = default;

RankedDiametralPairCatalogContext::RankedDiametralPairCatalogContext(
    RankedDiametralPairCatalogContext&&) noexcept = default;

RankedDiametralPairCatalogContext&
RankedDiametralPairCatalogContext::operator=(
    RankedDiametralPairCatalogContext&&) noexcept = default;

RankedDiametralPairCatalogBuildAttempt
RankedDiametralPairCatalogContext::build(std::size_t maximum_level) {
  if (!state_ || !host_) {
    throw std::logic_error(
        "a moved-from ranked-pair catalog context cannot be used");
  }
  if (build_attempted_) {
    throw std::logic_error(
        "a ranked-pair catalog context accepts exactly one multi-order "
        "build attempt");
  }
  if (maximum_level == 0U ||
      maximum_level > ranked_diametral_pair_catalog_maximum_level) {
    throw std::invalid_argument(
        "the ranked-pair maximum level must lie in [1, 10]");
  }

  return state_->with_launcher_section([&]() {
    const detail::Phase15RankedPairCatalogAdoptedTraversal traversal =
        host_->adopted_traversal();
    detail::Phase15RankedPairCatalogDeviceReceipt receipt =
        detail::build_phase15_ranked_diametral_pair_catalog_on_device(
            *state_, traversal, maximum_level, fixed_budget_);
    ValidatedReceipt validated = validate_receipt(
        receipt,
        *host_,
        fixed_budget_,
        maximum_level,
        last_buffer_epoch_);
    last_buffer_epoch_ = receipt.buffer_epoch;

    std::optional<RankedDiametralPairCatalogDeviceLease> catalog;
    if (validated.status ==
        RankedDiametralPairCatalogBuildStatus::complete) {
      auto combined_owner = std::make_shared<CombinedCatalogOwner>();
      combined_owner->traversal_owner =
          std::move(host_->adopted_traversal_owner);
      combined_owner->catalog_owner = receipt.output.owner;
      combined_owner->source_cloud_identity = host_->source_cloud_identity;

      RankedDiametralPairCatalogDeviceLeaseAudit lease_audit;
      lease_audit.contract_schema_version = receipt.contract_schema_version;
      lease_audit.point_count = host_->point_count;
      lease_audit.certified_node_count = host_->node_count;
      lease_audit.maximum_level = maximum_level;
      lease_audit.catalog_record_count =
          receipt.output.catalog_record_count;
      lease_audit.strict_interior_point_id_count =
          receipt.output.strict_point_id_count;
      lease_audit.extra_shell_point_id_count =
          receipt.output.shell_point_id_count;
      lease_audit.rank_offset_count = receipt.output.rank_offset_count;
      lease_audit.source_snapshot_epoch = host_->source_snapshot_epoch;
      lease_audit.catalog_buffer_epoch = receipt.buffer_epoch;
      lease_audit.closed_rank_catalog_complete = true;
      lease_audit.pair_support_relevant_gp_complete =
          receipt.pair_support_relevant_gp_closure.relevant_gp_complete;
      lease_audit.traversal_owner_retained =
          combined_owner->traversal_owner != nullptr;
      lease_audit.catalog_owner_retained =
          combined_owner->catalog_owner != nullptr;
      lease_audit.host_fake_lifecycle_exercised = true;
      lease_audit.morton_ownership_partition_validated =
          receipt.morton_ownership_partition_validated;
      lease_audit.yao48_filter_policy_validated =
          receipt.yao48_filter_policy_validated;
      lease_audit.candidate_point_id_canonicalization_validated =
          receipt.candidate_point_id_canonicalization_validated;
      lease_audit
          .candidate_deduplication_before_exact_classification_validated =
          receipt
              .candidate_deduplication_before_exact_classification_validated;
      lease_audit.morton_scientific_authority_claimed =
          receipt.morton_scientific_authority_claimed;
      lease_audit.raw_morton_pair_stream_materialized =
          receipt.raw_morton_pair_stream_materialized;

      RankedDiametralPairCatalogDeviceLease lease{
          lease_audit,
          std::move(combined_owner),
          receipt.output.support_u,
          receipt.output.support_v,
          receipt.output.closed_rank,
          receipt.output.rank_offsets,
          receipt.output.strict_offsets,
          receipt.output.strict_point_ids,
          receipt.output.shell_offsets,
          receipt.output.shell_point_ids,
          receipt.cuda_device,
          true};
      if (!lease.ready() || lease.cuda_resident() || !lease.host_fake()) {
        throw std::logic_error(
            "the ranked-pair host scaffold failed to close its fake lease");
      }
      catalog.emplace(std::move(lease));
    }

    build_attempted_ = true;
    RankedDiametralPairCatalogBuildAttempt result{
        validated.status,
        validated.stop_reason,
        receipt.closed_rank_closure,
        receipt.pair_support_relevant_gp_closure,
        validated.audit,
        std::move(catalog)};
    if (!result.validated_contract_attempt() ||
        result.scientific_outcome()) {
      throw std::logic_error(
          "the ranked-pair host scaffold did not close its non-scientific "
          "contract result");
    }
    return result;
  });
}

std::size_t RankedDiametralPairCatalogContext::point_count()
    const noexcept {
  return host_ == nullptr ? 0U : host_->point_count;
}

std::size_t RankedDiametralPairCatalogContext::certified_node_count()
    const noexcept {
  return host_ == nullptr ? 0U : host_->node_count;
}

const RankedDiametralPairCatalogBudget&
RankedDiametralPairCatalogContext::fixed_budget() const noexcept {
  return fixed_budget_;
}

}  // namespace morsehgp3d::gpu
