#include "morsehgp3d/hierarchy/size_condensed_hierarchy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
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
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistory;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactSizeCondensedComponentDecision;
using morsehgp3d::hierarchy::ExactSizeCondensedComponentRecord;
using morsehgp3d::hierarchy::ExactSizeCondensedHierarchy;
using morsehgp3d::hierarchy::ExactSizeCondensedHierarchyCounters;
using morsehgp3d::hierarchy::ExactSizeCondensedHierarchyDecision;
using morsehgp3d::hierarchy::ExactSizeCondensedSourceKind;
using morsehgp3d::hierarchy::ExactSizeCondensedSourceTransitionKind;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1EmstResult;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_persistent_reduced_gamma_order_history;
using morsehgp3d::hierarchy::build_exact_size_condensed_k1;
using morsehgp3d::hierarchy::build_exact_size_condensed_reduced_gamma;
using morsehgp3d::hierarchy::verify_exact_size_condensed_k1;
using morsehgp3d::hierarchy::verify_exact_size_condensed_reduced_gamma;
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
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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

[[nodiscard]] const ExactSizeCondensedComponentRecord* find_record(
    const ExactSizeCondensedHierarchy& result,
    ExactSizeCondensedSourceTransitionKind source_kind,
    const ExactLevel& squared_level) {
  const auto found = std::find_if(
      result.decision_records.begin(),
      result.decision_records.end(),
      [&](const ExactSizeCondensedComponentRecord& record) {
        return record.source_transition_kind == source_kind &&
               record.squared_level == squared_level;
      });
  return found == result.decision_records.end() ? nullptr : &*found;
}

void check_common_certified_facts(
    const ExactSizeCondensedHierarchy& result,
    const std::string& label,
    ExactSizeCondensedHierarchyDecision expected_decision =
        ExactSizeCondensedHierarchyDecision::
            complete_size_condensed_hierarchy) {
  check(
      result.size_condensed_hierarchy_certified &&
          result.distinct_point_coverage_certified &&
          result.at_least_threshold_relation_certified &&
          result.all_source_batches_decided_atomically &&
          result.hidden_growth_preserved &&
          result.threshold_entries_are_not_morse_births &&
          result.every_source_transition_accounted_exactly_once &&
          result.condensed_genealogy_certified &&
          result.horizontal_single_order_view_certified &&
          !result.vertical_maps_projected &&
          !result.full_pi0_equivalence_claimed &&
          !result.morsehgp3d_result_claimed &&
          !result.public_status_claimed &&
          result.output_digest_certified &&
          result.decision == expected_decision &&
          result.provenance.source_complete_exact_history_certified &&
          result.provenance.source_fresh_replay_certified &&
          !result.provenance.surrogate_v5_source_used,
      label + ": the result is an exact horizontal view with no public or full-pi0 claim");
  check(
      std::all_of(
          result.batch_metadata.begin(),
          result.batch_metadata.end(),
          [](const auto& batch) {
            return batch.source_batch_staged_completely_before_decisions &&
                   batch.source_batch_mutated_only_after_all_decisions;
          }) &&
          std::all_of(
              result.decision_records.begin(),
              result.decision_records.end(),
              [](const auto& record) {
                return record.decided_after_complete_source_batch;
              }),
      label + ": every decision follows complete exact-batch staging");
}

[[nodiscard]] CanonicalPointCloud separated_pairs_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(10.0), point(11.0)};
  return canonical_cloud(input);
}

void test_k1_exact_emst_condensation_and_equal_batch_permutation() {
  const CanonicalPointCloud cloud = separated_pairs_cloud();
  const K1EmstResult exact_emst = build_exact_complete_graph_emst(cloud);
  const K1CompactForest source = build_compact_k1_forest(exact_emst);
  const ExactSizeCondensedHierarchy visible =
      build_exact_size_condensed_k1(cloud, source, 2U);

  check_common_certified_facts(visible, "k1 visible-pairs condensation");
  check(
      visible.provenance.source_kind ==
              ExactSizeCondensedSourceKind::exact_k1_compact_forest &&
          visible.provenance.source_emst_minimality_reverified_by_condensation &&
          !visible.provenance.
              source_emst_certification_inherited_from_input_contract &&
          visible.provenance.source_counters.k1_compact_forest_counters ==
              std::optional{source.counters} &&
          !visible.provenance.source_counters.
              reduced_gamma_history_counters.has_value(),
      "k1 provenance binds the freshly rebuilt exact EMST and complete source counters");
  check(
      visible.decision_records.size() == 7U &&
          visible.counters.threshold_entry_count == 2U &&
          visible.counters.multifusion_count == 1U &&
          visible.counters.hidden_decision_count == 4U &&
          visible.counters.final_visible_root_count == 1U,
      "four hidden leaves form two visible size-two components before one binary visible multifusion");
  const ExactSizeCondensedComponentRecord* merge = find_record(
      visible,
      ExactSizeCondensedSourceTransitionKind::k1_equal_level_merge,
      level(81, 4));
  check(
      merge != nullptr &&
          merge->decision ==
              ExactSizeCondensedComponentDecision::multifusion &&
          merge->q_visible == 2U && merge->distinct_coverage_size == 4U &&
          merge->created_visible_component_id.has_value(),
      "the exact EMST bridge is one visible multifusion, not a binaryized surrogate tree");
  const auto verify_visible = [&](const ExactSizeCondensedHierarchy& observed) {
    return verify_exact_size_condensed_k1(cloud, source, 2U, observed);
  };
  const auto verification = verify_visible(visible);
  check(
      verification.exact_size_condensed_hierarchy_certified,
      "the k1 condensed view passes a fresh exact EMST and condensation replay");

  ExactSizeCondensedHierarchy bad_digest = visible;
  bad_digest.condensed_projection_digest = {};
  const auto bad_digest_verification = verify_visible(bad_digest);
  check(
      !bad_digest_verification.digests_certified &&
          !bad_digest_verification.
              exact_size_condensed_hierarchy_certified,
      "the fresh verifier rejects a mutated condensed digest");

  ExactSizeCondensedHierarchy stale_payload = visible;
  stale_payload.decision_records.front().distinct_coverage_size += 1U;
  const auto stale_payload_verification = verify_visible(stale_payload);
  check(
      stale_payload.condensed_projection_digest ==
              visible.condensed_projection_digest &&
          !stale_payload_verification.digests_certified &&
          !stale_payload_verification.
              exact_size_condensed_hierarchy_certified,
      "the digest certificate recomputes the observed payload and rejects stale stored digest bytes");

  ExactSizeCondensedHierarchy mutated_genealogy = visible;
  auto genealogy_record = std::find_if(
      mutated_genealogy.decision_records.begin(),
      mutated_genealogy.decision_records.end(),
      [](const ExactSizeCondensedComponentRecord& record) {
        return record.decision ==
                   ExactSizeCondensedComponentDecision::multifusion &&
               !record.prior_visible_component_ids.empty();
      });
  const bool genealogy_record_found =
      genealogy_record != mutated_genealogy.decision_records.end();
  if (genealogy_record_found) {
    genealogy_record->prior_visible_component_ids.front() += 17U;
  }
  const auto mutated_genealogy_verification =
      verify_visible(mutated_genealogy);
  check(
      genealogy_record_found &&
          !mutated_genealogy_verification.genealogy_certified &&
          !mutated_genealogy_verification.digests_certified &&
          !mutated_genealogy_verification.
              exact_size_condensed_hierarchy_certified,
      "the independent genealogy certificate rejects an internal ancestry mutation even when final visible ids stay unchanged");

  ExactSizeCondensedHierarchy mutated_batch_genealogy = visible;
  mutated_batch_genealogy.batch_metadata.back().visible_root_count_after += 1U;
  const auto mutated_batch_genealogy_verification =
      verify_visible(mutated_batch_genealogy);
  check(
      !mutated_batch_genealogy_verification.genealogy_certified &&
          !mutated_batch_genealogy_verification.batches_certified &&
          !mutated_batch_genealogy_verification.
              exact_size_condensed_hierarchy_certified,
      "the independent genealogy certificate also binds atomic batch root counts");

  ExactSizeCondensedHierarchy mutated_attestation = visible;
  mutated_attestation.hidden_growth_preserved = false;
  const auto mutated_attestation_verification =
      verify_visible(mutated_attestation);
  check(
      mutated_attestation_verification.genealogy_certified &&
          !mutated_attestation_verification.digests_certified &&
          !mutated_attestation_verification.result_facts_certified &&
          !mutated_attestation_verification.
              exact_size_condensed_hierarchy_certified,
      "the signed semantic payload rejects a top-level attestation mutation without conflating it with genealogy");

  std::vector<ExactEmstEdge> permuted_edges = exact_emst.emst_edges;
  std::reverse(permuted_edges.begin(), permuted_edges.end());
  for (ExactEmstEdge& edge : permuted_edges) {
    std::swap(edge.u, edge.v);
  }
  const K1CompactForest permuted_source =
      build_compact_k1_forest(cloud.size(), permuted_edges);
  const ExactSizeCondensedHierarchy permuted =
      build_exact_size_condensed_k1(cloud, permuted_source, 2U);
  check(
      permuted == visible,
      "permuting and reorienting all inputs of one equal-level EMST batch leaves the certified condensation byte-identical");

  const ExactSizeCondensedHierarchy crossing =
      build_exact_size_condensed_k1(cloud, source, 4U);
  check(
      crossing.counters.hidden_decision_count == 6U &&
          crossing.counters.threshold_entry_count == 1U &&
          crossing.counters.multifusion_count == 0U &&
          crossing.decision_records.back().decision ==
              ExactSizeCondensedComponentDecision::threshold_entry &&
          crossing.decision_records.back().q_visible == 0U &&
          crossing.decision_records.back().distinct_coverage_size == 4U,
      "two hidden k1 components crossing the threshold together create a threshold entry, never a false birth");

  const ExactSizeCondensedHierarchy singleton_visible =
      build_exact_size_condensed_k1(cloud, source, 1U);
  check(
      singleton_visible.batch_metadata.front().source_batch_index ==
              std::nullopt &&
          singleton_visible.batch_metadata.front().squared_level == level(0) &&
          singleton_visible.batch_metadata.front().threshold_entry_count == 4U &&
          singleton_visible.decision_records.front().source_transition_kind ==
              ExactSizeCondensedSourceTransitionKind::k1_initial_leaf &&
          singleton_visible.decision_records.front().decision ==
              ExactSizeCondensedComponentDecision::threshold_entry,
      "min_cluster_size=1 exposes all K1 leaves through one complete atomic level-zero batch");

  std::vector<ExactEmstEdge> nonminimal_edges{
      ExactEmstEdge{0U, 2U, level(100), level(25)},
      ExactEmstEdge{1U, 2U, level(81), level(81, 4)},
      ExactEmstEdge{2U, 3U, level(1), level(1, 4)}};
  const K1CompactForest nonminimal =
      build_compact_k1_forest(cloud.size(), nonminimal_edges);
  check_throws<std::invalid_argument>(
      [&cloud, &nonminimal]() {
        static_cast<void>(build_exact_size_condensed_k1(cloud, nonminimal, 2U));
      },
      "a valid spanning-tree CSR that is not the exact EMST is rejected before condensation");
}

void test_k1_exact_crossing_at_default_twenty() {
  std::vector<CertifiedPoint3> points;
  points.reserve(20U);
  for (std::size_t point_index = 0U; point_index < 20U; ++point_index) {
    points.push_back(point(static_cast<double>(point_index)));
  }
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  const K1CompactForest source = build_compact_k1_forest(
      build_exact_complete_graph_emst(cloud));
  const ExactSizeCondensedHierarchy result =
      build_exact_size_condensed_k1(cloud, source);
  const ExactSizeCondensedComponentRecord* entry = find_record(
      result,
      ExactSizeCondensedSourceTransitionKind::k1_equal_level_merge,
      level(1, 4));
  check(
      result.min_cluster_size == 20U && entry != nullptr &&
          entry->distinct_coverage_size == 20U && entry->q_visible == 0U &&
          entry->decision ==
              ExactSizeCondensedComponentDecision::threshold_entry &&
          result.counters.hidden_decision_count == 20U &&
          result.counters.threshold_entry_count == 1U &&
          result.counters.final_visible_root_count == 1U,
      "twenty exact collinear points cross the default at_least threshold in one complete equal-level EMST batch");
}

[[nodiscard]] CanonicalPointCloud two_triangles_cloud() {
  const std::array<CertifiedPoint3, 6> input{
      point(-2.0, 0.0),
      point(0.0, 2.0),
      point(2.0, 0.0),
      point(18.0, 0.0),
      point(20.0, 2.0),
      point(22.0, 0.0)};
  return canonical_cloud(input);
}

void test_gamma_hidden_fusion_crossing_and_visible_multifusion() {
  const CanonicalPointCloud cloud = two_triangles_cloud();
  const auto budget = tight_budget(cloud.size(), 2U);
  const ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(cloud, 2U, budget);

  const ExactSizeCondensedHierarchy hidden_crossing =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, budget, source, 4U);
  check_common_certified_facts(
      hidden_crossing, "Gamma hidden-fusion condensation");
  check(
      hidden_crossing.provenance.source_kind ==
              ExactSizeCondensedSourceKind::
                  exact_persistent_reduced_gamma_order_history &&
          hidden_crossing.provenance.source_counters.
                  reduced_gamma_history_counters ==
              std::optional{source.counters} &&
          !hidden_crossing.provenance.source_counters.
              k1_compact_forest_counters.has_value() &&
          !hidden_crossing.provenance.
              source_emst_minimality_reverified_by_condensation &&
          !hidden_crossing.provenance.
              source_emst_certification_inherited_from_input_contract,
      "Gamma provenance binds the freshly replayed bounded hgp_reduced history and its source counters");
  const ExactSizeCondensedComponentRecord* hidden_merge = find_record(
      hidden_crossing,
      ExactSizeCondensedSourceTransitionKind::gamma_multifusion,
      level(82));
  check(
      hidden_merge != nullptr &&
          hidden_merge->decision ==
              ExactSizeCondensedComponentDecision::threshold_entry &&
          hidden_merge->q_visible == 0U &&
          hidden_merge->distinct_coverage_size == 6U &&
          hidden_merge->source_prior_root_node_ids.size() == 2U,
      "a source multifusion of two hidden Gamma components crossing size four is a threshold entry, not a Morse birth");

  const ExactSizeCondensedHierarchy visible =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, budget, source, 2U);
  const ExactSizeCondensedComponentRecord* visible_merge = find_record(
      visible,
      ExactSizeCondensedSourceTransitionKind::gamma_multifusion,
      level(82));
  check(
      visible_merge != nullptr &&
          visible_merge->decision ==
              ExactSizeCondensedComponentDecision::multifusion &&
          visible_merge->q_visible == 2U &&
          visible_merge->prior_visible_component_ids.size() == 2U &&
          visible.counters.threshold_entry_count == 2U &&
          visible.counters.multifusion_count == 1U,
      "components visible at equality size three retain the later exact Gamma multifusion");
  check(
      visible.min_cluster_size == visible.provenance.source_counters.order &&
          !visible.full_pi0_equivalence_claimed &&
          !visible.vertical_maps_projected,
      "min_cluster_size<=order remains only a horizontal hgp_reduced view and never implies full-pi0 equivalence");
  const auto verification = verify_exact_size_condensed_reduced_gamma(
      cloud, 2U, budget, source, 2U, visible);
  check(
      verification.exact_size_condensed_hierarchy_certified,
      "the Gamma condensed view passes fresh source and condensation replays");

  ExactPersistentReducedGammaOrderHistory uncertified = source;
  uncertified.persistent_reduced_gamma_history_certified = false;
  check_throws<std::invalid_argument>(
      [&cloud, &budget, &uncertified]() {
        static_cast<void>(build_exact_size_condensed_reduced_gamma(
            cloud, 2U, budget, uncertified, 3U));
      },
      "the Gamma adapter rejects a source whose stored certification was falsified instead of trusting it");

  const ExactSizeCondensedHierarchy default_twenty =
      build_exact_size_condensed_reduced_gamma(cloud, 2U, budget, source);
  check(
      default_twenty.min_cluster_size == 20U &&
          default_twenty.public_component_record_indices.empty() &&
          default_twenty.final_visible_component_ids.empty(),
      "the requested default min_cluster_size=20 hides every component of a six-point oracle without changing its source replay");
}

void test_gamma_terminal_empty_condensation() {
  const std::array<CertifiedPoint3, 2> input{
      point(0.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactPersistentReducedGammaOrderHistoryBudget zero_budget;
  const ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, zero_budget);
  const ExactSizeCondensedHierarchy result =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, zero_budget, source);

  check_common_certified_facts(
      result,
      "Gamma terminal empty condensation",
      ExactSizeCondensedHierarchyDecision::complete_empty_terminal_order);
  check(
      result.min_cluster_size == 20U && result.decision_records.empty() &&
          result.public_component_record_indices.empty() &&
          result.batch_metadata.empty() &&
          result.final_visible_component_ids.empty() &&
          result.counters == ExactSizeCondensedHierarchyCounters{} &&
          result.provenance.source_counters.source_facet_count == 1U &&
          result.provenance.source_counters.source_coface_count == 0U,
      "the terminal k=n source projects to an explicitly certified empty horizontal condensation");
  const auto verification = verify_exact_size_condensed_reduced_gamma(
      cloud, 2U, zero_budget, source, 20U, result);
  check(
      verification.exact_size_condensed_hierarchy_certified,
      "the terminal empty condensation passes fresh source, digest and result replay");
}

[[nodiscard]] CanonicalPointCloud silent_growth_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  return canonical_cloud(input);
}

void test_gamma_silent_growth_before_threshold_entry() {
  const CanonicalPointCloud cloud = silent_growth_cloud();
  const auto budget = tight_budget(cloud.size(), 2U);
  const ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(cloud, 2U, budget);
  const ExactSizeCondensedHierarchy result =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, budget, source, 5U);

  const ExactSizeCondensedComponentRecord* silent = find_record(
      result,
      ExactSizeCondensedSourceTransitionKind::gamma_continuation,
      level(33, 2));
  const ExactSizeCondensedComponentRecord* entry = find_record(
      result,
      ExactSizeCondensedSourceTransitionKind::gamma_continuation,
      level(83886, 3563));
  check(
      silent != nullptr &&
          silent->decision == ExactSizeCondensedComponentDecision::hidden &&
          silent->q_visible == 0U &&
          silent->distinct_coverage_size == 4U &&
          entry != nullptr &&
          entry->decision ==
              ExactSizeCondensedComponentDecision::threshold_entry &&
          entry->q_visible == 0U && entry->distinct_coverage_size == 5U,
      "a hidden component keeps its silent incidence and enters visibility only when a later exact continuation reaches size five");
  check(
      result.counters.final_visible_root_count == 1U &&
          result.threshold_entries_are_not_morse_births,
      "silent growth produces one visible root without relabelling its late entry as a Morse birth");
}

[[nodiscard]] CanonicalPointCloud overlapping_roots_cloud() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0),
      point(0.25, 2.0),
      point(2.0, -1.0),
      point(2.0, 3.0),
      point(3.75, 2.0),
      point(4.0, 0.0)};
  return canonical_cloud(input);
}

void test_gamma_distinct_coverage_under_overlaps_and_equality() {
  const CanonicalPointCloud cloud = overlapping_roots_cloud();
  const auto budget = tight_budget(cloud.size(), 2U);
  const ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(cloud, 2U, budget);
  const ExactSizeCondensedHierarchy hidden =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, budget, source, 7U);
  const ExactSizeCondensedComponentRecord* hidden_merge = find_record(
      hidden,
      ExactSizeCondensedSourceTransitionKind::gamma_multifusion,
      level(13, 4));
  check(
      hidden_merge != nullptr &&
          hidden_merge->source_prior_root_node_ids.size() == 3U &&
          hidden_merge->summed_prior_distinct_coverage_size >
              hidden_merge->distinct_prior_coverage_union_size &&
          hidden_merge->distinct_coverage_size == 6U &&
          hidden_merge->decision ==
              ExactSizeCondensedComponentDecision::hidden,
      "overlapping Gamma components use the cardinality of the distinct point union, never the sum of simplex coverages");

  const ExactSizeCondensedHierarchy equality =
      build_exact_size_condensed_reduced_gamma(
          cloud, 2U, budget, source, 6U);
  const ExactSizeCondensedComponentRecord* equality_merge = find_record(
      equality,
      ExactSizeCondensedSourceTransitionKind::gamma_multifusion,
      level(13, 4));
  check(
      equality_merge != nullptr &&
          equality_merge->distinct_coverage_size == 6U &&
          equality_merge->decision ==
              ExactSizeCondensedComponentDecision::threshold_entry &&
          equality_merge->q_visible == 0U,
      "coverage equal to min_cluster_size is visible under the published at_least relation");
}

}  // namespace

int main() {
  test_k1_exact_emst_condensation_and_equal_batch_permutation();
  test_k1_exact_crossing_at_default_twenty();
  test_gamma_hidden_fusion_crossing_and_visible_multifusion();
  test_gamma_terminal_empty_condensation();
  test_gamma_silent_growth_before_threshold_entry();
  test_gamma_distinct_coverage_under_overlaps_and_equality();
  if (failures != 0) {
    std::cerr << failures << " size-condensed hierarchy test(s) failed\n";
    return 1;
  }
  std::cout << "all size-condensed hierarchy tests passed\n";
  return 0;
}
