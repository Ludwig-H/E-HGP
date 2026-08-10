// MorseHGP3D v3 — LA PORTE ENSEMBLISTE DE L'INDEX PREFIXE--PREFIXE.
//
// Trois etages, tous COMBINATOIRES purs (aucune geometrie) :
//
//   1. les FIXTURES GRAVEES de la note : elements communs aux DERNIERES
//      positions (la paire existe, les prefixes du contrat la voient, le
//      mutant prefix-length-minus-one la perd ; le seul point commun des deux
//      prefixes est le DERNIER de chacun, le mutant drop-last-posting la perd
//      aussi) ; faux candidat (un seul point partage < k : le filtre propose,
//      la recertification refuse — skip-recertification le laisse passer) ;
//   2. l'IDENTITE DE MASSE de la note : la somme des entrees d'index sur
//      k=1..K vaut exactement L(r,K) = m(r+1) - m(m+1)/2, m = min(K,r) ;
//   3. le DIFFERENTIEL contre le join quadratique : sur des familles
//      d'ensembles tires, pour TOUT k, l'ensemble des paires recertifiees
//      egale exactement l'ensemble des paires |M∩N| >= k — avec planchers de
//      paires ET de faux candidats (une campagne qui n'exerce pas la
//      recertification ne peut pas rendre OK).
//
// Codes du contrat : 4 = mutant tue, 2 = campagne/mutant refuse, 1 = ecart.
#include <charconv>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "prototype/prefix_index.hpp"

namespace {

using Pair = std::pair<int, int>;

// Toutes les paires certifiees par la voie prefixe sur une famille, pour k.
std::set<Pair> prefix_pairs(const std::vector<std::vector<int>>& sets, int universe, int k,
                            mhgp3v::PrefixIndexMutants mutants,
                            mhgp3v::PrefixIndexReceipt* receipt) {
  mhgp3v::PrefixIndex index;
  index.reset(universe);
  for (std::size_t s = 0; s < sets.size(); ++s)
    receipt->entries += mhgp3v::prefix_stage(&index, (int)s, sets[s], k, mutants);
  std::set<Pair> pairs;
  std::vector<int> candidates;
  for (std::size_t m = 0; m < sets.size(); ++m) {
    if ((int)sets[m].size() < k) continue;
    mhgp3v::prefix_query(index, sets[m], k, &candidates, receipt, mutants);
    for (int n : candidates) {
      if (n == (int)m) continue;
      const bool certified =
          mutants.skip_recertification ||
          mhgp3v::prefix_recertify(sets[m], sets[(std::size_t)n], k);
      if (!certified) {
        ++receipt->false_candidates;
        continue;
      }
      ++receipt->recertified_true;
      pairs.insert({std::min((int)m, n), std::max((int)m, n)});
    }
  }
  return pairs;
}

// La verite quadratique : toutes les paires d'intersection au moins k.
std::set<Pair> quadratic_pairs(const std::vector<std::vector<int>>& sets, int k) {
  std::set<Pair> pairs;
  for (std::size_t a = 0; a < sets.size(); ++a)
    for (std::size_t b = a + 1; b < sets.size(); ++b)
      if ((int)sets[a].size() >= k && (int)sets[b].size() >= k &&
          mhgp3v::prefix_recertify(sets[a], sets[b], k))
        pairs.insert({(int)a, (int)b});
  return pairs;
}

}  // namespace

int main(int argc, char** argv) {
  int families = 400, group = 24, universe = 30, max_order = 8;
  long long seed = 20260810, min_pairs = 2000, min_false = 200;
  const char* mutant_name = nullptr;
  auto integer = [](const char* text, long long* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--mutant")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --mutant\n"); return 2; }
      mutant_name = argv[++i];
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--families")) families = (int)value;
    else if (!strcmp(argv[i], "--sets")) group = (int)value;
    else if (!strcmp(argv[i], "--universe")) universe = (int)value;
    else if (!strcmp(argv[i], "--max-order")) max_order = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--min-pairs")) min_pairs = value;
    else if (!strcmp(argv[i], "--min-false")) min_false = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (families < 1 || families > 100000 || group < 2 || group > 256 || universe < 8 ||
      universe > 65536 || max_order < 1 || max_order > 11 || min_pairs < 0 || min_false < 0) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  mhgp3v::PrefixIndexMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "prefix-length-minus-one")) mutants.prefix_length_minus_one = true;
    else if (!strcmp(mutant_name, "drop-last-posting")) mutants.drop_last_posting = true;
    else if (!strcmp(mutant_name, "skip-recertification")) mutants.skip_recertification = true;
    else { std::printf("ECHEC : mutant inconnu %s\n", mutant_name); return 2; }
  }
  const auto kill = [&](const std::string& where, const std::string& why) {
    if (mutant_name != nullptr) {
      std::printf("mutant tue par %s : %s\n", where.c_str(), why.c_str());
      return 4;
    }
    std::printf("ECHEC %s : %s\n", where.c_str(), why.c_str());
    return 1;
  };

  // FIXTURE 1 : elements communs aux DERNIERES positions, k=3, rangs 6. Les
  // prefixes du contrat (longueur 4) se rencontrent seulement en leur DERNIER
  // point (10) ; la paire doit etre certifiee. prefix-length-minus-one ET
  // drop-last-posting la perdent chacun.
  {
    const std::vector<std::vector<int>> sets = {{0, 1, 2, 10, 11, 12}, {3, 4, 5, 10, 11, 12}};
    mhgp3v::PrefixIndexReceipt receipt;
    const std::set<Pair> got = prefix_pairs(sets, 16, 3, mutants, &receipt);
    if (got.count({0, 1}) == 0)
      return kill("la fixture des communs en dernieres positions",
                  "la paire {M,N} d'intersection 3 est perdue par le filtre");
  }
  // FIXTURE 2 : FAUX CANDIDAT, k=3. Un seul point partage (0) : les prefixes
  // se voient, la recertification doit refuser. skip-recertification le
  // laisse passer et rend une paire de trop.
  {
    const std::vector<std::vector<int>> sets = {{0, 1, 2, 9, 10, 11}, {0, 3, 4, 5, 6, 7}};
    mhgp3v::PrefixIndexReceipt receipt;
    const std::set<Pair> got = prefix_pairs(sets, 16, 3, mutants, &receipt);
    if (!got.empty())
      return kill("la fixture du faux candidat",
                  "une paire d'intersection 1 < k=3 a ete certifiee");
    if (mutant_name == nullptr && receipt.false_candidates == 0)
      return kill("la fixture du faux candidat", "la recertification n'a pas ete exercee");
  }

  // CAMPAGNE : familles d'ensembles tires, differentiel quadratique pour TOUT
  // k, identite de masse de la note par ensemble.
  std::mt19937 rng((unsigned)seed);
  long long total_pairs = 0, total_false = 0, total_entries = 0, expected_entries = 0;
  for (int f = 0; f < families; ++f) {
    std::vector<std::vector<int>> sets((std::size_t)group);
    for (std::vector<int>& s : sets) {
      const int rank = 1 + (int)(rng() % 11u);
      std::set<int> chosen;
      while ((int)chosen.size() < rank) chosen.insert((int)(rng() % (unsigned)universe));
      s.assign(chosen.begin(), chosen.end());
      // L'identite de masse de la note : somme des entrees sur k=1..K.
      const long long m = std::min((long long)max_order, (long long)s.size());
      expected_entries += m * ((long long)s.size() + 1) - m * (m + 1) / 2;
    }
    for (int k = 1; k <= max_order; ++k) {
      mhgp3v::PrefixIndexReceipt receipt;
      const std::set<Pair> got = prefix_pairs(sets, universe, k, mutants, &receipt);
      total_entries += receipt.entries;
      total_false += receipt.false_candidates;
      const std::set<Pair> want = quadratic_pairs(sets, k);
      if (got != want)
        return kill("le differentiel quadratique",
                    "famille " + std::to_string(f) + " k=" + std::to_string(k) + " : " +
                        std::to_string(got.size()) + " paires contre " +
                        std::to_string(want.size()));
      total_pairs += (long long)want.size();
    }
  }
  if (total_entries != expected_entries)
    return kill("l'identite de masse",
                std::to_string(total_entries) + " entrees contre L(r,K) somme " +
                    std::to_string(expected_entries));
  if (total_pairs < min_pairs) {
    std::printf("ECHEC : plancher de paires non atteint (%lld < %lld)\n", total_pairs,
                min_pairs);
    return 3;
  }
  if (total_false < min_false) {
    std::printf("ECHEC : plancher de faux candidats non atteint (%lld < %lld)\n", total_false,
                min_false);
    return 3;
  }
  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  std::printf("OK : prefixe--prefixe == join quadratique sur %d familles x %d ordres"
              " (%lld paires, %lld faux candidats refuses, masse d'index %lld == L(r,K))\n",
              families, max_order, total_pairs, total_false, total_entries);
  return 0;
}
