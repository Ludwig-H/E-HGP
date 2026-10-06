// Placement des taches du pipeline (forest_placement.hpp) : listes de freres du noyau, coeurs, plan sur une
// topologie de type G4 (24 coeurs x 2 fils), affectation de chaque tache, refus (sans SMT, K > 5, trop petite
// machine), affinite reellement appliquee au fil puis rendue.
#include <array>
#include <string>
#include <vector>

#include "test.hpp"
#include "tower/forest_placement.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;

namespace {
std::vector<u32> members(const std::array<bool, kPlacementCpus>& mask) {
  std::vector<u32> out;
  for (u32 c = 0; c < kPlacementCpus; ++c)
    if (mask[c]) out.push_back(c);
  return out;
}

// Topologie : threads fils par coeur, coeur i = {i, i + cores, i + 2 cores...} ; allowed[c] faux retire le fil c.
struct Topology {
  std::vector<std::string> text;
  std::array<std::string_view, kPlacementCpus> lists{};
  Topology(u32 cores, u32 threads, const std::vector<u32>& removed = {}) : text(kPlacementCpus) {
    for (u32 core = 0; core < cores; ++core) {
      std::string list;
      for (u32 t = 0; t < threads; ++t) list += (t ? "," : "") + std::to_string(core + t * cores);
      for (u32 t = 0; t < threads; ++t) text[core + t * cores] = list;
    }
    for (u32 c : removed) text[c].clear();
    for (u32 c = 0; c < kPlacementCpus; ++c) lists[c] = text[c];
  }
};

#if defined(__linux__)
std::vector<u32> cpus_of(const CpuSet* set) {
  std::vector<u32> out;
  for (u32 c = 0; set != nullptr && c < kPlacementCpus; ++c)
    if (CPU_ISSET(c, set)) out.push_back(c);
  return out;
}
#endif
}  // namespace

MHGP11_TEST(parse, 12) {
  std::array<bool, kPlacementCpus> mask{};
  CHECK(parse_cpu_list("0,24\n", mask) && members(mask) == std::vector<u32>({0, 24}));
  CHECK(parse_cpu_list("0-1", mask) && members(mask) == std::vector<u32>({0, 1}));
  CHECK(parse_cpu_list("3,5-6", mask) && members(mask) == std::vector<u32>({3, 5, 6}));
  CHECK(parse_cpu_list("1023", mask) && members(mask) == std::vector<u32>({1023}));
  CHECK(!parse_cpu_list("", mask));
  CHECK(!parse_cpu_list("a", mask));
  CHECK(!parse_cpu_list("5-3", mask));
  CHECK(!parse_cpu_list("1024", mask));
  CHECK(!parse_cpu_list("0,", mask));
  CHECK(!parse_cpu_list("0;1", mask));
  CHECK(!parse_cpu_list("-1", mask));
  CHECK(!parse_cpu_list("0--1", mask));
}

MHGP11_TEST(cores, 9) {
  CpuCores cores;
  const Topology g4(24, 2);
  CHECK(cores_from_lists(g4.lists, cores));
  CHECK_EQ(cores.count, 24u);
  CHECK(cores.first[0] == 0 && cores.second[0] == 24 && cores.first[23] == 23 && cores.second[23] == 47);
  // Fil retire de l'affinite du processus : son coeur n'a plus de second fil.
  const Topology partial(24, 2, {47});
  CHECK(cores_from_lists(partial.lists, cores));
  CHECK(cores.count == 24 && cores.second[23] == kNoThread);
  // Listes contradictoires : le fil 1 dans deux coeurs.
  Topology bad(4, 2);
  bad.text[1] = "0,1"; bad.lists[1] = bad.text[1];
  CHECK(!cores_from_lists(bad.lists, cores));
  // Liste qui ne contient pas son propre fil.
  Topology alien(4, 2);
  alien.text[2] = "3,7"; alien.lists[2] = alien.text[2];
  CHECK(!cores_from_lists(alien.lists, cores));
  std::array<std::string_view, kPlacementCpus> empty{};
  CHECK(!cores_from_lists(empty, cores));
  CHECK(cores.count == 0);
}

MHGP11_TEST(plan, 32) {
#if defined(__linux__)
  CpuCores cores;
  REQUIRE(cores_from_lists(Topology(24, 2).lists, cores));
  const u32 lanes = 39, kmax = 5;
  const PipelinePlacement plan = plan_pipeline(cores, lanes, kmax);
  CHECK_EQ(plan.cores, 24u);
  CHECK_EQ(plan.heavy_count, 4u);
  // Coeurs lourds : les quatre derniers ; leurs freres portent les taches legeres ; resolutions sur les 20 autres.
  CHECK(cpus_of(plan.set_of(lanes + 4)) == std::vector<u32>({23}));      // P5
  CHECK(cpus_of(plan.set_of(lanes + 3)) == std::vector<u32>({22}));      // P4
  CHECK(cpus_of(plan.set_of(lanes + 5 + 3)) == std::vector<u32>({21}));  // V5
  CHECK(cpus_of(plan.set_of(lanes + 5 + 2)) == std::vector<u32>({20}));  // V4
  const std::vector<u32> light{44, 45, 46, 47};
  for (u32 task : {lanes, lanes + 1, lanes + 2, lanes + 5, lanes + 6}) CHECK(cpus_of(plan.set_of(task)) == light);
  std::vector<u32> resolvers;
  for (u32 c = 0; c < 20; ++c) resolvers.push_back(c);
  for (u32 c = 24; c < 44; ++c) resolvers.push_back(c);
  for (u32 task : {0u, 17u, lanes - 1}) CHECK(cpus_of(plan.set_of(task)) == resolvers);
  CHECK(plan.set_of(lanes + 9) == nullptr);  // hors des taches
  // K = 2 : trois taches lourdes (P2, P1, V2).
  const PipelinePlacement two = plan_pipeline(cores, 45, 2);
  CHECK_EQ(two.heavy_count, 3u);
  CHECK(cpus_of(two.set_of(46)) == std::vector<u32>({23}));
  CHECK(cpus_of(two.set_of(45)) == std::vector<u32>({22}));
  CHECK(cpus_of(two.set_of(47)) == std::vector<u32>({21}));
  // Aucun plan : K > 5, K < 2, sans SMT, trop peu de coeurs, sans resolution, coeur lourd sans frere.
  CHECK_EQ(plan_pipeline(cores, 29, 6).cores, 0u);
  CHECK_EQ(plan_pipeline(cores, 47, 1).cores, 0u);
  CHECK_EQ(plan_pipeline(cores, 0, 5).cores, 0u);
  REQUIRE(cores_from_lists(Topology(24, 1).lists, cores));
  CHECK_EQ(plan_pipeline(cores, 39, 5).cores, 0u);
  REQUIRE(cores_from_lists(Topology(7, 2).lists, cores));
  CHECK_EQ(plan_pipeline(cores, 5, 5).cores, 0u);
  REQUIRE(cores_from_lists(Topology(8, 2).lists, cores));
  CHECK_EQ(plan_pipeline(cores, 7, 5).cores, 8u);
  REQUIRE(cores_from_lists(Topology(24, 2, {47}).lists, cores));
  CHECK_EQ(plan_pipeline(cores, 39, 5).cores, 0u);
  CHECK(PipelinePlacement{}.set_of(0) == nullptr);
#else
  CHECK(true);
#endif
}

MHGP11_TEST(affinity, 6) {
#if defined(__linux__)
  cpu_set_t before;
  REQUIRE(sched_getaffinity(0, sizeof(before), &before) == 0);
  u32 target = kPlacementCpus;
  for (u32 c = 0; c < kPlacementCpus && target == kPlacementCpus; ++c)
    if (CPU_ISSET(c, &before)) target = c;
  REQUIRE(target < kPlacementCpus);
  cpu_set_t one;
  CPU_ZERO(&one);
  CPU_SET(target, &one);
  {
    const ScopedAffinity placed(&one);
    cpu_set_t now;
    REQUIRE(sched_getaffinity(0, sizeof(now), &now) == 0);
    CHECK(CPU_EQUAL(&now, &one));
    CHECK_EQ(static_cast<u32>(sched_getcpu()), target);
  }
  cpu_set_t after;
  REQUIRE(sched_getaffinity(0, sizeof(after), &after) == 0);
  CHECK(CPU_EQUAL(&after, &before));
  {
    const ScopedAffinity none(nullptr);  // sans ensemble : aucun effet
    cpu_set_t now;
    REQUIRE(sched_getaffinity(0, sizeof(now), &now) == 0);
    CHECK(CPU_EQUAL(&now, &before));
  }
  CpuCores cores;
  CHECK(read_cpu_cores(cores) && cores.count > 0);  // /sys lisible sur les machines de porte (codespace, G4)
#else
  CHECK(true);
#endif
}

MHGP11_TEST_MAIN()
