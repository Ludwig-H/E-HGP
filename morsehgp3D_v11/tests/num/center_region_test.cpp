// Rejets de regions de centres : domaine T0, contacts fermes, degenerescences et faces q4 obtuses.
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point pt(std::array<i64, 3> xyz) {
  auto made = Point::make(xyz[0], xyz[1], xyz[2]);
  if (!made.ok()) throw std::runtime_error("point region invalide");
  return made.value();
}

CenterRegion box(std::array<i64, 3> lo, std::array<i64, 3> hi) {
  auto made = CenterRegion::make(lo, hi);
  if (!made.ok()) throw std::runtime_error("region invalide");
  return made.value();
}

struct PairCase {
  std::array<i64, 3> a, b, lo, hi;
  bool meets;
};
}  // namespace

static_assert(!std::is_default_constructible_v<CenterRegion>);
static_assert(!std::is_constructible_v<CenterRegion, std::array<i64, 3>, std::array<i64, 3>>);
static_assert(std::is_nothrow_copy_constructible_v<CenterRegion>);

MHGP11_TEST(region_domain, 28) {
  constexpr i64 maximum = i64{1} << kCoordBits;
  for (int axis = 0; axis < 3; ++axis)
    for (int mode = 0; mode < 4; ++mode) {
      std::array<i64, 3> lo{0, 0, 0}, hi{1, 1, 1};
      if (mode == 0) lo[axis] = -1;
      if (mode == 1) hi[axis] = 0;
      if (mode == 2) lo[axis] = 2;
      if (mode == 3) hi[axis] = maximum + 1;
      const auto refused = CenterRegion::make(lo, hi);
      CHECK(!refused.ok());
      CHECK(refused.outcome().reason == Reason::parameter_out_of_range);
    }
  std::array<i64, 3> lo{0, 0, 0}, hi{maximum, maximum, maximum};
  const auto made = CenterRegion::make(lo, hi);
  REQUIRE(made.ok());
  lo.fill(9); hi.fill(9);
  CHECK((made.value().lo() == std::array<i64, 3>{0, 0, 0}));
  CHECK((made.value().hi() == std::array<i64, 3>{maximum, maximum, maximum}));
  const auto copy = made.value();
  CHECK(copy.hi() == made.value().hi());
  CHECK(!CenterRegion::make({std::numeric_limits<i64>::min(), 0, 0}, {1, 1, 1}).ok());
}

MHGP11_TEST(region_pair, 28) {
  const i64 m = kCoordMax, maximum = m + 1;
  const std::array<PairCase, 13> cases{{
      {{0, 0, 0}, {4, 0, 0}, {0, 0, 0}, {2, 2, 2}, true},
      {{0, 0, 0}, {4, 0, 0}, {2, 0, 0}, {3, 2, 2}, true},
      {{0, 0, 0}, {4, 0, 0}, {0, 0, 0}, {1, 2, 2}, false},
      {{0, 0, 0}, {4, 0, 0}, {3, 0, 0}, {4, 2, 2}, false},
      {{0, 0, 0}, {2, 2, 0}, {0, 0, 0}, {1, 1, 1}, true},
      {{0, 0, 0}, {2, 2, 2}, {0, 0, 0}, {1, 1, 1}, true},
      {{0, 0, 0}, {2, 2, 2}, {1, 1, 1}, {2, 2, 2}, true},
      {{0, 0, 0}, {4, 4, 4}, {0, 0, 0}, {1, 1, 1}, false},
      {{0, 0, 0}, {m, m, m}, {0, 0, 0}, {maximum, maximum, maximum}, true},
      {{0, 0, 0}, {m, m, m}, {0, 0, 0}, {1, 1, 1}, false},
      {{0, 0, 0}, {m, m, m}, {m - 1, m - 1, m - 1}, {maximum, maximum, maximum}, false},
      {{0, 0, 0}, {0, 0, 0}, {m, m, m}, {maximum, maximum, maximum}, true},
      {{m, m, m}, {m, m, m}, {0, 0, 0}, {1, 1, 1}, true},
  }};
  for (const auto& fixture : cases) {
    const auto region = box(fixture.lo, fixture.hi);
    CHECK(bisector_meets(pt(fixture.a), pt(fixture.b), region) == fixture.meets);
    CHECK(bisector_meets(pt(fixture.b), pt(fixture.a), region) == fixture.meets);
  }
  CHECK(bisector_meets(pt({m, 0, 0}), pt({0, m, 0}), box({m, m, 0}, {maximum, maximum, 1})));
  CHECK(bisector_meets(pt({m, 0, 0}), pt({0, 0, m}), box({m, 0, m}, {maximum, 1, maximum})));
}

MHGP11_TEST(region_line, 130) {
  const i64 m = kCoordMax, maximum = m + 1;
  const auto triangle = std::array<Point, 3>{pt({0, 0, 0}), pt({4, 0, 0}), pt({0, 4, 0})};
  const std::array<CenterRegion, 5> regions{box({0, 0, 0}, {2, 2, 1}), box({2, 2, 0}, {3, 3, 1}),
      box({0, 1, 0}, {2, 3, 1}), box({0, 0, 0}, {1, 2, 1}), box({3, 2, 0}, {4, 3, 1})};
  std::array<int, 3> permutation{0, 1, 2};
  do {
    for (std::size_t i = 0; i < regions.size(); ++i)
      CHECK(center_line_meets(triangle[permutation[0]], triangle[permutation[1]], triangle[permutation[2]],
                             regions[i]) == (i < 3 ? CenterLineRelation::intersects : CenterLineRelation::disjoint));
    CHECK(center_line_meets(pt({2, 2, 1}), pt({1, 2, 2}), pt({0, 0, 1}), box({0, 0, 0}, {1, 1, 1})) ==
          CenterLineRelation::intersects);  // Contact au seul sommet (1,1,1).
    CHECK(center_line_meets(pt({0, 0, 0}), pt({m, 0, 0}), pt({0, m, 0}), box({0, 0, 0}, {1, 1, 1})) ==
          CenterLineRelation::disjoint);  // Produit cubique depassant i64 en B21/B24.
    CHECK(center_line_meets(pt({0, 0, 0}), pt({m, 0, 0}), pt({0, m, 0}),
                           box({0, 0, 0}, {maximum, maximum, maximum})) == CenterLineRelation::intersects);
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  const auto domain = box({0, 0, 0}, {maximum, maximum, maximum});
  CHECK(center_line_meets(triangle[0], triangle[0], triangle[2], domain) == CenterLineRelation::degenerate);
  CHECK(center_line_meets(pt({0, 0, 0}), pt({1, 1, 1}), pt({m, m, m}), domain) == CenterLineRelation::degenerate);
  const std::array<Point, 3> missed{pt({7, 4, 2}), pt({7, 0, 1}), pt({7, 3, 4})};
  const auto small = box({0, 0, 0}, {2, 2, 2});
  CHECK(bisector_meets(missed[0], missed[1], small));
  CHECK(bisector_meets(missed[0], missed[2], small));
  CHECK(bisector_meets(missed[1], missed[2], small));
  CHECK(center_line_meets(missed[0], missed[1], missed[2], small) == CenterLineRelation::disjoint);
  CHECK(center_line_meets(pt({4, 7, 2}), pt({2, 2, 7}), pt({6, 2, 2}), small) ==
        CenterLineRelation::disjoint);  // Seule la normale k=2 separe le zonogone de l'origine.
  const std::array<Point, 4> tetra{pt({1, 2, 6}), pt({8, 4, 8}), pt({2, 1, 3}), pt({7, 8, 5})};
  const auto sphere = Sphere::through(tetra[0], tetra[1], tetra[2], tetra[3]);
  REQUIRE(sphere.ok() && sphere.value());
  const auto positive = strictly_inside(*sphere.value(), tetra[0], tetra[1], tetra[2], tetra[3]);
  REQUIRE(positive.ok() && positive.value());
  CHECK(!strictly_acute(tetra[0], tetra[1], tetra[2]));
  // Centre exact (397,331,391)/82 dans [4,5]^3 ; toutes les faces, meme obtuses, survivent.
  std::array<int, 4> order{0, 1, 2, 3};
  do {
    for (int omitted = 0; omitted < 4; ++omitted) {
      std::array<Point, 3> face{};
      int at = 0;
      for (int i = 0; i < 4; ++i)
        if (i != omitted) face[at++] = tetra[order[i]];
      CHECK(center_line_meets(face[0], face[1], face[2], box({4, 4, 4}, {5, 5, 5})) ==
            CenterLineRelation::intersects);
    }
  } while (std::next_permutation(order.begin(), order.end()));
}
