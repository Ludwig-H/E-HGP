// Outils de test du module supports (hors produit) : domaine d'une fixture, boule designee par son S* en coordonnees,
// supports traduits en coordonnees, ensembles de supports compares a une homothetie pres.
#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

#include "supports/supports.hpp"

namespace supports_test {
using namespace mhgp11;
using Xyz = std::array<u32, 3>;
using Points = std::vector<Xyz>;

// Domaine FULL d'une fixture : PointId first, first + step, ... dans l'ordre de la liste ; index a feuilles de 2 ;
// catalogue Cat_kmax par la voie sequentielle de reference.
inline Result<FullDomain> domain_of(const Points& points, int kmax, MemoryBudget& budget, u32 first = 100,
                                    u32 step = 3) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (std::size_t i = 0; i < points.size(); ++i) {
    x.push_back(points[i][0]);
    y.push_back(points[i][1]);
    z.push_back(points[i][2]);
    ids.push_back(PointId{first + step * static_cast<u32>(i)});
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = kmax;
  return prepare_full_domain(std::move(index.value()), params, budget);
}

inline Xyz xyz(const FullDomain& domain, SiteIdx site) {
  const Cloud& cloud = domain.index().cloud();
  const u32 i = idx(site);
  return {cloud.x()[i], cloud.y()[i], cloud.z()[i]};
}

// Boule du catalogue dont le support canonique S* a exactement ces sites, dans l'ordre des SiteIdx.
inline std::optional<BallIdx> ball_of(const FullDomain& domain, const Points& star) {
  const auto balls = domain.catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b) {
    if (balls[b].qmin != star.size()) continue;
    bool same = true;
    for (std::size_t j = 0; same && j < star.size(); ++j) same = xyz(domain, balls[b].support[j]) == star[j];
    if (same) return BallIdx{b};
  }
  return std::nullopt;
}

inline std::vector<Points> coordinates(const FullDomain& domain, std::span<const supports::Support> supports) {
  std::vector<Points> out;
  for (const auto& support : supports) {
    Points sites;
    for (u32 j = 0; j < support.arity; ++j) sites.push_back(xyz(domain, support.sites[j]));
    out.push_back(sites);
  }
  return out;
}

// Ordre publie : arite croissante, puis ordre lexicographique des SiteIdx ; sites croissants, kNone au-dela.
inline bool published_order(std::span<const supports::Support> supports) {
  for (std::size_t i = 0; i < supports.size(); ++i) {
    const auto& s = supports[i];
    if (s.arity < 2 || s.arity > 4) return false;
    for (u32 j = 0; j < 4; ++j) {
      if (j >= s.arity) {
        if (idx(s.sites[j]) != kNone) return false;
      } else if (j > 0 && idx(s.sites[j - 1]) >= idx(s.sites[j])) {
        return false;
      }
    }
    if (i == 0) continue;
    const auto& r = supports[i - 1];
    if (r.arity != s.arity) {
      if (r.arity > s.arity) return false;
      continue;
    }
    if (!std::lexicographical_compare(r.sites.begin(), r.sites.begin() + r.arity, s.sites.begin(),
                                      s.sites.begin() + s.arity,
                                      [](SiteIdx a, SiteIdx b) { return idx(a) < idx(b); }))
      return false;
  }
  return true;
}

// Ensemble des supports en coordonnees divisees par `scale` (homothetie de rapport scale), sites tries.
inline std::vector<Points> normalized(const std::vector<Points>& supports, u32 scale) {
  std::vector<Points> out;
  for (Points sites : supports) {
    for (auto& p : sites)
      for (auto& c : p) c /= scale;
    std::sort(sites.begin(), sites.end());
    out.push_back(sites);
  }
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace supports_test
