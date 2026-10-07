// Valeurs de test independantes du stockage du profil : coefficients en i128 aux profils 21 et 24, en Wide au profil
// 32 (num/budgets.hpp, DomainBudget). Aucune selection de voie : seulement des conversions controlees et des ordres.
#pragma once

#include <stdexcept>

#include "num/num.hpp"

namespace profile_test {
using namespace mhgp12;
using namespace mhgp12::num;

template <class T>
Wide<8> wide8(const T& value) {
  Wide<8> out;
  if (!resize(to_wide(value), out)) throw std::runtime_error("valeur de test au-dela de 512 bits");
  return out;
}
template <class T>
int sign_of(const T& value) { return to_wide(value).sign(); }
template <class A, class B>
bool same_value(const A& a, const B& b) { return compare(wide8(a), wide8(b)) == 0; }
inline CenterInt center(i128 value) {
  const auto out = narrow<DomainBudget::center_numerator>(to_wide(value));
  if (!out) throw std::runtime_error("numerateur de test hors stockage");
  return *out;
}
inline CenterDen den(i128 value) {
  const auto out = narrow<DomainBudget::center_denominator>(to_wide(value));
  if (!out) throw std::runtime_error("denominateur de test hors stockage");
  return *out;
}
// a D + N, la coordonnee absolue d'un centre multipliee par D (juge de test, jamais une voie du produit).
template <class Ball>
Wide<8> absolute_center(const Ball& ball, int axis) {
  Wide<8> product, out;
  if (!multiply_into(to_wide(i128{ball.anchor().coordinates()[axis]}), wide8(ball.denominator()), product) ||
      !add(product, wide8(ball.numerator()[axis]), out)) throw std::runtime_error("centre absolu de test");
  return out;
}
// Texte decimal d'un entier signe jusqu'a 128 bits.
inline std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 magnitude = negative ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value);
  std::string digits;
  do {
    digits.insert(digits.begin(), static_cast<char>('0' + static_cast<int>(magnitude % 10)));
    magnitude /= 10;
  } while (magnitude != 0);
  return negative ? "-" + digits : digits;
}
// Tailles attendues sur les ABI natives de la matrice G4 (aucune promesse de serialisation).
inline constexpr std::size_t kSphereBytes = kCoordBits == 21 ? 144 : kCoordBits == 24 ? 160 : 232;
inline constexpr std::size_t kQ4CandidateBytes = kCoordBits <= 24 ? 80 : 144;

}  // namespace profile_test
