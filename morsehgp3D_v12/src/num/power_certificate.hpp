// Certificat de puissance lie a son domaine (CST-0201, docs/CONTRAT_NUMERIQUE.md, paragraphe 3). Pour des differences
// requete-ancre |v_j| < 2^t : D < 2^(123-2t) et |N_j| < 2^(124-t) bornent le terme quadratique D|v|^2 par 3*2^123 et
// chacun des trois termes lineaires 2|N_j v_j| par 2^125 ; la somme des magnitudes, < 15*2^123 < 2^127, borne chaque
// produit ET chaque somme partielle en i128. La preuve ne lit ni l'arite ni l'etendue du support : seulement t et les
// coefficients. Le certificat de la v11 (q3_global_power_i128) est le cas t = B, domaine de tous les Point du profil ;
// le recensement garde conserve t = s+2 : cette politique reste suffisante apres le resserrement du pave.
// Les bornes plus fines exploitant CertifiedBall se qualifient separement ; ce certificat generique reste inchange.
// Aucun coefficient public forgeable : les fabriques de num calculent le domaine une fois.
#pragma once
#include <array>
#include "num/budgets.hpp"

namespace mhgp12::num::detail {

inline constexpr bool power_certificate_i128(i128 denominator, const std::array<i128, 3>& numerator, int t) noexcept {
  if (t < 0 || 2 * t > 122) return false;
  const i128 d_limit = i128{1} << (123 - 2 * t);
  const i128 n_limit = i128{1} << (124 - t);
  if (denominator <= 0 || denominator >= d_limit) return false;
  for (const auto coordinate : numerator)
    if (coordinate <= -n_limit || coordinate >= n_limit) return false;
  return true;
}

// Magnitude en bits d'un i128 (0 pour zero), sans debordement sur le minimum.
inline constexpr int magnitude_bits(i128 value) noexcept {
  const u128 magnitude = value < 0 ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value);
  const u64 high = static_cast<u64>(magnitude >> 64);
  return high != 0 ? 128 - std::countl_zero(high) : 64 - std::countl_zero(static_cast<u64>(magnitude));
}

// Plus grand exposant t couvert par le certificat de puissance, -1 si aucun. Proposition par longueurs en bits,
// puis decision par le certificat exact lui-meme (la proposition ne decide rien).
inline constexpr int power_domain_i128(i128 denominator, const std::array<i128, 3>& numerator) noexcept {
  if (denominator <= 0) return -1;
  int numerator_bits = 0;
  for (const auto coordinate : numerator)
    numerator_bits = magnitude_bits(coordinate) > numerator_bits ? magnitude_bits(coordinate) : numerator_bits;
  int t = (123 - magnitude_bits(denominator)) / 2;
  t = 124 - numerator_bits < t ? 124 - numerator_bits : t;
  t = t > 61 ? 61 : t;
  while (t >= 0 && !power_certificate_i128(denominator, numerator, t)) --t;
  return t < 0 ? -1 : t;
}

// Certificat global de la v11 : domaine du profil, t = B. Suffisant pour TOUT Point du profil.
inline constexpr bool q3_global_power_i128(i128 denominator, const std::array<i128, 3>& numerator) noexcept {
  static_assert(123 - 2 * kCoordBits > 0 && 124 - kCoordBits < 127);
  return power_certificate_i128(denominator, numerator, kCoordBits);
}

}  // namespace mhgp12::num::detail
