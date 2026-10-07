// Fin d'etage, source unique hote et appareil : niveau exact d'une boule en entiers a mots, cle F3 et ordre F4,
// comparaison exacte de deux niveaux. Memes formules que num::Sphere::through (src/num/sphere.cpp), donc memes
// numerateur et denominateur NON REDUITS que le niveau rendu par num :
//   q2 (a, b)       : N = |b-a|^2, D = 4 ;
//   q3 (a, b, c)    : N = |b-a|^2 |c-a|^2 |c-b|^2, D = 4 |(b-a) x (c-a)|^2 ;
//   q4 (a, b, c, d) : N = sum_j N_j^2 avec N_j = |u|^2 (v x s)_j + |v|^2 (s x u)_j + |s|^2 (u x v)_j (u = b-a, v = c-a,
//                     s = d-a), D = (2 det)^2, det = u . (v x s).
// Chaque forme est symetrique en ses sommets (produits des carres des cotes, carre de l'aire, carre du volume ; |N|^2
// = R^2 D^2) : le choix de l'ancre ne change rien. Toutes les grandeurs ne lisent que des differences de coordonnees
// (|delta| < 2^32) : carres et produits scalaires en u128 ou i128, produits vectoriels en i128 (< 2^65), puis entiers a
// mots de 64 bits ; numerateur sur 8B+12 bits et denominateur sur 6B+8 bits (budgets du niveau, num/budgets.hpp).
// Cle F3 et marge F4 : celles de la voie CPU de la v11 (sort_level_key.hpp, ac081a06f) ; aucune decision en flottant,
// la cle ne fait que trier et toute paire non certainement ordonnee est comparee en exact (doctrine F1 a F4).
#pragma once

#include <cmath>

#include "catalogue/simt.hpp"

namespace mhgp12::catalogue_detail::fin {

inline constexpr int kNumWords = (8 * kCoordBits + 12 + 63) / 64;  // 3, 4, 5 aux profils 21, 24, 32
inline constexpr int kDenWords = (6 * kCoordBits + 8 + 63) / 64;   // 3, 3, 4

template <int W>
struct Words {
  u64 w[W];
};

// Niveau exact d'une boule : N / D, D > 0, mots de poids croissant.
struct LevelWords {
  u64 n[kNumWords];
  u64 d[kDenWords];
};

MHGP12_HD int clz64(u64 x) {  // x != 0
#if defined(__CUDA_ARCH__)
  return __clzll(static_cast<long long>(x));
#else
  return __builtin_clzll(x);
#endif
}

template <int W>
MHGP12_HD int bit_length(const u64 (&w)[W]) {
  for (int i = W - 1; i >= 0; --i)
    if (w[i] != 0) return 64 * i + 64 - clz64(w[i]);
  return 0;
}

template <int W>
MHGP12_HD Words<W> from_u128(u128 v) {
  Words<W> out{};
  out.w[0] = static_cast<u64>(v);
  if constexpr (W > 1) out.w[1] = static_cast<u64>(v >> 64);
  return out;
}

// Produit exact de deux entiers non signes : A + B mots.
template <int A, int B>
MHGP12_HD Words<A + B> mul(const Words<A>& a, const Words<B>& b) {
  Words<A + B> out{};
  for (int i = 0; i < A; ++i) {
    u64 carry = 0;
    for (int j = 0; j < B; ++j) {
      const u128 t = static_cast<u128>(a.w[i]) * b.w[j] + out.w[i + j] + carry;
      out.w[i + j] = static_cast<u64>(t);
      carry = static_cast<u64>(t >> 64);
    }
    out.w[i + B] = carry;
  }
  return out;
}

// a += b ; vrai si une retenue sort du dernier mot (impossible par les bornes : faute).
template <int W>
MHGP12_HD bool add_into(Words<W>& a, const Words<W>& b) {
  u64 carry = 0;
  for (int i = 0; i < W; ++i) {
    const u128 t = static_cast<u128>(a.w[i]) + b.w[i] + carry;
    a.w[i] = static_cast<u64>(t);
    carry = static_cast<u64>(t >> 64);
  }
  return carry != 0;
}

// a - b pour a >= b.
template <int W>
MHGP12_HD Words<W> sub(const Words<W>& a, const Words<W>& b) {
  Words<W> out{};
  u64 borrow = 0;
  for (int i = 0; i < W; ++i) {
    const u64 x = a.w[i], y = b.w[i];
    out.w[i] = x - y - borrow;
    borrow = (x < y || (x == y && borrow != 0)) ? 1u : 0u;
  }
  return out;
}

template <int W>
MHGP12_HD int cmp(const u64 (&a)[W], const u64 (&b)[W]) {
  for (int i = W - 1; i >= 0; --i)
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  return 0;
}

// Entier signe a mots (signe et valeur absolue) : sommes des trois termes de N_j en q4.
template <int W>
struct Signed {
  Words<W> mag;
  bool neg;
};

template <int W>
MHGP12_HD Signed<W> add_signed(const Signed<W>& a, const Signed<W>& b, bool& fault) {
  if (a.neg == b.neg) {
    Signed<W> out = a;
    fault = add_into(out.mag, b.mag) || fault;
    return out;
  }
  const int c = cmp(a.mag.w, b.mag.w);
  if (c >= 0) return Signed<W>{sub(a.mag, b.mag), a.neg && c != 0};
  return Signed<W>{sub(b.mag, a.mag), b.neg};
}

MHGP12_HD u128 abs128(i128 v) { return v < 0 ? u128{0} - static_cast<u128>(v) : static_cast<u128>(v); }

// Copie dans W mots ; faute si un mot superieur non nul serait perdu (budget du profil depasse).
template <int W, int V>
MHGP12_HD void narrow_into(u64 (&out)[W], const Words<V>& v, bool& fault) {
  for (int i = 0; i < W; ++i) out[i] = i < V ? v.w[i] : 0u;
  for (int i = W; i < V; ++i) fault = fault || v.w[i] != 0;
}

struct Delta {
  i64 v[3];
};
MHGP12_HD Delta delta(const u32* p, const u32* q) {  // q - p
  return Delta{{i64(q[0]) - i64(p[0]), i64(q[1]) - i64(p[1]), i64(q[2]) - i64(p[2])}};
}
MHGP12_HD u128 norm2(const Delta& u) {  // < 3 * 2^64
  u128 s = 0;
  for (int j = 0; j < 3; ++j) {
    const u64 m = static_cast<u64>(u.v[j] < 0 ? -u.v[j] : u.v[j]);
    s += static_cast<u128>(m) * m;
  }
  return s;
}
struct Cross {
  i128 v[3];
};
MHGP12_HD Cross cross(const Delta& u, const Delta& v) {  // |.| < 2^65
  return Cross{{i128(u.v[1]) * v.v[2] - i128(u.v[2]) * v.v[1], i128(u.v[2]) * v.v[0] - i128(u.v[0]) * v.v[2],
                i128(u.v[0]) * v.v[1] - i128(u.v[1]) * v.v[0]}};
}

// Somme des carres de trois entiers signes de 128 bits au plus (|x| < 2^126) : 4 mots.
MHGP12_HD Words<4> square_sum(const i128 (&x)[3]) {
  Words<4> s{};
  for (int j = 0; j < 3; ++j) {
    const Words<2> m = from_u128<2>(abs128(x[j]));
    (void)add_into(s, mul(m, m));
  }
  return s;
}

MHGP12_HD void level_q2(const u32* a, const u32* b, LevelWords& out, bool& fault) {
  const u128 n = norm2(delta(a, b));
  fault = fault || n == 0;
  narrow_into(out.n, from_u128<2>(n), fault);
  narrow_into(out.d, from_u128<1>(4), fault);
}

MHGP12_HD void level_q3(const u32* a, const u32* b, const u32* c, LevelWords& out, bool& fault) {
  const Delta u = delta(a, b), v = delta(a, c), w = delta(b, c);
  const Words<6> n = mul(mul(from_u128<2>(norm2(u)), from_u128<2>(norm2(v))), from_u128<2>(norm2(w)));
  const Cross x = cross(u, v);
  const i128 xs[3] = {x.v[0], x.v[1], x.v[2]};
  Words<4> g = square_sum(xs);  // |u x v|^2 < 3 * 2^130
  fault = fault || bit_length(g.w) == 0;
  Words<4> d = g;  // D = 4 g
  (void)add_into(d, g);
  const Words<4> twice = d;
  (void)add_into(d, twice);
  narrow_into(out.n, n, fault);
  narrow_into(out.d, d, fault);
}

MHGP12_HD void level_q4(const u32* a, const u32* b, const u32* c, const u32* e, LevelWords& out, bool& fault) {
  const Delta u = delta(a, b), v = delta(a, c), s = delta(a, e);
  const Cross vs = cross(v, s), su = cross(s, u), uv = cross(u, v);
  const i128 det = i128(u.v[0]) * vs.v[0] + i128(u.v[1]) * vs.v[1] + i128(u.v[2]) * vs.v[2];  // < 3 * 2^97
  fault = fault || det == 0;
  const Words<2> uu = from_u128<2>(norm2(u)), vv = from_u128<2>(norm2(v)), ss = from_u128<2>(norm2(s));
  Words<8> n{};
  for (int j = 0; j < 3; ++j) {
    const Signed<4> t0{mul(uu, from_u128<2>(abs128(vs.v[j]))), vs.v[j] < 0};
    const Signed<4> t1{mul(vv, from_u128<2>(abs128(su.v[j]))), su.v[j] < 0};
    const Signed<4> t2{mul(ss, from_u128<2>(abs128(uv.v[j]))), uv.v[j] < 0};
    const Signed<4> nj = add_signed(add_signed(t0, t1, fault), t2, fault);
    fault = add_into(n, mul(nj.mag, nj.mag)) || fault;
  }
  const Words<2> d2 = from_u128<2>(2 * abs128(det));
  narrow_into(out.n, n, fault);
  narrow_into(out.d, mul(d2, d2), fault);
}

// Niveau du support p[0..q) (q = 2, 3 ou 4) ; faute sur un support degenere ou un budget depasse (invariant).
MHGP12_HD void ball_level(const u32* const* p, u32 q, LevelWords& out, bool& fault) {
  if (q == 2) level_q2(p[0], p[1], out, fault);
  else if (q == 3) level_q3(p[0], p[1], p[2], out, fault);
  else if (q == 4) level_q4(p[0], p[1], p[2], p[3], out, fault);
  else fault = true;
}

// Cle F3 d'un entier non signe : 64 bits de tete en binaire64, mis a l'echelle exactement (ldexp).
template <int W>
MHGP12_HD double magnitude_key(const u64 (&w)[W]) {
  const int length = bit_length(w);
  if (length <= 64) return static_cast<double>(w[0]);
  const int shift = length - 64, word = shift / 64, bit = shift % 64;
  u64 top = w[word] >> bit;
  if (bit != 0) top |= w[word + 1] << (64 - bit);
  return ldexp(static_cast<double>(top), shift);
}

// Cle F3 du niveau (quotient de deux cles de tete, binaire64 positif) et ses bits (ordre des bits = ordre des cles).
MHGP12_HD u64 level_key_bits(const LevelWords& level) {
  const double key = magnitude_key(level.n) / magnitude_key(level.d);
  u64 bits = 0;
  memcpy(&bits, &key, sizeof bits);
  return bits;
}

// F4 : -1/+1 ordre CERTAIN des niveaux exacts ; 0 repli exact (cles egales ou voisines). c <= (1-u)^(Ex+Ey+1).
MHGP12_HD int key_order(u64 a_bits, u64 b_bits) {
  double a = 0, b = 0;
  memcpy(&a, &a_bits, sizeof a);
  memcpy(&b, &b_bits, sizeof b);
  constexpr double kOrdered = 1.0 - 0x1p-40;
  if (a == 0 || b == 0) return (a > b) - (a < b);
  if (a < kOrdered * b) return -1;
  if (b < kOrdered * a) return 1;
  return 0;
}

// Comparaison exacte de deux niveaux par produits croises N_a D_b et N_b D_a.
MHGP12_HD int compare_levels(const LevelWords& a, const LevelWords& b) {
  Words<kNumWords> na{}, nb{};
  Words<kDenWords> da{}, db{};
  for (int i = 0; i < kNumWords; ++i) {
    na.w[i] = a.n[i];
    nb.w[i] = b.n[i];
  }
  for (int i = 0; i < kDenWords; ++i) {
    da.w[i] = a.d[i];
    db.w[i] = b.d[i];
  }
  const Words<kNumWords + kDenWords> left = mul(na, db), right = mul(nb, da);
  return cmp(left.w, right.w);
}

}  // namespace mhgp12::catalogue_detail::fin
