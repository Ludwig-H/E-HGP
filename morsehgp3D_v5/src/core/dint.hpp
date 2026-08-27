// MorseHGP3D v5 — DI128 : entier SIGNE 128 bits PORTABLE (deux u64,
// complement a deux, `hi` porte le signe), compilable en host ET en device.
//
// Pourquoi : le code device CUDA n'a pas de __int128 fiable (les formes q3/q4
// de src/lanes/ et wide.hpp reposent sur i128/u128). Ce fichier n'utilise que
// des u64 et un seul « mot haut du produit 64×64 » (mulhi) :
//   host   : `(u64)(((u128)x * y) >> 64)` ;
//   device : `__umul64hi(x, y)` (intrinseque CUDA, sm_xx, sans en-tete) ;
//   simulation device sur host (-D__CUDA_ARCH__ -DMHGP5_FAKE_DEVICE) : repli
//            en limbes 32 bits `di_mulhi_u64_portable` (aucun u128).
// Tout est MHGP5_HD inline, constexpr des que le chemin ne passe pas par une
// intrinseque (MHGP5_DI_CONSTEXPR). Aucune allocation, aucune boucle non
// bornee (les boucles sont sur 3 mots au plus).
//
// SEMANTIQUE : add/sub/neg/shl1 sont l'anneau Z/2^128 en complement a deux
// (comme le materiel) ; cmp/sign lisent le signe de `hi`. Sur le profil u16
// aucune quantite des formes ne depasse 2^106 en module (q3_power), donc
// aucun repliement ne se produit : la porte `mhgp5_dint_gate` prouve
// l'egalite avec __int128 sur toute la plage (semantique modulaire declaree)
// et sur toutes les formes des lanes.
//
// PRECONDITIONS (documentees, pas verifiees a l'execution) :
//   di_mul_i64_i64(a, b)    : toujours exact (|ab| < 2^126 + 2^63 < 2^127).
//   di_mul_di128_i64(a, v)  : EXACT ssi le vrai produit est dans
//                             [-2^127, 2^127) — c'est le produit modulo 2^128
//                             de deux complements a deux, exact quand le
//                             resultat tient. Contrat des largeurs u16 :
//                             G < 2^68 × |v|² < 2^36 ; c1, c2 < 2^69 × d_i < 2^16 ;
//                             |W_i| < 2^87 × |v_i| < 2^16 ; det < 2^57 × |v|² < 2^36 ;
//                             |N'_i| < 2^72 × |v_i| < 2^16 ; det × |dp_i| < 2^16 ;
//                             |rc_i| < 2^75 × mineur < 2^33 — tous < 2^111.
//   di_div_by_4_exact(a)    : a divisible par 4 (lo & 3 == 0) ; sinon c'est
//                             un plancher (decalage arithmetique), pas un refus.
//   du_mul_128x128_192(x,y) : produit < 2^192 (sinon tronque mod 2^192).
//   di_sum_of_three_squares_192 : |a|, |b|, |c| < 2^95 (somme < 3·2^190).
//
// Mutant `dint-mulhi-dropped` (registre kMutants, a declarer dans la famille
// « entiers larges ») : le mot haut du produit 64×64 est ignore sur le chemin
// host. La porte le tue (code 4) sur les tirages et sur les formes : preuve
// que les produits traversent reellement les 64 bits hauts.
#pragma once

#include <cstdint>
#include <type_traits>

#include "device.hpp"
#include "mutants.hpp"
#include "types.hpp"
#include "wide.hpp"  // U192 (le TYPE seulement ; ses produits u128 ne sont pas appeles ici)

#if defined(__CUDA_ARCH__) && !defined(MHGP5_FAKE_DEVICE)
#define MHGP5_DI_CONSTEXPR
#else
#define MHGP5_DI_CONSTEXPR constexpr
#endif

namespace mhgp5 {

// Complement a deux sur 128 bits : valeur = hi·2^64 + lo, hi lu en signe.
struct DI128 {
  u64 lo;
  u64 hi;
};

// Magnitude non signee sur 128 bits (valeur absolue d'un DI128 : |INT128_MIN|
// = 2^127 tient).
struct DU128 {
  u64 lo;
  u64 hi;
};

// ---- mulhi 64×64 ----------------------------------------------------------------

// Mot haut de x·y en limbes 32 bits : aucun u128, aucune intrinseque. Repli de
// la simulation device et TEMOIN sur host (la porte l'egale au mulhi u128).
MHGP5_HD inline constexpr u64 di_mulhi_u64_portable(u64 x, u64 y) {
  const u64 m32 = 0xffffffffull;
  const u64 x0 = x & m32, x1 = x >> 32;
  const u64 y0 = y & m32, y1 = y >> 32;
  const u64 p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, p11 = x1 * y1;
  // mid < 3·2^32 : pas de debordement ; le total < 2^128 borne le mot haut.
  const u64 mid = (p00 >> 32) + (p01 & m32) + (p10 & m32);
  return p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

// Mot haut de x·y : selection par cible.
MHGP5_HD inline MHGP5_DI_CONSTEXPR u64 di_mulhi_u64(u64 x, u64 y) {
#if defined(__CUDA_ARCH__) && !defined(MHGP5_FAKE_DEVICE)
  return __umul64hi(x, y);  // intrinseque device CUDA (unsigned long long × 2)
#elif defined(__CUDA_ARCH__)
  return di_mulhi_u64_portable(x, y);  // simulation device sur host
#else
  if (!std::is_constant_evaluated()) {
    if (MHGP5_MUTANT("dint-mulhi-dropped")) return 0;  // MUTANT : mot haut jete
  }
  return (u64)(((u128)x * y) >> 64);
#endif
}

// ---- Constructions et conversions -----------------------------------------------

MHGP5_HD inline constexpr DI128 di_zero() { return DI128{0, 0}; }

MHGP5_HD inline constexpr DI128 di_from_i64(i64 v) { return DI128{(u64)v, v < 0 ? ~0ull : 0ull}; }

// Le DI128 tient-il en i64 ? (hi est l'extension de signe de lo)
MHGP5_HD inline constexpr bool di_fits_i64(DI128 a) { return a.hi == ((i64)a.lo < 0 ? ~0ull : 0ull); }

// Precondition : di_fits_i64(a).
MHGP5_HD inline constexpr i64 di_to_i64_unchecked(DI128 a) { return (i64)a.lo; }

// Ponts vers __int128 : fonctions HOTE (jamais MHGP5_HD), visibles dans les
// DEUX passes de nvcc — un `#if !defined(__CUDA_ARCH__)` les cacherait a la
// passe device, qui parse aussi le code hote et refuse l'identifiant
// (session G4 50fee05c).
// Host seulement : pont vers __int128 (pour les portes et le chemin CPU).
inline constexpr DI128 di_from_i128(i128 v) {
  const u128 u = (u128)v;
  return DI128{(u64)u, (u64)(u >> 64)};
}
inline constexpr i128 di_to_i128(DI128 a) { return (i128)(((u128)a.hi << 64) | a.lo); }
inline constexpr u128 du_to_u128(DU128 a) { return ((u128)a.hi << 64) | a.lo; }
inline constexpr DU128 du_from_u128(u128 u) { return DU128{(u64)u, (u64)(u >> 64)}; }


// ---- Anneau Z/2^128 -----------------------------------------------------------------

MHGP5_HD inline constexpr bool di_eq(DI128 a, DI128 b) { return a.lo == b.lo && a.hi == b.hi; }

MHGP5_HD inline constexpr DI128 di_add(DI128 a, DI128 b) {
  const u64 lo = a.lo + b.lo;
  return DI128{lo, a.hi + b.hi + (lo < a.lo ? 1ull : 0ull)};
}

MHGP5_HD inline constexpr DI128 di_sub(DI128 a, DI128 b) {
  return DI128{a.lo - b.lo, a.hi - b.hi - (a.lo < b.lo ? 1ull : 0ull)};
}

MHGP5_HD inline constexpr DI128 di_neg(DI128 a) { return di_sub(di_zero(), a); }

MHGP5_HD inline constexpr bool di_is_zero(DI128 a) { return (a.lo | a.hi) == 0; }
MHGP5_HD inline constexpr bool di_is_neg(DI128 a) { return (i64)a.hi < 0; }

MHGP5_HD inline constexpr int di_sign(DI128 a) { return di_is_neg(a) ? -1 : (di_is_zero(a) ? 0 : 1); }

// -1 / 0 / +1, ordre signe.
MHGP5_HD inline constexpr int di_cmp(DI128 a, DI128 b) {
  if (a.hi != b.hi) return (i64)a.hi < (i64)b.hi ? -1 : 1;
  if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
  return 0;
}

// ×2 (mod 2^128).
MHGP5_HD inline constexpr DI128 di_shl1(DI128 a) { return DI128{a.lo << 1, (a.hi << 1) | (a.lo >> 63)}; }

// /4 exact (precondition : divisible par 4). Decalage ARITHMETIQUE (C++20 :
// >> sur un signe negatif est defini comme arithmetique).
MHGP5_HD inline constexpr DI128 di_div_by_4_exact(DI128 a) {
  return DI128{(a.lo >> 2) | (a.hi << 62), (u64)((i64)a.hi >> 2)};
}

// |a| en magnitude 128 bits (|-2^127| = 2^127 : hi = 2^63, lo = 0).
MHGP5_HD inline constexpr DU128 di_abs(DI128 a) {
  if (!di_is_neg(a)) return DU128{a.lo, a.hi};
  const DI128 n = di_neg(a);
  return DU128{n.lo, n.hi};
}

// ---- Produits -------------------------------------------------------------------------

// i64 × i64 -> DI128, EXACT (toujours). Identite du produit signe par le mulhi
// non signe : avec ua = a mod 2^64, ub = b mod 2^64,
//   a·b ≡ ua·ub - 2^64·(ub·[a<0] + ua·[b<0])  (mod 2^128),
// et le vrai produit tient dans i128, donc le representant modulo est exact.
MHGP5_HD inline MHGP5_DI_CONSTEXPR DI128 di_mul_i64_i64(i64 a, i64 b) {
  const u64 ua = (u64)a, ub = (u64)b;
  u64 hi = di_mulhi_u64(ua, ub);
  if (a < 0) hi -= ub;
  if (b < 0) hi -= ua;
  return DI128{ua * ub, hi};
}

// DI128 × i64 -> DI128, EXACT ssi le vrai produit est dans [-2^127, 2^127).
// v etendu en signe sur 128 bits (vhi = 0 ou 2^64-1) ; produit mod 2^128 :
//   (a.lo + a.hi·2^64)(vlo + vhi·2^64) ≡ a.lo·vlo + 2^64·(a.hi·vlo + a.lo·vhi),
// et a.lo·vhi ≡ -a.lo quand v < 0.
MHGP5_HD inline MHGP5_DI_CONSTEXPR DI128 di_mul_di128_i64(DI128 a, i64 v) {
  const u64 uv = (u64)v;
  u64 hi = di_mulhi_u64(a.lo, uv) + a.hi * uv;
  if (v < 0) hi -= a.lo;
  return DI128{a.lo * uv, hi};
}

// ---- Vers les entiers larges non signes (niveaux) ------------------------------------

// Produit 128×128 -> 192 en limbes (precondition : produit < 2^192). Variante
// portable de wide.hpp::mul_128x128_192 (qui travaille en u128).
MHGP5_HD inline MHGP5_DI_CONSTEXPR U192 du_mul_128x128_192(DU128 x, DU128 y) {
  const u64 lo00 = x.lo * y.lo, hi00 = di_mulhi_u64(x.lo, y.lo);
  const u64 lo01 = x.lo * y.hi, hi01 = di_mulhi_u64(x.lo, y.hi);
  const u64 lo10 = x.hi * y.lo, hi10 = di_mulhi_u64(x.hi, y.lo);
  const u64 lo11 = x.hi * y.hi;  // hi11 = 0 par la precondition
  const u64 t1 = hi00 + lo01;
  const u64 c1 = t1 < hi00 ? 1ull : 0ull;
  const u64 t2 = t1 + lo10;
  const u64 c2 = t2 < t1 ? 1ull : 0ull;
  U192 r;
  r.w[0] = lo00;
  r.w[1] = t2;
  r.w[2] = hi01 + hi10 + lo11 + c1 + c2;
  return r;
}

MHGP5_HD inline MHGP5_DI_CONSTEXPR U192 du_square_192(DU128 x) { return du_mul_128x128_192(x, x); }

// Somme (mod 2^192 ; precondition : pas de debordement).
MHGP5_HD inline constexpr U192 u192_add(const U192& a, const U192& b) {
  U192 r;
  const u64 w0 = a.w[0] + b.w[0];
  const u64 c0 = w0 < a.w[0] ? 1ull : 0ull;
  const u64 w1a = a.w[1] + b.w[1];
  const u64 c1a = w1a < a.w[1] ? 1ull : 0ull;
  const u64 w1 = w1a + c0;
  const u64 c1b = w1 < w1a ? 1ull : 0ull;
  r.w[0] = w0;
  r.w[1] = w1;
  r.w[2] = a.w[2] + b.w[2] + c1a + c1b;
  return r;
}

// |a|² + |b|² + |c|² en U192 (precondition : |.| < 2^95). Variante portable de
// wide.hpp::sum_of_three_squares_192 pour |N'|² (q4_level).
MHGP5_HD inline MHGP5_DI_CONSTEXPR U192 di_sum_of_three_squares_192(DI128 a, DI128 b, DI128 c) {
  const U192 sa = du_square_192(di_abs(a));
  const U192 sb = du_square_192(di_abs(b));
  const U192 sc = du_square_192(di_abs(c));
  return u192_add(u192_add(sa, sb), sc);
}

// ---- Preuves compilees (host : le chemin u128 ; simulation device : le chemin
// portable). Les mots sont graves en hexadecimal, calcules hors C++. Pas en
// passe device reelle : `__umul64hi` n'est pas constexpr.
#if !defined(__CUDA_ARCH__) || defined(MHGP5_FAKE_DEVICE)
static_assert(di_mul_i64_i64(INT64_MIN, INT64_MIN).hi == 0x4000000000000000ull &&
                  di_mul_i64_i64(INT64_MIN, INT64_MIN).lo == 0ull,
              "(-2^63)^2 = 2^126");
static_assert(di_mul_i64_i64(INT64_MAX, INT64_MAX).hi == 0x3fffffffffffffffull &&
                  di_mul_i64_i64(INT64_MAX, INT64_MAX).lo == 1ull,
              "(2^63-1)^2 = 2^126 - 2^64 + 1");
static_assert(di_mul_i64_i64(INT64_MAX, INT64_MIN).hi == 0xc000000000000000ull &&
                  di_mul_i64_i64(INT64_MAX, INT64_MIN).lo == 0x8000000000000000ull,
              "(2^63-1)(-2^63) = -2^126 + 2^63");
static_assert(di_mul_i64_i64(-3, 5).hi == ~0ull && di_mul_i64_i64(-3, 5).lo == (u64)-15, "-3*5 = -15");
static_assert(di_mul_di128_i64(DI128{0, 0x4000000000000000ull}, -1).hi == 0xc000000000000000ull &&
                  di_mul_di128_i64(DI128{0, 0x4000000000000000ull}, -1).lo == 0ull,
              "2^126 * -1 = -2^126");
static_assert(di_mul_di128_i64(DI128{0xffffffffffffffffull, 0x7fffffffffffffffull}, 1).hi == 0x7fffffffffffffffull &&
                  di_mul_di128_i64(DI128{0xffffffffffffffffull, 0x7fffffffffffffffull}, 1).lo == ~0ull,
              "(2^127-1) * 1");
static_assert(di_mul_di128_i64(DI128{0, 0x8000000000000000ull}, 1).hi == 0x8000000000000000ull, "-2^127 * 1");
static_assert(di_cmp(di_from_i64(-1), di_from_i64(0)) == -1 && di_cmp(di_from_i64(0), di_from_i64(-1)) == 1 &&
                  di_cmp(DI128{0, 0x8000000000000000ull}, DI128{~0ull, 0x7fffffffffffffffull}) == -1,
              "ordre signe : -2^127 < 2^127-1");
static_assert(di_div_by_4_exact(di_from_i64(-8)).lo == (u64)-2 && di_div_by_4_exact(di_from_i64(-8)).hi == ~0ull,
              "-8 / 4 = -2");
static_assert(di_abs(DI128{0, 0x8000000000000000ull}).hi == 0x8000000000000000ull &&
                  di_abs(DI128{0, 0x8000000000000000ull}).lo == 0ull,
              "|-2^127| = 2^127");
static_assert(di_mulhi_u64_portable(~0ull, ~0ull) == 0xfffffffffffffffeull, "(2^64-1)^2 >> 64");
static_assert(du_mul_128x128_192(DU128{~0ull, ~0ull}, DU128{~0ull, 0}).w[2] == 0xfffffffffffffffeull &&
                  du_mul_128x128_192(DU128{~0ull, ~0ull}, DU128{~0ull, 0}).w[1] == ~0ull &&
                  du_mul_128x128_192(DU128{~0ull, ~0ull}, DU128{~0ull, 0}).w[0] == 1ull,
              "(2^128-1)(2^64-1) = 2^192 - 2^128 - 2^64 + 1");
#endif

}  // namespace mhgp5
