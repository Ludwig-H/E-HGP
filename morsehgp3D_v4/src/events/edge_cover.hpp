// MorseHGP3D v4 — COVER D'ARETE PARTAGE, parametre par le coefficient.
//
// q3 (audit du 17 aout, § 6.2) : porteurs ET interieurs d'une circum-boule
// q3 possedee par (a,b) verifient |2z-(a+b)|² <= 3·D². q4 (audit bc5b05d
// § 3.4, preuve v4 par Jung 3D dans MATHEMATIQUES.md § 4.5) : la circum-
// boule q4 est la miniball du tetraedre (R <= sqrt(3/8)·D), d'ou
// |2z-(a+b)|² <= 4·D² pour tout interieur/coquille ; les SOMMETS restent
// dans la lentille (coefficient 3). La meme infrastructure sert les deux
// lanes, `coef` ∈ {3, 4}.
//
// Deux niveaux de partage :
//   - `rect_cover_handles` : UNE traversee haute par rectangle WSPD vivant
//     (boite des sommes S_AB, elagage dist(2Box(Z),S_AB)² > coef·Dmax²),
//     rendant une antichaine de handles ;
//   - `anchor_cover_from_handles` : filtre exact par ancre depuis les
//     handles, seaux radiaux stables (32 seaux, ordre non contractuel).
// `cover_query` : la variante une-requete-par-ancre (porte appariee).
#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "../tree/radix_tree.hpp"
#include "q2_witness_count.hpp"  // NodeRange, range_of
#include "spindle_q2.hpp"        // AxisBox, box_of_node

namespace mhgp4 {

struct CoverPoint {
  i32 u;
  i64 dist2q;  // |2z-(a+b)|²
};

inline void cover_query(const CloudIndex& ix, const P3& pa, const P3& pb,
                        i64 D2, i64 coef, std::vector<CoverPoint>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = coef * D2;
  const auto box_min_dist2q = [&](const AxisBox& b) {
    i64 s = 0;
    for (int i = 0; i < 3; ++i) {
      i64 d = 0;
      if (2 * b.lo[i] > m2[i]) d = 2 * b.lo[i] - m2[i];
      else if (2 * b.hi[i] < m2[i]) d = m2[i] - 2 * b.hi[i];
      s += d * d;
    }
    return s;
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (box_min_dist2q(bz) > bound) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      i64 d2 = 0;
      const i64 c[3] = {p.x, p.y, p.z};
      for (int i = 0; i < 3; ++i) {
        const i64 t = 2 * c[i] - m2[i];
        d2 += t * t;
      }
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  std::sort(out->begin(), out->end(),
            [](const CoverPoint& x, const CoverPoint& y) {
              return x.dist2q != y.dist2q ? x.dist2q < y.dist2q : x.u < y.u;
            });
}

// `mutant_dmin` : remplace Dmax par Dmin dans la borne du rectangle — la
// porte appariee qui le tue prouve que la borne majore bien l'ancre.
inline void rect_cover_handles(const CloudIndex& ix, const AxisBox& A,
                               const AxisBox& B, i64 coef, bool mutant_dmin,
                               std::vector<NodeRef>* out, u64* tree_nodes) {
  out->clear();
  if (ix.nodes.empty()) return;
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
  const i64 bound = coef * (mutant_dmin ? dmin2 : dmax2);
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    ++*tree_nodes;
    const AxisBox bz = box_of_node(ix, z);
    i64 gap2 = 0;
    for (int i = 0; i < 3; ++i) {
      i64 g = 0;
      if (2 * bz.lo[i] > shi[i]) g = 2 * bz.lo[i] - shi[i];
      else if (2 * bz.hi[i] < slo[i]) g = slo[i] - 2 * bz.hi[i];
      gap2 += g * g;
    }
    if (gap2 > bound) continue;
    if (z < 0) {
      out->push_back(z);
      continue;
    }
    const NodeRange r = range_of(ix, z);
    if (r.last - r.first + 1 <= 32) {
      out->push_back(z);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
}

inline void anchor_cover_from_handles(const CloudIndex& ix,
                                      const std::vector<NodeRef>& handles,
                                      const P3& pa, const P3& pb, i64 D2,
                                      i64 coef, std::vector<CoverPoint>* out,
                                      u64* point_visits) {
  out->clear();
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = coef * D2;
  for (const NodeRef h : handles) {
    const NodeRange r = range_of(ix, h);
    for (i32 u = r.first; u <= r.last; ++u) {
      ++*point_visits;
      const P3& p = ix.upos[(size_t)u];
      const i64 t0 = 2 * p.x - m2[0];
      const i64 t1 = 2 * p.y - m2[1];
      const i64 t2 = 2 * p.z - m2[2];
      const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
    }
  }
  // Counting sort en 32 seaux sur dist2q (stable, sans tri par ancre).
  constexpr int kBins = 32;
  u32 cnt[kBins + 1] = {};
  const auto bin_of = [&](i64 d2) {
    return (int)((i128)kBins * d2 / (bound + 1));
  };
  for (const CoverPoint& cp : *out) ++cnt[bin_of(cp.dist2q) + 1];
  for (int b = 1; b <= kBins; ++b) cnt[b] += cnt[b - 1];
  std::vector<CoverPoint> tmp(out->size());
  for (const CoverPoint& cp : *out) tmp[cnt[bin_of(cp.dist2q)]++] = cp;
  out->swap(tmp);
}

}  // namespace mhgp4
