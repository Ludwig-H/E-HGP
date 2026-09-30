#include <array>
#include <cstdint>
#include <cstdio>
#include "arith/geometry.hpp"

using namespace mhgp10;
// Copies des seules dispositions des structures internes lues ; aucune allocation produit.
struct RecLayout {
  std::array<u32, 4> sup;
  u8 q, flags;
  u32 p, u, n_i;
  u64 pop_begin;
  u32 pop_len;
  geom::Level level;
};
struct RefLayout { double approx; std::array<u32, 4> sup; u32 local, rec; };
struct BallInfoLayout { u32 base; u16 m; u8 lo, hi, p, q, ext, pad; };
struct NodeLayout { double bmin[3], bmax[3]; u32 lo, hi, left, right; };

int main() {
  std::printf("sizeof Level=%zu Rec=%zu Ref=%zu BallInfo=%zu SiteTreeNode=%zu P3=%zu\n",
              sizeof(geom::Level), sizeof(RecLayout), sizeof(RefLayout), sizeof(BallInfoLayout), sizeof(NodeLayout), sizeof(geom::P3));
  // Second bloc de RankIndex::at_most, toutes ses valeurs de niveau <= e.
  // L=2^31+64 et lo=L/64 : exactement le dernier bloc de 64 niveaux.
  const u32 levels = (u32{1} << 31) + 64;
  const u32 lo = levels / 64;
  u32 a = (lo - 1) * 64 + 1, b = lo * 64;
  const u32 initial_a = a, initial_b = b, first_mid = (a + b) / 2;
  unsigned iterations = 0, out_of_interval = 0;
  while (a < b && iterations < 200) {
    const u32 mid = (a + b) / 2;  // expression de tower.cpp:1022, arithmetique u32
    out_of_interval += mid < a || mid >= b;
    a = mid + 1;                // niveau(mid) <= e
    ++iterations;
  }
  std::printf("RankIndex levels=%u initial_a=%u initial_b=%u first_mid=%u out_of_interval=%u iterations=%u terminated=%d\n",
              levels, initial_a, initial_b, first_mid, out_of_interval, iterations, a >= b);
  // Correction locale proposee, meme predicat monotone et meme intervalle.
  a = initial_a;
  iterations = 0;
  while (a < b && iterations < 200) {
    const u32 mid = a + (b - a) / 2;
    a = mid + 1;
    ++iterations;
  }
  std::printf("RankIndex safe_mid iterations=%u returned=%u expected=%u\n", iterations, a, levels);
}
