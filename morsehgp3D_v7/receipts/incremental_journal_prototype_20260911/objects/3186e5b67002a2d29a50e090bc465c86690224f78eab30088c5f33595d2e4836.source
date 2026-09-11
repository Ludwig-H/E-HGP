// MorseHGP3D v6 — cover d'arete partage, parametre par le coefficient.
//
// Tout sommet ou interieur d'une boule possedee par l'ancre (a,b) verifie
// |2z-(a+b)|² <= coef·D² : coef = 1 (q2, boule diametrale), 3 (q3 : lentille ;
// et sommets q4), 4 (q4 : interieurs et coquille — Jung, R <= √(3/8) D).
// Deux niveaux : `rect_cover_handles` (une traversee par rectangle vivant,
// boite des sommes S_AB, elagage dist(2Box(Z), S_AB)² > coef·Dmax², rend une
// antichaine de handles de <= 32 positions) puis `anchor_cover_from_handles`
// (filtre exact par ancre, 32 seaux radiaux stables). Le mutant
// `cover-rect-dmin` (borne par Dmin) est tue par la porte appariee.
#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "../core/device.hpp"
#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp7 {

struct CoverPoint {
  i32 u;
  i64 dist2q;  // |2z-(a+b)|²
};

// UNION FERMEE DES BOULES POSSIBLES D'UNE ANCRE MAXIMALE, filtre experimental
// opt-in. Le cover historique au coefficient 3 reste l'autorite de
// compatibilite ; cette enveloppe ne peut que le compacter. Le quantificateur
// est EXISTENTIEL (z appartient a au moins une boule possible) : un site garde
// n'est jamais, de ce seul fait, un temoin universel et ne credite aucun
// h0/ha/hb/hc. Les bornes supposent ab arete maximale ; ne pas les appliquer a
// une paire LCA ou a un role de bloc avant decision de l'owner. Avec
// d=b-a, w=2z-a-b,
// S=|w|²-D² et Xi=|d×w|² :
//   q3 : fermeture de l'union continue exacte, S<=0 ou 3S²<=4Xi ;
//   q4 : sur-ensemble de Jung, S<=0 ou S²<=2Xi.
// Les frontieres sont FERMEES : un point de coquille ne doit jamais etre
// perdu. Sous u16, S² et Xi exigent i128 (jusqu'a moins de 2^74 apres les
// petits facteurs). L'identite de Lagrange evite de former le produit
// vectoriel : Xi=D²|w|²-(d·w)².
enum class EdgeEnvelope : u8 { kNone, kQ3, kQ4Jung };

struct EdgeEnvelopeCounts {
  u64 sites_before = 0;  // sites du cover historique (coefficient deja applique)
  u64 sites_after = 0;   // sites conserves par l'enveloppe
  u64 cross_tests = 0;   // sites S>0 qui paient le test transverse
};

// Branche exterieure S>0, partagee avec la formation affine fusionnee. `xi`
// est Xi=|d x w|², forme en i128 avant tout carre.
MHGP7_HD inline bool edge_envelope_outer_contains(EdgeEnvelope envelope, i64 S, i128 xi) {
  if (envelope == EdgeEnvelope::kNone) return true;
  const i128 s2 = (i128)S * S;
  const bool factor_mutant = MHGP7_MUTANT("cover-envelope-factor");
  const i128 lhs = envelope == EdgeEnvelope::kQ3 ? 3 * s2 : s2;
  const i128 rhs = envelope == EdgeEnvelope::kQ3 ? (factor_mutant ? 3 : 4) * xi
                                                  : (factor_mutant ? 1 : 2) * xi;
  return MHGP7_MUTANT("cover-envelope-open") ? lhs < rhs : lhs <= rhs;
}

MHGP7_HD inline bool edge_envelope_contains(EdgeEnvelope envelope, const P3& pa, const P3& pb, const P3& pz,
                                            i64 D2, i64 dist2q) {
  if (envelope == EdgeEnvelope::kNone) return true;
  const i64 S = dist2q - D2;
  if (S <= 0) return true;
  const P3 d = p3_sub(pb, pa);
  const P3 w{2 * pz.x - pa.x - pb.x, 2 * pz.y - pa.y - pb.y, 2 * pz.z - pa.z - pb.z};
  const i64 dw = p3_dot(d, w);
  const i128 xi = (i128)D2 * dist2q - (i128)dw * dw;
  return edge_envelope_outer_contains(envelope, S, xi);
}

// `sorted = false` : resultat NON trie (pretests d'ancre, ordre indifferent —
// le tri par dist2q d'un resultat dense dominait le cout de la requete).
inline void cover_query(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2, i64 coef,
                        std::vector<CoverPoint>* out, bool sorted = true) {
  out->clear();
  if (ix.unique_count() == 0) return;
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = coef * D2;
  std::vector<NodeRef> stack{ix.root()};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = ix.box_of(z);
    i64 gap2 = 0;
    for (int i = 0; i < 3; ++i) {
      i64 d = 0;
      if (2 * bz.lo[i] > m2[i]) d = 2 * bz.lo[i] - m2[i];
      else if (2 * bz.hi[i] < m2[i]) d = m2[i] - 2 * bz.hi[i];
      gap2 += d * d;
    }
    if (gap2 > bound) continue;
    if (is_leaf(z)) {
      const i32 u = leaf_index(z);
      const P3& p = ix.upos[(size_t)u];
      const i64 t0 = 2 * p.x - m2[0], t1 = 2 * p.y - m2[1], t2 = 2 * p.z - m2[2];
      const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  if (!sorted) return;
  std::sort(out->begin(), out->end(), [](const CoverPoint& x, const CoverPoint& y) {
    return x.dist2q != y.dist2q ? x.dist2q < y.dist2q : x.u < y.u;
  });
}

// CANDIDATS DIAMETRAUX DU RECTANGLE (pretests d'ancre) : tous les points z tels
// que dist(2z, Box(A) + Box(B))² <= Dmax² — un sur-ensemble de la boule
// diametrale ouverte de CHAQUE ancre (a, b) du rectangle (|2z − (a+b)|² < D²
// avec a+b dans la boite des sommes et D <= Dmax). Une traversee par
// rectangle au lieu d'une requete par ancre ; les tests d'ancre filtrent
// ensuite exactement (q < 0 / |2w|² < D²). Resultat non trie.
inline void rect_diametral_candidates(const CloudIndex& ix, const AxisBox& A, const AxisBox& B, std::vector<CoverPoint>* out,
                                      u64* tree_nodes) {
  out->clear();
  if (ix.unique_count() == 0) return;
  i64 slo[3], shi[3], dmax2 = 0;
  for (int i = 0; i < 3; ++i) {
    slo[i] = A.lo[i] + B.lo[i];
    shi[i] = A.hi[i] + B.hi[i];
    const i64 e1 = A.hi[i] - B.lo[i];
    const i64 e2 = B.hi[i] - A.lo[i];
    const i64 w = std::max(std::llabs(e1), std::llabs(e2));
    dmax2 += w * w;
  }
  std::vector<NodeRef> stack{ix.root()};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    ++*tree_nodes;
    const AxisBox bz = ix.box_of(z);
    i64 gap2 = 0;
    for (int i = 0; i < 3; ++i) {
      i64 g = 0;
      if (2 * bz.lo[i] > shi[i]) g = 2 * bz.lo[i] - shi[i];
      else if (2 * bz.hi[i] < slo[i]) g = slo[i] - 2 * bz.hi[i];
      gap2 += g * g;
    }
    if (gap2 > dmax2) continue;
    if (is_leaf(z)) {
      const i32 u = leaf_index(z);
      const P3& p = ix.upos[(size_t)u];
      i64 d2 = 0;
      for (int i = 0; i < 3; ++i) {
        const i64 c = i == 0 ? p.x : (i == 1 ? p.y : p.z);
        i64 g = 0;
        if (2 * c > shi[i]) g = 2 * c - shi[i];
        else if (2 * c < slo[i]) g = slo[i] - 2 * c;
        d2 += g * g;
      }
      if (d2 <= dmax2) out->push_back(CoverPoint{u, d2});
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
}

inline void rect_cover_handles(const CloudIndex& ix, const AxisBox& A, const AxisBox& B, i64 coef,
                               std::vector<NodeRef>* out, u64* tree_nodes) {
  out->clear();
  if (ix.unique_count() == 0) return;
  i64 slo[3], shi[3], dmax2 = 0, dmin2 = 0;
  for (int i = 0; i < 3; ++i) {
    slo[i] = A.lo[i] + B.lo[i];
    shi[i] = A.hi[i] + B.hi[i];
    const i64 e1 = A.hi[i] - B.lo[i];
    const i64 e2 = B.hi[i] - A.lo[i];
    const i64 w = std::max(std::llabs(e1), std::llabs(e2));
    dmax2 += w * w;
    i64 lo = 0;
    if (B.lo[i] > A.hi[i]) lo = B.lo[i] - A.hi[i];
    else if (A.lo[i] > B.hi[i]) lo = A.lo[i] - B.hi[i];
    dmin2 += lo * lo;
  }
  const i64 bound = coef * (MHGP7_MUTANT("cover-rect-dmin") ? dmin2 : dmax2);
  std::vector<NodeRef> stack{ix.root()};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    ++*tree_nodes;
    const AxisBox bz = ix.box_of(z);
    i64 gap2 = 0;
    for (int i = 0; i < 3; ++i) {
      i64 g = 0;
      if (2 * bz.lo[i] > shi[i]) g = 2 * bz.lo[i] - shi[i];
      else if (2 * bz.hi[i] < slo[i]) g = slo[i] - 2 * bz.hi[i];
      gap2 += g * g;
    }
    if (gap2 > bound) continue;
    if (is_leaf(z)) {
      out->push_back(z);
      continue;
    }
    const NodeRange r = ix.range_of(z);
    if (r.last - r.first + 1 <= 32) {
      out->push_back(z);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
}

// `scratch` : tampon reutilise du counting sort (sans lui chaque ancre paie
// une allocation, poste visible du profil v4).
inline void anchor_cover_from_handles(const CloudIndex& ix, const std::vector<NodeRef>& handles, const P3& pa,
                                      const P3& pb, i64 D2, i64 coef, std::vector<CoverPoint>* out,
                                      u64* point_visits, std::vector<CoverPoint>* scratch) {
  out->clear();
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = coef * D2;
  for (const NodeRef h : handles) {
    const NodeRange r = ix.range_of(h);
    for (i32 u = r.first; u <= r.last; ++u) {
      ++*point_visits;
      const P3& p = ix.upos[(size_t)u];
      const i64 t0 = 2 * p.x - m2[0], t1 = 2 * p.y - m2[1], t2 = 2 * p.z - m2[2];
      const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
    }
  }
  constexpr int kBins = 32;
  u32 cnt[kBins + 1] = {};
  const auto bin_of = [&](i64 d2) { return (int)((i128)kBins * d2 / (bound + 1)); };
  for (const CoverPoint& cp : *out) ++cnt[bin_of(cp.dist2q) + 1];
  for (int b = 1; b <= kBins; ++b) cnt[b] += cnt[b - 1];
  scratch->resize(out->size());
  for (const CoverPoint& cp : *out) (*scratch)[cnt[bin_of(cp.dist2q)]++] = cp;
  out->swap(*scratch);
}

}  // namespace mhgp7

