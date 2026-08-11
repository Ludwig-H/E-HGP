// MorseHGP3D v3 — LE HARNAIS DIAGNOSTIQUE NOMME `warm_e2e_h0_v3_diagnostic`.
//
// CE QUE CETTE SERIE EST : la mesure chronometree, repetee et recue du
// PIPELINE HORIZONTAL PARTIEL aujourd'hui existant — disposition Morton/LBVH
// residente partagee, lane k=1 (EMST Boruvka exact, niveaux 4*niveau=d^2),
// lane q2 (coupe Yao48 stricte fail-open, enveloppe radiale, classification
// terminale par lots, comptes census) executee par shards d'ancres sur N
// threads. Chaque repetition regenere un nuage frais (graine+rep), rejoue le
// ledger complet et publie p50/p95 par phase.
//
// CE QUE CETTE SERIE N'EST PAS (PROPOSITION, audit etat courant) : elle ne
// ferme NI le p95 principal de 100 ms NI le p95 secondaire d'une seconde du
// SLO officiel, qui portent sur `BenchmarkOutputContract-v1` (dix forets,
// applications verticales, lots, certificat minimal copies cote hote). Les
// sources q3/q4, le census materialise, le resolver, le fold et le payload
// n'y sont pas : leur absence est DITE, jamais implicite. Aucun statut public
// n'avance par cette serie.
//
// Codes : 0 OK ; 1 violation (ledger/fermeture par ancre) ; 2 CLI ;
// 3 nuage non genere.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/emst_boruvka.hpp"
#include "prototype/morton_lbvh.hpp"
#include "prototype/yao48_source.hpp"

namespace {

using i64 = long long;

double percentile(std::vector<double> values, double q) {
  std::sort(values.begin(), values.end());
  if (values.empty()) return 0.0;
  const double index = q * (double)(values.size() - 1);
  const std::size_t low = (std::size_t)index;
  const std::size_t high = low + 1 < values.size() ? low + 1 : low;
  const double frac = index - (double)low;
  return values[low] * (1.0 - frac) + values[high] * frac;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 50000, coord = 0, leaf_size = 8, reps = 5;
  int threads = (int)std::thread::hardware_concurrency();
  i64 seed = 20260811, bank_pops = 512, chamber_visits = 100000;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 1000000000000ULL) return false;
    *value = (i64)magnitude;
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
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--reps")) reps = (int)value;
    else if (!strcmp(argv[i], "--threads")) threads = (int)value;
    else if (!strcmp(argv[i], "--bank-pops")) bank_pops = value;
    else if (!strcmp(argv[i], "--chamber-visits")) chamber_visits = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 64 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || reps < 1 || reps > 100 || threads < 1 || threads > 256 ||
      bank_pops < 48 || chamber_visits < 8) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);

  std::printf("serie=warm_e2e_h0_v3_diagnostic\n");
  std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
              " --leaf-size %d --reps %d --threads %d --bank-pops %lld\n", n, coord,
              seed, mhgp3v::cloud_family_name(family), leaf_size, reps, threads,
              bank_pops);
  std::printf("PORTEE : k=1 (EMST exact) + q2 (tombstones Yao48/radial + comptes"
              " census) sur LBVH partage. SANS q3/q4, census materialise, resolver,"
              " fold, payload : cette serie ne ferme AUCUN SLO officiel.\n");

  std::vector<double> t_build, t_emst, t_q2, t_warm;
  const i64 all_pairs = (i64)n * (n - 1) / 2;
  for (int rep = 0; rep < reps; ++rep) {
    const i64 rep_seed = seed + rep;   // un nuage FRAIS par repetition
    const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, rep_seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    const auto t0 = std::chrono::steady_clock::now();
    mhgp3v::MortonLbvh tree;
    tree.build(pts, leaf_size);
    const auto t1 = std::chrono::steady_clock::now();
    const mhgp3v::EmstResult emst = mhgp3v::emst_boruvka(tree);
    const auto t2 = std::chrono::steady_clock::now();
    if (!emst.ok || (int)emst.edges.size() != n - 1) {
      std::printf("ECHEC : EMST refuse ou arbre couvrant incomplet\n");
      return 1;
    }
    const mhgp3v::yao48::ShardedOutcome q2 = mhgp3v::yao48::run_sharded(
        tree, mhgp3v::yao48::SourceInjections{}, bank_pops, chamber_visits, threads);
    const auto t3 = std::chrono::steady_clock::now();
    if (q2.per_anchor_violated) {
      std::printf("ECHEC : la fermeture par ancre a ete violee\n");
      return 1;
    }
    const mhgp3v::yao48::SourceReceipt& r = q2.receipt;
    if (r.region_pruned_mass + r.point_tombstones + r.survivors != all_pairs) {
      std::printf("ECHEC : ledger q2 non ferme\n");
      return 1;
    }
    const double build_s = std::chrono::duration<double>(t1 - t0).count();
    const double emst_s = std::chrono::duration<double>(t2 - t1).count();
    const double q2_s = std::chrono::duration<double>(t3 - t2).count();
    t_build.push_back(build_s);
    t_emst.push_back(emst_s);
    t_q2.push_back(q2_s);
    t_warm.push_back(build_s + emst_s + q2_s);
    std::printf("rep %d : graine=%lld build=%.3f s emst=%.3f s (%d rondes)"
                " q2=%.3f s warm=%.3f s — q2 : coupe=%lld+%lld, survivantes=%lld,"
                " tombstones=%lld, census=%lld (fermes=%lld stricts=%lld"
                " contacts=%lld), ledger FERME\n", rep, rep_seed, build_s, emst_s,
                (int)emst.rounds, q2_s, build_s + emst_s + q2_s,
                r.region_pruned_mass, r.point_tombstones, r.survivors,
                r.classifier_tombstones, r.census_records, r.census_closed_total,
                r.census_strict_total, r.census_contact_total);
  }
  std::printf("p50 : build=%.3f s emst=%.3f s q2=%.3f s warm=%.3f s\n",
              percentile(t_build, 0.50), percentile(t_emst, 0.50),
              percentile(t_q2, 0.50), percentile(t_warm, 0.50));
  std::printf("p95 : build=%.3f s emst=%.3f s q2=%.3f s warm=%.3f s\n",
              percentile(t_build, 0.95), percentile(t_emst, 0.95),
              percentile(t_q2, 0.95), percentile(t_warm, 0.95));
  std::printf("OK : serie diagnostique %s — aucune revendication de SLO officiel\n",
              "warm_e2e_h0_v3_diagnostic");
  return 0;
}
