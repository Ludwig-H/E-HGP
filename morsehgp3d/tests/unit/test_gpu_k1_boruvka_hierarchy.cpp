#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "morsehgp3d/gpu/k1_dual_tree_normalized_product_adapter.hpp"
#include "morsehgp3d/gpu/k1_boruvka.hpp"
#include "morsehgp3d/hierarchy/boruvka.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1DualTreeExactBoruvkaResult;
using morsehgp3d::gpu::K1DualTreeNormalizedProductInitialization;
using morsehgp3d::gpu::K1DualTreeNormalizedProductInitializationDecision;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::K1SeededExactBoruvkaResult;
using morsehgp3d::gpu::K1SeededExactCompactHierarchy;
using morsehgp3d::gpu::K1SeededExactCompactHierarchyVerification;
using morsehgp3d::gpu::K1SeededExactHierarchyReductionStatus;
using morsehgp3d::gpu::K1SeededExactHierarchyScope;
using morsehgp3d::gpu::build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka;
using morsehgp3d::gpu::
    initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_k1_boruvka;
using morsehgp3d::gpu::verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::ExactDirectK1BoruvkaClosedCutSessionBudget;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductStreamBudget;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::hierarchy::
    initialize_exact_direct_k1_normalized_product_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<
              K1DualTreeNormalizedProductInitialization>);

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

template <typename Value>
[[nodiscard]] bool same_span(
    std::span<const Value> left,
    std::span<const Value> right) {
  return left.size() == right.size() &&
      std::equal(left.begin(), left.end(), right.begin());
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
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

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionBudget authority_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return ExactDirectK1BoruvkaClosedCutSessionBudget{
      point_count,
      64U,
      edge_count,
      edge_count,
      edge_count,
      2U * edge_count,
      point_count,
      point_count,
      point_count,
      point_count,
      point_count,
      2U * point_count,
      4096U};
}

[[nodiscard]] ExactDirectK1NormalizedProductStreamBudget product_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return ExactDirectK1NormalizedProductStreamBudget{
      point_count,
      point_count,
      edge_count,
      2U * edge_count,
      point_count + 1U,
      point_count,
      2U,
      edge_count,
      2U * edge_count,
      point_count / 20U,
      point_count / 20U};
}

[[nodiscard]] bool same_compact_forest(
    const K1CompactForest& left,
    const K1CompactForest& right) {
  return left.point_count == right.point_count &&
         left.levels == right.levels &&
         left.selected_edges == right.selected_edges &&
         left.merge_nodes == right.merge_nodes &&
         left.child_ids == right.child_ids &&
         left.equal_level_batches == right.equal_level_batches &&
         left.root_node_id == right.root_node_id &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters;
}

[[nodiscard]] bool same_source_result(
    const K1SeededExactBoruvkaResult& left,
    const K1SeededExactBoruvkaResult& right) {
  return left.point_count == right.point_count &&
         left.rounds == right.rounds &&
         left.emst_edges == right.emst_edges &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters &&
         left.morton_seed_policy == right.morton_seed_policy &&
         left.hierarchy_reduction_status ==
             right.hierarchy_reduction_status &&
         left.scientific_status == right.scientific_status &&
         left.bounded_morton_seed_chain_certified ==
             right.bounded_morton_seed_chain_certified &&
         left.exact_external_1nn_chain_certified ==
             right.exact_external_1nn_chain_certified &&
         left.cpu_exact_decision_chain_certified ==
             right.cpu_exact_decision_chain_certified &&
         left.canonical_contraction_chain_certified ==
             right.canonical_contraction_chain_certified &&
         left.fresh_replay_certified == right.fresh_replay_certified &&
         left.reference_cpu_witness_certified ==
             right.reference_cpu_witness_certified &&
         left.emst_witness_certified == right.emst_witness_certified;
}

[[nodiscard]] bool same_dual_tree_source_result(
    const K1DualTreeExactBoruvkaResult& left,
    const K1DualTreeExactBoruvkaResult& right) {
  return left.point_count == right.point_count &&
         left.rounds == right.rounds &&
         left.emst_edges == right.emst_edges &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.morton_seed_policy == right.morton_seed_policy &&
         left.hierarchy_reduction_status ==
             right.hierarchy_reduction_status &&
         left.scientific_status == right.scientific_status &&
         left.bounded_morton_seed_chain_certified ==
             right.bounded_morton_seed_chain_certified &&
         left.exact_dual_tree_chain_certified ==
             right.exact_dual_tree_chain_certified &&
         left.component_minima_only_persistence_certified ==
             right.component_minima_only_persistence_certified &&
         left.cpu_exact_decision_chain_certified ==
             right.cpu_exact_decision_chain_certified &&
         left.canonical_contraction_chain_certified ==
             right.canonical_contraction_chain_certified &&
         left.fresh_replay_certified == right.fresh_replay_certified &&
         left.reference_cpu_witness_certified ==
             right.reference_cpu_witness_certified &&
         left.emst_witness_certified == right.emst_witness_certified;
}

[[nodiscard]] bool all_local_certificates_close(
    const K1SeededExactCompactHierarchyVerification& verification) {
  return verification.source_emst_witness_certified &&
         verification.source_status_separation_certified &&
         verification.source_tree_binding_certified &&
         verification.exact_weights_certified &&
         verification.equal_level_batches_certified &&
         verification.canonical_multifusions_certified &&
         verification.compact_topology_certified &&
         verification.counters_certified &&
         verification.linear_storage_certified &&
         verification.reduction_status_certified &&
         verification.local_scope_certified &&
         verification.local_k1_hierarchy_certified;
}

void test_singleton_reduction_is_local_and_vacuous() {
  reset_fake_gpu_k1_boruvka();
  const K1SeededExactCompactHierarchy empty_reduction;
  check(
      empty_reduction.reduction_status ==
              K1SeededExactHierarchyReductionStatus::not_certified &&
          empty_reduction.scope == K1SeededExactHierarchyScope::unspecified,
      "default compact adapter certifies neither status nor scope");
  const std::array<CertifiedPoint3, 1> input{point(7.0, -2.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  const K1SeededExactBoruvkaResult source_snapshot = source;

  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);
  const K1SeededExactCompactHierarchyVerification verification =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, reduction);

  check(
      reduction.reduction_status ==
              K1SeededExactHierarchyReductionStatus::
                  compact_k1_forest_certified &&
          reduction.scope ==
              K1SeededExactHierarchyScope::local_k1_compact_forest_only,
      "singleton publishes only the dedicated local compact-hierarchy status");
  check(
      reduction.forest.point_count == 1U &&
          reduction.forest.levels.empty() &&
          reduction.forest.selected_edges.empty() &&
          reduction.forest.merge_nodes.empty() &&
          reduction.forest.child_ids.empty() &&
          reduction.forest.equal_level_batches.empty() &&
          reduction.forest.root_node_id == 0U &&
          reduction.forest.counters.linear_storage_entry_count == 0U &&
          reduction.forest.counters.linear_storage_entry_limit == 0U,
      "singleton compact hierarchy keeps one implicit leaf and no arena entry");
  check(
      all_local_certificates_close(verification),
      "singleton closes source replay, topology, counters and local scope");
  check(
      same_source_result(source, source_snapshot) &&
          source.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          source.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only,
      "singleton reduction leaves the source EMST witness byte-logically unchanged");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "singleton reduction and both verifiers need no GPU proposal");
  check(
      std::string_view{K1SeededExactCompactHierarchy::proof_basis} ==
          "verified_exact_emst_equal_level_compact_k1_reduction_v1",
      "local adapter exposes its separate proof basis");
}

void test_chain_matches_both_fresh_exact_reductions() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1ExactBoruvkaResult reference =
      build_exact_lbvh_boruvka(index, cloud);
  const K1CompactForest expected = build_compact_k1_forest(
      reference.point_count,
      std::span<const morsehgp3d::hierarchy::ExactEmstEdge>{
          reference.emst_edges});
  const K1SeededExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  const K1SeededExactBoruvkaResult source_snapshot = source;

  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);
  const K1SeededExactCompactHierarchyVerification verification =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, reduction);

  check(
      same_compact_forest(reduction.forest, expected) &&
          reduction.forest.selected_edges == source.emst_edges,
      "chain reduction equals the fresh CPU anchor and binds the source tree");
  check(
      reduction.forest.point_count == 8U &&
          reduction.forest.selected_edges.size() == 7U &&
          reduction.forest.counters.root_coverage_size == 8U &&
          reduction.forest.counters.stored_coverage_point_id_count == 0U &&
          reduction.forest.counters.linear_storage_entry_count <=
              reduction.forest.counters.linear_storage_entry_limit,
      "chain persists exactly one compact linear-size K1 forest");
  check(
      all_local_certificates_close(verification),
      "chain closes every local hierarchy certificate");
  check(
      same_source_result(source, source_snapshot) &&
          source.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          source.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only,
      "chain adapter never promotes or mutates its source witness");
}

void test_equal_level_square_is_one_canonical_multifusion() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0),
      point(0.0, 2.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  const K1SeededExactBoruvkaResult source_snapshot = source;
  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);
  const K1SeededExactCompactHierarchyVerification verification =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, reduction);
  const K1CompactForest& forest = reduction.forest;

  check(
      forest.levels == std::vector<ExactLevel>{level(1)} &&
          forest.selected_edges.size() == 3U &&
          forest.equal_level_batches.size() == 1U &&
          forest.equal_level_batches.front().selected_edge_count == 3U &&
          forest.equal_level_batches.front().merge_node_count == 1U,
      "square contracts all equal selected edges in one frozen exact batch");
  check(
      forest.merge_nodes.size() == 1U &&
          forest.merge_nodes.front().child_count == 4U &&
          forest.child_ids.size() == 4U &&
          forest.counters.multifusion_count == 1U &&
          forest.counters.max_merge_arity == 4U,
      "square remains one four-way canonical multifusion without binarization");
  check(
      all_local_certificates_close(verification) &&
          same_source_result(source, source_snapshot),
      "equal-level reduction certifies the multifusion and preserves its source");
}

void test_dual_tree_chain_reduces_to_the_same_compact_forest() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1ExactBoruvkaResult reference =
      build_exact_lbvh_boruvka(index, cloud);
  const K1CompactForest expected = build_compact_k1_forest(
      reference.point_count,
      std::span<const morsehgp3d::hierarchy::ExactEmstEdge>{
          reference.emst_edges});
  const K1DualTreeExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy);

  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);
  const K1SeededExactCompactHierarchyVerification verification =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, reduction);

  check(
      same_compact_forest(reduction.forest, expected) &&
          reduction.forest.selected_edges == source.emst_edges,
      "dual-tree source reduces to the independent CPU compact forest");
  check(
      all_local_certificates_close(verification) &&
          reduction.forest.counters.stored_coverage_point_id_count == 0U &&
          reduction.forest.counters.linear_storage_entry_count <=
              reduction.forest.counters.linear_storage_entry_limit,
      "dual-tree compact adapter closes replay, topology and linear storage");
  check(
      source.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          source.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only &&
          source.emst_witness_certified,
      "dual-tree source remains a separate local EMST witness");

  K1DualTreeExactBoruvkaResult falsified = source;
  ++falsified.rounds.front().dual_tree_search_audit.
      covered_unordered_point_pair_count;
  const K1SeededExactCompactHierarchyVerification falsified_verification =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, falsified, reduction);
  check(
      !falsified_verification.source_emst_witness_certified &&
          !falsified_verification.local_k1_hierarchy_certified,
      "copied true flags cannot launder a falsified dual-tree traversal");
  check_throws<std::logic_error>(
      [&index, &cloud, &falsified]() {
        static_cast<void>(
            build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
                index,
                cloud,
                K1BoruvkaMortonSeedPolicy{1U},
                falsified));
      },
      "dual-tree compact builder fails before reducing a falsified source");
}

void test_dual_tree_compact_authority_enters_normalized_k1_product_stream() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1DualTreeExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, policy);
  const K1DualTreeExactBoruvkaResult source_snapshot = source;
  const auto k1_budget = authority_budget(cloud.size());
  const auto stream_budget = product_budget(cloud.size());

  K1DualTreeNormalizedProductInitialization adapted =
      initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree(
          index,
          cloud,
          policy,
          source,
          k1_budget,
          stream_budget);
  const K1ExactBoruvkaResult reference =
      build_exact_lbvh_boruvka(index, cloud);
  auto independent = initialize_exact_direct_k1_normalized_product_stream(
      index,
      cloud,
      reference,
      k1_budget,
      stream_budget);

  check(
      adapted.certified_tower_consumable_stream() &&
          independent.certified_initialized_stream() &&
          adapted.decision ==
              K1DualTreeNormalizedProductInitializationDecision::
                  complete_certified_tower_consumable_k1_stream,
      "fresh dual-tree replay publishes a normalized K1 stream capability");
  check(
      adapted.stream.canonical_cloud_digest ==
              independent.canonical_cloud_digest &&
          adapted.stream.source_forest_digest ==
              independent.source_forest_digest &&
          adapted.recertified_compact_source_forest_digest ==
              independent.source_forest_digest &&
          adapted.stream.scientific_digest == independent.scientific_digest &&
          adapted.stream.initial_scientific_chain_digest ==
              independent.initial_scientific_chain_digest &&
          adapted.stream.initial_at_least20_view_digest ==
              independent.initial_at_least20_view_digest,
      "dual-tree compact identity equals the independently replayed normalized source");
  check(
      adapted.stream.exact_merge_node_count <= cloud.size() - 1U &&
          adapted.stream.exact_child_reference_count <=
              2U * (cloud.size() - 1U) &&
          adapted.shared_morton_lbvh_identity_certified &&
          adapted.returned_session_owns_only_one_compact_k1_forest &&
          !adapted.pair_matrix_gamma_delaunay_cells_or_cofaces_materialized &&
          !adapted.complete_graph_emst_fallback_used &&
          !adapted.public_status_claimed,
      "the tower seam stays Morton/LBVH sparse with no dense EMST fallback");
  check(
      same_dual_tree_source_result(source, source_snapshot) &&
          source.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          source.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only,
      "normalized adaptation does not mutate or promote the dual-tree source");

  std::size_t compared_batch_count = 0U;
  while (!adapted.stream.session.complete() &&
         !independent.session.complete()) {
    auto adapted_prepared = adapted.stream.session.prepare_next();
    auto independent_prepared = independent.session.prepare_next();
    check(
        adapted_prepared.certified_prepared_batch() &&
            independent_prepared.certified_prepared_batch() &&
            adapted_prepared.ticket->prepared_record() ==
                independent_prepared.ticket->prepared_record() &&
            same_span(
                adapted_prepared.ticket->at_least20_transitions(),
                independent_prepared.ticket->at_least20_transitions()) &&
            same_span(
                adapted_prepared.ticket->
                    at_least20_visible_child_root_ids(),
                independent_prepared.ticket->
                    at_least20_visible_child_root_ids()),
        "every dual-tree-adapted normalized batch equals the CPU replay");
    if (!adapted_prepared.certified_prepared_batch() ||
        !independent_prepared.certified_prepared_batch()) {
      break;
    }
    const auto adapted_commit = adapted.stream.session.commit(
        std::move(*adapted_prepared.ticket));
    const auto independent_commit = independent.session.commit(
        std::move(*independent_prepared.ticket));
    check(
        adapted_commit.certified_atomic_batch_commit() &&
            independent_commit.certified_atomic_batch_commit() &&
            adapted_commit.record == independent_commit.record &&
            adapted_commit.at_least20_transitions ==
                independent_commit.at_least20_transitions &&
            adapted_commit.at_least20_visible_child_root_ids ==
                independent_commit.at_least20_visible_child_root_ids,
        "dual-tree and CPU normalized batches commit the same exact science");
    ++compared_batch_count;
  }
  const auto adapted_seal = adapted.stream.session.seal();
  const auto independent_seal = independent.session.seal();
  check(
      adapted.stream.session.complete() && independent.session.complete() &&
          compared_batch_count == adapted.stream.exact_product_batch_count &&
          adapted_seal.certified_terminal_closure() &&
          independent_seal.certified_terminal_closure() &&
          adapted_seal.root_node_id == independent_seal.root_node_id &&
          adapted_seal.total_batch_count == independent_seal.total_batch_count &&
          adapted_seal.total_merge_node_count ==
              independent_seal.total_merge_node_count &&
          adapted_seal.total_child_reference_count ==
              independent_seal.total_child_reference_count &&
          adapted_seal.terminal_scientific_chain_digest ==
              independent_seal.terminal_scientific_chain_digest &&
          adapted_seal.terminal_at_least20_view_digest ==
              independent_seal.terminal_at_least20_view_digest,
      "the adapter closes the same terminal normalized K1 stream");

  K1DualTreeExactBoruvkaResult falsified = source;
  ++falsified.rounds.front().dual_tree_search_audit.
      covered_unordered_point_pair_count;
  const auto rejected_source =
      initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree(
          index,
          cloud,
          policy,
          falsified,
          k1_budget,
          stream_budget);
  check(
      rejected_source.decision ==
              K1DualTreeNormalizedProductInitializationDecision::
                  no_dual_tree_source_rejected &&
          !rejected_source.certified_tower_consumable_stream() &&
          !rejected_source.stream.session.certified_stream() &&
          !rejected_source.independent_reference_cpu_anchor_rebuilt &&
          !rejected_source.compact_authority_freshly_recertified,
      "a copied dual-tree certificate cannot reach a normalized K1 arena");

  auto insufficient_stream_budget = stream_budget;
  --insufficient_stream_budget.maximum_child_reference_count;
  const auto rejected_budget =
      initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree(
          index,
          cloud,
          policy,
          source,
          k1_budget,
          insufficient_stream_budget);
  check(
      rejected_budget.decision ==
              K1DualTreeNormalizedProductInitializationDecision::
                  no_normalized_stream_rejected &&
          rejected_budget.stream.decision ==
              morsehgp3d::hierarchy::
                  ExactDirectK1NormalizedProductStreamInitializationDecision::
                      no_preflight_budget_exhausted &&
          !rejected_budget.stream.session.certified_stream() &&
          !rejected_budget.certified_tower_consumable_stream(),
      "normalized cap minus one fails closed after source recertification");
}

void test_verifier_rejects_source_and_compact_arena_falsifications() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input = chain_points();
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);

  K1SeededExactBoruvkaResult bad_source = source;
  bad_source.hierarchy_reduction_status =
      static_cast<K1HybridHierarchyReductionStatus>(255U);
  const auto source_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, bad_source, reduction);
  check(
      !source_check.source_emst_witness_certified &&
          !source_check.source_status_separation_certified &&
          !source_check.local_k1_hierarchy_certified,
      "invalid source hierarchy status cannot be laundered by the local adapter");
  check_throws<std::logic_error>(
      [&index, &cloud, &bad_source]() {
        static_cast<void>(
            build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
                index,
                cloud,
                K1BoruvkaMortonSeedPolicy{1U},
                bad_source));
      },
      "builder fails closed when the source witness loses status separation");

  K1SeededExactCompactHierarchy bad_status = reduction;
  bad_status.reduction_status =
      K1SeededExactHierarchyReductionStatus::not_certified;
  const auto status_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_status);
  check(
      !status_check.reduction_status_certified &&
          !status_check.local_k1_hierarchy_certified,
      "untrusted local reduction status is recertified");

  K1SeededExactCompactHierarchy bad_scope = reduction;
  bad_scope.scope = static_cast<K1SeededExactHierarchyScope>(255U);
  const auto scope_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_scope);
  check(
      !scope_check.local_scope_certified &&
          !scope_check.local_k1_hierarchy_certified,
      "untrusted scope cannot escape the local K1 adapter");

  K1SeededExactCompactHierarchy bad_edge = reduction;
  bad_edge.forest.selected_edges.front().u =
      bad_edge.forest.selected_edges.front().v;
  const auto edge_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_edge);
  check(
      !edge_check.source_tree_binding_certified &&
          !edge_check.local_k1_hierarchy_certified,
      "selected-edge arena is bound exactly to both fresh trees");

  K1SeededExactCompactHierarchy bad_levels = reduction;
  bad_levels.forest.levels.front() = level(999);
  const auto level_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_levels);
  check(
      !level_check.equal_level_batches_certified &&
          !level_check.local_k1_hierarchy_certified,
      "exact-level arena and equality batches are recertified together");

  K1SeededExactCompactHierarchy bad_multifusion = reduction;
  ++bad_multifusion.forest.merge_nodes.front().minimum_point_id;
  const auto multifusion_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_multifusion);
  check(
      !multifusion_check.canonical_multifusions_certified &&
          !multifusion_check.local_k1_hierarchy_certified,
      "merge-node arena is compared to both canonical reductions");

  K1SeededExactCompactHierarchy bad_children = reduction;
  bad_children.forest.child_ids.front() = PointId{7U};
  const auto child_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_children);
  check(
      !child_check.canonical_multifusions_certified &&
          !child_check.local_k1_hierarchy_certified,
      "child CSR arena is independently recertified");

  K1SeededExactCompactHierarchy bad_root = reduction;
  bad_root.forest.root_node_id = 0U;
  const auto root_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_root);
  check(
      !root_check.compact_topology_certified &&
          !root_check.local_k1_hierarchy_certified,
      "compact root identity is independently recertified");

  K1SeededExactCompactHierarchy bad_weight = reduction;
  bad_weight.forest.total_squared_weight = level(0);
  const auto weight_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_weight);
  check(
      !weight_check.exact_weights_certified &&
          !weight_check.local_k1_hierarchy_certified,
      "both exact compact weights are bound to source and CPU anchor");

  K1SeededExactCompactHierarchy bad_counters = reduction;
  ++bad_counters.forest.counters.merge_event_count;
  const auto counter_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_counters);
  check(
      !counter_check.counters_certified &&
          !counter_check.local_k1_hierarchy_certified,
      "compact aggregate counters are recomputed");

  K1SeededExactCompactHierarchy bad_storage = reduction;
  ++bad_storage.forest.counters.linear_storage_entry_count;
  const auto storage_check =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source, bad_storage);
  check(
      !storage_check.counters_certified &&
          !storage_check.linear_storage_certified &&
          !storage_check.local_k1_hierarchy_certified,
      "five-arena entry count and linear bound fail closed");
}

void test_invalid_or_mismatched_trusted_policy_fails_before_gpu_replay() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 2> input{point(0.0), point(1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const K1SeededExactBoruvkaResult source =
      build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, policy);
  const K1SeededExactCompactHierarchy reduction =
      build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index, cloud, policy, source);

  std::size_t launches = fake_gpu_k1_boruvka_launch_count();
  const auto mismatch =
      verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
          index,
          cloud,
          K1BoruvkaMortonSeedPolicy{2U},
          source,
          reduction);
  check(
      !mismatch.source_emst_witness_certified &&
          !mismatch.local_k1_hierarchy_certified &&
          fake_gpu_k1_boruvka_launch_count() == launches,
      "mismatched trusted policy fails before fresh GPU replay");

  launches = fake_gpu_k1_boruvka_launch_count();
  check_throws<std::invalid_argument>(
      [&index, &cloud, &source, &reduction]() {
        static_cast<void>(
            verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
                index,
                cloud,
                K1BoruvkaMortonSeedPolicy{0U},
                source,
                reduction));
      },
      "zero trusted Morton radius is rejected as invalid policy");
  check(
      fake_gpu_k1_boruvka_launch_count() == launches,
      "invalid trusted policy throws before any GPU proposal");
}

}  // namespace

int main() {
  test_singleton_reduction_is_local_and_vacuous();
  test_chain_matches_both_fresh_exact_reductions();
  test_equal_level_square_is_one_canonical_multifusion();
  test_dual_tree_chain_reduces_to_the_same_compact_forest();
  test_dual_tree_compact_authority_enters_normalized_k1_product_stream();
  test_verifier_rejects_source_and_compact_arena_falsifications();
  test_invalid_or_mismatched_trusted_policy_fails_before_gpu_replay();

  if (failures != 0) {
    std::cerr << failures << " GPU K1 hierarchy reduction test(s) failed\n";
    return 1;
  }
  std::cout << "GPU K1 hierarchy reduction tests passed\n";
  return 0;
}
