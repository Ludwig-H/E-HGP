// Outils de test seulement : donnees possedees, IDs Morton et attendus analytiques de centres/niveaux.
#pragma once
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

#include "tower/tower.hpp"

namespace tower_test {
using namespace mhgp11;
using Xyz = std::array<u32, 3>;
struct Input {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  explicit Input(const std::vector<Xyz>& points) {
    for (std::size_t i = 0; i < points.size(); ++i) {
      x.push_back(points[i][0]); y.push_back(points[i][1]); z.push_back(points[i][2]);
      ids.push_back(PointId{static_cast<u32>(100 + 3 * i)});
    }
  }
  Result<Cloud> prepare(MemoryBudget& budget) const { return prepare_cloud(x, y, z, ids, CoordWidth{}, budget); }
};
inline SiteIdx site(const Cloud& cloud, Xyz point) {
  for (u32 i = 0; i < cloud.sites(); ++i)
    if (Xyz{cloud.x()[i], cloud.y()[i], cloud.z()[i]} == point) return SiteIdx{i};
  throw std::runtime_error("site de fixture absent");
}
inline std::vector<SiteIdx> all(const Cloud& cloud) {
  std::vector<SiteIdx> ids;
  for (u32 i = 0; i < cloud.sites(); ++i) ids.push_back(SiteIdx{i});
  return ids;
}
inline bool equal(std::span<const SiteIdx> a, std::span<const SiteIdx> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
inline bool center_is(const num::Sphere& s, std::array<i64, 3> n, i64 d) {
  for (unsigned j = 0; j < 3; ++j) {
    const auto left = num::multiply(num::to_wide(s.numerator()[j]), num::to_wide(d));
    const auto right = num::multiply(num::to_wide(s.denominator()),
                                     num::to_wide(n[j] - d * s.anchor().coordinates()[j]));
    if (num::compare(left, right) != 0) return false;
  }
  return true;
}
inline bool level_is(const num::Sphere& s, i64 n, i64 d) {
  const auto expected = num::Level::make(num::to_wide(n), num::to_wide(d));
  return expected.ok() && num::compare(s.level(), expected.value()) == 0;
}
inline bool support_strict(const Cloud& cloud, const BoundedMeb& result) {
  const auto ids = result.support();
  if (ids.empty() || ids.size() > 4) return false;
  std::array<num::Point, 4> p{};
  for (std::size_t i = 0; i < ids.size(); ++i) {
    const auto s = idx(ids[i]);
    if (s >= cloud.sites() || (i > 0 && idx(ids[i - 1]) >= s)) return false;
    p[i] = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]).value();
    const auto side = num::side(result.sphere(), p[i]);
    if (!side.ok() || side.value() != 0) return false;
  }
  if (ids.size() == 1) return level_is(result.sphere(), 0, 1);
  if (ids.size() == 2) return num::is_midpoint(result.sphere(), p[0], p[1]);
  if (ids.size() == 3) return num::strictly_acute(p[0], p[1], p[2]);
  const auto inside = num::strictly_inside(result.sphere(), p[0], p[1], p[2], p[3]);
  return inside.ok() && inside.value();
}
inline u64 presentations(u64 n) {
  u64 total = 0, choose = 1;
  for (u64 q = 1; q <= 4 && q <= n; ++q) { choose = choose * (n + 1 - q) / q; total += choose; }
  return total;
}
inline Input square() { return Input({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0}, {2, 2, 0}}); }
inline bool square_census(const Cloud& cloud, const Census& census, u32 threshold) {
  const auto center = site(cloud, {2, 2, 0});
  if (census.interior().size() != 1 || census.interior()[0] != center) return false;
  if (threshold == 1) return census.kind() == CensusKind::saturated && census.shell().empty();
  std::vector<SiteIdx> shell;
  for (auto s : all(cloud)) if (s != center) shell.push_back(s);
  return census.kind() == CensusKind::complete && equal(census.shell(), shell);
}
}  // namespace tower_test
