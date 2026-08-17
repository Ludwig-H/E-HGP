// MorseHGP3D v4 — WSPD PAR VAGUES SUR L'ARBRE RADIX, REPRISE DE ZERO.
//
// La decomposition de Callahan-Kosaraju produit O(s^3 n) rectangles `A x B`
// en dimension trois et partitionne EXACTEMENT toutes les paires non ordonnees
// de positions distinctes. Les deux defauts qui avaient fait sortir le front
// v3 de cette borne (arbitrage du 16 aout 2026) sont des interdits gravés ici :
//
//   1. TERMINAL DES QUE SEPARE. Aucun cap de population, de masse ou de
//      memoire n'entre dans le critere terminal — un cap est une propriete de
//      l'ordonnanceur AVAL (continuation), jamais une nouvelle definition du
//      rectangle. Un cap dans le critere force #rect >= C(n,2)/cap^2 :
//      quadratique par construction.
//   2. SCISSION DU FACTEUR DE PLUS GRAND DIAMETRE GEOMETRIQUE, jamais du plus
//      peuple : c'est l'invariant de l'argument d'empilement.
//
// SEPARATION ENTIERE, EUCLIDIENNE, SANS RACINE (reprise du predicat v3
// `wspd_separated_euclid`, seule piece du front v3 jugee saine) : avec
// `W2 = somme des carres des cotes` (carre du diametre de boite) et `D2` le
// carre de la distance des centres DOUBLES, la condition
// `d - rA - rB >= s max(rA, rB)` est IMPLIQUEE par
// `q^2 D2 >= (p+2q)^2 max(W2A, W2B)` pour `s = p/q`. Ce test peut manquer une
// separation (il majore `rA + rB` par `2 max`), jamais en inventer : un
// residu transmis ne casse rien, une separation inventee casserait la borne.
//
// SEPARATION SUR LA BOITE SERREE : la cellule de Morton porte la borne de
// packing, la boite serree — incluse — ne peut que rendre la separation plus
// facile, donc le front plus PETIT ; la partition des paires est garantie par
// la structure de la recursion, pas par le test (mesure v3 : la cellule seule
// multiplie le front par 6,5).
//
// FORME PAR VAGUES (transcription CPU du kernel vise) :
//   - graines : pour chaque nœud interne v, la paire (gauche(v), droite(v)) —
//     le cas diagonal deroule, un kernel plat de m-1 graines ;
//   - vague : chaque paire active rend 1 (terminal) ou 2 (scission) ;
//     count -> scan -> fill, profondeur bornee par ~2 log2 m.
//
// Le consommateur recoit chaque rectangle terminal par callback : c'est le
// POINT D'ANCRAGE du prefiltre de mort (h_coeur/h_a/h_b) qui, en aval,
// transformera « terminal » en « mort sans descente » — sans jamais modifier
// la partition ni la definition du rectangle.
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../tree/radix_tree.hpp"

namespace mhgp4 {

struct WspdRect {
  NodeRef a, b;
};

struct WspdStats {
  u64 rectangles = 0;
  u64 wave_peak = 0;    // plus grande vague (paires actives simultanees)
  u64 levels = 0;       // nombre de vagues
  u128 pair_mass = 0;   // somme des |A||B| ponderes par multiplicite
};

namespace detail {

struct NodeView {
  const i64* lo;
  const i64* hi;
};

inline NodeView node_view(const CloudIndex& ix, NodeRef v, i64 leaf_buf[3]) {
  if (v >= 0) return NodeView{ix.nodes[(size_t)v].tlo, ix.nodes[(size_t)v].thi};
  const P3& p = ix.upos[(size_t)(-1 - v)];
  leaf_buf[0] = p.x;
  leaf_buf[1] = p.y;
  leaf_buf[2] = p.z;
  return NodeView{leaf_buf, leaf_buf};
}

// Carre du diametre de la boite (somme des carres des cotes). i64 sous u16.
inline i64 box_w2(const NodeView& b) {
  i64 s = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 w = b.hi[i] - b.lo[i];
    s += w * w;
  }
  return s;
}

// `q^2 D2 >= (p+2q)^2 max(W2A, W2B)` — voir l'en-tete. Jamais de separation
// inventee ; i128 pour ne rien supposer des bornes de `p/q`.
inline bool separated(const NodeView& a, const NodeView& b, i64 p, i64 q) {
  i64 d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 u = (a.lo[i] + a.hi[i]) - (b.lo[i] + b.hi[i]);
    d2 += u * u;
  }
  const i64 r2 = std::max(box_w2(a), box_w2(b));
  const i64 k = p + 2 * q;
  // Elargir AVANT les carres : p/q viennent de la CLI, i64*i64 deborderait.
  return ((i128)q * q) * d2 >= ((i128)k * k) * r2;
}

inline u64 node_weight(const CloudIndex& ix, NodeRef v) {
  if (v >= 0) {
    const RadixNode& n = ix.nodes[(size_t)v];
    return ix.range_weight(n.first, n.last);
  }
  const size_t u = (size_t)(-1 - v);
  return ix.bucket_start[u + 1] - ix.bucket_start[u];
}

}  // namespace detail

// Deroule la WSPD complete ; `emit(rect)` recoit chaque rectangle terminal.
// `s = p/q` est la separation (les campagnes testent s = 6, 8, 10 entiers).
template <typename Emit>
inline WspdStats wspd_wavefront(const CloudIndex& ix, i64 s_num, i64 s_den, Emit&& emit) {
  WspdStats st;
  const size_t internal = ix.nodes.size();
  if (internal == 0) return st;  // 0 ou 1 position unique : aucune paire croisee

  std::vector<WspdRect> wave;
  wave.reserve(internal);
  for (size_t v = 0; v < internal; ++v)
    wave.push_back(WspdRect{ix.nodes[v].left, ix.nodes[v].right});

  std::vector<WspdRect> next;
  while (!wave.empty()) {
    ++st.levels;
    st.wave_peak = std::max(st.wave_peak, (u64)wave.size());
    next.clear();
    for (const WspdRect& r : wave) {
      i64 buf_a[3], buf_b[3];
      const detail::NodeView va = detail::node_view(ix, r.a, buf_a);
      const detail::NodeView vb = detail::node_view(ix, r.b, buf_b);
      if (detail::separated(va, vb, s_num, s_den)) {
        ++st.rectangles;
        st.pair_mass +=
            (u128)detail::node_weight(ix, r.a) * detail::node_weight(ix, r.b);
        emit(r);
        continue;
      }
      // Scission du facteur de plus grand diametre. Une feuille (diametre 0)
      // n'est jamais choisie : deux feuilles distinctes sont toujours separees
      // (d > 0, r = 0), donc au moins un facteur est interne ici.
      const i64 wa = detail::box_w2(va);
      const i64 wb = detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || wa >= wb);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      // L'ordre (enfant, garde) est fixe : il rend la liste des rectangles
      // deterministe sous permutation d'entree (l'arbre ne depend que des
      // cles de Morton, uniques par construction).
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
  return st;
}

// Masse de reference du ledger : paires de PointId de positions DISTINCTES,
// C(n,2) - somme_u C(mult_u, 2). La WSPD ne couvre jamais les paires
// co-positionnelles (D = 0) : elles ne sont pas des supports q2 propres et
// leur filtrage est exact, pas une perte.
inline u128 expected_pair_mass(const CloudIndex& ix) {
  const u128 n = ix.bucket_ids.size();
  u128 mass = n * (n - 1) / 2;
  for (size_t u = 0; u + 1 < ix.bucket_start.size(); ++u) {
    const u128 m = ix.bucket_start[u + 1] - ix.bucket_start[u];
    mass -= m * (m - 1) / 2;
  }
  return mass;
}

}  // namespace mhgp4
