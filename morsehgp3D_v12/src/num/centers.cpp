// Ordre lexicographique exact des centres globaux ; aucune positivite ni reduction PGCD supposee.
#include "num/geometry.hpp"

namespace mhgp12::num {

int compare_centers(const Sphere& a, const Sphere& b) noexcept {
  static_assert(Budget::global_center_numerator <= 127);
  static_assert(Budget::center_comparison <= 256);
  const auto anchor_a = a.anchor(), anchor_b = b.anchor();
  const i128 da = a.denominator(), db = b.denominator();
  for (u32 axis = 0; axis < 3; ++axis) {
    // |a_i D|<2^(5B+5), |N_i|<2^(5B+5), donc |a_i D+N_i|<2^(5B+6)<=2^126.
    const i128 na = i128{anchor_a.coordinates()[axis]} * da + a.numerator()[axis];
    const i128 nb = i128{anchor_b.coordinates()[axis]} * db + b.numerator()[axis];
    // D>0. Chaque produit croise <2^(9B+11)<=2^227 ; pas de soustraction des deux produits.
    const int order = compare(multiply(to_wide(na), to_wide(db)), multiply(to_wide(nb), to_wide(da)));
    if (order != 0) return order;
  }
  return 0;
}

}  // namespace mhgp12::num
