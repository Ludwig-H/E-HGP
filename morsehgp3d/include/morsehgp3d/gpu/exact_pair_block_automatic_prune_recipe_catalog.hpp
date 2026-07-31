#pragma once

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_automatic_prune_recipe_catalog_schema_version = 1U;
inline constexpr std::string_view
    exact_pair_block_automatic_prune_recipe_catalog_backend =
        "reference_cpu";
inline constexpr std::string_view
    exact_pair_block_automatic_prune_recipe_catalog_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_pair_block_automatic_prune_recipe_catalog_mode =
        "bounded_native_lbvh_product_dfs_exact_automatic_prune_recipes";
inline constexpr std::string_view
    exact_pair_block_automatic_prune_recipe_catalog_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_pair_block_automatic_prune_recipe_catalog_public_status =
        "not_claimed";

struct ExactPairBlockAutomaticPruneRecipeCatalogBudget {
  std::size_t maximum_product_visit_count{};
  std::size_t maximum_frontier_block_count{};
  std::size_t maximum_prune_recipe_count{};
  std::size_t maximum_terminal_pair_count{};

  friend bool operator==(
      const ExactPairBlockAutomaticPruneRecipeCatalogBudget&,
      const ExactPairBlockAutomaticPruneRecipeCatalogBudget&) = default;
};

enum class ExactPairBlockAutomaticPruneRecipeCatalogDecision : std::uint8_t {
  not_built,
  no_catalog_invalid_authority_or_rank,
  no_catalog_arithmetic_overflow,
  no_catalog_product_visit_budget_exhausted,
  no_catalog_frontier_budget_exhausted,
  no_catalog_recipe_budget_exhausted,
  no_catalog_terminal_budget_exhausted,
  no_catalog_automatic_receipt_rejected,
  no_catalog_allocation_failed,
  complete_exact_recipe_cut,
};

struct ExactPairBlockAutomaticPruneRecipeCatalogAudit {
  static constexpr std::string_view backend =
      exact_pair_block_automatic_prune_recipe_catalog_backend;
  static constexpr std::string_view profile =
      exact_pair_block_automatic_prune_recipe_catalog_profile;
  static constexpr std::string_view mode =
      exact_pair_block_automatic_prune_recipe_catalog_mode;
  static constexpr std::string_view deployment_status =
      exact_pair_block_automatic_prune_recipe_catalog_deployment_status;
  static constexpr std::string_view public_status =
      exact_pair_block_automatic_prune_recipe_catalog_public_status;

  std::uint32_t schema_version{
      exact_pair_block_automatic_prune_recipe_catalog_schema_version};
  ExactPairBlockAutomaticPruneRecipeCatalogBudget requested_budget{};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t required_witness_point_count{};
  std::size_t unordered_pair_universe_mass{};
  std::size_t product_visit_count{};
  std::size_t diagonal_product_visit_count{};
  std::size_t cross_product_visit_count{};
  std::size_t diagonal_split_count{};
  std::size_t cross_split_count{};
  std::size_t automatic_receipt_attempt_count{};
  std::size_t automatic_receipt_certified_count{};
  std::size_t automatic_receipt_inconclusive_count{};
  std::size_t automatic_search_node_visit_count{};
  std::size_t exact_phi_aabb_maximum_evaluation_count{};
  std::size_t prune_recipe_count{};
  std::size_t terminal_pair_count{};
  std::size_t maximum_frontier_block_count{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};
  bool source_authorities_authenticated{false};
  bool every_recipe_recertified{false};
  bool product_partition_mass_closed{false};
  bool complete_cut_without_terminal_payload_materialization{false};
  bool no_pair_matrix_materialized{true};
  bool no_terminal_pair_catalog_materialized{true};
  bool no_facet_coface_or_incidence_materialized{true};
  bool no_gamma_or_delaunay_structure_materialized{true};
  bool hierarchy_or_tree_claimed{false};
  bool public_status_claimed{false};
  ExactPairBlockAutomaticPruneRecipeCatalogDecision decision{
      ExactPairBlockAutomaticPruneRecipeCatalogDecision::not_built};

  [[nodiscard]] bool complete() const noexcept;

  friend bool operator==(
      const ExactPairBlockAutomaticPruneRecipeCatalogAudit&,
      const ExactPairBlockAutomaticPruneRecipeCatalogAudit&) = default;
};

struct ExactPairBlockAutomaticPruneRecipeCatalogResult {
  ExactPairBlockAutomaticPruneRecipeCatalogAudit audit{};
  std::vector<ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>
      recipes;

  [[nodiscard]] bool complete() const noexcept;
};

// Traverses only the deterministic LBVH product tree needed to locate exact
// prune blocks.  It retains one DFS frontier and the compact recipe output;
// terminal leaf pairs are counted but never materialized.  Any budget or
// structural failure clears the recipe payload atomically.
class ExactPairBlockAutomaticPruneRecipeCatalogBuilder final {
 public:
  [[nodiscard]] static ExactPairBlockAutomaticPruneRecipeCatalogResult build(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_closed_rank,
      ExactPairBlockAutomaticPruneRecipeCatalogBudget budget);
};

[[nodiscard]] ExactPairBlockAutomaticPruneRecipeCatalogResult
build_exact_pair_block_automatic_prune_recipe_catalog(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactPairBlockAutomaticPruneRecipeCatalogBudget budget);

}  // namespace morsehgp3d::gpu
