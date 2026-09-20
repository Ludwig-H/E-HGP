#pragma once

#include "lanes/q4_shallow.hpp"

namespace mhgp8 {

struct Q4WindowSelectionWork {
  u64 seed_queries{}, entry_heap_insertions{}, exit_heap_insertions{};
  u64 entry_heap_replacements{}, exit_heap_replacements{}, heap_comparisons{}, heap_sort_comparisons{};
  u64 constant_rejected_seeds{}, disjoint_rejected_seeds{}, fixed_depth_rejected_seeds{};
  u64 lower_bounds{}, upper_bounds{}, point_windows{}, second_pass_sites{}, window_comparisons{};
  // A point window owns its single endpoint in lower_ids; upper_ids is zero.
  u64 lower_ids{}, upper_ids{}, inner_ids{}, outside_ids{}, fixed_inside_sites{}, rejected_event_ids{};
  u64 max_inner_ids{}, max_endpoint_ids{}, peak_heap_bytes{}, peak_buffer_bytes{};
  bool operator==(const Q4WindowSelectionWork&) const = default;
};

struct Q4WindowSweepWork {
  // Deliberately adapted ledger: family.sites and all classifications count
  // FIRST-pass retained sites only; event_count counts ALL nonconstant roots.
  // family.sort_comparisons sorts STRICT window-interior roots only;
  // family.groups/callbacks count processed WINDOW groups, not all roots.
  // family.retained_capacity_bytes and sweep.peak_buffer_bytes include ALL
  // SIX simultaneously live ID-vector capacities, not just two family lists.
  Q4ShallowSweepWork sweep;
  Q4WindowSelectionWork window;
};

struct Q4WindowEdgeWork {
  Q4LocalGeometryWork geometry;
  Q4ShallowSetWork selection;
  Q4WindowSweepWork sweep;
  u64 seed_candidates{}, acute_tests{}, acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{};
  u64 peak_live_buffer_bytes{};
};

// Q4 only, same exact candidate and synchronous borrowed-view contract as29.
// For T=K-2 and c constant strict interiors, keep heaps of up to T smallest
// entry roots and T largest exit roots while scanning ALL retained sites.
// If c<T, H=T-c>=1: the closed possible window is [H-th largest exit,
// H-th smallest entry], with missing bounds represented by infinities.
// A second COMPLETE pass keeps both endpoints with every tied original ID,
// strict interior events, and the exact fixed contribution outside the window.
// Only strict-interior events are sorted; there are at most 2H-2 such IDs.
// Endpoints and the constant shell inherit original-ID order from the set.
// L==U is processed once, exits BEFORE strict count, entries AFTER; neither
// count saturation nor an open-window shortcut is allowed.
//
// Per seed, O(r log(1+min(T,r))+min(T,r) log(1+min(T,r)) + output/contacts),
// with at most two r-site scans and O(r+min(T,r)) buffers; fixed K is linear
// per seed, but S seeds still cause O(S*r) scans. NO global subquadratic,
// q3, catalogue, FULL or GPU qualification follows. No reserve(T) allocation.
//
// Invalid owner/callback, out-of-range IDs and nonacute seeds fail before
// emissions; removed owned acute seeds are rejected by the29 certificate.
// Allocation, arithmetic-counter or callback exceptions propagate, leaving
// earlier callbacks emitted and no successful partial work report. Immutable
// input ownership is retained for the call; concurrent calls need own buffers.
[[nodiscard]] Q4WindowSweepWork run_q4_window_seed_candidates(
    Q4ShallowSetPtr witnesses, std::size_t x_id, const Q34SeedConsumer& consumer);

// One supplied edge. Preparation is the unchanged29 exact shallow selection;
// only its retained owned acute seeds are visited. K1/2 returns empty q4 after
// argument validation. The peak counts simultaneous owned dynamic capacities
// of selection/geometry or retained IDs plus the six private buffers. Fixed
// objects, shared cloud/index, allocator overlap/metadata and callback memory
// are outside this metric; it is not an RSS bound.
[[nodiscard]] Q4WindowEdgeWork run_q4_window_edge_candidates(
    Q34EdgeCoverPtr cover, std::size_t kmax, const Q34SeedConsumer& consumer);

}  // namespace mhgp8
