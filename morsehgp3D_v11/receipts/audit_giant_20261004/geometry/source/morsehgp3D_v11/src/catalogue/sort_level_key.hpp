// Cles internes du tri : preparation F3 et decision F4, partagees sans option ni etat mutable.
#pragma once

#include <cmath>

#include "num/num.hpp"

namespace mhgp11::catalogue_detail {

// 64 bits de tete tronques : erreur relative <2^-63<=u=2^-52, puis conversion sous tout arrondi.
// ldexp est exact : les entiers Level ont <=204 bits. E=2 par entier, E=2+2+2=6 pour le quotient.
// Pour B18/21/24, Level::make certifie n>=0 et d>0, n<2^204 et d<2^152. Toute cle positive et
// tout intermediaire sont normaux et finis ; FTZ/DAZ ne changent donc aucune de ces valeurs.
inline constexpr int kKeyExponent = 6;
inline constexpr double kOrdered = 1.0 - 0x1p-40;
static_assert(2 * kKeyExponent + 1 <= 4096, "F4 : c=1-2^-40 couvre Ex+Ey+1<=4096");

template <int Words>
double magnitude_key(const num::Wide<Words>& value) noexcept {
  const int length = value.bit_length();
  if (length <= 64) return static_cast<double>(value.words[0]);
  const int shift = length - 64, word = shift / 64, bit = shift % 64;
  u64 top = value.words[word] >> bit;
  if (bit != 0) top |= value.words[word + 1] << (64 - bit);  // bit!=0 implique word+1<Words
  return std::ldexp(static_cast<double>(top), shift);
}

inline double level_key(const num::Level& level) noexcept {
  return magnitude_key(num::to_wide(level.numerator())) / magnitude_key(num::to_wide(level.denominator()));
}

// Entrees : cles issues de level_key. -1/+1 est un ordre CERTAIN des niveaux exacts ; zero demande
// le repli exact, y compris pour deux cles egales. Les arrondis de preparation/comparaison peuvent differer.
// F4 : c<=(1-u)^(Ex+Ey+1), le produit par c etant lui-meme arrondi. Zero est exact avant ce produit.
inline int level_key_order(double a, double b) noexcept {
  if (a == 0 || b == 0) return (a > b) - (a < b);
  if (a < kOrdered * b) return -1;
  if (b < kOrdered * a) return 1;
  return 0;
}

}  // namespace mhgp11::catalogue_detail
