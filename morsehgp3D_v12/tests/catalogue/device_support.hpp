// Outils des portes de la voie appareil et de la fin d'etage partagee (tranche T1-b) : nuages temoins, voie CPU,
// voie appareil jouee sur l'hote par l'executeur Pool (meme code que l'appareil), comparaison de deux catalogues.
#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "catalogue/device_pipeline.hpp"
#include "catalogue/exec_host.hpp"
#include "sched/sched.hpp"

namespace mhgp12::device_test {

using catalogue_detail::PoolExecutor;
using Points = std::vector<std::array<u32, 3>>;

inline Result<Cloud> make_cloud(const Points& pts, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    x.push_back(pts[i][0]);
    y.push_back(pts[i][1]);
    z.push_back(pts[i][2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(i)));
  }
  return prepare_cloud(x, y, z, ids, CoordWidth(), budget);
}

// Generateur deterministe (SplitMix64).
struct Mix {
  u64 state;
  u64 next() {
    state += 0x9E3779B97F4A7C15ull;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  u32 below(u64 bound) { return static_cast<u32>(next() % bound); }
};

// n points distincts de [0, side]^3.
inline Points scatter(u32 n, u32 side, u64 seed) {
  Mix mix{seed};
  Points out;
  while (out.size() < n) {
    const std::array<u32, 3> p{mix.below(u64{side} + 1), mix.below(u64{side} + 1), mix.below(u64{side} + 1)};
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  }
  return out;
}

// Coquille : permutations signees de base, a l'echelle, autour de offset (fixture de MES-M5).
inline Points shell(std::array<u32, 3> base, u32 scale, u32 offset) {
  Points out;
  std::array<u32, 3> p = base;
  std::sort(p.begin(), p.end());
  do {
    for (int signs = 0; signs < 8; ++signs) {
      std::array<u32, 3> q{};
      for (int a = 0; a < 3; ++a) q[a] = ((signs >> a) & 1) ? offset + p[a] * scale : offset - p[a] * scale;
      out.push_back(q);
    }
  } while (std::next_permutation(p.begin(), p.end()));
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

inline CatalogueParams params_of(int k, u32 leaf) {
  CatalogueParams p;
  p.kmax = k;
  p.leaf_size = leaf;
  return p;
}

inline Result<Catalogue> cpu(const Cloud& cloud, const CatalogueParams& params, u32 threads, MemoryBudget& budget,
                             CatalogueDiagnostics* diagnostics = nullptr) {
  auto pool = sched::make_pool({threads});
  if (!pool.ok()) return pool.outcome();
  return build_catalogue(cloud, params, budget, *pool.value(), diagnostics);
}

// Voie appareil jouee par l'executeur Pool ; etat garde par l'appelant (regime resident) ou neuf.
inline Result<Catalogue> host_device(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool,
                                     MemoryBudget& budget, catalogue_detail::dev::DeviceState<PoolExecutor>& state,
                                     CatalogueDiagnostics* diagnostics = nullptr) {
  PoolExecutor executor{pool, budget};
  CatalogueDiagnostics diag;
  auto out = guarded([&]() {
    return catalogue_detail::dev::device_catalogue(executor, state, cloud, params, budget, pool, diag);
  });
  if (out.ok() && diagnostics != nullptr) *diagnostics = diag;
  return out;
}

// Deux catalogues identiques : meme export (empreinte), meme grand livre, memes niveaux (numerateurs et
// denominateurs non reduits) et meme table (recherche de chaque S*).
inline bool same(const Cloud& cloud, const Catalogue& a, const Catalogue& b) {
  const auto da = catalogue_digest(cloud, a, "identite"), db = catalogue_digest(cloud, b, "identite");
  if (!da.ok() || !db.ok() || !(da.value() == db.value()) || !(a.ledger() == b.ledger())) return false;
  if (a.levels().size() != b.levels().size()) return false;
  for (u64 r = 0; r < a.levels().size(); ++r) {
    const auto& la = a.levels()[r];
    const auto& lb = b.levels()[r];
    if (!(num::to_wide(la.numerator()).words == num::to_wide(lb.numerator()).words) ||
        !(num::to_wide(la.denominator()).words == num::to_wide(lb.denominator()).words))
      return false;
  }
  for (u32 i = 0; i < a.balls(); ++i) {
    const auto& ball = a.balls_data()[i];
    const auto found = b.find_support(std::span<const SiteIdx>(ball.support.data(), ball.qmin));
    if (!found.has_value() || idx(*found) != i) return false;
  }
  return true;
}

}  // namespace mhgp12::device_test
