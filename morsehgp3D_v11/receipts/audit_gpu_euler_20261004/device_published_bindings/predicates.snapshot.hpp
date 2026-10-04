// Types et predicats exacts de la feuille source unique (leaf_device.hpp) : memes formules et memes bornes que
// num/ (budgets.hpp, predicates.cpp, center_region.cpp, q4_weights.hpp, power et orientation certificates),
// restreintes aux chemins i128 certifies ; un predicat non certifie rend faux et la feuille devient non resolue.
#pragma once

#include <cstdint>

#if defined(__CUDACC__)
#define MHGP11_LEAF_HD __host__ __device__ inline
#else
#define MHGP11_LEAF_HD inline
#endif

#ifndef MHGP11_COORD_BITS
#error "leaf_device : MHGP11_COORD_BITS (18, 21 ou 24) requis"
#endif

namespace mhgp11::leaf_device {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
__extension__ using i128 = __int128;  // extension GNU, aussi admise par nvcc ; meme type que core/types.hpp

inline constexpr int kBits = MHGP11_COORD_BITS;
static_assert(kBits == 18 || kBits == 21 || kBits == 24, "leaf_device : profil 18, 21 ou 24 bits");
inline constexpr u32 kMaxSites = 32;
inline constexpr u32 kNoSite = 0xFFFFFFFFu;
inline constexpr u32 kOk = 0, kUnresolved = 1;
// Cache J2 simule : un bit par triplet i<j<k<32, rang combinatoire dense de center_line_cache.hpp.
inline constexpr u32 kTriples = 32 * 31 * 30 / 6;
inline constexpr u32 kSeenWords = (kTriples + 31) / 32;

// Memes champs que LeafCounts (leaf.cpp) ; la voie graphe ne teste aucun couple (region_pair_* restent nuls).
struct Counts {
  u64 dominance_tests = 0, prefixes = 0, judged = 0, census_tests = 0, emitted = 0, incidences = 0;
  u64 q4_candidates = 0, q4_levels = 0;
  u64 region_pair_tests = 0, region_pair_rejects = 0, region_line_tests = 0, region_line_rejects = 0;
  u64 region_line_evaluations = 0, region_line_cache_hits = 0, region_line_fallbacks = 0;
};

struct Input {
  const u32* x = nullptr;  // coordonnees du nuage, indexees par SiteIdx global
  const u32* y = nullptr;
  const u32* z = nullptr;
  const u32* sites = nullptr;  // liste de la feuille : SiteIdx globaux croissants (ordre de Morton)
  u32 m = 0;                   // 1 <= m <= kMaxSites
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};  // boite T0 demi-ouverte, 0 <= lo < hi <= 2^B
  int kmax = 0;
  bool cache = false;  // option cache_center_lines : seulement les compteurs evaluations/hits
};

struct Ball {
  u32 support[4];  // S* en SiteIdx globaux, kNoSite au-dela de qmin
  u32 p, m, qmin;
};

MHGP11_LEAF_HD u32 popc(u64 x) {
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__popcll(static_cast<unsigned long long>(x)));
#else
  return static_cast<u32>(__builtin_popcountll(x));
#endif
}
MHGP11_LEAF_HD u32 ctz(u64 x) {
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__ffsll(static_cast<long long>(x)) - 1);
#else
  return static_cast<u32>(__builtin_ctzll(x));
#endif
}
MHGP11_LEAF_HD int sign(i128 v) { return (v > 0) - (v < 0); }
MHGP11_LEAF_HD i128 magnitude(i128 v) { return v < 0 ? -v : v; }

struct Vec { i64 v[3]; };
MHGP11_LEAF_HD Vec diff(const u32* a, const u32* b) {
  return Vec{{i64(a[0]) - i64(b[0]), i64(a[1]) - i64(b[1]), i64(a[2]) - i64(b[2])}};
}
// Bornes de num/budgets.hpp : dot < 3M^2 (2B+2 <= 50 bits), cross < 2M^2, en i64 aux trois profils.
MHGP11_LEAF_HD i64 dot(const Vec& a, const Vec& b) { return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]; }
MHGP11_LEAF_HD Vec cross(const Vec& a, const Vec& b) {
  return Vec{{a.v[1] * b.v[2] - a.v[2] * b.v[1], a.v[2] * b.v[0] - a.v[0] * b.v[2], a.v[0] * b.v[1] - a.v[1] * b.v[0]}};
}

// Centre a + N/D, D > 0, avec les certificats de sa fabrique (num/power_certificate.hpp, orientation_certificate.hpp).
struct Center {
  i64 anchor[3];
  i128 n[3];
  i128 d;
  u32 arity;
  bool q3_power;  // certificat global de puissance q3 (sans objet pour les arites 2 et 4)
  bool orient;    // certificat global d'orientation
};

MHGP11_LEAF_HD bool global_orientation(i128 d, const i128* n) {
  const i128 d_limit = i128(1) << (124 - 3 * kBits), n_limit = i128(1) << (124 - 2 * kBits);
  if (d <= 0 || d >= d_limit) return false;
  for (int j = 0; j < 3; ++j)
    if (n[j] <= -n_limit || n[j] >= n_limit) return false;
  return true;
}
MHGP11_LEAF_HD bool q3_global_power(i128 d, const i128* n) {
  const i128 d_limit = i128(1) << (123 - 2 * kBits), n_limit = i128(1) << (124 - kBits);
  if (d <= 0 || d >= d_limit) return false;
  for (int j = 0; j < 3; ++j)
    if (n[j] <= -n_limit || n[j] >= n_limit) return false;
  return true;
}

// use_native_power de predicates.cpp : Budget::side = 6B+8 <= 127 (u18), ou arite != 3, ou certificat q3.
MHGP11_LEAF_HD bool native_power_ok(const Center& c) { return 6 * kBits + 8 <= 127 || c.arity != 3 || c.q3_power; }

// Signe de D|z-a|^2 - 2N.(z-a) ; faux si la voie native n'est pas certifiee (leaf.cpp prendrait checked/Wide).
MHGP11_LEAF_HD bool side(const Center& c, const u32* z, int& out) {
  if (!native_power_ok(c)) return false;
  const i64 v0 = i64(z[0]) - c.anchor[0], v1 = i64(z[1]) - c.anchor[1], v2 = i64(z[2]) - c.anchor[2];
  i128 total = c.d * i128(v0 * v0 + v1 * v1 + v2 * v2);
  total += c.n[0] * (-2 * i128(v0));
  total += c.n[1] * (-2 * i128(v1));
  total += c.n[2] * (-2 * i128(v2));
  out = sign(total);
  return true;
}

// Produit mixte de quatre Point : < 6M^3, exact en i128 (num::orientation).
MHGP11_LEAF_HD int orientation4(const u32* a, const u32* b, const u32* c, const u32* d) {
  const Vec normal = cross(diff(b, a), diff(c, a)), w = diff(d, a);
  return sign(i128(normal.v[0]) * w.v[0] + i128(normal.v[1]) * w.v[1] + i128(normal.v[2]) * w.v[2]);
}

// center_orientation, voie native certifiee seulement.
MHGP11_LEAF_HD bool center_orientation(const u32* a, const u32* b, const u32* c, const Center& s, int& out) {
  if (!s.orient) return false;
  const Vec normal = cross(diff(b, a), diff(c, a));
  i128 total = 0;
  for (int j = 0; j < 3; ++j) {
    const i128 coordinate = s.n[j] + s.d * i128(s.anchor[j] - i64(a[j]));
    total += coordinate * normal.v[j];
  }
  out = sign(total);
  return true;
}

// center_inside : orientations des quatre faces, sommet oppose contre centre.
MHGP11_LEAF_HD bool center_inside(const Center& s, const u32* const* p, bool& out) {
  for (int opposite = 0; opposite < 4; ++opposite) {
    const u32* face[3];
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = p[i];
    const int vertex = orientation4(face[0], face[1], face[2], p[opposite]);
    if (vertex == 0) { out = false; return true; }
    int center = 0;
    if (!center_orientation(face[0], face[1], face[2], s, center)) return false;
    if (center != vertex) { out = false; return true; }
  }
  out = true;
  return true;
}

// center_midpoint : 2(D a_j + N_j) == D (a_j + b_j), < 2^(5B+7) <= 2^127.
MHGP11_LEAF_HD bool midpoint(const Center& s, const u32* a, const u32* b) {
  for (int j = 0; j < 3; ++j)
    if (2 * (s.d * s.anchor[j] + s.n[j]) != s.d * (i128(a[j]) + i128(b[j]))) return false;
  return true;
}

MHGP11_LEAF_HD bool strictly_acute(const u32* a, const u32* b, const u32* c) {
  return dot(diff(b, a), diff(c, a)) > 0 && dot(diff(a, b), diff(c, b)) > 0 && dot(diff(a, c), diff(b, c)) > 0;
}

// Lemme Z de num/center_region.cpp : 0 degenere, 1 disjoint, 2 rencontre la fermeture.
inline constexpr int kDegenerate = 0, kDisjoint = 1, kIntersects = 2;
MHGP11_LEAF_HD int center_line_meets(const u32* a, const u32* b, const u32* c, const i64* lo, const i64* hi) {
  i64 fu[3], gu[3], fc = 0, gc = 0;
  for (int k = 0; k < 3; ++k) {
    fu[k] = i64(a[k]) - i64(b[k]);
    gu[k] = i64(a[k]) - i64(c[k]);
    fc += i64(a[k]) * i64(a[k]) - i64(b[k]) * i64(b[k]);
    gc += i64(a[k]) * i64(a[k]) - i64(c[k]) * i64(c[k]);
  }
  i64 cr[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const i64 value = gu[i] * fu[j] - fu[i] * gu[j];
      cr[i][j] = cr[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return kDegenerate;
  i64 p0 = fc, p1 = gc;
  for (int k = 0; k < 3; ++k) {
    p0 -= (lo[k] + hi[k]) * fu[k];
    p1 -= (lo[k] + hi[k]) * gu[k];
  }
  for (int k = 0; k < 3; ++k) {
    if (fu[k] == 0 && gu[k] == 0) continue;
    const i128 left = magnitude(i128(gu[k]) * p0 - i128(fu[k]) * p1);
    i128 right = 0;
    for (int j = 0; j < 3; ++j) right += i128(hi[j] - lo[j]) * cr[k][j];
    if (left > right) return kDisjoint;
  }
  return kIntersects;
}

// catalogue.cpp center_in_box_impl : lo <= a + N/D < hi par axe, N + D(a - borne) < 2^(5B+6).
MHGP11_LEAF_HD bool center_in_box(const Center& s, const i64* lo, const i64* hi) {
  for (int j = 0; j < 3; ++j) {
    const i128 lower = s.n[j] + s.d * i128(s.anchor[j] - lo[j]);
    const i128 upper = s.n[j] + s.d * i128(s.anchor[j] - hi[j]);
    if (lower < 0 || upper >= 0) return false;
  }
  return true;
}

// Enveloppe fermee de points doubles contre la boite demi-ouverte doublee (leaf.cpp doubled_envelope_meets).
template <int N>
MHGP11_LEAF_HD bool doubled_envelope_meets(const i64 (&pts)[N][3], const i64* lo, const i64* hi) {
  for (int axis = 0; axis < 3; ++axis) {
    i64 low = pts[0][axis], high = pts[0][axis];
    for (int i = 1; i < N; ++i) {
      low = pts[i][axis] < low ? pts[i][axis] : low;
      high = pts[i][axis] > high ? pts[i][axis] : high;
    }
    if (high < 2 * lo[axis] || low >= 2 * hi[axis]) return false;
  }
  return true;
}

}  // namespace mhgp11::leaf_device
