// Outils des portes unitaires de l'etage G (hors produit) : nuage, index, catalogue et resolution d'un petit nuage
// grave ; domaine interne pour appeler resolve_part et lem_t1 directement sur une partie choisie.
#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "sched/sched.hpp"
#include "test.hpp"
#include "tower/internal.hpp"

namespace tower_test {
using namespace mhgp12;
using Xyz = std::array<u32, 3>;

// Nuage grave : index, catalogue Cat_K, resolution de l'etage G, points exacts ; un budget propre par cas.
struct Case {
  MemoryBudget budget{MemoryBudget::kUnlimited};
  std::unique_ptr<sched::Pool> pool;
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
  std::optional<Resolution> resolution;
  std::vector<num::Point> points;
  Outcome outcome;

  const Cloud& cloud() const { return index->cloud(); }
  tower_detail::Domain domain() const {
    return tower_detail::Domain{*index, *catalogue, std::span<const num::Point>(points.data(), points.size())};
  }
  u32 site(const Xyz& p) const {
    for (u32 s = 0; s < cloud().sites(); ++s)
      if (cloud().x()[s] == p[0] && cloud().y()[s] == p[1] && cloud().z()[s] == p[2]) return s;
    return kNone;
  }
  tower_detail::Part part(std::vector<Xyz> sites) const {
    tower_detail::Part f;
    std::vector<u32> ids;
    for (const auto& p : sites) ids.push_back(site(p));
    std::sort(ids.begin(), ids.end());
    for (u32 s : ids) f.id[f.k++] = s;
    return f;
  }
  // Boule du catalogue dont la population I u U est exactement ces sites.
  u32 ball(std::vector<Xyz> sites) const {
    std::vector<u32> want;
    for (const auto& p : sites) want.push_back(site(p));
    std::sort(want.begin(), want.end());
    for (u32 b = 0; b < catalogue->balls(); ++b) {
      const auto row = catalogue->interior(make_id<BallIdx>(b));
      const auto shell = catalogue->shell(make_id<BallIdx>(b));
      std::vector<u32> got;
      for (SiteIdx s : row) got.push_back(idx(s));
      for (SiteIdx s : shell) got.push_back(idx(s));
      std::sort(got.begin(), got.end());
      if (got == want) return b;
    }
    return kNone;
  }
  LevelRank rank(u32 b) const { return catalogue->balls_data()[b].rank; }
};

inline Outcome prepare(Case& c, const std::vector<Xyz>& pts, int kmax, u32 threads) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    x.push_back(pts[i][0]);
    y.push_back(pts[i][1]);
    z.push_back(pts[i][2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(i)));
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), c.budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{}, c.budget);
  if (!index.ok()) return index.outcome();
  c.index.emplace(std::move(index.value()));
  auto pool = sched::make_pool({threads});
  if (!pool.ok()) return pool.outcome();
  c.pool = std::move(pool.value());
  CatalogueParams params;
  params.kmax = kmax;
  params.leaf_size = static_cast<u32>(kmax + 3);
  auto cat = build_catalogue(c.index->cloud(), params, c.budget, *c.pool);
  if (!cat.ok()) return cat.outcome();
  c.catalogue.emplace(std::move(cat.value()));
  for (u32 s = 0; s < c.cloud().sites(); ++s)
    c.points.push_back(num::Point::make(c.cloud().x()[s], c.cloud().y()[s], c.cloud().z()[s]).value());
  auto r = resolve_tower(*c.index, *c.catalogue, c.budget, *c.pool);
  if (!r.ok()) return r.outcome();
  c.resolution.emplace(std::move(r.value()));
  return {};
}

inline std::unique_ptr<Case> build(const std::vector<Xyz>& pts, int kmax, u32 threads = 2) {
  auto c = std::make_unique<Case>();
  c->outcome = prepare(*c, pts, kmax, threads);
  return c;
}

// Resolution directe d'une partie a l'ordre k sous une jonction de rang donne (table de populations reconstruite).
struct Direct {
  Result<u32> target = fail(Reason::tower_invariant);
  OrderCounters counters;
};
inline Direct resolve_direct(Case& c, const tower_detail::Part& f, Order k, LevelRank junction) {
  Direct out;
  tower_detail::PopulationTable table;
  const auto keys = c.resolution->order(k).birth_keys();
  if (const Outcome o = table.build(*c.catalogue, keys, k, c.budget); !o.ok()) {
    out.target = o;
    return out;
  }
  auto workspace = CensusWorkspace::make(*c.index, c.budget);
  if (!workspace.ok()) {
    out.target = workspace.outcome();
    return out;
  }
  const auto domain = c.domain();
  const tower_detail::ResolveContext context{domain, *c.resolution, tower_detail::OrderView{k, keys, &table}};
  out.target = tower_detail::resolve_part(context, f, junction, *workspace.value(), out.counters);
  return out;
}

}  // namespace tower_test
