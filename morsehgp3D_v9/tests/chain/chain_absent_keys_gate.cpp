// MorseHGP3D v9 — juge d'echantillon des cles JAMAIS emises (completude du
// catalogue de la chaine a l'echelle ; question de l'auditeur B, 23 sept. 2026).
//
// Euler, les recoupements du census et la porte d'echelle de C ne jugent que
// des cles presentes ou des sommes. Ce juge part de l'autre cote : il tire des
// supports candidats S (2 a 4 sites) parmi les plus proches voisins de sites
// echantillonnes, calcule EXACTEMENT leur boule minimale (anchor_meb), garde
// ceux dont S est le support minimal (taille du support = |S|, centre
// interieur a conv S), recense la boule sur un index reconstruit (puissances
// exactes), calcule q_min (ShellTable pour une coquille etendue) et exige :
// toute boule de fenetre p + q_min <= min(Kmax+1, n) figure au catalogue
// (recherche exacte de sa cle). C'est un juge d'ECHANTILLON : il ne prouve pas
// l'absence d'omission, il la cherche la ou les boules critiques naissent
// (voisinages locaux), independamment du generateur.
//
// Controle de non-vacuite : sur le meme catalogue ampute d'une cle sur 61
// parmi celles que l'echantillon atteint, le juge doit trouver les manques.
//
//   mhgp9_chain_absent_keys_gate --selftest [--n=8000]
//
// Code 0 conforme, 1 omission trouvee (`cause=`), 2 argument, 3 plancher.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../../src/tower/forest/anchor_meb.hpp"
#include "../../src/tower/forest/local_plateau.hpp"
#include "../../src/tower/pipeline/census.hpp"
#include "../../src/tower/tree/cloud_index.hpp"
#include "../gen/front_fixtures.hpp"

namespace {
using mhgp9::tower::BallKey;
using mhgp9::tower::P3;

struct Totals {
  std::uint64_t candidates = 0, minimal = 0, admissible = 0, extended = 0, found = 0, missing = 0;
};

// Judge one catalogue (sorted keys) on the sampled neighbourhoods of `points`.
Totals judge(const std::vector<mhgp9::gen::Point3>& points, const std::vector<BallKey>& keys, unsigned kmax,
             std::size_t stride, std::size_t neighbours) {
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = {static_cast<mhgp9::tower::PointId>(i), P3{points[i].x, points[i].y, points[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  const std::size_t n = points.size();
  const std::size_t window = std::min<std::size_t>(kmax + 1, n);
  Totals t;
  mhgp9::tower::AnchorMebWork work;
  std::vector<mhgp9::tower::i32> interior, shell;
  std::vector<std::pair<std::int64_t, std::size_t>> distance;
  std::vector<P3> sites;
  const auto p3 = [&](std::size_t i) { return P3{points[i].x, points[i].y, points[i].z}; };
  const auto consider = [&](const std::vector<std::size_t>& support) {
    ++t.candidates;
    sites.clear();
    for (const auto i : support) sites.push_back(p3(i));
    const auto meb = mhgp9::tower::anchor_meb(sites, work);
    if (meb.status != mhgp9::tower::AnchorMebStatus::kOk || meb.support_size != support.size()) return;
    ++t.minimal;
    const auto status = mhgp9::tower::ball_census(ix, meb.key, window, 12, &interior, &shell);
    if (status != mhgp9::tower::CensusStatus::kOk) return;  // too deep (or shell > 12): outside the window
    unsigned q = static_cast<unsigned>(support.size());
    if (shell.size() != support.size()) {
      ++t.extended;
      mhgp9::tower::local_plateau::LocalCensus local{meb.key, {}, {}};
      for (auto u : interior) local.interior.push_back({ix.point_id(u), ix.upos[static_cast<std::size_t>(u)]});
      for (auto u : shell) local.shell.push_back({ix.point_id(u), ix.upos[static_cast<std::size_t>(u)]});
      q = mhgp9::tower::local_plateau::ShellTable::prepare(std::move(local)).q_min();
    }
    if (interior.size() + q > window) return;
    ++t.admissible;
    if (std::binary_search(keys.begin(), keys.end(), meb.key)) ++t.found;
    else ++t.missing;
  };
  for (std::size_t i = 0; i < n; i += stride) {
    distance.clear();
    for (std::size_t j = 0; j < n; ++j) {
      if (j == i) continue;
      const std::int64_t dx = static_cast<std::int64_t>(points[i].x) - points[j].x;
      const std::int64_t dy = static_cast<std::int64_t>(points[i].y) - points[j].y;
      const std::int64_t dz = static_cast<std::int64_t>(points[i].z) - points[j].z;
      distance.emplace_back(dx * dx + dy * dy + dz * dz, j);
    }
    const std::size_t m = std::min(neighbours, distance.size());
    std::partial_sort(distance.begin(), distance.begin() + static_cast<std::ptrdiff_t>(m), distance.end());
    std::vector<std::size_t> near(m);
    for (std::size_t a = 0; a < m; ++a) near[a] = distance[a].second;
    for (std::size_t a = 0; a < m; ++a) {
      consider({i, near[a]});
      for (std::size_t b = a + 1; b < m; ++b) {
        consider({i, near[a], near[b]});
        if (b < 6)
          for (std::size_t c = b + 1; c < 6 && c < m; ++c) consider({i, near[a], near[b], near[c]});
      }
    }
  }
  return t;
}

std::vector<BallKey> sorted_keys(const std::vector<mhgp9::tower::BallData>& balls) {
  std::vector<BallKey> keys;
  keys.reserve(balls.size());
  for (const auto& b : balls) keys.push_back(b.key);
  std::sort(keys.begin(), keys.end());
  return keys;
}
}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 8000;
  bool selftest = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "--selftest") selftest = true;
    else if (arg.starts_with("--n=")) {
      const auto digits = arg.substr(4);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
    } else return 2;
  }
  if (!selftest || n < 64 || n > 65536) {
    std::fprintf(stderr, "usage: mhgp9_chain_absent_keys_gate --selftest [--n=8000]\n");
    return 2;
  }
  const std::size_t stride = std::max<std::size_t>(1, n / 500), neighbours = 10;
  Totals all, planted_total;
  std::uint64_t planted = 0, planted_found = 0;
  for (const std::string_view family : {"uniform", "terrain", "clusters"})
    for (const unsigned kmax : {5u, 10u}) {
      const auto fixture = mhgp9::gen::bench::make_front_fixture(n, family, 3);
      mhgp9::ChainOptions options;
      options.kmax = kmax;
      options.separation_s = 8;
      options.workers = 2;
      options.run_tower = false;
      options.keep_catalogue = true;
      const auto chain = mhgp9::run_tower_chain(fixture.points, options);
      if (chain.status != mhgp9::ChainStatus::kComplete) {
        std::printf("cause=chain.status family=%s kmax=%u reason=%s\n", std::string(family).c_str(), kmax,
                    chain.reason.c_str());
        return 1;
      }
      const auto keys = sorted_keys(chain.catalogue_balls);
      const auto t = judge(fixture.points, keys, kmax, stride, neighbours);
      std::printf("absent_keys family=%s kmax=%u candidates=%llu minimal=%llu admissible=%llu extended=%llu "
                  "missing=%llu\n", std::string(family).c_str(), kmax, static_cast<unsigned long long>(t.candidates),
                  static_cast<unsigned long long>(t.minimal), static_cast<unsigned long long>(t.admissible),
                  static_cast<unsigned long long>(t.extended), static_cast<unsigned long long>(t.missing));
      if (t.missing != 0) {
        std::printf("cause=absent_key.missing family=%s kmax=%u missing=%llu\n", std::string(family).c_str(), kmax,
                    static_cast<unsigned long long>(t.missing));
        return 1;
      }
      all.candidates += t.candidates; all.admissible += t.admissible; all.extended += t.extended;
      // Planted omissions (non-vacuity): drop one key in 61 and judge again.
      if (kmax == 5) {
        std::vector<BallKey> amputated;
        for (std::size_t j = 0; j < keys.size(); ++j)
          if (j % 61 != 0) amputated.push_back(keys[j]); else ++planted;
        const auto p = judge(fixture.points, amputated, kmax, stride, neighbours);
        planted_found += p.missing;
        planted_total.admissible += p.admissible;
      }
    }
  std::printf("chain_absent_keys_gate n=%zu candidates=%llu admissible=%llu extended=%llu planted=%llu "
              "planted_detected=%llu\n", n, static_cast<unsigned long long>(all.candidates),
              static_cast<unsigned long long>(all.admissible), static_cast<unsigned long long>(all.extended),
              static_cast<unsigned long long>(planted), static_cast<unsigned long long>(planted_found));
  if (all.admissible < 1000 || planted_found == 0) {
    std::printf("cause=floor.absent_keys\n");
    return 3;
  }
  return 0;
}
