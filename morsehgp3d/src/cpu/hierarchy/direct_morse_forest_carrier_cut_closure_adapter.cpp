#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

using AdapterDecision =
    ExactDirectMorseForestCarrierCutClosureAdapterDecision;
using AdapterResult =
    ExactDirectMorseForestCarrierCutClosureAdapterResult;
using TerminalDisposition =
    ExactDirectMorseForestCarrierCutClosureTerminalDisposition;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.end(),
      right.point_ids.begin(),
      right.point_ids.end());
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key,
    std::size_t expected_cardinality,
    std::size_t cloud_point_count) noexcept {
  if (key.point_count != expected_cardinality || key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= cloud_point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectMorseForestCarrierCutClosureAdapterResult& result)
    noexcept {
  return !result.audit.closure_nodes_edges_or_projections_persisted &&
         !result.audit.locator_batch_committed &&
         !result.audit.external_binding_authority_replayed &&
         result.audit.forest_relative_only &&
         !result.audit
              .facet_coface_cell_gamma_or_delaunay_materialized &&
         !result.audit.public_status_claimed;
}

[[nodiscard]] bool terminal_summary_shape(
    const ExactDirectMorseForestCarrierCutClosureTerminalSummary& summary,
    std::size_t expected_seed_index,
    std::size_t target_order,
    std::size_t point_count,
    const ExactDirectSparseFacetWitness& query_witness) noexcept {
  if (summary.seed_index != expected_seed_index ||
      !canonical_key_shape(
          summary.source_facet_key, target_order, point_count) ||
      !canonical_key_shape(
          summary.terminal_facet_key, target_order, point_count)) {
    return false;
  }
  if (summary.disposition == TerminalDisposition::closure_unresolved) {
    return summary.closure_disposition ==
               ExactDirectSparseFacetDescentClosureDisposition::unresolved &&
           !summary.canonical_component_handle.has_value() &&
           !summary.binding_witness.has_value() &&
           !summary.carrier_cut_entry.has_value();
  }
  const bool latent =
      summary.disposition == TerminalDisposition::positive_active_latent;
  const bool resolved =
      summary.disposition ==
      TerminalDisposition::positive_resolved_reduced_root;
  if ((!latent && !resolved) ||
      summary.closure_disposition !=
          ExactDirectSparseFacetDescentClosureDisposition::relative_positive ||
      !summary.canonical_component_handle.has_value() ||
      !summary.binding_witness.has_value() ||
      !summary.carrier_cut_entry.has_value() ||
      summary.binding_witness->external_authority_id !=
          query_witness.external_authority_id ||
      summary.binding_witness->replay_token == 0U ||
      summary.carrier_cut_entry->component_handle !=
          *summary.canonical_component_handle ||
      summary.carrier_cut_entry->birth_record_index !=
          summary.carrier_cut_entry->component_handle) {
    return false;
  }
  return latent
             ? summary.carrier_cut_entry->disposition ==
                       ExactDirectMorseForestCarrierCutDisposition::
                           active_latent_without_reduced_root &&
                   !summary.carrier_cut_entry->reduced_root_node_id.has_value()
             : summary.carrier_cut_entry->disposition ==
                       ExactDirectMorseForestCarrierCutDisposition::
                           resolved_reduced_root &&
                   summary.carrier_cut_entry->reduced_root_node_id.has_value();
}

struct BuildContext {
  const ExactDirectMorseForestCarrierCutReplayView* view{};
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  std::span<const ExactDirectSparseFacetKey> keys;
  ExactDirectSparseFacetWitness query_witness{};
  const ExactDirectMorseForestCarrierCutClosureAdapterBudget* budget{};
  ExactDirectSparseFacetDescentClosureConfig closure_config{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  AdapterResult* result{};
};

void reject_without_closure(
    AdapterResult& result,
    AdapterDecision decision,
    bool preflight_rejection) noexcept {
  result.terminal_summaries.clear();
  result.contradiction_witness.reset();
  result.audit.terminal_summary_count = 0U;
  result.audit.retained_terminal_key_point_reference_count = 0U;
  result.audit.retained_logical_output_entry_count = 0U;
  result.audit.no_closure_constructed_on_preflight_rejection =
      preflight_rejection;
  result.decision = decision;
  result.scope =
      ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
}

void copy_closure_audit(
    AdapterResult& result,
    const ExactDirectSparseFacetDescentClosureResult& closure) {
  result.required_memo_slot_count = closure.required_memo_slot_count;
  result.closure_counters = closure.counters;
  result.closure_disposition = closure.disposition;
  result.closure_decision = closure.decision;
  result.contradiction_witness = closure.contradiction_witness;
  result.audit.transient_node_count = closure.nodes.size();
  result.audit.transient_edge_count = closure.edges.size();
  result.audit.transient_seed_projection_count =
      closure.seed_projections.size();
  result.audit.closure_partial_refinement_outcome_accepted =
      closure.certified_partial_refinement_outcome();
}

[[nodiscard]] bool project_complete_closure(
    BuildContext& context,
    const ExactDirectSparseFacetDescentClosureResult& closure) {
  AdapterResult& result = *context.result;
  if (closure.seed_projections.size() != context.keys.size()) {
    return false;
  }

  std::size_t retained_point_reference_count = 0U;
  std::size_t positive_lookup_count = 0U;
  bool any_unresolved = false;
  for (std::size_t seed_index = 0U;
       seed_index < context.keys.size();
       ++seed_index) {
    const auto& projection = closure.seed_projections[seed_index];
    if (projection.seed_index != seed_index ||
        projection.source_facet_key != context.keys[seed_index] ||
        projection.terminal_node_index >= closure.nodes.size()) {
      return false;
    }
    const auto& terminal = closure.nodes[projection.terminal_node_index];
    if (terminal.node_index != projection.terminal_node_index ||
        !terminal.terminal_pointer_certified ||
        terminal.terminal_node_index != terminal.node_index ||
        terminal.closure_disposition != projection.closure_disposition ||
        !canonical_key_shape(
            terminal.facet_key,
            result.target_order,
            context.cloud->size())) {
      return false;
    }

    ExactDirectMorseForestCarrierCutClosureTerminalSummary summary;
    summary.seed_index = seed_index;
    summary.source_facet_key = context.keys[seed_index];
    summary.terminal_facet_key = terminal.facet_key;
    summary.closure_disposition = projection.closure_disposition;

    if (projection.closure_disposition ==
        ExactDirectSparseFacetDescentClosureDisposition::unresolved) {
      if (terminal.resolved_component_handle.has_value() ||
          terminal.resolved_binding_witness.has_value()) {
        return false;
      }
      any_unresolved = true;
      summary.disposition = TerminalDisposition::closure_unresolved;
    } else if (
        projection.closure_disposition ==
        ExactDirectSparseFacetDescentClosureDisposition::relative_positive) {
      if (!terminal.resolved_component_handle.has_value() ||
          !terminal.resolved_binding_witness.has_value() ||
          terminal.resolved_binding_witness->external_authority_id !=
              context.query_witness.external_authority_id ||
          terminal.resolved_binding_witness->replay_token == 0U ||
          positive_lookup_count >=
              context.budget->maximum_carrier_cut_lookup_count) {
        return false;
      }
      ++positive_lookup_count;
      std::optional<ExactDirectMorseForestCarrierCutEntry> carrier_entry;
      try {
        carrier_entry = context.view->find_entry(
            *terminal.resolved_component_handle);
      } catch (const std::logic_error&) {
        // A malformed carrier parent/owner/root relation is an intrinsic
        // contradiction between the frozen locator and the replayed carrier
        // state.  Returning false asks the epoch hook to poison the session;
        // allocation failures still propagate to the adapter's outer catch.
        return false;
      }
      if (!carrier_entry.has_value() ||
          carrier_entry->disposition ==
              ExactDirectMorseForestCarrierCutDisposition::
                  inactive_at_closed_cut) {
        return false;
      }
      summary.canonical_component_handle =
          terminal.resolved_component_handle;
      summary.binding_witness = terminal.resolved_binding_witness;
      summary.carrier_cut_entry = carrier_entry;
      if (carrier_entry->disposition ==
          ExactDirectMorseForestCarrierCutDisposition::
              active_latent_without_reduced_root) {
        summary.disposition = TerminalDisposition::positive_active_latent;
      } else if (
          carrier_entry->disposition ==
          ExactDirectMorseForestCarrierCutDisposition::
              resolved_reduced_root) {
        summary.disposition =
            TerminalDisposition::positive_resolved_reduced_root;
      } else {
        return false;
      }
    } else {
      return false;
    }

    std::size_t summary_point_reference_count = 0U;
    if (!checked_add(
            summary.source_facet_key.point_count,
            summary.terminal_facet_key.point_count,
            summary_point_reference_count) ||
        !checked_add(
            retained_point_reference_count,
            summary_point_reference_count,
            retained_point_reference_count) ||
        retained_point_reference_count >
            context.budget
                ->maximum_terminal_key_point_reference_count) {
      return false;
    }
    result.terminal_summaries.push_back(summary);
  }

  std::size_t logical_output_entry_count = 0U;
  if (!checked_add(
          result.terminal_summaries.size(),
          retained_point_reference_count,
          logical_output_entry_count) ||
      logical_output_entry_count >
          context.budget->maximum_logical_output_entry_count) {
    return false;
  }
  result.audit.carrier_cut_lookup_count = positive_lookup_count;
  result.audit.terminal_summary_count =
      result.terminal_summaries.size();
  result.audit.retained_terminal_key_point_reference_count =
      retained_point_reference_count;
  result.audit.retained_logical_output_entry_count =
      logical_output_entry_count;
  result.audit.every_seed_projection_summarized_once = true;
  result.audit.every_positive_terminal_mapped_to_active_target_carrier =
      true;
  result.audit.terminal_summaries_sorted_unique_by_seed_index = true;
  result.decision = context.keys.empty()
                        ? AdapterDecision::complete_empty_key_set
                        : any_unresolved
                              ? AdapterDecision::
                                    complete_with_unresolved_terminal_summaries
                              : AdapterDecision::
                                    complete_all_positive_terminal_summaries;
  result.scope = ExactDirectMorseForestCarrierCutClosureAdapterScope::
      one_live_carrier_cut_epoch_one_transient_10_5c_closure_to_compact_terminal_carrier_summaries_only;
  return true;
}

bool build_with_frozen_locator(
    void* erased_context,
    std::size_t point_count,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& entry_stamp,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  auto& context = *static_cast<BuildContext*>(erased_context);
  AdapterResult& result = *context.result;
  result.point_count = point_count;
  result.target_order = target_order;
  result.closed_squared_level = closed_squared_level;
  result.locator_snapshot_stamp = entry_stamp;
  result.common_facet_cardinality = target_order;
  result.audit.view_epoch_checked_at_entry = true;

  std::size_t one_side_point_reference_count = 0U;
  std::size_t required_point_reference_count = 0U;
  std::size_t required_logical_output_entry_count = 0U;
  if (!checked_multiply(
          context.keys.size(),
          target_order,
          one_side_point_reference_count) ||
      !checked_multiply(
          one_side_point_reference_count,
          2U,
          required_point_reference_count) ||
      !checked_add(
          context.keys.size(),
          required_point_reference_count,
          required_logical_output_entry_count)) {
    reject_without_closure(
        result, AdapterDecision::no_adapter_capacity_overflow, true);
    return true;
  }
  result.required_terminal_summary_count = context.keys.size();
  result.required_terminal_key_point_reference_count =
      required_point_reference_count;
  result.required_logical_output_entry_count =
      required_logical_output_entry_count;

  if (context.keys.size() >
          context.budget->maximum_canonical_key_scan_count ||
      context.keys.size() >
          context.budget->maximum_terminal_summary_count ||
      context.keys.size() >
          context.budget->maximum_carrier_cut_lookup_count ||
      required_point_reference_count >
          context.budget
              ->maximum_terminal_key_point_reference_count ||
      required_logical_output_entry_count >
          context.budget->maximum_logical_output_entry_count) {
    reject_without_closure(
        result, AdapterDecision::no_adapter_budget_exhausted, true);
    return true;
  }

  bool valid_keys = point_count != 0U &&
                    context.cloud->size() == point_count &&
                    target_order != 0U &&
                    target_order <=
                        direct_sparse_positive_facet_maximum_point_count &&
                    context.index->validated_for(*context.cloud) &&
                    locator.certified_positive_locator() &&
                    context.query_witness.external_authority_id != 0U &&
                    context.query_witness.external_authority_id ==
                        locator.config().external_authority_id &&
                    context.query_witness.replay_token != 0U;
  for (std::size_t key_index = 0U;
       valid_keys && key_index < context.keys.size();
       ++key_index) {
    ++result.audit.canonical_key_scan_count;
    valid_keys = canonical_key_shape(
        context.keys[key_index], target_order, context.cloud->size());
    if (valid_keys && key_index != 0U) {
      valid_keys = facet_key_less(
          context.keys[key_index - 1U], context.keys[key_index]);
    }
  }
  if (!valid_keys) {
    reject_without_closure(
        result, AdapterDecision::no_adapter_input_shape_rejected, true);
    return true;
  }
  result.audit.cloud_point_count_matches_source_forest = true;
  result.audit.target_order_bound_to_common_key_cardinality = true;
  result.audit.canonical_distinct_input_revalidated = true;
  result.terminal_summaries.reserve(context.keys.size());
  result.audit.output_preallocated_before_closure = true;

  bool projection_succeeded = false;
  bool session_coupling_contradiction = false;
  {
    ++result.audit.closure_build_attempt_count;
    const auto closure =
        build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
            *context.index,
            *context.cloud,
            context.keys,
            closed_squared_level,
            context.query_witness,
            locator,
            context.budget->closure_budget,
            context.closure_config,
            context.traversal_order);
    ++result.audit.closure_build_completion_count;
    copy_closure_audit(result, closure);
    result.audit.exactly_one_closure_built = true;

    result.audit.closure_metadata_matches_live_cut =
        closure.requested_budget == context.budget->closure_budget &&
        closure.config == context.closure_config &&
        closure.traversal_order == context.traversal_order &&
        closure.closed_batch_squared_level == closed_squared_level &&
        closure.locator_query_witness == context.query_witness &&
        closure.locator_snapshot_stamp == entry_stamp &&
        closure.common_facet_cardinality ==
            (context.keys.empty() ? 0U : target_order);

    if (!result.audit.closure_metadata_matches_live_cut) {
      result.terminal_summaries.clear();
      result.contradiction_witness.reset();
      result.decision = AdapterDecision::no_adapter_closure_outcome_rejected;
      session_coupling_contradiction = true;
    } else if (closure.certified_complete_relative_positive_closure() ||
        closure.certified_complete_with_unresolved_terminals()) {
      projection_succeeded = project_complete_closure(context, closure);
      if (!projection_succeeded) {
        result.terminal_summaries.clear();
        result.audit.terminal_summary_count = 0U;
        result.audit.retained_terminal_key_point_reference_count = 0U;
        result.audit.retained_logical_output_entry_count = 0U;
        result.decision =
            AdapterDecision::no_adapter_terminal_projection_contradiction;
        session_coupling_contradiction = true;
        result.scope =
            ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
      }
    } else if (closure.certified_budget_exhaustion()) {
      result.terminal_summaries.clear();
      result.decision = AdapterDecision::no_adapter_closure_budget_exhausted;
    } else if (closure.certified_fail_closed_contradiction()) {
      result.terminal_summaries.clear();
      result.decision = AdapterDecision::no_adapter_closure_contradiction;
    } else {
      result.terminal_summaries.clear();
      result.contradiction_witness.reset();
      result.decision = AdapterDecision::no_adapter_closure_outcome_rejected;
    }
  }
  result.audit.transient_closure_graph_destroyed_before_return = true;
  if (!projection_succeeded) {
    result.scope =
        ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
  }
  return !session_coupling_contradiction;
}

[[nodiscard]] bool rejection_decision(AdapterDecision decision) noexcept {
  return decision != AdapterDecision::not_certified &&
         decision != AdapterDecision::complete_empty_key_set &&
         decision !=
             AdapterDecision::complete_all_positive_terminal_summaries &&
         decision !=
             AdapterDecision::complete_with_unresolved_terminal_summaries;
}

}  // namespace

ExactDirectMorseForestCarrierCutClosureAdapterResult
ExactDirectMorseForestCarrierCutReplayView::
    build_closure_summary_from_canonical_distinct_keys(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        std::span<const ExactDirectSparseFacetKey> canonical_distinct_keys,
        const ExactDirectSparseFacetWitness& locator_query_witness,
        const ExactDirectMorseForestCarrierCutClosureAdapterBudget& budget,
        const ExactDirectSparseFacetDescentClosureConfig& closure_config,
        spatial::LbvhTraversalOrder traversal_order) const {
  AdapterResult result;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.locator_query_witness = locator_query_witness;
  if (session_ == nullptr) {
    result.decision = AdapterDecision::no_adapter_stale_view_epoch;
    return result;
  }

  BuildContext context{
      this,
      &index,
      &cloud,
      canonical_distinct_keys,
      locator_query_witness,
      &budget,
      closure_config,
      traversal_order,
      &result};
  try {
    const auto visit = session_->view_with_frozen_locator(
        visit_epoch_, &context, build_with_frozen_locator);
    using VisitDecision =
        ExactDirectMorseForestCarrierCutReplaySession::
            FrozenLocatorViewDecision;
    switch (visit) {
      case VisitDecision::complete_stable_snapshot:
        result.audit.entry_and_post_locator_stamps_equal = true;
        result.audit.pre_result_and_post_locator_stamps_equal =
            result.audit.closure_build_completion_count == 1U &&
            result.audit.closure_metadata_matches_live_cut;
        break;
      case VisitDecision::session_not_ready:
        reject_without_closure(
            result, AdapterDecision::no_adapter_session_not_ready, false);
        break;
      case VisitDecision::session_poisoned:
        if (result.decision ==
                AdapterDecision::
                    no_adapter_terminal_projection_contradiction ||
            (result.decision ==
                 AdapterDecision::no_adapter_closure_outcome_rejected &&
             result.audit.closure_build_completion_count == 1U &&
             !result.audit.closure_metadata_matches_live_cut)) {
          result.audit.entry_and_post_locator_stamps_equal = true;
          result.audit.pre_result_and_post_locator_stamps_equal =
              result.audit.closure_build_completion_count == 1U &&
              result.audit.closure_metadata_matches_live_cut;
        } else {
          reject_without_closure(
              result, AdapterDecision::no_adapter_session_poisoned, false);
        }
        result.audit.session_poisoned = true;
        break;
      case VisitDecision::stale_view_epoch:
        reject_without_closure(
            result, AdapterDecision::no_adapter_stale_view_epoch, false);
        break;
      case VisitDecision::locator_snapshot_changed:
        reject_without_closure(
            result,
            AdapterDecision::no_adapter_locator_snapshot_changed,
            false);
        result.audit.locator_state_mutated = true;
        result.audit.session_poisoned = true;
        break;
    }
  } catch (const std::bad_alloc&) {
    result.terminal_summaries.clear();
    result.contradiction_witness.reset();
    result.audit.terminal_summary_count = 0U;
    result.audit.retained_terminal_key_point_reference_count = 0U;
    result.audit.retained_logical_output_entry_count = 0U;
    result.audit.transient_closure_graph_destroyed_before_return =
        result.audit.closure_build_attempt_count != 0U;
    if (session_->poisoned()) {
      result.decision = AdapterDecision::no_adapter_locator_snapshot_changed;
      result.audit.locator_state_mutated = true;
      result.audit.session_poisoned = true;
    } else {
      result.decision = AdapterDecision::no_adapter_allocation_failed;
    }
    result.scope =
        ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
  } catch (const std::length_error&) {
    result.terminal_summaries.clear();
    result.contradiction_witness.reset();
    result.audit.terminal_summary_count = 0U;
    result.audit.retained_terminal_key_point_reference_count = 0U;
    result.audit.retained_logical_output_entry_count = 0U;
    result.audit.transient_closure_graph_destroyed_before_return =
        result.audit.closure_build_attempt_count != 0U;
    if (session_->poisoned()) {
      result.decision = AdapterDecision::no_adapter_locator_snapshot_changed;
      result.audit.locator_state_mutated = true;
      result.audit.session_poisoned = true;
    } else {
      result.decision = AdapterDecision::no_adapter_capacity_overflow;
    }
    result.scope =
        ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
  } catch (const std::invalid_argument&) {
    result.terminal_summaries.clear();
    result.contradiction_witness.reset();
    result.audit.terminal_summary_count = 0U;
    result.audit.retained_terminal_key_point_reference_count = 0U;
    result.audit.retained_logical_output_entry_count = 0U;
    result.audit.transient_closure_graph_destroyed_before_return =
        result.audit.closure_build_attempt_count != 0U;
    if (session_->poisoned()) {
      result.decision = AdapterDecision::no_adapter_locator_snapshot_changed;
      result.audit.locator_state_mutated = true;
      result.audit.session_poisoned = true;
    } else {
      result.decision = result.audit.canonical_distinct_input_revalidated
                            ? AdapterDecision::
                                  no_adapter_closure_outcome_rejected
                            : AdapterDecision::no_adapter_input_shape_rejected;
    }
    result.scope =
        ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
  } catch (const std::exception&) {
    result.terminal_summaries.clear();
    result.contradiction_witness.reset();
    result.audit.terminal_summary_count = 0U;
    result.audit.retained_terminal_key_point_reference_count = 0U;
    result.audit.retained_logical_output_entry_count = 0U;
    result.audit.transient_closure_graph_destroyed_before_return =
        result.audit.closure_build_attempt_count != 0U;
    if (session_->poisoned()) {
      result.decision = AdapterDecision::no_adapter_locator_snapshot_changed;
      result.audit.locator_state_mutated = true;
      result.audit.session_poisoned = true;
    } else if (!session_->view_epoch_is_live(visit_epoch_)) {
      result.decision = AdapterDecision::no_adapter_stale_view_epoch;
    } else {
      result.decision = AdapterDecision::no_adapter_closure_outcome_rejected;
    }
    result.scope =
        ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified;
  }
  return result;
}

bool ExactDirectMorseForestCarrierCutClosureAdapterResult::
    certified_compact_closure_summary() const noexcept {
  const bool complete_decision =
      decision == AdapterDecision::complete_empty_key_set ||
      decision == AdapterDecision::complete_all_positive_terminal_summaries ||
      decision ==
          AdapterDecision::complete_with_unresolved_terminal_summaries;
  if (schema_version !=
          direct_morse_forest_carrier_cut_closure_adapter_schema_version ||
      !complete_decision || target_order == 0U ||
      point_count == 0U ||
      target_order > point_count ||
      locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.replay_token == 0U ||
      locator_snapshot_stamp.schema_version !=
          direct_sparse_positive_facet_locator_schema_version ||
      locator_snapshot_stamp.external_authority_id !=
          locator_query_witness.external_authority_id ||
      common_facet_cardinality != target_order ||
      terminal_summaries.size() != required_terminal_summary_count ||
      required_terminal_summary_count >
          requested_budget.maximum_canonical_key_scan_count ||
      required_terminal_summary_count >
          requested_budget.maximum_terminal_summary_count ||
      required_terminal_summary_count >
          requested_budget.maximum_carrier_cut_lookup_count ||
      required_terminal_summary_count >
          requested_budget.closure_budget.maximum_seed_count ||
      required_terminal_key_point_reference_count >
          requested_budget
              .maximum_terminal_key_point_reference_count ||
      required_logical_output_entry_count >
          requested_budget.maximum_logical_output_entry_count ||
      audit.transient_node_count >
          requested_budget.closure_budget.maximum_node_count ||
      audit.transient_seed_projection_count >
          requested_budget.closure_budget.maximum_seed_count ||
      audit.transient_seed_projection_count !=
          required_terminal_summary_count ||
      audit.transient_node_count != closure_counters.interned_node_count ||
      audit.transient_edge_count != closure_counters.strict_edge_count ||
      closure_counters.evaluated_step_source_count >
          requested_budget.closure_budget.maximum_step_call_count ||
      required_memo_slot_count >
          requested_budget.closure_budget.maximum_memo_slot_count ||
      audit.terminal_summary_count != terminal_summaries.size() ||
      audit.retained_terminal_key_point_reference_count !=
          required_terminal_key_point_reference_count ||
      audit.retained_logical_output_entry_count !=
          required_logical_output_entry_count ||
      contradiction_witness.has_value() ||
      !audit.view_epoch_checked_at_entry ||
      !audit.cloud_point_count_matches_source_forest ||
      !audit.matching_canonical_point_namespace_required ||
      audit.forest_to_cloud_namespace_identity_certified ||
      !audit.target_order_bound_to_common_key_cardinality ||
      !audit.canonical_distinct_input_revalidated ||
      !audit.output_preallocated_before_closure ||
      audit.closure_build_attempt_count != 1U ||
      audit.closure_build_completion_count != 1U ||
      !audit.exactly_one_closure_built ||
      !audit.closure_partial_refinement_outcome_accepted ||
      !audit.closure_metadata_matches_live_cut ||
      !audit.entry_and_post_locator_stamps_equal ||
      !audit.pre_result_and_post_locator_stamps_equal ||
      !audit.every_seed_projection_summarized_once ||
      !audit.every_positive_terminal_mapped_to_active_target_carrier ||
      !audit.terminal_summaries_sorted_unique_by_seed_index ||
      !audit.transient_closure_graph_destroyed_before_return ||
      audit.no_closure_constructed_on_preflight_rejection ||
      audit.locator_state_mutated ||
      audit.session_poisoned ||
      scope != ExactDirectMorseForestCarrierCutClosureAdapterScope::
                   one_live_carrier_cut_epoch_one_transient_10_5c_closure_to_compact_terminal_carrier_summaries_only ||
      !non_scope_is_honest(*this)) {
    return false;
  }
  if (decision == AdapterDecision::complete_empty_key_set) {
    if (!terminal_summaries.empty() ||
        closure_decision !=
            ExactDirectSparseFacetDescentClosureDecision::
                complete_empty_seed_set) {
      return false;
    }
  }
  bool any_unresolved = false;
  std::size_t positive_summary_count = 0U;
  std::size_t recomputed_point_reference_count = 0U;
  for (std::size_t index = 0U; index < terminal_summaries.size(); ++index) {
    if (!terminal_summary_shape(
            terminal_summaries[index],
            index,
            target_order,
            point_count,
            locator_query_witness)) {
      return false;
    }
    std::size_t summary_point_reference_count = 0U;
    if (!checked_add(
            terminal_summaries[index].source_facet_key.point_count,
            terminal_summaries[index].terminal_facet_key.point_count,
            summary_point_reference_count) ||
        !checked_add(
            recomputed_point_reference_count,
            summary_point_reference_count,
            recomputed_point_reference_count)) {
      return false;
    }
    any_unresolved = any_unresolved ||
                     terminal_summaries[index].disposition ==
                         TerminalDisposition::closure_unresolved;
    positive_summary_count +=
        terminal_summaries[index].disposition ==
                TerminalDisposition::closure_unresolved
            ? 0U
            : 1U;
    if (index != 0U &&
        !facet_key_less(
            terminal_summaries[index - 1U].source_facet_key,
            terminal_summaries[index].source_facet_key)) {
      return false;
    }
  }
  const bool decision_matches_closure =
      decision == AdapterDecision::complete_empty_key_set
          ? closure_decision ==
                ExactDirectSparseFacetDescentClosureDecision::
                    complete_empty_seed_set &&
                closure_disposition ==
                    ExactDirectSparseFacetDescentClosureDisposition::
                        relative_positive
          : decision ==
                    AdapterDecision::
                        complete_all_positive_terminal_summaries
                ? closure_decision ==
                      ExactDirectSparseFacetDescentClosureDecision::
                          complete_all_seeds_relative_positive &&
                      closure_disposition ==
                          ExactDirectSparseFacetDescentClosureDisposition::
                              relative_positive
                : closure_decision ==
                      ExactDirectSparseFacetDescentClosureDecision::
                          complete_all_seeds_with_unresolved_terminals &&
                      closure_disposition ==
                          ExactDirectSparseFacetDescentClosureDisposition::
                              unresolved;
  std::size_t recomputed_logical_output_entry_count = 0U;
  if (!checked_add(
          terminal_summaries.size(),
          recomputed_point_reference_count,
          recomputed_logical_output_entry_count)) {
    return false;
  }
  return decision_matches_closure &&
         required_terminal_key_point_reference_count ==
             recomputed_point_reference_count &&
         required_logical_output_entry_count ==
             recomputed_logical_output_entry_count &&
         audit.canonical_key_scan_count ==
             required_terminal_summary_count &&
         audit.carrier_cut_lookup_count == positive_summary_count &&
         closure_counters.input_seed_reference_count ==
             required_terminal_summary_count &&
         closure_counters.processed_seed_reference_count ==
             required_terminal_summary_count &&
         any_unresolved ==
             (decision ==
              AdapterDecision::
                  complete_with_unresolved_terminal_summaries);
}

bool ExactDirectMorseForestCarrierCutClosureAdapterResult::
    certified_atomic_rejection() const noexcept {
  const bool locator_change =
      decision == AdapterDecision::no_adapter_locator_snapshot_changed;
  const bool poison_expected =
      locator_change ||
      decision == AdapterDecision::no_adapter_session_poisoned ||
      decision ==
          AdapterDecision::no_adapter_terminal_projection_contradiction ||
      (decision == AdapterDecision::no_adapter_closure_outcome_rejected &&
       audit.closure_build_completion_count == 1U &&
       !audit.closure_metadata_matches_live_cut);
  if (schema_version !=
             direct_morse_forest_carrier_cut_closure_adapter_schema_version ||
      !rejection_decision(decision)) {
    return false;
  }
  if (!rejection_decision(decision) || !terminal_summaries.empty() ||
      audit.terminal_summary_count != 0U ||
      audit.retained_terminal_key_point_reference_count != 0U ||
      audit.retained_logical_output_entry_count != 0U ||
      audit.locator_state_mutated != locator_change ||
      audit.session_poisoned != poison_expected ||
      scope !=
          ExactDirectMorseForestCarrierCutClosureAdapterScope::unspecified ||
      !non_scope_is_honest(*this) ||
      audit.closure_build_completion_count >
          audit.closure_build_attempt_count ||
      audit.closure_build_attempt_count > 1U ||
      (audit.closure_build_attempt_count != 0U &&
       !audit.transient_closure_graph_destroyed_before_return)) {
    return false;
  }
  const bool preflight_rejection =
      decision == AdapterDecision::no_adapter_budget_exhausted ||
      decision == AdapterDecision::no_adapter_input_shape_rejected;
  if (preflight_rejection) {
    return audit.closure_build_attempt_count == 0U &&
           audit.closure_build_completion_count == 0U &&
           audit.no_closure_constructed_on_preflight_rejection &&
           audit.entry_and_post_locator_stamps_equal;
  }
  const bool certified_closure_rejection =
      decision == AdapterDecision::no_adapter_closure_budget_exhausted ||
      decision == AdapterDecision::no_adapter_closure_contradiction ||
      decision ==
          AdapterDecision::no_adapter_terminal_projection_contradiction;
  if (certified_closure_rejection) {
    const bool diagnostic_shape =
        decision == AdapterDecision::no_adapter_closure_budget_exhausted
            ? closure_disposition ==
                      ExactDirectSparseFacetDescentClosureDisposition::
                          budget_exhausted &&
                  !contradiction_witness.has_value()
            : decision == AdapterDecision::no_adapter_closure_contradiction
                  ? closure_disposition ==
                            ExactDirectSparseFacetDescentClosureDisposition::
                                contradiction &&
                        contradiction_witness.has_value()
                  : (closure_disposition ==
                             ExactDirectSparseFacetDescentClosureDisposition::
                                 relative_positive ||
                     closure_disposition ==
                         ExactDirectSparseFacetDescentClosureDisposition::
                             unresolved) &&
                        !contradiction_witness.has_value();
    return diagnostic_shape &&
           audit.closure_build_attempt_count == 1U &&
           audit.closure_build_completion_count == 1U &&
           audit.exactly_one_closure_built &&
           audit.closure_partial_refinement_outcome_accepted &&
           audit.closure_metadata_matches_live_cut &&
           audit.entry_and_post_locator_stamps_equal &&
           audit.pre_result_and_post_locator_stamps_equal &&
           !audit.no_closure_constructed_on_preflight_rejection;
  }
  return true;
}

bool ExactDirectMorseForestCarrierCutClosureAdapterResult::
    certified_outcome() const noexcept {
  return certified_compact_closure_summary() ||
         certified_atomic_rejection();
}

}  // namespace morsehgp3d::hierarchy
