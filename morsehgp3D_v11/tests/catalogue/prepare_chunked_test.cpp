// Preparation par tranches (prepare_nodes_chunked) contre prepare_node, noeud par noeud : memes sites dans le meme
// ordre, meme boite ajustee, meme capacite de liste, meme registre. Noeuds pris dans l'arbre de trois nuages jusqu'a
// la profondeur 5 ; tranches de 1 a 4096 sites ; K de 1 a 10. La grille symetrique met des temoins ex aequo au bord
// du reservoir : seul l'ordre (distance, rang dans le parent) les departage.
#include <algorithm>
#include <random>
#include <vector>

#include "adaptive_support.hpp"
#include "catalogue/internal.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace adaptive_test;
namespace cd = mhgp11::catalogue_detail;

namespace {

std::vector<Position> grid(u32 side, u32 step, u32 shift) {
  std::vector<Position> out;
  for (u32 x = 0; x < side; ++x)
    for (u32 y = 0; y < side; ++y)
      for (u32 z = 0; z < side; ++z) out.push_back({shift + step * x, shift + step * y, shift + step * z});
  return out;
}

std::vector<Position> scatter(u32 count, u32 width, u64 seed) {
  std::mt19937_64 rng(seed);
  std::vector<Position> out;
  for (u32 i = 0; i < count; ++i) {
    // Deux amas et un fond : des noeuds pleins, d'autres presque vides.
    const u32 base = i % 3 == 0 ? 0u : i % 3 == 1 ? width / 2 : width / 5;
    const u32 spread = i % 3 == 2 ? width : width / 8;
    out.push_back({base + static_cast<u32>(rng() % spread), base + static_cast<u32>(rng() % spread),
                   static_cast<u32>(rng() % spread)});
  }
  return out;
}

struct Spec { u64 parent; cd::Box box; u32 depth; };

bool same_box(const cd::Box& a, const cd::Box& b) { return a.lo == b.lo && a.hi == b.hi; }

}  // namespace

MHGP11_TEST(equivalence, 3000) {
  struct Fixture { std::vector<Position> points; int k; u32 leaf; };
  const std::vector<Fixture> fixtures{
      {grid(9, 2, 1), 1, 6}, {grid(9, 2, 1), 5, 6}, {grid(12, 1, 0), 10, 12},
      {line(700), 3, 8}, {scatter(3000, 4096, 20261007), 5, 8}, {scatter(2000, 512, 7), 2, 6}};
  auto p1 = sched::make_pool({1}), p6 = sched::make_pool({6});
  REQUIRE(p1.ok() && p6.ok());
  u64 compared = 0, chunked = 0, kept_nodes = 0, empty_nodes = 0;
  for (const auto& f : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited);
    auto cloud = prepare(f.points, owner);
    REQUIRE(cloud.ok());
    CatalogueParams p; p.kmax = f.k; p.leaf_size = f.leaf;
    cd::Workspace unused; cd::Collector collector;
    cd::Run run{cloud.value(), p, owner, unused, collector, {}};
    Buffer<SiteIdx> root;
    cd::Box box;
    REQUIRE(cd::make_root(run, root, box).ok());
    // Arbre de reference par prepare_node, en largeur jusqu'a la profondeur 5.
    std::vector<cd::ReadyNode> nodes;
    nodes.reserve(256);
    nodes.emplace_back();
    REQUIRE(cd::prepare_node(run, root.span(), box, 0, nodes[0]).ok());
    std::vector<Spec> specs;
    for (u64 i = 0; i < nodes.size() && nodes.size() < 250; ++i) {
      cd::Box left, right;
      if (nodes[i].count == 0 || nodes[i].depth >= 5 || !cd::split_ready(nodes[i], p, left, right)) continue;
      for (const cd::Box& side : {left, right}) {
        specs.push_back({i, side, nodes[i].depth + 1});
        cd::Run fresh{cloud.value(), p, owner, unused, collector, {}};
        cd::ReadyNode child;
        REQUIRE(cd::prepare_node(fresh, nodes[i].sites(), side, nodes[i].depth + 1, child).ok());
        nodes.push_back(std::move(child));
      }
    }
    REQUIRE(!specs.empty());
    // Boites loin du nuage : les sites gardes (les plus proches) sont tous hors de la boite, boite ajustee vide.
    const i64 top = i64{kCoordMax} + 1;
    specs.push_back({0, cd::Box{{top - 8, top - 8, top - 8}, {top, top, top}}, 1});
    specs.push_back({0, cd::Box{{top - 2, 0, 0}, {top, 2, 2}}, 1});
    for (u64 chunk : {u64{1}, u64{2}, u64{3}, u64{7}, u64{64}, u64{4096}}) {
      for (auto* pool : {p1.value().get(), p6.value().get()}) {
        std::vector<cd::ReadyNode> got(specs.size());
        std::vector<CatalogueLedger> ledgers(specs.size());
        std::vector<cd::NodeJob> jobs;
        for (u64 k = 0; k < specs.size(); ++k)
          jobs.push_back({nodes[specs[k].parent].sites(), specs[k].box, specs[k].depth, &got[k], &ledgers[k]});
        cd::Run batch{cloud.value(), p, owner, unused, collector, {}};
        REQUIRE(cd::prepare_nodes_chunked(batch, *pool, jobs, chunk).ok());
        CHECK(batch.ledger == CatalogueLedger{});  // le registre de chaque noeud va dans son *ledger
        for (u64 k = 0; k < specs.size(); ++k) {
          cd::Run fresh{cloud.value(), p, owner, unused, collector, {}};
          cd::ReadyNode want;
          REQUIRE(cd::prepare_node(fresh, nodes[specs[k].parent].sites(), specs[k].box, specs[k].depth, want).ok());
          const bool same = got[k].count == want.count && got[k].depth == want.depth &&
                            same_box(got[k].box, want.box) && got[k].storage.size() == want.storage.size() &&
                            std::equal(got[k].sites().begin(), got[k].sites().end(), want.sites().begin(),
                                       want.sites().end());
          CHECK(same);
          CHECK(ledgers[k] == fresh.ledger);
          ++compared;
          chunked += jobs[k].parent.size() > chunk;
          if (want.count == 0) ++empty_nodes; else ++kept_nodes;
        }
      }
    }
    CHECK(owner.used() >= root.size() * sizeof(SiteIdx));
  }
  CHECK(compared > 1500 && chunked > 1000 && kept_nodes > 1000 && empty_nodes > 0);
  std::printf("prepare_chunked compared=%llu chunked=%llu kept=%llu empty=%llu\n",
              static_cast<unsigned long long>(compared), static_cast<unsigned long long>(chunked),
              static_cast<unsigned long long>(kept_nodes), static_cast<unsigned long long>(empty_nodes));
}

MHGP11_TEST(refusals, 6) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare(grid(6, 3, 2), owner);
  auto pool = sched::make_pool({4});
  REQUIRE(cloud.ok() && pool.ok());
  CatalogueParams p; p.kmax = 3; p.leaf_size = 4;
  cd::Workspace unused; cd::Collector collector;
  cd::Run run{cloud.value(), p, owner, unused, collector, {}};
  Buffer<SiteIdx> root;
  cd::Box box;
  REQUIRE(cd::make_root(run, root, box).ok());
  cd::ReadyNode a, b, c;
  CatalogueLedger la, lb, lc;
  const std::vector<cd::NodeJob> three{{root.span(), box, 0, &a, &la}, {root.span(), box, 1, &b, &lb},
                                       {root.span(), box, 2, &c, &lc}};
  CHECK_EQ(cd::prepare_nodes_chunked(run, *pool.value(), three, 0).reason, Reason::parameter_out_of_range);
  // Quota exact : trois noeuds passent, deux ne suffisent pas (node_budget, comme prepare_node).
  CatalogueParams q = p; q.max_nodes = 3;
  cd::NodeQuota enough(3);
  cd::Run quota_run{cloud.value(), q, owner, unused, collector, {}, &enough};
  CHECK(cd::prepare_nodes_chunked(quota_run, *pool.value(), three, 5).ok());
  CatalogueParams r = p; r.max_nodes = 2;
  cd::NodeQuota short_quota(2);
  cd::Run short_run{cloud.value(), r, owner, unused, collector, {}, &short_quota};
  CHECK_EQ(cd::prepare_nodes_chunked(short_run, *pool.value(), three, 5).reason, Reason::node_budget);
  // Profondeur au-dela de kMaxDepth : invariant, avant tout calcul.
  const std::vector<cd::NodeJob> deep{{root.span(), box, cd::kMaxDepth + 1, &a, &la}};
  CHECK_EQ(cd::prepare_nodes_chunked(run, *pool.value(), deep, 5).reason, Reason::catalogue_invariant);
  // Budget : la liste du noeud ne tient pas.
  MemoryBudget tight(root.size() * sizeof(SiteIdx) - 1);
  cd::Run tight_run{cloud.value(), p, tight, unused, collector, {}};
  const std::vector<cd::NodeJob> one{{root.span(), box, 0, &a, &la}};
  CHECK_EQ(cd::prepare_nodes_chunked(tight_run, *pool.value(), one, 5).reason, Reason::memory_budget);
  a = cd::ReadyNode{};
  CHECK(tight.released().ok());
  CatalogueParams wide = p; wide.kmax = 13;
  cd::Run wide_run{cloud.value(), wide, owner, unused, collector, {}};
  CHECK_EQ(cd::prepare_nodes_chunked(wide_run, *pool.value(), one, 5).reason, Reason::parameter_out_of_range);
}

MHGP11_TEST_MAIN()
