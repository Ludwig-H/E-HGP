#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "morsehgp3d/gpu/k1_boruvka.hpp"
#include "morsehgp3d/hierarchy/boruvka.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaComponentDualTreeSearchStatus;
using morsehgp3d::gpu::K1BoruvkaComponentEnvelopeMode;
using morsehgp3d::gpu::K1BoruvkaExactSearchStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::gpu::K1DualTreeExactBoruvkaResult;
using morsehgp3d::gpu::K1DualTreeExactBoruvkaVerification;
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::K1SeededExactBoruvkaResult;
using morsehgp3d::gpu::K1SeededExactBoruvkaVerification;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_k1_boruvka;
using morsehgp3d::gpu::verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka;
using morsehgp3d::gpu::verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

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

[[nodiscard]] CertifiedPoint3 point(double x) {
  return CertifiedPoint3::from_binary64(x, 0.0, 0.0);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::array<CertifiedPoint3, 8> chain_points() {
  return {
      point(0.0),
      point(1.0),
      point(10.0),
      point(12.0),
      point(100.0),
      point(104.0),
      point(120.0),
      point(125.0)};
}

void test_singleton_is_vacuously_certified() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 1> input{point(7.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult result =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);

  check(
      result.point_count == 1U && result.rounds.empty() &&
          result.emst_edges.empty() &&
          result.total_squared_weight == ExactLevel{} &&
          result.total_hgp_weight == ExactLevel{},
      "singleton seeded chain has no round, edge or weight");
  check(
      result.counters.round_count == 0U &&
          result.counters.final_component_count == 1U &&
          result.counters.morton_gpu_kernel_launch_count == 0U &&
          result.counters.exact_node_visit_count == 0U,
      "singleton seeded chain has no proposal or search work");
  check(
      result.bounded_morton_seed_chain_certified &&
          result.exact_external_1nn_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.fresh_replay_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "singleton seeded chain closes every certificate vacuously");
  check(
      result.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          result.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only &&
          fake_gpu_k1_boruvka_launch_count() == 0U,
      "singleton stays local and launches no GPU proposal");

  const K1SeededExactBoruvkaVerification verification =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, result);
  check(
      verification.emst_witness_certified &&
          verification.replayed_round_count == 0U &&
          fake_gpu_k1_boruvka_launch_count() == 0U,
      "singleton explicit replay remains launch-free");
}

[[nodiscard]] K1SeededExactBoruvkaResult build_checked_chain(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& reference) {
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  reset_fake_gpu_k1_boruvka();
  K1SeededExactBoruvkaResult result =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  check(
      result.rounds.size() == 3U &&
          result.rounds.size() == reference.rounds.size() &&
          result.emst_edges == reference.emst_edges &&
          result.total_squared_weight == reference.total_squared_weight &&
          result.total_hgp_weight == reference.total_hgp_weight,
      "three-round seeded chain reproduces the CPU EMST and exact weights");
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const auto& observed = result.rounds[round_index];
    const auto& expected = reference.rounds[round_index];
    check(
        observed.seed_status ==
                K1BoruvkaSeedStatus::
                    bounded_morton_window_external_exact_monotone_certified &&
            observed.search_status ==
                K1BoruvkaExactSearchStatus::
                    exact_external_1nn_branch_and_bound_certified &&
            observed.decision_status ==
                K1HybridBoruvkaDecisionStatus::
                    cpu_exact_kappa_minima_certified &&
            observed.contraction_status ==
                K1HybridBoruvkaContractionStatus::
                    cpu_exact_canonical_contraction_certified,
        "each seeded round separates proposal, search, decision and contraction");
    check(
        observed.exact_decision.component_minima ==
                expected.component_minima &&
            observed.canonical_contraction.accepted_edges ==
                expected.accepted_edges &&
            observed.canonical_contraction.post_round_component_count ==
                expected.post_round_component_count,
        "each seeded round matches the independent CPU decision and contraction");
  }
  check(
      result.counters.round_count == 3U &&
          result.counters.component_minimum_count == 14U &&
          result.counters.accepted_edge_count == 7U &&
          result.counters.component_contraction_count == 7U &&
          result.counters.morton_seed_source_count == 24U &&
          result.counters.exact_point_query_count == 24U &&
          result.counters.exact_point_minimum_count == 24U &&
          result.counters.morton_gpu_kernel_launch_count == 3U &&
          result.counters.final_component_count == 1U,
      "seeded chain counters close 8-to-4-to-2-to-1");
  check(
      fake_gpu_k1_boruvka_launch_count() == 6U &&
          result.bounded_morton_seed_chain_certified &&
          result.exact_external_1nn_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.fresh_replay_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "builder performs one producer and one fresh replay launch per round");
  check(
      std::string_view{K1SeededExactBoruvkaResult::proof_basis} ==
          "gpu_bounded_morton_seed_cpu_exact_external_1nn_boruvka_v1",
      "seeded chain exposes its dedicated proof basis");
  return result;
}

void test_three_round_chain_and_falsifications() {
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult reference =
      build_exact_lbvh_boruvka(index, cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult result =
      build_checked_chain(index, cloud, reference);

  std::size_t launches = fake_gpu_k1_boruvka_launch_count();
  const K1SeededExactBoruvkaVerification explicit_replay =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, result);
  check(
      explicit_replay.emst_witness_certified &&
          explicit_replay.replayed_round_count == 3U &&
          explicit_replay.replayed_component_minimum_count == 14U &&
          explicit_replay.replayed_seed_kernel_launch_count == 3U &&
          explicit_replay.replayed_exact_node_visit_count ==
              explicit_replay.replayed_exact_aabb_bound_evaluation_count &&
          fake_gpu_k1_boruvka_launch_count() == launches + 3U,
      "explicit verifier reruns the complete seeded exact chain");

  launches = fake_gpu_k1_boruvka_launch_count();
  const K1SeededExactBoruvkaVerification wrong_trusted_policy =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, K1BoruvkaMortonSeedPolicy{2U}, result);
  check(
      !wrong_trusted_policy.trusted_seed_policy_certified &&
          !wrong_trusted_policy.emst_witness_certified &&
          fake_gpu_k1_boruvka_launch_count() == launches,
      "mismatched trusted policy fails before fresh GPU replay");
  K1SeededExactBoruvkaResult altered_policy = result;
  altered_policy.morton_seed_policy = K1BoruvkaMortonSeedPolicy{2U};
  const K1SeededExactBoruvkaVerification altered_policy_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, altered_policy);
  check(
      !altered_policy_check.trusted_seed_policy_certified &&
          !altered_policy_check.emst_witness_certified &&
          fake_gpu_k1_boruvka_launch_count() == launches,
      "untrusted embedded policy fails before fresh GPU replay");

  K1SeededExactBoruvkaResult bad_seed = result;
  bad_seed.rounds[0].morton_seed_audit.bounded_window_certified = false;
  const auto seed_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, bad_seed);
  check(
      !seed_check.bounded_morton_seed_chain_certified &&
          seed_check.exact_external_1nn_chain_certified &&
          seed_check.cpu_exact_decision_chain_certified &&
          seed_check.canonical_contractions_certified &&
          !seed_check.emst_witness_certified,
      "seed-audit falsification invalidates only the proposal layer");

  K1SeededExactBoruvkaResult bad_search = result;
  bad_search.rounds[0].exact_search_audit.
      complete_frontier_exhaustion_certified = false;
  const auto search_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, bad_search);
  check(
      search_check.bounded_morton_seed_chain_certified &&
          !search_check.exact_external_1nn_chain_certified &&
          search_check.cpu_exact_decision_chain_certified &&
          search_check.canonical_contractions_certified &&
          !search_check.emst_witness_certified,
      "search-audit falsification invalidates only exact frontier proof");

  K1SeededExactBoruvkaResult bad_decision = result;
  bad_decision.rounds[0].decision_status =
      K1HybridBoruvkaDecisionStatus::not_certified;
  const auto decision_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, bad_decision);
  check(
      decision_check.bounded_morton_seed_chain_certified &&
          decision_check.exact_external_1nn_chain_certified &&
          !decision_check.cpu_exact_decision_chain_certified &&
          decision_check.canonical_contractions_certified &&
          !decision_check.emst_witness_certified,
      "decision falsification preserves proposal and search certificates");

  K1SeededExactBoruvkaResult bad_contraction = result;
  bad_contraction.rounds[0].contraction_status =
      K1HybridBoruvkaContractionStatus::not_certified;
  const auto contraction_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, bad_contraction);
  check(
      contraction_check.bounded_morton_seed_chain_certified &&
          contraction_check.exact_external_1nn_chain_certified &&
          contraction_check.cpu_exact_decision_chain_certified &&
          !contraction_check.canonical_contractions_certified &&
          !contraction_check.emst_witness_certified,
      "contraction falsification preserves every upstream certificate");

  K1SeededExactBoruvkaResult bad_counters = result;
  ++bad_counters.counters.exact_node_visit_count;
  const auto counter_check =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy, bad_counters);
  check(
      counter_check.bounded_morton_seed_chain_certified &&
          counter_check.exact_external_1nn_chain_certified &&
          counter_check.cpu_exact_decision_chain_certified &&
          counter_check.canonical_contractions_certified &&
          !counter_check.counters_certified &&
          !counter_check.emst_witness_certified,
      "aggregate counter falsification leaves mathematical layers intact");
}

void test_invalid_policy_fails_before_gpu() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 2> input{point(0.0), point(1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  check_throws<std::invalid_argument>(
      [&index, &cloud]() {
        static_cast<void>(
            build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
                index, cloud, K1BoruvkaMortonSeedPolicy{0U}));
      },
      "seeded exact chain rejects a zero trusted Morton radius");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "invalid seeded policy fails before GPU proposal");
}

void test_dual_tree_singleton_is_vacuously_certified() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 1> input{point(7.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1DualTreeExactBoruvkaResult result =
      build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy);

  check(
      result.rounds.empty() && result.emst_edges.empty() &&
          result.total_squared_weight == ExactLevel{} &&
          result.total_hgp_weight == ExactLevel{} &&
          result.bounded_morton_seed_chain_certified &&
          result.exact_dual_tree_chain_certified &&
          result.component_minima_only_persistence_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.fresh_replay_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "singleton shared chain closes every certificate vacuously");
  check(
      result.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          result.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only &&
          fake_gpu_k1_boruvka_launch_count() == 0U,
      "singleton shared chain stays local and launch-free");

  const K1DualTreeExactBoruvkaVerification verification =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, result);
  check(
      verification.emst_witness_certified &&
          verification.replayed_round_count == 0U &&
          fake_gpu_k1_boruvka_launch_count() == 0U,
      "singleton explicit shared replay remains launch-free");
}

void test_dual_tree_three_round_chain_and_falsifications() {
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult reference =
      build_exact_lbvh_boruvka(index, cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  reset_fake_gpu_k1_boruvka();
  const K1DualTreeExactBoruvkaResult result =
      build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy);

  check(
      result.rounds.size() == 3U &&
          result.rounds.size() == reference.rounds.size() &&
          result.emst_edges == reference.emst_edges &&
          result.total_squared_weight == reference.total_squared_weight &&
          result.total_hgp_weight == reference.total_hgp_weight &&
          fake_gpu_k1_boruvka_launch_count() == 6U,
      "shared chain reproduces the 8-to-4-to-2-to-1 CPU EMST");
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const auto& observed = result.rounds[round_index];
    const auto& expected = reference.rounds[round_index];
    check(
        observed.seed_status ==
                K1BoruvkaSeedStatus::
                    bounded_morton_window_external_exact_monotone_certified &&
            observed.search_status ==
                K1BoruvkaComponentDualTreeSearchStatus::
                    exact_component_minima_shared_lbvh_dual_tree_certified &&
            observed.decision_status ==
                K1HybridBoruvkaDecisionStatus::
                    cpu_exact_kappa_minima_certified &&
            observed.contraction_status ==
                K1HybridBoruvkaContractionStatus::
                    cpu_exact_canonical_contraction_certified,
        "each shared round separates seed, search, decision and contraction");
    check(
        observed.exact_decision.component_minima ==
                expected.component_minima &&
            observed.canonical_contraction.accepted_edges ==
                expected.accepted_edges &&
            observed.canonical_contraction.post_round_component_count ==
                expected.post_round_component_count,
        "each shared round matches the independent CPU anchor");
    const auto& audit = observed.dual_tree_search_audit;
    check(
        audit.component_envelope_mode ==
                K1BoruvkaComponentEnvelopeMode::frozen_initial &&
            audit.cpu_component_witness_leaf_update_count == 0U &&
            audit.cpu_component_witness_ancestor_update_count == 0U &&
            audit.live_component_cutoff_upper_bound_certified &&
            audit.pointwise_at_most_frozen_envelope_certified &&
            audit.covered_unordered_point_pair_count == 28U &&
            audit.unordered_point_pair_count == 28U &&
            audit.lbvh_maximum_depth ==
                index.build_counters().maximum_depth &&
            audit.certified_depth_first_frontier_bound ==
                2U * index.build_counters().maximum_depth + 1U &&
            audit.certified_node_pair_visit_bound == 71U &&
            audit.maximum_cpu_frontier_size <=
                audit.certified_depth_first_frontier_bound &&
            audit.cpu_node_pair_visit_count <=
                audit.certified_node_pair_visit_bound &&
            audit.cpu_exact_aabb_pair_bound_evaluation_count ==
                audit.cpu_node_pair_visit_count &&
            audit.point_seed_count == 8U &&
            audit.component_seed_incumbent_count ==
                expected.pre_round_component_count &&
            audit.target_component_seed_offer_count == 8U &&
            audit.target_component_seed_kappa_update_count <= 8U &&
            audit.target_component_seed_strict_cutoff_decrease_count <=
                audit.target_component_seed_kappa_update_count &&
            audit.component_cutoff_upper_envelope_node_count ==
                index.build_counters().node_count &&
            audit.component_seed_reduction_certified &&
            audit.bidirectional_component_seed_reduction_certified &&
            audit.component_cutoff_upper_envelope_certified &&
            audit.canonical_unordered_pair_partition_certified &&
            audit.depth_first_frontier_bound_certified &&
            audit.node_pair_visit_bound_certified,
        "each shared round closes the canonical unordered-pair partition");
  }
  check(
      result.bounded_morton_seed_chain_certified &&
          result.exact_dual_tree_chain_certified &&
          result.component_minima_only_persistence_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.fresh_replay_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified &&
          std::string_view{K1DualTreeExactBoruvkaResult::proof_basis} ==
              "gpu_bounded_morton_seed_cpu_exact_bidirectional_component_dual_tree_boruvka_v4",
      "shared builder requires fresh replay and the independent CPU witness");

  std::size_t launches = fake_gpu_k1_boruvka_launch_count();
  const K1DualTreeExactBoruvkaVerification explicit_replay =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, result);
  check(
      explicit_replay.emst_witness_certified &&
          explicit_replay.replayed_round_count == 3U &&
          explicit_replay.replayed_component_minimum_count == 14U &&
          explicit_replay.replayed_seed_kernel_launch_count == 3U &&
          explicit_replay.replayed_node_pair_visit_count ==
              explicit_replay.replayed_aabb_pair_bound_evaluation_count &&
          fake_gpu_k1_boruvka_launch_count() == launches + 3U,
      "explicit verifier rebuilds every shared round in a fresh context");

  launches = fake_gpu_k1_boruvka_launch_count();
  const auto wrong_policy =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, K1BoruvkaMortonSeedPolicy{2U}, result);
  check(
      !wrong_policy.trusted_seed_policy_certified &&
          !wrong_policy.emst_witness_certified &&
          fake_gpu_k1_boruvka_launch_count() == launches,
      "mismatched trusted shared policy fails before GPU replay");

  K1DualTreeExactBoruvkaResult bad_search = result;
  ++bad_search.rounds[0].dual_tree_search_audit.
      covered_unordered_point_pair_count;
  const auto search_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_search);
  check(
      !search_check.exact_dual_tree_chain_certified &&
          search_check.cpu_exact_decision_chain_certified &&
          !search_check.emst_witness_certified,
      "shared coverage falsification invalidates only the traversal proof");

  K1DualTreeExactBoruvkaResult bad_visit_bound = result;
  --bad_visit_bound.rounds[0].dual_tree_search_audit.
      certified_node_pair_visit_bound;
  const auto visit_bound_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_visit_bound);
  check(
      !visit_bound_check.exact_dual_tree_chain_certified &&
          visit_bound_check.cpu_exact_decision_chain_certified &&
          !visit_bound_check.emst_witness_certified,
      "shared visit-bound falsification invalidates the traversal proof");

  K1DualTreeExactBoruvkaResult bad_envelope = result;
  bad_envelope.rounds[0].dual_tree_search_audit.
      component_cutoff_upper_envelope_certified = false;
  const auto envelope_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_envelope);
  check(
      !envelope_check.exact_dual_tree_chain_certified &&
          envelope_check.cpu_exact_decision_chain_certified &&
          !envelope_check.emst_witness_certified,
      "frozen component-envelope falsification invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_envelope_mode = result;
  bad_envelope_mode.rounds[0].dual_tree_search_audit.
      component_envelope_mode =
      K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone;
  const auto envelope_mode_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_envelope_mode);
  check(
      !envelope_mode_check.exact_dual_tree_chain_certified &&
          envelope_mode_check.cpu_exact_decision_chain_certified &&
          !envelope_mode_check.emst_witness_certified,
      "non-frozen persistent envelope mode invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_live_envelope = result;
  bad_live_envelope.rounds[0].dual_tree_search_audit.
      live_component_cutoff_upper_bound_certified = false;
  const auto live_envelope_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_live_envelope);
  check(
      !live_envelope_check.exact_dual_tree_chain_certified &&
          live_envelope_check.cpu_exact_decision_chain_certified &&
          !live_envelope_check.emst_witness_certified,
      "live component-cutoff falsification invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_pointwise_envelope = result;
  bad_pointwise_envelope.rounds[0].dual_tree_search_audit.
      pointwise_at_most_frozen_envelope_certified = false;
  const auto pointwise_envelope_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_pointwise_envelope);
  check(
      !pointwise_envelope_check.exact_dual_tree_chain_certified &&
          pointwise_envelope_check.cpu_exact_decision_chain_certified &&
          !pointwise_envelope_check.emst_witness_certified,
      "pointwise frozen-envelope falsification invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_witness_counter = result;
  bad_witness_counter.rounds[0].dual_tree_search_audit.
      cpu_component_witness_leaf_update_count = 1U;
  const auto witness_counter_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_witness_counter);
  check(
      !witness_counter_check.exact_dual_tree_chain_certified &&
          witness_counter_check.cpu_exact_decision_chain_certified &&
          !witness_counter_check.emst_witness_certified,
      "persistent witness-counter falsification invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_bidirectional_seed = result;
  bad_bidirectional_seed.rounds[0].dual_tree_search_audit.
      bidirectional_component_seed_reduction_certified = false;
  const auto bidirectional_seed_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_bidirectional_seed);
  check(
      !bidirectional_seed_check.exact_dual_tree_chain_certified &&
          bidirectional_seed_check.cpu_exact_decision_chain_certified &&
          !bidirectional_seed_check.emst_witness_certified,
      "bidirectional seed falsification invalidates only traversal");

  K1DualTreeExactBoruvkaResult bad_seed_counter = result;
  auto& bad_seed_audit =
      bad_seed_counter.rounds[0].dual_tree_search_audit;
  if (bad_seed_audit.target_component_seed_kappa_update_count != 8U ||
      bad_seed_audit.target_component_seed_strict_cutoff_decrease_count != 0U) {
    bad_seed_audit.target_component_seed_kappa_update_count = 8U;
    bad_seed_audit.target_component_seed_strict_cutoff_decrease_count = 0U;
  } else {
    bad_seed_audit.target_component_seed_kappa_update_count = 7U;
  }
  const auto seed_counter_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_seed_counter);
  check(
      !seed_counter_check.exact_dual_tree_chain_certified &&
          seed_counter_check.cpu_exact_decision_chain_certified &&
          !seed_counter_check.emst_witness_certified,
      "plausible bidirectional counter mutation fails fresh audit equality");

  K1DualTreeExactBoruvkaResult bad_decision = result;
  bad_decision.rounds[0].exact_decision.component_minima[0] =
      bad_decision.rounds[0].exact_decision.component_minima[1];
  const auto decision_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_decision);
  check(
      decision_check.exact_dual_tree_chain_certified &&
          !decision_check.cpu_exact_decision_chain_certified &&
          !decision_check.emst_witness_certified,
      "persisted component-minimum falsification fails independently");

  K1DualTreeExactBoruvkaResult bad_status = result;
  bad_status.hierarchy_reduction_status =
      static_cast<K1HybridHierarchyReductionStatus>(255U);
  const auto status_check =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy, bad_status);
  check(
      !status_check.hierarchy_status_separation_certified &&
          !status_check.emst_witness_certified,
      "local shared witness cannot claim a hierarchy reduction");
}

}  // namespace

int main() {
  test_singleton_is_vacuously_certified();
  test_three_round_chain_and_falsifications();
  test_invalid_policy_fails_before_gpu();
  test_dual_tree_singleton_is_vacuously_certified();
  test_dual_tree_three_round_chain_and_falsifications();

  if (failures != 0) {
    std::cerr << failures << " seeded exact chain test(s) failed\n";
    return 1;
  }
  std::cout << "GPU K1 Boruvka seeded exact-chain tests passed\n";
  return 0;
}
