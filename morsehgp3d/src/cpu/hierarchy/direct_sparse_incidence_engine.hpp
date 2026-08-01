#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_first_incidence.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace morsehgp3d::hierarchy::detail {

// Private shared outcome for the one-point-coface LBVH engine.  The public
// first-incidence API always invokes it without a threshold.  Successive
// incidence supplies an exact exclusive threshold and adapts this outcome to
// its own public receipt; the geometry, bounds, support reduction, traversal,
// budget checks and atomic minimizer shell remain a single implementation.
struct ExactDirectSparseIncidenceEngineOutcome {
  ExactDirectSparseFirstIncidenceResult traversal;
  std::optional<exact::ExactLevel> exclusive_lower_squared_level;
  std::size_t at_or_below_exclusive_threshold_point_count{};
};

[[nodiscard]] ExactDirectSparseIncidenceEngineOutcome
build_exact_direct_sparse_incidence_engine(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    std::span<const spatial::PointId> incumbent_seed_point_ids,
    std::optional<exact::ExactLevel> exclusive_lower_squared_level,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order);

}  // namespace morsehgp3d::hierarchy::detail
