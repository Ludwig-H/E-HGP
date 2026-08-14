// MorseHGP3D v3 — `Corner8BallDepth` : LE SUPPORT COMPLET, ET SA SEULE BOULE.
//
// Specification : section 4 de
// audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md et
// section 6 de
// audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=certificat_exact_fail_open,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE DEPLACEMENT DE QUANTIFICATEUR
//
// Tant qu'on cherche a fermer une PAIRE, le centre reste une variable libre :
// il parcourt le disque de Jung, et tout le certificat consiste a quantifier
// sur ce continuum. C'est ce que font le masque central, `SOC64`, `JungDual` et
// `BlockJungDual64` — et c'est pourquoi ils echouent sur `95 %` des paires.
//
// Une fois le SUPPORT COMPLET, il n'y a plus de variable libre. Le support
// determine son evenement, et un seul :
//
//   q2  {a,b}      -> la boule de diametre `ab`
//   q3  {a,b,c}    -> le circumcentre du triangle DANS SON PLAN
//   q4  {a,b,c,d}  -> le circumcentre du tetraedre
//
// Il n'y a donc rien a quantifier : il faut compter les interieurs d'UNE
// sphere. C'est ce fichier pour q4.
//
// ---------------------------------------------------------------------------
// LE PREDICAT, SANS DIVISION NI CENTRE
//
// Avec `u=b-a`, `v=x-a`, `w=y-a` et `r=z-a` :
//
//   O(a,b,x,y)   = det3(u,v,w)
//   J(a,b,x,y,z) = det4( (u,||u||^2), (v,||v||^2), (w,||w||^2), (r,||r||^2) )
//
// et `z` est STRICTEMENT interieur a la circumsphere du support si et seulement
// si `O*J < 0`. Le circumcentre n'est jamais forme, aucune division n'est faite,
// et le predicat est exact sur les entiers.
//
// ---------------------------------------------------------------------------
// POURQUOI HUIT COINS SUFFISENT — ET SEULEMENT DANS UN SENS
//
// A support fixe, posons `sigma = signe(O)`. Le coefficient de `||z||^2` dans
// `sigma*J` vaut `|O| > 0` : la forme est donc STRICTEMENT CONVEXE en `z`. Son
// maximum sur une boite temoin est atteint a l'un des huit coins. Si les huit
// coins donnent `sigma*J < 0`, alors tout point de la boite — entier ou non —
// est interieur. Les coins sont donc COMPLETS pour prouver `ALL_INTERIOR`.
//
// L'inverse est FAUX et doit rester un mutant : une fonction convexe peut etre
// negative au centre et positive aux huit coins. La contre-fixture est le
// tetraedre `(3,2,2),(1,2,2),(2,3,2),(2,2,3)` avec la boite `[1,3]^3`, dont les
// coins sont exterieurs et le centre interieur.
//
// ---------------------------------------------------------------------------
// DU POINT AU BLOC
//
// Sur un bloc `A x B x C x D` de supports, `O` et `J` deviennent des
// intervalles. Le produit `O*J` n'est JAMAIS forme : les deux signes sont
// bornes separement.
//
//   O_L > 0 et J_U < 0  -> ALL_INTERIOR
//   O_U < 0 et J_L > 0  -> ALL_INTERIOR
//   sinon                -> MIXED
//
// Une dependance perdue par l'arithmetique d'intervalles ne cree que des
// `MIXED`. Le verdict `NONE` n'est jamais rendu ici : il exigerait une autre
// preuve, et l'audit le dit explicitement.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `U = 65535` : chaque coordonnee de difference est dans `[-U,U]`, chaque
// norme carree dans `[0,3U^2]`. Donc
//   |O|          <= 6 U^3        < 2^51
//   chaque monome de `J` <= 3 U^5   et la somme des 24 <= 72 U^5 < 2^87
// `i128` suffit, a condition de juger les signes SEPAREMENT.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "spindle_cone.hpp"

namespace mhgp3v {
namespace c8 {

using mhgp::i128;
using mhgp::i64;

using cone::Box;
using cone::box_corner;

enum class BallVerdict { kAllInterior, kMixed };

// ---------------------------------------------------------------------------
// MUTANTS.
// ---------------------------------------------------------------------------
enum class C8Mutant {
  kNone,
  kCornersOutsideImpliesNone,  // inverse l'implication : convexite ignoree
  kAcceptEquality,             // `<=` au lieu de `<` : le shell devient interieur
  kDropCorner,                 // sept coins au lieu de huit
  kProduitOJ,                  // forme le produit `O*J` : deborde et melange
  kNormeAuxCoins,              // borne `||p-q||^2` aux seuls coins de la boite
};

inline const char* c8_mutant_name(C8Mutant m) {
  switch (m) {
    case C8Mutant::kNone: return "none";
    case C8Mutant::kCornersOutsideImpliesNone: return "corners-outside-implies-none";
    case C8Mutant::kAcceptEquality: return "c8-accept-equality";
    case C8Mutant::kDropCorner: return "c8-drop-corner";
    case C8Mutant::kProduitOJ: return "c8-produit-oj";
    case C8Mutant::kNormeAuxCoins: return "c8-norme-aux-coins";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// PREDICATS PONCTUELS EXACTS.
// ---------------------------------------------------------------------------
MHGP_HD inline i128 det3(const i64 u[3], const i64 v[3], const i64 w[3]) {
  return (i128)u[0] * ((i128)v[1] * w[2] - (i128)v[2] * w[1]) -
         (i128)u[1] * ((i128)v[0] * w[2] - (i128)v[2] * w[0]) +
         (i128)u[2] * ((i128)v[0] * w[1] - (i128)v[1] * w[0]);
}

MHGP_HD inline i128 orient3d(const i64 a[3], const i64 b[3], const i64 x[3],
                             const i64 y[3]) {
  i64 u[3], v[3], w[3];
  for (int i = 0; i < 3; ++i) { u[i] = b[i] - a[i]; v[i] = x[i] - a[i]; w[i] = y[i] - a[i]; }
  return det3(u, v, w);
}

// `J` par developpement sur la derniere colonne : quatre mineurs `3x3`.
MHGP_HD inline i128 insphere_j(const i64 a[3], const i64 b[3], const i64 x[3],
                               const i64 y[3], const i64 z[3]) {
  i64 row[4][3];
  i128 nrm[4];
  const i64* pts[4] = {b, x, y, z};
  for (int k = 0; k < 4; ++k) {
    for (int i = 0; i < 3; ++i) row[k][i] = pts[k][i] - a[i];
    nrm[k] = (i128)row[k][0] * row[k][0] + (i128)row[k][1] * row[k][1] +
             (i128)row[k][2] * row[k][2];
  }
  // det4 des lignes `(row_k, nrm_k)`, developpe sur la quatrieme colonne.
  i128 acc = 0;
  for (int k = 0; k < 4; ++k) {
    int idx[3], n = 0;
    for (int t = 0; t < 4; ++t) if (t != k) idx[n++] = t;
    const i128 m = det3(row[idx[0]], row[idx[1]], row[idx[2]]);
    const i128 sgn = ((3 - k) % 2 == 0) ? 1 : -1;
    acc += sgn * nrm[k] * m;
  }
  return acc;
}

// `z` est-il STRICTEMENT interieur a la circumsphere de `(a,b,x,y)` ?
// Vrai si et seulement si `O*J < 0`. Le produit n'est forme nulle part : les
// deux signes sont compares.
MHGP_HD inline bool interieur_strict(const i64 a[3], const i64 b[3], const i64 x[3],
                                     const i64 y[3], const i64 z[3]) {
  const i128 o = orient3d(a, b, x, y);
  if (o == 0) return false;   // support degenere : jamais un interieur
  const i128 j = insphere_j(a, b, x, y, z);
  if (j == 0) return false;   // shell
  return (o > 0) != (j > 0);
}

// ---------------------------------------------------------------------------
// ARITHMETIQUE D'INTERVALLES, i128.
// ---------------------------------------------------------------------------
struct Iv {
  i128 lo = 0, hi = 0;
};

MHGP_HD inline Iv iv_add(const Iv& p, const Iv& q) { return Iv{p.lo + q.lo, p.hi + q.hi}; }
MHGP_HD inline Iv iv_sub(const Iv& p, const Iv& q) { return Iv{p.lo - q.hi, p.hi - q.lo}; }

MHGP_HD inline Iv iv_mul(const Iv& p, const Iv& q) {
  const i128 c[4] = {p.lo * q.lo, p.lo * q.hi, p.hi * q.lo, p.hi * q.hi};
  i128 mn = c[0], mx = c[0];
  for (int k = 1; k < 4; ++k) {
    if (c[k] < mn) mn = c[k];
    if (c[k] > mx) mx = c[k];
  }
  return Iv{mn, mx};
}

// Intervalle de `det3` de trois lignes elles-memes intervalles.
MHGP_HD inline Iv iv_det3(const Iv u[3], const Iv v[3], const Iv w[3]) {
  const Iv m0 = iv_sub(iv_mul(v[1], w[2]), iv_mul(v[2], w[1]));
  const Iv m1 = iv_sub(iv_mul(v[0], w[2]), iv_mul(v[2], w[0]));
  const Iv m2 = iv_sub(iv_mul(v[0], w[1]), iv_mul(v[1], w[0]));
  return iv_add(iv_sub(iv_mul(u[0], m0), iv_mul(u[1], m1)), iv_mul(u[2], m2));
}

// Intervalle de `p_i - q_i` pour `p` dans la boite `P` et `q` FIXE.
MHGP_HD inline void diff_box_point(const Box& P, const i64 q[3], Iv out[3]) {
  for (int i = 0; i < 3; ++i) {
    out[i].lo = (i128)P.lo[i] - (i128)q[i];
    out[i].hi = (i128)P.hi[i] - (i128)q[i];
  }
}

// Intervalle de `||p-q||^2`. Le minimum peut etre INTERIEUR a la boite : le
// calculer aux seuls coins serait non sur, et c'est le mutant
// `c8-norme-aux-coins`.
MHGP_HD inline Iv norme_box_point(const Box& P, const i64 q[3], C8Mutant mu) {
  i128 lo = 0, hi = 0;
  for (int i = 0; i < 3; ++i) {
    const i128 dl = (i128)P.lo[i] - (i128)q[i];
    const i128 dh = (i128)P.hi[i] - (i128)q[i];
    const i128 a2 = dl * dl, b2 = dh * dh;
    hi += (a2 > b2) ? a2 : b2;
    if (mu == C8Mutant::kNormeAuxCoins) {
      lo += (a2 < b2) ? a2 : b2;
    } else {
      // Zero des que `q_i` tombe dans l'intervalle : la parabole y touche son
      // sommet.
      if (q[i] >= P.lo[i] && q[i] <= P.hi[i]) continue;
      lo += (a2 < b2) ? a2 : b2;
    }
  }
  return Iv{lo, hi};
}

// ---------------------------------------------------------------------------
// LE VERDICT DE BLOC.
//
// `A,B,X,Y` portent le support ; `Z` porte le temoin. Rend `kAllInterior`
// seulement si TOUT point de `Z` est strictement interieur a la circumsphere de
// TOUT support du bloc.
// ---------------------------------------------------------------------------
// L'orientation ne depend QUE des quatre facteurs support, jamais du temoin :
// la tester une fois par bloc, et non une fois par nœud visite, est a la fois
// un diagnostic et une economie. Rend `0` si le signe n'est pas fixe.
MHGP_HD inline int orientation_signe(const Box& A, const Box& B, const Box& X,
                                     const Box& Y) {
  // ---- FORME CENTREE, ET POURQUOI ELLE CHANGE TOUT.
  //
  // L'arithmetique d'intervalles naive developpe `det3` en six monomes traites
  // comme independants : son erreur est du PREMIER ordre en le rayon des
  // boites, et elle laisse le signe indecis sur `77 %` des blocs meme quand les
  // cellules sont minuscules.
  //
  // `det3` est TRILINEAIRE. En ecrivant `u = uc + du` avec `|du| <= ur`, le
  // developpement a huit termes se groupe par ordre :
  //
  //   ordre 0 : det3(uc,vc,wc)                    — la valeur au centre
  //   ordre 1 : somme_i ur_i |cross(vc,wc)_i| + les deux permutations
  //   ordre 2 et 3 : bornes par intervalles, mais ils sont en `r^2` et `r^3`
  //
  // L'erreur devient donc quadratique en le rayon. Tout est fait sur les
  // quantites DOUBLEES `2u = (B.lo+B.hi) - (A.lo+A.hi)` et
  // `2*ur = (B.hi-B.lo) + (A.hi-A.lo)`, ce qui evite toute division : le
  // determinant y est huit fois le vrai, et le SIGNE est le meme.
  i128 uc[3], ur[3], vc[3], vr[3], wc[3], wr[3];
  for (int i = 0; i < 3; ++i) {
    uc[i] = ((i128)B.lo[i] + B.hi[i]) - ((i128)A.lo[i] + A.hi[i]);
    ur[i] = ((i128)B.hi[i] - B.lo[i]) + ((i128)A.hi[i] - A.lo[i]);
    vc[i] = ((i128)X.lo[i] + X.hi[i]) - ((i128)A.lo[i] + A.hi[i]);
    vr[i] = ((i128)X.hi[i] - X.lo[i]) + ((i128)A.hi[i] - A.lo[i]);
    wc[i] = ((i128)Y.lo[i] + Y.hi[i]) - ((i128)A.lo[i] + A.hi[i]);
    wr[i] = ((i128)Y.hi[i] - Y.lo[i]) + ((i128)A.hi[i] - A.lo[i]);
  }
  auto cross_abs = [](const i128 p[3], const i128 q[3], i128 out[3]) {
    for (int i = 0; i < 3; ++i) {
      const int j = (i + 1) % 3, k = (i + 2) % 3;
      const i128 c = p[j] * q[k] - p[k] * q[j];
      out[i] = c < 0 ? -c : c;
    }
  };
  // Valeur au centre : `det3(uc,vc,wc)`.
  i128 cvw[3];
  for (int i = 0; i < 3; ++i) {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    cvw[i] = vc[j] * wc[k] - vc[k] * wc[j];
  }
  i128 centre = 0;
  for (int i = 0; i < 3; ++i) centre += uc[i] * cvw[i];

  // Ordre un : trois produits scalaires de rayons par des cofacteurs absolus.
  i128 e1 = 0;
  {
    i128 acvw[3], acwu[3], acuv[3];
    cross_abs(vc, wc, acvw);
    cross_abs(wc, uc, acwu);
    cross_abs(uc, vc, acuv);
    for (int i = 0; i < 3; ++i)
      e1 += ur[i] * acvw[i] + vr[i] * acwu[i] + wr[i] * acuv[i];
  }
  // Ordres deux et trois : les rayons sont positifs, donc majorer chaque
  // determinant par la somme de ses six monomes absolus suffit.
  auto det_abs_bound = [](const i128 p[3], const i128 q[3], const i128 r[3]) {
    i128 acc = 0;
    for (int i = 0; i < 3; ++i) {
      const int j = (i + 1) % 3, k = (i + 2) % 3;
      const i128 t1 = p[i] * q[j] * r[k];
      const i128 t2 = p[i] * q[k] * r[j];
      acc += (t1 < 0 ? -t1 : t1) + (t2 < 0 ? -t2 : t2);
    }
    return acc;
  };
  auto absv = [](const i128 p[3], i128 out[3]) {
    for (int i = 0; i < 3; ++i) out[i] = p[i] < 0 ? -p[i] : p[i];
  };
  i128 auc[3], avc[3], awc[3];
  absv(uc, auc); absv(vc, avc); absv(wc, awc);
  const i128 e2 = det_abs_bound(ur, vr, awc) + det_abs_bound(ur, avc, wr) +
                  det_abs_bound(auc, vr, wr);
  const i128 e3 = det_abs_bound(ur, vr, wr);
  const i128 err = e1 + e2 + e3;
  if (centre > err) return 1;
  if (centre < -err) return -1;
  return 0;
}

MHGP_HD inline BallVerdict corner8_block(const Box& A, const Box& B, const Box& X,
                                         const Box& Y, const Box& Z,
                                         C8Mutant mu = C8Mutant::kNone,
                                         long long* coins = nullptr) {
  // Orientation : son signe doit etre CONSTANT sur tout le bloc, sinon la
  // convexite ne fixe pas de direction et le verdict reste `MIXED`.
  Iv u[3], v[3], w[3];
  for (int i = 0; i < 3; ++i) {
    u[i] = Iv{(i128)B.lo[i] - (i128)A.hi[i], (i128)B.hi[i] - (i128)A.lo[i]};
    v[i] = Iv{(i128)X.lo[i] - (i128)A.hi[i], (i128)X.hi[i] - (i128)A.lo[i]};
    w[i] = Iv{(i128)Y.lo[i] - (i128)A.hi[i], (i128)Y.hi[i] - (i128)A.lo[i]};
  }
  const Iv o = iv_det3(u, v, w);
  int sigma = 0;
  if (o.lo > 0) sigma = 1;
  else if (o.hi < 0) sigma = -1;
  else return BallVerdict::kMixed;   // le signe de l'orientation n'est pas fixe

  // Pour chaque coin `q` de la boite temoin, borner `J` translate en `q` sur
  // tout le bloc support. La convexite stricte en `z` rend les huit coins
  // COMPLETS pour prouver l'interieur.
  const int ncoins = (mu == C8Mutant::kDropCorner) ? 7 : 8;
  for (int k = 0; k < ncoins; ++k) {
    i64 q[3];
    box_corner(Z, k, q);
    if (coins != nullptr) ++*coins;
    // Lignes du determinant translate : `(p-q, ||p-q||^2)` pour `p` dans
    // chacune des quatre boites support.
    const Box* boxes[4] = {&A, &B, &X, &Y};
    Iv row[4][3];
    Iv nrm[4];
    for (int t = 0; t < 4; ++t) {
      diff_box_point(*boxes[t], q, row[t]);
      nrm[t] = norme_box_point(*boxes[t], q, mu);
    }
    // det4 developpe sur la quatrieme colonne, en intervalles.
    Iv acc{0, 0};
    for (int t = 0; t < 4; ++t) {
      int idx[3], n = 0;
      for (int s = 0; s < 4; ++s) if (s != t) idx[n++] = s;
      const Iv m = iv_det3(row[idx[0]], row[idx[1]], row[idx[2]]);
      const Iv term = iv_mul(nrm[t], m);
      if ((3 - t) % 2 == 0) acc = iv_add(acc, term);
      else acc = iv_sub(acc, term);
    }
    // `z` interieur <=> `sigma * J < 0`. Le produit `O*J` n'est pas forme.
    const i128 borne = (sigma > 0) ? acc.hi : -acc.lo;
    const bool ok = (mu == C8Mutant::kAcceptEquality) ? (borne <= 0) : (borne < 0);
    if (mu == C8Mutant::kCornersOutsideImpliesNone) {
      // MUTANT : conclure de coins EXTERIEURS a un bloc entierement exterieur.
      // La convexite ne permet que l'implication inverse.
      if (!ok) return BallVerdict::kAllInterior;
      continue;
    }
    if (!ok) return BallVerdict::kMixed;
  }
  return BallVerdict::kAllInterior;
}

}  // namespace c8
}  // namespace mhgp3v
