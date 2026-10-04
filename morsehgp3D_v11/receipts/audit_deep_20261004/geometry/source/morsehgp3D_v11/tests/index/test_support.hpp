// Donnees de test et comparaisons independantes des parcours de l'index.
#pragma once
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

#include "index/index.hpp"

namespace index_test {
using namespace mhgp11;

struct Input {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  explicit Input(const std::vector<std::array<u32, 3>>& points) {
    for (std::size_t i = 0; i < points.size(); ++i) {
      x.push_back(points[i][0]); y.push_back(points[i][1]); z.push_back(points[i][2]);
      ids.push_back(PointId{static_cast<u32>(100 + 3 * i)});
    }
  }
  Result<Cloud> prepare(MemoryBudget& budget) const {
    return prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
  }
};
inline Input octa() {
  return Input({{2, 4, 4}, {6, 4, 4}, {4, 2, 4}, {4, 6, 4}, {4, 4, 2}, {4, 4, 6},
                {4, 4, 4}, {4, 5, 4}, {4, 4, 5}, {0, 0, 0}, {8, 8, 8}});
}
inline num::Point point(i64 x, i64 y, i64 z) {
  auto result = num::Point::make(x, y, z);
  if (!result.ok()) throw std::runtime_error("point index de test hors domaine");
  return result.value();
}
inline num::Sphere ball() {
  const auto result = num::Sphere::through(point(2, 4, 4), point(6, 4, 4));
  if (!result.ok() || !result.value()) throw std::runtime_error("sphere index de test invalide");
  return *result.value();
}
inline std::vector<SiteIdx> copy(std::span<const SiteIdx> values) { return {values.begin(), values.end()}; }
inline bool equal(std::span<const SiteIdx> a, std::span<const SiteIdx> b) { return copy(a) == copy(b); }

inline bool analytic(const Cloud& cloud, const Census& result, u32 threshold) {
  std::vector<SiteIdx> interior, shell;
  for (u32 i = 0; i < cloud.sites(); ++i) {
    const i64 x = i64{cloud.x()[i]} - 4, y = i64{cloud.y()[i]} - 4, z = i64{cloud.z()[i]} - 4;
    const i64 power = x*x + y*y + z*z - 4;
    if (power < 0) interior.push_back(SiteIdx{i});
    if (power == 0) shell.push_back(SiteIdx{i});
  }
  if (interior.size() < threshold)
    return result.kind() == CensusKind::complete && copy(result.interior()) == interior && copy(result.shell()) == shell;
  if (result.kind() != CensusKind::saturated || !result.shell().empty() || result.interior().size() != threshold)
    return false;
  u32 last = 0;
  bool first = true;
  for (const auto site : result.interior()) {
    if ((!first && idx(site) <= last) || idx(site) >= cloud.sites()) return false;
    bool found = false;
    for (auto expected : interior) found = found || site == expected;
    if (!found) return false;
    first = false; last = idx(site);
  }
  return true;
}

inline u64 nodes(u32 n, u32 leaf) {  // recurrence de l'arbre, pas formule quotient/reste du produit
  if (n <= leaf) return 1;
  return 1 + nodes(n / 2, leaf) + nodes(n - n / 2, leaf);
}
inline u64 depth(u32 n, u32 leaf) {
  if (n <= leaf) return 1;
  const auto left = depth(n / 2, leaf), right = depth(n - n / 2, leaf);
  return 1 + (left > right ? left : right);
}
}  // namespace index_test
