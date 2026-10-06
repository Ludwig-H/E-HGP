// Donnees de test et comparaisons independantes des parcours de l'index.
#pragma once
#include <algorithm>
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
  std::vector<std::array<u32, 3>> points;
  explicit Input(const std::vector<std::array<u32, 3>>& given) : points(given) {
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

// Recurrence independante de l'arbre radix de Morton : cles recalculees bit a bit (pas les tables d'ecartement de
// cloud/morton.hpp), positions distinctes triees par cle, coupe au plus haut bit qui differe trouvee par balayage
// lineaire (pas la recherche dichotomique du produit). Racine a profondeur 1, comme GlobalIndex::max_depth.
__extension__ typedef unsigned __int128 RadixKey;
inline RadixKey interleave(const std::array<u32, 3>& p) {
  RadixKey key = 0;
  for (int bit = 0; bit < 32; ++bit)
    for (int axis = 0; axis < 3; ++axis)
      if ((p[axis] >> bit) & 1u) key |= RadixKey{1} << (3 * bit + axis);
  return key;
}
struct Shape { u64 nodes = 0, depth = 0; };
inline Shape radix_shape(const std::vector<RadixKey>& keys, std::size_t begin, std::size_t end, u32 leaf) {
  if (end - begin <= leaf) return {1, 1};
  const RadixKey differ = keys[begin] ^ keys[end - 1];
  RadixKey top = 1;
  while (top <= differ / 2) top <<= 1;  // plus haut bit de differ
  std::size_t split = begin;
  while (split < end && (keys[split] & top) == 0) ++split;
  const Shape left = radix_shape(keys, begin, split, leaf), right = radix_shape(keys, split, end, leaf);
  return {1 + left.nodes + right.nodes, 1 + std::max(left.depth, right.depth)};
}
inline Shape radix_shape(const std::vector<std::array<u32, 3>>& points, u32 leaf) {
  std::vector<RadixKey> keys;
  for (const auto& p : points) keys.push_back(interleave(p));
  std::sort(keys.begin(), keys.end());
  keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
  return radix_shape(keys, 0, keys.size(), leaf);
}
}  // namespace index_test
