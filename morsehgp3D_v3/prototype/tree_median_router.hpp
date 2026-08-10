// MorseHGP3D v3 — LE ROUTEUR MEDIAN D'ARBRE (note dendrogramme, §5).
//
// Pour un point x muni d'une mesure de masse conservee sur les noeuds de
// l'arbre maitre M, le terminal t(x) minimise la somme ponderee des distances
// d'arbre. THEOREME 5.1 : t est optimal ssi chaque composante de M \ {t}
// porte une masse <= 1/2. L'algorithme : partir de la racine ; tant qu'un
// UNIQUE enfant porte STRICTEMENT plus de la moitie de la masse, y descendre ;
// sinon s'arreter. L'egalite 1/2--1/2 forme un segment de medianes et le
// contrat prend le noeud LE PLUS HAUT (stay) — la descente stricte s'y arrete
// d'elle-meme. Le terminal peut etre un noeud de masse directe NULLE : c'est
// une ambiguite reelle entre branches, le stay structurel est AUTORISE.
//
// Les masses sont des rationnels EXACTS a denominateur commun par point :
// comparer une masse m a 1/2 est l'entier 2*m <=> total — aucune division.
// Les mutants du chemin (glouton renomme, seuil >=, stay interdit, tie-break
// d'allocation) vivent ici pour que la porte prouve qu'elle mord.
#pragma once

#include <cstdint>
#include <vector>

namespace mhgp3v {

struct MasterTree {
  std::vector<int> parent;                  // -1 pour la racine
  std::vector<std::vector<int>> children;   // derivee de parent
  int root = 0;
};

inline MasterTree make_master_tree(const std::vector<int>& parent) {
  MasterTree tree;
  tree.parent = parent;
  tree.children.assign(parent.size(), {});
  for (std::size_t v = 0; v < parent.size(); ++v) {
    if (parent[v] < 0) tree.root = (int)v;
    else tree.children[(std::size_t)parent[v]].push_back((int)v);
  }
  return tree;
}

// La mesure d'un point : masses entieres sur les noeuds, somme == total.
struct PointMeasure {
  long long total = 0;
  std::vector<std::pair<int, long long>> mass;   // (noeud, masse > 0)
};

struct MedianMutants {
  bool greedy_plurality = false;    // descendre au max de masse SANS majorite
  bool half_inclusive = false;      // seuil >= 1/2 au lieu de > 1/2
  bool forbid_zero_stay = false;    // interdire le stay de masse directe nulle
  bool descend_on_tie = false;      // a 1/2--1/2, descendre au lieu de rester
};

// Le terminal median d'un point. Cout O(H) par appel (masses de sous-arbres
// par parcours postfixe) — la forme O(d_x log d_x) par arbre virtuel viendra
// avec le flux ; l'oracle exige d'abord l'EXACTITUDE.
inline int route_median(const MasterTree& tree, const PointMeasure& measure,
                        MedianMutants mutants = {}) {
  const std::size_t count = tree.parent.size();
  std::vector<long long> subtree(count, 0);
  std::vector<long long> direct(count, 0);
  for (const std::pair<int, long long>& entry : measure.mass)
    direct[(std::size_t)entry.first] += entry.second;
  // Somme de sous-arbre par ordre postfixe (pile explicite, arbre quelconque).
  {
    std::vector<int> order;
    std::vector<int> stack = {tree.root};
    while (!stack.empty()) {
      const int node = stack.back();
      stack.pop_back();
      order.push_back(node);
      for (int child : tree.children[(std::size_t)node]) stack.push_back(child);
    }
    for (std::size_t i = order.size(); i-- > 0;) {
      const int node = order[i];
      subtree[(std::size_t)node] = direct[(std::size_t)node];
      for (int child : tree.children[(std::size_t)node])
        subtree[(std::size_t)node] += subtree[(std::size_t)child];
    }
  }
  int node = tree.root;
  while (true) {
    int majority_child = -1;
    long long best_mass = -1;
    for (int child : tree.children[(std::size_t)node]) {
      const long long child_mass = subtree[(std::size_t)child];
      if (child_mass > best_mass) {
        best_mass = child_mass;
        majority_child = child;
      }
    }
    if (majority_child < 0) break;   // feuille
    bool descend;
    if (mutants.greedy_plurality) {
      // MUTANT : la pluralite du glouton — descendre tant qu'un enfant porte
      // plus que la masse directe restante, sans exiger la majorite absolue.
      descend = best_mass > 0 && best_mass >= subtree[(std::size_t)node] - best_mass;
      if (best_mass == 0) descend = false;
    } else if (mutants.half_inclusive) {
      descend = 2 * best_mass >= measure.total;   // MUTANT : seuil inclusif
    } else {
      descend = 2 * best_mass > measure.total;    // CONTRAT : STRICTEMENT
    }
    if (mutants.descend_on_tie && !descend && 2 * best_mass == measure.total)
      descend = true;   // MUTANT : l'egalite 1/2--1/2 descend au lieu de rester
    if (!descend) break;
    node = majority_child;
  }
  if (mutants.forbid_zero_stay && direct[(std::size_t)node] == 0 &&
      !tree.children[(std::size_t)node].empty()) {
    // MUTANT : le stay structurel interdit — pousser au plus gros enfant.
    int biggest = tree.children[(std::size_t)node].front();
    for (int child : tree.children[(std::size_t)node])
      if (subtree[(std::size_t)child] > subtree[(std::size_t)biggest]) biggest = child;
    node = biggest;
  }
  return node;
}

// L'ORACLE PAR ENUMERATION : la perte L(t) = somme_v mu(v) * d(t, v) aux
// longueurs unite, pour TOUS les terminaux — le minimum et son noeud LE PLUS
// HAUT (parmi les minima, celui dont aucun ancetre n'est aussi minimal).
inline long long enumeration_loss(const MasterTree& tree, const PointMeasure& measure,
                                  int terminal) {
  const std::size_t count = tree.parent.size();
  std::vector<int> distance(count, -1);
  std::vector<int> queue = {terminal};
  distance[(std::size_t)terminal] = 0;
  for (std::size_t head = 0; head < queue.size(); ++head) {
    const int node = queue[head];
    std::vector<int> around = tree.children[(std::size_t)node];
    if (tree.parent[(std::size_t)node] >= 0)
      around.push_back(tree.parent[(std::size_t)node]);
    for (int next : around)
      if (distance[(std::size_t)next] < 0) {
        distance[(std::size_t)next] = distance[(std::size_t)node] + 1;
        queue.push_back(next);
      }
  }
  long long loss = 0;
  for (const std::pair<int, long long>& entry : measure.mass)
    loss += entry.second * (long long)distance[(std::size_t)entry.first];
  return loss;
}

}  // namespace mhgp3v
