// Bornes conservatrices pour |difference de coordonnees| < M=2^B (et non 2^(B+1)).
// R2 fixait les types au profil u18 ; ici chaque type suit ces expressions, aux profils 21 et 24 (port de la v11,
// commit ac081a06f, sans le profil 18 abandonne par la decision D6 de la v12).
#pragma once

#include "num/integer.hpp"

namespace mhgp12::num {

template <int B>
struct Budgets {
  static_assert(B == 21 || B == 24, "num : profil 21 ou 24 bits");
  static constexpr int difference = B;
  static constexpr int dot = 2 * B + 2;       // somme de trois produits : < 3 M^2
  static constexpr int cross = 2 * B + 1;     // difference de deux produits : < 2 M^2
  static constexpr int determinant = 3 * B + 3;  // six produits : < 6 M^3
  static constexpr int numerator3 = 5 * B + 5;   // (uu v-vv u) x (u x v) : < 24 M^5
  static constexpr int denominator3 = 4 * B + 5; // 2 |u x v|^2 : < 24 M^4
  static constexpr int numerator4 = 4 * B + 5;   // trois produits norm2*cross : < 18 M^4
  static constexpr int denominator4 = 3 * B + 4; // 2 det : < 12 M^3
  static constexpr int center_numerator = numerator3;
  static constexpr int center_denominator = denominator3;
  static constexpr int global_center_numerator = 5 * B + 6; // a_i D + N_i : < 2^(5B+6)
  static constexpr int center_comparison = 9 * B + 11; // deux produits globaux separes, D>0
  static constexpr int side = 6 * B + 8;         // D |z-a|^2 - 2 N.(z-a) : < 216 M^6
  static constexpr int center_orientation = 7 * B + 9; // cross.(N+D(a-p)) : < 288 M^7
  static constexpr int level_numerator = 8 * B + 12;   // q4: 3*(18 M^4)^2 < 1024 M^8 ; marge pour le meme type
  static constexpr int level_denominator = 6 * B + 8;  // q4: (12 M^3)^2 < 256 M^6
  static constexpr int level_comparison = level_numerator + level_denominator;
  static_assert(dot <= 63 && cross <= 63 && determinant <= 127, "num : produits bas degre natifs");
  static_assert(center_numerator <= 127 && center_denominator <= 127, "num : centres natifs exacts");
  static_assert(6 * B + 5 <= level_numerator && 4 * B + 6 <= level_denominator,
                "num : formes reduites q2/q3 couvertes par le budget de niveau");
};

using Budget = Budgets<kCoordBits>;
using DotInt = Int<Budget::dot>;
using CrossInt = Int<Budget::cross>;
using DeterminantInt = Int<Budget::determinant>;
using CenterInt = Int<Budget::center_numerator>;
using CenterDen = Int<Budget::center_denominator>;
using SideInt = Int<Budget::side>;
static_assert(Budgets<21>::center_numerator == 110 && Budgets<24>::center_numerator == 125,
              "num : bornes des deux profils");

}  // namespace mhgp12::num
