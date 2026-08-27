// MorseHGP3D v5 — WSPD de Callahan-Kosaraju par vagues sur l'arbre radix.
//
// Produit O(s^3 n) rectangles `A x B` en dimension trois et partitionne
// EXACTEMENT les paires non ordonnees de positions distinctes. Deux interdits
// graves (post-mortem v3, arbitrage du 16 aout 2026) :
//   1. TERMINAL DES QUE SEPARE — aucun cap de population, de masse ou de
//      memoire dans le critere (un cap force #rect >= C(n,2)/cap², quadratique) ;
//   2. SCISSION DU FACTEUR DE PLUS GRAND DIAMETRE GEOMETRIQUE, jamais du plus
//      peuple (invariant de l'argument d'empilement).
// Les deux interdits ont leur mutant (`wspd-cap-terminal`, `wspd-split-heaviest`).
//
// Separation entiere sans racine : avec W2 = somme des carres des cotes de
// boite et D2 = carre de la distance des centres DOUBLES,
// `q² D2 >= (p+2q)² max(W2A, W2B)` implique `d - rA - rB >= s max(rA, rB)`
// pour s = p/q. Le test peut manquer une separation (front plus gros), jamais
// en inventer.
//
// Forme par vagues (transcription CPU du kernel vise) : graines = les paires
// (gauche(v), droite(v)) de chaque nœud interne ; chaque paire active rend 1
// (terminal) ou 2 (scission) ; profondeur ~2 log2 m.
#pragma once

#include <algorithm>
#include <vector>

#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp5 {

struct WspdRect {
  NodeRef a, b;
};

struct WspdStats {
  u64 rectangles = 0;
  u64 wave_peak = 0;
  u64 levels = 0;
  u128 pair_mass = 0;  // somme des |A||B| ponderes par multiplicite
};

namespace wspd_detail {

// Carre du diametre de la boite (somme des carres des cotes). i64 sous u16.
inline i64 box_w2(const AxisBox& b) {
  i64 s = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 w = b.hi[i] - b.lo[i];
    s += w * w;
  }
  return s;
}

inline bool separated(const AxisBox& a, const AxisBox& b, i64 p, i64 q) {
  i64 d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 u = (a.lo[i] + a.hi[i]) - (b.lo[i] + b.hi[i]);
    d2 += u * u;
  }
  const i64 r2 = std::max(box_w2(a), box_w2(b));
  const i64 k = p + 2 * q;
  return ((i128)q * q) * d2 >= ((i128)k * k) * r2;
}

}  // namespace wspd_detail

// Deroule la WSPD complete ; `emit(rect)` recoit chaque rectangle terminal.
// `s = s_num / s_den` (les campagnes testent s = 6, 8, 10 entiers).
template <typename Emit>
inline WspdStats wspd_wavefront(const CloudIndex& ix, i64 s_num, i64 s_den, Emit&& emit) {
  WspdStats st;
  const size_t internal = ix.nodes.size();
  if (internal == 0) return st;
  const bool mut_drop = MHGP5_MUTANT("wspd-drop-rect");
  const bool mut_cap = MHGP5_MUTANT("wspd-cap-terminal");
  const bool mut_heaviest = MHGP5_MUTANT("wspd-split-heaviest");
  bool drop_pending = mut_drop;

  std::vector<WspdRect> wave;
  wave.reserve(internal);
  for (size_t v = 0; v < internal; ++v) wave.push_back(WspdRect{ix.nodes[v].left, ix.nodes[v].right});

  std::vector<WspdRect> next;
  while (!wave.empty()) {
    ++st.levels;
    st.wave_peak = std::max(st.wave_peak, (u64)wave.size());
    next.clear();
    for (const WspdRect& r : wave) {
      const AxisBox va = ix.box_of(r.a);
      const AxisBox vb = ix.box_of(r.b);
      const u64 wa = ix.node_weight(r.a), wb = ix.node_weight(r.b);
      bool terminal = wspd_detail::separated(va, vb, s_num, s_den);
      // MUTANT : le cap v3 dans le critere terminal (pavage quadratique).
      if (terminal && mut_cap && wa * wb > 512 && (r.a >= 0 || r.b >= 0)) terminal = false;
      if (terminal) {
        if (drop_pending && wa * wb >= 4) {  // MUTANT : un rectangle perdu
          drop_pending = false;
          continue;
        }
        ++st.rectangles;
        st.pair_mass += (u128)wa * wb;
        emit(r);
        continue;
      }
      // Scission du facteur de plus grand diametre (jamais une feuille : deux
      // feuilles distinctes sont toujours separees).
      const i64 dwa = wspd_detail::box_w2(va);
      const i64 dwb = wspd_detail::box_w2(vb);
      bool split_a = (r.a >= 0) && (r.b < 0 || dwa >= dwb);
      if (mut_heaviest) split_a = (r.a >= 0) && (r.b < 0 || wa >= wb);  // MUTANT
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      // L'ordre (enfant, garde) est fixe : liste deterministe sous permutation.
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
  return st;
}

// Masse de reference du ledger : paires de PointId de positions DISTINCTES,
// C(n,2) - somme_u C(mult_u, 2). Les paires co-positionnelles (D = 0) ne sont
// pas des supports q2 : leur filtrage est exact, pas une perte.
inline u128 expected_pair_mass(const CloudIndex& ix) {
  const u128 n = ix.bucket_ids.size();
  u128 mass = n * (n - 1) / 2;
  for (size_t u = 0; u + 1 < ix.bucket_start.size(); ++u) {
    const u128 m = ix.bucket_start[u + 1] - ix.bucket_start[u];
    mass -= m * (m - 1) / 2;
  }
  return mass;
}

}  // namespace mhgp5
