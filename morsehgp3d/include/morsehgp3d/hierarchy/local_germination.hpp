#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <functional>

namespace morsehgp3d::hierarchy {

// Reference host generator of the certified local germination of minimal
// well-centred supports of size three and four.
//
// It replaces the product subdivision, whose cost is measured proportional to
// C(n,3)+C(n,4), by an enumeration seeded on the DIAMETER PAIR of each support.
// Completeness rests on two proved statements, both recorded with their proofs
// in docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md:
//
//   * Jung -- a minimal well-centred support has its circumball AS its
//     miniball, so r <= gamma_m diam(U) with gamma_3 = 1/sqrt(3) for a planar
//     triangle and gamma_4 = sqrt(3/8) for a tetrahedron; together with the
//     trivial diam(U) <= 2r this confines the circumradius to the COMPACT
//     interval [D/2, gamma_m D] once the diameter edge is fixed;
//   * the cascade -- with the diameter edge fixed, the locus of compatible
//     circumcentres is a disc of radius sqrt(gamma^2 - 1/4) D in the
//     perpendicular bisector plane, and adding a third vertex reduces it to a
//     segment of half-length sqrt(gamma^2 D^2 - r_triangle^2).
//
// Every rejection exhibits a ball PROVED INSIDE the circumball and finds it
// over-populated, so no accepted support can be lost.  The generator emits
// candidates; it decides nothing.  The exact terminal classification --
// analyze_circumcenter_support_integer then the indexed closed-ball query --
// remains the sole authority on acceptance, unchanged.
//
// Safety discipline.  Covering positions are computed in binary64 and their
// test radii are then shrunk by a certified margin that dominates both the
// covering radius and the floating-point error, and the population count only
// counts points PROVED inside.  Both approximations can only under-count and
// under-reject, never fabricate a rejection.  This is the same rule the
// interval filters follow: an approximation may cost work, never a verdict.
//
// This host reference deliberately scans the cloud for its neighbourhoods
// instead of traversing the LBVH.  It exists to certify completeness against
// the exhaustive census and to count the work an implementation would do; a
// device implementation must use the index.

struct LocalGerminationConfig {
  // s_max = min(K+1, n), exactly as the higher-support stream derives it.
  std::size_t maximum_relevant_closed_rank{};
  // Hexagonal rings covering the disc of compatible centres (J7).  Zero means
  // the single-ball seed test (J6).
  std::size_t seed_disc_ring_count{2U};
  // Positions covering the segment of compatible centres (J8).  One means the
  // single-ball incremental test.
  std::size_t segment_position_count{16U};
};

struct LocalGerminationCounters {
  std::size_t pairs_examined{};
  std::size_t pairs_retained{};
  std::size_t third_vertices_examined{};
  std::size_t third_vertices_free_rejected{};
  std::size_t third_vertices_retained{};
  std::size_t triple_candidates{};
  std::size_t quadruple_candidates{};
  std::size_t population_queries{};

  [[nodiscard]] std::size_t candidates() const noexcept {
    return triple_candidates + quadruple_candidates;
  }
};

// Receives every emitted candidate: the support ids in strictly increasing
// order, and its size.
using LocalGerminationSink =
    std::function<void(const std::array<spatial::PointId, 4>&, std::size_t)>;

// Emits candidate supports of size `support_size`, keyed on a pair that
// realises the support's diameter.  The GUARANTEE is completeness: every
// minimal well-centred support of closed rank at most
// `maximum_relevant_closed_rank` is emitted at least once.
//
// It is deliberately NOT uniqueness.  When a support's diameter is attained by
// several pairs, each of them owns it and emits it, so the consumer must
// deduplicate.  Breaking the tie here would mean comparing distances for
// equality in binary64, which is exactly the kind of fragile decision this
// project refuses to take outside exact arithmetic; the exact tie-break belongs
// with the exact classifier, which sees the support anyway.
[[nodiscard]] LocalGerminationCounters generate_local_germination_candidates(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t support_size,
    const LocalGerminationConfig& config,
    const LocalGerminationSink& sink);

}  // namespace morsehgp3d::hierarchy
