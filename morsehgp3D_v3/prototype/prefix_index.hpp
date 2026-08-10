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
// Ce fichier est l'autorite COMBINATOIRE pure : aucune geometrie, aucun DSU.
// L'ordre v1 est l'identifiant de point (les membres du catalogue sont deja
// tries par identifiant). Un ordre rare-first devra etre fige sur le
// catalogue final pour tous les lots d'un meme ordre k ; un ordre par
// requete, par lot ou par worker invalide le theoreme.
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
};

struct PrefixIndexReceipt {
  long long entries = 0;             // entrees d'index posees (masse L_k)
  long long queries = 0;             // requetes fallback
  long long hits = 0;                // hits bruts concatenes
  long long unique_candidates = 0;   // apres deduplication
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
// `id`, pour l'ordre k. Rend le nombre d'entrees posees.
template <class Members>
inline long long prefix_stage(PrefixIndex* index, int id, const Members& members, int k,
                              PrefixIndexMutants mutants = {}) {
  const int rank = (int)members.size();
  if (rank < k) return 0;
  int length = prefix_length(rank, k, mutants.prefix_length_minus_one);
  if (mutants.drop_last_posting && length > 0) --length;   // MUTANT : dernier omis
  for (int i = 0; i < length; ++i) index->lists[(std::size_t)members[i]].push_back(id);
  return length;
}

// Requete : concatener les listes du prefixe de M, dedupliquer. Le self n'est
// PAS retire ici — l'appelant decide (le fold exclut m, la porte ensembliste
// aussi).
template <class Members>
inline void prefix_query(const PrefixIndex& index, const Members& members, int k,
                         std::vector<int>* candidates, PrefixIndexReceipt* receipt,
                         PrefixIndexMutants mutants = {}) {
  candidates->clear();
  const int rank = (int)members.size();
  if (rank < k) return;
  ++receipt->queries;
  const int length = prefix_length(rank, k, mutants.prefix_length_minus_one);
  for (int i = 0; i < length; ++i) {
    const std::vector<int>& list = index.lists[(std::size_t)members[i]];
    receipt->hits += (long long)list.size();
    candidates->insert(candidates->end(), list.begin(), list.end());
  }
  std::sort(candidates->begin(), candidates->end());
  candidates->erase(std::unique(candidates->begin(), candidates->end()), candidates->end());
  receipt->unique_candidates += (long long)candidates->size();
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
