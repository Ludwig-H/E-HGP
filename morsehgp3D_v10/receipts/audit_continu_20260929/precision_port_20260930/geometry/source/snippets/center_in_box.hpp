#pragma once
#include "arith/geometry.hpp"
namespace mhgp10::micro_owner {
using geom::P3;
constexpr int kT = 6;
struct Box { i64 lo[3], hi[3]; };
inline bool center_in_box(const P3& a, const geom::Center& c, const Box& Q) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const i128 v = (i128(av[i]) * c.D + c.N[i]) * (i128(1) << kT);
    if (v < i128(Q.lo[i]) * c.D || v >= i128(Q.hi[i]) * c.D) return false;
  }
  return true;
}

}  // namespace mhgp10::micro_owner
