// Reference de test : la formule anterieure reste entierement large, sans selection native par arite.
// Fraction juge separement le centre et la puissance geometrique ; cette voie compare aussi les executions.
#pragma once

#include <array>
#include <stdexcept>

#include "num/num.hpp"

namespace num_test {

template <class Geometry>
mhgp11::num::Wide<4> wide_power(const Geometry& sphere, mhgp11::num::Point point) {
  using namespace mhgp11;
  using namespace mhgp11::num;
  const auto query = point.coordinates(), anchor = sphere.anchor().coordinates();
  std::array<i64, 3> difference{};
  i128 norm = 0;
  for (int axis = 0; axis < 3; ++axis) {
    difference[axis] = i64{query[axis]} - anchor[axis];
    norm += i128{difference[axis]} * difference[axis];
  }
  auto result = multiply(to_wide(sphere.denominator()), to_wide(norm));
  for (int axis = 0; axis < 3; ++axis) {
    const auto term = multiply(to_wide(sphere.numerator()[axis]), to_wide(-2 * i128{difference[axis]}));
    Wide<4> next;
    if (!add(result, term, next)) throw std::runtime_error("reference power : depassement de 256 bits");
    result = next;
  }
  return result;
}

template <class T>
bool equals_wide(const T& value, const mhgp11::num::Wide<4>& expected) {
  mhgp11::num::Wide<4> widened;
  return mhgp11::num::resize(mhgp11::num::to_wide(value), widened) &&
         mhgp11::num::compare(widened, expected) == 0;
}

}  // namespace num_test
