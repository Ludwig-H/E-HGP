// Chrono de la meme etape dans la v11 gelee (microbanc MES-M5 de la v12, hors produit) : frontiere et passe unique du
// catalogue, c'est-a-dire le parcours des boites jusqu'aux feuilles mises en file, sur W fils.
//
// Configuration : celle de la voie GPU de la v11 (masque 868347 de mhgp11_full_bench : frontiere adaptative, passe
// unique, graphe de paires, cache J2, tri indirect, assemblage parallele, cache de blocs du budget), a une difference
// pres qui ne touche ni la frontiere ni la passe unique : les feuilles mises en file sont traitees par l'executeur hote
// (batch_leaves, bit 32768) au lieu du GPU (cuda_leaves, bit 65536), la bibliotheque v11 des microbancs etant construite
// sans CUDA. Les deux sous-etages chronometres (timings.prefix_ns, timings.single_pass_ns) precedent le lot de feuilles
// et ne dependent pas de son executeur. Passes successives dans le meme processus (regime chaud), nuage refait hors
// chrono a chaque passe comme mhgp11_full_bench. Le catalogue est construit en entier a chaque passe ; ses comptes
// (noeuds, feuilles, tests G1) sont publies pour le rattacher au vidage du parcours.
//
// En plus, le walk sequentiel gele (un fil, feuilles mises en file et jetees) est chronometre --walk-reps fois :
// travail total du parcours a un fil, indicatif.
//
// Usage : mhgp12_v11_traversal_timing <xyz.u32le> <ids.u32le> <K> <feuille> <fils> <passes> [--walk-reps R]
// Sortie standard : une ligne JSON. Codes : 0 conforme, 2 refus.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <new>
#include <string>
#include <vector>

#include "catalogue/internal.hpp"
#include "sched/sched.hpp"
#include "whole_input.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;

namespace {

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

struct DropQueue final : LeafQueue {
  u64 leaves = 0, sites = 0;
  Outcome push(std::span<const SiteIdx> leaf, const Box&) noexcept override {
    ++leaves;
    sites += leaf.size();
    return {};
  }
};

int fail_with(const Outcome& o, const char* stage) {
  std::cout << "{\"phase\":\"exit\",\"stage\":\"" << stage << "\",\"reason\":\"" << reason_name(o.reason) << "\"}\n";
  return 2;
}

int run(int argc, char** argv) {
  if (argc < 7) return 2;
  u64 kmax = 0, leaf = 0, workers = 0, passes = 0, walk_reps = 3;
  if (!bench::parse(argv[3], kmax) || !bench::parse(argv[4], leaf) || !bench::parse(argv[5], workers) ||
      !bench::parse(argv[6], passes) || workers < 1 || workers > sched::kMaxWorkers || passes < 1 || passes > 64)
    return 2;
  for (int i = 7; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--walk-reps" && i + 1 < argc) {
      if (!bench::parse(argv[++i], walk_reps) || walk_reps > 64) return 2;
    } else {
      return 2;
    }
  }
  MemoryBudget budget(u64{64} << 30, u64{4} << 30);  // cache de blocs de 4 Gio (bit 524288 de mhgp11_full_bench)
  auto input = bench::read_input(argv[1], argv[2], budget);
  if (!input.ok()) return fail_with(input.outcome(), "read_input");
  CatalogueParams params;
  params.kmax = static_cast<int>(kmax);
  params.leaf_size = static_cast<u32>(leaf);
  params.max_leaf = 256;
  params.cache_center_lines = true;
  params.indirect_sort = true;
  params.adaptive_frontier = true;
  params.parallel_assembly = true;
  params.single_pass = true;
  params.pair_graph = true;
  params.batch_leaves = true;
  if (const Outcome o = check_catalogue_params(params); !o.ok()) return fail_with(o, "params");
  auto pool = sched::make_pool({static_cast<u32>(workers)});
  if (!pool.ok()) return fail_with(pool.outcome(), "pool");
  std::vector<u64> prefix, single, sum, catalogue_ns;
  CatalogueLedger ledger{};
  u64 balls = 0, batch_jobs = 0;
  for (u64 pass = 0; pass < passes; ++pass) {
    auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                               input.value().ids.span(), CoordWidth(), budget);
    if (!cloud.ok()) return fail_with(cloud.outcome(), "prepare_cloud");
    CatalogueTimings t;
    const u64 t0 = now_ns();
    auto cat = build_catalogue(cloud.value(), params, budget, *pool.value(), &t);
    const u64 t1 = now_ns();
    if (!cat.ok()) return fail_with(cat.outcome(), "build_catalogue");
    prefix.push_back(t.prefix_ns);
    single.push_back(t.single_pass_ns);
    sum.push_back(t.prefix_ns + t.single_pass_ns);
    catalogue_ns.push_back(t1 - t0);
    ledger = cat.value().ledger();
    balls = cat.value().balls();
    batch_jobs = t.batch_jobs;
  }
  // Walk sequentiel gele (un fil), feuilles jetees.
  std::vector<u64> walk_ns;
  CatalogueLedger walk_ledger{};
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return fail_with(cloud.outcome(), "prepare_cloud");
  for (u64 r = 0; r < walk_reps; ++r) {
    Workspace space;
    if (const Outcome o = space.allocate(params.max_leaf, budget, params.cache_center_lines, params.pair_graph); !o.ok())
      return fail_with(o, "workspace");
    Collector collector;
    DropQueue queue;
    Run walker{cloud.value(), params, budget, space, collector, {}, nullptr, &queue};
    const u64 t0 = now_ns();
    if (const Outcome o = walk(walker); !o.ok()) return fail_with(o, "walk");
    walk_ns.push_back(now_ns() - t0);
    walk_ledger = walker.ledger;
  }
  const auto list = [](const std::vector<u64>& v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) s += (i ? "," : "") + std::to_string(v[i]);
    return s + "]";
  };
  std::cout << "{\"phase\":\"v11_traversal_timing\",\"kmax\":" << kmax << ",\"leaf_size\":" << leaf
            << ",\"workers\":" << workers << ",\"passes\":" << passes << ",\"sites\":" << cloud.value().sites()
            << ",\"config\":\"868347 sans cuda_leaves, avec batch_leaves (executeur hote du lot)\""
            << ",\"prefix_ns\":" << list(prefix) << ",\"single_pass_ns\":" << list(single)
            << ",\"traversal_ns\":" << list(sum) << ",\"catalogue_ns\":" << list(catalogue_ns)
            << ",\"catalogue_nodes\":" << ledger.nodes << ",\"catalogue_leaves\":" << ledger.leaves
            << ",\"catalogue_filter_tests\":" << ledger.filter_tests << ",\"catalogue_max_depth\":" << ledger.max_depth
            << ",\"balls\":" << balls << ",\"batch_jobs\":" << batch_jobs << ",\"walk_ns\":" << list(walk_ns)
            << ",\"walk_nodes\":" << walk_ledger.nodes << ",\"walk_leaves\":" << walk_ledger.leaves
            << ",\"walk_filter_tests\":" << walk_ledger.filter_tests << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const std::bad_alloc&) {
    std::cout << "{\"phase\":\"exit\",\"stage\":\"allocation\",\"reason\":\"memory_budget\"}\n";
    return 2;
  }
}
