// MorseHGP3D v3 — LA PORTE DU JOIN PAR POSTINGS.
//
// `build_saturated_fold_postings` est la FORME D'ECHELLE du fold sature
// (NOTE_SOLUTION_JOIN_POSTINGS_50K_20260810) ; `build_saturated_fold` est la
// FORME DE VERITE O(G^2). Cette porte les DIFFERENCIE bit a bit — niveaux,
// partitions fermees, transcript (naissances/fusions/croissances/silencieux) —
// et verifie les identites combinatoires du recu :
//
//   somme_x |P_x|      == somme des |S|          (masse des postings),
//   somme_x C(|P_x|,2) == somme des poids reduits (P_post),
//   occurrences emises == somme des poids, par lot,
//   unions reussies    == somme des unions du fold de verite (l'ordre des
//                         unions ne change pas le nombre de fusions).
//
// TROIS ETAGES :
//   1. les FIXTURES NOMMEES de la note §6, avec transcript attendu GRAVE —
//      elles contredisent la porte elle-meme si la semantique Q1.2 change ;
//   2. une CAMPAGNE de nuages aleatoires (flat_catalogue), differentiel
//      complet avec partitions, et INVARIANCE PAR PERMUTATION du catalogue ;
//   3. les MUTANTS nommes (--mutant) : chacun doit etre TUE par une fixture,
//      le differentiel ou une identite de recu — sortie 4 « mutant tue »,
//      sortie 0 = mutant survivant, et le CTest a code attendu 4 rougit.
//
// Codes : 0 OK ; 1 desaccord ou fixture contredite ; 2 CLI ; 3 plancher ou
// nuage ; 4 mutant tue (seul code attendu sous --mutant).

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <charconv>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/saturated_fold.hpp"
#include "prototype/saturated_fold_global.hpp"

namespace {

// ---------------------------------------------------------------------------
// Catalogue synthetique : une sphere fabriquee par generateur, de niveau exact
// L^2 (base 0, num = (L,0,0), den = 1) — `sphere_cmp_beta` ne lit que num/den,
// donc des L croissants donnent des niveaux exacts croissants et des L egaux
// un MEME lot. Les fixtures de la note sont des ensembles abstraits ; c'est la
// representation minimale qui les realise dans l'API du fold.
// ---------------------------------------------------------------------------
struct SyntheticGenerator {
  int level;
  std::vector<mhgp::i32> members;   // tries, sans doublon
};

mhgp::Catalogue make_catalogue(const std::vector<SyntheticGenerator>& generators) {
  mhgp::Catalogue catalogue;
  for (const SyntheticGenerator& g : generators) {
    mhgp::CriticalSphere sphere{};
    sphere.n_support = 1;
    sphere.support[0] = g.members.empty() ? 0 : g.members.front();
    sphere.rank = (mhgp::i32)g.members.size();
    sphere.members_begin = (mhgp::i32)catalogue.members.size();
    sphere.sph = mhgp::Sphere{mhgp::P3{0, 0, 0}, g.level, 0, 0, 1, 1};
    sphere.beta = (double)g.level * (double)g.level;
    catalogue.members.insert(catalogue.members.end(), g.members.begin(), g.members.end());
    catalogue.spheres.push_back(sphere);
  }
  return catalogue;
}

// ---------------------------------------------------------------------------
// Comparaison exacte des deux folds. Les compteurs de jointure de la verite
// (join_comparisons) ne sont pas compares : ils mesurent l'algorithme, pas la
// semantique. Les representants de niveau sont compares par INDICE catalogue
// sauf si `ignore_representatives` (le test de permutation les renomme).
// ---------------------------------------------------------------------------
bool folds_agree(const mhgp3v::SaturatedFold& truth, const mhgp3v::SaturatedFold& candidate,
                 bool ignore_representatives, std::string* why) {
  if (!truth.ok) { *why = std::string("fold de verite refuse : ") + truth.refusal; return false; }
  if (!candidate.ok) { *why = std::string("fold candidat refuse : ") + candidate.refusal; return false; }
  if (truth.orders.size() != candidate.orders.size()) { *why = "nombre d'ordres"; return false; }
  for (std::size_t idx = 0; idx < truth.orders.size(); ++idx) {
    const mhgp3v::SaturatedOrderFold& a = truth.orders[idx];
    const mhgp3v::SaturatedOrderFold& b = candidate.orders[idx];
    const std::string k = "ordre k=" + std::to_string(idx + 1) + " : ";
    if (a.births != b.births) { *why = k + "naissances " + std::to_string(a.births) + " != " + std::to_string(b.births); return false; }
    if (a.fusions != b.fusions) { *why = k + "fusions " + std::to_string(a.fusions) + " != " + std::to_string(b.fusions); return false; }
    if (a.coverage_growth_batches != b.coverage_growth_batches) { *why = k + "croissances " + std::to_string(a.coverage_growth_batches) + " != " + std::to_string(b.coverage_growth_batches); return false; }
    if (a.silent_generator_batches != b.silent_generator_batches) { *why = k + "lots silencieux " + std::to_string(a.silent_generator_batches) + " != " + std::to_string(b.silent_generator_batches); return false; }
    if (a.gamma_births != b.gamma_births) { *why = k + "naissances Gamma " + std::to_string(a.gamma_births) + " != " + std::to_string(b.gamma_births); return false; }
    if (a.gamma_continuations != b.gamma_continuations) { *why = k + "continuations Gamma " + std::to_string(a.gamma_continuations) + " != " + std::to_string(b.gamma_continuations); return false; }
    if (a.gamma_multifusions != b.gamma_multifusions) { *why = k + "multifusions Gamma " + std::to_string(a.gamma_multifusions) + " != " + std::to_string(b.gamma_multifusions); return false; }
    if (a.event_guard_violations != b.event_guard_violations) { *why = k + "violations de garde " + std::to_string(a.event_guard_violations) + " != " + std::to_string(b.event_guard_violations); return false; }
    if (a.gamma_birth_at_level != b.gamma_birth_at_level ||
        a.gamma_continuation_at_level != b.gamma_continuation_at_level ||
        a.gamma_multifusion_at_level != b.gamma_multifusion_at_level) { *why = k + "transcript Gamma par niveau"; return false; }
    if (ignore_representatives) {
      // Permutation : les representants sont renumerotes — comparer les
      // records SANS leur indice de niveau, l'ordre (niveau, temoin) restant
      // canonique de part et d'autre.
      if (a.gamma_records.size() != b.gamma_records.size()) { *why = k + "nombre de records Gamma"; return false; }
      for (std::size_t ri = 0; ri < a.gamma_records.size(); ++ri)
        if (a.gamma_records[ri].closed_witness != b.gamma_records[ri].closed_witness ||
            a.gamma_records[ri].strict_witnesses != b.gamma_records[ri].strict_witnesses ||
            a.gamma_records[ri].type != b.gamma_records[ri].type) { *why = k + "record Gamma " + std::to_string(ri); return false; }
    } else if (!(a.gamma_records == b.gamma_records)) {
      *why = k + "records Gamma par temoin";
      return false;
    }
    if (a.level_representative.size() != b.level_representative.size()) { *why = k + "nombre de niveaux " + std::to_string(a.level_representative.size()) + " != " + std::to_string(b.level_representative.size()); return false; }
    if (!ignore_representatives && a.level_representative != b.level_representative) { *why = k + "representants de niveau"; return false; }
    if (a.closed_partitions.size() != b.closed_partitions.size()) { *why = k + "nombre de partitions"; return false; }
    for (std::size_t li = 0; li < a.closed_partitions.size(); ++li)
      if (a.closed_partitions[li] != b.closed_partitions[li]) { *why = k + "partition fermee au niveau " + std::to_string(li); return false; }
  }
  return true;
}

// Les identites de recu que le fold ne peut pas verifier seul : la somme des
// unions de la verite (l'ordre des unions ne change jamais leur NOMBRE).
bool receipt_agrees(const mhgp3v::SaturatedFold& truth, const mhgp3v::PostingsReceipt& receipt,
                    long long expected_postings_mass, std::string* why) {
  if (!receipt.identities_ok) { *why = "identites internes du recu violees"; return false; }
  if (receipt.postings_mass != expected_postings_mass) {
    *why = "masse des postings " + std::to_string(receipt.postings_mass) + " != somme des |S| " + std::to_string(expected_postings_mass);
    return false;
  }
  if (receipt.old_new_occurrences + receipt.new_new_occurrences != receipt.weight_sum) {
    *why = "occurrences != somme des poids";
    return false;
  }
  long long truth_unions = 0;
  for (const mhgp3v::SaturatedOrderFold& order : truth.orders) truth_unions += order.join_unions;
  if (receipt.unions_done != truth_unions) {
    *why = "unions " + std::to_string(receipt.unions_done) + " != unions de la verite " + std::to_string(truth_unions);
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// LES FIXTURES NOMMEES (note §6). Le transcript attendu est GRAVE : si le fold
// de verite s'en ecarte, c'est la porte qui est contredite, pas le candidat.
// ---------------------------------------------------------------------------
struct ExpectedOrder {
  long long births, fusions, growth, silent, levels;
};

struct Fixture {
  const char* name;
  int maximum_order;
  std::vector<SyntheticGenerator> generators;
  std::vector<ExpectedOrder> expected;   // par k-1
};

std::vector<Fixture> named_fixtures() {
  std::vector<Fixture> fixtures;
  // Ancien--nouveau ET nouveau--nouveau dans un meme lot : oublier les paires
  // nouveau--nouveau isole C ; decaler le seuil (w > k) isole B et C.
  fixtures.push_back({"ancien_nouveau_nouveau_nouveau", 2,
                      {{1, {0, 1, 2}}, {2, {1, 2, 3}}, {2, {2, 3, 4}}},
                      {{1, 0, 1, 0, 2}, {1, 0, 1, 0, 2}}});
  // Le silencieux indispensable : S n'ajoute ni composante ni couverture, mais
  // N ne rejoint la composante QUE par S. Le collecter perd l'attache.
  fixtures.push_back({"silencieux_indispensable", 2,
                      {{1, {1, 2, 3}}, {2, {2, 3, 4}}, {3, {1, 3, 4}}, {4, {1, 4, 5}}},
                      {{1, 0, 2, 1, 4}, {1, 0, 2, 1, 4}}});
  // La multifusion : S rejoint DEUX racines strictes — une seule multifusion,
  // jamais une continuation dependante de l'ordre des unions.
  fixtures.push_back({"multifusion", 2,
                      {{1, {0, 1}}, {2, {4, 5}}, {3, {0, 1, 4, 5}}},
                      {{2, 1, 0, 0, 3}, {2, 1, 0, 0, 3}}});
  // Le recouvrement au-dela de K+1 : |A inter B| = 5, invisible si les membres
  // sont tronques a K+1 = 4 (interdit par la note §1).
  fixtures.push_back({"recouvrement_au_dela_de_K_plus_1", 3,
                      {{1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}}, {2, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}}},
                      {{1, 0, 1, 0, 2}, {1, 0, 1, 0, 2}, {1, 0, 1, 0, 2}}});
  // Le lot de niveau egal a deux generateurs : committe atomiquement c'est UNE
  // multifusion en TROIS niveaux ; committe sequentiellement, le transcript et
  // le nombre de niveaux changent.
  fixtures.push_back({"lot_egal_multifusion", 2,
                      {{1, {0, 1}}, {2, {4, 5}}, {3, {0, 1, 4}}, {3, {1, 4, 5}}},
                      {{2, 1, 0, 0, 3}, {2, 1, 0, 0, 3}}});
  // La posting du dernier generateur du lot : l'omettre fait naitre B orphelin.
  fixtures.push_back({"posting_du_dernier", 2,
                      {{1, {0, 1, 2}}, {2, {0, 1, 3}}},
                      {{1, 0, 1, 0, 2}, {1, 0, 1, 0, 2}}});
  // Les PointId clairsemes — la sonde EXACTE de l'audit live, verifiee par
  // l'auditeur sous ASan/UBSan : la compression dense des joins postings doit
  // RETRADUIRE partitions, temoins et satures marquants avant le payload
  // public (temoin ferme {10,1000}, temoin strict {10,INT_MAX} a k=2) — la
  // verite G^2, non compressee, les compare bit a bit.
  fixtures.push_back({"identifiants_clairsemes", 2,
                      {{1, {10, 2147483647}},
                       {2, {10, 1000, 2147483647}},
                       {3, {1000, 2147483647}}},
                      {{1, 0, 1, 1, 3}, {1, 0, 1, 1, 3}}});
  return fixtures;
}

long long catalogue_members_mass(const mhgp::Catalogue& catalogue) {
  long long mass = 0;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) mass += sphere.rank;
  return mass;
}

// L'ORACLE INDEPENDANT DES POIDS (audit 621ee80 §5) : la table canonique
// (M, N) -> |M inter N| est reconstruite DIRECTEMENT des listes de membres —
// jamais de l'emission — et comparee clef par clef et poids par poids au dump
// du recu ; les masses ancien--nouveau et nouveau--nouveau sont recalculees
// des degres pre-lot. Une redistribution compensee des poids entre paires,
// au-dessus des memes seuils, preserverait masses, unions et partitions : elle
// ne peut pas preserver cette table.
bool independent_weights_agree(const mhgp::Catalogue& catalogue,
                               const mhgp3v::PostingsReceipt& receipt, std::string* why) {
  const std::size_t count = catalogue.spheres.size();
  std::vector<std::vector<mhgp::i32>> members(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    members[s].assign(catalogue.members.begin() + sphere.members_begin,
                      catalogue.members.begin() + sphere.members_begin + sphere.rank);
  }
  std::vector<int> by_level(count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });
  // Masses depuis les degres pre-lot, par lots de niveau exact.
  std::map<mhgp::i32, long long> degree;
  long long old_new = 0, new_new = 0;
  std::size_t cursor = 0;
  while (cursor < count) {
    std::size_t batch_end = cursor + 1;
    while (batch_end < count &&
           mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                                 catalogue.spheres[(std::size_t)by_level[batch_end]].sph) == 0)
      ++batch_end;
    std::map<mhgp::i32, long long> batch_count;
    for (std::size_t b = cursor; b < batch_end; ++b)
      for (mhgp::i32 x : members[(std::size_t)by_level[b]]) {
        const auto it = degree.find(x);
        old_new += it == degree.end() ? 0 : it->second;
        ++batch_count[x];
      }
    for (const auto& entry : batch_count) {
      new_new += entry.second * (entry.second - 1) / 2;
      degree[entry.first] += entry.second;
    }
    cursor = batch_end;
  }
  if (old_new != receipt.old_new_occurrences || new_new != receipt.new_new_occurrences) {
    *why = "oracle des poids : masses ancien/nouveau " + std::to_string(old_new) + "/" +
           std::to_string(new_new) + " != recu " +
           std::to_string(receipt.old_new_occurrences) + "/" +
           std::to_string(receipt.new_new_occurrences);
    return false;
  }
  // La table complete, par intersection de listes triees SANS sortie precoce.
  std::map<std::pair<int, int>, long long> table;
  for (std::size_t s = 0; s < count; ++s)
    for (std::size_t t = s + 1; t < count; ++t) {
      std::size_t i = 0, j = 0;
      long long weight = 0;
      while (i < members[s].size() && j < members[t].size()) {
        if (members[s][i] < members[t][j]) ++i;
        else if (members[t][j] < members[s][i]) ++j;
        else { ++weight; ++i; ++j; }
      }
      if (weight > 0) table[{(int)s, (int)t}] = weight;
    }
  if ((long long)table.size() != receipt.reduced_pairs) {
    *why = "oracle des poids : " + std::to_string(table.size()) + " paires != recu " +
           std::to_string(receipt.reduced_pairs);
    return false;
  }
  std::vector<std::pair<std::pair<int, int>, long long>> dump = receipt.pairs;
  std::sort(dump.begin(), dump.end());
  std::size_t index = 0;
  for (const auto& entry : table) {
    if (index >= dump.size() || dump[index].first != entry.first ||
        dump[index].second != entry.second) {
      *why = "oracle des poids : clef ou poids divergent a l'entree " + std::to_string(index);
      return false;
    }
    ++index;
  }
  return true;
}

// Un tour complet sur un catalogue : verite, candidat, differentiel, recu.
// Rend faux avec `why` au premier ecart. Sous mutant, un ecart est un SUCCES
// pour l'appelant --mutant.
bool run_differential(const mhgp::Catalogue& catalogue, int maximum_order,
                      mhgp3v::PostingsMutants mutants, const ExpectedOrder* expected,
                      std::size_t expected_count, std::string* why) {
  const mhgp3v::SaturatedFold truth = mhgp3v::build_saturated_fold(
      catalogue, maximum_order, /*keep_partitions=*/true, /*enforce_event_guard=*/true);
  if (!truth.ok) { *why = std::string("fold de verite refuse : ") + truth.refusal; return false; }
  if (expected != nullptr) {
    if (truth.orders.size() != expected_count) { *why = "fixture contredite : nombre d'ordres"; return false; }
    for (std::size_t idx = 0; idx < expected_count; ++idx) {
      const mhgp3v::SaturatedOrderFold& order = truth.orders[idx];
      const ExpectedOrder& want = expected[idx];
      if (order.births != want.births || order.fusions != want.fusions ||
          order.coverage_growth_batches != want.growth ||
          order.silent_generator_batches != want.silent ||
          (long long)order.level_representative.size() != want.levels) {
        *why = "fixture contredite par la verite a k=" + std::to_string(idx + 1) +
               " : naissances=" + std::to_string(order.births) +
               " fusions=" + std::to_string(order.fusions) +
               " croissances=" + std::to_string(order.coverage_growth_batches) +
               " silencieux=" + std::to_string(order.silent_generator_batches) +
               " niveaux=" + std::to_string(order.level_representative.size());
        return false;
      }
    }
  }
  mhgp3v::PostingsReceipt receipt;
  const mhgp3v::SaturatedFold candidate = mhgp3v::build_saturated_fold_postings(
      catalogue, maximum_order, /*keep_partitions=*/true, &receipt, mutants,
      /*enforce_event_guard=*/true, /*collect_pairs=*/true);
  if (!candidate.ok) { *why = std::string("fold candidat refuse : ") + candidate.refusal; return false; }
  if (!folds_agree(truth, candidate, /*ignore_representatives=*/false, why)) return false;
  if (!receipt_agrees(truth, receipt, catalogue_members_mass(catalogue), why)) return false;
  if (!independent_weights_agree(catalogue, receipt, why)) return false;

  // LA FORME GLOBALE (note §3, GPU-reprenable et repli multi-coeurs) : meme
  // fold bit a bit que la verite, recu champ a champ egal a la forme par lots
  // — y compris la table (M,N)->w — a UN thread ET a DEUX threads (contrat a
  // deux digests du repli multi-coeurs : T n'influe pas sur le resultat).
  for (int threads = 1; threads <= 2; ++threads) {
    mhgp3v::PostingsReceipt global_receipt;
    const mhgp3v::SaturatedFold global = mhgp3v::build_saturated_fold_postings_global(
        catalogue, maximum_order, /*keep_partitions=*/true, &global_receipt, threads,
        /*enforce_event_guard=*/true, /*collect_pairs=*/true);
    if (!global.ok) {
      *why = std::string("fold global refuse (threads=") + std::to_string(threads) + ") : " +
             global.refusal;
      return false;
    }
    if (!folds_agree(truth, global, /*ignore_representatives=*/false, why)) {
      *why = "forme globale (threads=" + std::to_string(threads) + ") : " + *why;
      return false;
    }
    if (global_receipt.old_new_occurrences != receipt.old_new_occurrences ||
        global_receipt.new_new_occurrences != receipt.new_new_occurrences ||
        global_receipt.reduced_pairs != receipt.reduced_pairs ||
        global_receipt.weight_sum != receipt.weight_sum ||
        global_receipt.p_post != receipt.p_post ||
        global_receipt.postings_mass != receipt.postings_mass ||
        global_receipt.max_posting != receipt.max_posting ||
        global_receipt.unions_attempted != receipt.unions_attempted ||
        global_receipt.unions_done != receipt.unions_done ||
        global_receipt.pairs != receipt.pairs) {
      *why = "recu de la forme globale (threads=" + std::to_string(threads) +
             ") divergent de la forme par lots";
      return false;
    }
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  int clouds = 0, points = 8, coord = 40, smax = 11, maximum_order = 3;
  long long seed = 20260810;
  long long minimum_generators = 0, minimum_unions = 0, minimum_silent = 0, minimum_levels = 0;
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
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--clouds")) target = &clouds;
    else if (!strcmp(argv[i], "--points")) target = &points;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--max-order")) target = &maximum_order;
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else if (!strcmp(argv[i], "--min-generators")) wide = &minimum_generators;
    else if (!strcmp(argv[i], "--min-unions")) wide = &minimum_unions;
    else if (!strcmp(argv[i], "--min-silent")) wide = &minimum_silent;
    else if (!strcmp(argv[i], "--min-levels")) wide = &minimum_levels;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (clouds < 0 || clouds > 1000 || points < 5 || points > 512 || coord < 2 ||
      coord > 65536 || smax < 2 || smax > mhgp::kMaxRank || maximum_order < 1 ||
      maximum_order + 1 > smax) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  mhgp3v::PostingsMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "strict-threshold")) mutants.strict_threshold = true;
    else if (!strcmp(mutant_name, "skip-new-new")) mutants.skip_new_new = true;
    else if (!strcmp(mutant_name, "omit-last-posting")) mutants.omit_last_posting = true;
    else if (!strcmp(mutant_name, "truncate-membership")) mutants.truncate_membership = true;
    else if (!strcmp(mutant_name, "collect-silent")) mutants.collect_silent = true;
    else if (!strcmp(mutant_name, "sequential-equal-levels")) mutants.sequential_equal_levels = true;
    else { std::printf("ECHEC : mutant inconnu %s\n", mutant_name); return 2; }
  }

  // ETAGE 1 : les fixtures nommees. Sous mutant, le premier ecart TUE.
  for (const Fixture& fixture : named_fixtures()) {
    std::string why;
    const bool agree = run_differential(make_catalogue(fixture.generators), fixture.maximum_order,
                                        mutants, fixture.expected.data(), fixture.expected.size(),
                                        &why);
    if (!agree) {
      if (mutant_name != nullptr && why.rfind("fixture contredite", 0) != 0) {
        std::printf("mutant tue par la fixture %s : %s\n", fixture.name, why.c_str());
        return 4;
      }
      std::printf("ECHEC fixture %s : %s\n", fixture.name, why.c_str());
      return 1;
    }
  }
  std::printf("fixtures   : 7/7 en accord verite==postings, transcripts graves respectes\n");

  // ETAGE 2 : la campagne differentielle sur nuages reels.
  long long processed = 0, skipped = 0;
  long long total_generators = 0, total_unions = 0, total_silent = 0, total_levels = 0;
  long long total_p_post = 0, total_weight = 0;
  std::size_t max_posting = 0;
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  for (int c = 0; c < clouds; ++c) {
    std::vector<mhgp::P3> pts;
    std::set<long long> keys;
    for (int guard = 0; (int)pts.size() < points && guard < 200 * points; ++guard) {
      mhgp::P3 q{};
      q.x = (mhgp::i32)pick(rng);
      q.y = (mhgp::i32)pick(rng);
      q.z = (mhgp::i32)pick(rng);
      const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
      if (!keys.insert(key).second) continue;
      pts.push_back(q);
    }
    if ((int)pts.size() < points) { ++skipped; continue; }
    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
    if (status != mhgp3v::CloudStatus::kOk) { ++skipped; continue; }

    std::string why;
    if (!run_differential(catalogue, maximum_order, mutants, nullptr, 0, &why)) {
      if (mutant_name != nullptr) {
        std::printf("mutant tue par la campagne, nuage %d : %s\n", c, why.c_str());
        return 4;
      }
      std::printf("ECHEC campagne, nuage %d : %s\n", c, why.c_str());
      return 1;
    }

    // INVARIANCE PAR PERMUTATION (note §7) : renverser l'ordre du catalogue ne
    // doit changer ni partitions ni transcript ni recu — seul le tie-break
    // interne des lots bouge. Les representants sont renumerotes, donc ignores.
    if (c == 0) {
      mhgp::Catalogue reversed;
      for (std::size_t s = catalogue.spheres.size(); s-- > 0;) {
        mhgp::CriticalSphere sphere = catalogue.spheres[s];
        const mhgp::i32 begin = sphere.members_begin;
        sphere.members_begin = (mhgp::i32)reversed.members.size();
        reversed.members.insert(reversed.members.end(), catalogue.members.begin() + begin,
                                catalogue.members.begin() + begin + sphere.rank);
        reversed.spheres.push_back(sphere);
      }
      mhgp3v::PostingsReceipt straight, mirrored;
      const mhgp3v::SaturatedFold a = mhgp3v::build_saturated_fold_postings(
          catalogue, maximum_order, true, &straight, mutants);
      const mhgp3v::SaturatedFold b = mhgp3v::build_saturated_fold_postings(
          reversed, maximum_order, true, &mirrored, mutants);
      std::string permutation_why;
      const bool invariant =
          a.ok && b.ok && folds_agree(a, b, /*ignore_representatives=*/true, &permutation_why) &&
          straight.reduced_pairs == mirrored.reduced_pairs &&
          straight.weight_sum == mirrored.weight_sum && straight.p_post == mirrored.p_post &&
          straight.unions_done == mirrored.unions_done &&
          straight.old_new_occurrences == mirrored.old_new_occurrences;
      if (!invariant) {
        if (permutation_why.empty()) permutation_why = "recu non invariant";
        if (mutant_name != nullptr) {
          std::printf("mutant tue par la permutation : %s\n", permutation_why.c_str());
          return 4;
        }
        std::printf("ECHEC permutation : %s\n", permutation_why.c_str());
        return 1;
      }
    }

    mhgp3v::PostingsReceipt receipt;
    const mhgp3v::SaturatedFold fold = mhgp3v::build_saturated_fold_postings(
        catalogue, maximum_order, false, &receipt, mutants);
    if (!fold.ok) {
      if (mutant_name != nullptr) {
        std::printf("mutant tue par le recu, nuage %d : %s\n", c, fold.refusal);
        return 4;
      }
      std::printf("ECHEC recu, nuage %d : %s\n", c, fold.refusal);
      return 1;
    }
    ++processed;
    total_generators += (long long)catalogue.spheres.size();
    total_unions += receipt.unions_done;
    total_p_post += receipt.p_post;
    total_weight += receipt.weight_sum;
    max_posting = std::max(max_posting, receipt.max_posting);
    for (const mhgp3v::SaturatedOrderFold& order : fold.orders) {
      total_silent += order.silent_generator_batches;
      total_levels += (long long)order.level_representative.size();
    }
  }

  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }

  if (clouds > 0) {
    std::printf("campagne   : %lld nuages differencies (%lld refuses), generateurs=%lld\n",
                processed, skipped, total_generators);
    std::printf("recus      : P_post=%lld poids=%lld unions=%lld silencieux=%lld"
                " niveaux=%lld |P_x| max=%zu\n",
                total_p_post, total_weight, total_unions, total_silent, total_levels, max_posting);
    if (total_generators < minimum_generators || total_unions < minimum_unions ||
        total_silent < minimum_silent || total_levels < minimum_levels || processed < 1) {
      std::printf("ECHEC : plancher non atteint (generateurs>=%lld unions>=%lld"
                  " silencieux>=%lld niveaux>=%lld)\n",
                  minimum_generators, minimum_unions, minimum_silent, minimum_levels);
      return 3;
    }
  }
  std::printf("OK : join postings == fold de verite, identites de recu respectees\n");
  return 0;
}
