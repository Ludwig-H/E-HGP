#pragma once

// MorseHGP3D v9 (24 septembre 2026) — S4b: portable copy (host and CUDA
// device) of the q4 lane of one certified edge, WITHOUT atlas, by lens
// buckets (synthesis of the S4b design, docs/s4b_conception_20260924/). The
// object is the product's Local28/LiveOnly q4 lane (lanes/q4_local.cpp,
// sweep): for every owned strictly acute seed x of ab, every root group of
// the family P - mu S whose strict depth on the cover is below T = K-2 emits
// its least-ID member y passing owned, positive and canonical, with shell =
// group + constant shell. Lemmas L1-L8 (docs/math/STATUT_PREUVES_ET_
// HEURISTIQUES.md, V9-S4b) give the equality:
// - L2: every positive owned root lies in [-mubar, mubar], 2 mubar^2 >= Q =
//   D(3G - 2EF) (lemon alpha4 = 2);
// - L1: on an integer grid of J = 8 buckets over that range, a site strictly
//   inside the members at BOTH ends of a bucket is inside every member of the
//   bucket (P - mu S is affine in mu): a bucket whose lens count reaches T
//   holds no emission. A seed whose eight lens counts all reach T is
//   certified empty (checked after every 32-site chunk, in scan order);
// - L4: in a live bucket [lo, hi], depth(r) = lens + exits of the bucket with
//   root > r + entries with root < r; the root group is inside the bucket's
//   events. L6: a bucket owns [lo, hi) (and hi for the last one);
// - L5: sign(root_z - root_c) = sgn(det_c |v|^2 - num_c.v) sgn(det_c) sgn(S(z)),
//   (det_c, num_c) = make_q4's numerator of the sphere a, b, x, c; the same
//   form gives the positivity weights and the key;
// - L3/L7: an emitted y has P(y) > 0, is owned and canonical: the product's
//   choice is the least ID over those candidates that pass positivity.
// The cover contains every positive owned q4 ball (R + |c-m| <= 0.966|ab|).
//
// One seed is processed by a GROUP of 32 lanes: the lens pass and every
// compare step are split across the lanes by site (ballots, warp sums); the
// control flow is uniform. The counters are uniform and written by the
// leader lane only (Q4Work in the group's slab). Records go to the edge's
// record slab. An event buffer beyond its capacity defers the edge (memory
// decision only).

#include "lanes.hpp"

namespace mhgp9::gpu {

inline constexpr u32 q4_buckets = 8;  // J

// Work of the q4 lanes (declared ledger, S4b): seeds, lens pass, survivor
// stage, groups and emissions kept apart (a seed may emit from several
// groups, auditor B). The survivor stage's chunk passes are all counted
// (auditor, 24 septembre): list_steps = the bucket list and foreign passes
// of every live bucket, group_steps = every pass of the group loop (least
// ID search, locate, compare, reset, positivity and its minimum, choice);
// compare_steps is the compare part of group_steps.
struct Q4Work {
  u64 edges, seeds, certified, certified_chunk1, survivors, pass_chunks, pass_site_tests;
  u64 buffered_events, max_buffered, live_buckets, filter_steps, bucket_events, candidates, foreign_candidates;
  u64 groups, compare_steps, depth_rejected_groups, positivity_tests, groups_without_valid, emitted;
  u64 emitting_seeds, multi_emission_seeds, max_emissions_per_seed, shell_ids, max_group, constant_shell_sites;
  u64 list_steps, group_steps;
};

// Sums; the max_* fields by MAX.
MHGP9_HD inline void add_q4(Q4Work& to, const Q4Work& from) {
  to.edges += from.edges; to.seeds += from.seeds; to.certified += from.certified;
  to.certified_chunk1 += from.certified_chunk1; to.survivors += from.survivors; to.pass_chunks += from.pass_chunks;
  to.pass_site_tests += from.pass_site_tests; to.buffered_events += from.buffered_events;
  to.max_buffered = to.max_buffered < from.max_buffered ? from.max_buffered : to.max_buffered;
  to.live_buckets += from.live_buckets; to.filter_steps += from.filter_steps; to.bucket_events += from.bucket_events;
  to.candidates += from.candidates; to.foreign_candidates += from.foreign_candidates; to.groups += from.groups;
  to.compare_steps += from.compare_steps; to.depth_rejected_groups += from.depth_rejected_groups;
  to.positivity_tests += from.positivity_tests; to.groups_without_valid += from.groups_without_valid;
  to.emitted += from.emitted; to.emitting_seeds += from.emitting_seeds;
  to.multi_emission_seeds += from.multi_emission_seeds;
  to.max_emissions_per_seed =
      to.max_emissions_per_seed < from.max_emissions_per_seed ? from.max_emissions_per_seed : to.max_emissions_per_seed;
  to.shell_ids += from.shell_ids;
  to.max_group = to.max_group < from.max_group ? from.max_group : to.max_group;
  to.constant_shell_sites += from.constant_shell_sites;
  to.list_steps += from.list_steps;
  to.group_steps += from.group_steps;
}

// Per-group scratch of the survivor stage: `capacity` buffered events.
struct Q4Slab {
  u32* positions;  // scan positions of the buffered events
  u32* bits;       // per buffered event: neg (9 bits) | pos (9) | flags
  u32* list;       // indices into the buffer: the current bucket's events
  u32 capacity;
};

inline constexpr u32 q4_candidate_bit = 1U << 18;
inline constexpr u32 q4_decided_bit = 1U << 19;
inline constexpr u32 q4_group_bit = 1U << 20;
inline constexpr u32 q4_valid_bit = 1U << 21;

// The seed's family: d = b - a, u = x - a, n = d x u, S(z) = n.(z - a),
// P(z) = G|z-a|^2 - W.(z - a) (q3_form), and the grid g[0..8] of L2.
struct Q4Family {
  Q3Form form;
  i64 d[3], u[3], n[3];
  i64 dd, uu;
  i64 grid[q4_buckets + 1];
};

MHGP9_HD inline i64 side_of(const Q4Family& f, const std::int32_t a[3], const std::int32_t z[3]) {
  i64 s = 0;
  for (int k = 0; k < 3; ++k) s += f.n[k] * (static_cast<i64>(z[k]) - a[k]);
  return s;
}

// L2's range: mubar = 1 + max{m : 2m^2 < Q}, Q = D(3G - 2EF) > 0 for an
// owned acute seed (0 < Q < 2^115); the grid is symmetric and contains 0.
// Returns false when a guard fails (a fault of the port, never a decision).
MHGP9_HD inline bool q4_family(const std::int32_t a[3], const std::int32_t b[3], const std::int32_t x[3],
                               Q4Family& f) {
  if (!q3_form(a, b, x, f.form)) return false;
  i64 ff = 0;
  f.dd = f.uu = 0;
  for (int k = 0; k < 3; ++k) {
    f.d[k] = static_cast<i64>(b[k]) - a[k];
    f.u[k] = static_cast<i64>(x[k]) - a[k];
    const i64 e = static_cast<i64>(x[k]) - b[k];
    f.dd += f.d[k] * f.d[k];
    f.uu += f.u[k] * f.u[k];
    ff += e * e;
  }
  f.n[0] = f.d[1] * f.u[2] - f.d[2] * f.u[1];
  f.n[1] = f.d[2] * f.u[0] - f.d[0] * f.u[2];
  f.n[2] = f.d[0] * f.u[1] - f.d[1] * f.u[0];
  const i128 q = static_cast<i128>(f.dd) * (3 * f.form.gram - 2 * static_cast<i128>(f.uu) * ff);
  if (q <= 0) return false;
  u64 m = 0;
  for (int bit = 59; bit >= 0; --bit) {
    const u64 c = m | (u64{1} << bit);
    if (2 * static_cast<u128>(c) * c < static_cast<u128>(q)) m = c;
  }
  if (m >= (u64{1} << 59)) return false;  // mubar < 2^60 by Q < 2^115
  const i64 mubar = static_cast<i64>(m + 1);
  constexpr int half = static_cast<int>(q4_buckets / 2);
  for (int i = 0; i <= half; ++i) {
    const i64 t = static_cast<i64>((static_cast<i128>(mubar) * i) / half);
    f.grid[half + i] = t;
    f.grid[half - i] = -t;
  }
  return true;
}

// The 9 signs of f_k = P - g_k S at the grid points: bit k of neg / pos.
MHGP9_HD inline void q4_signs(const Q4Family& f, const std::int32_t a[3], const std::int32_t z[3], i64& side,
                              u32& neg, u32& pos) {
  side = side_of(f, a, z);
  const i128 power = q3_power(f.form, a, z);
  neg = pos = 0;
  for (u32 k = 0; k <= q4_buckets; ++k) {
    const i128 value = power - static_cast<i128>(f.grid[k]) * side;
    if (value < 0) neg |= 1U << k;
    if (value > 0) pos |= 1U << k;
  }
}

// Bucket j: lens (inside at both ends), event (root in closed [g_j, g_j+1]).
MHGP9_HD inline bool q4_lens(u32 neg, u32 j) { return ((neg >> j) & 3U) == 3U; }
MHGP9_HD inline bool q4_event(i64 side, u32 neg, u32 pos, u32 j) {
  return side != 0 && ((neg >> j) & 3U) != 3U && ((pos >> j) & 3U) != 3U;
}

// Pivot form of the sphere a, b, x, w (make_q4's numerator before
// normalization): det = S(w) < 2^57, |num_k| <= 18M^4 < 2^77.
struct Q4Pivot {
  i128 det;
  i128 num[3];
};

MHGP9_HD inline void cross3(const i64 p[3], const i64 q[3], i64 out[3]) {
  out[0] = p[1] * q[2] - p[2] * q[1];
  out[1] = p[2] * q[0] - p[0] * q[2];
  out[2] = p[0] * q[1] - p[1] * q[0];
}

MHGP9_HD inline Q4Pivot q4_pivot(const Q4Family& f, const std::int32_t a[3], const std::int32_t w[3]) {
  i64 v[3];
  for (int k = 0; k < 3; ++k) v[k] = static_cast<i64>(w[k]) - a[k];
  i64 c0[3], c1[3];
  cross3(f.u, v, c0);
  cross3(v, f.d, c1);
  const i64 vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  Q4Pivot p{};
  p.det = static_cast<i128>(f.d[0]) * c0[0] + static_cast<i128>(f.d[1]) * c0[1] + static_cast<i128>(f.d[2]) * c0[2];
  for (int k = 0; k < 3; ++k)
    p.num[k] = static_cast<i128>(f.dd) * c0[k] + static_cast<i128>(f.uu) * c1[k] + static_cast<i128>(vv) * f.n[k];
  return p;
}

MHGP9_HD inline int sign128(i128 v) { return (v > 0) - (v < 0); }

// L5: sign(root_z - root_c), |det|v|^2 - num.v| < 2^97.
MHGP9_HD inline int q4_compare(const Q4Pivot& c, const std::int32_t a[3], const std::int32_t z[3], i64 side_z) {
  i64 v[3];
  for (int k = 0; k < 3; ++k) v[k] = static_cast<i64>(z[k]) - a[k];
  const i64 vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  const i128 q = c.det * vv - (c.num[0] * v[0] + c.num[1] * v[1] + c.num[2] * v[2]);
  return sign128(q) * sign128(c.det) * ((side_z > 0) - (side_z < 0));
}

// make_q4's positivity: barycentric weights num.cof_r > 0 and 2det^2 - sum > 0
// (each weight <= 108M^6 < 2^117).
MHGP9_HD inline bool q4_positive(const Q4Family& f, const std::int32_t a[3], const std::int32_t y[3],
                                 const Q4Pivot& p) {
  i64 v[3];
  for (int k = 0; k < 3; ++k) v[k] = static_cast<i64>(y[k]) - a[k];
  i64 cof[3][3];
  cross3(f.u, v, cof[0]);
  cross3(v, f.d, cof[1]);
  cross3(f.d, f.u, cof[2]);
  i128 remaining = 2 * p.det * p.det;
  for (int r = 0; r < 3; ++r) {
    const i128 weight = p.num[0] * cof[r][0] + p.num[1] * cof[r][1] + p.num[2] * cof[r][2];
    if (weight <= 0) return false;
    remaining -= weight;
  }
  return remaining > 0;
}

// make_q4's key: A = |det|, linear = sgn(det) num, translated to a, primitive.
MHGP9_HD inline void q4_key(const Q4Pivot& p, const std::int32_t a[3], i128 key[5]) {
  Q3Form form{};
  const i128 s = p.det < 0 ? -1 : 1;
  form.gram = p.det < 0 ? -p.det : p.det;
  for (int k = 0; k < 3; ++k) form.linear[k] = s * p.num[k];
  q3_key(form, a, key);
}

// Squared distance of two points of the slab / index.
MHGP9_HD inline i64 dist2(const std::int32_t p[3], const std::int32_t q[3]) {
  i64 s = 0;
  for (int k = 0; k < 3; ++k) {
    const i64 e = static_cast<i64>(p[k]) - q[k];
    s += e * e;
  }
  return s;
}

// The q4 lane of one seed (scan position `seed`) of an edge whose sites are
// slab.points / slab.ranks[0, sites) in scan order. Appends its records at
// slab.records[record_count, ...). Uniform result.
template <class Group>
MHGP9_HD CertificateStatus q4_seed(const Group& group, const LanesIndex& index, const std::int32_t* a,
                                   const std::int32_t* b, u32 id_a, u32 id_b, u32 seed, const LanesSlab& slab,
                                   u32 sites, unsigned kmax, const Q4Slab& q4, u32& record_count, Q4Work& w) {
  const std::int32_t* x = slab.points + 3 * static_cast<std::size_t>(seed);
  const u32 id_x = index.rank_ids[slab.ranks[seed]];
  Q4Family f{};
  if (!q4_family(a, b, x, f)) return CertificateStatus::fault;  // an owned acute seed has its family
  const u32 threshold = kmax - 2;
  const u32 low = id_a < id_b ? id_a : id_b, high = id_a < id_b ? id_b : id_a;
  u32 lens[q4_buckets];
  for (u32 j = 0; j < q4_buckets; ++j) lens[j] = 0;
  u32 buffered = 0, chunks = 0, shell_count = 0;
  u64 shell_sum = 0, shell_xor = 0;
  bool certified = false, overflow = false;
  struct Vote {
    u32 w0, w1;
    bool first, second;
  };
  for (u32 base = 0; base < sites; base += Group::size) {
    ++chunks;
    u32 live = 0;
    for (u32 j = 0; j < q4_buckets; ++j) live |= (lens[j] < threshold ? 1U : 0U) << j;
    u32 w0 = 0, w1 = 0, events = 0, zeros = 0;
    group.vote(base, sites, [&](u32 s) -> Vote {
      i64 side = 0;
      u32 neg = 0, pos = 0;
      q4_signs(f, a, slab.points + 3 * static_cast<std::size_t>(s), side, neg, pos);
      Vote v{0, 0, false, side == 0 && ((neg | pos) & (1U << (q4_buckets / 2))) == 0};
      for (u32 j = 0; j < q4_buckets; ++j) {
        if (q4_lens(neg, j)) {
          if (j < 4) v.w0 += 1U << (8 * j);
          else v.w1 += 1U << (8 * (j - 4));
        } else if (((live >> j) & 1U) != 0 && q4_event(side, neg, pos, j)) {
          v.first = true;
        }
      }
      return v;
    }, w0, w1, events, zeros);
    for (u32 j = 0; j < 4; ++j) {
      lens[j] += (w0 >> (8 * j)) & 0xffU;
      lens[j + 4] += (w1 >> (8 * j)) & 0xffU;
    }
    if (zeros != 0) {
      shell_count += popcount32(zeros);
      group.fingerprint(zeros, [&](u32 lane) { return mix64(index.rank_ids[slab.ranks[base + lane]]); },
                        shell_sum, shell_xor);
    }
    if (events != 0 && !overflow) {
      const u32 add = popcount32(events);
      if (add > q4.capacity - buffered) {
        overflow = true;
      } else {
        group.for_set(events, [&](u32 lane, u32 rank) { q4.positions[buffered + rank] = base + lane; });
        buffered += add;
      }
    }
    bool all = true;
    for (u32 j = 0; j < q4_buckets; ++j) all = all && lens[j] >= threshold;
    if (all) {
      certified = true;
      break;
    }
  }
  group.sync();
  if (group.leader()) {
    ++w.seeds;
    w.pass_chunks += chunks;
    w.pass_site_tests += sites < chunks * Group::size ? sites : chunks * Group::size;
  }
  if (certified) {
    if (group.leader()) {
      ++w.certified;
      if (chunks == 1) ++w.certified_chunk1;
    }
    return CertificateStatus::decided;
  }
  if (overflow) return CertificateStatus::deferred;
  if (group.leader()) {
    ++w.survivors;
    w.buffered_events += buffered;
    w.max_buffered = w.max_buffered < buffered ? buffered : w.max_buffered;
    w.constant_shell_sites += shell_count;
  }

  // Filter: the signs and the candidate flag of every buffered event.
  group.for_each(buffered, [&](u32 i) {
    const u32 s = q4.positions[i];
    const std::int32_t* y = slab.points + 3 * static_cast<std::size_t>(s);
    i64 side = 0;
    u32 neg = 0, pos = 0;
    q4_signs(f, a, y, side, neg, pos);
    bool candidate = ((pos >> (q4_buckets / 2)) & 1U) != 0;  // P(y) > 0 (L3)
    if (candidate) {
      const u32 id_y = index.rank_ids[slab.ranks[s]];
      const i64 diameter = f.dd;
      const i64 ay = dist2(a, y), by = dist2(b, y), xy = dist2(x, y);
      const u32 lx = id_x < id_y ? id_x : id_y, hx = id_x < id_y ? id_y : id_x;
      const bool xy_below = lx < low || (lx == low && hx < high);
      candidate = !(ay > diameter || (ay == diameter && pair_below(id_a, id_y, low, high))) &&
                  !(by > diameter || (by == diameter && pair_below(id_b, id_y, low, high))) &&
                  !(xy > diameter || (xy == diameter && xy_below));
      if (candidate && id_y < id_x) {
        // Canonical: a y below x that makes aby acute is left to that seed.
        const i64 ab = diameter;
        candidate = !(ab + ay > by && ab + by > ay && ay + by > ab);
      }
    }
    q4.bits[i] = neg | (pos << 9) | (candidate ? q4_candidate_bit : 0U);
  });
  group.sync();
  if (group.leader()) w.filter_steps += (buffered + Group::size - 1) / Group::size;

  u32 emits = 0;
  const auto point_of = [&](u32 i) { return slab.points + 3 * static_cast<std::size_t>(q4.positions[i]); };
  const auto id_of = [&](u32 i) { return index.rank_ids[slab.ranks[q4.positions[i]]]; };
  for (u32 j = 0; j < q4_buckets; ++j) {
    if (lens[j] >= threshold) continue;
    // The bucket's events (L4), in buffer order.
    u32 m = 0;
    for (u32 base = 0; base < buffered; base += Group::size) {
      u32 in = 0, unused = 0;
      group.ballot2(base, buffered, [&](u32 i) -> u32 {
        const u32 bits = q4.bits[i];
        const u32 neg = bits & 0x1ffU, pos = (bits >> 9) & 0x1ffU;
        // S != 0 for every buffered event: it was an event of some bucket.
        return ((neg >> j) & 3U) != 3U && ((pos >> j) & 3U) != 3U ? 1U : 0U;
      }, in, unused);
      group.for_set(in, [&](u32 lane, u32 rank) { q4.list[m + rank] = base + lane; });
      m += popcount32(in);
    }
    group.sync();
    // Candidates of this bucket: a root exactly at hi belongs to the right
    // neighbour, except for the last bucket (L6).
    const bool closed = j + 1 == q4_buckets;
    u32 foreign = 0;
    for (u32 base = 0; base < m; base += Group::size) {
      u32 fore = 0, unused = 0;
      group.ballot2(base, m, [&](u32 k) -> u32 {
        const u32 i = q4.list[k];
        u32 bits = q4.bits[i] & ~(q4_decided_bit | q4_group_bit | q4_valid_bit);
        const bool at_hi = ((bits >> (j + 1)) & 1U) == 0 && ((bits >> (9 + j + 1)) & 1U) == 0;
        if ((bits & q4_candidate_bit) != 0 && !closed && at_hi) {
          bits |= q4_decided_bit;  // never examined here
          q4.bits[i] = bits;
          return 1U;
        }
        q4.bits[i] = bits;
        return 0U;
      }, fore, unused);
      foreign += popcount32(fore);
    }
    group.sync();
    const u32 steps = (m + Group::size - 1) / Group::size;  // one pass over the bucket's list
    if (group.leader()) {
      ++w.live_buckets;
      w.bucket_events += m;
      w.foreign_candidates += foreign;
      w.list_steps += (buffered + Group::size - 1) / Group::size + steps;
    }
    // T1 (v9, 24 septembre 2026, plan des voies, etape 1): every undecided
    // candidate of the bucket, in list order, resolves its own root class by
    // one pass against its pivot form, which STOPS as soon as the depth
    // reaches T (the class is rejected whatever its representative: depth is
    // a class property, L4). A pass marks the members it read (current-pass
    // bit), then turns them decided: a class is resolved once when its pass
    // completes, and a rejected class's members read before the stop are
    // skipped. A completed pass handles its class like v1's group: size,
    // fingerprint, positivity of its candidates, least valid ID. Foreign
    // candidates share no class with this bucket's candidates (a class's
    // root is one value), so the least candidate ID of a class is v1's pivot
    // (the representative, L7). The bucket's emissions are put back in
    // representative order: the records are those of v1, in the same order.
    const u32 bucket_first = record_count;
    for (u32 k = 0; k < m; ++k) {
      const u32 ci = q4.list[k];
      const u32 cbits = q4.bits[ci];  // same value on every lane: uniform flow
      if ((cbits & q4_candidate_bit) == 0 || (cbits & q4_decided_bit) != 0) continue;
      const Q4Pivot pivot = q4_pivot(f, a, point_of(ci));
      u32 depth = lens[j], group_size = 0, candidates = 0, passed = 0, rep = 0xffffffffU;
      u64 group_sum = 0, group_xor = 0;
      bool rejected = false;
      for (u32 base = 0; base < m; base += Group::size) {
        u32 same = 0, inside = 0;
        group.ballot2(base, m, [&](u32 kk) -> u32 {
          const u32 i = q4.list[kk];
          const std::int32_t* z = point_of(i);
          const i64 side = side_of(f, a, z);
          const int order = q4_compare(pivot, a, z, side);
          if (order == 0) {
            q4.bits[i] |= q4_group_bit;
            return 1U;
          }
          return (side < 0 && order > 0) || (side > 0 && order < 0) ? 2U : 0U;
        }, same, inside);
        ++passed;
        depth += popcount32(inside);
        group_size += popcount32(same);
        if (same != 0) {
          group.fingerprint(same, [&](u32 lane) { return mix64(id_of(q4.list[base + lane])); }, group_sum,
                            group_xor);
          // The lane reads the bits it has just written itself.
          const u32 v = group.reduce_min(base, m, [&](u32 kk) -> u32 {
            const u32 bits = q4.bits[q4.list[kk]];
            return (bits & q4_group_bit) != 0 && (bits & q4_candidate_bit) != 0 ? id_of(q4.list[kk]) : 0xffffffffU;
          });
          rep = v < rep ? v : rep;
        }
#if defined(MHGP9_Q4_LANES_MUTANT_EARLY_EXIT_BELOW_T)
        if (depth + 1 >= threshold) {  // mutant: stops one interior site too early
#else
        if (depth >= threshold) {
#endif
          rejected = true;
          break;
        }
      }
      group.sync();
      if (group.leader()) {
        ++w.groups;
        w.compare_steps += passed;
        w.group_steps += passed;
        w.max_group = w.max_group < group_size ? group_size : w.max_group;
      }
      if (rejected) {
        // The members read before the stop: decided (their class is rejected).
        for (u32 base = 0; base < passed * Group::size && base < m; base += Group::size)
          group.for_each(m - base < Group::size ? m - base : Group::size, [&](u32 l) {
            u32& bits = q4.bits[q4.list[base + l]];
            if ((bits & q4_group_bit) != 0) bits = (bits & ~q4_group_bit) | q4_decided_bit;
          });
        group.sync();
        if (group.leader()) {
          ++w.depth_rejected_groups;
          w.group_steps += passed;  // members turned decided
        }
        continue;
      }
      // Positivity of the class's candidates (the members turn decided), then
      // the least valid ID.
      u32 valid_id = 0xffffffffU;
      for (u32 base = 0; base < m; base += Group::size) {
        u32 tested = 0, unused = 0;
        group.ballot2(base, m, [&](u32 kk) -> u32 {
          const u32 i = q4.list[kk];
          u32 bits = q4.bits[i];
          const bool member = (bits & q4_group_bit) != 0;
          const bool test = member && (bits & q4_candidate_bit) != 0;
          bits &= ~(q4_group_bit | q4_valid_bit);  // a valid mark belongs to this class only
          if (member) bits |= q4_decided_bit;
          if (test) {
            const std::int32_t* y = point_of(i);
            if (q4_positive(f, a, y, q4_pivot(f, a, y))) bits |= q4_valid_bit;
          }
          q4.bits[i] = bits;
          return test ? 1U : 0U;
        }, tested, unused);
        candidates += popcount32(tested);
        const u32 v = group.reduce_min(base, m, [&](u32 kk) -> u32 {
          const u32 i = q4.list[kk];
          return (q4.bits[i] & q4_valid_bit) != 0 ? id_of(i) : 0xffffffffU;
        });
        valid_id = v < valid_id ? v : valid_id;
      }
      group.sync();
      if (group.leader()) {
        w.positivity_tests += candidates;
        w.candidates += candidates;
        w.group_steps += 2 * steps;  // positivity + its minimum
      }
      if (valid_id == 0xffffffffU) {
        if (group.leader()) ++w.groups_without_valid;
        continue;
      }
      u32 chosen = 0xffffffffU;
      for (u32 base = 0; base < m; base += Group::size) {
        const u32 v = group.reduce_min(base, m, [&](u32 kk) -> u32 {
          const u32 i = q4.list[kk];
          return (q4.bits[i] & q4_valid_bit) != 0 && id_of(i) == valid_id ? i : 0xffffffffU;
        });
        chosen = v < chosen ? v : chosen;
      }
      if (group.leader()) w.group_steps += steps;  // choice
      if (record_count == slab.record_capacity) return CertificateStatus::deferred;
      if (group.leader()) {
        LaneRecord& r = slab.records[record_count];
        q4_key(q4_pivot(f, a, point_of(chosen)), a, r.key);
        u32 ids[4] = {id_a, id_b, id_x, valid_id};
        for (int p = 1; p < 4; ++p)
          for (int q = p; q > 0 && ids[q] < ids[q - 1]; --q) {
            const u32 t = ids[q];
            ids[q] = ids[q - 1];
            ids[q - 1] = t;
          }
        for (int p = 0; p < 4; ++p) r.support[p] = ids[p];
        r.edge = rep;  // sort key of the bucket's emissions (reset below)
        r.depth = depth;
        r.shell = group_size + shell_count;
        r.arity = 4;
        r.shell_sum = group_sum + shell_sum;
        r.shell_xor = group_xor ^ shell_xor;
        ++w.emitted;
        w.shell_ids += group_size + shell_count;
      }
      ++record_count;
      ++emits;
    }
    // v1's order: the bucket's emissions by increasing representative ID.
    if (group.leader()) {
      for (u32 p = bucket_first + 1; p < record_count; ++p)
        for (u32 q = p; q > bucket_first && slab.records[q].edge < slab.records[q - 1].edge; --q) {
          const LaneRecord t = slab.records[q];
          slab.records[q] = slab.records[q - 1];
          slab.records[q - 1] = t;
        }
      for (u32 p = bucket_first; p < record_count; ++p) slab.records[p].edge = absent32;
    }
    group.sync();
  }
  if (group.leader()) {
    if (emits != 0) ++w.emitting_seeds;
    if (emits > 1) ++w.multi_emission_seeds;
    w.max_emissions_per_seed = w.max_emissions_per_seed < emits ? emits : w.max_emissions_per_seed;
  }
  group.sync();
  return CertificateStatus::decided;
}

// The asked lanes of one edge (2: q3, 4: q4, 6: both): the prologue once,
// the q3 censuses, then the q4 seeds. `local` receives the prologue and q3
// work, `w4` (written by the leader lane) the q4 work of this edge; the
// records go to slab.records[0, record_count). Uniform result; on deferred
// or fault the caller adds nothing.
template <class Group>
MHGP9_HD CertificateStatus edge_lanes(const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank, u8 lanes,
                                      unsigned kmax, const LanesSlab& slab, const Q4Slab& q4, u32& record_count,
                                      EdgeQ3Work& local, Q4Work& w4) {
  record_count = 0;
  local = EdgeQ3Work{};
  if (group.leader()) w4 = Q4Work{};
  group.sync();
  if (lanes == 0 || (lanes & ~6U) != 0 || kmax < 2 || ((lanes & 4U) != 0 && kmax < 3))
    return CertificateStatus::fault;
  u32 sites = 0, seeds = 0;
  auto status = lanes_prologue(group, index, a_rank, b_rank, slab, sites, seeds, local);
  if (status != CertificateStatus::decided) return status;
  if ((lanes & 2U) != 0) {
    status = q3_census(group, index, a_rank, b_rank, kmax, slab, sites, seeds, record_count, local);
    if (status != CertificateStatus::decided) return status;
  }
  if ((lanes & 4U) != 0) {
    const std::int32_t* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
    const std::int32_t* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
    const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
    if (group.leader()) ++w4.edges;
    for (u32 i = 0; i < seeds; ++i) {
      status = q4_seed(group, index, a, b, id_a, id_b, slab.seeds[i], slab, sites, kmax, q4, record_count, w4);
      if (status != CertificateStatus::decided) return status;
    }
  }
  group.sync();
  return CertificateStatus::decided;
}

}  // namespace mhgp9::gpu
