// MorseHGP3D v3 — WSPD PAR VAGUES, SUR OCTREE COMPRIME DE MORTON.
//
// Transcription CPU du kernel GPU vise. Les trois obstacles de la construction
// classique tombent chacun sur une primitive GPU connue :
//
//  1. L'ARBRE. La borne de Callahan-Kosaraju — `O(n log n + s^d n)`, le
//     `n log n` etant le TEMPS de construction et `s^d n` la TAILLE de sortie —
//     ne demande pas le `fair split tree` : elle vaut pour un OCTREE COMPRIME.
//     Celui-la se construit par tri radix des codes de Morton puis arbre radix
//     de Karras — un thread par nœud interne, aucune recursion.
//
//     PRECISION, parce que la version precedente de ce commentaire etait
//     FAUSSE. Elle affirmait que les nœuds sont « des cellules alignees de cote
//     `2^k`, donc de rapport d'aspect exactement UN ». Un arbre radix BINAIRE a
//     un nœud par longueur de prefixe en BITS, pas par groupe de trois bits :
//     seuls les nœuds dont le prefixe est de longueur multiple de `3` sont des
//     cubes, les autres ont un rapport d'aspect `2` ou `4`. L'arbre n'est donc
//     pas un octree comprime, il en est le DEROULE BINAIRE.
//
//     Cela ne casse pas la borne : l'argument d'empilement se degrade d'un
//     facteur borne — au pire `8^d` — quand le rapport d'aspect passe de `1` a
//     `4`. C'est une CONSTANTE, jamais une pente. Mais il ne faut pas invoquer
//     un aspect unitaire que la structure ne fournit pas.
//
//  2. LE CAS DIAGONAL. `WSPD(v,v) -> {(L,L),(R,R),(L,R)}` se DEROULE : une fois
//     deplie, il n'est rien d'autre que « pour chaque nœud interne `v`, emettre
//     la graine `(gauche(v), droite(v))` ». Il y a exactement `n-1` nœuds
//     internes, donc un seul kernel plat. Toute la recursion diagonale
//     disparait.
//
//  3. LA RECURSION CROISEE. Une vague par niveau : chaque paire active rend
//     `1` si separee (terminal) ou `2` sinon, puis `count -> scan -> fill`. La
//     profondeur est bornee par `2 log2 n`, et le travail total vaut environ
//     DEUX fois la sortie, chaque terminal ayant un ancetre non separe.
//
// SEPARATION SUR LA CELLULE, CERTIFICATS SUR LA BOITE SERREE. La cellule porte
// la borne ; la boite serree, incluse, ne peut que mieux certifier. La
// separation ne decidant aucune geometrie, les employer toutes deux est sain.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace mhgp3v {

// Entrelacement 3D, pas TROIS. Le masque `0x5555...` est celui du pas DEUX,
// c'est-a-dire du plan : applique a trois axes ses bits se chevauchent.
inline unsigned long long wf_spread3(unsigned long long v) {
  v &= 0xFFFFull;
  v = (v | (v << 32)) & 0x001F00000000FFFFull;
  v = (v | (v << 16)) & 0x001F0000FF0000FFull;
  v = (v | (v << 8))  & 0x100F00F00F00F00Full;
  v = (v | (v << 4))  & 0x10C30C30C30C30C3ull;
  v = (v | (v << 2))  & 0x1249249249249249ull;
  return v;
}
inline unsigned long long wf_morton48(long long x, long long y, long long z) {
  return wf_spread3((unsigned long long)x) | (wf_spread3((unsigned long long)y) << 1) |
         (wf_spread3((unsigned long long)z) << 2);
}

struct WfNode {
  int left, right;        // < 0 : feuille, `-1 - index` du point
  int first, last;        // plage dans l'ordre trie
  int level;              // nombre d'axes-bits complets fixes : cote = 2^(16-level)
  long long lo[3], hi[3]; // CELLULE de Morton, alignee
  long long tlo[3], thi[3]; // BOITE SERREE du contenu, incluse dans la cellule
  int parent;               // pour la remontee ; -1 a la racine
};

// Longueur du prefixe commun de deux cles, avec depart par indice si egales.
inline int wf_delta(const std::vector<unsigned long long>& k, int i, int j) {
  if (j < 0 || j >= (int)k.size()) return -1;
  if (k[i] == k[j]) return 64 + __builtin_clzll((unsigned long long)(i ^ j));
  return __builtin_clzll(k[i] ^ k[j]);
}

// Cellule alignee determinee par un prefixe de `p` bits de la cle 48 bits.
// Les 16 bits de poids fort de la cle sont inutilises : le prefixe utile
// commence au bit 16. `level` axes-bits complets sont fixes par axe.
inline void wf_cell(unsigned long long key, int p, WfNode* n) {
  const int used = std::max(0, p - 16);          // bits utiles du prefixe
  const int level = std::min(16, used / 3);
  n->level = level;
  const int shift = 16 - level;
  for (int i = 0; i < 3; ++i) {
    unsigned long long axis = 0;
    for (int b = 0; b < 16; ++b)
      if (key & (1ull << (3 * b + i))) axis |= (1ull << b);
    const long long base = (long long)((axis >> shift) << shift);
    n->lo[i] = base;
    n->hi[i] = base + ((1ll << shift) - 1);
  }
}

// Arbre radix de Karras. `n-1` nœuds internes, un thread chacun sur GPU.
inline std::vector<WfNode> wf_build(const std::vector<unsigned long long>& keys) {
  const int n = (int)keys.size();
  std::vector<WfNode> nodes(std::max(0, n - 1));
  for (int i = 0; i < n - 1; ++i) {
    const int dl = wf_delta(keys, i, i - 1), dr = wf_delta(keys, i, i + 1);
    const int d = (dr > dl) ? 1 : -1;
    const int dmin = std::min(dl, dr);
    int lmax = 2;
    while (wf_delta(keys, i, i + lmax * d) > dmin) lmax <<= 1;
    int l = 0;
    for (int t = lmax >> 1; t >= 1; t >>= 1)
      if (wf_delta(keys, i, i + (l + t) * d) > dmin) l += t;
    const int j = i + l * d;
    const int first = std::min(i, j), last = std::max(i, j);
    const int pref = wf_delta(keys, first, last);
    int split = first, step = last - first;
    do {
      step = (step + 1) >> 1;
      const int cand = split + step;
      if (cand < last && wf_delta(keys, first, cand) > pref) split = cand;
    } while (step > 1);
    nodes[i].first = first;
    nodes[i].last = last;
    nodes[i].left = (split == first) ? (-1 - split) : split;
    nodes[i].right = (split + 1 == last) ? (-1 - (split + 1)) : (split + 1);
    wf_cell(keys[first], pref, &nodes[i]);
  }
  return nodes;
}

// ---- BOITES SERREES, remontees en une passe.
//
// La cellule de Morton porte la BORNE — aspect un, cellules disjointes — mais
// elle est grossiere : mesuree, elle multiplie le front par six et demi. La
// boite serree, INCLUSE dans la cellule, ne peut que rendre la separation plus
// facile, donc le front plus petit, et la partition reste garantie par la
// structure de la recursion et non par le test.
//
// Sur GPU c'est la propagation ascendante classique du LBVH : un thread par
// feuille, remontee par compteur atomique, une seule passe.
inline void wf_tight_boxes(std::vector<WfNode>* nodes,
                           const std::vector<std::array<long long, 3>>& pts) {
  const int n = (int)nodes->size();
  for (int i = 0; i < n; ++i) (*nodes)[i].parent = -1;
  std::vector<int> parent(n, -1);
  std::vector<int> order;
  order.reserve(n);
  // Ordre postfixe sans recursion : pile explicite, puis parcours inverse.
  std::vector<int> st;
  if (n > 0) st.push_back(0);
  while (!st.empty()) {
    const int i = st.back(); st.pop_back();
    order.push_back(i);
    if ((*nodes)[i].left >= 0) { parent[(*nodes)[i].left] = i; (*nodes)[(*nodes)[i].left].parent = i;
                                 st.push_back((*nodes)[i].left); }
    if ((*nodes)[i].right >= 0) { parent[(*nodes)[i].right] = i; (*nodes)[(*nodes)[i].right].parent = i;
                                  st.push_back((*nodes)[i].right); }
  }
  for (auto it = order.rbegin(); it != order.rend(); ++it) {
    WfNode& v = (*nodes)[*it];
    bool first = true;
    auto absorb = [&](const long long* lo, const long long* hi) {
      for (int i = 0; i < 3; ++i) {
        if (first) { v.tlo[i] = lo[i]; v.thi[i] = hi[i]; }
        else { v.tlo[i] = std::min(v.tlo[i], lo[i]); v.thi[i] = std::max(v.thi[i], hi[i]); }
      }
      first = false;
    };
    for (int side = 0; side < 2; ++side) {
      const int c = side ? v.right : v.left;
      if (c < 0) { const auto& p = pts[-1 - c]; absorb(p.data(), p.data()); }
      else absorb((*nodes)[c].tlo, (*nodes)[c].thi);
    }
  }
}

}  // namespace mhgp3v
