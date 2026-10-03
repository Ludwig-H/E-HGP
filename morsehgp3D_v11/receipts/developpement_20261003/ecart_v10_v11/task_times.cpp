// Diagnostic hors depot : durees par tache du catalogue une passe (W donne), sortie JSON par ligne.
#include <iostream>
#include "whole_input.hpp"
#include "sched/sched.hpp"
#include "catalogue/catalogue.hpp"
using namespace mhgp11;
using namespace mhgp11::bench;
int main(int argc, char** argv) {
  if (argc != 7) return 2;
  MemoryBudget budget(u64{8} << 30);
  auto input = read_input(argv[1], argv[2], budget);
  if (!input.ok()) return 3;
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return 3;
  const u32 workers = static_cast<u32>(std::stoul(argv[3]));
  auto pool = sched::make_pool({workers});
  CatalogueParams params;
  params.kmax = 5; params.leaf_size = static_cast<u32>(std::stoul(argv[4])); params.max_leaf = 256;
  params.cache_center_lines = true; params.indirect_sort = true; params.adaptive_frontier = std::stoul(argv[5]) != 0;
  params.parallel_assembly = true; params.single_pass = true; params.pair_graph = std::stoul(argv[6]) != 0;
  CatalogueTimings timings; CatalogueDiagnostics diagnostics;
  Stopwatch clock;
  auto cat = build_catalogue(cloud.value(), params, budget, *pool.value(), &timings, &diagnostics);
  const u64 wall = clock.nanoseconds();
  if (!cat.ok()) { std::cout << "fail\n"; return 1; }
  std::cout << "{\"wall_ns\":" << wall << ",\"single_pass_ns\":" << timings.single_pass_ns
            << ",\"prefix_ns\":" << timings.prefix_ns << ",\"sort_ns\":" << timings.sort_ns
            << ",\"tasks\":" << timings.tasks << ",\"single_task_sum_ns\":" << timings.single_task_sum_ns
            << ",\"single_task_max_ns\":" << timings.single_task_max_ns << ",\"balls\":" << cat.value().balls() << "}\n";
  std::cout << "{\"rounds\":" << diagnostics.planning().rounds << ",\"plan_nodes\":" << diagnostics.planning().plan_nodes << ",\"empty\":" << diagnostics.planning().empty_leaves << ",\"priority_tests\":" << diagnostics.planning().priority_tests << "}\n";
  for (const auto& t : diagnostics.tasks())
    std::cout << t.single_pass_ns << ' ' << t.count << ' ' << t.depth << '\n';
  return 0;
}
