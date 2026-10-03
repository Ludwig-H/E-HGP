// Images verticales identiques, mais graines strictes reutilisees et descentes effectivement evitees.
#include "regular_vertical_reuse_support.hpp"
#include "tower/forest_internal.hpp"
#include "test.hpp"
using namespace regular_vertical_test;

MHGP11_TEST(equivalence, 4500) {
  auto p1 = sched::make_pool({1}), p4 = sched::make_pool({4}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok()); REQUIRE(p4.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,3> pools{p1.value().get(),p4.value().get(),p48.value().get()};
  const u32 high = (u32{1} << kCoordBits) - 1;
  struct Fixture { std::vector<Xyz> points; Order k; };
  const std::vector<Fixture> fixtures{
    {{{0,0,0},{2,0,0},{4,0,0}},3}, {{{3,0,0},{0,3,0},{2,2,0}},3},
    {{{0,0,0},{2,0,0},{0,2,0},{2,2,0}},4},
    {{{0,0,0},{high,high,0},{high,0,high},{0,high,high}},4},
    {{{0,0,0},{2,0,0},{10,0,0},{12,0,0},{30,0,0},{32,0,0}},2},
    {{{0,0,0},{2,0,0},{0,2,0},{2,2,0},{10,0,0},{12,0,0},{11,2,0}},3}};
  u64 reused = 0, fallback = 0, memo_queries = 0, mixed_windows = 0, q4_reuses = 0;
  for (const auto& fixture : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(fixture.points); const Order kmax = fixture.k;
    auto baseline = tower_of(input,kmax,owner,work); REQUIRE(baseline.ok());
    const u64 held = work.used();
    std::array<std::array<std::array<ForestLedger,kMaxMebSites>,8>,4> expected{};
    for (u32 q : {0u,1u,2u,4096u}) for (auto* pool : pools) for (unsigned flags = 0; flags < 8; ++flags) {
      if ((q == 0 && pool->size() != 1) || (q != 0 && flags != 0 && flags != 7)) continue;
      auto domain = domain_of(input,owner,kmax); REQUIRE(domain.ok());
      const u64 table_bytes = 4 * domain.value().catalogue().balls();
      {
        FullTimings times;
        const auto params = reuse_params(q,flags);
        auto full = build_full(std::move(domain.value()),work,&times,params,q == 0 ? nullptr : pool);
        REQUIRE(full.ok()); CHECK(times.reuse_regular_verticals);
        CHECK_EQ(times.regular_vertical_reserved_bytes,table_bytes);
        CHECK_EQ(domain.value().catalogue().kmax(),0u);
        u64 bytes = held;
        for (Order k = 1; k <= kmax; ++k) {
          const auto& f = full.value().order(k); const auto& old = baseline.value().order(k);
          const auto& paid = f.ledger(); const u64 hits = regular_births(full.value().domain(),f);
          CHECK(same(f,old)); CHECK(structure(f)); CHECK(fixed_work(paid) == fixed_work(old.ledger()));
          CHECK_EQ(paid.vertical_reuses,hits);
          CHECK_EQ(paid.vertical_descents + paid.vertical_reuses,k == 1 ? 0u : f.births());
          CHECK_EQ(old.ledger().vertical_reuses,0u);
          CHECK_EQ(old.ledger().vertical_descents,k == 1 ? 0u : old.births());
          CHECK_EQ(paid.descent.memo.queries,params.memo_capacity == 0 ? 0u : paid.trace_resolutions+paid.vertical_descents);
          const unsigned route = q == 0 ? 0 : q == 1 ? 1 : q == 2 ? 2 : 3;
          if (pool->size() == 1) expected[route][flags][k-1] = paid;
          else CHECK(paid == expected[route][flags][k-1]);
          if (params.memo_capacity != 0) CHECK(counts(paid.descent));
          if (params.parallel_verticals) {
            CHECK_EQ(times.orders[k-1].vertical_resolutions,paid.vertical_descents);
            CHECK(vertical_bounds(times.orders[k-1],pool->size(),params.descent_lanes,q));
            if (q == 4096 && hits != 0 && paid.vertical_descents != 0) {
              CHECK_EQ(times.orders[k-1].vertical_batches,1u);
              CHECK_EQ(times.orders[k-1].max_vertical_batch,f.births());
              ++mixed_windows;
            }
          }
          bytes += retained_bytes(f); reused += hits; fallback += paid.vertical_descents;
          memo_queries += paid.descent.memo.queries;
          if (k > 1) for (u32 i = 0; i < f.births(); ++i) {
            const auto& b = full.value().domain().catalogue().balls_data()[f.nodes()[i].birth_key];
            q4_reuses += b.qmin == 4 && b.m == 4;
          }
        }
        CHECK_EQ(work.used(),bytes); CHECK(times.orders[kmax] == OrderTimings{});
      }
      CHECK_EQ(work.used(),held);
    }
    {
      auto domain = domain_of(input,owner,kmax); REQUIRE(domain.ok());
      auto untimed = build_full(std::move(domain.value()),work,nullptr,reuse_params(2,7),p4.value().get());
      REQUIRE(untimed.ok());
      for (Order k = 1; k <= kmax; ++k) {
        CHECK(same(untimed.value().order(k),baseline.value().order(k)));
        CHECK(untimed.value().order(k).ledger() == expected[2][7][k-1]);
      }
    }
    CHECK_EQ(work.used(),held);
  }
  CHECK(reused > 0); CHECK(fallback > 0); CHECK(memo_queries > 0); CHECK(mixed_windows > 0);
  CHECK(q4_reuses > 0);
}

MHGP11_TEST(closed_dates, 800) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  const u32 scale_high = ((u32{1} << kCoordBits)-1)/3;
  std::array<unsigned,3> axes{0,1,2};
  do {
    for (u32 scale : {1u,scale_high}) {
      std::vector<Xyz> points;
      for (Xyz p : {Xyz{3,0,0},Xyz{0,3,0},Xyz{2,2,0}})
        points.push_back({p[axes[0]]*scale,p[axes[1]]*scale,p[axes[2]]*scale});
      std::reverse(points.begin(),points.end());  // Ordre d'entree distinct du SiteIdx Morton.
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      const Input input(points); auto domain = domain_of(input,owner,3); REQUIRE(domain.ok());
      auto lower = build_forest(domain.value(),2,work); REQUIRE(lower.ok());
      auto upper = build_forest(domain.value(),3,work); REQUIRE(upper.ok());
      const i64 square_scale = i64{scale} * scale;
      const auto b = ball_at(domain.value(),1,2,2,9*square_scale,2);
      const auto& node = upper.value().nodes()[idx(birth_at(upper.value(),b))];
      CHECK_EQ(domain.value().catalogue().interior(b)[0],SiteIdx{2});
      CHECK_EQ(lower.value().births(),2u); CHECK_EQ(lower.value().nodes().size(),3u);
      const std::array<SiteIdx,2> historical{SiteIdx{0},SiteIdx{1}};
      auto old = descend(domain.value(),historical,2,work); REQUIRE(old.ok());
      CHECK(level_is(old.value().initial_level(),9*square_scale,2));
      std::array<NodeIdx,2> seeds{};
      for (u32 endpoint = 0; endpoint < 2; ++endpoint) {
        const std::array<SiteIdx,2> face{SiteIdx{endpoint},SiteIdx{2}};
        auto down = descend(domain.value(),face,2,work); REQUIRE(down.ok());
        CHECK(level_is(down.value().initial_level(),5*square_scale,4));
        auto seed = lower.value().birth_node(down.value().seed()); REQUIRE(seed.has_value());
        seeds[endpoint] = *seed;
        auto cache = RegularVerticalSeeds::make(domain.value(),work); REQUIRE(cache.ok());
        REQUIRE(cache.value().remember(lower.value(),b,*seed).ok());
        auto found = cache.value().find(lower.value(),node); REQUIRE(found.ok()); REQUIRE(found.value().has_value());
        CHECK_EQ(*found.value(),*seed); CHECK(*seed != lower.value().root());
        CHECK_EQ(lower.value().nodes()[idx(*seed)].parent,lower.value().root());
        CHECK_EQ(lower.value().nodes()[idx(lower.value().root())].rank,node.rank);
        u64 hops = 0;
        auto image = lower.value().ancestor_closed(*found.value(),node.rank,hops); REQUIRE(image.ok());
        CHECK_EQ(image.value(),lower.value().root()); CHECK_EQ(hops,1u);
      }
      CHECK(seeds[0] != seeds[1]);  // Deux terminaux distincts, une meme image seulement a la coupe fermee.
      for (u32 q : {0u,1u,2u,4096u}) {
        auto fresh = domain_of(input,owner,3); REQUIRE(fresh.ok());
        FullTimings times;
        auto full = build_full(std::move(fresh.value()),work,&times,reuse_params(q,7),q == 0 ? nullptr : pool.value().get());
        REQUIRE(full.ok()); const auto& high = full.value().order(3); const auto& low = full.value().order(2);
        CHECK_EQ(high.births(),1u); CHECK_EQ(high.lower()[0],low.root());
        CHECK_EQ(high.ledger().vertical_reuses,1u); CHECK_EQ(high.ledger().vertical_descents,0u);
        CHECK_EQ(times.orders[2].vertical_resolutions,0u); CHECK_EQ(times.orders[2].vertical_batches,0u);
        CHECK_EQ(times.orders[2].vertical_dispatch_ns,0u);
      }
    }
  } while (std::next_permutation(axes.begin(),axes.end()));
}

MHGP11_TEST(extended, 100) {
  const Input input({{0,0,0},{2,0,0},{0,2,0},{2,2,0}});
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
  const auto b = ball_at(domain.value(),0,4,2,2,1);
  auto cache = RegularVerticalSeeds::make(domain.value(),work); REQUIRE(cache.ok());
  for (Order k : {Order{3},Order{4}}) {
    auto low = build_forest(domain.value(),static_cast<Order>(k-1),work); REQUIRE(low.ok());
    auto high = build_forest(domain.value(),k,work); REQUIRE(high.ok());
    const auto& birth = high.value().nodes()[idx(birth_at(high.value(),b))];
    auto absent = cache.value().find(low.value(),birth); REQUIRE(absent.ok()); CHECK(!absent.value());
    CHECK(cache.value().remember(low.value(),b,NodeIdx{0}).ok());  // Une coquille etendue ne remplit pas la table.
    absent = cache.value().find(low.value(),birth); REQUIRE(absent.ok()); CHECK(!absent.value());
  }
  auto baseline = tower_of(input,4,owner,work); REQUIRE(baseline.ok());
  for (u32 workers : {1u,4u,48u}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    for (u32 q : {1u,2u,4096u}) {
      auto fresh = domain_of(input,owner,4); REQUIRE(fresh.ok()); FullTimings times;
      auto full = build_full(std::move(fresh.value()),work,&times,reuse_params(q,7),pool.value().get());
      REQUIRE(full.ok());
      for (Order k : {Order{3},Order{4}}) {
        const auto& f = full.value().order(k);
        CHECK(same(f,baseline.value().order(k))); CHECK_EQ(f.births(),1u);
        CHECK_EQ(f.ledger().vertical_reuses,0u); CHECK_EQ(f.ledger().vertical_descents,1u);
        CHECK_EQ(times.orders[k-1].vertical_resolutions,q == 1 ? 0u : 1u);
      }
      CHECK_EQ(full.value().order(2).ledger().vertical_descents,0u);
      CHECK_EQ(full.value().order(2).ledger().vertical_reuses,4u);
    }
  }
}

MHGP11_TEST(cache_contract, 40) {
  const Input input({{0,0,0},{2,0,0},{4,0,0},{20,0,0},{24,0,0},{40,0,0},{46,0,0}});
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(input,owner,3); REQUIRE(domain.ok());
  auto lower = build_forest(domain.value(),2,work); REQUIRE(lower.ok());
  auto upper = build_forest(domain.value(),3,work); REQUIRE(upper.ok());
  auto points = build_forest(domain.value(),1,work); REQUIRE(points.ok());
  const auto ball = ball_at(domain.value(),1,2,2,4,1);
  const auto& birth = upper.value().nodes()[idx(birth_at(upper.value(),ball))];
  const auto face = part_of(domain.value(),{{0,0,0},{2,0,0}});
  auto down = descend(domain.value(),face,2,work); REQUIRE(down.ok());
  auto seed = lower.value().birth_node(down.value().seed()); REQUIRE(seed.has_value());
  auto cache = RegularVerticalSeeds::make(domain.value(),work); REQUIRE(cache.ok());
  CHECK(cache.value().belongs_to(domain.value()));
  CHECK_EQ(cache.value().reserved_bytes(),4*domain.value().catalogue().balls());
  CHECK_EQ(cache.value().find(lower.value(),birth).outcome().reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().remember(lower.value(),BallIdx{kNone},*seed).reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().remember(lower.value(),ball,NodeIdx{kNone}).reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().remember(lower.value(),ball,lower.value().root()).reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().remember(points.value(),ball,NodeIdx{0}).reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().find(points.value(),birth).outcome().reason,Reason::tower_invariant);
  u64 rejected_dates = 0; bool equal_date = false, later_date = false;
  for (u32 i = 0; i < lower.value().births(); ++i)
    if (idx(lower.value().nodes()[i].rank) >= idx(birth.rank)) {
      CHECK_EQ(cache.value().remember(lower.value(),ball,NodeIdx{i}).reason,Reason::tower_invariant);
      equal_date = equal_date || lower.value().nodes()[i].rank == birth.rank;
      later_date = later_date || idx(lower.value().nodes()[i].rank) > idx(birth.rank);
      ++rejected_dates;
    }
  CHECK(rejected_dates >= 2); CHECK(equal_date); CHECK(later_date);
  CHECK_EQ(cache.value().find(lower.value(),birth).outcome().reason,Reason::tower_invariant);
  REQUIRE(cache.value().remember(lower.value(),ball,*seed).ok());
  CHECK_EQ(cache.value().remember(lower.value(),ball,*seed).reason,Reason::tower_invariant);
  auto wrong = birth; wrong.rank = LevelRank{kNone};
  CHECK_EQ(cache.value().find(lower.value(),wrong).outcome().reason,Reason::tower_invariant);
  wrong = birth; wrong.birth_key = kNone;
  CHECK_EQ(cache.value().find(lower.value(),wrong).outcome().reason,Reason::tower_invariant);
  auto moved = std::move(cache.value());
  CHECK(moved.belongs_to(domain.value())); CHECK(!cache.value().belongs_to(domain.value()));
  CHECK_EQ(cache.value().reserved_bytes(),0u);
  CHECK_EQ(cache.value().find(lower.value(),birth).outcome().reason,Reason::tower_invariant);
  CHECK_EQ(cache.value().remember(lower.value(),ball,*seed).reason,Reason::tower_invariant);
  auto found = moved.find(lower.value(),birth); REQUIRE(found.ok()); REQUIRE(found.value().has_value());
  CHECK_EQ(*found.value(),*seed);
  auto foreign = domain_of(input,owner,3); REQUIRE(foreign.ok()); CHECK(!moved.belongs_to(foreign.value()));
  const u64 held = work.used(); OrderTimings times{1,2,3,4}; const auto before = times;
  CHECK_EQ(forest_verticals(foreign.value(),lower.value(),upper.value(),work,nullptr,nullptr,&times,&moved).reason,
           Reason::parameter_out_of_range);
  CHECK_EQ(work.used(),held); CHECK(times == before); CHECK(upper.value().lower().empty());
}
MHGP11_TEST_MAIN()
