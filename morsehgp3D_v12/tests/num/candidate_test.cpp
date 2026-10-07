// Candidat q4 ferme : predicats avant materialisation, niveaux non reduits et invariants de possession.
#include <array>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

namespace {
template <class T>
concept HasLevel = requires(const T& value) { value.level(); };

static_assert(!HasLevel<Q4Candidate> && HasLevel<Sphere>);
static_assert(!std::is_default_constructible_v<Q4Candidate>);
static_assert(!std::is_constructible_v<Q4Candidate, Point, std::array<CenterInt, 3>, CenterDen>);
static_assert(!std::is_convertible_v<Q4Candidate, Sphere>);
static_assert(std::is_same_v<decltype(std::declval<const Q4Candidate&>().numerator()),
                             const std::array<CenterInt, 3>&>);

Point point(i64 x, i64 y, i64 z) {
  auto result = Point::make(x, y, z);
  if (!result.ok()) throw std::runtime_error("fixture candidate hors domaine");
  return result.value();
}

struct Fixture {
  std::array<Point, 4> points;
  bool inside, midpoint;
};
}  // namespace

MHGP12_TEST(candidate, 800) {
  // Taille attendue de ce type sans Level sur les ABI G4, pas un format de serialisation.
  CHECK_EQ(sizeof(Q4Candidate), std::size_t{80});
  const i64 m = kCoordMax;
  const Point origin = point(0, 0, 0);
  const std::array<Fixture, 6> fixtures{{
    {{{origin, point(m, m, 0), point(m, 0, m), point(0, m, m)}}, true, false},
    {{{origin, point(4, 0, 0), point(0, 4, 0), point(0, 0, 4)}}, false, false},
    {{{origin, point(4, 0, 0), point(2, 3, 0), point(2, 0, 2)}}, false, false},
    {{{origin, point(m, 1, 0), point(m - 1, 1, 0), point(m, 1, 1)}}, false, false},
    {{{point(10, 5, 5), point(9, 8, 5), point(5, 2, 1), point(1, 5, 8)}}, true, false},
    {{{point(0, 1, 1), point(2, 1, 1), point(1, 2, 1), point(1, 1, 2)}}, false, true}
  }};
  const std::array<Point, 7> queries{origin, point(1, 1, 1), point(4, 0, 0), point(0, 4, 0),
                                    point(m, m, m), point(m / 2, m / 2, m / 2), point(0, m, m)};
  int positive = 0, negative = 0;
  for (const auto& fixture : fixtures) {
    for (int order = 0; order < 2; ++order) {
      auto points = fixture.points;
      if (order != 0) std::swap(points[1], points[2]);
      const auto det = orientation(points[0], points[1], points[2], points[3]);
      positive += det > 0 ? 1 : 0;
      negative += det < 0 ? 1 : 0;
      const auto made = Q4Candidate::through(points[0], points[1], points[2], points[3]);
      REQUIRE(made.ok() && made.value());
      const auto candidate = *made.value();
      REQUIRE(candidate.presentation_arity() == 4);
      const auto anchor = candidate.anchor();
      const auto numerator = candidate.numerator();
      const auto denominator = candidate.denominator();
      CHECK(anchor == points[0]);
      CHECK(denominator > 0);
      CHECK_EQ(denominator, 2 * (det > 0 ? i128{det} : -i128{det}));
      const auto inside = strictly_inside(candidate, points[0], points[1], points[2], points[3]);
      REQUIRE(inside.ok());
      CHECK_EQ(inside.value(), fixture.inside);
      const auto oriented = orientation(points[0], points[1], points[2], candidate);
      REQUIRE(oriented.ok());
      const bool midpoint = is_midpoint(candidate, points[0], points[1]);
      CHECK_EQ(midpoint, fixture.midpoint && order == 0);
      std::array<SideInt, 7> powers{};
      std::array<int, 7> sides{};
      for (std::size_t i = 0; i < queries.size(); ++i) {
        const auto value = power(candidate, queries[i]);
        const auto sign = side(candidate, queries[i]);
        REQUIRE(value.ok() && sign.ok());
        const auto wide = num_test::wide_power(candidate, queries[i]);
        CHECK(num_test::equals_wide(value.value(), wide));
        CHECK_EQ(sign.value(), wide.sign());
        powers[i] = value.value(); sides[i] = sign.value();
      }
      const auto materialized = candidate.materialize(), repeated = candidate.materialize();
      const auto eager = Sphere::through(points[0], points[1], points[2], points[3]);
      REQUIRE(materialized.ok() && repeated.ok() && eager.ok() && eager.value());
      CHECK(candidate.anchor() == anchor);
      CHECK(candidate.numerator() == numerator);
      CHECK_EQ(candidate.denominator(), denominator);
      const auto& sphere = materialized.value();
      CHECK(sphere.numerator() == numerator);
      CHECK_EQ(sphere.denominator(), denominator);
      CHECK(sphere.anchor() == anchor);
      CHECK_EQ(sphere.presentation_arity(), 4);
      CHECK_EQ(compare(to_wide(sphere.level().numerator()), to_wide(repeated.value().level().numerator())), 0);
      CHECK_EQ(compare(to_wide(sphere.level().denominator()), to_wide(repeated.value().level().denominator())), 0);
      CHECK_EQ(compare(to_wide(sphere.level().numerator()), to_wide(eager.value()->level().numerator())), 0);
      CHECK_EQ(compare(to_wide(sphere.level().denominator()), to_wide(eager.value()->level().denominator())), 0);
      for (std::size_t i = 0; i < queries.size(); ++i) {
        const auto value = power(sphere, queries[i]);
        const auto sign = side(sphere, queries[i]);
        REQUIRE(value.ok() && sign.ok());
        CHECK_EQ(compare(to_wide(value.value()), to_wide(powers[i])), 0);
        CHECK_EQ(sign.value(), sides[i]);
      }
      const auto full_orientation = orientation(points[0], points[1], points[2], sphere);
      const auto full_inside = strictly_inside(sphere, points[0], points[1], points[2], points[3]);
      REQUIRE(full_orientation.ok() && full_inside.ok());
      CHECK_EQ(full_orientation.value(), oriented.value());
      CHECK_EQ(full_inside.value(), inside.value());
      CHECK_EQ(is_midpoint(sphere, points[0], points[1]), midpoint);
      points[0] = point(17, 19, 23);
      CHECK(candidate.anchor() == anchor);
    }
  }
  CHECK_EQ(positive, 6);
  CHECK_EQ(negative, 6);
  const std::array<std::array<Point, 4>, 4> degenerate{{
    {origin, origin, origin, origin},
    {origin, point(1, 0, 0), point(0, 1, 0), point(1, 1, 0)},
    {origin, point(1, 0, 0), point(2, 0, 0), point(3, 0, 0)},
    {origin, point(1, 0, 0), point(0, 1, 0), origin}
  }};
  for (const auto& points : degenerate) {
    const auto candidate = Q4Candidate::through(points[0], points[1], points[2], points[3]);
    const auto complete = Sphere::through(points[0], points[1], points[2], points[3]);
    REQUIRE(candidate.ok() && complete.ok());
    CHECK(!candidate.value());
    CHECK(!complete.value());
  }
}
