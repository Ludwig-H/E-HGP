// Aides des portes de la Session recouverte (hors produit) : nuages synthetiques, index et catalogue d'un nuage,
// empreinte FUL1 d'une tour (tests/tower/pipeline_unit.cpp, pipeline_levers.cpp).
#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "forest_support.hpp"
#include "index/index.hpp"
#include "io/io.hpp"

namespace mhgp12::tower_test {

struct Points {
  std::vector<u32> x, y, z;
};

inline Points random_points(std::mt19937_64& rng, u32 n, u32 side) {
  Points p;
  std::vector<std::array<u32, 3>> seen;
  while (p.x.size() < n) {
    const std::array<u32, 3> c{static_cast<u32>(rng() % side), static_cast<u32>(rng() % side),
                               static_cast<u32>(rng() % side)};
    if (std::find(seen.begin(), seen.end(), c) != seen.end()) continue;
    seen.push_back(c);
    p.x.push_back(c[0]);
    p.y.push_back(c[1]);
    p.z.push_back(c[2]);
  }
  return p;
}

inline Points grid_points(u32 a, u32 b, u32 c, u32 step) {
  Points p;
  for (u32 i = 0; i < a; ++i)
    for (u32 j = 0; j < b; ++j)
      for (u32 k = 0; k < c; ++k) {
        p.x.push_back(1000 + i * step);
        p.y.push_back(2000 + j * step);
        p.z.push_back(3000 + k * step);
      }
  return p;
}

// Index et catalogue d'un nuage (PointId = rang d'entree) ; nullopt si le catalogue refuse.
struct Chain {
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
};
inline Chain make_chain(const Points& p, int kmax, MemoryBudget& budget, sched::Pool& pool) {
  std::vector<PointId> ids(p.x.size());
  for (u32 i = 0; i < ids.size(); ++i) ids[i] = make_id<PointId>(i);
  Chain c;
  auto cloud = prepare_cloud(p.x, p.y, p.z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return c;
  auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
  if (!index.ok()) return c;
  c.index.emplace(std::move(index).take());
  CatalogueParams params;
  params.kmax = kmax;
  auto catalogue = build_catalogue(c.index->cloud(), params, budget, pool);
  if (catalogue.ok()) c.catalogue.emplace(std::move(catalogue).take());
  return c;
}

inline std::string digest_of(const Chain& c, const tower::TowerForests& forests) {
  const tower::BallSource balls = tower::catalogue_balls(*c.catalogue);
  const tower::FullSource source{&c.index->cloud(), c.catalogue->levels(), balls, &forests};
  auto d = tower::full_digest(source);
  if (!d.ok()) return "refus";
  const auto hex = io::to_hex(d.value());
  return std::string(hex.data(), hex.size());
}

}  // namespace mhgp12::tower_test
