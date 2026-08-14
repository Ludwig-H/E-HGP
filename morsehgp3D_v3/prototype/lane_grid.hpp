// MorseHGP3D v3 — LA GRILLE UNIFORME PARTAGEE DES SONDES DE SOURCE.
//
// Elle n'est pas une structure de production : ni Morton, ni octree, ni index
// hierarchique. C'est l'accelerateur de voisinage des sondes de mesure, extrait
// ici parce que deux d'entre elles l'emploient et qu'une copie divergerait.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace lanegrid {

using mhgp::i64;

struct Pt { i64 c[3] = {0, 0, 0}; };

inline i64 d2p(const Pt& a, const Pt& b) {
  const i64 x = a.c[0] - b.c[0], y = a.c[1] - b.c[1], z = a.c[2] - b.c[2];
  return x * x + y * y + z * z;
}

// ---------------------------------------------------------------------------
// GRILLE UNIFORME A OFFSETS TRIES : une requete qui abandonne tot ne paie que
// les premieres couronnes.
// ---------------------------------------------------------------------------
struct Grid {
  i64 cell = 1;
  i64 lo[3] = {0, 0, 0}, dim[3] = {1, 1, 1};
  std::vector<int> start, item;
  std::vector<std::array<int, 3>> offs;
  std::vector<i64> offd2;

  void build(const std::vector<Pt>& pts, i64 cell_size) {
    cell = cell_size < 1 ? 1 : cell_size;
    i64 hi[3];
    for (int c = 0; c < 3; ++c) { lo[c] = INT64_MAX; hi[c] = INT64_MIN; }
    for (const Pt& p : pts)
      for (int c = 0; c < 3; ++c) {
        if (p.c[c] < lo[c]) lo[c] = p.c[c];
        if (p.c[c] > hi[c]) hi[c] = p.c[c];
      }
    for (int c = 0; c < 3; ++c) dim[c] = (hi[c] - lo[c]) / cell + 1;
    const size_t nc = (size_t)dim[0] * dim[1] * dim[2];
    std::vector<int> cnt(nc + 1, 0);
    std::vector<size_t> idx(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) { idx[i] = cell_of(pts[i]); ++cnt[idx[i] + 1]; }
    for (size_t i = 0; i < nc; ++i) cnt[i + 1] += cnt[i];
    start = cnt;
    item.assign(pts.size(), 0);
    std::vector<int> cur(start.begin(), start.end() - 1);
    for (size_t i = 0; i < pts.size(); ++i) item[cur[idx[i]]++] = (int)i;
  }
  size_t cell_of(const Pt& p) const {
    i64 c[3];
    for (int k = 0; k < 3; ++k) {
      c[k] = (p.c[k] - lo[k]) / cell;
      if (c[k] < 0) c[k] = 0;
      if (c[k] >= dim[k]) c[k] = dim[k] - 1;
    }
    return (size_t)((c[2] * dim[1] + c[1]) * dim[0] + c[0]);
  }
  void build_offsets(i64 rmax) {
    const int R = (int)(rmax / cell) + 2;
    for (int z = -R; z <= R; ++z)
      for (int y = -R; y <= R; ++y)
        for (int x = -R; x <= R; ++x) {
          const i64 ax = std::max(0, std::abs(x) - 1);
          const i64 ay = std::max(0, std::abs(y) - 1);
          const i64 az = std::max(0, std::abs(z) - 1);
          offs.push_back({x, y, z});
          offd2.push_back((ax * ax + ay * ay + az * az) * cell * cell);
        }
    std::vector<size_t> ord(offs.size());
    for (size_t i = 0; i < ord.size(); ++i) ord[i] = i;
    std::sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return offd2[a] < offd2[b]; });
    std::vector<std::array<int, 3>> o2(offs.size());
    std::vector<i64> d2(offs.size());
    for (size_t i = 0; i < ord.size(); ++i) { o2[i] = offs[ord[i]]; d2[i] = offd2[ord[i]]; }
    offs.swap(o2);
    offd2.swap(d2);
  }
  template <class F>
  void query(const i64 ctr[3], i64 r, F&& f) const {
    i64 c[3];
    for (int k = 0; k < 3; ++k) {
      const i64 v = ctr[k] - lo[k];
      c[k] = v >= 0 ? v / cell : -(((-v) + cell - 1) / cell);
    }
    const i64 r2 = r * r;
    for (size_t i = 0; i < offs.size(); ++i) {
      if (offd2[i] > r2) return;
      const i64 gx = c[0] + offs[i][0], gy = c[1] + offs[i][1], gz = c[2] + offs[i][2];
      if (gx < 0 || gy < 0 || gz < 0 || gx >= dim[0] || gy >= dim[1] || gz >= dim[2]) continue;
      const size_t cc = (size_t)((gz * dim[1] + gy) * dim[0] + gx);
      for (int t = start[cc]; t < start[cc + 1]; ++t)
        if (!f(item[t])) return;
    }
  }
};


}  // namespace lanegrid
}  // namespace mhgp3v
