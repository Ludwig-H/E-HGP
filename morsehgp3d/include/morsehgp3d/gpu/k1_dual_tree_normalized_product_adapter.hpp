#pragma once

#include "morsehgp3d/gpu/k1_boruvka.hpp"
#include "morsehgp3d/hierarchy/direct_k1_normalized_product_stream.hpp"

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    k1_dual_tree_normalized_product_adapter_schema_version = 1U;
inline constexpr std::string_view
    k1_dual_tree_normalized_product_adapter_backend =
        "gpu_seeded_reference_cpu_exact";
inline constexpr std::string_view
    k1_dual_tree_normalized_product_adapter_profile = "hgp_reduced";
inline constexpr std::string_view
    k1_dual_tree_normalized_product_adapter_mode =
        "fresh_dual_tree_replay_compact_k1_recertification_normalized_product_v1";
inline constexpr std::string_view
    k1_dual_tree_normalized_product_adapter_public_status = "not_claimed";

enum class K1DualTreeNormalizedProductInitializationDecision
    : std::uint8_t {
  not_certified,
  no_dual_tree_source_rejected,
  no_compact_authority_rejected,
  no_reference_anchor_rejected,
  no_normalized_stream_rejected,
  no_source_identity_mismatch,
  no_allocation_failed,
  complete_certified_tower_consumable_k1_stream,
};

// This adapter is deliberately a one-way seam.  The GPU-seeded source remains
// a local EMST witness, while the returned stream owns the only persistent K1
// compact forest and has the exact type consumed by the normalized H0 product
// coordinator.  The transient source replay, reference anchor and compact
// cross-check are released before the result is returned.
struct K1DualTreeNormalizedProductInitialization {
  static constexpr std::string_view backend =
      k1_dual_tree_normalized_product_adapter_backend;
  static constexpr std::string_view profile =
      k1_dual_tree_normalized_product_adapter_profile;
  static constexpr std::string_view mode =
      k1_dual_tree_normalized_product_adapter_mode;
  static constexpr std::string_view public_status =
      k1_dual_tree_normalized_product_adapter_public_status;

  std::uint32_t schema_version{
      k1_dual_tree_normalized_product_adapter_schema_version};
  K1DualTreeExactBoruvkaVerification source_verification{};
  K1SeededExactCompactHierarchyVerification compact_verification{};
  hierarchy::ExactDirectK1NormalizedProductStreamInitialization stream{};
  contract::CanonicalId recertified_compact_source_forest_digest{};
  bool dual_tree_source_freshly_replayed{false};
  bool compact_authority_freshly_recertified{false};
  bool independent_reference_cpu_anchor_rebuilt{false};
  bool shared_morton_lbvh_identity_certified{false};
  bool normalized_authority_freshly_replayed{false};
  bool compact_and_normalized_source_identity_certified{false};
  bool compact_shape_and_exact_weights_certified{false};
  bool tower_consumable_normalized_k1_stream_certified{false};
  bool returned_session_owns_only_one_compact_k1_forest{false};
  bool pair_matrix_gamma_delaunay_cells_or_cofaces_materialized{false};
  bool complete_graph_emst_fallback_used{false};
  bool public_status_claimed{false};
  K1DualTreeNormalizedProductInitializationDecision decision{
      K1DualTreeNormalizedProductInitializationDecision::not_certified};

  [[nodiscard]] bool certified_tower_consumable_stream() const noexcept;
};

static_assert(!std::is_copy_constructible_v<
              K1DualTreeNormalizedProductInitialization>);
static_assert(std::is_nothrow_move_constructible_v<
              K1DualTreeNormalizedProductInitialization>);

// Both budgets are trusted caller policy.  A falsified dual-tree receipt is
// rejected before the reference anchor or normalized session is allocated.
// Success performs another exact Morton/LBVH Boruvka replay inside the
// normalized stream initializer and binds its canonical forest digest to the
// freshly recertified dual-tree compact forest.  It never calls the dense
// complete-graph EMST oracle and never materializes a pair or support catalog.
[[nodiscard]] K1DualTreeNormalizedProductInitialization
initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1DualTreeExactBoruvkaResult& source,
    const hierarchy::ExactDirectK1BoruvkaClosedCutSessionBudget&
        k1_authority_budget,
    const hierarchy::ExactDirectK1NormalizedProductStreamBudget&
        stream_budget) noexcept;

}  // namespace morsehgp3d::gpu
