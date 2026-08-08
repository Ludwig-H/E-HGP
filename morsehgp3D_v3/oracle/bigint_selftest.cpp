// MorseHGP3D v3 — validation differentielle de l'arithmetique de l'ORACLE.
//
// Le juge doit lui-meme etre juge. Deux temoins independants :
//   - `__int128` du compilateur, sur le domaine ou il est exact ;
//   - GMP, sur des largeurs de plusieurs milliers de bits (compile seulement si
//     <gmpxx.h> est disponible ; MHGP3V_HAVE_GMP le declare).
//
// Ce test n'introduit AUCUNE dependance dans l'oracle lui-meme : GMP n'apparait
// que dans ce binaire de validation.

#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "rational.hpp"

#if defined(MHGP3V_HAVE_GMP)
#include <gmpxx.h>
#endif

namespace {

int failures = 0;
long long checks = 0;

void fail(const std::string& what) {
  ++failures;
  std::printf("  ECHEC %s\n", what.c_str());
}

std::string i128_to_decimal(__int128 v) {
  if (v == 0) return "0";
  const bool neg = v < 0;
  unsigned __int128 m = neg ? (~static_cast<unsigned __int128>(v) + 1)
                            : static_cast<unsigned __int128>(v);
  std::string s;
  while (m != 0) { s.insert(s.begin(), static_cast<char>('0' + static_cast<int>(m % 10))); m /= 10; }
  return neg ? ("-" + s) : s;
}

// ---------------------------------------------------------------------------
// Temoin 1 : __int128, sur le domaine ou il ne deborde pas.
// ---------------------------------------------------------------------------
void against_int128(unsigned long long seed, int rounds) {
  std::mt19937_64 rng(seed);
  for (int i = 0; i < rounds; ++i) {
    // 62 bits par operande : somme et produit tiennent dans __int128.
    const int bits = 1 + static_cast<int>(rng() % 62);
    auto draw = [&]() -> __int128 {
      __int128 v = static_cast<__int128>(rng() & ((1ULL << bits) - 1ULL));
      if (rng() & 1ULL) v = -v;
      return v;
    };
    const __int128 x = draw(), y = draw();
    const mhgp3v::BigInt a = mhgp3v::BigInt::from_i128(x);
    const mhgp3v::BigInt b = mhgp3v::BigInt::from_i128(y);

    if ((a + b).to_decimal() != i128_to_decimal(x + y)) fail("i128 somme");
    if ((a - b).to_decimal() != i128_to_decimal(x - y)) fail("i128 difference");
    if ((a * b).to_decimal() != i128_to_decimal(x * y)) fail("i128 produit");
    const int want = (x > y) - (x < y);
    if (compare(a, b) != want) fail("i128 comparaison");
    if ((-a).to_decimal() != i128_to_decimal(-x)) fail("i128 oppose");
    if (a.sign() != ((x > 0) - (x < 0))) fail("i128 signe");
    checks += 6;
  }
}

// ---------------------------------------------------------------------------
// Temoin 2 : GMP, sur de grandes largeurs.
// ---------------------------------------------------------------------------
#if defined(MHGP3V_HAVE_GMP)
mhgp3v::BigInt from_mpz(const mpz_class& v) {
  // Passage par la chaine decimale : aucune structure interne n'est partagee.
  const std::string s = v.get_str();
  mhgp3v::BigInt out(0);
  const mhgp3v::BigInt ten(10);
  std::size_t start = 0;
  bool neg = false;
  if (!s.empty() && s[0] == '-') { neg = true; start = 1; }
  for (std::size_t i = start; i < s.size(); ++i)
    out = out * ten + mhgp3v::BigInt(static_cast<long long>(s[i] - '0'));
  return neg ? -out : out;
}

void against_gmp(unsigned long long seed, int rounds) {
  std::mt19937_64 rng(seed);
  gmp_randclass gr(gmp_randinit_default);
  gr.seed(static_cast<unsigned long>(seed));
  for (int i = 0; i < rounds; ++i) {
    const unsigned long bits_a = 1UL + static_cast<unsigned long>(rng() % 4096ULL);
    const unsigned long bits_b = 1UL + static_cast<unsigned long>(rng() % 4096ULL);
    mpz_class x = gr.get_z_bits(bits_a);
    mpz_class y = gr.get_z_bits(bits_b);
    if (rng() & 1ULL) x = -x;
    if (rng() & 1ULL) y = -y;

    const mhgp3v::BigInt a = from_mpz(x);
    const mhgp3v::BigInt b = from_mpz(y);
    if (a.to_decimal() != x.get_str()) fail("gmp aller-retour decimal");
    if ((a + b).to_decimal() != mpz_class(x + y).get_str()) fail("gmp somme");
    if ((a - b).to_decimal() != mpz_class(x - y).get_str()) fail("gmp difference");
    if ((a * b).to_decimal() != mpz_class(x * y).get_str()) fail("gmp produit");
    if (compare(a, b) != ::cmp(x, y) - (::cmp(x, y) > 0 ? 0 : 0)) {
      const int want = (::cmp(x, y) > 0) - (::cmp(x, y) < 0);
      if (compare(a, b) != want) fail("gmp comparaison");
    }
    if (x != 0) {
      const std::size_t want_bits = mpz_sizeinbase(mpz_class(abs(x)).get_mpz_t(), 2);
      if (a.bit_length() != want_bits) fail("gmp longueur en bits");
      const std::size_t want_tz = mpz_scan1(mpz_class(abs(x)).get_mpz_t(), 0);
      if (a.trailing_zero_bits() != want_tz) fail("gmp facteurs deux");
      const std::size_t k = static_cast<std::size_t>(rng() % 200ULL);
      mpz_class shifted = abs(x) >> k;
      if (x < 0) shifted = -shifted;
      if (a.shifted_right(k).to_decimal() != shifted.get_str()) fail("gmp decalage");
    }
    checks += 8;
  }
}
#endif

// ---------------------------------------------------------------------------
// Rationnels : les identites que l'oracle emploie reellement.
// ---------------------------------------------------------------------------
void rational_identities(unsigned long long seed, int rounds) {
  std::mt19937_64 rng(seed);
  auto draw = [&]() -> mhgp3v::Rational {
    const long long n = static_cast<long long>(rng() % 2000000ULL) - 1000000LL;
    long long d = static_cast<long long>(rng() % 2000000ULL) - 1000000LL;
    if (d == 0) d = 1;
    return mhgp3v::Rational(mhgp3v::BigInt(n), mhgp3v::BigInt(d));
  };
  for (int i = 0; i < rounds; ++i) {
    const mhgp3v::Rational a = draw(), b = draw(), c = draw();
    if (!((a + b) - b == a)) fail("rationnel (a+b)-b");
    if (!((a * b) * c == a * (b * c))) fail("rationnel associativite");
    if (!(a * (b + c) == a * b + a * c)) fail("rationnel distributivite");
    if (!b.is_zero() && !((a / b) * b == a)) fail("rationnel (a/b)*b");
    if (a.denominator().sign() <= 0) fail("rationnel denominateur non positif");
    const int s = compare(a, b);
    if (s != -compare(b, a)) fail("rationnel antisymetrie");
    checks += 6;
  }
}

}  // namespace

int main(int argc, char** argv) {
  const int rounds = (argc > 1) ? std::atoi(argv[1]) : 20000;
  if (rounds <= 0) {
    std::printf("ECHEC : nombre de tours absurde (%d)\n", rounds);
    return 2;
  }
  against_int128(20260808ULL, rounds);
  rational_identities(4242ULL, rounds);
#if defined(MHGP3V_HAVE_GMP)
  against_gmp(90210ULL, rounds / 20 + 1);
  const char* gmp = "oui";
#else
  const char* gmp = "NON (temoin large absent)";
#endif
  std::printf("verifications : %lld ; temoin gmp : %s\n", checks, gmp);
#if !defined(MHGP3V_HAVE_GMP)
  // FAIL-CLOSED : une qualification de largeur arbitraire ne peut pas devenir
  // verte sans son temoin large. `--dev` autorise explicitement le mode de
  // developpement, et il est alors dit dans la sortie.
  bool development = false;
  for (int i = 1; i < argc; ++i)
    if (std::string(argv[i]) == "--dev") development = true;
  if (!development) {
    std::printf("ECHEC : temoin large absent ; la qualification exige GMP "
                "(ou --dev pour un passage de developpement, non qualifiant)\n");
    return 3;
  }
  std::printf("AVERTISSEMENT : passage de developpement, NON qualifiant\n");
#endif
  if (failures != 0) {
    std::printf("ECHEC : %d\n", failures);
    return 1;
  }
  std::printf("OK : arithmetique de l'oracle validee contre ses temoins\n");
  return 0;
}
