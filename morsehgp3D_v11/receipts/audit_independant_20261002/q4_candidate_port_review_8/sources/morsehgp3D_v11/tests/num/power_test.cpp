// Voies exactes natives/larges : toutes arites, annulations hors i128, profils et copie de la certification.
#include <array>
#include <optional>
#include <stdexcept>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  auto result = Point::make(x, y, z);
  if (!result.ok()) throw std::runtime_error("fixture power hors domaine");
  return result.value();
}

Sphere sphere(u8 arity, const std::array<Point, 4>& points) {
  if (arity == 1) return Sphere::point(points[0]);
  auto result = arity == 2 ? Sphere::through(points[0], points[1]) :
                arity == 3 ? Sphere::through(points[0], points[1], points[2]) :
                             Sphere::through(points[0], points[1], points[2], points[3]);
  if (!result.ok() || !result.value()) throw std::runtime_error("fixture power degeneree");
  return *result.value();
}
}  // namespace

MHGP11_TEST(power_paths, 205) {
  // Tailles attendues sur les ABI natives de la matrice G4 ; aucune promesse de serialisation/ABI publique.
  CHECK_EQ(sizeof(Sphere), kCoordBits == 18 ? std::size_t{128} : kCoordBits == 21 ? std::size_t{144} : std::size_t{160});
  const i64 m = kCoordMax;
  const std::array<Point, 4> small{point(0, 0, 0), point(4, 0, 0), point(0, 4, 0), point(0, 0, 4)};
  const std::array<Point, 4> large{point(0, 0, 0), point(m, m, 0), point(m, 0, m), point(0, m, m)};
  const std::array<Point, 7> queries{point(0, 0, 0), point(1, 0, 0), point(4, 0, 0), point(0, 4, 0),
                                    point(m, m, m), point(m / 2, m / 2, m / 2), point(0, m, m)};
  for (const auto& points : {small, large}) {
    for (u8 q = 1; q <= 4; ++q) {
      const auto original = sphere(q, points);
      const auto copied = original;
      // Le tag est la precondition du chemin natif : refuser avant tout calcul si un mutant l'a falsifie.
      REQUIRE(copied.presentation_arity() == q);
      for (const auto query : queries) {
        const auto actual = power(copied, query);
        const auto sign = side(copied, query);
        REQUIRE(actual.ok() && sign.ok());
        const auto expected = num_test::wide_power(copied, query);
        CHECK(num_test::equals_wide(actual.value(), expected));
        CHECK_EQ(sign.value(), expected.sign());
      }
      for (u8 i = 0; i < q; ++i) CHECK_EQ(side(copied, points[i]).value(), 0);
    }
  }
  // H=4L^6 pour le triangle de trois coins et le quatrieme coin ; cette valeur depasse i128 des B21.
  const auto triangle = sphere(3, large);
  const i128 cube = i128{m} * m * m;
  const auto sixth = multiply(to_wide(cube), to_wide(cube));
  Wide<4> twice, expected;
  REQUIRE(add(sixth, sixth, twice));
  REQUIRE(add(twice, twice, expected));
  const auto actual = power(triangle, large[3]);
  REQUIRE(actual.ok());
  CHECK(num_test::equals_wide(actual.value(), expected));
  CHECK_EQ(expected.bit_length(), kCoordBits == 18 ? 110 : kCoordBits == 21 ? 128 : 146);
  CHECK_EQ(side(triangle, large[3]).value(), 1);
  // Annulation exacte : H(b)=0, mais D*||b-a||^2=12L^6 sort deja de i128 a B21/24.
  const auto first_term = multiply(to_wide(triangle.denominator()), to_wide(2 * i128{m} * m));
  CHECK_EQ(first_term.bit_length() > 127, kCoordBits != 18);
  const auto shell_power = power(triangle, large[1]);
  REQUIRE(shell_power.ok());
  CHECK_EQ(to_wide(shell_power.value()).sign(), 0);
  CHECK_EQ(side(triangle, large[1]).value(), 0);
}
