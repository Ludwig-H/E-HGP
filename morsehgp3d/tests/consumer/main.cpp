#include "morsehgp3d/morsehgp3d.hpp"

#include <iostream>

int main() {
  using morsehgp3d::api::DensityLevel;
  using morsehgp3d::api::PositiveRationalExponent;
  using morsehgp3d::api::compare_density_levels_exact;
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::ExactLevel;

  const DensityLevel order_one{1U, ExactLevel{BigInt{1}, BigInt{1}}};
  const DensityLevel order_two{2U, ExactLevel{BigInt{2}, BigInt{1}}};
  const PositiveRationalExponent exponent{2U, 1U};
  if (compare_density_levels_exact(order_one, order_two, exponent) != 0) {
    std::cerr << "installed public density comparison changed semantics\n";
    return 1;
  }
  return 0;
}
