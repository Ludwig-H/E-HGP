// MorseHGP3D v3 — L'INDEX EXACT PREFIXE--PREFIXE (note solution GPU index).
//
// THEOREME (note, preuve par le plus petit element commun) : sous un ordre
// total COMMUN ET IMMUABLE sur les PointId, avec P_k(S) = les |S|-k+1
// premiers elements de S :
//
//     |M ∩ N| >= k   =>   P_k(M) ∩ P_k(N) != vide.
//
// Le filtre ne perd donc AUCUN couple d'intersection au moins k ; une
// intersection courte des membres recertifie ensuite chaque candidat. La
// longueur |S|-k+1 est optimale au pire cas : elements communs APRES deux
// prefixes prives disjoints de taille |S|-k — c'est la fixture qui tue le
// mutant prefix-length-minus-one.
//
// POSSESSION CANONIQUE (audit 8df7ac8, P0) : le lot exact entier est stage
// avant la premiere requete, puis une requete M garde le candidat N ssi N est
// d'un lot strictement anterieur, ou non-requete du lot courant, ou
// ActivationId(N) < ActivationId(M). Ainsi une paire Q--Q est possedee
// exactement une fois par l'ActivationId le plus grand, une paire Q--R est
// toujours gardee par la requete (meme si le R est posterieur dans le lot),
// et les paires sans extremite requete restent l'obligation du sidecar
// principal. L'ActivationId v1 est la POSITION D'ACTIVATION dans l'ordre
// global (niveau exact, indice catalogue) — jamais un numero de racine DSU.
//
// PREFLIGHT EXACT (audit 8df7ac8, P0) : avec d_x le degre du posting prefixe
// et q_x le nombre de requetes dont le prefixe contient x, la concatenation
// lit exactement H_Q = somme_x q_x * d_x ; en tout-requete q_x = d_x et
// H = somme_x d_x^2. Les degres sont figes a la CLOTURE DU STAGING du lot ;
// l'identite `predicted_hits == hits` est un invariant a refus, pas une ligne
// decorative — un staging pendant la phase de requetes la casse.
//
// Ce fichier est l'autorite COMBINATOIRE pure : aucune geometrie, aucun DSU.
// L'ordre v1 est l'identifiant de point (les membres du catalogue sont deja
// tries par identifiant). Un ordre rare-first devra etre fige sur le
// catalogue final pour tous les lots d'un meme ordre k ; un ordre par
// requete, par lot ou par worker invalide le theoreme — c'est le mutant
// order-per-query.
#pragma once

#include <algorithm>
#include <vector>

namespace mhgp3v {

struct PrefixIndexMutants {
  bool prefix_length_minus_one = false;   // prefixes trop courts (refutes)
  bool drop_last_posting = false;         // le dernier point du prefixe non indexe
  bool skip_recertification = false;      // candidats crus sans |M∩N|>=k
  bool project_root_first = false;        // fold seulement : certifier contre la
                                          // couverture de la racine, pas le generateur
  bool duplicate_posting = false;         // premier point du prefixe indexe DEUX fois
                                          // (tue par l'identite de masse L(r,K))
  bool double_query_pair = false;         // possession desactivee : chaque requete
                                          // garde tout (tue par la multiplicite)
  bool order_per_query = false;           // la requete lit un SUFFIXE : un ordre par
                                          // requete invalide le theoreme
  bool future_visible = false;            // pilote : les lots futurs sont stages
                                          // d'avance (tue par le differentiel)
  bool stage_query_sequentially = false;  // pilote : stage juste avant sa propre
                                          // requete, pas le lot entier d'abord
};

struct PrefixIndexReceipt {
  long long entries = 0;             // entrees d'index posees (masse L_k)
  long long queries = 0;             // requetes fallback
  long long predicted_hits = 0;      // somme q_x*d_x, degres figes au staging
  long long hits = 0;                // hits bruts concatenes (== predicted_hits)
  long long candidate_ids_including_self = 0;   // apres unique, AVANT tout filtre
  long long candidate_pairs_after_filter = 0;   // apres self + possession, AVANT
                                                // recertification
  long long recertified_true = 0;    // |M∩N| >= k confirme
  long long false_candidates = 0;    // candidats rejetes par la recertification
};

// La longueur de prefixe du contrat : rang - k + 1. Le mutant la reduit d'un,
// plancher un — c'est la longueur juste en dessous de l'optimal, celle que la
// fixture aux elements communs en dernieres positions refute.
inline int prefix_length(int rank, int k, bool minus_one_mutant) {
  int length = rank - k + 1;
  if (minus_one_mutant) length = std::max(1, length - 1);
  return length;
}

// L'index CSR simple : listes par point. `universe` borne les identifiants.
struct PrefixIndex {
  std::vector<std::vector<int>> lists;
  void reset(int universe) {
    lists.assign((std::size_t)universe, {});
  }
};

// Stager un ensemble (membres TRIES par l'ordre commun) sous l'identifiant
// `id`, pour l'ordre k. Rend le nombre d'entrees posees — le mutant
// duplicate-posting pose une entree de plus et la COMPTE : c'est l'identite
// de masse qui doit le voir, pas un compteur complaisant.
template <class Members>
inline long long prefix_stage(PrefixIndex* index, int id, const Members& members, int k,
                              PrefixIndexMutants mutants = {}) {
  const int rank = (int)members.size();
  if (rank < k) return 0;
  int length = prefix_length(rank, k, mutants.prefix_length_minus_one);
  if (mutants.drop_last_posting && length > 0) --length;   // MUTANT : dernier omis
  for (int i = 0; i < length; ++i) index->lists[(std::size_t)members[i]].push_back(id);
  long long posted = length;
  if (mutants.duplicate_posting && length > 0) {   // MUTANT : premier point double
    index->lists[(std::size_t)members[0]].push_back(id);
    ++posted;
  }
  return posted;
}

// Les points effectivement interroges par la requete de M : le PREFIXE du
// contrat, ou le SUFFIXE sous le mutant order-per-query (un ordre par requete
// pendant que l'index reste stage par prefixe d'identifiants).
template <class Members>
inline void prefix_query_span(const Members& members, int k, PrefixIndexMutants mutants,
                              std::size_t* begin, std::size_t* end) {
  const int rank = (int)members.size();
  const int length = prefix_length(rank, k, mutants.prefix_length_minus_one);
  if (mutants.order_per_query) {
    *begin = (std::size_t)(rank - length);
    *end = (std::size_t)rank;
  } else {
    *begin = 0;
    *end = (std::size_t)length;
  }
}

// PREFLIGHT : la masse exacte que la requete de M lira, aux degres COURANTS
// de l'index. Appele a la cloture du staging du lot, avant toute requete —
// somme q_x*d_x morceau par morceau. En entiers 64 bits : la somme reste sous
// (entrees totales) * (requetes), tres loin du debordement aux tailles CPU ;
// le manifeste device recalculera en largeur verifiee.
template <class Members>
inline long long prefix_predicted_hits(const PrefixIndex& index, const Members& members,
                                       int k, PrefixIndexMutants mutants = {}) {
  const int rank = (int)members.size();
  if (rank < k) return 0;
  std::size_t begin = 0, end = 0;
  prefix_query_span(members, k, mutants, &begin, &end);
  long long predicted = 0;
  for (std::size_t i = begin; i < end; ++i)
    predicted += (long long)index.lists[(std::size_t)members[i]].size();
  return predicted;
}

// Requete : concatener les listes du prefixe de M, dedupliquer. Le self n'est
// PAS retire ici — l'appelant applique self + possession canonique et compte
// `candidate_pairs_after_filter` de son cote.
template <class Members>
inline void prefix_query(const PrefixIndex& index, const Members& members, int k,
                         std::vector<int>* candidates, PrefixIndexReceipt* receipt,
                         PrefixIndexMutants mutants = {}) {
  candidates->clear();
  const int rank = (int)members.size();
  if (rank < k) return;
  ++receipt->queries;
  std::size_t begin = 0, end = 0;
  prefix_query_span(members, k, mutants, &begin, &end);
  for (std::size_t i = begin; i < end; ++i) {
    const std::vector<int>& list = index.lists[(std::size_t)members[i]];
    receipt->hits += (long long)list.size();
    candidates->insert(candidates->end(), list.begin(), list.end());
  }
  std::sort(candidates->begin(), candidates->end());
  candidates->erase(std::unique(candidates->begin(), candidates->end()), candidates->end());
  receipt->candidate_ids_including_self += (long long)candidates->size();
}

// La recertification exacte : |M ∩ N| >= k sur deux listes triees, sortie
// anticipee des que k est atteint ou inatteignable.
template <class MembersA, class MembersB>
inline bool prefix_recertify(const MembersA& a, const MembersB& b, int k) {
  std::size_t i = 0, j = 0;
  int common = 0;
  while (i < a.size() && j < b.size()) {
    if (common + (int)std::min(a.size() - i, b.size() - j) < k) return false;
    if (a[i] < b[j]) ++i;
    else if (b[j] < a[i]) ++j;
    else {
      ++common;
      if (common >= k) return true;
      ++i;
      ++j;
    }
  }
  return common >= k;
}

}  // namespace mhgp3v
