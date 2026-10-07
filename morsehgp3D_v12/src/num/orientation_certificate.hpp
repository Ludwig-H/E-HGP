// Certificat d'orientation lie a son domaine (CST-0201) : pour trois points d'un MEME cube de cote < 2^t et une ancre
// a moins de 2^t du premier, D < 2^(124-3t) et |N_j| < 2^(124-2t) donnent |N_j + D offset_j| < 2^(125-2t) ; la normale
// de trois points d'un meme cube a chaque composante < 2^(2t) (determinant multiaffine, extrema aux coins) : trois
// produits < 2^125, somme des magnitudes < 3*2^125 < 2^127. Jamais pour deux Vec arbitraires. Le certificat de la v11
// (global_orientation_i128) est le cas t = B, domaine de tous les Point du profil.
#pragma once
#include <array>
#include "num/power_certificate.hpp"

namespace mhgp12::num::detail {

inline constexpr bool orientation_certificate_i128(i128 denominator, const std::array<i128, 3>& numerator,
                                                   int t) noexcept {
  if (t < 0 || 3 * t > 123) return false;
  const i128 d_limit = i128{1} << (124 - 3 * t);
  const i128 n_limit = i128{1} << (124 - 2 * t);
  if (denominator <= 0 || denominator >= d_limit) return false;
  for (const auto coordinate : numerator)
    if (coordinate <= -n_limit || coordinate >= n_limit) return false;
  return true;
}

// Plus grand exposant t couvert, -1 si aucun ; proposition par longueurs, decision par le certificat exact.
inline constexpr int orientation_domain_i128(i128 denominator, const std::array<i128, 3>& numerator) noexcept {
  if (denominator <= 0) return -1;
  int numerator_bits = 0;
  for (const auto coordinate : numerator)
    numerator_bits = magnitude_bits(coordinate) > numerator_bits ? magnitude_bits(coordinate) : numerator_bits;
  int t = (124 - magnitude_bits(denominator)) / 3;
  t = (124 - numerator_bits) / 2 < t ? (124 - numerator_bits) / 2 : t;
  t = t > 41 ? 41 : t;
  while (t >= 0 && !orientation_certificate_i128(denominator, numerator, t)) --t;
  return t < 0 ? -1 : t;
}

// Certificat GLOBAL de la v11 pour normale de trois Point du profil : t = B.
inline constexpr bool global_orientation_i128(i128 denominator, const std::array<i128, 3>& numerator) noexcept {
  static_assert(124 - 3 * kCoordBits > 0 && 124 - 2 * kCoordBits < 127);
  return orientation_certificate_i128(denominator, numerator, kCoordBits);
}

}  // namespace mhgp12::num::detail
