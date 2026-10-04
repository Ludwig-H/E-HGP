// Candidat q3 ferme (audit heritage 1235da4ac) : memes ancre, N/D, certificats et Level brut que Sphere::through3 ;
// le Level n'existe qu'a materialize. Fixtures gravees : tetraedre entier 0/2 et son homothetique au bord du profil.
#include <array>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
template <class T>
concept HasLevel = requires(const T& value) { value.level(); };

static_assert(!HasLevel<Q3Candidate> && HasLevel<Sphere>);
static_assert(!std::is_default_constructible_v<Q3Candidate>);
static_assert(!std::is_constructible_v<Q3Candidate, Point, std::array<CenterInt, 3>, CenterDen>);
static_assert(!std::is_convertible_v<Q3Candidate, Sphere> && !std::is_convertible_v<Q3Candidate, Q4Candidate>);

Point point(i64 x, i64 y, i64 z) {
  auto result = Point::make(x, y, z);
  if (!result.ok()) throw std::runtime_error("fixture q3 hors domaine");
  return result.value();
}

template <int N>
Wide<8> widen(const Wide<N>& value) {
  Wide<8> out;
  if (!resize(value, out)) throw std::runtime_error("elargissement de fixture");
  return out;
}

// Level brut ecrit ici a la main : |u|^2 |v|^2 |c-b|^2 / (4 |u x v|^2), sans PGCD ni |N|^2/D^2.
std::pair<Wide<8>, Wide<8>> raw_level(Point a, Point b, Point c) {
  auto diff = [](Point p, Point q) {
    return std::array<i128, 3>{i128{p.x()} - q.x(), i128{p.y()} - q.y(), i128{p.z()} - q.z()};
  };
  auto norm = [](const std::array<i128, 3>& v) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; };
  const auto u = diff(b, a), v = diff(c, a), bc = diff(c, b);
  const std::array<i128, 3> w{u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
  return {widen(multiply(multiply(to_wide(norm(u)), to_wide(norm(v))), to_wide(norm(bc)))),
          widen(multiply(to_wide(i128{4}), to_wide(norm(w))))};
}

void check_raw(const Level& level, const std::pair<Wide<8>, Wide<8>>& expected) {
  CHECK_EQ(compare(widen(to_wide(level.numerator())), expected.first), 0);
  CHECK_EQ(compare(widen(to_wide(level.denominator())), expected.second), 0);
}

// Candidat et fabrique complete : memes champs, memes predicats, Level brut identique et repetable.
void check_against_eager(Point a, Point b, Point c, const std::array<Point, 6>& queries) {
  const auto made = Q3Candidate::through(a, b, c);
  const auto eager = Sphere::through(a, b, c);
  REQUIRE(made.ok() && eager.ok());
  CHECK_EQ(made.value().has_value(), eager.value().has_value());
  if (!made.value() || !eager.value()) return;
  const auto& candidate = *made.value();
  const auto& sphere = *eager.value();
  CHECK_EQ(candidate.presentation_arity(), 3);
  CHECK(candidate.anchor() == a && sphere.anchor() == a);
  CHECK(candidate.numerator() == sphere.numerator());
  CHECK_EQ(candidate.denominator(), sphere.denominator());
  CHECK(candidate.denominator() > 0);
  CHECK_EQ(candidate.q3_power_i128_certified(), sphere.q3_power_i128_certified());
  CHECK_EQ(candidate.orientation_i128_certified(), sphere.orientation_i128_certified());
  const auto first = candidate.materialize(), second = candidate.materialize();
  REQUIRE(first.ok() && second.ok());
  CHECK_EQ(first.value().presentation_arity(), 3);
  CHECK(first.value().numerator() == sphere.numerator());
  CHECK_EQ(first.value().denominator(), sphere.denominator());
  CHECK_EQ(first.value().q3_power_i128_certified(), sphere.q3_power_i128_certified());
  CHECK_EQ(first.value().orientation_i128_certified(), sphere.orientation_i128_certified());
  const auto raw = raw_level(a, b, c);
  check_raw(first.value().level(), raw);
  check_raw(second.value().level(), raw);
  check_raw(sphere.level(), raw);
  for (const auto& query : queries) {
    const auto value = power(candidate, query), full = power(sphere, query);
    const auto sign = side(candidate, query);
    REQUIRE(value.ok() && full.ok() && sign.ok());
    const auto wide = num_test::wide_power(candidate, query);
    CHECK(num_test::equals_wide(value.value(), wide));
    CHECK_EQ(compare(widen(to_wide(value.value())), widen(to_wide(full.value()))), 0);
    CHECK_EQ(sign.value(), wide.sign());
  }
  for (const Point& vertex : {a, b, c}) {
    const auto sign = side(candidate, vertex);
    REQUIRE(sign.ok());
    CHECK_EQ(sign.value(), 0);
  }
  const auto plane = orientation(a, b, c, candidate), full_plane = orientation(a, b, c, sphere);
  REQUIRE(plane.ok() && full_plane.ok());
  CHECK_EQ(plane.value(), 0);
  CHECK_EQ(full_plane.value(), 0);
  CHECK_EQ(is_midpoint(candidate, a, b), is_midpoint(sphere, a, b));
}
}  // namespace

MHGP11_TEST(q3_candidate, 500) {
  const i64 m = kCoordMax;
  // Tetraedre entier F (ordre Morton 0,24,40,48) : faces strictes, chacune rejette le sommet oppose.
  const std::array<Point, 4> f{point(0, 0, 0), point(2, 2, 0), point(2, 0, 2), point(0, 2, 2)};
  const auto face = Q3Candidate::through(f[0], f[1], f[2]);
  REQUIRE(face.ok() && face.value());
  CHECK(face.value()->numerator() == (std::array<CenterInt, 3>{128, 64, 64}));
  CHECK_EQ(face.value()->denominator(), CenterDen{96});
  CHECK(face.value()->q3_power_i128_certified());
  const auto opposite = power(*face.value(), f[3]);
  REQUIRE(opposite.ok());
  CHECK_EQ(compare(widen(to_wide(opposite.value())), widen(to_wide(i128{256}))), 0);
  const auto level = face.value()->materialize();
  REQUIRE(level.ok());
  CHECK_EQ(compare(widen(to_wide(level.value().level().numerator())), widen(to_wide(i128{512}))), 0);
  CHECK_EQ(compare(widen(to_wide(level.value().level().denominator())), widen(to_wide(i128{192}))), 0);
  const std::array<std::array<int, 4>, 4> faces{{{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 3, 1}, {1, 2, 3, 0}}};
  for (const auto& t : faces) {
    const auto candidate = Q3Candidate::through(f[t[0]], f[t[1]], f[t[2]]);
    REQUIRE(candidate.ok() && candidate.value());
    const auto sign = side(*candidate.value(), f[t[3]]);
    REQUIRE(sign.ok());
    CHECK_EQ(sign.value(), 1);
  }
  // Homothetique au bord du profil : s=2^(B-1)-1, F*s+(1,1,1). La puissance du sommet oppose vaut 256 s^6
  // (128 bits en u21, 146 en u24) : la voie native q3 doit suivre son certificat, jamais un retag q4.
  const i64 s = (i64{1} << (kCoordBits - 1)) - 1;
  std::array<Point, 4> g{};
  for (int i = 0; i < 4; ++i)
    g[i] = point(i64{f[i].x()} * s + 1, i64{f[i].y()} * s + 1, i64{f[i].z()} * s + 1);
  const auto edge = Q3Candidate::through(g[0], g[1], g[2]);
  REQUIRE(edge.ok() && edge.value());
  const auto edge_power = power(*edge.value(), g[3]);
  REQUIRE(edge_power.ok());
  const auto s2 = multiply(to_wide(i128{s}), to_wide(i128{s}));
  const auto s6 = multiply(multiply(s2, s2), s2);
  CHECK_EQ(compare(widen(to_wide(edge_power.value())), widen(multiply(to_wide(i128{256}), s6))), 0);
  const auto edge_side = side(*edge.value(), g[3]);
  REQUIRE(edge_side.ok());
  CHECK_EQ(edge_side.value(), 1);
  CHECK_EQ(edge.value()->q3_power_i128_certified(), kCoordBits <= 18);
  const std::array<Point, 6> queries{point(0, 0, 0), point(1, 1, 1), point(m, m, m), point(m / 2, m / 2, m / 2),
                                     point(0, m, m), point(m, 0, 3)};
  const std::array<std::array<Point, 3>, 9> triangles{{
    {f[0], f[1], f[2]}, {f[1], f[2], f[3]}, {g[0], g[1], g[2]}, {g[3], g[2], g[1]},
    {point(0, 0, 0), point(4, 0, 0), point(2, 3, 0)},
    {point(10, 5, 5), point(9, 8, 5), point(5, 2, 1)},
    {point(0, 0, 0), point(m, 1, 0), point(m - 1, 1, 0)},
    {point(m, m, 0), point(m, 0, m), point(0, m, m)},
    {point(1, 2, 3), point(m, 7, 11), point(5, m, m - 3)}
  }};
  for (const auto& t : triangles) {
    check_against_eager(t[0], t[1], t[2], queries);
    check_against_eager(t[1], t[2], t[0], queries);
  }
  // Alignements et doublons : aucun candidat, comme la fabrique complete.
  const std::array<std::array<Point, 3>, 3> degenerate{{
    {point(0, 0, 0), point(0, 0, 0), point(1, 2, 3)},
    {point(0, 0, 0), point(1, 1, 1), point(m, m, m)},
    {point(3, 0, 0), point(1, 0, 0), point(2, 0, 0)}
  }};
  for (const auto& t : degenerate) {
    const auto candidate = Q3Candidate::through(t[0], t[1], t[2]);
    const auto complete = Sphere::through(t[0], t[1], t[2]);
    REQUIRE(candidate.ok() && complete.ok());
    CHECK(!candidate.value());
    CHECK(!complete.value());
  }
}
