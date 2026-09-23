// Audit-only regression: one prover reused across two different clouds.
// Build in a v9 checkout with a libmhgp9_gen.a rebuilt from the SAME commit:
//   g++ -std=c++20 -O2 -I morsehgp3D_v9/src -I morsehgp3D_v9/src/gen \
//     morsehgp3D_v9/audits/check_q34_dead_owner_aba_20260923.cpp \
//     morsehgp3D_v9/src/gen/lanes/q34_dead_lanes.cpp \
//     build/v9-dev/libmhgp9_gen.a -pthread -o /tmp/mhgp9_q34_dead_owner_aba
// Exit 1: false rejection, 0: agreement, 77: allocator did not reuse address.

#include "gen/lanes/q34_dead_lanes.hpp"
#include "gen/pipeline/prepared_cloud.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace mhgp9::gen;
  const std::vector<Point3> old_points{{0, 0, 0}, {10, 0, 0}, {5, 0, 0}};
  const std::vector<Point3> new_points{{0, 0, 0}, {10, 0, 0}, {5, 10, 0}};
  const auto old_cloud = prepare_cloud(old_points);
  const auto new_cloud = prepare_cloud(new_points);
  Q34DeadLaneProver reused;
  std::uintptr_t old_address = 0;
  {
    auto index = make_q2_cloud_index(old_cloud);
    auto cover = Q34EdgeCover::make(index, {0, 1});
    old_address = reinterpret_cast<std::uintptr_t>(index.get());
    Q34DeadLaneWork work{};
    reused.load(*cover, work);
    if (reused.prove(2, 2, work) != 2 || cover->site_count() != 3) return 2;
  }

  for (unsigned attempt = 0; attempt < 128; ++attempt) {
    auto index = make_q2_cloud_index(new_cloud);
    if (reinterpret_cast<std::uintptr_t>(index.get()) != old_address) continue;
    auto cover = Q34EdgeCover::make(index, {0, 1});
    Q34DeadLaneWork reused_work{}, fresh_work{};
    reused.load(*cover, reused_work);
    const auto reused_mask = reused.prove(2, 2, reused_work);
    Q34DeadLaneProver fresh;
    fresh.load(*cover, fresh_work);
    const auto fresh_mask = fresh.prove(2, 2, fresh_work);
    std::cout << "same_index_address=true reused=" << unsigned(reused_mask)
              << " fresh=" << unsigned(fresh_mask)
              << " cover_sites=" << cover->site_count() << '\n';
    return reused_mask == fresh_mask ? 0 : 1;
  }
  std::cout << "same_index_address=false; allocation reuse not exercised\n";
  return 77;
}
