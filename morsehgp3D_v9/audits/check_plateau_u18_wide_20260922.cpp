// Audit-only u18 regression: a signed normal·(rational centre - vertex) term
// exceeds INT128_MAX although the exact coplanarity sum is zero.
// Build, from repo root, against the pinned v9 source under review:
//   clang++ -std=c++20 -O1 -g -fsanitize=address,undefined \
//     -fno-sanitize-recover=all -I morsehgp3D_v9/src \
//     morsehgp3D_v9/audits/check_plateau_u18_wide_20260922.cpp -o <binary>
// The source is a diagnostic gate, not part of the v9 product/CMake.
#include <cstdio>

#include "tower/forest/plateau.hpp"
#include "tower/lanes/q3.hpp"

int main() {
  using namespace mhgp9::tower;
  const P3 a{1, 3, 5}, b{262136, 262132, 13}, x{262126, 19, 262120};
  const auto form = q3_form(a, b, x);
  if (form.g <= 0) return 1;  // Strictly acute nondegenerate q3 support.
  const BallKey key = q3_ball_key(form);
  const BallRat c = ball_center(key);
  const P3 n = p3_cross(p3_sub(b, a), p3_sub(x, a));
  const i128 v[3] = {c.cnum[0] - c.cden * a.x,
                     c.cnum[1] - c.cden * a.y,
                     c.cnum[2] - c.cden * a.z};
  const auto first = plateau_detail::s192_mul(n.x, v[0]);
  // This positive term is in [2^127, 2^128), so old signed i128 code
  // overflowed before the three coplanarity terms could cancel to zero.
  if (first.neg || first.mag.w[2] != 0 || first.mag.w[1] < (u64{1} << 63)) return 2;
  if (plateau_detail::s192_sign(plateau_detail::dot3_s192(n.x, n.y, n.z, v)) != 0)
    return 3;
  if (!plateau_detail::triangle_closed(c, a, b, x)) return 4;
  std::puts("PASS plateau_u18_wide");
  return 0;
}
