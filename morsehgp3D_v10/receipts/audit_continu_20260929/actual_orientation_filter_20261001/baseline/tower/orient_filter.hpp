// Filtre semi-statique de l'orientation du centre d'une MEB q4 par rapport a une face, dans la certification de MEB de
// la descente (tower.cpp, verify_meb). En-tete propre pour etre juge directement (fixture FX-ORIENT de
// mhgp10_precision_b21) : un filtre qui tranche a tort ne change en general que le chemin (MEB rejetee puis Welzl
// exact), pas les sorties de la tour ; seul un juge direct, sur un centre dans le plan d'une face (orientation exacte
// nulle), voit une borne trop petite (constat S3 de la revue B21 : 2^-80 au lieu de 2^-49 survivait a la porte).
//
// Hypotheses de la borne, a la charge de l'unite qui l'inclut (tower.cpp, et la porte qui le juge) : binaire64 strict
// sans reassociation ni precision etendue (refus a la compilation ci-dessous), conversions i128 -> double correctement
// arrondies, magnitudes du palier servi (ci-dessous). Sous un arrondi dirige (erreur relative < 2u par operation), la
// borne tiendrait encore : |s - v| < ((1 + 2u)^4 - 1) S' < 8,1u S' et fl(S) >= (1 - 2u)^4 S', S' = sum |w_i cc_i| ; la
// descente coupe pourtant ce filtre hors FE_TONEAREST, comme les filtres de distance (Arith::filters). Une contraction
// FMA ne fait que supprimer des arrondis de produits : la borne tient aussi.
#pragma once

#include <cmath>

#include "arith/geometry.hpp"
#include "core/types.hpp"

#if defined(__FAST_MATH__) || defined(__ASSOCIATIVE_MATH__) || defined(__RECIPROCAL_MATH__)
#error "mhgp10_orient_filter_fast_math_interdit : le filtre semi-statique d'orientation exige IEEE-754 strict"
#endif
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ != 0
#error "mhgp10_orient_filter_precision_etendue_interdite : le filtre semi-statique d'orientation exige le binaire64"
#endif

namespace mhgp10::orient_filter {

// Signe de w . (c - p) pour le centre c = anchor + N / D et le plan (p, q, r), w = (q - p) x (r - p) : meme predicat
// que geom::orient_center (D > 0). w exact (|w_i| <= 2 E^2 < 2^43 dans le palier servi, exact en double),
// cc = N + D (anchor - p) exact en i128 (centres q4 de MEB : 30 E^4 < 2^89) puis arrondi (erreur relative <= u = 2^-53).
// Chaque produit arrondi vaut w_i cc_i (1 + t), |t| <= gamma_2 ; la somme de trois termes ajoute gamma_2 :
// |s - v| <= gamma_4 / (1 - gamma_2) * S <= 5u S, ou S est la somme des |produits arrondis|. La borne calculee
// fl(S) * 2^-49 = 16u fl(S) > 5u S : si |s| la depasse, le signe est celui de v. Rend +1 ou -1 si le filtre tranche,
// 0 s'il ne conclut pas (l'appelant passe alors a l'orientation exacte) : en particulier quand v = 0 (centre dans le
// plan de la face), |s| <= 5u S < borne.
inline int semi_static(const geom::P3& p, const geom::P3& q, const geom::P3& r, const geom::P3& anchor,
                       const geom::Center& c) {
  const geom::P3 a = geom::sub(q, p), b = geom::sub(r, p);
  const i128 w[3] = {i128(a.y) * b.z - i128(a.z) * b.y, i128(a.z) * b.x - i128(a.x) * b.z,
                     i128(a.x) * b.y - i128(a.y) * b.x};
  const i128 cc[3] = {c.N[0] + c.D * (anchor.x - p.x), c.N[1] + c.D * (anchor.y - p.y),
                      c.N[2] + c.D * (anchor.z - p.z)};
  double sum = 0, mag = 0;
  for (int i = 0; i < 3; ++i) {
    const double t = static_cast<double>(w[i]) * static_cast<double>(cc[i]);
    sum += t;
    mag += std::fabs(t);
  }
  const double bound = mag * 0x1p-49;
  if (sum > bound) return 1;
  if (sum < -bound) return -1;
  return 0;
}

}  // namespace mhgp10::orient_filter
