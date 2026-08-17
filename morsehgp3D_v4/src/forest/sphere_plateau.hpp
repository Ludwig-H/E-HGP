// MorseHGP3D v4 — PLATEAU SPHERIQUE : le quotient exact hors position
// generale (MATHEMATIQUES.md § 5.3bis, audit bloquant du 17 aout).
//
// Pour une boule B (centre c, rationnel -B/(2A) depuis la BallKey
// primitive), I_B ses interieurs stricts et U_B sa coquille COMPLETE
// (supports inclus) : les K-simplexes de Gabriel de miniboule exactement B
// sont les σ = I_B ∪ T, T ⊆ U_B, |T| = K+1-|I_B|, c ∈ conv(T) FERME.
// Par Caratheodory, c ∈ conv(T) ⟺ T contient une paire diametrale, un
// triangle ferme ou un tetraedre ferme contenant c.
//
// Ceci est la voie ORACLE BORNE (enumeration des T, plafond de coquille
// explicite) ; la compression par supports minimaux viendra pour l'echelle.
// Largeurs : cnum < 2^86, cden = 2A < 2^69 ; les tests de paire et
// d'orientation tiennent en i128 ; les barycentriques du triangle passent
// par OBig (produits < 2^140) — regime oracle assume.
#pragma once

#include "../../oracle/obigint.hpp"
#include "../events/q3_event.hpp"

namespace mhgp4 {

struct BallRat {
  i128 cnum[3];
  i128 cden;  // > 0 (A > 0 primitive)
};

inline BallRat ball_center(const Q3BallKey& k) {
  BallRat c;
  c.cden = 2 * k.a;
  for (int i = 0; i < 3; ++i) c.cnum[i] = -k.b[i];
  return c;
}

namespace detail_plateau {

using OBp = mhgp4_oracle::OBig<6>;
inline OBp obp(i128 v) { return OBp::from_i128(v); }

// c est-il le milieu de (t0, t1) ? (paire diametrale)
inline bool pair_diametral(const BallRat& c, const P3& t0, const P3& t1) {
  const i64 s[3] = {t0.x + t1.x, t0.y + t1.y, t0.z + t1.z};
  for (int i = 0; i < 3; ++i)
    if (2 * c.cnum[i] != c.cden * s[i]) return false;
  return true;
}

// c dans le triangle FERME (t0, t1, t2) (coplanaire + barycentriques >= 0).
inline bool triangle_closed(const BallRat& c, const P3& t0, const P3& t1,
                            const P3& t2) {
  const P3 e1 = p3_sub(t1, t0), e2 = p3_sub(t2, t0);
  const P3 n = p3_cross(e1, e2);
  if (n.x == 0 && n.y == 0 && n.z == 0) return false;  // degenere
  // Coplanarite : n·(c - t0) == 0, homogene par cden > 0.
  const i128 v[3] = {c.cnum[0] - c.cden * t0.x, c.cnum[1] - c.cden * t0.y,
                     c.cnum[2] - c.cden * t0.z};
  if (n.x * v[0] + n.y * v[1] + n.z * v[2] != 0) return false;
  // Barycentriques fermes : alpha, beta >= 0, alpha + beta <= 1, avec
  // alpha·(G·cden) = d1·E2 - d2·F etc., G > 0, cden > 0.
  const i128 d1 = v[0] * e1.x + v[1] * e1.y + v[2] * e1.z;
  const i128 d2 = v[0] * e2.x + v[1] * e2.y + v[2] * e2.z;
  const i64 E1 = p3_norm2(e1), E2 = p3_norm2(e2), F = p3_dot(e1, e2);
  const OBp a = obp(d1) * obp(E2) - obp(d2) * obp(F);
  const OBp b = obp(d2) * obp(E1) - obp(d1) * obp(F);
  if (a.sign() < 0 || b.sign() < 0) return false;
  const OBp gc = obp((i128)E1 * E2 - (i128)F * F) * obp(c.cden);
  return cmp(a + b, gc) <= 0;
}

// c dans le tetraedre FERME (t0..t3) (non degenere ; faces au sens large).
inline bool tetra_closed(const BallRat& c, const P3& t0, const P3& t1,
                         const P3& t2, const P3& t3) {
  const P3* v[4] = {&t0, &t1, &t2, &t3};
  for (int f = 0; f < 4; ++f) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != f) fp[t++] = v[i];
    const P3 e1 = p3_sub(*fp[1], *fp[0]), e2 = p3_sub(*fp[2], *fp[0]);
    const P3 es = p3_sub(*v[f], *fp[0]);
    const auto det3 = [&](i128 x, i128 y, i128 z) {
      return (i128)(e1.y * e2.z - e1.z * e2.y) * x -
             (i128)(e1.x * e2.z - e1.z * e2.x) * y +
             (i128)(e1.x * e2.y - e1.y * e2.x) * z;
    };
    const i128 side_s = det3(es.x, es.y, es.z);
    if (side_s == 0) return false;  // coplanaire : les triangles couvrent
    const i128 vc[3] = {c.cnum[0] - c.cden * fp[0]->x,
                        c.cnum[1] - c.cden * fp[0]->y,
                        c.cnum[2] - c.cden * fp[0]->z};
    const i128 side_c = det3(vc[0], vc[1], vc[2]);  // signe × cden > 0
    if (side_c != 0 && ((side_c > 0) != (side_s > 0))) return false;
  }
  return true;
}

}  // namespace detail_plateau

// c ∈ conv(T) FERME, par Caratheodory : une paire diametrale, un triangle
// ferme ou un tetraedre ferme de T contient c.
inline bool center_in_conv(const BallRat& c, const std::vector<P3>& pts,
                           const std::vector<i32>& T) {
  const size_t n = T.size();
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j) {
      if (detail_plateau::pair_diametral(c, pts[(size_t)T[i]], pts[(size_t)T[j]]))
        return true;
      for (size_t k = j + 1; k < n; ++k) {
        if (detail_plateau::triangle_closed(c, pts[(size_t)T[i]],
                                            pts[(size_t)T[j]], pts[(size_t)T[k]]))
          return true;
        for (size_t l = k + 1; l < n; ++l)
          if (detail_plateau::tetra_closed(c, pts[(size_t)T[i]], pts[(size_t)T[j]],
                                           pts[(size_t)T[k]], pts[(size_t)T[l]]))
            return true;
      }
    }
  return false;
}

}  // namespace mhgp4
