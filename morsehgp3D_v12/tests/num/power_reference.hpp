// Reference de test : la formule anterieure reste entierement large, sans selection native par arite.
// Fraction juge separement le centre et la puissance geometrique ; cette voie compare aussi les executions.
#pragma once

#include <array>
#include <stdexcept>

#include "num/num.hpp"

namespace num_test {

template <class Geometry>
mhgp12::num::Wide<4> wide_power(const Geometry& sphere, mhgp12::num::Point point) {
  using namespace mhgp12;
  using namespace mhgp12::num;
  // Toujours 256 bits, quel que soit le stockage des coefficients (i128 aux profils 21 et 24, Wide au profil 32) :
  // 6B+8 <= 200 bits au profil 32.
  const auto query = point.coordinates(), anchor = sphere.anchor().coordinates();
  std::array<i64, 3> difference{};
  i128 norm = 0;
  for (int axis = 0; axis < 3; ++axis) {
    difference[axis] = i64{query[axis]} - anchor[axis];
    norm += i128{difference[axis]} * difference[axis];
  }
  Wide<4> result;
  if (!multiply_into(to_wide(sphere.denominator()), to_wide(norm), result))
    throw std::runtime_error("reference power : depassement de 256 bits");
  for (int axis = 0; axis < 3; ++axis) {
    Wide<4> term, next;
    if (!multiply_into(to_wide(sphere.numerator()[axis]), to_wide(-2 * i128{difference[axis]}), term) ||
        !add(result, term, next)) throw std::runtime_error("reference power : depassement de 256 bits");
    result = next;
  }
  return result;
}

template <class T>
bool equals_wide(const T& value, const mhgp12::num::Wide<4>& expected) {
  mhgp12::num::Wide<4> widened;
  return mhgp12::num::resize(mhgp12::num::to_wide(value), widened) &&
         mhgp12::num::compare(widened, expected) == 0;
}

}  // namespace num_test
