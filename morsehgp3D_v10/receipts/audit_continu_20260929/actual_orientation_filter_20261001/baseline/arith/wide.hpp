// Entiers larges exacts a largeur fixe (signe-magnitude, mots de 64 bits, petit-boutiste).
//
// Usage : comparaisons de niveaux rationnels (produits croises jusqu'a ~2^300) et predicats dont les
// bornes depassent l'i128. Aucune allocation. Deux familles d'operations :
//  - faillibles (add, sub, resize) : rendent false si le resultat ne tient pas ; [[nodiscard]], chaque appelant teste
//    le resultat ou prouve qu'il est toujours vrai ;
//  - infaillibles par construction (mul, sum, widen) : le type du resultat a assez de mots pour tout operande
//    (Wide<A> * Wide<B> -> Wide<A + B>, Wide<L> + Wide<L> -> Wide<L + 1>, Wide<From> -> Wide<To >= From>).
#pragma once

#include <string>

#include "core/types.hpp"

namespace mhgp10::arith {

template <int L>
struct Wide {
  static_assert(L >= 1 && L <= 8, "Wide<L> : 1 <= L <= 8 mots");
  bool neg = false;
  u64 w[L] = {};

  [[nodiscard]] static Wide from_u64(u64 v) {
    Wide r;
    r.w[0] = v;
    return r;
  }
  [[nodiscard]] static Wide from_i64(i64 v) {
    Wide r = from_u64(v < 0 ? u64(0) - static_cast<u64>(v) : static_cast<u64>(v));
    r.neg = v < 0;
    return r;
  }
  // Wide<1> exclu : le mot haut serait perdu en silence (constat P9 de l'audit precision, 30 septembre 2026).
  [[nodiscard]] static Wide from_u128(u128 v)
    requires(L >= 2)
  {
    Wide r;
    r.w[0] = static_cast<u64>(v);
    r.w[1] = static_cast<u64>(v >> 64);
    return r;
  }
  [[nodiscard]] static Wide from_i128(i128 v)
    requires(L >= 2)
  {
    const bool n = v < 0;
    const u128 m = n ? u128(0) - static_cast<u128>(v) : static_cast<u128>(v);
    Wide r = from_u128(m);
    r.neg = n && !r.is_zero();
    return r;
  }
  bool is_zero() const {
    for (int i = 0; i < L; ++i)
      if (w[i]) return false;
    return true;
  }
  int sign() const { return is_zero() ? 0 : (neg ? -1 : 1); }
  Wide negated() const {
    Wide r = *this;
    r.neg = !neg && !is_zero();
    return r;
  }
  int bit_length() const {
    for (int i = L - 1; i >= 0; --i)
      if (w[i]) return 64 * i + 64 - __builtin_clzll(w[i]);
    return 0;
  }
};

template <int L>
int cmp_mag(const Wide<L>& a, const Wide<L>& b) {
  for (int i = L - 1; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

template <int L>
int cmp(const Wide<L>& a, const Wide<L>& b) {
  const int sa = a.sign(), sb = b.sign();
  if (sa != sb) return sa < sb ? -1 : 1;
  const int m = cmp_mag(a, b);
  return sa >= 0 ? m : -m;
}

namespace detail {
template <int L>
[[nodiscard]] bool add_mag(const Wide<L>& a, const Wide<L>& b, Wide<L>& out) {
  u64 carry = 0;
  for (int i = 0; i < L; ++i) {
    const u128 s = u128(a.w[i]) + b.w[i] + carry;
    out.w[i] = static_cast<u64>(s);
    carry = static_cast<u64>(s >> 64);
  }
  return carry == 0;
}
template <int L>
void sub_mag(const Wide<L>& a, const Wide<L>& b, Wide<L>& out) {  // |a| >= |b|
  u64 borrow = 0;
  for (int i = 0; i < L; ++i) {
    const u128 d = u128(a.w[i]) - b.w[i] - borrow;
    out.w[i] = static_cast<u64>(d);
    borrow = static_cast<u64>(d >> 64) ? 1 : 0;
  }
}
}  // namespace detail

// out = a + b ; false en cas de debordement.
template <int L>
[[nodiscard]] bool add(const Wide<L>& a, const Wide<L>& b, Wide<L>& out) {
  Wide<L> r;
  if (a.neg == b.neg) {
    if (!detail::add_mag(a, b, r)) return false;
    r.neg = a.neg && !r.is_zero();
  } else if (cmp_mag(a, b) >= 0) {
    detail::sub_mag(a, b, r);
    r.neg = a.neg && !r.is_zero();
  } else {
    detail::sub_mag(b, a, r);
    r.neg = b.neg && !r.is_zero();
  }
  out = r;
  return true;
}

template <int L>
[[nodiscard]] bool sub(const Wide<L>& a, const Wide<L>& b, Wide<L>& out) {
  return add(a, b.negated(), out);
}

// Produit exact : Wide<A> * Wide<B> -> Wide<A + B> (jamais de debordement).
template <int A, int B>
[[nodiscard]] Wide<A + B> mul(const Wide<A>& a, const Wide<B>& b) {
  Wide<A + B> r;
  for (int i = 0; i < A; ++i) {
    u64 carry = 0;
    for (int j = 0; j < B; ++j) {
      const u128 t = u128(a.w[i]) * b.w[j] + r.w[i + j] + carry;
      r.w[i + j] = static_cast<u64>(t);
      carry = static_cast<u64>(t >> 64);
    }
    r.w[i + B] = carry;
  }
  r.neg = (a.neg != b.neg) && !r.is_zero();
  return r;
}

// Changement de largeur ; false si la valeur ne tient pas.
template <int To, int From>
[[nodiscard]] bool resize(const Wide<From>& a, Wide<To>& out) {
  Wide<To> r;
  for (int i = 0; i < From; ++i) {
    if (i < To) r.w[i] = a.w[i];
    else if (a.w[i]) return false;
  }
  r.neg = a.neg && !r.is_zero();
  out = r;
  return true;
}

// Elargissement (To >= From) : aucune perte possible, donc aucune valeur d'echec.
template <int To, int From>
[[nodiscard]] Wide<To> widen(const Wide<From>& a) {
  static_assert(To >= From, "widen : la largeur cible doit contenir la source");
  Wide<To> r;
  for (int i = 0; i < From; ++i) r.w[i] = a.w[i];
  r.neg = a.neg;
  return r;
}

// Somme exacte : Wide<L> + Wide<L> -> Wide<L + 1> (jamais de debordement : |a| + |b| < 2^(64 L + 1)).
template <int L>
[[nodiscard]] Wide<L + 1> sum(const Wide<L>& a, const Wide<L>& b) {
  Wide<L + 1> r;
  if (a.neg == b.neg) {
    u64 carry = 0;
    for (int i = 0; i < L; ++i) {
      const u128 s = u128(a.w[i]) + b.w[i] + carry;
      r.w[i] = static_cast<u64>(s);
      carry = static_cast<u64>(s >> 64);
    }
    r.w[L] = carry;
    r.neg = a.neg && !r.is_zero();
    return r;
  }
  Wide<L> m;
  const bool a_big = cmp_mag(a, b) >= 0;
  if (a_big) detail::sub_mag(a, b, m);
  else detail::sub_mag(b, a, m);
  for (int i = 0; i < L; ++i) r.w[i] = m.w[i];
  r.neg = (a_big ? a.neg : b.neg) && !r.is_zero();
  return r;
}

// Approximation double (tri approche des niveaux, filtres a repli exact, diagnostics). Horner sur les mots, du plus
// fort au plus faible : d <- d 2^64 + fl(w_i). Le produit par 2^64 est exact (puissance de deux ; |v| < 2^512, loin du
// debordement et des sous-normaux). Chaque etape arrondit la conversion du mot, puis l'addition (d = 0 au premier mot :
// addition exacte). Le mot de tete subit 1 + (L - 1) = L arrondis, le mot i < L - 1 en subit 1 + (i + 1) <= L ; tous
// les termes sont de meme signe, donc |d - v| <= gamma_L |v|, gamma_L = L u / (1 - L u) < (L + 2) u, u = 2^-53 en
// arrondi au plus proche (2 u par operation sous un arrondi dirige : gamma_L avec 2 u). Pour L = 3 (niveaux) :
// erreur relative <= gamma_3, et fl(num / den) a une erreur relative <= 2 gamma_3 + u + O(u^2) < 8 u = 2^-50.
template <int L>
double to_double(const Wide<L>& v) {
  double d = 0;
  for (int i = L - 1; i >= 0; --i) d = d * 18446744073709551616.0 + static_cast<double>(v.w[i]);
  return v.neg ? -d : d;
}

// Representation decimale (tests et diagnostics seulement).
template <int L>
std::string to_string(Wide<L> a) {
  if (a.is_zero()) return "0";
  std::string s;
  const bool n = a.neg;
  while (!a.is_zero()) {
    u128 rem = 0;
    for (int i = L - 1; i >= 0; --i) {
      const u128 cur = (rem << 64) | a.w[i];
      a.w[i] = static_cast<u64>(cur / 10);
      rem = cur % 10;
    }
    s.push_back(static_cast<char>('0' + static_cast<int>(rem)));
  }
  if (n) s.push_back('-');
  return std::string(s.rbegin(), s.rend());
}

using I128w = Wide<2>;
using I192 = Wide<3>;
using I256 = Wide<4>;
using I320 = Wide<5>;

}  // namespace mhgp10::arith
