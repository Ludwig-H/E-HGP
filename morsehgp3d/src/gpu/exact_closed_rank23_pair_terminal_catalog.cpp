#include "morsehgp3d/gpu/exact_closed_rank23_pair_terminal_catalog.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] ExactClosedRank23PairTerminalCatalogConfig validate_config(
    ExactClosedRank23PairTerminalCatalogConfig config) {
  if (config.anchor_tile_capacity == 0U ||
      config.anchor_tile_capacity >
          morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity) {
    throw std::out_of_range(
        "the exact closed-rank 2/3 terminal wrapper requires a nonzero "
        "supported anchor tile capacity");
  }
  const auto factor_supported = [](std::size_t factor) {
    return factor <=
           exact_closed_rank23_pair_terminal_catalog_maximum_linear_capacity_factor;
  };
  if (!factor_supported(config.catalog_record_capacity_per_point) ||
      !factor_supported(config.payload_point_id_capacity_per_point) ||
      !factor_supported(config.exact_fallback_capacity_per_point)) {
    throw std::out_of_range(
        "an exact closed-rank 2/3 arena factor exceeds the fixed linear "
        "capacity ceiling");
  }
  return config;
}

[[nodiscard]] bool checked_linear_capacity(
    std::size_t point_count,
    std::size_t factor,
    std::size_t& output) noexcept {
  if (factor != 0U &&
      point_count > std::numeric_limits<std::size_t>::max() / factor) {
    output = 0U;
    return false;
  }
  output = point_count * factor;
  return true;
}

[[nodiscard]] bool reconciles_three_way_partition(
    std::uint64_t first,
    std::uint64_t second,
    std::uint64_t third,
    std::uint64_t universe) noexcept {
  if (first > universe) {
    return false;
  }
  const std::uint64_t after_first = universe - first;
  if (second > after_first) {
    return false;
  }
  return third == after_first - second;
}

[[nodiscard]] ExactClosedRank23PairTerminalCatalogResult failed_result(
    ExactClosedRank23PairTerminalCatalogStatus status,
    ExactClosedRank23PairTerminalCatalogAudit audit) {
  audit.closed_rank23_pair_catalog_complete = false;
  audit.output_withheld = true;
  ExactClosedRank23PairTerminalCatalogResult result;
  result.status = status;
  result.audit = std::move(audit);
  return result;
}

void absorb_frontier_audit(
    ExactClosedRank23PairTerminalCatalogAudit& output,
    const MortonYao48DeviceTiledPairFrontierAdvance& advance) noexcept {
  output.last_frontier_status = advance.status;
  output.frontier_stop_reason = advance.stop_reason;
  output.candidate_count = advance.audit.cumulative_candidate_pair_mass;
  output.certified_pruned_pair_mass =
      advance.audit.cumulative_certified_pruned_pair_mass;
  output.frontier_unresolved_pair_mass = advance.audit.unresolved_pair_mass;
  output.unordered_pair_universe_count =
      advance.audit.unordered_pair_universe_count;
  output.frontier_mass_reconciled = reconciles_three_way_partition(
      output.candidate_count,
      output.certified_pruned_pair_mass,
      output.frontier_unresolved_pair_mass,
      output.unordered_pair_universe_count);
  output.candidate_device_to_host_performed =
      output.candidate_device_to_host_performed ||
      advance.audit.candidate_device_to_host_performed;
  output.certified_prune_device_to_host_performed =
      output.certified_prune_device_to_host_performed ||
      advance.audit.certified_prune_device_to_host_performed;
  output.dense_pair_scan_performed =
      output.dense_pair_scan_performed ||
      advance.audit.dense_pair_fallback_performed;
  output.global_pair_matrix_materialized =
      output.global_pair_matrix_materialized ||
      advance.audit.global_pair_matrix_materialized;
  output.higher_order_delaunay_mosaic_materialized =
      output.higher_order_delaunay_mosaic_materialized ||
      advance.audit.higher_order_delaunay_mosaic_materialized;
  output.host_fake_lifecycle_exercised =
      output.host_fake_lifecycle_exercised ||
      advance.audit.host_fake_launcher_exercised;
  output.cuda_execution_performed =
      output.cuda_execution_performed ||
      advance.audit.cuda_execution_performed;
}

void absorb_classifier_audit(
    ExactClosedRank23PairTerminalCatalogAudit& output,
    const MortonYao48RankedPairTileClassifierCommit& commit) noexcept {
  output.last_classifier_status = commit.status;
  output.classifier_stop_reason = commit.stop_reason;
  output.committed_chunk_count = commit.audit.commit_sequence;
  output.candidate_count = commit.audit.cumulative_candidate_count;
  output.catalog_record_count = static_cast<std::size_t>(
      commit.audit.cumulative_accepted_record_count);
  output.above_window_count = commit.audit.cumulative_above_window_count;
  output.unresolved_count = commit.audit.cumulative_unresolved_count;
  output.exact_fallback_count =
      commit.audit.cumulative_exact_fallback_count;
  output.strict_payload_point_id_count = static_cast<std::size_t>(
      commit.audit.cumulative_strict_payload_point_id_count);
  output.shell_payload_point_id_count = static_cast<std::size_t>(
      commit.audit.cumulative_shell_payload_point_id_count);
  output.certified_pruned_pair_mass =
      commit.audit.cumulative_certified_pruned_pair_mass;
  output.frontier_unresolved_pair_mass =
      commit.audit.frontier_unresolved_pair_mass;
  output.unordered_pair_universe_count =
      commit.audit.unordered_pair_universe_count;
  output.classifier_mass_reconciled = reconciles_three_way_partition(
      commit.audit.cumulative_accepted_record_count,
      output.above_window_count,
      output.unresolved_count,
      output.candidate_count);
  output.frontier_mass_reconciled = reconciles_three_way_partition(
      output.candidate_count,
      output.certified_pruned_pair_mass,
      output.frontier_unresolved_pair_mass,
      output.unordered_pair_universe_count);
  output.candidate_device_to_host_performed =
      output.candidate_device_to_host_performed ||
      commit.audit.candidate_device_to_host_performed;
  output.certified_prune_device_to_host_performed =
      output.certified_prune_device_to_host_performed ||
      commit.audit.certified_prune_device_to_host_performed;
  output.callback_per_pair_performed =
      output.callback_per_pair_performed ||
      commit.audit.callback_per_pair_performed;
  output.dense_pair_scan_performed =
      output.dense_pair_scan_performed ||
      commit.audit.dense_pair_scan_performed;
  output.global_pair_matrix_materialized =
      output.global_pair_matrix_materialized ||
      commit.audit.global_pair_matrix_materialized;
  output.ordinary_delaunay_materialized =
      output.ordinary_delaunay_materialized ||
      commit.audit.ordinary_delaunay_materialized;
  output.higher_order_delaunay_mosaic_materialized =
      output.higher_order_delaunay_mosaic_materialized ||
      commit.audit.higher_order_delaunay_mosaic_materialized;
  output.host_fake_lifecycle_exercised =
      output.host_fake_lifecycle_exercised ||
      commit.audit.host_fake_launcher_exercised;
  output.cuda_execution_performed =
      output.cuda_execution_performed ||
      commit.audit.cuda_execution_performed;
}

[[nodiscard]] bool forbidden_work_observed(
    const ExactClosedRank23PairTerminalCatalogAudit& audit) noexcept {
  return audit.candidate_device_to_host_performed ||
         audit.certified_prune_device_to_host_performed ||
         audit.callback_per_pair_performed || audit.dense_pair_scan_performed ||
         audit.global_pair_matrix_materialized ||
         audit.ordinary_delaunay_materialized ||
         audit.higher_order_delaunay_mosaic_materialized;
}

}  // namespace

ExactClosedRank23PairTerminalCatalog::
    ExactClosedRank23PairTerminalCatalog(
        ExactClosedRank23PairTerminalCatalogAudit audit,
        MortonYao48RankedPairTileCatalogLease&& resident_catalog)
    : audit_(std::move(audit)),
      resident_catalog_(std::move(resident_catalog)) {}

bool ExactClosedRank23PairTerminalCatalog::ready() const noexcept {
  return resident_catalog_.ready() && !resident_catalog_.host_fake() &&
         audit_.schema_version ==
             exact_closed_rank23_pair_terminal_catalog_schema_version &&
         audit_.maximum_closed_rank ==
             exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank &&
         audit_.fixed_rank23_window_enforced &&
         audit_.fixed_linear_capacities_validated && audit_.frontier_closed &&
         audit_.frontier_mass_reconciled &&
         audit_.classifier_mass_reconciled &&
         audit_.zero_unresolved_certified &&
         audit_.zero_exact_fallback_certified &&
         audit_.closed_rank23_pair_catalog_complete &&
         !audit_.output_withheld &&
         !audit_.host_fake_lifecycle_exercised &&
         !forbidden_work_observed(audit_) &&
         !audit_.gamma2_complete_claimed && !audit_.k2_complete_claimed &&
         !audit_.hierarchy_complete_claimed &&
         !audit_.latency_100ms_claimed && !audit_.public_status_claimed;
}

bool ExactClosedRank23PairTerminalCatalog::cuda_resident() const noexcept {
  return ready() && resident_catalog_.cuda_resident();
}

bool ExactClosedRank23PairTerminalCatalog::host_fake() const noexcept {
  return ready() && resident_catalog_.host_fake();
}

ExactClosedRank23PairTerminalCatalogResult
build_exact_closed_rank23_pair_terminal_catalog(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    ExactClosedRank23PairTerminalCatalogConfig config) {
  config = validate_config(config);
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "the exact closed-rank 2/3 terminal wrapper requires a ready "
        "traversal lease");
  }

  ExactClosedRank23PairTerminalCatalogAudit audit;
  audit.point_count = traversal_lease.audit().point_count;
  audit.maximum_closed_rank =
      exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank;
  audit.anchor_tile_capacity = config.anchor_tile_capacity;
  audit.catalog_record_capacity_per_point =
      config.catalog_record_capacity_per_point;
  audit.payload_point_id_capacity_per_point =
      config.payload_point_id_capacity_per_point;
  audit.exact_fallback_capacity_per_point =
      config.exact_fallback_capacity_per_point;
  audit.fixed_rank23_window_enforced = true;

  const bool capacities_fit =
      checked_linear_capacity(
          audit.point_count,
          config.catalog_record_capacity_per_point,
          audit.catalog_record_capacity) &&
      checked_linear_capacity(
          audit.point_count,
          config.payload_point_id_capacity_per_point,
          audit.payload_point_id_capacity) &&
      checked_linear_capacity(
          audit.point_count,
          config.exact_fallback_capacity_per_point,
          audit.exact_fallback_capacity);
  if (!capacities_fit) {
    return failed_result(
        ExactClosedRank23PairTerminalCatalogStatus::capacity_exhausted,
        std::move(audit));
  }
  audit.fixed_linear_capacities_validated = true;

  MortonYao48DeviceTiledPairFrontierContext frontier{
      std::move(traversal_lease),
      MortonYao48DeviceTiledPairFrontierConfig{
          exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank,
          config.anchor_tile_capacity}};
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{
          exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank,
          audit.catalog_record_capacity,
          audit.payload_point_id_capacity,
          audit.exact_fallback_capacity}};

  for (;;) {
    MortonYao48DeviceTiledPairFrontierAdvance advance = frontier.advance();
    ++audit.frontier_advance_count;
    absorb_frontier_audit(audit, advance);
    if (advance.status ==
        MortonYao48DeviceTiledPairFrontierStatus::censored) {
      return failed_result(
          ExactClosedRank23PairTerminalCatalogStatus::frontier_censored,
          std::move(audit));
    }
    if (!audit.frontier_mass_reconciled || forbidden_work_observed(audit)) {
      return failed_result(
          ExactClosedRank23PairTerminalCatalogStatus::frontier_censored,
          std::move(audit));
    }

    MortonYao48RankedPairTileClassifierCommit commit =
        classifier.commit(std::move(advance));
    ++audit.classifier_commit_count;
    absorb_classifier_audit(audit, commit);
    if (commit.status ==
        MortonYao48RankedPairTileClassifierStatus::capacity_exhausted) {
      return failed_result(
          ExactClosedRank23PairTerminalCatalogStatus::capacity_exhausted,
          std::move(audit));
    }
    if (commit.status ==
            MortonYao48RankedPairTileClassifierStatus::
                exact_predicate_failure ||
        audit.unresolved_count != 0U || audit.exact_fallback_count != 0U) {
      return failed_result(
          ExactClosedRank23PairTerminalCatalogStatus::
              exact_predicate_unresolved,
          std::move(audit));
    }
    if (!audit.classifier_mass_reconciled ||
        !audit.frontier_mass_reconciled || forbidden_work_observed(audit)) {
      return failed_result(
          ExactClosedRank23PairTerminalCatalogStatus::
              exact_predicate_unresolved,
          std::move(audit));
    }
    if (commit.status ==
        MortonYao48RankedPairTileClassifierStatus::frontier_closed) {
      audit.frontier_closed = true;
      break;
    }
    if (commit.status !=
        MortonYao48RankedPairTileClassifierStatus::chunk_committed) {
      throw std::runtime_error(
          "the exact closed-rank 2/3 terminal wrapper observed an unknown "
          "classifier status");
    }
  }

  MortonYao48RankedPairTileCatalogLease resident_catalog =
      classifier.finish();
  const MortonYao48RankedPairTileCatalogLeaseAudit& final =
      resident_catalog.audit();
  audit.catalog_record_count = final.catalog_record_count;
  audit.strict_payload_point_id_count =
      final.strict_payload_point_id_count;
  audit.shell_payload_point_id_count = final.shell_payload_point_id_count;
  audit.candidate_count = final.candidate_count;
  audit.above_window_count = final.above_window_count;
  audit.unresolved_count = final.unresolved_count;
  audit.exact_fallback_count = final.exact_fallback_count;
  audit.certified_pruned_pair_mass = final.certified_pruned_pair_mass;
  audit.frontier_unresolved_pair_mass = final.frontier_unresolved_pair_mass;
  audit.unordered_pair_universe_count = final.unordered_pair_universe_count;
  audit.committed_chunk_count = final.committed_chunk_count;
  audit.frontier_closed = final.frontier_closed;
  audit.classifier_mass_reconciled = reconciles_three_way_partition(
      static_cast<std::uint64_t>(audit.catalog_record_count),
      audit.above_window_count,
      audit.unresolved_count,
      audit.candidate_count);
  audit.frontier_mass_reconciled = reconciles_three_way_partition(
      audit.candidate_count,
      audit.certified_pruned_pair_mass,
      audit.frontier_unresolved_pair_mass,
      audit.unordered_pair_universe_count);
  audit.zero_unresolved_certified = audit.unresolved_count == 0U;
  audit.zero_exact_fallback_certified = audit.exact_fallback_count == 0U;
  audit.host_fake_lifecycle_exercised =
      audit.host_fake_lifecycle_exercised ||
      final.host_fake_lifecycle_exercised;
  audit.cuda_execution_performed =
      audit.cuda_execution_performed || final.cuda_device_storage_retained;
  audit.candidate_device_to_host_performed =
      audit.candidate_device_to_host_performed ||
      final.intermediate_candidate_device_to_host_performed;
  audit.certified_prune_device_to_host_performed =
      audit.certified_prune_device_to_host_performed ||
      final.certified_prune_device_to_host_performed;
  audit.callback_per_pair_performed =
      audit.callback_per_pair_performed || final.callback_per_pair_performed;
  audit.dense_pair_scan_performed =
      audit.dense_pair_scan_performed || final.dense_pair_scan_performed;
  audit.global_pair_matrix_materialized =
      audit.global_pair_matrix_materialized ||
      final.global_pair_matrix_materialized;
  audit.ordinary_delaunay_materialized =
      audit.ordinary_delaunay_materialized ||
      final.ordinary_delaunay_materialized;
  audit.higher_order_delaunay_mosaic_materialized =
      audit.higher_order_delaunay_mosaic_materialized ||
      final.higher_order_delaunay_mosaic_materialized;

  if (resident_catalog.host_fake() ||
      audit.host_fake_lifecycle_exercised) {
    return failed_result(
        ExactClosedRank23PairTerminalCatalogStatus::
            non_authoritative_host_fake,
        std::move(audit));
  }

  const bool final_exact =
      resident_catalog.ready() &&
      final.maximum_closed_rank ==
          exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank &&
      final.closed_rank_catalog_complete && audit.frontier_closed &&
      audit.frontier_mass_reconciled && audit.classifier_mass_reconciled &&
      audit.zero_unresolved_certified &&
      audit.zero_exact_fallback_certified && !forbidden_work_observed(audit);
  if (!final_exact) {
    return failed_result(
        audit.frontier_closed && audit.frontier_mass_reconciled
            ? ExactClosedRank23PairTerminalCatalogStatus::
                  exact_predicate_unresolved
            : ExactClosedRank23PairTerminalCatalogStatus::frontier_censored,
        std::move(audit));
  }

  audit.closed_rank23_pair_catalog_complete = true;
  audit.output_withheld = false;
  ExactClosedRank23PairTerminalCatalogResult result;
  result.status = ExactClosedRank23PairTerminalCatalogStatus::
      complete_exact_closed_rank23;
  result.audit = audit;
  result.catalog = ExactClosedRank23PairTerminalCatalog{
      audit, std::move(resident_catalog)};
  return result;
}

}  // namespace morsehgp3d::gpu
