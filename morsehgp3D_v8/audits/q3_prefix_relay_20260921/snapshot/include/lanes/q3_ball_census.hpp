#pragma once

#include "exact_ball.hpp"
#include "../pipeline/q2_census.hpp"

#include <cstddef>
#include <vector>

namespace mhgp8 {

struct Q3BallCensusWork {
  u64 queries{};
  u64 preparations{};
  u64 vertex_axes{};
  u64 accepted_queries{};
  u64 rejected_queries{};
  u64 count_node_visits{};
  u64 count_bounds_prepared{};
  u64 count_box_bound_tests{};
  u64 count_point_tests{};
  u64 count_inside_nodes{};
  u64 count_inside_sites{};  // Effective credits, saturated at threshold.
  u64 count_nonnegative_nodes{};
  u64 count_nonnegative_sites{};
  u64 count_split_nodes{};
  u64 count_prepared_unvisited{};  // Cached pending bounds at saturation.
  u64 count_saturations{};
  u64 shell_node_visits{};
  u64 shell_bounds_prepared{};
  u64 shell_box_bound_tests{};
  u64 shell_point_tests{};
  u64 shell_excluded_nodes{};
  u64 shell_split_nodes{};
  u64 shell_ids{};
  u64 peak_count_stack{};  // These last three fields merge by MAX.
  u64 peak_shell_stack{};
  u64 stack_storage_bytes{};  // One actual fixed array, reused by both passes.
  bool operator==(const Q3BallCensusWork&) const = default;
};

struct Q3BallCensusResult {
  bool accepted{};
  // Exact strict-interior count if accepted, otherwise the threshold at
  // which the first pass saturated, NOT the uncomputed global depth.
  std::size_t depth{};
  bool operator==(const Q3BallCensusResult&) const = default;
};

// Global index census from ZERO; no cover, local credit, seed list or factor
// scan is supplied or inherited. The q3 pipeline passes make_q3's positive
// ball. ExactBall deliberately does not encode a support arity: this function
// cannot certify its origin. Its arithmetic and census are nevertheless safe
// for every nonforgeable key from the existing q2/q3/q4 checked factories,
// whose coefficient bounds are dominated by q3 (<2^105 for power).
//
// threshold must be positive; invalid arguments leave shell/work unchanged.
// On a valid call shell is cleared, retaining its allocation. The first pass
// counts strict interiors, processing the child with smaller power minimum
// first (ties left). A successful low-depth result triggers a SECOND traversal
// collecting every contact. Returned shell IDs are original IDs, complete and
// distinct, but NOT sorted; sorting remains the caller's explicit paid step.
// On a saturated/rejected result shell is empty. No collection is paid then.
//
// Box bounds are exact on the integer lattice contained in the box, not an
// approximation of a real-valued minimum. A>0 permits per-axis floor/ceil of
// -B/(2A), prepared once and then clamped to each node. No B^2 is formed.
// First-pass min>=0 excludes a node from strict depth; max<0 admits it.
// Second-pass min>0 OR max<0 excludes a node from the shell: tangency survives.
// All prepared child bounds are counted even if saturation prevents a visit.
// count_bounds_prepared = count_node_visits + count_prepared_unvisited;
// count_bounds_prepared = count_box_bound_tests + count_point_tests.
// Analogous shell equalities hold with zero unvisited bounds on success.
//
// Index/ball/work/shell are borrowed synchronously. Concurrent calls may share
// index and ball, not mutable work/shell. Exceptions propagate; work and shell
// may be partial after a valid call starts. Discard any such partial census.
// Checked work additions cannot silently overflow. The only heap allocation
// is growth of the caller-owned shell. The single local49-frame DFS array is
// justified by Q2CensusIndex's midpoint splits on three16-bit coordinates,
// not by a search quota. Work is linear in visited/prepared nodes plus shell
// output; no bound on the total number of balls queried is established here.
[[nodiscard]] Q3BallCensusResult census_q3_ball(
    const Q2CensusIndex& index, const ExactBall& ball, std::size_t threshold,
    std::vector<std::size_t>& shell, Q3BallCensusWork& work);

}  // namespace mhgp8
