#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(
                  spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error(
        "a seeded exact K1 Boruvka point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] std::size_t theoretical_maximum_round_count(
    std::size_t point_count) {
  if (point_count <= 1U) {
    return 0U;
  }
  return static_cast<std::size_t>(std::bit_width(point_count - 1U));
}

[[nodiscard]] bool edge_less(
    const hierarchy::ExactEmstEdge& left,
    const hierarchy::ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] bool add_without_overflow(
    std::size_t& target,
    std::size_t value) noexcept {
  if (value > std::numeric_limits<std::size_t>::max() - target) {
    return false;
  }
  target += value;
  return true;
}

[[nodiscard]] bool valid_seed_policy(
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  return policy.window_radius != 0U &&
         policy.window_radius <=
             std::numeric_limits<std::size_t>::max() / 2U;
}

[[nodiscard]] std::optional<std::size_t> expected_morton_inspections(
    std::size_t point_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  if (!valid_seed_policy(policy)) {
    return std::nullopt;
  }
  if (point_count <= 1U) {
    return 0U;
  }
  const std::size_t effective_radius =
      std::min(policy.window_radius, point_count - 1U);
  std::size_t factor = point_count - 1U;
  if (!add_without_overflow(
          factor, point_count - effective_radius) ||
      factor > std::numeric_limits<std::size_t>::max() /
                   effective_radius) {
    return std::nullopt;
  }
  return effective_radius * factor;
}

[[nodiscard]] bool morton_seed_audit_closes(
    const K1BoruvkaMortonSeedAudit& audit,
    K1BoruvkaSeedStatus status,
    std::size_t point_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  const std::optional<std::size_t> expected_inspections =
      expected_morton_inspections(point_count, policy);
  if (point_count <= 1U || !expected_inspections.has_value()) {
    return false;
  }
  const std::size_t inspection_budget = std::min(
      point_count - 1U, 2U * policy.window_radius);
  std::size_t maximum_exact_seed_evaluations = point_count;
  if (!add_without_overflow(
          maximum_exact_seed_evaluations,
          audit.floating_proposal_count)) {
    return false;
  }
  return status ==
             K1BoruvkaSeedStatus::
                 bounded_morton_window_external_exact_monotone_certified &&
         audit.source_count == point_count &&
         audit.window_radius == policy.window_radius &&
         audit.neighbor_inspection_budget_per_source == inspection_budget &&
         audit.maximum_inspected_neighbor_count_per_source <=
             inspection_budget &&
         audit.inspected_neighbor_count == *expected_inspections &&
         audit.external_neighbor_count <= audit.inspected_neighbor_count &&
         audit.floating_proposal_count <= point_count &&
         audit.exact_selected_proposal_count <=
             audit.floating_proposal_count &&
         audit.exact_strict_improvement_count <=
             audit.exact_selected_proposal_count &&
         audit.exact_fallback_count ==
             point_count - audit.exact_selected_proposal_count &&
         audit.exact_seed_distance_evaluation_count >= point_count &&
         audit.exact_seed_distance_evaluation_count <=
             maximum_exact_seed_evaluations &&
         audit.gpu_kernel_launch_count == 1U &&
         audit.gpu_synchronization_count == 1U &&
         audit.complete_source_coverage_certified &&
         audit.bounded_window_certified &&
         audit.external_targets_recertified &&
         audit.exact_monotone_cutoff_certified;
}

[[nodiscard]] bool exact_search_audit_closes(
    const K1BoruvkaExactSearchAudit& audit,
    K1BoruvkaExactSearchStatus status,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t component_count) noexcept {
  if (point_count <= 1U || component_count <= 1U ||
      component_count > point_count ||
      audit.uniform_lbvh_node_count > node_count ||
      audit.mixed_lbvh_node_count !=
          node_count - audit.uniform_lbvh_node_count ||
      audit.cpu_internal_node_expansion_count >
          std::numeric_limits<std::size_t>::max() / 2U ||
      audit.cpu_uniform_component_prune_count >
          2U * audit.cpu_internal_node_expansion_count) {
    return false;
  }
  std::size_t visited_partition = audit.cpu_strict_aabb_prune_count;
  if (!add_without_overflow(
          visited_partition, audit.cpu_internal_node_expansion_count) ||
      !add_without_overflow(
          visited_partition,
          audit.cpu_exact_point_distance_evaluation_count) ||
      !add_without_overflow(
          visited_partition,
          audit.cpu_seed_leaf_distance_reuse_count)) {
    return false;
  }
  return status == K1BoruvkaExactSearchStatus::
                       exact_external_1nn_branch_and_bound_certified &&
         audit.resident_point_count == point_count &&
         audit.resident_node_count == node_count &&
         audit.frozen_component_count == component_count &&
         audit.point_query_count == point_count &&
         audit.seed_incumbent_count == point_count &&
         audit.point_minimum_count == point_count &&
         audit.component_minimum_count == component_count &&
         audit.maximum_cpu_node_visit_count_per_source > 0U &&
         audit.maximum_cpu_node_visit_count_per_source <=
             audit.cpu_node_visit_count &&
         audit.maximum_cpu_exact_point_distance_evaluation_count_per_source <=
             audit.cpu_exact_point_distance_evaluation_count &&
         audit.maximum_cpu_frontier_size_per_source > 0U &&
         audit.maximum_cpu_frontier_size_per_source <= node_count &&
         audit.cpu_exact_aabb_bound_evaluation_count ==
             audit.cpu_node_visit_count &&
         audit.cpu_seed_leaf_distance_reuse_count <= point_count &&
         visited_partition == audit.cpu_node_visit_count &&
         audit.frozen_labels_certified &&
         audit.lbvh_topology_and_exact_aabbs_certified &&
         audit.complete_source_seed_coverage_certified &&
         audit.external_seed_targets_recertified &&
         audit.exact_seed_cutoffs_recertified &&
         audit.uniform_component_prunes_certified &&
         audit.strict_only_aabb_pruning_certified &&
         audit.complete_frontier_exhaustion_certified &&
         audit.canonical_kappa_resolution_certified &&
         audit.point_minima_complete && audit.component_minima_complete;
}

[[nodiscard]] bool round_audits_close(
    const K1SeededExactBoruvkaRound& round,
    std::size_t round_index,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t component_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  const std::size_t post_component_count =
      round.canonical_contraction.post_round_component_count;
  if (component_count <= 1U || post_component_count == 0U ||
      post_component_count > component_count / 2U ||
      round.canonical_contraction.accepted_edges.size() !=
          component_count - post_component_count) {
    return false;
  }
  return morton_seed_audit_closes(
             round.morton_seed_audit,
             round.seed_status,
             point_count,
             policy) &&
         exact_search_audit_closes(
             round.exact_search_audit,
             round.search_status,
             point_count,
             node_count,
             component_count) &&
         round.decision_status ==
             K1HybridBoruvkaDecisionStatus::
                 cpu_exact_kappa_minima_certified &&
         round.exact_decision.round_index == round_index &&
         round.exact_decision.frozen_component_count == component_count &&
         round.exact_decision.component_minima.size() == component_count &&
         round.contraction_status ==
             K1HybridBoruvkaContractionStatus::
                 cpu_exact_canonical_contraction_certified;
}

[[nodiscard]] std::optional<K1SeededExactBoruvkaCounters>
recompute_counters(
    const K1SeededExactBoruvkaResult& result,
    std::size_t node_count) noexcept {
  K1SeededExactBoruvkaCounters counters;
  counters.point_count = result.point_count;
  counters.lbvh_node_count = node_count;
  counters.round_count = result.rounds.size();
  counters.theoretical_max_round_count =
      theoretical_maximum_round_count(result.point_count);
  counters.final_component_count = result.point_count;

  for (const K1SeededExactBoruvkaRound& round : result.rounds) {
    const K1BoruvkaMortonSeedAudit& seed = round.morton_seed_audit;
    const K1BoruvkaExactSearchAudit& search = round.exact_search_audit;
    const std::size_t pre_component_count =
        round.exact_decision.frozen_component_count;
    const std::size_t post_component_count =
        round.canonical_contraction.post_round_component_count;
    if (pre_component_count < post_component_count ||
        !add_without_overflow(
            counters.frozen_component_label_count,
            result.point_count) ||
        !add_without_overflow(
            counters.component_minimum_count,
            round.exact_decision.component_minima.size()) ||
        !add_without_overflow(
            counters.accepted_edge_count,
            round.canonical_contraction.accepted_edges.size()) ||
        !add_without_overflow(
            counters.component_contraction_count,
            pre_component_count - post_component_count) ||
        !add_without_overflow(
            counters.uniform_lbvh_node_tag_count,
            search.uniform_lbvh_node_count) ||
        !add_without_overflow(
            counters.mixed_lbvh_node_tag_count,
            search.mixed_lbvh_node_count) ||
        !add_without_overflow(
            counters.morton_seed_source_count,
            seed.source_count) ||
        !add_without_overflow(
            counters.morton_inspected_neighbor_count,
            seed.inspected_neighbor_count) ||
        !add_without_overflow(
            counters.morton_external_neighbor_count,
            seed.external_neighbor_count) ||
        !add_without_overflow(
            counters.morton_floating_proposal_count,
            seed.floating_proposal_count) ||
        !add_without_overflow(
            counters.morton_exact_selected_proposal_count,
            seed.exact_selected_proposal_count) ||
        !add_without_overflow(
            counters.morton_exact_strict_improvement_count,
            seed.exact_strict_improvement_count) ||
        !add_without_overflow(
            counters.morton_exact_fallback_count,
            seed.exact_fallback_count) ||
        !add_without_overflow(
            counters.morton_exact_seed_distance_evaluation_count,
            seed.exact_seed_distance_evaluation_count) ||
        !add_without_overflow(
            counters.morton_gpu_kernel_launch_count,
            seed.gpu_kernel_launch_count) ||
        !add_without_overflow(
            counters.morton_gpu_synchronization_count,
            seed.gpu_synchronization_count) ||
        !add_without_overflow(
            counters.exact_point_query_count,
            search.point_query_count) ||
        !add_without_overflow(
            counters.exact_seed_incumbent_count,
            search.seed_incumbent_count) ||
        !add_without_overflow(
            counters.exact_point_minimum_count,
            search.point_minimum_count) ||
        !add_without_overflow(
            counters.exact_node_visit_count,
            search.cpu_node_visit_count) ||
        !add_without_overflow(
            counters.exact_internal_node_expansion_count,
            search.cpu_internal_node_expansion_count) ||
        !add_without_overflow(
            counters.exact_aabb_bound_evaluation_count,
            search.cpu_exact_aabb_bound_evaluation_count) ||
        !add_without_overflow(
            counters.exact_point_distance_evaluation_count,
            search.cpu_exact_point_distance_evaluation_count) ||
        !add_without_overflow(
            counters.exact_seed_leaf_distance_reuse_count,
            search.cpu_seed_leaf_distance_reuse_count) ||
        !add_without_overflow(
            counters.exact_uniform_component_prune_count,
            search.cpu_uniform_component_prune_count) ||
        !add_without_overflow(
            counters.exact_strict_aabb_prune_count,
            search.cpu_strict_aabb_prune_count)) {
      return std::nullopt;
    }
    counters.maximum_exact_node_visit_count_per_source = std::max(
        counters.maximum_exact_node_visit_count_per_source,
        search.maximum_cpu_node_visit_count_per_source);
    counters.maximum_exact_point_distance_evaluation_count_per_source =
        std::max(
            counters.
                maximum_exact_point_distance_evaluation_count_per_source,
            search
                .maximum_cpu_exact_point_distance_evaluation_count_per_source);
    counters.maximum_exact_frontier_size_per_source = std::max(
        counters.maximum_exact_frontier_size_per_source,
        search.maximum_cpu_frontier_size_per_source);
    counters.final_component_count = post_component_count;
  }
  return counters;
}

[[nodiscard]] std::pair<exact::ExactLevel, exact::ExactLevel>
exact_weights(std::span<const hierarchy::ExactEmstEdge> edges) {
  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  for (const hierarchy::ExactEmstEdge& edge : edges) {
    total_squared_weight =
        total_squared_weight + edge.squared_length.rational();
    total_hgp_weight =
        total_hgp_weight + edge.merge_level.rational();
  }
  return {
      exact::ExactLevel{std::move(total_squared_weight)},
      exact::ExactLevel{std::move(total_hgp_weight)}};
}

struct FreshReplay {
  bool bounded_seed_chain{false};
  bool exact_search_chain{false};
  bool exact_decision_chain{false};
  bool canonical_contraction_chain{false};
  std::size_t round_count{};
  std::size_t component_minimum_count{};
  std::size_t seed_kernel_launch_count{};
  std::size_t seed_synchronization_count{};
  std::size_t exact_node_visit_count{};
  std::size_t exact_aabb_bound_evaluation_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
};

[[nodiscard]] FreshReplay replay_fresh_chain(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy policy,
    const K1SeededExactBoruvkaResult& result) {
  FreshReplay replay;
  if (result.point_count != cloud.size() || !valid_seed_policy(policy) ||
      result.morton_seed_policy != policy) {
    return replay;
  }

  std::vector<spatial::PointId> labels(result.point_count);
  for (std::size_t point_index = 0U;
       point_index < result.point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = result.point_count;
  bool seed_chain = true;
  bool search_chain = true;
  bool decision_chain = true;
  bool contraction_chain = true;

  try {
    std::optional<K1BoruvkaCandidateContext> context;
    if (component_count > 1U) {
      context.emplace(index, cloud);
    }
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      if (component_count <= 1U || !context.has_value()) {
        seed_chain = false;
        search_chain = false;
        decision_chain = false;
        contraction_chain = false;
        break;
      }
      const K1SeededExactBoruvkaRound& observed =
          result.rounds[round_index];
      if (!morton_seed_audit_closes(
              observed.morton_seed_audit,
              observed.seed_status,
              result.point_count,
              policy)) {
        seed_chain = false;
      }
      if (!exact_search_audit_closes(
              observed.exact_search_audit,
              observed.search_status,
              result.point_count,
              index.build_counters().node_count,
              component_count)) {
        search_chain = false;
      }
      if (observed.decision_status !=
              K1HybridBoruvkaDecisionStatus::
                  cpu_exact_kappa_minima_certified ||
          observed.exact_decision.round_index != round_index ||
          observed.exact_decision.frozen_component_count !=
              component_count ||
          observed.exact_decision.component_minima.size() !=
              component_count) {
        decision_chain = false;
      }
      const std::size_t observed_post_component_count =
          observed.canonical_contraction.post_round_component_count;
      if (observed.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          observed_post_component_count == 0U ||
          observed_post_component_count > component_count / 2U ||
          observed.canonical_contraction.accepted_edges.size() !=
              component_count - observed_post_component_count) {
        contraction_chain = false;
      }

      K1BoruvkaSeededExactRoundResolution resolution =
          context->resolve_round_exact_external_1nn(
              cloud, labels, policy);
      if (resolution.seed_status != observed.seed_status ||
          resolution.morton_seed_audit != observed.morton_seed_audit) {
        seed_chain = false;
      }
      if (resolution.search_status != observed.search_status ||
          resolution.search_audit != observed.exact_search_audit) {
        search_chain = false;
      }
      if (observed.decision_status !=
              K1HybridBoruvkaDecisionStatus::
                  cpu_exact_kappa_minima_certified ||
          observed.exact_decision.round_index != round_index ||
          observed.exact_decision.frozen_component_count !=
              component_count ||
          resolution.component_minima !=
              observed.exact_decision.component_minima) {
        decision_chain = false;
      }

      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud, labels, resolution.component_minima);
      if (observed.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          contraction.accepted_edges !=
              observed.canonical_contraction.accepted_edges ||
          contraction.post_round_component_count !=
              observed.canonical_contraction.post_round_component_count) {
        contraction_chain = false;
      }

      replay.emst_edges.insert(
          replay.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      if (!add_without_overflow(replay.round_count, 1U) ||
          !add_without_overflow(
              replay.component_minimum_count,
              resolution.component_minima.size()) ||
          !add_without_overflow(
              replay.seed_kernel_launch_count,
              resolution.morton_seed_audit.gpu_kernel_launch_count) ||
          !add_without_overflow(
              replay.seed_synchronization_count,
              resolution.morton_seed_audit.gpu_synchronization_count) ||
          !add_without_overflow(
              replay.exact_node_visit_count,
              resolution.search_audit.cpu_node_visit_count) ||
          !add_without_overflow(
              replay.exact_aabb_bound_evaluation_count,
              resolution.search_audit.
                  cpu_exact_aabb_bound_evaluation_count) ||
          !add_without_overflow(
              replay.exact_point_distance_evaluation_count,
              resolution.search_audit.
                  cpu_exact_point_distance_evaluation_count)) {
        seed_chain = false;
        search_chain = false;
        decision_chain = false;
        contraction_chain = false;
        break;
      }
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
    }
  } catch (const std::exception&) {
    return FreshReplay{};
  }

  std::sort(replay.emst_edges.begin(), replay.emst_edges.end(), edge_less);
  const bool completed = component_count == 1U;
  replay.bounded_seed_chain = seed_chain && completed;
  replay.exact_search_chain = search_chain && completed;
  replay.exact_decision_chain = decision_chain && completed;
  replay.canonical_contraction_chain = contraction_chain && completed;
  return replay;
}

[[nodiscard]] std::optional<std::size_t> unordered_point_pair_count(
    std::size_t point_count) noexcept {
  if (point_count == 0U) {
    return std::nullopt;
  }
  std::size_t left = point_count;
  std::size_t right = point_count - 1U;
  if ((left & 1U) == 0U) {
    left /= 2U;
  } else {
    right /= 2U;
  }
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] std::optional<std::size_t> dual_tree_node_pair_visit_bound(
    std::size_t point_count) noexcept {
  const std::optional<std::size_t> unordered_pairs =
      unordered_point_pair_count(point_count);
  if (!unordered_pairs.has_value()) {
    return std::nullopt;
  }
  std::size_t terminal_block_bound = *unordered_pairs;
  if (!add_without_overflow(terminal_block_bound, point_count) ||
      terminal_block_bound >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    return std::nullopt;
  }
  return (terminal_block_bound - 1U) * 2U + 1U;
}

[[nodiscard]] bool component_dual_tree_search_audit_closes(
    const K1BoruvkaComponentDualTreeSearchAudit& audit,
    K1BoruvkaComponentDualTreeSearchStatus status,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t lbvh_maximum_depth,
    std::size_t component_count) noexcept {
  const std::optional<std::size_t> expected_pairs =
      unordered_point_pair_count(point_count);
  const std::optional<std::size_t> expected_node_pair_visit_bound =
      dual_tree_node_pair_visit_bound(point_count);
  if (point_count <= 1U || component_count <= 1U ||
      component_count > point_count || !expected_pairs.has_value() ||
      !expected_node_pair_visit_bound.has_value() ||
      audit.uniform_lbvh_node_count > node_count ||
      audit.mixed_lbvh_node_count !=
          node_count - audit.uniform_lbvh_node_count ||
      lbvh_maximum_depth == 0U ||
      lbvh_maximum_depth >
          (std::numeric_limits<std::size_t>::max() - 1U) / 2U ||
      audit.cpu_exact_point_pair_distance_evaluation_count >
          *expected_pairs) {
    return false;
  }
  const std::size_t expected_frontier_bound =
      lbvh_maximum_depth * 2U + 1U;
  std::size_t visited_partition =
      audit.cpu_uniform_same_component_pair_prune_count;
  if (!add_without_overflow(
          visited_partition, audit.cpu_strict_aabb_pair_prune_count) ||
      !add_without_overflow(
          visited_partition,
          audit.cpu_exact_point_pair_distance_evaluation_count) ||
      !add_without_overflow(
          visited_partition, audit.cpu_node_pair_expansion_count)) {
    return false;
  }
  std::size_t maximum_component_updates =
      audit.cpu_exact_point_pair_distance_evaluation_count;
  if (!add_without_overflow(
          maximum_component_updates,
          audit.cpu_exact_point_pair_distance_evaluation_count)) {
    return false;
  }
  return status == K1BoruvkaComponentDualTreeSearchStatus::
                       exact_component_minima_shared_lbvh_dual_tree_certified &&
         audit.resident_point_count == point_count &&
         audit.resident_node_count == node_count &&
         audit.frozen_component_count == component_count &&
         audit.point_seed_count == point_count &&
         audit.component_seed_incumbent_count == component_count &&
         audit.target_component_seed_offer_count == point_count &&
         audit.target_component_seed_kappa_update_count <= point_count &&
         audit.target_component_seed_strict_cutoff_decrease_count <=
             audit.target_component_seed_kappa_update_count &&
         audit.component_cutoff_upper_envelope_node_count == node_count &&
         audit.component_minimum_count == component_count &&
         audit.unordered_point_pair_count == *expected_pairs &&
         audit.covered_unordered_point_pair_count == *expected_pairs &&
         audit.lbvh_maximum_depth == lbvh_maximum_depth &&
         audit.certified_depth_first_frontier_bound ==
             expected_frontier_bound &&
         audit.certified_node_pair_visit_bound ==
             *expected_node_pair_visit_bound &&
         audit.maximum_cpu_frontier_size > 0U &&
         audit.maximum_cpu_frontier_size <=
             audit.certified_depth_first_frontier_bound &&
         audit.maximum_cpu_frontier_size <=
             audit.cpu_node_pair_visit_count &&
         audit.cpu_exact_aabb_pair_bound_evaluation_count ==
             audit.cpu_node_pair_visit_count &&
         audit.cpu_node_pair_visit_count <=
             audit.certified_node_pair_visit_bound &&
         visited_partition == audit.cpu_node_pair_visit_count &&
         audit.cpu_component_kappa_update_count <=
             maximum_component_updates &&
         audit.cpu_strict_component_cutoff_decrease_count <=
             audit.cpu_component_kappa_update_count &&
         audit.frozen_labels_certified &&
         audit.lbvh_topology_and_exact_aabbs_certified &&
         audit.complete_source_seed_coverage_certified &&
         audit.external_seed_targets_recertified &&
         audit.exact_seed_cutoffs_recertified &&
         audit.component_seed_reduction_certified &&
         audit.bidirectional_component_seed_reduction_certified &&
         audit.component_cutoff_upper_envelope_certified &&
         audit.canonical_unordered_pair_partition_certified &&
         audit.uniform_component_pair_prunes_certified &&
         audit.strict_only_aabb_pair_pruning_certified &&
         audit.depth_first_frontier_bound_certified &&
         audit.node_pair_visit_bound_certified &&
         audit.complete_frontier_exhaustion_certified &&
         audit.canonical_kappa_resolution_certified &&
         audit.component_minima_complete;
}

[[nodiscard]] bool dual_tree_round_audits_close(
    const K1DualTreeExactBoruvkaRound& round,
    std::size_t round_index,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t lbvh_maximum_depth,
    std::size_t component_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  const std::size_t post_component_count =
      round.canonical_contraction.post_round_component_count;
  if (component_count <= 1U || post_component_count == 0U ||
      post_component_count > component_count / 2U ||
      round.canonical_contraction.accepted_edges.size() !=
          component_count - post_component_count) {
    return false;
  }
  return morton_seed_audit_closes(
             round.morton_seed_audit,
             round.seed_status,
             point_count,
             policy) &&
         component_dual_tree_search_audit_closes(
             round.dual_tree_search_audit,
             round.search_status,
             point_count,
             node_count,
             lbvh_maximum_depth,
             component_count) &&
         round.decision_status ==
             K1HybridBoruvkaDecisionStatus::
                 cpu_exact_kappa_minima_certified &&
         round.exact_decision.round_index == round_index &&
         round.exact_decision.frozen_component_count == component_count &&
         round.exact_decision.component_minima.size() == component_count &&
         round.contraction_status ==
             K1HybridBoruvkaContractionStatus::
                 cpu_exact_canonical_contraction_certified;
}

struct DualTreeFreshReplay {
  bool bounded_seed_chain{false};
  bool exact_search_chain{false};
  bool exact_decision_chain{false};
  bool canonical_contraction_chain{false};
  std::size_t round_count{};
  std::size_t component_minimum_count{};
  std::size_t seed_kernel_launch_count{};
  std::size_t seed_synchronization_count{};
  std::size_t node_pair_visit_count{};
  std::size_t aabb_pair_bound_evaluation_count{};
  std::size_t point_pair_distance_evaluation_count{};
  std::vector<hierarchy::ExactEmstEdge> emst_edges;
};

[[nodiscard]] DualTreeFreshReplay replay_fresh_dual_tree_chain(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy policy,
    const K1DualTreeExactBoruvkaResult& result) {
  DualTreeFreshReplay replay;
  if (result.point_count != cloud.size() || !valid_seed_policy(policy) ||
      result.morton_seed_policy != policy) {
    return replay;
  }

  std::vector<spatial::PointId> labels(result.point_count);
  for (std::size_t point_index = 0U;
       point_index < result.point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = result.point_count;
  bool seed_chain = true;
  bool search_chain = true;
  bool decision_chain = true;
  bool contraction_chain = true;

  try {
    std::optional<K1BoruvkaCandidateContext> context;
    if (component_count > 1U) {
      context.emplace(index, cloud);
    }
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      if (component_count <= 1U || !context.has_value()) {
        seed_chain = false;
        search_chain = false;
        decision_chain = false;
        contraction_chain = false;
        break;
      }
      const K1DualTreeExactBoruvkaRound& observed =
          result.rounds[round_index];
      if (!morton_seed_audit_closes(
              observed.morton_seed_audit,
              observed.seed_status,
              result.point_count,
              policy)) {
        seed_chain = false;
      }
      if (!component_dual_tree_search_audit_closes(
              observed.dual_tree_search_audit,
              observed.search_status,
              result.point_count,
              index.build_counters().node_count,
              index.build_counters().maximum_depth,
              component_count)) {
        search_chain = false;
      }
      if (observed.exact_decision.round_index != round_index ||
          observed.exact_decision.frozen_component_count !=
              component_count ||
          observed.exact_decision.component_minima.size() !=
              component_count ||
          observed.decision_status !=
              K1HybridBoruvkaDecisionStatus::
                  cpu_exact_kappa_minima_certified) {
        decision_chain = false;
      }

      K1BoruvkaSeededComponentDualTreeRoundResolution resolution =
          context->resolve_round_exact_component_minima_dual_tree(
              cloud, labels, policy);
      if (resolution.seed_status != observed.seed_status ||
          resolution.morton_seed_audit != observed.morton_seed_audit) {
        seed_chain = false;
      }
      if (resolution.search_status != observed.search_status ||
          resolution.search_audit != observed.dual_tree_search_audit) {
        search_chain = false;
      }
      if (resolution.component_minima !=
          observed.exact_decision.component_minima) {
        decision_chain = false;
      }

      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud, labels, resolution.component_minima);
      if (observed.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          contraction.accepted_edges !=
              observed.canonical_contraction.accepted_edges ||
          contraction.post_round_component_count !=
              observed.canonical_contraction.post_round_component_count) {
        contraction_chain = false;
      }

      replay.emst_edges.insert(
          replay.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      if (!add_without_overflow(replay.round_count, 1U) ||
          !add_without_overflow(
              replay.component_minimum_count,
              resolution.component_minima.size()) ||
          !add_without_overflow(
              replay.seed_kernel_launch_count,
              resolution.morton_seed_audit.gpu_kernel_launch_count) ||
          !add_without_overflow(
              replay.seed_synchronization_count,
              resolution.morton_seed_audit.gpu_synchronization_count) ||
          !add_without_overflow(
              replay.node_pair_visit_count,
              resolution.search_audit.cpu_node_pair_visit_count) ||
          !add_without_overflow(
              replay.aabb_pair_bound_evaluation_count,
              resolution.search_audit.
                  cpu_exact_aabb_pair_bound_evaluation_count) ||
          !add_without_overflow(
              replay.point_pair_distance_evaluation_count,
              resolution.search_audit.
                  cpu_exact_point_pair_distance_evaluation_count)) {
        return DualTreeFreshReplay{};
      }
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
    }
  } catch (const std::exception&) {
    return DualTreeFreshReplay{};
  }

  std::sort(replay.emst_edges.begin(), replay.emst_edges.end(), edge_less);
  const bool completed = component_count == 1U;
  replay.bounded_seed_chain = seed_chain && completed;
  replay.exact_search_chain = search_chain && completed;
  replay.exact_decision_chain = decision_chain && completed;
  replay.canonical_contraction_chain = contraction_chain && completed;
  return replay;
}

}  // namespace

K1SeededExactBoruvkaResult
build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "seeded exact K1 Boruvka requires a matching ready LBVH");
  }
  if (!valid_seed_policy(seed_policy)) {
    throw std::invalid_argument(
        "seeded exact K1 Boruvka requires a finite nonzero Morton radius");
  }

  const std::size_t point_count = cloud.size();
  K1SeededExactBoruvkaResult result;
  result.point_count = point_count;
  result.morton_seed_policy = seed_policy;
  std::vector<spatial::PointId> labels(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = point_count;
  const std::size_t maximum_round_count =
      theoretical_maximum_round_count(point_count);

  if (component_count > 1U) {
    K1BoruvkaCandidateContext context{index, cloud};
    while (component_count > 1U) {
      const std::size_t round_index = result.rounds.size();
      if (round_index >= maximum_round_count) {
        throw std::logic_error(
            "seeded exact K1 Boruvka exceeded ceil(log2(n)) rounds");
      }
      K1BoruvkaSeededExactRoundResolution resolution =
          context.resolve_round_exact_external_1nn(
              cloud, labels, seed_policy);
      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud, labels, resolution.component_minima);

      result.emst_edges.insert(
          result.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      K1SeededExactBoruvkaRound round;
      round.morton_seed_audit = resolution.morton_seed_audit;
      round.exact_search_audit = resolution.search_audit;
      round.exact_decision.round_index = round_index;
      round.exact_decision.frozen_component_count = component_count;
      round.exact_decision.component_minima =
          std::move(resolution.component_minima);
      round.canonical_contraction.accepted_edges =
          std::move(contraction.accepted_edges);
      round.canonical_contraction.post_round_component_count =
          contraction.post_round_component_count;
      round.seed_status = resolution.seed_status;
      round.search_status = resolution.search_status;
      round.decision_status = K1HybridBoruvkaDecisionStatus::
          cpu_exact_kappa_minima_certified;
      round.contraction_status = K1HybridBoruvkaContractionStatus::
          cpu_exact_canonical_contraction_certified;
      if (!round_audits_close(
              round,
              round_index,
              point_count,
              index.build_counters().node_count,
              component_count,
              seed_policy)) {
        throw std::logic_error(
            "a seeded exact K1 Boruvka round failed its local certificates");
      }
      result.rounds.push_back(std::move(round));
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
      // Per-source minima and all search-frontier storage die here.
    }
  }

  std::sort(result.emst_edges.begin(), result.emst_edges.end(), edge_less);
  if (result.emst_edges.size() != point_count - 1U ||
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) !=
          result.emst_edges.end()) {
    throw std::logic_error(
        "the seeded exact K1 Boruvka witness is not a canonical tree");
  }
  auto [total_squared_weight, total_hgp_weight] =
      exact_weights(result.emst_edges);
  result.total_squared_weight = std::move(total_squared_weight);
  result.total_hgp_weight = std::move(total_hgp_weight);

  const std::optional<K1SeededExactBoruvkaCounters> counters =
      recompute_counters(result, index.build_counters().node_count);
  if (!counters.has_value() || counters->final_component_count != 1U ||
      counters->round_count > counters->theoretical_max_round_count ||
      counters->accepted_edge_count != point_count - 1U ||
      counters->component_contraction_count != point_count - 1U) {
    throw std::logic_error(
        "the seeded exact K1 Boruvka counters do not close");
  }
  result.counters = *counters;

  const K1SeededExactBoruvkaVerification verification =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, seed_policy, result);
  if (!verification.emst_witness_certified) {
    throw std::logic_error(
        "the seeded exact K1 Boruvka witness failed its fresh replay");
  }
  result.bounded_morton_seed_chain_certified =
      verification.bounded_morton_seed_chain_certified;
  result.exact_external_1nn_chain_certified =
      verification.exact_external_1nn_chain_certified;
  result.cpu_exact_decision_chain_certified =
      verification.cpu_exact_decision_chain_certified;
  result.canonical_contraction_chain_certified =
      verification.canonical_contractions_certified;
  result.fresh_replay_certified = verification.fresh_replay_certified;
  result.reference_cpu_witness_certified =
      verification.reference_cpu_witness_certified;
  result.emst_witness_certified = true;
  return result;
}

K1SeededExactBoruvkaVerification
verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& result) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "seeded exact K1 Boruvka verification requires a matching ready LBVH");
  }
  if (!valid_seed_policy(trusted_seed_policy)) {
    throw std::invalid_argument(
        "seeded exact K1 Boruvka verification requires a trusted finite radius");
  }

  const hierarchy::K1ExactBoruvkaResult reference =
      hierarchy::build_exact_lbvh_boruvka(index, cloud);
  K1SeededExactBoruvkaVerification verification;
  verification.index_identity_certified = true;
  verification.reference_round_count = reference.rounds.size();
  verification.reference_component_minimum_count =
      reference.counters.component_minimum_count;
  verification.trusted_seed_policy_certified =
      result.point_count == cloud.size() &&
      result.morton_seed_policy == trusted_seed_policy;

  FreshReplay replay;
  if (verification.trusted_seed_policy_certified) {
    replay = replay_fresh_chain(
        index, cloud, trusted_seed_policy, result);
  }
  verification.bounded_morton_seed_chain_certified =
      verification.trusted_seed_policy_certified &&
      replay.bounded_seed_chain;
  verification.exact_external_1nn_chain_certified =
      verification.trusted_seed_policy_certified &&
      replay.exact_search_chain;
  verification.replayed_round_count = replay.round_count;
  verification.replayed_component_minimum_count =
      replay.component_minimum_count;
  verification.replayed_seed_kernel_launch_count =
      replay.seed_kernel_launch_count;
  verification.replayed_seed_synchronization_count =
      replay.seed_synchronization_count;
  verification.replayed_exact_node_visit_count =
      replay.exact_node_visit_count;
  verification.replayed_exact_aabb_bound_evaluation_count =
      replay.exact_aabb_bound_evaluation_count;
  verification.replayed_exact_point_distance_evaluation_count =
      replay.exact_point_distance_evaluation_count;

  const bool reference_shape =
      result.point_count == reference.point_count &&
      result.rounds.size() == reference.rounds.size();
  bool exact_decisions = reference_shape;
  bool reference_contractions = reference_shape;
  if (reference_shape) {
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      const K1SeededExactBoruvkaRound& observed =
          result.rounds[round_index];
      const hierarchy::K1BoruvkaRound& expected =
          reference.rounds[round_index];
      if (observed.decision_status !=
              K1HybridBoruvkaDecisionStatus::
                  cpu_exact_kappa_minima_certified ||
          observed.exact_decision.round_index != expected.round_index ||
          observed.exact_decision.frozen_component_count !=
              expected.pre_round_component_count ||
          observed.exact_decision.component_minima !=
              expected.component_minima) {
        exact_decisions = false;
      }
      if (observed.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          observed.canonical_contraction.accepted_edges !=
              expected.accepted_edges ||
          observed.canonical_contraction.post_round_component_count !=
              expected.post_round_component_count) {
        reference_contractions = false;
      }
    }
  }
  verification.cpu_exact_decision_chain_certified =
      exact_decisions && replay.exact_decision_chain;
  verification.canonical_contractions_certified =
      reference_contractions && replay.canonical_contraction_chain;
  verification.fresh_replay_certified =
      replay.bounded_seed_chain && replay.exact_search_chain &&
      replay.exact_decision_chain && replay.canonical_contraction_chain;
  verification.round_count_bound_certified =
      result.point_count == cloud.size() &&
      result.rounds.size() <=
          theoretical_maximum_round_count(result.point_count);

  std::vector<hierarchy::ExactEmstEdge> reconstructed_edges;
  for (const K1SeededExactBoruvkaRound& round : result.rounds) {
    reconstructed_edges.insert(
        reconstructed_edges.end(),
        round.canonical_contraction.accepted_edges.begin(),
        round.canonical_contraction.accepted_edges.end());
  }
  std::sort(
      reconstructed_edges.begin(), reconstructed_edges.end(), edge_less);
  verification.spanning_tree_certified =
      reconstructed_edges == result.emst_edges &&
      replay.emst_edges == result.emst_edges &&
      result.emst_edges == reference.emst_edges &&
      result.emst_edges.size() == result.point_count - 1U &&
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) ==
          result.emst_edges.end();

  const auto [total_squared_weight, total_hgp_weight] =
      exact_weights(result.emst_edges);
  verification.exact_weights_certified =
      result.total_squared_weight == total_squared_weight &&
      result.total_hgp_weight == total_hgp_weight &&
      result.total_squared_weight == reference.total_squared_weight &&
      result.total_hgp_weight == reference.total_hgp_weight;
  verification.reference_cpu_witness_certified =
      reference.emst_witness_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.canonical_contractions_certified &&
      verification.fresh_replay_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified;

  const std::optional<K1SeededExactBoruvkaCounters> recomputed =
      recompute_counters(result, reference.counters.lbvh_node_count);
  verification.counters_certified =
      recomputed.has_value() && *recomputed == result.counters &&
      result.point_count != 0U &&
      recomputed->final_component_count == 1U &&
      recomputed->accepted_edge_count == result.point_count - 1U &&
      recomputed->component_contraction_count == result.point_count - 1U;
  verification.hierarchy_status_separation_certified =
      result.hierarchy_reduction_status ==
          K1HybridHierarchyReductionStatus::not_performed &&
      result.scientific_status ==
          K1HybridScientificStatus::local_emst_witness_only;
  verification.emst_witness_certified =
      verification.index_identity_certified &&
      verification.trusted_seed_policy_certified &&
      verification.bounded_morton_seed_chain_certified &&
      verification.exact_external_1nn_chain_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.canonical_contractions_certified &&
      verification.round_count_bound_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified &&
      verification.reference_cpu_witness_certified &&
      verification.counters_certified &&
      verification.hierarchy_status_separation_certified;
  return verification;
}

K1DualTreeExactBoruvkaResult
build_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "dual-tree exact K1 Boruvka requires a matching ready LBVH");
  }
  if (!valid_seed_policy(seed_policy)) {
    throw std::invalid_argument(
        "dual-tree exact K1 Boruvka requires a finite nonzero Morton radius");
  }

  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "dual-tree exact K1 Boruvka requires a nonempty cloud");
  }
  K1DualTreeExactBoruvkaResult result;
  result.point_count = point_count;
  result.morton_seed_policy = seed_policy;
  std::vector<spatial::PointId> labels(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = point_count;
  const std::size_t maximum_round_count =
      theoretical_maximum_round_count(point_count);

  if (component_count > 1U) {
    K1BoruvkaCandidateContext context{index, cloud};
    while (component_count > 1U) {
      const std::size_t round_index = result.rounds.size();
      if (round_index >= maximum_round_count) {
        throw std::logic_error(
            "dual-tree exact K1 Boruvka exceeded ceil(log2(n)) rounds");
      }
      K1BoruvkaSeededComponentDualTreeRoundResolution resolution =
          context.resolve_round_exact_component_minima_dual_tree(
              cloud, labels, seed_policy);
      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud, labels, resolution.component_minima);

      result.emst_edges.insert(
          result.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      K1DualTreeExactBoruvkaRound round;
      round.morton_seed_audit = std::move(resolution.morton_seed_audit);
      round.dual_tree_search_audit = std::move(resolution.search_audit);
      round.exact_decision.round_index = round_index;
      round.exact_decision.frozen_component_count = component_count;
      round.exact_decision.component_minima =
          std::move(resolution.component_minima);
      round.canonical_contraction.accepted_edges =
          std::move(contraction.accepted_edges);
      round.canonical_contraction.post_round_component_count =
          contraction.post_round_component_count;
      round.seed_status = resolution.seed_status;
      round.search_status = resolution.search_status;
      round.decision_status = K1HybridBoruvkaDecisionStatus::
          cpu_exact_kappa_minima_certified;
      round.contraction_status = K1HybridBoruvkaContractionStatus::
          cpu_exact_canonical_contraction_certified;
      if (!dual_tree_round_audits_close(
              round,
              round_index,
              point_count,
              index.build_counters().node_count,
              index.build_counters().maximum_depth,
              component_count,
              seed_policy)) {
        throw std::logic_error(
            "a dual-tree exact K1 Boruvka round failed its local certificates");
      }
      result.rounds.push_back(std::move(round));
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
      // The frozen component envelope and complete pair frontier die here.
    }
  }

  std::sort(result.emst_edges.begin(), result.emst_edges.end(), edge_less);
  if (result.emst_edges.size() != point_count - 1U ||
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) !=
          result.emst_edges.end()) {
    throw std::logic_error(
        "the dual-tree exact K1 Boruvka witness is not a canonical tree");
  }
  auto [total_squared_weight, total_hgp_weight] =
      exact_weights(result.emst_edges);
  result.total_squared_weight = std::move(total_squared_weight);
  result.total_hgp_weight = std::move(total_hgp_weight);

  const K1DualTreeExactBoruvkaVerification verification =
      verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
          index, cloud, seed_policy, result);
  if (!verification.emst_witness_certified) {
    throw std::logic_error(
        "the dual-tree exact K1 Boruvka witness failed its fresh replay");
  }
  result.bounded_morton_seed_chain_certified =
      verification.bounded_morton_seed_chain_certified;
  result.exact_dual_tree_chain_certified =
      verification.exact_dual_tree_chain_certified;
  result.component_minima_only_persistence_certified =
      verification.component_minima_only_persistence_certified;
  result.cpu_exact_decision_chain_certified =
      verification.cpu_exact_decision_chain_certified;
  result.canonical_contraction_chain_certified =
      verification.canonical_contractions_certified;
  result.fresh_replay_certified = verification.fresh_replay_certified;
  result.reference_cpu_witness_certified =
      verification.reference_cpu_witness_certified;
  result.emst_witness_certified = true;
  return result;
}

K1DualTreeExactBoruvkaVerification
verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1DualTreeExactBoruvkaResult& result) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "dual-tree exact K1 Boruvka verification requires a matching ready LBVH");
  }
  if (!valid_seed_policy(trusted_seed_policy)) {
    throw std::invalid_argument(
        "dual-tree exact K1 Boruvka verification requires a trusted finite radius");
  }

  const hierarchy::K1ExactBoruvkaResult reference =
      hierarchy::build_exact_lbvh_boruvka(index, cloud);
  K1DualTreeExactBoruvkaVerification verification;
  verification.index_identity_certified = true;
  verification.reference_round_count = reference.rounds.size();
  verification.reference_component_minimum_count =
      reference.counters.component_minimum_count;
  verification.trusted_seed_policy_certified =
      result.point_count == cloud.size() &&
      result.morton_seed_policy == trusted_seed_policy;

  DualTreeFreshReplay replay;
  if (verification.trusted_seed_policy_certified) {
    replay = replay_fresh_dual_tree_chain(
        index, cloud, trusted_seed_policy, result);
  }
  verification.bounded_morton_seed_chain_certified =
      verification.trusted_seed_policy_certified &&
      replay.bounded_seed_chain;
  verification.exact_dual_tree_chain_certified =
      verification.trusted_seed_policy_certified &&
      replay.exact_search_chain;
  verification.replayed_round_count = replay.round_count;
  verification.replayed_component_minimum_count =
      replay.component_minimum_count;
  verification.replayed_seed_kernel_launch_count =
      replay.seed_kernel_launch_count;
  verification.replayed_seed_synchronization_count =
      replay.seed_synchronization_count;
  verification.replayed_node_pair_visit_count =
      replay.node_pair_visit_count;
  verification.replayed_aabb_pair_bound_evaluation_count =
      replay.aabb_pair_bound_evaluation_count;
  verification.replayed_point_pair_distance_evaluation_count =
      replay.point_pair_distance_evaluation_count;

  bool exact_decisions =
      result.point_count == reference.point_count &&
      result.rounds.size() == reference.rounds.size();
  bool reference_contractions = exact_decisions;
  if (exact_decisions) {
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      const K1DualTreeExactBoruvkaRound& observed =
          result.rounds[round_index];
      const hierarchy::K1BoruvkaRound& expected =
          reference.rounds[round_index];
      if (observed.decision_status !=
              K1HybridBoruvkaDecisionStatus::
                  cpu_exact_kappa_minima_certified ||
          observed.exact_decision.round_index != expected.round_index ||
          observed.exact_decision.frozen_component_count !=
              expected.pre_round_component_count ||
          observed.exact_decision.component_minima !=
              expected.component_minima) {
        exact_decisions = false;
      }
      if (observed.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          observed.canonical_contraction.accepted_edges !=
              expected.accepted_edges ||
          observed.canonical_contraction.post_round_component_count !=
              expected.post_round_component_count) {
        reference_contractions = false;
      }
    }
  }
  verification.component_minima_only_persistence_certified =
      exact_decisions;
  verification.cpu_exact_decision_chain_certified =
      exact_decisions && replay.exact_decision_chain;
  verification.canonical_contractions_certified =
      reference_contractions && replay.canonical_contraction_chain;
  verification.fresh_replay_certified =
      replay.bounded_seed_chain && replay.exact_search_chain &&
      replay.exact_decision_chain && replay.canonical_contraction_chain;
  verification.round_count_bound_certified =
      result.point_count == cloud.size() &&
      result.rounds.size() <=
          theoretical_maximum_round_count(result.point_count);

  std::vector<hierarchy::ExactEmstEdge> reconstructed_edges;
  for (const K1DualTreeExactBoruvkaRound& round : result.rounds) {
    reconstructed_edges.insert(
        reconstructed_edges.end(),
        round.canonical_contraction.accepted_edges.begin(),
        round.canonical_contraction.accepted_edges.end());
  }
  std::sort(
      reconstructed_edges.begin(), reconstructed_edges.end(), edge_less);
  verification.spanning_tree_certified =
      result.point_count != 0U &&
      reconstructed_edges == result.emst_edges &&
      replay.emst_edges == result.emst_edges &&
      result.emst_edges == reference.emst_edges &&
      result.emst_edges.size() == result.point_count - 1U &&
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) ==
          result.emst_edges.end();

  const auto [total_squared_weight, total_hgp_weight] =
      exact_weights(result.emst_edges);
  verification.exact_weights_certified =
      result.total_squared_weight == total_squared_weight &&
      result.total_hgp_weight == total_hgp_weight &&
      result.total_squared_weight == reference.total_squared_weight &&
      result.total_hgp_weight == reference.total_hgp_weight;
  verification.reference_cpu_witness_certified =
      reference.emst_witness_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.canonical_contractions_certified &&
      verification.fresh_replay_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified;
  verification.hierarchy_status_separation_certified =
      result.hierarchy_reduction_status ==
          K1HybridHierarchyReductionStatus::not_performed &&
      result.scientific_status ==
          K1HybridScientificStatus::local_emst_witness_only;
  verification.emst_witness_certified =
      verification.index_identity_certified &&
      verification.trusted_seed_policy_certified &&
      verification.bounded_morton_seed_chain_certified &&
      verification.exact_dual_tree_chain_certified &&
      verification.component_minima_only_persistence_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.canonical_contractions_certified &&
      verification.fresh_replay_certified &&
      verification.round_count_bound_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified &&
      verification.reference_cpu_witness_certified &&
      verification.hierarchy_status_separation_certified;
  return verification;
}

}  // namespace morsehgp3d::gpu
