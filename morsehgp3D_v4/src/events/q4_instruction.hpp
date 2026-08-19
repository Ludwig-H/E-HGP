// MorseHGP3D v4 — INSTRUCTION q4 : du seed aigu a l'evenement tetraedrique.
//
// Dossier : docs/MATHEMATIQUES.md § 4.5 (derive_v4). Resume :
//   - circumcentre par Cramer 3×3 RELATIF : M = (2(b-a), 2(x-a), 2(y-a)),
//     r = (|b-a|², |x-a|², |y-a|²), M·(c-a) = r, c-a = N'/det ;
//     canonisation d'orientation det > 0 (negation simultanee (det, N')) ;
//     det = 0 (coplanaires) : jamais un support q4, candidat ignore ;
//   - puissance AFFINE (pas de carre) : P4(z) = det·|z-a|² - 2·N'·(z-a)
//     = det·(|z-c|² - R²) ; sous det > 0 : < 0 interieur strict, = 0 shell.
//     Largeurs u16 : det < 2^57, |N'_i| < 2^72, P4 < 2^94 -> i128 ;
//   - arite 4 STRICTE : pour chaque face (p,q,r) de sommet oppose s,
//     sign(det3(q-p, r-p, N'-det·(p-a))) non nul et egal a
//     sign(det3(q-p, r-p, s-p)) ; largeurs < 2^112 -> i128. Un zero =>
//     centre sur une face => arite <= 3 => une autre lane ;
//   - owner 6 aretes : ab maximale, ex aequo par plus petite EdgeKey ;
//   - exact-once du seed (lemme du prefixe ternaire, theoreme_v3) : emettre
//     seulement si le carrier du seed est le plus petit PointId parmi les
//     faces incidentes a ab strictement aigues du tetraedre FORME.
#pragma once

#include "q3_instruction.hpp"

namespace mhgp4 {

struct Q4Form {
  i128 det = 0;    // > 0 apres canonisation ; 0 = coplanaire (refus)
  i128 np[3] = {0, 0, 0};  // N' : c - a = N'/det
  P3 a;            // origine de la forme
};

namespace detail_q4 {

inline i128 det3_i128(const i64 r0[3], const i64 r1[3], const i128 r2[3]) {
  return r2[0] * ((i128)r0[1] * r1[2] - (i128)r0[2] * r1[1]) -
         r2[1] * ((i128)r0[0] * r1[2] - (i128)r0[2] * r1[0]) +
         r2[2] * ((i128)r0[0] * r1[1] - (i128)r0[1] * r1[0]);
}

}  // namespace detail_q4

// Forme du circumcentre du tetraedre {a,b,x,y}. det = 0 si coplanaires.
inline Q4Form q4_form(const P3& a, const P3& b, const P3& x, const P3& y) {
  const P3 e1 = p3_sub(b, a);
  const P3 e2 = p3_sub(x, a);
  const P3 e3 = p3_sub(y, a);
  const i64 m[3][3] = {{2 * e1.x, 2 * e1.y, 2 * e1.z},
                       {2 * e2.x, 2 * e2.y, 2 * e2.z},
                       {2 * e3.x, 2 * e3.y, 2 * e3.z}};
  const i64 r[3] = {p3_norm2(e1), p3_norm2(e2), p3_norm2(e3)};
  Q4Form f;
  f.a = a;
  // det et N' par cofacteurs (adj(M)·r) ; tout en i128 (largeurs § 4.5).
  const auto cof = [&](int i0, int i1, int j0, int j1) {
    return (i128)m[i0][j0] * m[i1][j1] - (i128)m[i0][j1] * m[i1][j0];
  };
  const i128 c00 = cof(1, 2, 1, 2), c01 = -cof(1, 2, 0, 2), c02 = cof(1, 2, 0, 1);
  const i128 c10 = -cof(0, 2, 1, 2), c11 = cof(0, 2, 0, 2), c12 = -cof(0, 2, 0, 1);
  const i128 c20 = cof(0, 1, 1, 2), c21 = -cof(0, 1, 0, 2), c22 = cof(0, 1, 0, 1);
  f.det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
  f.np[0] = c00 * r[0] + c10 * r[1] + c20 * r[2];
  f.np[1] = c01 * r[0] + c11 * r[1] + c21 * r[2];
  f.np[2] = c02 * r[0] + c12 * r[1] + c22 * r[2];
  if (f.det < 0) {  // canonisation d'orientation : centre inchange
    f.det = -f.det;
    for (int i = 0; i < 3; ++i) f.np[i] = -f.np[i];
  }
  return f;
}

// P4(z) exact (i128, largeurs § 4.5). Precondition : det > 0.
inline i128 q4_power(const Q4Form& f, const P3& z) {
  const P3 v = p3_sub(z, f.a);
  return f.det * p3_norm2(v) -
         2 * (f.np[0] * v.x + f.np[1] * v.y + f.np[2] * v.z);
}

// Le circumcentre est-il STRICTEMENT interieur au tetraedre ? (arite 4
// stricte ; un zero => centre sur une face => le support retombe a une
// autre arite.) Precondition : det > 0.
inline bool q4_center_strictly_inside(const Q4Form& f, const P3& a, const P3& b,
                                      const P3& x, const P3& y,
                                      bool mutant_parity = false) {
  // LE VOLUME NE SE CALCULE QU'UNE FOIS (mesure du 19 aout : ce test tue
  // 42 % des paires (seed, y) de la complétion q4, premier poste CPU du
  // pipeline — et il recalculait quatre fois le meme determinant).
  //
  // Preuve. Posons u = b-a, p = x-a, q = y-a et V = det3(u, p, q).
  // L'orientation du sommet oppose a la face s est
  //   s=3 (face abx, oppose y) : det3(b-a, x-a, y-a)            = V ;
  //   s=2 (face aby, oppose x) : det3(b-a, y-a, x-a)            = -V
  //        (echange des deux dernieres lignes) ;
  //   s=1 (face axy, oppose b) : det3(x-a, y-a, b-a)            = V
  //        (permutation CYCLIQUE de (u,p,q), donc paire) ;
  //   s=0 (face bxy, oppose a) : det3(x-b, y-b, a-b)
  //        = det3(p-u, q-u, -u) = -det3(p-u, q-u, u) = -det3(p, q, u) = -V
  //        (multilinearite : les trois autres termes portent deux fois u).
  // Les quatre signes sont donc (-V, +V, -V, +V) : un seul determinant
  // suffit, et il tient en i64 (coordonnees u16, differences < 2^17,
  // det3 < 6·2^51). Seules les quatre orientations du CENTRE dependent
  // vraiment de la face et restent en i128.
  //
  // `V != 0` est garanti par la precondition : det = 8·V, et l'appelant
  // a deja refuse det == 0 (quatre points coplanaires).
  //
  // MUTANT q4-center-parity : parite inversee — le test devient faux sur
  // la moitie des faces.
  const i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
  const i64 px = x.x - a.x, py = x.y - a.y, pz = x.z - a.z;
  const i64 qx = y.x - a.x, qy = y.y - a.y, qz = y.z - a.z;
  const i64 vol = ux * (py * qz - pz * qy) - uy * (px * qz - pz * qx) +
                  uz * (px * qy - py * qx);
  const P3* v[4] = {&a, &b, &x, &y};
  for (int s = 0; s < 4; ++s) {
    // Face = les trois sommets != s, sommet oppose = v[s].
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = v[i];
    const P3 e1 = p3_sub(*fp[1], *fp[0]);
    const P3 e2 = p3_sub(*fp[2], *fp[0]);
    const i64 r0[3] = {e1.x, e1.y, e1.z};
    const i64 r1[3] = {e2.x, e2.y, e2.z};
    const P3 dp = p3_sub(*fp[0], f.a);
    const i128 rc[3] = {f.np[0] - f.det * dp.x, f.np[1] - f.det * dp.y,
                        f.np[2] - f.det * dp.z};
    const bool even_face = (s % 2) == 0;
    const bool side_s_pos =
        (even_face != mutant_parity) ? (vol < 0) : (vol > 0);
    const i128 side_c = detail_q4::det3_i128(r0, r1, rc);
    if (side_c == 0) return false;                   // centre sur la face
    if ((side_c > 0) != side_s_pos) return false;    // hors du tetraedre
  }
  return true;
}

// (a,b) est-elle l'arete owner du tetraedre {a,b,x,y} ? ab maximale parmi
// les six longueurs carrees, ex aequo departages par plus petite EdgeKey.
inline bool tetra_owned_by(i64 l_ab, i64 l_ax, i64 l_ay, i64 l_bx, i64 l_by,
                           i64 l_xy, PointId ida, PointId idb, PointId idx,
                           PointId idy) {
  const EdgeKey e_ab = edge_key(ida, idb);
  const auto beats = [&](i64 l, PointId u, PointId w) {
    return l > l_ab || (l == l_ab && edge_key_less(edge_key(u, w), e_ab));
  };
  return !beats(l_ax, ida, idx) && !beats(l_ay, ida, idy) &&
         !beats(l_bx, idb, idx) && !beats(l_by, idb, idy) &&
         !beats(l_xy, idx, idy);
}

// SupportKey q4 : quadruplet trie de vrais PointId.
struct SupportKey4 {
  PointId p[4];
  bool operator==(const SupportKey4& o) const {
    return p[0] == o.p[0] && p[1] == o.p[1] && p[2] == o.p[2] && p[3] == o.p[3];
  }
  bool operator<(const SupportKey4& o) const {
    for (int i = 0; i < 4; ++i)
      if (p[i] != o.p[i]) return p[i] < o.p[i];
    return false;
  }
};

inline SupportKey4 support_key4(PointId a, PointId b, PointId x, PointId y) {
  SupportKey4 k{{a, b, x, y}};
  for (int i = 1; i < 4; ++i)
    for (int j = i; j > 0 && k.p[j - 1] > k.p[j]; --j) std::swap(k.p[j - 1], k.p[j]);
  return k;
}

}  // namespace mhgp4
