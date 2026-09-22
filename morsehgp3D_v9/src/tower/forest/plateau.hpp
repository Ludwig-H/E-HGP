// MorseHGP3D v6 — plateau spherique : le quotient exact hors position generale
// (docs/MATHEMATIQUES.md § 5.3bis).
//
// Pour une boule B (centre rationnel −B/(2A) depuis la BallKey primitive),
// I_B ses interieurs stricts et U_B sa coquille COMPLETE (supports inclus) :
// les K-simplexes de Gabriel de miniboule exactement B sont les σ = I_B ∪ T,
// T ⊆ U_B, |T| = K+1−|I_B|, c ∈ conv(T) FERME. Par Caratheodory, c ∈ conv(T)
// ⟺ T contient une paire diametrale, un triangle ferme ou un tetraedre ferme
// contenant c. Facette σ∖{v} pour v ∈ T : ACTIVE ssi c ∉ conv(T∖{v}) (sa
// miniboule retrecit) ; sinon elle nait AU niveau (attachement). Les retraits
// d'interieur sont toujours des attachements.
//
// Voie ORACLE BORNEE (enumeration des T, plafond de coquille explicite) ; en
// regime regulier (|U_B| = arite du support minimal) exactement UN σ par K.
// Arithmetique de PRODUCTION (i128, et un entier signe 192 bits pour les
// barycentriques du triangle et les produits normale·(centre-sommet), < 2^155
// sous u18) — jamais celle de l'oracle.
#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include "../core/wide.hpp"
#include "../lanes/keys.hpp"

namespace mhgp9::tower {

struct BallRat {
  i128 cnum[3];
  i128 cden;  // = 2A > 0
};

inline BallRat ball_center(const BallKey& k) {
  BallRat c;
  c.cden = 2 * k.a;
  for (int i = 0; i < 3; ++i) c.cnum[i] = -k.b[i];
  return c;
}

namespace plateau_detail {

// Entier signe 192 bits (signe-magnitude) pour les barycentriques.
struct S192 {
  bool neg = false;
  U192 mag{{0, 0, 0}};
};

inline S192 s192_mul(i128 x, i128 y) {
  S192 r;
  r.neg = (x < 0) != (y < 0) && x != 0 && y != 0;
  r.mag = mul_128x128_192(uabs128(x), uabs128(y));
  return r;
}

// (nom propre : dint.hpp definit aussi un u192_add HD ; les deux visibles dans
// le pilote CUDA seraient ambigus)
inline U192 u192_add_plateau(const U192& a, const U192& b) {
  U192 r;
  u128 acc = (u128)a.w[0] + b.w[0];
  r.w[0] = (u64)acc;
  acc = (u128)a.w[1] + b.w[1] + (acc >> 64);
  r.w[1] = (u64)acc;
  r.w[2] = a.w[2] + b.w[2] + (u64)(acc >> 64);
  return r;
}

inline U192 u192_sub(const U192& a, const U192& b) {  // precondition a >= b
  U192 r;
  u128 borrow = 0;
  for (int i = 0; i < 3; ++i) {
    const u128 ai = a.w[i];
    const u128 bi = (u128)b.w[i] + borrow;
    r.w[i] = (u64)(ai - bi);
    borrow = ai < bi ? 1 : 0;
  }
  return r;
}

inline S192 s192_add(const S192& a, const S192& b) {
  S192 r;
  if (a.neg == b.neg) {
    r.neg = a.neg;
    r.mag = u192_add_plateau(a.mag, b.mag);
  } else {
    const int c = cmp_u192(a.mag, b.mag);
    if (c >= 0) {
      r.neg = a.neg;
      r.mag = u192_sub(a.mag, b.mag);
    } else {
      r.neg = b.neg;
      r.mag = u192_sub(b.mag, a.mag);
    }
  }
  if (r.mag.w[0] == 0 && r.mag.w[1] == 0 && r.mag.w[2] == 0) r.neg = false;
  return r;
}

inline S192 s192_neg(S192 a) {
  if (a.mag.w[0] || a.mag.w[1] || a.mag.w[2]) a.neg = !a.neg;
  return a;
}

inline int s192_sign(const S192& a) {
  if (a.mag.w[0] == 0 && a.mag.w[1] == 0 && a.mag.w[2] == 0) return 0;
  return a.neg ? -1 : 1;
}

// a <=> b.
inline int s192_cmp(const S192& a, const S192& b) { return s192_sign(s192_add(a, s192_neg(b))); }

// Produit scalaire exact n·v en 192 bits signes. Port v9 (18 bits) : avec
// |n_i| < 2^37 (produit vectoriel de deux differences) et |v_i| < 2^97
// (cnum - cden*t, cles < 2^96 / 2^77), chaque terme vaut < 2^134 : il deborde
// i128, pas S192 (somme < 2^136).
inline S192 dot3_s192(i128 n0, i128 n1, i128 n2, const i128 v[3]) {
  S192 r = s192_mul(n0, v[0]);
  r = s192_add(r, s192_mul(n1, v[1]));
  return s192_add(r, s192_mul(n2, v[2]));
}

// c est-il le milieu de (t0, t1) ?
inline bool pair_diametral(const BallRat& c, const P3& t0, const P3& t1) {
  const i64 s[3] = {t0.x + t1.x, t0.y + t1.y, t0.z + t1.z};
  for (int i = 0; i < 3; ++i)
    if (2 * c.cnum[i] != c.cden * s[i]) return false;
  return true;
}

// c dans le triangle FERME (t0, t1, t2) : coplanaire et barycentriques >= 0.
inline bool triangle_closed(const BallRat& c, const P3& t0, const P3& t1, const P3& t2) {
  const P3 e1 = p3_sub(t1, t0), e2 = p3_sub(t2, t0);
  const P3 n = p3_cross(e1, e2);
  if (n.x == 0 && n.y == 0 && n.z == 0) return false;
  const i128 v[3] = {c.cnum[0] - c.cden * t0.x, c.cnum[1] - c.cden * t0.y, c.cnum[2] - c.cden * t0.z};
  if (s192_sign(dot3_s192(n.x, n.y, n.z, v)) != 0) return false;
  // alpha·(G·cden) = d1·E2 − d2·F ; beta·(G·cden) = d2·E1 − d1·F ; alpha+beta <= 1.
  const i128 d1 = v[0] * e1.x + v[1] * e1.y + v[2] * e1.z;  // < 3·2^97·2^18 < 2^117 (u18)
  const i128 d2 = v[0] * e2.x + v[1] * e2.y + v[2] * e2.z;
  const i64 E1 = p3_norm2(e1), E2 = p3_norm2(e2), F = p3_dot(e1, e2);
  const S192 a = s192_add(s192_mul(d1, E2), s192_neg(s192_mul(d2, F)));
  const S192 b = s192_add(s192_mul(d2, E1), s192_neg(s192_mul(d1, F)));
  if (s192_sign(a) < 0 || s192_sign(b) < 0) return false;
  const S192 gc = s192_mul((i128)E1 * E2 - (i128)F * F, c.cden);
  return s192_cmp(s192_add(a, b), gc) <= 0;
}

// c dans le tetraedre FERME (non degenere ; faces au sens large).
inline bool tetra_closed(const BallRat& c, const P3& t0, const P3& t1, const P3& t2, const P3& t3) {
  const P3* v[4] = {&t0, &t1, &t2, &t3};
  for (int f = 0; f < 4; ++f) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != f) fp[t++] = v[i];
    const P3 e1 = p3_sub(*fp[1], *fp[0]), e2 = p3_sub(*fp[2], *fp[0]);
    const P3 es = p3_sub(*v[f], *fp[0]);
    const i128 n0 = e1.y * e2.z - e1.z * e2.y, n1 = -(e1.x * e2.z - e1.z * e2.x), n2 = e1.x * e2.y - e1.y * e2.x;
    // side_s : |es_i| < 2^18, terme < 2^55 (i128). side_c : terme < 2^134, S192.
    const i128 side_s = n0 * es.x + n1 * es.y + n2 * es.z;
    if (side_s == 0) return false;  // coplanaire : les triangles couvrent
    const i128 vc[3] = {c.cnum[0] - c.cden * fp[0]->x, c.cnum[1] - c.cden * fp[0]->y, c.cnum[2] - c.cden * fp[0]->z};
    const int side_c = s192_sign(dot3_s192(n0, n1, n2, vc));
    if (side_c != 0 && ((side_c > 0) != (side_s > 0))) return false;
  }
  return true;
}

}  // namespace plateau_detail

// c ∈ conv(T) FERME, par Caratheodory.
inline bool center_in_conv(const BallRat& c, const std::vector<P3>& pts, const std::vector<i32>& T) {
  using namespace plateau_detail;
  const size_t n = T.size();
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j) {
      if (pair_diametral(c, pts[(size_t)T[i]], pts[(size_t)T[j]])) return true;
      for (size_t k = j + 1; k < n; ++k) {
        if (triangle_closed(c, pts[(size_t)T[i]], pts[(size_t)T[j]], pts[(size_t)T[k]])) return true;
        for (size_t l = k + 1; l < n; ++l)
          if (tetra_closed(c, pts[(size_t)T[i]], pts[(size_t)T[j]], pts[(size_t)T[k]], pts[(size_t)T[l]])) return true;
      }
    }
  return false;
}

struct PlateauEvent {
  std::vector<i32> tpart, ipart;  // index de positions uniques
  u16 active_mask = 0;            // bit t : σ∖{T[t]} active
};

// Tous les σ = I_B ∪ T, T ⊆ U_B, |σ| <= kmax + 1, c ∈ conv(T) ferme.
inline void expand_plateau(const BallRat& c, const std::vector<P3>& pos, std::span<const i32> interior,
                           std::span<const i32> shell, size_t kmax_plus1, std::vector<PlateauEvent>* out) {
  const u32 nu = (u32)shell.size();
  for (u32 tm = 1; tm < (1u << nu); ++tm) {
    const int nt = __builtin_popcount(tm);
    if (nt < 2 || interior.size() + (size_t)nt > kmax_plus1) continue;
    std::vector<i32> T;
    for (u32 b = 0; b < nu; ++b)
      if (tm & (1u << b)) T.push_back(shell[(size_t)b]);
    if (!center_in_conv(c, pos, T)) continue;
    std::sort(T.begin(), T.end());
    PlateauEvent ev;
    ev.tpart = T;
    ev.ipart.assign(interior.begin(), interior.end());
    for (size_t v = 0; v < T.size(); ++v) {
      std::vector<i32> trest;
      for (size_t w = 0; w < T.size(); ++w)
        if (w != v) trest.push_back(T[w]);
      const bool same_ball = trest.size() >= 2 && center_in_conv(c, pos, trest);
      if (!same_ball) ev.active_mask |= (u16)(1u << v);
    }
    out->push_back(std::move(ev));
  }
}

}  // namespace mhgp9::tower

