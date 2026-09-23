// MorseHGP3D v9 — porte du tri parallele de la tour (parallel/pool.hpp).
//
// parallel_sort promet, pour un ordre STRICT et TOTAL, l'unique permutation
// triee quel que soit le nombre de fils : on la compare a std::sort sur des
// tableaux aleatoires (cles repetees, ordinaux uniques comme les requetes
// statiques), pour plusieurs tailles de part et d'autre du seuil parallele et
// plusieurs nombres de fils, dont des nombres impairs de tranches. Planchers :
// au moins un cas multi-ouvriers et un cas a nombre impair de tranches.
//
//   mhgp9_tower_parallel_sort_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
// Compile avec MHGP9_PARALLEL_SORT_MUTANT_UNSORTED_BUCKET, un seau reste non
// trie : la porte doit rendre 1 (mutant tue).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <utility>
#include <vector>

#include "../../src/tower/parallel/pool.hpp"

namespace {
struct Item {
  std::uint32_t key;
  std::uint64_t ordinal;
};
bool less(const Item& a, const Item& b) { return a.key != b.key ? a.key < b.key : a.ordinal < b.ordinal; }
bool same(const Item& a, const Item& b) { return a.key == b.key && a.ordinal == b.ordinal; }
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_tower_parallel_sort_gate --selftest\n");
    return 2;
  }
  std::uint64_t state = 0x9e3779b97f4a7c15ull;
  const auto next = [&] {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<std::uint32_t>(state >> 33);
  };
  std::size_t cases = 0, multi = 0, odd_runs = 0;
  for (const std::size_t n : {0ul, 1ul, 5ul, 4095ul, 8191ul, 8192ul, 8193ul, 12289ul, 50000ul, 200003ul})
    for (const std::uint32_t range : {1u, 7u, 1000u, 4000000000u})
      for (const int threads : {0, 1, 2, 3, 4, 5, 7, 8, 16, 48}) {
        std::vector<Item> input(n);
        for (std::size_t i = 0; i < n; ++i) input[i] = {range == 1 ? 0u : next() % range, i};
        for (std::size_t i = n; i > 1; --i) std::swap(input[i - 1], input[next() % i]);
        auto expected = input;
        std::sort(expected.begin(), expected.end(), less);
        auto actual = input;
        const std::size_t created = mhgp9::tower::parallel_sort(actual, threads, less);
        ++cases;
        if (!std::equal(actual.begin(), actual.end(), expected.begin(), expected.end(), same)) {
          std::printf("cause=parallel_sort.permutation n=%zu range=%u threads=%d\n", n, range, threads);
          return 1;
        }
        const std::size_t runs = mhgp9::tower::planned_workers(n / 4096, threads);
        if (created > 1) ++multi;
        if (runs > 1 && runs % 2 == 1) ++odd_runs;
      }
  std::printf("parallel_sort_gate cases=%zu multi_worker=%zu odd_runs=%zu\n", cases, multi, odd_runs);
  if (multi == 0 || odd_runs == 0) {
    std::printf("cause=floor.multi_worker_or_odd_runs\n");
    return 3;
  }
  return 0;
}
