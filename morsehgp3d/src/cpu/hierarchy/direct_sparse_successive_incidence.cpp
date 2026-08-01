#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence.hpp"

#include "direct_sparse_incidence_engine.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget engine_budget(
    const ExactDirectSparseSuccessiveIncidenceBudget& budget) noexcept {
  return {
      budget.maximum_source_support_enumeration_count,
      budget.maximum_node_visit_count,
      budget.maximum_internal_node_expansion_count,
      budget.maximum_exact_aabb_bound_evaluation_count,
      budget.maximum_exact_point_evaluation_count,
      budget.maximum_coface_support_enumeration_count,
      budget.maximum_candidate_point_classification_count,
      budget.maximum_frontier_entry_count,
      budget.maximum_cominimizer_count};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStopReason adapt_stop_reason(
    ExactDirectSparseFirstIncidenceStopReason reason) {
  switch (reason) {
    case ExactDirectSparseFirstIncidenceStopReason::none:
      return ExactDirectSparseSuccessiveIncidenceStopReason::none;
    case ExactDirectSparseFirstIncidenceStopReason::
        source_support_enumeration_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          source_support_enumeration_limit;
    case ExactDirectSparseFirstIncidenceStopReason::node_visit_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::node_visit_limit;
    case ExactDirectSparseFirstIncidenceStopReason::
        internal_node_expansion_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          internal_node_expansion_limit;
    case ExactDirectSparseFirstIncidenceStopReason::
        exact_aabb_bound_evaluation_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          exact_aabb_bound_evaluation_limit;
    case ExactDirectSparseFirstIncidenceStopReason::
        exact_point_evaluation_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          exact_point_evaluation_limit;
    case ExactDirectSparseFirstIncidenceStopReason::
        coface_support_enumeration_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          coface_support_enumeration_limit;
    case ExactDirectSparseFirstIncidenceStopReason::
        candidate_point_classification_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          candidate_point_classification_limit;
    case ExactDirectSparseFirstIncidenceStopReason::frontier_entry_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          frontier_entry_limit;
    case ExactDirectSparseFirstIncidenceStopReason::cominimizer_entry_limit:
      return ExactDirectSparseSuccessiveIncidenceStopReason::
          cominimizer_entry_limit;
  }
  throw std::logic_error("an incidence engine returned an invalid stop reason");
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceDecision adapt_decision(
    ExactDirectSparseFirstIncidenceDecision decision) {
  switch (decision) {
    case ExactDirectSparseFirstIncidenceDecision::not_certified:
      return ExactDirectSparseSuccessiveIncidenceDecision::not_certified;
    case ExactDirectSparseFirstIncidenceDecision::complete_no_coface:
      return ExactDirectSparseSuccessiveIncidenceDecision::
          complete_no_strictly_higher_coface;
    case ExactDirectSparseFirstIncidenceDecision::
        complete_exact_first_incidence:
      return ExactDirectSparseSuccessiveIncidenceDecision::
          complete_exact_next_incidence;
    case ExactDirectSparseFirstIncidenceDecision::
        no_first_incidence_budget_exhausted:
      return ExactDirectSparseSuccessiveIncidenceDecision::
          no_next_incidence_budget_exhausted;
  }
  throw std::logic_error("an incidence engine returned an invalid decision");
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceAudit adapt_audit(
    const ExactDirectSparseFirstIncidenceAudit& audit,
    std::size_t at_or_below_threshold_count) noexcept {
  return {
      audit.eligible_coface_point_count,
      audit.supplied_incumbent_seed_point_count,
      audit.exact_incumbent_seed_evaluation_count,
      audit.source_support_enumeration_count,
      audit.node_visit_count,
      audit.internal_node_expansion_count,
      audit.exact_aabb_bound_evaluation_count,
      audit.exact_point_evaluation_count,
      audit.excluded_facet_point_count,
      audit.coface_support_enumeration_count,
      audit.candidate_point_classification_count,
      audit.inside_or_boundary_source_ball_point_count,
      audit.outside_source_ball_point_count,
      at_or_below_threshold_count,
      audit.pruned_node_count,
      audit.pruned_eligible_point_count,
      audit.peak_frontier_entry_count,
      audit.peak_cominimizer_entry_count,
      audit.incumbent_improvement_count,
      audit.equal_incumbent_observation_count,
      audit.provisional_cominimizer_overflow_count,
      audit.traversal_complete};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceMinimizer adapt_minimizer(
    const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
  return {
      minimizer.added_point_id,
      minimizer.support_point_ids,
      minimizer.support_point_count,
      minimizer.center,
      minimizer.squared_level,
      minimizer.added_point_in_source_closed_ball,
      minimizer.added_point_in_selected_positive_support};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceResult adapt_outcome(
    detail::ExactDirectSparseIncidenceEngineOutcome outcome,
    const ExactDirectSparseSuccessiveIncidenceBudget& requested_budget) {
  const ExactDirectSparseFirstIncidenceResult& traversal = outcome.traversal;
  ExactDirectSparseSuccessiveIncidenceResult result;
  result.source_facet_key = traversal.source_facet_key;
  result.exclusive_lower_squared_level =
      std::move(outcome.exclusive_lower_squared_level);
  result.requested_budget = requested_budget;
  result.traversal_order = traversal.traversal_order;
  result.source_facet_miniball = traversal.source_facet_miniball;
  result.next_incidence_squared_level =
      traversal.first_incidence_squared_level;
  result.cominimizers.reserve(traversal.cominimizers.size());
  for (const ExactDirectSparseFirstIncidenceMinimizer& minimizer :
       traversal.cominimizers) {
    result.cominimizers.push_back(adapt_minimizer(minimizer));
  }
  result.audit = adapt_audit(
      traversal.audit,
      outcome.at_or_below_exclusive_threshold_point_count);
  result.stop_reason = adapt_stop_reason(traversal.stop_reason);
  result.decision = adapt_decision(traversal.decision);
  result.scope = ExactDirectSparseSuccessiveIncidenceScope::
      single_supplied_facet_next_strict_one_point_coface_level_only;
  result.trusted_authorities_certified =
      traversal.trusted_authorities_certified;
  result.source_facet_miniball_freshly_certified =
      traversal.source_facet_miniball_freshly_certified;
  result.every_nonexcluded_point_evaluated_or_strictly_pruned =
      traversal.every_nonexcluded_point_evaluated_or_strictly_pruned;
  result.aabb_lower_bounds_exact_and_valid =
      traversal.aabb_lower_bounds_exact_and_valid;
  result.equality_bounds_always_descended =
      traversal.equality_bounds_always_descended;
  result.every_strict_outside_coface_support_contains_added_point =
      traversal.every_strict_outside_coface_support_contains_added_point;
  result.all_cominimizers_retained_atomically =
      traversal.all_cominimizers_retained_atomically;
  result.no_partial_next_incidence_payload_published =
      traversal.no_partial_first_incidence_payload_published;
  result.no_global_facet_or_coface_catalog_materialized =
      traversal.no_global_facet_or_coface_catalog_materialized;
  result.no_gamma_or_higher_order_delaunay_materialized =
      traversal.no_gamma_or_higher_order_delaunay_materialized;
  result.public_status_claimed = traversal.public_status_claimed;
  result.partial_refinement_only = traversal.partial_refinement_only;
  return result;
}

[[nodiscard]] bool audit_within_budget(
    const ExactDirectSparseSuccessiveIncidenceAudit& audit,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget) noexcept {
  return audit.source_support_enumeration_count <=
             budget.maximum_source_support_enumeration_count &&
         audit.node_visit_count <= budget.maximum_node_visit_count &&
         audit.internal_node_expansion_count <=
             budget.maximum_internal_node_expansion_count &&
         audit.exact_aabb_bound_evaluation_count <=
             budget.maximum_exact_aabb_bound_evaluation_count &&
         audit.exact_point_evaluation_count <=
             budget.maximum_exact_point_evaluation_count &&
         audit.coface_support_enumeration_count <=
             budget.maximum_coface_support_enumeration_count &&
         audit.candidate_point_classification_count <=
             budget.maximum_candidate_point_classification_count &&
         audit.peak_frontier_entry_count <=
             budget.maximum_frontier_entry_count &&
         audit.peak_cominimizer_entry_count <=
             budget.maximum_cominimizer_count;
}

[[nodiscard]] bool audit_covers_eligible_points(
    const ExactDirectSparseSuccessiveIncidenceAudit& audit) noexcept {
  return audit.inside_or_boundary_source_ball_point_count <=
             audit.exact_point_evaluation_count &&
         audit.outside_source_ball_point_count ==
             audit.exact_point_evaluation_count -
                 audit.inside_or_boundary_source_ball_point_count &&
         audit.exact_point_evaluation_count <=
             audit.eligible_coface_point_count &&
         audit.pruned_eligible_point_count ==
             audit.eligible_coface_point_count -
                 audit.exact_point_evaluation_count;
}

[[nodiscard]] bool common_result_contract(
    const ExactDirectSparseSuccessiveIncidenceResult& result) noexcept {
  return result.schema_version ==
             direct_sparse_successive_incidence_schema_version &&
         result.trusted_authorities_certified &&
         result.aabb_lower_bounds_exact_and_valid &&
         result.equality_bounds_always_descended &&
         result.every_strict_outside_coface_support_contains_added_point &&
         result.no_partial_next_incidence_payload_published &&
         result.no_global_facet_or_coface_catalog_materialized &&
         result.no_gamma_or_higher_order_delaunay_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope == ExactDirectSparseSuccessiveIncidenceScope::
             single_supplied_facet_next_strict_one_point_coface_level_only &&
         result.audit.excluded_facet_point_count ==
             result.source_facet_key.point_count &&
         result.audit.supplied_incumbent_seed_point_count <=
             result.audit.eligible_coface_point_count &&
         result.audit.exact_incumbent_seed_evaluation_count <=
             result.audit.supplied_incumbent_seed_point_count &&
         result.audit.exact_incumbent_seed_evaluation_count <=
             result.audit.exact_point_evaluation_count &&
         result.audit.at_or_below_exclusive_threshold_point_count <=
             result.audit.exact_point_evaluation_count &&
         (result.exclusive_lower_squared_level.has_value() ||
          result.audit.at_or_below_exclusive_threshold_point_count == 0U) &&
         result.cominimizers.size() <=
             result.audit.peak_cominimizer_entry_count &&
         audit_within_budget(result.audit, result.requested_budget);
}

[[nodiscard]] bool minimizer_shape_valid(
    const ExactDirectSparseSuccessiveIncidenceMinimizer& minimizer,
    const ExactDirectSparseFacetKey& source_key,
    const exact::ExactLevel& common_level) noexcept {
  if (minimizer.support_point_count == 0U ||
      minimizer.support_point_count > minimizer.support_point_ids.size() ||
      minimizer.squared_level != common_level ||
      minimizer.added_point_in_source_closed_ball ==
          minimizer.added_point_in_selected_positive_support) {
    return false;
  }
  const auto source_begin = source_key.point_ids.begin();
  const auto source_end = source_begin +
      static_cast<std::ptrdiff_t>(source_key.point_count);
  if (std::binary_search(
          source_begin, source_end, minimizer.added_point_id)) {
    return false;
  }
  bool support_contains_added = false;
  for (std::size_t index = 0U;
       index < minimizer.support_point_count;
       ++index) {
    const PointId point_id = minimizer.support_point_ids[index];
    if ((index != 0U &&
         minimizer.support_point_ids[index - 1U] >= point_id) ||
        (point_id != minimizer.added_point_id &&
         !std::binary_search(source_begin, source_end, point_id))) {
      return false;
    }
    support_contains_added =
        support_contains_added || point_id == minimizer.added_point_id;
  }
  for (std::size_t index = minimizer.support_point_count;
       index < minimizer.support_point_ids.size();
       ++index) {
    if (minimizer.support_point_ids[index] != 0U) {
      return false;
    }
  }
  return support_contains_added ==
         minimizer.added_point_in_selected_positive_support;
}

[[nodiscard]] bool observed_storage_within_bounds(
    const ExactDirectSparseSuccessiveIncidenceResult& observed,
    std::size_t eligible_point_count,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget) noexcept {
  if (!audit_within_budget(observed.audit, budget) ||
      observed.cominimizers.size() > eligible_point_count ||
      observed.cominimizers.size() > budget.maximum_cominimizer_count) {
    return false;
  }
  if (observed.source_facet_miniball.has_value()) {
    const ExactFacetMiniballResult& source =
        *observed.source_facet_miniball;
    if (source.facet_point_ids.size() >
            direct_sparse_successive_incidence_maximum_source_point_count ||
        source.support_point_ids.size() >
            ExactFacetMiniballResult::maximum_support_point_count ||
        source.strictly_inside_point_ids.size() >
            direct_sparse_successive_incidence_maximum_source_point_count ||
        source.boundary_point_ids.size() >
            direct_sparse_successive_incidence_maximum_source_point_count) {
      return false;
    }
  }
  return std::all_of(
      observed.cominimizers.begin(),
      observed.cominimizers.end(),
      [](const ExactDirectSparseSuccessiveIncidenceMinimizer& minimizer) {
        if (minimizer.support_point_count == 0U ||
            minimizer.support_point_count >
                minimizer.support_point_ids.size()) {
          return false;
        }
        return std::all_of(
            minimizer.support_point_ids.begin() +
                static_cast<std::ptrdiff_t>(minimizer.support_point_count),
            minimizer.support_point_ids.end(),
            [](PointId point_id) { return point_id == 0U; });
      });
}

}  // namespace

bool ExactDirectSparseSuccessiveIncidenceResult::
    certified_complete_next_incidence() const noexcept {
  if (!common_result_contract(*this) ||
      !source_facet_miniball.has_value() ||
      !source_facet_miniball_freshly_certified ||
      !next_incidence_squared_level.has_value() || cominimizers.empty() ||
      !audit.traversal_complete || !audit_covers_eligible_points(audit) ||
      audit.exact_incumbent_seed_evaluation_count !=
          audit.supplied_incumbent_seed_point_count ||
      !every_nonexcluded_point_evaluated_or_strictly_pruned ||
      !all_cominimizers_retained_atomically ||
      stop_reason != ExactDirectSparseSuccessiveIncidenceStopReason::none ||
      decision != ExactDirectSparseSuccessiveIncidenceDecision::
                      complete_exact_next_incidence ||
      (exclusive_lower_squared_level.has_value() &&
       *next_incidence_squared_level <= *exclusive_lower_squared_level)) {
    return false;
  }
  PointId previous_point_id = 0U;
  bool first = true;
  for (const ExactDirectSparseSuccessiveIncidenceMinimizer& minimizer :
       cominimizers) {
    if (!minimizer_shape_valid(
            minimizer, source_facet_key, *next_incidence_squared_level) ||
        (!first && minimizer.added_point_id <= previous_point_id)) {
      return false;
    }
    previous_point_id = minimizer.added_point_id;
    first = false;
  }
  return true;
}

bool ExactDirectSparseSuccessiveIncidenceResult::
    certified_complete_no_strictly_higher_coface() const noexcept {
  return common_result_contract(*this) &&
         source_facet_miniball.has_value() &&
         source_facet_miniball_freshly_certified &&
         audit.exact_incumbent_seed_evaluation_count ==
             audit.supplied_incumbent_seed_point_count &&
         audit.traversal_complete && audit_covers_eligible_points(audit) &&
         every_nonexcluded_point_evaluated_or_strictly_pruned &&
         all_cominimizers_retained_atomically &&
         !next_incidence_squared_level.has_value() && cominimizers.empty() &&
         stop_reason == ExactDirectSparseSuccessiveIncidenceStopReason::none &&
         decision == ExactDirectSparseSuccessiveIncidenceDecision::
                         complete_no_strictly_higher_coface &&
         ((!exclusive_lower_squared_level.has_value() &&
           audit.eligible_coface_point_count == 0U) ||
          (exclusive_lower_squared_level.has_value() &&
           audit.at_or_below_exclusive_threshold_point_count ==
               audit.exact_point_evaluation_count));
}

bool ExactDirectSparseSuccessiveIncidenceResult::certified_budget_exhaustion()
    const noexcept {
  return common_result_contract(*this) &&
         !next_incidence_squared_level.has_value() && cominimizers.empty() &&
         !all_cominimizers_retained_atomically &&
         stop_reason != ExactDirectSparseSuccessiveIncidenceStopReason::none &&
         decision == ExactDirectSparseSuccessiveIncidenceDecision::
                         no_next_incidence_budget_exhausted;
}

ExactDirectSparseSuccessiveIncidenceResult
build_exact_direct_sparse_successive_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  return build_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
      index,
      cloud,
      source_facet_key,
      std::span<const PointId>{},
      std::move(exclusive_lower_squared_level),
      budget,
      traversal_order);
}

ExactDirectSparseSuccessiveIncidenceResult
build_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::span<const PointId> incumbent_seed_point_ids,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  detail::ExactDirectSparseIncidenceEngineOutcome outcome =
      detail::build_exact_direct_sparse_incidence_engine(
          index,
          cloud,
          source_facet_key,
          incumbent_seed_point_ids,
          std::move(exclusive_lower_squared_level),
          engine_budget(budget),
          traversal_order);
  ExactDirectSparseSuccessiveIncidenceResult result =
      adapt_outcome(std::move(outcome), budget);
  if (!result.certified_complete_next_incidence() &&
      !result.certified_complete_no_strictly_higher_coface() &&
      !result.certified_budget_exhaustion()) {
    throw std::logic_error(
        "a direct sparse successive incidence produced no certified outcome");
  }
  return result;
}

ExactDirectSparseSuccessiveIncidenceVerification
verify_exact_direct_sparse_successive_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceResult& observed) {
  return verify_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
      index,
      cloud,
      source_facet_key,
      std::span<const PointId>{},
      std::move(exclusive_lower_squared_level),
      budget,
      traversal_order,
      observed);
}

ExactDirectSparseSuccessiveIncidenceVerification
verify_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::span<const PointId> incumbent_seed_point_ids,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceResult& observed) {
  ExactDirectSparseSuccessiveIncidenceVerification verification;
  const std::size_t eligible_point_count =
      cloud.size() >= source_facet_key.point_count
          ? cloud.size() - source_facet_key.point_count
          : 0U;
  verification.observed_storage_within_budget =
      observed_storage_within_bounds(observed, eligible_point_count, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }

  const ExactDirectSparseSuccessiveIncidenceResult expected =
      build_exact_direct_sparse_successive_incidence_with_incumbent_seeds(
          index,
          cloud,
          source_facet_key,
          incumbent_seed_point_ids,
          std::move(exclusive_lower_squared_level),
          budget,
          traversal_order);
  verification.trusted_inputs_certified = true;
  verification.source_miniball_freshly_replayed =
      observed.source_facet_miniball == expected.source_facet_miniball &&
      observed.source_facet_miniball_freshly_certified ==
          expected.source_facet_miniball_freshly_certified;
  verification.branch_and_bound_freshly_replayed =
      observed.audit == expected.audit &&
      observed.next_incidence_squared_level ==
          expected.next_incidence_squared_level &&
      observed.every_nonexcluded_point_evaluated_or_strictly_pruned ==
          expected.every_nonexcluded_point_evaluated_or_strictly_pruned &&
      observed.aabb_lower_bounds_exact_and_valid ==
          expected.aabb_lower_bounds_exact_and_valid &&
      observed.equality_bounds_always_descended ==
          expected.equality_bounds_always_descended;
  verification.all_cominimizers_freshly_replayed =
      observed.cominimizers == expected.cominimizers &&
      observed.all_cominimizers_retained_atomically ==
          expected.all_cominimizers_retained_atomically &&
      observed.no_partial_next_incidence_payload_published ==
          expected.no_partial_next_incidence_payload_published &&
      observed.every_strict_outside_coface_support_contains_added_point ==
          expected.every_strict_outside_coface_support_contains_added_point;
  verification.counters_and_decision_freshly_replayed =
      observed.schema_version == expected.schema_version &&
      observed.source_facet_key == expected.source_facet_key &&
      observed.exclusive_lower_squared_level ==
          expected.exclusive_lower_squared_level &&
      observed.requested_budget == expected.requested_budget &&
      observed.traversal_order == expected.traversal_order &&
      observed.stop_reason == expected.stop_reason &&
      observed.decision == expected.decision &&
      observed.scope == expected.scope &&
      observed.trusted_authorities_certified ==
          expected.trusted_authorities_certified &&
      observed.public_status_claimed == expected.public_status_claimed &&
      observed.partial_refinement_only == expected.partial_refinement_only;
  verification.no_forbidden_global_structure_materialized =
      observed.no_global_facet_or_coface_catalog_materialized &&
      observed.no_gamma_or_higher_order_delaunay_materialized &&
      observed.no_global_facet_or_coface_catalog_materialized ==
          expected.no_global_facet_or_coface_catalog_materialized &&
      observed.no_gamma_or_higher_order_delaunay_materialized ==
          expected.no_gamma_or_higher_order_delaunay_materialized;
  verification.fresh_replay_certified =
      verification.trusted_inputs_certified &&
      verification.observed_storage_within_budget &&
      verification.source_miniball_freshly_replayed &&
      verification.branch_and_bound_freshly_replayed &&
      verification.all_cominimizers_freshly_replayed &&
      verification.counters_and_decision_freshly_replayed &&
      verification.no_forbidden_global_structure_materialized;
  verification.result_certified =
      verification.fresh_replay_certified &&
      (expected.certified_complete_next_incidence() ||
       expected.certified_complete_no_strictly_higher_coface() ||
       expected.certified_budget_exhaustion());
  return verification;
}

}  // namespace morsehgp3d::hierarchy
