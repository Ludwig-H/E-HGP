// Portes de l'auto-test F5 de la Session (tests/api/tests.cmake, mhgp11_api_selftest_*) :
//   modes : sous chacun des quatre modes d'arrondi, avec et sans FTZ/DAZ (tests/support/fenv.hpp), la mesure lit le
//           mode courant, l'auto-test est conforme et une Session se cree ; l'environnement est restaure ensuite ;
//   judge : le juge refuse environment_selftest toute mesure faussee : exception demasquee, mode inconnu, et pour
//           chaque temoin le voisin binaire64 hors de la paire admise (un ulp sous la borne basse, un ulp sur la borne
//           haute), precision etendue simulee ; il admet le zero negatif du temoin de precision et la mesure reelle.
// La faute reelle (exception demasquee par feenableexcept) est jouee par selftest_fault.cpp (code 3 attendu).
#include <cfenv>
#include <cmath>
#include <limits>

#include "api/api.hpp"
#include "api/selftest.hpp"
#include "fenv.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::api_detail;

MHGP11_TEST(modes, 81) {
  for (const int mode : test::kRoundModes) {
    for (const unsigned flush : test::kFlushModes) {
      test::FenvGuard guard;
      REQUIRE(guard.set(mode, flush));
      const FloatProbe probe = measure_float_environment();
      CHECK(probe.traps_masked);
      CHECK_EQ(probe.rounding, mode);
      CHECK(environment_selftest().ok());
      auto session = api::Session::make({MemoryBudget::kUnlimited, 2});
      CHECK(session.ok());
    }
  }
  CHECK_EQ(std::fegetround(), FE_TONEAREST);
}

MHGP11_TEST(judge, 66) {
  const FloatProbe real = measure_float_environment();
  REQUIRE(judge_float_environment(real).ok());
  FloatProbe trapped = real;
  trapped.traps_masked = false;
  CHECK_EQ(judge_float_environment(trapped).reason, Reason::environment_selftest);
  FloatProbe unknown = real;
  unknown.rounding = -1;
  CHECK_EQ(judge_float_environment(unknown).reason, Reason::environment_selftest);
  // Un ulp de part et d'autre de chaque resultat reel : hors de la paire admise, quel que soit le mode.
  constexpr double kInf = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < kWitnessCount; ++i) {
    for (const double toward : {-kInf, kInf}) {
      FloatProbe moved = real;
      double value = real.values[i];
      // Deux pas : depuis l'une des bornes, un seul pas peut retomber sur l'autre borne admise.
      value = std::nextafter(std::nextafter(value, toward), toward);
      moved.values[i] = value;
      const Outcome judged = judge_float_environment(moved);
      CHECK_EQ(judged.reason, Reason::environment_selftest);
      CHECK_EQ(exit_code(judged), 3);
    }
  }
  // Temoin de precision (indice 6) : ((2^53 + 1) - 2^53) vaut 1 sous une precision etendue ; -0 est admis.
  FloatProbe extended = real;
  extended.values[6] = 1.0;
  CHECK_EQ(judge_float_environment(extended).reason, Reason::environment_selftest);
  FloatProbe negative_zero = real;
  negative_zero.values[6] = -0.0;
  CHECK(judge_float_environment(negative_zero).ok());
  FloatProbe two = real;
  two.values[6] = 2.0;
  CHECK(judge_float_environment(two).ok());
}

MHGP11_TEST_MAIN()
