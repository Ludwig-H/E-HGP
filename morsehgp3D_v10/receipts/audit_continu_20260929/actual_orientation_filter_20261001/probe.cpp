#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>

#include "tower/orient_filter.hpp"

using namespace mhgp10;
using geom::P3;

int main() {
  constexpr std::array<P3, 4> vectors{{{202317, -171083, 81913}, {261103, 119999, -134873},
                                     {-130177, 80171, 224033}, {157339, -209999, 125003}}};
  constexpr P3 C{1048576, 1048576, 1048576};
  constexpr std::array<int, 4> modes{{FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}};
  const int original = std::fegetround();
  unsigned zero_total = 0, other_total = 0, wrong = 0, nearest_wrong = 0;
  std::printf("{\"variant\":\"%s\",\"rows\":[", ORIENT_MUTANT ? "bound_2pow_minus80" : "baseline");
  bool first = true;
  for (unsigned mi = 0; mi < modes.size(); ++mi) {
    if (std::fesetround(modes[mi]) != 0) return 2;
    unsigned mode_wrong = 0, mode_zero = 0, mode_other = 0;
    for (unsigned vi = 0; vi < vectors.size(); ++vi) {
      const P3 u = vectors[vi], v{u.z, u.x, -u.y}, t{-u.y, -u.z, u.x};
      const std::array<P3, 4> pts{{{C.x + u.x, C.y + u.y, C.z + u.z},
                                 {C.x - u.x, C.y - u.y, C.z - u.z},
                                 {C.x + v.x, C.y + v.y, C.z + v.z},
                                 {C.x + t.x, C.y + t.y, C.z + t.z}}};
      for (const P3& z : pts)
        if (z.x < 0 || z.y < 0 || z.z < 0 || z.x >= (i64{1} << 21) ||
            z.y >= (i64{1} << 21) || z.z >= (i64{1} << 21)) return 2;
      geom::Center ctr{};
      if (!geom::center4(pts[0], pts[1], pts[2], pts[3], ctr)) return 2;
      const P3 offset = geom::sub(C, pts[0]);
      if (ctr.D <= 0 || ctr.N[0] != ctr.D * offset.x || ctr.N[1] != ctr.D * offset.y ||
          ctr.N[2] != ctr.D * offset.z) return 2;
      for (unsigned f = 0; f < 4; ++f) {
        const P3 &a = pts[(f + 1) % 4], &b = pts[(f + 2) % 4], &c = pts[(f + 3) % 4];
        const P3 e1 = geom::sub(b, a), e2 = geom::sub(c, a);
        const i128 w[3] = {i128(e1.y) * e2.z - i128(e1.z) * e2.y,
                          i128(e1.z) * e2.x - i128(e1.x) * e2.z,
                          i128(e1.x) * e2.y - i128(e1.y) * e2.x};
        const i128 dot = w[0] * (C.x - a.x) + w[1] * (C.y - a.y) + w[2] * (C.z - a.z);
        // Every relative coordinate is < 2^19. This exact geometric dot is < 6*2^57;
        // no multiplication by ctr.D is needed for its sign, since ctr.D > 0.
        const int exact = dot < 0 ? -1 : (dot > 0 ? 1 : 0);
        const int observed = orient_filter::semi_static(a, b, c, pts[0], ctr);
        const bool bad = observed != 0 && observed != exact;
        wrong += bad;
        mode_wrong += bad;
        nearest_wrong += mi == 0 && bad;
        if (exact == 0) { ++zero_total; ++mode_zero; }
        else { ++other_total; ++mode_other; }
        if (!first) std::printf(",");
        first = false;
        std::printf("{\"rounding_index\":%u,\"fixture\":%u,\"face\":%u,\"exact\":%d,\"filter\":%d,\"wrong\":%s}",
                    mi, vi, f, exact, observed, bad ? "true" : "false");
      }
    }
    if (mode_zero != 8 || mode_other != 8 || (!ORIENT_MUTANT && mode_wrong != 0)) return 1;
  }
  if (std::fesetround(original) != 0) return 2;
  const bool ok = zero_total == 32 && other_total == 32 &&
                  (ORIENT_MUTANT ? nearest_wrong > 0 : wrong == 0);
  std::printf("],\"zero_faces\":%u,\"other_faces\":%u,\"wrong\":%u,\"nearest_wrong\":%u,\"expected_observation\":%s}\n",
              zero_total, other_total, wrong, nearest_wrong, ok ? "true" : "false");
  return ok ? 0 : 1;
}
