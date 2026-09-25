// MorseHGP3D v9 — porte differentielle de la queue de la tour en pipeline (E4).
//
// Nuage deterministe de la chaine (1 500 sites en trois grappes u18, LCG 64
// bits) plus six carres plantes loin des grappes : le cercle d'un carre est
// une coquille etendue (q_min 2, quatre sites) qui contribue aux ordres 3
// et 4, donc une reference nommee apres la jointure a K4. Chaine complete a
// Kmax 5 :
//   - queue en pipeline (defaut) a 1, 2, 3, 4 et 8 fils statiques (1 = la
//     boucle sequentielle, qui definit les IDs par premiere rencontre) ;
//   - temoin (tower_pipelined_tail = false, assign_populations apres la
//     jointure) a 4 et 8 fils.
// Tous rendent le MEME condense, la MEME banque (lignes en ordre d'ID), les
// MEMES references de contribution (ID, masque, interieur, segment) et le
// meme travail publie (champs de tower_work de la sonde). Planchers : ordres
// en pipeline = Kmax a plusieurs fils, pas de population de chaque ordre
// K >= 2 chronometre, references differees >= nombre de carres, requetes
// statiques >= 8 192.
// Integration avant R21 (auditeur B) : les huit combinaisons des trois
// leviers freres de la tour (queue en pipeline, regroupement hache de la
// phase 0, pool persistant) a 4 fils statiques, plus le temoin a 8 fils :
// meme condense, memes IDs, meme travail, et chaque levier prend reellement
// son chemin (ordres en pipeline, ordres haches, fils du pool).
//
//   mhgp9_chain_tower_tail_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include "../../src/chain/tower_chain.hpp"

namespace {
struct Run {
  int statics;
  bool pipelined;
  bool hashed = true;
  bool pool = true;
};

bool same_work(const mhgp9::tower::FullBallStats& x, const mhgp9::tower::FullBallStats& y) {
  // The probe's tower_work fields (sonde), and the vertical cursor's.
  return x.records == y.records && x.extra_records == y.extra_records && x.representatives == y.representatives &&
         x.anchor_hits == y.anchor_hits && x.key_lookups == y.key_lookups &&
         x.intruder_queries == y.intruder_queries && x.intruder_nodes == y.intruder_nodes &&
         x.resolve_work.calls == y.resolve_work.calls && x.resolve_work.power_tests == y.resolve_work.power_tests &&
         x.births == y.births && x.merges == y.merges && x.contributions == y.contributions &&
         x.grouped_lots == y.grouped_lots && x.resolver_cache_hits == y.resolver_cache_hits &&
         x.resolve_work.pair_distances == y.resolve_work.pair_distances &&
         x.resolve_work.materializations == y.resolve_work.materializations &&
         x.resolve_work.supports_by_size == y.resolve_work.supports_by_size &&
         x.resolve_work.proposals == y.resolve_work.proposals &&
         x.resolve_work.verified_proposals == y.resolve_work.verified_proposals;
}

// Same bank rows in ID order and same contribution references in every order.
bool same_ids(const mhgp9::tower::FullBallTowerResult& a, const mhgp9::tower::FullBallTowerResult& b) {
  if (a.orders.size() != b.orders.size() || a.orders.empty()) return false;
  const auto& x = a.orders.front().forest.populations()->rows();
  const auto& y = b.orders.front().forest.populations()->rows();
  if (x.size() != y.size()) return false;
  for (std::size_t p = 0; p < x.size(); ++p)
    if (x[p].interior != y[p].interior || x[p].shell != y[p].shell) return false;
  for (std::size_t k = 0; k < a.orders.size(); ++k) {
    const auto& u = a.orders[k].forest.contributions();
    const auto& v = b.orders[k].forest.contributions();
    if (u.size() != v.size()) return false;
    for (std::size_t c = 0; c < u.size(); ++c)
      if (u[c].segment != v[c].segment || u[c].ref.population != v[c].ref.population ||
          u[c].ref.shell_mask != v[c].ref.shell_mask || u[c].ref.include_interior != v[c].ref.include_interior)
        return false;
  }
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_chain_tower_tail_gate --selftest\n");
    return 2;
  }
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
  constexpr int kSquares = 6;
  for (int s = 0; s < kSquares; ++s) {
    const std::int32_t x = 150000 + 5000 * s, y = 150000 + 3000 * s, z = 200000;
    for (const auto& [dx, dy] : {std::pair{0, 0}, std::pair{2, 0}, std::pair{2, 2}, std::pair{0, 2}})
      points.push_back({x + dx, y + dy, z});
  }
  const Run runs[] = {{1, true}, {2, true}, {3, true}, {4, true}, {8, true}, {4, false}, {8, false},
                      {4, true, false, true}, {4, true, true, false}, {4, true, false, false},
                      {4, false, false, true}, {4, false, true, false}, {4, false, false, false},
                      {8, false, false, false}};
  std::vector<mhgp9::ChainResult> results;
  for (const auto& run : runs) {
    mhgp9::ChainOptions options;
    options.kmax = 5;
    options.workers = 4;
    options.tower_static_threads = run.statics;
    options.tower_pipelined_tail = run.pipelined;
    options.tower_hash_grouping = run.hashed;
    options.tower_persistent_pool = run.pool;
    auto r = mhgp9::run_tower_chain(points, options);
    if (r.status != mhgp9::ChainStatus::kComplete) {
      std::printf("cause=chain.status static=%d pipelined=%d reason=%s\n", run.statics, run.pipelined ? 1 : 0,
                  r.reason.c_str());
      return 1;
    }
    const auto& ts = r.tower_stats;
    const bool path = run.statics > 1 ? ts.overlapped_orders == 5 && ts.pipelined_orders == (run.pipelined ? 5U : 0U)
                                      : ts.parallel_orders == 0 && ts.pipelined_orders == 0;
    // Phase 0 runs at K = 2..5 on the static path; the pool has W - 1 threads.
    const bool levers = run.statics <= 1 ||
                        (ts.hashed_orders == (run.hashed ? 4U : 0U) &&
                         ts.pool_threads == (run.pool ? static_cast<std::uint64_t>(run.statics - 1) : 0U));
    if (!path || !levers) {
      std::printf("cause=tail.path static=%d pipelined=%d hashed=%d pool=%d pipelined_orders=%llu hashed_orders=%llu "
                  "pool_threads=%llu\n", run.statics, run.pipelined ? 1 : 0, run.hashed ? 1 : 0, run.pool ? 1 : 0,
                  static_cast<unsigned long long>(ts.pipelined_orders),
                  static_cast<unsigned long long>(ts.hashed_orders), static_cast<unsigned long long>(ts.pool_threads));
      return 1;
    }
    if (!results.empty()) {
      const auto& ref = results.front();
      if (r.tower_digest != ref.tower_digest) {
        std::printf("cause=tail.digest static=%d pipelined=%d hashed=%d pool=%d\n", run.statics,
                    run.pipelined ? 1 : 0, run.hashed ? 1 : 0, run.pool ? 1 : 0);
        return 1;
      }
      if (!same_ids(r.tower, ref.tower)) {
        std::printf("cause=tail.population_ids static=%d pipelined=%d hashed=%d pool=%d\n", run.statics,
                    run.pipelined ? 1 : 0, run.hashed ? 1 : 0, run.pool ? 1 : 0);
        return 1;
      }
      if (!same_work(ts, ref.tower_stats)) {
        std::printf("cause=tail.tower_work static=%d pipelined=%d hashed=%d pool=%d\n", run.statics,
                    run.pipelined ? 1 : 0, run.hashed ? 1 : 0, run.pool ? 1 : 0);
        return 1;
      }
    }
    results.push_back(std::move(r));
  }
  // Floors: the pipelined tail ran in every order, extended shells deferred
  // their second reference, the static path sorted in parallel.
  std::uint64_t deferred = ~0ull, timed = 0, requests = 0, pipelined_runs = 0;
  for (std::size_t j = 1; j < std::size(runs); ++j) {
    if (!runs[j].pipelined) continue;
    ++pipelined_runs;
    const auto& r = results[j];
    deferred = std::min<std::uint64_t>(deferred, r.tower_stats.population_deferred_refs);
    for (unsigned k = 2; k <= 5; ++k) timed += r.tower_times.populations_by_k[k] > 0 ? 1 : 0;
  }
  for (const auto count : results[3].tower_stats.static_requests) requests = std::max<std::uint64_t>(requests, count);
  const auto rows = results.front().tower.orders.front().forest.populations()->rows().size();
  std::printf("tower_tail_gate digest=%016llx rows=%zu deferred_refs=%llu timed_population_steps=%llu "
              "max_static_requests=%llu\n",
              static_cast<unsigned long long>(results.front().tower_digest), rows,
              static_cast<unsigned long long>(deferred), static_cast<unsigned long long>(timed),
              static_cast<unsigned long long>(requests));
  if (deferred < kSquares || pipelined_runs != 7 || timed != 4 * pipelined_runs || requests < 8192 || rows < 10000) {
    std::printf("cause=floor.pipelined_tail\n");
    return 3;
  }
  return 0;
}
