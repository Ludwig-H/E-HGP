// Pas de tableau geant : le predicat monotone virtuel possede un rang connu.
// Le plafond de visites est une obligation du test, jamais un quota moteur.
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "tower/rank_search.hpp"

using namespace mhgp10;

namespace {
u64 checks = 0;
int failures = 0;

void test(u32 size, u32 expected) {
  unsigned visits = 0;
  auto predicate = [&](u64 i) {
    if (++visits > 40 || i >= size) throw std::runtime_error("visite hors domaine ou recherche non convergente");
    return i < expected;
  };
  u32 got = 0;
  try {
    got = rank_search::at_most(size,
                             [&](u32 i) { return predicate(u64{i} * rank_search::kStep); },
                             [&](u32 i) { return predicate(i); });
    if (got != expected) {
      std::printf("ECHEC taille=%u attendu=%u obtenu=%u\n", size, expected, got);
      ++failures;
    }
  } catch (const std::exception& ex) {
    std::printf("ECHEC taille=%u attendu=%u %s\n", size, expected, ex.what());
    ++failures;
  }
  ++checks;
}
}  // namespace

int main() {
  // Tous les rangs : petits tableaux, frontieres de blocs et blocs incomplets.
  for (u32 size = 0; size <= 260; ++size)
    for (u32 expected = 0; expected <= size; ++expected) test(size, expected);

  const std::vector<u32> sizes = {1000000, 0x7fffffbfu, 0x7fffffffu, 0x80000000u, 0x80000001u,
                                 0x80000040u, 0xffffff80u, 0xffffffbfu, 0xffffffc0u,
                                 0xffffffc1u, 0xfffffffeu, 0xffffffffu};
  for (u32 size : sizes) {
    std::vector<u32> cuts = {0, 1, 63, 64, 65, size / 2, size - 65, size - 64,
                            size - 63, size - 2, size - 1, size};
    for (u32 delta = 0; delta < 128; ++delta) cuts.push_back(size - delta);
    for (u32 cut : cuts) test(size, cut);
  }

  // Juge independant pour des niveaux REPETES : std::upper_bound, pas la
  // formule « index < rang attendu » des tableaux virtuels precedents.
  std::vector<u64> levels;
  for (u32 i = 0; i < 513; ++i) levels.push_back(i / 3 + 1);
  for (u64 e = 0; e <= 175; ++e) {
    const u32 expected = static_cast<u32>(std::upper_bound(levels.begin(), levels.end(), e) - levels.begin());
    const u32 got = rank_search::at_most(static_cast<u32>(levels.size()),
                                       [&](u32 i) { return levels.at(u64{i} * rank_search::kStep) <= e; },
                                       [&](u32 i) { return levels.at(i) <= e; });
    if (got != expected) ++failures;
    ++checks;
  }
  std::printf("rank_search_checks %llu\n", static_cast<unsigned long long>(checks));
  if (checks < 35000) return 3;
  if (failures != 0) return 1;
  std::puts("rank_search_ok");
  return 0;
}
