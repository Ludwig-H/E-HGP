#pragma once

// MorseHGP3D v9 (23 septembre 2026) — S3: portable copy (host and CUDA device)
// of the dead-lane certificate of one surviving q3/q4 edge, as run by
// Engine::filtered_edge (pipeline/wspd_q34.cpp): the diametral core
// (Q34EdgeCover::make_diametral) and its certificate first, then, for the
// lanes it leaves open, the cover (Q34EdgeCover::make) and its certificate
// (lanes/q34_dead_lanes.cpp). Same integer arithmetic, same site order, same
// cells, same early stops and the same work counters, field by field.
//
// One edge is processed by a GROUP of 32 lanes: the tree walk and the cell
// recursion are uniform (every lane runs them); the frontier scans are
// split across the lanes with ballots, the exact stopping position of the
// sequential scan is recovered from the ballot masks, and the partial sites
// are compacted in scan order. HostGroup emulates the group sequentially,
// WarpGroup (filter_runner.cu) maps it to one CUDA warp.
//
// Exactness is judged, not assumed: the host compilation is compared edge by
// edge, counter by counter, with the product prover on LiDAR edges
// (tests/gpu/certificate_port_gate.cpp); the device run is compared with the
// CPU reference in the chain differential gates and on G4.
//
// Memory: the caller gives each group a fixed slab of `capacity` sites
// (ranges, three form arrays, and one frontier per scanned depth). An edge
// whose core or cover exceeds it returns Deferred with no counter: the CPU
// then runs the whole engine edge. Deferral is a memory decision only.

#include "witness_filter.hpp"

namespace mhgp9::gpu {

using u64 = unsigned long long;

// Q34EdgeCoverWork, field by field.
struct CoverWork {
  u64 node_visits, bound_tests, point_tests, admitted_nodes, rejected_nodes, split_nodes;
  u64 admitted_sites, rejected_sites, retained_ranges, merged_ranges;
};

// Q34DeadLaneWork, field by field.
struct DeadWork {
  u64 loads, form_sites, cells, outside_cells, deep_cells, failed_cells;
  u64 uniform_tests, point_tests, q3_proved, q3_open, q4_proved, q4_open;
};

// The certificate part of WspdQ34Work (core_*, cover_*, dead, dead_core).
struct CertificateWork {
  u64 core_builds, core_sites, core_closed_edges;
  CoverWork core_cover;
  DeadWork dead_core;
  u64 cover_builds, cover_sites, max_cover_sites;
  CoverWork cover;
  DeadWork dead;
};

MHGP9_HD inline void add_cover(CoverWork& to, const CoverWork& from) {
  to.node_visits += from.node_visits; to.bound_tests += from.bound_tests; to.point_tests += from.point_tests;
  to.admitted_nodes += from.admitted_nodes; to.rejected_nodes += from.rejected_nodes;
  to.split_nodes += from.split_nodes; to.admitted_sites += from.admitted_sites;
  to.rejected_sites += from.rejected_sites; to.retained_ranges += from.retained_ranges;
  to.merged_ranges += from.merged_ranges;
}

MHGP9_HD inline void add_dead(DeadWork& to, const DeadWork& from) {
  to.loads += from.loads; to.form_sites += from.form_sites; to.cells += from.cells;
  to.outside_cells += from.outside_cells; to.deep_cells += from.deep_cells; to.failed_cells += from.failed_cells;
  to.uniform_tests += from.uniform_tests; to.point_tests += from.point_tests;
  to.q3_proved += from.q3_proved; to.q3_open += from.q3_open;
  to.q4_proved += from.q4_proved; to.q4_open += from.q4_open;
}

// The same counters for ONE edge, in u32 (device registers): a walk visits
// fewer than node_count (< 2^32) nodes, a load at most `capacity` sites, a
// proof at most 1+4+...+4^6 = 5461 cells; the frontier tests (up to cells x
// capacity) stay in u64.
struct EdgeCoverWork {
  u32 node_visits, bound_tests, point_tests, admitted_nodes, rejected_nodes, split_nodes;
  u32 admitted_sites, rejected_sites, retained_ranges, merged_ranges;
};
struct EdgeDeadWork {
  u32 loads, form_sites, cells, outside_cells, deep_cells, failed_cells;
  u64 uniform_tests, point_tests;
  u32 q3_proved, q3_open, q4_proved, q4_open;
};
struct EdgeWork {
  u32 core_builds, core_sites, core_closed_edges;
  EdgeCoverWork core_cover;
  EdgeDeadWork dead_core;
  u32 cover_builds, cover_sites, max_cover_sites;
  EdgeCoverWork cover;
  EdgeDeadWork dead;
};

MHGP9_HD inline void add_edge_cover(CoverWork& to, const EdgeCoverWork& from) {
  to.node_visits += from.node_visits; to.bound_tests += from.bound_tests; to.point_tests += from.point_tests;
  to.admitted_nodes += from.admitted_nodes; to.rejected_nodes += from.rejected_nodes;
  to.split_nodes += from.split_nodes; to.admitted_sites += from.admitted_sites;
  to.rejected_sites += from.rejected_sites; to.retained_ranges += from.retained_ranges;
  to.merged_ranges += from.merged_ranges;
}

MHGP9_HD inline void add_edge_dead(DeadWork& to, const EdgeDeadWork& from) {
  to.loads += from.loads; to.form_sites += from.form_sites; to.cells += from.cells;
  to.outside_cells += from.outside_cells; to.deep_cells += from.deep_cells; to.failed_cells += from.failed_cells;
  to.uniform_tests += from.uniform_tests; to.point_tests += from.point_tests;
  to.q3_proved += from.q3_proved; to.q3_open += from.q3_open;
  to.q4_proved += from.q4_proved; to.q4_open += from.q4_open;
}

// One edge's work into the totals; max_cover_sites by MAX.
MHGP9_HD inline void add_edge(CertificateWork& to, const EdgeWork& from) {
  to.core_builds += from.core_builds; to.core_sites += from.core_sites;
  to.core_closed_edges += from.core_closed_edges;
  add_edge_cover(to.core_cover, from.core_cover);
  add_edge_dead(to.dead_core, from.dead_core);
  to.cover_builds += from.cover_builds; to.cover_sites += from.cover_sites;
  to.max_cover_sites = to.max_cover_sites < from.max_cover_sites ? from.max_cover_sites : to.max_cover_sites;
  add_edge_cover(to.cover, from.cover);
  add_edge_dead(to.dead, from.dead);
}

// Sums; max_cover_sites by MAX, as merge() in pipeline/wspd_q34.cpp.
MHGP9_HD inline void add_certificate(CertificateWork& to, const CertificateWork& from) {
  to.core_builds += from.core_builds; to.core_sites += from.core_sites;
  to.core_closed_edges += from.core_closed_edges;
  add_cover(to.core_cover, from.core_cover);
  add_dead(to.dead_core, from.dead_core);
  to.cover_builds += from.cover_builds; to.cover_sites += from.cover_sites;
  to.max_cover_sites = to.max_cover_sites < from.max_cover_sites ? from.max_cover_sites : to.max_cover_sites;
  add_cover(to.cover, from.cover);
  add_dead(to.dead, from.dead);
}

// Flat index with escape links (preorder: a node, its left subtree, its
// right subtree; escape = first node after the subtree, node_count at the
// end) and the coordinates in spatial rank order (x, y, z per rank).
struct CertificateIndex {
  const FlatNode* nodes;
  const u32* escapes;
  u32 node_count;
  const std::int32_t* rank_points;
};

// Per-group scratch: `capacity` sites. ranges holds 2*capacity u32 (first,
// last), forms three i64 arrays, frontiers one u32 array per scanned depth
// (depths min_depth..max_depth, see levels below).
struct CertificateSlab {
  u32* ranges;
  i64* form_constant;
  i64* form_x;
  i64* form_y;
  u32* frontiers;
  u32 capacity;
};

inline constexpr unsigned prover_max_depth = 6;  // Q34DeadLaneProver defaults
inline constexpr unsigned prover_min_depth = 2;
inline constexpr unsigned prover_levels = prover_max_depth - prover_min_depth + 1;

enum class CertificateStatus : u8 { decided = 0, deferred = 1, fault = 2 };

struct CertificateResult {
  CertificateStatus status;
  u8 mask;  // lanes left open (decided only)
};

// ---- Groups -----------------------------------------------------------

// Sequential emulation of a 32-lane group.
struct HostGroup {
  static constexpr u32 size = 32;
  // Bit l of the result: code(base + l) & which, for base + l < count.
  template <class Code>
  void ballot2(u32 base, u32 count, Code code, u32& first, u32& second) const {
    first = 0;
    second = 0;
    for (u32 lane = 0; lane < size && base + lane < count; ++lane) {
      const u32 c = code(base + lane);
      if ((c & 1U) != 0) first |= 1U << lane;
      if ((c & 2U) != 0) second |= 1U << lane;
    }
  }
  // f(lane, rank) for every set lane, rank = set lanes below it.
  template <class F>
  void for_set(u32 mask, F f) const {
    u32 rank = 0;
    for (u32 lane = 0; lane < size; ++lane)
      if (((mask >> lane) & 1U) != 0) f(lane, rank++);
  }
  // f(i) for i in [0, count), split across the lanes.
  template <class F>
  void for_each(u32 count, F f) const {
    for (u32 i = 0; i < count; ++i) f(i);
  }
  [[nodiscard]] bool leader() const { return true; }
  void sync() const {}
};

MHGP9_HD inline u32 popcount32(u32 x) {
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__popc(x));
#else
  return static_cast<u32>(__builtin_popcount(x));
#endif
}

// Lane index of the n-th (1-based) set bit of mask; n <= popcount(mask).
MHGP9_HD inline u32 nth_set_bit(u32 mask, u32 n) {
  for (u32 lane = 0; lane < 32; ++lane)
    if (((mask >> lane) & 1U) != 0 && --n == 0) return lane;
  return 32;  // unreachable when n <= popcount(mask)
}

// ---- Cover of an edge (Q34EdgeCover::build) ----------------------------

struct Ball {
  i64 center_twice[3];
  i64 radius_fourfold;
};

MHGP9_HD inline Ball edge_ball(const std::int32_t a[3], const std::int32_t b[3], bool diametral) {
  Ball ball{};
  for (int axis = 0; axis < 3; ++axis) {
    ball.center_twice[axis] = static_cast<i64>(a[axis]) + b[axis];
    const i64 delta = static_cast<i64>(b[axis]) - a[axis];
    ball.radius_fourfold += (diametral ? 1 : 4) * delta * delta;
  }
  return ball;
}

// Retained ranges, merged across adjacent ranks, in increasing spatial
// order; returns false (no usable output) when the sites exceed capacity.
template <class Group>
MHGP9_HD bool build_cover(const Group& group, const CertificateIndex& index, const Ball& ball,
                          const CertificateSlab& slab, u32& range_count, u32& sites, EdgeCoverWork& work) {
  range_count = 0;
  sites = 0;
  u32 cursor = 0, previous_last = absent32;  // end of the last retained range (uniform)
  while (cursor < index.node_count) {
    const FlatNode& node = index.nodes[cursor];
    ++work.node_visits;
    const u32 size = node.last - node.first;
    bool admit = false, reject = false;
    if (size == 1) {
      ++work.point_tests;
      const std::int32_t* p = index.rank_points + 3 * static_cast<std::size_t>(node.first);
      i64 norm = 0;
      for (int axis = 0; axis < 3; ++axis) {
        const i64 delta = 2 * static_cast<i64>(p[axis]) - ball.center_twice[axis];
        norm += delta * delta;
      }
      admit = norm <= ball.radius_fourfold;
      reject = !admit;
    } else {
      ++work.bound_tests;
      i64 minimum = 0, maximum = 0;
      for (int axis = 0; axis < 3; ++axis) {
        const i64 low = 2 * static_cast<i64>(node.box.low[axis]) - ball.center_twice[axis];
        const i64 high = 2 * static_cast<i64>(node.box.high[axis]) - ball.center_twice[axis];
        const i64 nearest = low > 0 ? low : (high < 0 ? high : 0);
        minimum += nearest * nearest;
        maximum += max_i64(low * low, high * high);
      }
      reject = minimum > ball.radius_fourfold;
      admit = !reject && maximum <= ball.radius_fourfold;
    }
    if (admit) {
      ++work.admitted_nodes;
      work.admitted_sites += size;
      if (size > slab.capacity - sites) return false;
      sites += size;
      if (range_count != 0 && previous_last == node.first) {
        if (group.leader()) slab.ranges[2 * (range_count - 1) + 1] = node.last;
        ++work.merged_ranges;
      } else {
        if (group.leader()) {
          slab.ranges[2 * range_count] = node.first;
          slab.ranges[2 * range_count + 1] = node.last;
        }
        ++range_count;
        ++work.retained_ranges;
      }
      previous_last = node.last;
      cursor = index.escapes[cursor];
    } else if (reject) {
      ++work.rejected_nodes;
      work.rejected_sites += size;
      cursor = index.escapes[cursor];
    } else {
      ++work.split_nodes;
      cursor = node.left;
    }
  }
  group.sync();  // the leader's ranges, read by every lane of load_forms
  return true;
}

// ---- Dead-lane prover (Q34DeadLaneProver) ------------------------------

struct Prover {
  i64 a_basis[3], b_basis[3];
  i64 diameter_squared;
  u32 threshold3, threshold4;
  u32 n;  // loaded sites (forms), a and b included
};

inline constexpr i64 prover_scale = i64{1} << 20;
inline constexpr i64 q3_disk_factor = 3;
inline constexpr i64 q4_disk_factor = 2;

MHGP9_HD inline i64 abs_i64(i64 x) { return x < 0 ? -x : x; }

// load(): forms of every site of the ranges, in range order.
template <class Group>
MHGP9_HD Prover load_forms(const Group& group, const CertificateIndex& index, const std::int32_t a[3],
                           const std::int32_t b[3], const CertificateSlab& slab, u32 range_count, u32 sites,
                           EdgeDeadWork& work) {
  Prover p{};
  i64 v[3], midpoint_twice[3];
  int main_axis = 0;
  for (int i = 0; i < 3; ++i) {
    v[i] = static_cast<i64>(b[i]) - a[i];
    midpoint_twice[i] = static_cast<i64>(a[i]) + b[i];
    if (abs_i64(v[i]) > abs_i64(v[main_axis])) main_axis = i;
  }
  const int axis_i = (main_axis + 1) % 3, axis_j = (main_axis + 2) % 3;
  const i64 h = abs_i64(v[main_axis]), sign = v[main_axis] > 0 ? 1 : -1;
  for (int i = 0; i < 3; ++i) p.a_basis[i] = p.b_basis[i] = 0;
  p.a_basis[axis_i] = h;
  p.a_basis[main_axis] = -sign * v[axis_i];
  p.b_basis[axis_j] = h;
  p.b_basis[main_axis] = -sign * v[axis_j];
  p.diameter_squared = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  ++work.loads;
  const i64 am = p.a_basis[main_axis], ai = p.a_basis[axis_i], bm = p.b_basis[main_axis], bj = p.b_basis[axis_j];
  u32 offset = 0;
  for (u32 r = 0; r < range_count; ++r) {
    const u32 first = slab.ranges[2 * r], last = slab.ranges[2 * r + 1];
    group.for_each(last - first, [&](u32 i) {
      const std::int32_t* z = index.rank_points + 3 * static_cast<std::size_t>(first + i);
      i64 w[3];
      for (int k = 0; k < 3; ++k) w[k] = 2 * static_cast<i64>(z[k]) - midpoint_twice[k];
      slab.form_constant[offset + i] = prover_scale * (w[0] * w[0] + w[1] * w[1] + w[2] * w[2] - p.diameter_squared);
      slab.form_x[offset + i] = -2 * (w[main_axis] * am + w[axis_i] * ai);
      slab.form_y[offset + i] = -2 * (w[main_axis] * bm + w[axis_j] * bj);
    });
    offset += last - first;
  }
  group.sync();
  p.n = sites;
  work.form_sites += sites - 2;  // caller guarantees sites >= 2 (a and b)
  return p;
}

struct ProverCell {
  i64 left, right, bottom, top;
};

MHGP9_HD inline bool cell_outside(const Prover& p, const ProverCell& c, i64 disk_factor) {
  i128 norm = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 x = p.a_basis[i], y = p.b_basis[i];
    const i64 low = x * (x < 0 ? c.right : c.left) + y * (y < 0 ? c.top : c.bottom);
    const i64 high = x * (x < 0 ? c.left : c.right) + y * (y < 0 ? c.bottom : c.top);
    const i128 nearest = low > 0 ? low : (high < 0 ? high : 0);
    norm += nearest * nearest;
  }
  return static_cast<i128>(disk_factor) * norm >
         static_cast<i128>(p.diameter_squared) * prover_scale * prover_scale;
}

MHGP9_HD inline bool center_inside(const Prover& p, i64 alpha, i64 beta, i64 disk_factor) {
  i128 norm = 0;
  for (int i = 0; i < 3; ++i) {
    const i128 t = static_cast<i128>(p.a_basis[i]) * alpha + static_cast<i128>(p.b_basis[i]) * beta;
    norm += t * t;
  }
  return static_cast<i128>(disk_factor) * norm <=
         static_cast<i128>(p.diameter_squared) * prover_scale * prover_scale;
}

// A frame of the explicit cell recursion (cell() of the prover), kept once
// its own scan is done and its children are being visited.
struct ProverFrame {
  ProverCell cell;
  u32 frontier_level;  // 0: identity 0..n-1; d+1: frontiers of depth d
  u32 frontier_size;
  u32 inside;
  u8 depth, lanes, need, child;
};

// Outcome of entering a cell: a returned lane set, or a frame to descend.
struct CellEntry {
  bool done;
  u8 value;
  ProverFrame frame;
};

template <class Group>
MHGP9_HD CellEntry enter_cell(const Group& group, const Prover& p, const CertificateSlab& slab, const ProverCell& c,
                              u8 depth, u32 frontier_level, u32 frontier_size, u32 inherited, u8 lanes,
                              EdgeDeadWork& work) {
  ++work.cells;
  u8 need = 0;
  if ((lanes & 2U) != 0 && !cell_outside(p, c, q3_disk_factor)) need = static_cast<u8>(need | 2U);
  if ((lanes & 4U) != 0 && !cell_outside(p, c, q4_disk_factor)) need = static_cast<u8>(need | 4U);
  if (need == 0) {
    ++work.outside_cells;
    return CellEntry{true, lanes, {}};
  }
  u32 inside = inherited;
  if (depth >= prover_min_depth) {
    const u32 target = (need & 2U) != 0 ? p.threshold3 : p.threshold4;
    if (inside >= target) {
      ++work.deep_cells;
      return CellEntry{true, lanes, {}};
    }
    // Every earlier read or write of this depth's frontier (a sibling or an
    // earlier cell at this depth, by any lane, possibly left by an early
    // return) is ordered before this cell's writes (review of 23 September:
    // ballots alone give no memory ordering on the device).
    group.sync();
    const u32* frontier =
        frontier_level == 0 ? nullptr : slab.frontiers + static_cast<std::size_t>(frontier_level - 1) * slab.capacity;
    u32* next = slab.frontiers + static_cast<std::size_t>(depth - prover_min_depth) * slab.capacity;
    u32 next_size = 0;
    const auto id_at = [&](u32 i) { return frontier == nullptr ? i : frontier[i]; };
    for (u32 base = 0; base < frontier_size; base += 32) {
      u32 uniform = 0, partial = 0;
      group.ballot2(base, frontier_size, [&](u32 i) {
        const u32 id = id_at(i);
        const i64 fc = slab.form_constant[id], fx = slab.form_x[id], fy = slab.form_y[id];
        const i64 maximum = fc + fx * (fx < 0 ? c.left : c.right) + fy * (fy < 0 ? c.bottom : c.top);
        if (maximum < 0) return 1U;
        const i64 minimum = fc + fx * (fx < 0 ? c.right : c.left) + fy * (fy < 0 ? c.top : c.bottom);
        return minimum < 0 ? 2U : 0U;
      }, uniform, partial);
      const u32 found = popcount32(uniform);
      if (inside + found >= target) {
        // The sequential scan stops at the (target-inside)-th uniform site.
        work.uniform_tests += base + nth_set_bit(uniform, target - inside) + 1;
        ++work.deep_cells;
        return CellEntry{true, lanes, {}};
      }
      inside += found;
      group.for_set(partial, [&](u32 lane, u32 rank) { next[next_size + rank] = id_at(base + lane); });
      next_size += popcount32(partial);
    }
    group.sync();
    work.uniform_tests += frontier_size;
    if ((need & 4U) != 0 && inside >= p.threshold4) need = static_cast<u8>(need & ~4U);
    u32 count = inside;
    u64 points = 0;
    bool counted = false, refuted = false;
    for (int step = 0; step < 2; ++step) {  // lane 2 (q3) then 4 (q4), no initializer_list on the device
      const u8 lane = step == 0 ? u8{2} : u8{4};
      if ((need & lane) == 0) continue;
      const u32 lane_target = lane == 2U ? p.threshold3 : p.threshold4;
      if (!center_inside(p, c.left, c.bottom, lane == 2U ? q3_disk_factor : q4_disk_factor)) continue;
      if (!counted) {
        u64 scanned = next_size;
        for (u32 base = 0; base < next_size; base += 32) {
          u32 negative = 0, unused = 0;
          group.ballot2(base, next_size, [&](u32 i) {
            const u32 id = next[i];
            return slab.form_constant[id] + slab.form_x[id] * c.left + slab.form_y[id] * c.bottom < 0 ? 1U : 0U;
          }, negative, unused);
          const u32 found = popcount32(negative);
          if (count + found >= p.threshold3) {
            scanned = base + nth_set_bit(negative, p.threshold3 - count) + 1;
            count = p.threshold3;
            break;
          }
          count += found;
        }
        points += scanned;
        counted = true;
      }
      if (count < lane_target) {
        need = static_cast<u8>(need & ~lane);
        lanes = static_cast<u8>(lanes & ~lane);
        refuted = true;
      }
    }
    work.point_tests += points;
    if (refuted) ++work.failed_cells;
    if (need == 0) return CellEntry{true, lanes, {}};
    frontier_level = depth - prover_min_depth + 1;
    frontier_size = next_size;
  }
  if (depth == prover_max_depth) {
    ++work.failed_cells;
    return CellEntry{true, static_cast<u8>(lanes & ~need), {}};
  }
  return CellEntry{false, 0, ProverFrame{c, frontier_level, frontier_size, inside, depth, lanes, need, 0}};
}

MHGP9_HD inline ProverCell child_cell(const ProverCell& c, u8 child) {
  const i64 x = c.left + (c.right - c.left) / 2, y = c.bottom + (c.top - c.bottom) / 2;
  switch (child) {
    case 0: return ProverCell{c.left, x, c.bottom, y};
    case 1: return ProverCell{x, c.right, c.bottom, y};
    case 2: return ProverCell{c.left, x, y, c.top};
    default: return ProverCell{x, c.right, y, c.top};
  }
}

// prove(): lanes of `lanes` (subset of 6) proved dead.
template <class Group>
MHGP9_HD u8 prove_lanes(const Group& group, Prover& p, const CertificateSlab& slab, unsigned kmax, u8 lanes,
                        EdgeDeadWork& work) {
  u8 tried = lanes;
  if (kmax < 2) tried = static_cast<u8>(tried & ~2U);
  if (kmax < 3) tried = static_cast<u8>(tried & ~4U);
  p.threshold3 = kmax >= 2 ? kmax - 1 : 0;
  p.threshold4 = kmax >= 3 ? kmax - 2 : 0;
  u8 proved = 0;
  if (tried != 0) {
    ProverFrame stack[prover_max_depth + 1];
    int top = 0;
    const ProverCell root{-2 * prover_scale, 2 * prover_scale, -2 * prover_scale, 2 * prover_scale};
    CellEntry entry = enter_cell(group, p, slab, root, 0, 0, p.n, 0, tried, work);
    if (entry.done) {
      proved = entry.value;
    } else {
      stack[top++] = entry.frame;
      while (top > 0) {
        ProverFrame& f = stack[top - 1];
        const u8 child = f.child++;
        entry = enter_cell(group, p, slab, child_cell(f.cell, child), static_cast<u8>(f.depth + 1),
                           f.frontier_level, f.frontier_size, f.inside, f.need, work);
        if (!entry.done) {
          stack[top++] = entry.frame;
          continue;
        }
        u8 kept = entry.value;
        for (;;) {
          ProverFrame& parent = stack[top - 1];
          parent.lanes = static_cast<u8>(parent.lanes & ~(parent.need & ~kept));
          parent.need = static_cast<u8>(parent.need & kept);
          if (parent.need != 0 && parent.child < 4) break;
          kept = parent.lanes;
          if (--top == 0) {
            proved = kept;
            break;
          }
        }
      }
    }
  }
  if ((lanes & 2U) != 0) ++((proved & 2U) != 0 ? work.q3_proved : work.q3_open);
  if ((lanes & 4U) != 0) ++((proved & 4U) != 0 ? work.q4_proved : work.q4_open);
  return proved;
}

// ---- One surviving edge (Engine::filtered_edge, certificate part) --------

// `mask` is the edge's surviving lanes (nonzero subset of 6). On decided,
// `work` has received exactly the engine's certificate counters of this
// edge and the result holds the lanes left for the q3/q4 generation (zero
// when the core or the cover closed the edge). Deferred and fault add
// nothing to `work`. Only the leader lane writes `work` (on the device it
// is the warp's total in shared memory).
template <class Group>
MHGP9_HD CertificateResult certify_edge(const Group& group, const CertificateIndex& index, u32 a_rank, u32 b_rank,
                                        u8 mask, unsigned kmax, bool dead_core, const CertificateSlab& slab,
                                        CertificateWork& work) {
  const std::int32_t* a = index.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const std::int32_t* b = index.rank_points + 3 * static_cast<std::size_t>(b_rank);
  EdgeWork local{};  // this edge's counters, added to `work` by the leader lane only
  u32 range_count = 0, sites = 0;
  if (dead_core) {
    if (!build_cover(group, index, edge_ball(a, b, true), slab, range_count, sites, local.core_cover))
      return CertificateResult{CertificateStatus::deferred, mask};
    if (sites < 2) return CertificateResult{CertificateStatus::fault, mask};
    ++local.core_builds;
    local.core_sites += sites;
    Prover prover = load_forms(group, index, a, b, slab, range_count, sites, local.dead_core);
    mask = static_cast<u8>(mask & ~prove_lanes(group, prover, slab, kmax, mask, local.dead_core));
    if (mask == 0) {
      ++local.core_closed_edges;
      if (group.leader()) add_edge(work, local);
      return CertificateResult{CertificateStatus::decided, 0};
    }
  }
  if (!build_cover(group, index, edge_ball(a, b, false), slab, range_count, sites, local.cover))
    return CertificateResult{CertificateStatus::deferred, mask};
  if (sites < 2) return CertificateResult{CertificateStatus::fault, mask};
  ++local.cover_builds;
  local.cover_sites += sites;
  local.max_cover_sites = sites;
  Prover prover = load_forms(group, index, a, b, slab, range_count, sites, local.dead);
  mask = static_cast<u8>(mask & ~prove_lanes(group, prover, slab, kmax, mask, local.dead));
  if (group.leader()) add_edge(work, local);
  return CertificateResult{CertificateStatus::decided, mask};
}

}  // namespace mhgp9::gpu
