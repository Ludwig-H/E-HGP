// Parite exacte, ordre d'emission, compteur paye et repli aux grandes feuilles ; natif sur G4 seulement.
#include "small_pair_graph_support.hpp"
#include "catalogue/frontier_dispatch.hpp"
#include "test.hpp"
using namespace pair_test;

MHGP11_TEST(masks, 80) {
  std::array<u64,32> rows;
  for (u32 m:{1u,2u,3u,31u,32u}) {
    rows.fill(~u64{0});
    auto made=SmallPairGraph::make(rows,m,true); REQUIRE(made.ok());
    auto graph=made.value(); CHECK(graph.enabled());
    CHECK(std::all_of(rows.begin(),rows.end(),[](u64 v){return v==0;}));
    CHECK_EQ(std::popcount(graph.initial()),static_cast<int>(m));
    CHECK_EQ(graph.initial()>>m,0u);
    if (m>1) {
      graph.connect(0,m-1); CHECK_EQ(graph.neighbors(0),u64{1}<<(m-1));
      CHECK_EQ(graph.neighbors(m-1),1u); CHECK_EQ(graph.neighbors(0)&1,0u);
    }
    u64 candidates=graph.initial();
    for (u32 i=0;i<m;++i) CHECK_EQ(graph.next(candidates,99),i);
    CHECK_EQ(graph.next(candidates,99),m); CHECK_EQ(candidates,0u);
  }
  CHECK_EQ(SmallPairGraph::make(std::span(rows).first(31),2,true).outcome().reason,Reason::catalogue_invariant);
  for (u32 m:{0u,32u,33u,64u,65u,1024u}) {
    auto off=SmallPairGraph::make({},m,false); REQUIRE(off.ok()); CHECK(!off.value().enabled());
    u64 bits=~u64{0}; CHECK_EQ(off.value().next(bits,17),17u); CHECK_EQ(bits,~u64{0});
    if (m>32) {
      auto fallback=SmallPairGraph::make({},m,true); REQUIRE(fallback.ok());
      CHECK(!fallback.value().enabled()); CHECK_EQ(fallback.value().initial(),0u);
    }
  }
}

MHGP11_TEST(leaf_order, 220) {
  auto cases=fixtures();
  for (u32 m:{31u,32u,33u,64u,65u}) cases.push_back(line(m));
  u64 pair_rejects=0, q4=0, last_bit=0;
  for (const auto& points:cases) {
    MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
    auto cloud=cloud_of(points,owner); REQUIRE(cloud.ok());
    const i64 hi=i64(kCoordMax)+1;
    for (Box box:{Box{{0,0,0},{hi,hi,hi}},Box{{0,0,0},{1,1,1}}})
      for (bool cache:{false,true}) {
        auto before=visit(cloud.value(),box,false,cache,work); REQUIRE(before.ok());
        auto after=visit(cloud.value(),box,true,cache,work); REQUIRE(after.ok());
        CHECK(same_order(before.value(),after.value())); CHECK(same_work(before.value().ledger,after.value().ledger));
        pair_rejects+=before.value().ledger.region_pair_rejects;
        q4+=after.value().ledger.q4_levels;
        if (cloud.value().sites()>32) CHECK(before.value().ledger==after.value().ledger);
        else { CHECK_EQ(after.value().ledger.region_pair_tests,0u); CHECK_EQ(after.value().ledger.region_pair_rejects,0u); }
        if (cloud.value().sites()>=32)
          for (SiteIdx site:after.value().population.span()) last_bit+=idx(site)==cloud.value().sites()-1 ? 1u:0u;
      }
  }
  CHECK(pair_rejects>0); CHECK(q4>0); CHECK(last_bit>0);
}

MHGP11_TEST(contacts_obtuse, 30) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto pair=cloud_of({{0,0,0},{4,0,0}},owner); REQUIRE(pair.ok());
  for (Box box:{Box{{0,0,0},{2,1,1}},Box{{2,0,0},{3,1,1}}}) {
    auto before=visit(pair.value(),box,false,true,work); REQUIRE(before.ok());
    auto after=visit(pair.value(),box,true,true,work); REQUIRE(after.ok());
    CHECK(same_order(before.value(),after.value())); CHECK(same_work(before.value().ledger,after.value().ledger));
    CHECK_EQ(after.value().ledger.judged,box.lo[0]==0 ? 0u:1u);
    CHECK_EQ(after.value().ledger.region_pair_rejects,0u);
  }
  auto tetra=cloud_of({{1,2,6},{8,4,8},{2,1,3},{7,8,5}},owner); REQUIRE(tetra.ok());
  const Box box{{4,4,4},{5,5,5}};
  for (bool cache:{false,true}) {
    auto before=visit(tetra.value(),box,false,cache,work); REQUIRE(before.ok());
    auto after=visit(tetra.value(),box,true,cache,work); REQUIRE(after.ok());
    CHECK(same_order(before.value(),after.value())); CHECK(same_work(before.value().ledger,after.value().ledger));
    CHECK_EQ(after.value().ledger.q4_candidates,1u); CHECK_EQ(after.value().ledger.q4_levels,1u);
    CHECK_EQ(after.value().ledger.region_line_tests,7u);
    u64 emitted_tetra=0;
    for (const auto& record:after.value().records.span())
      emitted_tetra+=record.ball.qmin==4 && record.ball.m==4 ? 1u:0u;
    CHECK_EQ(emitted_tetra,1u);
  }
}

MHGP11_TEST(mixed_fallback, 30) {
  CatalogueLedger old_sum,new_sum;
  u64 small_removed=0,fallback_paid=0;
  for (u32 m:{4u,33u,32u,65u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
    auto cloud=cloud_of(line(m),owner); REQUIRE(cloud.ok());
    const Box box{{0,0,0},{1,1,1}};
    auto before=visit(cloud.value(),box,false,true,work); REQUIRE(before.ok());
    auto after=visit(cloud.value(),box,true,true,work); REQUIRE(after.ok());
    CHECK(same_order(before.value(),after.value())); CHECK(same_work(before.value().ledger,after.value().ledger));
    REQUIRE(add_catalogue_ledger(old_sum,before.value().ledger).ok());
    REQUIRE(add_catalogue_ledger(new_sum,after.value().ledger).ok());
    if (m<=32) small_removed+=before.value().ledger.region_pair_rejects;
    else fallback_paid+=after.value().ledger.region_pair_rejects;
  }
  CHECK(small_removed>0); CHECK(fallback_paid>0); CHECK(same_work(old_sum,new_sum));
  CHECK_EQ(new_sum.prefixes,old_sum.prefixes-small_removed);
  CHECK_EQ(new_sum.region_pair_rejects,fallback_paid);
}

MHGP11_TEST(equivalence, 240) {
  u32 arities=0; u64 extended=0;
  for (auto points:fixtures()) for (u32 rotation=0;rotation<2;++rotation) {
    if (rotation!=0) {
      std::reverse(points.begin(),points.end());
      for (auto& p:points) p={p[2],p[0],p[1]};
    }
    MemoryBudget owner(MemoryBudget::kUnlimited),reference_budget(MemoryBudget::kUnlimited);
    auto cloud=cloud_of(points,owner); REQUIRE(cloud.ok());
    CatalogueParams p; p.kmax=5; p.leaf_size=8; p.cache_center_lines=true;
    auto reference=build_catalogue(cloud.value(),p,reference_budget); REQUIRE(reference.ok());
    p.pair_graph=true;
    for (u32 workers:{0u,1u,4u}) for (bool single:{false,true}) {
      if (workers==0 && single) continue;
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto pool=sched::make_pool({std::max(workers,1u)}); REQUIRE(pool.ok());
      p.single_pass=single; p.adaptive_frontier=single; p.indirect_sort=single; p.parallel_assembly=single;
      {
        auto result=workers==0 ? build_catalogue(cloud.value(),p,budget) :
                                build_catalogue(cloud.value(),p,budget,*pool.value());
        REQUIRE(result.ok()); CHECK(same_geometry(reference.value(),result.value()));
        CHECK(same_work(reference.value().ledger(),result.value().ledger()));
        for (const auto& b:result.value().balls_data()) { arities|=u32{1}<<b.qmin; extended+=b.m>b.qmin ? 1u:0u; }
      }
      CHECK(budget.released().ok());
    }
  }
  CHECK_EQ(arities,(u32{1}<<2)|(u32{1}<<3)|(u32{1}<<4)); CHECK(extended>0);
}

template<class Front>
void frontier_option(bool adaptive) {
  MemoryBudget owner(MemoryBudget::kUnlimited),budget(MemoryBudget::kUnlimited);
  auto cloud=cloud_of(line(17),owner); REQUIRE(cloud.ok());
  auto pool=sched::make_pool({4}); REQUIRE(pool.ok());
  CatalogueParams before; before.adaptive_frontier=adaptive;
  Workspace unused; Collector collector; Run first{cloud.value(),before,budget,unused,collector,{}};
  Front front; REQUIRE(prepare_frontier(front,first,*pool.value()).ok()); REQUIRE(front.size()>0);
  CatalogueParams changed=before; changed.pair_graph=true;
  Run other{cloud.value(),changed,budget,unused,collector,{}};
  CHECK_EQ(front.execute_task(0,other).reason,Reason::catalogue_invariant);
  CHECK_EQ(verify_frontier(front,other,*pool.value()).reason,Reason::catalogue_invariant);
  CHECK(other.ledger==CatalogueLedger{});
}
MHGP11_TEST(frontier_identity, 14) {
  frontier_option<Frontier>(false);
  frontier_option<AdaptiveFrontier>(true);
}
MHGP11_TEST_MAIN()
