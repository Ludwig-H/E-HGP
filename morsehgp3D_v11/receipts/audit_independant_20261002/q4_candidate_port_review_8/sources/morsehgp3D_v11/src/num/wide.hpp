// Port explicite de R2 865f5e6, src/arith/wide.hpp (sha256 9cd1a34563501fa1c26d9ec79d510f755f49e6fc0a452e432dfc271a209f4800).
// Signe-magnitude, mots de 64 bits, sans allocation. Add/sub/resize sont transactionnels en cas de depassement.
// Adaptations : constexpr/noexcept, countl_zero standard, conversion i128 exige deux mots (pas de troncature),
// pas de conversion decimale allouante dans le produit, limites de largeur calculees par les budgets des appelants.
#pragma once

#include <array>
#include <bit>

#include "core/core.hpp"

namespace mhgp11::num {

template <int Words>
struct Wide {
  static_assert(Words >= 1 && Words <= 32, "num : 1 a 32 mots par entier large");
  bool neg = false;
  std::array<u64, Words> words{};

  static constexpr Wide from_u64(u64 value) noexcept {
    Wide out;
    out.words[0] = value;
    return out;
  }
  static constexpr Wide from_u128(u128 value) noexcept requires(Words >= 2) {
    Wide out;
    out.words[0] = static_cast<u64>(value);
    out.words[1] = static_cast<u64>(value >> 64);
    return out;
  }
  static constexpr Wide from_i128(i128 value) noexcept requires(Words >= 2) {
    const bool negative = value < 0;
    const u128 magnitude = negative ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value);
    Wide out = from_u128(magnitude);
    out.neg = negative && !out.is_zero();
    return out;
  }
  constexpr bool is_zero() const noexcept {
    for (u64 word : words)
      if (word != 0) return false;
    return true;
  }
  constexpr int sign() const noexcept { return is_zero() ? 0 : (neg ? -1 : 1); }
  constexpr Wide negated() const noexcept {
    Wide out = *this;
    out.neg = !neg && !is_zero();
    return out;
  }
  constexpr int bit_length() const noexcept {
    for (int i = Words - 1; i >= 0; --i)
      if (words[i] != 0) return 64 * i + 64 - std::countl_zero(words[i]);
    return 0;
  }
};

template <int Words>
constexpr int compare_magnitude(const Wide<Words>& a, const Wide<Words>& b) noexcept {
  for (int i = Words - 1; i >= 0; --i)
    if (a.words[i] != b.words[i]) return a.words[i] < b.words[i] ? -1 : 1;
  return 0;
}

template <int Words>
constexpr int compare(const Wide<Words>& a, const Wide<Words>& b) noexcept {
  const int sa = a.sign(), sb = b.sign();
  if (sa != sb) return sa < sb ? -1 : 1;
  const int order = compare_magnitude(a, b);
  return sa >= 0 ? order : -order;
}

namespace detail {
template <int Words>
constexpr bool add_magnitude(const Wide<Words>& a, const Wide<Words>& b, Wide<Words>& out) noexcept {
  u64 carry = 0;
  for (int i = 0; i < Words; ++i) {
    const u128 sum = static_cast<u128>(a.words[i]) + b.words[i] + carry;
    out.words[i] = static_cast<u64>(sum);
    carry = static_cast<u64>(sum >> 64);
  }
  return carry == 0;
}

// Precondition interne : |a| >= |b|.
template <int Words>
constexpr void subtract_magnitude(const Wide<Words>& a, const Wide<Words>& b, Wide<Words>& out) noexcept {
  u64 borrow = 0;
  for (int i = 0; i < Words; ++i) {
    const u128 difference = static_cast<u128>(a.words[i]) - b.words[i] - borrow;
    out.words[i] = static_cast<u64>(difference);
    borrow = (difference >> 64) != 0 ? 1 : 0;
  }
}
}  // namespace detail

template <int Words>
[[nodiscard]] constexpr bool add(const Wide<Words>& a, const Wide<Words>& b, Wide<Words>& out) noexcept {
  Wide<Words> value;
  if (a.neg == b.neg) {
    if (!detail::add_magnitude(a, b, value)) return false;
    value.neg = a.neg && !value.is_zero();
  } else if (compare_magnitude(a, b) >= 0) {
    detail::subtract_magnitude(a, b, value);
    value.neg = a.neg && !value.is_zero();
  } else {
    detail::subtract_magnitude(b, a, value);
    value.neg = b.neg && !value.is_zero();
  }
  out = value;
  return true;
}

template <int Words>
[[nodiscard]] constexpr bool subtract(const Wide<Words>& a, const Wide<Words>& b, Wide<Words>& out) noexcept {
  return add(a, b.negated(), out);
}

// A+B mots suffisent : le produit de deux magnitudes de 64A et 64B bits est strictement < 2^(64(A+B)).
// Dans une colonne, (2^64-1)^2 + (2^64-1) + (2^64-1) = 2^128-1 : l'accumulateur u128 ne deborde pas.
template <int A, int B>
constexpr Wide<A + B> multiply(const Wide<A>& a, const Wide<B>& b) noexcept {
  Wide<A + B> out;
  for (int i = 0; i < A; ++i) {
    u64 carry = 0;
    for (int j = 0; j < B; ++j) {
      const u128 value = static_cast<u128>(a.words[i]) * b.words[j] + out.words[i + j] + carry;
      out.words[i + j] = static_cast<u64>(value);
      carry = static_cast<u64>(value >> 64);
    }
    out.words[i + B] = carry;
  }
  out.neg = (a.neg != b.neg) && !out.is_zero();
  return out;
}

template <int To, int From>
[[nodiscard]] constexpr bool resize(const Wide<From>& a, Wide<To>& out) noexcept {
  Wide<To> value;
  for (int i = 0; i < From; ++i) {
    if (i < To) value.words[i] = a.words[i];
    else if (a.words[i] != 0) return false;
  }
  value.neg = a.neg && !value.is_zero();
  out = value;
  return true;
}

}  // namespace mhgp11::num
