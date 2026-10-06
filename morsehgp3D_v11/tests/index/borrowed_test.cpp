// Census emprunte : egalite des populations, travail reel divise par deux et exclusions deterministes.
#include <latch>
#include <thread>
#include <type_traits>
#include "test_support.hpp"
#include "test.hpp"
using namespace mhgp11;
using namespace index_test;

namespace {
bool twice(const CensusLedger& a, const CensusLedger& b) {
  return a.nodes==2*b.nodes && a.bounds==2*b.bounds && a.point_tests==2*b.point_tests &&
         a.inside_blocks==2*b.inside_blocks && a.outside_blocks==2*b.outside_blocks && a.passes==2*b.passes;
}
struct Compare {
  const Census& expected;
  bool correct=false;
  u32 calls=0;
  static Outcome call(void* raw,const BorrowedCensus& result) {
    auto& c=*static_cast<Compare*>(raw); ++c.calls;
    c.correct=c.expected.kind()==result.kind() && equal(c.expected.interior(),result.interior()) &&
              equal(c.expected.shell(),result.shell()) && twice(c.expected.ledger(),result.ledger());
    return {};
  }
};
struct Count {
  u32 calls=0;
  static Outcome call(void* raw,const BorrowedCensus&) { ++static_cast<Count*>(raw)->calls; return {}; }
};
Outcome bad_alloc(void*,const BorrowedCensus&) { throw std::bad_alloc(); }
Outcome other_throw(void*,const BorrowedCensus&) { throw 7; }
Outcome refusal(void*,const BorrowedCensus&) { return fail(Reason::size_mismatch); }
}

MHGP11_TEST(fixtures, 200) {
  const std::array<Input,4> fixtures{octa(),
    Input({{2,4,4},{4,2,4},{6,4,4},{4,6,4}}),  // coplanaires, toute la coquille : m=n
    Input({{4,4,4},{4,5,4},{4,4,5}}),          // tout interieur, saturation seuil n
    Input({{2,4,4},{6,4,4},{4,2,4},{4,6,4},{4,4,2},{4,4,6},{4,4,4},{4,5,4},{4,4,5}})};
  for (const auto& input:fixtures) for (u32 leaf:{1u,4u,16u,256u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited),reference(MemoryBudget::kUnlimited);
    auto cloud=input.prepare(owner); REQUIRE(cloud.ok());
    auto index=build_index(std::move(cloud.value()),{leaf},owner); REQUIRE(index.ok());
    const u32 n=index.value().cloud().sites();
    auto workspace=CensusWorkspace::make(index.value(),work); REQUIRE(workspace.ok());
    CHECK_EQ(workspace.value()->capacity(),n); CHECK_EQ(work.used(),4*u64{n});
    for (u32 threshold:{1u,2u,3u,4u,n,n+1,kNone}) {
      auto expected=census(index.value(),ball(),threshold,reference); REQUIRE(expected.ok());
      Compare comparison{expected.value()};
      REQUIRE(workspace.value()->query(index.value(),ball(),threshold,&comparison,Compare::call).ok());
      CHECK(comparison.correct); CHECK_EQ(comparison.calls,1u);
      CHECK_EQ(work.used(),4*u64{n}); CHECK_EQ(work.peak(),4*u64{n});
    }
    const auto outside=num::Sphere::point(point(100,100,100));
    auto empty=census(index.value(),outside,1,reference); REQUIRE(empty.ok());
    Compare comparison{empty.value()};
    REQUIRE(workspace.value()->query(index.value(),outside,1,&comparison,Compare::call).ok());
    CHECK(comparison.correct);
  }
}

// Levier V3, census emprunte : meme decision a la racine que le census possede, en une seule passe.
MHGP11_TEST(lattice, 40) {
  const auto beside_ball=num::Sphere::through(point(0,0,0),point(4,0,0));
  const auto core_ball=num::Sphere::through(point(0,0,0),point(8,0,0));
  REQUIRE(beside_ball.ok() && beside_ball.value() && core_ball.ok() && core_ball.value());
  const std::array<Input,2> inputs{Input({{3,2,0},{5,2,0},{4,3,0},{3,3,0},{5,3,0}}),
                                   Input({{2,0,0},{6,0,0},{2,1,1},{6,1,1},{4,0,1}})};
  struct Root {
    CensusLedger ledger; u64 interior=0, shell=0; u32 calls=0;
    static Outcome call(void* raw,const BorrowedCensus& result) {
      auto& r=*static_cast<Root*>(raw); ++r.calls; r.ledger=result.ledger();
      r.interior=result.interior().size(); r.shell=result.shell().size(); return {};
    }
  };
  for (u32 leaf:{1u,16u}) for (int inside=0;inside<2;++inside) {
    MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
    auto cloud=inputs[inside].prepare(owner); REQUIRE(cloud.ok());
    auto index=build_index(std::move(cloud.value()),{leaf},owner); REQUIRE(index.ok());
    auto workspace=CensusWorkspace::make(index.value(),work); REQUIRE(workspace.ok());
    const num::Sphere& sphere=inside ? *core_ball.value() : *beside_ball.value();
    Root root;
    REQUIRE(workspace.value()->query(index.value(),sphere,kNone,&root,Root::call).ok());
    CHECK_EQ(root.calls,1u); CHECK_EQ(root.ledger.passes,1u); CHECK_EQ(root.ledger.nodes,1u);
    CHECK_EQ(root.ledger.bounds,1u); CHECK_EQ(root.ledger.point_tests,0u);
    CHECK_EQ(root.ledger.inside_blocks,inside ? 1u : 0u); CHECK_EQ(root.ledger.outside_blocks,inside ? 0u : 1u);
    CHECK_EQ(root.interior,inside ? 5u : 0u); CHECK_EQ(root.shell,0u);
  }
}

MHGP11_TEST(ownership, 22) {
  CHECK(!std::is_copy_constructible_v<CensusWorkspace> && !std::is_move_constructible_v<CensusWorkspace>);
  CHECK(!std::is_copy_constructible_v<BorrowedCensus> && !std::is_move_constructible_v<BorrowedCensus>);
  MemoryBudget owner(MemoryBudget::kUnlimited),work(44),short_budget(43);
  auto input=octa(); auto cloud=input.prepare(owner); REQUIRE(cloud.ok());
  auto index=build_index(std::move(cloud.value()),{4},owner); REQUIRE(index.ok());
  const u64 held=owner.used();
  auto denied=CensusWorkspace::make(index.value(),short_budget);
  CHECK(!denied.ok()); CHECK_EQ(denied.outcome().reason,Reason::memory_budget); CHECK(short_budget.released().ok());
  auto workspace=CensusWorkspace::make(index.value(),work); REQUIRE(workspace.ok());
  CHECK_EQ(work.used(),44u); CHECK_EQ(owner.used(),held);
  const auto* address=workspace.value().get();
  auto moved=std::move(workspace.value()); CHECK(moved.get()==address);
  input.x.assign(input.x.size(),200);  // le stockage brut du caller n'aliasait deja pas le Cloud
  Count count;
  CHECK_EQ(moved->query(index.value(),ball(),0,&count,Count::call).reason,Reason::parameter_out_of_range);
  CHECK_EQ(moved->query(index.value(),ball(),4,&count,nullptr).reason,Reason::parameter_out_of_range);
  CHECK_EQ(count.calls,0u);
  auto another_cloud=octa().prepare(owner); REQUIRE(another_cloud.ok());
  auto foreign=build_index(std::move(another_cloud.value()),{4},owner); REQUIRE(foreign.ok());
  CHECK_EQ(moved->query(foreign.value(),ball(),4,&count,Count::call).reason,Reason::parameter_out_of_range);
  REQUIRE(moved->query(index.value(),ball(),4,&count,Count::call).ok()); CHECK_EQ(count.calls,1u);
  GlobalIndex transferred(std::move(index.value()));
  CHECK_EQ(moved->query(index.value(),ball(),4,&count,Count::call).reason,Reason::parameter_out_of_range);
  CHECK_EQ(moved->query(transferred,ball(),4,&count,Count::call).reason,Reason::parameter_out_of_range);
  moved.reset(); CHECK(work.released().ok());
  CHECK_EQ(CensusWorkspace::make(index.value(),work).outcome().reason,Reason::empty_input);
}

namespace {
struct Nested {
  CensusWorkspace& workspace; const GlobalIndex& index; u32 calls=0; Outcome nested{};
  static Outcome call(void* raw,const BorrowedCensus&) {
    auto& c=*static_cast<Nested*>(raw); ++c.calls;
    Count ignored;
    c.nested=c.workspace.query(c.index,ball(),1,&ignored,Count::call);
    return {};
  }
};
struct Hold {
  std::latch entered{1},release{1}; bool called=false,unchanged=false;
  static Outcome call(void* raw,const BorrowedCensus& result) {
    auto& h=*static_cast<Hold*>(raw);
    const auto before=copy(result.shell());
    h.called=true;
    h.entered.count_down(); h.release.wait();
    h.unchanged=copy(result.shell())==before;
    return {};
  }
};
}
MHGP11_TEST(callbacks, 19) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=octa().prepare(owner); REQUIRE(cloud.ok());
  auto index=build_index(std::move(cloud.value()),{4},owner); REQUIRE(index.ok());
  auto workspace=CensusWorkspace::make(index.value(),work); REQUIRE(workspace.ok());
  for (auto callback:{bad_alloc,other_throw,refusal}) {
    const auto outcome=workspace.value()->query(index.value(),ball(),4,nullptr,callback);
    CHECK_EQ(outcome.reason,callback==bad_alloc ? Reason::memory_budget :
                            callback==other_throw ? Reason::task_exception : Reason::size_mismatch);
    Count count;
    REQUIRE(workspace.value()->query(index.value(),ball(),4,&count,Count::call).ok()); CHECK_EQ(count.calls,1u);
  }
  Nested nested{*workspace.value(),index.value()};
  REQUIRE(workspace.value()->query(index.value(),ball(),4,&nested,Nested::call).ok());
  CHECK_EQ(nested.calls,1u); CHECK_EQ(nested.nested.reason,Reason::parameter_out_of_range);
  Hold hold; Outcome from_worker;
  std::thread worker([&] {
    from_worker=workspace.value()->query(index.value(),ball(),4,&hold,Hold::call);
    if (!hold.called) hold.entered.count_down();  // Un refus precoce doit echouer, pas bloquer la porte.
  });
  hold.entered.wait(); Count refused;
  const auto issue=workspace.value()->query(index.value(),ball(),1,&refused,Count::call);
  hold.release.count_down(); worker.join();
  CHECK_EQ(issue.reason,Reason::parameter_out_of_range); CHECK_EQ(refused.calls,0u);
  CHECK(from_worker.ok()); CHECK(hold.unchanged);
  Count recovered; CHECK(workspace.value()->query(index.value(),ball(),1,&recovered,Count::call).ok());
}

namespace {
struct Concurrent {
  const Census& expected;
  std::latch& entered;
  std::latch& release;
  bool called=false,correct=false;
  static Outcome call(void* raw,const BorrowedCensus& result) {
    auto& c=*static_cast<Concurrent*>(raw);
    c.called=true;
    c.entered.count_down(); c.release.wait();
    c.correct=c.expected.kind()==result.kind() && equal(c.expected.interior(),result.interior()) &&
              equal(c.expected.shell(),result.shell()) && twice(c.expected.ledger(),result.ledger());
    return {};
  }
};
}
MHGP11_TEST(concurrency, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(4*11*4),reference(MemoryBudget::kUnlimited);
  auto cloud=octa().prepare(owner); REQUIRE(cloud.ok());
  auto index=build_index(std::move(cloud.value()),{1},owner); REQUIRE(index.ok());
  const std::array<num::Sphere,4> spheres{ball(),num::Sphere::point(point(0,0,0)),ball(),ball()};
  const std::array<u32,4> thresholds{4,1,1,kNone};
  std::array<std::unique_ptr<CensusWorkspace>,4> workspaces;
  std::array<std::optional<Census>,4> expected;
  for (u32 i=0;i<4;++i) {
    auto made=CensusWorkspace::make(index.value(),work); REQUIRE(made.ok());
    workspaces[i]=std::move(made.value());
    auto result=census(index.value(),spheres[i],thresholds[i],reference); REQUIRE(result.ok());
    expected[i].emplace(std::move(result.value()));
  }
  CHECK_EQ(work.used(),176u);
  std::latch entered(4),release(1);
  std::array<Concurrent,4> contexts{{{*expected[0],entered,release},{*expected[1],entered,release},
                                     {*expected[2],entered,release},{*expected[3],entered,release}}};
  std::array<Outcome,4> outcomes;
  std::array<std::thread,4> threads;
  for (u32 i=0;i<4;++i) threads[i]=std::thread([&,i] {
    outcomes[i]=workspaces[i]->query(index.value(),spheres[i],thresholds[i],&contexts[i],Concurrent::call);
    if (!contexts[i].called) entered.count_down();
  });
  entered.wait(); release.count_down();
  for (auto& thread:threads) thread.join();
  for (u32 i=0;i<4;++i) { CHECK(outcomes[i].ok()); CHECK(contexts[i].correct); workspaces[i].reset(); }
  CHECK(work.released().ok());
}
MHGP11_TEST_MAIN()
