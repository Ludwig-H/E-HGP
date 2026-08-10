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
  // LE TRANSCRIPT EST ENCORE UNE CLASSIFICATION INTERNE, PAS LE TRANSCRIPT
  // GAMMA : la categorie « lot silencieux » MELANGE les continuations Gamma
  // sans croissance de couverture et les activations redondantes (une
  // continuation peut ne changer ni partition ni couverture — refutation de
  // l'audit live). La separation exacte est le predicat structurel
  // |M| >= k et q_min(B) <= k+1 (NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN,
  // theoremes 1--2), a recevoir contre l'oracle Gamma avant d'etre publie.
  long long births = 0, fusions = 0;
  long long coverage_growth_batches = 0, silent_generator_batches = 0;
  // LE TRANSCRIPT GAMMA PAR MARQUAGE (theoremes 1--2 de la note q_min, recus
  // par le juge) : seules les racines finales atteintes par un GENERATEUR
  // D'EVENEMENT (|M| >= k et q_min <= k+1, q_min lu dans n_support — une
  // provenance CERTIFIEE par le juge sur ce chemin) produisent un evenement
  // public, classe par le nombre de racines strictes distinctes : 0 naissance,
  // 1 continuation (meme sans croissance de couverture), >= 2 multifusion.
  // Ce sont des evenements PAR COMPOSANTE, pas par lot. Sous famille tronquee
  // ils ne sont que relative_to_certified_subfamily.
  long long gamma_births = 0, gamma_continuations = 0, gamma_multifusions = 0;
  // GARDE FAIL-CLOSED du theoreme : une racine marquee sans racine stricte
  // doit contenir un generateur d'evenement avec q_min <= k ; sinon la source
  // est incomplete, q_min faux, le join incomplet ou le lot non atomique. Sous
  // famille complete la violation REFUSE le fold ; sinon elle est comptee.
  long long event_guard_violations = 0;
  // Les evenements Gamma par niveau, alignes sur level_representative — le
  // payload que le juge compare a la verite exhaustive niveau par niveau.
  std::vector<long long> gamma_birth_at_level, gamma_continuation_at_level,
      gamma_multifusion_at_level;
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
                                          bool keep_partitions = true,
                                          bool enforce_event_guard = false) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank) {
    fold.refusal = "ordre maximal hors contrat";
    return fold;
  }
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

      // 4bis. LE TRANSCRIPT GAMMA PAR MARQUAGE : seules les racines finales
      // atteintes par un generateur d'evenement (|M| >= k deja garanti par
      // l'activation, n_support <= k+1) produisent un evenement public.
      std::map<int, int> marked_minimum_support;
      for (std::size_t b = cursor; b < batch_end; ++b) {
        const int s = by_level[b];
        if (!active[(std::size_t)s]) continue;
        const int support_size = (int)catalogue.spheres[(std::size_t)s].n_support;
        if (support_size > k + 1) continue;
        const int root = find(s);
        const auto it = marked_minimum_support.find(root);
        if (it == marked_minimum_support.end())
          marked_minimum_support.emplace(root, support_size);
        else
          it->second = std::min(it->second, support_size);
      }
      long long births_here = 0, continuations_here = 0, multifusions_here = 0;
      for (const auto& entry : marked_minimum_support) {
        const auto it = strict_roots_of.find(entry.first);
        const std::size_t strict = it == strict_roots_of.end() ? 0 : it->second.size();
        if (strict == 0) {
          ++births_here;
          // GARDE DU THEOREME : une naissance marquee exige un support q <= k.
          if (entry.second > k) {
            ++order.event_guard_violations;
            if (enforce_event_guard) {
              fold.refusal = "garde d'evenement violee : naissance marquee sans support q<=k";
              return fold;
            }
          }
        } else if (strict == 1) {
          ++continuations_here;
        } else {
          ++multifusions_here;
        }
      }
      order.gamma_births += births_here;
      order.gamma_continuations += continuations_here;
      order.gamma_multifusions += multifusions_here;

      // 5. LA COUPE FERMEE : couverture par composante = union des satures
      // (S.4), lue dans les couvertures INCREMENTALES. Le lot entier est
      // committe d'un coup — jamais boule par boule. La materialisation par
      // niveau n'existe que pour le juge.
      order.level_representative.push_back(by_level[cursor]);
      order.gamma_birth_at_level.push_back(births_here);
      order.gamma_continuation_at_level.push_back(continuations_here);
      order.gamma_multifusion_at_level.push_back(multifusions_here);
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

// ---------------------------------------------------------------------------
// LE JOIN PAR POSTINGS — la forme d'echelle, differenciee contre la verite.
//
// Theoreme de reduction (note de l'auditeur, §1) : pour chaque point x, soit
// P_x la liste des generateurs qui le contiennent. Emettre une occurrence par
// paire non ordonnee dans chaque P_x puis reduire donne exactement
// w(M,N) = |M inter N| ; l'arete d'ordre k existe ssi w >= k, active au niveau
// du PLUS TARDIF des deux. Identites de recu par lot :
//   R_old_new = somme_{M in B} somme_{x in M} |P_x^-|
//   R_new_new = somme_x C(|B_x|, 2)
//   R_old_new + R_new_new = somme des poids reduits du lot.
// Et sur la famille entiere : P_post = somme_x C(d_x, 2) = somme des poids.
// Toute violation refuse le fold ENTIER — jamais un lot partiel.
//
// LE SCAN GLOBAL PAR LOT EST SUPPRIME (§4 de la note) : la coupe stricte —
// noeud public ET taille de couverture pre-lot — est capturee AU PREMIER
// CONTACT de chaque racine dans le lot (epoque), la classification ne parcourt
// que les racines touchees, et les racines non touchees ne sont ni parcourues
// ni triees. Seule la materialisation des partitions (juge, petits n) parcourt
// les racines vivantes.
// ---------------------------------------------------------------------------
struct PostingsReceipt {
  long long old_new_occurrences = 0, new_new_occurrences = 0;
  long long reduced_pairs = 0;    // paires uniques ponderees
  long long weight_sum = 0;       // somme des w reduits
  long long p_post = 0;           // somme_x C(d_x, 2), postings finales
  long long unions_attempted = 0, unions_done = 0;
  long long postings_mass = 0;    // somme des longueurs |P_x|
  std::size_t max_posting = 0;
  bool identities_ok = false;
  // LE PREFLIGHT (audit 621ee80 §6) : calcule AVANT toute emission, en
  // arithmetique verifiee — P_post predit depuis les degres finals (identite
  // post-hoc avec p_post, independante des lots), masse du plus gros lot et
  // pic memoire CONSERVATEUR. Le refus de budget arrive avant l'allocation.
  long long predicted_p_post = 0;
  long long max_batch_occurrences = 0;
  long long predicted_peak_bytes = 0;
  // La table complete (M, N) -> w, remplie seulement sur demande
  // (collect_pairs) : l'oracle independant de la porte la compare clef par
  // clef et poids par poids — une redistribution compensee des poids qui
  // preserverait masses, unions et partitions ne peut pas la tromper.
  std::vector<std::pair<std::pair<int, int>, long long>> pairs;
};

// LES SIX MUTANTS NOMMES PAR LA NOTE (§6) : chacun doit rougir par la porte
// differentielle, les fixtures nommees ou les identites de recu — jamais
// rester vert. Ils vivent DANS le code produit pour que la porte prouve
// qu'elle mord, pas pour etre actives en production.
struct PostingsMutants {
  bool strict_threshold = false;        // w > k au lieu de w >= k
  bool skip_new_new = false;            // oublier les paires nouveau--nouveau
  bool omit_last_posting = false;       // derniere posting du lot jamais publiee
  bool truncate_membership = false;     // membres tronques a K+1 (interdit §1)
  bool collect_silent = false;          // generateur silencieux collecte (interdit §5)
  bool sequential_equal_levels = false; // lot de niveau egal committe sequentiellement
};

inline SaturatedFold build_saturated_fold_postings(const mhgp::Catalogue& catalogue,
                                                   int maximum_order, bool keep_partitions,
                                                   PostingsReceipt* receipt,
                                                   PostingsMutants mutants = {},
                                                   bool enforce_event_guard = false,
                                                   bool collect_pairs = false,
                                                   long long memory_budget_bytes = 0) {
  SaturatedFold fold;
  fold.maximum_order = maximum_order;
  // LE RECU EST REMIS A ZERO A L'ENTREE : un refus ne doit jamais laisser
  // observable un ancien identities_ok=true (contrat fail-closed de l'audit).
  if (receipt != nullptr) *receipt = PostingsReceipt{};
  // K EST BORNE AVANT TOUTE ALLOCATION : il dimensionne les etats par ordre.
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank) {
    fold.refusal = "ordre maximal hors contrat";
    return fold;
  }
  const std::size_t count = catalogue.spheres.size();

  std::vector<std::vector<mhgp::i32>> members(count);
  mhgp::i32 max_point = -1;
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
    for (std::size_t t = 0; t < members[s].size(); ++t) {
      if (members[s][t] < 0) {
        fold.refusal = "membre negatif : les postings indexent par PointId";
        return fold;
      }
      if (t > 0 && members[s][t - 1] >= members[s][t]) {
        fold.refusal = "membres non tries ou dupliques";
        return fold;
      }
      max_point = std::max(max_point, members[s][t]);
    }
    // MUTANT truncate_membership : la note §1 l'interdit expressement — « les
    // listes de membres et postings ne peuvent jamais etre tronquees a K+1 ».
    if (mutants.truncate_membership &&
        members[s].size() > (std::size_t)(maximum_order + 1))
      members[s].resize((std::size_t)(maximum_order + 1));
  }

  std::vector<int> by_level((std::size_t)count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });

  fold.orders.resize((std::size_t)maximum_order);
  const int K = maximum_order;

  // LES IDENTIFIANTS SONT COMPRESSES : le tableau de postings est indexe par
  // le rang du PointId dans l'univers trie des membres, jamais par la valeur
  // brute — un identifiant clairsemé (jusqu'a INT_MAX) coutait O(max(PointId))
  // et `max_point + 1` pouvait deborder (contrat fail-closed de l'audit).
  std::vector<mhgp::i32> universe;
  for (const std::vector<mhgp::i32>& generator_members : members)
    universe.insert(universe.end(), generator_members.begin(), generator_members.end());
  std::sort(universe.begin(), universe.end());
  universe.erase(std::unique(universe.begin(), universe.end()), universe.end());
  const auto dense_of = [&universe](mhgp::i32 raw) {
    return (mhgp::i32)(std::lower_bound(universe.begin(), universe.end(), raw) -
                       universe.begin());
  };
  for (std::vector<mhgp::i32>& generator_members : members)
    for (mhgp::i32& x : generator_members) x = dense_of(x);
  static_cast<void>(max_point);

  // PREFLIGHT MEMOIRE (audit 621ee80 §6) : degres, L_sat, P_post et pic
  // conservateur en arithmetique VERIFIEE (u128 plafonne a 2^62), AVANT toute
  // emission — le refus de budget precede l'allocation dependante de la masse.
  // Le parcours des lots est celui de la boucle (by_level, memes classes).
  long long expected_postings_mass = 0;
  long long preflight_p_post = 0, preflight_max_batch = 0, preflight_peak = 0;
  {
    using u128 = unsigned __int128;
    const u128 ceiling = (u128)1 << 62;
    std::vector<long long> final_degree(universe.size(), 0);
    for (const std::vector<mhgp::i32>& generator_members : members) {
      expected_postings_mass += (long long)generator_members.size();
      for (mhgp::i32 x : generator_members) ++final_degree[(std::size_t)x];
    }
    u128 predicted = 0;
    for (long long d : final_degree) {
      predicted += (u128)d * (u128)(d - 1) / 2;
      if (predicted >= ceiling) {
        fold.refusal = "preflight : P_post deborde le contrat entier";
        return fold;
      }
    }
    std::vector<long long> pre_degree(universe.size(), 0);
    u128 max_batch = 0;
    std::size_t walker = 0;
    while (walker < count) {
      std::size_t walker_end = walker + 1;
      if (!mutants.sequential_equal_levels)
        while (walker_end < count &&
               mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[walker]].sph,
                                     catalogue.spheres[(std::size_t)by_level[walker_end]].sph) ==
                   0)
          ++walker_end;
      u128 batch_occurrences = 0;
      std::map<mhgp::i32, long long> batch_count;
      for (std::size_t b = walker; b < walker_end; ++b)
        for (mhgp::i32 x : members[(std::size_t)by_level[b]]) {
          batch_occurrences += (u128)pre_degree[(std::size_t)x];
          ++batch_count[x];
        }
      for (const auto& entry : batch_count) {
        batch_occurrences += (u128)entry.second * (u128)(entry.second - 1) / 2;
        pre_degree[entry.first] += entry.second;
      }
      if (batch_occurrences >= ceiling) {
        fold.refusal = "preflight : masse d'un lot deborde le contrat entier";
        return fold;
      }
      max_batch = std::max(max_batch, batch_occurrences);
      walker = walker_end;
    }
    // Pic CONSERVATEUR : 32 octets par occurrence du plus gros lot (emission
    // + reduction), 8 par posting, etats par ordre (DSU, noeuds, epoques) et
    // couvertures small-to-large bornees par 64 octets par membre par ordre.
    const u128 peak = max_batch * 32 + (u128)expected_postings_mass * 8 +
                      (u128)count * 40 * (u128)K +
                      (u128)expected_postings_mass * 64 * (u128)K;
    if (peak >= ceiling) {
      fold.refusal = "preflight : pic memoire predit deborde le contrat entier";
      return fold;
    }
    preflight_p_post = (long long)predicted;
    preflight_max_batch = (long long)max_batch;
    preflight_peak = (long long)peak;
    // La prediction est ecrite AVANT le refus de budget : elle reste
    // observable meme sur un NO-GO — c'est le manifeste du refus.
    if (receipt != nullptr) {
      receipt->predicted_p_post = preflight_p_post;
      receipt->max_batch_occurrences = preflight_max_batch;
      receipt->predicted_peak_bytes = preflight_peak;
    }
    if (memory_budget_bytes > 0 && peak > (u128)memory_budget_bytes) {
      fold.refusal = "preflight : pic memoire predit au-dessus du budget";
      return fold;
    }
  }

  // Postings : P[x] = generateurs contenant x, dans l'ordre d'activation.
  std::vector<std::vector<int>> postings(universe.size());

  // Etat PAR ORDRE. La capture d'epoque fige, par racine et par lot, le noeud
  // strict ET la taille de couverture pre-lot — c'est ce qui remplace le scan.
  struct OrderState {
    std::vector<int> parent;
    std::vector<long long> node_of_root;
    std::vector<std::set<mhgp::i32>> coverage;
    std::set<int> live_roots;
    std::vector<long long> touch_epoch;
    std::vector<long long> staged_node;
    std::vector<std::size_t> staged_cov;
    long long next_node = 0;
    int find(int a) {
      while (parent[(std::size_t)a] != a) {
        parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
        a = parent[(std::size_t)a];
      }
      return a;
    }
  };
  std::vector<OrderState> states((std::size_t)K);
  for (OrderState& st : states) {
    st.parent.resize(count);
    for (std::size_t s = 0; s < count; ++s) st.parent[s] = (int)s;
    st.node_of_root.assign(count, -1);
    st.coverage.resize(count);
    st.touch_epoch.assign(count, -1);
    st.staged_node.assign(count, -1);
    st.staged_cov.assign(count, 0);
  }
  long long epoch = 0;
  PostingsReceipt out;
  out.predicted_p_post = preflight_p_post;
  out.max_batch_occurrences = preflight_max_batch;
  out.predicted_peak_bytes = preflight_peak;

  std::size_t cursor = 0;
  while (cursor < count) {
    std::size_t batch_end = cursor + 1;
    // MUTANT sequential_equal_levels : committer un lot de niveau egal boule
    // par boule casse l'atomicite Q1.2 — la fixture multifusion a deux
    // generateurs le voit sur les niveaux ET le transcript.
    if (!mutants.sequential_equal_levels)
      while (batch_end < count &&
             mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                                   catalogue.spheres[(std::size_t)by_level[batch_end]].sph) == 0)
        ++batch_end;
    ++epoch;

    const auto touch = [&](OrderState& st, int root) {
      if (st.touch_epoch[(std::size_t)root] != epoch) {
        st.touch_epoch[(std::size_t)root] = epoch;
        st.staged_node[(std::size_t)root] = st.node_of_root[(std::size_t)root];
        st.staged_cov[(std::size_t)root] = st.coverage[(std::size_t)root].size();
      }
    };

    // 1. EMISSION : ancien--nouveau par les postings, nouveau--nouveau par B_x.
    // La forme d'echelle du §2 de la note : emettre les clefs canoniques dans
    // un vecteur, TRIER puis REDUIRE par plages — aucune map par occurrence.
    std::vector<std::pair<int, int>> occurrences;
    std::map<mhgp::i32, std::vector<int>> batch_postings;
    long long batch_old_new = 0, batch_new_new = 0;
    for (std::size_t b = cursor; b < batch_end; ++b) {
      const int m = by_level[b];
      for (mhgp::i32 x : members[(std::size_t)m]) {
        for (int nid : postings[(std::size_t)x]) {
          ++batch_old_new;
          occurrences.push_back({std::min(m, nid), std::max(m, nid)});
        }
        batch_postings[x].push_back(m);
      }
    }
    if (!mutants.skip_new_new)
      for (const auto& entry : batch_postings) {
        const std::vector<int>& bx = entry.second;
        batch_new_new += (long long)bx.size() * ((long long)bx.size() - 1) / 2;
        for (std::size_t i = 0; i < bx.size(); ++i)
          for (std::size_t j = i + 1; j < bx.size(); ++j)
            occurrences.push_back({std::min(bx[i], bx[j]), std::max(bx[i], bx[j])});
      }
    out.old_new_occurrences += batch_old_new;
    out.new_new_occurrences += batch_new_new;

    // 2. REDUCTION et IDENTITE DU LOT : occurrences emises == somme des poids.
    std::sort(occurrences.begin(), occurrences.end());
    std::vector<std::pair<std::pair<int, int>, long long>> weights;
    for (std::size_t i = 0; i < occurrences.size();) {
      std::size_t j = i;
      while (j < occurrences.size() && occurrences[j] == occurrences[i]) ++j;
      weights.push_back({occurrences[i], (long long)(j - i)});
      i = j;
    }
    long long batch_weight_sum = 0;
    for (const auto& entry : weights) batch_weight_sum += entry.second;
    out.reduced_pairs += (long long)weights.size();
    out.weight_sum += batch_weight_sum;
    if (collect_pairs) out.pairs.insert(out.pairs.end(), weights.begin(), weights.end());
    if (batch_weight_sum != batch_old_new + batch_new_new) {
      fold.refusal = "identite de lot violee : occurrences != somme des poids";
      return fold;
    }

    // 3. ACTIVATIONS par ordre, avec capture d'epoque des racines fraiches, et
    // collecte des GENERATEURS D'EVENEMENT (n_support <= k+1) du lot.
    std::vector<std::vector<int>> batch_touched((std::size_t)K);
    std::vector<std::vector<std::pair<int, int>>> batch_events((std::size_t)K);
    for (std::size_t b = cursor; b < batch_end; ++b) {
      const int m = by_level[b];
      const int support_size = (int)catalogue.spheres[(std::size_t)m].n_support;
      for (int k = 1; k <= K; ++k) {
        if ((int)members[(std::size_t)m].size() < k) continue;
        OrderState& st = states[(std::size_t)(k - 1)];
        touch(st, m);   // racine fraiche : noeud strict -1, couverture 0
        st.coverage[(std::size_t)m].insert(members[(std::size_t)m].begin(),
                                           members[(std::size_t)m].end());
        st.live_roots.insert(m);
        batch_touched[(std::size_t)(k - 1)].push_back(m);
        if (support_size <= k + 1)
          batch_events[(std::size_t)(k - 1)].push_back({m, support_size});
      }
    }

    // 4. UNIONS : chaque paire reduite de poids w alimente les DSU d'ordre
    // k <= min(K, w) — le mutant de seuil exige w > k, soit k <= min(K, w-1).
    for (const auto& entry : weights) {
      const int m = entry.first.first, nid = entry.first.second;
      const long long w = entry.second;
      const long long cap =
          std::min<long long>(K, mutants.strict_threshold ? w - 1 : w);
      for (int k = 1; (long long)k <= cap; ++k) {
        OrderState& st = states[(std::size_t)(k - 1)];
        if ((int)members[(std::size_t)m].size() < k ||
            (int)members[(std::size_t)nid].size() < k)
          continue;
        ++out.unions_attempted;
        int rm = st.find(m), rn = st.find(nid);
        touch(st, rm);
        touch(st, rn);
        if (rm == rn) continue;
        ++out.unions_done;
        if (st.coverage[(std::size_t)rm].size() > st.coverage[(std::size_t)rn].size())
          std::swap(rm, rn);
        st.coverage[(std::size_t)rn].insert(st.coverage[(std::size_t)rm].begin(),
                                            st.coverage[(std::size_t)rm].end());
        std::set<mhgp::i32>().swap(st.coverage[(std::size_t)rm]);
        st.parent[(std::size_t)rm] = rn;
        st.live_roots.erase(rm);
        batch_touched[(std::size_t)(k - 1)].push_back(rn);
        batch_touched[(std::size_t)(k - 1)].push_back(rm);
      }
    }

    // 5. CLASSIFICATION locale aux racines touchees. Le noeud strict et la
    // taille de couverture pre-lot sont lus dans les captures d'epoque : la
    // composante stricte porteuse du noeud a ete capturee AVANT toute union.
    bool batch_public_event = false;
    for (int k = 1; k <= K; ++k) {
      OrderState& st = states[(std::size_t)(k - 1)];
      SaturatedOrderFold& order = fold.orders[(std::size_t)(k - 1)];
      std::vector<int>& touched = batch_touched[(std::size_t)(k - 1)];
      if (touched.empty()) continue;
      std::map<int, std::set<long long>> strict_of;
      std::map<int, std::size_t> strict_cov_of;
      std::set<int> finals;
      for (int r : touched) {
        const int final_root = st.find(r);
        finals.insert(final_root);
        if (st.touch_epoch[(std::size_t)r] == epoch &&
            st.staged_node[(std::size_t)r] >= 0) {
          strict_of[final_root].insert(st.staged_node[(std::size_t)r]);
          // La taille pre-lot de LA composante stricte : celle capturee avec le
          // noeud. Plusieurs racines peuvent porter le meme noeud apres unions
          // anterieures du meme lot ; la premiere capture est la bonne, et
          // toutes portent la meme valeur pre-lot.
          strict_cov_of.emplace(final_root, st.staged_cov[(std::size_t)r]);
        }
      }
      for (int root : finals) {
        const auto it = strict_of.find(root);
        const std::size_t strict = it == strict_of.end() ? 0 : it->second.size();
        if (strict == 0) {
          ++order.births;
          batch_public_event = true;
          st.node_of_root[(std::size_t)root] = st.next_node++;
        } else if (strict == 1) {
          const std::size_t before = strict_cov_of[root];
          if (st.coverage[(std::size_t)root].size() > before) {
            ++order.coverage_growth_batches;
            batch_public_event = true;
          } else {
            ++order.silent_generator_batches;
          }
          st.node_of_root[(std::size_t)root] = *it->second.begin();
        } else {
          ++order.fusions;
          batch_public_event = true;
          st.node_of_root[(std::size_t)root] = st.next_node++;
        }
      }
      // LE TRANSCRIPT GAMMA PAR MARQUAGE, identique au fold de verite : les
      // racines finales des generateurs d'evenement, classees par les racines
      // strictes capturees a l'epoque, avec la garde q <= k des naissances.
      std::map<int, int> marked_minimum_support;
      for (const std::pair<int, int>& event : batch_events[(std::size_t)(k - 1)]) {
        const int root = st.find(event.first);
        const auto it = marked_minimum_support.find(root);
        if (it == marked_minimum_support.end())
          marked_minimum_support.emplace(root, event.second);
        else
          it->second = std::min(it->second, event.second);
      }
      long long births_here = 0, continuations_here = 0, multifusions_here = 0;
      for (const auto& entry : marked_minimum_support) {
        const auto it = strict_of.find(entry.first);
        const std::size_t strict = it == strict_of.end() ? 0 : it->second.size();
        if (strict == 0) {
          ++births_here;
          if (entry.second > k) {
            ++order.event_guard_violations;
            if (enforce_event_guard) {
              fold.refusal = "garde d'evenement violee : naissance marquee sans support q<=k";
              return fold;
            }
          }
        } else if (strict == 1) {
          ++continuations_here;
        } else {
          ++multifusions_here;
        }
      }
      order.gamma_births += births_here;
      order.gamma_continuations += continuations_here;
      order.gamma_multifusions += multifusions_here;

      order.level_representative.push_back(by_level[cursor]);
      order.gamma_birth_at_level.push_back(births_here);
      order.gamma_continuation_at_level.push_back(continuations_here);
      order.gamma_multifusion_at_level.push_back(multifusions_here);
      if (keep_partitions) {
        // Les couvertures vivent en identifiants COMPRESSES ; la partition
        // publiee est RETRADUITE en PointId bruts (l'ordre est preserve :
        // universe est croissant).
        FoldPartition partition;
        for (int root : st.live_roots) {
          std::vector<mhgp::i32> cluster;
          for (mhgp::i32 dense : st.coverage[(std::size_t)root])
            cluster.push_back(universe[(std::size_t)dense]);
          partition.push_back(std::move(cluster));
        }
        std::sort(partition.begin(), partition.end());
        order.closed_partitions.push_back(std::move(partition));
      }
    }

    // 6. PUBLIER les postings du lot — silencieux compris, jamais supprimes :
    // un silencieux porte des connexions futures (fixture de la note, §6).
    // MUTANT collect_silent : jeter les postings d'un lot sans evenement public
    // perd l'attache future (fixture A,B,S,N) ET casse l'identite globale.
    // MUTANT omit_last_posting : la derniere boule du lot n'est jamais publiee.
    if (!mutants.collect_silent || batch_public_event) {
      const std::size_t publish_end =
          mutants.omit_last_posting && batch_end > cursor ? batch_end - 1 : batch_end;
      for (std::size_t b = cursor; b < publish_end; ++b) {
        const int m = by_level[b];
        for (mhgp::i32 x : members[(std::size_t)m]) postings[(std::size_t)x].push_back(m);
      }
    }
    cursor = batch_end;
  }

  // IDENTITES GLOBALES : la masse publiee somme_x |P_x| == somme des |S|
  // (aucune posting omise ni collectee), P_post = somme_x C(d_x, 2) ==
  // somme des poids reduits (chaque paire intersectante reduite exactement une
  // fois, au niveau du plus tardif), et P_post reel == P_post PREDIT par le
  // preflight. Toute violation refuse le fold ENTIER.
  for (const std::vector<int>& px : postings) {
    out.postings_mass += (long long)px.size();
    out.max_posting = std::max(out.max_posting, px.size());
    out.p_post += (long long)px.size() * ((long long)px.size() - 1) / 2;
  }
  out.identities_ok =
      out.p_post == out.weight_sum && out.postings_mass == expected_postings_mass;
  if (out.postings_mass != expected_postings_mass) {
    fold.refusal = "identite de masse violee : postings incompletes";
    return fold;
  }
  if (out.p_post != out.weight_sum && !mutants.skip_new_new) {
    fold.refusal = "identite globale violee : P_post != somme des poids";
    return fold;
  }
  if (out.p_post != out.predicted_p_post) {
    fold.refusal = "identite de preflight violee : P_post reel != predit";
    return fold;
  }
  if (receipt != nullptr) *receipt = out;
  fold.ok = true;
  return fold;
}

}  // namespace mhgp3v
