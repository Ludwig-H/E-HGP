// MorseHGP3D v3 — LA PORTE MULTIENSEMBLISTE DE L'INDEX PREFIXE--PREFIXE.
//
// Cinq etages, tous COMBINATOIRES purs (aucune geometrie) :
//
//   1. les FIXTURES GRAVEES : elements communs aux DERNIERES positions
//      (prefix-length-minus-one et drop-last-posting la perdent) ; faux
//      candidat (skip-recertification le laisse passer) ; communs
//      premier-de-M / dernier-de-N (order-per-query la perd : un ordre par
//      requete invalide le theoreme) ; requete PRECOCE et non-requete TARDIVE
//      du meme lot (stage-query-sequentially la perd : le lot doit etre
//      ENTIEREMENT stage avant la premiere requete) ; paire R--R laissee au
//      sidecar ; ancien-Q -- nouveau-R entre deux lots (future-visible la
//      certifie a tort : les lots futurs doivent rester invisibles) ; candidat
//      de lot anterieur garde par la requete posterieure ;
//   2. l'IDENTITE DE MASSE : la somme des entrees d'index sur k=1..K vaut
//      exactement L(r,K) = m(r+1) - m(m+1)/2, m = min(K,r) — duplicate-posting
//      la casse, meme s'il respecte l'identite de preflight ;
//   3. l'IDENTITE DE PREFLIGHT : les hits lus egalent la prediction
//      somme q_x*d_x aux degres figes a la CLOTURE DU STAGING ATOMIQUE du lot,
//      jamais recalcules avant chaque requete — stage-query-sequentially meurt
//      ici (reponse auditeur Q3) ;
//   4. le DIFFERENTIEL MULTIENSEMBLISTE contre le join quadratique POSSEDE :
//      sous des lots contigus et un masque de requetes tires par famille, le
//      multiensemble des paires recertifiees egale exactement les paires
//      d'intersection >= k possedees — Q--Q du meme lot une fois par
//      l'ActivationId le plus grand, Q--R du meme lot par la requete, candidat
//      de lot anterieur par la requete posterieure, et RIEN d'autre (les
//      paires sans requete apte a les voir restent au sidecar principal) —
//      avec planchers SEPARES Q--Q, anterieur, posterieur-meme-lot, R--R et
//      faux candidats (reponse auditeur Q2) ;
//   5. la PERMUTATION GLOBALE COMMUNE : renommer tous les points par une
//      bijection (rare-first en est une) et rejouer sous le MEME ordre commun
//      rend LE MEME multiensemble de paires.
//
// Codes du contrat : 4 = mutant tue, 3 = plancher, 2 = campagne refusee,
// 1 = ecart. Le rang est tire jusqu'a min(11, universe) : la campagne TERMINE
// pour tout universe admis — le hang hostile universe<11 est refute par un
// CTest dedie.
#include <charconv>
#include <cstdio>
#include <cstring>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "prototype/prefix_index.hpp"

namespace {

using Pair = std::pair<int, int>;
using PairCounts = std::map<Pair, long long>;   // multiensemble : paire -> multiplicite

// Le pilote de la voie prefixe sur des lots CONTIGUS : staging atomique du
// lot, prediction figee a sa cloture, requetes du lot avec possession
// canonique (ActivationId = indice d'ensemble), recertification. La
// possession lit `staged_lot` — ce que le systeme SAIT du staging — pendant
// que la verite lit les lots logiques : c'est ce decalage qui fait mordre les
// mutants de calendrier (future-visible, stage-query-sequentially).
PairCounts prefix_pairs(const std::vector<std::vector<int>>& sets,
                        const std::vector<char>& queries, const std::vector<int>& lot_of,
                        int lot_count, int universe, int k,
                        mhgp3v::PrefixIndexMutants mutants,
                        mhgp3v::PrefixIndexReceipt* receipt, bool* preflight_ok) {
  mhgp3v::PrefixIndex index;
  index.reset(universe);
  mhgp3v::PrefixIndexReceipt local;
  std::vector<int> staged_lot(sets.size(), -1);
  PairCounts pairs;
  std::vector<int> candidates;
  for (int lot = 0; lot < lot_count; ++lot) {
    if (mutants.future_visible && lot == 0) {
      // MUTANT : tout est stage d'avance — les lots futurs deviennent
      // visibles et passent pour anterieurs.
      for (std::size_t s = 0; s < sets.size(); ++s) {
        local.entries += mhgp3v::prefix_stage(&index, (int)s, sets[s], k, mutants);
        staged_lot[s] = 0;
      }
    } else if (!mutants.future_visible && !mutants.stage_query_sequentially) {
      for (std::size_t s = 0; s < sets.size(); ++s)
        if (lot_of[s] == lot) {
          local.entries += mhgp3v::prefix_stage(&index, (int)s, sets[s], k, mutants);
          staged_lot[s] = lot;
        }
    }
    // CLOTURE DU STAGING ATOMIQUE DU LOT : degres et masque figes ICI ; la
    // prediction n'est JAMAIS recalculee avant une requete (reponse Q3) — un
    // pilote sequentiel n'a encore rien stage de son lot et l'identite mord.
    for (std::size_t s = 0; s < sets.size(); ++s)
      if (lot_of[s] == lot && queries[s] != 0 && (int)sets[s].size() >= k)
        local.predicted_hits += mhgp3v::prefix_predicted_hits(index, sets[s], k, mutants);
    for (std::size_t s = 0; s < sets.size(); ++s) {
      if (lot_of[s] != lot) continue;
      if (mutants.stage_query_sequentially) {
        local.entries += mhgp3v::prefix_stage(&index, (int)s, sets[s], k, mutants);
        staged_lot[s] = lot;
      }
      if (queries[s] == 0) continue;
      if ((int)sets[s].size() < k) continue;
      mhgp3v::prefix_query(index, sets[s], k, &candidates, &local, mutants);
      for (int n : candidates) {
        if (n == (int)s) continue;
        // POSSESSION CANONIQUE : M garde N ssi N est d'un lot (de staging)
        // strictement anterieur, ou non-requete du lot courant, ou
        // ActivationId(N) < ActivationId(M). Q--Q une fois, Q--R a la requete.
        const bool keep =
            mutants.double_query_pair || staged_lot[(std::size_t)n] < lot ||
            (staged_lot[(std::size_t)n] == lot &&
             (queries[(std::size_t)n] == 0 || n < (int)s));
        if (!keep) continue;
        ++local.candidate_pairs_after_filter;
        const bool certified =
            mutants.skip_recertification ||
            mhgp3v::prefix_recertify(sets[s], sets[(std::size_t)n], k);
        if (!certified) {
          ++local.false_candidates;
          continue;
        }
        ++local.recertified_true;
        pairs[{std::min((int)s, n), std::max((int)s, n)}] += 1;
      }
    }
  }
  if (preflight_ok != nullptr) *preflight_ok = local.predicted_hits == local.hits;
  receipt->entries += local.entries;
  receipt->queries += local.queries;
  receipt->predicted_hits += local.predicted_hits;
  receipt->hits += local.hits;
  receipt->candidate_ids_including_self += local.candidate_ids_including_self;
  receipt->candidate_pairs_after_filter += local.candidate_pairs_after_filter;
  receipt->recertified_true += local.recertified_true;
  receipt->false_candidates += local.false_candidates;
  return pairs;
}

// La verite quadratique POSSEDEE, et les quatre categories de la reponse Q2 :
// une paire d'intersection >= k est possedee par la voie prefixe ssi une
// requete peut la voir — meme lot avec au moins une requete, ou lots
// distincts avec l'extremite POSTERIEURE requete. Les autres — R--R et
// ancien-Q -- nouveau-R — restent l'obligation du sidecar principal.
struct OwnedTallies {
  long long qq = 0;        // meme lot, deux requetes (possedee par la plus grande)
  long long prior = 0;     // candidat anterieur (lot anterieur, ou non-requete
                           // anterieure du meme lot)
  long long later = 0;     // meme lot, non-requete POSTERIEURE a la requete
  long long sidecar = 0;   // aucune requete apte : R--R et ancien-Q--nouveau-R
};

PairCounts owned_truth(const std::vector<std::vector<int>>& sets,
                       const std::vector<char>& queries, const std::vector<int>& lot_of,
                       int k, OwnedTallies* tallies) {
  PairCounts pairs;
  for (std::size_t a = 0; a < sets.size(); ++a)
    for (std::size_t b = a + 1; b < sets.size(); ++b) {
      if ((int)sets[a].size() < k || (int)sets[b].size() < k) continue;
      if (!mhgp3v::prefix_recertify(sets[a], sets[b], k)) continue;
      if (lot_of[a] == lot_of[b]) {
        if (queries[a] != 0 && queries[b] != 0) {
          pairs[{(int)a, (int)b}] = 1;
          ++tallies->qq;
        } else if (queries[a] != 0 || queries[b] != 0) {
          pairs[{(int)a, (int)b}] = 1;
          const std::size_t q = queries[a] != 0 ? a : b;
          const std::size_t r = queries[a] != 0 ? b : a;
          if (r > q) ++tallies->later;
          else ++tallies->prior;
        } else {
          ++tallies->sidecar;
        }
      } else {
        const std::size_t late = lot_of[a] < lot_of[b] ? b : a;
        if (queries[late] != 0) {
          pairs[{(int)a, (int)b}] = 1;
          ++tallies->prior;
        } else {
          ++tallies->sidecar;
        }
      }
    }
  return pairs;
}

std::string pair_counts_diff(const PairCounts& got, const PairCounts& want) {
  for (const auto& entry : want) {
    const auto it = got.find(entry.first);
    if (it == got.end())
      return "paire {" + std::to_string(entry.first.first) + "," +
             std::to_string(entry.first.second) + "} perdue";
    if (it->second != entry.second)
      return "paire {" + std::to_string(entry.first.first) + "," +
             std::to_string(entry.first.second) + "} de multiplicite " +
             std::to_string(it->second) + " au lieu de " + std::to_string(entry.second);
  }
  for (const auto& entry : got)
    if (want.count(entry.first) == 0)
      return "paire {" + std::to_string(entry.first.first) + "," +
             std::to_string(entry.first.second) + "} certifiee a tort";
  return "";
}

}  // namespace

int main(int argc, char** argv) {
  int families = 400, group = 24, universe = 30, max_order = 8;
  long long seed = 20260810, min_pairs = 2000, min_false = 200;
  long long min_qq = 100, min_prior = 100, min_later = 50, min_rr = 100;
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
    else if (!strcmp(argv[i], "--min-qq")) min_qq = value;
    else if (!strcmp(argv[i], "--min-prior")) min_prior = value;
    else if (!strcmp(argv[i], "--min-later")) min_later = value;
    else if (!strcmp(argv[i], "--min-rr")) min_rr = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (families < 1 || families > 100000 || group < 2 || group > 256 || universe < 8 ||
      universe > 65536 || max_order < 1 || max_order > 11 || min_pairs < 0 || min_false < 0 ||
      min_qq < 0 || min_prior < 0 || min_later < 0 || min_rr < 0) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  mhgp3v::PrefixIndexMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "prefix-length-minus-one")) mutants.prefix_length_minus_one = true;
    else if (!strcmp(mutant_name, "drop-last-posting")) mutants.drop_last_posting = true;
    else if (!strcmp(mutant_name, "skip-recertification")) mutants.skip_recertification = true;
    else if (!strcmp(mutant_name, "duplicate-posting")) mutants.duplicate_posting = true;
    else if (!strcmp(mutant_name, "double-query-pair")) mutants.double_query_pair = true;
    else if (!strcmp(mutant_name, "order-per-query")) mutants.order_per_query = true;
    else if (!strcmp(mutant_name, "future-visible")) mutants.future_visible = true;
    else if (!strcmp(mutant_name, "stage-query-sequentially"))
      mutants.stage_query_sequentially = true;
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
  // Une fixture : ensembles, lots, masque, ordre k, multiensemble attendu.
  struct GateFixture {
    const char* name;
    int k;
    int fixture_universe;
    std::vector<std::vector<int>> sets;
    std::vector<int> lots;
    std::vector<char> queries;
    PairCounts expected;
    bool expects_false_candidate;
  };
  std::vector<GateFixture> fixtures;
  // FIXTURE 1 : elements communs aux DERNIERES positions, k=3, rangs 6. Les
  // prefixes du contrat (longueur 4) se rencontrent seulement en leur DERNIER
  // point (10). prefix-length-minus-one ET drop-last-posting la perdent ;
  // double-query-pair la duplique (deux requetes du meme lot).
  fixtures.push_back({"les communs en dernieres positions", 3, 16,
                      {{0, 1, 2, 10, 11, 12}, {3, 4, 5, 10, 11, 12}},
                      {0, 0}, {1, 1}, {{{0, 1}, 1}}, false});
  // FIXTURE 2 : FAUX CANDIDAT, k=3. Un seul point partage (0) : les prefixes
  // se voient, la recertification doit refuser — et etre reellement exercee.
  fixtures.push_back({"le faux candidat", 3, 16,
                      {{0, 1, 2, 9, 10, 11}, {0, 3, 4, 5, 6, 7}},
                      {0, 0}, {1, 1}, {}, true});
  // FIXTURE 3 : communs PREMIER-DE-M / DERNIER-DE-N, k=2. L'intersection est
  // {0,9} : 0 est le premier de M (exclu de son suffixe), 9 le dernier de N
  // (exclu de son prefixe indexe). Sous l'ordre commun la paire est vue par la
  // requete de N au point 0 ; order-per-query (la requete lit un suffixe) la
  // perd des deux cotes.
  fixtures.push_back({"les communs premier-de-M dernier-de-N", 2, 16,
                      {{0, 5, 6, 9}, {0, 1, 2, 9}},
                      {0, 0}, {1, 1}, {{{0, 1}, 1}}, false});
  // FIXTURE 4 : requete PRECOCE (indice 0) et NON-REQUETE tardive (indice 1)
  // du meme lot, k=3 — l'abstraction du « fallback precoce, fast tardif » de
  // l'audit. La possession garde la paire Q--R ; stage-query-sequentially la
  // perd (la requete part avant le staging du candidat) et casse d'abord
  // l'identite de preflight.
  fixtures.push_back({"la requete precoce et la non-requete tardive", 3, 16,
                      {{0, 1, 2, 10, 11, 12}, {3, 4, 5, 10, 11, 12}},
                      {0, 0}, {1, 0}, {{{0, 1}, 1}}, false});
  // FIXTURE 5 : paire R--R du meme lot, k=2 — AUCUNE extremite requete : la
  // voie prefixe ne certifie RIEN, la paire reste au sidecar principal.
  fixtures.push_back({"la paire R--R laissee au sidecar", 2, 16,
                      {{0, 1, 2}, {0, 1, 3}},
                      {0, 0}, {0, 0}, {}, false});
  // FIXTURE 6 : ANCIEN-Q -- NOUVEAU-R entre deux lots, k=3. La requete du lot
  // 0 part avant le staging du lot 1 ; le non-requete tardif n'interroge
  // jamais : la paire est l'obligation du sidecar. future-visible la certifie
  // a tort — les lots futurs doivent rester invisibles.
  fixtures.push_back({"l'ancien-Q nouveau-R entre deux lots", 3, 16,
                      {{0, 1, 2, 10, 11, 12}, {3, 4, 5, 10, 11, 12}},
                      {0, 1}, {1, 0}, {}, false});
  // FIXTURE 7 : candidat de LOT ANTERIEUR garde par la requete posterieure,
  // k=3 — la moitie « Q--R anterieur » de la regle de possession.
  fixtures.push_back({"le candidat de lot anterieur", 3, 16,
                      {{0, 1, 2, 10, 11, 12}, {3, 4, 5, 10, 11, 12}},
                      {0, 1}, {0, 1}, {{{0, 1}, 1}}, false});
  for (const GateFixture& fixture : fixtures) {
    mhgp3v::PrefixIndexReceipt receipt;
    bool preflight_ok = true;
    const int lot_count =
        1 + *std::max_element(fixture.lots.begin(), fixture.lots.end());
    const PairCounts got =
        prefix_pairs(fixture.sets, fixture.queries, fixture.lots, lot_count,
                     fixture.fixture_universe, fixture.k, mutants, &receipt, &preflight_ok);
    if (!preflight_ok)
      return kill(std::string("la fixture « ") + fixture.name + " »",
                  "identite de preflight violee : hits lus != prevus au staging");
    const std::string diff = pair_counts_diff(got, fixture.expected);
    if (!diff.empty())
      return kill(std::string("la fixture « ") + fixture.name + " »", diff);
    if (mutant_name == nullptr && fixture.expects_false_candidate &&
        receipt.false_candidates == 0)
      return kill(std::string("la fixture « ") + fixture.name + " »",
                  "la recertification n'a pas ete exercee");
  }

  // CAMPAGNE : familles d'ensembles tires, lots contigus tires, masque de
  // requetes tire, pour TOUT k : differentiel multiensembliste possede,
  // identites de masse et de preflight, permutation globale commune une
  // famille sur quatre.
  std::mt19937 rng((unsigned)seed);
  long long total_pairs = 0, total_false = 0, total_entries = 0, expected_entries = 0;
  OwnedTallies tallies;
  const int rank_ceiling = std::min(11, universe);
  const int lot_count = 3;
  for (int f = 0; f < families; ++f) {
    std::vector<std::vector<int>> sets((std::size_t)group);
    std::vector<char> queries((std::size_t)group, 0);
    std::vector<int> lot_of((std::size_t)group, 0);
    for (std::vector<int>& s : sets) {
      // Le rang est borne par l'univers : la campagne TERMINE pour tout
      // universe >= 8 — c'etait le hang hostile de l'audit 8df7ac8.
      const int rank = 1 + (int)(rng() % (unsigned)rank_ceiling);
      std::set<int> chosen;
      while ((int)chosen.size() < rank) chosen.insert((int)(rng() % (unsigned)universe));
      s.assign(chosen.begin(), chosen.end());
      const long long m = std::min((long long)max_order, (long long)s.size());
      expected_entries += m * ((long long)s.size() + 1) - m * (m + 1) / 2;
    }
    // Lots CONTIGUS en ActivationId (deux coupes), comme les lots de niveau
    // exact du fold ; masque de requetes uniforme.
    {
      int cut_a = (int)(rng() % (unsigned)(group + 1));
      int cut_b = (int)(rng() % (unsigned)(group + 1));
      if (cut_a > cut_b) std::swap(cut_a, cut_b);
      for (int s = 0; s < group; ++s) lot_of[(std::size_t)s] = s < cut_a ? 0 : (s < cut_b ? 1 : 2);
    }
    for (std::size_t s = 0; s < queries.size(); ++s) queries[s] = (rng() & 1u) != 0 ? 1 : 0;
    std::vector<PairCounts> per_order((std::size_t)max_order);
    for (int k = 1; k <= max_order; ++k) {
      mhgp3v::PrefixIndexReceipt receipt;
      bool preflight_ok = true;
      const PairCounts got = prefix_pairs(sets, queries, lot_of, lot_count, universe, k,
                                          mutants, &receipt, &preflight_ok);
      total_entries += receipt.entries;
      total_false += receipt.false_candidates;
      if (!preflight_ok)
        return kill("l'identite de preflight",
                    "famille " + std::to_string(f) + " k=" + std::to_string(k) +
                        " : hits lus != prevus au staging");
      const PairCounts want = owned_truth(sets, queries, lot_of, k, &tallies);
      const std::string diff = pair_counts_diff(got, want);
      if (!diff.empty())
        return kill("le differentiel multiensembliste",
                    "famille " + std::to_string(f) + " k=" + std::to_string(k) + " : " + diff);
      total_pairs += (long long)want.size();
      per_order[(std::size_t)(k - 1)] = got;
    }
    // PERMUTATION GLOBALE COMMUNE (une famille sur quatre) : une bijection de
    // l'univers — rare-first en est une — appliquee au staging ET aux
    // requetes rend les MEMES paires recertifiees, aux memes multiplicites.
    if (f % 4 == 0) {
      std::vector<int> sigma((std::size_t)universe);
      std::iota(sigma.begin(), sigma.end(), 0);
      std::shuffle(sigma.begin(), sigma.end(), rng);
      std::vector<std::vector<int>> renamed((std::size_t)group);
      for (std::size_t s = 0; s < sets.size(); ++s) {
        for (int x : sets[s]) renamed[s].push_back(sigma[(std::size_t)x]);
        std::sort(renamed[s].begin(), renamed[s].end());
      }
      for (int k = 1; k <= max_order; ++k) {
        mhgp3v::PrefixIndexReceipt renamed_receipt;
        const PairCounts renamed_pairs = prefix_pairs(renamed, queries, lot_of, lot_count,
                                                      universe, k, mutants, &renamed_receipt,
                                                      nullptr);
        const std::string diff =
            pair_counts_diff(renamed_pairs, per_order[(std::size_t)(k - 1)]);
        if (!diff.empty())
          return kill("la permutation globale commune",
                      "famille " + std::to_string(f) + " k=" + std::to_string(k) + " : " + diff);
      }
    }
  }
  if (total_entries != expected_entries)
    return kill("l'identite de masse",
                std::to_string(total_entries) + " entrees contre L(r,K) somme " +
                    std::to_string(expected_entries));
  if (total_pairs < min_pairs || tallies.qq < min_qq || tallies.prior < min_prior ||
      tallies.later < min_later || tallies.sidecar < min_rr) {
    std::printf("ECHEC : plancher non atteint (paires %lld >= %lld, Q--Q %lld >= %lld,"
                " anterieur %lld >= %lld, posterieur %lld >= %lld, sidecar %lld >= %lld"
                " exiges)\n",
                total_pairs, min_pairs, tallies.qq, min_qq, tallies.prior, min_prior,
                tallies.later, min_later, tallies.sidecar, min_rr);
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
  std::printf("OK : voie prefixe == paires possedees du join quadratique sur %d familles x %d"
              " ordres (%lld paires : Q--Q=%lld anterieur=%lld posterieur=%lld ;"
              " sidecar=%lld ; %lld faux candidats refuses ; masse d'index %lld == L(r,K))\n",
              families, max_order, total_pairs, tallies.qq, tallies.prior, tallies.later,
              tallies.sidecar, total_false, total_entries);
  return 0;
}
