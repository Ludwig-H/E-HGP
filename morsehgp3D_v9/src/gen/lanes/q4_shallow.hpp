#pragma once

#include "lanes/q4_shallow_set.hpp"
#include "lanes/q34_seed.hpp"

namespace mhgp9::gen {

struct Q4ShallowSweepWork {
  Q4FamilyWork family;
  u64 seed_queries{}, seed_owner_tests{}, seed_owner_rejections{}, removed_seed_rejections{};
  u64 membership_comparisons{}, depth_rejected_groups{}, depth_skipped_ids{};
  u64 presentations{}, owner_tests{}, owner_rejections{}, positive_tests{}, positive_rejections{};
  u64 canonical_tests{}, canonical_rejections{}, groups_without_support{}, unexamined_after_emit{};
  u64 emitted{}, shell_ids{}, peak_buffer_bytes{};
};

struct Q4ShallowEdgeWork {
  Q4LocalGeometryWork geometry;
  Q4ShallowSetWork selection;
  Q4ShallowSweepWork sweep;
  u64 seed_candidates{}, acute_tests{}, acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{};
  u64 peak_live_buffer_bytes{};
};

// Q4 only, same positive/owner/canonical candidate contract as q34_cover.
// The set's K is authoritative. A removed acute owned seed returns no output:
// its entire line has retained depth>=K-2. Invalid IDs/nonacute seeds and
// empty callbacks throw before any emission, even if the seed was removed.
// At retained depth<K-2 ALL removed witnesses are strictly exterior, hence
// retained depth and COMPLETE shell equal the full cover's. This is not a
// minorant added to another census; the retained census always starts at zero.
[[nodiscard]] Q4ShallowSweepWork run_q4_shallow_seed_candidates(
    Q4ShallowSetPtr witnesses, std::size_t x_id, const Q34SeedConsumer& consumer);

// Prepare one dual-layer selection per supplied edge, then sweep only the
// retained owned acute seeds against retained witnesses. No cloud/index copy,
// no q2/q3 acceptance gate, no atlas28 or pool26 prefix. Private buffers are
// reused between seeds; different calls may share an immutable witness set.
// K1/2 is valid empty q4 after argument validation. Exceptions leave previous
// callbacks emitted and return no successful partial work report.
// Selection O(m log(1+m)+(K-2)*m), then O(S*r log(1+r)), r retained sites,
// S retained owned acute seeds. r can equal m: NO global subquadratic claim.
[[nodiscard]] Q4ShallowEdgeWork run_q4_shallow_edge_candidates(
    Q34EdgeCoverPtr cover, std::size_t kmax, const Q34SeedConsumer& consumer);

}  // namespace mhgp9::gen
