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

namespace {

// Digest 64 bits independant de l'ordre : XOR de hachages par cluster, par
// niveau, par ordre. Un falsificateur compact, pas une egalite exacte.
unsigned long long fold_digest(const mhgp3v::SaturatedFold& fold) {
  unsigned long long digest = 1469598103934665603ULL;
  for (const mhgp3v::SaturatedOrderFold& order : fold.orders) {
    unsigned long long order_digest = 0;
    for (std::size_t li = 0; li < order.closed_partitions.size(); ++li)
      for (const std::vector<mhgp::i32>& cluster : order.closed_partitions[li]) {
        unsigned long long h = 14695981039346656037ULL + li;
        for (mhgp::i32 z : cluster) { h ^= (unsigned long long)z + 0x9e3779b9; h *= 1099511628211ULL; }
        order_digest ^= h;
      }
    digest = (digest ^ order_digest) * 1099511628211ULL;
  }
  return digest;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 200, coord = 0, smax = 11, max_order = 5;
  long long seed = 20260810;
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
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--max-order")) target = &max_order;
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (n < 5 || n > 100000 || coord < 0 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || max_order < 1 || max_order + 1 > smax) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
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
  const mhgp3v::SaturatedFold fold =
      mhgp3v::build_saturated_fold(catalogue, max_order, /*keep_partitions=*/false);
  const auto t2 = std::chrono::steady_clock::now();
  if (!fold.ok) { std::printf("ECHEC : fold refuse : %s\n", fold.refusal); return 3; }

  const double catalogue_seconds = std::chrono::duration<double>(t1 - t0).count();
  const double fold_seconds = std::chrono::duration<double>(t2 - t1).count();
  long long total_levels = 0, total_births = 0, total_fusions = 0;
  long long total_growth = 0, total_silent = 0, total_comparisons = 0, total_unions = 0;
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
  }
  std::printf("provenance : --points %d --coord %d --smax %d --max-order %d --seed %lld\n",
              n, coord, smax, max_order, seed);
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
  std::printf("jointure   : comparaisons=%lld unions=%lld  |S| max=%zu  (forme de verite"
              " O(G^2) par lot — c'est le mur mesure, le join par postings est le"
              " prochain travail)\n", total_comparisons, total_unions, max_generator);
  std::printf("           : les lots silencieux sont un etat interne persistant, JAMAIS des"
              " continuations Gamma (audit live du pipeline)\n");
  std::printf("temps      : catalogue %.3f s, fold %.3f s, total %.3f s\n", catalogue_seconds,
              fold_seconds, catalogue_seconds + fold_seconds);
  return 0;
}
