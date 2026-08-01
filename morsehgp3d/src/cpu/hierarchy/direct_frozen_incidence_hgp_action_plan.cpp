#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"

#include "direct_frozen_incidence_hgp_action_plan_internal.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] bool valid_provenance_kind(
    ExactFrozenIncidenceHyperedgeProvenanceKind kind) noexcept {
  switch (kind) {
    case ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family:
    case ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence:
      return true;
  }
  return false;
}

[[nodiscard]] bool budget_is_sufficient(
    const ExactFrozenIncidenceHgpActionPlanResult& result) noexcept {
  return result.required_source_hyperedge_capacity <=
             result.requested_budget.maximum_source_hyperedge_count &&
         result.required_provenance_capacity <=
             result.requested_budget.maximum_provenance_count &&
         result.required_root_attachment_capacity <=
             result.requested_budget.maximum_root_attachment_count &&
         result.required_group_capacity <=
             result.requested_budget.maximum_group_count &&
         result.required_direct_hyperedge_reference_capacity <=
             result.requested_budget
                 .maximum_direct_hyperedge_reference_count &&
         result.required_residual_hyperedge_reference_capacity <=
             result.requested_budget
                 .maximum_residual_hyperedge_reference_count &&
         result.required_prior_root_reference_capacity <=
             result.requested_budget.maximum_prior_root_reference_count &&
         result.required_scratch_entry_capacity <=
             result.requested_budget.maximum_scratch_entry_count &&
         result.required_logical_output_entry_capacity <=
             result.requested_budget.maximum_logical_output_entry_count;
}

[[nodiscard]] ExactFrozenIncidenceHgpActionPlanResult
make_requirement_result(
    std::size_t source_hyperedge_count,
    std::size_t provenance_count,
    std::size_t root_attachment_count,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget) noexcept {
  ExactFrozenIncidenceHgpActionPlanResult result;
  result.requested_budget = budget;
  result.scope = ExactFrozenIncidenceHgpActionPlanScope::
      exact_bounded_action_classification_from_fresh_frozen_incidence_quotient_only;
  result.required_source_hyperedge_capacity = source_hyperedge_count;
  result.required_provenance_capacity = provenance_count;
  result.required_root_attachment_capacity = root_attachment_count;
  result.required_group_capacity = source_hyperedge_count;
  result.required_direct_hyperedge_reference_capacity =
      source_hyperedge_count;
  result.required_residual_hyperedge_reference_capacity =
      source_hyperedge_count;
  result.required_prior_root_reference_capacity = root_attachment_count;

  const auto triple_hyperedge_count =
      checked_multiply(3U, source_hyperedge_count);
  const auto scratch_capacity = triple_hyperedge_count.has_value()
                                    ? checked_add(
                                          *triple_hyperedge_count,
                                          root_attachment_count)
                                    : std::nullopt;
  const auto double_hyperedge_count =
      checked_multiply(2U, source_hyperedge_count);
  const auto output_capacity = double_hyperedge_count.has_value()
                                   ? checked_add(
                                         *double_hyperedge_count,
                                         root_attachment_count)
                                   : std::nullopt;
  if (!scratch_capacity.has_value() || !output_capacity.has_value()) {
    result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
        no_frozen_incidence_hgp_action_plan_capacity_overflow;
    return result;
  }
  result.required_scratch_entry_capacity = *scratch_capacity;
  result.required_logical_output_entry_capacity = *output_capacity;
  if (!budget_is_sufficient(result)) {
    result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
        no_frozen_incidence_hgp_action_plan_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;
  return result;
}

struct RootAttachmentByPriorRoot {
  ExactFrozenIncidencePriorRootId prior_root_id{};
  ExactFrozenIncidenceTokenId rooted_carrier_token_id{};
  std::size_t group_index{};
};

struct FrozenIncidenceHgpActionPlanAnalysis {
  ExactFrozenIncidenceHgpActionPlanResult facts;
  std::vector<ExactFrozenIncidenceHgpActionGroup> group_facts;
  std::vector<RootAttachmentByPriorRoot> attachments_by_prior_root;
};

[[nodiscard]] ExactFrozenIncidenceHgpAction action_for_q_r(
    std::size_t q_r) noexcept {
  if (q_r == 0U) {
    return ExactFrozenIncidenceHgpAction::reduced_birth;
  }
  if (q_r == 1U) {
    return ExactFrozenIncidenceHgpAction::continuation;
  }
  return ExactFrozenIncidenceHgpAction::multifusion;
}

[[nodiscard]] bool provenance_record_is_well_formed(
    const ExactFrozenIncidenceHyperedgeProvenance& record,
    std::size_t expected_index) noexcept {
  if (record.source_hyperedge_index != expected_index ||
      !valid_provenance_kind(record.kind) ||
      (record.zero_point_delta_certified &&
       record.added_point_count != 0U)) {
    return false;
  }
  if (record.fully_redundant &&
      (record.kind != ExactFrozenIncidenceHyperedgeProvenanceKind::
                          residual_incidence ||
       !record.zero_point_delta_certified ||
       record.added_point_count != 0U)) {
    return false;
  }
  return true;
}

[[nodiscard]] FrozenIncidenceHgpActionPlanAnalysis
analyze_frozen_incidence_hgp_action_plan(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget,
    const internal::ExactFrozenIncidenceVerifiedQuotientAuthority*
        verified_quotient_authority) {
  FrozenIncidenceHgpActionPlanAnalysis analysis;
  analysis.facts = make_requirement_result(
      quotient_hyperedges.size(),
      provenance.size(),
      root_attachments.size(),
      budget);
  ExactFrozenIncidenceHgpActionPlanResult& result = analysis.facts;
  if (result.decision !=
      ExactFrozenIncidenceHgpActionPlanDecision::not_certified) {
    return analysis;
  }

  const bool quotient_certified =
      verified_quotient_authority != nullptr
          ? verified_quotient_authority->valid()
          : verify_exact_direct_frozen_incidence_quotient_streaming(
                quotient_hyperedges,
                quotient_token_references,
                trusted_quotient_budget,
                quotient)
                .result_certified;
  if (!quotient_certified) {
    result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
        no_frozen_incidence_hgp_action_plan_quotient_rejected;
    return analysis;
  }
  result.source_quotient_fresh_streaming_verified = true;

  if (provenance.size() != quotient_hyperedges.size()) {
    result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
        no_frozen_incidence_hgp_action_plan_input_shape_rejected;
    return analysis;
  }
  for (std::size_t index = 0U; index < provenance.size(); ++index) {
    if (!provenance_record_is_well_formed(provenance[index], index)) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_input_shape_rejected;
      return analysis;
    }
  }
  result.provenance_shape_and_local_deltas_certified = true;

  const std::size_t rooted_carrier_count =
      quotient.counters.distinct_rooted_carrier_count;
  if (root_attachments.size() != rooted_carrier_count ||
      quotient.token_bindings.size() < rooted_carrier_count) {
    result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
        no_frozen_incidence_hgp_action_plan_input_shape_rejected;
    return analysis;
  }
  analysis.attachments_by_prior_root.reserve(root_attachments.size());
  for (std::size_t index = 0U; index < root_attachments.size(); ++index) {
    const ExactFrozenIncidenceTokenBinding& token_binding =
        quotient.token_bindings[index];
    const ExactFrozenIncidenceRootAttachment& attachment =
        root_attachments[index];
    if (token_binding.token.kind !=
            ExactFrozenIncidenceTokenKind::rooted_carrier ||
        token_binding.token.token_id !=
            attachment.rooted_carrier_token_id ||
        token_binding.group_index >= quotient.groups.size()) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_input_shape_rejected;
      return analysis;
    }
    analysis.attachments_by_prior_root.push_back(
        {attachment.prior_root_id,
         attachment.rooted_carrier_token_id,
         token_binding.group_index});
  }
  std::sort(
      analysis.attachments_by_prior_root.begin(),
      analysis.attachments_by_prior_root.end(),
      [](const RootAttachmentByPriorRoot& left,
         const RootAttachmentByPriorRoot& right) {
        return left.prior_root_id < right.prior_root_id ||
               (left.prior_root_id == right.prior_root_id &&
                left.rooted_carrier_token_id <
                    right.rooted_carrier_token_id);
      });
  for (std::size_t index = 1U;
       index < analysis.attachments_by_prior_root.size();
       ++index) {
    if (analysis.attachments_by_prior_root[index - 1U].prior_root_id ==
        analysis.attachments_by_prior_root[index].prior_root_id) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_input_shape_rejected;
      return analysis;
    }
  }
  result.rooted_carrier_attachments_exhaustive_and_injective = true;

  analysis.group_facts.resize(quotient.groups.size());
  for (std::size_t group_index = 0U;
       group_index < quotient.groups.size();
       ++group_index) {
    ExactFrozenIncidenceHgpActionGroup& group =
        analysis.group_facts[group_index];
    group.group_index = group_index;
    group.q_r = quotient.groups[group_index].rooted_carrier_count;
    group.residual_zero_point_delta_certified = true;
    group.fully_redundant_residual_group = true;
  }

  for (std::size_t index = 0U; index < provenance.size(); ++index) {
    const ExactFrozenIncidenceHyperedgeProvenance& source =
        provenance[index];
    const std::size_t group_index =
        quotient.hyperedge_bindings[index].group_index;
    if (group_index >= analysis.group_facts.size()) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_input_shape_rejected;
      return analysis;
    }
    ExactFrozenIncidenceHgpActionGroup& group =
        analysis.group_facts[group_index];
    if (source.kind ==
        ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family) {
      ++group.direct_hyperedge_count;
      const auto group_delta = checked_add(
          group.direct_added_point_count, source.added_point_count);
      const auto total_delta = checked_add(
          result.counters.direct_added_point_count,
          source.added_point_count);
      if (!group_delta.has_value() || !total_delta.has_value()) {
        result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
            no_frozen_incidence_hgp_action_plan_capacity_overflow;
        return analysis;
      }
      group.direct_added_point_count = *group_delta;
      result.counters.direct_added_point_count = *total_delta;
    } else {
      ++group.residual_hyperedge_count;
      const auto group_delta = checked_add(
          group.residual_added_point_count, source.added_point_count);
      const auto total_delta = checked_add(
          result.counters.residual_added_point_count,
          source.added_point_count);
      if (!group_delta.has_value() || !total_delta.has_value()) {
        result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
            no_frozen_incidence_hgp_action_plan_capacity_overflow;
        return analysis;
      }
      group.residual_added_point_count = *group_delta;
      result.counters.residual_added_point_count = *total_delta;
      group.residual_zero_point_delta_certified =
          group.residual_zero_point_delta_certified &&
          source.zero_point_delta_certified &&
          source.added_point_count == 0U;
      group.fully_redundant_residual_group =
          group.fully_redundant_residual_group &&
          source.fully_redundant;
    }
  }

  for (const RootAttachmentByPriorRoot& attachment :
       analysis.attachments_by_prior_root) {
    ++analysis.group_facts[attachment.group_index].prior_root_count;
  }

  std::size_t direct_offset = 0U;
  std::size_t residual_offset = 0U;
  std::size_t prior_root_offset = 0U;
  for (std::size_t group_index = 0U;
       group_index < analysis.group_facts.size();
       ++group_index) {
    ExactFrozenIncidenceHgpActionGroup& group =
        analysis.group_facts[group_index];
    const ExactFrozenIncidenceQuotientGroup& quotient_group =
        quotient.groups[group_index];
    if (quotient_group.rooted_carrier_count == 0U &&
        quotient_group.latent_carrier_count == 0U) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_scope_rejected;
      return analysis;
    }
    if (group.prior_root_count != group.q_r) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_input_shape_rejected;
      return analysis;
    }

    group.direct_hyperedge_offset = direct_offset;
    group.residual_hyperedge_offset = residual_offset;
    group.prior_root_offset = prior_root_offset;
    direct_offset += group.direct_hyperedge_count;
    residual_offset += group.residual_hyperedge_count;
    prior_root_offset += group.prior_root_count;
    group.purely_residual =
        group.direct_hyperedge_count == 0U &&
        group.residual_hyperedge_count != 0U;
    group.mixed_provenance =
        group.direct_hyperedge_count != 0U &&
        group.residual_hyperedge_count != 0U;
    if (group.residual_hyperedge_count == 0U) {
      group.residual_zero_point_delta_certified = false;
      group.fully_redundant_residual_group = false;
    } else if (!group.purely_residual) {
      group.fully_redundant_residual_group = false;
    }
    group.action = action_for_q_r(group.q_r);

    if (group.purely_residual &&
        (group.q_r != 1U ||
         group.residual_added_point_count != 0U ||
         !group.residual_zero_point_delta_certified)) {
      result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_scope_rejected;
      return analysis;
    }

    switch (group.action) {
      case ExactFrozenIncidenceHgpAction::reduced_birth:
        ++result.counters.reduced_birth_group_count;
        break;
      case ExactFrozenIncidenceHgpAction::continuation:
        ++result.counters.continuation_group_count;
        break;
      case ExactFrozenIncidenceHgpAction::multifusion:
        ++result.counters.multifusion_group_count;
        break;
    }
    if (group.purely_residual) {
      ++result.counters.purely_residual_group_count;
    }
    if (group.mixed_provenance) {
      ++result.counters.mixed_provenance_group_count;
    }
    if (group.purely_residual && group.q_r == 1U &&
        group.residual_zero_point_delta_certified) {
      ++result.counters.silent_residual_continuation_group_count;
    }
    if (group.fully_redundant_residual_group) {
      ++result.counters.fully_redundant_residual_group_count;
    }
    result.counters.maximum_group_direct_hyperedge_count = std::max(
        result.counters.maximum_group_direct_hyperedge_count,
        group.direct_hyperedge_count);
    result.counters.maximum_group_residual_hyperedge_count = std::max(
        result.counters.maximum_group_residual_hyperedge_count,
        group.residual_hyperedge_count);
    result.counters.maximum_group_q_r = std::max(
        result.counters.maximum_group_q_r, group.q_r);
  }

  result.counters.source_hyperedge_count = quotient_hyperedges.size();
  result.counters.provenance_count = provenance.size();
  result.counters.root_attachment_count = root_attachments.size();
  result.counters.group_count = analysis.group_facts.size();
  result.counters.direct_hyperedge_reference_count = direct_offset;
  result.counters.residual_hyperedge_reference_count = residual_offset;
  result.counters.prior_root_reference_count = prior_root_offset;
  result.counters.logical_output_entry_count =
      analysis.group_facts.size() + direct_offset + residual_offset +
      prior_root_offset;
  result.counters.logical_scratch_entry_count =
      root_attachments.size() + 3U * analysis.group_facts.size();

  result.groups_and_source_slices_canonical = true;
  result.q_r_actions_certified = true;
  result.pure_residual_scope_rule_certified = true;
  result.carrier_presence_certified = true;
  result.fully_redundant_residual_groups_preserved = true;
  result.decision = ExactFrozenIncidenceHgpActionPlanDecision::
      complete_certified_frozen_incidence_hgp_action_plan;
  return analysis;
}

[[nodiscard]] bool requirements_match(
    const ExactFrozenIncidenceHgpActionPlanResult& expected,
    const ExactFrozenIncidenceHgpActionPlanResult& observed) noexcept {
  return observed.schema_version == expected.schema_version &&
         observed.required_source_hyperedge_capacity ==
             expected.required_source_hyperedge_capacity &&
         observed.required_provenance_capacity ==
             expected.required_provenance_capacity &&
         observed.required_root_attachment_capacity ==
             expected.required_root_attachment_capacity &&
         observed.required_group_capacity ==
             expected.required_group_capacity &&
         observed.required_direct_hyperedge_reference_capacity ==
             expected.required_direct_hyperedge_reference_capacity &&
         observed.required_residual_hyperedge_reference_capacity ==
             expected.required_residual_hyperedge_reference_capacity &&
         observed.required_prior_root_reference_capacity ==
             expected.required_prior_root_reference_capacity &&
         observed.required_scratch_entry_capacity ==
             expected.required_scratch_entry_capacity &&
         observed.required_logical_output_entry_capacity ==
             expected.required_logical_output_entry_capacity;
}

[[nodiscard]] bool result_facts_match(
    const ExactFrozenIncidenceHgpActionPlanResult& expected,
    const ExactFrozenIncidenceHgpActionPlanResult& observed) noexcept {
  return observed.budget_preflight_certified ==
             expected.budget_preflight_certified &&
         observed.source_quotient_fresh_streaming_verified ==
             expected.source_quotient_fresh_streaming_verified &&
         observed.provenance_shape_and_local_deltas_certified ==
             expected.provenance_shape_and_local_deltas_certified &&
         observed.rooted_carrier_attachments_exhaustive_and_injective ==
             expected.rooted_carrier_attachments_exhaustive_and_injective &&
         observed.groups_and_source_slices_canonical ==
             expected.groups_and_source_slices_canonical &&
         observed.q_r_actions_certified ==
             expected.q_r_actions_certified &&
         observed.pure_residual_scope_rule_certified ==
             expected.pure_residual_scope_rule_certified &&
         observed.carrier_presence_certified ==
             expected.carrier_presence_certified &&
         observed.fully_redundant_residual_groups_preserved ==
             expected.fully_redundant_residual_groups_preserved;
}

[[nodiscard]] bool mutation_non_scope_matches(
    const ExactFrozenIncidenceHgpActionPlanResult& expected,
    const ExactFrozenIncidenceHgpActionPlanResult& observed) noexcept {
  return !expected.nodes_created_or_mutated &&
         !expected.locator_or_caller_owned_state_mutated &&
         !expected.geometry_gamma_facets_or_cells_materialized &&
         !expected.public_status_claimed &&
         !observed.nodes_created_or_mutated &&
         !observed.locator_or_caller_owned_state_mutated &&
         !observed.geometry_gamma_facets_or_cells_materialized &&
         !observed.public_status_claimed;
}

[[nodiscard]] bool source_slices_match(
    const FrozenIncidenceHgpActionPlanAnalysis& analysis,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const std::size_t> observed_indices,
    ExactFrozenIncidenceHyperedgeProvenanceKind expected_kind) noexcept {
  const std::size_t expected_total =
      expected_kind ==
              ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family
          ? analysis.facts.counters.direct_hyperedge_reference_count
          : analysis.facts.counters.residual_hyperedge_reference_count;
  if (observed_indices.size() != expected_total) {
    return false;
  }
  for (const ExactFrozenIncidenceHgpActionGroup& group :
       analysis.group_facts) {
    const std::size_t offset =
        expected_kind ==
                ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family
            ? group.direct_hyperedge_offset
            : group.residual_hyperedge_offset;
    const std::size_t count =
        expected_kind ==
                ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family
            ? group.direct_hyperedge_count
            : group.residual_hyperedge_count;
    bool has_previous = false;
    std::size_t previous = 0U;
    for (std::size_t relative_index = 0U;
         relative_index < count;
         ++relative_index) {
      const std::size_t source_index =
          observed_indices[offset + relative_index];
      if (source_index >= provenance.size() ||
          provenance[source_index].kind != expected_kind ||
          quotient.hyperedge_bindings[source_index].group_index !=
              group.group_index ||
          (has_previous && previous >= source_index)) {
        return false;
      }
      previous = source_index;
      has_previous = true;
    }
  }
  return true;
}

[[nodiscard]] bool prior_root_slices_match(
    const FrozenIncidenceHgpActionPlanAnalysis& analysis,
    std::span<const ExactFrozenIncidencePriorRootId> observed_roots) {
  if (observed_roots.size() !=
      analysis.attachments_by_prior_root.size()) {
    return false;
  }
  for (const ExactFrozenIncidenceHgpActionGroup& group :
       analysis.group_facts) {
    bool has_previous = false;
    ExactFrozenIncidencePriorRootId previous{};
    for (std::size_t relative_index = 0U;
         relative_index < group.prior_root_count;
         ++relative_index) {
      const ExactFrozenIncidencePriorRootId root_id =
          observed_roots[group.prior_root_offset + relative_index];
      if (has_previous && previous >= root_id) {
        return false;
      }
      const auto position = std::lower_bound(
          analysis.attachments_by_prior_root.begin(),
          analysis.attachments_by_prior_root.end(),
          root_id,
          [](const RootAttachmentByPriorRoot& attachment,
             ExactFrozenIncidencePriorRootId candidate) {
            return attachment.prior_root_id < candidate;
          });
      if (position == analysis.attachments_by_prior_root.end() ||
          position->prior_root_id != root_id ||
          position->group_index != group.group_index) {
        return false;
      }
      previous = root_id;
      has_previous = true;
    }
  }
  return true;
}

[[nodiscard]] bool partition_matches(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t total) noexcept {
  return first <= total && second <= total - first &&
         third == total - first - second;
}

}  // namespace

bool ExactFrozenIncidenceHgpActionPlanResult::
    certified_frozen_incidence_hgp_action_plan() const noexcept {
  constexpr ExactFrozenIncidenceHgpActionPlanScope certified_scope =
      ExactFrozenIncidenceHgpActionPlanScope::
          exact_bounded_action_classification_from_fresh_frozen_incidence_quotient_only;
  return schema_version ==
             direct_frozen_incidence_hgp_action_plan_schema_version &&
         budget_preflight_certified &&
         source_quotient_fresh_streaming_verified &&
         provenance_shape_and_local_deltas_certified &&
         rooted_carrier_attachments_exhaustive_and_injective &&
         groups_and_source_slices_canonical && q_r_actions_certified &&
         pure_residual_scope_rule_certified && carrier_presence_certified &&
         fully_redundant_residual_groups_preserved &&
         !nodes_created_or_mutated &&
         !locator_or_caller_owned_state_mutated &&
         !geometry_gamma_facets_or_cells_materialized &&
         !public_status_claimed && budget_is_sufficient(*this) &&
         required_source_hyperedge_capacity ==
             counters.source_hyperedge_count &&
         required_provenance_capacity == counters.provenance_count &&
         required_root_attachment_capacity ==
             counters.root_attachment_count &&
         counters.group_count <= required_group_capacity &&
         counters.direct_hyperedge_reference_count <=
             required_direct_hyperedge_reference_capacity &&
         counters.residual_hyperedge_reference_count <=
             required_residual_hyperedge_reference_capacity &&
         counters.prior_root_reference_count <=
             required_prior_root_reference_capacity &&
         counters.direct_hyperedge_reference_count <=
             counters.source_hyperedge_count &&
         counters.residual_hyperedge_reference_count ==
             counters.source_hyperedge_count -
                 counters.direct_hyperedge_reference_count &&
         counters.prior_root_reference_count ==
             counters.root_attachment_count &&
         partition_matches(
             counters.reduced_birth_group_count,
             counters.continuation_group_count,
             counters.multifusion_group_count,
             counters.group_count) &&
         counters.purely_residual_group_count <= counters.group_count &&
         counters.mixed_provenance_group_count <= counters.group_count &&
         counters.silent_residual_continuation_group_count <=
             counters.purely_residual_group_count &&
         counters.fully_redundant_residual_group_count <=
             counters.purely_residual_group_count &&
         counters.logical_scratch_entry_count <=
             required_scratch_entry_capacity &&
         groups.size() == counters.group_count &&
         direct_hyperedge_indices.size() ==
             counters.direct_hyperedge_reference_count &&
         residual_hyperedge_indices.size() ==
             counters.residual_hyperedge_reference_count &&
         prior_root_ids.size() == counters.prior_root_reference_count &&
         counters.logical_output_entry_count ==
             groups.size() + direct_hyperedge_indices.size() +
                 residual_hyperedge_indices.size() + prior_root_ids.size() &&
         counters.logical_output_entry_count <=
             required_logical_output_entry_capacity &&
         decision == ExactFrozenIncidenceHgpActionPlanDecision::
                         complete_certified_frozen_incidence_hgp_action_plan &&
         scope == certified_scope;
}

bool ExactFrozenIncidenceHgpActionPlanResult::atomic_empty_failure()
    const noexcept {
  return decision !=
             ExactFrozenIncidenceHgpActionPlanDecision::not_certified &&
         decision != ExactFrozenIncidenceHgpActionPlanDecision::
                         complete_certified_frozen_incidence_hgp_action_plan &&
         groups.empty() && direct_hyperedge_indices.empty() &&
         residual_hyperedge_indices.empty() && prior_root_ids.empty();
}

namespace {

ExactFrozenIncidenceHgpActionPlanResult
build_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget,
    const internal::ExactFrozenIncidenceVerifiedQuotientAuthority*
        verified_quotient_authority) {
  try {
    FrozenIncidenceHgpActionPlanAnalysis analysis =
        analyze_frozen_incidence_hgp_action_plan(
            quotient_hyperedges,
            quotient_token_references,
            trusted_quotient_budget,
            quotient,
            provenance,
            root_attachments,
            budget,
            verified_quotient_authority);
    ExactFrozenIncidenceHgpActionPlanResult result =
        std::move(analysis.facts);
    if (result.decision != ExactFrozenIncidenceHgpActionPlanDecision::
                               complete_certified_frozen_incidence_hgp_action_plan) {
      return result;
    }

    result.groups = std::move(analysis.group_facts);
    result.direct_hyperedge_indices.resize(
        result.counters.direct_hyperedge_reference_count);
    result.residual_hyperedge_indices.resize(
        result.counters.residual_hyperedge_reference_count);
    result.prior_root_ids.resize(
        result.counters.prior_root_reference_count);
    std::vector<std::size_t> direct_cursors(result.groups.size());
    std::vector<std::size_t> residual_cursors(result.groups.size());
    std::vector<std::size_t> root_cursors(result.groups.size());
    for (std::size_t group_index = 0U;
         group_index < result.groups.size();
         ++group_index) {
      direct_cursors[group_index] =
          result.groups[group_index].direct_hyperedge_offset;
      residual_cursors[group_index] =
          result.groups[group_index].residual_hyperedge_offset;
      root_cursors[group_index] =
          result.groups[group_index].prior_root_offset;
    }
    for (std::size_t source_index = 0U;
         source_index < provenance.size();
         ++source_index) {
      const std::size_t group_index =
          quotient.hyperedge_bindings[source_index].group_index;
      if (provenance[source_index].kind ==
          ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family) {
        result.direct_hyperedge_indices[direct_cursors[group_index]] =
            source_index;
        ++direct_cursors[group_index];
      } else {
        result.residual_hyperedge_indices[residual_cursors[group_index]] =
            source_index;
        ++residual_cursors[group_index];
      }
    }
    for (const RootAttachmentByPriorRoot& attachment :
         analysis.attachments_by_prior_root) {
      result.prior_root_ids[root_cursors[attachment.group_index]] =
          attachment.prior_root_id;
      ++root_cursors[attachment.group_index];
    }
    return result;
  } catch (const std::bad_alloc&) {
    ExactFrozenIncidenceHgpActionPlanResult failure =
        make_requirement_result(
            quotient_hyperedges.size(),
            provenance.size(),
            root_attachments.size(),
            budget);
    if (failure.decision ==
        ExactFrozenIncidenceHgpActionPlanDecision::not_certified) {
      failure.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_allocation_failed;
    }
    return failure;
  } catch (const std::length_error&) {
    ExactFrozenIncidenceHgpActionPlanResult failure =
        make_requirement_result(
            quotient_hyperedges.size(),
            provenance.size(),
            root_attachments.size(),
            budget);
    if (failure.decision ==
        ExactFrozenIncidenceHgpActionPlanDecision::not_certified) {
      failure.decision = ExactFrozenIncidenceHgpActionPlanDecision::
          no_frozen_incidence_hgp_action_plan_allocation_failed;
    }
    return failure;
  }
}

ExactFrozenIncidenceHgpActionPlanStreamingVerification
verify_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& trusted_budget,
    const ExactFrozenIncidenceHgpActionPlanResult& observed,
    const internal::ExactFrozenIncidenceVerifiedQuotientAuthority*
        verified_quotient_authority) {
  ExactFrozenIncidenceHgpActionPlanStreamingVerification verification;
  try {
    const FrozenIncidenceHgpActionPlanAnalysis analysis =
        analyze_frozen_incidence_hgp_action_plan(
            quotient_hyperedges,
            quotient_token_references,
            trusted_quotient_budget,
            quotient,
            provenance,
            root_attachments,
            trusted_budget,
            verified_quotient_authority);
    const ExactFrozenIncidenceHgpActionPlanResult& expected =
        analysis.facts;
    const bool replay_complete =
        expected.decision == ExactFrozenIncidenceHgpActionPlanDecision::
                                 complete_certified_frozen_incidence_hgp_action_plan;
    if (replay_complete) {
      verification.source_hyperedge_scan_count =
          quotient_hyperedges.size();
      verification.source_provenance_scan_count = provenance.size();
      verification.source_root_attachment_scan_count =
          root_attachments.size();
    }

    verification.requested_budget_certified =
        observed.requested_budget == trusted_budget;
    verification.requirements_certified =
        requirements_match(expected, observed);
    verification.source_quotient_certified =
        replay_complete &&
        expected.source_quotient_fresh_streaming_verified &&
        observed.source_quotient_fresh_streaming_verified;

    bool groups_match =
        replay_complete &&
        observed.groups.size() == analysis.group_facts.size();
    if (groups_match) {
      for (std::size_t group_index = 0U;
           group_index < analysis.group_facts.size();
           ++group_index) {
        if (observed.groups[group_index] !=
            analysis.group_facts[group_index]) {
          groups_match = false;
          break;
        }
      }
    }
    verification.groups_certified = groups_match;
    verification.direct_hyperedge_slices_certified =
        replay_complete &&
        source_slices_match(
            analysis,
            quotient,
            provenance,
            observed.direct_hyperedge_indices,
            ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family);
    verification.residual_hyperedge_slices_certified =
        replay_complete &&
        source_slices_match(
            analysis,
            quotient,
            provenance,
            observed.residual_hyperedge_indices,
            ExactFrozenIncidenceHyperedgeProvenanceKind::
                residual_incidence);
    verification.prior_root_slices_certified =
        replay_complete &&
        prior_root_slices_match(analysis, observed.prior_root_ids);
    verification.counters_certified =
        replay_complete && observed.counters == expected.counters;
    verification.result_facts_certified =
        replay_complete && result_facts_match(expected, observed);
    verification.mutation_non_scope_certified =
        replay_complete && mutation_non_scope_matches(expected, observed);
    verification.decision_and_scope_certified =
        observed.decision == expected.decision &&
        observed.scope == expected.scope;
    verification.no_second_persistent_output_arena_certified =
        expected.groups.empty() &&
        expected.direct_hyperedge_indices.empty() &&
        expected.residual_hyperedge_indices.empty() &&
        expected.prior_root_ids.empty();
    verification.fresh_streaming_replay_certified =
        replay_complete && verification.requested_budget_certified &&
        verification.requirements_certified &&
        verification.source_quotient_certified &&
        verification.groups_certified &&
        verification.direct_hyperedge_slices_certified &&
        verification.residual_hyperedge_slices_certified &&
        verification.prior_root_slices_certified &&
        verification.counters_certified &&
        verification.result_facts_certified &&
        verification.mutation_non_scope_certified &&
        verification.decision_and_scope_certified &&
        verification.no_second_persistent_output_arena_certified;
    verification.result_certified =
        verification.fresh_streaming_replay_certified &&
        observed.certified_frozen_incidence_hgp_action_plan();
    return verification;
  } catch (const std::bad_alloc&) {
    return verification;
  } catch (const std::length_error&) {
    return verification;
  }
}

}  // namespace

ExactFrozenIncidenceHgpActionPlanResult
build_exact_direct_frozen_incidence_hgp_action_plan(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget) {
  return
      build_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
          quotient_hyperedges,
          quotient_token_references,
          trusted_quotient_budget,
          quotient,
          provenance,
          root_attachments,
          budget,
          nullptr);
}

ExactFrozenIncidenceHgpActionPlanStreamingVerification
verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
    std::span<const ExactFrozenIncidenceHyperedge> quotient_hyperedges,
    std::span<const ExactFrozenIncidenceToken> quotient_token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_quotient_budget,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& trusted_budget,
    const ExactFrozenIncidenceHgpActionPlanResult& observed) {
  return
      verify_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
          quotient_hyperedges,
          quotient_token_references,
          trusted_quotient_budget,
          quotient,
          provenance,
          root_attachments,
          trusted_budget,
          observed,
          nullptr);
}

namespace internal {

std::optional<ExactFrozenIncidenceVerifiedQuotientAuthority>
ExactFrozenIncidenceVerifiedQuotientAuthorityFactory::verify_once(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> tokens,
    const ExactFrozenIncidenceQuotientBudget& budget,
    const ExactFrozenIncidenceQuotientResult& quotient) {
  const auto verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          hyperedges, tokens, budget, quotient);
  if (!verification.result_certified) {
    return std::nullopt;
  }
  return ExactFrozenIncidenceVerifiedQuotientAuthority{
      hyperedges, tokens, budget, &quotient};
}

ExactFrozenIncidenceHgpActionPlanResult
build_exact_direct_frozen_incidence_hgp_action_plan_from_verified_quotient(
    const ExactFrozenIncidenceVerifiedQuotientAuthority& authority,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget) {
  return
      build_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
          authority.hyperedges(),
          authority.tokens(),
          authority.budget(),
          authority.quotient(),
          provenance,
          root_attachments,
          budget,
          &authority);
}

ExactFrozenIncidenceHgpActionPlanStreamingVerification
verify_exact_direct_frozen_incidence_hgp_action_plan_streaming_from_verified_quotient(
    const ExactFrozenIncidenceVerifiedQuotientAuthority& authority,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& trusted_budget,
    const ExactFrozenIncidenceHgpActionPlanResult& observed) {
  return
      verify_frozen_incidence_hgp_action_plan_with_optional_quotient_authority(
          authority.hyperedges(),
          authority.tokens(),
          authority.budget(),
          authority.quotient(),
          provenance,
          root_attachments,
          trusted_budget,
          observed,
          &authority);
}

}  // namespace internal

}  // namespace morsehgp3d::hierarchy
