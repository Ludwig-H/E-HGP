// Mini-gate différentiel ciblé : une vraie boule q4 positive à deux intérieurs.
// Compilation contre un build v9 déjà disponible, sans modifier le moteur :
//   c++ -std=c++20 -O2 -pthread \
//     -I build/v9-open-worktree/morsehgp3D_v9/src \
//     -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
//     morsehgp3D_v9/audits/check_q4_k5_interior_chain_20260922.cpp \
//     build/v9-open-worktree/build/v9/libmhgp9_chain.a \
//     build/v9-open-worktree/build/v9/libmhgp9_gen.a \
//     -o /tmp/check_q4_k5_interior_chain_20260922
// Ce gate ne prouve que ce petit cas, pas la complétude globale du générateur.
#include "chain/tower_chain.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {

using mhgp9::gen::Point3;

constexpr std::array<Point3, 6> points{{
    {3, 3, 3}, {3, 1, 1}, {1, 3, 1}, {1, 1, 3}, {2, 2, 2}, {2, 2, 3},
}};

// Sphere (x-2)^2+(y-2)^2+(z-2)^2=3, primitive power form.
const mhgp9::tower::BallKey target{1, {-4, -4, -4}, 9};

void check(unsigned k, bool reverse) {
  std::vector<Point3> input(points.begin(), points.end());
  if (reverse) std::reverse(input.begin(), input.end());
  mhgp9::ChainOptions options;
  options.kmax = k;
  options.separation_s = 8;
  options.workers = 1;
  options.run_tower = true;
  options.keep_catalogue = true;
  const auto result = mhgp9::run_tower_chain(input, options);
  if (result.status != mhgp9::ChainStatus::kComplete)
    throw std::runtime_error("public chain did not complete");
  if (result.orders.size() != k || result.tower.orders.size() != k ||
      result.tower_digest == 0)
    throw std::runtime_error("FULL tower was not materially constructed");
  if (result.catalogue.balls != result.catalogue_balls.size())
    throw std::runtime_error("public catalogue inventory disagrees with its rows");
  for (unsigned i = 0; i < k; ++i)
    if (result.orders[i].k != i + 1 || result.tower.orders[i].forest.order() != i + 1)
      throw std::runtime_error("FULL tower order index is inconsistent");

  unsigned target_rows = 0;
  for (const auto& row : result.catalogue_balls) {
    if (row.key != target) continue;
    ++target_rows;
    if (row.arity != 4 || row.n_interior != 2 || row.n_shell != 4)
      throw std::runtime_error("target q4 row has wrong qmin/interior/shell");
  }
  if (target_rows != (k == 5 ? 1U : 0U))
    throw std::runtime_error("target q4 ball missing or prematurely included");
  if (k == 5 && result.q4_emitted == 0)
    throw std::runtime_error("K5 q4 lane emitted nothing");

  std::printf("K=%u reverse=%u status=complete target_rows=%u catalogue=%llu "
              "q4_emitted=%llu digest=%llu\n", k, reverse ? 1U : 0U, target_rows,
      static_cast<unsigned long long>(result.catalogue.balls),
      static_cast<unsigned long long>(result.q4_emitted),
      static_cast<unsigned long long>(result.tower_digest));
}

}  // namespace

int main() {
  try {
    for (bool reverse : {false, true})
      for (unsigned k : {3U, 4U, 5U}) check(k, reverse);
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "FAIL: %s\n", e.what());
    return 1;
  }
}
