// MorseHGP3D v3 — LE CATALOGUE PARALLELE : le front d'onde CPU du parcours.
//
// Le profil est sans ambiguite (n=400 : parcours 40,5 s, recolte 2,3 s) : le
// poste dominant du pipeline est `navigate/reverse_search`, pas la recolte.
// Or la reverse search d'Avis--Fukuda est SANS ETAT PARTAGE : la filiation est
// une fonction du sommet seul et de la base du germe — deterministe depuis le
// nuage. Le decoupage est donc :
//
//   1. une COURONNE sequentielle a profondeur bornee, qui emet ses sommets et
//      collecte la FRONTIERE (les racines de sous-arbres, ni emises ni
//      poussees) ;
//   2. les sous-arbres distribues aux threads — chaque thread rejoue le MEME
//      parcours seme a sa racine (le germe et root_base se rederivent
//      identiquement du nuage), et emet exactement ses descendants ;
//   3. la concatenation deterministe couronne + sous-arbres en ordre de
//      frontiere, puis LA MEME recolte que `flat_catalogue` (sommets
//      precalcules).
//
// Chaque sommet est visite EXACTEMENT une fois (partition d'arbre). L'ORDRE
// des sommets differe du DFS sequentiel : le catalogue est semantiquement
// identique (memes boules, memes supports canoniques — independants du sommet
// decouvreur) mais l'ordre d'emission change ; le differentiel se fait par la
// comparaison canonique (records aux niveaux exacts), comme la permutation.
#pragma once

#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include "prototype/order_k_flats.hpp"

namespace mhgp3v {

// Les mutants du front d'onde vivent ici pour que la porte prouve qu'elle
// mord : perdre un sous-arbre de la frontiere doit se voir dans le catalogue.
struct ParallelCatalogueMutants {
  bool drop_odd_roots = false;   // MUTANT : un sous-arbre de frontiere sur deux est perdu
};

inline mhgp::Catalogue flat_catalogue_parallel(const std::vector<mhgp::P3>& points, int s_max,
                                               FlatStatistics* st, CloudStatus* status,
                                               int threads, bool use_index = true,
                                               int crown_depth = 8,
                                               ParallelCatalogueMutants mutants = {}) {
  *st = FlatStatistics{};
  *status = CloudStatus::kOk;
  if (threads < 1) threads = (int)std::thread::hardware_concurrency();
  if (threads < 1) threads = 1;
  const int level_ceiling = s_max - 2;

  CertifiedIndex grid;
  const bool indexed = use_index && !points.empty();
  if (indexed) grid.build(points, 16);

  const auto tick = []() { return std::chrono::steady_clock::now(); };
  const auto lap = [](std::chrono::steady_clock::time_point a,
                      std::chrono::steady_clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
  };
  const auto t0 = tick();
  // 1. LA COURONNE, et la frontiere des sous-arbres. Elle fait le preflight et
  // derive la base du germe UNE fois ; les workers la recoivent telle quelle et
  // sautent cette derivation (payee par racine, elle coutait trois fois le
  // parcours).
  std::vector<flats::Vertex> crown;
  std::vector<flats::Vertex> frontier;
  std::vector<mhgp::i32> shared_root_base;
  reverse_search_stream(points, level_ceiling, st, status,
                        [&](const flats::Vertex& v) {
                          crown.push_back(v);
                          return true;
                        },
                        indexed ? &grid : nullptr, nullptr, nullptr, crown_depth, &frontier,
                        &shared_root_base);
  if (*status != CloudStatus::kOk) return mhgp::Catalogue{};
  if (mutants.drop_odd_roots) {
    std::vector<flats::Vertex> kept;
    for (std::size_t r = 0; r < frontier.size(); r += 2) kept.push_back(frontier[r]);
    frontier = std::move(kept);
  }
  const auto t1 = tick();

  // 2. LES SOUS-ARBRES, ordonnances DYNAMIQUEMENT (compteur atomique) : la
  // charge par racine est tres irreguliere — l'arbre n'est profond que d'une
  // vingtaine de niveaux et quelques sous-arbres dominent — le vol de travail
  // par compteur lisse ce que le tour de role statique subissait.
  std::vector<std::vector<flats::Vertex>> harvested((std::size_t)frontier.size());
  std::vector<FlatStatistics> partial((std::size_t)threads);
  std::vector<CloudStatus> partial_status((std::size_t)threads, CloudStatus::kOk);
  std::atomic<std::size_t> next_root{0};
  {
    std::vector<std::thread> workers;
    for (int w = 0; w < threads; ++w)
      workers.push_back(std::thread([&, w]() {
        for (std::size_t r = next_root.fetch_add(1); r < frontier.size();
             r = next_root.fetch_add(1)) {
          CloudStatus local = CloudStatus::kOk;
          reverse_search_stream(points, level_ceiling, &partial[(std::size_t)w], &local,
                                [&](const flats::Vertex& v) {
                                  harvested[r].push_back(v);
                                  return true;
                                },
                                indexed ? &grid : nullptr, &frontier[r], &shared_root_base, 0,
                                nullptr);
          if (local != CloudStatus::kOk) partial_status[(std::size_t)w] = local;
        }
      }));
    for (std::thread& worker : workers) worker.join();
  }
  for (int w = 0; w < threads; ++w) {
    st->absorb(partial[(std::size_t)w]);
    if (partial_status[(std::size_t)w] != CloudStatus::kOk) {
      *status = partial_status[(std::size_t)w];
      return mhgp::Catalogue{};
    }
  }

  const auto t2 = tick();
  std::size_t subtree_total = 0;
  for (const std::vector<flats::Vertex>& part : harvested) subtree_total += part.size();
  std::fprintf(stderr,
               "[parallel] couronne %zu sommets %.3f s | frontiere %zu racines,"
               " sous-arbres %zu sommets %.3f s (%d threads)\n",
               crown.size(), lap(t0, t1), frontier.size(), subtree_total, lap(t1, t2), threads);
  // 3. CONCATENATION DETERMINISTE puis LA MEME recolte que la voie
  // sequentielle, sur les sommets precalcules.
  std::vector<flats::Vertex> vertices = std::move(crown);
  for (std::vector<flats::Vertex>& part : harvested)
    for (flats::Vertex& v : part) vertices.push_back(std::move(v));
  FlatStatistics harvest_stats{};
  const mhgp::Catalogue catalogue = flat_catalogue(points, s_max, &harvest_stats, status,
                                                   /*verify_census=*/false, use_index,
                                                   /*use_owner=*/false, &vertices);
  st->absorb(harvest_stats);
  std::fprintf(stderr, "[parallel] recolte %.3f s\n", lap(t2, tick()));
  return catalogue;
}

}  // namespace mhgp3v
