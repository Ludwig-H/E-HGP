#include "morsehgp3d/gpu/morton_yao48_tiled_pair_frontier.hpp"

#include <algorithm>
#include <exception>
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

[[nodiscard]] std::uint64_t checked_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_size_u64(
    std::size_t value,
    const char* message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] MortonYao48TiledPairFrontierConfig validate_config(
    const MortonLbvhDeviceTraversalLease& traversal_lease,
    MortonYao48TiledPairFrontierConfig config) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a tiled Morton/Yao48 frontier requires a ready traversal lease");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          morton_yao48_pair_frontier_maximum_closed_rank) {
    throw std::out_of_range(
        "a tiled Morton/Yao48 maximum closed rank must be in [2, 11]");
  }
  if (config.anchor_tile_capacity == 0U) {
    throw std::invalid_argument(
        "a tiled Morton/Yao48 frontier requires a nonzero anchor tile");
  }
  return config;
}

[[nodiscard]] std::size_t checked_remaining(
    std::size_t capacity,
    std::uint64_t used,
    const char* message) {
  if (used > static_cast<std::uint64_t>(capacity)) {
    throw std::logic_error(message);
  }
  return capacity - static_cast<std::size_t>(used);
}

[[nodiscard]] MortonYao48TiledPairFrontierStopReason map_stop_reason(
    MortonYao48PairFrontierStopReason reason) {
  switch (reason) {
    case MortonYao48PairFrontierStopReason::none:
      return MortonYao48TiledPairFrontierStopReason::none;
    case MortonYao48PairFrontierStopReason::anchor_completion_capacity:
      return MortonYao48TiledPairFrontierStopReason::anchor_tile_boundary;
    case MortonYao48PairFrontierStopReason::node_visit_capacity:
      return MortonYao48TiledPairFrontierStopReason::node_visit_budget;
    case MortonYao48PairFrontierStopReason::candidate_capacity:
      return MortonYao48TiledPairFrontierStopReason::candidate_budget;
    case MortonYao48PairFrontierStopReason::
        witness_bank_receipt_capacity:
      return MortonYao48TiledPairFrontierStopReason::
          witness_bank_receipt_budget;
    case MortonYao48PairFrontierStopReason::
        certified_prune_region_capacity:
      return MortonYao48TiledPairFrontierStopReason::
          certified_prune_region_budget;
  }
  throw std::logic_error(
      "a tiled Morton/Yao48 frontier received an unknown stop reason");
}

[[nodiscard]] std::uint64_t prune_pair_mass(
    const std::vector<MortonYao48CertifiedPruneRegion>& regions) {
  std::uint64_t mass = 0U;
  for (const MortonYao48CertifiedPruneRegion& region : regions) {
    mass = checked_sum(
        mass,
        region.certified_pair_mass,
        "a tiled Morton/Yao48 transaction prune mass overflows uint64");
  }
  return mass;
}

// Once the underlying cursor may have advanced, every exceptional exit is
// fail-stop.  This prevents a caller from resuming with last_frontier_audit_
// lagging behind the authoritative cursor.
class PoisonOnExceptionalExit final {
 public:
  explicit PoisonOnExceptionalExit(bool& poisoned) noexcept
      : poisoned_(poisoned), exception_count_(std::uncaught_exceptions()) {}

  ~PoisonOnExceptionalExit() noexcept {
    if (armed_ && std::uncaught_exceptions() > exception_count_) {
      poisoned_ = true;
    }
  }

  PoisonOnExceptionalExit(const PoisonOnExceptionalExit&) = delete;
  PoisonOnExceptionalExit& operator=(
      const PoisonOnExceptionalExit&) = delete;

  void release() noexcept { armed_ = false; }

 private:
  bool& poisoned_;
  int exception_count_{};
  bool armed_{true};
};

}  // namespace

MortonYao48TiledPairFrontierContext::
    MortonYao48TiledPairFrontierContext(
        MortonLbvhDeviceTraversalLease&& traversal_lease,
        MortonYao48TiledPairFrontierConfig config)
    : config_(validate_config(traversal_lease, config)),
      point_count_(traversal_lease.audit().point_count),
      maximum_node_visit_count_(checked_product(
          point_count_,
          config_.maximum_node_visits_per_point,
          "the tiled Morton/Yao48 node-visit budget overflows size_t")),
      maximum_candidate_count_(checked_product(
          point_count_,
          config_.maximum_candidates_per_point,
          "the tiled Morton/Yao48 candidate budget overflows size_t")),
      maximum_witness_bank_receipt_count_(checked_product(
          point_count_,
          config_.maximum_witness_bank_receipts_per_point,
          "the tiled Morton/Yao48 receipt budget overflows size_t")),
      maximum_certified_prune_region_count_(checked_product(
          point_count_,
          config_.maximum_certified_prune_regions_per_point,
          "the tiled Morton/Yao48 prune-region budget overflows size_t")),
      frontier_(
          std::move(traversal_lease),
          MortonYao48PairFrontierConfig{config_.maximum_closed_rank}) {}

bool MortonYao48TiledPairFrontierContext::ready() const noexcept {
  return frontier_.ready() && !poisoned_;
}

MortonYao48TiledPairFrontierAudit
MortonYao48TiledPairFrontierContext::make_audit(
    const MortonYao48PairFrontierAudit& before,
    const MortonYao48PairFrontierAudit& after,
    std::uint64_t transaction_candidate_mass,
    std::uint64_t transaction_pruned_mass,
    std::size_t transaction_receipt_count,
    std::size_t transaction_prune_region_count) const {
  if (after.completed_anchor_count < before.completed_anchor_count ||
      after.cumulative_node_visit_count <
          before.cumulative_node_visit_count ||
      after.cumulative_candidate_pair_mass <
          before.cumulative_candidate_pair_mass ||
      after.cumulative_certified_pruned_pair_mass <
          before.cumulative_certified_pruned_pair_mass ||
      after.cumulative_witness_bank_receipt_count <
          before.cumulative_witness_bank_receipt_count ||
      after.cumulative_certified_prune_region_count <
          before.cumulative_certified_prune_region_count) {
    throw std::logic_error(
        "a tiled Morton/Yao48 cumulative transcript moved backwards");
  }

  const std::uint64_t candidate_delta =
      after.cumulative_candidate_pair_mass -
      before.cumulative_candidate_pair_mass;
  const std::uint64_t pruned_delta =
      after.cumulative_certified_pruned_pair_mass -
      before.cumulative_certified_pruned_pair_mass;
  const std::uint64_t receipt_delta =
      after.cumulative_witness_bank_receipt_count -
      before.cumulative_witness_bank_receipt_count;
  const std::uint64_t prune_region_delta =
      after.cumulative_certified_prune_region_count -
      before.cumulative_certified_prune_region_count;
  const std::uint64_t node_visit_delta =
      after.cumulative_node_visit_count -
      before.cumulative_node_visit_count;
  const bool transaction_matches =
      candidate_delta == transaction_candidate_mass &&
      pruned_delta == transaction_pruned_mass &&
      receipt_delta == checked_size_u64(
          transaction_receipt_count,
          "a tiled Morton/Yao48 receipt count does not fit uint64") &&
      prune_region_delta == checked_size_u64(
          transaction_prune_region_count,
          "a tiled Morton/Yao48 prune count does not fit uint64");
  if (!transaction_matches) {
    throw std::logic_error(
        "a tiled Morton/Yao48 transaction does not match its mass deltas");
  }

  const std::uint64_t accounted = checked_sum(
      checked_sum(
          after.cumulative_candidate_pair_mass,
          after.cumulative_certified_pruned_pair_mass,
          "the tiled Morton/Yao48 processed mass overflows uint64"),
      after.unresolved_pair_mass,
      "the tiled Morton/Yao48 partition mass overflows uint64");
  const std::size_t completed_anchor_delta =
      after.completed_anchor_count - before.completed_anchor_count;
  if (completed_anchor_delta > config_.anchor_tile_capacity) {
    throw std::logic_error(
        "a tiled Morton/Yao48 advance crossed its anchor boundary");
  }

  MortonYao48TiledPairFrontierAudit audit;
  audit.advance_sequence = advance_sequence_;
  audit.source_snapshot_epoch = after.source_snapshot_epoch;
  audit.point_count = point_count_;
  audit.maximum_closed_rank = config_.maximum_closed_rank;
  audit.anchor_tile_capacity = config_.anchor_tile_capacity;
  audit.transaction_anchor_begin = before.next_anchor_position;
  audit.transaction_anchor_end = after.next_anchor_position;
  audit.transaction_completed_anchor_count = completed_anchor_delta;
  audit.transaction_node_visit_count = node_visit_delta;
  audit.transaction_candidate_pair_mass = transaction_candidate_mass;
  audit.transaction_certified_pruned_pair_mass =
      transaction_pruned_mass;
  audit.transaction_witness_bank_receipt_count =
      transaction_receipt_count;
  audit.transaction_certified_prune_region_count =
      transaction_prune_region_count;
  audit.unordered_pair_universe_count =
      after.unordered_pair_universe_count;
  audit.cumulative_candidate_pair_mass =
      after.cumulative_candidate_pair_mass;
  audit.cumulative_certified_pruned_pair_mass =
      after.cumulative_certified_pruned_pair_mass;
  audit.unresolved_pair_mass = after.unresolved_pair_mass;
  audit.maximum_node_visit_count = maximum_node_visit_count_;
  audit.maximum_candidate_count = maximum_candidate_count_;
  audit.maximum_witness_bank_receipt_count =
      maximum_witness_bank_receipt_count_;
  audit.maximum_certified_prune_region_count =
      maximum_certified_prune_region_count_;
  audit.maximum_transaction_node_visit_count =
      config_.maximum_node_visits_per_transaction;
  audit.maximum_transaction_candidate_count =
      config_.maximum_candidates_per_transaction;
  audit.maximum_transaction_witness_bank_receipt_count =
      config_.maximum_witness_bank_receipts_per_transaction;
  audit.maximum_transaction_certified_prune_region_count =
      config_.maximum_certified_prune_regions_per_transaction;
  audit.cumulative_node_visit_count =
      after.cumulative_node_visit_count;
  audit.cumulative_witness_bank_receipt_count =
      after.cumulative_witness_bank_receipt_count;
  audit.cumulative_certified_prune_region_count =
      after.cumulative_certified_prune_region_count;
  audit.source_build_authenticated = after.source_build_authenticated;
  audit.source_cloud_authenticated = after.source_cloud_authenticated;
  audit.tile_anchor_capacity_enforced =
      completed_anchor_delta <= config_.anchor_tile_capacity;
  audit.fixed_linear_work_budget_enforced =
      after.cumulative_node_visit_count <= maximum_node_visit_count_ &&
      after.cumulative_candidate_pair_mass <= maximum_candidate_count_ &&
      after.cumulative_witness_bank_receipt_count <=
          maximum_witness_bank_receipt_count_ &&
      after.cumulative_certified_prune_region_count <=
          maximum_certified_prune_region_count_;
  audit.fixed_transaction_work_budget_enforced =
      node_visit_delta <=
          checked_size_u64(
              config_.maximum_node_visits_per_transaction,
              "a tiled Morton/Yao48 transaction node cap does not fit uint64") &&
      candidate_delta <=
          checked_size_u64(
              config_.maximum_candidates_per_transaction,
              "a tiled Morton/Yao48 transaction candidate cap does not fit uint64") &&
      receipt_delta <=
          checked_size_u64(
              config_.maximum_witness_bank_receipts_per_transaction,
              "a tiled Morton/Yao48 transaction receipt cap does not fit uint64") &&
      prune_region_delta <=
          checked_size_u64(
              config_.maximum_certified_prune_regions_per_transaction,
              "a tiled Morton/Yao48 transaction prune cap does not fit uint64");
  audit.candidate_pruned_unresolved_partition_validated =
      after.source_build_authenticated &&
      after.source_cloud_authenticated &&
      after.morton_point_id_total_order_validated &&
      after.strict_postorder_interval_tree_validated &&
      after.every_completed_anchor_partition_validated &&
      after.candidate_plus_certified_pruned_mass_validated &&
      accounted == after.unordered_pair_universe_count;
  audit.transaction_records_match_mass_deltas = transaction_matches;
  audit.pair_coverage_partition_complete =
      audit.candidate_pruned_unresolved_partition_validated &&
      after.frontier_complete && after.unresolved_pair_mass == 0U &&
      after.cumulative_candidate_pair_mass +
              after.cumulative_certified_pruned_pair_mass ==
          after.unordered_pair_universe_count;
  audit.terminally_censored = terminally_censored_;
  audit.traversal_lease_owner_retained =
      after.traversal_lease_owner_retained;
  audit.host_fake_reference_execution =
      after.host_fake_reference_execution;
  audit.second_host_snapshot_retained =
      after.second_host_snapshot_retained;
  audit.active_witness_bank_storage_bounded_by_48_times_k =
      after.active_storage_bounded_by_48_times_k;
  audit.radial_subtree_certificates_reused =
      after.cumulative_radial_subtree_prune_count != 0U;
  audit.unfiltered_owned_subtree_emitted =
      after.unfiltered_owned_subtree_emitted;
  audit.dense_pair_fallback_performed =
      after.dense_pair_fallback_performed;
  audit.global_pair_matrix_materialized =
      after.global_pair_matrix_materialized;
  audit.exact_diametral_rank_predicate_evaluated =
      after.exact_diametral_rank_predicate_evaluated;
  audit.cuda_execution_performed = after.cuda_execution_performed;
  audit.scientific_decision_published =
      after.scientific_decision_published;
  audit.public_status_claimed = after.public_status_claimed;
  if (audit.exact_diametral_rank_predicate_evaluated ||
      audit.cuda_execution_performed) {
    throw std::logic_error(
        "a reference-CPU tiled Morton/Yao48 audit claims a forbidden "
        "scientific or CUDA execution");
  }
  return audit;
}

MortonYao48TiledPairFrontierAdvance
MortonYao48TiledPairFrontierContext::advance(
    const MortonLbvhDeviceBuildResult& certified_build,
    const spatial::CanonicalPointCloud& cloud) {
  if (poisoned_) {
    throw std::logic_error(
        "a poisoned tiled Morton/Yao48 frontier cannot advance");
  }
  MortonYao48PairFrontierAudit before;
  if (last_frontier_audit_.has_value()) {
    before = *last_frontier_audit_;
  } else {
    before.point_count = point_count_;
    before.maximum_closed_rank = config_.maximum_closed_rank;
    before.required_witness_count = config_.maximum_closed_rank - 1U;
    before.completed_anchor_count = 1U;
    // Morton position zero owns an empty prefix and is complete at birth.
    before.next_anchor_position = 1U;
  }
  PoisonOnExceptionalExit poison_on_exception{poisoned_};

  if (terminally_censored_ ||
      (last_frontier_audit_.has_value() &&
       last_frontier_audit_->frontier_complete)) {
    const MortonYao48PairFrontierAdvance probe = frontier_.advance(
        certified_build,
        cloud,
        MortonYao48PairFrontierAdvanceBudget{0U, 0U, 0U, 0U, 0U});
    if (!probe.candidates.empty() ||
        !probe.witness_bank_receipts.empty() ||
        !probe.certified_prune_regions.empty() ||
        probe.audit != before) {
      throw std::logic_error(
          "an idempotent tiled Morton/Yao48 probe mutated the frontier");
    }
    MortonYao48TiledPairFrontierAdvance result;
    result.status = terminally_censored_
                        ? MortonYao48TiledPairFrontierStatus::censored
                        : MortonYao48TiledPairFrontierStatus::
                              frontier_complete;
    result.stop_reason = terminally_censored_
                             ? terminal_stop_reason_
                             : MortonYao48TiledPairFrontierStopReason::none;
    result.audit = make_audit(before, probe.audit, 0U, 0U, 0U, 0U);
    if (!result.audit.fixed_linear_work_budget_enforced ||
        !result.audit.fixed_transaction_work_budget_enforced ||
        !result.audit.tile_anchor_capacity_enforced ||
        !result.audit.candidate_pruned_unresolved_partition_validated ||
        !result.audit.transaction_records_match_mass_deltas ||
        !result.audit.traversal_lease_owner_retained ||
        !result.audit.host_fake_reference_execution ||
        !result.audit.active_witness_bank_storage_bounded_by_48_times_k ||
        result.audit.second_host_snapshot_retained ||
        result.audit.unfiltered_owned_subtree_emitted ||
        result.audit.dense_pair_fallback_performed ||
        result.audit.global_pair_matrix_materialized ||
        result.audit.higher_order_structure_materialized ||
        result.audit.exact_diametral_rank_predicate_evaluated ||
        result.audit.cuda_execution_performed ||
        result.audit.scientific_pair_catalog_published ||
        result.audit.scientific_decision_published ||
        result.audit.public_status_claimed ||
        (terminally_censored_ &&
         (!result.audit.terminally_censored ||
          result.audit.pair_coverage_partition_complete ||
          result.audit.unresolved_pair_mass == 0U)) ||
        (!terminally_censored_ &&
         (result.audit.terminally_censored ||
          !result.audit.pair_coverage_partition_complete))) {
      throw std::logic_error(
          "an idempotent tiled Morton/Yao48 result violates its "
          "conservative contract");
    }
    last_frontier_audit_ = probe.audit;
    poison_on_exception.release();
    return result;
  }

  const std::size_t remaining_node_visits = checked_remaining(
      maximum_node_visit_count_,
      before.cumulative_node_visit_count,
      "the tiled Morton/Yao48 node counter exceeds its linear budget");
  const std::size_t remaining_candidates = checked_remaining(
      maximum_candidate_count_,
      before.cumulative_candidate_pair_mass,
      "the tiled Morton/Yao48 candidate counter exceeds its linear budget");
  const std::size_t remaining_receipts = checked_remaining(
      maximum_witness_bank_receipt_count_,
      before.cumulative_witness_bank_receipt_count,
      "the tiled Morton/Yao48 receipt counter exceeds its linear budget");
  const std::size_t remaining_prune_regions = checked_remaining(
      maximum_certified_prune_region_count_,
      before.cumulative_certified_prune_region_count,
      "the tiled Morton/Yao48 prune counter exceeds its linear budget");

  MortonYao48PairFrontierAdvance inner = frontier_.advance(
      certified_build,
      cloud,
      MortonYao48PairFrontierAdvanceBudget{
          std::min(
              remaining_node_visits,
              config_.maximum_node_visits_per_transaction),
          std::min(
              remaining_candidates,
              config_.maximum_candidates_per_transaction),
          std::min(
              remaining_receipts,
              config_.maximum_witness_bank_receipts_per_transaction),
          std::min(
              remaining_prune_regions,
              config_.maximum_certified_prune_regions_per_transaction),
          config_.anchor_tile_capacity});

  MortonYao48TiledPairFrontierAdvance result;
  result.candidates = std::move(inner.candidates);
  result.witness_bank_receipts =
      std::move(inner.witness_bank_receipts);
  result.certified_prune_regions =
      std::move(inner.certified_prune_regions);
  const std::uint64_t candidate_mass = checked_size_u64(
      result.candidates.size(),
      "a tiled Morton/Yao48 candidate count does not fit uint64");
  const std::uint64_t pruned_mass =
      prune_pair_mass(result.certified_prune_regions);

  if (inner.status == MortonYao48PairFrontierStatus::complete) {
    if (inner.stop_reason != MortonYao48PairFrontierStopReason::none) {
      throw std::logic_error(
          "a complete tiled Morton/Yao48 frontier has a stop reason");
    }
    result.status = MortonYao48TiledPairFrontierStatus::frontier_complete;
    result.stop_reason = MortonYao48TiledPairFrontierStopReason::none;
  } else if (
      inner.stop_reason == MortonYao48PairFrontierStopReason::
                               anchor_completion_capacity) {
    result.status = MortonYao48TiledPairFrontierStatus::tile_complete;
    result.stop_reason =
        MortonYao48TiledPairFrontierStopReason::anchor_tile_boundary;
  } else {
    result.status = MortonYao48TiledPairFrontierStatus::censored;
    result.stop_reason = map_stop_reason(inner.stop_reason);
    terminally_censored_ = true;
    terminal_stop_reason_ = result.stop_reason;
  }

  ++advance_sequence_;
  result.audit = make_audit(
      before,
      inner.audit,
      candidate_mass,
      pruned_mass,
      result.witness_bank_receipts.size(),
      result.certified_prune_regions.size());
  last_frontier_audit_ = inner.audit;

  const bool status_and_stop_reason_coherent =
      (result.status ==
           MortonYao48TiledPairFrontierStatus::frontier_complete &&
       result.stop_reason == MortonYao48TiledPairFrontierStopReason::none &&
       !result.audit.terminally_censored) ||
      (result.status == MortonYao48TiledPairFrontierStatus::tile_complete &&
       result.stop_reason ==
           MortonYao48TiledPairFrontierStopReason::anchor_tile_boundary &&
       result.audit.transaction_completed_anchor_count ==
           config_.anchor_tile_capacity &&
       !result.audit.terminally_censored) ||
      (result.status == MortonYao48TiledPairFrontierStatus::censored &&
       result.stop_reason != MortonYao48TiledPairFrontierStopReason::none &&
       result.stop_reason !=
           MortonYao48TiledPairFrontierStopReason::anchor_tile_boundary &&
       result.audit.terminally_censored);
  if (!result.audit.fixed_linear_work_budget_enforced ||
      !result.audit.fixed_transaction_work_budget_enforced ||
      !result.audit.tile_anchor_capacity_enforced ||
      !result.audit.candidate_pruned_unresolved_partition_validated ||
      !result.audit.transaction_records_match_mass_deltas ||
      !result.audit.traversal_lease_owner_retained ||
      !result.audit.host_fake_reference_execution ||
      !result.audit.active_witness_bank_storage_bounded_by_48_times_k ||
      !status_and_stop_reason_coherent ||
      result.audit.second_host_snapshot_retained ||
      result.audit.unfiltered_owned_subtree_emitted ||
      result.audit.dense_pair_fallback_performed ||
      result.audit.global_pair_matrix_materialized ||
      result.audit.higher_order_structure_materialized ||
      result.audit.exact_diametral_rank_predicate_evaluated ||
      result.audit.cuda_execution_performed ||
      result.audit.scientific_pair_catalog_published ||
      result.audit.scientific_decision_published ||
      result.audit.public_status_claimed ||
      (result.status ==
           MortonYao48TiledPairFrontierStatus::frontier_complete &&
       !result.audit.pair_coverage_partition_complete) ||
      (result.status == MortonYao48TiledPairFrontierStatus::censored &&
       (result.audit.pair_coverage_partition_complete ||
        result.audit.unresolved_pair_mass == 0U))) {
    throw std::logic_error(
        "a tiled Morton/Yao48 result violates its conservative contract");
  }
  poison_on_exception.release();
  return result;
}

}  // namespace morsehgp3d::gpu
