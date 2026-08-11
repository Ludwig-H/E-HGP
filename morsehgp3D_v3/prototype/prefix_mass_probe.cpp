// MorseHGP3D v3 — LA SONDE DE MASSE DE L'INDEX PREFIXE--PREFIXE.
//
// Deux masques (audit 8df7ac8, P0 « préflighter les hits ») :
//
//   --mask all    : le DIAGNOSTIC borne superieure historique — un lot unique,
//                   toutes les requetes posees. H_all = somme_x d_x^2 ; les
//                   occurrences self valent exactement L_k ; le flux dirige
//                   hors self vaut H_all - L_k.
//   --mask hybrid : le VRAI masque du fold — staging par lot de niveau exact,
//                   requetes = les seuls fallback du fold (memes aides
//                   partagees : certificat principal, decision de requete,
//                   possession canonique), preflight fige a la cloture du
//                   staging de chaque lot, et percentiles p50/p95/p99/max de
//                   H_Q par requete et par lot. C'est le recu qui dimensionne
//                   les chunks device et l'admissibilite du multiway merge.
//
// C'EST UN DIAGNOSTIC DE MASSE, PAS UN VERDICT : aucune semantique de fold
// n'est rejouee (aucune union), et les identites publiees ne certifient que
// les masses. La variante a seuil t (mask all) : prefixes de longueur r-k+t,
// candidats de multiplicite >= t seulement ; t=1 est la baseline du fold.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/parallel_catalogue.hpp"
#include "prototype/prefix_index.hpp"
#include "prototype/saturated_fold_hybrid.hpp"

namespace {

struct Percentiles {
  long long p50 = 0, p95 = 0, p99 = 0, max = 0;
};

Percentiles percentiles_of(std::vector<long long>* values) {
  Percentiles out;
  if (values->empty()) return out;
  std::sort(values->begin(), values->end());
  const auto at = [&](double p) {
    const std::size_t index = (std::size_t)((double)(values->size() - 1) * p + 0.5);
    return (*values)[std::min(index, values->size() - 1)];
  };
  out.p50 = at(0.50);
  out.p95 = at(0.95);
  out.p99 = at(0.99);
  out.max = values->back();
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 200, coord = 0, smax = 11, max_order = 5, catalogue_threads = 1, threshold = 1;
  long long seed = 20260810;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kUniform;
  bool hybrid_mask = false;
  // FACTORISATION DES EX AEQUO (reponse Q3) : les requetes ne lisent que les
  // lots anterieurs — le lot est stage a sa cloture. La masse mesuree est
  // celle du chemin autoritatif sous fermeture des carriers.
  bool factorise = false;
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
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else if (!strcmp(argv[i], "scanline_single_pass"))
        family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (!strcmp(argv[i], "scanline_overlap_multiecho"))
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--mask")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --mask\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "all")) hybrid_mask = false;
      else if (!strcmp(argv[i], "hybrid")) hybrid_mask = true;
      else if (!strcmp(argv[i], "hybrid-factorised")) {
        hybrid_mask = true;
        factorise = true;
      } else { std::printf("ECHEC : masque inconnu %s\n", argv[i]); return 2; }
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--smax")) smax = (int)value;
    else if (!strcmp(argv[i], "--max-order")) max_order = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--catalogue-threads")) catalogue_threads = (int)value;
    else if (!strcmp(argv[i], "--threshold")) threshold = (int)value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 5 || n > 100000 || coord < 0 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || max_order < 1 || max_order + 1 > smax ||
      catalogue_threads < 0 || catalogue_threads > 256 || threshold < 1 ||
      threshold > mhgp::kMaxRank) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  if (hybrid_mask && threshold != 1) {
    std::printf("ECHEC : le masque hybride est la baseline t=1 du fold\n");
    return 2;
  }
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }

  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const auto t0 = std::chrono::steady_clock::now();
  const mhgp::Catalogue catalogue =
      catalogue_threads > 1
          ? mhgp3v::flat_catalogue_parallel(pts, smax, &st, &status, catalogue_threads)
          : mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
  const auto t1 = std::chrono::steady_clock::now();
  if (status != mhgp3v::CloudStatus::kOk) {
    std::printf("ECHEC : statut nuage %s\n", mhgp3v::cloud_status_name(status));
    return 3;
  }
  const std::size_t count = catalogue.spheres.size();
  std::printf("provenance : --points %d --coord %d --smax %d --max-order %d --seed %lld"
              " --family %s --catalogue-threads %d --mask %s --threshold %d\n", n, coord, smax,
              max_order, seed, mhgp3v::cloud_family_name(family), catalogue_threads,
              factorise ? "hybrid-factorised" : (hybrid_mask ? "hybrid" : "all"), threshold);
  std::printf("catalogue  : %zu generateurs en %.3f s — DIAGNOSTIC DE MASSE (%s),"
              " aucune semantique de fold\n", count,
              std::chrono::duration<double>(t1 - t0).count(),
              hybrid_mask ? "masque hybride du fold, lots de niveau exact"
                          : "tout-requete, lot unique");

  std::vector<std::vector<mhgp::i32>> members(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    if (sphere.members_begin < 0 || sphere.rank < 0 ||
        (std::size_t)sphere.members_begin + (std::size_t)sphere.rank >
            catalogue.members.size()) {
      std::printf("ECHEC : tranche de pool hors catalogue\n");
      return 3;
    }
    members[s].assign(catalogue.members.begin() + sphere.members_begin,
                      catalogue.members.begin() + sphere.members_begin + sphere.rank);
  }

  if (!hybrid_mask) {
    // ----- MASQUE ALL : borne superieure historique, variante a seuil t. -----
    for (int k = 1; k <= max_order; ++k) {
      const int t = std::min(threshold, k);
      mhgp3v::PrefixIndex index;
      index.reset(n);
      mhgp3v::PrefixIndexReceipt receipt;
      const auto s0 = std::chrono::steady_clock::now();
      for (std::size_t s = 0; s < count; ++s) {
        const int rank = (int)members[s].size();
        if (rank < k) continue;
        const int length = std::min(rank, rank - k + t);
        for (int i = 0; i < length; ++i)
          index.lists[(std::size_t)members[s][(std::size_t)i]].push_back((int)s);
        receipt.entries += length;
      }
      // CLOTURE DU STAGING : la prediction H_all = somme_x d_x^2 se lit sur
      // les degres figes — identite exigee contre les hits reellement lus.
      for (const std::vector<int>& list : index.lists)
        receipt.predicted_hits += (long long)list.size() * (long long)list.size();
      const auto s1 = std::chrono::steady_clock::now();
      std::vector<int> raw;
      std::vector<long long> hits_per_query;
      long long pairs = 0;
      for (std::size_t m = 0; m < count; ++m) {
        const int rank = (int)members[m].size();
        if (rank < k) continue;
        ++receipt.queries;
        raw.clear();
        long long query_hits = 0;
        const int length = std::min(rank, rank - k + t);
        for (int i = 0; i < length; ++i) {
          const std::vector<int>& list = index.lists[(std::size_t)members[m][(std::size_t)i]];
          query_hits += (long long)list.size();
          raw.insert(raw.end(), list.begin(), list.end());
        }
        receipt.hits += query_hits;
        hits_per_query.push_back(query_hits);
        std::sort(raw.begin(), raw.end());
        for (std::size_t i = 0; i < raw.size();) {
          std::size_t j = i;
          while (j < raw.size() && raw[j] == raw[i]) ++j;
          const int candidate = raw[i];
          const long long multiplicity = (long long)(j - i);
          i = j;
          ++receipt.candidate_ids_including_self;
          if (candidate == (int)m || multiplicity < t) continue;
          ++receipt.candidate_pairs_after_filter;
          if (mhgp3v::prefix_recertify(members[m], members[(std::size_t)candidate], k)) {
            ++receipt.recertified_true;
            ++pairs;
          } else {
            ++receipt.false_candidates;
          }
        }
      }
      const auto s2 = std::chrono::steady_clock::now();
      if (receipt.predicted_hits != receipt.hits) {
        std::printf("ECHEC : identite de preflight violee a k=%d (%lld prevus, %lld lus)\n",
                    k, receipt.predicted_hits, receipt.hits);
        return 3;
      }
      const Percentiles per_query = percentiles_of(&hits_per_query);
      std::printf("k=%d t=%d    : entrees=%lld requetes=%lld hits=%lld (prevus=%lld,"
                  " self=%lld, hors-self=%lld) candidats+self=%lld filtres=%lld"
                  " recertifies=%lld faux=%lld — stage %.3f s, requete+recertif %.3f s\n",
                  k, t, receipt.entries, receipt.queries, receipt.hits, receipt.predicted_hits,
                  receipt.entries, receipt.hits - receipt.entries,
                  receipt.candidate_ids_including_self, receipt.candidate_pairs_after_filter,
                  receipt.recertified_true, receipt.false_candidates,
                  std::chrono::duration<double>(s1 - s0).count(),
                  std::chrono::duration<double>(s2 - s1).count());
      std::printf("           : H par requete p50=%lld p95=%lld p99=%lld max=%lld\n",
                  per_query.p50, per_query.p95, per_query.p99, per_query.max);
    }
    std::printf("OK : sonde de masse terminee (bornes hautes du fallback tout-requete)\n");
    return 0;
  }

  // ----- MASQUE HYBRID : le vrai masque du fold, lots de niveau exact. -----
  // Les MEMES aides que le fold : lots, certificat principal, decision de
  // requete, possession canonique — la sonde mesure ce que le fold paiera.
  std::vector<int> by_level;
  std::vector<std::pair<std::size_t, std::size_t>> batches;
  mhgp3v::hybrid_level_batches(catalogue, &by_level, &batches);
  std::vector<int> activation_of(count);
  for (std::size_t i = 0; i < count; ++i) activation_of[(std::size_t)by_level[i]] = (int)i;
  const auto p0 = std::chrono::steady_clock::now();
  std::vector<char> principal;
  long long certificates_verified = 0, certificates_failed = 0;
  mhgp3v::hybrid_principal_certificates(pts, catalogue, members, false, &principal,
                                        &certificates_verified, &certificates_failed);
  const auto p1 = std::chrono::steady_clock::now();
  std::printf("certificats: %lld principaux, %lld non principaux en %.3f s ; %zu lots de"
              " niveau exact\n", certificates_verified, certificates_failed,
              std::chrono::duration<double>(p1 - p0).count(), batches.size());
  for (int k = 1; k <= max_order; ++k) {
    mhgp3v::PrefixIndex index;
    index.reset(n);
    mhgp3v::PrefixIndexReceipt receipt;
    std::vector<long long> staged_epoch(count, -1);
    std::vector<char> is_query_now(count, 0);
    std::vector<long long> hits_per_query;
    std::vector<long long> hits_per_batch;
    long long fallback_queries = 0, multi_generator_batches = 0, pairs = 0;
    std::size_t max_posting = 0;
    const auto s0 = std::chrono::steady_clock::now();
    std::vector<int> candidates;
    for (std::size_t batch = 0; batch < batches.size(); ++batch) {
      const long long epoch = (long long)batch + 1;
      const bool solo_batch = batches[batch].second - batches[batch].first == 1;
      std::vector<int> batch_generators;
      for (std::size_t b = batches[batch].first; b < batches[batch].second; ++b) {
        const int m = by_level[b];
        if ((int)members[(std::size_t)m].size() < k) continue;
        batch_generators.push_back(m);
        staged_epoch[(std::size_t)m] = epoch;
        is_query_now[(std::size_t)m] =
            mhgp3v::hybrid_is_fallback_query(
                (int)members[(std::size_t)m].size(), k,
                (int)catalogue.spheres[(std::size_t)m].n_support,
                principal[(std::size_t)m] != 0, solo_batch, false)
                ? 1
                : 0;
        if (!factorise)
          receipt.entries += mhgp3v::prefix_stage(&index, m, members[(std::size_t)m], k);
      }
      if (batch_generators.size() > 1) ++multi_generator_batches;
      // CLOTURE DU STAGING DU LOT : preflight fige, H_Q du lot.
      long long batch_predicted = 0;
      for (int m : batch_generators)
        if (is_query_now[(std::size_t)m] != 0)
          batch_predicted +=
              mhgp3v::prefix_predicted_hits(index, members[(std::size_t)m], k);
      receipt.predicted_hits += batch_predicted;
      if (batch_predicted > 0) hits_per_batch.push_back(batch_predicted);
      for (int m : batch_generators) {
        if (is_query_now[(std::size_t)m] == 0) continue;
        ++fallback_queries;
        const long long hits_before = receipt.hits;
        mhgp3v::prefix_query(index, members[(std::size_t)m], k, &candidates, &receipt);
        hits_per_query.push_back(receipt.hits - hits_before);
        for (int candidate : candidates) {
          if (candidate == m) continue;
          if (staged_epoch[(std::size_t)candidate] == epoch &&
              is_query_now[(std::size_t)candidate] != 0 &&
              activation_of[(std::size_t)candidate] > activation_of[(std::size_t)m])
            continue;   // possede par l'autre requete du lot
          ++receipt.candidate_pairs_after_filter;
          if (mhgp3v::prefix_recertify(members[(std::size_t)m],
                                       members[(std::size_t)candidate], k)) {
            ++receipt.recertified_true;
            ++pairs;
          } else {
            ++receipt.false_candidates;
          }
        }
      }
      if (factorise)
        for (int m : batch_generators)
          receipt.entries += mhgp3v::prefix_stage(&index, m, members[(std::size_t)m], k);
      for (int m : batch_generators) is_query_now[(std::size_t)m] = 0;
    }
    for (const std::vector<int>& list : index.lists)
      max_posting = std::max(max_posting, list.size());
    const auto s1 = std::chrono::steady_clock::now();
    if (receipt.predicted_hits != receipt.hits) {
      std::printf("ECHEC : identite de preflight violee a k=%d (%lld prevus, %lld lus)\n",
                  k, receipt.predicted_hits, receipt.hits);
      return 3;
    }
    const Percentiles per_query = percentiles_of(&hits_per_query);
    const Percentiles per_batch = percentiles_of(&hits_per_batch);
    std::printf("k=%d hybrid : entrees=%lld requetes-fallback=%lld hits=%lld (prevus=%lld)"
                " candidats+self=%lld filtres=%lld recertifies=%lld faux=%lld"
                " lots-multiples=%lld posting-max=%zu — %.3f s\n",
                k, receipt.entries, fallback_queries, receipt.hits, receipt.predicted_hits,
                receipt.candidate_ids_including_self, receipt.candidate_pairs_after_filter,
                receipt.recertified_true, receipt.false_candidates, multi_generator_batches,
                max_posting, std::chrono::duration<double>(s1 - s0).count());
    std::printf("           : H_Q par requete p50=%lld p95=%lld p99=%lld max=%lld ;"
                " par lot p50=%lld p95=%lld p99=%lld max=%lld\n",
                per_query.p50, per_query.p95, per_query.p99, per_query.max, per_batch.p50,
                per_batch.p95, per_batch.p99, per_batch.max);
  }
  std::printf("OK : sonde de masse terminee (masque hybride du fold, possession canonique)\n");
  return 0;
}
