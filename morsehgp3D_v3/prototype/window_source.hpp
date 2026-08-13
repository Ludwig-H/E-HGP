// MorseHGP3D v3 — FENETRE k-NN CERTIFIEE : primitives ENTIERES du sujet.
//
// Specification : audits/NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md, section
// 4.1, telle que corrigee par
// audits/AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md, section 1.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE THEOREME, ET SEULEMENT LUI
//
// Soit `W_M(a)` l'ensemble exact des `M` AUTRES sites les plus proches de `a`,
// et soit la coupure au premier site OMIS
//
//   delta_out(a)^2 = min { |x-a|^2 : x hors de {a} union W_M(a) },
//
// valant +infini quand rien n'est omis. Pour une boule fermee `B(c,R)` dont `a`
// est un point de support, tout `y` de `B` verifie
// |y-a| <= |y-c| + |c-a| <= 2R. Donc
//
//   4 R^2 < delta_out(a)^2  ==>  X inter B inclus dans {a} union W_M(a).
//
// L'inclusion couvre le support `S`, les interieurs stricts `I_B` ET TOUT LE
// SHELL GLOBAL `U_B`. C'est la seule chose dont le certificat a besoin, et elle
// ne depend d'AUCUN compte : `smax` borne `|I_B|+|S|`, il ne borne NI `|U_B|`
// NI le rang ferme. Une version anterieure de la note affirmait le contraire ;
// le depot possede un support q2 pertinent de rang ferme douze et une fixture a
// shell trente.
//
// LA COUPURE EST LE PREMIER OMIS, PAS LE M-IEME INCLUS. Avec le M-ieme inclus,
// ce site-la ne peut lui-meme appartenir a aucune boule certifiee : la
// comparaison reste sure mais perd des supports sans raison. La primitive lit
// donc `M+1` voisins et n'en garde que `M`.
//
// L'INEGALITE EST STRICTE. A l'egalite, le premier omis peut etre sur la
// sphere : le cas part au residuel. Fixture du mutant `<=` :
// a=(0,0,0), b=(2,0,0), c=(0,2,0), M=1, ou 4R^2 = delta_out^2 = 4.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Pour un support {p_0..p_{q-1}}, poser v_i = p_i - p_0 et M = 2 V V^T. Par
// Cramer, le centre est p_0 + g/den avec den = det(M) > 0 (M est definie
// positive) et g = somme des det(M_i) v_i. Alors R^2 = |g|^2 / den^2.
//
//   |v_i . v_j|            <= 3 * 65535^2         = 1,29e10
//   entrees de M           <= 2,58e10
//   den = det(M), q=4      <= 6 * (2,58e10)^3     = 1,03e32     (i128)
//   det(M_i), q=4          <= 6 * (2,58e10)^2 * 1,29e10 = 5,2e31 (i128)
//   |g_j|, q=4             <= 3 * 5,2e31 * 65535  = 1,02e37     (i128, max 1,7e38)
//   |g|^2                  <= 3 * (1,02e37)^2     = 3,1e74      (BigInt<4>)
//   4|g|^2                 <= 1,25e75             ; BigInt<4> signe va a 5,79e76
//   |den*(z-p0) - g|^2     <= 3 * (1,7e37)^2      = 8,7e74      (BigInt<4>)
//
// Toutes les marges sont superieures a quarante. Aucun produit n'est forme en
// i64. Le mutant etroit doit mourir.
//
// ---------------------------------------------------------------------------
// CHEMIN RAPIDE ENTIER PAR JUNG
//
// La comparaison `4R^2 < delta_out^2` n'est PAS en general celle de deux
// entiers 64 bits : `R` est rationnel. Un chemin rapide existe pourtant. Pour
// un support propre positif, `B` est la miniboule de `S`, donc Jung donne
// R^2 <= (3/8) diam(S)^2, soit 4R^2 <= (3/2) diam(S)^2. Ainsi
//
//   3 diam(S)^2 < 2 delta_out(a)^2  ==>  4R^2 < delta_out(a)^2,
//
// et CE test-la tient entierement dans i64 sur le profil u16. Il est seulement
// SUFFISANT : son echec renvoie au test exact ci-dessus, jamais au residuel.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace window {

using mhgp::BigInt;
using mhgp::i128;
using mhgp::i64;
using mhgp::P3;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse UNE decision exacte.
// ---------------------------------------------------------------------------
enum class WindowMutant {
  kNone,
  kCutAtIncluded,    // coupure au M-ieme INCLUS au lieu du premier omis
  kAcceptEquality,   // `<=` au lieu de `<` dans le certificat
  kSkipPositivity,   // emet les supports a centre hors enveloppe convexe
  kNarrowI64,        // ne promeut pas : le certificat decide en i64
  kShellAsInterior,  // compte le shell dans `I_B`
  kJungOnly,         // n'essaie jamais le test exact apres l'echec de Jung
};

inline const char* window_mutant_name(WindowMutant m) {
  switch (m) {
    case WindowMutant::kNone: return "none";
    case WindowMutant::kCutAtIncluded: return "window-cut-at-included";
    case WindowMutant::kAcceptEquality: return "window-accept-equality";
    case WindowMutant::kSkipPositivity: return "window-skip-positivity";
    case WindowMutant::kNarrowI64: return "window-narrow-i64";
    case WindowMutant::kShellAsInterior: return "window-shell-as-interior";
    case WindowMutant::kJungOnly: return "window-jung-only";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// SPHERE CIRCONSCRITE ENTIERE d'un support affinement independant.
// centre = p_0 + g/den, R^2 = |g|^2/den^2, den > 0.
// ---------------------------------------------------------------------------
struct IntSphere {
  i128 g[3] = {0, 0, 0};
  i128 den = 0;
  bool ok = false;        // affinement independant
  bool positive = false;  // toutes les coordonnees barycentriques > 0
};

inline i128 dot3(const i64 a[3], const i64 b[3]) {
  return (i128)a[0] * b[0] + (i128)a[1] * b[1] + (i128)a[2] * b[2];
}

// Determinant 2x2 et 3x3 sur i128. Les entrees restent bornees par 2,58e10 :
// le 3x3 vaut au plus 1,03e32 et ne deborde pas.
inline i128 det2(i128 a, i128 b, i128 c, i128 d) { return a * d - b * c; }

inline i128 det3(const i128 m[3][3]) {
  return m[0][0] * det2(m[1][1], m[1][2], m[2][1], m[2][2]) -
         m[0][1] * det2(m[1][0], m[1][2], m[2][0], m[2][2]) +
         m[0][2] * det2(m[1][0], m[1][1], m[2][0], m[2][1]);
}

// `q` vaut 2, 3 ou 4. `pts[ids[0]]` est l'origine locale.
inline IntSphere sphere_of(const P3* pts, const int* ids, int q) {
  IntSphere s{};
  if (q < 2 || q > 4) return s;
  const P3& p0 = pts[ids[0]];
  i64 v[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  const int k = q - 1;
  for (int i = 0; i < k; ++i) {
    const P3& pi = pts[ids[i + 1]];
    v[i][0] = (i64)pi.x - (i64)p0.x;
    v[i][1] = (i64)pi.y - (i64)p0.y;
    v[i][2] = (i64)pi.z - (i64)p0.z;
  }
  i128 mat[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  i128 rhs[3] = {0, 0, 0};
  for (int i = 0; i < k; ++i) {
    for (int j = 0; j < k; ++j) mat[i][j] = 2 * dot3(v[i], v[j]);
    rhs[i] = dot3(v[i], v[i]);
  }

  i128 num[3] = {0, 0, 0};
  i128 den = 0;
  if (k == 1) {
    den = mat[0][0];
    num[0] = rhs[0];
  } else if (k == 2) {
    den = det2(mat[0][0], mat[0][1], mat[1][0], mat[1][1]);
    num[0] = det2(rhs[0], mat[0][1], rhs[1], mat[1][1]);
    num[1] = det2(mat[0][0], rhs[0], mat[1][0], rhs[1]);
  } else {
    den = det3(mat);
    for (int c = 0; c < 3; ++c) {
      i128 mc[3][3];
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) mc[i][j] = (j == c) ? rhs[i] : mat[i][j];
      num[c] = det3(mc);
    }
  }
  if (den == 0) return s;  // affinement dependant : refus, jamais un jitter
  // `mat` est definie positive quand les `v_i` sont independants, donc den > 0.
  // La normalisation reste ecrite : elle rend la positivite lisible et ne coute
  // rien.
  if (den < 0) {
    den = -den;
    for (int i = 0; i < k; ++i) num[i] = -num[i];
  }
  for (int j = 0; j < 3; ++j) {
    i128 acc = 0;
    for (int i = 0; i < k; ++i) acc += num[i] * (i128)v[i][j];
    s.g[j] = acc;
  }
  s.den = den;
  s.ok = true;
  // Coordonnees barycentriques : lambda_i = num_i/den pour i>=1, et
  // lambda_0 = (den - somme num_i)/den. Toutes strictement positives.
  i128 sum = 0;
  bool positive = true;
  for (int i = 0; i < k; ++i) {
    if (num[i] <= 0) positive = false;
    sum += num[i];
  }
  if (den - sum <= 0) positive = false;
  s.positive = positive;
  return s;
}

// ---------------------------------------------------------------------------
// Carre exact d'un vecteur i128, sur 256 bits.
// ---------------------------------------------------------------------------
inline BigInt<4> norm2_big(const i128 w[3]) {
  BigInt<4> acc = mhgp::big_zero<4>();
  for (int j = 0; j < 3; ++j) acc = mhgp::big_add(acc, mhgp::mul128(w[j], w[j]));
  return acc;
}

// Position de `z` par rapport a la sphere : -1 dedans, 0 sur le bord, +1 dehors.
// |z-c|^2 vs R^2 se compare apres multiplication par den^2 > 0 :
//   |den*(z-p0) - g|^2   vs   |g|^2.
inline int side_of(const IntSphere& s, const P3& p0, const P3& z) {
  const i64 d[3] = {(i64)z.x - (i64)p0.x, (i64)z.y - (i64)p0.y, (i64)z.z - (i64)p0.z};
  i128 w[3];
  for (int j = 0; j < 3; ++j) w[j] = s.den * (i128)d[j] - s.g[j];
  const BigInt<4> lhs = norm2_big(w);
  i128 gg[3] = {s.g[0], s.g[1], s.g[2]};
  const BigInt<4> rhs = norm2_big(gg);
  return mhgp::big_cmp(lhs, rhs);
}

// ---------------------------------------------------------------------------
// LE CERTIFICAT DE FENETRE : 4 R^2 < delta_out^2, soit 4|g|^2 < delta_out^2 den^2.
// `delta_out2 < 0` signifie +infini, c'est-a-dire un scan total sans omission.
// ---------------------------------------------------------------------------
inline bool window_certified(const IntSphere& s, i64 delta_out2,
                             WindowMutant mu = WindowMutant::kNone) {
  if (delta_out2 < 0) return true;  // rien n'est omis : tout est dans la fenetre
  if (mu == WindowMutant::kNarrowI64) {
    // MUTANT : le certificat decide en i64. Les carres de `g` debordent des
    // les supports q3 d'un nuage ordinaire, et la comparaison devient un tirage.
    const i64 g0 = (i64)s.g[0], g1 = (i64)s.g[1], g2 = (i64)s.g[2];
    const i64 den = (i64)s.den;
    return 4 * (g0 * g0 + g1 * g1 + g2 * g2) < delta_out2 * den * den;
  }
  i128 gg[3] = {s.g[0], s.g[1], s.g[2]};
  const BigInt<4> lhs = mhgp::big_mul_i128<4>(norm2_big(gg), (i128)4);
  const BigInt<4> rhs = mhgp::big_mul_i128<4>(mhgp::mul128(s.den, s.den), (i128)delta_out2);
  const int c = mhgp::big_cmp(lhs, rhs);
  // L'EGALITE PART AU RESIDUEL. A `4R^2 = delta_out^2`, le premier site omis
  // peut etre exactement sur la sphere, donc appartenir a `U_B`.
  return (mu == WindowMutant::kAcceptEquality) ? (c <= 0) : (c < 0);
}

// Chemin rapide de Jung, entierement en i64 : 3 diam(S)^2 < 2 delta_out^2.
// Suffisant, jamais necessaire. Son echec renvoie au test exact.
inline bool jung_fast_path(const P3* pts, const int* ids, int q, i64 delta_out2) {
  if (delta_out2 < 0) return true;
  i64 diam2 = 0;
  for (int i = 0; i < q; ++i)
    for (int j = i + 1; j < q; ++j) {
      const i64 dx = (i64)pts[ids[i]].x - (i64)pts[ids[j]].x;
      const i64 dy = (i64)pts[ids[i]].y - (i64)pts[ids[j]].y;
      const i64 dz = (i64)pts[ids[i]].z - (i64)pts[ids[j]].z;
      const i64 d2 = dx * dx + dy * dy + dz * dz;
      if (d2 > diam2) diam2 = d2;
    }
  return 3 * diam2 < 2 * delta_out2;
}

}  // namespace window
}  // namespace mhgp3v
