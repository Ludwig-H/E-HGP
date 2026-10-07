// Distance native : faits analytiques, sens des soustractions, seuils et comparaison aux deux voies q1 anterieures.
#include <array>
#include <stdexcept>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  const auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("distance fixture hors domaine");
  return made.value();
}
}  // namespace

MHGP12_TEST(distance, 525) {
  // i64 tant que 3(2^B-1)^2 y tient (profils 21 et 24), i128 au profil 32 (NUM-REQUETE).
  static_assert(std::same_as<decltype(squared_distance(Point{}, Point{})), std::conditional_t<(kCoordBits <= 30), i64, i128>>);
  static_assert(noexcept(squared_distance(Point{}, Point{})));
  const DotInt m = kCoordMax;
  std::array<Point, 8> corners{};
  for (u32 i = 0; i < corners.size(); ++i)
    corners[i] = point((i & 1u) ? m : 0, (i & 2u) ? m : 0, (i & 4u) ? m : 0);
  for (u32 i = 0; i < corners.size(); ++i) {
    for (u32 j = 0; j < corners.size(); ++j) {
      const u32 mask = i ^ j;
      const DotInt differing = DotInt{mask & 1u} + ((mask >> 1) & 1u) + ((mask >> 2) & 1u);
      const auto actual = squared_distance(corners[i], corners[j]);
      CHECK_EQ(actual, differing * m * m);  // Nombre d'axes opposes, sans recopier trois differences.
      CHECK_EQ(squared_distance(corners[j], corners[i]), actual);
      CHECK(actual >= 0);
      CHECK(actual < (DotInt{1} << (2 * kCoordBits + 2)));
      const auto old = power(Sphere::point(corners[i]), corners[j]);
      REQUIRE(old.ok());
      CHECK(num_test::equals_wide(actual, num_test::wide_power(Sphere::point(corners[i]), corners[j])));
      CHECK(num_test::equals_wide(old.value(), num_test::wide_power(Sphere::point(corners[i]), corners[j])));
      CHECK_EQ(actual == 0, i == j);
    }
  }
  const auto origin = corners[0], top = corners[7];
  for (int axis = 0; axis < 3; ++axis) {
    std::array<i64, 3> near{i64{kCoordMax}, i64{kCoordMax}, i64{kCoordMax}};
    --near[axis];
    const auto neighbor = point(near[0], near[1], near[2]);
    CHECK_EQ(squared_distance(top, neighbor), 1);
    CHECK_EQ(squared_distance(neighbor, top), 1);
    CHECK_EQ(squared_distance(origin, neighbor), 3 * m * m - 2 * m + 1);
  }
  CHECK_EQ(squared_distance(Point{}, origin), 0);
  CHECK_EQ(squared_distance(point(1, 2, 3), point(4, 6, 3)), 25);
  CHECK_EQ(squared_distance(point(i64{kCoordMax} - 3, i64{kCoordMax} - 4, kCoordMax), top), 25);
  CHECK_EQ(squared_distance(origin, top), 3 * m * m);
}
