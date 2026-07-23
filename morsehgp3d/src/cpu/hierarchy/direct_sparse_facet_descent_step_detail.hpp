#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"

#include <optional>

namespace morsehgp3d::hierarchy::detail {

// Non-owning, allocation-free callback used only by the Phase-10.5c caller.
// Every invocation receives the complete canonical facet key.  A null return
// is a cache miss; a non-null result must remain alive and immutable through
// the transient step.  The core rechecks the certified local scope, complete
// key identity, exact center and exact squared level before reusing it.
struct ExactDirectSparseCertifiedFacetMiniballLookup {
  using Function = const ExactFacetMiniballResult* (*)(
      const void*, const ExactDirectSparseFacetKey&) noexcept;

  const void* context{};
  Function function{};

  [[nodiscard]] const ExactFacetMiniballResult* operator()(
      const ExactDirectSparseFacetKey& key) const noexcept {
    return function == nullptr ? nullptr : function(context, key);
  }
};

// The scientific result remains the public 10.5b value.  Only miniballs built
// by this invocation are returned beside it so that 10.5c can move them into a
// bounded memo.  Reused cache entries are non-owning and never escape here.
struct ExactDirectSparseFacetDescentStepTransient {
  ExactDirectSparseFacetDescentStepResult result{};
  std::optional<ExactFacetMiniballResult> newly_built_source_miniball;
  std::optional<ExactFacetMiniballResult> newly_built_successor_miniball;
};

// Reusable 10.5b kernel.  certified_source_miniball is optional and bypasses
// only the bounded local source build.  certified_miniball_lookup is consulted
// only after a distinct canonical successor key has been established.  Both
// reuse paths fail closed on a malformed or mismatched certified payload.
[[nodiscard]] ExactDirectSparseFacetDescentStepTransient
build_exact_direct_sparse_facet_descent_step_transient(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first,
    const ExactFacetMiniballResult* certified_source_miniball = nullptr,
    ExactDirectSparseCertifiedFacetMiniballLookup
        certified_miniball_lookup = {});

// Phase 14F proposal seam.  Unlike the historical entry point above, this
// always seeds top-k with the complete source facet F, including when the
// proposal P is empty.  The spatial core validates F and P, evaluates every
// distinct PointId in F union P exactly once, keeps only the exact best K as
// its provisional heap and still completes the strict-pruning traversal and
// equality shell.  The proposal therefore cannot replace the known F bound.
[[nodiscard]] ExactDirectSparseFacetDescentStepTransient
build_exact_direct_sparse_facet_descent_step_transient_with_top_k_proposal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactFacetMiniballResult* certified_source_miniball,
    ExactDirectSparseCertifiedFacetMiniballLookup
        certified_miniball_lookup,
    std::span<const spatial::PointId> proposal_point_ids);

}  // namespace morsehgp3d::hierarchy::detail
