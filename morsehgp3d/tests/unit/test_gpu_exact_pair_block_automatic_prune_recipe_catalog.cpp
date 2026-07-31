#include "morsehgp3d/gpu/exact_pair_block_automatic_prune_recipe_catalog.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockAutomaticPruneRecipeCatalogBudget;
using morsehgp3d::gpu::ExactPairBlockAutomaticPruneRecipeCatalogDecision;
using morsehgp3d::gpu::ExactPairBlockAutomaticPruneRecipeCatalogResult;
using morsehgp3d::gpu::build_exact_pair_block_automatic_prune_recipe_catalog;
using morsehgp3d::hierarchy::ExactBlockRankPruneStatus;
using morsehgp3d::hierarchy::certify_exact_block_rank_prune_receipt;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_line_cloud(std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(point_index), 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_eight_clusters_cloud() {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<CertifiedPoint3> points;
  points.reserve(12U);
  for (std::size_t point_index = 0U; point_index < 12U; ++point_index) {
    const std::size_t cluster = point_index % 8U;
    const std::size_t local = point_index / 8U + 1U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const std::size_t permuted_y =
        (local * 40503U + cluster * 7919U) % 65536U;
    const std::size_t permuted_z =
        (local * 25717U + cluster * 104729U) % 65536U;
    points.push_back(CertifiedPoint3::from_binary64(
        center_x + static_cast<double>(local) * local_scale,
        center_y + static_cast<double>(permuted_y) * transverse_scale,
        center_z + static_cast<double>(permuted_z) * transverse_scale));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] std::size_t unordered_pair_count(std::size_t point_count) {
  return point_count < 2U
      ? 0U
      : (point_count * (point_count - 1U)) / 2U;
}

[[nodiscard]] ExactPairBlockAutomaticPruneRecipeCatalogBudget
generous_budget(std::size_t point_count) {
  const std::size_t pair_count = unordered_pair_count(point_count);
  return ExactPairBlockAutomaticPruneRecipeCatalogBudget{
      100000U,
      1024U,
      pair_count,
      pair_count};
}

void verify_complete_catalog(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    const ExactPairBlockAutomaticPruneRecipeCatalogResult& result) {
  const auto& audit = result.audit;
  require(result.complete(), "the exhaustive small-n catalog did not close");
  require(
      audit.product_visit_count ==
              audit.diagonal_product_visit_count +
                  audit.cross_product_visit_count &&
          audit.automatic_receipt_attempt_count ==
              audit.cross_product_visit_count &&
          audit.automatic_receipt_attempt_count ==
              audit.automatic_receipt_certified_count +
                  audit.automatic_receipt_inconclusive_count &&
          audit.diagonal_split_count ==
              audit.diagonal_product_visit_count &&
          audit.automatic_receipt_inconclusive_count ==
              audit.cross_split_count + audit.terminal_pair_count &&
          audit.prune_recipe_count == result.recipes.size(),
      "the catalog traversal counters do not form exact partitions");

  const std::size_t point_count = cloud.size();
  std::vector<unsigned char> pruned_pairs(
      point_count * point_count, static_cast<unsigned char>(0U));
  std::size_t recertified_pruned_mass = 0U;
  for (const auto& recipe : result.recipes) {
    require(
        recipe.source.kind == ExactPairBlockAuthorityKind::cross &&
            recipe.source.first.leaf_begin < recipe.source.first.leaf_end &&
            recipe.source.first.leaf_end <=
                recipe.source.second.leaf_begin &&
            recipe.source.second.leaf_begin <
                recipe.source.second.leaf_end &&
            recipe.source.second.leaf_end <= point_count &&
            recipe.witness_node_count != 0U &&
            recipe.witness_node_count <= recipe.witness_node_indices.size(),
        "a generated recipe has malformed native authority");
    const auto replay = certify_exact_block_rank_prune_receipt(
        index,
        cloud,
        recipe.source.first.node_index,
        recipe.source.second.node_index,
        std::span<const std::size_t>{
            recipe.witness_node_indices.data(), recipe.witness_node_count},
        maximum_closed_rank);
    require(
        replay.status() == ExactBlockRankPruneStatus::certified_above_rank &&
            replay.certified() && replay.receipt() != nullptr &&
            replay.receipt()->certifies(
                index,
                cloud,
                recipe.source.first.node_index,
                recipe.source.second.node_index,
                maximum_closed_rank) &&
            replay.receipt()->unordered_pair_mass() ==
                recipe.source.unordered_pair_mass,
        "a generated recipe failed independent exact recertification");

    std::size_t observed_recipe_mass = 0U;
    for (std::size_t first = recipe.source.first.leaf_begin;
         first < recipe.source.first.leaf_end;
         ++first) {
      for (std::size_t second = recipe.source.second.leaf_begin;
           second < recipe.source.second.leaf_end;
           ++second) {
        const std::size_t pair_offset = first * point_count + second;
        require(
            pruned_pairs[pair_offset] == 0U,
            "generated recipe support products overlap");
        pruned_pairs[pair_offset] = static_cast<unsigned char>(1U);
        ++observed_recipe_mass;
      }
    }
    require(
        observed_recipe_mass == recipe.source.unordered_pair_mass,
        "a generated recipe lost Cartesian product mass");
    recertified_pruned_mass += observed_recipe_mass;
  }

  std::size_t uncovered_pair_count = 0U;
  for (std::size_t first = 0U; first < point_count; ++first) {
    for (std::size_t second = first + 1U;
         second < point_count;
         ++second) {
      if (pruned_pairs[first * point_count + second] == 0U) {
        ++uncovered_pair_count;
      }
    }
  }
  require(
      recertified_pruned_mass == audit.pruned_unordered_pair_mass &&
          uncovered_pair_count == audit.terminal_pair_count &&
          recertified_pruned_mass + uncovered_pair_count ==
              unordered_pair_count(point_count) &&
          audit.product_partition_mass_closed &&
          audit.complete_cut_without_terminal_payload_materialization &&
          audit.every_recipe_recertified &&
          audit.no_pair_matrix_materialized &&
          audit.no_terminal_pair_catalog_materialized &&
          audit.no_facet_coface_or_incidence_materialized &&
          audit.no_gamma_or_delaunay_structure_materialized &&
          !audit.hierarchy_or_tree_claimed && !audit.public_status_claimed,
      "independent pair enumeration did not recertify the complete cut");
}

void test_exhaustive_small_n_all_ranks_and_determinism() {
  bool observed_recipe = false;
  bool observed_terminal = false;
  for (std::size_t point_count = 2U; point_count <= 9U; ++point_count) {
    const CanonicalPointCloud cloud = make_line_cloud(point_count);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    for (std::size_t maximum_closed_rank = 2U;
         maximum_closed_rank <= 11U;
         ++maximum_closed_rank) {
      const auto budget = generous_budget(point_count);
      const auto first =
          build_exact_pair_block_automatic_prune_recipe_catalog(
              index, cloud, maximum_closed_rank, budget);
      const auto second =
          build_exact_pair_block_automatic_prune_recipe_catalog(
              index, cloud, maximum_closed_rank, budget);
      verify_complete_catalog(index, cloud, maximum_closed_rank, first);
      require(
          first.audit == second.audit && first.recipes == second.recipes,
          "the automatic recipe catalog is not deterministic");
      observed_recipe = observed_recipe || !first.recipes.empty();
      observed_terminal =
          observed_terminal || first.audit.terminal_pair_count != 0U;
    }
  }
  require(
      observed_recipe && observed_terminal,
      "the exhaustive small-n sweep missed a recipe or terminal path");
}

void require_atomic_failure(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactPairBlockAutomaticPruneRecipeCatalogBudget budget,
    ExactPairBlockAutomaticPruneRecipeCatalogDecision expected) {
  const auto result = build_exact_pair_block_automatic_prune_recipe_catalog(
      index, cloud, maximum_closed_rank, budget);
  require(
      !result.complete() && result.audit.decision == expected &&
          result.audit.requested_budget == budget && result.recipes.empty() &&
          result.audit.prune_recipe_count == 0U &&
          !result.audit.every_recipe_recertified &&
          !result.audit.product_partition_mass_closed &&
          !result.audit.complete_cut_without_terminal_payload_materialization,
      "a failed catalog published a partial recipe payload or wrong decision");
}

void test_budget_and_rank_failures_are_atomic() {
  constexpr std::size_t point_count = 9U;
  constexpr std::size_t maximum_closed_rank = 2U;
  const CanonicalPointCloud cloud = make_line_cloud(point_count);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto wide_budget = generous_budget(point_count);
  const auto baseline =
      build_exact_pair_block_automatic_prune_recipe_catalog(
          index, cloud, maximum_closed_rank, wide_budget);
  verify_complete_catalog(index, cloud, maximum_closed_rank, baseline);
  require(
      baseline.audit.product_visit_count > 1U &&
          baseline.audit.maximum_frontier_block_count > 1U &&
          baseline.audit.prune_recipe_count > 0U &&
          baseline.audit.terminal_pair_count > 0U,
      "the atomic-budget fixture does not exercise every output class");

  auto budget = wide_budget;
  budget.maximum_product_visit_count =
      baseline.audit.product_visit_count - 1U;
  require_atomic_failure(
      index,
      cloud,
      maximum_closed_rank,
      budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_product_visit_budget_exhausted);

  budget = wide_budget;
  budget.maximum_frontier_block_count =
      baseline.audit.maximum_frontier_block_count - 1U;
  require_atomic_failure(
      index,
      cloud,
      maximum_closed_rank,
      budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_frontier_budget_exhausted);

  budget = wide_budget;
  budget.maximum_prune_recipe_count =
      baseline.audit.prune_recipe_count - 1U;
  require_atomic_failure(
      index,
      cloud,
      maximum_closed_rank,
      budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_recipe_budget_exhausted);

  budget = wide_budget;
  budget.maximum_terminal_pair_count =
      baseline.audit.terminal_pair_count - 1U;
  require_atomic_failure(
      index,
      cloud,
      maximum_closed_rank,
      budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_terminal_budget_exhausted);

  require_atomic_failure(
      index,
      cloud,
      1U,
      wide_budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_invalid_authority_or_rank);
  require_atomic_failure(
      index,
      cloud,
      12U,
      wide_budget,
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::
          no_catalog_invalid_authority_or_rank);
}

void test_full_chain_eight_clusters_fixture_exercises_exact_recipes() {
  const CanonicalPointCloud cloud = make_eight_clusters_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  for (const std::size_t maximum_closed_rank : {6U, 11U}) {
    const auto result =
        build_exact_pair_block_automatic_prune_recipe_catalog(
            index,
            cloud,
            maximum_closed_rank,
            generous_budget(cloud.size()));
    verify_complete_catalog(index, cloud, maximum_closed_rank, result);
    require(
        result.audit.prune_recipe_count != 0U &&
            result.audit.pruned_unordered_pair_mass != 0U,
        "the full-chain eight-clusters fixture did not exercise an exact "
        "automatic prune recipe");
  }
}

}  // namespace

int main() {
  try {
    test_exhaustive_small_n_all_ranks_and_determinism();
    test_budget_and_rank_failures_are_atomic();
    test_full_chain_eight_clusters_fixture_exercises_exact_recipes();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
