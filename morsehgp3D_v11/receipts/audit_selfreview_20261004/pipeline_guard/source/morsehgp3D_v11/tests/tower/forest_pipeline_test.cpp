// Pipeline des ordres concurrents : decisions du balayage suivi contre l'ordre sequentiel modele, puis memes forets
// que la voie sequentielle, memes verticales et compteurs (hors parcours census) que la voie par etages, a W48 repete.
#include <algorithm>
#include <chrono>
#include <random>
#include <set>
#include <thread>

#include "regular_vertical_reuse_support.hpp"
#include "tower/forest_internal.hpp"
#include "test.hpp"
using namespace regular_vertical_test;

namespace {
struct Model {
  std::vector<u32> births, merges;  // rangs croissants (au sens large)
  std::vector<u32> plateaus;        // rangs clos croissants stricts : rangs des fusions et plateaux sans fusion
};

Model draw(std::mt19937& rng) {
  Model m;
  std::uniform_int_distribution<u32> rank(1, 12), count(0, 6);
  for (u32 i = count(rng); i > 0; --i) m.births.push_back(rank(rng));
  for (u32 i = count(rng); i > 0; --i) m.merges.push_back(rank(rng));
  std::sort(m.births.begin(), m.births.end());
  std::sort(m.merges.begin(), m.merges.end());
  std::set<u32> closed(m.merges.begin(), m.merges.end());
  for (u32 i = count(rng); i > 0; --i) closed.insert(rank(rng));  // continuations : plateaux sans fusion
  m.plateaus.assign(closed.begin(), closed.end());
  return m;
}

// Ordre de sweep_all : naissance d'abord a rang egal ; vrai = naissance.
std::vector<bool> sequential(const Model& m) {
  std::vector<bool> order;
  u64 b = 0, f = 0;
  while (b < m.births.size() || f < m.merges.size()) {
    const bool birth = b < m.births.size() && (f == m.merges.size() || m.births[b] <= m.merges[f]);
    order.push_back(birth);
    (birth ? b : f) += 1;
  }
  return order;
}
}  // namespace

MHGP11_TEST(decisions, 20000) {
  std::mt19937 rng(20261003);
  u64 early_births = 0, merges = 0, waits = 0, ends = 0;
  for (int trial = 0; trial < 4000; ++trial) {
    const Model m = draw(rng);
    const auto order = sequential(m);
    // Etats annonces : apres chaque plateau clos (lot de un), puis la fin. published = fusions de rang <= clos.
    for (u64 state = 0; state <= m.plateaus.size(); ++state) {
      const bool done = state == m.plateaus.size();
      const u32 closed = state == 0 ? 0 : m.plateaus[state - 1] + 1;
      u64 published = 0;
      while (published < m.merges.size() && (done || m.merges[published] + 1 <= closed)) ++published;
      // Chaque prefixe du flux sequentiel dont les fusions consommees sont publiees.
      u64 b = 0, f = 0;
      for (u64 step = 0; step <= order.size(); ++step) {
        if (f > published) break;
        const bool birth_left = b < m.births.size(), merge_published = f < published;
        const auto got = tower_detail::follow_step(birth_left, LevelRank{birth_left ? m.births[b] : 0},
                                                   merge_published, LevelRank{merge_published ? m.merges[f] : 0},
                                                   done, closed);
        if (step == order.size()) {
          // Tout est consomme : fin seulement si la publication est finie, sinon attente.
          CHECK(got == (done ? tower_detail::FollowStep::end : tower_detail::FollowStep::wait));
          ends += got == tower_detail::FollowStep::end;
          break;
        }
        const bool next_birth = order[step];
        if (got == tower_detail::FollowStep::wait) {
          ++waits;
          CHECK(!done);  // une publication finie ne fait jamais attendre
        } else {
          CHECK(got != tower_detail::FollowStep::end);
          CHECK_EQ(got == tower_detail::FollowStep::birth, next_birth);
          if (!done) { if (next_birth) ++early_births; else ++merges; }
        }
        (next_birth ? b : f) += 1;
      }
    }
    // Activations basses : prete => toute fusion de rang <= level est publiee ; fin => prete.
    for (u64 state = 0; state <= m.plateaus.size(); ++state) {
      const bool done = state == m.plateaus.size();
      const u32 closed = state == 0 ? 0 : m.plateaus[state - 1] + 1;
      for (u32 level = 0; level <= 13; ++level) {
        const bool ready = tower_detail::follow_lower_ready(LevelRank{level}, done, closed);
        if (done) CHECK(ready);
        if (!ready) continue;
        for (const u32 r : m.merges)
          if (r <= level) CHECK(done || r + 1 <= closed);
      }
    }
  }
  CHECK(early_births > 1000); CHECK(merges > 1000); CHECK(waits > 1000); CHECK(ends > 1000);
  std::printf("pipeline_decisions early_births=%llu merges=%llu waits=%llu ends=%llu\n",
              (unsigned long long)early_births, (unsigned long long)merges, (unsigned long long)waits,
              (unsigned long long)ends);
}

namespace {
std::vector<Xyz> cloud(u32 count, u32 side, u32 seed, bool clustered) {
  std::mt19937 rng(seed);
  std::vector<Xyz> points;
  std::set<std::array<u32, 3>> seen;
  auto pick = [&rng](u32 bound) { return static_cast<u32>(rng() % bound); };
  while (points.size() < count) {
    const u32 centre = clustered ? side / 4 * pick(3) : 0, spread = clustered ? side / 8 : side;
    const std::array<u32, 3> p{centre + pick(spread), centre + pick(spread), pick(spread)};
    if (seen.insert(p).second) points.push_back({p[0], p[1], p[2]});
  }
  return points;
}

FullParams pipeline_params(bool reuse_census, bool lookup, bool dense) {
  FullParams p;
  p.regular_batch_capacity = 4096; p.descent_lanes = 48; p.lane_memo_capacity = 0; p.memo_capacity = 0;
  p.parallel_verticals = true; p.reuse_census_workspace = reuse_census; p.dense_birth_lookup = dense;
  p.reuse_regular_verticals = true; p.population_lookup = lookup; p.concurrent_orders = true;
  return p;
}

// Compteurs identiques hors parcours census (possede : deux passes ; espace reutilise : une).
ForestLedger without_census(ForestLedger ledger) {
  ledger.descent.census = CensusLedger{};
  return ledger;
}
}  // namespace

MHGP11_TEST(equivalence, 2000) {
  auto p1 = sched::make_pool({1}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok() && p48.ok());
  const std::vector<std::pair<std::vector<Xyz>, Order>> fixtures{
      {cloud(90, 64, 7, false), 5}, {cloud(140, 256, 11, true), 6}, {cloud(70, 16, 3, false), 4},
      {cloud(160, 1024, 29, true), 5}};
  u64 compared = 0, piped = 0;
  for (const auto& [points, k] : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(points);
    auto baseline = tower_of(input, k, owner, work);  // voie sequentielle : ni etages concurrents ni blocs
    REQUIRE(baseline.ok());
    for (unsigned mask = 0; mask < 8; ++mask) {
      const FullParams params = pipeline_params(mask & 1u, mask & 2u, mask & 4u);
      auto staged_domain = domain_of(input, owner, k); REQUIRE(staged_domain.ok());
      FullTimings staged_times;
      auto staged = build_full(std::move(staged_domain.value()), work, &staged_times, params, p1.value().get());
      REQUIRE(staged.ok());
      CHECK_EQ(staged_times.pipeline_lanes, 0u);
      for (int repeat = 0; repeat < 6; ++repeat) {
        auto domain = domain_of(input, owner, k); REQUIRE(domain.ok());
        FullTimings times;
        auto full = build_full(std::move(domain.value()), work, &times, params, p48.value().get());
        REQUIRE(full.ok());
        CHECK(times.pipeline_lanes > 0);
        piped += times.pipeline_lanes > 0;
        for (Order o = 1; o <= k; ++o) {
          const auto& f = full.value().order(o);
          const auto& g = staged.value().order(o);
          CHECK(same(f, g)); CHECK(same(f, baseline.value().order(o))); CHECK(structure(f));
          CHECK(without_census(f.ledger()) == without_census(g.ledger()));
          ++compared;
        }
      }
    }
  }
  CHECK(compared > 900); CHECK(piped > 150);
  std::printf("pipeline_equivalence orders=%llu pipelines=%llu\n", (unsigned long long)compared,
              (unsigned long long)piped);
}

namespace {
// Vue scriptee : chaque block() applique l'etat suivant du script, comme un reveil sur une nouvelle annonce.
struct ScriptedView {
  struct State { u32 closed; bool done, abandoned; };
  std::vector<State> script;
  u32 nodes = 0, closed = 0;
  bool done = false, abandoned = false;
  u64 blocks = 0;
  void block() noexcept {
    const State s = script[std::min<u64>(blocks, script.size() - 1)];
    ++blocks;
    closed = s.closed; done = s.done; abandoned = s.abandoned;
  }
};
}  // namespace

// Audit P1 du 4 octobre 2026 : un abandon publie pendant l'attente (closed = kNone, done faux) rend le predicat de
// pret vrai ; l'attente doit rendre faux. Vue scriptee (decisions exactes), puis vrai ForestProgress abandonne par un
// autre fil pendant que le balayage attend (appel nominal a std::atomic::wait).
MHGP11_TEST(abandon, 700) {
  using S = ScriptedView::State;
  u64 cases = 0;
  {  // abandon pendant l'attente : faux, apres un seul reveil
    ScriptedView v; v.script = {S{kNone, false, true}};
    CHECK(!tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 1u); ++cases;
  }
  {  // abandon deja visible : faux sans attente
    ScriptedView v; v.abandoned = true; v.closed = kNone;
    CHECK(!tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 0u); ++cases;
  }
  {  // deja pret : vrai sans attente
    ScriptedView v; v.closed = 9;
    CHECK(tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 0u); ++cases;
  }
  {  // annonces successives, puis pret : vrai apres trois reveils
    ScriptedView v; v.script = {S{3, false, false}, S{5, false, false}, S{6, false, false}};
    CHECK(tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 3u); ++cases;
  }
  {  // annonces puis abandon : faux
    ScriptedView v; v.script = {S{3, false, false}, S{kNone, false, true}};
    CHECK(!tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 2u); ++cases;
  }
  {  // fin normale pendant l'attente : vrai
    ScriptedView v; v.script = {S{kNone, true, false}};
    CHECK(tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 1u); ++cases;
  }
  // Vrai etat publie, abandon par un autre fil apres un delai variable (avant ou pendant l'attente).
  u64 threaded = 0;
  for (int trial = 0; trial < 256; ++trial) {
    tower_detail::ForestProgress state;
    state.closed.store(2, std::memory_order_release);  // une annonce : rangs < 2 clos
    std::thread publisher([&state, trial] {
      std::this_thread::sleep_for(std::chrono::microseconds(50 * (trial % 8)));
      state.finish(3, false);
    });
    tower_detail::ProgressView low{state};
    low.refresh();
    const bool ready = tower_detail::await_lower(low, LevelRank{7});
    publisher.join();
    CHECK(!ready); CHECK(low.abandoned); CHECK_EQ(low.closed, kNone);
    ++threaded;
  }
  CHECK_EQ(cases, 6u); CHECK_EQ(threaded, 256u);
  std::printf("pipeline_abandon cases=%llu threaded=%llu\n", (unsigned long long)cases,
              (unsigned long long)threaded);
}

MHGP11_TEST_MAIN()
