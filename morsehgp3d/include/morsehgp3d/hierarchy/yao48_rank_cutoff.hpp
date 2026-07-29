#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"

#include <cstdint>

namespace morsehgp3d::hierarchy {

enum class ExactYao48DirectionalCutoffSemantics : std::uint8_t {
  closed_ball,
  strict_interior,
};

// Exact one-direction decision shared by scalable range frontiers and bounded
// design oracles.  witness_squared_radius_upper_bound may be any positive
// upper bound for K distinct certified witnesses in the target's half-open
// Yao48 chamber; the witnesses need not be the K nearest.  A looser bound
// only loses prunes.  closed_ball uses three non-strict comparisons.
// strict_interior uses three strict comparisons and can therefore credit the
// RelevantGP lane.  The caller remains responsible for witness identities,
// distinctness, chamber membership and distance bounds.  No exhaustive pair
// producer is linked by this primitive.
[[nodiscard]] bool exact_yao48_directional_witness_radius_cutoff_certifies(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& witness_squared_radius_upper_bound,
    ExactYao48DirectionalCutoffSemantics semantics);

// Compatibility spelling for the exact-nearest bounded oracle.  Equality is
// a rejection because that oracle classifies the closed diametral rank.
[[nodiscard]] bool exact_yao48_directional_rank_cutoff_certifies_above(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& kth_squared_distance);

}  // namespace morsehgp3d::hierarchy
