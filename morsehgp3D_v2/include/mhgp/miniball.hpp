// MorseHGP3D v2 — miniboule exacte d'un petit ensemble de points 3D.
//
// Le support minimal d'une miniboule en dimension 3 compte au plus 4 points
// (FAIT 12 du manuscrit). On enumere les supports par taille croissante ; le
// premier qui est bien centre et couvre tout l'ensemble est la miniboule.
#pragma once

#include <algorithm>

#include "mhgp/exact.hpp"
#include "mhgp/sphere.hpp"

namespace mhgp {

struct MiniballResult {
  Sphere sph{};
  i32 support[4] = {-1, -1, -1, -1};
  int n_support = 0;
  bool ok = false;
};

template <class PointArray>
MiniballResult miniball_of(const PointArray& pts, const i32* ids, int m) {
  MiniballResult best;
  for (int sz = 1; sz <= 4 && sz <= m; ++sz) {
    int idx[4];
    for (int i = 0; i < sz; ++i) idx[i] = i;
    while (true) {
      Sphere s{};
      bool built = false;
      const P3 a = pts[static_cast<size_t>(ids[idx[0]])];
      if (sz == 1) {
        s = sphere1(a);
        built = true;
      } else if (sz == 2) {
        s = sphere2(a, pts[static_cast<size_t>(ids[idx[1]])]);
        built = true;
      } else if (sz == 3) {
        const P3 b = pts[static_cast<size_t>(ids[idx[1]])];
        const P3 c = pts[static_cast<size_t>(ids[idx[2]])];
        built = sphere3(a, b, c, &s) && well_centered3(a, b, c);
      } else {
        const P3 b = pts[static_cast<size_t>(ids[idx[1]])];
        const P3 c = pts[static_cast<size_t>(ids[idx[2]])];
        const P3 d = pts[static_cast<size_t>(ids[idx[3]])];
        built = sphere4(a, b, c, d, &s) && well_centered4(s, a, b, c, d);
      }
      if (built) {
        bool covers = true;
        for (int t = 0; t < m; ++t) {
          if (sphere_side(s, pts[static_cast<size_t>(ids[t])]) > 0) {
            covers = false;
            break;
          }
        }
        if (covers) {
          best.sph = s;
          best.n_support = sz;
          for (int i = 0; i < sz; ++i) best.support[i] = ids[idx[i]];
          std::sort(best.support, best.support + sz);
          best.ok = true;
          return best;
        }
      }
      int i = sz - 1;
      while (i >= 0 && idx[i] == m - sz + i) --i;
      if (i < 0) break;
      ++idx[i];
      for (int j = i + 1; j < sz; ++j) idx[j] = idx[j - 1] + 1;
    }
  }
  return best;
}

}  // namespace mhgp
