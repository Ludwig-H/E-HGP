#include "lanes/q4_local.hpp"
#include "lanes/q4_window.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"

#include <array>
#include <iostream>

int main() {
  using namespace mhgp9::gen;
  const std::array<Point3, 5> points{{
      {0, 0, 2}, {4, 0, 2}, {2, 3, 2}, {2, 2, 4}, {2, 2, 0}}};
  auto cover = Q34EdgeCover::make(make_q2_cloud_index(prepare_cloud(points)),
                                  {0, 1});
  auto geometry = Q4LocalGeometry::make(cover, Q4CenterDomainMode::Disk);
  auto shallow = Q4ShallowSet::make(geometry, 3);
  Q4LocalOptions options;
  auto atlas = Q4LocalAtlas::make(cover, 3, options);

  const auto print = [](const char* lane) {
    return [lane](const Q34SeedCandidate& candidate) {
      std::cout << lane << " support=";
      for (unsigned i = 0; i < candidate.arity; ++i) {
        if (i) std::cout << ',';
        std::cout << candidate.support_ids[i];
      }
      std::cout << " depth=" << candidate.depth
                << " shell="
                << candidate.shell_first.size() + candidate.shell_second.size()
                << '\n';
    };
  };

  const auto window = run_q4_window_seed_candidates(shallow, 2, print("window"));
  const auto local = run_q4_local_seed_candidates(atlas, 2, print("local"));
  std::cout << "window seed_queries=" << window.sweep.seed_queries
            << " groups=" << window.sweep.family.groups
            << " emitted=" << window.sweep.emitted << '\n';
  std::cout << "local seed_queries=" << local.seed_queries
            << " groups=" << local.groups
            << " emitted=" << local.emitted << '\n';
  return window.sweep.seed_queries == 1 && window.sweep.family.groups == 2 &&
                 window.sweep.emitted == 2 && local.seed_queries == 1 &&
                 local.groups == 2 && local.emitted == 2
             ? 0
             : 1;
}
