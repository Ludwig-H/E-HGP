// MorseHGP3D v9 — porte differentielle du MEB propose (anchor_meb_proposed).
//
// Juge : la reference anchor_meb (enumeration lexicographique, elle-meme jugee
// par le juge rationnel de mhgp9_tower_anchor_meb). Pour chaque ensemble de 1
// a 10 sites, la voie proposee doit rendre EXACTEMENT le meme resultat :
// statut, cle, niveau, taille et emplacements du support, coquille
// selectionnee. Ensembles : aleatoires sur grilles u18, 2^16 et minuscules
// (tres degeneres), points entiers cospheriques (x^2+y^2+z^2 = r^2, sommets de
// cube et d'octaedre) avec ou sans sites interieurs, permutations.
// Planchers : propositions verifiees, canonisations sur un bord plus grand que
// le support, et moins de supports essayes que la reference. Compilee avec
// MHGP9_MEB_PROPOSED_TEST_CORRUPT, une proposition sur trois est faussee : les
// replis sur l'enumeration complete doivent exister et rendre le meme resultat.
//
//   mhgp9_tower_anchor_meb_proposed_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
// Compile avec MHGP9_MEB_PROPOSED_MUTANT_NO_CANONICAL, la voie proposee rend
// le support verifie meme quand le bord est plus grand : code 1 attendu.
#include <algorithm>
#include <cstdio>
#include <random>
#include <string_view>
#include <vector>

#include "../../src/tower/forest/anchor_meb.hpp"

using namespace mhgp9::tower;

namespace {
u64 compared = 0;
bool same(const AnchorMebResult& a, const AnchorMebResult& b) {
  return a.status == b.status && a.key == b.key && same_exact_level(a.level, b.level) &&
         a.support_size == b.support_size && a.support_slots == b.support_slots &&
         a.selected_shell_count == b.selected_shell_count;
}
bool check(const std::vector<P3>& sites, AnchorMebWork& reference, AnchorMebWork& proposed) {
  const auto r = anchor_meb(sites, reference);
  const auto p = anchor_meb_proposed(sites, proposed);
  ++compared;
  if (!same(r, p)) {
    std::printf("cause=proposed.differs n=%zu ref_q=%u prop_q=%u ref_shell=%u prop_shell=%u\n", sites.size(),
                static_cast<unsigned>(r.support_size), static_cast<unsigned>(p.support_size),
                static_cast<unsigned>(r.selected_shell_count), static_cast<unsigned>(p.selected_shell_count));
    return false;
  }
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_tower_anchor_meb_proposed_gate --selftest\n");
    return 2;
  }
  AnchorMebWork reference, proposed;
  std::mt19937_64 random(20260923);
  // Random sets on several grids, each also shuffled.
  for (const u64 bound : {u64{262144}, u64{65536}, u64{64}, u64{8}, u64{4}})
    for (unsigned n = 1; n <= 10; ++n)
      for (unsigned repetition = 0; repetition < (bound <= 8 ? 400u : 150u); ++repetition) {
        std::vector<P3> sites;
        unsigned guard = 0;
        while (sites.size() != n && guard++ < 1000) {
          const P3 point{static_cast<i64>(random() % bound), static_cast<i64>(random() % bound),
                         static_cast<i64>(random() % bound)};
          if (std::find(sites.begin(), sites.end(), point) == sites.end()) sites.push_back(point);
        }
        if (sites.size() != n) continue;
        if (!check(sites, reference, proposed)) return 1;
        std::shuffle(sites.begin(), sites.end(), random);
        if (!check(sites, reference, proposed)) return 1;
      }
  // Integer points on spheres (many exact cospherical boundary sites),
  // shifted into the u18 domain, with random subsets and interior sites.
  std::vector<std::vector<P3>> spheres;
  for (const i64 r2 : {i64{3}, i64{9}, i64{25}, i64{50}, i64{169}}) {
    std::vector<P3> shell;
    for (i64 x = -13; x <= 13; ++x)
      for (i64 y = -13; y <= 13; ++y)
        for (i64 z = -13; z <= 13; ++z)
          if (x * x + y * y + z * z == r2) shell.push_back({x + 1000, y + 1000, z + 1000});
    if (shell.size() >= 4) spheres.push_back(shell);
  }
  for (const auto& shell : spheres)
    for (unsigned repetition = 0; repetition < 600; ++repetition) {
      std::vector<P3> pool = shell;
      std::shuffle(pool.begin(), pool.end(), random);
      const unsigned on = 2 + static_cast<unsigned>(random() % std::min<std::size_t>(9, pool.size() - 1));
      std::vector<P3> sites(pool.begin(), pool.begin() + std::min<std::size_t>(on, pool.size()));
      const unsigned inside = static_cast<unsigned>(random() % 3);
      for (unsigned i = 0; i < inside && sites.size() < 10; ++i) {
        const P3 point{1000 + static_cast<i64>(random() % 3) - 1, 1000 + static_cast<i64>(random() % 3) - 1,
                       1000 + static_cast<i64>(random() % 3) - 1};
        if (std::find(sites.begin(), sites.end(), point) == sites.end()) sites.push_back(point);
      }
      if (sites.size() > 10) sites.resize(10);
      if (!check(sites, reference, proposed)) return 1;
    }
  // Cube and octahedron vertices, with and without the centre.
  const std::vector<std::vector<P3>> solids{
      {{0,0,0},{2,0,0},{0,2,0},{0,0,2},{2,2,0},{2,0,2},{0,2,2},{2,2,2}},
      {{2,1,1},{0,1,1},{1,2,1},{1,0,1},{1,1,2},{1,1,0}},
      {{0,0,0},{2,0,0},{0,2,0},{0,0,2},{2,2,0},{2,0,2},{0,2,2},{2,2,2},{1,1,1}},
      {{2,1,1},{0,1,1},{1,2,1},{1,0,1},{1,1,2},{1,1,0},{1,1,1}}};
  for (const auto& solid : solids)
    for (u32 mask = 1; mask < (u32{1} << solid.size()); ++mask) {
      std::vector<P3> sites;
      for (unsigned i = 0; i < solid.size(); ++i)
        if ((mask & (u32{1} << i)) != 0) sites.push_back(solid[i]);
      if (!check(sites, reference, proposed)) return 1;
    }
  u64 reference_supports = 0, proposed_supports = 0;
  for (const auto count : reference.supports_by_size) reference_supports += count;
  for (const auto count : proposed.supports_by_size) proposed_supports += count;
  std::printf("anchor_meb_proposed_gate compared=%llu proposals=%llu verified=%llu canonical=%llu fallbacks=%llu "
              "supports_reference=%llu supports_proposed=%llu\n",
              static_cast<unsigned long long>(compared), static_cast<unsigned long long>(proposed.proposals),
              static_cast<unsigned long long>(proposed.verified_proposals),
              static_cast<unsigned long long>(proposed.boundary_canonicalizations),
              static_cast<unsigned long long>(proposed.proposal_fallbacks),
              static_cast<unsigned long long>(reference_supports), static_cast<unsigned long long>(proposed_supports));
#if defined(MHGP9_MEB_PROPOSED_TEST_CORRUPT)
  const bool fallback_floor = proposed.proposal_fallbacks > 0;  // corrupted proposals must fall back
#else
  const bool fallback_floor = true;  // real proposals may never fail: no floor
#endif
  if (proposed.verified_proposals == 0 || proposed.boundary_canonicalizations == 0 || !fallback_floor ||
      proposed_supports >= reference_supports) {
    std::printf("cause=floor.proposed_paths\n");
    return 3;
  }
  return 0;
}
