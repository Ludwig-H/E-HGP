// MorseHGP3D v9 — porte causale de la priorite des echecs de la tour FULL.
//
// Des points de panne (MHGP9_TESTING, jamais dans une cible produit) font
// echouer l'ordre K a la fin de ses lots (phase A) ou de ses images
// verticales (phase C). Pour chaque scenario a DEUX pannes, la chaine doit
// rendre la panne du plus petit K sur les deux phases, identique sur la
// boucle sequentielle (statique 1) et sur les ordres concurrents (4 et 8
// fils) ; le cas croise « lots K5 + images K2 » rend images K2. Le travail
// paye reste compte apres l'echec (naissances et contributions non nulles).
// Planchers : ordres concurrents mesures (parallel_orders) sur chaque cas
// multi-fils, et une tour complete sans panne. Enfin un fil qui ne peut etre
// lance dans le tri parallele des presentations de la chaine donne un refus
// de ressource (jamais un invariant), avec le temps de fusion paye publie et
// aucun resume d'ordre.
//
//   mhgp9_chain_order_failure_priority_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../../src/tower/forest/full_ball_tower.hpp"

#if !defined(MHGP9_TESTING)
#error This gate needs the MHGP9_TESTING failpoints
#endif

namespace {
struct Scenario {
  unsigned lots, images;  // bit K-1
  const char* expected;
};
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_chain_order_failure_priority_gate --selftest\n");
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
  namespace detail = mhgp9::tower::full_ball_detail;
  const auto bit = [](unsigned k) { return 1U << (k - 1); };
  const Scenario scenarios[] = {
      {bit(3) | bit(5), 0, "tower: failpoint_lots_k3"},
      {0, bit(2) | bit(4), "tower: failpoint_images_k2"},
      {bit(5), bit(2), "tower: failpoint_images_k2"},
      {bit(2), bit(2) | bit(4), "tower: failpoint_lots_k2"},
      {bit(4), bit(3), "tower: failpoint_images_k3"},
      {bit(1), bit(5), "tower: failpoint_lots_k1"},
  };
  std::uint64_t checks = 0, concurrent = 0;
  const auto run = [&](int statics) {
    mhgp9::ChainOptions options;
    options.kmax = 5;
    options.workers = 4;
    options.tower_static_threads = statics;
    return mhgp9::run_tower_chain(points, options);
  };
  for (const auto& scenario : scenarios)
    for (const int statics : {1, 4, 8}) {
      detail::failpoint_lots = scenario.lots;
      detail::failpoint_images = scenario.images;
      const auto r = run(statics);
      detail::failpoint_lots = 0;
      detail::failpoint_images = 0;
      ++checks;
      if (r.status != mhgp9::ChainStatus::kInvariantViolated || r.reason != scenario.expected) {
        std::printf("cause=priority static=%d expected=%s reason=%s\n", statics, scenario.expected, r.reason.c_str());
        return 1;
      }
      if (r.tower_stats.births == 0 || r.tower_stats.contributions == 0) {
        std::printf("cause=ledger.paid_work_lost static=%d expected=%s\n", statics, scenario.expected);
        return 1;
      }
      if (statics > 1 && r.tower_stats.parallel_orders == 5) ++concurrent;
    }
  {
    mhgp9::tower::parallel_detail::launch_fail_after = 1;
    const auto launch = run(4);
    mhgp9::tower::parallel_detail::launch_fail_after = static_cast<std::size_t>(-1);
    ++checks;
    if (launch.status != mhgp9::ChainStatus::kResourceExhausted ||
        launch.reason.rfind("chain_thread_launch_failed", 0) != 0 || !(launch.times.merge_ms > 0) ||
        !launch.orders.empty() || launch.times.q34_ms <= 0) {
      std::printf("cause=launch_failure status=%d reason=%s merge_ms=%.3f\n", static_cast<int>(launch.status),
                  launch.reason.c_str(), launch.times.merge_ms);
      return 1;
    }
  }
  const auto complete = run(4);
  ++checks;
  if (complete.status != mhgp9::ChainStatus::kComplete) {
    std::printf("cause=complete.status reason=%s\n", complete.reason.c_str());
    return 1;
  }
  std::printf("order_failure_priority_gate checks=%llu concurrent_failures=%llu digest=%016llx\n",
              static_cast<unsigned long long>(checks), static_cast<unsigned long long>(concurrent),
              static_cast<unsigned long long>(complete.tower_digest));
  if (concurrent != 2 * std::size(scenarios) || complete.tower_stats.parallel_orders != 5) {
    std::printf("cause=floor.concurrent_orders\n");
    return 3;
  }
  return 0;
}
