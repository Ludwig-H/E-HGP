#include "morsehgp3d/hierarchy/direct_frozen_root_quotient.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <optional>
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

struct FrozenRootQuotientAnalysis {
  ExactFrozenRootQuotientResult facts;
  std::vector<ExactFrozenRootId> sorted_root_ids;
  std::vector<std::size_t> group_by_local_root;
  std::vector<ExactFrozenRootQuotientGroup> group_facts;
};

[[nodiscard]] std::size_t local_root_index(
    const std::vector<ExactFrozenRootId>& sorted_root_ids,
    ExactFrozenRootId root_id) {
  return static_cast<std::size_t>(
      std::lower_bound(
          sorted_root_ids.begin(), sorted_root_ids.end(), root_id) -
      sorted_root_ids.begin());
}

[[nodiscard]] bool valid_canonical_csr(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::size_t root_reference_count,
    std::size_t& maximum_hyperedge_reference_count) {
  std::size_t expected_offset = 0U;
  for (std::size_t index = 0U; index < hyperedges.size(); ++index) {
    const ExactFrozenRootHyperedge& hyperedge = hyperedges[index];
    if (hyperedge.hyperedge_index != index ||
        hyperedge.root_reference_offset != expected_offset ||
        hyperedge.root_reference_count == 0U) {
      return false;
    }
    const auto next_offset = checked_add(
        expected_offset, hyperedge.root_reference_count);
    if (!next_offset.has_value() || *next_offset > root_reference_count) {
      return false;
    }
    expected_offset = *next_offset;
    maximum_hyperedge_reference_count = std::max(
        maximum_hyperedge_reference_count,
        hyperedge.root_reference_count);
  }
  return expected_offset == root_reference_count;
}

[[nodiscard]] bool budget_is_sufficient(
    const ExactFrozenRootQuotientResult& result) {
  return result.required_hyperedge_capacity <=
             result.requested_budget.maximum_hyperedge_count &&
         result.required_root_reference_capacity <=
             result.requested_budget.maximum_root_reference_count &&
         result.required_distinct_root_capacity <=
             result.requested_budget.maximum_distinct_root_count &&
         result.required_group_capacity <=
             result.requested_budget.maximum_group_count &&
         result.required_scratch_entry_capacity <=
             result.requested_budget.maximum_scratch_entry_count;
}

[[nodiscard]] ExactFrozenRootHyperedgeBinding expected_binding(
    const FrozenRootQuotientAnalysis& analysis,
    const ExactFrozenRootHyperedge& hyperedge,
    std::span<const ExactFrozenRootId> root_references) {
  const std::size_t first_root_index = local_root_index(
      analysis.sorted_root_ids,
      root_references[hyperedge.root_reference_offset]);
  return ExactFrozenRootHyperedgeBinding{
      hyperedge.hyperedge_index,
      hyperedge.root_reference_offset,
      hyperedge.root_reference_count,
      analysis.group_by_local_root[first_root_index]};
}

[[nodiscard]] FrozenRootQuotientAnalysis analyze_frozen_root_quotient(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::span<const ExactFrozenRootId> root_references,
    const ExactFrozenRootQuotientBudget& budget) {
  FrozenRootQuotientAnalysis analysis;
  ExactFrozenRootQuotientResult& result = analysis.facts;
  result.requested_budget = budget;
  result.scope = ExactFrozenRootQuotientScope::
      exact_equivalence_closure_of_externally_certified_frozen_roots_only;
  result.required_hyperedge_capacity = hyperedges.size();
  result.required_root_reference_capacity = root_references.size();
  result.required_distinct_root_capacity = root_references.size();
  result.required_group_capacity = hyperedges.size();

  const auto triple_root_count =
      checked_multiply(3U, root_references.size());
  const auto scratch_capacity = triple_root_count.has_value()
                                    ? checked_add(
                                          *triple_root_count,
                                          hyperedges.size())
                                    : std::nullopt;
  const auto double_hyperedge_count =
      checked_multiply(2U, hyperedges.size());
  const auto output_limit = double_hyperedge_count.has_value()
                                ? checked_add(
                                      *double_hyperedge_count,
                                      root_references.size())
                                : std::nullopt;
  if (!scratch_capacity.has_value() || !output_limit.has_value()) {
    result.decision = ExactFrozenRootQuotientDecision::
        no_frozen_root_quotient_capacity_overflow;
    return analysis;
  }
  result.required_scratch_entry_capacity = *scratch_capacity;
  result.logical_output_entry_limit = *output_limit;

  if (!budget_is_sufficient(result)) {
    result.decision = ExactFrozenRootQuotientDecision::
        no_frozen_root_quotient_budget_exhausted;
    return analysis;
  }
  result.budget_preflight_certified = true;

  std::size_t maximum_hyperedge_reference_count = 0U;
  if (!valid_canonical_csr(
          hyperedges,
          root_references.size(),
          maximum_hyperedge_reference_count)) {
    result.decision = ExactFrozenRootQuotientDecision::
        no_frozen_root_quotient_input_shape_rejected;
    return analysis;
  }
  result.input_shape_certified = true;

  analysis.sorted_root_ids.assign(
      root_references.begin(), root_references.end());
  std::sort(
      analysis.sorted_root_ids.begin(), analysis.sorted_root_ids.end());
  analysis.sorted_root_ids.erase(
      std::unique(
          analysis.sorted_root_ids.begin(),
          analysis.sorted_root_ids.end()),
      analysis.sorted_root_ids.end());
  const std::size_t distinct_root_count =
      analysis.sorted_root_ids.size();

  CanonicalLocalDisjointSet components(distinct_root_count);
  for (const ExactFrozenRootHyperedge& hyperedge : hyperedges) {
    const std::size_t first_root_index = local_root_index(
        analysis.sorted_root_ids,
        root_references[hyperedge.root_reference_offset]);
    for (std::size_t relative_index = 1U;
         relative_index < hyperedge.root_reference_count;
         ++relative_index) {
      const ExactFrozenRootId next_root_id = root_references[
          hyperedge.root_reference_offset + relative_index];
      components.unite(
          first_root_index,
          local_root_index(analysis.sorted_root_ids, next_root_id));
    }
  }

  const std::size_t sentinel = std::numeric_limits<std::size_t>::max();
  analysis.group_by_local_root.assign(distinct_root_count, sentinel);
  std::size_t group_count = 0U;
  for (std::size_t root_index = 0U;
       root_index < distinct_root_count;
       ++root_index) {
    if (components.find(root_index) == root_index) {
      analysis.group_by_local_root[root_index] = group_count;
      ++group_count;
    }
  }
  for (std::size_t root_index = 0U;
       root_index < distinct_root_count;
       ++root_index) {
    analysis.group_by_local_root[root_index] =
        analysis.group_by_local_root[components.find(root_index)];
  }

  analysis.group_facts.resize(group_count);
  for (std::size_t group_index = 0U;
       group_index < group_count;
       ++group_index) {
    analysis.group_facts[group_index].group_index = group_index;
  }
  for (std::size_t root_index = 0U;
       root_index < distinct_root_count;
       ++root_index) {
    ++analysis
          .group_facts[analysis.group_by_local_root[root_index]]
          .root_count;
  }

  std::size_t root_offset = 0U;
  for (ExactFrozenRootQuotientGroup& group : analysis.group_facts) {
    group.root_offset = root_offset;
    root_offset += group.root_count;
    group.disposition =
        group.root_count == 1U
            ? ExactFrozenRootQuotientDisposition::one_root_class
            : ExactFrozenRootQuotientDisposition::multiple_root_class;
  }
  for (const ExactFrozenRootHyperedge& hyperedge : hyperedges) {
    const ExactFrozenRootHyperedgeBinding binding =
        expected_binding(analysis, hyperedge, root_references);
    ++analysis.group_facts[binding.group_index].hyperedge_count;
  }

  result.counters.hyperedge_count = hyperedges.size();
  result.counters.root_reference_count = root_references.size();
  result.counters.distinct_root_count = distinct_root_count;
  result.counters.group_count = group_count;
  result.counters.maximum_hyperedge_reference_count =
      maximum_hyperedge_reference_count;
  for (const ExactFrozenRootQuotientGroup& group :
       analysis.group_facts) {
    if (group.disposition ==
        ExactFrozenRootQuotientDisposition::one_root_class) {
      ++result.counters.one_root_group_count;
    } else {
      ++result.counters.multiple_root_group_count;
    }
    result.counters.maximum_group_root_count = std::max(
        result.counters.maximum_group_root_count, group.root_count);
  }
  result.counters.logical_output_entry_count =
      hyperedges.size() + group_count + distinct_root_count;
  result.counters.logical_scratch_entry_count =
      root_references.size() + 2U * distinct_root_count + group_count;

  result.equivalence_closure_certified = true;
  result.groups_and_root_slices_canonical = true;
  result.every_hyperedge_bound_once = true;
  result.one_root_groups_preserved = true;
  result.decision = ExactFrozenRootQuotientDecision::
      complete_certified_frozen_root_quotient;
  return analysis;
}

[[nodiscard]] bool group_root_arena_matches(
    const FrozenRootQuotientAnalysis& analysis,
    const ExactFrozenRootQuotientResult& observed) {
  if (observed.group_root_ids.size() !=
      analysis.sorted_root_ids.size()) {
    return false;
  }
  for (const ExactFrozenRootQuotientGroup& group :
       analysis.group_facts) {
    // The exact cardinality plus strict ordering and component membership
    // proves equality with the expected slice without materializing it.
    bool has_previous = false;
    ExactFrozenRootId previous{};
    for (std::size_t relative_index = 0U;
         relative_index < group.root_count;
         ++relative_index) {
      const ExactFrozenRootId root_id =
          observed.group_root_ids[group.root_offset + relative_index];
      const auto position = std::lower_bound(
          analysis.sorted_root_ids.begin(),
          analysis.sorted_root_ids.end(),
          root_id);
      if (position == analysis.sorted_root_ids.end() ||
          *position != root_id || (has_previous && !(previous < root_id))) {
        return false;
      }
      const std::size_t local_index = static_cast<std::size_t>(
          position - analysis.sorted_root_ids.begin());
      if (analysis.group_by_local_root[local_index] != group.group_index) {
        return false;
      }
      previous = root_id;
      has_previous = true;
    }
  }
  return true;
}

[[nodiscard]] bool requirements_match(
    const ExactFrozenRootQuotientResult& expected,
    const ExactFrozenRootQuotientResult& observed) {
  return observed.schema_version == expected.schema_version &&
         observed.required_hyperedge_capacity ==
             expected.required_hyperedge_capacity &&
         observed.required_root_reference_capacity ==
             expected.required_root_reference_capacity &&
         observed.required_distinct_root_capacity ==
             expected.required_distinct_root_capacity &&
         observed.required_group_capacity ==
             expected.required_group_capacity &&
         observed.required_scratch_entry_capacity ==
             expected.required_scratch_entry_capacity &&
         observed.logical_output_entry_limit ==
             expected.logical_output_entry_limit;
}

[[nodiscard]] bool result_facts_match(
    const ExactFrozenRootQuotientResult& expected,
    const ExactFrozenRootQuotientResult& observed) {
  return observed.input_shape_certified ==
             expected.input_shape_certified &&
         observed.budget_preflight_certified ==
             expected.budget_preflight_certified &&
         observed.equivalence_closure_certified ==
             expected.equivalence_closure_certified &&
         observed.groups_and_root_slices_canonical ==
             expected.groups_and_root_slices_canonical &&
         observed.every_hyperedge_bound_once ==
             expected.every_hyperedge_bound_once &&
         observed.one_root_groups_preserved ==
             expected.one_root_groups_preserved;
}

[[nodiscard]] bool root_only_non_scope_matches(
    const ExactFrozenRootQuotientResult& expected,
    const ExactFrozenRootQuotientResult& observed) {
  return !expected.external_root_snapshot_membership_checked &&
         !expected.zero_root_hyperedges_supported &&
         !expected.latent_facet_tokens_supported &&
         !expected.hgp_batch_actions_claimed &&
         !expected.root_attachment_or_global_mutation_performed &&
         !expected.geometry_facets_or_cells_materialized &&
         !expected.public_status_claimed &&
         !observed.external_root_snapshot_membership_checked &&
         !observed.zero_root_hyperedges_supported &&
         !observed.latent_facet_tokens_supported &&
         !observed.hgp_batch_actions_claimed &&
         !observed.root_attachment_or_global_mutation_performed &&
         !observed.geometry_facets_or_cells_materialized &&
         !observed.public_status_claimed;
}

}  // namespace

bool ExactFrozenRootQuotientResult::certified_frozen_root_quotient()
    const noexcept {
  constexpr ExactFrozenRootQuotientScope certified_scope =
      ExactFrozenRootQuotientScope::
          exact_equivalence_closure_of_externally_certified_frozen_roots_only;
  return schema_version == direct_frozen_root_quotient_schema_version &&
         input_shape_certified && budget_preflight_certified &&
         equivalence_closure_certified &&
         groups_and_root_slices_canonical && every_hyperedge_bound_once &&
         one_root_groups_preserved &&
         !external_root_snapshot_membership_checked &&
         !zero_root_hyperedges_supported && !latent_facet_tokens_supported &&
         !hgp_batch_actions_claimed &&
         !root_attachment_or_global_mutation_performed &&
         !geometry_facets_or_cells_materialized && !public_status_claimed &&
         required_hyperedge_capacity == counters.hyperedge_count &&
         required_root_reference_capacity == counters.root_reference_count &&
         counters.distinct_root_count <= required_distinct_root_capacity &&
         counters.group_count <= required_group_capacity &&
         counters.one_root_group_count <= counters.group_count &&
         counters.multiple_root_group_count ==
             counters.group_count - counters.one_root_group_count &&
         counters.logical_scratch_entry_count <=
             required_scratch_entry_capacity &&
         hyperedge_bindings.size() == counters.hyperedge_count &&
         groups.size() == counters.group_count &&
         group_root_ids.size() == counters.distinct_root_count &&
         counters.logical_output_entry_count ==
             hyperedge_bindings.size() + groups.size() +
                 group_root_ids.size() &&
         counters.logical_output_entry_count <= logical_output_entry_limit &&
         decision == ExactFrozenRootQuotientDecision::
                         complete_certified_frozen_root_quotient &&
         scope == certified_scope;
}

ExactFrozenRootQuotientResult build_exact_direct_frozen_root_quotient(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::span<const ExactFrozenRootId> root_references,
    const ExactFrozenRootQuotientBudget& budget) {
  FrozenRootQuotientAnalysis analysis =
      analyze_frozen_root_quotient(hyperedges, root_references, budget);
  ExactFrozenRootQuotientResult result = std::move(analysis.facts);
  if (result.decision != ExactFrozenRootQuotientDecision::
                             complete_certified_frozen_root_quotient) {
    return result;
  }

  result.groups = std::move(analysis.group_facts);
  std::vector<std::size_t> group_root_write_cursors(result.groups.size());
  for (std::size_t group_index = 0U;
       group_index < result.groups.size();
       ++group_index) {
    group_root_write_cursors[group_index] =
        result.groups[group_index].root_offset;
  }
  result.group_root_ids.resize(analysis.sorted_root_ids.size());
  for (std::size_t root_index = 0U;
       root_index < analysis.sorted_root_ids.size();
       ++root_index) {
    const std::size_t group_index =
        analysis.group_by_local_root[root_index];
    result.group_root_ids[group_root_write_cursors[group_index]] =
        analysis.sorted_root_ids[root_index];
    ++group_root_write_cursors[group_index];
  }

  result.hyperedge_bindings.reserve(hyperedges.size());
  for (const ExactFrozenRootHyperedge& hyperedge : hyperedges) {
    result.hyperedge_bindings.push_back(
        expected_binding(analysis, hyperedge, root_references));
  }
  return result;
}

ExactFrozenRootQuotientStreamingVerification
verify_exact_direct_frozen_root_quotient_streaming(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::span<const ExactFrozenRootId> root_references,
    const ExactFrozenRootQuotientBudget& trusted_budget,
    const ExactFrozenRootQuotientResult& observed) {
  ExactFrozenRootQuotientStreamingVerification verification;
  const FrozenRootQuotientAnalysis analysis =
      analyze_frozen_root_quotient(
          hyperedges, root_references, trusted_budget);
  const ExactFrozenRootQuotientResult& expected = analysis.facts;
  const bool replay_complete =
      expected.decision == ExactFrozenRootQuotientDecision::
                               complete_certified_frozen_root_quotient;
  if (replay_complete) {
    verification.source_hyperedge_scan_count = hyperedges.size();
    verification.source_root_reference_scan_count = root_references.size();
  }

  verification.requested_budget_certified =
      observed.requested_budget == trusted_budget;
  verification.requirements_certified =
      requirements_match(expected, observed);
  verification.input_shape_certified =
      replay_complete && observed.input_shape_certified;

  bool bindings_match =
      replay_complete &&
      observed.hyperedge_bindings.size() == hyperedges.size();
  if (bindings_match) {
    for (const ExactFrozenRootHyperedge& hyperedge : hyperedges) {
      const ExactFrozenRootHyperedgeBinding binding =
          expected_binding(analysis, hyperedge, root_references);
      if (observed.hyperedge_bindings[hyperedge.hyperedge_index] !=
          binding) {
        bindings_match = false;
        break;
      }
    }
  }
  verification.hyperedge_bindings_certified = bindings_match;

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
  verification.group_root_ids_certified =
      replay_complete && group_root_arena_matches(analysis, observed);
  verification.counters_certified =
      replay_complete && observed.counters == expected.counters;
  verification.result_facts_certified =
      replay_complete && result_facts_match(expected, observed);
  verification.root_only_non_scope_certified =
      replay_complete && root_only_non_scope_matches(expected, observed);
  verification.decision_and_scope_certified =
      observed.decision == expected.decision &&
      observed.scope == expected.scope;
  verification.no_second_persistent_output_arena_certified =
      expected.hyperedge_bindings.empty() && expected.groups.empty() &&
      expected.group_root_ids.empty();
  verification.fresh_streaming_replay_certified =
      replay_complete && verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.input_shape_certified &&
      verification.hyperedge_bindings_certified &&
      verification.groups_certified &&
      verification.group_root_ids_certified &&
      verification.counters_certified &&
      verification.result_facts_certified &&
      verification.root_only_non_scope_certified &&
      verification.decision_and_scope_certified &&
      verification.no_second_persistent_output_arena_certified;
  verification.result_certified =
      verification.fresh_streaming_replay_certified &&
      observed.certified_frozen_root_quotient();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
