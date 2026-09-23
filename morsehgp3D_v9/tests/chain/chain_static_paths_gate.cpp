// MorseHGP3D v9 — porte differentielle des voies de resolution de la tour.
//
// Sur un nuage deterministe assez grand pour que la voie statique trie en
// parallele (au moins 8 192 requetes a un ordre), la chaine complete doit
// rendre la MEME tour (condense, ordres) sur la voie temporelle (statique 0)
// et sur la voie statique a 1, 4 et 8 fils. Planchers : requetes statiques
// >= 8 192 et plus d'un ouvrier cree a 4 fils. Juge differentiel, pas un
// oracle : l'exactitude de chaque voie est jugee par T2 sur petits nuages.
//
//   mhgp9_chain_static_paths_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <utility>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_chain_static_paths_gate --selftest\n");
    return 2;
  }
  // Trois grappes u18 (LCG 64 bits), comme le preflight natif du worker G4.
  std::vector<mhgp9::gen::Point3> points;
  std::uint64_t state = 3;
  for (int i = 0; i < 1500; ++i) {
    const std::int32_t centre = (i % 3) * 40000 + 20000;
    std::int32_t c[3];
    for (int axis = 0; axis < 3; ++axis) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      c[axis] = centre + static_cast<std::int32_t>((state >> 40) % 9000);
    }
    points.push_back({c[0], c[1], c[2]});
  }
  std::uint64_t reference = 0;
  std::vector<mhgp9::ChainResult> results;
  for (const int statics : {0, 1, 4, 8}) {
    mhgp9::ChainOptions options;
    options.kmax = 5;
    options.workers = 4;
    options.tower_static_threads = statics;
    auto r = mhgp9::run_tower_chain(points, options);
    if (r.status != mhgp9::ChainStatus::kComplete) {
      std::printf("cause=chain.status static=%d reason=%s\n", statics, r.reason.c_str());
      return 1;
    }
    if (results.empty()) reference = r.tower_digest;
    else if (r.tower_digest != reference || r.orders.size() != results.front().orders.size()) {
      std::printf("cause=static_paths.digest static=%d\n", statics);
      return 1;
    }
    results.push_back(std::move(r));
  }
  std::uint64_t requests = 0;
  for (const auto count : results[2].tower_stats.static_requests) requests = std::max<std::uint64_t>(requests, count);
  std::printf("chain_static_paths_gate digest=%016llx max_static_requests=%llu workers_created=%llu\n",
              static_cast<unsigned long long>(reference), static_cast<unsigned long long>(requests),
              static_cast<unsigned long long>(results[2].tower_stats.static_workers_created));
  if (requests < 8192 || results[2].tower_stats.static_workers_created < 2) {
    std::printf("cause=floor.parallel_static_path\n");
    return 3;
  }
  return 0;
}
