// MorseHGP3D v4 — LE JUGE DU JUGE : selftest de l'arithmetique OBig contre
// boost::multiprecision::cpp_int (troisieme autorite, tests uniquement —
// jamais dans un chemin produit ni dans l'oracle lui-meme).
//
// Exige par l'audit oracle du 17 aout (P0.2). Couverture demandee :
//   - zeros signes, INT128_MIN / INT128_MAX ;
//   - retenues traversant 1 a 6 limbes, borrows multi-limbes ;
//   - produits a termes hauts non nuls ;
//   - debordement EXACTEMENT au septieme limbe, rendu comme STATUT
//     (drapeau collant fail-closed), jamais comme signal ;
//   - distributivite et comparaison signee sur des milliers de valeurs
//     deterministes (splitmix64, graine gravee — aucune horloge).
// L'autorite cpp_int decide aussi QUAND le debordement est attendu : toute
// somme/tout produit >= 2^384 en magnitude doit lever le drapeau, tout
// resultat < 2^384 doit etre exact et sans drapeau.
//
// Codes : 0 accord total ; 1 desaccord avec cpp_int ; 3 plancher/invariant
// (drapeau manquant ou parasite, mutant non discrimine) ; 4 mutant tue
// (--inject=mul-carry-lost, la retenue du produit long jetee aux positions
// i+j >= 2 — le tuer prouve que les limbes hauts sont reellement traverses).
#include <cstdio>
#include <cstring>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "../oracle/obigint.hpp"

namespace {

using OB = mhgp4_oracle::OBig<6>;
using bmp = boost::multiprecision::cpp_int;

bmp to_bmp(const OB& v) {
  bmp r = 0;
  for (int i = 5; i >= 0; --i) {
    r <<= 64;
    r += v.w[i];
  }
  return v.neg ? bmp(-r) : r;
}

OB from_limbs(const std::uint64_t limbs[6], bool neg) {
  OB r;
  for (int i = 0; i < 6; ++i) r.w[i] = limbs[i];
  r.neg = neg;
  r.canon();
  return r;
}

// Generateur deterministe grave (splitmix64), graine fixe.
struct Split64 {
  std::uint64_t s;
  std::uint64_t next() {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }
};

int g_fail = 0;
void fail(const char* what) {
  std::fprintf(stderr, "DESACCORD selftest OBig : %s\n", what);
  ++g_fail;
}

const bmp kCap = bmp(1) << 384;  // premiere magnitude hors largeur

// Verifie une operation binaire contre l'autorite : si |attendu| < 2^384,
// exiger l'egalite exacte ET l'absence de drapeau ; sinon exiger le drapeau
// (echec ferme), la valeur empoisonnee n'etant pas comparee.
template <typename Op>
void check_op(const OB& a, const OB& b, const bmp& expected, Op op,
              const char* what) {
  mhgp4_oracle::clear_overflow();
  const OB got = op(a, b);
  const bool of = mhgp4_oracle::overflow_seen();
  if (abs(expected) >= kCap) {
    if (!of) fail(what);  // debordement attendu, statut manquant
    return;
  }
  if (of) {
    fail(what);  // drapeau parasite sur un resultat qui tient
    return;
  }
  if (to_bmp(got) != expected) fail(what);
}

void check_pair(const OB& a, const OB& b) {
  const bmp A = to_bmp(a), B = to_bmp(b);
  check_op(a, b, A + B, [](const OB& x, const OB& y) { return x + y; }, "add");
  check_op(a, b, A - B, [](const OB& x, const OB& y) { return x - y; }, "sub");
  check_op(a, b, A * B, [](const OB& x, const OB& y) { return x * y; }, "mul");
  // Comparaison signee (jamais de debordement possible).
  mhgp4_oracle::clear_overflow();
  const int c = cmp(a, b);
  const int e = A < B ? -1 : (A > B ? 1 : 0);
  if (c != e || mhgp4_oracle::overflow_seen()) fail("cmp");
}

}  // namespace

int main(int argc, char** argv) {
  bool mutant = false;
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--inject=mul-carry-lost") == 0) mutant = true;
  mhgp4_oracle::inject_mul_carry_lost() = mutant;
  mhgp4_oracle::overflow_log() = false;  // debordements attendus par milliers

  // ---- Pool deterministe : occupation de 1 a 6 limbes, motifs de retenue.
  std::vector<OB> pool;
  Split64 rng{0x0b1d5a17c0ffee01ull};
  for (int top = 0; top < 6; ++top) {
    for (int rep = 0; rep < 12; ++rep) {
      std::uint64_t limbs[6] = {};
      for (int i = 0; i <= top; ++i) limbs[i] = rng.next();
      if (!limbs[top]) limbs[top] = 1;  // occupation du limbe vise garantie
      pool.push_back(from_limbs(limbs, (rng.next() & 1) != 0));
    }
    // Motifs tout-a-un : la retenue traverse top+1 limbes sur « +1 ».
    std::uint64_t ones[6] = {};
    for (int i = 0; i <= top; ++i) ones[i] = ~0ull;
    pool.push_back(from_limbs(ones, false));
    pool.push_back(from_limbs(ones, true));
    // Puissance pure 2^(64·top) : borrow multi-limbes sur « -1 ».
    std::uint64_t pw[6] = {};
    pw[top] = 1;
    pool.push_back(from_limbs(pw, false));
  }
  pool.push_back(OB::from_i128(0));
  pool.push_back(OB::from_i128(1));
  pool.push_back(OB::from_i128(-1));
  {
    const oi128 imax = (oi128)(((ou128)1 << 127) - 1);
    pool.push_back(OB::from_i128(imax));        // INT128_MAX
    pool.push_back(OB::from_i128(-imax - 1));   // INT128_MIN
  }

  // ---- Fixtures gravees prealables (hors boucle, messages dedies).
  // Zero signe canonique : a - a est zero a neg=false.
  {
    mhgp4_oracle::clear_overflow();
    const OB z = pool[3] - pool[3];
    if (!z.is_zero() || z.neg) fail("zero signe canonique");
    if (OB::from_i128(0).neg) fail("from_i128(0) canonique");
  }
  // Septieme limbe exactement : 2^320 · 2^63 = 2^383 tient sans drapeau ;
  // 2^320 · 2^64 = 2^384 leve le STATUT (et le processus survit — un signal
  // ferait echouer la porte via run_expect).
  {
    std::uint64_t p320[6] = {};
    p320[5] = 1;
    const OB a = from_limbs(p320, false);
    std::uint64_t h63[6] = {};
    h63[0] = 1ull << 63;
    std::uint64_t h64[6] = {};
    h64[1] = 1;
    mhgp4_oracle::clear_overflow();
    const OB fits = a * from_limbs(h63, false);
    if (mhgp4_oracle::overflow_seen() || to_bmp(fits) != (bmp(1) << 383))
      fail("2^383 doit tenir");
    mhgp4_oracle::clear_overflow();
    (void)(a * from_limbs(h64, false));
    if (!mhgp4_oracle::overflow_seen()) fail("2^384 doit lever le statut");
    // Addition au bord : (2^384 - 1) + 1 leve le statut.
    std::uint64_t all1[6];
    for (auto& l : all1) l = ~0ull;
    mhgp4_oracle::clear_overflow();
    (void)(from_limbs(all1, false) + OB::from_i128(1));
    if (!mhgp4_oracle::overflow_seen()) fail("(2^384-1)+1 doit lever le statut");
  }

  // ---- Toutes les paires du pool : add/sub/mul/cmp contre l'autorite.
  std::uint64_t pairs = 0;
  for (const OB& a : pool)
    for (const OB& b : pool) {
      check_pair(a, b);
      ++pairs;
    }

  // ---- Distributivite a·(b+c) == a·b + a·c sur triplets echantillonnes
  // (uniquement quand l'autorite garantit que tout tient en largeur).
  std::uint64_t distrib = 0;
  Split64 rng2{0xdeadbeefcafe1234ull};
  for (int t = 0; t < 4000; ++t) {
    const OB& a = pool[rng2.next() % pool.size()];
    const OB& b = pool[rng2.next() % pool.size()];
    const OB& c = pool[rng2.next() % pool.size()];
    const bmp A = to_bmp(a), B = to_bmp(b), C = to_bmp(c);
    if (abs(B + C) >= kCap || abs(A * (B + C)) >= kCap || abs(A * B) >= kCap ||
        abs(A * C) >= kCap)
      continue;
    mhgp4_oracle::clear_overflow();
    const OB lhs = a * (b + c);
    const OB rhs = a * b + a * c;
    if (mhgp4_oracle::overflow_seen() || cmp(lhs, rhs) != 0 ||
        to_bmp(lhs) != A * (B + C))
      fail("distributivite");
    ++distrib;
  }
  if (distrib < 1000) {
    std::fprintf(stderr, "PLANCHER : distributivite testee %llu fois (< 1000)\n",
                 (unsigned long long)distrib);
    return 3;
  }

  std::printf(
      "obig_selftest : %zu valeurs, %llu paires, %llu triplets distributifs, "
      "desaccords=%d\n",
      pool.size(), (unsigned long long)pairs, (unsigned long long)distrib,
      g_fail);
  if (mutant) {
    if (g_fail > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return g_fail > 0 ? 1 : 0;
}
