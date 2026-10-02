// Ordre des centres : presentations distinctes, lexicographie, signes et dernier bit du profil.
#include <array>
#include <cfenv>
#include <initializer_list>
#include <stdexcept>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("point fixture");
  return made.value();
}
Sphere through(std::initializer_list<Point> p) {
  const auto a = p.begin();
  auto made = p.size() == 2 ? Sphere::through(a[0], a[1]) : p.size() == 3 ?
      Sphere::through(a[0], a[1], a[2]) : Sphere::through(a[0], a[1], a[2], a[3]);
  if (!made.ok() || !made.value()) throw std::runtime_error("sphere fixture");
  return *made.value();
}
}  // namespace

MHGP11_TEST(centers, 108) {
  const auto a = point(0, 0, 0), b = point(4, 0, 0), c = point(0, 4, 0), d = point(4, 4, 0);
  const auto triangle = through({a, b, c}), reversed = through({c, b, a});
  const auto diameter = through({a, d}), middle = Sphere::point(point(2, 2, 0));
  for (const auto& sphere : {triangle, reversed, diameter, middle}) {
    CHECK_EQ(compare_centers(sphere, middle), 0);
    CHECK_EQ(compare_centers(middle, sphere), 0);
  }
  const auto tetra = through({a, point(4, 4, 0), point(4, 0, 4), point(0, 4, 4)});
  CHECK_EQ(compare_centers(tetra, Sphere::point(point(2, 2, 2))), 0);
  CHECK_EQ(compare_centers(tetra, triangle), 1);  // Deux premieres coordonnees egales, rayon different.
  const std::array<Sphere, 4> ordered{Sphere::point(point(1, 9, 9)), Sphere::point(point(2, 1, 9)),
                                     Sphere::point(point(2, 2, 0)), Sphere::point(point(2, 2, 1))};
  for (u32 i = 0; i < ordered.size(); ++i)
    for (u32 j = 0; j < ordered.size(); ++j)
      CHECK_EQ(compare_centers(ordered[i], ordered[j]), (i > j) - (i < j));
  const i64 m = kCoordMax;
  const auto outside = through({point(0, 0, 0), point(m, 1, 0), point(m - 1, 1, 0)});
  CHECK_EQ(compare_centers(outside, Sphere::point(point(m - 1, 0, 0))), 1);
  CHECK_EQ(compare_centers(outside, Sphere::point(point(m, 0, 0))), -1);
  // Le centre est (m-1/2, (1-m*(m-1))/2, 0) : y negatif de grandeur quadratique.
  const auto same_x = through({point(m - 1, 0, 0), point(m, 0, 0)});
  CHECK_EQ(compare_centers(outside, same_x), -1);
  const int initial_round = std::fegetround();
  for (int mode : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    REQUIRE(std::fesetround(mode) == 0);
    CHECK_EQ(compare_centers(outside, same_x), -1);
    CHECK_EQ(compare_centers(triangle, reversed), 0);
    for (u32 i = 0; i < ordered.size(); ++i)
      for (u32 j = 0; j < ordered.size(); ++j)
        CHECK_EQ(compare_centers(ordered[i], ordered[j]), (i > j) - (i < j));
  }
  REQUIRE(std::fesetround(initial_round) == 0);
  const auto near = through({point(m - 1, m, m), point(m, m, m)});
  CHECK_EQ(compare_centers(near, Sphere::point(point(m, m, m))), -1);
  CHECK_EQ(compare_centers(near, Sphere::point(point(m - 1, m, m))), 1);
}
