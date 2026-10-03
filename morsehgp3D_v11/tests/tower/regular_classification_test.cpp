// Voie reguliere contre le classificateur general, puis tableaux FULL Gamma/Fraction graves.
#include "regular_classification_truth.hpp"
#include "tower/forest_internal.hpp"
#include "sched/sched.hpp"
#include "test.hpp"
using namespace regular_classification_test;

namespace {
std::array<u64,10> fields(const ClassificationLedger& x) {
  return {x.combinations,x.examined,x.meb_calls,x.meb.presentations,x.meb.nondegenerate,
          x.meb.positive,x.meb.containing,x.meb.comparisons,x.meb.diameter_pairs,x.meb.point_tests};
}
ClassificationLedger ledger(const std::array<u64,10>& v) {
  ClassificationLedger x;
  x.combinations=v[0]; x.examined=v[1]; x.meb_calls=v[2];
  x.meb.presentations=v[3]; x.meb.nondegenerate=v[4]; x.meb.positive=v[5];
  x.meb.containing=v[6]; x.meb.comparisons=v[7]; x.meb.diameter_pairs=v[8]; x.meb.point_tests=v[9];
  return x;
}
Input input_of(const Fixture& f,u32 scale,bool reverse) {
  auto points=f.xyz;
  for (auto& p:points) for (auto& c:p) c*=scale;
  if (reverse) std::reverse(points.begin(),points.end());
  return Input(points);
}
struct General {
  std::vector<u8> kinds;
  ClassificationLedger work;
  u64 cells=0,births=0;
};
General general(const FullDomain& domain,u32 k) {
  General result;
  const auto& cat=domain.catalogue();
  result.kinds.resize(cat.balls());
  result.births=k==1?domain.index().cloud().sites():0;
  std::array<u64,10> sums{};
  for (u32 b=0;b<cat.balls();++b) {
    const auto& ball=cat.balls_data()[b];
    if (u64{ball.p}+ball.qmin-1>k || u64{ball.p}+ball.m<k) continue;
    auto made=classify_cell(domain,BallIdx{b},static_cast<Order>(k));
    if (!CHECK(made.ok())) return result;
    result.kinds[b]=made.value().kind()==CellKind::birth?1:2;
    ++result.cells;
    if (result.kinds[b]==1) ++result.births;
    const auto one=fields(made.value().ledger());
    for (u32 j=0;j<one.size();++j) sums[j]+=one[j];
  }
  result.work=ledger(sums);
  return result;
}
void check_tables(const OrderForest& f,const ExpectedOrder& wanted,bool verticals) {
  REQUIRE(f.nodes().size()==wanted.nodes.size());
  REQUIRE(f.edges().size()==wanted.edges.size());
  CHECK(structure(f));
  for (u32 i=0;i<f.nodes().size();++i) {
    const auto& n=f.nodes()[i]; const auto& v=wanted.nodes[i];
    CHECK_EQ(idx(n.rank),v[0]); CHECK_EQ(idx(n.parent),v[1]); CHECK_EQ(n.child_begin,v[2]);
    CHECK_EQ(n.child_count,v[3]); CHECK_EQ(n.birth_key,v[4]);
  }
  for (u32 i=0;i<f.edges().size();++i) CHECK_EQ(idx(f.edges()[i]),wanted.edges[i]);
  if (verticals) {
    REQUIRE(f.lower().size()==wanted.lower.size());
    for (u32 i=0;i<f.lower().size();++i) CHECK_EQ(idx(f.lower()[i]),wanted.lower[i]);
  }
}
void check_classification(const OrderForest& f,const General& expected) {
  CHECK_EQ(f.births(),expected.births); CHECK_EQ(f.ledger().classified_cells,expected.cells);
  const auto got=fields(f.ledger().classification), wanted=fields(expected.work);
  for (u32 j=0;j<got.size();++j) CHECK_EQ(got[j],wanted[j]);
}
}  // namespace

MHGP11_TEST(classification, 7000) {
  std::array<std::array<u64,2>,3> regular{};
  u64 extended_searches=0,regular_after_search=0;
  for (const auto& fixture:fixtures()) for (u32 scale:{1u,u32{1}<<(kCoordBits-4)})
    for (bool reverse:{false,true}) {
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      auto domain=domain_of(input_of(fixture,scale,reverse),owner,static_cast<int>(fixture.xyz.size()));
      REQUIRE(domain.ok());
      for (u32 k=1;k<=fixture.xyz.size();++k) {
        const auto expected=general(domain.value(),k);
        bool searched=false;
        for (u32 b=0;b<expected.kinds.size();++b) if (expected.kinds[b]!=0) {
          const auto& ball=domain.value().catalogue().balls_data()[b];
          if (ball.m==ball.qmin) {
            REQUIRE(ball.qmin>=2 && ball.qmin<=4);
            ++regular[ball.qmin-2][ball.p>0?1:0];
            if (searched) ++regular_after_search;
          } else if (k-ball.p>=ball.qmin && k-ball.p<ball.m) {
            ++extended_searches; searched=true;
          }
        }
        {
          ForestBuilder builder(domain.value(),k,work);
          for (u64 repetition:{u64{1},u64{2}}) {
            REQUIRE(builder.classify().ok());
            CHECK_EQ(builder.result.order(),k); CHECK_EQ(builder.result.births(),expected.births);
            CHECK_EQ(builder.kinds.size(),expected.kinds.size());
            CHECK_EQ(work.used(),domain.value().catalogue().balls());
            for (u32 b=0;b<expected.kinds.size();++b) CHECK_EQ(builder.kinds[b],expected.kinds[b]);
            auto summed=fields(expected.work);
            for (auto& v:summed) v*=repetition;
            const auto got=fields(builder.result.ledger().classification);
            for (u32 j=0;j<got.size();++j) CHECK_EQ(got[j],summed[j]);
            ForestLedger complete;
            complete.classified_cells=repetition*expected.cells; complete.classification=ledger(summed);
            CHECK(builder.result.ledger()==complete);  // Tous champs hors classification doivent rester nuls.
          }
        }
        CHECK(work.released().ok());
      }
    }
  for (const auto& q:regular) for (u64 count:q) CHECK(count>0);
  CHECK(extended_searches>0); CHECK(regular_after_search>0);
}

MHGP11_TEST(full_tables, 10000) {
  for (const auto& fixture:fixtures()) for (u32 scale:{1u,u32{1}<<(kCoordBits-4)})
    for (bool reverse:{false,true}) {
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      auto domain=domain_of(input_of(fixture,scale,reverse),owner,static_cast<int>(fixture.xyz.size()));
      REQUIRE(domain.ok());
      std::vector<General> expected;
      for (u32 k=1;k<=fixture.xyz.size();++k) {
        expected.push_back(general(domain.value(),k));
        auto single=build_forest(domain.value(),k,work); REQUIRE(single.ok());
        check_tables(single.value(),fixture.orders[k-1],false); check_classification(single.value(),expected.back());
      }
      CHECK(work.released().ok());
      auto full=build_full(std::move(domain.value()),work); REQUIRE(full.ok());
      CHECK_EQ(full.value().kmax(),fixture.xyz.size());
      for (Order k=1;k<=full.value().kmax();++k) {
        check_tables(full.value().order(k),fixture.orders[k-1],true);
        check_classification(full.value().order(k),expected[k-1]);
      }
    }
}

MHGP11_TEST(options, 1400) {
  // Les variantes changent les descentes payees, jamais le travail du classificateur.
  for (const auto& fixture:fixtures()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto base=tower_of(input_of(fixture,1,false),static_cast<int>(fixture.xyz.size()),owner,work);
    REQUIRE(base.ok());
    for (u32 workers:{1u,4u}) for (bool enabled:{false,true}) {
      auto pool=sched::make_pool({workers}); REQUIRE(pool.ok());
      auto domain=domain_of(input_of(fixture,1,true),owner,static_cast<int>(fixture.xyz.size()));
      REQUIRE(domain.ok());
      FullParams p;
      p.regular_batch_capacity=2; p.descent_lanes=4; p.parallel_verticals=true;
      p.memo_capacity=enabled?8:0; p.lane_memo_capacity=enabled?8:0;
      p.reuse_census_workspace=enabled; p.dense_birth_lookup=enabled; p.reuse_regular_verticals=enabled;
      FullTimings times;
      auto full=build_full(std::move(domain.value()),work,&times,p,pool.value().get()); REQUIRE(full.ok());
      for (Order k=1;k<=full.value().kmax();++k) {
        const auto& actual=full.value().order(k); const auto& baseline=base.value().order(k);
        CHECK(same(actual,baseline));
        CHECK_EQ(actual.ledger().classified_cells,baseline.ledger().classified_cells);
        const auto a=fields(actual.ledger().classification), b=fields(baseline.ledger().classification);
        for (u32 j=0;j<a.size();++j) CHECK_EQ(a[j],b[j]);
      }
    }
  }
}
MHGP11_TEST_MAIN()
