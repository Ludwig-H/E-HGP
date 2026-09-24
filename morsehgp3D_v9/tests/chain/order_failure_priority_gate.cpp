// MorseHGP3D v9 — porte causale de la priorite des echecs de la tour FULL.
//
// Des points de panne (MHGP9_TESTING, jamais dans une cible produit) font
// echouer l'ordre K a la fin de ses lots (phase A), de ses populations
// (phase B, v9 E4) ou de ses images verticales (phase C). Pour chaque
// scenario a DEUX pannes, la chaine doit rendre la panne du plus petit K,
// et dans l'ordre K lots, puis populations, puis images, identique sur la
// boucle sequentielle (statique 1) et sur les ordres concurrents (4 et 8
// fils), classiques, recouverts avec la queue temoin et recouverts avec la
// queue en pipeline (v9 E4) ; le cas croise « lots K5 + images K2 » rend
// images K2. Le travail paye reste compte apres l'echec (naissances et
// contributions non nulles). Planchers : ordres concurrents mesures
// (parallel_orders), recouverts et en pipeline sur chaque cas multi-fils,
// et une tour complete sans panne. Enfin un fil qui ne peut etre
// lance dans le tri parallele des presentations de la chaine donne un refus
// de ressource (jamais un invariant), avec le temps de fusion paye publie et
// aucun resume d'ordre.
//
//   mhgp9_chain_order_failure_priority_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <tuple>
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
  unsigned lots, populations, images;  // bit K-1
  const char* expected;
};
struct Mode {
  bool overlap, pipelined;
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
      {bit(3) | bit(5), 0, 0, "tower: failpoint_lots_k3"},
      {0, 0, bit(2) | bit(4), "tower: failpoint_images_k2"},
      {bit(5), 0, bit(2), "tower: failpoint_images_k2"},
      {bit(2), 0, bit(2) | bit(4), "tower: failpoint_lots_k2"},
      {bit(4), 0, bit(3), "tower: failpoint_images_k3"},
      {bit(1), 0, bit(5), "tower: failpoint_lots_k1"},
      // v9 E4: population failures, between the lots and the images of K.
      {0, bit(3), bit(2), "tower: failpoint_images_k2"},
      {bit(4), bit(3), 0, "tower: failpoint_populations_k3"},
      {0, bit(2), bit(2), "tower: failpoint_populations_k2"},
      {bit(2), bit(2), 0, "tower: failpoint_lots_k2"},
      {0, bit(4) | bit(5), 0, "tower: failpoint_populations_k4"},
  };
  // Classic static path, overlapped with the witness tail, overlapped with
  // the pipelined tail (v9 E4).
  const Mode modes[] = {{true, true}, {true, false}, {false, true}};
  std::uint64_t checks = 0, concurrent = 0, overlapped = 0, pipelined = 0, static_checks = 0;
  Mode mode_now{true, true};
  const auto run = [&](int statics) {
    mhgp9::ChainOptions options;
    options.kmax = 5;
    options.workers = 4;
    options.tower_static_threads = statics;
    options.tower_overlap_static = mode_now.overlap;
    options.tower_pipelined_tail = mode_now.pipelined;
    return mhgp9::run_tower_chain(points, options);
  };
  // Phase-0 failures (static path): the SMALLEST failing K is reported, on
  // the overlapped path (phase 0 by decreasing K) as on the classic one,
  // before any lot or image failure.
  for (const auto& mode : modes) {
    mode_now = mode;
    for (const auto& [statics_mask, lots_mask, expected] :
         {std::tuple{bit(3) | bit(5), 0U, "tower: failpoint_static_k3"},
          std::tuple{bit(4), bit(2), "tower: failpoint_static_k4"},
          std::tuple{bit(2) | bit(3) | bit(4) | bit(5), bit(1), "tower: failpoint_static_k2"}}) {
      detail::failpoint_static = statics_mask;
      detail::failpoint_lots = lots_mask;
      const auto r = run(4);
      detail::failpoint_static = 0;
      detail::failpoint_lots = 0;
      ++checks; ++static_checks;
      if (r.status != mhgp9::ChainStatus::kInvariantViolated || r.reason != expected) {
        std::printf("cause=priority.static overlap=%d pipelined=%d expected=%s reason=%s\n", mode.overlap ? 1 : 0,
                    mode.pipelined ? 1 : 0, expected, r.reason.c_str());
        return 1;
      }
    }
  }
  for (const auto& mode : modes)
  for (const auto& scenario : scenarios)
    for (const int statics : {1, 4, 8}) {
      mode_now = mode;
      detail::failpoint_lots = scenario.lots;
      detail::failpoint_populations = scenario.populations;
      detail::failpoint_images = scenario.images;
      const auto r = run(statics);
      detail::failpoint_lots = 0;
      detail::failpoint_populations = 0;
      detail::failpoint_images = 0;
      ++checks;
      if (r.status != mhgp9::ChainStatus::kInvariantViolated || r.reason != scenario.expected) {
        std::printf("cause=priority static=%d overlap=%d pipelined=%d expected=%s reason=%s\n", statics,
                    mode.overlap ? 1 : 0, mode.pipelined ? 1 : 0, scenario.expected, r.reason.c_str());
        return 1;
      }
      if (r.tower_stats.births == 0 || r.tower_stats.contributions == 0) {
        std::printf("cause=ledger.paid_work_lost static=%d expected=%s\n", statics, scenario.expected);
        return 1;
      }
      if (statics > 1 && r.tower_stats.parallel_orders == 5) ++concurrent;
      if (statics > 1 && r.tower_stats.overlapped_orders == (mode.overlap ? 5U : 0U)) ++overlapped;
      if (statics > 1 && r.tower_stats.pipelined_orders == (mode.overlap && mode.pipelined ? 5U : 0U)) ++pipelined;
    }
  mode_now = {true, true};
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
  std::printf("order_failure_priority_gate checks=%llu concurrent_failures=%llu overlapped=%llu pipelined=%llu "
              "digest=%016llx\n",
              static_cast<unsigned long long>(checks), static_cast<unsigned long long>(concurrent),
              static_cast<unsigned long long>(overlapped), static_cast<unsigned long long>(pipelined),
              static_cast<unsigned long long>(complete.tower_digest));
  const std::size_t cases = 2 * std::size(modes) * std::size(scenarios);  // statics 4 and 8
  if (concurrent != cases || overlapped != cases || pipelined != cases || static_checks != 3 * std::size(modes) ||
      complete.tower_stats.parallel_orders != 5 || complete.tower_stats.overlapped_orders != 5 ||
      complete.tower_stats.pipelined_orders != 5) {
    std::printf("cause=floor.concurrent_orders\n");
    return 3;
  }
  return 0;
}
