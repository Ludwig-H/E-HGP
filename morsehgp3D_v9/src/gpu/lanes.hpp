#pragma once

// MorseHGP3D v9 (23 septembre 2026) — S4a: portable copy (host and CUDA
// device) of the q3 lane of one certified edge, WITHOUT atlas: the product
// q3 lane of Engine::generate_lanes (pipeline/wspd_q34.cpp) in its existing
// ScalarCover mode (no atlas consultation), with the seeds drawn from the
// cover instead of an index walk.
//
// Why the object is the product's (docs/s4_conception_20260923/PLAN_S4.md):
// - seeds: an owned acute seed x of ab has |x-a|^2 <= D and |x-b|^2 <= D
//   (D=|ab|^2), so |2x-a-b|^2 <= 3D <= 4D: it lies in the closed edge cover
//   (Q34EdgeCover::make), and the SET of seeds is the engine's (the order
//   is the scan order below); the predicate (strictly acute, ab the longest
//   side, ties broken by the sorted ID pair) is the engine's, verbatim;
// - census: the positive owned q3 ball lies inside the cover (comment of
//   q3_seed), so counting the cover sites of negative power up to K-1 is the
//   exact census of ScalarCover; the power is the UNREDUCED relative power
//   G|z-a|^2-W.(z-a) of ExactBall::make_q3 before `primitive` (< 2^116), of
//   the same sign as ExactBall::power (division by a positive gcd);
// - key: `translated` then `primitive` (binary gcd, same divisor), only for
//   an accepted seed.
//
// One edge is processed by a GROUP of 32 lanes. The seed scan and every
// census are split across the lanes by site (ballots); the exact stopping
// site of a rejected census (the (K-1)-th site of negative power in scan
// order) is recovered from the ballot mask, so the counters are those of a
// sequential census in scan order, field by field, on host and device.
// Seeds run one after the other: the control flow is uniform. HostGroup
// emulates the group sequentially, WarpGroup (filter_runner.cu) maps it to
// one CUDA warp.
//
// Exactness is judged, not assumed: the host compilation is compared edge by
// edge with the engine's q3 lane (tests/gpu/lanes_port_gate.cpp: the emitted
// multiset against the product GlobalBoxes census of the global index, and
// the seed and emission counts); the device run is judged edge by edge on
// G4 (judge_lanes_filter).
//
// Memory: each group owns a slab of `capacity` sites (ranges, coordinates,
// ranks, seeds, scratch) and `record_capacity` records. An edge whose cover
// or whose records exceed it returns Deferred with no counter: the CPU then
// runs its q3 lane. Deferral is a memory decision only.

#include "certificate.hpp"

namespace mhgp9::gpu {

__extension__ typedef unsigned __int128 u128;  // as i128 (-Wpedantic)

// The certificate index plus the original input ID of every spatial rank
// (ties of the owner test, support and shell of the records).
struct LanesIndex {
  CertificateIndex tree;
  const u32* rank_ids;
};

// One emitted q3 ball: the primitive key {A, Bx, By, Bz, C} (A > 0), the
// support IDs sorted (4th unused: absent32), the exact strict interior count
// and the shell (sites of zero power, a, b and x included): its size and an
// order-free fingerprint of its IDs (wrapping sum and xor of mix64(ID)).
struct LaneRecord {
  i128 key[5];
  u32 support[4];
  u32 edge;  // index of the edge in the call (set by the runner)
  u32 depth, shell, arity;
  u64 shell_sum, shell_xor;
};
static_assert(sizeof(LaneRecord) == 128, "LaneRecord is 128 bytes on host and device");

struct LanesSlab {
  u32* ranges;            // 2 * capacity (build_cover)
  std::int32_t* points;   // 3 * capacity: the cover sites in scan order
  u32* ranks;             // capacity: their spatial ranks
  u32* seeds;             // capacity: scan positions of the seeds
  u32* scratch;           // capacity: the ranks in range order, before the scan order
  LaneRecord* records;    // record_capacity
  u32 capacity, record_capacity;
};

// Scan order (before the lanes plan step 3; since L10 the default is the
// axial order below, the rings stay under MHGP9_LANES_SCAN_AXIAL=0): the
// cover sites by ring of |2z-a-b|^2 in [0, 4D] (eight rings
// of equal width, ring r = number of thresholds (i+1)D/2, i < 7, below it),
// increasing rank inside a ring (stable). Every owned q3 ball contains the
// ball of radius |ab|/(2 sqrt 3) around the midpoint (R <= |ab|/sqrt 3 for
// an acute triangle of longest side ab), so the inner rings come first and a
// rejected census stops early. Any fixed order gives the same balls; this
// one fixes the counters of host and device alike. MHGP9_LANES_SCAN_RINGS
// (measurement builds only, 1..8) changes the number of rings; 1 is the
// plain rank order of ScalarCover.
#ifndef MHGP9_LANES_SCAN_RINGS
#define MHGP9_LANES_SCAN_RINGS 8
#endif
inline constexpr u32 scan_rings = MHGP9_LANES_SCAN_RINGS;
static_assert(scan_rings >= 1 && scan_rings <= 8, "the ring index is read back from three ballot bits");

// Work of the q3 lanes (declared ledger of this mode, never compared with
// the engine's own q3 ledger): the rebuilt cover, the seed scan (every cover
// site is tested once), and the censuses in scan order.
// `edges` counts the decided edges (one prologue each: cover, scan order,
// seeds), `q3_edges` those whose q3 lane ran and `census_seeds` their seeds.
struct Q3Work {
  u64 edges, cover_sites, max_cover_sites;
  CoverWork cover;
  u64 seed_tests, acute_sites, owner_rejections, seeds;
  u64 pruned_sites;  // L11: cover sites removed from the scan order (of the seed_tests)
  u64 q3_edges, census_seeds;
  u64 census_point_tests, census_inside_sites, census_shell_sites, census_outside_sites;
  u64 depth_rejections, emitted, shell_ids;
};

// The same for ONE edge (u32 where a count is bounded by capacity or by the
// seeds; the census sums are up to seeds x sites, in u64).
struct EdgeQ3Work {
  u32 cover_sites;
  EdgeCoverWork cover;
  u32 seed_tests, acute_sites, owner_rejections, seeds;
  u32 pruned_sites;
  u32 q3_edges, census_seeds;
  u64 census_point_tests, census_inside_sites, census_shell_sites, census_outside_sites;
  u32 depth_rejections, emitted;
  u64 shell_ids;
};

MHGP9_HD inline void add_q3_edge(Q3Work& to, const EdgeQ3Work& from) {
  ++to.edges;
  to.cover_sites += from.cover_sites;
  to.max_cover_sites = to.max_cover_sites < from.cover_sites ? from.cover_sites : to.max_cover_sites;
  add_edge_cover(to.cover, from.cover);
  to.seed_tests += from.seed_tests; to.acute_sites += from.acute_sites;
  to.owner_rejections += from.owner_rejections; to.seeds += from.seeds; to.pruned_sites += from.pruned_sites;
  to.q3_edges += from.q3_edges; to.census_seeds += from.census_seeds;
  to.census_point_tests += from.census_point_tests; to.census_inside_sites += from.census_inside_sites;
  to.census_shell_sites += from.census_shell_sites; to.census_outside_sites += from.census_outside_sites;
  to.depth_rejections += from.depth_rejections; to.emitted += from.emitted; to.shell_ids += from.shell_ids;
}

// Sums; max_cover_sites by MAX.
MHGP9_HD inline void add_q3(Q3Work& to, const Q3Work& from) {
  to.edges += from.edges;
  to.cover_sites += from.cover_sites;
  to.max_cover_sites = to.max_cover_sites < from.max_cover_sites ? from.max_cover_sites : to.max_cover_sites;
  add_cover(to.cover, from.cover);
  to.seed_tests += from.seed_tests; to.acute_sites += from.acute_sites;
  to.owner_rejections += from.owner_rejections; to.seeds += from.seeds; to.pruned_sites += from.pruned_sites;
  to.q3_edges += from.q3_edges; to.census_seeds += from.census_seeds;
  to.census_point_tests += from.census_point_tests; to.census_inside_sites += from.census_inside_sites;
  to.census_shell_sites += from.census_shell_sites; to.census_outside_sites += from.census_outside_sites;
  to.depth_rejections += from.depth_rejections; to.emitted += from.emitted; to.shell_ids += from.shell_ids;
}

// ---- Exact arithmetic ---------------------------------------------------

// SplitMix64 finalizer: the shell fingerprint of one input ID.
MHGP9_HD inline u64 mix64(u64 x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

MHGP9_HD inline u32 ctz64(u64 x) {  // x != 0
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__ffsll(static_cast<long long>(x)) - 1);
#else
  return static_cast<u32>(__builtin_ctzll(x));
#endif
}

MHGP9_HD inline u32 ctz128(u128 x) {  // x != 0
  const u64 low = static_cast<u64>(x);
  return low != 0 ? ctz64(low) : 64 + ctz64(static_cast<u64>(x >> 64));
}

// Binary gcd (no 128-bit remainder on the device); gcd(a, 0) = a.
MHGP9_HD inline u128 gcd128(u128 a, u128 b) {
  if (a == 0) return b;
  if (b == 0) return a;
  const u32 shift = ctz128(a | b);
  a >>= ctz128(a);
  do {
    b >>= ctz128(b);
    if (a > b) {
      const u128 t = a;
      a = b;
      b = t;
    }
    b -= a;
  } while (b != 0);
  return a << shift;
}

// Relative power of the circumball of (a, b, x) at origin a:
// P(z) = G|z-a|^2 - W.(z-a), exactly the value make_q3 translates.
struct Q3Form {
  i128 gram;
  i128 linear[3];
};

// make_q3's guards: a strictly acute nondegenerate triangle, else false.
// Bounds (M=262143): G <= 12M^4, |W_i| <= 36M^5 (exact_ball.cpp).
MHGP9_HD inline bool q3_form(const std::int32_t a[3], const std::int32_t b[3], const std::int32_t x[3],
                             Q3Form& form) {
  i64 d[3], u[3];
  i64 dd = 0, uu = 0, du = 0;
  for (int axis = 0; axis < 3; ++axis) {
    d[axis] = static_cast<i64>(b[axis]) - a[axis];
    u[axis] = static_cast<i64>(x[axis]) - a[axis];
    dd += d[axis] * d[axis];
    uu += u[axis] * u[axis];
    du += d[axis] * u[axis];
  }
  if (du <= 0 || dd - du <= 0 || uu - du <= 0) return false;
  form.gram = static_cast<i128>(dd) * uu - static_cast<i128>(du) * du;
  if (form.gram <= 0) return false;
  const i128 along_d = static_cast<i128>(uu) * (dd - du);
  const i128 along_u = static_cast<i128>(dd) * (uu - du);
  for (int axis = 0; axis < 3; ++axis) form.linear[axis] = along_d * d[axis] + along_u * u[axis];
  return true;
}

// |P(z)| < 2^116 for every z of the coordinate domain (exact_ball.cpp).
MHGP9_HD inline i128 q3_power(const Q3Form& form, const std::int32_t a[3], const std::int32_t z[3]) {
  i64 e[3];
  i64 ee = 0;
  for (int axis = 0; axis < 3; ++axis) {
    e[axis] = static_cast<i64>(z[axis]) - a[axis];
    ee += e[axis] * e[axis];
  }
  return form.gram * ee - (form.linear[0] * e[0] + form.linear[1] * e[1] + form.linear[2] * e[2]);
}

MHGP9_HD inline i128 abs_i128(i128 x) { return x < 0 ? -x : x; }

// `translated` then `primitive` of exact_ball.cpp: the key of the ball.
MHGP9_HD inline void q3_key(const Q3Form& form, const std::int32_t a[3], i128 key[5]) {
  key[0] = form.gram;
  i128 constant = 0;
  i64 aa = 0;
  for (int axis = 0; axis < 3; ++axis) {
    aa += static_cast<i64>(a[axis]) * a[axis];
    constant += form.linear[axis] * a[axis];
    key[axis + 1] = -2 * form.gram * a[axis] - form.linear[axis];
  }
  key[4] = form.gram * aa + constant;
  u128 divisor = static_cast<u128>(key[0]);
  for (int i = 1; i < 5; ++i) divisor = gcd128(divisor, static_cast<u128>(abs_i128(key[i])));
  const i128 d = static_cast<i128>(divisor);
  for (int i = 0; i < 5; ++i) key[i] /= d;
}

// edge_key(p, x) < (low, high) of the engine: the sorted pair of p and x
// compared lexicographically with the sorted pair of the edge.
MHGP9_HD inline bool pair_below(u32 p, u32 x, u32 low, u32 high) {
  const u32 first = p < x ? p : x, second = p < x ? x : p;
  return first < low || (first == low && second < high);
}

// ---- Group reductions ----------------------------------------------------

// Wrapping sum and xor of h(lane) over the set lanes of `mask`; HostGroup
// and WarpGroup implement `fingerprint(mask, h, sum, x)` (sum and x are
// ADDED to).

// L11 (lanes plan step 3, 24 septembre 2026; STATUT V9-S4): with
// w = 2z-a-b, v = b-a, num = D-|w|^2 and den = D|w|^2-(w.v)^2 >= 0, a cover
// site with num < 0 and num^2 > 2 den is STRICTLY OUTSIDE every sphere
// through a and b whose centre lies in the disk of centres
// Delta4 = {c : (c-m).v = 0, 8|c-m|^2 <= D}: 4(|z-c|^2-|a-c|^2) =
// |w|^2-D-4w.u >= -num-sqrt(2 den) > 0 for c = m+u, u.v = 0, |u|^2 <= D/8.
// Those spheres hold every seed's q3 ball (|c0-m|^2 <= D/12) and every
// member of the q4 family whose centre is in Delta4 (2mu^2 <= Q, L2), which
// every bucket of the grid has at its inner end (|g_j| <= mubar-1 or 0).
// Such a site is never inside a census ball, never a lens site, never on a
// shell, never a seed, never a member of an emittable class: it is removed
// from the scan order (only declared counters change: pruned_sites, the
// seed scan, censuses, passes, tasks). |num| <= 3D, den <= 4D^2 < 2^78:
// num^2 < 2^80 in i128. MHGP9_LANES_PRUNE=0 (measurement and gate builds
// only) keeps every site.
#ifndef MHGP9_LANES_PRUNE
#define MHGP9_LANES_PRUNE 1
#endif
inline constexpr bool lanes_prune = MHGP9_LANES_PRUNE != 0;

// Engraved bounds on the u18 domain (coordinates 0..262143): |w|^2 < 2^40
// and D < 2^38 in i64; |num| <= max(|w|^2, D), den <= D|w|^2; num^2 and
// 2 den in i128 (L10 compares 64 num^2 with 144 den at most).
inline constexpr i128 lanes_max_ww = i128{3} * (2 * 262143) * (2 * 262143);
inline constexpr i128 lanes_max_dd = i128{3} * 262143 * 262143;
static_assert(lanes_max_ww < (i128{1} << 62) && lanes_max_dd < (i128{1} << 62), "num = D - |w|^2 in i64");
static_assert(64 * lanes_max_ww * lanes_max_ww < (i128{1} << 126), "64 num^2 in i128");
static_assert(144 * lanes_max_dd * lanes_max_ww < (i128{1} << 126), "144 den in i128");

MHGP9_HD inline bool lanes_outside_centres(i64 num, i128 den) {
#if defined(MHGP9_LANES_MUTANT_PRUNE_SMALL_DISK)
  return num < 0 && static_cast<i128>(num) * num > den;  // mutant: the disk 16|c-m|^2 <= D, too small
#else
  return num < 0 && static_cast<i128>(num) * num > 2 * den;
#endif
}

// The axial terms of a site z of the edge (a, b): num and den above.
MHGP9_HD inline void lanes_axial_terms(const std::int32_t a[3], const std::int32_t b[3], const std::int32_t z[3],
                                       i64& num, i128& den) {
  i64 ww = 0, wv = 0, dd = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 w = 2 * static_cast<i64>(z[axis]) - a[axis] - b[axis];
    const i64 v = static_cast<i64>(b[axis]) - a[axis];
    ww += w * w;
    wv += w * v;
    dd += v * v;
  }
  num = dd - ww;
  den = static_cast<i128>(dd) * ww - static_cast<i128>(wv) * wv;
}

// A site's place in the scan: its class (scan order: increasing class,
// increasing rank inside a class), or lanes_pruned_class.
inline constexpr u32 lanes_pruned_class = 63;

// L10 (lanes plan step 3, 24 septembre 2026): the AXIAL scan order. With
// t = nu / sqrt(delta) (+infinity when delta = 0 <= nu), the fraction of the
// disk of centres Delta4 whose spheres hold the site strictly inside grows
// with t: all of it for t >= sqrt 2, none for t <= -sqrt 2 (pruned by L11).
// The rings (|w|^2 only) ignore the distance to the axis ab. The site's
// class is k = 12 - max{i in [-13, 12] : t >= i/8} in [0, 25] (i = -13:
// none, only for sites L11 would prune), decided EXACTLY: t >= i/8 iff
// 8 nu >= i sqrt(delta), i.e. for i >= 0: nu >= 0 and 64 nu^2 >= i^2 delta;
// for i < 0: nu >= 0 or 64 nu^2 <= i^2 delta (64 nu^2 and 144 delta < 2^86,
// bounds engraved above). The predicate is monotone in i: five steps of
// bisection. Classes in increasing order, ranks increasing inside a class.
// Any fixed order gives the same balls (L10, STATUT V9-S4): only declared
// counters change. MHGP9_LANES_SCAN_AXIAL=0 (measurement and gate builds
// only) restores the rings.
#ifndef MHGP9_LANES_SCAN_AXIAL
#define MHGP9_LANES_SCAN_AXIAL 1
#endif
inline constexpr bool lanes_scan_axial = MHGP9_LANES_SCAN_AXIAL != 0;
inline constexpr u32 axial_classes = 26;
inline constexpr u32 scan_classes = lanes_scan_axial ? axial_classes : scan_rings;
static_assert(scan_classes <= 32 && lanes_pruned_class >= 32, "a kept class has five bits, the pruned one the sixth");

MHGP9_HD inline bool lanes_axial_at_least(i64 num, i128 den, int i) {
  const i128 lhs = 64 * static_cast<i128>(num) * num, rhs = static_cast<i128>(i * i) * den;
  return i >= 0 ? num >= 0 && lhs >= rhs : num >= 0 || lhs <= rhs;
}

MHGP9_HD inline u32 lanes_axial_class(i64 num, i128 den) {
  int low = -13, high = 12;  // lanes_axial_at_least holds at low (-13: by convention), fails above high
  while (low < high) {
    const int mid = low + (high - low + 1) / 2;
    if (lanes_axial_at_least(num, den, mid)) low = mid;
    else high = mid - 1;
  }
  return static_cast<u32>(12 - low);
}

// The site's class: pruned (L11), else the axial class (or the ring:
// number of thresholds (i+1)D/2 below |w|^2 in [0, 4D]).
MHGP9_HD inline u32 lanes_scan_class(const std::int32_t a[3], const std::int32_t b[3], const std::int32_t z[3],
                                     i64 diameter) {
  i64 num = 0;
  i128 den = 0;
  lanes_axial_terms(a, b, z, num, den);
  if (lanes_prune && lanes_outside_centres(num, den)) return lanes_pruned_class;
  if (lanes_scan_axial) return lanes_axial_class(num, den);
  const i64 norm = diameter - num;
  u32 r = 0;
  for (u32 i = 0; i + 1 < scan_rings; ++i) r += 2 * norm > static_cast<i64>(i + 1) * diameter ? 1U : 0U;
  return r;
}

// The seed code of a site x of the edge (a, b): 0 not strictly acute, 1
// acute but ab not owned, 3 an owned acute seed (the engine's q3_edge
// predicate, ties broken by the sorted ID pair).
MHGP9_HD inline u32 lanes_seed_code(const std::int32_t a[3], const std::int32_t b[3], const std::int32_t x[3],
                                    i64 diameter, u32 id_a, u32 id_b, u32 id_x) {
  const u32 low = id_a < id_b ? id_a : id_b, high = id_a < id_b ? id_b : id_a;
  i64 ax = 0, bx = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 da = static_cast<i64>(x[axis]) - a[axis], db = static_cast<i64>(x[axis]) - b[axis];
    ax += da * da;
    bx += db * db;
  }
  if (!(diameter + ax > bx && diameter + bx > ax && ax + bx > diameter)) return 0U;
  if (ax > diameter || bx > diameter) return 1U;
  if ((ax == diameter && pair_below(id_a, id_x, low, high)) || (bx == diameter && pair_below(id_b, id_x, low, high)))
    return 1U;
  return 3U;
}

// ---- One edge -------------------------------------------------------------

// Prologue of an edge, shared by its q3 and q4 lanes, in two steps (v9
// S4b tasks, 24 septembre 2026: the task runner reserves the cover's place
// in its arena between them). lanes_cover: the cover's ranges in
// slab.ranges (range_count ranges, `sites` sites) and its counters; deferred
// beyond slab.capacity, fault below two sites. Uniform result.
template <class Group>
MHGP9_HD CertificateStatus lanes_cover(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                       const LanesSlab& slab, u32& range_count, u32& sites, EdgeQ3Work& local) {
  range_count = sites = 0;
  const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  const CertificateSlab view{slab.ranges, nullptr, nullptr, nullptr, nullptr, slab.capacity};
  if (!build_cover(group, index.tree, edge_ball(a, b, false), view, range_count, sites, local.cover))
    return CertificateStatus::deferred;
  if (sites < 2) return CertificateStatus::fault;
  local.cover_sites = sites;
  return CertificateStatus::decided;
}

// lanes_order: the cover sites (ranges of lanes_cover, through
// slab.scratch) kept by L11 in scan order at slab.points / slab.ranks[0,
// sites) (`sites`: the cover's sites in, the kept ones out) and the owned
// acute seeds at slab.seeds[0, seeds) (scan positions); `local` receives
// the seed-scan counters. Returns the number of seeds.
template <class Group>
MHGP9_HD u32 lanes_order(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank, const LanesSlab& slab,
                         u32 range_count, u32& sites, EdgeQ3Work& local) {
  const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];

  // The cover ranks in range order (increasing rank).
  u32 position = 0;
  for (u32 r = 0; r < range_count; ++r) {
    const u32 first = slab.ranges[2 * r], count = slab.ranges[2 * r + 1] - first;
    group.for_each(count, [&](u32 i) { slab.scratch[position + i] = first + i; });
    position += count;
  }
  group.sync();

  i64 diameter = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(b[axis]) - a[axis];
    diameter += delta * delta;
  }
  // The class of every site, once, in slab.ranges (free after the fill).
  group.for_each(sites, [&](u32 s) {
    const std::int32_t* p = index.tree.rank_points + 3 * static_cast<std::size_t>(slab.scratch[s]);
    slab.ranges[s] = lanes_scan_class(a, b, p, diameter);
  });
  group.sync();
  // The kept sites in scan order (classes, then rank): a counting pass,
  // then a stable scatter. Each chunk's classes come back in three ballots
  // (bits 0-4 the class, bit 5 pruned).
  const auto class_bits = [&](u32 base, u32 bits[6]) {
    group.ballot2(base, sites, [&](u32 s) { return slab.ranges[s] & 3U; }, bits[0], bits[1]);
    group.ballot2(base, sites, [&](u32 s) { return (slab.ranges[s] >> 2) & 3U; }, bits[2], bits[3]);
    group.ballot2(base, sites, [&](u32 s) { return (slab.ranges[s] >> 4) & 3U; }, bits[4], bits[5]);
    const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
    const u32 valid = lanes == 32 ? 0xffffffffU : ((1U << lanes) - 1U);
    bits[5] &= valid;  // pruned
  };
  const auto class_mask = [](const u32 bits[6], u32 valid, u32 r, u32 class_bits_read) {
    u32 m = valid & ~bits[5];
    for (u32 k = 0; k < class_bits_read; ++k) m &= ((r >> k) & 1U) != 0 ? bits[k] : ~bits[k];
    return m;
  };
#if defined(MHGP9_LANES_MUTANT_AXIAL_FOUR_BITS)
  constexpr u32 count_bits = 4;  // mutant: the counting pass reads classes 16..25 as 0..9 (wrong offsets)
#else
  constexpr u32 count_bits = 5;
#endif
  u32 class_begin[scan_classes];
  for (u32 r = 0; r < scan_classes; ++r) class_begin[r] = 0;
  u32 pruned = 0;
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 bits[6];
    class_bits(base, bits);
    const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
    const u32 valid = lanes == 32 ? 0xffffffffU : ((1U << lanes) - 1U);
    pruned += popcount32(bits[5]);
    for (u32 r = 0; r + 1 < scan_classes; ++r) class_begin[r + 1] += popcount32(class_mask(bits, valid, r, count_bits));
  }
  for (u32 r = 1; r < scan_classes; ++r) class_begin[r] += class_begin[r - 1];
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 bits[6];
    class_bits(base, bits);
    const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
    const u32 valid = lanes == 32 ? 0xffffffffU : ((1U << lanes) - 1U);
    for (u32 r = 0; r < scan_classes; ++r) {
      const u32 mask = class_mask(bits, valid, r, 5);
      group.for_set(mask, [&](u32 lane, u32 order) {
        const u32 rank = slab.scratch[base + lane], to = class_begin[r] + order;
#if defined(MHGP9_LANES_MUTANT_AXIAL_FOUR_BITS)
        if (to >= sites) return;  // the mutant's surplus writes stay inside the reserved cover (no crash)
#endif
        const std::int32_t* p = index.tree.rank_points + 3 * static_cast<std::size_t>(rank);
        std::int32_t* q = slab.points + 3 * static_cast<std::size_t>(to);
        q[0] = p[0];
        q[1] = p[1];
        q[2] = p[2];
        slab.ranks[to] = rank;
      });
      class_begin[r] += popcount32(mask);
    }
  }
  group.sync();
  // Every cover site is classified once (L11 test, then the seed test on
  // the kept ones): seed_tests stays the cover's size.
  local.seed_tests = sites;
  local.pruned_sites = pruned;
  sites -= pruned;

  // Seeds: strictly acute, ab owned (the engine's q3_edge predicate).
  u32 seeds = 0;
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 acute = 0, owned = 0;
    group.ballot2(base, sites, [&](u32 s) -> u32 {
      return lanes_seed_code(a, b, slab.points + 3 * static_cast<std::size_t>(s), diameter, id_a, id_b,
                             index.rank_ids[slab.ranks[s]]);
    }, acute, owned);
    group.for_set(owned, [&](u32 lane, u32 rank) { slab.seeds[seeds + rank] = base + lane; });
    local.acute_sites += popcount32(acute);
    local.owner_rejections += popcount32(acute & ~owned);
    seeds += popcount32(owned);
  }
  local.seeds = seeds;
  group.sync();
  return seeds;
}

// The whole prologue: the cover, its sites in scan order (slab.points /
// slab.ranks[0, sites)) and the owned acute seeds (slab.seeds[0, seeds),
// scan positions). `local` receives the cover and seed-scan counters.
// Uniform result.
template <class Group>
MHGP9_HD CertificateStatus lanes_prologue(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                          const LanesSlab& slab, u32& sites_out, u32& seeds_out, EdgeQ3Work& local) {
  sites_out = seeds_out = 0;
  u32 range_count = 0, sites = 0;
  const auto status = lanes_cover(group, index, a_rank, b_rank, slab, range_count, sites, local);
  if (status != CertificateStatus::decided) return status;
  seeds_out = lanes_order(group, index, a_rank, b_rank, slab, range_count, sites, local);
  sites_out = sites;
  return CertificateStatus::decided;
}

// One census's state (S4a; since L15 also fed by the fused pass): the
// sites read in scan order, those of negative and of zero power among them,
// the shell fingerprint (whole chunks only: a rejected census has none).
struct Q3Census {
  u32 read, depth, shell;
  u64 shell_sum, shell_xor;
  bool rejected;
};

// One chunk of a census at `base` from its ballots (bit l: site base + l
// of negative power in `inside`, of zero power in `zero`): the sequential
// census stops at the need-th site of negative power (its exact stopping
// site recovered from the mask), else it reads the whole chunk.
template <class Group>
MHGP9_HD void q3_census_chunk(const Group& group, const LanesIndex& index, const LanesSlab& slab, u32 sites,
                              u32 threshold, u32 base, u32 inside, u32 zero, Q3Census& c) {
  const u32 need = threshold - c.depth;  // >= 1: depth < K-1 while the census runs
  if (popcount32(inside) >= need) {
    const u32 stop = nth_set_bit(inside, need);
    const u32 read = stop == 31 ? 0xffffffffU : ((1U << (stop + 1)) - 1U);
    c.read += stop + 1;
    c.depth += need;
    c.shell += popcount32(zero & read);
    c.rejected = true;
    return;
  }
  const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
  c.read += lanes;
  c.depth += popcount32(inside);
  c.shell += popcount32(zero);
  if (zero != 0)
    group.fingerprint(zero, [&](u32 lane) { return mix64(index.rank_ids[slab.ranks[base + lane]]); }, c.shell_sum,
                      c.shell_xor);
}

// A finished census's counters, as a census in scan order counts them.
MHGP9_HD inline void q3_census_count(const Q3Census& c, EdgeQ3Work& local) {
  local.census_point_tests += c.read;
  local.census_inside_sites += c.depth;
  local.census_shell_sites += c.shell;
  local.census_outside_sites += c.read - c.depth - c.shell;
}

// The record of an accepted census (one lane writes it).
MHGP9_HD inline void q3_record(const Q3Form& form, const std::int32_t a[3], u32 id_a, u32 id_b, u32 id_x,
                               const Q3Census& c, LaneRecord& r) {
  q3_key(form, a, r.key);
  u32 ids[3] = {id_a, id_b, id_x};
  for (int p = 1; p < 3; ++p)
    for (int q = p; q > 0 && ids[q] < ids[q - 1]; --q) {
      const u32 t = ids[q];
      ids[q] = ids[q - 1];
      ids[q - 1] = t;
    }
  r.support[0] = ids[0];
  r.support[1] = ids[1];
  r.support[2] = ids[2];
  r.support[3] = absent32;
  r.edge = absent32;
  r.depth = c.depth;
  r.shell = c.shell;
  r.arity = 3;
  r.shell_sum = c.shell_sum;
  r.shell_xor = c.shell_xor;
}

// The q3 censuses of the seeds [first, last) of the prologue, appended at
// slab.records[record_count, ...) in seed order: a record slab full at an
// accepted seed defers, a seed without its ball is a fault (the records
// before either stay valid). `local` receives the census counters only.
// Uniform result.
template <class Group>
MHGP9_HD CertificateStatus q3_census_range(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                           unsigned kmax, const LanesSlab& slab, u32 sites, u32 first, u32 last,
                                           u32& record_count, EdgeQ3Work& local) {
  if (kmax < 2) return CertificateStatus::fault;  // no q3 lane below K2
  const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
  const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  // One census per seed, split across the lanes by site.
  const u32 threshold = kmax - 1;
  for (u32 i = first; i < last; ++i) {
    const std::int32_t* x = slab.points + 3 * static_cast<std::size_t>(slab.seeds[i]);
    Q3Form form{};
    if (!q3_form(a, b, x, form)) return CertificateStatus::fault;  // an owned acute seed has its ball
    Q3Census c{0, 0, 0, 0, 0, false};
    for (u32 base = 0; base < sites && !c.rejected; base += Group::size) {
      u32 inside = 0, zero = 0;
      group.ballot2(base, sites, [&](u32 t) -> u32 {
        const i128 power = q3_power(form, a, slab.points + 3 * static_cast<std::size_t>(t));
        return (power < 0 ? 1U : 0U) | (power == 0 ? 2U : 0U);
      }, inside, zero);
      q3_census_chunk(group, index, slab, sites, threshold, base, inside, zero, c);
    }
    q3_census_count(c, local);
    if (c.rejected) {
      ++local.depth_rejections;
      continue;
    }
    if (record_count == slab.record_capacity) return CertificateStatus::deferred;
    if (group.leader())
      q3_record(form, a, id_a, id_b, index.rank_ids[slab.ranks[slab.seeds[i]]], c, slab.records[record_count]);
    ++record_count;
    ++local.emitted;
    local.shell_ids += c.shell;
  }
  group.sync();  // the leader's records, copied out by every lane
  return CertificateStatus::decided;
}

// The q3 censuses of all the prologue's seeds, with the edge's q3 counters.
template <class Group>
MHGP9_HD CertificateStatus q3_census(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                     unsigned kmax, const LanesSlab& slab, u32 sites, u32 seeds, u32& record_count,
                                     EdgeQ3Work& local) {
  if (kmax < 2) return CertificateStatus::fault;  // no q3 lane below K2
  ++local.q3_edges;
  local.census_seeds += seeds;
  return q3_census_range(group, index, a_rank, b_rank, kmax, slab, sites, 0, seeds, record_count, local);
}

// The q3 lane alone: prologue then censuses (S4a).
template <class Group>
MHGP9_HD CertificateStatus q3_lane(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                   unsigned kmax, const LanesSlab& slab, u32& record_count, EdgeQ3Work& local) {
  record_count = 0;
  local = EdgeQ3Work{};
  if (kmax < 2) return CertificateStatus::fault;  // no q3 lane below K2
  u32 sites = 0, seeds = 0;
  const auto status = lanes_prologue(group, index, a_rank, b_rank, slab, sites, seeds, local);
  if (status != CertificateStatus::decided) return status;
  return q3_census(group, index, a_rank, b_rank, kmax, slab, sites, seeds, record_count, local);
}

}  // namespace mhgp9::gpu
