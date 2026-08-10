// MorseHGP3D v3 — LE FOLD SATURE : les K forets par le graphe d'intersections.
//
// C'est le remplacant candidat de `mhgp::build_forest`, qui CENSURE le regime
// multiplicitaire (40/60 ordres declares non autoritatifs sur grille 4^3,
// mesure du juge Gamma). La semantique est celle des theoremes S.1--S.5 de
// `docs/math/TOUR_BOULES_SATUREES.md` et de la reponse complementaire de
// l'auditeur :
//
//   - un GENERATEUR est un sature S = X inter B (la liste `members` d'une
//     sphere critique du catalogue), au niveau exact de sa miniboule ;
//   - a l'ordre k, Gamma_k(a) = union des graphes de Johnson J_k(S) des
//     generateurs actifs de taille >= k (S.4) ; le graphe H_k — un sommet par
//     generateur actif, une arete ssi |S inter T| >= k — a les MEMES
//     composantes, et la couverture d'une composante est l'union de ses
//     satures ;
//   - le protocole de lot est celui de la reponse Q1.2 : coupe stricte figee,
//     TOUTES les activations d'un meme niveau exact appliquees, composantes du
//     lot entier, classification par racines strictes distinctes (0 naissance,
//     1 continuation, >= 2 multifusion).
//
// LA FAMILLE D'ENTREE DOIT ETRE COMPLETE pour que la sortie soit exacte :
// s_max >= n donne la famille saturee entiere (S.2 + la porte d'exhaustivite
// de `flat_catalogue`). Sous famille tronquee, S.6 ne garantit qu'un
// RAFFINEMENT (aucune fausse connexion, des fusions possiblement manquantes) —
// le consommateur doit le declarer.
//
// CE FICHIER EST DU CODE PRODUIT CANDIDAT, il est juge par l'oracle Gamma
// (`mhgp3v_gamma_judge --subject fold`) : partitions de couverture aux coupes
// stricte et fermee, sur l'union des niveaux. Il n'emploie AUCUNE arithmetique
// de l'oracle : les niveaux se comparent par `mhgp::sphere_cmp_beta`, exact sur
// la representation entiere des spheres.
//
// COMPLEXITE, dite honnetement : la jointure des generateurs est ici le
// balayage naif O(G^2) par lot avec intersection de listes triees et sortie
// precoce au seuil k. C'est la forme de VERITE, pas la forme d'echelle ; la
// forme d'echelle (jointure par tri sur les paires (point, generateur), foret
// de Kruskal S.5 en poids decroissants, lots transactionnels) doit etre jugee
// contre celle-ci avant de la remplacer.
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

// Une partition de couverture : collection triee d'ensembles tries.
using FoldPartition = std::vector<std::vector<mhgp::i32>>;

struct SaturatedOrderFold {
  // Niveaux d'evenement croissants (indices dans le catalogue trie par niveau,
  // un representant par classe d'egalite exacte), et partition de couverture a
  // la coupe FERMEE de chaque niveau. La coupe stricte d'un niveau est la
  // fermee du precedent. Les partitions ne sont MATERIALISEES que sur demande
  // (juge) : les stocker a chaque lot coutait O(niveaux * G * |S|) et tuait le
  // pipeline des n=200.
  std::vector<int> level_representative;   // indice catalogue du niveau
  std::vector<FoldPartition> closed_partitions;   // vide si keep_partitions=false
  // LE TRANSCRIPT SEPARE CE QUE L'AUDIT LIVE A SEPARE : un generateur qui
  // s'active sans changer ni composante ni couverture est un LOT SILENCIEUX
  // (etat interne persistant, jamais une continuation Gamma) ; une croissance
  // de couverture sans fusion est une CROISSANCE, pas une continuation.
  long long births = 0, fusions = 0;
  long long coverage_growth_batches = 0, silent_generator_batches = 0;
  // Masses de la jointure, publiees pour mesurer le mur avant de l'optimiser.
  long long join_comparisons = 0, join_unions = 0;
};

struct SaturatedFold {
  int maximum_order = 0;
  std::vector<SaturatedOrderFold> orders;   // indexe par k-1
  bool ok = false;
  const char* refusal = "";
};

// L'intersection de deux listes triees atteint-elle k ? Sortie precoce : le
// cout est min(|a|,|b|) au pire, et bien moins des que k est atteint.
inline bool sorted_intersection_reaches(const std::vector<mhgp::i32>& a,
                                        const std::vector<mhgp::i32>& b, int k) {
  if (k <= 0) return true;
  std::size_t i = 0, j = 0;
  int count = 0;
  while (i < a.size() && j < b.size()) {
    if (a[i] < b[j]) ++i;
    else if (b[j] < a[i]) ++j;
    else {
      if (++count >= k) return true;
      ++i;
      ++j;
    }
  }
  return false;
}

inline SaturatedFold build_saturated_fold(const mhgp::Catalogue& catalogue, int maximum_order,
                                          bool keep_partitions = true) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  const std::size_t count = catalogue.spheres.size();

  // Les membres de chaque generateur, tries — le pool est deja trie par
  // sphere, mais l'invariant est VERIFIE, pas suppose : le fold est un
  // consommateur fail-closed du catalogue.
  std::vector<std::vector<mhgp::i32>> members(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    if (sphere.members_begin < 0 || sphere.rank < 0 ||
        (std::size_t)sphere.members_begin + (std::size_t)sphere.rank >
            catalogue.members.size()) {
      fold.refusal = "tranche de pool hors catalogue";
      return fold;
    }
    members[s].assign(catalogue.members.begin() + sphere.members_begin,
                      catalogue.members.begin() + sphere.members_begin + sphere.rank);
    for (std::size_t t = 1; t < members[s].size(); ++t)
      if (members[s][t - 1] >= members[s][t]) {
        fold.refusal = "membres non tries ou dupliques";
        return fold;
      }
  }

  // Ordre des generateurs par NIVEAU EXACT croissant (sphere_cmp_beta), avec
  // l'indice catalogue en clef secondaire deterministe. Les classes d'egalite
  // exacte forment les lots.
  std::vector<int> by_level((std::size_t)count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });

  fold.orders.resize((std::size_t)maximum_order);
  for (int k = 1; k <= maximum_order; ++k) {
    SaturatedOrderFold& order = fold.orders[(std::size_t)(k - 1)];
    // DSU sur les generateurs retenus (|S| >= k).
    std::vector<int> parent(count);
    for (std::size_t s = 0; s < count; ++s) parent[s] = (int)s;
    const auto find = [&](int a) {
      while (parent[(std::size_t)a] != a) {
        parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
        a = parent[(std::size_t)a];
      }
      return a;
    };
    std::vector<char> active(count, 0);
    std::vector<long long> node_of_root(count, -1);
    long long next_node = 0;
    std::vector<int> active_list;
    // Couverture par racine, entretenue par fusion du petit dans le grand.
    std::vector<std::set<mhgp::i32>> coverage(count);

    std::size_t cursor = 0;
    while (cursor < count) {
      // Le LOT : toutes les spheres du meme niveau exact.
      std::size_t batch_end = cursor + 1;
      while (batch_end < count &&
             mhgp::sphere_cmp_beta(
                 catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                 catalogue.spheres[(std::size_t)by_level[batch_end]].sph) == 0)
        ++batch_end;

      // 1. FIGER la coupe stricte : le noeud de chaque generateur actif, et la
      // taille de couverture de chaque composante stricte (pour separer
      // croissance et lot silencieux).
      std::vector<std::pair<int, long long>> strict_nodes;   // (generateur, noeud)
      std::map<long long, std::size_t> strict_coverage_size;
      for (int s : active_list) {
        const int root = find(s);
        strict_nodes.push_back({s, node_of_root[(std::size_t)root]});
        if (node_of_root[(std::size_t)root] >= 0)
          strict_coverage_size[node_of_root[(std::size_t)root]] =
              coverage[(std::size_t)root].size();
      }

      // 2. ACTIVER le lot entier (les generateurs de taille >= k seulement),
      // puis 3. joindre chaque nouveau generateur a TOUS les actifs par le
      // seuil |S inter T| >= k — y compris entre nouveaux du meme lot.
      bool batch_touched = false;
      for (std::size_t b = cursor; b < batch_end; ++b) {
        const int s = by_level[b];
        if ((int)members[(std::size_t)s].size() < k) continue;
        active[(std::size_t)s] = 1;
        coverage[(std::size_t)s].insert(members[(std::size_t)s].begin(),
                                        members[(std::size_t)s].end());
        for (int t : active_list) {
          ++order.join_comparisons;
          if (sorted_intersection_reaches(members[(std::size_t)s], members[(std::size_t)t], k)) {
            int rs = find(s), rt = find(t);
            if (rs != rt) {
              ++order.join_unions;
              // Fusion du PETIT dans le GRAND : la racine survivante garde la
              // grande couverture, l'autre est versee puis liberee.
              if (coverage[(std::size_t)rs].size() > coverage[(std::size_t)rt].size())
                std::swap(rs, rt);
              coverage[(std::size_t)rt].insert(coverage[(std::size_t)rs].begin(),
                                               coverage[(std::size_t)rs].end());
              std::set<mhgp::i32>().swap(coverage[(std::size_t)rs]);
              const long long keep_node = node_of_root[(std::size_t)rt] >= 0
                                              ? node_of_root[(std::size_t)rt]
                                              : node_of_root[(std::size_t)rs];
              parent[(std::size_t)rs] = rt;
              node_of_root[(std::size_t)rt] = keep_node;
            }
          }
        }
        active_list.push_back(s);
        batch_touched = true;
      }
      if (!batch_touched) { cursor = batch_end; continue; }

      // 4. CLASSIFIER par racines strictes distinctes, par composante finale.
      std::vector<std::pair<int, int>> root_of;   // (racine finale, generateur)
      for (int s : active_list) root_of.push_back({find(s), s});
      std::sort(root_of.begin(), root_of.end());
      std::map<int, std::set<long long>> strict_roots_of;
      for (const auto& entry : strict_nodes) {
        if (entry.second >= 0)
          strict_roots_of[find(entry.first)].insert(entry.second);
      }
      // Seules les composantes TOUCHEES par le lot changent d'etat.
      std::set<int> touched_roots;
      for (std::size_t b = cursor; b < batch_end; ++b) {
        const int s = by_level[b];
        if (active[(std::size_t)s]) touched_roots.insert(find(s));
      }
      for (int root : touched_roots) {
        const auto it = strict_roots_of.find(root);
        const std::size_t strict = it == strict_roots_of.end() ? 0 : it->second.size();
        if (strict == 0) {
          ++order.births;
          node_of_root[(std::size_t)root] = next_node++;
        } else if (strict == 1) {
          const long long node = *it->second.begin();
          const auto before = strict_coverage_size.find(node);
          const bool grew = before == strict_coverage_size.end() ||
                            coverage[(std::size_t)root].size() > before->second;
          if (grew) ++order.coverage_growth_batches;
          else ++order.silent_generator_batches;
          node_of_root[(std::size_t)root] = node;
        } else {
          ++order.fusions;
          node_of_root[(std::size_t)root] = next_node++;
        }
      }

      // 5. LA COUPE FERMEE : couverture par composante = union des satures
      // (S.4), lue dans les couvertures INCREMENTALES. Le lot entier est
      // committe d'un coup — jamais boule par boule. La materialisation par
      // niveau n'existe que pour le juge.
      order.level_representative.push_back(by_level[cursor]);
      if (keep_partitions) {
        FoldPartition partition;
        std::set<int> roots_seen;
        for (const auto& entry : root_of)
          if (roots_seen.insert(entry.first).second)
            partition.push_back(std::vector<mhgp::i32>(
                coverage[(std::size_t)entry.first].begin(),
                coverage[(std::size_t)entry.first].end()));
        std::sort(partition.begin(), partition.end());
        order.closed_partitions.push_back(std::move(partition));
      }
      cursor = batch_end;
    }
  }
  fold.ok = true;
  return fold;
}

}  // namespace mhgp3v
