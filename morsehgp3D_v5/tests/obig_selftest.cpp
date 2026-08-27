// MorseHGP3D v5 — LE JUGE DU JUGE : selftest de l'arithmetique de l'oracle
// (oracle/obig.hpp, limbes 32 bits signe-magnitude) contre des autorites qui
// ne partagent aucune ligne de code avec lui :
//   A  __int128 materiel : add/sub/mul/cmp sur toutes les paires ordonnees
//      d'un pool deterministe (splitmix64, graine gravee — aucune horloge) de
//      bords (0, ±1, ±2^31, ±2^32, ±2^63, ±2^64, ±2^95, ±2^126, ±(2^127-1),
//      -2^127) et de tirages de largeur 30..126 bits ; PLANCHER >= 10000 cas,
//      dont >= 2000 produits croisant 64 bits (|ab| >= 2^64, tenant en i128) ;
//   R  reconstruction materielle des produits HORS i128 : |a| = a1·2^64 + a0,
//      |b| = b1·2^64 + b0, |ab| = a0b0 + (a0b1 + a1b0)·2^64 + a1b1·2^128 avec
//      quatre produits 64x64 -> u128 du materiel, assembles par mots ;
//      PLANCHER >= 2000 produits au-dela de 128 bits ;
//   B  algebre sur des triplets LARGES (jusqu'a 128 bits par facteur, tout
//      tient sous 384 bits) : distributivite a(b+c) = ab+ac, associativite
//      (ab)c = a(bc), commutativite, (a-b)+b = a, signe du produit ;
//      PLANCHER >= 1000 triplets ;
//   C  frontiere EXACTE du debordement : 2^191·2^192 = 2^383 tient sans
//      drapeau ; 2^192·2^192 = 2^384 leve le drapeau (STATUT, le processus
//      survit) ; (2^384-1)+1, (2^384-1)·2 (retenue finale), pow2(384),
//      from_u64_words trop large ; le drapeau est collant puis efface ;
//   D  fixtures GRAVEES (hexadecimal calcule hors C++) : (2^127-1)^2,
//      2^190·2^190, (2^64-1)^2, (-2^127)^2, (2^96-1)^2, (2^127-1)(2^126+1),
//      3·(-5), zero signe canonique, aller-retour to_i128 aux bords ;
//   E  boost::multiprecision::cpp_int si disponible (#if __has_include) :
//      troisieme autorite sur des valeurs jusqu'a 384 bits, debordement
//      decide par cpp_int (>= 2^384 => drapeau exige) ; sinon "boost absent"
//      et cette partie ne compte pas.
// Codes : 0 accord total ; 1 desaccord avec une autorite ; 2 refus (argument
// ou mutant inconnu) ; 3 plancher/invariant (drapeau manquant ou parasite,
// mutant non discrimine) ; 4 mutant tue (--inject=obig-carry-lost : la
// retenue du produit long jetee aux positions i+j >= 4 — le tuer prouve que
// les limbes >= 128 bits sont reellement traverses).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if __has_include(<boost/multiprecision/cpp_int.hpp>)
#include <boost/multiprecision/cpp_int.hpp>
#define MHGP5_HAS_BOOST_CPP_INT 1
#else
#define MHGP5_HAS_BOOST_CPP_INT 0
#endif

#include "../oracle/obig.hpp"

namespace {

using OB = mhgp5_oracle::OBig384;
using mhgp5_oracle::oi128;
using mhgp5_oracle::ou128;

struct Split64 {
  std::uint64_t s;
  std::uint64_t next() {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }
};

int g_disagree = 0;   // desaccord avec une autorite (code 1)
int g_invariant = 0;  // drapeau manquant/parasite, invariant (code 3)

void disagree(const char* what, const OB* a = nullptr, const OB* b = nullptr) {
  if (g_disagree < 20) {
    std::fprintf(stderr, "DESACCORD obig : %s", what);
    if (a) std::fprintf(stderr, " a=%s", a->hex().c_str());
    if (b) std::fprintf(stderr, " b=%s", b->hex().c_str());
    std::fprintf(stderr, "\n");
  }
  ++g_disagree;
}

void invariant(const char* what) {
  if (g_invariant < 20) std::fprintf(stderr, "INVARIANT obig : %s\n", what);
  ++g_invariant;
}

ou128 mag_of(oi128 v) { return v < 0 ? (ou128)(-(v + 1)) + 1 : (ou128)v; }

// Valeur aleatoire signee de `bits` bits significatifs exactement.
oi128 random_i128_bits(Split64* rng, int bits, bool negative) {
  ou128 m = ((ou128)rng->next() << 64) | rng->next();
  if (bits <= 0) return 0;
  if (bits < 128) m &= (((ou128)1) << bits) - 1;
  m |= ((ou128)1) << (bits - 1);
  if (bits >= 128) m >>= 1;  // 127 bits max en magnitude positive
  if (negative) return -(oi128)m;
  return (oi128)m;
}

// OBig de magnitude u128 placee a partir du mot 64 bits `word_shift`.
OB shifted_u128(ou128 m, int word_shift, bool negative) {
  std::uint64_t words[6] = {};
  words[word_shift] = (std::uint64_t)m;
  words[word_shift + 1] = (std::uint64_t)(m >> 64);
  return OB::from_u64_words(words, 6, negative);
}

// Reconstruction materielle de |a|·|b| par quatre produits 64x64 (autorite R).
OB reconstruct_product(oi128 a, oi128 b) {
  const ou128 ma = mag_of(a), mb = mag_of(b);
  const std::uint64_t a0 = (std::uint64_t)ma, a1 = (std::uint64_t)(ma >> 64);
  const std::uint64_t b0 = (std::uint64_t)mb, b1 = (std::uint64_t)(mb >> 64);
  const ou128 p00 = (ou128)a0 * b0, p01 = (ou128)a0 * b1, p10 = (ou128)a1 * b0, p11 = (ou128)a1 * b1;
  OB r = shifted_u128(p00, 0, false) + shifted_u128(p01, 1, false) + shifted_u128(p10, 1, false) +
         shifted_u128(p11, 2, false);
  const bool negative = (a < 0) != (b < 0) && !r.is_zero();
  return negative ? -r : r;
}

bool equals_i128(const OB& got, oi128 expected) {
  oi128 back = 0;
  if (!got.to_i128(&back)) return false;
  return back == expected;
}

// Tirage d'un OBig de `bits` bits significatifs (<= 128 ici), signe aleatoire.
OB random_wide(Split64* rng, int bits) {
  const oi128 v = random_i128_bits(rng, bits, false);
  OB r = OB::from_i128(v);
  if (rng->next() & 1) r = -r;
  return r;
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--inject=", 9) == 0) inject = argv[i] + 9;
    else {
      std::fprintf(stderr, "REFUS : argument inconnu %s\n", argv[i]);
      return 2;
    }
  }
  if (!inject.empty() && !mhgp5::mutants_enable(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }
  const bool mutant = !inject.empty();
  mhgp5_oracle::overflow_log() = false;  // debordements attendus (partie C, E)

  // ---- Pool deterministe de valeurs i128 -----------------------------------
  std::vector<oi128> pool;
  {
    const oi128 imax = (oi128)((((ou128)1) << 127) - 1);
    const oi128 borders[] = {
        0,
        1,
        2,
        (oi128)1 << 31,
        ((oi128)1 << 32) - 1,
        (oi128)1 << 32,
        ((oi128)1 << 63) - 1,
        (oi128)1 << 63,
        ((oi128)1 << 64) - 1,
        (oi128)1 << 64,
        (oi128)1 << 95,
        (oi128)1 << 96,
        (oi128)1 << 126,
        imax,
    };
    for (const oi128 v : borders) {
      pool.push_back(v);
      if (v != 0) pool.push_back(-v);
    }
    pool.push_back(-imax - 1);  // INT128_MIN
    Split64 rng{0x5a17c0ffee0b1d01ull};
    for (int t = 0; t < 24; ++t) pool.push_back(random_i128_bits(&rng, 30 + (int)(rng.next() % 11), (t & 1) != 0));
    for (int t = 0; t < 40; ++t) pool.push_back(random_i128_bits(&rng, 1 + (int)(rng.next() % 62), (t & 1) != 0));
    for (int t = 0; t < 40; ++t) pool.push_back(random_i128_bits(&rng, 63 + (int)(rng.next() % 64), (t & 1) != 0));
  }

  // ---- A + R : toutes les paires ordonnees contre __int128 / reconstruction --
  std::uint64_t cases = 0, mul_cross64 = 0, mul_beyond128 = 0;
  for (const oi128 a : pool)
    for (const oi128 b : pool) {
      const OB oa = OB::from_i128(a), ob = OB::from_i128(b);
      // aller-retour de la conversion
      if (!equals_i128(oa, a)) disagree("from_i128/to_i128", &oa);
      oi128 r = 0;
      mhgp5_oracle::clear_overflow();
      if (!__builtin_add_overflow(a, b, &r)) {
        if (!equals_i128(oa + ob, r)) disagree("add", &oa, &ob);
        ++cases;
      }
      if (!__builtin_sub_overflow(a, b, &r)) {
        if (!equals_i128(oa - ob, r)) disagree("sub", &oa, &ob);
        ++cases;
      }
      const OB prod = oa * ob;
      if (!__builtin_mul_overflow(a, b, &r)) {
        if (!equals_i128(prod, r)) disagree("mul", &oa, &ob);
        ++cases;
        if (mag_of(r) >= (((ou128)1) << 64)) ++mul_cross64;
      } else {
        ++mul_beyond128;
      }
      // reconstruction materielle (toujours, le produit tient sous 256 bits)
      if (cmp(prod, reconstruct_product(a, b)) != 0) disagree("mul reconstruction 64x64", &oa, &ob);
      ++cases;
      // comparaison signee
      const int c = cmp(oa, ob);
      const int e = a < b ? -1 : (a > b ? 1 : 0);
      if (c != e) disagree("cmp", &oa, &ob);
      ++cases;
      if (mhgp5_oracle::overflow_seen()) invariant("drapeau parasite sur une paire i128");
    }

  // ---- B : algebre sur triplets larges ---------------------------------------
  std::uint64_t triplets = 0;
  {
    Split64 rng{0xdeadbeefcafe5a5aull};
    for (int t = 0; t < 1500; ++t) {
      const OB a = random_wide(&rng, 1 + (int)(rng.next() % 128));
      const OB b = random_wide(&rng, 1 + (int)(rng.next() % 128));
      const OB c = random_wide(&rng, 1 + (int)(rng.next() % 127));
      mhgp5_oracle::clear_overflow();
      const OB lhs = a * (b + c);
      const OB rhs = a * b + a * c;
      if (cmp(lhs, rhs) != 0) disagree("distributivite", &a, &b);
      const OB l2 = (a * b) * c, r2 = a * (b * c);
      if (cmp(l2, r2) != 0) disagree("associativite", &a, &b);
      if (cmp(a * b, b * a) != 0) disagree("commutativite", &a, &b);
      if (cmp((a - b) + b, a) != 0) disagree("(a-b)+b", &a, &b);
      if (cmp(-(-a), a) != 0) disagree("double negation", &a);
      const int s = (a * b).sign();
      const int expected_sign = a.sign() * b.sign();
      if (s != expected_sign) disagree("signe du produit", &a, &b);
      if (mhgp5_oracle::overflow_seen()) invariant("drapeau parasite sur un triplet < 2^384");
      ++triplets;
    }
  }

  // ---- C : frontiere exacte du debordement -----------------------------------
  {
    mhgp5_oracle::clear_overflow();
    const OB fits = OB::pow2(191) * OB::pow2(192);
    if (mhgp5_oracle::overflow_seen()) invariant("2^383 : drapeau parasite");
    if (cmp(fits, OB::pow2(383)) != 0) disagree("2^191 * 2^192 != 2^383");
    if (fits.bit_length() != 384) disagree("bit_length(2^383) != 384");

    mhgp5_oracle::clear_overflow();
    (void)(OB::pow2(192) * OB::pow2(192));
    if (!mhgp5_oracle::overflow_seen()) invariant("2^384 par produit : drapeau manquant");

    // Collant : une operation saine ensuite ne l'efface pas.
    (void)(OB::from_i64(3) + OB::from_i64(4));
    if (!mhgp5_oracle::overflow_seen()) invariant("drapeau non collant");
    mhgp5_oracle::clear_overflow();
    if (mhgp5_oracle::overflow_seen()) invariant("clear_overflow inefficace");

    std::uint64_t all1[6];
    for (auto& l : all1) l = ~0ull;
    const OB max = OB::from_u64_words(all1, 6);
    if (mhgp5_oracle::overflow_seen()) invariant("2^384-1 : drapeau parasite a la construction");
    (void)(max + OB::from_i64(1));
    if (!mhgp5_oracle::overflow_seen()) invariant("(2^384-1)+1 : drapeau manquant");
    mhgp5_oracle::clear_overflow();
    (void)(max * OB::from_i64(2));
    if (!mhgp5_oracle::overflow_seen()) invariant("(2^384-1)*2 : drapeau manquant (retenue finale)");
    mhgp5_oracle::clear_overflow();
    (void)(max - OB::from_i64(1));
    if (mhgp5_oracle::overflow_seen()) invariant("(2^384-1)-1 : drapeau parasite");
    (void)(max + (-max));
    if (mhgp5_oracle::overflow_seen()) invariant("x + (-x) : drapeau parasite");
    (void)OB::pow2(384);
    if (!mhgp5_oracle::overflow_seen()) invariant("pow2(384) : drapeau manquant");
    mhgp5_oracle::clear_overflow();
    std::uint64_t seven[7] = {0, 0, 0, 0, 0, 0, 1};
    (void)OB::from_u64_words(seven, 7);
    if (!mhgp5_oracle::overflow_seen()) invariant("from_u64_words 7 mots : drapeau manquant");
    mhgp5_oracle::clear_overflow();
    (void)(OB::pow2(300) * OB::pow2(300));
    if (!mhgp5_oracle::overflow_seen()) invariant("2^600 : drapeau manquant");
    mhgp5_oracle::clear_overflow();
  }

  // ---- D : fixtures gravees ---------------------------------------------------
  {
    struct Fx {
      const char* name;
      OB got;
      const char* hex;
    };
    const oi128 imax = (oi128)((((ou128)1) << 127) - 1);
    const oi128 imin = -imax - 1;
    const Fx fx[] = {
        {"(2^127-1)^2", OB::from_i128(imax) * OB::from_i128(imax),
         "0x3fffffffffffffffffffffffffffffff00000000000000000000000000000001"},
        {"2^190*2^190", OB::pow2(190) * OB::pow2(190),
         "0x100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},
        {"(2^64-1)^2", OB::from_i128(((oi128)1 << 64) - 1) * OB::from_i128(((oi128)1 << 64) - 1),
         "0xfffffffffffffffe0000000000000001"},
        {"(-2^127)^2", OB::from_i128(imin) * OB::from_i128(imin),
         "0x4000000000000000000000000000000000000000000000000000000000000000"},
        {"(2^96-1)^2", OB::from_i128(((oi128)1 << 96) - 1) * OB::from_i128(((oi128)1 << 96) - 1),
         "0xfffffffffffffffffffffffe000000000000000000000001"},
        {"(2^127-1)(2^126+1)", OB::from_i128(imax) * OB::from_i128(((oi128)1 << 126) + 1),
         "0x200000000000000000000000000000003fffffffffffffffffffffffffffffff"},
        {"3*(-5)", OB::from_i64(3) * OB::from_i64(-5), "-0xf"},
        {"2^127-1 - (2^127-1)", OB::from_i128(imax) - OB::from_i128(imax), "0x0"},
        {"INT128_MIN + 1", OB::from_i128(imin) + OB::from_i64(1), "-0x7fffffffffffffffffffffffffffffff"},
    };
    for (const Fx& f : fx) {
      if (f.got.hex() != f.hex) {
        std::fprintf(stderr, "FIXTURE %s : lu %s attendu %s\n", f.name, f.got.hex().c_str(), f.hex);
        ++g_disagree;
      }
    }
    if (mhgp5_oracle::overflow_seen()) invariant("drapeau parasite sur les fixtures");
    // Zero signe canonique et aller-retour des bords.
    const OB z = OB::from_i64(-7) + OB::from_i64(7);
    if (!z.is_zero() || z.neg || z.sign() != 0) invariant("zero signe canonique");
    if (OB::from_i128(0).neg || (-OB::from_i128(0)).neg) invariant("zero canonique par negation");
    oi128 back = 0;
    if (!OB::from_i128(imin).to_i128(&back) || back != imin) invariant("aller-retour INT128_MIN");
    if (!OB::from_i128(imax).to_i128(&back) || back != imax) invariant("aller-retour INT128_MAX");
    if (OB::pow2(127).to_i128(&back)) invariant("2^127 ne tient pas en i128");
    if (!(-OB::pow2(127)).to_i128(&back) || back != imin) invariant("-2^127 tient en i128");
    if (OB::pow2(128).to_i128(&back)) invariant("2^128 ne tient pas en i128");
    if (OB::pow2(127).bit_length() != 128 || OB::from_i64(1).bit_length() != 1 || OB().bit_length() != 0)
      invariant("bit_length");
  }

  // ---- E : boost::cpp_int, troisieme autorite si presente --------------------
  std::uint64_t boost_cases = 0;
#if MHGP5_HAS_BOOST_CPP_INT
  {
    using bmp = boost::multiprecision::cpp_int;
    const bmp cap = bmp(1) << 384;
    const auto to_bmp = [](const OB& v) {
      bmp r = 0;
      for (int i = OB::kLimbs - 1; i >= 0; --i) {
        r <<= 32;
        r += v.w[i];
      }
      return v.neg ? bmp(-r) : r;
    };
    Split64 rng{0x0b005700c0ffee00ull};
    std::vector<OB> wide;
    for (int t = 0; t < 120; ++t) {
      OB v;
      const int top = (int)(rng.next() % 12);
      for (int i = 0; i <= top; ++i) v.w[i] = (std::uint32_t)rng.next();
      if (!v.w[top]) v.w[top] = 1;
      v.neg = (rng.next() & 1) != 0;
      v.canon();
      wide.push_back(v);
    }
    for (const OB& a : wide)
      for (const OB& b : wide) {
        const bmp A = to_bmp(a), B = to_bmp(b);
        const bmp ops[3] = {A + B, A - B, A * B};
        for (int k = 0; k < 3; ++k) {
          mhgp5_oracle::clear_overflow();
          const OB got = k == 0 ? a + b : (k == 1 ? a - b : a * b);
          const bool of = mhgp5_oracle::overflow_seen();
          if (abs(ops[k]) >= cap) {
            if (!of) invariant("cpp_int : debordement attendu sans drapeau");
          } else if (of) {
            invariant("cpp_int : drapeau parasite");
          } else if (to_bmp(got) != ops[k]) {
            disagree("cpp_int", &a, &b);
          }
          ++boost_cases;
        }
        const int c = cmp(a, b);
        const int e = A < B ? -1 : (A > B ? 1 : 0);
        if (c != e) disagree("cpp_int cmp", &a, &b);
        ++boost_cases;
      }
    mhgp5_oracle::clear_overflow();
    if (boost_cases < 10000) {
      std::fprintf(stderr, "PLANCHER : %llu cas cpp_int (< 10000)\n", (unsigned long long)boost_cases);
      return 3;
    }
  }
#else
  std::printf("boost absent : troisieme autorite cpp_int non exercee\n");
#endif

  std::printf(
      "obig_selftest : pool=%zu cas_i128=%llu mul_croisant_64=%llu mul_au_dela_128=%llu triplets=%llu "
      "cpp_int=%llu desaccords=%d invariants=%d\n",
      pool.size(), (unsigned long long)cases, (unsigned long long)mul_cross64, (unsigned long long)mul_beyond128,
      (unsigned long long)triplets, (unsigned long long)boost_cases, g_disagree, g_invariant);

  if (mutant) {
    if (g_disagree > 0 || g_invariant > 0) {
      std::printf("MUTANT TUE : %s\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", inject.c_str());
    return 3;
  }
  if (cases < 10000 || mul_cross64 < 2000 || mul_beyond128 < 2000 || triplets < 1000) {
    std::fprintf(stderr, "PLANCHER : cas=%llu (>= 10000), mul_croisant_64=%llu (>= 2000), mul_au_dela_128=%llu "
                         "(>= 2000), triplets=%llu (>= 1000)\n",
                 (unsigned long long)cases, (unsigned long long)mul_cross64, (unsigned long long)mul_beyond128,
                 (unsigned long long)triplets);
    return 3;
  }
  if (g_disagree > 0) return 1;
  if (g_invariant > 0) return 3;
  return 0;
}
