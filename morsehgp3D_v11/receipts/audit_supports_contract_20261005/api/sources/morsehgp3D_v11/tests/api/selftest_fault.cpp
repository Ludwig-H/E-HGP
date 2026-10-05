// Sonde de faute de l'auto-test F5 (hors produit ; portes mhgp11_api_selftest_fault_*). Elle change l'environnement
// flottant REEL du processus, puis cree une Session et rend exit_code de son issue, avec la ligne
// "selftest_probe <raison>" :
//   none      : environnement par defaut                      -> none, code 0 (temoin)
//   upward    : arrondi vers plus l'infini                     -> none, code 0
//   ftz_daz   : FTZ et DAZ poses (MXCSR)                       -> none, code 0 (admis par la doctrine)
//   inexact   : exception inexacte demasquee (feenableexcept)  -> environment_selftest, code 3
//   underflow : exception de sous-depassement demasquee        -> environment_selftest, code 3
//   denormal  : exception d'operande denormal demasquee (MXCSR, bit DM) -> environment_selftest, code 3
// Sans l'auto-test (mutant selftest_omis), une exception demasquee passerait inapercue : la Session serait creee et
// le code serait 0. Aucune operation flottante n'est faite par la sonde apres le changement d'environnement.
#include <cfenv>
#include <cstdio>
#include <cstring>

#if defined(__SSE__) && (defined(__x86_64__) || defined(__i386__))
#include <xmmintrin.h>
#endif

#include "api/api.hpp"

using namespace mhgp11;

namespace {

// Rend faux si le cas est inconnu ou indisponible sur cette plate-forme.
bool set_environment(const char* name) {
  if (std::strcmp(name, "none") == 0) return true;
  if (std::strcmp(name, "upward") == 0) return std::fesetround(FE_UPWARD) == 0;
#if defined(__SSE__) && (defined(__x86_64__) || defined(__i386__))
  if (std::strcmp(name, "ftz_daz") == 0) {
    _mm_setcsr(_mm_getcsr() | 0x8040u);
    return (_mm_getcsr() & 0x8040u) == 0x8040u;
  }
  if (std::strcmp(name, "denormal") == 0) {
    _mm_setcsr(_mm_getcsr() & ~0x0100u);
    return (_mm_getcsr() & 0x0100u) == 0;
  }
#endif
#if defined(__GLIBC__)
  if (std::strcmp(name, "inexact") == 0) return ::feenableexcept(FE_INEXACT) != -1;
  if (std::strcmp(name, "underflow") == 0) return ::feenableexcept(FE_UNDERFLOW) != -1;
#endif
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || !set_environment(argv[1])) {
    std::printf("selftest_probe usage : none|upward|ftz_daz|inexact|underflow|denormal\n");
    return 2;
  }
  const Result<api::Session> session = api::Session::make({MemoryBudget::kUnlimited, 2});
  const Outcome outcome = session.ok() ? Outcome{} : session.outcome();
  const std::string_view reason = reason_name(outcome.reason);
  std::printf("selftest_probe %.*s\n", static_cast<int>(reason.size()), reason.data());
  return exit_code(outcome);
}
