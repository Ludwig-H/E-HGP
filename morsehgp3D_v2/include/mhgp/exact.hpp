// MorseHGP3D v2 — arithmetique exacte.
//
// Les coordonnees d'entree sont quantifiees en entiers de [0, 2^kCoordBits).
// Toutes les largeurs employees plus bas sont bornees statiquement pour
// kCoordBits = 16 (les bornes sont rappelees a chaque fonction) ; aucun test
// n'est jamais decide par un flottant.
#pragma once

#include <cstdint>

#if defined(__CUDACC__)
#define MHGP_HD __host__ __device__
#else
#define MHGP_HD
#endif

namespace mhgp {

using u8 = std::uint8_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
#if defined(__SIZEOF_INT128__)
using i128 = __int128;
using u128 = unsigned __int128;
#else
#error "MorseHGP3D v2 requiert __int128"
#endif

inline constexpr int kCoordBits = 16;
inline constexpr i64 kCoordMax = (i64(1) << kCoordBits) - 1;

// ---------------------------------------------------------------------------
// Entier signe de N mots de 64 bits, complement a deux, little endian.
// ---------------------------------------------------------------------------
template <int N>
struct BigInt {
  u64 w[N];
};

template <int N>
MHGP_HD inline BigInt<N> big_zero() {
  BigInt<N> r;
  for (int i = 0; i < N; ++i) r.w[i] = 0;
  return r;
}

template <int N>
MHGP_HD inline BigInt<N> big_from_i128(i128 v) {
  BigInt<N> r;
  const u128 uv = static_cast<u128>(v);
  const u64 sign = (v < 0) ? ~u64(0) : u64(0);
  r.w[0] = static_cast<u64>(uv);
  if (N > 1) r.w[1] = static_cast<u64>(uv >> 64);
  for (int i = 2; i < N; ++i) r.w[i] = sign;
  return r;
}

template <int N>
MHGP_HD inline bool big_negative(const BigInt<N>& a) {
  return (a.w[N - 1] >> 63) != 0;
}

template <int N>
MHGP_HD inline bool big_is_zero(const BigInt<N>& a) {
  u64 acc = 0;
  for (int i = 0; i < N; ++i) acc |= a.w[i];
  return acc == 0;
}

template <int N>
MHGP_HD inline BigInt<N> big_neg(const BigInt<N>& a) {
  BigInt<N> r;
  u64 carry = 1;
  for (int i = 0; i < N; ++i) {
    const u64 v = ~a.w[i];
    const u64 s = v + carry;
    carry = (s < v) ? 1u : 0u;
    r.w[i] = s;
  }
  return r;
}

template <int N>
MHGP_HD inline BigInt<N> big_add(const BigInt<N>& a, const BigInt<N>& b) {
  BigInt<N> r;
  u64 carry = 0;
  for (int i = 0; i < N; ++i) {
    const u64 s = a.w[i] + b.w[i];
    const u64 c1 = (s < a.w[i]) ? 1u : 0u;
    const u64 s2 = s + carry;
    const u64 c2 = (s2 < s) ? 1u : 0u;
    r.w[i] = s2;
    carry = c1 | c2;
  }
  return r;
}

template <int N>
MHGP_HD inline BigInt<N> big_sub(const BigInt<N>& a, const BigInt<N>& b) {
  return big_add(a, big_neg(b));
}

template <int N>
MHGP_HD inline int big_sign(const BigInt<N>& a) {
  if (big_is_zero(a)) return 0;
  return big_negative(a) ? -1 : 1;
}

// Comparaison sans passer par une soustraction : la difference mathematique
// peut deborder la largeur signee, auquel cas son signe ne donne pas l'ordre.
template <int N>
MHGP_HD inline int big_cmp(const BigInt<N>& a, const BigInt<N>& b) {
  const bool na = big_negative(a), nb = big_negative(b);
  if (na != nb) return na ? -1 : 1;
  for (int i = N - 1; i >= 0; --i) {
    if (a.w[i] != b.w[i]) return (a.w[i] < b.w[i]) ? -1 : 1;
  }
  return 0;
}

// Magnitude d'un i128 sans negation signee (definie meme pour INT128_MIN).
MHGP_HD inline u128 abs_u128(i128 v) {
  const u128 u = static_cast<u128>(v);
  return (v < 0) ? (~u + 1) : u;
}

template <int N>
MHGP_HD inline BigInt<N> big_widen(const BigInt<2>& a) {
  BigInt<N> r;
  const u64 sign = big_negative(a) ? ~u64(0) : u64(0);
  r.w[0] = a.w[0];
  r.w[1] = a.w[1];
  for (int i = 2; i < N; ++i) r.w[i] = sign;
  return r;
}

// Produit exact 128 x 128 -> 256 bits.
MHGP_HD inline BigInt<4> mul128(i128 a, i128 b) {
  const bool neg = (a < 0) != (b < 0);
  const u128 ua = abs_u128(a);
  const u128 ub = abs_u128(b);
  const u64 a0 = static_cast<u64>(ua), a1 = static_cast<u64>(ua >> 64);
  const u64 b0 = static_cast<u64>(ub), b1 = static_cast<u64>(ub >> 64);

  const u128 p00 = static_cast<u128>(a0) * b0;
  const u128 p01 = static_cast<u128>(a0) * b1;
  const u128 p10 = static_cast<u128>(a1) * b0;
  const u128 p11 = static_cast<u128>(a1) * b1;

  BigInt<4> r;
  r.w[0] = static_cast<u64>(p00);
  const u128 mid = (p00 >> 64) + static_cast<u64>(p01) + static_cast<u64>(p10);
  r.w[1] = static_cast<u64>(mid);
  const u128 hi = (mid >> 64) + (p01 >> 64) + (p10 >> 64) + p11;
  r.w[2] = static_cast<u64>(hi);
  r.w[3] = static_cast<u64>(hi >> 64);
  return neg ? big_neg(r) : r;
}

// Multiplie un BigInt<M> par un i128 positif, resultat sur N mots (N >= M + 2).
// Exact tant que le produit tient sur N mots.
template <int N, int M>
MHGP_HD inline BigInt<N> big_mul_i128(const BigInt<M>& a, i128 b) {
  const bool na = big_negative(a);
  const BigInt<M> ua = na ? big_neg(a) : a;
  const u128 ub = abs_u128(b);
  const u64 b0 = static_cast<u64>(ub);
  const u64 b1 = static_cast<u64>(ub >> 64);
  BigInt<N> r = big_zero<N>();
  u128 carry = 0;
  for (int i = 0; i < M && i < N; ++i) {
    const u128 cur = static_cast<u128>(r.w[i]) + static_cast<u128>(ua.w[i]) * b0 + carry;
    r.w[i] = static_cast<u64>(cur);
    carry = cur >> 64;
  }
  for (int i = M; i < N && carry; ++i) {
    const u128 cur = static_cast<u128>(r.w[i]) + carry;
    r.w[i] = static_cast<u64>(cur);
    carry = cur >> 64;
  }
  carry = 0;
  for (int i = 0; i < M && i + 1 < N; ++i) {
    const u128 cur = static_cast<u128>(r.w[i + 1]) + static_cast<u128>(ua.w[i]) * b1 + carry;
    r.w[i + 1] = static_cast<u64>(cur);
    carry = cur >> 64;
  }
  for (int i = M + 1; i < N && carry; ++i) {
    const u128 cur = static_cast<u128>(r.w[i]) + carry;
    r.w[i] = static_cast<u64>(cur);
    carry = cur >> 64;
  }
  const bool neg = (na != (b < 0));
  return neg ? big_neg(r) : r;
}

template <int N>
MHGP_HD inline double big_to_double(const BigInt<N>& a) {
  const bool neg = big_negative(a);
  const BigInt<N> v = neg ? big_neg(a) : a;
  double r = 0.0;
  for (int i = N - 1; i >= 0; --i) r = r * 18446744073709551616.0 + static_cast<double>(v.w[i]);
  return neg ? -r : r;
}

// ---------------------------------------------------------------------------
// Points entiers
// ---------------------------------------------------------------------------
struct P3 {
  i64 x, y, z;
};

MHGP_HD inline P3 p3_sub(const P3& a, const P3& b) {
  return P3{a.x - b.x, a.y - b.y, a.z - b.z};
}

MHGP_HD inline i128 p3_dot(const P3& a, const P3& b) {
  return i128(a.x) * b.x + i128(a.y) * b.y + i128(a.z) * b.z;
}

MHGP_HD inline i128 p3_norm2(const P3& a) { return p3_dot(a, a); }

MHGP_HD inline P3 p3_cross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// det 3x3 ; entrees < 2^(kCoordBits+1) => |det| < 6 * 2^(3*17) = 2^54.6
MHGP_HD inline i128 det3(const P3& a, const P3& b, const P3& c) {
  return i128(a.x) * (i128(b.y) * c.z - i128(b.z) * c.y)
       - i128(a.y) * (i128(b.x) * c.z - i128(b.z) * c.x)
       + i128(a.z) * (i128(b.x) * c.y - i128(b.y) * c.x);
}

MHGP_HD inline int orient3d(const P3& a, const P3& b, const P3& c, const P3& d) {
  const i128 v = det3(p3_sub(a, d), p3_sub(b, d), p3_sub(c, d));
  return (v > 0) - (v < 0);
}

}  // namespace mhgp
