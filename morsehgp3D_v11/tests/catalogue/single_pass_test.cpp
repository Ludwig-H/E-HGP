// Blocs/passe unique, comparaison des catalogues et refus publics tardifs ; natif execute exclusivement sur G4.
#include "single_pass_support.hpp"
#include "test.hpp"
using namespace single_test;

MHGP11_TEST(pages, 2500) {
  CHECK_EQ(sizeof(Emission),kCoordBits==18 ? 96u : kCoordBits==21 ? 104u : 112u);
  CHECK_EQ((FixedPages<Emission,kEmissionBlock>::metadata_bytes()),80u);
  CHECK_EQ((FixedPages<SiteIdx,kPopulationBlock>::metadata_bytes()),80u);
  MemoryBudget budget(MemoryBudget::kUnlimited), zero(0);
  {
    FixedPages<u64,4> pages;
    const std::array<u64,3> a{2,3,5}, b{7,11,13};
    auto first=pages.prepare(a.size(),budget); REQUIRE(first.ok());
    pages.commit(std::move(first.value()),a);
    CHECK_EQ(pages.blocks(),1u); CHECK_EQ(pages.size(),3u);
    const u64 held=budget.used();
    { auto pending=pages.prepare(4,budget); REQUIRE(pending.ok()); CHECK(budget.used()>held); }
    CHECK_EQ(budget.used(),held);
    CHECK_EQ(pages.prepare(5,budget).outcome().reason,Reason::catalogue_invariant);
    CHECK_EQ(pages.prepare(3,zero).outcome().reason,Reason::memory_budget);
    CHECK_EQ(pages.size(),3u); CHECK_EQ(budget.used(),held);
    auto second=pages.prepare(b.size(),budget); REQUIRE(second.ok());
    pages.commit(std::move(second.value()),std::span(b).first(2),std::span(b).subspan(2));
    CHECK_EQ(pages.blocks(),2u); CHECK_EQ(pages.size(),6u);
    std::array<u64,6> copied{};
    REQUIRE(pages.copy_to(copied).ok());
    for (u32 i=0;i<3;++i) { CHECK_EQ(copied[i],a[i]); CHECK_EQ(copied[i+3],b[i]); }
    CHECK_EQ(pages.copy_to(std::span(copied).first(5)).reason,Reason::catalogue_invariant);
    CHECK_EQ(budget.used(),2*(4*sizeof(u64)+FixedPages<u64,4>::metadata_bytes()));
    const std::array<u64,4> c{17,19,23,29};
    auto third=pages.prepare(c.size(),budget); REQUIRE(third.ok());
    pages.commit(std::move(third.value()),std::span(c).first(2),std::span(c).subspan(2));
    std::array<u64,10> final{}; REQUIRE(pages.copy_to(final).ok());
    for (u32 i=0;i<4;++i) CHECK_EQ(final[6+i],c[i]);
  }
  CHECK(budget.released().ok());
  {
    SinglePassOutput output;
    for (u32 i=0;i<257;++i) REQUIRE(append(output,budget).ok());
    CatalogueExecution cost; cost.geometry_passes=1; REQUIRE(output.account(cost).ok());
    CHECK_EQ(cost.arena_blocks,4u); CHECK_EQ(cost.compact_records,257u); CHECK_EQ(cost.compact_population,2056u);
    CHECK_EQ(cost.arena_capacity_bytes,512*sizeof(Emission)+4096*sizeof(SiteIdx));
    CHECK_EQ(budget.used(),cost.arena_capacity_bytes+cost.arena_metadata_bytes);
    std::array<Emission,257> records;
    std::array<SiteIdx,2056> population;
    REQUIRE(output.compact(records,population,13).ok());
    for (u32 i=0;i<257;++i) CHECK_EQ(records[i].population_begin,13+8*u64{i});
    for (u32 i=0;i<2056;++i) CHECK_EQ(population[i],ids[i%8]);
  }
  CHECK(budget.released().ok());
}

MHGP11_TEST(equivalence, 440) {
  auto p1=sched::make_pool({1}),p4=sched::make_pool({4}),p48=sched::make_pool({48});
  REQUIRE(p1.ok()); REQUIRE(p4.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,3> pools{p1.value().get(),p4.value().get(),p48.value().get()};
  std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0}}, {{0,0,0},{4,0,0},{0,4,0},{4,4,0},{2,2,0}},
    {{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}},
    {{0,0,0},{kCoordMax,kCoordMax,0},{kCoordMax,0,kCoordMax},{0,kCoordMax,kCoordMax}}};
  std::vector<Xyz> line;
  for (u32 i=0;i<17;++i) line.push_back({2*i,0,0});
  fixtures.push_back(line);
  for (const auto& points:fixtures) for (bool adaptive:{false,true}) {
    MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
    auto cloud=cloud_of(points,owner); REQUIRE(cloud.ok());
    CatalogueParams params; params.kmax=3; params.leaf_size=6;
    params.cache_center_lines=true; params.indirect_sort=true; params.parallel_assembly=true;
    params.adaptive_frontier=adaptive;
    auto baseline=build_catalogue(cloud.value(),params,work,*p1.value()); REQUIRE(baseline.ok());
    CHECK_EQ(baseline.value().execution().geometry_passes,2u);
    params.single_pass=true; CatalogueExecution expected;
    for (auto* pool:pools) {
      CatalogueTimings t; CatalogueDiagnostics d;
      auto made=build_catalogue(cloud.value(),params,work,*pool,&t,&d); REQUIRE(made.ok());
      CHECK(same(made.value(),baseline.value())); const auto& e=made.value().execution();
      CHECK_EQ(e.geometry_passes,1u); CHECK_EQ(e.compact_records,made.value().balls());
      CHECK_EQ(e.compact_population,made.value().population().size());
      if (pool->size()==1) expected=e; else CHECK(e==expected);
      CHECK_EQ(t.count_ns+t.fill_ns+t.replay_ns,0u); CHECK_EQ(d.planning().replay_bytes,0u);
      const u64 concurrency=std::min<u64>(pool->size(),d.tasks().size());
      CHECK(t.single_task_sum_ns<=concurrency*t.single_pass_ns); CHECK(t.single_task_max_ns<=t.single_pass_ns);
      CHECK(t.compact_task_sum_ns<=concurrency*t.compact_ns); CHECK(t.compact_task_max_ns<=t.compact_ns);
      u64 single=0,compact=0;
      for (const auto& task:d.tasks()) { single+=task.single_pass_ns; compact+=task.compact_ns; }
      CHECK_EQ(single,t.single_task_sum_ns); CHECK_EQ(compact,t.compact_task_sum_ns);
    }
  }
}

namespace {
struct Nested {
  const Cloud& cloud; MemoryBudget& budget; sched::Pool& pool;
  CatalogueTimings timing=sentinel(); CatalogueDiagnostics diagnostic{}; Outcome outcome{};
  static Outcome call(void* raw,u64,u64,u32) noexcept {
    auto& self=*static_cast<Nested*>(raw);
    CatalogueParams p; p.kmax=1; p.single_pass=true;
    auto result=build_catalogue(self.cloud,p,self.budget,self.pool,&self.timing,&self.diagnostic);
    self.outcome=result.outcome(); return {};
  }
};
}
MHGP11_TEST(pool_busy, 10) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=cloud_of({{0,0,0},{2,0,0}},owner); REQUIRE(cloud.ok());
  auto pool=sched::make_pool({4}); REQUIRE(pool.ok());
  Nested nested{cloud.value(),work,*pool.value()}; const auto before=times(nested.timing);
  REQUIRE(pool.value()->parallel_for(1,1,&nested,Nested::call).ok());
  CHECK_EQ(nested.outcome.reason,Reason::pool_busy); CHECK(times(nested.timing)==before);
  CHECK(nested.diagnostic.tasks().empty()); CHECK(work.released().ok());
  CatalogueParams p; p.kmax=1; p.single_pass=true;
  auto again=build_catalogue(cloud.value(),p,work,*pool.value()); REQUIRE(again.ok());
  CHECK_EQ(again.value().balls(),1u); CHECK_EQ(again.value().execution().geometry_passes,1u);
}

MHGP11_TEST(refusals, 275) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=cloud_of({{0,0,0},{2,0,0},{4,0,0},{6,0,0},{8,0,0}},owner); REQUIRE(cloud.ok());
  auto pool=sched::make_pool({4}); REQUIRE(pool.ok());
  CatalogueParams p; p.kmax=1; p.leaf_size=4;
  CatalogueDiagnostics d; CatalogueTimings t;
  auto kept=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d); REQUIRE(kept.ok());
  const auto* previous=d.tasks().data(); const auto before=times(t); const u64 held=work.used();
  const auto* saved=kept.value().balls_data().data(); p.single_pass=true;
  CHECK_EQ(build_catalogue(cloud.value(),p,work).outcome().reason,Reason::parameter_out_of_range);
  const u64 nodes=kept.value().ledger().nodes;
  for (u32 mode=0;mode<2;++mode) {
    p.max_nodes=mode==0 ? nodes-1 : 0;
    p.ball_limit=mode==1 ? kept.value().balls() : kNone;
    auto refused=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d);
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason,mode==0 ? Reason::node_budget : Reason::index_overflow_u32);
    CHECK(times(t)==before); CHECK(d.tasks().data()==previous); CHECK_EQ(work.used(),held);
    CHECK(kept.value().balls_data().data()==saved);
  }
  p.max_nodes=nodes; p.ball_limit=u64{kept.value().balls()}+1;
  auto good=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d); REQUIRE(good.ok());
  CHECK(same(good.value(),kept.value())); CHECK_EQ(good.value().execution().geometry_passes,1u);
  {
    MemoryBudget bounded(kEmissionBlock*sizeof(Emission)+kPopulationBlock*sizeof(SiteIdx)+
                         FixedPages<Emission,kEmissionBlock>::metadata_bytes()+
                         FixedPages<SiteIdx,kPopulationBlock>::metadata_bytes());
    SinglePassOutput output;
    for (u32 i=0;i<256;++i) REQUIRE(append(output,bounded).ok());
    const u64 used=bounded.used();
    CHECK_EQ(append(output,bounded).reason,Reason::memory_budget);
    CHECK_EQ(output.balls(),256u); CHECK_EQ(output.incidences(),2048u); CHECK_EQ(bounded.used(),used);
  }
}
MHGP11_TEST_MAIN()
