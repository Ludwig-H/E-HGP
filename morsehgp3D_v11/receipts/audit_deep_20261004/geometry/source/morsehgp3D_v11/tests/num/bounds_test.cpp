// Boites fermees : domaine, contacts, singletons et puissances q3 hors i128 ; aucun oracle par epsilon.
#include <array>
#include <stdexcept>
#include <type_traits>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("fixture bounds hors domaine");
  return made.value();
}

Box box(Point lo, Point hi) {
  auto made = Box::make(lo, hi);
  if (!made.ok()) throw std::runtime_error("fixture bounds boite inversee");
  return made.value();
}

Sphere sphere(u8 q, const std::array<Point, 4>& p) {
  if (q == 1) return Sphere::point(p[0]);
  auto made = q == 2 ? Sphere::through(p[0], p[1]) :
              q == 3 ? Sphere::through(p[0], p[1], p[2]) : Sphere::through(p[0], p[1], p[2], p[3]);
  if (!made.ok() || !made.value()) throw std::runtime_error("fixture bounds support degenere");
  return *made.value();
}
}  // namespace

static_assert(!std::is_default_constructible_v<Box> && !std::is_constructible_v<Box, Point, Point>);
static_assert(std::is_nothrow_copy_constructible_v<Box>);

MHGP11_TEST(bounds, 360) {
  const Point zero = point(0, 0, 0), one = point(1, 1, 1);
  for (const Point hi : {point(0, 1, 1), point(1, 0, 1), point(1, 1, 0)}) {
    const auto invalid = Box::make(one, hi);
    CHECK(!invalid.ok());
    CHECK_EQ(invalid.outcome().reason, Reason::parameter_out_of_range);
  }
  const auto singleton = Box::make(one, one);
  REQUIRE(singleton.ok());
  CHECK(singleton.value().lo() == one && singleton.value().hi() == one);
  const auto copy = singleton.value();
  CHECK(copy.lo() == one && copy.hi() == one);
  const i64 m = kCoordMax;
  const std::array<Point, 4> points{zero, point(m, m, 0), point(m, 0, m), point(0, m, m)};
  for (u8 q = 1; q <= 4; ++q) {
    const auto geometry = sphere(q, points);
    REQUIRE(geometry.presentation_arity() == q);
    for (const Point query : {zero, one, points[1], points[3], point(m, m, m)}) {
      const auto bounds = power_bounds(geometry, box(query, query));
      REQUIRE(bounds.ok());
      const auto expected = num_test::wide_power(geometry, query);
      CHECK(num_test::equals_wide(bounds.value().lower, expected));
      CHECK(num_test::equals_wide(bounds.value().upper, expected));
    }
    const auto bounds = power_bounds(geometry, box(zero, point(m, m, m)));
    REQUIRE(bounds.ok());
    for (const Point corner : {zero, point(m, 0, 0), point(0, m, 0), point(0, 0, m),
                               point(m, m, 0), point(m, 0, m), point(0, m, m), point(m, m, m)}) {
      Wide<4> lower, upper;
      REQUIRE(resize(to_wide(bounds.value().lower), lower) && resize(to_wide(bounds.value().upper), upper));
      const auto actual = num_test::wide_power(geometry, corner);
      CHECK(compare(lower, actual) <= 0);
      CHECK(compare(upper, actual) >= 0);
    }
  }
  const auto pair = Sphere::through(zero, point(4, 0, 0));
  REQUIRE(pair.ok() && pair.value());
  const auto contact = power_bounds(*pair.value(), box(point(4, 0, 0), point(4, 0, 1)));
  REQUIRE(contact.ok());
  CHECK_EQ(to_wide(contact.value().lower).sign(), 0);
  CHECK(num_test::equals_wide(contact.value().upper, Wide<4>::from_u64(2)));
  const auto inside = power_bounds(*pair.value(), box(point(2, 0, 0), point(2, 0, 0)));
  REQUIRE(inside.ok());
  CHECK(num_test::equals_wide(inside.value().lower, Wide<4>::from_i128(-8)));
  CHECK(num_test::equals_wide(inside.value().upper, Wide<4>::from_i128(-8)));
  const auto loose = power_bounds(*pair.value(), box(zero, point(4, 0, 0)));
  REQUIRE(loose.ok());
  CHECK(num_test::equals_wide(loose.value().lower, Wide<4>::from_i128(-32)));
  CHECK(num_test::equals_wide(loose.value().upper, Wide<4>::from_i128(32)));
  // Signes seuls (parcours census) : contact, interieur strict, intervalle a cheval.
  const std::array<std::array<int, 2>, 3> expected{{{0, 1}, {-1, -1}, {-1, 1}}};
  const std::array<Box, 3> boxes{box(point(4, 0, 0), point(4, 0, 1)), box(point(2, 0, 0), point(2, 0, 0)),
                                  box(zero, point(4, 0, 0))};
  for (std::size_t i = 0; i < boxes.size(); ++i) {
    const auto signs = power_bound_signs(*pair.value(), boxes[i]);
    REQUIRE(signs.ok());
    CHECK_EQ(signs.value().lower, expected[i][0]);
    CHECK_EQ(signs.value().upper, expected[i][1]);
  }
  for (u8 q = 1; q <= 4; ++q) {  // Memes signes que power_bounds aux quatre arites, coins extremes compris.
    const auto geometry = sphere(q, points);
    for (const Point query : {zero, one, points[1], points[3], point(m, m, m)}) {
      for (const Box b : {box(query, query), box(zero, query), box(zero, point(m, m, m))}) {
        const auto bounds = power_bounds(geometry, b);
        const auto signs = power_bound_signs(geometry, b);
        REQUIRE(bounds.ok() && signs.ok());
        CHECK_EQ(signs.value().lower, to_wide(bounds.value().lower).sign());
        CHECK_EQ(signs.value().upper, to_wide(bounds.value().upper).sign());
      }
    }
  }
}
