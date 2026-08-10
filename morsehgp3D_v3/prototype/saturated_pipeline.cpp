// MorseHGP3D v3 — LE PIPELINE SATURE, CHRONOMETRE PAR ETAGE.
//
// nuage -> flat_catalogue (generateur) -> build_saturated_fold (K forets).
// C'est le binaire de MESURE du pipeline exact candidat : il publie les masses,
// les temps par etage et un digest d'ordre independant, jamais un verdict
// d'exactitude — l'exactitude se juge dans `mhgp3v_gamma_judge`, aux tailles ou
// la verite exhaustive existe.
//
// DEUX SEMANTIQUES, DECLAREES ET IMPRIMEES :
//   - s_max >= n : famille saturee COMPLETE (S.2), fold exact sous son juge ;
//   - s_max < n  : famille TRONQUEE — le fold est un RAFFINEMENT S.6
//     (`partial_refinement` : aucune fausse connexion, fusions possiblement
//     manquantes). Aucun chiffre de ce mode ne qualifie l'exactitude.
//
// La jointure du fold est la forme de VERITE O(G^2) par lot (sortie precoce au
// seuil k) : ce binaire mesure donc AUSSI le mur de la jointure, qui est le
// prochain travail d'echelle (jointure par tri, Kruskal S.5).

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/saturated_fold.hpp"
#include "prototype/saturated_fold_faceowner.hpp"
#include "prototype/saturated_fold_global.hpp"

namespace {

// Digest 64 bits DIAGNOSTIQUE, pas scientifique : hash de flux du transcript
// (compteurs et representants de niveau par ordre) puis des partitions quand
// elles existent. L'audit du profileur a montre que l'ancien digest, assis sur
// les seules partitions, devenait CONSTANT sous keep_partitions=false — il ne
// falsifiait plus rien. Celui-ci depend du transcript meme sans partitions ;
// le digest scientifique (serialisation canonique, hash de flux, provenance)
// reste un travail declare, pas celui-ci.
unsigned long long fold_digest(const mhgp3v::SaturatedFold& fold) {
  unsigned long long digest = 1469598103934665603ULL;
  const auto feed = [&digest](unsigned long long value) {
    digest ^= value + 0x9e3779b97f4a7c15ULL + (digest << 6) + (digest >> 2);
    digest *= 1099511628211ULL;
  };
  for (const mhgp3v::SaturatedOrderFold& order : fold.orders) {
    feed((unsigned long long)order.births);
    feed((unsigned long long)order.fusions);
    feed((unsigned long long)order.coverage_growth_batches);
    feed((unsigned long long)order.silent_generator_batches);
    feed((unsigned long long)order.gamma_births);
    feed((unsigned long long)order.gamma_continuations);
    feed((unsigned long long)order.gamma_multifusions);
    feed((unsigned long long)order.event_guard_violations);
    feed((unsigned long long)order.level_representative.size());
    for (int representative : order.level_representative)
      feed((unsigned long long)representative);
    for (long long value : order.gamma_birth_at_level) feed((unsigned long long)value);
    for (long long value : order.gamma_continuation_at_level) feed((unsigned long long)value);
    for (long long value : order.gamma_multifusion_at_level) feed((unsigned long long)value);
    for (std::size_t li = 0; li < order.closed_partitions.size(); ++li)
      for (const std::vector<mhgp::i32>& cluster : order.closed_partitions[li]) {
        unsigned long long h = 14695981039346656037ULL + li;
        for (mhgp::i32 z : cluster) { h ^= (unsigned long long)z + 0x9e3779b9; h *= 1099511628211ULL; }
        feed(h);
      }
  }
  return digest;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 200, coord = 0, smax = 11, max_order = 5;
  long long seed = 20260810;
  long long memory_budget_mb = 0;
  int compare_joins = 0, threads = 1;
  int join_mode = 0;   // 0 = g2, 1 = postings par lots, 2 = postings global
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(first, last, magnitude);
    if (r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--join")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --join\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "g2")) join_mode = 0;
      else if (!strcmp(argv[i], "postings")) join_mode = 1;
      else if (!strcmp(argv[i], "postings-global")) join_mode = 2;
      else if (!strcmp(argv[i], "faceowner")) join_mode = 3;
      else { std::printf("ECHEC : jointure inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--max-order")) target = &max_order;
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else if (!strcmp(argv[i], "--memory-budget-mb")) wide = &memory_budget_mb;
    else if (!strcmp(argv[i], "--compare-joins")) target = &compare_joins;
    else if (!strcmp(argv[i], "--threads")) target = &threads;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (n < 5 || n > 100000 || coord < 0 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || max_order < 1 || max_order + 1 > smax ||
      memory_budget_mb < 0 || compare_joins < 0 || compare_joins > 1 || threads < 0 ||
      threads > 256) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // Le mode compare-joins rejoue le meme catalogue par les DEUX joins : le
  // sujet mesure est une forme postings (par lots par defaut).
  if (compare_joins == 1 && join_mode == 0) join_mode = 1;
  const bool use_postings = join_mode != 0;
  // Emprise a DENSITE FIXE 1e-3 par defaut (protocole des profils d'echelle du
  // depot) : coord = cbrt(n / 1e-3), sauf surcharge explicite.
  if (coord == 0) {
    double c = std::cbrt((double)n * 1000.0);
    coord = (int)std::max(4.0, std::min(65536.0, c));
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  std::vector<mhgp::P3> pts;
  {
    std::set<long long> keys;
    for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
      mhgp::P3 q{};
      q.x = (mhgp::i32)pick(rng);
      q.y = (mhgp::i32)pick(rng);
      q.z = (mhgp::i32)pick(rng);
      const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
      if (!keys.insert(key).second) continue;
      pts.push_back(q);
    }
  }
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }

  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const auto t0 = std::chrono::steady_clock::now();
  const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
  const auto t1 = std::chrono::steady_clock::now();
  if (status != mhgp3v::CloudStatus::kOk) {
    std::printf("ECHEC : statut nuage %s\n", mhgp3v::cloud_status_name(status));
    return 3;
  }
  // La garde d'evenement ne REFUSE que sous famille complete (s_max >= n) ;
  // sous famille tronquee ses violations sont comptees et publiees.
  const bool enforce_guard = smax >= n;
  mhgp3v::PostingsReceipt receipt;
  mhgp3v::FaceOwnerReceipt faceowner_receipt;
  const mhgp3v::SaturatedFold fold =
      join_mode == 3
          ? mhgp3v::build_saturated_fold_faceowner(
                catalogue, max_order, /*keep_partitions=*/false, &faceowner_receipt,
                enforce_guard, {}, memory_budget_mb * 1048576)
      : join_mode == 2
          ? mhgp3v::build_saturated_fold_postings_global(
                catalogue, max_order, /*keep_partitions=*/false, &receipt, threads,
                enforce_guard, /*collect_pairs=*/false, memory_budget_mb * 1048576)
      : join_mode == 1
          ? mhgp3v::build_saturated_fold_postings(catalogue, max_order,
                                                  /*keep_partitions=*/false, &receipt, {},
                                                  enforce_guard, /*collect_pairs=*/false,
                                                  memory_budget_mb * 1048576)
          : mhgp3v::build_saturated_fold(catalogue, max_order,
                                         /*keep_partitions=*/false, enforce_guard);
  const auto t2 = std::chrono::steady_clock::now();
  if (!fold.ok) {
    if (use_postings && receipt.predicted_peak_bytes > 0)
      std::printf("preflight  : P_post predit=%lld, pic conservateur=%.1f Mo, plus gros"
                  " lot=%lld occurrences — manifeste du refus\n",
                  receipt.predicted_p_post, (double)receipt.predicted_peak_bytes / 1048576.0,
                  receipt.max_batch_occurrences);
    std::printf("ECHEC : fold refuse : %s\n", fold.refusal);
    return 3;
  }

  // LE MODE COMPARE-JOINS : le MEME catalogue rejoue par le fold de verite
  // O(G^2), transcript et digest compares in-process — le differentiel de
  // l'audit a n'importe quelle taille ou G^2 reste payable.
  if (compare_joins == 1) {
    const auto tc0 = std::chrono::steady_clock::now();
    const mhgp3v::SaturatedFold other = mhgp3v::build_saturated_fold(
        catalogue, max_order, /*keep_partitions=*/false, enforce_guard);
    const auto tc1 = std::chrono::steady_clock::now();
    if (!other.ok) { std::printf("ECHEC : fold G2 refuse : %s\n", other.refusal); return 3; }
    bool same = other.orders.size() == fold.orders.size();
    for (std::size_t idx = 0; same && idx < fold.orders.size(); ++idx) {
      const mhgp3v::SaturatedOrderFold& a = fold.orders[idx];
      const mhgp3v::SaturatedOrderFold& b = other.orders[idx];
      same = a.births == b.births && a.fusions == b.fusions &&
             a.coverage_growth_batches == b.coverage_growth_batches &&
             a.silent_generator_batches == b.silent_generator_batches &&
             a.gamma_births == b.gamma_births &&
             a.gamma_continuations == b.gamma_continuations &&
             a.gamma_multifusions == b.gamma_multifusions &&
             a.event_guard_violations == b.event_guard_violations &&
             a.level_representative == b.level_representative &&
             a.gamma_birth_at_level == b.gamma_birth_at_level &&
             a.gamma_continuation_at_level == b.gamma_continuation_at_level &&
             a.gamma_multifusion_at_level == b.gamma_multifusion_at_level;
    }
    std::printf("compare    : joins %s sur le meme catalogue — G2 %.3f s, digests"
                " %llu/%llu\n", same ? "IDENTIQUES" : "DIVERGENTS",
                std::chrono::duration<double>(tc1 - tc0).count(), fold_digest(fold),
                fold_digest(other));
    if (!same || fold_digest(fold) != fold_digest(other)) {
      std::printf("ECHEC : desaccord des joins sur le meme catalogue\n");
      return 1;
    }
  }

  const double catalogue_seconds = std::chrono::duration<double>(t1 - t0).count();
  const double fold_seconds = std::chrono::duration<double>(t2 - t1).count();
  long long total_levels = 0, total_births = 0, total_fusions = 0;
  long long total_growth = 0, total_silent = 0, total_comparisons = 0, total_unions = 0;
  long long gamma_births = 0, gamma_continuations = 0, gamma_multifusions = 0;
  long long guard_violations = 0;
  std::size_t max_generator = 0;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres)
    max_generator = std::max(max_generator, (std::size_t)sphere.rank);
  for (const mhgp3v::SaturatedOrderFold& order : fold.orders) {
    total_levels += (long long)order.level_representative.size();
    total_births += order.births;
    total_fusions += order.fusions;
    total_growth += order.coverage_growth_batches;
    total_silent += order.silent_generator_batches;
    total_comparisons += order.join_comparisons;
    total_unions += order.join_unions;
    gamma_births += order.gamma_births;
    gamma_continuations += order.gamma_continuations;
    gamma_multifusions += order.gamma_multifusions;
    guard_violations += order.event_guard_violations;
  }
  std::printf("provenance : --points %d --coord %d --smax %d --max-order %d --seed %lld"
              " --join %s --threads %d\n", n, coord, smax, max_order, seed,
              join_mode == 3   ? "faceowner"
              : join_mode == 2 ? "postings-global"
              : join_mode == 1 ? "postings"
                               : "g2",
              threads);
  std::printf("semantique : %s\n",
              smax >= n ? "famille saturee COMPLETE (exactitude jugee ailleurs)"
                        : "famille TRONQUEE — raffinement S.6 (partial_refinement),"
                          " AUCUNE exactitude revendiquee");
  std::printf("catalogue  : %zu generateurs, %zu membres (pool)\n", catalogue.spheres.size(),
              catalogue.members.size());
  std::printf("fold       : K=%d — niveaux=%lld naissances=%lld fusions=%lld"
              " croissances=%lld lots silencieux=%lld  digest diagnostique=%llu\n",
              max_order, total_levels, total_births, total_fusions, total_growth,
              total_silent, fold_digest(fold));
  long long forest_nodes = 0, forest_roots = 0, forest_orphans = 0;
  for (const mhgp3v::SaturatedOrderFold& order : fold.orders) {
    const mhgp3v::GammaForest gamma_forest = mhgp3v::build_gamma_forest(order.gamma_records);
    forest_nodes += (long long)gamma_forest.nodes.size();
    forest_roots += (long long)gamma_forest.roots.size();
    forest_orphans += gamma_forest.orphan_witnesses;
  }
  std::printf("forets     : K forets derivees des records — noeuds=%lld racines=%lld"
              " temoins orphelins=%lld%s\n", forest_nodes, forest_roots, forest_orphans,
              smax >= n ? "" : "  (sous-famille : orphelins possibles, payload non"
                               " autoritatif)");
  std::printf("transcript : Gamma par marquage q_min — naissances=%lld continuations=%lld"
              " multifusions=%lld violations de garde=%lld%s\n",
              gamma_births, gamma_continuations, gamma_multifusions, guard_violations,
              smax >= n ? "  (s_max >= n : censure par s_max exclue — la completude de"
                          " famille reste un certificat separe)"
                        : "  (sous-famille NON certifiee : aucun transcript autoritatif)");
  if (join_mode == 1 || join_mode == 2)
    std::printf("preflight  : P_post predit=%lld, pic conservateur=%.1f Mo, plus gros"
                " lot=%lld occurrences%s\n",
                receipt.predicted_p_post, (double)receipt.predicted_peak_bytes / 1048576.0,
                receipt.max_batch_occurrences,
                memory_budget_mb > 0 ? " (budget respecte)" : "");
  if (join_mode == 3) {
    std::printf("jointure   : face-owner (theoreme de la reponse masse) — incidences=%lld"
                " (predites=%lld) unions %lld/%lld  identites=%s\n",
                faceowner_receipt.incidences_total, faceowner_receipt.predicted_incidences,
                faceowner_receipt.unions_done, faceowner_receipt.unions_attempted,
                faceowner_receipt.identities_ok ? "respectees" : "VIOLEES");
    for (int k = 1; k <= max_order; ++k)
      std::printf("           : k=%d — G_k=%lld I_k=%lld signatures=%lld multiplicite_un=%lld"
                  " branches=%lld dedupliquees=%lld\n", k,
                  faceowner_receipt.generators_k[(std::size_t)(k - 1)],
                  faceowner_receipt.incidences_k[(std::size_t)(k - 1)],
                  faceowner_receipt.unique_signatures_k[(std::size_t)(k - 1)],
                  faceowner_receipt.multiplicity_one_k[(std::size_t)(k - 1)],
                  faceowner_receipt.star_branches_k[(std::size_t)(k - 1)],
                  faceowner_receipt.deduplicated_branches_k[(std::size_t)(k - 1)]);
  } else if (use_postings)
    std::printf("jointure   : postings — occurrences ancien/nouveau=%lld nouveau/nouveau=%lld"
                " paires reduites=%lld poids=%lld P_post=%lld unions %lld/%lld"
                " |P_x| max=%zu |S| max=%zu identites=%s\n",
                receipt.old_new_occurrences, receipt.new_new_occurrences, receipt.reduced_pairs,
                receipt.weight_sum, receipt.p_post, receipt.unions_done,
                receipt.unions_attempted, receipt.max_posting, max_generator,
                receipt.identities_ok ? "respectees" : "VIOLEES");
  else
    std::printf("jointure   : comparaisons=%lld unions=%lld  |S| max=%zu  (forme de verite"
                " O(G^2) par lot — le join par postings se demande par --join postings)\n",
                total_comparisons, total_unions, max_generator);
  std::printf("           : les lots dits silencieux MELANGENT encore continuations Gamma"
              " sans croissance et activations redondantes — la separation exacte est le"
              " predicat q_min (NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN), en cours de"
              " reception\n");
  std::printf("temps      : catalogue %.3f s, fold %.3f s, total %.3f s\n", catalogue_seconds,
              fold_seconds, catalogue_seconds + fold_seconds);
  return 0;
}
