#include "morsehgp3d/gpu/k1_dual_tree_normalized_product_adapter.hpp"

#include <exception>
#include <new>

namespace morsehgp3d::gpu {

bool K1DualTreeNormalizedProductInitialization::
    certified_tower_consumable_stream() const noexcept {
  return schema_version ==
             k1_dual_tree_normalized_product_adapter_schema_version &&
      source_verification.emst_witness_certified &&
      source_verification.hierarchy_status_separation_certified &&
      compact_verification.local_k1_hierarchy_certified &&
      stream.certified_initialized_stream() &&
      dual_tree_source_freshly_replayed &&
      compact_authority_freshly_recertified &&
      independent_reference_cpu_anchor_rebuilt &&
      shared_morton_lbvh_identity_certified &&
      normalized_authority_freshly_replayed &&
      compact_and_normalized_source_identity_certified &&
      compact_shape_and_exact_weights_certified &&
      tower_consumable_normalized_k1_stream_certified &&
      returned_session_owns_only_one_compact_k1_forest &&
      !pair_matrix_gamma_delaunay_cells_or_cofaces_materialized &&
      !complete_graph_emst_fallback_used &&
      !public_status_claimed &&
      decision == K1DualTreeNormalizedProductInitializationDecision::
                      complete_certified_tower_consumable_k1_stream;
}

K1DualTreeNormalizedProductInitialization
initialize_exact_direct_k1_normalized_product_stream_from_gpu_seeded_dual_tree(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1DualTreeExactBoruvkaResult& source,
    const hierarchy::ExactDirectK1BoruvkaClosedCutSessionBudget&
        k1_authority_budget,
    const hierarchy::ExactDirectK1NormalizedProductStreamBudget&
        stream_budget) noexcept {
  K1DualTreeNormalizedProductInitialization output;
  try {
    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_dual_tree_source_rejected;
    output.source_verification =
        verify_gpu_seeded_cpu_exact_dual_tree_k1_boruvka(
            index, cloud, trusted_seed_policy, source);
    output.dual_tree_source_freshly_replayed =
        output.source_verification.fresh_replay_certified;
    if (!output.source_verification.emst_witness_certified ||
        !output.source_verification.hierarchy_status_separation_certified) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_dual_tree_source_rejected;
      return output;
    }

    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_compact_authority_rejected;
    const K1SeededExactCompactHierarchy compact =
        build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
            index, cloud, trusted_seed_policy, source);
    output.compact_verification =
        verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
            index, cloud, trusted_seed_policy, source, compact);
    output.compact_authority_freshly_recertified =
        output.compact_verification.local_k1_hierarchy_certified;
    if (!output.compact_authority_freshly_recertified) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_compact_authority_rejected;
      return output;
    }

    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_reference_anchor_rejected;
    const hierarchy::K1ExactBoruvkaResult reference =
        hierarchy::build_exact_lbvh_boruvka(index, cloud);
    const hierarchy::K1BoruvkaVerification reference_verification =
        hierarchy::verify_exact_lbvh_boruvka(index, cloud, reference);
    output.independent_reference_cpu_anchor_rebuilt =
        reference_verification.emst_witness_certified;
    output.shared_morton_lbvh_identity_certified =
        output.source_verification.index_identity_certified &&
        reference_verification.index_identity_certified;
    if (!output.independent_reference_cpu_anchor_rebuilt ||
        !output.shared_morton_lbvh_identity_certified) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_reference_anchor_rejected;
      return output;
    }

    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_normalized_stream_rejected;
    output.stream =
        hierarchy::initialize_exact_direct_k1_normalized_product_stream(
            index,
            cloud,
            reference,
            k1_authority_budget,
            stream_budget);
    output.normalized_authority_freshly_replayed =
        output.stream.boruvka_authority_freshly_replayed;
    if (!output.stream.certified_initialized_stream()) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_normalized_stream_rejected;
      return output;
    }

    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_source_identity_mismatch;
    output.recertified_compact_source_forest_digest =
        hierarchy::canonical_exact_direct_k1_closed_cut_source_forest_digest(
            output.stream.canonical_cloud_digest, compact.forest);
    output.compact_and_normalized_source_identity_certified =
        output.recertified_compact_source_forest_digest ==
            output.stream.source_forest_digest &&
        output.stream.session.source_forest_digest() ==
            output.stream.source_forest_digest &&
        output.stream.session.canonical_cloud_digest() ==
            output.stream.canonical_cloud_digest;
    output.compact_shape_and_exact_weights_certified =
        compact.forest.point_count == output.stream.requirements.point_count &&
        compact.forest.point_count == source.point_count &&
        compact.forest.selected_edges == source.emst_edges &&
        compact.forest.selected_edges == reference.emst_edges &&
        compact.forest.total_squared_weight == source.total_squared_weight &&
        source.total_squared_weight == reference.total_squared_weight &&
        compact.forest.total_hgp_weight == source.total_hgp_weight &&
        source.total_hgp_weight == reference.total_hgp_weight &&
        output.stream.exact_product_batch_count ==
            compact.forest.equal_level_batches.size() + 1U &&
        output.stream.exact_merge_node_count ==
            compact.forest.merge_nodes.size() &&
        output.stream.exact_child_reference_count ==
            compact.forest.child_ids.size() &&
        output.stream.session.root_node_id() == compact.forest.root_node_id &&
        compact.forest.counters.stored_coverage_point_id_count == 0U &&
        compact.forest.counters.linear_storage_entry_count <=
            compact.forest.counters.linear_storage_entry_limit;
    if (!output.compact_and_normalized_source_identity_certified ||
        !output.compact_shape_and_exact_weights_certified) {
      output.stream.session =
          hierarchy::ExactDirectK1NormalizedProductStreamSession{};
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_source_identity_mismatch;
      return output;
    }

    output.tower_consumable_normalized_k1_stream_certified = true;
    output.returned_session_owns_only_one_compact_k1_forest = true;
    output.pair_matrix_gamma_delaunay_cells_or_cofaces_materialized = false;
    output.complete_graph_emst_fallback_used = false;
    output.public_status_claimed = false;
    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        complete_certified_tower_consumable_k1_stream;
    return output;
  } catch (const std::bad_alloc&) {
    output.stream.session =
        hierarchy::ExactDirectK1NormalizedProductStreamSession{};
    output.decision = K1DualTreeNormalizedProductInitializationDecision::
        no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.stream.session =
        hierarchy::ExactDirectK1NormalizedProductStreamSession{};
    if (output.decision ==
        K1DualTreeNormalizedProductInitializationDecision::not_certified) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_dual_tree_source_rejected;
    }
    return output;
  } catch (...) {
    // This API is deliberately noexcept because it is a gate between an
    // accelerator-fed receipt and the normalized product.  A foreign
    // launcher is allowed to violate the usual std::exception convention;
    // such a failure must revoke the partially built session, never terminate
    // the process or publish a tower-consumable capability.
    output.stream.session =
        hierarchy::ExactDirectK1NormalizedProductStreamSession{};
    if (output.decision ==
        K1DualTreeNormalizedProductInitializationDecision::not_certified) {
      output.decision = K1DualTreeNormalizedProductInitializationDecision::
          no_dual_tree_source_rejected;
    }
    return output;
  }
}

}  // namespace morsehgp3d::gpu
