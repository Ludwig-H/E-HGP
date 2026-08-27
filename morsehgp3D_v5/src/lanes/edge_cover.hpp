// MorseHGP3D v5 — cover d'arete partage, parametre par le coefficient.
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

#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp5 {

struct CoverPoint {
  i32 u;
  i64 dist2q;  // |2z-(a+b)|²
};

inline void cover_query(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2, i64 coef,
                        std::vector<CoverPoint>* out) {
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
  std::sort(out->begin(), out->end(), [](const CoverPoint& x, const CoverPoint& y) {
    return x.dist2q != y.dist2q ? x.dist2q < y.dist2q : x.u < y.u;
  });
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
  const i64 bound = coef * (MHGP5_MUTANT("cover-rect-dmin") ? dmin2 : dmax2);
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

}  // namespace mhgp5
