#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <array>
#include <iostream>

int main() {
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::ExactLevel;
  using morsehgp3d::exact::ExactRational;
  using morsehgp3d::exact::ExactRational3;

  const ExactLevel level{BigInt{6}, BigInt{8}};
  const ExactRational3 point{std::array<ExactRational, 3>{
      ExactRational{BigInt{1}, BigInt{2}},
      ExactRational{BigInt{-1}, BigInt{3}},
      ExactRational{BigInt{0}}}};
  const ExactRational decimal = ExactRational::from_binary64(0.1);

  std::cout << level.canonical_json() << '\n';
  std::cout << point.canonical_json() << '\n';
  std::cout << decimal.canonical_key() << '\n';
  return 0;
}
