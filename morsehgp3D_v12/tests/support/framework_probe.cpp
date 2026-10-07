// Sonde du cadre de test (hors produit) : des tests qui reussissent, echouent ou manquent leur plancher, pour juger
// tests/support/test.hpp lui-meme (tests/support/tests.cmake).
#include <stdexcept>
#include <string>
#include <string_view>

#include "test.hpp"

namespace {

__extension__ typedef __int128 Wide;
enum class Color : unsigned { red = 1, blue = 2 };

}  // namespace

// Conforme : controles de chaque forme, plancher atteint exactement.
MHGP12_TEST(pass, 8) {
  CHECK(1 + 1 == 2);
  CHECK_EQ(2u, 2);                       // entiers de signes differents, sans avertissement
  CHECK_EQ(-1, -1L);
  CHECK_EQ(std::string("a"), "a");
  CHECK_EQ(std::string_view("ab"), "ab");
  CHECK_EQ(Color::blue, Color::blue);
  CHECK_EQ(Wide{1} << 100, Wide{1} << 100);
  REQUIRE(true);
}

// Egalite sans piege de signe : -1 n'est pas egal a 2^32 - 1.
MHGP12_TEST(sign, 2) {
  CHECK(!mhgp12::test::equal(-1, 4294967295u));
  CHECK(mhgp12::test::equal(7, 7u));
}

// Un controle en echec : code 1.
MHGP12_TEST(fail, 1) { CHECK_EQ(1, 2); }

// REQUIRE quitte le test : le controle suivant, qui echouerait, n'est pas joue (1 echec et non 2).
MHGP12_TEST(require_stops, 1) {
  REQUIRE(false);
  CHECK(false);
}

// Plancher non atteint sans echec : code 3.
MHGP12_TEST(below_floor, 5) { CHECK(true); }

// Exception sortie d'un test : code 1, pas un arret par signal.
MHGP12_TEST(throws, 0) { throw std::runtime_error("exception de la sonde"); }

// Textes ecrits par CHECK_EQ en cas d'echec.
MHGP12_TEST(texts, 6) {
  using mhgp12::test::text_of;
  CHECK_EQ(text_of(true), "true");
  CHECK_EQ(text_of(-42), "-42");
  CHECK_EQ(text_of(static_cast<unsigned char>(200)), "200");
  CHECK_EQ(text_of(Color::red), "1");
  CHECK_EQ(text_of(-(Wide{1} << 100)), "-1267650600228229401496703205376");
  CHECK_EQ(text_of(std::string("x")), "\"x\"");
}

MHGP12_TEST_MAIN()
