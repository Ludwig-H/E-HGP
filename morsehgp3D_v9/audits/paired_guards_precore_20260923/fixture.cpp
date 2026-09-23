#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

using namespace mhgp9::gen;
namespace {
using i64 = std::int64_t;
using i128 = __int128_t;
using Vec = std::array<i64, 3>;

Vec vec(Point3 p) { return {p[0], p[1], p[2]}; }
Vec diff(Vec a, Vec b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
i64 dot(Vec a, Vec b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
i128 cross2(Vec a, Vec b) {
  const i128 x = static_cast<i128>(a[1])*b[2] - static_cast<i128>(a[2])*b[1];
  const i128 y = static_cast<i128>(a[2])*b[0] - static_cast<i128>(a[0])*b[2];
  const i128 z = static_cast<i128>(a[0])*b[1] - static_cast<i128>(a[1])*b[0];
  return x*x + y*y + z*z;
}
Vec twice_from_mid(Point3 g, Point3 a, Point3 b) {
  const auto G = vec(g), A = vec(a), B = vec(b);
  return {2*G[0]-A[0]-B[0], 2*G[1]-A[1]-B[1], 2*G[2]-A[2]-B[2]};
}
bool singleton(Point3 a, Point3 b, Point3 g, unsigned lane) {
  const auto d = diff(vec(b), vec(a));
  const auto w = twice_from_mid(g, a, b);
  const i64 H = dot(d, d) - dot(w, w);
  const i128 X = cross2(d, w), H2 = static_cast<i128>(H)*H;
  return H > 0 && (lane == 3 ? 3*H2 > 4*X : H2 > 2*X);
}
bool pair(Point3 a, Point3 b, Point3 g, Point3 h, unsigned lane) {
  const auto d = diff(vec(b), vec(a));
  const auto wg = twice_from_mid(g, a, b), wh = twice_from_mid(h, a, b);
  const Vec sum{wg[0]+wh[0], wg[1]+wh[1], wg[2]+wh[2]};
  const i64 H = 2*dot(d, d) - dot(wg, wg) - dot(wh, wh);
  const i128 X = cross2(d, sum), H2 = static_cast<i128>(H)*H;
  return H > 0 && (lane == 3 ? 3*H2 > 4*X : H2 > 2*X);
}
bool disjoint_pairs(const std::vector<std::array<std::size_t, 2>>& pairs) {
  std::set<std::size_t> seen;
  for (const auto [g, h] : pairs)
    if (g < 2 || h < 2 || !seen.insert(g).second || !seen.insert(h).second) return false;
  return true;
}
}  // namespace

int main() {
  // This is the translated K5 example: all sites lie on the u18 integer grid.
  const std::vector<Point3> points{{5,10,10}, {15,10,10},
      {8,13,10}, {8,7,10}, {9,13,10}, {9,7,10},
      {10,13,10}, {10,7,10}, {11,13,10}, {11,7,10}};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  Q34WitnessSearchWork search{};
  Q34WitnessBoundsWork bounds{};
  std::vector<Q34WitnessNode> trace;
  const auto surviving = filter_q34_witnesses(*index, points[0], points[1],
                                               5, 6, search, bounds, trace);
  if (surviving != 6 || search.q3_credits != 0 || search.q4_credits != 0)
    throw std::runtime_error("product S2 rejected the fixture or found a singleton universal guard");
  const std::vector<std::array<std::size_t, 2>> pairs{{2,3},{4,5},{6,7},{8,9}};
  if (!disjoint_pairs(pairs) || disjoint_pairs({{2,3},{2,5}}))
    throw std::runtime_error("pair IDs were not checked for disjointness");
  for (std::size_t g = 2; g < points.size(); ++g)
    if (singleton(points[0], points[1], points[g], 3) ||
        singleton(points[0], points[1], points[g], 4))
      throw std::runtime_error("a guard was individually universal");
  for (const auto [g,h] : pairs)
    if (!pair(points[0], points[1], points[g], points[h], 3) ||
        !pair(points[0], points[1], points[g], points[h], 4))
      throw std::runtime_error("paired guard failed its exact q3/q4 inequality");
  std::cout << "PASS surviving=" << unsigned(surviving)
            << " q3_credits=" << search.q3_credits
            << " q4_credits=" << search.q4_credits
            << " disjoint_pairs=" << pairs.size()
            << " q3_closed=1 q4_closed=1\n";
}
