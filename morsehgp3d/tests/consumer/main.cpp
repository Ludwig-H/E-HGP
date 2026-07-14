#include "morsehgp3d/exact/level.hpp"

#include <iostream>

int main() {
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::ExactLevel;

  const ExactLevel level{BigInt{2}, BigInt{8}};
  if (level.canonical_key() != "1/4") {
    std::cerr << "installed ExactLevel did not preserve canonical semantics\n";
    return 1;
  }
  return 0;
}
