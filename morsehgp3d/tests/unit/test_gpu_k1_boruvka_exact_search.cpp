#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "morsehgp3d/gpu/k1_boruvka.hpp"
#include "morsehgp3d/hierarchy/boruvka.hpp"
#include "morsehgp3d/hierarchy/emst.hpp"

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
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaComponentDualTreeSearchAudit;
using morsehgp3d::gpu::K1BoruvkaComponentDualTreeSearchStatus;
using morsehgp3d::gpu::K1BoruvkaComponentEnvelopeMode;
using morsehgp3d::gpu::K1BoruvkaDualTreeSearchStatus;
using morsehgp3d::gpu::K1BoruvkaExactSearchStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaPointMinimum;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::gpu::K1BoruvkaSeededComponentDualTreeRoundResolution;
using morsehgp3d::gpu::K1BoruvkaSeededDualTreeRoundResolution;
using morsehgp3d::gpu::K1BoruvkaSeededExactRoundResolution;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_k1_boruvka;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::K1BoruvkaComponentMinimum;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::hierarchy::contract_exact_k1_boruvka_round;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
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
    std::cerr << "FAIL: " << message << " (unexpected: "
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

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::vector<CertifiedPoint3> morton_overlap_points(
    std::size_t scale) {
  if (scale < 4U || scale > 16U ||
      (scale & (scale - 1U)) != 0U) {
    throw std::invalid_argument(
        "the Morton-overlap scale must be 4, 8, or 16");
  }
  const std::size_t guard_pair_count = scale * scale;
  const double dyadic_step =
      1.0 / (2.0 * static_cast<double>(scale));
  const double guard_denominator =
      8.0 * static_cast<double>(guard_pair_count);
  std::vector<CertifiedPoint3> points;
  points.reserve(4U * guard_pair_count + 2U);
  points.push_back(point(0.0, 0.0, 0.0));
  for (std::size_t guard = 0U;
       guard < guard_pair_count;
       ++guard) {
    const double x =
        9.0 * dyadic_step +
        static_cast<double>(guard + 1U) * dyadic_step /
            guard_denominator;
    points.push_back(point(x, 9.0, 9.0));
    points.push_back(point(x, 11.0, 11.0));
  }
  for (std::size_t y_index = 0U;
       y_index < scale;
       ++y_index) {
    for (std::size_t z_index = 0U;
         z_index < 2U * scale;
         ++z_index) {
      points.push_back(point(
          9.0 * dyadic_step + dyadic_step / 4.0,
          9.5 + static_cast<double>(y_index) * dyadic_step,
          9.5 + static_cast<double>(z_index) * dyadic_step));
    }
  }
  constexpr double morton_span = 16777216.0;
  points.push_back(point(
      morton_span * dyadic_step,
      morton_span,
      morton_span));
  return points;
}

[[nodiscard]] bool edge_less(
    const ExactEmstEdge& left, const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  return left.u != right.u ? left.u < right.u : left.v < right.v;
}

[[nodiscard]] PointId other_endpoint(
    const ExactEmstEdge& edge, PointId source) {
  if (edge.u == source) {
    return edge.v;
  }
  if (edge.v == source) {
    return edge.u;
  }
  throw std::logic_error("edge not incident to source");
}

[[nodiscard]] std::size_t component_count(
    std::span<const PointId> labels) {
  return static_cast<std::size_t>(std::count_if(
      labels.begin(), labels.end(),
      [index = PointId{0}](PointId label) mutable {
        const bool representative = label == index;
        ++index;
        return representative;
      }));
}

[[nodiscard]] std::vector<K1BoruvkaPointMinimum> brute_point_minima(
    const CanonicalPointCloud& cloud,
    std::span<const PointId> labels) {
  const auto complete = build_exact_complete_graph_emst(cloud);
  std::vector<K1BoruvkaPointMinimum> result;
  result.reserve(cloud.size());
  for (std::size_t source_index = 0U;
       source_index < cloud.size();
       ++source_index) {
    const PointId source = static_cast<PointId>(source_index);
    std::optional<ExactEmstEdge> best;
    for (const ExactEmstEdge& edge : complete.complete_edges) {
      if (edge.u != source && edge.v != source) {
        continue;
      }
      const PointId target = other_endpoint(edge, source);
      if (labels[static_cast<std::size_t>(target)] != labels[source_index] &&
          (!best.has_value() || edge_less(edge, *best))) {
        best = edge;
      }
    }
    if (!best.has_value()) {
      throw std::logic_error("nonterminal source has no external edge");
    }
    result.push_back(K1BoruvkaPointMinimum{source, std::move(*best)});
  }
  return result;
}

void check_round(
    const K1BoruvkaSeededExactRoundResolution& result,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> labels,
    const std::vector<K1BoruvkaComponentMinimum>& reference_minima,
    K1BoruvkaMortonSeedPolicy policy,
    const std::string& fixture) {
  const std::size_t point_count = cloud.size();
  const std::size_t frozen_component_count = component_count(labels);
  check(
      result.point_minima == brute_point_minima(cloud, labels) &&
          result.component_minima == reference_minima,
      fixture + " matches exhaustive point minima and reference components");
  check(
      result.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          result.search_status ==
              K1BoruvkaExactSearchStatus::
                  exact_external_1nn_branch_and_bound_certified,
      fixture + " certifies proposal and exact decision separately");

  for (std::size_t source_index = 0U;
       source_index < result.point_minima.size();
       ++source_index) {
    const auto& minimum = result.point_minima[source_index];
    const PointId source = static_cast<PointId>(source_index);
    const PointId target = other_endpoint(minimum.outgoing_edge, source);
    check(
        minimum.source_point_id == source &&
            minimum.outgoing_edge.u < minimum.outgoing_edge.v &&
            labels[static_cast<std::size_t>(target)] != labels[source_index],
        fixture + " orders one canonical external edge per PointId");
  }
  for (std::size_t index_in_minima = 1U;
       index_in_minima < result.component_minima.size();
       ++index_in_minima) {
    check(
        result.component_minima[index_in_minima - 1U].component_label <
            result.component_minima[index_in_minima].component_label,
        fixture + " orders component minima by representative");
  }

  const auto& seed = result.morton_seed_audit;
  check(
      seed.source_count == point_count &&
          seed.window_radius == policy.window_radius &&
          seed.gpu_kernel_launch_count == 1U &&
          seed.gpu_synchronization_count == 1U &&
          seed.complete_source_coverage_certified &&
          seed.bounded_window_certified &&
          seed.external_targets_recertified &&
          seed.exact_monotone_cutoff_certified,
      fixture + " closes the bounded Morton seed audit");

  const auto& audit = result.search_audit;
  check(
      audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == frozen_component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count &&
          audit.point_query_count == point_count &&
          audit.seed_incumbent_count == point_count &&
          audit.point_minimum_count == point_count &&
          audit.component_minimum_count == frozen_component_count &&
          audit.maximum_cpu_node_visit_count_per_source > 0U &&
          audit.maximum_cpu_node_visit_count_per_source <=
              audit.cpu_node_visit_count &&
          audit.maximum_cpu_exact_point_distance_evaluation_count_per_source <=
              audit.cpu_exact_point_distance_evaluation_count &&
          audit.maximum_cpu_frontier_size_per_source > 0U &&
          audit.maximum_cpu_frontier_size_per_source <=
              audit.resident_node_count &&
          audit.cpu_node_visit_count >= point_count &&
          audit.cpu_exact_aabb_bound_evaluation_count ==
              audit.cpu_node_visit_count &&
          audit.cpu_seed_leaf_distance_reuse_count <= point_count &&
          audit.cpu_node_visit_count ==
              audit.cpu_strict_aabb_prune_count +
                  audit.cpu_internal_node_expansion_count +
                  audit.cpu_exact_point_distance_evaluation_count +
                  audit.cpu_seed_leaf_distance_reuse_count,
      fixture + " closes exact-search work and cardinalities");
  check(
      audit.frozen_labels_certified &&
          audit.lbvh_topology_and_exact_aabbs_certified &&
          audit.complete_source_seed_coverage_certified &&
          audit.external_seed_targets_recertified &&
          audit.exact_seed_cutoffs_recertified &&
          audit.uniform_component_prunes_certified &&
          audit.strict_only_aabb_pruning_certified &&
          audit.complete_frontier_exhaustion_certified &&
          audit.canonical_kappa_resolution_certified &&
          audit.point_minima_complete && audit.component_minima_complete,
      fixture + " publishes every exact-search certificate");
}

void check_dual_tree_round(
    const K1BoruvkaSeededDualTreeRoundResolution& result,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> labels,
    const std::vector<K1BoruvkaComponentMinimum>& reference_minima,
    K1BoruvkaMortonSeedPolicy policy,
    const std::string& fixture) {
  const std::size_t point_count = cloud.size();
  const std::size_t frozen_component_count = component_count(labels);
  check(
      result.point_minima == brute_point_minima(cloud, labels) &&
          result.component_minima == reference_minima,
      fixture + " matches exhaustive point and component minima");
  check(
      result.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          result.search_status ==
              K1BoruvkaDualTreeSearchStatus::
                  exact_external_1nn_shared_lbvh_dual_tree_certified &&
          result.morton_seed_audit.window_radius == policy.window_radius,
      fixture + " separates the Morton proposal from dual-tree decision");

  const auto& audit = result.search_audit;
  const std::size_t maximum_unordered_point_pairs =
      point_count * (point_count - 1U) / 2U;
  check(
      audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == frozen_component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count &&
          audit.seed_incumbent_count == point_count &&
          audit.dynamic_incumbent_node_count == audit.resident_node_count &&
          audit.point_minimum_count == point_count &&
          audit.component_minimum_count == frozen_component_count &&
          audit.unordered_point_pair_count ==
              maximum_unordered_point_pairs &&
          audit.covered_unordered_point_pair_count ==
              maximum_unordered_point_pairs &&
          audit.lbvh_maximum_depth ==
              index.build_counters().maximum_depth &&
          audit.certified_depth_first_frontier_bound ==
              2U * index.build_counters().maximum_depth + 1U &&
          audit.certified_node_pair_visit_bound ==
              point_count * (point_count + 1U) - 1U &&
          audit.maximum_cpu_frontier_size > 0U &&
          audit.maximum_cpu_frontier_size <=
              audit.certified_depth_first_frontier_bound &&
          audit.maximum_cpu_frontier_size <=
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_aabb_pair_bound_evaluation_count ==
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_point_pair_distance_evaluation_count <=
              maximum_unordered_point_pairs &&
          audit.cpu_node_pair_visit_count <=
              audit.certified_node_pair_visit_bound &&
          audit.cpu_node_pair_visit_count ==
              audit.cpu_uniform_same_component_pair_prune_count +
                  audit.cpu_strict_aabb_pair_prune_count +
                  audit.cpu_exact_point_pair_distance_evaluation_count +
                  audit.cpu_node_pair_expansion_count,
      fixture + " closes shared node-pair work and cardinalities");
  check(
      audit.frozen_labels_certified &&
          audit.lbvh_topology_and_exact_aabbs_certified &&
          audit.complete_source_seed_coverage_certified &&
          audit.external_seed_targets_recertified &&
          audit.exact_seed_cutoffs_recertified &&
          audit.dynamic_incumbent_tree_certified &&
          audit.canonical_unordered_pair_partition_certified &&
          audit.uniform_component_pair_prunes_certified &&
          audit.strict_only_aabb_pair_pruning_certified &&
          audit.depth_first_frontier_bound_certified &&
          audit.node_pair_visit_bound_certified &&
          audit.complete_frontier_exhaustion_certified &&
          audit.canonical_kappa_resolution_certified &&
          audit.point_minima_complete && audit.component_minima_complete,
      fixture + " publishes every shared dual-tree certificate");
}

void check_component_dual_tree_round(
    const K1BoruvkaSeededComponentDualTreeRoundResolution& result,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> labels,
    const std::vector<K1BoruvkaComponentMinimum>& reference_minima,
    K1BoruvkaMortonSeedPolicy policy,
    const std::string& fixture,
    K1BoruvkaComponentEnvelopeMode expected_envelope_mode =
        K1BoruvkaComponentEnvelopeMode::frozen_initial) {
  const std::size_t point_count = cloud.size();
  const std::size_t frozen_component_count = component_count(labels);
  check(
      result.component_minima == reference_minima,
      fixture + " matches the independent component minima");
  check(
      result.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          result.search_status ==
              K1BoruvkaComponentDualTreeSearchStatus::
                  exact_component_minima_shared_lbvh_dual_tree_certified &&
          result.morton_seed_audit.window_radius == policy.window_radius,
      fixture + " separates point seeds from component-direct decisions");

  const auto& audit = result.search_audit;
  const std::size_t maximum_unordered_point_pairs =
      point_count * (point_count - 1U) / 2U;
  const bool deduplicated_current_envelope =
      expected_envelope_mode == K1BoruvkaComponentEnvelopeMode::
                                    exact_current_deduplicated_mixed_ancestors;
  const bool exact_current_envelope =
      deduplicated_current_envelope ||
      expected_envelope_mode == K1BoruvkaComponentEnvelopeMode::
                                    exact_current_maximal_uniform_roots;
  const bool deduplicated_counts_zero =
      audit.cpu_component_mixed_ancestor_discovery_count == 0U &&
      audit.cpu_component_distinct_mixed_ancestor_count == 0U &&
      audit.cpu_component_duplicate_mixed_ancestor_discovery_count == 0U &&
      audit.maximum_component_distinct_mixed_ancestor_count == 0U;
  const bool envelope_update_counts_close =
      (expected_envelope_mode ==
               K1BoruvkaComponentEnvelopeMode::frozen_initial &&
           audit.cpu_component_witness_leaf_update_count == 0U &&
           audit.cpu_component_witness_ancestor_update_count == 0U &&
           audit.component_uniform_root_count == 0U &&
           audit.component_uniform_root_leaf_coverage_count == 0U &&
           audit.cpu_component_uniform_root_update_count == 0U &&
           audit.cpu_component_mixed_ancestor_recomputation_count == 0U &&
           audit.cpu_component_mixed_ancestor_update_count == 0U &&
           deduplicated_counts_zero) ||
      (expected_envelope_mode ==
               K1BoruvkaComponentEnvelopeMode::
                   sparse_witness_path_monotone &&
           audit.cpu_component_witness_leaf_update_count ==
               audit.cpu_strict_component_cutoff_decrease_count &&
           audit.cpu_component_witness_ancestor_update_count <=
               audit.cpu_component_witness_leaf_update_count *
                   audit.lbvh_maximum_depth &&
           audit.component_uniform_root_count == 0U &&
           audit.component_uniform_root_leaf_coverage_count == 0U &&
           audit.cpu_component_uniform_root_update_count == 0U &&
           audit.cpu_component_mixed_ancestor_recomputation_count == 0U &&
           audit.cpu_component_mixed_ancestor_update_count == 0U &&
           deduplicated_counts_zero) ||
      (exact_current_envelope && !deduplicated_current_envelope &&
           audit.cpu_component_witness_leaf_update_count == 0U &&
           audit.cpu_component_witness_ancestor_update_count == 0U &&
           audit.component_uniform_root_count >= frozen_component_count &&
           audit.component_uniform_root_count <= point_count &&
           audit.component_uniform_root_leaf_coverage_count == point_count &&
           audit.cpu_component_uniform_root_update_count >=
               audit.cpu_strict_component_cutoff_decrease_count &&
           audit.cpu_component_uniform_root_update_count <=
               audit.cpu_strict_component_cutoff_decrease_count *
                   audit.component_uniform_root_count &&
           audit.cpu_component_mixed_ancestor_recomputation_count <=
               audit.cpu_component_uniform_root_update_count *
                   audit.lbvh_maximum_depth &&
           audit.cpu_component_mixed_ancestor_update_count <=
               audit.cpu_component_mixed_ancestor_recomputation_count &&
           deduplicated_counts_zero) ||
      (deduplicated_current_envelope &&
           audit.cpu_component_witness_leaf_update_count == 0U &&
           audit.cpu_component_witness_ancestor_update_count == 0U &&
           audit.component_uniform_root_count >= frozen_component_count &&
           audit.component_uniform_root_count <= point_count &&
           audit.component_uniform_root_leaf_coverage_count == point_count &&
           audit.cpu_component_uniform_root_update_count >=
               audit.cpu_strict_component_cutoff_decrease_count &&
           audit.cpu_component_duplicate_mixed_ancestor_discovery_count ==
               audit.cpu_component_uniform_root_update_count -
                   audit.cpu_strict_component_cutoff_decrease_count &&
           audit.cpu_component_mixed_ancestor_discovery_count ==
               audit.cpu_component_distinct_mixed_ancestor_count +
                   audit.cpu_component_duplicate_mixed_ancestor_discovery_count &&
           audit.cpu_component_uniform_root_update_count <=
               audit.cpu_component_mixed_ancestor_recomputation_count &&
           audit.cpu_component_uniform_root_update_count <=
               audit.cpu_component_distinct_mixed_ancestor_count &&
           audit.cpu_component_mixed_ancestor_recomputation_count <=
               audit.cpu_component_distinct_mixed_ancestor_count &&
           audit.cpu_component_distinct_mixed_ancestor_count <=
               audit.cpu_strict_component_cutoff_decrease_count *
                   audit.maximum_component_distinct_mixed_ancestor_count &&
           audit.cpu_component_mixed_ancestor_update_count <=
               audit.cpu_component_mixed_ancestor_recomputation_count &&
           audit.maximum_component_distinct_mixed_ancestor_count <=
               audit.mixed_lbvh_node_count &&
           audit.maximum_component_distinct_mixed_ancestor_count <=
               audit.cpu_component_distinct_mixed_ancestor_count);
  check(
      audit.component_envelope_mode == expected_envelope_mode &&
          audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == frozen_component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count &&
          audit.point_seed_count == point_count &&
          audit.component_seed_incumbent_count ==
              frozen_component_count &&
          audit.target_component_seed_offer_count == point_count &&
          audit.target_component_seed_kappa_update_count <=
              point_count &&
          audit.target_component_seed_strict_cutoff_decrease_count <=
              audit.target_component_seed_kappa_update_count &&
          audit.component_cutoff_upper_envelope_node_count ==
              audit.resident_node_count &&
          audit.component_minimum_count == frozen_component_count &&
          audit.unordered_point_pair_count ==
              maximum_unordered_point_pairs &&
          audit.covered_unordered_point_pair_count ==
              maximum_unordered_point_pairs &&
          audit.lbvh_maximum_depth ==
              index.build_counters().maximum_depth &&
          audit.certified_depth_first_frontier_bound ==
              2U * index.build_counters().maximum_depth + 1U &&
          audit.certified_node_pair_visit_bound ==
              point_count * (point_count + 1U) - 1U &&
          audit.maximum_cpu_frontier_size > 0U &&
          audit.maximum_cpu_frontier_size <=
              audit.certified_depth_first_frontier_bound &&
          audit.cpu_node_pair_visit_count <=
              audit.certified_node_pair_visit_bound &&
          audit.cpu_exact_aabb_pair_bound_evaluation_count ==
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_point_pair_distance_evaluation_count <=
              maximum_unordered_point_pairs &&
          audit.cpu_strict_component_cutoff_decrease_count <=
              audit.cpu_component_kappa_update_count &&
          envelope_update_counts_close &&
          audit.cpu_component_kappa_update_count <=
              2U *
                  audit.cpu_exact_point_pair_distance_evaluation_count &&
          audit.cpu_node_pair_visit_count ==
              audit.cpu_uniform_same_component_pair_prune_count +
                  audit.cpu_strict_aabb_pair_prune_count +
                  audit.cpu_exact_point_pair_distance_evaluation_count +
                  audit.cpu_node_pair_expansion_count,
      fixture + " closes direct component work and cardinalities");
  check(
      std::string_view{K1BoruvkaComponentDualTreeSearchAudit::proof_basis} ==
          "strict_exact_aabb_pair_bidirectional_component_seed_certified_upper_envelope_bounded_dfs_v5",
      fixture + " locks the certified component-envelope proof basis");
  check(
      audit.frozen_labels_certified &&
          audit.lbvh_topology_and_exact_aabbs_certified &&
          audit.complete_source_seed_coverage_certified &&
          audit.external_seed_targets_recertified &&
          audit.exact_seed_cutoffs_recertified &&
          audit.component_seed_reduction_certified &&
          audit.bidirectional_component_seed_reduction_certified &&
          audit.component_cutoff_upper_envelope_certified &&
          audit.live_component_cutoff_upper_bound_certified &&
          audit.pointwise_at_most_frozen_envelope_certified &&
          audit.maximal_uniform_component_roots_certified ==
              exact_current_envelope &&
          audit.exact_current_component_envelope_certified ==
              exact_current_envelope &&
          audit.deduplicated_mixed_ancestor_refresh_certified ==
              deduplicated_current_envelope &&
          audit.canonical_unordered_pair_partition_certified &&
          audit.uniform_component_pair_prunes_certified &&
          audit.strict_only_aabb_pair_pruning_certified &&
          audit.depth_first_frontier_bound_certified &&
          audit.node_pair_visit_bound_certified &&
          audit.complete_frontier_exhaustion_certified &&
          audit.canonical_kappa_resolution_certified &&
          audit.component_minima_complete,
      fixture + " publishes every direct-component certificate");
}

void test_round_chain_matches_reference() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> points{
      point(0.0), point(1.0), point(10.0), point(12.0),
      point(100.0), point(104.0), point(120.0), point(125.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto reference = build_exact_lbvh_boruvka(index, cloud);
  check(reference.rounds.size() == 3U, "chain exercises three exact rounds");

  std::vector<PointId> labels(cloud.size());
  for (std::size_t point_index = 0U; point_index < labels.size(); ++point_index) {
    labels[point_index] = static_cast<PointId>(point_index);
  }
  std::vector<ExactEmstEdge> accepted;
  K1BoruvkaCandidateContext context{index, cloud};
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  for (std::size_t round_index = 0U;
       round_index < reference.rounds.size();
       ++round_index) {
    const auto& expected = reference.rounds[round_index];
    const auto result = context.resolve_round_exact_external_1nn(
        cloud, std::span<const PointId>{labels}, policy);
    const auto dual_tree =
        context.resolve_round_exact_external_1nn_dual_tree(
            cloud, std::span<const PointId>{labels}, policy);
    const auto component_dual_tree =
        context.resolve_round_exact_component_minima_dual_tree(
            cloud, std::span<const PointId>{labels}, policy);
    check_round(
        result,
        index,
        cloud,
        std::span<const PointId>{labels},
        expected.component_minima,
        policy,
        "chain round " + std::to_string(round_index));
    check_dual_tree_round(
        dual_tree,
        index,
        cloud,
        std::span<const PointId>{labels},
        expected.component_minima,
        policy,
        "dual-tree chain round " + std::to_string(round_index));
    check_component_dual_tree_round(
        component_dual_tree,
        index,
        cloud,
        std::span<const PointId>{labels},
        expected.component_minima,
        policy,
        "component dual-tree chain round " +
            std::to_string(round_index));
    check(
        dual_tree.point_minima == result.point_minima &&
            dual_tree.component_minima == result.component_minima &&
            component_dual_tree.component_minima ==
                result.component_minima,
        "both shared dual-trees agree with the per-source resolver");
    const auto contraction = contract_exact_k1_boruvka_round(
        cloud,
        std::span<const PointId>{labels},
        std::span<const K1BoruvkaComponentMinimum>{
            component_dual_tree.component_minima});
    check(
        contraction.accepted_edges == expected.accepted_edges &&
            contraction.post_round_component_count ==
                expected.post_round_component_count,
        "exact resolver preserves the reference contraction");
    accepted.insert(
        accepted.end(),
        contraction.accepted_edges.begin(),
        contraction.accepted_edges.end());
    labels = contraction.post_round_component_labels;
  }
  std::sort(accepted.begin(), accepted.end(), edge_less);
  check(
      component_count(labels) == 1U && accepted == reference.emst_edges &&
          fake_gpu_k1_boruvka_launch_count() ==
              3U * reference.rounds.size(),
      "all three exact round chains reconstruct the reference EMST");
}

void test_exact_kappa_tie() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> points{
      point(-1.0, -1.0), point(-1.0, 1.0),
      point(1.0, -1.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto reference = build_exact_lbvh_boruvka(index, cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  K1BoruvkaCandidateContext context{index, cloud};
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const auto result = context.resolve_round_exact_external_1nn(
      cloud, std::span<const PointId>{labels}, policy);
  const auto dual_tree =
      context.resolve_round_exact_external_1nn_dual_tree(
          cloud, std::span<const PointId>{labels}, policy);
  const auto component_dual_tree =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud, std::span<const PointId>{labels}, policy);
  check_round(
      result,
      index,
      cloud,
      std::span<const PointId>{labels},
      reference.rounds.front().component_minima,
      policy,
      "square tie");
  check_dual_tree_round(
      dual_tree,
      index,
      cloud,
      std::span<const PointId>{labels},
      reference.rounds.front().component_minima,
      policy,
      "dual-tree square tie");
  check_component_dual_tree_round(
      component_dual_tree,
      index,
      cloud,
      std::span<const PointId>{labels},
      reference.rounds.front().component_minima,
      policy,
      "component dual-tree square tie");
  check(
      result.point_minima[0].outgoing_edge.u == PointId{0} &&
          result.point_minima[0].outgoing_edge.v == PointId{1} &&
          result.point_minima[3].outgoing_edge.u == PointId{1} &&
          result.point_minima[3].outgoing_edge.v == PointId{3},
      "equal distances use canonical endpoint kappa order");
}

void test_component_cutoff_upper_envelope_discriminant() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 5> points{
      point(0.0, 53.0, 45.0),
      point(1.0, 48.0, 41.0),
      point(2.0, 21.0, 4.0),
      point(3.0, 32.0, 46.0),
      point(4.0, 15.0, 14.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 5> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4}};
  K1BoruvkaCandidateContext context{index, cloud};
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const auto per_source = context.resolve_round_exact_external_1nn(
      cloud, std::span<const PointId>{labels}, policy);
  const auto dynamic =
      context.resolve_round_exact_external_1nn_dual_tree(
          cloud, std::span<const PointId>{labels}, policy);
  const auto component_direct =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud, std::span<const PointId>{labels}, policy);
  const auto component_sparse =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud,
          std::span<const PointId>{labels},
          policy,
          K1BoruvkaComponentEnvelopeMode::
              sparse_witness_path_monotone);
  const auto component_current =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud,
          std::span<const PointId>{labels},
          policy,
          K1BoruvkaComponentEnvelopeMode::
              exact_current_maximal_uniform_roots);
  const auto component_deduplicated_current =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud,
          std::span<const PointId>{labels},
          policy,
          K1BoruvkaComponentEnvelopeMode::
              exact_current_deduplicated_mixed_ancestors);

  check_dual_tree_round(
      dynamic,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "dynamic component-cutoff maximum-envelope discriminant");

  check_component_dual_tree_round(
      component_direct,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "component-cutoff maximum-envelope discriminant");
  check_component_dual_tree_round(
      component_sparse,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "sparse component-cutoff maximum-envelope discriminant",
      K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone);
  check_component_dual_tree_round(
      component_current,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "exact-current component-cutoff maximum-envelope discriminant",
      K1BoruvkaComponentEnvelopeMode::
          exact_current_maximal_uniform_roots);
  check_component_dual_tree_round(
      component_deduplicated_current,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "deduplicated exact-current component-cutoff maximum-envelope discriminant",
      K1BoruvkaComponentEnvelopeMode::
          exact_current_deduplicated_mixed_ancestors);
  check(
      component_direct.component_minima.size() == 5U &&
          component_direct.component_minima[3].outgoing_edge.u ==
              PointId{1} &&
          component_direct.component_minima[3].outgoing_edge.v ==
              PointId{3} &&
          component_direct.search_audit.
                  cpu_strict_component_cutoff_decrease_count > 0U,
      "the maximum envelope reaches the non-seed minimum of component 3");
  check(
      component_sparse.component_minima ==
              component_direct.component_minima &&
          component_sparse.morton_seed_audit ==
              component_direct.morton_seed_audit &&
          component_sparse.search_audit.cpu_node_pair_visit_count <=
              component_direct.search_audit.cpu_node_pair_visit_count &&
          component_sparse.search_audit.cpu_node_pair_expansion_count <=
              component_direct.search_audit.cpu_node_pair_expansion_count &&
          component_sparse.search_audit.
                  cpu_exact_aabb_pair_bound_evaluation_count <=
              component_direct.search_audit.
                  cpu_exact_aabb_pair_bound_evaluation_count &&
          component_sparse.search_audit.
                  cpu_exact_point_pair_distance_evaluation_count <=
              component_direct.search_audit.
                  cpu_exact_point_pair_distance_evaluation_count &&
          component_sparse.search_audit.
                  cpu_component_witness_leaf_update_count > 0U,
      "the sparse witness envelope preserves the exact proposal audit and "
      "does no more pair work than the frozen envelope");
  const auto no_more_pair_work = [](const auto& tighter, const auto& looser) {
    return tighter.cpu_node_pair_visit_count <=
               looser.cpu_node_pair_visit_count &&
           tighter.cpu_node_pair_expansion_count <=
               looser.cpu_node_pair_expansion_count &&
           tighter.cpu_exact_aabb_pair_bound_evaluation_count <=
               looser.cpu_exact_aabb_pair_bound_evaluation_count &&
           tighter.cpu_exact_point_pair_distance_evaluation_count <=
               looser.cpu_exact_point_pair_distance_evaluation_count;
  };
  check(
      component_current.component_minima ==
              component_direct.component_minima &&
          component_current.morton_seed_audit ==
              component_direct.morton_seed_audit &&
          component_current.morton_seed_audit ==
              component_sparse.morton_seed_audit &&
          component_current.morton_seed_audit ==
              dynamic.morton_seed_audit &&
          no_more_pair_work(
              component_current.search_audit,
              component_sparse.search_audit) &&
          no_more_pair_work(
              component_current.search_audit,
              component_direct.search_audit) &&
          no_more_pair_work(
              component_current.search_audit,
              dynamic.search_audit) &&
          component_current.search_audit.component_uniform_root_count == 5U &&
          component_current.search_audit.
                  component_uniform_root_leaf_coverage_count == 5U &&
          component_current.search_audit.
                  cpu_component_uniform_root_update_count > 0U,
      "the exact-current component envelope preserves the seed audit and "
      "does no more pair work than sparse, frozen, or dynamic envelopes");
  check(
      component_deduplicated_current.component_minima ==
              component_current.component_minima &&
          component_deduplicated_current.morton_seed_audit ==
              component_current.morton_seed_audit &&
          component_deduplicated_current.search_audit.
                  cpu_node_pair_visit_count ==
              component_current.search_audit.cpu_node_pair_visit_count &&
          component_deduplicated_current.search_audit.
                  cpu_node_pair_expansion_count ==
              component_current.search_audit.cpu_node_pair_expansion_count &&
          component_deduplicated_current.search_audit.
                  cpu_exact_aabb_pair_bound_evaluation_count ==
              component_current.search_audit.
                  cpu_exact_aabb_pair_bound_evaluation_count &&
          component_deduplicated_current.search_audit.
                  cpu_exact_point_pair_distance_evaluation_count ==
              component_current.search_audit.
                  cpu_exact_point_pair_distance_evaluation_count &&
          component_deduplicated_current.search_audit.
                  cpu_component_mixed_ancestor_recomputation_count <=
              component_current.search_audit.
                  cpu_component_mixed_ancestor_recomputation_count &&
          component_deduplicated_current.search_audit.
                  cpu_component_mixed_ancestor_update_count <=
              component_current.search_audit.
                  cpu_component_mixed_ancestor_update_count,
      "the deduplicated current envelope preserves traversal and does not "
      "repeat more mixed-ancestor maintenance");
  check(
      fake_gpu_k1_boruvka_launch_count() == 6U,
      "component cutoff discriminant launches one seed proposal per resolver");
}

void test_canonical_point_id_label_regression() {
  reset_fake_gpu_k1_boruvka();
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  const std::array<CertifiedPoint3, 5> source_points{
      point(0.0, 0.0),
      point(9.0 / 16.0, 9.0 / 16.0),
      point(7.0 / 16.0, 7.0 / 16.0),
      point(1.0, 0.0),
      point(0.0, 1.0)};
  const CanonicalPointCloud cloud = cloud_from(source_points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto leaves = index.leaves();
  check(
      cloud.source_index(PointId{0}) == 0U &&
          cloud.source_index(PointId{1}) == 4U &&
          cloud.source_index(PointId{2}) == 2U &&
          cloud.source_index(PointId{3}) == 1U &&
          cloud.source_index(PointId{4}) == 3U &&
          leaves.size() == 5U && leaves[0].point_id == PointId{0} &&
          leaves[1].point_id == PointId{2} &&
          leaves[2].point_id == PointId{1} &&
          leaves[3].point_id == PointId{4} &&
          leaves[4].point_id == PointId{3},
      "the regression locks PointId canonicalization and Morton order");

  const std::array<PointId, 5> unremapped_labels{
      PointId{0}, PointId{1}, PointId{0}, PointId{0}, PointId{0}};
  K1BoruvkaCandidateContext context{index, cloud};
  const auto unremapped_per_source =
      context.resolve_round_exact_external_1nn(
          cloud, std::span<const PointId>{unremapped_labels}, policy);
  const auto unremapped_direct =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud, std::span<const PointId>{unremapped_labels}, policy);
  const auto unremapped_current =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud,
          std::span<const PointId>{unremapped_labels},
          policy,
          K1BoruvkaComponentEnvelopeMode::
              exact_current_maximal_uniform_roots);

  check_component_dual_tree_round(
      unremapped_direct,
      index,
      cloud,
      std::span<const PointId>{unremapped_labels},
      unremapped_per_source.component_minima,
      policy,
      "unremapped source-order label regression");
  check_component_dual_tree_round(
      unremapped_current,
      index,
      cloud,
      std::span<const PointId>{unremapped_labels},
      unremapped_per_source.component_minima,
      policy,
      "fragmented exact-current component-root regression",
      K1BoruvkaComponentEnvelopeMode::
          exact_current_maximal_uniform_roots);
  check(
      unremapped_direct.search_audit.target_component_seed_offer_count == 5U &&
          unremapped_direct.search_audit.
                  target_component_seed_kappa_update_count == 0U &&
          unremapped_direct.search_audit.
                  target_component_seed_strict_cutoff_decrease_count == 0U,
      "source-order labels do not isolate the intended canonical point");
  check(
      unremapped_current.search_audit.component_uniform_root_count > 2U,
      "the non-singleton component has several maximal uniform roots");

  const std::array<PointId, 5> canonical_labels{
      PointId{0}, PointId{0}, PointId{0}, PointId{3}, PointId{0}};
  const auto canonical_per_source = context.resolve_round_exact_external_1nn(
      cloud, std::span<const PointId>{canonical_labels}, policy);
  const auto canonical_direct =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud, std::span<const PointId>{canonical_labels}, policy);
  check_component_dual_tree_round(
      canonical_direct,
      index,
      cloud,
      std::span<const PointId>{canonical_labels},
      canonical_per_source.component_minima,
      policy,
      "canonical PointId label regression");
  check(
      canonical_direct.component_minima.size() == 2U &&
          canonical_direct.component_minima[1].component_label == PointId{3} &&
          canonical_direct.component_minima[1].source_point_id == PointId{3} &&
          canonical_direct.component_minima[1].outgoing_edge.u == PointId{2} &&
          canonical_direct.component_minima[1].outgoing_edge.v == PointId{3} &&
          canonical_direct.component_minima[1].outgoing_edge.squared_length ==
              ExactLevel{BigInt{1}, BigInt{32}} &&
          canonical_direct.search_audit.target_component_seed_offer_count ==
              5U &&
          canonical_direct.search_audit.
                  target_component_seed_kappa_update_count == 2U &&
          canonical_direct.search_audit.
                  target_component_seed_strict_cutoff_decrease_count == 1U,
      "canonical labels recover the strict target offer 2-to-3 at 1/32");
  check(
      fake_gpu_k1_boruvka_launch_count() == 5U,
      "canonical label regression includes the fragmented current resolver");
}

void test_bidirectional_component_seed_discriminant() {
  reset_fake_gpu_k1_boruvka();
  constexpr K1BoruvkaMortonSeedPolicy policy{1U};
  // Morton order is 1-0-2-3. With W=1, component 2 first retains 3-to-0
  // at d^2=2134; the fallback seed 1-to-2 from component 0 has d^2=14 and
  // its target-oriented offer therefore tightens component 2 strictly.
  const std::array<CertifiedPoint3, 4> points{
      point(0.0, 37.0, 4.0),
      point(1.0, 1.0, 51.0),
      point(2.0, 3.0, 48.0),
      point(3.0, 7.0, 39.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{0}, PointId{2}, PointId{2}};
  K1BoruvkaCandidateContext context{index, cloud};
  const auto per_source = context.resolve_round_exact_external_1nn(
      cloud, std::span<const PointId>{labels}, policy);
  const auto component_direct =
      context.resolve_round_exact_component_minima_dual_tree(
          cloud, std::span<const PointId>{labels}, policy);

  check_component_dual_tree_round(
      component_direct,
      index,
      cloud,
      std::span<const PointId>{labels},
      per_source.component_minima,
      policy,
      "bidirectional component-seed discriminant");
  const auto leaves = index.leaves();
  check(
      leaves.size() == 4U && leaves[0].point_id == PointId{1} &&
          leaves[1].point_id == PointId{0} &&
          leaves[2].point_id == PointId{2} &&
          leaves[3].point_id == PointId{3},
      "the bidirectional discriminant locks Morton order 1-0-2-3");
  check(
      component_direct.component_minima.size() == 2U &&
          component_direct.component_minima[0].outgoing_edge.u == PointId{1} &&
          component_direct.component_minima[0].outgoing_edge.v == PointId{2} &&
          component_direct.component_minima[0].outgoing_edge.squared_length ==
              ExactLevel{BigInt{14}} &&
          component_direct.component_minima[1].outgoing_edge.u == PointId{1} &&
          component_direct.component_minima[1].outgoing_edge.v == PointId{2} &&
          component_direct.component_minima[1].outgoing_edge.squared_length ==
              ExactLevel{BigInt{14}},
      "the target-oriented seed 1-to-2 is feasible for both sides of the cut");
  check(
      component_direct.search_audit.target_component_seed_offer_count == 4U &&
          component_direct.search_audit.
                  target_component_seed_kappa_update_count == 1U &&
          component_direct.search_audit.
                  target_component_seed_strict_cutoff_decrease_count == 1U,
      "bidirectional reduction strictly tightens one component cutoff");
  check(
      fake_gpu_k1_boruvka_launch_count() == 2U,
      "bidirectional discriminant reuses recertified distances");
}

void test_morton_overlap_quadratic_lower_bound() {
  for (const std::size_t scale :
       std::array<std::size_t, 3>{4U, 8U, 16U}) {
    reset_fake_gpu_k1_boruvka();
    const std::size_t guard_pair_count = scale * scale;
    const std::size_t point_count = 4U * guard_pair_count + 2U;
    const std::string fixture =
        "Morton overlap s=" + std::to_string(scale);
    const std::vector<CertifiedPoint3> points =
        morton_overlap_points(scale);
    const CanonicalPointCloud cloud = cloud_from(points);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const auto leaves = index.leaves();
    const auto& build = index.build_counters();

    bool canonical_source_order = cloud.size() == point_count;
    for (std::size_t point_index = 0U;
         canonical_source_order && point_index < cloud.size();
         ++point_index) {
      canonical_source_order =
          cloud.source_index(static_cast<PointId>(point_index)) ==
          point_index;
    }
    bool central_collision_order = leaves.size() == point_count;
    if (central_collision_order) {
      for (std::size_t position = 1U;
           position + 1U < leaves.size();
           ++position) {
        central_collision_order =
            central_collision_order &&
            leaves[position].morton_code == std::uint64_t{7} &&
            leaves[position].point_id == static_cast<PointId>(position);
      }
    }
    constexpr std::uint64_t maximum_morton_code =
        (std::uint64_t{1} << 63U) - std::uint64_t{1};
    check(
        points.size() == point_count && cloud.size() == point_count &&
            canonical_source_order &&
            leaves.size() == point_count &&
            leaves.front().morton_code == std::uint64_t{0} &&
            leaves.front().point_id == PointId{0} &&
            leaves.back().morton_code == maximum_morton_code &&
            leaves.back().point_id ==
                static_cast<PointId>(point_count - 1U) &&
            central_collision_order &&
            build.morton_collision_group_count == 1U &&
            build.maximum_morton_collision_size == point_count - 2U,
        fixture + " forms one canonical n-2 Morton collision block");

    std::vector<PointId> labels(point_count);
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      labels[point_index] = static_cast<PointId>(point_index);
    }
    K1BoruvkaCandidateContext context{index, cloud};
    constexpr K1BoruvkaMortonSeedPolicy policy{1U};
    const auto result = context.resolve_round_exact_external_1nn(
        cloud, std::span<const PointId>{labels}, policy);
    const auto dual_tree =
        context.resolve_round_exact_external_1nn_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            policy);
    if (scale == 4U) {
      const auto reference = build_exact_lbvh_boruvka(index, cloud);
      const auto component_direct =
          context.resolve_round_exact_component_minima_dual_tree(
              cloud,
              std::span<const PointId>{labels},
              policy);
      check(
          !reference.rounds.empty() &&
              result.component_minima ==
                  reference.rounds.front().component_minima &&
              dual_tree.component_minima ==
                  reference.rounds.front().component_minima &&
              component_direct.component_minima ==
                  reference.rounds.front().component_minima,
          fixture + " matches the independent exact Boruvka anchor");
      check_component_dual_tree_round(
          component_direct,
          index,
          cloud,
          std::span<const PointId>{labels},
          reference.rounds.front().component_minima,
          policy,
          fixture + " component-direct traversal");
      check(
          component_direct.search_audit.
                  cpu_strict_component_cutoff_decrease_count > 0U &&
              component_direct.search_audit.
                  cpu_component_kappa_update_count > 0U,
          fixture + " forces component-direct improvements beyond seeds");
    }
    check(
        dual_tree.point_minima == result.point_minima &&
            dual_tree.component_minima == result.component_minima,
        fixture + " shared dual-tree preserves every exact minimum");

    const std::size_t first_query = 2U * guard_pair_count + 1U;
    const std::size_t query_end = 4U * guard_pair_count + 1U;
    const ExactLevel expected_query_squared_length{
        BigInt{1}, BigInt{4U * guard_pair_count}};
    bool query_minima_are_exact =
        result.point_minima.size() == point_count &&
        result.component_minima.size() == point_count;
    for (std::size_t query = first_query;
         query_minima_are_exact && query < query_end;
         ++query) {
      const PointId query_id = static_cast<PointId>(query);
      query_minima_are_exact =
          result.point_minima[query].source_point_id == query_id &&
          result.point_minima[query].outgoing_edge.squared_length ==
              expected_query_squared_length &&
          result.component_minima[query].component_label == query_id &&
          result.component_minima[query].source_point_id == query_id &&
          result.component_minima[query].outgoing_edge ==
              result.point_minima[query].outgoing_edge;
    }
    check(
        query_minima_are_exact,
        fixture + " gives every grid query its exact distance d squared");

    const std::size_t squared_guard_pair_count =
        guard_pair_count * guard_pair_count;
    const std::size_t internal_expansion_lower_bound =
        2U * squared_guard_pair_count;
    const std::size_t node_visit_lower_bound =
        6U * squared_guard_pair_count;
    const auto& audit = result.search_audit;
    const std::size_t seed_distance_upper_bound = 2U * point_count;
    const std::size_t aabb_upper_bound =
        point_count * (2U * point_count - 1U);
    const std::size_t point_distance_upper_bound =
        point_count * (point_count - 1U);
    const std::size_t exact_work =
        result.morton_seed_audit.exact_seed_distance_evaluation_count +
        audit.cpu_exact_aabb_bound_evaluation_count +
        audit.cpu_exact_point_distance_evaluation_count;
    check(
        result.seed_status ==
                K1BoruvkaSeedStatus::
                    bounded_morton_window_external_exact_monotone_certified &&
            result.search_status ==
                K1BoruvkaExactSearchStatus::
                    exact_external_1nn_branch_and_bound_certified &&
            audit.complete_frontier_exhaustion_certified &&
            audit.strict_only_aabb_pruning_certified &&
            audit.canonical_kappa_resolution_certified &&
            fake_gpu_k1_boruvka_launch_count() ==
                (scale == 4U ? 3U : 2U),
        fixture + " keeps proposal and exact decision certificates separate");
    check(
        audit.cpu_internal_node_expansion_count >=
                internal_expansion_lower_bound &&
            audit.cpu_node_visit_count >= node_visit_lower_bound &&
            audit.cpu_exact_aabb_bound_evaluation_count ==
                audit.cpu_node_visit_count &&
            audit.maximum_cpu_node_visit_count_per_source >=
                3U * guard_pair_count,
        fixture + " realizes the certified quadratic work lower bound");
    check(
        result.morton_seed_audit.exact_seed_distance_evaluation_count <=
                seed_distance_upper_bound &&
            audit.cpu_exact_aabb_bound_evaluation_count <=
                aabb_upper_bound &&
            audit.cpu_exact_point_distance_evaluation_count <=
                point_distance_upper_bound &&
            exact_work <= 3U * point_count * point_count,
        fixture + " closes the general three-n-squared round-work bound");

    const auto& dual_audit = dual_tree.search_audit;
    const std::size_t maximum_unordered_point_pairs =
        point_count * (point_count - 1U) / 2U;
    check(
        dual_tree.seed_status ==
                K1BoruvkaSeedStatus::
                    bounded_morton_window_external_exact_monotone_certified &&
            dual_tree.search_status ==
                K1BoruvkaDualTreeSearchStatus::
                    exact_external_1nn_shared_lbvh_dual_tree_certified &&
            dual_audit.dynamic_incumbent_tree_certified &&
            dual_audit.canonical_unordered_pair_partition_certified &&
            dual_audit.strict_only_aabb_pair_pruning_certified &&
            dual_audit.depth_first_frontier_bound_certified &&
            dual_audit.complete_frontier_exhaustion_certified &&
            dual_audit.canonical_kappa_resolution_certified &&
            dual_audit.unordered_point_pair_count ==
                maximum_unordered_point_pairs &&
            dual_audit.covered_unordered_point_pair_count ==
                maximum_unordered_point_pairs &&
            dual_audit.lbvh_maximum_depth ==
                index.build_counters().maximum_depth &&
            dual_audit.certified_depth_first_frontier_bound ==
                2U * index.build_counters().maximum_depth + 1U &&
            dual_audit.maximum_cpu_frontier_size <=
                dual_audit.certified_depth_first_frontier_bound &&
            dual_audit.cpu_strict_incumbent_decrease_count > 0U &&
            dual_audit.cpu_incumbent_ancestor_update_count > 0U &&
            dual_audit.cpu_exact_aabb_pair_bound_evaluation_count ==
                dual_audit.cpu_node_pair_visit_count &&
            dual_audit.cpu_exact_point_pair_distance_evaluation_count <=
                maximum_unordered_point_pairs,
        fixture + " certifies the shared unordered dual-tree frontier");
    check(
        dual_audit.cpu_node_pair_visit_count <=
                48U * guard_pair_count &&
            dual_audit.cpu_node_pair_expansion_count <=
                22U * guard_pair_count &&
            dual_audit.cpu_exact_point_pair_distance_evaluation_count <=
                7U * guard_pair_count &&
            dual_audit.maximum_cpu_frontier_size <=
                12U * guard_pair_count &&
            dual_audit.cpu_strict_incumbent_decrease_count <=
                3U * guard_pair_count &&
            dual_audit.cpu_incumbent_ancestor_update_count <=
                3U * guard_pair_count &&
            dual_audit.cpu_node_pair_visit_count < node_visit_lower_bound,
        fixture + " removes the repeated guard-query quadratic block");
  }
}

void test_invalid_contracts() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> points{
      point(0.0), point(1.0), point(11.0), point(13.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const CanonicalPointCloud other = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  K1BoruvkaCandidateContext context{index, cloud};
  const auto resolve = [&](const CanonicalPointCloud& query_cloud,
                           std::span<const PointId> query_labels,
                           std::size_t radius) {
    static_cast<void>(context.resolve_round_exact_external_1nn(
        query_cloud,
        query_labels,
        K1BoruvkaMortonSeedPolicy{radius}));
  };
  const auto resolve_dual_tree = [&]() {
    static_cast<void>(
        context.resolve_round_exact_external_1nn_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaMortonSeedPolicy{0U}));
  };
  const auto resolve_component_dual_tree = [&]() {
    static_cast<void>(
        context.resolve_round_exact_component_minima_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaMortonSeedPolicy{0U}));
  };
  const auto resolve_component_dual_tree_unspecified = [&]() {
    static_cast<void>(
        context.resolve_round_exact_component_minima_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaMortonSeedPolicy{1U},
            K1BoruvkaComponentEnvelopeMode::unspecified));
  };

  check_throws<std::invalid_argument>(
      [&]() { resolve(cloud, labels, 0U); }, "zero radius is rejected");
  check_throws<std::invalid_argument>(
      resolve_dual_tree, "dual-tree zero radius is rejected");
  check_throws<std::invalid_argument>(
      resolve_component_dual_tree,
      "component-direct dual-tree zero radius is rejected");
  check_throws<std::invalid_argument>(
      resolve_component_dual_tree_unspecified,
      "an unspecified component-direct envelope is rejected");
  check_throws<std::invalid_argument>(
      [&]() { resolve(cloud, std::span<const PointId>{labels.data(), 3U}, 1U); },
      "short labels are rejected");
  const std::array<PointId, 4> noncanonical{
      PointId{1}, PointId{1}, PointId{2}, PointId{3}};
  check_throws<std::invalid_argument>(
      [&]() { resolve(cloud, noncanonical, 1U); },
      "noncanonical labels are rejected");
  const std::array<PointId, 4> terminal{
      PointId{0}, PointId{0}, PointId{0}, PointId{0}};
  check_throws<std::invalid_argument>(
      [&]() { resolve(cloud, terminal, 1U); },
      "terminal partition is rejected");
  check_throws<std::invalid_argument>(
      [&]() { resolve(other, labels, 1U); },
      "foreign cloud identity is rejected");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "invalid contracts fail before the Morton proposal");
  resolve(cloud, labels, 1U);
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U,
      "pre-GPU rejections do not poison the resident context");
}

}  // namespace

int main() {
  test_round_chain_matches_reference();
  test_exact_kappa_tie();
  test_component_cutoff_upper_envelope_discriminant();
  test_canonical_point_id_label_regression();
  test_bidirectional_component_seed_discriminant();
  test_morton_overlap_quadratic_lower_bound();
  test_invalid_contracts();
  if (failures != 0) {
    std::cerr << failures << " exact-search test(s) failed\n";
    return 1;
  }
  std::cout << "GPU K1 Boruvka exact-search tests passed\n";
  return 0;
}
