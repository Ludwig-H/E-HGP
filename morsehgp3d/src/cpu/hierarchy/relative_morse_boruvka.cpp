#include "morsehgp3d/hierarchy/relative_morse_boruvka.hpp"

#include "morsehgp3d/hierarchy/direct_frozen_root_quotient.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t invalid_index =
    std::numeric_limits<std::size_t>::max();

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

[[nodiscard]] std::size_t checked_add_or_throw(
    std::size_t left,
    std::size_t right,
    const char* message) {
  const std::optional<std::size_t> result = checked_add(left, right);
  if (!result.has_value()) {
    throw std::length_error(message);
  }
  return *result;
}

[[nodiscard]] std::size_t theoretical_round_count(
    std::size_t birth_count,
    std::size_t link_count) {
  if (birth_count <= 1U || link_count == 0U) {
    return 0U;
  }
  return std::min(
      static_cast<std::size_t>(std::bit_width(birth_count - 1U)),
      link_count);
}

[[nodiscard]] bool diagnostic_payload_empty(
    const ExactRelativeMorseBoruvkaResult& result) {
  return result.rounds.empty() && result.canonical_links.empty() &&
      result.root_choices.empty() && result.selected_links.empty() &&
      result.retained_saddle_indices.empty() &&
      result.final_component_representative_birth_indices.empty();
}

[[nodiscard]] bool budget_sufficient(
    const ExactRelativeMorseBoruvkaRequirements& requirements,
    const ExactRelativeMorseBoruvkaBudget& budget) {
  return requirements.required_birth_capacity <=
             budget.maximum_birth_count &&
      requirements.required_saddle_capacity <=
          budget.maximum_saddle_count &&
      requirements.required_terminal_birth_reference_capacity <=
          budget.maximum_terminal_birth_reference_count &&
      requirements.required_canonical_link_capacity <=
          budget.maximum_canonical_link_count &&
      requirements.required_round_capacity <=
          budget.maximum_round_count &&
      requirements.required_root_choice_capacity <=
          budget.maximum_root_choice_count &&
      requirements.required_selected_link_capacity <=
          budget.maximum_selected_link_count &&
      requirements.required_retained_saddle_capacity <=
          budget.maximum_retained_saddle_count;
}

[[nodiscard]] bool input_shape_valid(
    const ExactRelativeMorseBoruvkaInput& input) {
  if (input.point_count < 2U || input.order == 0U || input.order > 10U ||
      input.order >= input.point_count || input.births.empty()) {
    return false;
  }
  for (std::size_t index = 0U; index < input.births.size(); ++index) {
    if (input.births[index].birth_index != index) {
      return false;
    }
  }
  std::size_t expected_offset = 0U;
  for (std::size_t index = 0U; index < input.saddles.size(); ++index) {
    const ExactRelativeMorseBoruvkaSaddle& saddle = input.saddles[index];
    if (saddle.saddle_index != index ||
        saddle.terminal_birth_reference_offset != expected_offset ||
        saddle.terminal_birth_reference_count == 0U ||
        saddle.terminal_birth_reference_count > 4U) {
      return false;
    }
    const std::optional<std::size_t> next_offset = checked_add(
        expected_offset, saddle.terminal_birth_reference_count);
    if (!next_offset.has_value() ||
        *next_offset > input.terminal_birth_indices.size()) {
      return false;
    }
    for (std::size_t reference_index = expected_offset;
         reference_index < *next_offset; ++reference_index) {
      const std::size_t birth_index =
          input.terminal_birth_indices[reference_index];
      if (birth_index >= input.births.size() ||
          !(input.births[birth_index].squared_level <
            saddle.squared_level)) {
        return false;
      }
    }
    expected_offset = *next_offset;
  }
  return expected_offset == input.terminal_birth_indices.size();
}

void sort_small_prefix(
    std::array<std::size_t, 4U>& values,
    std::size_t count) {
  if (count > values.size()) {
    throw std::logic_error(
        "a relative Morse saddle exceeds the supported arity");
  }
  for (std::size_t index = 1U; index < count; ++index) {
    const std::size_t value = values[index];
    std::size_t insertion_index = index;
    while (insertion_index > 0U &&
           value < values[insertion_index - 1U]) {
      values[insertion_index] = values[insertion_index - 1U];
      --insertion_index;
    }
    values[insertion_index] = value;
  }
}

[[nodiscard]] std::size_t unique_small_prefix(
    std::array<std::size_t, 4U>& values,
    std::size_t count) {
  if (count == 0U) {
    return 0U;
  }
  if (count > values.size()) {
    throw std::logic_error(
        "a relative Morse saddle exceeds the supported arity");
  }
  std::size_t unique_count = 1U;
  for (std::size_t index = 1U; index < count; ++index) {
    if (values[index] != values[unique_count - 1U]) {
      values[unique_count] = values[index];
      ++unique_count;
    }
  }
  return unique_count;
}

struct SaddleBirths {
  std::array<std::size_t, 4U> values{};
  std::size_t count{};
};

[[nodiscard]] SaddleBirths distinct_saddle_births(
    const ExactRelativeMorseBoruvkaInput& input,
    std::size_t saddle_index) {
  const ExactRelativeMorseBoruvkaSaddle& saddle =
      input.saddles[saddle_index];
  SaddleBirths births;
  for (std::size_t relative_index = 0U;
       relative_index < saddle.terminal_birth_reference_count;
       ++relative_index) {
    births.values[births.count] = input.terminal_birth_indices[
        saddle.terminal_birth_reference_offset + relative_index];
    ++births.count;
  }
  sort_small_prefix(births.values, births.count);
  births.count = unique_small_prefix(births.values, births.count);
  return births;
}

[[nodiscard]] std::optional<std::size_t> canonical_link_count(
    const ExactRelativeMorseBoruvkaInput& input) {
  std::size_t count = 0U;
  for (std::size_t saddle_index = 0U;
       saddle_index < input.saddles.size(); ++saddle_index) {
    const SaddleBirths births = distinct_saddle_births(input, saddle_index);
    if (births.count > 1U) {
      const std::optional<std::size_t> next =
          checked_add(count, births.count - 1U);
      if (!next.has_value()) {
        return std::nullopt;
      }
      count = *next;
    }
  }
  return count;
}

[[nodiscard]] std::vector<ExactRelativeMorseBoruvkaLink>
build_canonical_links(
    const ExactRelativeMorseBoruvkaInput& input,
    std::size_t expected_link_count) {
  std::vector<ExactRelativeMorseBoruvkaLink> links;
  links.reserve(expected_link_count);
  for (std::size_t saddle_index = 0U;
       saddle_index < input.saddles.size(); ++saddle_index) {
    const SaddleBirths births = distinct_saddle_births(input, saddle_index);
    for (std::size_t terminal_index = 1U;
         terminal_index < births.count; ++terminal_index) {
      links.push_back(ExactRelativeMorseBoruvkaLink{
          links.size(),
          saddle_index,
          births.values[0U],
          births.values[terminal_index]});
    }
  }
  if (links.size() != expected_link_count) {
    throw std::logic_error(
        "a relative Morse canonical-link preflight was inconsistent");
  }
  return links;
}

[[nodiscard]] std::size_t component_count(
    const std::vector<std::size_t>& root_by_birth) {
  std::vector<unsigned char> seen(root_by_birth.size(), 0U);
  std::size_t count = 0U;
  for (const std::size_t root : root_by_birth) {
    if (root >= root_by_birth.size()) {
      throw std::logic_error(
          "a relative Morse root is outside the birth arena");
    }
    if (seen[root] == 0U) {
      seen[root] = 1U;
      ++count;
    }
  }
  return count;
}

struct QuotientContraction {
  std::vector<std::size_t> root_by_birth;
  std::size_t component_count{};
};

[[nodiscard]] QuotientContraction contract_links(
    const std::vector<ExactRelativeMorseBoruvkaLink>& links,
    const std::vector<std::size_t>& link_indices,
    const std::vector<std::size_t>& frozen_root_by_birth) {
  QuotientContraction contraction;
  contraction.root_by_birth = frozen_root_by_birth;
  if (link_indices.empty()) {
    contraction.component_count = component_count(frozen_root_by_birth);
    return contraction;
  }

  const std::optional<std::size_t> root_reference_count =
      checked_multiply(2U, link_indices.size());
  const std::optional<std::size_t> scratch_count =
      checked_multiply(7U, link_indices.size());
  if (!root_reference_count.has_value() || !scratch_count.has_value()) {
    throw std::length_error(
        "a relative Morse link quotient bound overflows size_t");
  }

  std::vector<ExactFrozenRootHyperedge> hyperedges;
  std::vector<ExactFrozenRootId> root_references;
  hyperedges.reserve(link_indices.size());
  root_references.reserve(*root_reference_count);
  for (std::size_t local_index = 0U;
       local_index < link_indices.size(); ++local_index) {
    const std::size_t link_index = link_indices[local_index];
    if (link_index >= links.size()) {
      throw std::logic_error(
          "a relative Morse contraction references an invalid link");
    }
    const ExactRelativeMorseBoruvkaLink& link = links[link_index];
    hyperedges.push_back(ExactFrozenRootHyperedge{
        local_index, root_references.size(), 2U});
    root_references.push_back(static_cast<ExactFrozenRootId>(
        frozen_root_by_birth[link.anchor_birth_index]));
    root_references.push_back(static_cast<ExactFrozenRootId>(
        frozen_root_by_birth[link.terminal_birth_index]));
  }

  const ExactFrozenRootQuotientBudget quotient_budget{
      hyperedges.size(),
      root_references.size(),
      root_references.size(),
      hyperedges.size(),
      *scratch_count};
  const ExactFrozenRootQuotientResult quotient =
      build_exact_direct_frozen_root_quotient(
          hyperedges, root_references, quotient_budget);
  if (!quotient.certified_frozen_root_quotient()) {
    throw std::logic_error(
        "a preflighted relative Morse link quotient was rejected");
  }

  std::vector<std::size_t> representative_by_root(
      frozen_root_by_birth.size(), invalid_index);
  for (const ExactFrozenRootQuotientGroup& group : quotient.groups) {
    if (group.root_count == 0U ||
        group.root_offset >= quotient.group_root_ids.size() ||
        group.root_count >
            quotient.group_root_ids.size() - group.root_offset) {
      throw std::logic_error(
          "a certified relative Morse quotient has an invalid root slice");
    }
    const std::size_t representative = static_cast<std::size_t>(
        quotient.group_root_ids[group.root_offset]);
    if (representative >= representative_by_root.size()) {
      throw std::logic_error(
          "a relative Morse quotient representative is out of range");
    }
    for (std::size_t relative_index = 0U;
         relative_index < group.root_count; ++relative_index) {
      const std::size_t root = static_cast<std::size_t>(
          quotient.group_root_ids[group.root_offset + relative_index]);
      if (root >= representative_by_root.size()) {
        throw std::logic_error(
            "a relative Morse quotient root is out of range");
      }
      representative_by_root[root] = representative;
    }
  }
  for (std::size_t& root : contraction.root_by_birth) {
    if (representative_by_root[root] != invalid_index) {
      root = representative_by_root[root];
    }
  }
  contraction.component_count = component_count(contraction.root_by_birth);
  return contraction;
}

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t size) : parents_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t parent = parents_[value];
      parents_[value] = root;
      value = parent;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    parents_[std::max(left, right)] = std::min(left, right);
    return true;
  }

 private:
  std::vector<std::size_t> parents_;
};

[[nodiscard]] bool result_facts_equal(
    const ExactRelativeMorseBoruvkaResult& left,
    const ExactRelativeMorseBoruvkaResult& right) {
  return left.input_shape_certified == right.input_shape_certified &&
      left.budget_preflight_certified ==
          right.budget_preflight_certified &&
      left.canonical_star_expansion_certified ==
          right.canonical_star_expansion_certified &&
      left.roots_frozen_in_every_round ==
          right.roots_frozen_in_every_round &&
      left.all_exact_cominimizers_scanned_for_every_root_cut ==
          right.all_exact_cominimizers_scanned_for_every_root_cut &&
      left.exactly_one_canonical_link_selected_per_root ==
          right.exactly_one_canonical_link_selected_per_root &&
      left.selected_links_are_a_canonical_expansion_subset ==
          right.selected_links_are_a_canonical_expansion_subset &&
      left.frozen_selected_links_contracted_atomically ==
          right.frozen_selected_links_contracted_atomically &&
      left.retained_saddles_form_a_canonical_weighted_partition_basis ==
          right.retained_saddles_form_a_canonical_weighted_partition_basis &&
      left.all_input_threshold_partitions_preserved_relative_to_input ==
          right.all_input_threshold_partitions_preserved_relative_to_input &&
      left.caller_catalog_completeness_certified ==
          right.caller_catalog_completeness_certified &&
      left.silent_incidence_coverage_certified ==
          right.silent_incidence_coverage_certified &&
      left.m1_certified == right.m1_certified &&
      left.hierarchy_or_forest_constructed ==
          right.hierarchy_or_forest_constructed &&
      left.public_status_claimed == right.public_status_claimed &&
      left.diagnostic_outcomes_have_no_payload ==
          right.diagnostic_outcomes_have_no_payload;
}

}  // namespace

bool ExactRelativeMorseBoruvkaResult::
certified_relative_partition_sparsifier() const noexcept {
  return schema_version == relative_morse_boruvka_schema_version &&
      decision == ExactRelativeMorseBoruvkaDecision::
          complete_relative_partition_sparsifier &&
      scope == ExactRelativeMorseBoruvkaScope::
          exact_partition_sparsification_relative_to_one_caller_supplied_weighted_morse_hypergraph_only &&
      input_shape_certified && budget_preflight_certified &&
      canonical_star_expansion_certified &&
      roots_frozen_in_every_round &&
      all_exact_cominimizers_scanned_for_every_root_cut &&
      exactly_one_canonical_link_selected_per_root &&
      selected_links_are_a_canonical_expansion_subset &&
      frozen_selected_links_contracted_atomically &&
      retained_saddles_form_a_canonical_weighted_partition_basis &&
      all_input_threshold_partitions_preserved_relative_to_input &&
      !caller_catalog_completeness_certified &&
      !silent_incidence_coverage_certified && !m1_certified &&
      !hierarchy_or_forest_constructed && !public_status_claimed &&
      !diagnostic_outcomes_have_no_payload &&
      rounds.size() == audit.boruvka_round_count &&
      canonical_links.size() == audit.canonical_link_count &&
      canonical_links.size() ==
          requirements.required_canonical_link_capacity &&
      root_choices.size() == audit.root_choice_count &&
      selected_links.size() == audit.selected_link_count &&
      retained_saddle_indices.size() == audit.retained_saddle_count &&
      final_component_representative_birth_indices.size() ==
          requirements.required_birth_capacity &&
      audit.initial_component_count == requirements.required_birth_capacity;
}

ExactRelativeMorseBoruvkaResult build_exact_relative_morse_boruvka(
    const ExactRelativeMorseBoruvkaInput& input,
    const ExactRelativeMorseBoruvkaBudget& budget) {
  ExactRelativeMorseBoruvkaResult result;
  result.requested_budget = budget;
  result.point_count = input.point_count;
  result.order = input.order;
  result.scope = ExactRelativeMorseBoruvkaScope::
      exact_partition_sparsification_relative_to_one_caller_supplied_weighted_morse_hypergraph_only;
  result.audit.input_validation_pass_count = 1U;
  result.requirements.required_birth_capacity = input.births.size();
  result.requirements.required_saddle_capacity = input.saddles.size();
  result.requirements.required_terminal_birth_reference_capacity =
      input.terminal_birth_indices.size();

  if (!input_shape_valid(input)) {
    result.decision = ExactRelativeMorseBoruvkaDecision::
        no_sparsification_input_shape_rejected;
    result.diagnostic_outcomes_have_no_payload = diagnostic_payload_empty(result);
    return result;
  }
  result.input_shape_certified = true;

  const std::optional<std::size_t> link_count =
      canonical_link_count(input);
  if (!link_count.has_value()) {
    result.decision = ExactRelativeMorseBoruvkaDecision::
        no_sparsification_capacity_overflow;
    result.diagnostic_outcomes_have_no_payload = diagnostic_payload_empty(result);
    return result;
  }
  const std::size_t round_count = theoretical_round_count(
      input.births.size(), *link_count);
  const std::size_t selected_link_capacity = input.births.empty()
      ? 0U
      : std::min(*link_count, input.births.size() - 1U);
  const std::optional<std::size_t> double_links =
      checked_multiply(2U, *link_count);
  const std::optional<std::size_t> double_selected_links =
      checked_multiply(2U, selected_link_capacity);
  const std::optional<std::size_t> quotient_scratch =
      checked_multiply(7U, *link_count);
  const std::optional<std::size_t> observations_per_all_rounds =
      double_links.has_value()
          ? checked_multiply(*double_links, round_count)
          : std::nullopt;
  const std::optional<std::size_t> link_scans_per_all_rounds =
      double_links.has_value()
          ? checked_multiply(*double_links, round_count)
          : std::nullopt;
  if (!double_links.has_value() || !double_selected_links.has_value() ||
      !quotient_scratch.has_value() ||
      !observations_per_all_rounds.has_value() ||
      !link_scans_per_all_rounds.has_value()) {
    result.decision = ExactRelativeMorseBoruvkaDecision::
        no_sparsification_capacity_overflow;
    result.diagnostic_outcomes_have_no_payload = diagnostic_payload_empty(result);
    return result;
  }

  result.requirements.required_canonical_link_capacity = *link_count;
  result.requirements.required_round_capacity = round_count;
  result.requirements.required_root_choice_capacity = *double_selected_links;
  result.requirements.required_selected_link_capacity =
      selected_link_capacity;
  result.requirements.required_retained_saddle_capacity = std::min(
      input.saddles.size(), selected_link_capacity);
  result.audit.canonical_terminal_reference_authority_count =
      input.terminal_birth_indices.size();

  if (!budget_sufficient(result.requirements, budget)) {
    result.decision = ExactRelativeMorseBoruvkaDecision::
        no_sparsification_preflight_budget_insufficient;
    result.diagnostic_outcomes_have_no_payload = diagnostic_payload_empty(result);
    return result;
  }
  result.budget_preflight_certified = true;

  result.rounds.reserve(result.requirements.required_round_capacity);
  result.root_choices.reserve(
      result.requirements.required_root_choice_capacity);
  result.selected_links.reserve(
      result.requirements.required_selected_link_capacity);
  result.retained_saddle_indices.reserve(
      result.requirements.required_retained_saddle_capacity);
  result.canonical_links = build_canonical_links(input, *link_count);
  result.audit.canonical_link_count = result.canonical_links.size();

  std::vector<std::size_t> all_link_indices(result.canonical_links.size());
  std::iota(
      all_link_indices.begin(), all_link_indices.end(), std::size_t{0});
  std::vector<std::size_t> root_by_birth(input.births.size());
  std::iota(root_by_birth.begin(), root_by_birth.end(), std::size_t{0});
  const QuotientContraction full_contraction = contract_links(
      result.canonical_links, all_link_indices, root_by_birth);
  if (!all_link_indices.empty()) {
    result.audit.full_expansion_quotient_build_count = 1U;
  }
  result.audit.initial_component_count = input.births.size();
  result.audit.final_component_count = full_contraction.component_count;

  std::vector<unsigned char> link_selected(
      result.canonical_links.size(), 0U);
  std::size_t current_component_count = input.births.size();
  while (current_component_count > full_contraction.component_count) {
    if (result.rounds.size() >=
        result.requirements.required_round_capacity) {
      throw std::logic_error(
          "relative Morse--Boruvka exceeded its round theorem");
    }
    // root_by_birth is immutable until all choices have been recorded and
    // the selected links have been contracted, so this reference is the
    // frozen snapshot without a second O(B) partition copy.
    const std::vector<std::size_t>& frozen_root_by_birth = root_by_birth;
    // Pass one stores the minimum canonical link per root and its exact tie
    // count.  Once choices are emitted, the first arena is reused as a map to
    // root-choice indices.  ExactLevel objects are never copied per birth.
    std::vector<std::size_t> minimum_or_choice_index_by_root(
        input.births.size(), invalid_index);
    std::vector<std::size_t> cominimizer_count_by_root(
        input.births.size(), 0U);

    for (std::size_t link_index = 0U;
         link_index < result.canonical_links.size(); ++link_index) {
      result.audit.boruvka_link_scan_count = checked_add_or_throw(
          result.audit.boruvka_link_scan_count, 1U,
          "a relative Morse link scan count overflows size_t");
      const ExactRelativeMorseBoruvkaLink& link =
          result.canonical_links[link_index];
      const std::size_t left_root =
          frozen_root_by_birth[link.anchor_birth_index];
      const std::size_t right_root =
          frozen_root_by_birth[link.terminal_birth_index];
      if (left_root == right_root) {
        continue;
      }
      const auto consider_link = [&](std::size_t root) {
        const std::size_t current_link =
            minimum_or_choice_index_by_root[root];
        const exact::ExactLevel& candidate_level =
            input.saddles[link.saddle_index].squared_level;
        if (current_link == invalid_index ||
            candidate_level < input.saddles[
                result.canonical_links[current_link].saddle_index]
                                  .squared_level) {
          minimum_or_choice_index_by_root[root] = link_index;
          cominimizer_count_by_root[root] = 1U;
        } else if (candidate_level == input.saddles[
                       result.canonical_links[current_link].saddle_index]
                                             .squared_level) {
          cominimizer_count_by_root[root] = checked_add_or_throw(
              cominimizer_count_by_root[root], 1U,
              "a relative Morse co-minimizer count overflows size_t");
        }
      };
      consider_link(left_root);
      consider_link(right_root);
    }

    ExactRelativeMorseBoruvkaRound round;
    round.round_index = result.rounds.size();
    round.pre_round_component_count = current_component_count;
    round.root_choice_offset = result.root_choices.size();
    for (std::size_t root = 0U; root < input.births.size(); ++root) {
      const std::size_t minimum_link_index =
          minimum_or_choice_index_by_root[root];
      if (minimum_link_index == invalid_index) {
        continue;
      }
      if (root_by_birth[root] != root ||
          cominimizer_count_by_root[root] == 0U) {
        throw std::logic_error(
            "a relative Morse cut minimum is attached to a non-root");
      }
      if (result.root_choices.size() >=
          result.requirements.required_root_choice_capacity) {
        throw std::logic_error(
            "relative Morse root choices exceeded their proven bound");
      }
      const std::size_t choice_index = result.root_choices.size();
      result.root_choices.push_back(
          ExactRelativeMorseBoruvkaRootChoice{
              root,
              cominimizer_count_by_root[root],
              minimum_link_index});
      result.audit.exact_cominimizer_link_observation_count =
          checked_add_or_throw(
              result.audit.exact_cominimizer_link_observation_count,
              cominimizer_count_by_root[root],
              "relative Morse co-minimizer observations overflow size_t");
      minimum_or_choice_index_by_root[root] = choice_index;
    }
    round.root_choice_count =
        result.root_choices.size() - round.root_choice_offset;

    std::vector<unsigned char> selected_this_round(
        result.canonical_links.size(), 0U);
    for (std::size_t choice_index = round.root_choice_offset;
         choice_index < round.root_choice_offset + round.root_choice_count;
         ++choice_index) {
      const std::size_t link_index =
          result.root_choices[choice_index].selected_canonical_link_index;
      if (link_index >= selected_this_round.size()) {
        throw std::logic_error(
            "a relative Morse root choice selected an invalid link");
      }
      selected_this_round[link_index] = 1U;
    }

    round.selected_link_offset = result.selected_links.size();
    std::vector<std::size_t> selected_link_indices;
    for (std::size_t link_index = 0U;
         link_index < result.canonical_links.size(); ++link_index) {
      result.audit.boruvka_link_scan_count = checked_add_or_throw(
          result.audit.boruvka_link_scan_count, 1U,
          "a relative Morse link scan count overflows size_t");
      if (selected_this_round[link_index] == 0U) {
        continue;
      }
      if (link_selected[link_index] != 0U) {
        throw std::logic_error(
            "a relative Morse link was selected in two rounds");
      }
      const ExactRelativeMorseBoruvkaLink& link =
          result.canonical_links[link_index];
      const std::size_t left_root =
          frozen_root_by_birth[link.anchor_birth_index];
      const std::size_t right_root =
          frozen_root_by_birth[link.terminal_birth_index];
      if (left_root == right_root) {
        throw std::logic_error(
            "a relative Morse selected link is not outgoing");
      }
      if (result.selected_links.size() >=
          result.requirements.required_selected_link_capacity) {
        throw std::logic_error(
            "relative Morse selected links exceeded their proven bound");
      }
      result.selected_links.push_back(ExactRelativeMorseBoruvkaSelectedLink{
          link_index, link.saddle_index, left_root, right_root});
      selected_link_indices.push_back(link_index);
      link_selected[link_index] = 1U;
    }
    round.selected_link_count =
        result.selected_links.size() - round.selected_link_offset;
    if (selected_link_indices.empty()) {
      throw std::logic_error(
          "a relative Morse nonterminal quotient has no outgoing link");
    }

    QuotientContraction contraction = contract_links(
        result.canonical_links,
        selected_link_indices,
        frozen_root_by_birth);
    result.audit.frozen_quotient_build_count = checked_add_or_throw(
        result.audit.frozen_quotient_build_count, 1U,
        "a relative Morse quotient-build count overflows size_t");
    if (contraction.component_count >= current_component_count) {
      throw std::logic_error(
          "a relative Morse--Boruvka round made no quotient progress");
    }
    if (current_component_count - contraction.component_count !=
        selected_link_indices.size()) {
      throw std::logic_error(
          "canonical relative Morse cut-minimum links formed a cycle");
    }
    for (std::size_t birth_index = 0U;
         birth_index < input.births.size(); ++birth_index) {
      const std::size_t next_root = contraction.root_by_birth[birth_index];
      if (full_contraction.root_by_birth[next_root] !=
          full_contraction.root_by_birth[birth_index]) {
        throw std::logic_error(
            "a selected relative Morse link crossed a full component");
      }
    }
    current_component_count = contraction.component_count;
    root_by_birth = std::move(contraction.root_by_birth);
    round.post_round_component_count = current_component_count;
    round.roots_frozen_before_all_choices = true;
    round.every_choice_scans_all_exact_cominimizers = true;
    round.exactly_one_canonical_link_selected_per_root = true;
    round.quotient_committed_after_all_choices = true;
    result.rounds.push_back(std::move(round));
  }
  if (root_by_birth != full_contraction.root_by_birth) {
    throw std::logic_error(
        "relative Morse--Boruvka did not recover the full link quotient");
  }

  std::vector<unsigned char> selected_saddle(
      input.saddles.size(), 0U);
  for (const ExactRelativeMorseBoruvkaSelectedLink& selected :
       result.selected_links) {
    selected_saddle[selected.saddle_index] = 1U;
  }
  std::vector<std::size_t> selected_saddle_indices;
  for (std::size_t saddle_index = 0U;
       saddle_index < selected_saddle.size(); ++saddle_index) {
    if (selected_saddle[saddle_index] != 0U) {
      selected_saddle_indices.push_back(saddle_index);
    }
  }
  std::sort(
      selected_saddle_indices.begin(), selected_saddle_indices.end(),
      [&](std::size_t left, std::size_t right) {
        if (input.saddles[left].squared_level !=
            input.saddles[right].squared_level) {
          return input.saddles[left].squared_level <
              input.saddles[right].squared_level;
        }
        return left < right;
      });

  CanonicalDisjointSet retained_components(input.births.size());
  for (const std::size_t saddle_index : selected_saddle_indices) {
    SaddleBirths births = distinct_saddle_births(input, saddle_index);
    for (std::size_t birth_index = 0U;
         birth_index < births.count; ++birth_index) {
      births.values[birth_index] =
          retained_components.find(births.values[birth_index]);
    }
    sort_small_prefix(births.values, births.count);
    births.count = unique_small_prefix(births.values, births.count);
    if (births.count <= 1U) {
      continue;
    }
    if (result.retained_saddle_indices.size() >=
        result.requirements.required_retained_saddle_capacity) {
      throw std::logic_error(
          "the relative Morse partition basis exceeded its proven bound");
    }
    result.retained_saddle_indices.push_back(saddle_index);
    for (std::size_t root_index = 1U;
         root_index < births.count; ++root_index) {
      static_cast<void>(retained_components.unite(
          births.values[0U], births.values[root_index]));
    }
  }
  for (std::size_t birth_index = 0U;
       birth_index < input.births.size(); ++birth_index) {
    if (retained_components.find(birth_index) !=
        full_contraction.root_by_birth[birth_index]) {
      throw std::logic_error(
          "the relative Morse partition basis changed connectivity");
    }
  }

  result.final_component_representative_birth_indices =
      full_contraction.root_by_birth;
  result.audit.boruvka_round_count = result.rounds.size();
  result.audit.root_choice_count = result.root_choices.size();
  result.audit.selected_link_count = result.selected_links.size();
  result.audit.selected_saddle_provenance_count =
      selected_saddle_indices.size();
  result.audit.retained_saddle_count = result.retained_saddle_indices.size();
  result.roots_frozen_in_every_round = std::all_of(
      result.rounds.begin(), result.rounds.end(),
      [](const ExactRelativeMorseBoruvkaRound& round) {
        return round.roots_frozen_before_all_choices;
      });
  result.all_exact_cominimizers_scanned_for_every_root_cut =
      std::all_of(
          result.rounds.begin(), result.rounds.end(),
          [](const ExactRelativeMorseBoruvkaRound& round) {
            return round.every_choice_scans_all_exact_cominimizers;
          });
  result.exactly_one_canonical_link_selected_per_root = std::all_of(
      result.rounds.begin(), result.rounds.end(),
      [](const ExactRelativeMorseBoruvkaRound& round) {
        return round.exactly_one_canonical_link_selected_per_root;
      });
  result.canonical_star_expansion_certified = true;
  result.selected_links_are_a_canonical_expansion_subset = true;
  result.frozen_selected_links_contracted_atomically = std::all_of(
      result.rounds.begin(), result.rounds.end(),
      [](const ExactRelativeMorseBoruvkaRound& round) {
        return round.quotient_committed_after_all_choices;
      });
  result.retained_saddles_form_a_canonical_weighted_partition_basis = true;
  result.all_input_threshold_partitions_preserved_relative_to_input = true;
  result.caller_catalog_completeness_certified = false;
  result.silent_incidence_coverage_certified = false;
  result.m1_certified = false;
  result.hierarchy_or_forest_constructed = false;
  result.public_status_claimed = false;
  result.diagnostic_outcomes_have_no_payload = false;
  result.decision = ExactRelativeMorseBoruvkaDecision::
      complete_relative_partition_sparsifier;
  if (!result.certified_relative_partition_sparsifier()) {
    throw std::logic_error(
        "a completed relative Morse--Boruvka result is inconsistent");
  }
  return result;
}

ExactRelativeMorseBoruvkaVerification
verify_exact_relative_morse_boruvka(
    const ExactRelativeMorseBoruvkaInput& input,
    const ExactRelativeMorseBoruvkaBudget& budget,
    const ExactRelativeMorseBoruvkaResult& observed) {
  // A fresh build rescans every canonical link in every round.  The compact
  // persisted co-minimizer count is therefore not trusted on its own.
  const ExactRelativeMorseBoruvkaResult expected =
      build_exact_relative_morse_boruvka(input, budget);
  ExactRelativeMorseBoruvkaVerification verification;
  verification.requested_budget_certified =
      observed.requested_budget == expected.requested_budget;
  verification.requirements_certified =
      observed.schema_version == expected.schema_version &&
      observed.point_count == expected.point_count &&
      observed.order == expected.order &&
      observed.requirements == expected.requirements;
  verification.rounds_certified =
      observed.canonical_links == expected.canonical_links &&
      observed.rounds == expected.rounds &&
      observed.root_choices == expected.root_choices;
  verification.selected_links_and_saddles_certified =
      observed.selected_links == expected.selected_links &&
      observed.retained_saddle_indices == expected.retained_saddle_indices;
  verification.final_components_certified =
      observed.final_component_representative_birth_indices ==
      expected.final_component_representative_birth_indices;
  verification.audit_certified = observed.audit == expected.audit;
  verification.result_facts_certified =
      result_facts_equal(observed, expected);
  verification.decision_and_scope_certified =
      observed.decision == expected.decision &&
      observed.scope == expected.scope;
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.rounds_certified &&
      verification.selected_links_and_saddles_certified &&
      verification.final_components_certified &&
      verification.audit_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
