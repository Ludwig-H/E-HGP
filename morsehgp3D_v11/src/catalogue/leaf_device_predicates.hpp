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
using i32 = std::int32_t;
using i64 = std::int64_t;
__extension__ using i128 = __int128;  // extension GNU, aussi admise par nvcc ; meme type que core/types.hpp

inline constexpr int kBits = MHGP11_COORD_BITS;
static_assert(kBits == 18 || kBits == 21 || kBits == 24, "leaf_device : profil 18, 21 ou 24 bits");
inline constexpr u32 kMaxSites = 32;
// Compteurs d'une feuille de m<=kMaxSites sites (preuve R1 de leaf.cpp) : prefixes <= sum_{q<=4} C(32,q) = P,
// demandes <= 3P, census et incidences <= mP ; chaque champ, et les boules et incidences emises, < kCountBound < 2^22.
// Les feuilles se recouvrent (un site appartient a plusieurs feuilles : 353 456 feuilles pour 39 885 sites sur
// lidar_ng00) ; les executeurs refusent donc un lot de plus de kMaxBatchJobs = 2^40 feuilles
// (catalogue_counter_overflow), et toute somme de lot, par fil, bloc, ouvrier ou totale, reste < 2^62 : les
// reductions de lot n'ont pas besoin d'addition controlee.
inline constexpr u64 kPrefixBound = 32 + 496 + 4960 + 35960;
inline constexpr u64 kCountBound = u64{kMaxSites} * 3 * kPrefixBound;
inline constexpr u64 kMaxBatchJobs = u64{1} << 40;
static_assert(kMaxSites == 32 && kCountBound < (u64{1} << 22), "leaf_device : compteurs de feuille bornes");
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

// Triangle strictement aigu par trois produits scalaires en a : avec u = b - a et v = c - a, les produits aux sommets
// b et c sont (a - b).(c - b) = uu - uv et (a - c).(b - c) = vv - uv exactement (dot < 3*2^(2B), i64).
MHGP11_LEAF_HD bool acute_dots(i64 uu, i64 vv, i64 uv) { return uv > 0 && uu > uv && vv > uv; }
MHGP11_LEAF_HD bool strictly_acute(const u32* a, const u32* b, const u32* c) {
  const Vec u = diff(b, a), v = diff(c, a);
  return acute_dots(dot(u, u), dot(v, v), dot(u, v));
}

// Issues du lemme Z (center_line_meets ci-dessous).
inline constexpr int kDegenerate = 0, kDisjoint = 1, kIntersects = 2;

// Etendue de feuille (levier C, 6 octobre 2026). Leaf::prepare mesure, axe par axe, la longueur de l'enveloppe des
// sites de la feuille et de la fermeture [lo, hi] de sa boite ; une feuille d'etendue > kNarrowSpan = 2^20 est rendue
// kUnresolved (rejouee par leaf.cpp, memes decisions) avant tout prefixe. Aux profils B <= 20 la condition est
// automatique (coordonnees < 2^B, hi <= 2^B) ; a 21 et 24 bits elle est testee. Sous elle, les predicats J2 et
// l'orientation q4 tiennent en i32/i64 (bornes ci-dessous), au lieu de produits i128.
inline constexpr i64 kNarrowSpan = i64(1) << 20;

// Lemme Z de num/center_region.cpp : 0 degenere, 1 disjoint, 2 rencontre la fermeture. Memes tests, dans le meme
// ordre, que num::center_line_meets ; seule l'arithmetique change. Precondition : etendue D <= 2^20 (ci-dessus).
// Bornes, avec L = lo + hi : |f_k|, |g_k| <= D tiennent en i32 ; a_k + b_k - L_k est somme d'entiers < 2^26 en i32.
// p0 = fc - sum L_k f_k (forme historique) = sum f_k (a_k + b_k - L_k) exactement, et 4 p0 = |2a - L|^2 - |2b - L|^2
// avec |2a_k - L_k| <= |a_k - lo_k| + |a_k - hi_k| <= 2D, donc |p0|, |p1| <= 3 D^2 <= 3*2^40 (i64).
// |cr| = |g_i f_j - f_i g_j| <= 2 D^2 = 2^41 ; |g_k p0|, |f_k p1| <= 3 D^3 = 3*2^60, left <= 6*2^60 < 2^63 ;
// right = sum_{j != k} (hi_j - lo_j) |cr_kj| <= 2 D * 2 D^2 = 2^62 (cr_kk = 0). Tout est exact en i64.
MHGP11_LEAF_HD int center_line_meets(const u32* a, const u32* b, const u32* c, const i64* lo, const i64* hi) {
  i32 fu[3], gu[3];
  i64 p0 = 0, p1 = 0;
  for (int k = 0; k < 3; ++k) {
    const i32 ak = i32(a[k]), bk = i32(b[k]), ck = i32(c[k]), box = i32(lo[k] + hi[k]);
    fu[k] = ak - bk;
    gu[k] = ak - ck;
    p0 += i64(fu[k]) * (ak + bk - box);
    p1 += i64(gu[k]) * (ak + ck - box);
  }
  i64 cr[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const i64 value = i64(gu[i]) * fu[j] - i64(fu[i]) * gu[j];
      cr[i][j] = cr[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return kDegenerate;
  for (int k = 0; k < 3; ++k) {
    if (fu[k] == 0 && gu[k] == 0) continue;
    const i64 signed_left = i64(gu[k]) * p0 - i64(fu[k]) * p1;
    const i64 left = signed_left < 0 ? -signed_left : signed_left;
    i64 right = 0;
    for (int j = 0; j < 3; ++j) right += (hi[j] - lo[j]) * cr[k][j];
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
