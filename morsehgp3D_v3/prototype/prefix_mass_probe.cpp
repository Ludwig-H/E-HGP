// MorseHGP3D v3 — LA SONDE DE MASSE DE L'INDEX PREFIXE--PREFIXE.
//
// L'experience demandee par la note index avant toute nouvelle generation
// 50 k : sur un catalogue reel, par ordre k, stager TOUS les generateurs de
// rang >= k puis interroger CHACUN comme si le fallback etait tout-requete,
// et publier les masses — entrees d'index (la masse L_k de la note), hits
// bruts, candidats uniques, recertifies vrais, faux candidats — avec les
// temps par etage.
//
// C'EST UN DIAGNOSTIC DE MASSE, PAS UN VERDICT : le lot est unique (aucun
// staging par niveau), toutes les requetes sont posees (le vrai fallback est
// rare sous certificat principal), et aucune semantique de fold n'est
// rejouee. Les bornes hautes qu'il publie majorent le travail du vrai
// fallback ; elles ne certifient rien d'autre.
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

int main(int argc, char** argv) {
  int n = 200, coord = 0, smax = 11, max_order = 5, catalogue_threads = 1, threshold = 1;
  long long seed = 20260810;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kUniform;
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
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
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
              " --family %s --catalogue-threads %d\n", n, coord, smax, max_order, seed,
              mhgp3v::cloud_family_name(family), catalogue_threads);
  std::printf("catalogue  : %zu generateurs en %.3f s — DIAGNOSTIC DE MASSE tout-requete,"
              " lot unique, aucune semantique de fold\n", count,
              std::chrono::duration<double>(t1 - t0).count());

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

  // Variante a seuil t de la note : prefixes de longueur r-k+t des deux
  // cotes, seuls les candidats de multiplicite au moins t sont recertifies.
  // t = 1 est la baseline exacte de l'index du fold ; t > 1 exige t <= k.
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
    const auto s1 = std::chrono::steady_clock::now();
    std::vector<int> raw;
    long long pairs = 0;
    for (std::size_t m = 0; m < count; ++m) {
      const int rank = (int)members[m].size();
      if (rank < k) continue;
      ++receipt.queries;
      raw.clear();
      const int length = std::min(rank, rank - k + t);
      for (int i = 0; i < length; ++i) {
        const std::vector<int>& list = index.lists[(std::size_t)members[m][(std::size_t)i]];
        receipt.hits += (long long)list.size();
        raw.insert(raw.end(), list.begin(), list.end());
      }
      std::sort(raw.begin(), raw.end());
      for (std::size_t i = 0; i < raw.size();) {
        std::size_t j = i;
        while (j < raw.size() && raw[j] == raw[i]) ++j;
        const int candidate = raw[i];
        const long long multiplicity = (long long)(j - i);
        i = j;
        if (candidate == (int)m || multiplicity < t) continue;
        ++receipt.unique_candidates;
        if (mhgp3v::prefix_recertify(members[m], members[(std::size_t)candidate], k)) {
          ++receipt.recertified_true;
          ++pairs;
        } else {
          ++receipt.false_candidates;
        }
      }
    }
    const auto s2 = std::chrono::steady_clock::now();
    std::printf("k=%d t=%d    : entrees=%lld requetes=%lld hits=%lld candidats=%lld"
                " recertifies=%lld faux=%lld — stage %.3f s, requete+recertif %.3f s\n",
                k, t, receipt.entries, receipt.queries, receipt.hits,
                receipt.unique_candidates, receipt.recertified_true, receipt.false_candidates,
                std::chrono::duration<double>(s1 - s0).count(),
                std::chrono::duration<double>(s2 - s1).count());
  }
  std::printf("OK : sonde de masse terminee (bornes hautes du fallback tout-requete)\n");
  return 0;
}
