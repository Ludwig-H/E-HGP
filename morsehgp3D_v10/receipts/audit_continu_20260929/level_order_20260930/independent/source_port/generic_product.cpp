#include "arith/wide.hpp"
int main() {
  const auto a = mhgp10::arith::Wide<5>::from_u128(1);
  const auto b = mhgp10::arith::Wide<4>::from_u128(1);
  const auto product = mhgp10::arith::mul(a,b);
  return product.is_zero() ? 0 : 1;
}
