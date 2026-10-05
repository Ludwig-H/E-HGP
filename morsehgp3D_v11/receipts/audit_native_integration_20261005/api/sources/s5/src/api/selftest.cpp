// Auto-test F5 de l'environnement flottant (selftest.hpp ; docs/ARCHITECTURE.md, paragraphe 4). Ecrit a neuf pour la
// v11 : la v10 s'appuyait sur un contrat de compilation (IEEE-754 strict) qu'elle refusait option par option ; la v11
// n'a pas ce contrat, et ses usages du flottant restent justes sous tout mode d'arrondi, avec ou sans FTZ/DAZ. Cet
// auto-test controle a l'execution ce que F2 et F3 supposent de la machine, sans role dans les preuves :
//   T  aucune exception flottante n'est demasquee : sinon une operation inexacte, un debordement ou un operande
//      denormal (seuil F6 jusqu'a 2^-1074) arreteraient le processus par SIGFPE au lieu de rendre une valeur ;
//   M  le mode d'arrondi est l'un des quatre modes IEEE-754 ;
//   F2 un noyau entier de valeur absolue < 2^53 est exact (sommes, differences, produits, conversion d'un i64), et la
//      precision est celle du binaire64 : ((2^53 + 1) - 2^53) vaut 0 ou 2, jamais 1 (aucune precision etendue) ;
//   F3 une operation de resultat normal rend l'un des deux voisins binaire64 du resultat exact (addition,
//      soustraction, multiplication, division, conversions d'un u64 et d'un i64), signes negatifs compris.
// Les operandes sont lus par volatile : aucune operation n'est evaluee a la compilation (sous le mode par defaut).
// Aucun temoin n'est denormal : FTZ et DAZ, admis par la doctrine, ne changent aucun resultat attendu.
// Portes : mhgp11_api_selftest_modes (quatre modes, FTZ/DAZ), mhgp11_api_selftest_judge (mesures faussees),
// mhgp11_api_selftest_fault_* (exception demasquee par feenableexcept : refus, code 3).
#include "api/selftest.hpp"

#include <bit>
#include <cfenv>
#include <limits>

#if defined(__SSE__) && (defined(__x86_64__) || defined(__i386__))
#include <xmmintrin.h>
#endif

namespace mhgp11::api_detail {

namespace {

static_assert(std::numeric_limits<double>::is_iec559, "api : double doit etre le binaire64 IEEE-754");

enum class Op : u8 { add, sub, mul, div, from_u64, from_i64, excess };

// Temoin : operation, operandes (flottants ou entier), et les deux resultats admis lo <= hi (egaux si exact).
struct Witness {
  Op op;
  double a, b;
  u64 integer;
  double lo, hi;
};

constexpr u64 kMinusTwoPow62MinusOne = ~(u64{1} << 62);  // -(2^62 + 1) en complement a deux

constexpr std::array<Witness, kWitnessCount> kWitnesses = {{
    // F2 : noyaux entiers exacts, |valeur| < 2^53
    {Op::add, 0x1p52 - 1, 0x1p52, 0, 0x1p53 - 1, 0x1p53 - 1},
    {Op::sub, 0x1p53 - 1, 0x1p52 + 1, 0, 0x1p52 - 2, 0x1p52 - 2},
    {Op::mul, 0x1p26 + 1, 0x1p26 - 1, 0, 0x1p52 - 1, 0x1p52 - 1},
    {Op::mul, -(0x1p26 + 3), 0x1p26 + 5, 0, -(0x1p52 + 0x1p29 + 15), -(0x1p52 + 0x1p29 + 15)},
    {Op::add, -0x1p52, 0x1p52 - 7, 0, -7.0, -7.0},
    {Op::from_i64, 0, 0, (u64{1} << 53) - 1, 0x1p53 - 1, 0x1p53 - 1},
    // precision du binaire64 : 2^53 + 1 n'est pas representable
    {Op::excess, 0x1p53, 1.0, 0, 0.0, 2.0},
    // F3 : arrondi fidele
    {Op::add, 1.0, 0x1p-53, 0, 1.0, 0x1.0000000000001p+0},
    {Op::sub, -1.0, 0x1p-53, 0, -0x1.0000000000001p+0, -1.0},
    {Op::mul, 0x1.0000000000001p+0, 0x1.0000000000001p+0, 0, 0x1.0000000000002p+0, 0x1.0000000000003p+0},
    {Op::div, 1.0, 3.0, 0, 0x1.5555555555555p-2, 0x1.5555555555556p-2},
    {Op::div, -2.0, 3.0, 0, -0x1.5555555555556p-1, -0x1.5555555555555p-1},
    {Op::div, 0x1p1000, 3.0, 0, 0x1.5555555555555p+998, 0x1.5555555555556p+998},
    {Op::from_u64, 0, 0, ~u64{0}, 0x1.fffffffffffffp+63, 0x1p64},
    {Op::from_i64, 0, 0, kMinusTwoPow62MinusOne, -0x1.0000000000001p+62, -0x1p62},
}};

// Exceptions flottantes toutes masquees : MXCSR (IM, DM, ZM, OM, UM, PM, bits 7 a 12) sur x86, et le mot de controle
// x87 lu par fegetexcept (extension de la glibc).
bool traps_masked() noexcept {
  bool masked = true;
#if defined(__SSE__) && (defined(__x86_64__) || defined(__i386__))
  masked = masked && (_mm_getcsr() & 0x1F80u) == 0x1F80u;
#endif
#if defined(__GLIBC__)
  masked = masked && ::fegetexcept() == 0;
#endif
  return masked;
}

double apply(const Witness& w) noexcept {
  volatile double a = w.a, b = w.b;
  volatile u64 n = w.integer;
  volatile double out = 0;
  switch (w.op) {
    case Op::add: out = a + b; break;
    case Op::sub: out = a - b; break;
    case Op::mul: out = a * b; break;
    case Op::div: out = a / b; break;
    case Op::from_u64: out = static_cast<double>(static_cast<u64>(n)); break;
    case Op::from_i64: out = static_cast<double>(static_cast<i64>(static_cast<u64>(n))); break;
    case Op::excess: out = (a + b) - a; break;
  }
  return out;
}

// Motif binaire, zero signe confondu : (2^53 - 2^53) vaut -0 sous l'arrondi vers moins l'infini (IEEE-754, 6.3).
u64 pattern(double value) noexcept {
  const u64 bits = std::bit_cast<u64>(value);
  return (bits << 1) == 0 ? 0 : bits;
}

}  // namespace

FloatProbe measure_float_environment() noexcept {
  FloatProbe probe;
  probe.traps_masked = traps_masked();
  probe.rounding = std::fegetround();
  if (!probe.traps_masked) return probe;
  for (std::size_t i = 0; i < kWitnessCount; ++i) probe.values[i] = apply(kWitnesses[i]);
  return probe;
}

Outcome judge_float_environment(const FloatProbe& probe) noexcept {
  if (!probe.traps_masked) return fail(Reason::environment_selftest);
  const int r = probe.rounding;
  if (r != FE_TONEAREST && r != FE_DOWNWARD && r != FE_UPWARD && r != FE_TOWARDZERO)
    return fail(Reason::environment_selftest);
  for (std::size_t i = 0; i < kWitnessCount; ++i) {
    const u64 got = pattern(probe.values[i]);
    if (got != pattern(kWitnesses[i].lo) && got != pattern(kWitnesses[i].hi))
      return fail(Reason::environment_selftest);
  }
  return {};
}

Outcome environment_selftest() noexcept { return judge_float_environment(measure_float_environment()); }

}  // namespace mhgp11::api_detail
