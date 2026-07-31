#include "morsehgp3d/gpu/exact_pair_block_automatic_prune_recipe_catalog.hpp"

#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_product(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool checked_pair_count(
    std::size_t point_count,
    std::size_t& result) noexcept {
  std::size_t first = point_count;
  std::size_t second = point_count == 0U ? 0U : point_count - 1U;
  if ((first & 1U) == 0U) {
    first /= 2U;
  } else {
    second /= 2U;
  }
  return checked_product(first, second, result);
}

[[nodiscard]] bool is_expected_inconclusive(
    hierarchy::ExactBlockRankPruneStatus status) noexcept {
  return status ==
             hierarchy::ExactBlockRankPruneStatus::
                 insufficient_witness_mass ||
      status ==
             hierarchy::ExactBlockRankPruneStatus::
                 inconclusive_nonnegative_phi;
}

}  // namespace

bool ExactPairBlockAutomaticPruneRecipeCatalogAudit::complete()
    const noexcept {
  std::size_t resolved_mass = 0U;
  std::size_t visited_product_count = 0U;
  std::size_t receipt_outcome_count = 0U;
  std::size_t inconclusive_outcome_count = 0U;
  return schema_version ==
             exact_pair_block_automatic_prune_recipe_catalog_schema_version &&
      point_count >= 2U && maximum_closed_rank >= 2U &&
      maximum_closed_rank <=
          hierarchy::exact_block_rank_prune_maximum_closed_rank &&
      required_witness_point_count == maximum_closed_rank - 1U &&
      checked_add(
          diagonal_product_visit_count,
          cross_product_visit_count,
          visited_product_count) &&
      visited_product_count == product_visit_count &&
      automatic_receipt_attempt_count == cross_product_visit_count &&
      checked_add(
          automatic_receipt_certified_count,
          automatic_receipt_inconclusive_count,
          receipt_outcome_count) &&
      receipt_outcome_count == automatic_receipt_attempt_count &&
      diagonal_split_count == diagonal_product_visit_count &&
      checked_add(
          cross_split_count,
          terminal_pair_count,
          inconclusive_outcome_count) &&
      inconclusive_outcome_count == automatic_receipt_inconclusive_count &&
      prune_recipe_count == automatic_receipt_certified_count &&
      terminal_unordered_pair_mass == terminal_pair_count &&
      product_visit_count <= requested_budget.maximum_product_visit_count &&
      maximum_frontier_block_count <=
          requested_budget.maximum_frontier_block_count &&
      prune_recipe_count <= requested_budget.maximum_prune_recipe_count &&
      terminal_pair_count <= requested_budget.maximum_terminal_pair_count &&
      checked_add(
          pruned_unordered_pair_mass,
          terminal_unordered_pair_mass,
          resolved_mass) &&
      resolved_mass == unordered_pair_universe_mass &&
      source_authorities_authenticated && every_recipe_recertified &&
      product_partition_mass_closed &&
      complete_cut_without_terminal_payload_materialization &&
      no_pair_matrix_materialized && no_terminal_pair_catalog_materialized &&
      no_facet_coface_or_incidence_materialized &&
      no_gamma_or_delaunay_structure_materialized &&
      !hierarchy_or_tree_claimed && !public_status_claimed &&
      decision == ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                      complete_exact_recipe_cut;
}

bool ExactPairBlockAutomaticPruneRecipeCatalogResult::complete()
    const noexcept {
  return audit.complete() && recipes.size() == audit.prune_recipe_count;
}

ExactPairBlockAutomaticPruneRecipeCatalogResult
ExactPairBlockAutomaticPruneRecipeCatalogBuilder::build(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactPairBlockAutomaticPruneRecipeCatalogBudget budget) {
  ExactPairBlockAutomaticPruneRecipeCatalogResult result;
  auto& audit = result.audit;
  audit.requested_budget = budget;
  audit.point_count = cloud.size();
  audit.maximum_closed_rank = maximum_closed_rank;
  audit.required_witness_point_count = maximum_closed_rank == 0U
      ? 0U
      : maximum_closed_rank - 1U;
  const auto fail = [&result](
                        ExactPairBlockAutomaticPruneRecipeCatalogDecision
                            decision) {
    result.recipes.clear();
    result.audit.prune_recipe_count = 0U;
    result.audit.every_recipe_recertified = false;
    result.audit.product_partition_mass_closed = false;
    result.audit.complete_cut_without_terminal_payload_materialization =
        false;
    result.audit.decision = decision;
    return std::move(result);
  };

  if (!index.validated_for(cloud) || cloud.size() < 2U ||
      maximum_closed_rank < 2U ||
      maximum_closed_rank >
          hierarchy::exact_block_rank_prune_maximum_closed_rank ||
      index.root_index_ >= index.nodes_.size()) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_invalid_authority_or_rank);
  }
  audit.source_authorities_authenticated = true;
  if (!checked_pair_count(
          cloud.size(), audit.unordered_pair_universe_mass)) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_arithmetic_overflow);
  }

  const auto node_authority = [&index](std::size_t node_index) {
    const auto& node = index.nodes_[node_index];
    return ExactPairBlockNodeAuthority{
        node_index, node.leaf_begin, node.leaf_end};
  };
  const auto make_diagonal = [&index, &node_authority](
                                 std::size_t node_index,
                                 ExactPairBlockAuthority& block) {
    if (node_index >= index.nodes_.size()) {
      return false;
    }
    const auto node = node_authority(node_index);
    std::size_t mass = 0U;
    if (!checked_pair_count(node.leaf_count(), mass)) {
      return false;
    }
    block = {ExactPairBlockAuthorityKind::diagonal, node, node, mass};
    return true;
  };
  const auto make_cross = [&index, &node_authority](
                              std::size_t first_node_index,
                              std::size_t second_node_index,
                              ExactPairBlockAuthority& block) {
    if (first_node_index >= index.nodes_.size() ||
        second_node_index >= index.nodes_.size()) {
      return false;
    }
    auto first = node_authority(first_node_index);
    auto second = node_authority(second_node_index);
    if (second.leaf_begin < first.leaf_begin) {
      std::swap(first, second);
    }
    std::size_t mass = 0U;
    if (first.leaf_end > second.leaf_begin ||
        !checked_product(first.leaf_count(), second.leaf_count(), mass)) {
      return false;
    }
    block = {ExactPairBlockAuthorityKind::cross, first, second, mass};
    return true;
  };

  try {
    std::vector<ExactPairBlockAuthority> frontier;
    frontier.reserve(std::min<std::size_t>(
        budget.maximum_frontier_block_count,
        index.build_counters_.maximum_depth + 3U));
    result.recipes.reserve(std::min<std::size_t>(
        budget.maximum_prune_recipe_count, cloud.size()));
    ExactPairBlockAuthority root;
    if (!make_diagonal(index.root_index_, root) ||
        root.unordered_pair_mass != audit.unordered_pair_universe_mass) {
      return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                      no_catalog_invalid_authority_or_rank);
    }
    if (root.unordered_pair_mass != 0U) {
      if (budget.maximum_frontier_block_count == 0U) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_frontier_budget_exhausted);
      }
      frontier.push_back(root);
      audit.maximum_frontier_block_count = 1U;
    }

    const auto push_block = [&frontier, &audit, budget](
                                const ExactPairBlockAuthority& block) {
      if (block.unordered_pair_mass == 0U) {
        return true;
      }
      if (frontier.size() >= budget.maximum_frontier_block_count) {
        return false;
      }
      frontier.push_back(block);
      audit.maximum_frontier_block_count = std::max(
          audit.maximum_frontier_block_count, frontier.size());
      return true;
    };

    while (!frontier.empty()) {
      if (audit.product_visit_count >=
          budget.maximum_product_visit_count) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_product_visit_budget_exhausted);
      }
      const ExactPairBlockAuthority source = frontier.back();
      frontier.pop_back();
      ++audit.product_visit_count;

      if (source.kind == ExactPairBlockAuthorityKind::diagonal) {
        ++audit.diagonal_product_visit_count;
        if (source.first.node_index >= index.nodes_.size() ||
            source.first != source.second) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_invalid_authority_or_rank);
        }
        const auto& parent = index.nodes_[source.first.node_index];
        if (parent.is_leaf() ||
            parent.left_child >= index.nodes_.size() ||
            parent.right_child >= index.nodes_.size() ||
            parent.left_child == parent.right_child) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_invalid_authority_or_rank);
        }
        ExactPairBlockAuthority left;
        ExactPairBlockAuthority cross;
        ExactPairBlockAuthority right;
        std::size_t children_mass = 0U;
        if (!make_diagonal(parent.left_child, left) ||
            !make_cross(parent.left_child, parent.right_child, cross) ||
            !make_diagonal(parent.right_child, right) ||
            !checked_add(
                left.unordered_pair_mass,
                cross.unordered_pair_mass,
                children_mass) ||
            !checked_add(
                children_mass,
                right.unordered_pair_mass,
                children_mass) ||
            children_mass != source.unordered_pair_mass) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_arithmetic_overflow);
        }
        // Reverse push order gives the same left, cross, right DFS order as
        // the canonical host replay while retaining only one depth frontier.
        if (!push_block(right) || !push_block(cross) || !push_block(left)) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_frontier_budget_exhausted);
        }
        ++audit.diagonal_split_count;
        continue;
      }

      ++audit.cross_product_visit_count;
      if (source.kind != ExactPairBlockAuthorityKind::cross ||
          source.first.node_index >= index.nodes_.size() ||
          source.second.node_index >= index.nodes_.size() ||
          source.first.leaf_end > source.second.leaf_begin) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_invalid_authority_or_rank);
      }
      ++audit.automatic_receipt_attempt_count;
      hierarchy::ExactBlockRankPruneResult prune =
          hierarchy::generate_exact_block_rank_prune_receipt(
              index,
              cloud,
              source.first.node_index,
              source.second.node_index,
              maximum_closed_rank);
      std::size_t accumulated_search_node_visit_count = 0U;
      std::size_t accumulated_exact_phi_evaluation_count = 0U;
      if (!checked_add(
              audit.automatic_search_node_visit_count,
              prune.audit().automatic_search_node_visit_count,
              accumulated_search_node_visit_count) ||
          !checked_add(
              audit.exact_phi_aabb_maximum_evaluation_count,
              prune.audit()
                  .automatic_search_exact_phi_aabb_maximum_evaluation_count,
              accumulated_exact_phi_evaluation_count)) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_arithmetic_overflow);
      }
      audit.automatic_search_node_visit_count =
          accumulated_search_node_visit_count;
      audit.exact_phi_aabb_maximum_evaluation_count =
          accumulated_exact_phi_evaluation_count;
      if (prune.certified()) {
        if (result.recipes.size() >=
            budget.maximum_prune_recipe_count) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_recipe_budget_exhausted);
        }
        const hierarchy::ExactBlockRankPruneReceipt* receipt =
            prune.receipt();
        if (receipt == nullptr ||
            !receipt->certifies(
                index,
                cloud,
                source.first.node_index,
                source.second.node_index,
                maximum_closed_rank) ||
            receipt->unordered_pair_mass() !=
                source.unordered_pair_mass) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_automatic_receipt_rejected);
        }
        ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe recipe;
        recipe.source = source;
        const auto witnesses = receipt->witness_nodes();
        recipe.witness_node_count = witnesses.size();
        for (std::size_t witness_index = 0U;
             witness_index < witnesses.size();
             ++witness_index) {
          recipe.witness_node_indices[witness_index] =
              witnesses[witness_index].node_index;
        }
        result.recipes.push_back(recipe);
        if (!checked_add(
                audit.pruned_unordered_pair_mass,
                source.unordered_pair_mass,
                audit.pruned_unordered_pair_mass)) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_arithmetic_overflow);
        }
        ++audit.automatic_receipt_certified_count;
        continue;
      }
      if (!is_expected_inconclusive(prune.status())) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_automatic_receipt_rejected);
      }
      ++audit.automatic_receipt_inconclusive_count;

      const auto& first_node = index.nodes_[source.first.node_index];
      const auto& second_node = index.nodes_[source.second.node_index];
      if (first_node.is_leaf() && second_node.is_leaf()) {
        if (source.unordered_pair_mass != 1U) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_invalid_authority_or_rank);
        }
        if (audit.terminal_pair_count >=
            budget.maximum_terminal_pair_count) {
          return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                          no_catalog_terminal_budget_exhausted);
        }
        ++audit.terminal_pair_count;
        ++audit.terminal_unordered_pair_mass;
        continue;
      }

      const bool split_second = !second_node.is_leaf() &&
          (first_node.is_leaf() ||
           source.second.leaf_count() >= source.first.leaf_count());
      ExactPairBlockAuthority first_child;
      ExactPairBlockAuthority second_child;
      bool children_valid = false;
      if (split_second) {
        children_valid =
            make_cross(
                source.first.node_index,
                second_node.left_child,
                first_child) &&
            make_cross(
                source.first.node_index,
                second_node.right_child,
                second_child);
      } else if (!first_node.is_leaf()) {
        children_valid =
            make_cross(
                first_node.left_child,
                source.second.node_index,
                first_child) &&
            make_cross(
                first_node.right_child,
                source.second.node_index,
                second_child);
      }
      std::size_t children_mass = 0U;
      if (!children_valid ||
          !checked_add(
              first_child.unordered_pair_mass,
              second_child.unordered_pair_mass,
              children_mass) ||
          children_mass != source.unordered_pair_mass) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_arithmetic_overflow);
      }
      if (!push_block(second_child) || !push_block(first_child)) {
        return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                        no_catalog_frontier_budget_exhausted);
      }
      ++audit.cross_split_count;
    }
  } catch (const std::bad_alloc&) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_allocation_failed);
  } catch (const std::length_error&) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_allocation_failed);
  } catch (const std::logic_error&) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_automatic_receipt_rejected);
  }

  audit.prune_recipe_count = result.recipes.size();
  audit.every_recipe_recertified = true;
  std::size_t resolved_mass = 0U;
  audit.product_partition_mass_closed = checked_add(
      audit.pruned_unordered_pair_mass,
      audit.terminal_unordered_pair_mass,
      resolved_mass) &&
      resolved_mass == audit.unordered_pair_universe_mass;
  audit.complete_cut_without_terminal_payload_materialization =
      audit.product_partition_mass_closed;
  audit.decision = ExactPairBlockAutomaticPruneRecipeCatalogDecision::
      complete_exact_recipe_cut;
  if (!result.complete()) {
    return fail(ExactPairBlockAutomaticPruneRecipeCatalogDecision::
                    no_catalog_automatic_receipt_rejected);
  }
  return result;
}

ExactPairBlockAutomaticPruneRecipeCatalogResult
build_exact_pair_block_automatic_prune_recipe_catalog(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactPairBlockAutomaticPruneRecipeCatalogBudget budget) {
  return ExactPairBlockAutomaticPruneRecipeCatalogBuilder::build(
      index, cloud, maximum_closed_rank, budget);
}

}  // namespace morsehgp3d::gpu
