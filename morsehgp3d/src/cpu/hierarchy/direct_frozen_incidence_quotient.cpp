#include "morsehgp3d/hierarchy/direct_frozen_incidence_quotient.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] bool valid_token_kind(
    ExactFrozenIncidenceTokenKind kind) noexcept {
  switch (kind) {
    case ExactFrozenIncidenceTokenKind::rooted_carrier:
    case ExactFrozenIncidenceTokenKind::latent_carrier:
    case ExactFrozenIncidenceTokenKind::equal_facet:
      return true;
  }
  return false;
}

struct TypedTokenLess {
  [[nodiscard]] bool operator()(
      const ExactFrozenIncidenceToken& left,
      const ExactFrozenIncidenceToken& right) const noexcept {
    const auto left_kind = static_cast<std::uint8_t>(left.kind);
    const auto right_kind = static_cast<std::uint8_t>(right.kind);
    return left_kind < right_kind ||
           (left_kind == right_kind && left.token_id < right.token_id);
  }
};

class CanonicalLocalDisjointSet {
 public:
  explicit CanonicalLocalDisjointSet(std::size_t count) : parent_(count) {
    std::iota(parent_.begin(), parent_.end(), 0U);
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[value] != value) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  void unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left != right) {
      parent_[std::max(left, right)] = std::min(left, right);
    }
  }

 private:
  std::vector<std::size_t> parent_;
};

struct FrozenIncidenceQuotientAnalysis {
  ExactFrozenIncidenceQuotientResult facts;
  std::vector<ExactFrozenIncidenceToken> sorted_tokens;
  std::vector<std::size_t> group_by_local_token;
  std::vector<ExactFrozenIncidenceQuotientGroup> group_facts;
};

[[nodiscard]] bool budget_is_sufficient(
    const ExactFrozenIncidenceQuotientResult& result) noexcept {
  return result.required_hyperedge_capacity <=
             result.requested_budget.maximum_hyperedge_count &&
         result.required_token_reference_capacity <=
             result.requested_budget.maximum_token_reference_count &&
         result.required_distinct_token_capacity <=
             result.requested_budget.maximum_distinct_token_count &&
         result.required_group_capacity <=
             result.requested_budget.maximum_group_count &&
         result.required_hyperedge_binding_capacity <=
             result.requested_budget.maximum_hyperedge_binding_count &&
         result.required_token_binding_capacity <=
             result.requested_budget.maximum_token_binding_count &&
         result.required_group_token_capacity <=
             result.requested_budget.maximum_group_token_count &&
         result.required_scratch_entry_capacity <=
             result.requested_budget.maximum_scratch_entry_count &&
         result.required_logical_output_entry_capacity <=
             result.requested_budget.maximum_logical_output_entry_count;
}

[[nodiscard]] ExactFrozenIncidenceQuotientResult make_requirement_result(
    std::size_t hyperedge_count,
    std::size_t token_reference_count,
    const ExactFrozenIncidenceQuotientBudget& budget) noexcept {
  ExactFrozenIncidenceQuotientResult result;
  result.requested_budget = budget;
  result.scope = ExactFrozenIncidenceQuotientScope::
      exact_equivalence_closure_of_externally_certified_frozen_typed_tokens_only;
  result.required_hyperedge_capacity = hyperedge_count;
  result.required_token_reference_capacity = token_reference_count;
  result.required_distinct_token_capacity = token_reference_count;
  result.required_group_capacity = hyperedge_count;
  result.required_hyperedge_binding_capacity = hyperedge_count;
  result.required_token_binding_capacity = token_reference_count;
  result.required_group_token_capacity = token_reference_count;

  const auto triple_token_count =
      checked_multiply(3U, token_reference_count);
  const auto scratch_capacity = triple_token_count.has_value()
                                    ? checked_add(
                                          *triple_token_count,
                                          hyperedge_count)
                                    : std::nullopt;
  const auto double_token_count =
      checked_multiply(2U, token_reference_count);
  const auto double_hyperedge_count =
      checked_multiply(2U, hyperedge_count);
  const auto output_capacity =
      double_token_count.has_value() && double_hyperedge_count.has_value()
          ? checked_add(*double_token_count, *double_hyperedge_count)
          : std::nullopt;
  if (!scratch_capacity.has_value() || !output_capacity.has_value()) {
    result.decision = ExactFrozenIncidenceQuotientDecision::
        no_frozen_incidence_quotient_capacity_overflow;
    return result;
  }
  result.required_scratch_entry_capacity = *scratch_capacity;
  result.required_logical_output_entry_capacity = *output_capacity;
  if (!budget_is_sufficient(result)) {
    result.decision = ExactFrozenIncidenceQuotientDecision::
        no_frozen_incidence_quotient_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;
  return result;
}

[[nodiscard]] bool valid_canonical_csr_and_token_kinds(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    std::size_t& maximum_hyperedge_reference_count) noexcept {
  std::size_t expected_offset = 0U;
  for (std::size_t index = 0U; index < hyperedges.size(); ++index) {
    const ExactFrozenIncidenceHyperedge& hyperedge = hyperedges[index];
    if (hyperedge.hyperedge_index != index ||
        hyperedge.token_reference_offset != expected_offset ||
        hyperedge.token_reference_count == 0U) {
      return false;
    }
    const auto next_offset = checked_add(
        expected_offset, hyperedge.token_reference_count);
    if (!next_offset.has_value() ||
        *next_offset > token_references.size()) {
      return false;
    }
    expected_offset = *next_offset;
    maximum_hyperedge_reference_count = std::max(
        maximum_hyperedge_reference_count,
        hyperedge.token_reference_count);
  }
  if (expected_offset != token_references.size()) {
    return false;
  }
  for (const ExactFrozenIncidenceToken& token : token_references) {
    if (!valid_token_kind(token.kind)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::size_t local_token_index(
    const std::vector<ExactFrozenIncidenceToken>& sorted_tokens,
    const ExactFrozenIncidenceToken& token) {
  return static_cast<std::size_t>(
      std::lower_bound(
          sorted_tokens.begin(), sorted_tokens.end(), token, TypedTokenLess{}) -
      sorted_tokens.begin());
}

[[nodiscard]] ExactFrozenIncidenceQuotientDisposition disposition_for_q_r(
    std::size_t q_r) noexcept {
  if (q_r == 0U) {
    return ExactFrozenIncidenceQuotientDisposition::q_r_zero;
  }
  if (q_r == 1U) {
    return ExactFrozenIncidenceQuotientDisposition::q_r_one;
  }
  return ExactFrozenIncidenceQuotientDisposition::q_r_multiple;
}

[[nodiscard]] ExactFrozenIncidenceHyperedgeBinding expected_binding(
    const FrozenIncidenceQuotientAnalysis& analysis,
    const ExactFrozenIncidenceHyperedge& hyperedge,
    std::span<const ExactFrozenIncidenceToken> token_references) {
  const std::size_t first_local_token = local_token_index(
      analysis.sorted_tokens,
      token_references[hyperedge.token_reference_offset]);
  return ExactFrozenIncidenceHyperedgeBinding{
      hyperedge.hyperedge_index,
      hyperedge.token_reference_offset,
      hyperedge.token_reference_count,
      analysis.group_by_local_token[first_local_token]};
}

[[nodiscard]] FrozenIncidenceQuotientAnalysis
analyze_frozen_incidence_quotient(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    const ExactFrozenIncidenceQuotientBudget& budget) {
  FrozenIncidenceQuotientAnalysis analysis;
  analysis.facts = make_requirement_result(
      hyperedges.size(), token_references.size(), budget);
  ExactFrozenIncidenceQuotientResult& result = analysis.facts;
  if (result.decision !=
      ExactFrozenIncidenceQuotientDecision::not_certified) {
    return analysis;
  }

  std::size_t maximum_hyperedge_reference_count = 0U;
  if (!valid_canonical_csr_and_token_kinds(
          hyperedges,
          token_references,
          maximum_hyperedge_reference_count)) {
    result.decision = ExactFrozenIncidenceQuotientDecision::
        no_frozen_incidence_quotient_input_shape_rejected;
    return analysis;
  }
  result.input_shape_certified = true;
  result.typed_token_pairs_certified = true;

  analysis.sorted_tokens.assign(
      token_references.begin(), token_references.end());
  std::sort(
      analysis.sorted_tokens.begin(),
      analysis.sorted_tokens.end(),
      TypedTokenLess{});
  analysis.sorted_tokens.erase(
      std::unique(
          analysis.sorted_tokens.begin(), analysis.sorted_tokens.end()),
      analysis.sorted_tokens.end());
  const std::size_t distinct_token_count =
      analysis.sorted_tokens.size();

  CanonicalLocalDisjointSet components(distinct_token_count);
  for (const ExactFrozenIncidenceHyperedge& hyperedge : hyperedges) {
    const std::size_t first_local_token = local_token_index(
        analysis.sorted_tokens,
        token_references[hyperedge.token_reference_offset]);
    for (std::size_t relative_index = 1U;
         relative_index < hyperedge.token_reference_count;
         ++relative_index) {
      components.unite(
          first_local_token,
          local_token_index(
              analysis.sorted_tokens,
              token_references[
                  hyperedge.token_reference_offset + relative_index]));
    }
  }

  const std::size_t sentinel = std::numeric_limits<std::size_t>::max();
  analysis.group_by_local_token.assign(distinct_token_count, sentinel);
  std::size_t group_count = 0U;
  for (std::size_t token_index = 0U;
       token_index < distinct_token_count;
       ++token_index) {
    if (components.find(token_index) == token_index) {
      analysis.group_by_local_token[token_index] = group_count;
      ++group_count;
    }
  }
  for (std::size_t token_index = 0U;
       token_index < distinct_token_count;
       ++token_index) {
    analysis.group_by_local_token[token_index] =
        analysis.group_by_local_token[components.find(token_index)];
  }

  analysis.group_facts.resize(group_count);
  for (std::size_t group_index = 0U;
       group_index < group_count;
       ++group_index) {
    analysis.group_facts[group_index].group_index = group_index;
  }
  for (std::size_t token_index = 0U;
       token_index < distinct_token_count;
       ++token_index) {
    const ExactFrozenIncidenceToken& token =
        analysis.sorted_tokens[token_index];
    ExactFrozenIncidenceQuotientGroup& group =
        analysis.group_facts[
            analysis.group_by_local_token[token_index]];
    ++group.token_count;
    switch (token.kind) {
      case ExactFrozenIncidenceTokenKind::rooted_carrier:
        ++group.rooted_carrier_count;
        ++result.counters.distinct_rooted_carrier_count;
        break;
      case ExactFrozenIncidenceTokenKind::latent_carrier:
        ++group.latent_carrier_count;
        ++result.counters.distinct_latent_carrier_count;
        break;
      case ExactFrozenIncidenceTokenKind::equal_facet:
        ++group.equal_facet_count;
        ++result.counters.distinct_equal_facet_count;
        break;
    }
  }

  std::size_t token_offset = 0U;
  for (ExactFrozenIncidenceQuotientGroup& group :
       analysis.group_facts) {
    group.token_offset = token_offset;
    token_offset += group.token_count;
    group.disposition = disposition_for_q_r(group.rooted_carrier_count);
  }
  for (const ExactFrozenIncidenceHyperedge& hyperedge : hyperedges) {
    const ExactFrozenIncidenceHyperedgeBinding binding =
        expected_binding(analysis, hyperedge, token_references);
    ++analysis.group_facts[binding.group_index].hyperedge_count;
  }

  result.counters.hyperedge_count = hyperedges.size();
  result.counters.token_reference_count = token_references.size();
  result.counters.distinct_token_count = distinct_token_count;
  result.counters.group_count = group_count;
  result.counters.maximum_hyperedge_reference_count =
      maximum_hyperedge_reference_count;
  for (const ExactFrozenIncidenceQuotientGroup& group :
       analysis.group_facts) {
    switch (group.disposition) {
      case ExactFrozenIncidenceQuotientDisposition::q_r_zero:
        ++result.counters.q_r_zero_group_count;
        break;
      case ExactFrozenIncidenceQuotientDisposition::q_r_one:
        ++result.counters.q_r_one_group_count;
        break;
      case ExactFrozenIncidenceQuotientDisposition::q_r_multiple:
        ++result.counters.q_r_multiple_group_count;
        break;
    }
    result.counters.maximum_group_token_count = std::max(
        result.counters.maximum_group_token_count, group.token_count);
    result.counters.maximum_group_q_r = std::max(
        result.counters.maximum_group_q_r, group.rooted_carrier_count);
  }
  result.counters.logical_output_entry_count =
      hyperedges.size() + distinct_token_count + group_count +
      distinct_token_count;
  result.counters.logical_scratch_entry_count =
      token_references.size() + 2U * distinct_token_count + group_count;

  result.equivalence_closure_over_all_token_kinds_certified = true;
  result.groups_and_token_slices_canonical = true;
  result.every_hyperedge_bound_once = true;
  result.every_distinct_token_bound_once = true;
  result.q_r_counts_distinct_rooted_carriers_only = true;
  result.q_r_zero_one_and_multiple_preserved = true;
  result.decision = ExactFrozenIncidenceQuotientDecision::
      complete_certified_frozen_incidence_quotient;
  return analysis;
}

[[nodiscard]] bool requirements_match(
    const ExactFrozenIncidenceQuotientResult& expected,
    const ExactFrozenIncidenceQuotientResult& observed) noexcept {
  return observed.schema_version == expected.schema_version &&
         observed.required_hyperedge_capacity ==
             expected.required_hyperedge_capacity &&
         observed.required_token_reference_capacity ==
             expected.required_token_reference_capacity &&
         observed.required_distinct_token_capacity ==
             expected.required_distinct_token_capacity &&
         observed.required_group_capacity ==
             expected.required_group_capacity &&
         observed.required_hyperedge_binding_capacity ==
             expected.required_hyperedge_binding_capacity &&
         observed.required_token_binding_capacity ==
             expected.required_token_binding_capacity &&
         observed.required_group_token_capacity ==
             expected.required_group_token_capacity &&
         observed.required_scratch_entry_capacity ==
             expected.required_scratch_entry_capacity &&
         observed.required_logical_output_entry_capacity ==
             expected.required_logical_output_entry_capacity;
}

[[nodiscard]] bool result_facts_match(
    const ExactFrozenIncidenceQuotientResult& expected,
    const ExactFrozenIncidenceQuotientResult& observed) noexcept {
  return observed.input_shape_certified ==
             expected.input_shape_certified &&
         observed.budget_preflight_certified ==
             expected.budget_preflight_certified &&
         observed.typed_token_pairs_certified ==
             expected.typed_token_pairs_certified &&
         observed.equivalence_closure_over_all_token_kinds_certified ==
             expected.equivalence_closure_over_all_token_kinds_certified &&
         observed.groups_and_token_slices_canonical ==
             expected.groups_and_token_slices_canonical &&
         observed.every_hyperedge_bound_once ==
             expected.every_hyperedge_bound_once &&
         observed.every_distinct_token_bound_once ==
             expected.every_distinct_token_bound_once &&
         observed.q_r_counts_distinct_rooted_carriers_only ==
             expected.q_r_counts_distinct_rooted_carriers_only &&
         observed.q_r_zero_one_and_multiple_preserved ==
             expected.q_r_zero_one_and_multiple_preserved;
}

[[nodiscard]] bool combinatorial_non_scope_matches(
    const ExactFrozenIncidenceQuotientResult& expected,
    const ExactFrozenIncidenceQuotientResult& observed) noexcept {
  return !expected.external_token_snapshot_membership_checked &&
         !expected.hgp_batch_actions_claimed &&
         !expected.caller_owned_state_or_global_mutation_performed &&
         !expected.geometry_gamma_facets_or_cells_materialized &&
         !expected.public_status_claimed &&
         !observed.external_token_snapshot_membership_checked &&
         !observed.hgp_batch_actions_claimed &&
         !observed.caller_owned_state_or_global_mutation_performed &&
         !observed.geometry_gamma_facets_or_cells_materialized &&
         !observed.public_status_claimed;
}

[[nodiscard]] bool group_token_arena_matches(
    const FrozenIncidenceQuotientAnalysis& analysis,
    const ExactFrozenIncidenceQuotientResult& observed) {
  if (observed.group_tokens.size() != analysis.sorted_tokens.size()) {
    return false;
  }
  const TypedTokenLess less;
  for (const ExactFrozenIncidenceQuotientGroup& group :
       analysis.group_facts) {
    bool has_previous = false;
    ExactFrozenIncidenceToken previous{};
    for (std::size_t relative_index = 0U;
         relative_index < group.token_count;
         ++relative_index) {
      const ExactFrozenIncidenceToken& token =
          observed.group_tokens[group.token_offset + relative_index];
      if (!valid_token_kind(token.kind) ||
          (has_previous && !less(previous, token))) {
        return false;
      }
      const auto position = std::lower_bound(
          analysis.sorted_tokens.begin(),
          analysis.sorted_tokens.end(),
          token,
          less);
      if (position == analysis.sorted_tokens.end() ||
          *position != token) {
        return false;
      }
      const std::size_t local_index = static_cast<std::size_t>(
          position - analysis.sorted_tokens.begin());
      if (analysis.group_by_local_token[local_index] !=
          group.group_index) {
        return false;
      }
      previous = token;
      has_previous = true;
    }
  }
  return true;
}

[[nodiscard]] bool token_bindings_match(
    const FrozenIncidenceQuotientAnalysis& analysis,
    const ExactFrozenIncidenceQuotientResult& observed) noexcept {
  if (observed.token_bindings.size() != analysis.sorted_tokens.size()) {
    return false;
  }
  for (std::size_t token_index = 0U;
       token_index < analysis.sorted_tokens.size();
       ++token_index) {
    if (observed.token_bindings[token_index] !=
        ExactFrozenIncidenceTokenBinding{
            analysis.sorted_tokens[token_index],
            analysis.group_by_local_token[token_index]}) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool nonnegative_partition_matches(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t total) noexcept {
  return first <= total && second <= total - first &&
         third == total - first - second;
}

}  // namespace

bool ExactFrozenIncidenceQuotientResult::
    certified_frozen_incidence_quotient() const noexcept {
  constexpr ExactFrozenIncidenceQuotientScope certified_scope =
      ExactFrozenIncidenceQuotientScope::
          exact_equivalence_closure_of_externally_certified_frozen_typed_tokens_only;
  return schema_version ==
             direct_frozen_incidence_quotient_schema_version &&
         input_shape_certified && budget_preflight_certified &&
         typed_token_pairs_certified &&
         equivalence_closure_over_all_token_kinds_certified &&
         groups_and_token_slices_canonical &&
         every_hyperedge_bound_once && every_distinct_token_bound_once &&
         q_r_counts_distinct_rooted_carriers_only &&
         q_r_zero_one_and_multiple_preserved &&
         !external_token_snapshot_membership_checked &&
         !hgp_batch_actions_claimed &&
         !caller_owned_state_or_global_mutation_performed &&
         !geometry_gamma_facets_or_cells_materialized &&
         !public_status_claimed && budget_is_sufficient(*this) &&
         required_hyperedge_capacity == counters.hyperedge_count &&
         required_token_reference_capacity ==
             counters.token_reference_count &&
         counters.distinct_token_count <=
             required_distinct_token_capacity &&
         counters.group_count <= required_group_capacity &&
         nonnegative_partition_matches(
             counters.distinct_rooted_carrier_count,
             counters.distinct_latent_carrier_count,
             counters.distinct_equal_facet_count,
             counters.distinct_token_count) &&
         nonnegative_partition_matches(
             counters.q_r_zero_group_count,
             counters.q_r_one_group_count,
             counters.q_r_multiple_group_count,
             counters.group_count) &&
         counters.logical_scratch_entry_count <=
             required_scratch_entry_capacity &&
         hyperedge_bindings.size() == counters.hyperedge_count &&
         token_bindings.size() == counters.distinct_token_count &&
         groups.size() == counters.group_count &&
         group_tokens.size() == counters.distinct_token_count &&
         counters.logical_output_entry_count ==
             hyperedge_bindings.size() + token_bindings.size() +
                 groups.size() + group_tokens.size() &&
         counters.logical_output_entry_count <=
             required_logical_output_entry_capacity &&
         decision == ExactFrozenIncidenceQuotientDecision::
                         complete_certified_frozen_incidence_quotient &&
         scope == certified_scope;
}

bool ExactFrozenIncidenceQuotientResult::atomic_empty_failure()
    const noexcept {
  return decision != ExactFrozenIncidenceQuotientDecision::
                         complete_certified_frozen_incidence_quotient &&
         hyperedge_bindings.empty() && token_bindings.empty() &&
         groups.empty() && group_tokens.empty();
}

ExactFrozenIncidenceQuotientResult
build_exact_direct_frozen_incidence_quotient(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    const ExactFrozenIncidenceQuotientBudget& budget) {
  try {
    FrozenIncidenceQuotientAnalysis analysis =
        analyze_frozen_incidence_quotient(
            hyperedges, token_references, budget);
    ExactFrozenIncidenceQuotientResult result =
        std::move(analysis.facts);
    if (result.decision != ExactFrozenIncidenceQuotientDecision::
                               complete_certified_frozen_incidence_quotient) {
      return result;
    }

    result.groups = std::move(analysis.group_facts);
    result.hyperedge_bindings.resize(hyperedges.size());
    result.token_bindings.resize(analysis.sorted_tokens.size());
    result.group_tokens.resize(analysis.sorted_tokens.size());
    std::vector<std::size_t> group_token_write_cursors(
        result.groups.size());
    for (std::size_t group_index = 0U;
         group_index < result.groups.size();
         ++group_index) {
      group_token_write_cursors[group_index] =
          result.groups[group_index].token_offset;
    }

    for (std::size_t token_index = 0U;
         token_index < analysis.sorted_tokens.size();
         ++token_index) {
      const std::size_t group_index =
          analysis.group_by_local_token[token_index];
      result.token_bindings[token_index] =
          {analysis.sorted_tokens[token_index], group_index};
      result.group_tokens[group_token_write_cursors[group_index]] =
          analysis.sorted_tokens[token_index];
      ++group_token_write_cursors[group_index];
    }
    for (const ExactFrozenIncidenceHyperedge& hyperedge : hyperedges) {
      result.hyperedge_bindings[hyperedge.hyperedge_index] =
          expected_binding(analysis, hyperedge, token_references);
    }
    return result;
  } catch (const std::bad_alloc&) {
    ExactFrozenIncidenceQuotientResult failure =
        make_requirement_result(
            hyperedges.size(), token_references.size(), budget);
    if (failure.decision ==
        ExactFrozenIncidenceQuotientDecision::not_certified) {
      failure.decision = ExactFrozenIncidenceQuotientDecision::
          no_frozen_incidence_quotient_allocation_failed;
    }
    return failure;
  } catch (const std::length_error&) {
    ExactFrozenIncidenceQuotientResult failure =
        make_requirement_result(
            hyperedges.size(), token_references.size(), budget);
    if (failure.decision ==
        ExactFrozenIncidenceQuotientDecision::not_certified) {
      failure.decision = ExactFrozenIncidenceQuotientDecision::
          no_frozen_incidence_quotient_allocation_failed;
    }
    return failure;
  }
}

ExactFrozenIncidenceQuotientStreamingVerification
verify_exact_direct_frozen_incidence_quotient_streaming(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_budget,
    const ExactFrozenIncidenceQuotientResult& observed) {
  ExactFrozenIncidenceQuotientStreamingVerification verification;
  try {
    const FrozenIncidenceQuotientAnalysis analysis =
        analyze_frozen_incidence_quotient(
            hyperedges, token_references, trusted_budget);
    const ExactFrozenIncidenceQuotientResult& expected = analysis.facts;
    const bool replay_complete =
        expected.decision == ExactFrozenIncidenceQuotientDecision::
                                 complete_certified_frozen_incidence_quotient;
    if (replay_complete) {
      verification.source_hyperedge_scan_count = hyperedges.size();
      verification.source_token_reference_scan_count =
          token_references.size();
    }

    verification.requested_budget_certified =
        observed.requested_budget == trusted_budget;
    verification.requirements_certified =
        requirements_match(expected, observed);
    verification.input_shape_certified =
        replay_complete && observed.input_shape_certified;

    bool hyperedge_bindings_match =
        replay_complete &&
        observed.hyperedge_bindings.size() == hyperedges.size();
    if (hyperedge_bindings_match) {
      for (const ExactFrozenIncidenceHyperedge& hyperedge : hyperedges) {
        if (observed.hyperedge_bindings[hyperedge.hyperedge_index] !=
            expected_binding(analysis, hyperedge, token_references)) {
          hyperedge_bindings_match = false;
          break;
        }
      }
    }
    verification.hyperedge_bindings_certified =
        hyperedge_bindings_match;
    verification.token_bindings_certified =
        replay_complete && token_bindings_match(analysis, observed);

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
    verification.group_tokens_certified =
        replay_complete && group_token_arena_matches(analysis, observed);
    verification.counters_certified =
        replay_complete && observed.counters == expected.counters;
    verification.result_facts_certified =
        replay_complete && result_facts_match(expected, observed);
    verification.combinatorial_non_scope_certified =
        replay_complete && combinatorial_non_scope_matches(expected, observed);
    verification.decision_and_scope_certified =
        observed.decision == expected.decision &&
        observed.scope == expected.scope;
    verification.no_second_persistent_output_arena_certified =
        expected.hyperedge_bindings.empty() &&
        expected.token_bindings.empty() && expected.groups.empty() &&
        expected.group_tokens.empty();
    verification.fresh_streaming_replay_certified =
        replay_complete && verification.requested_budget_certified &&
        verification.requirements_certified &&
        verification.input_shape_certified &&
        verification.hyperedge_bindings_certified &&
        verification.token_bindings_certified &&
        verification.groups_certified &&
        verification.group_tokens_certified &&
        verification.counters_certified &&
        verification.result_facts_certified &&
        verification.combinatorial_non_scope_certified &&
        verification.decision_and_scope_certified &&
        verification.no_second_persistent_output_arena_certified;
    verification.result_certified =
        verification.fresh_streaming_replay_certified &&
        observed.certified_frozen_incidence_quotient();
    return verification;
  } catch (const std::bad_alloc&) {
    return verification;
  } catch (const std::length_error&) {
    return verification;
  }
}

}  // namespace morsehgp3d::hierarchy
