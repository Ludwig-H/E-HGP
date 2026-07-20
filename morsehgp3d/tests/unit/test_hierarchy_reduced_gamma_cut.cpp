#include "morsehgp3d/hierarchy/reduced_gamma_cut.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistory;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaActiveRoot;
using morsehgp3d::hierarchy::ExactReducedGammaCut;
using morsehgp3d::hierarchy::ExactReducedGammaCutBoundary;
using morsehgp3d::hierarchy::ExactReducedGammaCutBudget;
using morsehgp3d::hierarchy::ExactReducedGammaCutDecision;
using morsehgp3d::hierarchy::ExactReducedGammaCutScope;
using morsehgp3d::hierarchy::ExactReducedGammaCutVerification;
using morsehgp3d::hierarchy::build_exact_persistent_reduced_gamma_order_history;
using morsehgp3d::hierarchy::build_exact_reduced_gamma_cut;
using morsehgp3d::hierarchy::verify_exact_reduced_gamma_cut;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] std::size_t binomial(std::size_t n, std::size_t k) {
  if (k > n) {
    return 0U;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    value = value * (n - k + index) / index;
  }
  return value;
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget history_budget(
    std::size_t point_count, std::size_t order) {
  const std::size_t facet_count = binomial(point_count, order);
  const std::size_t coface_count = binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_count = facet_count + coface_count;
  const std::size_t replay_count = level_count + 1U;

  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {facet_count, coface_count, union_count};
  budget.maximum_activation_level_count = level_count;
  budget.maximum_total_facet_work_count = replay_count * facet_count;
  budget.maximum_total_coface_work_count = replay_count * coface_count;
  budget.maximum_total_union_work_count = replay_count * union_count;
  budget.maximum_node_count = coface_count;
  budget.maximum_child_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_root_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_count = level_count;
  budget.maximum_group_newly_active_facet_count = facet_count;
  budget.maximum_group_equal_level_coface_count = coface_count;
  budget.maximum_delta_facet_count = facet_count;
  budget.maximum_delta_point_reference_count = order * facet_count;
  return budget;
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistory history(
    const CanonicalPointCloud& cloud, std::size_t order) {
  return build_exact_persistent_reduced_gamma_order_history(
      cloud, order, history_budget(cloud.size(), order));
}

[[nodiscard]] ExactReducedGammaCutBudget maximum_cut_budget() {
  ExactReducedGammaCutBudget budget;
  budget.maximum_batch_count =
      ExactReducedGammaCutBudget::maximum_supported_batch_count;
  budget.maximum_group_record_count =
      ExactReducedGammaCutBudget::maximum_supported_group_record_count;
  budget.maximum_node_record_count =
      ExactReducedGammaCutBudget::maximum_supported_node_record_count;
  budget.maximum_prior_root_reference_count =
      ExactReducedGammaCutBudget::
          maximum_supported_prior_root_reference_count;
  budget.maximum_child_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_child_reference_count;
  budget.maximum_newly_active_facet_count =
      ExactReducedGammaCutBudget::
          maximum_supported_newly_active_facet_count;
  budget.maximum_equal_level_coface_count =
      ExactReducedGammaCutBudget::
          maximum_supported_equal_level_coface_count;
  budget.maximum_delta_facet_count =
      ExactReducedGammaCutBudget::maximum_supported_delta_facet_count;
  budget.maximum_delta_point_reference_count =
      ExactReducedGammaCutBudget::
          maximum_supported_delta_point_reference_count;
  budget.maximum_active_root_count =
      ExactReducedGammaCutBudget::maximum_supported_active_root_count;
  budget.maximum_output_facet_reference_count =
      ExactReducedGammaCutBudget::
          maximum_supported_output_facet_reference_count;
  budget.maximum_output_point_reference_count =
      ExactReducedGammaCutBudget::
          maximum_supported_output_point_reference_count;
  budget.maximum_facet_replay_work_count =
      ExactReducedGammaCutBudget::maximum_supported_facet_replay_work_count;
  budget.maximum_point_id_replay_work_count =
      ExactReducedGammaCutBudget::
          maximum_supported_point_id_replay_work_count;
  budget.maximum_result_incidence_facet_check_count =
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_facet_check_count;
  budget.maximum_result_incidence_point_id_work_count =
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_point_id_work_count;
  return budget;
}

[[nodiscard]] ExactReducedGammaCutBudget tight_cut_budget(
    const ExactReducedGammaCut& cut) {
  ExactReducedGammaCutBudget budget;
  budget.maximum_batch_count = cut.required_batch_capacity;
  budget.maximum_group_record_count = cut.required_group_record_capacity;
  budget.maximum_node_record_count = cut.required_node_record_capacity;
  budget.maximum_prior_root_reference_count =
      cut.required_prior_root_reference_capacity;
  budget.maximum_child_reference_count =
      cut.required_child_reference_capacity;
  budget.maximum_newly_active_facet_count =
      cut.required_newly_active_facet_capacity;
  budget.maximum_equal_level_coface_count =
      cut.required_equal_level_coface_capacity;
  budget.maximum_delta_facet_count = cut.required_delta_facet_capacity;
  budget.maximum_delta_point_reference_count =
      cut.required_delta_point_reference_capacity;
  budget.maximum_active_root_count = cut.required_active_root_capacity;
  budget.maximum_output_facet_reference_count =
      cut.required_output_facet_reference_capacity;
  budget.maximum_output_point_reference_count =
      cut.required_output_point_reference_capacity;
  budget.maximum_facet_replay_work_count =
      cut.required_facet_replay_work_capacity;
  budget.maximum_point_id_replay_work_count =
      cut.required_point_id_replay_work_capacity;
  budget.maximum_result_incidence_facet_check_count =
      cut.required_result_incidence_facet_check_capacity;
  budget.maximum_result_incidence_point_id_work_count =
      cut.required_result_incidence_point_id_work_capacity;
  return budget;
}

[[nodiscard]] bool all_certificates_close(
    const ExactReducedGammaCutVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.source_history_gate_outcome_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.cursor_certified &&
         verification.active_roots_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_journal_replay_certified &&
         verification.
             exact_journal_relative_reduced_gamma_cut_replay_decision_certified;
}

[[nodiscard]] const ExactPersistentReducedGammaActiveRoot*
find_root(const ExactReducedGammaCut& cut, std::size_t root_id) {
  for (const auto& root : cut.active_roots) {
    if (root.root_node_id == root_id) {
      return &root;
    }
  }
  return nullptr;
}

[[nodiscard]] ExactReducedGammaCut make_cut(
    const ExactPersistentReducedGammaOrderHistory& source,
    const ExactLevel& threshold,
    ExactReducedGammaCutBoundary boundary) {
  return build_exact_reduced_gamma_cut(
      source, threshold, boundary, maximum_cut_budget());
}

void check_verified_cut(
    const ExactPersistentReducedGammaOrderHistory& source,
    const ExactLevel& threshold,
    ExactReducedGammaCutBoundary boundary,
    const ExactReducedGammaCut& cut,
    const std::string& label) {
  const ExactReducedGammaCutVerification verification =
      verify_exact_reduced_gamma_cut(
          source, threshold, boundary, cut.requested_budget, cut);
  check(all_certificates_close(verification), label + ": fresh replay closes");
}

[[nodiscard]] CanonicalPointCloud triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud simultaneous_births_cloud() {
  const std::array<CertifiedPoint3, 6> input{
      point(-2.0, 0.0),
      point(0.0, 2.0),
      point(2.0, 0.0),
      point(18.0, 0.0),
      point(20.0, 2.0),
      point(22.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud silent_incidence_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud ternary_cloud() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0),
      point(0.25, 2.0),
      point(2.0, -1.0),
      point(2.0, 3.0),
      point(3.75, 2.0),
      point(4.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] const ExactPersistentReducedGammaOrderHistory&
triangle_history_fixture() {
  static const ExactPersistentReducedGammaOrderHistory source =
      history(triangle_cloud(), 2U);
  return source;
}

[[nodiscard]] const ExactPersistentReducedGammaOrderHistory&
e5_history_fixture() {
  static const ExactPersistentReducedGammaOrderHistory source =
      history(e5_cloud(), 2U);
  return source;
}

void test_default_terminal_empty_prefix_and_source_gate() {
  const ExactReducedGammaCut empty;
  check(
      empty.decision == ExactReducedGammaCutDecision::not_certified &&
          empty.scope == ExactReducedGammaCutScope::unspecified &&
          empty.active_roots.empty() &&
          !empty.journal_relative_cut_replay_certified,
      "the default cut certifies no journal-relative replay");

  const std::array<CertifiedPoint3, 2> terminal_input{
      point(0.0), point(2.0)};
  const CanonicalPointCloud terminal_cloud =
      canonical_cloud(terminal_input);
  const ExactPersistentReducedGammaOrderHistory terminal_history =
      history(terminal_cloud, 2U);
  for (const ExactReducedGammaCutBoundary boundary : {
           ExactReducedGammaCutBoundary::strict_open,
           ExactReducedGammaCutBoundary::closed}) {
    const ExactReducedGammaCut terminal_cut =
        build_exact_reduced_gamma_cut(
            terminal_history, level(1), boundary, {});
    check(
        terminal_cut.decision ==
                ExactReducedGammaCutDecision::
                    complete_empty_terminal_order &&
            terminal_cut.point_count == 2U && terminal_cut.order == 2U &&
            terminal_cut.boundary == boundary &&
            terminal_cut.cursor.activation_level_prefix_count == 0U &&
            terminal_cut.cursor.batch_prefix_count == 0U &&
            terminal_cut.cursor.group_record_prefix_count == 0U &&
            terminal_cut.cursor.node_record_prefix_count == 0U &&
            terminal_cut.active_roots.empty() &&
            terminal_cut.required_batch_capacity == 0U &&
            terminal_cut.required_group_record_capacity == 0U &&
            terminal_cut.required_node_record_capacity == 0U &&
            terminal_cut.required_facet_replay_work_capacity == 0U &&
            terminal_cut.required_point_id_replay_work_capacity == 0U &&
            terminal_cut.global_source_structure_audit_completed_before_prefix_selection &&
            terminal_cut.terminal_order_complete_empty &&
            !terminal_cut.empty_prefix_complete &&
            terminal_cut.journal_relative_cut_replay_certified,
        "terminal k=n is a complete empty cut under either boundary");
    check_verified_cut(
        terminal_history,
        level(1),
        boundary,
        terminal_cut,
        "terminal cut");
  }

  const ExactPersistentReducedGammaOrderHistory& triangle_history =
      triangle_history_fixture();
  const ExactReducedGammaCut empty_prefix =
      build_exact_reduced_gamma_cut(
          triangle_history,
          level(0),
          ExactReducedGammaCutBoundary::closed,
          {});
  check(
      empty_prefix.decision ==
              ExactReducedGammaCutDecision::complete_empty_prefix &&
          empty_prefix.cursor.activation_level_prefix_count == 0U &&
          empty_prefix.cursor.batch_prefix_count == 0U &&
          empty_prefix.cursor.group_record_prefix_count == 0U &&
          empty_prefix.cursor.node_record_prefix_count == 0U &&
          empty_prefix.cursor.selected_by_exact_upper_bound &&
          !empty_prefix.cursor.selected_by_exact_lower_bound &&
          empty_prefix.cursor.first_excluded_squared_level ==
              std::optional<ExactLevel>{level(5, 4)} &&
          empty_prefix.active_roots.empty() &&
          empty_prefix.global_source_structure_audit_completed_before_prefix_selection &&
          empty_prefix.preflight_budget_sufficient &&
          empty_prefix.empty_prefix_complete &&
          !empty_prefix.terminal_order_complete_empty &&
          empty_prefix.journal_relative_cut_replay_certified,
      "a threshold below the first activation closes an empty prefix with a zero budget");
  check_verified_cut(
      triangle_history,
      level(0),
      ExactReducedGammaCutBoundary::closed,
      empty_prefix,
      "zero-budget empty prefix");

  const ExactPersistentReducedGammaOrderHistory uncertified_history;
  const ExactReducedGammaCut rejected = build_exact_reduced_gamma_cut(
      uncertified_history,
      level(0),
      ExactReducedGammaCutBoundary::strict_open,
      {});
  check(
      rejected.decision ==
              ExactReducedGammaCutDecision::
                  source_history_claims_or_structure_rejected &&
          !rejected.source_history_claims_and_structure_accepted &&
          rejected.active_roots.empty() &&
          !rejected.root_replay_started_after_successful_preflight &&
          !rejected.journal_relative_cut_replay_certified,
      "an uncertified 6.14 object cannot be queried as a cut source");

  check_throws<std::invalid_argument>(
      [&triangle_history]() {
        static_cast<void>(build_exact_reduced_gamma_cut(
            triangle_history,
            level(0),
            static_cast<ExactReducedGammaCutBoundary>(255U),
            {}));
      },
      "an invalid external boundary is rejected before replay");
}

[[nodiscard]] std::size_t expected_group_prefix(
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t batch_prefix_count) {
  if (batch_prefix_count == 0U) {
    return 0U;
  }
  const auto& metadata = source.batch_metadata[batch_prefix_count - 1U];
  return metadata.first_group_record_index + metadata.group_record_count;
}

[[nodiscard]] std::size_t expected_node_prefix(
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t batch_prefix_count) {
  std::size_t count = 0U;
  for (std::size_t index = 0U; index < batch_prefix_count; ++index) {
    count += source.batch_metadata[index].created_node_count;
  }
  return count;
}

void check_cursor_prefix(
    const ExactPersistentReducedGammaOrderHistory& source,
    const ExactReducedGammaCut& cut,
    std::size_t expected_batch_prefix,
    const std::optional<ExactLevel>& expected_first_excluded,
    bool expect_lower_bound,
    const std::string& label) {
  check(
      cut.cursor.activation_level_prefix_count == expected_batch_prefix &&
          cut.cursor.batch_prefix_count == expected_batch_prefix &&
          cut.cursor.group_record_prefix_count ==
              expected_group_prefix(source, expected_batch_prefix) &&
          cut.cursor.node_record_prefix_count ==
              expected_node_prefix(source, expected_batch_prefix) &&
          cut.cursor.first_excluded_squared_level ==
              expected_first_excluded &&
          cut.cursor.selected_by_exact_lower_bound == expect_lower_bound &&
          cut.cursor.selected_by_exact_upper_bound == !expect_lower_bound &&
          cut.required_batch_capacity == expected_batch_prefix &&
          cut.required_group_record_capacity ==
              cut.cursor.group_record_prefix_count &&
          cut.required_node_record_capacity ==
              cut.cursor.node_record_prefix_count &&
          cut.counters.replayed_batch_count == expected_batch_prefix &&
          cut.counters.replayed_group_record_count ==
              cut.cursor.group_record_prefix_count &&
          cut.counters.replayed_node_record_count ==
              cut.cursor.node_record_prefix_count,
      label + ": exact boundary selects the expected complete prefix");
}

void test_exact_lower_and_upper_bound_at_every_e5_level() {
  const ExactPersistentReducedGammaOrderHistory& source =
      e5_history_fixture();
  check(
      source.activation_levels.size() == 12U &&
          source.batch_metadata.size() == source.activation_levels.size(),
      "E5 exposes the complete twelve-level sweep used by cut queries");

  for (std::size_t index = 0U;
       index < source.activation_levels.size(); ++index) {
    const ExactLevel& threshold = source.activation_levels[index];
    const ExactReducedGammaCut strict = make_cut(
        source, threshold, ExactReducedGammaCutBoundary::strict_open);
    const ExactReducedGammaCut closed = make_cut(
        source, threshold, ExactReducedGammaCutBoundary::closed);
    const std::optional<ExactLevel> closed_first_excluded =
        index + 1U < source.activation_levels.size()
            ? std::optional<ExactLevel>{source.activation_levels[index + 1U]}
            : std::nullopt;

    check_cursor_prefix(
        source,
        strict,
        index,
        std::optional<ExactLevel>{threshold},
        true,
        "E5 strict cut");
    check_cursor_prefix(
        source,
        closed,
        index + 1U,
        closed_first_excluded,
        false,
        "E5 closed cut");
    check(
        strict.decision ==
                (index == 0U
                     ? ExactReducedGammaCutDecision::complete_empty_prefix
                     : ExactReducedGammaCutDecision::
                           complete_strict_journal_relative_reduced_gamma_cut) &&
            closed.decision ==
                ExactReducedGammaCutDecision::
                    complete_closed_journal_relative_reduced_gamma_cut,
        "strict lower_bound and closed upper_bound retain distinct certified decisions");
    check_verified_cut(
        source,
        threshold,
        ExactReducedGammaCutBoundary::strict_open,
        strict,
        "E5 strict exact-level cut");
    check_verified_cut(
        source,
        threshold,
        ExactReducedGammaCutBoundary::closed,
        closed,
        "E5 closed exact-level cut");
  }

  const ExactReducedGammaCut midpoint_strict = make_cut(
      source, level(3, 2), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut midpoint_closed = make_cut(
      source, level(3, 2), ExactReducedGammaCutBoundary::closed);
  check(
      midpoint_strict.cursor.activation_level_prefix_count == 2U &&
          midpoint_closed.cursor.activation_level_prefix_count == 2U &&
          midpoint_strict.cursor.batch_prefix_count ==
              midpoint_closed.cursor.batch_prefix_count &&
          midpoint_strict.cursor.group_record_prefix_count ==
              midpoint_closed.cursor.group_record_prefix_count &&
          midpoint_strict.cursor.node_record_prefix_count ==
              midpoint_closed.cursor.node_record_prefix_count &&
          midpoint_strict.cursor.first_excluded_squared_level ==
              std::optional<ExactLevel>{level(25, 16)} &&
          midpoint_closed.cursor.first_excluded_squared_level ==
              midpoint_strict.cursor.first_excluded_squared_level &&
          midpoint_strict.active_roots == midpoint_closed.active_roots,
      "between activation levels, strict and closed searches select the same complete prefix and roots");

  const ExactReducedGammaCut above_strict = make_cut(
      source, level(11), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut above_closed = make_cut(
      source, level(11), ExactReducedGammaCutBoundary::closed);
  check(
      above_strict.cursor.activation_level_prefix_count == 12U &&
          above_closed.cursor.activation_level_prefix_count == 12U &&
          !above_strict.cursor.first_excluded_squared_level.has_value() &&
          !above_closed.cursor.first_excluded_squared_level.has_value() &&
          above_strict.active_roots == above_closed.active_roots &&
          above_strict.active_roots == source.final_active_roots,
      "a threshold above E5's maximum level returns the same final state for strict and closed cuts");
}

void test_e5_birth_multifusion_growth_and_redundant_cursor() {
  const ExactPersistentReducedGammaOrderHistory& source =
      e5_history_fixture();

  const ExactReducedGammaCut before_birth = make_cut(
      source, level(25, 16), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut at_birth = make_cut(
      source, level(25, 16), ExactReducedGammaCutBoundary::closed);
  const ExactPersistentReducedGammaActiveRoot* born_root =
      find_root(at_birth, 0U);
  check(
      before_birth.active_roots.empty() &&
          at_birth.active_roots.size() == 1U && born_root != nullptr &&
          born_root->facet_point_ids.size() == 3U &&
          born_root->covered_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}),
      "E5 closed 25/16 creates root zero while the strict cut remains rootless");

  const ExactReducedGammaCut before_merge = make_cut(
      source, level(13, 2), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut at_merge = make_cut(
      source, level(13, 2), ExactReducedGammaCutBoundary::closed);
  const ExactPersistentReducedGammaActiveRoot* merged_root =
      find_root(at_merge, 2U);
  check(
      before_merge.active_roots.size() == 2U &&
          find_root(before_merge, 0U) != nullptr &&
          find_root(before_merge, 1U) != nullptr &&
          at_merge.active_roots.size() == 1U && merged_root != nullptr &&
          merged_root->facet_point_ids.size() == 7U &&
          merged_root->covered_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U, 4U}) &&
          at_merge.cursor.node_record_prefix_count == 3U,
      "E5 strict 13/2 has two roots and the closed cut applies one binary multifusion node");

  const ExactReducedGammaCut before_growth = make_cut(
      source, level(17, 2), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut after_growth = make_cut(
      source, level(17, 2), ExactReducedGammaCutBoundary::closed);
  const ExactPersistentReducedGammaActiveRoot* strict_growth_root =
      find_root(before_growth, 2U);
  const ExactPersistentReducedGammaActiveRoot* closed_growth_root =
      find_root(after_growth, 2U);
  check(
      strict_growth_root != nullptr && closed_growth_root != nullptr &&
          before_growth.cursor.node_record_prefix_count ==
              after_growth.cursor.node_record_prefix_count &&
          strict_growth_root->facet_point_ids.size() + 1U ==
              closed_growth_root->facet_point_ids.size() &&
          strict_growth_root->covered_point_ids ==
              closed_growth_root->covered_point_ids,
      "E5 closed 17/2 grows root two by one facet without changing its ID, node cursor, or point coverage");

  const ExactReducedGammaCut before_redundant = make_cut(
      source, level(85, 9), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut after_redundant = make_cut(
      source, level(85, 9), ExactReducedGammaCutBoundary::closed);
  check(
      before_redundant.active_roots == after_redundant.active_roots &&
          before_redundant.cursor.batch_prefix_count + 1U ==
              after_redundant.cursor.batch_prefix_count &&
          before_redundant.cursor.group_record_prefix_count + 1U ==
              after_redundant.cursor.group_record_prefix_count &&
          before_redundant.cursor.node_record_prefix_count ==
              after_redundant.cursor.node_record_prefix_count &&
          before_redundant.counters.fully_redundant_group_count == 0U &&
          after_redundant.counters.fully_redundant_group_count == 1U,
      "E5 closed 85/9 consumes the fully redundant event even though its active-root payload is identical");
}

void test_triangle_deferred_prefix_changes_cursor_but_not_roots() {
  const ExactPersistentReducedGammaOrderHistory& source =
      triangle_history_fixture();
  const ExactReducedGammaCut strict_deferred = make_cut(
      source, level(5, 4), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut closed_deferred = make_cut(
      source, level(5, 4), ExactReducedGammaCutBoundary::closed);
  check(
      strict_deferred.active_roots.empty() &&
          closed_deferred.active_roots.empty() &&
          strict_deferred.cursor.batch_prefix_count == 0U &&
          closed_deferred.cursor.batch_prefix_count == 1U &&
          strict_deferred.cursor.group_record_prefix_count == 0U &&
          closed_deferred.cursor.group_record_prefix_count == 2U &&
          closed_deferred.counters.replayed_newly_active_facet_count == 2U &&
          closed_deferred.counters.applied_root_mutation_count == 0U,
      "two deferred facets advance the closed cursor without inventing a reduced root");

  const ExactReducedGammaCut strict_birth = make_cut(
      source, level(4), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut closed_birth = make_cut(
      source, level(4), ExactReducedGammaCutBoundary::closed);
  const ExactPersistentReducedGammaActiveRoot* root =
      find_root(closed_birth, 0U);
  check(
      strict_birth.active_roots.empty() &&
          closed_birth.active_roots.size() == 1U && root != nullptr &&
          root->facet_point_ids.size() == 3U &&
          root->covered_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}),
      "the triangle is reduced-empty strictly below four and has one complete root at the closed cut");
}

void test_silent_incidence_changes_facets_without_points_or_root_id() {
  const ExactPersistentReducedGammaOrderHistory source =
      history(silent_incidence_cloud(), 2U);
  const ExactReducedGammaCut strict = make_cut(
      source, level(33, 2), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut closed = make_cut(
      source, level(33, 2), ExactReducedGammaCutBoundary::closed);
  const ExactPersistentReducedGammaActiveRoot* strict_root =
      find_root(strict, 0U);
  const ExactPersistentReducedGammaActiveRoot* closed_root =
      find_root(closed, 0U);
  check(
      strict.active_roots.size() == 1U &&
          closed.active_roots.size() == 1U && strict_root != nullptr &&
          closed_root != nullptr &&
          strict_root->facet_point_ids.size() + 1U ==
              closed_root->facet_point_ids.size() &&
          strict_root->covered_point_ids == closed_root->covered_point_ids &&
          strict.cursor.node_record_prefix_count ==
              closed.cursor.node_record_prefix_count &&
          closed.counters.applied_root_mutation_count ==
              strict.counters.applied_root_mutation_count + 1U,
      "the silent 33/2 incidence grows facet coverage without a point, node, or root-ID change");
}

void test_simultaneous_births_and_ternary_multifusion_are_atomic() {
  const ExactPersistentReducedGammaOrderHistory births_source =
      history(simultaneous_births_cloud(), 2U);
  const ExactReducedGammaCut births_strict = make_cut(
      births_source, level(4), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut births_closed = make_cut(
      births_source, level(4), ExactReducedGammaCutBoundary::closed);
  check(
      births_strict.active_roots.empty() &&
          births_closed.active_roots.size() == 2U &&
          find_root(births_closed, 0U) != nullptr &&
          find_root(births_closed, 1U) != nullptr &&
          births_closed.cursor.node_record_prefix_count == 2U &&
          births_closed.active_roots[0].facet_point_ids.size() == 3U &&
          births_closed.active_roots[1].facet_point_ids.size() == 3U,
      "two equal-level births are both resolved against the empty frozen snapshot");

  std::vector<std::size_t> simultaneous_birth_record_indices;
  for (std::size_t index = 0U;
       index < births_source.group_records.size(); ++index) {
    const auto& record = births_source.group_records[index];
    if (record.squared_level == level(4) &&
        record.created_node_id.has_value()) {
      simultaneous_birth_record_indices.push_back(index);
    }
  }
  check(
      simultaneous_birth_record_indices.size() == 2U,
      "the separated-triangles fixture exposes exactly two simultaneous birth records");
  if (simultaneous_birth_record_indices.size() == 2U) {
    const std::size_t first_index = simultaneous_birth_record_indices[0];
    const std::size_t second_index = simultaneous_birth_record_indices[1];

    ExactPersistentReducedGammaOrderHistory reordered_births =
        births_source;
    auto& reordered_first = reordered_births.group_records[first_index];
    auto& reordered_second = reordered_births.group_records[second_index];
    std::swap(
        reordered_first.canonical_representative_facet_point_ids,
        reordered_second.canonical_representative_facet_point_ids);
    std::swap(
        reordered_first.newly_active_facet_point_ids,
        reordered_second.newly_active_facet_point_ids);
    std::swap(
        reordered_first.equal_level_coface_point_ids,
        reordered_second.equal_level_coface_point_ids);
    std::swap(
        reordered_first.coverage_delta,
        reordered_second.coverage_delta);
    const ExactReducedGammaCut reordered_cut =
        build_exact_reduced_gamma_cut(
            reordered_births,
            level(4),
            ExactReducedGammaCutBoundary::closed,
            maximum_cut_budget());
    check(
        reordered_cut.decision ==
                ExactReducedGammaCutDecision::
                    source_history_claims_or_structure_rejected &&
            reordered_cut.active_roots.empty(),
        "swapping simultaneous birth payloads while keeping root IDs fixed violates canonical group order");

    ExactPersistentReducedGammaOrderHistory crossed_incidences =
        births_source;
    std::swap(
        crossed_incidences.group_records[first_index]
            .equal_level_coface_point_ids,
        crossed_incidences.group_records[second_index]
            .equal_level_coface_point_ids);
    const ExactReducedGammaCut crossed_cut = build_exact_reduced_gamma_cut(
        crossed_incidences,
        level(4),
        ExactReducedGammaCutBoundary::closed,
        maximum_cut_budget());
    check(
        crossed_cut.decision ==
                ExactReducedGammaCutDecision::
                    source_history_claims_or_structure_rejected &&
            crossed_cut.active_roots.empty(),
        "equal-level cofaces cannot be reassigned across simultaneous result components");
  }

  const ExactPersistentReducedGammaOrderHistory ternary_source =
      history(ternary_cloud(), 2U);
  const ExactReducedGammaCut ternary_strict = make_cut(
      ternary_source, level(13, 4), ExactReducedGammaCutBoundary::strict_open);
  const ExactReducedGammaCut ternary_closed = make_cut(
      ternary_source, level(13, 4), ExactReducedGammaCutBoundary::closed);
  check(
      ternary_strict.active_roots.size() == 3U &&
          find_root(ternary_strict, 0U) != nullptr &&
          find_root(ternary_strict, 1U) != nullptr &&
          find_root(ternary_strict, 2U) != nullptr &&
          ternary_closed.active_roots.size() == 1U &&
          find_root(ternary_closed, 3U) != nullptr &&
          ternary_closed.cursor.node_record_prefix_count == 4U &&
          ternary_source.nodes.size() > 3U &&
          ternary_source.nodes[3U].child_node_ids ==
              std::vector<std::size_t>({0U, 1U, 2U}),
      "the closed 13/4 cut applies one ternary node and never exposes binary intermediates");
}

struct BudgetDimension {
  const char* name;
  std::size_t ExactReducedGammaCutBudget::*member;
};

void test_every_budget_dimension_fails_atomically() {
  const ExactPersistentReducedGammaOrderHistory& source =
      e5_history_fixture();
  const ExactLevel threshold = level(10);
  const ExactReducedGammaCut generous = make_cut(
      source, threshold, ExactReducedGammaCutBoundary::closed);
  const ExactReducedGammaCutBudget tight_budget =
      tight_cut_budget(generous);
  const ExactReducedGammaCut tight = build_exact_reduced_gamma_cut(
      source,
      threshold,
      ExactReducedGammaCutBoundary::closed,
      tight_budget);
  const std::size_t expected_global_label_validation_count =
      source.group_records.size() + 3U * source.exhaustive_facet_count +
      source.exhaustive_coface_count;
  const std::size_t expected_global_point_id_validation_count =
      source.order * source.group_records.size() +
      3U * source.order * source.exhaustive_facet_count +
      (source.order + 1U) * source.exhaustive_coface_count +
      source.counters.added_point_reference_count + source.point_count;
  check(
      tight.decision ==
              ExactReducedGammaCutDecision::
                  complete_closed_journal_relative_reduced_gamma_cut &&
          tight.scope ==
              ExactReducedGammaCutScope::
                  bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only &&
          tight.source_history_claims_and_structure_accepted &&
          tight.source_history_certification_is_external_assumption &&
          tight.source_history_geometry_not_freshly_certified &&
          tight.coherent_forged_history_cannot_be_excluded_without_cloud &&
          tight.global_source_structure_audit_completed_before_prefix_selection &&
          tight.prefix_selected_from_exact_threshold_and_boundary &&
          tight.cursor == generous.cursor &&
          tight.active_roots == generous.active_roots &&
          tight.counters == generous.counters &&
          tight.preflight_budget_sufficient &&
          tight.root_replay_started_after_successful_preflight &&
          tight.complete_batches_replayed_from_frozen_snapshots &&
          tight.coverage_deltas_applied_exactly &&
          tight.persistent_root_ids_preserved &&
          tight.active_roots_canonical_and_disjoint_by_facet &&
          tight.prefix_forest_accounting_certified &&
          tight.cursor_matches_replayed_prefix &&
          tight.journal_relative_cut_replay_certified &&
          tight.counters.global_structure_activation_level_count ==
              source.activation_levels.size() &&
          tight.counters.global_structure_batch_metadata_count ==
              source.batch_metadata.size() &&
          tight.counters.global_structure_node_record_count ==
              source.nodes.size() &&
          tight.counters.global_structure_group_record_count ==
              source.group_records.size() &&
          tight.counters.global_structure_label_validation_count ==
              expected_global_label_validation_count &&
          tight.counters.global_structure_point_id_reference_validation_count ==
              expected_global_point_id_validation_count &&
          tight.counters.global_dry_batch_count ==
              source.batch_metadata.size() &&
          tight.counters.global_dry_group_record_count ==
              source.group_records.size() &&
          tight.counters.global_dry_node_record_count == source.nodes.size() &&
          tight.counters.global_dry_prior_root_reference_count ==
              source.counters.group_root_reference_count &&
          tight.counters.global_dry_child_reference_count ==
              source.counters.child_reference_count &&
          tight.counters.global_dry_facet_state_work_count > 0U &&
          tight.counters.result_incidence_facet_check_count > 0U &&
          tight.counters.result_incidence_point_id_work_count > 0U,
      "the exact derived capacities suffice for the complete E5 cut");
  check_verified_cut(
      source,
      threshold,
      ExactReducedGammaCutBoundary::closed,
      tight,
      "tight-budget E5 cut");

  ExactReducedGammaCutBudget unsupported = maximum_cut_budget();
  unsupported.maximum_batch_count =
      ExactReducedGammaCutBudget::maximum_supported_batch_count + 1U;
  check_throws<std::invalid_argument>(
      [&source, &threshold, &unsupported]() {
        static_cast<void>(build_exact_reduced_gamma_cut(
            source,
            threshold,
            ExactReducedGammaCutBoundary::closed,
            unsupported));
      },
      "a requested capacity above a static cut cap is rejected");

  const std::array<BudgetDimension, 16> dimensions{{
      {"batch", &ExactReducedGammaCutBudget::maximum_batch_count},
      {"group record",
       &ExactReducedGammaCutBudget::maximum_group_record_count},
      {"node record",
       &ExactReducedGammaCutBudget::maximum_node_record_count},
      {"prior root reference",
       &ExactReducedGammaCutBudget::maximum_prior_root_reference_count},
      {"child reference",
       &ExactReducedGammaCutBudget::maximum_child_reference_count},
      {"new facet",
       &ExactReducedGammaCutBudget::maximum_newly_active_facet_count},
      {"equal coface",
       &ExactReducedGammaCutBudget::maximum_equal_level_coface_count},
      {"delta facet",
       &ExactReducedGammaCutBudget::maximum_delta_facet_count},
      {"delta point",
       &ExactReducedGammaCutBudget::maximum_delta_point_reference_count},
      {"active root",
       &ExactReducedGammaCutBudget::maximum_active_root_count},
      {"output facet",
       &ExactReducedGammaCutBudget::maximum_output_facet_reference_count},
      {"output point",
       &ExactReducedGammaCutBudget::maximum_output_point_reference_count},
      {"facet replay work",
       &ExactReducedGammaCutBudget::maximum_facet_replay_work_count},
      {"point-ID replay work",
       &ExactReducedGammaCutBudget::maximum_point_id_replay_work_count},
      {"result incidence facet checks",
       &ExactReducedGammaCutBudget::
           maximum_result_incidence_facet_check_count},
      {"result incidence point-ID work",
       &ExactReducedGammaCutBudget::
           maximum_result_incidence_point_id_work_count},
  }};

  for (const BudgetDimension& dimension : dimensions) {
    ExactReducedGammaCutBudget insufficient = tight_budget;
    check(
        insufficient.*(dimension.member) > 0U,
        std::string{"E5 final cut requires positive "} + dimension.name +
            " capacity");
    --(insufficient.*(dimension.member));
    const ExactReducedGammaCut failed = build_exact_reduced_gamma_cut(
        source,
        threshold,
        ExactReducedGammaCutBoundary::closed,
        insufficient);
    check(
        failed.decision ==
                ExactReducedGammaCutDecision::
                    no_cut_preflight_budget_insufficient &&
            !failed.preflight_budget_sufficient &&
            failed.global_source_structure_audit_completed_before_prefix_selection &&
            !failed.root_replay_started_after_successful_preflight &&
            failed.active_roots.empty() &&
            failed.counters.replayed_batch_count == 0U &&
            failed.counters.replayed_group_record_count == 0U &&
            failed.counters.replayed_node_record_count == 0U &&
            failed.counters.applied_root_mutation_count == 0U &&
            failed.counters.output_facet_reference_count == 0U &&
            failed.counters.output_point_reference_count == 0U &&
            failed.counters.facet_replay_work_count == 0U &&
            failed.counters.point_id_replay_work_count == 0U &&
            failed.counters.result_incidence_facet_check_count == 0U &&
            failed.counters.result_incidence_point_id_work_count == 0U &&
            failed.counters.global_dry_batch_count ==
                source.batch_metadata.size() &&
            failed.counters.global_dry_group_record_count ==
                source.group_records.size(),
        std::string{"insufficient "} + dimension.name +
            " capacity refuses the entire cut before root replay");
    check_verified_cut(
        source,
        threshold,
        ExactReducedGammaCutBoundary::closed,
        failed,
        std::string{"atomic insufficient "} + dimension.name +
            " preflight");
  }
}

void check_source_structure_rejected(
    const ExactPersistentReducedGammaOrderHistory& source,
    const std::string& label) {
  const ExactReducedGammaCut cut = build_exact_reduced_gamma_cut(
      source,
      level(10),
      ExactReducedGammaCutBoundary::closed,
      maximum_cut_budget());
  check(
      cut.decision ==
              ExactReducedGammaCutDecision::
                  source_history_claims_or_structure_rejected &&
          !cut.source_history_claims_and_structure_accepted &&
          !cut.preflight_budget_sufficient &&
          !cut.root_replay_started_after_successful_preflight &&
          cut.active_roots.empty(),
      label + ": malformed in-memory 6.14 structure is rejected before cut replay");
}

void test_bounded_hostile_source_structures_fail_closed() {
  const ExactPersistentReducedGammaOrderHistory& source =
      e5_history_fixture();
  check(
      !source.batch_metadata.empty() && !source.group_records.empty() &&
          !source.nodes.empty() && !source.final_active_roots.empty(),
      "E5 supplies every bounded source structure used by mutation tests");

  ExactPersistentReducedGammaOrderHistory bad_offset = source;
  bad_offset.batch_metadata[0].first_group_record_index =
      std::numeric_limits<std::size_t>::max();
  check_source_structure_rejected(bad_offset, "hostile batch offset");

  ExactPersistentReducedGammaOrderHistory non_strict_levels = source;
  non_strict_levels.activation_levels[1] =
      non_strict_levels.activation_levels[0];
  check_source_structure_rejected(
      non_strict_levels, "non-strict activation-level order");

  ExactPersistentReducedGammaOrderHistory bad_component_metadata = source;
  ++bad_component_metadata.batch_metadata[0]
        .strict_nontrivial_component_count;
  check_source_structure_rejected(
      bad_component_metadata, "incoherent strict-component metadata");

  ExactPersistentReducedGammaOrderHistory bad_record = source;
  bad_record.group_records[0].group_record_index =
      std::numeric_limits<std::size_t>::max();
  check_source_structure_rejected(bad_record, "hostile group record index");

  ExactPersistentReducedGammaOrderHistory oversized_nested_record = source;
  bool resized_prior_roots = false;
  for (auto& record : oversized_nested_record.group_records) {
    if (!record.prior_root_node_ids.empty()) {
      record.prior_root_node_ids.resize(
          ExactReducedGammaCutBudget::
                  maximum_supported_prior_root_reference_count +
              1U,
          0U);
      resized_prior_roots = true;
      break;
    }
  }
  check(
      resized_prior_roots,
      "E5 exposes a resolved group for the nested-cap regression");
  check_source_structure_rejected(
      oversized_nested_record, "nested prior-root vector above static cap");

  ExactPersistentReducedGammaOrderHistory bad_node = source;
  bad_node.nodes.back().child_node_ids.push_back(
      std::numeric_limits<std::size_t>::max());
  check_source_structure_rejected(bad_node, "hostile child node reference");

  ExactPersistentReducedGammaOrderHistory future_node_ids = source;
  std::optional<std::size_t> first_birth_record;
  std::optional<std::size_t> second_birth_record;
  for (std::size_t index = 0U;
       index < future_node_ids.group_records.size(); ++index) {
    const auto& record = future_node_ids.group_records[index];
    if (record.created_node_id == std::optional<std::size_t>{0U}) {
      first_birth_record = index;
    } else if (record.created_node_id == std::optional<std::size_t>{1U}) {
      second_birth_record = index;
    }
  }
  check(
      first_birth_record.has_value() && second_birth_record.has_value() &&
          future_node_ids.nodes.size() >= 2U,
      "E5 exposes its first two birth records for the future-node regression");
  if (first_birth_record.has_value() && second_birth_record.has_value() &&
      future_node_ids.nodes.size() >= 2U) {
    auto& first = future_node_ids.group_records[*first_birth_record];
    auto& second = future_node_ids.group_records[*second_birth_record];
    std::swap(first.created_node_id, second.created_node_id);
    std::swap(first.resulting_root_node_id, second.resulting_root_node_id);
    std::swap(
        future_node_ids.nodes[0].creation_batch_index,
        future_node_ids.nodes[1].creation_batch_index);
    std::swap(
        future_node_ids.nodes[0].creation_group_index,
        future_node_ids.nodes[1].creation_group_index);
    std::swap(
        future_node_ids.nodes[0].squared_level,
        future_node_ids.nodes[1].squared_level);
    const ExactReducedGammaCut future_cut = build_exact_reduced_gamma_cut(
        future_node_ids,
        level(25, 16),
        ExactReducedGammaCutBoundary::closed,
        maximum_cut_budget());
    check(
        future_cut.decision ==
                ExactReducedGammaCutDecision::
                    source_history_claims_or_structure_rejected &&
            !future_cut.source_history_claims_and_structure_accepted &&
            future_cut.active_roots.empty() &&
            future_cut.cursor.node_record_prefix_count == 0U,
        "a first birth cannot expose root one before node record one belongs to the replay prefix");
  }

  ExactPersistentReducedGammaOrderHistory bad_result = source;
  bad_result.final_active_roots[0].covered_point_ids.push_back(
      static_cast<PointId>(source.point_count));
  check_source_structure_rejected(bad_result, "hostile final-root point ID");

  ExactPersistentReducedGammaOrderHistory bad_delta_points = source;
  bool cleared_birth_delta = false;
  std::size_t removed_birth_point_reference_count = 0U;
  for (auto& record : bad_delta_points.group_records) {
    if (record.coverage_delta.has_value() &&
        record.prior_root_node_ids.empty() &&
        !record.coverage_delta->added_point_ids.empty()) {
      removed_birth_point_reference_count =
          record.coverage_delta->added_point_ids.size();
      record.coverage_delta->added_point_ids.clear();
      cleared_birth_delta = true;
      break;
    }
  }
  check(
      cleared_birth_delta,
      "E5 exposes a birth delta with points for the dry-preflight regression");
  check(
      bad_delta_points.counters.added_point_reference_count >=
          removed_birth_point_reference_count,
      "the source point-reference counter can remain coherent after the bounded mutation");
  if (bad_delta_points.counters.added_point_reference_count >=
      removed_birth_point_reference_count) {
    bad_delta_points.counters.added_point_reference_count -=
        removed_birth_point_reference_count;
  }
  ExactReducedGammaCutBudget zero_output_points = maximum_cut_budget();
  zero_output_points.maximum_output_point_reference_count = 0U;
  const ExactReducedGammaCut guarded = build_exact_reduced_gamma_cut(
      bad_delta_points,
      level(25, 16),
      ExactReducedGammaCutBoundary::closed,
      zero_output_points);
  check(
          guarded.decision ==
              ExactReducedGammaCutDecision::
                  no_cut_preflight_budget_insufficient &&
          guarded.source_history_claims_and_structure_accepted &&
          guarded.required_output_point_reference_capacity == 5U &&
          !guarded.preflight_budget_sufficient,
      "missing delta point IDs cannot make dry preflight underestimate the output-point capacity");
  check(
      !guarded.root_replay_started_after_successful_preflight &&
          guarded.active_roots.empty(),
      "the malformed delta never reaches a root allocation or partial replay under a zero point budget");
}

void check_observed_cut_mutation_rejected(
    const ExactPersistentReducedGammaOrderHistory& source,
    const ExactLevel& threshold,
    ExactReducedGammaCutBoundary boundary,
    const ExactReducedGammaCutBudget& budget,
    const ExactReducedGammaCut& bad,
    const std::string& label) {
  const ExactReducedGammaCutVerification verification =
      verify_exact_reduced_gamma_cut(
          source, threshold, boundary, budget, bad);
  check(
      !verification.fresh_journal_replay_certified &&
          !verification.
              exact_journal_relative_reduced_gamma_cut_replay_decision_certified,
      label + ": fresh verification rejects the observed cut mutation");
}

void test_observed_payload_mutations_cannot_steer_fresh_verification() {
  const ExactPersistentReducedGammaOrderHistory& source =
      e5_history_fixture();
  const ExactLevel threshold = level(85, 9);
  const ExactReducedGammaCut good = make_cut(
      source, threshold, ExactReducedGammaCutBoundary::closed);
  check_verified_cut(
      source,
      threshold,
      ExactReducedGammaCutBoundary::closed,
      good,
      "unmodified observed E5 cut");

  const auto rejects = [&](ExactReducedGammaCut bad,
                           const std::string& label) {
    check_observed_cut_mutation_rejected(
        source,
        threshold,
        ExactReducedGammaCutBoundary::closed,
        good.requested_budget,
        bad,
        label);
  };

  ExactReducedGammaCut bad_budget = good;
  --bad_budget.requested_budget.maximum_batch_count;
  rejects(bad_budget, "requested budget");

  ExactReducedGammaCut bad_external_level = good;
  bad_external_level.squared_level = level(10);
  rejects(bad_external_level, "embedded threshold");

  ExactReducedGammaCut bad_boundary = good;
  bad_boundary.boundary = ExactReducedGammaCutBoundary::strict_open;
  rejects(bad_boundary, "embedded boundary");

  ExactReducedGammaCut bad_cursor = good;
  bad_cursor.cursor.activation_level_prefix_count =
      std::numeric_limits<std::size_t>::max();
  bad_cursor.cursor.group_record_prefix_count =
      std::numeric_limits<std::size_t>::max();
  rejects(bad_cursor, "hostile observed cursor offsets");

  ExactReducedGammaCut bad_excluded_level = good;
  bad_excluded_level.cursor.first_excluded_squared_level = level(1);
  rejects(bad_excluded_level, "first excluded level");

  ExactReducedGammaCut bad_required_capacity = good;
  ++bad_required_capacity.required_group_record_capacity;
  rejects(bad_required_capacity, "derived required capacity");

  ExactReducedGammaCut bad_root_id = good;
  bad_root_id.active_roots[0].root_node_id =
      std::numeric_limits<std::size_t>::max();
  rejects(bad_root_id, "active-root record ID");

  ExactReducedGammaCut duplicate_root = good;
  duplicate_root.active_roots.push_back(duplicate_root.active_roots[0]);
  rejects(duplicate_root, "duplicate active-root record");

  ExactReducedGammaCut bad_facet_label = good;
  bad_facet_label.active_roots[0].facet_point_ids[0].push_back(
      static_cast<PointId>(source.point_count));
  rejects(bad_facet_label, "active-root facet label");

  ExactReducedGammaCut bad_point_result = good;
  bad_point_result.active_roots[0].covered_point_ids.pop_back();
  rejects(bad_point_result, "active-root point coverage");

  ExactReducedGammaCut bad_counter = good;
  ++bad_counter.counters.replayed_group_record_count;
  rejects(bad_counter, "replay counter");

  ExactReducedGammaCut bad_fact = good;
  bad_fact.cursor_matches_replayed_prefix = false;
  rejects(bad_fact, "result fact");

  ExactReducedGammaCut bad_global_audit_fact = good;
  bad_global_audit_fact
      .global_source_structure_audit_completed_before_prefix_selection =
      false;
  rejects(bad_global_audit_fact, "global source-audit fact");

  ExactReducedGammaCut bad_global_audit_counter = good;
  ++bad_global_audit_counter.counters
        .global_structure_label_validation_count;
  rejects(bad_global_audit_counter, "global source-audit counter");

  ExactReducedGammaCut bad_decision = good;
  bad_decision.decision = ExactReducedGammaCutDecision::not_certified;
  rejects(bad_decision, "decision");

  ExactReducedGammaCut bad_scope = good;
  bad_scope.scope = ExactReducedGammaCutScope::unspecified;
  rejects(bad_scope, "scope");
}

}  // namespace

int main() {
  test_default_terminal_empty_prefix_and_source_gate();
  test_exact_lower_and_upper_bound_at_every_e5_level();
  test_e5_birth_multifusion_growth_and_redundant_cursor();
  test_triangle_deferred_prefix_changes_cursor_but_not_roots();
  test_silent_incidence_changes_facets_without_points_or_root_id();
  test_simultaneous_births_and_ternary_multifusion_are_atomic();
  test_every_budget_dimension_fails_atomically();
  test_bounded_hostile_source_structures_fail_closed();
  test_observed_payload_mutations_cannot_steer_fresh_verification();

  if (failures != 0) {
    std::cerr << failures << " reduced Gamma cut test(s) failed\n";
    return 1;
  }
  std::cout << "all reduced Gamma cut tests passed\n";
  return 0;
}
