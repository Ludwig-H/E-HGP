// Recherche de rang a deux etages, sans addition ni produit debordant en u32.
// Les callbacks testent « niveau[index] <= seuil » dans des suites croissantes.
// Pure : aucune allocation ; les tests couvrent jusqu'a 2^32-1 niveaux virtuels.
#pragma once

#include <algorithm>

#include "core/types.hpp"

namespace mhgp10::rank_search {

inline constexpr u32 kStep = 64;

template <class SampleAtMost, class LevelAtMost>
u32 at_most(u32 size, SampleAtMost sample_at_most, LevelAtMost level_at_most) {
  const u32 samples = static_cast<u32>((u64{size} + kStep - 1) / kStep);
  u32 lo = 0, hi = samples;
  while (lo < hi) {
    const u32 mid = lo + (hi - lo) / 2;
    if (sample_at_most(mid)) lo = mid + 1;
    else hi = mid;
  }
  if (lo == 0) return 0;

  // lo*kStep peut valoir 2^32 meme lorsque size < kNone. Elargir AVANT
  // multiplication, puis borner, et seulement ensuite convertir en u32.
  u32 a = static_cast<u32>((u64{lo} - 1) * kStep + 1);
  u32 b = static_cast<u32>(std::min(u64{lo} * kStep, u64{size}));
  while (a < b) {
    const u32 mid = a + (b - a) / 2;
    if (level_at_most(mid)) a = mid + 1;
    else b = mid;
  }
  return a;
}

}  // namespace mhgp10::rank_search
