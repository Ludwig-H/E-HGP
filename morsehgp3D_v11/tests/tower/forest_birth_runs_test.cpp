// Attendus Gamma/Fraction graves dans forest_birth_runs_model.py ; aucun tri produit utilise comme oracle.
#include "forest_support.hpp"
#include "tower/forest_internal.hpp"
#include "sched/sched.hpp"
#include "test.hpp"
using namespace forest_test;

namespace {
struct Fixture {
  std::vector<Xyz> xyz;
  u32 order;
  u64 balls, nodes, edges, presentations, scratch;
  std::vector<u32> keys;
  std::vector<std::array<i64,2>> levels;
};
Fixture points() {
  return {{{8,0,0},{0,8,0},{0,0,8},{0,0,0},{0,8,8}},1,0,0,0,0,0,
          {0,3,2,4,1},{{0,1},{0,1},{0,1},{0,1},{0,1}}};
}
Fixture right() {
  return {{{0,4,0},{4,0,0},{4,4,0}},2,3,3,2,2,2,{1,0},{{4,1},{4,1}}};
}
Fixture distinct() {
  return {{{0,0,0},{2,0,0},{5,0,0},{9,0,0}},2,5,5,4,0,0,{0,1,2},{{1,1},{9,4},{4,1}}};
}
Fixture interrupted() {
  return {{{0,0,0},{4,0,0},{6,0,0},{8,0,0},{12,0,0}},2,7,6,5,4,2,{0,1,2,4},
          {{1,1},{1,1},{4,1},{4,1}}};
}
Fixture interrupted_maximum() {
  return {{{0,0,0},{4,0,0},{6,0,0},{8,0,0},{12,0,0},{16,0,0}},2,9,8,7,5,3,{0,1,2,4,5},
          {{1,1},{1,1},{4,1},{4,1},{4,1}}};
}
Fixture extended() {
  return {{{0,12,0},{2,10,0},{4,12,0},{2,14,0},{10,2,0},{12,0,0},{14,2,0},{12,4,0}},
          3,19,4,3,2,2,{9,8,12},{{4,1},{4,1},{34,1}}};
}
Fixture scaled(Fixture f,u32 factor,bool reverse) {
  for (auto& point : f.xyz) for (auto& v : point) v *= factor;
  for (auto& level : f.levels) level[0] *= i64{factor} * factor;
  if (reverse) std::reverse(f.xyz.begin(),f.xyz.end());
  return f;
}
void check_births(const FullDomain& domain,const OrderForest& forest,const Fixture& fixture) {
  REQUIRE(forest.births() == fixture.keys.size());
  for (u32 i = 0; i < forest.births(); ++i) {
    const auto& node = forest.nodes()[i];
    CHECK_EQ(node.birth_key,fixture.keys[i]); CHECK_EQ(node.child_count,0u);
    CHECK(level_is(domain.catalogue().levels()[idx(node.rank)],fixture.levels[i][0],fixture.levels[i][1]));
  }
  CHECK_EQ(forest.ledger().birth_presentations,fixture.presentations);
  if (fixture.presentations == 0) CHECK_EQ(forest.ledger().center_comparisons,0u);
  else CHECK(forest.ledger().center_comparisons > 0);
}
void check_lookup(const FullDomain& domain,const OrderForest& forest) {
  MemoryBudget queries(MemoryBudget::kUnlimited);
  for (u32 i = 0; i < forest.births(); ++i) {
    std::vector<SiteIdx> part;
    if (forest.order() == 1) part.push_back(SiteIdx{forest.nodes()[i].birth_key});
    else {
      const BallIdx ball{forest.nodes()[i].birth_key};
      const auto inner = domain.catalogue().interior(ball), shell = domain.catalogue().shell(ball);
      part.insert(part.end(),inner.begin(),inner.end()); part.insert(part.end(),shell.begin(),shell.end());
      std::sort(part.begin(),part.end()); part.resize(forest.order());
    }
    auto seed = descend(domain,part,forest.order(),queries); REQUIRE(seed.ok());
    CHECK(forest.birth_node(seed.value().seed()) == NodeIdx{i}); CHECK(queries.released().ok());
  }
}
}  // namespace

MHGP11_TEST(points, 140) {
  for (u32 factor : {1u,(u32{1} << (kCoordBits-4))}) for (bool reverse : {false,true}) {
    const auto f = scaled(points(),factor,reverse); const Input input(f.xyz);
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(input,owner,1); REQUIRE(domain.ok());
    auto forest = build_forest(domain.value(),1,work); REQUIRE(forest.ok());
    CHECK(structure(forest.value())); check_births(domain.value(),forest.value(),f);
    check_lookup(domain.value(),forest.value());
  }
}

MHGP11_TEST(cohorts, 270) {
  for (auto original : {right(),distinct(),interrupted(),interrupted_maximum()}) for (bool reverse : {false,true}) {
    const auto f = scaled(original,1,reverse); const Input input(f.xyz);
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(input,owner,static_cast<int>(f.order)); REQUIRE(domain.ok());
    CHECK_EQ(domain.value().catalogue().balls(),f.balls);
    auto forest = build_forest(domain.value(),f.order,work); REQUIRE(forest.ok());
    CHECK(structure(forest.value())); CHECK_EQ(forest.value().nodes().size(),f.nodes);
    CHECK_EQ(forest.value().edges().size(),f.edges); check_births(domain.value(),forest.value(),f);
    check_lookup(domain.value(),forest.value());
    if (f.balls == 7 || f.balls == 9) {
      // Une non-naissance separe les deux naissances du meme rang dans le catalogue.
      auto left = classify_cell(domain.value(),BallIdx{2},2), middle = classify_cell(domain.value(),BallIdx{3},2),
           last = classify_cell(domain.value(),BallIdx{4},2);
      REQUIRE(left.ok()); REQUIRE(middle.ok()); REQUIRE(last.ok());
      CHECK_EQ(left.value().kind(),CellKind::birth); CHECK_EQ(middle.value().kind(),CellKind::strict_traces);
      CHECK_EQ(last.value().kind(),CellKind::birth);
      const auto balls = domain.value().catalogue().balls_data();
      CHECK_EQ(balls[2].rank,balls[3].rank); CHECK_EQ(balls[3].rank,balls[4].rank);
    }
  }
}

MHGP11_TEST(extended, 145) {
  for (u32 factor : {1u,u32{1} << (kCoordBits-5)}) for (bool reverse : {false,true}) {
    const auto f = scaled(extended(),factor,reverse); const Input input(f.xyz);
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(input,owner,3); REQUIRE(domain.ok()); CHECK_EQ(domain.value().catalogue().balls(),19u);
    auto forest = build_forest(domain.value(),3,work); REQUIRE(forest.ok());
    CHECK(structure(forest.value())); CHECK_EQ(forest.value().nodes().size(),4u);
    CHECK_EQ(forest.value().edges().size(),3u); check_births(domain.value(),forest.value(),f);
    for (u32 key : f.keys) {
      const auto& ball = domain.value().catalogue().balls_data()[key];
      CHECK_EQ(ball.p,0u); CHECK_EQ(ball.m,4u); CHECK_EQ(ball.qmin,2u);
    }
    CHECK_EQ(forest.value().nodes()[idx(forest.value().root())].child_count,3u);
    CHECK(level_is(domain.value().catalogue().levels()[idx(forest.value().nodes()[idx(forest.value().root())].rank)],
                   i64{50}*factor*factor,1));
    check_lookup(domain.value(),forest.value());
  }
}

MHGP11_TEST(memory, 220) {
  // ABI qualifiee des profils courants, pas engagement de layout public : Sphere puis deux mots u32,
  // arrondis a son alignement16. Un tableau de naissances complet ferait echouer les seuils singleton/max2/max3.
  const u64 record_bytes = kCoordBits == 18 ? 144 : kCoordBits == 21 ? 160 : 176;
  CHECK_EQ(sizeof(num::Sphere)+16,record_bytes); CHECK_EQ(alignof(num::Sphere),16u);
  for (const auto& f : {points(),right(),distinct(),interrupted(),interrupted_maximum(),extended()}) {
    MemoryBudget owner(MemoryBudget::kUnlimited);
    auto domain = domain_of(Input(f.xyz),owner,static_cast<int>(f.order)); REQUIRE(domain.ok());
    const u64 b = f.keys.size(), retained_bytes = 24*(2*b-1)+4*(2*b-2)+8*b;
    const u64 held = 17, kinds = domain.value().catalogue().balls();
    const u64 exact = held+kinds+retained_bytes+f.scratch*record_bytes;
    for (u64 deficit : {u64{0},u64{1}}) {
      MemoryBudget work(exact-deficit); Buffer<u8> prior;
      REQUIRE(prior.allocate(held,work).ok()); std::fill(prior.span().begin(),prior.span().end(),u8{91});
      const auto* address = prior.data();
      {
        ForestBuilder prefix(domain.value(),f.order,work);
        REQUIRE(prefix.classify().ok()); CHECK_EQ(work.used(),held+kinds);
        const auto result = prefix.births();
        if (deficit == 0) {
          REQUIRE(result.ok()); CHECK_EQ(work.peak(),exact); CHECK_EQ(work.used(),held+kinds+retained_bytes);
          CHECK_EQ(prefix.result.nodes().size(),b); CHECK_EQ(prefix.result.node_capacity(),2*b-1);
          CHECK_EQ(prefix.result.edge_capacity(),2*b-2); check_births(domain.value(),prefix.result,f);
        } else {
          CHECK_EQ(result.reason,Reason::memory_budget); CHECK_EQ(work.used(),held+kinds);
          CHECK_EQ(work.peak(),held+kinds); CHECK_EQ(prefix.result.node_capacity(),0u);
        }
        CHECK(prior.data() == address); CHECK(std::all_of(prior.span().begin(),prior.span().end(),[](u8 x){return x==91;}));
      }
      CHECK_EQ(work.used(),held);
    }
  }
}

MHGP11_TEST(parallel_memo, 55) {
  const auto f = extended(); const Input input(f.xyz);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto baseline = tower_of(input,3,owner,work); REQUIRE(baseline.ok());
  for (u32 workers : {1u,4u}) for (u64 memo : {u64{0},u64{8}}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    auto domain = domain_of(input,owner,3); REQUIRE(domain.ok());
    FullTimings times;
    auto full = build_full(std::move(domain.value()),work,&times,FullParams{memo,2,4,memo,true},pool.value().get());
    REQUIRE(full.ok()); CHECK(times.parallel_verticals);
    for (Order k = 1; k <= 3; ++k) {
      CHECK(same(full.value().order(k),baseline.value().order(k)));
      CHECK_EQ(full.value().order(k).ledger().birth_presentations,baseline.value().order(k).ledger().birth_presentations);
      CHECK_EQ(full.value().order(k).ledger().center_comparisons,baseline.value().order(k).ledger().center_comparisons);
    }
    CHECK_EQ(full.value().order(3).ledger().birth_presentations,2u);
  }
}
MHGP11_TEST_MAIN()
