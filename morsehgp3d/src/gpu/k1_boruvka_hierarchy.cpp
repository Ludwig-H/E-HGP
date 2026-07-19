#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include <cstddef>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>

namespace morsehgp3d::gpu {
namespace {

struct LinearStorageAudit {
  std::size_t entry_count{};
  std::size_t entry_limit{};

  friend bool operator==(
      const LinearStorageAudit&,
      const LinearStorageAudit&) = default;
};

[[nodiscard]] bool checked_add(
    std::size_t& target,
    std::size_t increment) noexcept {
  if (increment > std::numeric_limits<std::size_t>::max() - target) {
    return false;
  }
  target += increment;
  return true;
}

[[nodiscard]] std::optional<LinearStorageAudit> linear_storage_audit(
    const hierarchy::K1CompactForest& forest) noexcept {
  if (forest.point_count == 0U) {
    return std::nullopt;
  }
  std::size_t entry_count = 0U;
  if (!checked_add(entry_count, forest.levels.size()) ||
      !checked_add(entry_count, forest.selected_edges.size()) ||
      !checked_add(entry_count, forest.merge_nodes.size()) ||
      !checked_add(entry_count, forest.child_ids.size()) ||
      !checked_add(entry_count, forest.equal_level_batches.size())) {
    return std::nullopt;
  }
  const std::size_t edge_count = forest.point_count - 1U;
  if (edge_count > std::numeric_limits<std::size_t>::max() / 6U) {
    return std::nullopt;
  }
  return LinearStorageAudit{entry_count, 6U * edge_count};
}

template <typename Value>
[[nodiscard]] bool equal_three(
    const Value& observed,
    const Value& from_source,
    const Value& from_reference) {
  return observed == from_source && from_source == from_reference;
}

[[nodiscard]] K1SeededExactCompactHierarchyVerification
verify_compact_k1_hierarchy_with_source_verification(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1SeededExactBoruvkaResult& source,
    const K1SeededExactCompactHierarchy& reduction,
    const K1SeededExactBoruvkaVerification& source_verification) {
  K1SeededExactCompactHierarchyVerification verification;
  verification.reduction_status_certified =
      reduction.reduction_status ==
      K1SeededExactHierarchyReductionStatus::
          compact_k1_forest_certified;
  verification.local_scope_certified =
      reduction.scope ==
      K1SeededExactHierarchyScope::local_k1_compact_forest_only;

  verification.source_emst_witness_certified =
      source_verification.emst_witness_certified;
  verification.source_status_separation_certified =
      source_verification.hierarchy_status_separation_certified &&
      source.hierarchy_reduction_status ==
          K1HybridHierarchyReductionStatus::not_performed &&
      source.scientific_status ==
          K1HybridScientificStatus::local_emst_witness_only;

  hierarchy::K1ExactBoruvkaResult reference;
  std::optional<hierarchy::K1CompactForest> from_source;
  std::optional<hierarchy::K1CompactForest> from_reference;
  try {
    reference = hierarchy::build_exact_lbvh_boruvka(index, cloud);
    from_source.emplace(hierarchy::build_compact_k1_forest(
        source.point_count,
        std::span<const hierarchy::ExactEmstEdge>{source.emst_edges}));
    from_reference.emplace(hierarchy::build_compact_k1_forest(
        reference.point_count,
        std::span<const hierarchy::ExactEmstEdge>{reference.emst_edges}));
  } catch (const std::exception&) {
    return verification;
  }
  verification.source_emst_witness_certified =
      verification.source_emst_witness_certified &&
      reference.emst_witness_certified;

  const hierarchy::K1CompactForest& observed = reduction.forest;
  const hierarchy::K1CompactForest& expected_source = *from_source;
  const hierarchy::K1CompactForest& expected_reference = *from_reference;

  verification.source_tree_binding_certified =
      observed.point_count == source.point_count &&
      source.point_count == reference.point_count &&
      reference.point_count == cloud.size() &&
      observed.selected_edges == source.emst_edges &&
      expected_source.selected_edges == source.emst_edges &&
      expected_reference.selected_edges == reference.emst_edges &&
      equal_three(
          observed.selected_edges,
          expected_source.selected_edges,
          expected_reference.selected_edges);
  verification.equal_level_batches_certified =
      equal_three(
          observed.levels,
          expected_source.levels,
          expected_reference.levels) &&
      equal_three(
          observed.equal_level_batches,
          expected_source.equal_level_batches,
          expected_reference.equal_level_batches);
  verification.canonical_multifusions_certified =
      equal_three(
          observed.merge_nodes,
          expected_source.merge_nodes,
          expected_reference.merge_nodes) &&
      equal_three(
          observed.child_ids,
          expected_source.child_ids,
          expected_reference.child_ids);
  verification.compact_topology_certified =
      observed.point_count == expected_source.point_count &&
      expected_source.point_count == expected_reference.point_count &&
      observed.root_node_id == expected_source.root_node_id &&
      expected_source.root_node_id == expected_reference.root_node_id &&
      observed.merge_nodes.size() == expected_source.merge_nodes.size() &&
      expected_source.merge_nodes.size() ==
          expected_reference.merge_nodes.size() &&
      observed.child_ids.size() == expected_source.child_ids.size() &&
      expected_source.child_ids.size() ==
          expected_reference.child_ids.size();
  verification.exact_weights_certified =
      observed.total_squared_weight == source.total_squared_weight &&
      source.total_squared_weight == reference.total_squared_weight &&
      equal_three(
          observed.total_squared_weight,
          expected_source.total_squared_weight,
          expected_reference.total_squared_weight) &&
      observed.total_hgp_weight == source.total_hgp_weight &&
      source.total_hgp_weight == reference.total_hgp_weight &&
      equal_three(
          observed.total_hgp_weight,
          expected_source.total_hgp_weight,
          expected_reference.total_hgp_weight);
  verification.counters_certified = equal_three(
      observed.counters,
      expected_source.counters,
      expected_reference.counters);

  const std::optional<LinearStorageAudit> observed_storage =
      linear_storage_audit(observed);
  const std::optional<LinearStorageAudit> source_storage =
      linear_storage_audit(expected_source);
  const std::optional<LinearStorageAudit> reference_storage =
      linear_storage_audit(expected_reference);
  verification.linear_storage_certified =
      observed_storage.has_value() && source_storage.has_value() &&
      reference_storage.has_value() &&
      *observed_storage == *source_storage &&
      *source_storage == *reference_storage &&
      observed_storage->entry_count <= observed_storage->entry_limit &&
      observed.counters.linear_storage_entry_count ==
          observed_storage->entry_count &&
      observed.counters.linear_storage_entry_limit ==
          observed_storage->entry_limit &&
      observed.counters.stored_coverage_point_id_count == 0U;

  verification.local_k1_hierarchy_certified =
      verification.source_emst_witness_certified &&
      verification.source_status_separation_certified &&
      verification.source_tree_binding_certified &&
      verification.exact_weights_certified &&
      verification.equal_level_batches_certified &&
      verification.canonical_multifusions_certified &&
      verification.compact_topology_certified &&
      verification.counters_certified &&
      verification.linear_storage_certified &&
      verification.reduction_status_certified &&
      verification.local_scope_certified;
  return verification;
}

}  // namespace

K1SeededExactCompactHierarchyVerification
verify_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& source,
    const K1SeededExactCompactHierarchy& reduction) {
  const K1SeededExactBoruvkaVerification source_verification =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, trusted_seed_policy, source);
  return verify_compact_k1_hierarchy_with_source_verification(
      index, cloud, source, reduction, source_verification);
}

K1SeededExactCompactHierarchy
build_compact_k1_hierarchy_from_gpu_seeded_exact_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1SeededExactBoruvkaResult& source) {
  // Revalidate before allocating any arena from the untrusted source.
  const K1SeededExactBoruvkaVerification source_verification =
      verify_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
          index, cloud, trusted_seed_policy, source);
  if (!source_verification.emst_witness_certified) {
    throw std::logic_error(
        "the seeded exact compact k=1 hierarchy requires a freshly "
        "recertified source EMST witness");
  }

  K1SeededExactCompactHierarchy reduction;
  reduction.forest = hierarchy::build_compact_k1_forest(
      source.point_count,
      std::span<const hierarchy::ExactEmstEdge>{source.emst_edges});
  reduction.reduction_status =
      K1SeededExactHierarchyReductionStatus::
          compact_k1_forest_certified;
  reduction.scope =
      K1SeededExactHierarchyScope::local_k1_compact_forest_only;

  const K1SeededExactCompactHierarchyVerification verification =
      verify_compact_k1_hierarchy_with_source_verification(
          index,
          cloud,
          source,
          reduction,
          source_verification);
  if (!verification.local_k1_hierarchy_certified) {
    throw std::logic_error(
        "the seeded exact compact k=1 hierarchy failed its independent "
        "source replay and two-source reduction verification");
  }
  return reduction;
}

}  // namespace morsehgp3d::gpu
