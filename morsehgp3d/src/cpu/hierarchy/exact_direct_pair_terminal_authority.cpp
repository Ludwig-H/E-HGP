#include "morsehgp3d/hierarchy/exact_direct_pair_terminal_authority.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] bool checked_unordered_pair_count(
    std::size_t point_count,
    std::size_t& result) noexcept {
  std::size_t first = point_count;
  std::size_t second = point_count == 0U ? 0U : point_count - 1U;
  if ((first & 1U) == 0U) {
    first /= 2U;
  } else {
    second /= 2U;
  }
  if (first != 0U &&
      second > std::numeric_limits<std::size_t>::max() / first) {
    return false;
  }
  result = first * second;
  return true;
}

[[nodiscard]] bool valid_point_ids(
    std::span<const spatial::PointId> ids,
    std::size_t point_count) noexcept {
  return std::is_sorted(ids.begin(), ids.end()) &&
      std::adjacent_find(ids.begin(), ids.end()) == ids.end() &&
      std::all_of(ids.begin(), ids.end(), [point_count](spatial::PointId id) {
        return id < point_count;
      });
}

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

}  // namespace

bool ExactDirectPairTerminalAudit::terminal_identities_hold() const noexcept {
  std::size_t expected_unordered_pair_universe_size = 0U;
  if (!checked_unordered_pair_count(
          point_count, expected_unordered_pair_universe_size)) {
    return false;
  }
  const bool source_kind_valid =
      (source_kind == ExactDirectPairTerminalSourceKind::
                          reference_host_fake_transactional_pair_cut &&
       host_fake_execution && !qualified_cuda_execution) ||
      (source_kind == ExactDirectPairTerminalSourceKind::
                          qualified_cuda_transactional_pair_cut &&
       qualified_cuda_execution && !host_fake_execution);
  return schema_version ==
             exact_direct_pair_terminal_authority_schema_version &&
      source_kind_valid && point_count >= 2U &&
      (requested_maximum_order == 5U || requested_maximum_order == 10U) &&
      effective_maximum_order ==
          std::min(requested_maximum_order, point_count) &&
      maximum_closed_rank ==
          std::max<std::size_t>(
              2U, std::min(effective_maximum_order + 1U, point_count)) &&
      unordered_pair_universe_size ==
          expected_unordered_pair_universe_size &&
      pruned_unordered_pair_mass <= unordered_pair_universe_size &&
      source_prune_receipt_count <= pruned_unordered_pair_mass &&
      source_terminal_pair_count ==
          unordered_pair_universe_size - pruned_unordered_pair_mass &&
      classification_started_count == source_terminal_pair_count &&
      classification_terminal_count == source_terminal_pair_count &&
      classification_budget_exhaustion_count == 0U &&
      above_rank_count <= classification_terminal_count &&
      emitted_record_count ==
          classification_terminal_count - above_rank_count &&
      accepted_event_count <= emitted_record_count &&
      relevant_extra_shell_diagnostic_count ==
          emitted_record_count - accepted_event_count &&
      source_partition_validated && every_prune_recertified &&
      unordered_pair_coverage_certified &&
      terminal_pair_classification_partition_certified &&
      output_partition_certified && retained_records_certified &&
      no_forbidden_global_structure_materialized &&
      !global_pair_matrix_materialized &&
      !ordinary_or_higher_order_delaunay_materialized &&
      !global_facet_coface_or_incidence_arena_materialized &&
      !hierarchy_reduction_performed && !public_status_claimed && complete &&
      !poisoned;
}

ExactDirectPairTerminalAuthority::ExactDirectPairTerminalAuthority(
    ExactPairSupportCheckpointManifest manifest,
    ExactDirectPairTerminalAudit audit,
    std::vector<ExactDirectPairTerminalRecord>&& records) noexcept
    : manifest_(std::move(manifest)),
      audit_(std::move(audit)),
      records_(std::move(records)),
      sealed_(audit_.terminal_identities_hold() &&
              records_.size() == audit_.emitted_record_count) {}

ExactDirectPairTerminalAuthority::ExactDirectPairTerminalAuthority(
    ExactDirectPairTerminalAuthority&& other) noexcept
    : manifest_(std::move(other.manifest_)),
      audit_(std::move(other.audit_)),
      records_(std::move(other.records_)),
      sealed_(other.sealed_) {
  other.sealed_ = false;
  other.audit_.retained_records_certified = false;
}

bool ExactDirectPairTerminalAuthority::bound_to(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) const noexcept {
  if (!sealed_in_process_terminal_authority() ||
      requested_maximum_order != audit_.requested_maximum_order ||
      !index.validated_for(cloud)) {
    return false;
  }
  try {
    return manifest_ == make_exact_pair_support_checkpoint_manifest(
                            index, cloud, requested_maximum_order);
  } catch (const std::exception&) {
    return false;
  }
}

bool ExactDirectPairTerminalAuthority::
    sealed_in_process_terminal_authority() const noexcept {
  if (!sealed_ || !audit_.terminal_identities_hold() ||
      records_.size() != audit_.emitted_record_count) {
    return false;
  }
  std::size_t event_count = 0U;
  std::size_t diagnostic_count = 0U;
  std::size_t point_id_reference_count = 0U;
  for (const ExactDirectPairTerminalRecord& record : records_) {
    if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      std::size_t closed_rank = 0U;
      std::size_t classified_point_count = 0U;
      std::size_t record_reference_count = 0U;
      if (event->support_ids[0] >= event->support_ids[1] ||
          event->support_ids[1] >= audit_.point_count ||
          !valid_point_ids(event->interior_ids, audit_.point_count) ||
          std::binary_search(
              event->interior_ids.begin(),
              event->interior_ids.end(),
              event->support_ids[0]) ||
          std::binary_search(
              event->interior_ids.begin(),
              event->interior_ids.end(),
              event->support_ids[1]) ||
          !checked_add(event->interior_ids.size(), 2U, closed_rank) ||
          event->closed_rank != closed_rank ||
          event->closed_rank > audit_.maximum_closed_rank ||
          !checked_add(
              event->closed_rank,
              event->exterior_count,
              classified_point_count) ||
          classified_point_count != audit_.point_count ||
          !checked_add(
              event->interior_ids.size(),
              2U,
              record_reference_count) ||
          !checked_add(
              point_id_reference_count,
              record_reference_count,
              point_id_reference_count)) {
        return false;
      }
      ++event_count;
    } else if (const auto* diagnostic =
                   std::get_if<ExactPairSupportExtraShellDiagnostic>(
                       &record)) {
      std::size_t minimum_rank = 0U;
      std::size_t observed_rank = 0U;
      std::size_t classified_point_count = 0U;
      std::size_t record_reference_count = 0U;
      if (diagnostic->support_ids[0] >= diagnostic->support_ids[1] ||
          diagnostic->support_ids[1] >= audit_.point_count ||
          diagnostic->shell_count <= 2U ||
          !valid_point_ids(
              diagnostic->interior_ids, audit_.point_count) ||
          std::binary_search(
              diagnostic->interior_ids.begin(),
              diagnostic->interior_ids.end(),
              diagnostic->support_ids[0]) ||
          std::binary_search(
              diagnostic->interior_ids.begin(),
              diagnostic->interior_ids.end(),
              diagnostic->support_ids[1]) ||
          diagnostic->canonical_extra_shell_witness_id >=
              audit_.point_count ||
          diagnostic->canonical_extra_shell_witness_id ==
              diagnostic->support_ids[0] ||
          diagnostic->canonical_extra_shell_witness_id ==
              diagnostic->support_ids[1] ||
          std::binary_search(
              diagnostic->interior_ids.begin(),
              diagnostic->interior_ids.end(),
              diagnostic->canonical_extra_shell_witness_id) ||
          !checked_add(
              diagnostic->interior_ids.size(), 2U, minimum_rank) ||
          diagnostic->minimum_possible_closed_rank != minimum_rank ||
          diagnostic->minimum_possible_closed_rank >
              audit_.maximum_closed_rank ||
          !checked_add(
              diagnostic->interior_ids.size(),
              diagnostic->shell_count,
              observed_rank) ||
          diagnostic->observed_closed_rank != observed_rank ||
          !checked_add(
              diagnostic->observed_closed_rank,
              diagnostic->exterior_count,
              classified_point_count) ||
          classified_point_count != audit_.point_count ||
          !checked_add(
              diagnostic->interior_ids.size(),
              3U,
              record_reference_count) ||
          !checked_add(
              point_id_reference_count,
              record_reference_count,
              point_id_reference_count)) {
        return false;
      }
      ++diagnostic_count;
    } else {
      return false;
    }
  }
  return event_count == audit_.accepted_event_count &&
      diagnostic_count == audit_.relevant_extra_shell_diagnostic_count &&
      point_id_reference_count ==
          audit_.emitted_point_id_reference_count;
}

std::vector<ExactDirectPairTerminalRecord>
ExactDirectPairTerminalAuthority::release_records() && {
  if (!sealed_in_process_terminal_authority()) {
    throw std::logic_error(
        "a revoked direct pair terminal authority cannot release records");
  }
  sealed_ = false;
  audit_.retained_records_certified = false;
  return std::move(records_);
}

}  // namespace morsehgp3d::hierarchy
