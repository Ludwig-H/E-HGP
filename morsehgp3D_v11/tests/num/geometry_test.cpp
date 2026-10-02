// Fixtures permanentes : degenerescences admises, sphere fermee, poids stricts et extremes de chaque profil.
#include <array>
#include <stdexcept>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point p(i64 x, i64 y, i64 z) {
  auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("fixture hors domaine");
  return made.value();
}
}  // namespace

MHGP11_TEST(domain, 10) {
  CHECK(Point::make(0, 0, 0).ok());
  CHECK(Point::make(kCoordMax, kCoordMax, kCoordMax).ok());
  for (const auto& coordinates : {std::array<i64, 3>{-1, 0, 0}, {0, -1, 0}, {0, 0, -1},
                                  {i64{kCoordMax} + 1, 0, 0}, {0, i64{kCoordMax} + 1, 0}, {0, 0, i64{kCoordMax} + 1}}) {
    const auto bad = Point::make(coordinates[0], coordinates[1], coordinates[2]);
    CHECK(!bad.ok());
    CHECK_EQ(bad.outcome().reason, Reason::coordinate_out_of_domain);
  }
}

MHGP11_TEST(geometry, 27) {
  const auto a = p(0, 0, 0), b = p(2, 0, 0), c = p(0, 2, 0), d = p(0, 0, 2);
  const auto ab = Sphere::through(a, b);
  REQUIRE(ab.ok() && ab.value());
  CHECK_EQ(side(*ab.value(), a).value(), 0);
  CHECK_EQ(side(*ab.value(), b).value(), 0);
  CHECK_EQ(side(*ab.value(), p(1, 0, 0)).value(), -1);
  CHECK_EQ(side(*ab.value(), c).value(), 1);
  CHECK(is_midpoint(*ab.value(), a, b));
  CHECK(!is_midpoint(*ab.value(), a, c));
  const auto abc = Sphere::through(a, b, c);
  REQUIRE(abc.ok() && abc.value());
  CHECK_EQ(side(*abc.value(), p(2, 2, 0)).value(), 0);
  CHECK(!strictly_acute(a, b, c));
  CHECK(strictly_acute(p(0, 0, 0), p(4, 0, 0), p(2, 3, 0)));
  const auto abcd = Sphere::through(a, b, c, d);
  REQUIRE(abcd.ok() && abcd.value());
  CHECK_EQ(orientation(a, b, c, d), 8);
  CHECK_EQ(orientation(a, c, b, d), -8);
  CHECK(!strictly_inside(*abcd.value(), a, b, c, d).value());
  const auto e = p(2, 2, 0), f = p(2, 0, 2), g = p(0, 2, 2);
  const auto regular = Sphere::through(a, e, f, g);
  REQUIRE(regular.ok() && regular.value());
  CHECK(strictly_inside(*regular.value(), a, e, f, g).value());
  CHECK_EQ(orientation(a, b, c, *ab.value()).value(), 0);
  CHECK_EQ(orientation(a, b, c, *regular.value()).value(), 1);
  // Un poids exactement nul : le centre q4 reste celui du triangle aigu abc, sur une face du tetraedre.
  const auto h = p(4, 0, 0), i = p(2, 3, 0), j = p(2, 0, 2);
  const auto face_center = Sphere::through(a, h, i, j), triangle_center = Sphere::through(a, h, i);
  REQUIRE(face_center.ok() && face_center.value() && triangle_center.ok() && triangle_center.value());
  CHECK(!strictly_inside(*face_center.value(), a, h, i, j).value());
  CHECK_EQ(compare(face_center.value()->level(), triangle_center.value()->level()), 0);
  CHECK_EQ(orientation(a, h, i, *face_center.value()).value(), 0);
  CHECK_EQ(side(Sphere::point(a), a).value(), 0);
  CHECK_EQ(side(Sphere::point(a), b).value(), 1);
  for (const auto& rejected : {Sphere::through(a, a), Sphere::through(a, a, c),
                              Sphere::through(a, b, p(4, 0, 0)), Sphere::through(a, b, c, p(2, 2, 0))}) {
    CHECK(rejected.ok());
    CHECK(!rejected.value());
  }
}

MHGP11_TEST(extremes, 15) {
  const i64 m = kCoordMax;
  const std::array<Point, 4> points = {p(0, 0, 0), p(m, m, 0), p(m, 0, m), p(0, m, m)};
  const auto sphere = Sphere::through(points[0], points[1], points[2], points[3]);
  REQUIRE(sphere.ok() && sphere.value());
  CHECK(strictly_inside(*sphere.value(), points[0], points[1], points[2], points[3]).value());
  for (auto point : points) CHECK_EQ(side(*sphere.value(), point).value(), 0);
  CHECK_EQ(side(*sphere.value(), p(m, m, m)).value(), 0);
  CHECK_EQ(side(*sphere.value(), p(m / 2, m / 2, m / 2)).value(), -1);
  const auto triangle = Sphere::through(points[0], points[1], points[2]);
  REQUIRE(triangle.ok() && triangle.value());
  for (auto point : {points[0], points[1], points[2]}) CHECK_EQ(side(*triangle.value(), point).value(), 0);
  CHECK_EQ(compare(triangle.value()->level(), sphere.value()->level()), -1);
  CHECK_EQ(orientation(points[0], points[1], points[2], points[3]), -2 * i128{m} * m * m);
  auto near_flat = Sphere::through(p(0, 0, 0), p(m, 1, 0), p(m - 1, 1, 0));
  REQUIRE(near_flat.ok() && near_flat.value());
  CHECK_EQ(side(*near_flat.value(), p(m - 1, 1, 0)).value(), 0);
  CHECK_EQ(side(*near_flat.value(), p(0, m, m)).value(), 1);
  CHECK(!is_midpoint(*near_flat.value(), p(0, m, m), p(m, m, m)));
}
