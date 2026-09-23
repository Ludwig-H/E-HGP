// MorseHGP3D v9 — porte du cache des noeuds temoins du filtre de paires.
//
// Juge: la recherche Affine exacte (filter_q34_witnesses) sur la paire meme.
// Sur des nuages deterministes de grappes, pour de nombreuses paires (a,b) :
//   - meme paire : les voies que le cache prouve a partir de la trace de sa
//     propre recherche sont EXACTEMENT les voies que la recherche rejette ;
//   - autre paire (a,b') : les voies prouvees par la trace de (a,b) sont un
//     SOUS-ENSEMBLE des voies que la recherche complete de (a,b') rejette.
// Planchers : rejets en cache sur d'autres paires, traces a noeuds de voies
// distinctes (q3 seule / q4 seule), K = 3, 5, 10.
//
//   mhgp9_gen_q34_witness_cache_gate --selftest
//
// Code 0 conforme, 1 desaccord (stderr causal), 2 argument, 3 plancher.
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <string_view>
#include <vector>

#include "lanes/q34_witness_search.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"

namespace {
using mhgp9::gen::Point3;

std::vector<Point3> clusters(std::size_t n, std::uint64_t seed, std::int32_t spread) {
  std::vector<Point3> out;
  std::uint64_t state = seed;
  for (std::size_t i = 0; i < n; ++i) {
    const std::int32_t centre = static_cast<std::int32_t>(i % 3) * 40000 + 20000;
    std::int32_t c[3];
    for (auto& value : c) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      value = centre + static_cast<std::int32_t>((state >> 40) % static_cast<std::uint64_t>(spread));
    }
    out.push_back({c[0], c[1], c[2]});
  }
  return out;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q34_witness_cache_gate --selftest\n";
    return 2;
  }
  std::uint64_t same_checks = 0, cross_checks = 0, cross_rejections = 0, mixed_traces = 0;
  for (const auto& [n, spread] : {std::pair<std::size_t, std::int32_t>{900, 9000}, {1500, 3000}}) {
    const auto points = clusters(n, 3 + n, spread);
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    const auto order = index->spatial_order();
    for (const unsigned k : {3U, 5U, 10U}) {
      mhgp9::gen::Q34WitnessSearchWork search{};
      mhgp9::gen::Q34WitnessBoundsWork bounds{};
      mhgp9::gen::Q34WitnessCacheWork cache{};
      std::vector<mhgp9::gen::Q34WitnessNode> trace;
      for (std::size_t i = 0; i + 41 < order.size(); i += 11) {
        const auto a = points[order[i]];
        for (std::size_t j = i + 1; j < i + 40; ++j) {
          const auto b = points[order[j]];
          const auto b2 = points[order[j + 1]];
          const auto filtered = mhgp9::gen::filter_q34_witnesses(*index, a, b, static_cast<std::uint8_t>(k), 6,
                                                                 search, bounds, trace);
          const std::uint8_t rejected = static_cast<std::uint8_t>(6U & ~filtered);
          bool q3_only = false, q4_only = false;
          for (const auto& entry : trace) { q3_only |= entry.lanes == 2; q4_only |= entry.lanes == 4; }
          if (q3_only && q4_only) ++mixed_traces;
          const auto same = mhgp9::gen::q34_cached_witness_rejections(*index, a, b, static_cast<std::uint8_t>(k), 6,
                                                                      trace, cache);
          ++same_checks;
          if (same != rejected) {
            std::cerr << "q34 witness cache gate: cached lanes differ from the pair's own exact search\n";
            return 1;
          }
          const auto cached = mhgp9::gen::q34_cached_witness_rejections(*index, a, b2, static_cast<std::uint8_t>(k), 6,
                                                                        trace, cache);
          std::vector<mhgp9::gen::Q34WitnessNode> other;
          const auto filtered2 = mhgp9::gen::filter_q34_witnesses(*index, a, b2, static_cast<std::uint8_t>(k), 6,
                                                                  search, bounds, other);
          const std::uint8_t rejected2 = static_cast<std::uint8_t>(6U & ~filtered2);
          ++cross_checks;
          if ((cached & ~rejected2) != 0) {
            std::cerr << "q34 witness cache gate: cached lanes not rejected by the other pair's exact search\n";
            return 1;
          }
          if (cached != 0) ++cross_rejections;
        }
      }
    }
  }
  // A forged span repeating a node on a lane is refused, never double-credited.
  {
    const auto points = clusters(900, 903, 9000);
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    const auto order = index->spatial_order();
    mhgp9::gen::Q34WitnessSearchWork search{};
    mhgp9::gen::Q34WitnessBoundsWork bounds{};
    mhgp9::gen::Q34WitnessCacheWork cache{};
    std::vector<mhgp9::gen::Q34WitnessNode> trace;
    bool refused = false, tried = false;
    for (std::size_t i = 0; i + 1 < order.size() && !tried; ++i) {
      static_cast<void>(mhgp9::gen::filter_q34_witnesses(*index, points[order[i]], points[order[i + 1]], 5, 6,
                                                         search, bounds, trace));
      if (trace.empty()) continue;
      tried = true;
      auto forged = trace;
      forged.push_back(trace.front());
      try {
        static_cast<void>(mhgp9::gen::q34_cached_witness_rejections(*index, points[order[i]], points[order[i + 1]], 5, 6,
                                                                    forged, cache));
      } catch (const std::invalid_argument&) {
        refused = true;
      }
    }
    if (!tried || !refused) {
      std::cerr << "q34 witness cache gate: a duplicated cached node was accepted\n";
      return 1;
    }
  }
  std::cout << "q34_witness_cache_gate same=" << same_checks << " cross=" << cross_checks
            << " cross_rejections=" << cross_rejections << " mixed_traces=" << mixed_traces << "\n";
  if (cross_rejections == 0 || mixed_traces == 0) {
    std::cerr << "q34 witness cache gate: floors not reached (cross rejections, mixed-lane traces)\n";
    return 3;
  }
  return 0;
}
