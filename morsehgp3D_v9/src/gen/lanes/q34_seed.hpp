#pragma once

#include "lanes/exact_ball.hpp"
#include "lanes/q4_family.hpp"

#include <array>
#include <functional>
#include <limits>
#include <span>

namespace mhgp9::gen {

// One positive presentation per accepted q4 root of THIS seed, plus its q3
// ball if accepted. This is not a deduplicated catalogue or all incidences.
// arity is the presentation's size, NOT the ball's global minimal support.
struct Q34SeedCandidate {
  unsigned arity{};
  std::array<std::size_t, 4> support_ids{};  // Sorted first arity entries.
  ExactBall ball;
  std::size_t depth{};  // Exact strict interior count, never a clipped value.
  // Disjoint complete-shell parts, each sorted by ORIGINAL ID. q3 uses
  // shell_first only; q4 uses root IDs then the family's constant shell.
  std::span<const std::size_t> shell_first;
  std::span<const std::size_t> shell_second;
};

struct Q34SeedWork {
  u64 seed_owner_tests{}, seed_owner_rejections{};
  u64 q3_point_tests{}, q3_shell_ids{}, q3_depth_rejections{}, q3_emitted{};
  u64 q3_shell_capacity_bytes{};
  u64 q4_depth_rejected_groups{}, q4_depth_skipped_ids{};
  u64 q4_presentations{}, q4_owner_tests{}, q4_owner_rejections{};
  u64 q4_positive_tests{}, q4_positive_rejections{};
  u64 q4_seed_tests{}, q4_seed_rejections{};
  u64 q4_groups_without_support{}, q4_unexamined_after_emit{}, q4_emitted{};
  Q4FamilyWork family;
};

using Q34SeedConsumer = std::function<void(const Q34SeedCandidate&)>;

// Input is an oriented (a,b,x) STRICTLY ACUTE seed; its first two IDs name
// the proposed owner edge. Ownership = longest edge, ties by smallest sorted
// pair of original IDs. Reversed a/b is allowed. Non-owned seeds return no
// candidates, with accounted owner tests. Invalid IDs, nonacute triangles,
// null owner/callback and kmax=0 throw before any emission.
//
// q3 is tested when kmax>=2, at depth<kmax-1. Independently q4 is swept when
// kmax>=3, at depth<kmax-2. A q3 rejection never suppresses the q4 family.
// Each q4 root tries its IDs until a strictly positive tetrahedron has this
// owner edge and x is the smallest-ID acute incident face vertex. The first
// VALID presentation is emitted; no claim about other presentations is made.
// No q2 acceptance, q3 acceptance, chord or approximate predicate gates q4.
//
// Callback records/spans are borrowed only during the callback. The call owns
// the cloud; exceptions propagate and leave prior emissions with the caller.
// Copy the ExactBall/value fields if retaining them. Emitted depths count all
// strict interiors, but their IDs are NOT emitted; collect once after future
// global deduplication. A rejected q3 may stop before consuming all sites.
// Work O(n+e log(1+e)) and buffers O(n) for ONE supplied seed, plus the user's
// callback work. Neither the number of seeds nor the full pipeline is bounded.
[[nodiscard]] Q34SeedWork run_q34_seed_candidates(
    CloudPtr cloud, std::array<std::size_t, 3> seed_ids, std::size_t kmax,
    const Q34SeedConsumer& consumer);

}  // namespace mhgp9::gen
