// Feuille cooperative emulee sur l'hote (leaf_device::run_leaf_coop) contre la feuille sequentielle (run_leaf) :
// memes statut, compteurs et emissions dans le meme ordre, quel que soit l'ordre de comptage des paires ; une feuille
// non resolue n'emet rien. Temoins de l'audit d117de397 (q3 obtus puis q4, cache J2), frontieres m = 1, 2, 3, 31, 32.
#include <array>
#include <vector>

#include "catalogue/leaf_device_coop.hpp"
#include "core/types.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace ld = mhgp11::leaf_device;
using Coordinates = std::array<u32, 3>;

namespace {
struct Emission {
  std::array<u32, 4> support;
  u32 p, m, qmin;
  std::vector<u32> interior, shell;
  bool operator==(const Emission&) const = default;
};
struct RecordSink {
  std::vector<Emission> out;
  void emit(const ld::Ball& b, const u32* interior, const u32* shell) {
    out.push_back({{b.support[0], b.support[1], b.support[2], b.support[3]}, b.p, b.m, b.qmin,
                   std::vector<u32>(interior, interior + b.p), std::vector<u32>(shell, shell + b.m)});
  }
};

struct Leaf {
  std::vector<u32> x, y, z, sites;
  ld::Input in;
  Leaf(const std::vector<Coordinates>& pts, i64 lo, i64 hi, int kmax, bool cache) {
    for (const auto& p : pts) {
      x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
      sites.push_back(static_cast<u32>(sites.size()));
    }
    in.x = x.data(); in.y = y.data(); in.z = z.data(); in.sites = sites.data();
    in.m = static_cast<u32>(pts.size());
    for (int j = 0; j < 3; ++j) { in.lo[j] = lo; in.hi[j] = hi; }
    in.kmax = kmax; in.cache = cache;
  }
};

bool same_counts(const ld::Counts& a, const ld::Counts& b) {
  return a.dominance_tests == b.dominance_tests && a.prefixes == b.prefixes && a.judged == b.judged &&
         a.census_tests == b.census_tests && a.emitted == b.emitted && a.incidences == b.incidences &&
         a.q4_candidates == b.q4_candidates && a.q4_levels == b.q4_levels &&
         a.region_line_tests == b.region_line_tests && a.region_line_rejects == b.region_line_rejects &&
         a.region_line_evaluations == b.region_line_evaluations &&
         a.region_line_cache_hits == b.region_line_cache_hits &&
         a.region_line_fallbacks == b.region_line_fallbacks;
}

struct Tally {
  u64 leaves = 0, unresolved = 0, partial = 0, emissions = 0, q4 = 0, hits = 0, orders = 0;
};

// Compare la feuille sequentielle et la cooperative sous plusieurs ordres de comptage ; rend les emissions sequentielles.
std::vector<Emission> compare(const Leaf& leaf, Tally& tally) {
  ld::Counts reference;
  RecordSink sequential;
  const u32 status = ld::run_leaf(leaf.in, reference, sequential);
  ++tally.leaves;
  if (status != ld::kOk) {
    ++tally.unresolved;
    if (!sequential.out.empty()) ++tally.partial;  // la feuille sequentielle avait deja emis
  } else {
    tally.emissions += sequential.out.size();
    tally.q4 += reference.q4_levels;
    tally.hits += reference.region_line_cache_hits;
  }
  for (u32 order : {0u, 1u, 2u, 3u, 7u, 31u}) {
    ld::Counts counts;
    RecordSink cooperative;
    CHECK_EQ(ld::run_leaf_coop(leaf.in, counts, cooperative, order), status);
    ++tally.orders;
    if (status != ld::kOk) {
      CHECK(cooperative.out.empty());  // non resolue : rien n'est publie
      continue;
    }
    CHECK(same_counts(reference, counts));
    CHECK(cooperative.out == sequential.out);
    CHECK_EQ(counts.region_line_tests, counts.region_line_evaluations + counts.region_line_cache_hits);
  }
  return status == ld::kOk ? sequential.out : std::vector<Emission>{};
}

u64 state = 0x9E3779B97F4A7C15ull;
u32 next(u32 bound) {
  state = state * 6364136223846793005ull + 1442695040888963407ull;
  return static_cast<u32>((state >> 33) % bound);
}
std::vector<Coordinates> random_points(u32 m, u32 side) {
  std::vector<Coordinates> pts;
  while (pts.size() < m) {
    const Coordinates p{next(side), next(side), next(side)};
    bool fresh = true;
    for (const auto& q : pts) fresh = fresh && q != p;
    if (fresh) pts.push_back(p);
  }
  return pts;
}
}  // namespace

// Temoin de l'audit : premier triangle obtus, boule q4 de centre (5,5,5), rayon carre 25, p = 0, coquille 4.
MHGP11_TEST(witness_q3_obtuse_q4, 20) {
  Tally tally;
  for (bool cache : {false, true}) {
    const Leaf leaf({{5, 2, 1}, {10, 5, 5}, {9, 8, 5}, {1, 5, 8}}, 0, 16, 3, cache);
    const auto out = compare(leaf, tally);
    bool found = false;
    for (const auto& e : out)
      found = found || (e.qmin == 4 && e.p == 0 && e.m == 4 && e.support == std::array<u32, 4>{0, 1, 2, 3});
    CHECK(found);
  }
  CHECK_EQ(tally.unresolved, 0u);
}

// Tetraedre a face obtuse et cube cosphérique de center_line_cache.cpp : le cache J2 compte des succes.
MHGP11_TEST(cache_hits, 40) {
  Tally tally;
  const std::vector<std::vector<Coordinates>> cases{
      {{1, 2, 6}, {8, 4, 8}, {2, 1, 3}, {7, 8, 5}},
      {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0}, {0, 0, 4}, {4, 0, 4}, {0, 4, 4}, {4, 4, 4}}};
  for (const auto& pts : cases)
    for (int kmax : {3, 5, 10})
      for (bool cache : {false, true}) compare(Leaf(pts, 0, 16, kmax, cache), tally);
  CHECK(tally.hits > 0);
  CHECK(tally.q4 > 0);
}

// Frontieres m = 1, 2, 3, 31, 32 (paires 0 a 496, mots du cache), K = 1 a 10, cache on/off, boite pleine ou centrale.
MHGP11_TEST(sizes, 2000) {
  Tally tally;
  for (u32 m : {1u, 2u, 3u, 5u, 31u, 32u})
    for (int kmax : {1, 2, 3, 5, 10})
      for (u32 side : {8u, 64u, 1024u})
        for (int trial = 0; trial < 3; ++trial) {
          const auto pts = random_points(m, side);
          for (bool cache : {false, true}) {
            compare(Leaf(pts, 0, side, kmax, cache), tally);
            // Sous-boite centrale : des sites hors de la boite, dominances frequentes (comme une feuille du lot).
            compare(Leaf(pts, side * 3 / 8, side * 5 / 8, kmax, cache), tally);
          }
        }
  CHECK(tally.emissions > 10000);
  CHECK(tally.q4 > 100);
  CHECK(tally.hits > 100);
}

// Coordonnees pres de kCoordMax : chemins i128 refuses, la feuille entiere part au repli sans rien publier.
MHGP11_TEST(near_max, 20) {
  Tally tally;
  const u32 M = kCoordMax;
  const std::vector<std::vector<Coordinates>> cases{
      {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}},
      {{0, 0, 0}, {1, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M - 1, M, M}},
      {{0, 0, 0}, {M / 2, 0, 0}, {0, M / 2, 0}, {0, 0, M / 2}, {M, M, M}}};
  for (const auto& pts : cases)
    for (int kmax : {3, 5, 10}) compare(Leaf(pts, 0, i64{M} + 1, kmax, true), tally);
  std::printf("leaf_coop_near_max feuilles=%llu non_resolues=%llu emises_avant_refus=%llu\n",
              static_cast<unsigned long long>(tally.leaves), static_cast<unsigned long long>(tally.unresolved),
              static_cast<unsigned long long>(tally.partial));
  CHECK(tally.partial > 0);
}

MHGP11_TEST_MAIN()
