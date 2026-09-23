// MorseHGP3D v9 — porte du proprietaire d'etat du certificat de voie morte.
//
// Juge : un prouveur NEUF sur la meme couverture.
//   (1) Un prouveur reutilise d'un nuage a l'autre rend exactement le masque
//       d'un prouveur neuf. Fixture ABA de l'audit (check_q34_dead_owner_aba) :
//       arete {0,1} d'un nuage colineaire (q3 prouvee morte a K2), puis d'un
//       nuage au site hors axe (q3 ouverte), avec essai de reutilisation de
//       l'adresse d'index ; puis de nombreuses aretes de deux nuages de
//       grappes alternees sur UN prouveur, K = 5 et 10.
//   (2) Un load() interrompu (bad_alloc injecte par l'operator new de cette
//       porte) laisse le prouveur inutilisable : prove() refuse
//       (logic_error) au lieu d'employer les formes precedentes ; un load()
//       complet ulterieur rend le masque neuf.
//   (3) prove() avant tout load() refuse.
// Planchers : sur la fixture ABA, masque perime != masque neuf et adresse
// d'index EFFECTIVEMENT reemployee (sauf sous AddressSanitizer, dont la
// quarantaine l'interdit par construction) ; une allocation interrompue dans
// load() ; des aretes prouvees et des aretes ouvertes dans la serie alternee.
//
//   mhgp9_gen_q34_dead_lanes_owner_gate --selftest
//
// Code 0 conforme, 1 desaccord (stderr causal), 2 argument, 3 plancher.
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "lanes/edge_cover.hpp"
#include "lanes/q34_dead_lanes.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"

#if defined(__SANITIZE_ADDRESS__)
#define MHGP9_OWNER_GATE_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MHGP9_OWNER_GATE_ASAN 1
#endif
#endif

namespace {
// Countdown of successful allocations before one bad_alloc; negative = off.
std::atomic<long> allocation_countdown{-1};
}  // namespace

void* operator new(std::size_t size) {
  if (allocation_countdown.load() >= 0 && allocation_countdown.fetch_sub(1) == 0) throw std::bad_alloc();
  if (void* p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {
using mhgp9::gen::Point3;
using mhgp9::gen::Q34DeadLaneProver;
using mhgp9::gen::Q34DeadLaneWork;
using mhgp9::gen::Q34EdgeCover;

std::uint8_t fresh_mask(const Q34EdgeCover& cover, unsigned k) {
  Q34DeadLaneProver fresh;
  Q34DeadLaneWork work{};
  fresh.load(cover, work);
  return fresh.prove(k, 6, work);
}

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
    std::cerr << "usage: mhgp9_gen_q34_dead_lanes_owner_gate --selftest\n";
    return 2;
  }
  // (3) Never loaded.
  {
    Q34DeadLaneProver unloaded;
    Q34DeadLaneWork work{};
    bool refused = false;
    try {
      static_cast<void>(unloaded.prove(5, 6, work));
    } catch (const std::logic_error&) {
      refused = true;
    }
    if (!refused) {
      std::cerr << "q34 dead-lane owner gate: prove() accepted a prover that was never loaded\n";
      return 1;
    }
  }
  // (1a) Audit ABA fixture: stale state would prove q3 dead on the new cloud.
  const std::vector<Point3> old_points{{0, 0, 0}, {10, 0, 0}, {5, 0, 0}};
  const std::vector<Point3> new_points{{0, 0, 0}, {10, 0, 0}, {5, 10, 0}};
  Q34DeadLaneProver reused;
  std::uint8_t old_mask = 0;
  std::uintptr_t old_address = 0;
  {
    auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(old_points));
    old_address = reinterpret_cast<std::uintptr_t>(index.get());
    const auto cover = Q34EdgeCover::make(index, {0, 1});
    Q34DeadLaneWork work{};
    reused.load(*cover, work);
    old_mask = reused.prove(2, 2, work);
  }
  bool same_address = false;
  std::uint8_t reused_mask = 0, new_mask = 0;
  for (unsigned attempt = 0; attempt < 128 && !same_address; ++attempt) {
    auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(new_points));
    same_address = reinterpret_cast<std::uintptr_t>(index.get()) == old_address;
    if (!same_address && attempt + 1 < 128) continue;
    const auto cover = Q34EdgeCover::make(index, {0, 1});
    Q34DeadLaneWork work{};
    reused.load(*cover, work);
    reused_mask = reused.prove(2, 2, work);
    Q34DeadLaneProver fresh;
    Q34DeadLaneWork fresh_work{};
    fresh.load(*cover, fresh_work);
    new_mask = fresh.prove(2, 2, fresh_work);
  }
  if (reused_mask != new_mask) {
    std::cerr << "q34 dead-lane owner gate: reused prover differs from a fresh one after a new cloud\n";
    return 1;
  }
  if (old_mask == new_mask) {
    std::cerr << "q34 dead-lane owner gate: ABA fixture is vacuous (stale and fresh masks agree)\n";
    return 3;
  }
#if !defined(MHGP9_OWNER_GATE_ASAN)
  if (!same_address) {
    std::cerr << "q34 dead-lane owner gate: the allocator never reused the index address (ABA not exercised)\n";
    return 3;
  }
#endif
  // (1b) One prover alternating between two cluster clouds, many edges.
  const auto index_a = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(clusters(900, 11, 9000)));
  const auto index_b = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(clusters(1100, 17, 5000)));
  const auto order_a = index_a->spatial_order();
  const auto order_b = index_b->spatial_order();
  std::uint64_t alternated = 0, proved = 0, open = 0;
  Q34DeadLaneProver shared;
  for (const unsigned k : {5U, 10U})
    for (std::size_t i = 0; i + 8 < order_a.size() && i + 8 < order_b.size(); i += 29)
      for (std::size_t j = 1; j < 8; j += 3)
        for (int which = 0; which < 2; ++which) {
          const auto& index = which ? index_b : index_a;
          const auto& order = which ? order_b : order_a;
          const auto cover = Q34EdgeCover::make(index, {order[i], order[i + j]});
          Q34DeadLaneWork work{};
          shared.load(*cover, work);
          const auto mask = shared.prove(k, 6, work);
          ++alternated;
          if (mask != fresh_mask(*cover, k)) {
            std::cerr << "q34 dead-lane owner gate: alternating prover differs from a fresh one\n";
            return 1;
          }
          (mask ? proved : open) += 1;
        }
  // (2) Interrupted load: the first allocation of load() on a LARGER cover fails.
  std::uint64_t interrupted = 0;
  {
    Q34DeadLaneProver prover;
    Q34DeadLaneWork work{};
    const auto small = Q34EdgeCover::make(index_a, {order_a[0], order_a[1]});
    prover.load(*small, work);
    static_cast<void>(prover.prove(5, 6, work));
    std::size_t far = 1;
    for (std::size_t j = 1; j < order_a.size(); ++j) {
      const auto candidate = Q34EdgeCover::make(index_a, {order_a[0], order_a[j]});
      if (candidate->site_count() > 4 * small->site_count() + 16) { far = j; break; }
    }
    const auto large = Q34EdgeCover::make(index_a, {order_a[0], order_a[far]});
    allocation_countdown = 0;
    try {
      prover.load(*large, work);
    } catch (const std::bad_alloc&) {
      ++interrupted;
    }
    allocation_countdown = -1;
    bool refused = false;
    try {
      static_cast<void>(prover.prove(5, 6, work));
    } catch (const std::logic_error&) {
      refused = true;
    }
    if (interrupted == 1 && !refused) {
      std::cerr << "q34 dead-lane owner gate: prove() used the forms of an earlier cover after a failed load\n";
      return 1;
    }
    prover.load(*large, work);
    if (prover.prove(5, 6, work) != fresh_mask(*large, 5)) {
      std::cerr << "q34 dead-lane owner gate: reload after a failed load differs from a fresh prover\n";
      return 1;
    }
  }
  std::cout << "q34_dead_lanes_owner_gate aba_same_address=" << (same_address ? 1 : 0)
            << " stale_mask=" << unsigned(old_mask) << " fresh_mask=" << unsigned(new_mask)
            << " alternated=" << alternated << " proved=" << proved << " open=" << open
            << " interrupted_loads=" << interrupted << "\n";
  if (interrupted != 1 || proved == 0 || open == 0) {
    std::cerr << "q34 dead-lane owner gate: floors not reached (interrupted load, proved and open edges)\n";
    return 3;
  }
  return 0;
}
