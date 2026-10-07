// Test G1 (g1.hpp) : la boucle de reference et le masque AVX2 rendent les memes decisions et le meme compte de tests,
// sur des termes aleatoires aux bornes reelles des entiers, avec des egalites exactes (inegalite stricte) et des
// quasi-egalites a +-1, pour K de 1 a 12 et 0 a 36 temoins ; puis le filtre d'une liste entiere contre une boucle de
// reference. Sans AVX2 sur la machine, la porte le dit et ne compare que la reference a elle-meme.
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>

#include "adaptive_support.hpp"
#include "catalogue/g1.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace cd = mhgp11::catalogue_detail;

namespace {

i64 draw(std::mt19937_64& rng, i64 bound) { return static_cast<i64>(rng() % static_cast<u64>(bound)); }

// Termes dans les bornes du profil : |scaled| < 2^(2B+2), square < 3 * 2^(2B).
cd::G1Terms random_terms(std::mt19937_64& rng) {
  const i64 scale = i64{1} << (2 * kCoordBits + 1);
  cd::G1Terms t{};
  for (auto& v : t.scaled) v = draw(rng, 2 * scale) - scale;
  t.square = draw(rng, 3 * (i64{1} << (2 * kCoordBits)));
  return t;
}

// Temoin qui met x exactement sur la frontiere (x.square - y.square == right), ou a +-delta d'elle.
cd::G1Terms boundary(const cd::G1Terms& x, std::mt19937_64& rng, i64 delta) {
  cd::G1Terms y{};
  i64 right = 0;
  for (int a = 0; a < 3; ++a) {
    y.scaled[a] = x.scaled[a] - (draw(rng, 3) == 0 ? -draw(rng, 1000) : draw(rng, 1000));
    right += std::max<i64>(0, x.scaled[a] - y.scaled[a]);
  }
  y.square = x.square - right - delta;  // delta = 0 : egalite, donc pas de domination stricte
  return y;
}

}  // namespace

MHGP11_TEST(equivalence, 50000) {
  const bool avx2 = cd::g1_avx2_available();
  std::mt19937_64 rng(20261007);
  u64 cases = 0, stopped = 0, ties = 0, mismatched = 0;
  for (int trial = 0; trial < 6000; ++trial) {
    const cd::G1Terms x = random_terms(rng);
    const u32 selected = static_cast<u32>(trial % 37);
    std::vector<cd::G1Terms> terms(selected);
    for (u32 i = 0; i < selected; ++i) {
      const int kind = static_cast<int>(rng() % 5);
      if (kind == 0) { terms[i] = boundary(x, rng, 0); ++ties; }
      else if (kind == 1) terms[i] = boundary(x, rng, 1);    // domine de justesse
      else if (kind == 2) terms[i] = boundary(x, rng, -1);   // manque de justesse
      else terms[i] = random_terms(rng);
    }
    cd::G1Witnesses w;
    cd::g1_witnesses(terms, w);
    for (u32 kmax = 1; kmax <= 12; ++kmax) {
      const cd::G1Result a = cd::g1_scalar(x, w, kmax);
      const cd::G1Result b = avx2 ? cd::g1_avx2(x, w, kmax) : cd::g1_scalar(x, w, kmax);
      const bool same = a.found == b.found && a.tests == b.tests;
      mismatched += !same;
      CHECK(same);
      stopped += a.found >= kmax;
      ++cases;
    }
  }
  CHECK(cases == 72000 && stopped > 2000 && ties > 10000 && mismatched == 0);
  std::printf("g1_equivalence avx2=%d cases=%llu stopped=%llu ties=%llu\n", int(avx2),
              static_cast<unsigned long long>(cases), static_cast<unsigned long long>(stopped),
              static_cast<unsigned long long>(ties));
}

// Filtre d'une liste entiere (g1_filter) contre la boucle de reference site par site, sur un vrai nuage.
MHGP11_TEST(filter, 200) {
  using adaptive_test::Position;
  std::mt19937_64 rng(7);
  std::vector<Position> points;
  for (int i = 0; i < 3000; ++i)
    points.push_back({static_cast<u32>(rng() % 4096), static_cast<u32>(rng() % 4096), static_cast<u32>(rng() % 512)});
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = adaptive_test::prepare(points, owner);
  REQUIRE(cloud.ok());
  std::vector<SiteIdx> parent;
  for (u32 s = 0; s < cloud.value().sites(); ++s) parent.push_back(make_id<SiteIdx>(s));
  u64 kept_total = 0, removed_total = 0;
  for (int trial = 0; trial < 70; ++trial) {
    const i64 lo = static_cast<i64>(rng() % 3000), width = 1 + static_cast<i64>(rng() % 1000);
    const cd::Box box{{lo, lo, 0}, {lo + width, lo + width, 512}};
    const u32 kmax = 1 + static_cast<u32>(trial % 12);
    std::vector<cd::G1Terms> terms;
    for (u32 i = 0; i < std::min<u32>(3 * kmax, 36); ++i) {
      const u32 s = static_cast<u32>(rng() % cloud.value().sites());
      terms.push_back(cd::g1_terms({cloud.value().x()[s], cloud.value().y()[s], cloud.value().z()[s]}, box));
    }
    cd::G1Witnesses w;
    cd::g1_witnesses(terms, w);
    std::vector<SiteIdx> out(parent.size()), expected;
    u32 count = 0;
    u64 tests = 0, expected_tests = 0;
    cd::g1_filter(cloud.value(), parent, box, w, kmax, out.data(), count, tests);
    for (SiteIdx s : parent) {
      const u32 i = idx(s);
      const cd::G1Result r =
          cd::g1_scalar(cd::g1_terms({cloud.value().x()[i], cloud.value().y()[i], cloud.value().z()[i]}, box), w, kmax);
      expected_tests += r.tests;
      if (r.found < kmax) expected.push_back(s);
    }
    CHECK_EQ(count, expected.size());
    CHECK(std::equal(expected.begin(), expected.end(), out.begin()));
    CHECK_EQ(tests, expected_tests);
    kept_total += count;
    removed_total += parent.size() - count;
  }
  CHECK(kept_total > 1000 && removed_total > 1000);
}

MHGP11_TEST_MAIN()
