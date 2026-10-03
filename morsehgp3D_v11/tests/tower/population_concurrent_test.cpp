// Table de populations et ordres concurrents : meme descente que la reference, memes forets et verticales.
#include <random>
#include <set>

#include "regular_vertical_reuse_support.hpp"
#include "tower/forest_internal.hpp"
#include "tower/population_lookup.hpp"
#include "test.hpp"
using namespace regular_vertical_test;

namespace {
std::vector<Xyz> cloud(u32 count, u32 side, u32 seed, bool clustered) {
  std::mt19937 rng(seed);
  std::vector<Xyz> points;
  std::set<std::array<u32, 3>> seen;
  auto draw = [&rng](u32 bound) { return static_cast<u32>(rng() % bound); };
  while (points.size() < count) {
    const u32 centre = clustered ? side / 4 * draw(3) : 0, spread = clustered ? side / 8 : side;
    const std::array<u32, 3> p{centre + draw(spread), centre + draw(spread), draw(spread)};
    if (seen.insert(p).second) points.push_back({p[0], p[1], p[2]});
  }
  return points;
}

struct Fixture { std::vector<Xyz> points; Order k; };
std::vector<Fixture> fixtures() {
  return {{{{0,0,0},{2,0,0},{4,0,0}},3}, {{{0,0,0},{2,0,0},{0,2,0},{2,2,0}},4},
          {{{0,0,0},{2,0,0},{0,2,0},{2,2,0},{10,0,0},{12,0,0},{11,2,0}},3},
          {cloud(90, 64, 7, false), 5}, {cloud(120, 256, 11, true), 6}, {cloud(70, 16, 3, false), 4}};
}

FullParams params(u32 q, bool lookup, bool concurrent, bool reuse) {
  FullParams p = reuse_params(q, reuse ? 7u : 0u);
  p.reuse_regular_verticals = reuse;
  p.population_lookup = lookup;
  p.concurrent_orders = concurrent;
  return p;
}

std::vector<SiteIdx> population(const FullDomain& domain, BallIdx ball) {
  std::vector<SiteIdx> part;
  for (SiteIdx s : domain.catalogue().interior(ball)) part.push_back(s);
  for (SiteIdx s : domain.catalogue().shell(ball)) part.push_back(s);
  std::reverse(part.begin(), part.end());  // ordre libre : la table trie elle-meme
  return part;
}
}  // namespace

MHGP11_TEST(lemma, 1200) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  u64 hits = 0, compared = 0, misses = 0;
  for (const auto& fixture : fixtures()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(fixture.points);
    auto domain = domain_of(input, owner, fixture.k); REQUIRE(domain.ok());
    const auto& d = domain.value();
    for (sched::Pool* p : {static_cast<sched::Pool*>(nullptr), pool.value().get()}) {
      auto lookup = tower_detail::PopulationLookup::make(d, work, p); REQUIRE(lookup.ok());
      u64 eligible = 0;
      for (u32 b = 0; b < d.catalogue().balls(); ++b) {
        const auto& data = d.catalogue().balls_data()[b];
        const u32 k = data.p + data.m;
        if (k > fixture.k) continue;
        ++eligible;
        const auto part = population(d, BallIdx{b});
        auto hit = lookup.value().descend(part, k); REQUIRE(hit.ok()); REQUIRE(hit.value().has_value());
        const auto& fast = *hit.value();
        CHECK(fast.seed().ball() == std::optional<BallIdx>{BallIdx{b}}); CHECK_EQ(fast.seed().order(), k);
        CHECK_EQ(fast.ledger().population_hits, 1u); CHECK_EQ(fast.ledger().catalogue_hits, 1u);
        // Le lemme contre la descente de reference : meme graine, memes dates exactes.
        auto slow = descend(d, part, k, work); REQUIRE(slow.ok());
        CHECK(slow.value().seed() == fast.seed());
        CHECK_EQ(num::compare(slow.value().initial_level(), fast.initial_level()), 0);
        CHECK_EQ(num::compare(slow.value().terminal_level(), fast.terminal_level()), 0);
        ++hits; ++compared;
        if (part.size() > 2) {  // Une sous-partie stricte n'est pas une population de boule de meme cardinal.
          std::vector<SiteIdx> shorter(part.begin() + 1, part.end());
          auto other = lookup.value().descend(shorter, k - 1); REQUIRE(other.ok());
          if (other.value()) {
            const auto& found = d.catalogue().balls_data()[idx(*other.value()->seed().ball())];
            CHECK_EQ(u64{found.p} + found.m, u64{k - 1});
          } else ++misses;
        }
      }
      CHECK_EQ(lookup.value().entries(), eligible);
      auto single = lookup.value().descend(std::vector<SiteIdx>{SiteIdx{0}}, 1); REQUIRE(single.ok());
      REQUIRE(single.value().has_value());
      CHECK(single.value()->seed().site() == std::optional<SiteIdx>{SiteIdx{0}});
      auto foreign = lookup.value().descend(std::vector<SiteIdx>{SiteIdx{0}, SiteIdx{0}}, 2);
      REQUIRE(foreign.ok()); CHECK(!foreign.value().has_value());  // doublon : la reference refusera
    }
    CHECK_EQ(work.used(), 0u);
  }
  CHECK(hits > 400); CHECK(compared == hits); CHECK(misses > 0);
  std::printf("population_lemma hits=%llu misses=%llu\n", (unsigned long long)hits, (unsigned long long)misses);
}

MHGP11_TEST(equivalence, 5500) {
  auto p1 = sched::make_pool({1}), p4 = sched::make_pool({4}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok() && p4.ok() && p48.ok());
  u64 compared = 0, population_hits = 0;
  for (const auto& fixture : fixtures()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(fixture.points);
    auto baseline = tower_of(input, fixture.k, owner, work); REQUIRE(baseline.ok());
    const u64 held = work.used();
    for (auto* pool : {p1.value().get(), p4.value().get(), p48.value().get()})
      for (u32 q : {1u, 4096u}) for (unsigned mask = 1; mask < 8; ++mask) {
        const bool lookup = mask & 1u, concurrent = mask & 2u, reuse = mask & 4u;
        auto domain = domain_of(input, owner, fixture.k); REQUIRE(domain.ok());
        FullTimings times;
        auto full = build_full(std::move(domain.value()), work, &times, params(q, lookup, concurrent, reuse), pool);
        if (!full.ok())
          std::printf("refus q=%u mask=%u workers=%u raison=%s\n", q, mask, pool->size(),
                      std::string(reason_name(full.outcome().reason)).c_str());
        REQUIRE(full.ok());
        CHECK_EQ(times.concurrent_orders, concurrent); CHECK_EQ(times.population_lookup, lookup);
        for (Order k = 1; k <= fixture.k; ++k) {
          const auto& f = full.value().order(k);
          CHECK(same(f, baseline.value().order(k))); CHECK(structure(f));
          const auto& paid = f.ledger().descent;
          CHECK_EQ(paid.census_calls + paid.catalogue_hits + paid.singleton_hits, paid.steps);
          CHECK(paid.population_hits <= paid.catalogue_hits + paid.singleton_hits);
          if (!lookup) CHECK_EQ(paid.population_hits, 0u);
          if (lookup && k == 1) CHECK_EQ(paid.population_hits, paid.steps);
          population_hits += paid.population_hits; ++compared;
        }
      }
    CHECK_EQ(work.used(), held);
  }
  CHECK(compared > 1000); CHECK(population_hits > 1000);
  std::printf("population_concurrent_equivalence orders=%llu population_hits=%llu\n",
              (unsigned long long)compared, (unsigned long long)population_hits);
}

MHGP11_TEST(refusals, 10) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input(fixtures()[1].points);
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  {
    auto domain = domain_of(input, owner, 4); REQUIRE(domain.ok());
    FullParams p; p.concurrent_orders = true;  // Q=0 : aucune voie parallele a reutiliser
    auto refused = build_full(std::move(domain.value()), work, nullptr, p, pool.value().get());
    CHECK_EQ(refused.outcome().reason, Reason::parameter_out_of_range);
    CHECK_EQ(domain.value().catalogue().kmax(), 4u);  // domaine conserve sur refus
  }
  CHECK_EQ(work.used(), 0u);
  auto first = domain_of(input, owner, 4), second = domain_of(input, owner, 4);
  REQUIRE(first.ok() && second.ok());
  auto lookup = tower_detail::PopulationLookup::make(first.value(), work); REQUIRE(lookup.ok());
  const std::vector<SiteIdx> part{SiteIdx{0}, SiteIdx{1}};
  auto foreign = tower_detail::resolve_descent(second.value(), part, 2, work, nullptr, nullptr, &lookup.value());
  CHECK_EQ(foreign.outcome().reason, Reason::parameter_out_of_range);
  MemoryBudget tight(16);
  auto denied = tower_detail::PopulationLookup::make(first.value(), tight);
  CHECK_EQ(denied.outcome().reason, Reason::memory_budget); CHECK_EQ(tight.used(), 0u);
}

MHGP11_TEST_MAIN()
