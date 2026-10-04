// Distance native : faits analytiques, sens des soustractions, seuils et comparaison aux deux voies q1 anterieures.
#include <array>
#include <stdexcept>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  const auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("distance fixture hors domaine");
  return made.value();
}
}  // namespace

MHGP11_TEST(distance, 525) {
  static_assert(std::same_as<decltype(squared_distance(Point{}, Point{})), i64>);
  static_assert(noexcept(squared_distance(Point{}, Point{})));
  const i64 m = kCoordMax;
  std::array<Point, 8> corners{};
  for (u32 i = 0; i < corners.size(); ++i)
    corners[i] = point((i & 1u) ? m : 0, (i & 2u) ? m : 0, (i & 4u) ? m : 0);
  for (u32 i = 0; i < corners.size(); ++i) {
    for (u32 j = 0; j < corners.size(); ++j) {
      const u32 mask = i ^ j;
      const i64 differing = i64{mask & 1u} + ((mask >> 1) & 1u) + ((mask >> 2) & 1u);
      const auto actual = squared_distance(corners[i], corners[j]);
      CHECK_EQ(actual, differing * m * m);  // Nombre d'axes opposes, sans recopier trois differences.
      CHECK_EQ(squared_distance(corners[j], corners[i]), actual);
      CHECK(actual >= 0);
      CHECK(actual < (i64{1} << 50));
      const auto old = power(Sphere::point(corners[i]), corners[j]);
      REQUIRE(old.ok());
      CHECK(num_test::equals_wide(actual, num_test::wide_power(Sphere::point(corners[i]), corners[j])));
      CHECK(num_test::equals_wide(old.value(), num_test::wide_power(Sphere::point(corners[i]), corners[j])));
      CHECK_EQ(actual == 0, i == j);
    }
  }
  const auto origin = corners[0], top = corners[7];
  for (int axis = 0; axis < 3; ++axis) {
    std::array<i64, 3> near{m, m, m};
    --near[axis];
    const auto neighbor = point(near[0], near[1], near[2]);
    CHECK_EQ(squared_distance(top, neighbor), 1);
    CHECK_EQ(squared_distance(neighbor, top), 1);
    CHECK_EQ(squared_distance(origin, neighbor), 3 * m * m - 2 * m + 1);
  }
  CHECK_EQ(squared_distance(Point{}, origin), 0);
  CHECK_EQ(squared_distance(point(1, 2, 3), point(4, 6, 3)), 25);
  CHECK_EQ(squared_distance(point(m - 3, m - 4, m), top), 25);
  CHECK_EQ(squared_distance(origin, top), 3 * m * m);
}
