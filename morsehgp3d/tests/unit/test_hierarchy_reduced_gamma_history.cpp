#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaBatchMetadata;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaHistoryGroupRecord;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaNodeKind;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistory;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryDecision;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryScope;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaOrderHistoryVerification;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::build_exact_persistent_reduced_gamma_order_history;
using morsehgp3d::hierarchy::verify_exact_persistent_reduced_gamma_order_history;
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

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] std::size_t binomial(
    std::size_t n, std::size_t k) {
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

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget tight_budget(
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

[[nodiscard]] bool all_certificates_close(
    const ExactPersistentReducedGammaOrderHistoryVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.high_cut_gamma_certified &&
         verification.activation_levels_certified &&
         verification.transient_reduced_gamma_batches_replayed_certified &&
         verification.nodes_certified &&
         verification.group_records_certified &&
         verification.batch_metadata_certified &&
         verification.final_active_roots_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_persistent_reduced_gamma_order_history_decision_certified;
}

void check_complete_facts(
    const ExactPersistentReducedGammaOrderHistory& history,
    const std::string& label) {
  check(
      history.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  complete_persistent_reduced_gamma_history &&
          history.scope ==
              ExactPersistentReducedGammaOrderHistoryScope::
                  bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only &&
          history.candidate_space_size_certified &&
          history.preflight_budget_sufficient &&
          history.geometry_started_after_successful_preflight &&
          history.high_cut_equals_twice_exact_squared_diameter &&
          history.high_cut_strictly_above_all_activation_levels &&
          history.high_cut_catalog_exhaustive &&
          history.activation_levels_canonical_and_complete &&
          history.all_reduced_gamma_batches_fresh_replay_certified &&
          history.groups_resolved_against_pre_batch_snapshots &&
          history.mutations_applied_after_complete_batch_resolution &&
          history.active_roots_match_nontrivial_components_after_every_batch &&
          history.node_ids_dense_and_deterministic &&
          history.children_precede_parent &&
          history.each_child_consumed_at_most_once &&
          history.every_exhaustive_coface_affected_exactly_once &&
          history.group_kinds_have_exact_persistent_effects &&
          history.every_non_deferred_group_has_exactly_one_coverage_delta &&
          history.fully_redundant_groups_preserved &&
          history.coverage_deltas_accounted_exactly &&
          history.final_single_root_covers_all_facets_and_points &&
          !history.terminal_order_complete_empty &&
          history.persistent_reduced_gamma_history_certified,
      label + ": all persistent reduced-history facts close");
}

[[nodiscard]] std::optional<std::size_t> batch_index_at(
    const ExactPersistentReducedGammaOrderHistory& history,
    const ExactLevel& squared_level) {
  for (std::size_t index = 0U;
       index < history.batch_metadata.size(); ++index) {
    if (history.batch_metadata[index].squared_level == squared_level) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<
    const ExactPersistentReducedGammaHistoryGroupRecord*>
events_at(
    const ExactPersistentReducedGammaOrderHistory& history,
    const ExactLevel& squared_level) {
  std::vector<const ExactPersistentReducedGammaHistoryGroupRecord*> events;
  for (const ExactPersistentReducedGammaHistoryGroupRecord& event :
       history.group_records) {
    if (event.squared_level == squared_level) {
      events.push_back(&event);
    }
  }
  return events;
}

using FacetSet = std::set<std::vector<PointId>>;
using PointSet = std::set<PointId>;

struct ReplayedRootCoverage {
  FacetSet facets;
  PointSet points;
};

[[nodiscard]] PointSet covered_points(const FacetSet& facets) {
  PointSet points;
  for (const std::vector<PointId>& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return points;
}

[[nodiscard]] std::vector<PointId> point_vector(const PointSet& points) {
  return {points.begin(), points.end()};
}

void check_compact_journal_replays_from_deltas(
    const ExactPersistentReducedGammaOrderHistory& history,
    const std::string& label) {
  std::map<std::size_t, ReplayedRootCoverage> active_roots;
  FacetSet assigned_new_facets;
  FacetSet assigned_delta_facets;
  std::set<std::vector<PointId>> assigned_cofaces;
  bool ranges_valid = true;
  bool records_valid = true;
  bool effects_valid = true;
  bool deltas_valid = true;

  for (const ExactPersistentReducedGammaBatchMetadata& metadata :
       history.batch_metadata) {
    if (metadata.first_group_record_index >
            history.group_records.size() ||
        metadata.group_record_count >
            history.group_records.size() -
                metadata.first_group_record_index) {
      ranges_valid = false;
      continue;
    }

    const auto snapshot = active_roots;
    std::set<std::size_t> consumed_roots;
    std::vector<std::pair<std::size_t, ReplayedRootCoverage>> outputs;
    outputs.reserve(metadata.group_record_count);

    for (std::size_t offset = 0U;
         offset < metadata.group_record_count; ++offset) {
      const auto& record = history.group_records[
          metadata.first_group_record_index + offset];
      records_valid =
          records_valid && record.batch_index == metadata.batch_index &&
          record.squared_level == metadata.squared_level;

      for (const std::vector<PointId>& facet :
           record.newly_active_facet_point_ids) {
        records_valid =
            records_valid && assigned_new_facets.insert(facet).second;
      }
      for (const std::vector<PointId>& coface :
           record.equal_level_coface_point_ids) {
        records_valid =
            records_valid && assigned_cofaces.insert(coface).second;
      }

      if (record.kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
        effects_valid =
            effects_valid && record.prior_root_node_ids.empty() &&
            !record.resulting_root_node_id.has_value() &&
            !record.created_node_id.has_value() &&
            !record.coverage_delta.has_value() &&
            record.equal_level_coface_point_ids.empty();
        continue;
      }

      if (!record.coverage_delta.has_value() ||
          !record.resulting_root_node_id.has_value()) {
        effects_valid = false;
        continue;
      }

      ReplayedRootCoverage output;
      PointSet parent_points;
      for (const std::size_t root_id : record.prior_root_node_ids) {
        const auto parent = snapshot.find(root_id);
        if (parent == snapshot.end()) {
          effects_valid = false;
          continue;
        }
        output.facets.insert(
            parent->second.facets.begin(), parent->second.facets.end());
        parent_points.insert(
            parent->second.points.begin(), parent->second.points.end());
        effects_valid = effects_valid && consumed_roots.insert(root_id).second;
      }

      const auto& delta = *record.coverage_delta;
      for (const std::vector<PointId>& facet :
           delta.added_facet_point_ids) {
        deltas_valid = deltas_valid &&
                       output.facets.insert(facet).second &&
                       assigned_delta_facets.insert(facet).second;
      }
      output.points = covered_points(output.facets);
      PointSet expected_added_points;
      std::set_difference(
          output.points.begin(),
          output.points.end(),
          parent_points.begin(),
          parent_points.end(),
          std::inserter(expected_added_points, expected_added_points.end()));
      deltas_valid =
          deltas_valid &&
          point_vector(expected_added_points) == delta.added_point_ids &&
          delta.fully_redundant ==
              (delta.added_facet_point_ids.empty() &&
               delta.added_point_ids.empty());

      const std::size_t resulting_root = *record.resulting_root_node_id;
      if (record.kind == ExactReducedGammaBatchGroupKind::birth) {
        effects_valid =
            effects_valid && record.prior_root_node_ids.empty() &&
            record.created_node_id == record.resulting_root_node_id;
      } else if (
          record.kind == ExactReducedGammaBatchGroupKind::continuation) {
        effects_valid =
            effects_valid && record.prior_root_node_ids.size() == 1U &&
            !record.created_node_id.has_value() &&
            record.prior_root_node_ids[0] == resulting_root;
      } else if (
          record.kind == ExactReducedGammaBatchGroupKind::multifusion) {
        effects_valid =
            effects_valid && record.prior_root_node_ids.size() >= 2U &&
            record.created_node_id == record.resulting_root_node_id;
      } else {
        effects_valid = false;
      }
      outputs.emplace_back(resulting_root, std::move(output));
    }

    std::map<std::size_t, ReplayedRootCoverage> next = snapshot;
    for (const std::size_t root_id : consumed_roots) {
      next.erase(root_id);
    }
    for (auto& [root_id, output] : outputs) {
      effects_valid =
          effects_valid && next.emplace(root_id, std::move(output)).second;
    }
    active_roots = std::move(next);
  }

  check(
      ranges_valid && records_valid && effects_valid && deltas_valid,
      label +
          ": the compact journal reconstructs every root only from frozen parents and exact deltas");
  check(
      assigned_new_facets.size() == history.exhaustive_facet_count &&
          assigned_cofaces.size() == history.exhaustive_coface_count &&
          assigned_delta_facets.size() == history.exhaustive_facet_count,
      label +
          ": every exhaustive facet and coface is assigned exactly once and every facet enters coverage exactly once");
  check(
      active_roots.size() == history.final_active_roots.size(),
      label + ": replay reconstructs the exact number of final roots");
  for (const auto& root : history.final_active_roots) {
    const auto replayed = active_roots.find(root.root_node_id);
    check(
        replayed != active_roots.end() &&
            replayed->second.facets ==
                FacetSet{
                    root.facet_point_ids.begin(),
                    root.facet_point_ids.end()} &&
            replayed->second.points ==
                PointSet{
                    root.covered_point_ids.begin(),
                    root.covered_point_ids.end()},
        label +
            ": final root coverage equals the independent delta replay");
  }
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

void test_default_terminal_and_order_one_rejection() {
  const ExactPersistentReducedGammaOrderHistory empty;
  check(
      empty.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  not_certified &&
          empty.scope ==
              ExactPersistentReducedGammaOrderHistoryScope::unspecified &&
          empty.activation_levels.empty() && empty.nodes.empty() &&
          empty.group_records.empty() && empty.final_active_roots.empty() &&
          !empty.persistent_reduced_gamma_history_certified,
      "the default persistent history certifies no genealogy");

  const std::array<CertifiedPoint3, 2> terminal_input{
      point(0.0), point(2.0)};
  const CanonicalPointCloud terminal_cloud =
      canonical_cloud(terminal_input);
  const ExactPersistentReducedGammaOrderHistoryBudget zero_budget;
  const ExactPersistentReducedGammaOrderHistory terminal =
      build_exact_persistent_reduced_gamma_order_history(
          terminal_cloud, 2U, zero_budget);
  const ExactPersistentReducedGammaOrderHistoryVerification verification =
      verify_exact_persistent_reduced_gamma_order_history(
          terminal_cloud, 2U, zero_budget, terminal);

  check(
      terminal.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  complete_empty_terminal_order &&
          terminal.scope ==
              ExactPersistentReducedGammaOrderHistoryScope::
                  bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only &&
          terminal.exhaustive_facet_count == 1U &&
          terminal.exhaustive_coface_count == 0U &&
          terminal.required_activation_level_capacity == 0U &&
          terminal.required_node_capacity == 0U &&
          terminal.required_child_reference_capacity == 0U &&
          terminal.required_group_root_reference_capacity == 0U &&
          terminal.activation_levels.empty() &&
          terminal.nodes.empty() && terminal.group_records.empty() &&
          terminal.batch_metadata.empty() &&
          terminal.final_active_roots.empty() &&
          !terminal.high_cut_squared_level.has_value() &&
          !terminal.high_cut_gamma.has_value() &&
          terminal.terminal_order_complete_empty &&
          terminal.persistent_reduced_gamma_history_certified &&
          !terminal.final_single_root_covers_all_facets_and_points &&
          terminal.counters.added_facet_count == 0U &&
          terminal.counters.created_node_count == 0U &&
          terminal.counters.child_reference_count == 0U &&
          terminal.counters.group_root_reference_count == 0U &&
          terminal.counters.final_active_root_count == 0U &&
          all_certificates_close(verification),
      "the terminal k=n>1 branch is an explicitly certified empty reduced history without underflow");

  ExactPersistentReducedGammaOrderHistory forged_terminal = terminal;
  forged_terminal.decision =
      ExactPersistentReducedGammaOrderHistoryDecision::
          complete_persistent_reduced_gamma_history;
  forged_terminal.exact_diameter_squared = level(1);
  forged_terminal.high_cut_squared_level = level(2);
  forged_terminal.high_cut_gamma.emplace();
  const auto forged_terminal_verification =
      verify_exact_persistent_reduced_gamma_order_history(
          terminal_cloud, 2U, zero_budget, forged_terminal);
  check(
      !forged_terminal_verification.high_cut_gamma_certified &&
          !forged_terminal_verification.decision_certified &&
          !forged_terminal_verification.fresh_replay_certified &&
          !forged_terminal_verification.
              exact_persistent_reduced_gamma_order_history_decision_certified,
      "an observed forged normal decision cannot steer a terminal subordinate replay");

  check_throws<std::invalid_argument>(
      [&terminal_cloud]() {
        static_cast<void>(
            build_exact_persistent_reduced_gamma_order_history(
                terminal_cloud,
                1U,
                ExactPersistentReducedGammaOrderHistoryBudget{}));
      },
      "order one remains owned by the exact EMST contract");
}

void test_triangle_deferred_facets_then_one_birth() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(3U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  check_complete_facts(history, "triangle history");
  check_compact_journal_replays_from_deltas(history, "triangle history");
  check(
      history.exact_diameter_squared ==
              std::optional<ExactLevel>{level(16)} &&
          history.high_cut_squared_level ==
              std::optional<ExactLevel>{level(32)} &&
          history.high_cut_gamma.has_value() &&
          history.high_cut_gamma->active_facets.size() == 3U &&
          history.high_cut_gamma->active_cofaces.size() == 1U &&
          history.counters.diameter_pair_distance_evaluation_count == 3U &&
          history.activation_levels ==
              std::vector<ExactLevel>({level(5, 4), level(4)}) &&
          history.batch_metadata.size() == 2U &&
          history.group_records.size() == 3U &&
          history.nodes.size() == 1U &&
          history.counters.deferred_group_count == 2U &&
          history.counters.birth_group_count == 1U &&
          history.counters.continuation_group_count == 0U &&
          history.counters.multifusion_group_count == 0U,
      "the triangle exposes its two lateral facets before its reduced birth");

  const auto deferred = events_at(history, level(5, 4));
  check(
      deferred.size() == 2U &&
          deferred[0]->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet &&
          deferred[1]->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet &&
          !deferred[0]->resulting_root_node_id.has_value() &&
          !deferred[1]->resulting_root_node_id.has_value(),
      "facet-only groups stay deferred without persistent identifiers");

  const auto births = events_at(history, level(4));
  check(
      births.size() == 1U &&
          births[0]->kind == ExactReducedGammaBatchGroupKind::birth &&
          births[0]->prior_root_node_ids.empty() &&
          births[0]->created_node_id == std::optional<std::size_t>{0U} &&
          births[0]->resulting_root_node_id ==
              std::optional<std::size_t>{0U} &&
          births[0]->newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 2U}}) &&
          births[0]->equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U, 2U}}) &&
          births[0]->coverage_delta.has_value() &&
          births[0]->coverage_delta->added_facet_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U}, {0U, 2U}, {1U, 2U}}) &&
          births[0]->coverage_delta->added_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}),
      "the first coface creates one reduced root covering all three facets");

  check(
      !history.nodes.empty() && history.nodes[0].node_id == 0U &&
          history.nodes[0].kind == ExactPersistentReducedGammaNodeKind::birth &&
          history.nodes[0].child_node_ids.empty() &&
          history.final_active_roots.size() == 1U &&
          history.final_active_roots[0].root_node_id == 0U &&
          history.final_active_roots[0].facet_point_ids.size() == 3U &&
          history.counters.added_facet_count == 3U,
      "the triangle genealogy and final coverage replay exactly");
}

void test_two_simultaneous_births_use_one_frozen_snapshot() {
  const std::array<CertifiedPoint3, 6> input{
      point(-2.0, 0.0),
      point(0.0, 2.0),
      point(2.0, 0.0),
      point(18.0, 0.0),
      point(20.0, 2.0),
      point(22.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(6U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  check_complete_facts(history, "two-triangle history");
  check_compact_journal_replays_from_deltas(
      history, "two-triangle history");
  const auto batch_index = batch_index_at(history, level(4));
  const auto births = events_at(history, level(4));
  check(
      batch_index.has_value() && births.size() == 2U,
      "the congruent triangles share exactly one two-group level-four batch");
  if (batch_index.has_value()) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[*batch_index];
    check(
        metadata.group_record_count == 2U &&
            metadata.created_node_count == 2U &&
            metadata.active_root_count_before == 0U &&
            metadata.active_root_count_after == 2U &&
            metadata.pre_batch_root_bijection_certified &&
            metadata.all_groups_resolved_before_mutation &&
            metadata.post_batch_root_bijection_certified,
        "both births resolve against the same empty pre-batch root snapshot");
  }
  if (births.size() == 2U) {
    check(
        births[0]->kind == ExactReducedGammaBatchGroupKind::birth &&
            births[1]->kind == ExactReducedGammaBatchGroupKind::birth &&
            births[0]->prior_root_node_ids.empty() &&
            births[1]->prior_root_node_ids.empty() &&
            births[0]->created_node_id ==
                std::optional<std::size_t>{0U} &&
            births[1]->created_node_id ==
                std::optional<std::size_t>{1U} &&
            births[0]->newly_active_facet_point_ids ==
                std::vector<std::vector<PointId>>({{0U, 2U}}) &&
            births[1]->newly_active_facet_point_ids ==
                std::vector<std::vector<PointId>>({{3U, 5U}}) &&
            births[0]->coverage_delta.has_value() &&
            births[1]->coverage_delta.has_value() &&
            births[0]->coverage_delta->added_facet_point_ids ==
                std::vector<std::vector<PointId>>(
                    {{0U, 1U}, {0U, 2U}, {1U, 2U}}) &&
            births[1]->coverage_delta->added_facet_point_ids ==
                std::vector<std::vector<PointId>>(
                    {{3U, 4U}, {3U, 5U}, {4U, 5U}}),
        "the second simultaneous group cannot observe the first new root");
  }

  const auto merges = events_at(history, level(82));
  check(
      merges.size() == 1U &&
          merges[0]->kind ==
              ExactReducedGammaBatchGroupKind::multifusion &&
          merges[0]->prior_root_node_ids ==
              std::vector<std::size_t>({0U, 1U}) &&
          merges[0]->created_node_id ==
              std::optional<std::size_t>{2U} &&
          merges[0]->resulting_root_node_id ==
              std::optional<std::size_t>{2U},
      "the first cross-triangle cofaces later create one binary multifusion");
  check(
      history.nodes.size() == 3U &&
          history.final_active_roots.size() == 1U &&
          history.final_active_roots[0].root_node_id == 2U &&
          history.final_active_roots[0].facet_point_ids.size() == 15U &&
          history.final_active_roots[0].covered_point_ids.size() == 6U,
      "the complete two-triangle sweep ends in one root covering all fifteen facets");
}

void test_one_equal_level_group_preserves_a_ternary_multifusion() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0),
      point(0.25, 2.0),
      point(2.0, -1.0),
      point(2.0, 3.0),
      point(3.75, 2.0),
      point(4.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, tight_budget(6U, 2U));

  const auto records = events_at(history, level(13, 4));
  const auto merge = std::find_if(
      records.begin(),
      records.end(),
      [](const auto* record) {
        return record->kind ==
               ExactReducedGammaBatchGroupKind::multifusion;
      });
  check(
      merge != records.end(),
      "the ternary fixture contains its exact multifusion level");
  if (merge != records.end()) {
    check(
        (*merge)->prior_root_node_ids.size() == 3U &&
            (*merge)->created_node_id.has_value() &&
            (*merge)->resulting_root_node_id ==
                (*merge)->created_node_id &&
            (*merge)->prior_root_node_ids ==
                std::vector<std::size_t>({0U, 1U, 2U}) &&
            (*merge)->created_node_id ==
                std::optional<std::size_t>{3U} &&
            (*merge)->equal_level_coface_point_ids ==
                std::vector<std::vector<PointId>>(
                    {{0U, 1U, 3U}, {3U, 4U, 5U}}) &&
            (*merge)->coverage_delta.has_value() &&
            !(*merge)->coverage_delta->fully_redundant &&
            (*merge)->coverage_delta->added_facet_point_ids ==
                std::vector<std::vector<PointId>>(
                    {{0U, 3U}, {3U, 5U}}) &&
            (*merge)->coverage_delta->added_point_ids.empty(),
        "one equal-level group contracts three frozen roots without binary expansion");
    if ((*merge)->created_node_id.has_value()) {
      const std::size_t node_id = *(*merge)->created_node_id;
      check(
          node_id < history.nodes.size() &&
              history.nodes[node_id].kind ==
                  ExactPersistentReducedGammaNodeKind::multifusion &&
              history.nodes[node_id].child_node_ids ==
                  (*merge)->prior_root_node_ids &&
              history.nodes[node_id].child_node_ids.size() == 3U,
          "the ternary multifusion is one parent node with three older children");
    }
  }
  check_compact_journal_replays_from_deltas(
      history, "ternary multifusion history");
}

void test_fully_redundant_continuation_keeps_its_event_and_id() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0),
      point(4.0, 0.0),
      point(1.0, 3.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(4U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  check_complete_facts(history, "fully redundant continuation history");
  check_compact_journal_replays_from_deltas(
      history, "fully redundant continuation history");
  const auto continuations = events_at(history, level(5));
  check(
      continuations.size() == 1U &&
          continuations[0]->kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          continuations[0]->prior_root_node_ids ==
              std::vector<std::size_t>({0U}) &&
          continuations[0]->resulting_root_node_id ==
              std::optional<std::size_t>{0U} &&
          !continuations[0]->created_node_id.has_value() &&
          continuations[0]->coverage_delta.has_value() &&
          continuations[0]->coverage_delta->added_facet_point_ids.empty() &&
          continuations[0]->coverage_delta->added_point_ids.empty() &&
          continuations[0]->coverage_delta->fully_redundant,
      "the level-five continuation retains root zero and an explicit empty delta");
  check(
      history.nodes.size() == 1U && history.group_records.size() > 1U &&
          history.final_active_roots.size() == 1U &&
          history.final_active_roots[0].root_node_id == 0U &&
          history.final_active_roots[0].facet_point_ids.size() == 6U,
      "a fully redundant group remains in the journal without creating a node");
}

void test_silent_incidence_persists_and_controls_the_later_continuation() {
  // Source labels are A, B, C, D, E. Canonical ids are D=0, A=1,
  // B=2, C=3 and E=4.
  const std::array<CertifiedPoint3, 5> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(5U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  check_complete_facts(history, "silent-incidence history");
  check_compact_journal_replays_from_deltas(
      history, "silent-incidence history");
  const auto silent = events_at(history, level(33, 2));
  check(
      silent.size() == 1U &&
          silent[0]->kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          silent[0]->prior_root_node_ids ==
              std::vector<std::size_t>({0U}) &&
          silent[0]->resulting_root_node_id ==
              std::optional<std::size_t>{0U} &&
          !silent[0]->created_node_id.has_value() &&
          silent[0]->coverage_delta.has_value() &&
          silent[0]->coverage_delta->added_facet_point_ids ==
              std::vector<std::vector<PointId>>({{1U, 3U}}) &&
          silent[0]->coverage_delta->added_point_ids.empty() &&
          !silent[0]->coverage_delta->fully_redundant &&
          silent[0]->newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{1U, 3U}}) &&
          silent[0]->equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 3U}, {1U, 3U, 4U}}),
      "the two non-Gabriel cofaces add AC=13 without adding a point or node");

  const auto later = events_at(history, level(83886, 3563));
  check(
      later.size() == 1U &&
          later[0]->kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          later[0]->prior_root_node_ids ==
              std::vector<std::size_t>({0U}) &&
          later[0]->resulting_root_node_id ==
              std::optional<std::size_t>{0U} &&
          !later[0]->created_node_id.has_value() &&
          later[0]->coverage_delta.has_value() &&
          later[0]->coverage_delta->added_facet_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{1U, 2U}, {2U, 3U}}) &&
          later[0]->coverage_delta->added_point_ids ==
              std::vector<PointId>({2U}) &&
          later[0]->newly_active_facet_point_ids.empty() &&
          later[0]->equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{1U, 2U, 3U}}),
      "the later ABC coface absorbs the already active AB and BC facets through silent AC and therefore continues root zero instead of creating a false birth");
  check(
      history.nodes.size() == 1U &&
          history.final_active_roots.size() == 1U &&
          history.final_active_roots[0].root_node_id == 0U &&
          history.final_active_roots[0].facet_point_ids.size() == 10U &&
          history.final_active_roots[0].covered_point_ids.size() == 5U,
      "the silent incidence remains effective through the final exhaustive history");
}

void test_e5_complete_genealogy_and_counters() {
  const CanonicalPointCloud cloud = e5_cloud();
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(5U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  check_complete_facts(history, "E5 persistent genealogy");
  check_compact_journal_replays_from_deltas(
      history, "E5 persistent genealogy");
  check(
      history.activation_levels ==
              std::vector<ExactLevel>(
                  {level(1),
                   level(5, 4),
                   level(25, 16),
                   level(5, 2),
                   level(13, 4),
                   level(17, 4),
                   level(1105, 242),
                   level(13, 2),
                   level(17, 2),
                   level(9),
                   level(85, 9),
                   level(10)}) &&
          history.batch_metadata.size() == 12U &&
          history.group_records.size() == 13U &&
          history.nodes.size() == 3U,
      "E5 sweeps every distinct facet and coface level exactly once");

  const auto birth0 = events_at(history, level(25, 16));
  const auto birth1 = events_at(history, level(1105, 242));
  const auto merge = events_at(history, level(13, 2));
  check(
      birth0.size() == 1U && birth1.size() == 1U &&
          merge.size() == 1U &&
          birth0[0]->kind == ExactReducedGammaBatchGroupKind::birth &&
          birth0[0]->created_node_id ==
              std::optional<std::size_t>{0U} &&
          birth1[0]->kind == ExactReducedGammaBatchGroupKind::birth &&
          birth1[0]->created_node_id ==
              std::optional<std::size_t>{1U} &&
          merge[0]->kind ==
              ExactReducedGammaBatchGroupKind::multifusion &&
          merge[0]->prior_root_node_ids ==
              std::vector<std::size_t>({0U, 1U}) &&
          merge[0]->created_node_id ==
              std::optional<std::size_t>{2U} &&
          merge[0]->resulting_root_node_id ==
              std::optional<std::size_t>{2U},
      "E5 creates two births followed by one binary multifusion");

  const auto continuation = events_at(history, level(17, 2));
  const auto redundant = events_at(history, level(85, 9));
  check(
      continuation.size() == 1U && redundant.size() == 1U &&
          continuation[0]->kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          continuation[0]->resulting_root_node_id ==
              std::optional<std::size_t>{2U} &&
          continuation[0]->coverage_delta.has_value() &&
          continuation[0]->coverage_delta->added_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 3U}}) &&
          continuation[0]->coverage_delta->added_point_ids.empty() &&
          redundant[0]->kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          redundant[0]->resulting_root_node_id ==
              std::optional<std::size_t>{2U} &&
          redundant[0]->coverage_delta.has_value() &&
          redundant[0]->coverage_delta->fully_redundant &&
          redundant[0]->coverage_delta->added_facet_point_ids.empty() &&
          redundant[0]->coverage_delta->added_point_ids.empty(),
      "E5 retains both growing and fully redundant continuations on root two");

  check(
          history.counters.activation_level_count == 12U &&
          history.counters.reduced_gamma_batch_build_count == 12U &&
          history.counters.reduced_gamma_group_count == 13U &&
          history.counters.history_group_record_count == 13U &&
          history.counters.deferred_group_count == 6U &&
          history.counters.birth_group_count == 2U &&
          history.counters.continuation_group_count == 4U &&
          history.counters.multifusion_group_count == 1U &&
          history.counters.fully_redundant_group_count == 1U &&
          history.counters.group_root_reference_count == 6U &&
          history.counters.group_newly_active_facet_count == 10U &&
          history.counters.group_equal_level_coface_count == 10U &&
          history.counters.created_node_count == 3U &&
          history.counters.child_reference_count == 2U &&
          history.counters.consumed_child_count == 2U &&
          history.counters.coverage_delta_count == 7U &&
          history.counters.added_facet_count == 10U &&
          history.counters.added_point_reference_count == 6U &&
          history.counters.final_active_root_count == 1U &&
          history.final_active_roots.size() == 1U &&
          history.final_active_roots[0].root_node_id == 2U &&
          history.final_active_roots[0].facet_point_ids.size() == 10U &&
          history.final_active_roots[0].covered_point_ids.size() == 5U &&
          history.nodes[0].child_node_ids.empty() &&
          history.nodes[1].child_node_ids.empty() &&
          history.nodes[2].kind ==
              ExactPersistentReducedGammaNodeKind::multifusion &&
          history.nodes[2].child_node_ids ==
              std::vector<std::size_t>({0U, 1U}),
      "E5 closes node, parent, event, delta and final-root accounting");
}

void check_atomic_budget_failure(
    const CanonicalPointCloud& cloud,
    const ExactPersistentReducedGammaOrderHistoryBudget& budget,
    const std::string& label) {
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);
  const ExactPersistentReducedGammaOrderHistoryVerification verification =
      verify_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget, history);
  check(
      history.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  no_history_preflight_budget_insufficient &&
          history.candidate_space_size_certified &&
          !history.preflight_budget_sufficient &&
          !history.geometry_started_after_successful_preflight &&
          !history.high_cut_squared_level.has_value() &&
          !history.high_cut_gamma.has_value() &&
          history.activation_levels.empty() &&
          history.nodes.empty() && history.group_records.empty() &&
          history.batch_metadata.empty() &&
          history.final_active_roots.empty() &&
          history.counters.preflight_count == 1U &&
          history.counters.diameter_pair_distance_evaluation_count == 0U &&
          history.counters.high_cut_gamma_build_count == 0U &&
          history.counters.reduced_gamma_batch_build_count == 0U &&
          history.counters.history_group_record_count == 0U &&
          all_certificates_close(verification),
      label + ": insufficient capacity fails before geometry and publishes no partial history");
}

void test_atomic_preflight_budgets_and_n14_caps() {
  const CanonicalPointCloud cloud = e5_cloud();
  const ExactPersistentReducedGammaOrderHistoryBudget sufficient =
      tight_budget(5U, 2U);

  ExactPersistentReducedGammaOrderHistoryBudget candidate = sufficient;
  --candidate.gamma_budget.maximum_enumerated_facet_count;
  check_atomic_budget_failure(cloud, candidate, "facet replay budget");
  candidate = sufficient;
  --candidate.gamma_budget.maximum_enumerated_coface_count;
  check_atomic_budget_failure(cloud, candidate, "coface replay budget");
  candidate = sufficient;
  --candidate.gamma_budget.maximum_union_attempt_count;
  check_atomic_budget_failure(cloud, candidate, "union replay budget");
  candidate = sufficient;
  --candidate.maximum_activation_level_count;
  check_atomic_budget_failure(cloud, candidate, "activation-level capacity");
  candidate = sufficient;
  --candidate.maximum_total_facet_work_count;
  check_atomic_budget_failure(cloud, candidate, "total facet-work capacity");
  candidate = sufficient;
  --candidate.maximum_total_coface_work_count;
  check_atomic_budget_failure(cloud, candidate, "total coface-work capacity");
  candidate = sufficient;
  --candidate.maximum_total_union_work_count;
  check_atomic_budget_failure(cloud, candidate, "total union-work capacity");
  candidate = sufficient;
  --candidate.maximum_node_count;
  check_atomic_budget_failure(cloud, candidate, "node capacity");
  candidate = sufficient;
  --candidate.maximum_child_reference_count;
  check_atomic_budget_failure(cloud, candidate, "child-reference capacity");
  candidate = sufficient;
  --candidate.maximum_group_root_reference_count;
  check_atomic_budget_failure(cloud, candidate, "group-root-reference capacity");
  candidate = sufficient;
  --candidate.maximum_group_count;
  check_atomic_budget_failure(cloud, candidate, "group capacity");
  candidate = sufficient;
  --candidate.maximum_group_newly_active_facet_count;
  check_atomic_budget_failure(cloud, candidate, "group facet-record capacity");
  candidate = sufficient;
  --candidate.maximum_group_equal_level_coface_count;
  check_atomic_budget_failure(cloud, candidate, "group coface-record capacity");
  candidate = sufficient;
  --candidate.maximum_delta_facet_count;
  check_atomic_budget_failure(cloud, candidate, "delta-facet capacity");
  candidate = sufficient;
  --candidate.maximum_delta_point_reference_count;
  check_atomic_budget_failure(cloud, candidate, "delta-point capacity");

  std::vector<CertifiedPoint3> maximal_points;
  maximal_points.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    maximal_points.push_back(point(static_cast<double>(index)));
  }
  const CanonicalPointCloud maximal_cloud = canonical_cloud(maximal_points);
  const ExactPersistentReducedGammaOrderHistoryBudget zero_budget;
  const ExactPersistentReducedGammaOrderHistory preflight =
      build_exact_persistent_reduced_gamma_order_history(
          maximal_cloud, 7U, zero_budget);
  const ExactPersistentReducedGammaOrderHistoryVerification verification =
      verify_exact_persistent_reduced_gamma_order_history(
          maximal_cloud, 7U, zero_budget, preflight);

  check(
      preflight.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  no_history_preflight_budget_insufficient &&
          preflight.exhaustive_facet_count == 3432U &&
          preflight.exhaustive_coface_count == 3003U &&
          preflight.exhaustive_union_attempt_count == 21021U &&
          preflight.required_activation_level_capacity == 6435U &&
          preflight.required_total_facet_work_capacity == 22088352U &&
          preflight.required_total_coface_work_capacity == 19327308U &&
          preflight.required_total_union_work_capacity == 135291156U &&
          preflight.required_node_capacity == 3003U &&
          preflight.required_child_reference_capacity == 3002U &&
          preflight.required_group_root_reference_capacity == 3002U &&
          preflight.required_group_capacity == 6435U &&
          preflight.required_group_newly_active_facet_capacity == 3432U &&
          preflight.required_group_equal_level_coface_capacity == 3003U &&
          preflight.required_delta_facet_capacity == 3432U &&
          preflight.required_delta_point_reference_capacity == 24024U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_activation_level_count == 6435U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_total_facet_work_count == 22088352U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_total_union_work_count == 135291156U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_delta_point_reference_count == 24024U &&
          preflight.counters.diameter_pair_distance_evaluation_count == 0U &&
          all_certificates_close(verification),
      "the n=14,k=7 maximal preflight closes every combinatorial cap without starting geometry");

  const ExactPersistentReducedGammaOrderHistory coface_maximum_preflight =
      build_exact_persistent_reduced_gamma_order_history(
          maximal_cloud, 6U, zero_budget);
  check(
      coface_maximum_preflight.decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  no_history_preflight_budget_insufficient &&
          coface_maximum_preflight.exhaustive_facet_count == 3003U &&
          coface_maximum_preflight.exhaustive_coface_count == 3432U &&
          coface_maximum_preflight.exhaustive_union_attempt_count == 20592U &&
          coface_maximum_preflight.required_activation_level_capacity ==
              6435U &&
          coface_maximum_preflight.required_total_facet_work_capacity ==
              19327308U &&
          coface_maximum_preflight.required_total_coface_work_capacity ==
              22088352U &&
          coface_maximum_preflight.required_total_union_work_capacity ==
              132530112U &&
          coface_maximum_preflight.required_node_capacity == 3432U &&
          coface_maximum_preflight.required_child_reference_capacity ==
              3431U &&
          coface_maximum_preflight.required_group_root_reference_capacity ==
              3431U &&
          coface_maximum_preflight
                  .required_group_equal_level_coface_capacity == 3432U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_total_coface_work_count == 22088352U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_node_count == 3432U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_child_reference_count == 3431U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_group_root_reference_count == 3431U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_group_equal_level_coface_count ==
              3432U &&
          ExactPersistentReducedGammaOrderHistoryBudget::
                  maximum_supported_diameter_pair_count == 91U &&
          coface_maximum_preflight.counters
                  .diameter_pair_distance_evaluation_count == 0U,
      "the n=14,k=6 preflight reaches the coface, node, child and diameter caps without geometry");
}

void test_fresh_verifier_rejects_every_persistent_layer() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactPersistentReducedGammaOrderHistoryBudget budget =
      tight_budget(3U, 2U);
  const ExactPersistentReducedGammaOrderHistory history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget);

  const auto verify =
      [&cloud, budget](
          const ExactPersistentReducedGammaOrderHistory& candidate) {
        return verify_exact_persistent_reduced_gamma_order_history(
            cloud, 2U, budget, candidate);
      };
  const auto rejects =
      [&verify](
          const ExactPersistentReducedGammaOrderHistory& candidate,
          bool layer_rejected,
          const std::string& message) {
        const auto verification = verify(candidate);
        check(
            layer_rejected && !verification.fresh_replay_certified &&
                !verification.
                    exact_persistent_reduced_gamma_order_history_decision_certified,
            message);
      };

  ExactPersistentReducedGammaOrderHistory bad_budget = history;
  --bad_budget.requested_budget.maximum_group_count;
  const auto bad_budget_verification = verify(bad_budget);
  rejects(
      bad_budget,
      !bad_budget_verification.requested_budget_certified,
      "the verifier rejects a mutated stored budget");

  ExactPersistentReducedGammaOrderHistory bad_input = history;
  ++bad_input.point_count;
  const auto bad_input_verification = verify(bad_input);
  rejects(
      bad_input,
      !bad_input_verification.external_inputs_certified,
      "the verifier rejects mutated stored external inputs");

  ExactPersistentReducedGammaOrderHistory bad_preflight = history;
  ++bad_preflight.required_group_capacity;
  const auto bad_preflight_verification = verify(bad_preflight);
  rejects(
      bad_preflight,
      !bad_preflight_verification.derived_preflight_sizes_certified,
      "the verifier reconstructs every derived preflight capacity");

  ExactPersistentReducedGammaOrderHistory bad_high_cut = history;
  ++bad_high_cut.high_cut_gamma->counters.active_facet_count;
  const auto bad_high_cut_verification = verify(bad_high_cut);
  rejects(
      bad_high_cut,
      !bad_high_cut_verification.high_cut_gamma_certified,
      "the verifier freshly reconstructs the exhaustive high cut");

  ExactPersistentReducedGammaOrderHistory bad_diameter = history;
  bad_diameter.exact_diameter_squared = level(1);
  const auto bad_diameter_verification = verify(bad_diameter);
  rejects(
      bad_diameter,
      !bad_diameter_verification.high_cut_gamma_certified,
      "the verifier reconstructs the exact squared diameter");

  ExactPersistentReducedGammaOrderHistory bad_high_cut_level = history;
  bad_high_cut_level.high_cut_squared_level = level(2);
  const auto bad_high_cut_level_verification = verify(bad_high_cut_level);
  rejects(
      bad_high_cut_level,
      !bad_high_cut_level_verification.high_cut_gamma_certified,
      "the verifier rejects a high cut not equal to twice the diameter");

  ExactPersistentReducedGammaOrderHistory bad_levels = history;
  bad_levels.activation_levels.pop_back();
  const auto bad_levels_verification = verify(bad_levels);
  rejects(
      bad_levels,
      !bad_levels_verification.activation_levels_certified,
      "the verifier reconstructs the complete canonical activation levels");

  ExactPersistentReducedGammaOrderHistory bad_node = history;
  ++bad_node.nodes[0].node_id;
  const auto bad_node_verification = verify(bad_node);
  rejects(
      bad_node,
      !bad_node_verification.nodes_certified,
      "the verifier rejects a mutated persistent node");

  ExactPersistentReducedGammaOrderHistory bad_event = history;
  ++bad_event.group_records[0].group_record_index;
  const auto bad_event_verification = verify(bad_event);
  rejects(
      bad_event,
      !bad_event_verification.group_records_certified,
      "the verifier rejects a mutated deferred history group record");

  const ExactPersistentReducedGammaOrderHistory e5_history =
      build_exact_persistent_reduced_gamma_order_history(
          e5_cloud(), 2U, tight_budget(5U, 2U));
  const auto e5_record_index =
      [&e5_history](
          const ExactLevel& squared_level,
          ExactReducedGammaBatchGroupKind kind) {
        for (std::size_t index = 0U;
             index < e5_history.group_records.size();
             ++index) {
          const auto& record = e5_history.group_records[index];
          if (record.squared_level == squared_level &&
              record.kind == kind) {
            return std::optional<std::size_t>{index};
          }
        }
        return std::optional<std::size_t>{};
      };
  using VerificationField =
      bool ExactPersistentReducedGammaOrderHistoryVerification::*;
  const auto e5_rejects =
      [](const ExactPersistentReducedGammaOrderHistory& candidate,
          VerificationField rejected_field,
          const std::string& message) {
        const CanonicalPointCloud cloud = e5_cloud();
        const auto verification =
            verify_exact_persistent_reduced_gamma_order_history(
                cloud, 2U, tight_budget(5U, 2U), candidate);
        check(
            !(verification.*rejected_field) &&
                !verification.fresh_replay_certified &&
                !verification.
                    exact_persistent_reduced_gamma_order_history_decision_certified,
            message);
      };

  const auto birth_index = e5_record_index(
      level(25, 16), ExactReducedGammaBatchGroupKind::birth);
  check(birth_index.has_value(), "E5 exposes a birth record to falsify");
  if (birth_index.has_value()) {
    ExactPersistentReducedGammaOrderHistory bad_birth = e5_history;
    bad_birth.group_records[*birth_index].resulting_root_node_id = 99U;
    e5_rejects(
        bad_birth,
        &ExactPersistentReducedGammaOrderHistoryVerification::
            group_records_certified,
        "the verifier rejects a birth with the wrong resulting root");
  }

  const auto continuation_index = e5_record_index(
      level(17, 2), ExactReducedGammaBatchGroupKind::continuation);
  check(
      continuation_index.has_value(),
      "E5 exposes a continuation record to falsify");
  if (continuation_index.has_value()) {
    ExactPersistentReducedGammaOrderHistory bad_continuation = e5_history;
    bad_continuation.group_records[*continuation_index].created_node_id = 99U;
    e5_rejects(
        bad_continuation,
        &ExactPersistentReducedGammaOrderHistoryVerification::
            group_records_certified,
        "the verifier rejects a continuation that spuriously creates a node");
  }

  const auto multifusion_index = e5_record_index(
      level(13, 2), ExactReducedGammaBatchGroupKind::multifusion);
  check(
      multifusion_index.has_value(),
      "E5 exposes a multifusion record to falsify");
  if (multifusion_index.has_value()) {
    ExactPersistentReducedGammaOrderHistory bad_multifusion = e5_history;
    bad_multifusion.group_records[*multifusion_index]
        .prior_root_node_ids.pop_back();
    e5_rejects(
        bad_multifusion,
        &ExactPersistentReducedGammaOrderHistoryVerification::
            group_records_certified,
        "the verifier rejects a multifusion missing one frozen child root");

    ExactPersistentReducedGammaOrderHistory bad_equal_cofaces = e5_history;
    bad_equal_cofaces.group_records[*multifusion_index]
        .equal_level_coface_point_ids.clear();
    e5_rejects(
        bad_equal_cofaces,
        &ExactPersistentReducedGammaOrderHistoryVerification::
            group_records_certified,
        "the verifier rejects a compact group missing its equal-level cofaces");
  }

  const auto redundant_index = e5_record_index(
      level(85, 9), ExactReducedGammaBatchGroupKind::continuation);
  check(
      redundant_index.has_value(),
      "E5 exposes a fully redundant continuation to falsify");
  if (redundant_index.has_value()) {
    ExactPersistentReducedGammaOrderHistory bad_empty_delta = e5_history;
    bad_empty_delta.group_records[*redundant_index]
        .coverage_delta->fully_redundant = false;
    e5_rejects(
        bad_empty_delta,
        &ExactPersistentReducedGammaOrderHistoryVerification::
            group_records_certified,
        "the verifier rejects a falsified fully redundant empty delta");
  }

  ExactPersistentReducedGammaOrderHistory bad_new_facet = e5_history;
  bad_new_facet.group_records.front().newly_active_facet_point_ids.clear();
  e5_rejects(
      bad_new_facet,
      &ExactPersistentReducedGammaOrderHistoryVerification::
          group_records_certified,
      "the verifier rejects a compact group missing its facet activation");

  ExactPersistentReducedGammaOrderHistory bad_child = e5_history;
  bad_child.nodes.back().child_node_ids.pop_back();
  e5_rejects(
      bad_child,
      &ExactPersistentReducedGammaOrderHistoryVerification::nodes_certified,
      "the verifier rejects a merge-tree node missing one child");

  ExactPersistentReducedGammaOrderHistory bad_metadata = history;
  ++bad_metadata.batch_metadata[0].group_record_count;
  const auto bad_metadata_verification = verify(bad_metadata);
  rejects(
      bad_metadata,
      !bad_metadata_verification.batch_metadata_certified,
      "the verifier rejects mutated frozen-batch metadata");

  ExactPersistentReducedGammaOrderHistory bad_root = history;
  bad_root.final_active_roots[0].covered_point_ids.pop_back();
  const auto bad_root_verification = verify(bad_root);
  rejects(
      bad_root,
      !bad_root_verification.final_active_roots_certified,
      "the verifier rejects a mutated final active root");

  ExactPersistentReducedGammaOrderHistory bad_fact = history;
  bad_fact.coverage_deltas_accounted_exactly = false;
  const auto bad_fact_verification = verify(bad_fact);
  rejects(
      bad_fact,
      !bad_fact_verification.result_facts_certified,
      "the verifier rejects a mutated global history fact");

  ExactPersistentReducedGammaOrderHistory bad_counter = history;
  ++bad_counter.counters.history_group_record_count;
  const auto bad_counter_verification = verify(bad_counter);
  rejects(
      bad_counter,
      !bad_counter_verification.counters_certified,
      "the verifier rejects a mutated history counter");

  ExactPersistentReducedGammaOrderHistory bad_decision = history;
  bad_decision.decision =
      ExactPersistentReducedGammaOrderHistoryDecision::not_certified;
  const auto bad_decision_verification = verify(bad_decision);
  rejects(
      bad_decision,
      !bad_decision_verification.decision_certified,
      "the verifier rejects a mutated history decision");

  ExactPersistentReducedGammaOrderHistory bad_scope = history;
  bad_scope.scope = ExactPersistentReducedGammaOrderHistoryScope::unspecified;
  const auto bad_scope_verification = verify(bad_scope);
  rejects(
      bad_scope,
      !bad_scope_verification.scope_certified,
      "the verifier rejects a mutated bounded history scope");

  ExactPersistentReducedGammaOrderHistoryBudget other_budget = budget;
  ++other_budget.maximum_group_count;
  const auto external_budget_verification =
      verify_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, other_budget, history);
  check(
      !external_budget_verification.requested_budget_certified &&
          !external_budget_verification.fresh_replay_certified &&
          !external_budget_verification.
              exact_persistent_reduced_gamma_order_history_decision_certified,
      "the verifier never trusts a different external budget policy");

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-3.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  const auto twin_verification =
      verify_exact_persistent_reduced_gamma_order_history(
          twin_cloud, 2U, budget, history);
  check(
      !twin_verification.high_cut_gamma_certified &&
          !twin_verification.fresh_replay_certified &&
          !twin_verification.
              exact_persistent_reduced_gamma_order_history_decision_certified,
      "a same-size twin cloud cannot reuse another cloud's certified history");
}

}  // namespace

int main() {
  test_default_terminal_and_order_one_rejection();
  test_triangle_deferred_facets_then_one_birth();
  test_two_simultaneous_births_use_one_frozen_snapshot();
  test_one_equal_level_group_preserves_a_ternary_multifusion();
  test_fully_redundant_continuation_keeps_its_event_and_id();
  test_silent_incidence_persists_and_controls_the_later_continuation();
  test_e5_complete_genealogy_and_counters();
  test_atomic_preflight_budgets_and_n14_caps();
  test_fresh_verifier_rejects_every_persistent_layer();
  if (failures != 0) {
    std::cerr << failures
              << " reduced Gamma persistent-history test(s) failed\n";
    return 1;
  }
  std::cout << "all reduced Gamma persistent-history tests passed\n";
  return 0;
}
