// Table inverse possedee : cles synthetiques fabriquees validement, geometrie et budgets ALL K inchanges.
#include "census_reuse_support.hpp"
#include "tower/forest_internal.hpp"
#include "test.hpp"
using namespace census_reuse_test;
static_assert(sizeof(NodeIdx) == 4 && sizeof(BirthEntry) == 8 && sizeof(ForestNode) == 24);

namespace {
u64 dense_bytes(const FullDomain& domain, Order k) {
  return 4 * u64{k == 1 ? domain.index().cloud().sites() : domain.catalogue().balls()};
}
u64 dense_retained(const OrderForest& f) {
  return 24*f.node_capacity()+4*f.edge_capacity()+f.lookup_reserved_bytes()+
         (f.lower().empty() ? 0 : 4*f.node_capacity());
}
Result<DescentResult> birth_seed(const FullDomain& domain, const OrderForest& forest, u32 i,
                                MemoryBudget& work) {
  std::vector<SiteIdx> part;
  const u32 key = forest.nodes()[i].birth_key;
  if (forest.order() == 1) part.push_back(SiteIdx{key});
  else {
    const auto inner = domain.catalogue().interior(BallIdx{key}), shell = domain.catalogue().shell(BallIdx{key});
    part.insert(part.end(),inner.begin(),inner.end()); part.insert(part.end(),shell.begin(),shell.end());
    std::sort(part.begin(),part.end()); part.resize(forest.order());
  }
  return descend(domain,part,forest.order(),work);
}
void check_births(const FullDomain& domain, const OrderForest& sparse, const OrderForest& dense) {
  MemoryBudget queries(MemoryBudget::kUnlimited);
  CHECK(same(sparse,dense)); CHECK(sparse.ledger() == dense.ledger()); CHECK(structure(dense));
  CHECK(!sparse.dense_birth_lookup()); CHECK(dense.dense_birth_lookup());
  CHECK_EQ(sparse.lookup_reserved_bytes(),8*u64{sparse.births()});
  CHECK_EQ(dense.lookup_reserved_bytes(),dense_bytes(domain,dense.order()));
  for (u32 i = 0; i < dense.births(); ++i) {
    auto seed = birth_seed(domain,dense,i,queries); REQUIRE(seed.ok());
    CHECK(dense.birth_node(seed.value().seed()) == NodeIdx{i});
    CHECK(sparse.birth_node(seed.value().seed()) == NodeIdx{i});
    CHECK(queries.released().ok());
  }
}
Input uniform_line(u32 n) {
  std::vector<Xyz> xyz;
  for (u32 i = 0; i < n; ++i) xyz.push_back({2*i,0,0});
  return Input(xyz);
}
}  // namespace

MHGP11_TEST(lookup, 300) {
  const std::vector<std::vector<Xyz>> fixtures{
    {{8,0,0},{0,8,0},{0,0,8},{0,0,0},{0,8,8}},
    {{0,0,0},{2,0,0},{4,0,0},{6,0,0},{8,0,0},{10,0,0}},
    {{0,1,0},{1,0,0},{2,1,0},{1,2,0}}, {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}};
  u64 checked = 0, nonidentity = 0;
  for (auto xyz : fixtures) for (u32 factor : {1u,u32{1} << (kCoordBits-4)}) {
    if (factor != 1) { for (auto& point : xyz) for (auto& v : point) v *= factor; std::reverse(xyz.begin(),xyz.end()); }
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(Input(xyz),owner,4); REQUIRE(domain.ok());
    for (u32 k = 1; k <= 4; ++k) {
      auto sparse = build_forest(domain.value(),k,work);
      auto dense = build_forest(domain.value(),k,work,nullptr,nullptr,nullptr,true);
      REQUIRE(sparse.ok()); REQUIRE(dense.ok()); check_births(domain.value(),sparse.value(),dense.value());
      checked += dense.value().births();
      for (u32 i = 0; i < dense.value().births(); ++i) nonidentity += dense.value().nodes()[i].birth_key != i ? 1u : 0u;
    }
  }
  CHECK(checked >= 64); CHECK(nonidentity > 0);
}

MHGP11_TEST(holes, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), queries(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,4,0},{4,0,0},{4,4,0}}),owner,2); REQUIRE(domain.ok());
  auto sparse = build_forest(domain.value(),2,work);
  auto dense = build_forest(domain.value(),2,work,nullptr,nullptr,nullptr,true);
  REQUIRE(sparse.ok()); REQUIRE(dense.ok());
  const u32 m = domain.value().catalogue().balls(); CHECK_EQ(m,3u);
  // These seeds only supply scalar keys. BirthSeed has no owner certificate; no cross-domain geometry claim.
  auto keys = domain_of(uniform_line(m+2),owner,2); REQUIRE(keys.ok());
  u64 absent = 0, present = 0;
  for (u32 key = 0; key <= m; ++key) {
    const std::array<SiteIdx,2> pair{SiteIdx{key},SiteIdx{key+1}};
    auto seed = descend(keys.value(),pair,2,queries); REQUIRE(seed.ok());
    CHECK(seed.value().seed().ball() == BallIdx{key});
    std::optional<NodeIdx> expected;
    for (u32 i = 0; i < dense.value().births(); ++i)
      if (dense.value().nodes()[i].birth_key == key) expected = NodeIdx{i};
    CHECK(dense.value().birth_node(seed.value().seed()) == expected);
    CHECK(sparse.value().birth_node(seed.value().seed()) == expected);
    if (key < m) { present += expected ? 1u : 0u; absent += expected ? 0u : 1u; }
  }
  CHECK_EQ(absent,1u); CHECK_EQ(present,2u); CHECK(queries.released().ok());
  auto one = build_forest(domain.value(),1,work,nullptr,nullptr,nullptr,true); REQUIRE(one.ok());
  const std::array<SiteIdx,1> unknown{SiteIdx{domain.value().index().cloud().sites()}};
  auto outside = descend(keys.value(),unknown,1,queries); REQUIRE(outside.ok());
  CHECK(!one.value().birth_node(outside.value().seed())); CHECK(!dense.value().birth_node(outside.value().seed()));
  auto seed = birth_seed(domain.value(),dense.value(),0,queries); REQUIRE(seed.ok());
  CHECK(!one.value().birth_node(seed.value().seed()));
  const auto* address = dense.value().nodes().data(); const u64 bytes = work.used();
  OrderForest moved(std::move(dense.value()));
  CHECK(moved.nodes().data() == address); CHECK_EQ(work.used(),bytes);
  CHECK(!dense.value().birth_node(seed.value().seed())); CHECK(!dense.value().dense_birth_lookup());
  CHECK_EQ(dense.value().lookup_reserved_bytes(),0u); CHECK_EQ(dense.value().order(),0u);
  CHECK(moved.birth_node(seed.value().seed()) == NodeIdx{0}); CHECK_EQ(moved.lookup_reserved_bytes(),4*u64{m});
  auto singleton = domain_of(Input({{1,2,3}}),owner,1); REQUIRE(singleton.ok());
  auto single = build_forest(singleton.value(),1,work,nullptr,nullptr,nullptr,true); REQUIRE(single.ok());
  auto sole = birth_seed(singleton.value(),single.value(),0,queries); REQUIRE(sole.ok());
  CHECK(single.value().birth_node(sole.value().seed()) == NodeIdx{0}); CHECK_EQ(single.value().lookup_reserved_bytes(),4u);
}

MHGP11_TEST(full, 250) {
  auto p1 = sched::make_pool({1}), p48 = sched::make_pool({48}); REQUIRE(p1.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,2> pools{p1.value().get(),p48.value().get()};
  for (const auto& xyz : {std::vector<Xyz>{{0,1,0},{1,0,0},{2,1,0},{1,2,0}},
                         std::vector<Xyz>{{0,0,0},{2,2,0},{2,0,2},{0,2,2}}}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    for (auto* pool : pools) for (u64 memo : {u64{0},u64{8}}) for (bool scratch : {false,true}) {
      const Input input(xyz); FullParams params{memo,2,48,memo,true,scratch,false};
      auto a = domain_of(input,owner,4), b = domain_of(input,owner,4); REQUIRE(a.ok()); REQUIRE(b.ok());
      auto sparse = build_full(std::move(a.value()),work,nullptr,params,pool); REQUIRE(sparse.ok());
      const u64 before = work.used(); params.dense_birth_lookup = true;
      FullTimings times = scratch_sentinel();
      auto dense = build_full(std::move(b.value()),work,&times,params,pool); REQUIRE(dense.ok());
      u64 retained_bytes = 0, lookups = 0;
      for (Order k = 1; k <= 4; ++k) {
        check_births(dense.value().domain(),sparse.value().order(k),dense.value().order(k));
        retained_bytes += dense_retained(dense.value().order(k));
        lookups += dense.value().order(k).lookup_reserved_bytes();
      }
      CHECK_EQ(lookups,4*(u64{xyz.size()}+3*dense.value().domain().catalogue().balls()));
      CHECK_EQ(work.used(),before+retained_bytes); CHECK(times.parallel_verticals);
      CHECK_EQ(times.census_workspaces,scratch ? std::min(pool->size(),2u) : 0u);
      CHECK_EQ(b.value().catalogue().kmax(),0u);
    }
  }
}

MHGP11_TEST(prefix, 40) {
  const std::array<Input,2> inputs{Input({{8,0,0},{0,8,0},{0,0,8},{0,0,0},{0,8,8}}),
                                 Input({{0,0,0},{2,0,0},{5,0,0},{9,0,0}})};
  for (u32 j = 0; j < inputs.size(); ++j) {
    const u32 k = j+1, b = k == 1 ? 5 : 3;
    MemoryBudget owner(MemoryBudget::kUnlimited);
    auto domain = domain_of(inputs[j],owner,static_cast<int>(k)); REQUIRE(domain.ok());
    const u64 m = domain.value().catalogue().balls(), mapping = dense_bytes(domain.value(),static_cast<Order>(k));
    const u64 retained_bytes = 24*(2*u64{b}-1)+4*(2*u64{b}-2)+mapping;
    const u64 scratch = k == 1 ? 8*u64{b} : 0;  // Distinct levels in the K2 fixture: no cohort scratch.
    const u64 exact = 17+m+retained_bytes+scratch;
    for (u64 deficit : {u64{0},u64{1}}) {
      MemoryBudget work(exact-deficit); Buffer<u8> prior; REQUIRE(prior.allocate(17,work).ok());
      std::fill(prior.span().begin(),prior.span().end(),u8{91}); const auto* saved = prior.data();
      {
        ForestBuilder prefix(domain.value(),k,work,nullptr,nullptr,nullptr,true);
        REQUIRE(prefix.classify().ok()); CHECK_EQ(work.used(),17+m);
        const auto out = prefix.births();
        if (deficit == 0) {
          REQUIRE(out.ok()); CHECK_EQ(work.peak(),exact); CHECK_EQ(work.used(),17+m+retained_bytes);
          CHECK_EQ(prefix.result.lookup_reserved_bytes(),mapping); CHECK(prefix.result.dense_birth_lookup());
          CHECK_EQ(prefix.result.node_capacity(),2*u64{b}-1); CHECK_EQ(prefix.result.nodes().size(),b);
        } else {
          CHECK_EQ(out.reason,Reason::memory_budget); CHECK_EQ(work.used(),17+m);
          CHECK_EQ(work.peak(),17+m); CHECK_EQ(prefix.result.node_capacity(),0u);
        }
        CHECK(prior.data() == saved);
        CHECK(std::all_of(prior.span().begin(),prior.span().end(),[](u8 value) { return value == 91; }));
      }
      CHECK_EQ(work.used(),17u);
    }
  }
}

MHGP11_TEST(memory, 50) {
  const auto input = uniform_line(12);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(input,owner,12); REQUIRE(domain.ok()); CHECK_EQ(domain.value().catalogue().balls(),66u);
  auto pool = sched::make_pool({1}); REQUIRE(pool.ok());
  const FullParams params{8,2,4,8,true,true,true};
  u64 retained_bytes = 0;
  for (u64 k = 1; k <= 12; ++k) {
    const u64 b = 13-k;
    retained_bytes += 24*(2*b-1)+4*(2*b-2)+4*(k == 1 ? 12 : 66)+(k == 1 ? 0 : 4*(2*b-1));
  }
  u64 peak = 0;
  {
    Buffer<u8> prior; REQUIRE(prior.allocate(17,work).ok());
    auto made = build_full(std::move(domain.value()),work,nullptr,params,pool.value().get()); REQUIRE(made.ok());
    CHECK_EQ(work.used(),17+retained_bytes); peak = work.peak();
    u64 lookup_bytes = 0;
    for (Order k = 1; k <= 12; ++k) {
      CHECK_EQ(made.value().order(k).births(),13u-k);
      CHECK(made.value().order(k).dense_birth_lookup());
      lookup_bytes += made.value().order(k).lookup_reserved_bytes();
    }
    CHECK_EQ(lookup_bytes,4u*(12u+11u*66u)); CHECK(peak > work.used());
  }
  CHECK(work.released().ok());
  for (u64 deficit : {u64{1},u64{0}}) {
    MemoryBudget exact(peak-deficit); Buffer<u8> prior; REQUIRE(prior.allocate(17,exact).ok());
    std::fill(prior.span().begin(),prior.span().end(),u8{91}); const auto* saved_prior = prior.data();
    auto next = domain_of(input,owner,12); REQUIRE(next.ok());
    const auto* saved = next.value().index().cloud().x().data(); const u64 held = owner.used();
    auto times = scratch_sentinel(); const auto before = times;
    {
      auto made = build_full(std::move(next.value()),exact,&times,params,pool.value().get());
      if (deficit) {
        CHECK_EQ(made.outcome().reason,Reason::memory_budget); CHECK(times == before);
        CHECK_EQ(exact.used(),17u); CHECK_EQ(owner.used(),held);
        CHECK(next.value().index().cloud().x().data() == saved);
      } else { REQUIRE(made.ok()); CHECK_EQ(exact.used(),17+retained_bytes); CHECK_EQ(exact.peak(),peak); }
      CHECK(prior.data() == saved_prior);
      CHECK(std::all_of(prior.span().begin(),prior.span().end(),[](u8 value) { return value == 91; }));
    }
    CHECK_EQ(exact.used(),17u);
  }
}
MHGP11_TEST_MAIN()
