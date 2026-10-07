// Outils internes des bornes entieres et de l'ordre des centres : plancher exact de N/D (D>0) en local, saturation.
#pragma once

#include "num/geometry_internal.hpp"

namespace mhgp12::num::detail {

// Plancher exact de C/D pour D>0 : C++ tronque vers zero, le reste negatif est ramene dans [0,D).
struct FloorDivision {
  i128 quotient, remainder;
};
inline FloorDivision floor_division(i128 numerator, i128 denominator) noexcept {
  i128 quotient = numerator / denominator;  // une seule division large ; |quotient*D| <= |numerator|, aucun depassement
  i128 remainder = numerator - quotient * denominator;
  if (remainder < 0) { --quotient; remainder += denominator; }
  return {quotient, remainder};
}

// Valeur ramenee dans [lo, hi] (lo <= hi), en i64.
inline i64 saturate(i128 value, i64 lo, i64 hi) noexcept {
  return value < lo ? lo : value > hi ? hi : static_cast<i64>(value);
}

}  // namespace mhgp12::num::detail
