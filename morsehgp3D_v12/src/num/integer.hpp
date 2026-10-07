// Selection par budget |x| < 2^Bits : i64, i128, puis signe-magnitude a nombre de mots calcule.
// Les conversions publiques verifient le budget AVANT la conversion native. Aucun depassement signe en C++.
#pragma once

#include <optional>
#include <type_traits>

#include "num/wide.hpp"

namespace mhgp12::num {

template <int Bits>
struct IntegerType {
  static_assert(Bits > 0 && Bits <= 1024, "num_budget_invalide : 1 <= Bits <= 1024");
  using Type = std::conditional_t<(Bits <= 63), i64,
               std::conditional_t<(Bits <= 127), i128, Wide<(Bits + 63) / 64>>>;
};
template <int Bits>
using Int = typename IntegerType<Bits>::Type;

template <int Bits>
constexpr Int<Bits> integer_one() noexcept {
  if constexpr (Bits <= 127) return Int<Bits>{1};
  else return Int<Bits>::from_u64(1);
}

static_assert(std::same_as<Int<63>, i64> && std::same_as<Int<64>, i128> && std::same_as<Int<127>, i128> &&
              std::same_as<Int<128>, Wide<2>>, "num : seuils des types signes exacts");

constexpr Wide<2> to_wide(i128 value) noexcept { return Wide<2>::from_i128(value); }
constexpr Wide<2> to_wide(i64 value) noexcept { return Wide<2>::from_i128(value); }
constexpr Wide<2> to_wide(u128 value) noexcept { return Wide<2>::from_u128(value); }
constexpr Wide<2> to_wide(u64 value) noexcept { return Wide<2>::from_u128(value); }
template <int Words>
constexpr Wide<Words> to_wide(const Wide<Words>& value) noexcept { return value; }

// Conversion verifiee, rien si |value| >= 2^Bits ; zero negatif normalise. Ne transforme pas une entree invalide
// en invariant interne. La borne stricte exclut les minima i64/i128 lorsqu'ils ne satisfont pas le budget.
template <int Bits, int Words>
constexpr std::optional<Int<Bits>> narrow(const Wide<Words>& value) noexcept {
  if (value.bit_length() > Bits) return std::nullopt;
  if constexpr (Bits <= 127) {
    u128 magnitude = value.words[0];
    if constexpr (Words >= 2) magnitude |= static_cast<u128>(value.words[1]) << 64;
    const i128 signed_value = value.sign() < 0 ? -static_cast<i128>(magnitude) : static_cast<i128>(magnitude);
    return static_cast<Int<Bits>>(signed_value);
  } else {
    Int<Bits> out;
    if (!resize(value, out)) return std::nullopt;
    return out;
  }
}

namespace detail {
// Ces deux gardes servent aux expressions dont la borne est deja demontree ; leur refus est alors un invariant.
// Les fabriques publiques filtrent d'abord leurs entrees (Point et Level).
template <int Bits, int Words>
Result<Int<Bits>> require_fit(const Wide<Words>& value) noexcept {
  auto out = narrow<Bits>(value);
  if (!out) return fail(Reason::arithmetic_invariant);
  return *out;
}
template <int Words>
Result<Wide<Words>> require_add(const Wide<Words>& a, const Wide<Words>& b) noexcept {
  Wide<Words> out;
  if (!add(a, b, out)) return fail(Reason::arithmetic_invariant);
  return out;
}
}  // namespace detail

}  // namespace mhgp12::num
