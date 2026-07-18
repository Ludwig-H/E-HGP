#pragma once

#include "morsehgp3d/spatial/query.hpp"

#include <cstddef>

namespace morsehgp3d::spatial {

// Exact reference query over X minus exclusions.  The returned cutoff shell is
// always the complete equality class, including IDs not needed by the chosen
// rank-k representative.
[[nodiscard]] TopKPartition brute_force_top_k(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions);

// Rank-one specialization.  All co-minimizers remain in cutoff_shell_ids();
// canonical_choice_ids() contains the smallest canonical ID among them.
[[nodiscard]] TopKPartition brute_force_nearest(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const ExclusionSet& exclusions);

// Complete global partition of X, without exclusions or rank truncation.
[[nodiscard]] ClosedBallPartition brute_force_closed_ball(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const exact::ExactLevel& squared_radius);

}  // namespace morsehgp3d::spatial
