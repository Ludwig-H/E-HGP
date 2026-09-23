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

// Scan order: the cover sites by ring of |2z-a-b|^2 in [0, 4D] (eight rings
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
struct Q3Work {
  u64 edges, cover_sites, max_cover_sites;
  CoverWork cover;
  u64 seed_tests, acute_sites, owner_rejections, seeds;
  u64 census_point_tests, census_inside_sites, census_shell_sites, census_outside_sites;
  u64 depth_rejections, emitted, shell_ids;
};

// The same for ONE edge (u32 where a count is bounded by capacity or by the
// seeds; the census sums are up to seeds x sites, in u64).
struct EdgeQ3Work {
  u32 cover_sites;
  EdgeCoverWork cover;
  u32 seed_tests, acute_sites, owner_rejections, seeds;
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
  to.owner_rejections += from.owner_rejections; to.seeds += from.seeds;
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
  to.owner_rejections += from.owner_rejections; to.seeds += from.seeds;
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

// ---- One edge -------------------------------------------------------------

// The q3 lane of one edge whose q3 lane a certificate left open. On decided,
// `local` holds this edge's work and slab.records[0, record_count) its
// emitted balls in seed order (edge field unset); on deferred or fault the
// caller adds nothing. Every lane gets the same result (uniform flow).
template <class Group>
MHGP9_HD CertificateStatus q3_lane(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
                                   unsigned kmax, const LanesSlab& slab, u32& record_count, EdgeQ3Work& local) {
  record_count = 0;
  local = EdgeQ3Work{};
  if (kmax < 2) return CertificateStatus::fault;  // no q3 lane below K2
  const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
  const CertificateSlab view{slab.ranges, nullptr, nullptr, nullptr, nullptr, slab.capacity};
  u32 range_count = 0, sites = 0;
  if (!build_cover(group, index.tree, edge_ball(a, b, false), view, range_count, sites, local.cover))
    return CertificateStatus::deferred;
  if (sites < 2) return CertificateStatus::fault;
  local.cover_sites = sites;

  // The cover ranks in range order (increasing rank).
  u32 position = 0;
  for (u32 r = 0; r < range_count; ++r) {
    const u32 first = slab.ranges[2 * r], count = slab.ranges[2 * r + 1] - first;
    group.for_each(count, [&](u32 i) { slab.scratch[position + i] = first + i; });
    position += count;
  }
  group.sync();

  i64 diameter = 0, center_twice[3];
  for (int axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(b[axis]) - a[axis];
    diameter += delta * delta;
    center_twice[axis] = static_cast<i64>(a[axis]) + b[axis];
  }
  // The cover sites in scan order (rings, then rank): a counting pass, then
  // a stable scatter. The ring index comes back in two ballots (bits 0-1, 2).
  const auto ring = [&](u32 s) -> u32 {
    const std::int32_t* p = index.tree.rank_points + 3 * static_cast<std::size_t>(slab.scratch[s]);
    i64 norm = 0;
    for (int axis = 0; axis < 3; ++axis) {
      const i64 delta = 2 * static_cast<i64>(p[axis]) - center_twice[axis];
      norm += delta * delta;
    }
    u32 r = 0;
    for (u32 i = 0; i + 1 < scan_rings; ++i) r += 2 * norm > static_cast<i64>(i + 1) * diameter ? 1U : 0U;
    return r;
  };
  u32 ring_begin[scan_rings];
  for (u32 r = 0; r < scan_rings; ++r) ring_begin[r] = 0;
  const auto ring_masks = [&](u32 base, u32 masks[scan_rings]) {
    u32 bit0 = 0, bit1 = 0, bit2 = 0, unused = 0;
    group.ballot2(base, sites, ring, bit0, bit1);
    group.ballot2(base, sites, [&](u32 s) { return ring(s) >> 2; }, bit2, unused);
    const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
    const u32 valid = lanes == 32 ? 0xffffffffU : ((1U << lanes) - 1U);
    for (u32 r = 0; r < scan_rings; ++r)
      masks[r] = valid & ((r & 1U) != 0 ? bit0 : ~bit0) & ((r & 2U) != 0 ? bit1 : ~bit1) &
                 ((r & 4U) != 0 ? bit2 : ~bit2);
  };
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 masks[scan_rings];
    ring_masks(base, masks);
    for (u32 r = 0; r + 1 < scan_rings; ++r) ring_begin[r + 1] += popcount32(masks[r]);
  }
  for (u32 r = 1; r < scan_rings; ++r) ring_begin[r] += ring_begin[r - 1];
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 masks[scan_rings];
    ring_masks(base, masks);
    for (u32 r = 0; r < scan_rings; ++r) {
      group.for_set(masks[r], [&](u32 lane, u32 order) {
        const u32 rank = slab.scratch[base + lane], to = ring_begin[r] + order;
        const std::int32_t* p = index.tree.rank_points + 3 * static_cast<std::size_t>(rank);
        std::int32_t* q = slab.points + 3 * static_cast<std::size_t>(to);
        q[0] = p[0];
        q[1] = p[1];
        q[2] = p[2];
        slab.ranks[to] = rank;
      });
      ring_begin[r] += popcount32(masks[r]);
    }
  }
  group.sync();

  // Seeds: strictly acute, ab owned (the engine's q3_edge predicate).
  const u32 low = id_a < id_b ? id_a : id_b, high = id_a < id_b ? id_b : id_a;
  u32 seeds = 0;
  for (u32 base = 0; base < sites; base += Group::size) {
    u32 acute = 0, owned = 0;
    group.ballot2(base, sites, [&](u32 s) -> u32 {
      const std::int32_t* x = slab.points + 3 * static_cast<std::size_t>(s);
      i64 ax = 0, bx = 0;
      for (int axis = 0; axis < 3; ++axis) {
        const i64 da = static_cast<i64>(x[axis]) - a[axis], db = static_cast<i64>(x[axis]) - b[axis];
        ax += da * da;
        bx += db * db;
      }
      if (!(diameter + ax > bx && diameter + bx > ax && ax + bx > diameter)) return 0U;
      if (ax > diameter || bx > diameter) return 1U;
      if (ax == diameter || bx == diameter) {
        const u32 id = index.rank_ids[slab.ranks[s]];
        if ((ax == diameter && pair_below(id_a, id, low, high)) ||
            (bx == diameter && pair_below(id_b, id, low, high)))
          return 1U;
      }
      return 3U;
    }, acute, owned);
    group.for_set(owned, [&](u32 lane, u32 rank) { slab.seeds[seeds + rank] = base + lane; });
    local.acute_sites += popcount32(acute);
    local.owner_rejections += popcount32(acute & ~owned);
    seeds += popcount32(owned);
  }
  local.seed_tests = sites;
  local.seeds = seeds;
  group.sync();

  // One census per seed, split across the lanes by site.
  const u32 threshold = kmax - 1;
  for (u32 i = 0; i < seeds; ++i) {
    const std::int32_t* x = slab.points + 3 * static_cast<std::size_t>(slab.seeds[i]);
    Q3Form form{};
    if (!q3_form(a, b, x, form)) return CertificateStatus::fault;  // an owned acute seed has its ball
    u32 depth = 0, shell = 0;
    u64 shell_sum = 0, shell_xor = 0;
    bool rejected = false;
    for (u32 base = 0; base < sites; base += Group::size) {
      u32 inside = 0, zero = 0;
      group.ballot2(base, sites, [&](u32 t) -> u32 {
        const i128 power = q3_power(form, a, slab.points + 3 * static_cast<std::size_t>(t));
        return (power < 0 ? 1U : 0U) | (power == 0 ? 2U : 0U);
      }, inside, zero);
      const u32 need = threshold - depth;  // >= 1: depth < K-1 while the census runs
      if (popcount32(inside) >= need) {
        // The sequential census stops at the need-th site of negative power.
        const u32 stop = nth_set_bit(inside, need);
        const u32 read = stop == 31 ? 0xffffffffU : ((1U << (stop + 1)) - 1U);
        const u32 zeros = popcount32(zero & read);
        local.census_point_tests += stop + 1;
        local.census_inside_sites += need;
        local.census_shell_sites += zeros;
        local.census_outside_sites += stop + 1 - need - zeros;
        rejected = true;
        break;
      }
      const u32 lanes = sites - base < Group::size ? sites - base : Group::size;
      const u32 in = popcount32(inside), zeros = popcount32(zero);
      depth += in;
      shell += zeros;
      local.census_point_tests += lanes;
      local.census_inside_sites += in;
      local.census_shell_sites += zeros;
      local.census_outside_sites += lanes - in - zeros;
      if (zero != 0)
        group.fingerprint(zero, [&](u32 lane) { return mix64(index.rank_ids[slab.ranks[base + lane]]); },
                          shell_sum, shell_xor);
    }
    if (rejected) {
      ++local.depth_rejections;
      continue;
    }
    if (record_count == slab.record_capacity) return CertificateStatus::deferred;
    if (group.leader()) {
      LaneRecord& r = slab.records[record_count];
      q3_key(form, a, r.key);
      u32 ids[3] = {id_a, id_b, index.rank_ids[slab.ranks[slab.seeds[i]]]};
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
      r.depth = depth;
      r.shell = shell;
      r.arity = 3;
      r.shell_sum = shell_sum;
      r.shell_xor = shell_xor;
    }
    ++record_count;
    ++local.emitted;
    local.shell_ids += shell;
  }
  group.sync();  // the leader's records, copied out by every lane
  return CertificateStatus::decided;
}

}  // namespace mhgp9::gpu
