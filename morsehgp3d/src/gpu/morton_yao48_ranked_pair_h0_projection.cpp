#include "morsehgp3d/gpu/morton_yao48_ranked_pair_h0_projection.hpp"

#include "phase15_morton_yao48_ranked_pair_h0_projection_internal.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

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

[[nodiscard]] MortonYao48RankedPairH0ProjectionConfig validate_config(
    MortonYao48RankedPairH0ProjectionConfig config) {
  if (config.record_capacity_per_point == 0U ||
      config.record_capacity_per_point >
          morton_yao48_ranked_pair_h0_projection_maximum_linear_capacity_factor) {
    throw std::invalid_argument(
        "the ranked-pair H0 projection requires a nonzero bounded linear "
        "record-capacity factor");
  }
  return config;
}

void validate_source(
    const MortonYao48RankedPairTileCatalogLease& source,
    const detail::Phase15MortonYao48RankedPairTileCatalogPrivateViews&
        views) {
  const auto& audit = source.audit();
  if (!source.ready() || !views.ready ||
      audit.maximum_closed_rank <
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
      audit.maximum_closed_rank >
          morton_yao48_ranked_pair_h0_projection_maximum_closed_rank ||
      audit.exact_fallback_count != 0U ||
      views.point_count != audit.point_count ||
      views.catalog_record_count != audit.catalog_record_count ||
      views.strict_point_id_count !=
          audit.strict_payload_point_id_count ||
      views.shell_point_id_count != audit.shell_payload_point_id_count ||
      views.rank_offset_count != audit.maximum_closed_rank + 2U ||
      views.record_offset_count != audit.catalog_record_count + 1U ||
      views.host_fake != source.host_fake()) {
    throw std::invalid_argument(
        "the ranked-pair H0 projection requires a complete exact source "
        "catalog with rank at most six and zero fallback");
  }
  const bool record_views_ready =
      audit.catalog_record_count == 0U ||
      (views.device_support_u != nullptr &&
       views.device_support_v != nullptr &&
       views.device_closed_rank != nullptr);
  const bool strict_payload_ready =
      audit.strict_payload_point_id_count == 0U ||
      views.device_strict_point_ids != nullptr;
  const bool shell_payload_ready =
      audit.shell_payload_point_id_count == 0U ||
      views.device_shell_point_ids != nullptr;
  if (!record_views_ready || !strict_payload_ready ||
      !shell_payload_ready ||
      (audit.catalog_record_count != 0U &&
       (views.device_strict_offsets == nullptr ||
        views.device_shell_offsets == nullptr)) ||
      (source.cuda_resident() && views.device_coordinate_bits == nullptr)) {
    throw std::invalid_argument(
        "the ranked-pair H0 source omitted a required private resident "
        "view");
  }
}

void validate_receipt(
    const detail::Phase15MortonYao48RankedPairH0ProjectionReceipt& receipt,
    const detail::Phase15MortonYao48RankedPairH0ProjectionRequest& request) {
  const bool expected_host_fake = request.host_fake;
  const bool execution_matches =
      expected_host_fake
          ? receipt.execution_kind ==
                detail::
                    Phase15MortonYao48RankedPairH0ProjectionExecutionKind::
                        host_fake
          : receipt.execution_kind ==
                detail::
                    Phase15MortonYao48RankedPairH0ProjectionExecutionKind::
                        cuda;
  const bool output_shape_ready =
      receipt.projected_record_count == 0U ||
      (receipt.output_owner && receipt.records != nullptr);
  if (!execution_matches ||
      receipt.point_count != request.point_count ||
      receipt.maximum_closed_rank != request.maximum_closed_rank ||
      receipt.record_capacity != request.record_capacity ||
      receipt.source_record_count != request.record_count ||
      receipt.strict_point_id_count != request.strict_point_id_count ||
      receipt.shell_point_id_count != request.shell_point_id_count ||
      receipt.projected_record_count != request.record_count ||
      receipt.regular_pair_candidate_count >
          receipt.projected_record_count ||
      receipt.exact_level_unresolved_count >
          receipt.projected_record_count ||
      !receipt.fixed_linear_capacity_validated ||
      !receipt.one_record_per_source_validated ||
      !receipt.strict_and_shell_payload_ranges_validated ||
      !receipt.regular_pair_closed_rank_identity_validated ||
      !receipt.scientific_birth_and_saddle_roles_withheld ||
      !output_shape_ready ||
      receipt.intermediate_record_device_to_host_performed ||
      receipt.payload_device_to_host_performed ||
      receipt.callback_per_pair_performed ||
      receipt.dense_pair_scan_performed ||
      receipt.global_pair_matrix_materialized ||
      receipt.ordinary_delaunay_materialized ||
      receipt.higher_order_delaunay_mosaic_materialized ||
      receipt.scientific_decision_published ||
      receipt.public_status_claimed) {
    throw std::runtime_error(
        "the ranked-pair H0 projection launcher returned an invalid "
        "receipt");
  }
  if (receipt.failure_code ==
          detail::Phase15MortonYao48RankedPairH0ProjectionFailureCode::
              none &&
      (receipt.regular_pair_candidate_count +
                   receipt.diagnostic_or_carrier_candidate_count !=
               receipt.projected_record_count ||
       receipt.exact_level_unresolved_count != 0U ||
       !receipt.exact_level_payload_constructed)) {
    throw std::runtime_error(
        "the ranked-pair H0 projection launcher published an incomplete "
        "exact-level payload");
  }
  if (receipt.failure_code ==
          detail::Phase15MortonYao48RankedPairH0ProjectionFailureCode::
              exact_level_unresolved &&
      receipt.regular_pair_candidate_count +
              receipt.diagnostic_or_carrier_candidate_count +
              receipt.exact_level_unresolved_count !=
          receipt.projected_record_count) {
    throw std::runtime_error(
        "the ranked-pair H0 projection launcher lost an unresolved "
        "record");
  }
  if (!expected_host_fake &&
      receipt.failure_code ==
          detail::Phase15MortonYao48RankedPairH0ProjectionFailureCode::
              none) {
    const bool launch_counts_valid =
        request.record_count == 0U
            ? receipt.kernel_launch_count == 0U &&
                  receipt.synchronization_count == 0U &&
                  receipt.scalar_control_device_to_host_count == 0U
            : receipt.kernel_launch_count == 1U &&
                  receipt.synchronization_count == 1U &&
                  receipt.scalar_control_device_to_host_count == 1U;
    if (!receipt.exact_levels_computed_from_source_coordinates ||
        !launch_counts_valid ||
        receipt.cuda_device != request.cuda_device) {
      throw std::runtime_error(
          "the ranked-pair H0 CUDA projection did not authenticate its "
          "resident execution");
    }
  }
}

[[nodiscard]] MortonYao48RankedPairH0ProjectionAudit make_audit(
    const detail::Phase15MortonYao48RankedPairH0ProjectionReceipt& receipt,
    const detail::Phase15MortonYao48RankedPairH0ProjectionRequest& request) {
  MortonYao48RankedPairH0ProjectionAudit audit;
  audit.point_count = request.point_count;
  audit.maximum_closed_rank = request.maximum_closed_rank;
  audit.record_capacity_per_point =
      request.config.record_capacity_per_point;
  audit.record_capacity = request.record_capacity;
  audit.source_record_count = request.record_count;
  audit.projected_record_count = receipt.projected_record_count;
  audit.regular_pair_candidate_count =
      receipt.regular_pair_candidate_count;
  audit.diagnostic_or_carrier_candidate_count =
      receipt.diagnostic_or_carrier_candidate_count;
  audit.strict_payload_point_id_count = request.strict_point_id_count;
  audit.shell_payload_point_id_count = request.shell_point_id_count;
  audit.exact_level_unresolved_count =
      receipt.exact_level_unresolved_count;
  audit.cuda_kernel_launch_count = receipt.kernel_launch_count;
  audit.cuda_synchronization_count = receipt.synchronization_count;
  audit.scalar_control_device_to_host_count =
      receipt.scalar_control_device_to_host_count;
  audit.cuda_device = receipt.cuda_device;
  audit.fixed_linear_capacity_validated =
      receipt.fixed_linear_capacity_validated;
  audit.one_record_per_source_validated =
      receipt.one_record_per_source_validated;
  audit.exact_level_payload_constructed =
      receipt.exact_level_payload_constructed;
  audit.exact_levels_computed_from_source_coordinates =
      receipt.exact_levels_computed_from_source_coordinates;
  audit.strict_and_shell_payload_ranges_validated =
      receipt.strict_and_shell_payload_ranges_validated;
  audit.regular_pair_requires_empty_extra_shell = true;
  audit.regular_pair_closed_rank_identity_validated =
      receipt.regular_pair_closed_rank_identity_validated;
  audit.scientific_birth_and_saddle_roles_withheld =
      receipt.scientific_birth_and_saddle_roles_withheld;
  audit.exact_center_derivable_from_retained_support_coordinates =
      !request.host_fake &&
      receipt.exact_levels_computed_from_source_coordinates;
  audit.sparse_direct_h0_payload_adapter_prerequisites_retained =
      audit.exact_center_derivable_from_retained_support_coordinates &&
      audit.exact_level_payload_constructed &&
      audit.regular_pair_closed_rank_identity_validated &&
      audit.scientific_birth_and_saddle_roles_withheld;
  // The source order is (rank, support_u, support_v), whereas the merge
  // contract starts with exact level.  A future resident radix adapter must
  // sort before constructing an input run.
  audit.sorted_by_sparse_direct_h0_terminal_key = false;
  audit.host_fake_lifecycle_exercised = request.host_fake;
  audit.cuda_execution_performed = !request.host_fake;
  audit.cuda_device_storage_retained =
      !request.host_fake &&
      (receipt.projected_record_count == 0U ||
       receipt.output_owner != nullptr);
  audit.intermediate_record_device_to_host_performed =
      receipt.intermediate_record_device_to_host_performed;
  audit.payload_device_to_host_performed =
      receipt.payload_device_to_host_performed;
  audit.callback_per_pair_performed = receipt.callback_per_pair_performed;
  audit.dense_pair_scan_performed = receipt.dense_pair_scan_performed;
  audit.global_pair_matrix_materialized =
      receipt.global_pair_matrix_materialized;
  audit.ordinary_delaunay_materialized =
      receipt.ordinary_delaunay_materialized;
  audit.higher_order_delaunay_mosaic_materialized =
      receipt.higher_order_delaunay_mosaic_materialized;
  return audit;
}

}  // namespace

MortonYao48RankedPairH0ProjectionLease::
    MortonYao48RankedPairH0ProjectionLease(
        MortonYao48RankedPairH0ProjectionAudit audit,
        std::shared_ptr<void> output_owner,
        std::shared_ptr<MortonYao48RankedPairTileCatalogLease> source_catalog,
        const void* device_records,
        int cuda_device,
        bool host_fake)
    : audit_(audit),
      output_owner_(std::move(output_owner)),
      source_catalog_(std::move(source_catalog)),
      device_records_(device_records),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool MortonYao48RankedPairH0ProjectionLease::ready() const noexcept {
  const bool output_ready =
      audit_.projected_record_count == 0U ||
      (output_owner_ && device_records_ != nullptr);
  const bool common_ready =
      audit_.schema_version ==
          morton_yao48_ranked_pair_h0_projection_schema_version &&
      source_catalog_ && source_catalog_->ready() &&
      audit_.source_catalog_consumed && audit_.source_catalog_retained &&
      audit_.source_identity_retained &&
      audit_.maximum_closed_rank >=
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank &&
      audit_.maximum_closed_rank <=
          morton_yao48_ranked_pair_h0_projection_maximum_closed_rank &&
      audit_.source_record_count == audit_.projected_record_count &&
      audit_.regular_pair_candidate_count <=
          audit_.projected_record_count &&
      audit_.diagnostic_or_carrier_candidate_count ==
          audit_.projected_record_count -
              audit_.regular_pair_candidate_count &&
      audit_.exact_level_unresolved_count == 0U &&
      audit_.fixed_linear_capacity_validated &&
      audit_.one_record_per_source_validated &&
      audit_.exact_level_payload_constructed &&
      audit_.strict_and_shell_payload_ranges_validated &&
      audit_.regular_pair_requires_empty_extra_shell &&
      audit_.regular_pair_closed_rank_identity_validated &&
      audit_.scientific_birth_and_saddle_roles_withheld &&
      !audit_.sorted_by_sparse_direct_h0_terminal_key && output_ready &&
      !audit_.intermediate_record_device_to_host_performed &&
      !audit_.payload_device_to_host_performed &&
      !audit_.callback_per_pair_performed &&
      !audit_.dense_pair_scan_performed &&
      !audit_.global_pair_matrix_materialized &&
      !audit_.ordinary_delaunay_materialized &&
      !audit_.higher_order_delaunay_mosaic_materialized &&
      !audit_.support3_complete_claimed &&
      !audit_.support4_complete_claimed &&
      !audit_.gamma2_complete_claimed &&
      !audit_.hierarchy_complete_claimed &&
      !audit_.latency_100ms_claimed && !audit_.public_status_claimed;
  if (!common_ready) {
    return false;
  }
  return host_fake_
             ? audit_.host_fake_lifecycle_exercised &&
                   !audit_.cuda_execution_performed &&
                   !audit_.cuda_device_storage_retained &&
                   !audit_.exact_levels_computed_from_source_coordinates &&
                   !audit_.
                       exact_center_derivable_from_retained_support_coordinates &&
                   !audit_.
                       sparse_direct_h0_payload_adapter_prerequisites_retained &&
                   cuda_device_ == -1
             : !audit_.host_fake_lifecycle_exercised &&
                   audit_.cuda_execution_performed &&
                   audit_.cuda_device_storage_retained &&
                   audit_.exact_levels_computed_from_source_coordinates &&
                   audit_.
                       exact_center_derivable_from_retained_support_coordinates &&
                   audit_.
                       sparse_direct_h0_payload_adapter_prerequisites_retained &&
                   cuda_device_ >= 0;
}

bool MortonYao48RankedPairH0ProjectionLease::cuda_resident()
    const noexcept {
  return ready() && !host_fake_;
}

bool MortonYao48RankedPairH0ProjectionLease::host_fake() const noexcept {
  return ready() && host_fake_;
}

detail::Phase15MortonYao48RankedPairH0ProjectionPrivateViews
detail::Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess::inspect(
    const MortonYao48RankedPairH0ProjectionLease& lease) noexcept {
  Phase15MortonYao48RankedPairH0ProjectionPrivateViews views;
  views.output_owner = lease.output_owner_;
  views.source_catalog = lease.source_catalog_;
  views.device_records =
      static_cast<const Phase15MortonYao48RankedPairH0Record*>(
          lease.device_records_);
  views.point_count = lease.audit_.point_count;
  views.record_count = lease.audit_.projected_record_count;
  views.strict_point_id_count =
      lease.audit_.strict_payload_point_id_count;
  views.shell_point_id_count =
      lease.audit_.shell_payload_point_id_count;
  views.cuda_device = lease.cuda_device_;
  views.host_fake = lease.host_fake_;
  views.ready = lease.ready();
  if (lease.source_catalog_) {
    const auto source_views =
        Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess::inspect(
            *lease.source_catalog_);
    views.device_coordinate_bits = source_views.device_coordinate_bits;
    views.device_strict_point_ids = source_views.device_strict_point_ids;
    views.device_shell_point_ids = source_views.device_shell_point_ids;
  }
  return views;
}

MortonYao48RankedPairH0ProjectionResult
project_morton_yao48_ranked_pair_h0_candidates(
    MortonYao48RankedPairTileCatalogLease&& source_catalog,
    MortonYao48RankedPairH0ProjectionConfig config) {
  config = validate_config(config);
  const auto source_views =
      detail::Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess::
          inspect(source_catalog);
  validate_source(source_catalog, source_views);
  const auto& source_audit = source_catalog.audit();
  const std::size_t record_capacity = checked_product(
      source_audit.point_count, config.record_capacity_per_point,
      "the ranked-pair H0 projection record capacity overflows size_t");

  MortonYao48RankedPairH0ProjectionResult result;
  result.audit.point_count = source_audit.point_count;
  result.audit.maximum_closed_rank = source_audit.maximum_closed_rank;
  result.audit.record_capacity_per_point =
      config.record_capacity_per_point;
  result.audit.record_capacity = record_capacity;
  result.audit.source_record_count = source_audit.catalog_record_count;
  result.audit.strict_payload_point_id_count =
      source_audit.strict_payload_point_id_count;
  result.audit.shell_payload_point_id_count =
      source_audit.shell_payload_point_id_count;
  if (source_audit.catalog_record_count > record_capacity) {
    result.status =
        MortonYao48RankedPairH0ProjectionStatus::capacity_exhausted;
    return result;
  }

  detail::Phase15MortonYao48RankedPairH0ProjectionRequest request;
  request.config = config;
  request.coordinate_bits = source_views.device_coordinate_bits;
  request.support_u = source_views.device_support_u;
  request.support_v = source_views.device_support_v;
  request.closed_rank = source_views.device_closed_rank;
  request.strict_offsets = source_views.device_strict_offsets;
  request.strict_point_ids = source_views.device_strict_point_ids;
  request.shell_offsets = source_views.device_shell_offsets;
  request.shell_point_ids = source_views.device_shell_point_ids;
  request.point_count = source_audit.point_count;
  request.maximum_closed_rank = source_audit.maximum_closed_rank;
  request.record_count = source_audit.catalog_record_count;
  request.strict_point_id_count =
      source_audit.strict_payload_point_id_count;
  request.shell_point_id_count =
      source_audit.shell_payload_point_id_count;
  request.record_capacity = record_capacity;
  request.cuda_device = source_views.cuda_device;
  request.host_fake = source_views.host_fake;

  auto receipt =
      detail::phase15_launch_morton_yao48_ranked_pair_h0_projection(
          request);
  validate_receipt(receipt, request);
  result.audit = make_audit(receipt, request);
  if (receipt.failure_code ==
      detail::Phase15MortonYao48RankedPairH0ProjectionFailureCode::
          exact_level_unresolved) {
    result.status =
        MortonYao48RankedPairH0ProjectionStatus::exact_level_unresolved;
    return result;
  }
  if (receipt.failure_code !=
      detail::Phase15MortonYao48RankedPairH0ProjectionFailureCode::none) {
    throw std::runtime_error(
        "the ranked-pair H0 projection failed closed on malformed input");
  }

  auto retained_source =
      std::make_shared<MortonYao48RankedPairTileCatalogLease>(
          std::move(source_catalog));
  result.audit.source_catalog_consumed = true;
  result.audit.source_catalog_retained = true;
  result.audit.source_identity_retained =
      retained_source->audit().source_cloud_identity_retained;
  MortonYao48RankedPairH0ProjectionLease projection{
      result.audit,
      std::move(receipt.output_owner),
      std::move(retained_source),
      receipt.records,
      receipt.cuda_device,
      request.host_fake};
  result.status =
      request.host_fake
          ? MortonYao48RankedPairH0ProjectionStatus::
                non_authoritative_host_fake
          : MortonYao48RankedPairH0ProjectionStatus::
                complete_resident_projection;
  result.projection.emplace(std::move(projection));
  if (!result.projection->ready()) {
    throw std::runtime_error(
        "the ranked-pair H0 projection did not retain a valid output "
        "lease");
  }
  return result;
}

}  // namespace morsehgp3d::gpu
